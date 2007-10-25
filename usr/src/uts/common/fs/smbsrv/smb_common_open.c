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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * This module provides the common open functionality to the various
 * open and create SMB interface functions.
 */

#include <smbsrv/smb_incl.h>
#include <smbsrv/smb_fsops.h>
#include <smbsrv/mlsvc.h>
#include <smbsrv/nterror.h>
#include <smbsrv/ntstatus.h>
#include <smbsrv/smbinfo.h>
#include <sys/fcntl.h>

extern uint32_t smb_is_executable(char *path);

#define	DENY_READ(share_access) ((share_access & FILE_SHARE_READ) == 0)

#define	DENY_WRITE(share_access) ((share_access & FILE_SHARE_WRITE) == 0)

#define	DENY_DELETE(share_access) ((share_access & FILE_SHARE_DELETE) == 0)

#define	DENY_RW(share_access) \
	((share_access & (FILE_SHARE_READ | FILE_SHARE_WRITE)) == 0)

#define	DENY_ALL(share_access) (share_access == 0)

#define	DENY_NONE(share_access) (share_access == FILE_SHARE_ALL)

/*
 * The default stability mode is to perform the write-through
 * behaviour requested by the client.
 */
int smb_stable_mode = 0;


/*
 * This macro is used to delete a newly created object
 * if any error happens after creation of object.
 */
#define	SMB_DEL_NEWOBJ(obj) \
	if (created) {							\
		if (is_dir)						\
			(void) smb_fsop_rmdir(sr, sr->user_cr,		\
			    obj.dir_snode, obj.last_comp, 0);		\
		else							\
			(void) smb_fsop_remove(sr, sr->user_cr,		\
			    obj.dir_snode, obj.last_comp, 0);		\
	}

/*
 * smb_set_stability
 *
 * Set the default stability mode. Normal (mode is zero) means perform
 * the write-through behaviour requested by the client. Synchronous
 * (mode is non-zero) means journal everything regardless of the write
 * through behaviour requested by the client.
 */
void
smb_set_stability(int mode)
{
	smb_stable_mode = mode;
}

/*
 * smb_access_generic_to_file
 *
 * Search MSDN for IoCreateFile to see following mapping.
 *
 * GENERIC_READ		STANDARD_RIGHTS_READ, FILE_READ_DATA,
 *			FILE_READ_ATTRIBUTES and FILE_READ_EA
 *
 * GENERIC_WRITE	STANDARD_RIGHTS_WRITE, FILE_WRITE_DATA,
 *               FILE_WRITE_ATTRIBUTES, FILE_WRITE_EA, and FILE_APPEND_DATA
 *
 * GENERIC_EXECUTE	STANDARD_RIGHTS_EXECUTE, SYNCHRONIZE, and FILE_EXECUTE.
 */
uint32_t
smb_access_generic_to_file(uint32_t desired_access)
{
	uint32_t access = 0;

	if (desired_access & GENERIC_ALL)
		return (FILE_ALL_ACCESS & ~SYNCHRONIZE);

	if (desired_access & GENERIC_EXECUTE) {
		desired_access &= ~GENERIC_EXECUTE;
		access |= (STANDARD_RIGHTS_EXECUTE |
		    SYNCHRONIZE | FILE_EXECUTE);
	}

	if (desired_access & GENERIC_WRITE) {
		desired_access &= ~GENERIC_WRITE;
		access |= (FILE_GENERIC_WRITE & ~SYNCHRONIZE);
	}

	if (desired_access & GENERIC_READ) {
		desired_access &= ~GENERIC_READ;
		access |= FILE_GENERIC_READ;
	}

	return (access | desired_access);
}

/*
 * smb_omode_to_amask
 *
 * This function converts open modes used by Open and Open AndX
 * commands to desired access bits used by NT Create AndX command.
 */
uint32_t
smb_omode_to_amask(uint32_t desired_access)
{
	switch (desired_access & SMB_DA_ACCESS_MASK) {
	case SMB_DA_ACCESS_READ:
		return (FILE_GENERIC_READ);

	case SMB_DA_ACCESS_WRITE:
		return (FILE_GENERIC_WRITE);

	case SMB_DA_ACCESS_READ_WRITE:
		return (FILE_GENERIC_READ | FILE_GENERIC_WRITE);

	case SMB_DA_ACCESS_EXECUTE:
		return (FILE_GENERIC_EXECUTE);
	}

	/* invalid open mode */
	return ((uint32_t)SMB_INVALID_AMASK);
}

