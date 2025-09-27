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
 * \updates       2024-09-26
 * \license       See above.
 *
 */

#include "rtl/midi/alsa/midi_alsa_data.hpp" /* RTL66_EXPORT, etc.           */
#include "util/msgfunctions.hpp"            /* util::warn_message(), etc.   */

#if defined RTL66_BUILD_ALSA

namespace rtl
{

/**
 * Note that most of the class members are initialized "in-class" (in the
 * class header file). We need to guarantee that a buffer exists before
 * usage, as the order of setup calls can vary.
 */

midi_alsa_data::midi_alsa_data () : m_trigger_fds ()    /* 2-element array  */
{
    if (buffer_size() == 0)
        buffer_size(c_event_size_max);

    m_buffer.reset(new (std::nothrow) midi::byte [buffer_size()]);
}

midi_alsa_data::~midi_alsa_data ()
{
    unallocate();
    (void) free_event_parser();
}

void
midi_alsa_data::clear ()
{
    m_alsa_client = nullptr;
    m_portnum = m_vport = -1;
    m_subscription = nullptr;
    unallocate();
    (void) free_event_parser();
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
        buffer_size(buffsize);

        int rc { pipe(m_trigger_fds) };
        result = rc == 0;
        if (! result)
        {
            util::error_message("ALSA pipe() failed");
        }
    }
    else if (iotype == midi::port::io::output)
    {
        m_event_parser = nullptr;

        /*
         * result = new_event_parser(buffsize);
         */

        int rc { ::snd_midi_event_new(buffsize, &m_event_parser) };
        result = rc == 0;
        if (result)
        {
            result = reallocate(buffsize);
            if (! result)
            {
                util::error_message("buffer allocation failed");
            }
        }
        else
        {
            util::error_message("snd_midi_event_new() failed");
        }
    }
    /*
     * set_initialized(true);
     */

    return result;
}

/*------------------------------------------------------------------------
 * Buffer functions
 *------------------------------------------------------------------------*/

bool
midi_alsa_data::reallocate (size_t buffsize)
{
    bool result { buffsize > 0 };
    if (result)
    {
        m_buffer.reset(new (std::nothrow) midi::byte [buffsize]);
        result = not_nullptr(buffer());
        if (result)
            buffer_size(buffsize);
        else
            util::error_message("reallocate() error");
    }
    return result;
}

void
midi_alsa_data::unallocate ()
{
    m_buffer.reset();
    buffer_size(0);
}

/*------------------------------------------------------------------------
 * ALSA data management wrapper functions
 *------------------------------------------------------------------------*/

/**
 *  Creates and initializes a MIDI event parser. This function creates and
 *  initializes a MIDI parser object to convert a MIDI byte stream to
 *  sequencer events (encoding) or to convert sequencer events to a MIDI
 *  byte stream (decoding).
 *
 *  The ALSA function snd_midi_event_init() resets both the encoder and
 *  decoder of the event parser.
 *
 *  The ALSA function snd_midi_event_no_status() enables command merging
 *  here; this means that MIDI running status can be used to re-use the
 *  previous event status byte.
 *
 * \param [in] buffsize
 *      The size of the buffer used for encoding, which should be large
 *      enough to hold the largest MIDI message to be encoded.
 *
 * \return
 *      Returns true on success. An error likely means "out of memory".
 */

bool
midi_alsa_data::new_event_parser (size_t buffsize)
{
    if (buffsize == 0)
        buffsize = buffer_size();

    int rc { ::snd_midi_event_new(buffsize, &m_event_parser) };
    return rc == 0;
}

bool
midi_alsa_data::init_event_parser (size_t buffsize)
{
    bool result { new_event_parser(buffsize) };
    if (result)
    {
        ::snd_midi_event_init(m_event_parser);      /* actually redundant   */
        ::snd_midi_event_no_status(m_event_parser, 1);
    }
    return result;
}

bool
midi_alsa_data::resize_event_parser (size_t buffsize)
{
    bool result { reallocate(buffsize) };
    if (result)
    {
        int rc { ::snd_midi_event_resize_buffer(event_parser(), buffsize) };
        result = rc == 0;
    }
    return result;
}

/**
 *  This function frees the event parser and indicates that by
 *  nullifying the parser pointer.
 */

bool
midi_alsa_data::free_event_parser ()
{
    bool result { not_nullptr(m_event_parser) };
    if (result)
    {
       ::snd_midi_event_free(m_event_parser);
       m_event_parser = nullptr;
    }
    return result;
}

void
midi_alsa_data::handler_cleanup ()
{
    unallocate();
    ::snd_midi_event_free(event_parser());
    event_parser(nullptr);
    thread_handle(dummy_thread_id());
}

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

/*
 * midi_alsa_data.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

