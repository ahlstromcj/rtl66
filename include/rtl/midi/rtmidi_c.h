#if ! defined RTL66_RTMIDI_C_H
#define RTL66_RTMIDI_C_H

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
 * \file          rtmidi_c.h
 *
 *    A reworking of rtmidi_c.h, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-01-31
 * \license       See above.
 *
 *  C interface to realtime MIDI input/output C++ classes.  rtmidi offers a
 *  C-style interface, principally for use in binding rtmidi to other
 *  programming languages.  All structs, enums, and functions listed here
 *  have direct analogs (and simply call to) items in the C++ rtmidi class
 *  and its supporting classes and types
 */

#include "c_macros.h"                   /* EXTERN_C_DEC                     */

#if ! defined __cplusplus

/*
 *  <cstdbool> is deprecated in C++17 and removed in C++20. Corresponding
 *  <stdbool.h> is still available in C++20.
 */

#include <stdbool.h>                    /* macros related to booleans       */
#include <stddef.h>                     /* NULL and other macros            */

#endif

#include "rtl/rtl_build_macros.h"       /* RTL66_API, etc.                  */
#include "midi/midibytes.hpp"           /* midi::byte alias, etc.           */

EXTERN_C_DEC                            /* the whole module is extern "C"   */

/*
 *  Types related to the correspong midibytes types in the rtl namespace.
 */

typedef unsigned char cmidibyte;
typedef cmidibyte * cmidibytes;
typedef const cmidibyte * const_midibytes;

/**
 *  Wraps an rtmidi object for C function return statuses.  We keep the original
 *  name for usage with C code and old clients.
 *
 *  Also note that rterror is a light rework of the original RtMidiError class;
 *  both are based on std::exception.  Furthermore, rtmidi is a rework of the
 *  RtMidi base class.
 */

struct RtMidiWrapper
{
    void * ptr;             // The wrapped rtmidi object.
    void * data;
    bool ok;                // True when the last function call was OK.
    const char * msg;       // If an error, set to an error message.
};

/**
 *  Old-style aliases for this C code.
 */

typedef struct RtMidiWrapper * RtMidiPtr;    /* points to I/O               */
typedef struct RtMidiWrapper * RtMidiInPtr;  /* points to input             */
typedef struct RtMidiWrapper * RtMidiOutPtr; /* points to output            */
typedef void * MidiApi;                      /* points to rtl::rtmidi_api   */
typedef void (* RtMidiCCallback)             /* rt_midi_in_data::callback_t */
(
    double timestammp,
    const const_midibytes message,
    size_t messagesize,
    void * userdata
);

/**
 *  MIDI API specifier arguments.  See rtl::rtmidi::api enumeration in the
 *  rtmidi.hpp module. These must line up, but we're not going to check
 *  for it at build time. Sorry folks.
 */

typedef enum
{
    RTMIDI_API_UNSPECIFIED,         /* rtl::rtmidi::api::unspecified        */
    RTMIDI_API_PIPEWIRE,            /* rtl::rtmidi::api::pipewire TODO@     */
    RTMIDI_API_UNIX_JACK,           /* rtl::rtmidi::api::jack               */
    RTMIDI_API_LINUX_ALSA,          /* rtl:rt:midi::api::alsa               */
    RTMIDI_API_MACOSX_CORE,         /* rtl:rt:midi::api::macosx_core        */
    RTMIDI_API_WINDOWS_MM,          /* rtl::rtmidi::api::windows_mm         */
    RTMIDI_API_WINDOWS_UWP,         /* rtl::rtmidi::api::windows_uwp        */
    RTMIDI_API_ANDROID_MIDI,        /* rtl::rtmidi::api::android_midi       */
    RTMIDI_API_WEB_MIDI,            /* rtl::rtmidi::api::web_midi           */
    RTMIDI_API_RTMIDI_DUMMY,        /* rtl::rtmidi::api::dummy              */
    RTMIDI_API_MAX                  /* rtl::rtmidi::api::max, terminator    */

} RtMidiApi;

/**
 *  Defined RtMidiError types. See rterror::kind.
 */

typedef enum
{
    RTMIDI_ERROR_WARNING,           /* rtl::rterror::kind::warning          */
    RTMIDI_ERROR_DEBUG_WARNING,     /* rtl::rterror::kind::debug_warning    */
    RTMIDI_ERROR_UNSPECIFIED,       /* rtl::rterror::kind::unspecified      */
    RTMIDI_ERROR_NO_DEVICES_FOUND,  /* rtl::rterror::kind::no_devices_found */
    RTMIDI_ERROR_INVALID_DEVICE,    /* rtl::rterror::kind::invalid_device   */
    RTMIDI_ERROR_MEMORY_ERROR,      /* rtl::rterror::kind::memory_error     */
    RTMIDI_ERROR_INVALID_PARAMETER, /* rtl::rterror::kind::invalid_parameter*/
    RTMIDI_ERROR_INVALID_USE,       /* rtl::rterror::kind::invalid_use      */
    RTMIDI_ERROR_DRIVER_ERROR,      /* rtl::rterror::kind::driver_error     */
    RTMIDI_ERROR_SYSTEM_ERROR,      /* rtl::rterror::kind::system_error     */
    RTMIDI_ERROR_THREAD_ERROR,      /* rtl::rterror::kind::thread_error     */
    RTMIDI_ERROR_MAX                /* rtl::rterror::kind::max              */

} RtMidiErrorType;

