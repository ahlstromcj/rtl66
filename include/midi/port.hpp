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
 * \updates       2024-10-28
 * \license       See above.
 *
 *  Contains information about a single MIDI port, as determined by
 *  enumerating existing system ports.
 */

#include <cstdint>                      /* uint32_t and other types         */
#include <string>                       /* std::string class                */
#include <vector>                       /* std::vector class                */

namespace midi
{

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
     */

    int m_buss_number;                  /**< Major buss number of the port. */
    std::string m_buss_name;            /**< System's name for the buss.    */
    int m_port_number;                  /**< Minor port number of the port. */
    std::string m_port_name;            /**< System's name for the port.    */
    int m_queue_number;                 /**< A number used in some APIs.    */
    io m_io_type;                       /**< Indicates input versus output. */
    kind m_port_type;                   /**< Flags normal/virt/system port. */
    std::string m_port_alias;           /**< Non-empty in some JACK setups. */
    uint32_t m_internal_id;             /**< Internal port number.          */

public:

    port ();
    port
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
        return m_port_name;
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

    void internal_id (uint32_t id)
    {
        m_internal_id = id;
    }

};          // class port

/*------------------------------------------------------------------------
 * Free functions for port
 *------------------------------------------------------------------------*/

inline int
io_to_int (port::io iotype)
{
    return static_cast<int>(iotype);
}

}           // namespace midi

#endif      // RTL66_MIDI_PORT_HPP

/*
 * port.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

