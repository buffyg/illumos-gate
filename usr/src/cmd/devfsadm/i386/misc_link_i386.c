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

#include <regex.h>
#include <devfsadm.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

static int lp(di_minor_t minor, di_node_t node);
static int serial_dialout(di_minor_t minor, di_node_t node);
static int serial(di_minor_t minor, di_node_t node);
static int diskette(di_minor_t minor, di_node_t node);
static int vt00(di_minor_t minor, di_node_t node);
static int kdmouse(di_minor_t minor, di_node_t node);
static int bmc(di_minor_t minor, di_node_t node);
static int agp_process(di_minor_t minor, di_node_t node);

static devfsadm_create_t misc_cbt[] = {
	{ "vt00", "ddi_display", NULL,
	    TYPE_EXACT, ILEVEL_0,	vt00
	},
	{ "mouse", "ddi_mouse", "mouse8042",
	    TYPE_EXACT | DRV_EXACT, ILEVEL_0, kdmouse
	},
	{ "pseudo", "ddi_pseudo", "bmc",
	    TYPE_EXACT | DRV_EXACT, ILEVEL_0, bmc,
	},
	{ "disk",  "ddi_block:diskette", NULL,
	    TYPE_EXACT, ILEVEL_1, diskette
	},
	{ "parallel",  "ddi_printer", NULL,
	    TYPE_EXACT, ILEVEL_1, lp
	},
	{ "serial", "ddi_serial:mb", NULL,
	    TYPE_EXACT, ILEVEL_1, serial
	},
	{ "serial",  "ddi_serial:dialout,mb", NULL,
	    TYPE_EXACT, ILEVEL_1, serial_dialout
	},
	{"agp", "ddi_agp:pseudo", NULL,
	    TYPE_EXACT, ILEVEL_0, agp_process
	},
	{"agp", "ddi_agp:target", NULL,
	    TYPE_EXACT, ILEVEL_0, agp_process
	},
	{"agp", "ddi_agp:cpugart", NULL,
	    TYPE_EXACT, ILEVEL_0, agp_process
	},
	{"agp", "ddi_agp:master", NULL,
	    TYPE_EXACT, ILEVEL_0, agp_process
	}
};

DEVFSADM_CREATE_INIT_V0(misc_cbt);

static char *debug_mid = "agp_mid";

typedef enum {
	DRIVER_AGPPSEUDO = 0,
	DRIVER_AGPTARGET,
	DRIVER_CPUGART,
	DRIVER_AGPMASTER,
	DRIVER_UNKNOWN
} driver_defs_t;

typedef struct {
	char	*driver_name;
	int	index;
} driver_name_table_entry_t;

static driver_name_table_entry_t driver_name_table[] = {
	{ "agpgart",		DRIVER_AGPPSEUDO },
	{ "agptarget",		DRIVER_AGPTARGET },
	{ "amd64_gart",		DRIVER_CPUGART },
	{ "vgatext",		DRIVER_AGPMASTER },
	{ NULL,			DRIVER_UNKNOWN }
};

static devfsadm_enumerate_t agptarget_rules[1] =
	{ "^agp$/^agptarget([0-9]+)$", 1, MATCH_ALL };
static devfsadm_enumerate_t cpugart_rules[1] =
	{ "^agp$/^cpugart([0-9]+)$", 1, MATCH_ALL };
static devfsadm_enumerate_t agpmaster_rules[1] =
	{  "^agp$/^agpmaster([0-9]+)$", 1, MATCH_ALL };

static devfsadm_remove_t misc_remove_cbt[] = {
	{ "vt", "vt[0-9][0-9]", RM_PRE|RM_ALWAYS,
		ILEVEL_0, devfsadm_rm_all
	}
};

DEVFSADM_REMOVE_INIT_V0(misc_remove_cbt);

/*
 * Handles minor node type "ddi_display", in addition to generic processing
 * done by display().
 *
 * This creates a /dev/vt00 link to /dev/fb, for backwards compatibility.
 */
/* ARGSUSED */
int
vt00(di_minor_t minor, di_node_t node)
{
	(void) devfsadm_secondary_link("vt00", "fb", 0);
	return (DEVFSADM_CONTINUE);
}

/*
 * type=ddi_block:diskette;addr=0,0;minor=c        diskette
 * type=ddi_block:diskette;addr=0,0;minor=c,raw    rdiskette
 * type=ddi_block:diskette;addr1=0;minor=c diskette\A2
 * type=ddi_block:diskette;addr1=0;minor=c,raw     rdiskette\A2
 */
