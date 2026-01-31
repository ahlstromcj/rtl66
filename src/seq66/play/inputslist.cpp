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
 * \file          inputslist.cpp
 *
 *  This module defines some of the more complex functions of the inputslist.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2026-01-31
 * \license       GNU GPLv2 or above
 *
 */

#include "play/inputslist.hpp"          /* seq66::inputslist class          */
#include "util/strfunctions.hpp"        /* util::string_format() template   */

namespace seq66
{

/**
 *  Default and principal constructor defined in the header.
 */

/**
 *  Saves the input settings read from the "rc" file so that they can be
 *  passed to the mastermidibus after it is created.
 *
 * \param index
 *      The ordinal buss number.
 *
 * \param available
 *      Indicates that the input is available.
 *
 * \param enabled
 *      Indicates in the input is enabled.
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
inputslist::add
(
    int index,
    midi::clock::clocking clocktype,
    const std::string & name,
    const std::string & nickname,
    const std::string & alias
)
{
    bool result { index >= 0 && ! name.empty() };
    if (result)
    {
        std::string portname { util::next_quoted_string(name) };
        if (portname.empty())                   /* was already parsed       */
            portname = name;

        midi::port ioitem;
        bool available { clocktype != midi::clock::clocking::unavailable };
        bool enabed
        {
            available && clocktype != midi::clock::clocking::disabled
        };
        ioitem.port_available(available);
        ioitem.port_enabled(enabed);
        ioitem.port_status(clocktype);
        ioitem.port_name(portname);
        ioitem.port_alias(alias);
        result = portslist::add(ioitem, index, nickname);
    }
    return result;
}

