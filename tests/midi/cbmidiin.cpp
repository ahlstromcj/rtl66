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
 * \file          cbmidiin.cpp (was cmidiin.cpp)
 *
 *      Simple program to test MIDI inpu with a callback functiont.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2004; refactoring by Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2023-07-19
 * \license       See above.
 *
 *      A simple program to test MIDI input and the use of a user callback
 *      function.
 *
 *      On Linux, run this test both with ALSA and with JACK.
 *
 * Refactoring (see midiout.cpp for more):
 *
 *  -   #include "RtMidi.h"                 See the headers below.
 *  -   RtMidiIn                            rtl::rtmidi_in
 *  -   setCallback()                       set_input_callback()
 *  -   ignoreTypes()                       ignore_midi_types()
 */

#include <iostream>                     /* std::cout, std::cin              */

#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

/**
 *  This callback just shows the incoming bytes (in hex format).
 */

static void
midibytes_callback
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
            std::cout
                << "Byte #" << i << " = "
                << "0x" << std::hex << int(b) << "; ";
        }
        if (nbytes > 0)
        {
            std::cout << "timestamp " << deltatime << std::endl;
        }
    }
}

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run = rt_simple_cli("cbmidiin", argc, argv);
    if (can_run)
    {
        int port = 0;

        /*
         * Call function to select port.
         */

        if (rt_virtual_test_port())
        {
            // TODO
        }
        else
        {
            port = rt_test_port();
            if (port < 0)
            {
                port = rt_choose_port_number(false);    /* for in, not out  */
                can_run = port >= 0;
            }
        }
        if (can_run)
        {
            try
            {
                rtl::rtmidi_in midiin(rtl::rtmidi::desired_api());
                midiin.open_port(port);

                /*
                 * Set our callback function.  This should be done immediately
                 * after opening the port to avoid having incoming messages
                 * written to the queue instead of sent to the callback
                 * function.
                 *
                 * (This seems like a race-condition we should fix!)
                 */

                midiin.set_input_callback(&midibytes_callback);

                /*
                 * Don't ignore sysex, timing, or active sensing messages.
                 */

                midiin.ignore_midi_types(false, false, false);
                std::cout << "Reading MIDI input ... press <Enter> to quit.\n";

                char input;
                std::cin.get(input);
            }
            catch (rtl::rterror & error)
            {
                exit(EXIT_FAILURE);                 /* msg already shown    */
            }
        }
    }
    return EXIT_SUCCESS;
}

/*
 * cbmidiin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

