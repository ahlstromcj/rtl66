#!/bin/bash
#----------------------------------------------------------------------------
##
# \file       	generate-pdf.sh
# \library    	rtl66 doc/tex
# \author     	Chris Ahlstrom
# \date       	2022-07-13
# \update     	2022-07-13
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 	A script to be called by meson.
#
#  However, the best way to make the document is ./work.sh --pdf.
#
#----------------------------------------------------------------------------

INPUT_FILE="$1"
OUTPUT_DIR="$2"

# OUTPUT_FILE="$(dirname $OUTPUT_DIR)/rtl6-dev-manual.pdf"

echo "Building PDF using ..."
echo "   latexmk -g -silent -pdf $INPUT_FILE"
latexmk -g -silent -pdf $INPUT_FILE
cp rtl66-dev-manual.pdf ../pdf/

echo "   cp rtl66-dev-manual.pdf $OUTPUT_DIR"
cp rtl66-dev-manual.pdf $OUTPUT_DIR

#******************************************************************************
# vim: ts=3 sw=3 et ft=sh
#------------------------------------------------------------------------------
