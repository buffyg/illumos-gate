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

#ifndef	_SYS_IB_IBTL_IBCI_H
#define	_SYS_IB_IBTL_IBCI_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * ibci.h
 *
 * Define the data structures and function prototypes that comprise
 * the IB Channel API (API for HCA drivers).  All CI handles are opaque
 * to the IBTF here, real data is accessed in the HCA driver by a
 * typecast to a driver specific struct.
 */

#include <sys/ib/ibtl/ibtl_types.h>
#include <sys/ib/ibtl/ibtl_ci_types.h>
#include <sys/modctl.h>


#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Define CI opaque IBTF handles.
 */
typedef	struct	ibtl_qp_s	*ibtl_qp_hdl_t;		/* ibt_alloc_qp() */
typedef struct	ibtl_eec_s	*ibtl_eec_hdl_t;	/* ibt_alloc_eec() */

/*
 * Define IBTF opaque CI handles.
 */
typedef struct ibc_hca_s	*ibc_hca_hdl_t; /* HCA/CI Handle */
typedef struct ibc_pd_s		*ibc_pd_hdl_t;	/* Protection Domain Handle */
typedef	struct ibc_rdd_s	*ibc_rdd_hdl_t;	/* Reliable Datagram Domain */
typedef struct ibc_ah_s		*ibc_ah_hdl_t;	/* Address Handle */
typedef struct ibc_qp_s		*ibc_qp_hdl_t;	/* Queue Pair Handle */
typedef struct ibc_srq_s	*ibc_srq_hdl_t;	/* Shared Receive Queue Hdl */
typedef struct ibc_qpn_s	*ibc_qpn_hdl_t;	/* Queue Pair Number Handle */
typedef struct ibc_cq_s		*ibc_cq_hdl_t;	/* Completion Queue Handle */
typedef struct ibc_eec_s	*ibc_eec_hdl_t; /* End-to-End Context Handle */

#define	ibc_mr_hdl_t	ibt_mr_hdl_t	/* Memory Region Handle */
#define	ibc_mw_hdl_t	ibt_mw_hdl_t	/* Memory Window Handle */
#define	ibc_ma_hdl_t	ibt_ma_hdl_t	/* Memory Area Handle */

/* Handle used by CI for up calls to IBTF */
typedef struct ibtl_hca_devinfo_s *ibc_clnt_hdl_t;	/* ibc_attach() */

/*
 * Fields opaque to TI, but visible to CI
 */

/*
 * ibt_qp_alloc_attr_t
 */
#define	qp_ibc_scq_hdl	qp_opaque1
#define	qp_ibc_rcq_hdl	qp_opaque2
#define	qp_ibc_srq_hdl	qp_opaque3


/*
 * ibt_status_t
 */
#define	IBT_HCA_RAWD_CHAN_EXCEEDED	IBT_ERR_OPAQUE1	/* Requested raw QP */
							/* exceeds HCA max */
							/* limit */
#define	IBT_CHAN_RAWD_NOT_SUPPORTED	IBT_ERR_OPAQUE2	/* Raw datagram QPs */
							/* not supported */
#define	IBT_CHAN_RD_NOT_SUPPORTED	IBT_ERR_OPAQUE3	/* RD not supported */
#define	IBT_EEC_HDL_INVALID		IBT_ERR_OPAQUE4	/* Invalid EEC handle */
#define	IBT_EEC_STATE_INVALID		IBT_ERR_OPAQUE5	/* Invalid EEC State */
#define	IBT_EEC_ATTR_RO			IBT_ERR_OPAQUE6	/* Can't Change EEC */
							/* Attribute */
#define	IBT_RDD_IN_USE			IBT_ERR_OPAQUE7	/* RDD in Use */
#define	IBT_RDD_HDL_INVALID		IBT_ERR_OPAQUE8	/* Invalid RDD */
#define	IBT_RNR_NAK_TIMER_INVALID	IBT_ERR_OPAQUE9	/* Invalid RNR NAK */
							/* Timer Value */
#define	IBT_RDD_NOT_SUPPORTED		IBT_ERR_OPAQUE10


/*
 * ibt_wc_status_t
 */
