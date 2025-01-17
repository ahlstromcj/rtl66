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
 * \file          trackdataex.cpp
 *
 *  This module declares a class for holding and managing MIDI data.
 *  ENHANCED CLASS for TEMPORARY STORAGE.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2024-04-30
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
 *  Somewhat analogous to the Seq66 midi_vector class.
 *
 *      Example:  seqtrack, to be used as a helper in seqfile.
 *
 */

#include <fstream>                      /* std::ifstream and std::ofstream  */

#include "c_macros.h"                   /* errprint() macro                 */
#include "midi/calculations.hpp"        /* midi::log2_power_of_2()          */
#include "midi/eventlist.hpp"           /* midi::eventlist & event classes  */
#include "midi/trackdata.hpp"           /* midi::trackdata class            */

namespace midi  // seq in the future
{

/**
 *  Fills in the few members of this class.
 *
 * \param seq
 *      Provides a reference to the sequence/track for which this container
 *      holds MIDI data.
 */

trackdataex::trackdataex () : trackdata ()
{
    // Empty body
}

/*-------------------------------------------------------------------------
 * Enhanced "get" functions
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Enhanced "put" functions
 *-------------------------------------------------------------------------*/

/**
 *  Fills this list with an exportable track.  Following stazed, we're
 *  consolidate the tracks at the beginning of the song, replacing the actual
 *  track number with a counter that is incremented only if the track was
 *  exportable.  Note that this loop is kind of an elaboration of what goes on in
 *  the track :: fill() function for normal Seq66 file writing.
 *
 *  Exportability ensures that the track pointer is valid.  This function adds
 *  all triggered events.
 *
 *  For each trigger in the track, add events to the list below; fill
 *  one-by-one in order, creating a single long track.  Then set a single
 *  trigger for the big track: start at zero, end at last trigger end with
 *  snap.  We're going to reference (not copy) the triggers now, since the
 *  write_song() function is now locked.
 *
 *  The we adjust the track length to snap to the nearest measure past the
 *  end.  We fill the MIDI container with trigger "events", and then the
 *  container's bytes are written.
 *
 *  tick_end() isn't quite a trigger length, off by 1.  Subtracting tick_start()
 *  can really screw it up.
 */

bool
track::song_put_track
(
    int track, int tempotrack,
    bool exportable, bool standalone,
    bool hastimesig, bool hastempo
)
{
    bool result = exportable;   // events().track_info().is_exportable();
    if (result)
    {
        clear();
        if (standalone)
        {
            put_seq_number(track);
            put_seq_name(events().track_info().name());
            if (track == tempotrack)
            {
                events().scan_meta_events();
                if (hastimesig)
                {
                    put_time_sig(events().track_info().timesig_info());
                }
                if (hastempo)
                {
                    put_tempo
                    (
                        events().track_info().tempo_info().us_per_quarter_note()
                    );
                }
            }
        }
        put_triggers(seq());
    }
    return result;
}

void
track::put_triggers (const track & seq)
{
    midi::pulse last_ts = 0;
    const auto & trigs = seq.get_triggers();
    for (auto & t : trigs)
        last_ts = song_put_seq_event(t, last_ts);

    const trigger & ender = trigs.back();
    midi::pulse seqend = ender.tick_end();
    midi::pulse measticks = seq.measures_to_ticks();
    if (measticks > 0)
    {
        midi::pulse remainder = seqend % measticks;
        if (remainder != (measticks - 1))
            seqend += measticks - remainder - 1;
    }
    song_put_seq_trigger(ender, seqend, last_ts);
}

/**
 *  Fills in the Seq66-specific information for the current track:
 *  The MIDI buss number, the time-signature, and the MIDI channel.  Then, if
 *  we're not using the legacy output format, we add the "events" for the
 *  musical key, musical scale, and the background track for the current
 *  track. Finally, if tranpose support has been compiled into the program,
 *  we add that information as well.
 */

void
track::put_seqspecs (const track & seq)  // was put_proprietary
{
    put_seqspec(seq66::seqspec::midibus, 1);
    put(seq.seq_midi_bus());                 /* MIDI buss number     */
    put_seqspec(seq66::seqspec::timesig, 2);
    put(seq.get_beats_per_bar());
    put(seq.get_beat_width());

    put_seqspec(seq66::seqspec::midichannel, 1);
    put(seq.seq_midi_channel());             /* 0 to 15 or 0x80      */
    if (! usr().global_seq_feature())
    {
        /**
         * New feature: save more track-specific values, if not saved
         * globally.  We use a single byte for the key and scale, and a long
         * for the background track.  We save these values only if they are
         * different from the defaults; in most cases they will have been left
         * alone by the user.  We save per-track values here only if the
         * global-background-track feature is not in force.
         */

        if (seq.musical_key() != c_key_of_C)
        {
            put_seqspec(seq66::seqspec::musickey, 1);
            put(seq.musical_key());
        }
        if (seq.musical_scale() != c_scales_off)
        {
            put_seqspec(seq66::seqspec::musicscale, 1);
            put(seq.musical_scale());
        }
        if (seq::valid(seq.background_track()))
        {
            put_seqspec(seq66::seqspec::backtrack, 4);
            put_long(seq.background_track());
        }
    }

    /**
     *  Generally only drum patterns will not be transposable.
     */

    bool transpose = seq.transposable();
    put_seqspec(seq66::seqspec::transpose, 1);                  /* byte     */
    put(midi::byte(transpose));
    if (seq.color() != c_seq_color_none)
    {
        put_seqspec(seq66::seqspec::seq_color, 1);              /* byte     */
        put(midi::byte(seq.color()));
    }
#if defined RTL66_SEQUENCE_EDIT_MODE                            /* useful?  */
    if (seq.edit_mode() != sequence::editmode::note)
    {
        put_seqspec(seq66::seqspec::seq_edit_mode, 1);          /* byte     */
        put(seq.edit_mode_byte());
    }
#endif
    if (seq.loop_count_max() > 0)
    {
        put_seqspec(seq66::seqspec::seq_loopcount, 2);          /* short    */
        put_short(midi::ushort(seq.loop_count_max()));
    }
}

/**
 *  Fills in track events based on the trigger and events in the track
 *  associated with this track.
 *
 *  This calculation needs investigation.  The number of times the pattern is
 *  played is given by how many pattern lengths fit in the trigger length.
 *  But the commented calculation adds to the value of 1 already assigned.
 *  And what about triggers that are somehow of 0 size?  Let's try a different
 *  calculation, currently the same.
 *
 *      int times_played = 1;
 *      times_played += (trig.tick_end() - trig.tick_start()) / len;
 *
 * \param trig
 *      The current trigger to be processed.
 *
 * \param prev_timestamp
 *      The time-stamp of the previous event.
 *
 * \return
 *      The next time-stamp value is returned.
 */

midi::pulse
track::song_put_seq_event
(
   const track & seq,
   const trigger & trig,
   midi::pulse prev_timestamp
)
{
    midi::pulse len = seq.length();
    midi::pulse trig_offset = trig.offset() % len;
    midi::pulse start_offset = trig.tick_start() % len;
    midi::pulse time_offset = trig.tick_start() + trig_offset - start_offset;
    int times_played = 1 + (trig.length() - 1) / len;
    if (trig_offset > start_offset)                 /* offset len too far   */
        time_offset -= len;

    int note_is_used[c_notes_count];
    for (int i = 0; i < c_notes_count; ++i)
        note_is_used[i] = 0;                        /* initialize to off    */

    for (int p = 0; p <= times_played; ++p, time_offset += len)
    {
        midi::pulse delta_time = 0;
        for (auto e : seq.events())               /* use a copy of event  */
        {
            midi::pulse timestamp = e.timestamp() + time_offset;
            if (timestamp >= trig.tick_start())     /* at/after trigger     */
            {
                /*
                 * Save the note; eliminate Note Off if Note On is unused.
                 */

                if (e.is_note())                    /* includes aftertouch  */
                {
                    midi::byte note = e.get_note();
                    if (trig.transposed())
                        e.transpose_note(trig.transpose());

                    if (e.is_note_on())
                    {
                        if (timestamp <= trig.tick_end())
                            ++note_is_used[note];   /* count the note       */
                        else
                            continue;               /* skip                 */
                    }
                    else if (e.is_note_off())
                    {
                        if (note_is_used[note] > 0)
                        {
                            /*
                             * We have a Note On, and if past the end of trigger,
                             * use the trigger end.
                             */

                            --note_is_used[note];   /* turn off the note    */
                            if (timestamp > trig.tick_end())
                                timestamp = trig.tick_end();
                        }
                        else
                            continue;               /* if no Note On, skip  */
                    }
                }
            }
            else
                continue;                           /* before trigger, skip */

            /*
             * If the event is past the trigger end, for non-notes, skip.
             */

            if (timestamp >= trig.tick_end())       /* event past trigger   */
            {
                if (! e.is_note())                  /* (also aftertouch)    */
                    continue;                       /* drop the event       */
            }

            delta_time = timestamp - prev_timestamp;
            prev_timestamp = timestamp;
            put_event(e, delta_time);               /* does it sort???      */
        }
    }
    return prev_timestamp;
}

/**
 *  Fills in one trigger for the track, for a song-performance export.
 *  There will be only one trigger, covering the beginning to the end of the
 *  fully unlooped track. Therefore, we use the older c_triggers_ex SeqSpec,
 *  which saves a byte, while indicating the track has already been
 *  transposed.
 *
 * Using all the trigger values seems to be the same as these values, but
 * we're basically zeroing the start and offset values to make "one big
 * trigger" for the whole pattern.
 *
 *      put_long(trig.tick_start());
 *      put_long(trig.tick_end());
 *      put_long(trig.offset());
 *
 * \param trig
 *      The current trigger to be processed.
 *
 * \param length
 *      Provides the total length of the track.
 *
 * \param prev_timestamp
 *      The time-stamp of the previous event, which is actually the first
 *      event.
 */

void
track::song_put_seq_trigger
(
    const trigger & trig,
    midi::pulse length,
    midi::pulse prev_timestamp
)
{
    put_seqspec
    (
        seq66::seqspec::triggers_ex,
        trigger::datasize(seq66::seqspec::triggers_ex)
    );
    put_long(0);                                /* start tick (see banner)  */
    put_long(trig.tick_end());                  /* the ending tick          */
    put_long(0);                                /* offset is done in event  */
    put_proprietary();
    put_meta_track_end(length - prev_timestamp);           /* delta time   */
}

