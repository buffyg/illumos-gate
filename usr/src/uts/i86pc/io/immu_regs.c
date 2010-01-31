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
 * Portions Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * immu_regs.c  - File that operates on a IMMU unit's regsiters
 */
#include <sys/dditypes.h>
#include <sys/ddi.h>
#include <sys/archsystm.h>
#include <sys/x86_archext.h>
#include <sys/spl.h>
#include <sys/immu.h>

#define	get_reg32(immu, offset)	ddi_get32((immu)->immu_regs_handle, \
		(uint32_t *)(immu->immu_regs_addr + (offset)))
#define	get_reg64(immu, offset)	ddi_get64((immu)->immu_regs_handle, \
		(uint64_t *)(immu->immu_regs_addr + (offset)))
#define	put_reg32(immu, offset, val)	ddi_put32\
		((immu)->immu_regs_handle, \
		(uint32_t *)(immu->immu_regs_addr + (offset)), val)
#define	put_reg64(immu, offset, val)	ddi_put64\
		((immu)->immu_regs_handle, \
		(uint64_t *)(immu->immu_regs_addr + (offset)), val)

/*
 * wait max 60s for the hardware completion
 */
#define	IMMU_MAX_WAIT_TIME		60000000
#define	wait_completion(immu, offset, getf, completion, status) \
{ \
	clock_t stick = ddi_get_lbolt(); \
	clock_t ntick; \
	_NOTE(CONSTCOND) \
	while (1) { \
		status = getf(immu, offset); \
		ntick = ddi_get_lbolt(); \
		if (completion) { \
			break; \
		} \
		if (ntick - stick >= drv_usectohz(IMMU_MAX_WAIT_TIME)) { \
			ddi_err(DER_PANIC, NULL, \
			    "immu wait completion time out");		\
			/*NOTREACHED*/   \
		} else { \
			iommu_cpu_nop();\
		}\
	}\
}

static ddi_device_acc_attr_t immu_regs_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_NEVERSWAP_ACC,
	DDI_STRICTORDER_ACC,
};

/*
 * iotlb_flush()
 *   flush the iotlb cache
 */
static void
iotlb_flush(immu_t *immu, uint_t domain_id,
    uint64_t addr, uint_t am, uint_t hint, immu_iotlb_inv_t type)
{
	uint64_t command = 0, iva = 0;
	uint_t iva_offset, iotlb_offset;
	uint64_t status = 0;

	ASSERT(MUTEX_HELD(&(immu->immu_regs_lock)));

	/* no lock needed since cap and excap fields are RDONLY */
	iva_offset = IMMU_ECAP_GET_IRO(immu->immu_regs_excap);
	iotlb_offset = iva_offset + 8;

	/*
	 * prepare drain read/write command
	 */
	if (IMMU_CAP_GET_DWD(immu->immu_regs_cap)) {
		command |= TLB_INV_DRAIN_WRITE;
	}

	if (IMMU_CAP_GET_DRD(immu->immu_regs_cap)) {
		command |= TLB_INV_DRAIN_READ;
	}

	/*
	 * if the hardward doesn't support page selective invalidation, we
	 * will use domain type. Otherwise, use global type
	 */
	switch (type) {
	case IOTLB_PSI:
		if (!IMMU_CAP_GET_PSI(immu->immu_regs_cap) ||
		    (am > IMMU_CAP_GET_MAMV(immu->immu_regs_cap)) ||
		    (addr & IMMU_PAGEOFFSET)) {
			goto ignore_psi;
		}
		command |= TLB_INV_PAGE | TLB_INV_IVT |
		    TLB_INV_DID(domain_id);
		iva = addr | am | TLB_IVA_HINT(hint);
		break;
ignore_psi:
	case IOTLB_DSI:
		command |= TLB_INV_DOMAIN | TLB_INV_IVT |
		    TLB_INV_DID(domain_id);
		break;
	case IOTLB_GLOBAL:
		command |= TLB_INV_GLOBAL | TLB_INV_IVT;
		break;
	default:
		ddi_err(DER_MODE, NULL, "%s: incorrect iotlb flush type",
		    immu->immu_name);
		return;
	}

	/* verify there is no pending command */
	wait_completion(immu, iotlb_offset, get_reg64,
	    (!(status & TLB_INV_IVT)), status);
	if (iva)
		put_reg64(immu, iva_offset, iva);
	put_reg64(immu, iotlb_offset, command);
	wait_completion(immu, iotlb_offset, get_reg64,
	    (!(status & TLB_INV_IVT)), status);
}

