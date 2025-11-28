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
 * \file          pollwrapper.cpp
 *
 *    Object for holding the current status of ALSA and ALSA MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2025-11-17
 * \updates       2025-11-27
 * \license       See above.
 *
 */

#include "rtl/midi/alsa/pollwrapper.hpp"    /* rtl::pollwrapper for ALSA    */
#include "util/msgfunctions.hpp"            /* util::warn_message(), etc.   */
#include "xpc/timing.hpp"                   /* xpc::millisleep()            */

#if defined RTL66_BUILD_ALSA

namespace rtl
{

/**
 *  Time to wait in the poll_for_midi () class member.
 *
 *  We did reduce the polling timeout from 1000 milliseconds (in Seq24) to 100
 *  milliseconds, and now, after testing, 10 milliseconds.
 */

static const int c_poll_wait_ms { 10 }; /* wait-time in milliseconds    */

/**
 * Note that most of the class members are initialized "in-class" (in the
 * class header file). We need to guarantee that a buffer exists before
 * usage, as the order of setup calls can vary.
 */

pollwrapper::pollwrapper
(
    snd_seq_t * client,
    int extra
) :
    m_alsa_client   (client)
{
    if (not_nullptr(client))
        m_is_initialized = get_poll_descriptors(extra);
}

pollwrapper::~pollwrapper ()
{
    remove_poll_descriptors();
}

/**
 *  This function might not be needed.
 *
 *  set_trigger_fd() should be called in midi_alsa_handler().
 */

bool
pollwrapper::initialize (snd_seq_t * c, int extra)
{
    bool result { not_nullptr(c) };
    (void) extra;
    if (result)
    {
        m_alsa_client = c;
        result = get_poll_descriptors();        /* extra not needed here    */
        if (result)
            m_is_initialized = true;
    }
    return result;
}

/**
 *  Get the number of MIDI input poll file descriptors.  Allocate the
 *  poll-descriptors array.  Then get the input poll-descriptors into the
 *  array.  Finally, set the input and output buffer sizes.  Can we do this
 *  before creating all the MIDI busses?  If not, we'll put them in a separate
 *  function to call later.
 *
 *  According to https://users.suse.com/~mana/alsa090_howto.html#sect04
 *  snd_seq_poll_descriptors_count(alsa_seq, POLLIN) always returns 1.
 *
 *  midi_alsa_handler() adds one to the count of poll descriptors. Why?
 *  It allocates an extra poll-descriptor, sets POLLIN to the extra
 *  one.
 *
 *  In poll.h, these bitmasks (and more) are defined:
 *
 *      POLLIN      0x0001  Data other than high-priority data may be
 *                          read without blocking.
 *      POLLPRI     0x0002  Data may be written without blocking.
 *      POLLOUT     0x0004  High-priority data may be read without blocking.
 *
 * snd_seq_poll_descriptors_count()
 *
 *  Returns the number of poll descriptors. Accepts a sequencer handle,
 *  the poll events to be checked (POLLIN and POLLOUT or POLLIN|POLLOUT)
 *
 *  Also see the midi_alsa_data::initialize() function and the usage of the
 *  poll descriptors described there.
 *
 * \param extra
 *      A special case for midi_alsa_handler(), which adds and extra descriptor
 *      for some reason.
 *
 * \return
 *      Returns true if the polldescriptors could be initialized.
 */

bool
pollwrapper::get_poll_descriptors (int extra)
{
    int pdcount
    {
        snd_seq_poll_descriptors_count(alsa_client(), POLLIN) // | POLLPRI)
    };
    bool result { pdcount > 0 };
    if (result)
    {
        m_num_poll_descriptors = pdcount + extra;
        m_poll_descriptors = new (std::nothrow) pollfd[num_poll_descriptors()];
        result = not_nullptr(m_poll_descriptors);
        if (result)
        {
            snd_seq_poll_descriptors                /* get input descriptors */
            (
                alsa_client(), m_poll_descriptors + extra,
                num_poll_descriptors() - extra, POLLIN
            );
        }
        else
        {
            m_num_poll_descriptors = 0;
            util::error_message("ALSA poll descriptors failure");
        }
    }
    else
        util::error_message("No ALSA poll descriptors found");

    return result;
}

/**
 *  Sets up the first poll descriptor with a trigger file-descriptor.
 *  Used only by midi_alsa_handler(). Might also be used in the
 *  (not ready) Windows MM implementation.
 *
 *  In midi_alsa_data(), the 2-element triggers_fd array is passed to the
 *  pipe() function.
 */

bool
pollwrapper::set_trigger_fd (int fd)
{
    bool result { fd >= 0 && is_initialized() };
    if (result)
    {
        result = num_poll_descriptors() > 1;
        if (result)
        {
            poll_descriptors()[0].fd = fd;
            poll_descriptors()[0].events = POLLIN;
            m_use_file_descriptor = true;
        }
    }
    return result;
}

/**
 *  Polls for any ALSA MIDI information using a timeout value of 10
 *  milliseconds (c_poll_wait_ms).  Currently there is only 1 poll
 *  descriptor.
 *
 * \return
 *      Returns the result of the call to poll() on the global ALSA poll
 *      descriptors.
 */

int
pollwrapper::poll_for_midi () const
{
    int result { 0 };
    if (is_initialized())
    {
        result = ::poll
        (
            poll_descriptors(), num_poll_descriptors(), c_poll_wait_ms
        );
        if (result >= 0)
        {
            /*
             * The original seq24 code did not do anything with
             * a pipe and reading.
             */
        }
    }
    else
        xpc::millisleep(c_poll_wait_ms);

    return result;
}

/**
 *  Not yet sure what this midi_alsa_handler() functionality is all about.
 */

int
pollwrapper::poll_file_descriptor () const
{
    int result { 0 };
    if (is_initialized() && use_file_descriptor())
    {
        result = ::poll(poll_descriptors(), num_poll_descriptors(), -1);
        if (result >= 0)
        {
            if (poll_descriptors(0)->revents & POLLIN)  /* short */
            {
                bool dummy;
                (void) ::read(poll_descriptors(0)->fd, &dummy, sizeof dummy);
            }
        }
    }
    return result;
}

/**
 *  Removes the poll descriptors.
 */

void
pollwrapper::remove_poll_descriptors ()
{
    if (not_nullptr(m_poll_descriptors))
    {
        struct pollfd * pds { m_poll_descriptors };
        m_poll_descriptors = nullptr;
        m_num_poll_descriptors = 0;
        delete [] pds;
    }
}

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

/*
 * pollwrapper.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

