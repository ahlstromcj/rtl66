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
 * \file          rtmidi.cpp
 *
 *    A reworking of RtMidi.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-06-09
 * \license       See above.
 *
 *  A member function correlation and check-list can be found in
 *  extras/notes/member-mappings.text.
 */

#include <stdexcept>                    /* std::invalid_argument, etc.      */

#include "platform_macros.h"            /* operating system detection       */
#include "c_macros.h"                   /* not_nullptr and other macros     */
#include "midi/event.hpp"               /* midi::eventd, midi::byte         */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_api class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */

namespace rtl
{

/*------------------------------------------------------------------------
 * Free functions in the rtl namespace
 *------------------------------------------------------------------------*/

/*
 *  Rtl66 library version information string. The two macros used are
 *  specified in the top-level meson.build file as additional build
 *  argurments.
 */

const std::string &
get_rtl_midi_version () noexcept
{
    static std::string s_info = RTL66_NAME "-" RTL66_VERSION " " __DATE__ ;
    return s_info;
}

/**
 *  A free function to determine the current RtMidi version version used to
 *  create the Rtl66 library. See include/rtl/rtl_build_macros.h.
 */

const std::string &
get_rtmidi_version () noexcept
{
    static const std::string s_version = RTL66_RTMIDI_VERSION;
    return s_version;
}

/**
 *  A free function to determine the RtMidi version used as the basis of this
 *  implementation. Updated to match the latest when patching in new
 *  RtMidi changes is performed. See include/rtl/rtl_build_macros.h.
 */

const std::string &
get_rtmidi_patch_version () noexcept
{
    static const std::string s_version = RTL66_RTMIDI_PATCHED;
    return s_version;
}

/*------------------------------------------------------------------------
 * rtl namespace static members
 *------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 * rtmidi static members
 *------------------------------------------------------------------------*/

rtmidi::api rtmidi::sm_desired_api  = rtmidi::api::unspecified; /* use fallback */
rtmidi::api rtmidi::sm_selected_api = rtmidi::api::unspecified; /* selected one */

/*------------------------------------------------------------------------
 * rtmidi
 *------------------------------------------------------------------------*/

RTL66_DLL_PUBLIC
rtmidi::rtmidi () :
    m_rt_api_ptr  (nullptr)                         /* rt_api_ptr() access  */
{
    // No code
}

/*
 * Make sure these move functions are correct!!!
 */

rtmidi::rtmidi (rtmidi && other) noexcept :
    m_rt_api_ptr (other.m_rt_api_ptr)
{
    other.m_rt_api_ptr = nullptr;
}

rtmidi &
rtmidi::operator = (rtmidi && other) noexcept
{
    if (this != &other)
    {
        m_rt_api_ptr = other.m_rt_api_ptr;
        other.m_rt_api_ptr = nullptr;
    }
    return *this;
}

rtmidi::~rtmidi()
{
    delete_rt_api_ptr();
}

/**
 *  Returns the MIDI API specifier for the current instance of
 *  rtmidi_in/out. Currently a virtual function.
 */

rtmidi::api
rtmidi::get_current_api () noexcept
{
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_current_api() : rtmidi::api::unspecified ;
}

void
rtmidi::delete_rt_api_ptr ()
{
    if (not_nullptr(m_rt_api_ptr))
    {
        delete m_rt_api_ptr;
        m_rt_api_ptr = nullptr;
    }
}

/*------------------------------------------------------------------------
 * rtmidi static functions and data
 *------------------------------------------------------------------------*/

/**
 *  Define API names and display names.  Must be in same order as the
 *  rtl::rtmidi::api enumeration class in rtmidi.hpp and the list returned
 *  by get_detected_apis().
 *
 *  They must also be C-linkable to be used in the rtmidi_c module.
 */

static const std::string cs_api_names[][2] =
{
    /*
     *   API name        Display name
     */

    { "unspecified",    "Fallback"              },
    { "pipewire",       "PipeWire"              },  /* TODO!                */
    { "jack",           "JACK"                  },
    { "alsa",           "ALSA"                  },
    { "macosx_core",    "CoreMidi"              },
    { "windows_mm",     "Windows MultiMedia"    },
    { "windows_uwp",    "Windows UWP"           },  /* Microsoft-deprecated */
    { "android_midi",   "Android MIDI API"      },  /* Not yet supported    */
    { "web_midi",       "Web MIDI API"          },
    { "dummy",          "Dummy"                 }
};

/*
 * Unused:
 *
 *  static const int cs_api_names_count =
 *      sizeof(cs_api_names)/sizeof(cs_api_names[0]);
 */

static const rtmidi::api_list cs_compiled_apis
{
    rtmidi::api::unspecified,
#if defined RTL66_BUILD_PIPEWIRE
    rtmidi::api::pipewire,
#endif
#if defined RTL66_BUILD_JACK
    rtmidi::api::jack,
#endif
#if defined RTL66_BUILD_ALSA
    rtmidi::api::alsa,
#endif
#if defined RTL66_BUILD_MACOSX_CORE
    rtmidi::api::macosx_core,
#endif
#if defined RTL66_BUILD_WIN_MM
    rtmidi::api::windows_mm,
#endif
#if defined RTL66_BUILD_WIN_UWP
    rtmidi::api::windows_uwp,               /* Microsoft-deprecated         */
#endif
#if defined RTL66_BUILD_ANDROID
    rtmidi::api::android_midi,              /* not yet coded at all         */
#endif
#if defined RTL66_BUILD_WEB_MIDI
    rtmidi::api::web_midi,
#endif
#if defined RTL66_BUILD_DUMMY
    rtmidi::api::dummy
#endif
};

/**
 *  A static function.
 */

rtmidi::api
rtmidi::api_by_index (int index)
{
    if (index >= 0 && index < int(cs_compiled_apis.size()))
        return cs_compiled_apis[index];
    else
        return rtmidi::api::unspecified;
}

int
rtmidi::api_count ()
{
    return int(cs_compiled_apis.size());
}

/**
 *  A static function to determine the available compiled MIDI APIs.  The
 *  values returned in the std::vector can be compared against the enumerated
 *  list values.  Note that there can be more than one API compiled for
 *  certain operating systems.
 *
 *  Now updated to find only compiled and detected APIs.
 */

void
rtmidi::get_compiled_apis (rtmidi::api_list & apis) noexcept
{
    apis = cs_compiled_apis;
}

/**
 *  A static function to get the APIs detected at run-time. Note that
 *  on Linux, the order of detection is Pipewire, JACK, then ALSA.
 */

void
rtmidi::get_detected_apis (rtmidi::api_list & apis) noexcept
{
    apis = detected_apis();
}

const rtmidi::api_list &
rtmidi::detected_apis () noexcept
{
    static bool s_uninitialized = true;
    static rtmidi::api_list s_api_list;
    if (s_uninitialized)
    {
#if defined RTL66_BUILD_PIPEWIRE
        if (detect_pipewire())
        {
            s_api_list.push_back(rtmidi::api::pipewire);
            s_uninitialized = false;
        }
#endif
#if defined RTL66_BUILD_JACK
        if (detect_jack(true, false))           /* check ports, no recheck  */
        {
            s_api_list.push_back(rtmidi::api::jack);
            s_uninitialized = false;
        }
#endif
#if defined RTL66_BUILD_ALSA
        if (detect_alsa(true))
        {
            s_api_list.push_back(rtmidi::api::alsa);
            s_uninitialized = false;
        }
#endif
#if defined RTL66_BUILD_MACOSX_CORE
        if (detect_core())
        {
            s_api_list.push_back(rtmidi::api::macosx_core);
            s_uninitialized = false;
        }
#endif
#if defined RTL66_BUILD_WIN_MM
        if (detect_win_mm())
        {
            s_api_list.push_back(rtmidi::api::windows_mm);
            s_uninitialized = false;
        }
#endif
#if defined RTL66_BUILD_WEB_MIDI
        if (detect_web_midi())
        {
            s_api_list.push_back(rtmidi::api::web_midi);
            s_uninitialized = false;
        }
#endif
#if defined RTL66_BUILD_DUMMY
        if (s_uninitialized && detect_dummy())
        {
            s_api_list.push_back(rtmidi::api::dummy);
            s_uninitialized = false;
        }
#endif

        /*
         * To do: handle one we get the basic functionality written.
         *
         * RTL66_BUILD_WIN_UWP
         * RTL66_BUILD_ANDROID
         */
    }
    return s_api_list;
}

void
rtmidi::show_apis (const std::string & tag, const api_list & apis)
{
    printf("%s:\n", tag.c_str());
    for (auto a : apis)
    {
        printf
        (
            "%12s: %s\n",
            cs_api_names[midiapi_to_int(a)][0].c_str(),
            cs_api_names[midiapi_to_int(a)][1].c_str()
        );
    }
}

/**
 *  Determines if the selected api is actually present on the system.
 *  Brute-force lookup, but the list is short.
 */

bool
rtmidi::is_detected_api (rtmidi::api rapi)
{
    bool result = false;
    const rtmidi::api_list & apilist = rtmidi::detected_apis();
    for (rtmidi::api a : apilist)
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
 * \static member function
 *
 *  Determines the MIDI API to use when no API is specified.  Currently, only
 *  the Linux APIs need this, but the rest return their platform's API, if
 *  compiled in and detected.
 *
 *  On Linux, the order of detection is Pipewire, JACK, then ALSA.
 *
 * \return
 *      Returns the first valid, operational MIDI API that is found.
 *      If there is none, rtl::rtmidi::api::max (or dummy???) is returned.
 */

rtmidi::api
rtmidi::fallback_api ()
{
    rtmidi::api result = rtmidi::api::max;
    const rtmidi::api_list & apilist = rtmidi::detected_apis();
    if (! apilist.empty())
        result = apilist[0];

    return result;
}

/**
 * \static member function
 *
 *  Return the name of a specified compiled MIDI API.
 *
 *  This obtains a short lower-case name used for identification purposes.
 *  This value is guaranteed to remain identical across library versions.
 *  If the API is unknown, this function will return the empty string.
 */

std::string
rtmidi::api_name (rtmidi::api rapi)
{
    std::string result;
    if (rapi >= rtmidi::api::unspecified && rapi < rtmidi::api::max)
        result = cs_api_names[midiapi_to_int(rapi)][0];

    return result;
}

/**
 * \static
 */

std::string
rtmidi::selected_api_name ()
{
    return api_name(selected_api());
}

/**
 *  Return the display name of a specified compiled MIDI API.
 *
 *  This obtains a long name used for display purposes.  If the API is
 *  unknown, this function will return the empty string.
 */

std::string
rtmidi::api_display_name (rtmidi::api rapi)
{
    std::string result;
    if (rapi >= rtmidi::api::unspecified && rapi < rtmidi::api::max)
        result = cs_api_names[midiapi_to_int(rapi)][1];

    return result;
}

/**
 * \static
 */

std::string
rtmidi::selected_api_display_name ()
{
    return api_display_name(selected_api());
}

/**
 *  Return the compiled MIDI API having the given name.
 *
 *  A case insensitive comparison will check the specified name against the
 *  list of compiled APIs, and return the one which matches. On failure, the
 *  function returns rtmidi::api::unspecified.
 */

rtmidi::api
rtmidi::api_by_name (const std::string & name)
{
    const rtmidi::api_list & available_apis = cs_compiled_apis;
    for (auto a : available_apis)
    {
        if (name == cs_api_names[midiapi_to_int(a)][0])
          return a;
    }
    return rtmidi::api::unspecified;
}

void
rtmidi::silence_messages (bool silent)      /* static */
{
#if defined RTL66_BUILD_JACK
    silence_jack_messages(silent);
#else
    (void) silent;                          /* do not warn about non-usage  */
#endif
}

#if defined RTL66_BUILD_JACK

static bool s_start_jack = false;

void
rtmidi::start_jack (bool flag)
{
    s_start_jack = flag;
}

bool
rtmidi::start_jack ()
{
    return s_start_jack;
}

#endif

/*------------------------------------------------------------------------
 * rtmidi virtual base-class functions
 *------------------------------------------------------------------------*/

bool
rtmidi::open_port (int portnumber, const std::string & portname)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
    {
        std::string pn = portname;
        if (portnumber >= 0)
        {
            pn += " ";
            pn += std::to_string(portnumber);
        }
        result = rt_api_ptr()->open_port(portnumber, portname);
    }
    return result;
}

bool
rtmidi::open_virtual_port (int portnumber, const std::string & portname)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
    {
        std::string pn = portname;
        if (portnumber >= 0)
        {
            pn += " ";
            pn += std::to_string(portnumber);
        }
        result = rt_api_ptr()->open_virtual_port(portname);
    }
    return result;
}

