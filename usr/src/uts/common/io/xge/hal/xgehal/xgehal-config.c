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
 *  Copyright (c) 2002-2005 Neterion, Inc.
 *  All right Reserved.
 *
 *  FileName :    xgehal-config.c
 *
 *  Description:  configuration functionality
 *
 *  Created:      14 May 2004
 */

#include "xgehal-config.h"
#include "xge-debug.h"

/*
 * __hal_tti_config_check - Check tti configuration
 * @new_config: tti configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_tti_config_check (xge_hal_tti_config_t *new_config)
{
	if ((new_config->urange_a < XGE_HAL_MIN_TX_URANGE_A) ||
		(new_config->urange_a > XGE_HAL_MAX_TX_URANGE_A)) {
		return XGE_HAL_BADCFG_TX_URANGE_A;
	}

	if ((new_config->ufc_a < XGE_HAL_MIN_TX_UFC_A) ||
		(new_config->ufc_a > XGE_HAL_MAX_TX_UFC_A)) {
		return XGE_HAL_BADCFG_TX_UFC_A;
	}

	if ((new_config->urange_b < XGE_HAL_MIN_TX_URANGE_B) ||
		(new_config->urange_b > XGE_HAL_MAX_TX_URANGE_B)) {
		return XGE_HAL_BADCFG_TX_URANGE_B;
	}

	if ((new_config->ufc_b < XGE_HAL_MIN_TX_UFC_B) ||
		(new_config->ufc_b > XGE_HAL_MAX_TX_UFC_B)) {
		return XGE_HAL_BADCFG_TX_UFC_B;
	}

	if ((new_config->urange_c < XGE_HAL_MIN_TX_URANGE_C) ||
		(new_config->urange_c > XGE_HAL_MAX_TX_URANGE_C)) {
		return XGE_HAL_BADCFG_TX_URANGE_C;
	}

	if ((new_config->ufc_c < XGE_HAL_MIN_TX_UFC_C) ||
		(new_config->ufc_c > XGE_HAL_MAX_TX_UFC_C)) {
		return XGE_HAL_BADCFG_TX_UFC_C;
	}

	if ((new_config->urange_d < XGE_HAL_MIN_TX_URANGE_D) ||
		(new_config->urange_d > XGE_HAL_MAX_TX_URANGE_D)) {
		return XGE_HAL_BADCFG_TX_URANGE_D;
	}

	if ((new_config->ufc_d < XGE_HAL_MIN_TX_UFC_D) ||
		(new_config->ufc_d > XGE_HAL_MAX_TX_UFC_D)) {
		return XGE_HAL_BADCFG_TX_UFC_D;
	}

	if ((new_config->timer_val_us < XGE_HAL_MIN_TX_TIMER_VAL) ||
		(new_config->timer_val_us > XGE_HAL_MAX_TX_TIMER_VAL)) {
		return XGE_HAL_BADCFG_TX_TIMER_VAL;
	}

	if ((new_config->timer_ci_en < XGE_HAL_MIN_TX_TIMER_CI_EN) ||
		(new_config->timer_ci_en > XGE_HAL_MAX_TX_TIMER_CI_EN)) {
		return XGE_HAL_BADCFG_TX_TIMER_CI_EN;
	}

	if ((new_config->timer_ac_en < XGE_HAL_MIN_TX_TIMER_AC_EN) ||
		(new_config->timer_ac_en > XGE_HAL_MAX_TX_TIMER_AC_EN)) {
		return XGE_HAL_BADCFG_TX_TIMER_AC_EN;
	}

	return XGE_HAL_OK;
}

/*
 * __hal_rti_config_check - Check rti configuration
 * @new_config: rti configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_rti_config_check (xge_hal_rti_config_t *new_config)
{
	if ((new_config->urange_a < XGE_HAL_MIN_RX_URANGE_A) ||
		(new_config->urange_a > XGE_HAL_MAX_RX_URANGE_A)) {
		return XGE_HAL_BADCFG_RX_URANGE_A;
	}

	if ((new_config->ufc_a < XGE_HAL_MIN_RX_UFC_A) ||
		(new_config->ufc_a > XGE_HAL_MAX_RX_UFC_A)) {
		return XGE_HAL_BADCFG_RX_UFC_A;
	}

	if ((new_config->urange_b < XGE_HAL_MIN_RX_URANGE_B) ||
		(new_config->urange_b > XGE_HAL_MAX_RX_URANGE_B)) {
		return XGE_HAL_BADCFG_RX_URANGE_B;
	}

	if ((new_config->ufc_b < XGE_HAL_MIN_RX_UFC_B) ||
		(new_config->ufc_b > XGE_HAL_MAX_RX_UFC_B)) {
		return XGE_HAL_BADCFG_RX_UFC_B;
	}

	if ((new_config->urange_c < XGE_HAL_MIN_RX_URANGE_C) ||
		(new_config->urange_c > XGE_HAL_MAX_RX_URANGE_C)) {
		return XGE_HAL_BADCFG_RX_URANGE_C;
	}

	if ((new_config->ufc_c < XGE_HAL_MIN_RX_UFC_C) ||
		(new_config->ufc_c > XGE_HAL_MAX_RX_UFC_C)) {
		return XGE_HAL_BADCFG_RX_UFC_C;
	}

	if ((new_config->ufc_d < XGE_HAL_MIN_RX_UFC_D) ||
		(new_config->ufc_d > XGE_HAL_MAX_RX_UFC_D)) {
		return XGE_HAL_BADCFG_RX_UFC_D;
	}

	if ((new_config->timer_val_us < XGE_HAL_MIN_RX_TIMER_VAL) ||
		(new_config->timer_val_us > XGE_HAL_MAX_RX_TIMER_VAL)) {
		return XGE_HAL_BADCFG_RX_TIMER_VAL;
	}

	if ((new_config->timer_ac_en < XGE_HAL_MIN_RX_TIMER_AC_EN) ||
		(new_config->timer_ac_en > XGE_HAL_MAX_RX_TIMER_AC_EN)) {
		return XGE_HAL_BADCFG_RX_TIMER_AC_EN;
	}

	return XGE_HAL_OK;
}


/*
 * __hal_fifo_queue_check - Check fifo queue configuration
 * @new_config: fifo queue configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_fifo_queue_check (xge_hal_fifo_queue_t *new_config)
{
	if ((new_config->initial < XGE_HAL_MIN_FIFO_QUEUE_LENGTH) ||
		(new_config->initial > XGE_HAL_MAX_FIFO_QUEUE_LENGTH)) {
		return XGE_HAL_BADCFG_FIFO_QUEUE_INITIAL_LENGTH;
	}

	/* FIXME: queue "grow" feature is not supported.
	 *        Use "initial" queue size as the "maximum";
	 *        Remove the next line when fixed. */
	new_config->max = new_config->initial;

	if ((new_config->max < XGE_HAL_MIN_FIFO_QUEUE_LENGTH) ||
		(new_config->max > XGE_HAL_MAX_FIFO_QUEUE_LENGTH)) {
		return XGE_HAL_BADCFG_FIFO_QUEUE_MAX_LENGTH;
	}

	if ((new_config->intr < XGE_HAL_MIN_FIFO_QUEUE_INTR) ||
		(new_config->intr > XGE_HAL_MAX_FIFO_QUEUE_INTR)) {
		return XGE_HAL_BADCFG_FIFO_QUEUE_INTR;
	}

	return XGE_HAL_OK;
}

