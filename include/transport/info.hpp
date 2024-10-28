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
 * \updates       2024-05-30
 * \license       See above.
 */

#if defined USE_ATOMIC_RESOLUTION_CHANGE_FLAG
#include <atomic>                       /* std::atomic template             */
#endif

#include "midi/midibytes.hpp"           /* rtl66::midibyte, other aliases   */

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
     *  What role is transport playing?
     */

    timebase m_timebase;

    /**
     *  Indicates that transport is running.
     *
     *  What about if the timebase is "non"?
     */

    bool m_is_running;

    /**
     *  Holds the beat width value as obtained from the MIDI file.  The default
     *  value is 4.  Also called "beat length" or "beat type".
     */

    int m_beat_width;

    /**
     *  Holds the beats/bar value as obtained from the MIDI file.
     *  The default value is 4. Also called "beats per measure".
     */

    int m_beats_per_bar;

    /**
     *  Holds the current BPM for the song (beats per minute).
     *  ALSA and JACK can use this value internally.
     */

    midi::bpm m_beats_per_minute;

    /**
     *  Holds the current PPQN (pulses per quarternote) for usage in various
     *  actions.  Unlike in the "legacy" Seq66, this will hold the file PPQN
     *  if provided. ALSA and JACK can use this value internally.
     */

    midi::ppqn m_ppqn;

    /**
     *  Indicates if the BPM or PPQN value has changed, for internal handling in
     *  output_func(). Note that atomic-bool has a deleted copy constructor.
     *  This ripples down the info class hierarchy, so we will punt and hope we
     *  can figure out a better way later.
     */

#if defined USE_ATOMIC_RESOLUTION_CHANGE_FLAG
    std::atomic<bool> m_resolution_change;
#else
    bool m_resolution_change;
#endif

    /**
     *  Useful in handling engines like JACK, where it's ticks are ten times as
     *  precise as MIDI ticks. 1.0 for ALSA or 10.0 * PPQN for JACK.
     */

    double m_ticks_per_beat;

    /**
     *  Holds the current duration of a MIDI pulse, in microseconds.
     */

    midi::microsec m_pulse_time_us;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of MIDI clocks between metronome clicks.  The default value of
     *  this item is 24.  It can also be read from some SMF 1 files, such as
     *  our hymne.mid example.
     */

    int m_clocks_per_metronome;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  Useful in export.  A
     *  duplicate of the same member in the sequence class.
     */

    int m_32nds_per_quarter;

    /**
     *  The duration of a quarter note (or beat as well?) in microseconds.
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Tempo meta event.  Useful in export.  A duplicate of the
     *  same member in the sequence class.
     */

    midi::microsec m_us_per_quarter_note;

    /**
     *  Holds the "one measure's worth" of pulses (ticks), which is normally
     *  m_ppqn * 4.  We can save some multiplications, and, more importantly,
     *  later define a more flexible definition of "one measure's worth" than
     *  simply four quarter notes.
     */

    midi::pulse m_one_measure;

    /**
     *  It seems that this member, if true, forces a repositioning to the left
     *  (L) tick marker.
     */

    bool m_reposition;

    /**
     *  Holds the starting tick for playing.  By default, this value is always
     *  reset to the value of the "left tick".  We want to eventually be able
     *  to leave it at the last playing tick, to support a "pause"
     *  functionality. Note that "tick" is actually "pulses".
     */

    mutable midi::pulse m_start_tick;   /* for starting playback            */

    /**
     *  The m_tick member holds the tick to be used in
     *  displaying the progress bars and the maintime pill.  It is mutable
     *  because sometimes we want to adjust it in a const function for pause
     *  functionality.
     */

    mutable midi::pulse m_tick;         /* current tick (MIDI pulse)        */
    mutable midi::pulse m_left_tick;    /* for looping                      */
    mutable midi::pulse m_right_tick;   /* for looping                      */
    mutable bool m_looping;

public:

    info ();
    info (int bw, int bpb, midi::bpm bpmin, midi::ppqn ppq);
    info (info &&) = delete;
    info (const info &) = default;
    info & operator = (info &&) = delete;
    info & operator = (const info &) = default;
    ~info () = default;

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
        return m_beat_width;
    }

    int beats_per_bar () const
    {
        return m_beats_per_bar;
    }

    midi::bpm beats_per_minute () const
    {
        return m_beats_per_minute;
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
        return m_ppqn;
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
        return m_clocks_per_metronome;
    }

    int get_32nds_per_quarter () const
    {
        return m_32nds_per_quarter;
    }

    midi::microsec us_per_quarter_note () const
    {
        return m_us_per_quarter_note;
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

    /*
     * Simple setter. for the one that iterates over patterns, see
     * set_beat_length().
     */

    void beat_width (int bw)
    {
        m_beat_width = bw;
#if defined RTL66_BUILD_JACK_ISREADY
        m_jack_transport.set_beat_width(bw);
#endif
    }

    /*
     * Simple setter. for the one that iterates over patterns, see
     * set_beats_per_measure().
     */

    void beats_per_bar (int bpb)
    {
        m_beats_per_bar = bpb;
#if defined RTL66_BUILD_JACK_ISREADY
        m_jack_transport.set_beats_per_measure(bpb);
#endif
    }

    void beats_per_minute (midi::bpm bp)
    {
        m_beats_per_minute = bp;
    }

    void ticks_per_beat (double tpb)
    {
        m_ticks_per_beat = tpb;
    }

    void set_ppqn (midi::ppqn ppq)
    {
        m_ppqn = ppq;
    }

    void pulse_time_us (midi::microsec jt)
    {
        m_pulse_time_us = jt;
    }

    void clocks_per_metronome (int cpm)
    {
        m_clocks_per_metronome = cpm;
    }

    void set_32nds_per_quarter (int tpq)
    {
        m_32nds_per_quarter = tpq;
    }

    void us_per_quarter_note (midi::microsec upqn)
    {
        m_us_per_quarter_note = upqn;
    }

    void one_measure (midi::pulse p)
    {
        m_one_measure = p * 4;              /* simplistic */
        m_right_tick = m_one_measure * 4;   /* simplistic */
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

