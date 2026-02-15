#if ! defined RTL66_SEQUENCE_HPP
#define RTL66_SEQUENCE_HPP

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
 * \file          sequence.hpp
 *
 *  This module declares/defines the base class for handling
 *  patterns/sequences.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2015-07-30
 * \updates       2026-02-07
 * \license       GNU GPLv2 or above
 *
 *  The functions add_list_var() and add_long_list() have been replaced by
 *  functions in the new midi_vector_base module.
 *
 *  We've offloaded most of the trigger code to the triggers class in its own
 *  module, and now just call its member functions to do the actual work.
 */

#include "cfg/history.hpp"              /* cfg::history<> template class    */
#include "ctrl/midimacro.hpp"           /* seq66::midimacro                 */
#include "midi/track.hpp"               /* midi::track base class           */
#include "midi/calculations.hpp"        /* midi::lengthfix, alteration      */
#include "midi/eventlist.hpp"           /* midi::eventlist                  */
#include "midi/timing.hpp"              /* midi::timing class structure     */
#include "play/triggers.hpp"            /* seq66::triggers, etc.            */
#include "xpc/automutex.hpp"            /* xpc::recmutex, automutex         */

namespace midi
{
    class masterbus;
};

namespace seq66
{

class notemapper;
class performer;

/**
 *  Provides an integer value for color that matches PaletteColor::none.  That
 *  is, no color has been assigned.  Track colors are represent by a plain
 *  integer in the seq66::sequence class.
 */

const int c_seq_color_none { -1 };
const int c_use_default_ppqn { -1 };

/**
 *  Provides a way to save a sequence palette color in a single byte.  This
 *  value is signed since we need a value of -1 to indicate no color, and 0 to
 *  127 to indicate the index that "points" to a palette color. (The actual
 *  limit is currently 31, though, which ought to be enough colors.)
 */

using colorbyte = char;

/**
 *  A structure for encapsulating the many input parameters of sequence ::
 *  fix_pattern(). Also serves as an output to describe exactly what
 *  happened with the calculations.
 *
 *  Must be created using an initializer list.
 *
 * \var fp_fix_type
 *      Indicates if the length of the pattern is to be affected, either by
 *      setting the number of measures, or by scaling the pattern.  In either
 *      of those cases, the timestamps of all events will be adjusted
 *      accordingly.
 *
 w \var fp_alter_type
 *      Indicates how all events are to be altered, such as being tightened,
 *      quantized, note-mapping, etc.
 *
 * \var fp_length
 *      Indicates to set the pattern length to a specified value, in ticks.
 *
 * \var fp_tighten_range
 *      Set a range for tightening (partial quantization) of the pattern's
 *      events.
 *
 * \var fp_random_range
 *      Set a range for randomization of events. Randomize velocity for notes.
 *
 * \var fp_pitch_range
 *      Set a range for randomization of note-event pitches.
 *
 * \var fp_quantize_range
 *      Set a range for full quantization) of the pattern's events.
 *
 * \var fp_jitter_range
 *      Set a range, in MIDI ticks, for "humanizing" a pattern.
 *
 * \var fp_align_left
 *      Indicates if the offset of the first event or, preferably first note
 *      event, is to be adjusted to 0, shifting all events leftward by the same
 *      ammount of time.
 *
 * \var fp_align_right
 *      The opposite of fp_align_right.
 *
 * \var fp_reverse
 *      Reverses the timestamps of event, while preserving the duration of the
 *      notes. The new timestamp is the distance of the event from the end
 *      (length) of the pattern, which we call the "reference".
 *
 * \var fp_reverse_in_place
 *      Similar to fp_reverse, except that the last event is used as the
 *      "reference" (instead of the pattern length).
 *
 * \var fp_save_note_length
 *      If true, do not scale the note-off timestamps.  Keep them at the same
 *      offset against the linked note-on event.
 *
 * \var fp_use_time_signature
 *      If true, try to alter the time signature.  This occurs if the measures
 *      string is a fraction (e.g. "3/4" or "5/4").
 *
 * \var fp_beats_per_bar
 *      If fp_use_time_signature is true, then this value is assumed to be the
 *      (possibly new) beats per bar.
 *
 * \var fp_beat_width
 *      If fp_use_time_signature is true, then this value is assumed to be the
 *      (possibly new) beat width.
 *
 * \var [inout] fp_measures
 *      The final length of the pattern,  Ignored if the fix_type is not
 *      lengthfix::measures, but the new bar count is returned here for
 *      display purposes.
 *
 * \var [inout] fp_scale_factor
 *      The factor used to change the length of the pattern,  Ignored if the
 *      fix_type is not lengthfix::rescale. Sanity checked to not too small,
 *      not too large, and not 0.  Might be changed according to process, so
 *      that the final value can be displayed.
 *
 * \var fp_notemap_file
 *      Provides the name of the note-map file to use to re-map notes.
 *
 * \var fp_reverse_notemap
 *      Re-map notes in the other directions
 *
 * \var [out] fp_effect
 *      Indicate the effect(s) of the change, using the fixeffect enumeration
 *      in the calculations module.
 */

struct fixparameters
{
    midi::lengthfix fp_fix_type;
    midi::alteration fp_alter_type;
    midi::pulse fp_length;
    int fp_tighten_range;
    int fp_quantize_range;
    int fp_random_range;
    int fp_pitch_range;
    int fp_jitter_range;
    bool fp_align_left;
    bool fp_align_right;
    bool fp_reverse;
    bool fp_reverse_in_place;
    bool fp_save_note_length;
    bool fp_use_time_signature;
    int fp_beats_per_bar;
    int fp_beat_width;
    double fp_measures;
    double fp_scale_factor;
    std::string fp_notemap_file;
    bool fp_reverse_notemap;
    midi::fixeffect fp_effect;
};

/**
 *  A structure for encapsulating the input parameters of sequence ::
 *  change_event_data_lfo().
 *
 * \var lfo_dc_offset
 *      Provides the "DC" value to be added to the waveform. Ranges from
 *      0 to 127.
 *
 * \var lfo_range
 *      Provides the range of the function, that is, its lowest value and
 *      its highest value. Ranges from 0 to 127.
 *
 * \var lfo_periods
 *      Also known as the "range". It provides the number of periods of
 *      the waveform to apply over the given duration (which is one
 *      measure or the whole pattern length). Ranges from 0 to 16.
 *
 * \var lfo_phase
 *      The starting phase of the waveform. Vaires from 0.0 to 1.0,
 *      which corresponds to a range of 0 to 360 degrees.
 *
 * \var lfo_waveform
 *      The waveform to be applied. See "enum class waveform" in
 *      the calculations.hpp module.
 *
 * \var lfo_use_measure
 *      If true (the normal case) the duration of a period of the
 *      waveform is one measure. If false, then the duration is
 *      the whole pattern length.
 *
 * \var lfo_multiply
 *      Normally, the y(t) value of each LFO calculation is given by
 *      the waveform function. If set to true, then the y(t) value
 *      is scaled from 0 to 127 to 0.0 to 1.0, and is then multiplied
 *      by the actual data value. This allows mutiple applications of
 *      waveform transformations.
 */

struct lfoparameters
{
    double lfo_dc_offset;
    double lfo_range;
    double lfo_periods;
    double lfo_phase;
    midi::waveform lfo_waveform;
    bool lfo_use_measure;
    bool lfo_multiply;
};

/**
 *  The sequence class is firstly a receptable for a single track of MIDI
 *  data read from a MIDI file or edited into a pattern.  More members than
 *  you can shake a stick at.
 */

class sequence : public midi::track
{
    friend class performer;             /* access to set_parent()   */
    friend class triggers;

public:

    /**
     *  Provides a setting for Live vs. Song mode.  Much easier to grok and
     *  expand than a boolean.
     */

    enum class playback
    {
        live,
        song,
        automatic,
        max
    };

    /**
     *  Provides a set of methods for drawing certain items.  These values are
     *  used in the sequence, seqroll, perfroll, and main window classes.
     */

    enum class draw
    {
        none,           /**< indicates that current event is not a note */
        finish,         /**< Indicates that drawing is finished.        */
        linked,         /**< Used for drawing linked notes.             */
        note_on,        /**< For starting the drawing of a note.        */
        note_off,       /**< For finishing the drawing of a note.       */
        tempo,          /**< For drawing tempo meta events.             */
        program,        /**< For drawing program change (patch) events. */
        controller,     /**< For all control-change events.             */
        pitchbend,      /**< For indicating a pitch-wheel event.        */
        max
    };

    /**
     *  Provides two editing modes for a sequence.  A feature adapted from
     *  Kepler34.  Not yet ready for prime time.
     */

    enum class editmode
    {
        note,                   /**< Edit as a Note, the normal edit mode.  */
        drum                    /**< Edit as Drum note, using short notes.  */
    };

    /**
     *  A structure that holds note information, used, for example, in
     *  sequence::get_next_note().
     *
     *  If the note is invalid (as might happen in searches), then the
     *  note value is (-1).
     *
     *  The usage of this small class has evolved to support other
     *  events, as indicated by the draw enumeration above.
     */

    class note_info
    {
        friend class sequence;

    private:

        midi::pulse ni_tick_start;
        midi::pulse ni_tick_finish;
        int ni_note;                /* for tempo, the location to paint it  */
        int ni_velocity;            /* for tempo, the truncated tempo value */
        bool ni_selected;
        bool ni_non_note;           /* true for all non-note events         */

    public:

        note_info () :
            ni_tick_start   (0),
            ni_tick_finish  (0),
            ni_note         (0),    /* we could initialize this to (-1)     */
            ni_velocity     (0),
            ni_selected     (false),
            ni_non_note     (false)
            {
                // no code
            }

       midi::pulse start () const
       {
           return ni_tick_start;
       }

       midi::pulse finish () const
       {
           return ni_tick_finish;
       }

       midi::pulse length () const
       {
           return ni_tick_finish - ni_tick_start;
       }

       int note () const
       {
           return ni_note;
       }

       bool valid () const
       {
           return note() >= 0;
       }

       int velocity () const
       {
           return ni_velocity;
       }

       bool selected () const
       {
           return ni_selected;
       }

       bool non_note ()
       {
           return ni_non_note;
       }

       void show () const;

    };      // nested class note_info

private:

#if ! defined USE_CFG_HISTORY

    /**
     *  Provides a stack of event-lists for use with the undo and redo
     *  facility.
     */

    using eventstack = std::stack<midi::eventlist>;

#endif

public:

    /**
     *  Holds partial information about a time signature.
     */

    using timesig = struct
    {
        double sig_start_measure;   /* Starting measure, precalculated.     */
        double sig_measures;        /* Size in measures, precalculated.     */
        int sig_beats_per_bar;      /* The beats-per-bar in the time-sig.   */
        int sig_beat_width;         /* The size of each beat in the bar.    */
        int sig_ticks_per_beat;     /* Simplifies later calculations.       */
        midi::pulse sig_start_tick; /* The pulse where time-sig was placed. */
        midi::pulse sig_end_tick;   /* Next time-sig start (0 == end?).     */
    };

    /**
     *  A list of time-signatures, which assumes that only the beats/bar and
     *  beat width vary.
     */

    using timesiglist = std::vector<timesig>;

private:

    /**
     *  The number of MIDI notes in what?  This value is used in the sequence
     *  module.  It looks like it is the maximum number of notes that
     *  seq24/seq66 can have playing at one time.  In other words, "only" 256
     *  simultaneously-playing notes can be managed.  Defines the maximum
     *  number of notes playing at one time that the application will support.
     *  BOGUS.  It was meant for counting legal notes, and only 128 are
     *  available (see the constant c_notes_count).
     *
     *      static const int c_playing_notes_max = 256;
     */

    /**
     *  Used as the default velocity parameter in adding notes.
     */

    static short sm_preserve_velocity;

    /*
     * Documented at the definition point in the cpp module.
     */

    static midi::eventlist sm_clipboard;      /* shared between sequences */

    /*
     * For fingerprinting check with speed.
     */

    static int sm_fingerprint_size;

private:

    /**
     *  For pause support, we need a way for the sequence to find out if JACK
     *  transport is active.  We can use the rcsettings flag(s), but JACK
     *  could be disconnected.  We could use a reference here, but, to avoid
     *  modifying the midifile class as well, we use a pointer.  It is set in
     *  performer::add_sequence().  This member would also be using for passing
     *  modification status to the parent, so that the GUI code doesn't have
     *  to do it.
     */

    performer * m_parent { nullptr };

    /**
     *  This list holds the current pattern/sequence events.  It used to be
     *  called m_list_events, but a vector implementation is now available,
     *  and is the default.
     */

    midi::eventlist m_events { };

    /**
     *  Holds the list of triggers associated with the sequence, used in the
     *  performance/song editor.
     */

    triggers m_triggers { };

    /**
     *  Holds a list of time-signatures in the pattern, for use when drawing
     *  the vertical grid-lines in the pattern-editor time, piano roll, and
     *  event (qstriggereditor) panes.
     */

    timesiglist m_time_signatures { };

    /**
     *  Provides a list of event actions to undo for the Stazed LFO and
     *  seqdata support.
     */

    midi::eventlist m_events_undo_hold;

#if defined USE_CFG_HISTORY

    /**
     *  Manages a set of event-lists to provide undo and redo capability.
     *  Replaces using eventstack = std::stack<midi::eventlist> and
     *  a couple booleans.
     */

    cfg::history<midi::eventlist> m_history;

#else

    /**
     *  A stazed flag indicating that we have some undo information.
     */

    bool m_have_undo { false };

    /**
     *  A stazed flag indicating that we have some redo information.
     *  Previously, unlike the perfedit, the seqedit did not provide a redo
     *  facility.
     */

    bool m_have_redo { false };

    /**
     *  Provides a list of event actions to undo.
     */

    eventstack m_events_undo { };

    /**
     *  Provides a list of event actions to redo.
     */

    eventstack m_events_redo { };

#endif

    /**
     *  A new feature for recording, based on a "stazed" feature.  If true
     *  (not the default), then Seq66 will record only MIDI events that match
     *  its output channel.  The old behavior is preserved if this variable is
     *  set to false.
     */

    bool m_channel_match { false };

