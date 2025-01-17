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
 * \file          midi_alsa.cpp
 *
 *    Implements the ALSA MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-05-14
 * \license       See above.
 *
 * To do:
 *
 *      Replace malloc() with new/delete or a wrapper class.
 */

#include "rtl/midi/alsa/midi_alsa.hpp"  /* rtl::midi_alsa class             */

#if defined RTL66_BUILD_ALSA

#include <sstream>                      /* std::ostringstream class         */

#if defined PLATFORM_DEBUG
#include <iostream>                     /* std::cerr, cout classes          */
#endif

#include "rtl66-config.h"               /* RTL66_HAVE_XXX                   */
#include "midi/calculations.hpp"        /* midi::tempo_us_from_bpm()        */
#include "midi/event.hpp"               /* midi::event class                */
#include "midi/eventcodes.hpp"          /* midi::is_sysex_end()             */
#include "midi/ports.hpp"               /* midi::ports                      */
#include "rtl/midi/alsa/midi_alsa_data.hpp"  /* rtl::rtmidi::midi_alsa_data */

namespace rtl
{

/*------------------------------------------------------------------------
 * ALSA free functions
 *------------------------------------------------------------------------*/

/**
 *  ALSA detection function.  Just opens a client without activating it, then
 *  closes it.  Also note that we must use the name "default", not a name of
 *  our choosing.
 */

bool
detect_alsa (bool checkports)
{
    bool result = false;
    snd_seq_t * alsaman;
    int rc = ::snd_seq_open
    (
        &alsaman, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK
    );
    if (rc == 0)
    {
        if (checkports)
        {
            // TODO
        }
        result = true;
        rc = ::snd_seq_close(alsaman);
        if (rc < 0)
        {
            errprint("error closing ALSA detection");
        }
    }
    else
    {
        errprint("ALSA not detected");
    }
    return result;
}

/**
 * Since this is build info, we do not use the run-time version of ALSA
 * that can be obtained from snd_asoundlib_version().
 *
 * This might be goofy!
 */

#if defined RTL66_USE_GLOBAL_CLIENTINFO

void
set_alsa_version ()
{
#if defined SND_LIB_VERSION_STR
    std::string jv{SND_LIB_VERSION_STR};
    midi::global_client_info().api_version(jv);
#endif
}

#else

void
set_alsa_version ()
{
    // no code
}

#endif

/**
 *  The ALSA Sequencer API is based on the use of a callback function for MIDI
 *  input.  API information found at
 *
 *      http://www.alsa-project.org/documentation.php#Library
 *
 *  Thanks to Pedro Lopez-Cabanillas for help with the ALSA sequencer time stamps
 *  and other assorted fixes!!!
 *
 *  If you don't need timestamping for incoming MIDI events, define the
 *  preprocessor definition RTL66_ALSA_AVOID_TIMESTAMPING to save resources
 *  associated with the ALSA sequencer queues.
 *
 *  For example, Seq66 seems to need this macro to be defined.
 *
 *      #define RTL66_ALSA_AVOID_TIMESTAMPING
 *
 *  If you want to include the "announce" port, as done ins Seq66, define this
 *  macro:
 *
 *      #define RTL66_ALSA_ANNOUNCE_PORT
 *
 *  Note that there are two known ALSA ports (clients):
 *
 *      -   SND_SEQ_PORT_SYSTEM_TIMER       = 0
 *      -   SND_SEQ_PORT_SYSTEM_ANNOUNCE    = 1
 *
 *  Not used:
 *
 *      #define PORT_TYPE (pinfo, bits ) \
 *              ((snd_seq_port_info_get_capability(pinfo) & (bits)) == (bits))
 */

/*------------------------------------------------------------------------
 * Time calculation
 *------------------------------------------------------------------------*/

static int
calculate_time
(
    snd_seq_real_time_t x,  /* the event time   */
    snd_seq_real_time_t y   /* the last time    */
)
{
    if (x.tv_nsec < y.tv_nsec)
    {
        int nsec = int(y.tv_nsec - x.tv_nsec) / 1000000000 + 1;
        y.tv_nsec -= 1000000000 * nsec;
        y.tv_sec += nsec;
    }
    if (x.tv_nsec - y.tv_nsec > 1000000000)
    {
        int nsec = int(x.tv_nsec - y.tv_nsec) / 1000000000;
        y.tv_nsec += 1000000000 * nsec;
        y.tv_sec -= nsec;
    }
    return int(x.tv_sec) - int(y.tv_sec) +
        int(x.tv_nsec - y.tv_nsec) * 1E-9;
}

/*------------------------------------------------------------------------
 * ALSA callbacks
 *------------------------------------------------------------------------*/

/**
 *  The ALSA sequencer has a maximum buffer size for MIDI sysex
 *  events of 256 bytes.  If a device sends sysex messages larger
 *  than this, they are segmented into 256 byte chunks.  So,
 *  we'll watch for this and concatenate sysex chunks into a
 *  single sysex message if necessary.
 *
 * Calculating the time-stamp:
 *
 * Method 1: Use the system time.
 *
 *      (void) gettimeofday(&tv, (struct timezone *) NULL);
 *      time = (tv.tv_sec * 1000000) + tv.tv_usec;
 *
 * Method 2: Use the ALSA sequencer event time data. Thanks to Pedro
 *           Lopez-Cabanillas!.
 *
 * Using method from:
 *
 *      https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 *
 *      Perform the carry for the later subtraction by updating y.
 *      Temp var y is timespec because computation requires signed types,
 *      while snd_seq_real_time_t has unsigned types.
 *
 * API data:
 *
 *      -#  Every API has an rtmidi_in_data structure [accessor: input_data()].
 *          -#  It's address is passed as an argument to pthread_create().
 *          -#  It has a void pointer to an API-specific data structure
 *              [midi_<APINAME>_data, accessor: api_data()].
 *      -#  The API-specific pointer can then be obtained by using a call to
 *          the API's data_cast() function.
 *
 * Riddles to solve:
 *
 *      -   Handling of 0xF9 MIDI Timing Tick.
 *      -   What's the status byte for MIDI Time Code? Is it the
 *          Quarter Frame (midi::status::quarter_frame) byte, 0xF1?
 *      -   Full Time Code: F0 7F 7F 01 01 hh mm ss ff F7
 *
 *  See https://en.wikipedia.org/wiki/MIDI_timecode
 */

void *
midi_alsa_handler (void * ptr)
{
    rtmidi_in_data * rtidata = midi_api::static_in_data_cast(ptr);
    midi_alsa_data * apidata = midi_alsa::static_data_cast(rtidata->api_data());
    double time;
    bool moresysex = false;
    bool dodecode = false;
    midi::message message;
    int poll_fd_count;
    struct pollfd * poll_fds;
    snd_seq_event_t * ev;

    /*
     * Why 0? That's the buffer size.  RtMidi does this, too.
     */

    int rc = ::snd_midi_event_new(0, apidata->event_address());
    if (rc < 0)
    {
        rtidata->do_input(false);
        error_print ("midi_alsa_handler", "error init'ing MIDI event parser");
        return nullptr;
    }

    midi::byte * buffer = new (std::nothrow) midi::byte[apidata->buffer_size()];
    if (is_nullptr(buffer))
    {
        rtidata->do_input(false);
        snd_midi_event_free(apidata->event_parser());   /* null check?  */
        apidata->event_parser(nullptr);
        error_print("midi_alsa_handler", "error init'ing buffer");
        return nullptr;
    }
    snd_midi_event_init(apidata->event_parser());       /* null check?  */

    /*
     * Suppress running status messages.
     */

    snd_midi_event_no_status(apidata->event_parser(), 1);
    poll_fd_count = 1 + ::snd_seq_poll_descriptors_count
    (
        apidata->alsa_client(), POLLIN
    );
    poll_fds = (struct pollfd *) alloca(poll_fd_count * sizeof(struct pollfd));
    ::snd_seq_poll_descriptors
    (
        apidata->alsa_client(), poll_fds + 1, poll_fd_count - 1, POLLIN
    );
    poll_fds[0].fd = apidata->trigger_fd(0);
    poll_fds[0].events = POLLIN;
    while (rtidata->do_input())
    {
        if (::snd_seq_event_input_pending(apidata->alsa_client(), 1) == 0)
        {
            // no data pending

            if (poll(poll_fds, poll_fd_count, -1) >= 0)
            {
                if (poll_fds[0].revents & POLLIN)
                {
                    bool dummy;
                    int rc = read(poll_fds[0].fd, &dummy, sizeof(dummy));
                    if (rc == (-1))
                        break;
                }
            }
            continue;
        }

        // If here, there should be data.

        int rc = ::snd_seq_event_input(apidata->alsa_client(), &ev);
        if (rc == -ENOSPC)
        {
            error_print("midi_alsa::midi_alsa_handler", "MIDI input overrun");
            continue;
        }
        else if (rc <= 0)
        {
            error_print("midi_alsa::midi_alsa_handler", "unknown input error");
            perror("System reports");
            continue;
        }

        /*
         * This is a bit weird, but we now have to decode an ALSA MIDI
         * event (back) into MIDI bytes.  We'll ignore non-MIDI types.
         */

        if (! moresysex)
            message.clear();

        dodecode = false;
        switch (ev->type )
        {
        case SND_SEQ_EVENT_PORT_SUBSCRIBED:
#if defined PLATFORM_DEBUG
            debug_print("midi_alsa::midi_alsa_handler", "port subscribed");
#endif
            break;

        case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
#if defined PLATFORM_DEBUG
            debug_print("midi_alsa::midi_alsa_handler", "port unsubscribed");
            std::cout
                << "sender = " << (int) ev->data.connect.sender.client << ":"
                << (int) ev->data.connect.sender.port
                << ", dest = " << (int) ev->data.connect.dest.client << ":"
                << (int) ev->data.connect.dest.port
                << std::endl
                ;
#endif
            break;

        case SND_SEQ_EVENT_QFRAME:                  // MIDI time code
            if (rtidata->allow_time_code())
                dodecode = true;
            break;

        case SND_SEQ_EVENT_TICK:                    // 0xF9 MIDI timing tick
            if (rtidata->allow_time_code())
                dodecode = true;
            break;

        case SND_SEQ_EVENT_CLOCK:                   // 0xF8 MIDI clock tick
            if (rtidata->allow_time_code())
                dodecode = true;
            break;

        case SND_SEQ_EVENT_SENSING:                 // 0xFE Active sensing
            if (rtidata->allow_active_sensing())
                dodecode = true;
            break;

        case SND_SEQ_EVENT_SYSEX:                   // 0xF0 System Exclusive
            if (rtidata->allow_sysex())
                break;

            if (ev->data.ext.len > apidata->buffer_size())
            {
                apidata->buffer_size(ev->data.ext.len);
                delete [] buffer;
                buffer = new (std::nothrow) midi::byte[apidata->buffer_size()];
                if (is_nullptr(buffer))
                {
                    rtidata->do_input(false);
                    error_print
                    (
                        "midi_alsa_in::midi_alsa_handler",
                        "error resizing buffer"
                    );
                    break;
                }
            }
            dodecode = true;
            break;

        default:
            dodecode = true;
        }
        if (dodecode)
        {
            long nbytes = snd_midi_event_decode
            (
                apidata->event_parser(), buffer, apidata->buffer_size(), ev
            );
            if (nbytes > 0)                 // see banner
            {
                if (! moresysex)
                {
                    message.assign(buffer, &buffer[nbytes]);
                }
                else
                {
                    message.append(buffer, &buffer[nbytes]);
                }
                moresysex = (ev->type == SND_SEQ_EVENT_SYSEX) &&
                    ! midi::is_sysex_end_msg(message.back());     // 0xF7

                if (! moresysex)
                {
                    /*
                     * Calculate the time stamp.  See the banner.
                     * Then compute the time difference.
                     */

                    time = calculate_time(ev->time.time, apidata->last_time());
                    apidata->last_time(ev->time.time);
                    if (rtidata->first_message())
                    {
                        rtidata->first_message(false);
                        message.jack_stamp(0.0);
                    }
                    else
                        message.jack_stamp(time);
                }
                else
                {
#if defined PLATFORM_DEBUG
                    error_print
                    (
                        "midi_alsa_in::midi_alsa_handler",
                        "parsing error/non-MIDI-event"
                    );
#endif
                }
            }
        }
        ::snd_seq_free_event(ev);
        if (message.empty() || moresysex)
            continue;

        if (rtidata->using_callback())
        {
            rtmidi_in_data::callback_t cb = rtidata->user_callback();
            cb(message.jack_stamp(), &message, rtidata->user_data());
        }
        else
        {
            /*
             * As long as we haven't reached our queue size limit, push the
             * message.
             */

            if (! rtidata->queue().push(message))
                error_print("midi_alsa_in", "message queue limit reached");
        }
    }
    if (not_nullptr(buffer))
        delete [] buffer;

    snd_midi_event_free(apidata->event_parser());
    apidata->event_parser(nullptr);
    apidata->thread_handle(apidata->dummy_thread_id());
    return 0;
}

/*------------------------------------------------------------------------
 * get_port_info() and related functions
 *------------------------------------------------------------------------*/

static const unsigned sm_input_caps =
    SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

static const unsigned sm_output_caps =
    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

static const unsigned sm_generic_caps =
    SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION;

/*------------------------------------------------------------------------
 * ALSA port capability strings (for troubleshooting)
 *------------------------------------------------------------------------*/

#if defined PLATFORM_DEBUG_TMI

/**
 *  See /usr/include/alsa/seq.h.
 *
 *  -   SND_SEQ_PORT_CAP_READ:          Readable from this port.
 *  -   SND_SEQ_PORT_CAP_WRITE:         Writable to this port.
 *  -   SND_SEQ_PORT_CAP_SYNC_READ:     Allow read subscriptions.
 *  -   SND_SEQ_PORT_CAP_SYNC_WRITE:    Allow write subscriptions.
 *  -   SND_SEQ_PORT_CAP_DUPLEX:        Allow read/write duplex.
 *  -   SND_SEQ_PORT_CAP_SUBS_READ:     Allow read subscription.
 *  -   SND_SEQ_PORT_CAP_SUBS_WRITE:    Allow write subscription.
 *  -   SND_SEQ_PORT_CAP_NO_EXPORT:     Routing not allowed.
 *
 */

static std::string
alsa_port_capabilities (unsigned bitmask)
{
    std::string result = "ALSA caps: ";
    if ((bitmask & SND_SEQ_PORT_CAP_READ) != 0)
        result += "read ";

    if ((bitmask & SND_SEQ_PORT_CAP_WRITE) != 0)
        result += "write ";

    if ((bitmask & SND_SEQ_PORT_CAP_SYNC_READ) != 0)
        result += "syncread ";

    if ((bitmask & SND_SEQ_PORT_CAP_SYNC_WRITE) != 0)
        result += "syncwrite ";

    if ((bitmask & SND_SEQ_PORT_CAP_DUPLEX) != 0)
        result += "duplex ";

    if ((bitmask & SND_SEQ_PORT_CAP_SUBS_READ) != 0)
        result += "subread ";

    if ((bitmask & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0)
        result += "subwrite ";

    if ((bitmask & SND_SEQ_PORT_CAP_NO_EXPORT) != 0)
        result += "noexport";

    return result;
}

#endif  // defined PLATFORM_DEBUG

/**
 *  Checks the port type for not being the "generic" types
 *  SND_SEQ_PORT_TYPE_MIDI_GENERIC and SND_SEQ_PORT_TYPE_SYNTH.
 *
 *  This check comes from Seq66, and might be incomplete.  We
 *  use not_a_midi_client() here instead.
 *
 *  Here is the complete list from /usr/include/alsa/seq.h:
 *
 *    SND_SEQ_PORT_TYPE_SPECIFIC        Message have device-specific semantics.
 *    SND_SEQ_PORT_TYPE_MIDI_GENERIC    Understands MIDI messages.
 *    SND_SEQ_PORT_TYPE_MIDI_GM         Compatible with GM spec.
 *    SND_SEQ_PORT_TYPE_MIDI_GS         Compatible with Roland GS standard.
 *    SND_SEQ_PORT_TYPE_MIDI_XG         Compatible with Yamaha XG spec.
 *    SND_SEQ_PORT_TYPE_MIDI_MT32       Compatible with Roland MT-32.
 *    SND_SEQ_PORT_TYPE_MIDI_GM2        Compatible with GM2 specification.
 *    SND_SEQ_PORT_TYPE_SYNTH           Understands SND_SEQ_EVENT_SAMPLE_xxx.
 *    SND_SEQ_PORT_TYPE_DIRECT_SAMPLE   Instruments can be downloaded.
 *    SND_SEQ_PORT_TYPE_SAMPLE          Similar to the preceding.
 *    SND_SEQ_PORT_TYPE_HARDWARE        Implemented in hardware.
 *    SND_SEQ_PORT_TYPE_SOFTWARE        Implemented in software.
 *    SND_SEQ_PORT_TYPE_SYNTHESIZER     Messages sent will generate sounds.
 *    SND_SEQ_PORT_TYPE_PORT            May connect to other devices.
 *    SND_SEQ_PORT_TYPE_APPLICATION     Belongs to an application.
 */

#if defined USE_CHECK_PORT_TYPE

static bool
check_port_type (snd_seq_port_info_t * pinfo)
{
    unsigned alsatype = snd_seq_port_info_get_type(pinfo);
    return
    (
        ((alsatype & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
        ((alsatype & SND_SEQ_PORT_TYPE_SYNTH) == 0)
    );
}

#endif

/**
 *  Encapsulates a number of MIDI port type checks.
 */

static bool
not_a_midi_client (unsigned ptype)
{
    return
    (
        ((ptype & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
        ((ptype & SND_SEQ_PORT_TYPE_SYNTH) == 0) &&
        ((ptype & SND_SEQ_PORT_TYPE_APPLICATION) == 0)
    );
}

/**
 *  2023-09-21. This check was added recently to the original RtMidi library.
 *
 *  ALSA: Avoid listing ports that are usually from 3rd.party managers or
 *  clients that have no subscriptable ports at all.  Avoid listing ports with
 *  SND_SEQ_PORT_CAP_NO_EXPORT (capabilities)
 */

static bool
no_routing_allowed (unsigned caps)
{
    return (caps & SND_SEQ_PORT_CAP_NO_EXPORT) != 0;
}

/**
 *  This function is used to count or get the pinfo structure for a given port
 *  number.  It is actually more like "get client info".
 *
 * SND_SEQ_CLIENT_SYSTEM:
 *
 *      This is the "announce" port; value of 0 in alsa/seq.h Client 0 won't
 *      have ports (timer and announce) that match the MIDI-generic and
 *      Synth types checked below.
 *
 *      In seq66, this is what is done, for input only:
 *
 *  #if ! defined RTL66_ALSA_ANNOUNCE_PORT
 *
 *      midi::midi_port_info & inputports;
 *      inputports.add
 *      (
 *          SND_SEQ_CLIENT_SYSTEM, "system",
 *          SND_SEQ_PORT_SYSTEM_ANNOUNCE, "announce",
 *          midi::port::io::input, midi::port::system,
 *          global_queue()
 *      );
 *
 *  #endif
 *
 * Notes:
 *
 *  snd_seq_client_info_set_client(cinfo, -1):
 *
 *      Sets the client id of a client_info container.  Why a -1 here?
 *      Because that tells snd_seq_query_next_client(seq, cinfo) to find
 *      the first client.
 *
 *  snd_seq_port_info_set_port(pinfo, -1):
 *
 *      Sets the port id of a port_info container.  The -1 tells tells
 *      snd_seq_query_next_port(seq, pinfo) to find the first port.
 *
 * \param seq
 *      Provides the ALSA client pointer, which must not be null.
 *
 * \param pinfo
 *      Provides the ALSA port pointer, which must not be null.
 *
 * \param capstype
 *      Provides the type of port to process.  The capabilities of
 *      the port must include only this capability.
 *
 * \param portnumber
 *      Provides the port number for which to get the data.
 *
 * \return
 *      The value returned is a kind of port count.  When the port count
 *      reaches portnumber, and the port is a match for capstype, then 1 is
 *      returned.  If a negative portnumber was used, the port count is
 *      returned.  Otherwise 0 is returned.
 */

static int
get_port_info
(
    snd_seq_t * seq,
    snd_seq_port_info_t * pinfo,
    unsigned capstype,
    int portnumber
)
{
    int count = 0;
    if (not_nullptr_2(seq, pinfo))
    {
        snd_seq_client_info_t * cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_client_info_set_client(cinfo, -1);          /* all clients  */
        while (::snd_seq_query_next_client(seq, cinfo) >= 0)
        {
            int client = ::snd_seq_client_info_get_client(cinfo);
            if (client == SND_SEQ_CLIENT_SYSTEM)            /* client == 0  */
            {
                continue;       /* skip ALSA "announce" or "timer" clients  */
            }
            ::snd_seq_port_info_set_client(pinfo, client);
            ::snd_seq_port_info_set_port(pinfo, -1);          /* all ports    */
            while (::snd_seq_query_next_port(seq, pinfo) >= 0)
            {
                unsigned ptype = ::snd_seq_port_info_get_type(pinfo);
                if (not_a_midi_client(ptype))
                    continue;

                unsigned caps = ::snd_seq_port_info_get_capability(pinfo);
#if defined PLATFORM_DEBUG_TMI
                std::string s = alsa_port_capabilities(caps);
                s += ": port ";
                s += std::to_string(count);
                infoprint(s.c_str());
#endif
                if ((caps & capstype) != capstype)
                    continue;

                /*
                 * Added recently to the original RtMidi library.
                 */

                if (no_routing_allowed(caps))
                    continue;

                if (count == portnumber)
                    return 1;

                ++count;
            }
        }
        if (portnumber < 0)     /* indicates to return the port count   */
            return count;
    }
    return 0;
}

/*------------------------------------------------------------------------
 * midi_alsa
 *------------------------------------------------------------------------*/

midi_alsa::midi_alsa () :
    midi_api        (),
    m_client_name   (),
    m_alsa_data     ()
{
    (void) initialize(client_name());
}

midi_alsa::midi_alsa
(
    midi::port::io iotype,
    const std::string & clientname,
    unsigned queuesize
) :
    midi_api        (iotype, queuesize),
    m_client_name   (clientname),
    m_alsa_data     ()
{
    (void) initialize(client_name());
}

midi_alsa::~midi_alsa ()
{
    delete_port();
}

/*------------------------------------------------------------------------
 * midi_alsa engine-related functions
 *------------------------------------------------------------------------*/

/**
 *  This function opens an ALSA-client connection.  It does this:
 *
 *  -   Opens the ALSA client:
 *      -   Input: opened in duplex (input/output), non-blocking mode.
 *      -   Output: opened in output, non-blocking mode.
 *      , and saves
 *      the snd_seq_t client handle in the midi_alsa_data structure held
 *      by this class instance.
 *  -   The desired client name is retrieved and set.
 */

void *
midi_alsa::engine_connect ()
{
    void * result = nullptr;
    snd_seq_t * seq;
    int streams = is_output() ? SND_SEQ_OPEN_OUTPUT : SND_SEQ_OPEN_DUPLEX;
    int mode = SND_SEQ_NONBLOCK;
    int rc = ::snd_seq_open(&seq, "default", streams, mode);
    if (rc == 0)
    {
        bool ok = set_seq_client_name(seq, client_name());
        if (ok)
            result = reinterpret_cast<void *>(seq);
        else
            (void) ::snd_seq_close(seq);
    }
    return result;
}

void
midi_alsa::engine_disconnect ()
{
    midi_alsa_data & data = alsa_data();
    snd_seq_t * c = data.alsa_client();
    if (not_nullptr(c))
    {
        int rc = ::snd_seq_close(c);
        data.alsa_client(nullptr);
        if (rc != 0)
        {
            error_print("snd_seq_close", "failed");
        }
    }
}

/**
 *  This function is called by the midi_alsa destructor, and deals with
 *  input versus output in slightly different ways. It first closes the
 *  connection. For input, it shuts down the input thread, then does cleanup,
 *  deletes the port, free its queue, etc.
 *
 *  Note that there is no need to delete the midi_alsa_data object, it is
 *  not allocated on the heap.
 *
 *  Also note that, currently, this is used only in the the destructor, so no
 *  need for a return value.
 */

void
midi_alsa::delete_port ()
{
    close_port();
    if (is_input())
        close_input_triggers();

    midi_alsa_data & data = alsa_data();
    if (data.vport() >= 0)
        snd_seq_delete_port(data.alsa_client(), data.vport());

    if (is_output())
    {
        if (not_nullptr(data.event_parser()))
            snd_midi_event_free(data.event_parser());

        /*
         * Created in midi_alsa_data.cpp line 101; perhaps we should
         * delete it in that module.
         *
         * if (data.buffer())
         *      free(data.buffer());
         */

        if (data.buffer())
            delete [] data.buffer();
    }
    else
    {
#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
        if (not_nullptr(data.alsa_client()))
            ::snd_seq_free_queue(data.alsa_client(), data.queue_id());
#endif
    }
    engine_disconnect();
}

void
midi_alsa::close_input_triggers ()
{
    midi_alsa_data & data = alsa_data();
    if (input_data().do_input())
    {
        bool doinput = false;
        input_data().do_input(doinput);

        int rc = write(data.trigger_fd(1), &doinput, sizeof(bool));
        if (rc != (-1))
            (void) join_input_thread();
    }
    close(data.trigger_fd(0));
    close(data.trigger_fd(1));
}

/**
 *  This function opens an ALSA-client connection.  It does this:
 *
 *  -   Opens the ALSA client:
 *      -   Input: opened in duplex (input/output), non-blocking mode.
 *      -   Output: opened in output, non-blocking mode.
 *      , and saves
 *      the snd_seq_t client handle in the midi_alsa_data structure held
 *      by this class instance.
 *  -   The desired client name is retrieved and set.
 */

bool
midi_alsa::connect ()
{
    bool result = false;
    midi_alsa_data & data = alsa_data();
    if (not_nullptr(data.alsa_client()))
        return true;

    snd_seq_t * c = client_handle(engine_connect());
    if (not_nullptr(c))
    {
        size_t buffersize = input_data().buffer_size();
//      result = data.initialize(c, is_input(), buffersize);  // ????
        result = data.initialize(c, port_io_type(), buffersize);  // ????
        if (result)
        {
            data.alsa_client(c);
            api_data(&data);
        }
    }
    return result;
}

/**
 *  Checks to see if a master client connection is available.  If so, it is
 *  logged as the client for the current port.
 */

bool
midi_alsa::reuse_connection ()
{
    bool result = master_is_connected();
    if (result)
    {
        snd_seq_t * seq = client_handle();
        result = not_nullptr(seq);
        if (result)
        {
            midi_alsa_data & data = alsa_data();
            data.alsa_client(seq);
            api_data(&data);
        }
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
midi_alsa::initialize (const std::string & clientname)
{
    bool result = true;
    if (! reuse_connection())
        result = connect();         /* calls midi_alsa_data::initialize()   */

    midi_alsa_data & data = alsa_data();
    if (result)
    {
        snd_seq_t * seq = data.alsa_client();
        if (is_output())
        {
            result = data.initialize(seq, port_io_type());      /* AGAIN !  */
            if (result)
                api_data(&data);
        }
        else
        {
            size_t inbuffersize = input_data().buffer_size();
            result = data.initialize(seq, port_io_type(), inbuffersize);
        }
        result = set_client_name(clientname);           /* can show errmsg  */
        if (result)
            client_name(clientname);                    /* necessary ???    */
    }
    if (result)
    {
        if (have_master_bus())
            master_bus()->client_handle(data.alsa_client());

        if (is_input())
        {
#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            midi::bpm bp = midi::global_client_info().global_bpm();
            midi::ppqn ppq = midi::global_client_info().global_ppqn();
            (void) set_seq_tempo_ppqn(data.alsa_client(), bp, ppq);
#endif
#endif
            input_data().api_data(reinterpret_cast<void *>(&data));
        }

        /*
         * Should already be done.
         *
         *      api_data(&data);
         */
    }
    else
    {
        error
        (
            rterror::kind::driver_error,
            "midi_alsa::initialize: error opening client"
        );
    }
    return result;
}

/**
 *  Wrapper/helper function.
 */

bool
midi_alsa::drain_output ()
{
    midi_alsa_data & data = alsa_data();
    int rc = snd_seq_drain_output(data.alsa_client());
    bool result = rc >= 0;
    if (! result)
        errprint(snd_strerror(rc));

    return result;
}

/**
 * Create the input queue and set arbitrary tempo (mm = 100) and
 * resolution (192).  FIXME/TODO
 */

bool
midi_alsa::set_seq_tempo_ppqn
(
    snd_seq_t * seq, midi::bpm bp,
#if defined RTL66_USE_GLOBAL_CLIENTINFO
    midi::ppqn /* ppq */
#else
    midi::ppqn ppq
#endif
)
{
    bool result = not_nullptr(seq);
    if (result)
    {
        unsigned tempo_us = unsigned(midi::tempo_us_from_bpm(bp));
#if defined RTL66_USE_GLOBAL_CLIENTINFO
        midi::ppqn ppq = midi::global_client_info().global_ppqn();
#endif
        midi_alsa_data & data = alsa_data();
        data.queue_id(snd_seq_alloc_named_queue(seq, "rtmidi queue"));

        snd_seq_queue_tempo_t * qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);

        /*
         * This function stores the current tempo in qtempo.  Not needed here.
         *
         *  (void) snd_seq_get_queue_tempo(apidata->alsa_client(), queue, qtempo);
         */

        snd_seq_queue_tempo_set_tempo(qtempo, tempo_us);
        snd_seq_set_queue_tempo(data.alsa_client(), data.queue_id(), qtempo);
        snd_seq_queue_tempo_set_ppq(qtempo, ppq);
        result = drain_output();
    }
    return result;
}


bool
midi_alsa::setup_input_port ()
{
    bool result = true;
    midi_alsa_data & data = alsa_data();
    if (! input_data().do_input())
    {
#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
        snd_seq_start_queue(data.alsa_client(), data.queue_id(), NULL);
        result = drain_output();
#endif
        result = start_input_thread(input_data());
        if (result)
        {
            input_data().do_input(true);
            is_connected(true);
        }
        else
        {
            (void) remove_subscription();
            input_data().do_input(false);
            error
            (
                rterror::kind::thread_error,
                "midi_alsa::open_port: error starting input thread"
            );
        }
    }
    return result;
}

/**
 *  This function implements registering a port with the ALSA client.
 */

bool
midi_alsa::open_port (int portnumber, const std::string & portname)
{
    if (is_connected())
    {
        error
        (
            rterror::kind::warning,
            "midi_alsa::open_port: connection already exists"
        );
        return true;
    }

    bool result = portnumber >= 0;                  /* -1 == uninit'ed      */
    if (result)
    {
        midi_alsa_data & data = alsa_data();
        int nsrc = get_port_count();
        snd_seq_port_info_t * src_pinfo = nullptr;  /* for input only       */
        snd_seq_port_info_t * dest_pinfo = nullptr; /* for output only      */
        result = nsrc > 0;
        if (result)
        {
            int pcount;
            if (is_output())
            {
                snd_seq_port_info_alloca(&dest_pinfo);
                pcount = get_port_info
                (
                    data.alsa_client(), dest_pinfo, sm_output_caps, portnumber
                );
            }
            else
            {
                snd_seq_port_info_alloca(&src_pinfo);
                pcount = get_port_info
                (
                    data.alsa_client(), src_pinfo, sm_output_caps, portnumber
                );
            }
            result = pcount > 0;
        }
        if (result)
        {
            snd_seq_addr_t sender;
            snd_seq_addr_t receiver;
            if (is_output())
            {
                receiver.client = snd_seq_port_info_get_client(dest_pinfo);
                receiver.port = snd_seq_port_info_get_port(dest_pinfo);
                sender.client = snd_seq_client_id(data.alsa_client());
                if (data.vport() < 0)
                {
                    /*
                     * Seems odd to be setting input capability here.
                     */

                    int rc = snd_seq_create_simple_port
                    (
                        data.alsa_client(), portname.c_str(),
                        sm_input_caps, sm_generic_caps
                    );
                    data.vport(rc);
                    if (rc < 0)
                        result = false;
                }
                if (result)
                {
                    sender.port = data.vport();

                    std::string errmsg;
                    result = midi_alsa::subscription
                    (
                        errmsg, sender, receiver
                    );
                }
            }
            else
            {
                sender.client = snd_seq_port_info_get_client(src_pinfo);
                sender.port = snd_seq_port_info_get_port(src_pinfo);
                receiver.client = snd_seq_client_id(data.alsa_client());

                snd_seq_port_info_t * pinfo;
                snd_seq_port_info_alloca(&pinfo);
                if (data.vport() < 0)
                {
                    /*
                     * Seems odd to be setting output capability here.
                     */

                    snd_seq_port_info_set_client(pinfo, 0);
                    snd_seq_port_info_set_port(pinfo, 0);
                    snd_seq_port_info_set_capability(pinfo, sm_output_caps);
                    snd_seq_port_info_set_type(pinfo, sm_generic_caps);
                    snd_seq_port_info_set_midi_channels(pinfo, 16);

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
                    snd_seq_port_info_set_timestamping(pinfo, 1);
                    snd_seq_port_info_set_timestamp_real(pinfo, 1);
                    snd_seq_port_info_set_timestamp_queue(pinfo, data.queue_id());
#endif
                    snd_seq_port_info_set_name(pinfo, portname.c_str());

                    int rc = snd_seq_create_port(data.alsa_client(), pinfo);
                    data.vport(rc);
                    if (rc < 0)
                    {
                        errprint(snd_strerror(rc));
                        result = false;
                    }
                    else
                    {
                        data.vport(snd_seq_port_info_get_port(pinfo));
                        receiver.port = data.vport();
                    }
                    if (result)
                    {
                        std::string errmsg;
                        result = midi_alsa::subscription
                        (
                            errmsg, sender, receiver
                        );
                    }
                    else
                        error("midi_alsa::open_port", portnumber);
                }
            }
        }
        if (result)
        {
            if (is_input())
                result = setup_input_port();
        }
        else
        {
            error
            (
                rterror::kind::warning,
                "midi_alsa::open_port: no sources"
            );
        }
    }
    return result;
}

/*
 * Wait for old thread to stop, if still running.  Then start the
 * input queue.  Then start the MIDI input thread.
 */

bool
midi_alsa::setup_input_virtual_port ()
{
    bool result = ! input_data().do_input();
    if (result)
    {
        midi_alsa_data & data = alsa_data();
        (void) join_input_thread();

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
        snd_seq_start_queue(data.alsa_client(), data.queue_id(), NULL);
        (void) drain_output();
#endif

        result = start_input_thread(input_data());
        if (result)
        {
            input_data().do_input(true);
        }
        else
        {
            (void) remove_subscription();
            input_data().do_input(false);
            error
            (
                rterror::kind::thread_error,
                "midi_alsa_in::open_port: error starting input thread"
            );
        }
    }
    return result;
}

/**
 *
 */

bool
midi_alsa::open_virtual_port (const std::string & portname)
{
    bool result = true;
    midi_alsa_data & data = alsa_data();
    if (is_output())
    {
        if (data.vport() < 0)
        {
            int rc = snd_seq_create_simple_port
            (
                data.alsa_client(), portname.c_str(),
                sm_input_caps, sm_generic_caps
            );
            data.vport(rc);
            result = rc == 0;
        }
    }
    else
    {
        if (data.vport() < 0)
        {
            snd_seq_port_info_t * pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_capability(pinfo, sm_output_caps);
            snd_seq_port_info_set_type(pinfo, sm_generic_caps);
            snd_seq_port_info_set_midi_channels(pinfo, 16);

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
            snd_seq_port_info_set_timestamping(pinfo, 1);
            snd_seq_port_info_set_timestamp_real(pinfo, 1);
            snd_seq_port_info_set_timestamp_queue(pinfo, data.queue_id());
#endif

            snd_seq_port_info_set_name(pinfo, portname.c_str());
            data.vport(snd_seq_create_port(data.alsa_client(), pinfo));
            result = data.vport() == 0;
            if (result)
            {
                data.vport(snd_seq_port_info_get_port(pinfo));
                result = setup_input_virtual_port();
            }
        }
    }
    if (! result)
    {
        error
        (
            rterror::kind::driver_error,
            "midi_alsa::open_virtual_port: error creating port"
        );
    }
    return result;
}

bool
midi_alsa::subscription
(
    std::string & errmsg,
    snd_seq_addr_t & sender,
    snd_seq_addr_t & receiver
)
{
    midi_alsa_data & data = alsa_data();
    snd_seq_port_subscribe_t * subscrib = data.subscription();    /* pointer  */
    bool result = is_nullptr(subscrib);
    if (result)
    {
        int rc = snd_seq_port_subscribe_malloc(&subscrib);
        if (rc >= 0)
        {
            snd_seq_port_subscribe_set_sender(subscrib, &sender);
            snd_seq_port_subscribe_set_dest(subscrib, &receiver);
            if (is_output())
            {
                snd_seq_port_subscribe_set_time_update(subscrib, 1);
                snd_seq_port_subscribe_set_time_real(subscrib, 1);
            }

            /*
             * Weird.  This fails with "Operation not permitted" when
             * VMPK is the input port selected.  Does not fail with
             * the Korg nanoKEY2.  Have noticed many issues with VMPK.
             * A virtual port issue?
             */

            rc = snd_seq_subscribe_port(data.alsa_client(), subscrib);
            if (rc == 0)
            {
                data.subscription(subscrib);
            }
            else
            {
                snd_seq_port_subscribe_free(data.subscription());
                data.subscription(nullptr);
                errmsg = "midi_alsa_in::open_port: error connecting port";
                errprint(snd_strerror(rc));
                result = false;
            }
        }
        else
        {
            errprint(snd_strerror(rc));
            result = false;
        }
    }
    return result;
}

bool
midi_alsa::remove_subscription ()
{
    midi_alsa_data & data = alsa_data();
    bool result = not_nullptr_2(data.alsa_client(), data.subscription());
    if (result)
    {
        snd_seq_unsubscribe_port(data.alsa_client(), data.subscription());
        snd_seq_port_subscribe_free(data.subscription());
        data.subscription(nullptr);
    }
    return result;
}

/**
 *  Obviously used only for input, but this code appears many times.
 */

bool
midi_alsa::start_input_thread (rtmidi_in_data & indata)
{
    bool result = true;
    if (is_input())
    {
        midi_alsa_data & data = alsa_data();
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);

        int err = pthread_create
        (
            data.thread_address(), &attr, midi_alsa_handler, &indata
        );
        pthread_attr_destroy(&attr);
        result = err == 0;
    }
    return result;
}

bool
midi_alsa::join_input_thread ()
{
    bool result = true;
    if (is_input())
    {
        midi_alsa_data & data = alsa_data();
        if (! pthread_equal(data.thread_handle(), data.dummy_thread_id()))
            pthread_join(data.thread_handle(), NULL);
    }
    return result;
}

/**
 *  Note that get_port_info() does a heckuva lotta work!
 */

int
midi_alsa::get_port_count ()
{
    int result = 0;
    midi_alsa_data & data = alsa_data();
    if (not_nullptr(data.alsa_client()))
    {
        unsigned caps = is_output() ? sm_output_caps : sm_input_caps ;
        snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        result = get_port_info(data.alsa_client(), pinfo, caps, -1);
    }
    return result;
}

/**
 *  In the RtMidi implementations of MidiInAlsa/MidiOutAlsa::getPortName().
 */

std::string
midi_alsa::get_port_name (int portnumber)
{
    std::string result;
    midi_alsa_data & data = alsa_data();
    if (not_nullptr(data.alsa_client()) && portnumber >= 0)
    {
        snd_seq_client_info_t * cinfo;
        snd_seq_port_info_t * pinfo;
        unsigned caps = is_output() ? sm_output_caps : sm_input_caps ;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);

        int pcount = get_port_info(data.alsa_client(), pinfo, caps, portnumber);
        if (pcount > 0)
        {
            int cnum = snd_seq_port_info_get_client(pinfo);
            snd_seq_get_any_client_info(data.alsa_client(), cnum, cinfo);

            /*
             * These lines added to make sure devices are listed with full
             * portnames added to ensure unique device names.
             */

            std::ostringstream os;
            os
                << snd_seq_client_info_get_name(cinfo) << ":"
                << snd_seq_port_info_get_name(pinfo) << " "
                << snd_seq_port_info_get_client(pinfo) << ":"
                << snd_seq_port_info_get_port(pinfo)
                ;
            result = os.str();
        }
    }
    if (result.empty())
        error(rterror::kind::warning, "midi_alsa::get_port_name: error");

    return result;
}

/**
 *  Output version just unsubscribes.
 *
 *  The input version stops the input queue.  Then stops the thread to avoid
 *  triggering the callback, since the port is intended to be closed.
 */

bool
midi_alsa::close_port ()
{
    bool result = is_connected();
    if (result)
    {
        result = remove_subscription();  // BEFORE OR AFTER???
        if (result)
        {
            if (is_input())
            {
                midi_alsa_data & data = alsa_data();

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
                snd_seq_stop_queue(data.alsa_client(), data.queue_id(), NULL);
                result = drain_output();
                if (result)
                    close_input_triggers();
            }
#endif
        }
    }
    is_connected(false);
    return result;
}

bool
midi_alsa::set_client_name (const std::string & clientname)
{
    midi_alsa_data & data = alsa_data();
    return set_seq_client_name(data.alsa_client(), clientname);
}

bool
midi_alsa::set_seq_client_name
(
    snd_seq_t * seq,
    const std::string & clientname
)
{
    bool result = not_nullptr(seq);
    if (result)
    {
        int rc = snd_seq_set_client_name(seq, clientname.c_str());
        result = rc == 0;
        if (! result)
        {
            char tmp[80];
            const char * msg = snd_strerror(rc);
            snprintf
            (
                tmp, sizeof tmp,
                "snd_seq_set_client_name error '%s' for client '%s'\n",
                msg, clientname.c_str()
            );
            errprint(tmp);
        }
    }
    return result;
}

bool
midi_alsa::set_port_name (const std::string & portname)
{
    midi_alsa_data & data = alsa_data();
    bool result = not_nullptr(data.alsa_client());
    if (result)
    {
        snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_get_port_info(data.alsa_client(), data.vport(), pinfo);
        snd_seq_port_info_set_name(pinfo, portname.c_str());
        snd_seq_set_port_info(data.alsa_client(), data.vport(), pinfo);

        /*
         * No deallocation of port info???
         */
    }
    return result;
}

/*------------------------------------------------------------------------
 * midi_alsa output-port functions
 *------------------------------------------------------------------------*/

bool
midi_alsa::flush_port ()
{
    bool result = true;
    if (is_output() && is_connected())
        result = drain_output();

    return result;
}

bool
midi_alsa::send_message (const midi::byte * message, size_t sz)
{
    midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
    size_t nbytes = sz;
    if (nbytes > apidata->buffer_size())
    {
        apidata->buffer_size(nbytes);
        int rc = snd_midi_event_resize_buffer(apidata->event_parser(), nbytes);
        if (rc != 0)
        {
            error
            (
                rterror::kind::driver_error,
                "midi_alsa::send_message: error resizing event buffer"
            );
            return false;
        }
        free(apidata->buffer());
        apidata->buffer((midi::byte *) malloc(apidata->buffer_size()));
        if (is_nullptr(apidata->buffer()))
        {
            error
            (
                rterror::kind::memory_error,
                "midi_alsa::initialize: error allocating buffer"
            );
            return false;
        }
    }
    for (size_t i = 0; i < nbytes; ++i)
        apidata->buffer()[i] = message[i];

    size_t offset = 0;
    while (offset < nbytes)
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);
        snd_seq_ev_set_source(&ev, apidata->vport());
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);
        int rc = snd_midi_event_encode
        (
            apidata->event_parser(), apidata->buffer() + offset,
            long(nbytes - offset), &ev
        );
        if (rc < 0)
        {
            error
            (
                rterror::kind::warning,
                "midi_alsa::send_message: event parsing error"
            );
            return false;
        }
        if (ev.type == SND_SEQ_EVENT_NONE)
        {
            error
            (
                rterror::kind::warning,
                "midi_alsa::send_message: incomplete message"
            );
            return false;
        }
        offset += rc;
        rc = snd_seq_event_output(apidata->alsa_client(), &ev);     /* send */
        if (rc < 0)
        {
            error
            (
                rterror::kind::warning,
                "midi_alsa::send_message: error"
            );
            return false;
        }
    }
    (void) drain_output();
    return true;
}

/*------------------------------------------------------------------------
 * Extensions
 *------------------------------------------------------------------------*/

/**
 *  It gets information on ALL ports, putting input data into one
 *  midi::clientinfo container, and putting output data into another
 *  midi::clientinfo container.  This function is not present in the original
 *  RtMidi library.
 *
 *  This function requires that the MIDI engine client already exist [via
 *  the connect() function].
 *
 * \auto iswriteable
 *      The kind of ports to find.  For example, for an Rtl66 list of output
 *      ports, we want to find all ports with "write" capabilities, such as
 *      FluidSynth.
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
 *      necessarily an error; there may be no ALSA apps running with exposed
 *      ports.  If there is no ALSA client, then -1 is returned.
 */

int
midi_alsa::get_io_port_info (midi::ports & ioports, bool preclear)
{
    int result = 0;
    midi_alsa_data & data = alsa_data();
    snd_seq_t * seq = data.alsa_client();
    if (preclear)
        ioports.clear();

    if (not_nullptr(seq))
    {
        bool iswriteable = is_output();
        snd_seq_port_info_t * pinfo;
        snd_seq_client_info_t * cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        while (snd_seq_query_next_client(seq, cinfo) >= 0)
        {
            int client = snd_seq_client_info_get_client(cinfo);
            if (client == SND_SEQ_CLIENT_SYSTEM)    /* i.e. 0 in alsa/seq.h */
            {
#if defined RTL66_ALSA_ANNOUNCE_PORT        /* unrecommended (Seq66 issues) */
                /*
                 * Client 0 won't have ports (timer and announce) that match
                 * the MIDI-generic and Synth types checked below. So we
                 * add it anyway.  But this feature is deprecated and
                 * undesirable.  Same for the timer port.
                 */

                if (! iswriteable)
                {
                    ioports.add
                    (
                        SND_SEQ_CLIENT_SYSTEM, "system",
                        SND_SEQ_PORT_SYSTEM_ANNOUNCE, "announce",
                        midi::port::io::input, midi::port::kind::system,
                        0   // TEMPORARY global_queue()
                    );
                    ++result;
                }
#else
                continue;
#endif
            }
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, client); /* reset query info */
            snd_seq_port_info_set_port(pinfo, -1);
            while (::snd_seq_query_next_port(seq, pinfo) >= 0)
            {
                unsigned ptype = snd_seq_port_info_get_type(pinfo);
                if (not_a_midi_client(ptype))   // check_port_type(pinfo))
                    continue;

                std::string clientname = snd_seq_client_info_get_name(cinfo);
                std::string portname = snd_seq_port_info_get_name(pinfo);
                int portnumber = snd_seq_port_info_get_port(pinfo);
                unsigned caps = snd_seq_port_info_get_capability(pinfo);

                /*
                 * Added recently to the original RtMidi library.
                 * Not sure that we need it here, though.
                 */

                if (no_routing_allowed(caps))
                    continue;

                bool can_add = iswriteable ?
                    (caps & sm_output_caps) == sm_output_caps :
                    (caps & sm_input_caps) == sm_input_caps ;

#if defined PLATFORM_DEBUG_TMI
                std::string s = alsa_port_capabilities(caps);
                s += "- ";
                s += clientname;
                infoprint(s.c_str());
#endif

                if (can_add)
                {
                    ioports.add
                    (
                        client, clientname, portnumber, portname,
                        midi::port::io::input, midi::port::kind::normal
                    );
                    ++result;
                }
                else
                {
                    /*
                     * When VMPK is running, we get this message for a
                     * client-name of 'VMPK Output'.
                     */

                    printf("Ignoring ALSA port '%s'\n", clientname.c_str());
                }
            }
        }
    }
    if (result == 0)
        result = (-1);

    return result;
}

#if defined RTL66_MIDI_EXTENSIONS

/*
 * --------------------------------------------------------------------------
 *  midi_alsa extensions
 * --------------------------------------------------------------------------
 *
 *  Other functions to consider porting:
 *
 *      void send_sysex (const event * ev)
 */

/**
 * Currently, this code is implemented in the midi_alsa_info module, since
 * it is a midi::masterbus function.  Note the implementation here, though.
 * Which actually gets used?
 */

bool
midi_alsa::PPQN (midi::ppqn ppq)
{
    bool result = is_output();
    if (result)
    {
        midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
        int queue = apidata->queue_id();
        snd_seq_queue_tempo_t * qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);

        int rc = snd_seq_get_queue_tempo(apidata->alsa_client(), queue, qtempo);
        if (rc == 0)
        {
            snd_seq_queue_tempo_set_ppq(qtempo, ppq);
            snd_seq_set_queue_tempo(apidata->alsa_client(), queue, qtempo);
        }
        else
            result = false;
    }
    return result;
}

