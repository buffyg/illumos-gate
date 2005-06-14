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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include <errno.h>
#include "messages.h"

#define 	SBUFSZ	256

extern int rmdir();
extern char *prerrno();

static char sptr[SBUFSZ];	/* buffer for system call */

int
rm_files(homedir, user)
char *homedir;			/* home directory to remove */
char *user;
{
	/* delete all files belonging to owner */
	(void) sprintf( sptr,"rm -rf %s", homedir );
	if( system(sptr) != 0 ) {
		errmsg( M_RMFILES );
		return( EX_HOMEDIR );
	}

	return( EX_SUCCESS );
}

