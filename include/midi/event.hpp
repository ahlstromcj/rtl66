#if ! defined RTL66_MIDI_EVENT_HPP
#define RTL66_MIDI_EVENT_HPP

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
 * \file          event.hpp
 *
 *  This module declares/defines the event class for operating with
 *  MIDI events.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2025-01-16
 * \license       GNU GPLv2 or above
 *
 *  This module also declares/defines the various constants, status-byte
 *  values, or data values for MIDI events.  This class is also a base class,
 *  so that we can manage "editable events".
 *
 *  One thing we need to add to this event class is a way to encapsulate Meta
 *  events.  First, we use the existing event::sysex to hold this data.
 *
 *  The MIDI protocol consists of MIDI events that carry four types of
 *  messages:
 *
 *      -   Voice messages.  0x80 to 0xEF; includes channel information.
 *      -   System common messages.  0xF0 (SysEx) to 0xF7 (End of SysEx)
 *      -   System realtime messages. 0xF8 to 0xFF.
 *      -   Meta messages. 0xFF is the flag, followed by type, length, and
 *          data.
 */

#include <string>                       /* used in to_string()              */

#include "midi/eventcodes.hpp"          /* event status bytes, free funcs   */
#include "midi/message.hpp"             /* storage for any MIDI event       */

#define RTL66_STAZED_SELECT_EVENT_HANDLE

namespace midi
{

/**
 *  Provides a sanity-check limit for the number of bytes in a MIDI Meta Text
 *  message and similar messages. Might be better larger, but.... Well, now
 *  that we're handling meta text  message, we'll make this a bit larger than
 *  1024. This value is also used in the main Session tab to limit the
 *  amount of text in the song info edit field.
 */

const size_t c_meta_text_limit = 32767;     /* good for a sanity check, too */

/**
 *  Provides events for management of MIDI events.
 *
 *  A MIDI event consists of 3 bytes (or more):
 *
 *      -#  Status byte, 1sssnnnn, where the 1sss bits specify the type of
 *          message, and the nnnn bits denote the channel number, 0 to 15.
 *          The status byte always starts with 1.
 *      -#  The first data byte, 0xxxxxxx, where the data byte always
 *          start with 0, and the xxxxxxx values range from 0 to 127.
 *      -#  The second data byte, 0xxxxxxx.
 *
 *  This class may have too many member functions.
 */

class event
{

    friend class eventlist;
    friend class track;         /* is this needed? */

public:

    /**
     *  Provides a type definition for a vector of midi::bytes.  This type will
     *  also hold the generally small amounts of data needed for Meta events,
     *  but doesn't help us encapsulate derived values, such as tempo.
     *  Replaced by midi::message for all messages.
     *
     *      using sysex = midi::bytes;
     */

    /**
     *  The data buffer for MIDI events.  This item replaces the
     *  eventlist::Events type definition so that we can replace event
     *  pointers with iterators, safely.
     */

    using buffer = std::vector<event>;
    using iterator = buffer::iterator;
    using const_iterator = buffer::const_iterator;
    using reverse_iterator = buffer::reverse_iterator;
    using const_reverse_iterator = buffer::const_reverse_iterator;

public:

    /**
     *  Provides a key value for an event.  Its types match the
     *  m_timestamp and get_rank() function of the event class.
     *  We eventually need this key for an "editable-events" container,
     *  which would use a multimap.
     */

    class key
    {

    private:

        midi::pulse m_timestamp;    /**< The primary key-value for the key. */
        int m_rank;                 /**< The sub-key-value for the key.     */

    public:

        key () = default;
        key (midi::pulse tstamp, int rank);
        key (const event & e);
        bool operator < (const key & rhs) const;
        bool operator == (const key & rhs) const;
        key (const key & ek) = default;
        key & operator = (const key & ek) = default;
    };

private:

    /**
     *  Indicates the input buss on which this event came in.  The default
     *  value is unusable: null_buss() from the midi::bytes.hpp module.
     */

    bussbyte m_input_buss;

    /**
     *  Provides the MIDI timestamp in ticks, otherwise known as the "pulses"
     *  in "pulses per quarter note" (PPQN). Also note that the midi::message
     *  m_message member holds a double version of the timestamp.
     */

    midi::pulse m_timestamp;

