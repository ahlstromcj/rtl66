#if ! defined RTL66_MIDI_EVENTCODES_HPP
#define RTL66_MIDI_EVENTCODES_HPP

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
 * \file          eventcodes.hpp
 *
 *  This module declares/defines the values for the MIDI status codes (also
 *  known as the event codes), and related values.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2024-05-16
 * \license       GNU GPLv2 or above
 *
 *  This module also declares/defines the various constants, status-byte
 *  values, or data values for MIDI events.  These values were extracted from
 *  Seq66's event.hpp header file in order to be useful beyond that
 *  application.  In addition, the values were changed to enum class values
 *  for better partitioning.
 *
 *  The MIDI protocol consists of MIDI events that carry four types of
 *  messages:
 *
 *      -   Voice messages.  0x80 to 0xEF; includes channel information.
 *      -   System common messages.  0xF0 (SysEx) to 0xF7 (End of SysEx)
 *      -   System realtime messages. 0xF8 to 0xFF.
 *      -   Meta messages. 0xFF is the flag, followed by type, length, and
 *          data.
 *
 *  See the cpp file for detailed information.  Also see:
 *
 *  http://www.muzines.co.uk/articles/everything-you-ever-wanted-to-know-
 *      about-system-exclusive/4558
 */

#include "midi/midibytes.hpp"           /* midi::byte alias and more        */

