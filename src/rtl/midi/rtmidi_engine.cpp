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
 * \file          rtmidi_engine.cpp
 *
 *    A reworking of RtMidi.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-12-15
 * \updates       2025-10-30
 * \license       See above.
 *
 */

#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "midi/ports.hpp"               /* midi::ports class                */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::try_open_midi_api()         */
#include "rtl/midi/rtmidi_engine.hpp"   /* rtl::rtmidi_engine class         */

namespace rtl
{

/**
 *  Constructor that allows an optional api and a midi::masterbus.
 *
 * \param mbus
 *      Provides midi::clientinfo that provides the PPQN, BPM, client
 *      name, etc. The client name will be used to group the ports that
 *      are created by the application.
 *
 * \param rapi
 *      An optional API id can be specified.  The default is
 *      rtl::rtmidi::api::unspecified, which will cause a lookup and
 *      perhaps a fallback to the default API (JACK to ALSA, for
 *      example.
 */

rtmidi_engine::rtmidi_engine
(
    midi::masterbus & mbus,
    rtmidi::api rapi
) :
    rtmidi          (),
    m_master_bus    (mbus)              /* accessor master_bus()            */
{
    const std::string & cname { mbus.client_info().client_name() };
    rapi = ctor_common_setup(rapi, cname);
    if (open_midi_api(rapi, cname, master_bus().queue_size()))
        rtmidi::selected_api(rapi);
}

/**
 *  This function accesses only the supported rtl::rtmidi::api values, and creates
 *  a new "midi_in_xxx" object only if a match is found. Note that, under
 *  Linux, JACK is tried first.
 *
 *  For Linux, there is a fallback process.  If there is no API specified,
 *  then the APIs are tried in the following order:
 *
 *      -   PipeWire (TODO! Currently it is a dummy implementation.)
 *      -   JACK
 *      -   ALSA
 */

bool
rtmidi_engine::open_midi_api
(
    rtmidi::api rapi,
    const std::string & clientname,
    unsigned qsize
)
{
    bool result { rapi != rtmidi::api::max };
    delete_rt_api_ptr();                    /* remove and nullify pointer   */
    (void) qsize;
    if (result)
    {
        rt_api_ptr
        (
            try_open_midi_api
            (
                rapi, midi::port::io::engine, clientname, qsize
            )
        );
        result = not_nullptr(rt_api_ptr());
        if (result)
            result = set_master_bus(&master_bus());
    }
    return result;
}

}           // namespace rtl

/*
 * rtmidi_engine.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

