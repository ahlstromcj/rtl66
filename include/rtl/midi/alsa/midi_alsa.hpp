#if ! defined RTL66_RTL_MIDI_ALSA_HPP
#define RTL66_RTL_MIDI_ALSA_HPP

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
 * \file          midi_alsa.hpp
 *
 *      Provides the rtl66 ALSA implmentation for MIDI input and output.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2024-05-14
 * \license       See above.
 *
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#if defined RTL66_BUILD_ALSA

#include <string>                       /* std::string class                */

#include "midi/ports.hpp"               /* midi::port etc. enums            */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_in/out_api classes     */
#include "rtl/midi/alsa/midi_alsa_data.hpp"  /* rtl::midi_alsa_data class   */

#undef  RTL66_USE_SEQ66_EXTENSIONS

namespace midi
{
    class event;
}

namespace rtl
{

/*
 *  Used for verifying the usability of the API.  If true, the checkports
 *  parameter requires that more than 0 ports must be found.
 */

extern bool detect_alsa (bool checkports);
extern void set_alsa_version ();

/*------------------------------------------------------------------------
 * midi_alsa
 *------------------------------------------------------------------------*/

/**
 *  This class is not a base class, but a mixin class for use with
 *  midi_alsa_in and midi_alsa_out.  We might eventually refactor it into
 *  a base class, but for now it exists so that the JACK I/O classes
 *  can share some implementation details.
 */

class RTL66_DLL_PUBLIC midi_alsa : public midi_api
{
    friend void * midi_alsa_handler (void *);

private:

    /**
     *  Moved the client name to this class.
     */

    std::string m_client_name;

    /**
     *  Moved the ALSA data to this class.
     */

    midi_alsa_data m_alsa_data;

public:

    midi_alsa ();
    midi_alsa
    (
        midi::port::io iotype,
        const std::string & clientname  = "",
        unsigned queuesize              = 0
    );
    midi_alsa (const midi_alsa &) = delete;
    midi_alsa & operator = (const midi_alsa &) = delete;
    virtual ~midi_alsa ();

    virtual rtmidi::api get_current_api () override
    {
        return rtmidi::api::alsa;
    }

    const std::string & client_name () const
    {
        return m_client_name;
    }

    midi_alsa_data & alsa_data ()
    {
        return m_alsa_data;
    }

    void client_name (const std::string & cname)
    {
        m_client_name = cname;
    }

protected:

    snd_seq_t * client_handle (void * c)
    {
        return reinterpret_cast<snd_seq_t *>(c);
    }

    snd_seq_t * client_handle ()
    {
        return have_master_bus() ?
            reinterpret_cast<snd_seq_t *>(master_bus()->client_handle()) :
            alsa_data().alsa_client() ;
    }

    virtual void * void_handle () override
    {
        return reinterpret_cast<void *>(client_handle());
    }

    virtual void * engine_connect () override;
    virtual void engine_disconnect () override;

    /*
     * Not a feature of ALSA.
     *
     *      virtual bool engine_activate () override;
     *      virtual bool engine_deactivate () override;
     */

    virtual bool connect () override;
    virtual bool reuse_connection () override;
    virtual bool initialize (const std::string & clientname) override;
    virtual bool open_port
    (
        int number                  = 0,
        const std::string & name    = ""
    ) override;

    virtual bool open_virtual_port (const std::string & name = "") override;
    virtual bool close_port () override;
    virtual bool set_client_name (const std::string & clientname) override;
    virtual bool set_port_name (const std::string & name) override;
    virtual int get_port_count () override;
    virtual std::string get_port_name (int number) override;
    virtual bool flush_port () override;
    virtual bool send_message (const midi::byte * message, size_t sz) override;
    virtual bool send_message (const midi::message & message) override
    {
        return send_message(message.data_ptr(), message.size());
    }

private:

    /*--------------------------------------------------------------------
     * Extensions
     *--------------------------------------------------------------------*/

    virtual int get_io_port_info
    (
        midi::ports & inputports, bool preclear = true
    ) override;

    /*
     * virtual std::string get_port_alias (const std::string & name) override;
     */

#if defined RTL66_MIDI_EXTENSIONS       // defined in Linux, FIXME

    virtual bool PPQN (midi::ppqn ppq) override;
    virtual bool BPM (midi::bpm bp) override;
    virtual bool send_byte (midi::byte evbyte) override;
    virtual bool clock_start () override;
    virtual bool clock_send (midi::pulse tick) override;
    virtual bool clock_stop () override;
    virtual bool clock_continue (midi::pulse tick, midi::pulse beats) override;

    /*
     * virtual int poll_for_midi () override;
    */

    /*
     * The ALSA poll_for_midi() function is not implemented at this time.
     */

    virtual bool get_midi_event (midi::event * inev) override;
    virtual bool send_event
    (
        const midi::event * ev, midi::byte channel
    ) override;

#if defined RTL66_ALSA_REMOVE_QUEUED_ON_EVENTS
    void remove_queued_on_events (int tag);
#endif

#endif  // defined RTL66_MIDI_EXTENSIONS

protected:

    /*
     * These functions are deliberately not virtual.
     */

    static midi_alsa_data * static_data_cast (void * ptr)
    {
        return reinterpret_cast<midi_alsa_data *>(ptr);
    }

    midi_alsa_data * data_cast ()
    {
        return reinterpret_cast<midi_alsa_data *>(api_data());
    }

    /*
     * Helper functions to enhance readability.
     */

    void delete_port ();
    void close_input_triggers ();
    bool drain_output ();
    bool set_seq_tempo_ppqn (snd_seq_t * seq, midi::bpm bp, midi::ppqn ppq);
    bool set_seq_client_name (snd_seq_t * seq, const std::string & clientname);
    bool setup_input_port ();
    bool setup_input_virtual_port ();
    bool subscription
    (
        std::string & errmsg,
        snd_seq_addr_t & sender,
        snd_seq_addr_t & receiver
    );
    bool remove_subscription ();
    bool start_input_thread (rtmidi_in_data & indata);
    bool join_input_thread ();

};          // class midi_alsa

}           // namespace rtl

#endif      // defined RTL66_BUILD_ALSA

#endif      // RTL66_RTL_MIDI_ALSA_HPP

/*
 * midi_alsa.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

