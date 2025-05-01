#*****************************************************************************
# meson.build (rtl66)
#-----------------------------------------------------------------------------
##
# \file        tests/midi/meson.build
# \library     rtl66
# \author      Chris Ahlstrom
# \date        2022-06-13
# \updates     2025-01-31
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "rtl66" library. See the top-level meson.build
#  file for license information.
#
#  Note that the libcfg66_library_dep dependency is for providing the
#  message and app-info functions used by the rtl66 library. The
#  libxpc66_library_dep dependency provides ...?
#
#-----------------------------------------------------------------------------

api_names_exe = executable(
   'api_names',
   sources : ['api_names.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

cbmidiin_exe = executable(
   'cbmidiin',
   sources : ['cbmidiin.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

#-----------------------------------------------------------------------------
# Make both in and out versions of the midiclock test application.
#-----------------------------------------------------------------------------

midiclock_in_exe = executable(
   'midiclock_in',
   sources : ['midiclock.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

midiclock_out_exe = executable(
   'midiclock_out',
   sources : ['midiclock.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

midiout_exe = executable(
   'midiout',
   sources : ['midiout.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

midiprobe_exe = executable(
   'midiprobe',
   sources : ['midiprobe.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

midiprobeex_exe = executable(
   'midiprobeex',
   sources : ['midiprobeex.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep, libxpc66_library_dep ]
   )

qmidiin_exe = executable(
   'qmidiin',
   sources : ['qmidiin.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

sysextest_exe = executable(
   'sysextest',
   sources : ['sysextest.cpp'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

test_c_api_exe = executable(
   'test_c_api',
   sources : ['test_c_api.c'],
   dependencies : [ rtl66_dep, liblib66_library_dep ]
   )

smoke_exe = executable(
   'smoke',
   sources : ['smoke.cpp'],
   dependencies : [
      rtl66_dep, liblib66_library_dep, libcfg66_library_dep,
      libxpc66_library_dep
      ]
   )

play_exe = executable(
   'play',
   sources : ['play.cpp'],
   dependencies : [
                     rtl66_dep, liblib66_library_dep, libcfg66_library_dep,
                     libxpc66_library_dep
                     ]
   )

test('API Names', api_names_exe)
test('Callback MIDI In', cbmidiin_exe)
test('MIDI Clock In', midiclock_in_exe)
test('MIDI Clock Out', midiclock_out_exe)
test('MIDI Out', midiout_exe)
test('MIDI API Probe', midiprobe_exe)
test('MIDI Bus Probe', midiprobeex_exe)
test('Queue MIDI In', qmidiin_exe)
test('Sysex Test', sysextest_exe)
test('C API', test_c_api_exe)
test('Smoke Test', smoke_exe)
test('Play Test', play_exe)
   
#****************************************************************************
# meson.build (tests/rtl66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------

