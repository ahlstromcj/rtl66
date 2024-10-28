#if ! defined RTL66_SESSION_RTLCONFIGURATION_HPP
#define RTL66_SESSION_RTLCONFIGURATION_HPP

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
 * \file          rtlconfiguration.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of administering a session of rtl66 usage.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2020-05-30
 * \updates       2024-01-15
 * \license       GNU GPLv2 or above
 *
 *  This class provides a process for starting, running, restarting, and
 *  closing down the Seq66 application, even without session management.  One
 *  of the goals is to be able to reload the player when the set of MIDI
 *  devices in the system changes.
 */

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */
#include <string>                       /* std::string                      */

#include "session/configuration.hpp"    /* session::configuration class     */

namespace session
{

/**
 *  Provides the session configuration.  This replaces many of the variables
 *  in the old Seq66 rcsettings class, though eventually these will be
 *  stored in the 'rc' in a completely compatible manner.
 */

class rtlconfiguration : public configuration
{

    friend class rtlmanager;

public:

    /**
     *  Provides a unique pointer to an rtlconfiguration, not to be confused
     *  with a configuration. Note that this alias is not directly used by the
     *  enclosing class.
     */

    using pointer = std::unique_ptr<rtlconfiguration>;

private:

    /**
     *  An indicator of the capabilities of an application, mostly of use in
     *  the Non/New Session Manager.
     */

    std::string m_capabilities;

    /**
     *  Holds the path to the user's desired location for MIDI files.
     */

    std::string m_midi_filepath;

    /**
     *  Holds the base name of the current MIDI file (e.g. "minetune.mid").
     */

    std::string m_midi_filename;

    /**
     *  If true, loaded the named MIDI file stored in the rtlconfiguration.
     */

    bool m_load_midi_file;

public:

    rtlconfiguration (const std::string & caps = "");
    rtlconfiguration (rtlconfiguration &&) = delete;
    rtlconfiguration (const rtlconfiguration &) = default;
    rtlconfiguration & operator = (rtlconfiguration &&) = delete;
    rtlconfiguration & operator = (const rtlconfiguration &) = default;
    virtual ~rtlconfiguration ();

    // set_config_files("rtl66cli");

    virtual void set_defaults () override
    {
        // TODO
    }

#if 0
    virtual int parse_command_line
    (
        int argc, char * argv [], std::string & errmessage
    ) override;
#endif

public:

    const std::string & capabilities () const
    {
        return m_capabilities;
    }

    bool load_midi_file () const
    {
        return m_load_midi_file;
    }

    const std::string & midi_filepath () const
    {
        return m_midi_filepath;
    }

    const std::string & midi_filename () const
    {
        return m_midi_filename;
    }

protected:

    void load_midi_file (bool flag)
    {
        m_load_midi_file = flag;
    }

    void midi_filepath (const std::string & f)
    {
        m_midi_filepath = f;
    }

    void midi_filename (const std::string & f)
    {
        m_midi_filename = f;
    }

};          // class rtlconfiguration

}           // namespace session

#endif      // RTL66_SESSION_RTLCONFIGURATION_HPP

/*
 * rtlconfiguration.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

