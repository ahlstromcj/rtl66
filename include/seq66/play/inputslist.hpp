#if ! defined RTL66_INPUTSLIST_HPP
#define RTL66_INPUTSLIST_HPP

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
 * \file          inputslist.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2024-06-13
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs, pulled out of the old perform module.
 */

#include "play/portslist.hpp"           /* seq66::portslist base class      */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks, as used in mastermidibus and the
 *  performer object.
 */

class inputslist final : public portslist
{
    friend bool build_input_port_map (const inputslist & lb);

public:

    inputslist (bool isportmap = false) : portslist (isportmap)
    {
        // Nothing to do
    }

    virtual ~inputslist () = default;

    virtual std::string io_list_lines () const override;
    virtual bool add_list_line (const std::string & line) override;
    virtual bool add_map_line (const std::string & line) override;

    bool add
    (
        int bussno,
//      bool available,
//      bool enabled,
        midi::midi::clocking inputstatus,
        const std::string & name,
        const std::string & nickname = "",
        const std::string & alias = ""
    );
    bool set (midi::bussbyte busno, bool inputing);
    bool get (midi::bussbyte busno) const;

};              // class inputslist

/*
 * Free functions
 */

extern inputslist & input_port_map ();
extern bool build_input_port_map (const inputslist & lb);
extern void clear_input_port_map ();
extern void activate_input_port_map (bool flag);
extern midi::bussbyte true_input_bus
(
    const inputslist & cl, midi::bussbyte nominalbuss
);

#if defined USE_IOPUT_PORT_NAME_FUNCTION
extern std::string input_port_name (midi::bussbyte b, bool addnumber = false);
#endif

extern midi::bussbyte input_port_number (midi::bussbyte b);
extern std::string input_port_map_list ();

}               // namespace seq66

#endif          // RTL66_INPUTSLIST_HPP

/*
 * inputslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

