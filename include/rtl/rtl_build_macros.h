#if ! defined RTL66_RTL_BUILD_MACROS_H
#define RTL66_RTL_BUILD_MACROS_H

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
 * \file          rtl_build_macros.h
 *
 *  Macros that depend upon the build platform.
 *
 * \library       rtl66
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2022-06-05
 * \updates       2024-06-01
 * \license       See above.
 *
 * Introduction:
 *
 *      We have two ways of configuring the build for the rtl66 library.  This
 *      file presents features that depend upon the build platform, and
 *      build options that might be considered permanent, such as the option
 *      to pick from various JACK-processing callbacks.
 */

#include "platform_macros.h"            /* generic detecting of OS platform */

/**
 *  This was the version of the RtMidi library from which this
 *  reimplementation was forked.  The RtMidi code is quite difficult to read,
 *  and so we adopted more readable coding and naming conventions.
 *
 *  Unlike the original Sequencer64/Seq66 implement of "rtmidi", this
 *  implementation more nearly aligns with RtMidi, and we force any new MIDI
 *  code to hew as much as possible to "rtl66"/RtMidi operation.  This makes
 *  cross-platform support much easier.
 */

#define RTL66_RTMIDI_VERSION    "5.0.0"     /* RtMidi revision at fork time */
#define RTL66_RTMIDI_PATCHED    "5.0.0"     /* latest RtMidi version patch  */

/*
 * To do: Use this once the Android and the deprecated Windows UWP are
 * provisionally written.
 */

#if 0
#define RTL66_RTMIDI_VERSION "6.0.0"        /* RtMidi revision at fork time */
#endif

/*
 * https://github.com/thestk/rtaudio.git
 */

#define RTL66_RTAUDIO_VERSION   "6.0.0"     /* RtAudio revision at fork     */
#define RTL66_RTAUDIO_PATCHED   "6.0.0"     /* latest RtAudio version patch */

/**
 *  Compiler/linker macros
 */

#if defined RTL66_EXPORT

#if defined PLATFORM_WINDOWS || defined PLATFORM_CYGWIN

#define RTL66_DLL_PUBLIC   __declspec(dllexport)
#define RTL66_API          __declspec(dllexport)

#else

#if PLATFORM_GNUC >= 4
#define RTL66_DLL_PUBLIC   __attribute__((visibility("default")))
#define RTL66_API          __attribute__((visibility("default")))
#else
#define RTL66_API
#define RTL66_DLL_PUBLIC
#endif

#endif

#else                       /* not exporting    */

#define RTL66_API
#define RTL66_DLL_PUBLIC

#endif

/*
 *  Default values for the most common parameters.
 */

#define RTL66_DEFAULT_PPQN      384             /* pulses per quarter note  */
#define RTL66_DEFAULT_BPM       120.0           /* beats per minute         */
#define RTL66_DEFAULT_Q_SIZE    100             /* input queue sie          */

/**
 *  What Platform APIS to build? A few notes:
 *
 *      -   We still need to figure out how to detect the iPhone OS (see the
 *          platform_macros.h header file.)
 *      -   We need a way to get the user to specify that the
 *          RTL66_BUILD_WEB_MIDI macro is defined universally.
 *      -   If no other build macros are found, then RTL66_BUILD_DUMMY is
 *          defined.
 *      -   There are additional macros for the "extra" audio platforms:
 *          PulseAudio, OSS, Windows ASIO, WASAPI, and DS. But note that
 *          MIDI and Audio have their own separate lists of supported APIs.
 */

#undef RTL66_BUILD_ALSA
#undef RTL66_BUILD_DUMMY
#undef RTL66_BUILD_IPHONE_OS
#undef RTL66_BUILD_JACK
#undef RTL66_BUILD_MACOSX_CORE
#undef RTL66_BUILD_OSS
#undef RTL66_BUILD_PIPEWIRE
#undef RTL66_BUILD_PULSEAUDIO
#undef RTL66_BUILD_WEB_MIDI
#undef RTL66_BUILD_WIN_ASIO
#undef RTL66_BUILD_WIN_DS
#undef RTL66_BUILD_WIN_MM
#undef RTL66_BUILD_WIN_WASAPI

/*
 * To do:
 */

#undef RTL66_BUILD_WIN_UWP
#undef RTL66_BUILD_ANDROID

/**
 * API feature macros (future)
 */

