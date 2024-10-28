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
 * \file          midiout.cpp
 *
 *      Simple program to test MIDI output.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2004; refactoring by Chris Ahlstrom
 * \date          2022-06-25
 * \updates       2023-07-19
 * \license       See above.
 *
 *      Tests that the C API for rtl (RtMidi refactored) is working.
 *
 *      On Linux, run this test both with ALSA and with JACK.
 *
 * Refactoring:
 *
 *  -   #include "RtMidi.h"                 See the headers below.
 *  -   chooseMidiPort()                    choose_midi_port()
 *  -   RtMidi                              rtl::rtmidi
 *  -   RtMidiOut                           rtl::rtmidi_out
 *  -   RtMidiError                         rtl::rterror
 *  -   std::vector<unsigned char>          midi::message
 *  -   printMessage()                      print_message()
 *  -   openVirtualPort()                   open_virtual_port()
 *  -   getPortCount()                      get_port_count()
 *  -   getPortName()                       get_port_name()
 *  -   openPort()                          open_port
 */

#include <iostream>

#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run = rt_simple_cli("midiout", argc, argv);
    if (can_run)
    {
        try
        {
            /*
             * Call function to select port.
             */

            rtl::rtmidi_out midiout(rtl::rtmidi::desired_api());
            if (! rt_virtual_test_port())
            {
                can_run = rt_choose_output_port(midiout);
            }
            if (can_run)
            {
                /* Send out a series of MIDI messages. */

                midi::message msg;
                msg.push(midi::status::program_change); // 0xC0 [ 192 ]
                msg.push(5);                            // Electric Piano?
                (void) midiout.send_message(msg);
                rt_test_sleep(500);

                msg.clear();
                msg.push(midi::status::quarter_frame);  // 0xF1
                msg.push(60);                           // ??
                (void) midiout.send_message(msg);

                msg.clear();
                msg.push(midi::status::control_change); // 0xB0 [ 176 ]
                msg.push(midi::ctrl::volume);           // 0x07
                msg.push(100);                          // volume level
                midiout.send_message(msg);

                msg.clear();
                msg.push(midi::status::note_on);        // 0x90 [ 144 ]
                msg.push(64);                           // note number
                msg.push(90);                           // velocity
                (void) midiout.send_message(msg);
                rt_test_sleep(500);

                msg.clear();
                msg.push(midi::status::note_off);       // 0x80 [ 128 ]
                msg.push(64);                           // note number
                msg.push(40);                           // velocity
                (void) midiout.send_message(msg);
                rt_test_sleep(500);

                msg.clear();
                msg.push(midi::status::control_change); // 0xB0 [ 176 ]
                msg.push(midi::ctrl::volume);           // 0x07
                msg.push(40);                           // volume level
                (void) midiout.send_message(msg);
                rt_test_sleep(500);

                msg.clear();
                msg.push(midi::status::sysex);          // 0xF0 [ 240 ]
                msg.push(67);                           // Yamaha (Manuf. ID.)
                msg.push(4);                            // ??
                msg.push(3);                            // ??
                msg.push(2);                            // ??
                msg.push(midi::status::sysex_end);      // 0xF7 [ 247 ]
                (void) midiout.send_message(msg);
            }
        }
        catch (rtl::rterror & error)
        {
            exit(EXIT_FAILURE);     // error.print_message();
        }
    }
    return 0;
}

/*
 * midiout.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

