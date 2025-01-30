#if ! defined RTL66_RTL_MIDI_RTMIDI_HPP
#define RTL66_RTL_MIDI_RTMIDI_HPP

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
 * \file          rtmidi.hpp
 *
 *      A reworking of RtMidi.hpp, with the same functionality but different
 *      coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-01-30
 * \license       See above.
 *
 *      Also contains some additional capabilities.
 */

#include <string>                       /* omnipresent std::string class    */

#include "c_macros.h"                   /* is_nullptr() macro               */
#include "midi/message.hpp"             /* midi::message data class         */
#include "midi/midibytes.hpp"           /* midi::ppqn, midi::bpm            */
#include "rtl/rtl_build_macros.h"       /* RTL66_DLL_PUBLIC, etc.           */
#include "rtl/rterror.hpp"              /* rtl::rterror::callback_t         */

/*
 *  Most functions here don't need to be virtual.  Fix after tests.
 */

#define VIRTUAL

/*
 * Note the support for the namespaces of midi and rtl::midi.
 */

namespace midi
{
    class event;
    class ports;
}

namespace rtl
{

class midi_api;                         /* forward reference for pointer    */

/**
 *  rtmidi_in / rtmidi_out are "controllers" used to select an available MIDI
 *  input or output interface.  They present common APIs for the user to call
 *  but all functionality is implemented by the classes midi_in_api,
 *  midi_out_api and their subclasses.  rtmidi_in and rtmidi_out each create an
 *  instance of a midi_in_api or midi_out_api subclass based on the user's API
 *  choice.  If no choice is made, they attempt to make a "logical" API
 *  selection.
 *
 *  The is_port_open() returns true if a port is open and false if not.
 *  Note that this only applies to connections made with the open_port()
 *  function, not to virtual ports.
 */

class RTL66_DLL_PUBLIC rtmidi
{

public:

    /**
     *    MIDI API specifier arguments.  These items used to be nested in
     *    the rtl66 class, but that only worked when RtMidi.cpp/h were
     *    large monolithic modules. Compare rtmidi::api to rtaudio::api.
     */

    enum class api
    {
        unspecified,        /**< Search for a working compiled API.         */
        pipewire,           /**< TODO! Will not be ready for quire awhile.  */
        jack,               /**< Linux/UNIX JACK Low-Latency MIDI Server.   */
        alsa,               /**< Advanced Linux Sound Architecture API.     */
        macosx_core,        /**< Macintosh OS-X Core Midi API.              */
        windows_mm,         /**< Microsoft Multimedia MIDI API.             */
        windows_uwp,        /**< Windows Universal Platform (deprecated).   */
        android_midi,       /**< Android MIDI API (not yet supported here). */
        web_midi,           /**< The Web MIDI API.                          */
        dummy,              /**< A compilable but non-functional API.       */
        max                 /**< A count of APIs; an erroneous value.       */
    };

    /**
     *  Provides a container of MIDI API values.
     */

    using api_list = std::vector<api>;

private:

    /**
     *  Provides the user's preferred API, if any.
     */

    static rtmidi::api sm_desired_api;

    /**
     *  Provides the API active for the duration of this run.  For example,
     *  under Linux we search for JACK, but if not found, fall back to
     *  ALSA.
     *
     *  To save repeated queries, we save this value.  Its default value is
     *  rtmidi::api::unspecified.  The enum class rtmidi::api is defined in
     *  rtmidi.hpp.
     *
     *  New to this "rtmidi"-derived library.
     */

    static rtmidi::api sm_selected_api;

private:

    /**
     *  Provides a pointer to the selected API implementation, such as
     *  midi_alsa or midi_jack.
     */

    midi_api * m_rt_api_ptr;

public:

    rtmidi (rtmidi && other) noexcept;
    rtmidi & operator = (rtmidi && other) noexcept;

protected:

    rtmidi ();

    /*
     * Make the class non-copyable
     */

    rtmidi (rtmidi & other) = delete;
    rtmidi & operator = (rtmidi & other) = delete;

public:

    /*
     * Must be public to use base-class pointer. See the usage of
     * std::unique_ptr<> in the test_helpers.cpp module.
     */

    virtual ~rtmidi ();

    /*--------------------------------------------------------------------
     * Public static functions
     *--------------------------------------------------------------------*/

    static rtmidi::api api_by_index (int index);
    static int api_count ();
    static void get_compiled_apis (api_list & apis) noexcept;
    static void get_detected_apis (api_list & apis) noexcept;
    static const api_list & detected_apis () noexcept;
    static void show_apis (const std::string & tag, const api_list & apis);
    static bool is_detected_api (rtmidi::api rapi);
    static rtmidi::api fallback_api ();
    static std::string api_name (rtmidi::api rapi);
    static std::string selected_api_name ();
    static std::string api_display_name (rtmidi::api rapi);
    static std::string selected_api_display_name ();
    static rtmidi::api api_by_name (const std::string & name);
    static void silence_messages (bool silent);

#if defined RTL66_BUILD_JACK
    static void start_jack (bool flag);
    static bool start_jack ();
#endif

    static rtmidi::api & desired_api ()
    {
        return sm_desired_api;
    }

    static rtmidi::api & selected_api ()
    {
        return sm_selected_api;
    }

    static void desired_api (rtmidi::api rapi)
    {
        sm_desired_api = rapi;
    }

    static void selected_api (rtmidi::api rapi)
    {
        sm_selected_api = rapi;
    }

