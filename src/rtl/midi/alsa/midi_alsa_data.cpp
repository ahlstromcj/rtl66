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
 * \updates       2024-09-02
 * \license       See above.
 *
 */

#include "rtl/midi/alsa/midi_alsa_data.hpp"  /* RTL66_EXPORT, etc.          */

#if defined RTL66_BUILD_ALSA

namespace rtl
{

/**
 * Note that most of the class members are initialize in-class (in the
 * class header file. We need to guarantee that a buffer exists before
 * usage, as the order of setup calls can vary.
 */

midi_alsa_data::midi_alsa_data () : m_trigger_fds ()    /* 2-element array  */
{
    if (buffer_size() == 0)
        buffer_size(c_event_size_max);

    m_buffer.reset(new (std::nothrow) midi::byte [buffer_size()]);
}

void
midi_alsa_data::clear ()
{
    m_alsa_client = nullptr;
    m_portnum = -1;
    m_vport = -1;
    m_subscription = nullptr;
    m_event_parser = nullptr;
    unallocate();
}

bool
midi_alsa_data::initialize
(
    snd_seq_t * seq,
    midi::port::io iotype,
    size_t buffsize
)
{
    bool result { true };
    if (is_initialized())
        return result;

    clear();
    m_alsa_client = seq;
    m_portnum = m_vport = (-1);

    /*
     * m_queue_id = (-1);
     * m_last_time.tv_sec = m_last_time.tv_nsec = 0;
     */

    m_buffer_size = buffsize;
    if (iotype == midi::port::io::input)
    {
        m_subscription = nullptr;
        m_buffer.reset();
        m_dummy_thread_id = pthread_self();
        m_thread = m_dummy_thread_id;
        m_trigger_fds[0] = m_trigger_fds[1] = (-1);

        int rc { pipe(m_trigger_fds) };
        result = rc == 0;
        if (! result)
        {
            errprint("ALSA pipe() failed");
        }
    }
    else if (iotype == midi::port::io::output)
    {
        m_event_parser = nullptr;

        int rc { ::snd_midi_event_new(buffsize, &m_event_parser) };
        result = rc == 0;
        if (result)
        {
            result = reallocate(buffsize);
            if (result)
            {
                ::snd_midi_event_init(m_event_parser);
            }
            else
            {
                errprint("buffer allocation failed");
            }
        }
        else
        {
            errprint("snd_midi_event_new() failed");
        }
    }
    return result;
}

bool
midi_alsa_data::reallocate (size_t buffsize)
{
    bool result = buffsize > 0;
    if (result)
    {
        m_buffer.reset(new (std::nothrow) midi::byte [buffsize]);
        result = not_nullptr(buffer());
        if (result)
            buffer_size(buffsize);
    }
    return result;
}

void
midi_alsa_data::unallocate ()
{
    m_buffer.reset();
    buffer_size(0);
}

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

/*
 * midi_alsa_data.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

