#if ! defined RTL66_MIDI_SPLITTER_HPP
#define RTL66_MIDI_SPLITTER_HPP

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
 * \file          splitter.hpp
 *
 *      This module declares/defines the base class for MIDI files.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-11-24
 * \updates       2024-05-26
 * \license       GNU GPLv2 or above
 *
 *      Rtl66 can split an SMF 0 file into multiple tracks, effectively converting
 *      it to SMF 1.  This class holds all the information needed to do that.
 */

#include "midi/track.hpp"               /* midi::track for SMF 0 tracks     */

namespace midi
{

    class player;                       /* precursor to seq::performer      */

/**
 *  This class handles the parsing and writing of SMF 0 files. We replace the
 *  Seq66 sequence with the more basic midi::track.
 */

class splitter
{

private:

    /**
     *  Provides support for SMF 0, indicates how many channels were found in
     *  the file in a single sequence.  SMF 1 file parsing will only warn
     *  about more than one channel found in a given sequence.
     */

    int m_smf0_channels_count;

    /**
     *  Provides support for SMF 0, holds a bool value that indicates the
     *  occurrence of a given channel.  We won't worry about multiple
     *  MIDI busses here.
     */

    bool m_smf0_channels[c_channel_max];

    /**
     *  Provides support for SMF 0, points to the initial SMF 0 track, from
     *  which the single-channel tracks will be created.
     */

    track * m_smf0_main_track;

    /**
     *  Provides support for SMF 0, holds the prospective sequence number of
     *  the main (SMF 0) sequence.  We want to be able to add that sequence
     *  last, for easier and cleaner removal of that sequence by the user.
     */

    track::number m_smf0_main_number;

public:

    splitter ();
    splitter (const splitter &) = delete;
    splitter (splitter &&) = default;
    splitter & operator = (const splitter &) = delete;
    splitter & operator = (splitter &&) = default;
    virtual ~splitter () = default;

    void initialize ();
    void increment (int channel);
    bool log_main_events (track & trk, track::number trkno);
    bool split (player & p);

    virtual void log_color ()
    {
        // color not supported in this base class
    }

    /**
     * \getter m_smf0_channels_count
     */

    int count () const
    {
        return m_smf0_channels_count;
    }

    bool smf0_logged () const
    {
        return m_smf0_main_number >= 0;
    }

    bool smf0_unlogged () const
    {
        return m_smf0_main_number < 0;
    }

    bool channel_logged (int chan) const
    {
        return is_good_channel(chan) ? m_smf0_channels[chan] : false ;
    }

protected:

    virtual void make_track_settings
    (
        const player & p,
        track & trk,
        const std::string & name,
        track::number chan
    );

    bool split_channel
    (
        const player & p,
        const track & maintrk,
        track & trk,
        int channel
    );

};          // class splitter

}           // namespace midi

#endif      // RTL66_MIDI_SPLITTER_HPP

/*
 * splitter.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

