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
 * \file          clientinfo.cpp
 *
 *    A class for obtaining system MIDI information.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2025-08-30
 * \license       See above.
 *
 *  This class helps collect a whole bunch of system MIDI information
 *  about client/buss number, port numbers, and port names, and hold it
 *  for usage when creating midibus objects and midi_api objects.
 *
 *  Serves as an intermediary between the Seq66 midibus concept (but
 *  greatly simplified for the Rtl66 library) and the refactored RtMidi code.
 *
 * Port Refresh Idea:
 *
 *      -#  When midi :: info :: get_io_port_info() is called, copy midi ::
 *          info :: m_io_ports into m_previous_ports().
 *      -#  Detect when a MIDI port registers or unregisters.
 *      -#  Compare the old set of ports to the new set found by
 *          get_io_port_info() to find new ports or missing ports.
 */

#include <iostream>                     /* std::cerr                        */
#include <sstream>                      /* std::ostringstream               */

#include "midi/clientinfo.hpp"          /* midi::clientinfo etc.            */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */
#include "util/strfunctions.hpp"        /* util::bool_to_string()           */

namespace midi
{

/*------------------------------------------------------------------------
 * midi::clientinfo class
 *------------------------------------------------------------------------*/

/**
 *  Principal constructor. There is no initializer list.
 *  The default values of members are set in-class. See the default
 *  constructor.
 */

clientinfo::clientinfo (midi::port::io iodirection)
{
    m_cd.cd_port_type = iodirection;        /* I/O, service, or duplex      */
    m_io_ports[c_input_port_index].port_io_types(port::io::input);
    m_io_ports[c_output_port_index].port_io_types(port::io::output);
}

clientinfo::clientinfo (const client_defaults & cd)
{
    m_cd = cd;
    m_io_ports[c_input_port_index].port_io_types(port::io::input);
    m_io_ports[c_output_port_index].port_io_types(port::io::output);
}

/**
 *  Returns the number of ports of the given type.  Currently only input,
 *  output, and duplex are supported.
 */

int
clientinfo::port_count (port::io iotype) const
{
    int result = 0;
    if (iotype == port::io::duplex)
    {
        result += m_io_ports[0].get_port_count();
        result += m_io_ports[1].get_port_count();
    }
    else
        result = m_io_ports[element(iotype)].get_port_count();

    return result;
}

#if defined THIS_CODE_IS_READY

bool
clientinfo::setup_virtual_ports (int incount, int outcount)
{
    bool result = false;
    if (incount > 0)
    {
        m_io_ports[element(port::io::input)].clear();
    }
    if (outcount > 0)
    {
        m_io_ports[element(port::io::output)].clear();
    }
    return result;
}

#endif

static std::string
bool_to_yesno (bool flag)
{
    return util::bool_to_string(flag, true);
}

std::string
clientinfo::to_string (const std::string & tagmsg) const
{
    std::ostringstream os;
    std::string ports_in = io_ports(port::io::input).to_string("Inputs");
    std::string ports_out = io_ports(port::io::output).to_string("Outputs");
    os
        << tagmsg << ":\n"
        << "API:           " << api_version()  << "\n"
        << "App:           " << app_name()     << "\n"
        << "Client:        " << client_name()  << "\n"
        << "JACK MIDI:     " << bool_to_yesno(jack_midi()) << "\n"
        << "Virtual ports: " << bool_to_yesno(virtual_ports()) << "\n"
        << "Auto-connect:  " << bool_to_yesno(auto_connect()) << "\n"
        << "Port refresh:  " << bool_to_yesno(port_refresh()) << "\n"
        << "Global PPQN:   " << std::to_string(int(global_ppqn())) << "\n"
        << "Global BPM:    " << std::to_string(int(global_bpm())) << "\n"
        << "Global BW:     " <<
            std::to_string(int(global_beat_width())) << "\n"
        << "Global BPB:    " <<
            std::to_string(int(global_beats_per_bar())) << "\n"
        << ports_in
        << ports_out
        << "I/O ports #s:  " << std::to_string(input_portnumber())
        << "/" << std::to_string(output_portnumber()) << "\n"
        ;
    return os.str();
}

bool
clientinfo::get_all_port_info (rtl::rtmidi::api rapi)
{
    return midi::get_all_port_info(*this, rapi);    /* defined at bottom    */
}

/**
 *  Generates a string listing all of the ports present in the
 *  selected port container. Useful for debugging and probing.
 *  This job could also be done using port::to_string() directly!
 *
 * \param iotype
 *      Provides the port type to query, restricted to port::io::input
 *      and port::io::output.  Might try to support duplex later.
 *
 * \return
 *      Returns a multi-line ASCII string enumerating all of the ports.
 */

std::string
clientinfo::port_list (port::io iotype) const
{
    const ports & p = io_ports(iotype);
    int portcount = p.get_port_count();
    std::ostringstream os;
    os << (iotype == port::io::input ? "Inputs" : "Outputs");
    os << " ports (" << portcount << "):" << std::endl;
    for (int i = 0; i < portcount; ++i)
    {
        std::string annotation;
        if (get_port_is_virtual(iotype, i))
            annotation = "virtual";
        else if (get_port_is_system(iotype, i))
            annotation = "system";

        os
            << "  [" << i << "] "
            << get_bus_id(iotype, i) << ":" << get_port_id(iotype, i) << " "
            << get_bus_name(iotype, i) << ":" << get_port_name(iotype, i)
            ;

        if (! annotation.empty())
            os << " (" << annotation << ")";

        os << std::endl;
    }
    return os.str();
}

std::string
clientinfo::port_list () const
{
    std::string result = port_list(port::io::input);
    result += "\n";
    result += port_list(port::io::output);
    return result;
}

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

clientinfo &
global_client_info ()
{
    static clientinfo s_client_info(port::io::duplex);
    return s_client_info;
}

bool
get_global_port_info (rtl::rtmidi::api rapi)
{
    return get_all_port_info(global_client_info(), rapi);
}

bool
set_global_client_info (const clientinfo & ci)
{
    clientinfo & gci = global_client_info();
    gci = ci;
    return true;
}

bool
get_global_client_info (clientinfo & ci)
{
    const clientinfo & gci = global_client_info();
    ci = gci;
    return true;
}

/**
 *  Creates temporary rtmidi-in/out objects in order to get information
 *  on all the MIDI I/O ports currently available.
 *
 * \param ioports
 *      Provides the ports object to be filled with the MIDI port data of
 *      ports that were discovered. Note: this function does NOT preclear the
 *      ports object.
 *
 * \param rapi
 *      Provides the desired MIDI API to use.  If rtl::rtmidi::api::unspecified,
 *      then the fallback process is used to obtain the API.  This
 *      happens in the rtmidi_in and rtmidi_out constructors.
 *
 * \return
 *      TODO
 */

bool
get_all_port_info (midi::clientinfo & cinfo, rtl::rtmidi::api rapi)
{
    bool result = false;
    try
    {
        rtl::rtmidi_in midiin(rapi);
        ports & in { cinfo.io_ports(port::io::input) };
        int incount { midiin.get_io_port_info(in, false) };    /* !preclear */
        void * clihandle { nullptr };
        if (incount > 0)
        {
            // anything to do with the output port info?
        }

        rtl::rtmidi_out midiout(rapi);
        ports & out { cinfo.io_ports(port::io::output) };
        int outcount { midiout.get_io_port_info(out, false) }; /* !preclear */
        if (outcount > 0)
        {
            // anything to do with the output port info?
        }
        result = incount > 0 || outcount > 0;
        cinfo.ports_queried(true);
    }
    catch (rtl::rterror & error)
    {
        result = false;
    }
    return result;
}

}           // namespace midi

/*
 * clientinfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

