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
 * \file          midi_jack_data.cpp
 *
 *    Object for holding the current status of ALSA and ALSA MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-26
 * \updates       2023-07-19
 * \license       See above.
 *
 */

#include "rtl/midi/jack/midi_jack_data.hpp"     /* RTL66_EXPORT, etc.       */

#if defined RTL66_BUILD_JACK

namespace rtl
{

/**
 *  Static members used for calculating frame offsets in the Seq66 JACK
 *  output callback.
 */

transport::jack::info midi_jack_data::m_transport_info;

/**
 *  Default constructor.
 */

midi_jack_data::midi_jack_data () :
    m_jack_client       (nullptr),
    m_jack_port         (nullptr),
    m_jack_buffer       (nullptr),      /* ring_buffer<midi::message>    */
    m_jack_lasttime     (0),
#if RTL66_HAVE_SEMAPHORE_H
    m_semaphores_inited (false),
    m_sem_cleanup       (),
    m_sem_needpost      (),
#endif
#if defined RTL66_JACK_PORT_REFRESH_CALLBACK
    m_internal_port_id  (null_system_port_id()),
#endif
    m_jack_rtmidiin     (nullptr)
{
    // Empty body
}

/**
 *  This destructor currently does nothing, as it owns nothing.
 */

midi_jack_data::~midi_jack_data ()
{
    // Empty body
}

#if RTL66_HAVE_SEMAPHORE_H

/**
 *  Initializes the JACK semaphores.  The semaphores are shared between the
 *  threads of the process, and the initial value of the semaphore is 0.
 */

bool
midi_jack_data::semaphore_init ()
{
    bool result = ! m_semaphores_inited;
    if (result)
    {
        int rc = sem_init(&m_sem_cleanup, 0, 0);
        result = rc != (-1);
        if (result)
        {
            (void) sem_init(&m_sem_needpost, 0, 0);
            m_semaphores_inited = true;
        }
        else
        {
            perror("semaphore_init failed");
        }
    }
    else
    {
        errprint("semaphores already initialized");
    }
    return result;
}

void
midi_jack_data::semaphore_destroy ()
{
    if (m_semaphores_inited)
    {
        int rc = sem_destroy(&m_sem_cleanup);
        if (rc == (-1))
        {
            perror("cleanup semaphore");
        }
        rc = sem_destroy(&m_sem_needpost);
        if (rc == (-1))
        {
            perror("needpost semaphore");
        }
        m_semaphores_inited = false;
    }
    else
    {
        errprint("destroying uninitialized semaphores");
    }
}

bool
midi_jack_data::semaphore_post_and_wait ()
{
    bool result = m_semaphores_inited;
    if (result)
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) != (-1))
        {
            ++ts.tv_sec;                            /* wait max one second  */

            int rc = sem_post(&m_sem_needpost);
            if (rc == (-1))
            {
                perror("needpost post");
            }
            else
            {
                rc = sem_timedwait(&m_sem_cleanup, &ts);
                if (rc == (-1))
                {
                    perror("cleanup timedwait");
                }
            }
        }
    }
    return result;
}

bool
midi_jack_data::semaphore_wait_and_post ()
{
    bool result = m_semaphores_inited;
    if (result)
    {
        int rc = sem_trywait(&m_sem_needpost);
        if (rc == (-1))
        {
            perror("needpost trywait");
        }
        else
        {
            rc = sem_post(&m_sem_cleanup);
            if (rc == (-1))
            {
                perror("cleanup post");
            }
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

