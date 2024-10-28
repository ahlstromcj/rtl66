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
 * \file          tracklist.cpp
 *
 *  This module declares a class for holding and managing MIDI data.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2022-07-09
 * \updates       2024-05-22
 * \license       GNU GPLv2 or above
 *
 *  This class is important when writing the MIDI and sequencer data out to a
 *  MIDI file.  The data handled here are specific to a single
 *  sequence/pattern/track.
 *
 *  Compare tracklist to the screenset and playset classes in the original
 *  Seq66.  A screenset contains a vector of seqs; a seq is a wrapper around
 *  a sequence that adds some more information. That information might better
 *  be added to a track-derived class. The container is a vector of fixed size
 *  that can include empty slots. The screenset serves to manage a "page" of
 *  tracks/sequeneces.
 *
 *  The playset holds a vector of shared-pointers to seqs. It holds all of
 *  the sequences that are to be played, and can span a number of screensets.
 *
 *  The tracklist (currently) holds a map to shared-pointers to tracks. Is
 *  using a map inefficient? It is slower than a vector, but the number of
 *  items is low. It can hold only existing tracks, and cannot represent an
 *  ordered set that includes empty track slots.  On the other hand, a vector
 *  must be sorted, which would require providing a track::operator <().
 *
 *  It would be nice to be able to use the tracklist for screensets and the
 *  playset! So we go the vector route.
 *
 */

#include <algorithm>                    /* std::sort()                      */
#include <stdexcept>                    /* std::out_of_range exception      */

#include "c_macros.h"                   /* errprint() macro                 */
#include "midi/tracklist.hpp"           /* midi::tracklist class            */

namespace midi
{

/**
 *  Fills in the few members of this class.
 *
 * \param seq
 *      Provides a reference to the sequence/track for which this container
 *      holds MIDI data.
 */

tracklist::tracklist () :
    m_tracks    (),
    m_modified  (false),
    m_sorted    (false)
{
    // Empty body
}

bool
tracklist::modified () const
{
    if (! m_modified)
    {
        for (const auto & trk : tracks())
        {
            if (trk->modified())
                return true;
        }
    }
    return m_modified;
}

void
tracklist::unmodify (lib66::notification n)
{
    (void) n;
    m_modified = false;
    for (const auto & trk : tracks())
        trk->unmodify();
}

/**
 *  Adds the pointer to the vector. This function currently does not check for
 *  duplicate track numbers. Nor does it sort the tracks; in this class
 *  we have no criterion for sorting tracks anyway.
 */

bool
tracklist::add (track::number trkno, track * trk)
{
    bool result = not_nullptr(trk);
    if (result)
    {
        track::pointer trkptr = track::pointer(trk);
        track::number tn = trkno;
        if (track::is_unassigned(trkno))
            tn = track::number(tracks().size());

        trk->track_number(tn);
        tracks().push_back(trkptr);     /* and do not sort the list */
        modify();
    }
    return result;
}

/**
 *  We need to reconsider this function.
 */

void
tracklist::sort ()
{
    std::sort(tracks().begin(), tracks().end());
    m_sorted = true;
}

track::pointer
tracklist::at (track::number trkno)
{
    try
    {
        track::pointer result = tracks().at(trkno);
        return result;
    }
    catch (const std::out_of_range &)
    {
        static track::pointer s_null;
        return s_null;
    }
}

const track::pointer
tracklist::at (track::number trkno) const
{
    try
    {
        track::pointer result = tracks().at(trkno);
        return result;
    }
    catch (const std::out_of_range &)
    {
        static track::pointer s_null;
        return s_null;
    }
}

}           // namespace midi

/*
 * track.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

