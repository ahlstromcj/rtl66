#if ! defined RTL66_MIDI_TIMING_HPP
#define RTL66_MIDI_TIMING_HPP

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
 * \file          timing.hpp
 *
 *  This module declares a small class for MIDI timing information.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2024-05-26
 * \license       GNU GPLv2 or above
 *
 *  Do not confuse this timing module with the timing module in the xpc66
 *  library. This module is MIDI timing, the other is OS-specific timing.
 */

#include "midi/midibytes.hpp"           /* midi::bpm double type            */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace midi
{

/**
 *  We anticipate the need to have a small structure holding the parameters
 *  needed to calculate MIDI times within an arbitrary song.
 */

class timing
{
    /**
     *  This value should match the BPM value selected when editing the song.
     *  This value is most commonly set to 120, but is also read from the MIDI
     *  file.  This value is needed if one want to calculate durations in true
     *  time units such as seconds, but is not needed to calculate the number
     *  of pulses/ticks/divisions.
     */

    bpm m_beats_per_minute {0.0};       /* T (tempo, BPM in upper-case)   */

    /**
     *  This value should match the numerator value selected when editing the
     *  sequence.  This value is most commonly set to 4.
     */

    int m_beats_per_measure {0};        /* B (bpm in lower-case)          */

    /**
     *  This value should match the denominator value selected when editing
     *  the sequence.  This value is most commonly set to 4, meaning that the
     *  fundamental beat unit is the quarter note.
     *
     */

    int m_beat_width {0};               /* W (bw in lower-case)           */

    /**
     *  This value provides the precision of the MIDI song.  This value is
     *  most commonly set to 192, but is also read from the MIDI file.  We are
     *  still working getting "non-standard" values to work.
     */

    ppqn m_ppqn {0};                    /* P (PPQN or ppqn)               */

public:

    timing () = default;

    timing (bpm bpminute, int bpmeasure, int beatwidth, int ppq) :
        m_beats_per_minute  (bpminute),
        m_beats_per_measure (bpmeasure),
        m_beat_width        (beatwidth),
        m_ppqn              (ppq)
    {
        // No code
    }

    bpm BPM () const
    {
        return m_beats_per_minute;
    }

    bpm beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    /**
     * \param b
     *      The value to which to set the number of beats/minute.
     *      We can add validation later.
     */

    void BPM (bpm b)
    {
        m_beats_per_minute = b;
    }

    void beats_per_minute (bpm b)
    {
        m_beats_per_minute = b;
    }

    int beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    /**
     * \param b
     *      The value to which to set the number of beats/measure.
     *      We can add validation later.
     */

    void beats_per_measure (int b)
    {
        m_beats_per_measure = b;
    }

    int beat_width () const
    {
        return m_beat_width;
    }

    /**
     * \param bw
     *      The value to which to set the number of beats in the denominator
     *      of the time signature.
     *      We can add validation later.
     */

    void beat_width (int bw)
    {
        m_beat_width = bw;
    }

    ppqn PPQN () const
    {
        return m_ppqn;
    }

    /**
     * \param p
     *      The value to which to set the PPQN member.
     *      We can add validation later.
     */

    void PPQN (int p)
    {
        m_ppqn = ppqn(p);
    }

};              // class timing

}               // namespace midi

#endif          // RTL66_MIDI_TIMING_HPP

/*
 * timing.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