/*
 * iotlb_psi()
 *   iotlb page specific invalidation
 */
static void
iotlb_psi(immu_t *immu, uint_t domain_id,
    uint64_t dvma, uint_t count, uint_t hint)
{
	uint_t am = 0;
	uint_t max_am = 0;
	uint64_t align = 0;
	uint64_t dvma_pg = 0;
	uint_t used_count = 0;

	mutex_enter(&(immu->immu_regs_lock));

	/* choose page specified invalidation */
	if (IMMU_CAP_GET_PSI(immu->immu_regs_cap)) {
		/* MAMV is valid only if PSI is set */
		max_am = IMMU_CAP_GET_MAMV(immu->immu_regs_cap);
		while (count != 0) {
			/* First calculate alignment of DVMA */
			dvma_pg = IMMU_BTOP(dvma);
			ASSERT(dvma_pg != NULL);
			ASSERT(count >= 1);
			for (align = 1; (dvma_pg & align) == 0; align <<= 1)
				;
			/* truncate count to the nearest power of 2 */
			for (used_count = 1, am = 0; count >> used_count != 0;
			    used_count <<= 1, am++)
				;
			if (am > max_am) {
				am = max_am;
				used_count = 1 << am;
			}
			if (align >= used_count) {
				iotlb_flush(immu, domain_id,
				    dvma, am, hint, IOTLB_PSI);
			} else {
				/* align < used_count */
				used_count = align;
				for (am = 0; (1 << am) != used_count; am++)
					;
				iotlb_flush(immu, domain_id,
				    dvma, am, hint, IOTLB_PSI);
			}
			count -= used_count;
			dvma = (dvma_pg + used_count) << IMMU_PAGESHIFT;
		}
	} else {
		/* choose domain invalidation */
		iotlb_flush(immu, domain_id, dvma, 0, 0, IOTLB_DSI);
	}

	mutex_exit(&(immu->immu_regs_lock));
}

/*
 * iotlb_dsi()
 *	domain specific invalidation
 */
static void
iotlb_dsi(immu_t *immu, uint_t domain_id)
{
	mutex_enter(&(immu->immu_regs_lock));
	iotlb_flush(immu, domain_id, 0, 0, 0, IOTLB_DSI);
	mutex_exit(&(immu->immu_regs_lock));
}

/*
 * iotlb_global()
 *     global iotlb invalidation
 */
static void
iotlb_global(immu_t *immu)
{
	mutex_enter(&(immu->immu_regs_lock));
	iotlb_flush(immu, 0, 0, 0, 0, IOTLB_GLOBAL);
	mutex_exit(&(immu->immu_regs_lock));
}


static int
gaw2agaw(int gaw)
{
	int r, agaw;

	r = (gaw - 12) % 9;

	if (r == 0)
		agaw = gaw;
	else
		agaw = gaw + 9 - r;

	if (agaw > 64)
		agaw = 64;

	return (agaw);
}

/*
 * set_immu_agaw()
 * 	calculate agaw for a IOMMU unit
 */
