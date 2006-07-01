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
# cmd/iscsi/iscsitgtd/Makefile.com

PROG=	iscsitgtd
OBJS	= main.o mgmt.o mgmt_create.o mgmt_list.o mgmt_modify.o mgmt_remove.o
OBJS	+= iscsi_authclient.o iscsi_authglue.o iscsi_cmd.o iscsi_conn.o
OBJS	+= iscsi_crc.o iscsi_ffp.o iscsi_login.o iscsi_sess.o radius.o
OBJS	+= t10_sam.o t10_spc.o t10_sbc.o t10_raw_if.o t10_ssc.o t10_osd.o
OBJS	+= util.o util_err.o util_ifname.o util_port.o util_queue.o
SRCS=	$(OBJS:%.o=../%.c) $(COMMON_SRCS)

include ../../../Makefile.cmd
include $(SRC)/cmd/iscsi/Makefile.iscsi

SUFFIX_LINT	= .ln

CFLAGS +=	$(CCVERBOSE)
CPPFLAGS +=	-D_LARGEFILE64_SOURCE=1 -I$(ISCSICOMMONDIR) -I/usr/include/libxml2
CFLAGS64 +=	$(CCVERBOSE)

GROUP=sys

CLEANFILES += $(OBJS)

.KEEP_STATE:

all: $(PROG)

LDLIBS	+=	-luuid -lxml2 -lsocket -lnsl -ldoor -lavl -lmd5 -ladm -lefi
$(PROG): $(OBJS) $(COMMON_OBJS)
	$(LINK.c) $(OBJS) $(COMMON_OBJS) -o $@ $(LDLIBS)
	$(POST_PROCESS)

lint := LINTFLAGS += -u
lint := LINTFLAGS64 += -u

# lint: lint_SRCS
lint: $(SRCS:../%=%$(SUFFIX_LINT))

%$(SUFFIX_LINT): ../%
	${LINT.c} -I.. ${INCLUDES} -y -c $< && touch $@

%.o:	$(ISCSICOMMONDIR)/%.c
	$(COMPILE.c) $<

%.o:	../%.c
	$(COMPILE.c) $<

clean:
	$(RM) $(CLEANFILES) $(COMMON_OBJS) *$(SUFFIX_LINT)

include ../../../Makefile.targ
