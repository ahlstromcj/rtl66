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
 * \file          midi_alsa_data.cpp
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

#include "rtl/midi/alsa/midi_alsa_data.hpp"  /* RTL66_EXPORT, etc.          */

#if defined RTL66_BUILD_ALSA

namespace rtl
{

midi_alsa_data::midi_alsa_data () :
    m_seq               (nullptr),
    m_portnum           (-1),
    m_vport             (-1),
    m_subscription      (nullptr),
    m_event_parser      (nullptr),
    m_buffer_size       (32),
    m_buffer            (nullptr),
    m_thread            (0),
    m_dummy_thread_id   (0),
    m_last_time         (),
    m_queue_id          (-1),
    m_trigger_fds       ()          // two-element array
{
    // Empty body
}

bool
midi_alsa_data::initialize
(
    snd_seq_t * seq,
    midi::port::io iotype,
    size_t buffersize
)
{
    bool result = true;
    m_seq = seq;
    m_portnum = m_vport = (-1);

    /*
     * m_queue_id = (-1);
     * m_last_time.tv_sec = m_last_time.tv_nsec = 0;
     */

    m_buffer_size = buffersize;
    if (iotype == midi::port::io::input)
    {
        m_subscription = nullptr;
        m_buffer = nullptr;
        m_dummy_thread_id = pthread_self();
        m_thread = m_dummy_thread_id;
        m_trigger_fds[0] = m_trigger_fds[1] = (-1);

        int rc = pipe(m_trigger_fds);
        result = rc == 0;
        if (! result)
        {
            errprint("ALSA pipe() failed");
        }
    }
    else if (iotype == midi::port::io::output)
    {
        m_event_parser = nullptr;

        int rc = snd_midi_event_new(buffersize, &m_event_parser);
        result = rc == 0;
        if (result)
        {
            m_buffer = new (std::nothrow) midi::byte[buffersize];
            result = not_nullptr(m_buffer);
            if (result)
            {
                snd_midi_event_init(m_event_parser);
                if (! result)
                {
                    errprint("ALSA buffer malloc() failed");
                }
            }
        }
        else
        {
            errprint("ALSA snd_midi_event_new() failed");
        }
    }
    return result;
}

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

/*
 * midi_alsa_data.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