#define	IBT_WC_LOCAL_EEC_OP_ERR		12	/* Internal consistency error */
#define	IBT_WC_LOCAL_RDD_VIOLATION_ERR	30	/* The RDD associated with */
						/* the QP does not match */
						/* the RDD associated with */
						/* the EE Context */
#define	IBT_WC_REMOTE_RD_REQ_INVALID	31	/* Detected an invalid */
						/* incoming RD message */
#define	IBT_WC_EEC_REMOTE_ABORTED	32	/* Requester aborted the */
						/* operation */
#define	IBT_WC_EEC_NUM_INVALID		33	/* Invalid EEC Number */
						/* detected */
#define	IBT_WC_EEC_STATE_INVALID	34	/* Invalid EEC state */

/*
 * ibt_async_code_t
 */
#define	IBT_EVENT_PATH_MIGRATED_EEC	IBT_ASYNC_OPAQUE1
#define	IBT_ERROR_CATASTROPHIC_EEC	IBT_ASYNC_OPAQUE3
#define	IBT_ERROR_PATH_MIGRATE_REQ_EEC	IBT_ASYNC_OPAQUE4

/*
 * ibt_object_type_t
 */
#define	IBT_HDL_EEC	IBT_HDL_OPAQUE1
#define	IBT_HDL_RDD	IBT_HDL_OPAQUE2


/*
 * ibt_hca_attr_t
 */
#define	hca_max_rdd		hca_opaque2	/* Max RDDs in HCA */
#define	hca_max_eec		hca_opaque3	/* Max EEContexts in HCA */
#define	hca_max_rd_sgl		hca_opaque4	/* Max SGL entries per RD WR */
#define	hca_max_rdma_in_ee	hca_opaque5	/* Max RDMA Reads/Atomics in */
						/* per EEC with HCA as target */
#define	hca_max_rdma_out_ee	hca_opaque6	/* Max RDMA Reads/Atomics out */
						/* per EE by this HCA */
#define	hca_max_ipv6_qp		hca_max_ipv6_chan
#define	hca_max_ether_qp	hca_max_ether_chan
#define	hca_eec_max_ci_priv_sz	hca_opaque7
#define	hca_rdd_max_ci_priv_sz	hca_opaque8


/*
 * ibt_wc_t
 */
#define	wc_eecn		wc_opaque3	/* End-to-End Context RD's only */


/* Channel Interface version */
typedef enum ibc_version_e {
	IBCI_V1		= 1
} ibc_version_t;


typedef enum ibc_free_qp_flags_e {
	IBC_FREE_QP_AND_QPN	= 0,	/* free all qp resources */
	IBC_FREE_QP_ONLY	= 1	/* OK to free the QP, but the QPN */
					/* cannot be reused until a future */
					/* call to ibc_release_qpn(qpn_hdl), */
					/* where qpn_hdl is a return value */
					/* of ibc_free_qp() */
} ibc_free_qp_flags_t;

/*
 * RDD alloc flags
 */
typedef enum ibc_rdd_flags_e {
	IBT_RDD_NO_FLAGS	= 0,
	IBT_RDD_USER_MAP	= (1 << 0),
	IBT_RDD_DEFER_ALLOC	= (1 << 1)
} ibc_rdd_flags_t;

/*
 * EEC alloc flags
 */
typedef enum ibc_eec_flags_e {
	IBT_EEC_NO_FLAGS	= 0,
	IBT_EEC_USER_MAP	= (1 << 0),
	IBT_EEC_DEFER_ALLOC	= (1 << 1)
} ibc_eec_flags_t;


/*
 * Completion Queues
 *
 */

/*
 * CQ handler attribute structure.
 */
typedef struct ibc_cq_handler_attr_s {
	ibt_cq_handler_id_t	h_id;		/* Valid ID != NULL */
	int			h_flags;	/* Flags of ddi_intr_get_cap */
	int			h_pri;		/* priority from */
						/* ddi_intr_get_pri */
	void			*h_bind;	/* unknown intrd stuff */
} ibc_cq_handler_attr_t;


/*
 * Event data for asynchronous events and errors. The QP/EEC/CQ/SRQ handle,
 * or port number associated with the Event/Error is passed as an argument
 * to the async handler.
 */
