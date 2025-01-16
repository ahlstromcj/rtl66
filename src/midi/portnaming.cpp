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
 * \file          portnaming.cpp
 *
 *  This module declares/defines some utility functions and portnaming
 *  needed by this application.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2022-12-18
 * \updates       2023-07-20
 * \license       GNU GPLv2 or above
 *
 */

#include <cctype>                       /* std::isspace(), std::isdigit()   */
#include <cmath>                        /* std::floor(), std::log()         */
#include <cstdlib>                      /* std::atoi(), std::strtol()       */
#include <cstring>                      /* std::memset()                    */
#include <ctime>                        /* std::strftime()                  */

#include "midi/portnaming.hpp"          /* MIDI-related port-name handling  */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi::selected_api()      */
#include "util/strfunctions.hpp"        /* util::strncompare(), simplify()  */

namespace midi
{

/**
 *  Indicates if one string can be found within another.  Doesn't force the
 *  caller to use size_type. Also see util::contains() in strfunctions.
 */

bool
contains (const std::string & original, const std::string & target)
{
    auto pos = original.find(target);
    return pos != std::string::npos;
}

/**
 *  Extracts the two names from the ALSA/JACK client/port name format:
 *
 *      [0] 128:0 client name:port name
 *
 *  When a2jmidid is running:
 *
 *      a2j:Midi Through [14] (playback): Midi Through Port-0
 *
 *  with "a2j" as the client name and the rest, including the second colon, as
 *  the port name.  For that kind of name, use extract_a2j_port_name().
 *
 * \param fullname
 *      The full port specification to be split.
 *
 * \param [out] clientname
 *      The destination for the client name portion, "clientname".
 *
 * \param [out] portname
 *      The destination for the port name portion, "portname".
 *
 * \return
 *      Returns true if all items are non-empty after the process.
 */

bool
extract_port_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    bool result = ! fullname.empty();
    clientname.clear();
    portname.clear();
    if (result)
    {
        std::string cname;
        std::string pname;
        std::size_t colonpos = fullname.find_first_of(":"); /* not last! */
        if (colonpos != std::string::npos)
        {
            /*
             * The client name consists of all characters up the the first
             * colon.  Period.  The port name consists of all characters
             * after that colon.  Period.
             */

            cname = fullname.substr(0, colonpos);
            pname = fullname.substr(colonpos+1);
            result = ! cname.empty() && ! pname.empty();
        }
        else
            pname = fullname;

        clientname = cname;
        portname = pname;
    }
    return result;
}

/**
 *  Extracts the buss name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "bus" portion of the string.  If there is no colon, then
 *      it is assumed there is no buss name, so an empty string is returned.
 */

std::string
extract_bus_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(0, colonpos) : std::string("");
}

/**
 *  Extracts the port name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "port" portion of the string.  If there is no colon, then
 *      it is assumed that the name is a port name, and so \a fullname is
 *      returned.
 */

std::string
extract_port_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(colonpos + 1) : fullname ;
}

/**
 *  Looks for the port name in the short-name list. We are interested in
 *  seeing if it is a generic name such as "midi in".
 *
 * \param portname
 *      The name to be checked.  This is the name after the colon in a
 *      "client:port" pair.
 *
 * \return
 *      Returns true if the port-name is found in the short-name list, or is
 *      empty. This is a signal to get the nick-name from the client name and
 *      the portname.
 */

static bool
detect_short_name (const std::string & portname)
{
    static const std::string s_short_names [] =
    {
        "midi_",
        "midi ",
        "in",
        "out",
        "input",
        "output",
        ""                              /* empty string is a terminator     */
    };
    bool result = portname.empty();
    if (! result)
    {
        for (int i = 0; /* forever */; ++i)
        {
            std::string compared = s_short_names[i];
            if (compared.empty())
            {
                break;                              /* there is no match    */
            }
            else
            {
                result = util::strncompare(compared, portname);
                if (result)
                    break;                          /* a match was found    */
            }
        }
    }
    return result;
}

static int
count_colons (const std::string & name)
{
    int result = 0;
    for (std::string::size_type cpos = 0; ; ++cpos)
    {
        cpos = name.find_first_of(":", cpos + 1);
        if (cpos != std::string::npos)
            ++result;
        else
            break;
    }
    return result;
}

