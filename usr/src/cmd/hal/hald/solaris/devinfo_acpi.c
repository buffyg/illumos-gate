/***************************************************************************
 *
 * devinfo_acpi : acpi devices
 *
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Licensed under the Academic Free License version 2.1
 *
 **************************************************************************/

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <libdevinfo.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../osspec.h"
#include "../logger.h"
#include "../hald.h"
#include "../hald_dbus.h"
#include "../device_info.h"
#include "../util.h"
#include "../hald_runner.h"
#include "devinfo_acpi.h"

#define		DEVINFO_PROBE_BATTERY_TIMEOUT	30000

static HalDevice *devinfo_acpi_add(HalDevice *, di_node_t, char *, char *);
static HalDevice *devinfo_battery_add(HalDevice *, di_node_t, char *, char *);

DevinfoDevHandler devinfo_acpi_handler = {
	devinfo_acpi_add,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

DevinfoDevHandler devinfo_battery_handler = {
	devinfo_battery_add,
	NULL,
	NULL,
	NULL,
	NULL,
	devinfo_battery_get_prober
};

static HalDevice *
devinfo_acpi_add(HalDevice *parent, di_node_t node, char *devfs_path,
    char *device_type)
{
	HalDevice *d, *computer;

	if (strcmp(devfs_path, "/acpi") != 0) {
		return (NULL);
	}

	d = hal_device_new();

	if ((computer = hal_device_store_find(hald_get_gdl(),
	    "/org/freedesktop/Hal/devices/computer")) ||
	    (computer = hal_device_store_find(hald_get_tdl(),
	    "/org/freedesktop/Hal/devices/computer"))) {
		hal_device_property_set_string(computer,
		    "power_management.type", "acpi");
	}
	devinfo_set_default_properties(d, parent, node, devfs_path);
	devinfo_add_enqueue(d, devfs_path, &devinfo_acpi_handler);

	return (d);
}

static HalDevice *
devinfo_battery_add(HalDevice *parent, di_node_t node, char *devfs_path,
    char *device_type)
{
	HalDevice *d, *computer;
	char	*driver_name;
	di_devlink_handle_t devlink_hdl;
	int	major;
	di_minor_t minor;
	dev_t   dev;
	char    *minor_path = NULL;
	char    *devpath;

	driver_name = di_driver_name(node);
	if ((driver_name == NULL) || (strcmp(driver_name, "battery") != 0)) {
		return (NULL);
	}

	d = hal_device_new();

	if ((computer = hal_device_store_find(hald_get_gdl(),
	    "/org/freedesktop/Hal/devices/computer")) ||
	    (computer = hal_device_store_find(hald_get_tdl(),
	    "/org/freedesktop/Hal/devices/computer"))) {
		hal_device_property_set_string(computer,
		    "system.formfactor", "laptop");
	}
	devinfo_set_default_properties(d, parent, node, devfs_path);
	devinfo_add_enqueue(d, devfs_path, &devinfo_battery_handler);

	major = di_driver_major(node);
	if ((devlink_hdl = di_devlink_init(NULL, 0)) == NULL) {
		return (d);
	}
	minor = DI_MINOR_NIL;
	while ((minor = di_minor_next(node, minor)) != DI_MINOR_NIL) {
		dev = di_minor_devt(minor);
		if ((major != major(dev)) ||
		    (di_minor_type(minor) != DDM_MINOR) ||
		    (di_minor_spectype(minor) != S_IFCHR) ||
		    ((minor_path = di_devfs_minor_path(minor)) == NULL)) {
			continue;
		}

		if (hal_device_store_match_key_value_string(hald_get_gdl(),
		    "solaris.devfs_path", minor_path) == NULL) {
			devinfo_battery_add_minor(d, node, minor_path, dev);
		}

		di_devfs_path_free(minor_path);
	}
	di_devlink_fini(&devlink_hdl);

	return (d);
}

void
devinfo_battery_add_minor(HalDevice *parent, di_node_t node, char *minor_path,
    dev_t dev)
{
	HalDevice *d;

	d = hal_device_new();
	devinfo_set_default_properties(d, parent, node, minor_path);
	devinfo_add_enqueue(d, minor_path, &devinfo_battery_handler);
}

void
devinfo_battery_device_rescan(char *parent_devfs_path, gchar *udi)
{
	HalDevice *d = NULL;

	d = hal_device_store_find(hald_get_gdl(), udi);
	if (d == NULL) {
		HAL_INFO(("device not found %s", udi));
		return;
	}

	hald_runner_run(d, "hald-probe-battery", NULL,
	    DEVINFO_PROBE_BATTERY_TIMEOUT, devinfo_battery_rescan_probing_done,
	    NULL, NULL);
}

static void
devinfo_battery_rescan_probing_done(HalDevice *d, guint32 exit_type,
    gint return_code, char **error, gpointer userdata1, gpointer userdata2)
{
	/* hald_runner_run() requires this function since cannot pass NULL */
}

const gchar *
devinfo_battery_get_prober(HalDevice *d, int *timeout)
{
	*timeout = DEVINFO_PROBE_BATTERY_TIMEOUT;    /* 30 second timeout */
	return ("hald-probe-battery");
}
