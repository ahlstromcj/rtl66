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
 * \file          portslist.cpp
 *
 *  This module defines some of the more complex functions of the portslist.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2026-01-30
 * \license       GNU GPLv2 or above
 *
 *  The listbase provides common code for the clockslist and inputslist
 *  classes.
 */

#include <iostream>                     /* std::cout, etc.                  */
#include <stdexcept>                    /* std::invalid_argument            */

#include "midi/portnaming.hpp"          /* midi::detect_short_name()        */
#include "play/portslist.hpp"           /* seq66::portslist class           */
#include "util/strfunctions.hpp"        /* util::string_to_int() etc.       */

namespace seq66
{

/*
 *  The simple destructor defined in the header file.  A few functions
 *  included here for better debugging.
 */

portslist::portslist (bool pmflag) :

    midi::ports     (),
    m_is_active     (false),
    m_is_port_map   (pmflag)
{
    // Nothing to do
}

void
portslist::activate (status s)
{
    m_is_active = s == status::on;
    if (s == status::cleared)
        clear();
}

#if defined USE_OBSOLETE

bool
portslist::add
(
    int bussno,
    bool available,
    int portstatus,
    const std::string & name,
    const std::string & nickname,
    const std::string & alias
)
{
    bool result = bussno >= 0 && ! name.empty();
    if (result)
    {
        io ioitem;
        int client, portno;
        if (extract_port_pair(name, client, portno))
        {
            ioitem.io_client_number = client;
            ioitem.io_port_number = portno;
        }
        else
        {
            ioitem.io_client_number = (-1);
            ioitem.io_port_number = (-1);
        }
        ioitem.io_available = available;
        ioitem.io_enabled = portstatus > 0;
        ioitem.out_clock = int_to_clock(portstatus);
        ioitem.io_name = name;
        ioitem.io_alias = alias;
        result = add(bussno, ioitem, nickname);
    }
    return result;
}

bool
portslist::add
(
    int bussno,
    io & ioitem,
    const std::string & nickname
)
{
    bool result = bussno >= 0;
    if (result)
    {
        if (nickname.empty())
        {
            std::string nick = extract_nickname(ioitem.io_name);
            ioitem.io_nick_name = nick;
        }
        else
            ioitem.io_nick_name = nickname;

        auto p = std::make_pair(midi::bussbyte(bussno), ioitem);
        m_master_io.insert(p);          // later, check the insertion
    }
    return result;
}

#endif  // defined USE_OBSOLETE

/**
 *  Added for clarity, convenience, and, last, but not least, cohesion.
 *  The issue is that this can mess up the clock type, so we leave that alone
 *  and rely on the boolean, which we should have been doing all along.
 *
 * \param portio
 *      The iterator to the io structure for the port.
 *
 * \param input
 *      The desired enabled status of the port.
 *
 * \return
 *      Returns true if the buss number lookup succeeded.
 */

bool
portslist::set_enabled (int index, bool enabled)
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
    {
        midi::clock::clocking c { midi::bool_to_clocking(enabled) };
        it->second.port_status(c);
    }
    return result;
}

bool
portslist::is_available (int index) const
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
        result = it->second.port_available();

    return result;
}

/**
 *  New rule:  whether input or output, an input value of "disabled" marks the
 *  port as missing or otherwise unusable. This is enforced in the child
 *  classes' set() functions.  The old check has some issues, in retrospect:
 *
 *      result = it->second.out_clock == midi::clock::clocking::disabled;
 */

bool
portslist::is_enabled (int index) const
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
        result = it->second.port_enabled();

    return result;
}

/**
 *  Sets the port name and nick-name.
 *
 * \param index
 *      The application's number for the buss.
 *
 * \param name
 *      Provides the port name IN WHAT FORMAT?
 *
 * \return
 *      Returns true if the buss number was found.
 */

bool
portslist::set_name (int index, const std::string & name)
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
    {
        std::string nick = midi::extract_nickname(name);
        it->second.port_name(name);
        it->second.port_nickname(nick);
    }
    return result;
}

#if defined USE_SET_NICK_NAME

bool
portslist::set_nick_name (int index, const std::string & nick)
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
        it->second.port_nickname(nick);

    return result;
}

