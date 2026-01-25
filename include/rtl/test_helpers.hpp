#if ! defined RTL66_TEST_HELPERS_HPP
#define RTL66_TEST_HELPERS_HPP

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
 * \file          test_helpers.hpp
 *
 *      Simple helper functions re-used in the test applications.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-06-30
 * \updates       2026-01-24
 * \license       See above.
 *
 *  Note that these functions are not in a namespace so that most of them
 *  can be used in C code.
 */

#if defined __cplusplus

#include <string>                       /* std::string class                */

#include <midi/port.hpp>                /* midi::port::io enumeration       */

namespace rtl
{
    class rtmidi_in;
    class rtmidi_out;
}

extern int rt_choose_port
(
    midi::port::io iotype,
    int & portcount,
    bool showalloption = true
);
extern bool rt_choose_input_port (rtl::rtmidi_in & rtin);
extern bool rt_choose_output_port (rtl::rtmidi_out & rtout);
extern int rt_choose_input_ports (int & portcount);
extern int rt_choose_output_ports (int & portcount);
extern bool rt_select_input_ports (int & portcount);
extern int rt_choose_port_number (midi::port::io iotype);
extern bool rt_simple_cli
(
    const std::string & appname,
    int argc, char * argv []
);
extern bool rt_get_extra_option
(
    int argc, char * argv [],
    const std::string & longopt, char shortopt,
    std::string * parameter = nullptr
);
extern bool rt_test_port_valid (int portno);
extern void set_rt_allow_open_all_ports (bool flag);
extern bool rt_allow_open_all_ports ();
extern bool rt_open_all_ports ();
extern void set_rt_open_all_ports ();
extern bool rt_virtual_test_port ();
extern bool rt_show_help ();
extern void rt_print_help (const std::string & appnae);
extern bool rt_use_callback ();
extern int rt_test_port ();
extern int rt_test_port_in ();
extern int rt_test_port_out ();
extern int rt_test_data_length ();
extern const std::string  & rt_test_name ();
extern void rt_test_sleep (int ms);
extern void set_rt_test_port (int portno);
extern void set_rt_test_port_in (int portno);
extern void set_rt_test_port_out (int portno);
extern void set_rt_test_port_name (const std::string & portname);
extern void set_rt_test_name (const std::string & portname);
extern void set_test_data_length (int len);

#endif      // defined __cplusplus

#endif      // ! defined RTL66_TEST_HELPERS_HPP

/*
 * test_helpers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

