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

/*
 * The InfiniBand  Nexus driver (IB nexus) is a bus nexus driver for IB bus.
 * It supports  Port nodes, Virtual Physical Point of Attachment nodes (VPPA)
 * for  HCAs registered with IBTL and IOC nodes for all the IOCs present in
 * the IB fabric (that are accessible to the host). It also supports Pseudo
 * device children to be enumerated using their .conf file(s). All Port nodes
 * and VPPA nodes are children of HCA drivers. All the IOC nodes and the Pseudo
 * device nodes are children of the IB nexus driver.
 *
 * IB nexus driver provides bus nexus entry points to all the HCA drivers.
 *
 * IB nexus  driver registers with  InfiniBand Device  Manager (IBDM) to get
 * information about all the HCA ports and  I/O Controllers (IOCs) connected
 * to the IB fabric. Based on that information, IB nexus will create all the
 * device tree nodes.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"


#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/taskq.h>
#include <sys/mdi_impldefs.h>
#include <sys/sunmdi.h>
#include <sys/sunpm.h>
#include <sys/ib/mgt/ibdm/ibdm_impl.h>
#include <sys/ib/ibnex/ibnex.h>
#include <sys/ib/ibnex/ibnex_devctl.h>
#include <sys/ib/ibtl/ibti.h>
#include <sys/ib/ibtl/impl/ibtl_ibnex.h>
#include <sys/file.h>
#include <sys/hwconf.h>

/* Function prototypes */
static int		ibnex_attach(dev_info_t *, ddi_attach_cmd_t);
static int		ibnex_getinfo(dev_info_t *, ddi_info_cmd_t,
			    void *, void **);
static int		ibnex_detach(dev_info_t *, ddi_detach_cmd_t);
static int		ibnex_busctl(dev_info_t *,
			    dev_info_t *, ddi_ctl_enum_t, void *, void *);
static int		ibnex_map_fault(dev_info_t *,
			    dev_info_t *, struct hat *, struct seg *,
			    caddr_t, struct devpage *, pfn_t, uint_t, uint_t);
static int		ibnex_init_child(dev_info_t *);
static ibnex_rval_t	ibnex_comm_svc_init(char *, ibnex_node_type_t);
static void		ibnex_comm_svc_fini();
dev_info_t		*ibnex_commsvc_initnode(dev_info_t *,
			    ibdm_port_attr_t *, int, int, ib_pkey_t, int *,
			    int);
static void		ibnex_delete_port_node_data(ibnex_node_data_t *);
int			ibnex_get_dip_from_guid(ib_guid_t, int,
			    ib_pkey_t, dev_info_t **);
static ibnex_node_data_t *ibnex_is_node_data_present(ibnex_node_type_t,
			    void *, int, ib_pkey_t);
static ibnex_node_data_t *ibnex_init_child_nodedata(ibnex_node_type_t, void *,
			    int, ib_pkey_t);
static int		ibnex_create_port_node_prop(ibdm_port_attr_t *,
			    dev_info_t *, char *, ib_pkey_t);
void			ibnex_dm_callback(void *, ibdm_events_t);
static int		ibnex_create_port_compatible_prop(dev_info_t *,
			    char *, ibdm_port_attr_t *);
static int		ibnex_create_ioc_srv_props(
			    dev_info_t *, ibdm_ioc_info_t *);
static int		ibnex_get_eventcookie(dev_info_t *,
			    dev_info_t *, char *, ddi_eventcookie_t *);
static int		ibnex_add_eventcall(dev_info_t *, dev_info_t *,
			    ddi_eventcookie_t, void (*)(dev_info_t *,
			    ddi_eventcookie_t, void *, void *),
			    void *arg, ddi_callback_id_t *cb_id);
static int		ibnex_remove_eventcall(dev_info_t *,
			    ddi_callback_id_t);
static int		ibnex_post_event(dev_info_t *, dev_info_t *,
			    ddi_eventcookie_t, void *);
static int		ibnex_bus_config(dev_info_t *, uint_t,
			    ddi_bus_config_op_t, void *, dev_info_t **);
static int		ibnex_bus_unconfig(dev_info_t *,
			    uint_t, ddi_bus_config_op_t, void *);
static dev_info_t	*ibnex_config_port_node(dev_info_t *, char *);
static dev_info_t	*ibnex_config_obp_args(dev_info_t *, char *);
static int		ibnex_get_pkey_commsvc_index_portnum(
			    char *, int *, ib_pkey_t *, uint8_t *);
static void		ibnex_config_all_children(dev_info_t *);
static int		ibnex_devname_to_portnum(char *, uint8_t *);
static void		ibnex_create_vppa_nodes(
			    dev_info_t *, ibdm_port_attr_t *);
static void		ibnex_create_port_nodes(
			    dev_info_t *, ibdm_port_attr_t *);
static void		ibnex_create_hcasvc_nodes(
			    dev_info_t *, ibdm_port_attr_t *);
static int		ibnex_config_root_iocnode(dev_info_t *, char *);
static int		ibnex_devname2port(char *, int *);
static int		ibnex_config_ioc_node(char *);
static int		ibnex_devname_to_node_n_ioc_guids(
			    char *, ib_guid_t *, ib_guid_t *);
static int		ibnex_is_ioc_present(ib_guid_t);
static void		ibnex_ioc_node_cleanup();
static void		ibnex_delete_ioc_node_data(ibnex_node_data_t *);
int			ibnex_ioc_initnode(ibdm_ioc_info_t *, int);
static int		ibnex_create_ioc_node_prop(
			    ibdm_ioc_info_t *, dev_info_t *);
static int		ibnex_create_ioc_compatible_prop(
			    dev_info_t *, ib_dm_ioc_ctrl_profile_t *);
uint64_t		ibnex_str2hex(char *, int, int *);
static int		ibnex_str2int(char *, int, int *);
static int		ibnex_create_ioc_portgid_prop(
			    dev_info_t *, ibdm_ioc_info_t *);
static void		ibnex_wakeup_reprobe_ioc(ibnex_node_data_t *, int);
static void		ibnex_wakeup_reprobe_all();
ibt_status_t		ibnex_ibtl_callback(ibtl_ibnex_cb_args_t *);
static int		ibnex_prom_devname_to_pkey_n_portnum(
			    char *, ib_pkey_t *, uint8_t *);
void			ibnex_pseudo_initnodes(void);
static char		*ibnex_lookup_unit_address_prop(ddi_prop_t *);
static void		ibnex_pseudo_node_cleanup(void);
static int		ibnex_name_child(dev_info_t *, char *, int);
static int		ibnex_name_pseudo_child(dev_info_t *, char *);

void			ibnex_reprobe_ioc_dev(void *);
void			ibnex_reprobe_ioc_all();
static void		ibnex_update_prop(ibnex_node_data_t *,
			    ibdm_ioc_info_t *);
static ibnex_rval_t	ibnex_unique_svcname(char *);
static void		ibnex_handle_reprobe_dev(void *arg);

extern int		ibnex_open(dev_t *, int, int, cred_t *);
extern int		ibnex_close(dev_t, int, int, cred_t *);
extern int		ibnex_ioctl(dev_t, int, intptr_t, int, cred_t *, int *);
extern int		ibnex_offline_childdip(dev_info_t *);

static int		ibnex_ioc_create_pi(
			    ibdm_ioc_info_t *, ibnex_node_data_t *);
static int		ibnex_bus_power(dev_info_t *, void *,
			    pm_bus_power_op_t, void *, void *);
int			ibnex_pseudo_create_pi(ibnex_node_data_t *);
int			ibnex_pseudo_config_one(
			    ibnex_node_data_t *, char *, char *);
static void		ibnex_config_pseudo_all(void);
/*
 * The bus_ops structure defines the capabilities of HCA nexus driver.
 */
struct bus_ops ibnex_ci_busops = {
	BUSO_REV,
	nullbusmap,		/* bus_map */
	NULL,			/* bus_get_intrspec */
	NULL,			/* bus_add_intrspec */
	NULL,			/* bus_remove_intrspec */
	ibnex_map_fault,	/* Map Fault */
	ddi_no_dma_map,		/* DMA related entry points */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ibnex_busctl,		/* bus_ctl */
	ddi_bus_prop_op,	/* bus_prop_op */
	NULL,			/* bus_get_eventcookie	*/
	NULL,			/* bus_add_eventcall	*/
	NULL,			/* bus_remove_eventcall	*/
	NULL,			/* bus_post_event	*/
	NULL,
	ibnex_bus_config,	/* bus config */
	ibnex_bus_unconfig	/* bus unconfig */
};


/*
 * Prototype declarations for the VHCI options
 */
/*
 * Functions registered with the mpxio framework
 */
static int ib_vhci_pi_init(dev_info_t *, mdi_pathinfo_t *, int);
static int ib_vhci_pi_uninit(dev_info_t *, mdi_pathinfo_t *, int);
static int ib_vhci_pi_state_change(dev_info_t *, mdi_pathinfo_t *,
		mdi_pathinfo_state_t, uint32_t, int);
static int ib_vhci_failover(dev_info_t *, dev_info_t *, int);


static mdi_vhci_ops_t ibnex_vhci_ops = {
	MDI_VHCI_OPS_REV,
	ib_vhci_pi_init,
	ib_vhci_pi_uninit,
	ib_vhci_pi_state_change,
	ib_vhci_failover
};


/*
 * The  bus_ops  structure  defines the  capabilities  of IB nexus driver.
 * IB nexus drivers does not  support any DMA  operations for its children
 * as there is  no  such concept in Infiniband.  All the memory operations
 * and DMA operations required by the child drivers can be performed using
 * the IBTF API.
 */
struct bus_ops ibnex_bus_ops = {
	BUSO_REV,
	nullbusmap,		/* bus_map */
	NULL,			/* bus_get_intrspec */
	NULL,			/* bus_add_intrspec */
	NULL,			/* bus_remove_intrspec */
	ibnex_map_fault,	/* Map Fault */
	ddi_no_dma_map,		/* DMA related entry points */
	ddi_no_dma_allochdl,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ibnex_busctl,		/* bus_ctl */
	ddi_bus_prop_op,	/* bus_prop_op */
	ibnex_get_eventcookie,	/* bus_get_eventcookie	*/
	ibnex_add_eventcall,	/* bus_add_eventcall	*/
	ibnex_remove_eventcall,	/* bus_remove_eventcall	*/
	ibnex_post_event,		/* bus_post_event	*/
	NULL,
	ibnex_bus_config,	/* bus config */
	ibnex_bus_unconfig,	/* bus unconfig */
	NULL,			/* bus fm init */
	NULL,			/* bus fm fini */
	NULL,			/* bus fm access enter */
	NULL,			/* bus fm access exit */
	ibnex_bus_power		/* bus power */
};


/* ibnex cb_ops */
static struct cb_ops ibnex_cbops = {
	ibnex_open,		/* open */
	ibnex_close,		/* close */
	nodev,			/* strategy */
	nodev,			/* print */
	nodev,			/* dump */
	nodev,			/* read */
	nodev,			/* write */
	ibnex_ioctl,		/* ioctl */
	nodev,			/* devmap */
	nodev,			/* mmap */
	nodev,			/* segmap */
	nochpoll,		/* poll */
	ddi_prop_op,		/* prop_op */
	NULL,			/* stream */
	D_MP,			/* cb_flag */
	CB_REV, 		/* rev */
	nodev,			/* int (*cb_aread)() */
	nodev			/* int (*cb_awrite)() */
};

/*
 * Device options
 * Note: ddi_no_info needs to change during devfs time frame. The drivers
 *	 with 1 to 1 mapping between minor node and instance should use
 *	 ddi_1to1_info. (See bug id 4424752)
 */
static struct dev_ops ibnex_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ibnex_getinfo,		/* info */
	nulldev,		/* identify */
	nulldev,		/* probe */
	ibnex_attach,		/* attach */
	ibnex_detach,		/* detach */
	nodev,			/* reset */
	&ibnex_cbops,		/* driver ops - devctl interfaces */
	&ibnex_bus_ops,		/* bus operations */
	nulldev			/* power */
};

/* Module linkage information for the kernel.  */
static struct modldrv modldrv = {
	&mod_driverops,		/* Driver module */
	"IB nexus %I%",		/* Driver name and version */
	&ibnex_ops,		/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/*
 * Global per-instance IB Nexus data.
 * There is only one instance of IB Nexus supported.
 */
ibnex_t ibnex;
#ifdef __lock_lint
extern ibdm_t ibdm;
#endif
_NOTE(MUTEX_PROTECTS_DATA(ibnex.ibnex_mutex, ibnex_s))
_NOTE(DATA_READABLE_WITHOUT_LOCK(ibnex.ibnex_num_comm_svcs
	ibnex.ibnex_comm_svc_names ibnex.ibnex_nvppa_comm_svcs
	ibnex.ibnex_vppa_comm_svc_names ibnex.ibnex_nhcasvc_comm_svcs
	ibnex.ibnex_hcasvc_comm_svc_names))
_NOTE(MUTEX_PROTECTS_DATA(ibnex.ibnex_mutex, ibnex_node_data_s))
_NOTE(LOCK_ORDER(ibdm.ibdm_hl_mutex ibnex.ibnex_mutex))

/* The port settling time in seconds */
int	ibnex_port_settling_time = 8;

/* create an array of properties supported, easier to add new ones here */
static struct ibnex_property {
	char			*name;
	ibnex_node_type_t	type;
} ibnex_properties[]  = {
	{ "port-svc-list",  IBNEX_PORT_COMMSVC_NODE},
	{ "vppa-svc-list",  IBNEX_VPPA_COMMSVC_NODE},
	{ "hca-svc-list",	IBNEX_HCASVC_COMMSVC_NODE}
};

#define	N_IBNEX_PROPS	(sizeof (ibnex_properties))/ \
				(sizeof (struct ibnex_property))

/*
 * Event Definition
 *	Single event, event name defined in ibti_common.h.
 *	Event posted to specific child handler. Event posted
 *	at kernel priority.
 */
static ndi_event_definition_t ibnex_ndi_event_defs[] = {
	{IB_EVENT_TAG_PROP_UPDATE,  IB_PROP_UPDATE_EVENT, EPL_KERNEL,
		NDI_EVENT_POST_TO_TGT}
};

#define	IB_N_NDI_EVENTS	\
	(sizeof (ibnex_ndi_event_defs) / sizeof (ndi_event_definition_t))

static ndi_event_set_t ib_ndi_events = {
	NDI_EVENTS_REV1, IB_N_NDI_EVENTS, ibnex_ndi_event_defs};


/*
 * _init
 *	Loadable module init, called before any other module.
 */
int
_init(void)
{
	int	error;

	IBTF_DPRINTF_L4("ibnex", "\t_init");
	mutex_init(&ibnex.ibnex_mutex, NULL, MUTEX_DRIVER, NULL);
	cv_init(&ibnex.ibnex_reprobe_cv, NULL, CV_DRIVER, NULL);
	if ((error = mod_install(&modlinkage)) != 0) {
		IBTF_DPRINTF_L2("ibnex", "\t_init: mod_install failed");
		mutex_destroy(&ibnex.ibnex_mutex);
		cv_destroy(&ibnex.ibnex_reprobe_cv);
	} else {
		ibdm_ibnex_register_callback(ibnex_dm_callback);
		ibtl_ibnex_register_callback(ibnex_ibtl_callback);
	}
	return (error);
}


/*
 * _fini
 *	Prepares a module for unloading.
 */
int
_fini(void)
{
	int	error;

	IBTF_DPRINTF_L4("ibnex", "\t_fini");
	if ((error = mod_remove(&modlinkage)) != 0) {
		return (error);
	}
	ibdm_ibnex_unregister_callback();
	ibtl_ibnex_unregister_callback();
	mutex_destroy(&ibnex.ibnex_mutex);
	cv_destroy(&ibnex.ibnex_reprobe_cv);
	return (0);
}


/*
 * _info
 *	Returns information about loadable module.
 */
int
_info(struct modinfo *modinfop)
{
	IBTF_DPRINTF_L4("ibnex", "\t_info");
	return (mod_info(&modlinkage, modinfop));
}


/*
 * ibnex_attach
 *	Configure and attach an instance of the IB Nexus driver
 *	Only one instance of IB Nexus is supported
 *	Create a minor node for cfgadm purpose
 *	Initialize communication services
 *	Register callback with IBDM
 *	Register callback with IBTL
 */
static int
ibnex_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int		i;
	int		instance = ddi_get_instance(dip);

	IBTF_DPRINTF_L4("ibnex", "\tattach: device = %p cmd = %x)", dip, cmd);

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		IBTF_DPRINTF_L4("ibnex", "\tattach: RESUME");
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	/* Fail attach for more than one instance */
	mutex_enter(&ibnex.ibnex_mutex);
	if (ibnex.ibnex_dip != NULL) {
		mutex_exit(&ibnex.ibnex_mutex);
		return (DDI_FAILURE);
	}
	mutex_exit(&ibnex.ibnex_mutex);

	/* Register with MPxIO framework */

	if (mdi_vhci_register(MDI_HCI_CLASS_IB, dip, &ibnex_vhci_ops, 0)
	    != MDI_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tattach: mdi_vhci_register() failed");
		return (DDI_FAILURE);
	}