#endif

bool
portslist::set_alias (int index, const std::string & alias)
{
    auto it { port_container().find(index) };
    bool result { it != port_container().end() };
    if (result)
        it->second.port_alias(alias);

    return result;
}

/**
 * MOVE TO portnaming
 */

static std::string
buss_string (const std::string & name, int index)
{
    std::string result;
    if (! name.empty())
    {
        result = "[" + std::to_string(int(index)) + "] " + name;
    }
    return result;
}

std::string
portslist::get_name (int index) const
{
    static std::string s_dummy;
    auto it { port_container().find(index) };
    std::string result
    {
        it != port_container().end() ? it->second.port_name() : s_dummy
    };
    return result;
}

std::string
portslist::get_nick_name (int index, midi::portnaming style) const
{
    static std::string s_dummy;
    bool addnumber { style != midi::portnaming::brief };
    auto it { port_container().find(index) };
    std::string result
    {
        it != port_container().end() ? it->second.port_nickname() : s_dummy
    };
    if (addnumber)
        result = buss_string(result, index);

    return result;
}

std::string
portslist::get_alias (int index, midi::portnaming style) const
{
    static std::string s_dummy;
    bool addnumber { style != midi::portnaming::brief };
    auto it { port_container().find(index) };
    std::string result
    {
        it != port_container().end() ? it->second.port_alias() : s_dummy
    };
    if (addnumber)
        result = buss_string(result, index);

    return result;
}

std::string
portslist::get_pair_name (int index) const
{
    std::string result;
    std::string name { get_name(index) };
    std::string nick { get_nick_name(index) };
    int client, portno;                                     /* side-effects */
    bool ok { midi::extract_port_pair(name, client, portno) };
    if (ok)
    {
        std::string pairdigits { std::to_string(client) };
        pairdigits += ":";
        pairdigits += std::to_string(portno);
        result = pairdigits + " " + nick;
    }
    else
        result = name;

    return result;
}

std::string
portslist::get_display_name (int index, midi::portnaming style) const
{
    std::string result;
    switch (style)
    {
    case midi::portnaming::brief:

        result = get_nick_name(index, style);
        break;

    case midi::portnaming::pair:

        result = get_pair_name(index);
        break;

    case midi::portnaming::full:

        result = get_name(index);
        break;

    default:

        break;
    }
    return result;
}

int
portslist::available_count () const
{
    int result { 0 };
    for (const auto & iopair : port_container())
    {
        if (iopair.second.port_available())
            ++result;
    }
    return result;
}

midi::bussbyte
portslist::bus_from_name (const std::string & nick) const
{
    midi::bussbyte result { midi::null_buss() };
    for (const auto & iopair : port_container())
    {
        if (nick == iopair.second.port_name())
        {
            result = midi::bussbyte(iopair.first);
            break;
        }
    }
    return result;
}

/**
 *  This function is used to get the buss number from the main clockslist or
 *  main inputslist, using its nick-name.
 *
 * \param nick
 *      Provides the nick-name to be looked up.  This name is obtained from
 *      the internal clockslist or (pending) inputslist by lookup given a
 *      nominal buss number.
 *
 * \return
 *      Returns the actual buss number that will be used for I/O.
 */

midi::bussbyte
portslist::bus_from_nick_name (const std::string & nick) const
{
    midi::bussbyte result { midi::null_buss() };
    for (const auto & iopair : port_container())
    {
        if (nick == iopair.second.port_nickname())
        {
            result = iopair.first;
            break;
        }
    }
    return result;
}

midi::bussbyte
portslist::bus_from_alias (const std::string & alias) const
{
    midi::bussbyte result { midi::null_buss() };
    for (const auto & iopair : port_container())
    {
        if (alias == iopair.second.port_alias())
        {
            result = iopair.first;
            break;
        }
    }
    return result;
}

/**
 *  Looks up the nick-name, which should be a string version of the nominal
 *  buss number.  Returns the port name (short name) if found in the list.
 *  This function should be used only on the internal clockslist [returned by
 *  output_port_map() in the clockslist module] or (pending) the internal
 *  inputslist.  Only these lists stored the buss number as a string.  It is a
 *  linear lookup, but the lists are short, usually a half-dozen elements.
 *
 * \param nominalbuss
 *      Provides the external, nominal buss number which is often stored in a
 *      pattern to indicate what output port is to be used.
 */