typedef struct ibc_async_event_s {
	uint64_t	ev_fma_ena;	/* fault management error data */
	ibtl_qp_hdl_t	ev_qp_hdl;	/* IBTF QP handle. */
	ibtl_eec_hdl_t	ev_eec_hdl;	/* IBTF EEC handle. */
	ibt_cq_hdl_t	ev_cq_hdl;	/* IBT CQ handle. */
	uint8_t		ev_port;	/* Valid for PORT UP/DOWN events */
	ibt_srq_hdl_t	ev_srq_hdl;	/* SRQ handle */
} ibc_async_event_t;


typedef struct ibc_operations_s {
	/* HCA */
	ibt_status_t (*ibc_query_hca_ports)(ibc_hca_hdl_t hca, uint8_t port,
	    ibt_hca_portinfo_t *info_p);
	ibt_status_t (*ibc_modify_ports)(ibc_hca_hdl_t hca, uint8_t port,
	    ibt_port_modify_flags_t flags, uint8_t init_type);
	ibt_status_t (*ibc_modify_system_image)(ibc_hca_hdl_t hca,
	    ib_guid_t sys_guid);

	/* Protection Domain */
	ibt_status_t (*ibc_alloc_pd)(ibc_hca_hdl_t hca, ibt_pd_flags_t flags,
	    ibc_pd_hdl_t *pd_p);
	ibt_status_t (*ibc_free_pd)(ibc_hca_hdl_t hca, ibc_pd_hdl_t pd);

	/* Reliable Datagram Domain */
	ibt_status_t (*ibc_alloc_rdd)(ibc_hca_hdl_t hca, ibc_rdd_flags_t flags,
	    ibc_rdd_hdl_t *rdd_p);
	ibt_status_t (*ibc_free_rdd)(ibc_hca_hdl_t hca, ibc_rdd_hdl_t rdd);

	/* Address Handle */
	ibt_status_t (*ibc_alloc_ah)(ibc_hca_hdl_t hca, ibt_ah_flags_t flags,
	    ibc_pd_hdl_t pd, ibt_adds_vect_t *attr_p, ibc_ah_hdl_t *ah_p);
	ibt_status_t (*ibc_free_ah)(ibc_hca_hdl_t hca, ibc_ah_hdl_t ah);
	ibt_status_t (*ibc_query_ah)(ibc_hca_hdl_t hca, ibc_ah_hdl_t ah,
	    ibc_pd_hdl_t *pd_p, ibt_adds_vect_t *attr_p);
	ibt_status_t (*ibc_modify_ah)(ibc_hca_hdl_t hca, ibc_ah_hdl_t ah,
	    ibt_adds_vect_t *attr_p);

	/* Queue Pair */
	ibt_status_t (*ibc_alloc_qp)(ibc_hca_hdl_t hca, ibtl_qp_hdl_t ibtl_qp,
	    ibt_qp_type_t type, ibt_qp_alloc_attr_t *attr_p,
	    ibt_chan_sizes_t *queue_sizes_p, ib_qpn_t *qpn, ibc_qp_hdl_t *qp_p);
	ibt_status_t (*ibc_alloc_special_qp)(ibc_hca_hdl_t hca, uint8_t port,
	    ibtl_qp_hdl_t ibt_qp, ibt_sqp_type_t type,
	    ibt_qp_alloc_attr_t *attr_p, ibt_chan_sizes_t *queue_sizes_p,
	    ibc_qp_hdl_t *qp_p);
	ibt_status_t (*ibc_free_qp)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ibc_free_qp_flags_t free_qp_flags, ibc_qpn_hdl_t *qpnh_p);
	ibt_status_t (*ibc_release_qpn)(ibc_hca_hdl_t hca, ibc_qpn_hdl_t qpnh);
	ibt_status_t (*ibc_query_qp)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ibt_qp_query_attr_t *attr_p);
	ibt_status_t (*ibc_modify_qp)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ibt_cep_modify_flags_t flags, ibt_qp_info_t *info_p,
	    ibt_queue_sizes_t *actual_sz);

	/* Completion Queues */
	ibt_status_t (*ibc_alloc_cq)(ibc_hca_hdl_t hca, ibt_cq_hdl_t ibt_cq,
	    ibt_cq_attr_t *attr_p, ibc_cq_hdl_t *cq_p, uint_t *actual_size);
	ibt_status_t (*ibc_free_cq)(ibc_hca_hdl_t hca, ibc_cq_hdl_t cq);
	ibt_status_t (*ibc_query_cq)(ibc_hca_hdl_t hca, ibc_cq_hdl_t cq,
	    uint_t *entries);
	ibt_status_t (*ibc_resize_cq)(ibc_hca_hdl_t hca, ibc_cq_hdl_t cq,
	    uint_t size, uint_t *actual_size);
	ibt_status_t (*ibc_alloc_cq_sched)(ibc_hca_hdl_t hca,
	    ibt_cq_sched_flags_t flags, ibc_cq_handler_attr_t *handler_attrs_p);
	ibt_status_t (*ibc_free_cq_sched)(ibc_hca_hdl_t hca,
	    ibt_cq_handler_id_t id);

	/* EE Context */
	ibt_status_t (*ibc_alloc_eec)(ibc_hca_hdl_t hca, ibc_eec_flags_t flags,
	    ibtl_eec_hdl_t ibtl_eec, ibc_rdd_hdl_t rdd, ibc_eec_hdl_t *eec_p);
	ibt_status_t (*ibc_free_eec)(ibc_hca_hdl_t hca, ibc_eec_hdl_t eec);
	ibt_status_t (*ibc_query_eec)(ibc_hca_hdl_t hca, ibc_eec_hdl_t eec,
	    ibt_eec_query_attr_t *attr_p);
	ibt_status_t (*ibc_modify_eec)(ibc_hca_hdl_t hca, ibc_eec_hdl_t eec,
	    ibt_cep_modify_flags_t flags, ibt_eec_info_t *info_p);

	/* Memory Registration */
	ibt_status_t (*ibc_register_mr)(ibc_hca_hdl_t hca, ibc_pd_hdl_t pd,
	    ibt_mr_attr_t *attr_p, void *ibtl_reserved, ibc_mr_hdl_t *mr_p,
	    ibt_mr_desc_t *mem_desc);
	ibt_status_t (*ibc_register_buf)(ibc_hca_hdl_t hca, ibc_pd_hdl_t pd,
	    ibt_smr_attr_t *attrp, struct buf *buf, void *ibtl_reserved,
	    ibc_mr_hdl_t *mr_hdl_p, ibt_mr_desc_t *mem_desc);
	ibt_status_t (*ibc_register_shared_mr)(ibc_hca_hdl_t hca,
	    ibc_mr_hdl_t mr, ibc_pd_hdl_t pd, ibt_smr_attr_t *attr_p,
	    void *ibtl_reserved, ibc_mr_hdl_t *mr_p, ibt_mr_desc_t *mem_desc);
	ibt_status_t (*ibc_deregister_mr)(ibc_hca_hdl_t hca, ibc_mr_hdl_t mr);
	ibt_status_t (*ibc_query_mr)(ibc_hca_hdl_t hca, ibc_mr_hdl_t mr,
	    ibt_mr_query_attr_t *info_p);
	ibt_status_t (*ibc_reregister_mr)(ibc_hca_hdl_t hca, ibc_mr_hdl_t mr,
	    ibc_pd_hdl_t pd, ibt_mr_attr_t *attr_p, void *ibtl_reserved,
	    ibc_mr_hdl_t *mr_p, ibt_mr_desc_t *mem_desc);
	ibt_status_t (*ibc_reregister_buf)(ibc_hca_hdl_t hca, ibc_mr_hdl_t mr,
	    ibc_pd_hdl_t pd, ibt_smr_attr_t *attrp, struct buf *buf,
	    void *ibtl_reserved, ibc_mr_hdl_t *mr_p, ibt_mr_desc_t *mem_desc);
	ibt_status_t (*ibc_sync_mr)(ibc_hca_hdl_t hca,
	    ibt_mr_sync_t *mr_segments, size_t	num_segments);

	/* Memory Window */
	ibt_status_t (*ibc_alloc_mw)(ibc_hca_hdl_t hca, ibc_pd_hdl_t pd,
	    ibt_mw_flags_t flags, ibc_mw_hdl_t *mw_p, ibt_rkey_t *rkey_p);
	ibt_status_t (*ibc_free_mw)(ibc_hca_hdl_t hca, ibc_mw_hdl_t mw);
	ibt_status_t (*ibc_query_mw)(ibc_hca_hdl_t hca, ibc_mw_hdl_t mw,
	    ibt_mw_query_attr_t *mw_attr_p);

	/* Multicast Group */
	ibt_status_t (*ibc_attach_mcg)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ib_gid_t gid, ib_lid_t lid);
	ibt_status_t (*ibc_detach_mcg)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ib_gid_t gid, ib_lid_t lid);

	/* WR processing */
	ibt_status_t (*ibc_post_send)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ibt_send_wr_t *wr_p, uint_t num_wr, uint_t *num_posted);
	ibt_status_t (*ibc_post_recv)(ibc_hca_hdl_t hca, ibc_qp_hdl_t qp,
	    ibt_recv_wr_t *wr_p, uint_t num_wr, uint_t *num_posted);
	ibt_status_t (*ibc_poll_cq)(ibc_hca_hdl_t hca, ibc_cq_hdl_t cq,
	    ibt_wc_t *wc_p, uint_t num_wc, uint_t *num_polled);
	ibt_status_t (*ibc_notify_cq)(ibc_hca_hdl_t hca, ibc_cq_hdl_t cq,
	    ibt_cq_notify_flags_t flags);

	/* CI Object Private Data */
	ibt_status_t (*ibc_ci_data_in)(ibc_hca_hdl_t hca,
	    ibt_ci_data_flags_t flags, ibt_object_type_t object,
	    void *ibc_object_handle, void *data_p, size_t data_sz);
	ibt_status_t (*ibc_ci_data_out)(ibc_hca_hdl_t hca,
	    ibt_ci_data_flags_t flags, ibt_object_type_t object,
	    void *ibc_object_handle, void *data_p, size_t data_sz);

	/* Shared Receive Queues */
	ibt_status_t (*ibc_alloc_srq)(ibc_hca_hdl_t hca, ibt_srq_flags_t flags,
	    ibt_srq_hdl_t ibt_srq, ibc_pd_hdl_t pd, ibt_srq_sizes_t *sizes,
	    ibc_srq_hdl_t *ibc_srq_p, ibt_srq_sizes_t *real_size_p);
	ibt_status_t (*ibc_free_srq)(ibc_hca_hdl_t hca, ibc_srq_hdl_t srq);
	ibt_status_t (*ibc_query_srq)(ibc_hca_hdl_t hca, ibc_srq_hdl_t srq,
	    ibc_pd_hdl_t *pd_p, ibt_srq_sizes_t *sizes_p, uint_t *limit);
	ibt_status_t (*ibc_modify_srq)(ibc_hca_hdl_t hca, ibc_srq_hdl_t srq,
	    ibt_srq_modify_flags_t flags, uint_t size, uint_t limit,
	    uint_t *real_size_p);
	ibt_status_t (*ibc_post_srq)(ibc_hca_hdl_t hca, ibc_srq_hdl_t srq,
	    ibt_recv_wr_t *wr, uint_t num_wr, uint_t *num_posted_p);

	/* Address translation */
	ibt_status_t (*ibc_map_mem_area)(ibc_hca_hdl_t hca_hdl,
	    ibt_va_attr_t *va_attrs, void *ibtl_reserved,
	    uint_t paddr_list_len, ibt_phys_buf_t *paddr_list_p,
	    uint_t *num_paddr_p, ibc_ma_hdl_t *ibc_ma_hdl_p);
	ibt_status_t (*ibc_unmap_mem_area)(ibc_hca_hdl_t hca_hdl,
	    ibc_ma_hdl_t ma_hdl);

	/* Allocate L_Key */
	ibt_status_t (*ibc_alloc_lkey)(ibc_hca_hdl_t hca_hdl, ibc_pd_hdl_t pd,
	    ibt_lkey_flags_t flags, uint_t phys_buf_list_sz,
	    ibc_mr_hdl_t *mr_p, ibt_pmr_desc_t *mem_desc_p);

	/* Physical Register Memory Region */
	ibt_status_t (*ibc_register_physical_mr)(ibc_hca_hdl_t hca,
	    ibc_pd_hdl_t pd, ibt_pmr_attr_t *mem_pattr, void *ibtl_reserved,
	    ibc_mr_hdl_t *mr_p, ibt_pmr_desc_t *mem_desc_p);
	ibt_status_t (*ibc_reregister_physical_mr)(ibc_hca_hdl_t hca,
	    ibc_mr_hdl_t mr, ibc_pd_hdl_t pd, ibt_pmr_attr_t *mem_pattr,
	    void *ibtl_reserved, ibc_mr_hdl_t *mr_p,
	    ibt_pmr_desc_t *mem_desc_p);
} ibc_operations_t;


