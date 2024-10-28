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
 * \file          midi_dummy.cpp
 *
 *    Message about this module...
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-03-06
 * \license       See above.
 *
 */

#include "rtl/midi/midi_dummy.hpp"      /* rtl::midi_dummy class            */

#if defined RTL66_BUILD_DUMMY

namespace rtl
{

bool
detect_dummy ()
{
    return true;
}

midi_dummy::midi_dummy
(
    const std::string & /*cliname*/,
    unsigned queuesizelimit
) :
    midi_api (midi::port::io::dummy, queuesizelimit)
{
    error_string("midi_dummy: provides no functionality");
    error(rterror::kind::warning, error_string());
}

}           // namespace rtl

#else

namespace rtl
{

bool
detect_dummy ()
{
    return false;
}

}           // namespace rtl

#endif      // defined RTL66_BUILD_DUMMY

/*
 * midi_dummy.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

