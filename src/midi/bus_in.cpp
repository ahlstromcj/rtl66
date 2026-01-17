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
 * \updates       2025-11-20
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/bus_in.hpp"              /* midi::bus and midi::bus_in       */
#include "midi/clientinfo.hpp"          /* midi::clientinfo class           */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */

namespace midi
{

/**
 *  The default constructor is useful in creating a dummy buss, where
 *  a midi_api-derived object is *not* created.
 */

bus_in::bus_in () :
    m_rtmidi_in (rtl::rtmidi::api::none)
{
    // no code
}

/**
 *  Creates a normal MIDI input port. See the banner for the bus_out
 *  constructor for information about the "masterbus paradigm".
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
    midi::bus   (master, index, midi::port::io::input),
    m_rtmidi_in (master)
{
    (void) queuesizelimit;                      /* masterbus::queue_size()  */
    if (not_nullptr(midi_api_ptr()))            /* midi_api object's ptr    */
    {
        (void) m_rtmidi_in.open_port(port_index(), port_name());
    }
}

/**
 *  A rote empty destructor. It avoids issues with unique_ptr.
 */

bus_in::~bus_in()
{
    // empty body
}

/**
 *  Gets port information into the class members.
 *
 *  WHY DO THIS? ci already has the port information !!!!
 */

int
bus_in::get_in_port_info ()
{
    int result { 0 };
    if (not_nullptr(master_bus()))
    {
        masterbus::info & ci { master_bus()->client_info() };
        result = m_rtmidi_in.get_io_port_info(ci.io_ports(port::io::input));
        if (result >= 0)
            get_port_items(port::io::input);
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
    bool result { false };
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

#if defined USE_MIDI_API_PTR        // undefined

/**
 *  Does checking for port_enabled() take too much time?
 */

int
bus_in::poll_for_midi () const
{
    int result { 0 };
    if (port_enabled())
    {
        if (not_nullptr(midi_api_ptr()))
            result = midi_api_ptr()->poll_for_midi();
    }
    return result;
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

midi::message
bus_in::get_message ()
{
    if (port_enabled())
    {
        return not_nullptr(midi_api_ptr()) ?
            midi_api_ptr()->get_message();
    }
    else
        return midi::message();
}

#else

int
bus_in::poll_for_midi () const
{
#if defined PLATFORM_DEBUG_TMI
    if (port_enabled())
    {
        int count { m_rtmidi_in.poll_for_midi() };
        if (count > 0)
        {
            printf("%s ", to_string().c_str());
            printf("    %d events\n", count);
        }
        return count;
    }
    else
    {
        printf("%s ", to_string().c_str());
        printf("    Not enabled\n");
        return 0;
    }
#else
    return port_enabled() ? m_rtmidi_in.poll_for_midi() : 0 ;
#endif

}

bool
bus_in::get_midi_event (event * inev)
{
    return port_enabled() ? m_rtmidi_in.get_midi_event(inev) : false ;
}

midi::message
bus_in::get_message ()
{
    return port_enabled() ? m_rtmidi_in.get_message() : midi::message() ;
}

#endif

}           // namespace midi

/*
 * bus_in.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

