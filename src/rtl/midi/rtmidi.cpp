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
 * \updates       2025-12-20
 * \license       See above.
 *
 *  A member function correlation and check-list can be found in
 *  extras/notes/member-mappings.text.
 */

#include "platform_macros.h"            /* operating system detection       */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_api class              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi class, etc.          */

namespace rtl
{

/**
 *  Though defaulted, defined here so that the size of midi_api can be
 *  known.
 */

rtmidi::~rtmidi ()
{
    // No code
}

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
    static std::string s_info { RTL66_NAME "-" RTL66_VERSION " " __DATE__ };
    return s_info;
}

/**
 *  A free function to determine the current RtMidi version version used to
 *  create the Rtl66 library. See include/rtl/rtl_build_macros.h.
 */

const std::string &
get_rtmidi_version () noexcept
{
    static const std::string s_version { RTL66_RTMIDI_VERSION };
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
    static const std::string s_version { RTL66_RTMIDI_PATCHED };
    return s_version;
}

/*--------------------------------------------------------------------------
 * rtl namespace rtmidi static members
 *--------------------------------------------------------------------------*/

rtmidi::api rtmidi::sm_desired_api                          /* use fallback */
{
    rtmidi::api::unspecified
};
rtmidi::api rtmidi::sm_selected_api                         /* selected one */
{
    rtmidi::api::unspecified
};

/*--------------------------------------------------------------------------
 * rtmidi
 *--------------------------------------------------------------------------*/

/**
 *  Returns the MIDI API specifier for the current instance of
 *  rtmidi_in/out. Currently a virtual function.
 */

rtmidi::api
rtmidi::get_current_api () noexcept
{
    return rt_api_ptr() ?
        rt_api_ptr()->get_current_api() : rtmidi::api::unspecified ;
}

void
rtmidi::rt_api_ptr (midi_api * p)
{
    m_rt_api_ptr.reset(p);
}

void
rtmidi::delete_rt_api_ptr ()
{
    if (m_rt_api_ptr)
        m_rt_api_ptr.reset();
}

void *
rtmidi::api_data ()
{
    return m_rt_api_ptr->api_data();
}

const void *
rtmidi::api_data () const
{
    return m_rt_api_ptr->api_data();
}

/**
 *  Used for the rtl libraries "bus" concept. It sets:
 *
 *      -   The client handle (e.g. the snd_seq_t pointer) for this
 *          rtmidi object.
 *      -   The masterbus pointer for the midi_api-derived object.
 *      -   The client handle for the midi_api-derived object,
 *          which get copied into the API's data structure (e.g.
 *          midi_alsa_data). See the master_client_ptr() function.
 *
 *  This function is called in these contexts:
 *
 *      -   rtmidi_engine::open_midi_api(). This is called when the
 *          rtmidi_engine is set up, so that the masterbus gets
 *          the MIDI API's client pointer [a.k.a. client_handle()]
 *      -   midi::bus_in() and midi_bus_out(), sort of. This gives
 *          rtmidi_in and rtmidi_out access to the client handle.
 */

bool
rtmidi::set_master_bus (midi::masterbus * mb)
{
    bool result { not_nullptr(mb) };
    if (result)
    {
        void * rvch { rt_api_ptr()->void_client_handle() };
        result = not_nullptr(rvch);
        if (result)
        {
            mb->void_client_handle(rvch);       /* first log client handle  */
            result = set_master_bus_ptr(mb);    /* then log the masterbus   */
        }
#if defined PLATFORM_DEBUG_TMI
        printf
        (
            "set_master_bus() pointers:\n"
            "  rt_api_ptr() = %p\n"
            "  \" client handle = %p\n",
            (void *)(rt_api_ptr()), rvch
        );
#endif
    }
    return result;
}

/**
 *  This function assumes the function above has been called already
 *  to set up the masterbus with the data it needs at startup.
 *  This function is used to set up the masterbus for usage by the
 *  midi::bus.
 *
 *  This function is called in these contexts:
 *
 *      -   midi::bus_in().
 *      -   midi::bus_out().
 */

