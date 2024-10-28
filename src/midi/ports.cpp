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
 * \file          ports.cpp
 *
 *    A class for obrtaining system MIDI information
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2024-06-12
 * \license       See above.
 *
 * Classes defined:
 *
 *      -   midi::port. A nested class holding data about a port.
 *      -   midi::ports. Holds multiple nfos and provides accessors.
 *
 *  These classes are meant to collect a whole bunch of system MIDI information
 *  about client/buss number, port numbers, and port names, and hold it
 *  for usage (e.g. when creating midibus objects).
 *
 * Port Refresh Idea:
 *
 *  -#  When midi :: ports :: get_io_port_info() is called, copy midi :: ports
 *      :: input_ports() and output_ports() midi :: ports :: m_previous_input
 *      and midi :: ports :: m_previous_output.
 *  -#  Detect when a MIDI port registers or unregisters.
 *  -#  Compare the old set of ports to the new set found by
 *      get_io_port_info() to find new ports or missing ports.
 */


#if defined USE_CONFIGURATION
#include "cfg/settings.hpp"             /* access to rc() configuration     */
#endif

#if defined USE_MIDI_BUS
#include "midi/midibus.hpp"             /* select portmidi/rtmidi headers   */
#endif

#include "midi/ports.hpp"               /* midi::ports etc.                 */

namespace midi
{

/*------------------------------------------------------------------------
 * ports
 *------------------------------------------------------------------------*/

/**
 *  Default constructor.  Instantiated just for visibility.
 */

ports::ports () :
    m_port_count        (0),
    m_port_container    ()
{
    // Empty body
}

/**
 *  Add a port to the container.
 */

bool
ports::add (const port & p)
{
    size_t count = m_port_container.size();
#if defined USE_THIS_CODE
    std::string nick = extract_nickname(p.portname());
    p.nick_name(nick);
#endif
    m_port_container.push_back(p);
    m_port_count = int(m_port_container.size());
    return m_port_count == int(count + 1);
}


/**
 *  Adds a set of port information to the port container.
 *
 * \param clientnumber
 *      Provides the client or buss number for the port.  This is a value like
 *
 * \param clientname
 *      Provides the system or user-supplied name for the client or buss.
 *
 * \param portnumber
 *      Provides the port number, usually re 0.
 *
 * \param portname
 *      Provides the system or user-supplied name for the port.
 *
 * \param iotype
 *      Indicates if the port is an input port or an output port.
 *
 * \param porttype
 *      If the system currently has no input or output port available, then we
 *      want to create a virtual port so that the application has something to
 *      work with.  In some systems, we need to create and activate a system
 *      port, such as a timer port or an ALSA announce port.  For all other
 *      ports, this value is note used.
 *
 * \param queuenumber
 *      Provides the optional queue number, if applicable.  For example, the
 *      rtl66 application grabs the client number (normally valued at 1)
 *      from the ALSA subsystem.
 *
 * \param alias
 *      In some JACK configurations an alias is available.  This lets one see
 *      the device's model name without running the a2jmidid daemon.
 */

bool
ports::add
(
    int clientnumber,
    const std::string & clientname,
    int portnumber,
    const std::string & portname,
    port::io iotype,
    port::kind porttype,
    int queuenumber,
    const std::string & alias
)
{
    port temp
    (
        clientnumber, clientname, portnumber, portname,
        iotype, porttype, queuenumber, alias
    );
    // m_port_container.push_back(temp);
    // m_port_count = int(m_port_container.size());

#if defined PLATFORM_DEBUG
    bool makevirtual = porttype == port::kind::manual;
    bool makesystem = porttype == port::kind::system;
    bool makeinput =  iotype == port::io::input;
    const char * vport = makevirtual ? "virtual" : "auto" ;
    const char * iport = makeinput ? "input" : "output" ;
    const char * sport = makesystem ? "system" : "device" ;
    char str[128];
    snprintf
    (
        str, sizeof str, "Added port \"%s:%s\" %s (%s %s %s)",
        clientname.c_str(), portname.c_str(), alias.c_str(),
        vport, iport, sport
    );
    (void) util::info_message(str);
#endif
    return add(temp);
}

#if defined USE_MIDI_BUS

/**
 *  Adds values from a midibus (actually a midibase-derived class).
 */

void
ports::add (const midibus * m)
{
    add
    (
        m->bus_id(), m->bus_name(),
        m->port_id(), m->port_name(),
        m->io_type(), m->port_type()
    );
}

#endif

/**
 *  Retrieve the index of a client:port combination (e.g. in ALSA, the output
 *  of the "aplaymidi -l" or "arecordmidi -l" commands) in the port-container.
 *
 * \param client
 *      Provides the client number.
 *
 * \param port
 *      Provides the port number, the number of a sub-port of the client.
 *
 * \return
 *      Returns the index of the pair in the port container, which will match
 *      up with the listing one sees in the "MIDI Input" or "MIDI Clocks"
 *      pages in the "Preferences" dialog.  If not found, a -1 is returned.
 */

bussbyte
ports::get_port_index (int bussnum, int portnum) const
{
    bussbyte result = null_buss();
    for (int i = 0; i < m_port_count; ++i)
    {
        if (m_port_container[i].m_buss_number != bussnum)
            continue;

        if (m_port_container[i].m_port_number == portnum)
        {
            result = bussbyte(i);
            break;
        }
    }
    return result;
}

/**
 *  Provides the bus name and port name in canonical JACK format:
 *  "busname:portname".  This function is basically the same as
 *  midibase::connect_name() function.  If either the bus name or port
 *  name are empty, then an empty string is returned.
 */

std::string
ports::get_connect_name (int index) const
{
    std::string result = get_bus_name(index);
    if (! result.empty())
    {
        std::string pname = get_port_name(index);
        if (! pname.empty())
        {
            result += ":";
            result += pname;
        }
    }
    return result;
}

/**
 *  Creates a dump of the port strings, mostly for troubleshooting.
 */

std::string
ports::to_string (const std::string & tagmsg) const
{
    std::string result = tagmsg;
    result += ":\n";
    for (const auto & information : m_port_container)
    {
        result += information.to_string();
    }
    return result;
}

}           // namespace midi

/*
 * ports.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

