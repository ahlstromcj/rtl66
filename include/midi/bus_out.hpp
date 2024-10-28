#if ! defined RTL66_MIDI_BUS_OUT_HPP
#define RTL66_MIDI_BUS_OUT_HPP

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
 * \file          bus_out.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-23
 * \updates       2024-06-09
 * \license       GNU GPLv2 or above
 *
 *  The bus module is the new base class for the various implementations
 *  of the bus module.  There is enough commonality to be worth creating a
 *  base class for all such classes.
 */

#include "midi/bus.hpp"                 /* midi::bus base/mixin class       */
#include "rtl/midi/rtmidi_out.hpp"      /* rtl::rtmidi_out class            */

namespace midi
{

/*------------------------------------------------------------------------
 * midi::bus_out class
 *------------------------------------------------------------------------*/

/**
 *  This class is base/mixin class for providing Seq66 concepts for
 *  an output buss.
 */

class bus_out final : public midi::bus
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class masterbus;

private:

    /**
     *  Rather than inherit from rtl::rtmidi_out, which introduces the issue
     *  of having to add a bunch of boilerplate virtual function overrides,
     *  we include it here as a member.
     */

    rtl::rtmidi_out m_rtmidi_out;

    /**
     *  The last (most recent? final?) tick processed on this port.
     *  This value is used in emitting MIDI Clock.
     */

    pulse m_last_tick;

public:

    bus_out () = delete;
    bus_out (midi::masterbus & master, int index);
    bus_out (const bus_out &) = delete;
    bus_out (bus_out &&) = delete;
    bus_out & operator = (const bus_out &) = delete;
    bus_out & operator = (bus_out &&) = delete;
    virtual ~bus_out ();

    virtual int get_out_port_info () override;
    virtual bool init_clock (pulse tick) override;
    virtual bool send_event (const event * e24, midi::byte channel) override;
    virtual bool send_sysex (const event * e24) override;
    virtual bool clock_start () override;
    virtual bool clock_stop () override;
    virtual bool clock_send (pulse tick) override;
    virtual bool clock_continue (pulse tick) override;

};          // class bus_out

}           // namespace midi

#endif      // RTL66_MIDI_BUS_OUT_HPP

/*
 * bus_out.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

