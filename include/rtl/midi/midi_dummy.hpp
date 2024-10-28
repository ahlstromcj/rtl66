#if ! defined RTL66_RTL_MIDI_MIDI_DUMMY_HPP
#define RTL66_RTL_MIDI_MIDI_DUMMY_HPP

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
 * \file          midi_dummy.hpp
 *
 *      Provides a non-functional module, possibly useful for testing and
 *      debuggin.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-06-01
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_DUMMY

#include <string>                       /* std::string class                */

#include "rtl/midi/midi_api.hpp"        /* rtl::midi_in/out_api classes     */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi, midi_api class      */

namespace rtl
{

/*
 *  Used for verifying the usability of the API.
 */

extern bool detect_dummy ();

/*------------------------------------------------------------------------
 * midi_dummy
 *------------------------------------------------------------------------*/

class RTL66_DLL_PUBLIC midi_dummy final : public midi_api
{

public:

    midi_dummy () = default;
    midi_dummy
    (
        const std::string & /*cliname*/,
        unsigned queuesizelimit = 0
    );
    midi_dummy (const midi_dummy &) = delete;
    midi_dummy (midi_dummy &&) = delete;
    midi_dummy & operator = (const midi_dummy &) = delete;
    midi_dummy & operator = (midi_dummy &&) = delete;

    virtual ~midi_dummy ()
    {
        // no code needed
    }

    virtual rtmidi::api get_current_api () override
    {
        return rtmidi::api::dummy;
    }

    virtual bool PPQN (midi::ppqn /*ppq*/) override
    {
        return true;
    }

    virtual bool BPM (midi::bpm /*bp*/) override
    {
        return true;
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
        return 0;
    }

    virtual std::string get_port_name (int /*number*/) override
    {
        return std::string("");
    }

protected:

    void * client_handle ()
    {
        return nullptr;
    }

    virtual void * void_handle () override
    {
        return nullptr;
    }

    virtual bool initialize (const std::string & /*name*/) override
    {
        return true;
    }

    virtual bool connect () override
    {
        return true;
    }

    virtual bool send_message
    (
        const midi::byte * /*msg*/, size_t /*sz*/
    ) override
    {
        return true;
    }

    virtual bool send_message (const midi::message & /*msg*/) override
    {
        return true;
    }

};          // class midi_dummy

}           // namespace rtl

#endif      // defined RTL66_BUILD_DUMMY

#endif      // RTL66_RTL_MIDI_MIDI_DUMMY_HPP

/*
 * midi_dummy.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

