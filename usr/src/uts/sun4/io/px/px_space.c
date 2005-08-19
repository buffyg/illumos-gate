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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * PCI Express nexus driver tunables
 */

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/time.h>
#include <sys/pcie.h>
#include "px_space.h"

/*LINTLIBRARY*/

uint32_t px_spurintr_duration = 60000000; /* One minute */
uint64_t px_spurintr_msgs = PX_SPURINTR_MSG_DEFAULT;

/*
 * The following variable enables a workaround for the following obp bug:
 *
 *	1234181 - obp should set latency timer registers in pci
 *		configuration header
 *
 * Until this bug gets fixed in the obp, the following workaround should
 * be enabled.
 */
uint_t px_set_latency_timer_register = 1;

/*
 * The following driver parameters are defined as variables to allow
 * patching for debugging and tuning.  Flags that can be set on a per
 * PBM basis are bit fields where the PBM device instance number maps
 * to the bit position.
 */
uint_t px_mmu_error_intr_enable = (uint_t)-1;
uint_t px_rerun_disable = 0;

uint_t px_error_intr_enable = (uint_t)-1;
uint_t px_dwsync_disable = 0;
uint_t px_intsync_disable = 0;

uint_t px_intr_retry_intv = 5;		/* for interrupt retry reg */
uint8_t px_latency_timer = 0x40;	/* for pci latency timer reg */
uint_t px_panic_on_fatal_errors = 1;	/* should be 1 at beta */
uint_t px_thermal_intr_fatal = 1;	/* thermal interrupts fatal */
uint_t px_buserr_interrupt = 1;	/* safari buserr interrupt */
uint_t px_ctx_no_active_flush = 0;	/* cannot handle active ctx flush */
uint_t px_use_contexts = 1;

hrtime_t px_intrpend_timeout = 5ull * NANOSEC;	/* 5 seconds in nanoseconds */

uint64_t px_perr_fatal = -1ull;
uint64_t px_serr_fatal = -1ull;
uint64_t px_errtrig_pa = 0x0;

/*
 * The following flag controls behavior of the ino handler routine
 * when multiple interrupts are attached to a single ino.  Typically
 * this case would occur for the ino's assigned to the PCI bus slots
 * with multi-function devices or bus bridges.
 *
 * Setting the flag to zero causes the ino handler routine to return
 * after finding the first interrupt handler to claim the interrupt.
 *
 * Setting the flag to non-zero causes the ino handler routine to
 * return after making one complete pass through the interrupt
 * handlers.
 */
uint_t px_check_all_handlers = 1;

/*
 * The following value is the number of consecutive unclaimed interrupts that
 * will be tolerated for a particular ino_p before the interrupt is deemed to
 * be jabbering and is blocked.
 */
uint_t px_unclaimed_intr_max = 20;

/*
 * The following value will cause the nexus driver to block an ino after
 * px_unclaimed_intr_max unclaimed interrupts have been seen.  Setting this
 * value to 0 will cause interrupts to never be blocked, no matter how many
 * unclaimed interrupts are seen on a particular ino.
 */
uint_t px_unclaimed_intr_block = 1;

uint_t px_lock_tlb = 0;

uint64_t px_dvma_debug_on = 0;
uint64_t px_dvma_debug_off = 0;
uint32_t px_dvma_debug_rec = 512;

/*
 * dvma address space allocation cache variables
 */
uint_t px_dvma_page_cache_entries = 0x200;	/* # of chunks (1 << bits) */
uint_t px_dvma_page_cache_clustsz = 0x8;	/* # of pages per chunk */
#ifdef PX_DMA_PROF
uint_t px_dvmaft_npages = 0;			/* FT fail due npages */
uint_t px_dvmaft_limit = 0;			/* FT fail due limits */
uint_t px_dvmaft_free = 0;			/* FT free */
uint_t px_dvmaft_success = 0;			/* FT success */
uint_t px_dvmaft_exhaust = 0;			/* FT vmem fallback */
uint_t px_dvma_vmem_alloc = 0;			/* vmem alloc */
uint_t px_dvma_vmem_xalloc = 0;		/* vmem xalloc */
uint_t px_dvma_vmem_xfree = 0;			/* vmem xfree */
uint_t px_dvma_vmem_free = 0;			/* vmem free */
#endif
uint_t px_disable_fdvma = 0;
uint_t px_mmu_ctx_lock_failure = 0;

