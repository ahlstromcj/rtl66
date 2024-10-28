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
 * \file          bus_in.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  various MIDI APIs.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-23
 * \updates       2024-06-07
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/bus_in.hpp"              /* midi::bus and midi::bus_in       */
#include "midi/clientinfo.hpp"          /* midi::clientinfo class           */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_api [for rt_api_ptr()] */

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

bus_in::bus_in
(
    midi::masterbus & master,
    int index,
    unsigned queuesizelimit
) :
    midi::bus (master, index, midi::port::io::input),
    m_rtmidi_in
    (
        master.selected_api(),
        master.client_info()->client_name(),
        queuesizelimit
    )
{
    set_midi_api_ptr(m_rtmidi_in.rt_api_ptr());
    if (not_nullptr(midi_api_ptr()))
        midi_api_ptr()->master_bus(&master);
}

/**
 *  A rote empty destructor.
 */

bus_in::~bus_in()
{
    // empty body
}

/**
 *  Gets port information into the class members.
 */

int
bus_in::get_in_port_info ()
{
    int result = 0;
    auto mip = master_bus().client_info();
    if (mip)
    {
        result = m_rtmidi_in.get_io_port_info(mip->io_ports(port::io::input));
        if (result >= 0)
            get_port_items(mip, port::io::input);
    }
    return result;
}

/**
 *  Set status to of "inputting" to the given value.  If the parameter is
 *  true, then init_in() is called; otherwise, deinit_in() is called.
 *
 *  Should consider checking port::io m_io_type, although generally called
 *  only for input ports.
 *
 * \param inputing
 *      The inputing value to set.  For input system ports, it is always set
 *      to true, no matter how it is configured in the "rc" file.
 */

bool
bus_in::init_input (bool inputing)
{
    bool result = false;
    if (is_system_port())
    {
        activate();
        clock_type(clocking::none);
    }
    else
    {
        if (inputing)
            activate();
        else
            deactivate();

        clock_type(inputing ? clocking::none : clocking::disabled);
        result = true;
    }
    return result;
}

/**
 *  Does checking for port_enabled() take too much time?
 */

int
bus_in::poll_for_midi ()
{
    if (port_enabled())
    {
        return not_nullptr(midi_api_ptr()) ?
            midi_api_ptr()->poll_for_midi() : 0 ;
    }
    else
        return false;
}

bool
bus_in::get_midi_event (event * inev)
{
    if (port_enabled())
    {
        return not_nullptr(midi_api_ptr()) ?
            midi_api_ptr()->get_midi_event(inev) : false ;
    }
    else
        return false;
}

}           // namespace midi

/*
 * bus_in.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