namespace midi
{

/**
 *  This highest bit of the STATUS byte is always 1.  If this bit is not set,
 *  then the MIDI byte is a DATA byte.  Data bytes range from 0 to 0x7F, while
 *  status bytes range from 0x80 to 0xFF.
 */

const byte status_null          = 0x00;     // a Seq66 value
const byte status_bit           = 0x80;     // a status-detection mask
const byte realtime             = 0xf0;     // 0xFn when masked
const byte sysex_continue       = 0xf7;     // redundant, see below

/**
 *  These file masks are used to obtain (or mask off) the channel data and
 *  status portion from an (incoming) status byte.
 */

const byte chan_mask_nybble     = 0x0f;     // mask for the channel nybble
const byte status_mask_nybble   = 0xf0;     // mask for the status nybble
const byte data_mask_byte       = 0x7f;     // mask for data values
const byte data_max             = 0x7f;     // mask for data values

/**
 *  Defines the MIDI status status bytes, from 0x80 to 0xff.  For 0x80 to
 *  0xEF, the second digit (0 to F, i.e. 0 to 15) represent the channel
 *  number, ranging from 1 to 16 in musical terms.
 *
 *  See eventcodes.cpp for more documentation.
 */

enum class status : byte
{
    note_off            = 0x80, // 0kkkkkkk 0vvvvvvv (Middle C = 60 dec)
    note_on             = 0x90, // 0kkkkkkk 0vvvvvvv (Middle C = 0x3C)
    aftertouch          = 0xA0, // 0kkkkkkk 0vvvvvvv (Polyphone Key Pressure)
    control_change      = 0xB0, // 0ccccccc 0vvvvvvv (c = 0x0 to 0x77)
    program_change      = 0xC0, // 0ppppppp (1-byte message)
    channel_pressure    = 0xD0, // 0vvvvvvv (1-byte; "global aftertouch")
    pitch_wheel         = 0xE0, // 0lllllll 0mmmmmmm (14-bits, center=0x2000)
    real_time           = 0xF0, // all of the following, masked in parsing
    sysex               = 0xF0, // variable data and data size
    quarter_frame       = 0xF1, // System Common start; 0nnnddd
    song_pos            = 0xF2, // 2 data bytes
    song_select         = 0xF3, // 1 data byte
    song_F4             = 0xF4, // undefined
    song_F5             = 0xF5, // undefined
    tune_select         = 0xF6, // 0 data bytes
    sysex_continue      = 0xF7, // redundant, see below TODO
    sysex_end           = 0xF7, // redundant, see below TODO
    clk_clock           = 0xF8, // real-time: 0 data bytes
    timing_tick         = 0xF9, // a MIDI timing tick
    clk_start           = 0xFA, // real-time: 0 data bytes
    clk_continue        = 0xFB, // real-time: 0 data bytes
    clk_stop            = 0xFC, // real-time: 0 data bytes
    song_FD             = 0xFD, // undefined
    active_sense        = 0xFE, // 0 data bytes
    meta_msg            = 0xFF, // tricky escape code, see meta enum
    reset               = 0xFF, // 0 data bytes
    erroneous           = 0x00  // somehow got out of synch, MIDI file error
};

inline byte
to_byte (status s)
{
    return static_cast<byte>(s);
}

inline status
to_status (byte b)
{
    return static_cast<status>(b);
}

/**
 *  The "any event" (0x00) value is useful in allowing any event to be dealt
 *  with.  If the status byte is the value 0x00, then any event will be
 *  obtained or used, left unfiltered.
 */

inline bool
any_event (byte b)
{
    return b == 0x00;
}

/**
 *  Test for the channel message/statuse values: Note On, Note Off,
 *  Aftertouch, Control Change, Program Change, Channel Pressure, and
 *  Pitch Wheel.  This function is also a test for a Voice Category
 *  status.  The allowed range is 0x80 to 0xEF.  Currently not used
 *  anywhere.
 *
 * \param m
 *      The channel status or message byte to be tested.
 *
 * \return
 *      Returns true if the byte represents a MIDI channel message.
 */

inline bool
is_channel_msg (byte m)
{
    return m >= 0x80 && m < realtime;       // 0xF0
}

/**
 *  Provides values for the currently-supported Meta events, and many others.
 *  As a "type" (overloaded on channel) value for a Meta event, 0xFF indicates
 *  an illegal meta type.
 *
 *  See the cpp file for their formats and sizes.
 */

enum class meta : byte
{
    seq_number          = 0x00,
    text_event          = 0x01,
    copyright           = 0x02,
    track_name          = 0x03,
    instrument          = 0x04,
    lyric               = 0x05,
    marker              = 0x06,
    cue_point           = 0x07,
    program_name        = 0x08,         /* TODO: add support for this event */
    port_name           = 0x09,         /* TODO: add support for this event */
    midi_channel        = 0x20,         /* obsolete, handled generically    */
    midi_port           = 0x21,         /* obsolete, handled generically    */
    end_of_track        = 0x2F,
    set_tempo           = 0x51,
    smpte_offset        = 0x54,
    time_signature      = 0x58,
    key_signature       = 0x59,
    seq_spec            = 0x7F,
    meta_byte           = 0xFF          /* illegal; needed to detect meta   */
};

inline byte
to_byte (meta m)
{
    return static_cast<byte>(m);
}

inline meta
to_meta (byte b)
{
    return static_cast<meta>(b);
}

inline bool
is_meta (byte b)
{
    return to_status(b) == status::meta_msg;
}

inline bool
is_meta_seq_spec (byte b)
{
    return to_meta(b) == meta::seq_spec;
}

/**
 *  Assumes the message has already been determined to be a meta message.
 *  Includes text_event, copyright, track_name, instrument, lyric, marker, and
 *  cue_point Meta messages.
 */

inline bool
is_meta_text_msg (byte b)
{
    return b >= to_byte(meta::text_event) && b <= to_byte(meta::cue_point);
}

/**
 *  Control Change Messages:
 *
 *      This enumeration summarizes the MIDI Continuous Controllers (CC) that
 *      are available.  See the cpp file for more details.
 *
 *  Copped from seq66/libseq66/include/midi/controllers.hpp.
 */

enum class ctrl : byte
{
    bank_select       =   0, /**< Switches patch bank;16,384 patches/chann.  */
    modulation        =   1, /**< Patch vibrato (pitch/loudness/brightness). */
    breath_controller =   2, /**< Aftertouch, MIDI control, modulation.      */
    undefined_03      =   3,
    foot_controller   =   4, /**< Aftertouch, stream of pedal values, etc.   */
    portamento        =   5, /**< Controls rate to slide between 2 notes.    */
    data_entry        =   6, /**< MSB sets value for NRPN/RPN parameters.    */
    volume            =   7, /**< Controls the volume of the channel.        */
    balance           =   8, /**< Left/right balance for stereo patches.     */
    undefined_09      =   9,
    pan               =  10, /**< Left/right balance for mono patches.       */
    expression        =  11, /**< Expression, a percentage of volume (CC7).  */
    effect_control_1  =  12, /**< Control a parameter of a synth effect.     */
    effect_control_2  =  13, /**< Control a parameter of a synth effect.     */
    undefined_14      =  14,
    undefined_15      =  15,
    general_purp_16   =  16,
    general_purp_17   =  17,
    general_purp_18   =  18,
    general_purp_19   =  19,

    /*
     * 20 – 31 Undefined.
     * 32 – 63 Controllers 0 to 31, Least Significant Bit (LSB).
     */

