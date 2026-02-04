#if ! defined RTL66_MIDICONTROL_HPP
#define RTL66_MIDICONTROL_HPP

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
 * \file          midicontrol.hpp
 *
 *  This module declares/defines the class for handling MIDI control data for
 *  the application.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2026-02-04
 * \license       GNU GPLv2 or above
 *
 *  This module defines a number of constants relating to control of pattern
 *  unmuting, group control, and a number of additional controls to make Seq66
 *  controllable without a graphical user interface.
 *
 *  It requires C++11 and above.
 *
 * Concept:
 * Status bits:
 *
 *  See the top banner in the automation.cpp module.
 */

#include "ctrl/keycontrol.hpp"          /* seq66::keycontrol class          */
#include "midi/event.hpp"               /* midi::event class, bussbyte      */

namespace midi
{
    class event;
}

namespace seq66
{

/**
 *  This class contains the MIDI control information for sequences that make
 *  up a live set.  It defines a single MIDI control.  It is derived from
 *  keycontrol so that we can store a whole control section stanza, including
 *  the key name, in one configuration stanza.
 *
 *  Note that, although we've converted this to a full-fledged class, the
 *  ordering of variables and the data arrays used to fill them is very
 *  significant.
 */

class midicontrol : public keycontrol
{

public:

    /**
     *  Provides a key for looking up a MIDI control in the midicontainer.
     *  When doing a lookup, the status and first data byte must match. Once
     *  found, if the minimum and maximum byte values are not 0, then the
     *  range is also checked. Also, the buss is now used, so that the
     *  user can guarantee that only one device will control Seq66. It is
     *  not part of the lookup, however.
     */

    class key
    {
        friend class midicontrol;

    private:

        midi::bussbyte m_buss;  /**< Indicates the port of the event.       */
        midi::byte m_status;    /**< Provides the (incoming) event type.    */
        midi::byte m_d0;        /**< Provides the first byte, for searches. */

    public:

        key () = default;
        key (const key &) = default;
        key & operator = (const key &) = default;
        ~key () = default;

        key (byte status, byte d0) :
            m_buss      (null_buss()),
            m_status    (status),
            m_d0        (d0)
        {
            // no code
        }

        key (const event & ev) :
            m_buss      (ev.input_bus()),
            m_status    (ev.get_status()),
            m_d0        (0)
        {
            ev.get_data(m_d0);
        }

        bool operator < (const key & rhs) const
        {
            return (m_status == rhs.m_status) ?
                (m_d0 < rhs.m_d0) : (m_status < rhs.m_status) ;
        }

        midi::bussbyte buss () const
        {
            return m_buss;
        }

        midi::byte status () const
        {
            return m_status;
        }

        midi::byte d0 () const
        {
            return m_d0;
        }

    };      // nested class key

private:

    /**
     *  Provides the value for active.  If false, this control will be
     *  ignored.
     */

    bool m_active;

    /**
     *  Provides the value for inverse-active.
     */

    bool m_inverse_active;

    /**
     *  Provides the value for the status.  Big question is, is the channel
     *  included here?  Yes.  So the next question is, is it ignored?  No.
     *  A number of control devices (eg. the Launchpad
     */

    midi::byte m_status;

    /**
     *  Provides the value for the first data byte of the event, d0.
     *  Useful for searches and for incoming data.
     */

    midi::byte m_d0;

    /**
     *  Provides the second data byte, d1.  It is used to check that the
     *  incoming d1 is in the range specified.  Also, though not used yet, it
     *  can further refine the operation of a MIDI control.
     */

    midi::byte m_d1;

    /**
     *  Provides the minimum value for the second data byte of the event, d1,
     *  if applicable.
     */

    midi::byte m_min_d1;

    /**
     *  Provides the maximum value for the second data byte of the event, d1,
     *  if applicable.
     */

    midi::byte m_max_d1;

public:

    /*
     *  A default constructor is needed to provide a dummy object to return
     *  when the desired one cannot be found.  The opcontrol::is_usable()
     *  function will return false.
     */

    midicontrol ();

    /*
     * The move and copy constructors, the move and copy assignment operators,
     * and the destructors are all compiler generated.
     */

    midicontrol
    (
        const std::string & keyname,
        automation::category opcategory,
        automation::action actioncode,
        automation::slot opnumber,
        int opcode
    );

    midicontrol (const midicontrol &) = default;
    midicontrol & operator = (const midicontrol &) = default;
    midicontrol (midicontrol &&) = default;
    midicontrol & operator = (midicontrol &&) = default;
    virtual ~midicontrol () = default;

    bool active () const
    {
        return m_active;
    }

    bool inverse_active () const
    {
        return m_inverse_active;
    }

    int status () const
    {
        return m_status;
    }

    int d0 () const
    {
        return m_d0;
    }

    int d1 () const
    {
        return m_d1;
    }

    int min_d1 () const
    {
        return m_min_d1;
    }

    int max_d1 () const
    {
        return m_max_d1;
    }

    /*
     *  This test does not include "inverse".
     */

    bool blank () const
    {
        return
        (
            ! m_active && m_status == 0 && m_d0 == 0 &&
            m_d1 == 0 && m_min_d1 == 0 && m_max_d1 == 0
        );
    }

    bool set (int values [automation::SUBCOUNT]);

    /**
     *  Handles a common check in the perform module.
     *
     * \param status
     *      Provides the status byte, which is checked against m_status.
     *
     * \param d0
     *      Provides the data byte, which is checked against m_d0.
     */

    bool match (midi::byte status, midi::byte d0) const
    {
        return
        (
            m_active && (status == m_status) && (d0 == m_d0)
        );
    }

    /**
     *  Handles a common check in the performer module.
     *  Move this to eventcodes.
     */

    bool in_range (midi::byte d1) const
    {
        return d1 >= midi::byte(m_min_d1) && d1 <= midi::byte(m_max_d1);
    }

public:

    key make_key () const
    {
        return key(m_status, m_d0);     /* no usage of m_d1 needed here */
    }

    bool merge_key_match (automation::category c, int opslot) const;
    void show (bool add_newline = true) const;

};              // class midicontrol

}               // namespace seq66

#endif          // RTL66_MIDICONTROL_HPP

/*
 * midicontrol.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