/*
 * The ibc_hca_info_s structure is used for HCA drivers to communicate its
 * HCA specific information to IBTF when it attaches a device via ibc_attach().
 *
 * IBTF assumes that the structures pointed to by the hca_ops and hca_attr
 * structure members are persistent.
 */
typedef struct ibc_hca_info_s {
	ibc_version_t		hca_ci_vers;	/* CI Version */
	dev_info_t		*hca_dip;	/* HCA dev_info */
	ibc_hca_hdl_t		hca_handle;	/* used for call through */
						/* "hca_ops" */
	ibc_operations_t	*hca_ops;
	ibt_hca_attr_t		*hca_attr;
	ibc_cq_handler_attr_t	hca_def_cq_handler_attr;
} ibc_hca_info_t;


/* Channel Interface return status */
typedef enum ibc_status_e {
	IBC_SUCCESS = 0,
	IBC_FAILURE = 1
} ibc_status_t;

/*
 * CI up-calls to IBTF.
 */

/*
 * ibc_init
 *	Registers CI clients with the Solaris I/O framework
 *
 * ibc_fini
 *	Un-Registers CI clients with the Solaris I/O framework
 */
int ibc_init(struct modlinkage *modlp);
void ibc_fini(struct modlinkage *modlp);

/*
 * ibc_attach
 *	Register HCA device with IBTF. During this call HCA driver provides
 *	driver's information neededby IBTF.
 *
 * ibc_post_attach
 *	After a successful ibc_attach, this must be called.
 *
 * ibc_pre_detach
 *	Attempt to De-Register HCA Device from IBTF.
 *	This requires the cooperation of IBTF clients to
 *	stop using this HCA.  Upon success, the HCA driver
 *	is committed to calling ibc_detach.
 *
 * ibc_detach
 *	De-Register HCA Device from IBTF.
 *	This function will succeed if ibc_pre_detach has previously
 *	succeeded for this device.
 */
