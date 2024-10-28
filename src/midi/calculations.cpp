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
 * \file          calculations.cpp
 *
 *  This module declares/defines some utility functions and calculations
 *  needed by this application.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-07
 * \updates       2024-05-09
 * \license       GNU GPLv2 or above
 *
 *  This code was moved from the globals module so that other modules
 *  can include it only if they need it.
 *
 *  To convert the ticks for each MIDI note into a millisecond value to
 *  display the notes visually along a timeline, one needs to use the division
 *  and the tempo to determine the value of an individual tick.  That
 *  conversion looks like:
 *
\verbatim
        1 min    60 sec   1 beat     Z clocks
       ------- * ------ * -------- * -------- = seconds
       X beats   1 min    Y clocks       1
\endverbatim
 *
 *  X is the tempo (beats per minute, or BPM), Y is the division (pulses per
 *  quarter note, PPQN), and Z is the number of clocks from the incoming
 *  event. All of the units cancel out, yielding a value in seconds. The
 *  condensed version of that conversion is therefore:
 *
\verbatim
        (60 * Z) / (X * Y) = seconds
        seconds = 60 * clocks / (bpm * ppqn)
\endverbatim
 *
 *  The value given here in seconds is the number of seconds since the
 *  previous MIDI event, not from the sequence start.  One needs to keep a
 *  running total of this value to construct a coherent sequence.  Especially
 *  important if the MIDI file contains tempo changes.  Leaving the clocks (Z)
 *  out of the equation yields the periodicity of the clock.
 *
 *  The inverse calculation is:
 *
\verbatim
        clocks = seconds * bpm * ppqn / 60
\endverbatim
 *
 * \todo
 *      There are additional user-interface and MIDI scaling variables in the
 *      perfroll module that we need to move here.
 */

#include <cctype>                       /* std::isspace(), std::isdigit()   */
#include <cmath>                        /* std::floor(), std::log()         */
#include <cstdlib>                      /* std::atoi(), std::strtol()       */
#include <cstring>                      /* std::memset()                    */
#include <ctime>                        /* std::strftime()                  */

#include "midi/calculations.hpp"        /* MIDI-related calculations        */
#include "midi/eventcodes.hpp"          /* functions to analyze MIDI bytes  */
#include "util/strfunctions.hpp"        /* util::string_to_long()           */

#if defined SEQ66_USE_UNIFORM_INT_DISTRIBUTION
#include <random>                       /* std::uniform_int_distribution    */
#endif

