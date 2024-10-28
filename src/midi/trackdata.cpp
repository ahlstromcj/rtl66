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
 * \file          trackdata.cpp
 *
 *  This module declares a class for holding and managing MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2024-05-22
 *
 * \license       GNU GPLv2 or above
 *
 *  This class is important when writing the MIDI and sequencer data out to a
 *  MIDI file, or reading it in. The data handled here are specific to a single
 *  sequence/pattern/track.
 *
 *      -   Reads a whole file into the byte-vector.
 *      -   Write the byte-vector to a file.
 *      -   Provides functions to get various MIDI events from the byte-vector
 *          and put them into events or data structures such as timesiginfo.
 *      -   Similarly for putting data back into the byte-vector.
 *
 *  Somewhat analogous to the Seq66 midi_vector class. It combines the
 *  functionality of that class and seq66::midifile, so that we don't have
 *  separate modules for input and output.
 *
 *  Here are the mappings between old and new functions. The trackdata
 *  versions generally do more checking, but no error reporting.
 *  Note that the some of the trackdata function return types are in the
 *  "midi" namespace. Some functions have been commented out, and are
 *  marked with an exclamation point.
 *
 *  trackdata              midifile                   midi_vector/_base
 *
 *  seek()                 read_seek()
 *  peek()                 peek()
 *  get()                  read_byte()
 *  get_short()            read_short()
 *  get_long()             read_long()
 *  get_status() !!        ----------------
 *  get_array() x 2 !!     read_byte_array()
 *  get_meta() !!          read_meta_data()
 *  get_string() !!        read_string()
 *  get_varinum()          read_varinum()
 *  get_meta_text() !!     read_meta_data()
 *  get_track_name() !!    read_track_name()
 *  get_tempo() !!         ----------------
 *  get_time_signature()!! ----------------
 *  get_key_signature() !! ----------------
 *  extract_meta_msg()
 *  extract_track_number()
 *  extract_track_name()
 *  extract_end_of_track()
 *  extract_tempo()
 *  extract_time_signature()
 *  extract_key_signature()
 *  ----------------       read_split_long()
 *  put()                  write_byte()
 *  put_varinum()          write_varinum()          add_varinum()
 *  put_triple()           write_triple()
 *  put_long()             write_long()             add_long()
 *  put_short()            write_short()            add_short()
 *  put_channel_event()    ----------------         add_event()
 *  put_ex_event()         ----------------         add_ex_event()
 *  put_meta_header()      ----------------         put_meta()
 *  put_meta()             ----------------
 *  put_meta_text()        ----------------         fill_meta_text()
 *  put_meta_track_end()   write_track_end()        fill_meta_track_end()
 *  put_seqspec()          ----------------
 *  put_seqspec_code()     ----------------
 *  put_seqspec_data()     ----------------
 *  put_track()            write_track()            fill()
 *  put_track_events()     write_track()            fill()
 *  put_track_number()     write_seq_number()       fill_seq_number()
 *  put_track_name()       write_track_name()       fill_seq_name()
 *  put_track_end()        write_track_end()
 *  put_time_sig() x 2     write_time_sig()         fill_time_sig()
 *  put_tempo()            write_start_tempo()      fill_tempo()
 *  put_key_sig() x 2      -----------
 *  ----------------       write_split_long()
 *  ----------------       ----------------         fill_proprietary()
 *  ----------------       ----------------         song_fill_track()
 *  ----------------       ----------------         song_fill_seq_event()
 *  ----------------       ----------------         song_fill_seq_trigger()
 */

#include <fstream>                      /* std::ifstream and std::ofstream  */

#include "c_macros.h"                   /* errprint() macro                 */
#include "midi/calculations.hpp"        /* midi::log2_power_of_2()          */
#include "midi/eventlist.hpp"           /* midi::eventlist & event classes  */
#include "midi/player.hpp"              /* midi::player class               */
#include "midi/trackdata.hpp"           /* midi::trackdata class            */
#include "midi/track.hpp"               /* midi::track class                */

