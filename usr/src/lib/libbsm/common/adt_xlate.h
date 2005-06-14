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
 * adt_xlate.h
 *
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Automatically generated code; do not edit
 */

#ifndef _BSM_XLATE_H
#define	_BSM_XLATE_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <bsm/libbsm.h>
#include <priv.h>
#include <bsm/adt_event.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef TEXT_DOMAIN
#define	TEXT_DOMAIN	"SYS_TEST"
#endif

/*
 * values for adt_session_model
 * In the session model, the session and process are unrelated, so
 * such things as the supplementary group token make no sense.  In
 * the process model, the process and session are the same.
 */
#define	ADT_SESSION_MODEL	1
#define	ADT_PROCESS_MODEL	0

#define	ADT_HAVE_MASK	0x01
#define	ADT_HAVE_TID	0x02
#define	ADT_HAVE_AUID	0x04
#define	ADT_HAVE_ASID	0x08
#define	ADT_HAVE_IDS	0x16
#define	ADT_HAVE_ALL	(uint32_t)\
	(ADT_HAVE_MASK | ADT_HAVE_TID | ADT_HAVE_AUID | ADT_HAVE_ASID |\
	ADT_HAVE_IDS)

/*
 * dummy token types for privilege
 */
#define	ADT_AUT_PRIV_L	-100	/* limit set */
#define	ADT_AUT_PRIV_I	-101	/* inherited set */
#define	ADT_AUT_PRIV_E	-102	/* effective set */
/* dummy token type for alternate command */
#define	ADT_CMD_ALT	-103

enum adt_generic {ADT_GENERIC}; /* base for text enums */

typedef struct adt_internal_state	adt_internal_state_t;

union union_of_events {
	union adt_event_data	d0;
};
enum adt_msg_list {
	ADT_LIST_FAIL_PAM,
	ADT_LIST_FAIL_VALUE,
	ADT_LIST_LOGIN_TEXT};

enum datatype {ADT_UNDEFINED = 0,
    ADT_DATE,
    ADT_MSG,
    ADT_UINT,
    ADT_INT,
    ADT_INT32,
    ADT_UINT16,
    ADT_UINT32,
    ADT_UINT32STAR,
    ADT_UINT32ARRAY,
    ADT_UID,
    ADT_GID,
    ADT_UIDSTAR,
    ADT_GIDSTAR,
    ADT_UINT64,
    ADT_LONG,
    ADT_ULONG,
    ADT_CHAR,
    ADT_CHARSTAR,
    ADT_CHAR2STAR,	/* char **			*/
    ADT_PID,
    ADT_PRIVSTAR,
    ADT_TERMIDSTAR
};
typedef enum datatype datatype_t;

union convert {
    enum adt_generic	msg_selector;
    boolean_t		tbool;
    uint_t		tuint;
    int			tint;
    int32_t		tint32;
    uint16_t		tuint16;
    uint32_t		tuint32;
    uint64_t		tuint64;
    int32_t		*tint32star;
    uint32_t		*tuint32star;
    uid_t		tuid;
    gid_t		tgid;
    uid_t		*tuidstar;
    gid_t		*tgidstar;
    pid_t		tpid;
    long		tlong;
    ulong_t		tulong;
    char		tchar;
    char		*tcharstar;
    char		**tchar2star;
    au_tid_addr_t 	*ttermid;
    priv_set_t		*tprivstar;
};

struct adt_event_state {
	union union_of_events	ae_event_data;

	/* above is user's area; below is internal.  Order matters */

	uint_t		ae_check;	/* see adt_internal_state	*/
	int		ae_event_handle;
	au_event_t	ae_event_id;	/* external id			*/
	au_event_t	ae_internal_id; /* translated			*/
	int		ae_rc;		/* exit token rc		*/
	int		ae_type;	/* exit error type		*/
	struct adt_internal_state *ae_session;
};

struct datadefs {
	datatype_t	dd_datatype;	/* input data type */
	size_t		dd_input_size;	/* input data size */
};
typedef struct datadefs datadef;

typedef void (* adt_token_func_t)(datadef *, void *, int,
    struct adt_event_state *, char *);

typedef char *(* adt_msg_func_t)(enum adt_generic);

#define	ADT_VALID	0xAAAA5555

