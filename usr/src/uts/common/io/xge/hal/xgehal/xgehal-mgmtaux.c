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
 *  FileName :    xgehal-mgmtaux.c
 *
 *  Description:  Xframe-family management auxiliary API implementation
 *
 *  Created:      1 September 2004
 */

#include "xgehal-mgmt.h"
#include "xgehal-driver.h"
#include "xgehal-device.h"

#ifdef XGE_OS_HAS_SNPRINTF
#define __hal_aux_snprintf(retbuf, bufsize, fmt, key, value, retsize) \
    if (bufsize <= 0) return XGE_HAL_ERR_OUT_OF_SPACE; \
    retsize = xge_os_snprintf(retbuf, bufsize, fmt, key, \
			XGE_HAL_AUX_SEPA, value); \
    if (retsize < 0 || retsize >= bufsize) return XGE_HAL_ERR_OUT_OF_SPACE;
#else
#define __hal_aux_snprintf(retbuf, bufsize, fmt, key, value, retsize) \
	if (bufsize <= 0) return XGE_HAL_ERR_OUT_OF_SPACE; \
        retsize = xge_os_sprintf(retbuf, fmt, key, XGE_HAL_AUX_SEPA, value); \
	xge_assert(retsize < bufsize); \
	if (retsize < 0 || retsize >= bufsize) \
		return XGE_HAL_ERR_OUT_OF_SPACE;
#endif

#define __HAL_AUX_ENTRY_DECLARE(size, buf) \
	int entrysize = 0, leftsize = size; \
	char *ptr = buf;

#define __HAL_AUX_ENTRY(key, value, fmt) \
	ptr += entrysize; leftsize -= entrysize; \
	__hal_aux_snprintf(ptr, leftsize, "%s%c"fmt"\n", key, value, entrysize)

#define __HAL_AUX_ENTRY_END(bufsize, retsize) \
	leftsize -= entrysize; \
	*retsize = bufsize - leftsize;

/**
 * xge_hal_aux_bar0_read - Read and format Xframe BAR0 register.
 * @devh: HAL device handle.
 * @offset: Register offset in the BAR0 space.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read Xframe register from BAR0 space. The result is formatted as an ascii string.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_OUT_OF_SPACE - Buffer size is very small.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_INVALID_OFFSET - Register offset in the BAR space is not
 * valid.
 * XGE_HAL_ERR_INVALID_BAR_ID - BAR id is not valid.
 *
 * See also: xge_hal_mgmt_reg_read().
 */
