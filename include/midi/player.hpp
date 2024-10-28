#if ! defined RTL66_MIDI_PLAYER_HPP
#define RTL66_MIDI_PLAYER_HPP

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
 * \file          player.hpp
 *
 *  This module declares/defines a base class for handling some facets
 *  of playing or recording from a single MIDI port.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-10
 * \updates       2024-06-01
 * \license       GNU GPLv2 or above
 *
 *  The player class is a severely cut-down version of seq66::performer, with
 *  an additional, single, port parameter. It handles the following tasks:
 *
 *      -   Connecting (automatically if specified) to a port
 *      -   Reading a MIDI file
 *      -   Playing all the tracks in the file to the single port.
 *      -   Recording to a designated track via a single port.
 *
 *  It leaves out the following performer concepts:
 *
 *      -   Play-lists
 *      -   Mute-groups.
 *      -   Sets, set-master, and set-mapper..
 *      -   Change management (enum class change etc.).
 *      -   On-action callbacks (performer::callbacks nested class).
 *      -   Note-mapping.
 *      -   Metronome support.
 *      -   Song triggers, song-box select, looping, song recording.
 *      -   L/R markers.
 *      -   FF/RW.
 *      -   Cut/copy/paste support.
 *      -   Clocks-list, inputs-list, and port-mapping.
 *      -   MIDI Control/Display I/O.
 *      -   Automation functions.
 *      -   Queueing.
 *      -   Transposition.
 *      -   Resume note-ons.
 *      -   Tap BPM.
 *      -   Master buss (may add it later).
 *      -   Filtering by channel.
 *
 *  It will be a base class for the new version of performer.
 */

#include <functional>                       /* std::function<void(int)>     */
#include <memory>                           /* std::unique_ptr<>            */
#include <thread>                           /* std::thread                  */

#include "xpc/condition.hpp"                /* xpc::condition/synchronizer  */
#include "midi/masterbus.hpp"               /* access to all MIDI busses    */
#include "midi/ports.hpp"                   /* access to MIDI ports         */
#include "midi/tracklist.hpp"               /* provides a set of tracks     */
#include "rtl/iothread.hpp"                 /* rtl::iothread class          */
#include "transport/jack/scratchpad.hpp"    /* transport::jack::scratchpad  */
#include "transport/jack/transport.hpp"     /* transport::jack::transport   */
#include "transport/clock/info.hpp"         /* transport::clock::info       */

namespace midi
{

/**
 *  This class supports the limited performance mode.
 */

class player
{
    friend class file;
    friend class track;
    friend class trackdata;
    friend class tracklist;

#if defined RTL66_BUILD_JACK

    friend class transport::jack::transport;
    friend int jack_sync_callback
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );
    friend int jack_transport_callback (jack_nframes_t nframes, void * arg);
    friend void jack_shutdown (void * arg);
    friend void jack_timebase_callback
    (
        jack_transport_state_t state, jack_nframes_t nframes,
        jack_position_t * pos, int new_pos, void * arg
    );
    friend long get_current_jack_position (void * arg);

#endif  // RTL66_BUILD_JACK

public:

    /**
     *  A nest class to provide an implementation of the synchronizer
     *  class.
     */

    class synch : public xpc::synchronizer
    {
    private:

        player & m_perf;

    public:

        synch (player & p) : xpc::synchronizer (), m_perf (p)
        {
            // no code
        }

        synch () = delete;
        synch (const synch &) = delete;
        synch & operator =(const synch &) = delete;

        virtual bool predicate () const override
        {
            return m_perf.is_running() || m_perf.done();
        }
    };

public:

    /**
     *  Provides a function type that can be applied to each track number in a
     *  selection. Generally, the caller will bind a member function to use
     *  in operate_on_set(). The first parameter is a track number (obtained
     *  from the selection). The caller can bind additional placeholders or
     *  parameters, if desired. See the old seq64 perfroll module.
     */

#if defined USE_SONG_BOX_SELECT
    using seqoperation = std::function<void(int)>;
#endif

private:

    /**
     *  Holds the "manufacturer ID" for the current application. Usually it is
     *  one byte. For Seq66-related applications, it is "0x24 0x24 0x00" and
     *  is followed by a one-byte feature code (e.g. for "triggers"); the last
     *  byte is variable then.
     */

    midi::bytes m_manufacturer_id;

    /**
     *  Provides our MIDI buss.  We changed this item to a pointer so that we
     *  can delay the creation of this object until after all settings have
     *  been read.  Use a smart pointer! Seems like unique_ptr<> is best
     *  here.  See the master_bus() accessors below.
     */