    /**
     *  Contains the global MIDI output channel for this sequence.  However,
     *  if this value is null_channel() (0x80), then this sequence is a
     *  multi-chanel track, and has no single channel, or represents a track
     *  who's recorded channels we do not want to replace.  Please note that
     *  this is the output channel.  However, if set to a valid channel, then
     *  that channel will be forced on notes created via painting in the
     *  seqroll.
     */

    midi::byte m_midi_channel { 0 };      /* pattern's global MIDI channel    */

    /**
     *  This value indicates that the global MIDI channel associated with this
     *  pattern is not used.  Instead, the actual channel of each event is
     *  used.  This is true when m_midi_channel == null_channel().
     */

    bool m_free_channel { false };

    /**
     *  Contains the nominal output MIDI bus number for this sequence/pattern.
     *  This number is saved in the sequence/pattern. If port-mapping is in
     *  place, this number is used only to look up the true output buss.
     */

    midi::bussbyte m_nominal_bus { midi::null_buss() };

    /**
     *  Contains the actual buss number to be used in output.
     */

    midi::bussbyte m_true_bus { midi::null_buss() };

    /**
     *  Similar to the above, but for the input buss, a new feature.
     *  Unlike the output buss, this input buss is optional.
     */

    midi::bussbyte m_nominal_in_bus { midi::null_buss() };
    midi::bussbyte m_true_in_bus { midi::null_buss() };

    /**
     *  Provides a flag for pattern playback song muting.
     */

    bool m_song_mute { false };

    /**
     *  Indicate if the sequence is transposable or not.  A potential feature
     *  from stazed's seq32 project.  Now it is an actual, configurable
     *  feature.
     */

    bool m_transposable { true };

    /**
     *  Provides a member to hold the polyphonic step-edit note counter.  We
     *  will never come close to the short limit of 32767.
     */

    short m_notes_on { 0 };

    /**
     *  Provides the master MIDI buss which handles the output of the sequence
     *  to the proper buss and MIDI channel.
     */

    midi::masterbus * m_master_bus { nullptr };

    /**
     *  Provides a "map" for Note On events.  It is used when muting, to shut
     *  off the notes that are playing.
     *
     *      unsigned short m_playing_notes[c_notes_count];
     */

    std::vector<unsigned short> m_playing_notes;

    /**
     *  Indicates if the sequence was playing.  This value is set at the end
     *  of the play() function.  It is used to continue playing after changing
     *  the pattern length. Turns out to be unused in both Seq24 and Seq66.
     *
     *      bool m_was_playing;
     */

    /**
     *  True if sequence playback currently is possible for this sequence.
     *  In other words, the sequence is armed.
     */

    bool m_armed { false };

    /**
     *  True if sequence recording currently is in progress for this sequence.
     */

    bool m_recording { false };
    mutable bool m_draw_locked { false };

    /**
     *  If true, the first incoming event in the step-edit (auto-step) part of
     *  stream_event() will reset the starting tick to 0.  Useful when
     *  recording a stock pattern from a drum machine.
     *
     *  Hmmm, no longer in seq66::sequence.
     */

    bool m_auto_step_reset { false };

    /**
     *  Eliminates a bunch of booleans. The default style is merge.
     */

    midi::recordstyle m_recording_style { midi::recordstyle::merge };

    /**
     *  Replaces a potential bunch of booleans. The data type is defined in
     *  the calculations module.
     */

    midi::alteration m_record_alteration { midi::alteration::none };

    /**
     *  True if recording in MIDI-through mode.
     */

    bool m_thru { false };

    /**
     *  True if there's a popup-menu present. See how it is used in
     *  qloopbutton.
     */

    bool m_has_popup { false };

    /**
     *  True if the events are queued.
     */

    bool m_queued { false };

    /**
     *  A member from the Kepler34 project to indicate we are in one-shot mode
     *  for triggering.  Set to false whenever playing-state changes.  Used in
     *  sequence :: play_queue() to maybe play from the one-shot tick, then
     *  toggle play and toggle queuing before playing normally.
     *
     *  One-shot mode is entered when the MIDI control c_status_oneshot event
     *  is received.  Kepler34 reserves the period '.' to initiate this event.
     */

    bool m_one_shot { false };

    /**
     *  A member from the Kepler34 project, set in sequence ::
     *  toggle_one_shot() to m_last_tick adjusted to the length of the
     *  sequence.  Compare this member to m_queued_tick.
     */

    midi::pulse m_one_shot_tick { 0 };

    /**
     *  A counter used in the step-edit (auto-edit) feature.
     *
     *  Hmmmm, not in seq66::sequence.
     */

    int m_step_count { 0 };

    /**
     *  Number of times to play the pattern in Live mode.  A value of 0 means
     *  to play the pattern endlessly in Live mode, like normal.  The maximum
     *  loop-count, if non-zero, is stored in a c_seq_loopcount SeqSpec as a
     *  short integer.
     */

    int m_loop_count_max { 0 };

    /**
     *  Indicates if we have turned off from a snap operation.
     */

    bool m_off_from_snap { false };

    /**
     *  Used to temporarily block Song Mode events while recording new
     *  ones.  Set to false if at a trigger transition in trigger playback.
     *  Otherwise, triggers are allow to be processed.  Turned off when
     *  song-recording stops.
     */

    bool m_song_playback_block { false };

    /**
     *  Used to keep on blocking Song Mode events while recording new ones.
     *  Allows recording a live performance, by storing the sequence triggers.
     *  Adapted from Kepler34.
     */

    bool m_song_recording { false };

    /**
     *  This value indicates that the following feature is active: the number
     *  of ticks to snap recorded improvisations and manually-added triggers.
     */

    bool m_song_recording_snap { true };

    /**
     *  Saves the tick from when we started recording live song data.
     */

    midi::pulse m_song_record_tick { 0 };

    /**
     *  Indicates if the play marker has gone to the beginning of the sequence
     *  upon looping.
     */

    bool m_loop_reset { false };

    /**
     *  Hold the current unit for a measure.  Need to clarifiy this one.
     *  It is calculated when needed (lazy evaluation).
     */

    mutable midi::pulse m_unit_measure { 0 };

    /**
     *  The "m_dirty"  flags indicate that the content of the sequence has
     *  changed due to recording, editing, performance management, or a name
     *  change.  They all start out as "true" in the sequence constructor.
     *
     *      -   The function sequence::set_dirty_mp() sets all but the "dirty
     *          edit" flag to true. It is set when modifying the BPM,
     *          beat-width, toggling cueing, changing the pattern name,
     *      -   The function sequence::set_dirty() sets all four flags to true.
     *
     *  Provides the main dirtiness flag.  In Seq24, it was:
     *
     *      -   Set in perform::is_dirty_main() to set the same status for a
     *          given sequence.  (It also set "was active main" for the
     *          sequence.)
     *      -   Cause mainwnd to update a given sequence in the live frame.
     */

    mutable std::atomic<bool> m_dirty_main { true };

    /**
     *  Provides the main is-edited flag. In Seq24, it was:
     *
     *      -   Set in perform::is_dirty_edit() to set the same status for a
     *          given sequence.  (It also set "was active edit" for the
     *          sequence.)
     *      -   Used in seqedit::timeout to refresh the seqroll, seqdata, and
     *          seqevent panes.
     */

    mutable std::atomic<bool> m_dirty_edit { true };

