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
 * \file          busin.cpp
 *
 *      Simple program to test MIDI input using the masterbus/bus paradigm.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2025-10-09
 * \updates       2025-10-27
 * \license       See above.
 *
 *      This application but merely opens one port and accepts messages,
 *      similarly to qmidiin.
 *
 *      On Linux, run this test both with ALSA and with JACK.
 */

#include <iostream>
#include <signal.h>                     /* is there a C++ version?          */

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "midi/bus_in.hpp"              /* midi::bus_in class               */
#include "midi/event.hpp"               /* midi::event class                */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::find_midi_api() module      */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

namespace
{

/**
 *  Provides a flag and a signal handler for setting it.
 */

bool s_is_done { false };

/**
 *  This function is called when Ctrl-C is struck.
 */

void
finish (int /*ignore*/)
{
    s_is_done = true;
}

/**
 *  Client info
 */

midi::client_defaults s_clientinfo_defaults
{
    RTL66_VERSION,                      /* API version                      */
    "inclient",                         /* client name                      */
    "read",                             /* app name                         */
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
    midi::port::io::input,              /* MIDI port type                   */
    -1,                                 /* input port number                */
    -1                                  /* output port number               */
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
 *  Handles an input message. Usage of this callback is an option.
 */

void
midibytes_callback
(
    double deltatime,                   /* always 0 in this test program    */
    midi::message * msg,
    void * userdata
)
{
    (void) userdata;
    if (not_nullptr(msg))
    {
        midi::message & m = *msg;
        deltatime = m.jack_stamp();
        size_t nbytes = m.size();
        if (nbytes > 0)
        {
            std::string msgline { "Msg:" };
            msgline += m.to_string();
            util::status_message(msgline);
        }
        else
        {
            std::cout
                << "Empty message w/delta " << deltatime << std::endl
                ;
        }
    }
}

/**
 *  Provides an override of the default masterbus::m_input_specs member
 *  that can be used to establish a callback. All members are defaulted
 *  except for the callback function.
 */

midi::masterbus::inputspecs s_input_specs_override
{
    false,                                      /* input_use_sysex          */
    false,                                      /* input_use_time_code      */
    false,                                      /* input_use_active_sensing */
    reinterpret_cast<void *>(midibytes_callback), /* input_callback         */
    nullptr                                     /* input_user_data          */
};

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

    static midi::masterbus s_master_bus { rapi };
    static bool s_uninitialized { true };
    if (s_uninitialized)
    {
        bool ok { rapi != rtl::rtmidi::api::unspecified };
        if (ok)
        {
            /*
             * The client_info_reset() call seems redundant, but
             * it is not. We need to find out why.
             */

            if (rt_use_callback())
                s_master_bus.set_inputspecs(s_input_specs_override);

            ok = s_master_bus.client_info_reset(ci);
            if (ok)
                ok = s_master_bus.engine_initialize(ci);

            if (ok)
                s_master_bus.engine_activate();
        }
        if (ok)
            s_uninitialized = false;
    }
    return s_master_bus;
}

}           // namespace anonymous

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run { rt_simple_cli("busin", argc, argv) };
    if (can_run)
    {
        cfg::set_app_name(app_client_info().app_name());
        cfg::set_client_name(app_client_info().client_name());
        try
        {
            if (! rt_virtual_test_port())
            {
                if (! rt_test_port_valid(rt_test_port()))
                {
                    rtl::rtmidi_in midiin { rtl::rtmidi::desired_api() };
                    can_run = rt_choose_input_port(midiin);
                }
            }
        }
        catch (rtl::rterror & error)
        {
            can_run = false;                        // error.print_message()
        }
        if (can_run)
        {
            int portnumber { rt_test_port() };
            app_client_info().input_portnumber(portnumber);

            rtl::rtmidi::api rapi { rtl::rtmidi::selected_api() };
            midi::masterbus & master { master_bus(rapi, app_client_info()) };
            midi::bus & inbus { master.get_in_bus(portnumber) };
            can_run = inbus.initialize();
            if (can_run)
            {
                try
                {
                    midi::bus_in & busin
                    {
                        dynamic_cast<midi::bus_in &>(inbus)
                    };

                    /*
                     * Don't ignore sysex, timing, or active sensing
                     * messages. Install an interrupt handler function.
                     * Periodically check input queue.
                     *
                     * busin.ignore_midi_types(false, false, false);
                     */

                    midi::message msg;
                    if (rt_use_callback())
                    {
                        /*
                         * Disabled, occurs too late in the process.
                         *
                         * busin.set_input_callback(&midibytes_callback);
                         */

                        std::cout
                            << "Reading MIDI input ... press <Enter> to quit.\n"
                            ;

                        char input;
                        std::cin.get(input);
                    }
                    else
                    {
                        s_is_done = false;
                        (void) signal(SIGINT, finish);
                        std::cout
                            << "Reading MIDI from port "
                            << busin.port_name()
                            << " ... quit with Ctrl-C."
                            << std::endl
                            ;
                        while (! s_is_done)
                        {
                            (void) busin.get_message(msg);
                            if (msg.count() > 0)
                            {
                                std::string msgline { "Msg:" };
                                msgline += msg.to_string();
                                util::status_message(msgline);
                            }
                            rt_test_sleep(10);  /* sleep for 10 msec    */
                        }
                    }
                }
                catch (rtl::rterror & error)
                {
                    can_run = false;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

/*
 * busin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

