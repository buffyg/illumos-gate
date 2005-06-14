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
# ident	"%Z%%M%	%I%	%E% SMI"
#
# lib/libkvm/Makefile.com
#

LIBRARY=	libkvm.a
VERS=		.1

OBJECTS=	kvm.o kvm_getcmd.o

# include library definitions
include ../../Makefile.lib

MAPFILE=	$(MAPDIR)/mapfile
SRCS=		$(OBJECTS:%.o=../common/%.c)

LIBS =		$(DYNLIB) $(LINTLIB)

$(LINTLIB):= SRCS=../common/llib-lkvm

LINTSRC=	$(LINTLIB:%.ln=%)

CFLAGS	+=	$(CCVERBOSE)
DYNFLAGS +=	-M $(MAPFILE)
DYNFLAGS32 +=	-Wl,-f,/usr/platform/\$$PLATFORM/lib/$(DYNLIBPSR)
DYNFLAGS64 +=	-Wl,-f,/usr/platform/\$$PLATFORM/lib/$(MACH64)/$(DYNLIBPSR)
LDLIBS +=	-lelf -lc

CPPFLAGS = -D_KMEMUSER -D_LARGEFILE64_SOURCE=1 -I.. $(CPPFLAGS.master)

CLOBBERFILES += test test.o

.KEEP_STATE:

lint: lintcheck

$(DYNLIB): 	$(MAPFILE)

$(MAPFILE):
	@cd $(MAPDIR); $(MAKE) mapfile

test: ../common/test.c
	$(COMPILE.c) ../common/test.c
	$(LINK.c) -o $@ test.o -lkvm -lelf

# include library targets
include ../../Makefile.targ

objs/%.o pics/%.o: ../common/%.c ../kvm.h
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

# install rule for lint library target
$(ROOTLINTDIR)/%:	../common/%
	$(INS.file)
