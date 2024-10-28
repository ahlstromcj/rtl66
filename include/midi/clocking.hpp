#if ! defined RTL66_MIDI_CLOCKING_HPP
#define RTL66_MIDI_CLOCKING_HPP

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
 * \file          clocking.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2024-06-12
 * \license       GNU GPLv2 or above
 *
 *  Defines some midibus constants and the clock_e enumeration.
 */

namespace midi
{

namespace clock
{

/**
 *  Actions to perform with MIDI clocking.  Allow for consolidating
 *  some functions to simplify the API.
 *
 * \var init
 *      Initialize the MIDI clocking variables.
 *
 * \var start
 *      Start emitting MIDI clock.
 *
 * \var continue_from
 *      Continue the clock from the given tick.
 *
 * \var stop
 *      Stop MIDI clock.
 *
 * \var emit
 *      Emit MIDI clock at the given tick.
 *
 */

enum class action
{
    init,
    start,
    continue_from,
    stop,
    emit
};

}           // namespace clock

/**
 *  A clock enumeration, as used in the File / Options / MIDI Clock dialog.
 *  For savings in parameter usage, the enabling/disabling of input has
 *  been added as a clocking "status".
 *
 * \var unavailable
 *      This value indicates that a port defined in a port-map is not
 *      present on the system.
 *
 * \var disabled
 *      A new, currently-hidden value to indicate to ignore/disable an
 *      output port.  If a port always fails to open, we want just to
 *      ignore it.
 *
 * \var off
 *      Corresponds to the "Off" selection in the MIDI Clock tab.  With
 *      this setting, the MIDI Clock is disabled for the buss using this
 *      setting.  Notes will still be sent that buss, of course.  Some
 *      software synthesizers might require this setting in order to make
 *      a sound.  Note: This value also doubles as "enabled" for inputs,
 *      which don't support the concept of clocks. For inputs, any other
 *      value is equivalent to "not inputing".
 *
 * \var input
 *      Same as "off", but flags that the port is input, not output, and
 *      that it is enabled.
 *
 * \var pos
 *      Corresponds to the "Pos" selection in the MIDI Clock tab.  With
 *      this setting, MIDI Clock will be sent to this buss, and, if
 *      playback is starting beyond tick 0, then MIDI Song Position and
 *      MIDI Continue will also be sent on this buss.
 *
 * \var mod
 *      Corresponds to the "Mod" selection in the MIDI Clock tab.  With
 *      this setting, MIDI Clock and MIDI Start will be sent.  But
 *      clocking won't begin until the Song Position has reached the start
 *      modulo (in 1/16th notes) that is specified.
 *
 * \var max
 *      Illegal value for terminator.  Follows our convention for enum
 *      class maximums-out-of-bounds.
 */

enum class clocking
{
    unavailable = -2,
    disabled = -1,
    none = 0,
    input = 0,
    pos,
    mod,
    max
};

/*
 *  Inline free functions.
 */

inline clocking
int_to_clocking (int e)
{
    return e < static_cast<int>(clocking::max) ?
        static_cast<clocking>(e) : clocking::disabled ;
}

inline int
clocking_to_int (clocking e)
{
    return static_cast<int>(e == clocking::max ? clocking::disabled : e);
}

inline clocking
bool_to_clocking (bool f)
{
    return f ? clocking::none : clocking::disabled ;
}

inline bool
clock_enabled (clocking c)
{
    return c == clocking::pos || c == clocking::mod;
}

inline bool
clock_mod (clocking c)
{
    return c == clocking::mod;
}

inline bool
clock_pos (clocking c)
{
    return c == clocking::pos;
}

/**
 *  Could call this function clocking_to_bool(), too.
 */

inline bool
inputing_enabled (clocking ce)
{
    return ce == clocking::input;     /* tricky, same as "none"   */
}

inline bool
port_unavailable (clocking ce)
{
    return ce == clocking::unavailable;
}

inline bool
port_disabled (clocking ce)
{
    return ce == clocking::disabled;
}

}           // namespace midi

#endif      // RTL66_MIDI_CLOCKING_HPP

/*
 * clocking.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

