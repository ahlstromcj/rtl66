#if ! defined RTL66_MIDI_TRACKINFO_HPP
#define RTL66_MIDI_TRACKINFO_HPP

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
 * \file          trackinfo.hpp
 *
 *  This module provides a stand-alone module for the event-list container
 *  used by the application.
 *
 * \library       rtl66
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2024-05-26
 * \license       GNU GPLv2 or above
 *
 *  In addition, this version contains some information from the performer and
 *  sequence classes of Seq66 that is of use in any MIDI application, so
 *  that this library can stand along for basic MIDI operations.
 *
 *  These structures hold some data that is also held in rtl::transport_info.
 *  That class is meant for use by player/performer and (JACK) transport.
 *
 */

#include <string>                       /* the omnipresent std::string      */

#include "midibytes.hpp"                /* midi::pulse, midi::byte          */

namespace midi
{

/*------------------------------------------------------------------------
 * Tempo
 *------------------------------------------------------------------------*/

/**
 *  Information needed for processing tempo.  See the cpp module
 *  for more details.
 */

class tempoinfo
{

private:

    /**
     *  The tempo track specified by the user.  Normally track 0.
     */

    int m_tempo_track;

    /**
     *  The tempo in beats/minute. Note that midi::bpm is a double value.
     */

    midi::bpm m_beats_per_minute;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Tempo meta event.
     */

    unsigned m_us_per_quarter_note;

public:

    tempoinfo (midi::bpm tempobpm = 120.0, int tempotrack = 0);
    tempoinfo (const tempoinfo &) = default;
    tempoinfo (tempoinfo &&) = default;
    tempoinfo & operator = (const tempoinfo &) = default;
    tempoinfo & operator = (tempoinfo &&) = default;
    ~tempoinfo () = default;

    std::string bpm_to_string () const;
    std::string bpm_labelled () const;
    std::string usperqn_to_string () const;
    std::string usperqn_labelled () const;

    int tempo_track () const
    {
        return m_tempo_track;
    }

    void tempo_track (int t)
    {
        m_tempo_track = t;
    }

    midi::bpm beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    void beats_per_minute (midi::bpm b)
    {
        m_beats_per_minute = b;
        m_us_per_quarter_note = int(60000000.0 / b);
    }

    unsigned us_per_quarter_note () const
    {
        return m_us_per_quarter_note;
    }

    void us_per_quarter_note (unsigned usperqn)
    {
        m_us_per_quarter_note = usperqn;
        m_beats_per_minute = 60000000.0 / usperqn;
    }

    double tempo_period_us () const;

};          // tempoinfo

/*------------------------------------------------------------------------
 * Time signature
 *------------------------------------------------------------------------*/

/**
 *  Information needed for processing a time signature. See the cpp module
 *  for more details.
 */

class timesiginfo
{

private:

    /**
     *  Holds the beats/bar value as obtained from the MIDI file.
     *  The default value is 4.
     */

    int m_beats_per_bar;

    /**
     *  Holds the beat width value as obtained from the MIDI file.
     *  The default value is 4.
     */

    int m_beat_width;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of MIDI clocks between metronome clicks.  The default value of
     *  this item is 24.  It can also be read from some SMF 1 files, such as
     *  our hymne.mid example.
     */

    unsigned m_clocks_per_metronome;

    /**
     *  Number of 32nd notes per quarter note. Defaults to 8. Kind of a
     *  weird concept.
     */

    unsigned m_thirtyseconds_per_qn;

public:

    timesiginfo
    (
        int bpb                 = 4,
        int bw                  = 4,
        unsigned cpm            = 24,
        unsigned n32nds_per_qn  = 8
    );
    timesiginfo (const timesiginfo &) = default;
    timesiginfo (timesiginfo &&) = default;
    timesiginfo & operator = (const timesiginfo &) = default;
    timesiginfo & operator = (timesiginfo &&) = default;

    std::string timesig_to_string ();
    std::string timesiginfo_labelled ();

    int beats_per_bar () const
    {
        return m_beats_per_bar;
    }

    void beats_per_bar (int bpb)
    {
        m_beats_per_bar = bpb;
    }

    int beat_width () const
    {
        return m_beat_width;
    }

    void beat_width (int bw)
    {
        m_beat_width = bw;
    }

    unsigned clocks_per_metronome () const
    {
        return m_clocks_per_metronome;
    }

    void clocks_per_metronome (unsigned cpm)
    {
        m_clocks_per_metronome = cpm;
    }

    unsigned thirtyseconds_per_qn () const
    {
        return m_thirtyseconds_per_qn;
    }

    void thirtyseconds_per_qn (unsigned v)
    {
        m_thirtyseconds_per_qn = v;
    }

};          // class timesiginfo

/*------------------------------------------------------------------------
 * Key signature
 *------------------------------------------------------------------------*/

/**
 *  Encapsulates the key signature values.
 */

class keysiginfo
{

private:

    /**
     *  The name of the key signature. It can be "calculated" from
     *  the other members. A TODO item.
     */

    std::string m_keysig_name;

    /**
     *  The number of sharps, if positive, and the number of flats, if
     *  negative.  Ranges from -7 to 7.
     */

    int m_sharp_flat_count;

    /**
     *  Indicates if the scale is a minor scale.  If false, it is a major
     *  scale.
     */

    bool m_is_minor_scale;

public:

    keysiginfo
    (
        int sfcount       = 0,
        bool minorscale   = false
    );
    keysiginfo (const keysiginfo &) = default;
    keysiginfo (keysiginfo &&) = default;
    keysiginfo & operator = (const keysiginfo &) = default;
    keysiginfo & operator = (keysiginfo &&) = default;
    ~keysiginfo () = default;

    std::string key_name ();

    const std::string & keysig_name () const
    {
        return m_keysig_name;
    }

    int sharp_flat_count () const
    {
        return int(m_sharp_flat_count);
    }

    bool is_minor_scale () const
    {
        return m_is_minor_scale;
    }

public:

    void key_name (const std::string & n)
    {
        m_keysig_name = n;
    }

    void sharp_flat_count (int sf);
    void is_minor_scale (bool isminor);

};          // class keysiginfo

/*------------------------------------------------------------------------
 * Track information
 *------------------------------------------------------------------------*/

/**
 *  Data from seq66::performers and seq66::sequence, used in building the
 *  track data from the eventslist.  This item assumes the most fundamental
 *  values used by these old classes, in order to support basic MIDI files.
 *  This information now exists in the midi::eventlist.
 */

class trackinfo
{

private:

    /**
     *  Provides the default name/title for a track.
     */

    static const std::string sm_default_name;

    /**
     *  Holds the name of the track. Grabbed from the Seq66 sequence class.
     */

    std::string m_track_name;

    /**
     *  Indicates if the track is exportable.  To be determined.
     */

    bool m_is_exportable;

    /**
     *  The putative length of the track.
     */

    midi::pulse m_length;

    /**
     *  Tempo information.
     *
     *      -   Tempo track number
     *      -   BPM
     *      -   Microseconds/quarter note
     */

    tempoinfo m_tempo_info;

    /**
     *  Time signature information. Encapulates:
     *
     *      -   Beat/width
     *      -   Beats/bar
     *      -   Clocks_per_metronome
     *      -   32ths/quarter_note
     */

    timesiginfo m_timesig_info;

    /**
     *  Key signature information.
     */

    keysiginfo m_keysig_info;

    /**
     *  Holds the channel of the eventlist, if applicable. If not the
     *  null_channel(), then all events might be modified to use this
     *  channel number.
     */

    midi::byte m_channel;

public:

    trackinfo ();
    trackinfo
    (
        const std::string & trackname,
        const tempoinfo & ti,
        const timesiginfo & tsi,
        const keysiginfo & ksi,
        bool exportable = true
    );
    trackinfo (const trackinfo &) = default;
    trackinfo (trackinfo &&) = default;
    trackinfo & operator = (const trackinfo &) = default;
    trackinfo & operator = (trackinfo &&) = default;

    timesiginfo & timesig_info ()
    {
        return m_timesig_info;
    }

    const timesiginfo & timesig_info () const
    {
        return m_timesig_info;
    }

    keysiginfo & keysig_info ()
    {
        return m_keysig_info;
    }

    const keysiginfo & keysig_info () const
    {
        return m_keysig_info;
    }

    tempoinfo & tempo_info ()
    {
        return m_tempo_info;
    }

    const tempoinfo & tempo_info () const
    {
        return m_tempo_info;
    }

    const std::string & track_name () const
    {
        return m_track_name;
    }

    bool is_exportable () const
    {
        return m_is_exportable;
    }

    midi::pulse length () const
    {
        return m_length;
    }

    midi::byte channel () const
    {
        return m_channel;
    }

public:

    static const std::string & default_name ()
    {
        return sm_default_name;
    }

    void track_name (const std::string & n)
    {
        m_track_name = n.empty() ? sm_default_name : n ;
    }

    bool is_default_name () const
    {
        return m_track_name == sm_default_name;     /* track name not set   */
    }

    void is_exportable (bool flag)
    {
        m_is_exportable = flag;;
    }

    void length (midi::pulse len)
    {
        m_length = len;
    }

    void channel (midi::byte b)
    {
        m_channel = b;
    }

};          // class trackinfo

/*------------------------------------------------------------------------
 * Free functions?
 *------------------------------------------------------------------------*/

}           // namespace midi

#endif      // RTL66_MIDI_TRACKINFO_HPP

/*
 * trackinfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