/**
 *  Set the BPM value (beats per minute).  This is done by creating
 *  an ALSA tempo structure, adding tempo information to it, and then
 *  setting the ALSA sequencer object with this information.
 *
 *  We fill the ALSA tempo structure (snd_seq_queue_tempo_t) with the current
 *  tempo information, set the BPM value, put it in the tempo structure, and
 *  give the tempo value to the ALSA queue.
 *
 * \note
 *      Consider using snd_seq_change_queue_tempo() here if the ALSA queue has
 *      already been started.  It's arguments would be the client handle,
 *      m_queue, tempo (microseconds), and null.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

bool
midi_alsa::BPM (midi::bpm bp)
{
    bool result = is_output();
    if (result)
    {
        midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
        int queue = apidata->queue_id();
        unsigned tempo_us = unsigned(midi::tempo_us_from_bpm(bp));
        snd_seq_queue_tempo_t * qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);    /* allocate tempo struct    */

        int rc = snd_seq_get_queue_tempo(apidata->alsa_client(), queue, qtempo);
        if (rc == 0)
        {
            snd_seq_queue_tempo_set_tempo(qtempo, tempo_us);
            snd_seq_set_queue_tempo(apidata->alsa_client(), queue, qtempo);
        }
        else
            result = false;
    }
    return result;
}