    /**
     *  The event status byte. now folded into the first byte of the
     *  midi::message object m_message.
     *
     *      midi::status m_status;
     *
     *  The two bytes of data for the MIDI event.  Remember that the
     *  most-significant bit of a data byte is always 0.  A one-byte message
     *  uses only the 0th index. Now folded into midi::message.
     *
     *      midi::byte m_data[m_data_byte_count];
     */

    /**
     *  All bytes of status and data for the MIDI event. It includes the
     *  event time-stamp, event status, event channel if applicable, d0
     *  and d1 data bytes or the array of data for Meta messages and SysEx.
     *
     *  Remember that the most-significant bit of a data byte is always 0.  A
     *  one-byte message uses only the 1st index.
     *
     *  The channel is included when recording MIDI, but, once a track with
     *  a matching channel is found, the channel nybble is cleared for
     *  storage.  The channel will be added back on the MIDI bus upon
     *  playback.  The high nybble = type of event; The low nybble = channel.
     *  Bit 7 is present in all status bytes.
     *
     *  Note that, for status values of 0xF0 (Sysex) or 0xFF (Meta), special
     *  handling of the event can occur.  We would like to eventually use
     *  inheritance to keep the event class simple, but that would interfere
     *  with event containment by copy (we don't want to manage event
     *  pointers).
     */

    midi::message m_message;                           /* status, d0, d1, ...  */

    /**
     *  In order to be able to handle MIDI channel-splitting of an SMF 0 file,
     *  we need to store the channel, even if we override it when playing the
     *  MIDI data.
     *
     *  Overload:  For Meta events, where is_meta() is true, this value holds
     *  the type of Meta event. See the editable_event::sm_meta_event_names[]
     *  array.
     */

    midi::byte m_channel;

    /**
     *  The data buffer for SYSEX messages.  Adapted from Stazed's Seq32
     *  project on GitHub.  This object will also hold the generally small
     *  amounts of data needed for Meta events.  Compare is_sysex() to
     *  is_meta() and is_ex_data() [which tests for both].
     *  Now folded into midi::message:
     *
     *      sysex m_sysex;
     */

    /**
     *  This event is used to link NoteOns and NoteOffs together.  The NoteOn
     *  points to the NoteOff, and the NoteOff points to the NoteOn.  See, for
     *  example, eventlist::link_notes().
     *
     *  We currently do not link tempo events; this would be necessary to
     *  display a line from one tempo event to the next.  Currently we display
     *  a small circle for each tempo event.
     */

    iterator m_linked;

    /**
     *  Indicates that a link has been made.  This item is used [via
     *  the get_link() and link() accessors] in the sequence class.
     */

    bool m_has_link;

    /**
     *  Answers the question "is this event selected in editing."
     */

    bool m_selected;

    /**
     *  Answers the question "is this event marked in processing."  This
     *  marking is more of an internal function for purposes of reorganizing
     *  events.
     */

    bool m_marked;

#if defined RTL66_SUPPORT_PAINTED_EVENTS

    /**
     *  Answers the question "is this event being painted."  This setting is
     *  made by sequence::add_event() or add_note() if the paint parameter is
     *  true (it defaults to false).
     */

    bool m_painted;

#endif

public:

    event ();
    event
    (
        midi::pulse tstamp,
        midi::byte status,
        midi::byte d0,
        midi::byte d1 = 0
    );
    event (midi::pulse tstamp, midi::bpm tempo);
    event
    (
        midi::pulse tstamp,
        midi::status notekind,
        midi::byte channel,
        int note, int velocity
    );
    event (const event & rhs);
    event (event &&) = default;
    event & operator = (const event & rhs);         /* REVISIT! */
    event & operator = (event &&) = default;        /* TODO?    */
    virtual ~event ();

    /*
     * Operator overload, the only one needed for sorting events in a list
     * or a map.
     */

    bool operator < (const event & rhsevent) const;
    bool match (const event & target) const;
    void prep_for_send (midi::pulse tick, const event & source);

    void set_input_bus (bussbyte b)
    {
        if (is_good_buss(b))
            m_input_buss = b;
    }

    bussbyte input_bus () const
    {
        return m_input_buss;
    }

    void set_timestamp (midi::pulse time)
    {
        m_timestamp = time;
        m_message.jack_stamp(double(time));
    }