    std::unique_ptr<masterbus> m_master_bus;

    /**
     *  Supports a single input port and a single output port. A port is used
     *  if the port number is greater than or equal to 0 and the port exists.
     */

    int m_in_portnumber;
    int m_out_portnumber;

    /**
     *  Contains all tracks that are (heh heh) in play.
     */

    tracklist m_track_list;

    /**
     *  Keeps track of created sequences, whether or not they are active.
     *  Used by the install_track() function.  Note that this value is not
     *  a suitable replacement for m_track_max, because there can be
     *  inactive sequences amidst the active sequences.
     */

    int m_track_count;

    /**
     *  Actually, this value could go up to 2047... 2048 is used
     *  to indicate that there is no background sequence.  See track::limit().
     */

    track::number m_track_max;

    /**
     *  Indicates the highest-number sequence.  This value starts as -1, to
     *  indicate no sequences loaded, and then contains the highest sequence
     *  number hitherto loaded, plus 1 so that it can be used as a for-loop
     *  limit similar to m_track_max.  It's maximum value should be
     *  m_track_max.
     *
     *  Currently meant only for limited context to try to squeeze a little
     *  extra speed out of playback.  There's no easy way to lower this value
     *  when the highest sequence is deleted, though.
     */

    track::number m_track_high;

    /**
     *  If true (the default is false, the events in a track are sorted
     *  when the track is installed.
     */

    bool m_sort_on_install;

    /**
     *  Indicates the format of this file, either SMF 0 or SMF 1.
     *  Note that Seq66 always converts files from SMF 0 to SMF 1,
     *  and saves them to default to SMF 1.  This setting, if set to 0,
     *  indicates that the song has been converted to SMF 0, for export only.
     */

    int m_smf_format;

private:                            /* key, midi, and op container section  */

    /**
     *  Provides information for managing threads.  Provides a "handle" to
     *  the input and output threads.
     */

    rtl::iothread m_out_thread;
    rtl::iothread m_in_thread;

    /**
     *  Indicates that playback is running.  However, this flag is conflated
     *  with some JACK support, and we have to supplement it with another
     *  flag, m_is_pattern_playing.
     */

    std::atomic<bool> m_is_running;

    /**
     *  Indicates that a pattern is playing.  It replaces rc_settings ::
     *  is_pattern_playing(), which is gone, since the player is now
     *  visible to all classes that care about it.
     */

    bool m_is_pattern_playing;

    /**
     *  Holds the current PPQN from a MIDI file that has been read.  It might
     *  be 0. Moved to transport::info.
     *
     *      midi::ppqn m_file_ppqn;
     */

    /**
     *  Holds the underrun value for possible display during very busy
     *  playback.
     */

    microsec m_delta_us;

    /**
     *  Holds a bunch of JACK transport settings. Also holds pulse-counting
     *  data used by other MIDI APIs.
     */

    transport::jack::scratchpad m_jack_pad;

    /**
     *  Let's try to save the last JACK pad structure tick for re-use with
     *  resume after pausing.
     */

    midi::pulse m_jack_tick;

    /**
     *  Support for pause, which does not reset the "last tick" when playback
     *  stops/starts.  All this member is used for is keeping the last tick
     *  from being reset.
     */

    bool m_dont_reset_ticks;

    /**
     *  A condition variable to protect playback.  It is signalled if playback
     *  has been started.  The output thread function waits on this variable
     *  until m_is_running and m_io_active are false.  This variable is also
     *  signalled in the player destructor.  This implementation is
     *  new for 0.98.0, and it avoids segfaults, exit-hangs, and high CPU
     *  usage in Windows that have occurred with other implmentations.
     */

    synch m_condition_var;

    /**
     *  We need to adjust the clock increment for the PPQN that is in force.
     *  Higher PPQN need a longer increment than 8 in order to get 24 clocks
     *  per quarter note.
     */

    transport::clock::info m_clock_info;

    /**
     *  Consolidates a number of ALSA/JACK/etc. transport parameters. It
     *  includes settings and live values.  The accessor is transportinfo().
     *  Also see the rtl::midi_clock_info member.  If set, then transport
     *  will be initialized. It will either be one of the JACK options,
     *  or midiclock.
     */

    transport::info m_transport_info;

#if defined RTL66_BUILD_JACK

    /**
     *  A wrapper object for the JACK support of this application.  It
     *  implements most of the JACK stuff.  Not used on Windows (we use
     *  PortMidi instead).
     */

