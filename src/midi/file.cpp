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
 * \file          file.cpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2025-01-16
 * \license       GNU GPLv2 or above
 *
 *  A midi::file is file-header data plus the data in each of the tracks of
 *  the file. Each track holds its own data, which is extracted from the
 *  file's data array.
 *
 *  For a quick guide to the MIDI format, see, for example:
 *
 *  http://www.mobilefish.com/tutorials/midi/midi_quickguide_specification.html
 *
 *  It is important to note that most sequencers have taken a shortcut or two
 *  in reading the MIDI format.  For example, most will silently ignore an
 *  unadorned control tag (0x242400nn) which has not been packaged up as a
 *  proper sequencer-specific meta event.  The midicvt program
 *  (https://github.com/ahlstromcj/midicvt, derived from midicomp, midi2text,
 *  and mf2t/t2mf) does not ignore this lack of a SeqSpec wrapper, and hence
 *  we decided to provide a new, more strict input and output format for the
 *  the proprietary/SeqSpec track in Seq66.
 *
 *  Elements written:
 *
 *      -   MIDI header.
 *      -   Tracks.
 *          These items are then written, preceded by the "MTrk" tag and
 *          the track size.
 *          -   Track number.
 *          -   Track name.
 *          -   Time-signature and tempo (track 0 only)
 *          -   Track events.
 *
 *  Items handled in midi::track:
 *
 *      -   Key signature.  Although Seq66 does not create or use this
 *          event, if present, it is preserved so that it can be written out
 *          to the file when saved.
 *      -   Time signature.
 *      -   Tempo.
 *      -   Track number.
 *      -   Track end.
 *      -   Generic SeqSpec data.
 *
 *  Uses the new format for the proprietary footer section of the Seq24 MIDI
 *  file.
 *
 *  In the new format, each sequencer-specfic value (0x242400xx) is preceded
 *  by the sequencer-specific prefix, 0xFF 0x7F len).
 *
 * Running status:
 *
 *      A recommended approach for a receiving device is to maintain its
 *      "running status buffer" as so:
 *
 *      -#  Buffer is cleared (ie, set to 0) at power up.
 *      -#  Buffer stores the status when a Voice Category Status (ie, 0x80
 *          to 0xEF) is received.
 *      -#  Buffer is cleared when a System Common Category Status (ie, 0xF0
 *          to 0xF7) is received.
 *      -#  Nothing is done to the buffer when a RealTime Category message is
 *          received.
 *      -#  Any data bytes are ignored when the buffer is 0.
 *
 * Reading and Writing Tracks:
 *
 *      The eventlist and vector of raw track event bytes are now consolidated
 *      in midi::trackdata, which is one of the base classes of midi::track.
 *
 *      Instead of reading all the data into one buffer, we create a track for
 *      each sequence in the file, and then use midi::trackdata functions to
 *      build each track.
 *
 * Differences from Seq66 (version 1):
 *
 *  In Seq66, the midifile class uses its own code to parse the MIDI file
 *  byte stream, but the writing uses a midi_vector(_base) class. The Seq66v2
 *  layout is diffe rent.
 *
 *      -   Parsing.
 *          -   The MIDI file is read into a util::bytevector, which is
 *              a class in the cfg66 library. This class provides for
 *              positioning incrementing and decrementing, seeking, and
 *              put/poke/peek/get functions.
 */

#include <fstream>                      /* std::ifstream & std::ofstream    */
#include <memory>                       /* std::unique_ptr<>                */

#include "midi/file.hpp"                /* midi::file base read/write class */
#include "midi/player.hpp"              /* midi::player coordinator class   */
#include "util/filefunctions.hpp"       /* util::file_extension_match()     */
#include "util/msgfunctions.hpp"        /* msglevel & util::msgfunctions    */

