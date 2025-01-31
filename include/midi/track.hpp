#if ! defined RTL66_MIDI_TRACK_HPP
#define RTL66_MIDI_TRACK_HPP

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
 * \file          track.hpp
 *
 *  This module declares a class for managing raw MIDI events in an event list.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2024-05-26
 * \license       GNU GPLv2 or above
 *
 *  This class is meant to hold the bytes that represent MIDI events and other
 *  MIDI data, which can then be dumped to a MIDI file.
 *
 *  A track is something that can be read from a MIDI file and written back to a
 *  MIDI file, handling non-seq66-specific events.
 */

#include <atomic>                       /* std::atomic<bool> for dirtying   */
#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */
#include <string>                       /* std::string class                */

#include "cpp_types.hpp"                /* lib66::notification              */
#include "midi/trackdata.hpp"           /* midi::trackdata event-data class */
#include "midi/trackinfo.hpp"           /* midi::trackinfo parameters class */
#include "xpc/automutex.hpp"            /* xpc::recmutex, automutex         */
#include "util/bytevector.hpp"          /* util::bytevector big-endian data */

namespace midi
{
    class masterbus;
    class player;

/**
 * track:
 *
 *  This class holds a container of MIDI track information.  It adds the
 *  ability to write a track to a file stream.
 *
 *  Note the protected inheritance.  It means that track is implemented in
 *  terms of trackdata and trackinfo, so that their functions can be
 *  called directly, eliminating a goodly number of pass-through functions.
 *  This may evolve as time goes on.
 *
 * trackdata:
 *
 *  Holds the midi::eventlist which contains all the midi::events for this
 *  track. It also provides a midi::bytes vector to use when converting
 *  the events to raw data (and vice versa).
 *
 * trackinfo:
 *
 *  Holds the more common MIDI track parameters in convenient structures.
 *  Includes information on key signature, time signature, etc.
 */

class track
{
    friend class file;
    friend class player;
    friend class trackdata;
    friend class tracklist;

public:

    /**
     *  Provides a more descriptive alias for the track numbers (which
     *  range from 0 to the maximum track number allowed for a given run
     *  of the application.
     */

    using number = int;

    /**
     *  Provides public access to the shared pointer for a track. It cannot
     *  be a unique_ptr<> because m_seq needs to be returned to callers.
     */

    using pointer = std::shared_ptr<track>;

    /**
     *  References to track objects.
     */

    using ref = track &;
    using cref = const track &;

    /**
     *  The recording style, when recording.  Also see the m_recording
     *  boolean. Replaces multiple booleans from the old seq66::sequence.
     */

    enum class record
    {
        normal,
        quantized,
        tightened,
        overwrite,
        oneshot,
        max
    };

private:

    /**
     *  A useful link to the player that is using this track. Obviously
     *  the track does not own the parent.
     */

    midi::player * m_parent;

    /**
     *  This list holds the current track events.  Now inherited or contained
     *  via midi::trackdata, along with a midi::bytes vectory.
     *
     *      eventlist m_events;
     */

    /**
     *  Holds the events and raw-data bytes.
     */

    mutable trackdata m_data;

    /**
     *  Holds stock items of information:
     *
     *      -   Tempo and tempo track number
     *      -   Time signature information
     *      -   Key signature information
     *      -   Track name
     *      -   Track length (pulses)
     *      -   Track channel (or null channel)
     *
     *  TODO: handle multiple time-sigs as in Seq66.
     */

    trackinfo m_info;

    /**
     *  Provides locking for the sequence.  Made mutable for use in
     *  certain locked getter functions. Note that it has a deleted
     *  copy constructor, rendering track uncopyable. Must implement
     *  a partial-copy function.
     */

    mutable xpc::recmutex m_mutex;

    /**
     *  Indicates the track number.  This value is used for ordering the
     *  tracks and indicating their place in a potential grid of tracks.
     */

    number m_track_number;

    /**
     *  Each boolean value in this array is set to true if a sequence is
     *  active, meaning that it will be used to hold some kind of MIDI data,
     *  even if only Meta events.  This array can have "holes" with inactive
     *  sequences, so every sequence needs to be checked before using it.
     *  This flag will be true only if the sequence pointer is not null and if
     *  the sequence potentially contains some MIDI data.
     */

    bool m_active;

    /**
     *  Provides a member to hold the polyphonic step-edit note counter.  We
     *  will never come close to the short limit of 32767.
     */

    short m_notes_on;

    /**
     *  Provides the master MIDI buss which handles the output of the track to
     *  the proper buss and MIDI channel. Obviously the track does not own the
     *  buss.
     */

    midi::masterbus * m_master_bus;

