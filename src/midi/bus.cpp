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
 * \file          bus.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  various MIDI APIs.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2016-11-25
 * \updates       2024-06-12
 * \license       GNU GPLv2 or above
 *
 *  This file provides a cross-platform implementation of MIDI support.
 *
 *  Elements of a MIDI buss:
 *
 *      -   Client.  This is the application:  seq66, seq66portmidi, or
 *          seq66rtmidi.
 *      -   Buss.   This is the main MIDI item, such as MIDI Through (14)
 *          or TiMidity (128).  The buss numbers are provided by the system.
 *          Currently, the buss name is empty.
 *      -   Port.  This is one of the items provided by the buss, and the
 *          number usually starts at 0.  The port numbers are provided by the
 *          system.  Currently, the port name includes the buss name as
 *          provided by the system, as a single unit.
 *      -   Index.  This number is the order of the input or output MIDI
 *          device as enumerated by the system lookup code, and always starts
 *          at 0.
 *
 * rcsettings:
 *
        result = port_enabled() || rc().init_disabled_ports();
        if (rc().verbose())
 *
 */

#include "rtl/midi/midi_api.hpp"        /* rtl::midi_api base class         */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "midi/bus.hpp"                 /* midi::bus class                  */
#include "rtl/rterror.hpp"              /* rtl::rterror for null pointer    */

