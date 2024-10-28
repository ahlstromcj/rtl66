#if ! defined RTL66_TRANSPORT_CLOCK_INFO_HPP
#define RTL66_TRANSPORT_CLOCK_INFO_HPP

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
 * \file          transport/clock/info.hpp
 *
 *    Object for holding the current status of MIDI clock data.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2022-11-17
 * \updates       2022-12-01
 * \license       See above.
 *
 */

#include "midi/midibytes.hpp"           /* midi::ppqn and midi::pulse       */

namespace transport
{

namespace clock
{

/**
 *  Encapsulates MIDI clock status and information.
 */

class info
{

private:

    /**
     *  MIDI clock support.
     */

    bool m_usemidiclock;

    /**
     *  MIDI clock support.  Indicates if the MIDI clock is stopped or
     *  started.
     */

    bool m_midiclockrunning;

    /**
     *  MIDI clock support.
     */

    int m_midiclocktick;

    /**
     *  We need to adjust the clock increment for the PPQN that is in force.
     *  Higher PPQN need a longer increment than 8 in order to get 24 clocks
     *  per quarter note.
     */

    int m_midiclockincrement;

    /**
     *  MIDI clock support.
     */

    int m_midiclockpos;

public:

    info (midi::ppqn ppq = 0);
    info (const info &) = default;
    info & operator = (const info &) = default;
    ~info () = default;

    long adjust_midi_tick ();
    void clock_start ();
    void clock_continue (midi::pulse tick);
    void clock_stop (midi::pulse tick);
    void clock_increment ();

public:

    bool usemidiclock () const
    {
        return m_usemidiclock;
    }

    bool midiclockrunning () const
    {
        return m_midiclockrunning;
    }

    int midiclocktick () const
    {
        return m_midiclocktick;
    }

    int midiclockincrement () const
    {
        return m_midiclockincrement;
    }

    int midiclockpos () const
    {
        return m_midiclockpos;
    }

    void usemidiclock (bool flag)
    {
        m_usemidiclock = flag;
    }

    void midiclockrunning (bool flag)
    {
        m_midiclockrunning = flag;
    }

    void midiclocktick (int t)
    {
        m_midiclocktick = t;
    }

    void midiclockincrement (int i)
    {
        m_midiclockincrement = i;
    }

    void midiclockpos (int p)
    {
        m_midiclockpos = p;
    }

};          // class info

}           // namespace clock

}           // namespace transport

#endif      // RTL66_TRANSPORT_CLOCK_INFO_HPP

/*
 * transport/clock/info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

