/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 *	Copyright (c) 1988 AT&T
 *	  All Rights Reserved
 *
 *
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include	<sys/types.h>
#include	<sys/mman.h>
#include	<signal.h>
#include	<dlfcn.h>
#include	<synch.h>
#include	"_rtld.h"

/*
 * Declarations of global variables used in ld.so.
 */
Rt_lock		rtldlock;

/*
 * Major link-map lists.
 */
Lm_list		lml_main =	{ 0 };		/* the `main's link map list */
Lm_list		lml_rtld =	{ 0 };		/* rtld's link map list */

/*
 * BEGIN: Exposed to rtld_db, don't change without a coordinated handshake with
 * librtld_db (remembering that librtld_db must be able to read old as well as
 * current core files).
 */
List		dynlm_list =	{ 0, 0 };	/* dynamic list of link-maps */
/*
 * END: Exposed to rtld_db
 */

Reglist *	reglist = 0;			/* list of register symbols */

void		(*thrinit)() = 0;		/* thread initialization */

ulong_t		hwcap = 0;			/* hardware capabilities */
ulong_t		sfcap = 0;			/* software capabilities */

/*
 * Initialized fmap structure.
 */
static Fmap	_fmap = { 0, 0, 0, 0, 0 };
Fmap *		fmap = &_fmap;			/* initial file mapping info */

/*
 * Set of integers to track how many of what type of
 * PLT's have been bound.  This is only really interesting
 * for SPARC since ia32 basically just has the one PLT.
 */
uint32_t	pltcnt21d = 0;
uint32_t	pltcnt24d = 0;
uint32_t	pltcntu32 = 0;
uint32_t	pltcntu44 = 0;
uint32_t	pltcntfull = 0;
uint32_t	pltcntfar = 0;

/*
 * Enable technology (via status flags for RTLD) dependent upon whether we're
 * in a patch or major release build environment.
 */
uint_t		rtld_flags =
#ifdef	EXPAND_RELATIVE
			RT_FL_RELATIVE |
#endif
#ifdef	SIEBEL_DISABLE
			RT_FL_DISFIX_1 |
#endif
			RT_FL_NOCONCUR;
uint_t		rtld_flags2 = 0;

/*
 * Various other global data.
 */
const char	*pr_name;
const char	*rt_name;		/* the run time linkers name */
char		*lasterr = (char *)0;	/* string describing last error */
					/*	cleared by each dlerror() */
Interp		*interp = 0;		/* ELF interpreter info */
List		hdl_list[HDLIST_SZ+2];	/* dlopen() handle list */
size_t		syspagsz = 0;		/* system page size */
unsigned long	at_flags = 0;		/* machine specific file flags */
char		*platform = 0;		/* platform name from AT_SUN_PLATFORM */
size_t		platform_sz = 0;	/* platform string length */
Uts_desc	*uts;			/* utsname descriptor */
Isa_desc	*isa;			/* isalist descriptor */

uint_t		audit_argcnt = 64;	/* no. of stack args to copy (default */
					/*	is all) */
Audit_desc	*auditors = 0;		/* global auditors (LD_AUDIT) */

const char	*locale = 0;		/* locale environment definition */

const char	*rpl_audit = 0;		/* replaceable LD_AUDIT string */
const char	*rpl_debug = 0;		/* replaceable LD_DEBUG string */
const char	*rpl_ldflags = 0;	/* replaceable LD_FLAGS string */
const char	*rpl_libpath = 0;	/* replaceable LD_LIBRARY_PATH string */
Pnode		*rpl_libdirs = 0;	/*	and associated Pnode list */
const char	*rpl_preload = 0;	/* replaceable LD_PRELOAD string */

const char	*prm_audit = 0;		/* permanent LD_AUDIT string */
const char	*prm_debug = 0;		/* permanent LD_DEBUG string */
const char	*prm_ldflags = 0;	/* permanent LD_FLAGS string */
const char	*prm_libpath = 0;	/* permanent LD_LIBRARY_PATH string */
Pnode		*prm_libdirs = 0;	/*	and associated Pnode list */
const char	*prm_preload = 0;	/* permanent LD_PRELOAD string */

uint_t		env_info = 0;		/* information regarding environment */
					/*	variables */