bool
midi_alsa::send_byte (midi::byte evbyte)
{
    (void) evbyte;
    return false;       // TODO
}

/**
 *  This function gets the MIDI clock a-runnin'. It sends the MIDI Clock
 *  Start message.
 */

bool
midi_alsa::clock_start ()
{
    midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* memsets it to 0  */
    ev.type = SND_SEQ_EVENT_START;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, apidata->vport());       /* set the source   */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_event_output(apidata->alsa_client(), &ev);  /* pump into queue  */
    return true;
}

/**
 *  Generates the MIDI clock, starting at the given tick value.
 *  Also sets the event tag to 127 so the sequences won't remove it.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the starting tick, unused in the ALSA implementation.
 */

bool
midi_alsa::clock_send (midi::pulse /*tick*/)
{
    midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* clear event      */
    ev.type = SND_SEQ_EVENT_CLOCK;
    ev.tag = 127;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, apidata->vport());       /* set source       */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_event_output(apidata->alsa_client(), &ev);  /* pump into queue  */
    return true;
}

/**
 *  Stop the MIDI clock.
 */

bool
midi_alsa::clock_stop ()
{
    midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* memsets it to 0  */
    ev.type = SND_SEQ_EVENT_STOP;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, apidata->vport());       /* set the source   */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_event_output(apidata->alsa_client(), &ev);  /* pump into queue  */
    return true;
}

