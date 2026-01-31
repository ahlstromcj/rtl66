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
 * \updates       2026-01-31
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
 *  -#  When midi::ports::get_io_port_info() is called, copy midi :: ports
 *      :: input_ports() and output_ports() midi :: ports :: m_previous_input
 *      and midi::ports::m_previous_output.
 *  -#  Detect when a MIDI port registers or unregisters.
 *  -#  Compare the old set of ports to the new set found by
 *      get_io_port_info() to find new ports or missing ports.
 */

#include "cpp_types.hpp"                /* V() and other functions          */

#if defined USE_CONFIGURATION
#include "cfg/settings.hpp"             /* access to rc() configuration     */
#endif

#if defined USE_MIDI_BUS
#include "midi/midibus.hpp"             /* select portmidi/rtmidi headers   */
#endif

#include "midi/portnaming.hpp"          /* midi namespace functions         */
#include "midi/ports.hpp"               /* midi::ports etc.                 */

#if defined PLATFORM_DEBUG_TMI
#include "util/msgfunctions.hpp"        /* util::info_message()             */
#endif

namespace midi
{

/*------------------------------------------------------------------------
 * ports
 *------------------------------------------------------------------------*/

/**
 *  Default constructor is declared and its members initialized in the
 *  declaration.
 */

/**
 *  Add a port to the container. Use this version if full control over
 *  the contents of the port object is desired.
 */

bool
ports::add (const port & p, int index, const std::string & nickname)
{
    int count { index >= 0 ? index : m_port_count };
    port pnew { p };
    pnew.port_index(count);
    if (nickname.empty())
    {
        const std::string & existing_nick { pnew.port_nickname() };
        if (existing_nick.empty())
        {
            std::string nick { extract_nickname(pnew.port_name()) };
            pnew.port_nickname(nick);
        }
    }
    else
        pnew.port_nickname(nickname);

    auto entry { std::make_pair(m_port_count, pnew) };
    auto result { m_port_container.insert(entry) };
    if (result.second)
        ++m_port_count;

    return result.second;
}

/**
 *  Adds a set of port information to the port container.
 *  Note that this overload does not deal with aliases; use the
 *  overload below.
 *
 * \param clientnumber
 *      Provides the client or buss number for the port.  This is a value like
 *
 * \param clientname
 *      Provides the system or user-supplied name for the client or buss.
 *
 * \param portnumber
 *      Provides the port number, usually re 0, at least in ALSA.
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
 * \param portindex
 *      The index number of the port, starting at 0.
 *
 * \param queuenumber
 *      Provides the optional queue number, if applicable.  For example, the
 *      rtl66 application grabs the client number (normally valued at 1)
 *      from the ALSA subsystem.
 *
 * \return
 *      Returns true if the addition succeeded.
 */

bool
ports::add
(
    int bussnumber,
    const std::string & bussname,
    int portnumber,
    const std::string & portname,
    port::io iotype,
    port::kind porttype,
    int portindex,
    int queuenumber
)
{
    std::string nick;
    port temp
    (
        bussnumber, bussname, portnumber, portname,
        iotype, porttype, portindex, queuenumber, nick
    );

#if defined PLATFORM_DEBUG_TMI
    bool makevirtual { porttype == port::kind::manual };
    bool makesystem { porttype == port::kind::system };
    bool makeinput {  iotype == port::io::input };
    const char * vport { makevirtual ? "virtual" : "auto" };
    const char * iport { makeinput ? "input" : "output" };
    const char * sport { makesystem ? "system" : "device" };
    char str[128];
    snprintf
    (
        str, sizeof str,
        "Added port #%d \"%s:%s\" [%d:%d] %s (%s %s %s)",
        portindex, V(clientname), V(portname),
        clientnumber, portnumber, V(alias0),        // TODO
        vport, iport, sport
    );
    (void) util::info_message(str);
#endif
    return add(temp);
}

/**
 *  This overload can add aliases, if available.
 */

bool
ports::add
(
    const std::string & fullname,
    const lib66::tokenization & aliases,
    int bussnumber,
    int portnumber,
    port::io iotype,
    port::kind porttype,
    int portindex,
    int queuenumber
)
{
    std::string bussname;
    std::string portname;
    std::string nickname;
    bool result
    {
        process_aliases(fullname, aliases, bussname, portname, nickname)
    };
    if (result)
    {
        std::string alias0 { aliases.size() > 0 ? aliases[0] : "" };
        std::string alias1 { aliases.size() > 1 ? aliases[1] : "" };
        port temp
        (
            bussnumber, bussname, portnumber, portname,
            iotype, porttype, portindex, queuenumber,
            nickname, alias0, alias1
        );
        result = add(temp);
    }
    return result;
}

/**
 *  Retrieve the index of a client:port combination (e.g. in ALSA, the output
 *  of the "aplaymidi -l" or "arecordmidi -l" commands) in the port-container.
 *
 * \param bussno
 *      Provides the buss number to look up, the major number of "bus:port".
 *
 * \param portno
 *      Provides the port number to look up, the number of a sub-port of
 *      the bus.
 *
 * \return
 *      Returns the index of the pair in the port container, which will match
 *      up with the listing one sees in the "MIDI Input" or "MIDI Clocks"
 *      pages in the "Preferences" dialog.  If not found, a -1 (i.e. the
 *      value of null_buss()] is returned.
 */

bussbyte
ports::get_port_index (int bussno, int portno) const
{
    bussbyte result { null_buss() };
    for (const auto & entry : m_port_container)
    {
        int index { entry.first };
        const midi::port & p { entry.second };
        if (p.buss_number() == bussno)
        {
            if (p.port_number() == portno)
            {
                result = bussbyte(index);
                break;
            }
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
    std::string result { get_bus_name(index) };
    if (! result.empty())
    {
        std::string pname { get_port_name(index) };
        if (! pname.empty())
        {
            result += ":";
            result += pname;
        }
    }
    return result;
}

midi::port &
ports::portref (int index)
{
    static midi::port s_dummy;
    auto it { m_port_container.find(index) };
    return it != m_port_container.end() ? it->second : s_dummy ;
}

const midi::port &
ports::portref (int index) const
{
    static midi::port s_dummy;
    auto it { m_port_container.find(index) };
    return it != m_port_container.end() ? it->second : s_dummy ;
}

/**
 *  Creates a dump of the port strings, mostly for troubleshooting.
 */

std::string
ports::to_string (const std::string & tagmsg) const
{
    std::string result;
    if (! tagmsg.empty())
    {
        result = tagmsg;
        result += ":\n";
    }
    for (const auto & information : m_port_container)
        result += information.second.to_string();

    return result;
}

}           // namespace midi

/*
 * ports.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

