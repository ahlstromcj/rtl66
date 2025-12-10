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
 * \file          seq_event_codes.cpp
 *
 *    A mapping an snd_seq_event_type_t values to matching MIDI event
 *    statuses.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2025-12-10
 * \updates       2025-12-10
 * \license       See above.
 *
 *  Not sure yet if this will be useful, except as a reference.
 */

#include <alsa/seq_event.h>                     /* SND_SEQ_EVENT_xxx codes  */
#include "rtl/midi/alsa/seq_event_codes.hpp"    /* rtl::functions           */

#if defined RTL66_BUILD_ALSA

namespace rtl
{

struct snd_seq_event_midi_pairs
{
    unsigned char snd_seq_event,
    unsigned char midi_event_status;    // * might need uint16_t
};

static snd_seq_event_midi_pairs s_event_pairs
{
	{ SND_SEQ_EVENT_SYSTEM,                 0x00 },     /* system status    */
	{ SND_SEQ_EVENT_RESULT,                 0x00 },     /* result status    */
	{ SND_SEQ_EVENT_NOTE,                   0x00 },     /* note on/off/dur. */
	{ SND_SEQ_EVENT_NOTEON,                 0x90 },     /* note on          */
	{ SND_SEQ_EVENT_NOTEOFF,                0x80 },     /* note off         */
	{ SND_SEQ_EVENT_KEYPRESS,               0xA0 },     /* aftertouch       */
	{ SND_SEQ_EVENT_CONTROLLER,             0xB0 },
	{ SND_SEQ_EVENT_PGMCHANGE,              0xC0 },
	{ SND_SEQ_EVENT_CHANPRESS,              0xD0 },     /* channel pressure */
	{ SND_SEQ_EVENT_PITCHBEND,              0xE0 },
	{ SND_SEQ_EVENT_CONTROL14,              0x00 },
	{ SND_SEQ_EVENT_NONREGPARAM,            0x00 },
	{ SND_SEQ_EVENT_REGPARAM,               0x00 },
	{ SND_SEQ_EVENT_SONGPOS,                0x00 },
	{ SND_SEQ_EVENT_SONGSEL,                0x00 },
	{ SND_SEQ_EVENT_QFRAME,                 0xF1 },
	{ SND_SEQ_EVENT_TIMESIGN,               0x00 },     // *
	{ SND_SEQ_EVENT_KEYSIGN,                0x00 },     // *
	{ SND_SEQ_EVENT_START,                  0xFA },
	{ SND_SEQ_EVENT_CONTINUE,               0xFB },
	{ SND_SEQ_EVENT_STOP,                   0xFC },
	{ SND_SEQ_EVENT_SETPOS_TICK,            0x00 },
	{ SND_SEQ_EVENT_SETPOS_TIME,            0x00 },
	{ SND_SEQ_EVENT_TEMPO,                  0x00 },
	{ SND_SEQ_EVENT_CLOCK,                  0xF8 },
	{ SND_SEQ_EVENT_TICK,                   0x00 },
	{ SND_SEQ_EVENT_QUEUE_SKEW,             0x00 },
	{ SND_SEQ_EVENT_SYNC_POS,               0x00 },
	{ SND_SEQ_EVENT_TUNE_REQUEST,           0xF6 },
	{ SND_SEQ_EVENT_RESET,                  0x00 },
	{ SND_SEQ_EVENT_SENSING,                0x00 },
	{ SND_SEQ_EVENT_ECHO,                   0x00 },
	{ SND_SEQ_EVENT_OSS,                    0x00 },
	{ SND_SEQ_EVENT_CLIENT_START,           0x00 },
	{ SND_SEQ_EVENT_CLIENT_EXIT,            0x00 },
	{ SND_SEQ_EVENT_CLIENT_CHANGE,          0x00 },
	{ SND_SEQ_EVENT_PORT_START,             0x00 },
	{ SND_SEQ_EVENT_PORT_EXIT,              0x00 },
	{ SND_SEQ_EVENT_PORT_CHANGE,            0x00 },
	{ SND_SEQ_EVENT_PORT_SUBSCRIBED,        0x00 },
	{ SND_SEQ_EVENT_PORT_UNSUBSCRIBED,      0x00 },
	{ SND_SEQ_EVENT_UMP_EP_CHANGE,          0x00 },
	{ SND_SEQ_EVENT_UMP_BLOCK_CHANGE,       0x00 },
	{ SND_SEQ_EVENT_USR0,                   0x00 },
	{ SND_SEQ_EVENT_USR1,                   0x00 },
	{ SND_SEQ_EVENT_USR2,                   0x00 },
	{ SND_SEQ_EVENT_USR3,                   0x00 },
	{ SND_SEQ_EVENT_USR4,                   0x00 },
	{ SND_SEQ_EVENT_USR5,                   0x00 },
	{ SND_SEQ_EVENT_USR6,                   0x00 },
	{ SND_SEQ_EVENT_USR7,                   0x00 },
	{ SND_SEQ_EVENT_USR8,                   0x00 },
	{ SND_SEQ_EVENT_USR9,                   0x00 },
	{ SND_SEQ_EVENT_SYSEX,                  0xF0 },
	{ SND_SEQ_EVENT_BOUNCE,                 0x00 },
	{ SND_SEQ_EVENT_USR_VAR0,               0x00 },
	{ SND_SEQ_EVENT_USR_VAR1,               0x00 },
	{ SND_SEQ_EVENT_USR_VAR2,               0x00 },
	{ SND_SEQ_EVENT_USR_VAR3,               0x00 },
	{ SND_SEQ_EVENT_USR_VAR4,               0x00 },
	{ SND_SEQ_EVENT_NONE,                   0xFF }
};

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

/*
 * seq_event_codes.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