    midi::pulse timestamp () const
    {
        return m_timestamp;
    }

    midi::byte channel () const
    {
        return m_channel;
    }

public:

    /**
     *  Calculates the value of the current timestamp modulo the given
     *  parameter.
     *
     * \param modtick
     *      The tick value to mod the timestamp against.  Usually the length
     *      of the pattern receiving this event.
     */

    void mod_timestamp (midi::pulse modtick)
    {
        if (modtick > 1)
            m_timestamp %= modtick;
    }

    void set_status (status s)
    {
        m_message[0] = midi::to_byte(s);
    }

    void set_status (midi::byte s);
    void set_channel (midi::byte channel);
    void set_channel_status (midi::byte eventcode, midi::byte channel);
    void set_meta_status (midi::meta metatype);

    void set_meta_status (midi::byte metatype)
    {
        set_meta_status(to_meta(metatype));
    }

    void set_status_keep_channel (midi::byte eventcode);
#if defined RTL66_THIS_FUNCTION_IS_USED
    void set_note_off (int note, midi::byte channel);
#endif
    bool set_midi_event
    (
        midi::pulse timestamp,
        const midi::bytes & buffer,
        size_t count
    );
    bool set_midi_event (const midi::message & msg);

    /**
     *  Note that we have ensured that status ranges from 0x80 to 0xFF.
     *  And recently, the status now holds the channel, redundantly.
     *  Unless the event is a meta event, in which case the channel is the
     *  number of the event.  We can return the bare status, or status with
     *  the channel stripped, for channel messages.
     */

    midi::byte status () const
    {
        return m_message[0];
    }

    midi::byte normalized_status () const
    {
        return midi::normalized_status(status());   /* may strip ch. nybble */
    }

    midi::byte get_status (midi::byte channel) const
    {
        return midi::mask_status(status()) | channel;
    }

    midi::byte get_meta_status () const
    {
        return midi::is_meta_msg(status()) ? channel() : 0 ;
    }

    bool valid_status () const
    {
        return midi::is_status_msg(status());
    }

    /**
     *  Checks that statuses match, clearing the channel nybble if needed.
     *
     * \param s
     *      Provides the desired status, without any channel nybble (that is,
     *      the channel is 0).
     *
     * \return
     *      Returns true if the event's status (after removing the channel)
     *      matches the status parameter.
     */

    bool match_status (midi::byte s) const
    {
        return (has_channel() ?
            midi::mask_status(status()) : status()) == s;
    }

    /**
     *  Checks the channel to see if the event's channel matches it, or if the
     *  event has no channel.  Used in the SMF 0 track-splitting code.  The
     *  value of 0xFF is Seq66's channel value that indicates that the event's
     *  channel() value is bogus.  It also means that the channel, if
     *  applicable, is encoded in the status byte.  This is our work-around to
     *  be able to hold a multi-channel SMF 0 track in a sequence.  In a Seq66
     *  SMF 0 track, every event has a channel.  In a Seq66 SMF 1 track, the
     *  events do not have a channel.  Instead, the channel is a global value of
     *  the sequence, and is stuffed into each event when the event is played,
     *  but not when written to a MIDI file.
     *
     * \param ch
     *      The channel to check for a match.
     *
     * \return
     *      Returns true if the given channel matches the event's channel.
     */

    bool match_channel (int ch) const
    {
        return is_null_channel(channel()) || midi::byte(ch) == channel();
    }

    /**
     *  Clears the most-significant-bit of both parameters, and sets them into
     *  the first and second bytes of m_message.  This function assumes a
     *  valid midi::message with one or two data bytes.
     *
     * \param d1
     *      The first byte value to set.
     *
     * \param d2
     *      The second byte value to set.
     */

    void set_data (midi::byte d0, midi::byte d1 = 0)
    {
        m_message[1] = mask_data(d0);
        m_message[2] = mask_data(d1);
    }

    /**
     *  Yet another overload.
     */

    void set_data
    (
        midi::pulse tstamp, midi::byte status,
        midi::byte d0, midi::byte d1
    );

    /**
     *  Clears the data, useful in reusing an event to hold incoming MIDI.
     */

    void clear_data ()
    {
        m_message[1] = m_message[2] = 0;
    }

    /**
     *  Clears a note-link. Does not (yet) touch the note to which it was
     *  linked.
     */

