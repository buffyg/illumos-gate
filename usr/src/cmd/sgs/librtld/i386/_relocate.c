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
#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include	<string.h>
#include	"machdep.h"
#include	"reloc.h"
#include	"_librtld.h"
#include	"_elf.h"

/*
 * Undo relocations that have been applied to a memory image.  Basically this
 * involves copying the original files relocation offset into the new image
 * being created.
 */
void
undo_reloc(void *vrel, uchar_t *oaddr, uchar_t *iaddr, Reloc *reloc)
{
	Rel	*rel = vrel;
	/* LINTED */
	ulong_t	*_oaddr = (ulong_t *)oaddr;
	/* LINTED */
	ulong_t	*_iaddr = (ulong_t *)iaddr;

	switch (ELF_R_TYPE(rel->r_info)) {
	case R_386_NONE:
		break;

	case R_386_COPY:
		(void) memset((void *)oaddr, 0, (size_t)reloc->r_size);
		break;

	case R_386_JMP_SLOT:
		if (_iaddr)
			*_oaddr = *_iaddr + reloc->r_value;
		else
			*_oaddr = reloc->r_value;
		break;

	default:
		if (_iaddr)
			*_oaddr = *_iaddr;
		else
			*_oaddr = 0;
		break;
	}
}

/*
 * Copy a relocation record and increment its value.  The record must reflect
 * the new address to which this image is fixed.  Note that .got entries
 * associated with .plt's must be fixed to the new base address.
 */
void
inc_reloc(void *vnrel, void *vorel, Reloc *reloc, uchar_t *oaddr,
    uchar_t *iaddr)
{
	Rel	*nrel = vnrel;
	Rel	*orel = vorel;
	/* LINTED */
	ulong_t	*_oaddr = (ulong_t *)oaddr;
	/* LINTED */
	ulong_t	*_iaddr = (ulong_t *)iaddr;

	if (ELF_R_TYPE(nrel->r_info) == R_386_JMP_SLOT) {
		if (_iaddr)
			*_oaddr = *_iaddr + reloc->r_value;
		else
			*_oaddr = reloc->r_value;
	}

	*nrel = *orel;
	nrel->r_offset += reloc->r_value;
}

/*
 * Clear a relocation record.  The relocation has been applied to the image and
 * thus the relocation must not occur again.
 */
void
clear_reloc(void *vrel)
{
	Rel	*rel = vrel;

	rel->r_offset = 0;
	rel->r_info = ELF_R_INFO(0, R_386_NONE);
}

/*
 * Apply a relocation to an image being built from an input file.  Use the
 * runtime linkers routines to do the necessary magic.
 */
void
apply_reloc(void *vrel, Reloc *reloc, const char *name, uchar_t *oaddr,
    Rt_map *lmp)
{
	Rel	*rel = vrel;
	Xword	type = ELF_R_TYPE(rel->r_info);
	Word	value = reloc->r_value;

	if (type == R_386_JMP_SLOT) {
		uintptr_t	addr, vaddr;

		if (FLAGS(lmp) & FLG_RT_FIXED)
			vaddr = 0;
		else
			vaddr = ADDR(lmp);
		addr = (uintptr_t)oaddr - rel->r_offset;
		/* LINTED */
		elf_plt_write((uintptr_t)addr, vaddr, rel,
		    (uintptr_t)value, reloc->r_pltndx);

	} else if (type == R_386_COPY) {
		(void) memcpy((void *)oaddr, (void *)value,
		    (size_t)reloc->r_size);
	} else {
		(void) do_reloc(type, oaddr, &value, reloc->r_name, name,
		    LIST(lmp));
	}
}
