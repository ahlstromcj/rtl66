#if ! defined RTL66_RTL_MIDI_JACK_CALLBACKS_HPP
#define RTL66_RTL_MIDI_JACK_CALLBACKS_HPP

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
 * \file          midi_jack_callbacks.hpp
 *
 *      Provides the rtl66 JACK MIDI/Audio callbacks and easier setup functions
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-12-11
 * \updates       2023-03-15
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include <jack/jack.h>                  /* JACK API functions, etc.         */

namespace rtl
{

/*------------------------------------------------------------------------
 * JACK callback setup functions (used in midi_jack)
 *------------------------------------------------------------------------*/

extern bool jack_set_process_cb
(
    jack_client_t * c,
    JackProcessCallback cb,
    void * apidata                  /* JACK API data structure  */
);
extern bool jack_set_shutdown_cb
(
    jack_client_t * c,
    JackShutdownCallback cb,
    void * self                     /* i.e. the "this" pointer  */
);
extern bool jack_set_port_connect_cb
(
    jack_client_t * c,
    JackPortConnectCallback cb,
    void * self                     /* i.e. the "this" pointer  */
);
extern bool jack_set_port_registration_cb
(
    jack_client_t * c,
    JackPortRegistrationCallback cb,
    void * self                     /* i.e. the "this" pointer  */
);

/*------------------------------------------------------------------------
 * JACK metadata setup
 *------------------------------------------------------------------------*/

extern bool jack_set_meta_data
(
    jack_client_t * c,
    const std::string & app_icon_name
);

/*------------------------------------------------------------------------
 * JACK callbacks
 *------------------------------------------------------------------------*/

extern int jack_process_in (jack_nframes_t nframes, void * arg);
extern int jack_process_out (jack_nframes_t nframes, void * arg);
extern int jack_process_io (jack_nframes_t nframes, void * arg);

#if defined RTL66_JACK_PORT_REFRESH_CALLBACK
extern void jack_port_register_callback
(
    jack_port_id_t portid, int regv, void * arg
);
#endif

#if defined RTL66_JACK_PORT_CONNECT_CALLBACK
extern void jack_port_connect_callback
(
    jack_port_id_t a, jack_port_id_t b, int connect, void * arg
);
#endif

#if defined RTL66_JACK_PORT_SHUTDOWN_CALLBACK
extern void jack_shutdown_callback (void * arg);
#endif

/*------------------------------------------------------------------------
 * midi_jack
 *------------------------------------------------------------------------*/

}           // namespace rtl

#endif      // defined RTL66_BUILD_JACK

#endif      // RTL66_RTL_MIDI_JACK_CALLBACKS_HPP

/*
 * midi_jack_callbacks.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

