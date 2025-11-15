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
 * \file          midi_api.cpp
 *
 *    Provides the base class for the MIDI I/O implementations.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-07
 * \updates       2025-11-10
 * \license       See above.
 *
 */

#include <iostream>                     /* std::cout, std::cerr             */
#include <sstream>                      /* std::ostringstream class         */

#include "c_macros.h"                   /* not_nullptr() and friends        */
#include "midi/masterbus.hpp"           /* midi::masterbus                  */
#include "rtl/midi/midi_api.hpp"        /* rtl::midi_api class              */

namespace rtl
{

/*------------------------------------------------------------------------
 * midi_api basic functions
 *------------------------------------------------------------------------*/

/**
 *  These constructors use "in-class" member initialization.
 */

midi_api::midi_api
(
    midi::masterbus & mbus,
    midi::port::io iotype
) :
    api_base        (),
    m_port_io_type  (iotype),
    m_input_data    (mbus.queue_size()),        /* input data structure     */
    m_master_bus    (&mbus),
    m_has_master    (true),
    m_queue_size    (mbus.queue_size())
{
}

midi_api::midi_api (midi::port::io iotype, unsigned qsize) :
    api_base                    (),
    m_port_io_type              (iotype),
    m_input_data                (qsize),        /* input data structure     */
    m_queue_size                (qsize)
{
    /*
     * Now done in the rtmidi_in_data class. If a non-zero queue size
     * is provided, that implies an input port.
     *
     *  if (iotype == midi::port::io::input && qsize > 0)
     *      m_input_data.queue().allocate(qsize);
     */
}

midi_api::~midi_api ()
{
    if (m_port_io_type == midi::port::io::input)
        m_input_data.queue().deallocate();
}

const midi::clientinfo *
midi_api::client_info () const
{
    const midi::clientinfo * result = nullptr;
    if (has_master())
        result = &master_bus()->client_info();

    return result;
}

/*
 * Note: PPQN and BPM redundantly stored in clientinfo and masterbus.
 *
 * These are public virtual functions.
 */

midi::ppqn
midi_api::PPQN () const
{
    return has_master() ? master_bus()->PPQN() : RTL66_DEFAULT_PPQN ;
}

midi::bpm
midi_api::BPM () const
{
    return has_master() ? master_bus()->BPM() : RTL66_DEFAULT_BPM ;
}

/**
 *  Indicates if the global client handle has been set.
 */

bool
midi_api::master_is_connected () const
{
    bool result { false };
    if (has_master())
        result = master_bus()->info_is_connected();

    return result;
}

/*------------------------------------------------------------------------
 * midi_api input functions
 *------------------------------------------------------------------------*/

void
midi_api::set_input_callback (rtmidi_in_data::callback_t cb, void * userdata)
{
    if (m_input_data.using_callback())
    {
        std::string msg { "midi_api::set_input_callback: already set" };
        error(rterror::kind::warning, msg);
        return;
    }
    if (is_nullptr(cb))
    {
        std::string msg { "rtmidi::set_input_callback: null function" };
        error(rterror::kind::warning, msg);
        return;
    }
    m_input_data.user_callback(cb, userdata);
}

void
midi_api::cancel_input_callback ()
{
    if (! m_input_data.using_callback())
    {
        std::string msg { "rtmidi_in::cancel_callback: no function set" };
        error(rterror::kind::warning, msg);
        return;
    }
    m_input_data.clear_callback();
}

void
midi_api::ignore_midi_types (bool midisysex, bool miditime, bool midisense)
{
    m_input_data.ignore_flags(midisysex, miditime, midisense);
}

midi::message
midi_api::get_message () const
{
    midi::message result;
    if (m_input_data.using_callback())
    {
        std::string msg { "midi_in_api::get_message: user callback in use" };
        error(rterror::kind::warning, msg);
    }
    else
        result = m_input_data.queue().pop_front();

    return result;
}

void
midi_api::set_buffer_size (size_t sz, int count)
{
    m_input_data.set_buffer_size(sz, count);
}

/*------------------------------------------------------------------------
 * midi_api output functions need no extra code here.
 *------------------------------------------------------------------------*/

}           // namespace rtl

/*
 * midi_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