namespace midi
{

#if defined UNUSED_VARIABLES

/**
 *  Minimal MIDI file size.  Used for a sanity check, it is also the length of
 *  the header portion of the MIDI file.
 *
 * <Header Chunk> = <chunk type><length><format><ntrks><division>
 *
 * Bytes:                 4   +   4   +   2  +   2   +   2   = 14
 */

static const size_t c_midi_header_size = 14;

/**
 *  A manifest constant for controlling the length of the stream buffering
 *  array in a MIDI file. This value is 0x0400.
 */

static const int c_midi_line_max = 1024;

#endif

/**
 *  Highlights the MIDI file header value, "MThd".
 */

static const midi::tag c_mthd_tag  = 0x4D546864;      /* magic number 'MThd'  */

/**
 *  The chunk header value for the Seq66 proprietary/SeqSpec section.  We
 *  might try other chunks, as well, since, as per the MIDI specification,
 *  unknown chunks should not cause an error in a sequencer (or our midicvt
 *  program).  For now, we stick with "MTrk".
 *
 *      static const midi::tag c_prop_chunk_tag = c_mtrk_tag;
 */

/**
 *  Provides the track number for the proprietary/SeqSpec data when using
 *  the new format.  (There is no track number for the legacy format.)
 *  Can't use numbers, such as 0xFFFF, that have MIDI meta tags in them,
 *  confuses our "proprietary" track parser.
 *
 *      static const midi::ushort c_prop_seq_number     = 0x3FFF;
 *      static const midi::ushort c_prop_seq_number_old = 0x7777;
 */

/**
 *  Provides the track name for the "proprietary" data when using the new
 *  format.  (There is no track-name for the "proprietary" footer track when
 *  the legacy format is in force.)  This is more useful for examining a hex
 *  dump of a Seq66 song than for checking its validity.  It's overkill
 *  that causes needless error messages.
 *
 *  Also, this value is not used in plain MIDI files.
 */

static const std::string c_prop_track_name = "Seq66-S";

/**
 *  This const is used for detecting SeqSpec data that Seq66 does not handle.
 *  If this word is found, then we simply extract the expected number of
 *  characters specified by that construct, and skip them when parsing a MIDI
 *  file.
 *
 *  Also, this value is not used in plain MIDI files.
 *
 *      static const midi::tag c_prop_tag_word = 0x24240000;
 */

/**
 *  Defines the size of the time-signature and tempo information.  The sizes of
 *  these meta events consists of the delta time of 0 (1 byte), the event and
 *  size bytes (3 bytes), and the data (4 bytes for time-signature and 3 bytes
 *  for the tempo.  So, 8 bytes for the time-signature and 7 bytes for the
 *  tempo.  Unused, so commented out.
 *
 *      static const int c_time_tempo_size  = 15;
 */

/*
 *  Internal functions.
 */

/**
 *  An easier, shorter test for the c_prop_tag_word part of a long
 *  value, that clients can use.
 *
 *      static bool
 *      is_proptag (midi::ulong p)
 *      {
 *          return (midi::tag(p) & c_prop_tag_word) == c_prop_tag_word;
 *      }
 */

/**
 *  Principal constructor.
 *
 * \param filespec
 *      Provides the name of the MIDI file to be read or written. This is the
 *      full path including the file-name.
 *
 * \param p
 *      Provides a mandator player object, which wll hold some important
 *      global parameters (BPM, PPQN, etc.) about the file.
 *
 * \param smf0split
 *      If true (the default), and the MIDI file is an SMF 0 file, then it
 *      will be split into multiple tracks by channel.
 */

file::file
(
    const std::string & filespec,
    player & p,
    bool smf0split
) :
    m_coordinator       (p),
    m_file_size         (0),
    m_data              (),                 /* vector of big-endianbytes    */
    m_file_spec         (filespec),
    m_file_ppqn         (0),                /* will change                  */
    m_smf0_splitter     (),
    m_smf0_split        (smf0split)
{
    // no other code needed
}

/**
 *  A rote destructor.
 */

file::~file ()
{
    // empty body
}

/**
 *  Creates the stream input, reads it into the "buffer", and then closes
 *  the file.  No file buffering needed on these beefy machines!  :-)
 *  As a side-effect, also sets m_file_size.
 *
 *  We were using the assignment operator, but this caused an error using old
 *  32-bit debian stable, g++ 4.9 on one of our old laptops.  The assignment
 *  operator was deleted by the compiler.  So now we use constructor notation.
 *  A little bit odd, since we thought the compiler would convert assignment
 *  operator notation to constructor notation, but hey, compilers are not
 *  perfect.  Also, no need to use the krufty string pointer for the
 *  file-name.
 *
 * \param tag
 *      Basically an informative string to denote what kind of file is being
 *      opened, "MIDI" or "WRK".
 *
 * \return
 *      Returns true if the input stream was successfully opend on a good
 *      file.  Use it only if the return value is true.
 */

bool
file::parse (const std::string & tag)
{
    bool result = m_data.read(m_file_spec);
    if (result)
    {
        m_file_size = m_data.size();                /* just logged for now  */
        util::file_message(tag, m_file_spec);
        result = parse_smf_1();
    }
    return result;
}

/**
 *  Grabs the basic information from the header of the MIDI file.
 *  Depending on the MIDI file format code found, SMF 0 splitting may
 *  be set up in the coordinator() [player] object.
 *
 *  Format of a MIDI header chunk:
 *
 *      -   MThd. The magic bytes for a MIDI file.
 *          -   Offset 0 relative to the start of the file.
 *          -   4 bytes.
 *      -   Header Length.
 *          -   Offset 4.
 *          -   4 bytes! Always equal to 6.
 *      -   MIDI Format (SMF).
 *          -   Offset 8.
 *          -   2 bytes.
 *          -   Values:
 *              -   0.  File contains one multi-channel track.
 *              -   1.  File contains one or more simultaneous tracks.
 *              -   2.  File contains one or more independent single-track
 *                      patterns.
 *      -   Number of Tracks.
 *          -   Offset A (10 dec).
 *          -   2 bytes. Always 1 for SMF 0.
 *      -   Division.
 *          -   Offset C (12 dec)
 *          -   2 bytes.
 *          -   The very first bit is either 0 or 1 to indicate the following:
 *              -   0.  Ticks (pulses) per quarter-note (PPQN). The highest
 *                      value is 0x7FFF = 32767. Is 24 the lowest allowed?
 *              -   1.  Negative SMPTE format (MSB) and Ticks per frame
 *                      (LSB). Not supported in Rtl66 at this time.
 *
 *      After the header, at offset E (14 dec), the first track starts.
 *      See the parse_smf_1() function's banner for some details.
 *
 * \return
 *      Returns the number of tracks.  If 0, the file is considered bad.
 */

int
file::read_header ()
{
    int result = 0;
    bool ok = true;
    midi::ulong ID = read_long();                       /* hdr chunk magic  */
    midi::ulong hdrlength = read_long();                /* MThd length      */
    clear_errors();
    if (ID != c_mthd_tag || hdrlength != 6)             /* magic 'MThd'     */
    {
        ok = set_error_dump("Invalid MIDI header chunk", ID);
    }

    if (ok)
    {
        midi::ushort Format = read_short();             /* 0, 1, or 2       */
        if (Format == 0)
        {
            m_smf0_splitter.initialize();               /* SMF 0 support    */
            coordinator().smf_format(0);
        }
        else if (Format == 1)
        {
            coordinator().smf_format(1);
            smf0_split(false);
        }
        else
        {
            ok = set_error_dump
            (
                "Unsupported MIDI format number", midi::ulong(Format)
            );
        }
    }
    if (ok)
    {
        midi::ushort trackcount = read_short();
        midi::ushort fppqn = read_short();
        file_ppqn(midi::ppqn(fppqn));
        coordinator().set_ppqn(file_ppqn());            /* let player know  */
        result = int(trackcount);
    }
    return result;
}

/**
 *  This function parses an SMF 0 binary MIDI file as if it were an SMF 1
 *  file, then, if more than one MIDI channel was encountered in the sequence,
 *  splits all of the channels in the sequence out into separate sequences.
 *  The original sequence remains in place, in sequence slot 16 (the 17th
 *  slot).  The user is responsible for deleting it if it is not needed.
 *
 * \param p
 *      Provides a reference to the player object to which sequences/tracks
 *      are to be added.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
file::parse_smf_0 ()
{
    bool result = parse_smf_1();                    /* format 0 conversion  */
    if (result)
    {
        if (smf0_split())
        {
            result = m_smf0_splitter.split(coordinator());
            coordinator().modify();                 /* to prompt for save   */
            coordinator().smf_format(1);            /* converted to SMF 1   */
        }
        else
        {
            track::pointer trkptr = coordinator().get_track(0);
            if (trkptr)
            {
                trkptr->midi_channel(null_channel());

                /*
                 *  Not supported in this base implementation.
                 *
                 * trk->set_color(palette_to_int(cyan));
                 */

                coordinator().smf_format(0);
            }
        }
    }
    return result;
}

