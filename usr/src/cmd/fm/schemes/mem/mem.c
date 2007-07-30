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

#include <mem.h>
#include <fm/fmd_fmri.h>

#include <string.h>
#include <strings.h>
#include <sys/mem.h>

#ifdef	sparc
#include <sys/fm/ldom.h>
ldom_hdl_t *mem_scheme_lhp;
#endif	/* sparc */

mem_t mem;

static int
mem_fmri_get_unum(nvlist_t *nvl, char **unump)
{
	uint8_t version;
	char *unum;

	if (nvlist_lookup_uint8(nvl, FM_VERSION, &version) != 0 ||
	    version > FM_MEM_SCHEME_VERSION ||
	    nvlist_lookup_string(nvl, FM_FMRI_MEM_UNUM, &unum) != 0)
		return (fmd_fmri_set_errno(EINVAL));

	*unump = unum;

	return (0);
}

ssize_t
fmd_fmri_nvl2str(nvlist_t *nvl, char *buf, size_t buflen)
{
	char format[64];
	ssize_t size, presz;
	char *rawunum, *preunum, *escunum, *prefix;
	uint64_t val;
	int i;

	if (mem_fmri_get_unum(nvl, &rawunum) < 0)
		return (-1); /* errno is set for us */

	/*
	 * If we have a well-formed unum (hc-FMRI), use the string verbatim
	 * to form the initial mem:/// components.  Otherwise use unum=%s.
	 */
	if (strncmp(rawunum, "hc://", 5) != 0)
		prefix = FM_FMRI_MEM_UNUM "=";
	else
		prefix = "";

	/*
	 * If we have a DIMM offset, include it in the string.  If we have a PA
	 * then use that.  Otherwise just format the unum element.
	 */
	if (nvlist_lookup_uint64(nvl, FM_FMRI_MEM_OFFSET, &val) == 0) {
		(void) snprintf(format, sizeof (format),
		    "%s:///%s%%1$s/%s=%%2$llx",
		    FM_FMRI_SCHEME_MEM, prefix, FM_FMRI_MEM_OFFSET);
	} else if (nvlist_lookup_uint64(nvl, FM_FMRI_MEM_PHYSADDR, &val) == 0) {
		(void) snprintf(format, sizeof (format),
		    "%s:///%s%%1$s/%s=%%2$llx",
		    FM_FMRI_SCHEME_MEM, prefix, FM_FMRI_MEM_PHYSADDR);
	} else {
		(void) snprintf(format, sizeof (format),
		    "%s:///%s%%1$s", FM_FMRI_SCHEME_MEM, prefix);
	}

	/*
	 * If we have a well-formed unum (hc-FMRI), we skip over the
	 * the scheme and authority prefix.
	 * Otherwise, the spaces and colons will be escaped,
	 * rendering the resulting FMRI pretty much unreadable.
	 * We're therefore going to do some escaping of our own first.
	 */
	if (strncmp(rawunum, "hc://", 5) == 0) {
		rawunum += 5;
		rawunum = strchr(rawunum, '/');
		++rawunum;
		/* LINTED: variable format specifier */
		size = snprintf(buf, buflen, format, rawunum, val);
	} else {
		preunum = fmd_fmri_strdup(rawunum);
		presz = strlen(preunum) + 1;

		for (i = 0; i < presz - 1; i++) {
			if (preunum[i] == ':' && preunum[i + 1] == ' ') {
				bcopy(preunum + i + 2, preunum + i + 1,
				    presz - (i + 2));
			} else if (preunum[i] == ' ') {
				preunum[i] = ',';
			}
		}

		escunum = fmd_fmri_strescape(preunum);
		fmd_fmri_free(preunum, presz);

		/* LINTED: variable format specifier */
		size = snprintf(buf, buflen, format, escunum, val);
		fmd_fmri_strfree(escunum);
	}

	return (size);
}

