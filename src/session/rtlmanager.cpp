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
 * \file          rtlmanager.cpp
 *
 *  This module declares/defines a module for managing a generic rtl66
 *  session.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-22
 * \updates       2024-01-16
 * \license       GNU GPLv2 or above
 *
 *  This module provides functionality that is useful even if session support
 *  is not enabled.
 *
 *  The process:
 *
 *      -# Call settings(argc, argv).  It sets defaults, does some parsing
 *         of command-line options and files.  It saves the MIDI file-name, if
 *         provided.
 *      -# Call create_player(), which could delete an existing player.
 *         It then launches the player.  Save the unique-pointer.
 *      -# If the MIDI file-name is set, open it via a call to open_midi_file().
 *      -# If a user-interface is needed, create a unique-pointer to it, then
 *         show it.  This will remove any previous pointer.  The function is
 *         virtual, create_window().
 */

#include <cstring>                      /* std::strlen()                    */

#include "platform_macros.h"            /* PLATFORM_UNIX, etc.              */
#include "cfg/appinfo.hpp"              /* cfg::set_app_name()              */
#include "midi/file.hpp"                /* midi::write_midi_file()          */
#include "midi/player.hpp"              /* midi::player class               */
#include "rtl/test_helpers.hpp"         /* rt_simple_cli()                  */
#include "session/rtlmanager.hpp"       /* session::rtlmanager()            */
#include "util/msgfunctions.hpp"        /* util::file_message() etc.        */
#include "util/filefunctions.hpp"       /* util::file_readable() etc.       */
#include "xpc/daemonize.hpp"            /* xpc::session_setup() etc.        */

/**
 *  The default path to the configuration files.  Generally needs to be
 *  overridden in derived implementations.
 */

#if defined PLATFORM_UNIX
static const std::string c_home_directory = "~/.config/rtl66";
#elif defined PLATFORM_WINDOWS
static const std::string c_home_directory = "C:/Users/doofus/AppData/Local";
#endif

