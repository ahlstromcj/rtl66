#if ! defined RTL66_TRACKDATA_HPP
#define RTL66_TRACKDATA_HPP

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
 *  This module declares a class for managing raw MIDI events in a
 *  sequence/track.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2022-11-19
 * \license       GNU GPLv2 or above
 *
 *  This class is meant to hold the bytes that represent MIDI events and other
 *  MIDI data, which can then be dumped to a MIDI file.
 */

#include <string>                       /* std::string class                */

#include "midi/midibytes.hpp"           /* midi::byte, midi::bytes, etc.    */

namespace midi
{
    class event;

/**
 *    This class is the abstract base class for a container of MIDI track
 *    information.  It is the base class for midi_list and midi_vector.
 */

class trackdata
{
    //  friend class midi::file;

private:

    /**
     *  Provides a container for the raw MIDI bytes of a track.
     */

    bytes m_bytes;

    /**
     *  Provide a hook into a sequence so that we can exchange data with a
     *  sequence object.
     */

    sequence & m_sequence;

    /**
     *  Provides the position in the container when making a series of get()
     *  calls on the container.
     */

    mutable unsigned m_position_for_get;

public:

    trackdata (sequence & seq);

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~trackdata ()
    {
        // empty body
    }

    bool song_fill_track (int track, bool standalone = true);
    void fill (int tracknumber, const performer & p, bool doseqspec = true);

    /**
     *  Returns the size of the container, in midi::bytes.
     */

    size_t size () const
    {
        return m_bytes.size();
    }

    void clear ()
    {
        m_bytes.clear();
    }

    /**
     *  Instead of checking for the size of the container when "emptying" it
     *  [see the midi::file::write() function], use this function, which is
     *  overridden to match the type of container being used.
     */

    bool done () const
    {
        return position() >= size();
    }

    /**
     *  Provide a way to get the next byte from the container.  In this
     *  implementation, m_position_for_get is used.  As a side-effect, the
     *  position value is incremented.
     *
     * \return
     *      Returns the next byte in the character vector.
     */

    virtual midi::byte get () const
    {
        midi::byte result = m_bytes[position()];
        position_increment();
        return result;
    }

    /**
     *  Provides a way to add a MIDI byte into the list.
     */

    void put (midi::byte b)
    {
        m_bytes.push_back(b);
    }

    /**
     *  Combines a number of put() calls.  It puts the preamble for a MIDI Meta
     *  event.  After this function is called, the call then puts() the actual
     *  data.  Note that the data-length is assumed to fit into a midi::byte (255
     *  maximum).
     */

    void put_meta (midi::byte metavalue, int datalen, midi::pulse deltatime = 0);
    void put_seqspec (midi::long spec, int datalen);

    /**
     *  Provide a way to get the next byte from the container.  It also
     *  increments m_position_for_get.
     */

    virtual midi::byte get () const = 0;

    /**
     *  Provides a way to clear the container.
     */

    virtual void clear () = 0;

protected:

    sequence & seq ()
    {
        return m_sequence;
    }

    /**
     *  Sets the position to 0 and then returns that value. So far, it is not
     *  used, because we create a new midi_vector for each write_track()
     *  call.
     */

    unsigned position_reset () const
    {
        m_position_for_get = 0;
        return m_position_for_get;
    }

    /**
     * \getter m_position_for_get
     *      Returns the current position.
     */

    unsigned position () const
    {
        return m_position_for_get;
    }

    /**
     * \getter m_position_for_get
     *      Increments the current position.
     */

    void position_increment () const
    {
        ++m_position_for_get;
    }

private:

    void add_byte (midi::byte b)
    {
        put(b);
    }

    void add_varinum (midi::long v);
    void add_long (midi::long x);
    void add_short (midi::short x);
    void add_event (const event & e, midi::pulse deltatime);
    void add_ex_event (const event & e, midi::pulse deltatime);
    void fill_meta_track_end (midi::pulse deltatime);
    void fill_seqspec ();

protected:

    void fill_seq_number (int seq);
    void fill_seq_name (const std::string & name);

#if defined SEQ_USE_FILL_TIME_SIG_AND_TEMPO
    void fill_time_sig_and_tempo
    (
        const performer & p,
        bool has_time_sig = false,
        bool has_tempo    = false
    );
    void fill_time_sig (const performer & p);
    void fill_tempo (const performer & p);
#endif

    midi::pulse song_fill_seq_event
    (
        const trigger & trig, midi::pulse prev_timestamp
    );
    void song_fill_seq_trigger
    (
        const trigger & trig, midi::pulse len, midi::pulse prev_timestamp
    );

};          // class trackdata

}           // namespace midi

#endif      // RTL66_TRACKDATA_HPP

/*
 * track.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

