#if ! defined RTL66_TRIGGERS_HPP
#define RTL66_TRIGGERS_HPP

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
 * \file          triggers.hpp
 *
 *  This module declares/defines the base class for handling
 *  triggers used with patterns/sequences.
 *
 * \library       rtl66 library
 * \author        Chris Ahlstrom
 * \date          2015-10-30
 * \updates       2024-06-13
 * \license       GNU GPLv2 or above
 *
 *  By segregating trigger support into its own module, the sequence class is
 *  a bit easier to understand.
 */

#include <string>
#include <stack>
#include <vector>

#include "midi/midibytes.hpp"           /* midi::pulse alias, etc.          */

namespace seq66
{

class sequence;
class triggers;

/**
 *  Indicates that there is no paste-trigger.  This is a new feature from the
 *  stazed/seq32 code.
 */

const int c_no_paste_trigger    = (-1);

/**
 *  This class hold a single trigger for a sequence object.  This class is
 *  used in playback, and instantiations of this class are contained in the
 *  triggers class defined later in this module.
 */

class trigger
{

    friend class triggers;

public:

    /**
     *  Indicates how/where a trigger will be split.
     */

    enum class splitpoint
    {
        middle,         /**< Make the split in the middle of the trigger.   */
        snap,           /**< Make the split at the nearest snap point.      */
        exact           /**< Make the split at the exact point clicked.     */
    };

private:

    /**
     *  Provides the starting tick for this trigger.  Also known as "tick on".
     */

    midi::pulse m_tick_start;

    /**
     *  Provides the ending tick for this trigger.  Also known as "tick off".
     */

    midi::pulse m_tick_end;

    /**
     *  Provides the offset for this trigger.  The offset indicates where the
     *  trigger is placed on the "perf roll".
     */

    midi::pulse m_offset;

    /**
     *  New feature.  An additional byte indicates to transpose this trigger,
     *  to implement the new c_trig_transpose SeqSpec tag.  The values range
     *  from 0 to 0x80.  0x00 indicates that transposition is not in effect.
     *  0x40 indicates that it is in effect, but has a value of 0.  Values
     *  from 0x41 to 0x80 indicate transposition from +1 to +63.  Values from
     *  0x3F to 0x01 indicate transposition from -1 to -63.
     */

    int m_transpose;

    /**
     *  Indicates that the trigger is part of a selection.
     */

    bool m_selected;

public:

    trigger ();
    trigger
    (
        midi::pulse tick, midi::pulse len,
        midi::pulse offset, midi::byte transpose = 0
    );
    trigger (const trigger &) = default;
    trigger & operator = (const trigger &) = default;
    ~trigger () = default;

    /**
     *  This operator compares only the m_tick_start members.
     *
     * \param rhs
     *      The "right-hand side" of the less-than operation.
     *
     * \return
     *      Returns true if m_tick_start is less than rhs's.
     */

    bool operator < (const trigger & rhs) const
    {
        return m_tick_start < rhs.m_tick_start;
    }

    std::string to_string () const;

    bool is_valid () const
    {
        return m_tick_end > m_tick_start;
    }

    /**
     * \getter m_tick_end and m_tick_start.
     *      We've seen that some of the calculations of trigger length are
     *      wrong, being 1 tick less than the true length of the trigger in
     *      pulses.  This function calculates trigger length the correct way.
     */

    midi::pulse length () const
    {
        return m_tick_end - m_tick_start + 1;
    }

    midi::pulse tick_start () const
    {
        return m_tick_start;
    }

    void tick_start (midi::pulse s)
    {
        m_tick_start = s;
    }

    void increment_tick_start (midi::pulse s)
    {
        m_tick_start += s;
    }

    void decrement_tick_start (midi::pulse s)
    {
        m_tick_start -= s;
    }

    /**
     *  Test if the input parameters indicate we are touching a trigger
     *  transition.  If we have reached a new chunk of drawn pattern in the
     *  Song data, and we are not recording, we unset the playback-block on
     *  this sequence's events.
     *
     * \param s
     *      The starting tick.
     *
     * \param e
     *      The ending tick.
     */

    bool at_trigger_transition (midi::pulse s, midi::pulse e)
    {
        return
        (
            s == m_tick_start || e == m_tick_start ||
            s == m_tick_end   || e == m_tick_end
        );
    }

