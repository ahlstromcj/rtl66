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
 * \file          bussdata.cpp
 *
 *    A class for specifying MIDI bus parameters.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-06-10
 * \updates       2024-10-28
 * \license       See above.
 *
 *  This class is meant to specify system MIDI information about client/buss
 *  number, buss numbers, and buss names, and hold it for usage (e.g. when
 *  creating midi::bus objects).
 *
 */

#include <sstream>                      /* std::ostringstream               */

#include "midi/bussdata.hpp"            /* midi::bussdata etc.              */

namespace midi
{

/*--------------------------------------------------------------------------
 * Static pre-constructed buss data
 *--------------------------------------------------------------------------*/

/**
 *  Make a copy of these bussdata objects and modify the copy as needed.
 */

const bussdata &
stock_in_buss_settings ()
{
    static port s_stock_port;
    static bussdata s_stock_bussdata
    (
        0, clocking::input, s_stock_port, "port 0",
        256, bussdata::ignore::none
    );
    return s_stock_bussdata;
}

const bussdata &
stock_out_buss_settings ()
{
    static port s_stock_port;
    static bussdata s_stock_bussdata;
    return s_stock_bussdata;
}

/*--------------------------------------------------------------------------
 * bussdata
 *--------------------------------------------------------------------------*/

bussdata::bussdata
(
    int index,                                  /* application's ordinal    */
    clocking c,                                 /* for use when changing    */
    int bussnumber,                             /* start of port parameters */
    const std::string & bussname,
    int portnumber,
    const std::string & portname,
    port::io iotype,
    port::kind busstype,
    int queuenumber,
    const std::string & aliasname,
    const std::string & nickname,               /* start of buss parameters */
    std::size_t queuesize,
    ignore ignoreflags
) :
    port
    (
        bussnumber,
        bussname,
        portnumber,
        portname,
        iotype,
        busstype,
        queuenumber,
        aliasname
    ),
    m_bus_index         (index),
    m_nick_name         (nickname),
    m_out_clock         (c),
    m_queue_size        (queuesize),
    m_ignore_midi_flags (ignoreflags)
{
    // No other code
}

bussdata::bussdata
(
    int index,
    clocking c,
    const port & p,
    const std::string & nickname,
    std::size_t queuesize,
    ignore ignoreflags
) :
    port                (p),
    m_bus_index         (index),
    m_nick_name         (nickname),
    m_out_clock         (c),
    m_queue_size        (queuesize),
    m_ignore_midi_flags (ignoreflags)
{
    // No other code
}

/**
 *  Sets the name of the buss by assembling the name components obtained from
 *  the system in a straightforward manner:
 *
 *      [0] 128:2 seq66:seq66 port 2
 *
 *  We want to see if the user has configured a port name. If so, and this is
 *  an output port, then the buss name is overridden by the entry in the "usr"
 *  configuration file.  Otherwise, we fall back to the parameters.  Note that
 *  this has been tweaked versus Seq24, where the "usr" devices were also
 *  applied to the input ports.  Also note that the "usr" device names should
 *  be kept short, and the actual buss name from the system is shown in
 *  brackets.
 *
 * \member m_bus_index
 *      Provides the desired ordinal for the port/buss. This starts at 0
 *      and each port will have an index number that the application can
 *      control.
 *
 * \auto appname
 *      This is the name of the client, or application.  Not to be confused
 *      with the ALSA client-name, which is actually a buss or subsystem name.
 *
 * \auto busname
 *      Provides the name of the sub-system, such as "Midi Through" or
 *      "TiMidity".
 *
 * \auto portname
 *      Provides the name of the port.  In ALSA, this is something like
 *      "busname port X".
 */

std::string
bussdata::construct_bus_name (const std::string & appname)
{
    std::string result;
    std::string busname = buss_name();
    std::string portname = port_name();
    bool is_output = io_type() == port::io::output;
    bool is_virtual = port_type() == port::kind::manual;
    char name[256];
    if (is_virtual)
    {
        /*
         * See banner.  Let's also assign any "usr" names to the virtual ports
         * as well.
         */

        if (is_output && ! busname.empty())
        {
            snprintf
            (
                name, sizeof name, "%s [%s]", busname.c_str(), portname.c_str()
            );
            result = name;
        }
        else
        {
            snprintf
            (
                name, sizeof name, "[%d] %d:%d %s:%s",
                m_bus_index, buss_number(), port_number(),
                appname.c_str(), portname.c_str()
            );
            result = name;
        }
    }
    else
    {
        char aliasname[128];
        if (is_output && ! busname.empty())
        {
            snprintf
            (
                aliasname, sizeof aliasname, "%s [%s]",
                busname.c_str(), portname.c_str()
            );
            result = name;
        }
        else if (! busname.empty())
        {
            snprintf
            (
                aliasname, sizeof aliasname, "%s:%s",
                busname.c_str(), portname.c_str()
            );
            result = name;
        }
        else
            snprintf(aliasname, sizeof aliasname, "%s", portname.c_str());

        snprintf                            /* copy the client name parts */
        (
            name, sizeof name, "[%d] %d:%d %s",
            m_bus_index, buss_number(), port_number(), aliasname
        );
        result = name;
    }
    return result;
}

/**
 *  TODO
 */

std::string
bussdata::make_nickname () const
{
    std::string result;
    if (! port_name().empty())
    {
        result = port_name();           // TODO TODO TODO
    }
    return result;
}

}           // namespace midi

/*
 * bussdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

