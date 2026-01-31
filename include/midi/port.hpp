#if ! defined RTL66_MIDI_PORT_HPP
#define RTL66_MIDI_PORT_HPP

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
 * \file          port.hpp
 *
 *  A data class for holding the current status of the MIDI system on the host.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-05-24        (seq66::midi_port_info)
 * \updates       2026-01-30
 * \license       See above.
 *
 *  Contains information about a single MIDI port, as determined by
 *  enumerating existing system ports.
 */

#include <cstdint>                      /* uint32_t and other types         */
#include <string>                       /* std::string class                */

#include "cpp_types.hpp"                /* lib66::tokenization of strings   */
#include "midi/clocking.hpp"            /* output and input port statuses   */

namespace midi
{

/*------------------------------------------------------------------------
 * Inline functions for port
 *------------------------------------------------------------------------*/

/**
 *  In the latest versions of JACK, 0xFFFE is the macro "NO_PORT".  Although
 *  krufty, we can use this value in Seq66 no matter the version of JACK, or
 *  even what API is used. Another value used is -1.
 */

inline uint32_t
null_system_port_id ()
{
    return 0xFFFE;
}

inline bool
is_null_system_port_id (uint32_t portid)
{
    return portid == null_system_port_id();
}

/**
 *  Constants for a common usage. See port::io below.
 */

const int c_input_port_index { 0 };
const int c_output_port_index { 1 };

/**
 *  A structure for hold basic information about a single (MIDI) port.
 *  Except for the virtual-vs-normal status, this information is obtained by
 *  scanning the system at the startup time of the application.
 */

class port
{
    friend class ports;

public:

    /**
     *  Constants for selecting input versus output ports in a more obvious
     *  way. Tested by the midi_api::is_input/output/duplex/engine/...()
     *  functions.
     *
     *  Currently, the io::engine value is used for supporting the use of a
     *  midi_api-derived object to handle only engine ("client") functionality,
     *  such as enumerating the existing ports on a system.
     */

    enum class io
    {
        input,          /**< The port is an input MIDI port.                */
        output,         /**< The port is an output MIDI port.               */
        duplex,         /**< Input or output port.                          */
        engine,         /**< The port is purely for use by midi::masterbus. */
        dummy,          /**< Use by midi_dummy or for undetermined ports.   */
        finder          /**< Used by the find_midi_api() function,          */
    };

    /**
     *  Constants for selecting virtual versus normal versus built-in system
     *  ports.  Used in the rtmidi midibus constructors.  Tested by the
     *  is_virtual_port() and is_system_port() functions.
     */

    enum class kind
    {
        normal,         /**< Able to be automatically connected.            */
        manual,         /**< A virtual port (virtual is a keyword, though). */
        system,         /**< A system port (ALSA only).                     */
        undetermined    /**< The port data has not yet been determined.     */
    };

private:

    /*
     *  We provide a default constructor rather than set defaults here.
     *  Compare this set to the seq66::portslist::io structure. The new
     *  concept here is the "nick-name".
     *
     *  Also, note that the port number (in ALSA) comes from a query and
     *  indicates the number re a particular client. For example,
     *  "Midi Through:Midi Through Port-0" has a client:port number
     *  pair of "14:0". But for applications, we need an index value
     *  for the port, for lookup purposes. Hence the m_port_index number.
     */

