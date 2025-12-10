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
 *      Simple program to test MIDI inpu with a callback function.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2004; refactoring by Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2025-11-20
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
#include <vector>                       /* std::vector<> of rtmidi_in's     */

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */
#include "util/msgfunctions.hpp"        /* util::status_message()           */
#include "xpc/kbhit.hpp"                /* xpc::kbget()                     */

/**
 *  This callback just shows the incoming bytes (in hex format).
 */

namespace
{

/**
 *  Handles an input message.
 */

void
midibytes_callback
(
    double deltatime,                   /* always 0 in this test program    */
    midi::message & msg,
    void * userdata
)
{
    (void) userdata;
    deltatime = msg.jack_stamp();
    size_t nbytes { msg.size() };
    if (nbytes > 0)
    {
        std::string msgline { "Msg:" };
        msgline += msg.to_string();
        util::status_message(msgline);
    }
    else
        std::cout << "Empty message w/delta " << deltatime << std::endl;
}

/*
 * Set our callback function.  This should be done immediately after opening
 * the port to avoid having incoming messages written to the queue instead
 * of sent to the callback function. This seems like a race-condition we
 * should fix, so we now set it before opening a port.
 */

bool
read_port (rtl::rtmidi::api rapi, int portnumber)
{
    bool result { rt_test_port_valid(portnumber) };
    try
    {
        rtl::rtmidi_in midiin(rapi, "cbmidiin");
        midiin.set_input_callback(&midibytes_callback);

        /*
         * Don't ignore sysex, timing, or active sensing
         * messages.
         */

        midiin.ignore_midi_types(false, false, false);

        /*
         * Open the port.
         */

        result = midiin.open_port(portnumber);
        if (result)
        {
            std::cout << "Reading MIDI input ... press <Enter> to quit.\n";
            (void) xpc::kbget();                /* c = std::cin.get()   */
        }
        else
        {
            std::cerr
                << "Could not open port " << portnumber
                << " ... aborting" << std::endl
                ;
        }
    }
    catch (rtl::rterror & error)
    {
        error.print_message();
        result = false;
    }
    return result;
}

/*
 * We could use a unique_ptr<>, to avoid the delete loop after kbget(). See
 * the other test, qmidiiin.
 */

bool
read_all_ports (rtl::rtmidi::api rapi, int portcount)
{
    bool result { portcount > 0 };
    if (result)
    {
        try
        {
            std::vector<rtl::rtmidi_in *> allports;
            std::string basename { "cbmidiin-" };
            for (int p = 0; p < portcount; ++p)
            {
                std::string name { basename };
                name += std::to_string(p);

                rtl::rtmidi_in * inptr
                {
                    new (std::nothrow) rtl::rtmidi_in(rapi, name)
                };
                if (not_nullptr(inptr))
                {
                    inptr->set_input_callback(&midibytes_callback);
                    if (inptr->open_port(p, name))
                    {
                        allports.push_back(inptr);
                    }
                    else
                    {
                        std::cerr << "Aborting at port #" << p << std::endl;
                        result = false;
                    }
                }
            }
            if (result)
            {
                std::cout
                    << "Reading MIDI inputs ... press <Enter> to quit."
                    << std::endl
                    ;
                (void) xpc::kbget();                /* c = std::cin.get()   */
                for (auto ptr : allports)
                    delete ptr;
            }
        }
        catch (rtl::rterror & error)
        {
            result = false;
            error.print_message();
        }
    }
    return result;
}

}           // namespace anonymous

/**
 *  The main routine.
 *
 *  When opening all ports ("--port all"), a list like the following, showing
 *  the hardware and the application ports generated, while waiting for input,
 *  should appear (using ALSA):
 *
 *  $ aconnect -lio
 *  client 0: 'System' [type=kernel]
 *      0 'Timer           '
 *      1 'Announce        '
 *  client 14: 'Midi Through' [type=kernel]
 *      0 'Midi Through Port-0'
 *          Connecting To: 128:0
 *  client 32: 'nanoKEY2' [type=kernel,card=4]
 *      0 'nanoKEY2 _ CTRL '
 *          Connecting To: 129:0
 *  client 36: 'Q25' [type=kernel,card=5]
 *      0 'Q25 MIDI 1      '
 *          Connecting To: 130:0
 *  client 128: 'cbmidiin-0' [type=user,pid=431532]
 *      0 'cbmidiin-0      '
 *          Connected From: 14:0
 *  client 129: 'cbmidiin-1' [type=user,pid=431532]
 *      0 'cbmidiin-1      '
 *          Connected From: 32:0
 *  client 130: 'cbmidiin-2' [type=user,pid=431532]
 *      0 'cbmidiin-2      '
 *          Connected From: 36:0
 *
 *  One should note that each application port has it's own client number.
 *  This is the default setup using the original RtMidi paradigm. Each
 *  port is handled by a different thread, and using a unique client
 *  number (i.e. a unique snd_seq_t pointer) for each port avoids
 *  segfaults. (Recall that the ALSA API is *not* thread-safe in user
 *  space.)
 */

int
main (int argc, char * argv [])
{
    bool can_run { rt_simple_cli("cbmidiin", argc, argv) };
    cfg::set_client_name("cbmidiin");
    bool success { true };
    if (can_run)
    {
        int port { 0 };
        int portcount { 0 };

        /*
         * Call function to select port.
         */

        if (rt_virtual_test_port())
        {
            // TODO
        }
        else
        {
            can_run = rt_select_input_ports(portcount);
        }
        if (can_run)
        {
            rtl::rtmidi::api rapi { rtl::rtmidi::selected_api() };
            if (rt_open_all_ports())
            {
                success = read_all_ports(rapi, portcount);
            }
            else
            {
                port = rt_test_port();
                success = read_port(rapi, port);
            }
        }
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE ;
}

/*
 * cbmidiin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
