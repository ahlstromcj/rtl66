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
 * \file          test_c_api.c
 *
 *      A test-file for the API names, codes, and lookups.
 *
 * \library       rtl66
 * \author        Jean Pierre Cimalando; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-03-06
 * \license       See above.
 *
 *      Tests that the C API for rtl (RtMidi refactored) is working.
 *
 *      On Linux, run this test both with ALSA and with JACK.
 */

#include <stdio.h>                      /* printf()                         */
#include <string.h>                     /* strcmp()                         */

#include "rtl/midi/rtmidi_c.h"

static struct RtMidiWrapper * s_midiin  = NULL;
static struct RtMidiWrapper * s_midiout = NULL;

int
main (int argc, char * argv [])
{
    bool can_run = rtmidi_simple_cli("test_api_c", argc, argv);
    if (can_run)
    {
        if ((s_midiin = rtmidi_in_create_default()))
        {
            unsigned int portcount = rtmidi_get_port_count(s_midiin);
            printf("-- MIDI input ports found: %u\n", portcount);
            rtmidi_close_port(s_midiin);
            rtmidi_in_free(s_midiin);
        }
        if ((s_midiout = rtmidi_out_create_default()))
        {
            unsigned int portcount = rtmidi_get_port_count(s_midiout);
            printf("-- MIDI output ports found: %u\n", portcount);
            rtmidi_close_port(s_midiout);
            rtmidi_out_free(s_midiout);
        }
    }
    return 0;
}

/*
 * test_c_api.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