/**
 *  This function parses an SMF 1 binary MIDI file; it is basically the
 *  original seq66 midifile::parse() function. It assumes the file-data has
 *  already been read into memory in totality, in the bytevector m_data.
 *  It first calls read_header(); see that function's banner for the
 *  layout of a MIDI header. The layout of a track is shown here:
 *
 *      -   Mtrk. The magic bytes for the start of a MIDI track.
 *          -   Offset 0 relative to the start of the track.
 *          -   4 bytes.
 *      -   Track length.
 *          -   Offset 4.
 *          -   4 bytes.
 *      -   Channel events, meta events, and system exclusive events.
 *      -   End-of-track event.
 *
 * \param is_smf0
 *      True if we detected that the MIDI file is in SMF 0 format.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
file::parse_smf_1 ()
{
    bool result = true;
    size_t offset;                              /* used/adjusted in loop    */
    int track_count = read_header();
    track_list().clear();
    for (int trk = 0; trk < track_count; ++trk)
    {
        const size_t s_track_header_size = 8;   /* size of ID and length    */
        midi::ulong ID = read_long();           /* get track marker 'MTrk'  */
        midi::ulong tracklen = read_long();     /* get track length         */
        if (ID == c_mtrk_tag)                   /* magic number 'MTrk'?     */
        {

            track * sp = create_track();        /* create new track         */
            bool ok = not_nullptr(sp);
            if (ok)
            {
                offset = m_data.real_position();

                /*
                 * Do this at install-track time.
                 *
                 * sp->set_parent(&coordinator()); // also sets track params   //
                 */

#if defined PLATFORM_DEBUG_TMI
                printf("Parsing track %d\n", trk);
#endif

                sp->track_number(trk);
                offset = sp->parse_track(m_data, offset, tracklen);
                ok = offset > 0;
                if (ok)
                {
                    track::number trkno = sp->track_number();

                    // TODO
                    // if (! is_null_buss(buss_override))
                    //     (void) sp->midi_bus(buss_override);

                    if (smf0_split())
                        ok = m_smf0_splitter.log_main_events(*sp, trkno);
                    else
                        ok = finalize_track(sp, trkno);

                    if (ok)
                    {
                        /*
                         * The track was processed, skip to the next track.
                         * The current version of Seq66 doesn't count the
                         * final SeqSpec track in the MIDI track number.
                         * In case this could be true for this file, increment
                         * the track count if we're on the last track, but
                         * there a enough remaining bytes to be processed.
                         */

                        if (offset >= m_data.size())
                        {
                            break;                          /* we are done  */
                        }
                        else
                        {
                            set_position(offset);
                            if (trk == (track_count - 1))
                            {
                                size_t r = remainder();
                                if (r >= s_track_header_size)
                                {
                                    ++track_count;          /* BEWARE!!!    */
                                }
                            }
                        }
                    }
                    else
                    {
                        result = set_error_dump
                        (
                            "Could not finalize track", (unsigned long)(trk)
                        );
                        break;
                    }
                }
            }
        }
        else
        {
            if (trk > 0)                                /* non-fatal later  */
            {
                (void) set_error_dump("Skipped unknown track ID", ID);
            }
            else                                        /* fatal in 1st one */
            {
                result = set_error_dump("Unsupported track ID", ID);
                break;
            }
        }
    }
    return result;
}

