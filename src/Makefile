MKPATH=../mk/
include $(MKPATH)buildsys.mk

# import source list
include Makefile.src
INCLUDE = $(HDRS)
OBJECTS = $(ANGFILES) $(CFILES) $(ZFILES)
SRCS    = ${OBJECTS:.o=.c} ${MAINFILES:.o=.c}
PROG    = $(PROGNAME)$(PROG_SUFFIX)
ROOT_PROG = ../$(PROGNAME)$(PROG_SUFFIX)
# VERSION := $(shell ../scripts/version.sh)
ifneq (${VERSION},)
	CFLAGS += -DBUILD_ID=${VERSION}
endif

CFLAGS += -I. -std=c99 -O0 -Wdeclaration-after-statement -fno-omit-frame-pointer

CLEAN = $(ROOT_PROG) froxcomposband.o $(OBJECTS) $(PROGNAME).res
DISTCLEAN = autoconf.h

export CFLAGS LDFLAGS LIBS

all: $(ROOT_PROG)

$(PROG): $(OBJECTS) $(MAINFILES)
	$(CC) -o $@ $(OBJECTS) $(MAINFILES) $(LDFLAGS) $(LDADD) $(LIBS)
	@printf "%10s %-20s\n" LINK $@

$(ROOT_PROG): $(PROG)
	ln -sfn src/$(PROG) $(ROOT_PROG)
	@printf "%10s %-20s\n" LINK $@

# The .res target is forced to x86-64 because an unprefixed windres can
# default to i386 and fail the x64 link, and releases are x64-only. If a
# 32-bit (or other-arch) build is ever taken on, change the target below
# (e.g. -F pe-i386 for 32-bit) or derive it from the compiler instead:
#   $(RC) $< -O coff -F $$($(CC) -dumpmachine 2>/dev/null) -o $@
$(PROGNAME).res: $(PROGNAME).rc
	$(RC) $< -O coff -F pe-x86-64 -o $@

splint:
	splint -f .splintrc ${OBJECTS:.o=.c} main.c main-gcu.c
