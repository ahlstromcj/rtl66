#if ! defined RTL66_MIDI_TRACKDATAEX_HPP
#define RTL66_MIDI_TRACKDATAEX_HPP

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
 * \file          trackdataex.hpp
 *
 *  This module declares a class for managing raw MIDI events in an event
 *  list.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2022-12-04
 * \license       GNU GPLv2 or above
 *
 *  This class is meant to hold the bytes that represent MIDI events and other
 *  MIDI data, which can then be dumped to a MIDI file.
 */

#include <iosfwd>                       /* std::ifstream forward reference  */
#include <string>                       /* std::string class                */

#include "midi/eventlist.hpp"           /* midi::eventlist container class  */
#include "midi/trackinfo.hpp"           /* small info classes for MIDI      */

namespace midi
{

/**
 *    This class is the base class for a container of MIDI track
 *    information.  It is one of the base classes for the track class.
 */

class trackdataex
{
private:

public:

    trackdataex ();

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~trackdataex ()
    {
        // empty body
    }

    /*---------------------------------------------------------------------
     * "get" functions
     *---------------------------------------------------------------------*/

    /*---------------------------------------------------------------------
     * "put" functions
     *---------------------------------------------------------------------*/

    void put_seqspec ();
    midi::pulse song_put_seq_event
    (
        const trigger & trig, midi::pulse prev_timestamp
    );
    void song_put_seq_trigger
    (
        const trigger & trig, midi::pulse len, midi::pulse prev_timestamp
    );
    bool song_put_track
    (
        int track, int tempotrack,
        bool exportable = true, bool standalone = true,
        bool hastimesig = false, bool hastempo = false
    );

protected:

    /*---------------------------------------------------------------------
     * "extract" functions
     *---------------------------------------------------------------------*/

    bool extract_meta_msg (midi::pulse currenttime, midi::status eventstat);
    bool extract_tempo (track & trk, int tracknumber);
    bool extract_time_signature (midi::track & trk, int tracknumber);

    /*---------------------------------------------------------------------
     * "parse" functions
     *---------------------------------------------------------------------*/

    bool parse (const midi::bytes & data, size_t offset, size_t len);
    midi::ulong parse_seqspec_header (int file_size);
    bool parse_seqspec_track (midi::eventlist & evlist, int file_size);

protected:

};          // class trackdataex

}           // namespace midi

#endif      // RTL66_MIDI_TRACKDATAEX_HPP

/*
 * trackdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

