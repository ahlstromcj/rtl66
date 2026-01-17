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
 * \updates       2026-01-15
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
 * get_message ()                                   x
 * set_buffer_size (size_t sz, int count)           x
 * set_error_callback (callback_t, void * data)     x    x
 * send_message (midi_message & msg)                x    x
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

#include "rtl/rtl_build_macros.h"       /* RTL_DEFAULT_PPQN, _DEFAULT_BPM   */
#include "rtl/rt_types.hpp"             /* rtl::rtmidi::api enum class      */
#include "midi/busarray.hpp"            /* midi::busarray a la Seq66        */
#include "midi/clientinfo.hpp"          /* midi::clientinfo                 */
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

class masterbus final
{
    /*
     * Let's make certain calls public, rather than have a bunch of
     * friends.
     *
     *  friend class player;
     *  friend class poller;
     *  friend class track;
     */

public:

    /**
     *  The clientinfo class provides application-specific information,
     *  some desired settings for the run, the list of existing system
     *  MIDI ports, a void pointer to a "MIDI handle", etc.
     *
     *      using info = std::unique_ptr<midi::clientinfo>;
     *      using info = midi::clientinfo::pointer;    // shared_ptr<>
     *
     *  Do we need a pointer? No.
     */

    using info = midi::clientinfo;

private:

    /**
     *  Seq66 rc() and setup items:
     *
     *      -   ppqn
     *      -   bpm
     *      -   manual_ports(). Boolean for using virtual ports.
     *      -   manual_auto_enable(). TO INVESTIGATE.
     *      -   manual_port_count(). Number of output virtual ports.
     *      -   manual_in_port_count(). Number of input virtual ports.
     *      -   midi_master().full_port_count()
     *      -   midi_master().get_port_count() [mode in or out]
     *      -   midi_master().selected_api()
     *      -   with_jack_midi()
     *      -   app_client_name()
     */

    /**
     *  The API to use.  For now we will assume some prior code to validate
     *  the API.  We may need to select it here, though.
     */

    rtl::rtmidi::api m_selected_api { rtl::rtmidi::api::unspecified };

    /**
     *  Provides a pointer to the selected API implementation.
     *  See the accessors. However, we can get this from the
     *  rtmidi_engine.
     *
     *      rtl::midi_api * m_rt_api_ptr;
     */

    /**
     *  Encapsulates information about the input busses.
     */

    midi::busarray m_inbus_array { };

    /**
     *  Encapsulates information about the output busses.
     */

    midi::busarray m_outbus_array { };

    /**
     *  For "dumping" MIDI input to a track for recording.  This value
     *  is set to true when a sequence editor window is open and the user
     *  has clicked the "record MIDI" or "thru MIDI" button.  See the
     *  set_sequence_input() function.
     */

    bool m_dumping_input { false };

    /**
     *  Points to the sequence object.  Set in set_sequence_input().  See that
     *  function's description.
     */

    midi::track * m_input_track { nullptr };

    /**
     *  The locking mutex.  This object is passed to an automutex object that
     *  lends exception-safety to the mutex locking.
     */

    mutable xpc::recmutex m_mutex { };

    /**
     *  The "global" client handle, stored here so we do not recreate it every
     *  time.
     */

    void * m_void_client_handle { nullptr };

    /**
     *  The MIDI API client ID.???
     */

    int m_client_id { 0 };          /* not -1 ??? */

    /**
     *  The maximum number of busses (ports) supported.
     *
     * int m_max_busses { c_busscount_max };
     */

    /**
     *  This is a midi::clientinfo object. No longer a pointer.
     *  Common code access for input and output, especially the MIDI
     *  client handles. If these are not null, that changes the original
     *  RtMidi API to handle our Seq66-style API. These items include
     *  application wide values (e.g. app name) as well a the list of
     *  existing ports on the system.
     */

    info m_client_info { };

    /**
     *  Provides access to the selected API in order to hook up to the desired
     *  MIDI engine as a client and perform key operations on it.
     *
     * \tricky
     *      This member must come last because it also sets client information
     *      members.
     */

    rtl::rtmidi_engine m_engine;            /* default ctor is deleted      */