track *
file::create_track ()
{
    track * result = new (std::nothrow) track();        /* track 0  */
    if (not_nullptr(result))
    {
        midi::masterbus * masterbus = coordinator().master_bus();
        if (not_nullptr(masterbus))
            result->master_midi_bus(masterbus);
    }
    return result;
}

/**
 *  If the track pointer is valid and the "coordinator" player object exists,
 *  then this function calls player::install_track() to add the track pointer
 *  to the set-manager and hook it into the master bus.
 *
 * \param trk
 *      Provides a pointer to the track to be finalized and installed.
 *
 * \param trkno
 *      The starting track number to use, which might not be available.
 *
 * \return
 *      Returns true only if the track was installed in the player.
 */

bool
file::finalize_track (track * trk, int trkno)
{
    bool result = not_nullptr(trk);
    if (result)
    {
        int preferred_seqnum = trkno;
        result = coordinator().install_track(trk, preferred_seqnum, true);
    }
    return result;
}

/**
 * We want to write:
 *
 *  -   0x4D54726B.
 *      The track tag "MTrk".  The MIDI spec requires that software can skip
 *      over non-standard chunks. "Prop"?  Would require a fix to midicvt.
 *  -   0xaabbccdd.
 *      The length of the track.  This needs to be calculated somehow.
 *  -   0x00.  A zero delta time.
 *  -   0x7f7f.  Sequence number, a special value, well out of normal range.
 *  -   The name of the track:
 *      -   "Seq24-Spec"
 *      -   "Seq66-S"
 *
 *   Then follows the proprietary/SeqSpec data, written in the normal manner.
 *   Finally, tack on the track-end meta-event.
 *
 *   Components of final track size:
 *
 *      -# Delta time.  1 byte, always 0x00.
 *      -# Sequence number.  5 bytes.  OPTIONAL.  We won't write it.
 *      -# Track name. 3 + 10 or 3 + 15
 *      -# Series of proprietary/SeqSpec specs if applicable:
 *         -# Prop header:
 *            -# If legacy [obsolete] format, 4 bytes.
 *            -# Otherwise, 2 bytes + varinum_size(length) + 4 bytes.
 *            -# Length of the prop data.
 *      -# Track End. 3 bytes.
 *
 * \param numtracks
 *      The number of tracks to be written.  For SMF 0 this should be 1.
 *
 * \param smfformat
 *      The SMF value to write.  Defaults to 1.
 */