/*
 * __hal_ring_queue_check - Check ring queue configuration
 * @new_config: ring queue configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_ring_queue_check (xge_hal_ring_queue_t *new_config)
{

	if ((new_config->initial < XGE_HAL_MIN_RING_QUEUE_BLOCKS) ||
		(new_config->initial > XGE_HAL_MAX_RING_QUEUE_BLOCKS)) {
		return XGE_HAL_BADCFG_RING_QUEUE_INITIAL_BLOCKS;
	}

	/* FIXME: queue "grow" feature is not supported.
	 *        Use "initial" queue size as the "maximum";
	 *        Remove the next line when fixed. */
	new_config->max = new_config->initial;

	if ((new_config->max < XGE_HAL_MIN_RING_QUEUE_BLOCKS) ||
		(new_config->max > XGE_HAL_MAX_RING_QUEUE_BLOCKS)) {
		return XGE_HAL_BADCFG_RING_QUEUE_MAX_BLOCKS;
	}

	if ((new_config->buffer_mode != XGE_HAL_RING_QUEUE_BUFFER_MODE_1) &&
		(new_config->buffer_mode != XGE_HAL_RING_QUEUE_BUFFER_MODE_3) &&
		(new_config->buffer_mode != XGE_HAL_RING_QUEUE_BUFFER_MODE_5)) {
		return XGE_HAL_BADCFG_RING_QUEUE_BUFFER_MODE;
	}

        /*
	 * Herc has less DRAM; the check is done later inside
	 * device_initialize()
	 */
	if (((new_config->dram_size_mb < XGE_HAL_MIN_RING_QUEUE_SIZE) ||
	     (new_config->dram_size_mb > XGE_HAL_MAX_RING_QUEUE_SIZE_XENA)) &&
	      new_config->dram_size_mb != XGE_HAL_DEFAULT_USE_HARDCODE)
		return XGE_HAL_BADCFG_RING_QUEUE_SIZE;

	if ((new_config->backoff_interval_us <
			XGE_HAL_MIN_BACKOFF_INTERVAL_US) ||
		(new_config->backoff_interval_us >
			XGE_HAL_MAX_BACKOFF_INTERVAL_US)) {
		return XGE_HAL_BADCFG_BACKOFF_INTERVAL_US;
	}

	if ((new_config->max_frm_len < XGE_HAL_MIN_MAX_FRM_LEN) ||
		(new_config->max_frm_len > XGE_HAL_MAX_MAX_FRM_LEN)) {
		return XGE_HAL_BADCFG_MAX_FRM_LEN;
	}

	if ((new_config->priority < XGE_HAL_MIN_RING_PRIORITY) ||
		(new_config->priority > XGE_HAL_MAX_RING_PRIORITY)) {
		return XGE_HAL_BADCFG_RING_PRIORITY;
	}

	if ((new_config->rth_en < XGE_HAL_MIN_RING_RTH_EN) ||
		(new_config->rth_en > XGE_HAL_MAX_RING_RTH_EN)) {
		return XGE_HAL_BADCFG_RING_RTH_EN;
	}

	if ((new_config->rts_mac_en < XGE_HAL_MIN_RING_RTS_MAC_EN) ||
		(new_config->rts_mac_en > XGE_HAL_MAX_RING_RTS_MAC_EN)) {
		return XGE_HAL_BADCFG_RING_RTS_MAC_EN;
	}

	if (new_config->indicate_max_pkts <
	XGE_HAL_MIN_RING_INDICATE_MAX_PKTS ||
	    new_config->indicate_max_pkts >
	    XGE_HAL_MAX_RING_INDICATE_MAX_PKTS) {
		return XGE_HAL_BADCFG_RING_INDICATE_MAX_PKTS;
	}

	return __hal_rti_config_check(&new_config->rti);
}

