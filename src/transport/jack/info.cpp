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
 * \file          transport/jack/info.cpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2022-09-13
 * \updates       2022-12-01
 * \license       See above.
 *
 *  This class contains information that applies to all JACK MIDI ports.
 *  Also see the midi_jack_data class, which includes a static instance
 *  of the transport::jack::info class.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include <cmath>                        /* std::trunc(double)               */

#include "transport/jack/info.hpp"      /* transport::jack::info class      */

namespace transport
{

namespace jack
{

/**
 *  Default constructor.
 */

info::info () :
    transport::info         (),             /* adds beat, tick info, etc.   */
    m_jack_frame_rate       (1),            /* an impossible value          */
    m_jack_start_frame      (0),
    m_cycle_frame_count     (0),
    m_size_compensation     (0),
    m_cycle_time_us         (0),
    m_jack_frame_factor     (0.0),
    m_use_offset            (false)
{
    // Empty body
}

/**
 *  Emobdies the calculation of the pulse factor reasoned out in the
 *  frame_offset() function below.
 *
 * TODO:
 *
 *      -   The jack_position_t's ticks_per_beat and beats_per_minute are
 *          currently zero if no transport (slave or master).  Then
 *          we need to make sure the beats-per-minute is set from the
 *          tune settings, and the ticks-per-beat is set to PPQN * 10.
 *      -   It would be good to call this function during a settings change as
 *          indicated by calls to the jack_set_buffer_size(frames, arg) or
 *          jack_set_sample_rate(frames, arg) callbacks.
 *
 *  Adjustment number:
 *
 *      The lower this number, the faster the calculated offsets vary.
 *      Not sure why, but using the "adjustment" factor makes playback
 *      better behaved at 2048 and 4096 buffer sizes.  A really low number
 *      (0.1) causes "belays", with glitches at each one.
 *
 *  TODO: Figure out how to handle these configuration items.
 *        bool useoffset = rc().with_jack_transport() && rc().jack_use_offset();
 */

bool
info::recalculate_frame_factor
(
    const jack_position_t & pos,
    jack_nframes_t F
)
{
    bool changed = false;
    if
    (
        (pos.ticks_per_beat > 1.0) &&                       /* sanity check */
        (ticks_per_beat() != pos.ticks_per_beat)
    )
    {
        ticks_per_beat(pos.ticks_per_beat);
        changed = true;
    }
    if
    (
        (pos.beats_per_minute > 1.0) &&                     /* sanity check */
        (beats_per_minute() != pos.beats_per_minute)
    )
    {
        beats_per_minute(pos.beats_per_minute);
        changed = true;
    }
    if (frame_rate() != pos.frame_rate)
    {
        frame_rate(pos.frame_rate);
        changed = true;
    }
    if (changed)
    {
        /*
         * A value of 2 or 3 would represent the ALSA nperiod, which does
         * not affect (purportedly) the calculations.
         */

        const double adjustment = 1.0;                  /* nperiod?         */
        double factor = 600.0 /                         /* seconds/pulse    */
        (
            adjustment * ticks_per_beat() * beats_per_minute()
        );
        double cycletime = double(F) / frame_rate();
        double compensation = double(F) * 0.10 + 0.5;
        bool useoffset = true;                          /* see banner TODO  */
        cycle_frame_count(F);
        cycle_time_us(1000000.0 * cycletime);           /* microsec/cycle   */
        pulse_time_us(1000000.0 * factor);              /* microsec/pulse   */
        frame_factor(frame_rate() * factor);            /* frames/pulse     */
        size_compensation(jack_nframes_t(compensation));
        use_offset(useoffset);
    }
    return changed;
}

/**
 *  Attempts to convert ticks to a frame offset (0 to framect).
 *
 * The problem (issue #100):
 *
 *  Our RtMidi-based process handles output by first pushing the current
 *  playback tick (pulse) and the message bytes to the JACK ringubffer.
 *  This happens at frame f0 and time t1.  In the JACK output callback,
 *  the message data is plucked from the ringbuffer at later frame f1 and time
 *  t1.  There, we need an offset from the current frame to the actual frame, so
 *  that the messages don't jitter to much with large frame sizes.
 *
 * The solution:
 *
 *  -#  We assume that the tick position p is in the same relative location
 *      relative to the current frame when placed into the ringbuffer and when
 *      retrieved from the ring buffer. Actually, this is unlikely, but
 *      all events spacing should still be preserved.
 *  -#  We can use the calculations modules pulse_length_us() function to
 *      get the length of a tick in seconds:  60 / ppqn / bpm. Therefore the
 *      tick time t(p) = p * 60 / ppqn / bpm.
 *  -#  What is the number of frames at at given time?  The duration of each
 *      frame is D = 1 / R. So the time at the end of frame Fn is
 *      T(n) = n / R.
 *
 *  The latency L of a frame is given by the number of frames, F, the sampling
 *  rate R (e.g. 48000), and the number of periods P: L = P * F / R.  Here is a
 *  brief table for 48000, 2 periods:
 *
\verbatim
        Frames      Latency     Qjackctl P=2    Qjackctl P-3
          32        0.667 ms    (doubles them)  (triples them)
          64        1.333 ms
         128        2.667 ms
         256        5.333 ms
         512       10.667 ms
        1024       21.333 ms
        2048       42.667 ms
        4096       85.333 ms
\endverbatim
 *
 *  The jackd arguments from its man apge:
 *
 *      -   --rate:     The sample rate, R.  Defaults 48000.
 *      -   --period:   The number of frames between the process callback
 *                      calls, F.  Must be a power of 2, defaults to 1024. Set
 *                      as low as possible to avoid xruns.
 *      -   --nperiods: The number of periods of playback latency, P. Defaults
 *                      to the minimum, 2.  For USB audio, 3 is recommended.
 *                      This an ALSA engine parameter, but it might contribute
 *                      to the duration of a process callback cycle [proved
 *                      using probe code to calculate microseconds.]
 *
 *  So, given a tick position p, what is the frame offset needed?
 *
\verbatim
        t(p) = 60 * p / ppqn / bpm, but ppqn = ticks_per_beat / 10, so
        t(p) = 60 * p / (Tpb / 10) / bpm
        T(n) = n * P * F / R
        60 * 10 * p / (Tpb * bpm) = n * P * F / R
        n = 60 * 10 * p * R / (Tpb * bpm * P * F)

                                seconds    1     beats    minutes
        frame = pulse * 10 * 60 ------- * ---- * ------ * -------
                                minute    Tpb    ticks     beat
\endverbatim
 *
 * Typical values:
 *
\verbatim
        frame_rate          = 48000
        beats_per_bar       = 4
        beat_type           = 4
        ticks_per_beat      = 1920
        beats_per_minute    = 120
\endverbatim
 *
 * \param F
 *      The current frame count (the jack_nframes framect) parameter to the
 *      callback.
 *
 * \param p
 *      The running timestamp, in pulses, of the event being emitted.
 *
 * \return
 *      Returns a putative offset value to use in reserving the event.
 */

jack_nframes_t
info::frame_offset (jack_nframes_t F, midi::pulse p) const
{
    jack_nframes_t result = frame_estimate(p) + start_frame();
    if (F > 1)
        result = result % F;

    return result;
}

/**
 *  This method is an adaptation of the ttymidi.c module's method.
 *  How it works:
 *
 *  1.  Get the cycle_start frame-number, fc.
 *  2.  bufsize_compensation, b = jack_get_buffer_size() / 10.0 + 0.5)
 *  3.  Read the ringbuffer data into bufc in the old rtmidi way. New
 *      wrinkle format: [data, data_size, frame].  The frame is set this way:
 *      a.  Clock messages. The frame = jack_frame_time().
 *      b.  Other MIDI.  Same.
 *  4.  We have the frame, f, from the data, and the frame-count, F.
 *      f += F - b.
 *  5.  If last_buf_frame > f, f = last_buf_frame else last_buf_frame = f.
 *  6.  If f >= fc offset = f - fc else 0.  If offset > F, offset = F-1
 *
 *  The implementation here currently leaves out Step 5, but seems to work
 *  well anyway. It seems to avoid the delay of an event to a later cyle
 *  at random times.
 */

jack_nframes_t
info::frame_offset
(
    jack_nframes_t fc,                              /* Step 1, cycle-start  */
    jack_nframes_t F,
    midi::pulse p                                   /* Step 3, sort of      */
)
{
    jack_nframes_t result = 0;
    jack_nframes_t b = size_compensation();         /* Step 2.              */
    jack_nframes_t f = frame_estimate(p) + F - b;   /* Step 4.              */
    if (f > fc)
        result = f - fc;

    if (result > F)
        result = F - 1;

    return result;
}

/**
 *  Calculates pulses * frames / pulse to estimate the frame value.
 *  This value is rounded and truncated.
 */

jack_nframes_t
info::frame_estimate (midi::pulse p) const
{
    double temp = double(p) * frame_factor() + 0.5;
    return jack_nframes_t(temp);
}

void
info::cycle_frame
(
    midi::pulse p, jack_nframes_t & cycle, jack_nframes_t & offset
) const
{
    double f = double(p) * frame_factor() + 0.5;        /* frame estimate   */
    double c = f / double(cycle_frame_count());         /* cycle + fraction */
    double fullc = std::trunc(c);
    double fraction = c - fullc;
    cycle = jack_nframes_t(c);                          /* cycle number     */
    offset = jack_nframes_t(fraction * cycle_frame_count());
}

/**
 *  Calculates the putative cycle number, without truncation, because we want
 *  the fractional part.
 *
 * \param f
 *      The current frame number.
 *
 * \param F
 *      The number of frames in a cycle.  (A power of 2.)
 *
 * \return
 *      The cycle number with fractional portion.
 */

double
info::cycle (jack_nframes_t f, jack_nframes_t F) const
{
    return double(f) / double(F);
}

double
info::pulse_cycle (midi::pulse p, jack_nframes_t F) const
{
    return p * frame_factor() / double(F);
}

}           // namespace jack

}           // namespace transport

#endif      // RTL66_BUILD_JACK

/*
 * transport/jack/info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