    transport::jack::transport m_jack_transport;

#endif

    /**
     *  Indicates that an internal setup error occurred (e.g. a device could
     *  not be set up in PortMidi).  In this case, we will eventually want to
     *  emit an error prompt, though we keep going in order to populate the
     *  "rc" file correctly.
     */

    mutable bool m_error_pending;

    /**
     *  Holds the accumulated error messages.
     */

    mutable std::string m_error_messages;

    /**
     *  It may be a good idea to eventually centralize all of the dirtiness of
     *  a performance here.  All the GUIs use a player.
     */

    bool m_modified;

    /**
     *  A stronger "modified" flag. It indicates that some user-interface
     *  update would be needed.
     */

    bool m_needs_update;

public:

    player
    (
        int out_portnumber = (-1),
        int in_portnumber  = (-1)
    );
    player (const player &) = delete;
    player (player &&) = delete;                    /* forced by iothread   */
    player & operator = (const player &) = delete;
    player & operator = (player &&) = delete;       /* ditto */
    virtual ~player ();

    virtual bool create_master_bus ();
    virtual bool clear_all (bool clearplaylist = false);
    virtual bool track_playing_toggle (track::number trkno);
    virtual bool track_playing_change (track::number trkno, bool on);

    virtual bool song_mode () const
    {
        return false;                   /* must derive to override  */
    }

    virtual bool resume_note_ons () const
    {
        return false;                   /* must derive to override  */
    }

    tracklist & track_list ()
    {
        return m_track_list;
    }

    const tracklist & track_list () const
    {
        return m_track_list;
    }

    void clear ()
    {
        m_track_count = 0;
        m_track_high = track::unassigned();

        /*
         * There is more to do!
         */
    }

    int track_count () const
    {
        return m_track_count;
    }

    track::number track_high () const
    {
        return m_track_high;
    }

    track::number track_max () const
    {
        return m_track_max;
    }

    /*
     * Transport Information functions.  Some have inline wrappers for
     * the caller's convenience.
     */

    transport::info & transportinfo ()
    {
        return m_transport_info;
    }

    const transport::info & transportinfo () const
    {
        return m_transport_info;
    }

    bool jack_transport () const;

    transport::clock::info & clockinfo ()
    {
        return m_clock_info;
    }

    const transport::clock::info & clockinfo () const
    {
        return m_clock_info;
    }

    midi::ppqn get_ppqn () const
    {
        return transportinfo().get_ppqn();
    }

    bool set_ppqn (midi::ppqn ppq);

    midi::pulse tick () const
    {
        return transportinfo().tick();
    }

    track::pointer get_track (track::number trk)
    {
        return track_list().at(trk);
    }

    const track::pointer get_track (track::number trk) const
    {
        return track_list().at(trk);
    }

    bool is_track_active (track::number trk) const
    {
        return bool(get_track(trk));
    }

    midi::bpm beats_per_minute () const
    {
        return transportinfo().beats_per_minute();
    }

    int beats_per_bar () const
    {
        return transportinfo().beats_per_bar();
    }

    int beat_width () const
    {
        return transportinfo().beat_width();
    }

    midi::microsec us_per_quarter_note () const
    {
        return transportinfo().us_per_quarter_note();
    }

    int get_32nds_per_quarter () const
    {
        return transportinfo().get_32nds_per_quarter();
    }

    bool beats_per_minute (midi::bpm bp, bool userchange = false);
    bool beats_per_bar (int bpmeasure, bool user_change = false);
    bool beat_width (int bw, bool user_change = false); // make virtual???

    void us_per_quarter_note (midi::microsec us)
    {
        transportinfo().us_per_quarter_note(us);
    }

    void set_32nds_per_quarter (int tpq)
    {
        transportinfo().set_32nds_per_quarter(tpq);
    }

    /*
     * Meant to set the last tick for all tracks. Could move the loop into
     * the tracklist. Can the tracklist hold null pointers?  Or disabled tracks
     * with no events???
     */

    void set_last_ticks (midi::pulse t)
    {
        for (auto & trk : track_list().tracks())
        {
            if (trk->active())
                trk->set_last_tick(t);
        }
    }

    void off_tracks ()
    {
        for (auto & trk : track_list().tracks())
        {
            if (trk->active())
                trk->set_armed(false);
        }
    }

    midi::pulse left_tick () const
    {
        return transportinfo().left_tick();
    }

    midi::pulse right_tick () const
    {
        return transportinfo().right_tick();
    }

    double left_right_size () const
    {
        return 0.0;     // TODO
    }

