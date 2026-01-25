#!/bin/sh
#
# Additional options:
#
#   --suppressions=contrib/valgrind/seq66.supp
#   --leak-resolution=high

DOVERBOSE=""

if test $# -ge 1 ; then
    if test "$1" = "-v" ; then
        DOVERBOSE="--verbose --show-error-list=yes"
        echo "Adding $DOVERBOSE options."
    fi
fi

valgrind $DOVERBOSE --leak-check=full --track-origins=yes --num-callers=20 \
 --log-file=valgrind.log --show-leak-kinds=all $*

