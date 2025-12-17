#if ! defined RTL66_MIDI_BUS_IN_HPP
#define RTL66_MIDI_BUS_IN_HPP

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
 * \file          bus_in.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-23
 * \updates       2025-12-11
 * \license       GNU GPLv2 or above
 *
 *  The bus module is the new base class for the various implementations
 *  of the bus module.  There is enough commonality to be worth creating a
 *  base class for all such classes.
 */

#include "midi/bus.hpp"                 /* midi::bus base/mixin class       */
#include "rtl/midi/rtmidi_in.hpp"       /* rtl::rtmidi_in class             */

namespace midi
{

/*------------------------------------------------------------------------
 * midi::bus_in class
 *------------------------------------------------------------------------*/

/**
 *  This class is base/mixin class for providing Seq66 concepts for
 *  an input buss.
 */

class bus_in final : public midi::bus
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class midi::masterbus;

private:

    /**
     *  Rather than inherit from rtl::rtmidi_in, which introduces the issue
     *  of having to add a bunch of boilerplate virtual function overrides,
     *  we include it here as a member.
     */

    rtl::rtmidi_in m_rtmidi_in;

public:

    bus_in ();
    bus_in
    (
        midi::masterbus & master,
        int index,
        unsigned queuesizelimit = 0
    );
    bus_in (const bus_in &) = delete;
    bus_in (bus_in &&) = delete;
    bus_in & operator = (const bus_in &) = delete;
    bus_in & operator = (bus_in &&) = delete;
    virtual ~bus_in ();

    virtual int get_in_port_info () override;
    virtual bool init_input (bool inputing) override;
    virtual int poll_for_midi () const override;
    virtual bool get_midi_event (midi::event * inev) override;
    virtual midi::message get_message () override;

    virtual void ignore_midi_types (bool sysex, bool time, bool sense)
    {
        midi_in().ignore_midi_types(sysex, time, sense);
    }

#if 0
    virtual rtl::midi_api * rt_api_ptr () override
    {
        return midi_in().rt_api_ptr();
    }

    virtual const rtl::midi_api * rt_api_ptr () const override
    {
        return midi_in().rt_api_ptr();
    }
#endif

#if defined USE_THIS_CODE

    virtual void set_input_callback
    (
        rtl::rtmidi_in_data::callback_t callback,
        void * userdata = nullptr
    )
    {
        midi_in().set_input_callback(callback, userdata);
    }

    virtual void cancel_input_callback ()
    {
        midi_in().cancel_input_callback();
    }

#endif

private:

    rtl::rtmidi_in & midi_in ()
    {
        return m_rtmidi_in;
    }

    const rtl::rtmidi_in & midi_in () const
    {
        return m_rtmidi_in;
    }

};          // class bus_in

}           // namespace midi

#endif      // RTL66_MIDI_BUS_IN_HPP

/*
 * bus_in.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