std::string
portslist::port_name_from_bus (int nominalbuss) const
{
    std::string result;
    if (midi::is_null_buss(nominalbuss))
    {
        result = "0xFF";
    }
    else
    {
        std::string nick { std::to_string(int(nominalbuss)) };
        for (const auto & iopair : port_container())
        {
            if (nick == iopair.second.port_nickname())
            {
                result = iopair.second.port_name();
                break;
            }
        }
    }
    return result;
}

/**
 *  Sets the enabled/disabled status in the destination port-list based on the
 *  statuses set in the port-map.  The port-map is what the user will see
 *  in the MIDI Clocks and Inputs tabs, and that is where the user will
 *  enable/disable the ports, if port-mapping is enabled (recommended).
 *
 *  Used to prepare the lists for showing the port-map along with the status
 *  of the disabled ports.  Each port in the port-map is looked up in the
 *  given source list.  If not found, it is unavailabl3 and hence disabled.
 *
 *  It is assumed that the "this" here is the portslist object
 *  returned by the input_port_map() or output_port_map() functions. Recall
 *  that its full-name is the nick-name of an actual port, and its nick-name
 *  is a string version of the port-number.  Too tricky... unless it works.
 *  :-)
 *
 *  Call this function when the port-map is enabled.
 *
 * \param destination
 *      The destination for the statuses to be applied.  Ultimately, the
 *      destinations are the clocks and inputs from the performer, provided by
 *      mastermidibase :: get_port_statuses(), which gets the system ports.
 *
 * \return
 *      Returns true if all the port-map entries mapped to a source port item.
 *      If false is returned, the port-map may need to be reconstructed.
 */

void
portslist::match_system_to_map (portslist & destination) const
{
    if (is_port_map())
    {
        for (const auto & iopair : port_container())
        {
            const midi::port & item { iopair.second };
            const std::string & portname { item.port_name() };  /* nickname */
            midi::port & destinitem { destination.io_block(portname) };
            if (valid(destinitem))
            {
                destinitem.port_available(true);
                destinitem.port_enabled(item.port_enabled());
                destinitem.port_status(item.port_status());
            }
            else
            {
                midi::port & ncitem { const_cast<midi::port &>(item) };
                ncitem.port_available(false);
                ncitem.port_enabled(false);
                ncitem.port_status(midi::clock::clocking::unavailable);
            }
        }
    }
}

/**
 *  The opposite of match_system_to_map(), this function takes the source
 *  portslists and makes the statuses of the map match the system (the
 *  mastermidibus).
 *
 *  Call this function when the port-map is disabled.
 */

void
portslist::match_map_to_system (const portslist & source)
{
    if (is_port_map())
    {
        for (auto & iopair : port_container())
        {
            midi::port & destinitem { iopair.second };
            const std::string & portname { destinitem.port_name() };
            const midi::port & srcitem { source.const_io_block(portname) };
            if (valid(srcitem))
            {
                destinitem.port_available(srcitem.port_available());
                destinitem.port_enabled(srcitem.port_enabled());
                destinitem.port_status(srcitem.port_status());
            }
        }
    }
}

/**
 *  Given a port-name (which might be a nick-name), this function checks if
 *  the master (internal) I/O item's nick-name or alias matches the given
 *  nick-name.  If it matches, then that internal item is returned.
 *
 * Issue:
 *
 *  The [midi-clock] name and the [midi-clock-map] nick-name might be like:
 *
 *      -   "FLUID Synth (3088150):Synth input port (3088150:0)"
 *      -   "FLUID Synth (1070760)"
 *
 */

// const portslist::io &

const midi::port &
portslist::const_io_block (const std::string & nickname) const
{
    static bool s_needs_initing { true };
    static midi::port s_dummy_io;
    if (s_needs_initing)
    {
        s_needs_initing = false;
        s_dummy_io.port_available(false);
        s_dummy_io.port_enabled(false);
        s_dummy_io.port_status(midi::clock::clocking::disabled);
    }
    for (const auto & iopair : port_container())
    {
        const midi::port & item = iopair.second;
        const std::string & comparison
        {
            item.port_alias().empty() ?
                item.port_nickname() : item.port_alias()
        };
        bool matches { util::contains(comparison, nickname) };
        if (matches)
            return item;            /* iopair.second */
    }
    return s_dummy_io;
}