/*
 * This flag preserves prom MMU settings by copying prom TSB entries
 * to corresponding kernel TSB entry locations. It should be removed
 * after the interface properties from obp have become default.
 */
uint_t px_preserve_mmu_tsb = 1;

/*
 * memory callback list id callback list for kmem_alloc failure clients
 */
uintptr_t px_kmem_clid = 0;

uint64_t px_tlu_ue_intr_mask	= PX_ERR_EN_ALL;
uint64_t px_tlu_ue_log_mask	= PX_ERR_EN_ALL;
uint64_t px_tlu_ue_count_mask	= PX_ERR_EN_ALL;

uint64_t px_tlu_ce_intr_mask	= PX_ERR_MASK_NONE;
uint64_t px_tlu_ce_log_mask	= PX_ERR_MASK_NONE;
uint64_t px_tlu_ce_count_mask	= PX_ERR_MASK_NONE;

/*
 * Do not enable Link Interrupts
 */
uint64_t px_tlu_oe_intr_mask	= PX_ERR_EN_ALL & ~0x80000000800;
uint64_t px_tlu_oe_log_mask	= PX_ERR_EN_ALL & ~0x80000000800;
uint64_t px_tlu_oe_count_mask	= PX_ERR_EN_ALL;

uint64_t px_mmu_intr_mask	= PX_ERR_EN_ALL;
uint64_t px_mmu_log_mask	= PX_ERR_EN_ALL;
uint64_t px_mmu_count_mask	= PX_ERR_EN_ALL;

uint64_t px_imu_intr_mask	= PX_ERR_EN_ALL;
uint64_t px_imu_log_mask	= PX_ERR_EN_ALL;
uint64_t px_imu_count_mask	= PX_ERR_EN_ALL;

/*
 * (1ull << ILU_INTERRUPT_ENABLE_IHB_PE_S) |
 * (1ull << ILU_INTERRUPT_ENABLE_IHB_PE_P);
 */
uint64_t px_ilu_intr_mask	= (((uint64_t)0x10 << 32) | 0x10);
uint64_t px_ilu_log_mask	= (((uint64_t)0x10 << 32) | 0x10);
uint64_t px_ilu_count_mask	= PX_ERR_EN_ALL;

uint64_t px_cb_intr_mask	= PX_ERR_EN_ALL;
uint64_t px_cb_log_mask		= PX_ERR_EN_ALL;
uint64_t px_cb_count_mask	= PX_ERR_EN_ALL;

/*
 * LPU Intr Registers are reverse encoding from the registers above.
 * 1 = disable
 * 0 = enable
 *
 * Log and Count are however still the same.
 */
uint64_t px_lpul_intr_mask	= LPU_INTR_DISABLE;
uint64_t px_lpul_log_mask	= PX_ERR_EN_ALL;
uint64_t px_lpul_count_mask	= PX_ERR_EN_ALL;

uint64_t px_lpup_intr_mask	= LPU_INTR_DISABLE;
uint64_t px_lpup_log_mask	= PX_ERR_EN_ALL;
uint64_t px_lpup_count_mask	= PX_ERR_EN_ALL;

uint64_t px_lpur_intr_mask	= LPU_INTR_DISABLE;
uint64_t px_lpur_log_mask	= PX_ERR_EN_ALL;
uint64_t px_lpur_count_mask	= PX_ERR_EN_ALL;

uint64_t px_lpux_intr_mask	= LPU_INTR_DISABLE;
uint64_t px_lpux_log_mask	= PX_ERR_EN_ALL;
uint64_t px_lpux_count_mask	= PX_ERR_EN_ALL;

uint64_t px_lpus_intr_mask	= LPU_INTR_DISABLE;
uint64_t px_lpus_log_mask	= PX_ERR_EN_ALL;
uint64_t px_lpus_count_mask	= PX_ERR_EN_ALL;

uint64_t px_lpug_intr_mask	= LPU_INTR_DISABLE;
uint64_t px_lpug_log_mask	= PX_ERR_EN_ALL;
uint64_t px_lpug_count_mask	= PX_ERR_EN_ALL;

/* timeout in micro seconds for receiving PME_To_ACK */
uint64_t px_pme_to_ack_timeout	= PX_PME_TO_ACK_TIMEOUT;

/* timeout in micro seconds for receiving link up interrupt */
uint64_t px_linkup_timeout = PX_LINKUP_TIMEOUT;

