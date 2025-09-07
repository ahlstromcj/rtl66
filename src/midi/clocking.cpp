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
 * \file          clocking.cpp
 *
 *    Support for the clocking (I/O enabling/disabling) enumeration.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2025-08-15
 * \updates       2025-09-06
 * \license       See above.
 *
 *  midi::clocking. A module for enum class clocking.
 */

#include <sstream>                      /* std::ostringstream               */

#include "midi/clocking.hpp"            /* midi::clocking etc.              */

namespace midi
{

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

std::string
clocking_to_string (midi::clocking e)
{
    std::string result;
    switch (e)
    {
        case midi::clocking::unavailable: result = "Unavailable";      break;
        case midi::clocking::disabled:    result = "Disabled";         break;
        case midi::clocking::none:        result = "Enabled/No clock"; break;
        case midi::clocking::pos:         result = "Pos";              break;
        case midi::clocking::mod:         result = "Mod";              break;
        default:                          result = "Unknown";          break;
    }
    return result;
}

}           // namespace midi

/*
 * clocking.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