bool
rtmidi::open_virtual_port (const std::string & portname)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->open_virtual_port(portname);

    return result;
}

/**
 *  This function is meant for making one connection to the MIDI engine, and
 *  is a kind of extension to RtMidi.  It can also be used by the more
 *  conventional RtMidi connect() function.
 */

void *
rtmidi::engine_connect ()
{
    void * result = nullptr;
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->engine_connect();

    return result;
}

void
rtmidi::engine_disconnect ()
{
    if (not_nullptr(rt_api_ptr()))
        rt_api_ptr()->engine_disconnect();
}

/**
 * This function is meant for connecting the application to the JACK
 * port graph. Not yet sure if other APIs support this concept.  If
 * not, they won't override the default simplistic implementation in
 * midi_api.
 */

bool
rtmidi::engine_activate ()
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->engine_activate();

    return result;
}

/**
 * This function is meant for disconnecting the application from the JACK
 * port graph. Not yet sure if other APIs support this concept.  If
 * not, they won't override the default simplistic implementation in
 * rtl::midi_api.
 */

bool
rtmidi::engine_deactivate ()
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->engine_activate();

    return result;
}

bool
rtmidi::set_client_name (const std::string & clientname)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->set_client_name(clientname);

    return result;
}

bool
rtmidi::set_port_name (const std::string & portname)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->set_port_name(portname);

    return result;
}