    // OVERRIDE
    // OVERRIDE
    // OVERRIDE

void
track::put_track (int track, int tempotrack, bool doseqspec)
{
        /*
         * Here, we add SeqSpec entries (specific to rtl66) for triggers
         * (c_triggers_ex or c_trig_transpose), the MIDI buss (c_midibus),
         * time signature (c_timesig), and MIDI channel (c_midichannel).
         * Should we restrict this to only track 0?  No, Seq66 saves these
         * events with each track.  Also, the datasize needs to be
         * calculated differently for c_trig_transpose versus c_triggers_ex.
         */

        const triggers::container & triggerlist = seq().triggerlist();
        bool transtriggers = ! rc().save_old_triggers();
        if (transtriggers)
            transtriggers = seq().any_trigger_transposed();

        if (transtriggers)
        {
            int datasize = seq().triggers_datasize(c_trig_transpose);
            put_seqspec(c_trig_transpose, datasize);
        }
        else
        {
            int datasize = seq().triggers_datasize(c_triggers_ex);
            put_seqspec(c_triggers_ex, datasize);
        }
        for (auto & t : triggerlist)
        {
            put_long(t.tick_start());
            put_long(t.tick_end());
            put_long(t.offset());
            if (transtriggers)
                put(t.transpose_byte());
        }
        put_seqspec();
}

/*-------------------------------------------------------------------------
 * Enhanced "parse seqspec" functions
 *-------------------------------------------------------------------------*/

/**
 *  Parse the proprietary header for sequencer-specific data.  The new format
 *  creates a final track chunk, starting with "MTrk".  Then comes the
 *  delta-time (here, 0), and the event.  An event is a MIDI event, a SysEx
 *  event, or a Meta event.
 *
 *  A MIDI Sequencer Specific meta message includes either a delta time or
 *  absolute time, and the MIDI Sequencer Specific event encoded as
 *  follows:
 *
\verbatim
        0x00 0xFF 0x7F length data
\endverbatim
 *
 *  For convenience, this function first checks the amount of file data left.
 *  If enough, then it reads a long value.  If the value starts with 0x00 0xFF
 *  0x7F, then that is a SeqSpec event, which signals usage of the new
 *  Seq66 "proprietary" format.  Otherwise, it is probably the old
 *  format, and the long value is a control tag (0x242400nn), which can be
 *  returned immedidately.
 *
 *  If it is the new format, we back up to the FF, then get the next byte,
 *  which should be a 7F.  If so, then we read the length (a variable
 *  length value) of the data, and then read the long value, which should
 *  be the control tag, which, again, is returned by this function.
 *
 * \note
 *      Most sequencers seem to be tolerant of both the lack of an "MTrk"
 *      marker and of the presence of an unwrapped control tag, and so can
 *      handle both the old and new formats of the final proprietary track.
 *
 * \param file_size
 *      The size of the data file.  This value is compared against the
 *      member m_pos (the position inside m_data[]), to make sure there is
 *      enough data left to process.
 *
 * \return
 *      Returns the control-tag value found.  These are the values, such as
 *      c_midichannel, found in the midi::track_base module, that indicate the
 *      type of sequencer-specific data that comes next.  If there is not
 *      enough data to process, then 0 is returned.
 */

midi::ulong
file::parse_seqspec_header (int file_size)
{
    midi::ulong result = 0;
    if ((file_size - m_pos) > int(sizeof(midi::ulong)))
    {
        result = read_long();                   /* status (new), or c_xxxx  */
        midi::byte bstatus = (result & 0x00FF0000) >> 16;   /* 2-byte shift */
        if (midi::is_meta_msg(bstatus))
        {
            skip(-2);                           /* back up to meta type     */
            midi::byte type = read_byte();      /* get meta type            */
            if (midi::is_meta_seq_spec(type))   /* 0x7F event marker        */
            {
                (void) read_varinum();          /* prop section length      */
                result = read_long();           /* control tag              */
            }
            else
            {
                msgprintf
                (
                    lib66::msglevel::error,
                    "Unexpected meta type 0x%x offset ~0x%lx",
                    int(type), long(m_pos)
                );
            }
        }
    }
    return result;
}

/**
 *  After all of the conventional MIDI tracks are read, we're now at the
 *  "proprietary" Seq24 data section, which describes the various features
 *  that Seq24 supports.  It consists of series of tags, layed out in the
 *  midi::track_base.hpp header file (search for c_mutegroups, for example).
 *
 *  The format is (1) tag ID; (2) length of data; (3) the data.
 *
 *  First, we separated out this function for a little more clarity.  Then we
 *  added code to handle reading both the legacy Seq24 format and the new,
 *  MIDI-compliant format.  Note that even the new format is not quite
 *  correct, since it doesn't handle a MIDI manufacturer's ID, making it a
 *  single byte that is part of the data.  But it does have the "MTrk" marker
 *  and track name, so that must be processed for the new format.
 *
 *  Now, in our "midicvt" project, we have a test MIDI file,
 *  b4uacuse-non-mtrk.midi that is good, except for having a tag "MUnk"
 *  instead of "MTrk".  We should consider being more permissive, if possible.
 *  Otherwise, though, the only penality is that the "proprietary" chunk is
 *  completely skipped.
 *
 * Extra precision BPM:
 *
 *  Based on a request for two decimals of precision in beats-per-minute, we
 *  now save a scaled version of BPM.  Our supported range of BPM is 2.0 to
 *  600.0.  If this range is encountered, the value is read as is.  If greater
 *  than this range (actually, we use 999 as the limit), then we divide the
 *  number by 1000 to get the actual BPM, which can thus have more precision
 *  than the old integer value allowed.  Obviously, when saving, we will
 *  multiply by 1000 to encode the BPM.
 *
 * \param p
 *      The performance object that is being set via the incoming MIDI file.
 *
 * \param file_size
 *      The file size as determined in the parse() function.
 *
 *  There are also implicit parameters, with the m_pos member
 *  variable.
 */

bool
file::parse_seqspec_track (track * trkptr, int file_size)
{
    bool result = true;
    midi::ulong ID = read_long();                      /* Get ID + Length      */
    if (ID == c_prop_chunk_tag)                     /* magic number 'MTrk'  */
    {
        midi::ulong tracklength = read_long();
        if (tracklength > 0)
        {
            /*
             * The old number, 0x7777, is now 0x3FFF.  We don't want to
             * startle people, so we will silently ignore (and replace upon
             * saving) this number.
             */

            int sn = read_seq_number();
            bool ok = (sn == c_prop_seq_number) || (sn == c_prop_seq_number_old);
            if (ok)                                 /* sanity check         */
            {
                std::string trackname = read_track_name();
                result = ! trackname.empty();

                /*
                 * This "sanity check" is probably a bit much.  It causes
                 * errors in Sequencer24 tracks, which are otherwise fine
                 * to scan in the new format.  Let the "MTrk" and 0x3FFF
                 * markers be enough.
                 *
                 * if (trackname != c_prop_track_name)
                 *     result = false;
                 */
            }
            else if (sn == (-1))
            {
                m_error_is_fatal = false;
                result = set_error_dump
                (
                    "No sequence number in SeqSpec track, extra data"
                );
            }
            else
                result = set_error("Unexpected sequence number, SeqSpec track");
        }
    }
    else
        skip(-4);                                   /* unread the "ID code" */

    return result;
}

/*-------------------------------------------------------------------------
 * Enhanced "write seqspec" functions
 *-------------------------------------------------------------------------*/

/**
 *  Calculates the size of a trackname and the meta event that specifies
 *  it.
 *
 * \param trackname
 *      Provides the name of the track to be written to the MIDI file.
 *
 * \return
 *      Returns the length of the event, which is of the format "0x00 0xFF
 *      0x03 len track-name-bytes".
 */

long
file::track_name_size (const std::string & trackname) const
{
    long result = 0;
    if (! trackname.empty())
    {
        result += 3;                                    /* 0x00 0xFF 0x03   */
        result += varinum_size(long(trackname.size())); /* variable length  */
        result += long(trackname.size());               /* data size        */
    }
    return result;
}

/**
 *  Writes a "proprietary" (SeqSpec) Seq24 footer header in the new
 *  MIDI-compliant format.  This function does not write the data.  It
 *  replaces calls such as "write_long(c_midichannel)" in the proprietary
 *  secton of write().
 *
 *  The new format writes 0x00 0xFF 0x7F len 0x242400xx; the first 0x00 is the
 *  delta time.
 *
 *  In the new format, the 0x24 is a kind of "manufacturer ID".  At
 *  http://www.midi.org/techspecs/manid.php we see that most manufacturer IDs
 *  start with 0x00, and are thus three bytes long, or start with codes at
 *  0x40 and above.  Similary, this site shows that no manufacturer uses 0x24:
 *
 *      http://sequence15.blogspot.com/2008/12/midi-manufacturer-ids.html
 *
 * \warning
 *      Currently, the manufacturer ID is not handled; it is part of the
 *      data, which can be misleading in programs that analyze MIDI files.
 *
 * \param control_tag
 *      Determines the type of sequencer-specific section to be written.
 *      It should be one of the value in the globals module, such as
 *      c_midibus or c_mutegroups.
 *
 * \param data_length
 *      The amount of data that will be written.  This parameter does not
 *      count the length of the header itself.
 */

void
file::write_seqspec_header (midi::ulong control_tag, long len)
{
    write_byte(0);                      /* delta time                   */
    write_byte(EVENT_MIDI_META);        /* 0xFF meta marker             */
    write_byte(EVENT_META_SEQSPEC);     /* 0x7F sequencer-specific mark */
    write_varinum(len + 4);             /* data + sizeof(control_tag);  */
    write_long(control_tag);            /* use legacy output call       */
}

/**
 *  Writes out the final proprietary/SeqSpec section, using the new format.
 *
 *  The first thing to do, for the new format only, is calculate the length
 *  of this big section of data.  This was quite tricky; we tweaked and
 *  adjusted until the midicvt program handled the whole new-format file
 *  without emitting any errors.
 *
 *  Here's the basics of what Seq24 did for writing the data in this part of
 *  the file:
 *
 *      -#  Write the c_midictrl value, then write a 0.  To us, this looks like
 *          no one wrote any code to write this data.  And yet, the parsing
 *          code can handles a non-zero value, which is the number of sequences
 *          as a long value, not a byte.  So shouldn't we write 4 bytes, not
 *          one?  Yes, indeed, we made a mistake.  However, we should be
 *          writing out the full data set as well.  But not even Seq24 does
 *          that!  Perhaps they decided it was best kept in the "rc"
 *          configuration file.
 *      -#  MORE TO COME.
 *
 *  We need a way to make the group mute data optional.  Why write 4096 bytes
 *  of zeroes?
 *
 * \param p
 *      Provides the object that will contain and manage the entire
 *      performance.
 *
 * \return
 *      Always returns true.  No efficient way to check all of the writes that
 *      can happen.  Might revisit this issue if some bug crops up.
 */

bool
trackdataex::write_seqspec_track (const track * trkptr)
{
    const mutegroups & mutes = p.mutes();
    long tracklength = 0;
    int cnotesz = 2;                            /* first value is short     */
    int highset = p.highest_set();              /* high set number re 0     */
    int maxsets = c_max_sets;                   /* i.e. 32                  */
    if (highset >= maxsets)
        maxsets = highset + 1;

    for (int s = 0; s < maxsets; ++s)
    {
        if (s <= highset)                       /* unused tracks = no name  */
        {
            const std::string & note = p.set_name(s);
            cnotesz += 2 + note.length();       /* short + note length      */
        }
    }

    unsigned groupcount = c_max_groups;         /* 32, the maximum          */
    unsigned groupsize = p.screenset_size();
    int gmutesz = 0;
    if (mutes.saveable_to_midi())
    {
        groupcount = unsigned(mutes.count());   /* includes unused groups   */
        groupsize = unsigned(mutes.group_count());
        if (rc().save_old_mutes())
            gmutesz = 4 + groupcount * (4 + groupsize * 4); /* 4-->longs    */
        else
            gmutesz = 4 + groupcount * (1 + groupsize);     /* 1-->bytes    */

        gmutesz += mutes.group_names_letter_count();        /* NEW NEW NEW  */
    }
    tracklength += seq_number_size();           /* bogus sequence number    */
    tracklength += track_name_size(c_prop_track_name);
    tracklength += prop_item_size(4);           /* c_midictrl               */
    tracklength += prop_item_size(4);           /* c_midiclocks             */
    tracklength += prop_item_size(cnotesz);     /* c_notes                  */
    tracklength += prop_item_size(4);           /* c_bpmtag, beats/minute   */
    tracklength += track_end_size();            /* Meta TrkEnd              */
    write_long(c_prop_chunk_tag);               /* "MTrk" or something else */
    write_long(tracklength);
    write_seq_number(c_prop_seq_number);        /* bogus sequence number    */
    write_track_name(c_prop_track_name);        /* bogus track name         */
    write_seqspec_header(c_midictrl, 4);        /* midi control tag + 4     */
    write_long(0);                              /* Seq24 writes only a zero */
    write_seqspec_header(c_midiclocks, 4);      /* bus mute/unmute data + 4 */
    write_long(0);                              /* Seq24 writes only a zero */
    write_seqspec_header(c_notes, cnotesz);     /* namepad data tag + data  */
    write_short(maxsets);                       /* data, not a tag          */
    for (int s = 0; s < maxsets; ++s)           /* see "cnotesz" calc       */
    {
        if (s <= highset)                       /* unused tracks = no name  */
        {
            const std::string & note = p.set_name(s);
            write_short(midi::ushort(note.length()));
            for (unsigned n = 0; n < unsigned(note.length()); ++n)
                write_byte(midi::byte(note[n]));
        }
        else
            write_short(0);                         /* name is empty        */
    }
    write_seqspec_header(c_bpmtag, 4);              /* bpm tag + long data  */

    /*
     *  We now encode the Seq66-specific BPM value by multiplying it
     *  by 1000.0 first, to get more implicit precision in the number.
     *  We should probably sanity-check the BPM at some point.
     */

    midi::ulong scaled_bpm = usr().scaled_bpm(p.get_beats_per_minute());
    write_long(scaled_bpm);                         /* 4 bytes              */
    return true;
}

}           // namespace midi

/*
 * trackdataex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

