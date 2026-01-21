#if ! defined RTL66_RTL_MIDI_API_HPP
#define RTL66_RTL_MIDI_API_HPP

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
 * \file          midi_api.hpp
 *
 *      The base class for classes that due that actual MIDI work for the MIDI
 *      framework selected at run-time.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2026-01-18
 * \license       See above.
 *
 *      This class is mostly similar to the original RtMidi MidiApi class, but
 *      adds some additional (and optional) features, plus a slightly simpler
 *      class hierarchy that can support both input and output in the same
 *      port.
 */

#include "rtl/rtl_build_macros.h"       /* RTL66_EXPORT, etc.               */

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */
#include <string>                       /* std::string class                */

#include "rtl/api_base.hpp"             /* rtl::api_base class              */
#include "midi/masterbus.hpp"           /* midi::masterbus                  */
#include "midi/message.hpp"             /* midi::message class              */
#include "midi/ports.hpp"               /* midi::ports                      */
#include "rtl/midi/rtmidi_in_data.hpp"  /* rtl::rtmidi_in_data class        */

/*
 * Note the support for the namespaces of midi and rtl.
 */

namespace midi
{
    class bus;
    class bus_in;
    class bus_out;
    class event;
    class masterbus;
}

namespace rtl
{

/*------------------------------------------------------------------------
 * midi_api
 *------------------------------------------------------------------------*/

/**
 *  Subclasses of midi_api will contain all API- and OS-specific code
 *  necessary to fully implement the modified rtmidi API.
 *
 *  Note that midi_api is an abstract base class and cannot be explicitly
 *  instantiated.  rtmidi_in and rtmidi_out will create instances of the
 *  proper API-implementing subclass.
 */

class RTL66_DLL_PUBLIC midi_api : public api_base
{
    /*
     * Concepts added to the rtl66 library.
     */

    friend class midi::bus;
    friend class midi::bus_in;
    friend class midi::bus_out;
    friend class midi::masterbus;

    /*
     * Concepts adapted from the RtMidi library.
     */

    friend class rtmidi;
    friend class rtmidi_in;
    friend class rtmidi_out;

private:

    /**
     *  The type of port: input, output, duplex, or engine.
     */

    midi::port::io m_port_io_type { midi::port::io::engine };

    /**
     *  Holds the port number (an index starting at 0). Defaults to
     *  -1, and set in open_port().
     */

    int m_port_number { -1 };

    /**
     *  Data for usage by input ports.  Among the items it contains are a
     *  midi_queue, midi::message, a void pointer to an API-specific data
     *  structure (such as midi_alsa_data and midi_jack_data), and buffer
     *  information.
     *
     *  It also contains flags for first-message, continue-sysex, ignoring
     *  certain events, allowing input, and using a callback function (with a
     *  pointer to user data).
     *
     *  It's mutable so that we can call the pop const function.
     *
     *  The size if the default structure is under 100 bytes.
     */

    mutable rtmidi_in_data m_input_data { };

    /**
     *  Holds optional application-wide information about the MIDI ports.
     *  Can be used to prevent multiple client connections, for example.
     *  Normally this pointer is null, but the masterbus can set this pointer.
     *  When set, access to the MIDI engine handle (e.g. snd_seq_t or
     *  jack_client_t) is done through the masterbus, without constantly
     *  recreating the client (and its callbacks). The masterbus destructor
     *  will unset this pointer.  Note that we do not want a shared pointer,
     *  since the masterbus owns this pointer.
     */

    midi::masterbus * m_master_bus { nullptr }; /* a paradigm changer!!!    */

    /**
     *  Quicker than checking the pointer.
     */

    bool m_has_master { false };

    /**
     *  Data specific to each API.  Includes the client handle as an exact
     *  type. Might point to a member variable.
     */

    void * m_api_data { nullptr };

    /**
     *  Indicates if the port (or client???) is connected and usable.
     */

    bool m_is_connected { false };

    /**
     *  Current input queue size, if applicable.
     */

    int m_queue_size { 0 };

    /**
     *  Optional MIDI tempo queue. Set only by the ALSA API.
     */

    int m_midi_tempo_queue { -1 };

public:

    /*
     * The default constructor does nothing but use the in-class
     * definition.  The masterbus constructor changes how the API
     * works; it is more akin to how Seq66 does it. The queuesize
     * parameter is meant only for input.
     */