/* PIL at which PME_To_ACK message interrupt is handled */
uint32_t px_pwr_pil		= PX_PWR_PIL;

uint32_t px_max_l1_tries	= PX_MAX_L1_TRIES;

/* Fire PCIe Error that should cause panics */
uint32_t px_fabric_die = 1;

uint32_t px_fabric_die_rc_ce = 0;
uint32_t px_fabric_die_rc_ue = PCIE_AER_UCE_UR |
    PCIE_AER_UCE_TO |
    PCIE_AER_UCE_RO |
    PCIE_AER_UCE_FCP |
    PCIE_AER_UCE_DLP;

/* Fire PCIe Error that should cause panics even under protected access */
uint32_t px_fabric_die_rc_ce_gos = 0;
uint32_t px_fabric_die_rc_ue_gos = PCIE_AER_UCE_RO |
    PCIE_AER_UCE_FCP |
    PCIE_AER_UCE_DLP;

/* Fabric Error that should cause panics */
uint32_t px_fabric_die_ce = 0;
uint32_t px_fabric_die_ue = PCIE_AER_UCE_UR |
    PCIE_AER_UCE_UC |
    PCIE_AER_UCE_TO |
    PCIE_AER_UCE_RO |
    PCIE_AER_UCE_FCP |
    PCIE_AER_UCE_DLP |
    PCIE_AER_UCE_TRAINING;

/* Fabric Error that should cause panics even under protected access */
uint32_t px_fabric_die_ce_gos = 0;
uint32_t px_fabric_die_ue_gos = PCIE_AER_UCE_UC |
    PCIE_AER_UCE_TO |
    PCIE_AER_UCE_RO |
    PCIE_AER_UCE_FCP |
    PCIE_AER_UCE_DLP |
    PCIE_AER_UCE_TRAINING;

/* Fabric Bridge Sec. Error that should cause panics */
uint16_t px_fabric_die_bdg_sts = PCI_STAT_S_PERROR |
    PCI_STAT_R_TARG_AB |
    PCI_STAT_R_MAST_AB |
    PCI_STAT_S_SYSERR |
    PCI_STAT_PERROR;

/*
 * Fabric Bridge Sec. Error that should cause panics even under
 * protected access
 */
uint16_t px_fabric_die_bdg_sts_gos = PCI_STAT_S_PERROR |
    PCI_STAT_PERROR;

/* Fabric Switch Sec. Error that should cause panics */
uint16_t px_fabric_die_sw_sts = PCI_STAT_R_TARG_AB |
    PCI_STAT_R_MAST_AB |
    PCI_STAT_S_SYSERR;

/*
 * Fabric Switch Sec. Error that should cause panics even under
 * protected access
 */
uint16_t px_fabric_die_sw_sts_gos = 0;

uint32_t px_fabric_die_sue = PCIE_AER_SUCE_TA_ON_SC |
    PCIE_AER_SUCE_MA_ON_SC |
    PCIE_AER_SUCE_RCVD_TA |
    PCIE_AER_SUCE_RCVD_MA |
    PCIE_AER_SUCE_USC_ERR |
    PCIE_AER_SUCE_USC_MSG_DATA_ERR |
    PCIE_AER_SUCE_UC_DATA_ERR |
    PCIE_AER_SUCE_UC_ATTR_ERR |
    PCIE_AER_SUCE_UC_ADDR_ERR |
    PCIE_AER_SUCE_TIMER_EXPIRED |
    PCIE_AER_SUCE_PERR_ASSERT |
    PCIE_AER_SUCE_SERR_ASSERT |
    PCIE_AER_SUCE_INTERNAL_ERR;

uint32_t px_fabric_die_sue_gos = PCIE_AER_SUCE_TA_ON_SC |
    PCIE_AER_SUCE_MA_ON_SC |
    PCIE_AER_SUCE_USC_ERR |
    PCIE_AER_SUCE_USC_MSG_DATA_ERR |
    PCIE_AER_SUCE_UC_DATA_ERR |
    PCIE_AER_SUCE_UC_ATTR_ERR |
    PCIE_AER_SUCE_UC_ADDR_ERR |
    PCIE_AER_SUCE_TIMER_EXPIRED |
    PCIE_AER_SUCE_PERR_ASSERT |
    PCIE_AER_SUCE_SERR_ASSERT |
    PCIE_AER_SUCE_INTERNAL_ERR;
