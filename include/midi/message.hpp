#if ! defined RTL66_MIDI_MESSAGE_HPP
#define RTL66_MIDI_MESSAGE_HPP

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
 * \file          message.hpp
 *
 *      A type for holding the rew data of a MIDI message.
 *
 * \library       rtl66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2024-05-26
 * \license       See above.
 *
 *  Should we add operator [] for setting as well?
 *
 *  See the cpp module for more information.
 */

#include <vector>                       /* std::vector container            */

#include "midi/eventcodes.hpp"          /* midi::byte, event status codes   */

namespace midi
{

/**
 *  Provides a handy capsule for a MIDI message, based on the
 *  std::vector<unsigned char> data type from the RtMidi project.
 *  See the cpp file's banner.
 */

class message
{

public:

    /**
     *  Holds the data of the MIDI message.  Callers should use
     *  message::container rather than using the vector directly.
     *  Bytes are added by the push() function, and are safely accessed
     *  (with bounds-checking) by operator [].
     *
     *      using container = std::vector<midi::byte>;
     */

    using container = midi::bytes;

private:

#if defined RTL66_PLATFORM_DEBUG

    /**
     *  Provide a static counter to keep track of events. Currently needed for
     *  trouble-shooting.  We don't care about wraparound.
     */

    static unsigned sm_msg_number;

    /**
     *  Provides the message counter value when this event was created.
     */

    unsigned m_msg_number;

#endif

    /**
     *  Holds the event status, length (for events supporting that)
     *  and data bytes.
     */

    container m_bytes;

#if defined RTL66_USE_MESSAGE_HEADER_SIZE

    /**
     *  Indicates the number of bytes devoted to status, meta-type (if
     *  applicable), and the varinum length. Useful in accessing the
     *  data itself. For non-meta, non-sysex events, this should be 0.
     */

    size_t m_header_size;

#endif

    /**
     *  Holds the (optional) timestamp of the MIDI message.  Holds the
     *  timestamp of the MIDI message. Non-zero only in the JACK
     *  implementation at present.  It can also hold a JACK frame number. The
     *  caller can know this only by context at present.
     */

    double m_time_stamp;

public:

    message (double ts = 0.0);
    message (const midi::byte * mbs, std::size_t sz);
    message (const midi::bytes & mbs);
    message (const message & rhs) = default;
    message (message &&) = default;
    message & operator = (const message & rhs) = default;
    message & operator = (message &&) = default;
    ~message () = default;

    /**
     *  Unlike std::map::operator [], the vector version never adds an entry
     *  if the index/key is out of range.  The behavior is supposed to be
     *  undefined, but here we simply return a 0 byte if the index is bad.
     */

    midi::byte & operator [] (size_t i)
    {
        static midi::byte s_zero = 0;
        return i < m_bytes.size() ? m_bytes[i] : s_zero ;
    }

    const midi::byte & operator [] (size_t i) const
    {
        static midi::byte s_zero = 0;
        return i < m_bytes.size() ? m_bytes[i] : s_zero ;
    }

#if defined RTL66_USE_MESSAGE_HEADER_SIZE

    /**
     *  The next two functions are useful in sending MIDI data.
     */

    const midi::byte * data_bytes () const
    {
        return m_bytes.data() + size_t(header_size());
    }

    int data_byte_count () const
    {
        return int(m_bytes.size() - header_size());
    }

    size_t header_size () const
    {
        return m_header_size;
    }

    /**
     *  Call this when getting meta/sysex data bytes.
     */

    void log_header_size ()
    {
        m_header_size = m_bytes.size();
    }

#endif

    midi::bytes & event_bytes ()
    {
        return m_bytes;
    }

    const midi::bytes & event_bytes () const
    {
        return m_bytes;
    }

    const midi::byte * data_ptr () const
    {
        return m_bytes.data();
    }

    size_t size () const
    {
        return m_bytes.size();
    }

    size_t event_byte_count () const
    {
        return m_bytes.size();
    }

#if defined RTL66_PLATFORM_DEBUG

    unsigned msg_number () const
    {
        return m_msg_number;
    }

#endif

    void clear ()
    {
        m_bytes.clear();
    }

    bool empty () const
    {
        return m_bytes.empty();
    }

    void push (midi::byte b)
    {
        m_bytes.push_back(b);
    }

    void push (midi::status s)
    {
        midi::byte b = midi::to_byte(s);
        m_bytes.push_back(b);
    }

    void push (midi::meta m)
    {
        midi::byte b = midi::to_byte(m);
        m_bytes.push_back(b);
    }

    void push (midi::ctrl c)
    {
        midi::byte b = midi::to_byte(c);
        m_bytes.push_back(b);
    }

    /*
     *  Beginning and ending must point to the same buffer for these next
     *  two functions.
     *
     *  They are used only in midi_alsa.cpp, though.
     */

    void assign (midi::byte * beginning, midi::byte * ending)
    {
        m_bytes.assign(beginning, ending);
    }

    void append (midi::byte * beginning, midi::byte * ending)
    {
        m_bytes.insert(m_bytes.end(), beginning, ending);
    }

    void resize (int len)
    {
        m_bytes.resize(size_t(len));
    }

    midi::byte front () const
    {
        return m_bytes.front();
    }

    midi::byte back () const
    {
        return m_bytes.back();
    }

    midi::pulse time_stamp () const
    {
        return midi::pulse(m_time_stamp);
    }

    double jack_stamp () const
    {
        return m_time_stamp;
    }

    void jack_stamp (double t)
    {
        m_time_stamp = t;
    }

    bool is_sysex () const
    {
        return m_bytes.size() > 0 ? midi::is_sysex_msg(m_bytes[0]) : false ;
    }

    midi::byte status () const
    {
        return event_byte_count() > 0 ? m_bytes[0] : 0 ;
    }

    std::string to_string () const;

};          // class message

/**
 *  MIDI caller callback function type definition.  The timestamp parameter
 *  has been folded into the message class (a wrapper for
 *  std::vector<unsigned char>), and the pointer has been replaced by a
 *  reference.
 */

using rt_midi_callback_t = void (*)
(
    message & message,             /* includes the timestamp already */
    void * userdata
);

}           // namespace midi

#endif      // RTL66_MIDI_MESSAGE_HPP

/*
 * message.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

