#if ! defined RTL66_RTL_RTMIDI_OUT_HPP
#define RTL66_RTL_RTMIDI_OUT_HPP

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
 * \file          rtmidi_out.hpp
 *
 *    A reworking of RtMidi.hpp, to avoid circular dependencies involving
 *    rtmidi_in.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-09-14
 * \license       See above.
 *
 */

#include <string>
#include <vector>

#include "c_macros.h"                   /* not_nullptr and other macros     */
#include "midi/midibytes.hpp"           /* midi::byte and other aliases     */
#include "rtl/rtl_build_macros.h"       /* RTL66_DLL_PUBLIC, etc.           */
#include "rtl/rterror.hpp"              /* rterror::callback_t              */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi base class           */

namespace midi
{
    class masterbus;
}

namespace rtl
{

/**
 *  A realtime MIDI output class.  This class provides a common,
 *  platform-independent API for MIDI output.  It allows one to probe
 *  available MIDI output ports, to connect to one such port, and to send MIDI
 *  bytes immediately over the connection.  Create multiple instances of this
 *  class to connect to more than one MIDI device at the same time.  With the
 *  OS-X, Linux ALSA and JACK MIDI APIs, it is also possible to open a virtual
 *  port to which other MIDI software clients can connect.
 */

class RTL66_DLL_PUBLIC rtmidi_out : public rtmidi
{

public:

    /**
     *  Default constructor that allows an optional client name.
     *  An exception will be thrown if a MIDI system initialization error
     *  occurs.  If no API argument is specified and multiple API support has
     *  been compiled, the default order of use is ALSA, JACK (Linux) and
     *  CORE, JACK (OS-X).
     */

    rtmidi_out                          /* default & principal constructor   */
    (
        api rapi                        = api::unspecified,
        const std::string & clientname  = ""
    );
    rtmidi_out (const midi::masterbus & mb);
    rtmidi_out (const rtmidi_out & other) = delete;
    rtmidi_out & operator = (rtmidi_out & other) = delete;
    rtmidi_out (rtmidi_out && other) = default;
    rtmidi_out & operator = (rtmidi_out && other) = default;
    virtual ~rtmidi_out ();

    /*
     * Defined in rtmidi:  virtual api get_current_api () noexcept;
     */

    virtual bool open_port
    (
        int portnumber = 0,
        const std::string & portname = ""
    ) override;
    virtual bool open_virtual_port (const std::string & portname = "") override;

protected:

    virtual bool open_midi_api
    (
        api rapi                        = api::unspecified,
        const std::string & clientname  = "",
        unsigned queuesize              = 0         /* used for input only  */
    ) override;
    virtual bool open_midi_api (const midi::masterbus & mb) override;

};          // class rtmidi_out

}           // namespace rtl

#endif      // RTL66_RTL_RTMIDI_OUT_HPP

/*
 * rtmidi_out.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