    midi_api () = default;
    midi_api
    (
        midi::masterbus & mbus,
        midi::port::io iotype,
        bool formastersetup = false
    );
    midi_api (midi::port::io iotype, unsigned queuesize = 0);
    midi_api (const midi_api &) = delete;
    midi_api (midi_api &&) = delete;
    midi_api & operator = (const midi_api &) = delete;
    midi_api & operator = (midi_api &&) = delete;
    virtual ~midi_api ();

public:

    /*
     * Seq66-style MIDI concepts. These elements, if present, allow for
     * global access to MIDI engine ("client") information, including the
     * client handles (e.g. jack_client_t or snd_seq_t) and the current set
     * of MIDI ports active on the system.  The midi::masterbus pointer is not
     * owned by the midi_api objects.
     */

    bool has_master () const
    {
        return m_has_master;                /* not_nullptr(master_bus())    */
    }

    midi::masterbus * master_bus ()
    {
        return m_master_bus;
    }

    const midi::masterbus * master_bus () const
    {
        return m_master_bus;
    }

    void master_bus (midi::masterbus * mb)
    {
        m_master_bus = mb;
        if (not_nullptr(mb))
            m_has_master = true;
    }

    midi::port::io port_io_type () const
    {
        return m_port_io_type;
    }

    std::string port_io_string () const
    {
        return CSTR(midi::io_to_string(port_io_type()));
    }

    bool is_input () const
    {
        return m_port_io_type == midi::port::io::input ||
            m_port_io_type == midi::port::io::duplex;
    }

    bool is_output () const
    {
        return m_port_io_type == midi::port::io::output ||
            m_port_io_type == midi::port::io::duplex;
    }

    bool is_duplex () const
    {
        return m_port_io_type == midi::port::io::duplex;
    }

    bool is_engine () const
    {
        return m_port_io_type == midi::port::io::engine;
    }

    bool is_dummy () const
    {
        return m_port_io_type == midi::port::io::dummy;
    }

    bool is_finder () const
    {
        return m_port_io_type == midi::port::io::finder;
    }

    int port_number () const
    {
        return m_port_number;
    }

    /*
     * These accessors return nullptr unless have_master_bus() returns true
     * and the masterbus has them.
     */

    const midi::clientinfo * client_info () const;

    bool master_is_connected () const;

    /*
     * Provides the client handle; cast to the proper type before usage.
     * Or create a non-virtual API-specific function, like the following, to
     * do the job:
     *
     *  jack_client_t * client_handle ()
     *  {
     *      return reinterpret_cast<jack_client_t *>(void_client_handle());
     *  }
     */

    virtual void * void_client_handle () = 0;
    virtual void void_client_handle (void *) = 0;
    virtual midi::ppqn PPQN () const;
    virtual midi::bpm BPM () const;

protected:

    int midi_tempo_queue () const
    {
        return m_midi_tempo_queue;
    }

    void midi_tempo_queue (int mtq)
    {
        m_midi_tempo_queue = mtq;
    }

    virtual void close_midi_tempo_queue ()
    {
        // implemented only in the ALSA MIDI API
    }

public:

    /*
     * Functions provided by the "rtmidi" API.  Similar to the originals, but
     * with different naming conventions.
     */

    void * api_data ()
    {
        return m_api_data;
    }

    const void * api_data () const
    {
        return m_api_data;
    }

    void api_data (void * vp)
    {
        m_api_data = vp;
    }

    /*
     * virtual functions applying to the client. Only some APIs support the
     * concept of Audio/MIDI engine activation.
     */

    virtual void * engine_connect ()
    {
        return nullptr;     // need to add to ALSA, WinMM, MacOSX, etc.
    }

    virtual void engine_disconnect ()
    {
        // no code by default
    }

    virtual bool engine_activate ()
    {
        return true;
    }

    virtual bool engine_deactivate ()
    {
        return true;
    }

    /*
     * Overridden in derived classes to indicate which MIDI engine API is
     * in force.
     */

    virtual rtmidi::api get_current_api () = 0;

    rtmidi_in_data & input_data ()
    {
        return m_input_data;
    }

    const rtmidi_in_data & input_data () const
    {
        return m_input_data;
    }

    bool use_internal_thread () const
    {
        return input_data().use_internal_thread();
    }

    void cancel_internal_thread ()
    {
        input_data().cancel_internal_thread();
    }

    static rtmidi_in_data * static_in_data_cast (void * vp)
    {
        return reinterpret_cast<rtmidi_in_data *>(vp);
    }

protected:

    /**
     * Pure virtual functions to support general MIDI input or output port
     * operations. The connect() function creates and logs a client-handle to
     * be used by a new port.
     */

    virtual bool connect () = 0;