    /**
     *  Provides a "map" for Note On events.  It is used when muting, to shut
     *  off the notes that are playing. The c_notes_count is simply the
     *  maximum number of MIDI notes, 128.  See the midibytes.hpp module.
     */

    unsigned short m_playing_notes[c_notes_count];

    /**
     *  True if sequence playback currently is possible for this sequence.  In
     *  other words, the sequence is armed.
     */

    bool m_armed;

    /**
     *  True if sequence recording currently is in progress for this sequence.
     */

    bool m_recording;
    record m_recording_type;

    /**
     *  True if recording in MIDI-through mode.
     */

    bool m_thru;

    /**
     *  Indicates if the track has been altered and should be redisplayed.
     *  This is a weaker version of the "modified" flag.
     */

    mutable std::atomic<bool> m_is_dirty;

    /**
     *  Indicates if the track has been modified (by the user via editing or
     *  recording.
     */

    mutable std::atomic<bool> m_modified;

    /**
     *  Holds the length of the sequence in pulses (ticks).  This value should
     *  be a power of two when used as a bar unit.  This value depends on the
     *  settings of beats/minute, pulses/quarter-note, the beat width, and the
     *  number of measures.
     */

    midi::pulse m_length;

    /**
     *  Holds the last number of measures, purely for detecting changes that
     *  affect the measure count.  Normally, get_measures() makes a live
     *  calculation of the current measure count.  For example, changing the
     *  beat-width to a smaller value could increase the number of measures.
     */

    mutable int m_measures;

    /**
     *  Hold the current unit for a measure.
     *  It is calculated when needed (lazy evaluation).
     */

    mutable midi::pulse m_unit_measure;

    /**
     *  Provides the number of beats per bar used in this sequence.  Defaults
     *  to 4.  Used by the sequence editor to mark things in correct time on
     *  the user-interface.
     *
     *  See transport::info and midi::timesiginfo.
     */

    unsigned short m_beats_per_bar;

    /**
     *  Provides with width of a beat.  Defaults to 4, which means the beat is
     *  a quarter note.  A value of 8 would mean it is an eighth note.  Used
     *  by the sequence editor to mark things in correct time on the
     *  user-interface.
     *
     *  See transport::info and midi::timesiginfo.
     */

    unsigned short m_beat_width;

    /**
     *  This member manages where we are in the playing of this sequence.
     */

    midi::pulse m_last_tick;          /**< Provides the last tick played.     */

    /**
     *  The Note On velocity used, set to usr().note_on_velocity().  If the
     *  recording velocity (m_rec_vol) is non-zero, this value will be set to
     *  the desired recording velocity.  A "stazed" feature.  Note that
     *  we use (-1) for flagging preserving the velocity of incoming notes.
     */

    short m_note_on_velocity;

    /**
     *  The Note Off velocity used, set to usr().note_on_velocity(), and
     *  currently unmodifiable.  A "stazed" feature.
     */

    short m_note_off_velocity;

    /**
     *  Contains the nominal output MIDI bus number for this sequence/pattern.
     *  This number is saved in the sequence/pattern. If port-mapping is in
     *  place, this number is used only to look up the true output buss.
     */

    midi::bussbyte m_nominal_bus;

    /**
     *  Contains the actual buss number to be used in output.  In this base
     *  class, this is always the same as the nominal buss.
     */

    midi::bussbyte m_true_bus;

    /**
     *  Contains the global MIDI channel for this sequence.  However, if this
     *  value is null_channel() (0x80), then this sequence is a multi-chanel
     *  track, and has no single channel, or represents a track who's recorded
     *  channels we do not want to replace.  Please note that this is the
     *  output channel.  However, if set to a valid channel, then that channel
     *  will be forced on notes created via painting in the seqroll.
     */

    midi::byte m_midi_channel;          /* pattern's global MIDI channel    */

    /**
     *  This value indicates that the global MIDI channel associated with this
     *  track is not used.  Instead, the actual channel of each event is
     *  used.  This is true when m_midi_channel == null_channel().
     */

    bool m_free_channel;

public:

    track (number tn = 0);
    track (const track & rhs) = delete;                 /* default; */
    track (track &&) = delete;                          /* forced by mutex */
    track & operator = (const track & rhs) = delete;    /* default; */
    track & operator = (track &&) = delete;             /* forced by mutex */
    virtual ~track () = default;

    /*
     *  This concept may be useful, as it is in Seq66.
     *
     *  void partial_assign (const track & rhs, bool toclipboard = false);
     */

    bool operator < (const track & rhs) const
    {
        return track_number() < rhs.track_number();
    }

    /*-----------------------------------------------------------------------
     * trackinfo
     *-----------------------------------------------------------------------*/

    trackinfo & info ()
    {
        return m_info;
    }

