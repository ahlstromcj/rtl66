#if ! defined RTL66_RTL_MIDI_MIDI_TYPES_HPP
#define RTL66_RTL_MIDI_MIDI_TYPES_HPP

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
 * \file          rt_midi_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2024-06-04
 * \license       See above.
 *
 */

namespace rtl
{

/*
 * This breaks the build by putting midi::xxx items into the
 * rtl::midi::namespace. Also, we do not have a corresponding audio
 * namesapce in rt_audio_types.hpp.
 *
 *  namespace midi
 *  {
 *  }           // namespace midi
 */

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_MIDI_TYPES_HPP

/*
 * rt_midi_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