/*
 * __hal_mac_config_check - Check mac configuration
 * @new_config: mac configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_mac_config_check (xge_hal_mac_config_t *new_config)
{
	if ((new_config->tmac_util_period < XGE_HAL_MIN_TMAC_UTIL_PERIOD) ||
		(new_config->tmac_util_period > XGE_HAL_MAX_TMAC_UTIL_PERIOD)) {
		return XGE_HAL_BADCFG_TMAC_UTIL_PERIOD;
	}

	if ((new_config->rmac_util_period < XGE_HAL_MIN_RMAC_UTIL_PERIOD) ||
		(new_config->rmac_util_period > XGE_HAL_MAX_RMAC_UTIL_PERIOD)) {
		return XGE_HAL_BADCFG_RMAC_UTIL_PERIOD;
	}

	if ((new_config->rmac_bcast_en < XGE_HAL_MIN_RMAC_BCAST_EN) ||
		(new_config->rmac_bcast_en > XGE_HAL_MAX_RMAC_BCAST_EN)) {
		return XGE_HAL_BADCFG_RMAC_BCAST_EN;
	}

	if ((new_config->rmac_pause_gen_en < XGE_HAL_MIN_RMAC_PAUSE_GEN_EN) ||
		(new_config->rmac_pause_gen_en>XGE_HAL_MAX_RMAC_PAUSE_GEN_EN)) {
		return XGE_HAL_BADCFG_RMAC_PAUSE_GEN_EN;
	}

	if ((new_config->rmac_pause_rcv_en < XGE_HAL_MIN_RMAC_PAUSE_RCV_EN) ||
		(new_config->rmac_pause_rcv_en>XGE_HAL_MAX_RMAC_PAUSE_RCV_EN)) {
		return XGE_HAL_BADCFG_RMAC_PAUSE_RCV_EN;
	}

	if ((new_config->rmac_pause_time < XGE_HAL_MIN_RMAC_HIGH_PTIME) ||
		(new_config->rmac_pause_time > XGE_HAL_MAX_RMAC_HIGH_PTIME)) {
		return XGE_HAL_BADCFG_RMAC_HIGH_PTIME;
	}

	if ((new_config->media < XGE_HAL_MIN_MEDIA) ||
		(new_config->media > XGE_HAL_MAX_MEDIA)) {
		return XGE_HAL_BADCFG_MEDIA;
	}

	if ((new_config->mc_pause_threshold_q0q3 <
			XGE_HAL_MIN_MC_PAUSE_THRESHOLD_Q0Q3) ||
		(new_config->mc_pause_threshold_q0q3 >
			XGE_HAL_MAX_MC_PAUSE_THRESHOLD_Q0Q3)) {
		return XGE_HAL_BADCFG_MC_PAUSE_THRESHOLD_Q0Q3;
	}

	if ((new_config->mc_pause_threshold_q4q7 <
			XGE_HAL_MIN_MC_PAUSE_THRESHOLD_Q4Q7) ||
		(new_config->mc_pause_threshold_q4q7 >
			XGE_HAL_MAX_MC_PAUSE_THRESHOLD_Q4Q7)) {
		return XGE_HAL_BADCFG_MC_PAUSE_THRESHOLD_Q4Q7;
	}

	return XGE_HAL_OK;
}

/*
 * __hal_fifo_config_check - Check fifo configuration
 * @new_config: fifo configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_fifo_config_check (xge_hal_fifo_config_t *new_config)
{
	int i;

	/*
	 * recompute max_frags to be multiple of 4,
	 * which means, multiple of 128 for TxDL
	 */
	new_config->max_frags = ((new_config->max_frags + 3) >> 2) << 2;

	if ((new_config->max_frags < XGE_HAL_MIN_FIFO_FRAGS) ||
		(new_config->max_frags > XGE_HAL_MAX_FIFO_FRAGS))  {
		return XGE_HAL_BADCFG_FIFO_FRAGS;
	}

	if ((new_config->reserve_threshold <
			XGE_HAL_MIN_FIFO_RESERVE_THRESHOLD) ||
		(new_config->reserve_threshold >
			XGE_HAL_MAX_FIFO_RESERVE_THRESHOLD)) {
		return XGE_HAL_BADCFG_FIFO_RESERVE_THRESHOLD;
	}

	if ((new_config->memblock_size < XGE_HAL_MIN_FIFO_MEMBLOCK_SIZE) ||
		(new_config->memblock_size > XGE_HAL_MAX_FIFO_MEMBLOCK_SIZE)) {
		return XGE_HAL_BADCFG_FIFO_MEMBLOCK_SIZE;
	}

	for(i = 0;  i < XGE_HAL_MAX_FIFO_NUM; i++) {
		xge_hal_status_e status;

		if (!new_config->queue[i].configured)
                        continue;

		if ((status = __hal_fifo_queue_check(&new_config->queue[i]))
					!= XGE_HAL_OK) {
			return status;
		}
	}

	return XGE_HAL_OK;
}

