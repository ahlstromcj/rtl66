#if ! defined RTL66_API_BASE_HPP
#define RTL66_API_BASE_HPP

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
 * \file          api_base.hpp
 *
 *      The base class for classes that due that actual MIDI work for the MIDI
 *      framework selected at run-time.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2023-03-08
 * \updates       2024-01-15
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

#include "rtl/rt_types.hpp"             /* rtl::midi/audio API types        */
#include "rtl/rterror.hpp"              /* rtl::rterror and others          */

namespace rtl
{

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

extern void error_print (const std::string & tag, const std::string & msg);
extern void debug_print (const std::string & tag, const std::string & msg);

/*------------------------------------------------------------------------
 * api_base
 *------------------------------------------------------------------------*/

/**
 *  The api_base provides facilities common to the MIDI and Audio APIs.
 */

class RTL66_DLL_PUBLIC api_base
{

private:

    /**
     *  Error handling.
     */

    std::string m_error_string;
    rterror::callback_t m_error_callback;
    bool m_first_error;
    void * m_error_callback_user_data;

public:

    /*
     * The default constructor creates an output port.
     */

    api_base ();
    api_base (const api_base &) = default;              /* delete; */
    api_base & operator = (const api_base &) = default; /* delete; */
    virtual ~api_base () = default;

public:

    /**
     * A basic error reporting function for rtmidi classes.
     */

    void set_error_callback (rterror::callback_t cb, void * userdata);
    void error (rterror::kind type, const std::string & errorstring);
    void error (const std::string & tag, int portnumber);
    void warning_no_devices(const std::string & tag, bool isoutput);
    void warning_unimplemented(const std::string & tag);

    void error (rterror::kind type)
    {
        error(type, error_string());
    }

    const std::string & error_string () const
    {
        return m_error_string;
    }

    void error_string (const std::string & errmsg)
    {
        m_error_string = errmsg;
    }

};          // class api_base

}           // namespace rtl

#endif      // RTL66_API_BASE_HPP

/*
 * api_base.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

