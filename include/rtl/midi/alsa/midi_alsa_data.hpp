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
 * \updates       2023-07-20
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_ALSA

#include <alsa/asoundlib.h>             /* ALSA header file                 */
#include <pthread.h>                    /* pthread_t, etc.                  */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "midi/midibytes.hpp"           /* midi::byte, other aliases        */
#include "midi/ports.hpp"               /* midi::port::io type              */

namespace rtl
{

/**
 *  Contains the ALSA MIDI API data as a kind of scratchpad for this object.
 */

class RTL66_DLL_PUBLIC midi_alsa_data
{

    friend class midi_alsa;

private:

    snd_seq_t * m_seq;
    int m_portnum;
    int m_vport;
    snd_seq_port_subscribe_t * m_subscription;
    snd_midi_event_t * m_event_parser;
    size_t m_buffer_size;
    midi::byte * m_buffer;
    pthread_t m_thread;
    pthread_t m_dummy_thread_id;
    snd_seq_real_time_t m_last_time;
    int m_queue_id;       // input queue needed to get timestamped events
    int m_trigger_fds[2];

public:

    midi_alsa_data ();

    bool initialize
    (
        snd_seq_t * seq,
        midi::port::io iotype,      // bool isinput = false,
        size_t buffersize   = 32
    );

    /**
     *  This destructor currently does nothing.  We rely on the enclosing
     *  class to close out the things that it created.
     */

    ~midi_alsa_data ()
    {
        // Empty body
    }

    snd_seq_t * alsa_client ()
    {
        return m_seq;
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

    snd_midi_event_t ** event_address ()
    {
        return &m_event_parser;
    }

    size_t buffer_size () const
    {
        return m_buffer_size;
    }

    midi::byte * buffer ()
    {
        return m_buffer;
    }

    bool valid_buffer () const
    {
        return not_nullptr(m_buffer);
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
        return (i == 0 || i == 1) ?  m_trigger_fds[i] : (-1) ;
    }

public:

    void alsa_client (snd_seq_t * c)
    {
        m_seq = c;
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

    void event_parser (snd_midi_event_t * ep)
    {
        m_event_parser = ep;
    }

    void buffer_size (size_t sz)
    {
        m_buffer_size = sz;
    }

    void buffer (midi::byte * b)
    {
        m_buffer = b;
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

};          // class midi_alsa_data

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

#endif      // RTL66_RTL_MIDI_ALSA_DATA_HPP

/*
 * midi_alsa_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

