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
 * \file          trackinfo.cpp
 *
 *  This module declares/defines a class for handling MIDI events in a list
 *  container.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2024-05-06
 * \license       GNU GPLv2 or above
 *
 *  This container now can indicate if certain Meta events (time-signaure or
 *  tempo) have been added to the container.
 */

#include <algorithm>                    /* std::sort(), std::merge()        */
#include <map>                          /* std::map                         */

#include "midi/calculations.hpp"        /* midi::tempo_us_from_bpm()        */
#include "midi/trackinfo.hpp"           /* Various MIDI data structures     */

namespace midi
{

/*------------------------------------------------------------------------
 * Tempo
 *------------------------------------------------------------------------*/

/**
 * Tempo:
 *
 *     FF 51 03 tt tt tt
 *
 *     BPM = 60,000,000 / (tt tt tt)
 *
 *     120 BPM = 07 A1 20 microseconds per quarter note
 *
 *  Principal and default constructor. Defaults:
 *
 *      -   tempobpm    = 120.0
 *      -   tempotrack  = 0)
 */

tempoinfo::tempoinfo
(
    double tempobpm,
    int tempotrack
) :
    m_tempo_track           (tempotrack),
    m_beats_per_minute      (tempobpm),
    m_us_per_quarter_note   (0)
{
    us_per_quarter_note(tempo_us_from_bpm(m_beats_per_minute));
}

std::string
tempoinfo::bpm_to_string () const
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%6.2f", beats_per_minute());
    return std::string(tmp);
}

std::string
tempoinfo::bpm_labelled () const
{
    return bpm_to_string() + " BPM";
}

std::string
tempoinfo::usperqn_to_string () const
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%d", us_per_quarter_note());
    return std::string(tmp);
}

std::string
tempoinfo::usperqn_labelled () const
{
    return usperqn_to_string() + " us/qn";
}

double
tempoinfo::tempo_period_us () const
{
    return tempo_us_from_bpm(beats_per_minute());
}

/*------------------------------------------------------------------------
 * Time signature
 *------------------------------------------------------------------------*/

/**
 * Time Signature:
 *
 *      FF 58 04 nn dd cc bb
 *
 *  Time signature is expressed as 4 numbers. nn and dd represent the
 *  "numerator" and "denominator" of the signature as notated on sheet
 *  music. The denominator is a negative power of 2: 2 = quarter note, 3 =
 *  eighth, etc.
 *
 *  The cc expresses the number of MIDI clocks in a metronome click.
 *
 *  The bb parameter expresses the number of notated 32nd notes in a MIDI
 *  quarter note (24 MIDI clocks). This event allows a program to relate
 *  what MIDI thinks of as a quarter, to something entirely different.
 *
 *  For example, 6/8 time with a metronome click every 3 eighth notes and 24
 *  clocks per quarter note would be the following event:
 *
 *      FF 58 04 06 03 18 08
 *
 *  If there are no time signature events in a MIDI file, then the time
 *  signature is assumed to be 4/4.
 *
 *  In a format 0 file, the time signatures changes are scattered throughout
 *  the one MTrk. In format 1, the very first MTrk should consist of only
 *  the time signature (and tempo) events so that it could be read by some
 *  device capable of generating a "tempo map". It is best not to place MIDI
 *  events in this MTrk.  In format 2, each MTrk should begin with at least
 *  one initial time signature (and tempo) event.
 *
 *  Principal/default constructor. Defaults:
 *
 *      -   bpb       = 4
 *      -   bw        = 4
 *      -   cpm       = 24
 *      -   n32sperqn = 8
 */

timesiginfo::timesiginfo
(
    int bpb,
    int bw,
    unsigned cpm,
    unsigned n32sperqn
) :
    m_beats_per_bar         (bpb),
    m_beat_width            (bw),
    m_clocks_per_metronome  (cpm),
    m_thirtyseconds_per_qn  (n32sperqn)
{
    // No code
}

std::string
timesiginfo::timesig_to_string ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%d/%d", beats_per_bar(), beat_width());
    return std::string(tmp);
}

std::string
timesiginfo::timesiginfo_labelled ()
{
    char tmp[32];
    snprintf
    (
        tmp, sizeof tmp, " %d clocks/metro %d 32s/qn",
        clocks_per_metronome(), thirtyseconds_per_qn()
    );
    return timesig_to_string() + std::string(tmp);
}

/*------------------------------------------------------------------------
 * Key signature
 *------------------------------------------------------------------------*/

/**
 * Key Signature:
 *
 *      FF 59 02 sf mf
 *
 *  The sf byte has values between -7 and 7 and specifies the key signature in
 *  terms of number of flats (if negative) or sharps (if positive).
 *
 *  The mf byte of the message specifies the scale of the MIDI file;
 *  0 = the scale is major and 1 = the scale is minor.
 *
\verbatim
 *      sf          mf      Key signature
 *      0x00        0x00        C major
\endverbatim
 *
 *  See the following link for a complete summary.
 *
 *      https://www.recordingblogs.com/wiki/midi-key-signature-meta-message
 *
 *
 */

/**
 *  Principal and default constructor.
 */

keysiginfo::keysiginfo (int sf, bool mf) :
    m_keysig_name       (),
    m_sharp_flat_count  (0),
    m_is_minor_scale    (false)
{
    sharp_flat_count(sf);
    is_minor_scale(mf);

    // To do: create the name string
}

void
keysiginfo::sharp_flat_count (int sf)
{
    if (sf >= (-7) && sf <= 7)
    {
        m_sharp_flat_count = int(sf);
        m_keysig_name = key_name();
    }
}

void
keysiginfo::is_minor_scale (bool isminor)
{
    m_is_minor_scale = isminor;
    m_keysig_name = key_name();
}

