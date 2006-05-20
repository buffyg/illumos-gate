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

/*
 * Sun Fire X4500 Disk Diagnosis Engine
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <libnvpair.h>
#include <fm/fmd_api.h>
#include <fm/fmd_fmri.h>
#include <sys/fm/protocol.h>
#include <fm/libtopo.h>

#include "sfx4500-disk.h"
#include "fault_mgr.h"
#include "hotplug_mgr.h"
#include "schg_mgr.h"
#include "plugin_mgr.h"
#include "fm_disk_events.h"
#include "topo_gather.h"

#define	THIS_FMD_MODULE_NAME "sfx4500-disk"

static enum sfx4500_init_state {
	INIT_STATE_NONE = 0,
	PLUGIN_MGR_INITTED = 1,
	STATE_CHANGE_MGR_INITTED = 2,
	HOTPLUG_MGR_INITTED = 4,
	FAULT_MGR_INITTED = 8
} g_init_state = INIT_STATE_NONE;

typedef enum {
	LT_SUSPECT,
	LT_REPAIRED
} fm_list_type_t;

/*
 * Global verbosity flag -- controls chattiness of debug messages and
 * warnings.  Its value is determined by the fmd property "log-level"
 * settable in the DE's .conf file.
 */
log_class_t			g_verbose = 0;
cfgdata_t			*config_data = NULL;
fmd_xprt_t			*g_xprt_hdl = NULL;
fmd_hdl_t			*g_fm_hdl = NULL;

static const fmd_prop_t		fmd_props[];

static void
diskmon_teardown_all(void)
{
	/*
	 * Cleanup the fault manager first -- it depends
	 * on the state change manager and may race with
	 * its cleanup if left around.
	 */
	cleanup_fault_manager(config_data);
	cleanup_hotplug_manager();
	cleanup_state_change_manager(config_data);
	cleanup_plugin_manager();
	config_fini();
}

static int
count_disks(diskmon_t *disklistp)
{
	int i = 0;

	while (disklistp != NULL) {
		i++;
		disklistp = disklistp->next;
	}

	return (i);
}

static int
diskmon_init(void)
{
	if (init_plugin_manager() != 0)
		goto cleanup;
	else
		g_init_state |= PLUGIN_MGR_INITTED;

	block_state_change_events();

	if (init_hotplug_manager() != 0)
		goto cleanup;
	else
		g_init_state |= HOTPLUG_MGR_INITTED;

	if (init_state_change_manager(config_data) != 0)
		goto cleanup;
	else
		g_init_state |= STATE_CHANGE_MGR_INITTED;

	if (init_fault_manager(config_data) != 0)
		goto cleanup;
	else
		g_init_state |= FAULT_MGR_INITTED;

	return (E_SUCCESS);

cleanup:

	unblock_state_change_events();

	/*
	 * The cleanup order here does matter, due to dependencies between the
	 * managers.
	 */
	if (g_init_state & FAULT_MGR_INITTED)
		cleanup_fault_manager(config_data);
	if (g_init_state & HOTPLUG_MGR_INITTED)
		cleanup_hotplug_manager();
	if (g_init_state & STATE_CHANGE_MGR_INITTED)
		cleanup_state_change_manager(config_data);
	if (g_init_state & PLUGIN_MGR_INITTED)
		cleanup_plugin_manager();

	return (E_ERROR);
}

static boolean_t
dm_suspect_de_is_me(nvlist_t *suspectnvl)
{
	nvlist_t *de_nvl;
	char *modname;

	if (nvlist_lookup_nvlist(suspectnvl, FM_SUSPECT_DE, &de_nvl) != 0 ||
	    nvlist_lookup_string(de_nvl, FM_FMRI_FMD_NAME, &modname) != 0 ||
	    strcmp(modname, THIS_FMD_MODULE_NAME) != 0)
		return (B_FALSE);
	else
		return (B_TRUE);
}

static disk_flt_src_e
nvl2fltsrc(fmd_hdl_t *hdl, nvlist_t *nvl)
{
	if (fmd_nvl_class_match(hdl, nvl, FAULT_DISK_OVERTEMP))
		return (DISK_FAULT_SOURCE_OVERTEMP);

	if (fmd_nvl_class_match(hdl, nvl, FAULT_DISK_STFAIL))
		return (DISK_FAULT_SOURCE_SELFTEST);

	if (fmd_nvl_class_match(hdl, nvl, FAULT_DISK_PREDFAIL))
		return (DISK_FAULT_SOURCE_INFO_EXCPT);

	return (DISK_FAULT_SOURCE_NONE);
}