    /**
     *  Provides performance dirty flagflag.
     *
     *      -   Set in perform::is_dirty_perf() to set the same status for a
     *          given sequence.  (It also set "was active perf" for the
     *          sequence.)
     *      -   Used in perfroll to redraw each "dirty perf" sequence.
     */

    mutable std::atomic<bool> m_dirty_perf { true };

    /**
     *  Provides the names dirtiness flag.
     *
     *      -   Set in perform::is_dirty_names() to set the same status for a
     *          given sequence.  (It also set "was active names" for the
     *          sequence.)
     *      -   Used in perfnames to redraw each "dirty names" sequence.
     */

    mutable std::atomic<bool> m_dirty_names { true };

    /**
     *  Indicates that the sequence is currently being edited.
     */

    bool m_seq_in_edit { false };

    /**
     *  Set by seqedit for the handle_action() function to use.
     */

    midi::byte m_status { 0 };
    midi::byte m_cc { 0 };

    /**
     *  Provides the name/title for the sequence.
     */

    std::string m_name { "Untitled" };

    /**
     *  Provides the default name/title for the sequence.
     */

    static const std::string sm_default_name;

    /**
     *  These members manage where we are in the playing of this sequence,
     *  including triggering.
     */

    midi::pulse m_last_tick { 0 };      /**< Provides the last tick played.     */
    midi::pulse m_queued_tick { 0 };    /**< Provides the tick for queuing.     */
    midi::pulse m_trigger_offset { 0 }; /**< Provides the trigger offset.       */

    /**
     *  This constant provides the scaling used to calculate the time position
     *  in ticks (pulses), based also on the PPQN value.  Hardwired to
     *  c_maxbeats at present.
     */

    const int m_maxbeats { 0xFFFF };    // c_maxbeats TO BE DEFINED

    /**
     *  Holds PPQN, BPM, beats/bar, beat width, and some other mostly
     *  constant quantities.
     */

    midi::timing m_timing;

    /**
     *  A new member so that the sequence number is carried along with the
     *  sequence.  This number is set in the performer::install_sequence()
     *  function.
     *
     *  Also see the alias seq::number, which is not short, but int!
     */

    int m_seq_number { -1 };            // unassigned()

    /**
     *  Implements a feature from the Kepler34 project.  It is an index into a
     *  palette.  The colorbyte type is defined in the midi::bytes.hpp file.
     */

    colorbyte m_seq_color { c_seq_color_none };

    /**
     * A feature adapted from Kepler34.
     */

    editmode m_seq_edit_mode { editmode::note };

    /**
     *  Holds the length of the sequence in pulses (ticks).  This value should
     *  be a power of two when used as a bar unit.  This value depends on the
     *  settings of beats/minute, pulses/quarter-note, the beat width, and the
     *  number of measures.
     */

    midi::pulse m_length { 0 };

    /**
     *  Used in handling one-shot recording while playback is in progress.
     *  This value allows the user to wait a few loops before starting to play
     *  the one-shot pattern.
     */

    midi::pulse m_next_boundary { 0 };

    /**
     *  Holds the last number of measures, purely for detecting changes that
     *  affect the measure count.  Normally, get_measures() makes a live
     *  calculation of the current measure count.  For example, changing the
     *  beat-width to a smaller value could increase the number of measures.
     */

    mutable int m_measures { 0 };

    /**
     *  The size of snap in units of pulses (ticks).  It starts out as the
     *  value m_ppqn / 4.
     */

    midi::pulse m_snap_tick { RTL66_DEFAULT_PPQN / 4 };

    /**
     *  The size of adding an auto-step (step-edit) note in units of pulses
     *  (ticks).  It starts out as the value m_ppqn / 4.
     */

    midi::pulse m_step_edit_note_length { RTL66_DEFAULT_PPQN / 4 };

    /**
     *  New members to use for the c_timesig SeqSpec. Rather than hold
     *  the last time-signature that was set, this holds the first one,
     *  or the value in a c_timesig SeqSpec. If 0, the c_timesig values
     *  have not yet been set.
     */

    unsigned short m_timesig_beats_per_measure { 0 };
    unsigned short m_timesig_beat_width { 0 };

    /**
     *  The volume to be used when recording.  It can range from 0 to 127,
     *  or be set to the preserve-velocity (-1).
     */

    short m_rec_vol;

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
     *  Holds a copy of the musical key for this sequence, which we now
     *  support writing to this sequence.  If the value is
     *  c_key_of_C, then there is no musical key to be set.
     */

    midi::byte m_musical_key;

    /**
     *  Holds a copy of the musical scale for this sequence, which can be
     *  written to this sequence.  If the value is the enumeration
     *  value scales::off, then there is no musical scale to be set.
     *  Provides the index pointing to the optional scale to be shown on the
     *  background of the pattern.
     */

    midi::byte m_musical_scale;

    /**
     *  Holds a copy of the musical chord for this sequence.
     */

    midi::byte m_musical_chord;

    /**
     *  Holds a copy of the background sequence number for this sequence,
     *  which we now support writing to this sequence.  If the value is
     *  greater than max_sequence(), then there is no background sequence to
     *  be set.
     */

    short m_background_sequence;

    /**
     *  Provides locking for the sequence.  Made mutable for use in
     *  certain locked getter functions.
     */

    mutable xpc::recmutex m_mutex;

private:

    /*
     * We're going to replace this operator with the more specific
     * partial_assign() function.
     */

    sequence & operator = (const sequence & rhs);

public:

    sequence (int ppqn = c_use_default_ppqn);

    /*
     * What is the cost of adding virtual here, at runtime?
     */

    virtual ~sequence ();

    void partial_assign (const sequence & rhs, bool toclipboard = false);

    static int maximum ()
    {
        return 1024;
    }

    static int recorder ()
    {
        return 2040;
    }

    static int is_recorder (int s)
    {
        return s == 2040;
    }

    static int metronome ()
    {
        return 2047;
    }

    static bool is_metronome (int s)
    {
        return s == 2047;
    }

    static int limit ()
    {
        return 2048;                                /* 0x0800               */
    }

    static bool is_normal (int s)
    {
        return s < 1024;                            /* see maximum() above  */
    }

    static int unassigned ()
    {
        return (-1);
    }

    midi::eventlist & events ()
    {
        return m_events;
    }

    const midi::eventlist & events () const
    {
        return m_events;
    }

    bool empty () const
    {
        return m_events.empty();
    }

    bool any_selected_notes () const
    {
        return m_events.any_selected_notes();
    }

    bool any_selected_events () const
    {
        return m_events.any_selected_events();
    }

    bool any_selected_events (midi::byte status, midi::byte cc) const
    {
        return m_events.any_selected_events(status, cc);
    }

    bool is_exportable () const
    {
        return ! get_song_mute() && trigger_count() > 0;
    }

    const triggers::container & triggerlist () const
    {
        return m_triggers.triggerlist();
    }

    triggers::container & triggerlist ()
    {
        return m_triggers.triggerlist();
    }

    std::string trigger_listing () const
    {
        return m_triggers.to_string();
    }

    /**
     *  Gets the trigger count, useful for exporting a sequence.
     */

    int trigger_count () const
    {
        return int(m_triggers.count());
    }

    int triggers_datasize (midi::ulong seqspec) const
    {
        return m_triggers.datasize(seqspec);
    }