bool
file::put_header (int numtracks, int smfformat)
{
    put_long(0x4D546864);                 /* MIDI Format 1 header MThd    */
    put_long(6);                          /* Length of the header         */
    put_short(smfformat);                 /* MIDI Format 1 (or 0)         */
    put_short(numtracks);                 /* number of tracks             */
    put_short(m_file_ppqn);               /* parts per quarter note       */
    return numtracks > 0;
}

/**
 *  Write a MIDI track to the file buffer.  This assumes the track has already
 *  been converted from events to raw MIDI data bytes, so this is the first
 *  call.
 *
 *  Note that trackdata::put_track() gets some track data from member
 *  functions rather than events. It also applies event channel numbers.
 *
 * \param trk
 *      The MIDI track containing the events.
 *
 * \return
 *      Returns true if the function succeeded.
 */

bool
file::put_track (/*const*/ midi::track & trk)
{
    trackdata & trkdata = trk.data();
    bool result = trkdata.put_track(trk, 0, false); // tempotrack, doseqspec
    if (result)
    {
        midi::ulong tracksize = midi::ulong(trkdata.size());
        if (tracksize > 0)
        {
            put_long(c_mtrk_tag);               /* magic number 'MTrk'      */
            put_long(tracksize);
            while (! trkdata.done())            /* get track data to buffer */
                put(trkdata.get());

            // midi::bytes ender = lst.end_of_track();  // already in track
        }
    }
    return result;
}

/**
 *  Similar to put_track(), but gets events directly from the track,
 *  Note that trackdata::put_track_events() gets all track data from
 *  events only.
 *
 * \return
 *      Returns true if the function succeeded.
 */

bool
file::put_track_events (/*const*/ midi::track & trk)
{
    trackdata & trkdata = trk.data();
    bool result = trkdata.put_track_events(trk);
    if (result)
    {
        midi::ulong tracksize = midi::ulong(trkdata.size());
        if (tracksize > 0)
        {
            put_long(c_mtrk_tag);               /* magic number 'MTrk'      */
            put_long(tracksize);
            trkdata.reset_position();           /* get back to first byte   */
            while (! trkdata.done())            /* get track data to buffer */
                put(trkdata.get());

            // midi::bytes ender = lst.end_of_track();  // already in track
        }
    }
    return result;
}

/**
 *  Write the whole MIDI data and Seq24 information out to the file.
 *  Also see the put_song() function, for exporting to standard MIDI.
 *
 *  Seq66 sometimes reverses the order of some events, due to popping
 *  from its container.  Not an issue, but can make a file slightly different
 *  for no reason.
 *
 *  Also can handle the time-signature and tempo meta events, if they are
 *  not part of the file's MIDI data.  All the events are put into the
 *  container, and then the container's bytes are written out below.
 *
 * \param eventsonly
 *      If true, write all events from tracks.
 *
 * \param smfformat ???
 *      Defaults to 1.  Can be set to 0 for writing an SMF 0 file.
 *
 * \return
 *      Returns true if the write operations succeeded.  If false is returned,
 *      then m_error_message will contain a description of the error.
 */

