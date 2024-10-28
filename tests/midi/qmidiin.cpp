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
 * \file          qmidiin.cpp
 *
 *      Simple program to test MIDI output.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2004; refactoring by Chris Ahlstrom
 * \date          2022-07-01
 * \updates       2024-06-01
 * \license       See above.
 *
 *      Simple program to test MIDI input and retrieval from the queue.
 *
 * Refactoring: See midiout.cpp for a list.
 */

#include <iostream>                     /* std::cout, std::cerr             */
#include <memory>                       /* std::unique_ptr<>                */
#include <signal.h>                     /* is there a C++ version?          */

#include "midi/clientinfo.hpp"          /* midi::global_client_info()       */
#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

static bool s_is_done;

static void
finish (int /*ignore*/)
{
    s_is_done = true;
}

int
main (int argc, char * argv[])
{
    bool can_run = rt_simple_cli("midiout", argc, argv);
    if (can_run)
    {
        std::unique_ptr<rtl::rtmidi_in> midiin;
        midi::message message;
        int nbytes, i;
        double stamp;
        try
        {
            rtl::rtmidi::api rapi = rtl::rtmidi::desired_api(); /* static */
#if defined RTL66_USE_GLOBAL_CLIENTINFO
            midiin.reset
            (
                new rtl::rtmidi_in(rapi, midi::global_client_info().client_name())
            );
#else
            midiin.reset(new rtl::rtmidi_in(rapi, "qmidiin"));
#endif

            /*
             * Check available ports vs. specified.
             */

            int port = rt_test_port();
            int nports = midiin->get_port_count();
            if (port == (-1))
            {
                port = 0;
                infoprint("Using port 0; use --port p option if desired.");
            }
            if (port >= nports)
            {
                std::cout << "invalid test-port" << std::endl;
            }
            else
            {
                try
                {
                    /*
                     * Don't ignore sysex, timing, or active sensing messages.
                     * Install an interrupt handler function.  Periodically check
                     * input queue.
                     */

                    if (midiin->open_port(port))
                    {
                        midiin->ignore_midi_types(false, false, false);
                        s_is_done = false;
                        (void) signal(SIGINT, finish);
                        std::cout
                            << "Reading MIDI from port "
                            << midiin->get_port_name(port)
                            << " ... quit with Ctrl-C."
                            << std::endl
                            ;
                        while (! s_is_done)
                        {
                            stamp = midiin->get_message(message);
                            nbytes = message.size();
                            for (i = 0; i < nbytes; ++i)
                                std::cout << "Byte " << i << " = "
                                    << int(message[i]) << "; " ;

                            if (nbytes > 0)
                                std::cout << "timestamp = " << stamp << std::endl;

                            rt_test_sleep(10);      /* sleep for 10 msec    */
                        }
                    }
                }
                catch (rtl::rterror & error)
                {
                    error.print_message();
                }
            }
        }
        catch (rtl::rterror & error)
        {
            exit(EXIT_FAILURE);         // error.print_message();
        }
    }
    return 0;
}

/*
 * qmidiin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