/**
 *  Continue from the given tick.  A simplification on send_message().
 *  It sends a MIDI Song Position event followed by a MIDI Continue event.
 *
 * \param tick
 *      The continuing tick, unused in the ALSA implementation here.
 *      The midibase::continue_from() function uses it.
 *
 * \param beats
 *      The beats value calculated by midibase::continue_from().
 */

bool
midi_alsa::clock_continue (midi::pulse /* tick */, midi::pulse beats)
{
    midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event          */
    ev.type = SND_SEQ_EVENT_CONTINUE;

    snd_seq_event_t evc;
    snd_seq_ev_clear(&evc);                         /* clear event          */
    evc.type = SND_SEQ_EVENT_SONGPOS;
    evc.data.control.value = beats;                 /* what about ticks?    */
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_fixed(&evc);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_priority(&evc, 1);
    snd_seq_ev_set_source(&evc, apidata->vport());      /* set the source   */
    snd_seq_ev_set_subs(&evc);
    snd_seq_ev_set_source(&ev, apidata->vport());
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_ev_set_direct(&evc);
    snd_seq_event_output(apidata->alsa_client(), &evc); /* send Song Pos.   */
    snd_seq_drain_output(apidata->alsa_client());       /* flush the output */
    snd_seq_event_output(apidata->alsa_client(), &ev);  /* send Continue    */
    return true;
}

