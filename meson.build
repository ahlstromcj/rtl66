#*****************************************************************************
# meson.build (rtl66)
#-----------------------------------------------------------------------------
##
# \file        meson.build
# \library     rtl66
# \author      Chris Ahlstrom
# \date        2022-06-07
# \updates     2024-04-28
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the rtl66 library.
#
#  The rtl66 library is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  The rtl66 library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with the rtl66 library. If not, see <https://www.gnu.org/licenses/>.
#
#  See rtl66/meson.build for more information.
#
#  The rtl66 library is a heavy refactoring of RtMidi 5.0.0, meant to replace
#  the seq_rtmidi library in version 2 of Seq66.
#
#  For information on meson subprojects, see
#
#     https://mesonbuild.com/Subprojects.html
#
#  Also see the "Pitchfork" project layout conventions.
#
#     https://github.com/vector-of-bool/pitchfork
#
#   cxx-pflR1
#   The Pitchfork Layout (PFL)
#   A Collection of Interesting Ideas, 2022-06-04
#
#   https://api.csswg.org/bikeshed/?force=1&url=
#       https://raw.githubusercontent.com/vector-of-bool/
#           pitchfork/develop/data/spec.bs
#
#  Note that we prefer to employ "3.1.2 Separate Header Placement".
#
# meson_version:
#
#     On mesonbuild.com: 1.4
#     Installed via pip: 1.4
#
#     If errors occur in running this build, then consider replacing your
#     distro's old meson by removing it and running:
#
#        # pip install meson
#
#     Requires version >= 1.1.0 which enables "preserve_paths" and usage of
#     the "meson.options" file.
#
# Windows:
#
#     Installed Git Bash, QtCreator with MingW usage as per Seq66, Python 3
#     from an installer (added the feature that allows > 260 character path
#     names), and Meson 1.4 from an MSI installer.  Did not install the
#     Visual Studio Community edition on our Windows virtual machine.
#
# Setup:
#
#     See the "work" bash script.
#
# Release:
#
#     See the description of the 'buildtype' option in contrib/meson.text.
#     Specify "--buildtype=release" on the meson command line.
#
# Reconfigure:
#
#        $ meson configure -Dprefix=/home/me/tmp -Dbuildtype=release build/
#
# Installation:
#
#     The library is build/libXXX66-0.1.a. We will install the XXX66 libraries
#     all in in /usr/lib/rtl66-0.1 or /usr/local/lib/rtl66-0.1, rather
#     than a subdirectory for each of these somewhat related libraries..
#
# Also see:
#
#     "Porting poppler to meson"
#     https://fasterthanli.me/series/dont-shell-out/part-5
#
#     "Getting started with Meson in C++ (part 2)"
#     https://germandiagogomez.medium.com/
#        getting-started-with-meson-in-c-part-2-58150354ff17
#
# Note that C++14 is required, in order to support the usage of the
# std::make_unique<>() function.
#
#-----------------------------------------------------------------------------

project('rtl66',
   [ 'c', 'cpp' ],
   version : '0.1.0',
   license : 'GPLv3',
   meson_version : '>=1.1.0',
   default_options : [
      'c_std=c11', 'cpp_std=c++14', 'warning_level=3',
      'default_library=both'
   ]
   )

#-----------------------------------------------------------------------------
# Conditional enabling of the Potext translation library.
#-----------------------------------------------------------------------------

rtl66_use_potext = false
if get_option('potext')

   rtl66_use_potext = true
   add_project_arguments('-DRTL66_USE_POTEXT', language : [ 'c', 'cpp' ])

endif

#-----------------------------------------------------------------------------
# Information for this sub-project.
#-----------------------------------------------------------------------------

rtl66_pkg_description = 'Refactoring/extension of the RtMidi library for Seq66v2.'
rtl66_pkg_version = meson.project_version()
rtl66_api_version = '0.1'
rtl66_info_date = '2024-04-28'
rtl66_info_build_type = get_option('buildtype')
rtl66_info_build_root = meson.project_build_root()
rtl66_info_lib_type = get_option('default_library')
rtl66_info_project_root = meson.project_source_root()
rtl66_debug = get_option('buildtype').contains('debug')
rtl66_project_base = 'rtl66-@0@'.format(rtl66_api_version)
rtl66_pkgconfig_name = 'librtl66'
rtl66_subproj = meson.is_subproject()

if rtl66_debug
   if not rtl66_subproj
      add_project_arguments('-DDEBUG', language : 'cpp')
      add_project_arguments('-fno-inline-functions', language : 'cpp')
   endif
endif

#-----------------------------------------------------------------------------
# Easy access to directory options.
#-----------------------------------------------------------------------------

rtl66_dir = 'rtl66-@0@'.format(rtl66_api_version)
rtl66_prefix = get_option('prefix')
rtl66_bindir = join_paths(rtl66_prefix, get_option('bindir'))
rtl66_includedir = join_paths(rtl66_prefix, get_option('includedir'), rtl66_dir)
rtl66_libdir = join_paths(rtl66_prefix, get_option('libdir'), rtl66_dir)
rtl66_datadir = join_paths(rtl66_prefix, get_option('datadir'), rtl66_dir)
rtl66_docdir = join_paths(rtl66_prefix, rtl66_datadir, 'doc', rtl66_dir)
libraries_top = meson.project_source_root()
librtl66_include_top = join_paths(libraries_top, 'include')

