/*
 *  This file is part of rtl66.
 *  #endif
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
 * \file          midi_jack_data.cpp
 *
 *    Object for holding the current status of ALSA and ALSA MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-26
 * \updates       2025-12-09
 * \license       See above.
 *
 */

#include "rtl/midi/jack/midi_jack_data.hpp"     /* RTL66_EXPORT, etc.       */
#include "util/msgfunctions.hpp"                /* util::error_message()    */

#if defined RTL66_BUILD_JACK

namespace rtl
{

/**
 *  Static members used for calculating frame offsets in the Seq66 JACK
 *  output callback.
 */

transport::jack::info midi_jack_data::m_transport_info;

/*------------------------------------------------------------------------
 * midi_jack_data sized constructor
 *------------------------------------------------------------------------*/

/**
 *  Allocates data. Tired of pointers.
 *
 *  The other members are initialized "in-class".
 */

midi_jack_data::midi_jack_data (rtmidi_in_data & rid, std::size_t sz) :
    m_jack_buffer   (sz == 0 ? c_jack_ringbuffer_size : sz),
    m_jack_rtmidiin (rid)
{
    // no other code
}

/**
 *  The default constructor uses "in-class" member initialization.
 */

#if RTL66_HAVE_SEMAPHORE_H

/**
 *  Initializes the JACK semaphores.  The semaphores are shared between the
 *  threads of the process, and the initial value of the semaphore is 0.
 *  The value in sem_init() means how many times we could execute sem_wait()
 *  without really waiting without any sem_post().
 *
 *  The semaphore value represents the number of common resources available
 *  to be shared among the threads. If the value is greater than 0, then
 *  the thread calling sem_wait() need not wait; it just decrements the value
 *  and (1) if negative, it blocks or (2) it otherwise proceeds to access the
 *  common resource.
 *
 *  sem_post() adds a resource back to the pool, so it increments the value.
 *  If the value is 0, then sem_wait() waits until sem_post() is called.
 *
 *  This function is called in midi_jack::initialize().
 */

bool
midi_jack_data::semaphore_init ()
{
    bool result = { m_semaphores_inited };
    if (! result)
    {
        int rc { ::sem_init(&m_sem_cleanup, 0, 0) };
        result = rc != (-1);
        if (result)
        {
            rc = ::sem_init(&m_sem_needpost, 0, 0);
            result = rc != (-1);
            if (result)
            {
                m_semaphores_inited = true;

                /*
                 *
                 *  printf
                 *  (
                 *      "semaphores %p & %p initialized\n",
                 *      (void *) &m_sem_cleanup, (void *) &m_sem_needpost
                 *  );
                 */
            }
            else
            {
                ::perror("needpost semaphore init");
                (void) ::sem_destroy(&m_sem_cleanup);
            }
        }
        else
            ::perror("cleanup semaphore init");
    }
    else
    {
        char temp[80];
        (void) snprintf
        (
            temp, sizeof temp, "semaphores %p & %p already initialized",
            (void *) &m_sem_cleanup, (void *) &m_sem_needpost
        );
        util::warn_message(temp);
    }
    return result;
}

/**
 *  Called in midi_jack::~midi_jack().
 */

void
midi_jack_data::semaphore_destroy ()
{
    if (m_semaphores_inited)
    {
        int rc { ::sem_destroy(&m_sem_cleanup) };
        if (rc == (-1))
            ::perror("cleanup semaphore");

        rc = ::sem_destroy(&m_sem_needpost);
        if (rc == (-1))
            ::perror("needpost semaphore");

        m_semaphores_inited = m_semaphores_post_waited = false;
    }
    else
    {
#if defined PLATFORM_DEBUG
        util::error_message("uninitialized semaphores");
#endif
    }
}

/**
 *  Called in midi_jack::close_port().
 */

bool
midi_jack_data::semaphore_post_and_wait ()
{
    bool result { m_semaphores_inited };
    if (result)
    {
        struct timespec ts;
        if (::clock_gettime(CLOCK_REALTIME, &ts) != (-1))
        {
            ++ts.tv_sec;                            /* wait max one second  */

            int rc { ::sem_post(&m_sem_needpost) };
            if (rc != 0)
                ::perror("needpost post");

            rc = ::sem_timedwait(&m_sem_cleanup, &ts);
            if (rc != 0)
                ::perror("cleanup timedwait");

            m_semaphores_post_waited = true;
        }
    }
    return result;
}

/**
 *  Called at the end of the jack_process_out() callback.
 *
 *  However, it is the first thing called and never succeeds
 *  (or maybe succeeds when the program ends).
 */

bool
midi_jack_data::semaphore_wait_and_post ()
{
    bool result { m_semaphores_inited };
    if (result)
    {
        int rc { ::sem_trywait(&m_sem_needpost) };
        if (rc == 0)
        {
            rc = ::sem_post(&m_sem_cleanup);
            if (rc != 0)
                ::perror("cleanup post");
        }
        else
        {
#if defined PLATFORM_DEBUG
            /*
             * This happens until semaphore_post_and_wait() is
             * called. This does not seem right. Commented
             * to avoid a flood of console output.
             */

            if (m_semaphores_post_waited)
                ::perror("needpost trywait");
#endif
        }
    }
    return result;
}

#endif      // RTL66_HAVE_SEMAPHORE_H

}           // namespace rtl

#endif      // RTL66_BUILD_JACK

/*
 * midi_jack_data.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

