#if ! defined RTL66_PORTLIST_HPP
#define RTL66_PORTLIST_HPP

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
 * \file          portslist.hpp
 *
 *  An abstract base class for inputslist and clockslist.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2020-12-11
 * \updates       2024-06-13
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs and outputs (clocks).  We've combined them
 *  for "convenience". :-) Oh, and for port-mapping.
 */

#include <string>                       /* std::string                      */
#include <map>                          /* std::map<buss, I/O struct>       */

#include "midi/clocking.hpp"            /* enum class midi::clocking, etc.  */
#include "midi/midibytes.hpp"           /* midi::bussbyte and other types   */

namespace seq66
{

/**
 *  Indicates whether to use the short (internal) or the long (normal)
 *  port names in visible user-interface elements.  If there is no
 *  internal map, this option is forced to "long".
 */

enum class portname
{
    brief,  /**< "short": Use short names: "[0] midi_out".              */
    pair,   /**< "pair": Pair names: "36:0 fluidsynth:midi_out".        */
    full,   /**< "long": Long names: "[0] 36:0 fluidsynth:midi_out".    */
    max     /**< Keep this last... a size value.                        */
};

/**
 *  A wrapper for a vector of clocks and inputs values, as used in
 *  mastermidibus and the performer object.
 */

class portslist
{
    friend std::string output_port_map_list ();
    friend std::string input_port_map_list ();

public:

    /**
     *  A boolean is not quite enough for activating, deactivating, and
     *  deactivating and clearing a port list.
     */

    enum class status
    {
        cleared,                        /**< Deactivate and clear the list. */
        off,                            /**< Deactivate the list.           */
        on                              /**< Activate the list.             */
    };

public:

    /**
     *  Provides a port name and the input or output values.  Note that the
     *  clock setting will be off (not disabled) for all input values.  This
     *  is so that we can disable missing inputs when port-mapping.  The clock
     *  setting will be disabled for output values that are actually disabled
     *  by the user or are missing from the actual system ports.  There is
     *  also a static function valid() in portslist to check that the io_name
     *  is not empty.
     */

    using io = struct
    {
        bool io_available;          /**< Portmapped-bus not present.        */
        bool io_enabled;            /**< The status setting for this buss.  */
        midi::clocking out_clock;   /**< Clock/disabled setting for buss.   */
        std::string io_name;        /**< The name of the I/O buss.          */
        std::string io_nick_name;   /**< The short name of the I/O buss.    */
        std::string io_alias;       /**< FYI only, and only for JACK.       */
        int io_client_number;       /**< The system client number.          */
        int io_port_number;         /**< The system port number.            */
    };

protected:

    /**
     *  The container type for io information.  Replaces std::vector<io>.
     */

    using container = std::map<midi::bussbyte, io>;

    /**
     *  Saves the input or clock settings obtained from the "rc" (options)
     *  file so that they can be loaded into the mastermidibus once it is
     *  created.
     */

    container m_master_io;

    /**
     *  Indicates if the list is to be used.  It will always be saved and read,
     *  but not used if this flag is false.
     */

    bool m_is_active;

    /**
     *  Indicates if this list is a port-mapper list.  Useful in debugging.
     */

    bool m_is_port_map;

public:

    portslist (bool pmflag = false);
    virtual ~portslist () = default;

    virtual std::string io_list_lines () const = 0;
    virtual bool add_list_line (const std::string & line) = 0;
    virtual bool add_map_line (const std::string & line) = 0;

    static bool parse_port_line
    (
        const std::string & line,
        int & portnumber,
        int & portstatus,
        std::string & portname
    );
    static bool valid (const io & item);

    void match_system_to_map (portslist & destination) const;
    void match_map_to_system (const portslist & source);

    void clear ()
    {
        m_master_io.clear();
    }

    void activate (status s);
    int available_count () const;

    int count () const
    {
        return int(m_master_io.size());
    }

    bool not_empty () const
    {
        return ! m_master_io.empty();
    }

    bool active () const
    {
        return m_is_active && not_empty();
    }

    bool is_port_map () const
    {
        return m_is_port_map;
    }

    void active (bool flag)
    {
        m_is_active = flag;
    }

    void set_name (midi::bussbyte bus, const std::string & name);
#if defined USE_SET_NICK_NAME
    void set_nick_name (midi::bussbyte bus, const std::string & name);
#endif
    void set_alias (midi::bussbyte bus, const std::string & name);
    std::string get_name (midi::bussbyte bus) const;
    std::string get_pair_name (midi::bussbyte bus) const;
    std::string get_nick_name
    (
        midi::bussbyte bus,
        portname style = portname::brief
    ) const;
    std::string get_alias
    (
        midi::bussbyte bus,
        portname style = portname::brief
    ) const;
    std::string get_display_name (midi::bussbyte bus, portname style) const;
    midi::bussbyte bus_from_name (const std::string & nick) const;
    midi::bussbyte bus_from_nick_name (const std::string & nick) const;
    midi::bussbyte bus_from_alias (const std::string & alias) const;
    std::string port_name_from_bus (midi::bussbyte nominalbuss) const;
    void show (const std::string & tag = "") const;
    bool set_enabled (midi::bussbyte bus, bool enabled);
    bool is_available (midi::bussbyte bus) const;
    bool is_enabled (midi::bussbyte bus) const;

    bool is_disabled (midi::bussbyte bus) const
    {
        return ! is_enabled(bus);
    }

protected:

    container & master_io ()
    {
        return m_master_io;
    }

    const container & master_io () const
    {
        return m_master_io;
    }

    std::string to_string (const std::string & tag = "") const;
    std::string extract_nickname (const std::string & name) const;
    bool extract_port_pair
    (
        const std::string & name,
        int & client,
        int & portno
    ) const;
    std::string midi::clocking_to_string (midi::clocking e) const;
    std::string port_map_list (bool isclock) const;
    std::string io_line
    (
        int portnumber,
        int mstatus,
        const std::string & portname,
        const std::string & portalias = ""
    ) const;
    bool add
    (
        int bussno,
        bool available,
        int mstatus,
        const std::string & name,
        const std::string & nickname = "",
        const std::string & alias = ""
    );
    bool add (int bussno, io & ioitem, const std::string & nickname);
    const io & const_io_block (const std::string & nickname) const;

    io & io_block (const std::string & nickname)
    {
        return const_cast<io &>(const_io_block(nickname));
    }

};              // class portslist

}               // namespace seq66

#endif          // RTL66_PORTLIST_HPP

/*
 * portslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