/*
 * __hal_ring_config_check - Check ring configuration
 * @new_config: Ring configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
static xge_hal_status_e
__hal_ring_config_check (xge_hal_ring_config_t *new_config)
{
	int i;

	if ((new_config->memblock_size < XGE_HAL_MIN_RING_MEMBLOCK_SIZE) ||
		(new_config->memblock_size > XGE_HAL_MAX_RING_MEMBLOCK_SIZE)) {
		return XGE_HAL_BADCFG_RING_MEMBLOCK_SIZE;
	}

	for(i = 0;  i < XGE_HAL_MAX_RING_NUM; i++) {
		xge_hal_status_e status;

		if (!new_config->queue[i].configured)
                        continue;

		if ((status = __hal_ring_queue_check(&new_config->queue[i]))
					!= XGE_HAL_OK) {
			return status;
		}
	}

	return XGE_HAL_OK;
}


/*
 * __hal_device_config_check_common - Check device configuration.
 * @new_config: Device configuration information
 *
 * Check part of configuration that is common to
 * Xframe-I and Xframe-II.
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 *
 * See also: __hal_device_config_check_xena().
 */
xge_hal_status_e
__hal_device_config_check_common (xge_hal_device_config_t *new_config)
{
	xge_hal_status_e status;

	if ((new_config->mtu < XGE_HAL_MIN_MTU) ||
		(new_config->mtu > XGE_HAL_MAX_MTU)) {
		return XGE_HAL_BADCFG_MAX_MTU;
	}

	if ((new_config->no_isr_events < XGE_HAL_NO_ISR_EVENTS_MIN) ||
		(new_config->no_isr_events > XGE_HAL_NO_ISR_EVENTS_MAX)) {
		return XGE_HAL_BADCFG_NO_ISR_EVENTS;
	}

	if ((new_config->isr_polling_cnt < XGE_HAL_MIN_ISR_POLLING_CNT) ||
		(new_config->isr_polling_cnt > XGE_HAL_MAX_ISR_POLLING_CNT)) {
		return XGE_HAL_BADCFG_ISR_POLLING_CNT;
	}

	if (new_config->latency_timer &&
	    new_config->latency_timer != XGE_HAL_USE_BIOS_DEFAULT_LATENCY) {
                if ((new_config->latency_timer < XGE_HAL_MIN_LATENCY_TIMER) ||
		    (new_config->latency_timer > XGE_HAL_MAX_LATENCY_TIMER)) {
                        return XGE_HAL_BADCFG_LATENCY_TIMER;
		}
	}

	if (new_config->max_splits_trans != XGE_HAL_USE_BIOS_DEFAULT_SPLITS)  {
		if ((new_config->max_splits_trans <
			XGE_HAL_ONE_SPLIT_TRANSACTION) ||
		    (new_config->max_splits_trans >
			XGE_HAL_THIRTYTWO_SPLIT_TRANSACTION))
		return XGE_HAL_BADCFG_MAX_SPLITS_TRANS;
	}

	if (new_config->mmrb_count != XGE_HAL_DEFAULT_BIOS_MMRB_COUNT) 
	{
	    if ((new_config->mmrb_count < XGE_HAL_MIN_MMRB_COUNT) ||
		    (new_config->mmrb_count > XGE_HAL_MAX_MMRB_COUNT)) {
    		return XGE_HAL_BADCFG_MMRB_COUNT;
	    }
	}

	if ((new_config->shared_splits < XGE_HAL_MIN_SHARED_SPLITS) ||
		(new_config->shared_splits > XGE_HAL_MAX_SHARED_SPLITS)) {
		return XGE_HAL_BADCFG_SHARED_SPLITS;
	}

	if (new_config->stats_refresh_time_sec !=
	        XGE_HAL_STATS_REFRESH_DISABLE)  {
	        if ((new_config->stats_refresh_time_sec <
				        XGE_HAL_MIN_STATS_REFRESH_TIME) ||
	            (new_config->stats_refresh_time_sec >
				        XGE_HAL_MAX_STATS_REFRESH_TIME)) {
		        return XGE_HAL_BADCFG_STATS_REFRESH_TIME;
	        }
	}

	if ((new_config->intr_mode != XGE_HAL_INTR_MODE_IRQLINE) &&
		(new_config->intr_mode != XGE_HAL_INTR_MODE_MSI) &&
		(new_config->intr_mode != XGE_HAL_INTR_MODE_MSIX)) {
		return XGE_HAL_BADCFG_INTR_MODE;
	}

	if ((new_config->sched_timer_us < XGE_HAL_SCHED_TIMER_MIN) ||
		(new_config->sched_timer_us > XGE_HAL_SCHED_TIMER_MAX)) {
		return XGE_HAL_BADCFG_SCHED_TIMER_US;
	}

	if ((new_config->sched_timer_one_shot !=
			XGE_HAL_SCHED_TIMER_ON_SHOT_DISABLE)  &&
		(new_config->sched_timer_one_shot !=
			XGE_HAL_SCHED_TIMER_ON_SHOT_ENABLE)) {
		return XGE_HAL_BADCFG_SCHED_TIMER_ON_SHOT;
	}

	if (new_config->sched_timer_us) {
		if ((new_config->rxufca_intr_thres <
					XGE_HAL_RXUFCA_INTR_THRES_MIN) ||
		    (new_config->rxufca_intr_thres >
					XGE_HAL_RXUFCA_INTR_THRES_MAX)) {
			return XGE_HAL_BADCFG_RXUFCA_INTR_THRES;
		}

		if ((new_config->rxufca_hi_lim < XGE_HAL_RXUFCA_HI_LIM_MIN) ||
		    (new_config->rxufca_hi_lim > XGE_HAL_RXUFCA_HI_LIM_MAX)) {
			return XGE_HAL_BADCFG_RXUFCA_HI_LIM;
		}

		if ((new_config->rxufca_lo_lim < XGE_HAL_RXUFCA_LO_LIM_MIN) ||
		    (new_config->rxufca_lo_lim > XGE_HAL_RXUFCA_LO_LIM_MAX) ||
		    (new_config->rxufca_lo_lim > new_config->rxufca_hi_lim)) {
			return XGE_HAL_BADCFG_RXUFCA_LO_LIM;
		}

		if ((new_config->rxufca_lbolt_period <
					XGE_HAL_RXUFCA_LBOLT_PERIOD_MIN) ||
		    (new_config->rxufca_lbolt_period >
					XGE_HAL_RXUFCA_LBOLT_PERIOD_MAX)) {
			return XGE_HAL_BADCFG_RXUFCA_LBOLT_PERIOD;
		}
	}

	if ((new_config->link_valid_cnt < XGE_HAL_LINK_VALID_CNT_MIN) ||
		(new_config->link_valid_cnt > XGE_HAL_LINK_VALID_CNT_MAX)) {
		return XGE_HAL_BADCFG_LINK_VALID_CNT;
	}

	if ((new_config->link_retry_cnt < XGE_HAL_LINK_RETRY_CNT_MIN) ||
		(new_config->link_retry_cnt > XGE_HAL_LINK_RETRY_CNT_MAX)) {
		return XGE_HAL_BADCFG_LINK_RETRY_CNT;
	}

	if (new_config->link_valid_cnt > new_config->link_retry_cnt)
		return XGE_HAL_BADCFG_LINK_VALID_CNT;

	if (new_config->link_stability_period != XGE_HAL_DEFAULT_USE_HARDCODE) {
	        if ((new_config->link_stability_period <
				        XGE_HAL_MIN_LINK_STABILITY_PERIOD) ||
		        (new_config->link_stability_period >
				        XGE_HAL_MAX_LINK_STABILITY_PERIOD)) {
		        return XGE_HAL_BADCFG_LINK_STABILITY_PERIOD;
	        }
	}

	if (new_config->device_poll_millis !=
	                XGE_HAL_DEFAULT_USE_HARDCODE)  {
	        if ((new_config->device_poll_millis <
			        XGE_HAL_MIN_DEVICE_POLL_MILLIS) ||
		        (new_config->device_poll_millis >
			        XGE_HAL_MAX_DEVICE_POLL_MILLIS)) {
		        return XGE_HAL_BADCFG_DEVICE_POLL_MILLIS;
	        }
        }

	if ((status = __hal_ring_config_check(&new_config->ring))
			!= XGE_HAL_OK) {
		return status;
	}

	if ((status = __hal_mac_config_check(&new_config->mac)) !=
	    XGE_HAL_OK) {
		return status;
	}

	/*
	 * Validate the tti configuration parameters only if the TTI
	 * feature is enabled.
	 */
	if (new_config->tti.enabled) {
		if ((status = __hal_tti_config_check(&new_config->tti)) !=
							XGE_HAL_OK) {
			return status;
		}
	}

	if ((status = __hal_fifo_config_check(&new_config->fifo)) !=
	    XGE_HAL_OK) {
		return status;
	}

	return XGE_HAL_OK;
}