#-----------------------------------------------------------------------------
# The *.pc file goes into $libdir/rtl66-0.1/pkgconfig instead of
# $libdir/pkgconfig as the meson documentation asserts.  Not sure why, but
# wiring the expected directory here works.
#-----------------------------------------------------------------------------

alt_pkgconfig_libdir = join_paths(
   rtl66_prefix, get_option('libdir'), 'pkgconfig'
   )

#-----------------------------------------------------------------------------
# librtl66_headers provides the list of all headers in the project. These are
# currently all installable, public headers. They are set in
# include/meson.build.
#
# librtl66_sources provides the source-code files. They are set in
# src/meson.build.
#-----------------------------------------------------------------------------

librtl66_headers = []
librtl66_sources = []
build_args = []

#-----------------------------------------------------------------------------
# Unlike the cfg66 library, this library will have platform-dependent
# characteristics.
#-----------------------------------------------------------------------------

os_unix     = false
os_freebsd  = false
os_linux    = false
os_win32    = false
os_darwin   = false

if host_machine.system() == 'darwin'
   os_darwin = true
elif host_machine.system() == 'windows'
   os_win32 = true
elif host_machine.system() == 'linux'
   os_linux = true
elif host_machine.system() == 'freebsd'
   os_freebsd = true
endif

os_unix = not os_win32

#-----------------------------------------------------------------------------
# Each include file directory is added to the list passed to the compiler.
# We prefer to use the style '#include "cfg/header.hpp".
#-----------------------------------------------------------------------------

librtl66_includes = include_directories('.',    # librtl66_include_top,
   'include',
   'include/midi',
   'include/rtl',
   'include/transport'
   )

#-----------------------------------------------------------------------------
# Used by the rtl66_xxx_version() function.
#-----------------------------------------------------------------------------

build_args += [
   '-DRTL66_NAME=' + '"' + meson.project_name() + '"',
   '-DRTL66_VERSION=' + '"' + meson.project_version() + '"'
   ]

#-----------------------------------------------------------------------------
# Only make public interfaces visible
#-----------------------------------------------------------------------------

'''
if target_machine.system() == 'windows' or target_machine.system() == 'cygwin'
   build_args += '-DLIBPUBLIC="__declspec(dllexport)"'
else
   build_args += '-DLIBPUBLIC=__attribute__((visibility("default")))'
endif
'''

#-----------------------------------------------------------------------------
# Set up some "HAVE" macros.  By default the header file is generated in the
# "build" directory. If configure_file() is moved to include/meson.build,
# the the header goes into "build/include", which is what we want.
#-----------------------------------------------------------------------------

cc = meson.get_compiler('cpp')
cdata = configuration_data()
cdata.set('fcntl_h', cc.has_header('fcntl.h'))
cdata.set('limits_h', cc.has_header('limits.h'))
cdata.set('semaphore_h', cc.has_header('semaphore.h'))
cdata.set('jack_metadata_h', cc.has_header('jack/metadata.h'))
cdata.set('jack_port_rename',
   cc.has_header_symbol('jack/jack.h', 'jack_port_rename'))
cdata.set('jack_get_version_string',
   cc.has_header_symbol('jack/jack.h', 'jack_get_version_string'))

#-----------------------------------------------------------------------------
# Potential sub-projects
#-----------------------------------------------------------------------------

if rtl66_use_potext

   libpotext_library_dep = dependency(
      'libpotext', fallback : [ 'libpotext', 'libpotext_dep' ]
      )

endif

libcfg66_library_dep = dependency(
   'libcfg66', fallback : [ 'libcfg66', 'libcfg66_dep' ]
   )
libxpc66_library_dep = dependency(
   'libxpc66', fallback : [ 'libxpc66', 'libxpc66_dep' ]
   )

#-----------------------------------------------------------------------------
# Sub-directories
#-----------------------------------------------------------------------------

subdir('include')
subdir('src')

#-----------------------------------------------------------------------------
# Dependencies on Linux
#-----------------------------------------------------------------------------
#
# ALSA:
#     The libsound version on Ubuntu 20.04 is something like 1.2.2. On
#     Ubuntua 18, 1.1.3. The current version in configure.ac of Seq66 is
#     0.9.0. Let's go with version 1.0.0.
#
# JACK:
#     The libjack-jackd2-0 version on Ubuntu 20 is something like 1.9.20.
#     On Ubuntua 18, 1.9.12. The current version in configure.ac of Seq66
#     is 0.90.0.
#
# pthreads:
#     The 'threads' dependency is required, but currently we don't care
#     about the version.
#
# Later:
#
#  if host_machine.system() == 'linux'
#     . . .
#  endif
#
#-----------------------------------------------------------------------------

