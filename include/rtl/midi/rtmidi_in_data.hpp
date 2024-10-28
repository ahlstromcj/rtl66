#if ! defined RTL66_RTL_MIDI_RTMIDI_IN_DATA_HPP
#define RTL66_RTL_MIDI_RTMIDI_IN_DATA_HPP

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
 * \file          rtmidi_in_data.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2023-07-19
 * \license       See above.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition, and slightly easier to read.
 */

#include "c_macros.h"                   /* not_nullptr() and friends        */

#if defined __cplusplus                 /* do not expose this to C code     */

#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/midi/midi_queue.hpp"      /* rtl::midi_queue class            */

namespace rtl
{

/**
 *  The rtmidi_in_data structure is used to pass private class data to the
 *  MIDI input handling function or thread.  Used to be nested in the
 *  rtl66_in class.
 */

class RTL66_DLL_PUBLIC rtmidi_in_data
{
    enum flags : unsigned char
    {
        flag_sysex          = 0x01,
        flag_time_code      = 0x02,
        flag_active_sensing = 0x04,
        flag_ignore_all     = 0x07
    };

public:

    /**
     *  User callback function type definition.  Such a callback is needed
     *  only for input.
     */

    using callback_t = void (*)
    (
        double timestamp,
        midi::message * message,
        void * userdata
    );

private:

    /**
     *  Provides a queue of MIDI messages. Used when not using a JACK callback
     *  for MIDI input.
     */

    midi_queue m_queue;

    /**
     *  NEW in RtMidi.
     *  This is used to hold the latest message.
     */

    midi::message m_message;

    /**
     *  A one-time flag that starts out true and is falsified when the first
     *  MIDI messages comes in to this port.  It simply resets the delta JACK
     *  time.
     */

    bool m_first_message;

    /**
     *  Indicates that SysEx is still coming in.
     */

    bool m_continue_sysex;

    /*
     *  A ton of "new" stuff from RtMidi....
     */

    unsigned char m_ignore_flags;
    bool m_do_input;
    void * m_api_data;
    bool m_using_callback;
    callback_t m_user_callback;
    void * m_user_data;
    size_t m_buffer_size;
    int m_buffer_count;

public:

    rtmidi_in_data ();

    const midi_queue & queue () const
    {
        return m_queue;
    }

    midi_queue & queue ()
    {
        return m_queue;
    }

    const midi::message & message () const
    {
        return m_message;
    }

    midi::message & message ()
    {
        return m_message;
    }

    bool first_message () const
    {
        return m_first_message;
    }

    void first_message (bool flag)
    {
        m_first_message = flag;
    }

    bool continue_sysex () const
    {
        return m_continue_sysex;
    }

    void continue_sysex (bool flag)
    {
        m_continue_sysex = flag;
    }

    /*
     * New stuff follows.
     */

    void ignore_flags (bool sysex, bool time, bool sense);

    bool do_input () const
    {
        return m_do_input;
    }

    void do_input (bool flag)
    {
        m_do_input = flag;
    }

    void * api_data ()
    {
        return m_api_data;
    }

    void api_data (void * dp)
    {
        m_api_data = dp;
    }

    bool using_callback () const
    {
        return m_using_callback;
    }

    callback_t user_callback ()
    {
        return m_user_callback;
    }

    void user_callback (callback_t cb, void * ud)
    {
        m_user_callback = cb;
        using_callback(not_nullptr(cb));
        user_data(ud);
    }

    void clear_callback ()
    {
        user_callback(nullptr, nullptr);
    }

    void * user_data ()
    {
        return m_user_data;
    }

    size_t buffer_size () const
    {
        return m_buffer_size;
    }

    int buffer_count () const
    {
        return m_buffer_count;
    }

    void set_buffer_size (size_t sz, int count)
    {
        m_buffer_size = sz;
        m_buffer_count = count;
    }

    bool allow_sysex () const
    {
        return (m_ignore_flags & flag_sysex) == 0;
    }

    bool allow_time_code () const
    {
        return (m_ignore_flags & flag_time_code) == 0;
    }

    bool allow_active_sensing () const
    {
        return (m_ignore_flags & flag_active_sensing) == 0;
    }

    void allow_sysex (bool flag)
    {
        if (flag)
            m_ignore_flags |= flag_sysex;
        else
            m_ignore_flags &= ~flag_sysex;
    }

    void allow_time_code (bool flag)
    {
        if (flag)
            m_ignore_flags |= flag_time_code;
        else
            m_ignore_flags &= ~flag_time_code;
    }

    void allow_active_sensing (bool flag)
    {
        if (flag)
            m_ignore_flags |= flag_active_sensing;
        else
            m_ignore_flags &= ~flag_active_sensing;
    }

private:

    void buffer_size (size_t sz)
    {
        m_buffer_size = sz;
    }

    void buffer_count (int count)
    {
        m_buffer_count = count;
    }

    unsigned char ignore_flags () const
    {
        return m_ignore_flags;
    }

    void ignore_flags (unsigned char flagset)
    {
        m_ignore_flags = flagset;
    }

    void using_callback (bool flag)
    {
        m_using_callback = flag;
    }

    void user_data (void * ud)
    {
        m_user_data = ud;
    }

};              // class rtmidi_in_data

}               // namespace rtl

#endif          // defined __cplusplus : do not expose to C code

#endif          // RTL66_RTL_MIDI_RTMIDI_IN_DATA_HPP

/*
 * rtmidi_in_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