    /**
     *  Indicates that setup has already been done. Call the clear() function
     *  to start over.
     */

    bool m_is_setup { false };

public:

    masterbus () = delete;
    masterbus
    (
        rtl::rtmidi::api rapi,
        const midi::clientinfo & ci
    );
    masterbus (const masterbus &) = delete;
    masterbus (masterbus &&) = delete;              /* forced by recmutex   */
    masterbus & operator = (const masterbus &) = delete;
    masterbus & operator = (masterbus &&) = delete;

    /*
     * We do not need virtual functions at all here, because all
     * non-data functionality is offloaded to the rtl66 API.
     */

    ~masterbus ()
    {
        // No other code needed
    }

    void * void_client_handle ()
    {
        return m_void_client_handle;
    }

    rtl::rtmidi_engine & engine ()
    {
        return m_engine;
    }

    const rtl::rtmidi_engine & engine () const
    {
        return m_engine;
    }

    /**
     *  Remember that the "info" type is a shared_ptr. In this usage
     *  it cannot be a unique_ptr.
     */

    info & client_info ()
    {
        return m_client_info;
    }

    const info & client_info () const
    {
        return m_client_info;
    }

    /**
     *  Some accessor for ease of use.
     */

    rtl::rtmidi::api selected_api () const
    {
        return m_selected_api;
    }

    midi::port::io port_type () const
    {
        return client_info().port_type();
    }

    bool use_input_thread () const
    {
        return client_info().use_input_thread();
    }

    void cancel_input_thread ()
    {
        client_info().use_input_thread(false);
    }

    const std::string & client_name () const
    {
        return client_info().client_name();
    }

    int queue_size () const
    {
        return client_info().queue_size();
    }

    bool is_setup () const
    {
        return m_is_setup;
    }

    bool setup ();
    void clear ();
    bool client_info_reset ();
    bool client_info_reset (clientinfo & cinfo);
    std::string port_io_listing () const;
    void print () const;                    // redundant?

    /**
     *  Indicates if the global client handle has been set.
     */

    bool info_is_connected () const
    {
        return client_info().is_connected();
    }

    const midi::busarray & inbus_array () const
    {
        return m_inbus_array;
    }

    midi::busarray & inbus_array ()
    {
        return m_inbus_array;
    }

    const midi::busarray & outbus_array () const
    {
        return m_outbus_array;
    }

    midi::busarray & outbus_array ()
    {
        return m_outbus_array;
    }

    /**
     *  Gets either a good midi:bus reference or a reference to a dummy
     *  bus. Useful mainly for testing.
     */

    midi::bus_in & get_in_bus (int index)
    {
        return inbus_array().buss_in(bussbyte(index));
    }

    midi::bus_out & get_out_bus (int index)
    {
        return outbus_array().buss_out(bussbyte(index));
    }

    int get_num_out_buses () const
    {
        return outbus_array().count();
    }

    int get_num_in_buses () const
    {
        return inbus_array().count();
    }

    void void_client_handle (void * clienthandle);

    int client_id () const
    {
        return m_client_id;
    }

    midi::bpm BPM () const
    {
        return client_info().global_bpm();
    }

    midi::ppqn PPQN () const
    {
        return client_info().global_ppqn();
    }

    input_specs & get_input_specs ()
    {
        return client_info().get_input_specs();
    }

    const input_specs & get_input_specs () const
    {
        return client_info().get_input_specs();
    }

    /**
     *  Gets the application index (Seq66-style buss number).
     */

    int get_port_id (port::io iotype, int bussno, int portno) const
    {
        return client_info().get_port_id(iotype, bussno, portno);
    }

    /*
     * These rt_api_ptr() functions are duplicates of those in the
     * rtmidi class.
     */

    rtl::midi_api * rt_api_ptr ()
    {
        return engine().rt_api_ptr();
    }

    const rtl::midi_api * rt_api_ptr () const
    {
        return engine().rt_api_ptr();
    }

    void * api_data ()
    {
        return engine().api_data();
    }

    const void * api_data () const
    {
        return engine().api_data();
    }

public:

    void selected_api (rtl::rtmidi::api rapi)
    {
        m_selected_api = rapi;
    }

