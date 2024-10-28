#if ! defined RTL66_RTERROR_HPP
#define RTL66_RTERROR_HPP

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
 * \file          rterror.hpp
 *
 *  An abstract base class for MIDI error handling.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2023-03-07
 * \license       See above.
 *
 * #include <exception>                    // std::exception base class
 */

#include <cstdio>                       /* stderr                           */
#include <stdexcept>                    /* std::runtime_error class         */
#include <string>                       /* std::string                      */

namespace rtl
{

/**
 *  Exception handling class for our version of "rtl66".  The rterror class is
 *  quite simple but it does allow errors to be "caught" by rterror::kind.
 *
 *  Please note that, in this refactoring of rtl66, we've done away with all
 *  the exception specifications, on the advice of Herb Sutter.  They may be
 *  more relevent to C++11 and beyond, but this library is too small to worry
 *  about them, for now.
 *
 *  Note that this class is the same for both MIDI and audio processing.
 *  (However, the audio version uses std::runtime_error vs. std::exception, so
 *  that is what we will use here.)
 */

class RTL66_DLL_PUBLIC rterror : public std::runtime_error  /*std::exception*/
{

public:

    enum class kind
    {
        warning,           /**< A non-critical error.                       */
        debug_warning,     /**< Non-critical error useful for debugging.    */
        unspecified,       /**< The default, unspecified error type.        */
        no_devices_found,  /**< No devices found on system.                 */
        invalid_device,    /**< An invalid device ID was specified.         */
        memory_error,      /**< An error occured during memory allocation.  */
        invalid_parameter, /**< Invalid parameter specified to a function.  */
        invalid_use,       /**< The function was called incorrectly.        */
        driver_error,      /**< A system driver error occured.              */
        system_error,      /**< A system error occured.                     */
        thread_error,      /**< A thread error occured.                     */
        max                /**< An "illegal" value for range-checking       */
    };

    /**
     *  rtl66 error callback function prototype.
     *
     *  Note that class behaviour is undefined after a critical error (not
     *  a warning) is reported.
     *
     * \param errtype
     *      Type of error.
     *
     * \param errormsg
     *      Error description.
     *
     * \param userdata
     *      Data provided by the caller.  For audio errors, not yet supported.
     */

    using callback_t = void (*)
    (
        rterror::kind errtype,
        const std::string & errormsg,
        void * userdata
    );

private:

    /**
     *  Holds the type or severity of the exception.
     */

    kind m_type;

public:

    rterror (const std::string & message, kind errtype = kind::unspecified) :
        std::runtime_error  (message),
        m_type              (errtype)
    {
        // no code
    }

    virtual ~rterror () noexcept
    {
        // no code
    }

    /**
     *  Prints thrown error message to stderr.
     */

    virtual void print_message () const noexcept
    {
       std::fprintf(stderr, "\n%s\n\n", what());
    }

    /**
     *  Returns the thrown error message type.
     */

    virtual const kind & get_type () const noexcept
    {
        return m_type;
    }

    /**
     *  Returns the thrown error message string.
     */

    virtual std::string get_message () const noexcept
    {
        return std::string(what());
    }

};

/*
 * Inline functions.
 */

inline rterror::kind
int_to_error_kind (int index)
{
    bool valid = index >= 0 && index < static_cast<int>(rterror::kind::max);
    return valid ? static_cast<rterror::kind>(index) : rterror::kind::max ;
}

inline int
error_kind_to_int (rterror::kind ek)
{
    return static_cast<int>(ek);
}

}           // namespace rtl

#endif      // RTL66_RTEXERROR_HPP

/*
 * rterror.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

