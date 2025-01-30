#if ! defined RTL66_RTAUDIO_HPP
#define RTL66_RTAUDIO_HPP

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
 *
 *  Realtime audio i/o C++ classes derived by refactoring RtAudio.
 *
 *  RtAudio provides a common API (Application Programming Interface)
 *  for realtime audio input/output across Linux (native ALSA, Jack,
 *  and OSS), Macintosh OS X (CoreAudio and Jack), and Windows
 *  (DirectSound, ASIO and WASAPI) operating systems.
 *
 *  RtAudio GitHub site: https://github.com/thestk/rtaudio
 *  RtAudio WWW site: http://www.music.mcgill.ca/~gary/rtaudio/
 *
 *  RtAudio: realtime audio i/o C++ classes
 *  Copyright (c) 2001-2022 Gary P. Scavone
 */

/**
 * \file          rtaudio.hpp
 *
 *      A reworking of RtAudio.hpp, with the same functionality but different
 *      coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-07
 * \updates       2025-01-20
 * \license       See above.
 *
 *      Also contains some additional capabilities.
 */

#include <string>                       /* omnipresent std::string class    */
#include <vector>

#include "c_macros.h"                   /* is_nullptr() macro               */
#include "rtl/rtl_build_macros.h"       /* RTL66_DLL_PUBLIC, etc.           */
#include "rtl/rterror.hpp"              /* rtl::rterror::callback_t         */
#include "rtl/audio/audio_support.hpp"  /* rtl::device_info                 */

namespace rtl
{

/**
 *  Instead of this...
 *
 *      typedef std::function<void(RtAudioErrorType type,
 *          const std::string &errorText )> RtAudioErrorCallback;
 *
 *  ... see rterror and the rterror::callback_t type.
 *
 *  Do we want to upgrade that to use std::functional???
 */

class audio_api;                         /* forward reference for pointer    */

/**
 *  rtaudio_in / rtaudio_out are "controllers" ...
 */

class RTL66_DLL_PUBLIC rtaudio
{

public:

    /**
     *  Audio API specifiers. These are more extensive than the MIDI API
     *  specifiers. Compare rtl::rtaudio::api to rtl::rtmidi::api.
     *
     *  Note that not all the items declared in this file are in the audio
     *  namespace.  For example, all of the "stream_" types.
     */

    enum class api
    {
        unspecified,    /**< Search for a working compiled API.             */
        pipewire,       /**< TODO! Will not be ready for quire awhile.      */
        jack,           /**< Linux/UNIX JACK Low-Latency Audio Server API.  */
        alsa,           /**< Advanced Linux Sound Architecture API.         */
        oss,            /**< Linux Open Sound System API.                   */
        pulseaudio,     /**< Linux PulseAudio API.                          */
        macosx_core,    /**< Macintosh OS-X Core Midi API.                  */
        windows_asio,   /**< The Steinberg Audio Stream I/O API.            */
        windows_ds,     /**< The Microsoft DirectSound API.                 */
        windows_wasapi, /**< The Microsoft WASAPI API.                      */
        dummy,          /**< A compilable but non-functional API.           */
        max             /**< A count of APIs; an erroneous value.           */
    };

    using api_list = std::vector<api>;

private:

    /**
     *  Provides the user's preferred API, if any.
     */

    static api sm_desired_api;

    /**
     *  Provides the API active for the duration of this run.  For example,
     *  under Linux we search for JACK, but if not found, fall back to
     *  ALSA.
     *
     *  To save repeated queries, we save this value.  Its default value is
     *  api::unspecified.
     *
     *  New to this "rtaudio"-derived library.
     */

    static api sm_selected_api;

private:

    /**
     *  Provides a pointer to the selected API implementation.
     */

    audio_api * m_rt_api_ptr;

public:

    rtaudio (rtaudio && other) noexcept;

protected:

    rtaudio ();

    /*
     * Make the class non-copyable
     */

    rtaudio (rtaudio & other) = delete;
    rtaudio & operator = (rtaudio & other) = delete;

public:

    /*
     * Must be public to use base-class pointer. See the usage of
     * std::unique_ptr<> in the test_helpers.cpp module.
     */

    virtual ~rtaudio ();

    /*
     *  Gets the active audio API if the API pointer is good.  Otherwise
     *  api::unspecified is returned.
     */

    api get_current_api () noexcept;
    unsigned get_device_count ();
    device_info get_device_info (unsigned deviceid);
    unsigned get_default_input_device ();
    unsigned get_default_output_device ();
    bool close_stream ();
    bool start_stream ();
    bool stop_stream ();
    bool abort_stream ();
    bool is_stream_open () const;
    bool is_stream_running () const;
    long get_stream_latency ();
    unsigned get_stream_sample_rate () const;
    void set_stream_time (double t);

public:

    /*
     * Static functions.
     */

public:

    /*
     * API client functions.
     */

    /*
     *
     * Note that some of these functions do not need to be virtual
     * because the polymorphism is implemented in the audio_api-based classes.
     * We will worry about that optimization at a LATER date.
     */

    bool open_stream
    (
        stream_parameters * outparameters,
        stream_parameters * inparameters,
        stream_format format,
        unsigned samplerate,
        unsigned bufferframes,
        callback_t cb,
        void * userdata,
        stream_options * options,
        rterror::callback_t errorcb
    );

public:

    static void silence_messages (bool silent);     /* already in rtmidi... */
    static void get_compiled_apis (api_list & apis) noexcept;
    static const api_list & detected_apis () noexcept;
    static bool is_detected_api (api rapi);
    static api fallback_api ();
    static std::string api_name (api rapi);
    static std::string api_display_name (api rapi);
    static std::string selected_api_name ();
    static std::string selected_api_display_name ();
    static api api_by_name (const std::string & name);
    static api api_by_index (int index);
    static int api_count ();

    static api & desired_api ()
    {
        return sm_desired_api;
    }

    static api & selected_api ()
    {
        return sm_selected_api;
    }

    static void desired_api (api rapi)
    {
        sm_desired_api = rapi;
    }

    static void selected_api (api rapi)
    {
        sm_selected_api = rapi;
    }

protected:

    audio_api * rt_api_ptr ()
    {
        return m_rt_api_ptr;
    }

    const audio_api * rt_api_ptr () const
    {
        return m_rt_api_ptr;
    }

    void rt_api_ptr (audio_api * p)
    {
        m_rt_api_ptr = p;
    }

    void delete_rt_api_ptr ();

    bool have_rt_api_ptr () const
    {
        return not_nullptr(m_rt_api_ptr);
    }

    bool no_rt_api_ptr () const
    {
        return is_nullptr(m_rt_api_ptr);
    }

protected:

    virtual bool open_audio_api
    (
        api rapi                        = api::unspecified,
        const std::string & clientname  = "",
        unsigned queuesize              = 0     /* useful with input ports  */
    ) = 0;

#if 0
    api ctor_common_setup
    (
        api rapi, const std::string & clientname
    );
#endif

};          // class rtaudio

/*------------------------------------------------------------------------
 * Free functions in the rtl namespace
 *------------------------------------------------------------------------*/

/**
 *  Provides a container of Audio API values.
 */

inline rtaudio::api
int_to_audioapi (int index)
{
    bool valid = index >= 0 && index < static_cast<int>(rtaudio::api::max);
    return valid ? static_cast<rtaudio::api>(index) : rtaudio::api::max ;
}

inline int
audioapi_to_int (rtaudio::api rapi)
{
    return static_cast<int>(rapi);
}

extern const std::string & rtl_audio_version () noexcept;
extern const std::string & get_rtaudio_version () noexcept;
extern const std::string & get_rtaudio_patch_version () noexcept;

/*------------------------------------------------------------------------
 * rtaudio API-detection function declarations.  Each is defined in the
 * appropriate audio_xxxxx module. Also included are some slightly useful
 * helper functions.
 *------------------------------------------------------------------------*/

#if defined RTL66_BUILD_PIPEWIRE
extern bool detect_pipewire ();
#endif

#if defined RTL66_BUILD_JACK

/*
 * These declarations duplicate those in audio_jack.hpp!
 */

extern bool detect_jack (bool forcecheck = false);
extern void silence_jack_errors (bool silent);
extern void silence_jack_info (bool silent);
extern void silence_jack_messages (bool silent);
#endif

#if defined RTL66_BUILD_ALSA
extern bool detect_alsa (bool checkports);
#endif

#if defined RTL66_BUILD_OSS
extern bool detect_oss (bool checkports);
#endif

#if defined RTL66_BUILD_PULSEAUDIO
extern bool detect_pulseaudio (bool checkports);
#endif

#if defined RTL66_BUILD_MACOSX_CORE
extern bool detect_core ();
#endif

#if defined RTL66_BUILD_WIN_ASIO
extern bool detect_win_asio ();
#endif

#if defined RTL66_BUILD_WIN_DS
extern bool detect_win_ds ();
#endif

#if defined RTL66_BUILD_WIN_WASAPI
extern bool detect_win_wasapi ();
#endif

#if defined RTL66_BUILD_DUMMY
extern bool detect_dummy ();
#endif

}           // namespace rtl

#endif      // RTL66_RTAUDIO_HPP

/*
 * rtaudio.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