    int any_trigger_transposed () const
    {
        return m_triggers.any_transposed();
    }

    /**
     *  Gets the number of selected triggers.  That is, selected in the
     *  perfroll.
     */

    int selected_trigger_count () const
    {
        return m_triggers.number_selected();
    }

    void set_trigger_paste_tick (midi::pulse tick)
    {
        m_triggers.set_trigger_paste_tick(tick);
    }

    midi::pulse get_trigger_paste_tick () const
    {
        return m_triggers.get_trigger_paste_tick();
    }

    bool analyze_time_signatures ();

    int time_signature_count () const
    {
        return int(m_time_signatures.size());
    }

    const timesig & get_time_signature (size_t index) const;
    bool current_time_signature (midi::pulse p, int & beats, int & beatwidth) const;
    int measure_number (midi::pulse p) const;
    midi::pulse time_signature_pulses (const std::string & s) const;

    bool is_recorder_seq () const
    {
        return m_seq_number == recorder();
    }

    bool is_metro_seq () const
    {
        return m_seq_number == metronome();
    }

    /*
     * Indicates a normal, modifiable sequence. The sequence is not one of
     * our hidden workhorses for metronome and auto-recording functions.
     * It is normally not visible and not modifiable.
     */

    bool is_normal_seq () const
    {
        return m_seq_number < maximum();
    }

    int seq_number () const
    {
        return m_seq_number;
    }

    std::string seq_number_string () const
    {
        return std::to_string(seq_number());
    }

    void seq_number (int seqno)
    {
        if (seqno >= 0 && seqno <= limit())
            m_seq_number = seqno;
    }

    int color () const
    {
        return int(m_seq_color);
    }

    bool set_color (int c, bool user_change = false);
    void empty_coloring ();

    editmode edit_mode () const
    {
        return m_seq_edit_mode;
    }

    midi::byte edit_mode_byte () const
    {
        return static_cast<midi::byte>(m_seq_edit_mode);
    }

    void edit_mode (editmode mode)
    {
        m_seq_edit_mode = mode;
    }

    void edit_mode (midi::byte b)
    {
        m_seq_edit_mode = b == 0 ? editmode::note : editmode::drum ;
    }

    bool loop_count_max (int m, bool user_change = false);

    virtual void modify
    (
        lib66::notification n = lib66::notification::no
    ) override;

    virtual void unmodify
    (
        lib66::notification n = lib66::notification::no
    ) override;

    int event_count () const;
    int note_count () const;
    bool first_notes (midi::pulse & ts, int & n) const;
    int playable_count () const;
    bool is_playable () const;
    bool minmax_notes (int & lowest, int & highest);

    bool have_undo () const
    {
        return m_have_undo;
    }

    /**
     *  No reliable way to "unmodify" the performance here.
     */

    void set_have_redo ()
    {
        m_have_redo = m_events_redo.size() > 0;
    }

    bool have_redo () const
    {
        return m_have_redo;
    }

    void set_have_undo ();
    void push_undo (bool hold = false);     /* adds stazed parameter    */
    void pop_undo ();
    void pop_redo ();
    void push_trigger_undo ();
    void pop_trigger_undo ();
    void pop_trigger_redo ();
    void set_name (const std::string & name = "");
    int calculate_measures (bool reset = false) const;
    int get_measures (midi::pulse newlength) const;
    int get_measures () const;

    int measures () const
    {
        return m_measures;
    }

    bool event_threshold () const
    {
        return note_count() > sm_fingerprint_size;
    }

    int get_ppqn () const
    {
        return m_timing.PPQN();
    }

    void set_beats_per_bar (int beatspermeasure, bool user_change = false);

    int get_beats_per_bar () const
    {
        return m_timing.BPB();
    }

    void set_beat_width (int beatwidth, bool user_change = false);

    int get_beat_width () const
    {
        return m_timing.BW();
    }

    int timesig_beats_per_measure () const
    {
        return m_timesig_beats_per_measure;     /* stores the main BPB      */
    }

    int timesig_beat_width () const
    {
        return m_timesig_beat_width;            /* stores the main BW       */
    }

    void set_time_signature (int bpb, int bw);

    /**
     *  A convenience function for calculating the number of ticks in the
     *  given number of measures.
     */

    midi::pulse measures_to_ticks (int measures = 1) const
    {
        return midi::measures_to_ticks          /* "calculations" module    */
        (
            m_timing.BPB(), int(m_timing.PPQN()),
            m_timing.BW(), measures
        );
    }

    void clocks_per_metronome (int cpm)
    {
        m_timing.clocks_per_metronome(cpm);
    }

    int clocks_per_metronome () const
    {
        return m_timing.clocks_per_metronome();
    }

    void set_32nds_per_quarter (int tpq)
    {
        m_timing.set_32nds_per_quarter(tpq);
    }

    int get_32nds_per_quarter () const
    {
        return m_timing.get_32nds_per_quarter();
    }

    void us_per_quarter_note (long upqn)
    {
        m_timing.us_per_quarter_note(upqn);
    }

    long us_per_quarter_note () const
    {
        return m_timing.us_per_quarter_note();
    }

    void set_rec_vol (int rec_vol);
    void set_song_mute (bool mute);
    void toggle_song_mute ();

    bool get_song_mute () const
    {
        return m_song_mute;
    }

    void apply_song_transpose ();
    void set_transposable (bool flag, bool user_change = false);

    bool transposable () const
    {
        return m_transposable;
    }

    std::string title () const;

    const std::string & name () const
    {
        return m_name;
    }

    /**
     *  Tests the name for being changed.
     */

    bool is_default_name () const
    {
        return m_name == sm_default_name;
    }

    bool is_new_pattern () const
    {
        return is_default_name() && event_count() == 0;
    }

    static bool valid_scale_factor (double s, bool ismeasure = false);
    static int trunc_measures (double m);

    static const std::string & default_name ()
    {
        return sm_default_name;
    }

    void seq_in_edit (bool edit)
    {
        m_seq_in_edit = edit;
    }

    bool seq_in_edit () const
    {
        return m_seq_in_edit;
    }

    bool set_length_ex
    (
        midi::pulse len = 0,
        bool adjust_triggers = true,
        bool verify = true
    );

    bool set_measures (int measures, bool user_change = false);
    int increment_measures ();
    bool apply_length
    (
        int bpb, int ppqn, int bw,
        int measures = 0, bool user_change = false
    );
    bool extend_length ();
    bool double_length ();

    bool apply_length (int meas = 0, bool user_change = false)
    {
        return apply_length(0, 0, 0, meas, user_change);
    }

    midi::pulse get_length () const
    {
        return m_length;
    }

    midi::pulse get_length_plus () const
    {
        int bpmeas { m_timing.BPB() };
        if (bpmeas == 0)
            bpmeas = 4;

        return m_length + m_unit_measure / bpmeas;
    }

    midi::pulse get_tick () const;
    midi::pulse get_last_tick () const;
    void set_last_tick (midi::pulse tick = midi::c_null_pulse);

    midi::pulse last_tick () const
    {
        return m_last_tick;
    }

    /**
     *  Some MIDI file errors and other things can lead to an m_length of 0,
     *  which causes arithmetic errors when m_last_tick is modded against it.
     *  This function replaces the "m_last_tick % m_length", returning
     *  m_last_tick if m_length is 0 or 1.
     */

