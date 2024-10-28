#if ! defined RTL66_MIDI_BUS_HPP
#define RTL66_MIDI_BUS_HPP

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
 * \file          bus.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2016-11-24
 * \updates       2024-06-12
 * \license       GNU GPLv2 or above
 *
 *  The bus module is the new base class for the various implementations
 *  of the bus module.  There is enough commonality to be worth creating a
 *  base class for all such classes.
 *
 *  The bus is an rtmidi_in or rtmidi_out, plus a reference to its owning
 *  masterbus.
 *
 * Rtl66 terminology:
 *
 *      -   App. This is the short name (e.g. "seq66v2") of the application.
 *          See the next definition.
 *      -   Client. The client name is associated with the software access
 *          the ports.  It is usually the application's short name or some
 *          modification of it. It should show up in an ALSA or JACK
 *          connection list.
 *      -   Buss. A buss is considered to be a single piece of equipment
 *          that has one or more ports. In ALSA, the buss number is the first
 *          number in "14:0". The confusion between buss and client comes
 *          partly from ALSA naming.
 *      -   Port. A port is considered to be part of a bus. In ALSA, it is the
 *          second number in "14:0".
 *      -   Bus. While the "buss" and "port" are represented by data structures,
 *          the midi::bus encapsulates both, includes the client name, and
 *          adds other values useful in MIDI I/O.
 *      -   Bus index. This value is an ordinal, starting at 0, used in
 *          midi::bus lookup.
 *
 *      As time goes on, we will try to make the naming rigorous and
 *      consistent.
 */

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */

#include "c_macros.h"                   /* not_nullptr() macro              */
#include "midi/clientinfo.hpp"          /* midi::clientinfo class           */
#include "midi/clocking.hpp"            /* midi::clocking                   */
#include "midi/midibytes.hpp"           /* midi::byte alias, etc.           */
#include "xpc/automutex.hpp"            /* xpc::recmutex recursive mutex    */

#define RTL66_SHOW_BUS_VALUES

namespace rtl
{
    class midi_api;
}

namespace midi
{
    class event;
    class masterbus;

/**
 *  Manifest global constants.
 *
 *  The c_midibus_output_size value is passed, in midi::masterbus, to
 *  snd_seq_set_output_buffer_size().  Not sure if the value needs to be so
 *  large.
 */

const int c_midibus_output_size = 0x100000;     // 1048576

/**
 *  The c_midibus_input_size value is passed, in midi::masterbus,  to
 *  snd_seq_set_input_buffer_size().  Not sure if the value needs to be so
 *  large.
 */

const int c_midibus_input_size  = 0x100000;     // 1048576

/**
 *  Controls the amount a SysEx data sent at one time, in the midibus module.
 */

const int c_midibus_sysex_chunk = 0x100;        // 256

/*--------------------------------------------------------------------------
 * midi::bus base class
 *--------------------------------------------------------------------------*/

/**
 *  This class is base/mixin class for providing Seq66 concepts for
 *  bus_in and bus_out.
 */

class bus
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class masterbus;

public:

    /*
     * These have been move to namespace midi in the ports module.
     *
     *      enum class io
     *      enum class port
     */

    /**
     *  A pointer to a bus object. See the busarray class module.
     */

    using pointer = std::unique_ptr<bus>;

private:

    /**
     *  This is another name for "16 * 4".
     */

    static int m_clock_mod;

    /**
     *  Holds the midi_api pointer for quicker access.
     */

    rtl::midi_api * m_midi_api_ptr;

    /**
     *  The "parent" of this bus.  It's lifetime always completely contains
     *  that of a bus. We want to handle ports/busses coming and going as
     *  devices are plugged in/out of the computer.
     */

    masterbus & m_master_bus;

    /**
     *  Set to true if the bus has been successfully initialized.
     */

    bool m_initialized;

    /**
     *  Provides the index of the bus object in either the input list or
     *  the output list.  Otherwise, it is currently -1.
     */

    const int m_bus_index;

