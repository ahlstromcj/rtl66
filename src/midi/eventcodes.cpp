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
 * \file          eventcodes.cpp
 *
 *  This module declares/defines the values for the MIDI status codes (also
 *  known as the event codes).
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2024-05-13
 * \license       GNU GPLv2 or above
 *
 *  This module also declares/defines the various constants, status-byte
 *  values, or data values for MIDI events.  These values were extracted from
 *  Seq66's event.hpp header file in order to be useful beyond that
 *  application.  In addition, the values were changed to enum class values
 *  for better partitioning.
 *
 *  The MIDI protocol consists of MIDI events that carry four types of messages:
 *
 *      -   Voice messages.  0x80 to 0xEF; includes channel information.
 *      -   System common messages.  0xF0 (SysEx) to 0xF7 (End of SysEx)
 *      -   System realtime messages. 0xF8 to 0xFF.
 *      -   Meta messages. 0xFF is the flag, followed by type, length, and data.
 *
 *  Control Change Messages:
 *
 *      This enumeration summarizes the MIDI Continuous Controllers (CC) that
 *      are available.
 *
 * Notes:
 *
 *  For balance and pan: 0 = hard left, 64 = center, 127 = hard right
 *
 *  For all on/off switches, 0 to 63 = Off and 64 to 127 = On.
 *
 *  Damper Pedal 64 versus Sostenuto CC 66:
 *
 *      Sustain is a pedal on/off switch that controls sustain.  Sostenuto is
 *      an on/off switch like the Sustain controller (CC 64), but it only holds
 *      notes that were on when the pedal was pressed.  People use it to “hold”
 *      chords” and play melodies over the held chord.
 *
 *  NRPN 98 and 99: Non-Registered Parameter Number LSB and MSB. For
 *  controllers 6, 38, 96, and 97, it selects the NRPN parameter.
 *
 *  RPN 100 and 101: Registered Parameter Number LSB and MSB. For controllers
 *  6, 38, 96, and 97, it selects the RPN parameter.
 *
 *  Local On/Off Switch 122: Turns the internal connection of a MIDI
 *  keyboard/workstation, etc., On or Off.  For a computer, one will most
 *  likely want Local Control off to avoid notes being played twice, once
 *  locally and twice when the note is sent back from the computer to your
 *  keyboard.
 *
 *  All Notes Off 123: Mutes all sounding notes. Release time will be
 *  maintained, and notes held by sustain will not turn off until sustain
 *  pedal is depressed.
 *
 *  Undefined values summary: 3, 9, 14-15, 20-31, 85-90, and 102-119.
 *
 *  Values 32 to 63 are for Controllers 0 to 31, the Least Significant Bit
 *  (LSB).
 *
 *  Copped from seq66/libseq66/include/midi/controllers.hpp.
 *
 *  But consider this, events other than System Exclusive, which do not have
 *  an arbitrary number of bytes, but a definite number. These events are:
 *
\verbatim
        -   Sequence No.:   FF 00 02 s1 s1
        -   MIDI Channel:   FF 20 01 cc
        -   MIDI Port:      FF 21 01 pp
        -   Set Tempo:      FF 51 03 tt tt tt
        -   SMPTE Offset:   FF 54 05 hh mm ss fr ff
        -   Time Signature: FF 58 04 nn dd cc bb
        -   Key Signature:  FF 59 02 sf mi
\endverbatim
 *
 *  The arbitrarily-sized Meta events are:
 *
\verbatim
        -   Text:           FF 01 len text
        -   Copyright:      FF 02 len text
        -   Track Name:     FF 03 len name
        -   Instrument:     FF 04 len name
        -   Marker:         FF 05 len text
        -   Cue Point:      FF 06 len text
        -   Seq. Specific:  FF 7F len data
