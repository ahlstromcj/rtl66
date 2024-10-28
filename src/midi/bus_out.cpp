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
 * \updates       2024-06-09
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/bus_out.hpp"             /* midi::bus and midi::bus_out      */
#include "midi/clientinfo.hpp"          /* midi::clientinfo class           */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "rtl/midi/midi_api.hpp"        /* rtl::rtmidi::midi_api            */

namespace midi
{

/**
 *  Creates a normal MIDI input port.
 *
 * \param master
 *      Provides a reference to midi::masterbus.
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
        master.client_info()->client_name(),
    },
    m_last_tick (0)
{
    set_midi_api_ptr(m_rtmidi_out.rt_api_ptr());
    if (not_nullptr(midi_api_ptr()))
        midi_api_ptr()->master_bus(&master);
}

/**
 *  A rote empty destructor.
 */

bus_out::~bus_out()
{
    // empty body
}

/**
 *  Gets port information into the class members.
 */

int
bus_out::get_out_port_info ()
{
    int result = 0;
    auto mip = master_bus().client_info();
    if (mip)
    {
        result = m_rtmidi_out.get_io_port_info(mip->io_ports(port::io::output));
        if (result >= 0)
            get_port_items(mip, port::io::output);
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
    bool result = port_enabled();
    if (result)
    {
        if (clock_pos(clock_type()) && tick != 0)
        {
            clock_continue(tick);
        }
        else if (clock_mod(clock_type()) || tick == 0)
        {
            clock_start();

            /*
             * The next equation is effectively (192 / 4) * 16 * 4, or
             * 192 * 16.  Note that later we have pp16th = (192 / 4).
             * If any left-overs, wait for next beat (16th note) to clock.
             */

            pulse clock_mod_ticks = (PPQN() / 4) * get_clock_mod();
            pulse leftover = (tick % clock_mod_ticks);
            pulse starting_tick = tick - leftover;
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
bus_out::send_event (const event * e24, midi::byte channel)
{
    bool result = not_nullptr(midi_api_ptr());
    if (result)
        midi_api_ptr()->send_event(e24, channel);

    return result;
}

bool
bus_out::send_sysex (const event * e24)
{
    bool result = not_nullptr(midi_api_ptr());
    if (result)
        midi_api_ptr()->send_sysex(e24);

    return result;
}

bool
bus_out::clock_start ()
{
    bool result = not_nullptr(midi_api_ptr());
    if (result)
        midi_api_ptr()->clock_start();

    return result;
}

bool
bus_out::clock_stop ()
{
    bool result = not_nullptr(midi_api_ptr());
    if (result)
        midi_api_ptr()->clock_stop();

    return result;
}

bool
bus_out::clock_send (pulse tick)
{
    bool result = not_nullptr(midi_api_ptr());
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
    bool result = not_nullptr(midi_api_ptr());
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

