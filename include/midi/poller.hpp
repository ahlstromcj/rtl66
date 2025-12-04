#if ! defined RTL66_MIDI_POLLER_HPP
#define RTL66_MIDI_POLLER_HPP

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
 * \file          poller.hpp
 *
 *  This module declares/defines a simple test class for polling a number of
 *  midi::bus_in objects.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2025-11-08
 * \updates       2025-12-04
 * \license       GNU GPLv2 or above
 *
 *  The poller class is a severely cut-down version of midi::poller with
 *  only functionality pertaining to MIDI input.
 */

#include <memory>                           /* std::unique_ptr<>            */
#include <queue>                            /* std::queue<>                 */

#include "xpc/condition.hpp"                /* xpc::condition/synchronizer  */
#include "midi/masterbus.hpp"               /* access to all MIDI busses    */
#include "midi/ports.hpp"                   /* access to MIDI ports         */
#include "rtl/iothread.hpp"                 /* rtl::iothread class          */
#include "rtl/midi/rtmidi_in_data.hpp"      /* rtl::rtmidi_in_data class    */
#include "transport/clock/info.hpp"         /* transport::clock::info       */
#include "xpc/fifo.hpp"                     /* xpc::filo template class     */

namespace midi
{

/**
 *  This class supports the limited performance mode.
 */

class poller
{
#if defined RTL66_BUILD_JACK_DISABLED
#include "transport/jack/scratchpad.hpp"    /* transport::jack::scratchpad  */
#include "transport/jack/transport.hpp"     /* transport::jack::transport   */

    friend class transport::jack::transport;
    friend int jack_sync_callback
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );
    friend int jack_transport_callback (jack_nframes_t nframes, void * arg);
    friend void jack_shutdown (void * arg);
    friend void jack_timebase_callback
    (
        jack_transport_state_t state, jack_nframes_t nframes,
        jack_position_t * pos, int new_pos, void * arg
    );
    friend long get_current_jack_position (void * arg);

#endif  // RTL66_BUILD_JACK

public:

    /**
     *  A FIFO queue so that we can push incoming midi::messages to the
     *  back and pop them from the front. Compare this to the home-grown
     *  rtl::midi_queue for midi::messages in the rtl/midi directories.
     */

    using inqueue = xpc::fifo<midi::message>;
    using inqueueptr = std::unique_ptr<inqueue>;

    /**
     *  The input queue is created only if requested.
     */

    inqueueptr m_input_q_ptr;

    /**
     *  A valid size (greater than 0) in the constructor sets this
     *  to true and attempts to create the inqueueptr.
     */

    bool m_use_input_q { false };

    /**
     *  This boolean is a hand-massaged value for testing. If true,
     *  messages are put on the queue to be retrieved via
     *  poller::get_message(), otherwise they are handled by poller.
     */

    bool m_enqueue_messages { true };

    /**
     *  A nested class to provide an implementation of the synchronizer
     *  class.
     */

    class synch : public xpc::synchronizer
    {
    private:

        poller & m_perf;

    public:

        synch (poller & p) : xpc::synchronizer (), m_perf (p)
        {
            // no code
        }

        synch () = delete;
        synch (const synch &) = delete;
        synch & operator = (const synch &) = delete;

        virtual bool predicate () const override
        {
            return m_perf.is_running() || m_perf.done();
        }
    };

public:

    /**
     *  Provides our MIDI buss list.
     */

    midi::masterbus & m_master_bus;

    /**
     *  Supports a single input port and a single output port. A port is used
     *  if the port number is greater than or equal to 0, the port exists,
     *  and the number isn't the special value RTL66_PORTS_ALL.
     */

    int m_in_portnumber { RTL66_PORTS_ALL };

private:                            /* key, midi, and op container section  */

    /**
     *  Rather than a whole midi::input_specs structure, just the two
     *  callback-related items.
     */

    bool m_input_using_callback { false };

    /**
     *  The optional callback function pointer.
     */

    rtl::rtmidi_in_data::callback_t m_input_callback { nullptr };

    /**
     *  Provides an optional queue for storing incoming events and
     *  saving them for the caller to pop.
     */