\endverbatim
 *
 *  The maximum amount of constant-size data is 5 bytes.  We should aim to
 *  increase this and use it, using event::m_status as the "meta" byte and
 *  perhaps m_channel as the "meta-event" byte.  But curently, the tempo and
 *  time signature events are stored as data in the sequence object, so
 *  that's probably the best tact for the future.
 *
 *  Channel Voice Messages:
 *
 *      The MIDI events with status from 0x80 to 0xEF are channel messages.
 *      The comments in eventcodes.hpp represent the one or two data-bytes
 *      of the message.
 *
 *      Note that Channel Mode Messages use the same code as the Control Change,
 *      but uses reserved controller numbers ranging from 120 to 127.
 *
 *      -   All sound off: c = 120, v = 0
 *      -   Reset all controllers: c = 121, v = 0 (usually)
 *      -   Local control: c = 122, v = 0 (Off), v = 127 (On)
 *      -   All Notes off:
 *          -   All off: c = 123, v = 0
 *          -   Omni mode off: c = 124, v = 0
 *          -   Omni mode on: c = 125, v = 0
 *          -   Mono mode on (poly off): c = 126, v = 0 (Omni on) or M (channels)
 *          -   Poly mode on: c = 127, v = 0
 *
 *      The "any" (0x00) value may prove to be useful in allowing any event to be
 *      dealt with.  Not sure yet, but the cost is minimal.
 *
 *  System Messages:
 *
 *      The following MIDI events have no channel.  We have included redundant
 *      constant variables for the SysEx Start and End bytes just to make it
 *      clear that they are part of this sequence of values, though usually
 *      treated separately.
 *
 *      Only the following constants are followed by some data bytes:
 *
 *      -   sysex           = 0xF0  // ends with 0xF7
 *      -   quarter_frame   = 0xF1  // and 0x0n to 0x7n
 *      -   song_pos        = 0xF2  // and 0x0 to 0x3FFF 16th note
 *      -   song_select     = 0xF3  // and 0x0 to 0x7F song number
 *      -   tune_select     = 0xF6  // no data, tune yourself
 *
 *      A MIDI System Exclusive (SYSEX) message starts with F0, followed by the
 *      manufacturer ID (how many? bytes), a number of data bytes, and ended by
 *      an F7.
 *
 * MIDI System Real-Time Messages:
 *
 *      -   https://en.wikipedia.org/wiki/MIDI_beat_clock
 *      -   http://www.midi.org/techspecs/midimessages.php
 *
 *      Real-time message are used for synchronization of all clock-based units
 *      in a system.  They contain only a status byte, no data bytes.
 *
 * Meta Messages:
 *
 *      0xFF is a MIDI "escape code" used in MIDI files to introduce a MIDI meta
 *      event.  Note that it has the same code (0xFF) as the Reset message, but
 *      the Meta message is read from a MIDI file, while the Reset message is
 *      sent to the sequencer by other MIDI participants.
 *
 *
 */

#include <map>                          /* std::map<>                       */
#include "midi/eventcodes.hpp"          /* midi::status, to_byte(), etc.    */

/*
 *  Note that it is named "midi", not "rtl"; this scoping helps reduce the
 *  length of enumeration value names.
 */