namespace midi
{

/**
 *  This value represent the smallest horizontal unit in a Sequencer66 grid.
 *  It is the number of pixels in the smallest increment between vertical
 *  lines in the grid.  For a zoom of 2, this number gets doubled.
 */

static const int c_pixels_per_substep = 6;

/**
 *  This value represents the fundamental beats-per-bar.
 */

static const int c_qn_beats = 4;

/**
 *  Taken from rtl66 usrsettings.
 *
 *      static const midi::bpm c_def_beats_per_minute =  120.0;
 *      static const int c_def_bpm_precision    =    0;
 *      static const int c_min_bpm_precision    =    0;
 */

static const midi::bpm c_min_beats_per_minute =    2.0;
static const midi::bpm c_max_beats_per_minute =  600.0;
static const int c_max_bpm_precision          =    2;

/**
 *  Convenience function. We don't want to use seq66::string_to_int()
 *  because that uses a leading "0" or "0x" to determine the base of the
 *  conversions.
 */

static int
strtoi (const std::string & v)
{
    return v.empty() ? 0 : std::atoi(v.c_str());
}

/**
 *  Rigorously convert an unsigned character to an integer as per MIDI.
 */

int
byte_to_int (midi::byte b)
{
    bool negative = (b & 0x80) != 0;
    int result = int(b & 0x7F);
    if (negative)
        result = -result;

    return result;
}

midi::byte
int_to_byte (int v)
{
    midi::byte result = v < 0 ? 0x80 : 0x00;
    result += v & 0x7F;
    return result;
}

/**
 *  Extracts up to 4 numbers from a colon-delimited string.
 *
 *      -   measures : beats : divisions
 *          -   "8" represents solely the number of pulses.  That is, if the
 *              user enters a single number, it is treated as the number of
 *              pulses.
 *          -   "8:1" represents a measure and a beat.
 *          -   "213:4:920"  represents a measure, a beat, and pulses.
 *      -   hours : minutes : seconds . fraction.  We really don't support
 *          this concept at present.  Beware!
 *          -   "2:04:12.14"
 *          -   "0:1:2"
 *
 * \warning
 *      This is not the most efficient implementation you'll ever see.
 *      At some point we will tighten it up.  This function is tested in the
 *      rtl66-tests project, in the "calculations_unit_test" module.
 *      At present this test is surely BROKEN!
 *
 * \param s
 *      Provides the input time string, in measures or time format,
 *      to be processed.
 *
 * \param [out] part_1
 *      The destination reference for the first part of the time.
 *      In some contexts, this number alone is a pulse (ticks) value;
 *      in other contexts, it is a measures value.
 *
 * \param [out] part_2
 *      The destination reference for the second part of the time.
 *
 * \param [out] part_3
 *      The destination reference for the third part of the time.
 *
 * \param [out] fraction
 *      The destination reference for the fractional part of the time.
 *
 * \return
 *      Returns the number of parts provided, ranging from 0 to 4.
 *      If 0, there is an error. If 1, it is assumed to be an single number,
 *      such as 768.
 */

int
extract_timing_numbers
(
    const std::string & s,
    std::string & part_1,
    std::string & part_2,
    std::string & part_3,
    std::string & fraction
)
{
    lib66::tokenization tokens;
    int count = tokenize_string(s, tokens);
    part_1.clear();
    part_2.clear();
    part_3.clear();
    fraction.clear();
    if (count > 0)
        part_1 = tokens[0];

    if (count > 1)
        part_2 = tokens[1];

    if (count > 2)
        part_3 = tokens[2];

    if (count > 3)
        fraction = tokens[3];

    return count;
}

/**
 *  Tokenizes a string using the colon, space, or period as delimiters.  They
 *  are treated equally, and the caller must determine what to do with the
 *  parts.  Here are the steps:
 *
 *      -#  Skip any delimiters found at the beginning.  The position will
 *          either exist, or there will be nothing to parse.
 *      -#  Get to the next delimiter.  This will exist, or not.  Get all
 *          non-delimiters until the next delimiter or the end of the string.
 *      -#  Repeat until no more delimiters exist.
 *
 *  Compare this function to tokenize() in the util/strfunctions module.
 *
 * \param source
 *      The string to be parsed and tokenized.
 *
 * \param tokens
 *      Provides a vector into which to push the tokens.
 *
 * \return
 *      Returns the number of tokens pushed (i.e. the final size of the tokens
 *      vector).
 */

int
tokenize_string
(
    const std::string & source,
    lib66::tokenization & tokens
)
{
    static std::string s_delims = ":. ";
    int result = 0;
    tokens.clear();
    auto pos = source.find_first_not_of(s_delims);
    if (pos != std::string::npos)
    {
        for (;;)
        {
            auto depos = source.find_first_of(s_delims, pos);
            if (depos != std::string::npos)
            {
                tokens.push_back(source.substr(pos, depos - pos));
                pos = source.find_first_not_of(s_delims, depos +1);
                if (pos == std::string::npos)
                    break;
            }
            else
            {
                tokens.push_back(source.substr(pos));
                break;
            }
        }
        result = int(tokens.size());
    }
    return result;
}

/**
 *  Converts MIDI pulses (also known as ticks, clocks, or divisions) into a
 *  string.
 *
 * \param p
 *      The MIDI pulse/tick value to be converted.
 *
 * \return
 *      Returns the string as an unsigned ASCII integer number.
 */

std::string
pulses_to_string (pulse p)
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%lu", (unsigned long)(p));
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
 * \param p
 *      The number of MIDI pulses (clocks, divisions, ticks, you name it) to
 *      be converted.  If the value is c_null_pulse, it is converted
 *      to 0, because callers don't generally worry about such niceties, and
 *      the least we can do is convert illegal measure-strings (like
 *      "000:0:000") to a legal value.
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.  These values
 *      are needed in the calculations.
 *
 * \return
 *      Returns the string, in measures notation, for the absolute pulses that
 *      mark this duration.
 */

std::string
pulses_to_measurestring (pulse p, const timing & seqparms)
{
    measures m;                             /* measures, beats, divisions   */
    char tmp[32];
    if (is_null_pulse(p))
        p = 0;                              /* punt the runt!               */

    pulses_to_midi_measures(p, seqparms, m); /* fill measures struct */
    snprintf
    (
        tmp, sizeof tmp, "%03d:%d:%03d",
        m.bars(), m.beats(), m.divisions()
    );
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
\verbatim
        m = p * W / (4 * P * B)
\endverbatim
 *
 * \param p
 *      Provides the MIDI pulses (as in "pulses per quarter note") that are to
 *      be converted to MIDI measures format.
 *
 * \param seqparms
 *      This small structure provides the beats/measure (B), beat-width (W),
 *      and PPQN (P) that hold for the sequence involved in this calculation.
 *      The beats/minute (T for tempo) value is not needed.
 *
 * \param [out] measures
 *      Provides the current MIDI song time structure to hold the results,
 *      which are the measures, beats, and divisions values for the time of
 *      interest.  Note that the measures and beats are corrected to be re 1,
 *      not 0.
 *
 * \return
 *      Returns true if the calculations were able to be made.  The P, B, and
 *      W values all need to be greater than 0.
 */

bool
pulses_to_midi_measures
(
    pulse p,
    const timing & seqparms,
    measures & bars
)
{
    int W = seqparms.beat_width();
    int P = seqparms.PPQN();
    int B = seqparms.beats_per_measure();
    bool result = (W > 0) && (P > 0) && (B > 0);
    if (result)
    {
        double qnotes = double(c_qn_beats) * B / W; /* Q notes per measure  */
        double measlength = P * qnotes;             /* pulses in a measure  */
        int beatticks = measlength / B;             /* pulses in a beat     */
        int m = int(p / measlength) + 1;            /* measure no. of pulse */
        int metro = 1 + ((p * W / P / c_qn_beats ) % B);
        bars.bars(m);                               /* number of measures   */
        bars.beats(metro);                          /* beats within measure */
        bars.divisions(int(p % beatticks));         /* leftover pulses      */
    }
    return result;
}

/**
 *  Function used in sequence::analyze_time_signatures() to precalculate
 *  the size of each time-signature (sequence::timesig) segment.
 *
 * \param p
 *      Provides either the time in ticks (pulses), or the duration of a
 *      timesig segment.
 *
 * \param P
 *      The PPQN in force for this calculation. Must be greater than 0.
 *
 * \param B
 *      The beats/measure in force for this calculation. Must be greater
 *      than 0.
 *
 * \param W
 *      The beat width in force for this calculation. Must be greater
 *      than 0.
 *
 * \return
 *      If the parameters are valid, returns the measure count or size
 *      as a floating-point value.  The caller is responsible for any
 *      rounding.  If parameters are invalid, 0.0 is returned.
 */

double
pulses_to_measures
(
    midi::pulse p,                                  /* time or duration     */
    int P,                                          /* PPQN                 */
    int B,                                          /* beats per measure    */
    int W                                           /* beat width           */
)
{
    double result = 0.0;                            /* indicates an error   */
    bool ok = (W > 0) && (P > 0) && (B > 0);
    if (ok)
    {
        double qnotes = double(c_qn_beats) * B / W; /* Q notes per measure  */
        double measlength = P * qnotes;             /* pulses/std measure   */
        result = p / measlength;
    }
    return result;
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  See the other pulses_to_time_string()
 *  overload.
 *
 * \param p
 *      Provides the number of ticks, pulses, or divisions in the MIDI
 *      event time.
 *
 * \param timinginfo
 *      Provides the tempo of the song, in beats/minute, and the
 *      pulse-per-quarter-note of the song.
 *
 * \return
 *      Returns the return-value of the other pulses_to_time_string() function.
 */

std::string
pulses_to_time_string (pulse p, const timing & timinginfo)
{
    return pulses_to_time_string
    (
        p, timinginfo.BPM(), timinginfo.PPQN()
    );
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  If the fraction part is 0, then it is
 *  not shown.  Examples:
 *
 *      -   "0:0:0"
 *      -   "0:0:0.102333"
 *      -   "12:3:1"
 *      -   "12:3:1.000001"
 *
 * \param p
 *      Provides the number of ticks, pulses, or divisions in the MIDI
 *      event time.
 *
 * \param bpm
 *      Provides the tempo of the song, in beats/minute.
 *
 * \param ppq
 *      Provides the pulses-per-quarter-note of the song.
 *
 * \param showus
 *      If true (the default), shows the microseconds as well.  Hours are now
 *      shown only if greater than zero.
 *
 * \return
 *      Returns the time-string representation of the pulse (ticks) value.
 */

std::string
pulses_to_time_string
(
    midi::pulse p,
    midi::bpm bp,
    midi::ppqn ppq,
    bool showus
)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bp, ppq);
    int seconds = int(microseconds / 1000000UL);
    int minutes = seconds / 60;
    int hours = seconds / (60 * 60);
    int hoursecs = hours * 60 * 60;
    int minutesecs = minutes * 60;
    minutes -= hours * 60;
    seconds -= hoursecs + minutesecs;

    char tmp[48];
    if (showus)
    {
        microseconds -= (hoursecs + minutesecs + seconds) * 1000000UL;
        microseconds /= 10000L;
        if (hours > 0)
        {
            snprintf
            (
                tmp, sizeof tmp, "%d:%02d:%02d.%02lu",
                hours, minutes, seconds, microseconds
            );
        }
        else
        {
            snprintf
            (
                tmp, sizeof tmp, "%02d:%02d.%02lu",
                minutes, seconds, microseconds
            );
        }
    }
    else
    {
        /*
         * Why the spaces?  It is inconsistent.  But see the
         * timestring_to_pulses() function first.
         */

        if (hours > 0)
        {
            snprintf
            (
                tmp, sizeof tmp, "%d:%02d:%02d   ", hours, minutes, seconds
            );
        }
        else
            snprintf(tmp, sizeof tmp, "%02d:%02d   ", minutes, seconds);
    }
    return std::string(tmp);
}

/**
 *  A handy function for checking for long songs (an hour or more).
 */

int
pulses_to_hours (midi::pulse p, midi::bpm bp, midi::ppqn ppq)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bp, ppq);
    int seconds = int(microseconds / 1000000UL);
    return seconds / (60 * 60);
}

