/* $OpenBSD: v3_bitst.c,v 1.14 2017/01/29 17:49:23 beck Exp $ */
/* Written by Dr Stephen N Henson (steve@openssl.org) for the OpenSSL
 * project 1999.
 */
/* ====================================================================
 * Copyright (c) 1999 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <stdio.h>
#include <string.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

static BIT_STRING_BITNAME ns_cert_type_table[] = {
	{0, "SSL Client", "client"},
	{1, "SSL Server", "server"},
	{2, "S/MIME", "email"},
	{3, "Object Signing", "objsign"},
	{4, "Unused", "reserved"},
	{5, "SSL CA", "sslCA"},
	{6, "S/MIME CA", "emailCA"},
	{7, "Object Signing CA", "objCA"},
	{-1, NULL, NULL}
};

static BIT_STRING_BITNAME key_usage_type_table[] = {
	{0, "Digital Signature", "digitalSignature"},
	{1, "Non Repudiation", "nonRepudiation"},
	{2, "Key Encipherment", "keyEncipherment"},
	{3, "Data Encipherment", "dataEncipherment"},
	{4, "Key Agreement", "keyAgreement"},
	{5, "Certificate Sign", "keyCertSign"},
	{6, "CRL Sign", "cRLSign"},
	{7, "Encipher Only", "encipherOnly"},
	{8, "Decipher Only", "decipherOnly"},
	{-1, NULL, NULL}
};

const X509V3_EXT_METHOD v3_nscert = {
	.ext_nid = NID_netscape_cert_type,
	.ext_flags = 0,
	.it = &ASN1_BIT_STRING_it,
	.ext_new = NULL,
	.ext_free = NULL,
	.d2i = NULL,
	.i2d = NULL,
	.i2s = NULL,
	.s2i = NULL,
	.i2v = (X509V3_EXT_I2V)i2v_ASN1_BIT_STRING,
	.v2i = (X509V3_EXT_V2I)v2i_ASN1_BIT_STRING,
	.i2r = NULL,
	.r2i = NULL,
	.usr_data = ns_cert_type_table,
};

const X509V3_EXT_METHOD v3_key_usage = {
	.ext_nid = NID_key_usage,
	.ext_flags = 0,
	.it = &ASN1_BIT_STRING_it,
	.ext_new = NULL,
	.ext_free = NULL,
	.d2i = NULL,
	.i2d = NULL,
	.i2s = NULL,
	.s2i = NULL,
	.i2v = (X509V3_EXT_I2V)i2v_ASN1_BIT_STRING,
	.v2i = (X509V3_EXT_V2I)v2i_ASN1_BIT_STRING,
	.i2r = NULL,
	.r2i = NULL,
	.usr_data = key_usage_type_table,
};

STACK_OF(CONF_VALUE) *
i2v_ASN1_BIT_STRING(X509V3_EXT_METHOD *method, ASN1_BIT_STRING *bits,
    STACK_OF(CONF_VALUE) *ret)
{
	BIT_STRING_BITNAME *bnam;

	for (bnam = method->usr_data; bnam->lname; bnam++) {
		if (ASN1_BIT_STRING_get_bit(bits, bnam->bitnum))
			X509V3_add_value(bnam->lname, NULL, &ret);
	}
	return ret;
}

ASN1_BIT_STRING *
v2i_ASN1_BIT_STRING(X509V3_EXT_METHOD *method, X509V3_CTX *ctx,
    STACK_OF(CONF_VALUE) *nval)
{
	CONF_VALUE *val;
	ASN1_BIT_STRING *bs;
	int i;
	BIT_STRING_BITNAME *bnam;

	if (!(bs = ASN1_BIT_STRING_new())) {
		X509V3error(ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	for (i = 0; i < sk_CONF_VALUE_num(nval); i++) {
		val = sk_CONF_VALUE_value(nval, i);
		for (bnam = method->usr_data; bnam->lname; bnam++) {
			if (!strcmp(bnam->sname, val->name) ||
			    !strcmp(bnam->lname, val->name) ) {
				if (!ASN1_BIT_STRING_set_bit(bs,
				    bnam->bitnum, 1)) {
					X509V3error(ERR_R_MALLOC_FAILURE);
					ASN1_BIT_STRING_free(bs);
					return NULL;
				}
				break;
			}
		}
		if (!bnam->lname) {
			X509V3error(X509V3_R_UNKNOWN_BIT_STRING_ARGUMENT);
			X509V3_conf_err(val);
			ASN1_BIT_STRING_free(bs);
			return NULL;
		}
	}
	return bs;
}
