#if ! defined RTL66_RTL_RTMIDI_IO_INFO_HPP
#define RTL66_RTL_RTMIDI_IO_INFO_HPP

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
 * \file          rtmidi_io_info.hpp
 *
 *    Functions to help unify rtmidi code and midi/masterbus code.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2025-11-28
 * \updates       2025-11-29
 * \license       See above.
 *
 */

#include "midi/ports.hpp"               /* midi::ports and midi::port       */
#include "rtl/midi/rtmidi.hpp"          /* rtl::rtmidi base class           */

namespace rtl
{

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

extern bool rtmidi_get_io_info
(
    midi::port::io & iotype,
    midi::ports & ioports,
    rtl::rtmidi::api rapi
);

}           // namespace rtl

#endif      // RTL66_RTL_RTMIDI_IO_INFO_HPP

/*
 * rtmidi_io_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