#undef RTL66_ALSA_ANNOUNCE_PORT
#undef RTL66_ALSA_AVOID_TIMESTAMPING
#undef RTL66_ALSA_REMOVE_QUEUED_ON_EVENTS
#undef RTL66_JACK_BPMINUTE_CALCULATION
#undef RTL66_JACK_FALLBACK_TO_VIRTUAL_PORT  /* no real need for this one    */
#undef RTL66_JACK_METADATA                  /* RTL66_HAVE_JACK_METADATA_H   */
#undef RTL66_JACK_PORT_CONNECT_CALLBACK
#undef RTL66_JACK_PORT_REFRESH_CALLBACK
#undef RTL66_JACK_PORT_SHUTDOWN_CALLBACK
#undef RTL66_JACK_SESSION
#undef RTL66_JACK_SYNC_CALLBACK
#undef RTL66_JACK_TRANSPORT
#undef RTL66_MIDI_EXTENSIONS
#undef RTL66_USE_MEMORY_LOCK

/**
 *  Default for Windows is to add an identifier to the port names; this
 *  flag can be defined (e.g. in your project file) to disable this behaviour.
 */

#undef RTL66_WIN_MM_NONUNIQUE_PORTNAMES

#if defined PLATFORM_LINUX

/*
 * MIDI
 */

#define RTL66_JACK_TRANSPORT

/*
 * Audio NOT READY
 */

#undef RTL66_BUILD_OSS
#undef RTL66_BUILD_PULSEAUDIO

/*
 * MIDI and Audio
 */

#define RTL66_BUILD_ALSA
#define RTL66_BUILD_JACK
#define PLATFORM_UNIX

/*
 * Not ready:
 *
#define RTL66_BUILD_PIPEWIRE
 */

#define RTL66_MIDI_EXTENSIONS

/*
 * To do:
 *
 * #define RTL66_ALSA_AVOID_TIMESTAMPING
 * #define RTL66_ALSA_REMOVE_QUEUED_ON_EVENTS
 * #define RTL66_BUILD_PIPEWIRE
 * #define RTL66_JACK_BPMINUTE_CALCULATION
 * #define RTL66_JACK_FALLBACK_TO_VIRTUAL_PORT  // no real need for this one
 * #define RTL66_JACK_METADATA
 * #define RTL66_JACK_PORT_CONNECT_CALLBACK
 * #define RTL66_JACK_PORT_REFRESH_CALLBACK
 * #define RTL66_JACK_PORT_SHUTDOWN_CALLBACK
 * #define RTL66_JACK_SESSION
 * #define RTL66_JACK_SYNC_CALLBACK
 * #define RTL66_USE_MEMORY_LOCK
 */

/*
 * We no longer see a need for this port.  This will introduce a minor
 * incompatibility with Seq66's ALSA port numbering.
 *
 *      #define RTL66_ALSA_ANNOUNCE_PORT
 */

#elif defined PLATFORM_MACOSX

#define RTL66_BUILD_MACOSX_CORE

#elif defined PLATFORM_UNIX

#define RTL66_BUILD_JACK
#define RTL66_JACK_TRANSPORT
#undef RTL66_ALLOW_DEPRECATED_JACK_FUNCTIONS

#elif defined PLATFORM_WINDOWS

#define RTL66_BUILD_WIN_MM

#if defined PLATFORM_WINDOWS_UWP // TODO
#undef RTL66_BUILD_WIN_UWP
#endif

#elif defined PLATFORM_IPHONE_OS

#define RTL66_BUILD_MACOSX_CORE
#define RTL66_BUILD_IPHONE_OS

#elif defined PLATFORM_ANDROID  // TODO

#define RTL66_BUILD_ANDROID

#endif

/*
 * ca 2024-06-01
 * Let us always build the dummy code. Might expose some bugs.
 *
 *  #if ! defined RTL66_BUILD_ALSA && \
 *       ! defined RTL66_BUILD_JACK && \
 *       ! defined RTL66_BUILD_PIPEWIRE && \
 *       ! defined RTL66_BUILD_MACOSX_CORE && \
 *       ! defined RTL66_BUILD_WIN_MM && \
 *       ! defined RTL66_BUILD_IPHONE_OS && \
 *       ! defined RTL66_BUILD_WEB_MIDI && \
 *       ! defined RTL66_BUILD_WIN_UWP && \
 *       ! defined RTL66_BUILD_ANDROID
 *  #define RTL66_BUILD_DUMMY
 *  #endif          // list of define tests
 */

#define RTL66_BUILD_DUMMY

#endif          // RTL66_RTL_BUILD_MACROS_H

/*
 * rtl_build_macros.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

