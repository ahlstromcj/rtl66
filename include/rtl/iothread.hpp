#if ! defined  RTL66_MIDI_IOTHREAD_HPP
#define RTL66_MIDI_IOTHREAD_HPP

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
 * \file          iothread.hpp
 *
 *  This module declares/defines a base class for handling some facets
 *  of playing or recording threads.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2024-05-22
 * \updates       2024-06-01
 * \license       GNU GPLv2 or above
 *
 *  The iothread class encapsulates the management of the I/O threads of
 *  midi::player.
 */

#include <atomic>                           /* std::atomic<>                */
#include <functional>                       /* std::function<>              */
#include <memory>                           /* std::unique_ptr<>            */
#include <thread>                           /* std::thread                  */

namespace rtl
{

/**
 *  This class supports the limited performance mode.
 */

class iothread
{
    friend class player;

public:

    /**
     *  Provides a type for a thread function which will run for a
     *  "long time". It returns true or false when done executing.
     *  TBD
     */

    using functor = std::function<bool ()>;

private:                            /* key, midi, and op container section  */

    /**
     *  Provides information for managing threads.  Provides a "handle" to
     *  the output thread.
     */

    std::unique_ptr<std::thread> m_io_thread;

    /**
     *  The desired priority for launching. Defaults to 0.
     */

    int m_priority;

    /**
     *  Indicates that the output thread has been started.
     */

    std::atomic<bool> m_launched;

    /**
     *  Indicates merely that the input or output thread functions can keep
     *  running.
     */

    std::atomic<bool> m_active;

public:

    iothread (int priority = 0);
    iothread (const iothread &) = delete;
    iothread (iothread &&) = delete;                    /* okay? */
    iothread & operator = (const iothread &) = delete;
    virtual ~iothread ();

public:

    bool active () const
    {
        return m_active;
    }

    bool done () const
    {
        return ! active();
    }

    void deactivate ()
    {
        m_active = false;
    }

public:

    bool launch (functor f);
    bool finish ();

private:

    bool joinable () const
    {
        return m_launched && m_io_thread->joinable();
    }

    void join ()
    {
        m_io_thread->join();
        m_launched = false;
        m_active = false;                       /* set done() for predicate */
    }

    std::thread & io_thread ()                  /* call only if launched    */
    {
        return *m_io_thread;
    }

};          // class iothread

}           // namespace rtl

#endif      // RTL66_MIDI_IOTHREAD_HPP

/*
 * iothread.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