    void set_client_id (int id)
    {
        m_client_id = id;
    }

    void play_and_flush (midi::bussbyte bus, event * e24, midi::byte channel);

    /**
     *  Test the sequencer to see if any more input is pending.  Calls the
     *  implementation-specific API function.
     *
     *  Note that the ALSA implementation calls a single "input-pending" function,
     *  while the PortMidi implementation loops through all of the input midibus
     *  objects, calling the poll_for_midi() function of each.
     *
     * \threadsafe
     *
     * \return
     *      Returns true if ALSA is supported, and the returned size is greater
     *      than 0, or false otherwise.
     */

    bool is_more_input () const
    {
        return poll_for_midi() > 0;
    }

    bool activate ();

public:     // public because used in test applications

    midi::bus * make_bus
    (
        int busno,
        midi::port::io iotype,
        int qsize = RTL66_DEFAULT_INPUT_Q_SIZE
    );
    bool engine_initialize ();
    bool engine_initialize (const clientinfo & ci);
    bool engine_query ();
    bool engine_activate ();
    bool engine_deactivate ();
    std::string port_list (midi::port::io iotype) const;
    int choose_port                     /* similar to rt_choose_port()      */
    (
        midi::port::io iotype,
        int & portcount,
        bool showalloption = true
    );

private:    // API pass-alongs

    void * engine_connect ();
    void engine_disconnect ();

private:

    bool setup (clientinfo & cinfo);

public:     // API implementations

    /*
     * This function replaces start(), continue_from(), etc.
     */

    bool handle_clock (midi::clock::action act, midi::pulse ts = 0);

    bool PPQN (midi::ppqn ppq);
    bool BPM (midi::bpm bp);
    bool flush ();
    bool flush_port (midi::bussbyte b);
    bool panic (int displaybuss = (-1));
    bool sysex (midi::bussbyte bus, const event * ev);
    void play (midi::bussbyte bus, event * e24, midi::byte channel);
    bool set_clock (midi::bussbyte bus, midi::clocking clocktype);
    bool save_clock (midi::bussbyte bus, midi::clocking clock);
    midi::clocking get_clock (midi::bussbyte bus) const;
    bool save_input (midi::bussbyte bus, bool inputing);
    bool set_input (midi::bussbyte bus, bool inputing);
    bool get_input (midi::bussbyte bus) const;
    std::string get_midi_bus_name
    (
        midi::bussbyte bus, midi::port::io iotype
    ) const;
    int poll_for_midi () const;
    int poll_port (int portnumber) const;
    bool get_midi_event (midi::event * inev);
    midi::message get_message (int portnumber = RTL66_PORTS_ALL);
    bool port_start (int client, int port);     // TODO
    bool port_exit (int client, int port);      // TODO
    bool set_track_input (bool state, midi::track * trk);

#if defined THIS_CODE_IS_READY
    void dump_midi_input (midi::event ev);
    void api_set_ppqn_and_beats_per_minute (midi::ppqn,  midi::bpm);
    void api_init (midi::ppqn, midi::bpm); // = 0;
    void api_start ();
    void api_continue_from (midi::pulse);
    void api_init_clock (midi::pulse);
    void api_stop ();
    void api_port_start (int /* client */, int /* port */);
    bool api_get_midi_event (midi::event * inev) ; //= 0;
#endif

    /*
     *  So far, there is no need for these API-specific functions.
     *
     *  api_init_in_sub()
     *  api_init_out_sub()
     *  api_deinit_in()
     *  api_deinit_out()
     *
     *  void api_sysex (const midi::event * ev) = 0;
     *  void api_play
     *  (
     *      midi::bussbyte bus, const midi::event * e24, midi::byte channel
     *  ) = 0;
     *  void api_set_clock (midi::bussbyte bus, clocking clocktype) = 0;
     *  void api_get_clock (midi::bussbyte bus) = 0;
     *  void api_set_input (midi::bussbyte bus, bool inputting) = 0;
     *  void api_get_input (midi::bussbyte bus) = 0;
     */

};          // class masterbus

}           // namespace midi

#endif      // RTL66_MIDI_MASTERBUS_HPP

/*
 * masterbus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