namespace midi
{

/**
 *  Initialize this static member.
 */

int bus::m_clock_mod = 16 * 4;

/**
 *  Creates a MIDI port, which will correspond to an existing system
 *  MIDI port, such as one provided by Timidity or a running JACK application,
 *  or a virtual port, which has a name made up by the application.  Provides
 *  a constructor with client name, buss number and name, port number and
 *  name, name of client, name of port.
 *
 *  For brevity in parameter passing, these values and others are provided by the
 *  midi::buss data object (not to be confused with midi::bus).
 *
 *  This constructor is the one that seems to be the one that is used for
 *  the MIDI input and output busses, when the [manual-ports] option is
 *  <i> not </i> in force.
 *
 * Path to port data:
 *
 *      -   masterbus. Passed in as a reference parameter. Important members:
 *          -   Selected API.
 *          -   MIDI engine handle.
 *          -   midi::clientinfo for input and output.
 *          -   PPQN and BPM
 *      -   midi::clientinfo:
 *          -   info_in().
 *          -   info_out().
 *      -   info_in/out()-io_ports().
 *      -   port::get_xxxx(index) function.  These are port- container lookup
 *          functions. Important functions include:
 *          -   get_port_count().
 *          -   get_port_index(int client, int index).
 *          -   get_bus_name(int index).
 *          -   get_queue_number(int index).
 *          -   get_connect_name(int index).
 *      -   ports::container[index]
 *      -   Desired data item
 *
 *      To summarize:
 *
 *          mastermbus --> info_in/out() --> io_ports() --> get_xxxx(index) -->
 *          container[index].<dataitem>
 *
 *      Is it worth short-cutting this process by supplying a pointer to
 *      container[index]?  Or by filling in bus members in the constructor?
 *      We have a winner!
 *
 *      midi::clientinfo calls identically-named functions in midi::ports, which
 *      validates the index and returns the value.
 *
 * \param master
 *      Provides a reference to midi::masterbus.
 *
 * \param index
 *      Provides the ordinal of this buss/port, for display purposes and for
 *      more portable lists of ports.
 *
 *  TODO: do we need a mutex?
 */

bus::bus
(
    masterbus & master,
    int index,
    port::io io_type
) :
    m_midi_api_ptr      (nullptr),
    m_master_bus        (master),
    m_initialized       (false),
    m_bus_index         (index),
    m_bus_id            (-1),
    m_port_id           (-1),
    m_clock_type        (clocking::none),
    m_io_active         (false),
    m_display_name      (),
    m_bus_name          (), // (busname),
    m_port_name         (), // (portname),
    m_port_alias        (), // (portalias),
    m_io_type           (io_type),
    m_port_type         ()  // (porttype),
{
    // no code (yet)
}

/**
 *  A rote empty destructor.
 */

bus::~bus()
{
    // empty body
}

/**
 *  Sets the pointer to the midi_api. If null, an error is thrown to bugger
 *  out immediately. We don't want to check pointers allatime.
 */

void
bus::set_midi_api_ptr (rtl::midi_api * rmap)
{
    m_midi_api_ptr = rmap;
    if (is_nullptr(rmap))
    {
        std::string msg{"bus::midi_api_ptr(nullptr)"};
        throw(rtl::rterror(msg, rtl::rterror::kind::invalid_parameter));
    }
}

/**
 *  Retrieve MIDI I/O setting from a clientinfo pointer, assumed to be
 *  properly filled already.
 */

void
bus::get_port_items (std::shared_ptr<clientinfo> mip, port::io iotype)
{
    if (mip)
    {
        int index = m_bus_index;
        m_bus_id = mip->get_bus_id(iotype, index);
        m_port_id = mip->get_port_id(iotype, index);
        m_port_name = mip->get_port_name(iotype, index);
        m_port_alias = mip->get_port_alias(iotype, index);
        m_port_type = mip->get_port_type(iotype, index);
    }
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
 * \param appname
 *      This is the name of the client, or application.  Not to be confused
 *      with the ALSA client-name, which is actually a buss or subsystem name.
 *
 * \param busname
 *      Provides the name of the sub-system, such as "Midi Through" or
 *      "TiMidity".
 *
 * \param portname
 *      Provides the name of the port.  In ALSA, this is something like
 *      "busname port X".
 */

void
bus::set_name
(
    const std::string & appname,
    const std::string & busname,
    const std::string & portname
)
{
    char name[128];
    if (is_virtual_port())
    {
        /*
         * See banner.  Let's also assign any "usr" names to the virtual ports
         * as well.
         */

        std::string bname = "TODO";              // usr().bus_name(m_bus_index);
        if (is_output_port() && ! bname.empty())
        {
            snprintf
            (
                name, sizeof name, "%s [%s]", bname.c_str(), portname.c_str()
            );
            bus_name(bname);
        }
        else
        {
            snprintf
            (
                name, sizeof name, "[%d] %d:%d %s:%s",
                bus_index(), bus_id(), port_id(),
                appname.c_str(), portname.c_str()
            );
            bus_name(appname);
            port_name(portname);
        }
    }
    else
    {
        /*
         * See banner.
         *
         * Old: std::string bname = usr().bus_name(port_id());
         */

        char alias[80];                                     /* was 128  */
        std::string bname = "TODO";         // usr().bus_name(m_bus_index);
        if (is_output_port() && ! bname.empty())
        {
            snprintf
            (
                alias, sizeof alias, "%s [%s]", bname.c_str(), portname.c_str()
            );
            bus_name(bname);
        }
        else if (! busname.empty())
        {
            snprintf
            (
                alias, sizeof alias, "%s:%s", busname.c_str(), portname.c_str()
            );
            bus_name(busname);              // bus_name(alias);
        }
        else
            snprintf(alias, sizeof alias, "%s", portname.c_str());

        snprintf                            /* copy the client name parts */
        (
            name, sizeof name, "[%d] %d:%d %s",
            bus_index(), bus_id(), port_id(), alias
        );
    }
    display_name(name);
}

/**
 *  Sets the name of the buss in a different way.  If the port is virtual,
 *  this function just calls set_name().  Otherwise, it reassembles the name
 *  so that it refers to a port found on the system, but modified to make it a
 *  unique application port.  For example:
 *
 *      [0] 128:0 yoshimi:midi in
 *
 *  is transformed to this:
 *
 *      [0] 128:0 seq66:yoshimi midi in
 *
 *  As a side-effect, the "short" portname is changed, from (for example)
 *  "midi in" to "yoshimi midi in".
 *
 *  This function is used only by the MIDI JACK modules.
 *
 * \param appname
 *      This is the name of the client, or application.  Not to be confused
 *      with the ALSA/JACK client-name, which is actually a buss or subsystem
 *      name.
 *
 * \param busname
 *      Provides the name of the sub-system, such as "Midi Through",
 *      "TiMidity", or "seq66".
 */

void
bus::set_alt_name
(
    const std::string & appname,
    const std::string & busname
)
{
    std::string portname = connect_name();
    if (is_virtual_port())
    {
        set_name(appname, busname, portname);
    }
    else
    {
        std::string bname = busname;
        std::string pname = portname;
        char alias[128];
        snprintf                            /* copy the client name parts */
        (
            alias, sizeof alias, "[%d] %d:%d %s",
            bus_index(), bus_id(), port_id(), pname.c_str()
        );
        bus_name(bname);
        port_name(pname);
        display_name(alias);
    }
}

/**
 * \getter m_bus_name and m_port_name
 *      Concatenates the bus and port names into a string of the form
 *      "busname:portname".  If either name is empty, an empty string is
 *      returned.
 */

std::string
bus::connect_name () const
{
    std::string result = m_bus_name;
    if (! result.empty() && ! m_port_name.empty())
    {
        result += ":";
        result += m_port_name;
    }
    return result;
}

/**
 *  Indicates if we can connect a port (even if disabled).  Used only in
 *  midi_jack_info.so far.
 */

bool
bus::is_port_connectable () const
{
    bool result = ! is_virtual_port();
    if (result)
        result = port_enabled();        //  || rc().init_disabled_ports();

    return result;
}


/**
 *  Prints m_name.
 */

void
bus::print ()
{
    printf("%s:%s", m_bus_name.c_str(), m_port_name.c_str());
}

midi::ppqn
bus::PPQN () const
{
    return not_nullptr(midi_api_ptr()) ?
        midi_api_ptr()->PPQN() : 0 ;
}

midi::bpm
bus::BPM () const
{
    return not_nullptr(midi_api_ptr()) ?
        midi_api_ptr()->BPM() : 0 ;
}

/**
 *  Need to revisit this at some point. If we enable the full set_clock(), we
 *  get two instances of each output port.
 *
 * \param clocktype
 *      The value used to set the clock-type.
 */

bool
bus::set_clock (clocking clocktype)
{
    m_clock_type = clocktype;
    m_io_active = clocktype != clocking::disabled &&
        clocktype != clocking::unavailable;

    return true;
}

/*--------------------------------------------------------------------------
 * Virtual functions
 *--------------------------------------------------------------------------*/

bool
bus::connect ()
{
    bool result = not_nullptr(midi_api_ptr());
    if (result)
    {
        result = midi_api_ptr()->connect();
    }
    else
    {
        char temp[80];
        snprintf
        (
            temp, sizeof temp, "null pointer port '%s'",
            display_name().c_str()
        );
        errprint(temp);
    }
    return result;
}

#if defined RTL66_SHOW_BUS_VALUES

/**
 *  A static debug function, enabled only for trouble-shooting.
 *
 * \param context
 *      Human readable context information (e.g. "ALSA").
 *
 * \param tick
 *      Provides the current tick value.
 */

void
bus::show_clock (const std::string & context, pulse tick)
{
    printf("%s clock [%ld]", context.c_str(), tick);
}

/**
 * Shows most bus members.
 */

void
bus::show_bus_values ()
{
    const char * vport = is_virtual_port() ? "virtual" : "non-virtual" ;
    const char * iport = is_input_port() ? "input" : "output" ;
    const char * sport = is_system_port() ? "system" : "device" ;
    printf
    (
        "client id:         %d\n"
        "bus id:            %d\n"
        "port id:           %d\n"
        "display name:      %s\n"
        "connect name:      %s\n"
        "bus : port name:   %s : %s\n"
        "bus type:          %s %s %s\n"
        "clock & enabling:  %d & %s\n"
        ,
        client_id(), bus_id(), port_id(),
        display_name().c_str(), connect_name().c_str(),
        m_bus_name.c_str(), m_port_name.c_str(),
        vport, iport, sport,
        int(get_clock_mod()), port_enabled() ? "yes" : "no"
    );
}

#endif      // defined RTL66_SHOW_BUS_VALUES

}           // namespace midi

/*
 * bus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

