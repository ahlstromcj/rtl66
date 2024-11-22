#if ! defined RTL66_MIDI_MEASURES_HPP
#define RTL66_MIDI_MEASURES_HPP

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
 * \file          measures.hpp
 *
 *  This module declares a class for holding measure information.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2024-11-22
 * \license       GNU GPLv2 or above
 *
 */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace midi
{

/**
 *  Provides a data structure to hold the numeric equivalent of the measures
 *  string "measures:beats:divisions" ("m:b:d").  More commonly known as
 *  "bars:beats:ticks", or "BBT".
 */

class measures
{

private:

    /**
     *  The integral number of measures in the measures-based time.
     */

    int m_bars;

    /**
     *  The integral number of beats in the measures-based time.
     */

    int m_beats;

    /**
     *  The integral number of divisions/pulses in the measures-based time.
     *  There are two possible translations of the two bytes of a division. If
     *  the top bit of the 16 bits is 0, then the time division is in "ticks
     *  per beat" (or “pulses per quarter note”). If the top bit is 1, then
     *  the time division is in "frames per second".  This member deals only
     *  with the ticks/beat definition.
     */

    int m_divisions;

public:

    measures () = default;

    measures (int bars, int beats, int divisions) :
        m_bars      (bars),
        m_beats     (beats),
        m_divisions (divisions)
    {
        // No code
    }

    measures (const measures &) = default;
    measures (measures &&) = default;
    measures & operator = (const measures &) = default;
    measures & operator = (measures &&) = default;

    void clear ()
    {
        m_bars = m_beats = m_divisions = 0;
    }

    int bars () const
    {
        return m_bars;
    }

    /**
     * \param m
     *      The value to which to set the number of measures.
     *      We can add validation later.
     */

    void bars (int m)
    {
        m_bars = m;
    }

    int beats () const
    {
        return m_beats;
    }

    /**
     * \param b
     *      The value to which to set the number of beats.
     *      We can add validation later.
     */

    void beats (int b)
    {
        m_beats = b;
    }

    int divisions () const
    {
        return m_divisions;
    }

    midi::pulse ticks () const
    {
        return midi::pulse(m_divisions);
    }

    /**
     * \param d
     *      The value to which to set the number of divisions.
     *      We can add validation later.
     */

    void divisions (int d)
    {
        m_divisions = d;
    }

};              // class measures

}               // namespace midi

#endif          // RTL66_MIDI_MEASURES_HPP

/*
 * measures.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

