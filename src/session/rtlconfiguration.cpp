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
 * \file          rtlconfiguration.cpp
 *
 *  This module declares/defines a module for managing a generic rtl66
 *  session.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-22
 * \updates       2023-01-01
 * \license       GNU GPLv2 or above
 *
 */

#include <cstring>                      /* std::strlen()                    */

#include "session/rtlconfiguration.hpp" /* sessions::rtlconfiguration()     */
#include "util/msgfunctions.hpp"        /* util::msgprintf()                */
#include "util/filefunctions.hpp"       /* util::file_readable() etc.       */

namespace session
{

/**
 *  Does the usual construction.  It also calls set_defaults() from the
 *  settings.cpp module in order to guarantee that we have rc() and usr()
 *  available.  See that function for more information.
 */

rtlconfiguration::rtlconfiguration (const std::string & caps) :
    configuration       (),
    m_capabilities      (caps),
    m_midi_filepath     (),
    m_midi_filename     (),
    m_load_midi_file    (false)
{
    // set_rtlconfiguration_defaults();
}

/**
 *  This might be a good place to save the rtlconfiguration.
 */

rtlconfiguration::~rtlconfiguration ()
{
    // no code so far
}

#if 0

/**
 *      -   Parse log option.
 *      -   Parse --option/-o options into a structure?
 *      -   Other options.
 *
 *  If "-o log=file.ext" occurred, handle it early on in startup.
 *
 *  The user migh specify -o options that are also set up in the rtlconfiguration
 *  file; the command line must take precedence. The "log" option is processed
 *  early in the startup sequence.
 */

int
rtlconfiguration::parse_command_line
(
    int argc, char * argv [],
    std::string & errmessage
)
{
    int optionindex = -1;
    (void) argc;
    (void) argv;
    errmessage = "Not implemented, please program an implementation";
#if defined THIS_CODE_IS_READY
//      optionindex = parse_command_line(argc, argv, errmessage);
//      result = optionindex >= 0;
//      result = parse_o_options(argc, argv);

    std::string logfile = usr().option_logfile();
    if (usr().option_use_logfile())
        (void) xpc::reroute_stdio(logfile);

    m_midi_filename.clear();
    if (optionindex < argc)                 /* MIDI filename given? */
    {
        std::string fname = argv[optionindex];
        std::string errmsg;
        if (util::file_readable(fname))
        {
            std::string path;               /* not used here        */
            std::string basename;
            m_midi_filename = fname;
            if (filename_split(fname, path, basename))
                rc().midi_filename(basename);
        }
        else
        {
            char temp[512];
            (void) snprintf
            (
                temp, sizeof temp,
                "MIDI file not readable: '%s'", fname.c_str()
            );
//          append_error_message(temp);     /* raises the message   */
            m_midi_filename.clear();
        }
    }

#endif
    return optionindex;
}

#endif

}           // namespace session

/*
 * rtlconfiguration.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

