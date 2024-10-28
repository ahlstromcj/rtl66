#if ! defined RTL66_TRANSPORT_JACK_INFO_HPP
#define RTL66_TRANSPORT_JACK_INFO_HPP

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
 * \file          transport/jack/info.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2017-01-02
 * \updates       2024-01-16
 * \license       See above.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 *
 * jack_position_t:
 *
 *      -   jack_time_t usecs           // uint64_t, or midi::microsec
 *      -   jack_nframes_t frame_rate   // uint32_t current frames/second
 *      -   jack_nframes_t frame        // frame number
 *      -   int32_t bar, beat, tick     // bar, beat-in-bar, tick-in-beat
 *      -   double bar_start_tick
 *      -   float beats_per_bar
 *      -   float beat_type
 *      -   double ticks_per_beat
 *      -   double beats_per_minute
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include <jack/jack.h>                  /* system-wide JACK header          */

#include "transport/info.hpp"           /* transport::info class            */

namespace transport
{

namespace jack
{

/**
 *  Contains the JACK MIDI API data as a kind of scratchpad for this object.
 *  This guy needs a constructor taking parameters for an rtmidi_in_data
 *  pointer.
 */

class info : public transport::info
{
    /**
     *  Holds data about JACK transport, to be used in midi_jack ::
     *  jack_frame_offset(). These values are a subset of what appears in the
     *  jack_position_t structure in jack/types.h.
     */

    jack_nframes_t m_jack_frame_rate;   /* e.g. 48000 or 96000 Hz   */
    jack_nframes_t m_jack_start_frame;  /* 0 or large number?       */
    jack_nframes_t m_cycle_frame_count; /* progress callback param  */
    jack_nframes_t m_size_compensation; /* from ttymidi.c           */
    jack_time_t m_cycle_time_us;        /* time between callbacks   */
    double m_jack_frame_factor;         /* frames per PPQN tick     */
    bool m_use_offset;                  /* requires JACK transport  */

    /**
     *  Holds the JACK sequencer client pointer so that it can be used by the
     *  midibus objects.  This is actually an opaque pointer; there is no way
     *  to get the actual fields in this structure; they can only be accessed
     *  through functions in the JACK API.  Note that it is also stored as a
     *  void pointer in midi_info::m_midi_handle.  This item is the single
     *  JACK client created by the midi_jack_info object.
     */

    jack_client_t * m_jack_client;

public:

    info ();
    info (info &&) = delete;
    info (const info &) = default;
    info & operator = (info &&) = delete;
    info & operator = (const info &) = default;
    ~info () = default;

    /*
     *  Frame offset-related functions.
     */

    bool recalculate_frame_factor
    (
        const jack_position_t & pos,
        jack_nframes_t F
    );
    jack_nframes_t frame_offset (jack_nframes_t F, midi::pulse p) const;
    jack_nframes_t frame_offset
    (
        jack_nframes_t cyclestart,
        jack_nframes_t F,
        midi::pulse p
    );
    jack_nframes_t frame_estimate (midi::pulse p) const;
    void cycle_frame
    (
        midi::pulse p, jack_nframes_t & cycle, jack_nframes_t & offset
    ) const;
    double cycle (jack_nframes_t f, jack_nframes_t F) const;
    double pulse_cycle (midi::pulse p, jack_nframes_t F) const;

    double frame (midi::pulse p) const
    {
        return double(p) * frame_factor();
    }

    jack_nframes_t frame_rate () const
    {
        return m_jack_frame_rate;
    }

    jack_nframes_t start_frame () const
    {
        return m_jack_start_frame;
    }

    double frame_factor () const
    {
        return m_jack_frame_factor;
    }

    bool use_offset () const
    {
        return m_use_offset;
    }

    jack_nframes_t cycle_frame_count () const
    {
        return m_cycle_frame_count;
    }

    jack_nframes_t size_compensation () const
    {
        return m_size_compensation;
    }

    jack_time_t cycle_time_us () const
    {
        return m_cycle_time_us;
    }

    unsigned cycle_time_ms () const
    {
        return m_cycle_time_us / 1000;
    }

    void frame_rate (jack_nframes_t nf)
    {
        m_jack_frame_rate = nf;
    }

    void start_frame (jack_nframes_t nf)
    {
        m_jack_start_frame = nf;
    }

    void frame_factor (double ff)
    {
        m_jack_frame_factor = ff;
    }

    void use_offset (bool flag)
    {
        m_use_offset = flag;
    }

    void cycle_frame_count (jack_nframes_t cfc)
    {
        m_cycle_frame_count = cfc;
    }

    void size_compensation (jack_nframes_t szc)
    {
        m_size_compensation = szc;
    }

    void cycle_time_us (jack_time_t jt)
    {
        m_cycle_time_us = jt;
    }

    /*
     *  Basic member access. Getters and setters.
     */

    jack_client_t * jack_client ()
    {
        return m_jack_client;
    }

    void jack_client (jack_client_t * jc)
    {
        m_jack_client = jc;
    }

};          // class info

}           // namespace jack

}           // namespace transport

#endif      // RTL66_BUILD_JACK

#endif      // RTL66_TRANSPORT_JACK_INFO_HPP

/*
 * transport/jack/info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

