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
 * \updates       2025-11-23
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

#include <cctype>                       /* std::isdigit()                   */
#include <iostream>                     /* std::cout and std::cerr          */

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "util/strfunctions.hpp"        /* util::string_to_int()            */
#include "midi/bus_out.hpp"             /* midi::bus_out class              */
#include "midi/player.hpp"              /* midi::player class               */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::find_midi_api() module      */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */
#include "xpc/timing.hpp"               /* xpc::millisleep()                */

namespace
{

/**
 *  Client info
 */

midi::client_defaults s_clientinfo_defaults
{
    RTL66_VERSION,                      /* API version                      */
    "playclient",                       /* client name                      */
    "play",                             /* app name                         */
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
    -1,                                 /* queue size, a bad value          */
    false,                              /* ALSA MIDI is not threadsafe      */
    midi::c_port_null,                  /* input port number (default)      */
    midi::c_port_null                   /* output port number (default)     */
};

/**
 *  The port-numbers can be changed, so this item is not const.
 */

midi::clientinfo &
app_client_info ()
{
    static midi::clientinfo s_clientinfo { s_clientinfo_defaults };
    return s_clientinfo;
}

/**
 *  Tests of MIDI file parsing and writing for various files.
 */

const std::string s_base_directory { "tests/data/midi" };

const lib66::tokenization s_test_files
{
    "1Bar-export.mid",                      /* a simple standard MIDI file  */
    "1Bar.midi",
    "simpleblast-ch1-8th-notes.midi",
    "simpleblast-ch1-8th-notes-960.midi",
    "smoke.mid"
};

/**
 *  Assumes the player has already been set up.
 */

bool
play_it (midi::player & p, std::string & errmsg)
{
    /*
     * The first call just zips through playback, ignoring time-stamps.
     * The second call does not start playback properly. The third
     * does not return a status value.
     *
     * bool result { p.simple_play() };
     * bool result = p.play();
     * p.start_playing();
     */


    bool result { p.arm_all_tracks() };
    if (result)
    {
        result = p.auto_pause();            /* vs auto_play(), auto_stop()  */
        if (result)
        {
            while (! p.at_song_end())       /* plus an extra PPQN / 4       */
                ;

            (void) p.auto_stop();

            /*
             * If we don't wait a bit before the next tune, it can play
             * incorrectly (e.g. very slowly).
             */

            xpc::millisleep(1000);
        }
    }
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

bool
play_test (midi::player & p, const std::string & file)
{
    std::string errmsg;
    bool result { p.read_midi_file(file, errmsg, false) };
    if (result)
    {
        result = p.set_midi_bus(rt_test_port());    /* not a user change    */
        if (result)
            result = play_it(p, errmsg);

        if (result)
        {
            util::status_message("Success", file);
        }
        else
        {
            result = false;
            util::error_message("Failure", file);
            util::error_message("Error", errmsg);
        }
    }
    else
    {
        result = false;
        std::cerr << "Failed to read " << file << std::endl;
    }
    return result;
}

/**
 *  Provides a masterbus object, of which only a few facilties will be
 *  used, to support a single midi::bus_out object. Compare it to
 *  player::create_master_bus() and the follow-on code in launch().
 */

midi::masterbus &
master_bus (rtl::rtmidi::api rapi, midi::clientinfo & ci)
{
    if (rapi == rtl::rtmidi::api::unspecified)
        rapi = rtl::find_midi_api();

    static midi::masterbus s_master_bus { rapi, ci };
    static bool s_uninitialized { true };
    if (s_uninitialized)
    {
        bool ok { rapi != rtl::rtmidi::api::unspecified };
        if (ok)
            ok = s_master_bus.setup(ci);

        if (ok)
            s_uninitialized = false;
    }
    return s_master_bus;
}

}           // namespace anonymous

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
    int out_port = { app_client_info().output_portnumber() };    /* --port  */
    bool can_run = { rt_simple_cli("play", argc, argv) };
    std::string single_filename { rt_test_name() };
    cfg::set_app_name(app_client_info().app_name());
    cfg::set_client_name(app_client_info().client_name());
    if (can_run)
    {
        if (! single_filename.empty())
        {
            if (single_filename == "list")
            {
                int i { 0 };
                for (const auto & file : s_test_files)
                {
                    std::string testfile { s_base_directory };
                    testfile += "/";
                    testfile += file;
                    std::cout << "[" << i++ << "] " << testfile << std::endl;
                }
                can_run = false;
            }
            else
            {
                if (std::isdigit(single_filename[0]))
                {
                    int itemno { util::string_to_int(single_filename) };
                    if (itemno >= 0 && itemno < int(s_test_files.size()))
                    {
                        std::string testfile { s_base_directory };
                        testfile += "/";
                        testfile += s_test_files[itemno];
                        single_filename = testfile;
                    }
                }
                else
                {
                    /*
                     * Will play the given file if it can be found.
                     */
                }
            }
        }
    }
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
            app_client_info().output_portnumber(out_port);
        }
    }
    if (can_run)
    {
        /*
         * Later we will add the PPQN and BPM parameters.
         *
         * Don't really need the global info, do we?

        can_run = midi::set_global_client_info(app_client_info());
         */

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
            rtl::rtmidi::api rapi { rtl::rtmidi::selected_api() };
            midi::masterbus & master { master_bus(rapi, app_client_info()) };
            midi::player p { master };
            can_run = p.launch();
            if (can_run)
            {
                midi::masterbus * const masterptr { &p.master_bus() };
                if (not_nullptr(masterptr))
                {
                    midi::bus & outbus
                    {
                        masterptr->get_out_bus(rt_test_port())
                    };
                    can_run = outbus.initialize();
                }
                else
                    can_run = false;
            }
            if (can_run)
            {
                std::string tag
                {
                    rtl::rtmidi::selected_api_display_name()
                };
                std::cout << "Running with " << tag << std::endl;
                rcode = EXIT_SUCCESS;
                if (single_filename.empty())
                {
                    for (const auto & file : s_test_files)
                    {
                        std::string testfile { s_base_directory };
                        testfile += "/";
                        testfile += file;

                        bool success { play_test(p, testfile) };
                        if (! success)
                        {
                            rcode = EXIT_FAILURE;
                            break;
                        }
                    }
                }
                else
                {
                    bool success { play_test(p, single_filename) };
                    if (! success)
                        rcode = EXIT_FAILURE;
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