    bool covers (midi::pulse tick)
    {
        return tick >= m_tick_start && tick <= m_tick_end;
    }

    midi::pulse tick_end () const
    {
        return m_tick_end;
    }

    void tick_end (midi::pulse e)
    {
        m_tick_end = e;
    }

    void increment_tick_end (midi::pulse s)
    {
        m_tick_end += s;
    }

    void decrement_tick_end (midi::pulse s)
    {
        m_tick_end -= s;
    }

    midi::pulse offset () const
    {
        return m_offset;
    }

    void offset (midi::pulse o)
    {
        m_offset = o;
    }

    void increment_offset (midi::pulse s)
    {
        m_offset += s;
    }

    void decrement_offset (midi::pulse s)
    {
        m_offset -= s;
    }

    /**
     *  This function maps 0x00 to 0, values less than 0x40 to transposing
     *  downward in semitones, and values greater than 0x40, but less than
     *  0x80, to transposing upward in semitones. Value 0x40 is not used.  We
     *  can transpose up and down by 63 semitones, or a little more than 5
     *  octaves.
     */

    midi::byte transpose_byte () const
    {
        return m_transpose == 0 ? 0 : midi::byte(m_transpose + 0x40);
    }

    void transpose_byte (midi::byte t)                /* when reading a file  */
    {
        if (t > 0x00 && t < 0x80)
            m_transpose = t - 0x40;
        else
            m_transpose = 0;                        /* no transpose         */
    }

    int transpose () const
    {
        return m_transpose;
    }

    bool transposed () const
    {
        return m_transpose != 0;
    }

    static int datasize (midi::ulong seqspec);

    void transpose (int t)                          /* to modify a trigger  */
    {
        if (t > (-64) && t < 64)                    /* -63 to 0 to +63      */
            m_transpose = t;
    }

    bool selected () const
    {
        return m_selected;
    }

    void selected (bool s)
    {
        m_selected = s;
    }

    /**
     *  Encapsulates a qperfroll trigger calculation.
     */

    midi::pulse trigger_marker (midi::pulse len)
    {
        return m_tick_start - (m_tick_start % len) + (m_offset % len) - len;
    }

private:

    void rescale (int oldppqn, int newppqn);

};          // class trigger

/**
 *  The triggers class is a receptable the triggers that can be used with a
 *  sequence object.
 */

class triggers
{
    friend class midi_vector_base;
    friend class midifile;
    friend class sequence;

public:

    /**
     *  Provides an alias introduced by Stazed to make the trigger grow/move
     *  code easier to understand.
     */

    enum class grow
    {
        start  = 0,    /**< Grow the start of the trigger.         */
        end    = 1,    /**< Grow the end of the trigger.           */
        move   = 2     /**< Move the entire trigger block.         */
    };

private:

    /**
     *  Exposes the triggers type, currently needed for midi_vector_base only.
     *  We might convert to using a vector instead of a list.
     */

    using container = std::vector<trigger>;

    /**
     *  Provides a stack for use with the undo/redo features of the
     *  trigger support.
     */

    using stack = std::stack<container>;

private:

    /**
     *  Holds a reference to the parent sequence object that owns this trigger
     *  object.
     */

    sequence & m_parent;

    /**
     *  This list holds the current pattern/triggers events.
     */

    container m_triggers;

    /**
     *  Holds a count of the selected triggers, for better control over
     *  selections.
     */

    int m_number_selected;

    /**
     *  This item holds a single copied trigger, to be pasted later.
     */

    trigger m_clipboard;

    /**
     *  Handles the undo list for a series of operations on triggers.
     */

    stack m_undo_stack;

    /**
     *  Handles the redo list for a series of operations on triggers.
     */

    stack m_redo_stack;

    /**
     *  An iterator for cycling through the triggers during drawing.
     */

    container::iterator m_draw_iterator;

    /**
     *  Set to true if there is an active trigger in the trigger clipboard.
     */

    bool m_trigger_copied;

    /**
     *  The tick point for pasting.  Set to -1 if not in force.  This is a new
     *  feature from stazed's Seq32 project.
     */

    midi::pulse m_paste_tick;

    /**
     *  Holds the value of the PPQN from the parent sequence, for easy access.
     *  This should not change, but we have to set it after construction, and
     *  so we provide a setter for it, set_ppqn(), called by the sequence
     *  constructor.
     */

