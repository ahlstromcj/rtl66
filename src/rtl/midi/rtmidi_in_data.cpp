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
 * \file          rtmidi_in_data.cpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2023-03-06
 * \license       See above.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition (avoiding header madness), and slightly easier to
 *  read.
 */

#include "rtl/midi/rtmidi_in_data.hpp"  /* rtl::rtmidi_in_data class        */

namespace rtl
{

rtmidi_in_data::rtmidi_in_data () :
    m_queue             (),
    m_message           (),
    m_first_message     (true),
    m_continue_sysex    (false),
    m_ignore_flags      (flag_ignore_all),
    m_do_input          (false),
    m_api_data          (nullptr),
    m_using_callback    (false),
    m_user_callback     (nullptr),
    m_user_data         (nullptr),
    m_buffer_size       (1024),
    m_buffer_count      (4)
{
    // No code
}

void
rtmidi_in_data::ignore_flags (bool sysex, bool time, bool sense)
{
    m_ignore_flags = 0;
    if (sysex)
        m_ignore_flags |= flag_sysex;

    if (time)
        m_ignore_flags |= flag_time_code;

    if (sense)
        m_ignore_flags |= flag_active_sensing;
}

}           // namespace rtl

/*
 * rtmidi_in_data.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