static int
set_agaw(immu_t *immu)
{
	int mgaw, magaw, agaw;
	uint_t bitpos;
	int max_sagaw_mask, sagaw_mask, mask;
	int nlevels;

	/*
	 * mgaw is the maximum guest address width.
	 * Addresses above this value will be
	 * blocked by the IOMMU unit.
	 * sagaw is a bitmask that lists all the
	 * AGAWs supported by this IOMMU unit.
	 */
	mgaw = IMMU_CAP_MGAW(immu->immu_regs_cap);
	sagaw_mask = IMMU_CAP_SAGAW(immu->immu_regs_cap);

	magaw = gaw2agaw(mgaw);

	/*
	 * Get bitpos corresponding to
	 * magaw
	 */

	/*
	 * Maximum SAGAW is specified by
	 * Vt-d spec.
	 */
	max_sagaw_mask = ((1 << 5) - 1);

	if (sagaw_mask > max_sagaw_mask) {
		ddi_err(DER_WARN, NULL, "%s: SAGAW bitmask (%x) "
		    "is larger than maximu SAGAW bitmask "
		    "(%x) specified by Intel Vt-d spec",
		    immu->immu_name, sagaw_mask, max_sagaw_mask);
		return (DDI_FAILURE);
	}

	/*
	 * Find a supported AGAW <= magaw
	 *
	 *	sagaw_mask    bitpos   AGAW (bits)  nlevels
	 *	==============================================
	 *	0 0 0 0 1	0	30		2
	 *	0 0 0 1 0	1	39		3
	 *	0 0 1 0 0	2	48		4
	 *	0 1 0 0 0	3	57		5
	 *	1 0 0 0 0	4	64(66)		6
	 */
	mask = 1;
	nlevels = 0;
	agaw = 0;
	for (mask = 1, bitpos = 0; bitpos < 5;
	    bitpos++, mask <<= 1) {
		if (mask & sagaw_mask) {
			nlevels = bitpos + 2;
			agaw = 30 + (bitpos * 9);
		}
	}

	/* calculated agaw can be > 64 */
	agaw = (agaw > 64) ? 64 : agaw;

	if (agaw < 30 || agaw > magaw) {
		ddi_err(DER_WARN, NULL, "%s: Calculated AGAW (%d) "
		    "is outside valid limits [30,%d] specified by Vt-d spec "
		    "and magaw",  immu->immu_name, agaw, magaw);
		return (DDI_FAILURE);
	}

	if (nlevels < 2 || nlevels > 6) {
		ddi_err(DER_WARN, NULL, "%s: Calculated pagetable "
		    "level (%d) is outside valid limits [2,6]",
		    immu->immu_name, nlevels);
		return (DDI_FAILURE);
	}

	ddi_err(DER_LOG, NULL, "Calculated pagetable "
	    "level (%d), agaw = %d", nlevels, agaw);

	immu->immu_dvma_nlevels = nlevels;
	immu->immu_dvma_agaw = agaw;

	return (DDI_SUCCESS);
}

static int
setup_regs(immu_t *immu)
{
	int error;

	ASSERT(immu);
	ASSERT(immu->immu_name);

	/*
	 * This lock may be acquired by the IOMMU interrupt handler
	 */
	mutex_init(&(immu->immu_regs_lock), NULL, MUTEX_DRIVER,
	    (void *)ipltospl(IMMU_INTR_IPL));

	/*
	 * map the register address space
	 */
	error = ddi_regs_map_setup(immu->immu_dip, 0,
	    (caddr_t *)&(immu->immu_regs_addr), (offset_t)0,
	    (offset_t)IMMU_REGSZ, &immu_regs_attr,
	    &(immu->immu_regs_handle));

	if (error == DDI_FAILURE) {
		ddi_err(DER_WARN, NULL, "%s: Intel IOMMU register map failed",
		    immu->immu_name);
		mutex_destroy(&(immu->immu_regs_lock));
		return (DDI_FAILURE);
	}

	/*
	 * get the register value
	 */
	immu->immu_regs_cap = get_reg64(immu, IMMU_REG_CAP);
	immu->immu_regs_excap = get_reg64(immu, IMMU_REG_EXCAP);

	/*
	 * if the hardware access is non-coherent, we need clflush
	 */
	if (IMMU_ECAP_GET_C(immu->immu_regs_excap)) {
		immu->immu_dvma_coherent = B_TRUE;
	} else {
		immu->immu_dvma_coherent = B_FALSE;
		if (!(x86_feature & X86_CLFSH)) {
			ddi_err(DER_WARN, NULL,
			    "immu unit %s can't be enabled due to "
			    "missing clflush functionality", immu->immu_name);
			ddi_regs_map_free(&(immu->immu_regs_handle));
			mutex_destroy(&(immu->immu_regs_lock));
			return (DDI_FAILURE);
		}
	}

	/*
	 * Check for Mobile 4 series chipset
	 */
	if (immu_quirk_mobile4 == B_TRUE &&
	    !IMMU_CAP_GET_RWBF(immu->immu_regs_cap)) {
		ddi_err(DER_LOG, NULL,
		    "IMMU: Mobile 4 chipset quirk detected. "
		    "Force-setting RWBF");
		IMMU_CAP_SET_RWBF(immu->immu_regs_cap);
		ASSERT(IMMU_CAP_GET_RWBF(immu->immu_regs_cap));
	}

	/*
	 * retrieve the maximum number of domains
	 */
	immu->immu_max_domains = IMMU_CAP_ND(immu->immu_regs_cap);

	/*
	 * calculate the agaw
	 */
	if (set_agaw(immu) != DDI_SUCCESS) {
		ddi_regs_map_free(&(immu->immu_regs_handle));
		mutex_destroy(&(immu->immu_regs_lock));
		return (DDI_FAILURE);
	}
	immu->immu_regs_cmdval = 0;

	return (DDI_SUCCESS);
}