    int m_ppqn;

    /**
     *  Holds the value of the length from the parent sequence, for easy access.
     *  This might change, we're not yet sure.
     */

    int m_length;

public:

    triggers (sequence & parent);
    ~triggers () = default;
    triggers (const triggers & rhs) = default;
    triggers & operator = (const triggers & rhs);

    std::string to_string () const;
    bool change_ppqn (int p);

    /**
     * \setter m_ppqn
     *      We have to set this value after construction for best safety.
     */

    void set_ppqn (int ppqn)
    {
        if (ppqn > 0)
            m_ppqn = ppqn;
    }

    /**
     * \setter m_length
     *      We have to set this value after construction for best safety.
     *      Also, there a chance that the length of the parent might change
     *      from time to time.  Currently, only the sequence constructor and
     *      midifile call this function.
     */

    void set_length (int len)
    {
        if (len > 0)
            m_length = len;
    }

    const container & triggerlist () const
    {
        return m_triggers;
    }

    container & triggerlist ()
    {
        return m_triggers;
    }

    int count () const
    {
        return int(m_triggers.size());
    }

    int datasize (midi::ulong seqspec) const;
    bool any_transposed () const;

    int number_selected () const
    {
        return m_number_selected;
    }

    void push_undo ();
    void pop_undo ();
    void pop_redo ();
    void print (const std::string & seqname) const;
    bool play
    (
        midi::pulse & starttick, midi::pulse & endtick,
        int & transpose, bool resume = false
    );
    void add
    (
        midi::pulse tick, midi::pulse len,
        midi::pulse offset = 0, midi::byte transpose = 0,
        bool adjustoffset = true
    );
    void adjust_offsets_to_length (midi::pulse newlen);
    bool split (midi::pulse tick, trigger::splitpoint splittype);
    bool grow_trigger (midi::pulse tickfrom, midi::pulse tickto, midi::pulse length);
    const trigger & find_trigger (midi::pulse tick) const;
    const trigger & find_trigger_by_index (int index) const;
    bool remove (midi::pulse tick);
    bool get_state (midi::pulse tick) const;
    bool transpose (midi::pulse tick, int transposition);
    bool select (midi::pulse tick);
    bool unselect (midi::pulse tick);
    bool unselect ();
    bool intersect (midi::pulse position, midi::pulse & start, midi::pulse & end);
    bool intersect (midi::pulse position);

    bool remove_selected ();
    void copy_selected ();
    void paste (midi::pulse paste_tick = c_no_paste_trigger);
    bool move_selected
    (
        midi::pulse tick, bool adjustoffset,
        triggers::grow which = triggers::grow::move
    );
    midi::pulse get_selected_start ();
    midi::pulse get_selected_end ();
    midi::pulse get_maximum () const;
    bool move
    (
        midi::pulse starttick, midi::pulse distance,
        bool direction, bool single = true
    );
    void move_split
    (
        midi::pulse starttick, midi::pulse distance,
        bool direction
    );
    void copy (midi::pulse starttick, midi::pulse distance);

    /**
     *  Clears the whole list of triggers, and zeroes the number selected.
     */

    void clear ()
    {
        m_triggers.clear();
        m_number_selected = 0;
    }

    trigger next ();

    /**
     *  Sets the draw-trigger iterator to the beginning of the trigger list.
     */

    void reset_draw_trigger_marker ()
    {
        m_draw_iterator = m_triggers.begin();
    }

    void set_trigger_paste_tick (midi::pulse tick)
    {
        m_paste_tick = tick;
    }

    midi::pulse get_trigger_paste_tick () const
    {
        return m_paste_tick;
    }

private:

    void sort ();
    bool split (trigger & t, midi::pulse splittick);
    bool rescale (int oldppqn, int newppqn);
    midi::pulse adjust_offset (midi::pulse offset);
    void offset_selected (midi::pulse tick, grow editmode);
    void select (trigger & t, bool count = true);
    void unselect (trigger & t, bool count = true);

    bool cend (container::iterator & evi) const // no can do const_iterator
    {
        return evi == m_triggers.cend();
    }

};          // class triggers

}           // namespace seq66

#endif      // RTL66_TRIGGERS_HPP

/*
 * triggers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

