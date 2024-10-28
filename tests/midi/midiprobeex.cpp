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
 * \file          midiprobeex.cpp
 *
 *      Simple program to check MIDI inputs and outputs.
 *
 * \library       rtl66
 * \author        Gary Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2023-03-06
 * \license       See above.
 *
 */

#include <iostream>                     /* std::cout, std::cerr             */

#include "midi/masterbus.hpp"           /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi::desired_api()       */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

/**
 *  Main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run = rt_simple_cli("midiprobeex", argc, argv);
    if (can_run)
    {
        midi::masterbus mb(rtl::rtmidi::desired_api());
        if (mb.engine_initialize())
        {
            std::string portlist = mb.port_listing();
            std::cout << portlist;
        }
        else
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/*
 * midiprobeex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

