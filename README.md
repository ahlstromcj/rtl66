# README for Library Rtl66 0.1.0 2024-10-28

__Rtl66__ is a Audio/MIDI API loosely adapted from the __RtAudio__
and __RtMidi__ projects (https://www.music.mcgill.ca/~gary/rtaudio & rtmidi).
It is a complete refactoring for readability, modularization, and building with
the __Meson__ build system. It also extends this library using library code
adopted from the __Seq66__ application project.

THIS PROJECT IS NOT COMPLETE. CURRENTLY FOR TESTING/REFERENCE ONLY.

It uses the Cfg66 and Xpc66 projects, and the optional Potext project,
via Meson wraps.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

# Major Features

    *   Extension and refactoring of RtMidi and (eventually) RtAudio.
    *   Additional MIDI code to support basic applications.
    *   Code from Seq66 incorporated to implement Seq66-specific features.

##  Library Features

    *   Can be built using GNU C++ or Clang C++.
    *   Basic dependencies: Meson 1.1 and above; C++14 and above.
    *   The build system is Meson, and sample wrap files are provided
        for using Cfg66, Xpc66, and Potext as C++ subprojects.
    *   PDF documentation built from LaTeX.
    *   Code separated into modules to avoid giant cpp/hpp/h files.

##  Code

    *   The code is a mix of hard-core C++ and C-like functions.
    *   The C++ STL and advanced language features are used as much as
    *   possible
    *   C++14 is required for some of its features.
    *   The GNU and Clang C++ compilers are supported.
    *   Broken into modules for easier maintenance.

##  RtMidi

    *   The C++ API is similar, but with different naming conventions.
        (the author finds camelCase to be difficult to read).
    *   The C API is externally identical to the original, but with some
        additional 'extern "C"' functions.
    *   Error-checking has been beefed up.
    *   A ton of clean-up and refactoring.
    *   Additional API to be supported: PipeWire (wait for it).

##  Multiple Builds

    Multiple ways to build the library are provided.

    *   Uses the Meson build system and ninja "make" program.
    *   ALSA and JACK MIDI.
    *   Windows MultiMedia MIDI.
    *   MacOSX Core MIDI (no way to test this, though).
    *   Web MIDI (in progress).
    *   PipeWire (currently a non-working dummy implemention).

##  Additional Features (in progress)

    *   Linux:
        *   Default selection of JACK with fallback to ALSA if needed.
        *   JACK port aliases (JACK-version-dependent).
        *   JACK port refresh (to do).
        *   JACK transport support.
    *   Audio clip support? (not yet sure about this one).

##  Fixes

    *   Improved the work.sh, added an --uninstall option.

##  Documentation

    *   A PDF developers guide is in progress.

## To Do

    *   Beef up testing.
    *   Beef up the LaTeX documentation.

## Recent Changes

    *   Version 0.1.0:
        *   Usage of meson instead of autotools or cmake.
        *   Got the code to build (Linux, maybe Windows) and the test programs
            to work.

// vim: sw=4 ts=4 wm=2 et ft=markdown