/**
 *  The nick-name of a port is roughly all the text following the last colon
 *  in the display-name [see midibase::display_name()].  It seems to be the
 *  same text whether the port name comes from ALSA or from a2jmidid when
 *  running JACK.  We don't have any MIDI hardware that JACK detects without
 *  a2jmidid.
 *
 *  QSynth has a name like the following, which breaks the algorithm and makes
 *  the space position far outside the bounds of the string.  In that case, we
 *  punt and get the whole string.  Also see extract_port_names() in the
 *  calculations module.  Another issue is that each incarnation of QSynth
 *  produces a name with a different port number.
 *
\verbatim
        [6] 130:0 FLUID Synth (125507):Synth input port (125507:0)
\endverbatim
 *
 *  Other cases to handle:
 *
\verbatim
        "[3] 36:0 Launchpad Mini MIDI 1"
        a2j:Midi Through [14] (playback): Midi Through Port-0
\endverbatim
 *
 */

std::string
extract_nickname (const std::string & name)
{
    std::string result;
    int colons = count_colons(name);
    if (colons > 2)
    {
        if (rtl::rtmidi::selected_api() == rtl::rtmidi::api::jack)
        {
            auto cpos = name.find_last_of(":");
            ++cpos;
            if (name[cpos] == ' ')
                cpos = name.find_first_not_of(" ", cpos);

            result = name.substr(cpos);
        }
        else
        {
            auto cpos = name.find_first_of(":");
            auto spos = name.find_first_of(" ", cpos);
            if (spos != std::string::npos)
            {
                ++spos;
                cpos = name.find_first_of(":", cpos + 1);
                result = name.substr(spos, cpos - spos);
            }
        }
    }
    else
    {
        auto cpos = name.find_last_of(":");
        if (cpos != std::string::npos)
        {
            ++cpos;
            if (std::isdigit(name[cpos]))
            {
                cpos = name.find_first_of(" ", cpos);
                if (cpos != std::string::npos)
                    ++cpos;
            }
            else if (std::isspace(name[cpos]))
                ++cpos;

            if (cpos == std::string::npos)
                cpos = 0;

            result = name.substr(cpos);
        }
    }
    if (detect_short_name(result))
    {
        std::string clientname, portname;
        bool extracted = extract_port_names(name, clientname, portname);
        if (extracted)
            result = clientname + ":" + portname;

        if (result == name)
            result = util::simplify(result);    /* can we call only this?? */
    }
    else
    {
        auto ppos = result.find_first_of("(");  /* happens with fluidsynth  */
        if (ppos != std::string::npos && ppos > 1)
        {
            --ppos;
            if (result[ppos] == ' ')
                --ppos;

            result = result.substr(0, ppos + 1);
        }
    }
    if (result.empty())
        result = name;

    return result;
}

/**
 *  Sets the name to be displayed for showing to the user, and hopefully,
 *  later, for look-up.
 *
 *  NOT YET USED.
 *
 *  For JACK ports created by a2jmidid (a2j_control), we want to shorten the
 *  name radically, and also set the bus ID, which is contained in square
 *  brackets.
 *
 *  - ALSA: "[0] 14:0 Midi Through Port-0"
 *  - JACK: "[0] 0:0 rtl66:system midi_playback_1"
 *  - A2J:  "[0] 0:0 rtl66:a2j Midi Through [14] (playback): Midi Through Port-0"
 *
 *  Skip past the two colons to get to the main part of the name.  Extract it,
 *  and prepend "a2j".
 *
 * \param alias
 *      The system-supplied or a2jmidid name for the port.  One example:
 *      "a2j:Midi Through [14] (playback): Midi Through Port-0".  Obviously,
 *      this function depends on the formatting of a2jmidid name assignments
 *      not changing.
 *
 * \sideeffect
 *      The bus ID is also modified, if present in the string (see "[14]"
 *      above).
 *
 * \return
 *      Returns the bare port-name is "a2j" appears in the alias. Otherwise, and
 *      empty string is returned.
 */

std::string
extract_a2j_port_name (const std::string & alias)
{
    std::string result;
    if (contains(alias, "a2j"))
    {
        auto lpos = alias.find_first_of(":");
        if (lpos != std::string::npos)
        {
            lpos = alias.find_first_of(":", lpos + 1);
            if (lpos != std::string::npos)
            {
                result = alias.substr(lpos + 2);
                result = "A2J " + result;
            }
        }
    }
    return result;
}

}       // namespace midi

/*
 * portnaming.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

