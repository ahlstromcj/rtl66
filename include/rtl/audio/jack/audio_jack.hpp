#if ! defined RTL66_AUDIO_JACK_HPP
#define RTL66_AUDIO_JACK_HPP

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
 * \file          audio_jack.hpp
 *
 *      Provides the rtl66 JACK implmentation for audio input and output.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-17
 * \updates       2025-01-20
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include <string>                       /* std::string class                */
#include <jack/jack.h>                  /* JACK API functions, etc.         */

#include "rtl/audio/audio_api.hpp"        /* rtl::audio_in/out_api classes     */

/*
 * Note that here we support two namespaces: audio and rtl::audio.
 */

namespace rtl
{

/*------------------------------------------------------------------------
 * Additional JACK support functions
 *------------------------------------------------------------------------*/

/*
 *  Used for verifying the usability of the API.  If true, the checkports
 *  parameter requires that more than 0 ports must be found.
 */

#if defined JACK_AUDIO_READY
extern bool detect_jack (bool forcecheck = false);
extern void set_jack_version ();
extern void silence_jack_errors (bool silent);
extern void silence_jack_info (bool silent);
extern void silence_jack_messages (bool silent);
#endif

/*------------------------------------------------------------------------
 * JACK callbacks are now defined in the audio_jack_callbacks.cpp module.
 *------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 * audio_jack
 *------------------------------------------------------------------------*/

/**
 *  This class is not a base class, but a mixin class for use with
 *  audio_jack_in and audio_jack_out.  We might eventually refactor it into
 *  a base class, but for now it exists so that the JACK I/O classes
 *  can share some implementation details.
 */

class RTL66_DLL_PUBLIC audio_jack : public audio_api
{

#if defined JACK_AUDIO_READY
    friend int jack_process_in (jack_nframes_t, void *);
    friend int jack_process_out (jack_nframes_t, void *);
    friend int jack_process_io (jack_nframes_t, void *);
#if defined RTL66_JACK_PORT_REFRESH_CALLBACK
    friend void jack_port_register_callback (jack_port_id_t, int, void *);
#endif
#if defined RTL66_JACK_PORT_CONNECT_CALLBACK
    friend void jack_port_connect_callback
    (
        jack_port_id_t, jack_port_id_t, int, void *
    );
#endif
#if defined RTL66_JACK_PORT_SHUTDOWN_CALLBACK
    friend void jack_shutdown_callback (void *);
#endif
#endif

private:

    /**
     *  Moved the client name to this class.
     */

    std::string m_client_name;

    /**
     *  Moved the JACK data to this class, and now it doesn't have to
     *  be allocated, just have it's pointer assigned.

    audio_jack_data m_jack_data;
     */

public:

    audio_jack ();
    audio_jack
    (
        const std::string & clientname  = "",       // default to RtApiJack?
        unsigned queuesize              = 0
    );
    audio_jack (const audio_jack &) = delete;
    audio_jack & operator = (const audio_jack &) = delete;
    virtual ~audio_jack ();

    virtual rtaudio::api get_current_api () override
    {
        return rtaudio::api::jack;
    }

    const std::string & client_name () const
    {
        return m_client_name;
    }

    audio_jack_data & jack_data ()
    {
        return m_jack_data;
    }

    void client_name (const std::string & cname)
    {
        m_client_name = cname;
    }

    // MIDI stuff useful here?
    // Code shared between audio and MIDI should be moved to a common module.

    void * engine_connect();

protected:

    jack_client_t * client_handle (void * c)
    {
        return reinterpret_cast<jack_client_t *>(c);
    }

    jack_client_t * client_handle ()
    {
        return have_master_bus() ?
            reinterpret_cast<jack_client_t *>(master_bus()->client_handle()) :
            jack_data().jack_client() ;
    }

    virtual void * void_handle ()
    {
        return reinterpret_cast<void *>(client_handle());
    }


};          // class audio_jack

}           // namespace rtl

#endif      // defined RTL66_BUILD_JACK

#endif      // RTL66_AUDIO_JACK_HPP

/*
 * audio_jack.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