	/*
	 * Create the "fabric" devctl minor-node for IB DR support.
	 * The minor number for the "devctl" node is in the same format
	 * as the AP minor nodes.
	 */
	if (ddi_create_minor_node(dip, IBNEX_FABRIC, S_IFCHR, instance,
	    DDI_NT_IB_ATTACHMENT_POINT, 0) != DDI_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tattach: failed to create fabric minornode");
		(void) mdi_vhci_unregister(dip, 0);
		return (DDI_FAILURE);
	}


	/*
	 * Set pm-want-child-notification property for
	 * power management of the phci and client
	 */
	if (ddi_prop_create(DDI_DEV_T_NONE, dip, DDI_PROP_CANSLEEP,
	    "pm-want-child-notification?", NULL, NULL) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "_attach: create pm-want-child-notification failed");
		(void) ddi_remove_minor_node(dip, NULL);
		(void) mdi_vhci_unregister(dip, 0);
		return (DDI_FAILURE);
	}

	mutex_enter(&ibnex.ibnex_mutex);
	ibnex.ibnex_dip  = dip;
	mutex_exit(&ibnex.ibnex_mutex);

	/*
	 * Event Handling: Definition and Binding.
	 */
	if (ndi_event_alloc_hdl(dip, 0, &ibnex.ibnex_ndi_event_hdl,
	    NDI_SLEEP) != NDI_SUCCESS) {
		(void) ddi_remove_minor_node(dip, NULL);
		IBTF_DPRINTF_L2("ibnex",
		    "_attach: ndi_event_alloc_hdl failed");
		(void) mdi_vhci_unregister(dip, 0);
		return (DDI_FAILURE);
	}
	if (ndi_event_bind_set(ibnex.ibnex_ndi_event_hdl, &ib_ndi_events,
	    NDI_SLEEP) != NDI_SUCCESS) {
		(void) ddi_remove_minor_node(dip, NULL);
		(void) ndi_event_free_hdl(ibnex.ibnex_ndi_event_hdl);
		IBTF_DPRINTF_L2("ibnex",
		    "_attach: ndi_event_bind_set failed");
		(void) mdi_vhci_unregister(dip, 0);
		return (DDI_FAILURE);
	}

	for (i = 0; i < N_IBNEX_PROPS; i++) {
		if (ibnex_comm_svc_init(ibnex_properties[i].name,
		    ibnex_properties[i].type) != IBNEX_SUCCESS) {
			ibnex_comm_svc_fini();
			(void) ndi_event_unbind_set(ibnex.ibnex_ndi_event_hdl,
			    &ib_ndi_events, NDI_SLEEP);
			(void) ddi_remove_minor_node(dip, NULL);
			(void) ndi_event_free_hdl(
			    ibnex.ibnex_ndi_event_hdl);
			ibnex.ibnex_ndi_event_hdl = NULL;
			IBTF_DPRINTF_L2("ibnex", "_attach: ibnex_comm_svc_init"
			    " failed %s", ibnex_properties[i].name);
			(void) mdi_vhci_unregister(dip, 0);
			return (DDI_FAILURE);
		}
	}

	/*
	 * before anything else comes up:
	 * Initialize the internal list of pseudo device nodes by
	 * getting all pseudo children of "ib" and processing them.
	 */
	ibnex_pseudo_initnodes();

	return (DDI_SUCCESS);
}


/*
 * ibnex_getinfo()
 * Given the device number, return the devinfo pointer or the
 * instance number.
 * Note: always succeed DDI_INFO_DEVT2INSTANCE, even before attach.
 */

/*ARGSUSED*/
static int
ibnex_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **result)
{
	int ret = DDI_SUCCESS;

	IBTF_DPRINTF_L4("ibnex", "\tgetinfo: Begin");
	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (ibnex.ibnex_dip != NULL)
			*result = ibnex.ibnex_dip;
		else {
			*result = NULL;
			ret = DDI_FAILURE;
		}
		break;

	case DDI_INFO_DEVT2INSTANCE:
		*result = 0;
		break;

	default:
		ret = DDI_FAILURE;
	}
	return (ret);
}


/*
 * ibnex_detach
 *	Unregister callback with the IBDM
 *	Unregister callback with the IBTL
 *	Uninitialize the communication entries
 *	Remove all the minor nodes created by this instance
 */
static int
ibnex_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{

	IBTF_DPRINTF_L4("ibnex", "\tdetach: dip = %p cmd = %x)", dip, cmd);

	switch (cmd) {

	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
		IBTF_DPRINTF_L4("ibnex", "\t_detach: Suspend");
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	mutex_enter(&ibnex.ibnex_mutex);
	if (ibt_hw_is_present()) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tdetach: IB HW is present ");
		mutex_exit(&ibnex.ibnex_mutex);
		return (DDI_FAILURE);
	}
	if (ndi_event_free_hdl(ibnex.ibnex_ndi_event_hdl)) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tdetach: ndi_event_free_hdl() failed");
		mutex_exit(&ibnex.ibnex_mutex);
		return (DDI_FAILURE);
	}
	ibnex.ibnex_ndi_event_hdl = NULL;
	ibnex.ibnex_prop_update_evt_cookie = NULL;

	ibnex_pseudo_node_cleanup();
	ibnex_comm_svc_fini();
	ibnex_ioc_node_cleanup();

	(void) ddi_remove_minor_node(dip, NULL);
	ibnex.ibnex_dip = NULL;
	mutex_exit(&ibnex.ibnex_mutex);
	(void) mdi_vhci_unregister(dip, 0);
	return (DDI_SUCCESS);
}


/*
 * ibnex_pseudo_node_cleanup()
 *	This checks if all the "dips" have been deallocated (implying
 *	that all the children have been unconfigured) first.
 *	If not, it just returns.
 *	If yes, then it frees up memory allocated for devi_name,
 *	node_addr, and property list.
 */
static void
ibnex_pseudo_node_cleanup(void)
{
	ibnex_node_data_t	*nodep =  ibnex.ibnex_pseudo_node_head;
	ibnex_pseudo_node_t	*pseudo;

	IBTF_DPRINTF_L4("ibnex", "\tpseudo_node_cleanup:");
	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	for (; nodep; nodep = nodep->node_next)
		if (nodep->node_dip)
			return;

	IBTF_DPRINTF_L4("ibnex", "\tpseudo_node_cleanup: freeing up memory");
	for (nodep =  ibnex.ibnex_pseudo_node_head; nodep;
	    nodep = nodep->node_next) {

		pseudo = &nodep->node_data.pseudo_node;
		if (pseudo->pseudo_node_addr) {
			kmem_free(pseudo->pseudo_node_addr,
			    strlen(pseudo-> pseudo_node_addr) + 1);
			pseudo->pseudo_node_addr = NULL;
		}

		if (pseudo->pseudo_devi_name) {
			kmem_free(pseudo->pseudo_devi_name,
			    strlen(pseudo-> pseudo_devi_name) + 1);
			pseudo->pseudo_devi_name = NULL;
		}

		if (pseudo->pseudo_unit_addr) {
			kmem_free(pseudo->pseudo_unit_addr,
			    pseudo->pseudo_unit_addr_len);
		}
	}
}


/*
 * This functions wakes up any reprobe requests waiting for completion
 * of reprobe of this IOC. It also send an NDI event, if  :
 *
 *	notify_flag is set. This is set if :
 *		ibt_reprobe_ioc has returned with SUCCESS
 *		IBTF client has not been notified for this node.
 *	node_data->node_dip != NULL
 *	node_state has IBNEX_NODE_REPROBE_NOTIFY_ALWAYS set
 *	An NDI event cookie has been registered.
 */
static void
ibnex_wakeup_reprobe_ioc(ibnex_node_data_t *node_data, int notify_flag)
{
	ddi_eventcookie_t	evt_cookie;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));
	evt_cookie = ibnex.ibnex_prop_update_evt_cookie;

	if ((ibnex.ibnex_reprobe_state == IBNEX_REPROBE_IOC_WAIT) ||
	    (node_data->node_reprobe_state != 0)) {
		if (notify_flag && (node_data->node_dip != NULL) &&
		    (node_data->node_state &
		    IBNEX_NODE_REPROBE_NOTIFY_ALWAYS) &&
		    (evt_cookie != NULL)) {
			ibt_prop_update_payload_t	evt_data;

			mutex_exit(&ibnex.ibnex_mutex);

			bzero(&evt_data, sizeof (evt_data));
			if (ndi_post_event(ibnex.ibnex_dip,
			    node_data->node_dip,
			    evt_cookie, &evt_data) != NDI_SUCCESS)
				IBTF_DPRINTF_L2("ibnex",
				    "\tndi_post_event failed\n");

			mutex_enter(&ibnex.ibnex_mutex);
		}

		node_data->node_reprobe_state = 0;
		cv_broadcast(&ibnex.ibnex_reprobe_cv);
	}
	node_data->node_reprobe_state = 0;
}

/*
 * This function wakes up any reprobe request waiting for completion
 * of reprobe of all IOCs.
 */
static void
ibnex_wakeup_reprobe_all()
{
	ibnex_node_data_t *ioc_node;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	/* Notify if another reprobe_all is pending */
	if (ibnex.ibnex_reprobe_state == IBNEX_REPROBE_ALL_WAIT) {
		ibnex.ibnex_reprobe_state = 0;
		cv_broadcast(&ibnex.ibnex_reprobe_cv);
	}
	ibnex.ibnex_reprobe_state = 0;

	/*
	 * The IOC may be hot-removed after the reprobe request.
	 * Reset the reprobe states for such IOCs.
	 */
	for (ioc_node = ibnex.ibnex_ioc_node_head; ioc_node;
	    ioc_node = ioc_node->node_next) {
		if (ioc_node->node_reprobe_state != 0) {
			ibnex_wakeup_reprobe_ioc(ioc_node, 1);
		}
	}
}

/*
 * ibnex_ibnex_callback:
 *	IBTL_IBNEX_IBC_INIT:
 *		Called from ibc_init() which is called from
 *		HCA driver _init entry point
 *		Initializes the HCA dev_ops structure with default
 *		IB nexus structure.
 *	IBTL_IBNEX_IBC_FINI:
 *		Called from ibc_fini() which is called from
 *		HCA driver _fini entry point
 *		Un-Initializes the HCA dev_ops structure with default
 *		IB nexus strucuture.
 *	Returns IBT_SUCCESS
 */
ibt_status_t
ibnex_ibtl_callback(ibtl_ibnex_cb_args_t *cb_args)
{
	int			retval = IBT_SUCCESS;
	struct dev_ops 		*hca_dev_ops;
	dev_info_t		*clnt_dip;
	ibnex_node_data_t	*node_data;

	IBTF_DPRINTF_L5("ibnex", "\tibtl_callback");

	switch (cb_args->cb_flag) {
	case IBTL_IBNEX_IBC_INIT:
		/*
		 * Get the devops structure of the HCA,
		 * and put IB nexus default busops vector in its place.
		 */
		hca_dev_ops = ((struct modldrv *)
			(cb_args->cb_modlp->ml_linkage[0]))->drv_dev_ops;
		ASSERT((hca_dev_ops) && (hca_dev_ops->devo_bus_ops == NULL));
		hca_dev_ops->devo_bus_ops = &ibnex_ci_busops;
		break;

	case IBTL_IBNEX_IBC_FINI:
		hca_dev_ops = ((struct modldrv *)
			(cb_args->cb_modlp->ml_linkage[0]))->drv_dev_ops;
		hca_dev_ops->devo_bus_ops = NULL;
		break;

	case IBTL_IBNEX_REPROBE_DEV_REQ:
		/* IBTL pass down request for ibt_reprobe_dev */
		clnt_dip = cb_args->cb_dip;
		ASSERT(clnt_dip);

		node_data = ddi_get_parent_data(clnt_dip);
		ASSERT(node_data);

		/* Reprobe for IOC nodes only */
		ASSERT(node_data->node_type == IBNEX_IOC_NODE);

		/*
		 * Start the reprobe. This could sleep as it is not
		 * from interrupt context.
		 */
		if (taskq_dispatch(system_taskq, ibnex_handle_reprobe_dev,
		    clnt_dip, TQ_SLEEP) == 0) {
			IBTF_DPRINTF_L2("ibnex",
			    "ibnex_ibtl_callback: taskq_dispatch failed");
			mutex_enter(&ibnex.ibnex_mutex);
			ibnex_wakeup_reprobe_ioc(node_data, 0);
			mutex_exit(&ibnex.ibnex_mutex);
			return (IBT_INSUFF_KERNEL_RESOURCE);
		}
		return (IBT_SUCCESS);
	}

	return (retval);
}


/*
 * Bus-ops entry points
 */

/*
 * ibnex_map_fault
 * 	IOC drivers need not map memory. Return failure to fail any
 *	such calls.
 */
/*ARGSUSED*/
static int
ibnex_map_fault(dev_info_t *dip, dev_info_t *rdip, struct hat *hat,
    struct seg *seg, caddr_t addr, struct devpage *dp, pfn_t pfn,
    uint_t prot, uint_t lock)
{
	return (DDI_FAILURE);
}


/*
 * ibnex_busctl
 * 	bus_ctl bus_ops entry point
 */
/*ARGSUSED*/
static int
ibnex_busctl(dev_info_t *dip, dev_info_t *rdip,
    ddi_ctl_enum_t ctlop, void *arg, void *result)
{
	dev_info_t		*child_dip;

	IBTF_DPRINTF_L4("ibnex",
	    "\tbusctl: dip = %p, rdip = %p, ctlop = %x,", dip, rdip, ctlop);
	IBTF_DPRINTF_L4("ibnex", "\tbusctl: targ = %p, result %p", arg, result);

	switch (ctlop) {
	case DDI_CTLOPS_REPORTDEV:
		if (rdip == NULL) {
			return (DDI_FAILURE);
		}

		/* Log the relevant details of dip to sysbuf */
		cmn_err(CE_CONT, "?IB device: %s@%s, %s%d\n",
		    ddi_node_name(rdip), ddi_get_name_addr(rdip),
		    ddi_driver_name(rdip), ddi_get_instance(rdip));

		return (DDI_SUCCESS);

	case DDI_CTLOPS_INITCHILD:
		child_dip = (dev_info_t *)arg;
		return (ibnex_init_child(child_dip));

	case DDI_CTLOPS_UNINITCHILD:
		child_dip = (dev_info_t *)arg;
		ddi_set_name_addr(child_dip, NULL);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_ATTACH:
	case DDI_CTLOPS_DETACH:
	case DDI_CTLOPS_POWER :
		return (DDI_SUCCESS);

	case DDI_CTLOPS_SIDDEV:
		/*
		 * Return DDI_SUCCESS for IOC/PORT/VPPA nodes and
		 * DDI_FAILURE for the nodes enumerated by a Pseudo file.
		 */
		return (ndi_dev_is_persistent_node(rdip) ?
		    DDI_SUCCESS : DDI_FAILURE);


	case DDI_CTLOPS_IOMIN:
		/*
		 * Return DDI_SUCCESS, so that consistent buf alloc
		 * gets the default DMA IO minimum for the platform
		 */
		return (DDI_SUCCESS);

	/*
	 * These ops correspond to functions that "shouldn't" be
	 * called by IB Nexus driver.
	 */
	case DDI_CTLOPS_DMAPMAPC:
	case DDI_CTLOPS_REPORTINT:
	case DDI_CTLOPS_REGSIZE:
	case DDI_CTLOPS_NREGS:
	case DDI_CTLOPS_SLAVEONLY:
	case DDI_CTLOPS_AFFINITY:
	case DDI_CTLOPS_POKE:
	case DDI_CTLOPS_PEEK:
		IBTF_DPRINTF_L2("ibnex",
		    "%s%d: invalid op (%d) from %s inst%d",
		    ddi_get_name(dip), ddi_get_instance(dip),
		    ctlop, ddi_get_name(rdip), ddi_get_instance(rdip));
		return (DDI_FAILURE);

	/*
	 * Everything else (PTOB/BTOP/BTOPR/DVMAPAGESIZE requests) we
	 * pass up
	 */
	default:
		return (ddi_ctlops(dip, rdip, ctlop, arg, result));
	}
}


/*
 * ibnex_init_child()
 *
 * Initialize a child device node. This is called for the DDI_CTLOPS_INITCHILD
 * entry. Function returns DDI_SUCCESS,  DDI_FAILURE or DDI_NOT_WELL_FORMED.
 */
static int
ibnex_init_child(dev_info_t *child)
{
	int			ret;
	char			name[MAXNAMELEN];

	IBTF_DPRINTF_L4("ibnex", "\tinit_child: child = %p", child);

	/* Handle Pseudo nodes of client children */
	if (ndi_dev_is_persistent_node(child) == 0) {
		if (ibnex_name_pseudo_child(child, name) != DDI_SUCCESS)
			return (DDI_FAILURE);

		ddi_set_name_addr(child, name);
		/*
		 * Merge the .conf node
		 */
		if (ndi_merge_node(child,
		    ibnex_name_child) == DDI_SUCCESS) {
			ddi_set_name_addr(child, NULL);
			return (DDI_FAILURE);
		}
		return (DDI_NOT_WELL_FORMED);

	}

	if ((ret = ibnex_name_child(child, name, 0)) != DDI_SUCCESS)
		return (ret);

	ddi_set_name_addr(child, name);

	return (DDI_SUCCESS);
}


