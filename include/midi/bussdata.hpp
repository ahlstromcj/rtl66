#if ! defined RTL66_MIDI_BUSSDATA_HPP
#define RTL66_MIDI_BUSSDATA_HPP

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
 * \file          bussdata.hpp
 *
 *  A data class for holding the desired status of a midi::bus object, plus
 *  the midi::port data.
 *
 * \library       rtl66 application
 * \author        Chris Ahlstrom
 * \date          2024-06-10
 * \updates       2024-10-28
 * \license       See above.
 *
 *  Contains information about a single MIDI bus. It extends the midi::port
 *  class to provide additional information needed to create a midi::bus
 *  object.
 *
 *  Don't confuse midi::bussdata with midi::bus. The former is data, the
 *  latter is created in part based on that data.
 */

#include "clocking.hpp"                 /* midi::clocking                   */
#include "port.hpp"                     /* midi::port                       */

namespace midi
{

/**
 *  A structure for hold basic information about a single (MIDI) port.
 *  Except for the virtual-vs-normal status, this information is obtained by
 *  scanning the system at the startup time of the application.
 */

class bussdata : public port
{

public:

    /**
     *  Provides a bit mask for ignoring some MIDI events, a feature from
     *  RtMidi.
     */

    enum class ignore
    {
        none            = 0x00,
        sysex           = 0x01,
        timing          = 0x02,
        active_sense    = 0x04,
        all             = 0x07
    };

private:

    /*
     *  Note that we now use just clocking to indicate enabled/disabled,
     *  at the risk of some confusion.
     *
     *      bool m_enabled {false};
     *
     * To do?
     *
     *  -   Do we need a buss override value?
     *  -   We can use all these parameters in an update MIDI port GUI.
     */

    int m_bus_index {0};                        /**< Ordinal from caller.   */
    std::string m_nick_name {};                 /**< Short buss name.       */
    clocking m_out_clock {clocking::unavailable}; /**< I/O/Clock setting.   */
    std::size_t m_queue_size {0};               /**< Max. ALSA buffer size. */
    ignore m_ignore_midi_flags {ignore::none};  /**< Allow fast events.     */

public:

    bussdata () = default;
    bussdata
    (
        int index,
        clocking c,
        int bussnumber,
        const std::string & bussname,
        int portnumber,
        const std::string & portname,
        io iotype,
        kind porttype,
        int queuenumber                 = (-1),
        const std::string & aliasname   = "",
        const std::string & nickname    = "",
        std::size_t queuesize           = 0,
        ignore ignoreflags              = ignore::none
    );
    bussdata
    (
        int index,
        clocking c,
        const port & p,
        const std::string & nickname    = "",
        std::size_t queuesize           = 0,
        ignore ignoreflags              = ignore::none
    );
    bussdata (const bussdata &) = default;
    bussdata (bussdata &&) = default;
    bussdata & operator = (const bussdata &) = default;
    bussdata & operator = (bussdata &&) = default;
    ~bussdata () = default;

    /*----------------------------------------------------------------------
     * Getters
     *----------------------------------------------------------------------*/

    int bus_index () const
    {
        return m_bus_index;
    }

    const std::string & nick_name () const
    {
        return m_nick_name;
    }

    clocking out_clock () const
    {
        return m_out_clock;
    }

    std::size_t queue_size () const
    {
        return m_queue_size;
    }

    ignore ignore_midi_flags () const
    {
        return m_ignore_midi_flags;
    }

    /*
     * This class cannot see the inline operators at the bottom of this module.
     */

    bool ignore_test (bussdata::ignore lhs, bussdata::ignore rhs) const
    {
        return static_cast<bussdata::ignore>
        (
            static_cast<int>(lhs) & static_cast<int>(rhs)
        ) != ignore::none;
    }

    bool ignore_sysex () const
    {
        return ignore_test(ignore_midi_flags(), ignore::sysex);
    }

    bool ignore_timing () const
    {
        return ignore_test(ignore_midi_flags(), ignore::timing);
    }

    bool ignore_active_sense () const
    {
        return ignore_test(ignore_midi_flags(), ignore::active_sense);
    }

    /*----------------------------------------------------------------------
     * Setters
     *----------------------------------------------------------------------*/

#if 0
    void set_ignore_midi {bussdata::ignore f}
    {
        m_ignore_midi_flags = f;
    }
#endif

protected:

    std::string construct_bus_name (const std::string & appname);
    std::string make_nickname () const;

    void queue_size (std::size_t sz)
    {
        m_queue_size = sz;
    }

};          // class bussdata


/*--------------------------------------------------------------------------
 * Inline and free fuctions in the midi namespace.
 *--------------------------------------------------------------------------*/

inline bussdata::ignore
operator | (bussdata::ignore lhs, bussdata::ignore rhs)
{
    return static_cast<bussdata::ignore>
    (
        static_cast<int>(lhs) | static_cast<int>(rhs)
    );
}

inline bussdata::ignore &
operator |= (bussdata::ignore & lhs, bussdata::ignore rhs)
{
    lhs = static_cast<bussdata::ignore>
    (
        static_cast<int>(lhs) | static_cast<int>(rhs)
    );
    return lhs;
}

inline bussdata::ignore
operator & (bussdata::ignore lhs, bussdata::ignore rhs)
{
    return static_cast<bussdata::ignore>
    (
        static_cast<int>(lhs) & static_cast<int>(rhs)
    );
}

inline bussdata::ignore &
operator &= (bussdata::ignore & lhs, bussdata::ignore rhs)
{
    lhs = static_cast<bussdata::ignore>
    (
        static_cast<int>(lhs) & static_cast<int>(rhs)
    );
    return lhs;
}

extern const bussdata & stock_in_buss_settings ();
extern const bussdata & stock_out_buss_settings ();

}           // namespace midi

#endif      // RTL66_MIDI_BUSSDATA_HPP

/*
 * bussdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

