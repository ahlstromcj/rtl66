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
 *  An abstract base class for seq66::inputslist and seq66::clockslist.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2020-12-11
 * \updates       2026-01-31
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs and outputs (clocks).  We've combined them
 *  for "convenience". :-) Oh, and for port-mapping.
 *
 *  The seq66::portslist class is a more featureful version of midi::ports.
 */

#include <string>                       /* std::string                      */
#include <map>                          /* std::map<buss, I/O struct>       */

#include "midi/midibytes.hpp"           /* midi::bussbyte and other types   */
#include "midi/portnaming.hpp"          /* midi:::portnaming enumeration    */
#include "midi/ports.hpp"               /* midi:::ports, clock::clocking    */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks and inputs values, as used in
 *  mastermidibus and the performer object.
 */

class portslist : public midi::ports
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
     * io = struct   { replaced by midi::port }
     *
     *      midi::port:             seq66::portslist::io
     *
     *      m_buss_number           io_client_number
     *      m_buss_name             <provided by the API's client name>
     *      m_port_number           io_port_number
     *      m_port_name             io_name, io_nick_name, io_alias
     *      m_queue_number          <not stored>
     *      m_io_type               <Indicated by which portlist is active>
     *      m_port_type             <To be stored in the configuration>
     *      m_port_alias            <To be stored in the configuration>
     *      m_port_index            <To be stored in the configuration>
     *      m_internal_id           <not stored>
     *      m_io_status             out_clock, io_enabled, io_available
     *
     *  Basically, the port class defines what is encountered in the
     *  system, while the io structure holds information to be stored
     *  in a configuration file.
     */

protected:

    /**
     *  The container type for io information.  Replaces std::vector<io>.
     *
     *      using container = std::map<midi::bussbyte, io>;
     *      container m_master_io;
     *
     *  Saves the input or clock settings obtained from the "rc" (options)
     *  file so that they can be loaded into the mastermidibus once it is
     *  created.
     */

    /**
     *  Indicates if the list is to be used.  It will always be saved and read,
     *  but not used if this flag is false.
     *
     *  For usage in portmapping (a future feature) this could be
     *  false to indicate that the mapping will not be used.
     */

    bool m_is_active { false };

    /**
     *  Indicates if this list is a port-mapper list.  Useful in debugging.
     */

    bool m_is_port_map { false };

public:

    portslist () = default;
    portslist (bool pmflag);
    portslist (const portslist &) = default;
    portslist (portslist &&) = default;
    portslist & operator = (const portslist &) = default;
    portslist & operator = (portslist &&) = default;
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
    static bool valid (const midi::port & item);

    void match_system_to_map (portslist & destination) const;
    void match_map_to_system (const portslist & source);
    void activate (status s);
    int available_count () const;

    bool active () const
    {
        return m_is_active && port_container().empty();
    }

    bool is_port_map () const
    {
        return m_is_port_map;
    }

    void active (bool flag)
    {
        m_is_active = flag;
    }

    bool set_name (int index, const std::string & name);
#if defined USE_SET_NICK_NAME
    bool set_nick_name (int index, const std::string & name);
#endif
    bool set_alias (int index, const std::string & name);
    std::string get_name (int index) const;
    std::string get_pair_name (int index) const;
    std::string get_nick_name
    (
        int index, midi::portnaming style = midi::portnaming::brief
    ) const;
    std::string get_alias
    (
        int index, midi::portnaming style = midi::portnaming::brief
    ) const;
    std::string get_display_name (int index, midi::portnaming style) const;
    midi::bussbyte bus_from_name (const std::string & nick) const;
    midi::bussbyte bus_from_nick_name (const std::string & nick) const;
    midi::bussbyte bus_from_alias (const std::string & alias) const;
    std::string port_name_from_bus (int index) const;
    void show (const std::string & tag = "") const;
    bool set_enabled (int index, bool enabled);
    bool is_available (int index) const;
    bool is_enabled (int index) const;

    bool is_disabled (int index) const
    {
        return ! is_enabled(index);
    }

protected:

    std::string to_string (const std::string & tag = "") const;
    std::string port_map_list (bool isclock) const;
    std::string io_line
    (
        int portnumber,
        int mstatus,
        const std::string & portname,
        const std::string & portalias = ""
    ) const;
    const midi::port & const_io_block (const std::string & nickname) const;

    midi::port & io_block (const std::string & nickname)
    {
        return const_cast<midi::port &>(const_io_block(nickname));
    }

};              // class portslist

}               // namespace seq66

#endif          // RTL66_PORTLIST_HPP

/*
 * portslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