    /**
     *  Provides information for managing threads. Provides a "handle" to
     *  the input thread.
     */

    rtl::iothread m_in_thread { };

    /**
     *  Indicates that playback is running. However, this flag is conflated
     *  with some JACK support, and we have to supplement it with another
     *  flag, m_is_pattern_playing.
     */

    std::atomic<bool> m_is_running { false };

    /**
     *  MIDI Clock support. The m_tick member holds the tick to be used in
     *  displaying the progress bars and the maintime pill. It is mutable
     *  because sometimes we want to adjust it in a const function for pause
     *  functionality.
     */

    mutable midi::pulse m_tick { 0 };

    /**
     *  A condition variable to protect playback. It is signalled if playback
     *  has been started. The output thread function waits on this variable
     *  until m_is_running and m_io_active are false. This variable is also
     *  signalled in the poller destructor. This implementation is
     *  new for 0.98.0, and it avoids segfaults, exit-hangs, and high CPU
     *  usage in Windows that have occurred with other implmentations.
     */

    synch m_condition_var;              /* there is no default constructor  */

    /**
     *  We need to adjust the clock increment for the PPQN that is in force.
     *  Higher PPQN need a longer increment than 8 in order to get 24 clocks
     *  per quarter note.
     */

    transport::clock::info m_clock_info { };

public:

    poller
    (
        midi::masterbus & mbus,
        int portnumber = RTL66_PORTS_ALL,
        int inqueuesz = (-1)
    );
    poller (const poller &) = delete;
    poller (poller &&) = delete;                    /* forced by iothread   */
    poller & operator = (const poller &) = delete;
    poller & operator = (poller &&) = delete;       /* ditto */
    virtual ~poller ();

    virtual bool setup_master_bus
    (
        clientinfo & ci = midi::global_client_info()
    );

    midi::pulse tick () const
    {
        return m_tick;
    }

    int in_portnumber () const
    {
        return m_in_portnumber;
    }

    bool use_all_ports () const
    {
        return m_in_portnumber == RTL66_PORTS_ALL;
    }

    bool use_input_q () const
    {
        return m_use_input_q;
    }

    bool enqueue_messages () const
    {
        return m_enqueue_messages;
    }

    void enqueue_messages (bool flag)
    {
        m_enqueue_messages = flag;
    }

    midi::message get_message ()
    {
        static midi::message s_dummy;
        return use_input_q() ? m_input_q_ptr->pop() : s_dummy ;
    }

public:

    int client_id () const
    {
        return m_master_bus.client_id();
    }

    bool is_running () const
    {
        return m_is_running;
    }

    bool done () const;
    bool launch (clientinfo & ci = midi::global_client_info());
    bool finish ();
    bool activate ();
    bool auto_stop ();

public:

    /**
     * http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/ssp.htm
     */

    void start_polling ()
    {
        // if (! is_jack_running())
            inner_start();
    }

    void stop_polling ()
    {
        // if (! is_jack_running())
            inner_stop();
    }

    const masterbus & master_bus () const
    {
        return m_master_bus;
    }

    masterbus & master_bus ()
    {
        return m_master_bus;
    }

    bool input_using_callback () const
    {
        return m_input_using_callback;
    }

    rtl::rtmidi_in_data::callback_t input_callback ()
    {
        return m_input_callback;
    }

protected:

    void inner_start ();
    void inner_stop (bool midiclock = false);
    void handle_message (const midi::message & incoming);

    rtl::iothread & in_thread ()
    {
        return m_in_thread;
    }

    const rtl::iothread & in_thread () const
    {
        return m_in_thread;
    }

public:                             /* access functions for the containers  */

    void signal_save ();
    void signal_quit ();

private:

    void is_running (bool flag)
    {
        m_is_running = flag;
    }

private:

    bool input_func ();
    bool poll_cycle ();
    bool launch_input_thread ();
    void midi_start ();
    void midi_continue ();
    void midi_stop ();

    synch & cv ()
    {
        return m_condition_var;
    }

};          // class poller

}           // namespace midi

#endif      // RTL66_MIDI_POLLER_HPP

/*
 * poller.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

