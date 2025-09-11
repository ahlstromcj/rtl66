#if ! defined RTL66_TRANSPORT_INFO_HPP
#define RTL66_TRANSPORT_INFO_HPP

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
 * \file          info.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2017-11-10
 * \updates       2025-09-09
 * \license       See above.
 */

#if defined RTL_ATOMIC_RESOLUTION_CHANGE_FLAG
#include <atomic>                       /* std::atomic template             */
#endif

#include "midi/midibytes.hpp"           /* midi::midibyte, other aliases    */
#include "midi/timing.hpp"              /* midi::timing class               */
#include "rtl/rtl_build_macros.h"       /* PPQN, BPM, and other macros      */

namespace transport
{

/**
 *  Indicates whether Seq66 or another program is the timebase master, if that
 *  concept is applicable..
 *
 * \var none
 *      Transport (e.g. JACK) is not being used.
 *
 * \var slave
 *      An external program is timebase master and we disregard all local
 *      tempo information. Instead, we use onl the BPM provided by JACK.
 *
 * \var master
 *      Whether by force or conditionally, this program is JACK master.
 *
 * \var conditional
 *      This value is just for requesting conditional master in the 'rc' file.
 *
 * \var midiclock
 *      Use MIDI clock instead of other transport (e.g. JACK transport).
 */

enum class timebase
{
    none,
    slave,
    master,
    conditional,
    midiclock

    /*
     * timecode // mmc ?
     * smpte?
     */
};

/**
 *  Holds data about general and some JACK transport.  Contains the JACK MIDI
 *  API data as a kind of scratchpad for this object.
 */

class info
{
    /**
     *  Encapsulates beats/minute, beats/bar, beat width, PPQN, and items
     *  related to the MIDI tempo event. Access them via time_values()
     *  functions.
     */

    midi::timing m_time_values;

    /**
     *  What role is transport playing?
     */

    timebase m_timebase { timebase::none };

    /**
     *  Indicates that transport is running.
     *
     *  What about if the timebase is "non"?
     */

    bool m_is_running { false };

    /**
     *  Indicates if the BPM or PPQN value has changed, for internal handling in
     *  output_func(). Note that atomic-bool has a deleted copy constructor.
     *  This ripples down the info class hierarchy, so we will punt and hope we
     *  can figure out a better way later.
     */

#if defined RTL_ATOMIC_RESOLUTION_CHANGE_FLAG
    std::atomic<bool> m_resolution_change { true };
#else
    bool m_resolution_change { true };
#endif

    /**
     *  Useful in handling engines like JACK, where it's ticks are ten times as
     *  precise as MIDI ticks. 1.0 for ALSA or 10.0 * PPQN for JACK.
     */

    double m_ticks_per_beat { RTL66_DEFAULT_PPQN };             /* 192      */

    /**
     *  Holds the current duration of a MIDI pulse, in microseconds.
     */

    midi::microsec m_pulse_time_us { 0 };

    /**
     *  Holds the "one measure's worth" of pulses (ticks), which is normally
     *  m_ppqn * 4.  We can save some multiplications, and, more importantly,
     *  later define a more flexible definition of "one measure's worth" than
     *  simply four quarter notes.
     */

    midi::pulse m_one_measure { 0 };

    /**
     *  It seems that this member, if true, forces a repositioning to the left
     *  (L) tick marker.
     */

    bool m_reposition { false };

    /**
     *  Holds the starting tick for playing.  By default, this value is always
     *  reset to the value of the "left tick".  We want to eventually be able
     *  to leave it at the last playing tick, to support a "pause"
     *  functionality. Note that "tick" is actually "pulses".
     */

    mutable midi::pulse m_start_tick { 0 };         /* to start playback    */

    /**
     *  The m_tick member holds the tick to be used in
     *  displaying the progress bars and the maintime pill.  It is mutable
     *  because sometimes we want to adjust it in a const function for pause
     *  functionality.
     */

    mutable midi::pulse m_tick { 0 };               /* current MIDI pulse   */
    mutable midi::pulse m_left_tick { 0 };          /* for looping          */
    mutable midi::pulse m_right_tick { 0 };         /* for looping          */
    mutable bool m_looping { false };

public:

    info () = default;
    info (int bw, int bpb, midi::bpm bpmin, midi::ppqn ppq);
    info (info &&) = delete;
    info (const info &) = default;
    info & operator = (info &&) = delete;
    info & operator = (const info &) = default;
    ~info () = default;

    midi::timing & time_values ()
    {
        return m_time_values;
    }

    const midi::timing & time_values () const
    {
        return m_time_values;
    }

    bool is_running () const
    {
        return m_is_running;
    }

    bool is_master () const
    {
        return m_timebase == timebase::master;
    }

