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
 * \file          poller.cpp
 *
 *  This module defines the base class for a simple test poller of MIDI input.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom and others
 * \date          2025-11-08
 * \updates       2025-11-20
 * \license       GNU GPLv2 or above
 *
 */

#include "c_macros.h"                   /* not_nullptr macro                */
#include "midi/clientinfo.hpp"          /* midi::clientinfo global          */
#include "midi/event.hpp"               /* midi::event()                    */
#include "midi/poller.hpp"              /* midi::poller, this class         */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::find_midi_api() etc.        */
#include "util/msgfunctions.hpp"        /* util::warn_message() etc.        */
#include "xpc/daemonize.hpp"            /* xpc::signal_for_exit()           */
#include "xpc/timing.hpp"               /* xpc::microsleep(), microtime()   */

namespace midi
{

/**
 *  Principal constructor. Note that most of the members are default in
 *  the class header (i.e. "in-class").
 *
 * Callback (optional):
 *
 *      -   The application (e.g. tests/midi/busin.cpp) provides an
 *          rtmidi_in_data::callback_t static/extern function.
 *      -   It fills in a midi::input_specs structure with the callback
 *          and an optional pointer to user-data.
 *      -   This structure is copied into a midi::clientinfo object.
 *      -   That object is used in constructing a midi::masterbus.
 *          After construction, midi::masterbus::setup() is called.
 */

poller::poller (midi::masterbus & mbus, int portnumber ) :
    m_master_bus            (mbus),
    m_in_portnumber         (portnumber),       /* default is all in-ports  */
    m_condition_var         (*this)             /* private access via cv()  */
{
    const midi::clientinfo & ci { mbus.client_info() };
    m_in_portnumber = ci.input_portnumber();

    const midi::input_specs & mis { mbus.get_input_specs() };
    m_input_callback = mis.input_callback;
    m_input_using_callback = not_nullptr(m_input_callback);
}

/**
 *  The destructor sets some running flags to false, signals this condition,
 *  then joins the input and output threads if the were launched.
 *
 *  A thread that has finished executing code, but has not yet been joined is
 *  still considered an active thread of execution and is therefore joinable.
 */

poller::~poller ()
{
    (void) finish();
}

/**
 *
 */

void
poller::inner_start ()
{
    if (! done())                               /* won't start when exiting */
    {
        if (! is_running())
        {
            is_running(true);                   /* part of cv()'s predicate */
            cv().signal();                      /* signal we are running    */
        }
    }
}

/**
 *
 * \param midiclock
 *      If true, indicates that the MIDI clock should be used.  The default
 *      value is false.
 */

void
poller::inner_stop (bool midiclock)
{
    is_running(false);
    (void) midiclock;               /* clockinfo().usemidiclock(midiclock); */
}

/**
 *  Creates the midi::masterbus.  We need to delay creation until launch time,
 *  so that settings can be obtained before determining just how to set up the
 *  application.
 *
 * \return
 *      Returns true if the creation succeeded, or if the buss already exists.
 */

bool
poller::setup_master_bus (clientinfo & ci)
{
    bool result { false };

    /*
     *  Find an available API.  Here, we rely on finding the fallback API,
     *  rather than a specified API. Hmmmmmm.
     *
     *      rtl::rtmidi::api midiapi { rtl::find_midi_api() };
     */

    rtl::rtmidi::api midiapi { rtl::rtmidi::selected_api() };
    if (midiapi != rtl::rtmidi::api::unspecified)
    {
        /*
         * Cannot use std::make_unique<midi::masterbus> because its copy
         * constructor is deleted.
         *
         *  Also, at this point, do we have the actual complement of
         *  inputs and clocks, as opposed to what's in the rc file?
         */

        result = master_bus().setup(ci);
    }
    return result;
}

/**
 *      return ! m_is_running;
 */

bool
poller::done () const
{
    return in_thread().done();
}

/**
 *  Creates the master MIDI buss. At the end of this function, the
 *  caller can display the ports that were found and enable/disable them.
 */

bool
poller::launch (clientinfo & ci)
{
    bool result { setup_master_bus(ci) };
    if (result)
    {
        result = activate();
        if (result)
        {
            if (m_in_portnumber >= 0)
                launch_input_thread();
        }
    }
    return result;
}

/**
 *  Creates the input thread using input_thread_func().  This might be a good
 *  candidate for a small thread class derived from a small base class.
 *  The creation of a thread can have boosted priority, but the default is
 *  no change.
 */

bool
poller::launch_input_thread ()
{
    rtl::iothread::functor threadfunc
    {
        std::bind(&poller::input_func, this)
    };
#if defined PLATFORM_DEBUG_TMI
    bool result { in_thread().launch(threadfunc) };
    if (result)
        printf("Input thread launched\n");
    else
        printf("Input thread launch failed\n");

    return result;
#else
    return in_thread().launch(threadfunc);
#endif
}

/**
 *  The rough opposite of launch(); it doesn't stop the threads.  A minor
 *  simplification for the main() routine, hides the JACK support macro.
 *  We might need to add code to stop any ongoing outputing.
 *
 *  Also gets the settings made/changed while the application was running from
 *  the mastermidibase class to here.  This action is the converse of calling
 *  the set_port_statuses() function defined in the mastermidibase module.
 *
 *  Also note that m_is_running and m_io_active are both used in the
 *  poller::synch::predicate() override.
 */

bool
poller::finish ()
{
    bool result { true };
    if (! done())
    {
        stop_polling();                     /* see notes in banner          */
        m_is_running = false;               /* set is_running() off         */
        in_thread().deactivate();           /* set the output 'done' flag   */
        cv().signal();                      /* signal the end of play       */
        (void) in_thread().finish();
#if defined PLATFORM_DEBUG_TMI
        printf("Input thread finished\n");
#endif
    }
    return result;
}

/**
 *  Performs a controlled activation of the rtmidi_engine and
 *  the logged busses.
 *
 *  Also see poller::setup_master_bus().
 */

bool
poller::activate ()
{
    bool result { master_bus().engine_activate() };
    if (result)
        result = master_bus().activate();

    return result;
}

/**
 *  This function is called by input_thread_func().  It handles certain MIDI
 *  input events.  Many of them are now handled by functions for easier reading
 *  and trouble-shooting (of MIDI clock).
 */

bool
poller::input_func ()
{
    if (xpc::set_timer_services(true))  /* wrapper for a Windows-only func. */
    {
        while (! done())
        {
            if (! poll_cycle())
            {
#if defined PLATFORM_DEBUG_TMI
                printf("leaving input func\n");
#endif
                break;
            }
        }
        xpc::set_timer_services(false);
        return true;
    }
    else
        return false;
}

/**
 *  A helper function for input_func().
 */

bool
poller::poll_cycle ()
{
#if defined PLATFORM_DEBUG_TMI
    printf("poll cycle\n");
#endif
    bool ok { false };
    bool result { ! done() };
    if (result)
    {
        if (use_all_ports())
            ok = master_bus().poll_for_midi() > 0;
        else
            ok = master_bus().poll_port(in_portnumber()) > 0;
    }
    if (ok)
    {
        do
        {
            if (done())
            {
#if defined PLATFORM_DEBUG_TMI
                printf("done!\n");
#endif
                result = false;
                break;                              /* spurious exit events */
            }

            /*
             * Here, we pass the port number logged in the constructor.
             * By default, all ports are queried (RTL66_PORTS_ALL).
             */

            midi::message incoming
            {
                master_bus().get_message(in_portnumber())
            };
            if (incoming.empty())
            {
                break;
            }
            else
            {
                midi::event ev(incoming);
#if defined PLATFORM_DEBUG_TMI
                std::string estr { ev.to_string() };        /* incoming     */
                util::status_message("MIDI event", estr);
#endif
                if (m_input_using_callback)
                {
                    /*
                     * In this simple polling class we don't care about
                     * maintaining delta-time, so it is set to zero.
                     * Use incoming or ev.get_message()?
                     */

                    input_callback()(0.0, incoming, nullptr);
                }
                if (ev.is_below_sysex())                    /* below 0xF0   */
                {
#if defined USE_MASTER_BUS
                    if (master_bus().is_dumping())         /* see banner   */
                    {
                        ev.set_timestamp(tick());
                        if (m_filter_by_channel)
                            master_bus().dump_midi_input(ev);
                        else
                            master_bus().get_track()->stream_event(ev);
                    }
#endif
                }
            }
            if (use_all_ports())
                ok = master_bus().poll_for_midi() > 0;
            else
                ok = master_bus().poll_port(in_portnumber()) > 0;

        } while (ok);

        /*
         * } while (master_bus().is_more_input());
         */
    }
    return result;
}

/**
 * EVENT_MIDI_STOP
 */

void
poller::midi_stop ()
{
    (void) auto_stop();
}

bool
poller::auto_stop ()
{
    stop_polling();
    return true;
}

void
poller::signal_quit ()
{
    stop_polling();
    xpc::signal_for_exit();
}

}           // namespace midi

/*
 * poller.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