/**
 *  Grab a MIDI event.  First, a rather large buffer is allocated on the stack
 *  to hold the MIDI event data.  Next, if the --alsa-manual-ports option is
 *  not in force, then we check to see if the event is a port-start,
 *  port-exit, or port-change event, and we prcess it, and are done.
 *
 *  Otherwise, we create a "MIDI event parser" and decode the MIDI event.
 *
 *  We've beefed up the error-checking in this function due to crashes we got
 *  when connected to VMPK and suddenly getting a rush of ghost notes, then a
 *  seqfault.  This also occurs in legacy seq66.  To reproduce, run VMPK and
 *  make it the input source.  Open a new pattern, turn on recording, and
 *  start the ALSA transport.  Record one note.  Then activate the button for
 *  "dump input to MIDI bus".  You will here the note through VMPK, then ghost
 *  notes start appearing and seq66/seq66 eventually crash.  A bug in VMPK, or
 *  our processing?  At any rate, we catch the bug now, and don't crash, but
 *  eventually processing gets swamped until we kill VMPK.  And we now have a
 *  note sounding even though neither app is running.  Really screws up ALSA!
 *
 * ALSA events:
 *
 *      The ALSA events are listed in the snd_seq_event_type enumeration in
 *      /usr/lib/alsa/seq_event.h, where the "normal" MIDI events (from Note
 *      On to Key Signature) have values ranging from 5 to almost 30.  But
 *      there are some special ALSA events we need to handle in a different
 *      manner (currently by ignoring them):
 *
 *      -  0x3c: SND_SEQ_EVENT_CLIENT_START
 *      -  0x3d: SND_SEQ_EVENT_CLIENT_EXIT
 *      -  0x3e: SND_SEQ_EVENT_CLIENT_CHANGE
 *      -  0x3f: SND_SEQ_EVENT_PORT_START
 *      -  0x40: SND_SEQ_EVENT_PORT_EXIT
 *      -  0x41: SND_SEQ_EVENT_PORT_CHANGE
 *      -  0x42: SND_SEQ_EVENT_PORT_SUBSCRIBED
 *      -  0x43: SND_SEQ_EVENT_PORT_UNSUBSCRIBED
 *
 *  We will add more special events here as we find them.
 *
 * VMPK:
 *
 *      This ALSA-based application is weird and causes weird behavior.
 *      Running it, letting Seq66 auto-connect, then hitting a piano key in
 *      VMPK, causes a Seq66 message "input FIFO overrun".  Later, VMPK
 *      crashes.
 *
 * \todo
 *      Also, we need to consider using the new remcount return code to loop
 *      on receiving events as long as we are getting them.
 *
 * \param inev
 *      The event to be set based on the found input event.  It is the
 *      destination for the incoming event.
 *
 * \return
 *      This function returns false if we are not using virtual/manual ports
 *      and the event is an ALSA port-start, port-exit, or port-change event.
 *      It also returns false if there is no event to decode.  Otherwise, it
 *      returns true.
 */

