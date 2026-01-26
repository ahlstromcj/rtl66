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
 *  This module declares/defines an array of midi::bus objects.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-06-02
 * \updates       2026-01-26
 * \license       GNU GPLv2 or above
 *
 *  The busarray module defines the busarray and busarray classes so that we
 *  can start avoiding arrays and explicit access to them.
 *
 *  The busarray class holds a pointer to its midi::bus object.
 */

#include <memory>                       /* std::unique_ptr<>                */
#include <vector>                       /* for containing the bus objects   */

#include "midi/port.hpp"                /* midi::port & enum classes        */
#include "midi/clocking.hpp"            /* clock::clocking I/O enum class   */
#include "midi/message.hpp"             /* midi::message                    */
#include "midi/midibytes.hpp"           /* midi::bussbyte and other types   */

namespace midi
{

class bus;
class bus_in;
class bus_out;
class event;

/**
 *  Holds a number of busarray objects.
 */

class busarray
{

private:

    /**
     *  Got stuck in header madness somehow, so trying to hide the
     *  dependence on midi::bus, which is broken.
     */

    class container;

    /**
     *  The pointer to the actual bus-container implementation.
     */

    std::unique_ptr<container> p_impl;

    /**
     *  Holds the kind of ports stored in this container.
     *  The default is a mix of input and output (duplex).
     *  Change this with set_io_type().
     */

    midi::port::io m_io_type { midi::port::io:: duplex };

public:

    busarray ();
    busarray (const busarray &) = delete;   // default;
    busarray & operator = (const busarray &) = delete;  // default;
    busarray (busarray &&) = delete;
    busarray & operator = (busarray &&) = delete;
    ~busarray ();

    bool add (midi::bus * b);
    void clear ();
    bool initialize ();
    int count () const;
    bool bus_valid (midi::bussbyte b) const;
    midi::bus & buss (midi::bussbyte b);
    midi::bus_in & buss_in (midi::bussbyte b);
    midi::bus_out & buss_out (midi::bussbyte b);
    int client_id (midi::bussbyte b);
    bool port_active (midi::bussbyte b);

    /*
     * Functions called for all busses.
     */

    void clock_start ();
    void clock_stop ();
    void clock_continue (midi::pulse tick);
    void init_clock (midi::pulse tick);
    void set_clock (midi::clock::clocking clocktype);
    bool set_clock (midi::bussbyte b, midi::clock::clocking clocktype);
    midi::clock::clocking get_clock (midi::bussbyte b) const;

    // bool save_clock(bussbyte b, clocking clk

    void send_event
    (
        midi::bussbyte b, const midi::event * e24, midi::byte channel
    );
    void send_sysex (midi::bussbyte b, const midi::event * ev);

    std::string get_midi_bus_name (int b) const;  /* full display name!   */
    std::string get_midi_port_name (int b) const; /* without the client   */
    std::string get_midi_alias (int b) const;

    void print () const;
    void port_exit (int client, int port);
    bool set_input (midi::bussbyte b, bool inputing);
    void set_all_inputs (bool inputing);
    bool get_input (midi::bussbyte b) const;
    bool is_system_port (midi::bussbyte b) const;
    bool is_port_unavailable (midi::bussbyte b) const;
    bool is_port_locked (midi::bussbyte b) const;
    int poll_for_midi () const;
    int poll_for_midi (int portindex) const;
    bool get_midi_event (midi::event * inev);
    midi::message get_message (int portindex = RTL66_PORTS_ALL);
    int replacement_port (int b, int p);

    midi::port::io io_type () const
    {
        return m_io_type;
    }

    void set_io_type (midi::port::io iot)
    {
        m_io_type = iot;
    }

};          // class busarray

/*
 * Free functions
 */

#if defined THIS_CODE_IS_READY
extern void swap (midi::bus & buses0, midi::bus & buses1);
#endif

}           // namespace midi

#endif      // RTL66_MIDI_BUSARRAY_HPP

/*
 * busarray.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
