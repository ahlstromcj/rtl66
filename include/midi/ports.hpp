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
 * \updates       2026-01-26
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

    using container = std::map<midi::bussbyte, io>;

    /**
     *  Indicates if the list is to be used.  It will always be saved and read,
     *  but not used if this flag is false.  For normal I/O usage, this will
     *  be true.  For usage in portmapping (a future feature) this could be
     *  false to indicate that the mapping will not be used.
     */

    bool m_is_active { false };

#else
    using container = std::vector<midi::port>;
#endif

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
    ~ports () = default;

    bool add (const midi::port & p);
    bool add
    (
        int bussnumber,                     /* example: "14" for MIDI thru  */
        const std::string & bussname,       /* example: "Midi Through"      */
        int portnumber,                     /* example: "0"                 */
        const std::string & portname,       /* e.g. "Midi Through Port-0:   */
        midi::port::io iotype,
        midi::port::kind porttype,
        int portid,                         /* an index value from 0 on up  */
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
        int portid,                         /* an index value from 0 on up  */
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

    bool empty () const
    {
        return m_port_container.empty();
    }

    int port_count () const
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

    midi::bussbyte get_port_id (int bussnumber, int port) const;
    std::string to_string (const std::string & tagmsg = "") const;

    midi::port & portref (int index);
    const midi::port & portref (int index) const;

    int get_bus_number (int index) const
    {
        if (index < port_count())
            return portref(index).buss_number();
        else
            return (-1);
    }

    std::string get_bus_name (int index) const
    {
        if (index < port_count())
            return portref(index).buss_name();
        else
            return std::string("");
    }

    /**
     *  Get minor port number.
     */

    int get_port_number (int index) const
    {
        if (index < port_count())
            return portref(index).port_number();
        else
            return (-1);
    }

    /**
     *  Get the port index. This is weird. Using
     *  get_port_id() makes more sense.
     */

    int get_port_index (int index) const
    {
        if (index < port_count())
            return portref(index).port_index();
        else
            return (-1);
    }

    std::string get_port_name (int index) const
    {
        if (index < port_count())
            return portref(index).port_name();
        else
            return std::string("");
    }

    std::string get_port_alias (int index, int aliasno = 0) const
    {
        static std::string s_dummy;
        return index < port_count() ?
            portref(index).port_alias(aliasno) : s_dummy ;
    }

    const lib66::tokenization & get_port_aliases (int index) const
    {
        static lib66::tokenization s_dummy;
        return index < port_count() ?
            portref(index).port_aliases() : s_dummy ;
    }

    bool get_port_is_input (int index) const
    {
        if (index < port_count())
            return portref(index).io_type() == midi::port::io::input;
        else
            return false;
    }

    midi::port::kind get_port_type (int index) const
    {
        if (index < 0 || index >= port_count())
            index = 0;

        return portref(index).port_type();
    }

    /*
     * Are the next few functions useful?
     */

    bool get_port_is_virtual (int index) const
    {
        if (index < port_count())
            return portref(index).port_type() ==
                midi::port::kind::manual;
        else
            return false;
    }

    bool get_port_is_system (int index) const
    {
        if (index < port_count())
            return portref(index).port_type() ==
                midi::port::kind::system;
        else
            return false;
    }

    int get_port_queue_number (int index) const
    {
        if (index < port_count())
            return portref(index).queue_number();
        else
            return (-1);
    }

    midi::clock::clocking get_port_status (int index) const
    {
        if (index < port_count())
            return portref(index).port_status();
        else
            return midi::clock::clocking::unavailable;
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

