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
 * \file          track.cpp
 *
 *  This module declares a class for holding and managing MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2022-12-05
 * \license       GNU GPLv2 or above
 *
 *  This class is important when writing the MIDI and track data out to a
 *  MIDI file.  The data handled here are specific to a single
 *  sequence/pattern/track.
 */

#include "c_macros.h"                   /* errprint() macro                 */
#include "midi/calculations.hpp"        /* midi::log2_power_of_2()          */
#include "midi/eventlist.hpp"           /* midi::eventlist class            */
#include "midi/masterbus.hpp"           /* midi::masterbus class            */
#include "midi/player.hpp"              /* midi::player class               */
#include "midi/track.hpp"               /* midi::track class                */
#include "xpc/timing.hpp"               /* xpc::microsleep(), etc.          */

namespace midi
{

/**
 *  Default constructor.
 */

track::track (number tn) :
    m_parent            (nullptr),          /* set when track installed     */
    m_data              (),                 /* holds events and raw bytes   */
    m_info              (),                 /* holds various "signatures"   */
    m_mutex             (),
    m_track_number      (tn),
    m_active            (false),
    m_notes_on          (0),
    m_master_bus        (nullptr),
    m_playing_notes     (),                 /* an array                     */
    m_armed             (false),
    m_recording         (false),
    m_recording_type    (record::normal),
    m_is_dirty          (false),
    m_modified          (false),
    m_length            (0),
    m_measures          (0),
    m_unit_measure      (0),
    m_beats_per_bar     (4),
    m_beat_width        (4),
    m_last_tick         (0),
    m_note_on_velocity  (96),
    m_note_off_velocity (0),
    m_nominal_bus       (0),
    m_true_bus          (0),
    m_midi_channel      (0),
    m_free_channel      (false)
{
    // copy_events(evlist);
}

/**
 *  Sets the playing state of this track.  When playing, and the sequencer
 *  is running, notes get dumped to the ALSA buffers.
 *
 *  If we're turning play on, we now (2021-04-27 issue #49) turn song-mute
 *  off, so that the pattern will not get turned off when playback starts.
 *  This covers the case where the user enables and then disables a mute
 *  group, which sets song-mute to true on all tracks.
 *
 * \param p
 *      Provides the playing status to set.  True means to turn on the
 *      playing, false means to turn it off, and turn off any notes still
 *      playing.
 */

bool
track::set_armed (bool p)
{
    xpc::automutex locker(m_mutex);
    bool result = p != armed();
    if (result)
    {
        armed(p);
        if (! p)
            off_playing_notes();

        set_dirty();
    }
    return result;
}

/**
 *  This function sets m_notes_on to 0, only if the recording status has
 *  changed.  It is called by set_recording().  We probably need to explicitly
 *  turn off all playing notes; not sure yet.
 *
 *  Like performer::set_sequence_input(), but it uses the internal recording
 *  status directly, rather than getting it from seqedit.
 *
 *  Except if already Thru and trying to turn recording (input) off, set input
 *  on here no matter what, because even if m_thru, input could have been
 *  replaced in another track.
 *
 * \param recordon
 *      Provides the desired status to set recording.
 *
 * \param r
 *      Indicates the style of recording, normal, quantized, overwrite etc.
 *
 * \param toggle
 *      If true, ignore the first parameter and toggle the flag.  The default
 *      value is false.
 */

bool
track::set_recording (bool recordon, record r, bool toggle)
{
    xpc::automutex locker(m_mutex);
    if (toggle)
        recordon = ! m_recording;

    bool result = toggle || recordon != m_recording;
#if defined USE_MASTER_BUS
    if (result)
        result = master_bus()->set_sequence_input(recordon, this);
#endif

    if (result)
    {
        m_notes_on = 0;                 /* reset the step-edit note counter */
        if (recordon)
        {
            m_recording = true;
        }
        else
        {
            // m_recording = m_quantized_recording = m_tightened_recording = false;
            m_recording = false;
        }
        if (r != record::normal)
        {
            // provide this in the derived class.
        }
        set_dirty();
    }
    return result;
}

/**
 *  Sets the state of MIDI Thru.
 *
 * \param thru_active
 *      Provides the desired status to set the through state.
 *
 * \param toggle
 *      If true, ignore the first parameter and toggle the flag.  The default
 *      value is false.
 */

bool
track::set_thru (bool thruon, bool toggle)
{
    xpc::automutex locker(m_mutex);
    if (toggle)
        thruon = ! m_thru;

    bool result = thruon != m_thru;
    if (result)
    {
        /*
         * Except if already recording and trying to turn Thru (hence input)
         * off, set input to here no matter what, because even in m_recording,
         * input could have been replaced in another track.
         */

#if defined USE_MASTER_BUS
         if (! m_recording)
            result = master_bus()->set_sequence_input(thruon, this);
#endif

        if (result)
            m_thru = thruon;
    }
    return result;
}

/**
 *  Plays a note from the piano roll on the main bus on the master MIDI
 *  buss.  It flushes a note to the midibus to preview its sound, used by
 *  the virtual piano.
 *
 * \threadsafe
 *
 * \param note
 *      The note to play.  It is not checked for range validity, for the sake
 *      of speed.
 */

void
track::play_note_on (int note)
{
    xpc::automutex locker(m_mutex);
    midi::byte channel = 0;                 // midi_channel(e)
    midi::byte nvalue = midi::byte(note);
    event e(0, midi::status::note_on, channel, nvalue, m_note_on_velocity);
    // TODO
    // master_bus()->play_and_flush(m_true_bus, &e, midi_channel(e));
}

/**
 *  Turns off a note from the piano roll on the main bus on the master MIDI
 *  buss.
 *
 * \threadsafe
 *
 * \param note
 *      The note to turn off.  It is not checked for range validity, for the
 *      sake of speed.
 */

void
track::play_note_off (int note)
{
    xpc::automutex locker(m_mutex);
    midi::byte channel = 0;                 // midi_channel(e)
    midi::byte nvalue = midi::byte(note);
    event e(0, midi::status::note_off, channel, nvalue, m_note_off_velocity);
    // TODO
    // master_bus()->play_and_flush(m_true_bus, &e, midi_channel(e));
}

/**
 *  A simple version of toggle_playing().
 *
 * \return
 *      Returns true if playing.
 */

bool
track::toggle_playing ()
{
    return toggle_playing(parent()->tick(), parent()->resume_note_ons());
}

/**
 *  Toggles the playing status of this track.
 *
 * \param tick
 *      The position from which to resume Note Ons, if appplicable. Resuming
 *      is a song-playback/song-recording feature.
 *
 * \param resumenoteons
 *      A song-recording option. Not supported in this base class.
 */

bool
track::toggle_playing (midi::pulse tick, bool resumenoteons)
{
    set_armed(! armed());
    if (armed() && resumenoteons)
        resume_note_ons(tick);

    // off_from_snap(false);    // related to queuing
    return armed();
}

/**
 *  Turn the playing of a sequence on or off.  Used for the implementation of
 *  sequence_playing_on() and sequence_playing_off().
 *
 * \param on
 *      True if the sequence is to be turned on, false if it is to be turned
 *      off.
 *
 * \param qinprogress
 *      Indicates if queuing is in progress.  This status is obtained from the
 *      performer.  Not supported in this base class.
 */

void
track::track_playing_change (bool on, bool qinprogress)
{
    (void) qinprogress;
    set_armed(on);
}

/**
 *  Sends a note-off event for all active notes.  This function does not
 *  bother checking if m_master_bus is a null pointer.
 *
 * \threadsafe
 */

void
track::off_playing_notes ()
{
    xpc::automutex locker(m_mutex);
    int channel = 0;    // TODO: free_channel() ? 0 : seq_midi_channel() ;
    event e(0, midi::status::note_off, channel, 0, 0);
    for (int x = 0; x < c_notes_count; ++x)
    {
        while (m_playing_notes[x] > 0)
        {
            e.set_data(x);
            // TODO
            // master_bus()->play(m_true_bus, &e, channel);
            --m_playing_notes[x];
        }
    }
    // TODO
    // if (not_nullptr(master_bus()))
    //     master_bus()->flush();
}

/**
 *  Provides a helper function simplify and speed up performer ::
 *  reset_tracks().  In Live mode, the user controls playback, while in
 *  Song mode, the performance/song editor controls playback.  This function
 *  used to be called "reset()".
 *
 * \param song_mode
 *      Defaults to false, not supported in this base class.
 */

void
track::stop (bool songmode)
{
    bool state = armed();
    off_playing_notes();
    zero_markers();                         /* sets the "last-tick" value   */
    set_armed(songmode ? false : state);
}

/**
 *  A pause version of stop().  It still includes the note-shutoff capability
 *  to prevent notes from lingering.  Note that we do not call set_arm(false);
 *  it disarms the track, which we do not want upon pausing.
 *
 * \param song_mode
 *      Set to true if song mode is in force.  This setting corresponds to
 *      performer::playback::song.  False (the default) corresponds to
 *      performer::playback::live.
 */

void
track::pause (bool song_mode)
{
    bool state = armed();
    off_playing_notes();
    if (! song_mode)
        set_armed(state);
}

/**
 *
 * \param tick
 *      Provides the current active pulse position, the tick/pulse from which
 *      to start playing.
 *
 * \param playbackmode
 *      If true, we are in Song mode.  Otherwise, Live mode. This defaults to
 *      false, and is not supported in this base/basic class.
 *
 * \param resumenoteons
 *      Indicates if we are to resume Note Ons.  Used by performer::play().
 *      Defaults to false, and is not supported here.
 */

void
track::play_queue (midi::pulse tick, bool playbackmode, bool resumenoteons)
{
#if defined USE_QUEUEING_SUPPORT
    if (check_queued_tick(tick))
    {
        play(get_queued_tick() - 1, playbackmode, resumenoteons);
        (void) toggle_playing(tick, resumenoteons);
        (void) parent()->set_ctrl_status
        (
            automation::action::off, automation::ctrlstatus::queue
        );
    }
    if (check_one_shot_tick(tick))
    {
        play(one_shot_tick() - 1, playbackmode, resumenoteons);
        (void) toggle_playing(tick, resumenoteons);
        (void) toggle_queued(); /* queue it to mute it again after one play */
        // (void) parent()->set_ctrl_status
        // (
        //     automation::action::off, automation::ctrlstatus::oneshot
        // );
    }
#endif
    if (is_metro_track())
    {
        live_play(tick);
    }
    else
    {
        play(tick, playbackmode, resumenoteons);
    }
}

/**
 *  This function plays without supporting song-mode, triggers, transposing,
 *  resuming notes, loop count, meta events, and song recording.  It is
 *  meant to be used for a metronome pattern.
 *
 *  Do we want to support tempo in a metronome pattern?
 *
 *  One issue is in removing the metronome, and we end up with a null event,
 *  causing a segfault.  We should just mute the metronome.
 */

void
track::live_play (midi::pulse tick)
{
    xpc::automutex locker(m_mutex);
    midi::pulse start_tick = m_last_tick;
    midi::pulse end_tick = tick;              /* ditto                        */
    if (armed())                            /* play notes in the frame      */
    {
        midi::pulse len = length() > 0 ?
            length() : parent()->get_ppqn() ;

        midi::pulse start_tick_offset = start_tick + len;
        midi::pulse end_tick_offset = end_tick + len;
        midi::pulse times_played = m_last_tick / len;
        midi::pulse offset_base = times_played * len;
#if defined USE_LOOP_COUNT

        // This code will go into the derived class.

        if (loop_count_max() > 0)
        {
            if (times_played >= loop_count_max())
            {
#if defined SEQ66_METRO_COUNT_IN_ENABLED
                if (is_metro_seq())                 /* count-in is complete */
                    m_parent->finish_count_in();
#endif
                return;
            }
        }
#endif

        auto e = events().begin();
        while (e != events().end())
        {
            event & er = eventlist::dref(e);
            midi::pulse stamp = er.timestamp() + offset_base;
            if (stamp >= start_tick_offset && stamp <= end_tick_offset)
            {
#if defined SUPPORT_TEMPO_IN_LIVE_PLAY
                if (er.is_tempo())
                {
                    parent()->beats_per_minute(er.tempo());
                }
#endif
                put_event_on_bus(er);               /* frame still going    */
            }
            else if (stamp > end_tick_offset)
                break;                              /* frame is done        */

            ++e;                                    /* go to next event     */
            if (e == events().end())                /* did we hit the end ? */
            {
                e = events().begin();               /* yes, start over      */
                offset_base += len;                 /* for another go at it */
                (void) xpc::microsleep(1);
            }
        }
    }
    m_last_tick = end_tick + 1;                     /* for next frame       */
}

/**
 *  The play() function dumps notes starting from the given tick, and it
 *  pre-buffers ahead.  This function is called by the sequencer thread in
 *  player.  The tick comes in as global tick.  It turns the track off
 *  after we play in this frame.
 *
 * \note
 *      With pause support, the progress bar for the pattern/track editor
 *      does what we want:  pause with the pause button, and rewind with the
 *      stop button.  Works with JACK, with issues, but we'd like to have
 *      the stop button do a rewind in JACK, too.
 *
 * \param tick
 *      Provides the current end-tick value.  The tick comes in as a global
 *      tick.
 *
 * \param playback_mode
 *      Defaults to false and is not supported in this basic/base class.
 *
 * \param resumenoteons
 *      Defaults to false and is not supported in this basic/base class.
 *
 * \threadsafe
 */

void
track::play
(
    midi::pulse tick,
    bool /* playback_mode */,
    bool /* resumenoteons */
)
{
    xpc::automutex locker(m_mutex);
    midi::pulse start_tick = m_last_tick;
    if (armed())                                    /* play notes in frame  */
    {
        midi::pulse len = length() > 0 ?
            length() : parent()->get_ppqn() ;

        midi::pulse offset = len;
        midi::pulse start_tick_offset = start_tick + offset;
        midi::pulse end_tick_offset = tick + offset;
        midi::pulse times_played = m_last_tick / len;
        midi::pulse offset_base = times_played * len;
#if defined USE_LOOP_COUNT

        // This code will go into the derived class.

        if (loop_count_max() > 0)
        {
            if (times_played >= loop_count_max())
            {
#if defined SEQ66_METRO_COUNT_IN_ENABLED
                if (is_metro_track())               /* count-in is complete */
                    m_parent->finish_count_in();
#endif
                return;
            }
        }
#endif

        auto e = events().begin();
        while (e != events().end())
        {
            event & er = eventlist::dref(e);
            midi::pulse ts = er.timestamp();
            midi::pulse stamp = ts + offset_base;
            if (stamp >= start_tick_offset && stamp <= end_tick_offset)
            {
                if (er.is_tempo())
                {
                    parent()->beats_per_minute(er.tempo());
                }
                else if (! er.is_ex_data())
                {
                    put_event_on_bus(er);       /* frame still going    */
                }
            }
            else if (stamp > end_tick_offset)
                break;                              /* frame is done        */

            ++e;                                    /* go to next event     */
            if (e == events().end())                /* did we hit the end ? */
            {
                e = events().begin();               /* yes, start over      */
                offset_base += len;                 /* for another go at it */

                /*
                 * Putting this sleep here doesn't reduce the total CPU load,
                 * but it does prevent one CPU from being hammered at 100%.
                 * millisleep(1) made the live-grid progress bar jittery when
                 * unmuting shorter patterns, which play() relentlessly.
                 */

                (void) xpc::microsleep(1);
            }
        }
    }
    m_last_tick = tick + 1;                         /* for next frame       */
}

void
track::track_name (const std::string & n)
{
    bool change = n != info().track_name();
    info().track_name(n);
    if (change)
        set_dirty();
}

/**
 *  Sets the "parent" of this track, so that it can get some extra
 *  information about the performance.  Remember that m_parent is not at all
 *  owned by the track.  We just don't want to do all the work necessary to
 *  make it a reference, at this time.
 *
 *  Add the buss override, if specified.  We can't set it until after
 *  assigning the master MIDI buss, otherwise we get a segfault.
 *
 * Issue #88:
 *
 *      If the time signature is something like 4/16, then the beat length
 *      will be much less than PPQN.  It will be the quarter note size / 4.
 *      Also check out pulses_to_midi_measures().
 *
 * \param p
 *      A pointer to the parent, assigned only if not already assigned.
 *
 * \param sorting
 *      If enabled (the default is toggle::off), then the event are sorted.
 */

void
track::set_parent (player * p, lib66::toggle sorting)
{
    if (not_nullptr(p))
    {
        midi::pulse ppnote = 4 * p->get_ppqn() / beat_width();
        midi::pulse barlength = ppnote * beats_per_bar();
        m_parent = p;
        manufacturer_id(p->manufacturer_id());
#if defined USE_MASTER_BUS
        master_midi_bus(p->master_bus());
#endif
        if (sorting == lib66::toggle::on)
            sort_events();                  /* sort the events now          */

        set_length();                       /* final verify_and_link()      */
        if (length() < barlength)           /* pad sequence to a measure    */
            set_length(barlength, false);

        (void) midi_bus(m_nominal_bus);
        // beats_per_bar(p->get_beats_per_bar());
        // beat_width(p->get_beat_width());
        unmodify();
    }
}

/**
 * \setter m_master_bus
 *
 * \threadsafe
 *
 * \param mmb
 *      Provides a pointer to the master MIDI buss for this sequence.  This
 *      should be a reference, but isn't, nor is it checked.
 */

bool
track::master_midi_bus (const midi::masterbus * mmb)
{
    xpc::automutex locker(m_mutex);
    m_master_bus = const_cast<midi::masterbus *>(mmb);
    return not_nullptr(mmb);
}

/**
 *  Sets the MIDI buss/port number to dump MIDI data to.
 *
 *  Override this function in a derived class in order to provide port-mapping.
 *
 * \threadsafe
 *
 * \param nominalbus
 *      The MIDI buss to set as the buss number for this sequence.  Also
 *      called the "MIDI port" number.  This number is not necessarily the
 *      true system bus number.  However, this parameter is only the real
 *      buss number in the base class.
 *
 * \param user_change
 *      If true (the default value is false), the user has decided to change
 *      this value, and we might need to modify the performer's dirty flag, so
 *      that the user gets prompted for a change.  This is a response to
 *      GitHub issue #47, where buss changes do not cause a prompt to save the
 *      sequence.
 *
 * \return
 *      Returns true if a change was made.
 */

bool
track::midi_bus (midi::bussbyte nominalbus, bool user_change)
{
    xpc::automutex locker(m_mutex);
    bool result = nominalbus != m_nominal_bus && is_good_buss(nominalbus);
    if (result)
    {
        off_playing_notes();                /* off notes except initial     */
        m_nominal_bus = nominalbus;
        if (not_nullptr(parent()))
        {
            m_true_bus = nominalbus;
        }
        else
            m_true_bus = null_buss();       /* provides an invalid value    */

        if (user_change)
            modify();                       /* no easy way to undo this     */

        set_dirty();                        /* this is for display updating */
    }
    return result;
}

/**
 *  Sets the m_midi_channel number, which is the output channel for this
 *  sequence.
 *
 * \threadsafe
 *
 * \param ch
 *      The MIDI channel to set as the output channel number for this
 *      sequence.  This value can range from 0 to 15, but c_midichannel_null
 *      equal to 0x80 means we are just setting the "no-channel" status.
 *
 * \param user_change
 *      If true (the default value is false), the user has decided to change
 *      this value, and we might need to modify the player's dirty flag, so
 *      that the user gets prompted for a change,  This is a response to
 *      GitHub issue #47, where channel changes do not cause a prompt to save
 *      the sequence.
 */

bool
track::midi_channel (midi::byte ch, bool user_change)
{
    xpc::automutex locker(m_mutex);
    bool result = ch != m_midi_channel;
    if (result)
        result = is_valid_channel(ch);      /* 0 to 15 or null_channel()    */

    if (result)
    {
        off_playing_notes();
        m_free_channel = is_null_channel(ch);
        m_midi_channel = ch;                /* if (! m_free_channel)        */
        if (user_change)
            modify();                       /* no easy way to undo this     */

        set_dirty();                        /* this is for display updating */
    }
    return result;
}

/**
 *  Sets the length (m_length). Useful when when the user changes
 *  beats/bar or the sequence length in measures.  This function is also
 *  called when reading a MIDI file.
 *
 *  That function calculates the length in ticks:
 *
\verbatim
    L = M x B x 4 x P / W
        L == length (ticks or pulses)
        M == number of measures
        B == beats per measure
        P == pulses per quarter-note
        W == beat width in beats per measure
\endverbatim
 *
 *  For our "b4uacuse" MIDI file, M can be about 100 measures, B is 4,
 *  P can be 192 (but we want to support higher values), and W is 4.
 *  So L = 100 * 4 * 4 * 192 / 4 = 76800 ticks.  Seems small.
 *
 * Issue #88:
 *
 *      If the time signature is something like 4/16, then the pattern length
 *      will be much less than PPQN * 4.   This affects the set_parent()
 *      function.
 *
 * \threadsafe
 *
 * \param len
 *      The length value to be set.  If it is smaller than ppqn/4, then
 *      it is set to that value, unless it is zero, in which case m_length is
 *      used and does not change.  It also sets the length value for the
 *      sequence's triggers.
 *
 * \param verify
 *      Defaults to true.  If true, verify_and_link() is
 *      called.  Otherwise, it is not, and the caller should call this
 *      function with the default value after reading all the events.
 *
 * \return
 *      Returns true if the length value actually changed.
 */

bool
track::set_length (midi::pulse len, bool verify)
{
    xpc::automutex locker(m_mutex);
    bool result = len != m_length;
    if (result)
    {
        bool was_playing = armed();             /* was it armed?            */
        set_armed(false);                       /* mute the pattern         */
        if (len > 0)
        {
            if (len < midi::pulse(parent()->get_ppqn() / 4))    // beat width!!!
                len = midi::pulse(parent()->get_ppqn() / 4);

            m_length = len;
            result = true;
        }
        else
            len = length();

        events().length(len);
        if (verify)
            verify_and_link();

        if (was_playing)                        /* start up and refresh     */
            set_armed(true);
    }
    return result;
}

/**
 *  Calculates and sets u = 4BP/W, where u is m_unit_measure, B is the
 *  beats/bar, P is the PPQN, and W is the beat-width. When any of these
 *  quantities change, we need to recalculate.
 *
 * \param reset
 *      If true (the default is false), make the calculateion anyway.
 *
 * \return
 *      Returns the size of a measure.
 */

midi::pulse
track::unit_measure (bool reset) const
{
    xpc::automutex locker(m_mutex);
    if (m_unit_measure == 0 || reset)
        m_unit_measure = measures_to_ticks();       /* length of 1 measure  */

    return m_unit_measure;
}

/**
 *  A convenience function for calculating the number of ticks in the
 *  given number of measures.
 */

midi::pulse
track::measures_to_ticks (int measures) const
{
    return midi::measures_to_ticks                  /* see "calculations"   */
    (
        int(m_beats_per_bar), int(parent()->get_ppqn()),
        int(m_beat_width), measures
    );
}

void
track::set_measures (int measures)
{
    bool modded = set_length(measures * unit_measure(true));
    if (modded)
        modify();
}

/**
 *  Encapsulates a calculation needed in user interfaces.
 *
 *  Let's say we have a measure of notes in a 4/4 sequence. If we want to fit
 *  it into a 3/4 measure, the number of ticks is 3/4th of the original, and
 *  we'd have to rescale the notes to that new number.  If we leave the notes
 *  alone, then the measure-count increments, and in playback an space of
 *  silence is introduced.  Better to avoid changing the numerator.
 *
 *  If we change 4/4 to 4/8, then playback slows down by half.  Be aware of
 *  this feature.
 *
 * \param newlength
 *      If 0 (the default), then the current pattern length is used in the
 *      calculation of measures.  Otherwise, this parameter is used, and is
 *      useful in the process of changing the number of measures in the
 *      fix_pattern() function.
 *
 * \return
 *      Returns the whole number of measure in the specified length of the
 *      sequence.  Essentially rounds up if there is some leftover ticks.
 */

int
track::get_measures (midi::pulse newlength) const
{
    midi::pulse um = unit_measure();
    midi::pulse len = newlength > 0 ? newlength : length() ;
    int measures = int(len / um);
    if (len % int(um) != 0)
        ++measures;

    return measures;
}

int
track::get_measures () const
{
    m_measures = get_measures(0);
    return m_measures;
}

/**
 *  Calculates the number of measures in the track based on the
 *  unit-measure and the current length, in pulses, of the track.
 *
 * \return
 *      Returns the track length divided by the measure length, roughly.
 *      m_unit_measure is 0.  The lowest valid measure is 1.
 */

int
track::calculate_measures (bool reset) const
{
    midi::pulse um = unit_measure(reset);
    return 1 + (length() - 1) / um;
}

/**
 * \threadsafe
 *
 * \param bpb
 *      The new setting of the beats-per-bar value.
 *
 * \param user_change
 *      If true (default is false), then call the change a modification.
 *      This change can happen at load time, which is not a modification.
 */

void
track::beats_per_bar (int bpb, bool user_change)
{
    xpc::automutex locker(m_mutex);
    bool modded = false;
    if (bpb != int(m_beats_per_bar))
    {
        m_beats_per_bar = bpb;
        if (user_change)
            modded = true;
    }

    int m = get_measures();
    if (m != m_measures)
    {
        m_measures = m;
        if (user_change)
            modded = true;
    }
    if (modded)
        modify();
}

/**
 * \threadsafe
 *
 * \param bw
 *      The new setting of the beat width value.
 *
 * \param user_change
 *      If true (default is false), then call the change a modification.
 *      This change can happen at load time, which is not a modification.
 */

void
track::beat_width (int bw, bool user_change)
{
    xpc::automutex locker(m_mutex);
    bool modded = false;
    if (bw != int(m_beat_width))
    {
        m_beat_width = bw;
        if (user_change)
            modded = true;
    }

    int m = get_measures();
    if (m != m_measures)
    {
        m_measures = m;
        if (user_change)
            modded = true;
    }
    if (modded)
        modify();
}

/**
 *  Takes an event that this track is holding, and places it on the MIDI
 *  buss.  This function does not bother checking if m_master_bus is a null
 *  pointer.
 *
 *  Note that the call to midi_channel() yields the event channel if
 *  free_channel() is true.  Otherwise the global pattern channel is true.
 *
 * \param ev
 *      The event to put on the buss.
 *
 * \threadsafe
 */

void
track::put_event_on_bus (const event & ev)
{
    midi::byte note = ev.get_note();
    bool skip = false;
    if (ev.is_note_on())
    {
        ++m_playing_notes[note];
    }
    else if (ev.is_note_off())
    {
        if (m_playing_notes[note] == 0)
            skip = true;
        else
            --m_playing_notes[note];
    }
    if (! skip)
    {
        event evout;
        evout.prep_for_send(m_parent->tick(), ev);          /* issue #100   */
#if defined USE_MASTER_BUS
        master_bus()->play_and_flush(m_true_bus, &evout, midi_channel(ev));
#endif
    }
}

void
track::set_last_tick (midi::pulse t)
{
    xpc::automutex locker(m_mutex);
    if (midi::is_null_pulse(t))
        t = m_length;

    m_last_tick = t;
}

/**
 *  Adds an event to the internal event list in a sorted manner.  Then it
 *  sets the dirty flag.
 *
 *  Here, we could ignore events not on the sequence's channel, as an option.
 *  We have to be careful because this function can be used in painting events.
 *
 * \threadsafe
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.  Actually, this is true only
 *      in Seq24, we've fixed that behavior for Seq66.
 *
 * \param er
 *      Provide a reference to the event to be added; the event is copied into
 *      the events container.
 *
 * \return
 *      Returns true if the event was added.
 */

bool
track::add_event (const event & er)
{
    xpc::automutex locker(m_mutex);

    /*
     * verify_and_link() sorts, as does add().  So just append().
     *
     * bool result = events().add(er);  // post/auto-sorts by time & rank
     */

    bool result = events().append(er);      /* no-sort insertion of event   */
    if (result)
    {
        verify_and_link();                  /* for proper drawing; sorts    */
        modify(lib66:: notification::yes);  /* notify of changes            */
    }
    return result;
}

/**
 *  An alternative to add_event() that does not sort the events, even if the
 *  event list is implemented by an std::list.  This function is meant mainly
 *  for reading the MIDI file, to save a lot of time.  We could also add a
 *  channel parameter, if the event has a channel.  This reveals that in
 *  midifile and wrkfile, we update the channel setting too many times.
 *  SOMETHING TO INVESTIGATE.
 *
 * \param er
 *      Provide a reference to the event to be added; the event is copied into
 *      the events container.
 *
 * \return
 *      Returns true if the event was appended.
 */

bool
track::append_event (const event & er)
{
    xpc::automutex locker(m_mutex);
    return events().append(er);     /* does *not* sort, too time-consuming */
}

void
track::sort_events ()
{
    xpc::automutex locker(m_mutex);
    events().sort();
}

/**
 *  Verifies that all note-ons have a note-off, and links
 *  note-offs with their note-ons.
 *
 * \threadsafe
 *
 * \param wrap
 *      Optionally (the default is false) wrap when relinking.
 */

void
track::verify_and_link (bool wrap)
{
    xpc::automutex locker(m_mutex);
    events().verify_and_link(length(), wrap);
}

/**
 *  Returns the number of events stored in events().  Note that only playable
 *  events are counted in a track.  If a track class function provides a
 *  mutex, call events().count() instead.
 *
 * \threadsafe
 *
 * \return
 *      Returns events().count().
 */

int
track::event_count () const
{
    xpc::automutex locker(m_mutex);
    return events().count();
}

int
track::note_count () const
{
    xpc::automutex locker(m_mutex);
    return events().note_count();
}

int
track::playable_count () const
{
    xpc::automutex locker(m_mutex);
    return events().playable_count();
}

bool
track::is_playable () const
{
    xpc::automutex locker(m_mutex);
    return events().is_playable();
}

/**
 *  A new function provided so that we can find the minimum and maximum notes
 *  with only one (not two) traversal of the event list.
 *
 * \todo
 *      For efficency, we should calculate this only when the event set
 *      changes, and save the results and return them if good.
 *
 * \threadsafe
 *
 * \param lowest
 *      A reference parameter to return the note with the lowest value.
 *      if there are no notes, then it is set to max_midi_value(), and
 *      false is returned.
 *
 * \param highest
 *      A reference parameter to return the note with the highest value.
 *      if there are no notes, then it is set to 0, and false is returned.
 *
 * \return
 *      If there are no notes or tempo events in the list, then false is
 *      returned, and the results should be disregarded.  If true is returned,
 *      but there are only tempo events, then the low/high range is 0 to 127.
 */

bool
track::minmax_notes (int & lowest, int & highest) // const
{
    xpc::automutex locker(m_mutex);
    bool result = false;
    int low = int(max_midi_value());
    int high = -1;
    for (auto & er : events())
    {
        if (er.is_strict_note())
        {
            if (er.get_note() < low)
            {
                low = er.get_note();
                result = true;
            }
            else if (er.get_note() > high)
            {
                high = er.get_note();
                result = true;
            }
        }
        else if (er.is_tempo())
        {
            midi::byte notebyte = tempo_to_note_value(er.tempo());
            if (notebyte < low)
                low = notebyte;
            else if (notebyte > high)
                high = notebyte;

            result = true;
        }
    }
    lowest = low;
    highest = high;
    return result;
}

}           // namespace midi

/*
 * track.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

