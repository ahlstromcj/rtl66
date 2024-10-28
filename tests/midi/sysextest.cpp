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
 * \file          sysextest.cpp
 *
 *      Simple program to test MIDI sysex sending and receiving.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2005; refactoring by Chris Ahlstrom
 * \date          2022-07-01
 * \updates       2023-07-19
 * \license       See above.
 *
 * Refactoring: See midiout.cpp for a list.
 */

#include <iostream>                     /* std::cout, std::cin              */
#include <memory>                       /* std::unique_ptr<>                */

#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

static void
midi_input_callback
(
    double deltatime,
    midi::message * message,
    void * /*userdata*/
)
{
    if (not_nullptr(message))
    {
        midi::message & msg = *message;
        size_t nbytes = msg.size();
        for (size_t i = 0; i < nbytes; ++i)
        {
            midi::byte b = msg[i];
            std::cout << "Byte " << i << " = " << int(b) << "; ";
        }
        if (nbytes > 0)
        {
            std::cout
                << "# of bytes = " << nbytes
                << ", timestamp = " << deltatime << std::endl
                ;
        }
    }
}

int
main (int argc, char * argv [])
{
    bool can_run = rt_simple_cli("sysextest", argc, argv);
    if (can_run)
    {
        std::unique_ptr<rtl::rtmidi_out> midiout;
        std::unique_ptr<rtl::rtmidi_in> midiin;
        midi::message message;
        int nbytes = rt_test_data_length();
        if (nbytes > 0)
        {
            try
            {
                midiout.reset(new rtl::rtmidi_out());
                midiin.reset(new rtl::rtmidi_in());

                /*
                 * Don't ignore sysex, timing, or active sensing messages.
                 */

                midiin->ignore_midi_types(false, true, true);
                try
                {
                    int port_in = rt_test_port_in();
                    int port_out = rt_test_port_out();
                    bool ok = port_in >= 0 && port_out >= 0;
                    if (! ok)
                    {
                        ok = rt_choose_input_port(*midiin);
                        if (ok)
                            ok = rt_choose_output_port(*midiout);
                    }
                    if (ok)
                    {
                        midiin->set_input_callback(&midi_input_callback);
                        message.push(midi::status::tune_select);    // 0xF6
                        midiout->send_message(message);
                        rt_test_sleep(500);                         // delay

                        /*
                         * Create a long sysex message of numbered bytes and
                         * send it out ... twice.
                         */

                        std::cout << "Sending sysex 4 times..." << std::endl;
                        for (int n = 0; n < 4; ++n)
                        {
                            message.clear();
                            message.push(midi::status::sysex);      // 0xF0
                            for (int i = 0; i < nbytes; ++i)
                                message.push(midi::byte(i % 128));

                            message.push(midi::status::sysex_end);  // 0xF7
                            std::cout << "  Sending sysex..." << std::endl;

                            bool ok = midiout->send_message(message);
                            if (ok)
                            {
                                rt_test_sleep(500);                 // delay
                            }
                            else
                            {
                                errprint("   ... failed");
                                break;
                            }
                        }
                    }
                }
                catch (rtl::rterror & error)
                {
                    error.print_message();
                }
            }
            catch (rtl::rterror & error)
            {
                error.print_message();
            }
        }
        else
        {
            errprint("Specify test-byte count with via '--length nn'.");
        }
    }
    return 0;
}

/*
 * sysextest.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

