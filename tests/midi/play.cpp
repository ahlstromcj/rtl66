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
 * \file          play.cpp
 *
 *      Simple program to test basic MIDI processing.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2024-05-26
 * \updates       2025-09-05
 * \license       See above.
 *
 *      Provides a play test for reading and playing a short MIDI file.
 *      It tests:
 *
 *          -   midi::file
 *          -   midi::track
 *          -   midi::player
 *          -   midi::bus
 *
 *      and their dependencies.
 *
 *  This module is still very much a work in progress.
 *
 *  It assumes it is run from the top-level directory of the rtl66 project.
 */

#include <iostream>                     /* std::cout and std::cerr          */

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "midi/player.hpp"              /* midi::player class               */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

/**
 *  Client info
 */

midi::client_defaults s_clientinfo_defaults
{
    RTL66_VERSION,                      /* API version                      */
    "playclient",                       /* client name                      */
    "play",                             /* client name                      */
    false,                              /* JACK MIDI                        */
    false,                              /* virtual ports                    */
    0,                                  /* no virtual input ports           */
    0,                                  /* no virtual output ports          */
    true,                               /* auto connect                     */
    false,                              /* port refresh                     */
    4,                                  /* the default global beat width    */
    4,                                  /* the default global beats per bar */
    384,                                /* global PPQN, not 192             */
    148,                                /* global BPM, not 120              */
    midi::port::io::duplex,             /* MIDI port type                   */
    -1,                                 /* input port number                */
    0                                   /* output port number               */
};

/**
 *  The port-numbers can be changed, so this item is not const.
 */

static midi::clientinfo s_clientinfo { s_clientinfo_defaults };

/**
 *  Tests of MIDI file parsing and writing for various files.
 */

static const std::string s_base_directory { "tests/data/midi" };
static const lib66::tokenization s_test_files
{
    "1Bar-export.mid",                      /* a simple standard MIDI file  */
//  "play.mid",
//  "1Bar.midi",
//  "simpleblast-ch1-8th-notes.midi",
//  "simpleblast-ch1-8th-notes-960.midi"
};

/**
 *  Assumes the player has already been set up.
 */

static
bool play_it (midi::player & p, std::string & errmsg)
{
#if defined PLATFORM_DEBUG_TMI
     p.print_tracks("Play");                /* show all events in tracks    */
#endif

    bool result { p.simple_play() };
    if (! result)
    {
        if (p.error_pending())
            errmsg = p.error_messages();
    }
    return result;
}

/**
 *  Assumes the player has already been set up.
 */

static
bool play_test (midi::player & p, const std::string & file)
{
    std::string errmsg;
    std::string testfile { s_base_directory };
    testfile += "/";
    testfile += file;

    bool result { p.read_midi_file(testfile, errmsg, false) };
    if (result)
    {
        result = p.set_midi_bus(rt_test_port());
        if (result)
            result = play_it(p, errmsg);

        if (result)
        {
            std::cout << "Success for " << testfile << std::endl;
        }
        else
        {
            result = false;
            std::cerr
                << "Failed to play " << testfile << "\n"
                << "Error: " << errmsg << std::endl;
                ;
        }
    }
    else
    {
        result = false;
        std::cerr << "Failed to read " << testfile << std::endl;
    }
    return result;
}

/**
 *  The main routine. It first checks if a port was specified and queries
 *  the user if not. If valid, then the test files are opened and played.
 *
 *  rt_choose_port_number() in test_helpers opens temporary I/O ports to get
 *  data, and lets the user choose a port number.
 *
 *  We would like to support setting up a player, getting the port list from it,
 *  letting the user choose one, then launching the player.  TODO.
 */

int
main (int argc, char * argv [])
{
    int rcode { EXIT_FAILURE };
    int out_port = { s_clientinfo.output_portnumber() };    /* --port p     */
    bool can_run = { rt_simple_cli("play", argc, argv) };
    cfg::set_client_name("play");
    if (can_run)
    {
        if (! rt_virtual_test_port())
        {
            if (rt_test_port_valid(rt_test_port()))
            {
                out_port = rt_test_port();                  /* --port p     */
            }
            else
            {
                /*
                 * Compare this to the midiout test application, which uses
                 * rt_choose_output_port(midiout), where midiout is
                 * an rtl::rtmidi_out object that is directly opened
                 * and used in the midiout test.
                 */

                out_port = rt_choose_port_number();     /* output port #    */
            }
        }
        can_run = rt_test_port_valid(out_port);
        if (can_run)
        {
            set_rt_test_port(out_port);                 /* port of interest */
            s_clientinfo.output_portnumber(out_port);
        }
    }
    if (can_run)
    {
        /*
         * Later we will add the PPQN and BPM parameters.
         */

        can_run = midi::set_global_client_info(s_clientinfo);

        /*
         * This function:
         *
         *  -   Creates the player's masterbus, which fills its
         *      clientinfo member with port data via engine_query().
         *      BUT what about all other members of clientinfo (e.g.
         *      ppqn, bpm)?
         *  -   Initializes JACK transport optionally.
         *  -   calls masterbus::engine_initialize() which uses
         *      data from transport/info, some of which is also in
         *      clientinfo. Sets PPQN and BPM.
         *  -   Activates the masterbus and maybe JACK transport.
         *  -   Launches the I/O threads.
         *
         *  TODO: make sure the port is opened somewhere along
         *        the line.
         *  TODO: verify the track-list.
         */

        if (can_run)
        {
            midi::clientinfo & ci { midi::global_client_info() };
            midi::player p { rt_test_port() };
            ci.output_portnumber(rt_test_port());
            can_run = p.launch();
            if (can_run)
            {
                std::string tag
                {
                    rtl::rtmidi::selected_api_display_name()
                };
                std::cout << "Running with " << tag << std::endl;
                rcode = EXIT_SUCCESS;
                for (const auto & file : s_test_files)
                {
                    bool success { play_test(p, file) };
                    if (! success)
                    {
                        rcode = EXIT_FAILURE;
                        break;
                    }
                }
            }
        }
    }
    return rcode;
}

/*
 * play.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