    /*
     * Other functions.
     */

    int smf_format () const
    {
        return m_smf_format;
    }

    void smf_format (int value)
    {
        m_smf_format = value == 0 ? 0 : 1 ;
    }

    bool error_pending () const
    {
        return m_error_pending;
    }

    const std::string & error_messages () const
    {
        return m_error_messages;
    }

    bool modified () const;

    /**
     *  This setter only sets the modified-flag to true.  The setter that can
     *  falsify it, unmodify(), is private.  No one but player and its friends
     *  should falsify this flag.
     */

    void modify (lib66::notification n = lib66::notification::no)
    {
        (void) n;
        m_modified = true;
    }

    void unmodify (lib66::notification n = lib66::notification::no);
    bool read_midi_file
    (
        const std::string & fn,
        std::string & errmsg,
        bool addtorecent = true
    );
    bool write_midi_file
    (
        const std::string & fn,
        std::string & errmsg,
        bool eventsonly = true
    );

public:

#if 0
    midi::ppqn file_ppqn () const
    {
        return m_file_ppqn;
    }

    void file_ppqn (midi::ppqn p)
    {
        m_file_ppqn = p;
    }
#endif

    midi::bytes & manufacturer_id ()
    {
        return m_manufacturer_id;
    }

    const midi::bytes & manufacturer_id () const
    {
        return m_manufacturer_id;
    }

    void manufacturer_id (const midi::bytes & manufid)
    {
        m_manufacturer_id = manufid;
    }

    int client_id () const
    {
        return m_master_bus->client_id();
    }

    bool is_running () const
    {
        return m_is_running;
    }

    bool is_pattern_playing () const
    {
        return m_is_pattern_playing;
    }

    void is_pattern_playing (bool flag)
    {
        m_is_pattern_playing = flag;
    }

    bool done () const;
    void left_tick (midi::pulse t);
    void right_tick (midi::pulse t);
    void left_tick_snap (midi::pulse tick, midi::pulse snap);
    void right_tick_snap (midi::pulse tick, midi::pulse snap);

    /*
     * ---------------------------------------------------------------------
     *  JACK Transport
     * ---------------------------------------------------------------------
     */

    transport::jack::scratchpad & pad ()
    {
        return m_jack_pad;
    }

    bool jack_output (transport::jack::scratchpad & pad)
    {
#if defined RTL66_BUILD_JACK
        return m_jack_transport.output(pad);
#else
        (void) pad;
        return false;
#endif
    }

    /**
     * \getter m_jack_transport.is_running()
     *      This function is useful for announcing the status of JACK in
     *      user-interface items that only have access to the player.
     */

    bool is_jack_running () const
    {
#if defined RTL66_BUILD_JACK
        return m_jack_transport.is_running();
#else
        return false;
#endif
    }

    /**
     *  Also now includes is_jack_running(), since one cannot be JACK Master
     *  if JACK is not running.
     */

    bool is_jack_master () const
    {
#if defined RTL66_BUILD_JACK
        return m_jack_transport.is_running() && m_jack_transport.is_master();
#else
        return false;
#endif
    }

    bool is_jack_slave () const
    {
#if defined RTL66_BUILD_JACK
        return m_jack_transport.is_running() && m_jack_transport.is_slave();
#else
        return false;
#endif
    }

    bool no_jack_transport () const
    {
#if defined RTL66_BUILD_JACK
        return ! m_jack_transport.is_running() || m_jack_transport.no_transport();
#else
        return true;
#endif
    }

    bool jack_transport_not_starting () const
    {
#if defined RTL66_BUILD_JACK
        return ! is_jack_running() || m_jack_transport.transport_not_starting();
#else
        return true;
#endif
    }

    /**
     *  If JACK is supported, starts the JACK transport.
     */

    void start_jack ()
    {
#if defined RTL66_BUILD_JACK
        m_jack_transport.start();
#endif
    }

    void stop_jack ()
    {
#if defined RTL66_BUILD_JACK
        m_jack_transport.stop();
#endif
    }

    bool init_transport ();
    bool deinit_transport ();

#if defined RTL66_BUILD_JACK
    void position_jack (bool songmode, midi::pulse tick)
    {
        m_jack_transport.position(songmode, tick);
    }
#else
    void position_jack (bool, midi::pulse) { /* no code */ }
#endif

    bool set_jack_mode (bool connect);

    void toggle_jack_mode ()
    {
#if defined RTL66_BUILD_JACK
        m_jack_transport.toggle_jack_mode();
#endif
    }

