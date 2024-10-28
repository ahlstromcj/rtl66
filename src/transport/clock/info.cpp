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
 * \file          transport/clock/info.cpp
 *
 *    Object for holding the current status of MIDI Clock data.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2022-11-17
 * \updates       2022-12-01
 * \license       See above.
 *
 */

#include "midi/calculations.hpp"        /* clock_ticks_from_ppqn()          */
#include "transport/clock/info.hpp"     /* transport::clock::info()         */

namespace transport
{

namespace clock
{

/**
 * \ctor info
 */

info::info (midi::ppqn ppq) :
    m_usemidiclock          (false),
    m_midiclockrunning      (false),
    m_midiclocktick         (0),
    m_midiclockincrement    (midi::clock_ticks_from_ppqn(ppq)),
    m_midiclockpos          (0)
{
    // Empty body
}

long
info::adjust_midi_tick ()
{
    long result = 0;
    if (m_usemidiclock)
    {
        result = m_midiclocktick;           /* int to long          */
        m_midiclocktick = 0;
        if (m_midiclockpos >= 0)            /* was after this if    */
        {
            result = 0;

            /*
             * TODO: FIXME and FIGGERTHIS OUT
             * pad().set_current_tick(midi::pulse(m_midiclockpos));
             */
            m_midiclockpos = -1;
        }
    }
    return result;
}

void
info::clock_start ()
{
    m_midiclockrunning = m_usemidiclock = true;
    m_midiclocktick = m_midiclockpos = 0;
}

void
info::clock_continue (midi::pulse tick)
{
    m_midiclockpos = tick;
    m_midiclockrunning = m_usemidiclock = true;
}

void
info::clock_stop (midi::pulse tick)
{
    m_usemidiclock = true;
    m_midiclockrunning = false;
    m_midiclockpos = tick;
}

void
info::clock_increment ()
{
    if (m_midiclockrunning)
        m_midiclocktick += m_midiclockincrement;
}

}           // namespace clock

}           // namespace transport

/*
 * transport/clock/info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

