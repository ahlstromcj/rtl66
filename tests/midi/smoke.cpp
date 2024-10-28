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
 * \file          smoke.cpp
 *
 *      Simple program to test basic MIDI processing.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-11-25
 * \updates       2024-06-05
 * \license       See above.
 *
 *      Provides a smoke test for reading and writing a short MIDI file and
 *      for setting up MIDI.
 *
 *      It tests:
 *
 *          -   midi::file
 *          -   midi::track
 *          -   midi::player
 *          -   midi::bus
 *
 *      and their dependencies.
 *
 *  This test is still a work in progress; we need to add even more files
 *  to test. For playback tests, see play.cpp.
 *
 *  It assume it is run from the top-level directory of the rtl66 project.
 */

#include <iostream>

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */
#include "midi/masterbus.hpp"           /* rtl::rtmidi class, etc.          */
#include "midi/player.hpp"              /* midi::player class               */

/**
 *  Tests of MIDI file parsing and writing for various files.
 */

static const std::string s_base_directory{"tests/data/midi"};
static const std::string s_out_wart{"-out"};
static const std::vector<std::string> s_test_files
{
    "smoke.mid",
    "1Bar.midi",
    "simpleblast-ch1-8th-notes.midi",
    "simpleblast-ch1-8th-notes-960.midi"
};

static
bool file_test (midi::player & p, const std::string & file)
{
    std::string errmsg;
    std::string testfile{s_base_directory};
    testfile += "/";
    testfile += file;

    bool result = p.read_midi_file(testfile, errmsg, false);
    if (result)
    {
        std::string outfile = testfile;
        auto ppos = outfile.find_first_of(".");
        outfile.insert(ppos, "-out");

        /*
         * First write using the event-only option. We want that working
         * for basic usage.  We might test this automatically later.
         */

        result = p.write_midi_file(outfile, errmsg, true);
        if (result)
        {
            std::cout
                << "Success. In " << s_base_directory << " "
                << file << " and its -out version should be identical."
                << std::endl
                ;
        }
        else
        {
            result = false;
            std::cerr << "Failed to write " << outfile << std::endl;
        }
    }
    else
    {
        result = false;
        std::cerr << "Failed to load " << testfile << std::endl;
    }
    return result;
}

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    int rcode = EXIT_FAILURE;
    bool can_run = rt_simple_cli("smoke", argc, argv);
    if (can_run)
    {
        cfg::set_client_name("smoke");
        try
        {
            /*
             * Call function to select port.
             */

            rtl::rtmidi::api rapi = rtl::rtmidi::desired_api();
            if (rt_virtual_test_port())
            {
                // TODO
            }
            else
            {
                midi::clientinfo cinfo;
                can_run = midi::get_all_port_info(cinfo, rapi);
                if (can_run)
                {
                    /*
                     * tag += rtl::rtmidi::selected_api_name();
                     */

                    std::string tag = rtl::rtmidi::selected_api_display_name();
                    tag += " MIDI Ports";

                    std::string plist = cinfo.port_list();
                    std::cout << plist << std::endl;
                }
            }
            if (can_run)
            {
                /*
                 * Later we will add the PPQN and BPM parqmeters.
                 */

                rtl::rtmidi::api rapi = rtl::rtmidi::selected_api();
                midi::masterbus mbus(rapi);
                midi::player p;
                if (mbus.engine_initialize())       /* default PPQN, BPM    */
                {
                    std::cout << "Master bus initialized." << std::endl;
                }
                else
                {
                    std::cerr << "masterbus initialization failed" << std::endl;
                    rcode = EXIT_FAILURE;
                    can_run = false;
                }
                if (can_run)
                {
                    std::string tag = rtl::rtmidi::selected_api_display_name();
                    std::cout << "Running with " << tag << std::endl;
                    rcode = EXIT_SUCCESS;
                    for (const auto & file : s_test_files)
                    {
                        bool success = file_test(p, file);
                        if (! success)
                        {
                            rcode = EXIT_FAILURE;
                            break;
                        }
                    }
                }
            }
        }
        catch (rtl::rterror & error)
        {
            exit(EXIT_FAILURE);     // error.print_message();
        }
    }
    return rcode;
}

/*
 * smoke.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

