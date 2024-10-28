#if ! defined RTL66_RTL_MIDI_MIDI_QUEUE_HPP
#define RTL66_RTL_MIDI_MIDI_QUEUE_HPP

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
 * \file          midi_queue.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2024-02-03
 * \license       See above.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition, and slightly easier to read.
 */

#include "midi/message.hpp"             /* midi::message class              */
#include "rtl/rtl_build_macros.h"       /* RTL66_DLL_PUBLIC, etc.           */

namespace rtl
{

/**
 *  Default size of the MIDI queue.
 */

const int c_default_queue_size  = 128;

/**
 *  Provides a queue of midi::message structures.  This entity used to be a
 *  plain structure nested in the midi_in_api class.  We made it a class to
 *  encapsulate some common operations to save a burden on the callers.
 */

class RTL66_DLL_PUBLIC midi_queue
{

private:

    unsigned m_front;
    unsigned m_back;
    unsigned m_size;
    unsigned m_ring_size;
    midi::message * m_ring;

public:

    midi_queue ();
    ~midi_queue ();
    midi_queue (const midi_queue &) = delete;
    midi_queue & operator = (const midi_queue &) = delete;

    bool empty () const
    {
        return m_size == 0;
    }

    int count () const
    {
        return int(m_size);
    }

    bool full () const
    {
        return m_size == m_ring_size;
    }

    const midi::message & front () const
    {
        return m_ring[m_front];
    }

    bool push (const midi::message & mmsg);
    void pop ();
    midi::message pop_front ();
    void allocate (unsigned queuesize = c_default_queue_size);
    void deallocate ();

};          // class midi_queue

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_MIDI_QUEUE_HPP

/*
 * midi_queue.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

