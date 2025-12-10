#if ! defined RTL66_RTL_MIDI_ALSA_DATA_HPP
#define RTL66_RTL_MIDI_ALSA_DATA_HPP

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
 * \file          midi_alsa_data.hpp
 *
 *    Object for holding the current status of ALSA and ALSA MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-06-17
 * \updates       2025-12-09
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_ALSA

#include <alsa/asoundlib.h>             /* ALSA header file                 */
#include <pthread.h>                    /* pthread_t, etc.                  */
#include <memory>                       /* std::unique_ptr()                */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "midi/midibytes.hpp"           /* midi::byte, other aliases        */
#include "midi/port.hpp"                /* midi::port::io type              */

namespace rtl
{

class rtmidi_in_data;

const size_t c_event_size_max { 12 };   /* from Seq66                       */

/**
 *  Contains the ALSA MIDI API data as a kind of scratchpad for this object.
 */

class RTL66_DLL_PUBLIC midi_alsa_data
{

    friend class midi_alsa;

private:

    bool m_is_initialized { false };
    snd_seq_t * m_alsa_client { nullptr };
    int m_portnum { -1 };
    int m_vport { -1 };
    snd_seq_port_subscribe_t * m_subscription { nullptr };

    /**
     *  This event parser is created in the ALSA handler and freed at the end
     *  of it and in case of error. It is init'd and used to turn off running
     *  status. It is used to decode events.
     *
     *  It is also created in initialize(), init'd there, and freed in the
     *  MIDI-out destructor.
     *
     *  It can be resized in send_message(), where in encodes the MIDI
     *  message.
     */

    snd_midi_event_t * m_event_parser { nullptr };

    size_t m_buffer_size { c_event_size_max };      /* increase as needed   */

    /**
     *  Change to an exception-safe object. We could also use the
     *  midi::bytes vector, but it's less flexible for the purpose,
     *  which includes reallocation.
     *
     *      midi::byte * m_buffer { nullptr };
     */

    std::unique_ptr<midi::byte []> m_buffer { };

    pthread_t m_thread { };
    pthread_t m_dummy_thread_id { };
    snd_seq_real_time_t m_last_time { };
    int m_queue_id { -1 };       /* input queue to get timestamped events   */
    int m_trigger_fds[2];

    /**
     *  Holds special data peculiar to the client and its MIDI input
     *  processing. This data consists of the midi_queue message queue and a
     *  few boolean flags.
     *
     *  Whoops! Already defined as a true member in midi_api, accessed via
     *  midi_api::input_data(). A small waste of space if the port is not
     *  meant for input.
     */

    rtmidi_in_data & m_alsa_rtmidiin;   /* initialize in the constructor    */

public:

    midi_alsa_data () = delete;
    midi_alsa_data (rtmidi_in_data &);
    midi_alsa_data (const midi_alsa_data &) = delete;
    midi_alsa_data (midi_alsa_data &&) = default;
    midi_alsa_data & operator = (const midi_alsa_data &) = delete;
    midi_alsa_data & operator = (midi_alsa_data &&) = default;
    ~midi_alsa_data ();

    void clear ();
    bool initialize
    (
        ::snd_seq_t * seq,
        midi::port::io iotype,
        size_t buffsize = RTL66_DEFAULT_ALSA_EV_BUFSIZE
    );
    bool reallocate (size_t buffsize = RTL66_DEFAULT_ALSA_EV_BUFSIZE);
    void unallocate ();

    bool is_initialized () const
    {
        return m_is_initialized;
    }

    void set_initialized (bool flag)
    {
        m_is_initialized = flag;
    }

    /*
     *  Already accessible via midi_api::input_data(), but we don't
     *  have direct access to that here.
     *
     *   void rt_midi_in (rtmidi_in_data * rid)
     *   {
     *       m_alsa_rtmidiin = rid;
     *   }
     */

     rtmidi_in_data & rt_midi_in ()
     {
        return m_alsa_rtmidiin;
     }

     const rtmidi_in_data & rt_midi_in () const
     {
        return m_alsa_rtmidiin;
     }

    snd_seq_t * alsa_client ()
    {
        return m_alsa_client;
    }

    const snd_seq_t * alsa_client () const
    {
        return m_alsa_client;
    }

    int port_number () const
    {
        return m_portnum;
    }

    int vport () const
    {
        return m_vport;
    }

    snd_seq_port_subscribe_t * subscription ()
    {
        return m_subscription;
    }

    snd_midi_event_t * event_parser ()
    {
        return m_event_parser;
    }

    const snd_midi_event_t * event_parser () const
    {
        return m_event_parser;
    }

    size_t buffer_size () const
    {
        return m_buffer_size;
    }

    midi::byte * buffer ()
    {
        return m_buffer.get();
    }

    const midi::byte * buffer () const
    {
        return m_buffer.get();
    }

    bool valid_buffer () const
    {
        return bool(m_buffer);
    }

    pthread_t thread_handle ()
    {
        return m_thread;
    }

    pthread_t * thread_address ()
    {
        return &m_thread;
    }

    pthread_t dummy_thread_id () const
    {
        return m_dummy_thread_id;
    }

    snd_seq_real_time_t last_time () const
    {
        return m_last_time;
    }

    int queue_id () const
    {
        return m_queue_id;
    }

    int trigger_fd (int i) const
    {
        return (i == 0 || i == 1) ? m_trigger_fds[i] : (-1) ;
    }

public:

    void alsa_client (snd_seq_t * c)
    {
        m_alsa_client = c;
    }

    void port_number (int p)
    {
        m_portnum = p;
    }

    void vport (int v)
    {
        m_vport = v;
    }

    void subscription (snd_seq_port_subscribe_t * sp)
    {
        m_subscription = sp;
    }

    void buffer_size (size_t sz)
    {
        m_buffer_size = sz;
    }

    void buffer (midi::byte * b)            // hmmmmmmmmm
    {
        m_buffer.reset(b);
    }

    void thread_handle (pthread_t pt)
    {
        m_thread = pt;
    }

    void dummy_thread_id (pthread_t pt)
    {
        m_dummy_thread_id = pt;
    }

    void last_time (snd_seq_real_time_t lt)
    {
        m_last_time = lt;
    }

    void queue_id (int q)
    {
        m_queue_id = q;
    }

public:     // wrapper functions for some of the data items

    bool new_event_parser (size_t buffsize = 0);
    bool init_event_parser (size_t buffsize = 0);
    bool free_event_parser ();
    bool resize_event_parser (size_t buffsize);
    void handler_cleanup ();

private:

    void event_parser (snd_midi_event_t * ep)
    {
        m_event_parser = ep;
    }

};          // class midi_alsa_data

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

#endif      // RTL66_RTL_MIDI_ALSA_DATA_HPP

/*
 * midi_alsa_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

