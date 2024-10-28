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
 * \file          message.cpp
 *
 *    Provides a way to encapsulate the bytes of a MIDI message.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-01
 * \updates       2024-05-15
 * \license       See above.
 *
 *  Provides a basic type for the (heavily-factored) rtl66 library, very
 *  loosely based on Gary Scavone's RtMidi library.
 *
 *  Please note that the ALSA module in rtl66's rtl66 infrastructure
 *  uses the rtl::event rather than the rtl::message object.
 *  For the moment, we will translate between them until we have the
 *  interactions between the old and new modules under control.
 *
 *  For output, ALSA fills a small buffer with the event status, d0, and
 *  d1. For SysEx, it gets the buffer from the event and outputs it.
 *
 *  For input, ALSA gets a snd_seq_event_t, gets the timestamp, decodes the
 *  event into a buffer, and calls event::set_midi_event() to create it.
 *  It also sets the event's input bus.
 *
 *  For output, JACK creates a midi::message from a midi::event and pushes
 *  it onto the ringbuffer in send_message(). After a delay, the
 *  JACK output callback loops through the buffer and writes MIDI event
 *  using a JACK port buffer, a frame offset, and the data buffer containing
 *  the bytes of the midi::message popped off the ringbuffer.
 *
 *  For input, JACK gets an event (status byte plus data), creates a
 *  midi::message with the delta time [jack_get_time() minus "last time",
 *  see below], then pushes it into the input queue, see below.
 *  Eventually, it pops the midi::message and calls set_midi_event with
 *  the message timestamp and event bytes.
 *
 *      -   Verify that Seq66 is correct in handling JACK time!
 *      -   See if the old midi_queue can be refactored using the
 *          ring_buffer template.
 *
 *  We add the timestamp (in units of MIDI ticks, also known as pulses, but
 *  converted to a double value) to the midi::message.  We then provide
 *  functions to handle the array of data in two different ways:
 *
 *      -#  Buffer: Access the data buffer for all bytes, in order to put
 *          them on the JACK ringbuffer for the process callback to use:
 *          -   MIDI timestamp bytes (4)
 *          -   Status byte
 *          -   Data bytes
 *      -#  Event: Access the status and data bytes as a unit to pass them
 *          to the JACK engine for transmitting.
 *
 *  Apart from time-stamp, the byte contents of midi::message are as follows.
 *  Note that "n" denotes the channel, d0 denotes the first data byte, and d1
 *  denotes the second data byte (0 or not present if not applicable).
 *
 *  -    note_off            8n d0 d1 (key, velocity)
 *  -    note_on             9n d0 d1 (key, velocity)
 *  -    aftertouch          An d0 d1 (key, pressure)
 *  -    control_change      Bn d0 d1 (controller, value)
 *  -    program_change      Cn d0    (program/patch number)
 *  -    channel_pressure    Dn d0    (pressure)
 *  -    pitch_wheel         En d0 d1 (least significant and most significant)
 *  -    real_time           F0 -----
 *  -    sysex               F0 id len data F7
 *  -    quarter_frame       F1 len? type+value (1 byte)
 *  -    song_pos            F2 lsb msb (14 bits)
 *  -    song_select         F3 song-number (1 byte)
 *  -    song_F4             F4 -----
 *  -    song_F5             F5 -----
 *  -    tune_select         F6 (no data; a request to tune oscillators)
 *  -    sysex_continue      F7 -----
 *  -    sysex_end           F7 ----- (see sysex enum value above)
 *  -    clock               F8 ----- (timing clock, 24 times per quarter note)
 *  -    song_F9             F9 -----
 *  -    clk_start           FA ----- (Clock start)
 *  -    clk_continue        FB ----- (Clock continue)
 *  -    clk_stop            FC ----- (Clock stop)
 *  -    song_FD             FD -----
 *  -    active_sense        FE ----- (can be sent repeatedly as a watchdog)
 *  -    meta_msg            FF See below
 *  -    reset               FF ----- (request receivers to restart)
 *
 *  Meta messages:
 *
 *      These messages are never sent. They are read from and written to a
 *      MIDI file, and specify features of the tune important to a sequencer
 *      and its user.
 *
 *  -    seq_number          FF 00 02 ss (track number)
 *  -    text_event          FF 01 len text
 *  -    copyright           FF 02 len text
 *  -    track_name          FF 03 len text
 *  -    instrument          FF 04 len text
 *  -    lyric               FF 05 len text
 *  -    marker              FF 06 len text
 *  -    cue_point           FF 07 len text
 *  -    midi_channel        FF 20 01 channel (obsolete)
 *  -    midi_port           FF 21 01 port (obsolete)
 *  -    end_of_track        FF 2F 00
 *  -    set_tempo           FF 51 03 tt tt tt (microseconds)
 *  -    smpte_offset        FF 54 03 tt tt tt (not yet supported fully)
 *  -    time_signature      FF 58 04 nn dd cc bb
 *  -    key_signature       FF 59 02 ss kk (scale and major/minor flag)
 *  -    seq_spec            FF 7f len [id] data
 */

