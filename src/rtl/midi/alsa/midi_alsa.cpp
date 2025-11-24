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
 *    Implements much of the ALSA MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-11-24
 * \license       See above.
 *
 */

#include "rtl/midi/alsa/midi_alsa.hpp"  /* rtl::midi_alsa class             */

#if defined RTL66_BUILD_ALSA

#include <sstream>                      /* std::ostringstream class         */

#if defined PLATFORM_DEBUG_TMI
#include <iostream>                     /* std::cerr, cout classes          */
#endif

#include "rtl66-config.h"               /* RTL66_HAVE_XXX                   */
#include "midi/calculations.hpp"        /* midi::tempo_us_from_bpm()        */
#include "midi/event.hpp"               /* midi::event class                */
#include "midi/eventcodes.hpp"          /* midi::is_sysex_end()             */
#include "midi/ports.hpp"               /* midi::ports                      */
#include "rtl/midi/alsa/midi_alsa_data.hpp"  /* rtl::midi_alsa_data         */

#if defined PLATFORM_DEBUG_TMI
#include "xpc/errornumbers.hpp"
#endif

/**
 *
 *  The ALSA Sequencer API is based on the use of a callback function for
 *  MIDI input. Thanks to Pedro Lopez-Cabanillas for help with the ALSA
 *  sequencer time stamps and other assorted fixes!
 *
 * RTL66_ALSA_AVOID_TIMESTAMPING
 *
 *  If you don't need timestamping for incoming MIDI events, define this
 *  preprocessor definition to save resources associated with the ALSA
 *  sequencer queues.
 */

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
    bool result { false };
    ::snd_seq_t * alsaman { nullptr };
    int rc
    {
        ::snd_seq_open
        (
            &alsaman, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK
        )
    };
    if (rc == 0)
    {
        if (checkports)
        {
            /*
             * Do we need to make the effort to check for
             * ports?
             */
        }
        result = true;
        rc = ::snd_seq_close(alsaman);
        if (rc < 0)
            error_print("detect_alsa()", "error closing client");
    }
    else
        error_print("detect_alsa()", "failed");

    return result;
}

/**
 * Since this is build info, we do not use the run-time version of ALSA
 * that can be obtained from snd_asoundlib_version().
 *
 * This might be goofy!
 */

void
set_alsa_version ()
{
#if defined SND_LIB_VERSION_STR
    std::string jv { SND_LIB_VERSION_STR };
    midi::global_client_info().api_version(jv);
#endif
}

#include "midi_alsa_handler.cpp"        /* input thread static functions    */

/*------------------------------------------------------------------------
 * get_port_info() and related functions
 *------------------------------------------------------------------------*/

static const unsigned sm_input_caps                             /* 0x21     */
{
    SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ
};

static const unsigned sm_output_caps                            /* 0x42     */
{
    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE
};

static const unsigned sm_generic_caps                           /* 0x100002 */
{
    SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
};

