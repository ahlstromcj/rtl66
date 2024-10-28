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
 * \file          rtaudio.cpp
 *
 *    A reworking of RtAudio.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-07
 * \updates       2024-01-30
 * \license       See above.
 *
 */

#include <stdexcept>                    /* std::invalid_argument, etc.      */

#include "platform_macros.h"            /* operating system detection       */
#include "c_macros.h"                   /* not_nullptr and other macros     */
#include "rtl/audio/audio_api.hpp"      /* rtl::audio_api class             */
#include "rtl/audio/rtaudio.hpp"        /* rtl::rtaudio class, etc.         */

namespace rtl
{

/*---------------------------------------------------------------------------
 *  Library version information
 *--------------------------------------------------------------------------*/

/*
 * Version information string.
 */

const std::string &
rtl_audio_version () noexcept
{
    static std::string s_info = RTL66_NAME "-" RTL66_VERSION " " __DATE__ ;
    return s_info;
}

/**
 *   A free function to determine the current rtl66 version.
 */

const std::string &
get_rtaudio_version () noexcept
{
    static const std::string s_version = RTL66_RTAUDIO_VERSION;
    return s_version;
}

/**
 *   A free function to determine the RtAudio version used as the basis
 *   of this implementation. Updated to match the latest when patching is done.
 *   This function is probably more useful.
 */

const std::string &
get_rtaudio_patch_version () noexcept
{
    static const std::string s_version = RTL66_RTAUDIO_PATCHED;
    return s_version;
}

/*------------------------------------------------------------------------
 * rtaudio static members
 *------------------------------------------------------------------------*/

rtaudio::api rtaudio::sm_desired_api  = rtaudio::api::unspecified; /* fallback */
rtaudio::api rtaudio::sm_selected_api = rtaudio::api::unspecified; /* selected */

/*------------------------------------------------------------------------
 * rtaudio
 *------------------------------------------------------------------------*/

RTL66_DLL_PUBLIC
rtaudio::rtaudio () :
    m_rt_api_ptr  (nullptr)                         /* rt_api_ptr() access  */
{
    // No code
}

rtaudio::rtaudio (rtaudio && other) noexcept :
    m_rt_api_ptr (other.m_rt_api_ptr)
{
    other.m_rt_api_ptr = nullptr;
}

rtaudio::~rtaudio()
{
    delete_rt_api_ptr();
}

void
rtaudio::delete_rt_api_ptr ()
{
    if (not_nullptr(m_rt_api_ptr))
    {
        delete m_rt_api_ptr;
        m_rt_api_ptr = nullptr;
    }
}

/**
 *  Define API names and display names.  Must be in same order as the
 *  rtl::audio::api enumercation class in rtaudio_types.hpp.  They must also be
 *  C-linkable to be used in the rtaudio_c module.
 */

static const std::string cs_api_names[][2] =
{
    /*
     *   name            display name
     */

    { "unspecified",    "Unknown"               },
    { "pipewire",       "PipeWire"              },  /* TODO! */
    { "jack",           "JACK"                  },
    { "alsa",           "ALSA"                  },
    { "oss",            "Open Sound System"     },  /* TODO! */
    { "pulseaudio",     "PulseAudio"            },  /* TODO! */
    { "macosx_core",    "CoreAudio"             },
    { "windows_asio",   "Windows ASIO"          },
    { "windows_ds",     "Windows DirectSound"   },
    { "windows_wasapi", "Windows Audio Session" },
    { "dummy",          "Dummy"                 }
};

/*
 * The order here will control the order of rtaudio's API search in the
 * constructor. Note that audio::api::unspecified terminates the list and is not
 * included in the API count.
 */

static const rtaudio::api cs_compiled_apis [] =
{
    rtaudio::api::unspecified,
#if defined RTL66_BUILD_PIPEWIRE
    rtaudio::api::pipewire,
#endif
#if defined RTL66_BUILD_JACK
    rtaudio::api::jack,
#endif
#if defined RTL66_BUILD_ALSA
    rtaudio::api::alsa,
#endif
#if defined RTL66_BUILD_OSS
    rtaudio::api::oss,
#endif
#if defined RTL66_BUILD_PULSEAUDIO
    rtaudio::api::pulseaudio,
#endif
#if defined RTL66_BUILD_MACOSX_CORE
    rtaudio::api::macosx_core,
#endif
#if defined RTL66_BUILD_WIN_ASIO
    rtaudio::api::windows_asio,
#endif
#if defined RTL66_BUILD_WIN_DS
    rtaudio::api::windows_ds,
#endif
#if defined RTL66_BUILD_WIN_WASAPI
    rtaudio::api::windows_wasapi,
#endif
#if defined RTL66_BUILD_DUMMY
    rtaudio::api::dummy
#endif
};

static const int cs_compiled_apis_count =
    sizeof(cs_compiled_apis) / sizeof(cs_compiled_apis[0]) - 1;

/**
 *  A static function to determine the available compiled Audio APIs.  The
 *  values returned in the std::vector can be compared against the enumerated
 *  list values.  Note that there can be more than one API compiled for
 *  certain operating systems.
 */

void
rtaudio::get_compiled_apis (rtaudio::api_list & apis) noexcept
{
    apis = rtaudio::api_list
    (
        cs_compiled_apis,
        cs_compiled_apis + cs_compiled_apis_count
    );
}

/**
 *  TODO:  We already have detect_jack() and detect_alsa() in the
 *  MIDI area; we ought to provide an area for code common to both MIDI
 *  and AUDIO.
 */

const rtaudio::api_list &
rtaudio::detected_apis () noexcept
{
    static bool s_uninitialized = true;
    static rtaudio::api_list s_api_list;
    if (s_uninitialized)
    {
#if defined RTL66_BUILD_PIPEWIRE
        if (detect_pipewire())
            s_api_list.push_back(api::pipewire);
#endif
#if defined RTL66_BUILD_JACK_TODO               // TEMPORARILY DISABLED
        if (detect_jack(true, false))           /* check ports, no recheck  */
            s_api_list.push_back(api::jack);
#endif
#if defined RTL66_BUILD_ALSA_TODO               // TEMPORARILY DISABLED
        if (detect_alsa(true))
            s_api_list.push_back(api::alsa);
#endif
#if defined RTL66_BUILD_OSS
        if (detect_oss(true))
            s_api_list.push_back(api::oss);
#endif
#if defined RTL66_BUILD_PULSEAUDIO
        if (detect_pulseaudio(true))
            s_api_list.push_back(api::pulseaudio);
#endif
#if defined RTL66_BUILD_MACOSX_CORE
        if (detect_core())
            s_api_list.push_back(api::macosx_core);
#endif
#if defined RTL66_BUILD_WIN_ASIO
        if (detect_win_asio())
            s_api_list.push_back(api::windows_asio);
#endif
#if defined RTL66_BUILD_WIN_DS
        if (detect_win_ds())
            s_api_list.push_back(api::windows_ds);
#endif
#if defined RTL66_BUILD_WIN_WASAPI
        if (detect_win_wasapi())
            s_api_list.push_back(api::windows_wasapi);
#endif
#if defined RTL66_BUILD_DUMMY
        if (detect_dummy())
            s_api_list.push_back(api::dummy);
#endif
    }
    return s_api_list;
}

/**
 *  Determines if the selected api is actually present on the system.
 */

bool
rtaudio::is_detected_api (rtaudio::api rapi)
{
    bool result = false;
    const rtaudio::api_list & apilist = rtaudio::detected_apis();
    for (rtaudio::api a : apilist)
    {
        if (a == rapi)
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Determines the Audio API to use when no API is specified.  Currently, only
 *  the Linux APIs need this, but the rest return their platform's API, if
 *  compiled in and detected.
 *
 * \return
 *      Returns the first valid, operational Audio API that is found.
 *      If there is none, rtl::audio::api::max (or dummy???) is returned.
 */

rtaudio::api
rtaudio::fallback_api ()                 /* static   */
{
    rtaudio::api result = rtaudio::api::max;
    const rtaudio::api_list & apilist = rtaudio::detected_apis();
    if (! apilist.empty())
        result = apilist[0];

    return result;
}

/**
 *  Return the name of a specified compiled Audio API.
 *
 *  This obtains a short lower-case name used for identification purposes.
 *  This value is guaranteed to remain identical across library versions.
 *  If the API is unknown, this function will return the empty string.
 */

std::string
rtaudio::api_name (rtaudio::api rapi)           /* static */
{
    std::string result;
    if (rapi > rtaudio::api::unspecified && rapi < rtaudio::api::max)
        result = cs_api_names[audioapi_to_int(rapi)][0];

    return result;
}

std::string
rtaudio::selected_api_name ()               /* static */
{
    return api_name(selected_api());
}

/**
 *  Return the display name of a specified compiled Audio API.
 *
 *  This obtains a long name used for display purposes.  If the API is
 *  unknown, this function will return the empty string.
 */

std::string
rtaudio::api_display_name (rtaudio::api rapi)
{
    std::string result;
    if (rapi > rtaudio::api::unspecified && rapi < rtaudio::api::max)
        result = cs_api_names[audioapi_to_int(rapi)][1];
    else
        result = std::string("Unknown");

    return result;
}

std::string
rtaudio::selected_api_display_name ()            /* static */
{
    return api_display_name(selected_api());
}

/**
 *  Return the compiled Audio API having the given name.
 *
 *  A case insensitive comparison will check the specified name against the
 *  list of compiled APIs, and return the one which matches. On failure, the
 *  function returns audio::api::unspecified.
 */

rtaudio::api
rtaudio::api_by_name (const std::string & name)
{
    for (int i = 0; i < cs_compiled_apis_count; ++i)
    {
        if (name == cs_api_names[audioapi_to_int(cs_compiled_apis[i])][0])
          return cs_compiled_apis[i];
    }
    return api::unspecified;
}

bool
rtaudio::open_stream
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
)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
    {
        result = rt_api_ptr()->open_stream
        (
            outparameters,
            inparameters,
            format,
            samplerate,
            bufferframes,
            cb,
            userdata,
            options,
            errorcb
        );
    }
    return result;
}

rtaudio::api
rtaudio::get_current_api () noexcept
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_current_api() : api::unspecified ;
}

