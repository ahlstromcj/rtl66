#if ! defined RTL66_MIDI_PORTNAMING_HPP
#define RTL66_MIDI_PORTNAMING_HPP

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
 * \file          portnaming.hpp
 *
 *  This module declares/defines some common portnaming needed by the
 *  application.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2022-12-18
 * \updates       2022-12-19
 * \license       GNU GPLv2 or above
 *
 *  These items were moved from the globals.h module so that only the modules
 *  that need them need to include them.  Also included are some minor
 *  "utility" functions dealing with MIDI and port-related strings.  Many of
 *  the functions are defined in this header file, as inline code.
 */

#include "cpp_types.hpp"                /* std::string, tokenization alias  */
#include "midi/midibytes.hpp"           /* pulse alias and much more        */
#include "midi/measures.hpp"            /* midi::measures class             */
#include "midi/timing.hpp"              /* midi::timing class               */

/*
 * Global functions in the midi namespace for MIDI timing portnaming.
 */

namespace midi
{

/**
 *  Indicates whether to use the short (internal) or the long (normal)
 *  port names in visible user-interface elements.  If there is no
 *  internal map, this option is forced to "long".
 */

enum class portnaming
{
    brief,  /**< "short": Use short names: "[0] midi_out".                  */
    pair,   /**< "pair": Pair names: "36:0 fluidsynth:midi_out".            */
    full,   /**< "long": Long names: "[0] 36:0 fluidsynth:midi_out".        */
    max     /**< Keep this last... a size/illegal value.                    */
};

/*------------------------------------------------------------------------
 * Free functions in the midi namespace.
 *------------------------------------------------------------------------*/

extern bool contains
(
    const std::string & original,
    const std::string & target
);
extern bool extract_port_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
);
extern std::string extract_bus_name (const std::string & fullname);
extern std::string extract_port_name (const std::string & fullname);
extern std::string extract_nickname (const std::string & name);
extern std::string extract_a2j_port_name (const std::string & alias);

}           // namespace midi

#endif      // RTL66_MIDI_PORTNAMING_HPP

/*
 * portnaming.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