/**
 *  Converts a string that represents "measures:beats:division" (also known
 *  as "B:B:T") to a MIDI pulse/ticks/clock value. Note that, here, "division"
 *  is simply a number of pulses less than a beat.
 *
 *  If the third value (the MIDI pulses or ticks value) is set to the dollar
 *  sign ("$"), then the pulses are set to PPQN-1, as a handy shortcut to
 *  indicate the end of the beat.
 *
 * \warning
 *      If only one number is provided, it is treated in this function like
 *      a measures value, not a pulses value.
 *
 * \param measures
 *      Provides the current MIDI song time in "measures:beats:divisions"
 *      format, where divisions are the MIDI pulses in
 *      "pulses-per-quarter-note".
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the input
 *      string is empty, then 0 is returned.
 */

midi::pulse
measurestring_to_pulses
(
    const std::string & measures,
    const timing & seqparms
)
{
    midi::pulse result = 0;
    if (! measures.empty())
    {
        std::string m, b, d, dummy;
        int valuecount = extract_timing_numbers(measures, m, b, d, dummy);
        if (valuecount >= 1)
        {
            midi::measures meas_values;         /* initializes to 0 in ctor */
            meas_values.bars(strtoi(m));
            if (valuecount > 1)
            {
                meas_values.beats(strtoi(b));
                if (valuecount > 2)
                {
                    if (d == "$")
                        meas_values.divisions(seqparms.PPQN() - 1);
                    else
                        meas_values.divisions(strtoi(d));
                }
            }
            result = midi_measures_to_pulses(meas_values, seqparms);
        }
    }
    return result;
}