ibc_status_t ibc_attach(ibc_clnt_hdl_t *ibc_hdl_p, ibc_hca_info_t *info_p);
void ibc_post_attach(ibc_clnt_hdl_t ibc_hdl);
ibc_status_t ibc_pre_detach(ibc_clnt_hdl_t ibc_hdl, ddi_detach_cmd_t cmd);
void ibc_detach(ibc_clnt_hdl_t ibc_hdl);

/*
 * ibc_cq_handler
 *	IBTF Completion Queue Notification Handler.
 */
void ibc_cq_handler(ibc_clnt_hdl_t ibc_hdl, ibt_cq_hdl_t ibt_cq);

/*
 * ibc_async_handler
 *	IBTF Asynchronous event/error handler.
 */
void ibc_async_handler(ibc_clnt_hdl_t ibc_hdl, ibt_async_code_t code,
    ibc_async_event_t *event_p);

/*
 * ibc_memory_handler
 *	IBTF memory event/error handler.
 */
void ibc_memory_handler(ibc_clnt_hdl_t ibc_hdl, ibt_mem_code_t code,
    ibt_mem_data_t *data_p, void *ibtl_reserved);

/*
 * ibc_get_ci_failure()
 *
 *	Used to obtain a special IBTF failure code for CI specific failures,
 *	failures other than those defined in ibt_status_t.
 */
ibt_status_t ibc_get_ci_failure(uint64_t ena);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_IB_IBTL_IBCI_H */
