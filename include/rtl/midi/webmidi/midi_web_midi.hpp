#if ! defined RTL66_RTL_MIDI_WEB_MIDI_HPP
#define RTL66_RTL_MIDI_WEB_MIDI_HPP

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
 * \file          midi_web_midi.hpp
 *
 *      Support for the Web MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-07-20
 * \license       See above.
 *
 * https://developer.mozilla.org/en-US/docs/Web/API/Web_MIDI_API
 *
 *      "The Web MIDI API connects to and interacts with Musical Instrument
 *      Digital Interface (MIDI) Devices.  The interfaces deal with the
 *      practical aspects of sending and receiving MIDI messages. Therefore,
 *      the API can be used for musical and non-musical uses, with any MIDI
 *      device connected to your computer."
 */

#include "rtl/rtl_build_macros.h"           /* RTL66_EXPORT, etc.           */

#if defined RTL66_BUILD_WEB_MIDI

#include <string>                           /* std::string class            */

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

extern bool detect_web_midi ();

/*------------------------------------------------------------------------
 * midi_web_midi
 *------------------------------------------------------------------------*/

class RTL66_DLL_PUBLIC web_midi_shim
{

public:

    web_midi_shim();
    ~web_midi_shim();

    std::string get_port_name (int portnumber, bool isinput);

};

class RTL66_DLL_PUBLIC midi_web final : public midi_api
{

private:

    std::string client_name {};
    std::string web_midi_id {};
    int open_port_number {-1};

public:

    midi_web (const std::string & clientname, unsigned queuesizelimit);
    virtual ~midi_web ();

    virtual rtmidi::api get_current_api () override
    {
        return rtmidi::api::web;
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

    void on_midi_message (uint8_t * data, double domHishResTimeStamp);

protected:

    /*
     * TODO
     */

    void * client_handle ()
    {
        // TODO:
        // return have_master_bus() ?
        //     reinterpret_cast<webmidi_data_t *>
        //     (
        //          webmidi_client(master_bus()->client_handle())
        //     ) :
        //     webmidi_data().webmidi_client() ;

        return nullptr;
    }

    virtual void * void_handle () override
    {
        return reinterpret_cast<void *>(client_handle());
    }

    virtual bool initialize (const std::string & clientname) override;

    virtual bool connect () override
    {
        return true;
    }

};          // class midi_web

}           // namespace rtl

#endif      // defined RTL66_BUILD_WEB_MIDI

#endif      // RTL66_RTL_MIDI_WEB_MIDI_HPP

/*
 * midi_web_midi.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

