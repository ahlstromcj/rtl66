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
 * \file          iothread.cpp
 *
 *  This module defines the base class for a I/O thread for playback or
 *  recording.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom and others
 * \date          2024-05-22
 * \updates       2024-06-01
 * \license       GNU GPLv2 or above
 *
 *  Thread functions`:
 *
 *      The thread function can be created in various ways:
 *
 *      Lambda:
 *
 *      set_mapper().exec_thread_function
 *      (
 *          [p] (args) { bool result = some_code...; return result; }
 *      );
 *      if (result) ...
 *
 *      Bind (more expensive):
 *
 *          iothread::functor threadfunc =
 *          (
 *              std::bind ( &player::poll_cycle, this, // _1, _2 // )
 *          );
 *          inthread().exec_thread_function(threadfunc);
 *
 *          Note the arguments (f and args):
 *
 *              f: Callable object (function object, pointer to function,
 *                 reference to function, pointer to member function, or
 *                 pointer to data member) that will be bound to some
 *                 arguments.
 *
 *           args: List of arguments to bind, with unbound arguments
 *                 replaced by placeholders _1, _2, _3... in namespace
 *                 std::placeholders.
 */

#include "c_macros.h"                   /* not_nullptr macro                */
#include "rtl/iothread.hpp"             /* rtl::iothread, this class       */
#include "util/msgfunctions.hpp"        /* util::warn_message() etc.        */
#include "xpc/timing.hpp"               /* xpc::set_thread_priority()       */

namespace rtl
{

#if defined UNUSED_VARIABLE

/**
 *  This value is the "trigger width" in microseconds, a Seq24 concept.
 *  It also had a "lookahead" time of 2 ms, not used however.
 */

static const int c_thread_trigger_width_us = 4 * 1000;

/**
 *  This high-priority value is used if the --priority option is specified.
 *  Needs more testing, we really haven't needed it yet.
 */

static const int c_thread_priority = 1;

#endif

/**
 *  Principal constructor.
 */

iothread::iothread (int priority) :
    m_io_thread     (),                 /* unique_ptr<std::thread>          */
    m_priority      (priority),         /* requires root to elevate it      */
    m_launched      (false),            /* is the thread running?           */
    m_active        (false)             /* is it supposed to do anything?   */
{
    // no code
}

/**
 *  A thread that has finished executing code, but has not yet been joined is
 *  still considered an active thread of execution and is therefore joinable.
 */

iothread::~iothread ()
{
    if (joinable())
        join();
}

/**
 *  Player:
 *
 *      iothread in_thread(priority);
 *      iothread::functor threadfunc = std::bind(&player::poll_cycle, this);
 *      bool ok = in_thread.launch(threadfunc);
 *      if (ok) ...
 *
 *      m_io_thread = std::thread(&iothread::output_func, this);
 */

bool
iothread::launch (functor f)
{
    bool result = true;
    if (! m_launched)
    {
        m_io_thread.reset(new (std::nothrow) std::thread(f));
        if (m_io_thread)
        {
            if (m_priority > 0)
            {
                bool ok = xpc::set_thread_priority(io_thread(), m_priority);
                if (ok)
                {
#if defined RTL66_PLATFORM_UNIX
                    util::warn_message("Thread priority elevated");
#endif
                    m_launched = true;
                }
                else
                {
                    errprint
                    (
                        "output thread: couldn't set scheduler to FIFO, "
                        "need root priviledges."
                    );
                    join();
                    result = false;
                }
            }
            else
                m_launched = true;
        }
        else
            errprint("Could not start thread");
    }
    return result;
}

/**
 *  Ends the thread.
 */

bool
iothread::finish ()
{
    bool result = false;
    if (m_launched)
    {
        if (done())
        {
            result = true;
            m_active = false;                   /* set done() for predicate */
            if (joinable())
                join();

            m_launched = false;
        }
    }
    return result;
}

}           // namespace rtl

/*
 * iothread.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

