#if ! defined RTL66_AUDIO_ALSA_HPP
#define RTL66_AUDIO_ALSA_HPP

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
 * \file          audio_alsa.hpp
 *
 *      Provides the rtl66 ALSA implmentation for AUDIO input and output.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-17
 * \updates       2023-07-19
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_ALSA

#include <string>                       /* std::string class                */

#include "rtl/audio/audio_api.hpp"      /* rtl::audio_in/out_api classes    */

namespace audio
{
    class event;
}

namespace rtl
{

#if defined USE_AUDIO_ALSA_DETECTION

/*
 *  Used for verifying the usability of the API.  If true, the checkports
 *  parameter requires that more than 0 ports must be found.
 */

extern bool detect_alsa (bool checkports);
extern void set_alsa_version ();

#endif

/*------------------------------------------------------------------------
 * audio_alsa
 *------------------------------------------------------------------------*/

/**
 *  This class is not a base class, but a mixin class for use with
 *  audio_alsa_in and audio_alsa_out.  We might eventually refactor it into
 *  a base class, but for now it exists so that the JACK I/O classes
 *  can share some implementation details.
 */

class RTL66_DLL_PUBLIC audio_alsa : public audio_api
{
    friend void * audio_alsa_handler (void *);

private:

    /**
     *  Moved the client name to this class.
     */

    std::string m_client_name;

    /**
     *  Moved the ALSA data to this class.
     */

    audio_alsa_data m_alsa_data;

public:

    audio_alsa ();
    audio_alsa
    (
        ::audio::port::io iotype,
        const std::string & clientname  = "",
        unsigned queuesize              = 0
    );
    audio_alsa (const audio_alsa &) = delete;
    audio_alsa & operator = (const audio_alsa &) = delete;
    virtual ~audio_alsa ();

    virtual rtaudio::api get_current_api () override
    {
        return rtaudio::api::alsa;
    }

    const std::string & client_name () const
    {
        return m_client_name;
    }

    audio_alsa_data & alsa_data ()
    {
        return m_alsa_data;
    }

    void client_name (const std::string & cname)
    {
        m_client_name = cname;
    }

protected:

    snd_seq_t * client_handle (void * c)
    {
        return reinterpret_cast<snd_seq_t *>(c);
    }

    snd_seq_t * client_handle ()
    {
        return have_master_bus() ?
            reinterpret_cast<snd_seq_t *>(master_bus()->client_handle()) :
            alsa_data().alsa_client() ;
    }

    virtual void * void_handle () override
    {
        return reinterpret_cast<void *>(client_handle());
    }

    virtual void * engine_connect () override;
    virtual void engine_disconnect () override;

private:

protected:

};          // class audio_alsa

}           // namespace rtl

#endif      // defined RTL66_BUILD_ALSA

#endif      // RTL66_AUDIO_ALSA_HPP

/*
 * audio_alsa.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