    /*
     *  Gets the active API if the API pointer is good.  Otherwise
     *  rtmidi::api::unspecified is returned.
     */

    virtual rtmidi::api get_current_api () noexcept;

    /*
     * API client functions.
     */

    /*
     * API port functions. Where these virtual functions are implemented,
     * they perform a test of the MIDI API pointer before calling the API
     * function. Note that some of these functions do not need to be virtual
     * because the polymorphism is implemented in the midi_api-based classes.
     * But we have some slightly tailoring for input vs. output.
     */

    virtual bool open_port
    (
        int portnumber = 0,
        const std::string & portname = ""
    );
    virtual bool open_virtual_port      /* an extension to the RtMidi API   */
    (
        int portnumber = 0,
        const std::string & portname = ""
    );
    virtual bool open_virtual_port (const std::string & portname = "");

    /*--------------------------------------------------------------------
     * Callers of virtual midi_api member functions
     *--------------------------------------------------------------------*/

    void * engine_connect ();
    void engine_disconnect ();
    bool engine_activate ();
    bool engine_deactivate ();
    bool set_client_name (const std::string & clientname);
    bool set_port_name (const std::string & portname);
    bool flush_port();
    bool close_port();
    bool is_port_open() const;
    int get_port_count ();
    std::string get_port_name (int portnumber = 0);

    /*--------------------------------------------------------------------
     * Extensions
     *--------------------------------------------------------------------*/

    int get_io_port_info
    (
        midi::ports & inputports, bool preclear = true
    );

#if defined RTL66_MIDI_EXTENSIONS       // defined in Linux, FIXME

    std::string get_port_alias (const std::string & portname);  // int??
    bool PPQN (midi::ppqn ppq);
    midi::ppqn PPQN () const;
    bool BPM (midi::bpm bp);
    midi::bpm BPM () const;
    void set_error_callback
    (
        rterror::callback_t cb,
        void * userdata = nullptr
    );
    bool send_byte (midi::byte evbyte);
    bool clock_start ();
    bool clock_send (midi::pulse tick);
    bool clock_stop ();
    bool clock_continue (midi::pulse tick, int beats);
    int poll_for_midi ();
    bool get_midi_event (midi::event * inev);
    bool send_event (const midi::event * ev, midi::byte channel);
    bool send_message (const midi::message & msg);
    bool send_message (const midi::byte * msg, size_t sz);

#endif  // defined RTL66_MIDI_EXTENSIONS

#if defined ADDITIONAL_FUNCTIONS_ARE_READY

    VIRTUAL bool play (const event * inev, midi::byte channel) = 0;

    // Utilities:
    //
    //  create_ringbuffer() [JACK]
    //  register_port()
    //  connect_ports()
    //  set_virtual_name()

#endif

public:

    midi_api * rt_api_ptr ()
    {
        return m_rt_api_ptr;
    }

    const midi_api * rt_api_ptr () const
    {
        return m_rt_api_ptr;
    }

protected:

    void rt_api_ptr (midi_api * p)
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

    virtual bool open_midi_api
    (
        rtmidi::api rapi                = rtmidi::api::unspecified,
        const std::string & clientname  = "",
        unsigned queuesize              = 0     /* useful with input ports  */
    ) = 0;

    rtmidi::api ctor_common_setup
    (
        rtmidi::api rapi, const std::string & clientname
    );

};          // class rtmidi

/*------------------------------------------------------------------------
 * Free and inline functions in the rtl namespace
 *------------------------------------------------------------------------*/

inline rtmidi::api
int_to_midiapi (int index)
{
    bool valid = index >= 0 && index < static_cast<int>(rtmidi::api::max);
    return valid ? static_cast<rtmidi::api>(index) : rtmidi::api::max ;
}

inline int
midiapi_to_int (rtmidi::api rapi)
{
    return static_cast<int>(rapi);
}

inline bool
is_midiapi_valid (rtmidi::api rapi)
{
    return rapi >= rtmidi::api::unspecified && rapi < rtmidi::api::max;
}

extern const std::string & get_rtl_midi_version () noexcept;
extern const std::string & get_rtmidi_version () noexcept;
extern const std::string & get_rtmidi_patch_version () noexcept;

/*------------------------------------------------------------------------
 * rtmidi API-detection function declarations.  Each is defined in the
 * appropriate midi_xxxxx module. Also included are some slightly useful
 * helper functions.
 *------------------------------------------------------------------------*/

#if defined RTL66_BUILD_PIPEWIRE
extern bool detect_pipewire ();
#endif

#if defined RTL66_BUILD_JACK

/*
 * These declarations duplicate those in midi_jack.hpp!
 */

extern bool detect_jack (bool forcecheck);          /* = false */
extern void silence_jack_errors (bool silent);
extern void silence_jack_info (bool silent);
extern void silence_jack_messages (bool silent);
#endif

#if defined RTL66_BUILD_ALSA
extern bool detect_alsa (bool checkports);
#endif

#if defined RTL66_BUILD_MACOSX_CORE
extern bool detect_core ();
#endif

#if defined RTL66_BUILD_WIN_MM
extern bool detect_win_mm ();
#endif

#if defined RTL66_BUILD_WEB_MIDI
extern bool detect_web_midi ();
#endif

#if defined RTL66_BUILD_DUMMY
extern bool detect_dummy ();
#endif

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_RTMIDI_HPP

/*
 * rtmidi.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

