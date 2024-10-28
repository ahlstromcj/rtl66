#if ! defined RTL66_RTL_MIDI_WIN_MM_HPP
#define RTL66_RTL_MIDI_WIN_MM_HPP

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
 * \file          midi_win_mm.hpp
 *
 *  This module supports the Windows Multimedia MIDI API.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2023-07-20
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_WIN_MM

#include <string>                       /* std::string class                */

#include "midi/ports.hpp"               /* midi::port etc.  enums           */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_in/out_api classes     */
#include "rtl/midi/winmm/midi_win_mm_data.hpp" /* rtl::midi_win_mm_data etc */

namespace midi
{
    class ports;
}

namespace rtl
{

/*
 *  Used for verifying the usability of the API.
 */

extern bool detect_win_mm ();

/*------------------------------------------------------------------------
 * midi_win_mm
 *------------------------------------------------------------------------*/

class RTL66_DLL_PUBLIC midi_win_mm : public midi_api
{

    friend void CALLBACK midi_input_callback
    (
        HMIDIIN, UINT, DWORD_PTR, DWORD_PTR, DWORD
    );

private:

    /**
     *  Moved the client name to this class.
     */

    std::string m_client_name;

    /**
     *  Moved the ALSA data to this class.
     */

    midi_win_mm_data m_win_mm_data;

public:

    midi_win_mm ();
    midi_win_mm
    (
        midi::port::io iotype,
        const std::string & clientname  = "",
        unsigned queuesize              = 0
    );
    midi_win_mm (const midi_win_mm &) = delete;
    midi_win_mm & operator = (const midi_win_mm &) = delete;
    virtual ~midi_win_mm ();

    virtual rtmidi::api get_current_api () override
    {
        return rtmidi::api::windows_mm;
    }

    const std::string & client_name () const
    {
        return m_client_name;
    }

    midi_win_mm_data & win_mm_data ()
    {
        return m_win_mm_data;
    }

    void client_name (const std::string & cname)
    {
        m_client_name = cname;
    }

protected:

    HMIDIIN client_in_handle ()
    {
        win_mm_data().in_client();
    }

    HMIDIOUT client_out_handle ()
    {
        win_mm_data().out_client();
    }

    /*
     * ARE THESE RIGHT and USEFUL?
     */

    HANDLE client_handle ()
    {
        if (have_master_bus())
        {
            return reinterpret_cast<HANDLE>(master_bus()->client_handle());
        }
        else
        {
            return is_input() ? client_in_handle() : client_out_handle() ;
        }
    }

    virtual void * void_handle ()
    {
        return is_input() ?
            reinterpret_cast<void *>(client_in_handle()) :
            reinterpret_cast<void *>(client_out_handle()) ;
    }

    /*
     * TODO:  implement this one
     */

    virtual bool connect () override;
    {
        return true;
    }

    // virtual bool reuse_connection () override;

    virtual bool initialize (const std::string & clientname) override;
    virtual bool open_port
    (
        int number                  = 0,
        const std::string & name    = ""
    ) override;

    virtual bool open_virtual_port (const std::string & name = "") override;
    virtual bool close_port () override;
    virtual int get_port_count () override;
    virtual std::string get_port_name (int number) override;

    // virtual std::string get_port_alias (const std::string & /*name*/)

    virtual int get_io_port_info
    (
        midi::ports & inputports, bool preclear = true
    ) override
    {
        (void) inputports;
        (void) preclear;
        return 0;   // TODO
    }

    virtual bool set_client_name (const std::string & clientname) override;
    virtual bool set_port_name (const std::string & name) override;

    // virtual void set_error_callback (rterror::callback_t cb, void * userdata);

    virtual bool send_message (const midi::byte * message, size_t sz) override;

    virtual bool send_message (const midi::message & message) override
    {
        return send_message(message.data(), message.size());
    }

protected:

    /*
     * These functions are deliberately not virtual.
     */

    static midi_win_mm_data * static_data_cast (void * ptr)
    {
        return reinterpret_cast<midi_win_mm_data *>(ptr);
    }

    midi_win_mm_data * data_cast ()
    {
        return reinterpret_cast<midi_win_mm_data *>(api_data());
    }

};          // class midi_win_mm

}           // namespace rtl

#endif      // defined RTL66_BUILD_WIN_MM

#endif      // RTL66_RTL_MIDI_WIN_MM_HPP

/*
 * midi_win_mm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

