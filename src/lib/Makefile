-include ../config
ifndef TARGET
TARGET=unix	# TODO: Remove dupe.
endif
include ../mk/Makefile.targets

SUBDIRS = bdb simpleio simpleio-stdlib

ifneq (,$(filter $(TARGET), $(COMMODORE_TARGETS)))
SUBDIRS += blit conio directory ingle log posix simpleio-cbm tunix zp lineedit text
endif

ifeq ($(TARGET), vic20)
SUBDIRS += gfx gui keyboard term term-test ultifs ultimem
endif

SUBDIRS += lisp

include ../mk/Makefile.subdir