int
fmd_fmri_expand(nvlist_t *nvl)
{
	char *unum, **serids;
	uint_t nnvlserids;
	size_t nserids;
	int rc;

	if ((mem_fmri_get_unum(nvl, &unum) < 0) || (*unum == '\0'))
		return (fmd_fmri_set_errno(EINVAL));

	if ((rc = nvlist_lookup_string_array(nvl, FM_FMRI_MEM_SERIAL_ID,
	    &serids, &nnvlserids)) == 0) { /* already have serial #s */
		mem_expand_opt(nvl, unum, serids);
		return (0);
	} else if (rc != ENOENT)
		return (fmd_fmri_set_errno(EINVAL));

	if (mem_get_serids_by_unum(unum, &serids, &nserids) < 0) {
		/* errno is set for us */
		if (errno == ENOTSUP)
			return (0); /* nothing to add - no s/n support */
		else
			return (-1);
	}

	rc = nvlist_add_string_array(nvl, FM_FMRI_MEM_SERIAL_ID, serids,
	    nserids);
	mem_expand_opt(nvl, unum, serids);

	mem_strarray_free(serids, nserids);

	if (rc != 0)
		return (fmd_fmri_set_errno(EINVAL));
	else
		return (0);
}

static int
serids_eq(char **serids1, uint_t nserids1, char **serids2, uint_t nserids2)
{
	int i;

	if (nserids1 != nserids2)
		return (0);

	for (i = 0; i < nserids1; i++) {
		if (strcmp(serids1[i], serids2[i]) != 0)
			return (0);
	}

	return (1);
}

int
fmd_fmri_present(nvlist_t *nvl)
{
	char *unum, **nvlserids, **serids;
	uint_t nnvlserids;
	size_t nserids;
	uint64_t memconfig;
	int rc;

	if (mem_fmri_get_unum(nvl, &unum) < 0)
		return (-1); /* errno is set for us */

	if (nvlist_lookup_string_array(nvl, FM_FMRI_MEM_SERIAL_ID, &nvlserids,
	    &nnvlserids) != 0) {
		/*
		 * Some mem scheme FMRIs don't have serial ids because
		 * either the platform does not support them, or because
		 * the FMRI was created before support for serial ids was
		 * introduced.  If this is the case, assume it is there.
		 */
		if (mem.mem_dm == NULL)
			return (1);
		else
			return (fmd_fmri_set_errno(EINVAL));
	}

	/*
	 * Hypervisor will change the memconfig value when the mapping of
	 * pages to DIMMs changes, e.g. for change in DIMM size or interleave.
	 * If we detect such a change, we discard ereports associated with a
	 * previous memconfig value as invalid.
	 *
	 * The test (mem.mem_memconfig != 0) means we run on a system that
	 * actually suplies a memconfig value.
	 */

	if ((nvlist_lookup_uint64(nvl, FM_FMRI_MEM_MEMCONFIG,
	    &memconfig) == 0) && (mem.mem_memconfig != 0) &&
	    (memconfig != mem.mem_memconfig))
		return (0);

	if (mem_get_serids_by_unum(unum, &serids, &nserids) < 0) {
		if (errno == ENOTSUP)
			return (1); /* assume it's there, no s/n support here */
		if (errno != ENOENT) {
			/*
			 * Errors are only signalled to the caller if they're
			 * the caller's fault.  This isn't - it's a failure on
			 * our part to burst or read the serial numbers.  We'll
			 * whine about it, and tell the caller the named
			 * module(s) isn't/aren't there.
			 */
			fmd_fmri_warn("failed to retrieve serial number for "
			    "unum %s", unum);
		}
		return (0);
	}

	rc = serids_eq(serids, nserids, nvlserids, nnvlserids);

	mem_strarray_free(serids, nserids);

	return (rc);
}

int
fmd_fmri_contains(nvlist_t *er, nvlist_t *ee)
{
	char *erunum, *eeunum;
	uint64_t erval = 0, eeval = 0;

	if (mem_fmri_get_unum(er, &erunum) < 0 ||
	    mem_fmri_get_unum(ee, &eeunum) < 0)
		return (-1); /* errno is set for us */

	if (mem_unum_contains(erunum, eeunum) <= 0)
		return (0); /* can't parse/match, so assume no containment */

	if (nvlist_lookup_uint64(er, FM_FMRI_MEM_OFFSET, &erval) == 0) {
		return (nvlist_lookup_uint64(ee,
		    FM_FMRI_MEM_OFFSET, &eeval) == 0 && erval == eeval);
	}

	if (nvlist_lookup_uint64(er, FM_FMRI_MEM_PHYSADDR, &erval) == 0) {
		return (nvlist_lookup_uint64(ee,
		    FM_FMRI_MEM_PHYSADDR, &eeval) == 0 && erval == eeval);
	}

	return (1);
}

