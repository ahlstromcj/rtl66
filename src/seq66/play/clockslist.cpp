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
 * \file          clockslist.cpp
 *
 *  This module defines some of the more complex functions of the clockslist.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2024-06-13
 * \license       GNU GPLv2 or above
 *
 */

#include "play/clockslist.hpp"          /* seq66::clockslist class          */
#include "util/strfunctions.hpp"        /* util::string_format() template   */

namespace seq66
{

/**
 *  Default and principal constructor defined in the header.
 */

/**
 *  Saves the clock settings read from the "rc" file so that they can be
 *  passed to the mastermidibus after it is created.  Also used in the
 *  creation of a port-map, in which the port nick-name is ultimately used to
 *  look up an index into the ports actually discovered in the system.
 *  The input string is of the following form (in ALSA), which includes the
 *  quotes:
 *
 *      1 0 "[1] 32:0 Launchpad Mini MIDI 1"
 *
 * \param bussno
 *      The buss number read from the "rc" file.
 *
 * \param clocktype
 *      The clock value read from the "rc" file.
 *
 * \param name
 *      The full name of the port, except when a port-map is being formed.
 *      Then, this is just a string version of the buss number.  If this
 *      parameter is empty, nothing is added to the list.
 *
 * \param nickname
 *      The short name for the port, normally.  This is generally the text
 *      after the last colon in the bus/port name discovered by the system.
 *      By default, it is empty.
 *
 * \return
 *      Returns true if the item was added to the list.
 */

bool
clockslist::add
(
    int bussno,
    bool available,
    midi::clocking clocktype,
    const std::string & name,
    const std::string & nickname,
    const std::string & alias
)
{
    bool result = bussno >= 0 && ! name.empty();
    if (result)
    {
        std::string portname = next_quoted_string(name);
        if (portname.empty())                   /* was already parsed       */
            portname = name;

        int pstatus = (-1);
        io ioitem;
        ioitem.io_available = available;
        if (! available)
        {
            pstatus = clock_to_int(midi::clocking::unavailable);
            ioitem.io_enabled = false;
            ioitem.out_clock = midi::clocking::unavailable;
        }
        else
        {
            ioitem.io_enabled = clocktype != midi::clocking::disabled;
            ioitem.out_clock = clocktype;
        }
        ioitem.io_name = portname;
        ioitem.io_alias = alias;
        ioitem.io_client_number = ioitem.io_port_number = pstatus;
        result = portslist::add(bussno, ioitem, nickname);
    }
    return result;
}

bool
clockslist::add_list_line (const std::string & line)
{
    int pnumber;
    int pstatus;
    std::string pname;
    bool result = parse_port_line(line, pnumber, pstatus, pname);
    if (result)
    {
        midi::clocking clocktype = int_to_clock(pstatus);
        bool available = pstatus != (-2);
        result = add(pnumber, available, clocktype, pname);
    }
    return result;
}

/**
 *  Parses a string of the form:
 *
 *      0 1 "Nickname of the Port" (nick-name or alias)
 *
 *  These lines are created by input_ or output_port_map_list().  Their
 *  format is strict.  These lines are those created in the
 *  port_map_list() function.
 *
 * \return
 *      Returns true if the line started with a number, followed by text
 *      contained inside double-quotes.
 */

bool
clockslist::add_map_line (const std::string & line)
{
    int pnumber;
    int pstatus;
    std::string pname;
    bool result = parse_port_line(line, pnumber, pstatus, pname);
    if (result)
    {
        bool available = true;
        if (pstatus == (-2))
            available = false;

        midi::clocking clocktype = int_to_clock(pstatus);
        std::string pnum = std::to_string(pnumber);
        result = add(pnumber, available, clocktype, pname, pnum); /* no alias */
    }
    return result;
}

/**
 *  Sets a single clock item, if in the currently existing range.
 *  Mostly meant for use by the Options / MIDI Input tab and configuration
 *  files.
 *
 * \param bussno
 *      The buss number, used to look up the io structure.
 *
 * \param clocktype
 *      The type of clock setting. Also used to set the enabled status.
 *
 * \return
 *      Returns true if the buss number lookup succeeded.
 */

bool
clockslist::set (midi::bussbyte bussno, midi::clocking clocktype)
{
    auto it = m_master_io.find(bussno);
    bool result = it != m_master_io.end();
    if (result)
    {
        bool enabled = clocktype != midi::clocking::disabled;
        it->second.io_enabled = enabled;
        it->second.out_clock = clocktype;
    }
    return result;
}

midi::clocking
clockslist::get (midi::bussbyte bussno) const
{
    auto it = m_master_io.find(bussno);
    return it != m_master_io.end()
        ? it->second.out_clock : midi::clocking::none ;
}

std::string
clockslist::io_list_lines () const
{
    std::string result;
    int bussno = 0;
    for (const auto & iopair : m_master_io)
    {
        const io & item = iopair.second;
        int status = clock_to_int(item.out_clock);
        result += io_line(bussno, status, item.io_name, item.io_alias);
        ++bussno;
    }
    return result;
}

/*
 * Free functions
 */

clockslist &
output_port_map ()
{
    static clockslist s_clocks_list(true);      /* flag this as a port-map  */
    return s_clocks_list;
}

#if defined USE_IOPUT_PORT_NAME_FUNCTION

/**
 *  Gets the nominal port name for the given bus, from the internal port-map
 *  object for clocks.
 */

std::string
output_port_name (midi::bussbyte b, bool addnumber)
{
    const clockslist & opm = output_port_map();
    portname style = addnumber ?  portname::full : portname::brief ;
    return opm.get_name(b, style);
}

#endif

/**
 *  Gets the port-string (e.g. "1") from the internal port-map object for
 *  clocks.
 */

midi::bussbyte
output_port_number (midi::bussbyte b)
{
    midi::bussbyte result = b;
    const clockslist & opm = output_port_map();
    std::string nickname = opm.get_nick_name(b, portname::brief);
    if (! nickname.empty())
        result = string_to_int(nickname);

    return result;
}

/**
 *  Builds the internal clockslist which holds a simplified list of nominal
 *  outputs where the io_name field of each element is the nick-ndame of the
 *  source clockslist's element, and the io_nick_name field is the index
 *  number (starting from 0) converted to a string.
 *
 *  If an alias exists, it is used in preference to the nick-name.  See
 *  the add() function.
 */

bool
build_output_port_map (const clockslist & cl)
{
    bool result = cl.not_empty();
    if (result)
    {
        clockslist & opm = output_port_map();
        opm.clear();
        int bussno = 0;
        for (const auto & iopair : cl.master_io())
        {
            const portslist::io & item = iopair.second;
            std::string number = std::to_string(bussno);
            bool available = item.io_available;
            midi::clocking ec = midi::clocking::none;
            if (! item.io_enabled)
                ec = midi::clocking::disabled;

            if (item.io_alias.empty())
            {
                result = opm.add
                (
                    bussno, available, ec, item.io_nick_name, number
                );
            }
            else
            {
                result = opm.add
                (
                    bussno, available, ec, item.io_alias, number
                );
            }
            if (! result)
            {
                opm.clear();
                break;
            }
            ++bussno;
        }
        opm.active(result);
    }
    return result;
}

void
clear_output_port_map ()
{
    clockslist & opm = output_port_map();
    opm.activate(portslist::status::cleared);
}

void
activate_output_port_map (bool flag)
{
    clockslist & opm = output_port_map();
    portslist::status s = flag ?
        portslist::status::on : portslist::status::off ;

    opm.activate(s);
}

/**
 *  If an output map exists and is not empty [see the output_port_map()
 *  function], this function looks up the nominal buss number in order to find
 *  the registered (in the '[midi-clocks-map]' section of the 'rc' file) name
 *  of this port. That name is then used to look up the actual buss number of
 *  that port as set up by the system according to existing MIDI equipment.
 *
 *  If there is an error, the caller can assemble an error message.
 *
 * \param cl
 *      Provides the clockslist that holds the actual existing MIDI output
 *      ports.
 *
 * \param seqbuss
 *      Provides the 'virtual' (nominal) buss number to be mapped to the true
 *      buss number. The 'virtual' (nominal) buss number is the number stored
 *      with each pattern in the MIDI tune, and should never change just
 *      because the set of MIDI equipment changes.  In this manner, one can
 *      easily remap the configuration to fit the setup on someone else's
 *      system.
 *
 * \return
 *      If the port map exists, the looked-up port/buss number is returned. If
 *      that port cannot be found by name, then null_buss() (0xFF) is
 *      returned.  Otherwise, the nominal buss parameter is returned, which
 *      preserves the legacy behavior of the pattern buss number. Also,
 *      null_buss() will be returned if the nomimal buss is that value.
 *      Test with the is_null_buss() function.
 */

midi::bussbyte
true_output_bus (const clockslist & cl, midi::bussbyte seqbuss)
{
    midi::bussbyte result = seqbuss;
    if (! is_null_buss(result))
    {
        const clockslist & opm = output_port_map();
        if (opm.active())
        {
            std::string shortname = opm.port_name_from_bus(seqbuss);
            if (shortname.empty())
            {
                std::string msg = util::string_format
                (
                    "Bad output buss %d", seqbuss
                );
                errprint(msg);
                result = null_buss();
            }
            else
            {
                result = cl.bus_from_alias(shortname);
                if (is_null_buss(result))
                    result = cl.bus_from_nick_name(shortname);
            }
        }
    }
    return result;
}

/**
 *  Returns a string representing the two columns of the internal clocks list.
 *  It is suitable for writing to a configuration file.  Quotes are included
 *  for readability and parse-ability.
 *
\verbatim
        0   "MIDI Port 1 Through"
        1   "Jazzy MIDI Out 1"
        2   "Jazzy MIDI Out 2"
\endverbatim
 *
 * \return
 *      Returns a string like the above.  If it is empty, the output port map
 *      is empty.
 */

std::string
output_port_map_list ()
{
    const clockslist & opm = output_port_map();
    return opm.port_map_list(true);                 /* is clock */
}

}               // namespace seq66

/*
 * clockslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

