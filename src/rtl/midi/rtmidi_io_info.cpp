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
 * \file          rtmidi_io_info.cpp
 *
 *    A class for obtaining MIDI port information.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2025-11-28
 * \updates       2025-11-28
 * \license       See above.
 *
 *  This class helps collect a whole bunch of system MIDI information
 *  about client/buss number, port numbers, and port names, and hold it
 *  for usage when creating midibus objects, and midi_api objects.
 *
 *  Also see the midi/clientinfo module.
 *
 */

#include <iostream>                     /* std::cerr                        */
#include <sstream>                      /* std::ostringstream               */

#include "midi/rtmidi_io_info.hpp"      /* midi::rtmidi_io_info functions   */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */

namespace midi
{

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

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
rtmidi_get_io_info
(
    midi::port::io & iotype,
    midi::ports & ioports,
    rtl::rtmidi::api rapi
)
{
    bool result { rapi != rtl::rtmidi::api::unspecified};
    if (result)
    {
        bool doduplex { iotype == midi::port::io::duplex };
        bool doinput { iotype == midi::port::io::input || doduplex };
        bool dooutput { iotype == midi::port::io::output || doduplex };
        ioports.clear();
        try
        {
            int incount { 0 };
            int outcount { 0 };
            if (doinput)
            {
                rtl::rtmidi_in midiin(rapi);
                incount = midiin.get_io_port_info(ioports, false);
                if (incount > 0)
                {
                    // anything to do with the output port info?
                }
            }

            if (dooutput)
            {
                rtl::rtmidi_out midiout(rapi);
                outcount = midiout.get_io_port_info(ioports, false);
                if (outcount > 0)
                {
                    // anything to do with the output port info?
                }
            }
            result = incount > 0 || outcount > 0;
        }
        catch (rtl::rterror & error)
        {
            result = false;
        }
    }
    return result;
}

}           // namespace midi

/*
 * rtmidi_io_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

