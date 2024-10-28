#if ! defined RTL66_MIDI_MASTERBUS_HPP
#define RTL66_MIDI_MASTERBUS_HPP

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
 * \file          masterbus.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2016-11-23
 * \updates       2024-06-09
 * \license       GNU GPLv2 or above
 *
 *  The masterbus module is the base-class version of the mastermidi::bus
 *  module.
 *
 *  Summary of RtMidi API functions available:
 *
 *                                                                   Bus
 *      Function                                   In   Out  Master In  Out
 *
 * open_port (int number, string name)              x    x     ?
 * open_virtual_port (string name)                  x    x
 * set_input_callback (callback_t, void * data)     x
 * cancel_input_callback ()                         x
 * flush_port ()                                    ?    x
 * close_port ()                                    x    x
 * is_port_open () const                            x    x
 * get_port_count ()                                x    x
 * get_port_name (int portnumber)                   x    x
 * get_port_alias (string name)                     -    - ADD TO rtmidi
 * get_io_port_info (midi::ports &, bool clear)     x    x     x     x   x
 * ignore_midi_types (bool, bool, bool)             x
 * get_message (midi::message & message)            x
 * set_buffer_size (size_t sz, int count)           x
 * set_error_callback (callback_t, void * data)     x    x
 * send_message (midi_message & message)            x    x
 * send_message (const midi::byte *, size_t sz)          x
 *
 * set_master_bus()                                                  x   x
 *
 * get_current_api () noexcept
 * set_client_name (string clientname)
 * set_port_name (string portname)
 *
 * client_activate ()                                          x
 * client_deactivate ()                                        x
 * connect()                                       NEED TO ADD TO rtmidi
 * initialize (string clientname)                  NEED TO ADD TO rtmidi
 *
 * poll_for_midi ()                                NEED TO ADD TO rtmidi
 * get_midi_event (event * inev)                   Ditto for all the rest
 * send_event (event * ev, midi::byte channel)
 * send_sysex (event * ev)
 * send_continue_from (pulse, pulse beats)
 * send_start ()
 * send_stop ()
 * send_clock (pulse)
 * send_byte (byte evbyte)
 * send_byte (status evstatus)
 * set_ppqn (ppqn ppq)
 * set_beats_per_minute (bpm bp)
 * connect_ports (iotype...)
 */

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */

#include "rtl/rtl_build_macros.h"       /* RTL_DEFAULT_PPQN, _DEFAULT_BPM   */
#include "rtl/rt_types.hpp"             /* rtl::rtmidi::api enum class      */
#include "midi/busarray.hpp"            /* midi::busarray a la Seq66        */
#include "midi/clientinfo.hpp"          /* midi::clientinfo a la Seq66      */
#include "midi/clocking.hpp"            /* midi::clock::action enumertion   */
#include "rtl/midi/rtmidi_engine.hpp"   /* rtl::rtmidi_engine class         */
#include "xpc/recmutex.hpp"             /* xpc::recmutex                    */

namespace rtl
{
    class midi_api;                     /* forward reference for pointer    */
}

namespace midi
{
    class event;
    class track;

/**
 *  The class that "supervises" all of the MIDI I/O ports.  It provides
 *  many similar-named functions as midi_api, but it is not based on it.
 *  Instead, it provides a midi_api pointer to pass the calls along to an
 *  rtmidi_engine (new feature) object.
 */

class masterbus
{
    friend class player;

private:

    /**
     *  The API to use.  For now we will assume some prior code to validate
     *  the API.  We may need to select it here, though.
     */

    rtl::rtmidi::api m_selected_api;

    /**
     *  Provides a pointer to the selected API implementation.
     *
     *  CURRENTLY NOT REALLY USED. See the accessors.`
     */

    rtl::midi_api * m_rt_api_ptr;

    /**
     *  Provides access to the selected API in order to hook up to the desired
     *  MIDI engine as a client and perform key operations on it.
     */

    rtl::rtmidi_engine m_engine;

    /**
     *  Encapsulates information about the input busses.
     */

    busarray m_inbus_array;

    /**
     *  Encapsulates information about the output busses.
     */

    busarray m_outbus_array;

    /**
     *  The locking mutex.  This object is passed to an automutex object that
     *  lends exception-safety to the mutex locking.
     */

    mutable xpc::recmutex m_mutex;

    /**
     *  The "global" client handle, stored here so we do not recreate it every
     *  time.
     */

    void * m_client_handle;

    /**
     *  The MIDI API client ID.???
     */

    int m_client_id;

    /**
     *  The maximum number of busses (ports) supported.
     */

    int m_max_busses;

    /**
     *  Common code access for input and output, especially the MIDI
     *  client handles. If these are not null, that changes the original
     *  RtMidi API to handle our Seq66-style API. These items include
     *  application wide values (e.g. app name) as well a the list of
     *  existing ports on the system.
     */

    std::shared_ptr<midi::clientinfo> m_client_info;

    /**
     *  Main resolution in parts per quarter note.
     */

    midi::ppqn m_ppqn;

    /**
     *  BPM (beats per minute).
     */

    midi::bpm m_beats_per_minute;

public:

    masterbus () = delete;
    masterbus
    (
        rtl::rtmidi::api rapi,
        midi::ppqn ppq  = 192,
        midi::bpm bp    = 120.0
    );
    masterbus (const masterbus &) = delete;
    masterbus (masterbus &&) = delete;          /* forced by recmutex :-(   */
    masterbus & operator = (const masterbus &) = delete;
    masterbus & operator = (masterbus &&) = delete;         /* ditto */