static int
diskette(di_minor_t minor, di_node_t node)
{
	char *a2;
	char link[PATH_MAX];
	char *addr = di_bus_addr(node);
	char *mn = di_minor_name(minor);

	if (strcmp(addr, "0,0") == 0) {
		if (strcmp(mn, "c") == 0) {
			(void) devfsadm_mklink("diskette", node, minor, 0);
		} else if (strcmp(mn, "c,raw") == 0) {
			(void) devfsadm_mklink("rdiskette", node, minor, 0);
		}

	}

	if (addr[0] == '0') {
		if ((a2 = strchr(addr, ',')) != NULL) {
			a2++;
			if (strcmp(mn, "c") == 0) {
				(void) strcpy(link, "diskette");
				(void) strcat(link, a2);
				(void) devfsadm_mklink(link, node, minor, 0);
			} else if (strcmp(mn, "c,raw") == 0) {
				(void) strcpy(link, "rdiskette");
				(void) strcat(link, a2);
				(void) devfsadm_mklink(link, node, minor, 0);
			}
		}
	}

	return (DEVFSADM_CONTINUE);
}

/*
 * type=ddi_printer;name=lp;addr=1,3bc      lp0
 * type=ddi_printer;name=lp;addr=1,378      lp1
 * type=ddi_printer;name=lp;addr=1,278      lp2
 */
static int
lp(di_minor_t minor, di_node_t node)
{
	char *addr = di_bus_addr(node);
	char *buf;
	char path[PATH_MAX + 1];
	devfsadm_enumerate_t rules[1] = {"^ecpp([0-9]+)$", 1, MATCH_ALL};

	if (strcmp(addr, "1,3bc") == 0) {
		(void) devfsadm_mklink("lp0", node, minor, 0);

	} else if (strcmp(addr, "1,378") == 0) {
		(void) devfsadm_mklink("lp1", node, minor, 0);

	} else if (strcmp(addr, "1,278") == 0) {
		(void) devfsadm_mklink("lp2", node, minor, 0);
	}

	if (strcmp(di_driver_name(node), "ecpp") != 0) {
		return (DEVFSADM_CONTINUE);
	}

	if ((buf = di_devfs_path(node)) == NULL) {
		return (DEVFSADM_CONTINUE);
	}

	(void) snprintf(path, sizeof (path), "%s:%s",
	    buf, di_minor_name(minor));

	di_devfs_path_free(buf);

	if (devfsadm_enumerate_int(path, 0, &buf, rules, 1)) {
		return (DEVFSADM_CONTINUE);
	}

	(void) snprintf(path, sizeof (path), "ecpp%s", buf);
	free(buf);
	(void) devfsadm_mklink(path, node, minor, 0);
	return (DEVFSADM_CONTINUE);
}

/*
 * type=ddi_serial:mb;minor=a      tty00
 * type=ddi_serial:mb;minor=b      tty01
 * type=ddi_serial:mb;minor=c      tty02
 * type=ddi_serial:mb;minor=d      tty03
 */
static int
serial(di_minor_t minor, di_node_t node)
{

	char *mn = di_minor_name(minor);
	char link[PATH_MAX];

	(void) strcpy(link, "tty");
	(void) strcat(link, mn);
	(void) devfsadm_mklink(link, node, minor, 0);

	if (strcmp(mn, "a") == 0) {
		(void) devfsadm_mklink("tty00", node, minor, 0);

	} else if (strcmp(mn, "b") == 0) {
		(void) devfsadm_mklink("tty01", node, minor, 0);

	} else if (strcmp(mn, "c") == 0) {
		(void) devfsadm_mklink("tty02", node, minor, 0);

	} else if (strcmp(mn, "d") == 0) {
		(void) devfsadm_mklink("tty03", node, minor, 0);
	}
	return (DEVFSADM_CONTINUE);
}

/*
 * type=ddi_serial:dialout,mb;minor=a,cu   ttyd0
 * type=ddi_serial:dialout,mb;minor=b,cu   ttyd1
 * type=ddi_serial:dialout,mb;minor=c,cu   ttyd2
 * type=ddi_serial:dialout,mb;minor=d,cu   ttyd3
 */
static int
serial_dialout(di_minor_t minor, di_node_t node)
{
	char *mn = di_minor_name(minor);

	if (strcmp(mn, "a,cu") == 0) {
		(void) devfsadm_mklink("ttyd0", node, minor, 0);
		(void) devfsadm_mklink("cua0", node, minor, 0);

	} else if (strcmp(mn, "b,cu") == 0) {
		(void) devfsadm_mklink("ttyd1", node, minor, 0);
		(void) devfsadm_mklink("cua1", node, minor, 0);

	} else if (strcmp(mn, "c,cu") == 0) {
		(void) devfsadm_mklink("ttyd2", node, minor, 0);
		(void) devfsadm_mklink("cua2", node, minor, 0);

	} else if (strcmp(mn, "d,cu") == 0) {
		(void) devfsadm_mklink("ttyd3", node, minor, 0);
		(void) devfsadm_mklink("cua3", node, minor, 0);
	}
	return (DEVFSADM_CONTINUE);
}