/*
 * smb_denymode_to_sharemode
 *
 * This function converts deny modes used by Open and Open AndX
 * commands to share access bits used by NT Create AndX command.
 */
uint32_t
smb_denymode_to_sharemode(uint32_t desired_access, char *fname)
{
	switch (desired_access & SMB_DA_SHARE_MASK) {
	case SMB_DA_SHARE_COMPATIBILITY:
		if (smb_is_executable(fname))
			return (FILE_SHARE_READ | FILE_SHARE_WRITE);
		else {
			if ((desired_access &
			    SMB_DA_ACCESS_MASK) == SMB_DA_ACCESS_READ)
				return (FILE_SHARE_READ);
			else
				return (FILE_SHARE_NONE);
		}

	case SMB_DA_SHARE_EXCLUSIVE:
		return (FILE_SHARE_NONE);

	case SMB_DA_SHARE_DENY_WRITE:
		return (FILE_SHARE_READ);

	case SMB_DA_SHARE_DENY_READ:
		return (FILE_SHARE_WRITE);

	case SMB_DA_SHARE_DENY_NONE:
		return (FILE_SHARE_READ | FILE_SHARE_WRITE);
	}

	/* invalid deny mode */
	return ((uint32_t)SMB_INVALID_SHAREMODE);
}

/*
 * smb_ofun_to_crdisposition
 *
 * This function converts open function values used by Open and Open AndX
 * commands to create disposition values used by NT Create AndX command.
 */
uint32_t
smb_ofun_to_crdisposition(uint16_t  ofun)
{
	static int ofun_cr_map[3][2] =
	{
		{ -1,			FILE_CREATE },
		{ FILE_OPEN,		FILE_OPEN_IF },
		{ FILE_OVERWRITE,	FILE_OVERWRITE_IF }
	};

	int row = ofun & SMB_OFUN_OPEN_MASK;
	int col = (ofun & SMB_OFUN_CREATE_MASK) >> 4;

	if (row == 3)
		return ((uint32_t)SMB_INVALID_CRDISPOSITION);

	return (ofun_cr_map[row][col]);
}

/*
 * smb_open_share_check
 *
 * check file sharing rules for current open request
 * against the given existing open.
 *
 * Returns NT_STATUS_SHARING_VIOLATION if there is any
 * sharing conflict, otherwise returns NT_STATUS_SUCCESS.
 */
uint32_t
smb_open_share_check(struct smb_request *sr,
    struct smb_node *node,
    struct smb_ofile *open)
{
	uint32_t desired_access;
	uint32_t share_access;

	desired_access = sr->arg.open.desired_access;
	share_access   = sr->arg.open.share_access;

	/*
	 * As far as I can tell share modes are not relevant to
	 * directories. The check for exclusive access (Deny RW)
	 * remains because I don't know whether or not it was here
	 * for a reason.
	 */
	if (node->attr.sa_vattr.va_type == VDIR) {
		if (DENY_RW(open->f_share_access) &&
		    (node->n_orig_uid != crgetuid(sr->user_cr))) {
			return (NT_STATUS_SHARING_VIOLATION);
		}

		return (NT_STATUS_SUCCESS);
	}

	/* if it's just meta data */
	if ((open->f_granted_access & FILE_DATA_ALL) == 0)
		return (NT_STATUS_SUCCESS);

	/*
	 * Check requested share access against the
	 * open granted (desired) access
	 */
	if (DENY_DELETE(share_access) && (open->f_granted_access & DELETE))
		return (NT_STATUS_SHARING_VIOLATION);

	if (DENY_READ(share_access) &&
	    (open->f_granted_access & (FILE_READ_DATA | FILE_EXECUTE)))
		return (NT_STATUS_SHARING_VIOLATION);