/**
 *  Flush an open MIDI connection to ensure the events are emptied.
 *  Normatlly for output.
 */

bool
rtmidi::flush_port ()
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->flush_port();

    return result;
}

/**
 *  Close an open MIDI connection (if one exists).
 */

bool
rtmidi::close_port ()
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->close_port();

    return result;
}

bool
rtmidi::is_port_open () const
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->is_port_open();

    return result;
}

int
rtmidi::get_port_count ()
{
    int result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->get_port_count();

    return result;
}

/**
 *  Return a string identifier for the specified MIDI port type and number.
 *
 * \return
 *      The name of the port with the given Id is returned.  An empty string is
 *      returned if an invalid port specifier is provided. User code should
 *      assume a UTF-8 encoding.
 */

std::string
rtmidi::get_port_name (int portnumber)
{
    std::string result;
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->get_port_name(portnumber);

    return result;
}

/*------------------------------------------------------------------------
 * Extensions
 *------------------------------------------------------------------------*/

int
rtmidi::get_io_port_info (midi::ports & ports, bool preclear)
{
    int result = 0;
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->get_io_port_info(ports, preclear);

    return result;
}

#if defined RTL66_MIDI_EXTENSIONS       // defined in Linux, FIXME

/**
 *  Some versions of JACK support this concept. However, do we want to do this
 *  lookup via port name or port number?
 */

