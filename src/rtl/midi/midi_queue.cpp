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
 * \file          midi_queue.cpp
 *
 *    A MIDI message queue.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-01
 * \updates       2023-07-19
 * \license       See above.
 *
 *  Provides some basic types for the (heavily-factored) rtl66 library, very
 *  loosely based on Gary Scavone's RtMidi library.
 */

#include "c_macros.h"                   /* is_nullptr(), not_nullptr()      */
#include "rtl/midi/midi_queue.hpp"      /* rtl::midi_queue class            */

namespace rtl
{

/**
 *  Default constructor.
 */

midi_queue::midi_queue () :
    m_front     (0),
    m_back      (0),
    m_size      (0),
    m_ring_size (0),
    m_ring      (nullptr)
{
    allocate();
}

/**
 *  Destructor.
 */

midi_queue::~midi_queue ()
{
    deallocate();
}

/**
 *
 *  This would be better off as a constructor operation.  But one step at a
 *  time.
 */

void
midi_queue::allocate (unsigned queuesize)
{
    deallocate();
    if (queuesize > 0 && is_nullptr(m_ring))
    {
        m_ring = new (std::nothrow) midi::message[queuesize];
        m_ring_size = not_nullptr(m_ring) ? queuesize : 0 ;
    }
}

/**
 *  This would be better off as a destructor operation.  But one step at a
 *  time.
 */

void
midi_queue::deallocate ()
{
    if (not_nullptr(m_ring))
    {
        delete [] m_ring;
        m_ring = nullptr;
    }
}

/**
 *  As long as we haven't reached our queue size limit, push the message.
 */

bool
midi_queue::push (const midi::message & mmsg)
{
    bool result = ! full();
    if (result)
    {
        m_ring[m_back++] = mmsg;
        if (m_back == m_ring_size)
            m_back = 0;

        ++m_size;
    }
    else
    {
        /*
         * errprintfunc("message queue limit reached");
         */
    }
    return result;
}

/**
 *  Pops, so to speak, the front message out of the queue, effectively
 *  throwing it away.  One useful call sequence is:
 *
\verbatim
    midi::message latest = queue.front();
    queue.pop();
\endverbatim
 *
 *  An alternative is to use the pop_front() function instead.
 */

void
midi_queue::pop ()
{
    --m_size;
    ++m_front;
    if (m_front == m_ring_size)
        m_front = 0;
}

/**
 *  Pops a copy of the front message.   Could be a little inefficient, since a
 *  couple of copies are made, and we cannot use return-code optimization.
 *
 *  Perhaps at some point we could use move semantics?
 *
 * \return
 *      Returns a copy of the message that was in front before the popping.
 *      If the queue is empty, an empty (all zeros) message is returned.
 *      Can be checked with the midi::message::empty() function.
 */

midi::message
midi_queue::pop_front ()
{
    midi::message result;
    if (m_size != 0)
    {
        result = m_ring[m_front];
        pop();
    }
    return result;
}

}           // namespace rtl

/*
 * midi_queue.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

