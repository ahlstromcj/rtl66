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
 * \file          find_midi_api.cpp
 *
 *    A reworking of RtMidi.cpp, with the same functionality but different
 *    coding conventions.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-07-23
 * \updates       2025-12-23
 * \license       See above.
 *
 */

#include "rtl/midi/find_midi_api.hpp"       /* rtl API searching functions  */
#include "rtl/midi/midi_dummy.hpp"          /* rtl::midi_web classes        */
#include "rtl/midi/alsa/midi_alsa.hpp"      /* rtl::midi_alsa classes       */
#include "rtl/midi/jack/midi_jack.hpp"      /* rtl::midi_jack classes       */
#include "rtl/midi/macosx/midi_macosx_core.hpp" /* rtl::midi_core classes   */
#include "rtl/midi/pipewire/midi_pipewire.hpp"  /* rtl::midi_pipewire class */
#include "rtl/midi/webmidi/midi_web_midi.hpp"   /* rtl::midi_web classes    */
#include "rtl/midi/winmm/midi_win_mm.hpp"   /* rtl::midi_win_mm classes     */

#if defined LIBS66_USE_POTEXT
#include "po/potext.hpp"                /* the po::gettext() interfaces     */
#else
#define _(str)      (str)
#define N_(str)     str
#endif

namespace rtl
{

/*--------------------------------------------------------------------------
 * rtmidi helper functions for derived classes
 *--------------------------------------------------------------------------*/

/**
 *  \deprecated ?
 *
 *  This function probes for an existing MIDI API engine.  It tries to
 *  test an output port.  Either input or output would work. Once detected,
 *  the port is deleted; we just want to find an API.
 *
 *  However, this would set up a JACK process prematurely, so let's just
 *  use the functions detect_alsa(), detect_jack(), etc.
 *
 *  \param desiredapi
 *      Provides the desired API.  The value rtmidi::api::unspecified means that
 *      a preferred API (or a fallback if the preferred API does not exist)
 *      will be selectied if possible. This is the default value.
 *
 *  \param cname
 *      Provide the desired setting for the name of the client.
 *      By default this value is empty.
 */

rtmidi::api
find_midi_api (rtmidi::api desiredapi, std::string cname)
{
    rtmidi::api result { rtmidi::api::unspecified };
    midi_api * m { nullptr };
    if (desiredapi == rtmidi::api::unspecified)
        desiredapi = rtmidi::fallback_api();

    /*
     * When looking for a usable MIDI API, we don't need to set up
     * callbacks, just a temporary client handle. We use the dummy I/O value.
     * Perhaps for clarity we should make a "find" value? Yes.
     *
     * m = try_open_midi_api(desiredapi, midi::port::io::output, cname);
     */

    m = try_open_midi_api(desiredapi, midi::port::io::finder, cname);
    if (not_nullptr(m))
    {
        delete m;
        result = desiredapi;
    }
    return result;
}

static bool
try_match (rtmidi::api current, rtmidi::api target)
{
    return current == rtmidi::api::unspecified || current == target;
}

/**
 *  Similar to try_open_midi_api(), but does not create a midi_api-derived
 *  object. This is one way to avoid setting up a JACK process
 *  prematurely, for example.
 */

rtmidi::api
detect_midi_api (rtmidi::api rapi)
{
    rtmidi::api result { rtmidi::api::unspecified };
    if (rapi != rtmidi::api::max)
    {
#if defined RTL66_BUILD_PIPEWIRE
        if (try_match(rapi, rtmidi::api::pipewire))
        {
            if (detect_pipewire(true))
                result = rtmidi::api::pipewire;
        }
#endif
#if defined RTL66_BUILD_JACK
        if (result == rtmidi::api::unspecified)
        {
            if (try_match(rapi, rtmidi::api::jack))
            {
                if (detect_jack(true))
                    result = rtmidi::api::jack;
            }
        }
#endif
#if defined RTL66_BUILD_ALSA
        if (result == rtmidi::api::unspecified)
        {
            if (try_match(rapi, rtmidi::api::alsa))
            {
                if (detect_alsa(true))
                    result = rtmidi::api::alsa;
            }
        }
#endif
#if defined RTL66_BUILD_MACOSX_CORE
        if (result == rtmidi::api::unspecified)
        {
            if (try_match(rapi, rtmidi::api::macosx_core))
            {
                if (detect_macosx_core(true))
                    result = rtmidi::api::macosx_core;
            }
        }
#endif
#if defined RTL66_BUILD_WIN_MM
        if (result == rtmidi::api::unspecified)
        {
            if (try_match(rapi, rtmidi::api::windows_mm))
            {
                if (detect_win_mm(true))
                    result = rtmidi::api::windows_mm;
            }
        }
#endif
#if defined RTL66_BUILD_WEB_MIDI
        if (result == rtmidi::api::unspecified)
        {
            if (try_match(rapi, rtmidi::api::web_midi))
            {
                if (detect_web_midi(true))
                    result = rtmidi::api::web_midi;
            }
        }
#endif
#if defined RTL66_BUILD_DUMMY
        if (result == rtmidi::api::unspecified)
        {
            result = rtmidi::api::dummy;
        }
#endif

        /*
         * To do:  Add RTL66_BUILD_WIN_UWP and RTL66_BUILD_ANDROID.
         */

        if (result == rtmidi::api::unspecified)
            error_print("find_midi_api", "no support for API");
    }
    return result;
}

/**
 *  This function accesses only the supported rtl::rtmidi::api values, and
 *  creates a new "midi_in_xxx" object only if a match is found. Note that,
 *  under Linux, JACK is tried first.
 *
 *  For Linux, there is a fallback process.  If there is no API specified,
 *  then the APIs are tried in the following order:
 *
 *      -   PipeWire (TODO! Currently it is a dummy implementation.)
 *      -   JACK
 *      -   ALSA
 */

midi_api *
try_open_midi_api
(
    rtmidi::api rapi,
    midi::port::io iotype,
    const std::string & clientname,
    unsigned qsize
)
{
    midi_api * result { nullptr };
    try
    {
        if (rapi != rtmidi::api::max)
        {
#if defined RTL66_BUILD_PIPEWIRE
            if (try_match(rapi, rtmidi::api::pipewire))
                result = new midi_pipewire_in(clientname, qsize);
#endif
#if defined RTL66_BUILD_JACK
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::jack))
                    result = new midi_jack(iotype, clientname, qsize);
            }
#endif
#if defined RTL66_BUILD_ALSA
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::alsa))
                    result = new midi_alsa(iotype, clientname, qsize);
            }