	if (DENY_WRITE(share_access) &&
	    (open->f_granted_access & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
		return (NT_STATUS_SHARING_VIOLATION);

	/* check requested desired access against the open share access */
	if (DENY_DELETE(open->f_share_access) && (desired_access & DELETE))
		return (NT_STATUS_SHARING_VIOLATION);

	if (DENY_READ(open->f_share_access) &&
	    (desired_access & (FILE_READ_DATA | FILE_EXECUTE)))
		return (NT_STATUS_SHARING_VIOLATION);

	if (DENY_WRITE(open->f_share_access) &&
	    (desired_access & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
		return (NT_STATUS_SHARING_VIOLATION);

	return (NT_STATUS_SUCCESS);
}

/*
 * smb_file_share_check
 *
 * check file sharing rules for current open request
 * against all existing opens for a file.
 *
 * Returns NT_STATUS_SHARING_VIOLATION if there is any
 * sharing conflict, otherwise returns NT_STATUS_SUCCESS.
 */
uint32_t
smb_file_share_check(struct smb_request *sr, struct smb_node *node)
{
	struct smb_ofile *open;
	uint32_t status;

	if (node == 0 || node->n_refcnt <= 1)
		return (NT_STATUS_SUCCESS);

	/* if it's just meta data */
	if ((sr->arg.open.desired_access & FILE_DATA_ALL) == 0)
		return (NT_STATUS_SUCCESS);

	smb_llist_enter(&node->n_ofile_list, RW_READER);
	open = smb_llist_head(&node->n_ofile_list);
	while (open) {
		status = smb_open_share_check(sr, node, open);
		if (status == NT_STATUS_SHARING_VIOLATION) {
			smb_llist_exit(&node->n_ofile_list);
			return (status);
		}
		open = smb_llist_next(&node->n_ofile_list, open);
	}
	smb_llist_exit(&node->n_ofile_list);

	return (NT_STATUS_SUCCESS);
}

/*
 * smb_amask_to_amode
 * Converts specific read/write access rights of access mask to access
 * mode flags.
 */
int
smb_amask_to_amode(unsigned long amask)
{
	if ((amask & FILE_READ_DATA) &&
	    (amask & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
		return (O_RDWR);

	if (amask & (FILE_WRITE_DATA | FILE_APPEND_DATA))
		return (O_WRONLY);

	return (O_RDONLY);
}

/*
 * smb_open_subr
 *
 * Notes on write-through behaviour. It looks like pre-LM0.12 versions
 * of the protocol specify the write-through mode when a file is opened,
 * (SmbOpen, SmbOpenAndX) so the write calls (SmbWrite, SmbWriteAndClose,
 * SmbWriteAndUnlock) don't need to contain a write-through flag.
 *
 * With LM0.12, the open calls (SmbCreateAndX, SmbNtTransactCreate)
 * don't indicate which write-through mode to use. Instead the write
 * calls (SmbWriteAndX, SmbWriteRaw) specify the mode on a per call
 * basis.
 *
 * We don't care which open call was used to get us here, we just need
 * to ensure that the write-through mode flag is copied from the open
 * parameters to the node. We test the omode write-through flag in all
 * write functions.
 *
 * This function will return NT status codes but it also raises errors,
 * in which case it won't return to the caller. Be careful how you
 * handle things in here.
 */
uint32_t
smb_open_subr(struct smb_request *sr)
{
	int			created = 0;
	struct smb_node		*node = 0;
	struct smb_node		*dnode = 0;
	struct smb_node		*cur_node;
	struct open_param	*op = &sr->arg.open;
	int			rc;
	struct smb_ofile	*of;
	smb_attr_t		new_attr;
	int			pathlen;
	int			max_requested = 0;
	uint32_t		max_allowed;
	unsigned int		granted_oplock;
	uint32_t		status = NT_STATUS_SUCCESS;
	int			is_dir;
	smb_error_t		err;
	int			is_stream;
	int			lookup_flags = SMB_FOLLOW_LINKS;
	uint32_t		daccess;

	is_dir = (op->create_options & FILE_DIRECTORY_FILE) ? 1 : 0;

	if (is_dir) {
		/*
		 * The file being created or opened is a directory file.
		 * With this flag, the Disposition parameter must be set to
		 * one of FILE_CREATE, FILE_OPEN, or FILE_OPEN_IF
		 */
		if ((op->create_disposition != FILE_CREATE) &&
		    (op->create_disposition != FILE_OPEN_IF) &&
		    (op->create_disposition != FILE_OPEN)) {
			smbsr_raise_cifs_error(sr, NT_STATUS_INVALID_PARAMETER,
			    ERRDOS, ERROR_INVALID_ACCESS);
			/* invalid open mode */
			/* NOTREACHED */
		}
	}

	if (op->desired_access & MAXIMUM_ALLOWED) {
		max_requested = 1;
		op->desired_access &= ~MAXIMUM_ALLOWED;
	}
	op->desired_access = smb_access_generic_to_file(op->desired_access);

	if (sr->session->s_file_cnt >= SMB_SESSION_OFILE_MAX) {

		ASSERT(sr->uid_user);
		cmn_err(CE_NOTE, "smbd[%s\\%s]: %s", sr->uid_user->u_domain,
		    sr->uid_user->u_name,
		    xlate_nt_status(NT_STATUS_TOO_MANY_OPENED_FILES));

		smbsr_raise_cifs_error(sr, NT_STATUS_TOO_MANY_OPENED_FILES,
		    ERRDOS, ERROR_TOO_MANY_OPEN_FILES);
		/* NOTREACHED */
	}

	/* This must be NULL at this point */
	sr->fid_ofile = NULL;

	op->devstate = 0;

	switch (sr->tid_tree->t_res_type & STYPE_MASK) {
	case STYPE_DISKTREE:
		break;

	case STYPE_IPC:
		/*
		 * No further processing for IPC, we need to either
		 * raise an exception or return success here.
		 */
		if ((rc = smb_rpc_open(sr)) != 0) {
			smbsr_raise_nt_error(sr, rc);
			/* NOTREACHED */
		} else {
			return (NT_STATUS_SUCCESS);
		}
		break;

	default:
		smbsr_raise_error(sr, ERRSRV, ERRinvdevice);
		/* NOTREACHED */
		break;
	}

	if ((pathlen = strlen(op->fqi.path)) >= MAXPATHLEN) {
		smbsr_raise_error(sr, ERRSRV, ERRfilespecs);
		/* NOTREACHED */
	}

	/*
	 * Some clients pass null file names; NT interprets this as "\".
	 */
	if (pathlen == 0) {
		op->fqi.path = "\\";
		pathlen = 1;
	}

	op->fqi.srch_attr = op->fqi.srch_attr;

	if ((status = smb_validate_object_name(op->fqi.path, is_dir)) != 0) {
		smbsr_raise_cifs_error(sr, status, ERRDOS, ERROR_INVALID_NAME);
		/* NOTREACHED */
	}

	cur_node = op->fqi.dir_snode ?
	    op->fqi.dir_snode : sr->tid_tree->t_snode;

	if (rc = smb_pathname_reduce(sr, sr->user_cr, op->fqi.path,
	    sr->tid_tree->t_snode, cur_node, &op->fqi.dir_snode,
	    op->fqi.last_comp)) {
		smbsr_raise_errno(sr, rc);
		/* NOTREACHED */
	}

	/*
	 * If the access mask has only DELETE set (ignore
	 * FILE_READ_ATTRIBUTES), then assume that this
	 * is a request to delete the link (if a link)
	 * and do not follow links.  Otherwise, follow
	 * the link to the target.
	 */

	daccess = op->desired_access & ~FILE_READ_ATTRIBUTES;

	if (daccess == DELETE)
		lookup_flags &= ~SMB_FOLLOW_LINKS;

	rc = smb_fsop_lookup_name(sr, kcred, lookup_flags,
	    sr->tid_tree->t_snode, op->fqi.dir_snode, op->fqi.last_comp,
	    &op->fqi.last_snode, &op->fqi.last_attr);

	if (rc == 0) {
		op->fqi.last_comp_was_found = 1;
		(void) strcpy(op->fqi.last_comp_od,
		    op->fqi.last_snode->od_name);
	} else if (rc == ENOENT) {
		op->fqi.last_comp_was_found = 0;
		op->fqi.last_snode = NULL;
		rc = 0;
	} else {
		smb_node_release(op->fqi.dir_snode);
		SMB_NULL_FQI_NODES(op->fqi);
		smbsr_raise_errno(sr, rc);
		/* NOTREACHED */
	}

	if (op->fqi.last_comp_was_found) {
		node = op->fqi.last_snode;
		dnode = op->fqi.dir_snode;

		/*
		 * Reject this request if the target is a directory
		 * and the client has specified that it must not be
		 * a directory (required by Lotus Notes).
		 */
		if ((op->create_options & FILE_NON_DIRECTORY_FILE) &&
		    (op->fqi.last_attr.sa_vattr.va_type == VDIR)) {
			smb_node_release(node);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);
			smbsr_raise_cifs_error(sr,
			    NT_STATUS_FILE_IS_A_DIRECTORY,
			    ERRDOS, ERROR_ACCESS_DENIED);
			/* NOTREACHED */
		}

		if (op->fqi.last_attr.sa_vattr.va_type == VDIR) {
			if ((sr->smb_com == SMB_COM_OPEN_ANDX) ||
			    (sr->smb_com == SMB_COM_OPEN)) {
				/*
				 * Directories cannot be opened
				 * with the above commands
				 */
				smb_node_release(node);
				smb_node_release(dnode);
				SMB_NULL_FQI_NODES(op->fqi);
				smbsr_raise_cifs_error(sr,
				    NT_STATUS_FILE_IS_A_DIRECTORY,
				    ERRDOS, ERROR_ACCESS_DENIED);
				/* NOTREACHED */
			}
		} else if (op->my_flags & MYF_MUST_BE_DIRECTORY) {
			smb_node_release(node);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);
			smbsr_raise_cifs_error(sr, NT_STATUS_NOT_A_DIRECTORY,
			    ERRDOS, ERROR_DIRECTORY);
			/* NOTREACHED */
		}

		/*
		 * No more open should be accepted when "Delete on close"
		 * flag is set.
		 */
		if (node->flags & NODE_FLAGS_DELETE_ON_CLOSE) {
			smb_node_release(node);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);
			smbsr_raise_cifs_error(sr, NT_STATUS_DELETE_PENDING,
			    ERRDOS, ERROR_ACCESS_DENIED);
			/* NOTREACHED */
		}

		/*
		 * Specified file already exists so the operation should fail.
		 */
		if (op->create_disposition == FILE_CREATE) {
			smb_node_release(node);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);
			smbsr_raise_cifs_error(sr,
			    NT_STATUS_OBJECT_NAME_COLLISION, ERRDOS,
			    ERROR_ALREADY_EXISTS);
			/* NOTREACHED */
		}

		/*
		 * Windows seems to check read-only access before file
		 * sharing check.
		 */
		if (NODE_IS_READONLY(node)) {
			/* Files data only */
			if (node->attr.sa_vattr.va_type != VDIR) {
				if (op->desired_access & (FILE_WRITE_DATA |
				    FILE_APPEND_DATA)) {
					smb_node_release(node);
					smb_node_release(dnode);
					SMB_NULL_FQI_NODES(op->fqi);
					smbsr_raise_error(sr, ERRDOS,
					    ERRnoaccess);
					/* NOTREACHED */
				}
			}
		}

		status = smb_file_share_check(sr, node);
		if (status == NT_STATUS_SHARING_VIOLATION) {
			smb_node_release(node);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);
			return (status);
		}

		status = smb_fsop_access(sr, sr->user_cr, node,
		    op->desired_access);

		if (status != NT_STATUS_SUCCESS) {
			smb_node_release(node);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);
			if (status == NT_STATUS_PRIVILEGE_NOT_HELD) {
				smbsr_raise_cifs_error(sr,
				    status,
				    ERRDOS,
				    ERROR_PRIVILEGE_NOT_HELD);
			} else {
				smbsr_raise_cifs_error(sr,
				    NT_STATUS_ACCESS_DENIED,
				    ERRDOS,
				    ERROR_ACCESS_DENIED);
			}
		}

		/*
		 * Break the oplock before share checks. If another client
		 * has the file open, this will force a flush or close,
		 * which may affect the outcome of any share checking.
		 */
		if (OPLOCKS_IN_FORCE(node)) {
			status = smb_break_oplock(sr, node);

			if (status != NT_STATUS_SUCCESS) {
				smb_node_release(node);
				smb_node_release(dnode);
				SMB_NULL_FQI_NODES(op->fqi);
				smbsr_raise_cifs_error(sr, status,
				    ERRDOS, ERROR_VC_DISCONNECTED);
				/* NOTREACHED */
			}
		}

		switch (op->create_disposition) {
		case FILE_SUPERSEDE:
		case FILE_OVERWRITE_IF:
		case FILE_OVERWRITE:
			if (node->attr.sa_vattr.va_type == VDIR) {
				smb_node_release(node);
				smb_node_release(dnode);
				SMB_NULL_FQI_NODES(op->fqi);
				smbsr_raise_cifs_error(sr,
				    NT_STATUS_ACCESS_DENIED, ERRDOS,
				    ERROR_ACCESS_DENIED);
				/* NOTREACHED */
			}

			if (node->attr.sa_vattr.va_size != op->dsize) {
				node->flags &= ~NODE_FLAGS_SET_SIZE;
				new_attr.sa_vattr.va_size = op->dsize;
				new_attr.sa_mask = SMB_AT_SIZE;
				if ((rc = smb_fsop_setattr(sr, sr->user_cr,
				    (&op->fqi)->last_snode, &new_attr,
				    &op->fqi.last_attr)) != 0) {
					smb_node_release(node);
					smb_node_release(dnode);
					SMB_NULL_FQI_NODES(op->fqi);
					smbsr_raise_errno(sr, rc);
					/* NOTREACHED */
				}

			}

			/*
			 * If file is being replaced,
			 * we should remove existing streams
			 */
			if (SMB_IS_STREAM(node) == 0)
				(void) smb_fsop_remove_streams(sr, sr->user_cr,
				    node);

			op->action_taken = SMB_OACT_TRUNCATED;
			break;

		default:
			/*
			 * FILE_OPEN or FILE_OPEN_IF.
			 */
			op->action_taken = SMB_OACT_OPENED;
			break;
		}
	} else {

		/* Last component was not found. */
		dnode = op->fqi.dir_snode;

		if ((op->create_disposition == FILE_OPEN) ||
		    (op->create_disposition == FILE_OVERWRITE)) {
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);

			is_stream = smb_stream_parse_name(op->fqi.path,
			    NULL, NULL);
			/*
			 * The requested file not found so the operation should
			 * fail with these two dispositions
			 */
			if (is_stream)
				smbsr_raise_cifs_error(sr,
				    NT_STATUS_OBJECT_NAME_NOT_FOUND,
				    ERRDOS, ERROR_FILE_NOT_FOUND);
			else
				smbsr_raise_error(sr, ERRDOS, ERRbadfile);
			/* NOTREACHED */
		}