xge_hal_status_e xge_hal_aux_bar0_read(xge_hal_device_h devh,
			unsigned int offset, int bufsize, char *retbuf,
			int *retsize)
{
	xge_hal_status_e status;
	u64 retval;

	status = xge_hal_mgmt_reg_read(devh, 0, offset, &retval);
	if (status != XGE_HAL_OK) {
		return status;
	}

	if (bufsize < XGE_OS_SPRINTF_STRLEN) {
		return XGE_HAL_ERR_OUT_OF_SPACE;
	}

	*retsize = xge_os_sprintf(retbuf, "0x%04X%c0x%08X%08X\n", offset,
				XGE_HAL_AUX_SEPA, (u32)(retval>>32), (u32)retval);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_bar1_read - Read and format Xframe BAR1 register.
 * @devh: HAL device handle.
 * @offset: Register offset in the BAR1 space.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read Xframe register from BAR1 space. The result is formatted as ascii string.
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_OUT_OF_SPACE - Buffer size is very small.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_INVALID_OFFSET - Register offset in the BAR space is not
 * valid.
 * XGE_HAL_ERR_INVALID_BAR_ID - BAR id is not valid.
 *
 * See also: xge_hal_mgmt_reg_read().
 */
xge_hal_status_e xge_hal_aux_bar1_read(xge_hal_device_h devh,
			unsigned int offset, int bufsize, char *retbuf,
			int *retsize)
{
	xge_hal_status_e status;
	u64 retval;

	status = xge_hal_mgmt_reg_read(devh, 1, offset, &retval);
	if (status != XGE_HAL_OK) {
		return status;
	}

	if (bufsize < XGE_OS_SPRINTF_STRLEN) {
		return XGE_HAL_ERR_OUT_OF_SPACE;
	}

        *retsize = xge_os_sprintf(retbuf, "0x%04X%c0x%08X%08X\n",
        offset,
				XGE_HAL_AUX_SEPA, (u32)(retval>>32), (u32)retval);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_bar0_write - Write BAR0 register.
 * @devh: HAL device handle.
 * @offset: Register offset in the BAR0 space.
 * @value: Regsister value (to write).
 *
 * Write BAR0 register.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_INVALID_OFFSET - Register offset in the BAR space is not
 * valid.
 * XGE_HAL_ERR_INVALID_BAR_ID - BAR id is not valid.
 *
 * See also: xge_hal_mgmt_reg_write().
 */
xge_hal_status_e xge_hal_aux_bar0_write(xge_hal_device_h devh,
			unsigned int offset, u64 value)
{
	xge_hal_status_e status;

	status = xge_hal_mgmt_reg_write(devh, 0, offset, value);
	if (status != XGE_HAL_OK) {
		return status;
	}

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_about_read - Retrieve and format about info.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Retrieve about info (using xge_hal_mgmt_about()) and sprintf it
 * into the provided @retbuf.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 * XGE_HAL_FAIL - Failed to retrieve the information.
 *
 * See also: xge_hal_mgmt_about(), xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_about_read(xge_hal_device_h devh, int bufsize,
			char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_mgmt_about_info_t about_info;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_about(devh, &about_info,
				  sizeof(xge_hal_mgmt_about_info_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("vendor", about_info.vendor, "0x%x");
	__HAL_AUX_ENTRY("device", about_info.device, "0x%x");
	__HAL_AUX_ENTRY("subsys_vendor", about_info.subsys_vendor, "0x%x");
	__HAL_AUX_ENTRY("subsys_device", about_info.subsys_device, "0x%x");
	__HAL_AUX_ENTRY("board_rev", about_info.board_rev, "0x%x");
	__HAL_AUX_ENTRY("vendor_name", about_info.vendor_name, "%s");
	__HAL_AUX_ENTRY("chip_name", about_info.chip_name, "%s");
	__HAL_AUX_ENTRY("media", about_info.media, "%s");
	__HAL_AUX_ENTRY("hal_major", about_info.hal_major, "%s");
	__HAL_AUX_ENTRY("hal_minor", about_info.hal_minor, "%s");
	__HAL_AUX_ENTRY("hal_fix", about_info.hal_fix, "%s");
	__HAL_AUX_ENTRY("hal_build", about_info.hal_build, "%s");
	__HAL_AUX_ENTRY("ll_major", about_info.ll_major, "%s");
	__HAL_AUX_ENTRY("ll_minor", about_info.ll_minor, "%s");
	__HAL_AUX_ENTRY("ll_fix", about_info.ll_fix, "%s");
	__HAL_AUX_ENTRY("ll_build", about_info.ll_build, "%s");

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_stats_tmac_read - Read TMAC hardware statistics.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read TMAC hardware statistics. This is a subset of stats counters
 * from xge_hal_stats_hw_info_t{}.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_mgmt_hw_stats{}, xge_hal_stats_hw_info_t{},
 * xge_hal_aux_stats_pci_read(),
 * xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_stats_tmac_read(xge_hal_device_h devh, int	bufsize,
				char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_mgmt_hw_stats_t hw;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_hw_stats(devh, &hw,
				     sizeof(xge_hal_mgmt_hw_stats_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("tmac_data_octets", hw.tmac_data_octets, "%u");
	__HAL_AUX_ENTRY("tmac_frms", hw.tmac_frms, "%u");
	__HAL_AUX_ENTRY("tmac_drop_frms", (unsigned long long)
			hw.tmac_drop_frms, "%llu");
	__HAL_AUX_ENTRY("tmac_bcst_frms", hw.tmac_bcst_frms, "%u");
	__HAL_AUX_ENTRY("tmac_mcst_frms", hw.tmac_mcst_frms, "%u");
	__HAL_AUX_ENTRY("tmac_pause_ctrl_frms", (unsigned long long)
			hw.tmac_pause_ctrl_frms, "%llu");
	__HAL_AUX_ENTRY("tmac_ucst_frms", hw.tmac_ucst_frms, "%u");
	__HAL_AUX_ENTRY("tmac_ttl_octets", hw.tmac_ttl_octets, "%u");
	__HAL_AUX_ENTRY("tmac_any_err_frms", hw.tmac_any_err_frms, "%u");
	__HAL_AUX_ENTRY("tmac_nucst_frms", hw.tmac_nucst_frms, "%u");
	__HAL_AUX_ENTRY("tmac_ttl_less_fb_octets", (unsigned long long)
			hw.tmac_ttl_less_fb_octets, "%llu");
	__HAL_AUX_ENTRY("tmac_vld_ip_octets", (unsigned long long)
			hw.tmac_vld_ip_octets, "%llu");
	__HAL_AUX_ENTRY("tmac_drop_ip", hw.tmac_drop_ip, "%u");
	__HAL_AUX_ENTRY("tmac_vld_ip", hw.tmac_vld_ip, "%u");
	__HAL_AUX_ENTRY("tmac_rst_tcp", hw.tmac_rst_tcp, "%u");
	__HAL_AUX_ENTRY("tmac_icmp", hw.tmac_icmp, "%u");
	__HAL_AUX_ENTRY("tmac_tcp", (unsigned long long)
			hw.tmac_tcp, "%llu");
	__HAL_AUX_ENTRY("reserved_0", hw.reserved_0, "%u");
	__HAL_AUX_ENTRY("tmac_udp", hw.tmac_udp, "%u");

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_stats_rmac_read - Read RMAC hardware statistics.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read RMAC hardware statistics. This is a subset of stats counters
 * from xge_hal_stats_hw_info_t{}.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_mgmt_hw_stats{}, xge_hal_stats_hw_info_t{},
 * xge_hal_aux_stats_pci_read(), xge_hal_aux_stats_tmac_read(),
 * xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_stats_rmac_read(xge_hal_device_h devh, int	bufsize,
				char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_mgmt_hw_stats_t hw;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_hw_stats(devh, &hw,
				     sizeof(xge_hal_mgmt_hw_stats_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("rmac_data_octets", hw.rmac_data_octets, "%u");
	__HAL_AUX_ENTRY("rmac_vld_frms", hw.rmac_vld_frms, "%u");
	__HAL_AUX_ENTRY("rmac_fcs_err_frms", (unsigned long long)
			hw.rmac_fcs_err_frms, "%llu");
	__HAL_AUX_ENTRY("mac_drop_frms", (unsigned long long)
			hw.rmac_drop_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_vld_bcst_frms", hw.rmac_vld_bcst_frms, "%u");
	__HAL_AUX_ENTRY("rmac_vld_mcst_frms", hw.rmac_vld_mcst_frms, "%u");
	__HAL_AUX_ENTRY("rmac_out_rng_len_err_frms",
			hw.rmac_out_rng_len_err_frms, "%u");
	__HAL_AUX_ENTRY("rmac_in_rng_len_err_frms",
			hw.rmac_in_rng_len_err_frms, "%u");
	__HAL_AUX_ENTRY("rmac_long_frms", (unsigned long long)
			hw.rmac_long_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_pause_ctrl_frms", (unsigned long long)
			hw.rmac_pause_ctrl_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_unsup_ctrl_frms", (unsigned long long)
			hw.rmac_unsup_ctrl_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_accepted_ucst_frms",
			hw.rmac_accepted_ucst_frms, "%u");
	__HAL_AUX_ENTRY("rmac_ttl_octets", hw.rmac_ttl_octets, "%u");
	__HAL_AUX_ENTRY("rmac_discarded_frms", hw.rmac_discarded_frms, "%u");
	__HAL_AUX_ENTRY("rmac_accepted_nucst_frms",
			hw.rmac_accepted_nucst_frms, "%u");
	__HAL_AUX_ENTRY("reserved_1", hw.reserved_1, "%u");
	__HAL_AUX_ENTRY("rmac_drop_events", hw.rmac_drop_events, "%u");
	__HAL_AUX_ENTRY("rmac_ttl_less_fb_octets", (unsigned long long)
			hw.rmac_ttl_less_fb_octets, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_frms", (unsigned long long)
			hw.rmac_ttl_frms, "%llu");
	__HAL_AUX_ENTRY("reserved_2", (unsigned long long)
			hw.reserved_2, "%llu");
	__HAL_AUX_ENTRY("rmac_usized_frms", hw.rmac_usized_frms, "%u");
	__HAL_AUX_ENTRY("reserved_3", hw.reserved_3, "%u");
	__HAL_AUX_ENTRY("rmac_frag_frms", hw.rmac_frag_frms, "%u");
	__HAL_AUX_ENTRY("rmac_osized_frms", hw.rmac_osized_frms, "%u");
	__HAL_AUX_ENTRY("reserved_4", hw.reserved_4, "%u");
	__HAL_AUX_ENTRY("rmac_jabber_frms", hw.rmac_jabber_frms, "%u");
	__HAL_AUX_ENTRY("rmac_ttl_64_frms", (unsigned long long)
			hw.rmac_ttl_64_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_65_127_frms", (unsigned long long)
			hw.rmac_ttl_65_127_frms, "%llu");
	__HAL_AUX_ENTRY("reserved_5", (unsigned long long)
			hw.reserved_5, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_128_255_frms", (unsigned long long)
			hw.rmac_ttl_128_255_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_256_511_frms", (unsigned long long)
			hw.rmac_ttl_256_511_frms, "%llu");
	__HAL_AUX_ENTRY("reserved_6", (unsigned long long)
			hw.reserved_6, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_512_1023_frms", (unsigned long long)
			hw.rmac_ttl_512_1023_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_1024_1518_frms", (unsigned long long)
			hw.rmac_ttl_1024_1518_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ip", hw.rmac_ip, "%u");
	__HAL_AUX_ENTRY("reserved_7", hw.reserved_7, "%u");
	__HAL_AUX_ENTRY("rmac_ip_octets", (unsigned long long)
			hw.rmac_ip_octets, "%llu");
	__HAL_AUX_ENTRY("rmac_drop_ip", hw.rmac_drop_ip, "%u");
	__HAL_AUX_ENTRY("rmac_hdr_err_ip", hw.rmac_hdr_err_ip, "%u");
	__HAL_AUX_ENTRY("reserved_8", hw.reserved_8, "%u");
	__HAL_AUX_ENTRY("rmac_icmp", hw.rmac_icmp, "%u");
	__HAL_AUX_ENTRY("rmac_tcp", (unsigned long long)
			hw.rmac_tcp, "%llu");
	__HAL_AUX_ENTRY("rmac_err_drp_udp", hw.rmac_err_drp_udp, "%u");
	__HAL_AUX_ENTRY("rmac_udp", hw.rmac_udp, "%u");
	__HAL_AUX_ENTRY("rmac_xgmii_err_sym", (unsigned long long)
			hw.rmac_xgmii_err_sym, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q0", (unsigned long long)
			hw.rmac_frms_q0, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q1", (unsigned long long)
			hw.rmac_frms_q1, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q2", (unsigned long long)
			hw.rmac_frms_q2, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q3", (unsigned long long)
			hw.rmac_frms_q3, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q4", (unsigned long long)
			hw.rmac_frms_q4, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q5", (unsigned long long)
			hw.rmac_frms_q5, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q6", (unsigned long long)
			hw.rmac_frms_q6, "%llu");
	__HAL_AUX_ENTRY("rmac_frms_q7", (unsigned long long)
			hw.rmac_frms_q7, "%llu");
	__HAL_AUX_ENTRY("rmac_full_q3", hw.rmac_full_q3, "%d");
	__HAL_AUX_ENTRY("rmac_full_q2", hw.rmac_full_q2, "%d");
	__HAL_AUX_ENTRY("rmac_full_q1", hw.rmac_full_q1, "%d");
	__HAL_AUX_ENTRY("rmac_full_q0", hw.rmac_full_q0, "%d");
	__HAL_AUX_ENTRY("rmac_full_q7", hw.rmac_full_q7, "%d");
	__HAL_AUX_ENTRY("rmac_full_q6", hw.rmac_full_q6, "%d");
	__HAL_AUX_ENTRY("rmac_full_q5", hw.rmac_full_q5, "%d");
	__HAL_AUX_ENTRY("rmac_full_q4", hw.rmac_full_q4, "%d");
	__HAL_AUX_ENTRY("reserved_9", hw.reserved_9, "%u");
	__HAL_AUX_ENTRY("rmac_pause_cnt", hw.rmac_pause_cnt, "%u");
	__HAL_AUX_ENTRY("rmac_xgmii_data_err_cnt", (unsigned long long)
			hw.rmac_xgmii_data_err_cnt, "%llu");
	__HAL_AUX_ENTRY("rmac_xgmii_ctrl_err_cnt", (unsigned long long)
			hw.rmac_xgmii_ctrl_err_cnt, "%llu");
	__HAL_AUX_ENTRY("rmac_err_tcp", hw.rmac_err_tcp, "%u");
	__HAL_AUX_ENTRY("rmac_accepted_ip", hw.rmac_accepted_ip, "%u");

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_stats_herc_enchanced - Get Hercules hardware statistics.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read Hercules device hardware statistics.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_mgmt_hw_stats{}, xge_hal_stats_hw_info_t{},
 * xge_hal_aux_stats_tmac_read(), xge_hal_aux_stats_rmac_read(),
 * xge_hal_aux_device_dump().
*/
xge_hal_status_e xge_hal_aux_stats_herc_enchanced(xge_hal_device_h devh,
				  int bufsize, char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_mgmt_hw_stats_t hw;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_hw_stats(devh, &hw,
				     sizeof(xge_hal_mgmt_hw_stats_t));
	if (status != XGE_HAL_OK) {
		return status;
	}
	__HAL_AUX_ENTRY("tmac_frms_oflow", hw.tmac_frms_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_data_octets_oflow", hw.tmac_data_octets_oflow,
			"%u");
	__HAL_AUX_ENTRY("tmac_mcst_frms_oflow", hw.tmac_mcst_frms_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_bcst_frms_oflow", hw.tmac_bcst_frms_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_ttl_octets_oflow", hw.tmac_ttl_octets_oflow,
			"%u");
	__HAL_AUX_ENTRY("tmac_ucst_frms_oflow", hw.tmac_ucst_frms_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_nucst_frms_oflow", hw.tmac_nucst_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("tmac_any_err_frms_oflow", hw.tmac_any_err_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("tmac_vlan_frms", (unsigned long long)hw.tmac_vlan_frms,
			"%llu");
	__HAL_AUX_ENTRY("tmac_vld_ip_oflow", hw.tmac_vld_ip_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_drop_ip_oflow", hw.tmac_drop_ip_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_icmp_oflow", hw.tmac_icmp_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_rst_tcp_oflow", hw.tmac_rst_tcp_oflow, "%u");
	__HAL_AUX_ENTRY("tmac_udp_oflow", hw.tmac_udp_oflow, "%u");
	__HAL_AUX_ENTRY("tpa_unknown_protocol", hw.tpa_unknown_protocol, "%u");
	__HAL_AUX_ENTRY("tpa_parse_failure", hw.tpa_parse_failure, "%u");
	__HAL_AUX_ENTRY("rmac_vld_frms_oflow", hw.rmac_vld_frms_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_data_octets_oflow", hw.rmac_data_octets_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_vld_mcst_frms_oflow", hw.rmac_vld_mcst_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_vld_bcst_frms_oflow", hw.rmac_vld_bcst_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_ttl_octets_oflow", hw.rmac_ttl_octets_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_accepted_ucst_frms_oflow",
			hw.rmac_accepted_ucst_frms_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_accepted_nucst_frms_oflow",
			hw.rmac_accepted_nucst_frms_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_discarded_frms_oflow",
			hw.rmac_discarded_frms_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_drop_events_oflow", hw.rmac_drop_events_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_usized_frms_oflow", hw.rmac_usized_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_osized_frms_oflow", hw.rmac_osized_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_frag_frms_oflow", hw.rmac_frag_frms_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_jabber_frms_oflow", hw.rmac_jabber_frms_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_ip_oflow", hw.rmac_ip_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_drop_ip_oflow", hw.rmac_drop_ip_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_icmp_oflow", hw.rmac_icmp_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_udp_oflow", hw.rmac_udp_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_err_drp_udp_oflow", hw.rmac_err_drp_udp_oflow,
			"%u");
	__HAL_AUX_ENTRY("rmac_pause_cnt_oflow", hw.rmac_pause_cnt_oflow, "%u");
	__HAL_AUX_ENTRY("rmac_ttl_1519_4095_frms",
			(unsigned long long)hw.rmac_ttl_1519_4095_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_4096_8191_frms",
			(unsigned long long)hw.rmac_ttl_4096_8191_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_8192_max_frms",
			(unsigned long long)hw.rmac_ttl_8192_max_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_ttl_gt_max_frms",
			(unsigned long long)hw.rmac_ttl_gt_max_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_osized_alt_frms",
			(unsigned long long)hw.rmac_osized_alt_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_jabber_alt_frms",
			(unsigned long long)hw.rmac_jabber_alt_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_gt_max_alt_frms",
			(unsigned long long)hw.rmac_gt_max_alt_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_vlan_frms",
			(unsigned long long)hw.rmac_vlan_frms, "%llu");
	__HAL_AUX_ENTRY("rmac_fcs_discard", hw.rmac_fcs_discard, "%u");
	__HAL_AUX_ENTRY("rmac_len_discard", hw.rmac_len_discard, "%u");
	__HAL_AUX_ENTRY("rmac_da_discard", hw.rmac_da_discard, "%u");
	__HAL_AUX_ENTRY("rmac_pf_discard", hw.rmac_pf_discard, "%u");
	__HAL_AUX_ENTRY("rmac_rts_discard", hw.rmac_rts_discard, "%u");
	__HAL_AUX_ENTRY("rmac_red_discard", hw.rmac_red_discard, "%u");
	__HAL_AUX_ENTRY("rmac_ingm_full_discard", hw.rmac_ingm_full_discard,
			"%u");
	__HAL_AUX_ENTRY("rmac_accepted_ip_oflow", hw.rmac_accepted_ip_oflow,
			"%u");
	__HAL_AUX_ENTRY("link_fault_cnt", hw.link_fault_cnt, "%u");

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_stats_rmac_read - Read PCI hardware statistics.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read PCI statistics counters, including number of PCI read and
 * write transactions, PCI retries, discards, etc.
 * This is a subset of stats counters from xge_hal_stats_hw_info_t{}.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_mgmt_hw_stats{}, xge_hal_stats_hw_info_t{},
 * xge_hal_aux_stats_tmac_read(), xge_hal_aux_stats_rmac_read(),
 * xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_stats_pci_read(xge_hal_device_h devh, int bufsize,
				char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_mgmt_hw_stats_t hw;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_hw_stats(devh, &hw,
				     sizeof(xge_hal_mgmt_hw_stats_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("new_rd_req_cnt", hw.new_rd_req_cnt, "%u");
	__HAL_AUX_ENTRY("rd_req_cnt", hw.rd_req_cnt, "%u");
	__HAL_AUX_ENTRY("rd_rtry_cnt", hw.rd_rtry_cnt, "%u");
	__HAL_AUX_ENTRY("new_rd_req_rtry_cnt", hw.new_rd_req_rtry_cnt, "%u");
	__HAL_AUX_ENTRY("wr_req_cnt", hw.wr_req_cnt, "%u");
	__HAL_AUX_ENTRY("wr_rtry_rd_ack_cnt", hw.wr_rtry_rd_ack_cnt, "%u");
	__HAL_AUX_ENTRY("new_wr_req_rtry_cnt", hw.new_wr_req_rtry_cnt, "%u");
	__HAL_AUX_ENTRY("new_wr_req_cnt", hw.new_wr_req_cnt, "%u");
	__HAL_AUX_ENTRY("wr_disc_cnt", hw.wr_disc_cnt, "%u");
	__HAL_AUX_ENTRY("wr_rtry_cnt", hw.wr_rtry_cnt, "%u");
	__HAL_AUX_ENTRY("txp_wr_cnt", hw.txp_wr_cnt, "%u");
	__HAL_AUX_ENTRY("rd_rtry_wr_ack_cnt", hw.rd_rtry_wr_ack_cnt, "%u");
	__HAL_AUX_ENTRY("txd_wr_cnt", hw.txd_wr_cnt, "%u");
	__HAL_AUX_ENTRY("txd_rd_cnt", hw.txd_rd_cnt, "%u");
	__HAL_AUX_ENTRY("rxd_wr_cnt", hw.rxd_wr_cnt, "%u");
	__HAL_AUX_ENTRY("rxd_rd_cnt", hw.rxd_rd_cnt, "%u");
	__HAL_AUX_ENTRY("rxf_wr_cnt", hw.rxf_wr_cnt, "%u");
	__HAL_AUX_ENTRY("txf_rd_cnt", hw.txf_rd_cnt, "%u");

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_stats_hal_read - Read HAL (layer) statistics.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read HAL statistics.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 * XGE_HAL_INF_STATS_IS_NOT_READY - Statistics information is not
 * currently available.
 *
 * See also: xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_stats_hal_read(xge_hal_device_h devh,
			int bufsize, char *retbuf, int *retsize)
{
	xge_list_t *item;
	xge_hal_channel_t *channel;
	xge_hal_device_t *hldev = (xge_hal_device_t*)devh;
	xge_hal_status_e status;
	xge_hal_mgmt_device_stats_t devstat;
	xge_hal_mgmt_channel_stats_t chstat;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_device_stats(hldev, &devstat,
				     sizeof(xge_hal_mgmt_device_stats_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("rx_traffic_intr_cnt", devstat.rx_traffic_intr_cnt, "%u");
	__HAL_AUX_ENTRY("tx_traffic_intr_cnt", devstat.tx_traffic_intr_cnt, "%u");
	__HAL_AUX_ENTRY("not_traffic_intr_cnt",
			devstat.not_traffic_intr_cnt, "%u");
	__HAL_AUX_ENTRY("traffic_intr_cnt", devstat.traffic_intr_cnt, "%u");
	__HAL_AUX_ENTRY("total_intr_cnt", devstat.total_intr_cnt, "%u");
	__HAL_AUX_ENTRY("soft_reset_cnt", devstat.soft_reset_cnt, "%u");
	__HAL_AUX_ENTRY("rxufca_lo_adjust_cnt",
			devstat.rxufca_lo_adjust_cnt, "%u");
	__HAL_AUX_ENTRY("rxufca_hi_adjust_cnt",
			devstat.rxufca_hi_adjust_cnt, "%u");
#if defined(XGE_HAL_CONFIG_LRO)
	__HAL_AUX_ENTRY("tot_frms_lroised",
			devstat.tot_frms_lroised, "%u");
	__HAL_AUX_ENTRY("tot_lro_sessions",
			devstat.tot_lro_sessions, "%u");
#endif

	/* for each opened rx channel */
	xge_list_for_each(item, &hldev->ring_channels) {
		char key[XGE_OS_SPRINTF_STRLEN];
		channel = xge_container_of(item, xge_hal_channel_t, item);

		status = xge_hal_mgmt_channel_stats(channel, &chstat,
				     sizeof(xge_hal_mgmt_channel_stats_t));
		if (status != XGE_HAL_OK) {
			return status;
		}

		(void) xge_os_sprintf(key, "ring%d_", channel->post_qid);

		xge_os_strcpy(key+6, "full_cnt");
		__HAL_AUX_ENTRY(key, chstat.out_of_dtrs_cnt, "%u");
		xge_os_strcpy(key+6, "reserve_free_swaps_cnt");
		__HAL_AUX_ENTRY(key, chstat.reserve_free_swaps_cnt, "%u");
		xge_os_strcpy(key+6, "avg_compl_per_intr_cnt");
		__HAL_AUX_ENTRY(key, chstat.avg_compl_per_intr_cnt, "%u");
		xge_os_strcpy(key+6, "total_compl_cnt");
		__HAL_AUX_ENTRY(key, chstat.total_compl_cnt, "%u");
		xge_os_strcpy(key+6, "bump_cnt");
		__HAL_AUX_ENTRY(key, chstat.ring_bump_cnt, "%u");
	}

	/* for each opened tx channel */
	xge_list_for_each(item, &hldev->fifo_channels) {
		char key[XGE_OS_SPRINTF_STRLEN];
		channel = xge_container_of(item, xge_hal_channel_t, item);

		status = xge_hal_mgmt_channel_stats(channel, &chstat,
				     sizeof(xge_hal_mgmt_channel_stats_t));
		if (status != XGE_HAL_OK) {
			return status;
		}

		(void) xge_os_sprintf(key, "fifo%d_", channel->post_qid);

		xge_os_strcpy(key+6, "full_cnt");
		__HAL_AUX_ENTRY(key, chstat.out_of_dtrs_cnt, "%u");
		xge_os_strcpy(key+6, "reserve_free_swaps_cnt");
		__HAL_AUX_ENTRY(key, chstat.reserve_free_swaps_cnt, "%u");
		xge_os_strcpy(key+6, "avg_compl_per_intr_cnt");
		__HAL_AUX_ENTRY(key, chstat.avg_compl_per_intr_cnt, "%u");
		xge_os_strcpy(key+6, "total_compl_cnt");
		__HAL_AUX_ENTRY(key, chstat.total_compl_cnt, "%u");
		xge_os_strcpy(key+6, "total_posts");
		__HAL_AUX_ENTRY(key, chstat.total_posts, "%u");
		xge_os_strcpy(key+6, "total_posts_many");
		__HAL_AUX_ENTRY(key, chstat.total_posts_many, "%u");
		xge_os_strcpy(key+6, "total_buffers");
		__HAL_AUX_ENTRY(key, chstat.total_buffers, "%u");
		xge_os_strcpy(key+6, "avg_buffers_per_post");
		__HAL_AUX_ENTRY(key, chstat.avg_buffers_per_post, "%u");
		xge_os_strcpy(key+6, "avg_buffer_size");
		__HAL_AUX_ENTRY(key, chstat.avg_buffer_size, "%u");
		xge_os_strcpy(key+6, "avg_post_size");
		__HAL_AUX_ENTRY(key, chstat.avg_post_size, "%u");
		xge_os_strcpy(key+6, "total_posts_dtrs_many");
		__HAL_AUX_ENTRY(key, chstat.total_posts_dtrs_many, "%u");
		xge_os_strcpy(key+6, "total_posts_frags_many");
		__HAL_AUX_ENTRY(key, chstat.total_posts_frags_many, "%u");
		xge_os_strcpy(key+6, "total_posts_dang_dtrs");
		__HAL_AUX_ENTRY(key, chstat.total_posts_dang_dtrs, "%u");
		xge_os_strcpy(key+6, "total_posts_dang_frags");
		__HAL_AUX_ENTRY(key, chstat.total_posts_dang_frags, "%u");
	}

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}



/**
 * xge_hal_aux_stats_sw_dev_read - Read software device statistics.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read software-maintained device statistics.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 * XGE_HAL_INF_STATS_IS_NOT_READY - Statistics information is not
 * currently available.
 *
 * See also: xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_stats_sw_dev_read(xge_hal_device_h devh,
				int bufsize, char *retbuf, int *retsize)
{
	xge_hal_device_t *hldev = (xge_hal_device_t*)devh;
	xge_hal_status_e status;
	xge_hal_mgmt_sw_stats_t sw_dev_err_stats;
	int t_code;
	char buf[XGE_OS_SPRINTF_STRLEN];

	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_sw_stats(hldev, &sw_dev_err_stats,
				     sizeof(xge_hal_mgmt_sw_stats_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("sm_err_cnt",sw_dev_err_stats.sm_err_cnt, "%u");
	__HAL_AUX_ENTRY("single_ecc_err_cnt",sw_dev_err_stats.single_ecc_err_cnt, "%u");
	__HAL_AUX_ENTRY("double_ecc_err_cnt",sw_dev_err_stats.double_ecc_err_cnt, "%u");
	__HAL_AUX_ENTRY("ecc_err_cnt", sw_dev_err_stats.ecc_err_cnt, "%u");
	__HAL_AUX_ENTRY("parity_err_cnt",sw_dev_err_stats.parity_err_cnt, "%u");
	__HAL_AUX_ENTRY("serr_cnt",sw_dev_err_stats.serr_cnt, "%u");

	for (t_code = 1; t_code < 16; t_code++) {
	        int t_code_cnt = sw_dev_err_stats.rxd_t_code_err_cnt[t_code];
	        if (t_code_cnt)  {
			(void) xge_os_sprintf(buf, "rxd_t_code_%d", t_code);
			__HAL_AUX_ENTRY(buf, t_code_cnt, "%u");
	        }
	        t_code_cnt = sw_dev_err_stats.txd_t_code_err_cnt[t_code];
		if (t_code_cnt)	{
			(void) xge_os_sprintf(buf, "txd_t_code_%d", t_code);
			__HAL_AUX_ENTRY(buf, t_code_cnt, "%u");
		}
	}

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_pci_config_read - Retrieve and format PCI Configuration
 * info.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Retrieve about info (using xge_hal_mgmt_pci_config()) and sprintf it
 * into the provided @retbuf.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_mgmt_pci_config(), xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_pci_config_read(xge_hal_device_h devh, int	bufsize,
				char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_mgmt_pci_config_t pci_config;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_pci_config(devh, &pci_config,
				  sizeof(xge_hal_mgmt_pci_config_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("vendor_id", pci_config.vendor_id, "0x%04X");
	__HAL_AUX_ENTRY("device_id", pci_config.device_id, "0x%04X");
	__HAL_AUX_ENTRY("command", pci_config.command, "0x%04X");
	__HAL_AUX_ENTRY("status", pci_config.status, "0x%04X");
	__HAL_AUX_ENTRY("revision", pci_config.revision, "0x%02X");
	__HAL_AUX_ENTRY("pciClass1", pci_config.pciClass[0], "0x%02X");
	__HAL_AUX_ENTRY("pciClass2", pci_config.pciClass[1], "0x%02X");
	__HAL_AUX_ENTRY("pciClass3", pci_config.pciClass[2], "0x%02X");
	__HAL_AUX_ENTRY("cache_line_size",
			pci_config.cache_line_size, "0x%02X");
	__HAL_AUX_ENTRY("latency_timer", pci_config.latency_timer, "0x%02X");
	__HAL_AUX_ENTRY("header_type", pci_config.header_type, "0x%02X");
	__HAL_AUX_ENTRY("bist", pci_config.bist, "0x%02X");
	__HAL_AUX_ENTRY("base_addr0_lo", pci_config.base_addr0_lo, "0x%08X");
	__HAL_AUX_ENTRY("base_addr0_hi", pci_config.base_addr0_hi, "0x%08X");
	__HAL_AUX_ENTRY("base_addr1_lo", pci_config.base_addr1_lo, "0x%08X");
	__HAL_AUX_ENTRY("base_addr1_hi", pci_config.base_addr1_hi, "0x%08X");
	__HAL_AUX_ENTRY("not_Implemented1",
			pci_config.not_Implemented1, "0x%08X");
	__HAL_AUX_ENTRY("not_Implemented2", pci_config.not_Implemented2,
			"0x%08X");
	__HAL_AUX_ENTRY("cardbus_cis_pointer", pci_config.cardbus_cis_pointer,
			"0x%08X");
	__HAL_AUX_ENTRY("subsystem_vendor_id", pci_config.subsystem_vendor_id,
			"0x%04X");
	__HAL_AUX_ENTRY("subsystem_id", pci_config.subsystem_id, "0x%04X");
	__HAL_AUX_ENTRY("rom_base", pci_config.rom_base, "0x%08X");
	__HAL_AUX_ENTRY("capabilities_pointer",
			pci_config.capabilities_pointer, "0x%02X");
	__HAL_AUX_ENTRY("interrupt_line", pci_config.interrupt_line, "0x%02X");
	__HAL_AUX_ENTRY("interrupt_pin", pci_config.interrupt_pin, "0x%02X");
	__HAL_AUX_ENTRY("min_grant", pci_config.min_grant, "0x%02X");
	__HAL_AUX_ENTRY("max_latency", pci_config.max_latency, "0x%02X");
	__HAL_AUX_ENTRY("msi_cap_id", pci_config.msi_cap_id, "0x%02X");
	__HAL_AUX_ENTRY("msi_next_ptr", pci_config.msi_next_ptr, "0x%02X");
	__HAL_AUX_ENTRY("msi_control", pci_config.msi_control, "0x%04X");
	__HAL_AUX_ENTRY("msi_lower_address", pci_config.msi_lower_address,
			"0x%08X");
	__HAL_AUX_ENTRY("msi_higher_address", pci_config.msi_higher_address,
			"0x%08X");
	__HAL_AUX_ENTRY("msi_data", pci_config.msi_data, "0x%04X");
	__HAL_AUX_ENTRY("msi_unused", pci_config.msi_unused, "0x%04X");
	__HAL_AUX_ENTRY("vpd_cap_id", pci_config.vpd_cap_id, "0x%02X");
	__HAL_AUX_ENTRY("vpd_next_cap", pci_config.vpd_next_cap, "0x%02X");
	__HAL_AUX_ENTRY("vpd_addr", pci_config.vpd_addr, "0x%04X");
	__HAL_AUX_ENTRY("vpd_data", pci_config.vpd_data, "0x%08X");
	__HAL_AUX_ENTRY("pcix_cap", pci_config.pcix_cap, "0x%02X");
	__HAL_AUX_ENTRY("pcix_next_cap", pci_config.pcix_next_cap, "0x%02X");
	__HAL_AUX_ENTRY("pcix_command", pci_config.pcix_command, "0x%04X");
	__HAL_AUX_ENTRY("pcix_status", pci_config.pcix_status, "0x%08X");

	if (xge_hal_device_check_id(devh) == XGE_HAL_CARD_HERC) {
		char key[XGE_OS_SPRINTF_STRLEN];
		int i;

		for (i = 0;
		     i < (XGE_HAL_PCI_XFRAME_CONFIG_SPACE_SIZE - 0x68)/4;
		     i++) {
			(void) xge_os_sprintf(key, "%03x:", 4*i + 0x68);
			__HAL_AUX_ENTRY(key, *((int *)pci_config.rsvd_b1 + i),
					"0x%08X");
		}
	}

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}


/**
 * xge_hal_aux_channel_read - Read channels information.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read HAL statistics.
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_OUT_OF_SPACE - Buffer size is very small.
 * See also: xge_hal_aux_device_dump().
 */
xge_hal_status_e xge_hal_aux_channel_read(xge_hal_device_h devh,
				int bufsize, char *retbuf, int *retsize)
{
	xge_list_t *item;
	xge_hal_channel_t *channel;
	xge_hal_device_t *hldev = (xge_hal_device_t*)devh;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	if (hldev->magic != XGE_HAL_MAGIC) {
                return XGE_HAL_ERR_INVALID_DEVICE;
        }

	/* for each opened rx channel */
	xge_list_for_each(item, &hldev->ring_channels) {
		char key[XGE_OS_SPRINTF_STRLEN];
		channel = xge_container_of(item, xge_hal_channel_t, item);

		if (channel->is_open != 1)
			continue;

		(void) xge_os_sprintf(key, "ring%d_", channel->post_qid);
		xge_os_strcpy(key+6, "type");
		__HAL_AUX_ENTRY(key, channel->type, "%u");
		xge_os_strcpy(key+6, "length");
		__HAL_AUX_ENTRY(key, channel->length, "%u");
		xge_os_strcpy(key+6, "is open");
		__HAL_AUX_ENTRY(key, channel->is_open, "%u");
		xge_os_strcpy(key+6, "reserve initial");
		__HAL_AUX_ENTRY(key, channel->reserve_initial, "%u");
		xge_os_strcpy(key+6, "reserve max");
		__HAL_AUX_ENTRY(key, channel->reserve_max, "%u");
		xge_os_strcpy(key+6, "reserve length");
		__HAL_AUX_ENTRY(key, channel->reserve_length, "%u");
		xge_os_strcpy(key+6, "reserve top");
		__HAL_AUX_ENTRY(key, channel->reserve_top, "%u");
		xge_os_strcpy(key+6, "reserve threshold");
		__HAL_AUX_ENTRY(key, channel->reserve_threshold, "%u");
		xge_os_strcpy(key+6, "free length");
		__HAL_AUX_ENTRY(key, channel->free_length, "%u");
		xge_os_strcpy(key+6, "post index");
		__HAL_AUX_ENTRY(key, channel->post_index, "%u");
		xge_os_strcpy(key+6, "compl index");
		__HAL_AUX_ENTRY(key, channel->compl_index, "%u");
		xge_os_strcpy(key+6, "per dtr space");
		__HAL_AUX_ENTRY(key, channel->per_dtr_space, "%u");
	}

	/* for each opened tx channel */
	xge_list_for_each(item, &hldev->fifo_channels) {
		char key[XGE_OS_SPRINTF_STRLEN];
		channel = xge_container_of(item, xge_hal_channel_t, item);

		if (channel->is_open != 1)
			continue;

		(void) xge_os_sprintf(key, "fifo%d_", channel->post_qid);
		xge_os_strcpy(key+6, "type");
		__HAL_AUX_ENTRY(key, channel->type, "%u");
		xge_os_strcpy(key+6, "length");
		__HAL_AUX_ENTRY(key, channel->length, "%u");
		xge_os_strcpy(key+6, "is open");
		__HAL_AUX_ENTRY(key, channel->is_open, "%u");
		xge_os_strcpy(key+6, "reserve initial");
		__HAL_AUX_ENTRY(key, channel->reserve_initial, "%u");
		xge_os_strcpy(key+6, "reserve max");
		__HAL_AUX_ENTRY(key, channel->reserve_max, "%u");
		xge_os_strcpy(key+6, "reserve length");
		__HAL_AUX_ENTRY(key, channel->reserve_length, "%u");
		xge_os_strcpy(key+6, "reserve top");
		__HAL_AUX_ENTRY(key, channel->reserve_top, "%u");
		xge_os_strcpy(key+6, "reserve threshold");
		__HAL_AUX_ENTRY(key, channel->reserve_threshold, "%u");
		xge_os_strcpy(key+6, "free length");
		__HAL_AUX_ENTRY(key, channel->free_length, "%u");
		xge_os_strcpy(key+6, "post index");
		__HAL_AUX_ENTRY(key, channel->post_index, "%u");
		xge_os_strcpy(key+6, "compl index");
		__HAL_AUX_ENTRY(key, channel->compl_index, "%u");
		xge_os_strcpy(key+6, "per dtr space");
		__HAL_AUX_ENTRY(key, channel->per_dtr_space, "%u");
	}

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

/**
 * xge_hal_aux_device_dump - Dump driver "about" info and device state.
 * @devh: HAL device handle.
 *
 * Dump driver & device "about" info and device state,
 * including all BAR0 registers, hardware and software statistics, PCI
 * configuration space.
 * See also: xge_hal_aux_about_read(), xge_hal_mgmt_reg_read(),
 * xge_hal_aux_pci_config_read(), xge_hal_aux_stats_sw_dev_read(),
 * xge_hal_aux_stats_tmac_read(), xge_hal_aux_stats_rmac_read(),
 * xge_hal_aux_channel_read(), xge_hal_aux_stats_hal_read().
 * Returns:
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_OUT_OF_SPACE - Buffer size is very small.
 */
xge_hal_status_e
xge_hal_aux_device_dump(xge_hal_device_h devh)
{
	xge_hal_device_t *hldev = (xge_hal_device_t*)devh;
	xge_hal_status_e status;
	int retsize;
	int offset;
	u64 retval;

	xge_assert(hldev->dump_buf != NULL);

	xge_os_println("********* xge DEVICE DUMP BEGIN **********");

	status = xge_hal_aux_about_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                hldev->dump_buf,
	                                &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);


	for (offset = 0; offset < 1574; offset++) {

		status = xge_hal_mgmt_reg_read(hldev, 0, offset*8, &retval);
		if (status != XGE_HAL_OK) {
			goto error;
		}

		if (!retval) continue;

		xge_os_printf("0x%04x 0x%08x%08x", offset*8,
					(u32)(retval>>32), (u32)retval);
	}
	xge_os_println("\n");

	status = xge_hal_aux_pci_config_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                     hldev->dump_buf,
	                                     &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	status = xge_hal_aux_stats_tmac_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                     hldev->dump_buf,
	                                     &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	status = xge_hal_aux_stats_rmac_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                     hldev->dump_buf,
	                                     &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	status = xge_hal_aux_stats_pci_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                    hldev->dump_buf,
	                                    &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	if (xge_hal_device_check_id(hldev) == XGE_HAL_CARD_HERC) {
		status = xge_hal_aux_stats_herc_enchanced(hldev,
			     XGE_HAL_DUMP_BUF_SIZE, hldev->dump_buf, &retsize);
		if (status != XGE_HAL_OK) {
			goto error;
		}
		xge_os_println(hldev->dump_buf);
	}

	status = xge_hal_aux_stats_sw_dev_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
					     hldev->dump_buf, &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	status = xge_hal_aux_channel_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                  hldev->dump_buf,
	                                  &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	status = xge_hal_aux_stats_hal_read(hldev, XGE_HAL_DUMP_BUF_SIZE,
	                                    hldev->dump_buf,
	                                    &retsize);
	if (status != XGE_HAL_OK) {
		goto error;
	}
	xge_os_println(hldev->dump_buf);

	xge_os_println("********* XFRAME DEVICE DUMP END **********");

error:
	return status;
}


/**
 * xge_hal_aux_driver_config_read - Read Driver configuration.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read driver configuration,
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_aux_device_config_read().
 */
xge_hal_status_e
xge_hal_aux_driver_config_read(int bufsize, char *retbuf, int *retsize)
{
	xge_hal_status_e status;
	xge_hal_driver_config_t  drv_config;
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_driver_config(&drv_config,
					  sizeof(xge_hal_driver_config_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("queue size initial",
			drv_config.queue_size_initial, "%u");
	__HAL_AUX_ENTRY("queue size max", drv_config.queue_size_max, "%u");
	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}


/**
 * xge_hal_aux_device_config_read - Read device configuration.
 * @devh: HAL device handle.
 * @bufsize: Buffer size.
 * @retbuf: Buffer pointer.
 * @retsize: Size of the result. Cannot be greater than @bufsize.
 *
 * Read device configuration,
 *
 * Returns: XGE_HAL_OK - success.
 * XGE_HAL_ERR_INVALID_DEVICE - Device is not valid.
 * XGE_HAL_ERR_VERSION_CONFLICT - Version it not maching.
 *
 * See also: xge_hal_aux_driver_config_read().
 */
xge_hal_status_e xge_hal_aux_device_config_read(xge_hal_device_h devh,
				int bufsize, char *retbuf, int *retsize)
{
	int i;
	xge_hal_status_e status;
	xge_hal_device_config_t  dev_config;
	char key[XGE_OS_SPRINTF_STRLEN];
	__HAL_AUX_ENTRY_DECLARE(bufsize, retbuf);

	status = xge_hal_mgmt_device_config(devh, &dev_config,
					  sizeof(xge_hal_device_config_t));
	if (status != XGE_HAL_OK) {
		return status;
	}

	__HAL_AUX_ENTRY("mtu", dev_config.mtu, "%u");
	__HAL_AUX_ENTRY("isr polling count", dev_config.isr_polling_cnt, "%u");
	__HAL_AUX_ENTRY("latency timer", dev_config.latency_timer, "%u");
	__HAL_AUX_ENTRY("max split transactions",
			dev_config.max_splits_trans, "%u");
	__HAL_AUX_ENTRY("mmrb count", dev_config.mmrb_count, "%d");
	__HAL_AUX_ENTRY("shared splits", dev_config.shared_splits, "%u");
	__HAL_AUX_ENTRY("statistics refresh time(in sec)",
			dev_config.stats_refresh_time_sec, "%u");
	__HAL_AUX_ENTRY("pci freq(in mherz)", dev_config.pci_freq_mherz, "%u");
	__HAL_AUX_ENTRY("intr mode", dev_config.intr_mode, "%u");
	__HAL_AUX_ENTRY("sched timer(in us)", dev_config.sched_timer_us, "%u");
	__HAL_AUX_ENTRY("sched timer one shot(flag)",
			dev_config.sched_timer_one_shot,  "%u");
	__HAL_AUX_ENTRY("ring memblock size",
			dev_config.ring.memblock_size,  "%u");

	for(i = 0; i < XGE_HAL_MAX_RING_NUM;  i++)
	{
		xge_hal_ring_queue_t *ring = &dev_config.ring.queue[i];
		xge_hal_rti_config_t *rti =  &ring->rti;

		if (!ring->configured)
			continue;

		(void) xge_os_sprintf(key, "ring%d_", i);
		xge_os_strcpy(key+6, "inital rxd blocks");
		__HAL_AUX_ENTRY(key, ring->initial, "%u");
		xge_os_strcpy(key+6, "max rxd blocks");
		__HAL_AUX_ENTRY(key, ring->max, "%u");
		xge_os_strcpy(key+6, "buffer mode");
		__HAL_AUX_ENTRY(key, ring->buffer_mode, "%u");
		xge_os_strcpy(key+6, "dram size(in mb)");
		__HAL_AUX_ENTRY(key, ring->dram_size_mb, "%u");
		xge_os_strcpy(key+6, "backoff interval(in us)");
		__HAL_AUX_ENTRY(key, ring->backoff_interval_us, "%u");
		xge_os_strcpy(key+6, "max frame len");
		__HAL_AUX_ENTRY(key, ring->max_frm_len, "%d");
		xge_os_strcpy(key+6, "priority");
		__HAL_AUX_ENTRY(key, ring->priority,  "%u");
		xge_os_strcpy(key+6, "rth en");
		__HAL_AUX_ENTRY(key, ring->rth_en,  "%u");
		xge_os_strcpy(key+6, "no snoop bits");
		__HAL_AUX_ENTRY(key, ring->no_snoop_bits,  "%u");
		xge_os_strcpy(key+6, "urange_a");
		__HAL_AUX_ENTRY(key, rti->urange_a,  "%u");
		xge_os_strcpy(key+6, "ufc_a");
		__HAL_AUX_ENTRY(key, rti->ufc_a,  "%u");
		xge_os_strcpy(key+6, "urange_b");
		__HAL_AUX_ENTRY(key, rti->urange_b,  "%u");
		xge_os_strcpy(key+6, "ufc_b");
		__HAL_AUX_ENTRY(key, rti->ufc_b,  "%u");
		xge_os_strcpy(key+6, "urange_c");
		__HAL_AUX_ENTRY(key, rti->urange_c,  "%u");
		xge_os_strcpy(key+6, "ufc_c");
		__HAL_AUX_ENTRY(key, rti->ufc_c,  "%u");
		xge_os_strcpy(key+6, "ufc_d");
		__HAL_AUX_ENTRY(key, rti->ufc_d,  "%u");
		xge_os_strcpy(key+6, "timer val(in us)");
		__HAL_AUX_ENTRY(key, rti->timer_val_us,  "%u");
	}

	{
		xge_hal_mac_config_t *mac= &dev_config.mac;

		__HAL_AUX_ENTRY("tmac util period",
				mac->tmac_util_period, "%u");
		__HAL_AUX_ENTRY("rmac util period",
				mac->rmac_util_period, "%u");
		__HAL_AUX_ENTRY("rmac bcast enable(flag)",
				mac->rmac_bcast_en, "%u");
		__HAL_AUX_ENTRY("rmac pause generator enable(flag)",
				mac->rmac_pause_gen_en, "%d");
		__HAL_AUX_ENTRY("rmac pause receive enable(flag)",
				mac->rmac_pause_rcv_en, "%d");
		__HAL_AUX_ENTRY("rmac pause time",
				mac->rmac_pause_time, "%u");
		__HAL_AUX_ENTRY("mc pause threshold qoq3",
				mac->mc_pause_threshold_q0q3, "%u");
		__HAL_AUX_ENTRY("mc pause threshold q4q7",
				mac->mc_pause_threshold_q4q7, "%u");
	}

	{
		xge_hal_tti_config_t *tti = &dev_config.tti;
		__HAL_AUX_ENTRY("tti enabled", tti->enabled, "%u");
		__HAL_AUX_ENTRY("tti urange_a", tti->urange_a, "%u");
		__HAL_AUX_ENTRY("tti ufc_a", tti->ufc_a, "%u");
		__HAL_AUX_ENTRY("tti urange_b", tti->urange_b, "%u");
		__HAL_AUX_ENTRY("tti ufc_b", tti->ufc_b, "%u");
		__HAL_AUX_ENTRY("tti urange_c", tti->urange_c, "%u");
		__HAL_AUX_ENTRY("tti ufc_c", tti->ufc_c, "%u");
		__HAL_AUX_ENTRY("tti urange_d", tti->urange_d, "%u");
		__HAL_AUX_ENTRY("tti ufc_d", tti->ufc_d, "%u");
		__HAL_AUX_ENTRY("tti timer val(in us)",
				tti->timer_val_us, "%u");
		__HAL_AUX_ENTRY("tti timer ci en(flag)",
				tti->timer_ci_en, "%u");
	}


	__HAL_AUX_ENTRY("fifo max frags", dev_config.fifo.max_frags, "%u");
	__HAL_AUX_ENTRY("fifo reserve threshold",
			dev_config.fifo.reserve_threshold, "%u");
	__HAL_AUX_ENTRY("fifo memblock size",
			dev_config.fifo.memblock_size, "%u");
#ifdef XGE_HAL_ALIGN_XMIT
	__HAL_AUX_ENTRY("fifo alignment size",
			dev_config.fifo.alignment_size, "%u");
#endif

	for(i = 0; i < XGE_HAL_MAX_FIFO_NUM;  i++)
	{
		xge_hal_fifo_queue_t *fifo = &dev_config.fifo.queue[i];

		if (!fifo->configured)
			continue;

		(void) xge_os_sprintf(key, "fifo%d_", i);
		xge_os_strcpy(key+6, "initial len");
		__HAL_AUX_ENTRY(key, fifo->initial, "%u");
		xge_os_strcpy(key+6, "max len");
		__HAL_AUX_ENTRY(key, fifo->max, "%u");
		xge_os_strcpy(key+6, "intr mode");
		__HAL_AUX_ENTRY(key, fifo->intr, "%u");
		xge_os_strcpy(key+6, "no snoop bits");
		__HAL_AUX_ENTRY(key, fifo->no_snoop_bits, "%u");
	}
	__HAL_AUX_ENTRY("dump on serr(flag)", dev_config.dump_on_serr, "%u");
	__HAL_AUX_ENTRY("dump on ecc err(flag)",
			dev_config.dump_on_eccerr, "%u");
	__HAL_AUX_ENTRY("dump on parity err(flag)",
			dev_config.dump_on_parityerr, "%u");
	__HAL_AUX_ENTRY("rth en(flag)", dev_config.rth_en, "%u");
	__HAL_AUX_ENTRY("rth bucket size", dev_config.rth_bucket_size, "%u");

	__HAL_AUX_ENTRY_END(bufsize, retsize);

	return XGE_HAL_OK;
}

