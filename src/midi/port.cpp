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
 * \updates       2025-12-17
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
 * port. See the in-class member initializations as well.
 *------------------------------------------------------------------------*/

port::port
(
    int bussnumber,
    const std::string & bussname,
    int portnumber,
    const std::string & portname,
    io iotype,
    kind porttype,
    int portindex,
    int queuenumber,
    const std::string & nick,
    const std::string & alias0,
    const std::string & alias1
) :
    m_buss_number   (bussnumber),
    m_buss_name     (bussname),
    m_port_number   (portnumber),
    m_port_name     (portname),
    m_queue_number  (queuenumber),
    m_io_type       (iotype),
    m_port_type     (porttype),
    m_port_aliases  (),
    m_port_nickname (nick),
    m_port_index    (portindex)

    /*
     *  m_internal_id   (null_system_port_id())
     *  m_io_status     (clocking::none)
     */
{
    if (! alias0.empty())
    {
        m_port_aliases.push_back(alias0);
        if (! alias1.empty())
            m_port_aliases.push_back(alias1);
    }
}

std::string
port::to_string () const
{
    std::ostringstream os;
    os << io_to_string(io_type()) << " #" << port_index();
    if (port_type() != kind::normal)
        os << "(" << kind_to_string(port_type()) << ")";

    os
        << ": " << buss_number() << ":" << port_number() << " "
        << buss_name() << ":" << port_name()
        ;
    if (! port_nickname().empty())
        os << " (" << m_port_nickname << ")" << std::endl;
    else
        os << std::endl;

    if (! port_aliases().empty())
    {
        os << "              " << port_alias(0) << "\n";
        if (port_aliases().size() > 1)
            os << "              " << port_alias(1) << std::endl;
    }
    else
        os << std::endl;

    return os.str();
}

/*------------------------------------------------------------------------
 * port free functions
 *------------------------------------------------------------------------*/

std::string
io_to_string (port::io iotype)
{
    std::string result;
    if (iotype == port::io::input)
        result = std::string("Input");
    else if (iotype == port::io::output)
        result = std::string("Output");
    else if (iotype == port::io::duplex)
        result = std::string("Duplex");
    else if (iotype == port::io::engine)
        result = std::string("Engine");
    else if (iotype == port::io::dummy)
        result = std::string("Dummy");

    return result;
}

std::string
kind_to_string (port::kind ptype)
{
    std::string result;
    if (ptype == port::kind::normal)
        result = std::string("Normal");
    else if (ptype == port::kind::manual)
        result = std::string("Virtual");
    else if (ptype == port::kind::system)
        result = std::string("System");

    return result;
}

}           // namespace midi

/*
 * port.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

