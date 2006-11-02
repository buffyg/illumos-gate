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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * basic declarations for implementation of the share management
 * libraries.
 */

#ifndef _LIBSHARE_IMPL_H
#define	_LIBSHARE_IMPL_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <libshare.h>
#include <libscf.h>
#include <scfutil.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* directory to find plugin modules in */
#define	SA_LIB_DIR	"/usr/lib/fs"

/* default group name for dfstab file */
#define	SA_DEFAULT_FILE_GRP	"sys"

typedef void *sa_phandle_t;

#define	SA_PLUGIN_VERSION	1
struct sa_plugin_ops {
	int	sa_version;
	char	*sa_protocol;			/* protocol name */
	int	(*sa_init)();
	void	(*sa_fini)();
	int	(*sa_share)(sa_share_t);	/* start sharing */
	int	(*sa_unshare)(char *);	/* stop sharing */
	int	(*sa_valid_prop)(sa_property_t, sa_optionset_t);
	int	(*sa_valid_space)(char *);	/* is name valid optionspace? */
	int	(*sa_security_prop)(char *);	/* property is security */
	int	(*sa_legacy_opts)(sa_group_t, char *); /* parse legacy opts */
	char   *(*sa_legacy_format)(sa_group_t, int);
	int	(*sa_set_proto_prop)(sa_property_t);
	sa_protocol_properties_t (*sa_get_proto_set)();
	char   *(*sa_get_proto_status)();
	char   *(*sa_space_alias)(char *);
	int	(*sa_update_legacy)(sa_share_t);
	int	(*sa_delete_legacy)(sa_share_t);
	int	(*sa_run_command)(int, int, char **); /* proto specific */
	int	(*sa_command_help)();
};

struct sa_proto_handle {
	int			sa_num_proto;
	char			**sa_proto;
	struct sa_plugin_ops	**sa_ops;
};

typedef struct propertylist {
	struct propertylist	*pl_next;
	int			pl_type;
	union propval {
	    sa_optionset_t	pl_optionset;
	    sa_security_t	pl_security;
	    void		*pl_void;
	}			pl_value;
} property_list_t;

extern int sa_proto_share(char *, sa_share_t);
extern int sa_proto_unshare(char *, char *);
extern int sa_proto_valid_prop(char *, sa_property_t, sa_optionset_t);
extern int sa_proto_security_prop(char *, char *);
extern int sa_proto_legacy_opts(char *, sa_group_t, char *);

/* internal utility functions */
extern sa_optionset_t sa_get_derived_optionset(sa_group_t, char *, int);
extern void sa_free_derived_optionset(sa_optionset_t);
extern sa_optionset_t sa_get_all_security_types(void *, char *, int);
extern sa_security_t sa_get_derived_security(void *, char *, char *, int);
extern void sa_free_derived_security(sa_security_t);
extern sa_protocol_properties_t sa_create_protocol_properties(char *);
extern int sa_start_transaction(scfutilhandle_t *, char *);
extern int sa_end_transaction(scfutilhandle_t *);
extern void sa_abort_transaction(scfutilhandle_t *);
extern int sa_commit_share(scfutilhandle_t *, sa_group_t, sa_share_t);
extern int sa_set_property(scfutilhandle_t *, char *, char *);
extern void sa_free_fstype(char *fstyp);
extern int sa_delete_share(scfutilhandle_t *, sa_group_t, sa_share_t);
extern int sa_delete_instance(scfutilhandle_t *, char *);
extern int sa_create_pgroup(scfutilhandle_t *, char *);
extern int sa_delete_pgroup(scfutilhandle_t *, char *);

/* ZFS functions */
extern int sa_get_zfs_shares(char *);
extern int sa_zfs_update(sa_share_t);

/* plugin specific functions */
extern int proto_plugin_init();
extern int sa_proto_set_property(char *, sa_property_t);
extern int sa_proto_delete_legacy(char *, sa_share_t);
extern int sa_proto_update_legacy(char *, sa_share_t);

#define	PL_TYPE_PROPERTY	0
#define	PL_TYPE_SECURITY	1

/* values only used by the internal dfstab/sharetab parser */
#define	SA_SHARE_PARSER		0x1000

/* plugin handler only */
struct sa_proto_plugin {
	struct sa_proto_plugin	*plugin_next;
	struct sa_plugin_ops	*plugin_ops;
	void			*plugin_handle;
};

#ifdef	__cplusplus
}
#endif

#endif /* _LIBSHARE_IMPL_H */