    /**
     *  Major buss number of the port. Applicable to ALSA. System devices
     *  (which includes plugged-in MIDI hardware) range 0 to 127; software
     *  MIDI clients go from 128 on up. Here's a typical setup; the
     *  additional lines are port numbers:
     *
     *      $ aconnect -l               (output cleansed for readability)
     *      client 0: 'System' [type=kernel]
     *          0 'Timer           '    Connecting To: 144:0
     *          1 'Announce        '    Connecting To: 144:0, 128:0
     *      client 14: 'Midi Through' [type=kernel]
     *          0 'Midi Through Port-0'
     *              Connecting To: 128:1
     *              Connected From: 128:2
     *      client 28: 'nanoKEY2' [type=kernel,card=3]
     *          0 'nanoKEY2 _ CTRL '    Connected From: 128:3
     *      client 36: 'Q25' [type=kernel,card=5]
     *          0 'Q25 MIDI 1      '    Connected From: 128:4
     *      client 144: 'PipeWire-System' [type=user,pid=2541941]
     *          0 'input           '    Connected From: 0:1, 0:0
     *      client 145: 'PipeWire-RT-Event' [type=user,pid=2541941]
     *          0 'input
     *
     *  Also known as the "client number" or "client ID".
     *  Note that an application has one client (buss) number
     *  (e.g. 128 for Seq66 or 144 for PipwWire), but can have multiple
     *  port numbers.
     *
     *  Do not confuse this with the port index, defined below.
     */

    int m_buss_number { -1 };

    /**
     *  System's name for the buss. See ALSA examples above.
     */

    std::string m_buss_name { };

    /**
     *  Minor port number of the port/client/buss.
     */

    int m_port_number { -1 };

    /**
     *  System's name for the port. See ALSA examples above.
     */

    std::string m_port_name { };

    /**
     *  A number used in some APIs.
     */

    int m_queue_number { -1 };

    /**
     *  Indicates input versus output versus duplex.
     */

    io m_io_type { io::dummy };

    /**
     *  Flags normal/virtual/system port. Note that "normal" refers to
     *  an actual port associate with a device to which the application
     *  is to connect, and "virtual" refers to an application port
     *  that can be connected manually (e.g. by aconnect).
     */

    kind m_port_type { kind::undetermined };

    /**
     *  Non-empty in some JACK setups. For example, here is a list of
     *  created ports:
     *
     *      jack_lsp --alias                midiout --jack (test app)
     *
     *      Midi-Through:midi/playback_1    system:midi_playback_1
     *      Midi-Through:midi/capture_1     system:midi_capture_1
     *      nanoKEY2:midi/playback_1        system:midi_playback_1
     *      nanoKEY2:midi/capture_1         system:midi_capture_1
     *      Q25:midi/playback_1             system:midi_playback_1
     *      Q25:midi/capture_1              system:midi_capture_1
     *
     *  Not shown here are the aliases; here's an example:
     *
     *  system:midi_capture_3
     *     alsa_pcm:Q25/midi_playback_1     The full name of the port.
     *     Q25:midi/playback_1              The full alias of the port.
     *  system:midi_playback_3
     *     alsa_pcm:Q25/midi_capture_1
     *     Q25:midi/capture_1
     *
     *  We want to store full alias, plus a nick name. Also, since JACK
     *  can provide 2 alias, we use a (short) vector of strings from
     *  the small lib66 library.
     *
     *  We could put all port names (normal name, nick-name, and aliases)
     *  into the vector, but that seems too intractable at this time.
     */

    lib66::tokenization m_port_aliases { };

    /**
     *  The nick-name of the port, i.e. all characters up to the colon.
     *  For example, "Q25:midi/capture_1" becomes "Q25". The purpose of
     *  the nick-name is to make lookups easier for things like mapping
     *  ports to integers.
     */

    std::string m_port_nickname { };

    /**
     *  Application port-number/index. These always range from 0 on up.
     *  Input and output ports are numbered separately.
     */

    int m_port_index { -1 };

    /**
     *  Internal port number.
     */

    uint32_t m_internal_id
    {
        null_system_port_id()
    };

    /**
     *  On Off (disabled) Clocking...
     *  A basic flag for "port enabled" (perhaps with MIDI clocking)
     *  and "port disabled.
     */

    clock::clocking m_io_status
    {
        clock::clocking::none
    };

public:

    port () = default;
    port                                // TODO add clocking parameter
    (
        int bussnumber,
        const std::string & bussname,
        int portnumber,
        const std::string & portname,
        io iotype,
        kind porttype,
        int portid,
        int queuenumber                 = (-1),
        const std::string & nick        = "",
        const std::string & alias0      = "",
        const std::string & alias1      = ""
    );
    port (const port &) = default;
    port (port &&) = default;
    port & operator = (const port &) = default;
    port & operator = (port &&) = default;
    ~port () = default;

