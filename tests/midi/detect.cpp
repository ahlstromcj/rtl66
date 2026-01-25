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
 * \file          detect.cpp
 *
 *      Simple program to check for leaks in a MIDI API.
 *
 * \library       rtl66
 * \author        Gary Scavone; refactoring by Chris Ahlstrom
 * \date          2026-01-21
 * \updates       2026-01-24
 * \license       See above.
 *
 */

#include <iostream>                     /* std::cout, std::cerr             */

#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi::desired_api()       */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

namespace
{

bool
detect (rtl::rtmidi::api rapi)
{
#if defined RTL66_BUILD_DUMMY           /* always defined                   */
    bool result = rapi == rtl::rtmidi::api::dummy && rtl::detect_dummy();
#endif

#if defined RTL66_BUILD_JACK
    if (! result)
        result = rapi == rtl::rtmidi::api::jack && rtl::detect_jack();
#endif

#if defined RTL66_BUILD_ALSA
    if (! result)
        result = rapi == rtl::rtmidi::api::alsa && rtl::detect_alsa();
#endif

#if defined RTL66_BUILD_PIPEWIRE
    if (! result)
        result = rapi == rtl::rtmidi::api::pipewire && detect_pipewire();
#endif

#if defined RTL66_BUILD_MACOSX_CORE
    if (! result)
        result = rapi == rtl::rtmidi::api::macosx_core && detect_core();
#endif

#if defined RTL66_BUILD_WIN_MM          /* deprecated, not implemented      */
    if (! result)
        result = rapi == rtl::rtmidi::api::windows_mm && detect_win_mm();
#endif

#if defined RTL66_BUILD_WIN_UWP
    if (! result)
        result rapi == rtl::rtmidi::api::windows_uwp && detect_win_uwp();
#endif

#if defined RTL66_BUILD_WEB_MIDI
    if (! result)
        result = rapi == rtl::rtmidi::api::web_midi && detect_web_midi();
#endif

    std::string name { rtl::rtmidi::api_name(rapi) };
    if (result)
    {
        std::cout << "API '" << name << "' detected." << std::endl;
    }
    else
    {
        std::cerr
            << "API '" << name
            << "' either not detected or not compiled in."
            << std::endl
            ;
    }
    return result;
}

}           // namespace anonymous

/**
 *  Main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run { rt_simple_cli("detect", argc, argv) };
    if (can_run)
    {
        rtl::rtmidi::api rapi { rtl::rtmidi::desired_api() };
        std::string name { rtl::rtmidi::api_name(rapi) };
        if (rapi == rtl::rtmidi::api::unspecified)
        {
            rt_print_help("detect");
            std::cerr
                << "Specify a valid MIDI API, e.g. --alsa, --jack, etc."
                << std::endl
            ;
        }
        else
        {
            std::cout << "API '" << name << "' requested." << std::endl;
            return detect(rapi) ? EXIT_SUCCESS : EXIT_FAILURE ;
        }
    }
    else
        return EXIT_FAILURE;
}

/*
 * detect.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

