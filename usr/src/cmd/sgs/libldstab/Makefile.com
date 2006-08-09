#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
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
# Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

LIBRARY=	libldstab.a
VERS=		.1

COMOBJS=	stab.o
DUPOBJS+=
BLTOBJ=		msg.o

OBJECTS=	$(BLTOBJ) $(COMOBJS) $(DUPOBJS)

include		$(SRC)/lib/Makefile.lib
include		$(SRC)/cmd/sgs/Makefile.com

SRCDIR =	../common
SRCBASE=	../../../..

LDLIBS +=	$(CONVLIBDIR) $(CONV_LIB) $(ELFLIBDIR) -lelf -lc
DYNFLAGS +=	$(VERSREF)

LINTFLAGS +=	-erroff=E_NAME_DECL_NOT_USED_DEF2 \
		-erroff=E_NAME_DEF_NOT_USED2 \
		-erroff=E_NAME_USED_NOT_DEF2
LINTFLAGS64 +=	-erroff=E_NAME_DECL_NOT_USED_DEF2 \
		-erroff=E_NAME_DEF_NOT_USED2 \
		-erroff=E_NAME_USED_NOT_DEF2


# A bug in pmake causes redundancy when '+=' is conditionally assigned, so
# '=' is used with extra variables.
# $(DYNLIB) :=	DYNFLAGS += -Yl,$(SGSPROTO)
#
XXXFLAGS=
$(DYNLIB) :=	XXXFLAGS= $(USE_PROTO)
DYNFLAGS +=	$(XXXFLAGS)

BLTDEFS=	msg.h
BLTDATA=	msg.c
BLTMESG=	$(SGSMSGDIR)/libldstab

BLTFILES=	$(BLTDEFS) $(BLTDATA) $(BLTMESG)

SGSMSGCOM=	../common/libldstab.msg
SGSMSGALL=	$(SGSMSGCOM)
SGSMSGTARG=	$(SGSMSGCOM)
SGSMSGFLAGS +=	-h $(BLTDEFS) -d $(BLTDATA) -m $(BLTMESG) -n libldstab_msg

SRCS=		$(COMOBJS:%.o=../common/%.c) $(BLTDATA)
LINTSRCS=	$(SRCS)

CLEANFILES +=	$(LINTOUTS) $(BLTFILES)
CLOBBERFILES +=	$(DYNLIB)

ROOTDYNLIB=	$(DYNLIB:%=$(ROOTLIBDIR)/%)