unsigned
rtaudio::get_device_count () // noexcept
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_device_count() : 0 ;
}

device_info
rtaudio::get_device_info (unsigned deviceid) // noexcept
{
    static device_info s_dummy;
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_device_info(deviceid) : s_dummy ;
}

unsigned
rtaudio::get_default_input_device ()
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_default_input_device() :
        device_info::invalid_id ;
}

unsigned
rtaudio::get_default_output_device ()
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_default_output_device() :
        device_info::invalid_id ;
}

/**
 *  A function that closes a stream and frees any associated stream memory.
 *  If a stream is not open, this function issues a warning and
 *  returns (no exception is thrown).
 */

bool
rtaudio::close_stream () // noexcept;
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->close_stream() : false ;
}

bool
rtaudio::start_stream () // noexcept;
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->start_stream() : false ;
}

bool
rtaudio::stop_stream () // noexcept;
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->stop_stream() : false ;
}

bool
rtaudio::abort_stream () // noexcept;
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->abort_stream() : false ;
}

bool
rtaudio::is_stream_open () const
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->is_stream_open() : false ;
}

bool
rtaudio::is_stream_running () const
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->is_stream_running() : false ;
}

long
rtaudio::get_stream_latency ()
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_stream_latency () : 0 ;
}

unsigned
rtaudio::get_stream_sample_rate () const
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_stream_sample_rate () : 0 ;
}

void
rtaudio::set_stream_time (double t)
{
    if (not_nullptr(rt_api_ptr()))
        rt_api_ptr()->set_stream_time(t);

}

/*------------------------------------------------------------------------
 * rtaudio virtual base-class functions
 *------------------------------------------------------------------------*/

#if 0

/**
 *  Simply provides some common code for the rtaudio-derived constructors.
 */

audio::api
rtaudio::ctor_common_setup (audio::api rapi, const std::string & clientname)
{
#if 0
    std::string cname = clientname;
    if (cname.empty())
        cname = audio::global_client_info().client_name();
    else
       audio::global_client_info().client_name(cname);      // IFFY
#else
    (void) clientname;
#endif

    if (rapi == audio::api::unspecified)
        rapi = fallback_api();

    return rapi;
}

#endif

}           // namespace rtl

/*
 * rtaudio.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