/*
 * We can only make a usable/unusable determination for pages.  Mem FMRIs
 * without page addresses will be reported as usable since Solaris has no
 * way at present to dynamically disable an entire DIMM or DIMM pair.
 */
int
fmd_fmri_unusable(nvlist_t *nvl)
{
	uint64_t val;
	uint8_t version;
	int rc, err1, err2;
	nvlist_t *nvlcp = NULL;
	int retval;

	if (nvlist_lookup_uint8(nvl, FM_VERSION, &version) != 0 ||
	    version > FM_MEM_SCHEME_VERSION)
		return (fmd_fmri_set_errno(EINVAL));

	err1 = nvlist_lookup_uint64(nvl, FM_FMRI_MEM_OFFSET, &val);
	err2 = nvlist_lookup_uint64(nvl, FM_FMRI_MEM_PHYSADDR, &val);

	if (err1 == ENOENT && err2 == ENOENT)
		return (0); /* no page, so assume it's still usable */

	if ((err1 != 0 && err1 != ENOENT) || (err2 != 0 && err2 != ENOENT))
		return (fmd_fmri_set_errno(EINVAL));

	if ((err1 = mem_unum_rewrite(nvl, &nvlcp)) != 0)
		return (fmd_fmri_set_errno(err1));

	/*
	 * Ask the kernel if the page is retired, using either the rewritten
	 * hc FMRI or the original mem FMRI with the specified offset or PA.
	 * Refer to the kernel's page_retire_check() for the error codes.
	 */
	rc = mem_page_cmd(MEM_PAGE_FMRI_ISRETIRED, nvlcp ? nvlcp : nvl);

	if (rc == -1 && errno == EIO) {
		/*
		 * The page is not retired and is not scheduled for retirement
		 * (i.e. no request pending and has not seen any errors)
		 */
		retval = 0;
	} else if (rc == 0 || errno == EAGAIN || errno == EINVAL) {
		/*
		 * The page has been retired, is in the process of being
		 * retired, or doesn't exist.  The latter is valid if the page
		 * existed in the past but has been DR'd out.
		 */
		retval = 1;
	} else {
		/*
		 * Errors are only signalled to the caller if they're the
		 * caller's fault.  This isn't - it's a failure of the
		 * retirement-check code.  We'll whine about it and tell
		 * the caller the page is unusable.
		 */
		fmd_fmri_warn("failed to determine page %s=%llx usability: "
		    "rc=%d errno=%d\n", err1 == 0 ? FM_FMRI_MEM_OFFSET :
		    FM_FMRI_MEM_PHYSADDR, (u_longlong_t)val, rc, errno);
		retval = 1;
	}

	if (nvlcp)
		nvlist_free(nvlcp);

	return (retval);
}

int
fmd_fmri_init(void)
{
#ifdef	sparc
	mem_scheme_lhp = ldom_init(fmd_fmri_alloc, fmd_fmri_free);
#endif	/* sparc */
	return (mem_discover());
}

void
fmd_fmri_fini(void)
{
	mem_dimm_map_t *dm, *em;
	mem_seg_map_t *sm, *tm;

	for (dm = mem.mem_dm; dm != NULL; dm = em) {
		em = dm->dm_next;
		fmd_fmri_strfree(dm->dm_label);
		fmd_fmri_strfree(dm->dm_part);
		fmd_fmri_strfree(dm->dm_device);
		fmd_fmri_free(dm, sizeof (mem_dimm_map_t));
	}
	for (sm = mem.mem_seg; sm != NULL; sm = tm) {
		tm = sm->sm_next;
		fmd_fmri_free(sm, sizeof (mem_seg_map_t));
	}
#ifdef	sparc
	ldom_fini(mem_scheme_lhp);
#endif	/* sparc */
}