/*
 *  Some code to think about enabling.
 */

#if defined ENABLE_KEY_SCALES_MAPPING

struct keysigkey
{
    int ksk_sharp_flat;
    bool ksk_is_minor;
};

using keylist = std::vector<std::string>;

using keyscales = std::map<keysigkey, keylist>;

static keyscales s_all_keys
{
    // Major scales: Sharps: Key

    { {  0, false }, { "C" , "D" , "E" , "F" , "G" , "A" , "B"  } },
    { {  1, false }, { "G" , "A" , "B" , "C" , "D" , "E" , "F#" } },
    { {  2, false }, { "D" , "E" , "F#", "G" , "A" , "B" , "C#" } },
    { {  3, false }, { "A" , "B" , "C#", "D" , "E" , "F#", "G#" } },
    { {  4, false }, { "E" , "F#", "G#", "A" , "B" , "C#", "D#" } },
    { {  5, false }, { "B" , "C#", "D#", "E" , "F#", "G#", "A#" } },
    { {  6, false }, { "F#", "G#", "A#", "B" , "C#", "D#", "E#" } },
    { {  7, false }, { "C#", "D#", "E#", "F#", "G#", "A#", "B#" } },

    // Major scales: Flats: Key

    { { -1, false }, { "F" , "G" , "A" , "Bb", "C" , "D" , "E"  } },
    { { -2, false }, { "Bb", "C" , "D" , "Eb", "F" , "G" , "A"  } },
    { { -3, false }, { "Eb", "F" , "G" , "Ab", "Bb", "C" , "D"  } },
    { { -4, false }, { "Ab", "Bb", "C" , "Db", "Eb", "F" , "G"  } },
    { { -5, false }, { "Db", "Eb", "F" , "Gb", "Ab", "Bb", "C"  } },
    { { -6, false }, { "Gb", "Ab", "Bb", "Cb", "Db", "Eb", "F"  } },
    { { -7, false }, { "Cb", "Db", "Eb", "Fb", "Gb", "Ab", "Bb" } },

    // Minor scales: Sharps: Key

    { {  0, true  }, { "A" , "B" , "C" , "D" , "E" , "F" , "G"  } },
    { {  1, true  }, { "E" , "F#", "G" , "A" , "B" , "C" , "D"  } },
    { {  2, true  }, { "B" , "C#", "D" , "E" , "F#", "G" , "A"  } },
    { {  3, true  }, { "F#", "G#", "A" , "B" , "C#", "D" , "E"  } },
    { {  4, true  }, { "C#", "D#", "E" , "F#", "G#", "A" , "B"  } },
    { {  5, true  }, { "G#", "A#", "B" , "C#", "D#", "E" , "F#" } },
    { {  6, true  }, { "D#", "E#", "F#", "G#", "A#", "B" , "C#" } },
    { {  7, true  }, { "A#", "B#", "C#", "D#", "E#", "F#", "G#" } },

    // Minor scales: Flats: Key

    { { -1, true  }, { "D" , "E" , "F" , "G" , "A" , "Bb", "C"  } },
    { { -2, true  }, { "G" , "A" , "Bb", "C" , "D" , "Eb", "F"  } },
    { { -3, true  }, { "C" , "D" , "Eb", "F" , "G" , "Ab", "Bb" } },
    { { -4, true  }, { "F" , "G" , "Ab", "Bb", "C" , "Db", "Eb" } },
    { { -5, true  }, { "Bb", "C" , "Db", "Eb", "F" , "Gb", "Ab" } },
    { { -6, true  }, { "Eb", "F" , "Gb", "Ab", "Bb", "Cb", "Db" } },
    { { -7, true  }, { "Ab", "Bb", "Cb", "Db", "Eb", "Fb", "Gb" } },
};

#endif  // defined ENABLE_KEY_SCALES_MAPPING

std::map<int, std::string> keysiglist     // see "Key" columns above
{
    {  0,   "C",    },
    {  1,   "G",    },
    {  2,   "D",    },
    {  3,   "A",    },
    {  4,   "E",    },
    {  5,   "B",    },
    {  6,   "F#",   },
    {  7,   "C#",   },
    { -1,   "F",    },
    { -2,   "Bb",   },
    { -3,   "Eb",   },
    { -4,   "Ab",   },
    { -5,   "Db",   },
    { -6,   "Gb",   },
    { -7,   "Cb",   }
};

std::string
keysiginfo::key_name ()
{
    std::string result = keysiglist[m_sharp_flat_count];
    result += is_minor_scale() ? "minor" : "major" ;
    return result;
}

/*------------------------------------------------------------------------
 * Track Info
 *------------------------------------------------------------------------*/

/**
 *  Provides the default name/title for the track.
 */

const std::string trackinfo::sm_default_name = "Untitled";

/**
 *  Default constructor.
 */

trackinfo::trackinfo () :
    m_track_name    (sm_default_name),
    m_is_exportable (true),
    m_length        (0),
    m_tempo_info    (),
    m_timesig_info  (),
    m_keysig_info   (),
    m_channel       (midi::null_channel())
{
    // No code needed
}

/**
 *  Principal constructor.
 */

trackinfo::trackinfo
(
    const std::string & trackname,
    const tempoinfo & ti,
    const timesiginfo & tsi,
    const keysiginfo & ksi,
    bool exportable
) :
    m_track_name    (trackname),
    m_is_exportable (exportable),
    m_length        (0),
    m_tempo_info    (ti),
    m_timesig_info  (tsi),
    m_keysig_info   (ksi),
    m_channel       (midi::null_channel())
{
    // No code needed
}

/*------------------------------------------------------------------------
 * Free functions?
 *------------------------------------------------------------------------*/

}           // namespace midi

/*
 * trackinfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

