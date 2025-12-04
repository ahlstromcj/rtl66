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
 * \updates       2025-12-04
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
#include "midi/poller.hpp"              /* midi::poller class               */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::find_midi_api() module      */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */
#include "xpc/kbhit.hpp"                /* xpc::kbhit_ex()                  */

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
 *  Client info.
 *
 *  Note that, currently, the port type must be duplex in order for
 *  masterbus to get all the port information.
 */

midi::client_defaults s_client_defaults
{
    RTL66_VERSION,                      /* API version (the default value)  */
    "inclient",                         /* client name (default = "rtl66")  */
    "busin",                            /* app name (default = "rtl66")     */
    false,                              /* JACK MIDI (default)              */
    false,                              /* virtual ports (default)          */
    0,                                  /* no virtual input ports (default) */
    0,                                  /* no virtual output ports (")      */
    true,                               /* auto connect (default)           */
    false,                              /* port refresh (default)           */
    4,                                  /* default global beat width        */
    4,                                  /* default global beats per bar     */
    384,                                /* global PPQN, not 192             */
    148,                                /* global BPM, not 120              */
    midi::port::io::duplex,             /* MIDI port type                   */
    -1,                                 /* queue size, a bad value          */
    false,                              /* ALSA MIDI is not threadsafe      */
    midi::c_port_null,                  /* input port number (default)      */
    midi::c_port_null                   /* output port number (default)     */
};

/**
 *  Provides an member that can be used to establish a callback. All members
 *  are defaulted except for the callback function.
 */

midi::input_specs s_input_specs
{
    false,                              /* input_active                     */
    false,                              /* input_use_sysex                  */
    false,                              /* input_use_time_code              */
    false,                              /* input_use_active_sensing         */
    false,                              /* input_using_callback             */
    nullptr,                            /* input_callback                   */
    nullptr                             /* input_user_data                  */
};

/**
 *  The port-numbers can be changed, so this item is not const.
 */

midi::clientinfo &
app_client_info ()
{
    static midi::clientinfo s_clientinfo(s_client_defaults, s_input_specs);
    return s_clientinfo;
}

/**
 *  Handles an input message. Usage of this callback is an option.
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
    size_t nbytes = msg.size();
    if (nbytes > 0)
    {
        std::string msgline { "Input:" };
        msgline += msg.to_string();
        util::status_message(msgline);
    }
    else
    {
        std::cout
            << "input callback: empty message w/delta "
            << deltatime << std::endl
            ;
    }
}

/**
 *  Provides a masterbus object, of which only a few facilties will be
 *  used, to support a single midi::bus_out object. Compare it to
 *  player::create_master_bus() and the follow-on code in launch().
 */

midi::masterbus &
master_bus (rtl::rtmidi::api rapi, midi::clientinfo & ci)
{
    static bool s_uninitialized { true };
    if (s_uninitialized)
    {
        if (rapi == rtl::rtmidi::api::unspecified)
            rapi = rtl::find_midi_api();

        /*
         * Not so sure about this. We have a relatively new input_active
         * flag in inputspecs. The clientinfo class has an I/O type flag,
         * but for the masterbus engine, it is set to duplex, so we
         * need a way to setup an input flag.
         */

        if (rt_use_callback())
            ci.set_input_callback(midibytes_callback);
    }

    static midi::masterbus s_master_bus { rapi, ci };
    if (s_uninitialized)
    {
        bool ok { rapi != rtl::rtmidi::api::unspecified };
        if (ok)
            ok = s_master_bus.setup();          /* ci */

        if (ok)
            s_uninitialized = false;
    }
    return s_master_bus;
}

#if defined USE_SUSCEPTIBLE_TEST

/**
 *  A usage that breaks (can cause segfaults) in ALSA because the RtMidi-based
 *  implementation uses a polling thread, but ALSA is not thread-safe, and thus
 *  cannot be used with a single ALSA client and multiple ports.
 */