    void clear_link ()
    {
        unmark();
        unlink();
    }

    /**
     *  Retrieves only the first data byte from m_message[] and copies it into
     *  the parameter.
     *
     * \param d0 [out]
     *      The return reference for the first byte.
     */

    void get_data (midi::byte & d0) const
    {
        d0 = m_message[1];
    }

    /**
     *  Retrieves the two data bytes from m_message[] and copies each into its
     *  respective parameter.
     *
     * \param d0 [out]
     *      The return reference for the first byte.
     *
     * \param d1 [out]
     *      The return reference for the first byte.
     */

    void get_data (midi::byte & d0, midi::byte & d1) const
    {
        d0 = m_message[1];
        d1 = m_message[2];
    }

    /**
     *  Two alternative getters for the data bytes.  Useful for one-offs.
     */

    midi::byte d0 () const
    {
        return m_message[1];
    }

    void d0 (midi::byte b)
    {
        m_message[1] = b;
    }

    midi::byte d1 () const
    {
        return m_message[2];
    }

    void d1 (midi::byte b)
    {
        m_message[1] = b;
    }

    /**
     *  Increments the first data byte (m_message[1]) and clears the most
     *  significant bit.
     */

    void increment_d0 ()
    {
        d0(mask_data(d0() + 1));
    }

    /**
     *  Decrements the first data byte (m_message[1]) and clears the most
     *  significant bit.
     */

    void decrement_d0 ()
    {
        d0(mask_data(d0() - 1));
    }

    /**
     *  Increments the second data byte (m_message[1]) and clears the most
     *  significant bit.
     */

    void increment_d1 ()
    {
        d1(mask_data(d1() + 1));
    }

    /**
     *  Decrements the second data byte (m_message[1]) and clears the most
     *  significant bit.
     */

    void decrement_1 ()
    {
        d1(mask_data(d1() - 1));
    }

    bool append_meta_data
    (
        midi::meta metatype,
        const midi::bytes & data
    );
    bool set_text (const std::string & s);
    std::string get_text () const;

    bool append_sysex (midi::byte data);
    bool append_sysex (const midi::byte * dataptr, size_t count);

    bool append_sysex (const midi::bytes & bdata, size_t count)
    {
        return append_sysex(bdata.data(), count);
    }

    void reset_sysex ()
    {
        m_message.clear();                 // m_sysex.clear();
        m_message.push(midi::to_byte(status::sysex));
    }

    midi::message & get_message ()
    {
        return m_message;
    }

    const midi::message & get_message () const
    {
        return m_message;
    }

    midi::byte get_message (size_t i) const
    {
        return i < m_message.size() ? m_message[i] : 0 ;  /* or is F7 better? */
    }

    /**
     *  Resets and adds ex data.
     *
     * \param data
     *      Provides the SysEx/Meta data.  If not provided, nothing is done,
     *      and false is returned.
     *
     * \param len
     *      The number of bytes to set. STILL NEEDED???
     *
     * \return
     *      Returns true if the function succeeded.
     */

    bool set_sysex (const midi::bytes & data)
    {
        reset_sysex();
        return append_sysex(data, data.size());
    }

    void set_sysex_size (size_t len)
    {
        if (len == 0)
            m_message.clear();
        else
            m_message.resize(len);
    }

    size_t sysex_size () const
    {
        return is_sysex() ? m_message.event_byte_count() : 0 ;
    }

    /**
     *  The size of the message minus the header data.
     */

    size_t meta_data_size () const
    {
        return int(m_message.event_byte_count());
    }

    /**
     *  Determines if this event is a note-on event and is not already linked.
     */

    bool on_linkable () const
    {
        return is_note_on() && ! is_linked();
    }

    bool off_linkable () const
    {
        return is_note_off() && ! is_linked();
    }

    /**
     *  Determines if a Note Off event is linkable to this event (which is
     *  assumed to be a Note On event).  A test used in verify_and_link().
     *
     * \param e
     *      Normally this is a Note Off event.  This status is required.
     *
     * \return
     *      Returns true if the event is a Note Off, it's the same note as
     *      this note, and the Note Off is not yet linked.
     */

    bool off_linkable (buffer::iterator & eoff) const
    {
        return eoff->off_linkable() ? eoff->get_note() == get_note() : false ;
    }