bool
file::write (bool eventsonly)
{
    int numtracks = 0;
    int trackhigh = coordinator().track_high() + 1; /* convert to a count   */
    int smfformat = coordinator().smf_format();
    clear();                                        /* clear errors+buffer  */
    for (int i = 0; i < trackhigh; ++i)
    {
        if (coordinator().is_track_active(track::number(i)))
            ++numtracks;                            /* count active tracks  */
    }
    file_ppqn(coordinator().get_ppqn());

    bool result = put_header(numtracks, smfformat);
    if (result)
    {
        std::string caption = "Writing MIDI SMF ";
        caption += std::to_string(smfformat);
        caption += " MIDI file ";
        caption += std::to_string(m_file_ppqn);
        caption += " PPQN";
        util::file_message(caption, m_file_spec);
        for (int t = 0; t < trackhigh; ++t)
        {
            track::pointer trkptr = coordinator().get_track(t);
            if (trkptr)
            {
#if defined PLATFORM_DEBUG_TMI
                printf
                (
                    "Writing track %d: %d events\n",
                    trkptr->track_number(), trkptr->event_count()
                );
#endif
                if (eventsonly)
                    result = put_track_events(*trkptr);
                else
                    result = put_track(*trkptr);

                if (! result)
                    break;
            }
        }
        if (result)
            result = m_data.write(m_file_spec);
    }
    else
        result = set_error_dump("No patterns/tracks to write.");

    if (result)
        coordinator().unmodify();      /* it worked, tell player about it   */

    return result;
}

/**
 *  Creates a MIDI file object based on the file extension.  The base version
 *  supports only stock MIDI files.  Derived classes can create other file
 *  types, such as Cakewalk WRK files.
 *
 * Derived class:
 *
 *      bool is_wrk = util::file_extension_match(fn, "wrk");
 *      if (is_wrk)
 *          fp = new (std::nothrow) wrkfile(fn, ppqn);
 */

file *
make_midi_file_object
(
    player & p,
    const std::string & filespec,
    bool smf0split
)
{
    file * result = nullptr;
    bool is_midi =
        util::file_extension_match(filespec, "mid")  ||
        util::file_extension_match(filespec, "midi") ||
            util::file_extension_match(filespec, "smf")
        ;

    if (is_midi)
        result = new (std::nothrow) file(filespec, p, smf0split);

    return result;
}

/**
 *  A global function to unify the opening of a MIDI or WRK file.  It also
 *  handles PPQN discovery.
 *
 *  We do not need to clear any existing playlist.  The new function,
 *  player::clear_all(), also does a clear-all, including the playlist, if
 *  its boolean parameter is set to true.  We leave it false here.
 *
 * \todo
 *      Tighten up wrkfile/file handling re PPQN!!!
 *
 * \param [in,out] p
 *      Provides the performance object to update with information read from
 *      the file.
 *
 * \param fn
 *      The full path specification for the file to be opened.
 *
 * \param [out] errmsg
 *      If the function fails, this string is filled with the error message.
 *
 * \return
 *      Returns true if reading the MIDI/WRK file succeeded. As a side-effect,
 *      the usrsettings::file_ppqn() is set to return the final PPQN to be
 *      used.
 */

bool
read_midi_file
(
    player & p,
    const std::string & fn,
    std::string & errmsg
)
{
    bool result = util::file_readable(fn);
    if (result)
    {
        std::unique_ptr<file> mf(make_midi_file_object(p, fn));
        if (mf)
        {
            p.clear_all();                          /* see banner notes     */
            if (result)
                result = mf->parse();               /* add a tag string?    */

            if (result)
                util::file_message("Read MIDI file", fn);
            else
                errmsg = mf->error_message();
        }
        else
            errmsg = "Could not make MIDI-file object";
    }
    else
    {
        std::string msg = "File not accessible";
        util::file_error(msg, fn);
        errmsg = msg + ": " + fn;
    }
    return result;
}

bool
write_midi_file
(
    player & p,
    const std::string & fn,
    std::string & errmsg,
    bool eventsonly
)
{
    bool result = false;
    if (fn.empty())
    {
        errmsg = "No file-name to write";
    }
    else
    {
        file f(fn, p, false);
        result = f.write(eventsonly);
        if (result)
        {
            util::file_message("Wrote MIDI file", fn);
            p.unmodify();
        }
        else
        {
            errmsg = f.error_message();
            util::file_error("Write failed", fn);
        }
    }
    return result;
}

}           // namespace seq66

/*
 * file.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

