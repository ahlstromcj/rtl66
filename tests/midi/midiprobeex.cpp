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
 * \file          midiprobeex.cpp
 *
 *      Simple program to check MIDI inputs and outputs.
 *
 * \library       rtl66
 * \author        Gary Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2025-11-07
 * \license       See above.
 *
 */

#include <iostream>                     /* std::cout, std::cerr             */

#include "cfg/appinfo.hpp"              /* cfg::set_client_name()           */
#include "midi/masterbus.hpp"           /* rtl::rtmidi class, etc.          */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi::desired_api()       */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli(), etc.            */

namespace
{

/**
 *  Client info
 */

midi::client_defaults s_client_defaults
{
    RTL66_VERSION,                      /* API version                      */
    "midiprobe",                        /* client name                      */
    "midiprobeex",                      /* app name                         */
    false,                              /* JACK MIDI                        */
    false,                              /* virtual ports                    */
    0,                                  /* no virtual input ports           */
    0,                                  /* no virtual output ports          */
    true,                               /* auto connect                     */
    false,                              /* port refresh                     */
    4,                                  /* the default global beat width    */
    4,                                  /* the default global beats per bar */
    384,                                /* global PPQN, not 192             */
    148,                                /* global BPM, not 120              */
    midi::port::io::duplex,             /* MIDI port type                   */
    -1,                                 /* queue size, a bad value          */
    false,                              /* ALSA MIDI is not threadsafe      */
    -1,                                 /* input port number                */
    -1                                  /* output port number               */
};

/**
 *  The port-numbers can be changed, so this item is not const. The clientinfo
 *  constructor is the one that uses the default midi::input_specs member.
 */

midi::clientinfo &
app_client_info ()
{
    static midi::clientinfo s_clientinfo(s_client_defaults);
    return s_clientinfo;
}

}           // namespace anonymous

/**
 *  Main routine.
 */

int
main (int argc, char * argv [])
{
    bool can_run { rt_simple_cli("midiprobeex", argc, argv) };
    if (can_run)
    {
        cfg::set_app_name(app_client_info().app_name());
        cfg::set_client_name(app_client_info().client_name());
        rtl::rtmidi::api rapi { rtl::rtmidi::selected_api() };
        midi::masterbus mb(rapi, app_client_info());

        /*
         *  masterbus::engine_initialize() calls masterbus::client_info_reset()
         *  anyway.
         *
         *      bool ok { mb.client_info_reset() };
         *      if (ok)
         *      {
         */

        /*
         * Here, we use the overload of masterbus::engine_initialize()
         * that has no parameter. It creates/gets the global
         * clientinfo object and fills it with MIDI port information
         * and then uses that to create a midi::bus_in or midi::bus_out
         * for each port.
         */

        if (mb.engine_initialize())
        {
            mb.engine_activate();
            std::string portlist = mb.port_listing();
            std::cout << portlist;
        }
        else
            return EXIT_FAILURE;

        /*
         *      }
         *      else
         *          return EXIT_FAILURE;
         */
    }
    return EXIT_SUCCESS;
}

/*
 * midiprobeex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

