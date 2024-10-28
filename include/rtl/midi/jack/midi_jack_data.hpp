#if ! defined RTL66_RTL_MIDI_JACK_DATA_HPP
#define RTL66_RTL_MIDI_JACK_DATA_HPP

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
 * \file          midi_jack_data.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2017-01-02
 * \updates       2023-07-20
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "rtl66-config.h"               /* RTL66_HAVE_SEMAPHORE_H           */

#include <jack/jack.h>

#if RTL66_HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#include "midi/midibytes.hpp"           /* midi::byte, other aliases        */
#include "midi/message.hpp"             /* midi::message                    */
#include "transport/jack/info.hpp"      /* transport::jack::info class      */
#include "xpc/ring_buffer.hpp"          /* midi::ring_buffer<TYPE> template */

namespace rtl
{

class rtmidi_in_data;

/**
 *  Contains the JACK MIDI API data as a kind of scratchpad for this object.
 *  This guy needs a constructor taking parameters for an rtmidi_in_data
 *  pointer.
 */

class RTL66_DLL_PUBLIC midi_jack_data
{
    /**
     *  Holds data about JACK transport, to be used in midi_jack ::
     *  jack_frame_offset(). These values are a subset of what appears in the
     *  jack_position_t structure in jack/types.h.  Note that it has
     *  a copy of the jack_client_t pointer, or else the global version
     *  of it.  BEWARE!
     *
     *  This should ultimately be removed and used separately.
     */

    static transport::jack::info m_transport_info;

    /**
     *  Holds the JACK sequencer client pointer so that it can be used by the
     *  midibus objects.  This is actually an opaque pointer; there is no way
     *  to get the actual fields in this structure; they can only be accessed
     *  through functions in the JACK API.  Note that it is also stored as a
     *  void pointer in global_client_info().midi_handle().  This item is the
     *  single JACK client created by the midi_jack_info object.
     */

     jack_client_t * m_jack_client;

    /**
     *  Holds the JACK port information of the JACK client.
     */

    jack_port_t * m_jack_port;

    /**
     *  Holds a pointer to the size of data for communicating between the
     *  client ring-buffer and the JACK port's internal buffer.
     */

    xpc::ring_buffer<midi::message> * m_jack_buffer;

    /**
     *  The last time-stamp obtained.  Use for calculating the delta time, I
     *  would imagine.
     */

    jack_time_t m_jack_lasttime;

    /**
     *  Optional, used if available.
     */

#if RTL66_HAVE_SEMAPHORE_H

    bool m_semaphores_inited;
    sem_t m_sem_cleanup;
    sem_t m_sem_needpost;

#endif

#if defined RTL66_JACK_PORT_REFRESH_CALLBACK

    /**
     *  An unsigned 32-bit port ID that starts out as null_system_port_id(),
     *  and, at least in JACK can be filled with actual internal port number
     *  assigned during port registration.
     */

    jack_port_id_t m_internal_port_id;

#endif

    /**
     *  Holds special data peculiar to the client and its MIDI input
     *  processing. This data consists of the midi_queue message queue and a
     *  few boolean flags.
     */

    rtmidi_in_data * m_jack_rtmidiin;

public:

    midi_jack_data ();
    midi_jack_data (const midi_jack_data &) = delete;
    midi_jack_data & operator = (const midi_jack_data &) = delete;
    ~midi_jack_data ();

    /*
     *  Frame offset-related functions.
     */

    static bool recalculate_frame_factor
    (
        const jack_position_t & pos, jack_nframes_t F
    )
    {
        return m_transport_info.recalculate_frame_factor(pos, F);
    }

    static jack_nframes_t frame_offset (jack_nframes_t F, midi::pulse p)
    {
        return m_transport_info.frame_offset(F, p);
    }

    static jack_nframes_t frame_offset
    (
        jack_nframes_t f, jack_nframes_t F, midi::pulse p
    )
    {
        return m_transport_info.frame_offset(f, F, p);
    }

    static jack_nframes_t frame_estimate (midi::pulse p)
    {
        return m_transport_info.frame_estimate(p);
    }

    static void cycle_frame
    (
        midi::pulse p, jack_nframes_t & cycle, jack_nframes_t & offset
    )
    {
        return m_transport_info.cycle_frame(p, cycle, offset);
    }

    static double cycle (jack_nframes_t f, jack_nframes_t F)
    {
        return m_transport_info.cycle(f, F);
    }

    static double pulse_cycle (midi::pulse p, jack_nframes_t F)
    {
        return m_transport_info.cycle(p, F);
    }

