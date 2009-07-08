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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * px_msiq.c
 */

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/machsystm.h>	/* intr_dist_add */
#include <sys/modctl.h>
#include <sys/disp.h>
#include <sys/stat.h>
#include <sys/ddi_impldefs.h>
#include "px_obj.h"

static void px_msiq_get_props(px_t *px_p);

/*
 * px_msiq_attach()
 */
int
px_msiq_attach(px_t *px_p)
{
	px_ib_t		*ib_p = px_p->px_ib_p;
	px_msiq_state_t	*msiq_state_p = &ib_p->ib_msiq_state;
	int		i, ret = DDI_SUCCESS;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_attach\n");

	/*
	 * Check for all MSIQ related properties and
	 * save all information.
	 *
	 * Avaialble MSIQs and its properties.
	 */
	px_msiq_get_props(px_p);

	/*
	 * 10% of available MSIQs are reserved for the PCIe messages.
	 * Around 90% of available MSIQs are reserved for the MSI/Xs.
	 */
	msiq_state_p->msiq_msg_qcnt = howmany(msiq_state_p->msiq_cnt, 10);
	msiq_state_p->msiq_msi_qcnt = msiq_state_p->msiq_cnt -
	    msiq_state_p->msiq_msg_qcnt;

	msiq_state_p->msiq_1st_msi_qid = msiq_state_p->msiq_1st_msiq_id;
	msiq_state_p->msiq_1st_msg_qid = msiq_state_p->msiq_1st_msiq_id +
	    msiq_state_p->msiq_msi_qcnt;

	mutex_init(&msiq_state_p->msiq_mutex, NULL, MUTEX_DRIVER, NULL);
	msiq_state_p->msiq_p = kmem_zalloc(msiq_state_p->msiq_cnt *
	    sizeof (px_msiq_t), KM_SLEEP);

	for (i = 0; i < msiq_state_p->msiq_cnt; i++) {
		msiq_state_p->msiq_p[i].msiq_id =
		    msiq_state_p->msiq_1st_msiq_id + i;
		msiq_state_p->msiq_p[i].msiq_refcnt = 0;
		msiq_state_p->msiq_p[i].msiq_state = MSIQ_STATE_FREE;
		(void) px_ib_alloc_ino(ib_p, px_msiqid_to_devino(px_p,
		    msiq_state_p->msiq_p[i].msiq_id));
	}

	if ((ret = px_lib_msiq_init(px_p->px_dip)) != DDI_SUCCESS)
		px_msiq_detach(px_p);

	msiq_state_p->msiq_redist_flag = B_TRUE;
	return (ret);
}

/*
 * px_msiq_detach()
 */
void
px_msiq_detach(px_t *px_p)
{
	px_msiq_state_t	*msiq_state_p = &px_p->px_ib_p->ib_msiq_state;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_detach\n");

	if (px_lib_msiq_fini(px_p->px_dip) != DDI_SUCCESS) {
		DBG(DBG_MSIQ, px_p->px_dip,
		    "px_lib_msiq_fini: failed\n");
	}

	mutex_destroy(&msiq_state_p->msiq_mutex);
	kmem_free(msiq_state_p->msiq_p,
	    msiq_state_p->msiq_cnt * sizeof (px_msiq_t));

	bzero(msiq_state_p, sizeof (px_msiq_state_t));
}

/*
 * px_msiq_resume()
 */
void
px_msiq_resume(px_t *px_p)
{
	px_msiq_state_t	*msiq_state_p = &px_p->px_ib_p->ib_msiq_state;
	int		i;

	for (i = 0; i < msiq_state_p->msiq_cnt; i++) {
		(void) px_lib_msiq_gethead(px_p->px_dip,
		    msiq_state_p->msiq_p[i].msiq_id,
		    &msiq_state_p->msiq_p[i].msiq_curr_head_index);
		msiq_state_p->msiq_p[i].msiq_new_head_index = 0;
		msiq_state_p->msiq_p[i].msiq_recs2process = 0;
	}
}

/*
 * px_msiq_alloc()
 */