int
ibnex_name_pseudo_child(dev_info_t *child, char *name)
{
	char **unit_addr;
	uint_t n;
	if (ddi_prop_lookup_string_array(DDI_DEV_T_ANY, child,
	    DDI_PROP_DONTPASS, "unit-address", &unit_addr, &n) !=
	    DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
		    "cannot find unit-address in %s.conf",
		    ddi_get_name(child));
		return (DDI_FAILURE);
	}
	if (n != 1 || *unit_addr == NULL || **unit_addr == 0) {
		cmn_err(CE_WARN, "unit-address property in %s.conf"
		    " not well-formed", ddi_get_name(child));
		ddi_prop_free(unit_addr);
		return (DDI_FAILURE);
	}
	(void) snprintf(name, MAXNAMELEN, "%s", *unit_addr);
	ddi_prop_free(unit_addr);
	return (DDI_SUCCESS);
}


/*ARGSUSED*/
int
ibnex_name_child(dev_info_t *child, char *name, int flag)
{
	ibnex_pseudo_node_t	*pseudo;
	ibnex_node_data_t	*node_datap;
	ibnex_port_node_t	*port_node;
	ibnex_ioc_node_t	*ioc;

	node_datap = ddi_get_parent_data(child);
	if (node_datap == NULL) {
		IBTF_DPRINTF_L2("ibnex", "\tname_child: Node data is NULL");
		return (DDI_NOT_WELL_FORMED);
	}
	IBTF_DPRINTF_L4("ibnex", "\tname_sid_child: Node data %p"
	    "Node type %x", node_datap, node_datap->node_type);
	switch (node_datap->node_type) {
	case IBNEX_PORT_COMMSVC_NODE:
		port_node = &node_datap->node_data.port_node;
		(void) snprintf(name, IBNEX_MAX_NODEADDR_SZ, "%x,0,%s",
		    port_node->port_num,
		    ibnex.ibnex_comm_svc_names[port_node->port_commsvc_idx]);
		break;
	case IBNEX_VPPA_COMMSVC_NODE:
		port_node = &node_datap->node_data.port_node;
		(void) snprintf(name, IBNEX_MAX_NODEADDR_SZ, "%x,%x,%s",
		    port_node->port_num, port_node->port_pkey, ibnex.
		    ibnex_vppa_comm_svc_names[port_node->port_commsvc_idx]);
		break;
	case IBNEX_HCASVC_COMMSVC_NODE:
		port_node = &node_datap->node_data.port_node;
		(void) snprintf(name, IBNEX_MAX_NODEADDR_SZ, "%x,0,%s",
		    port_node->port_num,
		    ibnex.ibnex_hcasvc_comm_svc_names[port_node->
		    port_commsvc_idx]);
		break;
	case IBNEX_IOC_NODE:
		ioc = &node_datap->node_data.ioc_node;
		(void) snprintf(name, IBNEX_MAX_NODEADDR_SZ, "%llX,%llX",
		    (longlong_t)ioc->ioc_guid, (longlong_t)ioc->iou_guid);
		break;
	case IBNEX_PSEUDO_NODE:
		pseudo = &node_datap->node_data.pseudo_node;
		(void) snprintf(name,
		    IBNEX_MAX_NODEADDR_SZ, pseudo->pseudo_unit_addr);
		break;
	default:
		IBTF_DPRINTF_L2("ibnex", "\name_child: Not well formed");
		return (DDI_NOT_WELL_FORMED);
	}

	return (DDI_SUCCESS);
}


/*
 * ibnex_bus_config()
 *
 * BUS_CONFIG_ONE:
 *	Enumerate the exact instance of the driver. Use the device node name
 *	to locate the exact instance.
 *	Query IBDM to find whether the hardware exits for the instance of the
 *	driver. If exists, create a device node and return NDI_SUCCESS.
 *
 * BUS_CONFIG_ALL:
 *	Enumerate all the instances of all the possible children (seen before
 *	and never seen before).
 *
 * BUS_CONFIG_DRIVER:
 *	Enumerate all the instances of a particular driver.
 */
static int
ibnex_bus_config(dev_info_t *parent, uint_t flag,
    ddi_bus_config_op_t op, void *devname, dev_info_t **child)
{
	int			ret = IBNEX_SUCCESS, len, circ;
	char 			*device_name, *cname = NULL, *caddr = NULL;
	char			*srvname, nameaddr[MAXNAMELEN];
	dev_info_t		*cdip, *pdip = NULL;
	ibnex_node_data_t	*node_data;
	ibnex_port_node_t	*port_node;

	ndi_devi_enter(parent, &circ);

	switch (op) {
	case BUS_CONFIG_ONE:
		IBTF_DPRINTF_L4("ibnex", "\tbus_config: CONFIG_ONE");

		len = strlen((char *)devname) + 1;
		device_name = i_ddi_strdup(devname, KM_SLEEP);
		i_ddi_parse_name(device_name, &cname, &caddr, NULL);

		if (caddr == NULL || (strlen(caddr) == 0)) {
			kmem_free(device_name, len);
			ndi_devi_exit(parent, circ);
			return (NDI_FAILURE);
		}

		IBTF_DPRINTF_L4("ibnex",
		    "\tbus_config: cname %s addr %s", cname, caddr);

		cdip = ndi_devi_findchild(parent, device_name);
		if (cdip == NULL) {
			/* Node is not present */
			if (strncmp(cname, IBNEX_IOC_CNAME, 3) == 0) {
				if (parent == ibnex.ibnex_dip)
					ret = ibnex_config_ioc_node(devname);
				else {
					ret = ibnex_config_root_iocnode(
					    parent, devname);
					if (ibnex.ibnex_dip)
						pdip = ibnex.ibnex_dip;
					else
						ret = IBNEX_FAILURE;
				}
			} else if ((strncmp(cname,
			    IBNEX_IBPORT_CNAME, 6) == 0) &&
			    (parent != ibnex.ibnex_dip)) { /* parent is HCA */
				cdip = ibnex_config_port_node(parent, devname);
				if (cdip)
					ret = IBNEX_SUCCESS;
				else
					ret = IBNEX_FAILURE;
				/* Allows enumeration under PHCI */
				flag |= NDI_MDI_FALLBACK;
			} else if (parent == ibnex.ibnex_dip) {
				/*
				 * if not IOC or PORT device then always
				 * assume a Pseudo child
				 */
				ret = IBNEX_SUCCESS;
				ibnex_pseudo_initnodes();
				mutex_enter(&ibnex.ibnex_mutex);
				ret = ibnex_pseudo_config_one(
				    NULL, cname, caddr);
				mutex_exit(&ibnex.ibnex_mutex);
			} else
				ret = IBNEX_FAILURE;
		}
		kmem_free(device_name, len);
		break;

	case BUS_CONFIG_OBP_ARGS:
		cdip = ibnex_config_obp_args(parent, devname);
		if (cdip) {
			/*
			 * Boot case.
			 * Special handling because the "devname"
			 * format for the enumerated device is
			 * different.
			 */
			node_data = ddi_get_parent_data(cdip);
			port_node = &node_data->node_data.port_node;
			if (node_data->node_type ==
			    IBNEX_VPPA_COMMSVC_NODE) {
				srvname =
				    ibnex.ibnex_vppa_comm_svc_names[
				    port_node->port_commsvc_idx];
				(void) snprintf(nameaddr, MAXNAMELEN,
				    "ibport@%x,%x,%s",
				    port_node->port_num,
				    port_node->port_pkey, srvname);
			}
			devname = (void *)nameaddr;
		} else {
			IBTF_DPRINTF_L2("ibnex",
			    "\tbus_config: CONFIG_OBP_ARGS : invalid state!!");

			ret = IBNEX_FAILURE;
		}
		break;
	case BUS_CONFIG_ALL:
		IBTF_DPRINTF_L4("ibnex", "\tbus_config: CONFIG_ALL");
		ibnex_config_all_children(parent);
		break;

	case BUS_CONFIG_DRIVER:
		IBTF_DPRINTF_L4("ibnex", "\tbus_config: CONFIG_DRIVER");
		ibnex_config_all_children(parent);
		break;

	default:
		IBTF_DPRINTF_L4("ibnex", "\tbus_config: error");
		ret = IBNEX_FAILURE;
		break;
	}
	ndi_devi_exit(parent, circ);
	if (ret == IBNEX_SUCCESS) {
		if (op == BUS_CONFIG_OBP_ARGS)
			op = BUS_CONFIG_ONE;

		if (pdip == NULL)
			pdip = parent;

		ret = ndi_busop_bus_config(
		    pdip, flag, op, devname, child, 0);
		IBTF_DPRINTF_L4("ibnex", "\tbus_config:"
		    "ndi_busop_bus_config : retval %d", ret);
		return (ret);
	}

	IBTF_DPRINTF_L2("ibnex", "\tbus_config: Failure End");
	return (NDI_FAILURE);
}


/*
 * ibnex_config_root_iocnode()
 *	Configures one particular instance of the IOC driver.
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_config_root_iocnode(dev_info_t *parent, char *device_name)
{
	int			ret, port = 0, iter = 0;
	boolean_t		displayed = B_FALSE;
	char			*portstr;
	ib_guid_t		hca_guid, iou_guid, ioc_guid;
	ibdm_ioc_info_t		*ioc_info;
	ibdm_port_attr_t	*port_attr;

	IBTF_DPRINTF_L4("ibnex",
	    "\tconfig_root_iocnode: name %s", device_name);

	portstr = strstr(device_name, ":port=");
	if (portstr == NULL) {
		return (IBNEX_FAILURE);
	}

	portstr[0] = 0; portstr++;
	if (ibnex_devname2port(portstr, &port)) {
		IBTF_DPRINTF_L4("ibnex", "\tconfig_root_iocnode: invalid port");
		return (IBNEX_FAILURE);
	}

	if (ibnex_devname_to_node_n_ioc_guids(
	    device_name, &iou_guid, &ioc_guid) != IBNEX_SUCCESS) {
		return (IBNEX_FAILURE);
	}

	(void) snprintf(device_name, (IBNEX_MAX_NODEADDR_SZ + 4),
	    "ioc@%llX,%llX", (longlong_t)ioc_guid, (longlong_t)iou_guid);

	hca_guid = ibtl_ibnex_hcadip2guid(parent);
	if ((port_attr = ibdm_ibnex_probe_hcaport(hca_guid, port)) == NULL) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tconfig_root_iocnode: Port does not exist");
		return (IBNEX_FAILURE);
	}

	/* Wait until "port is up" */
	while (port_attr->pa_state != IBT_PORT_ACTIVE) {
		ibdm_ibnex_free_port_attr(port_attr);
		delay(drv_usectohz(10000));
		if ((port_attr = ibdm_ibnex_probe_hcaport(
		    hca_guid, port)) == NULL) {
			return (IBNEX_FAILURE);
		}

		if (iter++ == 400) {
			if (displayed == B_FALSE) {
				cmn_err(CE_NOTE, "\tWaiting for Port %d "
				    "initialization", port_attr->pa_port_num);
				displayed = B_TRUE;
			}
		}
	}
	ibdm_ibnex_free_port_attr(port_attr);
	IBTF_DPRINTF_L4("ibnex", "\tconfig_rootioc_node:"
	    "Port is initialized");

	if ((ioc_info = ibdm_ibnex_probe_ioc(iou_guid, ioc_guid, 0)) == NULL) {
		ibdm_ibnex_free_ioc_list(ioc_info);
		return (IBNEX_FAILURE);
	}
	mutex_enter(&ibnex.ibnex_mutex);
	if (ibnex_is_ioc_present(ioc_guid) == IBNEX_SUCCESS) {
		IBTF_DPRINTF_L4("ibnex", "\tconfig_root_iocnode: IOC present");
		ret = IBNEX_SUCCESS;
	} else
		ret = ibnex_ioc_initnode(ioc_info, IBNEX_DEVFS_ENUMERATE);
	mutex_exit(&ibnex.ibnex_mutex);
	ibdm_ibnex_free_ioc_list(ioc_info);
	return (ret);
}


static int
ibnex_devname2port(char *portstr, int *port)
{
	char	*temp;
	int	ret = IBNEX_FAILURE;

	IBTF_DPRINTF_L4("ibnex", "\tdevname2port: Begin");

	temp = strchr(portstr, '=');
	if (temp != NULL) {
		temp++;
		*port = ibnex_str2int(temp, strlen(temp), &ret);
	}
	return (ret);
}


/*
 * ibnex_config_all_children()
 *	Wait for lata SM initialization case before enumerating the nodes
 *	Get list of HCA's and HCA port information
 *		Create device device nodes and its node properties
 *		for port nodes and VPPA nodes
 *	Get list of all the IOC node information
 *		Create device nodes and its properties for all the IOCs
 *		if not created already
 *	Bind drivers for all the newly created device nodes
 *	Support Pseudo nodes enumerated using their .conf file
 */
static void
ibnex_config_all_children(dev_info_t *parent)
{
	int			ii;
	time_t			wait_time;
	ibdm_ioc_info_t		*ioc_list, *ioc;
	ibdm_hca_list_t		*hca_list;
	ib_guid_t		hca_guid;

	IBTF_DPRINTF_L4("ibnex", "\tconfig_all_children: Begin");

	if (parent != ibnex.ibnex_dip) {
		/*
		 * Parent is a HCA node. Enumerate only children of
		 * this HCA, port nodes, VPPA & HCA_SVC nodes
		 */
		hca_guid = ibtl_ibnex_hcadip2guid(parent);
		wait_time = ibdm_ibnex_get_waittime(
			hca_guid, &ibnex_port_settling_time);
		if (wait_time) {
			delay(drv_usectohz(wait_time * 1000000));
		}
		hca_list = ibdm_ibnex_get_hca_info_by_guid(hca_guid);
		if (hca_list == NULL)
			return;
		ibnex_create_hcasvc_nodes(parent, hca_list->hl_hca_port_attr);
		for (ii = 0; ii < hca_list->hl_nports; ii++) {
			ibnex_create_port_nodes(
			    parent, &hca_list->hl_port_attr[ii]);
			ibnex_create_vppa_nodes(
			    parent, &hca_list->hl_port_attr[ii]);
		}
		ibdm_ibnex_free_hca_list(hca_list);
	} else {

		ibnex_pseudo_initnodes();

		/* Parent is a IB nexus. Enumerate all the IOC's */
		wait_time = ibdm_ibnex_get_waittime(
			0, &ibnex_port_settling_time);
		if (wait_time)
			delay(drv_usectohz(wait_time * 1000000));


		ioc_list = ibdm_ibnex_get_ioc_list(IBDM_IBNEX_NORMAL_PROBE);
		ioc = ioc_list;

		mutex_enter(&ibnex.ibnex_mutex);

		while (ioc_list) {
			if (ibnex_is_ioc_present(
			    ioc_list->ioc_profile.ioc_guid) != IBNEX_SUCCESS) {
				(void) ibnex_ioc_initnode(ioc_list,
				    IBNEX_DEVFS_ENUMERATE);
			}
			ioc_list = ioc_list->ioc_next;
		}
		ibnex_config_pseudo_all();
		mutex_exit(&ibnex.ibnex_mutex);
		ibdm_ibnex_free_ioc_list(ioc);
	}
	IBTF_DPRINTF_L4("ibnex", "\tconfig_all_children: End");
}


/*
 * ibnex_create_port_nodes:
 *	Creates a device node per each communication service defined
 *	in the "port-commsvc-list" property per HCA port
 */
static void
ibnex_create_port_nodes(dev_info_t *parent, ibdm_port_attr_t *port_attr)
{
	int		idx;
	dev_info_t	*dip;
	int		rval;

	mutex_enter(&ibnex.ibnex_mutex);
	for (idx = 0; idx < ibnex.ibnex_num_comm_svcs; idx++) {
		rval = ibnex_get_dip_from_guid(port_attr->pa_port_guid,
		    idx, 0, &dip);
		if (rval != IBNEX_SUCCESS) {
			(void) ibnex_commsvc_initnode(parent, port_attr, idx,
			    IBNEX_PORT_COMMSVC_NODE, 0, &rval,
			    IBNEX_DEVFS_ENUMERATE);
		}
	}
	mutex_exit(&ibnex.ibnex_mutex);
}


/*
 * ibnex_create_vppa_nodes:
 *	Creates a device node per each communication service defined
 *	in the "vppa-commsvc-list" property and per each PKEY that
 *	this particular port supports and per HCA port
 */
static void
ibnex_create_vppa_nodes(dev_info_t *parent, ibdm_port_attr_t *port_attr)
{
	int 		idx, ii;
	int		rval;
	ib_pkey_t 	pkey;
	dev_info_t	*dip;

	IBTF_DPRINTF_L4("ibnex", "\tcreate_vppa_nodes: Begin");

	mutex_enter(&ibnex.ibnex_mutex);
	if (port_attr->pa_state != IBT_PORT_ACTIVE) {
		IBTF_DPRINTF_L4("ibnex", "\tcreate_vppa_nodes: "
		    "Port %d is down", port_attr->pa_port_num);
		mutex_exit(&ibnex.ibnex_mutex);
		return;
	}
	for (idx = 0; idx < ibnex.ibnex_nvppa_comm_svcs; idx++) {
		for (ii = 0; ii < port_attr->pa_npkeys; ii++) {
			pkey = port_attr->pa_pkey_tbl[ii].pt_pkey;

			if (IBNEX_INVALID_PKEY(pkey)) {
				continue;
			}
			rval = ibnex_get_dip_from_guid(
			    port_attr->pa_port_guid, idx, pkey, &dip);
			if (rval != IBNEX_SUCCESS) {
				(void) ibnex_commsvc_initnode(parent, port_attr,
					idx, IBNEX_VPPA_COMMSVC_NODE,
					pkey, &rval, IBNEX_CFGADM_ENUMERATE);
				IBTF_DPRINTF_L5("ibnex", "\tcreate_vppa_nodes: "
				    "commsvc_initnode failed rval %x", rval);
			}
		}
	}
	mutex_exit(&ibnex.ibnex_mutex);
}