    bool is_slave () const
    {
        return m_timebase == timebase::slave;
    }

    bool have_transport () const
    {
        return m_timebase != timebase::none;
    }

    bool jack_transport () const
    {
        return is_master() || is_slave();
    }

    bool no_transport () const
    {
        return m_timebase == timebase::none;
    }

    int beat_width () const
    {
        return time_values().BW();
    }

    int beats_per_bar () const
    {
        return time_values().BPB();
    }

    midi::bpm beats_per_minute () const
    {
        return time_values().BPM();
    }

    double ticks_per_beat () const
    {
        return m_ticks_per_beat;
    }

    bool resolution_change () const
    {
        return m_resolution_change;
    }

    void resolution_change_clear ()
    {
        m_resolution_change = false;
    }

    void resolution_change_management
    (
        midi::bpm bpmfactor,
        midi::ppqn ppq,
        int & bpm_times_ppqn,
        double & dct,
        double & pus
    );

    midi::ppqn get_ppqn () const
    {
        return time_values().PPQN();
    }

    midi::microsec pulse_time_us () const
    {
        return m_pulse_time_us;
    }

    unsigned pulse_time_ms () const
    {
        return unsigned(m_pulse_time_us / 1000);
    }

    unsigned delta_time_ms (midi::pulse p) const;

    int clocks_per_metronome () const
    {
        return time_values().clocks_per_metronome();
    }

    int get_32nds_per_quarter () const
    {
        return time_values().get_32nds_per_quarter();
    }

    midi::microsec us_per_quarter_note () const
    {
        return time_values().us_per_quarter_note();
    }

    midi::pulse one_measure () const
    {
        return m_one_measure;
    }

    bool reposition () const
    {
        return m_reposition;
    }

    midi::pulse start_tick () const
    {
        return m_start_tick;
    }

    midi::pulse tick () const
    {
        return m_tick;
    }

    midi::pulse left_tick () const
    {
        return m_left_tick;
    }

    midi::pulse right_tick () const
    {
        return m_right_tick;
    }

    bool looping () const
    {
        return m_looping;
    }

public:

    /*
     * TODO: validation or sanity checks.
     */

    void time_signature (int bpb, int bw);
    void time_resolution (midi::ppqn ppq, midi::bpm bpmin);

    /*
     * Simple setter. for the one that iterates over patterns, see
     * set_beat_length().
     */

    void beat_width (int bw)
    {
        time_values().BW(bw);
#if defined RTL66_BUILD_JACK_HERE
        m_jack_transport.set_beat_width(bw);
#endif
    }

    /*
     * Simple setter. for the one that iterates over patterns, see
     * set_beats_per_measure().
     */

    bool beats_per_bar (int bpb)
    {
        bool result = time_values().BPB(bpb);
        if (result)
        {
#if defined RTL66_BUILD_JACK_HERE
        m_jack_transport.set_beats_per_measure(bpb);
#endif
        }
        return result;
    }

    bool beats_per_minute (midi::bpm bp)
    {
        return time_values().BPM(bp);
    }

    bool ticks_per_beat (double tpb)
    {
        m_ticks_per_beat = tpb;
        return true;
    }

    bool set_ppqn (midi::ppqn ppq)
    {
        return time_values().PPQN(ppq);
    }

    bool pulse_time_us (midi::microsec jt)
    {
        m_pulse_time_us = jt;
        return true;
    }

    bool clocks_per_metronome (int cpm)
    {
        return time_values().clocks_per_metronome(cpm);
    }

    bool set_32nds_per_quarter (int tpq)
    {
        return time_values().set_32nds_per_quarter(tpq);
    }

    bool us_per_quarter_note (midi::microsec upqn)
    {
        return time_values().us_per_quarter_note(upqn);
    }

    bool one_measure (midi::pulse p)
    {
        if (p > 0)
        {
            m_one_measure = p * 4;              /* simplistic */
            m_right_tick = m_one_measure * 4;   /* simplistic */
            return true;
        }
        else
            return false;
    }

    void reposition (bool flag)
    {
        m_reposition = flag;
    }

    void start_tick (midi::pulse tick)
    {
        m_start_tick = tick;         /* starting JACK tick/pulse value   */
    }

    void tick (midi::pulse t)
    {
        m_tick = t;
    }

    void looping (bool looping)
    {
        m_looping = looping;
    }

    /*
     * MIDI pulse (tick) management.
     */

    void left_tick (midi::pulse tick);
    midi::pulse left_tick_snap (midi::pulse tick, midi::pulse snap);
    void right_tick (midi::pulse tick);
    midi::pulse right_tick_snap (midi::pulse tick, midi::pulse snap);

};          // class info

}           // namespace transport

#endif      // RTL66_TRANSPORT_INFO_HPP

/*
 * info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