int
px_msiq_alloc(px_t *px_p, msiq_rec_type_t rec_type, msiqid_t *msiq_id_p)
{
	px_ib_t		*ib_p = px_p->px_ib_p;
	px_msiq_state_t	*msiq_state_p = &ib_p->ib_msiq_state;
	msiqid_t	first_msiq_id;
	uint_t		msiq_cnt;
	ushort_t	least_refcnt;
	int		i;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_alloc\n");

	ASSERT(MUTEX_HELD(&ib_p->ib_ino_lst_mutex));
	mutex_enter(&msiq_state_p->msiq_mutex);

	if (rec_type == MSG_REC) {
		msiq_cnt = msiq_state_p->msiq_msg_qcnt;
		first_msiq_id = msiq_state_p->msiq_1st_msg_qid;
	} else {
		msiq_cnt = msiq_state_p->msiq_msi_qcnt;
		first_msiq_id = msiq_state_p->msiq_1st_msi_qid;
	}

	*msiq_id_p = first_msiq_id;
	least_refcnt = msiq_state_p->msiq_p[first_msiq_id].msiq_refcnt;

	/* Allocate MSIQs */
	for (i = first_msiq_id; i < (first_msiq_id + msiq_cnt); i++) {
		if (msiq_state_p->msiq_p[i].msiq_state == MSIQ_STATE_FREE) {
			msiq_state_p->msiq_p[i].msiq_state = MSIQ_STATE_INUSE;
			(void) px_lib_msiq_gethead(px_p->px_dip, i,
			    &msiq_state_p->msiq_p[i].msiq_curr_head_index);
			*msiq_id_p = msiq_state_p->msiq_p[i].msiq_id;
			break;
		}

		if (least_refcnt > msiq_state_p->msiq_p[i].msiq_refcnt) {
			*msiq_id_p = msiq_state_p->msiq_p[i].msiq_id;
			least_refcnt = msiq_state_p->msiq_p[i].msiq_refcnt;
		}
	}

	msiq_state_p->msiq_p[*msiq_id_p].msiq_refcnt++;

	DBG(DBG_MSIQ, px_p->px_dip,
	    "px_msiq_alloc: msiq_id 0x%x\n", *msiq_id_p);

	mutex_exit(&msiq_state_p->msiq_mutex);
	return (DDI_SUCCESS);
}

/*
 * px_msiq_alloc_based_on_cpuid()
 */
int
px_msiq_alloc_based_on_cpuid(px_t *px_p, msiq_rec_type_t rec_type,
    cpuid_t cpuid, msiqid_t *msiq_id_p)
{
	px_ib_t		*ib_p = px_p->px_ib_p;
	px_msiq_state_t	*msiq_state_p = &ib_p->ib_msiq_state;
	msiqid_t	first_msiq_id, free_msiq_id;
	uint_t		msiq_cnt;
	ushort_t	least_refcnt;
	px_ino_t	*ino_p;
	int		i;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_alloc_based_on_cpuid: "
	    "cpuid 0x%x\n", cpuid);

	ASSERT(MUTEX_HELD(&ib_p->ib_ino_lst_mutex));

	mutex_enter(&msiq_state_p->msiq_mutex);

	if (rec_type == MSG_REC) {
		msiq_cnt = msiq_state_p->msiq_msg_qcnt;
		first_msiq_id = msiq_state_p->msiq_1st_msg_qid;
	} else {
		msiq_cnt = msiq_state_p->msiq_msi_qcnt;
		first_msiq_id = msiq_state_p->msiq_1st_msi_qid;
	}

	*msiq_id_p = free_msiq_id = (msiqid_t)-1;
	least_refcnt = (ushort_t)-1;

	/* Allocate MSIQs */
	for (i = first_msiq_id; i < (first_msiq_id + msiq_cnt); i++) {
		ino_p = px_ib_locate_ino(ib_p, px_msiqid_to_devino(px_p, i));

		if ((ino_p->ino_cpuid == cpuid) &&
		    (least_refcnt > msiq_state_p->msiq_p[i].msiq_refcnt)) {
			*msiq_id_p = msiq_state_p->msiq_p[i].msiq_id;
			least_refcnt = msiq_state_p->msiq_p[i].msiq_refcnt;
		}

		if ((*msiq_id_p == -1) && (free_msiq_id == -1) &&
		    (msiq_state_p->msiq_p[i].msiq_state == MSIQ_STATE_FREE))
			free_msiq_id = msiq_state_p->msiq_p[i].msiq_id;
	}

	if (*msiq_id_p == -1) {
		if (free_msiq_id == -1) {
			DBG(DBG_MSIQ, px_p->px_dip,
			    "px_msiq_alloc_based_on_cpuid: No EQ is available "
			    "for CPU 0x%x\n", cpuid);

			mutex_exit(&msiq_state_p->msiq_mutex);
			return (DDI_EINVAL);
		}

		*msiq_id_p = free_msiq_id;
		ino_p = px_ib_locate_ino(ib_p,
		    px_msiqid_to_devino(px_p, *msiq_id_p));
		ino_p->ino_cpuid = ino_p->ino_default_cpuid = cpuid;
	}

	if (msiq_state_p->msiq_p[*msiq_id_p].msiq_state == MSIQ_STATE_FREE) {
		msiq_state_p->msiq_p[*msiq_id_p].msiq_state = MSIQ_STATE_INUSE;
		(void) px_lib_msiq_gethead(px_p->px_dip, *msiq_id_p,
		    &msiq_state_p->msiq_p[*msiq_id_p].msiq_curr_head_index);
	}

	msiq_state_p->msiq_p[*msiq_id_p].msiq_refcnt++;

	DBG(DBG_MSIQ, px_p->px_dip,
	    "px_msiq_alloc_based_on_cpuid: msiq_id 0x%x\n", *msiq_id_p);

	mutex_exit(&msiq_state_p->msiq_mutex);
	return (DDI_SUCCESS);
}

