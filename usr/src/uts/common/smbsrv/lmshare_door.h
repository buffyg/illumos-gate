/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SMBSRV_LMSHARE_DOOR_H
#define	_SMBSRV_LMSHARE_DOOR_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <smbsrv/smbinfo.h>
#include <smbsrv/smb_common_door.h>
#include <smbsrv/smbinfo.h>

/*
 * Door interface for CIFS share management.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define	LMSHR_DOOR_NAME		"/var/run/smb_lmshare_door"
#define	LMSHR_DOOR_VERSION	1

#define	LMSHR_DOOR_COOKIE	((void*)(0xdeadbeef^LMSHR_DOOR_VERSION))
#define	LMSHR_DOOR_SIZE		(65 * 1024)

/*
 * Door interface
 *
 * Define door operations
 */
#define	LMSHR_DOOR_NUM_SHARES		1
#define	LMSHR_DOOR_DELETE		2
#define	LMSHR_DOOR_RENAME		3
#define	LMSHR_DOOR_GETINFO		4
#define	LMSHR_DOOR_ADD			5
#define	LMSHR_DOOR_SETINFO		6
#define	LMSHR_DOOR_EXISTS		7
#define	LMSHR_DOOR_IS_SPECIAL		8
#define	LMSHR_DOOR_IS_RESTRICTED	9
#define	LMSHR_DOOR_IS_ADMIN		10
#define	LMSHR_DOOR_IS_VALID		11
#define	LMSHR_DOOR_IS_DIR		12
#define	LMSHR_DOOR_LIST			13
#define	LMSHR_DOOR_ENUM			14

/*
 * Door server status
 *
 * LMSHR_DOOR_ERROR is returned by the door server if there is problem
 * with marshalling/unmarshalling. Otherwise, LMSHR_DOOR_SUCCESS is
 * returned.
 *
 */
#define	LMSHR_DOOR_SRV_SUCCESS		0
#define	LMSHR_DOOR_SRV_ERROR		-1

/*
 * struct door_request {
 * 	int		req_type;
 *	<parameters>
 *	};
 *
 * struct door_response {
 * 	int		door_srv_status;
 *	<response>
 *	};
 */

void smb_dr_get_lmshare(smb_dr_ctx_t *, lmshare_info_t *);
void smb_dr_put_lmshare(smb_dr_ctx_t *, lmshare_info_t *);

void smb_dr_get_lmshr_list(smb_dr_ctx_t *, lmshare_list_t *);
void smb_dr_put_lmshr_list(smb_dr_ctx_t *, lmshare_list_t *);

void lmshrd_door_close(void);

#ifdef __cplusplus
}
#endif

#endif /* _SMBSRV_LMSHARE_DOOR_H */
