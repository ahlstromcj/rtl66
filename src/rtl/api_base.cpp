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
 * \file          api_base.cpp
 *
 *    Provides the base class for the MIDI I/O implementations.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-08
 * \updates       2023-03-08
 * \license       See above.
 *
 */

#include <iostream>                     /* std::cout, std::cerr             */
#include <sstream>                      /* std::ostringstream class         */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "rtl/api_base.hpp"             /* rtl::api_base class              */

namespace rtl
{

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

void
error_print (const std::string & tag, const std::string & msg)
{
    std::cerr << tag << ": " << msg << std::endl;
}

void
debug_print (const std::string & tag, const std::string & msg)
{
#if defined PLATFORM_DEBUG
    std::cout << tag << ": " << msg << std::endl;
#else
    (void) tag;
    (void) msg;
#endif
}

/*------------------------------------------------------------------------
 * api_base basic functions
 *------------------------------------------------------------------------*/

api_base::api_base () :
    m_error_string              (),
    m_error_callback            (nullptr),
    m_first_error               (false),
    m_error_callback_user_data  (nullptr)
{
    /*
     * Currently we use midi::port::io::output for probing for an exsiting API
     * [see the rtl::find_api_base() function.]
     */
}

/**
 *  Set an error callback function to be invoked when an error has occured.
 *
 *  The callback function will be called whenever an error has occured. It is
 *  best to set the error callback function before opening a port.
 */

void
api_base::set_error_callback (rterror::callback_t cb, void * userdata)
{
    m_error_callback = cb;
    m_error_callback_user_data = userdata;
}

void
api_base::error (rterror::kind type, const std::string & errmsg)
{
    error_string(errmsg);                   /* new 2022-07-25 */
    if (not_nullptr(m_error_callback))
    {
        if (! m_first_error)
        {
            m_first_error = true;
            const std::string errormessage = errmsg;
            m_error_callback(type, errormessage, m_error_callback_user_data);
            m_first_error = false;
            m_error_string = errmsg;
        }
    }
    else
    {
        if (type == rterror::kind::warning)
        {
            std::cerr << '\n' << errmsg << "\n";
        }
        else if (type == rterror::kind::debug_warning)
        {
#if defined PLATFORM_DEBUG
            std::cerr << '\n' << errmsg << "\n";
#endif
        }
        else
        {
            std::cerr << '\n' << errmsg << "\n";
            throw rterror(errmsg, type);
        }
    }
}

void
api_base::error (const std::string & tag, int portnumber)
{
    std::ostringstream ost;
    ost << tag << ": portnumber (" << portnumber << ") is invalid.";
    error(rterror::kind::invalid_parameter, ost.str());
}

void
api_base::warning_no_devices(const std::string & tag, bool isoutput)
{
    std::ostringstream ost;
    ost << tag << ": no " <<
    (isoutput ? "output" : "input") << " devices available";
    error(rterror::kind::warning, ost.str());
}

void
api_base::warning_unimplemented(const std::string & tag)
{
    std::string msg = tag + ": unimplemented";
    error(rterror::kind::warning, msg);
}

}           // namespace rtl

/*
 * api_base.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

