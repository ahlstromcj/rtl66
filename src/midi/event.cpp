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
 * \file          event.cpp
 *
 *  This module declares/defines the base class for MIDI events.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2025-01-15
 * \license       GNU GPLv2 or above
 *
 *  A MIDI event (i.e. "track event") is encapsulated by the midi::event
 *  object.
 *
 *      -   Varinum delta time stamp.
 *      -   Event byte:
 *          -   MIDI event.
 *              -   Channel event (0x80 to 0xE0, channel in low nybble).
 *                  -   Data byte 1.
 *                  -   Data byte 2 (for all but patch and channel pressure).
 *              -   Non-channel event (0xF0 to 0xFF).
 *                  -   SysEx (0xF0), discussed below, includes data bytes.
 *                  -   Song Position (0xF2) includes data bytes.
 *                  -   Song Select (0xF3) includes data bytes.
 *                  -   The rest of the non-channel events don't include data
 *                      byte.
 *          -   Meta event (0xFF).
 *              -   Meta type byte.
 *              -   Varinum length.
 *              -   Data bytes.
 *          -   SysEx event.
 *              -   Start byte (0xF0) or continuation/escape byte (0xF7).
 *              -   Varinum length???
 *              -   Data bytes (not yet fully supported in event class).
 *              -   End byte (0xF7).
 *
 *  Running status is used, where status bytes of MIDI channel messages can be
 *  omitted if the preceding event is a MIDI channel message with the same
 *  status.  Running status continues across delta-times.
 *
 *  In Seq24/Seq66, none of the non-channel events are stored in an
 *  event object.  There is some provisional support for storing SysEx, but
 *  none of the support functions are yet called.  In midi::masterbus, there is
 *  support for recording SysEx data, but it is macro'ed out.  In rcsettings,
 *  there is an option for SysEx.  The midibus and performer objects also deal
 *  with Sysex.  But the midifile module does not read it -- it skips SysEx.
 *  Apparently, it does serve as a Thru for incoming SysEx, though.
 *  See this message thread:
 *
 *     http://sourceforge.net/p/seq66/mailman/message/1049609/
 *
 *  In Seq24/Seq66, the Meta events are handled directly, and they
 *  set up sequence parameters.
 *
 *  This module also defines the event::key object.
 */

#include "c_macros.h"                   /* not_nullptr(), errprint()        */
#include "midi/event.hpp"               /* midi::event class                */
#include "midi/calculations.hpp"        /* midi::rescale_tick(), etc.       */