/* ############### Functions exported ################## */

/*
 * immu_regs_setup()
 *       Setup mappings to a IMMU unit's registers
 *       so that they can be read/written
 */
void
immu_regs_setup(list_t *listp)
{
	int i;
	immu_t *immu;

	for (i = 0; i < IMMU_MAXSEG; i++) {
		immu = list_head(listp);
		for (; immu; immu = list_next(listp, immu)) {
			/* do your best, continue on error */
			if (setup_regs(immu) != DDI_SUCCESS) {
				immu->immu_regs_setup = B_FALSE;
			} else {
				immu->immu_regs_setup = B_TRUE;
			}
		}
	}
}

/*
 * immu_regs_map()
 */
int
immu_regs_resume(immu_t *immu)
{
	int error;

	/*
	 * remap the register address space
	 */
	error = ddi_regs_map_setup(immu->immu_dip, 0,
	    (caddr_t *)&(immu->immu_regs_addr), (offset_t)0,
	    (offset_t)IMMU_REGSZ, &immu_regs_attr,
	    &(immu->immu_regs_handle));
	if (error != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	immu_regs_set_root_table(immu);

	immu_regs_intr_enable(immu, immu->immu_regs_intr_msi_addr,
	    immu->immu_regs_intr_msi_data, immu->immu_regs_intr_uaddr);

	(void) immu_intr_handler(immu);

	immu_regs_intrmap_enable(immu, immu->immu_intrmap_irta_reg);

	immu_regs_qinv_enable(immu, immu->immu_qinv_reg_value);


	return (error);
}

/*
 * immu_regs_suspend()
 */
void
immu_regs_suspend(immu_t *immu)
{

	immu->immu_intrmap_running = B_FALSE;

	/* Finally, unmap the regs */
	ddi_regs_map_free(&(immu->immu_regs_handle));
}

/*
 * immu_regs_startup()
 *	set a IMMU unit's registers to startup the unit
 */
void
immu_regs_startup(immu_t *immu)
{
	uint32_t status;

	if (immu->immu_regs_setup == B_FALSE) {
		return;
	}

	ASSERT(immu->immu_regs_running == B_FALSE);

	ASSERT(MUTEX_HELD(&(immu->immu_lock)));

	mutex_enter(&(immu->immu_regs_lock));
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_TE);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (status & IMMU_GSTS_TES), status);
	immu->immu_regs_cmdval |= IMMU_GCMD_TE;
	immu->immu_regs_running = B_TRUE;
	mutex_exit(&(immu->immu_regs_lock));

	ddi_err(DER_NOTE, NULL, "IMMU %s running", immu->immu_name);
}

/*
 * immu_regs_shutdown()
 *	shutdown a unit
 */
void
immu_regs_shutdown(immu_t *immu)
{
	uint32_t status;

	if (immu->immu_regs_running == B_FALSE) {
		return;
	}

	ASSERT(immu->immu_regs_setup == B_TRUE);

	ASSERT(MUTEX_HELD(&(immu->immu_lock)));

	mutex_enter(&(immu->immu_regs_lock));
	immu->immu_regs_cmdval &= ~IMMU_GCMD_TE;
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, !(status & IMMU_GSTS_TES), status);
	immu->immu_regs_running = B_FALSE;
	mutex_exit(&(immu->immu_regs_lock));

	ddi_err(DER_NOTE, NULL, "IOMMU %s stopped", immu->immu_name);
}

/*
 * immu_regs_intr()
 *        Set a IMMU unit regs to setup a IMMU unit's
 *        interrupt handler
 */
void
immu_regs_intr_enable(immu_t *immu, uint32_t msi_addr, uint32_t msi_data,
    uint32_t uaddr)
{
	mutex_enter(&(immu->immu_regs_lock));
	immu->immu_regs_intr_msi_addr = msi_addr;
	immu->immu_regs_intr_uaddr = uaddr;
	immu->immu_regs_intr_msi_data = msi_data;
	put_reg32(immu, IMMU_REG_FEVNT_ADDR, msi_addr);
	put_reg32(immu, IMMU_REG_FEVNT_UADDR, uaddr);
	put_reg32(immu, IMMU_REG_FEVNT_DATA, msi_data);
	put_reg32(immu, IMMU_REG_FEVNT_CON, 0);
	mutex_exit(&(immu->immu_regs_lock));
}

