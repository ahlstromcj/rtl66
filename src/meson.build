#*****************************************************************************
# meson.build (rtl66)
#-----------------------------------------------------------------------------
##
# \file        src/meson.build
# \library     rtl66
# \author      Chris Ahlstrom
# \date        2022-06-08
# \updates     2024-10-28
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "rtl66" library. See the top-level meson.build
#  file for license information.
#
#  Here we locate the library source-code files.  They reside in:
#
#     -  src. The main include directory, currently nothing needed there.
#     -  src/midi. Contains code adapted from the Seq66 project.
#     -  src/rtl. Contains code refactored from the RtMidi project.
#     -  src/transport. Contains code related to JACK transport
#        (and perhaps other transport like Pipewire and MIDI clock).
#
#-----------------------------------------------------------------------------

librtl66_sources += files(
   'midi/busarray.cpp',
   'midi/bus.cpp',
   'midi/bus_in.cpp',
   'midi/bus_out.cpp',
   'midi/bussdata.cpp',
   'midi/calculations.cpp',
   'midi/clientinfo.cpp',
   'midi/event.cpp',
   'midi/eventcodes.cpp',
   'midi/eventlist.cpp',
   'midi/file.cpp',
   'midi/masterbus.cpp',
   'midi/message.cpp',
   'midi/midibytes.cpp',
   'midi/player.cpp',
   'midi/portnaming.cpp',
   'midi/port.cpp',
   'midi/ports.cpp',
   'midi/splitter.cpp',
   'midi/track.cpp',
   'midi/trackdata.cpp',
   'midi/trackinfo.cpp',
   'midi/tracklist.cpp',
   'rtl/api_base.cpp',
   'rtl/iothread.cpp',
   'rtl/test_helpers.cpp',
   'rtl/audio/audio_api.cpp',
   'rtl/audio/audio_support.cpp',
   'rtl/audio/rtaudio.cpp',
   'rtl/midi/alsa/midi_alsa.cpp',
   'rtl/midi/alsa/midi_alsa_data.cpp',
   'rtl/midi/find_midi_api.cpp',
   'rtl/midi/jack/midi_jack.cpp',
   'rtl/midi/jack/midi_jack_callbacks.cpp',
   'rtl/midi/jack/midi_jack_data.cpp',
   'rtl/midi/macosx/midi_macosx_core.cpp',
   'rtl/midi/midi_api.cpp',
   'rtl/midi/midi_dummy.cpp',
   'rtl/midi/midi_queue.cpp',
   'rtl/midi/pipewire/midi_pipewire.cpp',
   'rtl/midi/rtmidi.cpp',
   'rtl/midi/rtmidi_c.cpp',
   'rtl/midi/rtmidi_engine.cpp',
   'rtl/midi/rtmidi_in.cpp',
   'rtl/midi/rtmidi_in_data.cpp',
   'rtl/midi/rtmidi_out.cpp',
   'rtl/midi/webmidi/midi_web_midi.cpp',
   'rtl/midi/winmm/midi_win_mm.cpp',
   'rtl/midi/winmm/midi_win_mm_data.cpp',
   'session/rtlconfiguration.cpp',
   'session/rtlmanager.cpp',
   'transport/clock/info.cpp',
   'transport/info.cpp',
   'transport/jack/info.cpp',
   'transport/jack/scratchpad.cpp',
   'transport/jack/transport.cpp'
   )

#****************************************************************************
# meson.build (src/rtl66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
