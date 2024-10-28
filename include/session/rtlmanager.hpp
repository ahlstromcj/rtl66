#if ! defined RTL66_SESSION_RTLMANAGER_HPP
#define RTL66_SESSION_RTLMANAGER_HPP

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
 * \file          rtlmanager.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of administering a session of rtl66 usage.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2020-05-30
 * \updates       2023-01-01
 * \license       GNU GPLv2 or above
 *
 *  This class provides a process for starting, running, restarting, and
 *  closing down the Seq66 application, even without session management.  One
 *  of the goals is to be able to reload the player when the set of MIDI
 *  devices in the system changes.
 */

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */

#include "session/manager.hpp"          /* session::manager base class      */
#include "session/rtlconfiguration.hpp" /* session::rtlconfiguration basics */
#include "midi/player.hpp"              /* midi::player (performer base)    */

namespace session
{

/**
 *  This class supports managing a run of an rtl66-based application.
 */

class rtlmanager : public manager
{

public:

    /**
     *  Provides a unique pointer to a player, to enable the player to
     *  be recreated.
     */

    using pointer = std::unique_ptr<midi::player>;

private:

    /**
     *  Pointer to a configuration object.
     */

    rtlconfiguration::pointer m_config_ptr;

    /**
     *  Provides a pointer to the player to be managed.  This player can
     *  be removed and recreated as needed (e.g. when another MIDI device
     *  comes online.)
     */

    pointer m_player_ptr;

    /**
     *  Holds the capabilities string (if applicable) for the application
     *  using this session manager.  Meant mainly for NSM, which returns
     *  the capabilities provided.
     */

public:

    rtlmanager (const std::string & caps = "");
    rtlmanager (rtlmanager &&) = delete;
    rtlmanager (const rtlmanager &);
    rtlmanager & operator = (const rtlmanager &&) = delete;
    rtlmanager & operator = (const rtlmanager &);
    virtual ~rtlmanager ();

    const std::string & config_filename () const
    {
        static std::string s_dummy{""};
//      return not_nullptr(config_ptr()) ?
//          config_ptr()->config_filename() : s_dummy ;
        return s_dummy;
    }

    const std::string & log_filename () const
    {
        static std::string s_dummy{""};
//      return not_nullptr(config_ptr()) ?
//          config_ptr()->log_filename() : s_dummy ;
        return s_dummy;
    }

    const std::string & midi_filename () const
    {
        static std::string s_dummy{""};
//      return not_nullptr(config_ptr()) ?
//          config_ptr()->midi_filename() : s_dummy ;
        return s_dummy;
    }

    bool load_midi_file () const
    {
        return not_nullptr(config_ptr()) ?
            config_ptr()->load_midi_file() : false ;
    }

    virtual bool internal_error_pending () const override
    {
        return bool(player_ptr()) ?
            player_ptr()->error_pending() : true ;
    }

    bool make_path_names
    (
        const std::string & path,
        std::string & outcfgpath,
        std::string & outmidipath,
        const std::string & midisubdir = ""
    );
    bool import_into_session
    (
        const std::string & path,
        const std::string & sourcebase
    );
    bool import_configuration
    (
        const std::string & sourcepath,
        const std::string & sourcebase,
        const std::string & cfgfilepath,
        const std::string & midifilepath
    );


public:

    virtual bool parse_option_file (std::string & errmessage) override;
    virtual bool parse_command_line
    (
        int argc, char * argv[],
        std::string & errmessage
    ) override;
    virtual bool write_option_file (std::string & errmessage) override;
    virtual bool create_session (int argc = 0, char * argv [] = nullptr) override;
    virtual bool close_session (std::string & msg, bool ok = true) override;
    virtual bool save_session (std::string & msg, bool ok = true) override;
    virtual bool create_project
    (
        int, char * [],
        const std::string &
        // int argc, char * argv [],
        // const std::string & path
    ) override
    {
        // TODO????
        return false;
    }
    virtual bool create_manager (int argc, char * argv []) override;
    virtual bool settings (int argc, char * argv []) override;
    virtual bool create_player ();
    virtual std::string open_midi_file (const std::string & fname);
    virtual bool run () override
    {
        return false;               // TODO???
    }

    virtual bool open_playlist ()
    {
        return false;
    }

    virtual bool open_note_mapper ()
    {
        return false;
    }

    virtual void show_message
    (
        const std::string & tag,
        const std::string & msg
    ) const override;
    virtual void show_error
    (
        const std::string & tag,
        const std::string & msg
    ) const override;

protected:

    virtual bool create_window () override
    {
        return true;
    }

protected:

    const rtlconfiguration * config_ptr () const
    {
        return m_config_ptr.get();
    }

    rtlconfiguration * config_ptr ()
    {
        return m_config_ptr.get();
    }

    const midi::player * player_ptr () const
    {
        return m_player_ptr.get();
    }

    midi::player * player_ptr ()
    {
        return m_player_ptr.get();
    }

    void midi_filename (const std::string & fname)
    {
        if (not_nullptr(config_ptr()))
            config_ptr()->midi_filename(fname);
    }

    void load_midi_file (bool flag)
    {
        if (not_nullptr(config_ptr()))
            config_ptr()->load_midi_file(flag);
    }

    void log_filename (const std::string & fname)
    {
        (void) fname;
//      if (not_nullptr(config_ptr()))
//          config_ptr()->log_filename(fname);
    }

    void config_filename (const std::string & fname)
    {
        (void) fname;
//      if (not_nullptr(config_ptr()))
//          config_ptr()->config_filename(fname);
    }

    bool create_midi_configuration
    (
        int argc, char * argv [],
        const std::string & mainpath,
        const std::string & cfgfilepath,
        const std::string & midifilepath
    );
    bool read_midi_configuration
    (
        int argc, char * argv [],
        const std::string & cfgfilepath,
        const std::string & midifilepath
    );

};          // class rtlmanager

}           // namespace session

#endif      // RTL66_SESSION_RTLMANAGER_HPP

/*
 * rtlmanager.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

