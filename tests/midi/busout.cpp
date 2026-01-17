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
 * \file          busout.cpp
 *
 *      Simple program to test MIDI output.
 *
 * \library       rtl66
 * \author        Gary Scavone, 2003-2004; refactoring by Chris Ahlstrom
 * \date          2025-08-26
 * \updates       2026-01-09
 * \license       See above.
 *
 *      This application has elements of the play test application,
 *      but merely opens one port and sends messages directly, rather
 *      than using a busarray.
 *
 *      On Linux, run this test both with ALSA and with JACK.
 */

#include <iostream>

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "midi/bus_out.hpp"             /* midi::bus_out class              */
#include "midi/event.hpp"               /* midi::event class                */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::find_midi_api() module      */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

namespace
{

/**
 *  Client info
 *
 *  Note that, currently, the port type must be duplex in order for
 *  masterbus to get all the port information.
 */

midi::client_defaults s_client_defaults
{
    RTL66_VERSION,                      /* API version                      */
    "busclient",                        /* client name                      */
    "busout",                           /* app name                         */
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
    -1,                                 /* queue size immaterial for output */
    false,                              /* threads immaterial for output    */
    midi::c_port_null,                  /* input port number (default)      */
    midi::c_port_null                   /* output port number (default)     */
};

/**
 *  The port-numbers can be changed, so this item is not const. The clientinfo
 *  constructor is the one that uses the default midi::input_specs member
 *  (i.e. disabled since this test program does only output).
 *
 */

midi::clientinfo &
app_client_info ()
{
    static midi::clientinfo s_clientinfo(s_client_defaults);
    return s_clientinfo;
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
            ok = s_master_bus.setup();      /* ci */

        if (ok)
            s_uninitialized = false;
    }
    return s_master_bus;
}

/**
 *  Chooses which function to use to send the event / message and
 *  sends it.
 *
 *  Chooses the usage of bus_out::send_message() vs bus_out::send_event().
 */

bool
send_the_message (midi::bus_out & b, const midi::message & msg)
{
    std::string testname { rt_test_name() };
    bool result;
    if (testname == "event")
    {
        midi::event ev(msg);            /* convert the bytes to an event    */
        result = b.send_event(&ev);
    }
    else
    {
        testname = "message";
        result = b.send_message(msg);
    }
    if (! result)
        printf("send_%s failed\n", testname.c_str());

    return result;
}

}           // namespace anonymous

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run { rt_simple_cli("busout", argc, argv) };
    bool had_error = false;
    if (can_run)
    {
        cfg::set_app_name(app_client_info().app_name());
        cfg::set_client_name(app_client_info().client_name());

#if defined USE_REGULAR_RT_SELECT_PORTS

        try
        {
            if (! rt_virtual_test_port())
            {
                if (! rt_test_port_valid(rt_test_port()))
                {
                    rtl::rtmidi_out midiout { rtl::rtmidi::desired_api() };
                    can_run = rt_choose_output_port(midiout);
                }
            }
        }
        catch (rtl::rterror & error)
        {
            std::cerr << "Caught rtl::rterror!" << std::endl;
            had_error = true;                        // error.print_message()
            can_run = false;
        }

#else

        rtl::rtmidi::api srapi { rtl::rtmidi::selected_api() };
        midi::masterbus & master { master_bus(srapi, app_client_info()) };
        int portcount { 0 };
        int p                               /* show w/out all-ports option  */
        {
            master.choose_port(midi::port::io::output, portcount, false)
        };
        can_run = midi::is_good_buss(p);    /* ! midi::is_null_buss(p);     */

#endif  // defined USE_REGULAR_RT_SELECT_PORTS

        if (can_run)
        {
            set_rt_test_port(p);                                /* klunky   */

            int portnumber { rt_test_port() };
            app_client_info().output_portnumber(portnumber);

            midi::bus & outbus { master.get_out_bus(portnumber) };
            can_run = outbus.initialize();
            if (can_run)
            {
                try
                {
                    midi::bus_out & busout
                    {
                        dynamic_cast<midi::bus_out &>(outbus)
                    };

                    /* Send out a series of MIDI messages. */

                    midi::message msg;
                    msg.push(midi::status::program_change); // 0xC0 [ 192 ]
                    msg.push(5);                            // Electric Piano?
                    (void) send_the_message(busout, msg);
                    rt_test_sleep(500);

                    msg.clear();
                    msg.push(midi::status::quarter_frame);  // 0xF1
                    msg.push(60);                           // ??
                    (void) send_the_message(busout, msg);

                    msg.clear();
                    msg.push(midi::status::control_change); // 0xB0 [ 176 ]
                    msg.push(midi::ctrl::volume);           // 0x07
                    msg.push(100);                          // volume level
                    (void) send_the_message(busout, msg);

                    msg.clear();
                    msg.push(midi::status::note_on);        // 0x90 [ 144 ]
                    msg.push(64);                           // note number
                    msg.push(90);                           // velocity
                    (void) send_the_message(busout, msg);
                    rt_test_sleep(500);

                    msg.clear();
                    msg.push(midi::status::note_off);       // 0x80 [ 128 ]
                    msg.push(64);                           // note number
                    msg.push(40);                           // velocity
                    (void) send_the_message(busout, msg);
                    rt_test_sleep(500);

                    msg.clear();
                    msg.push(midi::status::control_change); // 0xB0 [ 176 ]
                    msg.push(midi::ctrl::volume);           // 0x07
                    msg.push(40);                           // volume level
                    (void) send_the_message(busout, msg);
                    rt_test_sleep(500);

                    msg.clear();
                    msg.push(midi::status::sysex);          // 0xF0 [ 240 ]
                    msg.push(67);                           // Yamaha (Man. ID.)
                    msg.push(4);                            // ??
                    msg.push(3);                            // ??
                    msg.push(2);                            // ??
                    msg.push(midi::status::sysex_end);      // 0xF7 [ 247 ]
                    (void) send_the_message(busout, msg);
                }
                catch (rtl::rterror & error)
                {
                    std::cerr << "Caught rtl::rterror!" << std::endl;
                    had_error = true;           // error.print_message()
                }
            }
            else
            {
                std::cerr
                    << "Could not initialize port #" << portnumber << "!"
                    << std::endl
                    ;
                had_error = true;
            }
        }
    }
    return had_error ? EXIT_FAILURE : EXIT_SUCCESS ;
}

/*
 * busout.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