namespace midi
{

/**
 *  The maximum length of a track name.
 */

static const int c_trackname_max =  256;

/**
 *  The maximum allowed variable length value for a MIDI file, which allows
 *  the length to fit in a 32-bit integer.
 */

static const int c_varlength_max = 0x0FFFFFFF;

/**
 *  Provides the track number for the SeqSpec data when using
 *  the new format.  (There is no track number for the legacy format.)
 *  Can't use numbers, such as 0xFFFF, that have MIDI meta tags in them,
 *  confuses our SeqSpec track parser.

static const midi::ushort c_prop_seq_number_old = 0x7777;
 */

static const midi::ushort c_prop_seq_number     = 0x3FFF;

/**
 *  Fills in the few members of this class.
 *
 * \param seq
 *      Provides a reference to the sequence/track for which this container
 *      holds MIDI data.
 */

trackdata::trackdata () :
    m_events                (),
    m_data                  (),
    m_running_status_action (rsaction::recover),
    m_manufacturer_id       (),
    m_end_of_track_found    (false)
{
    // Empty body
}

/**
 *  Put here for debuggability.
 */

size_t
trackdata::position () const
{
    return m_data.position();
}

size_t
trackdata::real_position () const
{
    return m_data.real_position();
}

/*-------------------------------------------------------------------------
 * "get" functions are defined in the header unless debugging.
 *-------------------------------------------------------------------------*/

#if defined PLATFORM_DEBUG_TMI

static void
show_byte (midi::byte b, size_t pos)
{
    static int s_counter = 0;
    printf("b[%03zx]=%02x ", pos, b);
    if ((++s_counter % 8) == 0)
        printf("\n");
}

midi::byte
trackdata::get () const
{
    midi::byte b = m_data.get_byte();
    show_byte(b, position());
    return b;
}

midi::byte
trackdata::peek () const
{
    midi::byte b = m_data.peek_byte();
    show_byte(b, position());
    return b;
}

#endif

/*-------------------------------------------------------------------------
 * "extract" functions
 *-------------------------------------------------------------------------*/

/**
 *  This function gets generic or unsupported meta. It simply stores the
 *  bytes and creates the event, so that it can be reconstructed when
 *  writing the MIDI file.
 *
 * \param trk
 *      Provides the track object, just in case.
 *
 * \param e
 *      Provides a prepped event [see the call to extract_meta_msg(), around
 *      lines 1782 and 1830] that will hold the event.
 *
 * \param metatype
 *      The byte representing the type of meta event, so it can be properly
 *      identified.
 *
 * \param len
 *      Holds the expected length of the meta event. This value could be 0.
 */

bool
trackdata::extract_generic_meta
(
    track & /* trk */, event & e,
    midi::meta metatype, size_t len
)
{
    midi::bytes metadata;
    for (size_t i = 0; i < len; ++i)
    {
        midi::byte c = get();
        metadata.push_back(c);
    }

    bool result = e.append_meta_data(metatype, metadata);
    if (result)
        result = append_event(e);

    return result;
}

/**
 *  Extracts a track number meta message. Handles both variants, as described
 *  in the "MIDI Meta and System Exclusive Events" section of the Rtl66
 *  manual:
 *
 *      FF 00 02 ss ss  (length == 2)
 *      FF 00 00        (length == 0)
 *
 * \param trk
 *      Provides the track object, so that its track-number can be set.
 *
 * \param e
 *      Provides a prepped event [see extract_meta_msg()] that will
 *      hold the track-number event.
 *
 * \param len
 *      If 2, the first format above is used. If 0, the second format
 *      is used. In the second case, the track number is already present
 *      in the track, and is not changed.
 *
 * \return
 *      Returns true if the event could be modified and appended to the
 *      event list.
 */

bool
trackdata::extract_track_number (track & trk, event & e, size_t len)
{
    bool no_track = len == 0;
    midi::ushort n = no_track ? trk.track_number() : get_short() ;
    track::number tn = track::number(n);
    midi::bytes b;
    b.push_back((n & 0xff00) >> 8);
    b.push_back(n & 0x00ff);

    bool result = e.append_meta_data(midi::meta::seq_number, b);
    if (result)
    {
        result = append_event(e);
        if (result)
            trk.track_number(tn);
    }
    return result;
}

/**
 *  Reads the track name. This could be generalized to get all meta
 *  text events. Could call get_meta_text() instead.
 *
 * \param trk
 *      Provides the track object, so that its track-name can be set.
 *
 * \param e
 *      Provides a prepped event [see the call to extract_meta_msg(), around
 *      lines 1782 and 1830] that will hold the track-name event.
 *
 * \param len
 *      Holds the expected length of the track name. This value could be 0.
 *
 * \return
 *      Returns true if the event could be modified and appended to the event
 *      list.
 */

bool
trackdata::extract_track_name (track & trk, event & e, size_t len)
{
    midi::bytes trackname;              /* track name from MIDI file data   */
    for (size_t i = 0; i < len; ++i)
    {
        midi::byte c = get();
        if (i < c_trackname_max)
            trackname.push_back(c);
        else
            break;
    }

    bool result = e.append_meta_data(midi::meta::track_name, trackname);
    if (result)
    {
        result = append_event(e);
        if (result)
            trk.track_name(bytes_to_string(trackname));
    }
    return result;
}

/**
 *  This function handles all meta text events except for track name.
 *  It is currently the same as extract_generic_meta().
 *
 * \param trk
 *      Provides the track object, so that its track-name can be set.
 *
 * \param e
 *      Provides a prepped event [see the call to extract_meta_msg(), around
 *      lines 1782 and 1830] that will hold the track-name event.
 *
 * \param metatype
 *      The byte representing the type of meta text event, so it can be properly
 *      identified.
 *
 * \param len
 *      Holds the expected length of the track name. This value could be 0.
 *
 * \return
 *      Returns true if the event could be modified and appended to the event
 *      list.
 */

bool
trackdata::extract_text_event
(
    track & /* trk */, event & e,
    midi::meta metatype, size_t len
)
{
    midi::bytes metadata;
    for (size_t i = 0; i < len; ++i)
    {
        midi::byte c = get();
        metadata.push_back(c);
    }

    bool result = e.append_meta_data(metatype, metadata);
    if (result)
    {
        result = append_event(e);
        if (result)
        {
#if defined RTL66_USE_SONG_INFO     // TO WORK OUT AT A LATER DATE
            bool get_song_info = track == 0 &&
                mtype == midi::meta::text_event &&
                ! got_song_info
                ;
            if (get_song_info)
            {
                got_song_info = true;
                p.song_info(e.get_text());
            }
#endif
        }
    }
    return result;
}

/**
 *  Reads the end-of-track event. This event is not optional.
 *
 * \param trk
 *      Provides the track object, so that its length can be set.
 *
 * \param e
 *      Provides a prepped event [see extract_meta_msg()] that will
 *      hold the end-of-track event.
 *
 * \return
 *      Returns true if the track name was not empty and if the event could
 *      then be modified and appended to the event list.
 */

bool
trackdata::extract_end_of_track (track & /* trk */, event & e)
{
    midi::bytes nodata;
    bool result = e.append_meta_data(midi::meta::end_of_track, nodata);
    if (result)
    {
        result = append_event(e);
        if (result)
            m_end_of_track_found = true;

        /*
         *  This is better done at install-track time, in the
         *  player::set_parent() function.
         *
         *  if (result)
         *      trk.set_length(e.timestamp(), false);
         */
    }
    return result;
}

/**
 *  Assumes the meta type meta::set_tempo has been found, and the length
 *  (3) also obtained.
 *
 *  Compare to trackdata::get_tempo().
 *
 * tracknumber
 *      Indicates the track number. If track 0, then some track
 *      parameters are set.
 *
 * \param trk
 *      Provides the midi::track into which to add the event.
 *
 * \param e
 *      Provides a prepped event [see extract_meta_msg()] that will
 *
 * \return
 *      Returns true if the tempo was non-zero and the event was able to be
 *      appended.
 */

bool
trackdata::extract_tempo (track & trk, event & e)
{
    midi::bytes bt;
    bt.push_back(get());                    /* tt   */
    bt.push_back(get());                    /* tt   */
    bt.push_back(get());                    /* tt   */

    double tempo_us = tempo_us_from_bytes(bt);
    bool result = tempo_us > 0;
    if (result)
    {
#if defined USE_THIS_CODE
        static bool gotfirst = false;
        if (tracknumber == 0)
        {
            midi::bpm bp = bpm_from_tempo_us(tempo_us);
            if (! gotfirst)
            {
                player & p = coordinator();
                p.beats_per_minute(bp);
                p.us_per_quarter_note(int(tempo_us));
                gotfirst = true;
            }
            if (! gotfirst)
                gotfirst = true;
        }
#endif
        bool result = e.append_meta_data(midi::meta::set_tempo, bt);
        if (result)
        {
            result = append_event(e);
            if (result)
            {
                tempoinfo & rti = trk.tempo_info();
                midi::bpm bp = bpm_from_tempo_us(tempo_us);
                if (track::is_legal(trk.track_number()))
                    rti.tempo_track(int(trk.track_number()));
                else
                    rti.tempo_track(0);

                rti.beats_per_minute(bp);       /* us_per_quarter_note()    */
                if (not_nullptr(trk.parent()))
                {
                    player & p = *trk.parent();
                    p.beats_per_minute(bp);
                    p.us_per_quarter_note(midi::microsec(tempo_us));
                }
            }
        }
    }
    return result;
}

/**
 *  Assumes meta type meta::time_signature has been found, and the length
 *  (4) also obtained.
 *
 *  Compare to trackdata::get_time_signature().
 *
 * tracknumber
 *      Indicates the track number. If track 0 then some track
 *      parameters are set.
 *
 * \virtual
 *
 * \param trk
 *      Provides the midi::track) into which to add the event.
 *
 * \return
 *      Returns true if the tempo was non-zero and the event was able to be
 *      appended.
 */

bool
trackdata::extract_time_signature (track & trk, event & e)
{
    int bpb = int(get());                   /* nn */
    int logbase2 = int(get());              /* dd */
    int cc = get();                         /* cc */
    int bb = get();                         /* bb */
    int bw = beat_power_of_2(logbase2);

    midi::bytes bt;
    bt.push_back(midi::byte(bpb));
    bt.push_back(midi::byte(logbase2));
    bt.push_back(midi::byte(cc));
    bt.push_back(midi::byte(bb));

    bool result = e.append_meta_data(midi::meta::time_signature, bt);
    if (result)
    {
        result = append_event(e);
        if (result)
        {
            timesiginfo tsi(bpb, bw, cc, bb);
            trk.set_timesig_info(tsi);          /* init beats, beat width   */

            /*
             * TODO, set in player, and check out usage of track info.
             * trk.clocks_per_metronome(cc);
             * trk.thirtyseconds_per_qn(bb);
             *
             * Should use c_perf_bp_mes and c_perf_bw instead in Seq66.
             */

            int tracknumber = trk.track_number();
            if (tracknumber == 0)
            {
                if (not_nullptr(trk.parent()))
                {
                    player & p = *trk.parent();
                    p.beats_per_bar(bpb);
                    p.beat_width(bw);
                    // p.clocks_per_metronome(cc);          // TODO
                    p.set_32nds_per_quarter(bb);
                }
            }
        }
    }
    return result;
}

/**
 *  Assumes the meta type meta::key_signature has been found, and the length
 *  (2) also obtained.
 *
 *  Compare to trackdata::get_key_signature().
 *
 * tracknumber
 *      Indicates the track number. If track 0, then some track
 *      parameters are set.
 *
 * \param trk
 *      Provides the midi::track into which to add the event.
 *
 * \param e
 *      Provides a prepped event [see extract_meta_msg()] that will
 *
 * \return
 *      Returns true if the tempo was non-zero and the event was able to be
 *      appended.
 */

bool
trackdata::extract_key_signature (track & trk, event & e)
{
    midi::bytes bt;
    bt.push_back(get());                        /* #/b no.                  */
    bt.push_back(get());                        /* min/maj                  */

    bool result = e.append_meta_data(midi::meta::key_signature, bt);
    if (result)
    {
        result = append_event(e);
        if (result)
        {
            int scale = int(bt[0]);
            bool ismajor = bt[1] == 0;
            keysiginfo ksi(scale, ismajor);
            trk.set_keysig_info(ksi);
        }
    }
    return result;
}

/**
 *  Internal function to check for and report a bad length value.
 *  A length of zero is now considered legal, but a "warning" message is shown.
 *  The largest value allowed within a MIDI file is 0x0FFFFFFF. This limit is
 *  set to allow variable-length quantities to be manipulated as 32-bit
 *  integers.
 *
 * \param len
 *      The length value to be checked, usually greater than 0.
 *      However, we have seen files with zero-length events, such as Lyric
 *      events (0x05) and the end-of-track event.
 *
 * \param type
 *      The type of meta event.  Used for displaying an error.
 *
 * \return
 *      Returns true if the length parameter is valid.  This now means it is
 *      simply less than 0x0FFFFFFF.
 */

bool
trackdata::checklen (midi::ulong len, midi::byte type)
{
    bool result = len <= c_varlength_max;                   /* 0x0FFFFFFF */
    if (! result)
    {
        printf("bad data length for meta type 0x%02X \n", type);
#if 0
        char m[40];
        snprintf(m, sizeof m, "bad data length for meta type 0x%02X", type);
        (void) set_error_dump(m);
#endif
    }
    return result;
}

/**
 *  Reads and analyzes a meta event to get the data.  MIDI events are appended
 *  to the events() object for later copying into the track itself.
 *
 *  Some track and global variables are copied directly into the track.
 *  We still have to figure out the best way to get them into the player
 *  (coordinator).
 *
 * Format of a MIDI meta message:
 *
 *      0xFF 0xmm len data
 *
 *      -   0xFF is the exact status byte value.
 *      -   0xmm is the meta type byte.
 *      -   len is the varinum length value of the data portion.
 *      -   data is the variable-length data for the meta event.
 *
 * \param trk
 *      A reference to the track holding this track data. Some meta
 *      values might be stored in it.
 *
 * \param e
 *      Provides the initial event values, already set, as follows:
 *
 *          -   e.set_timestamp(currenttime): this is the cumulative time;
 *              converted back to a delta time on writing.
 *          -   e.set_status_keep_channel(bstatus): should be 0xFF to signal
 *              a meta event.
 *
 *      To these will be added the meta type, length, and the meta event
 *      data values.
 *
 * \return
 *      Returns true if the end-of-track event has been found. Otherwise
 *      returns false.
 */

bool
trackdata::extract_meta_msg (track & trk, event & e)
{
    midi::byte mtype = get();                   /* meta message type byte   */
    midi::meta metatype = to_meta(mtype);       /* static_cast<> it         */
    midi::ulong len = get_varinum();            /* get data length          */
    bool it_checks = checklen(len, mtype);      /* mostly a sanity check    */
    if (! it_checks)
        return false;

    bool result = false;
    switch (metatype)
    {
    case midi::meta::seq_number:                /* FF 00 02 ss ss; FF 00 00 */

        (void) extract_track_number(trk, e, size_t(len));
        break;

    case midi::meta::track_name:                /* FF 03 len text           */

        (void) extract_track_name(trk, e, len);
        break;

    case midi::meta::end_of_track:              /* FF 2F 00                 */

        if (extract_end_of_track(trk, e))       /* trk.zero_markers()?      */
            result = true;
        break;

    case midi::meta::set_tempo:                 /* FF 51 03 tt tt tt        */

        if (len == 3)
            (void) extract_tempo(trk, e);
        else
            skip(len);                          /* eat the data             */
        break;

    case midi::meta::time_signature:            /* FF 58 04 nn dd cc bb     */

        if (len == 4)                           /* && ! timesig_set) ?      */
            (void) extract_time_signature(trk, e);
        else
            skip(len);                          /* eat the data             */
        break;

    case midi::meta::key_signature:             /* FF 59 02 ss kk           */

        if (len == 2)
            (void) extract_key_signature(trk, e);
        else
            skip(len);                          /* eat the data             */
        break;

    case midi::meta::seq_spec:                  /* FF 7F = SeqSpec          */

        /*
         * In a Seq66v2 sequence, will override with Seq66's SeqSpec code.
         * Here, we just grab it without any other processing. This function
         * is virtual.
         */

        (void) extract_seq_spec(trk, e, len);
        break;

    /*
     * Handled above: midi::meta::track_name
     */

    case midi::meta::text_event:                /* FF 01 len text           */
    case midi::meta::copyright:                 /* FF 02 len text           */
    case midi::meta::instrument:                /* FF 04 len text           */
    case midi::meta::lyric:                     /* FF 05 len text           */
    case midi::meta::marker:                    /* FF 06 len text           */
    case midi::meta::cue_point:                 /* FF 07 len text           */
    case midi::meta::program_name:              /* FF 08 len text           */
    case midi::meta::port_name:                 /* FF 09 len text           */

        (void) extract_text_event(trk, e, metatype, len);
        break;

    case midi::meta::midi_channel:              /* FF 20 01 cc, deprecated  */
    case midi::meta::midi_port:                 /* FF 21 01 pp, deprecated  */
    case midi::meta::smpte_offset:              /* FF 54 03 tt tt tt        */

        /*
         * The first two events are obsolete; SMPTE is not supported. But we
         * append them anyway to preserve, somewhat, the original file.
         */

        (void) extract_generic_meta(trk, e, metatype, len);
        break;

    default:

        /*
         * if (rc().verbose())
         */

        std::string m = "Illegal meta value skipped";
        (void) m_data.set_error_dump(m, m_data.real_position());
        break;
    }
    return result;                              /* if true, we are done     */
}

/*-------------------------------------------------------------------------
 * "put" functions
 *-------------------------------------------------------------------------*/

/**
 *  Combines a number of put() calls.  It puts the preamble for a MIDI Meta
 *  event. After this function is called, the caller then puts() the actual
 *  data.
 */

void
trackdata::put_meta_header
(
    midi::meta metaevent,
    int datalen,
    midi::pulse deltatime
)
{
    midi::byte metamarker = midi::to_byte(midi::status::meta_msg);
    midi::byte metacode = midi::to_byte(metaevent);
    put_varinum(midi::ulong(deltatime));
    put(metamarker);                                /* 0xFF meta marker     */
    put(metacode);                                  /* which meta event     */
    put_varinum(midi::ulong(datalen));
}

/**
 *  Currently not used.
 */

void
trackdata::put_meta
(
    midi::meta metaevent,
    const midi::bytes & data,
    midi::pulse deltatime
)
{
    put_meta_header(metaevent, int(data.size()), deltatime);
    for (auto b : data)
        put(b);
}

/**
 *  Writes a generic Sequencer-specific item. The format is:
 *
 *      0xFF 0x7f len  mfid ...
 *       ^    ^    ^    ^    ^
 *       |    |    |    |    |
 *       |    |    |    |     ---- Data bytes
 *       |    |    |     --------- Manufacturers ID (optional)
 *       |    |     -------------- Length of ID plus data bytes
 *       |     ------------------- Sequencer-specific
 *        ------------------------ Meta message
 *
 *  Particular sequencers may use this variation: the first byte(s) of data
 *  is a manufacturer ID (these are one byte, or, if the first byte is 00,
 *  three bytes). As with MIDI System Exclusive, manufacturers who define
 *  something using this meta-event should publish it so that others may know
 *  how to use it.  This type of event may be used by a sequencer which
 *  elects to use this as its only file format; sequencers with their
 *  established feature-specific formats should probably stick to the
 *  standard features when using this format.
 *
 *  \param spec
 *      Provides the specific 
 */

void
trackdata::put_seqspec (midi::ulong spec, int datalen)
{
    datalen += sizeof spec;                         /* manufacturer ID      */
    put_meta_header(midi::meta::seq_spec, datalen, 0);
    put_long(spec);                                 /* e.g. c_midibus       */
}

/**
 *  Writes a Seq66 Sequencer-specific item. The format is:
 *
 *      0xFF 0x7f len 0x24 0x24 0x00 feature data
 *       ^    ^    ^   ^    ^    ^    ^
 *       |    |    |   |    |    |    |
 *       |    |    |   |    |    |     ----- Seq66 feature selector code byte
 *       |    |    |   |    |     ---------- Zero for padding
 *       |    |    |   |     --------------- "ID" continued
 *       |    |    |    -------------------- Manufacturers "ID" (Hohner :-)
 *       |    |     ------------------------ Length of "ID" plus ... bytes
 *       |     ----------------------------- Sequencer-specific
 *        ---------------------------------- Meta message
 *
 *  Note that the "ID" and feature are packed into an unsigned long value:
 *  0x242400nn where nn is the feature byte.
 *
 *  Note that the delta-time is always 0.
 *
 * \param spec
 *      Provides a 4-byte code that indicates the special feature being
 *      written.
 *
 * \param datalen
 *      The number of bytes that will (must!) be written after this call.
 */

void
trackdata::put_seqspec_code (midi::ulong spec, int datalen)
{
    datalen += sizeof spec;                         /* we hope it is 4 :-D  */
    put_meta_header(midi::meta::seq_spec, datalen); /* set it up for data   */
    put_long(spec);                                 /* e.g. c_midibus       */
}

/**
 *  Similar to put_seqspec_code(), but also writes the data in the same
 *  call.
 *
 *  Note that the delta-time is always 0.
 */

void
trackdata::put_seqspec_data (midi::ulong spec, const midi::bytes & data)
{
    put_seqspec_code(spec, int(data.size()));
    for (auto b : data)
        put(b);
}

/**
 *  Calculates the size of a proprietary item, as written by the
 *  write_seqspec_header() function, plus whatever is called to write the
 *  data.  If using the new format, the length includes the sum of
 *  sequencer-specific tag (0xFF 0x7F) and the size of the variable-length
 *  value.  Then, for the new format, 4 bytes are added for the Seq24 MIDI
 *  control value, and then the data length is added.
 *
 * \param data_length
 *      Provides the data length value to be encoded.
 *
 * \return
 *      Returns the length of the item size, including the delta time, meta
 *      bytes, length byes, the control tag, and the data-length itself.
 */

long
trackdata::seqspec_item_size (long data_length) const
{
    long result = 0;
    int len = data_length + 4;              /* data + sizeof(control_tag);  */
    result += 3;                            /* count delta time, meta bytes */
    result += varinum_size(len);            /* count the length bytes       */
    result += 4;                            /* write_long(control_tag);     */
    result += data_length;                  /* add the data size itself     */
    return result;
}

/**
 *  Adds an event to the container.  It handles regular MIDI events, but not
 *  "extended" (our term) MIDI events (SysEx and Meta events).
 *
 *  For normal MIDI events, if the sequence's MIDI channel is null_channel()
 *  == 0x80, then it is the copy of an SMF 0 sequence that the midi_splitter
 *  created.  We want to be able to save it along with the other tracks, but
 *  won't be able to read it back if all the channels are bad.  So we just use
 *  the channel from the event.
 *
 *  SysEx and Meta events are detected and passed to the new put_ex_event()
 *  function for proper putting.
 *
 * \param e
 *      Provides the event to be added to the container. It will already have
 *      been tweaked, if necessary, in put_track().
 *
 * \param deltatime
 *      Provides the time-location of the event.
 */

void
trackdata::put_channel_event (const event & e, midi::pulse deltatime)
{
    midi::byte d0 = e.data(0);
    midi::byte d1 = e.data(1);
    midi::byte st = e.status();
    put_varinum(midi::ulong(deltatime));        /* encode delta_time    */
    put(st);                                    /* add (fixed) status   */
    if (e.has_channel())
    {
        midi::status s = midi::to_status(mask_status(st));
        switch (s)
        {
        case midi::status::note_off:                            /* 0x80 */
        case midi::status::note_on:                             /* 0x90 */
        case midi::status::aftertouch:                          /* 0xA0 */
        case midi::status::control_change:                      /* 0xB0 */
        case midi::status::pitch_wheel:                         /* 0xE0 */

            put(d0);
            put(d1);
            break;

        case midi::status::program_change:                      /* 0xC0 */
        case midi::status::channel_pressure:                    /* 0xD0 */

            put(d0);
            break;

        default:

            break;
        }
    }
}

/**
 *  Adds the bytes of a SysEx or Meta MIDI event. This function was using
 *  put() to write a single count byte, but put`varinum() must be used for
 *  this value.
 *
 *  -   Meta:                   delta FF type len bytes
 *  -   SysEx:
 *      -   Single:             delta F0 len bytes F7
 *      -   Continuation:       delta F0 len bytes ; delta F7 len bytes ; ...
 *                              delta F7 len bytes F7
 *      -   Escape sequence:    TODO
 *
 *  The SysEx continuation messages are stored as separate events. The only
 *  difference in writing them is the status byte.
 *
 * \param e
 *      Provides the MIDI event to add.  The caller must ensure that this is
 *      either SysEx or Meta event, using the event::is_ex_data() function.
 *
 * \param deltatime
 *      Provides the time of the event, which is encoded into the event.
 */

void
trackdata::put_ex_event (const event & e, midi::pulse deltatime)
{
    put_varinum(midi::ulong(deltatime));        /* encode delta_time        */
    if (e.is_sysex())
    {
        size_t count = e.sysex_size();          /* includes F0 ... F7       */
        put_varinum(midi::ulong(count));
        for (size_t i = 0; i < count; ++i)
            put(e.get_message(i));
    }
    else if (e.is_meta())
    {
        size_t count = e.meta_data_size();      /* includes FF nn len....   */
        for (size_t i = 0; i < count; ++i)
            put(e.get_message(i));
    }
}

/**
 *  Fills in the track number.  Writes 0xFF 0x00 0x02 ss ss, where ss ss is
 *  the variable-length value for the sequence number.  This function is used
 *  in the new midi::file::write_song() function, which should be ready to go
 *  by the time you're reading this.  Compare this function to the beginning
 *  of trackdata::fill().
 *
 *  Now, for sequence 0, an alternate format is "FF 00 00".  But that format
 *  can only occur in the first track, and the rest of the tracks then don't
 *  need a sequence number, since it is assumed to increment.  Our application
 *  doesn't bother with that shortcut.
 *
 * \warning
 *      This is an optional event, which must occur only at the start of a
 *      track, before any non-zero delta-time.  For Format 2 MIDI files, this
 *      is used to identify each track. If omitted, the sequences are numbered
 *      sequentially in the order the tracks appear.  For Format 1 files, this
 *      event should occur on the first track only.  So, are we writing a
 *      hybrid format?
 *
 * \param seq
 *      The sequence/track number to write.
 */

void
trackdata::put_track_number (int trkno)
{
    put_meta_header(midi::meta::seq_number, 2);     /* 0x00, 2 bytes long   */
    put_short(midi::ushort(trkno));
}

/**
 *  Fills in the sequence name.  Writes 0xFF 0x03, and then the track name.
 *  This function is used in the new midi::file::write_song() function, which
 *  should be ready to go by the time you're reading this.
 *  Compare this function to trackdata::put_meta_text().
 *
 *  Could call put_meta_text(midi::meta::trackname, name);
 *
 * \param name
 *      The sequence/track name to set.  We could get this item from
 *      seq(), but the parameter allows the flexibility to change the
 *      name.
 */

void
trackdata::put_track_name (const std::string & name)
{
    size_t len = name.length();
    put_meta_header(midi::meta::track_name, len);   /* 0x03, len bytes long */
    for (size_t i = 0; i < len; ++i)
        put(midi::byte(name[i]));
}

/**
 *  Fill in the time-signature information.  This function is used only for
 *  the first track, and only if no such event is in the track data.
 *
 *  We now make sure that the proper values are part of the eventlist for
 *  usage in this particular track.  For export, we cannot guarantee that the
 *  first (0th) track/sequence is exportable.
 *
 * \param p
 *      Provides the performance object from which we get some global MIDI
 *      parameters.
 */

void
trackdata::put_time_sig
(
    int bpb, int beatwidth,
    int cpm, int get32pq
)
{
    int bw = log2_power_of_2(beatwidth);
    put_meta_header(midi::meta::time_signature, 4); /* 0x58 marker, 4 bytes */
    put(byte(bpb));
    put(byte(bw));
    put(byte(cpm));
    put(byte(get32pq));
}

void
trackdata::put_time_sig (const midi::timesiginfo & tsi)
{
    put_time_sig
    (
        tsi.beats_per_bar(), tsi.beat_width(),
        tsi.clocks_per_metronome(), tsi.thirtyseconds_per_qn()
    );
}

/**
 *  Fill in the key-signature information.
 */

void
trackdata::put_key_sig (int sf, bool mf)
{
    put_meta_header(midi::meta::key_signature, 2);  /* 0x59 marker, 2 bytes */
    put(byte(sf));
    put(byte(mf));
}

void
trackdata::put_key_sig (const midi::keysiginfo & ksi)
{
    put_key_sig(ksi.sharp_flat_count(), ksi.is_minor_scale());
}

/**
 *  Fill in the tempo information.  This function is used only for the first
 *  track, and only if no such event is int the track data.
 *
 *  We now make sure that the proper values are part of the eventlist
 *  object for usage in this particular track.  For export, we cannot guarantee
 *  that the first (0th) track/sequence is exportable.
 *
 * \change ca 2017-08-15
 *      Fixed issue #103, was writing tempo bytes in the wrong order here.
 *      Accidentally committed along with fruity changes, sigh, so go back a
 *      couple of commits to see the changes.
 */

void
trackdata::put_tempo (int usperqn)
{
    midi::bytes tt;                                /* hold tempo bytes     */
    tempo_us_to_bytes(tt, usperqn);
    put_meta_header(midi::meta::set_tempo, 3);      /* 0x51, 3 bytes long   */
    put(tt[0]);                                     /* NOT 2, 1, 0!         */
    put(tt[1]);
    put(tt[2]);
}

/**
 *  This function fills the given track's midi::bytes vector with MIDI data
 *  from the current track's midi::eventlist vector, preparatory to writing it
 *  to a file.  Note that some of the events might not come out in the same
 *  order they were stored in (we see that with program-change events).
 *  This function replaces sequence::put_list().
 *
 *  Now, for track 0, an alternate format for writing the track number chunk
 *  is "FF 00 00".  But that format can only occur in the first track, and the
 *  rest of the tracks then don't need a track number, since it is assumed to
 *  increment.  This application doesn't use that shortcut.
 *
 *  We have noticed differences in saving files in sets=4x8 versus sets=8x8,
 *  and pre-sorting the event list gets rid of some of the differences, except
 *  for the last, multi-line SeqSpec.  Some event-reordering still seems to
 *  occur, though.
 *
 * Triggers (Seq66):
 *
 *      Triggers are added by first calling put_varinum(0), which is needed
 *      because why?
 *
 *      Then 0xFF 0x7F is written, followed by the length value, which is the
 *      number of triggers at 3 long integers per trigger, plus the 4-byte
 *      code for triggers, c_triggers_ex = 0x24240008.
 *
 *      However, we're now extending triggers (c_trig_transpose = 0x24240020)
 *      to include a transposition byte which allows up to 5 octaves of
 *      tranposition either way, as a way to re-use patterns.  Inspired by
 *      Kraftwerk's "Europe Endless" background track, with patterns being
 *      shifted up and down in pitch.
 *
 * Meta and SysEx Events:
 *
 *      These events can now be detected and added to the list of bytes to
 *      dump.  However, historically Seq24 has forced Time Signature and Set
 *      Tempo events to be written to the container, and has ignored these
 *      events (after the first occurrence).  So we need to figure out what to
 *      do here yet; we need to distinguish between forcing these events and
 *      them being part of the edit.
 *
 *      To allow other track to read Seq24/Seq66 files, we should
 *      provide the Time Signature and Tempo meta events, in the 0th (first)
 *      track (sequence).  These events must precede any "real" MIDI events.
 *      We also need to skip this if tempo track support is in force.
 *
 * \threadunsafe
 *      The track object bound to this container needs to provide the
 *      locking mechanism when calling this function.
 *
 * \param trk
 *      Provides the track itself, so we can get information about the track,
 *      such as track number, re 0.  This number is masked into the track
 *      information. Also important is track channel information.
 *
 * \param tempotrack
 *      The number of the tempo track. Defaults to 0.
 *
 * \param doseqspec
 *      If true (the default), writes out the SeqSpec information.  If false,
 *      we want to write out a regular MIDI track without this information; it
 *      writes a smaller file. Defaults to true.
 *
 * \return
 *      Returns true if the track data could be written. It fails only
 *      if there is a mixup in delta times.
 */

bool
trackdata::put_track (/*const*/ track & trk, int tempotrack, bool doseqspec)
{
    bool result = true;
    int trkno = trk.track_number();
    eventlist evl = events();
    evl.sort();                             /* hmmmm                        */
    clear_buffer();                         /* must reconstruct raw bytes   */
    put_track_number(trkno);                /* optional, but add it anyway  */
    put_track_name(trk.track_name());
    if (trkno == tempotrack)                /* see notes about Meta events  */
    {
        if (evl.has_time_signature())
            put_time_sig(trk.info().timesig_info());

        if (evl.has_tempo())
            put_tempo(trk.info().tempo_info().us_per_quarter_note());
    }

    midi::pulse timestamp = 0;
    midi::pulse deltatime = 0;
    midi::pulse prevtimestamp = 0;
    for (auto & e : evl)
    {
        timestamp = e.timestamp();
        deltatime = timestamp - prevtimestamp;
        if (deltatime < 0)                          /* midi::pulse == long  */
        {
            errprint("put_track(): bad delta-time, aborting");
            result = false;
            break;
        }
        prevtimestamp = timestamp;
        if (e.has_channel())                        /* fix event before put */
        {
            if (! trk.free_channel())           // || is_null_channel(channel))
            {
                midi::byte channel = trk.track_midi_channel();
                midi::byte st = midi::mask_status(e.status());
                st = st | channel;                  /* channel from track   */
                e.set_status(st);
            }
            put_channel_event(e, deltatime);
        }
        else if (e.is_ex_data())                    /* meta or sysex        */
        {
            if (doseqspec || ! e.is_seq_spec())
                put_ex_event(e, deltatime);
        }
    }

    /*
     * Last, but certainly not least, write the non-optional end-of-track
     * meta-event. If the nominal length of the track is less than the
     * last timestamp, we set the delta-time to 0.  Better would be to
     * make sure this can never happen.
     */

    midi::pulse len = events().length();
    if (len < prevtimestamp)
        deltatime = 0;
    else
        deltatime = len - prevtimestamp;     /* meta track end   */

    /*
     * Should not be needed.
     *
     *      put_meta_track_end(deltatime);
     */

    return result;
}

/**
 *  Puts out only the events that were read in or created. No modifications
 *  due to any track parameters, including track number.
 *
 *      int trkno = trk.track_number();
 *
 *  For now, we don't sort the events. They come out in the order they
 *  were read in.
 */

bool
trackdata::put_track_events (/*const*/ track & /*trk*/)
{
    bool result = true;
    midi::pulse timestamp = 0;
    midi::pulse deltatime = 0;
    midi::pulse prevtimestamp = 0;
    eventlist & evl = events();             /* access to midi::eventlist    */
    clear_buffer();                         /* must reconstruct raw bytes   */
#if defined PLATFORM_DEBUG_TMI
        std::string label = "Putting track ";
        label += std::to_string(trk.track_number());
        label += ": ";
        label += trk.track_name();
        printf("%s; %d events\n", label.c_str(), evl.count());
#endif
    for (const auto & e : evl)
    {
#if defined PLATFORM_DEBUG_TMI
        std::string label = "Track ";
        label += std::to_string(trk.track_number());
        label += ": ";
        label += trk.track_name();
        e.print(label);
#endif
        timestamp = e.timestamp();
        deltatime = timestamp - prevtimestamp;
        if (deltatime < 0)                          /* midi::pulse == long  */
        {
            errprint("put_track(): bad delta-time, aborting");
            result = false;
            break;
        }
        prevtimestamp = timestamp;
        if (e.has_channel())
            put_channel_event(e, deltatime);
        else if (e.is_ex_data())                    /* meta or sysex        */
            put_ex_event(e, deltatime);
    }
    return result;
}

/*-------------------------------------------------------------------------
 * "parse" functions
 *-------------------------------------------------------------------------*/

/**
 *  This function opens a binary MIDI file and parses it into sequences
 *  and other application objects.
 *
 *  Standard MIDI provides for port and channel specification meta events, but
 *  they are apparently considered obsolete:
 *
\verbatim
    Obsolete meta-event:                Replacement:
    MIDI port (buss):   FF 21 01 po     Device (port) name: FF 09 len text
    MIDI channel:       FF 20 01 ch
\endverbatim
 *
 *  What do other applications use for specifying port/channel?
 *
 * SysEx notes:
 *
 *      Some files (e.g. Dixie04.mid) do not always encode System Exclusive
 *      messages properly for a MIDI file.  Instead of a varinum length value,
 *      they are followed by extended IDs (0x7D, 0x7E, or 0x7F).
 *
 *      We've covered some of those cases by disabling access to m_data if the
 *      position passes the size of the file, but we want try to bypass these
 *      odd cases properly.  So we look ahead for one of these special values.
 *
 *      This MIDI file object handles SysEx message only to the
 *      extend of ....passing them via MIDI Thru.
 *
 *  This function parses an SMF 1 binary MIDI file; it is basically the
 *  original seq66 file::parse() function.  It assumes the file-data has
 *  already been read into memory.  It also assumes that the ID, track-length,
 *  and format have already been read.
 *
 *  If the MIDI file contains both proprietary (c_timesig) and MIDI type 0x58
 *  then it came from seq42 or seq32 (Stazed versions).  In this case the MIDI
 *  type is parsed first (because it is listed first) then it gets overwritten
 *  by the proprietary, above.
 *
 *  Note that track_count doesn't count the Seq24 "proprietary" footer
 *  section, even if it uses the new format, so that section will still be
 *  read properly after all normal tracks have been processed.
 *
 * PPQN:
 *
 *      Current time (runningtime) is re the ppqn according to the file, we
 *      have to adjust it to our own ppqn.  PPQN / ppqn gives us the ratio.
 *      (This change is not enough; a song with a ppqn of 120 plays too fast
 *      in Seq24, which has a constant ppqn of 192.  Triggers must also be
 *      modified.)
 *
 * Tempo events:
 *
 *      If valid, set the global tempo to the first encountered tempo; this is
 *      legacy behavior.  Bad tempos occur and stick around, munging exported
 *      songs.  We log only the first tempo officially; the rest are stored as
 *      events if in the first track.  We also adjust the upper draw-tempo
 *      range value to about twice this value, to give some headroom... it
 *      will not be saved unless the --user-save option is in force.
 *
 * Channel:
 *
 *      We are transitioning away from preserving the channel in the status
 *      byte, which will require using the event::m_channel value.
 *
 * Time Signature:
 *
 *      Like Tempo, Time signature is now handled more robustly.
 *
 * Key Signature and other Meta events:
 *
 *      Although we don't support these events, we do want to keep them, so we
 *      can output them upon saving.  Instead of bypassing unhandled Meta
 *      events, we now store them, so that they are not lost when
 *      exporting/saving the MIDI data.
 *
 * Track name:
 *
 *      This event is optional. It's interpretation depends on its context. If
 *      it occurs in the first track of a format 0 or 1 MIDI file, then it
 *      gives the pattern Name. Otherwise it gives the Track Name.
 *
 * End of Track:
 *
 *      "If delta is 0, then another event happened at the same time as
 *      track-end.  Class track discards the last note.  This fixes that.
 *      A native Seq24 file will always have a delta >= 1." Not true!  We've
 *      fixed the real issue by commenting the code that increments current
 *      time.  Question:  What if BPM is set *after* this event?
 *
 * Tracks:
 *
 *      If the track is shorter than a quarter note, assume it needs to be
 *      padded to a measure.  This happens anyway if the short pattern is
 *      opened in the sequence editor (seqedit).
 *
 *      Add sorting after reading all the events for the sequence.  Then add
 *      the sequence with it's preferred location as a hint.
 *
 * Unknown chunks:
 *
 *      Let's say we don't know what kind of chunk it is.  It's not a MTrk, we
 *      don't know how to deal with it, so we just eat it.  If this happened
 *      on the first track, it is a fatal error.
 *
 * Not handled in this base class:
 *
 *      -   Seq66 SeqSpec values.
 *      -   Buss override
 *      -   PPQN ratio conversion
 *
 *  This function grabs the bytes for this track and then parses them.
 *
 * \param trk
 *      A reference to the track to be filled with the parsed MIDI data
 *      or settings extracted from the MIDI data.
 *
 * \param data
 *      The vector of bytes, normally provided by a MIDI file object.
 *      This is normally all of the data in the MIDI file. However,
 *      this function processes only the current track's data.
 *
 * \param offset
 *      The offset of the start of the track data in the data buffer of the
 *      midi::file object that called this function.
 *
 * \param trklength
 *      The length of the track data.
 *
 * \return
 *      Returns the offset into the next track if successful. Otherwise,
 *      0 is returned.
 */

size_t
trackdata::parse_track
(
    track & trk,
    const util::bytevector & data,
    size_t offset, size_t trklength
)
{
    size_t result = offset + trklength;     /* presumed next track offset   */
    midi::pulse runningtime = 0;            /* reset timestamp accumulator  */
    midi::pulse currenttime = 0;            /* adjust by PPQN?              */
    midi::ushort trkno = c_ushort_max;      /* see midibytes.hpp            */
    midi::byte runningstatus = 0;
    midi::byte tentative_channel = null_channel();
    midi::byte last_runningstatus = 0;      /* EXPERIMENTAL                 */
    bool skip_to_end = false;               /* EXPERIMENTAL                 */
    bool finished = false;
    clear_all();                            /* clear events and raw bytes   */
    m_data.assign(data.byte_list(), offset, trklength);
    while (! finished)                      /* get the events in the track  */
    {
        if (done())                         /* safety check                 */
        {
#if defined PLATFORM_DEBUG
            printf("Length reached for track %d\n", trk.track_number());
            printf
            (
                "Track %d has %d bytes\n",
                trk.track_number(), int(m_data.size())
            );
#endif
            break;
        }

        event e;                            /* note-off, no channel default */
        midi::ulong len;                    /* important counter!           */
        midi::byte d0, d1;                  /* the two MIDI data bytes      */
        midi::pulse delta = get_varinum();  /* time delta from previous ev  */
        midi::byte bstatus = peek();        /* check the current event byte */
        if (fatal_error())
            break;

        if (midi::is_status_msg(bstatus))   /* is there a 0x80 bit?         */
        {
            /*
             * For SysEx, the skip is undone. For meta events, the
             * correct event type is obtained anyway.
             */

            skip(1);                                    /* to get to d0     */
            if (midi::is_system_common_msg(bstatus))    /* eventcodes.hpp   */
            {
                runningstatus = 0;                      /* clear status     */
            }
            else if (! midi::is_realtime_msg(bstatus))  /* i.e. < 0xF8      */
            {
                runningstatus = bstatus;                /* log status       */
                if (m_running_status_action == rsaction::recover)
                    last_runningstatus = bstatus;       /* EXPERIMENTAL     */
            }
        }
        else
        {
            /*
             * Handle data values. If in running status, set that as status;
             * the next value to be read is the d0 value.  If not running
             * status, is this an error?
             */

            if (skip_to_end)                            /* EXPERIMENTAL     */
                continue;

            if (runningstatus > 0)                      /* in force?        */
            {
                bstatus = runningstatus;                /* yes, use it      */
            }
            else if (last_runningstatus > 0)
            {
                runningstatus = last_runningstatus;
                bstatus = runningstatus;
            }
#if 0
            else                                        /* EXPERIMENTAL     */
            {
                finished = true;
                continue;
            }
#endif
        }
        runningtime += delta;                           /* add the time     */
        currenttime = runningtime;

#if defined USE_TIMESTAMP_PPQN_SCALING
        if (scaled())                   /* adjust time via ppqn     */
            currenttime = midipulse(currenttime * ppqn_ratio());
#endif
        e.set_timestamp(currenttime);
        e.set_status_keep_channel(bstatus);

        midi::byte eventcode = midi::mask_status(bstatus);   /* F0 mask     */
        midi::byte channel = midi::mask_channel(bstatus);    /* 0F mask     */
        midi::status eventstat = to_status(eventcode);
        switch (eventstat)
        {
        case midi::status::erroneous:                   /* 0 byte           */

            continue;
            break;

        case midi::status::note_off:                    /* 3-byte events    */
        case midi::status::note_on:
        case midi::status::aftertouch:
        case midi::status::control_change:
        case midi::status::pitch_wheel:

            d0 = get();                                 /* gets a byte      */
            d1 = get();
            if (midi::is_note_off_velocity(eventcode, d1))
                e.set_channel_status(eventcode, channel);

            e.set_data(d0, d1);                         /* set data in ev   */

            /*
             * trackdata::append_event() doesn't sort events; sort after we
             * get them all.  Also, it is kind of weird we change the
             * channel for the whole sequence here.
             */

            if (append_event(e))                        /* does not sort    */
                tentative_channel = channel;            /* log MIDI channel */
            break;

        case midi::status::program_change:              /* 1-byte events    */
        case midi::status::channel_pressure:

            d0 = get();
            e.set_data(d0);                             /* set data in ev   */
            if (append_event(e))                        /* does not sort    */
            {
                tentative_channel = channel;            /* log MIDI channel */
                // if (is_smf0)
                //      m_smf0_splitter.increment(channel); /* count chan.  */
            }
            break;

        case midi::status::real_time:                   /* 0xFn MIDI events */

            if (midi::is_meta_msg(bstatus))             /* 0xFF             */
            {
                finished = extract_meta_msg(trk, e);    /* get meta+data    */
            }
            else if (midi::is_sysex_msg(bstatus))       /* 0xF0 len sysex   */
            {
                /*
                 * Some files do not properly encode SysEx messages;
                 * see the function banner for notes.
                 */

                midi::byte check = get();
                if (is_sysex_special_id(check))
                {
                    /*
                     * TMI: "SysEx ID byte = 7D to 7F");
                     */
                }
                else                                    /* handle normally  */
                {
                    m_data.decrement();                 /* trackdata func   */
                    len = get_varinum();
                    while (len--)
                    {
                        midi::byte b = get();
                        if (! e.append_sysex(b))        /* SysEx end byte?  */
                            break;
                    }
                    skip(len);                          /* eat it, just eat */
                }
            }
            else if (midi::is_sysex_end_msg(bstatus))   /* ... 0xF7         */
            {
                    (void) e.append_sysex(bstatus);     /* TODO             */
            }
            else
            {
                errprint("Unexpected meta code");
#if 0
                // This is in midi::file
                (void) set_error_dump
                (
                    "Unexpected meta code", midi::ulong(bstatus)
                );
#endif
            }
            break;

        default:

            /*
             * Some files (e.g. 2rock.mid, which has "00 24 40"
             * hanging out there all alone at offset 0xba) have junk
             * in them.
             */

#if defined PLATFORM_DEBUG
            printf
            (
                "Unsupported MIDI event 0x%02x at 0x%04x\n",
                bstatus, unsigned(position() + offset)
            );
#endif

#if 0
                // This is in midi::file
            (void) set_error_dump
            (
                "Unsupported MIDI event", midi::ulong(bstatus)
            );
#endif
            return result;      /* allow further processing */
            break;
        }
    }                          /* while not done loading Trk chunk */

    /*
     * Tracks has been filled, add it to the performance or SMF 0
     * splitter.  If there was no sequence number embedded in the
     * track, use the for-loop track number.  It's not fool-proof.
     * "If the ID numbers are omitted, the sequences' locations in
     * order in the file are used as defaults."
     */

    if (trkno == midi::c_ushort_max)
        trkno = 0;                      // FIXME

    if (trkno < c_prop_seq_number)
    {
        trk.midi_channel(tentative_channel);

        /*
         * The rest is handled in the midi::file class.
         */
    }
    return result;
}

/*-------------------------------------------------------------------------
 * "write" functions
 *-------------------------------------------------------------------------*/