    std::string to_string () const;

public:                                 /* getters                          */

    bool valid () const
    {
        return m_port_type != kind::undetermined &&
            m_io_type != io::dummy;
    }

    int buss_number () const
    {
        return m_buss_number;
    }

    const std::string & buss_name () const
    {
        return m_buss_name;
    }

    int port_number () const
    {
        return m_port_number;
    }

    int port_index () const
    {
        return m_port_index;
    }

    const std::string & port_name () const
    {
        return m_port_name;
    }

    /**
     *  The alias number is either 0 or 1, though we allow for even more
     *  aliases to be added (developer's choice).
     */

    const std::string & port_alias (int aliasno = 0) const
    {
        static std::string s_dummy;
        return std::size_t(aliasno) < m_port_aliases.size() ?
            m_port_aliases[aliasno] : s_dummy ;
    }

    const lib66::tokenization & port_aliases () const
    {
        return m_port_aliases;
    }

    const std::string & port_nickname () const
    {
        return m_port_nickname;
    }

    int queue_number () const
    {
        return m_queue_number;              /* a number used in some APIs.  */
    }

    io io_type () const
    {
        return m_io_type;
    }

    kind port_type () const
    {
        return m_port_type;
    }

    uint32_t internal_id () const
    {
        return m_internal_id;
    }

    clock::clocking port_status () const
    {
        return m_io_status;
    }

    bool port_available () const
    {
        return midi::clock_is_available(port_status());
    }

    bool clock_enabled () const
    {
        return midi::clock_is_enabled(port_status());
    }

    bool port_enabled () const
    {
        return ! port_disabled();
    }

    bool port_disabled () const
    {
        return midi::port_is_disabled(port_status());
    }

public:                                 /* setters                          */

    void buss_number (int b)
    {
        m_buss_number = b;
    }

    void  buss_name (const std::string & bn)
    {
        m_buss_name = bn;
    }

    void port_number (int p)
    {
        m_port_number = p;
    }

    void port_index (int p)
    {
        m_port_index = p;
    }

    void port_name (const std::string & pn)
    {
        m_port_name = pn;
    }

    /**
     *  Most APIs do not support aliases. JACK supports two. But
     *  the developer can add more.
     */

    void port_alias (const std::string & pa)
    {
        m_port_aliases.push_back(pa);
    }

    void port_aliases (const lib66::tokenization & pat)
    {
        m_port_aliases = pat;
    }

    void port_nickname (const std::string & pn)
    {
        m_port_nickname = pn;
    }

    void queue_number (int q)
    {
        m_queue_number = q;           /* a number used in some APIs.  */
    }

    void io_type (io iot)
    {
        m_io_type = iot;
    }

    void port_type (kind k)
    {
        m_port_type = k;
    }

    void internal_id (uint32_t id)
    {
        m_internal_id = id;
    }

    void port_status (clock::clocking clk)
    {
        m_io_status = clk;
    }

    void port_available (bool flag)
    {
        m_io_status = flag ?
            midi::clock::clocking::none : midi::clock::clocking::unavailable ;
    }

    void port_enabled (bool flag)
    {
        m_io_status = flag ?
            midi::clock::clocking::none : midi::clock::clocking::disabled ;
    }

    void port_disabled (bool flag)
    {
        m_io_status = flag ?
            midi::clock::clocking::disabled : midi::clock::clocking::none ;
    }

};          // class port

/*------------------------------------------------------------------------
 * Free and additional inline functions for port
 *------------------------------------------------------------------------*/

inline int
io_to_int (port::io iotype)
{
    return static_cast<int>(iotype);
}

extern std::string io_to_string (port::io iotype);
extern std::string kind_to_string (port::kind ptype);

}           // namespace midi

#endif      // RTL66_MIDI_PORT_HPP

/*
 * port.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

