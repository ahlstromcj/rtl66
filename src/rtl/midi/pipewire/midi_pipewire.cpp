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
 * \file          midi_pipewire.cpp
 *
 *    Will ultimately implement a PipeWire interface.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-06-17
 * \updates       2023-03-06
 * \license       See above.
 *
 * https://docs.pipewire.org/page_midi.html#:~:text=
 * PipeWire%20media%20session%20uses%20the,ports%20per%20MIDI%20client%2Fstream.
 */

#include "rtl/midi/pipewire/midi_pipewire.hpp"  /* rtl::midi_pipewire class */

#if defined RTL66_BUILD_PIPEWIRE

namespace rtl
{

/**
 *  PipeWire detection function. TODO!
 */

bool
detect_pipewire ()
{
    return false;
}


midi_pipewire::midi_pipewire
(
    const std::string & /*cliname*/, unsigned queuesize
) :
    midi_api (queuesize)
{
    m_error_string = "midi_pipewire: provides no functionality yet";
    error(rterror::kind::warning, m_error_string);
}

/*
 * Currently the dummy pipewire code is defined in the header file.
 */

}           // namespace rtl

#endif      // defined RTL66_BUILD_PIPEWIRE

/*
 * midi_pipewire.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