    static double frame (midi::pulse p)
    {
        return m_transport_info.frame(p);
    }

    static jack_nframes_t frame_rate ()
    {
        return m_transport_info.frame_rate();
    }

    static jack_nframes_t start_frame ()
    {
        return m_transport_info.start_frame();
    }

    static double ticks_per_beat ()
    {
        return m_transport_info.ticks_per_beat();
    }

    static double beats_per_minute ()
    {
        return m_transport_info.beats_per_minute();
    }

    static double frame_factor ()
    {
        return m_transport_info.frame_factor();
    }

    static bool use_offset ()
    {
        return m_transport_info.use_offset();
    }

    static jack_nframes_t cycle_frame_count ()
    {
        return m_transport_info.cycle_frame_count();
    }

    static jack_nframes_t size_compensation ()
    {
        return m_transport_info.size_compensation();
    }

    static jack_time_t cycle_time_us ()
    {
        return m_transport_info.cycle_time_us();
    }

    static unsigned cycle_time_ms ()
    {
        return m_transport_info.cycle_time_ms();
    }

    static jack_time_t pulse_time_us ()
    {
        return m_transport_info.pulse_time_us();
    }

    static unsigned pulse_time_ms ()
    {
        return m_transport_info.pulse_time_ms();
    }

    static unsigned delta_time_ms (midi::pulse p)
    {
        return m_transport_info.delta_time_ms(p);
    }

    static void frame_rate (jack_nframes_t nf)
    {
        return m_transport_info.frame_rate(nf);
    }

    static void start_frame (jack_nframes_t nf)
    {
        return m_transport_info.start_frame(nf);
    }

    static void ticks_per_beat (double tpb)             // double????
    {
        return m_transport_info.ticks_per_beat(tpb);
    }

    static void beats_per_minute (midi::bpm bp)
    {
        return m_transport_info.beats_per_minute(bp);
    }

    static void frame_factor (double ff)
    {
        return m_transport_info.frame_factor(ff);
    }

    static void use_offset (bool flag)
    {
        return m_transport_info.use_offset(flag);
    }

    static void cycle_frame_count (jack_nframes_t cfc)
    {
        return m_transport_info.cycle_frame_count(cfc);
    }

    static void size_compensation (jack_nframes_t szc)
    {
        return m_transport_info.size_compensation(szc);
    }

    static void cycle_time_us (jack_time_t jt)
    {
        return m_transport_info.cycle_time_us(jt);
    }

    static void pulse_time_us (jack_time_t jt)
    {
        return m_transport_info.pulse_time_us(jt);
    }

    /*
     *  Basic member access. Getters and setters.
     */

    bool valid_buffer () const
    {
        return not_nullptr(m_jack_buffer);
    }

    xpc::ring_buffer<midi::message> * jack_buffer ()
    {
        return m_jack_buffer;
    }

    void jack_buffer (xpc::ring_buffer<midi::message> * rb)
    {
        m_jack_buffer = rb;
    }

    jack_client_t * jack_client ()
    {
        return m_jack_client;           // not m_transport_info.jack_client()
    }

    jack_port_t * jack_port ()
    {
        return m_jack_port;
    }

    rtmidi_in_data * rt_midi_in()
    {
        return m_jack_rtmidiin;
    }

    jack_time_t jack_lasttime () const
    {
        return m_jack_lasttime;
    }

#if defined RTL66_JACK_PORT_REFRESH_CALLBACK

    jack_port_id_t internal_port_id () const
    {
        return m_internal_port_id;
    }

#endif

public:

    void jack_client (jack_client_t * c)
    {
        m_jack_client = c;              // not m_transport_info.jack_client(c)
    }

    void jack_port (jack_port_t * p)
    {
        m_jack_port = p;
    }

    void rt_midi_in (rtmidi_in_data * rid)
    {
        m_jack_rtmidiin = rid;
    }

    void jack_lasttime (jack_time_t lt)
    {
        m_jack_lasttime = lt;
    }

#if defined RTL66_JACK_PORT_REFRESH_CALLBACK

    void internal_port_id (uint32_t id)
    {
        m_internal_port_id = id;
    }

#endif

#if RTL66_HAVE_SEMAPHORE_H

    bool semaphore_init ();
    void semaphore_destroy ();
    bool semaphore_post_and_wait ();
    bool semaphore_wait_and_post ();

#endif

};          // class midi_jack_data

}           // namespace rtl

#endif      // RTL66_BUILD_JACK

#endif      // RTL66_RTL_MIDI_JACK_DATA_HPP

/*
 * midi_jack_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

