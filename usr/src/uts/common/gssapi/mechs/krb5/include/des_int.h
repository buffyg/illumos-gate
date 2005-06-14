/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * lib/crypto/des/des_int.h
 *
 * Copyright 1987, 1988, 1990 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 *
 * Private include file for the Data Encryption Standard library.
 */

/*
 * Copyright (C) 1998 by the FundsXpress, INC.
 *
 * All rights reserved.
 *
 * Export of this software from the United States of America may require
 * a specific license from the United States Government.  It is the
 * responsibility of any person or organization contemplating export to
 * obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of FundsXpress. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  FundsXpress makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* only do the whole thing once	 */
#ifndef	DES_INTERNAL_DEFS
#define	DES_INTERNAL_DEFS

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <k5-int.h>
/*
 * Begin "mit-des.h"
 */
#ifndef KRB5_MIT_DES__
#define KRB5_MIT_DES__

#if !defined(PROTOTYPE)
#if defined(__STDC__) || defined(_MSDOS)
#define PROTOTYPE(x) x
#else
#define PROTOTYPE(x) ()
#endif
#endif

typedef krb5_octet mit_des_cblock[8];		/* crypto-block size */

#ifndef DES_INT32
#ifdef SIZEOF_INT
#if SIZEOF_INT >= 4
#define DES_INT32 int
#else
#define DES_INT32 long
#endif
#else /* !defined(SIZEOF_INT) */
#include <limits.h>
#if (UINT_MAX >= 0xffffffff)
#define DES_INT32 int
#else
#define DES_INT32 long
#endif
#endif /* !defined(SIZEOF_INT) */
#endif /* !defined(DES_INT32) */

/* Triple-DES structures */
typedef mit_des_cblock		mit_des3_cblock[3];

#define MIT_DES_ENCRYPT	1
#define MIT_DES_DECRYPT	0

#define K5ROUNDUP(x, align)     (-(-(x) & -(align)))

/* the first byte of the key is already in the keyblock */

#define MIT_DES_BLOCK_LENGTH 		(8*sizeof(krb5_octet))
#define	MIT_DES_CBC_CRC_PAD_MINIMUM	CRC32_CKSUM_LENGTH
/* This used to be 8*sizeof(krb5_octet) */
#define MIT_DES_KEYSIZE		 	8

#define MIT_DES_CBC_CKSUM_LENGTH	(4*sizeof(krb5_octet))

/*
 * Check if k5-int.h has been included before us.  If so, then check to see
 * that our view of the DES key size is the same as k5-int.h's.
 */
#ifdef	KRB5_MIT_DES_KEYSIZE
#if	MIT_DES_KEYSIZE != KRB5_MIT_DES_KEYSIZE
error(MIT_DES_KEYSIZE does not equal KRB5_MIT_DES_KEYSIZE)
#endif	/* MIT_DES_KEYSIZE != KRB5_MIT_DES_KEYSIZE */
#endif	/* KRB5_MIT_DES_KEYSIZE */
#endif /* KRB5_MIT_DES__ */
/*
 * End "mit-des.h"
 */

#ifndef _KERNEL
/* afsstring2key.c */
extern krb5_error_code mit_afs_string_to_key
	PROTOTYPE((krb5_context context,
		krb5_keyblock FAR *keyblock,
		const krb5_data FAR *data,
		const krb5_data FAR *salt));
#endif

/* f_cksum.c */
extern unsigned long mit_des_cbc_cksum
    PROTOTYPE((
	krb5_context context,
	krb5_octet FAR *, krb5_octet FAR *, long ,
	krb5_keyblock *, krb5_octet FAR *));

/* f_cbc.c */
extern int mit_des_cbc_encrypt
    PROTOTYPE((krb5_context context,
	const mit_des_cblock FAR *in,
	mit_des_cblock FAR *out, long length,
	krb5_keyblock *key,
	mit_des_cblock ivec,
	int encrypt));

/* fin_rndkey.c */
extern krb5_error_code mit_des_finish_random_key
    PROTOTYPE(( const krb5_encrypt_block FAR *,
		krb5_pointer FAR *));

/* finish_key.c */
extern krb5_error_code mit_des_finish_key
    PROTOTYPE(( krb5_encrypt_block FAR *));

/* key_parity.c */
extern void mit_des_fixup_key_parity PROTOTYPE((mit_des_cblock ));
extern int mit_des_check_key_parity PROTOTYPE((mit_des_cblock ));

/* process_ky.c */
extern krb5_error_code mit_des_process_key
    PROTOTYPE(( krb5_encrypt_block FAR *,  const krb5_keyblock FAR *));

/* string2key.c */
extern krb5_error_code mit_des_string_to_key
    PROTOTYPE((const krb5_encrypt_block FAR *,
		krb5_keyblock FAR *,
		const krb5_data FAR *,
		const krb5_data FAR *));

/* weak_key.c */
extern int mit_des_is_weak_key PROTOTYPE((mit_des_cblock ));

/* cmb_keys.c */
krb5_error_code mit_des_combine_subkeys
    PROTOTYPE((const krb5_keyblock FAR *, const krb5_keyblock FAR *,
	       krb5_keyblock FAR * FAR *));

/* f_pcbc.c */
int mit_des_pcbc_encrypt ();

/* misc.c */
extern void swap_bits PROTOTYPE((char FAR *));
extern unsigned long long_swap_bits PROTOTYPE((unsigned long ));
extern unsigned long swap_six_bits_to_ansi PROTOTYPE((unsigned long ));
extern unsigned long swap_four_bits_to_ansi PROTOTYPE((unsigned long ));
extern unsigned long swap_bit_pos_1 PROTOTYPE((unsigned long ));
extern unsigned long swap_bit_pos_0 PROTOTYPE((unsigned long ));
extern unsigned long swap_bit_pos_0_to_ansi PROTOTYPE((unsigned long ));
extern unsigned long rev_swap_bit_pos_0 PROTOTYPE((unsigned long ));
extern unsigned long swap_byte_bits PROTOTYPE((unsigned long ));
extern unsigned long swap_long_bytes_bit_number PROTOTYPE((unsigned long ));
#ifdef FILE
/* XXX depends on FILE being a #define! */
extern void test_set PROTOTYPE((FILE *, const char *, int, const char *, int));
#endif

/* d3_cbc.c */
extern int mit_des3_cbc_encrypt
	PROTOTYPE((krb5_context context,
		const mit_des_cblock FAR *in,
		mit_des_cblock FAR *out,
		long length,
		krb5_keyblock *key,
		mit_des_cblock ivec,
		int encrypt));

/* d3_procky.c */
extern krb5_error_code mit_des3_process_key
	PROTOTYPE((krb5_encrypt_block * eblock,
		   const krb5_keyblock * keyblock));

/* d3_str2ky.c */
extern krb5_error_code mit_des3_string_to_key
	PROTOTYPE((const krb5_encrypt_block FAR *,
		   krb5_keyblock FAR *,
		   const krb5_data FAR *,
		   const krb5_data FAR *));


/* u_nfold.c */
extern krb5_error_code mit_des_n_fold
	PROTOTYPE((const krb5_octet * input,
		   const size_t in_len,
		   krb5_octet * output,
		   const size_t out_len));

extern krb5_error_code mit_des_set_random_sequence_number
	PROTOTYPE((const krb5_data * sequence,
		   krb5_pointer random_state));

#endif	/*DES_INTERNAL_DEFS*/
