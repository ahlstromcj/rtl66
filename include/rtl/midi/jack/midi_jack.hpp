#if ! defined RTL66_RTL_MIDI_JACK_HPP
#define RTL66_RTL_MIDI_JACK_HPP

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
 * \file          midi_jack.hpp
 *
 *      Provides the rtl66 JACK implmentation for MIDI input and output.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-01-20
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_JACK

#include <string>                       /* std::string class                */
#include <jack/jack.h>                  /* JACK API functions, etc.         */

#include "midi/ports.hpp"               /* midi::port etc. enums            */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_in/out_api classes     */
#include "rtl/midi/jack/midi_jack_data.hpp"  /* rtl::midi_jack_data class   */

/*
 * Note that here we support two namespaces: midi and rtl::midi.
 */

namespace midi
{
    class event;
}

namespace rtl
{

/*------------------------------------------------------------------------
 * Additional JACK support functions
 *------------------------------------------------------------------------*/

/*
 *  Used for verifying the usability of the API.  If true, the checkports
 *  parameter requires that more than 0 ports must be found.
 */

extern bool detect_jack (bool forcecheck = false);
extern void set_jack_version ();
extern void silence_jack_errors (bool silent);
extern void silence_jack_info (bool silent);
extern void silence_jack_messages (bool silent);

/*------------------------------------------------------------------------
 * JACK callbacks are now defined in the midi_jack_callbacks.cpp module.
 *------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 * midi_jack
 *------------------------------------------------------------------------*/

/**
 *  This class is not a base class, but a mixin class for use with
 *  midi_jack_in and midi_jack_out.  We might eventually refactor it into
 *  a base class, but for now it exists so that the JACK I/O classes
 *  can share some implementation details.
 */

class RTL66_DLL_PUBLIC midi_jack : public midi_api
{

    friend int jack_process_in (jack_nframes_t, void *);
    friend int jack_process_out (jack_nframes_t, void *);
    friend int jack_process_io (jack_nframes_t, void *);
#if defined RTL66_JACK_PORT_REFRESH_CALLBACK
    friend void jack_port_register_callback (jack_port_id_t, int, void *);
#endif
#if defined RTL66_JACK_PORT_CONNECT_CALLBACK
    friend void jack_port_connect_callback
    (
        jack_port_id_t, jack_port_id_t, int, void *
    );
#endif
#if defined RTL66_JACK_PORT_SHUTDOWN_CALLBACK
    friend void jack_shutdown_callback (void *);
#endif

private:

    /**
     *  Moved the client name to this class.
     */

    std::string m_client_name;

    /**
     *  Moved the JACK data to this class, and now it doesn't have to
     *  be allocated, just have it's pointer assigned.
     */

    midi_jack_data m_jack_data;

public:

    midi_jack ();
    midi_jack
    (
        midi::port::io iotype,
        const std::string & clientname  = "",
        unsigned queuesize              = 0
    );
    midi_jack (const midi_jack &) = delete;
    midi_jack & operator = (const midi_jack &) = delete;
    virtual ~midi_jack ();

    virtual rtmidi::api get_current_api () override
    {
        return rtmidi::api::jack;
    }

    const std::string & client_name () const
    {
        return m_client_name;
    }

    midi_jack_data & jack_data ()
    {
        return m_jack_data;
    }

    void client_name (const std::string & cname)
    {
        m_client_name = cname;
    }

protected:

    jack_client_t * client_handle (void * c)
    {
        return reinterpret_cast<jack_client_t *>(c);
    }

    jack_client_t * client_handle ()
    {
        return have_master_bus() ?
            reinterpret_cast<jack_client_t *>(master_bus()->client_handle()) :
            jack_data().jack_client() ;
    }

    /*
     * MIDI engine functions.
     */

    virtual void * void_handle () override
    {
        return reinterpret_cast<void *>(client_handle());
    }

    virtual void * engine_connect () override;
    virtual void engine_disconnect () override;
    virtual bool engine_activate () override;
    virtual bool engine_deactivate () override;
    virtual bool connect () override;
    virtual bool reuse_connection () override;
    virtual int get_port_count () override;
    virtual std::string get_port_name (int portnumber) override;

    /*
     * MIDI port functions
     */

    virtual bool initialize (const std::string & clientname) override;
    virtual bool open_port
    (
        int portnumber,
        const std::string & portname
    ) override;
    virtual bool open_virtual_port (const std::string & portname) override;
    virtual bool close_port () override;
    virtual bool set_client_name (const std::string & clientname) override;
    virtual bool set_port_name (const std::string & portname) override;
    virtual bool send_message (const midi::byte * message, size_t sz) override;
    virtual bool send_message (const midi::message & message) override;

protected:

    /*
     * These functions are deliberately not virtual.
     */

    static midi_jack_data * static_data_cast (void * ptr)
    {
        return reinterpret_cast<midi_jack_data *>(ptr);
    }

    midi_jack_data * data_cast ()
    {
        return reinterpret_cast<midi_jack_data *>(api_data());
    }

    void delete_port ();
    bool create_ringbuffer (size_t rbsize);

    /*--------------------------------------------------------------------
     * Extensions
     *--------------------------------------------------------------------*/

    virtual int get_io_port_info
    (
        midi::ports & inputports, bool preclear = true
    ) override;
    virtual std::string get_port_alias (const std::string & name) override;

#if defined RTL66_MIDI_EXTENSIONS       // defined in Linux, FIXME

    virtual bool PPQN (midi::ppqn ppq) override;
    virtual bool BPM (midi::bpm bp) override;
    virtual bool send_byte (midi::byte evbyte) override;
    virtual bool clock_start () override;
    virtual bool clock_send (midi::pulse tick) override;
    virtual bool clock_stop () override;
    virtual bool clock_continue (midi::pulse tick, midi::pulse beats) override;
    virtual int poll_for_midi () override;
    virtual bool get_midi_event (midi::event * inev) override;
    virtual bool send_event
    (
        const midi::event * ev, midi::byte channel
    ) override;

    bool send_sysex (const midi::event * ev) override;
    bool connect_ports
    (
        midi::port::io iotype,
        const std::string & srcportname,
        const std::string & destportname
    );

#endif  // defined RTL66_MIDI_EXTENSIONS


};          // class midi_jack

}           // namespace rtl

#endif      // defined RTL66_BUILD_JACK

#endif      // RTL66_RTL_MIDI_JACK_HPP

/*
 * midi_jack.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

