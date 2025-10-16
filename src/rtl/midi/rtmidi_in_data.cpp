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
 * \updates       2025-10-10
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

/**
 *  Default constructor. Most members are initialized "in-class".
 *
 * \param qsize
 *      The size of the input queue. If 0 (the default), the queue
 *      remains unallocated.
 */

rtmidi_in_data::rtmidi_in_data (unsigned qsize) :
    m_queue             (qsize)
{
    // no code necessary
}

void
rtmidi_in_data::ignore_flags (bool sysex, bool time, bool sense)
{
    m_ignore_flags = ignoreflag::allow_all;
    if (sysex)
        m_ignore_flags |= ignoreflag::sysex;

    if (time)
        m_ignore_flags |= ignoreflag::time_code;

    if (sense)
        m_ignore_flags |= ignoreflag::active_sensing;
}

}           // namespace rtl

/*
 * rtmidi_in_data.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