/**
 *  Converts a string that represents "measures:beats:division" to a MIDI
 *  pulse/ticks/clock value.
 *
 *  p = 4 * P * m * B / W
 *      p == pulse count (ticks or pulses)
 *      m == number of measures
 *      B == beats per measure (constant)
 *      P == pulses per quarter-note (constant)
 *      W == beat width in beats per measure (constant) [NOT CORRECT]
 *
 *  Note that the 0-pulse MIDI measure is "1:1:0", which means "at the
 *  beginning of the first beat of the first measure, no pulses'.  It is not
 *  "0:0:0" as one might expect.
 *
 *  If we get a 0 for measures or for beats, we
 *  treat them as if they were 1.  It is too easy for the user to mess up.
 *
 *  We should consider clamping the beats to the beat-width value as well.
 *
 *  Example: Current time-signature = 3/16. Then qn_per_beat = 4/16 = 0.25.
 *  For 1 measure and 3 beats, the pulses are p = 1 * 3 * 0.25 * PPQN. If PPQN
 *  is 192, the pulses per beat are 0.25 * PPQN = 48.
 *
 * \param measures
 *      Provides the current MIDI song time structure holding the
 *      measures, beats, and divisions values for the time of interest.
 *      Note that it does not employ beat-width. It is a time position.
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the
 *      pulse-value cannot be calculated, then c_null_pulse is
 *      returned.
 */

midi::pulse
midi_measures_to_pulses
(
    const measures & bars,
    const timing & seqparms
)
{
    midi::pulse result = c_null_pulse;
    int m = bars.bars() - 1;                /* true measure count   */
    int b = bars.beats() - 1;
    if (m < 0)
        m = 0;

    if (b < 0)
        b = 0;

    double qn_per_beat = double(c_qn_beats) / seqparms.beat_width();
    result = 0;
    if (m > 0)
        result += int(m * seqparms.beats_per_measure() * qn_per_beat);

    if (b > 0)
        result += int(b * qn_per_beat);

    result *= seqparms.PPQN();
    result += bars.divisions();
    return result;
}

/**
 *  A new function to create a midi_measures structure from a string assumed
 *  to have a formate of "B:B:T".  Any fractional part is ignored as being
 *  less than a pulse.
 */

measures
string_to_measures (const std::string & bbt)
{
    std::string m;
    std::string b;
    std::string t;
    std::string fraction;
    int count = extract_timing_numbers(bbt, m, b, t, fraction);
    if (count > 0)
    {
        int meas = strtoi(m);
        int beats = strtoi(b);
        int ticks = strtoi(t);
        if (meas == 0)
            meas = 1;

        if (beats == 0)
            beats = 1;

        return measures(meas, beats, ticks);
    }
    else
    {
        static midi::measures s_dummy;
        return s_dummy;
    }
}

/**
 *  Converts a string that represents "hours:minutes:seconds.fraction" into a
 *  MIDI pulse/ticks/clock value.
 *
 * \param timestring
 *      The time value to be converted, which must be of the form
 *      "hh:mm:ss" or "hh:mm:ss.fraction".  That is, all four parts must
 *      be found.
 *
 * \param bpm
 *      The beats-per-minute tempo (e.g. 120) of the current MIDI song.
 *
 * \param ppqn
 *      The parts-per-quarter note precision (e.g. 192) of the current MIDI
 *      song.
 *
 * \return
 *      Returns 0 if an error occurred or if the number actually translated to
 *      0.
 */

midi::pulse
timestring_to_pulses
(
    const std::string & timestring,
    midi::bpm bp, midi::ppqn ppq
)
{
    midi::pulse result = 0;
    if (! timestring.empty())
    {
        std::string sh, sm, ss, us;
        if (extract_timing_numbers(timestring, sh, sm, ss, us) >= 4)
        {
            /**
             * This conversion assumes that the fractional parts of the
             * seconds is padded with zeroes on the left or right to 6 digits.
             */

            int hours = strtoi(sh);
            int minutes = strtoi(sm);
            int seconds = strtoi(ss);
            double secfraction = atof(us.c_str());
            long sec = ((hours * 60) + minutes) * 60 + seconds;
            long microseconds = 1000000 * sec + long(1000000.0 * secfraction);
            double pulses = delta_time_us_to_ticks(microseconds, bp, ppq);
            result = midi::pulse(pulses);
        }
    }
    return result;
}

/**
 *  Converts a time string to pulses.  First, the type of string is deduced by
 *  the characters in the string.  If the string contains two colons and a
 *  decimal point, it is assumed to be a time-string ("hh:mm:ss.frac"); in
 *  addition ss will have to be less than 60. ???  Actually, now we only care if
 *  four numbers are provided.
 *
 *  If the string just contains two colons, then it is assumed to be a
 *  measure-string ("measures:beats:divisions").
 *
 *  If it has none of the above, it is assumed to be pulses.  Testing is not
 *  rigorous.
 *
 * measurestring_to_pulses(): Converts "B:B:T" values to pulses.
 * timestring_to_pulses(): Converts "H:M:S.f" values to pulses.
 *
 * \param s
 *      Provides the string to convert to pulses.
 *
 * \param mt
 *      Provides the structure needed to provide BPM and other values needed
 *      for some of the conversions done by this function.
 *
 * \param timestring
 *      If true, interpret the string as an "H:M:S" string.
 *
 * \return
 *      Returns the string as converted to MIDI pulses (or divisions, clocks,
 *      ticks, whatever you call it).
 */

midi::pulse
string_to_pulses
(
    const std::string & s,
    const timing & mt,
    bool timestring
)
{
    midi::pulse result = 0;
    lib66::tokenization tokens;
    int count = tokenize_string(s, tokens);     /* function in this module  */
    if (count == 1)                             /* no colons in it          */
    {
        result = midi::pulse(util::string_to_long(s));
    }
    else if (count > 1)
    {
        if (timestring)
            result = timestring_to_pulses(s, mt.BPM(), mt.PPQN());
        else
            result = measurestring_to_pulses(s, mt);
    }
    return result;
}

