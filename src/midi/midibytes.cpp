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
 * \file          midibytes.cpp
 *
 *  This module declares a couple of useful data classes.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2024-05-09
 * \license       GNU GPLv2 or above
 *
 */

#if defined __cplusplus                 /* do not expose this to C code     */

#include <stdexcept>                    /* std::invalid_argument            */

#include "midi/midibytes.hpp"           /* midi::byte and other types   */

namespace midi
{

/*
 *  Free functions.
 */

/**
 *  Converts a vector of bytes to a human-readable string in hex notation.
 *
 * \param b
 *      Provides the vector of bytes to be rendered readable.
 *
 * \param limit
 *      If non-zero, then the number of bytes converted is limited.
 *      The default is zero, which allows conversion of all bytes.
 *
 * \return
 *      Returns the human-readable string of hex numbers.
 */

std::string
hex_bytes_string (const bytes & b, int limit)
{
    std::string result;
    int count = int(b.size());
    bool no_0x = limit > 0;
    int len = count;
    if (no_0x && (limit < count))
        len = limit;

    if (len > 0)
    {
        char tmp[8];
        const char * fmt = no_0x ? "%02X" : "0x%02x" ;
        for (int i = 0; i < len; ++i)
        {
            (void) snprintf(tmp, sizeof tmp, fmt, unsigned(b[i]));
            result += tmp;
            if (i < (len + 1))
                result += " ";
        }
        if (len < count)
            result += " ...";
    }
    return result;
}

/**
 *  Stores data bytes in an std::string.  Saves the caller from doing
 *  a reinterpret_cast<>().
 *
 * \param b
 *      Provides the vector of bytes.
 *
 * \return
 *      Returns the resulting string. Note that any 0 byte terminates
 *      the string. Hopefully the only 0 byte is at then end.
 */

std::string
bytes_to_string (const bytes & b)
{
    std::string result;
    for (auto c : b)
    {
        if (c == 0)
            break;

        result.push_back(static_cast<char>(c));
    }
    return result;
}

/**
 *  Converts a string to a MIDI byte.  Similar to string_to_long() in the
 *  cfg66/util/strfunctions module.
 *
 * \param s
 *      Provides the string to convert to a MIDI byte.
 *
 * \return
 *      Returns the MIDI byte value represented by the string.
 */

byte
string_to_byte (const std::string & s, byte defalt)
{
    byte result = defalt;
    try
    {
        int temp = std::stoi(s, nullptr, 0);
        if (temp >= 0 && temp <= UCHAR_MAX)
            result = byte(temp);
    }
    catch (std::invalid_argument const &)
    {
        // no code
    }
    return result;
}

/**
 *  Fix up the size of a midi::booleans vector.  It copies as many midi::bool
 *  values as is correct from the old vector.  Then, if it needs to be longer,
 *  additional 0 values are pushed into the result.
 */

booleans
fix_booleans (const booleans & mbs, int newsz)
{
    booleans result;
    int sz = int(mbs.size());
    if (newsz >= sz)
    {
        result = mbs;
        if (newsz > sz)
        {
            int diff = newsz - sz;
            for (int i = 0; i < diff; ++i)
                result.push_back(0);
        }
    }
    else
    {
        for (int i = 0; i < newsz; ++i)
            result.push_back(mbs[i]);
    }
    return result;
}

}           // namespace midi

#endif          // defined __cplusplus : do not expose to C code

/*
 * midibytes.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

