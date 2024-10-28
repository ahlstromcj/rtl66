#if ! defined RTL66_MIDI_TRACKDATA_HPP
#define RTL66_MIDI_TRACKDATA_HPP

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
 * \file          trackdata.hpp
 *
 *  This module declares a class for managing raw MIDI events in an event list.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2024-05-26
 * \license       GNU GPLv2 or above
 *
 *  This class is meant to hold the bytes that represent MIDI events and other
 *  MIDI data, which can then be dumped to a MIDI file.
 */

#include <iosfwd>                       /* std::ifstream forward reference  */
#include <string>                       /* std::string class                */

#include "midi/eventlist.hpp"           /* midi::eventlist container class  */
#include "midi/trackinfo.hpp"           /* small info classes for MIDI      */
#include "util/bytevector.hpp"          /* util::bytevector big-endian data */

namespace midi
{

class track;

/**
 *  Highlights the MIDI file track-marker (chunk) value, "MTrk".
 */

const midi::tag c_mtrk_tag  = 0x4D54726B;           /* magic number 'MTrk'  */

/**
 *  We need to offer some options for handling running-status issues in
 *  some MIDI files. See the midi::file class.
 *
 * \var recover
 *      Try to recover the running-status value.
 *
 * \var skip
 *      Skip the rest of the track.
 *
 * \var proceed
 *      Allow running status errors to cascade.
 *
 * \var abort
 *      Stop processing the rest of the tracks.
 */

enum class rsaction
{
    recover,    /* Try to recover the running-status value.                 */
    skip,       /* Skip the rest of the track.                              */
    proceed,    /* Allow running status errors to cascade.                  */
    abort       /* Stop processing the rest of the tracks.                  */
};

/**
 *    This class is the base class for a container of MIDI track
 *    information.  It is one of the base classes for the track class.
 */

class trackdata
{
    friend class file;
    friend class track;

private:

    /**
     *  Provide a hook into an eventlist so that we can exchange data with a
     *  it.
     */

    midi::eventlist m_events;

    /**
     *  Provides management code for handling big-endian data, which include
     *  MIDI data.
     */

    mutable util::bytevector m_data;


    /**
     *  Holds the value for how to handle mistakes in running status.
     */

    rsaction m_running_status_action;

    /**
     *  Holds a copy of the "manufacturer ID". Useful in getting and putting
     *  SeqSpec data.
     */

    midi::bytes m_manufacturer_id;

    /**
     *  Indicates that an end-of-track event was encountered. Stop processing
     *  the track data.
     */

    bool m_end_of_track_found;

public:

    trackdata ();
    trackdata (const trackdata &) = default;
    trackdata (trackdata &&) = delete;              /* forced by eventlist  */
    trackdata & operator = (const trackdata &) = default;
    trackdata & operator = (trackdata &&) = delete; /* ditto */
    virtual ~trackdata () = default;

    size_t size () const
    {
        return m_data.size();
    }

    void clear_buffer ()
    {
        m_data.clear();
    }

    void clear_all ()
    {
        m_events.clear();
        clear_buffer();
    }

    bool done () const
    {
        return position() >= size();
    }

    midi::bytes & byte_list ()
    {
        return m_data.byte_list();
    }

    const midi::bytes & byte_list () const
    {
        return m_data.byte_list();
    }

    midi::bytes & manufacturer_id ()
    {
        return m_manufacturer_id;
    }

    void manufacturer_id (midi::bytes manufid)
    {
        m_manufacturer_id = manufid;
    }

    const midi::bytes & manufacturer_id () const
    {
        return m_manufacturer_id;
    }

    bool end_of_track_found () const
    {
        return m_end_of_track_found;
    }

    /*---------------------------------------------------------------------
     * "get" functions
     *---------------------------------------------------------------------*/

#if defined PLATFORM_DEBUG_TMI

    midi::byte get () const;
    midi::byte peek () const;

#else

    midi::byte get () const
    {
        return m_data.get_byte();
    }

    midi::byte peek () const
    {
        return m_data.peek_byte();
    }

#endif