    /**
     *  Attempts to use the existing client connection, if any. This is done
     *  to avoid creating multiple client handles when only one is needed.
     *  This is kind of an extension to RtMidi.
     *
     *  Returns false if there is no connection to re-use.
     */

    virtual bool reuse_connection ()
    {
        return false;
    }

    /**
     *  This function finishes the initialization of a client connection.
     */

    virtual bool initialize (const std::string & clientname ) = 0;
    virtual bool open_port
    (
        int number                  = 0,
        const std::string & name    = ""
    ) = 0;
    virtual bool open_virtual_port (const std::string & name = "") = 0;
    virtual bool close_port () = 0;
    virtual int get_port_count () = 0;
    virtual std::string get_port_name (int number) = 0;
    virtual bool set_client_name (const std::string & clientname) = 0;
    virtual bool set_port_name (const std::string & name) = 0;

    virtual bool is_port_open () const
    {
        return m_is_connected;
    }

    /*--------------------------------------------------------------------
     * Extensions
     *--------------------------------------------------------------------*/

    virtual int get_io_port_info
    (
        midi::ports & portsout, bool preclear = true
    ) = 0;

    /*
     * Gets an alternate name for the port. Currently supported only in some
     * versions of JACK. Up to two aliases can be set for each port.
     */

    virtual std::string get_port_alias
    (
        const std::string & name,
        int aliasno = 0
    )
    {
        static std::string s_dummy;
        (void) name;
        (void) aliasno;
        return s_dummy;
    }

    virtual lib66::tokenization get_port_aliases (const std::string & name)
    {
        static lib66::tokenization s_dummy;
        (void) name;
        return s_dummy;
    }

#if defined RTL66_MIDI_EXTENSIONS       // defined in Linux, FIXME

    /*
     * Not virtual because send_byte() is virtual.
     */

    bool send_status (midi::status evstatus)
    {
        return send_byte(midi::to_byte(evstatus));
    }

    virtual bool PPQN (midi::ppqn ppq)
    {
        (void) ppq;
        return false;
    }

    virtual bool BPM (midi::bpm bp)
    {
        (void) bp;
        return false;
    }

    virtual bool send_byte (midi::byte evbyte) const = 0;
    virtual bool send_event
    (
        const midi::event * ev,
        midi::byte channel = midi::null_channel()
    ) const = 0;
    virtual bool send_message (const midi::message & msg) const = 0;
    virtual bool send_message (const midi::bytes & msg) const = 0;
    virtual bool send_message (const midi::byte * msg, size_t sz) const = 0;
    virtual bool send_sysex (const midi::event * ev) const = 0;

    virtual bool clock_start ()
    {
        return false;
    }

    virtual bool clock_send (midi::pulse tick)
    {
        (void) tick;
        return false;
    }

    virtual bool clock_stop ()
    {
        return false;
    }

    virtual bool clock_continue
    (
        midi::pulse tick,
        midi::pulse beats = 0
    )
    {
        (void) tick;
        (void) beats;
        return false;
    }

    virtual int poll_for_midi () const
    {
        return 0;
    }

    virtual bool get_midi_event (midi::event * inev)
    {
        (void) inev;
        return false;
    }

    virtual midi::message get_message ();

#endif  // defined RTL66_MIDI_EXTENSIONS

    bool is_connected () const
    {
        return m_is_connected;
    }

    void is_connected (bool flag)
    {
        m_is_connected = flag;
    }

    int queue_size () const
    {
        return m_queue_size;
    }

protected:

    /*
     * Functions to support input ports.  The RtMidi class hierarchy that
     * uses separate input and output ports requires a certain amount of
     * duplicate code.  We have move a number of virtual functions into
     * the rtmidi base class, and also created non-virtual functions
     * here,
     */

    void ignore_midi_types
    (
        bool midisysex, bool miditime, bool midisense
    );
    void set_buffer_size (size_t sz, int count);
    void set_input_callback
    (
        rtmidi_in_data::callback_t cb, void * userdata
    );
    void cancel_input_callback ();

    void port_number (int n)
    {
        m_port_number = n;
    }

protected:

    /*
     * ALSA supports flush for output.  JACK does not.  Rather than a raft of
     * simplistic overrides, provide no functionality by default. Also,
     * for ALSA, this is a whole-client action. Both provided for
     * potential usage in other APIs.
     */

    virtual bool flush ()
    {
        return true;
    }

    virtual bool flush_port (midi::bussbyte b)
    {
        (void) b;
        return true;
    }

};          // class midi_api

}           // namespace rtl

#endif      // RTL66_RTL_MIDI_API_HPP

/*
 * midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