/**
 *  Creates a number with a negative-to-0-to-positive range.
 *
 *  The first call is seeded with the current time, then a pseudo-random
 *  number is returned.  This is a simplistic linear congruence generator. It
 *  returns a random number between -range and + range. [Could consider using
 *  random(3) which has a much longer periodicity.]
 *
 *  Another options is rand() % (2 * range + 1) + (-range + 1), but it uses
 *  only the low-order bits of the number.
 *
 *  C++11 has a default random engine template, but it is a pain!
 *
 *  return rand() / (RAND_MAX / (2 * range + 1) + 1) - range;
 *
 * \param range
 *      The amount of "randomness" desired.
 *
 * \return
 *      Returns a number from -range to +range, uniformly distributed.
 */

int
randomize (int range, int seed)
{
    static bool s_uninitialized = true;
    if (s_uninitialized)
    {
        s_uninitialized = false;
        if (seed == 0)
            seed = time(NULL);

        srand(unsigned(seed));
    }
    if (range != 0)
    {
        if (range < 0)
            range = -range;

        long result = (2 * range * long(rand()) / RAND_MAX) - range;
        return int(result);
    }
    else
        return 0;
}

#if defined SEQ66_USE_UNIFORM_INT_DISTRIBUTION

class randomizer
{
private:

    static const int s_upper_limit = std::numeric_limits<int>::max();

    std::random_device m_rd;    /* seed source for random number engine     */
    std::mt19937 m_mtwister;    /* mersenne_twister_engine, maybe seeded    */
    std::uniform_int_distribution<int> m_distribution;

public:

    randomizer (int seed = -1) :
        m_rd            (),                         /* random device (opt.) */
        m_mtwister      (m_rd()),                   /* internal generator   */
        m_distribution  (0, s_upper_limit)          /* uniform int range    */
    {
        if (seed != (-1))
            m_mtwister.seed(seed);
    }

    int generate ()
    {
        return m_distribution(m_mtwister);
    }

    int generate (int range)
    {
        int rnd = generate();
        long result = 2 * range * long(rnd) / long(s_upper_limit);
        return int(result) - range;
    }

};          // class randomizer

int
randomize_uniformly (int range, int seed)
{
    static bool s_uninitialized = true;
    static randomizer * s_randomizer_pointer = nullptr;
    if (s_uninitialized)
    {
        static randomizer s_randomizer(seed);   /* create it secretly   */
        s_randomizer_pointer = &s_randomizer;   /* point to it          */
        s_uninitialized = false;
    }
    if (range != 0)
    {
        if (range < 0)
            range = -range;

        return s_randomizer_pointer->generate(range);
    }
    else
        return 0;
}

#endif

/**
 *  Returns true if a number is a power of 2.  MIDI's beatwidth values
 *  provide the power of 2 needed for a valid beat width value.
 *  First b in the below expression is for the case when b is 0.
 *
 *  Taken from:
 *
 * https://www.geeksforgeeks.org/c-program-to-find-whether-a-no-is-power-of-two/
 */

bool
is_power_of_2 (int b)
{
    return b && (! (b & (b - 1)));
}

/**
 *  Calculates the log-base-2 value of a number that is already a power of 2.
 *  Useful in converting a time signature's denominator to a Time Signature
 *  meta event's "dd" value.
 *
 * \param tsd
 *      The time signature denominator, which must be a power of 2:  2, 4, 8,
 *      16, or 32.
 *
 * \return
 *      Returns the power of 2 that achieves the \a tsd parameter value.
 */

int
log2_power_of_2 (int tsd)
{
    int result = 0;
    while (tsd > 1)
    {
        ++result;
        tsd >>= 1;
    }
    return result;
}

/**
 *  This function provides the size of the smallest horizontal grid unit in
 *  units of pulses (ticks).  We need this to be able to increment grid
 *  drawing by more than one (time-wasting!) without skipping any lines.
 *
 *  The smallest grid unit in the seqroll is a "sub-step".  The next largest
 *  unit is a "note-step", which is inside a note.  Each note contains PPQN
 *  ticks.
 *
 *  Current status at PPQN = 192, Base pixels = 6:
 *
\verbatim
    Zoom    Note-steps  Substeps   Substeps/Note (#SS)
      1         4           8           32
      2         4           4           16
      4         4           2            8
      8         4           0            4
     16         2           0            2
     32         0           0            1
     64        1/2          0           1/2
    128         0           0           1/4
\endverbatim
 *
 *  Pulses-per-substep is given by PPSS = PPQN / #SS, where #SS = 32 / Zoom.
 *  But 32 is the base PPQN (192) divided by the base pixels (6). In the end,
 *
 *      PPSS = (PPQN * Zoom * Base Pixels) / Base PPQN
 *
 *  Currently the Base values are hardwired (see usrsettings).  The base
 *  pixels value is c_pixels_per_substep = 6, and the base PPQN is
 *  usr().base_ppqn() = 192.  The numerator of this equation is well within
 *  the limit of a 32-bit integer.
 *
 * \param ppqn
 *      Provides the actual PPQN used by the currently-loaded tune.
 *
 * \param zoom
 *      Provides the current zoom value.  Defaults to 1, but is normally
 *      another value.
 *
 * \return
 *      The result of the above equation is returned.
 */

static const midi::pulse s_usr_base_ppqn = 192;     // TODO

int
pulses_per_substep (midi::pulse ppq, int zoom)
{
    return int((ppq * zoom * c_pixels_per_substep) / s_usr_base_ppqn);
}

/**
 *  Similar to pulses_per_substep(), but for a single pixel.  Actually, what
 *  this function does is scale the PPQN against usr().base_ppqn() (192).
 *
 * \param ppq
 *      Provides the actual PPQN used by the currently-loaded tune.
 *
 * \param zoom
 *      Provides the current zoom value.  Defaults to 1, which can be used
 *      to simply get the ratio between the actual PPQN, but only when PPQN >=
 *      usr().base_ppqn().
 *
 * \return
 *      The result of the above equation is returned.
 */

