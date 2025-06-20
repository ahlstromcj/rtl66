#if ! defined RTL66_MIDI_MIDIBYTES_HPP
#define RTL66_MIDI_MIDIBYTES_HPP

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
 * \file          midibytes.hpp
 *
 *  This module declares a number of useful "midi" aliases.
 *  stand-by for type definition).
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2025-06-20
 * \license       GNU GPLv2 or above
 *
 *  These aliases are intended to remove ambiguity seen between signed and
 *  unsigned values.  MIDI bytes and pulses, ticks, or clocks are, by their
 *  nature, unsigned, and we should enforce that.  (However, current pulses
 *  are considered signed long values to avoid compiler warnings.)
 *
 *  One minor issue is why we didn't tack on "_t" to most of these types, to
 *  adhere to C conventions.  Well, no real reason except to save a couple
 *  characters and increase the beauty.  Besides, it is easy to set up vim to
 *  highlight these new types in a special color, making them stand out easily
 *  while reading the code.
 *
 *  Also included are some small classes for encapsulating MIDI timing
 *  information.
 */

#if defined __cplusplus                 /* do not expose this to C code     */

#include <climits>                      /* LONG_MAX and other limits        */
#include <cstdint>                      /* uint64_t and other types         */
#include <string>                       /* std::string, basic_string        */
#include <vector>                       /* std::vector<byte> etc.           */