    damper_pedal      =  64, /**< On/off switch that controls sustain.       */
    portamento_onoff  =  65, /**< On/off switch that controls portamento.    */
    sostenuto         =  66, /**< On/off switch to holds only On notes.      */
    soft_pedal        =  67, /**< On/off switch to lower volume of notes.    */
    legato            =  68, /**< On/off switch for legato between 2 notes.  */
    undefined_69      =  69,
    sound_control_1   =  70, /**< Control sound producing [Sound Variation]. */
    sound_control_2   =  71, /**< Shapes the VCF [Resonance, timbre].        */
    sound_control_3   =  72, /**< Control release time of the VCA.           */
    sound_control_4   =  73, /**< Control attack time of the VCA.            */
    sound_control_5   =  74, /**< Controls the VCF cutoff frquency.          */
    sound_control_6   =  75, /**< Shapes the VCF [Resonance, timbre].        */
    sound_control_7   =  76, /**< Manufacturer-dependent sound alteration.   */
    sound_control_8   =  77, /**< Manufacturer-dependent sound alteration.   */
    sound_control_9   =  78, /**< Manufacturer-dependent sound alteration.   */
    sound_control_10  =  79, /**< Manufacturer-dependent sound alteration.   */
    gp_onoff_switch_1 =  80, /**< Provides a general purpose on/off switch.  */
    gp_onoff_switch_2 =  81, /**< Provides a general purpose on/off switch.  */
    gp_onoff_switch_3 =  82, /**< Provides a general purpose on/off switch.  */
    gp_onoff_switch_4 =  83, /**< Provides a general purpose on/off switch.  */
    portamento_cc     =  84, /**< Controls the amount of portamento.         */

    /*
     * 85 – 90 Undefined. Are they device-specific?
     */

    effect_1_depth    =  91, /**< Usually controls reverb send amount.       */
    effect_2_depth    =  92, /**< Usually controls tremolo amount.           */
    effect_3_depth    =  93, /**< Usually controls chorus amount.            */
    effect_4_depth    =  94, /**< Usually controls detune amount.            */
    effect_5_depth    =  95, /**< Usually controls phaser amount.            */
    data_increment    =  96, /**< Increment data for RPN and NRPN messages.  */
    data_decrement    =  97, /**< Decrement data for RPN and NRPN messages.  */
    nrpn_lsb          =  98, /**< CC 6, 38, 96, and 97: selects NRPN LSB.    */
    nrpn_msb          =  99, /**< CC 6, 38, 96, and 97: selects NRPN MSB.    */
    rpn_lsb           = 100, /**< CC 6, 38, 96, and 97: selects RPN LSB.     */
    rpn_msb           = 101, /**< CC 6, 38, 96, and 97: selects RPN MSB.     */

    /*
     * 102 – 119 Undefined.
     * 120 is now a Channel Mode message.
     *
     * The ones below aren
     */