/*------------------------------------------------------------------------
 * ALSA port capability strings (for troubleshooting)
 *------------------------------------------------------------------------*/

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
    std::string result { "Caps:" };
    if ((bitmask & SND_SEQ_PORT_CAP_READ) != 0)
        result += " R";

    if ((bitmask & SND_SEQ_PORT_CAP_WRITE) != 0)
        result += " W";

    if ((bitmask & SND_SEQ_PORT_CAP_SYNC_READ) != 0)
        result += " SyncR";

    if ((bitmask & SND_SEQ_PORT_CAP_SYNC_WRITE) != 0)
        result += " SyncW";

    if ((bitmask & SND_SEQ_PORT_CAP_DUPLEX) != 0)
        result += " Duplex";

    if ((bitmask & SND_SEQ_PORT_CAP_SUBS_READ) != 0)
        result += " SubR";

    if ((bitmask & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0)
        result += " SubW";

    if ((bitmask & SND_SEQ_PORT_CAP_NO_EXPORT) != 0)
        result += " NoExport";

    return result;
}

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
    unsigned alsatype { snd_seq_port_info_get_type(pinfo) };
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
 * \param cinfo
 *      Provides the ALSA client info pointer, which must not be null.
 *
 * \param pinfo
 *      Provides the ALSA port info pointer, which must not be null.
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
    ::snd_seq_t * seq,
    ::snd_seq_client_info_t * cinfo,
    ::snd_seq_port_info_t * pinfo,
    unsigned capstype,
    int portnumber
)
{
    int count { 0 };
    if (not_nullptr_2(seq, pinfo))
    {
        ::snd_seq_client_info_set_client(cinfo, -1);        /* all clients  */
        while (::snd_seq_query_next_client(seq, cinfo) >= 0)
        {
            int client { ::snd_seq_client_info_get_client(cinfo) };
            if (client == SND_SEQ_CLIENT_SYSTEM)            /* client == 0  */
            {
                continue;       /* skip ALSA "announce" or "timer" clients  */
            }
            ::snd_seq_port_info_set_client(pinfo, client);
            ::snd_seq_port_info_set_port(pinfo, -1);          /* all ports    */
            while (::snd_seq_query_next_port(seq, pinfo) >= 0)
            {
                unsigned ptype { ::snd_seq_port_info_get_type(pinfo) };
                if (not_a_midi_client(ptype))
                    continue;

                unsigned caps { ::snd_seq_port_info_get_capability(pinfo) };
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

static int
get_port_info
(
    ::snd_seq_t * seq,
    ::snd_seq_port_info_t * pinfo,
    unsigned capstype,
    int portnumber
)
{
    if (not_nullptr_2(seq, pinfo))
    {
        ::snd_seq_client_info_t * cinfo;
        snd_seq_client_info_alloca(&cinfo);                 /* a macro      */
        return get_port_info(seq, cinfo, pinfo, capstype, portnumber);
    }
    return 0;
}

#if defined USE_SHOW_BASIC_CLIENT_INFO

/**
 *  These functions are to be enabled only for trouble-shooting.
 */

static void
show_basic_client_info (snd_seq_t * client)
{
    if (not_nullptr(client))
    {
        int clientid { ::snd_seq_client_id(client) };
        ::snd_seq_client_info_t * cinfo;
        snd_seq_client_info_alloca(&cinfo);
        ::snd_seq_get_any_client_info(client, clientid, cinfo);

        const char * cname { ::snd_seq_client_info_get_name(cinfo) };

        printf
        (
            "Client '%s' at %p; ID %d;\n",
            cname, client, clientid
        );
    }
    else
        printf("null client pointer\n");
}

static void
show_basic_port_info (snd_seq_t * client, bool isoutput, int portnumber)
{
    if (not_nullptr(port))
    {
        int clientid { ::snd_seq_client_id(client) };

        unsigned caps { is_output() ? sm_output_caps : sm_input_caps };
        ::snd_seq_client_info_t * cinfo;
        ::snd_seq_port_info_t * pinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);

        (void) get_port_info(client, ....

        printf
        (
            "Client %d at %p: Port   \n"
            clientid, client
        );
    }
    else
        printf("null client pointer\n");
}

#endif  // defined USE_SHOW_CLIENT_INFO


/*------------------------------------------------------------------------
 * midi_alsa constructors
 *------------------------------------------------------------------------*/

/*
 *  This constructor is for use the in masterbus overloads of the
 *  rtmidi_in and rtmidi_out constructors.
 *
 *  It allows delaying initialization until after setting the masterbus via
 *  the midi::bus I/O objects and the masterbus overloads of the
 *  rtl::rtmidi I/O objects.
 */

midi_alsa::midi_alsa
(
    midi::masterbus & mbus,
    midi::port::io iotype
) :
    midi_api        (mbus, iotype),
    m_client_name   (mbus.client_name()),
    m_poll_wrapper
    (
        reinterpret_cast<snd_seq_t *>(mbus.void_client_handle())
    ),
    m_alsa_data     ()
{
    /*
     *  (void) initialize(client_name());
     *
     * m_alsa_data.set_initialized(true);
     */
}

/**
 *  This constructor preserves (mostly) the RtMidi stand-alone port
 *  paradigm.
 */

midi_alsa::midi_alsa
(
    midi::port::io iotype,
    const std::string & clientname,
    unsigned queuesize
) :
    midi_api        (iotype, queuesize),
    m_client_name   (clientname),
    m_poll_wrapper  (),
    m_alsa_data     ()
{
    if (clientname.empty())
        client_name("rtl-alsa");

    (void) initialize(client_name());
}

/**
 *  MIDI ALSA destructor.
 */

midi_alsa::~midi_alsa ()
{
    bool canclose { is_engine() || ! has_master() };
    if (canclose)
    {
        midi_alsa_data & data { alsa_data() };
        close_midi_tempo_queue();

        ::snd_seq_t * s { data.alsa_client() };
        if (not_nullptr(s))
        {
            int rc { ::snd_seq_close(s) };              /* close client     */
            if (rc == 0)
                data.alsa_client(nullptr);
            else
                printf("~midi_alsa() client-close error\n");
        }
        int rc { ::snd_config_update_free_global() };   /* more cleanup     */
        if (rc != 0)
            printf("~midi_alsa() config-free error\n");
    }
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
 *  -   The desired client name is retrieved and set.
 *
 *  Where is this done: save the snd_seq_t client handle in the
 *  midi_alsa_data structure held by this class instance? In the
 *  connect() function. No, better to use the masterbus's client
 *  handle if set.
 */

void *
midi_alsa::engine_connect ()
{
    void * result { nullptr };
    if (has_master())
    {
        result = client_handle();       /* grabs masterbus's client handle  */
        if (not_nullptr(result))
        {
            set_seq_client_name(client_handle(), client_name());
#if defined PLATFORM_DEBUG_TMI
            printf("masterbus client handle = %p\n", (void *)(client_handle()));
#endif
        }
        else
        {
            error
            (
                rterror::kind::driver_error,
                "engine_connect(): null masterbus client"
            );
        }
    }
    else
    {
        int streams
        {
            is_output() ? SND_SEQ_OPEN_OUTPUT : SND_SEQ_OPEN_DUPLEX
        };
        int mode { SND_SEQ_NONBLOCK };
        ::snd_seq_t * seq { nullptr };
        int rc { ::snd_seq_open(&seq, "default", streams, mode) };
        if (rc == 0)
        {
            bool ok { set_seq_client_name(seq, client_name()) };
            if (ok)
            {
                if (is_input())
                    (void) m_poll_wrapper.initialize(seq);  // NEW

                if (is_engine())
                {
                    rc = ::snd_seq_alloc_queue(seq);    /* tempo queue id   */
                    if (rc >= 0)
                        midi_tempo_queue(rc);
                }
                result = seq;           /* reinterpret_cast<void *>(seq)    */
            }
            else
            {
                /*
                 * (void) ::snd_seq_close(seq);
                 */
            }
        }
    }
    return result;
}

/**
 *  Closes the client if ...
 */

void
midi_alsa::engine_disconnect ()
{
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_t * c { data.alsa_client() };
    if (not_nullptr(c))
    {
        close_midi_tempo_queue();                   // new ca 2025-09-19
        int rc { ::snd_seq_close(c) };
        (void) ::snd_config_update_free_global();   /* new: more cleanup    */
        data.alsa_client(nullptr);
        // remove_poll_descriptors();
        if (rc != 0)
            error_print("snd_seq_close()", "failed");
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
    midi_alsa_data & data { alsa_data() };
    close_port();
    if (is_input())
        close_input_triggers();

    if (data.vport() >= 0)
        ::snd_seq_delete_port(data.alsa_client(), data.vport());

    if (is_output())
    {
        if (not_nullptr(data.event_parser()))
            ::snd_midi_event_free(data.event_parser());

        /*
         * Created in midi_alsa_data.cpp line 101; perhaps we should
         * delete it in that module.
         */

        data.unallocate();
    }
    else
    {
#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
        if (not_nullptr(data.alsa_client()))
            ::snd_seq_free_queue(data.alsa_client(), data.queue_id());
#endif
    }
    if (! has_master())
        engine_disconnect();
}

void
midi_alsa::close_input_triggers ()
{
    midi_alsa_data & data { alsa_data() };
    if (input_data().do_input())
    {
        bool doinput { false };
        input_data().do_input(doinput);

        int rc { int(write(data.trigger_fd(1), &doinput, sizeof(bool))) };
        if (rc != (-1))
        {
            // if ( !pthread_equal(data->thread, data->dummy_thread_id) )
            (void) join_input_thread();
        }
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
 *      -   Saves the snd_seq_t client handle in the midi_alsa_data
 *          structure held by this class instance.
 *  -   The desired client name is retrieved and set.
 */

bool
midi_alsa::connect ()
{
    midi_alsa_data & data { alsa_data() };
    if (not_nullptr(data.alsa_client()))
        return true;

    ::snd_seq_t * c { client_handle(engine_connect()) };
    bool result { not_nullptr(c) };
    if (result)
    {
        /*
         * This is done in midi_alsa::initialize() after connect() is
         * called.
         *
         *  size_t buffersize = input_data().buffer_size();
         *  result = data.initialize(c, port_io_type(), buffersize);
         */

        data.alsa_client(c);
        api_data(&data);
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
    bool result { master_is_connected() };
    if (result)
    {
        ::snd_seq_t * seq { client_handle() };
        result = not_nullptr(seq);
        if (result)
        {
            midi_alsa_data & data { alsa_data() };
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
 *  querying each port serially.
 *
 *  What implications does this have for latency, thread starvation, and
 *  total processor usage? Does JACK do thread-pooling?
 */

bool
midi_alsa::initialize (const std::string & clientname)
{
    bool result { true };
    if (! reuse_connection())
        result = connect();         /* calls midi_alsa_data::initialize()   */

    midi_alsa_data & data { alsa_data() };
    api_data(&data);
    if (result)
    {
        ::snd_seq_t * seq { data.alsa_client() };
        if (is_output())
        {
            result = data.initialize(seq, port_io_type());      /* AGAIN !  */
            if (result)
            {
                result = data.new_event_parser();
                if (! result)
                {
                    error
                    (
                        rterror::kind::driver_error,
                        "initialize(): error init'ing event parser"
                    );
                }
            }
        }
        else
        {
            size_t inbuffersize { input_data().buffer_size() };
            result = data.initialize(seq, port_io_type(), inbuffersize);
        }
        result = set_client_name(clientname);           /* can show errmsg  */
        if (result)
            client_name(clientname);                    /* make it official */
    }
    if (result)
    {
        if (is_input())
        {
#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
            midi::bpm bp { midi::global_client_info().global_bpm() };
            midi::ppqn ppq { midi::global_client_info().global_ppqn() };
            (void) set_seq_tempo_ppqn(data.alsa_client(), bp, ppq);
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
            "initialize(): error opening client"
        );
    }
    return result;
}

/**
 *  Wrapper/helper function. Notes on snd_seq_drain_output:
 *
 *  It returns 0 when all events are drained and sent to sequencer.
 *  When events still remain on the buffer, the byte size of remaining
 *  events are returned. On error a negative error code is returned.
 */

bool
midi_alsa::drain_output () const
{
    const midi_alsa_data & data { alsa_data() };
    midi_alsa_data & mad_data { const_cast<midi_alsa_data &>(data) };
    int rc { ::snd_seq_drain_output(mad_data.alsa_client()) };
    bool result { rc >= 0 };
#if defined PLATFORM_DEBUG_TMI
    printf
    (
        "ALSA client handle = %p, %s\n",
        (void *)(mad_data.alsa_client()),
        result ? "success" : "failure"
    );
#endif
    if (! result)
    {
        error_print("drain_output() --> ", snd_strerror(rc));
#if defined PLATFORM_DEBUG_TMI
        printf("Error code %d = %s\n", errno, V(xpc::errno_name(errno)));
#endif
    }
    return result;
}

/**
 * Create the input queue and set arbitrary tempo (mm = 100) and
 * resolution (192).  FIXME/TODO
 *
 * This function stores the current tempo in qtempo.  Not needed here.
 *
 *  (void) snd_seq_get_queue_tempo(apidata->alsa_client(), queue, qtempo);
 */

bool
midi_alsa::set_seq_tempo_ppqn
(
    ::snd_seq_t * seq, midi::bpm bp,
    midi::ppqn /* ppq */
)
{
    bool result { not_nullptr(seq) };
    if (result)
    {
        unsigned tempo_us { unsigned(midi::tempo_us_from_bpm(bp)) };
        midi::ppqn ppq { midi::global_client_info().global_ppqn() };
        midi_alsa_data & data { alsa_data() };
        data.queue_id(snd_seq_alloc_named_queue(seq, "rtmidi queue"));

        ::snd_seq_queue_tempo_t * qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);        /* this is a macro      */
        ::snd_seq_queue_tempo_set_tempo(qtempo, tempo_us);
        ::snd_seq_set_queue_tempo(data.alsa_client(), data.queue_id(), qtempo);
        ::snd_seq_queue_tempo_set_ppq(qtempo, ppq);
        result = drain_output();
    }
    return result;
}

/**
 *  In what situations does the input thread need to be used?
 *
 *      -   When using an input callback.
 *      -   When not using midi_alsa::get_message() or
 *          midi_alsa::get_midi_event().
 */

bool
midi_alsa::setup_input_port ()
{
    bool result { true };
    midi_alsa_data & data { alsa_data() };
    if (! input_data().do_input())
    {
#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING

        /*
         * Start the input queue. This function is a macro.
         */

        snd_seq_start_queue(data.alsa_client(), data.queue_id(), NULL);
        result = drain_output();
#endif
        bool startthread { use_internal_thread() };
        if (has_master())
            startthread = master_bus()->use_input_thread();

        if (startthread)
            result = start_input_thread(input_data());

        if (result)
        {
            input_data().do_input(true);
            is_connected(true);             // ?
        }
        else
        {
            (void) remove_subscription();
            input_data().do_input(false);
            error
            (
                rterror::kind::thread_error,
                "setup_input_port(): error starting thread"
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
        error_print("open_port()", "connection already exists");
        return true;
    }

    bool result { portnumber >= 0 };                    /* -1 == uninit'ed  */
    if (result)
    {
        midi_alsa_data & data { alsa_data() };
        int nsrc { get_port_count() };
        ::snd_seq_port_info_t * src_pinfo { nullptr };  /* input only       */
        ::snd_seq_port_info_t * dest_pinfo { nullptr }; /* output only      */
        result = nsrc > 0;
        if (! result)
        {
            error_print("open_port()", "no MIDI ports");
            return false;
        }

#if defined PLATFORM_DEBUG_TMI
        printf
        (
            "open_port() %s #%d client handle = %p\n",
            V(port_io_string()), portnumber,
            (void *)(data.alsa_client())
        );
#endif
        if (result)             /* compare get_port_info() to portInfo()    */
        {
            int pcount;
            if (is_output())
            {
                snd_seq_port_info_alloca(&dest_pinfo);  /* a macro          */
                pcount = get_port_info
                (
                    data.alsa_client(), dest_pinfo, sm_output_caps, portnumber
                );
            }
            else
            {
                snd_seq_port_info_alloca(&src_pinfo);   /* a macro          */
                pcount = get_port_info
                (
                    data.alsa_client(), src_pinfo, sm_input_caps, portnumber
                );
            }
            result = pcount > 0;
        }
        if (result)
        {
            ::snd_seq_addr_t sender;
            ::snd_seq_addr_t receiver;
            if (is_output())
            {
                receiver.client = ::snd_seq_port_info_get_client(dest_pinfo);
                receiver.port = ::snd_seq_port_info_get_port(dest_pinfo);
                sender.client = ::snd_seq_client_id(data.alsa_client());
                if (data.vport() < 0)
                {
                    /*
                     * Seems odd to be setting input (READ) capability here.
                     */

                    int portnum
                    {
                        ::snd_seq_create_simple_port
                        (
                            data.alsa_client(), CSTR(portname),
                            sm_input_caps, sm_generic_caps
                        )
                    };
                    if (portnum >= 0)
                        data.vport(portnum);
                    else
                        result = false;
                }
                if (result)
                {
                    sender.port = data.vport();

#if defined PLATFORM_DEBUG_TMI
                    printf
                    (
                        "1: sender %d:%d receiver %d:%d\n",
                        sender.client, sender.port,
                        receiver.client, receiver.port
                    );
#endif

                    std::string errmsg;
                    result = midi_alsa::subscription
                    (
                        errmsg, sender, receiver
                    );
                }
            }
            else                                                /* is input */
            {
                sender.client = ::snd_seq_port_info_get_client(src_pinfo);
                sender.port = ::snd_seq_port_info_get_port(src_pinfo);
                receiver.client = ::snd_seq_client_id(data.alsa_client());

                ::snd_seq_port_info_t * pinfo;
                snd_seq_port_info_alloca(&pinfo);               /* a macro  */
                if (data.vport() < 0)
                {
                    /*
                     * Seems odd to be setting output capability here.
                     * But that's what RtMidi does.
                     */

                    ::snd_seq_port_info_set_client(pinfo, 0);
                    ::snd_seq_port_info_set_port(pinfo, 0);
                    ::snd_seq_port_info_set_capability(pinfo, sm_output_caps);
                    ::snd_seq_port_info_set_type(pinfo, sm_generic_caps);
                    ::snd_seq_port_info_set_midi_channels(pinfo, 16);

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
                    ::snd_seq_port_info_set_timestamping(pinfo, 1);
                    ::snd_seq_port_info_set_timestamp_real(pinfo, 1);
                    ::snd_seq_port_info_set_timestamp_queue
                    (
                        pinfo, data.queue_id()
                    );
#endif
                    ::snd_seq_port_info_set_name(pinfo, CSTR(portname));

                    int vp = ::snd_seq_create_port(data.alsa_client(), pinfo);
                    data.vport(vp);
                    if (vp < 0)
                    {
                        error_print("open_port(input)", snd_strerror(vp));
                        result = false;
                    }
                    else
                    {
                        data.vport(::snd_seq_port_info_get_port(pinfo));
                        receiver.port = data.vport();
                    }
                    if (result)
                    {
#if defined PLATFORM_DEBUG_TMI
                        printf
                        (
                            "2: sender %d:%d receiver %d:%d\n",
                            sender.client, sender.port,
                            receiver.client, receiver.port
                        );
#endif
                        std::string errmsg;
                        result = midi_alsa::subscription
                        (
                            errmsg, sender, receiver
                        );
                    }
                    else
                        error("open_port()", portnumber);
                }
            }
        }
        if (result)
        {
            port_number(portnumber);
            if (is_input())
                result = setup_input_port();
        }
        else
        {
            warning("open_port(): no sources");
            result = false;
        }
    }
    else
        error_print("open_port()", "no MIDI ports");

    return result;
}

/*
 * Wait for old thread to stop, if still running.  Then start the
 * input queue.  Then start the MIDI input thread.
 *
 * This function is a macro: snd_seq_start_queue().
 */

bool
midi_alsa::setup_input_virtual_port ()
{
    bool result { ! input_data().do_input() };
    if (result)
    {
        midi_alsa_data & data { alsa_data() };
        (void) join_input_thread();

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
        snd_seq_start_queue(data.alsa_client(), data.queue_id(), NULL);
        result = drain_output();
#endif
        bool startthread { true };
        if (has_master())
            startthread = master_bus()->use_input_thread();

        if (startthread)
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
                "setup_input_virtual_port(): error starting input thread"
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
    bool result { true };
    midi_alsa_data & data { alsa_data() };
    if (is_output())
    {
        if (data.vport() < 0)
        {
            int rc
            {
                ::snd_seq_create_simple_port
                (
                    data.alsa_client(), CSTR(portname),
                    sm_input_caps, sm_generic_caps
                )
            };
            data.vport(rc);
            result = rc == 0;
        }
    }
    else
    {
        if (data.vport() < 0)
        {
            ::snd_seq_port_info_t * pinfo;
            snd_seq_port_info_alloca(&pinfo);                   /* a macro  */
            ::snd_seq_port_info_set_capability(pinfo, sm_output_caps);
            ::snd_seq_port_info_set_type(pinfo, sm_generic_caps);
            ::snd_seq_port_info_set_midi_channels(pinfo, 16);

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING
            ::snd_seq_port_info_set_timestamping(pinfo, 1);
            ::snd_seq_port_info_set_timestamp_real(pinfo, 1);
            ::snd_seq_port_info_set_timestamp_queue(pinfo, data.queue_id());
#endif

            ::snd_seq_port_info_set_name(pinfo, CSTR(portname));
            data.vport(::snd_seq_create_port(data.alsa_client(), pinfo));
            result = data.vport() == 0;
            if (result)
            {
                data.vport(::snd_seq_port_info_get_port(pinfo));
                result = setup_input_virtual_port();
            }
        }
    }
    if (! result)
    {
        error
        (
            rterror::kind::driver_error,
            "open_virtual_port(): error creating port"
        );
    }
    return result;
}

bool
midi_alsa::subscription
(
    std::string & /*errmsg*/,
    ::snd_seq_addr_t & sender,
    ::snd_seq_addr_t & receiver
)
{
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_port_subscribe_t * subscrib { data.subscription() };
    bool result { is_nullptr(subscrib) };
    if (result)
    {
        int rc { ::snd_seq_port_subscribe_malloc(&subscrib) };
        if (rc >= 0)
        {
            ::snd_seq_port_subscribe_set_sender(subscrib, &sender);
            ::snd_seq_port_subscribe_set_dest(subscrib, &receiver);
            if (is_output())
            {
                ::snd_seq_port_subscribe_set_time_update(subscrib, 1);
                ::snd_seq_port_subscribe_set_time_real(subscrib, 1);
            }

            /*
             * Weird.  This fails with "Operation not permitted" when
             * VMPK is the input port selected.  Does not fail with
             * the Korg nanoKEY2.  Have noticed many issues with VMPK.
             * A virtual port issue?
             */

            rc = ::snd_seq_subscribe_port(data.alsa_client(), subscrib);
            if (rc == 0)
            {
                data.subscription(subscrib);
            }
            else
            {
                ::snd_seq_port_subscribe_free(data.subscription());
                data.subscription(nullptr);
                error_print("subscription()", snd_strerror(rc));
                result = false;
            }
        }
        else
        {
            error_print("subscription()", snd_strerror(rc));
            result = false;
        }
    }
    return result;
}

bool
midi_alsa::remove_subscription ()
{
    midi_alsa_data & data { alsa_data() };
    bool result = not_nullptr_2(data.alsa_client(), data.subscription());
    if (result)
    {
        ::snd_seq_unsubscribe_port(data.alsa_client(), data.subscription());
        ::snd_seq_port_subscribe_free(data.subscription());
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
    // printf("START_INPUT_THREAD()\n");   // CAN WE USE IOTHREAD HERE???

    bool result { true };
    if (is_input())
    {
        midi_alsa_data & data { alsa_data() };
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        indata.do_input(true);

        int err
        {
            pthread_create
            (
                data.thread_address(), &attr, midi_alsa_handler, &indata
            )
        };
        pthread_attr_destroy(&attr);
        result = err == 0;
    }
    return result;
}

bool
midi_alsa::join_input_thread ()
{
    bool result { true };
    if (is_input())
    {
        midi_alsa_data & data { alsa_data() };
        input_data().do_input(false);
        if (! pthread_equal(data.thread_handle(), data.dummy_thread_id()))
            pthread_join(data.thread_handle(), NULL);
    }
    return result;
}

/**
 *  Note that get_port_info() does a heckuva lotta work! Also, compare
 *  this function to the JACK version.
 */

int
midi_alsa::get_port_count ()
{
    int result { 0 };
    midi_alsa_data & data { alsa_data() };
    if (not_nullptr(data.alsa_client()))
    {
        unsigned caps { is_output() ? sm_output_caps : sm_input_caps };
        ::snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        result = get_port_info(data.alsa_client(), pinfo, caps, -1);
    }
    return result;
}

/**
 *  Our rtmidi implementations of MidiInAlsa/MidiOutAlsa::getPortName().
 */

std::string
midi_alsa::get_port_name (int portnumber)
{
    std::string result;
    midi_alsa_data & data { alsa_data() };
    if (not_nullptr(data.alsa_client()) && portnumber >= 0)
    {
        ::snd_seq_client_info_t * cinfo;
        ::snd_seq_port_info_t * pinfo;
        unsigned caps { is_output() ? sm_output_caps : sm_input_caps };
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);

        int pcount
        {
            get_port_info(data.alsa_client(), pinfo, caps, portnumber)
        };
        if (pcount > 0)
        {
            int cnum { ::snd_seq_port_info_get_client(pinfo) };
            ::snd_seq_get_any_client_info(data.alsa_client(), cnum, cinfo);

            /*
             * These lines added to make sure devices are listed with full
             * portnames added to ensure unique device names.
             */

            std::ostringstream os;
            os
                << ::snd_seq_client_info_get_name(cinfo) << ":"
                << ::snd_seq_port_info_get_name(pinfo) << " "
                << ::snd_seq_port_info_get_client(pinfo) << ":"
                << ::snd_seq_port_info_get_port(pinfo)
                ;
            result = os.str();
        }
    }
    if (result.empty())
        warning("get_port_name(): warning");

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
    bool result { is_connected() };
    if (result)
    {
        result = remove_subscription();
        if (result)
        {
            if (is_input())
            {
                midi_alsa_data & data { alsa_data() };

#if ! defined RTL66_ALSA_AVOID_TIMESTAMPING     /* stop the input queue     */
                ::snd_seq_stop_queue(data.alsa_client(), data.queue_id(), NULL);
                result = drain_output();
                if (result)
                    close_input_triggers();
#endif
            }
        }
    }
    is_connected(false);
    return result;
}

bool
midi_alsa::set_client_name (const std::string & clientname)
{
    midi_alsa_data & data { alsa_data() };
    return set_seq_client_name(data.alsa_client(), clientname);
}

bool
midi_alsa::set_seq_client_name
(
    ::snd_seq_t * seq,
    const std::string & clientname
)
{
    bool result { not_nullptr(seq) };
    if (result)
    {
        int rc { ::snd_seq_set_client_name(seq, CSTR(clientname)) };
        result = rc == 0;
        if (! result)
        {
            char tmp[80];
            const char * msg { snd_strerror(rc) };
            snprintf
            (
                tmp, sizeof tmp,
                "snd_seq_set_client_name error '%s' for client '%s'\n",
                msg, V(clientname)
            );
            error_print("set_seq_client_name()", tmp);
        }
    }
    return result;
}

bool
midi_alsa::set_port_name (const std::string & portname)
{
    midi_alsa_data & data { alsa_data() };
    bool result = not_nullptr(data.alsa_client());
    if (result)
    {
        ::snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        ::snd_seq_get_port_info(data.alsa_client(), data.vport(), pinfo);
        ::snd_seq_port_info_set_name(pinfo, CSTR(portname));
        ::snd_seq_set_port_info(data.alsa_client(), data.vport(), pinfo);

        /*
         * No deallocation of port info???
         */
    }
    return result;
}

/*------------------------------------------------------------------------
 * midi_alsa output-port functions
 *------------------------------------------------------------------------*/

/**
 *  This function acts on the ALSA client, not on a single port.
 *
 *  Note that flush_port() is not supported; see midi_api.hpp.
 */

bool
midi_alsa::flush ()
{
    bool result { true };
    if (is_output() && is_connected())
        result = drain_output();

    return result;
}

bool
midi_alsa::send_message (const midi::byte * msg, size_t sz) const
{
    size_t nbytes { sz };
    midi_alsa_data & data { const_cast<midi_alsa_data &>(alsa_data()) };
    if (nbytes > data.buffer_size())
    {
        bool ok { data.resize_event_parser(nbytes) };
        if (! ok)
        {
            error                       /* error() throws an rtl::rterror   */
            (
                rterror::kind::driver_error,
                "send_message(): error resizing buffer"
            );
        }
    }

    midi::byte * b { data.buffer() };
    if (not_nullptr(b))
    {
        for (size_t i = 0; i < nbytes; ++i)
            b[i] = msg[i];
    }
    else
        error(rterror::kind::driver_error, "send_message(): null buffer");

    bool ok = data.new_event_parser();
    if (! ok)
        error(rterror::kind::driver_error, "send_message(): out of memory");

    ::snd_midi_event_t * parser { data.event_parser() };
    size_t offset { 0 };
    while (offset < nbytes)
    {
        ::snd_seq_event_t ev;
        :: snd_seq_ev_clear(&ev);
        snd_seq_ev_set_source(&ev, data.vport());   /* macro? */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);

        long rc
        {
            ::snd_midi_event_encode
            (
                parser, b + offset, long(nbytes - offset), &ev
            )
        };
        if (rc < 0)
        {
            warning("send_message(): parsing error");
            return false;
        }
        if (ev.type == SND_SEQ_EVENT_NONE)
        {
            warning("send_message(): incomplete message");
            return false;
        }
        offset += rc;

        int ec { ::snd_seq_event_output(data.alsa_client(), &ev) };
        if (ec < 0)
        {
            warning("send_message(): output error");
            return false;
        }
    }
    (void) drain_output();
    (void) data.free_event_parser();
    return true;
}

/*------------------------------------------------------------------------
 * Extensions
 *------------------------------------------------------------------------*/

/**
 *  It gets information on all ports of either input or output type.
 *  This function requires that the MIDI engine client already exist [via
 *  the connect() function].
 *
 * \auto iswriteable
 *      The kind of ports to find.  For example, for an Rtl66 list of output
 *      ports, we want to find all ports with "write" capabilities, such as
 *      FluidSynth.
 *
 * \param [inout] ioports
 *      The list of ports to populate. It will either be a list of input ports
 *      (readable ports) or a list of output ports (writeable ports). Duplex
 *      ports are both. See the ports::port_io_types() function to determine
 *      the status.
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
    int result { 0 };
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_t * seq { data.alsa_client() };
    if (preclear)
        ioports.clear();

    if (not_nullptr(seq))
    {
        bool iswriteable { is_output() };           /* a midi_api function  */
        midi::port::io iotype
        {
            iswriteable ? midi::port::io::output : midi::port::io::input
        };
        ::snd_seq_port_info_t * pinfo;
        ::snd_seq_client_info_t * cinfo;

        bool match { iswriteable && ioports.are_output() };
        if (! match)
            match = ! iswriteable && ioports.are_input();

        if (! match)
            return 0;

        snd_seq_client_info_alloca(&cinfo);
        ::snd_seq_client_info_set_client(cinfo, -1);

        int index { 0 };
        while (::snd_seq_query_next_client(seq, cinfo) >= 0)
        {
            int client { ::snd_seq_client_info_get_client(cinfo) };
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
                        0 /*index*/  // TEMPORARY global_queue()
                    );
                    ++result;
                }
#else
                continue;
#endif
            }
            snd_seq_port_info_alloca(&pinfo);
            ::snd_seq_port_info_set_client(pinfo, client); /* reset query info */
            ::snd_seq_port_info_set_port(pinfo, -1);
            while (::snd_seq_query_next_port(seq, pinfo) >= 0)
            {
                unsigned ptype { ::snd_seq_port_info_get_type(pinfo) };
                if (not_a_midi_client(ptype))   // check_port_type(pinfo))
                    continue;

                std::string clientname { ::snd_seq_client_info_get_name(cinfo) };
                std::string portname { ::snd_seq_port_info_get_name(pinfo) };
                int portnumber { ::snd_seq_port_info_get_port(pinfo) };
                unsigned caps { ::snd_seq_port_info_get_capability(pinfo) };

                /*
                 * Added recently to the original RtMidi library.
                 * Not sure that we need it here, though.
                 */

                if (no_routing_allowed(caps))
                    continue;

                bool can_add
                {
                    iswriteable ?
                        (caps & sm_output_caps) == sm_output_caps : /* 0x42 */
                        (caps & sm_input_caps) == sm_input_caps     /* 0x21 */
                };
                if (can_add)
                {
#if defined PLATFORM_DEBUG_TMI
                    std::string s { alsa_port_capabilities(caps) };
                    printf
                    (
                        "[%d] Add %s ALSA buss '%s' #%d  "
                        "'%s'\n",
                        result, ( iswriteable ? "out" : "in" ),
                        V(clientname), portnumber, V(s)
                    );
#endif
                    ioports.add
                    (
                        client, clientname, portnumber, portname,
                        iotype, midi::port::kind::normal, result   /* index */
                    );
                    ++result;
                }
                else
                {
                    /*
                     * When VMPK is running, we get this message for a
                     * client-name of 'VMPK Output'.
                     */

                    std::string s { alsa_port_capabilities(caps) };
                    printf
                    (
                        "[%d] Skip %s ALSA buss '%s' #%d "
                        "'%s'\n",
                        index, ( iswriteable ? "out" : "in" ),
                        V(clientname), portnumber, V(s)
                    );
                }
            }
            ++index;
        }
    }
    if (result == 0)
        result = (-1);

    return result;
}

#if defined RTL66_MIDI_EXTENSIONS       /* leave this defined   */

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
    bool result { is_output() || is_engine() };
    if (result)
    {
        midi_alsa_data & data { alsa_data() };
        int q { midi_tempo_queue() };               /* data.queue_id()      */
        ::snd_seq_queue_tempo_t * qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);

        int rc { ::snd_seq_get_queue_tempo(data.alsa_client(), q, qtempo) };
        if (rc == 0)
        {
            ::snd_seq_queue_tempo_set_ppq(qtempo, ppq);
            ::snd_seq_set_queue_tempo(data.alsa_client(), q, qtempo);
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
    bool result = is_output() || is_engine();
    if (result)
    {
        midi_alsa_data & data { alsa_data() };
        int q { midi_tempo_queue() };                   /* data.queue_id()  */
        unsigned tempo_us { unsigned(midi::tempo_us_from_bpm(bp)) };
        ::snd_seq_queue_tempo_t * qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);            /* make tempo struc */

        int rc { ::snd_seq_get_queue_tempo(data.alsa_client(), q, qtempo) };
        if (rc == 0)
        {
            ::snd_seq_queue_tempo_set_tempo(qtempo, tempo_us);
            rc = ::snd_seq_set_queue_tempo(data.alsa_client(), q, qtempo);
            if (rc < 0)
                result = false;
        }
        else
            result = false;
    }
    return result;
}

/**
 *  If the ALSA MIDI tempo queue is valid, close it.
 */

void
midi_alsa::close_midi_tempo_queue ()
{
    int mtq { midi_tempo_queue() };
    if (mtq >= 0)
    {
        midi_alsa_data & data { alsa_data() };
        ::snd_seq_t * seq { data.alsa_client() };
        if (not_nullptr(seq))
        {
            ::snd_seq_event_t ev;
            ::snd_seq_ev_clear(&ev);                    /* memset it to 0   */
            ::snd_seq_stop_queue(seq, mtq, &ev);
            ::snd_seq_free_queue(seq, mtq);
        }
        midi_tempo_queue(-1);
    }
}

/**
 *  This function gets the MIDI clock a-runnin'. It sends the MIDI Clock
 *  Start message.
 */

bool
midi_alsa::clock_start ()
{
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* memsets it to 0  */
    ev.type = SND_SEQ_EVENT_START;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, data.vport());           /* set the source   */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_event_output(data.alsa_client(), &ev);      /* pump into queue  */
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
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* clear event      */
    ev.type = SND_SEQ_EVENT_CLOCK;
    ev.tag = 127;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, data.vport());           /* set source       */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_event_output(data.alsa_client(), &ev);      /* pump into queue  */
    return true;
}

/**
 *  Stop the MIDI clock.
 */

bool
midi_alsa::clock_stop ()
{
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* memsets it to 0  */
    ev.type = SND_SEQ_EVENT_STOP;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, data.vport());           /* set the source   */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_event_output(data.alsa_client(), &ev);      /* pump into queue  */
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
    midi_alsa_data & data { alsa_data() };
    ::snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                                  /* clear event  */
    ev.type = SND_SEQ_EVENT_CONTINUE;

    ::snd_seq_event_t evc;
    snd_seq_ev_clear(&evc);                                 /* clear event  */
    evc.type = SND_SEQ_EVENT_SONGPOS;
    evc.data.control.value = beats;                         /* no ticks?    */
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_fixed(&evc);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_priority(&evc, 1);
    snd_seq_ev_set_source(&evc, data.vport());
    snd_seq_ev_set_subs(&evc);
    snd_seq_ev_set_source(&ev, data.vport());
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                             /* immediate    */
    snd_seq_ev_set_direct(&evc);
    snd_seq_event_output(data.alsa_client(), &evc);         /* Song Pos.    */
    snd_seq_drain_output(data.alsa_client());               /* flush output */
    snd_seq_event_output(data.alsa_client(), &ev);          /* Continue     */
    return true;
}

/**
 *  Checks to see if events (midi::messages) are in the input queue.
 */

int
midi_alsa::poll_for_midi () const
{
    bool usepolling { false };          /* as opposed to checking the queue */
    if (has_master())
        usepolling = ! master_bus()->use_input_thread();

    if (usepolling)
    {
        return m_poll_wrapper.poll_for_midi();
    }
    else
    {
        const rtmidi_in_data & rtidata { input_data() };
        const midi_queue & mq { rtidata.queue() };
        return mq.count();
    }
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
 *  Buffers:
 *
 *      ALSA documentation states that 12 bytes are enough for decoding
 *      MIDI events except for SysEx. We probably need a "long sysex"
 *      option.
 *
 * VMPK:
 *
 *      This ALSA-based application is weird and causes weird behavior.
 *      Running it, letting Seq66 auto-connect, then hitting a piano key in
 *      VMPK, causes a Seq66 message "input overrun".  Later, VMPK
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
 *      returns true. Note that this function does not work if the input
 *      thread is getting events.
 */

bool
midi_alsa::get_midi_event (midi::event * inev)
{
    bool result { false };
    ::snd_seq_t * client { alsa_data().alsa_client() };
    ::snd_seq_event_t * ev;
    int remcount { ::snd_seq_event_input(client, &ev) };
    if (remcount < 0 || is_nullptr(ev))
    {
        if (remcount == -EAGAIN)
        {
            // no input in non-blocking mode
        }
        else if (remcount == -ENOSPC)
            error_print("get_midi_event()", "input overrun");
        else
            error_print("get_midi_event()", "failure");

        return false;
    }
    switch (ev->type)               // if (! rc().manual_ports())
    {
    case SND_SEQ_EVENT_CLIENT_START:
    case SND_SEQ_EVENT_CLIENT_EXIT:
    case SND_SEQ_EVENT_CLIENT_CHANGE:
    case SND_SEQ_EVENT_PORT_START:

        /*
         * Figure out how to best do this.  It has way too many parameters
         * now, and is currently meant to be called from mastermidibus.
         * See mastermidibase::port_start().
         *
         * port_start(masterbus, ev->data.addr.client, ev->data.addr.port);
         * api_port_start (mastermidibus & masterbus, int bus, int port)
         */

    case SND_SEQ_EVENT_PORT_EXIT:

        /*
         * The port_exit() function is defined in mastermidibase and in
         * businfo.  They seem to cover this functionality.
         *
         * port_exit(masterbus, ev->data.addr.client, ev->data.addr.port);
         */

    case SND_SEQ_EVENT_PORT_CHANGE:
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
        return false;
        break;

    default:
        break;
    }

    const size_t buffersize { 256 };            /* 12 enough but for SysEx  */
    midi::bytes buff(buffersize);               /* pre-allocate the data    */
    ::snd_midi_event_t * mididev;               /* make ALSA MIDI parser    */
    int rc { ::snd_midi_event_new(buffersize, &mididev) };
    if (rc < 0)                                 /* || is_nullptr(mididev)   */
    {
        error_print("snd_midi_event_new()", "failed");
        return false;
    }

    /*
     *  Note that ev->time.tick is always 0.  (Same in Seq32).  Not sure about
     *  this handling of SysEx data. Apparently one can get only up to ALSA
     *  buffer size (4096) of data.  Also, the snd_seq_event_input() function
     *  is said to block!
     */

    long bytecount
    {
        ::snd_midi_event_decode(mididev, buff.data(), long(buffersize), ev)
    };
    if (bytecount > 0)
    {
        result = inev->set_midi_event(ev->time.tick, buff, bytecount);
        if (result)
        {
            if (has_master())
            {
                int b
                {
                    master_bus()->get_port_id       /* i.e. bus index   */
                    (
                        midi::port::io::input,
                        int(ev->source.client), int(ev->source.port)
                    )
                };
                inev->set_input_bus(midi::bussbyte(b));
            }
            bool sysex { inev->is_sysex() };
#if defined PLATFORM_DEBUG_TMI
            warnprintf("Input on buss %d\n", int(b));
#endif
            while (sysex)           /* sysex might be more than one message */
            {
                remcount = ::snd_seq_event_input(client, &ev);
                bytecount = ::snd_midi_event_decode
                (
                    mididev, buff.data(), long(buffersize), ev
                );
                if (bytecount > 0)
                {
                    sysex = inev->append_sysex(buff, bytecount);
                    if (remcount == 0)
                        sysex = false;
                }
                else
                    sysex = false;
            }
        }
        result = true;
    }
    else
    {
        /*
         * This happens even at startup, before anything is really happening.
         */

        result = false;
    }
    ::snd_midi_event_free(mididev);
    return result;
}

/**
 *  A simpler version of get_midi_event() that merely puts the incoming event
 *  onto the input queue. Not true. It overrides midi_api::get_message()
 *  which either calss the input callback or gets a message from the front
 *  of the input queue.
 *
 * snd_seq_event_t::type
 *
 *      SND_SEQ_EVENT_CLIENT_START = 60     (/usr/include/alsa/seq_event.h)
 *      SND_SEQ_EVENT_CLIENT_EXIT
 *      SND_SEQ_EVENT_CLIENT_CHANGE
 *      SND_SEQ_EVENT_PORT_START
 *      SND_SEQ_EVENT_PORT_EXIT
 *      SND_SEQ_EVENT_PORT_CHANGE
 *      SND_SEQ_EVENT_PORT_SUBSCRIBED
 *      SND_SEQ_EVENT_PORT_UNSUBSCRIBED
 *
 * snd_seq_tick_time_t snd_seq_event_t::tick
 * snd_seq_tick_time_t is an unsigned int.
 *
 * snd_seq_event_data_t union:
 *
 *      snd_seq_ev_note_t note
 *      snd_seq_ev_ctrl_t control
 *      snd_seq_ev_raw8_t  raw8
 *      snd_seq_ev_raw32_t  raw32
 *      snd_seq_ev_ext_t  ext
 *      snd_seq_ev_queue_control_t  queue
 *      snd_seq_timestamp_t  time
 *      snd_seq_addr_t  addr
 *      snd_seq_connect_t  connect
 *      snd_seq_result_t  result
 *
 * \return
 *      Returns the retrieved message, or an empty message.  Note that this
 *      function does not work if the input thread is getting events.
 */

midi::message
midi_alsa::get_message ()
{
    midi::message result;
    ::snd_seq_t * ncclient { const_cast<::snd_seq_t *>(client_handle()) };
    ::snd_seq_event_t * ev;
    int remcount { ::snd_seq_event_input(ncclient, &ev) };
    if (remcount < 0 || is_nullptr(ev))
    {
        if (remcount == -EAGAIN)
        {
            // no input in non-blocking mode
        }
        else if (remcount == -ENOSPC)
            error_print("get_midi_event()", "input overrun");
        else
            error_print("get_midi_event()", "input error");

        return false;
    }

    /*
     * SND_SEQ_EVENT_xxx codes stripped. We might need to add another
     * code or two to the midi::message.
     */

    const size_t buffersize { 256 };            /* 12 enough but for SysEx  */
    midi::bytes buff(buffersize);               /* pre-allocate the data    */
    ::snd_midi_event_t * mididev;               /* make ALSA MIDI parser    */
    int rc { ::snd_midi_event_new(buffersize, &mididev) };
    if (rc < 0)                                 /* || is_nullptr(mididev)   */
    {
        error_print("snd_midi_event_new()", "failed");
        return false;
    }

    long bytecount
    {
        ::snd_midi_event_decode(mididev, buff.data(), long(buffersize), ev)
    };
    if (bytecount > 0)
    {
        buff.resize(size_t(bytecount));
        midi::message msg(buff);
        msg.jack_stamp(double(ev->time.tick));

        int b { int(midi::null_buss()) };
        if (has_master())
        {
            b = master_bus()->get_port_id
            (
                midi::port::io::input,
                int(ev->source.client), int(ev->source.port)
            );
        }

        bool sysex { msg.is_sysex() };
        msg.midi_buss(b);
        while (sysex)           /* sysex might be more than one message */
        {
            remcount = ::snd_seq_event_input(ncclient, &ev);
            bytecount = ::snd_midi_event_decode
            (
                mididev, buff.data(), long(buffersize), ev
            );
            if (bytecount > 0)
            {
                sysex = msg.append_sysex(buff, bytecount);
                if (remcount == 0)
                    sysex = false;
            }
            else
                sysex = false;
        }
        result = msg;           /* inefficient? */
    }
    ::snd_midi_event_free(mididev);
    return result;
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

#if defined USE_BROKEN_SEND_EVENT

/*
 * Not sure why this does not work. As a workaround we will call
 * send_message() using the byte pointer and size values stored
 * in the midi::event. See the "#else" clause.
 */

bool
midi_alsa::send_event (const midi::event * evp, midi::byte channel) const
{
    midi_alsa_data & mad_data { const_cast<midi_alsa_data &>(alsa_data()) };
    const midi::message & msg { evp->get_message() };
    size_t sz { msg.size() };
    ::snd_midi_event_t * mididev;                       /* MIDI parser      */
    int rc { ::snd_midi_event_new(sz, &mididev) };
    if (rc == 0)
    {
        midi::bytes buff(sz);                           /* temp data        */
        if (channel != midi::null_channel())
        {
            if (sz >= 3)
            {
                buff[0] = evp->get_status(channel);     /* status+channel   */
                evp->get_data(buff[1], buff[2]);        /* set the data     */
            }
            else if (sz == 2)
            {
                buff[0] = evp->get_status(channel);     /* status+channel   */
                evp->get_data(buff[1]);
            }
        }

        ::snd_seq_event_t ev;                           /* event memory     */
        ::snd_seq_ev_clear(&ev);                        /* clear event      */
        snd_seq_ev_set_source(&ev, mad_data.vport());   /* set source       */
        snd_seq_ev_set_subs(&ev);                       /* subscriber       */
        snd_seq_ev_set_direct(&ev);                     /* immediate        */

        long bytecount
        {
            ::snd_midi_event_encode(mididev, buff.data(), sz, &ev)
        };
        if (bytecount >= 0)
        {
            snd_seq_event_output(mad_data.alsa_client(), &ev); /* pump to q */
        }
        else
        {
            warning("send_event(): parsing error");
            ::snd_midi_event_free(mididev);             /* free parser      */
            return false;
        }
        ::snd_midi_event_free(mididev);                 /* free parser      */
        return true;
    }
    else
        error(rterror::kind::driver_error, "send_event(): out of memory");

    return true;
}

#else

bool
midi_alsa::send_event (const midi::event * evp, midi::byte channel) const
{
    const midi::message & msg { evp->get_message() };
    midi::bytes & buff { const_cast<midi::bytes &>(msg.event_bytes()) };
    size_t sz { msg.size() };
    if (channel != midi::null_channel())
    {
        if (sz >= 3)
        {
            buff[0] = evp->get_status(channel);     /* status+channel   */
            evp->get_data(buff[1], buff[2]);        /* set the data     */
        }
        else if (sz == 2)
        {
            buff[0] = evp->get_status(channel);     /* status+channel   */
            evp->get_data(buff[1]);
        }
    }
    return send_message(buff.data(), sz);
}

#endif  // defined USE_BROKEN_SEND_EVENT

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
        midi_alsa_data & data { alsa_data() };
        ::snd_seq_remove_events_t * remove_events;
        snd_seq_remove_events_malloc(&remove_events);
        ::snd_seq_remove_events_set_condition
        (
            remove_events, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH |
                SND_SEQ_REMOVE_IGNORE_OFF
        );
        ::snd_seq_remove_events_set_tag(remove_events, tag);
        ::snd_seq_remove_events(data.alsa_client(), remove_events);
        ::snd_seq_remove_events_free(remove_events);
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