int
pulses_per_pixel (midi::pulse ppq, int zoom)
{
    midi::pulse result = (ppq * zoom) / s_usr_base_ppqn;
    if (result == 0)
        result = 1;

    return result;
}

/**
 *  Internal function for simple calculation of a power of 2 without a lot of
 *  math.  Use for calculating the denominator of a time signature.
 *
 * \param logbase2
 *      Provides the power to which 2 is to be raised.  This integer is
 *      probably only rarely greater than 5 (which represents a denominator of
 *      32).
 *
 * \return
 *      Returns 2 raised to the logbase2 power.
 */

int
beat_power_of_2 (int logbase2)
{
    int result;
    if (logbase2 == 0)
    {
        result = 1;
    }
    else
    {
        result = 2;
        for (int c = 1; c < logbase2; ++c)
            result *= 2;
    }
    return result;
}

/**
 *  Calculates positive integer powers.
 *
 * \param base
 *      The number to be raised to the power.
 *
 * \param exponent
 *      The power to be applied.  Only 0 or above are accepted.  However,
 *      there is currently no check for integer overflow of the result. This
 *      function is meant for reasonably small powers.
 *
 * \return
 *      Returns the power.  If the exponent is illegal, the 0 is returned.
 */

int
power (int base, int exponent)
{
    int result = 0;
    if (exponent > 1)
    {
        result = base;
        for (int p = exponent; p > 1; --p)
            result *= base;
    }
    else if (exponent == 1)
        result = base;
    else if (exponent == 0)
        result = 1;

    return result;
}

/**
 *  Calculates the base-2 log of a number. This number is truncated to an
 *  integer byte value, as it is used in calculating values to be written to a
 *  MIDI file.
 *
 * \param value
 *      The integer value for which log2(value) is needed.
 *
 * \return
 *      Returns log2(value).
 */

midi::byte
beat_log2 (int value)
{
    return midi::byte(std::log(double(value)) / std::log(2.0));
}

/**
 *  Calculates the tempo in microseconds from the bytes read from a Tempo
 *  event in the MIDI file.
 *
 * \param tt
 *      Provides the 3-byte vector of values making up the raw tempo data.
 *
 * \return
 *      Returns the result of converting the bytes to a double value.
 */

midi::bpm
tempo_us_from_bytes (const midi::bytes & tt)
{
    midi::bpm result = midi::bpm(tt[0]);
    result = (result * 256) + midi::bpm(tt[1]);
    result = (result * 256) + midi::bpm(tt[2]);
    return result;
}

/**
 *  Provide a way to convert a tempo value (microseconds per quarter note)
 *  into the three bytes needed as value in a Tempo meta event.  Recall the
 *  format of a Tempo event:
 *
 *  0 FF 51 03 t2 t1 t0 (tempo as number of microseconds per quarter note)
 *
 *  This code is the inverse of the lines of code around line 768 in
 *  midifile.cpp, which is basically
 *  <code> ((t2 * 256) + t1) * 256 + t0 </code>.
 *
 *  As a test case, note that the default tempo is 120 beats/minute, which is
 *  equivalent to tttttt=500000 (0x07A120).  The output of this function will
 *  be t[] = { 0x07, 0xa1, 0x20 } [the indices go 0, 1, 2].
 *
 * \param t
 *      Provides a small array of 3 elements to hold each tempo byte.
 *
 * \param tempo_us
 *      Provides the temp value in microseconds per quarter note.  This is
 *      always an integer, not a double, so do not get confused here.
 */

bool
tempo_us_to_bytes (midi::bytes & tt, midi::bpm tempo_us)
{
    bool result = tempo_us > 0.0;
    tt.clear();
    if (result)
    {
        int temp = int(tempo_us + 0.5);
        tt.push_back(midi::byte((temp & 0xFF0000) >> 16));
        tt.push_back(midi::byte((temp & 0x00FF00) >> 8));
        tt.push_back(midi::byte(temp & 0x0000FF));
    }
    else
    {
        tt.push_back(0);
        tt.push_back(0);
        tt.push_back(0);
    }
    return result;
}

/**
 *  Converts a tempo value to a MIDI note value for the purpose of displaying
 *  a tempo value in the mainwnd, seqdata section (hopefully!), and the
 *  perfroll.  It implements the following linear equation, with clamping just
 *  in case.
 *
\verbatim
                           N1 - N0
        N = N0 + (B - B0) ---------     where (N1 - N0) is always 127
                           B1 - B0
\endverbatim
 *
\verbatim
                        127
        N = (B - B0) ---------
                      B1 - B0
\endverbatim
 *
 *  where N0 = 0 (MIDI note 0 is the minimum), N1 = 127 (the maximum MIDI
 *  note), B0 is the value of c_min_beats_per_minute) B1 is the value of
 *  usr().midi_bpm_maximum(), B is the input beats/minute, and N is the
 *  resulting note value.  As a precaution due to rounding error, we clamp the
 *  values between 0 and 127.
 *
 * \param tempovalue
 *      The tempo in beats/minute.
 *
 * \return
 *      Returns the tempo value scaled to the range 0 to 127, based on the
 *      configured BPM minimum and maximum values.
 */

midi::byte
tempo_to_note_value (midi::bpm tempovalue)
{
    double slope = double(max_midi_value());
    slope /= c_max_beats_per_minute - c_min_beats_per_minute;

    int note = int(slope * (tempovalue - c_min_beats_per_minute) + 0.5);
    return clamp_midi_value(note);
}