static int
kdmouse(di_minor_t minor, di_node_t node)
{
	(void) devfsadm_mklink("kdmouse", node, minor, 0);
	return (DEVFSADM_CONTINUE);
}

static int
bmc(di_minor_t minor, di_node_t node)
{
	(void) devfsadm_mklink("bmc", node, minor, 0);
	return (DEVFSADM_CONTINUE);
}
static int
agp_process(di_minor_t minor, di_node_t node)
{
	char *minor_nm, *drv_nm;
	char *devfspath;
	char *I_path, *p_path, *buf;
	char *name = (char *)NULL;
	int i, index;
	devfsadm_enumerate_t rules[1];

	minor_nm = di_minor_name(minor);
	drv_nm = di_driver_name(node);

	if ((minor_nm == NULL) || (drv_nm == NULL)) {
		return (DEVFSADM_CONTINUE);
	}

	devfsadm_print(debug_mid, "agp_process: minor=%s node=%s\n",
		minor_nm, di_node_name(node));

	devfspath = di_devfs_path(node);
	if (devfspath == NULL) {
		devfsadm_print(debug_mid, "agp_process: devfspath is NULL\n");
		return (DEVFSADM_CONTINUE);
	}

	I_path = (char *)malloc(PATH_MAX);

	if (I_path == NULL) {
		di_devfs_path_free(devfspath);
		devfsadm_print(debug_mid,  "agp_process: malloc failed\n");
		return (DEVFSADM_CONTINUE);
	}

	p_path = (char *)malloc(PATH_MAX);

	if (p_path == NULL) {
		devfsadm_print(debug_mid,  "agp_process: malloc failed\n");
		di_devfs_path_free(devfspath);
		free(I_path);
		return (DEVFSADM_CONTINUE);
	}

	(void) strlcpy(p_path, devfspath, PATH_MAX);
	(void) strlcat(p_path, ":", PATH_MAX);
	(void) strlcat(p_path, minor_nm, PATH_MAX);
	di_devfs_path_free(devfspath);

	devfsadm_print(debug_mid, "agp_process: path %s\n", p_path);

	for (i = 0; ; i++) {
		if ((driver_name_table[i].driver_name == NULL) ||
		    (strcmp(drv_nm, driver_name_table[i].driver_name) == 0)) {
			index = driver_name_table[i].index;
			break;
		}
	}
	switch (index) {
	case DRIVER_AGPPSEUDO:
		devfsadm_print(debug_mid,
			    "agp_process: psdeudo driver name\n");
		name = "agpgart";
		(void) snprintf(I_path, PATH_MAX, "%s", name);
		devfsadm_print(debug_mid,
		    "mklink %s -> %s\n", I_path, p_path);

		(void) devfsadm_mklink(I_path, node, minor, 0);

		free(I_path);
		free(p_path);
		return (DEVFSADM_CONTINUE);
	case DRIVER_AGPTARGET:
		devfsadm_print(debug_mid,
			    "agp_process: target driver name\n");
		rules[0] = agptarget_rules[0];
		name = "agptarget";
		break;
	case DRIVER_CPUGART:
		devfsadm_print(debug_mid,
			    "agp_process: cpugart driver name\n");
		rules[0] = cpugart_rules[0];
		name = "cpugart";
		break;
	case DRIVER_AGPMASTER:
		devfsadm_print(debug_mid,
			    "agp_process: agpmaster driver name\n");
		rules[0] = agpmaster_rules[0];
		name = "agpmaster";
		break;
	case DRIVER_UNKNOWN:
		devfsadm_print(debug_mid,
			    "agp_process: unknown driver name=%s\n", drv_nm);
		free(I_path);
		free(p_path);
		return (DEVFSADM_CONTINUE);
	}

	if (devfsadm_enumerate_int(p_path, 0, &buf, rules, 1)) {
		devfsadm_print(debug_mid, "agp_process: exit/coninue\n");
		free(I_path);
		free(p_path);
		return (DEVFSADM_CONTINUE);
	}


	(void) snprintf(I_path, PATH_MAX, "agp/%s%s", name, buf);

	devfsadm_print(debug_mid, "agp_process: p_path=%s buf=%s\n",
		    p_path, buf);

	free(buf);

	devfsadm_print(debug_mid, "mklink %s -> %s\n", I_path, p_path);

	(void) devfsadm_mklink(I_path, node, minor, 0);

	free(p_path);
	free(I_path);

	return (DEVFSADM_CONTINUE);
}
