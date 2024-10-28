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
 * \updates       2024-06-12
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
 *  port_info structures, and are easily looked up when midi::masterbus creates
 *  a midibus object.
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
#include <vector>                       /* std::vector class                */

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

#if defined USE_SEQ66_PORTLIST_VALUES       // thinking about this....

    using container = std::map<bussbyte, io>;

    /**
     *  Indicates if the list is to be used.  It will always be saved and read,
     *  but not used if this flag is false.  For normal I/O usage, this will
     *  be true.  For usage in portmapping (a future feature) this could be
     *  false to indicate that the mapping will not be used.
     */

    bool m_is_active;

#else
    using container = std::vector<port>;
#endif

    /**
     *  Holds the number of ports counted.
     */

    int m_port_count;

    /**
     *  Holds information on all of the ports that were "scanned".
     */

    container m_port_container;

public:

    ports ();
    ports (const ports &) = default;
    ports (ports &&) = default;
    ports & operator = (const ports &) = default;
    ports & operator = (ports &&) = default;
    ~ports () = default;

    bool add (const port & p);
    bool add
    (
        int bussnumber,                   // buss number/ID
        const std::string & bussname,     // buss name
        int portnumber,
        const std::string & portname,
        port::io iotype,
        port::kind porttype,
        int queuenumber             = (-1),
        const std::string & alias   = ""    // not always available
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

    bool empty () const
    {
        return m_port_container.empty();
    }

    int get_port_count () const
    {
        return m_port_count;
    }

    bussbyte get_port_index (int bussnumber, int port) const;
    std::string to_string (const std::string & tagmsg = "") const;

    int get_bus_id (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_buss_number;
        else
            return (-1);
    }

    std::string get_bus_name (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_buss_name;
        else
            return std::string("");
    }

    int get_port_id (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_number;
        else
            return (-1);
    }

    std::string get_port_name (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_name;
        else
            return std::string("");
    }

    std::string get_port_alias (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_alias;
        else
            return std::string("");
    }

    bool get_input (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_io_type == port::io::input;
        else
            return false;
    }

    port::kind get_port_type (int index) const
    {
        if (index < 0 || index >= get_port_count())
            index = 0;

        return m_port_container[index].m_port_type;
    }

    /*
     * Are the next few functions useful?
     */

    bool get_virtual (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_type == port::kind::manual;
        else
            return false;
    }

    bool get_system (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_type == port::kind::system;
        else
            return false;
    }

    int get_queue_number (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_queue_number;
        else
            return (-1);
    }

    std::string get_connect_name (int index) const;

};          // class ports

}           // namespace midi

#endif      // RTL66_MIDI_PORTS_HPP

/*
 * ports.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