namespace midi
{

/*
 * --------------------------------------------------------------
 *  event
 * --------------------------------------------------------------
 */

/**
 *  This constructor simply initializes all of the class members to default
 *  values.
 *
 *  Note that the MIDI status and data are stored in a MIDI message and
 *  we currently insure it has three data bytes for easier modification.
 */

event::event () :
    m_input_buss    (null_buss()),              /* 0xFF                     */
    m_timestamp     (0),
    m_message       (),                         /* now a midi::message      */
    m_channel       (null_channel()),           /* 0x80                     */
    m_linked        (),
    m_has_link      (false),
    m_selected      (false),
#if defined RTL66_SUPPORT_PAINTED_EVENTS
    m_marked        (false),
    m_painted       (false)
#else
    m_marked        (false)
#endif
{
    m_message.push(midi::to_byte(status::note_off));
    m_message.push(0);
    m_message.push(0);
}

/**
 *  This constructor initializes some of the class members to default
 *  values, and provides the most oft-changed values a parameters.
 *
 * \param tstamp
 *      Provides the timestamp of this event.
 *
 * \param s
 *      Provides the status value.  The channel nybble is cleared, since the
 *      channel is generally provided by the settings of the sequence.
 *      However, this value should include the channel if applicable!
 *
 * \param d0
 *      Provides the first data byte.  There is no default value.
 *
 * \param d1
 *      Provides the second data byte.  There is no default value.
 */

event::event (midi::pulse tstamp, midi::byte s, midi::byte d0, midi::byte d1) :
    m_input_buss    (null_buss()),          /* 0xFF                         */
    m_timestamp     (tstamp),
    m_message       (),                     /* now a midi::message          */
    m_channel       (mask_channel(s)),
    m_linked        (),
    m_has_link      (false),
    m_selected      (false),
#if defined RTL66_SUPPORT_PAINTED_EVENTS
    m_marked        (false),
    m_painted       (false)
#else
    m_marked        (false)
#endif
{
    m_message.push(s);
    m_message.push(d0);
    m_message.push(d1);
}

/**
 *  Creates a tempo event.
 */

event::event (midi::pulse tstamp, midi::bpm tempo) :
    m_input_buss    (null_buss()),
    m_timestamp     (tstamp),
    m_message          (),                     /* now a midi::message ????     */
    m_channel       (midi::to_byte(meta::set_tempo)),
    m_linked        (),
    m_has_link      (false),
    m_selected      (false),
#if defined RTL66_SUPPORT_PAINTED_EVENTS
    m_marked        (false),
    m_painted       (false)
#else
    m_marked        (false)
#endif
{
    set_tempo(tempo);                       /* fills the m_message vector      */
}

/**
 *  Creates a note event.
 */

event::event
(
    midi::pulse tstamp,
    midi::status notekind,
    midi::byte channel,
    int note,
    int velocity
) :
    m_input_buss    (null_buss()),
    m_timestamp     (tstamp),
    m_message          (),                     /* now a midi::message          */
    m_channel       (channel),
    m_linked        (),
    m_has_link      (false),
    m_selected      (false),
#if defined RTL66_SUPPORT_PAINTED_EVENTS
    m_marked        (false),
    m_painted       (false)
#else
    m_marked        (false)
#endif
{
    if (is_null_channel(channel))
    {
        m_channel = 0;
    }
    else
    {
        midi::byte statbyte = midi::byte(notekind);
        midi::byte chan = midi::mask_channel(channel);
        statbyte = midi::add_channel(statbyte, chan);
        notekind = midi::status(statbyte);
        m_channel = chan;
    }
    m_message.push(notekind);
    m_message.push(midi::byte(note));
    m_message.push(midi::byte(velocity));
}

/**
 *  This copy constructor initializes most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the members are not set to useful
 *  values when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      Note that now events are also copied when creating the editable_events
 *      container, so this function is even more important.  The event links,
 *      for linking Note Off events to their respective Note On events, are
 *      dropped.  Generally, they will need to be reconstituted by calling the
 *      eventlist::verify_and_link() function.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx events was not complete in Seq24, and it is still not
 *      complete in Seq66.  Nor does it currently bother with the
 *      links, as noted above.
 *
 * \param rhs
 *      Provides the event object to be copied.
 */

event::event (const event & rhs) :
    m_input_buss    (rhs.m_input_buss),
    m_timestamp     (rhs.m_timestamp),
    m_message          (rhs.m_message),
    m_channel       (rhs.m_channel),
    m_linked        (rhs.m_linked),         /* for vector implemenation */
    m_has_link      (rhs.m_has_link),       /* m_linked has 2 linkers!  */
    m_selected      (rhs.m_selected),
#if defined RTL66_SUPPORT_PAINTED_EVENTS
    m_marked        (rhs.m_marked),
    m_painted       (rhs.m_painted)
#else
    m_marked        (rhs.m_marked)
#endif
{
    // No code needed
}

/**
 *  This principal assignment operator sets most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the member are not set to useful value
 *  when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      This function now copies the SysEx data, but the inclusion of SysEx
 *      events was not complete in Seq24, and it is still not complete in
 *      Seq66.  Nor does it currently bother with the link the event
 *      might have, except in the std::vector implementation.
 *
 * \param rhs
 *      Provides the event object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of events.
 */

event &
event::operator = (const event & rhs)
{
    if (this != &rhs)
    {
        m_input_buss    = rhs.m_input_buss;
        m_timestamp     = rhs.m_timestamp;
        m_message          = rhs.m_message;
        m_channel       = rhs.m_channel;
        m_linked        = rhs.m_linked;             /* vector implemenation */
        m_has_link      = rhs.m_has_link;           /* two linkers!         */
        m_selected      = rhs.m_selected;           /* false instead?       */
        m_marked        = rhs.m_marked;             /* false instead?       */
#if defined RTL66_SUPPORT_PAINTED_EVENTS
        m_painted       = rhs.m_painted;            /* false instead?       */
#endif
    }
    return *this;
}

/**
 *
 */

event::~event ()
{
    // Automatic destruction of members is enough
}

/**
 *  Copies a subset of data to the calling event.  Used in the
 *  sequence::put_event_on_bus() function to add a timestamp to outgoing
 *  events, a lapse in earlier versions of the SeqXX series. We don't
 *  need the following settings in an event merely to be played.
 *
 *      m_channel       = source.m_channel;
 *      m_linked        = source.m_linked;
 *      m_has_link      = source.m_has_link;
 *      m_selected      = source.m_selected;
 *      m_marked        = source.m_marked;
 *      m_painted       = source.m_painted;
 *
 *  It is assumed that "this" event has been default constructed.
 *
 * \param tick
 *      Provides the current tick (pulse) time of playback.  This always
 *      increases, and never loops back.
 *
 * \param source
 *      The event to be sent.  We need just some items from this
 *      event.
 */

void
event::prep_for_send (midi::pulse tick, const event & source)
{
    m_input_buss    = source.m_input_buss;
    m_timestamp     = tick;
    m_message          = source.m_message;    /* holds event data or SysEx bytes  */
}

/**
 *  If the current timestamp equal the event's timestamp, then this
 *  function returns true if the current rank is less than the event's
 *  rank.
 *
 *  Otherwise, it returns true if the current timestamp is less than
 *  the event's timestamp.
 *
 * \warning
 *      The less-than operator is supposed to support a "strict weak
 *      ordering", and is supposed to leave equivalent values in the same
 *      order they were before the sort.  However, every time we load and
 *      save our sample MIDI file, events get reversed.  Here are
 *      program-changes that get reversed:
 *
\verbatim
        Save N:     0070: 6E 00 C4 48 00 C4 0C 00  C4 57 00 C4 19 00 C4 26
        Save N+1:   0070: 6E 00 C4 26 00 C4 19 00  C4 57 00 C4 0C 00 C4 48
\endverbatim
 *
 *      The 0070 is the offset within the versions of the b4uacuse-seq64.midi
 *      file.
 *
 *      Because of this issue, and the very slow speed of loading a MIDI file
 *      when built for debugging, we explored using an std::multimap instead
 *      of an std::list.  This worked better than a list, for loading MIDI
 *      events, but may cause the upper limit of the number of playing
 *      sequences to drop a little, due to the overhead of incrementing
 *      multimap iterators versus list iterators).  Now we use only the
 *      std::vector implementation, slightly faster than using std::list.
 *
 * \param rhs
 *      The object to be compared against.
 *
 * \return
 *      Returns true if the time-stamp and "rank" are less than those of the
 *      comparison object.
 */

bool
event::operator < (const event & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return get_rank() < rhs.get_rank();
    else
        return m_timestamp < rhs.m_timestamp;
}

/**
 *  Indicates that the events are "identical".  This function is not a
 *  replacement for operator < ().  It is meant to be used in a brute force
 *  search for one particular event in the sorted event list.
 *
 *  SysEx (or text) data are not checked, just the status and channel values..
 */

bool
event::match (const event & target) const
{
    bool result = false;
    bool ignore_ts = is_null_pulse(target.timestamp());
    if (ignore_ts || timestamp() == target.timestamp())
    {
        result =
        (
            status() == target.status() &&
            channel() == target.channel()
        );
        if (result && ! is_meta())
        {
            result =
            (
                m_message[0] == target.m_message[0] &&
                m_message[1] == target.m_message[1]
            );
        }
    }
    return result;
}

/**
 *  Returns true if the event's status is *not* a control-change, but does
 *  match the given status OR if the event's status is a control-change that
 *  matches the given status, and has a control value matching the given
 *  control-change value.
 *
 * \param s
 *      The event status to be checked.
 *
 * \param cc
 *      The controller value to be matched, for control-change events.
 */

bool
event::is_desired (midi::byte s, midi::byte cc) const
{
    bool result;
    if (midi::is_tempo_msg(s))
    {
        result = is_tempo();
    }
    else
    {
        midi::byte stb = midi::mask_status(s);
        result = stb == m_message[0];                      /* d0 */
        if (result && midi::is_controller_msg(s))
            result = m_message[1] == cc;                   /* d1 */
    }
    return result;
}

#if defined RTL66_ENABLE_THIS_CODE

/**
 *  This is a bit tricky. The range of pixels in the data pane is 0 to 127
 *  or 0 to 64 (in shrunken mode). For note events, the ranges is also
 *  0 to 127. The diameter of tempo grab handle is currently s_handled_d = 8,
 *  which can represent a wide range in tempos.
 *
 *      event::is_data_in_handle_range (midi::byte target) const
 *
 *  See the uncommented duplicate below! Which is better?
 */

bool
event::is_data_in_handle_range (midi::byte target) const
{
    bool result = false;
    if (is_tempo())
    {
        static const midi::bpm s_delta = note_value_to_tempo(4);
        midi::bpm t = tempo();
        midi::bpm tdesired = note_value_to_tempo(target);
        result = t >= (tdesired - s_delta) && t <= (tdesired + s_delta);
    }
    else
    {
        static const midi::byte s_delta = 2;                /* seq32 provision  */
        static const midi::byte max = c_midibyte_value_max - s_delta;
        midi::byte datum = is_one_byte() ? d0() : d1() ;
        result = target >= s_delta && target <= max;
        if (result)
        {
            result = datum >= (target - s_delta) &&
                datum <= (target + s_delta);
        }
    }
    return result;
}

#endif

bool
event::is_desired (midi::byte s, midi::byte cc, midi::byte data) const
{
    bool result;
    if (midi::is_tempo_msg(s))
    {
        result = is_tempo();
    }
    else
    {
        result = midi::match_status(s, status());
        if (result && (midi::is_controller_msg(s)))
        {
            result = status() == cc;
            if (result)
                result = is_data_in_handle_range(data); /* check d0/d1()    */
        }
    }
    return result;
}

/**
 *  We should also match tempo events here.  But we have to treat them
 *  differently from the matched status events.
 */

bool
event::is_desired_ex (midi::byte s, midi::byte cc) const
{
    bool result;                            /* is_desired_cc_or_not_cc      */
    bool match = match_status(s);
    if (midi::is_controller_msg(s))
    {
        result = match && m_message[1] == cc;  /* correct status & correct CC  */
    }
    else
    {
        if (is_tempo())
            result = true;                  /* Set tempo always editable    */
        else
            result = match;                 /* correct status and not CC    */
    }
    return result;
}

bool
event::is_data_in_handle_range (midi::byte target) const
{
    static const midi::byte delta = 2;                  /* seq32 provision  */
    static const midi::byte max = c_byte_value_max - delta;
    midi::byte datum = is_one_byte() ? d0() : d1() ;
    bool result = target >= delta && target <= max;
    if (result)
        result = datum >= (target - delta) && datum <= (target + delta);

    return result;
}

/**
 *  Sometimes we need to alter the event completely.
 */

void
event::set_data
(
    midi::pulse tstamp,
    midi::byte status,
    midi::byte d0,
    midi::byte d1
)
{
    set_timestamp(tstamp);
    set_status(status);
    set_data(d0, d1);
}

/**
 *  Transpose the note, if possible.
 *
 * \param tn
 *      The amount (positive or negative) to transpose a note.  If the result
 *      is out of range, the transposition is not performered.
 */

void
event::transpose_note (int tn)
{
    int note = int(m_message[0]) + tn;
    if (note >= 0 && note < midi::data_max)
        m_message[0] = midi::byte(note);
}

void
event::set_channel (midi::byte channel)
{
    if (is_null_channel(channel))
    {
        m_channel = channel;
    }
    else
    {
        int chan = mask_channel(channel);           /* clears status nybble */
        m_channel = chan;
        if (has_channel())                          /* a channel message    */
            m_message[0] = add_channel(m_message[0], channel);
    }
}

/**
 *  Sets the status member to the value of status.  If \a status is a
 *  channel event, then the channel portion of the status is cleared using
 *  a bitwise AND against EVENT_GET_STATUS_MASK.  This version is basically
 *  the Seq24 version with the additional setting of the Seq66-specific
 *  m_channel member.
 *
 *  Found in yet another fork of seq24: // ORL fait de la merde
 *  He also provided a very similar routine: set_status_midibus().
 *
 *  Stazed:
 *
 *      The record parameter, if true, does not clear channel portion
 *      on record for channel specific recording. The channel portion is
 *      cleared in track::stream_event() by calling set_status() (record
 *      = false) after the matching channel is determined.  Otherwise, we use
 *      a bitwise AND to clear the channel portion of the status.  All events
 *      will be stored without the channel nybble.  This is necessary since
 *      the channel is appended by midibus::play() based on the track.
 *
 *  Instead of adding a "record" parameter to set_status(), we provide a more
 *  specific function, set_status_keep_channel(), for use in the midi::masterbus
 *  class.  This usage also has the side-effect of allowing the usage of
 *  channel in the MIDI-control feature.
 *
 * \param s
 *      The status byte, perhaps read from a MIDI file or edited in the
 *      sequencer's event editor.  Sometimes, this byte will have the channel
 *      nybble masked off.  If that is the case, the eventcode/channel
 *      overload of this function is more appropriate.  Only values with the
 *      highest bit set are allowed, as per the MIDI specification.
 */

void
event::set_status (midi::byte s)
{
    if (midi::is_system_msg(s))             /* 0xF0 and above               */
    {
        m_message[0] = s;
        m_channel = null_channel();         /* channel "not applicable"     */
    }
    else if (is_channel_msg(s))             /* 0x80 to 0xEF                 */
    {
        m_message[0] = mask_status(s);
        m_channel = mask_channel(s);
    }
}

/**
 *  This overload is useful when synthesizing events, such as converting a
 *  Note On event with a velocity of zero to a Note Off event.  See its usage
 *  in midifile and qseventslots, for example.
 *
 * \param s
 *      The status byte, perhaps read from a MIDI file. If the event is a
 *      channel event, it will have its channel updated via the \a channel
 *      parameter as well.
 *
 * \param channel
 *      The channel byte.  Combined with the event-code, this makes a valid
 *      MIDI "status" byte.  This byte is masked to guarantee the high nybble
 *      is clear.
 */

void
event::set_channel_status (midi::byte s, midi::byte channel)
{
    m_message[0] = s;
    set_channel(channel);
}

/**
 *  This function is used in recording to preserve the input channel
 *  information for deciding what to do with an incoming MIDI event.
 *  It replaces stazed's set_status() that had an optional "record"
 *  parameter. This call allows channel to be detected and used in MIDI
 *  control events. It "keeps" the channel in the status byte.
 *
 * \param eventcode
 *      The status byte, generally read from the MIDI buss.  If it is not a
 *      channel message, the channel is not modified.
 */

void
event::set_status_keep_channel (midi::byte eventcode)
{
    m_message[0] = eventcode;
    if (is_channel_msg(eventcode))
        m_channel = mask_channel(eventcode);
}

#if defined RTL66_THIS_FUNCTION_IS_USED

void
event::set_note_off (int note, midi::byte channel)
{
    midi::byte chan = mask_channel(channel);
    m_message[0] = add_channel(midi::to_byte(status::note_off), chan);
    set_data(midi::byte(note), 0);
}

#endif

/**
 *  In relation to issue #206.
 *
 *  Combines a bunch of common operations in getting events.  Code used in:
 *
 *      -   midi_in_jack::api_get_midi_event(event *)
 *      -   midi_alsa_info::api_get_midi_event(event *)
 *
 *  Can we use it in these contexts?
 *
 *      -   wrkfile::NoteArray(int, int)
 *      -   midifile::parse_smf_1(...)      [very unlikely]
 *
 *  Some keyboards send Note On with velocity 0 for Note Off, so we take care
 *  of that situation here by creating a Note Off event, with the channel
 *  nybble preserved. Note that we call event::set_status_keep_channel()
 *  instead of using stazed's set_status function with the "record" parameter.
 *  Also, we have to mask in the actual channel number.
 *
 *  Encapsulates some common code.  This function assumes we have already set
 *  the status and data bytes.
 */

bool
event::set_midi_event
(
    midi::pulse timestamp,
    const midi::bytes & buffer,
    size_t count
//  const midi::byte * buffer,
//  int count
)
{
    bool result = true;
    set_timestamp(timestamp);
//  set_sysex_size(count);
    if (count == 0)             /* portmidi: analyze the event to get count */
    {
        if (is_two_byte_msg(buffer[0]))
            count = 3;
        else if (is_one_byte_msg(buffer[0]))
            count = 2;
        else
            count = 1;
    }
    if (count == 3)
    {
        set_status_keep_channel(buffer[0]);
        set_data(buffer[1], buffer[2]);
        if (is_note_off_recorded())
        {
            midi::byte channel = mask_channel(buffer[0]);
            midi::byte status = add_channel
            (
                midi::to_byte(status::note_off), channel
            );
            set_status_keep_channel(status);
        }
    }
    else if (count == 2)
    {
        set_status_keep_channel(buffer[0]);
        set_data(buffer[1]);
    }
    else if (count == 1)
    {
        set_status(buffer[0]);
        clear_data();
    }
    else
    {
        if (midi::is_sysex_msg(buffer[0]))
        {
            reset_sysex();            /* set up for sysex if needed   */
            set_sysex_size(buffer.size());
            if (! append_sysex(buffer, count))
            {
                errprint("event::append_sysex() failed");
            }
        }
        else
            result = false;
    }
    return result;
}

/**
 *  An overload. One set of issues:
 *
 *      -   The timestamp is used only in JACK at present.
 *      -   It is a double value. The highest integer it can store without
 *          losing precision is 2^53 (9007199254740992). The value of
 *          LONG_MAX is 2^31-1 (2147483647).  The value of LLONG_MAX is
 *          2^63-1 (9223372036854775807).
 *      -   It can hold pulses or JACK frame counts. Be careful!
 */

bool
event::set_midi_event (const midi::message & msg)
{
    return set_midi_event
    (
        msg.time_stamp(), msg.event_bytes(), int(msg.size())
    );
}

/**
 *  If the event is a text event, then this function converts the midi::bytes
 *  to a string.
 *
 * \return
 *      Returns the text if valid, otherwise returns an empty string. Note
 *      that the text is in "midi-bytes" format, where characters greater
 *      than 127 are encodes as a hex value, "\xx".
 */

std::string
event::get_text () const
{
    return get_meta_event_text(m_message.event_bytes());   /* calculations.cpp */
}

bool
event::set_text (const std::string & s)
{
    return set_meta_event_text(m_message.event_bytes(), s);
}

/**
 *  An overload for logging SYSEX data byte-by-byte. If the byte is F7
 *  and no bytes have been pushed, then this is a SysEx Continue.
 *  and continues the SysEx, rather than ending it.
 *
 * \param data
 *      A single MIDI byte of data, assumed to be part of a SYSEX message
 *      event.
 *
 * \return
 *      Returns true if the event is not a SysEx-end event.
 */

bool
event::append_sysex (midi::byte data)
{
    bool firstbyte = m_message.empty();
    m_message.push(data);
    return firstbyte || ! midi::is_sysex_end_msg(data);
}

/**
 *  Appends SYSEX data to a new buffer.  We now use a vector instead of an
 *  array, so there is no need for reallocation and copying of the current
 *  SYSEX data.  The data represented by data and dsize is appended to that
 *  data buffer.
 *
 * \param data
 *      Provides the additional SysEx/Meta data.  If not provided, nothing is
 *      done, and false is returned.
 *
 * \param dsize
 *      Provides the size of the additional SYSEX data.  If not provided,
 *      nothing is done.
 *
 * \return
 *      Returns true if there was data to add.  The End-of-SysEx byte is
 *      included.
 */

bool
event::append_sysex (const midi::byte * data, size_t dsize)
{
    bool result = not_nullptr(data) && (dsize > 0);
    if (result)
    {
        for (size_t i = 0; i < dsize; ++i)
        {
            m_message.push(data[i]);
            if (is_sysex_end_msg(data[i]))
                break;
        }
    }
    else
    {
        errprint("event::append_sysex(): null parameters");
    }
    return result;
}

/**
 *  Sets a Meta event.  Meta events have a status byte of EVENT_MIDI_META ==
 *  0xff and a channel value that reflects the type of Meta event (e.g. 0x51
 *  for a "Set Tempo" event). We also (redundantly) store it it m_message[1].
 *
 *  Note that the data bytes (if any) for this event will still need to be
 *  added to the event via (for example) the append_sysex(), set_sysex(),
 *  or append_meta_data() functions.
 *
 * \param metatype
 *      Indicates the type of meta event.
 */

void
event::set_meta_status (midi::meta metatype)
{
    set_status(status::meta_msg);                       /* sets m_message[0]   */
    m_message[1] = m_channel = midi::to_byte(metatype);
}

/**
 *  This function appends Meta-event data from a vector to a new buffer.
 *
 *      0xff mm len data...
 *
 *  Appends Meta-event data to a new buffer.  Similar to append_sysex(), but
 *  useful for holding the data for a Meta event.  Please note that Meta
 *  events and SysEx events shared the same "extended" data buffer that
 *  originated to support SysEx.
 *
 *  Also see set_meta_status(), which, like this function, sets the
 *  event::m_channel_member to indicate the type of Meta event, but, unlike
 *  this function, leaves the data alone.  Also note that the set_status()
 *  call in midifile flags the event as a Meta event.  The handling of Meta
 *  events is not yet uniform between all the modules.
 *
 * \param metatype
 *      Provides the type of the Meta event, which is stored in the m_channel
 *      member.
 *
 * \param data
 *      Provides the Meta event's data as a vector.
 *
 * \return
 *      Returns false if an error occurred, and the caller needs to stop
 *      trying to process the data.
 */

bool
event::append_meta_data
(
    midi::meta metatype,
    const midi::bytes & data
)
{
    midi::bytes vlen = varinum_to_bytes(midi::ulong(data.size()));
    m_message.clear();                     /* empty it to reconstruct it       */
    m_message.push(0);                     /* allocate message::m_message[0]      */
    m_message.push(0);                     /* allocate message::m_message[1]      */
    set_meta_status(metatype);          /* set m_message[0] and [1]            */
    for (auto b : vlen)
        m_message.push(b);

#if defined RTL66_USE_MESSAGE_HEADER_SIZE
    m_message.log_header_size();           /* log the offset to actual data    */
#endif

    for (auto b : data)                 /* add the actual data              */
         m_message.push(b);

    return true;
}

/**
 *  Prints out the timestamp, data size, the current status byte, channel
 *  (which is the type value for Meta events), any SysEx or
 *  Meta data if present, or the two data bytes for the status byte.
 *
 *  There's really no percentage in converting this code to use std::cout, we
 *  feel.  We might want to make it contingent on the --verbose option at some
 *  point.
 */

void
event::print (const std::string & tag) const
{
    std::string buffer = to_string();
    if (tag.empty())
        printf("%s", buffer.c_str());
    else
        printf("%s: %s", tag.c_str(), buffer.c_str());
}

void
event::print_note (bool showlink) const
{
    if (is_note())
    {
        bool shownote = is_note_on() || (is_note_off() && ! showlink);
        if (shownote)
        {
            std::string type = is_note_on() ? "On " : "Off" ;
            char channel[8];
            if (m_channel == null_channel())
            {
                channel[0] = '-';
                channel[1] = 0;
            }
            else
            {
                snprintf(channel, sizeof channel, "%1x", int(m_channel));
            }
            printf
            (
                "%06ld Note %s:%s %3d Vel %02X",
                m_timestamp, type.c_str(), channel,
                int(m_message[0]), int(m_message[1])
            );
            if (is_linked() && showlink)
            {
                const_iterator mylink = link();
                printf(" --> ");
                mylink->print_note(false);
            }
            else
                printf("\n");
        }
    }
}

/**
 *  Prints out the timestamp, data size, the current status byte, channel
 *  (which is the type value for Meta events), any SysEx or
 *  Meta data if present, or the two data bytes for the status byte.
 *
 *  There's really no percentage in converting this code to use std::cout, we
 *  feel.  We might want to make it contingent on the --verbose option at some
 *  point.
 */

std::string
event::to_string () const
{
    midi::pulse ts = timestamp();           /* normal event timestamp       */
    if (is_sysex() || is_meta())
        ts = get_message().time_stamp();    /* message jack/pulse timestamp */

    char tmp[64];
    (void) snprintf(tmp, sizeof tmp, "[%06ld] (", long(ts));

    std::string result = tmp;
    if (is_sysex())
    {
        result += "SysEx) ";
    }
    else if (is_meta())
    {
        result += "Meta ) ";
    }
    else
    {
        result += is_linked() ? "L" : " ";
        result += is_marked() ? "M" : " ";
        result += is_selected() ? "S" : " ";
//      result += is_painted() ? "P" : " ";
        result += is_midi_clock() ? "C" : " ";
        result += ") ";
    }
    if (is_sysex())
    {
        result += get_message().to_string();
        result += "\n";
    }
    else if (is_meta())
    {
        result += get_message().to_string();
        result += "\n";
    }
    else
    {
        const char * label = is_meta() ? "type" : "channel" ;
        (void) snprintf
        (
            tmp, sizeof tmp, "event 0x%02X %s 0x%02X d0=%d d1=%d\n",
            unsigned(status()), label, unsigned(m_channel),
            int(m_message[0]), int(m_message[1])
        );
        result += tmp;
    }
    return result;
}

void
event::rescale (midi::ppqn newppqn, midi::ppqn oldppqn)
{
    set_timestamp(rescale_tick(timestamp(), newppqn, oldppqn));
}

/**
 *  This function is used in sorting MIDI status events (e.g. note on/off,
 *  aftertouch, control change, etc.)  The sort order is not determined by the
 *  actual status values.
 *
 *  The ranking, from high to low, is note off, note on, aftertouch, channel
 *  pressure, and pitch wheel, control change, and program changes.  The lower
 *  the ranking, the more upfront an item comes in the sort order, given the
 *  same time-stamp.
 *
 *  We also now consider SysEx and Meta event. Meta comes first, SysEx comes
 *  last.
 *
 * Note:
 *      We could add the channel number as part of the ranking. Sound?
 *
 * \return
 *      Returns the rank of the current status byte.
 */

int
event::get_rank () const
{
    int result;
    if (is_sysex())
    {
        result = 0x3000;
    }
    else if (is_meta())
    {
        result = 0x0030;
    }
    else
    {
        midi::byte eventcode = mask_status(status());
        midi::status s = to_status(eventcode);
        switch (s)
        {
            case midi::status::note_off:
                result = 0x2000 + get_note();
                break;

            case midi::status::note_on:
                result = 0x1000 + get_note();
                break;

            case midi::status::aftertouch:
            case status::channel_pressure:
            case status::pitch_wheel:
                result = 0x0050;
                break;

            case midi::status::control_change:
                result = 0x0020;
                break;

            case midi::status::program_change:
                result = 0x0010;
                break;

            default:
                result = 0;
                break;
            }
            if (result != 0)
                result += mask_channel(status()) << 8;
    }
    return result;
}

/**
 *  Calculates the tempo from the stored event bytes, if the event is a Tempo
 *  meta-event and has valid data.  Remember that we are overloading the SysEx
 *  support to hold Meta-event data.
 *
 *  The data in m_message for a tempo event is: FF 51 03 tt tt tt
 *
 * \return
 *      Returns the result of calculating the tempo from the three data bytes.
 *      If an error occurs, 0.0 is returned.
 */

midi::bpm
event::tempo () const
{
    midi::bpm result = 0.0;
    if (is_tempo() && sysex_size() == 3)
    {
        /*
        midi::byte b[3];
        b[0] = m_message[1];                  // convert vector to array type //
        b[1] = m_message[2];
        b[2] = m_message[3];
        result = bpm_from_bytes(b);
        */
        midi::bytes tt;
        tt.push_back(m_message[3]);
        tt.push_back(m_message[4]);
        tt.push_back(m_message[5]);
        result = bpm_from_bytes(tt);
    }
    return result;
}

/**
 *  The inverse of tempo().  First, we convert beats/minute to a tempo
 *  microseconds value.  Then we convert the microseconds to three tempo
 *  bytes.
 */

bool
event::set_tempo (midi::bpm tempo)
{
    double us = tempo_us_from_bpm(tempo);
    midi::bytes tt;
    return tempo_us_to_bytes(tt, midi::bpm(us)) ? set_sysex(tt) : 0 ;
}

bool
event::set_tempo (const midi::bytes & tt)
{
    double ttus = tempo_us_from_bytes(tt);
    bool result = ttus > 0.0;
    if (result)
        set_sysex(tt);

    return result;
}

/*
 *  Helper functions for alteration of events.
 */

/**
 *  Modifies the velocity (for notes) or some other amplitude. However,
 *  the caller will like not want to change the amplitude of a Note On with
 *  velocity 0. Let the caller beware!
 *
 * \param range
 *      The range of the changes up and down. Not used if 0 or less.
 *
 * \return
 *      Returns true if the amplitude was actually jittered.
 */

bool
event::randomize (int range)
{
    bool result = range > 0;
    if (result)
    {
        bool twobytes = is_two_bytes();
        int datum = int(twobytes ? m_message[1] : m_message[0]);
#if defined RTL66_USE_UNIFORM_INT_DISTRIBUTION
        int delta = midi::randomize_uniformly(range);
#else
        int delta = midi::randomize(range);
#endif
        result = delta != 0;
        if (result)
        {
            midi::byte d = clamp_midi_value(datum + delta);
            if (twobytes)
                m_message[1] = d;
            else
                m_message[0] = d;
        }
    }
    return result;
}

/**
 *  Modifies the timestamp of the event by plus or minus the range value.
 *
 * \param range
 *      The range of the changes up and down. Not used if 0 or less.
 *
 * \return
 *      Returns true if the timestamp was actually jittered.
 */

bool
event::jitter (int snap, int range, midi::pulse seqlength)
{
    bool result = range > 0;
    if (result)
    {
#if defined RTL66_USE_UNIFORM_INT_DISTRIBUTION
        midi::pulse delta = midi::pulse(midi::randomize_uniformly(range));
#else
        midi::pulse delta = midi::pulse(midi::randomize(range));
#endif
        result = delta != 0;
        if (result)
        {
            if (delta < -snap)
                delta = -snap + 1;
            else if (delta > snap)
                delta = snap - 1;

            midi::pulse tstamp = timestamp() + delta;
            if (tstamp >= seqlength)
                tstamp = seqlength - 1;
            else if (tstamp < 0)
                tstamp = 0;

            set_timestamp(tstamp);
        }
    }
    return result;
}

/**
 *  Division by 2 "tightens" toward the nearest snap time.
 *
 * \param snap
 *      The time boundary length to snap to. A common value is the number of
 *      ticks (pulses) in a 16th note.
 *
 * \param seqlength
 *      The length in ticks of the pattern that holds this event.
 *
 * \return
 *      Returns true if the time-stamp was actually altered.
 */

bool
event::tighten (int snap, midi::pulse seqlength)
{
    bool result = snap > 0;
    if (result)
    {
        midi::pulse t = timestamp();
        midi::pulse tremainder = t % snap;
        midi::pulse tdelta;
        if (tremainder < snap / 2)
            tdelta = -(tremainder / 2);
        else
            tdelta = (snap - tremainder) / 2;

        result = tdelta != 0;
        if (result)
        {
            t += tdelta;
            if (t >= seqlength)
                t = seqlength - 1;
            else if (t < 0)
                t = 0;

            set_timestamp(t);
        }
    }
    return result;
}

/**
 *  Quantizes the time-stamp toward the nearest snap time.
 *
 * \param snap
 *      The time boundary length to snap to. A common value is the number of
 *      ticks (pulses) in a 16th note.
 *
 * \param seqlength
 *      The length in ticks of the pattern that holds this event.
 *
 * \return
 *      Returns true if the time-stamp was actually altered.
 */

bool
event::quantize (int snap, midi::pulse seqlength)
{
    bool result = snap > 0;
    if (result)
    {
        midi::pulse t = timestamp();
        midi::pulse tremainder = t % snap;
        midi::pulse tdelta;
        if (tremainder < snap / 2)
            tdelta = -tremainder;
        else
            tdelta = (snap - tremainder);

        result = tdelta != 0;
        if (result)
        {
            t += tdelta;
            if (t >= seqlength)
                t = seqlength - 1;
            else if (t < 0)
                t = 0;

            set_timestamp(t);
        }
    }
    return result;
}

/*---------------------------------------------------------------------
 *  event::key
 *---------------------------------------------------------------------*/

/**
 *  Principal event::key constructor.
 *
 * \param tstamp
 *      The time-stamp is the primary part of the key.  It is the most
 *      important key item.
 *
 * \param rank
 *      Rank is an arbitrary number used to order events that have the
 *      same time-stamp.  See the event::get_rank() function for more
 *      information.
 */

event::key::key (midi::pulse tstamp, int rank) :
    m_timestamp (tstamp),
    m_rank      (rank)
{
    // Empty body
}

/**
 *  Event-based constructor.  This constructor makes it even easier to
 *  create an key.  Note that the call to event::get_rank() makes a
 *  simple calculation based on the status of the event.
 *
 * \param rhs
 *      Provides the event key to be copied.
 */

event::key::key (const event & rhs) :
    m_timestamp (rhs.timestamp()),
    m_rank      (rhs.get_rank())
{
    // Empty body
}

/**
 *  Provides the minimal operator needed to sort events using an key.
 *
 * \param rhs
 *      Provides the event key to be compared against.
 *
 * \return
 *      Returns true if the rank and timestamp of the current object are less
 *      than those of rhs.
 */

bool
event::key::operator < (const key & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return (m_rank < rhs.m_rank);
    else
        return (m_timestamp < rhs.m_timestamp);
}

/**
 *  Necessary for comparing keys directly (rather than in sorting).
 */

bool
event::key::operator == (const key & rhs) const
{
    return (m_timestamp == rhs.m_timestamp) && m_rank == rhs.m_rank;
}

/*---------------------------------------------------------------------
 *  Free functions
 *---------------------------------------------------------------------*/

/**
 *  Creates and returns a tempo event.
 */

event
create_tempo_event (midi::pulse tick, midi::bpm tempo)
{
    event e(tick, tempo);
    return e;
}

}           // namespace midi

/*
 * event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

