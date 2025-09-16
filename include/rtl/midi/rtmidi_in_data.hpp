#if ! defined RTL66_RTL_RTMIDI_IN_DATA_HPP
#define RTL66_RTL_RTMIDI_IN_DATA_HPP

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
 * \updates       2025-09-14
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
 *  Maximum expected incoming message size.
 *
 *  For APIs that require manual buffer management, it can be useful to set
 *  the buffer size and buffer count when expecting to receive large SysEx
 *  messages. Setting this value has no effect when called after open_port().
 *  The default buffer size is 1024 with a count of 4 buffers, which should
 *  be sufficient for most cases; as mentioned, this does not affect all API
 *  backends, since most either support dynamically scalable buffers or take
 *  care of buffer handling themselves. It is principally intended for users
 *  of the Windows MM backend who must support receiving especially large
 *  messages.
 */

const size_t c_buffer_size_max { 256 }; /* was 1024 as noted above          */
const size_t c_buffer_count    {   4 }; /* as noted above                   */

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

    midi_queue m_queue { };

    /**
     *  This is used to hold the latest MIDI message.
     *  In RtMidi, this is the MidiMessage class.
     */

    midi::message m_message { };

    /**
     *  A one-time flag that starts out true and is falsified when the first
     *  MIDI messages comes in to this port. It simply resets the delta JACK
     *  time.
     */

    bool m_first_message { true };

    /**
     *  Indicates that SysEx is still coming in.
     */

    bool m_continue_sysex { false };

    /*
     *  A ton of "new" stuff from RtMidi....
     */

    /**
     *  Provides a set of bits to indicate to ignore certain MIDI messages
     *  upon input:
     *
     *      -   sysex
     *      -   time_code
     *      -   active_sensing
     *      -   All of them.
     */

    unsigned char m_ignore_flags { flag_ignore_all };

    /**
     *  This boolean is used in midi_alsa_handler(), for example. If not
     *  yet true when opening a regular or virtual input port, it is set to
     *  true in open_port(), just before starting the input thread.
     *
     *  While true, the handler loops, checking for pending input.
     *  If a fatal error occurs in the handler, it is set to false.
     *
     *  In the midi_alsa destructor,it is set to false after close_port(),
     *  and before joining the input thread.
     */

    bool m_do_input { false };

    /**
     *  Points to the midi_api-derived object representing the input port.
     *  Note that the derived class will provide a function [such as
     *  midi_alsa::alsa_client()] that can be accessed indirectly through
     *  this pointer to provide the global application client handle, or
     *  the client handle of a stand-alone port.
     */

    void * m_api_data { nullptr };

    /**
     *  Indicates if we're using a callback function to handle input.
     */

    bool m_using_callback { false };

    /**
     *  The input callback fuction. See the midi_api functions
     *  set_input_callback() and cance_input_callback.
     */

    callback_t m_user_callback { nullptr };

    /**
     *  This member points to a class-specific data structure that is passed
     *  to the user callback function.
     */

    void * m_user_data { nullptr };

    /**
     *  The size of the input-data buffer. It is also copied to the
     *  midi_alsa_data class (for example) in the initialize() function.
     *  That structure also uses the size to initialize an ALSA event parser
     *  (known as "coder" in RtMidi).
     */

    size_t m_buffer_size { c_buffer_size_max };

    /**
     *  The number of buffers, used to allocate and initialize the SysEx
     *  buffers in the Windows MIDI API only.
     */

    int m_buffer_count { c_buffer_count };

public:

    rtmidi_in_data (unsigned qsize = RTL66_DEFAULT_Q_SIZE);
    rtmidi_in_data (const rtmidi_in_data &) = delete;
    rtmidi_in_data (rtmidi_in_data &&) = default;
    rtmidi_in_data & operator = (const rtmidi_in_data &) = delete;
    rtmidi_in_data & operator = (rtmidi_in_data &&) = default;

    const midi_queue & queue () const
    {
        return m_queue;
    }

    midi_queue & queue ()
    {
        return m_queue;
    }

    const midi::message & midi_msg () const
    {
        return m_message;
    }

    midi::message & midi_msg ()
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

#endif          // RTL66_RTL_RTMIDI_IN_DATA_HPP

/*
 * rtmidi_in_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