 // TO DO

#if defined RTL66_PROVIDE_EXTRA_GET_FUNCTIONS

/**
 *  A helper function to simplify reading midi_control data from the MIDI
 *  file. Same as seq66::midifile::read_byte_array(), which Seq66 uses in
 *  seq66::midifile::parse_c_midictrl(), which is legacy Seq24 code.
 *
 * \param b
 *      The byte array to receive the data.
 *
 * \param sz
 *      The number of bytes in the array, and to be read.
 *
 * \return
 *      Returns the number of bytes actually obtained. If an error occurs,
 *      then 0 is returned.
 */

size_t
trackdata::get_array (midi::byte * b, size_t sz)
{
    size_t result = 0;
    if (not_nullptr(b) && sz > 0)
    {
        for (size_t i = 0; i < sz; ++i)
        {
            if (position() < size())
            {
                result = i;
                *b++ = get();
            }
            else
            {
                result = 0;
                *b = 0;
                break;
            }
        }
    }
    return result;
}

/**
 *  A overload function to simplify reading midi_control data from the MIDI
 *  file.  It uses a bytes object instead of a buffer.
 *
 * \param b
 *      The midistring to receive the data.
 *
 * \param sz
 *      The number of bytes to be read.
 *
 * \return
 *      Returns the number of bytes "read".  The string \a b will be empty if
 *      0 is returned.
 */

size_t
trackdata::get_array (midi::bytes & b, size_t sz)
{
    size_t result = 0;
    b.clear();
    if (sz > b.capacity())
        b.reserve(sz);

    for (size_t i = 0; i < sz; ++i)
    {
        if (position() < size())
        {
            ++result;
            b.push_back(get());
        }
        else
        {
            result = 0;
            b.clear();
            break;
        }
    }
    return result;
}

/**
 *  A overloaded function to simplify reading midi_control data from the MIDI
 *  file. It uses a standard string object instead of a buffer. The
 *  seq66::midifile class defines read_string(), but does not use it.
 *
 * \param b
 *      The std::string to receive the data.
 *
 * \param sz
 *      The number of bytes to be read.
 *
 * \return
 *      Returns the number of bytes "read".  The string \a b will be empty if
 *      0 is returned.
 */

bool
trackdata::get_string (std::string & b, size_t sz)
{
    size_t result = 0;
    b.clear();
    if (sz > b.capacity())
        b.reserve(sz);

    for (size_t i = 0; i < sz; ++i)
    {
        if (position() < size())
        {
            result = i;
            b.push_back(char(get()));
        }
        else
        {
            result = 0;
            b.clear();
            break;
        }
    }
    return result;
}

std::string
trackdata::get_track_name ()
{
    std::string result;

    /*
     * Hmmm, should we get_varinum() instead? If so, fix seq66::midifile too
     */

    (void) get();                                   /* toss delta time      */
    midi::byte msg = get();                         /* get meta marker      */
    if (midi::is_meta_msg(msg))                                     /* 0xFF */
    {
        msg = get();
        if (midi::is_meta_msg(msg, midi::meta::track_name))         /* 0x03 */
        {
            midi::ulong tl = get_varinum();         /* length of the name   */
            for (midi::ulong i = 0; i < tl; ++i)
                result += get();
        }
    }
    return result;
}

std::string
trackdata::get_meta_text ()
{
    std::string result;

    /*
     * Again, get_varinum() should be used.
     */

    (void) get();                                   /* toss delta time      */
    midi::byte msg = get();                         /* get meta marker      */
    if (midi::is_meta_text_msg(msg))                                /* 0xFF */
    {
        msg = get();                                /* get meta msg type    */

        midi::ulong tl = get_varinum();         /* length of the text   */
        for (midi::ulong i = 0; i < tl; ++i)
            result += get();
    }
    return result;
}

/**
 *  Reads the sequence number.  Meant only for usage in the
 *  proprietary/SeqSpec footer track, in the new file format.
 *
 * \return
 *      Returns the sequence number found, or -1 if it was not found.
 */

int
trackdata::get_track_number ()
{
    int result = -1;
    (void) get();                                   /* toss delta time      */
    midi::byte msg = get();                         /* get seq-spec marker  */
    if (midi::is_meta_msg(msg))                                     /* 0xFF */
    {
        msg = get();
        if (midi::is_meta_msg(msg, midi::meta::seq_number))         /* 0x00 */
        {
            if (get() == 2)
                result = int(get_short());
        }
    }
    return result;
}

/**
 *  Compare to trackdata::extract_tempo().
 */

bool
trackdata::get_tempo (midi::tempoinfo & destination)
{
    bool result = false;
    (void) get();                                   /* toss delta time      */
    midi::byte msg = get();                         /* get seq-spec marker  */
    if (midi::is_meta_msg(msg))                                     /* 0xFF */
    {
        msg = get();
        if (midi::is_meta_msg(msg, midi::meta::set_tempo))          /* 0x51 */
        {
            if (get() == 3)
            {
                midi::bytes bt;
                bt.push_back(get());                /* tt   */
                bt.push_back(get());                /* tt   */
                bt.push_back(get());                /* tt   */

                double tt = tempo_us_from_bytes(bt);
                destination.us_per_quarter_note(tt);    /* (also sets BPM)  */
                result = true;
            }
        }
    }
    return result;
}

/**
 *  Compare to trackdata::extract_time_signature().
 *
 *      Hmmmm, redundant re extract_time_signature.
 */

bool
trackdata::get_time_signature (midi::timesiginfo & destination)
{
    bool result = false;
    (void) get();                                   /* toss delta time      */
    midi::byte msg = get();                         /* get seq-spec marker  */
    if (midi::is_meta_msg(msg))                                     /* 0xFF */
    {
        msg = get();
        if (midi::is_meta_msg(msg, midi::meta::time_signature))     /* 0x58 */
        {
            if (get() == 4)
            {
                int bpb = int(get());
                int bw = int(get());
                int cpm = int(get());
                int n32perqn = int(get());
                result = true;

                // TODO GET THE 4 values; and add string to the timesiginof

                destination.beats_per_bar(bpb);
                destination.beat_width(bw);
                destination.clocks_per_metronome(cpm);
                destination.thirtyseconds_per_qn(n32perqn);
            }
        }
    }
    return result;
}

/**
 *  Compare to trackdata::extract_key_signature().
 */

bool
trackdata::get_key_signature (midi::keysiginfo & destination)
{
    bool result = false;
    (void) get();                                   /* toss delta time      */
    midi::byte msg = get();                         /* get seq-spec marker  */
    if (midi::is_meta_msg(msg))                                     /* 0xFF */
    {
        msg = get();
        if (midi::is_meta_msg(msg, midi::meta::key_signature))      /* 0x59 */
        {
            if (get() == 2)
            {
                int sf = midi::byte_to_int(get());
                bool mf = get() != 0;
                destination.sharp_flat_count(sf);
                destination.is_minor_scale(mf);
                result = true;
            }
        }
    }
    return result;
}

/**
 *  A helper function for arbitrary, otherwise unhandled meta data.
 */

bool
trackdata::get_meta (midi::event & e, midi::meta metatype, size_t sz)
{
    bool result = checklen(sz, metatype);
    if (result)
    {
        midi::bytes bt;
        for (size_t i = 0; i < sz; ++i)
        {
            if (position() < size())
                bt.push_back(get());
            else
                return false;
        }
        result = e.append_meta_data(metatype, bt);
        if (result)
            result = append_event(e);
    }
    return result;
}

/**
 *  Jumps/skips the given number of bytes in the data stream.  If too large,
 *  the position is left at the end.  Primarily used in the derived class
 *  wrkfile in the seq66 library (not the application) project.
 *
 * \param sz
 *      Provides the gap size, in bytes.
 */

bool
trackdata::get_gap (size_t sz)
{
    bool result = sz > 0;
    if (result)
    {
        size_t p = position() + sz;
        if (p >= size())
        {
            p = size() - 1;
            result = false;
        }
        position() = p;
    }
    return result;
}

#endif      // RTL66_PROVIDE_EXTRA_GET_FUNCTIONS

#if defined RTL66_PROVIDE_EXTRA_PUT_FUNCTIONS

/**
 *  Writes the initial or only tempo, occurring at the beginning of a MIDI
 *  song.  Compare this function to midi::track_base::fill_time_sig_and_tempo().
 *
 * \param start_tempo
 *      The beginning tempo value.
 */

void
trackdata::put_start_tempo (midi::bpm start_tempo)
{
    write_byte(0x00);                       /* delta time at beginning      */
    write_short(0xFF51);
    write_byte(0x03);                       /* message length, must be 3    */
    write_triple(midi::ulong(60000000.0 / start_tempo));
}

/**
 *  Writes out the end-of-track marker. It emits 0xFF 0x2F 0x00. No data
 *  needed.
 */

void
trackdata::put_track_end ()
{
    put_meta_header(midi::meta::end_of_track, 0);
}

/**
 * Last, but certainly not least, write the end-of-track meta-event.
 * Only the meta header needs to be written, as there is no data.
 *
 *      FF 2F 00
 *
 * \param deltatime
 *      The MIDI delta time to write before the meta-event itself.
 */

void
trackdata::put_meta_track_end (midi::pulse deltatime)
{
    put_meta_header(midi::meta::end_of_track, 0, deltatime);
}

/**
 *  Puts the text data out. A general function for writing the textual MIDI
 *  events. But see put_ex_event(). Also see put_track_name(), which could
 *  be implemented in terms of this function.
 */

void
trackdata::put_meta_text
(
    midi::meta metacode,
    const std::string & text
)
{
    size_t len = text.length();
    put_meta_header(metacode, len);                 /* 0x01-0x07, len bytes */
    for (size_t i = 0; i < len; ++i)
        put(midi::byte(text[i]));
}

#endif      // RTL66_PROVIDE_EXTRA_PUT_FUNCTIONS

}           // namespace midi

/*
 * trackdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