/**
 *  The inverse of tempo_to_note_value().
 *
 * \param note
 *      The note value used for displaying the tempo in the seqdata pane, the
 *      perfroll, and in a mainwnd slot.
 *
 * \return
 *      Returns the tempo in beats/minute.
 */

midi::bpm
note_value_to_tempo (midi::byte note)
{
    double result = c_max_beats_per_minute - c_min_beats_per_minute;
    result *= double(note);
    result /= double(max_midi_value());
    result += c_min_beats_per_minute;
    return result;
}

/**
 *  Fixes the tempo value, truncating it to the number of digits of tempo
 *  precision (0, 1, or 2) specified by "bpm_precision" in the "usr" file.
 *
 * \param bpm
 *      The uncorrected BPM value.
 *
 * \return
 *      Returns the BPM truncated to the desired precision.
 */

midi::bpm
fix_tempo (midi::bpm bp)
{
    int precision = c_max_bpm_precision;    /* 0/1/2 digits past decimal    */
    if (precision > 0)
    {
        bp *= 10.0;
        if (precision == 2)
            bp *= 10.0;
    }
    bp = trunc(bp);
    if (precision > 0)
    {
        bp /= 10.0;
        if (precision == 2)
            bp /= 10.0;
    }
    return bp;
}

/**
 *  Combines bytes into an unsigned-short value.
 *
 *  http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/wheel.htm
 *
 *  Two data bytes follow the status. The two bytes should be combined
 *  together to form a 14-bit value. The first data byte's bits 0 to 6 are
 *  bits 0 to 6 of the 14-bit value. The second data byte's bits 0 to 6 are
 *  really bits 7 to 13 of the 14-bit value. In other words, assuming that a
 *  C program has the first byte in the variable First and the second data
 *  byte in the variable Second, here's how to combine them into a 14-bit
 *  value (actually 16-bit since most computer CPUs deal with 16-bit, not
 *  14-bit, integers).
 *
 *  I think Kepler34 got the bytes backward.
 *
 * \param b0
 *      The first byte to be combined.
 *
 * \param b1
 *      The second byte to be combined.
 *
 * \return
 *      Returns the bytes basically OR'd together.
 */

unsigned short
combine_bytes (midi::byte b0, midi::byte b1)
{
   unsigned short short_14bit = (unsigned short)(b1);
   short_14bit <<= 7;
   short_14bit |= (unsigned short)(b0);
   return short_14bit * 48;
}

/**
 *  Converts a double value to range from 0.0 to 1.0. That is, it returns the
 *  fractional portion.  For example, 4.145 would become 0.145.
 */

double
unit_truncation (double angle)
{
    double result = angle;
    if (result > 1.0)
    {
        double truncated = trunc(result);
        result -= truncated;
    }
    return result;
}

/**
 *  This function maps byte values from 0 to 127 to the range of
 *  approximately e^(-2.436) to e^(+2.436), which is 0.884 to 11.314.
 *  These values convert nicely to a range of 11.314 / 0.0884 = 128.
 *
 *  So we take the angle A:
 *
 *  -#  Convert A via truncation so it ranges from 0.0 to 1.0.  Call it T.
 *  -#  Re-map A to range from Emin = -2.436 to Emax = 2.436. Call it A'.
 *  -#  Take e to the power A'.
 *  -#  Rescale the result to the range of 0.0 to 1.0, for use in the LFO
 *      generator.
 *
 *  The re-mapping equations is y = mx + b, where the y-intercept is easily
 *  seen to be b = Emin, and the slope is m = Emax - Emin = range.
 *
 *  For a reverse exponential, we simply negate the exponent
 */

double
exp_normalize (double angle, bool negate)
{
    static const double s_range = log(double(max_midi_value())); /* 4.852 */
    static const double s_exp_max = s_range / 2.0;               /* +2.42 */
    static const double s_exp_min = -s_exp_max;                  /* -2.42 */
    static const double s_scaler = exp(s_exp_min);
    double T = unit_truncation(angle);
    double Aprime = s_range * T + s_exp_min;
    if (negate)
        Aprime = -Aprime;

    double result = exp(Aprime);
    result *= s_scaler;
    return result;
}

/**
 *  Converts an array containing a MIDI Variable-Length Value (VLV),
 *  which has a variable number of bytes.  This function gets the bytes
 *  while bit 7 is set in each byte.  Bit 7 is a continuation bit.
 *  See varinum_to_bytes() for more information.
 *
 * \param bdata
 *      A vector of bytes. The varinum data is assumed to start at bdata[0].
 *
 * \param offset
 *      A starting-offset parameter; bytes are skipped until this offset
 *      is reached.
 *
 * \return
 *      Returns the accumulated values as a single number.
 */

midi::ulong
bytes_to_varinum (const midi::bytes & bdata, size_t offset)
{
    midi::ulong result = 0;
    size_t count = 0;
    for (auto c : bdata)
    {
        if (count >= offset)
        {
            result <<= 7;                           /* shift result 7 bits  */
            result += c & 0x7F;                     /* add base-128 "digit" */
            if ((c & 0x80) == 0x00)                 /* no bit 7 set, done   */
                break;
        }
        ++count;
    }
    return result;
}

