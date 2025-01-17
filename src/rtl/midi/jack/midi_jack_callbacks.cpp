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
 * \file          midi_jack_callbacks.cpp
 *
 *    Implements the JACK MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-06-09
 * \license       See above.
 *
 *  The JACK callbacks have been moved into a separate file for better
 *  maintainability, as a number of additional callbacks have been added.
 */

#include "rtl/midi/jack/midi_jack.hpp"  /* rtl::midi_jack class             */

#if defined RTL66_BUILD_JACK

#include <cstring>                      /* std::memcpy()                    */
#include <sstream>                      /* std::ostringstream               */
#include <jack/midiport.h>
#include <jack/metadata.h>              /* JACK_METADATA_ICON names         */
#include <jack/uuid.h>                  /* JACK_UUID_EMPTY_INITIALIZER etc. */

#include "rtl66-config.h"               /* RTL66_HAVE_JACK_PORT_RENAME      */
#include "midi/eventcodes.hpp"          /* midi::status enum, functions...  */
#include "midi/calculations.hpp"        /* midi::extract_port_names()       */
#include "midi/ports.hpp"               /* ::midi:ports class               */
#include "rtl/midi/rtmidi_in_data.hpp"  /* rtl::rtmidi_in_data class        */
#include "util/msgfunctions.hpp"        /* util::async_safe_strprint() etc. */
#include "transport/jack/transport.hpp" /* transport::jack::transport class  */
#include "xpc/ring_buffer.hpp"          /* xpc::ring_buffer<> template      */