/**
 *  The rtmidi "C" API. Determine the available compiled MIDI APIs.  If the
 *  given `apis` parameter is null, returns the number of available APIs.
 *  Otherwise, fill the given apis array with the rtl::rtmidi::api values.
 *
 * \param apis
 *      An array or a null value.
 *
 * \param apis_size
 *      Number of elements pointed to by apis.
 *
 * \return
 *      Number of items needed for apis array if apis == NULL, or number of
 *      items written to apis array otherwise.  A negative return value
 *      indicates an error.
 */

/*
 *  static void get_compiled_apis (api_list & apis) noexcept;
 *  static void get_detected_apis (api_list & apis) noexcept;
 *  static std::string api_name (rtmidi::api rapi);
 *  static std::string api_display_name (rtmidi::api rapi);
 *  static rtmidi::api api_by_name (const std::string & name);
 *  static rtmidi::api api_by_index (int index);
 *  static void show_apis (const std::string & tag, const api_list & apis);
 */

RTL66_API int rtmidi_get_compiled_apis (RtMidiApi * apis, int apissize);
RTL66_API int rtmidi_get_detected_apis (RtMidiApi * apis, int apissize);
RTL66_API const char * rtmidi_api_name (RtMidiApi rapi);
RTL66_API const char * rtmidi_api_display_name (RtMidiApi rapi);
RTL66_API RtMidiApi rtmidi_api_by_name (const char * name);
RTL66_API RtMidiApi rtmidi_api_by_index (int index);

/*
 *  static void start_jack (bool flag);
 *
 *  static void silence_messages (bool silent);
 *  static void desired_api (rtmidi::api rapi)
 *  static void selected_api (rtmidi::api rapi)
 */

#if defined RTL66_BUILD_JACK
RTL66_API void rtmidi_start_jack (bool startit);
#endif

RTL66_API void rtmidi_silence_messages (bool silent);
RTL66_API bool rtmidi_set_desired_api (RtMidiApi rapi);
RTL66_API bool rtmidi_set_select_api (RtMidiApi rapi);

/*
 *  Might need to clarify/investigate these.
 */

RTL66_API void rtmidi_use_virtual_ports (bool flag);
RTL66_API void rtmidi_use_auto_connect (bool flag);
RTL66_API void rtmidi_global_ppqn (int ppq);
RTL66_API void rtmidi_global_bpm (double b);

RTL66_API const char * rtmidi_api_display_name (RtMidiApi rapi);
RTL66_API void rtmidi_error
(
    MidiApi * mapi,
    RtMidiErrorType type,
    const char * errorstring
);
RTL66_API void rtmidi_open_port
(
    RtMidiPtr device,
    int portnumber,
    const char * portname
);
RTL66_API void rtmidi_open_virtual_port
(
    RtMidiPtr device,
    const char * portname
);
RTL66_API void rtmidi_close_port (RtMidiPtr device);
RTL66_API unsigned rtmidi_get_port_count (RtMidiPtr device);
RTL66_API int rtmidi_get_port_name
(
    RtMidiPtr device,
    int portnumber,
    char * bufout,
    int * buflen
);

/*
RTL66_API bool rtmidi_simple_cli
(
    const char * appname,
    int argc, char * argv []
);
*/

/**
 *  rtmidi_in API
 */

RTL66_API RtMidiInPtr rtmidi_in_create_default (void);
RTL66_API RtMidiInPtr rtmidi_in_create
(
    RtMidiApi api,
    const char * clientname,
    unsigned queuesizelimit
);
RTL66_API void rtmidi_in_free (RtMidiInPtr device);
RTL66_API RtMidiApi rtmidi_in_get_current_api (RtMidiPtr device);
RTL66_API void rtmidi_in_set_callback
(
    RtMidiInPtr device,
    RtMidiCCallback cb,                 /* rtl::rtmidi_in_data::callback_t  */
    void * userDatai
);
RTL66_API void rtmidi_in_cancel_callback (RtMidiInPtr device);
RTL66_API void rtmidi_in_ignore_types
(
    RtMidiInPtr device,
    bool midisysex,
    bool miditime,
    bool midisense
);
RTL66_API double rtmidi_in_get_message
(
    RtMidiInPtr device,
    cmidibytes * message,
    size_t * sz
);

/**
 *  rtmidi_out API
 */

RTL66_API RtMidiOutPtr rtmidi_out_create_default (void);
RTL66_API RtMidiOutPtr rtmidi_out_create
(
    RtMidiApi api,
    const char * clientname
);
RTL66_API void rtmidi_out_free (RtMidiOutPtr device);
RTL66_API RtMidiApi rtmidi_out_get_current_api (RtMidiPtr device);
RTL66_API int rtmidi_out_send_message
(
    RtMidiOutPtr device,
    const_midibytes message,
    int len
);
RTL66_API bool rtmidi_simple_cli
(
    const char * appname,
    int argc, char * argv []
);

EXTERN_C_END

#endif          // RTL66_RTMIDI_C_H

/*
 * rtmidi_c.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