bool
inputslist::add_list_line (const std::string & line)
{
    int pnumber;
    int pstatus;
    std::string pname;
    bool result { parse_port_line(line, pnumber, pstatus, pname) };
    if (result)
    {
        midi::clock::clocking clocktype { midi::int_to_clocking(pstatus) };
//      bool available { clocktype != midi::clock::clocking::unavailable };
//      bool enabed { clocktype != midi::clock::clocking::disabled };
        result = add(pnumber, clocktype, pname);
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
inputslist::add_map_line (const std::string & line)
{
    int pnumber;
    int pstatus;
    std::string pname;
    bool result = parse_port_line(line, pnumber, pstatus, pname);
    if (result)
    {
        midi::clock::clocking clocktype { midi::int_to_clocking(pstatus) };
        std::string pnum { std::to_string(pnumber) };
        result = add(pnumber, clocktype, pname, pnum); /* no alias */
    }
    return result;
}

/**
 *  Sets a single clock item, if in the currently existing range.
 *  Mostly meant for use by the Options / MIDI Input tab and configuration
 *  files.
 *
 * \param index
 *      The buss number, used to look up the io structure.
 *
 * \param input
 *      The desired enabled status of the port.
 *
 * \return
 *      Returns true if the buss number lookup succeeded.
 */

bool
inputslist::set (int index, bool inputing)      // midi::clock::clocking ?
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
    {
        midi::clock::clocking clocktype { midi::bool_to_clocking(inputing) };
        it->second.port_enabled(inputing);
        it->second.port_status(clocktype);
    }
    return result;
}

bool
inputslist::get (int index) const
{
    auto it { port_container().find(index) };
    return it != port_container().end() ? it->second.port_enabled() : false ;
}

std::string
inputslist::io_list_lines () const
{
    std::string result;
    int index { 0 };
    for (const auto & iopair : port_container())
    {
        const midi::port & item { iopair.second };

        // TODO: fix this in seq66 too ???
        //
        // int s { item.port_enabled() ? 1 : 0 };

        int s { midi::clocking_to_int(item.port_status()) };
        result += io_line(index, s, item.port_name(), item.port_alias());
        ++index;
    }
    return result;
}

/*
 * Free functions
 */

inputslist &
input_port_map ()
{
    static inputslist s_inputs_list(true);      /* flag this as a port-map  */
    return s_inputs_list;
}

#if defined USE_IOPUT_PORT_NAME_FUNCTION

/**
 *  Gets the nominal port name for the given bus, from the internal port-map
 *  object for inputs.
 */

std::string
input_port_name (int b, bool addnumber)
{
    const inputslist & ipm { input_port_map() };
    midi::portnaming style
    {
        addnumber ?  midi::portnaming::full : midi::portnaming::brief
    };
    return ipm.get_name(b, style);
}

#endif

/**
 *  Gets the port-string (e.g. "1") from the internal port-map object for
 *  inputs.
 */

midi::bussbyte
input_port_number (int b)
{
    midi::bussbyte result { midi::bussbyte(b) };
    const inputslist & ipm { input_port_map() };
    std::string nickname { ipm.get_nick_name(b, midi::portnaming::brief) };
    if (! nickname.empty())
        result = util::string_to_int(nickname);

    return result;
}

/**
 *  Builds the internal inputslist which holds a simplified list of nominal
 *  inputs where the portname field of each element is the nick-name of the
 *  source inputslist's element, and the io_nick_name field is the index
 *  number (starting from 0) converted to a string.
 */

bool
build_input_port_map (const inputslist & il)
{
    bool result { ! il.empty() };
    if (result)
    {
        inputslist & ipm { input_port_map() };
        int index { 0 };
        ipm.clear();
        for (const auto & iopair : il.port_container())
        {
            const midi::port & item { iopair.second };
            std::string number { std::to_string(index) };
            midi::clock::clocking ec { midi::clock::clocking::none };
            if (! item.port_enabled())
                ec = midi::clock::clocking::disabled;

            if (item.port_alias().empty())
            {
                result = ipm.add
                (
                    index, ec, item.port_nickname(), number
                );
            }
            else
            {
                result = ipm.add
                (
                    index, ec, item.port_alias(), number
                );
            }

            if (! result)
            {
                ipm.clear();
                break;
            }
            ++index;
        }
        ipm.active(result);
    }
    return result;
}

void
clear_input_port_map ()
{
    inputslist & ipm { input_port_map() };
    ipm.activate(portslist::status::cleared);
}

void
activate_input_port_map (bool flag)
{
    inputslist & ipm { input_port_map() };
    portslist::status s
    {
        flag ? portslist::status::on : portslist::status::off
    };
    ipm.activate(s);
}

/**
 *  If an input map exists and is not empty [see the input_port_map()
 *  function], this function looks up the nominal buss number in order to find
 *  the registered (in the '[midi-clocks-map]' section of the 'rc' file) name
 *  of this port. That name is then used to look up the actual buss number of
 *  that port as set up by the system according to existing MIDI equipment.
 *
 *  If there is an error, the caller can assemble an error message.
 *
 * \param cl
 *      Provides the clockslist that holds the actual existing MIDI input
 *      ports.
 *
 * \param seqbuss
 *      Provides the buss number to be mapped to the true buss number. The
 *      nominal buss number is the number stored with each pattern in the
 *      tune, and should never change just because the set of MIDI equipment
 *      changes.
 *
 * \return
 *      If the port map exists, the looked-up port/buss number is returned. If
 *      that port cannot be found by name, then null_buss() (0xFF) is
 *      returned.  Otherwise, the nominal buss parameter is returned, which
 *      preserves the legacy behavior of the pattern buss number.  Also,
 *      null_buss() will be returned if the nomimal buss is that value.
 *      Test with the is_null_buss() function.
 */

midi::bussbyte
true_input_bus (const inputslist & cl, int seqbuss)
{
    midi::bussbyte result { midi::bussbyte(seqbuss) };
    if (! midi::is_null_buss(result))
    {
        const inputslist & ipm { input_port_map() };
        if (ipm.active())
        {
            std::string shortname { ipm.port_name_from_bus(seqbuss) };
            if (shortname.empty())
            {
                std::string msg
                {
                    util::string_format("Bad input buss %d", seqbuss)
                };
                errprint(msg.c_str());          // FIXME
                result = midi::null_buss();
            }
            else
            {
                result = cl.bus_from_alias(shortname);
                if (midi::is_null_buss(result))
                    result = cl.bus_from_nick_name(shortname);
            }
        }
    }
    return result;
}

/**
 *  Returns a string representing the two columns of the internal inputs list.
 *  It is suitable for writing to a configuration file.  Quotes are included
 *  for readability and parse-ability.
 *
\verbatim
        0   "MIDI Port 1 Through"
        1   "Jazzy MIDI In 1"
        2   "Jazzy MIDI In 2"
\endverbatim
 *
 * \return
 *      Returns a string like the above.  If it is empty, the input port map
 *      is empty.
 */

std::string
input_port_map_list ()
{
    const inputslist & ipm { input_port_map() };
    return ipm.port_map_list(false);                /* not clock */
}

}               // namespace seq66

/*
 * inputslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

