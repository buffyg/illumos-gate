#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident	"%Z%%M%	%I%	%E% SMI"
#


LIBRARY =	libuutil.a
VERS =		.1

OBJECTS = \
	avl.o \
	uu_alloc.o \
	uu_avl.o \
	uu_dprintf.o \
	uu_ident.o \
	uu_list.o \
	uu_misc.o \
	uu_open.o \
	uu_pname.o \
	uu_strtoint.o

include ../../Makefile.lib
include ../../Makefile.rootfs

LIBS =		$(DYNLIB) $(LINTLIB)

$(NOT_NATIVE)NATIVE_BUILD = $(POUND_SIGN)
$(NATIVE_BUILD)VERS =
$(NATIVE_BUILD)LIBS = $(DYNLIB)

SRCS =	\
	../../../common/avl/avl.c \
	../common/uu_alloc.c \
	../common/uu_avl.c \
	../common/uu_dprintf.c \
	../common/uu_ident.c \
	../common/uu_list.c \
	../common/uu_misc.c \
	../common/uu_open.c \
	../common/uu_pname.c \
	../common/uu_strtoint.c

LINTS =		$(OBJECTS:%.o=%.ln)

$(LINTLIB):=	SRCS = $(SRCDIR)/$(LINTSRC)
LDLIBS +=	-lc

SRCDIR =	../common
AVLDIR =	../../../common/avl
MAPDIR =	../spec/$(TRANSMACH)
SPECMAPFILE =	$(MAPDIR)/mapfile

CFLAGS +=	-v
CPPFLAGS +=	-I$(SRCDIR) -I../../common/inc
LINTFLAGS +=	-erroff=E_GLOBAL_COULD_BE_STATIC2
LINTFLAGS64 +=	-erroff=E_GLOBAL_COULD_BE_STATIC2

MY_NATIVE_CPPFLAGS = -DNATIVE_BUILD -I$(SRCDIR)
MY_NATIVE_LDLIBS = -lc

$(NOT_RELEASE_BUILD)CPPFLAGS += -DDEBUG

.KEEP_STATE:

all: $(LIBS)

lint: $(LINTLIB) globallint

globallint: $(LINTS)
	$(LINT.c) $(LINTS) $(LDLIBS)

%.ln: $(SRCDIR)/%.c
	$(LINT.c) -c $<

%.ln: $(AVLDIR)/%.c
	$(LINT.c) -c $<

pics/%.o:	$(AVLDIR)/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

include ../../Makefile.targ