#include <iostream>                     /* std::cerr                        */
#include <iomanip>                      /* std::hex, setw()  manipulators   */

#include "midi/message.hpp"             /* midi::message class              */

namespace midi
{

/*
 * class message
 */

#if defined RTL66_PLATFORM_DEBUG
unsigned message::sm_msg_number = 0;
#endif

/**
 *  Constructs an empty MIDI message.  Well, empty except for the timestamp
 *  bytes.  The caller will then push the data bytes for the MIDI event.
 *
 * \param ts
 *      The timestamp of the event in ticks (pulses) in double format for
 *      precision.
 */

message::message (double ts) :
#if defined RTL66_PLATFORM_DEBUG
    m_msg_number    (sm_msg_number++),
#endif
    m_bytes         (),
#if defined RTL66_USE_MESSAGE_HEADER_SIZE
    m_header_size   (0),
#endif
    m_time_stamp    (ts)
{
    // Empty body
}

/**
 *  Constructs a message from an array of bytes (versus a vector of bytes).
 *
 *  One issue is this data needs to be analyzed to determine if it includes
 *  header (status, type, length) data.
 *
 * \param mbs
 *      Provides the data, which should start with the timestamp bytes, and
 *      optionally followed by event bytes.
 *
 * \param sz
 *      The putative size of the byte array.
 */

message::message (const midi::byte * mbs, std::size_t sz) :

#if defined RTL66_PLATFORM_DEBUG
    m_msg_number    (sm_msg_number++),
#endif
    m_bytes         (),
#if defined RTL66_USE_MESSAGE_HEADER_SIZE
    m_header_size   (0),
#endif
    m_time_stamp    (0)
{
    for (std::size_t i = 0; i < sz; ++i)
        m_bytes.push_back(*mbs++);
}

/**
 *  Constructs a message from a vector of bytes.
 *
 * \param mbs
 *      Provides the data, which should start with the timestamp bytes, and
 *      optionally followed by event bytes.
 */

message::message (const midi::bytes & mbs) :

#if defined RTL66_PLATFORM_DEBUG
    m_msg_number    (sm_msg_number++),
#endif
    m_bytes         (),
    m_time_stamp    (0)
{
    for (auto c : mbs)
        m_bytes.push_back(c);
}

/**
 *  Shows the bytes in a string, for trouble-shooting.  It includes only the
 *  timestamp and the first few bytes.
 */

std::string
message::to_string () const
{
    size_t counter = 12;
    bool incomplete = event_byte_count() > counter;
    char bcount[8];
    (void) snprintf(bcount, sizeof bcount, "%3zd", event_byte_count());

    std::string result = bcount;
    result += " hex bytes";

    if (! incomplete)
        counter = event_byte_count();

    for (size_t i = 0; i < counter; ++i)
    {
        char temp[8];
        snprintf(temp, sizeof temp, " %02x", m_bytes[i]);
        result += temp;
    }
    if (incomplete)
        result += "...";

    return result;
}

}           // namespace midi

/*
 * message.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