/*
 * ibnex_create_hcasvc_nodes:
 *	Creates a device node per each communication service defined
 *	in the "port-commsvc-list" property per HCA port
 */
static void
ibnex_create_hcasvc_nodes(dev_info_t *parent, ibdm_port_attr_t *port_attr)
{
	int		idx;
	dev_info_t	*dip;
	int		rval;

	mutex_enter(&ibnex.ibnex_mutex);
	for (idx = 0; idx < ibnex.ibnex_nhcasvc_comm_svcs; idx++) {
		rval = ibnex_get_dip_from_guid(port_attr->pa_port_guid,
		    idx, 0, &dip);
		if (rval != IBNEX_SUCCESS) {
			(void) ibnex_commsvc_initnode(parent, port_attr, idx,
			    IBNEX_HCASVC_COMMSVC_NODE, 0, &rval,
			    IBNEX_DEVFS_ENUMERATE);
			IBTF_DPRINTF_L5("ibnex", "create_hcasvc_nodes: "
			    "commsvc_initnode failed, rval %x", rval);
		}
	}
	mutex_exit(&ibnex.ibnex_mutex);
}

/*
 * ibnex_is_ioc_present()
 *	Returns IBNEX_SUCCESS if an entry found in the global linked list
 *	Returns IBNEX_FAILURE, if no match found
 */
static int
ibnex_is_ioc_present(ib_guid_t ioc_guid)
{
	ibnex_node_data_t	*head;
	ibnex_ioc_node_t	*ioc;
	int			ret = IBNEX_FAILURE;

	IBTF_DPRINTF_L4("ibnex", "\tis_ioc_present: Begin");
	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	head = ibnex.ibnex_ioc_node_head;
	while (head) {
		ioc = &head->node_data.ioc_node;
		if (ioc->ioc_guid == ioc_guid)
			break;
		head = head->node_next;
	}
	if (head)
		ret = IBNEX_SUCCESS;

	return (ret);
}


/*
 * ibnex_bus_unconfig()
 *
 *	Unconfigure a particular device node or all instance of a device
 *	driver device or all children of IBnex
 */
static int
ibnex_bus_unconfig(dev_info_t *parent,
    uint_t flag, ddi_bus_config_op_t op, void *device_name)
{
	return (ndi_busop_bus_unconfig(parent, flag, op, device_name));
}


/*
 * ibnex_config_port_node()
 *
 *	Configures a particular port / HCA  node for a particular
 *	communication service.
 *	The format of the device_name is
 *		ibport@<Port#>,<pkey>,<service name>
 *	where pkey = 0 for port communication service nodes
 *		  Port# = 0 for HCA_SVC communication service nodes
 *	Returns "dev_info_t" of the "child" node just created
 *	NULL when failed to enumerate the child node
 */
static dev_info_t *
ibnex_config_port_node(dev_info_t *parent, char *devname)
{
	int			ii, index;
	int			rval;
	time_t			wait_time;
	uint8_t			port_num;
	ib_guid_t		hca_guid, port_guid;
	ib_pkey_t		pkey;
	dev_info_t		*cdip;
	ibdm_port_attr_t	*port_attr;
	ibdm_hca_list_t		*hca_list;

	IBTF_DPRINTF_L4("ibnex", "\tconfig_port_node: %s", devname);

	if (ibnex_get_pkey_commsvc_index_portnum(devname,
	    &index, &pkey, &port_num) != IBNEX_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tconfig_port_node: Invalid Service Name");
		return (NULL);
	}

	hca_guid = ibtl_ibnex_hcadip2guid(parent);
	if (port_num == 0) {
		/*
		 * Use the dummy port attribute for HCA node in hca_list
		 * Note : pa_state is always IBT_PORT_ACTIVE.
		 */
		hca_list = ibdm_ibnex_get_hca_info_by_guid(hca_guid);
		ASSERT(hca_list != NULL);
		port_attr = hca_list->hl_hca_port_attr;
	} else {
		if ((port_attr = ibdm_ibnex_probe_hcaport(
		    hca_guid, port_num)) == NULL) {
			IBTF_DPRINTF_L2("ibnex",
			    "\tconfig_port_node: Port does not exist");
			return (NULL);
		}

		if (port_attr->pa_state != IBT_PORT_ACTIVE) {
			wait_time = ibdm_ibnex_get_waittime(
				hca_guid, &ibnex_port_settling_time);
			if (wait_time) {
				delay(drv_usectohz(wait_time * 1000000));
			}
			ibdm_ibnex_free_port_attr(port_attr);
			if ((port_attr = ibdm_ibnex_probe_hcaport(
			    hca_guid, port_num)) == NULL) {
				return (NULL);
			}
		}
	}

	port_guid = port_attr->pa_port_guid;
	mutex_enter(&ibnex.ibnex_mutex);
	if ((rval = ibnex_get_dip_from_guid(port_guid, index, pkey,
	    &cdip)) == IBNEX_SUCCESS) {
		IBTF_DPRINTF_L4("ibnex", "\tconfig_port_node: Node exists");
		mutex_exit(&ibnex.ibnex_mutex);
		if (port_num != 0)
			ibdm_ibnex_free_port_attr(port_attr);
		else
			ibdm_ibnex_free_hca_list(hca_list);
		return (cdip);
	}

	if (pkey == 0 && port_num != 0) {
		cdip = ibnex_commsvc_initnode(parent,
		    port_attr, index, IBNEX_PORT_COMMSVC_NODE, pkey, &rval,
		    IBNEX_DEVFS_ENUMERATE);
		IBTF_DPRINTF_L5("ibnex",
		    "\t ibnex_commsvc_initnode rval %x", rval);
	} else if (pkey == 0 && port_num == 0) {
		cdip = ibnex_commsvc_initnode(parent,
		    port_attr, index, IBNEX_HCASVC_COMMSVC_NODE, pkey, &rval,
		    IBNEX_DEVFS_ENUMERATE);
		IBTF_DPRINTF_L5("ibnex",
		    "\t ibnex_commsvc_initnode rval %x", rval);
	} else {
		if (port_attr->pa_state != IBT_PORT_ACTIVE) {
			IBTF_DPRINTF_L4("ibnex", "\tconfig_port_nodes: "
			    "Port %d is down", port_attr->pa_port_num);
			ibdm_ibnex_free_port_attr(port_attr);
			mutex_exit(&ibnex.ibnex_mutex);
			return (NULL);
		}
		for (ii = 0; ii < port_attr->pa_npkeys; ii++) {
			if (pkey == port_attr->pa_pkey_tbl[ii].pt_pkey) {
				cdip = ibnex_commsvc_initnode(parent, port_attr,
				    index, IBNEX_VPPA_COMMSVC_NODE,
				    pkey, &rval, IBNEX_CFGADM_ENUMERATE);
				IBTF_DPRINTF_L5("ibnex",
				    "\t ibnex_commsvc_initnode rval %x", rval);
				break;
			}
		}
	}
	mutex_exit(&ibnex.ibnex_mutex);
	if (port_num != 0)
		ibdm_ibnex_free_port_attr(port_attr);
	else
		ibdm_ibnex_free_hca_list(hca_list);
	return (cdip);
}


/*
 * ibnex_config_obp_args()
 *	Configures a particular port node for a IP over IB communication
 *	service.
 *	The format of the input string "devname" is
 *		port=x,pkey=y,protocol=ip,<wanboot options>
 *	Thr format of the node name created here is
 *		ibport@<Port#>,<pkey>,<service name>
 *	where pkey = 0 for port communication service nodes
 *	Returns "dev_info_t" of the "child" node just created
 *	NULL when failed to enumerate the child node
 *
 */
static dev_info_t *
ibnex_config_obp_args(dev_info_t *parent, char *devname)
{
	int			ii, index;
	int			rval, iter = 0;
	char			*temp;
	uint8_t			port_num;
	ib_guid_t		hca_guid, port_guid;
	ib_pkey_t		pkey;
	dev_info_t		*cdip;
	boolean_t		displayed = B_FALSE;
	ibdm_port_attr_t	*port_attr;

	IBTF_DPRINTF_L4("ibnex", "\tconfig_obp_args: %s", devname);

	/* Is this OBP node for IPoIB ? */
	temp = devname;
	do {
		temp = strstr(temp, ",protocol=ip");
		if (temp == NULL)
			break;

		if (strlen(devname) > (int)((temp - devname) + 12)) {
			if (temp[12] == ',')
				break;
		} else {
			break;
		}
		temp++;
	} while (temp);

	if (temp == NULL)
		return (NULL);
	if (ibnex_prom_devname_to_pkey_n_portnum(
	    devname, &pkey, &port_num) != IBNEX_SUCCESS) {
		return (NULL);
	}
	for (index = 0; index < ibnex.ibnex_nvppa_comm_svcs; index++) {
		if (strcmp(ibnex.ibnex_vppa_comm_svc_names[index],
			"ipib") == 0) {
			break;
		}
	}

	hca_guid = ibtl_ibnex_hcadip2guid(parent);
	if ((port_attr = ibdm_ibnex_probe_hcaport(
	    hca_guid, port_num)) == NULL) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tconfig_port_node: Port does not exist");
		return (NULL);
	}

	/* Wait until "port is up" */
	while (port_attr->pa_state != IBT_PORT_ACTIVE) {
		ibdm_ibnex_free_port_attr(port_attr);
		delay(drv_usectohz(10000));
		if ((port_attr = ibdm_ibnex_probe_hcaport(
		    hca_guid, port_num)) == NULL) {
			return (NULL);
		}
		if (iter++ == 400) {
			if (displayed == B_FALSE) {
				cmn_err(CE_NOTE, "\tWaiting for Port %d "
				    "initialization", port_attr->pa_port_num);
				displayed = B_TRUE;
			}
		}
	}
	IBTF_DPRINTF_L4("ibnex", "\tPort is initialized");

	mutex_enter(&ibnex.ibnex_mutex);
	port_guid = port_attr->pa_port_guid;
	if ((rval = ibnex_get_dip_from_guid(port_guid, index, pkey,
	    &cdip)) == IBNEX_SUCCESS) {
		IBTF_DPRINTF_L4("ibnex", "\tconfig_port_node: Node exists");
		mutex_exit(&ibnex.ibnex_mutex);
		ibdm_ibnex_free_port_attr(port_attr);
		return (cdip);
	}
	for (ii = 0; ii < port_attr->pa_npkeys; ii++) {
		if (pkey == port_attr->pa_pkey_tbl[ii].pt_pkey) {
			cdip = ibnex_commsvc_initnode(parent, port_attr,
			    index, IBNEX_VPPA_COMMSVC_NODE, pkey, &rval,
			    IBNEX_CFGADM_ENUMERATE);
			IBTF_DPRINTF_L5("ibnex",
			    "\t ibnex_commsvc_initnode rval %x", rval);
			break;
		}
	}
	mutex_exit(&ibnex.ibnex_mutex);

	ibdm_ibnex_free_port_attr(port_attr);
	return (cdip);
}


/*
 * ibnex_prom_devname_to_pkey_n_portnum()
 *	Parses the device node name and extracts "PKEY" and "port#"
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_prom_devname_to_pkey_n_portnum(
    char *devname, ib_pkey_t *pkey, uint8_t *port)
{
	int	ret = IBNEX_SUCCESS;
	char	*tmp, *tmp1;

	if ((tmp = strstr(devname, "port=")) != NULL) {
		if ((tmp = strchr(++tmp, '=')) != NULL)
			if ((tmp1 = strchr(++tmp, ',')) != NULL)
				*port = ibnex_str2int(tmp, (tmp1 - tmp), &ret);
	} else
		ret = IBNEX_FAILURE;

	if ((ret == IBNEX_SUCCESS) &&
	    (tmp = strstr(devname, "pkey=")) != NULL) {
		if ((tmp = strchr(++tmp, '=')) != NULL)
			if ((tmp1 = strchr(++tmp, ',')) != NULL)
				*pkey = ibnex_str2hex(tmp, (tmp1 - tmp), &ret);
	} else
		ret = IBNEX_FAILURE;

	return (ret);
}


/*
 * ibnex_get_pkey_commsvc_index_portnum()
 *	Parses the device node name and extracts PKEY, communication
 *	service index & Port #.
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_get_pkey_commsvc_index_portnum(char *device_name, int *index,
    ib_pkey_t *pkey, uint8_t *port_num)
{
	char 	*srv, **service_name, *temp;
	int  	ii, ncommsvcs, ret;

	if (ibnex_devname_to_portnum(device_name, port_num) !=
		IBNEX_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tget_pkey_commsvc_index_portnum: Invalid PortGuid");
		return (NULL);
	}
	srv = strchr(device_name, ',');
	if (srv == 0)
		return (IBNEX_FAILURE);

	srv++;
	temp = strchr(srv, ',');
	if (temp == 0)
		return (IBNEX_FAILURE);
	temp++;
	*pkey = ibnex_str2hex(srv, (temp - srv - 1), &ret);
	if (ret != IBNEX_SUCCESS)
		return (ret);

	if (*pkey == 0 && *port_num != 0) {
		service_name = ibnex.ibnex_comm_svc_names;
		ncommsvcs = ibnex.ibnex_num_comm_svcs;
	} else if (*pkey == 0 && *port_num == 0) {
		service_name = ibnex.ibnex_hcasvc_comm_svc_names;
		ncommsvcs = ibnex.ibnex_nhcasvc_comm_svcs;
	} else {
		service_name = ibnex.ibnex_vppa_comm_svc_names;
		ncommsvcs = ibnex.ibnex_nvppa_comm_svcs;
	}

	for (ii = 0; ii < ncommsvcs; ii++) {
		if (strcmp(service_name[ii], temp) == 0) {
			break;
		}
	}
	if (ii == ncommsvcs)
		return (IBNEX_FAILURE);

	*index = ii;
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_devname_to_portnum()
 *	Get portguid from device name
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_devname_to_portnum(char *device_name, uint8_t *portnum)
{
	int	ret;
	char	*temp1, *temp2;

	temp1 = strchr(device_name, '@');
	if (temp1 == NULL) {
		return (IBNEX_FAILURE);
	}
	temp2 = strchr(temp1, ',');
	if (temp2 == NULL)
		return (IBNEX_FAILURE);
	temp1++;
	*portnum = ibnex_str2hex(temp1, (temp2 - temp1), &ret);
	return (ret);
}


/*
 * ibnex_config_ioc_node()
 *	Configures one particular instance of the IOC driver.
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_config_ioc_node(char *device_name)
{
	int			ret;
	time_t			wait_time;
	ib_guid_t		iou_guid, ioc_guid;
	ibdm_ioc_info_t		*ioc_info;

	IBTF_DPRINTF_L4("ibnex", "\tconfig_ioc_node: Begin");

	if (ibnex_devname_to_node_n_ioc_guids(
	    device_name, &iou_guid, &ioc_guid) != IBNEX_SUCCESS) {
		return (IBNEX_FAILURE);
	}

	wait_time = ibdm_ibnex_get_waittime(0, &ibnex_port_settling_time);
	if (wait_time)
		delay(drv_usectohz(wait_time * 1000000));

	if ((ioc_info = ibdm_ibnex_probe_ioc(iou_guid, ioc_guid, 0)) ==
	    NULL) {
		ibdm_ibnex_free_ioc_list(ioc_info);
		return (IBNEX_FAILURE);
	}
	mutex_enter(&ibnex.ibnex_mutex);
	if (ibnex_is_ioc_present(ioc_guid) == IBNEX_SUCCESS) {
		IBTF_DPRINTF_L4("ibnex", "\tconfig_ioc_node: IOC present");
		ret = IBNEX_SUCCESS;
	} else
		ret = ibnex_ioc_initnode(ioc_info, IBNEX_DEVFS_ENUMERATE);
	mutex_exit(&ibnex.ibnex_mutex);
	ibdm_ibnex_free_ioc_list(ioc_info);
	return (ret);
}


/*
 * ibnex_devname_to_node_n_ioc_guids()
 *	Get node guid and ioc guid from the device name
 *	Format of the device node name is:
 *		ioc@<IOC GUID>,<IOU GUID>
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_devname_to_node_n_ioc_guids(
    char *device_name, ib_guid_t *iou_guid, ib_guid_t *ioc_guid)
{
	char	*temp1, *temp;
	int	len, ret;

	IBTF_DPRINTF_L4("ibnex", "\tdevname_to_node_n_ioc_guids:"
	    "Device Name %s", device_name);

	if ((temp = strchr(device_name, '@')) == NULL) {
		return (IBNEX_FAILURE);
	}
	if ((temp1 = strchr(++temp, ',')) == NULL) {
		return (IBNEX_FAILURE);
	}
	*ioc_guid = ibnex_str2hex(temp, (temp1 - temp), &ret);
	if (ret == IBNEX_SUCCESS) {
		len = device_name + strlen(device_name) - ++temp1;
		*iou_guid = ibnex_str2hex(temp1, len, &ret);
	}
	return (ret);
}


/*ARGSUSED*/
/*
 * ibnex_ioc_initnode()
 *	Allocate a pathinfo node for the IOC
 *	Initialize the device node
 *	Bind driver to the node
 *	Update IBnex global data
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE/IBNEX_BUSY
 */