/*
 * px_msiq_free()
 */
int
px_msiq_free(px_t *px_p, msiqid_t msiq_id)
{
	px_ib_t		*ib_p = px_p->px_ib_p;
	px_msiq_state_t	*msiq_state_p = &ib_p->ib_msiq_state;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_free: msiq_id 0x%x", msiq_id);

	ASSERT(MUTEX_HELD(&ib_p->ib_ino_lst_mutex));
	mutex_enter(&msiq_state_p->msiq_mutex);

	if ((msiq_id < msiq_state_p->msiq_1st_msiq_id) || (msiq_id >=
	    (msiq_state_p->msiq_1st_msiq_id + msiq_state_p->msiq_cnt))) {
		DBG(DBG_MSIQ, px_p->px_dip,
		    "px_msiq_free: Invalid msiq_id 0x%x", msiq_id);

		mutex_exit(&msiq_state_p->msiq_mutex);
		return (DDI_FAILURE);
	}

	if (--msiq_state_p->msiq_p[msiq_id].msiq_refcnt == 0)
		msiq_state_p->msiq_p[msiq_id].msiq_state = MSIQ_STATE_FREE;

	mutex_exit(&msiq_state_p->msiq_mutex);
	return (DDI_SUCCESS);
}

/*
 * px_msiq_redist()
 */
void
px_msiq_redist(px_t *px_p)
{
	px_ib_t		*ib_p = px_p->px_ib_p;
	px_msiq_state_t	*msiq_state_p = &ib_p->ib_msiq_state;
	px_ino_t	*ino_p;
	int		i;

	ASSERT(MUTEX_HELD(&ib_p->ib_ino_lst_mutex));

	mutex_enter(&msiq_state_p->msiq_mutex);

	if (msiq_state_p->msiq_redist_flag == B_FALSE) {
		mutex_exit(&msiq_state_p->msiq_mutex);
		return;
	}

	for (i = 0; i < msiq_state_p->msiq_cnt; i++) {
		ino_p = px_ib_locate_ino(ib_p,
		    px_msiqid_to_devino(px_p, msiq_state_p->msiq_p[i].msiq_id));

		if (ino_p) {
			ino_p->ino_cpuid = ino_p->ino_default_cpuid =
			    intr_dist_cpuid();

			DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_redist: "
			    "sysino 0x%llx current cpuid 0x%x "
			    "default cpuid 0x%x\n", ino_p->ino_sysino,
			    ino_p->ino_cpuid, ino_p->ino_default_cpuid);
		}
	}

	msiq_state_p->msiq_redist_flag = B_FALSE;
	mutex_exit(&msiq_state_p->msiq_mutex);
}

