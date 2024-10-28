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
 * \file          audio_alsa.cpp
 *
 *    Implements the ALSA AUDIO API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2023-03-17
 * \updates       2023-03-17
 * \license       See above.
 *
 * To do:
 *
 *      Replace malloc() with new/delete or a wrapper class.
 */

#include "rtl/audio/alsa/audio_alsa.hpp"  /* rtl::audio_alsa class             */

#if defined RTL66_BUILD_ALSA

#include <sstream>                      /* std::ostringstream class         */

#if defined PLATFORM_DEBUG
#include <iostream>                     /* std::cerr, cout classes          */
#endif

#include "rtl66-config.h"               /* RTL66_HAVE_XXX                   */

namespace rtl
{

/*------------------------------------------------------------------------
 * ALSA free functions
 *------------------------------------------------------------------------*/

#if defined USE_AUDIO_ALSA_DETECTION

/**
 *  ALSA detection function.  Just opens a client without activating it, then
 *  closes it.  Also note that we must use the name "default", not a name of
 *  our choosing.
 */

bool
detect_alsa (bool checkports)
{
    bool result = false;
    snd_seq_t * alsaman;
    int rc = ::snd_seq_open
    (
        &alsaman, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK
    );
    if (rc == 0)
    {
        if (checkports)
        {
            // TODO
        }
        result = true;
        rc = ::snd_seq_close(alsaman);
        if (rc < 0)
        {
            errprint("error closing ALSA detection");
        }
    }
    else
    {
        errprint("ALSA not detected");
    }
    return result;
}

/**
 * Since this is build info, we do not use the run-time version of ALSA
 * that can be obtained from snd_asoundlib_version().
 *
 * This might be goofy!
 */

void
set_alsa_version ()
{
#if defined SND_LIB_VERSION_STR
    std::string jv{SND_LIB_VERSION_STR};
    ::audio::global_client_info().api_version(jv);
#endif
}

#endif  // defined USE_AUDIO_ALSA_DETECTION

/*------------------------------------------------------------------------
 * ALSA callbacks
 *------------------------------------------------------------------------*/

/**
 */

void *
audio_alsa_handler (void * ptr)
{
    rtaudio_in_data * rtidata = audio_api::static_in_data_cast(ptr);
    audio_alsa_data * apidata = audio_alsa::static_data_cast(rtidata->api_data());


    return 0;
}

/*------------------------------------------------------------------------
 * audio_alsa
 *------------------------------------------------------------------------*/

audio_alsa::audio_alsa () :
    audio_api        (),
    m_client_name   (),
    m_alsa_data     ()
{
    (void) initialize(client_name());
}

audio_alsa::audio_alsa
(
    ::audio::port::io iotype,
    const std::string & clientname,
    unsigned queuesize
) :
    audio_api        (iotype, queuesize),
    m_client_name   (clientname),
    m_alsa_data     ()
{
    (void) initialize(client_name());
}

audio_alsa::~audio_alsa ()
{
    delete_port();
}

/*------------------------------------------------------------------------
 * audio_alsa engine-related functions
 *------------------------------------------------------------------------*/

/**
 *  This function opens an ALSA-client connection.  It does this:
 *
 *  -   Opens the ALSA client:
 *      -   Input: opened in duplex (input/output), non-blocking mode.
 *      -   Output: opened in output, non-blocking mode.
 *      , and saves
 *      the snd_seq_t client handle in the audio_alsa_data structure held
 *      by this class instance.
 *  -   The desired client name is retrieved and set.
 */

void *
audio_alsa::engine_connect ()
{
    void * result = nullptr;
    snd_seq_t * seq;
    int streams = is_output() ? SND_SEQ_OPEN_OUTPUT : SND_SEQ_OPEN_DUPLEX;
    int mode = SND_SEQ_NONBLOCK;
    int rc = ::snd_seq_open(&seq, "default", streams, mode);
    if (rc == 0)
    {
        bool ok = set_seq_client_name(seq, client_name());
        if (ok)
            result = reinterpret_cast<void *>(seq);
        else
            (void) ::snd_seq_close(seq);
    }
    return result;
}

void
audio_alsa::engine_disconnect ()
{
    audio_alsa_data & data = alsa_data();
    snd_seq_t * c = data.alsa_client();
    if (not_nullptr(c))
    {
        int rc = ::snd_seq_close(c);
        data.alsa_client(nullptr);
        if (rc != 0)
        {
            error_print("snd_seq_close", "failed");
        }
    }
}

}               // namespace rtl

#endif          // defined RTL66_BUILD_ALSA

/*
 * audio_alsa.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

