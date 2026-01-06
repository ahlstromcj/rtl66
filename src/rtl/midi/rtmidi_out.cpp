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
 * \file          rtmidi_out.cpp
 *
 *    A reworking of RtMidi.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-11-03
 * \license       See above.
 *
 */

#include "midi/masterbus.hpp"               /* midi::masterbus class        */
#include "midi/ports.hpp"                   /* midi::ports class            */
#include "rtl/midi/find_midi_api.hpp"       /* rtl::try_open_midi_api()     */
#include "rtl/midi/rtmidi_out.hpp"          /* rtl::rtmidi_out class, etc.  */

namespace rtl
{

/**
 *  Attempts to open the specified API.  If no compiled support for specified
 *  API value, then issue a warning and continue as if no API was specified.
 *  Otherwise, iterate through the compiled APIs and return as soon as we find
 *  one with at least one port, or we reach the end of the list.
 *
 *  See the banner for the rtmidi_in constructor for more information.
 *
 *  \param rapi
 *      An optional API id can be specified.  The default is
 *      rtl::rtmidi::api::unspecified.
 *
 *  \param clientname
 *      An optional client name can be specified. This will be used to group
 *      the ports that are created by the application.
 */

rtmidi_out::rtmidi_out (rtmidi::api rapi, const std::string & clientname) :
    rtmidi  ()
{
    bool nomaster { rapi != rtmidi::api::none };
    if (nomaster)
    {
        rapi = ctor_common_setup(rapi, clientname);
        if (is_midiapi_valid(rapi))
        {
            if (open_midi_api(rapi, clientname))
                rtmidi::selected_api(rapi);
        }
    }
}

rtmidi_out::rtmidi_out (midi::masterbus & mb) : rtmidi ()
{
    rtmidi::api rapi { mb.selected_api() };
    const std::string & clientname { mb.client_name() };
    rapi = ctor_common_setup(rapi, clientname);
    if (is_midiapi_valid(rapi))
    {
        if (open_midi_api(mb))
            rtmidi::selected_api(rapi);
    }
    if (not_nullptr(rt_api_ptr()))
    {
        midi::masterbus * ncmb { const_cast<midi::masterbus *>(&mb) };
        if (set_master_bus_ptr(ncmb))
        {
            /*
             * We could open the port here, but we don't have
             * the port number or the port name. So this is done
             * in bus_out. Also, the initialize call is
             * already done in the midi_alsa, midi_jack, etc.
             * constructor.
             *
             *  if (rapi == rtmidi::api::alsa)
             *      (void) rt_api_ptr()->initialize(clientname);
             */
        }
    }
    else
        printf("No rtmidi_out API pointer\n");
}

rtmidi_out::~rtmidi_out ()
{
    // No code needed
}

bool
rtmidi_out::open_midi_api
(
    rtmidi::api rapi,
    const std::string & clientname,
    unsigned /* queuesize */
)
{
    bool result { rapi != rtmidi::api::max };
    delete_rt_api_ptr();
    if (result)
    {
        rt_api_ptr(try_open_midi_api(rapi, midi::port::io::output, clientname));
        result = not_nullptr(rt_api_ptr());
    }
    return result;
}

bool
rtmidi_out::open_midi_api (/* const */ midi::masterbus & mb)
{
    rtmidi::api rapi { mb.selected_api() };
    bool result { rapi != rtmidi::api::max };
    delete_rt_api_ptr();                    /* remove and nullify pointer   */
    if (result)
    {
        rt_api_ptr(try_open_midi_api(mb, midi::port::io::output));
        result = not_nullptr(rt_api_ptr());
    }
    return result;
}

/**
 *  Open a MIDI output connection.  An optional port number greater than 0
 *  can be specified. Otherwise, the default or first port found is opened.
 *  An exception is thrown if an error occurs while attempting to make
 *  the port connection.
 */

bool
rtmidi_out::open_port (int portnumber, const std::string & portname)
{
    std::string pname { portname };
    if (pname.empty())
        pname = numbered_port_name(portnumber, "rtl66 midi out");

    return rtmidi::open_port(portnumber, pname);
}

/**
 *  Create a virtual output port, with optional name, to allow software
 *  connections (OS X, JACK and ALSA only).  This function creates a virtual
 *  MIDI output port to which other software applications can connect.  This
 *  type of functionality is currently only supported by the Macintosh OS-X,
 *  Linux ALSA and JACK APIs (the function does nothing with the other APIs).
 *  An exception is thrown if an error occurs while attempting to create the
 *  virtual port.
 *
 *  TODO:  Add an overload containing a port (index) number.
 */

bool
rtmidi_out::open_virtual_port (const std::string & portname)
{
    std::string pn { portname };
    if (pn.empty())
        pn = numbered_port_name(0 /* TODO */, "rtl66 midi vout");

    return rtmidi::open_virtual_port(pn);
}

}           // namespace rtl

/*
 * rtmidi_out.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

