#*****************************************************************************
# meson.build (rtl66)
#-----------------------------------------------------------------------------
##
# \file        include/meson.build
# \library     rtl66
# \author      Chris Ahlstrom
# \date        2022-06-08
# \updates     2024-10-28
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "rtl66" library. See the top-level meson.build
#  file for license information.
#
#  Here we locate the headers.  They reside in:
#
#     -  include. The main include directory, for general definitions.
#     -  include/midi. Contains headers adapted from the Seq66 project.
#     -  include/rtl. Contains headers refactored from the RtMidi project.
#     -  include/transport. Will contain headers related to JACK transport
#        (and perhaps other transport like Pipewire).
#
#-----------------------------------------------------------------------------

librtl66_headers += files(
   'midi/busarray.hpp',
   'midi/bus.hpp',
   'midi/bus_in.hpp',
   'midi/bus_out.hpp',
   'midi/bussdata.hpp',
   'midi/calculations.hpp',
   'midi/clientinfo.hpp',
   'midi/clocking.hpp',
   'midi/event.hpp',
   'midi/eventcodes.hpp',
   'midi/eventlist.hpp',
   'midi/file.hpp',
   'midi/masterbus.hpp',
   'midi/measures.hpp',
   'midi/message.hpp',
   'midi/midibytes.hpp',
   'midi/player.hpp',
   'midi/portnaming.hpp',
   'midi/port.hpp',
   'midi/ports.hpp',
   'midi/splitter.hpp',
   'midi/timing.hpp',
   'midi/track.hpp',
   'midi/trackdata.hpp',
   'midi/trackinfo.hpp',
   'midi/tracklist.hpp',
   'rtl/api_base.hpp',
   'rtl/iothread.hpp',
   'rtl/rtl_build_macros.h',
   'rtl/rt_types.hpp',
   'rtl/rterror.hpp',
   'rtl/test_helpers.hpp',
   'rtl/audio/audio_api.hpp',
   'rtl/audio/audio_support.hpp',
   'rtl/audio/rt_audio_types.hpp',
   'rtl/audio/rtaudio.hpp',
   'rtl/midi/alsa/midi_alsa.hpp',
   'rtl/midi/alsa/midi_alsa_data.hpp',
   'rtl/midi/find_midi_api.hpp',
   'rtl/midi/jack/midi_jack.hpp',
   'rtl/midi/jack/midi_jack_callbacks.hpp',
   'rtl/midi/jack/midi_jack_data.hpp',
   'rtl/midi/macosx/midi_macosx_core.hpp',
   'rtl/midi/midi_api.hpp',
   'rtl/midi/midi_dummy.hpp',
   'rtl/midi/midi_queue.hpp',
   'rtl/midi/pipewire/midi_pipewire.hpp',
   'rtl/midi/rt_midi_types.hpp',
   'rtl/midi/rtmidi.hpp',
   'rtl/midi/rtmidi_c.h',
   'rtl/midi/rtmidi_engine.hpp',
   'rtl/midi/rtmidi_in.hpp',
   'rtl/midi/rtmidi_in_data.hpp',
   'rtl/midi/rtmidi_out.hpp',
   'rtl/midi/webmidi/midi_web_midi.hpp',
   'rtl/midi/winmm/midi_win_mm.hpp',
   'rtl/midi/winmm/midi_win_mm_data.hpp',
   'session/rtlconfiguration.hpp',
   'session/rtlmanager.hpp',
   'transport/clock/info.hpp',
   'transport/info.hpp',
   'transport/jack/info.hpp',
   'transport/jack/scratchpad.hpp',
   'transport/jack/transport.hpp'
   )

configure_file(
   input : 'rtl66-config.h.in',
   output : 'rtl66-config.h',
   configuration : cdata
   )

#****************************************************************************
# meson.build (include/rtl66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