/*
 * __hal_device_config_check_xena - Check Xframe-I configuration
 * @new_config: Device configuration.
 *
 * Check part of configuration that is relevant only to Xframe-I.
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 *
 * See also: __hal_device_config_check_common().
 */
xge_hal_status_e
__hal_device_config_check_xena (xge_hal_device_config_t *new_config)
{
	if ((new_config->pci_freq_mherz != XGE_HAL_PCI_FREQ_MHERZ_33) &&
		(new_config->pci_freq_mherz != XGE_HAL_PCI_FREQ_MHERZ_66) &&
		(new_config->pci_freq_mherz != XGE_HAL_PCI_FREQ_MHERZ_100) &&
		(new_config->pci_freq_mherz != XGE_HAL_PCI_FREQ_MHERZ_133) &&
		(new_config->pci_freq_mherz != XGE_HAL_PCI_FREQ_MHERZ_266)) {
		return XGE_HAL_BADCFG_PCI_FREQ_MHERZ;
	}

	return XGE_HAL_OK;
}

/*
 * __hal_device_config_check_herc - Check device configuration
 * @new_config: Device configuration.
 *
 * Check part of configuration that is relevant only to Xframe-II.
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 *
 * See also: __hal_device_config_check_common().
 */
xge_hal_status_e
__hal_device_config_check_herc (xge_hal_device_config_t *new_config)
{
	return XGE_HAL_OK;
}