    midi::pulse mod_last_tick ()
    {
        return (m_length > 1) ? (m_last_tick % m_length) : m_last_tick ;
    }

    /*
     * Documented at the definition point in the cpp module.
     */

    bool set_armed (bool p);

    bool armed () const
    {
        return m_armed;
    }

    bool muted () const
    {
        return ! m_armed;
    }

    bool sequence_playing_toggle ();
    bool toggle_playing ();
    bool toggle_playing (midi::pulse tick, bool resumenoteons);
    bool toggle_queued ();

    void set_popup (bool flag)
    {
        m_has_popup = flag;
    }

    bool has_popup () const
    {
        return m_has_popup;
    }

    bool get_queued () const
    {
        return m_queued;
    }

    midi::pulse get_queued_tick () const
    {
        return m_queued_tick;
    }

    bool check_queued_tick (midi::pulse tick) const
    {
        return get_queued() && (get_queued_tick() <= tick);
    }

    bool set_recording_style (midi::recordstyle rs);

    /*
     * The lib66::lib66::toggler flag enumeration is off, on, and flip!
     * Compare these two functions to midi::track::set_recording().
     */

    bool set_recording_ex (lib66::toggler flag);
    bool set_recording_ex (midi::alteration q, lib66::toggler flag);
    bool set_thru (bool thru_active, bool toggle = false);

    bool recording () const
    {
        return m_recording;
    }

    bool alter_recording () const
    {
        return m_record_alteration != midi::alteration::none;
    }

    midi::alteration record_alteration () const
    {
        return m_record_alteration;
    }

    void record_alteration (midi::alteration a)
    {
        m_record_alteration = a;
    }

    bool quantized_recording () const
    {
        return m_record_alteration == midi::alteration::quantize;
    }

    bool quantizing () const
    {
        return quantized_recording();
    }

    bool tightened_recording () const
    {
        return m_record_alteration == midi::alteration::tighten;
    }

    bool tightening () const
    {
        return tightened_recording();
    }

    bool notemapped_recording () const
    {
        return m_record_alteration == midi::alteration::notemap;
    }

    bool notemapping () const
    {
        return notemapped_recording();
    }

    bool expanded_recording () const
    {
        return m_recording_style == midi::recordstyle::expand;
    }

    bool expanding () const
    {
        return recording() && expanded_recording();
    }

    bool auto_step_reset () const
    {
        return m_auto_step_reset;
    }

    bool oneshot_recording () const
    {
        return m_recording_style == midi::recordstyle::oneshot;
    }

    void auto_step_reset (bool flag)
    {
        m_auto_step_reset = flag;
        m_step_count = 0;
    }

    bool expand_recording () const;     /* does more checking for status    */

    bool overwriting () const
    {
        return m_recording_style == midi::recordstyle::overwrite;
    }

    bool thru () const
    {
        return m_thru;
    }

    midi::pulse snap () const
    {
        return m_snap_tick;
    }

    midi::pulse step_edit_note_length () const    /* auto-step/step-edit      */
    {
        return m_step_edit_note_length;
    }

    void snap (int st);
    void step_edit_note_length (int len);
    void off_one_shot ();
    void song_recording_start (midi::pulse tick, bool snap = true);
    void song_recording_stop (midi::pulse tick);

    bool one_shot () const
    {
        return m_one_shot;
    }

    midi::pulse one_shot_tick () const
    {
        return m_one_shot_tick;
    }

    bool check_one_shot_tick (midi::pulse tick) const
    {
        return one_shot() && (one_shot_tick() <= tick);
    }

    int step_count () const
    {
        return m_step_count;
    }

    int loop_count_max () const
    {
        return m_loop_count_max;
    }

    bool song_recording () const
    {
        return m_song_recording;
    }

    bool off_from_snap () const
    {
        return m_off_from_snap;
    }

    bool snap_it () const
    {
        return armed() && (get_queued() || off_from_snap());
    }

    bool song_playback_block () const
    {
        return m_song_playback_block;
    }

    bool song_recording_snap () const
    {
        return m_song_recording_snap;
    }

    midi::pulse song_record_tick () const
    {
        return m_song_record_tick;
    }

    void resume_note_ons (midi::pulse tick);
    bool toggle_one_shot ();
    bool is_dirty_main () const;
    bool is_dirty_edit () const;
    bool is_dirty_perf () const;
    bool is_dirty_names () const;
    void set_dirty_mp ();
    void set_dirty ();
    std::string channel_string () const;            /* "F" or "<channel+1>" */
    bool set_channels (int channel);                /* modifies event list  */

    midi::byte seq_midi_channel () const
    {
        return m_midi_channel;                      /* midi_channel() below */
    }

    midi::byte get_midi_channel (const midi::event & ev) const
    {
        return m_free_channel ? ev.channel() : m_midi_channel ;
    }

    midi::byte get_midi_channel () const
    {
        return m_free_channel ? midi::null_channel() : m_midi_channel ;
    }

    bool free_channel () const
    {
        return m_free_channel;
    }

    /**
     *  Returns true if this sequence is an SMF 0 sequence.
     */

    bool is_smf_0 () const
    {
        return midi::is_null_channel(m_midi_channel);
    }

