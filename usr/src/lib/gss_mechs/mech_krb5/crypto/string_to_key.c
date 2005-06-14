/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"
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

#include <k5-int.h>
#include <etypes.h>

krb5_error_code KRB5_CALLCONV
krb5_c_string_to_key_with_params(context, enctype, string, salt, params, key)
	krb5_context context;
	krb5_enctype enctype;
	const krb5_data *string;
	const krb5_data *salt;
	const krb5_data *params;
	krb5_keyblock *key;
{
    int i;
    krb5_error_code ret;
    const struct krb5_enc_provider *enc;
    size_t keybytes, keylength;

    for (i=0; i<krb5_enctypes_length; i++) {
	if (krb5_enctypes_list[i].etype == enctype)
            break;
    }

    if (i == krb5_enctypes_length)
	return(KRB5_BAD_ENCTYPE);

    enc = krb5_enctypes_list[i].enc;

    (*(enc->keysize))(&keybytes, &keylength);

    if ((key->contents = (krb5_octet *) malloc(keylength)) == NULL)
	return(ENOMEM);

    key->magic = KV5M_KEYBLOCK;
    key->enctype = enctype;
    key->length = keylength;
    key->dk_list = NULL;
    key->hKey = CK_INVALID_HANDLE;

    ret = (*krb5_enctypes_list[i].str2key)(context, enc, string, salt,
			params, key);
    if (ret) {
	memset(key->contents, 0, keylength);
	free(key->contents);
	key->contents = NULL;
    }

    return(ret);
}

/*ARGSUSED*/
KRB5_DLLIMP krb5_error_code KRB5_CALLCONV
krb5_c_string_to_key(context, enctype, string, salt, key)
     krb5_context context;
     krb5_enctype enctype;
     krb5_const krb5_data *string;
     krb5_const krb5_data *salt;
     krb5_keyblock *key;
{
    return krb5_c_string_to_key_with_params(context, enctype, string, salt,
                                            NULL, key);
}