    bool get_jack_mode () const
    {
#if defined RTL66_BUILD_JACK
        return m_jack_transport.get_jack_mode();
#else
        return false;
#endif
    }

    midi::pulse jack_stop_tick () const
    {
#if defined RTL66_BUILD_JACK
        return m_jack_transport.jack_stop_tick();
#else
        return 0;
#endif
    }

    bool jack_set_beats_per_minute (midi::bpm bpm);

    bool jack_set_ppqn (int p)
    {
#if defined RTL66_BUILD_JACK
        m_jack_transport.set_ppqn(p);
        return true;
#else
        return p > 0;
#endif
    }

#if defined RTL66_BUILD_JACK
    void jack_stop_tick (midi::pulse tick)
    {
        m_jack_transport.jack_stop_tick(tick);
    }
#else
    void jack_stop_tick (midi::pulse)
    {
        /* no code needed */
    }
#endif

    midi::pulse get_jack_tick () const
    {
        return m_jack_tick;
    }

    void set_jack_tick (midi::pulse tick)
    {
        m_jack_tick = tick;             /* current JACK tick/pulse value    */
    }

    void jack_reposition (midi::pulse tick, midi::pulse stoptick);

    void set_reposition (bool postype)
    {
        transportinfo().reposition(postype);
    }

public:

    bool set_track_name (track::ref s, const std::string & name);
    bool set_midi_bus (track::number trkno, int buss);
    bool set_midi_channel (track::number trkno, int channel);
    bool set_recording
    (
        track::ref t, bool recordon,
        track::record r = track::record::normal,
        bool toggle = false
    );
    bool set_recording
    (
        track::number trkno, bool recordon,
        track::record r = track::record::normal,
        bool toggle = false
    );
    bool set_thru (track::ref t, bool thruon, bool toggle);
    bool set_thru (track::number trkno, bool thruon, bool toggle);

    bool setup ();
    bool launch ();
    bool finish ();
    bool activate ();
    bool new_track
    (
        track::number & finalseq,
        track::number seq = track::unassigned()
    );

    bool request_track (track::number seq = track::unassigned())
    {
        static track::number s_dummy;
        return new_track(s_dummy, seq);
    }

    bool delay_stop ();
    bool auto_stop ();
    bool auto_pause ();
    bool auto_play ();
    bool play (midi::pulse tick);
    void all_notes_off ();

    bool panic ();                                      /* from kepler43    */
    void set_tick (midi::pulse tick, bool dontreset = false);

    void start_tick (midi::pulse tick)
    {
        transportinfo().start_tick(tick);
    }

public:

    long delta_us () const
    {
        return m_delta_us;
    }

    bool install_track
    (
        track * seq, track::number & trkno, bool fileload = false
    );
    void inner_start ();
    void inner_stop (bool midiclock = false);

    void start ()
    {
        if (! is_jack_running())
            inner_start();
    }

    void stop ()
    {
        if (! is_jack_running())
            inner_stop();
    }

public:

    void start_playing ();
    void pause_playing ();
    void stop_playing ();
    midi::pulse get_max_extent () const;

    bool needs_update () const
    {
        return m_needs_update;
    }

    void needs_update (bool flag = true)
    {
        m_needs_update = flag;
    }

    const masterbus * master_bus () const
    {
        return m_master_bus.get();
    }

protected:

    masterbus * master_bus ()
    {
        return m_master_bus.get();
    }

    rtl::iothread & out_thread ()
    {
        return m_out_thread;
    }

    const rtl::iothread & out_thread () const
    {
        return m_out_thread;
    }

    rtl::iothread & in_thread ()
    {
        return m_in_thread;
    }

    const rtl::iothread & in_thread () const
    {
        return m_in_thread;
    }

private:

    void append_error_message (const std::string & msg) const;
    void reset_tracks (bool pause = false);

public:                             /* access functions for the containers  */

    void signal_save ();
    void signal_quit ();

private:

    void is_running (bool flag)
    {
        m_is_running = flag;
    }

private:

    bool output_func ();
    bool input_func ();
    bool poll_cycle ();
    bool launch_input_thread ();
    bool launch_output_thread ();
    void midi_start ();
    void midi_continue ();
    void midi_stop ();
    void midi_clock ();
    void midi_song_pos (const event & ev);
    void midi_sysex (const event & ev);

    synch & cv ()
    {
        return m_condition_var;
    }

private:

public:

};          // class player

}           // namespace midi

#endif      // RTL66_MIDI_PLAYER_HPP

/*
 * player.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