    /**
     *  The client ID of the Seq66 application as determined by the MIDI
     *  subsystem [ALSA, function snd_seq_client_id()]. It is set in the
     *  midi_alsa constructor.  If there are no other MIDI *applications*
     *  running this value will end up being 129.
     *
     *  For example, on one system the client IDs are 14 (MIDI Through),
     *  128 (TiMidity), and 129 (Yoshimi).
     *
     *  For JACK, this is currently set to the same value as the buss ID.
     */

    int m_client_id;

    /**
     *  The buss ID of the bus object as determined by the MIDI subsystem
     *  [ALSA, function snd_seq_client_info_get_client()] The buss ID of the
     *  midibase object represents *other* MIDI devices and applications
     *  (besides Seq66) present at Seq66 startup.  For example, on one system
     *  the IDs are 14 (MIDI Through), 20 (LaunchPad Mini), 128 (TiMidity),
     *  and 129 (Yoshimi).
     *
     *  See ports::get_bus_id() and port::m_client_number.
     */

    int m_bus_id;

    /**
     *  The port ID of the bus object. Numbering starts at 0.
     *
     *  See ports::get_port_id() and port::m_port_number.
     */

    int m_port_id;

    /**
     *  The type of clock to use.  The special value clocking::disabled means
     *  we will not be using the port, so that a failure in setting up the
     *  port is not a "fatal error".  We could have added an "m_outputing"
     *  boolean as an alternative. However, we can overload m_inputing instead.
     */

    clocking m_clock_type;

    /**
     *  This flag indicates if an input or output bus has been selected for
     *  action as an input device (such as a MIDI controller).  It is turned on
     *  if the user enables the port in the Options / MIDI Input tab or the
     *  Options / MIDI Clocks tab.
     *
     *  Setter/getter are port_enabled().
     */

    bool m_io_active;

    /**
     *  Holds the full display name of the bus, index, ID numbers, and item
     *  names.  Assembled by the set_name() function.
     */

    std::string m_display_name;

    /**
     *  The name of the MIDI buss.  This should be something like a major device
     *  name or the name of a subsystem such as Timidity.
     *  See ports::get_bus_name() and port::m_client_number.
     */

    std::string m_bus_name;

    /**
     *  The name of the MIDI port.  This should be the name of a specific device
     *  or port on a major device.  This value, for JACK is reconstructed by
     *  set_alt_name() so that it is essentially the "short" port name that JACK
     *  recognizes. See get_port_name() and ports::m_port_name.
     */

    std::string m_port_name;

    /**
     *  The alias of the MIDI port.  This item is specific to JACK, and is
     *  empty for other APIs.  See get_port_alias() and ports::m_port_alias.
     */

    std::string m_port_alias;

    /**
     *  Indicates if the port is to be an input (versus output) port.
     *  It matters when we are creating the name of the port, where we don't
     *  want an input virtual port to have the same name as an output virtual
     *  port... one of them will fail.
     *
     *  See port::get_input() and port::m_io_type.
     */

    port::io m_io_type;

    /**
     *  Indicates if the port is a system port.  Two examples are the ALSA
     *  System Timer buss and the ALSA System Announce bus, the latter being
     *  necessary for input subscription and notification.  For most ports,
     *  this value will be port::normal.  A restricted setter is provided.
     *  Only the rtmidi ALSA implementation sets up system ports.
     *
     *  See port::get_port_type(), port::get_virtual(), port::get_system(),
     *  and port::m_port_type.
     */

    port::kind m_port_type;

    /**
     *  Locking mutex. This one is based on std:::recursive_mutex.
     */

    xpc::recmutex m_mutex;

public:

    /*
     * The default ctor wouldn't initialize the masterbus reference.
     */

    bus () = delete;
    bus
    (
        masterbus & master,
        int index,
        port::io io_type
    );
    bus (const bus &) = delete;
    bus (bus &&) = delete;
    bus & operator = (const bus &) = delete;
    bus & operator = (bus &&) = delete;
    virtual ~bus ();

#if defined RTL66_SHOW_BUS_VALUES

