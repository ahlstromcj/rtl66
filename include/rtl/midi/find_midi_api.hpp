#if ! defined RTL66_RTL_MIDI_FIND_MIDI_API_HPP
#define RTL66_RTL_MIDI_FIND_MIDI_API_HPP

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
 * \file          find_midi_api.hpp
 *
 *    A reworking of RtMidi.hpp, to avoid circular dependencies involving
 *    rtmidi_in.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-07-23
 * \updates       2024-05-26
 * \license       See above.
 *
 */

#include "rtl/midi/midi_api.hpp"
#include "rtl/midi/rtmidi.hpp"

namespace rtl
{

extern rtmidi::api find_midi_api
(
    rtmidi::api desiredapi  = rtmidi::api::unspecified,
    std::string clientname  = ""
);
extern midi_api * try_open_midi_api
(
    rtmidi::api rapi,
    midi::port::io iotype,
    std::string clientname  = "",
    unsigned qsize          = 0
);

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_FIND_MIDI_API_HPP

/*
 * find_midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