    /**
     *  Sets m_has_link and sets m_link to the provided event pointer.
     *
     * \param ev
     *      Provides a pointer to the event value to set.  Since we're using
     *      an iterator, we can't use a null-pointer test that.  We assume the
     *      caller has checked that the value is not end() for the container.
     */

    void link (iterator ev)
    {
        m_linked = ev;
        m_has_link = true;
    }

    iterator link () const
    {
        return m_linked;        /* iterator could be invalid, though    */
    }

    bool is_linked () const
    {
        return m_has_link;
    }

    bool is_note_on_linked () const
    {
        return is_note_on() && is_linked();
    }

    bool is_note_unlinked () const
    {
        return is_strict_note() && ! is_linked();
    }

    void unlink ()
    {
        m_has_link = false;
    }

#if defined RTL66_SUPPORT_PAINTED_EVENTS

    void paint ()
    {
        m_painted = true;
    }

    void unpaint ()
    {
        m_painted = false;
    }

    bool is_painted () const
    {
        return m_painted;
    }

#endif  // defined RTL66_SUPPORT_PAINTED_EVENTS

    void mark ()
    {
        m_marked = true;
    }

    void unmark ()
    {
        m_marked = false;
    }

    bool is_marked () const
    {
        return m_marked;
    }

    void select ()
    {
        m_selected = true;
    }

    void unselect ()
    {
        m_selected = false;
    }

    bool is_selected () const
    {
        return m_selected;
    }

    /**
     *  Sets the event to a clock event.
     */

    void make_clock ()
    {
        m_message.clear();
        m_message.push(midi::to_byte(status::clk_clock));
    }

    midi::byte data (int index) const    /* index not checked, for speed */
    {
        return m_message[index + 1];
    }

    /**
     *  Assuming m_message[] holds a note, get the note number, which is in the
     *  first data byte, m_message[1].
     */

    midi::byte get_note () const
    {
        return m_message[1];
    }

    /**
     *  Sets the note number, clearing off the most-significant-bit and
     *  assigning it to the first data byte, m_message[0].
     *
     * \param note
     *      Provides the note value to set.
     */

    void set_note (midi::byte note)
    {
        m_message[1] = mask_data(note);
    }

    void transpose_note (int tn);

    /**
     *  Sets the note velocity, which is held in the second data byte, and
     *  clearing off the most-significant-bit, storing it in m_message[1].
     *
     * \param vel
     *      Provides the velocity value to set.
     */

    void note_velocity (int vel)
    {
        m_message[2] = mask_data(midi::byte(vel));
    }

    midi::byte note_velocity () const
    {
        return is_note() ? m_message[2] : 0 ;
    }

    bool is_note () const
    {
        return midi::is_note_msg(status());
    }

    bool is_note_on () const
    {
        return midi::is_note_on_msg(status());
    }

    bool is_note_off () const
    {
        return midi::is_note_off_msg(status());
    }

    bool is_strict_note () const
    {
        return midi::is_strict_note_msg(status());
    }

    bool is_selected_note () const
    {
        return is_selected() && is_note();
    }

    bool is_selected_note_on () const
    {
        return is_selected() && is_note_on();
    }

    bool is_controller () const
    {
        return midi::is_controller_msg(status());
    }

    bool is_pitchbend () const
    {
        return midi::is_pitchbend_msg(status());
    }

    bool is_playable () const
    {
        return midi::is_playable_msg(status()) || is_tempo();
    }

    bool is_selected_status (midi::byte s) const
    {
        return is_selected() &&
            midi::mask_status(status()) == midi::mask_status(s);
    }

    bool is_desired (midi::byte status, midi::byte cc) const;
    bool is_desired (midi::byte status, midi::byte cc, midi::byte data) const;
    bool is_desired_ex (midi::byte status, midi::byte cc) const;
    bool is_data_in_handle_range (midi::byte target) const;

    /**
     *  Some keyboards send Note On with velocity 0 for Note Off, so we
     *  provide this function to test that during recording.
     *
     * \return
     *      Returns true if the event is a Note On event with velocity of 0.
     */

    bool is_note_off_recorded () const
    {
        return is_note_off_velocity(status(), m_message[2]);
    }

    bool is_midi_start () const
    {
        return midi::is_midi_start_msg(status());
    }

