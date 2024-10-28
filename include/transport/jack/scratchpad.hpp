#if ! defined RTL66_TRANSPORT_JACK_SCRATCHPAD_HPP
#define RTL66_TRANSPORT_JACK_SCRATCHPAD_HPP

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
 * \file          transport/jack/scratchpad.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of playering (playing) a full MIDI song using JACK.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-07-23
 * \updates       2022-12-01
 * \license       GNU GPLv2 or above
 *
 *  This class contains a number of functions that used to reside in the
 *  still-large player module.
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include "midi/midibytes.hpp"           /* midi::pulse alias                */

namespace transport
{

namespace jack
{

/**
 *  Provide a temporary structure for passing data and results between a
 *  player and transport::jack::transport object.  The
 *  transport::jack::transport class already has access to the members of
 *  player, but it needs access to and modification of "local" variables in
 *  player::output_func().  This scratchpad structure is useful even if
 *  JACK support is not enabled.
 */

class scratchpad
{

public:

    double js_current_tick;             /**< Holds current location.        */
    double js_total_tick;               /**< Current location ignoring L/R. */
    double js_clock_tick;               /**< Identical to js_total_tick.    */
    bool js_jack_stopped;               /**< Flags player::inner_stop().    */
    bool js_dumping;                    /**< Non-JACK playback in progress? */
    bool js_init_clock;                 /**< We now have a good JACK lock.  */
    bool js_looping;                    /**< seqedit loop button is active. */
    bool js_playback_mode;              /**< Song mode (versus live mode).  */
    double js_ticks_converted;          /**< Keeps track of ...?            */
    double js_ticks_delta;              /**< Minor difference in tick.      */
    double js_ticks_converted_last;     /**< Keeps track of position?       */
    long js_delta_tick_frac;            /**< More precision for rtl66 0.9.3 */

public:

    scratchpad ();
    void initialize
    (
        midi::pulse currenttick, bool islooping, bool songmode = false
    );
    void set_current_tick (midi::pulse curtick);
    void add_delta_tick (midi::pulse deltick);
    void set_current_tick_ex (midi::pulse curtick);

};          // class scratchpad

}           // namespace jack

}           // namespace transport

#endif      // defined RTL66_BUILD_JACK

#endif      // RTL66_TRANSPORT_JACK_SCRATCHPAD_HPP

/*
 * transport/jack/scratchpad.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

