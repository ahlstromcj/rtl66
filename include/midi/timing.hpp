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
 * \updates       2026-01-25
 * \license       GNU GPLv2 or above
 *
 *  This is a header-only module.
 *
 *  Do not confuse this timing module with the timing module in the xpc66
 *  library. This module is MIDI timing, the other is OS-specific timing.
 */

#include "midi/midibytes.hpp"           /* midi::bpm double type            */
#include "rtl/rtl_build_macros.h"       /* PPQN, BPM, and other macros      */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace midi
{

/**
 *  Minimum, default, and maximum values for "beats-per-measure".  A new
 *  addition for the Qt 5 user-interface.  This is the "numerator" in a 4/4
 *  time signature.  It is also the value used for JACK's
 *  jack_position_t.beats_per_bar field.  For abbreviation, we will call this
 *  value "BPB", or "beats per bar", to distinguish it from "BPM", or "beats
 *  per minute".
 */

static const int c_min_beats_per_measure {  1 };
static const int c_def_beats_per_measure {  4 };
static const int c_max_beats_per_measure { 32 };

/**
 *  The minimum, default, and maximum values of the beat width.  A new
 *  addition for the Qt 5 user-interface.  This is the "denominator" in a 4/4
 *  time signature.  It is also the value used for JACK's
 *  jack_position_t.beat_type field. For abbreviation, we will call this value
 *  "BW", or "beat width", not to be confused with "bandwidth".
 */

static const int c_min_beat_width {  1 };
static const int c_def_beat_width {  4 };
static const int c_max_beat_width { 32 };

/**
 *  Minimum, default, and maximum values for global beats-per-minute, also known
 *  as "BPM".  Do not confuse this "bpm" with the other one, "beats per measure";
 *  we use "BPB" (beats-per-bar) for clarity.  Also, we multiply the BPM by a
 *  scale factor so that we can get extra precision in the value when stored as a
 *  long integer in the MIDI file in the proprietary "bpm" section.  See the
 *  midifile class.  Lastly, we provide a tap-button timeout value (which could
 *  some day be mode configurable.
 */

static const midi::bpm c_min_beats_per_minute {    2.0 };
static const midi::bpm c_def_beats_per_minute {  120.0 };
static const midi::bpm c_max_beats_per_minute {  600.0 };
static const float c_beats_per_minute_scale   { 1000.0 };
static const long c_bpm_tap_button_timeout    { 5000L };       /* milliseconds */
static const int c_min_bpm_precision          {    0 };
static const int c_def_bpm_precision          {    0 };
static const int c_max_bpm_precision          {    2 };
static const midi::bpm c_min_bpm_increment    {    0.01 };
static const midi::bpm c_def_bpm_increment    {    1.0 };
static const midi::bpm c_max_bpm_increment    {    50.0 };

/**
 *  Minimum and maximum supported PPQN values.  Now hidden, used in the public
 *  function usrsettings::is_ppqn_valid().
 */

static const int c_minimum_ppqn  {    24 }; /* was 32, not a multiple of 24 */
static const int c_maximum_ppqn  { 19200 }; /* way above the useful maximum */

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
     *  of pulses/ticks/divisions. Symbol T (tempo, BPM in upper-case).
     */

    midi::bpm m_beats_per_minute { RTL66_DEFAULT_BPM };         /* 120.0    */

    /**
     *  This value should match the numerator value selected when editing the
     *  sequence.  This value is most commonly set to 4.
     */

    int m_beats_per_measure  { RTL66_DEFAULT_BEATS_PER_BAR };   /* 4        */

    /**
     *  This value should match the denominator value selected when editing
     *  the sequence.  This value is most commonly set to 4, meaning that the
     *  fundamental beat unit is the quarter note.
     *
     */

    int m_beat_width { RTL66_DEFAULT_BEAT_WIDTH };              /* 4        */

    /**
     *  A modifiable version of the default PPQN.
     */

    midi::ppqn m_default_ppqn { RTL66_DEFAULT_PPQN };           /* 192      */

    /**
     *  This value provides the precision of the MIDI song.  This value is
     *  most commonly set to 192, but is also read from the MIDI file.
     */

    midi::ppqn m_ppqn { RTL66_DEFAULT_PPQN };                   /* 192      */

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of MIDI clocks between metronome clicks.  The default value of
     *  this item is 24.  It can also be read from some SMF 1 files, such as
     *  our hymne.mid example.
     */

    int m_clocks_per_metronome
    {
        RTL66_DEFAULT_CLOCKS_PER_METRO                          /* 24       */
    };

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  Useful in export.  A
     *  duplicate of the same member in the sequence class.
     */

    int m_32nds_per_quarter
    {
        RTL66_DEFAULT_32NDS_PER_QUARTER                         /* 8        */
    };

    /**
     *  The duration of a quarter note (or beat as well?) in microseconds.
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Tempo meta event.  Useful in export.  A duplicate of the
     *  same member in the sequence class. Defaults to 500,000.
     */

    midi::microsec m_us_per_quarter_note { RTL66_DEFAULT_US_PER_Q };

public:

    timing () = default;

    timing (midi::bpm bpminute, int bpmeasure, int beatwidth, int ppq) :
        m_beats_per_minute  (bpminute),
        m_beats_per_measure (bpmeasure),
        m_beat_width        (beatwidth),
        m_ppqn              (ppq)
    {
        /*
         * inline midi::microsec tempo_us_from_bpm (midi::bpm bp) from the
         * calculations module reimplemented here for convenience.
         */

        m_us_per_quarter_note = midi::microsec
        (
            bpminute >= 1.0 ? (60000000.0 / bpminute) : 0.0
        );
    }

    bool BPM_is_valid (midi::bpm b) const
    {
        return b >= c_min_beats_per_minute && b <= c_max_beats_per_minute;
    }

    midi::bpm BPM_default () const
    {
        return c_def_beats_per_minute;
    }

    midi::bpm BPM () const
    {
        return m_beats_per_minute;
    }

    midi::bpm beats_per_minute () const
    {
        return BPM();
    }

    bool BPM (midi::bpm b)
    {
        if (BPM_is_valid(b))
        {
            m_beats_per_minute = b;
            return true;
        }
        else
            return false;
    }

    void beats_per_minute (midi::bpm b)
    {
        BPM(b);
    }

    midi::ulong BPM_scaled (midi::bpm b)
    {
        return b * c_beats_per_minute_scale;
    }

    midi::bpm BPM_unscaled (midi::ulong b)
    {
        midi::bpm result = midi::bpm(b);
        if (result > (c_beats_per_minute_scale - 1.0f))
            result /= c_beats_per_minute_scale;

        return result;
    }

    long BPM_tap_button_timeout () const
    {
        return c_bpm_tap_button_timeout;
    }

    bool BPB_is_valid (int b) const
    {
        return b >= c_min_beats_per_measure && b <= c_max_beats_per_measure;
    }

    int BPB_default () const
    {
        return c_def_beats_per_measure;
    }

    int BPB () const
    {
        return m_beats_per_measure;
    }

    int beats_per_measure () const
    {
        return BPB();
    }

    bool BPB (int b)
    {
        if (BPB_is_valid(b))
        {
            m_beats_per_measure = b;
            return true;
        }
        return false;
    }

    void beats_per_measure (int b)
    {
        m_beats_per_measure = b;
    }

    /**
     *  Beat width should be a power of two.  We do not enforce that, though.
     */

    bool BW_is_valid (int b) const
    {
        return b >= c_min_beat_width && b <= c_max_beat_width;
    }

    int BW_default () const
    {
        return c_def_beat_width;
    }

    int BW () const
    {
        return m_beat_width;
    }

    int beat_width () const
    {
        return BW();
    }

    bool BW (int b)
    {
        if (BW_is_valid(b))
        {
            m_beat_width = b;
            return true;
        }
        else
            return false;
    }

    bool beat_width (int b)
    {
        return BW(b);
    }

    bool PPQN_is_valid (int p) const
    {
        return p >= c_minimum_ppqn && p <= c_maximum_ppqn;
    }

    void PPQN_set_default (int p)
    {
        if (PPQN_is_valid(p))
            m_default_ppqn = p;
    }

    midi::ppqn PPQN_default () const
    {
        return m_default_ppqn;
    }

    midi::ppqn PPQN () const
    {
        return m_ppqn;
    }

    bool PPQN (int p)
    {
        if (PPQN_is_valid(p))
        {
            m_ppqn = midi::ppqn(p);
            return true;
        }
        else
            return false;
    }

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

    bool clocks_per_metronome (int cpm)
    {
        m_clocks_per_metronome = cpm;
        return true;
    }

    bool set_32nds_per_quarter (int tpq)
    {
        m_32nds_per_quarter = tpq;
        return true;
    }

    bool us_per_quarter_note (midi::microsec upqn)
    {
        m_us_per_quarter_note = upqn;
        return true;
    }

};              // class timing

}               // namespace midi

#endif          // RTL66_MIDI_TIMING_HPP

/*
 * timing.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

