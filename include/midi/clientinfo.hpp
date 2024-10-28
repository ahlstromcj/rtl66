#if ! defined RTL66_MIDI_CLIENTINFO_HPP
#define RTL66_MIDI_CLIENTINFO_HPP

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
 * \file          clientinfo.hpp
 *
 *  A class for holding the current status of the MIDI system on the host,
 *  which includes additional information over what rtmidi holds.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-05
 * \updates       2024-06-01
 * \license       See above.
 *
 *  We need to have a way to get all of the API information from each
 *  framework, without supporting the full API.  The midi::masterbus and
 *  midi::bus classes require certain information to be known when they are
 *  created:
 *
 *      -   Port counts.  The number of input ports and output ports needs to
 *          be known so that we can iterate properly over them to create
 *          midi::bus objects.
 *      -   Port information.  We want to assemble port names just once, and
 *          never have to deal with it again (assuming that MIDI ports do not
 *          come and go during the execution of the application).
 *      -   Client information.  We want to assemble client names or numbers
 *          just once.
 *
 *  Note that, while the other midi_api-based classes access port via the port
 *  numbers assigned by the MIDI subsystem, midi::clientinfo-based classes use
 *  the concept of an "index", which ranges from 0 to one less than the number
 *  of input or output ports.  These values are indices into a vector of
 *  port_info structures, and are easily looked up when midi::masterbus
 *  creates a midi::bus object.
 *
 *  Unlike the Seq66 version of midi::clientinfo, this information applies
 *  only to input or output, not to both.  Don't want to have a "mode bit"
 *  anymore (see Tracy Kidder's "Soul of a New Machine"). However, apart from
 *  the ports and the values, the information that is common to both
 *  input and output is:
 *
 *      -   The MIDI client handle. A JACK handle, ALSA handle, etc.
 *      -   Queue number.  ALSA only at present.
 *      -   Error messages.
 */

#include <cmath>                        /* std::nearbyint()                 */

#include "midi/midibytes.hpp"           /* midi::ppqn, midi::bpm            */
#include "midi/ports.hpp"               /* midi::ports, etc.                */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi::api::unspecified    */

/**
 *  For investigative use. Do we really need global client info, when there
 *  is no masterbus to provide one?
 *
 *  We think the answer is "yes", especially to support the rtl "C" code.
 */

#define RTL66_USE_GLOBAL_CLIENTINFO

/**
 *  A potential future feature, macroed to avoid issues until it is perfected.
 *  Meant to allow detecting changes in the set of MIDI ports, and
 *  disconnecting or connecting as appropriate, if not in manual/virtual mode.
 *
 *  It is now definable (currently for test purposes) in the configuration
 *  process and in the rtmidi qmake configuration, so that it may be centrally
 *  located, because it may have implication throughout Seq66.
 *
 * #undef  RTL66_JACK_PORT_REFRESH
 */

namespace midi
{

/**
 *  The class for holding basic information on the MIDI input and output ports
 *  currently present in the system.
 *
 *  Note that we now provide initializers for the non-class members (e.g.
 *  bool, in) so that the default constructor provides non-random values.
 */

class clientinfo
{
    /**
     *  Provides the rtl (not RtMidi) API version string.
     */

    std::string m_api_version;

    /**
     *  Provides the name of the client for display in a JACK connection graph
     *  (for example).  Examples are "seq66" or "seq66v2".
     */

    std::string m_client_name;

    /**
     *  Holds this value for passing along, to reduce the number of arguments
     *  needed.  This value is the main application name as determined at
     *  ./configure time(e.g. qseq66v2).
     */

    std::string m_app_name;

    /**
     *  Provides the preference of using JACK (versus other MIDI engines).
     *  If JACK is not found, we want to fall back to ALSA.
     */

    bool m_jack_midi {false};

    /**
     *  Indicates that the application will use virtual (manually-connected)
     *  ports.
     */

    bool m_virtual_ports {false};

    /**
     *  Indicates that the application will try to auto-connect to MIDI ports
     *  already existing in the system.  Cannot be used by virtual ports.
     */

    bool m_auto_connect {false};

    /**
     *  Always false until this feature is complete.
     */

    bool m_port_refresh {false};

    /**
     *  Holds the global PPQN value.  This is an addition to the RtMidi
     *  interface. Some MIDI engines can use this information.
     */

    midi::ppqn m_global_ppqn {0};

    /**
     *  Holds the global BPM (beats per minute) value, which is a double to
     *  allow for better precision.  This is an addition to the RtMidi
     *  interface. Some MIDI engines can use this information.  Do not confuse
     *  this value, supported internally by some APIS (JACK, ALSA), with beats
     *  per measure.
     */

    midi::bpm m_global_bpm {0};

private:

#if defined RTL66_JACK_PORT_REFRESH

    /**
     *  Stores that last port configuration, used when port-registration or
     *  port-unregistration is detected.
     */

    ports m_previous_ports[2];

#endif  //  RTL66_JACK_PORT_REFRESH

    /**
     *  Holds data on the ALSA/JACK/Core/WinMM inputs, outputs, or both,
     *  depending on m_port_type. Element 0 is input, element 1 is output.
     *  Use midi::io_to_int().
     */

    ports m_io_ports[2];

    /**
     *  The input port number.  If equal to -1, then (in the future)
     *  will work with all ports. Currently 0 by default.
     */

    int m_input_portnumber {-1};

    /**
     *  The output port number.  If equal to -1, then (in the future)
     *  will work with all ports. Currently 0 by default.
     */

    int m_output_portnumber {-1};

    /**
     *  The ID of the ALSA MIDI queue.
     */

    int m_global_queue {-1};

    /**
     *  Provides a handle to the main ALSA or JACK implementation object.
     *  Created by the class derived from midi::clientinfo.
     */

    void * m_midi_handle {nullptr};

    /**
     *  A kludge to indicate if this object is meant for output or input
     *  ports. Compare it to the boolean parameter of the "impl_xxx() member
     *  functions (e.g. in midi_jack and midi_alsa).
     */

    port::io m_port_type {port::io::duplex};

    /**
     *  True if the handle has been obtained.
     */

    bool m_is_connected {false};

protected:

    /**
     *  Error string for the midi::clientinfo interface.
     */

    std::string m_error_string;

public:

    /*
     * Beware! In GNU C++, the default constructor leaves many members
     * with "random" values.
     */

    clientinfo () = default;
    clientinfo (midi::port::io iodirection);
    clientinfo (const clientinfo &) = default;
    clientinfo (clientinfo &&) = default;
    clientinfo & operator = (const clientinfo &) = default;
    clientinfo & operator = (clientinfo &&) = default;

    virtual ~clientinfo ()
    {
        // Empty body
    }

    /*
     *  Values generally applying throughout the application.
     *  Rather than making them static, we will provide a "global"
     *  clientinfo.
     */

    /**
     *  Sets version strings.  Meant to be called where the engine or API is
     *  used.
     */

    void api_version (const std::string & v)
    {
        m_api_version = v;
    }

    const std::string & api_version () const
    {
        return m_api_version;
    }

    void client_name (const std::string & cname)
    {
        m_client_name = cname;
    }

    const std::string & client_name () const
    {
        return m_client_name;
    }

    void app_name (const std::string & aname)
    {
        m_app_name = aname;
    }

    const std::string & app_name () const
    {
        return m_app_name;
    }

    static bool all_ports (int portnumber)
    {
        return portnumber == (-1);
    }

    int input_portnumber () const
    {
        return m_input_portnumber;
    }

    void input_portnumber (int p)
    {
        if (p >= (-1))
            m_input_portnumber = p;
    }

    int output_portnumber () const
    {
        return m_output_portnumber;
    }

    void output_portnumber (int p)
    {
        if (p >= (-1))
            m_output_portnumber = p;
    }

    bool jack_midi () const
    {
#if defined RTL66_BUILD_JACK
        return m_jack_midi;
#else
        return false;
#endif
    }

    void jack_midi (bool flag)
    {
        m_jack_midi = flag;
    }

    bool virtual_ports () const
    {
        return m_virtual_ports;
    }

    void virtual_ports (bool flag)
    {
        m_virtual_ports = flag;
    }

    bool auto_connect () const
    {
        return m_auto_connect;
    }

    void auto_connect (bool flag)
    {
        m_auto_connect = flag;
    }

    bool port_refresh () const
    {
        return m_port_refresh;
    }

    void port_refresh (bool flag)
    {
        m_port_refresh = flag;
    }

    midi::ppqn global_ppqn () const
    {
        return m_global_ppqn;
    }

    void global_ppqn (midi::ppqn p)
    {
        m_global_ppqn = p;     /* no validation yet    */
    }

    midi::bpm global_bpm () const
    {
        return m_global_bpm;
    }

    void global_bpm (midi::bpm  b)
    {
        m_global_bpm = b;      /* no validation yet    */
    }

    /*
     *  Also see man fesetround(3).
     */

    unsigned global_tempo_us () const
    {
        return unsigned(std::nearbyint(m_global_bpm));
    }

    /*
     *  Value more likely to vary.
     */

    void * midi_handle ()
    {
        return m_midi_handle;
    }

    port::io port_type () const
    {
        return m_port_type;
    }

    bool is_output () const
    {
        return m_port_type == port::io::output;
    }

    // DEPRECATED
    bool is_input () const
    {
        return m_port_type == port::io::input;
    }

    // DEPRECATED
    bool is_engine () const
    {
        return m_port_type == port::io::engine;
    }

    bool is_duplex () const
    {
        return m_port_type == port::io::duplex;
    }

    bool is_connected () const
    {
        return m_is_connected;
    }

    ports & io_ports (port::io iotype)
    {
        return m_io_ports[element(iotype)];
    }

    const ports & io_ports (port::io iotype) const
    {
        return m_io_ports[element(iotype)];
    }

#if defined RTL66_JACK_PORT_REFRESH
    ports & previous_ports ()
    {
        return m_previous_ports;
    }
#endif

    void clear ()
    {
        m_io_ports[0].clear();
        m_io_ports[1].clear();
    }

    bool empty () const
    {
        return m_io_ports[0].empty() && m_io_ports[1].empty();
    }

    int port_count (port::io iotype) const;

#if defined USE_THIS_CODE

    /*
     * We will let the derived API-specific class do the populating of
     * port information.
     */

    virtual int get_all_port_info (ports & /*ioports*/)
    {
        return 0;
    }

    int get_all_io_port_info (port::io iotype)
    {
        return get_all_port_info(io_ports(iotype));
    }

#endif

    int get_port_count (port::io iotype) const
    {
        return io_ports(iotype).get_port_count();
    }

    int get_bus_id (port::io iotype, int index) const
    {
        return io_ports(iotype).get_bus_id(index);
    }

    std::string get_bus_name (port::io iotype, int index) const
    {
        return io_ports(iotype).get_bus_name(index);
    }

    int get_port_id (port::io iotype, int index) const
    {
        return io_ports(iotype).get_port_id(index);
    }

    std::string get_port_name (port::io iotype, int index) const
    {
        return io_ports(iotype).get_port_name(index);
    }

    std::string get_port_alias (port::io iotype, int index) const
    {
        return io_ports(iotype).get_port_alias(index);
    }

    port::kind get_port_type (port::io iotype, int index) const
    {
        return io_ports(iotype).get_port_type(index);
    }

    bool get_input (port::io iotype, int index) const
    {
        return io_ports(iotype).get_input(index);
    }

    bool get_virtual (port::io iotype, int index) const
    {
        return io_ports(iotype).get_virtual(index);
    }

    bool get_system (port::io iotype, int index) const
    {
        return io_ports(iotype).get_system(index);
    }

    int queue_number (port::io iotype, int index) const
    {
        return io_ports(iotype).get_queue_number(index);
    }

    std::string connect_name (port::io iotype, int index) const
    {
        return io_ports(iotype).get_connect_name(index);
    }

    std::string to_string (const std::string & tagmsg) const;
    std::string port_list (port::io iotype) const;
    std::string port_list () const;

    int global_queue () const
    {
        return m_global_queue;
    }

protected:

    int element (port::io iotype) const
    {
        int result = io_to_int(iotype);
        if (result > 1)
            result = 0;                  /* for safety reasons   */

        return result;
    }

    void global_queue (int q)
    {
        m_global_queue = q;
    }

    void midi_handle (void * h)
    {
        m_midi_handle = h;
    }

};          // clientinfo

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

#if defined RTL66_USE_GLOBAL_CLIENTINFO

extern clientinfo & global_client_info ();
extern bool get_global_port_info
(
    rtl::rtmidi::api rapi = rtl::rtmidi::api::unspecified
);

#endif

extern bool get_all_port_info
(
    midi::clientinfo & cinfo,
    rtl::rtmidi::api rapi = rtl::rtmidi::api::unspecified
);

}           // namespace midi

#endif      // RTL66_MIDI_CLIENTINFO_HPP

/*
 * clientinfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