/*
 * immu_regs_passthru_supported()
 *       Returns B_TRUE ifi passthru is supported
 */
boolean_t
immu_regs_passthru_supported(immu_t *immu)
{
	if (IMMU_ECAP_GET_PT(immu->immu_regs_excap)) {
		return (B_TRUE);
	}

	ddi_err(DER_WARN, NULL, "Passthru not supported");
	return (B_FALSE);
}

/*
 * immu_regs_is_TM_reserved()
 *       Returns B_TRUE if TM field is reserved
 */
boolean_t
immu_regs_is_TM_reserved(immu_t *immu)
{
	if (IMMU_ECAP_GET_DI(immu->immu_regs_excap) ||
	    IMMU_ECAP_GET_CH(immu->immu_regs_excap)) {
		return (B_FALSE);
	}
	return (B_TRUE);
}

/*
 * immu_regs_is_SNP_reserved()
 *       Returns B_TRUE if SNP field is reserved
 */
boolean_t
immu_regs_is_SNP_reserved(immu_t *immu)
{

	return (IMMU_ECAP_GET_SC(immu->immu_regs_excap) ? B_FALSE : B_TRUE);
}

/*
 * immu_regs_wbf_flush()
 *     If required and supported, write to IMMU
 *     unit's regs to flush DMA write buffer(s)
 */
void
immu_regs_wbf_flush(immu_t *immu)
{
	uint32_t status;

	if (!IMMU_CAP_GET_RWBF(immu->immu_regs_cap)) {
		return;
	}

	mutex_enter(&(immu->immu_regs_lock));
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_WBF);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (!(status & IMMU_GSTS_WBFS)), status);
	mutex_exit(&(immu->immu_regs_lock));
}

/*
 * immu_regs_cpu_flush()
 * 	flush the cpu cache line after CPU memory writes, so
 *      IOMMU can see the writes
 */
void
immu_regs_cpu_flush(immu_t *immu, caddr_t addr, uint_t size)
{
	uint_t i;

	ASSERT(immu);

	if (immu->immu_dvma_coherent == B_TRUE)
		return;

	for (i = 0; i < size; i += x86_clflush_size) {
		clflush_insn(addr+i);
	}

	mfence_insn();
}

void
immu_regs_iotlb_flush(immu_t *immu, uint_t domainid, uint64_t dvma,
    uint64_t count, uint_t hint, immu_iotlb_inv_t type)
{
	ASSERT(immu);

	switch (type) {
	case IOTLB_PSI:
		ASSERT(domainid > 0);
		ASSERT(dvma > 0);
		ASSERT(count > 0);
		iotlb_psi(immu, domainid, dvma, count, hint);
		break;
	case IOTLB_DSI:
		ASSERT(domainid > 0);
		ASSERT(dvma == 0);
		ASSERT(count == 0);
		ASSERT(hint == 0);
		iotlb_dsi(immu, domainid);
		break;
	case IOTLB_GLOBAL:
		ASSERT(domainid == 0);
		ASSERT(dvma == 0);
		ASSERT(count == 0);
		ASSERT(hint == 0);
		iotlb_global(immu);
		break;
	default:
		ddi_err(DER_PANIC, NULL, "invalid IOTLB invalidation type: %d",
		    type);
		/*NOTREACHED*/
	}
}

/*
 * immu_regs_context_flush()
 *   flush the context cache
 */
