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

#include <sys/zfs_context.h>
#include <sys/uberblock_impl.h>
#include <sys/vdev_impl.h>

/* Keep the uberblock version in a varialbe so we can get at it with mdb */
static uint64_t uberblock_version = UBERBLOCK_VERSION;

int
uberblock_verify(uberblock_t *ub)
{
	if (ub->ub_magic == BSWAP_64((uint64_t)UBERBLOCK_MAGIC))
		byteswap_uint64_array(ub, sizeof (uberblock_t));

	if (ub->ub_magic != UBERBLOCK_MAGIC)
		return (EINVAL);

	if (ub->ub_version != UBERBLOCK_VERSION)
		return (ENOTSUP);

	return (0);
}

/*
 * Update the uberblock and return a boolean value indicating whether
 * anything changed in this transaction group.
 */
int
uberblock_update(uberblock_t *ub, vdev_t *rvd, uint64_t txg)
{
	ASSERT(ub->ub_txg < txg);

	ub->ub_magic = UBERBLOCK_MAGIC;
	ub->ub_version = UBERBLOCK_VERSION;
	ub->ub_txg = txg;
	ub->ub_guid_sum = rvd->vdev_guid_sum;
	ub->ub_timestamp = gethrestime_sec();

	return (ub->ub_rootbp.blk_birth == txg);
}