/**
 *  Writes a MIDI Variable-Length Value (VLV), which has a variable number
 *  of bytes. In MIDI, this value is used for encoding delta times and
 *  data lengths. It is essentially a string of base-128 values.
 *
 *  A MIDI file Variable Length Value is stored in bytes. Each byte has
 *  two parts: 7 bits of data and 1 continuation bit. The highest-order
 *  bit is set to 1 if there is another byte of the number to follow. The
 *  highest-order bit is set to 0 if this byte is the last byte in the
 *  VLV.
 *
 *  To recreate a number represented by a VLV, first you remove the
 *  continuation bit and then concatenate the leftover bits into a single
 *  number.
 *
 *  To generate a VLV from a given number, break the number up into 7 bit
 *  units and then apply the correct continuation bit to each byte.
 *  Basically this is a base-128 system where the digits range from 0 to 7F.
 *
 *  In theory, you could have a very long VLV number which was quite
 *  large; however, in the standard MIDI file specification, the maximum
 *  length of a VLV value is 5 bytes, and the number it represents can not
 *  be larger than 4 bytes.
 *
 *  Here are some common cases:
 *
 *      -   Numbers between 0 and 127 are represented by a single byte:
 *          0x00 to 7F.
 *      -   0x80 is represented as 0x81 00.  The first number is
 *          10000001, with the left bit being the continuation bit.
 *          The rest of the number is multiplied by 128, and the second byte
 *          (0) is added to that. So 0x82 would be 0x81 0x01
 *      -   The largest 2-byte MIDI value (e.g. a sequence number) is
 *          0xFF 7F, which is 127 * 128 + 127 = 16383 = 0x3FFF.
 *      -   The largest 3-byte MIDI value is 0xFF FF 7F = 2097151 = 0x1FFFFF.
 *      -   The largest number, 4 bytes, is 0xFF FF FF 7F = 536870911 =
 *          0xFFFFFFF.
 *
 *  Also see the varinum_size() and midi::track_base::add_variable() functions.
 *  This function masks off the lower 8 bits of the long parameter, then
 *  shifts it right 7, and, if there are still set bits, it encodes it into
 *  the buffer in reverse order.  This function "replaces"
 *  sequence::put_list_var().  It is almost identical to midifile ::
 *  write_varinum() from Seq66.
 *
 * \param v
 *      The data value to be added to the current event in the MIDI container.
 */

midi::bytes
varinum_to_bytes (midi::ulong v)
{
    midi::bytes result;
    midi::ulong buffer = v & 0x7F;                  /* mask a no-sign byte  */
    while (v >>= 7)                                 /* shift right, test    */
    {
        buffer <<= 8;                               /* move LSB bits to MSB */
        buffer |= ((v & 0x7F) | 0x80);              /* add LSB, set bit 7   */
    }
    for (;;)
    {
        result.push_back(midi::byte(buffer) & 0xFF);    /* add the LSB      */
        if (buffer & 0x80)                              /* if bit 7 set     */
            buffer >>= 8;                               /* get next MSB     */
        else
            break;
    }
    return result;
}

/**
 *  Calculates the length of a variable length value (VLV).  This function is
 *  needed when calculating the length of a track.  Note that it handles
 *  only the following situations:
 *
 *      https://en.wikipedia.org/wiki/Variable-length_quantity
 *
 *  This restriction allows the calculation to be simple and fast.
 *
\verbatim
       1 byte:  0x00 to 0x7F
       2 bytes: 0x80 to 0x3FFF
       3 bytes: 0x4000 to 0x001FFFFF
       4 bytes: 0x200000 to 0x0FFFFFFF
\endverbatim
 *
 * \param len
 *      The long value whose length, when encoded as a MIDI varinum, is to be
 *      found.
 *
 * \return
 *      Returns values as noted above.  Anything beyond that range returns
 *      0.
 */

int
varinum_size (long len)
{
    int result = 0;
    if (len >= 0x00 && len < 0x80)
        result = 1;
    else if (len >= 0x80 && len < 0x4000)
        result = 2;
    else if (len >= 0x4000 && len < 0x200000)
        result = 3;
    else if (len >= 0x200000 && len < 0x10000000)
        result = 4;

    return result;
}

/**
 *  An internal helper function to test if the data provided is
 *  a meta text message.
 */

static bool
check_metatext (const midi::bytes & bdata)
{
    static size_t s_min_metatext_size = 4;      /* FF xx len onecharacter   */
    bool result =
    (
        bdata.size() >= s_min_metatext_size &&
        midi::is_meta(bdata[0]) &&
        midi::is_meta_text_msg(bdata[1])
    );
    return result;
}

/**
 *  Extracts a string from a text event, with some error-checking:
 *
 *      -   Is the message long enough to have all 4 components?
 *      -   Is it a meta message?
 *      -   Is it a meta text message?
 *      -   How many bytes is the variable-length value?
 */

std::string
get_meta_event_text (const midi::bytes & bdata)
{
    static size_t s_length_offset = 2;          /* FF nn len ...            */
    std::string result;
    if (check_metatext(bdata))
    {
        size_t len = size_t(bytes_to_varinum(bdata, s_length_offset));
        size_t textoffset = varinum_size(long(len)) + s_length_offset;
        for (size_t i = 0; i < len; ++i, ++textoffset)
        {
            char c = static_cast<char>(bdata[textoffset]);
            result.push_back(c);
        }
    }
    return result;
}

/**
 *  Changes the text in a byte vector reflecting an existing event.  The data
 *  must exist; this function is meant only to modify the data.
 *
 *          FF xx len characters
 */

bool
set_meta_event_text (midi::bytes & bdata, const std::string & text)
{
    bool result = check_metatext(bdata);
    if (result)
    {
        midi::byte metatype = bdata[1];     /* to restore later */
        midi::bytes lenbytes = varinum_to_bytes(midi::ulong(text.length()));
        bdata.clear();
        bdata.push_back(midi::to_byte(midi::status::meta_msg)); /* 0xFF */
        bdata.push_back(metatype);
        for (auto c : lenbytes)
            bdata.push_back(c);

        for (auto c : text)
        {
            midi::byte b = static_cast<midi::byte>(c);
            bdata.push_back(b);
        }
    }
    return result;
}

}       // namespace midi

/*
 * calculations.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

