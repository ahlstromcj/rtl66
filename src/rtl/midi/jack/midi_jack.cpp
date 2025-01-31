/*
 *  This file is part of rtl66.
 *
 *  rtl66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  rtl66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with rtl66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          midi_jack.cpp
 *
 *    Implements the JACK MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-01-30
 * \license       See above.
 *
 *  Engine candidates:
 *
 *      -   engine_connect()        [jack_client_open()]
 *      -   engine_activate()       [jack_activate(jackclient)]
 *      -   engine_deactivate()     [jack_deactivate(jackclient)]
 *      -   engine_disconnect()     [delete_port(): jack_client_close() call]
 *      -   reuse_connection()
 *      -   connect()               [jack_client_open() and callbacks setting]
 *      -   get_port_count()      * [jack_get_ports()]
 *      -   get_io_port_info()      [jack_get_ports()]
 *      -   get_port_name()       * [jack_get_ports()]
 *      -   get_port_alias()        [jack_port_by_name()]
 *
 *  Warnings *:
 *
 *      -   initialize() calls connect(); who calls initialize()?
 *      -   open_port() calls connect()
 *      -   open_virtual_port() calls connect()
 *      -   get_port_count() calls connect()
 *      -   get_port_name() calls connect()
 */

#include "rtl/midi/jack/midi_jack.hpp"  /* rtl::midi_jack class             */

#if defined RTL66_BUILD_JACK

#include <stdint.h>                     /* std::uint64_t                    */
#include <cstring>                      /* std::strlen                      */
#include <jack/midiport.h>              /* ::jack_midi_get_event_count()    */
#include <pthread.h>                    /* the pthreads API                 */

#include "rtl66-config.h"               /* RTL66_HAVE_JACK_PORT_RENAME      */
#include "midi/event.hpp"               /* midi::event class                */
#include "midi/eventcodes.hpp"          /* midi::status enum, functions...  */
#include "midi/portnaming.hpp"          /* midi::extract_port_names()       */
#include "rtl/midi/jack/midi_jack_callbacks.hpp" /* rtl::jack callbacks     */
#include "rtl/midi/rtmidi_in_data.hpp"  /* rtl::rtmidi_in_data class        */
#include "util/msgfunctions.hpp"        /* util::error_message(), etc.      */
#include "xpc/ring_buffer.hpp"          /* xpc::ring_buffer<> template      */
#include "xpc/timing.hpp"               /* xpc::std_sleep_us()              */

