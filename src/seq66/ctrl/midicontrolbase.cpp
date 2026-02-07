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
 * \file          midicontrolbase.hpp
 *
 *  This module declares/defines the base class for handling MIDI control
 *  <i>I/O</i> of the application.
 *
 * \library       rtl66 library
 * \author        C. Ahlstrom
 * \date          2019-11-25
 * \updates       2026-02-04
 * \license       GNU GPLv2 or above
 *
 * The class contained in this file encapsulates most of the functionality to
 * send feedback to an external control surface in order to reflect the state
 * of seq66. This includes updates on the playing and queueing status of the
 * sequences.
 */

#include "ctrl/midicontrolbase.hpp"     /* seq66::midicontrolbase class     */

namespace seq66
{

midicontrolbase::midicontrolbase (const std::string & name) :
    m_name              (name),
    m_buss              (midi::null_buss()),       /* 0xFF */
    m_true_buss         (midi::null_buss()),
    m_configured_buss   (midi::null_buss()),
    m_is_blank          (true),
    m_is_enabled        (false),
    m_configure_enabled (false),
    m_offset            (0),
    m_rows              (0),
    m_columns           (0)
{
    // No code needed
}

bool
midicontrolbase::initialize (int buss, int rows, int columns)
{
    midi::bussbyte b { midi::bussbyte(buss) };
    m_buss = m_true_buss = b;
    m_rows = rows;
    m_columns = columns;
    return midi::is_valid_buss(b) && rows > 0 && columns > 0;
}

}           // namespace seq66

/*
 * midicontrolbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