/*
 *  Since we're using unsigned variables for counting pulses, we can't do the
 *  occasional test for negativity, we have to use wraparound.  One way is to
 *  use this macro.  However, we will probably just ignore the issue of
 *  wraparound.  With 32-bit longs, we have a maximum of 4,294,967,295.  Even
 *  at an insane PPQN of 9600, that's almost 450,000 quarter notes.  And for
 *  64-bit code?  Forgeddaboudid!
 *
 *  #define IS_RTL66_MIDIPULSE_WRAPAROUND(x)  ((x) > (ULONG_MAX / 2))
 */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace midi
{

/**
 *  Provides a fairly common type definition for a byte value.  This can be
 *  used for a MIDI buss/port number or for a MIDI channel number.
 */

using byte = uint8_t;

/**
 *  Provides an array-like container for midibytes. Compare it to the
 *  midistring type.
 */

using bytes = std::vector<byte>;

/**
 *  There are issues with using std::vector<bool>, so we need a type that can
 *  be returned by reference.  See the Seq66 mutegroup class, for example.
 */

using boolean = unsigned char;

/**
 *  Distinguishes a buss/bus number from other MIDI bytes.
 */

using bussbyte = unsigned char;

/**
 *  Distinguishes a short value from the unsigned short values implicit in
 *  short-valued MIDI numbers.
 */

using ushort = uint16_t;

/**
 *  Provides a 4-byte value for use in reading MIDI files. It is identical
 *  to the midi::ulong alias, but marks a set of four-bytes that represent
 *  a string (such as 'MThd' and 'MTrk', or one of the 0x242400nn values
 *  used by Seq66).
 */

using tag = uint32_t;

/**
 *  Distinguishes a long value from the unsigned long values implicit in
 *  long-valued MIDI numbers.
 */

using ulong = uint32_t;

/**
 *  Distinguishes a JACK tick from a MIDI tick (pulse).  The latter are ten
 *  times as long as the JACK tick.
 */

using jacktick = long;

/**
 *  Distinguishes a long value from the unsigned long values implicit in MIDI
 *  time measurements.
 *
 *  HOWEVER, CURRENTLY, if you make this value unsigned, then perfroll won't
 *  show any notes in the sequence bars!!!  Also, a number of manipulations of
 *  this type currently depend upon it being a signed value.
 *
 *  JACK timestamps are in units of "frames":
 *
 *      typedef uint32_t jack_nframes_t;
 *      #define JACK_MAX_FRAMES (4294967295U)   // UINT32_MAX
 *
 *  A long value is the same size in 32-bit code, and longer in 64-bit code,
 *  so we can use a midi::pulse to hold a frame number.
 */

using pulse = long;

/**
 *  JACK encodes jack_time_t as a uint64_t (8-byte) value.  We will use our
 *  own alias, of course.  The unsigned long long type is guaranteed to be at
 *  least 8 bytes long on all platforms, but could be longer.
 */

using microsec = uint64_t;

/**
 *  Provides the data type for BPM (beats per minute) values.  This value used
 *  to be an integer, but we need to provide more precision in order to
 *  support better tempo matching.
 */

using bpm = double;

/**
 *  Provides a more searchable type for the PPQN values.
 */

using ppqn = short;

/*
 *  Container types.  The next few type are common enough to warrant aliasing
 *  in this file.
 */

using string = std::basic_string<byte>;         /* remember, midi::string   */

/**
 *  Provides a convenient way to package a number of booleans, such as
 *  mute-group values or a screenset's sequence statuses.
 */

using booleans = std::vector<boolean>;

/**
 *  Default settings for MIDI as per the specification.
 */

const int c_midi_clocks_per_metronome   = 24;
const int c_midi_32nds_per_quarter      =  8;
const int c_midi_pitch_wheel_range      =  2;       /* +/- 2 semitones      */

/**
 *  We need a unique pulse value that can be used to be indicate a bad,
 *  unusable pulse value (type definition pulse).  This value should be
 *  modified if the alias of pulse is changed.  For a signed long value,
 *  -1 can be used.  For an unsigned long value, ULONG_MAX is probably best.
 *  To avoid issues, when testing for this value, use the inline function
 *  is_null_pulse().
 */

const pulse c_null_pulse = -1;
const pulse c_pulse_max  = LONG_MAX;                /* for sanity checks    */

/**
 *  Defines the maximum number of MIDI values, and one more than the
 *  highest MIDI value, which is 17.
 */

const byte c_byte_data_max  = byte(0x80u);
const byte c_byte_value_max = 127;

/**
 *  The number of MIDI notes supported.  The notes range from 0 to 127.
 */

const int c_notes_count = 128;
const byte c_note_max   = 127;

/**
 *  Maximum and unusable values.  Use these values to avoid sign issues.
 *  Also see c_null_pulse.  No global buss override is in force if the
 *  buss override number is c_bussbyte_max (0xFF).
 */

const byte c_byte_max           = byte(0xFFu);
const bussbyte c_bussbyte_max   = bussbyte(0xFFu);
const ushort c_ushort_max       = ushort(0xFFFF);
const ulong c_ulong_max         = ulong(0xFFFFFFFF);

/**
 *  Default value for c_max_busses.  Some people use a lot of ports, so we
 *  have increased this value from 32 to 48.
 */

const int c_busscount_max       = 48;

/**
 *  Indicates the maximum number of MIDI channels, counted internally from 0
 *  to 15, and by humans (sequencer user-interfaces) from 1 to 16. This value
 *  is also used as a code to indicate that a sequence will use the events
 *  present in the channel.
 */

const int c_channel_max         = 16;
const int c_channel_null        = 0x80;

/**
 *  Indicates an integer that is not a valid ID.  IDs normally start from 0,
 *  this value is negative.
 */

const int c_bad_id              = (-1);

/*
 * -------------------------------------------------------------------------
 *  midi::measures: see measures.hpp
 *  midi::timing: see timing.hpp
 * -------------------------------------------------------------------------
 */

/**
 *  Compares a pulse value to c_null_pulse.  By "null" in this
 *  case, we mean "unusable", not 0.  Sigh, it's always something.
 */

inline bool
is_null_pulse (pulse p)
{
    return p == c_null_pulse;
}

/**
 *  Compares a bussbyte value to the maximum value.  The maximum value is well
 *  over the c_busscount_max = 48 value, being 0xFF = 255, and thus is a
 *  useful flag value to indicate an unusable bussbyte.
 */

inline bool
is_null_buss (bussbyte b)
{
    return b == c_bussbyte_max;
}

inline bussbyte
null_buss ()
{
    return c_bussbyte_max;
}

inline bool
is_good_buss (bussbyte b)
{
    return b < bussbyte(c_busscount_max);
}

inline bool
is_valid_buss (bussbyte b)
{
    return is_good_buss(b) || is_null_buss(b);
}

inline bool
is_good_busscount (int b)
{
    return b > 0 && b <= c_busscount_max;
}

inline bool
is_good_data_byte (byte b)
{
    return b < c_byte_data_max;
}

inline byte
max_byte ()
{
    return c_byte_max;          /* 255 */
}

inline byte
max_midi_value ()
{
    return c_byte_value_max;    /* 127 */
}

inline byte
clamp_midi_value (int b)
{
    if (b < 0)
        return 0;
    else if (b > max_midi_value())
        return max_midi_value();
    else
        return byte(b);
}

inline byte
abs_byte_value (int b)
{
    if (b < 0)
        b = -b;

    if (b > max_midi_value())
        return max_midi_value();
    else
        return byte(b);
}

inline const byte *
midi_bytes (const bytes & b)
{
    return static_cast<const byte *>(b.data());
}

/*
 *  More free functions, not inline. For reference only.

extern std::string midi_bytes_string (const midistring & b, int limit = 0);
extern midibyte string_to_midibyte (const std::string & s, midibyte defalt = 0);
extern midibooleans fix_midibooleans (const midibooleans & mbs, int newsz);

 */

/**
 *  Compares a channel value to the maximum (and illegal) value.
 */

inline bool
is_null_channel (byte c)
{
    return c == c_channel_null;
}

inline byte
null_channel ()
{
    return c_channel_null;
}

inline bool
is_good_channel (byte c)
{
    return c < c_channel_max;                       /* 0 to 15 are good */
}

inline bool
is_good_channel (int c)
{
    return c >= 0 && c < c_channel_max;             /* 0 to 15 are good */
}

inline bool
is_valid_channel (byte c)
{
    return is_good_channel(c) || is_null_channel(c);    /* null is valid    */
}

inline int
bad_id ()
{
    return c_bad_id;
}

/**
 *  This function is meant to deal with values that range from 0 to 127, but
 *  need to be displayed over a different number of pixels.  In qseqdata, the
 *  data is normally shown as one pixel per value (up to 128 pixels). But now
 *  we want a height half that; it's better to provide a function for that.
 *
 * \param height
 *      The current height of the range in pixels.  This will be used to scale
 *      the value against 128.
 *
 * \param value
 *      The value of the byte, assumed to range from 0 to 127.
 *
 * \return
 *      Returns the actual pixel height of the value in the range from
 *      0 to height, versus 0 to 128.
 */

inline int
byte_height (int height, byte value)
{
    const int s_max_height = 128;
    return int(value) * height / s_max_height;
}

/**
 *  The inverse of byte_height().  Parameters and result not checked, for
 *  speed.  Note that height can represent the y-difference between two
 *  pixels.
 */

inline int
byte_value (int height, int value)
{
    const int s_max_height = 128;
    return s_max_height * value / height;
}
/*
 *  In the latest versions of JACK, 0xFFFE is the macro "NO_PORT".  Although
 *  krufty, we can use this value in Seq66 no matter the version of JACK, or
 *  even what API is used.
 *
 *  See port.hpp for these definitions.
 *
 *  inline uint32_t
 *  null_system_port_id ()
 *  {
 *      return 0xFFFE;
 *  }
 *  inline bool
 *  is_null_system_port_id (uint32_t portid)
 *  {
 *      return portid == null_system_port_id();
 *  }
 */

/*
 *  More free functions, not inline.
 */

extern std::string hex_bytes_string (const bytes & b, int limit = 0);
extern std::string bytes_to_string (const bytes & b);
extern byte string_to_byte (const std::string & s, byte defalt = 0);
extern booleans fix_booleans (const booleans & mbs, int newsz);

}               // namespace midi

#endif          // defined __cplusplus : do not expose to C code

#endif          // RTL66_MIDI_MIDIBYTES_HPP

/*
 * midibytes.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

