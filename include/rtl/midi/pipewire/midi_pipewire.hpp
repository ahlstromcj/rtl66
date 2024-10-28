#if ! defined RTL66_RTL_MIDI_PIPEWIRE_HPP
#define RTL66_RTL_MIDI_PIPEWIRE_HPP

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
 * \file          midi_pipewire.hpp
 *
 *      PipeWire interface to come in the future.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-17
 * \updates       2023-03-15
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"           /* RTL66_EXPORT, etc.           */

#if defined RTL66_BUILD_PIPEWIRE

#include <string>                           /* std::string class            */

#include "rtl/midi/rtmidi.hpp"              /* rtl::rtmidi, midi_api class  */

namespace rtl
{

/*
 *  Used for verifying the usability of the API.
 */

extern bool detect_pipewire ();

/*------------------------------------------------------------------------
 * midi_pipewire
 *------------------------------------------------------------------------*/

class RTL66_DLL_PUBLIC midi_pipewire final : public midi_api
{

public:

    midi_pipewire () = default;
    midi_pipewire (const std::string & clientname, unsigned queuesize);
    midi_pipewire (const midi_pipewire &) = delete;
    midi_pipewire & operator = (const midi_pipewire &) = delete;

    virtual ~midi_pipewire ()
    {
        // no code needed
    }

    virtual api get_current_api () override
    {
        return api::pipewire;
    }

    virtual bool open_port
    (
        int /*number*/, const std::string & /*name*/
    ) override
    {
        return true;
    }

    virtual bool open_virtual_port (const std::string & /*name*/) override
    {
        return true;
    }

    virtual bool close_port () override
    {
        return true;
    }

    virtual bool set_client_name (const std::string & /*name*/) override
    {
        return true;
    }

    virtual bool set_port_name (const std::string & /*name*/) override
    {
        return true;
    }

    virtual int get_port_count () override
    {
        return 0;
    }

    virtual int get_io_port_info
    (
        midi::ports & inputports, bool preclear = true
    ) override
    {
        (void) inputports;
        (void) preclear;
        return 0;   // TODO
    }

    virtual std::string get_port_name (int /*number*/) override
    {
        return std::string("");
    }

protected:

    /*
     * TODO
     */

    void * client_handle ()
    {
        // TODO
        // return have_master_bus() ?
        //     reinterpret_cast<pipewire *>(master_bus()->client_handle()) :
        //     pipe_data().pipe_client() ;

        return nullptr;
    }

    virtual void * void_handle () override
    {
        return reinterpret_cast<void *>(client_handle());
    }

    virtual bool initialize (const std::string & /*name*/) override
    {
        // no code
    }

    virtual bool connect () override
    {
        return true;
    }

};          // class midi_pipewire

}           // namespace rtl

#endif      // defined RTL66_BUILD_PIPEWIRE

#endif      // RTL66_RTL_MIDI_PIPEWIRE_HPP

/*
 * midi_pipewire.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