int
ibnex_ioc_initnode(ibdm_ioc_info_t *ioc_info, int flag)
{
	int			rval;
	ibnex_node_data_t	*node_data;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	node_data = ibnex_is_node_data_present(IBNEX_IOC_NODE,
	    (void *)ioc_info, 0, 0);

	/*
	 * prevent any races
	 * we have seen this node_data and it has been initialized
	 * Note that node_dip is already NULL if unconfigure is in
	 * progress.
	 */
	if (node_data && node_data->node_dip) {
		return ((node_data->node_state == IBNEX_CFGADM_CONFIGURING) ?
		    IBNEX_BUSY : IBNEX_SUCCESS);
	} else if (node_data == NULL) {
		node_data = ibnex_init_child_nodedata(IBNEX_IOC_NODE,
		    ioc_info, 0, 0);
	}

	/*
	 * Return EBUSY if another configure/unconfigure
	 * operation is in progress
	 */
	if (node_data->node_state == IBNEX_CFGADM_UNCONFIGURING) {
		return (IBNEX_BUSY);
	}

	ASSERT(node_data->node_state != IBNEX_CFGADM_CONFIGURED);
	node_data->node_state = IBNEX_CFGADM_CONFIGURING;


	mutex_exit(&ibnex.ibnex_mutex);

	rval = ibnex_ioc_create_pi(ioc_info, node_data);

	mutex_enter(&ibnex.ibnex_mutex);
	if (rval == IBNEX_SUCCESS)
		node_data->node_state = IBNEX_CFGADM_CONFIGURED;

	return (rval);
}


/*
 * ibnex_config_pseudo_all()
 *	Configure all the pseudo nodes
 */
static void
ibnex_config_pseudo_all(void)
{
	ibnex_node_data_t	*nodep;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	for (nodep = ibnex.ibnex_pseudo_node_head;
	    nodep; nodep = nodep->node_next) {
		(void) ibnex_pseudo_config_one(nodep, NULL, NULL);
	}
}


/*
 * ibnex_pseudo_config_one()
 */
int
ibnex_pseudo_config_one(ibnex_node_data_t *node_data, char *cname, char *caddr)
{
	int			rval, len;
	char			*node_addr;

	IBTF_DPRINTF_L4("ibnex", "\tpseudo_config_one:Begin");

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	if (node_data == NULL) {
		IBTF_DPRINTF_L4("ibnex", "\tpseudo_config_one:"
		    "cname = %s caddr = %s", cname, caddr);

		len = strlen(cname) + strlen(caddr) + 2;
		node_addr = (char *)kmem_alloc(len, KM_SLEEP);

		(void) snprintf(node_addr, len, "%s,%s", cname, caddr);
		node_data = ibnex_is_node_data_present(IBNEX_PSEUDO_NODE,
		    (void *)node_addr, 0, 0);
		kmem_free(node_addr, len);
	}

	/*
	 * prevent any races
	 * we have seen this node_data and it has been initialized
	 * Note that node_dip is already NULL if unconfigure is in
	 * progress.
	 */
	if (node_data && node_data->node_dip) {
		return ((node_data->node_state == IBNEX_CFGADM_CONFIGURING) ?
		    IBNEX_BUSY : IBNEX_SUCCESS);
	} else if (node_data == NULL) {
		IBTF_DPRINTF_L2("ibnex", "\tpseudo_config_one: Invalid node");
		return (IBNEX_FAILURE);
	}

	/*
	 * Return EBUSY if another configure/unconfigure
	 * operation is in progress
	 */
	if (node_data->node_state == IBNEX_CFGADM_UNCONFIGURING) {
		return (IBNEX_BUSY);
	}

	if (node_data->node_state == IBNEX_CFGADM_CONFIGURED)
		return (IBNEX_SUCCESS);

	/*
	 * Prevent configuring pseudo nodes specifically unconfigured
	 * by cfgadm. This is done by checking if this is a newly
	 * created node, not yet configured by BUS_CONFIG or cfgadm
	 */
	if (node_data->node_data.pseudo_node.pseudo_new_node != 1)
		return (IBNEX_FAILURE);
	node_data->node_data.pseudo_node.pseudo_new_node = 0;

	node_data->node_state = IBNEX_CFGADM_CONFIGURING;

	mutex_exit(&ibnex.ibnex_mutex);
	rval = ibnex_pseudo_create_pi(node_data);
	mutex_enter(&ibnex.ibnex_mutex);

	if (rval == IBNEX_SUCCESS)
		node_data->node_state = IBNEX_CFGADM_CONFIGURED;
	else {
		node_data->node_dip = NULL;
		node_data->node_state = IBNEX_CFGADM_UNCONFIGURED;
		node_data->node_data.pseudo_node.pseudo_new_node = 1;
	}

	return (rval);
}


/*
 * ibnex_pseudo_create_pi()
 *	Create a path info node for each pseudo entry
 */
int
ibnex_pseudo_create_pi(ibnex_node_data_t *nodep)
{
	mdi_pathinfo_t		*pip;
	int			rval, hcacnt;
	dev_info_t		*hca_dip, *cdip = NULL;
	ibdm_hca_list_t		*hca_list, *head;
	ibnex_pseudo_node_t	*pseudo;

	IBTF_DPRINTF_L4("ibnex", "\tibnex_pseudo_create_pi: %p", nodep);

	pseudo = &nodep->node_data.pseudo_node;

	ibdm_ibnex_get_hca_list(&hca_list, &hcacnt);

	head = hca_list;

	for (; hca_list != NULL; hca_list = hca_list->hl_next) {

		hca_dip = ibtl_ibnex_hcaguid2dip(hca_list->hl_hca_guid);

		rval = mdi_pi_alloc(hca_dip,
		    pseudo->pseudo_devi_name, pseudo->pseudo_node_addr,
		    pseudo->pseudo_node_addr, 0, &pip);

		if (rval != MDI_SUCCESS) {
			(void) ibnex_offline_childdip(cdip);
			return (IBNEX_FAILURE);
		}
		cdip = mdi_pi_get_client(pip);

		IBTF_DPRINTF_L4("ibnex",
		    "\tpseudo_create_pi: New dip %p", cdip);

		nodep->node_dip = cdip;
		ddi_set_parent_data(cdip, nodep);

		rval = mdi_pi_online(pip, 0);

		if (rval != MDI_SUCCESS) {
			ddi_set_parent_data(cdip, NULL);
			IBTF_DPRINTF_L2("ibnex", "\tpseudo_create_pi:"
			    "mdi_pi_online: failed for pseudo dip %p,"
			    " rval %d", cdip, rval);
			(void) ibnex_offline_childdip(cdip);
			rval = IBNEX_FAILURE;
			break;
		} else
			rval = IBNEX_SUCCESS;
	}
	if (head)
		ibdm_ibnex_free_hca_list(head);
	return (rval);
}


/*
 * ibnex_ioc_create_pi()
 *	Create a pathinfo node for the ioc node
 */
static int
ibnex_ioc_create_pi(ibdm_ioc_info_t *ioc_info, ibnex_node_data_t *node_data)
{
	char			ioc_guid[33], iou_guid[33];
	mdi_pathinfo_t		*pip;
	int			rval;
	dev_info_t		*hca_dip, *cdip = NULL;
	int			flag = 1;
	ibdm_hca_list_t		*hca_list;

	IBTF_DPRINTF_L4("ibnex", "\tibnex_ioc_create_pi Begin");

	(void) snprintf(ioc_guid, 33, "%llx",
		(longlong_t)ioc_info->ioc_profile.ioc_guid);
	(void) snprintf(iou_guid, 33, "%llx",
		(longlong_t)ioc_info->ioc_iou_guid);

	hca_list = ioc_info->ioc_hca_list;

	for (; hca_list != NULL; hca_list = hca_list->hl_next) {

		hca_dip = ibtl_ibnex_hcaguid2dip(hca_list->hl_hca_guid);

		IBTF_DPRINTF_L4("ibnex", "\tioc_create_pi "
		    "hca guid %llx", hca_list->hl_hca_guid);

		rval =  mdi_pi_alloc(hca_dip,
		    IBNEX_IOC_CNAME, ioc_guid, iou_guid, 0, &pip);
		if (rval != MDI_SUCCESS) {
			(void) ibnex_offline_childdip(cdip);
			return (IBNEX_FAILURE);
		}
		cdip = mdi_pi_get_client(pip);

		IBTF_DPRINTF_L4("ibnex",
		    "\tioc_create_pi: New IOC dip %p", cdip);

		node_data->node_dip = cdip;
		ddi_set_parent_data(cdip, node_data);

		if (flag) {
			if ((rval = ibnex_create_ioc_node_prop(
			    ioc_info, cdip)) != IBNEX_SUCCESS) {
				ibnex_delete_ioc_node_data(node_data);
				ddi_prop_remove_all(cdip);
				ddi_set_parent_data(cdip, NULL);

				(void) ibnex_offline_childdip(cdip);
				return (IBNEX_FAILURE);
			}
			flag = 0;
		}

		rval = mdi_pi_online(pip, 0);

		if (rval != MDI_SUCCESS) {
			ibnex_delete_ioc_node_data(node_data);
			ddi_prop_remove_all(cdip);
			ddi_set_parent_data(cdip, NULL);
			IBTF_DPRINTF_L2("ibnex", "\tioc_create_pi: "
			    "mdi_pi_online() failed ioc dip %p, rval %d",
			    cdip, rval);
			(void) ibnex_offline_childdip(cdip);
			rval = IBNEX_FAILURE;
			break;
		} else
			rval = IBNEX_SUCCESS;
	}
	return (rval);
}


/*
 * ibnex_create_ioc_node_prop()
 *	Create IOC device node properties
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_create_ioc_node_prop(ibdm_ioc_info_t *ioc_info, dev_info_t *cdip)
{
	uint16_t		 capabilities;
	ib_dm_ioc_ctrl_profile_t *ioc_profile = &ioc_info->ioc_profile;

	IBTF_DPRINTF_L4("ibnex", "\tcreate_ioc_node_prop");

	if (ibnex_create_ioc_compatible_prop(cdip,
	    ioc_profile) != IBNEX_SUCCESS) {
		return (IBNEX_FAILURE);
	}
	if ((ioc_info->ioc_iou_dc_valid) &&
	    (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "iou-diagcode",
	    ioc_info->ioc_iou_diagcode)) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: iou-diagcode create failed");
		return (IBNEX_FAILURE);
	}
	if ((ioc_info->ioc_diagdeviceid) && (ioc_info->ioc_dc_valid)) {
		if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "ioc-diagcode",
		    ioc_info->ioc_diagcode) != DDI_PROP_SUCCESS) {
			IBTF_DPRINTF_L2("ibnex", "\tcreate_ioc_node_prop: "
			    "ioc-diagcode create failed");
			return (IBNEX_FAILURE);
		}
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "rdma-queue-depth",
	    ioc_profile->ioc_rdma_read_qdepth) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: rdma-queue-depth create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "rdma-transfer-size",
	    ioc_profile->ioc_rdma_xfer_sz) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex", "\tcreate_ioc_node_prop: "
		    "rdma-transfer-size create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "send-message-size",
	    ioc_profile->ioc_send_msg_sz) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: send-message-size create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "send-queue-depth",
	    ioc_profile->ioc_send_msg_qdepth) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: send-queue-depth create failed");
		return (IBNEX_FAILURE);
	}

	capabilities = (ioc_profile->ioc_ctrl_opcap_mask << 8);
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip,
		"capabilities", capabilities) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: capabilities create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_string(DDI_DEV_T_NONE, cdip, "id-string",
	    (char *)ioc_profile->ioc_id_string) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: id-string failed");
		return (IBNEX_FAILURE);
	}

	/*
	 * Create properties to represent all the service entries supported
	 * by the IOC. Each service entry consists of 1) Service ID (64 bits)
	 * and 2) Service name (40 bytes). The service entry table is
	 * represented by two properties, service-ids and service-names. The
	 * service-id property is a array of int64's and service names is
	 * array of strings. The first element in the "service-ids" property
	 * corresponds to first string in the "service-names" and so on.
	 */
	if ((ioc_profile->ioc_service_entries != 0) &&
	    (ibnex_create_ioc_srv_props(cdip, ioc_info) != IBNEX_SUCCESS))
		return (IBNEX_FAILURE);

	/* Create destination port GID properties */
	if (ibnex_create_ioc_portgid_prop(cdip, ioc_info) != IBNEX_SUCCESS)
		return (IBNEX_FAILURE);

	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "protocol-version",
	    ioc_profile->ioc_protocol_ver) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: protocol-version create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "protocol",
	    ioc_profile->ioc_protocol) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: protocol create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "io-subclass",
	    ioc_profile->ioc_io_subclass) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: subclass create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "io-class",
	    ioc_profile->ioc_io_class) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: class prop create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "subsystem-id",
	    ioc_profile->ioc_subsys_id) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: subsys_id create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "subsystem-vendor-id",
	    ioc_profile->ioc_subsys_vendorid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: subsystem vendor create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int64(DDI_DEV_T_NONE, cdip, "ioc-guid",
	    ioc_profile->ioc_guid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: protocol create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "device-version",
	    ioc_profile->ioc_device_ver) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: product-id create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "device-id",
	    ioc_profile->ioc_deviceid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: product-id create failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "vendor-id",
	    ioc_profile->ioc_vendorid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_node_prop: vendor-id create failed");
		return (IBNEX_FAILURE);
	}
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_create_ioc_portgid_prop()
 *	Creates "port-entries", "port-list" properties
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_create_ioc_portgid_prop(
    dev_info_t *cdip, ibdm_ioc_info_t *ioc_info)
{
	uint64_t	*port_gids;
	int		length, ii, jj;
	int		prop_len;
	ibnex_node_data_t *node_data;

	IBTF_DPRINTF_L4("ibnex", "\tcreate_ioc_portgid_prop");

	node_data = ddi_get_parent_data(cdip);
	ASSERT(node_data);

	prop_len = (ioc_info->ioc_nportgids != 0) ?
	    (2 * ioc_info->ioc_nportgids) : 1;
	length = sizeof (uint64_t) * prop_len;
	port_gids = kmem_zalloc(length, KM_SLEEP);

	for (ii = 0, jj = 0; ii < ioc_info->ioc_nportgids; ii++) {
		port_gids[jj++] = ioc_info->ioc_gid_list[ii].gid_dgid_hi;
		port_gids[jj++] = ioc_info->ioc_gid_list[ii].gid_dgid_lo;
	}
	if (ndi_prop_update_int64_array(DDI_DEV_T_NONE, cdip, "port-list",
	    (int64_t *)port_gids, prop_len) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_portgid_prop: port-list create failed");
		kmem_free(port_gids, length);
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "port-entries",
	    ioc_info->ioc_nportgids) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_portgid_prop: port-entries create failed");
		kmem_free(port_gids, length);
		return (IBNEX_FAILURE);
	}

	kmem_free(port_gids, length);
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_create_ioc_srv_props()
 *	Creates "service-name" and "service-id" properties
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_create_ioc_srv_props(
    dev_info_t *cdip, ibdm_ioc_info_t *ioc_info)
{
	int			length, ii;
	uint64_t		*srv_id;
	char			*temp, *srv_names[IB_DM_MAX_IOCS_IN_IOU];
	ib_dm_ioc_ctrl_profile_t *profile = &ioc_info->ioc_profile;
	ibdm_srvents_info_t	 *srvents = ioc_info->ioc_serv;

	IBTF_DPRINTF_L4("ibnex", "\tcreate_ioc_srv_props");

	length = profile->ioc_service_entries * sizeof (ib_dm_srv_t);
	srv_id = kmem_zalloc(length, KM_SLEEP);
	temp = (char *)((char *)srv_id + (8 * profile->ioc_service_entries));
	for (ii = 0; ii < profile->ioc_service_entries; ii++) {
		srv_names[ii] = (char *)temp + (ii * IB_DM_MAX_SVC_NAME_LEN);
	}

	for (ii = 0; ii < profile->ioc_service_entries; ii++) {
		srv_id[ii] = srvents[ii].se_attr.srv_id;
		bcopy(srvents[ii].se_attr.srv_name,
		    srv_names[ii], (IB_DM_MAX_SVC_NAME_LEN - 1));
		IBTF_DPRINTF_L4("ibnex", "\tcreate_ioc_srv_props "
		    "Service Names : %s", srv_names[ii]);
		IBTF_DPRINTF_L4("ibnex", "\tcreate_ioc_srv_props "
		    "Service ID : %llx", srv_id[ii]);
	}

	if (ndi_prop_update_int64_array(DDI_DEV_T_NONE, cdip,
	    "service-id", (int64_t *)srv_id,
	    profile->ioc_service_entries) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_srv_props: service-id create failed");
		kmem_free(srv_id, length);
		return (IBNEX_FAILURE);
	}

	if (ndi_prop_update_string_array(DDI_DEV_T_NONE, cdip,
	    "service-name", (char **)srv_names,
	    profile->ioc_service_entries) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_ioc_srv_props: service-name create failed");
		kmem_free(srv_id, length);
		return (IBNEX_FAILURE);
	}
	kmem_free(srv_id, length);
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_create_ioc_compatible_prop()
 *	Creates "compatible" property values
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_create_ioc_compatible_prop(
    dev_info_t *cdip, ib_dm_ioc_ctrl_profile_t *ioc_profile)
{
	char		*temp;
	int		rval, ii;
	char		*compatible[IBNEX_MAX_COMPAT_NAMES];

	/*
	 * Initialize the "compatible" property string as below:
	 * Compatible Strings :
	 *	1. ib.V<vid>P<pid>S<subsys vid>s<subsys id>v<ver>
	 *	2. ib.V<vid>P<pid>S<subsys vid>s<subsys id>
	 *	3. ib.V<vid>P<pid>v<ver>
	 *	4. ib.V<vid>P<pid>
	 *	5. ib.C<Class>c<Subclass>p<protocol>r<protocol ver>
	 *	6. ib.C<Class>c<Subclass>p<protocol>
	 *
	 * Note:
	 *	All leading zeros must be present
	 *	All numeric values must specified in hex without prefix "0x"
	 */

	temp = kmem_alloc(IBNEX_MAX_COMPAT_PROP_SZ, KM_SLEEP);
	for (ii = 0; ii < IBNEX_MAX_COMPAT_NAMES; ii++)
		compatible[ii] = temp + (ii * IBNEX_MAX_COMPAT_LEN);

	(void) snprintf(compatible[0], IBNEX_MAX_COMPAT_LEN,
	    "ib.V%06xP%08xS%06xs%08xv%04x",
	    ioc_profile->ioc_vendorid, ioc_profile->ioc_deviceid,
	    ioc_profile->ioc_subsys_vendorid, ioc_profile->ioc_subsys_id,
	    ioc_profile->ioc_device_ver);

	(void) snprintf(compatible[1], IBNEX_MAX_COMPAT_LEN,
	    "ib.V%06xP%08xS%06xs%08x",
	    ioc_profile->ioc_vendorid, ioc_profile->ioc_deviceid,
	    ioc_profile->ioc_subsys_vendorid, ioc_profile->ioc_subsys_id);

	(void) snprintf(compatible[2], IBNEX_MAX_COMPAT_LEN,
	    "ib.V%06xP%08xv%04x",
	    ioc_profile->ioc_vendorid, ioc_profile->ioc_deviceid,
	    ioc_profile->ioc_device_ver);

	(void) snprintf(compatible[3], IBNEX_MAX_COMPAT_LEN,
	    "ib.V%06xP%08x",
	    ioc_profile->ioc_vendorid, ioc_profile->ioc_deviceid);

	(void) snprintf(compatible[4], IBNEX_MAX_COMPAT_LEN,
	    "ib.C%04xc%04xp%04xr%04x",
	    ioc_profile->ioc_io_class, ioc_profile->ioc_io_subclass,
	    ioc_profile->ioc_protocol, ioc_profile->ioc_protocol_ver);

	(void) snprintf(compatible[5], IBNEX_MAX_COMPAT_LEN,
	    "ib.C%04xc%04xp%04x",
	    ioc_profile->ioc_io_class, ioc_profile->ioc_io_subclass,
	    ioc_profile->ioc_protocol);
	for (ii = 0; ii < IBNEX_MAX_COMPAT_NAMES; ii++)
		IBTF_DPRINTF_L4("ibnex", "\tcompatible: %s", compatible[ii]);

	/* Create the compatible property for child cdip */
	rval = ndi_prop_update_string_array(DDI_DEV_T_NONE, cdip,
	    "compatible", (char **)compatible, IBNEX_MAX_COMPAT_NAMES);

	if (rval != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex", "\tcompatible prop_create failed");
		kmem_free(temp, IBNEX_MAX_COMPAT_PROP_SZ);
		return (IBNEX_FAILURE);
	}

	kmem_free(temp, IBNEX_MAX_COMPAT_PROP_SZ);
	return (IBNEX_SUCCESS);
}


static void
ibnex_ioc_node_cleanup()
{
	ibnex_node_data_t *node, *delete;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));
	for (node = ibnex.ibnex_ioc_node_head; node; ) {
		delete = node;
		node = node->node_next;
		mutex_exit(&ibnex.ibnex_mutex);
		ibnex_delete_ioc_node_data(delete);
		mutex_enter(&ibnex.ibnex_mutex);
	}
}

