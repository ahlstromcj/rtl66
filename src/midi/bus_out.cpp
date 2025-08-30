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
 * \file          bus_out.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  various MIDI APIs.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-23
 * \updates       2025-08-30
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/bus_out.hpp"             /* midi::bus and midi::bus_out      */
#include "midi/clientinfo.hpp"          /* midi::clientinfo class           */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */

namespace midi
{

/**
 *  Creates a normal MIDI input port. However, rtl::rtmidi::api::none
 *  makes the rtmidi_out object not open a MIDI API (midi_api) client
 *  pointer for JACK, ALSA, etc., and not set the client name. Instead,
 *  the masterbus's API pointer is shared. Some items that can be
 *  obtained via the masterbus:
 *
 *      -   selected_api()
 *      -   client_handle()
 *      -   void_client_handle()
 *      -   engine() [rtl::rtmidi::engine reference]
 *      -   client_info()
 *      -   In and Out busarrays and their busses
 *
 * \param master
 *      Provides a reference to midi::masterbus. It's address is passed
 *      to the midi_api-derived object to activate the masterbus paradigm,
 *      where the masterbus's client pointer is used, instead of creating
 *      a client pointer for each port.
 *
 * \param index
 *      Provides the ordinal of this buss/port, mostly for display purposes.
 */

bus_out::bus_out
(
    midi::masterbus & master,
    int index
) :
    midi::bus (master, index, midi::port::io::output),
    m_rtmidi_out
    {
        master.selected_api(),
        master.client_info().client_name(),
    },
    m_last_tick (0)
{
    if (not_nullptr(midi_api_ptr()))            /* masterbus's API pointer? */
    {
        /*
         * Set up the masterbus paradigm for the input, but the
         * masterbus's API pointer will not be changed.
         */

        m_rtmidi_out.set_master_bus_ptr(&master);
    }
}

/**
 *  A rote empty destructor. It avoids issues with unique_ptr.
 */

bus_out::~bus_out ()
{
    // empty body
}

/**
 *  Gets port information into the class members.
 */

int
bus_out::get_out_port_info ()
{
    int result { 0 };
    if (not_nullptr(master_bus()))
    {
        auto & ci { master_bus()->client_info() };
        result = m_rtmidi_out.get_io_port_info(ci.io_ports(port::io::output));
        if (result >= 0)
            get_port_items(port::io::output);
    }
    return result;
}

/*------------------------------------------------------------------------
 * midi::bus overrides
 *------------------------------------------------------------------------*/

/**
 *  Initialize the clock, continuing from the given tick. This function
 *  doesn't depend upon the MIDI API in use.  Here, midi::clocking::none and
 *  midi::clocking::disabled have the same effect... none.
 *
 * \param tick
 *      The starting tick.
 */

bool
bus_out::init_clock (pulse tick)
{
    bool result { port_enabled() };
    if (result)
    {
        if (clock_is_pos(clock_type()) && tick != 0)
        {
            clock_continue(tick);
        }
        else if (clock_is_mod(clock_type()) || tick == 0)
        {
            clock_start();

            /*
             * The next equation is effectively (192 / 4) * 16 * 4, or
             * 192 * 16.  Note that later we have pp16th = (192 / 4).
             * If any left-overs, wait for next beat (16th note) to clock.
             */

            pulse clock_mod_ticks { (PPQN() / 4) * get_clock_mod() };
            pulse leftover { (tick % clock_mod_ticks) };
            pulse starting_tick { tick - leftover };
            if (leftover > 0)
                starting_tick += clock_mod_ticks;

            m_last_tick = starting_tick - 1;
        }
    }
    return result;
}

/**
 *  For speed here, we do not check if the port is enabled.
 */

bool
bus_out::send_byte (midi::byte evbyte) const
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->send_byte(evbyte);

    return result;
}

bool
bus_out::send_event (const midi::event * e24, midi::byte channel) const
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->send_event(e24, channel);

    return result;
}

bool
bus_out::send_message (const midi::message & msg) const
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->send_message(msg);

    return result;
}

bool
bus_out::send_message (const midi::bytes & msg) const
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->send_message(msg);

    return result;
}

bool
bus_out::send_message (const midi::byte * msg, size_t sz) const
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->send_message(msg, sz);

    return result;
}

bool
bus_out::send_sysex (const midi::event * e24) const
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->send_sysex(e24);

    return result;
}

bool
bus_out::clock_start ()
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->clock_start();

    return result;
}

bool
bus_out::clock_stop ()
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->clock_stop();

    return result;
}

bool
bus_out::clock_send (pulse tick)
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->clock_send(tick);

    return result;
}

/**
 *  This function is implemented as in Seq66's midibase::continue_from()
 *  function. See bus_out::clock_continue().
 */

bool
bus_out::clock_continue (pulse tick)
{
    bool result { not_nullptr(midi_api_ptr()) };
    if (result)
        midi_api_ptr()->clock_continue(tick);

    return result;
}

}           // namespace midi

/*
 * bus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

