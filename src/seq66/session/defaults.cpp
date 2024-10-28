/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          defaults.cpp
 *
 *  This module declares/defines the values for all options.
 *
 * \library       defaults
 * \author        Chris Ahlstrom
 * \date          2023-02-20
 * \updates       2023-03-05
 * \license       GNU GPLv2 or above
 *
 */

namespace seq66
{

/**
 * Items not part of the app-wde configuration:
 *
 *      bpm-maximum     Merged into min-def-max (option_default) field of
 *      bpm-minimum     the beats-per-minute option.
 *      config-type
 *      count           Renamed to "recent-count". Should be read only.
 *      verbose         Move to 'session' file.
 *      version
 *
 * In session file:
 *
 *      session
 *      url
 *      visibility
 *
 * Change opt_enabled to opt_readonly for read-only???
 */

static cfg::options::list s_default_options
{
    /*
     *   Name, Code, Kind, Enabled,
     *   Default, Value, FromCli, Dirty,
     *   Description
     */
    {
        "armed", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "A new pattern is automatically armed."
    },
    {
        "auto-save-rc", "", "boolean", opt_enabled,        // "_cfg"
        "false", "false", false, false,
        "Option files are saved automatically at exit."
    },
    {
        "backseq", "", "string", opt_enabled,
        "dense2", "dense2", false, false,
        "Specifies the Qt brush used for background sequences."
    },
    {
        "base-directory", "", "string", opt_enabled,
        "", "", false, false,
        "The base directory for all playlist files."
    },
    {
        "beat-width", "", "integer", opt_enabled,
        "4", "4", false, false,
        "The denominator of the default time signature."
    },
    {
        "beat-width-metro", "", "integer", opt_enabled,
        "4", "4", false, false,
        "The denominator of the metronome time signature."
    },
    {
        "beats-per-bar", "", "integer", opt_enabled,
        "4", "4", false, false,
        "The beat count in the default time signature."
    },
    {
        "beats-per-bar-metro", "", "integer", opt_enabled,
        "4", "4", false, false,
        "The default beats for the metronome."
    },
    {
        "beats-per-minute", "", "floating", opt_enabled,
        "2.0<120.0<600.0", "120.0", false, false,
        "The default beat-rate of the song."
    },
    {
        "bpm-page-increment", "", "floating", opt_enabled,
        "10.0<10.0<50.0", "10.0", false, false,
        "The large increment/decrement of the BPM."
    },
    {
        "bpm-precision", "", "integer", opt_enabled,
        "0<0<2", "0", false, false,
        "The number of digits in the BPM (0, 1, or 2)."
    },
    {
        "bpm-step-increment", "", "floating", opt_enabled,
        "0.01<1.0<50.0", "1.0", false, false,
        "The small increment/decrement of the BPM."
    },
    {
        "buss-override", "", "integer", opt_enabled,
        "0<-1<254", "-1", false, false,
        "The value to override the bus specified in all tracks."
    },
    {
        "button-ctrl-columns-out", "", "integer", opt_enabled,  // button-columns
        "4<8<12", "12", false, false,
        "The number of columns in a screen-set."
    },
    {
        "button-ctrl-columns-in", "", "integer", opt_enabled,   // button-columns
        "4<8<12", "12", false, false,
        "The number of columns for MIDI control."
    },
    {
        "button-ctrl-offset-in", "", "integer", opt_obsolete,
        "0<0<22", "0", false, false,
        "Provides a way to offset MIDI control items."
    },
    {
        "button-ctrl-offset-out", "", "integer", opt_obsolete,
        "0<0<22", "0", false, false,
        "Provides a way to offset MIDI control items."
    },
    {
        "button-ctrl-rows-in", "", "integer", opt_enabled,      // button-rows
        "4<4<12", "4", false, false,
        "The number of rows for MIDI control."
    },
    {
        "button-ctrl-rows-out", "", "integer", opt_enabled,     // button-rows
        "4<4<12", "4", false, false,
        "The number of rows in a screen-set."
    },
    {
        "control-buss", "", "integer", opt_enabled,
        "0<0<254", "-1", false, false,
        "The output buss used for MIDI control."
    },
    {
        "convert-to-smf-1", "", "boolean", opt_enabled,
        "true", "true", false, false,
        "If true, automatically convert SMF 0 files to SMF 1."
    },
    {
        "recent-count", "", "integer", opt_readonly,
        "0", "0", false, false,
        "Holds the current number of recent-file entries."
    },
    {
        "daemonize", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Fork the CLI version of application as a daemon."
    },
    {
        "dark-theme", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Indicates that the desktop is using a dark theme."
    },
    {
        "deep-verify", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "If true, load all songs in the playlist to verify correctness."
    },
    {
        "default-ppqn", "", "integer", opt_enabled,
        "32<192<19200", "192", false, false,
        "The PPQN value used if not read from a MIDI file."
    },
    {
        "default-zoom", "", "integer", opt_enabled,
        "1<2<512", "2", false, false,
        "The default or initial zoom of the piano rolls."
    },
    {
        "double-click-edit", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Allows a double-click on a grid button to open the pattern editor."
    },
    {
        "drop-empty-controls", "", "boolean", opt_obsolete,
        "false", "false", false, false,
        "Do not add empty MIDI controls to the control map."
    },
    {
        "empty", "", "string", opt_enabled,
        "nobrush", "nobrush", false, false,
        "Specifies the Qt brush for empty space."
    },
    {
        "fingerprint-size", "", "integer", opt_enabled,
        "0<32<128", "32", false, false,
        "The number of notes to show in progress box; 0 means show all."
    },
    {
        "footer", "", "string", opt_enabled,
        "0xF7", "0XF7", false, false,
        "Provides the byte sequence that ends a MIDI macro."
    },
    {
        "full-paths", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Shows the full-paths of recent files in the menu."
    },
    {
        "global-seq-feature", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "If true, key/scale/background-seq apply to all patterns."
    },
    {
        "groups-format", "", "string", opt_enabled,
        "binary", "binary", false, false,
        "Sets the format of mute-group stanzas to binary or hex."
    },
    {
        "header", "", "string", opt_enabled,
        "0xF0", "0XF0", false, false,
        "Provides the byte sequence that starts a MIDI macro."
    },
    {
        "init-disabled-ports", "", "boolean", opt_disabled,
        "false", "false", false, false,
        "An option that does not work."
    },
    {
        "input-port-count", "", "integer", opt_enabled,
        "1<4<48", "4", false, false,
        "The number of virtual input ports to create."
    },
    {
        "inverse-colors", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Use the inverse color palette for the application."
    },
    {
        "jack-auto-connect", "", "boolean", opt_enabled,
        "true", "true", false, false,
        "Application connects to existing JACK ports, vs via a session manager."
    },
    {
        "jack-midi", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Use JACK for MIDI, even if ALSA is available."
    },
    {
        "key-height", "", "integer", opt_enabled,
        "6<10<32", "10", false, false,
        "Specifies the initialize vertical width of the piano keys."
    },
    {
        "key-view", "", "string", opt_enabled,
        "octave-letters", "octave-letters", false, false,
        "Specifies how to show the note labels of the piano keys."
    },
    {
        "keyboard-layout", "", "string", opt_enabled,
        "qwerty", "qwerty", false, false,
        "Specifies the keyboard layout, to some extent."
    },
    {
        "load-most-recent", "", "boolean", opt_enabled,
        "true", "true", false, false,
        "Allows the most recent file to be reloaded at startup."
    },
    {
        "load-mute-groups", "", "string", opt_enabled,
        "both", "both", false, false,
        "Indicates to load mute groups, and from song or 'mutes' file."
    },
    {
        "lock-main-window", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Prevents the resizing of the main window."
    },
    {
        "log", "", "string", opt_enabled,
        "", "", false, false,
        "Override the log file specified by the session file."
    },
    {
        "main-note", "", "integer", opt_enabled,
        "0<75<127", "75", false, false,
        "The main note number to use for the metronome."
    },
    {
        "main-note-length", "", "floating", opt_enabled,       // ticks????
        "0.125<0.125<2.0", "0.125", false, false,
        "The metronome main note length relative to the beat."
    },
    {
        "main-note-velocity", "", "integer", opt_enabled
        "0<120<127", "120", false, false,
        "The metronome main note velocity."
    },
    {
        "main-patch-metro", "", "integer", opt_enabled,
        "0<15<127", "15", false, false,
        "The MIDI program/patch to use for the main note of the metronome."
    },
    {
        "mainwnd-columns", "", "integer", opt_enabled,
        "4<8<12", "8", false, false,
        "Number of columns in the Live grid."
    },
    {
        "mainwnd-rows", "", "integer", opt_enabled,
        "4<4<8", "4", false, false,
        "Number of rows in the Live grid."
    },
    {
        "mainwnd-spacing", "", "integer", opt_enabled,
        "0<2<16", "2", false, false,
        "Number of pixels between buttons in the Live grid."
    },
    {
        "midi-ctrl-in", "", "boolean", opt_enabled,            // "midi-enabled"
        "false", "false", false, false,
        "Enables using MIDI control of the application."
    },
    {
        "midi-ctrl-out", "", "boolean", opt_enabled,           // "midi-enabled"
        "false", "false", false, false,
        "Enables using MIDI to display status of the application."
    },
    {
        "mute-group-columns", "", "integer", opt_enabled,       // make a pair??
        "4<4<8", "4", false, false,
        "Number of columns in a mute-group."
    },
    {
        "mute-group-count", "", "integer", opt_readonly,
        "32<32<32", "32", false, false,
        "The number of mute groups, constant at 4 x 8."
    },
    {
        "mute-group-rows", "", "integer", opt_enabled,          // make a pair??
        "4<8<12", "8", false, false,
        "Number of columns in a mute-group."
    },
    {
        "mute-group-selected", "", "integer", opt_enabled,
        "0<-1<31", "-1", false, false,
        "The mute-group to apply at startup/file-load, if any."
    },
    {
        "note", "", "string", opt_enabled,          // TODO: implement this!!!
        "nobrush", "nobrush", false, false,         // "lineargradient" default
        "Specifies the Qt brush for empty space."
    },
    {
        "note-resume", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Allows notes-in-progress to be resumed when play is toggled."
    },
    {
        "output-buss-metro", "", "integer", opt_enabled,
        "0<15<15", "15", false, false,
        "Sets the output buss for the metronome."
    },
    {
        "output-buss", "", "integer", opt_enabled,
        "0<15<15", "-1", false, false,
        "Sets the output buss for displaying MIDI status on a device."
    },
    {
        "output-channel-metro"
        "0<9<15", "9", false, false,
        "Sets the output channel for the metronome."
    },
    {
        "output-port-count", "", "integer", opt_enabled,
        "1<4<48", "4", false, false,
        "The number of virtual output ports to create."
    },
    {
        "port-naming", "", "string", opt_enabled,   // -----> add [port-options]
        "short", "short", false, false,
        "Determines how much detail is provided in port names."
    },
    {
        "progress-bar-thick", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Make the progress bar/box-border thick, use bold slot font."
    },
    {
        "progress-box-height", "", "floating", opt_enabled,
        "0.10<0.50<1.0", "0.50", false,false,
        "The scaled height of the grid button progress box; 0 disables it."
    },
    {
        "progress-box-width", "", "floating", opt_enabled,
        "0.10<0.50<1.0", "0.50", false,false,
        "The scaled width of the grid button progress box; 0 disables it."
    },
    {
        "progress-note-max", "", "integer", opt_enabled,
        "0<127<127", "127", false, false,
        "The top of the range of note values for the progress box."
    },
    {
        "progress-note-min", "", "integer", opt_enabled,
        "0<127<127", "0", false, false,
        "The bottom of the range of note values for the progress box."
    },
    {
        "qrecord", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "A new pattern is set to quantize-record immediately."
    },
    {
        "record", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "A new pattern is set to record immediately."
    },
    {
        "record-by-channel", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "When recording, send each event to the patterns by channel."
    },
    {
        "record-style", "", "string", opt_enabled,
        "merge", "merge", false, false,
        "A new pattern is set for the given style of recordng."
    },
    {
        "reset", "", "string", opt_enabled,
        "$header 0x00 $footer", "$header 0x00 $footer", false, false,
        "Provides a byte sequence that resets some MIDI controllers."
    },
    {
        "save-mutes-to", "", "string", opt_enabled,
        "both", "both", false, false,
        "Indicates to save mute groups, and to song or 'mutes' file."
    },
    {
        "save-old-mutes", "", "boolean", opt_obsolete,
        "false", "false", false, false,
        "Save mute-groups in Seq24 format."
    },
    {
        "save-old-triggers", "", "boolean", opt_obsolete,
        "false", "false", false, false,
        "Save song triggers in Seq24 format."
    },
    {
        "scale", "", "string", opt_enabled,         // TODO: implement this!!!
        "nobrush", "nobrush", false, false,         // "lineargradient" default
        "Specifies the Qt brush for scales shown in the pattern editor."
    },
    {
        "ctrl-set-size", "", "integer", opt_readonly,   // set-size
        "16<32<96", "32", false, false,
        "The size of a screen-set."
    },
    {
        "sets-mode", "", "string", opt_disabled, // move to [mute-group-flags]
        "normal", "normal", false, false,
        "Indicates how moving to another set is handled."
    },
    {
        "show-system-ports", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "If true, ignore any instrument names defined in the 'usr' file."
    },
    {
        "shutdown", "", "string", opt_enabled,
        "$header 0x00 $footer", "$header 0x00 $footer", false, false,
        "Provides the default byte sequence sent at application shutdown."
    },
    {
        "snap-split", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Allows splitting song-editor triggers at the nearest snap point."
    },
    {
        "song-start-mode", "", "string", opt_enabled,
        "auto", "auto", false, false,
        "Indicates if song mode is live, song, or automatic."
    },
    {
        "startup", "", "string", opt_enabled,
        "$header 0x00 $footer", "$header 0x00 $footer", false, false,
        "Provides the default byte sequence sent at application start-up."
    },
    {
        "strip-empty", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Indicates to strip empty mute-groups from the file."
    },
    {
        "style-sheet", "", "string", opt_enabled,
        "", "", false, false,
        "Provides the name of a Qt style sheet to apply."
    },
    {
        "style-sheet-active", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Indicates if the style sheet should be applied."
    },
    {
        "sub-note", "", "integer", opt_enabled
        "0<76<127", "76", false, false,
        "The sub note number to use for the metronome."
    },
    {
        "sub-note-length", "", "floating", opt_enabled     // ticks????
        "0.125<0.125<2.0", "0.125", false, false,
        "The metronome sub note length relative to the beat."
    },
    {
        "sub-note-velocity", "", "integer", opt_enabled
        "0<84<127", "84", false, false,
        "The metronome sub note velocity."
    },
    {
        "sub-patch-metro"
        "0<76<127", "76", false, false,
        "The MIDI program/patch to use for the sub note of the metronome."
    },
    {
        "swap-coordinates", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "If true, swap rows and columns in the grid(s)."
    },
    {
        "tempo-track", "", "integer", opt_enabled,
        "0<0<32", "0", false, false,
        "Indicates an alternate tempo track number."
    },
    {
        "thru", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "A new pattern is set to use MIDI Thru."
    },
    {
        "mod-ticks", "", "integer", opt_enabled,
        "1<64<256", "64", false, false,
        "The song position (16th notes) at which clocking can begin."
    },
    {
        "toggle-active-only", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Toggle only the patterns specified in the mute-group."
    },
    {
        "transport-type", "", "string", opt_enabled,
        "none", "none", false, false,
        "Indicates the type of JACK transport to use."
    },
    {
        "unmute-new-song", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Indicates to unmute the next song picked in the playlist."
    },
    {
        "use-file-ppqn", "", "boolean", opt_enabled,
        "true", "true", false, false,
        "Use the file's PPQN instead of scaling to the app's PPQN."
    },
    {
        "velocity-override", "", "integer", opt_enabled,
        "-1<-1<127", "-1", false, false,
        "If set, the velocity at which all notes are recorded."
    },
    {
        "virtual-ports", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Indicates to use manual (virtual) ports."
    },
    {
        "window-redraw-rate", "", "integer", opt_enabled,
#if defined PLATFORM_WINDOWS
        "10<25<100", "25", false, false,
#else
        "10<40<100", "25", false, false,
#endif
        "The base window refresh intervale in milliseconds."
    },
    {
        "window-scale", "", "floating", opt_enabled,
        "0.5<1.0<3.0", "1.0", false, false,
        "Horizontal scaling of the main window."
    },
    {
        "window-scale-y", "", "floating", opt_enabled,
        "0.5<1.0<3.0", "1.0", false, false,
        "Vertical scaling of the main window."
    },
    {
        "wrap-around", "", "boolean", opt_enabled,
        "false", "false", false, false,
        "Recorded notes are allowed to wrap around to the pattern beginning."
    }
}           // namespace seq66

/**
 *  Above we have defined all of the options possible in this application.
 *
 *      -   First we define a set of INI sections (inisection objects) which
 *          together cover all of the options above.
 *      -   Nwxt we define a set of INI files (inifile objects) which each
 *          hold a list of the INI sections it covers.
 *      -   We can also define an inifiles object that contains a list of
 *          the inifile objects.
 *
 *  Each of these items are given a name via a constructor.
 *
 *  Option sections.
 */

/**
 *  [Cfg66.rc] sections.
 *
 *      config-type = 'rc'
 *      version = 0
 */

/*
 * TODO:  Consider CONSOLIDATING the SMALL SECTIONS.
 *
 * TODO:  Add the description text to each section.
 */

static cfg::inisection options s_rc_midi_meta_events
{
    "tempo-track"
};

static cfg::inisection options s_rc_manual_ports
{
    "input-port-count",
    "output-port-count",
    "virtual-ports"
};

static cfg::inisection options s_rc_midi_clock_mod_ticks
{
    "mod-ticks",
    "record-by-channel"
};

static cfg::inisection options s_rc_midi_reveal_ports
{
    "show-system-ports"
};

static cfg::inisection options s_rc_interaction_method
{
    "snap-split",
    "double-click-edit"
};

static cfg::inisection options s_rc_jack_transport
{
    "transport-type",
    "song-start-mode",
    "jack-midi = true",
    "jack-auto-connect"
};

static cfg::inisection options s_rc_
{
};

/**
 *  [Cfg66.metro] sections.
 *
 *      config-type = 'rc'
 *      version = 0
 */

static cfg::inisection options s_metro_metronome
{
    "output-buss-metro",
    "output-channel-metro",
    "beats-per-bar-metro",
    "beat-width-metro",
    "main-patch-metro",
    "main-note",
    "main-note-velocity",
    "main-note-length",
    "sub-patch-metro",
    "sub-note",
    "sub-note-velocity",
    "sub-note-length"
}

/*
 * defaults.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

