#if ! defined RTL66_MIDI_BUSARRAY_HPP
#define RTL66_MIDI_BUSARRAY_HPP

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
 * \file          busarray.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-06-02
 * \updates       2024-06-09
 * \license       GNU GPLv2 or above
 *
 *  The busarray module defines the busarray and busarray classes so that we can
 *  start avoiding arrays and explicit access to them.
 *
 *  The busarray class holds a pointer to its midi::bus object.
 */

#include <vector>                       /* for containing the bus objects   */

#include "midi/bus.hpp"                 /* midi::bus, clientinfo, clocking  */

namespace midi
{

class event;

/**
 *  Holds a number of busarray objects.
 */

class busarray
{

public:

    using container = std::vector<bus::pointer>;    /* see bus.hpp  */

private:

    /**
     *  The full set of busarray objects, only some of which will actually be
     *  used.
     */

    container m_container;

public:

    busarray ();
    ~busarray ();

    bool add (bus * b, clocking clock);

    /**
     *  Adds a new midi::bus object to the list.  Then the inputing value
     *  is set.  This function is meant for input ports.
     *
     *  We need to belay the initialization until later, when we know the
     *  configured inputing settings for the input ports.  So initialization
     *  has been removed from the constructor and moved to the initialize()
     *  function. However, now we know the configured status and can apply
     *  it right away.
     *
     * \param b
     *      The midi::bus to be hooked into the array of busses.
     *
     * \param inputing
     *      The input flag value for the bus.  If true, this value indicates that
     *      the user has selected this bus to be the input MIDI bus.
     *
     * \return
     *      Returns true if the bus was added successfully, though, really, it
     *      cannot fail.
     */

    bool add (bus * b, bool inputing)
    {
        return add(b, bool_to_clocking(inputing));
    }

    bool initialize ();

    int count () const
    {
        return int(m_container.size());
    }

    bool bus_valid (bussbyte b) const
    {
        return b < bussbyte(m_container.size());
    }

    bus * bus_pointer (bussbyte b)
    {
        return bus_valid(b) ? m_container[b].get() : nullptr ;
    }

    int client_id (bussbyte b)
    {
        return bus_valid(b) ? m_container[b]->client_id() : 0 ;
    }

    bool port_active (bussbyte b)
    {
        return bus_valid(b) && m_container[b]->port_enabled();
    }

    /*
     * Functions called for all busses.
     */

    void clock_start ();
    void clock_stop ();
    void clock_continue (pulse tick);
    void init_clock (pulse tick);
    void set_clock (clocking clocktype);

    /*
     * Functions called for a specific buss.
     */

    bool set_clock (bussbyte b, clocking clocktype);
    clocking get_clock (bussbyte b) const;
    void send_event (bussbyte b, const event * e24, byte channel);
    void send_sysex (bussbyte b, const event * ev);

    std::string get_midi_bus_name (int b) const;  /* full display name!   */
    std::string get_midi_port_name (int b) const; /* without the client   */
    std::string get_midi_alias (int b) const;

    void print () const;
    void port_exit (int client, int port);
    bool set_input (bussbyte b, bool inputing);
    void set_all_inputs (bool inputing);
    bool get_input (bussbyte b) const;
    bool is_system_port (bussbyte b) const;
    bool is_port_unavailable (bussbyte b) const;
    bool is_port_locked (bussbyte b) const;
    int poll_for_midi ();
    bool get_midi_event (event * inev);
    int replacement_port (int b, int p);

};          // class busarray

/*
 * Free functions
 */

#if defined THIS_CODE_IS_READY
extern void swap (bus & buses0, bus & buses1);
#endif

}           // namespace midi

#endif      // RTL66_MIDI_BUSARRAY_HPP

/*
 * busarray.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
