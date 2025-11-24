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
 * \file          midi_alsa_handler.cpp
 *
 *  The MIDI handler function for ALSA.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-11-17
 * \license       See above.
 *
 *  This module is meant to be #include'd in midi_alsa.cpp. It's been
 *  pulled out in order better encapsulated the code as we try to
 *  fix segfault bugs.
 */

/*------------------------------------------------------------------------
 * ALSA free functions
 *------------------------------------------------------------------------*/

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
    ::snd_seq_real_time_t x,  /* the event time   */
    ::snd_seq_real_time_t y   /* the last time    */
)
{
    if (x.tv_nsec < y.tv_nsec)
    {
        int nsec { int(y.tv_nsec - x.tv_nsec) / 1000000000 + 1 };
        y.tv_nsec -= 1000000000 * nsec;
        y.tv_sec += nsec;
    }
    if (x.tv_nsec - y.tv_nsec > 1000000000)
    {
        int nsec { int(x.tv_nsec - y.tv_nsec) / 1000000000 };
        y.tv_nsec += 1000000000 * nsec;
        y.tv_sec -= nsec;
    }
    return int(x.tv_sec) - int(y.tv_sec) +
        int(x.tv_nsec - y.tv_nsec) * 1E-9;
}

static bool
decode_event
(
    rtmidi_in_data * rtidata,
    midi_alsa_data * mad_data,
    ::snd_seq_event_t * ev
)
{
    bool dodecode = false;
    switch (ev->type )
    {
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
#if defined PLATFORM_DEBUG_TMI
        debug_print("midi_alsa_handler()", "port subscribed");
#endif
        break;

    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
#if defined PLATFORM_DEBUG_TMI
        debug_print("midi_alsa_handler()", "port unsubscribed");
#endif
        break;

    case SND_SEQ_EVENT_QFRAME:                  // MIDI time code
    case SND_SEQ_EVENT_TICK:                    // 0xF9 MIDI timing tick
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

        if (ev->data.ext.len > mad_data->buffer_size())
        {
            size_t nbytes = ev->data.ext.len;
            bool ok = mad_data->reallocate(nbytes);
            if (ok)
            {
                rtidata->do_input(false);
                error_print
                (
                    "midi_alsa_handler()", "error resizing buffer"
                );
                break;
            }
        }
        dodecode = true;
        break;

    default:
        dodecode = true;
        break;
    }
    return dodecode;
}

/*------------------------------------------------------------------------
 * ALSA callbacks
 *------------------------------------------------------------------------*/

/**
 *  This function provides a thread function, set up via
 *  midi_alsa::start_input_thread(rtmidi_in_data &).
 *
 *  The ALSA sequencer has a maximum buffer size for MIDI sysex
 *  events of 256 bytes. If a device sends sysex messages larger
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
 *           Lopez-Cabanillas.
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
 *      -#  Every API has a data structure passed to the pthread_create()
 *          function.
 *          -#  JACK: jack_data() [midi_jack_data]
 *          -#  ALSA: alsa_data() [midi_alsa_data]
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
 *
 * \param ptr
 *      Provides a void pointer to the midi_alsa_data structure for this
 *      port.
 */

void *
midi_alsa_handler (void * ptr)
{
    rtmidi_in_data * rtidata { midi_api::static_in_data_cast(ptr) };
    midi_alsa_data * mad_data
    {
        midi_alsa::static_data_cast(rtidata->api_data())
    };
    ::snd_seq_t * client { mad_data->alsa_client() };
    if (rtidata->queue().unallocated())
    {
        error_print("midi_alsa_handler()", "queue unallocated");
        return nullptr;
    }

    /*
     * Why 0? That's the buffer size. RtMidi does this, too. Makes no sense.
     * Undocumented behavior?
     *
     * bool success = mad_data->init_event_parser(c_event_size_max);
     *  ::snd_midi_event_new(c_event_size_max, mad_data->event_parser_address())
     */

    bool ok { mad_data->init_event_parser() };      /* see midi_alsa_data   */
    if (! ok)
    {
        rtidata->do_input(false);
        error_print("midi_alsa_handler()", "new event parser failed");
        return nullptr;
    }
    ok = mad_data->reallocate();
    if (! ok)
    {
        rtidata->do_input(false);
        return nullptr;
    }

    bool moresysex { false };
    midi::message mmsg;
    pollwrapper pwrap(client, 1);
    if (! pwrap.set_trigger_fd(mad_data->trigger_fd(0)))
        return nullptr;

    while (rtidata->do_input())
    {
        int count { ::snd_seq_event_input_pending(client, 1) };
        if (count == 0)                                 /* no data pending  */
        {
            (void) pwrap.poll_file_descriptor();
            continue;                                   /* no MIDI data     */
        }

        /*
         * Seqfaults can occur here when two threads call midi_alsa_handler().
         */

        ::snd_seq_event_t * ev;
        int rc { ::snd_seq_event_input(client, &ev) };  /* retrieve event   */
        if (rc == -ENOSPC)
        {
            error_print("midi_alsa_handler()", "input overrun");
            continue;
        }
        else if (rc <= 0)
        {
            error_print("midi_alsa_handler()", "input error");
            perror("   ");
            continue;
        }

        /*
         * This is a bit weird, but we now have to decode an ALSA MIDI
         * event (back) into MIDI bytes. We ignore non-MIDI types.
         */

        if (! moresysex)
            mmsg.clear();

        bool dodecode { decode_event(rtidata, mad_data, ev) };
        if (dodecode)
        {
            midi::byte * buff { mad_data->buffer() };
            long nbytes
            {
                ::snd_midi_event_decode
                (
                    mad_data->event_parser(), buff,
                    mad_data->buffer_size(), ev
                )
            };
            if (nbytes > 0)                 // see banner
            {
                if (! moresysex)
                    mmsg.assign(buff, &buff[nbytes]);
                else
                    mmsg.append(buff, &buff[nbytes]);

                moresysex = (ev->type == SND_SEQ_EVENT_SYSEX) &&
                    ! midi::is_sysex_end_msg(mmsg.back());     // 0xF7

                if (! moresysex)
                {
                    /*
                     * Calculate the time difference.  See the banner.
                     */

                    double time = calculate_time
                    (
                        ev->time.time, mad_data->last_time()
                    );
                    mad_data->last_time(ev->time.time);
                    if (rtidata->first_message())
                    {
                        rtidata->first_message(false);
                        mmsg.jack_stamp(0.0);
                    }
                    else
                        mmsg.jack_stamp(time);
                }
                else
                {
#if defined PLATFORM_DEBUG
                    error_print("midi_alsa_handler()", "parse error");
#endif
                }
            }
        }
        ::snd_seq_free_event(ev);
        if (mmsg.empty() || moresysex)
            continue;

        if (rtidata->using_callback())
        {
printf("CALLBACK\n");
            rtmidi_in_data::callback_t cb = rtidata->user_callback();
            cb(mmsg.jack_stamp(), mmsg, rtidata->user_data());
        }
        else
        {
printf("PUSH\n");
            if (! rtidata->queue().push(mmsg))
                error_print("midi_alsa_handler()", "input queue limit hit");
        }
    }
    mad_data->handler_cleanup();
    return nullptr;
}

/*
 * midi_alsa_handler.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
