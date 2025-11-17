#if ! defined RTL66_RTL_POLLWRAPPER_HPP
#define RTL66_RTL_POLLWRAPPER_HPP

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
 * \file          pollwrapper.hpp
 *
 *    Object for holding the current status of ALSA and ALSA MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2025-11-17
 * \updates       2025-11-17
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
#include "midi/ports.hpp"               /* midi::port::io type              */

namespace rtl
{

/**
 *  A class to encapsulate the calls to the poll(2) interface.
 */

class pollwrapper
{

    friend class midi_alsa;
    friend void * midi_alsa_handler (void *);

private:

    bool m_is_initialized { false };

    /**
     *  The number of descriptors for polling.
     */

    int m_num_poll_descriptors { 0 };

    /**
     *  Points to the list of descriptors for polling.
     */

    struct pollfd * m_poll_descriptors { nullptr };

    /**
     *  The caller's ALSA client pointer.
     */

    snd_seq_t * m_alsa_client { nullptr };

    /**
     *  Indicates that the caller has set up to poll a file descriptor
     *  wired as the first ALSA descriptor. This value is set to true
     *  in set_trigger_fd().
     */

    bool m_use_file_descriptor { false };

public:

    pollwrapper () = default;
    pollwrapper (snd_seq_t * client, int extra = 0);
    pollwrapper (const pollwrapper &) = default;
    pollwrapper (pollwrapper &&) = default;
    pollwrapper & operator = (const pollwrapper &) = default;
    pollwrapper & operator = (pollwrapper &&) = default;
    ~pollwrapper ();

    bool is_initialized () const
    {
        return m_is_initialized;
    }
    bool set_trigger_fd (int fd);       /* a special case */

private:

    bool initialize (snd_seq_t * c);

    void alsa_client (snd_seq_t * c)
    {
        m_alsa_client = c;
    }

    snd_seq_t * alsa_client ()
    {
        return m_alsa_client;
    }

    const snd_seq_t * alsa_client () const
    {
        return m_alsa_client;
    }

    int num_poll_descriptors () const
    {
        return m_num_poll_descriptors;
    }

#if 0
    struct pollfd * poll_descriptors (int index = 0)
    {
        bool ok { is_initialized() && index < num_poll_descriptors() };
        return ok ? m_poll_descriptors + index : nullptr ;
    }
#endif

    struct pollfd * poll_descriptors (int index = 0) const
    {
        bool ok { is_initialized() && index < num_poll_descriptors() };
        return ok ? m_poll_descriptors + index : nullptr ;
    }

    bool use_file_descriptor () const
    {
        return m_use_file_descriptor;
    }

    bool get_poll_descriptors (int extra = 0);
    int poll_for_midi () const;
    int poll_file_descriptor () const;
    void remove_poll_descriptors ();

};          // class pollwrapper

}           // namespace rtl

#endif      // RTL66_BUILD_ALSA

#endif      // RTL66_RTL_POLLWRAPPER_HPP

/*
 * pollwrapper.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