/**
 * __hal_driver_config_check - Check HAL configuration
 * @new_config: Driver configuration information
 *
 * Returns: XGE_HAL_OK - success,
 * otherwise one of the xge_hal_status_e{} enumerated error codes.
 */
xge_hal_status_e
__hal_driver_config_check (xge_hal_driver_config_t *new_config)
{
	if ((new_config->queue_size_initial <
                XGE_HAL_MIN_QUEUE_SIZE_INITIAL) ||
	    (new_config->queue_size_initial >
                XGE_HAL_MAX_QUEUE_SIZE_INITIAL)) {
		return XGE_HAL_BADCFG_QUEUE_SIZE_INITIAL;
	}

	if ((new_config->queue_size_max < XGE_HAL_MIN_QUEUE_SIZE_MAX) ||
		(new_config->queue_size_max > XGE_HAL_MAX_QUEUE_SIZE_MAX)) {
		return XGE_HAL_BADCFG_QUEUE_SIZE_MAX;
	}

#ifdef XGE_TRACE_INTO_CIRCULAR_ARR
	if ((new_config->tracebuf_size < XGE_HAL_MIN_CIRCULAR_ARR) ||
		(new_config->tracebuf_size > XGE_HAL_MAX_CIRCULAR_ARR)) {
		return XGE_HAL_BADCFG_TRACEBUF_SIZE;
	}
#endif

	return XGE_HAL_OK;
}
