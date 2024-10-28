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
 * \file          transport/info.cpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2022-11-10
 * \updates       2022-12-01
 * \license       See above.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 */

#include "midi/calculations.hpp"        /* ticks_to_delta_time_us() etc.    */
#include "transport/info.hpp"           /* transport::info class            */

namespace transport
{

/**
 * \ctor info
 */

info::info () :
    m_timebase              (timebase::none),
    m_is_running            (false),
    m_beat_width            (4),
    m_beats_per_bar         (4),
    m_beats_per_minute      (120.0),
    m_ppqn                  (192),
    m_resolution_change     (true),
    m_ticks_per_beat        (192),
    m_pulse_time_us         (0),
    m_clocks_per_metronome  (24),
    m_32nds_per_quarter     (8),
    m_us_per_quarter_note   (midi::tempo_us_from_bpm(m_beats_per_minute)),
    m_one_measure           (0),
    m_reposition            (false),
    m_start_tick            (0),
    m_tick                  (0),
    m_left_tick             (0),
    m_right_tick            (0),
    m_looping               (false)
{
    // Empty body
}

info::info
(
    int beatwidth,
    int beatsperbar,
    midi::bpm beatspermin,
    midi::ppqn ppq
) :
    m_timebase              (timebase::none),
    m_is_running            (false),
    m_beat_width            (beatwidth),
    m_beats_per_bar         (beatsperbar),
    m_beats_per_minute      (beatspermin),
    m_ppqn                  (ppq),
    m_resolution_change     (true),
    m_ticks_per_beat        (ppq),
    m_pulse_time_us         (0),
    m_clocks_per_metronome  (24),
    m_32nds_per_quarter     (8),
    m_us_per_quarter_note   (midi::tempo_us_from_bpm(m_beats_per_minute)),
    m_one_measure           (0),
    m_reposition            (false),
    m_start_tick            (0),
    m_tick                  (0),
    m_left_tick             (0),
    m_right_tick            (0),
    m_looping               (false)
{
    // Empty body
}

unsigned
info::delta_time_ms (midi::pulse p) const
{
    unsigned result = 0;
    double tpb = ticks_per_beat();
    double bpmin = beats_per_minute();
    if (tpb > 10.0 && bpmin > 1.0)
    {
        double ms = 0.001 * midi::ticks_to_delta_time_us(p, tpb / 10.0, bpmin);
        result = unsigned(ms + 0.5);
    }
    return result;
}

void
info::resolution_change_management
(
    midi::bpm bpmfactor, midi::ppqn ppq,
    int & bpm_times_ppqn,
    double & dct,
    double & pus
)
{
    if (resolution_change())
    {
        bpm_times_ppqn = bpmfactor * ppq;
        dct = midi::double_ticks_from_ppqn(ppq);
        pus = midi::pulse_length_us(bpmfactor, ppq);
        m_resolution_change = false;
    }
}

/**
 *  Set the left marker at the given tick.  We let the caller determine if
 *  this setting is a modification.  If the left tick is later than the right
 *  tick, the right tick is move to one measure past the left tick.
 *
 * \todo
 *      The player::m_one_measure member is currently hardwired to PPQN*4.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the left tick.  If the left
 *      tick is greater than or equal to the right tick, then the right ticked
 *      is moved forward by one "measure's length" (PPQN * 4) past the left
 *      tick.
 */

void
info::left_tick (midi::pulse t)
{
    m_start_tick = m_left_tick = t;
    m_reposition = false;
//  if (is_jack_master())                       /* don't use in slave mode  */
//  {
//      position_jack(true, t);
//      tick(t);
//  }
//  else if (! is_jack_running())
//      tick(t);

    if (m_left_tick >= m_right_tick)
        m_right_tick = m_left_tick + m_one_measure;
}

midi::pulse
info::left_tick_snap (midi::pulse t, midi::pulse snap)
{
    midi::pulse remainder = t % snap;
    if (remainder > (snap / 2))
        t += snap - remainder;               /* move up to next snap     */
    else
        t -= remainder;                      /* move down to next snap   */

    if (m_right_tick <= t)
        right_tick_snap(t + 4 * snap, snap);

    m_left_tick = t;
    start_tick(t);
    m_reposition = false;
//  if (is_jack_master())                       /* don't use in slave mode  */
//      position_jack(true, tick);
//  else if (! is_jack_running())
//      set_tick(tick);

    return t;
}

/**
 *  Set the right marker at the given tick.  This setting is made only if the
 *  tick parameter is at or beyond the first measure.  We let the caller
 *  determine is this setting is a modification.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the right tick.  If less than
 *      or equal to the left tick setting, then the left tick is backed up by
 *      one "measure's worth" (PPQN * 4) worth of ticks from the new right
 *      tick.
 *
 *
 */

void
info::right_tick (midi::pulse t)
{
    if (t == 0)
        t = m_one_measure;

    if (t >= m_one_measure)
    {
        m_right_tick = t;
        if (m_right_tick <= m_left_tick)
        {
            m_left_tick = m_right_tick - m_one_measure;
            start_tick(m_left_tick);
            m_reposition = false;
//          if (is_jack_master())
//              position_jack(true, m_left_tick);
//          else
//              set_tick(m_left_tick);
        }
    }
}

midi::pulse
info::right_tick_snap (midi::pulse t, midi::pulse snap)
{
    midi::pulse remainder = t % snap;
    if (remainder > (snap / 2))
        t += snap - remainder;               /* move up to next snap     */
    else
        t -= remainder;                      /* move down to next snap   */

    if (t > m_left_tick)
    {
        m_right_tick = t;
        start_tick(m_left_tick);
        m_reposition = false;
//      if (is_jack_master())
//          position_jack(true, m_left_tick);
//      else
//          set_tick(m_left_tick);
    }
    return m_left_tick;
}

}           // namespace transport

/*
 * transport/info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