    bool is_midi_continue () const
    {
        return midi::is_midi_continue_msg(status());
    }

    bool is_midi_stop () const
    {
        return midi::is_midi_stop_msg(status());
    }

    bool is_midi_clock () const
    {
        return midi::is_midi_clock_msg(status());
    }

    bool is_midi_song_pos () const
    {
        return midi::is_midi_song_pos_msg(status());
    }

    bool has_channel () const
    {
        return midi::is_channel_msg(status());
    }

    /**
     *  Indicates if the status value is a one-byte message (Program Change
     *  or Channel Pressure.  Channel is stripped, because sometimes we keep
     *  the channel.
     */

    bool is_one_byte () const
    {
        return midi::is_one_byte_msg(status());
    }

    /**
     *  Indicates if the status value is a two-byte message (everything
     *  except Program Change and Channel Pressure.  Channel is stripped,
     *  because sometimes we keep the channel.
     */

    bool is_two_bytes () const
    {
        return midi::is_two_byte_msg(status());
    }

    bool is_program_change () const
    {
        return midi::is_program_change_msg(status());
    }

    /**
     *  Indicates an event that has a line-drawable data item, such as
     *  velocity.  It is false for discrete data such as program/path number
     *  or Meta events.
     */

    bool is_continuous_event () const
    {
        return midi::is_continuous_event_msg(status());
    }

    /**
     *  Indicates if the event is a System Exclusive event or not.
     *  We're overloading the SysEx support to handle Meta events as well.
     *  Perhaps we need to split this support out at some point.
     */

    bool is_sysex () const
    {
        return midi::is_sysex_msg(status());
    }

    bool is_below_sysex () const
    {
        return midi::is_below_sysex_msg(status());
    }

    /**
     *  Indicates if the event is a Sense event or a Reset event.
     *  Currently ignored by Sequencer64.
     */

    bool is_sense_reset ()
    {
        return midi::is_sense_or_reset_msg(status());
    }

    /**
     *  Indicates if the event is a Meta event or not.
     *  We're overloading the SysEx support to handle Meta events as well.
     */

    bool is_meta () const
    {
        return midi::is_meta_msg(status());
    }

    bool is_meta_text () const
    {
        return is_meta() && midi::is_meta_text_msg(channel());
    }

    bool is_seq_spec () const
    {
        return midi::is_meta_seq_spec(status());
    }

    /**
     *  Indicates if we need to use extended data (SysEx or Meta).  If true,
     *  then channel() encodes the type of meta event.
     */

    bool is_ex_data () const
    {
        return midi::is_ex_data_msg(status());
    }

    bool is_system () const
    {
        return midi::is_system_msg(status());
    }

    /**
     *  Indicates if the event is a tempo event.  See sm_meta_event_names[].
     */

    bool is_tempo () const
    {
        return is_meta() && midi::is_tempo_msg(channel());
    }

    midi::bpm tempo () const;
    bool set_tempo (midi::bpm tempo);
    bool set_tempo (const midi::bytes & tt);

    /**
     *  Indicates if the event is a Time Signature event.  See
     *  sm_meta_event_names[].
     */

    bool is_time_signature () const
    {
        return is_meta() && midi::is_time_signature_msg(channel());
    }

    /**
     *  Indicates if the event is a Key Signature event.  See
     *  sm_meta_event_names[].
     */

    bool is_key_signature () const
    {
        return is_meta() && midi::is_key_signature_msg(channel());
    }

    void print (const std::string & tag = "") const;
    void print_note (bool showlink = true) const;
    std::string to_string () const;
    int get_rank () const;
    void rescale (midi::ppqn newppqn, midi::ppqn oldppqn);

private:    // used by friend eventlist

    /*
     * Changes timestamp.
     */

    bool jitter (int snap, int range, midi::pulse seqlength);
    bool tighten (int snap, midi::pulse seqlength);
    bool quantize (int snap, midi::pulse seqlength);

    /*
     * Changes the amplitude of d0 or d1, depending on the event.
     */

    bool randomize (int range);

};          // class event

/*
 * Global functions in the midi
 */

extern event create_tempo_event (midi::pulse tick, midi::bpm tempo);

}           // namespace midi

#endif      // RTL66_MIDI_EVENT_HPP

/*
 * event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