bool
run_susceptible_test (rtl::rtmidi::api rapi, int portno)
{
    midi::masterbus & master { master_bus(rapi, app_client_info()) };
    midi::bus_in & busin { master.get_in_bus(portno) };
    bool result { busin.initialize() };
    if (result)
    {
        try
        {
            /*
             * Don't ignore sysex, timing, or active sensing
             * messages. Install an interrupt handler function.
             * Periodically check input queue.
             *
             * busin.ignore_midi_types(false, false, false);
             */

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
                    << " ... quit with any key or <Ctrl-C>."
                    << std::endl
                    ;
                xpc::clear_kb_ex();
                while (! s_is_done)
                {
                    midi::message msg { busin.get_message() };
                    if (msg.count() > 0)
                    {
                        std::string msgline { "Msg:" };
                        msgline += msg.to_string();
                        util::status_message(msgline);
                    }
                    if (xpc::kbcheck_ex())
                        break;

                    rt_test_sleep(10);  /* sleep for 10 msec    */
                }
            }
        }
        catch (const rtl::rterror & error)
        {
            std::cerr << "Caught rtl::rterror!" << std::endl;
            result = false;
        }
    }
    return result;
}

#endif

/**
 *  This function works by creating a masterbus, and looking up the
 *  bus_in object for the desired port number.
 *
 *  It requires that the poller not do anything but poll for a message.
 *  The poller runs its own input thread, and if it calls
 *  masterbus::get_message(), that means the thread in this test
 *  never (or rarely) gets a chance to grab a message.
 */

bool
poll_port (rtl::rtmidi::api rapi, int portno)
{
    midi::masterbus & master { master_bus(rapi, app_client_info()) };
    midi::bus & inbus { master.get_in_bus(portno) };
    bool result { inbus.initialize() };
    app_client_info().input_portnumber(portno);
    std::cout << "Using port #" << portno << std::endl;
    if (result)
    {
        midi::poller p(master, portno);
        result = p.launch(app_client_info());   /* global info is default   */
        if (result)
        {
            p.start_polling();
            try
            {
                midi::bus_in & busin
                {
                    dynamic_cast<midi::bus_in &>(inbus)
                };
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
                        << " ... quit with any key or <Ctrl-C>."
                        << std::endl
                        ;
                    xpc::clear_kb_ex();
                    while (! s_is_done)
                    {
                        midi::message msg { busin.get_message() };
                        if (msg.count() > 0)
                        {
                            if (msg.midi_buss() == portno)
                            {
                                std::string msgline { "Msg:" };
                                msgline += msg.to_string();
                                util::status_message(msgline);
                            }
                            else
                            {
                                std::string msgline
                                {
                                    "Msg from unselected port "
                                };
                                msgline += std::to_string(msg.midi_buss());
                                util::warn_message(msgline);
                            }
                        }
                        if (xpc::kbcheck_ex())
                            break;

                        rt_test_sleep(10);  /* sleep for 10 msec    */
                    }
                    p.stop_polling();
                }
            }
            catch (const rtl::rterror & error)
            {
                std::cerr << "Caught rtl::rterror!" << std::endl;
                result = false;
            }
        }
    }
    return result;
}

bool
poll_all_ports (rtl::rtmidi::api rapi, int portcount)
{
    (void) portcount;       // ???????????????????

    bool result { true };
    int portno { RTL66_PORTS_ALL };
    app_client_info().input_portnumber(portno);

    midi::masterbus & master { master_bus(rapi, app_client_info()) };
    midi::poller p(master, portno);
    result = p.launch(app_client_info());   /* global info is default   */
    if (result)
    {
        p.start_polling();
        try
        {
            if (rt_use_callback())
            {
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
                    << "Reading MIDI from all ports "
                    << " ... quit with any key or <Ctrl-C>."
                    << std::endl
                    ;
                xpc::clear_kb_ex();
                while (! s_is_done)
                {
                    /*
                     * Note: the default port number for
                     * masterbus::get_message(int portno)
                     * is RTL66_PORTS_ALL.
                     */

                    midi::message msg { master.get_message() };
                    if (msg.count() > 0)
                    {
                        std::string msgline { "Msg:" };
                        msgline += msg.to_string();
                        util::status_message(msgline);
                    }
                    if (xpc::kbcheck_ex())
                        break;

                    rt_test_sleep(10);  /* sleep for 10 msec    */
                }
                p.stop_polling();
            }
        }
        catch (const rtl::rterror & error)
        {
            std::cerr << "Caught rtl::rterror!" << std::endl;
            result = false;
        }
    }

    std::cout << "All-ports test" << std::endl;
    return result;
}

