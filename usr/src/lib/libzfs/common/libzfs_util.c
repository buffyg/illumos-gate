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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * Internal utility routines for the ZFS library.
 */

#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mnttab.h>

#include <libzfs.h>

#include "libzfs_impl.h"

int zfs_fd;

void (*error_func)(const char *, va_list);

/*
 * All error handling is kept within libzfs where we have the most information
 * immediately available.  While this may not be suitable for a general purpose
 * library, it greatly simplifies our commands.  This command name is used to
 * prefix all error messages appropriately.
 */
void
zfs_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (error_func != NULL) {
		error_func(fmt, ap);
	} else {
		(void) vfprintf(stderr, fmt, ap);
		(void) fprintf(stderr, "\n");
	}

	va_end(ap);
}

/*
 * An internal error is something that we cannot recover from, and should never
 * happen (such as running out of memory).  It should only be used in
 * exceptional circumstances.
 */
void
zfs_fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (error_func != NULL) {
		error_func(fmt, ap);
	} else {
		(void) vfprintf(stderr, fmt, ap);
		(void) fprintf(stderr, "\n");
	}

	va_end(ap);

	exit(1);
}

/*
 * Consumers (such as the JNI interface) that need to capture error output can
 * override the default error handler using this function.
 */
void
zfs_set_error_handler(void (*func)(const char *, va_list))
{
	error_func = func;
}

/*
 * Display an out of memory error message and abort the current program.
 */
void
no_memory(void)
{
	assert(errno == ENOMEM);
	zfs_fatal(dgettext(TEXT_DOMAIN, "internal error: out of memory\n"));
}

/*
 * A safe form of malloc() which will die if the allocation fails.
 */
void *
zfs_malloc(size_t size)
{
	void *data;

	if ((data = calloc(1, size)) == NULL)
		no_memory();

	return (data);
}

/*
 * A safe form of strdup() which will die if the allocation fails.
 */
char *
zfs_strdup(const char *str)
{
	char *ret;

	if ((ret = strdup(str)) == NULL)
		no_memory();

	return (ret);
}

/*
 * Initialize the library.  Sets the command name used when reporting errors.
 * This command name is used to prefix all error messages appropriately.
 * Also opens /dev/zfs and dies if it cannot be opened.
 */
#pragma init(zfs_init)
void
zfs_init(void)
{
	if ((zfs_fd = open(ZFS_DEV, O_RDWR)) < 0)
		zfs_fatal(dgettext(TEXT_DOMAIN,
		    "internal error: cannot open zfs device"));

	if ((mnttab_file = fopen(MNTTAB, "r")) == NULL)
		zfs_fatal(dgettext(TEXT_DOMAIN, "internal error: unable to "
		    "open %s\n"), MNTTAB);

	sharetab_file = fopen("/etc/dfs/sharetab", "r");
}

/*
 * Cleanup function for library.  Simply close the file descriptors that we
 * opened as part of libzfs_init().
 */
#pragma fini(zfs_fini)
void
zfs_fini(void)
{
	(void) close(zfs_fd);
}

/*
 * Convert a number to an appropriately human-readable output.
 */
void
zfs_nicenum(uint64_t num, char *buf, size_t buflen)
{
	uint64_t n = num;
	int index = 0;
	char u;

	while (n >= 1024) {
		n = (n + (1024 / 2)) / 1024; /* Round up or down */
		index++;
	}

	u = " KMGTPE"[index];

	if (index == 0)
		(void) snprintf(buf, buflen, "%llu", n);
	else if (n < 10 && (num & (num - 1)) != 0)
		(void) snprintf(buf, buflen, "%.2f%c",
		    (double)num / (1ULL << 10 * index), u);
	else if (n < 100 && (num & (num - 1)) != 0)
		(void) snprintf(buf, buflen, "%.1f%c",
		    (double)num / (1ULL << 10 * index), u);
	else
		(void) snprintf(buf, buflen, "%llu%c", n, u);
}