    /**
     *  Returns true if the bytevector encountered an attempt to get data
     *  past the end of the byte-vector.  Kind of indicates an end-of-file.
     *  Raised by get(), but not peek().
     */

    bool fatal_error () const
    {
        return m_data.fatal_error();
    }

    /**
     *  Seeks to a new, absolute, position in the data stream.  All this function
     *  does is change the value of m_position.  All of the data is already in
     *  memory.
     *
     * \param pos
     *      Provides the new position to seek.
     *
     * \return
     *      Returns true if the seek could be accomplished.  No error message is
     *      logged, but the caller should take evasive action if false is
     *      returned.  And, in that case, m_position is unchanged.
     */

    bool seek (size_t pos)
    {
        return m_data.seek(pos);
    }

    midi::ushort get_short ()
    {
        return m_data.get_short();
    }

    midi::ulong get_long ()
    {
        return m_data.get_long();
    }

#if defined RTL66_PROVIDE_EXTRA_GET_FUNCTIONS

    midi::status get_status () const
    {
        return midi::to_status(get());
    }

    size_t get_array (midi::byte * b, size_t len);
    size_t get_array (midi::bytes & b, size_t len);
    bool get_string (std::string & b, size_t len);
    bool get_meta (midi::event & e, midi::meta metatype, size_t len);
    bool get_seqspec (midi::ulong spec, int len);
    std::string get_track_name ();
    std::string get_meta_text ();
    int get_track_number ();
    bool get_tempo (midi::tempoinfo & destination);
    bool get_time_signature (midi::timesiginfo & destination);
    bool get_key_signature (midi::keysiginfo & destination);
    midi::ulong get_split_long (unsigned & highbytes, unsigned & lowbytes);
    bool get_gap (size_t sz);

#endif  // RTL66_PROVIDE_EXTRA_GET_FUNCTIONS

    /**
     *  Read a MIDI Variable-Length Value (VLV), which has a variable number
     *  of bytes.  This function reads the bytes while bit 7 is set in each
     *  byte.  Bit 7 is a continuation bit.  See write_varinum() for more
     *  information.
     *
     * \return
     *      Returns the accumulated values as a single number.
     */

    midi::ulong get_varinum ()
    {
        return m_data.get_varinum();
    }

    bool checklen (midi::ulong len, midi::byte type);

    bool checklen (midi::ulong len, midi::meta type)
    {
        return checklen(len, midi::to_byte(type));
    }

    /*---------------------------------------------------------------------
     * "put" functions
     *---------------------------------------------------------------------*/

    void put (midi::byte b)
    {
        m_data.put_byte(b);
    }

    /**
     *  Writes a MIDI Variable-Length Value (VLV), which has a variable number
     *  of bytes. A MIDI file Variable Length Value is stored in bytes.
     *  See the function varinum_to_bytes() in the calculations module.
     *
     * \param v
     *      The data value to be added to the current event in the MIDI
     *      container.
     */

    void put_varinum (midi::ulong v)
    {
        m_data.put_varinum(v);
    }

    /**
     *  Adds a short value (two bytes) to the container.
     *
     * \param x
     *      Provides the timestamp (pulse value) to be added to the container.
     */

    void put_short (midi::ushort x)
    {
        m_data.put_short(x);
    }

    /**
     *  Writes 3 bytes, each extracted from the long value and shifted rightward
     *  down to byte size, using the write_byte() function.
     *
     *  This function is kind of the reverse of tempo_us_to_bytes() defined in the
     *  calculations.cpp module. In Seq66 it can be used in seq66 :: midifile ::
     *  write_start_tempo().
     *
     * \warning
     *      This code looks endian-dependent.
     *
     * \param x
     *      The long value to be written to the MIDI file.
     */

    void put_triple (midi::ulong x)
    {
        m_data.put_triple(x);
    }