    reset_all         = 121, /**< Reset all controllers to their default.    */
    local_switch      = 122, /**< Switches internal connection of a device.  */
    all_notes_off     = 123, /**< Mutes all sounding notes. See notes.       */
    omni_off          = 124, /**< Sets to “Omni Off” mode.                   */
    omni_on           = 125, /**< Sets to “Omni On” mode.                    */
    mono_on           = 126, /**< device mode to Monophonic.                 */
    poly_on           = 127  /**< device mode to Polyphonic.                 */
};

inline byte
to_byte (ctrl c)
{
    return static_cast<byte>(c);
}

inline ctrl
to_ctrl (byte b)
{
    return static_cast<ctrl>(b);
}

inline byte
mask_channel (byte m)
{
    return m & chan_mask_nybble;
}

inline byte
mask_status (byte m)
{
    return m & status_mask_nybble;
}

inline status
mask_status (status s)
{
    return status(byte(s) & status_mask_nybble);
}

inline bool
match_status (byte m, byte s)
{
    return m == s;
}

inline bool
match_status (byte m, status s)
{
    return m == to_byte(s);
}

inline byte
mask_data (byte m)
{
    return m & data_mask_byte;
}

inline byte
add_channel (byte bstatus, byte channel)
{
    return mask_status(bstatus) | channel;
}

/**
 *  Test for the status bit.  The opposite test is is_status_msg().
 *  Currently not used anywhere.
 *
 * \return
 *      Returns true if the status bit is not set.
 */

inline bool
is_data_msg (byte m)
{
    return (m & status_bit) == 0x00;
}

/**
 *  Test for the status bit.  The "opposite" test is is_data().
 *  Currently used only in midifile.
 *
 * \return
 *      Returns true if the status bit is set.  Covers 0x80 to 0xFF.
 */

inline bool
is_status_msg (byte m)
{
    return (m & status_bit) != 0;
}

/**
 *  Makes sure the status byte matches the "status" message bytes exactly
 *  by stripping the channel nybble if necessary.
 *
 * \param s
 *      Provides a midi::status byte value.
 */

inline byte
normalized_status (byte s)
{
    return is_channel_msg(s) ? mask_status(s) : s ;
}

/*
 *  Functions used in event and editable event.  These can be useful
 *  to any caller.
 */

inline bool
is_system_msg (byte m)
{
    return m >= to_byte(status::sysex);
}

/**
 *  0xFF is a MIDI "escape code" used in MIDI files to introduce a MIDI meta
 *  event.  Note that it has the same code (0xFF) as the Reset message, but
 *  the Meta message is read from a MIDI file, while the Reset message is sent
 *  to the sequencer by other MIDI participants.
 */

inline bool
is_meta_msg (byte m)
{
    return m == to_byte(meta::meta_byte);
}

/**
 *  Checks a presumed midi:meta value against the given byte. Use only with
 *  a known meta status byte in play.
 */

inline bool
is_meta_msg (byte m, meta mmsg)
{
    return m == to_byte(mmsg);
}

inline bool
is_ex_data_msg (byte m)
{
    return m == to_byte(meta::meta_byte) || m == to_byte(status::sysex);
}

inline bool
is_pitchbend_msg (byte m)
{
    return mask_status(m) == to_byte(status::pitch_wheel);
}

inline bool
is_controller_msg (byte m)
{
    return mask_status(m) == to_byte(status::control_change);
}

/**
 *  We don't want a progress bar for patterns that just contain textual
 *  information. Tempo event are important, though, and visible in some
 *  pattern views.
 */

inline bool
is_playable_msg (byte m)
{
    return m != to_byte(meta::meta_byte) && m != to_byte(status::sysex);
}

/*
 *  Functions used in analysizing MIDI events by external callers.
 */

/**
 *  Test for channel messages that have only one data byte.
 *
 *      -   Program Change      = 0xC0
 *      -   Channel Pressure    = 0xD0
 *
 * \param m
 *      The channel status or message byte to be tested. The channel
 *      bits are masked off before the test.
 *
 * \return
 *      Returns true if the byte represents a MIDI channel message that
 *      has only one data byte.  Note that the size of the message is one byte
 *      more if the status byte is counted.
 */

inline bool
is_one_byte_msg (byte m)
{
    m = mask_status(m);
    return m == 0xC0 || 0xD0;
}

/**
 *  Test for channel messages that have two data bytes: Note On,
 *  Note Off, Control Change, Aftertouch, and Pitch Wheel.
 *
 * \param m
 *      The channel status or message byte to be tested. The channel
 *      bits are masked off before the test.
 *
 * \return
 *      Returns true if the byte represents a MIDI channel message that
 *      has two data bytes.  Note that the size of the message is one byte
 *      more if the status byte is counted.
 */

inline bool
is_two_byte_msg (byte s)
{
    return (s >= 0x80 && s < 0xC0) || (mask_status(s) == 0xE0);
}

/**
 *  Test for messages that involve notes and velocity: Note On,
 *  Note Off, and Aftertouch.  These three events have values in an easy range
 *  to check.
 *
 * \param m
 *      The channel status or message byte to be tested.
 *
 * \return
 *      Returns true if the byte represents a MIDI note message.
 */

inline bool
is_note_msg (byte m)
{
    return m >= 0x80 && m < 0xB0;       /* twixt Note Off & Control Change  */
}

inline bool
is_note_off_msg (byte m)
{
    return m >= 0x80 && m < 0x90;       /* twixt Note Off & Note On         */
}

inline bool
is_note_on_msg (byte m)
{
    return m >= 0x90 && m < 0xA0;       /* twixt Note On & Aftertouch       */
}

/**
 *  Test for messages that involve notes only: Note On and
 *  Note Off, useful in note-event linking.  Events between Note On and
 *  below Aftertouch are detected.  Aftertouch is ignored in this test.
 *
 * \param m
 *      The channel status or message byte to be tested.
 *
 * \return
 *      Returns true if the byte represents a MIDI note on/off message.
 */

inline bool
is_strict_note_msg (byte m)
{
    return m >= 0x80 && m < 0xA0;       /* twixt Note Off & Aftertouch      */
}

/**
 *  Tests for a Note On with a velocity of 0.
 *
 * \param status
 *      The type of event, which might be status::note_on.
 *
 * \param vel
 *      The velocity byte to check.  It should be zero for a note-on is
 *      note-off event.
 */

inline bool
is_note_off_velocity (byte status, byte vel)
{
    return mask_status(status) == 0x90 && vel == 0;
}

inline bool
is_program_change_msg (byte m)
{
    return mask_status(m) == 0xC0;
}

inline bool
is_below_sysex_msg (byte m)
{
    return m < 0xF0;
}

/**
 *  Checks for a System Common status, which is supposed to clear any
 *  running status.  Use in midifile.
 */

inline bool
is_system_common_msg (byte m)
{
    return m >= 0xF0 && m < 0xF8;   /* enum class status sysex to clock     */
}

inline bool
is_sysex_msg (byte m)
{
    return m == 0xF0 || m == 0xF7;  /* sysex or sysex_continue status byte  */
}

inline bool
is_sysex_end_msg (byte m)
{
    return m == 0xF7;
}

/**
 *  Check for special SysEx ID byte.
 *
 * \param ch
 *      Provides the byte to be checked against 0x7D through 0x7F.
 *
 * \return
 *      Returns true if the byte is SysEx special ID.
 */

inline bool
is_sysex_special_id (byte ch)
{
    return ch >= 0x7D && ch <= 0x7F;
}

inline bool
is_quarter_frame_msg (byte m)
{
    return m == 0xF1;
}

inline bool
is_midi_song_pos_msg (byte m)
{
    return m == 0xF2;
}

/**
 *  Checks for a Realtime Category status, which ignores running status.
 *  Ranges from 0xF8 to 0xFF,  and m <= EVENT_MIDI_RESET is always true.
 *  Use in midi::file or midi::trackdata.
 *
 *  ????
 */

inline bool
is_realtime_msg (byte m)
{
    return m >= 0xF8;
}

inline bool
is_midi_clock_msg (byte m)
{
    return m == 0xF8;
}

inline bool
is_midi_start_msg (byte m)
{
    return m == 0xFA;
}

inline bool
is_midi_continue_msg (byte m)
{
    return m == 0xFB;
}

inline bool
is_midi_stop_msg (byte m)
{
    return m == 0xFC;
}

/**
 *  Used in midi_jack.
 */

inline bool
is_sense_or_reset_msg (byte m)
{
    return m == 0xFE || m == 0xFF;
}

inline bool
is_sense_msg (byte m)
{
    return m == 0xFE;
}

/**
 *  The following functions are useful only if is_meta() is true.
 */

inline bool
is_tempo_msg (byte m)
{
    return m == 0x51;
}

inline bool
is_time_signature_msg (byte m)
{
    return m == 0x58;
}

inline bool
is_key_signature_msg (byte m)
{
    return m == 0x59;
}

/**
 *  Indicates an event that has a line-drawable data item, such as
 *  velocity.  It is false for discrete data such as program/path number
 *  or Meta events.
 */

inline bool
is_continuous_event_msg (byte m)
{
    return ! is_program_change_msg(m) && ! is_meta_msg(m);
}

/**
 *  Test for channel messages that are either not control-change messages, or
 *  are and match the given controller value.  This function is used in Seq66.
 *
 * \param m
 *      The channel status or message byte to be tested.
 *
 * \param cc
 *      The desired cc value, which the datum must match, if the message
 *      is a control-change message.
 *
 * \param datum
 *      The current datum, to be compared to cc, if the message is a
 *      control-change message.
 *
 * \return
 *      Returns true if the message is not a control-change, or if it is
 *      and the cc and datum parameters match.
 */

inline bool
is_desired_cc_or_not_cc (byte m, byte cc, byte datum)
{
    return (mask_status(m) != 0xB0) || (datum == cc);
}

inline bool
has_channel (byte m)
{
    return is_channel_msg(m);
}

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

/**
 *  Provides the default names of MIDI controllers, which a specified in the
 *  controllers.cpp module.  This array is used
 *  only by the qseqedit classes.
 *
 *  We could make this list a configuration option.  Overkill? No, it is
 *  already configuration in the 'usr' file.  So at some point we offload this
 *  stuff to an as-shipped 'usr' file.
 */

extern std::string midi_controller_name (int index);
extern std::string gm_program_name (int index);
extern std::string gm_percussion_name (int index);
extern int status_msg_size (byte s);
extern int meta_msg_size (byte m);
extern std::string status_label (byte m);
extern std::string meta_text_label (byte m);
extern std::string meta_label (byte m);

}           // namespace midi

#endif      // RTL66_MIDI_EVENTCODES_HPP

/*
 * eventcodes.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