bool
rtmidi::set_master_bus_ptr (midi::masterbus * mb)
{
    bool result { not_nullptr(mb) && not_nullptr(rt_api_ptr()) };
#if defined PLATFORM_DEBUG_TMI
        printf("masterbus * mb = %p\n", (void *)(mb));
#endif
    if (result)
    {
        void * mvch { mb->void_client_handle() };
        result = not_nullptr(mvch);
        if (result)
        {
            master_client_ptr(mvch);
            rt_api_ptr()->master_bus(mb);
        }
    }
    return true;
}

void
rtmidi::master_client_ptr (void * p)
{
    if (not_nullptr(p))
    {
        m_master_client_ptr = p;    // perhaps unnecessary
        rt_api_ptr()->void_client_handle(p);
        m_has_master = true;
    }
    else
        m_has_master = false;
}

/*--------------------------------------------------------------------------
 * rtmidi static functions and data
 *--------------------------------------------------------------------------*/

/**
 *  Define API names and display names.  Must be in same order as the
 *  rtl::rtmidi::api enumeration class in rtmidi.hpp and the list returned
 *  by get_detected_apis().
 *
 *  They must also be C-linkable to be used in the rtmidi_c module.
 */

static const std::string cs_api_names[][2]
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
    { "dummy",          "Dummy"                 },
    { "none",           "None"                  }   /* currently for tests  */
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
    static bool s_uninitialized { true };
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
        if (detect_jack(false))                 /* check ports, no recheck  */
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
    printf("%s:\n", V(tag));
    for (auto a : apis)
    {
        printf
        (
            "%12s: %s\n",
            V(cs_api_names[midiapi_to_int(a)][0]),
            V(cs_api_names[midiapi_to_int(a)][1])
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
    bool result { false };
    const rtmidi::api_list & apilist { rtmidi::detected_apis() };
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
    rtmidi::api result { rtmidi::api::max };
    const rtmidi::api_list & apilist { rtmidi::detected_apis() };
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
    const rtmidi::api_list & available_apis { cs_compiled_apis };
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

static bool s_start_jack { false };

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

/**
 *  Constructs a port name by appending the portnumber. Used if an
 *  empty port name is provided.
 */

std::string
rtmidi::numbered_port_name (int pnumber, const std::string & pname)
{
    std::string result { pname };
    if (pnumber >= 0)
    {
        result += " ";
        result += std::to_string(pnumber);
    }
    return result;
}

/*--------------------------------------------------------------------------
 * rtmidi virtual base-class functions
 *--------------------------------------------------------------------------*/

bool
rtmidi::open_port (int portnumber, const std::string & portname)
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->open_port(portnumber, portname);

    return result;
}

bool
rtmidi::open_virtual_port (int portnumber, const std::string & portname)
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
    {
        std::string pn { portname };
        if (portnumber >= 0)
        {
            pn += " ";
            pn += std::to_string(portnumber);
        }
        result = rt_api_ptr()->open_virtual_port(pn);
    }
    return result;
}

bool
rtmidi::open_virtual_port (const std::string & portname)
{
    bool result { not_nullptr(rt_api_ptr()) };
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
    void * result { nullptr };
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
    bool result { not_nullptr(rt_api_ptr()) };
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
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->engine_activate();

    return result;
}

bool
rtmidi::set_client_name (const std::string & clientname)
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->set_client_name(clientname);

    return result;
}

bool
rtmidi::set_port_name (const std::string & portname)
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->set_port_name(portname);

    return result;
}

/**
 *  Flush an open MIDI *client* connection to ensure the events are
 *  emptied. Normatlly for output. Supported by ALSA, but no JACK.
 */

bool
rtmidi::flush ()
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->flush();

    return result;
}

/**
 *  Flush an open MIDI connection to ensure the events are emptied.
 *  Normatlly for output.
 */

bool
rtmidi::flush_port (midi::bussbyte b)
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->flush_port(b);

    return result;
}

/**
 *  Close an open MIDI connection (if one exists).
 */

bool
rtmidi::close_port ()
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->close_port();

    return result;
}

bool
rtmidi::is_port_open () const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->is_port_open();

    return result;
}