namespace rtl
{

/**
 *  This item exists in the JACK 2 source code, but not in the installed JACK
 *  headers on our development system.
 */

const char * JACK_METADATA_ICON_NAME =
    "http://jackaudio.org/metadata/icon-name";

/*
 ----------------------------------------------------------------------------
  Static items
 ----------------------------------------------------------------------------
 */

/**
 *  The buffer size for basic MIDI messages.  Probably excludes SysEx
 *  messages.
 */

static const size_t s_message_buffer_size = 256;

/**
 *  Checks a frame offset for validity.
 */

inline bool
valid_frame_offset (jack_nframes_t offset)
{
    return offset != UINT32_MAX;
}

/*------------------------------------------------------------------------
 * JACK callback setup functions (used in midi_jack)
 *------------------------------------------------------------------------*/

bool
jack_set_process_cb
(
    jack_client_t * c,
    JackProcessCallback cb,
    void * apidata
)
{
    int rc = ::jack_set_process_callback(c, cb, apidata);
    bool result = rc == 0;
    if (! result)
    {
        error_print("jack_set_process_callback", "failed");
    }
    return result;
}

bool
jack_set_shutdown_cb
(
    jack_client_t * c,
    JackShutdownCallback cb,
    void * self                     /* i.e. the "this" pointer  */
)
{
    bool result = not_nullptr_2(c, cb);
    (void) ::jack_on_shutdown(c, cb, self);
    return result;

#if 0
    int rc = ::jack_on_shutdown(c, cb, self);
    if (rc != 0)
    {
        error_print("jack_set_shutdown_callback", "failed");
        result = false;
    }
    return result;
#endif
}

bool
jack_set_port_connect_cb
(
    jack_client_t * c,
    JackPortConnectCallback cb,
    void * self                     /* i.e. the "this" pointer  */
)
{
    int rc = ::jack_set_port_connect_callback(c, cb, self);
    bool result = rc == 0;
    if (! result)
    {
        error_print("jack_set_port_connect_callback", "failed");
    }
    return result;
}

bool
jack_set_port_registration_cb
(
    jack_client_t * c,
    JackPortRegistrationCallback cb,
    void * self                     /* i.e. the "this" pointer  */
)
{
    int rc = ::jack_set_port_registration_callback(c, cb, self);
    bool result = rc == 0;
    if (! result)
    {
        error_print("jack_set_port_registrationn_callback", "failed");
    }
    return result;
}

/*------------------------------------------------------------------------
 * JACK metadata setup
 *------------------------------------------------------------------------*/

static const std::string qseq66_32x32
{
"iVBORw0KGgoAAAANSUhEUgAAACAAAAAgAQMAAABJtOi3AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH5gEDDCAXPFGMYAAAAAZQTFRFAAAA////pdmf3QAAAENJREFUCNdj+A8EDGCiwYCFEYno//inEIloMLBBJv4Z/kEm0PQCjXvM8ICRgRlE8EOJ/+z/oQRCDEw8ZIcRqBIgVwEAyEs6odvCCJwAAAAASUVORK5CYII="
};

static const std::string qseq66_128x128
{
"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"
};

/**
 *  A free function in the Seq66 namespace
 *
 *  This works to get back the correct client name:
 *
 *      char * name = jack_get_client_name_by_uuid(jc, luuid);
 */

static std::string
get_jack_client_uuid (jack_client_t * jc)
{
    std::string result;
#if defined RTL66_JACK_SESSION          /* deprecated, use Non Session Mgr. */
    char * luuid = ::jack_client_get_uuid(jc);
    if (not_nullptr(luuid))
    {
        result = luuid;                 /* see note in the banner           */
        jack_free(luuid);
    }
#else
    char * lname = ::jack_get_client_name(jc);
    if (not_nullptr(lname))
    {
        char * luuid = ::jack_get_uuid_for_client_name(jc, lname);
        if (not_nullptr(luuid))
        {
            result = luuid;

            /*
             * Apparently not needed here: jack_free(luuid);
             */
        }
    }
#endif
    return result;
}

/**
 *  Need type to be "image/png;" in some of these calls.
 *
 * \param key
 *      Provides the name of this property, as a URI string.
 *
 * \param value
 *      Provides the value of the property, a null-terminated string.
 *
 * \param type
 *      Provides the type of data as an empty string (implies UTF-8
 *      text/plain), a MIME type such as "image/png;base64" (image) or a URI
 *      such as http://www.w3.org/2001/XMLSchema#int (integer).
 */

static bool
set_jack_client_property
(
    jack_client_t * jc,
    const std::string & key,
    const std::string & value,
    const std::string & type
)
{
    std::string uuid = get_jack_client_uuid(jc);
    bool result = ! uuid.empty();
    if (result)
    {
        jack_uuid_t u2 = JACK_UUID_EMPTY_INITIALIZER;
        int rc = ::jack_uuid_parse(uuid.c_str(), &u2);
        result = rc == 0;
        if (result)
        {
            const char * k = key.c_str();
            const char * v = value.c_str();
            const char * t = type.c_str();
            rc = ::jack_set_property(jc, u2, k, v, t);
            result = rc == 0;
        }
    }
    return result;
}

#if  0

/*
 * Not yet used.
 */

static bool
set_jack_port_property
(
    jack_client_t * jc,
    jack_port_t * jp,
    const std::string & key,
    const std::string & value,
    const std::string & type
)
{
    jack_uuid_t uuid = ::jack_port_uuid(jp);
    const char * k = key.c_str();
    const char * v = value.c_str();
    const char * t = type.empty() ? NULL : type.c_str() ;   /* important!   */
    int rc = ::jack_set_property(jc, uuid, k, v, t);
    return rc == 0;
}

/**
 *  This version does not seem to work.
 */

static bool
set_jack_port_property
(
    jack_client_t * jc,
    const std::string & portname,
    const std::string & key,
    const std::string & value,
    const std::string & type
)
{
    jack_port_t * jp = ::jack_port_by_name(jc, portname.c_str());
    jack_uuid_t uuid = ::jack_port_uuid(jp);
    const char * k = key.c_str();
    const char * v = value.c_str();
    const char * t = type.empty() ? NULL : type.c_str() ;   /* important!   */
    int rc = ::jack_set_property(jc, uuid, k, v, t);    // use t == NULL?
    return rc == 0;
}

#endif

bool
jack_set_meta_data
(
    jack_client_t * c,
    const std::string & n               /* application icon name    */
)
{
    bool result = set_jack_client_property
    (
        c, JACK_METADATA_ICON_NAME, n, "image/png;base64"
    );
    if (result)
    {
        debug_print("Set 32x32 icon", n);
        result = set_jack_client_property
        (
            c, JACK_METADATA_ICON_SMALL, qseq66_32x32, "image/png;base64"
        );
        if (! result)
            error_print("Set 32x32 icon", "failed");
    }
    else
        error_print("Set client icon", "failed");

    if (result)
    {
        debug_print("Set 128x128 icon", n);
        result = set_jack_client_property
        (
            c, JACK_METADATA_ICON_LARGE,
            qseq66_128x128, "image/png;base64"
        );
        if (! result)
            error_print("Set 128x128 icon", "failed");
    }
    return result;
}

/*------------------------------------------------------------------------
 * JACK callbacks
 *------------------------------------------------------------------------*/

/**
 * Jack input process callback.
 *
 *  The jack_port_get_buffer() function returns a pointer to the memory area
 *  associated with the specified port. For an output port, it will be a memory
 *  area that can be written to; for an input port, it will be an area containing
 *  the data from the port's connection(s), or zero-filled. if there are multiple
 *  inbound connections, the data will be mixed appropriately.
 *
 *  Do not cache the returned address across process() callbacks. Port buffers
 *  have to be retrieved in each callback for proper functionning.
 */

int
jack_process_in (jack_nframes_t framect, void * arg)
{
    midi_jack_data * jackdata = midi_jack::static_data_cast(arg);
    rtmidi_in_data * rtdata = jackdata->rt_midi_in();
    if (is_nullptr(jackdata->jack_port()))   /* is port not yet created?         */
        return 0;

    void * buff = ::jack_port_get_buffer(jackdata->jack_port(), framect);
    bool allowsysex = rtdata->allow_sysex();
    bool moresysex = rtdata->continue_sysex();
    int evcount = ::jack_midi_get_event_count(buff);
    for (int j = 0; j < evcount; ++j)           /* MIDI events in buffer    */
    {
        midi::message & message = rtdata->message();
        jack_midi_event_t event;
        int rc = ::jack_midi_event_get(&event, buff, j);
        if (rc == ENODATA)
        {
            util::async_safe_errprint("jack_process_in() no data");
            return 0;
        }
        else if (rc == ENOBUFS)
        {
            util::async_safe_errprint("jack_process_in() no buffers");
            return 0;
        }

        jack_time_t jtime = ::jack_get_time();  /* compute the delta time   */
        jack_time_t delta_jtime;                /* uint64_t time in usec    */
        if (rtdata->first_message())
        {
            rtdata->first_message(false);
            delta_jtime = 0;
            message.jack_stamp(0.0);
        }
        else
        {
            jtime -= jackdata->jack_lasttime();
            delta_jtime = jack_time_t(jtime * 0.000001);    /* microsecs!!! */
            message.jack_stamp(delta_jtime);                 /* Seq66 #100   */
        }

        jackdata->jack_lasttime(jtime);
        if (! moresysex)
            message.clear();

        bool issysex = (moresysex || midi::is_sysex_msg(event.buffer[0])) &&
            allowsysex;

        if (! issysex)
        {
            /*
             * Unless this is a (possibly continued) SysEx message and we're
             * ignoring SysEx, copy the event buffer into the MIDI message
             * struct.
             */

            for (unsigned i = 0; i < event.size; ++i)
                message.push(event.buffer[i]);
        }
        midi::status ebs = midi::to_status(event.buffer[0]);
        switch (ebs)
        {
        case midi::status::sysex:         // 0xF0 Start of a SysEx message

            moresysex = ! midi::is_sysex_end_msg(event.buffer[event.size-1]);
            rtdata->continue_sysex(moresysex);
            if (! allowsysex)
                continue;
            break;

        case midi::status::quarter_frame: // 0xF1 MIDI Time Code
        case midi::status::clk_clock:     // 0xF8 Timing Clock message

            if (! rtdata->allow_time_code())
                continue;
            break;

        case midi::status::active_sense:  // 0xFE Active Sensing message

            if (! rtdata->allow_active_sensing())
                continue;
            break;

        default:                            // All other MIDI messages

            if (moresysex)
            {
                moresysex = ! midi::is_sysex_end_msg(event.buffer[event.size-1]);
                rtdata->continue_sysex(moresysex);
                if (allowsysex)
                    continue;
            }
            break;
        }
        if (! moresysex)
        {
            /*
             * If not a continuation of a SysEx message, invoke the user
             * callback function or queue the message.
             */

            if (rtdata->using_callback())
            {
                rtmidi_in_data::callback_t cb = rtdata->user_callback();
                cb(message.jack_stamp(), &message, rtdata->user_data());
            }
            else
            {
                /*
                 * As long as we haven't reached our queue size limit, push the
                 * message.
                 */

                if (! rtdata->queue().push(message))
                {
                    util::async_safe_errprint
                    (
                        "jack_process_in() message overflow"
                    );
                }
            }
        }
    }
    return 0;
}

/**
 *  Handles peeking and reading data from our replacement for the JACK
 *  ringbuffer.
 *
 * \param jackdata
 *      The source of JACK information for this port.
 *
 * \param framect
 *      The size of the JACK buffer in "samples".  Needed to calculate the
 *      offset, it also provides the limit for the offset value.
 *
 * \param lastvalue
 *      Provides the last frame offset.
 *
 * \param [out] dest
 *      The bytes from the ringbuffer that are to be written to MIDI are
 *      stored here.
 *
 * \param [inout] destsz
 *      Provides the size of the destination buffer.  It is modified to return
 *      the number of bytes actually written.
 *
 * \return
 *      Returns the calculated frame offset.  If UINT32_MAX, then there is
 *      no data, or the frame offset of the event is earlier than the last
 *      offset, and so the data are not read, put off to the next process
 *      callback cycle.
 */

static jack_nframes_t
jack_get_event_data
(
    midi_jack_data * jackdata,
    jack_nframes_t framect,
    jack_nframes_t cycle_start,
    jack_nframes_t & lastvalue,
    char * dest, size_t & destsz
)
{
    jack_nframes_t result = UINT32_MAX;
    xpc::ring_buffer<midi::message> * buffmsg = jackdata->jack_buffer();
    int count = int(buffmsg->read_space());
    bool process = count > 0;
    if (process)
    {
        static bool s_use_offset = midi_jack_data::use_offset();
        const midi::message & msg = buffmsg->front();
        midi::pulse ts = msg.jack_stamp();
        if (s_use_offset)
        {
#if defined USE_FULL_TTYMIDI_METHOD // handling "lastvalue" doesn't seem to help
            jack_nframes_t frame = midi_jack_data::frame_estimate(ts);
            frame += framect - midi_jack_data::size_compensation();
            if (lastvalue > frame)
                frame = lastvalue;
            else
                lastvalue = frame;

            if (frame > cycle_start)
            {
                result = frame - cycle_start;
                if (result >= framect)
                    result = framect - 1;
            }
            else
                result = 0;
#else
            (void) lastvalue;
            result = midi_jack_data::frame_offset(cycle_start, framect, ts);
#endif
        }
        else
            result = 0;

        if (process)                /* belaying not enabled at this time    */
        {
            size_t datasz = size_t(msg.event_byte_count());
            if (datasz <= destsz)
            {
                std::memcpy(dest, msg.data_ptr(), datasz);
                destsz = datasz;
            }
            buffmsg->pop_front();
        }
        else
        {
#if defined PLATFORM_DEBUG_TMI
            char value[util::c_async_safe_utoa_size];
            char text[util::c_async_safe_utoa_size + 32];
            std::strcpy(text, "Event ");
            util::async_safe_utoa(value, msg.msg_number());
            std::strcat(text, value);
            std::strcat(text, " belayed");
            util::async_safe_errprint(text);
#endif
            result = UINT32_MAX;
        }
    }
    return result;
}

/**
 *  Jack output process callback.
 *
 *  Defines the JACK output process callback for a MIDI output port (a
 *  midi_out_jack object associated with, for example,
 *  "system:midi_playback_1", representing, for example, a Korg nanoKEY2 to
 *  which we can send information), also known as a "Writable Client" by
 *  qjackctl.  Here's how it works:
 *
 *      -#  Get the JACK port buffer, for our local jack port.  Clear it.
 *      -#  Loop while the number of bytes available for reading [via
 *          jack_ringbuffer_read_space()] is non-zero.  Note that the second
 *          parameter is where the data is copied.
 *          NOTE: This byte buffer has been replaced by a midi::message
 *          ring-buffer.
 *      -#  Get the size of each event, and allocate space for an event to be
 *          written to an event port buffer (JACK reserve/write functions).
 *      -#  Read the data from the ringbuffer into this port buffer.  JACK
 *          will then send it to the remote port.
 *
 *  Since this is an output port, "buff" is the area to which we can write
 *  data, to send it to the "remote" (i.e. outside our application) port.  The
 *  data is written to the ringbuffer in api_init_out(), and here we read the
 *  ring buffer and pass it to the output buffer.
 *
 *  We were wondering if, like the JACK midiseq example program, we need to
 *  wrap the out-process in a for-loop over the number of frames.  In our
 *  tests, we are getting 1024 frames, and the code seems to work without that
 *  loop.  A for-loop over the number of frames?  See discussion above.
 *
 *  Could consider using JACK callbacks to detect server-setting changes.
 *  That would be more robust.
 *
 * \param framect
 *    The number of frames to be processed.
 *
 * \param arg
 *    A pointer to the midi_jack_data object, which holds basic information
 *    about the JACK client.
 *
 * \return
 *    Returns 0.
 */

int
jack_process_out (jack_nframes_t framect, void * arg)
{
    midi_jack_data * jackdata = midi_jack::static_data_cast(arg);
    jack_port_t * jackport = jackdata->jack_port();
    if (not_nullptr(jackport))
    {
        char mbuffer[s_message_buffer_size];
        char * mbuf = &mbuffer[0];
        const jack_nframes_t cycle_start =
            ::jack_last_frame_time(jackdata->jack_client());

        /*
         * Seq66's version might need to be FIXED!
         */

        jack_nframes_t lastvalue = 0;
        void * buff = ::jack_port_get_buffer(jackdata->jack_port(), framect);
        jack_position_t pos =
            transport::jack::transport::get_jack_parameters().position;

        if (midi_jack_data::recalculate_frame_factor(pos, framect))
            util::async_safe_errprint("JACK settings changed");

        ::jack_midi_clear_buffer(buff);
        for (;;)
        {
            size_t destsz = s_message_buffer_size;
            jack_nframes_t offset = jack_get_event_data
            (
                jackdata, framect, cycle_start, lastvalue, mbuf, destsz
            );
            if (destsz > 0 && valid_frame_offset(offset))
            {
                const jack_midi_data_t * data =
                    reinterpret_cast<const jack_midi_data_t *>(mbuf);

                int rc = ::jack_midi_event_write(buff, offset, data, destsz);
                if (rc != 0)
                {
                    util::async_safe_errprint("JACK MIDI write error");
                    break;
                }
                lastvalue = offset;            /* tricky code */
            }
            else
                break;
        }
#if RTL66_HAVE_SEMAPHORE_H
        (void) jackdata->semaphore_wait_and_post();
#endif
    }
    return 0;
}

/**
 *  Provides a JACK callback function that uses the callbacks defined in the
 *  midi_jack module.  This function calls both the input callback and
 *  the output callback, depending on the port type.  This may lead to
 *  delays, depending on the size of the JACK MIDI buffer.
 *
 * \param framect
 *      The frame number from the JACK API.
 *
 * \param arg
 *      The putative pointer to the transport::jack::info structure.
 *
 * \return
 *      Always returns 0.
 */

int
jack_process_io (jack_nframes_t framect, void * /*arg*/)
{
    if (framect > 0)
    {
#if 0
        transport::jack::info * self = reinterpret_cast<transport::jack::info *>(arg);
        if (not_nullptr(self))
        {
            /*
             * Go through the I/O ports and route the data appropriately.
             */

            for (auto mj : self->jack_ports())  /* midi_jack pointers       */
            {
                if (mj->enabled())
                {
#if defined PLATFORM_DEBUG_TMI                  /* printf() asynch unsafe   */
                    if (mj->is_input_port())
                        printf("Enabled: %s\n", mj->port_name().c_str());
#endif
                    midi_jack_data * mjp = &mj->jack_data();
                    if (mj->parent_bus().is_input_port())
                        (void) jack_process_in(framect, mjp);
                    else
                        (void) jack_process_out(framect, mjp);
                }
#if defined PLATFORM_DEBUG_TMI                  /* printf() asynch unsafe   */
                else
                {
                    if (mj->is_input_port())
                        printf("Disabled: %s\n", mj->port_name().c_str());
                }
#endif
            }
        }
#endif
    }
    return 0;
}

#if defined RTL66_JACK_PORT_REFRESH_CALLBACK

/**
 *  Provides a callback for registering a port, so that we can detect when a
 *  port appears or disappears in the system.  We use jack_port_by_id() to
 *  get the port, then jack_port_is_mine() so that we can avoid processing our
 *  own port.
 *
 *  The original port handle is set via the port_handle() setter called in the
 *  member function register_port(). If this port_handle can be found in the
 *  list of ports, that should mean that it is a Seq66 port, and nothing should
 *  be done.  Otherwise, the I/O port containers should be updated.
 *
 *  Also see the api_get_port_name() function and the midi_jack_data class.
 *
 * Notes:
 *
 *  -   The documentation for jack_set_port_registration_callback() says that
 *      all notifications are received in a separate non-real-time thread, so
 *      the code doesn't need to be suitable for real-time execution. What about
 *      re-entrant? Async-safe?  Based on tests, we are pretty sure that
 *      a series of calls don't interfere with each other.
 *  -   For a2j ports, the long and short names are like this:
 *      -   Long: 'a2j:nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *      -   Short: 'nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *  -   For Seq66's connections to a2j ports, the long and short names are
 *      like this:
 *      -   Long: 'seq66:a2j nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *      -   Short: 'a2j nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *  -   FluidSynth (on Ubuntu 20):
 *      -   Long: 'fluidsynth-midi:midi_00'
 *      -   Short: 'midi_00'.
 *
 * Desired activity steps:
 *
 *      -#  Startup.
 *          -#  Enable port-refresh (callback, processing) only if
 *              port-mapping is enabled.
 *          -#  As done now, gather the input and output ports into these
 *              containers (midi_port_info).
 *          -#  Also make copies of these containers.
 *      -#  MORE
 *
 * Rules for handling port registration and unregistration:
 *
 *      -#  Port change detection.
 *          -#  Changes should only be detected in non-Seq66 ("not mine") ports.
 *          -#  This occurs when registration or un-registration causes the
 *              callback to occur.
 *      -#  No special processing unless the user has enabled port-mapping.
 *          Otherwise, Seq66 port numbers, which are simply indexes into the
 *          port vectors, would change, breaking the song.
 *      -#  Ports are never removed from the port map.
 *          -#  Unregistered ports are simply disabled.
 *          -#  New ports are appended to the port map.
 *          -#  Ports accumulate.  The user must (currently) edit the 'rc' file
 *              to remove ports from the port-map, and restart the application.
 *      -#  Port removal.
 *          -#  Detection. Examine the active ports and the port map.
 *              -#  If the port is not in the active port list, do nothing.
 *              -#  If the port is not in the port map, do nothing.
 *          -#  Disable or de-initialize the port in:
 *              -#  In transport::jack::info/midi_info.
 *              -#  The port map.
 *              -#  The midi::masterbus. ???
 *              -#  The busarray.
 *      -#  Port addition.
 *          -#  Detection. Examine the active ports and the port map.
 *              -#  If the port is in the active port list, do nothing.
 *              -#  If the port is in the port map, enable it.
 *          -#  If the port is new:
 *              -#  Append it to the portmap.
 *              -#  Append it to transport::jack::info/midi_info.
 *              -#  Add it to the mastermidis. ???
 *              -#  Add it to the busarray and initialize it.
 *      -#  User interface.  It must be updated to reflect the new port map.
 *          -#  Edit / Preferences MIDI Clock and MIDI Input.
 *          -#  Main window port list.
 *          #   The pattern editor(s) (if output).
 *
 * \param portid
 *      The ID of the port. It is a 32-bit unsigned integer.
 *
 * \param regv
 *      The registration value. It is non-zero if the port is being registered,
 *      and zero if the port is being unregistered.
 *
 * \param arg
 *      Pointer to a client supplied data, the transport::jack::info object.
 */

void
jack_port_register_callback (jack_port_id_t portid, int regv, void * arg)
{
    transport::jack::info * jackinfo =
        reinterpret_cast<transport::jack::info *>(arg);

    if (not_nullptr(jackinfo))
    {
        jack_client_t * handle = jackinfo->client_handle();
        jack_port_t * portptr = nullptr;
        if (not_nullptr(handle))
        {
            bool mine = false;
            int flags = 0;
            midibase::io iotype = midibase::io::indeterminate;
            std::string longname;
            std::string shortname;
            std::string porttype;
            portptr = ::jack_port_by_id(handle, portid);
            if (not_nullptr(portptr))
            {
                const char * ln = ::jack_port_name(portptr);
                const char * sn = ::jack_port_short_name(portptr);
                if (not_nullptr(ln))
                    longname = std::string(ln);

                if (not_nullptr(sn))
                    shortname = std::string(sn);

                mine = ::jack_port_is_mine(handle, portptr) != 0;
                flags = ::jack_port_flags(portptr);
                porttype = ::jack_port_type(portptr);
                if (flags & JackPortIsInput)
                    iotype = midibase::io::input;
                else if (flags & JackPortIsOutput)
                    iotype = midibase::io::output;
            }
            else
                return;                             /* fatal error, bug out */

            if (rc().investigate())
            {
                /*
                 * JACK docs say this function doesn't need to be suitable for
                 * real-time execution. But what about async-safety? It seems
                 * to be necessary; debug_message() yields intermixed output.
                 */

                const char * iot = "TBD";
                char value[util::c_async_safe_utoa_size];
                char temp[128];
                if (iotype == midibase::io::input)
                    iot = " In";
                else if (iotype == midibase::io::output)
                    iot = "Out";

                util::async_safe_utoa(value, unsigned(portid), false);
                std::strcpy(temp, "Port ");
                std::strcat(temp, value);
                std::strcat(temp, ": ");
                std::strcat(temp, regv != 0 ? "Reg" : "Unreg");
                std::strcat(temp, " ");
                std::strcat(temp, iot);
                std::strcat(temp, " ");
                std::strncat(temp, shortname.c_str(), 30);   /* truncate it  */
                std::strcat(temp, "/ ");
                if (is_nullptr(arg))
                    std::strcat(temp, "nullptr! ");

                std::strcat(temp, porttype.c_str());
                if (mine)
                    std::strcat(temp, " seq66");

                util::async_safe_strprint(temp);
            }
            jackinfo->update_port_list
            (
                mine, int(portid), regv, shortname, longname
            );
        }
    }
}

#endif      // defined RTL66_JACK_PORT_REFRESH_CALLBACK

#if defined RTL66_JACK_PORT_CONNECT_CALLBACK

/**
 *  Port-connect and port-register callbacks seem to get interleaved, making
 *  for confusing output.  We currently want to know only about registration,
 *  anwyay.
 *
 * \param a
 *      One of two ports connected/disconnected.
 *
 * \param b
 *      One of two ports connected/disconnected.
 *
 * \param connect
 *      Non-zero if ports were connected, zero if ports were disconnected.
 *
 * \param arg
 *      Pointer to a client supplied data (a transport::jack::info pointer).
 */

void
jack_port_connect_callback
(
    jack_port_id_t port_a, jack_port_id_t port_b,
    int connect, void * arg
)
{
    transport::jack::info * jack = reinterpret_cast<transport::jack::info *>(arg);
    if (not_nullptr(jack))
    {
        if (rc().investigate())
        {
            char value[util::c_async_safe_utoa_size];
            char temp[2 * util::c_async_safe_utoa_size + 48];
            std::strcpy(temp, "port-connect(");
            util::async_safe_utoa(value, unsigned(port_a));
            std::strcat(temp, value);
            util::async_safe_utoa(value, unsigned(port_b));
            std::strcat(temp, value);
            util::async_safe_utoa(value, unsigned(connect));
            std::strcat(temp, connect != 0 ? " connect" : " disconnect");
            std::strcat(temp, value);
            std::strcat(temp, not_nullptr(arg) ? " arg)" : " nullptr)");
            util::async_safe_strprint(temp);
        }
    }
}

#endif      // defined RTL66_JACK_PORT_CONNECT_CALLBACK

#if defined RTL66_JACK_PORT_SHUTDOWN_CALLBACK

/**
 *  This callback is to shut down JACK by clearing the transport :: jack ::
 *  transport :: m_jack_running flag.
 *
 * \param arg
 *      Points to the transport::jack::transport object in charge of JACK
 *      support for the player object.
 */

void
jack_shutdown_callback (void * arg)
{
    transport::jack::info * jack = reinterpret_cast<transport::jack::info *>(arg);
    if (not_nullptr(jack))
        infoprint("JACK Shutdown");             // async_safe_strprint("...");
    else
        errprint("JACK Shutdown null pointer"); // async_safe_errprint("...");
}

#endif      // defined RTL66_JACK_PORT_SHUTDOWN_CALLBACK

}           // namespace rtl

#endif      // defined RTL66_BUILD_JACK

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