    const trackinfo & info () const
    {
        return m_info;
    }

    static const std::string & default_name ()
    {
        return trackinfo::default_name();
    }

    const std::string & track_name () const
    {
        return info().track_name();
    }

    void track_name (const std::string & n);

    timesiginfo & time_sig_info ()
    {
        return info().timesig_info();
    }

    const timesiginfo & time_sig_info () const
    {
        return info().timesig_info();
    }

    void set_timesig_info (const timesiginfo & tsi)
    {
        info().timesig_info() = tsi;
        m_beats_per_bar = tsi.beats_per_bar();
        m_beat_width = tsi.beat_width();
    }

    keysiginfo & key_sig_info ()
    {
        return info().keysig_info();
    }

    const keysiginfo & key_sig_info () const
    {
        return info().keysig_info();
    }

    void set_keysig_info (const keysiginfo & ksi)
    {
        info().keysig_info() = ksi;
    }

    tempoinfo & tempo_info ()
    {
        return info().tempo_info();
    }

    const tempoinfo & tempo_info () const
    {
        return info().tempo_info();
    }

    void set_tempo_info (const tempoinfo & ti)
    {
        info().tempo_info() = ti;
    }

    bool is_default_name () const
    {
        return info().is_default_name();
    }

    /*-----------------------------------------------------------------------
     * trackdata
     *-----------------------------------------------------------------------*/

    trackdata & data ()
    {
        return m_data;
    }

    const trackdata & data () const
    {
        return m_data;
    }

    void manufacturer_id (const midi::bytes & manufid)
    {
        data().manufacturer_id(manufid);
    }

    eventlist & events ()
    {
        return data().events();
    }

    const eventlist & events () const
    {
        return data().m_events;
    }

    /*-----------------------------------------------------------------------
     * track
     *-----------------------------------------------------------------------*/

    number track_number () const
    {
        return m_track_number;
    }

    bool is_new_pattern () const
    {
        return is_default_name() && event_count() == 0;
    }

    midi::pulse length () const
    {
        return m_length;
    }

    void set_active (bool flag)
    {
        m_active = flag;
    }

    bool active () const
    {
        return m_active;
    }

    bool armed () const
    {
        return m_armed;
    }

    bool recording (bool recordon, bool toggler = false);

    bool recording () const
    {
        return m_recording;
    }

    record recording_type () const
    {
        return m_recording_type;
    }

    bool thru () const
    {
        return m_thru;
    }

    bool is_dirty () const
    {
        return m_is_dirty;
    }

    midi::byte track_midi_channel () const
    {
        return m_midi_channel;
    }

    bool free_channel () const
    {
        return m_free_channel;
    }

    void copy_events (const eventlist & evlist)
    {
        events() = evlist;
    }

    /**
     *  Resets everything to zero.  This function is used when the sequencer
     *  stops.  This function currently sets m_last_tick = 0, but we would
     *  like to avoid that if doing a pause, rather than a stop, of playback.
     */

    void zero_markers ()
    {
        set_last_tick(0);
    }

    void set_last_tick (midi::pulse t);
    int event_count () const;
    int note_count () const;
    int playable_count () const;
    bool is_playable () const;
    bool minmax_notes (int & lowest, int & highest);

#if defined MOVE_THIS_TO_DERIVED_CLASS
    bool song_put_track...
#endif

    void play_note_on (int note);
    void play_note_off (int note);
    void off_playing_notes ();

    bool set_thru (bool thru_active, bool toggler = false);
    bool toggle_playing ();
    bool toggle_playing (midi::pulse tick, bool resumenoteons);
    midi::pulse unit_measure (bool reset = false) const;
    midi::pulse measures_to_ticks (int measures = 1) const;
    void set_measures (int measures);
    int get_measures (midi::pulse newlength) const;
    int get_measures () const;
    int calculate_measures (bool reset) const;

    /**
     *  These function recalculate based on the player's PPQN value.
     *  Before the parent is set, set_timesig_info() is called, and that
     *  merely sets the member variables.
     */

    void beats_per_bar (int beatspermeasure, bool user_change = false);
    void beat_width (int beatwidth, bool user_change = false);

    int beats_per_bar () const
    {
        return int(m_beats_per_bar);
    }

    int beat_width () const
    {
        return int(m_beat_width);
    }

public:

    /*
     * Virtual MIDI/bus functions.
     */

    virtual void set_parent
    (
        player * p, lib66::toggler sorting = lib66::toggler::off
    );
    virtual bool set_length (midi::pulse len = 0, bool verify = true);
    virtual bool master_midi_bus (const midi::masterbus * mmb);
    virtual bool midi_bus (midi::bussbyte mb, bool user_change = false);
    virtual bool midi_channel (midi::byte ch, bool user_change = false);