    static void show_clock (const std::string & context, pulse tick);

    void show_bus_values ();

#endif

    void get_port_items
    (
        std::shared_ptr<clientinfo> mip,
        port::io iotype
    );

    masterbus & master_bus ()
    {
        return m_master_bus;
    }

    const masterbus & master_bus () const
    {
        return m_master_bus;
    }

    bool initialized () const
    {
        return m_initialized;
    }

    /*
     * ----------------------------------------------------------------------
     *  Start of TODOs (from businfo).
     * ----------------------------------------------------------------------
     */

    bool initialize ()                          /* TODO  VIRTUAL? */  
    {
        m_initialized = true;
        return true;
    }

    void activate ()
    {
        m_io_active = true;             // MORE TODO?
    }

    void deactivate ()
    {
        m_io_active = false;            // MORE TODO?
    }

    bool active () const
    {
        return m_io_active;             // conflated with port_enabled()
    }

    /*
     * ----------------------------------------------------------------------
     *  Start of TODOs (from businfo).
     * ----------------------------------------------------------------------
     */

    const std::string & display_name () const
    {
        return m_display_name;
    }

    const std::string & bus_name () const
    {
        return m_bus_name;
    }

    const std::string & port_name () const
    {
        return m_port_name;
    }

    const std::string & port_alias () const
    {
        return m_port_alias;
    }

    std::string connect_name () const;

    int bus_index () const
    {
        return m_bus_index;
    }

    int client_id () const
    {
        return m_client_id;
    }

    int bus_id () const
    {
        return m_bus_id;
    }

    int port_id () const
    {
        return m_port_id;
    }

    /**
     *  Checks if the given parameters match the current bus and port numbers.
     */

    bool match (int b, int p)
    {
        return (m_port_id == p) && (m_bus_id == b);
    }

    port::kind port_type () const
    {
        return m_port_type;
    }

    bool is_virtual_port () const
    {
        return m_port_type == port::kind::manual;
    }

    /**
     *  This function is needed in the rtmidi library to set the is-virtual
     *  flag in the init_*_sub() functions, so that midi_alsa, midi_jack
     *  (and any other additional APIs that end up supported by our
     *  heavily-refactored rtmidi library), as well as the original bus,
     *  can know that they represent a virtual port.
     */

    void is_virtual_port (bool flag)
    {
        if (! is_system_port())
            m_port_type = flag ? port::kind::manual : port::kind::normal;
    }

    port::io io_type () const
    {
        return m_io_type;
    }

    bool is_input_port () const
    {
        return m_io_type == port::io::input;
    }

    bool is_output_port () const
    {
        return m_io_type == port::io::output;
    }

    void is_input_port (bool flag)
    {
        m_io_type = flag ? port::io::input : port::io::output ;
    }

    bool is_system_port () const
    {
        return m_port_type == port::kind::system;
    }

    bool is_port_connectable () const;
    bool set_clock (clocking clocktype);

    bool is_port_locked () const
    {
        return false;                               /* Windows only FIXME   */
    }

    clocking clock_type () const
    {
        return m_clock_type;
    }

    void clock_type (clocking c)
    {
        m_clock_type = c;
    }

    bool port_enabled () const                      /* replaces get_input() */
    {
        return m_io_active;
    }

    bool clock_enabled () const
    {
        return midi::clock_enabled(m_clock_type);   /* pos and mod enabled  */
    }

    bool port_unavailable () const
    {
        return m_clock_type == clocking::unavailable;
    }

    void port_enabled (bool flag)                   /* set_io_status(bool)  */
    {
        m_io_active = flag;
    }

    /**
     *  Useful for setting the buss ID when using the rtmidi_info object to
     *  create a list of busses and ports.  Would be protected, but midi_alsa
     *  needs to change this value to reflect the user-client ID actually
     *  assigned by ALSA.  (That value ranges from 128 to 191.)
     */