bool
midi_alsa::get_midi_event (midi::event * inev)
{
    bool result = false;
    snd_seq_event_t * ev;
    int remcount = snd_seq_event_input(client_handle(), &ev);
    if (remcount < 0 || is_nullptr(ev))
    {
        if (remcount == -EAGAIN)
        {
            // no input in non-blocking mode
        }
        else if (remcount == -ENOSPC)
        {
            errprint("input FIFO overrun");             /* see VMPK note    */
        }
        else
            errprint("snd_seq_event_input() failure");

        return false;
    }
    // if (! rc().manual_ports())
    switch (ev->type)
    {
    case SND_SEQ_EVENT_CLIENT_START:

        // result = show_event(ev, "Client start");
        break;

    case SND_SEQ_EVENT_CLIENT_EXIT:

        // result = show_event(ev, "Client exit");
        break;

    case SND_SEQ_EVENT_CLIENT_CHANGE:

        // result = show_event(ev, "Client change");
        break;

    case SND_SEQ_EVENT_PORT_START:
    {
        /*
         * Figure out how to best do this.  It has way too many parameters
         * now, and is currently meant to be called from mastermidibus.
         * See mastermidibase::port_start().
         *
         * port_start(masterbus, ev->data.addr.client, ev->data.addr.port);
         * api_port_start (mastermidibus & masterbus, int bus, int port)
         */

        // result = show_event(ev, "Port start");
        break;
    }
    case SND_SEQ_EVENT_PORT_EXIT:
    {
        /*
         * The port_exit() function is defined in mastermidibase and in
         * businfo.  They seem to cover this functionality.
         *
         * port_exit(masterbus, ev->data.addr.client, ev->data.addr.port);
         */

        // result = show_event(ev, "Port exit");
        break;
    }
    case SND_SEQ_EVENT_PORT_CHANGE:
    {
        // result = show_event(ev, "Port change");
        break;
    }
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:

        // result = show_event(ev, "Port subscribed");
        break;

    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:

        // result = show_event(ev, "Port unsubscribed");
        break;

    default:
#if defined PLATFORM_DEBUG_TMI
        // result = show_event(ev, "Port other");
#endif
        break;
    }
    if (result)
        return false;

    /*
     * ALSA documentation states that 12 bytes are enough for decoding
     * MIDI events except for SysEx. We probably need a "long sysex"
     * option.
     */

    const size_t buffersize = 256;              /* 12 enough but for SysEx  */
    midi::bytes buffer(buffersize);             /* pre-allocate the data    */
    snd_midi_event_t * midi_ev;                 /* make ALSA MIDI parser    */
    int rc = snd_midi_event_new(buffersize, &midi_ev);
    if (rc < 0 || is_nullptr(midi_ev))
    {
        errprint("snd_midi_event_new() failed");
        return false;
    }

    /*
     *  Note that ev->time.tick is always 0.  (Same in Seq32).  Not sure about
     *  this handling of SysEx data. Apparently one can get only up to ALSA
     *  buffer size (4096) of data.  Also, the snd_seq_event_input() function
     *  is said to block!
     */

    long bytecount = snd_midi_event_decode
    (
        midi_ev, buffer.data(), long(buffersize), ev
    );
    if (bytecount > 0)
    {
        result = inev->set_midi_event(ev->time.tick, buffer, bytecount);
        if (result)
        {
            midi_alsa_data * apidata =      // FIXME
                reinterpret_cast<midi_alsa_data *>(api_data());

            midi::bussbyte b = 0;       // TODO
#if defined THIS_CODE_IS_READY
            midi::bussbyte b = input_ports().get_port_index
            (
                int(ev->source.client), int(ev->source.port)
            );
#endif
            bool sysex = inev->is_sysex();
            inev->set_input_bus(b);
#if defined PLATFORM_DEBUG_TMI
            warnprintf("Input on buss %d\n", int(b));
#endif
            while (sysex)           /* sysex might be more than one message */
            {
                int remcount = snd_seq_event_input(apidata->alsa_client(), &ev);
                bytecount = snd_midi_event_decode
                (
                    midi_ev, buffer.data(), long(buffersize), ev
                );
                if (bytecount > 0)
                {
                    sysex = inev->append_sysex(buffer, bytecount);
                    if (remcount == 0)
                        sysex = false;
                }
                else
                    sysex = false;
            }
        }
        snd_midi_event_free(midi_ev);
        return true;
    }
    else
    {
        /*
         * This happens even at startup, before anything is really happening.
         */

        snd_midi_event_free(midi_ev);

#if defined PLATFORM_DEBUG    // _TMI
        errprintf("snd_midi_event_decode() returned %ld", bytecount);
#endif

        return false;
    }
}

