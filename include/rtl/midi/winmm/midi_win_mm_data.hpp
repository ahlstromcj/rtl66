#if ! defined RTL66_RTL_MIDI_WIN_MM_DATA_HPP
#define RTL66_RTL_MIDI_WIN_MM_DATA_HPP

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
 * \file          midi_win_mm_data.hpp
 *
 *    Object for holding the current status of WIN_MM and WIN_MM MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-08-02
 * \updates       2023-07-19
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_WIN_MM

#include <windows.h>                    /* general Win(32) items            */
#include <mmsystem.h>                   /* Windows MM MIDI header files     */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "midi/message.hpp"             /* midi::message class              */
#include "midi/midibytes.hpp"           /* midi::byte, other aliases        */

namespace rtl
{

/**
 *
 * Old name: struct WinMidiData
 *
 *  Contains the WIN_MM MIDI API data as a kind of scratchpad for this object.
 *
 *  A structure to hold variables related to the CoreMIDI API
 *  implementation.
 *
 *  [Patrice] See
 *
 *      https://groups.google.com/forum/#!topic/mididev/6OUjHutMpEo
 */

class RTL66_DLL_PUBLIC midi_win_mm_data
{

    friend class midi_win_mm;

    /*
     * The MIDIHDR structure defines the header used to identify a MIDI
     * system-exclusive or stream buffer.
     */

    using header = std::vector<LPMIDIHDR>;

private:

    HMIDIIN m_input_handle;                 // handle to MIDI input device
    HMIDIOUT m_output_handle;               // handle to MIDI output device
    DWORD m_last_time;
    midi::message m_midi_message;
    header m_sysex_buffer;
    CRITICAL_SECTION m_mutex;               // see note above

public:

    midi_win_mm_data ();

    /**
     *  This destructor currently does nothing.  We rely on the enclosing
     *  class to close out the things that it created.
     */

    ~midi_win_mm_data ()
    {
        // Empty body
    }

    HMIDIIN & in_client ()
    {
        return m_input_handle;
    }

    HMIDIOUT & out_client ()
    {
        return m_output_handle;
    }

#if 0
    int port_number () const
    {
        return m_portnum;
    }

    size_t buffer_size () const
    {
        return m_buffer_size;
    }

    midi::byte * buffer ()
    {
        return m_buffer;
    }

    bool valid_buffer () const
    {
        return not_nullptr(m_buffer);
    }
#endif

public:

    void in_client (HMIDIIN hin)
    {
        m_input_handle = hin;
    }

    void out_client (HMIDIOUT hout)
    {
        m_output_handle = hout;
    }

#if 0
    void port_number (int p)
    {
        m_portnum = p;
    }

    void buffer_size (size_t sz)
    {
        m_buffer_size = sz;
    }

    void buffer (midi::byte * b)
    {
        m_buffer = b;
    }
#endif

    void last_time (DWORD lt)
    {
        m_last_time = lt;
    }

};          // class midi_win_mm_data

}           // namespace rtl

#endif      // RTL66_BUILD_WIN_MM

#endif      // RTL66_RTL_MIDI_WIN_MM_DATA_HPP

/*
 * midi_win_mm_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

