#if ! defined RTL66_RTL_MIDI_MACOSX_CORE_HPP
#define RTL66_RTL_MIDI_MACOSX_CORE_HPP

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
 * \file          midi_macosx_core.hpp
 *
 *      Support the Mac OSX Core MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-07-20
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"           /* RTL66_BUILD_ALSA, etc.       */

#if defined RTL66_BUILD_MACOSX_CORE

#include <string>                           /* std::string class            */
#include <CoreMIDI/CoreMIDI.h>

#include "rtl/midi/rtmidi.hpp"              /* rtl::rtmidi, midi_api class  */

namespace midi
{
    class ports;
}

namespace rtl
{

/*
 *  Used for verifying the usability of the API.
 */

extern bool detect_core ();

/*------------------------------------------------------------------------
 * midi_core
 *------------------------------------------------------------------------*/

class RTL66_DLL_PUBLIC midi_core final : public midi_api
{

public:

    midi_core (const std::string & clientname, unsigned queuesizelimit);
    virtual ~midi_core ();

    virtual rtmidi::api get_current_api () override
    {
        return rtmidi::api::macosx_core;
    }

    virtual bool open_port
    (
        int portnumber, const std::string & portname
    ) override;
    virtual bool open_virtual_port (const std::string & portname) override;
    virtual bool close_port () override;
    virtual bool set_client_name (const std::string & clientname) override;
    virtual bool set_port_name (const std::string & portname) override;
    virtual int get_port_count () override;

    virtual int get_io_port_info
    (
        midi::ports & inputports, bool preclear = true
    ) override
    {
        (void) inputports;
        (void) preclear;
        return 0;   // TODO
    }

    virtual std::string get_port_name (int portnumber) override;

protected:

    void * client_handle ()
    {
        // TODO
        // return have_master_bus() ?
        //     reinterpret_cast<core_data_t *>(master_bus()->client_handle()) :
        //     core_data().core_client() ;

        return nullptr;
    }

    virtual void * void_handle () override
    {
        return reinterpret_cast<void *>(client_handle());
    }

    MIDIClientRef get_core_midi_client_singleton
    (
        const std::string & clientname
    ) noexcept;

    virtual bool initialize (const std::string & clientname) override;

    virtual bool connect () override
    {
        return true;
    }

};          // class midi_core_in

}           // namespace rtl

#endif      // defined RTL66_BUILD_MACOSX_CORE

#endif      // RTL66_RTL_MIDI_MACOSX_CORE_HPP

/*
 * midi_macosx_core.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

