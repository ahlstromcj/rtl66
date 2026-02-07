#*****************************************************************************
# meson.build (rtl66)
#-----------------------------------------------------------------------------
##
# \file        src/meson.build
# \library     rtl66
# \author      Chris Ahlstrom
# \date        2022-06-08
# \updates     2026-02-07
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "rtl66" library. See the top-level meson.build
#  file for license information.
#
#  Here we locate the library source-code files.  They reside in:
#
#     -  src. The main include directory, currently nothing needed there.
#     -  src/audio. RtAudio code adaptation. NOT READY.
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
   'midi/calculations.cpp',
   'midi/clientinfo.cpp',
   'midi/clocking.cpp',
   'midi/event.cpp',
   'midi/eventcodes.cpp',
   'midi/eventlist.cpp',
   'midi/file.cpp',
   'midi/masterbus.cpp',
   'midi/message.cpp',
   'midi/midibytes.cpp',
   'midi/player.cpp',
   'midi/poller.cpp',
   'midi/portnaming.cpp',
   'midi/port.cpp',
   'midi/ports.cpp',
   'midi/scales.cpp',
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
   'rtl/midi/alsa/pollwrapper.cpp',
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
   'rtl/midi/rtmidi_io_info.cpp',
   'rtl/midi/rtmidi_out.cpp',
   'rtl/midi/webmidi/midi_web_midi.cpp',
   'rtl/midi/winmm/midi_win_mm.cpp',
   'rtl/midi/winmm/midi_win_mm_data.cpp',
   'seq66/ctrl/automation.cpp',
   'seq66/ctrl/keycontainer.cpp',
   'seq66/ctrl/keycontrol.cpp',
   'seq66/ctrl/keymap.cpp',
   'seq66/ctrl/keystroke.cpp',
   'seq66/ctrl/midicontrolbase.cpp',
   'seq66/ctrl/midicontrol.cpp',
   'seq66/ctrl/midicontrolin.cpp',
   'seq66/ctrl/midicontrolout.cpp',
   'seq66/ctrl/midimacro.cpp',
   'seq66/ctrl/midimacros.cpp',
   'seq66/ctrl/midioperation.cpp',
   'seq66/ctrl/opcontainer.cpp',
   'seq66/ctrl/opcontrol.cpp',
   'seq66/play/clockslist.cpp',
   'seq66/play/inputslist.cpp',
   'seq66/play/mutegroup.cpp',
   'seq66/play/mutegroups.cpp',
   'seq66/play/notemapper.cpp',
   'seq66/play/portslist.cpp',
   'seq66/play/seq.cpp',
   'seq66/play/triggers.cpp',
   'session/rtlconfiguration.cpp',
   'session/rtlmanager.cpp',
   'transport/clock/info.cpp',
   'transport/info.cpp',
   'transport/scratchpad.cpp',
   'transport/jack/info.cpp',
   'transport/jack/transport.cpp'
   )

'''
   'seq66/play/sequence.cpp',
   'seq66/midi/seqspec.cpp',
   'seq66/midi/trackdata.cpp'
   'seq66/play/metro.cpp',
   'seq66/play/performer.cpp',
   'seq66/play/playlist.cpp',
   'seq66/play/screenset.cpp',
   'seq66/play/setmapper.cpp',
   'seq66/play/setmaster.cpp',
   'seq66/play/songsummary.cpp',
   'seq66/session/defaults.cpp',
   'seq66/session/nsm/clinsmanager.cpp',
'''

#****************************************************************************
# meson.build (src/rtl66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