    /*
     * Playback functions. Note that song mode is not supported in the base
     * class, but can be in derived classes. Song mode uses "triggers" to
     * trigger tracks at various times.
     */

    virtual bool set_armed (bool p);
    virtual void stop (bool song_mode = false);     /* playback::live vs song   */
    virtual void pause (bool song_mode = false);    /* playback::live vs song   */
    virtual void live_play (midi::pulse tick);
    virtual void play_queue
    (
        midi::pulse tick, bool playbackmode = false, bool resume = false
    );
    virtual void play
    (
        midi::pulse tick, bool playback_mode = false, bool resume = false
    );
    virtual bool set_recording
    (
        bool recordon, record r = record::normal, bool toggler = false
    );
    virtual void track_playing_change (bool on, bool qinprogress = false);

public:

    void resume_note_ons (midi::pulse tick)
    {
        (void) tick;
    }

    bool is_recorder_track () const
    {
        return m_track_number == recorder();
    }

    bool is_metro_track () const
    {
        return m_track_number == metronome();
    }

public:

    /*
     * Static functions.
     */

    /*
     *  The maximum number of tracks supported (for now).
     */

    static number maximum ()
    {
        return number(1024);
    }

    static bool is_normal (number t)
    {
        return t < maximum();
    }

    /**
     *  The limiting track number, in macro form.
     */

    static number limit ()
    {
        return number(maximum() * 2);
    }

    /**
     *  A track number that indicates the track is to be used as a
     *  metronome.  Not used here, but in Seq66(v2).
     */

    static number metronome ()
    {
        return limit() - 1;
    }

    static bool is_metronome (number t)
    {
        return t == metronome();
    }

    static short recorder ()
    {
        return limit() - 2;;
    }

    static short is_recorder (int s)
    {
        return short(s) == recorder();
    }

    /**
     *  Indicates that a track number has not been assigned.
     */

    static number unassigned ()
    {
        return number(-1);
    }

    /**
     *  Indicates that all tracks will be processed by a function taking a
     *  track::number parameter.
     */

    static number all ()
    {
        return number(-2);
    }

    /**
     *  A convenient macro function to test against limit().  Although above
     *  the range of usable loop numbers, it is a legal value.
     *  Compare this to the is_valid() function.
     */

    static bool is_legal (number trkno)
    {
        return trkno >= 0 && trkno <= limit();
    }

    /**
     *  Checks if a the track number is an assigned one, i.e. not equal to
     *  -1.  Replaces the null() function.
     */

    static bool is_unassigned (number trkno)
    {
        return trkno == unassigned();
    }

    static bool is_assigned (number trkno)
    {
        return trkno != unassigned();
    }

    /**
     *  Similar to legal(), but excludes limit().
     */

    static bool is_valid (number trkno)
    {
        return trkno >= 0 && trkno < maximum();
    }

    /**
     *  A convenient function to test against track::limit().
     *  This function does not allow that value as a valid value to use.
     */

    static bool is_disabled (number trkno)
    {
        return trkno == limit();
    }

public:

    void track_number (number tn)
    {
        if (tn >= 0 && tn <= limit())
            m_track_number = tn;
    }

    void set_dirty (bool flag = true)
    {
        m_is_dirty = flag;
    }

protected:

    midi::masterbus * master_bus ()
    {
        return m_master_bus;
    }

    const player * parent () const
    {
        return m_parent;
    }

    player * parent ()
    {
        return m_parent;
    }

    virtual void modify (lib66::notification n = lib66::notification::no)
    {
        (void) n;               /* not used in the base class */
        m_modified = true;
    }

    virtual void unmodify (lib66::notification n = lib66::notification::no)
    {
        (void) n;               /* not used in the base class */
        m_modified = false;
    }

    size_t parse_track
    (
        const util::bytevector & datavec,
        size_t offset, size_t len
    )
    {
        return data().parse_track(*this, datavec, offset, len);
    }

    bool modified () const
    {
        return m_modified;
    }

    void armed (bool flag)
    {
        m_armed = flag;
    }

    void free_channel (bool flag)
    {
        m_free_channel = flag;
    }

#if defined MOVE_THIS_TO_DERIVED_CLASS
    void put_seqspec ()....
#endif

protected:

    bool add_event (const event & er);
    bool append_event (const event & er);
    void sort_events ();
    void verify_and_link (bool wrap = false);
    void put_event_on_bus (const event & ev);

#if defined MOVE_THIS_TO_DERIVED_CLASS
    midi::pulse song_put_seq_event...
    void song_put_seq_trigger...
#endif

};          // class track

}           // namespace midi

#endif      // RTL66_MIDI_TRACK_HPP

/*
 * track.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

