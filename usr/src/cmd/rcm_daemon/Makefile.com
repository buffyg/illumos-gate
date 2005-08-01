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
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

include ../../Makefile.cmd

COMMON = ../common
CMDINETCOMMON = $(SRC)/cmd/cmd-inet/common

RCM_SRC = \
	$(COMMON)/rcm_event.c \
	$(COMMON)/rcm_main.c \
	$(COMMON)/rcm_impl.c \
	$(COMMON)/rcm_subr.c \
	$(COMMON)/rcm_lock.c \
	$(COMMON)/rcm_script.c 

RCM_OBJ = \
	rcm_event.o \
	rcm_main.o \
	rcm_impl.o \
	rcm_subr.o \
	rcm_lock.o \
	rcm_script.o

COMMON_MOD_SRC = \
	$(COMMON)/filesys_rcm.c \
	$(COMMON)/dump_rcm.c \
	$(COMMON)/swap_rcm.c \
	$(COMMON)/network_rcm.c \
	$(COMMON)/ip_rcm.c \
	$(COMMON)/cluster_rcm.c \
	$(COMMON)/pool_rcm.c \
	$(COMMON)/mpxio_rcm.c \
	$(COMMON)/ip_anon_rcm.c \
	$(COMMON)/svm_rcm.c

sparc_MOD_SRC = $(COMMON)/ttymux_rcm.c

COMMON_MOD_OBJ = \
	filesys_rcm.o \
	dump_rcm.o \
	swap_rcm.o \
	network_rcm.o \
	ip_rcm.o \
	cluster_rcm.o \
	pool_rcm.o \
	mpxio_rcm.o \
	ip_anon_rcm.o \
	svm_rcm.o

sparc_MOD_OBJ = ttymux_rcm.o

RCM_DAEMON = rcm_daemon

COMMON_RCM_MODS = \
	SUNW_filesys_rcm.so \
	SUNW_dump_rcm.so \
	SUNW_swap_rcm.so \
	SUNW_network_rcm.so \
	SUNW_ip_rcm.so \
	SUNW_cluster_rcm.so \
	SUNW_pool_rcm.so \
	SUNW_mpxio_rcm.so \
	SUNW_ip_anon_rcm.so \
	SUNW_svm_rcm.so

sparc_RCM_MODS = SUNW_ttymux_rcm.so

RCM_DIR = rcm
MOD_DIR = modules
SCRIPT_DIR = scripts

CLOBBERFILES = $(COMMON_RCM_MODS) $($(MACH)_RCM_MODS) $(RCM_DAEMON)

LINT_MODULES = $(COMMON_MOD_SRC:.c=.ln) $($(MACH)_MOD_SRC:.c=.ln)

CPPFLAGS += -I.. -I $(CMDINETCOMMON)
CPPFLAGS += -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT
CFLAGS += $(CCVERBOSE) $(C_PICFLAGS)

LINTFLAGS += -u -erroff=E_FUNC_ARG_UNUSED

LDLIBS_MODULES = 
SUNW_pool_rcm.so := LDLIBS_MODULES += -L$(ROOT)/usr/lib -lpool
SUNW_svm_rcm.so	:= LDLIBS_MODULES += -L$(ROOT)/usr/lib -lmeta
SUNW_network_rcm.so := LDLIBS_MODULES += -L$(ROOT)/lib -llaadm

LDLIBS += -lgen -lelf -lcmd -lrcm -lnvpair -ldevinfo -lnsl -lsocket -lrt

SRCS = $(RCM_SRC) $(COMMON_MOD_SRC)

POFILES = $(SRCS:.c=.po)
POFILE = prcm_daemon.po

# install specifics

ROOTLIB_RCM = $(ROOTLIB)/$(RCM_DIR)
ROOTLIB_RCM_MOD = $(ROOTLIB_RCM)/$(MOD_DIR)
ROOTLIB_RCM_DAEMON = $(RCM_DAEMON:%=$(ROOTLIB_RCM)/%)
ROOTLIB_RCM_MODULES = $(COMMON_RCM_MODS:%=$(ROOTLIB_RCM_MOD)/%) \
			$($(MACH)_RCM_MODS:%=$(ROOTLIB_RCM_MOD)/%)
ROOTLIB_RCM_SCRIPT = $(ROOTLIB_RCM)/$(SCRIPT_DIR)
ROOTETC_RCM = $(ROOTETC)/$(RCM_DIR)
ROOTETC_RCM_SCRIPT = $(ROOTETC_RCM)/$(SCRIPT_DIR)

all :=		TARGET= all
install :=	TARGET= install
clean :=	TARGET= clean
clobber :=	TARGET= clobber
lint :=		TARGET= lint

$(ROOTETC_RCM) :=		GROUP = sys
$(ROOTETC_RCM_SCRIPT) :=	GROUP = sys

.KEEP_STATE:

all: $(RCM_DAEMON) $(COMMON_RCM_MODS) $($(MACH)_RCM_MODS)

install: all			\
	$(ROOTLIB_RCM)		\
	$(ROOTLIB_RCM_DAEMON)	\
	$(ROOTLIB_RCM_MOD)	\
	$(ROOTLIB_RCM_MODULES)	\
	$(ROOTLIB_RCM_SCRIPT)	\
	$(ROOTETC_RCM)		\
	$(ROOTETC_RCM_SCRIPT)

clean:
	$(RM) $(RCM_OBJ) $(COMMON_MOD_OBJ) $($(MACH)_MOD_OBJ) $(POFILES)

lint: $(RCM_DAEMON).ln $(LINT_MODULES)

$(RCM_DAEMON).ln: FRC
	$(LINT.c) $(RCM_SRC) $(LDLIBS)

%.ln: FRC
	$(LINT.c) $(RCM_SRC) $(@:.ln=.c) $(LDLIBS)

FRC:

include ../../Makefile.targ

$(POFILE):      $(POFILES)
	$(RM) $@; cat $(POFILES) > $@

$(RCM_DAEMON): $(RCM_OBJ)
	$(LINK.c) -o $@ $< $(RCM_OBJ) $(LDLIBS)
	$(POST_PROCESS)

SUNW_%.so: %.o
	$(LINK.c) -o $@ $(GSHARED) -h $@ $< $(LDLIBS_MODULES)

%.o: $(COMMON)/%.c
	$(COMPILE.c) -o $@ $<

$(ROOTLIB_RCM):
	$(INS.dir)

$(ROOTLIB_RCM)/%: %
	$(INS.file)

$(ROOTLIB_RCM_MOD):
	$(INS.dir)

$(ROOTLIB_RCM_MOD)/%: %
	$(INS.file)

$(ROOTLIB_RCM_SCRIPT):
	$(INS.dir)

$(ROOTETC_RCM):
	$(INS.dir)

$(ROOTETC_RCM_SCRIPT):
	$(INS.dir)