void
immu_regs_context_flush(immu_t *immu, uint8_t function_mask,
    uint16_t sid, uint_t did, immu_context_inv_t type)
{
	uint64_t command = 0;
	uint64_t status;

	ASSERT(immu);
	ASSERT(rw_write_held(&(immu->immu_ctx_rwlock)));

	/*
	 * define the command
	 */
	switch (type) {
	case CONTEXT_FSI:
		command |= CCMD_INV_ICC | CCMD_INV_DEVICE
		    | CCMD_INV_DID(did)
		    | CCMD_INV_SID(sid) | CCMD_INV_FM(function_mask);
		break;
	case CONTEXT_DSI:
		ASSERT(function_mask == 0);
		ASSERT(sid == 0);
		command |= CCMD_INV_ICC | CCMD_INV_DOMAIN
		    | CCMD_INV_DID(did);
		break;
	case CONTEXT_GLOBAL:
		ASSERT(function_mask == 0);
		ASSERT(sid == 0);
		ASSERT(did == 0);
		command |= CCMD_INV_ICC | CCMD_INV_GLOBAL;
		break;
	default:
		ddi_err(DER_PANIC, NULL,
		    "%s: incorrect context cache flush type",
		    immu->immu_name);
		/*NOTREACHED*/
	}

	mutex_enter(&(immu->immu_regs_lock));
	/* verify there is no pending command */
	wait_completion(immu, IMMU_REG_CONTEXT_CMD, get_reg64,
	    (!(status & CCMD_INV_ICC)), status);
	put_reg64(immu, IMMU_REG_CONTEXT_CMD, command);
	wait_completion(immu, IMMU_REG_CONTEXT_CMD, get_reg64,
	    (!(status & CCMD_INV_ICC)), status);
	mutex_exit(&(immu->immu_regs_lock));
}

void
immu_regs_set_root_table(immu_t *immu)
{
	uint32_t status;

	mutex_enter(&(immu->immu_regs_lock));
	put_reg64(immu, IMMU_REG_ROOTENTRY,
	    immu->immu_ctx_root->hwpg_paddr);
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_SRTP);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (status & IMMU_GSTS_RTPS), status);
	mutex_exit(&(immu->immu_regs_lock));
}


/* enable queued invalidation interface */
void
immu_regs_qinv_enable(immu_t *immu, uint64_t qinv_reg_value)
{
	uint32_t status;

	if (immu_qinv_enable == B_FALSE)
		return;

	mutex_enter(&immu->immu_regs_lock);
	immu->immu_qinv_reg_value = qinv_reg_value;
	/* Initialize the Invalidation Queue Tail register to zero */
	put_reg64(immu, IMMU_REG_INVAL_QT, 0);

	/* set invalidation queue base address register */
	put_reg64(immu, IMMU_REG_INVAL_QAR, qinv_reg_value);

	/* enable queued invalidation interface */
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_QIE);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (status & IMMU_GSTS_QIES), status);
	mutex_exit(&immu->immu_regs_lock);

	immu->immu_regs_cmdval |= IMMU_GCMD_QIE;
	immu->immu_qinv_running = B_TRUE;

}

/* enable interrupt remapping hardware unit */
void
immu_regs_intrmap_enable(immu_t *immu, uint64_t irta_reg)
{
	uint32_t status;

	if (immu_intrmap_enable == B_FALSE)
		return;

	/* set interrupt remap table pointer */
	mutex_enter(&(immu->immu_regs_lock));
	immu->immu_intrmap_irta_reg = irta_reg;
	put_reg64(immu, IMMU_REG_IRTAR, irta_reg);
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_SIRTP);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (status & IMMU_GSTS_IRTPS), status);
	mutex_exit(&(immu->immu_regs_lock));

	/* global flush intr entry cache */
	if (immu_qinv_enable == B_TRUE)
		immu_qinv_intr_global(immu);

	/* enable interrupt remapping */
	mutex_enter(&(immu->immu_regs_lock));
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_IRE);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (status & IMMU_GSTS_IRES),
	    status);
	immu->immu_regs_cmdval |= IMMU_GCMD_IRE;

	/* set compatible mode */
	put_reg32(immu, IMMU_REG_GLOBAL_CMD,
	    immu->immu_regs_cmdval | IMMU_GCMD_CFI);
	wait_completion(immu, IMMU_REG_GLOBAL_STS,
	    get_reg32, (status & IMMU_GSTS_CFIS),
	    status);
	immu->immu_regs_cmdval |= IMMU_GCMD_CFI;
	mutex_exit(&(immu->immu_regs_lock));

	immu->immu_intrmap_running = B_TRUE;
}

uint64_t
immu_regs_get64(immu_t *immu, uint_t reg)
{
	return (get_reg64(immu, reg));
}

uint32_t
immu_regs_get32(immu_t *immu, uint_t reg)
{
	return (get_reg32(immu, reg));
}

void
immu_regs_put64(immu_t *immu, uint_t reg, uint64_t val)
{
	put_reg64(immu, reg, val);
}

void
immu_regs_put32(immu_t *immu, uint_t reg, uint32_t val)
{
	put_reg32(immu, reg, val);
}