static void
dm_fault_execute_actions(diskmon_t *diskp, disk_flt_src_e fsrc)
{
	const char		*action_prop = NULL;
	const char		*action_string;
	dm_plugin_error_t	rv;

	/*
	 * The predictive failure action is the activation of the fault
	 * indicator.
	 */
	switch (fsrc) {
	case DISK_FAULT_SOURCE_OVERTEMP:

		action_prop = DISK_PROP_OTEMPACTION;
		break;

	case DISK_FAULT_SOURCE_SELFTEST:

		action_prop = DISK_PROP_STFAILACTION;
		break;
	}

	dm_fault_indicator_set(diskp, INDICATOR_ON);

	if (action_prop != NULL &&
	    (action_string = dm_prop_lookup(diskp->props, action_prop))
	    != NULL) {

		rv = dm_pm_indicator_execute(action_string);
		if (rv != DMPE_SUCCESS) {
			log_warn("Fault action `%s' did not successfully "
			    "complete.\n", action_string);
		}
	}
}

static void
diskmon_agent(fmd_hdl_t *hdl, nvlist_t *nvl, fm_list_type_t list_type)
{
	char		*uuid = NULL;
	nvlist_t	**nva;
	uint_t		nvc;
	diskmon_t	*diskp;
	nvlist_t	*fmri;
	nvlist_t	*fltnvl;
	int		err = 0;

	switch (list_type) {

	case LT_REPAIRED:

		err |= nvlist_lookup_string(nvl, FM_SUSPECT_UUID, &uuid);
		err |= nvlist_lookup_nvlist_array(nvl, FM_SUSPECT_FAULT_LIST,
		    &nva, &nvc);
		if (err != 0)
			return;

		while (nvc-- != 0) {

			fltnvl = *nva++;

			if (nvlist_lookup_nvlist(fltnvl, FM_FAULT_RESOURCE,
			    &fmri) != 0)
				continue;

			if ((diskp = dm_fmri_to_diskmon(hdl, fmri)) == NULL)
				continue;

			log_msg(MM_MAIN, "Disk %s repaired!\n",
			    diskp->location);

			dm_fault_indicator_set(diskp, INDICATOR_OFF);

			dm_state_change(diskp, HPS_REPAIRED);
		}

		break;

	case LT_SUSPECT:


		err |= nvlist_lookup_string(nvl, FM_SUSPECT_UUID, &uuid);
		err |= nvlist_lookup_nvlist_array(nvl, FM_SUSPECT_FAULT_LIST,
		    &nva, &nvc);
		if (err != 0)
			return;

		while (nvc-- != 0 && !fmd_case_uuclosed(hdl, uuid)) {

			fltnvl = *nva++;

			if (nvlist_lookup_nvlist(fltnvl, FM_FAULT_RESOURCE,
			    &fmri) != 0)
				continue;

			if ((diskp = dm_fmri_to_diskmon(hdl, fmri)) == NULL)
				continue;

			/* Execute the actions associated with this fault */
			dm_fault_execute_actions(diskp,
			    nvl2fltsrc(hdl, fltnvl));

			/*
			 * If the fault wasn't generated by this module, send a
			 * state change event to the state change manager
			 */
			if (!dm_suspect_de_is_me(nvl))
				dm_state_change(diskp, HPS_FAULTED);

			/* Case is closed */
			fmd_case_uuclose(hdl, uuid);
		}

		break;
	}
}

/*ARGSUSED*/
static void
diskmon_recv(fmd_hdl_t *hdl, fmd_event_t *ep, nvlist_t *nvl, const char *class)
{
	diskmon_t	*diskp;
	nvlist_t	*fmri;
	nvlist_t	*fltnvl;
	boolean_t	ismyereport;
	nvlist_t	*det_nvl;
	fmd_case_t	*cs;
	const char	*fltclass;
	disk_flt_src_e	fsrc;

	if (g_verbose & MM_MAIN)
		nvlist_print(stderr, nvl);

	ismyereport = fmd_nvl_class_match(hdl, nvl, EREPORT_SATA ".*");

	/*
	 * Act on the fault suspect list or repaired list (embedded agent
	 * action).
	 */
	if (fmd_nvl_class_match(hdl, nvl, "list.repaired")) {

		diskmon_agent(hdl, nvl, LT_REPAIRED);
		return;

	} else if (fmd_nvl_class_match(hdl, nvl, "list.suspect")) {

		diskmon_agent(hdl, nvl, LT_SUSPECT);
		return;
	}

	/*
	 * If we get any replayed faults, set the diskmon's faulted
	 * flag for the appropriate fault, then change the diskmon's state
	 * to faulted.
	 */
	if (fmd_nvl_class_match(hdl, nvl, FAULT_DISK ".*")) {

		if (nvlist_lookup_nvlist(nvl, FM_FAULT_RESOURCE,
		    &fmri) != 0)
			return;

		if ((diskp = dm_fmri_to_diskmon(hdl, fmri)) == NULL)
			return;

		fsrc = nvl2fltsrc(hdl, nvl);

		/* Execute the actions associated with this fault */
		dm_fault_execute_actions(diskp, fsrc);

		assert(pthread_mutex_lock(&diskp->disk_faults_mutex) == 0);
		diskp->disk_faults |= fsrc;
		assert(pthread_mutex_unlock(&diskp->disk_faults_mutex) == 0);

		/*
		 * If the fault wasn't generated by this module, send a
		 * state change event to the state change manager
		 */
		dm_state_change(diskp, HPS_FAULTED);
		return;
	}

	if (!ismyereport) {
		log_msg(MM_MAIN, "Unknown class = %s\n", class);
		return;
	}

	if (nvlist_lookup_nvlist(nvl, FM_EREPORT_DETECTOR, &det_nvl) != 0) {
		log_msg(MM_MAIN, "Cannot handle event %s: Couldn't get "
		    FM_EREPORT_DETECTOR "\n", class);
		return;
	}

	if ((diskp = dm_fmri_to_diskmon(hdl, det_nvl)) == NULL)
		return;

	fltclass =
	    fmd_nvl_class_match(hdl, nvl, EREPORT_SATA_PREDFAIL) ?
		FAULT_DISK_PREDFAIL :
	    fmd_nvl_class_match(hdl, nvl, EREPORT_SATA_OVERTEMP) ?
		FAULT_DISK_OVERTEMP :
	    fmd_nvl_class_match(hdl, nvl, EREPORT_SATA_STFAIL) ?
		FAULT_DISK_STFAIL : NULL;

	if (fltclass != NULL) {

		/*
		 * Create and solve a new case by adding the ereport
		 * and its corresponding fault to the case and
		 * solving it.  It'll be closed when we get the
		 * list.suspect event later.
		 */
		cs = fmd_case_open(hdl, NULL);
		fmd_case_add_ereport(hdl, cs, ep);
		fltnvl = fmd_nvl_create_fault(hdl, fltclass, 100,
		    diskp->asru_fmri, diskp->fru_fmri,
		    diskp->disk_res_fmri);
		fmd_case_add_suspect(hdl, cs, fltnvl);
		fmd_case_solve(hdl, cs);
	}
}

