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
 * \updates       2025-09-05
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
    midi::bus       (master, index, midi::port::io::output),
    m_rtmidi_out    (master),
    m_last_tick     (0)
{
    if (not_nullptr(midi_api_ptr()))            /* midi_api object's ptr    */
    {
        (void) m_rtmidi_out.open_port(port_index(), port_name());
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
bus_out::init_clock (midi::pulse tick)
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

            midi::pulse clock_mod_ticks { (PPQN() / 4) * get_clock_mod() };
            midi::pulse leftover { (tick % clock_mod_ticks) };
            midi::pulse starting_tick { tick - leftover };
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
    return midi_out().send_byte(evbyte);
}

bool
bus_out::send_event (const midi::event * e24, midi::byte channel) const
{
    return midi_out().send_event(e24, channel);
}

bool
bus_out::send_message (const midi::message & msg) const
{
    return midi_out().send_message(msg);
}

bool
bus_out::send_message (const midi::bytes & msg) const
{
    return midi_out().send_message(msg);
}

bool
bus_out::send_message (const midi::byte * msg, size_t sz) const
{
    return midi_out().send_message(msg, sz);
}

bool
bus_out::send_sysex (const midi::event * e24) const
{
    return midi_out().send_sysex(e24);
}

bool
bus_out::clock_start ()
{
    return midi_out().clock_start();
}

bool
bus_out::clock_stop ()
{
    return midi_out().clock_stop();
}

bool
bus_out::clock_send (midi::pulse tick)
{
    return midi_out().clock_send(tick);
}

/**
 *  This function is implemented as in Seq66's midibase::continue_from()
 *  function. See bus_out::clock_continue().
 */

bool
bus_out::clock_continue (midi::pulse tick)
{
    return midi_out().clock_continue(tick, 4); // TODO: beats);
}

}           // namespace midi

/*
 * bus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

