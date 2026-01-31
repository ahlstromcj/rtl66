#if ! defined RTL66_MIDI_PORTS_HPP
#define RTL66_MIDI_PORTS_HPP

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
 * \file          ports.hpp
 *
 *  A class for holding the current status of the MIDI system on the host.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-05        (seq66::midi_port_info)
 * \updates       2026-01-30
 * \license       See above.
 *
 *  We need to have a way to get all of the API information from each
 *  framework, without supporting the full API.  The buss classes require
 *  certain information to be known when they are created:
 *
 *      -   Port counts.  The number of input ports and output ports needs to
 *          be known so that we can iterate properly over them to create
 *          midibus objects.
 *      -   Port information.  We want to assemble port names just once, and
 *          never have to deal with it again (assuming that MIDI ports do not
 *          come and go during the execution of Seq66.
 *      -   Buss information.  We want to assemble buss names or numbers
 *          just once.
 *
 *  Note that, while the other midi_api-based classes access port via the port
 *  numbers assigned by the MIDI subsystem, midi::ports-based classes use the
 *  concept of an "index", which ranges from 0 to one less than the number of
 *  input or output ports.  These values are indices into a vector of
 *  port_info structures, and are easily looked up when midi::masterbus
 *  creates a midibus object.
 */

/**
 *  A potential future feature, macroed to avoid issues until it is perfected.
 *  Meant to allow detecting changes in the set of MIDI ports, and
 *  disconnecting or connecting as appropriate, if not in manual/virtual mode.
 *
 *  It is now definable (currently for test purposes) in the configuration
 *  process and in the rtmidi qmake configuration, so that it may be centrally
 *  located, because it may have implication throughout Seq66.
 *
 * #undef  RTL66_JACK_PORT_REFRESH
 */

#include <string>                       /* std::string class                */
#include <map>                          /* std::map class                   */

#include "midi/midibytes.hpp"           /* midi::bussbyte, etc.             */
#include "midi/port.hpp"                /* midi::port class                 */

namespace midi
{

/**
 *  A class for holding port information for a number of ports.
 */

class ports
{

private:

    /**
     *  A port data container. A port has three numeric identifiers: a client
     *  (buss) number and a port number from the ALSA engine, plus an index
     *  number (0 on up) assigned by this library. One operation we need is,
     *  given the client and port number, find which index it has. This is a
     *  brute-force search whether a vector or map is used. Another very
     *  common operation is getting a buss or port name based on the index
     *  number; this can be done using operator [] for either kind of
     *  container. But with a vector, ports must always be added
     *  in numerical order.
     *
     *          using container = std::vector<midi::port>;
     *
     *  The map key is the port index number, and the value is the port
     *  data.
     */

    using container = std::map<int, midi::port>;

    /**
     *  Holds the number of ports counted.
     */

    int m_port_count { 0 };

    /**
     *  Indicates the kind of ports: input, output, or both.
     *  Used in lookups such as get_all_port_info() in the
     *  clientinfo class. The dummy value effectively disables
     *  the port list.
     */

    port::io m_port_io_types { port::io::dummy };

    /**
     *  Holds information on all of the ports that were "scanned".
     */

    container m_port_container { };

public:

    ports () = default;
    ports (const ports &) = default;
    ports (ports &&) = default;
    ports & operator = (const ports &) = default;
    ports & operator = (ports &&) = default;
    virtual ~ports () = default;

    bool add
    (
        const midi::port & p,
        int index                       = (-1),
        const std::string & nickname    = ""
    );
    bool add
    (
        int bussnumber,                     /* example: "14" for MIDI thru  */
        const std::string & bussname,       /* example: "Midi Through"      */
        int portnumber,                     /* example: "0"                 */
        const std::string & portname,       /* e.g. "Midi Through Port-0:   */
        midi::port::io iotype,
        midi::port::kind porttype,
        int portindex,                      /* an index value from 0 on up  */
        int queuenumber             = (-1)
    );
    bool add                                /* useful for JACK aliases      */
    (
        const std::string & fullname,       /* "clientname:portname         */
        const lib66::tokenization & aliases,
        int bussnumber,                     /* example: "14" for MIDI thru  */
        int portnumber,                     /* example: "0"                 */
        midi::port::io iotype,
        midi::port::kind porttype,
        int portindex,                      /* an index value from 0 on up  */
        int queuenumber             = (-1)
    );

    /**
     *  This function is useful in replacing the discovered system ports with
     *  the manual/virtual ports added in "manual" mode.
     */

    void clear ()
    {
        m_port_container.clear();
        m_port_count = 0;
    }

    bool empty () const                 // not_empty()
    {
        return m_port_container.empty();
    }

    int port_count () const             // count()
    {
        return m_port_count;
    }

    port::io port_io_types () const
    {
        return m_port_io_types;
    }

    void port_io_types (port::io iotype)
    {
        m_port_io_types = iotype;
    }

    /*
     * The next functions are similar to these functions in midiapi, except
     * that they apply to this whole list instead of a single port.
     */

    bool are_input () const
    {
        return m_port_io_types == midi::port::io::input ||
            m_port_io_types == midi::port::io::duplex;
    }

    bool are_output () const
    {
        return m_port_io_types == midi::port::io::output ||
            m_port_io_types == midi::port::io::duplex;
    }

    bool are_duplex () const
    {
        return m_port_io_types == midi::port::io::duplex;
    }

    std::string to_string (const std::string & tagmsg = "") const;

    midi::port & portref (int index);
    const midi::port & portref (int index) const;

    int get_bus_number (int index) const
    {
        return portref(index).buss_number();
    }

    std::string get_bus_name (int index) const
    {
        return portref(index).buss_name();
    }

    /**
     *  Get minor port number.
     */

    int get_port_number (int index) const
    {
        return portref(index).port_number();
    }

    midi::bussbyte get_port_index (int bussnumber, int port) const;

    int get_port_index (int index) const
    {
        return portref(index).port_index();
    }

    std::string get_port_name (int index) const
    {
        return portref(index).port_name();
    }

    std::string get_port_alias (int index, int aliasno = 0) const
    {
        return portref(index).port_alias(aliasno);
    }

    const lib66::tokenization & get_port_aliases (int index) const
    {
        return portref(index).port_aliases();
    }

    bool get_port_is_input (int index) const
    {
        return portref(index).io_type() == midi::port::io::input;
    }

    midi::port::kind get_port_type (int index) const
    {
        return portref(index).port_type();
    }

    /*
     * Are the next few functions useful?
     */

    bool get_port_is_virtual (int index) const
    {
        return portref(index).port_type() == midi::port::kind::manual;
    }

    bool get_port_is_system (int index) const
    {
        return portref(index).port_type() == midi::port::kind::system;
    }

    int get_port_queue_number (int index) const
    {
        return portref(index).queue_number();
    }

    midi::clock::clocking get_port_status (int index) const
    {
        return portref(index).port_status();
    }

    std::string get_connect_name (int index) const;

protected:

    container & port_container ()
    {
        return m_port_container;
    }

    const container & port_container () const
    {
        return m_port_container;
    }

};          // class ports

}           // namespace midi

#endif      // RTL66_MIDI_PORTS_HPP

/*
 * ports.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