		/*
		 * lock the parent dir node in case another create
		 * request to the same parent directory comes in.
		 */
		smb_rwx_rwenter(&dnode->n_lock, RW_WRITER);

		bzero(&new_attr, sizeof (new_attr));
		if (is_dir == 0) {
			new_attr.sa_vattr.va_type = VREG;
			new_attr.sa_vattr.va_mode = 0666;
			new_attr.sa_mask = SMB_AT_TYPE | SMB_AT_MODE;
			rc = smb_fsop_create(sr, sr->user_cr, dnode,
			    op->fqi.last_comp, &new_attr,
			    &op->fqi.last_snode, &op->fqi.last_attr);
			if (rc != 0) {
				smb_rwx_rwexit(&dnode->n_lock);
				smb_node_release(dnode);
				SMB_NULL_FQI_NODES(op->fqi);
				smbsr_raise_errno(sr, rc);
				/* NOTREACHED */
			}

			if (op->dsize) {
				new_attr.sa_vattr.va_size = op->dsize;
				new_attr.sa_mask = SMB_AT_SIZE;
				rc = smb_fsop_setattr(sr, sr->user_cr,
				    op->fqi.last_snode, &new_attr,
				    &op->fqi.last_attr);
				if (rc != 0) {
					smb_node_release(op->fqi.last_snode);
					(void) smb_fsop_remove(sr, sr->user_cr,
					    dnode, op->fqi.last_comp, 0);
					smb_rwx_rwexit(&dnode->n_lock);
					smb_node_release(dnode);
					SMB_NULL_FQI_NODES(op->fqi);
					smbsr_raise_errno(sr, rc);
					/* NOTREACHED */
				}
			}

		} else {
			op->dattr |= SMB_FA_DIRECTORY;
			new_attr.sa_vattr.va_type = VDIR;
			new_attr.sa_vattr.va_mode = 0777;
			new_attr.sa_mask = SMB_AT_TYPE | SMB_AT_MODE;
			rc = smb_fsop_mkdir(sr, sr->user_cr, dnode,
			    op->fqi.last_comp, &new_attr,
			    &op->fqi.last_snode, &op->fqi.last_attr);
			if (rc != 0) {
				smb_rwx_rwexit(&dnode->n_lock);
				smb_node_release(dnode);
				SMB_NULL_FQI_NODES(op->fqi);
				smbsr_raise_errno(sr, rc);
				/* NOTREACHED */
			}
		}

