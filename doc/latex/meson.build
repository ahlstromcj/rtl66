#*****************************************************************************
# meson.build (rtl66)
#-----------------------------------------------------------------------------
##
# \file        doc/tex/meson.build
# \library     rtl66
# \author      Chris Ahlstrom
# \date        2022-07-12
# \updates     2022-07-13
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "rtl66" library. See the top-level meson.build
#  file for license information.
#
#  The project root directory is obtained with the following meson syntax:
#
#        meson.source_root()
#
#  The current source directory can be obtained with the following meson
#  syntax:
#
#        meson.current_source_dir()
#
#  But current_source_dir() gives a relative path to the source_root(), so
#  one need to do:
#
#        join_paths(meson.source_root(), meson.current_source_dir())
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Build tool for LaTeX documentation
#-----------------------------------------------------------------------------

latexmk = find_program('latexmk', required : get_option('enable-docs'))
generate_pdf = find_program('generate_pdf.sh')

if not latexmk.found()
   subdir_done()
endif

summary(
   {
      'LaTeX Maker' : latexmk
   },
   section : 'Documentation'
   )

inputdir = join_paths(meson.source_root(), 'doc/latex/tex')
destdir = 'doc'

custom_target('dev manual',
   input : 'rtl66-dev-manual.tex',
   output : destdir,
   command : [ generate_pdf, '@INPUT@', '@OUTPUT@' ],
   install : true,
   install_dir : destdir
   )

run_target('dev manual')

# command : [ latexmk, '-g -silent -pdf rtl66-dev-manual' ],
# destdir = join_paths(meson.source_root(), meson.current_source_dir())
# install : get_option('enable-docs')

#****************************************************************************
# meson.build (doc/tex/rtl66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
