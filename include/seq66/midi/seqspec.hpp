#if ! defined SEQ66_SEQSPEC_HPP
#define SEQ66_SEQSPEC_HPP

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          seqspec.hpp
 *
 *  This module declares a class for sequencer-specific MIDI events in a
 *  sequence/track.
 *
 * \library       seq66
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2022-07-05
 * \license       GNU GPLv2 or above
 *
 */

#include <string>                       /* std::string class                */

#include "midi/midibytes.hpp"           /* midi::byte, midi::bytes, etc.    */

namespace seq66
{

/**
 *  Provides tags used by the midifile class to control the reading and
 *  writing of the extra "proprietary" information stored in a Seq24 MIDI
 *  file.  See the cpp file for more information.
 */

enum class seqspec : midi::long
{
    midibus        = 0x24240001,        /**< Track buss number.         */
    midichannel    = 0x24240002,        /**< Track channel number.      */
    midiclocks     = 0x24240003,        /**< Track clocking.            */
    triggers       = 0x24240004,        /**< Legacy Seq24 triggers.     */
    notes          = 0x24240005,        /**< Song data.                 */
    timesig        = 0x24240006,        /**< Track time signature.      */
    bpmtag         = 0x24240007,        /**< Song beats/minute.         */
    triggers_ex    = 0x24240008,        /**< Trigger data w/offset.     */
    mutegroups     = 0x24240009,        /**< Song mute group data.      */
    gap_A          = 0x2424000A,        /**< Gap. A.                    */
    gap_B          = 0x2424000B,        /**< Gap. B.                    */
    gap_C          = 0x2424000C,        /**< Gap. C.                    */
    gap_D          = 0x2424000D,        /**< Gap. D.                    */
    gap_E          = 0x2424000E,        /**< Gap. E.                    */
    gap_F          = 0x2424000F,        /**< Gap. F.                    */
    midictrl       = 0x24240010,        /**< Song MIDI control.         */
    musickey       = 0x24240011,        /**< The track's key. *         */
    musicscale     = 0x24240012,        /**< The track's scale. *       */
    backsequence   = 0x24240013,        /**< Track background sequence. */
    transpose      = 0x24240014,        /**< Track transpose value.     */
    perf_bp_mes    = 0x24240015,        /**< Perfedit beats/measure.    */
    perf_bw        = 0x24240016,        /**< Perfedit beat-width.       */
    tempo_map      = 0x24240017,        /**< Reserve seq32 tempo map.   */
    reserved_1     = 0x24240018,        /**< Reserved for expansion.    */
    reserved_2     = 0x24240019,        /**< Reserved for expansion.    */
    tempo_track    = 0x2424001A,        /**< Alternate tempo track no.  */
    seq_color      = 0x2424001B,        /**< Feature from Kepler34.     */
    seq_edit_mode  = 0x2424001C,        /**< Feature from Kepler34.     */
    seq_loopcount  = 0x2424001D,        /**< N-play loop, 0 = infinite. */
    reserved_3     = 0x2424001E,        /**< Reserved for expansion.    */
    reserved_4     = 0x2424001F,        /**< Reserved for expansion.    */
    trig_transpose = 0x24240020,        /**< Triggers with transpose.   */
    max                                 /**< Terminator                 */
};

/*------------------------------------------------------------------------
 * Free functions
 *------------------------------------------------------------------------*/

extern std::string to_string (seqspec sp);

}           // namespace seq66

#endif      // SEQ66_SEQSPEC_HPP

/*
 * track.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