struct adt_internal_state {
	uint32_t	as_check;	/* == ADT_VALID when created,	*/
					/* == zero when freed		*/
	uid_t		as_euid;
	uid_t		as_ruid;
	gid_t		as_egid;
	gid_t		as_rgid;

	struct auditinfo_addr as_info;
	/*
	 * ai_auid				audit id
	 * ai_mask.am_success			pre-selection mask
	 * ai_mask.am_failure
	 * ai_termid	.at_port		terminal id
	 *		.at_type
	 *		.ai_termid.at_addr[0]
	 *		.ai_termid.at_addr[1]
	 *		.ai_termid.at_addr[2]
	 *		.ai_termid.at_addr[3]
	 * ai_asid				session id
	 */
	int		as_audit_enabled;	/* audit enable/disable state */
	/*
	 * data above this line is exported / imported
	 * To maintain upward compatibility, the above structures
	 * can't change, so for version 2, all changes will need
	 * to be added here and the old format (above) maintained.
	 */

	uint32_t		as_have_user_data;

	int			as_kernel_audit_policy;
	int			as_session_model;
	adt_session_flags_t	as_flags;
};

/*
 * export data format
 * version number changes when adt_internal_state's export portion
 * changes.
 */
#define	PROTOCOL_VERSION 1

/*
 * most recent version is at the top; down level consumers are
 * expected to search down via "prev_offsetX" to a version they
 * understand.  "v1" is first, "v0" is used to illustrate correct
 * order for future use.
 */

struct adt_export_v1 {
	int32_t		ax_euid;
	int32_t		ax_ruid;
	int32_t		ax_egid;
	int32_t		ax_rgid;
	int32_t		ax_auid;
	uint32_t	ax_mask_success;
	uint32_t	ax_mask_failure;
	uint32_t	ax_port;
	uint32_t	ax_type;
	uint32_t	ax_addr[4];
	uint32_t	ax_asid;
	int		ax_audit_enabled;
	uint32_t	ax_size_of_tsol_data;	/* zero for non-TSOL systems */
};
struct export_link {
	int32_t		ax_version;
	int32_t		ax_offset;
};
struct export_header {
	uint32_t		ax_check;
	int32_t			ax_buffer_length;
	struct export_link	ax_link;
};

struct adt_export_data {
	struct export_header	ax_header;

	struct		adt_export_v1 ax_v1;
	/*
	 * end of version 1 data
	 * struct export_link	ax_next_A;
	 * data for older version
	 * struct adt_export_v0 ax_v0;
	 */
	struct export_link	ax_last; /* terminator */
};

/*
 * struct entry defines rows in tables defined in adt_xlate.c
 */

struct entry {
	char		en_token_id;	/* token id */
	int		en_count_types;	/* # of input fields for this token */
	datadef		*en_type_def;	/* field type and size of each input */
	struct entry	*en_next_token;	/* linked list pointer */
	size_t		en_offset;	/* offset into structure for input */
	int		en_required;	/* if 1, always output a token */
	int		en_tsol;	/* if 1, output only #ifdef TSOL */
	char		*en_msg_format;	/* pointer to sprintf format string */
};

struct translation {
	int		tx_offsetsCalculated;	/* eponymous */
	au_event_t	tx_external_event;	/* event id, external view */
	au_event_t	tx_internal_event;	/* event id, internal view */
	int		tx_entries;		/* array size of entry array */
	struct entry	*tx_first_entry;	/* start of linked list */
	struct entry	*tx_top_entry;		/* first array element */
};

extern struct translation *xlate_table[];

struct token_jmp {
	long			jmp_id;
	adt_token_func_t	jmp_to;
};

struct msg_text {
	int	ml_min_index;
	int	ml_max_index;
	char	**ml_msg_list;
	int	ml_offset;
};

extern void adt_write_syslog(const char *, int);
extern void adt_token_open(struct adt_event_state *);
extern void adt_token_close(struct adt_event_state *);
extern void adt_generate_token(struct entry *, void *,
    struct adt_event_state *);
extern void *adt_adjust_address(void *, size_t, size_t);
extern void adt_preload(au_event_t, adt_event_data_t *);

extern struct msg_text adt_msg_text[];

#ifdef	__cplusplus
}
#endif

#endif	/* _BSM_XLATE_H */