static const fmd_hdl_ops_t fmd_ops = {
	diskmon_recv,	/* fmdo_recv */
	NULL,		/* fmdo_timeout */
	NULL,		/* fmdo_close */
	NULL,		/* fmdo_stats */
	NULL,		/* fmdo_gc */
};

static const fmd_prop_t fmd_props[] = {
	{ GLOBAL_PROP_FAULT_POLL, FMD_TYPE_UINT32, "3600" },
	{ GLOBAL_PROP_FAULT_INJ, FMD_TYPE_UINT32, "0" },
	{ GLOBAL_PROP_LOG_LEVEL, FMD_TYPE_UINT32, "0" },
	/*
	 * Default fault monitoring options are:
	 * (OPTION_SELFTEST_ERRS_ARE_FATAL |
	 * OPTION_OVERTEMP_ERRS_ARE_FATAL) == 0xC
	 */
	{ GLOBAL_PROP_FAULT_OPTIONS, FMD_TYPE_UINT32, "0xC" },
	{ GLOBAL_PROP_IPMI_BMC_MON, FMD_TYPE_UINT32, "1" },
	{ GLOBAL_PROP_IPMI_ERR_INJ, FMD_TYPE_UINT32, "0" },
	{ NULL, 0, NULL }
};

static const fmd_hdl_info_t fmd_info = {
	"Sun Fire X4500 Disk Diagnosis Engine",
	SFX4500_DISK_MODULE_VERSION,
	&fmd_ops,
	fmd_props
};

void
_fmd_init(fmd_hdl_t *hdl)
{
	fmd_case_t	*cp;
	int		disk_count;

	g_fm_hdl = hdl;

	if (fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info) != 0) {
		return;
	}

	if (config_init()) {
		log_err("Could not initialize configuration!\n");
		fmd_hdl_unregister(hdl);
		return;
	}

	if (config_get(hdl, fmd_props)) {
		config_fini();
		log_err("Could not retrieve configuration from libtopo!\n");
		fmd_hdl_unregister(hdl);
		return;
	}

	/*
	 * If there are no disks to monitor, bail out
	 */
	if ((disk_count = count_disks(config_data->disk_list)) == 0) {
		config_fini();
		fmd_hdl_unregister(hdl);
		return;
	}

	g_xprt_hdl = fmd_xprt_open(hdl, FMD_XPRT_RDONLY, NULL, NULL);
	if (diskmon_init() == E_ERROR) {
		fmd_xprt_close(hdl, g_xprt_hdl);
		g_xprt_hdl = NULL;
		config_fini();
		fmd_hdl_unregister(hdl);
		return;
	}

	log_msg(MM_MAIN, "Monitoring %d disks.\n", disk_count);

	/*
	 * Iterate over all active cases.
	 * Since we automatically solve all cases, these cases must have
	 * had the fault added, but the DE must have been interrupted
	 * before they were solved.
	 */
	for (cp = fmd_case_next(hdl, NULL);
	    cp != NULL; cp = fmd_case_next(hdl, cp)) {

		if (!fmd_case_solved(hdl, cp))
			fmd_case_solve(hdl, cp);
	}
}

void
_fmd_fini(fmd_hdl_t *hdl)
{
	diskmon_teardown_all();
	fmd_xprt_close(hdl, g_xprt_hdl);
	g_fm_hdl = NULL;
	g_xprt_hdl = NULL;
}
