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
 * \updates       2025-11-01
 * \license       See above.
 *
 *      Simple program to test MIDI input and retrieval from the queue.
 *
 * Refactoring: See midiout.cpp for a list.
 */

#include <iostream>                     /* std::cout, std::cerr             */
#include <memory>                       /* std::unique_ptr<>                */
#include <signal.h>                     /* is there a C++ version?          */

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "midi/clientinfo.hpp"          /* midi::clientinfo class           */
#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */
#include "util/msgfunctions.hpp"        /* util::status_message()           */
#include "xpc/kbhit.hpp"                /* xpc::kbhit_ex()                     */

namespace
{

/**
 *  Provides a flag and a signal handler for setting it.
 */

bool s_is_done { false  };

void
finish (int /*ignore*/)
{
    s_is_done = true;
}

}           // namespace anonymous

/**
 *  This function sets the global clientinfo object via rt_simple_cli().
 *
 *  The midi::global_client_info() accessor provides initial setup
 *  information and then current port information, application-wide.
 */

int
main (int argc, char * argv[])
{
    bool can_run = rt_simple_cli("qmidiin", argc, argv);
    cfg::set_client_name("qmidiin");
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
                /*
                 * Strictly speaking, we don't need a unique_ptr here;
                 * we could use an object directly.
                 */

                rtl::rtmidi::api rapi = rtl::rtmidi::desired_api(); /* static */
                std::unique_ptr<rtl::rtmidi_in> midiin
                {
                    new rtl::rtmidi_in
                    (
                        rapi, midi::global_client_info().client_name()
                    )
                };

                /*
                 * Check available ports vs. specified.
                 */

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
                         * Don't ignore sysex, timing, or active sensing
                         * messages. Install an interrupt handler function.
                         * Periodically check input queue.
                         */

                        midi::message msg;
                        midiin->ignore_midi_types(false, false, false);
                        if (midiin->open_port(port))
                        {
                            s_is_done = false;
                            (void) signal(SIGINT, finish);
                            std::cout
                                << "Reading MIDI from port "
                                << midiin->get_port_name(port)
                                << " ... quit with <Enter> or <Ctrl-C>."
                                << std::endl
                                ;

                            /*
                             * while (kbhit())
                             *     (void) getchar();
                             */

                            xpc::clear_kb();
                            while (! s_is_done)
                            {
                                (void) midiin->get_message(msg);
                                if (msg.count() > 0)
                                {
                                    std::string msgline { "Msg:" };
                                    msgline += msg.to_string();
                                    util::status_message(msgline);
                                }

                                /*
                                 *  if (kbhit())
                                 *  {
                                 *      (void) getchar();
                                 *      break;
                                 *  }
                                 *
                                 *  if (xpc::kbcheck())
                                 *      break;
                                 */

                                if (xpc::kbhit_ex())
                                {
                                    (void) getchar();
                                    break;
                                }
                                rt_test_sleep(10);  /* sleep for 10 msec    */
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
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

/*
 * qmidiin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