    std::string to_string () const;
    void play (midi::pulse tick, bool playback_mode, bool resume = false);
    void live_play (midi::pulse tick);
    void play_queue (midi::pulse tick, bool playbackmode, bool resume);
    bool push_add_note
    (
        midi::pulse tick, midi::pulse len, int note,
        bool repaint = false,
        int velocity = sm_preserve_velocity
    );
    bool push_add_chord
    (
        int chord, midi::pulse tick, midi::pulse len,
        int note, int velocity = sm_preserve_velocity
    );
    bool add_painted_note
    (
        midi::pulse tick, midi::pulse len, int note,
        bool repaint = false,
        int velocity = sm_preserve_velocity
    );
    bool add_note (midi::pulse len, const midi::event & e);
    bool add_chord
    (
        int chord, midi::pulse tick, midi::pulse len, int note,
        int velocity = sm_preserve_velocity
    );
    bool add_tempo (midi::pulse tick, midi::bpm tempo, bool repaint = false);
    bool add_tempos
    (
        midi::pulse tick_s, midi::pulse tick_f,
        int tempo_s, int tempo_f
    );
//  bool add_time_signature (midi::pulse tick, int beats, int width);
//  bool delete_time_signature (midi::pulse tick);
    bool log_time_signature
    (
        midi::pulse tick, int beats, int width, bool user_change = false
    );
    bool update_time_signature (int bpb, int bw, bool user_change = false);
    bool add_timesig_event (const midi::event & e, bool main_ts = false);
    bool add_timesig_event
    (
        midi::pulse t,
        int bpb = 4, int bw = 4,
        bool replace = true
    );
    bool set_main_time_signature ();
    bool add_c_timesig (int bpb, int bw, bool main_ts = false);
    bool delete_time_signature (midi::pulse tick);
    bool detect_time_signature
    (
        midi::pulse & tstamp, int & numerator, int & denominator,
        midi::pulse start = 0,
        midi::pulse limit = midi::c_null_pulse
    );
    bool add_event (const midi::event & er);    /* another declared below   */
    bool add_event
    (
        midi::pulse tick, midi::byte status,
        midi::byte d0, midi::byte d1, bool repaint = false
    );
    bool add_event (midi::pulse tick, const midi::bytes & dbytes);
    bool add_macro (midi::pulse tick, const midimacro & macro);
    bool append_event (const midi::event & er);
    void sort_events ();
    midi::event find_event (const midi::event & e, bool nextmatch = false);
    note_info find_note (midi::pulse tick, int note);
    bool remove_duplicate_events (midi::pulse tick, int note = (-1));
    void notify_change (bool userchange = true);
    void notify_trigger ();
    void print_triggers () const;
    bool add_trigger
    (
        midi::pulse tick, midi::pulse len,
        midi::pulse offset    = 0,
        midi::byte tpose      = 0,
        bool adjust_offset  = true
    );
    bool split_trigger (midi::pulse tick, trigger::splitpoint splittype);
    bool grow_trigger (midi::pulse tick_from, midi::pulse tick_to, midi::pulse len);
    bool grow_trigger (midi::pulse tick_from, midi::pulse tick_to);
    const trigger & find_trigger (midi::pulse tick) const;
    bool delete_trigger (midi::pulse tick);
    bool clear_triggers ();
    bool get_trigger_state (midi::pulse tick) const;
    bool transpose_trigger (midi::pulse tick, int transposition);
    bool select_trigger (midi::pulse tick);
    triggers::container get_triggers () const;
    bool unselect_trigger (midi::pulse tick);
    bool unselect_triggers ();

#if defined USE_INTERSECT_FUNCTIONS
    bool intersect_triggers (midi::pulse pos, midi::pulse & start, midi::pulse & end);
    bool intersect_triggers (midi::pulse pos);
    bool intersect_notes
    (
        midi::pulse position, int position_note,
        midi::pulse & start, midi::pulse & ender, int & note
    );
    bool intersect_events
    (
        midi::pulse posstart, midi::pulse posend,
        midi::byte status, midi::pulse & start
    );
#endif

    bool delete_selected_triggers ();
    bool cut_selected_triggers ();
    bool copy_selected_triggers ();
    bool paste_trigger (midi::pulse paste_tick = c_no_paste_trigger);
    bool move_triggers
    (
        midi::pulse start_tick, midi::pulse distance,
        bool direction, bool single = true
    );
    bool move_triggers
    (
        midi::pulse tick, bool adjust_offset,
        triggers::grow which = triggers::grow::move
    );
    void offset_triggers
    (
        midi::pulse offset,
        triggers::grow editmode = triggers::grow::move
    );
    bool selected_trigger
    (
        midi::pulse droptick, midi::pulse & tick0, midi::pulse & tick1
    );
    midi::pulse selected_trigger_start ();
    midi::pulse selected_trigger_end ();
    midi::pulse get_max_timestamp () const;
    midi::pulse get_max_trigger () const;
    void copy_triggers (midi::pulse start_tick, midi::pulse distance);

    midi::pulse get_trigger_offset () const
    {
        return m_trigger_offset;
    }

    midi::bussbyte seq_midi_bus () const
    {
        return m_nominal_bus;
    }

    midi::bussbyte true_bus () const
    {
        return m_true_bus;
    }

    midi::bussbyte seq_midi_in_bus () const
    {
        return m_nominal_in_bus;
    }

    midi::bussbyte true_in_bus () const
    {
        return m_true_in_bus;
    }

    bool has_in_bus () const
    {
        return midi::is_good_buss(m_true_in_bus);
    }

    bool set_master_midi_bus (const midi::masterbus * mmb);
    bool set_midi_bus (midi::bussbyte mb, bool user_change = false);
    bool set_midi_channel (midi::byte ch, bool user_change = false);
    bool set_midi_in_bus (midi::bussbyte mb, bool user_change = false);
    int select_note_events
    (
        midi::pulse tick_s, int note_h,
        midi::pulse tick_f, int note_l, midi::eventlist::select action
    );
    int select_notes_by_pitch (int note_h, int note_l);
    int select_events
    (
        midi::pulse tick_s, midi::pulse tick_f,
        midi::status sstatus, midi::byte cc, midi::eventlist::select action
    );
    int select_events
    (
        midi::status sstatus, midi::byte cc, bool inverse = false
    );
    int select_event_handle
    (
        midi::pulse tick_s, midi::pulse tick_f,
        midi::status sstatus, midi::byte cc,
        midi::byte data
    );
    void adjust_event_handle (midi::status sstatus, midi::byte data);

    /**
     *  New convenience function.  What about Aftertouch events?  I think we
     *  need to select them as well in seqedit, so let's add that selection
     *  here as well.
     *
     * \param inverse
     *      If set to true (the default is false), then this causes the
     *      selection to be inverted.
     */

    void select_all_notes (bool inverse = false)
    {
        (void) select_events(midi::status::note_on, 0, inverse);
        (void) select_events(midi::status::note_off, 0, inverse);
        (void) select_events(midi::status::aftertouch, 0, inverse);
    }

    int get_num_selected_notes () const;
    int get_num_selected_events (midi::status sstatus, midi::byte cc) const;
    void select_all ();
    void select_by_channel (int channel);
    void select_notes_by_channel (int channel);
    void unselect ();
    bool repitch (const notemapper & nmap, bool all = false);
    bool copy_selected ();
    bool cut_selected (bool copyevents = true);
    bool paste_selected (midi::pulse tick, int note);
    bool merge_events (const sequence & source);
    bool selected_box
    (
        midi::pulse & tick_s, int & note_h,
        midi::pulse & tick_f, int & note_l
    );
    bool onsets_selected_box
    (
        midi::pulse & tick_s, int & note_h,
        midi::pulse & tick_f, int & note_l
    );
    bool clipboard_box
    (
        midi::pulse & tick_s, int & note_h,
        midi::pulse & tick_f, int & note_l
    );
    midi::pulse clip_timestamp (midi::pulse ontime, midi::pulse offtime);
    bool move_selected_notes (midi::pulse deltatick, int deltanote);
    bool move_selected_events (midi::pulse deltatick);
    bool stream_event (midi::event & ev);
    bool change_event_data_range
    (
        midi::pulse tick_s, midi::pulse tick_f,
        midi::status sstatus, midi::byte cc,
        int d_s, int d_f, bool finalize = false
    );
    bool change_event_data_relative
    (
        midi::pulse tick_s, midi::pulse tick_f,
        midi::status sstatus, midi::byte cc,
        int newval, bool finalize = false
    );
    void change_event_data_lfo
    (
        const lfoparameters & lp, midi::status sstatus, midi::byte cc
    );
    bool fix_pattern (fixparameters & param);   /* for qpatternfix dialog   */
    void increment_selected (midi::status sstatus, midi::byte /*control*/);
    void decrement_selected (midi::status sstatus, midi::byte /*control*/);
    bool grow_selected (midi::pulse deltatick);
    bool stretch_selected (midi::pulse deltatick);
    bool randomize (midi::status sstatus, int range = (-1), bool all = false);
    bool randomize_note_velocities (int range = (-1), bool all = false);
    bool randomize_note_pitches (int range = (-1), bool all = false);
    bool jitter_notes (int jitter = (-1), bool all = false);
    bool mark_selected ();
    void unpaint_all ();
    void verify_and_link (bool wrap = false);
    void link_new ();
    bool edge_fix ();
    bool remove_unlinked_notes ();

