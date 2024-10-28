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
 * \file          midiprobe.cpp
 *
 *      Simple program to check MIDI inputs and outputs.
 *
 * \library       rtl66
 * \author        Gary Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2024-01-29
 * \license       See above.
 *
 *      By Gary Scavone, 2003-2012.
 */

#include <iostream>                     /* std::cout, std::cerr             */
#include <map>                          /* std::map<>                       */
#include <memory>                       /* std::unique_ptr<>                */

#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_show_help()                   */

/**
 *  Show help.
 */

static void
show_help ()
{
    printf
    (
        "--quiet    Try to hide any of the APIs' output messages\n"
    );
}

/**
 *  Main routine. Steps:
 *
 *  -   Create an rtl::rtmidi::api map.
 */

int
main (int argc, char * argv [])
{
    bool can_run = true;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (rt_show_help())
        {
            can_run = false;
            show_help();
        }
    }
    if (can_run)
    {
        rtl::rtmidi::api_list apis;
        rtl::rtmidi::get_detected_apis(apis);

        std::map<rtl::rtmidi::api, std::string> api_map;
        api_map[rtl::rtmidi::api::unspecified]  = "Unspecified";
        api_map[rtl::rtmidi::api::pipewire]     = "PipeWire";
        api_map[rtl::rtmidi::api::jack]         = "Jack";
        api_map[rtl::rtmidi::api::alsa]         = "Linux ALSA";
        api_map[rtl::rtmidi::api::macosx_core]  = "OS-X CoreMIDI";
        api_map[rtl::rtmidi::api::windows_mm]   = "Windows MultiMedia";
        api_map[rtl::rtmidi::api::web_midi]     = "Web MIDI";
        api_map[rtl::rtmidi::api::dummy]        = "Rtmidi Dummy";

        std::cout << "Compiled APIs:\n";
        for (size_t i = 0; i < apis.size(); ++i)
            std::cout << "  " << api_map[apis[i]] << std::endl;

        std::cout << std::endl;

        for (size_t i = 0; i < apis.size(); ++i)
        {
            /*
             * With the rtl::rtmidi_in/out constructors, exceptions are
             * possible.
             */

            std::unique_ptr<rtl::rtmidi_in> midiin;
            std::unique_ptr<rtl::rtmidi_out> midiout;
            std::cout << "Probing with API " << api_map[apis[i]] << std::endl;
            try
            {
                midiin.reset(new rtl::rtmidi_in(apis[i]));
                std::cout
                    << "Current input API: "
                    << api_map[midiin->get_current_api()]
                    << std::endl
                    ;

                int nports = midiin->get_port_count();
                std::cout << "There are " << nports << " MIDI inputs.\n";
                for (int i = 0; i < nports; ++i)
                {
                    std::string portname = midiin->get_port_name(i);
                    std::cout
                        << "  Input Port #" << i << ": "
                        << portname << std::endl
                        ;
                }
                midiout.reset(new rtl::rtmidi_out(apis[i]));
                std::cout
                    << "Current output API: "
                    << api_map[midiout->get_current_api()]
                    << std::endl
                    ;

                nports = midiout->get_port_count();
                std::cout << "There are " << nports << " MIDI outputs.\n";
                for (int i = 0; i < nports; ++i)
                {
                    std::string portname = midiout->get_port_name(i);
                    std::cout
                        << "  Output Port #" << i << ": "
                        << portname << std::endl
                        ;
                }
                std::cout << std::endl;

            }
            catch (rtl::rterror & error)
            {
                error.print_message();
            }
        }
    }
    return 0;
}

/*
 * midiprobe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