int		killsig = SIGKILL;	/* signal sent on fatal exit */
uint_t		dbg_mask;		/* debugging classes */
const char	*dbg_file = 0;		/* debugging directed to file */

#pragma weak	environ = _environ	/* environ for PLT tracing - we */
char		**_environ = 0;		/* supply the pair to satisfy any */
					/* libc requirements (hwmuldiv) */

const char	*profile_name;		/* object being profiled */
const char	*profile_out;		/* profile output file */
const char	*profile_lib;		/* audit library to perform profile */

unsigned char	search_rules[] = {	/* dependency search rules */
		RPLENV,			/*	replaceable LD_LIBRARY_PATH */
		PRMENV,			/*	permanent LD_LIBRARY_PATH */
		RUNPATH,		/*	callers runpath */
		DEFAULT,		/*	default library path */
		0
};

/*
 * Frequently used messages are cached here to reduce _dgettext() overhead and
 * also provide for resetting should the locale change (see _ld_libc()).
 */
const char	*err_strs[ERR_NUM] = { 0 };
const char	*nosym_str = 0;


/*
 * Rejection error message tables.
 */
const Msg
ldd_reject[] = {
		MSG_STR_EMPTY,
		MSG_LDD_REJ_MACH,	/* MSG_INTL(MSG_LDD_REJ_MACH) */
		MSG_LDD_REJ_CLASS,	/* MSG_INTL(MSG_LDD_REJ_CLASS) */
		MSG_LDD_REJ_DATA,	/* MSG_INTL(MSG_LDD_REJ_DATA) */
		MSG_LDD_REJ_TYPE,	/* MSG_INTL(MSG_LDD_REJ_TYPE) */
		MSG_LDD_REJ_BADFLAG,	/* MSG_INTL(MSG_LDD_REJ_BADFLAG) */
		MSG_LDD_REJ_MISFLAG,	/* MSG_INTL(MSG_LDD_REJ_MISFLAG) */
		MSG_LDD_REJ_VERSION,	/* MSG_INTL(MSG_LDD_REJ_VERSION) */
		MSG_LDD_REJ_HAL,	/* MSG_INTL(MSG_LDD_REJ_HAL) */
		MSG_LDD_REJ_US3,	/* MSG_INTL(MSG_LDD_REJ_US3) */
		MSG_LDD_REJ_STR,	/* MSG_INTL(MSG_LDD_REJ_STR) */
		MSG_LDD_REJ_UNKFILE,	/* MSG_INTL(MSG_LDD_REJ_UNKFILE) */
		MSG_LDD_REJ_HWCAP_1,	/* MSG_INTL(MSG_LDD_REJ_HWCAP_1) */
	};


const Msg
err_reject[] = {
		MSG_STR_EMPTY,
		MSG_ERR_REJ_MACH,	/* MSG_INTL(MSG_ERR_REJ_MACH) */
		MSG_ERR_REJ_CLASS,	/* MSG_INTL(MSG_ERR_REJ_CLASS) */
		MSG_ERR_REJ_DATA,	/* MSG_INTL(MSG_ERR_REJ_DATA) */
		MSG_ERR_REJ_TYPE,	/* MSG_INTL(MSG_ERR_REJ_TYPE) */
		MSG_ERR_REJ_BADFLAG,	/* MSG_INTL(MSG_ERR_REJ_BADFLAG) */
		MSG_ERR_REJ_MISFLAG,	/* MSG_INTL(MSG_ERR_REJ_MISFLAG) */
		MSG_ERR_REJ_VERSION,	/* MSG_INTL(MSG_ERR_REJ_VERSION) */
		MSG_ERR_REJ_HAL,	/* MSG_INTL(MSG_ERR_REJ_HAL) */
		MSG_ERR_REJ_US3,	/* MSG_INTL(MSG_ERR_REJ_US3) */
		MSG_ERR_REJ_STR,	/* MSG_INTL(MSG_ERR_REJ_STR) */
		MSG_ERR_REJ_UNKFILE,	/* MSG_INTL(MSG_ERR_REJ_UNKFILE) */
		MSG_ERR_REJ_HWCAP_1,	/* MSG_INTL(MSG_ERR_REJ_HWCAP_1) */
	};
