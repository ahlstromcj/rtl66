#if ! defined RTL66_MIDI_CALCULATIONS_HPP
#define RTL66_MIDI_CALCULATIONS_HPP

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
 * \file          calculations.hpp
 *
 *  This module declares/defines some common calculations needed by the
 *  application.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-07
 * \updates       2025-05-01
 * \license       GNU GPLv2 or above
 *
 *  These items were moved from the globals.h module so that only the modules
 *  that need them need to include them.  Also included are some minor
 *  "utility" functions dealing with MIDI and port-related strings.  Many of
 *  the functions are defined in this header file, as inline code.
 */

#include "cpp_types.hpp"                /* std::string, tokenization alias  */
#include "midi/midibytes.hpp"           /* midi::pulse alias and much more  */
#include "midi/measures.hpp"            /* midi::measures class             */
#include "midi/timing.hpp"              /* midi::timing class               */

/**
 *  This is supposed to be a better method than rand(), but it still
 *  yields, seemingly, more negative numbers than positive numbers.
 */

#undef  CFG66_USE_UNIFORM_INT_DISTRIBUTION     /* EXPERIMENTAL */

/*
 * Global functions in the midi namespace for MIDI timing calculations.
 */

namespace midi
{

/**
 *  Indicates what kind of snap movement to apply in the snapped() template
 *  function.
 */

enum class snapper
{
    down,
    closest,
    up
};

/**
 *  Provides a clear enumeration of wave types supported by the wave function.
 *  See qlfoframe.
 */

enum class waveform
{
    none = 0,                   /**< No waveform, never used.               */
    sine,                       /**< Sine wave modulation.                  */
    sawtooth,                   /**< Saw-tooth (ramp) modulation.           */
    reverse_sawtooth,           /**< Reverse saw-tooth (decay).             */
    triangle,                   /**< No waveform, never used.               */
    exponential,                /**< A partial exponential rise.            */
    reverse_exponential,        /**< A partial exponential fall.            */
    max                         /**< Illegal value.                         */
};

inline int
cast (waveform wv)
{
    return static_cast<int>(wv);
}

inline waveform
waveform_cast (int v)
{
    return static_cast<waveform>(v);
}

/**
 *  Provides the short list of options for fixing a pattern length in the
 *  qpatternfix dialog.
 */

enum class lengthfix
{
    none = 0,                   /**< Not adjusting pattern length.          */
    measures,                   /**< The user sets the desired measures.    */
    rescale,                    /**< The user wants to rescale the pattern. */
    max                         /**< Illegal value.                         */
};

inline int
cast (lengthfix lv)
{
    return static_cast<int>(lv);
}

inline lengthfix
lengthfix_cast (int v)
{
    return static_cast<lengthfix>(v);
}

/**
 *  Provides the short list of options for the type of alteration used in
 *  the qpatternfix dialog.  Might be useful elsewhere as well. Note the
 *  automation::slot values shown.
 */

enum class alteration
{
    none = 0,       /**< grid_quant_none:    No adjustment of pattern.      */
    tighten,        /**< grid_quant_tighten: Adjust timing less halfway.    */
    quantize,       /**< grid_quant_full:    Adjust timing strictly.        */
    jitter,         /**< grid_quant_jitter:  Randomize timing slightly.     */
    random,         /**< grid_quant_random:  Randomize event magnitude.     */
    notemap,        /**< grid_quant_notemap: Apply configured note-mapping. */
    rev_notemap,    /**< Apply the note-map in the reverser direction.      */
    max         /**<                      Illegal value.                    */
};

inline int
cast (alteration lv)
{
    return static_cast<int>(lv);
}

inline alteration
quantization_cast (int v)
{
    return static_cast<alteration>(v);
}

/**
 *  Manifest constants for the "Applied Effects" GroupBox in qpatternfix.
 */

enum class fixeffect
{
    none            = 0x00,
    alteration      = 0x01,
    shifted         = 0x02,
    reversed        = 0x04,
    reversed_abs    = 0x08,             /* short for "reversed in place"    */
    shrunk          = 0x10,
    expanded        = 0x20,
    time_sig        = 0x40,
    truncated       = 0x80,
    all             = 0xFF
};

/**
 *  Tests that the rhs value's bit(s) is (are) set in the lhs value.
 */

inline bool
bit_test (fixeffect lhs, fixeffect rhs)
{
    return (static_cast<int>(lhs) & static_cast<int>(rhs)) != 0;
}

/**
 *  Grabs the bit(s) from rhs and OR's them into lhs, and returns the
 *  new value of lhs.
 */

inline fixeffect
bit_set (fixeffect lhs, fixeffect rhs)
{
    int L = static_cast<int>(lhs) | static_cast<int>(rhs);
    lhs = static_cast<fixeffect>(L);
    return lhs;
}

/*------------------------------------------------------------------------
 * Inline functions in the midi namespace.
 *------------------------------------------------------------------------*/

/**
 *  The base PPQN matches that of Seq24 through Seq66.
 */

inline int
base_ppqn ()
{
    return 192;
}

/**
 *  This value represent the smallest horizontal unit in a Sequencer66 grid.
 *  It is the number of pixels in the smallest increment between vertical
 *  lines in the grid.  For a zoom of 2, this number gets doubled.
 */

inline int
pixels_per_substep ()
{
    return 6;
}

/**
 *  This value represents the fundamental beats-per-bar.
 */

inline int
qn_beats ()
{
    return 4;
}

/**
 *  Taken from Seq66 usrsettings.
 *
 *      static const midi::bpm c_def_beats_per_minute =  120.0;
 *      static const int c_def_bpm_precision    =    0;
 *      static const int c_min_bpm_precision    =    0;
 */

inline midi::bpm
min_beats_per_minute ()
{
    return 2.0;
}

inline midi::bpm
max_beats_per_minute ()
{
    return 600.0;
}

inline int
max_bpm_precision ()
{
    return 2;
}

/**
 *  Formalizes the rescaling of ticks base on changing the PPQN.  For speed
 *  the parameters are all assumed to be valid.  The PPQN values supported
 *  explicity range from 32 to 19200.  The maximum tick value for 32-bit code
 *  is 2147483647.  At the highest PPQN that's almost 28000 measures.  64-bit
 *  code maxes at over 9E18.
 *
 *  This function is similar to pulses_scaled(), but that other function
 *  always scales against usr().base_ppqn() and allows for a zoom factor.
 *
 * \param tick
 *      The tick value to be rescaled.
 *
 * \param newppqn
 *      The new PPQN.
 *
 * \param oldppqn
 *      The original PPQN.  Defaults to 192.
 *
 * \return
 *      Returns the new tick value.
 */

inline midi::pulse
rescale_tick (midi::pulse tick, int newppqn, int oldppqn)
{
    return midi::pulse(double(tick) * newppqn / oldppqn + 0.5);
}

/**
 *  Converts tempo (e.g. 120 beats/minute) to microseconds.
 *  This function is the inverse of bpm_from_tempo_us().
 *
 * \param bpm
 *      The value of beats-per-minute.  If this value is 0, we'll get an
 *      arithmetic exception.
 *
 * \return
 *      Returns the tempo in qn/us.  If the bpm value is 0, then 0 is
 *      returned.
 */

inline double
tempo_us_from_bpm (midi::bpm bp)
{
    return bp > 0.009999999 ? (60000000.0 / bp) : 0.0 ;
}

/**
 *  This function calculates the effective beats-per-minute based on the value
 *  of a Tempo meta-event.  The tempo event's numeric value is given in 3
 *  bytes, and is in units of microseconds-per-quarter-note (us/qn).
 *
 * \param tempous
 *      The value of the Tempo meta-event, in units of us/qn.  If this value
 *      is 0, we'll get an arithmetic exception.
 *
 * \return
 *      Returns the beats per minute.  If the tempo value is 0, then 0 is
 *      returned.
 */

inline midi::bpm
bpm_from_tempo_us (double tempous)
{
    return tempous >= 1.0 ? (60000000.0 / tempous) : 0.0 ;
}

/**
 *  Provides a direct conversion from a byte array to the beats/minute
 *  value.
 *
 *  It might be worthwhile to provide an std::vector version at some point.
 *
 * \param t
 *      The 3 tempo bytes that were read directly from a MIDI file.
 */

extern midi::bpm tempo_us_from_bytes (const midi::bytes & tt);

inline midi::bpm
bpm_from_bytes (const midi::bytes & tt)
{
    return bpm_from_tempo_us(tempo_us_from_bytes(tt));
}

/**
 *  Calculates pulse-length from the BPM (beats-per-minute) and PPQN
 *  (pulses-per-quarter-note) values.  The formula for the pulse-length in
 *  seconds is:
 *
\verbatim
                 60
        P = ------------
             BPM * PPQN
\endverbatim
 *
 *  An alternate calculation is 60000000.0 / ppq / bp, but we can save
 *  a division operation.
 *
 * \param bpm
 *      Provides the beats-per-minute value.  No sanity check is made.  If
 *      this value is 0, we'll get an arithmetic exception.
 *
 * \param ppq
 *      Provides the pulses-per-quarter-note value.  No sanity check is
 *      made.  If this value is 0, we'll get an arithmetic exception.
 *
 * \return
 *      Returns the pulse length in microseconds.  If either parameter is
 *      invalid, then this function will crash. :-D
 */

inline double
pulse_length_us (midi::bpm bp, midi::ppqn ppq)
{
    /*
     * Let's use the original notation for now.
     *
     * return 60000000.0 / double(bp * p);
     */

    return 60000000.0 / ppq / bp;
}

/**
 *  Converts delta time in microseconds to ticks.  This function is the
 *  inverse of ticks_to_delta_time_us().
 *
 *  Please note that terms "ticks" and "pulses" are equivalent, and refer to
 *  the "pulses" in "pulses per quarter note".
 *
\verbatim
             beats       pulses           1 minute       1 sec
    P = 120 ------ * 192 ------ * T us *  ---------  * ---------
            minute       beats            60 sec       1,000,000 us
\endverbatim
 *
 *  Note that this formula assumes that a beat is a quarter note.  If a beat
 *  is an eighth note, then the P value would be halved, because there would
 *  be only 96 pulses per beat.  We will implement an additional function to
 *  account for the beat; the current function merely blesses some
 *  calculations made in the application.
 *
 * \param us
 *      The number of microseconds in the delta time.
 *
 * \param bpm
 *      Provides the beats-per-minute value, otherwise known as the "tempo".
 *
 * \param ppq
 *      Provides the pulses-per-quarter-note value, otherwise known as the
 *      "division".
 *
 * \return
 *      Returns the tick value.
 */

inline double
delta_time_us_to_ticks (unsigned long us, midi::bpm bp, midi::ppqn ppq)
{
    return double(bp * ppq * (us / 60000000.0f));
}

/**
 *  Converts the time in ticks ("clocks") to delta time in microseconds.
 *  The inverse of delta_time_us_to_ticks().
 *
 *  Please note that terms "ticks" and "pulses" are equivalent, and refer to
 *  the "pulses" in "pulses per quarter note".
 *
 *  Old:  60000000.0 * double(delta_ticks) / (double(bpm) * double(ppqn));
 *
 * \param delta_ticks
 *      The number of ticks or "clocks".
 *
 * \param bpm
 *      Provides the beats-per-minute value, otherwise known as the "tempo".
 *
 * \param ppq
 *      Provides the pulses-per-quarter-note value, otherwise known as the
 *      "division".
 *
 * \return
 *      Returns the time value in microseconds.
 */

inline double
ticks_to_delta_time_us (midi::pulse delta, midi::bpm bp, midi::ppqn ppq)
{
    return double(delta) * pulse_length_us(bp, ppq);
}

/**
 *  The MIDI beat clock (also known as "MIDI timing clock" or "MIDI clock") is
 *  a clock signal that is broadcast via MIDI to ensure that several
 *  MIDI-enabled devices or sequencers stay in synchronization.  Do not
 *  confuse it with "MIDI timecode".
 *
 *  The standard MIDI beat clock ticks every 24 times every quarter note
 *  (crotchet).
 *
 *  Unlike MIDI timecode, the MIDI beat clock is tempo-dependent. Clock events
 *  are sent at a rate of 24 PPQN (pulses per quarter note). Those pulses are
 *  used to maintain a synchronized tempo for synthesizers that have
 *  BPM-dependent voices and also for arpeggiator synchronization.
 *  The following value represents the standard MIDI clock rate in
 *  beats-per-quarter-note.
 */

inline int
midi_clock_beats_per_qn ()
{
    return 24;
}

/**
 *  A simple calculation to convert PPQN to MIDI clock ticks, which are emitting
 *  24 times per quarter note.
 *
 * \param ppq
 *      The number of pulses per quarter note.  For example, the default value
 *      for Seq24 is 192.
 *
 * \return
 *      The integer value of ppq / 24 [MIDI clock PPQN] is returned.
 */

inline int
clock_ticks_from_ppqn (midi::ppqn ppq)
{
    return ppq / midi_clock_beats_per_qn();
}

/**
 *  A simple calculation to convert PPQN to MIDI clock ticks.  The same as
 *  clock_ticks_from_ppqn(), but returned as a double float.
 *
 * \param ppq
 *      The number of pulses per quarter note.
 *
 * \return
 *      The double value of ppq / 24 [midi_clock_beats_per_qn] is returned.
 */

inline double
double_ticks_from_ppqn (midi::ppqn ppq)
{
    return ppq / double(midi_clock_beats_per_qn());
}

/**
 *  This provides a kind of fundamental value. See measures_to_ticks() and
 *  midi_clock_beats_per_qn()
 */

inline double
qn_per_beat (int bw = 4)
{
    return (bw > 0) ? 4.0 / double(bw) : 1.0 ;
}

/**
 *  Calculates the pulses per measure.  This calculation is extremely simple,
 *  and it provides an important constraint to pulse (ticks) calculations:
 *  the default number of pulses in a measure is always 4 times the PPQN value,
 *  regardless of the time signature.  The number pulses in a 7/8 measure is
 *  *not* the same as in a 4/4 measure.
 *
 *  A candidate for moving to a zoomer class.
 */

inline int
default_pulses_per_measure (int ppq, int bpb = 4)
{
    return ppq * bpb;
}

/**
 *  Factors in the number of beats in a measure.
 *
 *  A candidate for moving to a zoomer class.
 */

inline int
pulses_per_measure (int ppq, int bpb = 4, int bw = 4)
{
    return (bw > 0) ? 4 * ppq * bpb / bw : ppq * bpb ;
}

/**
 *  Calculates the number of pulses in a quarter beat, with an adjustment
 *  for 120 and 240 PPQN.
 *
 *  A candidate for moving to a zoomer class.
 */

inline int
pulses_per_quarter_beat (int ppq, int bpb = 4, int bw = 4)
{
    return (bw > 0) ? ppq * bpb / bw : ppq ;
}

/**
 *  Calculates the pulses in a beat. For a 4/4 time signature, this is the
 *  same as PPQN.
 *
 *  Now, pulses/beat should be the same as qn/beat x pulse/qn. So we do not
 *  need the number of beats, just the beatwidth. This is wrong:
 *
 *          return ppq * bpb / bw;
 *
 *  Used only in the metro class.
 */

inline int
pulses_per_beat (int ppq, int beatspm = 4, int beatwidth = 4)
{
    return beatspm * ppq / beatwidth;
}

/*
 * Defined in the cpp file.
 *
 *      int pulses_per_substep (midipulse ppq, int zoom)
 *      int pulses_per_pixel (midipulse ppq, int zoom = 2)
 */

/**
 *  Calculates the length of an integral number of measures, in ticks.
 *  This function is called in seqedit::apply_length(), when the user
 *  selects a sequence length in measures.  That function calculates the
 *  length in ticks.  The number of pulses is given by the number of quarter
 *  notes times the pulses per quarter note.  The number of quarter notes is
 *  given by the measures times the quarter notes per measure.  The quarter
 *  notes per measure is given by the beats per measure times 4 divided by
 *  beat_width beats.  So:
 *
\verbatim
    p = 4 * P * M * B / W
        p == pulse count (ticks or pulses)
        M == number of measures
        B == beats per measure (constant)
        P == pulses per quarter-note (constant)
        W == beat width in beats per measure (constant)
\endverbatim
 *
 *  Testing the units to make sure they cancel out to "pulses":
 *
\verbatim
                4   qn      pulses     beats
    p pulses = --- ---- x P ------ x B ----- x M bars
                W  beat       qn        bar
\endverbatim
 *
 *  For our "b4uacuse" MIDI file, M can be about 100 measures, B is 4,
 *  P can be 192 (but we want to support higher values), and W is 4.
 *  So p = 100 * 4 * 4 * 192 / 4 = 76800 ticks.
 *
 *  Note that 4 * P is a constraint encapsulated by the inline function
 *  default_pulses_per_measure() using the default value of beats.
 *  Also note that 4 / W is calculable using qn_per_beat().
 *
 * \param bpb
 *      The B value in the equation, beats/measure or beats/bar.
 *
 * \param ppq
 *      The P value in the equation, pulses/qn.
 *
 * \param bw
 *      The W value in the equation, the denominator of the time signature.
 *      If this value is 0, we'll get an arithmetic exception (crash), so we
 *      just return 0 in this case. The quantity 4 / W is in units of
 *      quarter-notes/beat. So a beat-width of 8 is 1/2 qn/beat, or
 *      an eighth note.
 *
 * \param measures
 *      The M value in the equation.  It defaults to 1, in case one desires a
 *      simple "ticks per measure" number.
 *
 * \return
 *      Returns the L value (ticks or pulses) as calculated via the given
 *      equation.  If bw is 0, then 0 is returned.
 */

inline midi::pulse
measures_to_ticks (int bpb, midi::ppqn ppq, int bw, int measures = 1)
{
    return (bw > 0) ? midi::pulse(4 * ppq * measures * bpb / bw) : 0 ;
}

/**
 *  The inverse of measures_to_ticks(). Note that callers who want to
 *  display the measure number to a user should add 1 to it.
 *
 *  Compare this function to pulses_to_measures(), which returns a double.
 *
 * \param B
 *      The B value in the equation, beats/measure or beats/bar.
 *
 * \param P
 *      The P value in the equation, pulses/qn.
 *
 * \param W
 *      The W value in the equation, the denominator of the time signature,
 *      the beat-width.  If this value is 0, we'll get an arithmetic exception
 *      (crash), so we just return 0 in this case.
 *
 * \param p
 *      The p (pulses) value in the equation.
 *
 * \return
 *      Returns the M value (measures or bars) as calculated via the inverse
 *      equation.  If P or B are 0, then 0 is returned.
 */

inline int
ticks_to_measures (midi::pulse p, int P, int B, int W)
{
    return (P > 0 && B > 0.0) ? (p * W) / (4.0 * P * B) : 0 ;
}

inline int
ticks_to_beats (midi::pulse p, int P, int B, int W)
{
    return (P > 0 && B > 0.0) ? ((p * W / P / 4 ) % B) : 0 ;
}

template <typename INTTYPE>
INTTYPE snapped (snapper snaptype, int S, INTTYPE p)
{
    INTTYPE result = 0;
    if (p > 0 && S > 0)
    {
        INTTYPE snap = INTTYPE(S);
        INTTYPE p0 = p - (p % snap);          /* drop down to a snap      */
        if (snaptype == snapper::down)
        {
            return p0;
        }
        else if (snaptype == snapper::up)
        {
            return p0 + snap;
        }
        else
        {
            INTTYPE p1 = p0 + snap;             /* go up by one snap        */
            int deltalo = int(p - p0);          /* amount to lower snap     */
            int deltahi = int(p1 - p);          /* amount to upper snap     */
            result = deltalo <= deltahi ? p0 : p1 ; /* use closest one      */
        }
    }
    return result;
}

/*------------------------------------------------------------------------
 * Free functions in the midi namespace.
 *------------------------------------------------------------------------*/

inline int
pitch_value_absolute (midi::byte d0, midi::byte d1)
{
    return int(d1) * 128 + int(d0);     /* absolute range is 0 to 16383     */
}

inline int
pitch_value (midi::byte d0, midi::byte d1)
{
    return pitch_value_absolute(d0, d1) - 8192;
}

inline int
pitch_value_scaled (midi::byte d0, midi::byte d1)
{
    return pitch_value(d0, d1) / 128;
}

extern int byte_to_int (midi::byte b);
extern midi::byte int_to_byte (int v);
extern std::string wave_type_name (waveform wv);
extern int extract_timing_numbers
(
    const std::string & s,
    std::string & part_1,
    std::string & part_2,
    std::string & part_3,
    std::string & fraction
);
extern int tokenize_string
(
    const std::string & source,
    lib66::tokenization & tokens
);
extern std::string pulses_to_string (midi::pulse p);
extern std::string pulses_to_measurestring
(
    midi::pulse p,
    const timing & seqparms
);
extern bool pulses_to_midi_measures
(
    midi::pulse p,
    const timing & seqparms,
    measures & measures
);
extern double pulses_to_measures
(
    midi::pulse p, int P, int B, int W
);
extern std::string pulses_to_time_string
(
    midi::pulse p,
    const timing & timinginfo
);
extern std::string pulses_to_time_string
(
    midi::pulse pulses, midi::bpm b, midi::ppqn ppq, bool showus = true
);
extern int pulses_to_hours (midi::pulse pulses, midi::bpm b, int ppq);
extern midi::pulse measurestring_to_pulses
(
    const std::string & measures,
    const timing & seqparms
);
extern midi::pulse midi_measures_to_pulses
(
    const measures & measures,
    const timing & seqparms
);
extern midi::pulse timestring_to_pulses
(
    const std::string & timestring,
    midi::bpm bp, midi::ppqn ppq
);
extern midi::pulse string_to_pulses
(
    const std::string & s,
    const timing & mt,
    bool timestring = false
);

/*
 * These functions are candidate for moving to a zoomer class, as in Seq66.
 */

extern int pulses_per_substep (midi::pulse ppq, int zoom = 1);
extern int pulses_per_pixel (midi::pulse ppq, int zoom = 1);
extern double pitch_value_semitones (midi::byte d0, midi::byte d1);
extern double wave_func (double angle, waveform wavetype);
extern midi::pulse closest_snap (int S, midi::pulse p);
extern midi::pulse down_snap (int S, midi::pulse p);
extern midi::pulse up_snap (int S, midi::pulse p);

/*
 * Randomisation of MIDI values like velocity or ticks.
 */

extern int randomize (int range, int seed = 0);

#if defined RTL66_USE_UNIFORM_INT_DISTRIBUTION
extern int randomize_uniformly (int range, int seed = -1);
#endif

extern bool is_power_of_2 (int value);
extern int log2_of_power_of_2 (int tsd);
extern int beat_power_of_2 (int logbase2);
extern int previous_power_of_2 (int value);
extern int next_power_of_2 (int value);
extern int power (int base, int exponent);
extern midi::byte beat_log2 (int value);
extern bool tempo_us_to_bytes (midi::bytes & tt, midi::bpm tempo_us);
extern midi::byte tempo_to_note_value (midi::bpm tempo);
extern midi::bpm note_value_to_tempo (midi::byte tempo);
extern midi::bpm fix_tempo (midi::bpm tempo);
extern unsigned short combine_bytes (midi::byte b0, midi::byte b1);
extern double unit_truncation (double angle);
extern double exp_normalize (double angle, bool negate = false);
extern midi::ulong bytes_to_varinum
(
    const midi::bytes & bdata,
    size_t offset = 0
);
extern midi::bytes varinum_to_bytes (midi::ulong v);
extern int varinum_size (long len);
extern std::string get_meta_event_text (const midi::bytes & bdata);
extern bool set_meta_event_text
(
    midi::bytes & bdata,
    const std::string & text
);

}           // namespace midi

#endif      // RTL66_MIDI_CALCULATIONS_HPP

/*
 * calculations.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

