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
 * \file          port.cpp
 *
 *    A class for obtaining system MIDI information.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-05-24
 * \updates       2024-10-28
 * \license       See above.
 *
 *  midi::port. A class holding data about a port.  This class is meant to
 *  collect system MIDI information about client/buss number, port numbers,
 *  and port names, and hold it for usage (e.g. when creating midi::bus
 *  objects).
 *
 */

#include <sstream>                      /* std::ostringstream               */

#include "midi/port.hpp"                /* midi::ports etc.                 */

namespace midi
{

/*------------------------------------------------------------------------
 * port static functions
 *------------------------------------------------------------------------*/

static std::string
io_to_string (port::io iotype)
{
    std::string result;
    if (iotype == port::io::input)
        result = std::string("input");
    else if (iotype == port::io::output)
        result = std::string("output");
    else if (iotype == port::io::duplex)
        result = std::string("duplex");
    else if (iotype == port::io::engine)
        result = std::string("engine");

    return result;
}

static std::string
port_to_string (port::kind ptype)
{
    std::string result;
    if (ptype == port::kind::normal)
        result = std::string("normal");
    else if (ptype == port::kind::manual)
        result = std::string("virtual");
    else if (ptype == port::kind::system)
        result = std::string("system");

    return result;
}

/*------------------------------------------------------------------------
 * port
 *------------------------------------------------------------------------*/

port::port () :
    m_buss_number   (-1),
    m_buss_name     (),
    m_port_number   (-1),
    m_port_name     (),
    m_queue_number  (-1),
    m_io_type       (io::dummy),
    m_port_type     (kind::undetermined),
    m_port_alias    (),
    m_internal_id   (null_system_port_id())
{
    // No other code
}

port::port
(
    int bussnumber,
    const std::string & bussname,
    int portnumber,
    const std::string & portname,
    io iotype,
    kind porttype,
    int queuenumber,
    const std::string & alias
) :
    m_buss_number   (bussnumber),
    m_buss_name     (bussname),
    m_port_number   (portnumber),
    m_port_name     (portname),
    m_queue_number  (queuenumber),
    m_io_type       (iotype),
    m_port_type     (porttype),
    m_port_alias    (alias),
    m_internal_id   (null_system_port_id())
{
    // No other code
}

std::string
port::to_string () const
{
    std::ostringstream os;
    os
        << "Port '" << m_buss_name << "': "
        << m_buss_number << ":" << m_port_number
        << " '" << m_port_name << "' "
        << io_to_string(m_io_type) << "/" << port_to_string(m_port_type)
        ;

    if (! m_port_alias.empty())
        os << " (alias '" << m_port_alias << "')";

    os << std::endl;
    return os.str();
}

}           // namespace midi

/*
 * port.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

