Moderator's README
==================

This directory contains the base source
code for the smallC compiler (actually
three versions:  the 8080, 6809 and vax
code generators are here also.)

The "includes" directory contains
headers which are intended to be
included in user programs - the place
where these files reside should be set
in the Makefile as INCDIR.  The
directories "6809", "8080", and "vax"
contain runtime support for the
respective compilers.  The directory
"lib" contains the source code for some
common C library functions (portable
ones).
