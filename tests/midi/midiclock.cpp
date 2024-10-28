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
 * \file          midiclock.cpp
 *
 *      Simple program to test MIDI output.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2004; refactoring by Chris Ahlstrom
 * \date          2022-06-25
 * \updates       2024-02-02
 * \license       See above.
 *
 *  Simple program to test MIDI clock sync.  Run midiclock_in in one
 *  console and midiclock_out in the other, make sure to choose
 *  options that connect the clocks between programs on your platform.
 *
 *  Note that both applications are built from the same code, and the
 *  function depends on the name used to execute this application.
 *  A soft link would work as well.
 *
 *  (C) 2016 Refer to README.md in this archive for copyright:
 *
 * Legal and ethical:
 *
 *      The RtMidi license is similar to the MIT License, with the added
 *      *feature* (non-binding) that modifications be sent to the developer.
 */

#include <iostream>                     /* std::cout, std::cin              */

#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

/*
 * Test callback function.
 */

static void
midi_clock_callback
(
    double deltatime,
    midi::message * message,
    void * userdata
)
{
    if (not_nullptr(message))
    {
        unsigned * clock_count = reinterpret_cast<unsigned *>(userdata);
        midi::message & msg = *message;
        if (msg.size() == 1)           /* ignore longer messages           */
        {
            midi::byte b = msg[0];                      // message->at(0);
            if (midi::is_midi_start_msg(b))             // 0xFA
                std::cout << "START" << std::endl;

            if (midi::is_midi_continue_msg(b))          // 0xFB
                std::cout << "CONTINUE" << std::endl;

            if (midi::is_midi_stop_msg(b))              // 0xFC
                std::cout << "STOP" << std::endl;

            if (midi::is_midi_clock_msg(b))             // 0xF8
            {
                if (++*clock_count == 24)               /* yikes! */
                {
                    double bpminute = 60.0 / 24.0 / deltatime;
                    std::cout << "One beat, estimated BPM = "
                        << bpminute <<std::endl
                        ;
                    *clock_count = 0;
                }
            }
            else
            *clock_count = 0;
        }
    }
}

/*
 *  Test for receiving MIDI Clock.
 */

static int
clock_in ()
{
    unsigned clock_count = 0;
    try
    {
        rtl::rtmidi_in midiin(rtl::rtmidi::desired_api());
        if (rt_choose_input_port(midiin))
        {
            /*
             * Set the callback function.  This should be done immediately
             * after opening the port to avoid having incoming messages
             * written to the queue instead of sent to the callback function.
             * Don't ignore sysex, timing, or active sensing messages.
             */

            midiin.set_input_callback(&midi_clock_callback, &clock_count);
            midiin.ignore_midi_types(false, false, false);
            std::cout <<
"Start the midiclock_out application and select the port selected in\n"
"that application to read MIDI clock from it. Press <Enter> to quit.\n"
                ;
            char input;
            std::cin.get(input);
        }
    }
    catch (rtl::rterror & error)
    {
        exit(EXIT_FAILURE);     // error.print_message();
    }
    return 0;
}

/**
 *  Duty now for the future!!!
 *
 * Setup:
 *
 *      Period in ms = 100 BPM
 *      100*24 ticks / 1 minute, so (60*1000) / (100*24) = 25 ms / tick
 *
 *      Later we can use rtl::rtmidi::global_bpm() to handle this?
 *
 * Play:
 *
 *      Send out a series of MIDI clock messages:
 *
 *          -   Start
 *          -   Continue
 *          -   Clock
 *          -   Stop
 */

static int
clock_out ()
{
    try
    {
        rtl::rtmidi_out midiout(rtl::rtmidi::desired_api());
        if (rt_choose_output_port(midiout))
        {
            int sleep_ms = 25;
            std::cout
                << "Generating clock at " << (60.0/24.0/sleep_ms*1000.0)
                << " BPM." << std::endl
                ;

            midi::message msg;
            msg.push(0xFA);                                 // MIDI start
            (void) midiout.send_message(msg);
            std::cout << "MIDI start" << std::endl;
            for (int j = 0; j < 8; ++j)
            {
                if (j > 0)
                {
                    msg.clear();
                    msg.push(0xFB);                         // MIDI continue
                    (void) midiout.send_message(msg);
                    std::cout << "MIDI continue" << std::endl;
                }
                for (int k = 0; k < 96; ++k)
                {
                    msg.clear();
                    msg.push(0xF8);                         // MIDI clock
                    (void) midiout.send_message(msg);
                    if (k % 24 == 0)
                        std::cout << "MIDI clock beat" << std::endl;

                    rt_test_sleep(sleep_ms);
                }
                msg.clear();
                msg.push(0xFC);                             // MIDI stop
                (void) midiout.send_message(msg);
                std::cout << "MIDI stop" << std::endl;
                rt_test_sleep(500);
            }
            msg.clear();
            msg.push(0xFC);                                 // MIDI stop
            (void) midiout.send_message(msg);
            std::cout << "MIDI stop" << std::endl;
            rt_test_sleep(500);
            std::cout << "Done!" << std::endl;
        }
    }
    catch (rtl::rterror & error)
    {
        exit(EXIT_FAILURE);     // error.print_message();
    }
    return 0;
}

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run = rt_simple_cli("midiclock", argc, argv);
    if (can_run)
    {
        std::string prog(argv[0]);
        if (prog.find("midiclock_in") != prog.npos)
        {
            (void) clock_in();
        }
        else if (prog.find("midiclock_out") != prog.npos)
        {
            (void) clock_out();
        }
        else
        {
            std::cout
                << "Don't know what to do as " << prog << ".\n"
                << " Add soft links to midiclock, named"
                << " midiclock_in and midiclock_out."
                << std::endl
                ;
        }
    }
    return 0;
}

/*
 * midiclock.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