/*
 * ibnex_delete_ioc_node_data()
 *	Delete IOC node from the list
 */
static void
ibnex_delete_ioc_node_data(ibnex_node_data_t *node)
{
	IBTF_DPRINTF_L4("ibnex", "\tdelete_ioc_node_data:");

	mutex_enter(&ibnex.ibnex_mutex);
	if ((node->node_next == NULL) && (node->node_prev == NULL)) {
		ASSERT(ibnex.ibnex_ioc_node_head == node);
		ibnex.ibnex_ioc_node_head = NULL;
	} else if (node->node_next == NULL)
		node->node_prev->node_next = NULL;
	else if (node->node_prev == NULL) {
		node->node_next->node_prev = NULL;
		ibnex.ibnex_ioc_node_head = node->node_next;
	} else {
		node->node_prev->node_next = node->node_next;
		node->node_next->node_prev = node->node_prev;
	}
	IBTF_DPRINTF_L4("ibnex", "\tdelete_ioc_node_data: head %p",
	    ibnex.ibnex_ioc_node_head);
	mutex_exit(&ibnex.ibnex_mutex);
	kmem_free(node, sizeof (ibnex_node_data_t));
}


/*
 * ibnex_dm_callback()
 *
 *	This routine is registered with the IBDM during IB nexus attach. It
 *	is called by the IBDM module when it discovers
 *		New HCA port
 *		HCA port removal
 *		New HCA added
 *		HCA removed
 */
void
ibnex_dm_callback(void *arg, ibdm_events_t flag)
{
	char	hca_guid[IBNEX_HCAGUID_STRSZ];
	ibdm_ioc_info_t	*ioc_list, *ioc;
	ibnex_node_data_t	*node_data;

	IBTF_DPRINTF_L4("ibnex", "\tdm_callback: attr %p event %x", arg, flag);

	switch (flag) {
	case IBDM_EVENT_HCA_ADDED:
		(void) snprintf(hca_guid, IBNEX_HCAGUID_STRSZ, "%llX",
		    (*(longlong_t *)arg));
		/* Create a devctl minor node for the HCA's port  */
		if (ddi_create_minor_node(ibnex.ibnex_dip, hca_guid, S_IFCHR,
		    ddi_get_instance(ibnex.ibnex_dip),
		    DDI_NT_IB_ATTACHMENT_POINT, 0) != DDI_SUCCESS) {
			IBTF_DPRINTF_L4("ibnex", "\tdm_callback: failed to "
			    "create minor node for port w/ guid %s", hca_guid);
		}

		break;

	case IBDM_EVENT_HCA_REMOVED:
		(void) snprintf(hca_guid, IBNEX_HCAGUID_STRSZ, "%llX",
		    (*(longlong_t *)arg));
		ddi_remove_minor_node(ibnex.ibnex_dip, hca_guid);
		break;

	case IBDM_EVENT_IOC_PROP_UPDATE:
		ioc = ioc_list = (ibdm_ioc_info_t *)arg;
		if (ioc_list == NULL)
			break;

		mutex_enter(&ibnex.ibnex_mutex);
		while (ioc_list) {
			if ((node_data = ibnex_is_node_data_present(
			    IBNEX_IOC_NODE, ioc_list, 0, 0)) != NULL &&
			    node_data->node_dip != NULL) {
				ibnex_update_prop(node_data, ioc_list);
			}
			ioc_list = ioc_list->ioc_next;
		}
		mutex_exit(&ibnex.ibnex_mutex);
		ibdm_ibnex_free_ioc_list(ioc);
	}
}


/*
 * ibnex_get_dip_from_guid()
 *
 *	Searches the linked list of the port nodes and returns the dip for
 *	the of the Port / Node guid requested.
 *	Returns NULL if not found
 */
int
ibnex_get_dip_from_guid(ib_guid_t guid, int index, ib_pkey_t pkey,
    dev_info_t **dip)
{
	int			node_index;
	ib_guid_t		node_guid;
	ib_pkey_t		node_pkey;
	ibnex_node_data_t	*node_data;

	IBTF_DPRINTF_L4("ibnex",
	    "\tget_dip_from_guid: guid = %llx", guid);

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));
	/* Search for a matching entry in internal lists */
	node_data = ibnex.ibnex_port_node_head;
	while (node_data) {
		node_guid = node_data->node_data.port_node.port_guid;
		node_index = node_data->node_data.port_node.port_commsvc_idx;
		node_pkey = node_data->node_data.port_node.port_pkey;
		if ((node_guid == guid) && (index == node_index) &&
		    (node_pkey == pkey)) {
			break;
		}
		node_data = node_data->node_next;
	}

	/* matching found with a valid dip */
	if (node_data && node_data->node_dip) {
		*dip = node_data->node_dip;
		return (IBNEX_SUCCESS);
	} else if (node_data && !node_data->node_dip) {	/* dip is invalid */
		*dip = NULL;
		return (IBNEX_SUCCESS);
	}

	/* no match found */
	*dip = NULL;
	return (IBNEX_FAILURE);
}


/*
 * ibnex_comm_svc_init()
 *	Read the property and cache the values in the global
 *	structure.
 *	Check for max allowed length (4 bytes) of service name
 *	(each element of the property)
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static ibnex_rval_t
ibnex_comm_svc_init(char *property, ibnex_node_type_t type)
{
	int		i, len, count;
	int		ncomm_svcs;
	char		**comm_svcp;
	char		**servicep = NULL;
	uint_t		nservices = 0;
	int			*valid = NULL;

	IBTF_DPRINTF_L4("ibnex", "\tcomm_svc_init : %s property, type = %x",
	    property, type);

	/* lookup the string array property */
	if (ddi_prop_lookup_string_array(DDI_DEV_T_ANY, ibnex.ibnex_dip,
	    DDI_PROP_DONTPASS, property, &servicep, &nservices) !=
	    DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex", "\t%s property undefined", property);
		return (IBNEX_SUCCESS);
	}

	if (nservices)
		valid = kmem_zalloc(nservices * sizeof (int), KM_SLEEP);


	/* first read the file to get a count of valid service entries */
	for (ncomm_svcs = 0, count = 0; count < nservices; count++) {
		int j;

		len = strlen(servicep[count]);
		if (len == 0 || len > 4) {
			IBTF_DPRINTF_L2("ibnex", "\tcomm_svc_init : "
			    "Service name %s invalid : length %d",
			    servicep[count], len);
			continue;
		}
		if (ibnex_unique_svcname(servicep[count]) != IBNEX_SUCCESS) {
			IBTF_DPRINTF_L2("ibnex", "\tcomm_svc_init : "
			    "Service name %s invalid : Not unique",
			    servicep[count]);
			continue;
		}

		/*
		 * ibnex_unique_svcname checks for uniqueness in service names
		 * communication services fully initialized. Check uniqueness
		 * in service names currently initialized.
		 */
		for (j = 0; j < count; j++)
			if (valid[j] && strncmp(servicep[count],
			    servicep[j], 4) == 0) {
				IBTF_DPRINTF_L2("ibnex", "\tcomm_svc_init : "
				    "Service name %s invalid : Not unique",
				    servicep[count]);
					continue;
			}

		valid[count] = 1;
		ncomm_svcs++;
	}

	/* if no valid entries found, bailout */
	if (nservices == 0 || ncomm_svcs == 0) {
		IBTF_DPRINTF_L4("ibnex", "\tNo %s entries found", property);
		ddi_prop_free(servicep); /* free the property */
		if (valid)
			kmem_free(valid, nservices * sizeof (int));
		return (IBNEX_SUCCESS);
	}

	comm_svcp = kmem_zalloc((ncomm_svcs * sizeof (char *)), KM_SLEEP);
	if (type == IBNEX_PORT_COMMSVC_NODE) {
		ibnex.ibnex_comm_svc_names = comm_svcp;
		ibnex.ibnex_num_comm_svcs = ncomm_svcs;
	} else if (type == IBNEX_VPPA_COMMSVC_NODE) {
		ibnex.ibnex_vppa_comm_svc_names = comm_svcp;
		ibnex.ibnex_nvppa_comm_svcs = ncomm_svcs;
	} else if (type == IBNEX_HCASVC_COMMSVC_NODE) {
		ibnex.ibnex_hcasvc_comm_svc_names = comm_svcp;
		ibnex.ibnex_nhcasvc_comm_svcs = ncomm_svcs;
	}

	/* copy the services into an array of strings */
	for (i = 0, count = 0; count < nservices; count++) {
		if (valid[count] == 0)	/* Skip invalid ones */
			continue;
		comm_svcp[i] = kmem_alloc(len + 1, KM_SLEEP);
		(void) strcpy(comm_svcp[i], servicep[count]);
		IBTF_DPRINTF_L4("ibnex",
		    "\t\tService [%d]: %s", i, comm_svcp[i]);
		++i;
	}
	ddi_prop_free(servicep);
	kmem_free(valid, nservices * sizeof (int));
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_comm_svc_fini()
 *	Deallocate all the memory allocated for the communication
 *	service arrays.
 */
static void
ibnex_comm_svc_fini()
{
	int	index;

	for (index = 0; index < ibnex.ibnex_num_comm_svcs; index++) {
		kmem_free(ibnex.ibnex_comm_svc_names[index],
		    (strlen(ibnex.ibnex_comm_svc_names[index]) + 1));
	}
	if (ibnex.ibnex_comm_svc_names) {
		kmem_free(ibnex.ibnex_comm_svc_names,
		    ibnex.ibnex_num_comm_svcs * sizeof (char *));
	}
	for (index = 0; index < ibnex.ibnex_nvppa_comm_svcs; index++) {
		kmem_free(ibnex.ibnex_vppa_comm_svc_names[index],
		    strlen(ibnex.ibnex_vppa_comm_svc_names[index]) +1);
	}
	if (ibnex.ibnex_vppa_comm_svc_names) {
		kmem_free(ibnex.ibnex_vppa_comm_svc_names,
		    ibnex.ibnex_nvppa_comm_svcs * sizeof (char *));
	}
	for (index = 0; index < ibnex.ibnex_nhcasvc_comm_svcs; index++) {
		kmem_free(ibnex.ibnex_hcasvc_comm_svc_names[index],
		    strlen(ibnex.ibnex_hcasvc_comm_svc_names[index]) +1);
	}
	if (ibnex.ibnex_hcasvc_comm_svc_names) {
		kmem_free(ibnex.ibnex_hcasvc_comm_svc_names,
		    ibnex.ibnex_nhcasvc_comm_svcs * sizeof (char *));
	}
	ibnex.ibnex_comm_svc_names = NULL;
	ibnex.ibnex_num_comm_svcs = 0;
	ibnex.ibnex_vppa_comm_svc_names = NULL;
	ibnex.ibnex_nvppa_comm_svcs = 0;
	ibnex.ibnex_hcasvc_comm_svc_names = NULL;
	ibnex.ibnex_nhcasvc_comm_svcs = 0;
}


/*
 * ibnex_commsvc_initnode()
 *	This routine is specific to port/VPPA node creation
 *	Creates a device node for the comm service specified by commsvc_index
 *	Creates all the device node properties
 *	Allocates and initializes the node specific data
 *	Binds the device driver of the device node
 *	Returns "dev_info_t" of the child node or NULL in case of failure
 *	Sets IBNEX_SUCCESS/IBNEX_FAILURE/IBNEX_BUSY in "rval" to reflect
 *	if the operation was successful, failed or was not performed.
 */