		created = 1;
		op->action_taken = SMB_OACT_CREATED;
	}

	if (node == 0) {
		node = op->fqi.last_snode;
	}

	if ((op->fqi.last_attr.sa_vattr.va_type != VREG) &&
	    (op->fqi.last_attr.sa_vattr.va_type != VDIR) &&
	    (op->fqi.last_attr.sa_vattr.va_type != VLNK)) {
		/* not allowed to do this */
		SMB_DEL_NEWOBJ(op->fqi);
		smb_node_release(node);
		if (created)
			smb_rwx_rwexit(&dnode->n_lock);
		smb_node_release(dnode);
		SMB_NULL_FQI_NODES(op->fqi);
		smbsr_raise_error(sr, ERRDOS, ERRnoaccess);
		/* NOTREACHED */
	}

	if (max_requested) {
		smb_fsop_eaccess(sr, sr->user_cr, node, &max_allowed);
		op->desired_access |= max_allowed;
	}

	/*
	 * smb_ofile_open() will copy node to of->node.  Hence
	 * the hold on node (i.e. op->fqi.last_snode) will be "transferred"
	 * to the "of" structure.
	 */

	of = smb_ofile_open(sr->tid_tree, node, sr->smb_pid, op->desired_access,
	    op->create_options, op->share_access, SMB_FTYPE_DISK, NULL, 0,
	    &err);

	if (of == NULL) {
		SMB_DEL_NEWOBJ(op->fqi);
		smb_node_release(node);
		if (created)
			smb_rwx_rwexit(&dnode->n_lock);
		smb_node_release(dnode);
		SMB_NULL_FQI_NODES(op->fqi);
		smbsr_raise_cifs_error(sr, err.status, err.errcls, err.errcode);
		/* NOTREACHED */
	}

	/*
	 * Propagate the write-through mode from the open params
	 * to the node: see the notes in the function header.
	 *
	 * IR #102318 Mirroring may force synchronous
	 * writes regardless of what we specify here.
	 */
	if (smb_stable_mode || (op->create_options & FILE_WRITE_THROUGH))
		node->flags |= NODE_FLAGS_WRITE_THROUGH;

	op->fileid = op->fqi.last_attr.sa_vattr.va_nodeid;

	if (op->fqi.last_attr.sa_vattr.va_type == VDIR) {
		/* We don't oplock directories */
		op->my_flags &= ~MYF_OPLOCK_MASK;
		op->dsize = 0;
	} else {
		status = smb_acquire_oplock(sr, of, op->my_flags,
		    &granted_oplock);
		op->my_flags &= ~MYF_OPLOCK_MASK;

		if (status != NT_STATUS_SUCCESS) {
			(void) smb_ofile_close(of, 0);
			smb_ofile_release(of);
			if (created)
				smb_rwx_rwexit(&dnode->n_lock);
			smb_node_release(dnode);
			SMB_NULL_FQI_NODES(op->fqi);

			smbsr_raise_cifs_error(sr, status,
			    ERRDOS, ERROR_SHARING_VIOLATION);
			/* NOTREACHED */
		}

		op->my_flags |= granted_oplock;
		op->dsize = op->fqi.last_attr.sa_vattr.va_size;
	}

	if (created) {
		node->flags |= NODE_FLAGS_CREATED;
		/*
		 * Clients may set the DOS readonly bit on create but they
		 * expect subsequent write operations on the open fid to
		 * succeed.  Thus the DOS readonly bit is not set until the
		 * file is closed.  The NODE_CREATED_READONLY flag will
		 * inhibit other attempts to open the file with write access
		 * and act as the indicator to set the DOS readonly bit on
		 * close.
		 */
		if (op->dattr & SMB_FA_READONLY) {
			node->flags |= NODE_CREATED_READONLY;
			op->dattr &= ~SMB_FA_READONLY;
		}
		smb_node_set_dosattr(node, op->dattr | SMB_FA_ARCHIVE);
		if (op->utime.tv_sec == 0 || op->utime.tv_sec == 0xffffffff)
			(void) microtime(&op->utime);
		smb_node_set_time(node, NULL, &op->utime, 0, 0, SMB_AT_MTIME);
		(void) smb_sync_fsattr(sr, sr->user_cr, node);
	} else {
		/*
		 * If we reach here, it means that file already exists
		 * and if create disposition is one of: FILE_SUPERSEDE,
		 * FILE_OVERWRITE_IF, or FILE_OVERWRITE it
		 * means that client wants to overwrite (or truncate)
		 * the existing file. So we should overwrite the dos
		 * attributes of destination file with the dos attributes
		 * of source file.
		 */

		switch (op->create_disposition) {
		case FILE_SUPERSEDE:
		case FILE_OVERWRITE_IF:
		case FILE_OVERWRITE:
			smb_node_set_dosattr(node,
			    op->dattr | SMB_FA_ARCHIVE);
			(void) smb_sync_fsattr(sr, sr->user_cr, node);
		}
		op->utime = *smb_node_get_crtime(node);
		op->dattr = smb_node_get_dosattr(node);
	}

	/*
	 * Set up the file type in open_param for the response
	 */
	op->ftype = SMB_FTYPE_DISK;
	sr->smb_fid = of->f_fid;
	sr->fid_ofile = of;

	if (created) {
		smb_rwx_rwexit(&dnode->n_lock);
	}
	smb_node_release(dnode);
	SMB_NULL_FQI_NODES(op->fqi);

	return (NT_STATUS_SUCCESS);
}