    /**
     *  Adds a long value (a MIDI pulse/tick value) to the container.
     *
     *  What is the difference between this function and put_list_var()?
     *  This function "replaces" sequence::put_long_list().
     *  This was a <i> global </i> internal function called addLongList().
     *  Let's at least make it a private member now, and hew to the naming
     *  conventions of this class.
     *
     * \param x
     *      Provides the timestamp (pulse value) to be added to the container.
     */

    void put_long (midi::ulong x)
    {
        m_data.put_long(x);
    }

    void put_channel_event (const event & e, midi::pulse deltatime);
    void put_ex_event (const event & e, midi::pulse deltatime);
    void put_meta_header
    (
        midi::meta value, int datalen,
        midi::pulse deltatime = 0
    );
    void put_meta
    (
        midi::meta value,
        const midi::bytes & data,
        midi::pulse deltatime = 0
    );

#if defined RTL66_PROVIDE_EXTRA_PUT_FUNCTIONS
    void put_start_tempo (midi::bpm start_tempo);
    void put_meta_text
    (
        midi::meta metacode,
        const std::string & text
    );
    void put_track_end ();
    void put_meta_track_end (midi::pulse deltatime);
#endif

    bool put_track_events (/*const*/ track & trk);
    bool put_track
    (
        /*const*/ track & trk,
        int tempotrack = 0,
        bool doseqspec = true
    );
    void put_seqspec (midi::ulong spec, int datalen);
    void put_seqspec_code (midi::ulong spec, int datalen);
    void put_seqspec_data (midi::ulong spec, const midi::bytes & data);
    void put_track_number (int trkno);
    void put_track_name (const std::string & name);
    void put_time_sig
    (
        int bpb, int beatwidth,
        int cpm, int get32pq
    );
    void put_time_sig (const midi::timesiginfo & tsi);
    void put_tempo (int usperqn);
    void put_key_sig (int sf, bool mf);
    void put_key_sig (const midi::keysiginfo & ksi);

    /*---------------------------------------------------------------------
     * "size" functions
     *---------------------------------------------------------------------*/

    long seqspec_item_size (long data_length) const;

    /**
     *  Returns the size of a track-number event, which is always 5
     *  bytes, plus one byte for the delta time that precedes it.
     */

    long seq_number_size () const
    {
        return 6;
    }

    /**
     *  Returns the size of a track-end event, which is always 3 bytes.
     */

    long track_end_size () const
    {
        return 3;
    }

protected:

    /*---------------------------------------------------------------------
     * "extract" functions
     *---------------------------------------------------------------------*/

    virtual bool extract_seq_spec (track & trk, event & e, size_t len)
    {
        return extract_generic_meta(trk, e, midi::meta::seq_spec, len);
    }

    bool extract_meta_msg (track & trk, event & e);
    bool extract_generic_meta
    (
        track & trk, event & e,
        midi::meta metatype, size_t len
    );
    bool extract_track_number (track & trk, event & e, size_t len);
    bool extract_track_name (track & trk, event & e, size_t len);
    bool extract_text_event
    (
        track & trk, event & e,
        midi::meta metatype, size_t len
    );
    bool extract_end_of_track (track & trk, event & e);
    bool extract_tempo (track & trk, event & e);
    bool extract_time_signature (track & trk, event & e);
    bool extract_key_signature (track & trk, event & e);
    size_t parse_track              /* put_track() is the "inverse" */
    (
        track & trk,
        const util::bytevector & data,
        size_t offset, size_t len
    );

    eventlist & events ()
    {
        return m_events;
    }

    const eventlist & events () const
    {
        return m_events;
    }

    bool append_event (const event & e)
    {
        return events().append(e);
    }

    void reset_position () const
    {
        m_data.reset();
    }

    size_t position () const;
    size_t real_position () const;

    void skip (size_t sz)
    {
        m_data.skip(sz);
    }

    midi::bytes end_of_track () const
    {
        static midi::bytes s_bytes{0xff, 0x2f, 0x00};
        return s_bytes;
    }

};          // class trackdata

}           // namespace midi

#endif      // RTL66_MIDI_TRACKDATA_HPP

/*
 * trackdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