if os_linux

alsa_dep = dependency(
   'alsa', version : '>=1.0.0', required : get_option('enable-alsa')
   )

jack_dep = dependency(
   'jack', version : '>=1.9.0', required : get_option('enable-jack')
   )

threads_dep = dependency('threads', required : true)

system_depends = [ alsa_dep, jack_dep, threads_dep ]

endif

if os_freebsd

alsa_dep = dependency(
   'alsa', version : '>=1.0.0', required : get_option('enable-alsa')
   )

jack_dep = dependency(
   'jack', version : '>=1.9.0', required : get_option('enable-jack')
   )

threads_dep = dependency('threads', required : true)

system_depends = [ alsa_dep, jack_dep, threads_dep ]

endif

if os_win32

# Yields a pkg-config error
# winmm_dep = dependency('winmm')

   winmm_dep = cc.find_library('winmm')
   system_depends = [ winmm_dep ]

endif

#-----------------------------------------------------------------------------
# We recommemd using a recent version of meson by installing it outside the
# Linux distro's repository.  For example, meson on Ubuntu is pretty damn
# old.
#
# The choice of library type is of some import for debugging:
#
#     library:          Defaults to a shared library.
#     shared_library:   Cannot see source files until stepped into.
#     static_library:   Can see the source files when debugging.
#     both_libraries:   Product both static and shared libraries.
#
# This one causes the linker to not see the library functions, wotta pain:
#
#     gnu_symbol_visibility: 'hidden'
#
# Also see the https://gcc.gnu.org/wiki/Visibility site.
#
#-----------------------------------------------------------------------------

if rtl66_use_potext

   rtl66_library_build = library(
      rtl66_project_base,
      librtl66_sources,
      install : not rtl66_subproj,
      install_dir : rtl66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends, libcfg66_library_dep, libxpc66_library_dep,
         libpotext_library_dep
         ],
      include_directories : [ librtl66_includes ]
      )

else

   rtl66_library_build = library(
      rtl66_project_base,
      librtl66_sources,
      install : not rtl66_subproj,
      install_dir : rtl66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends, libcfg66_library_dep, libxpc66_library_dep
         ],
      include_directories : [ librtl66_includes ]
      )

endif

#-----------------------------------------------------------------------------
# Making the internal library usable as a Meson subproject requires using
# declare_dependency().
#-----------------------------------------------------------------------------

rtl66_dep = declare_dependency(
   include_directories : [ librtl66_includes ],
   link_with : [ rtl66_library_build ]
   )

#-----------------------------------------------------------------------------
# Make this library usable from the system's package manager.
#-----------------------------------------------------------------------------

install_headers(librtl66_headers, subdir : rtl66_dir, preserve_path : true)

#-----------------------------------------------------------------------------
# Package config
#-----------------------------------------------------------------------------
#  On Arch Linux, the .pc file is installed to /usr/local/lib/pkgconfig.
#  On Ubuntu, the .pc file is installed to
#     /usr/local/lib/x86_64-linux-gnu/pkgconfig.
#-----------------------------------------------------------------------------

if not rtl66_subproj

   pkg_mod = import('pkgconfig')
   pkg_mod.generate(
      rtl66_library_build,
      filebase : rtl66_pkgconfig_name,
      version : rtl66_pkg_version,
      name : rtl66_project_base,
      description : rtl66_pkg_description,
      install_dir : alt_pkgconfig_libdir,
      subdirs : rtl66_dir,
      libraries : rtl66_library_build,
      url : 'https://github.com/ahlstromcj/rtl66',
      )

# TODO: figure out where to put and install the logo

   install_data(
      files(
         'ChangeLog',
         'LICENSE.md',
         'README.md',
         'doc/rtl66-dev-manual.pdf'
         ),
      install_dir : get_option('docdir')
      )

   if get_option('enable-tests')
      subdir('tests')
   endif

   if get_option('enable-docs')
      subdir('doc')
   endif

endif

#-----------------------------------------------------------------------------
# Info
#-----------------------------------------------------------------------------

summary(
   {
      'Date' : rtl66_info_date,
      'Filebase' : rtl66_project_base,
      'Version' : rtl66_pkg_version,
      'Base' : rtl66_project_base,
      'Description' : rtl66_pkg_description,
      'Source Root' : rtl66_info_project_root,
      'Headers Root' : librtl66_include_top,
      'Sub Dir' : rtl66_project_base,
      'Build Type' : rtl66_info_build_type,
      'Build Root' : rtl66_info_build_root,
      'Library Type' : rtl66_info_lib_type,
      'Subproject' : rtl66_subproj
   },
   section : 'Project'
   )

summary(
   {
      'prefix' : rtl66_prefix,
      'includedir' : rtl66_includedir,
      'bindir' : rtl66_bindir,
      'libdir' : rtl66_libdir,
      'datadir' : rtl66_datadir,
      'appdatadir' : rtl66_datadir
   },
   section : 'Rtl66 Install Directories'
   )

#****************************************************************************
# meson.build (rtl66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------