std::string
rtmidi::get_port_alias (const std::string & portname)
{
    std::string result;
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->get_port_alias(portname);

    return result;
}

/**
 *  Changes the PPQN in the MIDI engine, as well as in the member variable.
 *  Not all APIs support this concept.
 */

bool
rtmidi::PPQN (midi::ppqn ppq)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->PPQN(ppq);

    return result;
}

midi::ppqn
rtmidi::PPQN () const
{
    return not_nullptr(rt_api_ptr()) ? rt_api_ptr()->PPQN() : 0 ;
}

/**
 *  Changes the BPM in the MIDI engine, as well as in the member variable.
 *  Not all APIs support this concept.
 */

bool
rtmidi::BPM (midi::bpm bp)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->BPM(bp);

    return result;
}

midi::bpm
rtmidi::BPM () const
{
    return not_nullptr(rt_api_ptr()) ? rt_api_ptr()->BPM() : 0 ;
}

/**
 *  Set an error callback function to be invoked when an error has occured.
 *
 *  The callback function will be called whenever an error has occured. It is
 *  best to set the error callback function before opening a port.
 */

void
rtmidi::set_error_callback (rterror::callback_t cb, void * userdata)
{
    if (not_nullptr(rt_api_ptr()))
        rt_api_ptr()->set_error_callback(cb, userdata);
}

