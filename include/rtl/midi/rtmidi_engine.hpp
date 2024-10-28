#if ! defined RTL66_RTL_MIDI_RTMIDI_ENGINE_HPP
#define RTL66_RTL_MIDI_RTMIDI_ENGINE_HPP

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
 * \file          rtmidi_engine.hpp
 *
 *    Provides a specialized rtmidi class to support the midi::masterbus.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-12-15
 * \updates       2023-07-20
 * \license       See above.
 *
 */

#include <string>
#include <vector>

#include "c_macros.h"                   /* not_nullptr and other macros     */
#include "rtl/rtl_build_macros.h"       /* RTL66_DLL_PUBLIC, etc.           */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi::rtmidi base class   */

namespace midi
{
    class masterbus;
}

namespace rtl
{

/**
 *  This class provides a common, platform-independent API for connections
 *  to a MIDI engine on behalf of an application.  It is meant to be used by
 *  the midi::masterbus class.
 *
 *  It does not support (except accidentally) direct operations on ports,
 *  though as we develope it, port-refresh might be supported.
 */

class RTL66_DLL_PUBLIC rtmidi_engine : public rtmidi
{
    /**
     *  Holds a pointer to the owner of this object.  Only the rtmidi_engine
     *  can reference a masterbus.  Used in the open_midi_api() override.
     */

    midi::masterbus * m_master_bus;   /* note: not rtl::rtmidi! */

public:

    rtmidi_engine
    (
        midi::masterbus * mbus,
        rtmidi::api rapi                = rtmidi::api::unspecified,
        const std::string & clientname  = ""
    );

    rtmidi_engine (rtmidi_engine && other) noexcept :
        rtmidi  (std::move(other))
    {
        // No code
    }

    virtual ~rtmidi_engine () noexcept;

protected:

    virtual bool open_midi_api
    (
        rtmidi::api rapi                = rtmidi::api::unspecified,
        const std::string & clientname  = "",
        unsigned queuesize              = 0
    ) override;

};          // class rtmidi_engine

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_RTMIDI_ENGINE_HPP

/*
 * rtmidi_engine.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