/**
 *  This function is used by input_port_map_list() and output_port_map_list()
 *  in rcfile to dump the maps into the 'rc' file.
 *
 *  Compare this function to io_list_lines(). This one does not emit an alias,
 *  as port-maps don't use them directly.
 *
 * \param isclock
 *      Unfortunately, we need to determine the derived object (clockslist vs
 *      inputslist) with this freakin' flag.
 */

std::string
portslist::port_map_list (bool isclock) const
{
    std::string result;
    if (! empty())
    {
        for (const auto & iopair : port_container())
        {
            const midi::port & item { iopair.second };
            std::string pname { item.port_name() };
            int pnumber { util::string_to_int(item.port_nickname()) };
            int pstatus;
            if (isclock)
            {
                pstatus = midi::clocking_to_int(item.port_status());
            }
            else
            {
                if (! item.port_available())
                {
                    pstatus = midi::clocking_to_int
                    (
                        midi::clock::clocking::unavailable
                    );
                }
                else
                    pstatus = item.port_enabled() ? 1 : 0 ;
            }

            std::string tmp { io_line(pnumber, pstatus, pname) };
            result += tmp;
        }
    }
    return result;
}

/**
 *  Static function to parse port lines in a unified fashion.
 */

bool
portslist::parse_port_line
(
    const std::string & line,
    int & portnumber,
    int & portstatus,
    std::string & portname
)
{
    lib66::tokenization tokens = util::tokenize_quoted(line);
    bool result { tokens.size() >= 3 };     /* buss, status, & quoted name  */
    if (result)
    {
        int pnumber { util::string_to_int(tokens[0]) };
        int pstatus { util::string_to_int(tokens[1], (-1)) };
        std::string pname { tokens[2] };    /* next_quoted_string(line)     */
        portnumber = pnumber;
        portstatus = pstatus;
        portname   = pname;
    }
    return result;
}

/**
 *  Static function to test an io object for validity.  To be valid it must
 *  have a non-empty io_name field.
 */

bool
portslist::valid (const midi::port & item)
{
    return ! item.port_name().empty();
}

/**
 *  This virtual base-class function writes a port line (for the 'rc' file)
 *  from a clockslist or inputslist.  The line consists of two integers,
 *  followed by the quoted port name, and optionally followed by the alias,
 *  shown as a comment.
 */

std::string
portslist::io_line
(
    int portnumber,
    int status,
    const std::string & portname,
    const std::string & portalias
) const
{
    std::string name { util::add_quotes(portname) };
    char tmp[128];
    if (portalias.empty())
    {
        snprintf
        (
            tmp, sizeof tmp, "%2d %2d   %s\n",
            portnumber, status, V(name)
        );
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%2d %2d   %-40s  # '%s'\n",
            portnumber, status, V(name), V(portalias)
        );
    }
    return std::string(tmp);
}

std::string
portslist::to_string (const std::string & tag) const
{
    std::string result { "I/O List: '" + tag + "'\n" };
    int count { 0 };
    for (const auto & iopair : port_container())
    {
        const midi::port & item { iopair.second };
        std::string temp { std::to_string(count) + ". " };
        temp += item.port_enabled() ? "Enabled;  " : "Disabled; " ;
        if (! item.port_available())
            temp += "Unavailable ";

        temp += "Clock = " + midi::clocking_to_string(item.port_status());
        temp += "\n   ";
        temp += "Name:     " + item.port_name() + "\n  ";
        temp += "Nickname: " + item.port_nickname() + "\n  ";
        temp += "Alias:    " + item.port_alias() + "\n";
        result += temp;
        ++count;
    }
    return result;
}

void
portslist::show (const std::string & tag) const
{
    std::string listdump { to_string(tag) };
    std::cout << listdump << std::endl;
}

}               // namespace seq66

/*
 * portslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

