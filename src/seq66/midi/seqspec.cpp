/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          seqspec.cpp
 *
 *  This module defines number for sequencer-specific MIDI events in a
 *  sequence/track.
 *
 * \library       seq66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2022-07-05
 * \license       GNU GPLv2 or above
 *
 */

#include <map>                          /* std::map class                   */

#include "midi/seqspec.hpp"             /* enum class midi::seqspec         */

namespace seq66
{

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

/**
 *  Provides tags used by the midifile class to control the reading and
 *  writing of the extra "proprietary" information stored in a Seq24 MIDI
 *  file.  See the cpp file for more information.
 */

std::string
to_string (seqspec sp)
{
    static midi::long s_min = static_cast<midi::long> seqspec::midibus;
    static midi::long s_max = static_cast<midi::long> seqspec::max;
    static std::map<midi::long, std::string> s_string_map
    {
        { midibus,          "MIDI Bus"                  },
        { midichannel,      "MIDI Channel"              },
        { midiclocks,       "MIDI Clocks"               },
        { triggers,         "Triggers (legacy)"         },
        { notes,            "Set Name/Notes"            },
        { timesig,          "Time Signature"            },
        { bpmtag,           "Beats/Minute"              },
        { triggers_ex,      "Triggers (Seq64)"          },
        { mutegroups,       "Mute Groups"               },
        { gap_A,            "Gap A"                     },
        { gap_B,            "Gap B"                     },
        { gap_C,            "Gap C"                     },
        { gap_D,            "Gap D"                     },
        { gap_E,            "Gap E"                     },
        { gap_F,            "Gap F"                     },
        { midictrl,         "MIDI Control (obsolete)"   },
        { musickey,         "Music Key"                 },
        { musicscale,       "Music Scale"               },
        { backsequence,     "Background Sequence"       },
        { transpose,        "Transposition"             },
        { perf_bp_mes,      "Beats/Measure (Seq32)"     },
        { perf_bw,          "Beat Width (Seq32)"        },
        { tempo_map,        "Tempo Map (Seq32)"         },
        { reserved_1,       "Reserved 1"                },
        { reserved_2,       "Reserved 2"                },
        { tempo_track,      "Tempo Track Number"        },
        { seq_color,        "Pattern Color"             },
        { seq_edit_mode,    "Pattern Edit Mode"         },
        { seq_loopcount,    "Pattern Loop Count"        },
        { reserved_3,       "Reserved 3"                },
        { reserved_4,       "Reserved 4"                },
        { trig_transpose,   "Triggers (Seq66)"          }
    };
    std::string result;
    midi::long splong = static_cast<midi::long>(sp);
    if (splong >= s_min and splong < s_max)
        result = s_string_map[sp];

    return result;
}

}           // namespace seq66

/*
 * seqspec.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

