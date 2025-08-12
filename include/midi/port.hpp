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
 * \updates       2025-08-12
 * \license       See above.
 *
 *  Contains information about a single MIDI port, as determined by
 *  enumerating existing system ports.
 */

#include <cstdint>                      /* uint32_t and other types         */
#include <string>                       /* std::string class                */

#include "midi/clocking.hpp"            /* output and input port statuses   */

namespace midi
{

/*------------------------------------------------------------------------
 * Inline functions for port
 *------------------------------------------------------------------------*/

/**
 *  In the latest versions of JACK, 0xFFFE is the macro "NO_PORT".  Although
 *  krufty, we can use this value in Seq66 no matter the version of JACK, or
 *  even what API is used.
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

const int input_port_index { 0 };
const int output_port_index { 1 };

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
     *  way. Tested by the midi_api::is_input/output/duplex/engine()
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
        duplex,         /**< Input/output port, or covering the engine.     */
        engine,         /**< The port can be used by midi::masterbus.       */
        dummy           /**< Use by the midi_dummy class                    */
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
     *  Compare this set to the seq66::portslist::io structure. The only
     *  concept missing here is the "nick-name".
     */

    int m_buss_number { -1 };          /**< *Major buss number of the port. */
    std::string m_buss_name { };       /**< *System's name for the buss.    */
    int m_port_number { -1 };          /**< *Minor port number of the port. */
    std::string m_port_name { };       /**< *System's name for the port.    */
    int m_queue_number { -1 };         /**< xA number used in some APIs.    */
    io m_io_type { io::dummy };        /**< *Indicates input versus output. */
    kind m_port_type                   /**< *Flags normal/virt/system port. */
    {
        kind::undetermined
    };
    std::string m_port_alias { };      /**< *Non-empty in some JACK setups. */
    uint32_t m_internal_id             /**< xInternal port number.          */
    {
        null_system_port_id()
    };
    clocking m_io_status               /**< *On, off (disabled), clocking...*/
    {
        clocking::none                 /**< Basic flag for "port enabled".  */
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
        int queuenumber                 = (-1),
        const std::string & aliasname   = ""
    );
    port (const port &) = default;
    port (port &&) = default;
    port & operator = (const port &) = default;
    port & operator = (port &&) = default;
    ~port () = default;

    std::string to_string () const;

public:                                 /* getters                          */

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

    const std::string & port_name () const
    {
        return m_port_name;
    }

    const std::string & port_alias () const
    {
        return m_port_alias;
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

    clocking port_status () const
    {
        return m_io_status;
    }

    bool port_disabled () const
    {
        return midi::port_is_disabled(m_io_status);
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

    void port_name (const std::string & pn)
    {
        m_port_name = pn;
    }

    void port_alias (const std::string & pa)
    {
        m_port_alias = pa;
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

    void port_status (clocking clk)
    {
        m_io_status = clk;
    }

    void port_disabled (bool flag)
    {
        m_io_status = flag ? midi::clocking::none : midi::clocking::disabled ;
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

