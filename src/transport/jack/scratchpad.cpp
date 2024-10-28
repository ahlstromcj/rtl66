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
 * \file          transport/jack/scratchpad.cpp
 *
 *  This module defines the helper class for using JACK in the performance
 *  mode.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-09-14
 * \updates       2022-12-01
 * \license       GNU GPLv2 or above
 *
 */

#include "transport/jack/scratchpad.hpp"    /* transport::jack::scratchpad  */

/*
 *  All library code in the Seq66 project is in the rtl66 namespace.
 */

namespace transport
{

namespace jack
{

#if defined RTL66_BUILD_JACK

/*
 * -------------------------------------------------------------------------
 *  JACK scratch-pad
 * -------------------------------------------------------------------------
 */

scratchpad::scratchpad () :
    js_current_tick         (0.0),
    js_total_tick           (0.0),
    js_clock_tick           (0.0),
    js_jack_stopped         (false),
    js_dumping              (false),
    js_init_clock           (true),
    js_looping              (false),
    js_playback_mode        (false),
    js_ticks_converted      (0.0),
    js_ticks_delta          (0.0),
    js_ticks_converted_last (0.0),
    js_delta_tick_frac      (0L)
{
    // No other code
}

void
scratchpad::initialize
(
    midi::pulse currenttick,
    bool islooping,
    bool songmode
)
{
    js_current_tick         = double(currenttick);
    js_total_tick           = 0.0;
    js_clock_tick           = 0.0;
    js_jack_stopped         = false;
    js_dumping              = false;
    js_init_clock           = true;
    js_looping              = islooping;
    js_playback_mode        = songmode;
    js_ticks_converted      = 0.0;
    js_ticks_delta          = 0.0;
    js_ticks_converted_last = 0.0;
    js_delta_tick_frac      = 0L;
}

void
scratchpad::set_current_tick (midi::pulse curtick)
{
    double ct = double(curtick);
    js_current_tick = js_total_tick = js_clock_tick = ct;
}

void
scratchpad::set_current_tick_ex (midi::pulse curtick)
{
    double ct = double(curtick);
    js_current_tick = js_total_tick = js_clock_tick =
        js_ticks_converted_last = ct;
}

void
scratchpad::add_delta_tick (midi::pulse deltick)
{
    double dt = double(deltick);
    js_current_tick += dt;
    js_total_tick += dt;
    js_clock_tick += dt;
    js_dumping = true;
}

#endif      // defined RTL66_BUILD_JACK

}           // namespace jack

}           // namespace transport

/*
 * transport/jack/scratchpad.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