/**
 *  This send_event() function takes a native event, encodes it to an ALSA MIDI
 *  sequencer event, sets the broadcasting to the subscribers, sets the
 *  direct-passing mode to send the event without queueing, and puts it in the
 *  queue.
 *
 * \threadsafe
 *
 * \param ev
 *      The event to be played on this bus.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel of the playback.  This channel is either the global MIDI
 *      channel of the sequence, or the channel of the event.  Either way, we
 *      mask it into the event status.
 */

bool
midi_alsa::send_event (const midi::event * evp, midi::byte channel)
{
    static const size_t s_event_size_max = 10;          /* from Seq66       */
    midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
    snd_midi_event_t * midi_ev;                         /* MIDI parser      */
    int rc = snd_midi_event_new(s_event_size_max, &midi_ev);
    if (rc == 0)
    {
        snd_seq_event_t ev;                             /* event memory     */
        midi::byte buff[4];                             /* temp data        */
        buff[0] = evp->get_status(channel);             /* status+channel   */
        evp->get_data(buff[1], buff[2]);                /* set the data     */
        snd_seq_ev_clear(&ev);                          /* clear event      */
        snd_midi_event_encode(midi_ev, buff, 3, &ev);   /* 3 raw bytes      */
        snd_midi_event_free(midi_ev);                   /* free parser      */
        snd_seq_ev_set_source(&ev, apidata->vport());   /* set source       */
        snd_seq_ev_set_subs(&ev);                       /* subscriber       */
        snd_seq_ev_set_direct(&ev);                     /* immediate        */
        snd_seq_event_output(apidata->alsa_client(), &ev);   /* pump to que */
        return true;
    }
    else
    {
        errprint("ALSA out-of-memory error");
        return false;
    }
}

#if defined RTL66_ALSA_REMOVE_QUEUED_ON_EVENTS

/**
 *  Deletes events in the queue.  This function is not used anywhere, and
 *  there was no comment about the intent/context of this function.
 */

void
midi_alsa::remove_queued_on_events (int tag)
{
    if (is_output())
    {
        midi_alsa_data * apidata = reinterpret_cast<midi_alsa_data *>(api_data());
        snd_seq_remove_events_t * remove_events;
        snd_seq_remove_events_malloc(&remove_events);
        snd_seq_remove_events_set_condition
        (
            remove_events, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH |
                SND_SEQ_REMOVE_IGNORE_OFF
        );
        snd_seq_remove_events_set_tag(remove_events, tag);
        snd_seq_remove_events(apidata->alsa_client(), remove_events);
        snd_seq_remove_events_free(remove_events);
    }
}

#endif          // defined RTL66_REMOVE_QUEUED_ON_EVENTS

#endif          // defined RTL66_MIDI_EXTENSIONS

}               // namespace rtl

#endif          // defined RTL66_BUILD_ALSA

/*
 * midi_alsa.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