namespace session
{

/**
 *  Does the usual construction.
 */

rtlmanager::rtlmanager (const std::string & /* caps */) :
    manager         (),
    m_config_ptr    (),
    m_player_ptr    ()                              /* player_ptr() accessor */
{
    /*
     * This has to wait: m_player_ptr = create_player();
     */

    // set_configuration_defaults();
}

/**
 *  The pointers are unique, so copying and assignment should replicate them.
 */

rtlmanager::rtlmanager  (const rtlmanager & rhs) :
    manager         (rhs),
    m_config_ptr    (),
    m_player_ptr    ()                              /* player_ptr() accessor */
{
    m_config_ptr.reset(new (std::nothrow) rtlconfiguration(rhs.capabilities()));

    /*
     * TODO:
     * m_player_ptr.reset(new (std::nothrow) midi::player(....);
     */
}

/**
 *  Note the base-class operator =() call.  If we implement the move operator,
 *  the line would be:
 *
 *      (void) manager::operator =(std::move(rhs));
 */

rtlmanager &
rtlmanager::operator = (const rtlmanager & rhs)
{
    if (this != & rhs)
    {
        (void) manager::operator =(rhs);
        m_config_ptr.reset(new (std::nothrow) rtlconfiguration(rhs.capabilities()));

        /*
         * TODO:
         * m_player_ptr.reset(new (std::nothrow) midi::player(....);
         */
    }
    return *this;
}

/**
 *  We found that on a Debian developer laptop, this destructor took a couple
 *  of seconds to call get_deleter().  Works fine on our Ubuntu developer
 *  laptop.  Weird.  Actually might have been a side-effect of installing a
 *  KxStudio PPA.
 */

rtlmanager::~rtlmanager ()
{
    if (! is_help())
        (void) util::info_message("Exiting session rtlmanager");
}

/**
 *  The standard C/C++ entry point to this application.  The first thing is to
 *  set the various settings defaults, and then try to read the "user" and
 *  "rc" configuration files, in that order.  There are currently no options
 *  to change the names of those files.  If we add that code, we'll move the
 *  parsing code to where the configuration file-names are changed from the
 *  command-line.  The last thing is to override any other settings via the
 *  command-line parameters.
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if this code worked properly and was not a request for
 *      help/version information.
 */

bool
rtlmanager::settings (int argc, char * argv [])
{
    // TODO: get the app name from arg 0
    //
    bool result = rt_simple_cli("session::rtlmanager", argc, argv);
    if (cfg::get_app_cli())
    {
        cfg::set_app_name("rtl66cli");
        cfg::set_app_type("cli");
        cfg::set_client_name("rtl66cli");
        // rc().set_config_files("rtl66cli");
    }
//  else
//      cfg::set_app_name(RTL66_APP_NAME);   /* "qrtl66" by default          */

    cfg::set_arg_0(argv[0]);                 /* how it got started           */

    bool ishelp = rt_show_help();
//  bool sessionmodified = false;
    if (ishelp)
    {
        is_help(true);
    }
    if (result)
    {
        /*
         */

//      if (! sessionmodified)
//          (void) cmdlineopts::parse_log_option(argc, argv);

        /*
         *  If parsing fails, report it and disable usage of the application and
         *  saving bad garbage out when exiting.  Still must launch, otherwise a
         *  segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        result = parse_option_file(errmessage);
        if (result)
        {
            result = parse_command_line(argc, argv, errmessage);
            if (! result)
                append_error_message(errmessage);
        }
        else
            append_error_message(errmessage);
    }
    return result;
}

bool
rtlmanager::parse_option_file (std::string & errmessage)
{
    bool result = not_nullptr(config_ptr());
    errmessage = "parse_option_file() not implemented";
#if THIS_CODE_IS_READY
    if (result)
        result = config_ptr()->parse_file(errmessage);
#endif

    return result;
}

bool
rtlmanager::parse_command_line
(
    int argc, char * argv[],
    std::string & errmessage
)
{
    bool result = not_nullptr(config_ptr());
    errmessage = "parse_command_line() not implemented";
    (void) argc;
    (void) argv;
//  if (result)
//      result = config_ptr()->parse_command_line(argc, argv, errmessage);

    return result;
}

bool
rtlmanager::write_option_file (std::string & errmessage)
{
    errmessage = "write_option_file() not yet implemented";
    return false;
}

/**
 *  This function is currently meant to be called by the owner of this
 *  rtlmanager.  This call must occur before creating the application main
 *  window.
 *
 *  Otherwise, rtl66 will not register with NSM (if enabled) in a timely
 *  fashion.  Also, we always have to launch, even if an error occurred, to
 *  avoid a segfault and show at least a minimal message.
 *
 * NSM:
 *
 *  The NSM API requires that applications MUST NOT register their JACK client
 *  until receiving an NSM "open" message.  So, in the main() function of the
 *  application, we make the rtlmanager calls in this order:
 *
 *      -#  settings().  Gets the normal Seq66 configuration items from
 *          the "rc", "usr", "mutes", "ctrl", and "playlist" files.
 *      -#  create_session().  Sets up the session and does the NSM "announce"
 *          handshake protocol. We ignore the return code so that Seq66 can
 *          run even if NSM is not available.  This call also receives the
 *          "open" response, which provides the NSM path, display name, and
 *          the client ID.
 *      -#  create_project().  This function firsts makes sure that the client
 *          directory [$HOME/NSM Sessions/Session/rtl66.nXYZZ] exists.
 *          It then gets the session configuration.  Do we want to
 *          reload a complete set of configuration files from this directory,
 *          or just a session-specific subset from an "nsm" file ???
 *          Most of this work is in the non-GUI-specific clinrtlmanager
 *          derived class.
 *      -#  create_player().  This sets up the ports and launches the
 *          threads.
 *      -#  open_playist().  If applicable.  Probably better as part of the
 *          session.
 *      -#  open_midi_file().  If applicable.  Probably better as part of the
 *          session.
 *      -#  create_window().  This creates the main window, and the Sessions
 *          tab can be hidden if not needed.  Menu entries can also be
 *          adjusted for session support; see qsmainwnd.
 *      -#  run().  The program runs until the user or NSM kills it.  This is
 *          called from the main() function of the application.
 *      -#  close_session().  Should tell NSM that it is bowing out of the
 *          session.  Done normally in main(), but here if a serious error
 *          occurs.
 *
 * \return
 *      Returns false if the player wasn't able to be created and launched.
 *      Other failures, such as not getting good settings, might be ignored.
 */

bool
rtlmanager::create_player ()
{
    pointer p(new (std::nothrow) midi::player(/* int portnum, bool isoutput*/));
    bool result = bool(p);
    if (result)
    {
//      (void) p->get_settings(rc(), usr());
        m_player_ptr = std::move(p);              /* change the ownership */
        result = player_ptr()->launch();
        if (! result)
            append_error_message("player launch failed");
    }
    else
        append_error_message("player creation failed");

    return result;
}

/**
 *  Encapsulates opening the MIDI file, if specified (on the command-line).
 *
 * \return
 *      Returns the name of the MIDI file, if successful.  Otherwise, an empty
 *      string is returned.
 */

std::string
rtlmanager::open_midi_file (const std::string & fname)
{
    std::string midifname;                          /* start out blank      */
    std::string errmsg;
    bool result = player_ptr()->read_midi_file(fname, errmsg);
    if (result)
    {
        std::string infomsg = "PPQN set to ";
        infomsg += std::to_string(player_ptr()->get_ppqn());
        (void) util::info_message(infomsg);
        midifname = fname;
    }
    else
        append_error_message(errmsg); // errmsg = "Open failed: '" + fname + "'";

    return midifname;
}

/**
 *  The clinrtlmanager::create_session() function supercedes this one, but calls
 *  it.
 */

bool
rtlmanager::create_session (int /*argc*/, char * /*argv*/ [])
{
    xpc::session_setup();        /* daemonize: set basic signal handlers */
    return true;
}

/**
 *  Closes the session, with an option to handle errors in the session.
 *
 *  Note that we do not save if in a session, as we rely on the session
 *  rtlmanager to tell this application to save before forcing this application
 *  to end.
 *
 * \param [out] msg
 *      Provides a place to store any error message for the caller to use.
 *
 * \param ok
 *      Indicates if an error occurred, or not.  The default is true, which
 *      indicates "no problem".
 *
 * \return
 *      Returns the ok parameter if false, otherwise, the result of finishing
 *      up is returned.
 */

bool
rtlmanager::close_session (std::string & msg, bool ok)
{
    bool result = not_nullptr(player_ptr());
    if (result)
    {
        result = player_ptr()->finish();             /* tear down player       */
        if (result)
            (void) save_session(msg, result);
    }
    result = ok;
    (void) xpc::session_close();               /* daemonize signals exit   */
    return result;
}

/**
 *  This function saves the following files (so far):
 *
 *      -   *.rc
 *      -   *.ctrl (via the 'rc' file)
 *      -   *.mutes (via the 'rc' file)
 *      -   *.usr
 *      -   *.drums
 *
 *  The clinrtlmanager::save_session() function saves the MIDI file to the
 *  session, if applicable.  That function also clears the message parameter
 *  before the saving starts.
 *
 * \param [out] msg
 *      Provides a place to store any error message for the caller to use.
 *
 * \param ok
 *      Indicates if an error occurred, or not.  The default is true, which
 *      indicates "no problem".
 */

bool
rtlmanager::save_session (std::string & msg, bool ok)
{
    bool result = not_nullptr(player_ptr());
    if (result)
    {
        if (ok)                     /* code moved from clinrtlmanager to here */
        {
            if (player_ptr()->modified())
            {
                std::string filename = midi_filename();
                if (filename.empty())
                {
                    /* Don't need this: msg = "MIDI file-name empty"; */
                }
                else
                {
//                  bool is_wrk = util::file_extension_match(filename, ".wrk");
//                  if (is_wrk)
//                      filename = util::file_extension_set(filename, ".midi");

                    result = midi::write_midi_file(*player_ptr(), filename, msg);
                    if (result)
                        msg = result ? "Saved: " : "Not able to save: " ;

                    msg += filename;
                }
            }
        }
        if (result && ok)
        {
            std::string errmessage;
            bool save = not_nullptr(config_ptr()) ?
                config_ptr()->modified() : false ;

            if (save)
            {
                util::file_message("Save session", "Options");
                if (! write_option_file(errmessage))
                {
                    msg = "Config write failed: ";
                    msg += errmessage;
                }
            }
#if defined MOVE_TO_DERIVED_CLASS
            if (rc().auto_ctrl_save())
            {
                std::string mcfname = rc().midi_control_filespec();
                util::file_message("Save session", "Controls");
                result = write_midi_control_file(mcfname, rc());
            }
            if (rc().auto_mutes_save())
            {
                util::file_message("Save session", "Mutes");
                result = player_ptr()->save_mutegroups();         // add msg return?
            }
            if (rc().auto_playlist_save())
            {
                util::file_message("Save session", "Playlist");
                result = player_ptr()->save_playlist();           // add msg return?
            }
            if (rc().auto_drums_save())
            {
                util::file_message("Save session", "Notemapper");
                result = player_ptr()->save_note_mapper();        // add msg return?
            }
#endif  // defined MOVE_TO_DERIVED_CLASS
        }
        else
        {
            result = false;
            if (! is_help())
            {
                std::string errmessage;
                if (not_nullptr(config_ptr()))
                {
//                  config_ptr()->config_filename("erroneous");
                    (void) write_option_file(errmessage);
                }
                if (error_active())
                {
                    append_error_message(error_message());
                    msg = error_message();
                }
            }
        }
    }
    else
    {
        msg = "no player!";
    }
    return result;
}

/*
 * Having this here after creating the main window may cause issue
 * #100, where ladish doesn't see rtl66's ports in time.
 *
 *      player_ptr()->launch(usr().midi_ppqn());
 *
 * We also check for any "fatal" PortMidi errors, so we can display
 * them.  But we still want to keep going, in order to at least
 * generate the log-files and configuration files to
 * C:/Users/me/AppData/Local/rtl66 or ~/.config/rtl66.
 */

void
rtlmanager::show_message (const std::string & tag, const std::string & msg) const
{
    std::string fullmsg = tag + ": " + msg;
    util::info_message(fullmsg);       /* checks for "debug" and adds "[]" */
}

void
rtlmanager::show_error (const std::string & tag, const std::string & msg) const
{
    std::string fullmsg = tag + ": " + msg;
    util::error_message(msg);
}

/**
 *  Refactored so that the basic NSM session can be set up before launch(), as
 *  per NSM rules.
 *
 *  The following call detects a session, creates an nsmclient, sends an NSM
 *  announce message, waits for the response, uses it to set the session
 *  information.  What we really see:
 *
 *      nsmclient::announce()       Send announcement, wait for response
 *      <below>                     Gets rtlmanager path!!!
 *      nsmclient::open()           Sets rtlmanager path
 *
 *  We run() the window, get the exit status, and close the session in the
 *  main() function of the application.
 *
 *  Call sequence summary:
 *
 *      -   settings()
 *      -   create_session()
 *      -   create_project()
 *      -   create_player()
 *      -   open_midi_file() if specified on command-line; otherwise
 *      -   Open most-recent file if that option is enabled:
 *          Get full path to the most recently-opened or imported file.  What if
 *          rtlmanager::open_midi_file() has already been called via the
 *          command-line? Then skip this step.
 *      -   create_window()
 *      -   run(), done in main()
 *      -   close_session(), done in main()
 */

bool
rtlmanager::create_manager (int argc, char * argv [])
{
    bool result = settings(argc, argv);
    if (result)
    {
        bool ok = create_session(argc, argv);   /* get path, client ID, etc */
        if (ok)
        {
            std::string homedir = manager_path();
            if (homedir.empty())
                homedir = c_home_directory;

            util::file_message("Session rtlmanager path", homedir);
            (void) create_project(argc, argv, homedir);
        }
        result = create_player();        /* fails if player not made  */
        if (result)
        {
            std::string fname = midi_filename();
            if (fname.empty())
            {
                if (result && load_midi_file())
                {
                    std::string midifname = midi_filename();
                    if (! midifname.empty())
                    {
                        std::string errmsg;
                        std::string tmp = open_midi_file(midifname);
                        if (! tmp.empty())
                        {
                            util::file_message("Opened", tmp);
                            midi_filename(midifname);
                        }
                    }
                }
            }
            else
            {
                /*
                 * No window at this time; should save the message for later.
                 * For now, write to the console.
                 */

                std::string errormessage;
                std::string tmp = open_midi_file(fname);
                if (! tmp.empty())
                {
                    util::file_message("Opened", tmp);
                    midi_filename(fname);
                }
            }
        }
        if (result)
        {
            result = create_window();
            if (result)
            {
                error_handling();
            }
            else
            {
                std::string msg;                        /* maybe errmsg? */
                result = close_session(msg, false);
            }

            /*
             * TODO:  expose the error message to the user here
             */
        }
//      if (! is_help())
//          cmdlineopts::show_locale();
    }
    else
    {
        if (! is_help())
        {
            std::string msg;
            (void) create_player();
            (void) create_window();
            error_handling();
            (void) create_session();
            (void) run();
            (void) close_session(msg, false);
        }
    }
    return result;
}

bool
rtlmanager::create_midi_configuration
(
    int argc, char * argv [],
    const std::string & mainpath,
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    bool result = ! cfgfilepath.empty();
    if (result)
    {
        std::string rcbase = config_filename();
        std::string rcfile = util::filename_concatenate(cfgfilepath, rcbase);
        bool already_created = util::file_exists(rcfile);
        midi_filename(midifilepath);                    /* do this first    */
        if (already_created)
        {
            util::file_message("File exists", rcfile);  /* comforting       */
            result = read_midi_configuration(argc, argv, cfgfilepath, midifilepath);
            if (result)
            {
#if defined MOVE_TO_DERIVED
                if (usr().in_nsm_session())
                {
                    rc().auto_rc_save(true);
                }
                else
                {
                    bool u = rc().auto_usr_save();      /* --user-save?     */
                    rc().set_save_list(false);          /* save them all    */
                    rc().auto_usr_save(u);              /* restore it       */
                }
#endif
            }
        }
        else
        {
            result = util::make_directory_path(mainpath);
            if (result)
            {
                util::file_message("Ready", mainpath);
                result = util::make_directory_path(cfgfilepath);
                if (result)
                {
                    util::file_message("Ready", cfgfilepath);
//                  rc().home_config_path(cfgfilepath);
                }
            }
            if (result && ! midifilepath.empty())
            {
                result = util::make_directory_path(midifilepath);
                if (result)
                    util::file_message("Ready", midifilepath);
            }
//          rc().set_save_list(true);                   /* save all configs */
#if defined USE+NSM
            if (usr().in_nsm_session())
            {
                usr().session_visibility(false);        /* new session=hide */

#if defined NSM_DISABLE_LOAD_MOST_RECENT
                rc().load_most_recent(false;            /* don't load MIDI  */
#else
                rc().load_most_recent(true);            /* issue #41        */
#endif
            }
#endif
#if defined DO_NOT_BELAY_UNTIL_EXIT
            if (result)
            {
                util::file_message("Saving session configuration", cfgfilepath);
                result = cmdlineopts::write_options_files();
            }
#endif
        }
    }
    return result;
}

bool
rtlmanager::read_midi_configuration
(
    int argc, char * argv [],
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    (void) cfgfilepath;
//  config_ptr()->home_config_path(cfgfilepath);    /* set NSM dir      */
    config_ptr()->midi_filepath(midifilepath);           /* set MIDI dir     */
    if (! midifilepath.empty())
    {
        util::file_message("MIDI path", config_ptr()->midi_filepath());
        util::file_message("MIDI file", config_ptr()->midi_filename());
    }

    std::string errmessage;
    bool result = parse_option_file(errmessage);
    if (result)
    {
        /*
         * Perhaps at some point, the "rc"/"usr" options might affect NSM
         * usage.  In the meantime, we still need command-line options, if
         * present, to override the file-specified options.  One big example
         * is the --buss override. The rtlmanager::settings() function is
         * called way before create_project();
         */

        if (argc > 1)
        {
            int rcode = parse_command_line(argc, argv, errmessage);
            result = rcode != (-1);
            if (! result)               // DONE above parse_o_options(argc, argv);
                is_help(true);          /* a hack to avoid create_window()  */
        }
    }
    else
    {
        // TODO: add full-path getters to configuration and rtlconfiguration.
//      util::file_error(errmessage, rc().config_filespec());
    }
    return result;
}

bool
rtlmanager::make_path_names
(
    const std::string & path,
    std::string & outcfgpath,
    std::string & outmidipath,
    const std::string & midisubdir
)
{
    bool result = ! path.empty();
    if (result)
    {
        std::string cfgpath = path;
        std::string midipath = path;
        std::string subdir = "midi";
        if (! midisubdir.empty())
            subdir = midisubdir;

        // FIXME
        if (result) // TODO:  in session call? in_nsm_session()) // nsm_active()
        {
        // FIXME
            midipath = util::pathname_concatenate(cfgpath, subdir);
            cfgpath = util::pathname_concatenate(cfgpath, "config");
        }
        else
        {
        // FIXME
            /*
             * There's no "config" subdirectory outside of an NSM session.
             */

            midipath = util::pathname_concatenate(midipath, subdir);
        }
        outcfgpath = cfgpath;
        outmidipath = midipath;
    }
    return result;
}

/**
 *  Function for the main window to call.
 *
 *  When this function succeeds, we need to signal a session-reload and the
 *  following settings:
 *
 *  rc().load_most_recent(false);               // don't load MIDI  //
 *  rc().set_save_list(true);                   // save all configs //
 *
 *  We don't really need to care that NSM is active here
 */

bool
rtlmanager::import_into_session
(
    const std::string & sourcepath,
    const std::string & sourcebase              /* e.g. qrrtl66.rc */
)
{
    bool result = ! sourcepath.empty() && ! sourcebase.empty();
    if (result)
    {
#if defined THIS_CODE_IS_READY
        std::string destdir = rc().home_config_directory();
        std::string destbase = rc().config_filename();
        std::string cfgpath;
        std::string midipath;
        result = make_path_names(destdir, cfgpath, midipath);
        if (result)
            result = delete_configuration(cfgpath, destbase);

        if (result)
            result = copy_configuration(sourcepath, sourcebase, cfgpath);

        if (result)
        {
            result = import_configuration
            (
                sourcepath, sourcebase, cfgpath, midipath
            );
        }
#endif  // defined THIS_CODE_IS_READY
    }
    return result;
}

/**
 *  This function is like create_configuration(), but the source is not the
 *  standard configuration-file directory, but a directory chosen by the user.
 *  This function should be called only for importing a configuration into an
 *  NSM session directory.
 *
 * \param sourcepath
 *      The source path is the path to the configuration files chosen for
 *      import by the user.  Often it is the usual ~/.config/rtl66 directory.
 *
 * \param sourcebase
 *      This is the actual configuration file chosen by the user.  It can be
 *      any file qrtl66.*, only the first part of the name (e.g. "qrtl66") is
 *      used.
 *
 * \param cfgfilepath
 *      This is the configuration directory used in this NSM session.  It is
 *      often something like "~/NSM Sessions/MySession/rtl66.nLTQC/config.
 *      This is the destination for the imported configuration files.
 *
 * \param midifilepath
 *      This is the MIDI directory used in this NSM session.  It is often
 *      something like "~/NSM Sessions/MySession/rtl66.nLTQC/midi.  This is
 *      the destination for the imported MIDI files (due to being part of an
 *      imported play-list.
 *
 * \return
 *      Returns true if the import succeeded.
 */

bool
rtlmanager::import_configuration
(
    const std::string & sourcepath,
    const std::string & sourcebase,
    const std::string & /* cfgfilepath */,
    const std::string & /* midifilepath */
)
{
    bool result = ! sourcepath.empty() && ! sourcebase.empty();
    if (result)
    {
#if defined THIS_CODE_IS_READY
        std::string rcbase = util::file_extension_set(sourcebase, ".rc");
        std::string rcfile = util::filename_concatenate(sourcepath, rcbase);
        result = util::file_exists(rcfile);             /* a valid source   */
        if (result)
        {
            config_filename(rcfile);

            std::string errmessage;
            result = parse_option_file(rcfile, errmessage);
            if (result)
            {
#if defined NSM_DISABLE_LOAD_MOST_RECENT
                if (usr().in_nsm_session())
                    rc().load_most_recent(false);       /* don't load MIDI  */
#else
//              if (usr().in_nsm_session())
//                  rc().load_most_recent(true);        /* issue #41        */
#endif
            }
        }
        if (result)
        {
            util::file_message("Saving imported configuration", cfgfilepath);
            result = write_option_file();
            if (not_nullptr(config_ptr()))
                config_ptr()->modified(true);
        }
#endif  // defined THIS_CODE_IS_READY
    }
    return result;
}

}           // namespace session

/*
 * rtlmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

