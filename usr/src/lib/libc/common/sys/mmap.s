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
/*	Copyright (c) 1988 AT&T	*/
/*	All Rights Reserved	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

	.file	"%M%"

#include <sys/asm_linkage.h>

#if !defined(_LARGEFILE_SOURCE)
	ANSI_PRAGMA_WEAK(mmap,function)
#else
	ANSI_PRAGMA_WEAK(mmap64,function)
#endif

#include "SYS.h"
#include <sys/mman.h>		/* Need _MAP_NEW definition     */

#if !defined(_LARGEFILE_SOURCE)

/*
 * C library -- mmap
 * caddr_t mmap(caddr_t addr, size_t len, int prot,
 *	int flags, int fd, off_t off)
 */

	ANSI_PRAGMA_WEAK2(_private_mmap,mmap,function)

	ENTRY(mmap)
#if defined(__sparc)
	/* this depends upon the _MAP_NEW flag being in the top bits */
	sethi	%hi(_MAP_NEW), %g1
	or	%g1, %o3, %o3
#endif
	SYSTRAP_RVAL1(mmap)
	SYSCERROR
	RET
	SET_SIZE(mmap)

#else

/*
 * C library -- mmap64
 * caddr_t mmap64(caddr_t addr, size_t len, int prot,
 *	int flags, int fd, off64_t off)
 */

	ENTRY(mmap64)
#if defined(__sparc)
	/* this depends upon the _MAP_NEW flag being in the top bits */
	sethi	%hi(_MAP_NEW), %g1
	or	%g1, %o3, %o3
#endif
	SYSTRAP_RVAL1(mmap64)
	SYSCERROR
	RET
	SET_SIZE(mmap64)

#endif