namespace midi
{

/**
 *  An internal structure for accessing controller and other names via
 *  a status byte or meta byte. Bare midi::bytes are used.
 */

using namepair = struct
{
    byte number;
    std::string name;
};

/**
 *  An internal map to support looking up the names of status bytes.
 */

using midi_name_map = std::map<midi::byte, std::string>;

/*
 *  Functions used in analysizing MIDI events by external callers.
 *
 * is_one_byte (byte m):
 *
 *      Test for channel messages that have only one data byte:
 *
 *          -   Program Change      = 0xC0
 *          -   Channel Pressure    = 0xD0
 *
 *      The rest of the channel messages have two data bytes.
 *
 *      We use the actual bytes rather than the status enumeration to save a
 *      couple of static_casts.
 *
 *          -   to_byte(status::program_change)
 *          -   to_byte(status::channel_pressure)
 *
 * is_two_byte (byte m):
 *
 *      Test for channel messages that have two data bytes: Note On, Note Off,
 *      Control Change, Aftertouch, and Pitch Wheel.  We use the actual bytes,
 *      to save some static_casts.
 *
 * status_msg_size (byte m):
 *
 *      In addition to the functions above, this one get the actual expected
 *      size of the message.  Returns -1 for SysEx.
 *
 * is_note (byte m):
 *
 *      Tests for messages that involve notes and velocity: Note On,
 *      Note Off, and Aftertouch.
 *
 * is_note_off_velocity (byte status, byte vel):
 *
 *      Tests for a Note On with a velocity of 0.  This function is used in the
 *      midifile module and in the is_note_off_recorded() function in Seq66.
 *
 *  There are many more such "one-liner" functions.  See the header file.
 */

/**
 *  In addition to the functions above, this one get the actual expected
 *  size of a status message.
 *
 * \param s
 *      Must provide a status byte, otherwise this call makes no sense.
 *
 * \return
 *      Returns the expected size of the message, or (-1) if other data
 *      is needed to get the message size. Includes the size of the status
 *      byte.
 */

int
status_msg_size (byte s)
{
    int result = (-1);
    if (is_two_byte_msg(s) || s == 0xF2)        /* status = song_pos        */
    {
        result = 3;                             /* status + d0 + d1         */
    }
    else if
    (
        is_one_byte_msg(s) || s == 0xF1 ||      /* status = quarter frame   */
        s == 0xF3                               /* status = song_select     */
    )
    {
        result = 2;                             /* status + d0              */
    }
    else if
    (
        s == 0xF6 ||                            /* status = tune_select     */
        s == 0xF8 ||                            /* status = clock           */
        s == 0xFA ||                            /* status = clk_start       */
        s == 0xFB ||                            /* status = clk_continue    */
        s == 0xFC ||                            /* status = clk_stop        */
        s == 0xFE ||                            /* status = active_sense    */
        s == 0xFF                               /* status = reset/meta !!!  */
    )
    {
        result = 1;
    }
    return result;
}

/**
 *  Get the actual expected size of a Meta message.
 *
 * \param m
 *      Must provide a meta byte, otherwise this call makes no sense.
 *
 * \return
 *      Returns the expected size of the message, or (-1) if other data
 *      is needed to get the message size.  Includes the size of the meta
 *      status byte (0xFF).
 */

int
meta_msg_size (byte m)
{
    int result = (-1);
    if (m == 0x51)                      /* set_tempo                        */
    {
        result = 6;
    }
    else if (m == 0x58)                 /* time_signature                   */
    {
        result = 7;
    }
    else if (m == 0x00 || m == 0x59)    /* seq_number, key_signature        */
    {
        result = 5;
    }
    else if (m == 0x20 || m == 0x21)    /* midi_channel, midi_port (deprec) */
    {
        result = 4;
    }
    else if (m == 0x54)                 /* smpte_offset                     */
    {
        result = 8;
    }
    return result;
}

/**
 *  Provides the default names of MIDI controllers.  This array is used
 *  only by the seqedit class.
 */

static const namepair
c_midi_controller_names [c_byte_data_max]
{
    {   0, "Bank Select"                        },
    {   1, "Modulation Wheel "                  },
    {   2, "Breath controller "                 },
    {   3, "---"                                },
    {   4, "Foot Pedal "                        },
    {   5, "Portamento Time "                   },
    {   6, "Data Entry "                        },
    {   7, "Volume "                            },
    {   8, "Balance "                           },
    {   9, "---"                                },
    {  10, "Pan position"                       },
    {  11, "Expression "                        },
    {  12, "Effect Control 1 "                  },
    {  13, "Effect Control 2 "                  },
    {  14, "---"                                },
    {  15, "---"                                },
    {  16, "General Purpose Slider 1"           },
    {  17, "General Purpose Slider 2"           },
    {  18, "General Purpose Slider 3"           },
    {  19, "General Purpose Slider 4"           },
    {  20, "---"                                },
    {  21, "---"                                },
    {  22, "---"                                },
    {  23, "---"                                },
    {  24, "---"                                },
    {  25, "---"                                },
    {  26, "---"                                },
    {  27, "---"                                },
    {  28, "---"                                },
    {  29, "---"                                },
    {  30, "---"                                },
    {  31, "---"                                },
    {  32, "Bank Select (fine)"                 },
    {  33, "Modulation Wheel (fine)"            },
    {  34, "Breath controller (fine)"           },
    {  35, "---"                                },
    {  36, "Foot Pedal (fine)"                  },
    {  37, "Portamento Time (fine)"             },
    {  38, "Data Entry (fine)"                  },
    {  39, "Volume (fine)"                      },
    {  40, "Balance (fine)"                     },
    {  41, "---"                                },
    {  42, "Pan position (fine)"                },
    {  43, "Expression (fine)"                  },
    {  44, "Effect Control 1 (fine)"            },
    {  45, "Effect Control 2 (fine)"            },
    {  46, "---"                                },
    {  47, "---"                                },
    {  48, "---"                                },
    {  49, "---"                                },
    {  50, "---"                                },
    {  51, "---"                                },
    {  52, "---"                                },
    {  53, "---"                                },
    {  54, "---"                                },
    {  55, "---"                                },
    {  56, "---"                                },
    {  57, "---"                                },
    {  58, "---"                                },
    {  59, "---"                                },
    {  60, "---"                                },
    {  61, "---"                                },
    {  62, "---"                                },
    {  63, "---"                                },
    {  64, "Hold Pedal (on/off)"                },
    {  65, "Portamento (on/off)"                },
    {  66, "Sustenuto Pedal (on/off)"           },
    {  67, "Soft Pedal (on/off)"                },
    {  68, "Legato Pedal (on/off)"              },
    {  69, "Hold 2 Pedal (on/off)"              },
    {  70, "Sound Variation"                    },
    {  71, "Sound Timbre"                       },
    {  72, "Sound Release Time"                 },
    {  73, "Sound Attack Time"                  },
    {  74, "Sound Brightness"                   },
    {  75, "Sound Control 6"                    },
    {  76, "Sound Control 7"                    },
    {  77, "Sound Control 8"                    },
    {  78, "Sound Control 9"                    },
    {  79, "Sound Control 10"                   },
    {  80, "General Purpose Button 1"           },
    {  81, "General Purpose Button 2"           },
    {  82, "General Purpose Button 3"           },
    {  83, "General Purpose Button 4"           },
    {  84, "---"                                },
    {  85, "---"                                },
    {  86, "---"                                },
    {  87, "---"                                },
    {  88, "---"                                },
    {  89, "---"                                },
    {  90, "---"                                },
    {  91, "Effects Level"                      },
    {  92, "Tremulo Level"                      },
    {  93, "Chorus Level"                       },
    {  94, "Celeste Level"                      },
    {  95, "Phaser Level"                       },
    {  96, "Data Button increment"              },
    {  97, "Data Button decrement"              },
    {  98, "Non-registered Parameter fine"      },
    {  99, "Non-registered Parameter coarse"    },
    { 100, "Registered Parameter fine"          },
    { 101, "Registered Parameter coarse"        },
    { 102, "---"                                },
    { 103, "---"                                },
    { 104, "---"                                },
    { 105, "---"                                },
    { 106, "---"                                },
    { 107, "---"                                },
    { 108, "---"                                },
    { 109, "---"                                },
    { 110, "---"                                },
    { 111, "---"                                },
    { 112, "---"                                },
    { 113, "---"                                },
    { 114, "---"                                },
    { 115, "---"                                },
    { 116, "---"                                },
    { 117, "---"                                },
    { 118, "---"                                },
    { 119, "---"                                },
    { 120, "All Sound Off"                      },
    { 121, "All Controllers Off"                },
    { 122, "Local Keyboard On/Off"              },
    { 123, "All Notes Off"                      },
    { 124, "Omni Mode Off"                      },
    { 125, "Omni Mode On"                       },
    { 126, "Mono On"                            },
    { 127, "Poly On"                            }
};

std::string
midi_controller_name (int index)
{
    std::string result;
    if (index < c_byte_data_max)
    {
        std::string name = c_midi_controller_names[index].name;
        result = std::to_string(index);
        result += " ";
        result += name;
    }
    return result;
}

/**
 *  Provides the default names of General MIDI program changes.  Note that the
 *  numbering starts from 0 internally.  We could add support for this kind of
 *  list in usrsettings, or the note-mapper, or in a new 'patch' configuration
 *  file that holds this mapping.
 */

static const namepair
c_gm_program_names [c_byte_data_max]
{
    {   0, "Acoustic Grand Piano"       },
    {   1, "Bright Acoustic Piano"      },
    {   2, "Electric Grand Piano"       },
    {   3, "Honky-tonk Piano"           },
    {   4, "Electric Piano 1"           },
    {   5, "Electric Piano 2"           },
    {   6, "Harpsichord"                },
    {   7, "Clavi"                      },
    {   8, "Celesta"                    },
    {   9, "Glockenspiel"               },
    {  10, "Music Box"                  },
    {  11, "Vibraphone"                 },
    {  12, "Marimba"                    },
    {  13, "Xylophone"                  },
    {  14, "Tubular Bells"              },
    {  15, "Dulcimer"                   },
    {  16, "Drawbar Organ"              },
    {  17, "Percussive Organ"           },
    {  18, "Rock Organ"                 },
    {  19, "Church Organ"               },
    {  20, "Reed Organ"                 },
    {  21, "Accordion"                  },
    {  22, "Harmonica"                  },
    {  23, "Tango Accordion"            },
    {  24, "Acoustic Guitar (nylon)"    },
    {  25, "Acoustic Guitar (steel)"    },
    {  26, "Electric Guitar (jazz)"     },
    {  27, "Electric Guitar (clean)"    },
    {  28, "Electric Guitar (muted)"    },
    {  29, "Overdriven Guitar"          },
    {  30, "Distortion Guitar"          },
    {  31, "Guitar harmonics"           },
    {  32, "Acoustic Bass"              },
    {  33, "Electric Bass (finger)"     },
    {  34, "Electric Bass (pick)"       },
    {  35, "Fretless Bass"              },
    {  36, "Slap Bass 1"                },
    {  37, "Slap Bass 2"                },
    {  38, "Synth Bass 1"               },
    {  39, "Synth Bass 2"               },
    {  40, "Violin"                     },
    {  41, "Viola"                      },
    {  42, "Cello"                      },
    {  43, "Contrabass"                 },
    {  44, "Tremolo Strings"            },
    {  45, "Pizzicato Strings"          },
    {  46, "Orchestral Harp"            },
    {  47, "Timpani"                    },
    {  48, "String Ensemble 1"          },
    {  49, "String Ensemble 2"          },
    {  50, "SynthStrings 1"             },
    {  51, "SynthStrings 2"             },
    {  52, "Choir Aahs"                 },
    {  53, "Voice Oohs"                 },
    {  54, "Synth Voice"                },
    {  55, "Orchestra Hit"              },
    {  56, "Trumpet"                    },
    {  57, "Trombone"                   },
    {  58, "Tuba"                       },
    {  59, "Muted Trumpet"              },
    {  60, "French Horn"                },
    {  61, "Brass Section"              },
    {  62, "SynthBrass 1"               },
    {  63, "SynthBrass 2"               },
    {  64, "Soprano Sax"                },
    {  65, "Alto Sax"                   },
    {  66, "Tenor Sax"                  },
    {  67, "Baritone Sax"               },
    {  68, "Oboe"                       },
    {  69, "English Horn"               },
    {  70, "Bassoon"                    },
    {  71, "Clarinet"                   },
    {  72, "Piccolo"                    },
    {  73, "Flute"                      },
    {  74, "Recorder"                   },
    {  75, "Pan Flute"                  },
    {  76, "Blown Bottle"               },
    {  77, "Shakuhachi"                 },
    {  78, "Whistle"                    },
    {  79, "Ocarina"                    },
    {  80, "Lead 1 (square)"            },
    {  81, "Lead 2 (sawtooth)"          },
    {  82, "Lead 3 (calliope)"          },
    {  83, "Lead 4 (chiff)"             },
    {  84, "Lead 5 (charang)"           },
    {  85, "Lead 6 (voice)"             },
    {  86, "Lead 7 (fifths)"            },
    {  87, "Lead 8 (bass + lead)"       },
    {  88, "Pad 1 (new age)"            },
    {  89, "Pad 2 (warm)"               },
    {  90, "Pad 3 (polysynth)"          },
    {  91, "Pad 4 (choir)"              },
    {  92, "Pad 5 (bowed)"              },
    {  93, "Pad 6 (metallic)"           },
    {  94, "Pad 7 (halo)"               },
    {  95, "Pad 8 (sweep)"              },
    {  96, "FX 1 (rain)"                },
    {  97, "FX 2 (soundtrack)"          },
    {  98, "FX 3 (crystal)"             },
    {  99, "FX 4 (atmosphere)"          },
    { 100, "FX 5 (brightness)"          },
    { 101, "FX 6 (goblins)"             },
    { 102, "FX 7 (echoes)"              },
    { 103, "FX 8 (sci-fi)"              },
    { 104, "Sitar"                      },
    { 105, "Banjo"                      },
    { 106, "Shamisen"                   },
    { 107, "Koto"                       },
    { 108, "Kalimba"                    },
    { 109, "Bag pipe"                   },
    { 110, "Fiddle"                     },
    { 111, "Shanai"                     },
    { 112, "Tinkle Bell"                },
    { 113, "Agogo"                      },
    { 114, "Steel Drums"                },
    { 115, "Woodblock"                  },
    { 116, "Taiko Drum"                 },
    { 117, "Melodic Tom"                },
    { 118, "Synth Drum"                 },
    { 119, "Reverse Cymbal"             },
    { 120, "Guitar Fret Noise"          },
    { 121, "Breath Noise"               },
    { 122, "Seashore"                   },
    { 123, "Bird Tweet"                 },
    { 124, "Telephone Ring"             },
    { 125, "Helicopter"                 },
    { 126, "Applause"                   },
    { 127, "Gunshot"                    }
};

std::string
gm_program_name (int index)
{
    std::string result;
    if (index < c_byte_data_max)
    {
        std::string name = c_gm_program_names[index].name;
        result = std::to_string(index);
        result += " ";
        result += name;
    }
    return result;
}

/**
 *  Provides the default names of General MIDI program changes.  Note that the
 *  numbering starts from 0 internally.  We could add support for this kind of
 *  list in usrsettings, or the note-mapper, or in a new 'patch' configuration
 *  file that holds this mapping.
 *
 *  Here follows the Standard GM Drum Kit (Program Change 0).
 */

static const namepair
c_gm_percussion_names [c_byte_data_max]
{
    {   35,     "Acoustic Bass Drum"    },
    {   36,     "Bass Drum 1"           },
    {   37,     "Side Stick"            },
    {   38,     "Acoustic Snare"        },
    {   39,     "Hand Clap"             },
    {   40,     "Electric Snare"        },
    {   41,     "Low Floor Tom"         },
    {   42,     "Closed Hi-Hat"         },
    {   43,     "High Floor Tom"        },
    {   44,     "Pedal Hi-Hat"          },
    {   45,     "Low Tom"               },
    {   46,     "Open Hi-Hat"           },
    {   47,     "Low-Mid Tom"           },
    {   48,     "Hi-Mid Tom"            },
    {   49,     "Crash Cymbal 1"        },
    {   50,     "High Tom"              },
    {   51,     "Ride Cymbal 1"         },
    {   52,     "Chinese Cymbal"        },
    {   53,     "Ride Bell"             },
    {   54,     "Tambourine"            },
    {   55,     "Splash Cymbal"         },
    {   56,     "Cowbell"               },
    {   57,     "Crash Cymbal 2"        },
    {   58,     "Vibraslap"             },
    {   59,     "Ride Cymbal 2"         },
    {   60,     "Hi Bongo"              },
    {   61,     "Low Bongo"             },
    {   62,     "Mute Hi Conga"         },
    {   63,     "Open Hi Conga"         },
    {   64,     "Low Conga"             },
    {   65,     "High Timbale"          },
    {   66,     "Low Timbale"           },
    {   67,     "High Agogo"            },
    {   68,     "Low Agogo"             },
    {   69,     "Cabasa"                },
    {   70,     "Maracas"               },
    {   71,     "Short Whistle"         },
    {   72,     "Long Whistle"          },
    {   73,     "Short Guiro"           },
    {   74,     "Long Guiro"            },
    {   75,     "Claves"                },
    {   76,     "Hi Wood Block"         },
    {   77,     "Low Wood Block"        },
    {   78,     "Mute Cuica"            },
    {   79,     "Open Cuica"            },
    {   80,     "Mute Triangle"         },
    {   81,     "Open Triangle"         }
};

/**
 *  For some devices, one can change the patch/program on channel 10.  Here
 *  are some of the kits available:
 *
 * GM Drum Kits:
 *
 *       1  Standard Drum Kit
 *       8  Room Drum Kit
 *      16  Power Drum Kit
 *      24  Electric Drum Kit
 *      25  Rap TR808 Drums
 *      33  Jazz Drum Kit
 *      41  Brush Kit
 *
 * Additional kits for the Microsoft GS Wavetable SW Synth:
 *
 *      48  Orchestra
 *      56  SFX
 *
 *  Can we do anything with this?
 *
 *      "As long as your MIDI device supports GM2, then you can choose your drum
 *      set with a program change message.  Be sure to send the 'GM2 On' SysEx
 *      message first.  Details are included in the GM2 specification."
 *
 *      "Drum kits are usually selected via Bank Select commands."
 */

std::string
gm_percussion_name (int index)
{
    std::string result;
    if (index >= 35 && index < 82)      /* see the numbers above */
    {
        std::string name = c_gm_program_names[index].name;
        result = std::to_string(index);
        result += " ";
        result += name;
    }
    return result;
}

/**
 *  Name of the initial text meta events (00 through 07).
 *
 *  TODO: Use the namepair structure as the key.
 */

static const std::string sm_meta_text_labels [] =
{
    "Seq number",
    "Text",
    "Copyright",
    "Track Name",
    "Instrument Name",
    "Lyric",
    "Marker",
    "Cue Point",
    "Program Name",     /* TODO: backport to Seq66  */
    "Port Name"         /* TODO: backport to Seq66  */
};

std::string
meta_text_label (byte m)
{
    std::string result{"Unknown"};
    if (m < 8)
    {
        result = sm_meta_text_labels[int(m)];
    }
    else
    {
        switch (m)
        {
        case 0x20:  result = "MIDI Channel";        break;
        case 0x21:  result = "MIDI Port";           break;
        case 0x2f:  result = "End of Track";        break;
        case 0x51:  result = "Set Tempo";           break;
        case 0x54:  result = "SMPTE Offset";        break;
        case 0x58:  result = "Time Signature";      break;
        case 0x59:  result = "Key Signature";       break;
        case 0x7f:  result = "Seq Spec";            break;
        }
    }
    return result;
}

/*
 *  Basic statuses names.
 */

static const midi_name_map s_status_names
{
    { 0x80, "Note Off"              },
    { 0x90, "Note On"               },
    { 0xA0, "Aftertouch"            },
    { 0xB0, "Control"               },
    { 0xC0, "Program"               },
    { 0xD0, "Ch Pressure"           },
    { 0xE0, "Pitch Wheel"           },
    { 0xF0, "System Exclusive"      },
    { 0xF1, "Quarter Frame"         },
    { 0xF2, "Song Positoin"         },
    { 0xF3, "Song Select"           },
    { 0xF4, "Song F4"               },
    { 0xF5, "Song F5"               },
    { 0xF6, "Tune Select"           },
    { 0xF7, "Sysex Continue/End"    },
    { 0xF8, "MIDI Clock"            },
    { 0xF9, "Timing Tick"           },
    { 0xFA, "Clock Start"           },
    { 0xFB, "Clock Continue"        },
    { 0xFC, "Clock Stop"            },
    { 0xFD, "Song FD"               },
    { 0xFE, "Active Sense"          },
    { 0xFF, "Meta Message"          }
};

std::string
status_label (byte m)
{
    std::string result{"Unknown"};
    const auto iter = s_status_names.find(m);
    if (iter != s_status_names.end())
        result = iter->second;

    return result;
}

/*
 *  Meta messages names.
 */

static const midi_name_map s_meta_names
{
    { 0x00, "Seq Number"        },
    { 0x01, "Text Event"        },
    { 0x02, "Copyright"         },
    { 0x03, "Track Name"        },
    { 0x04, "Instrument"        },
    { 0x05, "Lyric"             },
    { 0x06, "Marker"            },
    { 0x07, "Cue Point"         },
    { 0x08, "Program Name"      },
    { 0x09, "Port Name"         },
    { 0x20, "MIDI Channel"      },
    { 0x21, "MIDI Port"         },
    { 0x2F, "End of Track"      },
    { 0x51, "Set Tempo"         },
    { 0x54, "SMPTE Offset"      },
    { 0x58, "Time Signature"    },
    { 0x59, "Key Signature"     },
    { 0x7F, "Seq Spec"          }
};

std::string
meta_label (byte m)
{
    std::string result{"Unknown"};
    const auto iter = s_meta_names.find(m);
    if (iter != s_meta_names.end())
        result = iter->second;

    return result;
}

}           // namespace midi

/*
 * eventcodes.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