    virtual ~masterbus ()
    {
        // No other code needed
    }

    rtl::rtmidi::api selected_api () const
    {
        return m_selected_api;
    }

    void * client_handle ()
    {
        return m_client_handle;
    }

    void * void_handle ()
    {
        return client_handle();
    }

    rtl::rtmidi_engine & engine ()
    {
        return m_engine;
    }

    const rtl::rtmidi_engine & engine () const
    {
        return m_engine;
    }

    std::shared_ptr<midi::clientinfo> client_info ()
    {
        return m_client_info;
    }

    const std::shared_ptr<midi::clientinfo> client_info () const
    {
        return m_client_info;
    }

    std::string port_listing () const;

    int get_num_out_buses () const
    {
        return m_outbus_array.count();
    }

    int get_num_in_buses () const
    {
        return m_inbus_array.count();
    }

    void client_handle (void * clienthandle)
    {
        m_client_handle = clienthandle;
    }

    /**
     *  Indicates if the global client handle has been set.
     */

    bool info_is_connected () const
    {
        bool result = bool(m_client_info);
        if (result)
            result = m_client_info->is_connected();

        return result;
    }

    int client_id () const
    {
        return m_client_id;
    }

    midi::bpm BPM () const
    {
        return m_beats_per_minute;
    }

    midi::ppqn PPQN () const
    {
        return m_ppqn;
    }

protected:

    rtl::midi_api * rt_api_ptr ()
    {
        return m_rt_api_ptr;
    }

    const rtl::midi_api * rt_api_ptr () const
    {
        return m_rt_api_ptr;
    }

    void rt_api_ptr (rtl::midi_api * p)
    {
        m_rt_api_ptr = p;
    }

    void set_client_id (int id)
    {
        m_client_id = id;
    }

    void play_and_flush (midi::bussbyte bus, event * e24, midi::byte channel);
    bool is_more_input () const
    {
        return poll_for_midi() > 0;
    }

public:     // used in a test application

    virtual bool engine_initialize
    (
        midi::ppqn ppq  = RTL66_DEFAULT_PPQN,
        midi::bpm bp    = RTL66_DEFAULT_BPM
    );
    virtual bool engine_query ();

protected:  // API pass-alongs

    virtual bool engine_activate ();
    virtual bool engine_connect ();
    virtual bool engine_make_busses
    (
        bool autoconnect, int inputport, int outputport
    );

protected:  // API implementations

    virtual bool PPQN (midi::ppqn ppq);
    virtual bool BPM (midi::bpm bp);
    virtual bool handle_clock (midi::clock::action act, midi::pulse ts = 0);
    virtual bool flush ();
    virtual bool panic (int displaybuss = (-1));
    virtual bool sysex (midi::bussbyte bus, const event * ev);
    virtual void play (midi::bussbyte bus, event * e24, midi::byte channel);
    virtual bool set_clock (midi::bussbyte bus, midi::clocking clocktype);
    virtual bool save_clock (midi::bussbyte bus, midi::clocking clock);
    virtual midi::clocking get_clock (midi::bussbyte bus) const;

    // TODO or derived classes
    //
    // void masterbus::copy_io_busses ();
    // void get_port_statuses (clockslist & outs, inputslist & ins);
    // void get_out_port_statuses (clockslist & outs);
    // void get_in_port_statuses (inputslist & ins);

    virtual bool save_input (midi::bussbyte bus, bool inputing);
    virtual bool set_input (midi::bussbyte bus, bool inputing);
    virtual bool get_input (midi::bussbyte bus) const;
    virtual std::string get_midi_bus_name
    (
        midi::bussbyte bus, midi::port::io iotype
    ) const;
    virtual int poll_for_midi () const;
    virtual bool port_start (int client, int port);     // TODO
    virtual bool port_exit (int client, int port);      // TODO
    virtual bool set_track_input (bool state, track * trk);
    virtual void dump_midi_input (event ev);

#if defined THIS_CODE_IS_READY
    virtual void api_set_ppqn_and_beats_per_minute (midi::ppqn,  midi::bpm);
    virtual void api_init (midi::ppqn, midi::bpm); // = 0;
    virtual void api_start ();
    virtual void api_continue_from (midi::pulse);
    virtual void api_init_clock (midi::pulse);
    virtual void api_stop ();
    virtual void api_port_start (int /* client */, int /* port */);
    virtual bool api_get_midi_event (event * inev) ; //= 0;
#endif

    /*
     *  So far, there is no need for these API-specific functions.
     *
     *  api_init_in_sub()
     *  api_init_out_sub()
     *  api_deinit_in()
     *  api_deinit_out()
     *
     *  virtual void api_sysex (const event * ev) = 0;
     *  virtual void api_play
     *  (
     *      midi::bussbyte bus, const event * e24, midi::byte channel
     *  ) = 0;
     *  virtual void api_set_clock (midi::bussbyte bus, clocking clocktype) = 0;
     *  virtual void api_get_clock (midi::bussbyte bus) = 0;
     *  virtual void api_set_input (midi::bussbyte bus, bool inputting) = 0;
     *  virtual void api_get_input (midi::bussbyte bus) = 0;
     */

};          // class masterbus

}           // namespace midi

#endif      // RTL66_MIDI_MASTERBUS_HPP

/*
 * masterbus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