    /**
     *  Resets everything to zero.  This function is used when the sequencer
     *  stops.  This function currently sets m_last_tick = 0, but we would
     *  like to avoid that if doing a pause, rather than a stop, of playback.
     */

    void zero_markers ()
    {
        set_last_tick(0);
    }

    void play_note_on (int note);
    void play_note_off (int note);
    void off_playing_notes ();
    void stop (bool song_mode = false);     /* playback::live vs song   */
    void pause (bool song_mode = false);    /* playback::live vs song   */
    void reset_draw_trigger_marker ();
    bool clear_events ();
    void draw_lock () const;
    void draw_unlock () const;

    midi::event::buffer::const_iterator cbegin () const
    {
        return m_events.cbegin();
    }

    bool cend (midi::event::buffer::const_iterator & evi) const
    {
        return evi == m_events.cend();
    }

    bool reset_interval
    (
        midi::pulse t0, midi::pulse t1,
        midi::event::buffer::const_iterator & it0,
        midi::event::buffer::const_iterator & it1
    ) const;
    draw get_next_note
    (
        note_info & niout,
        midi::event::buffer::const_iterator & evi
    ) const;
    bool get_next_event_match
    (
        midi::byte status, midi::byte cc,
        midi::event::buffer::const_iterator & evi
    );
    bool get_next_meta_match
    (
        midi::byte metamsg,
        midi::event::buffer::const_iterator & evi,
        midi::pulse start = 0,
        midi::pulse range = midi::c_null_pulse
    );
    bool get_next_event
    (
        midi::byte & status, midi::byte & cc,
        midi::event::buffer::const_iterator & evi
    );
    bool next_trigger (trigger & trig);
    bool push_quantize (midi::byte status, midi::byte cc, int divide);
    bool push_quantize_notes (int divide);
    bool push_jitter_notes (int range = -1);
    bool transpose_notes (int steps, int scale, int key = 0);

#if defined RTL66_SEQ32_SHIFT_SUPPORT
    void shift_notes (midi::pulse ticks);
#endif

    midi::byte musical_key () const
    {
        return m_musical_key;
    }

    midi::byte musical_scale () const
    {
        return m_musical_scale;
    }

    midi::byte musical_chord () const
    {
        return m_musical_chord;
    }

    int background_sequence () const
    {
        return int(m_background_sequence);
    }

    void musical_key (int key, bool user_change = false);
    void musical_scale (int scale, bool user_change = false);
    void musical_chord (int c, bool user_change = false);
    bool background_sequence (int bs, bool user_change = false);
    void show_events () const;
    bool copy_events (const midi::eventlist & newevents);
    midi::pulse unit_measure (bool reset = false) const;
    midi::pulse expand_threshold () const;
    midi::pulse progress_value () const;

    /**
     *  The master bus needs to know if the match feature is truly in force,
     *  otherwise it must pass the incoming events to all recording sequences.
     *  Compare this function to channels_match().
     */

    bool channel_match () const
    {
        return m_channel_match;
    }

    void loop_reset (bool reset);

    bool loop_reset () const
    {
        return m_loop_reset;
    }

    midi::pulse handle_size (midi::pulse start, midi::pulse finish);
    void handle_edit_action (midi::eventlist::edit action, int var);
    bool check_loop_reset ();

public:

    static void clear_clipboard ()
    {
        sm_clipboard.clear();                   /* shared between sequences */
    }

    static midi::recordstyle loop_record_style (int ri);

    /**
     *  Short hand for testing a draw parameter.
     */

    static bool is_draw_note (draw dt)
    {
        return dt == sequence::draw::linked ||
            dt == sequence::draw::note_on || dt == sequence::draw::note_off;
    }

    /**
     *  Necessary for drawing notes in a perf roll.  Why?
     */

    static bool is_draw_note_onoff (draw dt)
    {
        return dt == sequence::draw::note_on || dt == sequence::draw::note_off;
    }

    bool remove_selected ();
    bool remove_marked ();                      /* a forwarding function    */
    bool update_recording (int index);
    bool remove_orphaned_events ();

private:

    bool flatten (sequence & destseq, bool maketrigger = true);
    midi::pulse flatten_trigger
    (
        sequence & destseq,
        const trigger & trig,
        midi::pulse prev_timestamp
    );

protected:

    // TODO: reconcile with midi::track::set_parent()

    void set_parent_ex (performer * p);

    void armed (bool flag)
    {
        m_armed = flag;
    }

    void free_channel (bool flag)
    {
        m_free_channel = flag;
    }

private:

    midi::pulse apply_time_factor
    (
        double factor,
        bool savenotelength = false,
        bool relink = false
    );

    midi::masterbus * master_bus ()
    {
        return m_master_bus;
    }

    const performer * perf () const
    {
        return m_parent;
    }

    performer * perf ()
    {
        return m_parent;
    }

    bool check_oneshot_recording ();
    bool quantize_events (midi::byte status, midi::byte cc, int divide);
    bool quantize_notes (int divide);
    bool change_ppqn (int p);
    void put_event_on_bus (const midi::event & ev);
//  void reset_loop ();
    void set_trigger_offset (midi::pulse trigger_offset);
    void adjust_trigger_offsets_to_length (midi::pulse newlen);
    midi::pulse adjust_offset (midi::pulse offset);
    draw get_note_info                      /* used only internally     */
    (
        note_info & niout,
        midi::event::buffer::const_iterator & evi
    ) const;

    timesig default_time_signature () const;
    void push_default_time_signature ();

#if defined USE_SEQUENCE_REMOVE_EVENTS
    void remove (midi::event::buffer::iterator i);
    void remove (midi::event & e);
#endif

    bool remove_first_match (const midi::event & e, midi::pulse starttick = 0);
    bool remove_all ();

    /**
     *  Checks to see if the event's channel matches the sequence's nominal
     *  channel.
     *
     * \param e
     *      The event whose channel nybble is to be checked.
     *
     * \return
     *      Returns true if the channel-matching feature is enabled and the
     *      channel matches, or true if the channel-matching feature is turned
     *      off, in which case the sequence accepts events on any channel.
     */

    bool channels_match (const midi::event & e) const
    {
        return m_channel_match ?
            midi::mask_channel(e.status_byte()) == m_midi_channel : true ;
    }

    void draw_locked (bool flag)
    {
        m_draw_locked = flag;
    }

    void one_shot (bool f)
    {
        m_one_shot = f;
    }

    void off_from_snap (bool f)
    {
        m_off_from_snap = f;
    }

    void song_playback_block (bool f)
    {
        m_song_playback_block = f;
    }

    void song_recording (bool f)
    {
        m_song_recording = f;
    }

    void song_recording_snap (bool f)
    {
        m_song_recording_snap = f;
    }

    void song_record_tick (midi::pulse t)
    {
        m_song_record_tick = t;
    }

    void channel_match (bool flag)
    {
        m_channel_match = flag;
    }

};          // class sequence

}           // namespace seq66

#endif      // RTL66_SEQUENCE_HPP

/*
 * sequence.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

