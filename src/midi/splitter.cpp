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
 * \file          splitter.cpp
 *
 *  This module declares/defines the class for splitting a MIDI track based on
 *  channel number.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-11-24
 * \updates       2022-12-02
 * \license       GNU GPLv2 or above
 *
 *  We have recently updated this module to put Set Tempo events into the
 *  first track (channel 0).
 */

#include "c_macros.h"                   /* errprint() macro, etc.           */
#include "midi/eventlist.hpp"           /* midi::eventlist                  */
#include "midi/player.hpp"              /* midi::player track coordinator   */
#include "midi/splitter.hpp"            /* midi::splitter for SMF 0 tracks  */

namespace midi
{

/**
 *  Principal constructor.
 *
 */

splitter::splitter () :
    m_smf0_channels_count   (0),
    m_smf0_channels         (),         /* array, initialized in parse()    */
    m_smf0_main_track       (nullptr),
    m_smf0_main_number      (-1)
{
    initialize();
}

/**
 *  Resets the SMF 0 support variables in preparation for parsing a new MIDI
 *  file.
 */

void
splitter::initialize ()
{
    for (int i = 0; i < c_channel_max; ++i)
        m_smf0_channels[i] = false;
}

/**
 *  Processes a channel number by raising its flag in the m_smf0_channels[]
 *  array.  If it is the first entry for that channel, m_smf0_channels_count
 *  is incremented.  We won't check the channel number, to save time,
 *  until someday we segfault :-D
 *
 * \param channel
 *      The MIDI channel number.  The caller is responsible to make sure it
 *      ranges from 0 to 15.
 */

void
splitter::increment (int channel)
{
    if (! m_smf0_channels[channel])             /* channel not yet logged?  */
    {
        m_smf0_channels[channel] = true;
        ++m_smf0_channels_count;
    }
}

/**
 *  Logs the main track (an SMF 0 track) for later usage in splitting the
 *  track.
 *
 * /param trk
 *      The main track to be logged.
 *
 * /param trkno
 *      The track number of the main track.
 *
 * /return
 *      Returns true if the main track's address was logged, and false if
 *      it was already logged.
 */

bool
splitter::log_main_events (track & trk, track::number trkno)
{
    bool result = smf0_unlogged();              /* no main track or number  */
    if (result)
    {
        result = trkno >= 0;
        if (result)
        {
            trk.events().sort();
            m_smf0_main_track = &trk;           /* point to the main track  */
            m_smf0_main_number = trkno;
            log_color();                        /* virtual function         */
            for (const auto & ev : trk.events())
            {
                midi::byte channel = ev.channel();
                increment(channel);             /* flag & count unique chs. */
            }
            infoprint("SMF 0 main track logged and analyzed");
        }
        else
        {
            errprint("SMF 0 bad track number");
        }
    }
    else
    {
        errprint("SMF 0 main track already logged");
    }
    return result;
}

/**
 *  This function splits an SMF 0 file, splitting all of the channels in the
 *  track out into separate track, and adding each to the player
 *  object.  Lastly, it adds the SMF 0 track as the last track; the user can
 *  then examine it before removing it.  Is this worth the effort?
 *
 *  There is a little oddity, in that, if the SMF 0 track has events for only
 *  one channel, this code will still create a new track, as well as the
 *  main track.  Not sure if this is worth extra code to just change the
 *  channels on the main track and put it into the correct track for the
 *  one channel it contains.  In fact, we just want to keep it in pattern slot
 *  number 16, to keep it out of the way.
 *
 * \param p
 *      Provides a reference to the player object into which tracks
 *      are to be added.
 *
 * \return
 *      Returns true if the parsing succeeded.  Returns false if no SMF 0 main
 *      track was logged.
 */

bool
splitter::split (player & p)
{
    bool result = smf0_logged();
    if (result)
    {
        if (m_smf0_channels_count > 0)
        {
            int trkno = 0;
            for (int chan = 0; chan < c_channel_max; ++chan, ++trkno)
            {
                if (m_smf0_channels[chan])
                {
                    track * trkptr = new (std::nothrow) track(chan);
                    if (not_nullptr(trkptr))
                    {
                        if (split_channel(p, *m_smf0_main_track, *trkptr, chan))
                            p.install_track(trkptr, trkno, true);
                        else
                            delete trkptr;  /* empty track */
                    }

                }
            }
            // m_smf0_main_track->track_info().channel(null_channel());
            m_smf0_main_track->midi_channel(null_channel());
            p.install_track(m_smf0_main_track, trkno, true);    /* file load */
        }
    }
    return result;
}

/*
 * It is necessary to (redundantly, it turns out) set the master MIDI buss
 * first, before setting the other track parameters.
 *
 * These Seq66 settings are not present here:
 *
 *      trk.zero_markers();
 */


void
splitter::make_track_settings
(
    const player & p,
    track & trk,
    const std::string & name,
    track::number chan
)
{
    trk.master_midi_bus(p.master_bus());
    trk.midi_bus(chan, 0);                  /* TODO */
    trk.midi_channel(chan);
    trk.track_name(name);
}

/**
 *  This function splits the given track into a new set of tracks for the
 *  given channels found in the SMF 0 track.  It is called for each possible
 *  channel, resulting in multiple passes over the SMF 0 track.
 *
 *  The events that are read from the MIDI file have delta times. We convert
 *  these delta times to cumulative times.  Conversion back to delta times is
 *  needed only when saving the tracks to a file.  This is done in
 *  track::fill().  We have to accumulate the delta times in order to be able to
 *  set the length of the track in pulses.
 *
 *  It doesn't set the track number of the track; that is set when the
 *  track is added to the player object.
 *
 * \param maintrk
 *      This parameter is the whole SMF 0 track that was read from the MIDI
 *      file.  It contains all of the channel data that needs to be split into
 *      separate tracks.
 *
 * \param trk
 *      Provides the new track that needs to have its settings made, and
 *      all of the selected channel events added to it.
 *
 * \param channel
 *      Provides the MIDI channel number (re 0) that marks the channel data
 *      the needs to be extracted and added to the new track.  If this
 *      channel is 0, then we need to add certain Meta events to this
 *      track, as well.  So far we support only Tempo Meta events.
 *
 * \return
 *      Returns true if at least one event got added.   If none were added,
 *      the caller should delete the track object represented by parameter
 *      \a trk.
 */

bool
splitter::split_channel
(
    const player & p,
    const track & maintrk,
    track & trk,
    int chan
)
{
    bool result = false;
    char tmp[64];
    std::string main_name = maintrk.track_name();
    if (main_name.empty())
        snprintf(tmp, sizeof tmp, "Track %d", chan + 1);
    else
        snprintf(tmp, sizeof tmp, "%d: %.20s", chan + 1, main_name.c_str());

    make_track_settings(p, trk, std::string(tmp), track::number(chan));
    midi::pulse length_in_ticks = 0;            /* accumulates delta times  */
    const midi::eventlist & evl = maintrk.events();
    for (auto i = evl.cbegin(); i != evl.cend(); ++i)
    {
        const midi::event & er = midi::eventlist::cdref(i);
        if (er.is_ex_data())
        {
            if (chan == 0 || er.is_sysex())
            {
                length_in_ticks = er.timestamp();
                if (trk.events().append(er))
                    result = true;
            }
        }
        else if (er.match_channel(chan))
        {
            length_in_ticks = er.timestamp();
            if (trk.events().append(er))
                result = true;
        }
    }

    /*
     * No triggers to add.  Whew!  And setting the length is now a no-brainer,
     * since the tick value is that of the last logged event in the track.
     * Also, sort all the events after they have been appended.
     */

    trk.set_length(length_in_ticks);
    trk.events().sort();
    return result;
}

}           // namespace midi

/*
 * splitter.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