/**
 *  Unlike the poll_port() test function above, this version relies
 *  on poller grabbing the message and putting it in an input queue.
 *  The poll below then tries to grab the message off the queue.
 *  In this case, no bus_in object is needed.
 *
 *  This is simply another polling paradigm we are testing.
 */

bool
poll_queue (rtl::rtmidi::api rapi, int portno, bool useq = true)
{
    midi::masterbus & master { master_bus(rapi, app_client_info()) };
//  midi::bus & inbus { master.get_in_bus(portno) };
//  bool result { inbus.initialize() };
    bool result { master.is_setup() };
    app_client_info().input_portnumber(portno);
    std::cout << "Reading the queue for port #" << portno << std::endl;
    if (result)
    {
        int qsize { 32 };
        midi::poller p(master, portno, qsize);
        if (! useq)
            p.enqueue_messages(false);

        result = p.launch(app_client_info());   /* global info is default   */
        if (result)
        {
            p.start_polling();
            try
            {
                if (rt_use_callback())
                {
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
                        << "queue"          // TODO: busin.port_name()
                        << " ... quit with any key or <Ctrl-C>."
                        << std::endl
                        ;
                    xpc::clear_kb_ex();
                    while (! s_is_done)
                    {
                        midi::message msg { p.get_message() };
                        if (msg.count() > 0)
                        {
                            if (msg.midi_buss() == portno)
                            {
                                std::string msgline { "Msg:" };
                                msgline += msg.to_string();
                                util::status_message(msgline);
                            }
                            else
                            {
                                std::string msgline
                                {
                                    "Msg from unselected port "
                                };
                                msgline += std::to_string(msg.midi_buss());
                                util::warn_message(msgline);
                            }
                        }
                        if (xpc::kbcheck_ex())
                            break;

                        rt_test_sleep(10);  /* sleep for 10 msec    */
                    }
                    p.stop_polling();
                }
            }
            catch (const rtl::rterror & error)
            {
                std::cerr << "Caught rtl::rterror!" << std::endl;
                result = false;
            }
        }
    }
    return result;
}

}           // namespace anonymous

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run { rt_simple_cli("busin", argc, argv) };
    bool success { true };
    if (can_run)
    {
        cfg::set_app_name(app_client_info().app_name());
        cfg::set_client_name(app_client_info().client_name());
        int portcount { 0 };
        try
        {
            if (! rt_virtual_test_port())
            {
                if (! rt_test_port_valid(rt_test_port()))   /* check --port */
                {
                    /*
                     * rt_choose_input_port() gets the port number and also
                     * opens the port, which starts an input-thread.
                     * We call rt_choose_port_number() instead.
                     *
                     * Note: The midiout test uses rt_choose_output_port(),
                     * as does busout. These open the port, but no thread
                     * is started.
                     *
                     *  rtl::rtmidi_in midiin { rtl::rtmidi::desired_api() };
                     *  can_run = rt_choose_input_port(midiin);
                     *  int pn { rt_choose_port_number(false) // input // };
                     */

                    can_run = rt_select_input_ports(portcount);
                }
            }
        }
        catch (rtl::rterror & error)
        {
            std::cerr << "Caught rtl::rterror!" << std::endl;
            can_run = success = false;
        }
        if (can_run)
        {
            rtl::rtmidi::api rapi { rtl::rtmidi::selected_api() };
            int portno { rt_test_port() };
            if (rt_open_all_ports())
            {
                success = poll_all_ports(rapi, portcount);
            }
            else
            {
                app_client_info().input_portnumber(portno);
                if (rt_test_name() == "queue")
                    success = poll_queue(rapi, portno);         /* enqueue  */
                else if (rt_test_name() == "noqueue")
                    success = poll_queue(rapi, portno, false);  /* handle   */
                else
                    success = poll_port(rapi, portno);
            }

#if defined USE_SUSCEPTIBLE_TEST
            bool ok { run_susceptible_test(portno) };
            if (ok)
                ok = poll_port(portno);
            bool ok { poll_port(portno) };

            if (! ok)
                success = false;
#endif
        }
        else
        {
            std::cerr << "Could not choose a port!" << std::endl;
            success = false;
        }
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE ;
}

/*
 * busin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