    void set_bus_id (int id)
    {
        m_bus_id = id;
    }

    void set_client_id (int id)
    {
        m_client_id = id;
    }

public:

    void display_name (const std::string & name)
    {
        m_display_name = name;
    }

    void bus_name (const std::string & name)
    {
        m_bus_name = name;
    }

    void port_name (const std::string & name)
    {
        m_port_name = name;
    }

    /**
     *  Useful for setting the port ID when using the rtmidi_info object to
     *  inspect and create a list of busses and ports.
     */

    void set_port_id (int id)
    {
        m_port_id = id;
    }

    void set_name
    (
        const std::string & appname,
        const std::string & busname,
        const std::string & portname
    );
    void set_alt_name
    (
        const std::string & appname,
        const std::string & busname
    );

    /**
     *  Set the clock mod to the given value, if legal.
     *
     * \param clockmod
     *      If this value is not equal to 0, it is used to set the static
     *      member m_clock_mod.
     */

    static void set_clock_mod (int clockmod)
    {
        if (clockmod != 0)
            m_clock_mod = clockmod;
    }

    /**
     *  Get the global clock mod value.
     */

    static int get_clock_mod ()
    {
        return m_clock_mod;
    }

    /*
     *  These functions use the midi_api pointer set appropriately in the
     *  bus_in and bus_out constructors.
     */

    midi::ppqn PPQN () const;
    midi::bpm BPM () const;

    /*----------------------------------------------------------------------
     * Virtual functions
     *----------------------------------------------------------------------*/

    virtual bool connect ();                        /* common to in/out tho */

    /*----------------------------------------------------------------------
     * Input functions
     *----------------------------------------------------------------------*/

    virtual int get_in_port_info ()
    {
        return 0;
    }

    virtual bool init_input (bool inputing)         /* coded in bus_in      */
    {
        (void) inputing;
        return false;
    }

    virtual int poll_for_midi ()
    {
        return 0;
    }

    virtual bool get_midi_event (event * inev)
    {
        (void) inev;
        return false;
    }

    /*----------------------------------------------------------------------
     * Output functions
     *----------------------------------------------------------------------*/

    virtual int get_out_port_info ()
    {
        return 0;
    }

    virtual bool init_clock (pulse tick)            /* coded in bus_out     */
    {
        (void) tick;
        return false;
    }

    virtual bool send_event (const event * e24, midi::byte channel)
    {
        (void) e24;
        (void) channel;
        return false;
    }

    virtual bool send_sysex (const event * e24)
    {
        (void) e24;
        return false;
    }

    virtual bool clock_start ()
    {
        return false;
    }

    virtual bool clock_stop ()
    {
        return false;
    }

    virtual bool clock_send (pulse tick)
    {
        (void) tick;
        return false;
    }

    /*
     *  This function is implemented as in Seq66's midibase::continue_from()
     *  function. See bus_out::clock_continue().
     */

    virtual bool clock_continue (pulse tick)
    {
        (void) tick;
        return false;
    }

#if defined THIS_CODE_IS_READY
    virtual bool open_port ();
    virtual bool close_port ();
    virtual bool flush_port ();
    virtual bool get_port_name();
    virtual bool get_port_alias();
    virtual bool auto_connect ();
    virtual bool send_byte ();
    virtual bool send_status ();
    virtual bool send_message ();
    virtual bool get_message ();
#endif

    void print ();

protected:

    /*
     * Overridden in bus_in and bus_out to provide, through rtmidi_in or
     * rtmidi_out, the pointer to the instantiated midi_api class.
     */

    rtl::midi_api * midi_api_ptr ()
    {
        return m_midi_api_ptr;
    }

    const rtl::midi_api * midi_api_ptr () const
    {
        return m_midi_api_ptr;
    }

    void set_midi_api_ptr (rtl::midi_api * rmap);

};          // class bus

}           // namespace midi

#endif      // RTL66_MIDI_BUS_HPP

/*
 * bus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