dev_info_t *
ibnex_commsvc_initnode(dev_info_t *parent, ibdm_port_attr_t *port_attr,
    int index, int node_type, ib_pkey_t pkey, int *rval, int flag)
{
	int			ret;
	char			*svcname;
	dev_info_t		*cdip;
	ibnex_node_data_t	*node_data;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	*rval = IBNEX_SUCCESS;

	/*
	 * prevent any races
	 * we have seen this node_data and it has been initialized
	 * Note that node_dip is already NULL if unconfigure is in
	 * progress.
	 */
	node_data = ibnex_is_node_data_present(node_type, (void *)port_attr,
	    index, pkey);
	if (node_data && node_data->node_dip) {
		/*
		 * Return NULL if another configure
		 * operation is in progress
		 */
		if (node_data->node_state == IBNEX_CFGADM_CONFIGURING) {
			*rval = IBNEX_BUSY;
			return (NULL);
		} else {
			return (node_data->node_dip);
		}
	} else if (node_data == NULL) {
		/* allocate a new ibnex_node_data_t */
		node_data = ibnex_init_child_nodedata(node_type, port_attr,
		    index, pkey);
	}

	/*
	 * Return NULL if another unconfigure operation is in progress
	 */
	if (node_data->node_state == IBNEX_CFGADM_UNCONFIGURING) {
		*rval = IBNEX_BUSY;
		return (NULL);
	}

	ASSERT(node_data->node_state != IBNEX_CFGADM_CONFIGURED);
	node_data->node_state = IBNEX_CFGADM_CONFIGURING;

	ndi_devi_alloc_sleep(parent,
	    IBNEX_IBPORT_CNAME, (pnode_t)DEVI_SID_NODEID, &cdip);

	node_data->node_dip	= cdip;
	ddi_set_parent_data(cdip, node_data);
	mutex_exit(&ibnex.ibnex_mutex);

	switch (node_type) {
		case IBNEX_VPPA_COMMSVC_NODE :
			svcname = ibnex.ibnex_vppa_comm_svc_names[index];
			break;
		case IBNEX_HCASVC_COMMSVC_NODE :
			svcname = ibnex.ibnex_hcasvc_comm_svc_names[index];
			break;
		case IBNEX_PORT_COMMSVC_NODE :
			svcname = ibnex.ibnex_comm_svc_names[index];
			break;
		default :
			IBTF_DPRINTF_L2("ibnex", "\tcommsvc_initnode:"
			    "\tInvalid Node type");
			*rval = IBNEX_FAILURE;
			ibnex_delete_port_node_data(node_data);
			ddi_prop_remove_all(cdip);
			ddi_set_parent_data(cdip, NULL);
			(void) ndi_devi_free(cdip);
			mutex_enter(&ibnex.ibnex_mutex);
			return (NULL);
	}

	if (ibnex_create_port_node_prop(port_attr, cdip, svcname, pkey) ==
	    IBNEX_SUCCESS) {
		if (flag == IBNEX_DEVFS_ENUMERATE)
			ret = ndi_devi_bind_driver(cdip, 0);
		else
			ret = ndi_devi_online(cdip, 0);
		if (ret == NDI_SUCCESS) {
			mutex_enter(&ibnex.ibnex_mutex);
			node_data->node_state	= IBNEX_CFGADM_CONFIGURED;
			return (cdip);
		}
	}
	*rval = IBNEX_FAILURE;
	ibnex_delete_port_node_data(node_data);
	ddi_prop_remove_all(cdip);
	ddi_set_parent_data(cdip, NULL);
	(void) ndi_devi_free(cdip);
	mutex_enter(&ibnex.ibnex_mutex);
	IBTF_DPRINTF_L4("ibnex", "\tcommsvc_initnode: failure exit");
	return (NULL);
}


/*
 * ibnex_create_port_node_prop()
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_create_port_node_prop(ibdm_port_attr_t *port_attr,
    dev_info_t *child_dip, char *srvname, ib_pkey_t pkey)
{
	if (ibnex_create_port_compatible_prop(child_dip,
	    srvname, port_attr) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_port_node_prop: compatible update failed");
		return (IBNEX_FAILURE);
	}
	if ((pkey != 0) && (ndi_prop_update_int(DDI_DEV_T_NONE, child_dip,
	    "port-pkey", pkey) != DDI_PROP_SUCCESS)) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_port_node_prop: port-num update failed");
		return (IBNEX_FAILURE);
	}

	/*
	 * For HCA_SVC device nodes, port_num will be 0.
	 * Do not create the "port-number" & "port-guid" properties.
	 */
	if (port_attr->pa_port_num != 0) {
		if (ndi_prop_update_int(DDI_DEV_T_NONE, child_dip,
			"port-number", port_attr->pa_port_num) !=
		    DDI_PROP_SUCCESS) {
			IBTF_DPRINTF_L2("ibnex",
			    "\tcreate_port_node_prop: port-num update failed");
			return (IBNEX_FAILURE);
		}
		if (ndi_prop_update_int64(DDI_DEV_T_NONE, child_dip,
			"port-guid", port_attr->pa_port_guid) !=
		    DDI_PROP_SUCCESS) {
			IBTF_DPRINTF_L2("ibnex",
			    "\tcreate_port_node_prop: port-guid update failed");
			return (IBNEX_FAILURE);
		}
	} else {
		ibdm_hca_list_t	*hca_list;

		/*
		 * HCA_SVC nodes require "num-ports" & "port-guids"
		 * properties.
		 *
		 * To create the num-ports & port-guids attribute :
		 * 1. Get HCA list (ibdm_ibnex_get_hca_info_by_guid)
		 * 2. Form the array of port GUIDs.
		 */
		if ((hca_list = ibdm_ibnex_get_hca_info_by_guid(
		    port_attr->pa_hca_guid)) == NULL) {
			IBTF_DPRINTF_L2("ibnex",
			    "\tcreate_port_node_prop: hca_info_by_guid failed");
			return (IBNEX_FAILURE);
		}

		if (hca_list->hl_nports != 0) {
			ib_guid_t	*port_guids;
			uint8_t		portnum;

			ASSERT(hca_list->hl_port_attr != NULL);

			port_guids = kmem_zalloc(
			    hca_list->hl_nports * sizeof (ib_guid_t),
			    KM_SLEEP);

			for (portnum = 0; portnum < hca_list->hl_nports;
			    portnum++) {
				port_guids[portnum] = (hca_list->
				    hl_port_attr[portnum]).pa_port_guid;
			}

			if (ndi_prop_update_int64_array(DDI_DEV_T_NONE,
			    child_dip, "port-guids", (int64_t *)port_guids,
			    hca_list->hl_nports) != DDI_PROP_SUCCESS) {
				IBTF_DPRINTF_L2("ibnex",
				    "\tcreate_port_node_prop: port-guids "
				    "create failed");
				kmem_free(port_guids, hca_list->hl_nports *
				    sizeof (ib_guid_t));
				ibdm_ibnex_free_hca_list(hca_list);
				return (IBNEX_FAILURE);
			}
			kmem_free(port_guids, hca_list->hl_nports *
			    sizeof (ib_guid_t));
		}

		if (ndi_prop_update_int(DDI_DEV_T_NONE, child_dip,
			"num-ports", hca_list->hl_nports) != DDI_PROP_SUCCESS) {
			IBTF_DPRINTF_L2("ibnex",
			    "\tcreate_port_node_prop: num-ports update failed");
			ibdm_ibnex_free_hca_list(hca_list);
			return (IBNEX_FAILURE);
		}
		ibdm_ibnex_free_hca_list(hca_list);
	}

	if (ndi_prop_update_int64(DDI_DEV_T_NONE, child_dip,
		"hca-guid", port_attr->pa_hca_guid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_port_node_prop: hca-guid update failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, child_dip,
		"product-id", port_attr->pa_productid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_port_node_prop: product-id update failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, child_dip,
		"vendor-id", port_attr->pa_vendorid) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_port_node_prop: vendor-id update failed");
		return (IBNEX_FAILURE);
	}
	if (ndi_prop_update_int(DDI_DEV_T_NONE, child_dip, "device-version",
	    port_attr->pa_dev_version) != DDI_PROP_SUCCESS) {
		IBTF_DPRINTF_L2("ibnex",
		    "\tcreate_port_node_prop: device-version update failed");
		return (IBNEX_FAILURE);
	}
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_str2int()
 *	Utility function that converts a string of length  "len" to
 *	integer.
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_str2int(char *c, int len, int *ret)
{
	int intval = 0, ii;

	IBTF_DPRINTF_L4("ibnex", "\tstr2int: Int string %s..", c);
	*ret = IBNEX_SUCCESS;
	for (ii = 0; ii < len; ii ++) {
		if ((c[ii] >= '0') && (c[ii] <= '9'))
			intval = intval * 10 +c[ii] - '0';
		else {
			IBTF_DPRINTF_L2("ibnex",
			    "\tstr2int: Invalid integer string %s..", c);
			*ret = IBNEX_FAILURE;
			break;
		}
	}

	return (intval);
}


/*
 * ibnex_str2hex()
 *	Utility functions that converts a string of length  "len" to
 *	hex value. Note. This function does not handle strings which
 *	string length more than 8 bytes.
 *
 */
uint64_t
ibnex_str2hex(char *c, int len, int *ret)
{
	uint64_t hex = 0, ii;

	*ret = IBNEX_SUCCESS;
	for (ii = 0; ii < len; ii ++) {
		hex = (ii == 0) ? hex : (hex << 4);
		if ((c[ii] >= '0') && (c[ii] <= '9'))
			hex |= c[ii] - '0';
		else if ((c[ii] >= 'a') && (c[ii] <= 'f'))
			hex |= c[ii] - 'a' + 10;
		else if ((c[ii] >= 'A') && (c[ii] <= 'F'))
			hex |= c[ii] - 'A' + 10;
		else {
			IBTF_DPRINTF_L2("ibnex",
			    "\tstr2hex: Invalid integer string");
			*ret = IBNEX_FAILURE;
			break;
		}
	}

	return (hex);
}


/*
 * ibnex_create_port_compatible_prop()
 *	Creates 'Compatibility' property for port / HCA_SVC device nodes
 *	Returns IBNEX_SUCCESS/IBNEX_FAILURE
 */
static int
ibnex_create_port_compatible_prop(dev_info_t *child_dip,
    char *comm_svcp, ibdm_port_attr_t *port_attr)
{
	int 	rval, i;
	char	*temp;
	char	*compatible[IBNEX_MAX_IBPORT_COMPAT_NAMES];

	IBTF_DPRINTF_L4("ibnex", "\tcreate_port_compatible_prop: Begin");
	/*
	 * Initialize the "compatible" property string as below:
	 * Compatible Strings :
	 * 1. ib.V<vid>P<pid>v<revision>.<service name>.
	 * 2. ib.V<vid>P<pid>.<service name>.
	 * 3. ib.<service name>
	 * Leading zeros must be present
	 */
	temp = kmem_alloc(IBNEX_MAX_IBPORT_COMPAT_PROP_SZ, KM_SLEEP);
	for (i = 0; i < IBNEX_MAX_IBPORT_COMPAT_NAMES; i++) {
		compatible[i] = temp + (i * IBNEX_MAX_COMPAT_LEN);
	}

	(void) snprintf(compatible[0], IBNEX_MAX_COMPAT_LEN,
	    "ib.V%06xP%08xv%04x.%s",
	    port_attr->pa_vendorid, port_attr->pa_productid,
	    port_attr->pa_dev_version, comm_svcp);
	(void) snprintf(compatible[1], IBNEX_MAX_COMPAT_LEN,
	    "ib.V%06xP%08x.%s",
	    port_attr->pa_vendorid, port_attr->pa_productid,
	    comm_svcp);
	(void) snprintf(compatible[2], IBNEX_MAX_COMPAT_LEN,
	    "ib.%s", comm_svcp);

	for (i = 0; i < IBNEX_MAX_IBPORT_COMPAT_NAMES; i++)
		IBTF_DPRINTF_L4("ibnex", "\tcompatible: %s", compatible[i]);

	rval = ndi_prop_update_string_array(DDI_DEV_T_NONE, child_dip,
	    "compatible", (char **)compatible, IBNEX_MAX_IBPORT_COMPAT_NAMES);

	if (rval != DDI_PROP_SUCCESS) {
		kmem_free(temp, IBNEX_MAX_IBPORT_COMPAT_PROP_SZ);
		return (IBNEX_FAILURE);
	}
	kmem_free(temp, IBNEX_MAX_IBPORT_COMPAT_PROP_SZ);
	return (IBNEX_SUCCESS);
}


/*
 * ibnex_delete_port_node_data()
 *	Delete the parent private node data from the linked list
 *	Deallocate the memory of the port/ioc attributes
 *	Deallocate the memory of the node data
 */
static void
ibnex_delete_port_node_data(ibnex_node_data_t *node)
{
	mutex_enter(&ibnex.ibnex_mutex);
	if ((node->node_next == NULL) && (node->node_prev == NULL))
		ibnex.ibnex_port_node_head = NULL;
	else if (node->node_next == NULL)
		node->node_prev->node_next = NULL;
	else if (node->node_prev == NULL) {
		node->node_next->node_prev = NULL;
		ibnex.ibnex_port_node_head = node->node_next;
	} else {
		node->node_prev->node_next = node->node_next;
		node->node_next->node_prev = node->node_prev;
	}
	mutex_exit(&ibnex.ibnex_mutex);
	kmem_free(node, sizeof (ibnex_node_data_t));
}


/*
 * ibnex_is_node_data_present()
 *	Checks whether ibnex_node_t is created already
 *	Returns ibnex_node_data_t if found, otherwise NULL
 */
static ibnex_node_data_t *
ibnex_is_node_data_present(ibnex_node_type_t node_type, void *attr,
    int index, ib_pkey_t pkey)
{
	ibnex_node_data_t	*nodep;
	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));
	if (node_type == IBNEX_IOC_NODE) {
		ibdm_ioc_info_t *ioc_infop = (ibdm_ioc_info_t *)attr;

		for (nodep = ibnex.ibnex_ioc_node_head; nodep != NULL;
		    nodep = nodep->node_next) {
			if (nodep->node_data.ioc_node.ioc_guid ==
			    ioc_infop->ioc_profile.ioc_guid) {
				return (nodep);
			}
		}

	} else if (node_type == IBNEX_PSEUDO_NODE) {
		for (nodep = ibnex.ibnex_pseudo_node_head; nodep;
		    nodep = nodep->node_next)
			if (strcmp(nodep->node_data.pseudo_node.
			    pseudo_node_addr, (char *)attr) == 0)
				return (nodep);

	} else {
		ibdm_port_attr_t *pattrp = (ibdm_port_attr_t *)attr;

		for (nodep = ibnex.ibnex_port_node_head; nodep != NULL;
		    nodep = nodep->node_next) {
			if ((nodep->node_data.port_node.port_guid ==
			    pattrp->pa_port_guid) &&
			    (nodep->node_data.port_node.port_commsvc_idx ==
			    index) &&
			    (nodep->node_data.port_node.port_pkey == pkey)) {
				return (nodep);
			}
		}
	}
	return (NULL);
}

/*
 * ibnex_lookup_unit_address_prop:
 *
 *	If "unit-address" property is found, return its value
 *	otherwise return NULL.
 */
static char *
ibnex_lookup_unit_address_prop(ddi_prop_t *head)
{
	ddi_prop_t	*propp;

	/* Search the list of properties for "unit-address" */
	for (propp = head; propp != NULL; propp = propp->prop_next)  {
		if (strcmp(propp->prop_name, "unit-address") != 0)
			continue;
		/* "unit-address" property should be valid and have a value */
		if (propp->prop_len <= 1)
			break;
		return ((char *)propp->prop_val);
	}

	return ((char *)0);
}


/*
 * ibnex_pseudo_initnodes()
 *	This routine is specific to pseudo node information handling
 *	Creates a ibnex_node_data_t all pseudo nodes children of ibnex
 */
void
ibnex_pseudo_initnodes()
{
	int			pnam_len, len;
	ibnex_node_data_t	*nodep;
	struct hwc_spec		*list, *spec;
	char			*node_addr, *temp, *unit_addr;

	IBTF_DPRINTF_L4("ibnex", "\tpseudo_initnodes");

	mutex_enter(&ibnex.ibnex_mutex);
	/*
	 * get a list of all "pseudo" children of "ib".
	 * for these children initialize/allocate an internal
	 * ibnex_node_data_t.
	 */
	list = hwc_get_child_spec(ibnex.ibnex_dip, (major_t)-1);
	for (spec = list; spec != NULL; spec = spec->hwc_next) {
		if (spec->hwc_devi_sys_prop_ptr == NULL)
			continue;

		/* "unit-address" property should be present */
		temp = ibnex_lookup_unit_address_prop(
		    spec->hwc_devi_sys_prop_ptr);
		if (temp == NULL)
			continue;

		pnam_len = strlen(spec->hwc_devi_name) + strlen(temp) + 2;

		node_addr = kmem_zalloc(pnam_len, KM_SLEEP);

		(void) snprintf(node_addr,
		    pnam_len, "%s,%s", spec->hwc_devi_name, temp);

		nodep = ibnex_is_node_data_present(
		    IBNEX_PSEUDO_NODE, (void *)node_addr, 0, 0);

		if (nodep) {
			kmem_free(node_addr, pnam_len);
			continue;
		}

		nodep = ibnex_init_child_nodedata(IBNEX_PSEUDO_NODE,
		    (void *)spec->hwc_devi_name, 0, 0);

		nodep->node_data.pseudo_node.pseudo_node_addr = node_addr;
		(void) snprintf(nodep->node_data.
		    pseudo_node.pseudo_node_addr, pnam_len, "%s", node_addr);

		len = strlen(temp) + 1;
		unit_addr = (char *)kmem_alloc(len, KM_SLEEP);
		nodep->node_data.pseudo_node.pseudo_unit_addr = unit_addr;
		(void) snprintf(unit_addr, len, "%s", temp);
		nodep->node_data.pseudo_node.pseudo_unit_addr_len = len;

		/* Mark this as a new psuedo node */
		nodep->node_data.pseudo_node.pseudo_new_node = 1;

		IBTF_DPRINTF_L3("ibnex", "\tpseudo_initnodes: unit addr = %s"
		    " : drv name = %s", unit_addr, spec->hwc_devi_name);
	}
	hwc_free_spec_list(list);
	mutex_exit(&ibnex.ibnex_mutex);
}


/*
 * ibnex_init_child_nodedata()
 *
 *	Allocate memory for the parent private data for device node
 *	Initializes the parent private child device node data.
 *	Returns pointer to the parent private data
 */
