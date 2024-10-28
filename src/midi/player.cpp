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
 * \file          player.cpp
 *
 *  This module defines the base class for a player of MIDI patterns.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom and others
 * \date          2022-07-10
 * \updates       2024-06-01
 * \license       GNU GPLv2 or above
 *
 */

#include <algorithm>                    /* std::find() for std::vector      */

#include "c_macros.h"                   /* not_nullptr macro                */
#include "midi/calculations.hpp"        /* midi::tempo_us_from_bpm()        */
#include "midi/file.hpp"                /* midi::read_midi_file()           */
#include "midi/player.hpp"              /* midi::player, this class         */
#include "rtl/midi/find_midi_api.hpp"   /* rtl::find_midi_api() etc.        */
#include "util/msgfunctions.hpp"        /* util::warn_message() etc.        */
#include "util/filefunctions.hpp"       /* util::filename_base(), etc.      */
#include "xpc/daemonize.hpp"            /* xpc::signal_for_exit()           */
#include "xpc/timing.hpp"               /* xpc::microsleep(), microtime()   */

namespace midi
{

/**
 *  This value is the "trigger width" in microseconds, a Seq24 concept.
 *  It also had a "lookahead" time of 2 ms, not used however.
 */

static const int c_thread_trigger_width_us = 4 * 1000;

#if defined UNUSED_VARIABLE

/**
 *  This high-priority value is used if the --priority option is specified.
 *  Needs more testing, we really haven't needed it yet.
 */

static const int c_thread_priority = 1;

#endif

/**
 *  When operating a playlist, especially from a headless seq66cli run, and
 *  with JACK transport active, the change from a playing tune to the next
 *  tune would really jack up JACK, crashing the app (corrupted double-linked
 *  list, double frees in destructors, etc.) and sometimes leaving a loud tone
 *  buzzing.  So after we stop the current tune, we delay a little bit to
 *  allow JACK playback to exit.  See the delay_stop() member function and its
 *  usages herein.
 *
 * Not needed in this base class.
 *
 *      static const int c_delay_stop_ms = 100;
 */

/**
 *  Indicates how much of a long file-path we will show using the
 *  shorten_file_spec() function.
 *
 *      static const int c_long_path_max = 56;
 */

/**
 *  The value of the default PPQN supported by this library. Moved to
 *  transport::info.
 *
 *      static const midi::ppqn c_default_ppqn = 192;
 */

/**
 *  Principal constructor.
 */

player::player (int out_portnumber, int in_portnumber) :
    m_manufacturer_id       {0},                /* 1 to 4 bytes             */
    m_master_bus            (),                 /* unique pointer           */
    m_in_portnumber         (in_portnumber),
    m_out_portnumber        (out_portnumber),
    m_track_list            (),
    m_track_count           (0),
    m_track_max             (1024),
    m_track_high            (track::unassigned()),
    m_sort_on_install       (false),
    m_smf_format            (1),
    m_out_thread            (),
    m_in_thread             (),
    m_is_running            (false),
    m_is_pattern_playing    (false),
    m_delta_us              (0),
    m_jack_pad              (),                 /* data for JACK... & ALSA  */
    m_jack_tick             (0),
    m_dont_reset_ticks      (false),            /* support for pausing      */
    m_condition_var         (*this),            /* private access via cv()  */
    m_clock_info            (),
    m_transport_info        (),                 /* a reference or pointer?  */
#if defined RTL66_BUILD_JACK
    m_jack_transport                // TODO: use transportinfo() as a parameter.
    (
        *this, 120.0,                           /* beats per minute         */
        transportinfo().get_ppqn(), 4,          /* beats per bar (measure)  */
        4                                       /* beat width (denominator) */
    ),
#endif
    m_error_pending         (false),
    m_error_messages        (),
    m_modified              (false),
    m_needs_update          (false)
{
    // no code
}

/**
 *  The destructor sets some running flags to false, signals this condition,
 *  then joins the input and output threads if the were launched.
 *
 *  A thread that has finished executing code, but has not yet been joined is
 *  still considered an active thread of execution and is therefore joinable.
 */

player::~player ()
{
    (void) finish();
}

/**
 *  This function emits an error message to cerr via the function
 *  util::error_message().
 */

void
player::append_error_message (const std::string & msg) const
{
    static std::vector<std::string> s_old_msgs;
    std::string newmsg = msg;
    m_error_pending = true;                         /* a mutable boolean    */
    if (newmsg.empty())
        newmsg = "Player error";

    if (! m_error_messages.empty())
    {
        const auto finding = std::find
        (
            s_old_msgs.cbegin(), s_old_msgs.cend(), newmsg
        );
        if (finding == s_old_msgs.cend())
        {
            m_error_messages += " ";
            m_error_messages += newmsg;
            s_old_msgs.push_back(newmsg);
            util::error_message("Player", newmsg);
        }
    }
    else
    {
        m_error_messages = newmsg;
        s_old_msgs.push_back(newmsg);
        util::error_message("Player", newmsg);
    }
}

/**
 *  Unmodify this object and all the tracks it contains.
 */

void
player::unmodify (lib66::notification n)
{
    (void) n;
    m_modified = false;                     /* m_needs_update = false;  */
    track_list().unmodify();
}

/**
 *  This improved version checks all of the tracks. This allows the user to
 *  unmodify a track without using player::modify().
 */

bool
player::modified () const
{
    bool result = m_modified;
    if (! result)
        result = track_list().modified();

    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Sequence Creation/Installation
 * -------------------------------------------------------------------------
 */

/**
 *  It assumes values have
 *  already been checked.  It does not set the "is modified" flag, since
 *  adding a track by loading a MIDI file should not set it.
 *  This function *does not* delete the track already present
 *  with the given track number; instead, it keeps incrementing the
 *  track number until an open slot is found.
 *
 * \param seq
 *      The pointer to the pattern/track to add.
 *
 * \param trkno
 *      The track number of the pattern to be added.  Not validated, to
 *      save some time.  This is only the starting value; if already filled,
 *      then next open slot is used, and this value will be updated to the
 *      actual number.
 *
 * \param fileload
 *      If true (the default is false), the modify flag will not be set.
 *
 * \return
 *      Returns true if the track was successfully added.
 */

bool
player::install_track
(
    track * trk,
    track::number & trkno,
    bool fileload
)
{
    bool result = track_list().add(trkno, trk); /* access tracklist, add    */
    if (result)
    {
        trkno = trk->track_number();            /* side-effect              */
        if (trkno > track_high())
            m_track_high = trkno;

#if defined PLATFORM_DEBUG_TMI
        std::string label = "Installed track ";
        label += std::to_string(trk->track_number());
        label += ": ";
        label += trk->track_name();
        printf("%s\n", label.c_str());
#endif
        lib66::toggle sorting = m_sort_on_install ?
            lib66::toggle::on : lib66::toggle::off ;

        trk->set_parent(this, sorting);         /* also sets a lot of stuff */
        if (! fileload)
            modify();
    }
    return result;
}

/**
 *  Creates a new pattern/track for the given slot, and sets the new
 *  pattern's master MIDI bus address.  Then it activates the pattern [this is
 *  done in the install_track() function].  It doesn't deal with thrown
 *  exceptions.
 *
 *  This function is called by the seqmenu and mainwnd objects to create a new
 *  track.  We now pass this track to install_track() to better
 *  handle potential memory leakage, and to make sure the track gets
 *  counted.  Also, adding a new track from the user-interface is a
 *  significant modification, so the "is modified" flag gets set.
 *
 *  If enabled, wire in the MIDI buss override.
 *
 * \param seq
 *      The prospective track number of the new track.  If not set to
 *      track::unassigned() (-1), then the track is also installed, and this
 *      value will be updated to the actual number.
 *
 * \return
 *      Returns true if the track is valid.  Do not use the
 *      track if false is returned, it will be null.
 */

bool
player::new_track (track::number & finalseq, track::number trkno)
{
    track * trkptr = new (std::nothrow) track();    // transportinfo().get_ppqn());
    bool result = not_nullptr(trkptr);
    if (result && trkno != track::unassigned())
    {
        result = install_track(trkptr, trkno);
        if (result)                                 /* new 2021-10-01   */
        {
            const track::pointer t = get_track(trkno);
            result = not_nullptr(t);
            if (result)
            {
                t->set_dirty();
                finalseq = t->track_number();
                // notify_track_change(finalseq, change::recreate);
            }
        }
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  NEXT
 * -------------------------------------------------------------------------
 */

/**
 *  Sets the PPQN for the master buss, JACK assistant, and the player.
 *  Note that we do not set the modify flag or do notification notification
 *  here.  See the change_ppqn() function instead.
 *
 * \setter ppqn
 *      Also sets other related members.
 *
 *      While running it is better to call change_ppqn(), in order to run
 *      though ALL patterns and user-interface objects to fix them.
 *
 * \param ppq
 *      Provides a PPQN that should be different from the current value and be
 *      in the legal range of PPQN values.
 */

bool
player::set_ppqn (midi::ppqn ppq)
{
    bool result = ppq != transportinfo().get_ppqn(); // && ppqn_in_range(p);
    if (result)
    {
        transportinfo().set_ppqn(ppq);
        if (transportinfo().one_measure() == 0)
            transportinfo().one_measure(ppq);
    }
    return result;
}

/**
 *  Locks on m_condition_var [accessed by function cv()].  Then, if not
 *  is_running(), the playback mode is set to the given state.  If that state
 *  is true, call off_tracks().  Set the running status, unlock, and signal
 *  the condition.
 *
 *  Note that we reverse unlocking/signalling from what Seq64 does (BUG!!!)
 *  Manual unlocking should be done before notifying, to avoid waking waking
 *  up the waiting thread, only to lock again.  See the notify_one() notes for
 *  details.
 *
 *  This function should be considered the "second thread", that is the thread
 *  that starts after the worker thread is already working.
 *
 *  In ALSA mode, restarting the track moves the progress bar to the
 *  beginning of the track, even if just pausing.  This is fixed by
 *  disabling calling off_tracks() when starting playback from the song
 *  editor / performance window.
 *
 * \param songmode
 *      Sets the playback mode, and, if true, turns off all of the tracks
 *      before setting the is-running condition.
 */

void
player::inner_start ()
{
    if (! done())                               /* won't start when exiting */
    {
        if (! is_running())
        {
            is_running(true);                   /* part of cv()'s predicate */
            pad().js_jack_stopped = false;
            cv().signal();                      /* signal we are running    */
        }
    }
}

/**
 *  Unconditionally, and without locking, clears the running status and resets
 *  the tracks.  Sets m_usemidiclock to the given value.  Note that we do
 *  need to set the running flag to false here, even when JACK is running.
 *  Otherwise, JACK starts ping-ponging back and forth between positions under
 *  some circumstances.
 *
 *  However, if JACK is running, we do not want to reset the tracks... this
 *  causes the progress bar for each track to move to near the end of the
 *  track.
 *
 * \param midiclock
 *      If true, indicates that the MIDI clock should be used.  The default
 *      value is false.
 */

void
player::inner_stop (bool midiclock)
{
    is_running(false);
    reset_tracks();                  /* resets, and flushes the buss     */
    m_clock_info.usemidiclock(midiclock);
}

/**
 *  Sets the value of the BPM into the master MIDI buss, after making
 *  sure it is squelched to be between 20 and 500.  Replaces
 *  player::set_bpm() from seq24.
 *
 *  The value is set only if neither JACK nor this player object are
 *  running.
 *
 *  It's not clear that we need to set the "is modified" flag just because we
 *  changed the beats per minute.  This setting does get saved to the MIDI
 *  file, with the c_bpmtag.
 *
 *  Do we need to adjust the BPM of all of the tracks, including the
 *  potential tempo track???  It is "merely" the putative main tempo of the
 *  MIDI tune.  Actually, this value can now be recorded as a Set Tempo event
 *  by user action in the main window (and, later, by incoming MIDI Set Tempo
 *  events).
 *
 * \param bpmin
 *      Provides the beats/minute value to be set.  Checked for validity.  It
 *      is a wide range of speeds, well beyond what normal music needs.
 *
 * \param userchange
 *      If true, the user changed the parameter.  Not used in this base class
 *      function.
 */

bool
player::beats_per_minute (midi::bpm bpmin, bool userchange)
{
    bool result = bpmin != transportinfo().beats_per_minute();
    if (result)
    {
        bpmin = fix_tempo(bpmin);
        transportinfo().beats_per_minute(bpmin);
        result = jack_set_beats_per_minute(bpmin);  /* not just JACK though */
        (void) userchange;
    }
    return result;
}

/**
 *  This is a faster version, meant for jack_transport to call.  This logic
 *  matches the original seq24, but is it really correct?  Well, we fixed it
 *  so that, whether JACK transport is in force or not, we can modify the BPM
 *  and have it stick.  No test for JACK Master or for JACK and normal running
 *  status needed.
 *
 *  Note that the JACK server, especially when transport is stopped,
 *  sends some artifacts (really low BPM), so we avoid dealing with low
 *  values.
 */

bool
player::jack_set_beats_per_minute (midi::bpm bpmin)
{
    bool result = true;
    if (result)
    {
#if defined RTL66_BUILD_JACK
        m_jack_transport.beats_per_minute(bpmin);  /* see banner note */
#endif
        if (m_master_bus)
            m_master_bus->BPM(bpmin);

        transportinfo().us_per_quarter_note(tempo_us_from_bpm(bpmin));
        transportinfo().beats_per_minute(bpmin);
    }
    return result;
}

/**
 *  Clears all of the patterns/tracks. Attempts to reset the player to
 *  its startup condition when no MIDI file is loaded
 *
 *  The mainwnd module calls this function, and the midifile and wrkfile
 *  classes, if a play-list is being loaded and verified.  Note that perform
 *  now handles the "is modified" flag on behalf of all external objects, to
 *  centralize and simplify the dirtying of a MIDI tune.
 *
 *  Anything else to clear?  What about all the other track flags?  We can
 *  beef up remove_track() for them, at some point.
 *
 *  Added stazed code from 1.0.5 to abort clearing if any of the tracks are
 *  in editing.
 *
 * \warning
 *      We have an issue.  Each GUIs conditional_update() function can call
 *      this one, potentially telling later GUIs that they do not need to
 *      update.  This affect the needs_update() function when not running
 *      playback.
 *
 * \param clearplaylist
 *      Defaults to false. Not supported in the base class.
 *
 * \return
 *      Returns true if the clear-all operation could be performed.  If false,
 *      then at least one active track was in editing mode (derived classes
 *      only).
 */

bool
player::clear_all (bool clearplaylist)
{
    (void) clearplaylist;
    track_list().clear();
    return true;
}

/**
 *  For all active patterns/tracks, get its playing state, turn off the
 *  playing notes, set playing to false, zero the markers, and, if not in
 *  playback mode, restore the playing state.  Note that these calls are
 *  folded into one member function of the track class.  Finally, flush the
 *  master MIDI buss.
 *
 *  Could use a member function pointer to avoid having to code two loops.
 *  We did it.  Note that std::shared_ptr does not support operator::->*, so
 *  we have to get() the pointer.
 *
 * \param p
 *      Try to prevent notes from lingering on pause if true.  By default, it
 *      is false.
 */

void
player::reset_tracks (bool p)
{
    void (track::* f) (bool) = p ? &track::pause : &track::stop ;
    bool songmode = false;                              // song_mode();
    for (auto & trk : track_list().tracks())
    {
        track * trkptr = trk.get();       // (trk->*f)(songmode);
        (trkptr->*f)(songmode);
    }

    /*
     * Alread flushed in the loop above.
     *
     * if (m_master_bus)
     *     m_master_bus->flush();
     */
}

/**
 *  Creates the midi::masterbus.  We need to delay creation until launch time,
 *  so that settings can be obtained before determining just how to set up the
 *  application.
 *
 * \return
 *      Returns true if the creation succeeded, or if the buss already exists.
 */

bool
player::create_master_bus ()
{
    bool result = bool(m_master_bus);
    if (! result)                       /* no master buss yet?  */
    {
        /*
         *  Find an available API.  Here, we rely on finding the fallback API,
         *  rather than a specified API.
         *
         *      rtl::rtmidi::api midiapi = rtl::rtmidi::selected(api);
         */

         rtl::rtmidi::api midiapi = rtl::find_midi_api();
        if (midiapi != rtl::rtmidi::api::unspecified)
        {
            /*
             * Cannot use std::make_unique<midi::masterbus> because its copy
             * constructor is deleted.
             *
             *  Also, at this point, do we have the actual complement of
             *  inputs and clocks, as opposed to what's in the rc file?
             */

            m_master_bus.reset
            (
                // new (std::nothrow) midi::masterbus(rtmidi::api, m_ppqn, m_bpm)

                new (std::nothrow) midi::masterbus(midiapi) /* TODO */
            );
            if (m_master_bus)
            {
#if DERIVED_CLASS
                midi::masterbus * mmb = m_master_bus.get();
                mmb->filter_by_channel(m_filter_by_channel);
                mmb->set_port_statuses(m_clocks, m_inputs);
                midi_control_out().set_master_bus(mmb);
#endif
                result = true;
            }
        }
    }
    return result;
}

/**
 *      return ! m_is_running;
 */

bool
player::done () const
{
    return in_thread().done() && out_thread().done();
}

/**
 *  Creates the master MIDI buss. At the end of this function, the
 *  caller can display the ports that were found and enable/disable them.
 */

bool
player::setup ()
{
    bool result = create_master_bus();      /* creates m_master_bus         */
    if (result)
    {
        /* TODO? */
    }
    return result;
}

/**
 *  Calls the MIDI buss and JACK initialization functions and the input/output
 *  thread-launching functions.  This function is called in main().  We
 *  collected all the calls here as a simplification, and renamed it because
 *  it is more than just initialization.  This function must be called after
 *  the perform constructor and after the configuration file and command-line
 *  configuration overrides.  The original implementation, where the master
 *  buss was an object, was too inflexible to handle a JACK implementation.
 */

bool
player::launch ()
{
    bool result =  bool(m_master_bus);
    if (! result)
        result = setup();

    if (result)
    {
        result = init_transport();
        if (result)
        {
            result = m_master_bus->engine_initialize
            (
                get_ppqn(), transportinfo().beats_per_minute()
            );
        }
        if (result)
            result = activate();

        if (result)
        {
            /*
             * Get and store the clocks and inputs created (disabled or not)
             * by the midi::masterbus during api_init().  After this call, the
             * clocks and inputs now have names.  These calls are necessary to
             * populate the port lists the first time Seq66 is run.
             *
             * m_master_bus->get_port_statuses(m_clocks, m_inputs); the
             * statuses from e.g. midi_jack_info are already obtained in the
             * call stack of create_master_bus().
             */

#if defined USE_MASTER_BUS

            /*
             * Not defined in the base class yet.
             */

            m_master_bus->copy_io_busses();
            m_master_bus->get_port_statuses(m_clocks, m_inputs);
#endif

            if (m_in_portnumber >= 0)
                launch_input_thread();

            if (m_out_portnumber >= 0)
                launch_output_thread();
        }
        else
            m_error_pending = true;
    }
    return result;
}

/**
 *  Sets the beats per measure.
 *
 *  Note that a lambda function is used to make the changes.
 */

bool
player::beats_per_bar (int bpmeasure, bool /* user_change */)
{
    bool result = bpmeasure != transportinfo().beats_per_bar();
    if (result)
    {
        /* MUST also sets in jack_transport  */
        transportinfo().beats_per_bar(bpmeasure);
#if defined USE_MAPPER
        mapper().exec_set_function
        (
            [bpmeasure, user_change] (track::pointer sp, track::number /*sn*/)
            {
                bool result = bool(sp);
                if (result)
                {
                    sp->set_beats_per_bar(bpmeasure, user_change);
                    sp->set_measures(sp->get_measures());
                }
                return result;
            }
        );
#else
    // WE NEED TO ITERATE over the tracklist
#endif
    }
    return result;
}

/**
 *
 * \param bw
 *      Provides the value for beat-width.
 *
 *      TODO: Also used to set the
 *      beat-width in the JACK assistant object.
 */

bool
player::beat_width (int bw, bool /* user_change */)
{
    bool result = bw != transportinfo().beat_width();
    if (result)
    {
        transportinfo().beat_width(bw);
#if defined USE_MAPPER
        set_beat_length(bw);            /* also sets in jack_transport  */
        mapper().exec_set_function
        (
            [bw, user_change] (track::pointer sp, track::number /*sn*/)
            {
                bool result = bool(sp);
                if (result)
                {
                    sp->set_beat_width(bw, user_change);
                    sp->set_measures(sp->get_measures());
                }
                return result;
            }
        );
#else
    // WE NEED TO ITERATE over the tracklist
#endif
    }
    return result;
}

/**
 *  Creates the output thread using output_thread_func().  This might be a
 *  good candidate for a small thread class derived from a small base class.
 *
 *  -   We may want more control over lifetime of object, for example to
 *      initialize it "lazily". The std::thread already supports it. It can be
 *      made in "not representing a thread" state, assigned a real thread
 *      later when needed, and it has to be explicitly joined or detacheded
 *      before destruction.
 *  -   We may want a member to be transferred to/from some other ownership.
 *      No need of pointer for that since std::thread already supports move
 *      and swap.
 *  -   We may want opaque pointer for to hide implementation details and
 *      reduce dependencies, but std::thread is standard library class so we
 *      can't make it opaque.
 *  -   We may want to have shared ownership. That is fragile scenario with
 *      std::thread. Multiple owners who can assign, swap, detach, or join it
 *      (and there are no much other operations with thread) can cause
 *      complications.
 *  -   There is mandatory cleanup before destruction like if
 *      (xthread.joinable()) xthread.join(); or xthread.detach();. That is
 *      also more robust and easier to read in destructor of owning class
 *      instead of code that instruments same thing into deleter of a smart
 *      pointer.
 *
 *  So unless there is some uncommon reason we should use thread as data
 *  member directly.
 */

bool
player::launch_output_thread ()
{
    rtl::iothread::functor threadfunc = std::bind(&player::input_func, this);
    return out_thread().launch(threadfunc);
}

/**
 *  Creates the input thread using input_thread_func().  This might be a good
 *  candidate for a small thread class derived from a small base class.
 */

bool
player::launch_input_thread ()
{
    rtl::iothread::functor threadfunc = std::bind(&player::input_func, this);
    return in_thread().launch(threadfunc);
}

/**
 *  The rough opposite of launch(); it doesn't stop the threads.  A minor
 *  simplification for the main() routine, hides the JACK support macro.
 *  We might need to add code to stop any ongoing outputing.
 *
 *  Also gets the settings made/changed while the application was running from
 *  the mastermidibase class to here.  This action is the converse of calling
 *  the set_port_statuses() function defined in the mastermidibase module.
 *
 *  Note that we call stop_playing().  This will stop JACK transport. If we
 *  restart Seq66 without doing this, transport keeps running (as can be seen
 *  in QJackCtl).  So playback starts while loading a MIDI file while Seq66
 *  starts.  Not only is this kind of surprising, it can lead to a seqfault at
 *  random times.
 *
 *  Also note that m_is_running and m_io_active are both used in the
 *  player::synch::predicate() override.
 */

bool
player::finish ()
{
    bool result = true;
    if (! done())
    {
        stop_playing();                     /* see notes in banner          */
        reset_tracks();                     /* stop all output upon exit    */
        m_is_running = false;               /* set is_running() off         */
        out_thread().deactivate();          /* set the input 'done' flag    */
        in_thread().deactivate();           /* set the output 'done' flag   */
        cv().signal();                      /* signal the end of play       */
        (void) out_thread().finish();
        (void) in_thread().finish();

        bool ok = deinit_transport();

#if defined USE_MASTER_BUS

        /*
         * Not defined in the base class.
         * We're supporting only one of each I/O port here.
         */

        result = bool(m_master_bus);
        if (result)
            m_master_bus->get_port_statuses(m_clocks, m_inputs);
#endif

        result = ok && result;
    }
    return result;
}

/**
 *  Performs a controlled activation of the jack_transport and other JACK
 *  modules. Currently does work only for JACK; the activate() calls for other
 *  APIs just return true without doing anything.  However...
 *
 * ca 2021-07-14 Move this. Why doing it even if no JACK transport specified?
 *
 *  Also, can we "activate" the I/O threads here?
 */

bool
player::activate ()
{
    bool result = m_master_bus && m_master_bus->engine_activate();

#if defined RTL66_BUILD_JACK_ACTIVATE_HERE // init_jack_transport() instead
    if (result)
        result = m_jack_transport.activate();
#endif

    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Tick Support
 * -------------------------------------------------------------------------
 */

void
player::set_tick (midi::pulse tick, bool dontreset)
{
    transportinfo().tick(tick);
    if (dontreset)
    {
        m_dont_reset_ticks = true;
        transportinfo().start_tick(tick);
    }
}

/**
 *  Set the left marker at the given tick.  We let the caller determine if
 *  this setting is a modification.  If the left tick is later than the right
 *  tick, the right tick is move to one measure past the left tick.
 *
 * \todo
 *      The player::m_one_measure member is currently hardwired to PPQN*4.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the left tick.  If the left
 *      tick is greater than or equal to the right tick, then the right ticked
 *      is moved forward by one "measure's length" (PPQN * 4) past the left
 *      tick.
 */

// MOVE JACK STUFF INTO JACK_TRANSPORT_INFO
// MOVE JACK STUFF INTO JACK_TRANSPORT_INFO
// MOVE JACK STUFF INTO JACK_TRANSPORT_INFO

void
player::left_tick (midi::pulse tick)
{
    transportinfo().left_tick(tick);
    if (is_jack_master())                       /* don't use in slave mode  */
    {
        position_jack(true, tick);
        transportinfo().tick(tick);
    }
    else if (! is_jack_running())
        transportinfo().tick(tick);

//  if (m_left_tick >= m_right_tick)
//      m_right_tick = m_left_tick + m_one_measure;
}

/**
 *  Set the right marker at the given tick.  This setting is made only if the
 *  tick parameter is at or beyond the first measure.  We let the caller
 *  determine is this setting is a modification.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the right tick.  If less than
 *      or equal to the left tick setting, then the left tick is backed up by
 *      one "measure's worth" (PPQN * 4) worth of ticks from the new right
 *      tick.
 *
 *      MOVE SOME OF THIS CODE TO transport_info::right_tick ().
 */

void
player::right_tick (midi::pulse tick)
{
    transportinfo().right_tick(tick);
    if (tick == 0)
        tick = transportinfo().one_measure();

    if (tick >= transportinfo().one_measure())
    {
        transportinfo().right_tick(tick);
        if (transportinfo().right_tick() <= transportinfo().left_tick())
        {
            midi::pulse lefttick = transportinfo().left_tick() -
                transportinfo().one_measure();

            start_tick(lefttick);
            transportinfo().reposition(false);
            if (is_jack_master())
                position_jack(true, transportinfo().left_tick());
            else
                set_tick(transportinfo().left_tick());
        }
    }
}

/**
 *  Used by qseqtime and qperftime in Seq66.
 */

void
player::left_tick_snap (midi::pulse tick, midi::pulse snap)
{
    tick = transportinfo().left_tick_snap(tick, snap);
    midi::pulse remainder = tick % snap;
    if (remainder > (snap / 2))
        tick += snap - remainder;               /* move up to next snap     */
    else
        tick -= remainder;                      /* move down to next snap   */

    if (transportinfo().right_tick() <= tick)
        transportinfo().right_tick_snap(tick + 4 * snap, snap);

    transportinfo().left_tick(tick);
    start_tick(tick);
    transportinfo().reposition(false);
    if (is_jack_master())                       /* don't use in slave mode  */
        position_jack(true, tick);
    else if (! is_jack_running())
        transportinfo().tick(tick);
}

void
player::right_tick_snap (midi::pulse tick, midi::pulse snap)
{
    tick = transportinfo().right_tick_snap(tick, snap);
    midi::pulse remainder = tick % snap;
    if (remainder > (snap / 2))
        tick += snap - remainder;               /* move up to next snap     */
    else
        tick -= remainder;                      /* move down to next snap   */

    if (tick > transportinfo().left_tick())
    {
        transportinfo().right_tick(tick);
        start_tick(transportinfo().left_tick());
        transportinfo().reposition(false);
        if (is_jack_master())
            position_jack(true, tick);
        else
            set_tick(tick);
    }
}

bool
player::set_midi_bus (track::number trkno, int buss)
{
    track::pointer tp = get_track(trkno);
    bool result = bool(tp);
    if (result)
        result = tp->midi_bus(buss, true);              /* a user change    */

    return result;
}

/**
 *  The only legal values for channel are 0 through 15, and null_channel(),
 *  which is 0x80, and indicates a "Free" channel (i.e. the pattern is
 *  channel-free.
 *
 *  The live-grid popup-menu calls this function, while the pattern-editor
 *  dropdown calls track::midi_channel() directly.  Both calls set the
 *  user-change flag.
 *
 * \param trkno
 *      Provides the track number for the channel setting.
 *
 * \paramm channel
 *      Provides the channel setting, 0 to 15.  If greater than that, it is
 *      coerced to the null-channel (0x80).
 */

bool
player::set_midi_channel (track::number trkno, int channel)
{
    track::pointer s = get_track(trkno);
    bool result = bool(s);
    if (result)
    {
        if (channel >= c_channel_max)                   /* 0 to 15, Free    */
            channel = null_channel();                   /* Free             */

        result = s->midi_channel(midi::byte(channel), true);  /* user ch. */
    }
    return result;
}

/**
 *  Also modify()'s.
 */

bool
player::set_track_name (track::ref t, const std::string & name)
{
    bool result = name != t.track_name();
    if (result)
    {
//      track::number trkno = t.track_number();
        t.track_name(name);
//      set_needs_update();             /* tell GUIs to refresh. FIXME  */
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Recording
 * -------------------------------------------------------------------------
 */

/**
 *  Encapsulates code used by the track editing frames' recording-change
 *  callbacks.
 *
 *  Note that this base class currently supports only record::normal recording.
 *
 * \param recordon
 *      Provides the current status of the Record button.
 *
 * \param r
 *      Provides the type of recording (e.g. normal vs. quantized).
 *
 * \param s
 *      The track that the seqedit window represents.  This pointer is
 *      checked.
 */

bool
player::set_recording (track::ref t, bool recordon, track::record r, bool toggle)
{
    return t.set_recording(recordon, r, toggle);
}

/**
 *  Encapsulates code used internally by player's automation mechanism. This
 *  is a private function.
 *
 * \param trkno
 *      The track number; the resulting pointer is checked.
 *
 * \param recordon
 *      Provides the current status of the Record button.
 *
 * \param r
 *      Provides the type of recording (e.g. normal vs. quantized).
 *
 * \param toggle
 *      If true, ignore the first flag and let the track toggle its
 *      setting.  Passed along to track::input_recording().
 */

bool
player::set_recording
(
    track::number trkno, bool recordon, track::record r, bool toggle
)
{
    track * tp = get_track(trkno).get();
    bool result = not_nullptr(tp);
    if (result)
        result = set_recording(*tp, recordon, r, toggle);

    return result;
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().
 *
 * \param thruon
 *      Provides the current status of the Thru button.
 *
 * \param toggle
 *      Indicates to toggle the status.
 *
 * \param s
 *      The track that the seqedit window represents.  This pointer is
 *      checked.
 */

bool
player::set_thru (track::ref t, bool thruon, bool toggle)
{
    return t.set_thru(thruon, toggle);
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().  However, this
 *  function depends on the track, not the seqedit, for obtaining the
 *  recording status.
 *
 * \param thruon
 *      Provides the current status of the Thru button.
 *
 * \param seq
 *      The track number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the track toggle its
 *      setting.  Passed along to track::set_input_thru().
 */

bool
player::set_thru (track::number trkno, bool thruon, bool toggle)
{
    track * tp = get_track(trkno).get();
    bool result = not_nullptr(tp);
    if (result)
        result = set_thru(*tp, thruon, toggle);

    return result;
}

/*
 * -------------------------------------------------------------------------
 *  JACK Transport (and some others)
 * -------------------------------------------------------------------------
 */

/**
 *  Returns true if JACK is built in, JACK transport is specified, and
 *  JACK is the selected API.
 */

bool
player::jack_transport () const
{
#if defined RTL66_BUILD_JACK
    bool is_jack_api = m_master_bus->selected_api() == rtl::rtmidi::api::jack;
    return is_jack_api && transportinfo().jack_transport();
#else
    return false;
#endif
}

/**
 *  Initializes JACK support, if defined.  The launch() function and
 *  options module (when Connect is pressed) call this.
 *
 * \return
 *      Returns the result of midi::jack::transport.init() if built
 *      for JACK. Returns true if not built for JACK.
 */

bool
player::init_transport ()
{
    if (jack_transport())
        return m_jack_transport.init();
    else
        return true;
}

/**
 *  Tears down the JACK infrastructure.  Called by launch() and the
 *  options module (when Disconnect is pressed).  This function operates
 *  only while we are not outputing, otherwise we have a race condition
 *  that can lead to a crash.
 *
 * \return
 *      Returns the result of the init() call; true if JACK sync is now
 *      no longer running or JACK is not supported.
 */

bool
player::deinit_transport ()
{
    if (jack_transport())
        return m_jack_transport.deinit();
    else
        return true;
}

/**
 *  Encapsulates behavior needed by perfedit.  Note that we moved some of the
 *  code from perfedit::set_jack_mode() [the seq32 version] to this function.
 *
 * \param connect
 *      Indicates if JACK is to be connected versus disconnected.
 *
 * \return
 *      Returns true if JACK is running currently, and false otherwise.
 */

bool
player::set_jack_mode (bool connect)
{
    if (! is_running())
    {
        if (connect)
            (void) init_transport();
        else
            (void) deinit_transport();
    }
#if defined RTL66_BUILD_JACK
    m_jack_transport.set_jack_mode(is_jack_running()); /* seqroll keybinding  */
#endif

    /*
     *  For setting the transport tick to display in the correct location.
     *  FIXME: does not work for slave from disconnected; need JACK position.
     */

    start_tick(transportinfo().tick());
    return is_jack_running();
}

/**
 *
 * \param tick
 *      The current JACK position in ticks.
 *
 * \param stoptick
 *      The current JACK stop-tick.
 */

void
player::jack_reposition (midi::pulse tick, midi::pulse stoptick)
{
    midi::pulse diff = tick - stoptick;
    if (diff != 0)
    {
        set_reposition(true);
        start_tick(tick);
        jack_stop_tick(tick);
    }
}

/**
 *  Set up the performance and start the thread.  This function should be
 *  considered the "worker thread".  We rely on C++11's thread handling to set
 *  up the thread properly on Linux and Windows.  It runs while m_io_active is
 *  true, which is set in the constructor, stays that way basically for the
 *  duration of the application.  We do not use std::unique_lock<std::mutex>,
 *  because we want a recursive mutex.
 *
 * \warning
 *      Valgrind shows that output_func() is being called before the JACK
 *      client pointer is being initialized!!!
 *
 *  See the old global output_thread_func() in Sequencer64.  This locking is
 *  similar to that of inner_start(), except that signalling (notification) is
 *  not done here. While running, we:
 *
 *      -#  Before the "is-running" loop:  If in any view (song, grid, or pattern
 *          editor), we care about starting from the m_start_tick offset.
 *          However, if the pause key is what resumes playback, we do not want
 *          to reset the position.  So how to detect that situation, since
 *          m_is_pause is now false?
 *      -#  At the top of the "is-running" loop:
 *          -# Get delta time (current - last).
 *          -# Get delta ticks from time.
 *          -# Add to current_ticks.
 *          -# Compute prebuffer ticks.
 *          -# Play from current tick to prebuffer.
 *      -#  Delta time to ticks; get delta ticks.  seq24 0.9.3 changes
 *          delta_tick's type and adds code -- delta_ticks_frac is in 1000th of
 *          a tick.  This code is meant to correct for clock drift.  However,
 *          this code breaks the MIDI clock speed.  So we use the "Stazed"
 *          version of the code, from seq32.  We get delta ticks, delta_ticks_f
 *          is in 1000th of a tick.
 *
 * microsleep() call:
 *
 *      Figure out how much time we need to sleep, and do it.  Then we want to
 *      trigger every c_thread_trigger_width_us; it took delta_us microseconds
 *      to play().  Also known as the "sleeping_us".  Check the MIDI clock
 *      adjustment:  60000000.0f / PPQN * / bpm.
 *
 *      With the long patterns in a rendition of Kraftwerk's "Europe Endless",
 *      we found that that the Qt thread was getting starved, as evidenced by
 *      a great slow-down in counting in the timer function in qseqroll. And
 *      many other parts of the user interface were slow.  This bug did not
 *      appear in version 0.91.3, but we never found out the difference that
 *      caused it.  However, a microsleep(1) call mitigated the problem
 *      without causing playback issues.
 *
 *      What might be happening is the the call on line 3144 is not being
 *      made.  But it's not being called in 0.91.3 either!  However, we found
 *      we were using milliseconds, not microseconds!  Once corrected, we
 *      were getting sleep deltas from 3800, down to around 1000 as the load
 *      level (number of playing patterns) increased.
 *
 *      Now, why the 2.0 factor in this?
 *
 *          if (next_clock_delta_us < (c_thread_trigger_width_us * 2.0))
 *
 * Stazed code (when ready):
 *
 *      If we reposition key-p, FF, rewind, adjust delta_tick for change then
 *      reset to adjusted starting.  We have to grab the clock tick if looping
 *      is unchecked while we are running the performance; we have to
 *      initialize the MIDI clock (send EVENT_MIDI_SONG_POS); we have to
 *      restart at the left marker; and reset the tempo list (which Seq64
 *      doesn't have).
 */

bool
player::output_func ()
{
    if (! xpc::set_timer_services(true))    /* wrapper for Win-only func.   */
    {
        (void) xpc::set_timer_services(false);
        return false;
    }
    while (out_thread().active())
    {
        cv().wait();                        /* lock mutex, predicate wait   */
        if (out_thread().done())            /* if stopping, exit the thread */
            break;

        pad().initialize(0, transportinfo().looping(), song_mode());

        /*
         * If song-mode Master, then start the left tick marker if the Stazed
         * "key-p" position was set.  If live-mode master, start at 0.  This
         * code is also present at about line #3209, and covers more
         * complexities.
         */

        if (! m_dont_reset_ticks)           /* no repositioning in pause    */
        {
            if (song_mode())
            {
                if (is_jack_master() && transportinfo().reposition())
                    position_jack(true, left_tick());
            }
            else
                position_jack(false, 0);
        }

        /*
         *  See note 1 in the function banner.
         */

        midi::pulse startpoint;
        if (m_dont_reset_ticks)
            startpoint = transportinfo().tick();
        else if (transportinfo().looping())
            startpoint = transportinfo().left_tick();
        else
            startpoint = transportinfo().start_tick();

        pad().set_current_tick(startpoint);
        set_last_ticks(startpoint);

        /*
         * We still need to make sure the BPM and PPQN changes are airtight!
         * Check jack_set_beats_per_minute() and change_ppqn()
         */

        double bwdenom = 4.0 / beat_width();
        midi::bpm bpmfactor = m_master_bus->BPM() * bwdenom;
        int ppq = m_master_bus->PPQN();
        int bpm_times_ppqn = bpmfactor * ppq;
        double dct = double_ticks_from_ppqn(ppq);
        double pus = pulse_length_us(bpmfactor, ppq);
        long current;                           /* current time             */
        long elapsed_us, delta_us;              /* current - last           */
        long last = xpc::microtime();           /* beginning time           */
        transportinfo().resolution_change_clear();
        while (is_running())
        {
            if (transportinfo().resolution_change())    /* atomic boolean   */
            {
                bwdenom = 4.0 / beat_width();
                bpmfactor = m_master_bus->BPM() * bwdenom;
                ppq = m_master_bus->PPQN();
                bpm_times_ppqn = bpmfactor * ppq;
                dct = double_ticks_from_ppqn(ppq);
                pus = pulse_length_us(bpmfactor, ppq);
                transportinfo().resolution_change_management
                (
                    bpmfactor, ppq, bpm_times_ppqn, dct, pus
                );
            }

            /**
             *  See note 2 and the microsleep() note in the function banner.
             *  See note 3 in the function banner.
             */

            current = xpc::microtime();
            delta_us = elapsed_us = current - last;

            long long delta_tick_num = bpm_times_ppqn * delta_us +
                pad().js_delta_tick_frac;

            long delta_tick = long(delta_tick_num / 60000000LL);
            pad().js_delta_tick_frac = long(delta_tick_num % 60000000LL);
#if 0
            if (m_usemidiclock)
            {
                delta_tick = m_midiclocktick;       /* int to long          */
                m_midiclocktick = 0;
                if (m_midiclockpos >= 0)            /* was after this if    */
                {
                    delta_tick = 0;
                    pad().set_current_tick(midi::pulse(m_midiclockpos));
                    m_midiclockpos = -1;
                }
            }
#else
            if (m_clock_info.usemidiclock())
            {
                delta_tick = m_clock_info.adjust_midi_tick();
            }
#endif

            bool jackrunning = jack_output(pad());
            if (jackrunning)
            {
                // No additional code needed besides the output() call above.
            }
            else
            {
#if defined USE_THIS_STAZED_CODE_WHEN_READY
                if (! m_clock_info.usemidiclock() && transportinfo().reposition())
                {
                    current_tick = clock_tick;
                    delta_tick = transportinfo().start_tick() - clock_tick;
                    init_clock = true;
                    transportinfo().start_tick = transportinfo().left_tick();
                    transportinfo().reposition(false);
                    m_reset_tempo_list = true;
                }
#endif
                pad().add_delta_tick(delta_tick);   /* add to current ticks */
            }

            /*
             * pad().js_init_clock will be true when we run for the first time,
             * or as soon as JACK gets a good lock on playback.
             */

            if (pad().js_init_clock)
            {
                m_master_bus->handle_clock
                (
                    midi::clock::action::init, midi::pulse(pad().js_clock_tick)
                );
                pad().js_init_clock = false;
            }
            if (pad().js_dumping)
            {
                if (transportinfo().looping())
                {
                    /*
                     * This stazed JACK code works better than the original
                     * code, so it is now permanent code.
                     */

                    static bool jack_position_once = false;
                    midi::pulse rtick = right_tick();     /* can change? */
                    if (pad().js_current_tick >= rtick)
                    {
                        if (is_jack_master() && ! jack_position_once)
                        {
                            position_jack(true, left_tick());
                            jack_position_once = true;
                        }

                        double leftover_tick = pad().js_current_tick - rtick;
                        if (jack_transport_not_starting())  /* no FF/RW xrun */
                        {
                            play(rtick - 1);
                        }
                        reset_tracks();

                        midi::pulse ltick = left_tick();
                        set_last_ticks(ltick);
                        pad().js_current_tick = double(ltick) + leftover_tick;
                    }
                    else
                        jack_position_once = false;
                }

                /*
                 * Don't play during JackTransportStarting to avoid xruns on
                 * FF or RW.
                 */

                if (jack_transport_not_starting())
                {
                    play(midi::pulse(pad().js_current_tick));
                }

                /*
                 * The next line enables proper pausing in both old and seq32
                 * JACK builds.
                 */

                set_jack_tick(pad().js_current_tick);
                m_master_bus->handle_clock
                (
                    midi::clock::action::emit, midi::pulse(pad().js_clock_tick)
                );
            }

            /*
             *  See "microsleep() call" in banner.  Code is similar to line
             *  3096 above.
             */

            last = current;
            current = xpc::microtime();
            elapsed_us = current - last;
            delta_us = c_thread_trigger_width_us - elapsed_us;

            double next_clock_delta = dct - 1;
            double next_clock_delta_us = next_clock_delta * pus;
            if (next_clock_delta_us < (c_thread_trigger_width_us * 2.0))
                delta_us = long(next_clock_delta_us);

            if (delta_us > 0)
            {
                (void) xpc::microsleep(int(delta_us));
                m_delta_us = 0;
            }
            else
            {
#if defined RTL66_PLATFORM_DEBUG
                if (delta_us != 0)
                {
                    print_client_tag(lib66::msglevel::warn);
                    fprintf
                    (
                        stderr, "Play underrun %ld us          \r",
                        delta_us
                    );
                }
#endif
                m_delta_us = delta_us;
            }
            if (pad().js_jack_stopped)
                inner_stop();
        }

        /*
         * Disabling this setting allows all of the progress bars (seqroll,
         * perfroll, and the slots in the mainwnd) to stay visible where
         * they paused.  However, the progress still restarts when playback
         * begins again, without some other changes.  The "tick" is the progress
         * play tick that determines the progress bar location.
         */

        if (! m_dont_reset_ticks)
        {
            midi::pulse start = song_mode() ? transportinfo().left_tick() : 0 ;
            if (is_jack_master())
                position_jack(song_mode(), start);
            else if (! m_clock_info.usemidiclock() && ! is_jack_running())
                transportinfo().tick(start);
        }

        /*
         * This means we leave m_tick at stopped location if in slave mode or
         * if m_usemidiclock == true.
         */

        m_master_bus->flush();
#if defined USE_MASTER_BUS
        m_master_bus->stop();
#endif
    }
    (void) xpc::set_timer_services(false);
    return true;
}

/**
 *  This function is called by input_thread_func().  It handles certain MIDI
 *  input events.  Many of them are now handled by functions for easier reading
 *  and trouble-shooting (of MIDI clock).
 */

bool
player::input_func ()
{
    if (xpc::set_timer_services(true))  /* wrapper for a Windows-only func. */
    {
        while (! done())
        {
            if (! poll_cycle())
                break;
        }
        xpc::set_timer_services(false);
        return true;
    }
    else
        return false;
}

/**
 *  A helper function for input_func().
 */

bool
player::poll_cycle ()
{
    bool result = ! done();
    if (result)
        result = m_master_bus->poll_for_midi() > 0;

    if (result)
    {
        do
        {
            if (done())
            {
                result = false;
                break;                              /* spurious exit events */
            }

            event ev;
#if defined USE_MASTER_BUS
            bool incoming = m_master_bus->get_midi_event(&ev);
#else
            bool incoming = false;
#endif
            if (incoming)
            {
                if (ev.is_below_sysex())                    /* below 0xF0   */
                {
#if defined RTL66_PLATFORM_DEBUG_TMI
                    std::string estr = ev.to_string();
                    util::status_message("MIDI event", estr);
#endif
#if defined USE_MASTER_BUS
                    if (m_master_bus->is_dumping())         /* see banner   */
                    {
                        ev.set_timestamp(tick());
                        if (m_filter_by_channel)
                            m_master_bus->dump_midi_input(ev);
                        else
                            m_master_bus->get_track()->stream_event(ev);
                    }
#endif
                }
                else if (ev.is_midi_start())
                {
                    midi_start();
                }
                else if (ev.is_midi_continue())
                {
                    midi_continue();
                }
                else if (ev.is_midi_stop())
                {
                    midi_stop();
                }
                else if (ev.is_midi_clock())
                {
                    midi_clock();
                }
                else if (ev.is_midi_song_pos())
                {
                    midi_song_pos(ev);
                }
                else if (ev.is_tempo())             /* added for issue #76  */
                {
                    /*
                     * Should we do this only if JACK transport is not
                     * enabled?
                     */

                    if (is_jack_master() || ! is_jack_running())
                        (void) beats_per_minute(ev.tempo());
                }
                else if (ev.is_sysex())
                {
                    midi_sysex(ev);
                }
#if defined USE_ACTIVE_SENSE_AND_RESET
                else if (ev.is_sense_reset())
                {
                    return false;                   /* see note in banner   */
                }
#endif
                else
                {
                    /* ignore the event */
                }
            }
        } while (m_master_bus->is_more_input());
    }
    return result;
}

/**
 * http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/ssp.htm
 *
 *      Example: If a Song Position value of 8 is received, then a trackr
 *      (or drum box) should cue playback to the third quarter note of the
 *      song.  (8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
 *      Since there are 24 MIDI Clocks in a quarter note, the first quarter
 *      occurs on a time of 0 MIDI Clocks, the second quarter note occurs upon
 *      the 24th MIDI Clock, and the third quarter note occurs on the 48th
 *      MIDI Clock).
 *
 *      8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
 *
 *      The MIDI-control check will limit the controls to start, stop and record
 *      only. The function returns a a bool flag indicating whether the event was
 *      used or not. The flag is used to exclude from recording the events that
 *      are used for control purposes and should not be recorded (dumping).  If
 *      the used event is a note on, then the linked note off will also be
 *      excluded.
 *
 * http://midi.teragonaudio.com/tech/midispec/seq.htm
 *
 *      Provides a description of how the following events and Song Position
 *      work.
 *
 * EVENT_MIDI_START:
 *
 *      Starts the MIDI Time Clock.  The Master sends this message, which
 *      alerts the slave that, upon receipt of the very next MIDI Clock
 *      message, the slave should start playback.  MIDI Start puts the slave
 *      in "play mode", and the receipt of that first MIDI Clock marks the
 *      initial downbeat of the song.  MIDI B
 *
 *      Kepler34 does "stop(); set_playback_mode(false); start();" in its
 *      version of this event.  This sets the playback mode to Live mode. This
 *      behavior seems reasonable, though the function names Seq66 uses are
 *      different.  Used when starting from the beginning of the song.  Obey
 *      the MIDI time clock.
 *
 * Issue #76:
 *
 *      Somehow auto_stop() was placed after auto_play().  Brain fart?
 */

void
player::midi_start ()
{
    (void) auto_stop();
    (void) auto_play();
    m_clock_info.clock_start();
}

/**
 * EVENT_MIDI_CONTINUE:
 *
 *      MIDI continue: start from current position.  Sent immediately
 *      after EVENT_MIDI_SONG_POS, and is used for starting from other than
 *      beginning of the song, or for starting from previous location at
 *      EVENT_MIDI_STOP. Again, converted to Kepler34 mode of setting the
 *      playback mode to Live mode.
 */

void
player::midi_continue ()
{
    m_clock_info.clock_continue(tick());
    (void) auto_pause(); (void) auto_play();
}

/**
 * EVENT_MIDI_STOP:
 *
 *      A master stops the slave simultaneously by sending a MIDI Stop
 *      message. The master may then continue to send MIDI Clocks at the rate
 *      of its tempo, but the slave should ignore these, and not advance its
 *      song position.
 *
 *      Do nothing, just let the system pause.  Since we're not getting ticks
 *      after the stop, the song won't advance when start is received, we'll
 *      reset the position. Or, when continue is received, we won't reset the
 *      position.  We do an inner_stop(); the m_midiclockpos member holds the
 *      stop position in case the next event is "continue".  This feature is
 *      not in Kepler34.
 */

void
player::midi_stop ()
{
    all_notes_off();
    m_clock_info.clock_stop(tick());
    (void) auto_stop();
}

/**
 * EVENT_MIDI_CLOCK:
 *
 *      MIDI beat clock (MIDI timing clock or simply MIDI clock) is a clock
 *      signal broadcast via MIDI to ensure that MIDI-enabled devices stay in
 *      synchronization. It is not MIDI timecode.  Unlike MIDI timecode, MIDI
 *      beat clock is tempo-dependent. Clock events are sent at a rate of 24
 *      ppqn (pulses per quarter note). Those pulses maintain a synchronized
 *      tempo for synthesizers that have BPM-dependent voices and for
 *      arpeggiator synchronization.  Location information can be specified
 *      using MIDI Song Position Pointer.  Many simple MIDI devices ignore
 *      this message.
 */

void
player::midi_clock ()
{
    m_clock_info.clock_increment();
}

/**
 * EVENT_MIDI_SONG_POS:
 *
 *      MIDI song position pointer message tells a MIDI device to cue to a
 *      point in the MIDI track to be ready to play.  This message consists
 *      of three bytes of data. The first byte, the status byte, is 0xF2 to
 *      flag a song position pointer message. Two bytes follow the status
 *      byte.  These two bytes are combined in a 14-bit value to show the
 *      position in the song to cue to. The top bit of each of the two bytes
 *      is not used.  Thus, the value of the position to cue to is between
 *      0x0000 and 0x3FFF. The position represents the MIDI beat, where a
 *      track always starts on beat zero and each beat is a 16th note.
 *      Thus, the device will cue to a specific 16th note.  Also see the
 *      combine_bytes() function.
 */

void
player::midi_song_pos (const event & ev)
{
    midi::byte d0, d1;
    ev.get_data(d0, d1);
    m_clock_info.midiclockpos(combine_bytes(d0, d1));
}

/**
 * EVENT_MIDI_SYSEX:
 *
 *      These messages are system-wide messages.  We filter system-wide
 *      messages.  If the master MIDI buss is dumping, set the timestamp of
 *      the event and stream it on the track.  Otherwise, use the event
 *      data to control the trackr, if it is valid for that action.
 *
 *      "Dumping" is set when a seqedit window is open and the user has
 *      clicked the "record MIDI" or "thru MIDI" button.  In this case, if
 *      seq32 support is in force, dump to it, else stream the event, with
 *      possibly multiple tracks set.  Otherwise, handle an incoming MIDI
 *      control event.
 *
 *      Also available (but macroed out) is Stazed's parse_sysex() function.
 *      It seems specific to certain Yamaha devices, but might prove useful
 *      later.
 *
 *      Not sure what to do with this code, so we just show the data if
 *      allowed to.  We would need to add back the non-buss version of the
 *      various sysex() functions.
 */

void
player::midi_sysex (const event & ev)
{
     m_master_bus->sysex(/*port/buss number */ 0, &ev);     // TODO
}

/**
 *  Encapsulates a series of calls used in mainwnd.  We've reversed the
 *  start() and start_jack() calls so that JACK is started first, to match all
 *  of the other use-cases for playing that we've found in the code.  Note
 *  that the complementary function, stop_playing(), is an inline function
 *  defined in the header file.
 *
 *  The player::start() function passes its boolean flag to
 *  player::inner_start(), which sets the playback mode to that flag; if
 *  that flag is false, that turns off "song" mode.  So that explains why
 *  mute/unmute is disabled.
 *
 * Playback use cases:
 *
 *      These use cases are meant to apply to either a Seq32 or a regular build
 *      of Sequencer64, eventually.  Currently, the regular build does not have
 *      a concept of a "global" perform song-mode flag.
 *
 *      -#  mainwnd.
 *          -#  Play.  If the perform song-mode is "Song", then use that mode.
 *              Otherwise, use "Live" mode.
 *          -#  Stop.  This action is modeless here.  In ALSA, it will cause
 *              a rewind (but currently seqroll doesn't rewind until Play is
 *              clicked, a minor bug).
 *          -#  Pause.  Same processing as Play or Stop, depending on current
 *              status.  When stopping, the progress bars in seqroll and
 *              perfroll remain at their current point.
 *      -#  perfedit.
 *          -#  Play.  Override the current perform song-mode to use "Song".
 *          -#  Stop.  Revert the perfedit setting, in case play is restarted
 *              or resumed via mainwnd.
 *          -#  Pause.  Same processing as Play or Stop, depending on current
 *              status.
 *       -# ALSA versus JACK.  One issue here is that, if JACK isn't "running"
 *          at all (i.e. we are in ALSA mode), then we cannot be JACK Master.
 *
 *  Helgrind shows a read/write race condition in m_start_from_perfedit
 *  bewteen jack_transport_callback() and start_playing() here.  Is inline
 *  function access of a boolean atomic?
 *
 *  song_mode() indicates if the caller wants to start the playback in Song
 *  mode (sometimes erroneously referred to as "JACK mode").  In the seq32
 *  code at GitHub, this flag was identical to the "global_jack_start_mode"
 *  flag, which is true for Song mode, and false for Live mode.  False
 *  disables Song mode, and is the default, which matches seq24.  Generally,
 *  we pass true in this parameter if we're starting playback from the
 *  perfedit window.  It alters the m_start_from_perfedit member, not the
 *  m_song_start_mode member (which replaces the global flag now).
 */

void
player::start_playing ()
{
    if (is_jack_master() && ! m_dont_reset_ticks)       // ca 2021-05-20
        position_jack(false, 0);

    start_jack();
    start();
}

/**
 *  Pause playback, so that progress bars stay where they are, and playback
 *  always resumes where it left off, at least in ALSA mode, which doesn't
 *  have to worry about being a "slave".
 *
 *  Currently almost the same as stop_playing(), but expanded as noted in the
 *  comments so that we ultimately have more granular control over what
 *  happens.  We're researching the whole track of stopping and starting,
 *  and it can be tricky to make correct changes.
 *
 *  We still need to make restarting pick up at the same place in ALSA mode;
 *  in JACK mode, JACK transport takes care of that feature.
 *
 *  User layk noted this call, and it makes sense to not do this here, since it
 *  is unknown at this point what the actual status is.  Note that we STILL
 *  need to FOLLOW UP on calls to pause_playing() and stop_playing() in
 *  perfedit, mainwnd, etc.
 *
 *      is_pattern_playing(false);
 *
 *  But what about is_running()?
 *
 * \param songmode
 *      Indicates that, if resuming play, it should play in Song mode (true)
 *      or Live mode (false).  See the comments for the start_playing()
 *      function.
 */

void
player::pause_playing ()
{
    m_dont_reset_ticks = true;
    is_running(! is_running());
    stop_jack();
    if (! is_jack_running())
        m_clock_info.usemidiclock(false);

    reset_tracks(true);                      /* don't reset "last-tick"  */
}

/**
 *  Encapsulates a series of calls used in mainwnd.  Stops playback, turns off
 *  the (new) m_dont_reset_ticks flag, and set the "is-pattern-playing" flag
 *  to false.  With stop, reset the start-tick to either the left-tick or the
 *  0th tick (to be determined, currently resets to 0).  If looping, act like
 *  pause_playing(), but allow reset to the left tick (as opposed to 0).
 */

void
player::stop_playing ()
{
    stop_jack();
    stop();
    m_dont_reset_ticks = false;
}

bool
player::auto_play ()
{
    bool onekey = false;                /* keys().start() == keys().stop(); */
    bool result = false;                /* was isplaying                    */
    if (onekey)
    {
        if (is_running())
        {
            (void) stop_playing();
        }
        else
        {
            start_playing();
            result = true;
        }
    }
    else if (! is_running())
    {
        start_playing();
        result = true;
    }
    is_pattern_playing(result);
    return result;
}

bool
player::auto_pause ()
{
    bool isplaying = false;
    if (is_running())
    {
        pause_playing();
    }
    else
    {
        start_playing();
        isplaying = true;
    }
    is_pattern_playing(isplaying);
    return true;
}

bool
player::auto_stop ()
{
    stop_playing();
    is_pattern_playing(false);
    return true;
}

bool
player::delay_stop ()
{
    return auto_stop();

    /*
     * Not needed in this base class.
     *
     * xpc::millisleep(c_delay_stop_ms);
     */
}

/**
 *  Starts the playing of all the patterns/tracks.  This function just runs
 *  down the list of tracks and has them dump their events.  It skips
 *  tracks that have no playable MIDI events.
 *
 *  Note how often the "sp" (track) pointer was used.  It was worth
 *  offloading all these calls to a new track function.  Hence the new
 *  track::play_queue() function.
 *
 *  This function is called twice in a row with the same tick value, causing
 *  notes to be played twice. This happens because JACK "ticks" are 10 times
 *  as fast as MIDI ticks, and the conversion can result in the same MIDI tick
 *  value consecutively, especially at lower PPQN.  However, it also can play
 *  notes twice when the tick changes by a small amount.  Not yet sure what to
 *  do about this.
 *
 * \param tick
 *      Provides the tick at which to start playing.  This value is also
 *      copied to m_tick.
 */

bool
player::play (midi::pulse tick)
{
    if (tick != transportinfo().tick() || tick == 0)    /* avoid replays    */
    {
        bool songmode = song_mode();
        set_tick(tick);
        for (const auto & trk : track_list().tracks())
        {
            if (trk)
                trk->play_queue(tick, songmode, resume_note_ons());
            else
                append_error_message("play() on null track");
        }
        m_master_bus->flush();                          /* flush MIDI buss  */
    }
    return true;
}

/**
 *  For all active patterns/tracks, turn off its playing notes.
 *  Then flush the master MIDI buss.
 */

void
player::all_notes_off ()
{
    if (m_master_bus)
        m_master_bus->flush();                      /* flush MIDI buss  */
}

/**
 *  Similar to all_notes_off(), but also sends Note Off events directly to the
 *  active busses.  Adapted from Oli Kester's Kepler34 project.
 */

bool
player::panic ()
{
    bool result = bool(m_master_bus);
    stop_playing();
    inner_stop();                                   /* force inner stop     */
    transportinfo().tick(0);
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Other handling
 * -------------------------------------------------------------------------
 */

/**
 *  If the given track is active, then it is toggled as per the current
 *  value of control-status.  If control-status is
 *  automation::ctrlstatus::queue, then the track's toggle_queued()
 *  function is called.  This is the "mod queue" implementation.
 *
 *  Otherwise, if it is automation::ctrlstatus::replace, then the status is
 *  unset, and all tracks are turned off.  Then the track's
 *  toggle-playing() function is called, which should turn it back on.  This is
 *  the "mod replace" implementation; it is like a Solo.  But can it be undone?
 *
 *  This function is called in loop_control() to implement a toggling of the
 *  track of the pattern slot in the current screen-set that is represented
 *  by the keystroke.
 *
 *  One-shots are allowed only if we are not playing this track.
 *
 * \param seq
 *      The track number of the track to be potentially toggled.
 *      This value must be a valid and active track number. If in
 *      queued-replace mode, and if this pattern number is different from the
 *      currently-stored number (m_queued_replace_slot), then we clear the
 *      currently stored set of patterns and set new stored patterns.
 */

bool
player::track_playing_toggle (track::number trkno)
{
    track::pointer tp = get_track(trkno);
    bool result = bool(tp);
    if (result)
    {
        tp->toggle_playing(tick(), resume_note_ons());   /* kepler34 */
    }
    return result;
}

/**
 *  Changes the play-state of the given track.  This does not cause a modify
 *  action.
 *
 * \param trkno
 *      The number of the track to be turned on or off.
 *
 * \param on
 *      The state (true = armed) to which to set the track.
 */

bool
player::track_playing_change (track::number trkno, bool on)
{
    track::pointer tp = get_track(trkno);
    bool result = bool(tp);
    if (result)
        tp->track_playing_change(trkno, on);

    return true;
}

void
player::signal_save ()
{
    stop_playing();
    xpc::signal_for_save();
}

void
player::signal_quit ()
{
    stop_playing();
    xpc::signal_for_exit();
}

/*
 * -------------------------------------------------------------------------
 *  File handling
 * -------------------------------------------------------------------------
 */

/**
 *  This function calls the midi::read_midi_file() free function, and then
 *  sets the PPQN value.
 *
 * \param fn
 *      Provides the full path file-specification for the MIDI file.
 *
 * \param [out] errmsg
 *      Provides the destination for an error message, if any.
 *
 * \param addtorecent
 *      Not used here, as there is not recent-file support in this potential
 *      base class. However, true by default.
 *
 * \return
 *      Returns true if the function succeeded.  If false is returned, there
 *      should be an error message to display.
 */

bool
player::read_midi_file
(
    const std::string & fn,
    std::string & errmsg,
    bool addtorecent
)
{
    errmsg.clear();
    bool result = midi::read_midi_file(*this, fn, errmsg);
    if (result)
    {
        (void) addtorecent;
    }
    return result;
}

/**
 *  This function calls the midi::write_midi_file() free function.
 *
 * \param fn
 *      Provides the full path file-specification for the MIDI file.
 *
 * \param [out] errmsg
 *      Provides the destination for an error message, if any.
 *
 * \param eventsonly
 *      If true, only events existing in the tracks are written.
 *      Of course, the MIDI header is still written.
 *
 * \return
 *      Returns true if the function succeeded.  If false is returned, there
 *      should be an error message to display.
 */

bool
player::write_midi_file
(
    const std::string & fn,
    std::string & errmsg,
    bool eventsonly
)
{
    errmsg.clear();
    bool result = midi::write_midi_file(*this, fn, errmsg, eventsonly);
    if (result)
    {
        // to do?
    }
    return result;
}

}           // namespace midi

/*
 * player.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