#endif
#if defined RTL66_BUILD_MACOSX_CORE
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::macosx_core))
                    result = new midi_macosx_core(clientname, qsize);
            }
#endif
#if defined RTL66_BUILD_WIN_MM
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::windows_mm))
                    result = new midi_win_mm(clientname, qsize);
            }
#endif
#if defined RTL66_BUILD_WEB_MIDI
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::web_midi))
                    result = new midi_web_midi(clientname, qsize);
            }
#endif
#if defined RTL66_BUILD_DUMMY
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::dummy))
                    result = new midi_dummy(clientname, qsize);
            }
#endif

            /*
             * To do:  Add RTL66_BUILD_WIN_UWP and RTL66_BUILD_ANDROID.
             */

            if (is_nullptr(result))
            {
                error_print("try_open_midi_api_in", "no support for API");

#if defined RTL66_THROW_RTERROR
                std::string errormsg =
                    "find_midi_api(in): no API support found";

                throw(rterror(errormsg, rterror::kind::unspecified));
#endif
            }
        }
    }
    catch (rtl::rterror & error)
    {
        error.print_message();
    }
    catch (...)
    {
        std::string msg { _("Unknown exception... fix the catch") };
        errprint(CSTR(msg));
    }
    return result;
}

/**
 *  RTL66_FULL_MASTERBUS_SUPPORT
 *
 *  This overload differs in the following ways:
 *
 *      -   It uses the master bus to get the API parameters, stored
 *          in the masterbus's midi::clientinfo structure:
 *          -   masterbus::selected_api()
 *          -   masterbus::port_type()      [not used here]
 *          -   masterbus::client_name()    [not used here]
 *          -   masterbus::queue_size()     [not used here]
 *      -   It calls the default MIDI API constructors, which do
 *          not call the initialize() function.
 *      -   It sets the masterbus for the MIDI API object.
 */

midi_api *
try_open_midi_api (midi::masterbus & mb, midi::port::io iotype)
{
    midi_api * result { nullptr };
    rtmidi::api rapi { mb.selected_api() };
    std::string clientname { mb.client_name() };

    try
    {
        if (rapi != rtmidi::api::max)
        {
#if defined RTL66_BUILD_PIPEWIRE
            if (try_match(rapi, rtmidi::api::pipewire))
                result = new midi_pipewire_in();
#endif
#if defined RTL66_BUILD_JACK
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::jack))
                    result = new midi_jack(mb, iotype, true); // formaster
            }
#endif
#if defined RTL66_BUILD_ALSA
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::alsa))
                    result = new midi_alsa(mb, iotype, true); // formaster
            }
#endif
#if defined RTL66_BUILD_MACOSX_CORE
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::macosx_core))
                    result = new midi_macosx_core();
            }
#endif
#if defined RTL66_BUILD_WIN_MM
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::windows_mm))
                    result = new midi_win_mm();
            }
#endif
#if defined RTL66_BUILD_WEB_MIDI
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::web_midi))
                    result = new midi_web_midi();
            }
#endif
#if defined RTL66_BUILD_DUMMY
            if (is_nullptr(result))
            {
                if (try_match(rapi, rtmidi::api::dummy))
                    result = new midi_dummy();
            }
#endif

            /*
             * To do:  Add RTL66_BUILD_WIN_UWP and RTL66_BUILD_ANDROID.
             */

            if (is_nullptr(result))
            {
                error_print("try_open_midi_api_in", "no support for API");

#if defined RTL66_THROW_RTERROR
                std::string errormsg =
                    "find_midi_api(in): no API support found";

                throw(rterror(errormsg, rterror::kind::unspecified));
#endif
            }
        }
    }
    catch (rtl::rterror & error)
    {
        error.print_message();
    }
    catch (...)
    {
        std::string msg { _("Unknown exception... fix the catch") };
        errprint(CSTR(msg));
    }
    return result;
}

}           // namespace rtl

/*
 * find_midi_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

