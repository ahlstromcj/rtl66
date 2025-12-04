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
 * \updates       2025-12-02
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
 *  Default constructor. Uses "in-class" member initialization.
 */

midi_queue::midi_queue (unsigned qsize)
{
    if (qsize > 0)
        allocate(qsize);
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

#if defined USE_EXTRA_QUEUE_FUNCTIONS

unsigned
midi_queue::size (unsigned * back, unsigned * front)
{
    /*
     * Access back/front members exactly once and make stack copies for
     * size calculation.
     */

    unsigned currback { m_back };
    unsigned currfront { m_front };
    unsigned currsize
    {
        currback >= currfront ?
            currback - currfront : m_ring_size - currfront + currback ;
    };

    /*
     * Return copies of back/front so no new and unsynchronized accesses
     * to member variables are needed.
     */

    if (back)
        *back = currback;

    if (front)
        *front = currfront;

    return currsize;
}

#endif  // defined USE_EXTRA_QUEUE_FUNCTIONS

#if defined USE_EXTRA_QUEUE_FUNCTIONS

/**
 *  As long as we haven't reached our queue size limit, push the message.
 *  Local stack copies of front/back. Get back/front indexes exactly
 *  once and calculate current size.
 */

bool
midi_queue::push (const midi::message & msg)
{
    unsigned currback;
    unsigned currfront;
    unsigned currsize { size(&currback, &currfront );
    if (currsize < m_ring_size - 1)
    {
        m_ring[currback] = msg;
        m_back = (m_back + 1) % m_ring_size;
        return true;
    }
    return false;
}

#else

/**
 *  As long as we haven't reached our queue size limit, push the message.
 */

bool
midi_queue::push (const midi::message & mmsg)
{
    if (m_ring_size == 0)
        return true;                    /* fake it, app has no input        */

    bool result { ! full() };
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

#endif  // defined USE_EXTRA_QUEUE_FUNCTIONS

#if defined USE_EXTRA_QUEUE_FUNCTIONS

/**
 *  Set local stack copies of front/back.
 *  Get back/front indexes exactly once and calculate current size.
 *  Copy queued message to the vector pointer argument and then "pop" it.
 */

bool
midi_queue::pop (std::vector<unsigned char> * msg, double * timestamp)
{
    unsigned currback;
    unsigned currfront;
    unsigned currsize { size(&currback, &currfront) } ;
    if (currsize == 0)
        return false;

    msg->assign
    (
        m_ring[currfront].bytes.begin(), m_ring[currfront].bytes.end()
    );
    *timestamp = m_ring[currfront].jack_stamp;
    m_front = (m_front + 1) % m_ring_size;      /* Update front */
    return true;
}

#else

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

#endif  // defined USE_EXTRA_QUEUE_FUNCTIONS

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