/*
 * px_msiqid_to_devino()
 */
devino_t
px_msiqid_to_devino(px_t *px_p, msiqid_t msiq_id)
{
	px_msiq_state_t	*msiq_state_p = &px_p->px_ib_p->ib_msiq_state;
	devino_t	devino;

	devino = msiq_state_p->msiq_1st_devino +
	    msiq_id - msiq_state_p->msiq_1st_msiq_id;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiqid_to_devino: "
	    "msiq_id 0x%x devino 0x%x\n", msiq_id, devino);

	return (devino);
}

/*
 * px_devino_to_msiqid()
 */
msiqid_t
px_devino_to_msiqid(px_t *px_p, devino_t devino)
{
	px_msiq_state_t	*msiq_state_p = &px_p->px_ib_p->ib_msiq_state;
	msiqid_t	msiq_id;

	msiq_id = msiq_state_p->msiq_1st_msiq_id +
	    devino - msiq_state_p->msiq_1st_devino;

	DBG(DBG_MSIQ, px_p->px_dip, "px_devino_to_msiq: "
	    "devino 0x%x msiq_id 0x%x\n", devino, msiq_id);

	return (msiq_id);
}

/*
 * px_msiq_get_props()
 */
static void
px_msiq_get_props(px_t *px_p)
{
	px_msiq_state_t *msiq_state_p = &px_p->px_ib_p->ib_msiq_state;
	int	ret = DDI_SUCCESS;
	int	length = sizeof (int);
	char	*valuep = NULL;

	DBG(DBG_MSIQ, px_p->px_dip, "px_msiq_get_props\n");

	/* #msi-eqs */
	msiq_state_p->msiq_cnt = ddi_getprop(DDI_DEV_T_ANY, px_p->px_dip,
	    DDI_PROP_DONTPASS, "#msi-eqs", PX_DEFAULT_MSIQ_CNT);

	DBG(DBG_MSIQ, px_p->px_dip, "obp: msiq_cnt=%d\n",
	    msiq_state_p->msiq_cnt);

	/* msi-eq-size */
	msiq_state_p->msiq_rec_cnt = ddi_getprop(DDI_DEV_T_ANY, px_p->px_dip,
	    DDI_PROP_DONTPASS, "msi-eq-size", PX_DEFAULT_MSIQ_REC_CNT);

	DBG(DBG_MSIQ, px_p->px_dip, "obp: msiq_rec_cnt=%d\n",
	    msiq_state_p->msiq_rec_cnt);

	/* msi-eq-to-devino: msi-eq#, devino# fields */
	ret = ddi_prop_op(DDI_DEV_T_ANY, px_p->px_dip, PROP_LEN_AND_VAL_ALLOC,
	    DDI_PROP_DONTPASS, "msi-eq-to-devino", (caddr_t)&valuep,
	    &length);

	if (ret == DDI_PROP_SUCCESS) {
		msiq_state_p->msiq_1st_msiq_id =
		    ((px_msi_eq_to_devino_t *)valuep)->msi_eq_no;
		msiq_state_p->msiq_1st_devino =
		    ((px_msi_eq_to_devino_t *)valuep)->devino_no;
		kmem_free(valuep, (size_t)length);
	} else {
		msiq_state_p->msiq_1st_msiq_id = PX_DEFAULT_MSIQ_1ST_MSIQ_ID;
		msiq_state_p->msiq_1st_devino = PX_DEFAULT_MSIQ_1ST_DEVINO;
	}

	DBG(DBG_MSIQ, px_p->px_dip, "obp: msiq_1st_msiq_id=%d\n",
	    msiq_state_p->msiq_1st_msiq_id);

	DBG(DBG_MSIQ, px_p->px_dip, "obp: msiq_1st_devino=%d\n",
	    msiq_state_p->msiq_1st_devino);
}
