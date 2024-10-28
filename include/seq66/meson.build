#*****************************************************************************
# meson.build (seq66/include)
#-----------------------------------------------------------------------------
##
# \file        include/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2022-07-05
# \updates     2023-01-07
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Moved to the topmost include directory.
#
# libseq66_headers += files(
#    'c_macros.h',
#    'platform_macros.h'
# )
#-----------------------------------------------------------------------------

configure_file(
   input : 'seq66-config.h.in',
   output : 'seq66-config.h',
   configuration : cdata
   )

libseq66_headers += files(
   'midi/seqspec.hpp'
   )

install_headers(libseq66_headers)

#****************************************************************************
# meson.build (seq66/include)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