/*
 * smb_validate_object_name
 *
 * Very basic file name validation. Directory validation is handed off
 * to smb_validate_dirname. For filenames, we check for names of the
 * form "AAAn:". Names that contain three characters, a single digit
 * and a colon (:) are reserved as DOS device names, i.e. "COM1:".
 *
 * Returns NT status codes.
 */
uint32_t
smb_validate_object_name(char *path, unsigned int ftype)
{
	char *filename;

	if (path == 0)
		return (0);

	if (ftype)
		return (smb_validate_dirname(path));

	/*
	 * Basename with backslashes.
	 */
	if ((filename = strrchr(path, '\\')) != 0)
		++filename;
	else
		filename = path;

	if (strlen(filename) == 5 &&
	    mts_isdigit(filename[3]) &&
	    filename[4] == ':') {
		return (NT_STATUS_OBJECT_NAME_INVALID);
	}

	return (0);
}

/*
 * smb_preset_delete_on_close
 *
 * Set the DeleteOnClose flag on the smb file. When the file is closed,
 * the flag will be transferred to the smb node, which will commit the
 * delete operation and inhibit subsequent open requests.
 *
 * When DeleteOnClose is set on an smb_node, the common open code will
 * reject subsequent open requests for the file. Observation of Windows
 * 2000 indicates that subsequent opens should be allowed (assuming
 * there would be no sharing violation) until the file is closed using
 * the fid on which the DeleteOnClose was requested.
 */
void
smb_preset_delete_on_close(smb_ofile_t *file)
{
	mutex_enter(&file->f_mutex);
	file->f_flags |= SMB_OFLAGS_SET_DELETE_ON_CLOSE;
	mutex_exit(&file->f_mutex);
}
