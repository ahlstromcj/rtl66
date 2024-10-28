#if ! defined RTL66_RTL_MIDI_RTMIDI_IN_HPP
#define RTL66_RTL_MIDI_RTMIDI_IN_HPP

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
 * \file          rtmidi_in.hpp
 *
 *    A reworking of RtMidi.hpp, to avoid circular dependencies involving
 *    rtmidi_in.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-07-19
 * \license       See above.
 *
 */

#include <string>
#include <vector>

#include "c_macros.h"                   /* not_nullptr and other macros     */
#include "rtl/rtl_build_macros.h"       /* RTL66_DLL_PUBLIC, etc.           */
#include "midi/message.hpp"             /* midi::message                    */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi base class           */
#include "rtl/midi/rtmidi_in_data.hpp"  /* rtl::rtmidi_in_data class        */
#include "rtl/rterror.hpp"              /* rterror::callback_t              */

namespace rtl
{

/**
 *  This class provides a common, platform-independent API for realtime MIDI
 *  input.  It allows access to a single MIDI input port.  Incoming MIDI
 *  messages are either saved to a queue for retrieval using the get_message()
 *  function or immediately passed to a user-specified callback function.
 *  Create multiple instances of this class to connect to more than one MIDI
 *  device at the same time.  With the OS-X, Linux ALSA, and JACK MIDI APIs, it
 *  is also possible to open a virtual input port to which other MIDI software
 *  clients can connect.
 */

class RTL66_DLL_PUBLIC rtmidi_in : public rtmidi
{

public:

    rtmidi_in
    (
        api rapi                        = api::unspecified,
        const std::string & clientname  = "",
        unsigned queuesizelimit         = 0
    );

    rtmidi_in (rtmidi_in && other) noexcept :
        rtmidi  (std::move(other))
    {
        // No code
    }

    virtual ~rtmidi_in () noexcept;

    /*
     * Defined in rtmidi now:  virtual api get_current_api () noexcept;
     */

    virtual bool open_port
    (
        int portnumber = 0,
        const std::string & portname = ""
    ) override;
    virtual bool open_virtual_port (const std::string & portname = "") override;
    virtual void set_buffer_size (size_t sz, int count);    /* not an override */

    void set_input_callback
    (
        rtmidi_in_data::callback_t callback,
        void * userdata = nullptr
    );
    void cancel_input_callback ();
    void ignore_midi_types
    (
        bool midisysex  = true,
        bool miditime   = true,
        bool midisense  = true
    );
    double get_message (midi::message & msg);

protected:

    virtual bool open_midi_api
    (
        api rapi                        = api::unspecified,
        const std::string & clientname  = "",
        unsigned queuesize              = 0
    ) override;

};          // class rtmidi_in

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_RTMIDI_IN_HPP

/*
 * rtmidi_in.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