int
rtmidi::get_port_count ()
{
    int result { 0 };
    if (not_nullptr(rt_api_ptr()))
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

/**
 *  If there are no port aliases (as with the ALSA API and with
 *  software synths running in JACK), then this function is just liek
 *  get_port_name() defined above.
 *
 *  Otherwise, it finds a better name, which is usually given, in
 *  JACK, by the second alias.
 */

std::string
rtmidi::best_port_name (int portnumber)
{
    std::string result;
    if (not_nullptr(rt_api_ptr()))
    {
        result = rt_api_ptr()->get_port_name(portnumber);

        std::string alias1 { rt_api_ptr()->get_port_alias(result, 1) };
        if (! alias1.empty())
        {
            result = alias1;            /* this seems to be the best one    */
        }
        else
        {
            std::string alias0 { rt_api_ptr()->get_port_alias(result, 0) };
            if (! alias0.empty())
                result = alias0;        /* might be the engine (alsa_pcm)   */
        }
    }
    return result;
}

/*------------------------------------------------------------------------
 * Extensions
 *------------------------------------------------------------------------*/

int
rtmidi::get_io_port_info (midi::ports & ports, bool preclear)
{
    int result { 0 };
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
rtmidi::get_port_alias
(
    const std::string & portname, int aliasno
)
{
    std::string result;
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->get_port_alias(portname, aliasno);

    return result;
}

/**
 *  Issue: a bit intrusive.
 *
 *  Use clientinfo INSTEAD.
 */

lib66::tokenization
rtmidi::get_port_aliases (const std::string & portname)
{
    static lib66::tokenization s_dummy;                 /* a size 0 vector  */
    return not_nullptr(rt_api_ptr()) ?
        rt_api_ptr()->get_port_aliases(portname) : s_dummy;
}

/**
 *  Changes the PPQN in the MIDI engine, as well as in the member variable.
 *  Not all APIs support this concept.
 */

bool
rtmidi::PPQN (midi::ppqn ppq)
{
    bool result { not_nullptr(rt_api_ptr()) };
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
    bool result { not_nullptr(rt_api_ptr()) };
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
    bool result { not_nullptr(rt_api_ptr()) };
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
    bool result { not_nullptr(rt_api_ptr()) };
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
    bool result { not_nullptr(rt_api_ptr()) };
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
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->clock_continue(tick, beats);

    return result;
}

int
rtmidi::poll_for_midi () const
{
    int result { 0 };
    if (not_nullptr(rt_api_ptr()))
        result = rt_api_ptr()->poll_for_midi();

    return result;
}

bool
rtmidi::get_midi_event (midi::event * inev)
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->get_midi_event(inev);

    return result;
}

/**
 *  Fill the user-provided vector with the data bytes for the next available
 *  MIDI message in the input queue and return the event delta-time in seconds.
 *
 *  This function returns immediately whether a new message is available or
 *  not.  A valid message is indicated by a non-zero vector size.  An exception
 *  is thrown if an error occurs during message retrieval or an input
 *  connection was not previously established.
 */

midi::message
rtmidi::get_message ()
{
    midi::message result;
    bool ok { not_nullptr(rt_api_ptr()) };
    if (ok)
        result = rt_api_ptr()->get_message();

    return result;
}

bool
rtmidi::send_byte (midi::byte evbyte) const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->send_byte(evbyte);

    return result;
}

bool
rtmidi::send_event (const midi::event * ev, midi::byte channel) const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->send_event(ev, channel);

    return result;
}

bool
rtmidi::send_message (const midi::message & msg) const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->send_message(msg);

    return result;
}

bool
rtmidi::send_message (const midi::bytes & msg) const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->send_message(msg.data(), msg.size());

    return result;
}

bool
rtmidi::send_message (const midi::byte * msg, size_t sz) const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->send_message(msg, sz);

    return result;
}

bool
rtmidi::send_sysex (const midi::event * ev) const
{
    bool result { not_nullptr(rt_api_ptr()) };
    if (result)
        result = rt_api_ptr()->send_sysex(ev);

    return result;
}

#endif  // defined RTL66_MIDI_EXTENSIONS

/**
 *  Simply provides some common code for the rtmidi-derived constructors.
 */

rtmidi::api
rtmidi::ctor_common_setup (rtmidi::api rapi, const std::string & clientname)
{
    if (rapi == rtmidi::api::unspecified)
        rapi = fallback_api();

    if (! clientname.empty())
       midi::global_client_info().client_name(clientname);

    return rapi;
}

}           // namespace rtl

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