bool
rtmidi::send_byte (midi::byte evbyte)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->send_byte(evbyte);

    return result;
}

/**
 *  MIDI clock from the "master" controls the playback rate of MIDI slaves.
 *  24 MIDI clocks are send during every quarter-note interval. The sequence
 *  is explained at:
 *
 *      https://jazzdup.nfshost.com/tutorial/tech/midispec/seq.htm
 *
 *      -   Master sends MIDI Start.
 *      -   The slave prepares for an incoming MIDI Clock; it is now in "play
 *          mode."
 *      -   Immediately, or after a small (1 ms) interval, the master sends
 *          a MIDI Clock, which marks the initial downbeat (MIDI Beat 0) of
 *          the song.
 *      -   The master keeps sending MIDI Clocks.
 *      -   The master sends a MIDI Stop. It can continue to send MIDI Clocks,
 *          but...
 *      -   ... The slave immediately stops and ignores any incoming MIDI
 *          Clocks, except perhaps to keep track of tempo.
 *      -   The master can send a MIDI Stop.
 *      -   The slave stores the current Song Position at the stop for use
 *          with a possible MIDI Continue.
 *      -   To start beyond the beginning, the master sends a Song Position
 *          of value S while the playback is stopped. S is a 14-bit value
 *          equal to the MIDI Beat on which to start playback. Each MIDI
 *          Beat is spans 6 MIDI Clocks (i.e. a 16th note.) The number of
 *          MIDI Clocks is given by C = 6S.
 *      -   The slave stores the current Song Position for use when
 *          continuing.
 *      -   The master then sends a MIDI Continue instead of a MIDI Start.
 *      -   Immediately the master sends a MIDI Clock downbeat to continue
 *          playback.
 *
 *  Q. What are MIDI Tick messages (sent every 10 ms)?
 *
 *  The following function should send MIDI Start.
 */

bool
rtmidi::clock_start ()
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        rt_api_ptr()->clock_start();

    return result;
}

/**
 *  The following function sends MIDI Clock.
 *
 *  TODO: look at midibus::clock() in the Seq32 project. It looks based on teh
 *  tick. Is this just an ALSA thing.
 *
 */

bool
rtmidi::clock_send (midi::pulse tick)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        rt_api_ptr()->clock_send(tick);

    return result;
}

/**
 *  The following function sends MIDI Stop.
 */

bool
rtmidi::clock_stop ()
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        rt_api_ptr()->clock_stop();

    return result;
}

/**
 *  The following function sends MIDI Continue.
 */

bool
rtmidi::clock_continue (midi::pulse tick, int beats)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->clock_continue(tick, beats);

    return result;
}

int
rtmidi::poll_for_midi ()
{
    int result = 0;
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->poll_for_midi();

    return result;
}

bool
rtmidi::get_midi_event (midi::event * inev)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->get_midi_event(inev);

    return result;
}

bool
rtmidi::send_event (const midi::event * ev, midi::byte channel)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->send_event(ev, channel);

    return result;
}

bool
rtmidi::send_message (const midi::byte * msg, size_t sz)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->send_message(msg, sz);

    return result;
}

bool
rtmidi::send_message (const midi::message & msg)
{
    bool result = not_nullptr(rt_api_ptr());
    if (result)
        result = rt_api_ptr()->send_message(msg);

    return result;
}

#endif  // defined RTL66_MIDI_EXTENSIONS

/**
 *  Simply provides some common code for the rtmidi-derived constructors.
 */

#if defined RTL66_USE_GLOBAL_CLIENTINFO

rtmidi::api
rtmidi::ctor_common_setup (rtmidi::api rapi, const std::string & clientname)
{
    std::string cname = clientname;
    if (cname.empty())                                      // NOT it!
        cname = midi::global_client_info().client_name();   // REMOVE
    else
       midi::global_client_info().client_name(cname);      // IFFY

    if (rapi == rtmidi::api::unspecified)
        rapi = fallback_api();

    return rapi;
}

#else

rtmidi::api
rtmidi::ctor_common_setup
(
    rtmidi::api rapi,
    const std::string & // clientname
)
{
    if (rapi == rtmidi::api::unspecified)
        rapi = fallback_api();

    return rapi;
}

#endif

}           // namespace rtl

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