static ibnex_node_data_t *
ibnex_init_child_nodedata(ibnex_node_type_t node_type, void *attr, int index,
    ib_pkey_t pkey)
{
	char			 *devi_name;
	ibdm_ioc_info_t		 *ioc_info;
	ibnex_ioc_node_t	 *ioc_node;
	ibnex_node_data_t	 *node_data;
	ib_dm_ioc_ctrl_profile_t *ioc_profile;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	node_data = kmem_zalloc(sizeof (ibnex_node_data_t), KM_SLEEP);
	node_data->node_state = IBNEX_CFGADM_CONFIGURING;
	node_data->node_type	= node_type;

	if (node_type == IBNEX_IOC_NODE) {
		ioc_info	= (ibdm_ioc_info_t *)attr;
		ioc_profile	= &ioc_info->ioc_profile;
		ioc_node	= &node_data->node_data.ioc_node;

		ioc_node->iou_guid = ioc_info->ioc_iou_guid;
		ioc_node->ioc_guid = ioc_profile->ioc_guid;
		(void) strncpy(ioc_node->ioc_id_string,
		    (char *)ioc_profile->ioc_id_string,
		    IB_DM_IOC_ID_STRING_LEN);
		ioc_node->ioc_ngids = ioc_info->ioc_nportgids;

		node_data->node_next = ibnex.ibnex_ioc_node_head;
		node_data->node_prev = NULL;
		if (ibnex.ibnex_ioc_node_head)
			ibnex.ibnex_ioc_node_head->node_prev = node_data;
		ibnex.ibnex_ioc_node_head = node_data;
	} else if (node_type == IBNEX_PSEUDO_NODE) {
		devi_name = (char *)attr;
		node_data->node_data.pseudo_node.pseudo_devi_name =
		    kmem_zalloc(strlen(devi_name) + 1, KM_SLEEP);
		(void) strncpy(node_data->node_data.pseudo_node.
		    pseudo_devi_name, devi_name, strlen(devi_name));
		node_data->node_next = ibnex.ibnex_pseudo_node_head;
		node_data->node_prev = NULL;
		if (ibnex.ibnex_pseudo_node_head)
			ibnex.ibnex_pseudo_node_head->node_prev = node_data;
		ibnex.ibnex_pseudo_node_head = node_data;
	} else {
		node_data->node_data.port_node.port_hcaguid =
		    ((ibdm_port_attr_t *)attr)->pa_hca_guid;
		node_data->node_data.port_node.port_guid =
		    ((ibdm_port_attr_t *)attr)->pa_port_guid;
		node_data->node_data.port_node.port_num =
		    ((ibdm_port_attr_t *)attr)->pa_port_num;
		node_data->node_data.port_node.port_commsvc_idx = index;
		node_data->node_data.port_node.port_pkey = pkey;

		node_data->node_next = ibnex.ibnex_port_node_head;
		node_data->node_prev = NULL;
		if (ibnex.ibnex_port_node_head)
			ibnex.ibnex_port_node_head->node_prev = node_data;
		ibnex.ibnex_port_node_head = node_data;
	}
	return (node_data);
}

static int
ibnex_get_eventcookie(dev_info_t *dip, dev_info_t *rdip,
    char *eventname, ddi_eventcookie_t *cookie)
{
	int rc;


	IBTF_DPRINTF_L4("ibnex", "ibnex_get_eventcookie(%p, %p, %s, 0x%X)",
	    dip, rdip, eventname, cookie);

	rc = ndi_event_retrieve_cookie(ibnex.ibnex_ndi_event_hdl,
	    rdip, eventname, cookie, NDI_EVENT_NOPASS);
	if (rc == NDI_SUCCESS) {
		mutex_enter(&ibnex.ibnex_mutex);
		ibnex.ibnex_prop_update_evt_cookie = *cookie;
		mutex_exit(&ibnex.ibnex_mutex);
	}

	return (rc);
}

static int
ibnex_add_eventcall(dev_info_t *dip, dev_info_t *rdip,
    ddi_eventcookie_t cookie, void (*callback)(dev_info_t *dip,
    ddi_eventcookie_t cookie, void *arg, void *bus_impldata),
    void *arg, ddi_callback_id_t *cb_id)
{
	IBTF_DPRINTF_L4("ibnex",
	    "ibnex_add_eventcall(%p, %p, 0x%X, %p, %p, %p)",
	    dip, rdip, cookie, callback, arg, cb_id);

	return (ndi_event_add_callback(ibnex.ibnex_ndi_event_hdl,
	    rdip, cookie, callback, arg, NDI_SLEEP, cb_id));
}

static int
ibnex_remove_eventcall(dev_info_t *dip, ddi_callback_id_t cb_id)
{
	IBTF_DPRINTF_L4("ibnex", "ibnex_remove_eventcall(%p, 0x%X)",
	    dip, cb_id);

	return (ndi_event_remove_callback(ibnex.ibnex_ndi_event_hdl,
	    cb_id));
}

static int
ibnex_post_event(dev_info_t *dip, dev_info_t *rdip,
    ddi_eventcookie_t cookie, void *bus_impldata)
{
	IBTF_DPRINTF_L4("ibnex", "ibnex_post_event(%p, %p, 0x%X, %p)",
	    dip, rdip, cookie, bus_impldata);

	return (ndi_event_run_callbacks(ibnex.ibnex_ndi_event_hdl, rdip,
	    cookie, bus_impldata));
}

/*
 * ibnex_reprobe_ioc_dev()
 *
 * This could be called as a result of ibt_reprobe_dev request or
 * cfgadm command. The function is called from a taskq in case of
 * ibt_reprobe_dev and from user context for cfgadm command.
 *
 * This function reprobes the properties for one IOC dip.
 *
 * node_reprobe_state should be set before calling this function.
 */
void
ibnex_reprobe_ioc_dev(void *arg)
{
	dev_info_t	*dip = (dev_info_t *)arg;
	ibnex_node_data_t	*node_data;
	ibnex_ioc_node_t	*ioc_data;
	ibdm_ioc_info_t		*ioc_info;

	/* ASSERT(NO_LOCKS_HELD); */
	ASSERT(dip != NULL);

	node_data = ddi_get_parent_data(dip);
	ASSERT(node_data);

	if (node_data->node_dip == NULL) {
		IBTF_DPRINTF_L4("ibnex", "reprobe for unconfigured dip");
		mutex_enter(&ibnex.ibnex_mutex);
		ibnex_wakeup_reprobe_ioc(node_data, 0);
		mutex_exit(&ibnex.ibnex_mutex);
		return;
	}
	ioc_data = &(node_data->node_data.ioc_node);

	/* Reprobe the IOC */
	ioc_info = ibdm_ibnex_probe_ioc(ioc_data->iou_guid, ioc_data->ioc_guid,
	    1);
	if (ioc_info == NULL) {
		IBTF_DPRINTF_L2("ibnex", "Null ioc_info from reprobe");
		mutex_enter(&ibnex.ibnex_mutex);
		ibnex_wakeup_reprobe_ioc(node_data, 1);
		mutex_exit(&ibnex.ibnex_mutex);
		return;
	}

	mutex_enter(&ibnex.ibnex_mutex);
	if (node_data->node_dip)
		ibnex_update_prop(node_data, ioc_info);
	ibnex_wakeup_reprobe_ioc(node_data, 0);
	mutex_exit(&ibnex.ibnex_mutex);

	ibdm_ibnex_free_ioc_list(ioc_info);
}

/*
 * ibnex_reprobe_all()
 *
 * This could be called as a result of cfgadm command. The function
 * is called from user context.
 *
 * This function reprobes the properties for all IOC dips.
 *
 * ibnex_reprobe_state should be set before calling this function.
 */
void
ibnex_reprobe_ioc_all()
{
	ibnex_node_data_t	*node_data;
	ibdm_ioc_info_t		*ioc_info_list, *ioc;

	/* ASSERT(NO_LOCKS_HELD); */

	/* Sweep the fabric */
	ioc = ioc_info_list = ibdm_ibnex_get_ioc_list(
	    IBDM_IBNEX_REPROBE_ALL);
	if (ioc_info_list == NULL) {
		mutex_enter(&ibnex.ibnex_mutex);
		ibnex_wakeup_reprobe_all();
		mutex_exit(&ibnex.ibnex_mutex);
		return;
	}

	mutex_enter(&ibnex.ibnex_mutex);
	while (ioc_info_list) {
		if ((node_data = ibnex_is_node_data_present(IBNEX_IOC_NODE,
		    ioc_info_list, 0, 0)) != NULL &&
		    node_data->node_dip != NULL) {
			ibnex_update_prop(node_data, ioc_info_list);
		}
		ioc_info_list = ioc_info_list->ioc_next;
	}
	ibnex_wakeup_reprobe_all();
	mutex_exit(&ibnex.ibnex_mutex);

	ibdm_ibnex_free_ioc_list(ioc);
}

/*
 * Update the properties, if it has modified and notify IBTF client.
 */
static void
ibnex_update_prop(ibnex_node_data_t *node_data, ibdm_ioc_info_t *ioc_info)
{
	ibt_prop_update_payload_t	evt_data;
	dev_info_t	*dip = node_data->node_dip;
	ddi_eventcookie_t	evt_cookie;
	ibnex_ioc_node_t *ioc;

	ASSERT(MUTEX_HELD(&ibnex.ibnex_mutex));

	ASSERT(dip != NULL);

	ioc = &node_data->node_data.ioc_node;

	evt_data = ioc_info->ioc_info_updated;
	evt_cookie = ibnex.ibnex_prop_update_evt_cookie;

	/*
	 * For a disconnected IOC :
	 *	Store the ioc_profile for supplying cfgadm info
	 *	ibdm maintains no info of disconnected IOC
	 *
	 * For reconnected IOC :
	 *	ibdm has info of previous service entries
	 *	ioc_profile maintained by ibnexus is used to
	 *	update ib_srv_prop_updated.
	 *	Free the ibnex maintained ioc_profile
	 */
	if (ioc_info->ioc_nportgids == 0) {
		IBTF_DPRINTF_L4("ibnex",
		    "\tupdate_prop:  IOC disconnected");
		ioc->ioc_profile = (ib_dm_ioc_ctrl_profile_t *)kmem_zalloc(
		    sizeof (ib_dm_ioc_ctrl_profile_t), KM_SLEEP);
		bcopy(&ioc_info->ioc_profile, ioc->ioc_profile,
		    sizeof (ib_dm_ioc_ctrl_profile_t));

		ibnex.ibnex_num_disconnect_iocs++;
	} else if (ioc_info->ioc_nportgids != 0 && ioc->ioc_ngids == 0 &&
	    ioc->ioc_profile != NULL) {
		IBTF_DPRINTF_L4("ibnex",
		    "\tupdate_prop: IOC reconnected");
		if (ioc->ioc_profile->ioc_service_entries !=
		    ioc_info->ioc_profile.ioc_service_entries)
			evt_data.ib_srv_prop_updated = 1;

		ibnex.ibnex_num_disconnect_iocs--;
		kmem_free(ioc->ioc_profile, sizeof (ib_dm_ioc_ctrl_profile_t));
		ioc->ioc_profile = NULL;
	}

	/* Update the properties that have changed */
	mutex_exit(&ibnex.ibnex_mutex);
	if (evt_data.ib_gid_prop_updated) {
		if (ibnex_create_ioc_portgid_prop(dip, ioc_info) !=
		    IBNEX_SUCCESS) {
			mutex_enter(&ibnex.ibnex_mutex);
			return;
		}
	}
	if (evt_data.ib_srv_prop_updated) {
		if (ioc_info->ioc_profile.ioc_service_entries != 0 &&
		    (ibnex_create_ioc_srv_props(dip, ioc_info) !=
		    IBNEX_SUCCESS)) {
			mutex_enter(&ibnex.ibnex_mutex);
			return;
		} else if (ioc_info->ioc_profile.ioc_service_entries == 0) {
			(void) ndi_prop_remove(DDI_DEV_T_NONE, dip,
			    "service-id");
			(void) ndi_prop_remove(DDI_DEV_T_NONE, dip,
			    "service-name");
		}
	}
	mutex_enter(&ibnex.ibnex_mutex);
	ioc->ioc_ngids = ioc_info->ioc_nportgids;

	/*
	 * Post an event if :
	 *	1. Properites have changed or NOTIFY_ALWAYS is set.
	 *	2. child dip is configured and a valid cookie for
	 *	   IB_PROP_UPDATE_EVENT.
	 */
	if ((evt_data.ib_prop_updated != 0 ||
	    (node_data->node_reprobe_state &
	    IBNEX_NODE_REPROBE_NOTIFY_ALWAYS)) &&
	    ((node_data->node_state == IBNEX_CFGADM_CONFIGURED) &&
		    (evt_cookie != NULL))) {
			mutex_exit(&ibnex.ibnex_mutex);

			if (ndi_post_event(ibnex.ibnex_dip, dip,
			    evt_cookie, &evt_data) != NDI_SUCCESS)
				IBTF_DPRINTF_L2("ibnex",
				    "\tndi_post_event failed\n");

			mutex_enter(&ibnex.ibnex_mutex);
	}

	/*
	 * Cleanup node_reprobe_state, for ibt_reprobe_dev
	 * requests, when reprobe all / node reprobe is in
	 * progress. ibnex_reprobe_ioc_dev is not called
	 * in this case.
	 */
	if (node_data->node_reprobe_state ==
	    IBNEX_NODE_REPROBE_NOTIFY_ALWAYS)
		ibnex_wakeup_reprobe_ioc(node_data, 0);
}

static ibnex_rval_t
ibnex_unique_svcname(char *svcname)
{
	int i;

	/* Check Port Services */
	for (i = 0; i < ibnex.ibnex_num_comm_svcs; i++)
		if (ibnex.ibnex_comm_svc_names[i] && strncmp(svcname,
		    ibnex.ibnex_comm_svc_names[i], 4) == 0)
			return (IBNEX_FAILURE);

	/* Check VPPA Services */
	for (i = 0; i < ibnex.ibnex_nvppa_comm_svcs; i++)
		if (ibnex.ibnex_vppa_comm_svc_names[i] && strncmp(svcname,
		    ibnex.ibnex_vppa_comm_svc_names[i], 4) == 0)
			return (IBNEX_FAILURE);

	/* Check HCA_SVC Services */
	for (i = 0; i < ibnex.ibnex_nhcasvc_comm_svcs; i++)
		if (ibnex.ibnex_hcasvc_comm_svc_names[i] && strncmp(svcname,
		    ibnex.ibnex_hcasvc_comm_svc_names[i], 4) == 0)
			return (IBNEX_FAILURE);

	return (IBNEX_SUCCESS);
}

static void
ibnex_handle_reprobe_dev(void *arg)
{
	dev_info_t	*dip = (dev_info_t *)arg;
	ibnex_node_data_t	*node_data;

	ASSERT(dip != NULL);
	node_data = ddi_get_parent_data(dip);
	ASSERT(node_data);

	/*
	 * Return success if:
	 *	1. Reprobe for all nodes are in progress
	 *	2. Reprobe for this node is in progress.
	 * The reprobe in progress will complete eventually and
	 * update the properties, if required.
	 */
	mutex_enter(&ibnex.ibnex_mutex);
	if (ibnex.ibnex_reprobe_state != 0 ||
	    node_data->node_reprobe_state != 0) {
		/*
		 * Setting NOTIFY_ALWAYS to ensure that
		 * DDI event is delivered always for
		 * ibt_reprobe_dev
		 */
		node_data->node_reprobe_state |=
		    IBNEX_NODE_REPROBE_NOTIFY_ALWAYS;
		mutex_exit(&ibnex.ibnex_mutex);
		return;
	}
	node_data->node_reprobe_state =
	    IBNEX_NODE_REPROBE_NOTIFY_ALWAYS;
	mutex_exit(&ibnex.ibnex_mutex);
	ibnex_reprobe_ioc_dev(arg);
}


/*
 * MPxIO pathmangement routines. Currently IB nexus does not support
 * any kind of pathmangement. So, just return success to make MPxIO
 * framework happy.
 */
/*ARGSUSED*/
static int
ib_vhci_pi_init(dev_info_t *dip, mdi_pathinfo_t *pip, int flag)
{
	IBTF_DPRINTF_L4("ibnex", "\tpi_init: dip %p pip %p", dip, pip);
	return (MDI_SUCCESS);
}


/*ARGSUSED*/
static int
ib_vhci_pi_uninit(dev_info_t *dip, mdi_pathinfo_t *pip, int flag)
{
	IBTF_DPRINTF_L4("ibnex", "\tpi_uninit: dip %p pip %p", dip, pip);
	return (MDI_SUCCESS);
}


/*ARGSUSED*/
static int
ib_vhci_pi_state_change(dev_info_t *dip, mdi_pathinfo_t *pip,
		mdi_pathinfo_state_t state, uint32_t arg1, int arg2)
{
	IBTF_DPRINTF_L4("ibnex",
	    "\tpi_state_change: dip %p pip %p state %x", dip, pip, state);
	return (MDI_SUCCESS);
}


/*ARGSUSED*/
static int
ib_vhci_failover(dev_info_t *dip1, dev_info_t *dip2, int arg)
{
	return (MDI_SUCCESS);
}


static int
ibnex_bus_power(dev_info_t *parent, void *impl_arg,
    pm_bus_power_op_t op, void *arg, void *result)
{

	int ret = DDI_SUCCESS;

	IBTF_DPRINTF_L4("ibnex", "\tbus_power: begin: op = %d", op);

	/*
	 * Generic processing in MPxIO framework
	 */
	ret = mdi_bus_power(parent, impl_arg, op, arg, result);

	switch (ret) {
	case MDI_SUCCESS:
		ret = DDI_SUCCESS;
		break;
	case MDI_FAILURE:
		ret = DDI_FAILURE;
		break;
	default:
		break;
	}

	return (ret);
}