namespace rtl
{

/*------------------------------------------------------------------------
 * JACK implementation flags and values.
 *------------------------------------------------------------------------*/

/**
 *  Delimits the size of the JACK ringbuffer. Related to issue #100, when
 *  we play the Sequencer64 MIDI tune "b4uacuse-stress.midi", we can fail to
 *  write to the ringbuffer.  So we've doubled the size. Without timestamps,
 *  that allows about 10K events. With time-stamps, about 4.7K events.
 *
 *  For the new midi::message ringbuffer, running the stress file from the
 *  Sequencer64 project, the maximum number of events in the ring buffer
 *  is about 200 at 192 PPQN.  At 960 PPQN, thousands of events are dropped
 *  and the buffer maxes out (1024 events).  Let's try 4096 instead.
 *  Weird, now that tune yields the max of 196! Let's try 2048. Might be a
 *  useful configuration option.
 */

static const size_t c_jack_ringbuffer_size = 2048;  /* tentative */

/*------------------------------------------------------------------------
 * Additional free functions
 *------------------------------------------------------------------------*/

/**
 *  JACK detection function.  Just opens a client without activating it, then
 *  closes it.
 *
 *  However, if only jackdbus is running, this function will detect JACK,
 *  but later attempts to activate JACK and open ports will fail.
 *
 * falkTX wrote:
 *
 *      Both jackd and jackdbus implement a JACK server.
 *
 *      -   Jackd is started via command-line and having it running means the
 *          jack server is also running.
 *      -   Jackdbus is a user service that can stop/start the JACK server
 *          dynamically.  jackd != JACK server.
 *
 *      Jackdbus can be always running because it doesn't imply the JACK
 *      server is also running.  It's only there to listen for requests on the
 *      dbus service.  If jackdbus is running the JACK server can be started
 *      with:
 *
 *          $ jack_control start
 *
 *  If jackdbus is running both jack_client_open() and jack_activate() work.
 *  We need a better test.
 *
 *  To do:  Use this function to log the JACK version information:
 *
 *          const char * version = ::jack_get_version_string();
 *
 *  Needs testing under various conditions.
 */

bool
detect_jack (bool forcecheck)
{
    bool result = false;
    static bool s_already_checked = false;
    static bool s_jack_was_detected = false;
    if (forcecheck)
    {
        s_already_checked = false;
        s_jack_was_detected = false;
    }
    if (s_already_checked)
    {
        return s_jack_was_detected;
    }
    else
    {
        const char * cname = "rtl_jack_detector";
        jack_status_t status;
        jack_status_t * ps = &status;
        jack_options_t jopts = JackNoStartServer;
        jack_client_t * jackman = ::jack_client_open(cname, jopts, ps);
        if (not_nullptr(jackman))
        {
            int rc = ::jack_activate(jackman);
            if (rc == 0)
            {
                const char ** ports = ::jack_get_ports
                (
                    jackman, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
                );
                result = not_nullptr(ports);
                if (result)
                {
                    int count = 0;
                    while (not_nullptr(ports[count]))
                        ++count;

                    result = count > 0;
                }
            }
            ::jack_deactivate(jackman);
            (void) ::jack_client_close(jackman);
        }
        if (result)
        {
            s_jack_was_detected = true;
        }
        else
        {
            warnprint("JACK not detected");
        }
        s_already_checked = true;
    }
    return result;
}

#if defined RTL66_USE_GLOBAL_CLIENTINFO

void
set_jack_version ()
{
#if defined RTL66_HAVE_JACK_GET_VERSION_STRING
    const char * sjv = ::jack_get_version_string();
    if (not_nullptr(sjv) && std::strlen(sjv) > 0)
    {
        std::string jv{sjv};
        midi::global_client_info().api_version(std::string(jv));
    }
#else
    std::string jv{"JACK < v. 1"};
    midi::global_client_info().api_version(jv);
#endif
}

#else

void
set_jack_version ()
{
    // no code
}

#endif

/**
 *  This function merely eats the string passed as a parameter. This
 *  helps prevent annoying JACK console output. Taken from Seq66's
 *  "rtmidi" library code.
 */

static void
jack_message_bit_bucket (const char *)
{
    // Into the bit-bucket with ye ya scalliwag!
}

/**
 *  This function silences JACK error output to the console.  Probably not
 *  good to silence this output, but let's provide the option, for the sake of
 *  symmetry, consistency, non-annoyance, what have you.
 *
 *  The JACK library provides two built-in callbacks for this purpose:
 *  default_jack_error_callback() and silent_jack_error_callback(). However,
 *  these are not declared in the JACK header files.
 */

void
silence_jack_errors (bool silent)
{
    if (silent)
        ::jack_set_error_function(jack_message_bit_bucket);
}

/**
 *  This function silences JACK info output to the console.  We were getting
 *  way too many informational messages, to the point of obscuring the debug
 *  and error output.
 */

void
silence_jack_info (bool silent)
{
    if (silent)
        ::jack_set_info_function(jack_message_bit_bucket);
}

/**
 *  Silences all JACK console output.
 */

void
silence_jack_messages (bool silent)
{
    silence_jack_errors(silent);
    silence_jack_info(silent);
}

/*------------------------------------------------------------------------
 * JACK callbacks
 *------------------------------------------------------------------------
 *
 *  The JACK callbacks have been moved to midi_jack_callbacks.cpp for
 *  maintainability.
 *
 *------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 * midi_jack
 *------------------------------------------------------------------------*/

midi_jack::midi_jack () :
    midi_api        (),
    m_client_name   (),
    m_jack_data     ()
{
    (void) initialize(client_name());
}

midi_jack::midi_jack
(
    midi::port::io iotype,
    const std::string & clientname,
    unsigned queuesize
) :
    midi_api        (iotype, queuesize),
    m_client_name   (clientname),
    m_jack_data     ()
{
    (void) initialize(client_name());
}

midi_jack::~midi_jack ()
{
    delete_port();
}

/*------------------------------------------------------------------------
 * midi_jack engine-related functions
 *------------------------------------------------------------------------*/

/**
 *  Creates a connection to the JACK engine/client. It creates it only
 *  if the connection does not already exist.  In Seq66v2 we will consolidate
 *  JACK transport, JACK MIDI I/O, and, Linus willing, JACK Audio I/O.
 *
 *  This function also sets up all of the JACK callbacks.
 *
 *  One hidden "parameter" is the client name. Another is the port::io type.
 *  If service, that will change the process callback that is set, for use by
 *  a master bus.
 *
 *  Note that JACK is NOT ACTIVATED by this function.
 *
 * \return
 *      Returns the engine handle as a void pointer if the client handle
 *      was created or already existed.
 */

void *
midi_jack::engine_connect ()
{
    void * result = nullptr;
    midi_jack_data & data = jack_data();
    bool ok = is_nullptr(data.jack_client()) || ! is_engine();
    if (ok)
    {
        const char * cname = client_name().c_str();

        jack_options_t jopts = JackNoStartServer;
        if (rtmidi::start_jack())
            jopts = JackNullOption;

#if defined USE_JACK_STATUS_RETURN
        jack_status_t status;
        jack_status_t * ps = &status;
        jack_client_t * c = ::jack_client_open(cname, jopts, ps);

        // Here, can shows the bits of the status, if desired.
#else
        jack_client_t * c = ::jack_client_open(cname, jopts, NULL /*ps*/);
#endif
        if (not_nullptr(c))
        {
            void * apidata = reinterpret_cast<void *>(&data);
            data.jack_client(c);
            api_data(&data);
            if (have_master_bus())
                master_bus()->client_handle(c);

            JackProcessCallback cb = jack_process_io;
            if (is_output())
                cb = jack_process_out;
            else if (is_input())
                cb = jack_process_in;

            bool ok = jack_set_process_cb(c, cb, apidata);
            if (ok)
            {
#if defined RTL66_JACK_PORT_SHUTDOWN                    // TODO
#if defined RTL66_JACK_SESSION                          // TODO
                std::string uuid = rc().jack_session(); // e.g. 8589934670
                if (uuid.empty())
                    uuid = get_jack_client_uuid(result);

                if (! uuid.empty())
                    rc().jack_session(uuid);
#endif
                JackShutdownCallback cb = jack_shutdown_callback;
                (void) jack_set_shutdown_cb(c, cb, (void *) this);
#endif
#if defined RTL66_JACK_PORT_CONNECT_CALLBACK
                JackPortConnectCallback cb = jack_port_connect_callback;
                (void) jack_set_port_connect_cb (c, cb, (void *) this);
#endif
#if defined RTL66_JACK_PORT_REFRESH_CALLBACK
                JackPortRegistrationCallback cb = jack_port_register_callback;
                (void) jack_set_port_registration_cb(c, cb, (void *) this);
#endif
#if defined RTL66_JACK_METADATA
                std::string n = "seq_icon_name()";
                bool ok = jack_set_meta_data(c, n);
                (
                    c, JACK_METADATA_ICON_NAME, n
                );
#endif
            }
        }
    }
    else
    {
        result = data.jack_client();
        debug_print("JACK", "Reusing engine/client connection");
    }
    return result;
}

void
midi_jack::engine_disconnect ()
{
    midi_jack_data & data = jack_data();
    jack_client_t * c = data.jack_client();
    if (not_nullptr(c))
    {
        int rc = ::jack_client_close(c);
        data.jack_client(nullptr);
        if (rc != 0)
        {
            error_print("jack_client_close", "failed");
        }
    }
}

/**
 *  Activates a connection to the JACK engine/client.
 */

bool
midi_jack::engine_activate ()
{
    bool result = true;
    midi_jack_data & data = jack_data();
    if (not_nullptr(data.jack_client()))
    {
        int rc = ::jack_activate(data.jack_client());
        result = rc == 0;
        if (! result)
            error_print("jack_activate", "failed");
    }
    return result;
}

/**
 *  Somewhat the opposite of connect().  Similar to delete_port(), but also
 *  takes the application out of the JACK client graph.
 *
 *  This function should be used only at the end of the application, and
 *  perhaps by a "master bus" class.
 */

bool
midi_jack::engine_deactivate ()
{
    bool result = true;
    midi_jack_data & data = jack_data();
    if (not_nullptr(data.jack_client()))
    {
        int rc = ::jack_deactivate(data.jack_client());
        result = rc == 0;
    }
    return result;
}

/*------------------------------------------------------------------------
 * midi_jack port-related functions
 *------------------------------------------------------------------------*/

/**
 *  Count the input or output MIDI ports. This original rtmidi-based
 *  implementation preserves the behavior of connecting to the engine/client
 *  every time it is called.
 */

int
midi_jack::get_port_count ()
{
    int result = 0;
    bool ok = connect();     /* WHY CALL THIS AGAIN?    */
    if (ok)
    {
        midi_jack_data & data = jack_data();
        if (not_nullptr(data.jack_client()))
        {
            unsigned long flag = is_output() ?
                JackPortIsInput : JackPortIsOutput ;

            const char ** ports = ::jack_get_ports
            (
                data.jack_client(), NULL, JACK_DEFAULT_MIDI_TYPE, flag
            );
            if (is_nullptr(ports))
            {
                debug_print("get_port_count", "no ports");
            }
            else
            {
                int count = 0;
                while (not_nullptr(ports[count]))
                    ++count;

                result = count;
                ::jack_free(ports);
            }
        }
    }
    return result;
}

/**
 *  Creates the JACK output ring-buffer.
 */

bool
midi_jack::create_ringbuffer (size_t rbsize)
{
    bool result = rbsize > 0;
    if (result)
    {
        xpc::ring_buffer<midi::message> * rb =
            new (std::nothrow) xpc::ring_buffer<midi::message>(rbsize);

        result = not_nullptr(rb);
        if (result)
        {
            jack_data().jack_buffer(rb);
        }
        else
        {
            util::error_message("ring_buffer creation error");
            // error(rterror::kind::warning, m_error_string);
        }
    }
    return result;
}

/**
 *  This function opens a JACK-client connection.  Here are the things it
 *  does, in this order:
 *
 *  -   If there is already a client/engine for this port, do nothing.
 *  -   If output, the JACK output ringbuffers for this port are created.
 *  -   The desired client name is retrieved and set.
 *  -   The starting options are set. If we want to auto-start the JACK
 *      server, then the option is set to JackNullOption; otherwise it is
 *      set to JackNoStartServer.
 *  -   A call to jack_client_open() gives us the jack_client_t handle to
 *      save in the midi_jack_data structure held by this class instance.
 *  -   The various callbacks are set:
 *      -   jack_process_io(), jack_process_in(), or jack_process_out().
 *      -   jack_shutdown_callback() [optional]
 *      -   jack_port_connect_callback() [optional]
 *      -   jack_port_register_callback() [optional]
 *      -   JACK metadata properties [optional]
 *  -   Lastly, JACK is activated.
 *
 *  Most of the functionality above is wrapped in engine_connect() and
 *  engine_activate() for reusability.
 *
 *  Be careful about mixing our "masterbus" paradigm with the normal
 *  RtMidi paradigm.
 *
 *  Then the JACK client is initialized.  Finally, the JACK process
 *  callback for input or output is set.
 *
 *  Hmmm, compare to seq66::midi_jack::create_ringbuffer() !!!
 */

bool
midi_jack::connect ()
{
    bool result = false;
    midi_jack_data & data = jack_data();
    if (not_nullptr(data.jack_client()))
        return true;

    jack_client_t * c = client_handle(engine_connect());
    if (not_nullptr(c))
    {
        data.jack_client(c);
        api_data(&data);
        if (is_output())
            result = create_ringbuffer(c_jack_ringbuffer_size);

        if (result)
            result = engine_activate();
    }
    return result;
}

/**
 *  Note the big difference between this JACK MIDI implementation and
 *  that of the implementation in Seq66:  Here there are separate process
 *  callbacks for input and output (for each port!), while in Seq66 there
 *  is one callback that calls either the input or output callback in a loop
 *  querying each port.
 *
 *  What implications does this have for latency, thread starvation, and
 *  total processor usage? Does JACK do thread-pooling?
 */

bool
midi_jack::initialize (const std::string & clientname)
{
    bool result;
    midi_jack_data & data = jack_data();
    data.jack_port(nullptr);
    data.jack_client(nullptr);
    client_name(clientname);                        /* necessary for API    */
#if RTL66_HAVE_SEMAPHORE_H
    if (is_output())
    {
        result = data.semaphore_init();
    }
#endif
    if (! reuse_connection())
    {
        midi_jack_data & data = jack_data();
        if (have_master_bus())
            master_bus()->client_handle(data.jack_client());

        api_data(&data);
        result = connect();
        if (is_input())
            data.rt_midi_in(&input_data());
    }
    if (! result)
    {
        error
        (
            rterror::kind::driver_error,
            "midi_jack::initialize: error opening client"
        );
    }
    return result;
}

/**
 *  This function implements registering a port with the JACK client
 *  [data.jack_client()], getting the port name, then calling ::jack_connect().
 */

bool
midi_jack::open_port (int portnumber, const std::string & portname)
{
    if (is_connected())
    {
        error
        (
            rterror::kind::warning,
            "midi_jack::open_port: connection already exists"
        );
        return true;
    }

    bool result = portnumber >= 0;                  /* -1 == uninit'ed      */
    if (result)
        result = connect();    /* WHY CALL THIS AGAIN? */

    if (result)
    {
        midi_jack_data & data = jack_data();
        if (is_nullptr(data.jack_port()))           /* can create the port  */
        {
            const char * pn = portname.c_str();
            jack_port_t * jptr = ::jack_port_register
            (
                data.jack_client(), pn, JACK_DEFAULT_MIDI_TYPE,
                is_output() ? JackPortIsOutput : JackPortIsInput, 0
            );
            if (is_nullptr(jptr))
            {
                result = false;
                error_print("jack_port_register", "failed");
            }
            else
            {
                std::string name = get_port_name(portnumber);
                data.jack_port(jptr);
                int rc = ::jack_connect             /* source/destin names  */
                (
                    data.jack_client(),
                    jack_port_name(data.jack_port()),
                    name.c_str()
                );
                if (rc != 0)                        /* not connected!       */
                {
                    result = false;
                    error_print("jack_connect", "failed");
                }
            }
        }
        is_connected(result);
    }
    if (! result)
    {
        std::string msg = "midi_jack::open_port: error";
        if (portname.size() >= size_t(::jack_port_name_size()))
            msg += " (port name too long?)";

        error(rterror::kind::driver_error, msg);
    }
    return result;
}

bool
midi_jack::open_virtual_port (const std::string & portname)
{
    bool result = connect();    /* WHY CALL THIS AGAIN? */
    if (result)
    {
        midi_jack_data & data = jack_data();
        if (is_nullptr(data.jack_port()))
        {
            jack_port_t * jptr = ::jack_port_register
            (
                data.jack_client(), portname.c_str(),
                JACK_DEFAULT_MIDI_TYPE,
                is_output() ? JackPortIsOutput : JackPortIsInput, 0
            );
            if (is_nullptr(jptr))
            {
                result = false;
                error_print("jack_port_register", "virtual port, failed");
            }
            else
                data.jack_port(jptr);
        }
    }
    if (! result)
    {
        std::string msg = "midi_jack::open_virtual_port: error creating port";
        if (portname.size() >= size_t(::jack_port_name_size()))
            msg += " (port name too long?)";

        error(rterror::kind::driver_error, msg);
    }
    return result;
}

/**
 *  This function is called by the midi_jack destructor.
 *
 *  Note that there is no need to delete the midi_jack_data object, it is
 *  not allocated on the heap.
 *
 *  Also note that, currently, this is used only in the the destructor, so no
 *  need for a return value.
 */

void
midi_jack::delete_port ()
{
    midi_jack_data & data = jack_data();
    (void) close_port();
    if (is_output())
    {
        if (not_nullptr(jack_data().jack_buffer()))
        {
            xpc::ring_buffer<midi::message> * rb = jack_data().jack_buffer();
            if (rb->dropped() > 0 || rb->count_max() > (rb->buffer_size() / 2))
            {
                char tmp[64];
                snprintf
                (
                    tmp, sizeof tmp, "%d events dropped, %d max/%d",
                    rb->dropped(), rb->count_max(), rb->buffer_size()
                );
                (void) util::warn_message("ring-buffer", tmp);
            }
            delete jack_data().jack_buffer();
        }
    }
    engine_disconnect();

#if RTL66_HAVE_SEMAPHORE_H
    if (is_output())
    {
        data.semaphore_destroy();
    }
#endif
}

/**
 *  In the RtMidi implementations of MidiInJack/MidiOutJack::getPortName():
 *
 *      -   The flags JackPortIsOutput and JackPortIsInput are reversed. Why?
 *      -   The input version uses the sneaky for-loop mentioned below, while
 *          the output version doesn't.  This seems like an oversight.
 *
 *  The jack_free() function isto be used on memory returned by
 *  jack_port_get_connections(), jack_port_get_all_connections() and
 *  jack_get_ports().  This is MANDATORY on Windows!!! Otherwise all nasty
 *  runtime version related crashes can occur. Developers are strongly
 *  encouraged to use this function instead of the standard "free" function
 *  in new code.
 */

std::string
midi_jack::get_port_name (int portnumber)
{
    std::string result;
    midi_jack_data & data = jack_data();
    if (not_nullptr(data.jack_client()) && portnumber >= 0)
    {
        bool ok = connect();
        if (ok)
        {
            unsigned long flag = is_output() ?
                JackPortIsInput : JackPortIsOutput ;

            const char ** ports = ::jack_get_ports
            (
                data.jack_client(), NULL, JACK_DEFAULT_MIDI_TYPE, flag
            );
            if (is_nullptr(ports))
            {
                error_print("jack_get_ports", "found no ports");
            }
            else
            {
                if (portnumber >= 0)
                {
                    /*
                     *  We have to sneak up on the terminating null pointer,
                     *  else risk a segfault.
                     */

                    int p;
                    for (p = 0; p <= portnumber; ++p)
                    {
                        if (is_nullptr(ports[p]))
                        {
                            p = (-1);
                            break;
                        }
                    }
                    if (p != (-1))
                        result.assign(ports[portnumber]);

                }
                ::jack_free(ports);             /* see note in banner   */
            }
        }
    }
    if (result.empty())
        error("midi_jack::get_port_name", portnumber);

    return result;
}

bool
midi_jack::close_port ()
{
    bool result = false;
    midi_jack_data & data = jack_data();
    if (not_nullptr_2(data.jack_client(), data.jack_port()))
    {
#if RTL66_HAVE_SEMAPHORE_H
        if (is_output())
        {
            (void) data.semaphore_post_and_wait();
        }
#endif
        int rc = ::jack_port_unregister(data.jack_client(), data.jack_port());
        if (rc == 0)
            result = true;
        else
            error_print("jack_port_unregister", "failed");

        data.jack_port(nullptr);
    }
    return result;
}

/**
 *  The function jack_port_set_name() is deprecated. The replacement is:
 *
 * int jack_port_rename(jack_client_t *, jack_port_t *, const char * portname)
 *
 *      Modify a port's short name. May NOT be called from a callback handling
 *      a server event.  If the resulting full name (including the
 *      "client_name:" prefix) is longer than jack_port_name_size(), it's
 *      truncated.  This differs from jack_port_set_name() by triggering
 *      PortRename notifications to clients that have registered a port rename
 *      handler.  Returns 0 on success, otherwise a non-zero error code.
 *
 *  If not available, this function returns false.
 */

bool
midi_jack::set_port_name (const std::string & portname)
{
    bool result = false;
    midi_jack_data & data = jack_data();
    if (not_nullptr_2(data.jack_client(), data.jack_port()))
    {
#if defined RTL66_HAVE_JACK_PORT_RENAME             /* rtl66-config.h (.in) */
        int rc = ::jack_port_rename
        (
            data.jack_client(), data.jack_port(), portname.c_str()
        );
        if (rc == 0)
            result = true;
#else
#if defined RTL66_ALLOW_DEPRECATED_JACK_FUNCTIONS   /* rtl_build_macros.h   */
        int rc = ::jack_port_set_name(data.jack_port(), portname.c_str());
        if (rc == 0)
            result = true;
#else
#error Deprecated function jack_port_set_name() not allowed
#endif
#endif
    }
    return result;
}

/*------------------------------------------------------------------------
 * midi_jack input
 *------------------------------------------------------------------------*/

/*
 *  This function will not work for JACK, as the client name is set
 *  in the connect() function.
 */

bool
midi_jack::set_client_name (const std::string & /*clientname*/)
{
    error
    (
        rterror::kind::warning,
        "midi_jack::set_client_name: not in JACK"
    );
    return true;
}

/*------------------------------------------------------------------------
 * Extensions
 *------------------------------------------------------------------------*/

/**
 *  This function is not present in the RtMidi library.
 *
 *  Gets information on ALL ports, putting input data into one
 *  midi::clientinfo container, and putting output data into another
 *  midi::clientinfo container.
 *
 * \tricky
 *      When we want to connect to a system input port, we want to use an
 *      output port to do that.  When we want to connect to a system output
 *      port, we want to use an input port to do that.  Therefore, we search
 *      for the <i> opposite </i> kind of port.
 *
 *  If there is no system input port, or no system output port, then we add a
 *  virtual port of that type so that the application has something to work
 *  with.
 *
 *  Note that, at some point, we ought to consider how to deal with
 *  transitory system JACK clients and ports, and adjust for it.  A kind of
 *  miniature form of session management.  Also, don't forget about the
 *  usefulness of jack_get_port_by_id() and jack_get_port_by_name().
 *
 * bool iswriteable:
 *
 *      The kind of ports to find.  For example, for an Rtl66 list of output
 *      ports, we want to find all ports with "write" capabilities, such as
 *      FluidSynth.  If not writeable, then find ports with "read"
 *      capabilities. This function is not used if a service port; we
 *      enumerate input and output ports separately.
 *
 * Error handling:
 *
 *  Not having any JACK input ports present isn't necessarily an error.  There
 *  may not be any, and there may still be at least one output port.  Also, if
 *  there are none, we try to make a virtual port so that the application has
 *  something to work with.  The only issue is the client number.  Currently
 *  all virtual ports we create have a client number of 0.
 *
 * JackPortFlags enum:
 *
 *      JackPortIsInput:    If set, then the port can receive data.
 *      JackPortIsOutput:   If set, then data can be read from the port.
 *      JackPortIsPhysical: If this flag is added, only ports corresponding
 *                          to a physical device get detected/connected.
 *
 * \param [out] ioports
 *      The list of ports to populate. It will either be a list of input ports
 *      (readable ports) or a list of ouput ports (writeable ports).
 *
 * \param preclear
 *      If true (the default), then clear the ports parameter first. This is
 *      done event if the MIDI engine client is not valid.
 *
 * \return
 *      Returns the total number of ports found.  Note that 0 ports is not
 *      necessarily an error; there may be no JACK apps running with exposed
 *      ports.  If there is no JACK client, then -1 is returned.  Also see
 *      get_port_count();
 */

int
midi_jack::get_io_port_info (midi::ports & ioports, bool preclear)
{
    int result = 0;
    bool iswriteable = is_output();
    midi_jack_data & data = jack_data();
    if (preclear)
        ioports.clear();

    if (not_nullptr(data.jack_client()))
    {
#if defined PLATFORM_DEBUG
        infoprint(iswriteable ? "Writable ports:" : "Readable ports:");
#endif
        unsigned long flag = iswriteable ?
            JackPortIsInput : JackPortIsOutput ;

        const char ** ports = ::jack_get_ports
        (
            data.jack_client(), NULL, JACK_DEFAULT_MIDI_TYPE, flag
        );
        if (is_nullptr(ports))
        {
#if defined RTL66_JACK_FALLBACK_TO_VIRTUAL_PORT
            int clientnumber = 0;
            int portnumber = 0;
            std::string clientname = "";            // TODO: seq_client_name();
            std::string portname = "midi in 0";
            ioports.add
            (
                clientnumber, clientname, portnumber, portname,
                midi::port::io::input, midi::port::kind::manual
            );
            ++result;
#endif
            error_print("jack_get_ports", "found no ports");
        }
        else
        {
            int clientnumber = 0;                   /* JACK: doesn't apply  */
            int count = 0;
            while (not_nullptr(ports[count]))
            {
                std::string fullname = ports[count];
                std::string clientname;
                std::string portname;
                std::string alias = get_port_alias(fullname);
                if (alias == fullname)
                    alias.clear();

                /*
                 * TODO:  somehow get the 32-bit ID of the port and add it as
                 * a system port ID to use for lookup when detecting newly
                 * registered or unregistered ports.
                 */

                midi::extract_port_names(fullname, clientname, portname);

                /*
                 * Retrofit this commenting out!
                 *
                 *  if (client == -1 || clientname != client_name_list.back())
                 *  {
                 *      client_name_list.push_back(clientname);
                 *      ++client;
                 *  }
                 */

                ioports.add
                (
                    clientnumber, clientname, count, portname,
                    midi::port::io::input, midi::port::kind::normal,
                    0, alias
                );
                ++count;
            }
            ::jack_free(ports);
            result += count;
        }
    }
    return result;
}

/**
 *  If the name includes "system:", try getting an alias instead via
 *  jack_port_by_name() and jack_port_get_aliases().
 *
 *  The cast of the result of malloc() is needed in C++, but not C.
 *
 *  The jack_port_t pointer type is actually an opaque value equivalent to an
 *  integer greater than 1, which is a port index.
 *
 *  Examples of aliases retrieved:
 *
\verbatim
      Out-port: "system:midi_capture_2":

        alsa_pcm:Launchpad-Mini/midi_capture_1
        Launchpad-Mini:midi/capture_1
\endverbatim
 *
 *  and
 *
\verbatim
      In-port "system:midi_playback_2":

        alsa_pcm:Launchpad-Mini/midi_playback_1
        Launchpad-Mini:midi/playback_1
\endverbatim
 *
 *  Ports created by "a2jmidid --export-hw" do not have JACK aliases.  Ports
 *  created by Seq66 do not have JACK aliases.  Ports created by qsynth do not
 *  have JACK aliases.
 *
 * \param name
 *      Provides the name of the port as retrieved by jack_get_ports(). This
 *      name is used to look up the JACK port handle.
 *
 * \return
 *      Returns the alias, if it exists, for a system port.  That is, one with
 *      "system:" in its name.  Brittle.  And some JACK systems do not provide
 *      an alias.
 */

std::string
midi_jack::get_port_alias (const std::string & name)
{
    std::string result;
    midi_jack_data & data = jack_data();
    if (not_nullptr(data.jack_client()))
    {
        bool is_system_port = midi::contains(name, "system:");  /* brittle code */
        if (is_system_port)
        {
            jack_port_t * p = ::jack_port_by_name(data.jack_client(), name.c_str());
            if (not_NULL(p))
            {
                char * aliases[2];
                const int sz = ::jack_port_name_size();
                aliases[0] = reinterpret_cast<char *>(malloc(sz));  /* alsa_pcm */
                aliases[1] = reinterpret_cast<char *>(malloc(sz));  /* dev name */
                if (is_nullptr_2(aliases[0], aliases[1]))
                    return result;                                  /* bug out  */

                aliases[0][0] = aliases[1][0] = 0;                  /* 0 length */
                int rc = ::jack_port_get_aliases(p, aliases);
                if (rc > 1)
                {
                    std::string nick = std::string(aliases[1]);     /* brittle  */
                    auto colonpos = nick.find_first_of(":");        /* brittle  */
                    if (colonpos != std::string::npos)
                        result = nick.substr(0, colonpos);

                    /*
                     * Another bit of brittleness:  the name generated via the
                     * a2jmidid program uses spaces, but the system alias returned
                     * by JACK uses a hyphen.  Convert them to spaces.
                     */

                    auto hyphenpos = result.find_first_of("-");     /* brittle  */
                    while (hyphenpos != std::string::npos)
                    {
                        result[hyphenpos] = ' ';
                        hyphenpos = result.find_first_of("-", hyphenpos);
                    }
                }
                else
                {
                    /*
                     * To do:
                     *
                    if (rc < 0)
                        errprint("JACK port aliases error");
                    else
                        infoprint("JACK aliases unavailable");
                     */
                }
                free(aliases[0]);
                free(aliases[1]);
            }
        }
    }
    return result;
}

/**
 *  Checks to see if a master client connection is available.  If so, it is
 *  logged as the client for the current port.
 */

bool
midi_jack::reuse_connection ()
{
    bool result = master_is_connected();
    if (result)
    {
        jack_client_t * jc = client_handle();
        result = not_nullptr(jc);
        if (result)
        {
            midi_jack_data & data = jack_data();
            data.jack_client(jc);
            api_data(&data);
        }
    }
    return result;
}

#if defined RTL66_MIDI_EXTENSIONS

/**
 *  Empty body for setting PPQN.
 */

bool
midi_jack::PPQN (midi::ppqn /*ppq*/)
{
    return false;                   // TODO
}

/**
 *  Empty body for setting BPM.
 */

bool
midi_jack::BPM (midi::bpm /*bp*/)
{
    return false;                   // TODO
}

/**
 *  An internal helper function for sending MIDI clock bytes.  This function
 *  ought to be called "send_realtime_message()".
 *
 *  Based on the GitHub project "jack_midi_clock", we could try to bypass the
 *  ringbuffer used here and convert the ticks to jack_nframes_t, and use the
 *  following code:
 *
 *      uint8_t * buffer = jack_midi_event_reserve(port_buf, time, 1);
 *      if (buffer)
 *          buffer[0] = rt_msg;
 *
 *  We generally need to send the (realtime) MIDI clock messages Start, Stop,
 *  and Continue if the JACK transport state changed.
 *
 * \param evbyte
 *      The status byte to send.
 */

bool
midi_jack::send_byte (midi::byte evbyte)
{
    midi::message message;
    message.push(evbyte);
    bool result = send_message(message);
    if (! result)
    {
        errprint("JACK send_byte() failed");
    }
    return result;
}

/**
 *  Starts this JACK client and sends MIDI start.   Note that the
 *  jack_transport code (which implements JACK transport) checks if JACK is
 *  running, but a check of the JACK client handle here should be enough.
 */

bool
midi_jack::clock_start ()
{
    midi_jack_data * jkdata = reinterpret_cast<midi_jack_data *>(api_data());
    ::jack_transport_start(jkdata->jack_client());
    return send_status(midi::status::clk_start);
}

/**
 *  Sends a MIDI clock event.
 *
 * \param tick
 *      The value of the tick to use.
 */

bool
midi_jack::clock_send (midi::pulse tick)
{
    if (tick >= 0)
    {
#if defined PLATFORM_DEBUG_TMI
        midibase::show_clock("JACK", tick);
#endif
        return send_status(midi::status::clk_clock);
    }
    else
        return false;
}

/**
 *  Stops this JACK client and sends MIDI stop.   Note that the jack_transport
 *  code (which implements JACK transport) checks if JACK is running, but a
 *  check of the JACK client handle here should be enough.
 */

bool
midi_jack::clock_stop ()
{
    // Could use jack_data() as well.  Double-check.

    midi_jack_data * jkdata = reinterpret_cast<midi_jack_data *>(api_data());
    ::jack_transport_stop(jkdata->jack_client());
    return send_status(midi::status::clk_stop);
}

/**
 *  jack_transport_locate(), jack_transport_reposition(), or something else?
 *  What is used by jack_transport?
 *
 * \param tick
 *      Provides the tick value to continue from.
 *
 * \param beats
 *      The parameter "beats" is currently unused in the JACK implementation.
 */

bool
midi_jack::clock_continue (midi::pulse tick, midi::pulse /*beats*/)
{
    midi_jack_data * jkdata = reinterpret_cast<midi_jack_data *>(api_data());
    int beat_width = 4;                                 // no m_beat_width !!!
    int ticks_per_beat = midi_api::PPQN() * 10;
    midi::bpm beats_per_minute = midi_api::BPM();
    uint64_t tick_rate =
    (
        uint64_t(::jack_get_sample_rate(jkdata->jack_client())) * tick * 60.0
    );
    long tpb_bpm = long(ticks_per_beat * beats_per_minute * 4.0 / beat_width);
    uint64_t jack_frame = tick_rate / tpb_bpm;
    if (::jack_transport_locate(jkdata->jack_client(), jack_frame) == 0)
    {
        /*
         * New code to work like the ALSA version, needs testing.  Related to
         * issue #67.  However, the ALSA version sends Continue, flushes, and
         * then sends Song Position, so we will match that here.  No flush
         * needed in JACK.
         */

        send_status(midi::status::song_pos);
        send_status(midi::status::clk_continue);
        return true;
    }
    else
    {
        infoprint("JACK Continue failed");
        return false;
    }
}

/**
 *  Checks the rtmidi_in_data queue for the number of items in the queue.
 *
 * \return
 *      Returns the value of rtindata->queue().count(), unless the caller is
 *      using an rtmidi callback function, in which case 0 is always returned.
 */

int
midi_jack::poll_for_midi ()         // input
{
    rtmidi_in_data * rtindata = jack_data().rt_midi_in();
    (void) xpc::microsleep(xpc::std_sleep_us());            /* 10 us IIRC   */
    return rtindata->queue().count();
}

/**
 *  Gets a MIDI event.  This implementation gets a midi::message off the front
 *  of the queue and converts it to a Seq66 event.
 *
 * \change ca 2017-11-04
 *      Issue #4 "Bug with Yamaha PSR in JACK native mode" in the
 *      seq66-packages project has been fixed.  For now, we ignore
 *      system messages.  Yamaha keyboards like my PSS-790 constantly emit
 *      active sensing messages (0xfe) which are not logged, and the previous
 *      event (typically pitch wheel 0xe0 0x0 0x40) is continually emitted.
 *      One result (we think) is odd artifacts in the seqroll when recording
 *      and passing through.
 *
 * \param inev
 *      Provides the destination for the MIDI event.
 *
 * \return
 *      Returns true if a MIDI event was obtained, indicating that the return
 *      parameter can be used.  Note that we force a value of false for all
 *      system messages at this time; they cannot yet be handled gracefully in
 *      the native JACK implementation.
 */

bool
midi_jack::get_midi_event (midi::event * inev)           // input
{
    rtmidi_in_data * rtindata = jack_data().rt_midi_in();
    bool result = ! rtindata->queue().empty();
    if (result)
    {
        /*
         * Cannot speed this up.
         *
         *      result = inev->set_midi_event(rtindata->queue().pop_front());
         */

        midi::message mm = rtindata->queue().pop_front();
        result = inev->set_midi_event(mm);
        if (result)
        {
            /*
             * For now, ignore certain messages; they're not handled by the
             * perform object.  Could be handled there, but saves some
             * processing time if done here.  Also could move the output code
             * to perform so it is available for frameworks beside JACK.
             */

            midi::byte st = mm[0];
#if defined PLATFORM_DEBUG
            if (midi::is_realtime_msg(st))
            {
                midi::status eventstat = midi::to_status(st);
                static int s_count = 0;
                char c;
                switch (eventstat)
                {
                case midi::status::clk_clock:      c = 'C';    break;
                case midi::status::active_sense:   c = 'S';    break;
                case midi::status::reset:          c = 'R';    break;
                case midi::status::clk_start:      c = '>';    break;
                case midi::status::clk_continue:   c = '|';    break;
                case midi::status::clk_stop:       c = '<';    break;
                case midi::status::sysex:          c = 'X';    break;
                default:                           c = '.';    break;
                }
                (void) putchar(c);
                if (++s_count == 80)
                {
                    s_count = 0;
                    (void) putchar('\n');
                }
                fflush(stdout);
            }
#endif
            if (midi::is_sense_or_reset_msg(st))
                result = false;

            /*
             * Issue #55 (ignoring the channel).  The status is already set,
             * with channel nybble, above, no need to set it here.
             */
        }
    }
    return result;
}

/**
 *  We could push the bytes of the event into a midi::byte vector, as done in
 *  send_message().  The ALSA code (seq_alsamidi/src/midibus.cpp) sticks the
 *  event bytes in an array, which might be a little faster than using
 *  push_back(), but let's try the vector first.  The rtmidi code here is from
 *  midi_out_jack::send_message().
 */

bool
midi_jack::send_event (const midi::event * ev, midi::byte channel)
{
    midi::byte evstatus = ev->get_status(channel);
    midi::byte d0, d1;
    ev->get_data(d0, d1);

    midi::message message;
    message.push(evstatus);
    message.push(d0);
    if (ev->is_two_bytes())
        message.push(d1);

    if (send_message(message))
    {
        return true;
    }
    else
    {
        errprint("JACK send_event() failed");
        return false;
    }
}

/**
 *  Work on this routine now in progress.  Unlike the ALSA implementation, we
 *  do not try to send large messages (greater than 255 bytes) in chunks.
 *
 *  The event::sysex data type is a vector of midi::bytes.  Also note that both
 *  Meta and Sysex messages are covered by the event :: is_ex_data() function
 *  and via the sysex() function to send data.
 *
 *  This SysEx capability is also used in the midicontrolout class to create
 *  an event that contains the MIDI bytes of a control macro, which can then
 *  be sent out via midi::masterbus::sysex().
 */

bool
midi_jack::send_sysex (const midi::event * ev)
{
    const midi::message & message = ev->get_message();
    bool result = send_message(message);
    if (! result)
    {
        errprint("JACK SysEx failed");
    }
    return result;
}

/**
 *  Connects two named JACK ports, but only if they are not virtual/manual
 *  ports.  First, we register the local port.  If this is nominally a local
 *  input port, it is really doing output, and this is the source-port name.
 *  If this is nominally a local output port, it is really accepting input,
 *  and this is the destination-port name.
 *
 * \param input
 *      Indicates true if the port to register and connect is an input port,
 *      and false if the port is an output port.  Useful macros for
 *      readability: midibase::io::input and midibase::io::output.
 *
 * \param srcportname
 *      Provides the destination port-name for the connection.  For input,
 *      this should be the name associated with the JACK client handle; it is
 *      the port that gets registered.  For output, this should be the full
 *      name of the port that was enumerated at start-up.  The JackPortFlags
 *      of the source port must include JackPortIsOutput.
 *
 * \param destportname
 *      For input, this should be full name of port that was enumerated at
 *      start-up.  For output, this should be the name associated with the
 *      JACK client handle; it is the port that gets registered.  The
 *      JackPortFlags of the destination port must include JackPortIsInput.
 *      Now, if this name is empty, that basically means that there is no such
 *      port, and we create a virtual port in this case.  So jack_connect() is
 *      not called in this case.  We do not treat this case as an error.
 *
 * \return
 *      If the jack_connect() call succeeds, true is returned.  If the port is
 *      a virtual (manual) port, then it is not connected, and true is
 *      returned without any action.
 */

bool
midi_jack::connect_ports        // IN OR OUT!!!!!
(
    midi::port::io iotype,
    const std::string & srcportname,
    const std::string & destportname
)
{
    bool result = true;
    midi_jack_data * jkdata = reinterpret_cast<midi_jack_data *>(api_data());
    result = ! srcportname.empty() && ! destportname.empty();
    if (result)
    {
        int rc = ::jack_connect
        (
            jkdata->jack_client(), srcportname.c_str(), destportname.c_str()
        );
        result = rc == 0;
        if (! result)
        {
            if (rc == EEXIST)
            {
                /*
                 * Probably not worth emitting a warning to the console.
                 */
            }
            else
            {
                bool input = iotype == midi::port::io::input;
                std::string msg = "JACK Connect error";
                msg += input ? "input '" : "output '";
                msg += srcportname;
                msg += "' to '";
                msg += destportname;
                msg += "'";
                error(rterror::kind::driver_error, msg);
            }
        }
    }
    return result;
}

#endif          // defined RTL66_MIDI_EXTENSIONS

/*------------------------------------------------------------------------
 * midi_jack output
 *------------------------------------------------------------------------*/

/**
 *  Writes the full message to buffer. Waits for JACK write space, then
 *  writes the size of the message, then the message itself.
 */

bool
midi_jack::send_message (const midi::byte * message, size_t sz)
{
    bool result = not_nullptr(message) && sz > 0;
    if (result)
    {
        midi::message msg;
        for (size_t i = 0; i < sz; ++i)
            msg.push(message[i]);

        result = send_message(msg);
    }
    return result;
#if 0
        while
        (
            ::jack_ringbuffer_write_space(jkdata->buffer()) <
                sizeof(nbytes) + sz
        )
        {
            pthread_yield();
        }
#endif
}

/**
 *  Sends a JACK MIDI output message.  It writes the full message size and
 *  the message itself to the JACK ring buffer (actually our new ring_buffer
 *  template. The timestamp was not provided by the original "rtmidi"
 *  implementation.  This new data and the custom ringbuffer are
 *  added for issue #100.
 *
 *  Let's try the frame count instead of the timestamp.  See the ttymidi.c
 *  module.
 *
 * \param message
 *      Provides the MIDI message object, which contains the bytes to send.
 *
 * \return
 *      Returns true if the buffer message and buffer size seem to be written
 *      correctly.
 */

bool
midi_jack::send_message (const midi::message & message)
{
    xpc::ring_buffer<midi::message> * rb = jack_data().jack_buffer();
    return rb->push_back(message);
}

}           // namespace rtl

#endif      // defined RTL66_BUILD_JACK

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

