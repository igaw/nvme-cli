// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016 Intel Corporation. All rights reserved.
 * Copyright (c) 2016 HGST, a Western Digital Company.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This file implements the discovery controller feature of NVMe over
 * Fabrics specification standard.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <libnvme.h>

#ifdef HAVE_LIBKMOD
#include <libkmod.h>
#endif

#include "common.h"
#include "nvme.h"
#include "nvme-print.h"
#include "fabrics.h"
#include "util/cleanup.h"
#include "logging.h"
#include "util/sighdl.h"

#define PATH_NVMF_DISC		SYSCONFDIR "/nvme/discovery.conf"
#define PATH_NVMF_CONFIG	SYSCONFDIR "/nvme/config.json"
#define PATH_NVMF_RUNDIR	RUNDIR "/nvme"
#define MAX_DISC_ARGS		32
#define MAX_DISC_RETRIES	10

#define NVMF_DEF_DISC_TMO	30

/* Name of file to output log pages in their raw format */
static char *raw;
static bool persistent;
static bool quiet;
static bool dump_config;

static const char *nvmf_tport		= "transport type";
static const char *nvmf_traddr		= "transport address";
static const char *nvmf_nqn		= "subsystem nqn";
static const char *nvmf_trsvcid		= "transport service id (e.g. IP port)";
static const char *nvmf_htraddr		= "host traddr (e.g. FC WWN's)";
static const char *nvmf_hiface		= "host interface (for tcp transport)";
static const char *nvmf_hostnqn		= "user-defined hostnqn";
static const char *nvmf_hostid		= "user-defined hostid (if default not used)";
static const char *nvmf_hostkey		= "user-defined dhchap key (if default not used)";
static const char *nvmf_ctrlkey		= "user-defined dhchap controller key (for bi-directional authentication)";
static const char *nvmf_nr_io_queues	= "number of io queues to use (default is core count)";
static const char *nvmf_nr_write_queues	= "number of write queues to use (default 0)";
static const char *nvmf_nr_poll_queues	= "number of poll queues to use (default 0)";
static const char *nvmf_queue_size	= "number of io queue elements to use (default 128)";
static const char *nvmf_keep_alive_tmo	= "keep alive timeout period in seconds";
static const char *nvmf_reconnect_delay	= "reconnect timeout period in seconds";
static const char *nvmf_ctrl_loss_tmo	= "controller loss timeout period in seconds";
static const char *nvmf_fast_io_fail_tmo = "fast I/O fail timeout (default off)";
static const char *nvmf_tos		= "type of service";
static const char *nvmf_keyring		= "Keyring for TLS key lookup (key id or keyring name)";
static const char *nvmf_tls_key		= "TLS key to use (key id or key in interchange format)";
static const char *nvmf_tls_key_legacy	= "TLS key to use (key id)";
static const char *nvmf_tls_key_identity = "TLS key identity";
static const char *nvmf_dup_connect	= "allow duplicate connections between same transport host and subsystem port";
static const char *nvmf_disable_sqflow	= "disable controller sq flow control (default false)";
static const char *nvmf_hdr_digest	= "enable transport protocol header digest (TCP transport)";
static const char *nvmf_data_digest	= "enable transport protocol data digest (TCP transport)";
static const char *nvmf_tls		= "enable TLS";
static const char *nvmf_concat		= "enable secure concatenation";
static const char *nvmf_config_file	= "Use specified JSON configuration file or 'none' to disable";
static const char *nvmf_context		= "execution context identification string";

struct nvmf_args {
	const char *subsysnqn;
	const char *transport;
	const char *traddr;
	const char *host_traddr;
	const char *host_iface;
	const char *trsvcid;
	const char *hostnqn;
	const char *hostid;
	const char *hostkey;
	const char *ctrlkey;
	const char *keyring;
	const char *tls_key;
	const char *tls_key_identity;
	int queue_size;
	int nr_io_queues;
	int reconnect_delay;
	int ctrl_loss_tmo;
	int fast_io_fail_tmo;
	int keep_alive_tmo;
	int nr_write_queues;
	int nr_poll_queues;
	int tos;
	long keyring_id;
	long tls_key_id;
	long tls_configured_key_id;
	bool duplicate_connect;
	bool disable_sqflow;
	bool hdr_digest;
	bool data_digest;
	bool tls;
	bool concat;
};

#define NVMF_ARGS(n, f, ...)                                                                  \
	NVME_ARGS(n,                                                                              \
		OPT_STRING("transport",       't', "STR", &f.transport,     nvmf_tport),         \
		OPT_STRING("nqn",             'n', "STR", &f.subsysnqn,     nvmf_nqn),           \
		OPT_STRING("traddr",          'a', "STR", &f.traddr,        nvmf_traddr),        \
		OPT_STRING("trsvcid",         's', "STR", &f.trsvcid,       nvmf_trsvcid),       \
		OPT_STRING("host-traddr",     'w', "STR", &f.host_traddr,   nvmf_htraddr),       \
		OPT_STRING("host-iface",      'f', "STR", &f.host_iface,    nvmf_hiface),        \
		OPT_STRING("hostnqn",         'q', "STR", &f.hostnqn,       nvmf_hostnqn),       \
		OPT_STRING("hostid",          'I', "STR", &f.hostid,        nvmf_hostid),        \
		OPT_STRING("dhchap-secret",   'S', "STR", &f.hostkey,       nvmf_hostkey),       \
		OPT_STRING("dhchap-ctrl-secret", 'C', "STR", &f.ctrlkey,    nvmf_ctrlkey),       \
		OPT_STRING("keyring",          0,  "STR", &f.keyring,       nvmf_keyring),       \
		OPT_STRING("tls-key",          0,  "STR", &f.tls_key,       nvmf_tls_key),       \
		OPT_STRING("tls-key-identity", 0,  "STR", &f.tls_key_identity, nvmf_tls_key_identity), \
		OPT_INT("nr-io-queues",       'i', &f.nr_io_queues,       nvmf_nr_io_queues),    \
		OPT_INT("nr-write-queues",    'W', &f.nr_write_queues,    nvmf_nr_write_queues), \
		OPT_INT("nr-poll-queues",     'P', &f.nr_poll_queues,     nvmf_nr_poll_queues),  \
		OPT_INT("queue-size",         'Q', &f.queue_size,         nvmf_queue_size),      \
		OPT_INT("keep-alive-tmo",     'k', &f.keep_alive_tmo,     nvmf_keep_alive_tmo),  \
		OPT_INT("reconnect-delay",    'c', &f.reconnect_delay,    nvmf_reconnect_delay), \
		OPT_INT("ctrl-loss-tmo",      'l', &f.ctrl_loss_tmo,      nvmf_ctrl_loss_tmo),   \
		OPT_INT("fast_io_fail_tmo",   'F', &f.fast_io_fail_tmo,   nvmf_fast_io_fail_tmo),\
		OPT_INT("tos",                'T', &f.tos,                nvmf_tos),             \
		OPT_INT("tls_key",              0, &f.tls_key_id,         nvmf_tls_key_legacy),  \
		OPT_FLAG("duplicate-connect", 'D', &f.duplicate_connect,  nvmf_dup_connect),     \
		OPT_FLAG("disable-sqflow",      0, &f.disable_sqflow,     nvmf_disable_sqflow),  \
		OPT_FLAG("hdr-digest",        'g', &f.hdr_digest,         nvmf_hdr_digest),      \
		OPT_FLAG("data-digest",       'G', &f.data_digest,        nvmf_data_digest),     \
		OPT_FLAG("tls",                 0, &f.tls,                nvmf_tls),             \
		OPT_FLAG("concat",              0, &f.concat,             nvmf_concat),          \
		##__VA_ARGS__                                                                    \
	)

static void nvmf_default_args(struct nvmf_args *fa)
{
	fa->tos = -1;
	fa->ctrl_loss_tmo = NVMF_DEF_CTRL_LOSS_TMO;
}

static void save_discovery_log(char *raw, struct nvmf_discovery_log *log)
{
	uint64_t numrec = le64_to_cpu(log->numrec);
	int fd, len, ret;

	fd = open(raw, O_CREAT | O_RDWR | O_TRUNC, 0600);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s: %s\n", raw, libnvme_strerror(errno));
		return;
	}

	len = sizeof(struct nvmf_discovery_log) + numrec * sizeof(struct nvmf_disc_log_entry);

	ret = write(fd, log, len);
	if (ret < 0)
		fprintf(stderr, "failed to write to %s: %s\n",
			raw, libnvme_strerror(errno));
	else
		printf("Discovery log is saved to %s\n", raw);

	close(fd);
}

#ifdef CONFIG_LIBSYSTEMD
#include <systemd/sd-bus.h>

static int systemd_daemon_reload()
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus *bus;
	int err;

	err = sd_bus_open_system(&bus);
	if (err < 0)
		return err;

	err = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		"Reload", &error, NULL, "");

	if (err < 0)
		fprintf(stderr, "%s\n", error.message);

	sd_bus_error_free(&error);
	sd_bus_unref(bus);

	if (err > 0)
		print_debug("systemd daemon reload\n");

	return err > 0? 0 : err;
}

static int systemd_start_unit(const char *unit_name)
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus *bus = NULL;
	int err;

	err = sd_bus_open_system(&bus);
	if (err < 0)
		return err;

	err = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "RestartUnit", &error, NULL,
		"ss", unit_name, "replace");
	if (err < 0)
		fprintf(stderr, "%s\n", error.message);

	sd_bus_error_free(&error);
	sd_bus_unref(bus);

	if (err > 0)
		print_debug("systemd unit started: %s\n", unit_name);

	return err > 0? 0 : err;
}

struct unit_entry {
	struct list_node list;
	char *name;
};

static int create_unit(struct libnvmf_context *fctx,
		struct nvmf_disc_log_entry *e,
		int id, char **punit_name)
{
	__cleanup_free char *filename = NULL;
	__cleanup_free char *unit_name = NULL;
	__cleanup_free char *path = NULL;
	__cleanup_free char *pnqn = NULL;
	__cleanup_free char *cli = NULL;
	const char *val;
	FILE *f;
	int err;

	err = asprintf(&pnqn, "%d-%s", id, e->subnqn);
	if (err < 0)
		return -errno;

	err = sd_bus_path_encode("/nvme", pnqn, &path);
	if (err < 0)
		return err;

	err = asprintf(&unit_name, "nvme-connect-%s.service", pnqn);
	if (err < 0)
		return -errno;

	err = asprintf(&filename, "/run/systemd/system/%s", unit_name);
	if (err < 0)
		return -errno;

	err = asprintf(&cli, "%s/nvme", get_current_dir_name());
	if (err < 0)
		return -errno;

	f = fopen(filename, "w");
	if (!f)
		return -errno;


	fprintf(f, "[Unit]\n");
	fprintf(f, "Description=NVMe connection to %s\n", e->subnqn);
	fprintf(f, "After=network-online.target\n");
	fprintf(f, "Wants=network-online.target\n\n");

	fprintf(f, "[Service]\n");
	fprintf(f, "Type=simple\n");

	fprintf(f, "ExecStart=%s connect \\\n", cli);
	if (is_printable_at_level(LIBNVME_LOG_DEBUG))
		fprintf(f, "  -vv \\\n");
	fprintf(f, "  --disconnect-on-sigint \\\n");
	fprintf(f, "  --transport=%s \\\n", libnvmf_trtype_str(e->trtype));
	fprintf(f, "  --traddr=%s \\\n", e->traddr);
	fprintf(f, "  --trsvcid=%s \\\n", e->trsvcid);
	fprintf(f, "  --nqn=%s \\\n", e->subnqn);
	fprintf(f, "  --hostnqn=%s ", libnvmf_context_get_hostnqn(fctx));
	val = libnvmf_context_get_host_traddr(fctx);
	if (val)
		fprintf(f, "\\\n  --host-traddr=%s ", val);
	val = libnvmf_context_get_host_iface(fctx);
	if (val)
		fprintf(f, "\\\n  --host-iface=%s", val);
	fprintf(f, "\n");

	fprintf(f, "[Install]\n");
	fprintf(f, "WantedBy=multi-user.target\n");

	fclose(f);

	print_debug("systemd unit created: %s\n", filename);

	*punit_name = unit_name;
	unit_name = NULL;

	return 0;
}

static int create_ctrl_udev_rule(struct libnvmf_context *fctx,
		struct nvmf_disc_log_entry *e,
		int id, const char *unit_name)
{
	__cleanup_free char *filename = NULL;
	__cleanup_free char *path = NULL;
	__cleanup_free char *pnqn = NULL;
	const char *val;
	FILE *f;
	int err;

	err = asprintf(&pnqn, "%d-%s", id, e->subnqn);
	if (err < 0)
		return -errno;

	err = sd_bus_path_encode("/nvme", pnqn, &path);
	if (err < 0)
		return err;

	err = asprintf(&filename, "/run/udev/rules.d/99-nvme-%d-%s.rules",
		id, e->subnqn);
	if (err < 0)
		return -errno;

	f = fopen(filename, "w");
	if (!f)
		return -errno;

	fprintf(f, "# Generated for %s\n", unit_name);
	fprintf(f, "ACTION==\"add|change\", SUBSYSTEM==\"nvme\", \\\n");
	val = libnvmf_context_get_host_traddr(fctx);
	fprintf(f, "  ATTR{address}==\"traddr=%s,host_traddr=%s\", \\\n", e->traddr, val);
	fprintf(f, "  ATTR{transport}==\"%s\", \\\n", libnvmf_trtype_str(e->trtype));
	fprintf(f, "  ATTR{subsysnqn}==\"%s\", \\\n", e->subnqn);
	fprintf(f, "  TAG+=\"systemd\", \\\n");
	fprintf(f, "  ENV{SYSTEMD_WANTS}+=\"%s\"\n", unit_name);

	fclose(f);

	print_debug("udev rule created: %s\n", filename);
	return 0;
}

static int create_subsystem_udev_rule(struct libnvmf_context *fctx,
		struct nvmf_disc_log_entry *e,
		int id, const char *unit_name)
{
	__cleanup_free char *filename = NULL;
	FILE *f;
	int err;

	return 0;

	err = asprintf(&filename, "/run/udev/rules.d/99-nvme-subsystem-%s.rules",
		e->subnqn);
	if (err < 0)
		return -errno;

	f = fopen(filename, "w");
	if (!f)
		return -errno;

	fprintf(f, "ACTION==\"add|change\", SUBSYSTEM==\"nvme-subsystem\", \\\n");
	fprintf(f, "  ATTRS{subsysnqn}==\"%s\", \\\n", e->subnqn);
	fprintf(f, "  TAG+=\"systemd\", \\\n");
	fprintf(f, "  ENV{SYSTEMD_WANTS}+=\"%s\"\n", unit_name);

	fclose(f);
	print_debug("udev rule created: %s\n", filename);

	return 0;
}

static int create_ns_udev_rule(struct libnvmf_context *fctx,
		struct nvmf_disc_log_entry *e,
		int id, const char *unit_name)
{
	__cleanup_free char *filename = NULL;
	FILE *f;
	int err;

	return 0;

	err = asprintf(&filename, "/run/udev/rules.d/99-nvme-ns-%s.rules",
		e->subnqn);
	if (err < 0)
		return -errno;

	f = fopen(filename, "w");
	if (!f)
		return -errno;

	fprintf(f, "ACTION==\"add|change\", SUBSYSTEM==\"block\", ENV{DEVTYPE}==\"disk\", \\\n");
	fprintf(f, "  ATTRS{subsysnqn}==\"%s\", \\\n", e->subnqn);
	fprintf(f, "  TAG+=\"systemd\", \\\n");
	fprintf(f, "  ENV{SYSTEMD_WANTS}+=\"%s\"\n", unit_name);

	fclose(f);
	print_debug("udev rule created: %s\n", filename);

	return 0;
}

static int udev_reload(void)
{
	int err;
	/* Reload the daemon's internal cache of rules */

	err = system("udevadm control --reload-rules");
	if (err)
		return -errno;
	err = system("udevadm trigger --action=add --subsystem-match=block");
	if (err)
		return -errno;

	print_debug("udevd reloaded and restarted\n");

	return 0;
}

static enum nvmf_trtype str_to_trtype(const char *str)
{
	if (!str)
		return NVMF_TRTYPE_UNSPECIFIED;

	if (!strcasecmp(str, "rdma"))
		return NVMF_TRTYPE_RDMA;
	else if (!strcasecmp(str, "fc"))
		return NVMF_TRTYPE_FC;
	else if (!strcasecmp(str, "tcp"))
		return NVMF_TRTYPE_TCP;
	else if (!strcasecmp(str, "loop"))
		return NVMF_TRTYPE_LOOP;

	return NVMF_TRTYPE_UNSPECIFIED;
}

static void create_units(struct libnvmf_context *fctx,
		struct nvmf_discovery_log *log, uint64_t numrec)
{
	struct unit_entry *unit, *_unit;
	LIST_HEAD(unit_list);
	int err, i;
	enum nvmf_trtype trtype;
	bool reload_udevd = true;

	trtype = str_to_trtype(libnvmf_context_get_transport(fctx));

	for (i = 0; i < numrec; i++) {
		struct nvmf_disc_log_entry *e = &log->entries[i];
		char *unit_name;

		if (e->subtype != NVME_NQN_NVME)
			continue;

		if (e->trtype != trtype)
			continue;

		err = create_unit(fctx, e, i, &unit_name);
		if (err) {
			fprintf(stderr,
				"Failed to create unit file: %s\n",
				strerror(-err));
			goto cleanup_list;
		}

		err = mkdir("/run/udev/rules.d/", 0x755);
		if (err &&  && errno != EEXIST) {
			fprintf(stderr, "Skip udev rules setup\n");
			reload_udevd = false;
			goto add_unit;
		}

		err = create_ctrl_udev_rule(fctx, e, i, unit_name);
		if (err) {
			fprintf(stderr,
				"Failed to create ctrl udev file: %s\n",
				strerror(-err));
			goto cleanup_list;
		}

		err = create_subsystem_udev_rule(fctx, e, i, unit_name);
		if (err) {
			fprintf(stderr,
				"Failed to create subsytem udev file: %s\n",
				strerror(-err));
			goto cleanup_list;
		}

		err = create_ns_udev_rule(fctx, e, i, unit_name);
		if (err) {
			fprintf(stderr,
				"Failed to create ns udev file: %s\n",
				strerror(-err));
			goto cleanup_list;
		}

	add_unit:
		unit = calloc(1, sizeof(*unit));
		if (!unit)
			goto cleanup_list;

		list_node_init(&unit->list);
		unit->name = unit_name;

		list_add_tail(&unit_list, &unit->list);
	}

	if (reload_udevd) {
		err = udev_reload();
		if (err) {
			fprintf(stderr, "Failed to reload udev rules");
			goto cleanup_list;
		}
	}

	err = systemd_daemon_reload();
	if (err && err != -ENOTSUP) {
		fprintf(stderr, "Failed to systemd reload daemon: %s\n",
			strerror(-err));
		goto cleanup_list;
	}

	list_for_each(&unit_list, unit, list) {
		err = systemd_start_unit(unit->name);
		if (err)
			fprintf(stderr, "Failed to start unit: %s\n",
				strerror(-err));
	}

cleanup_list:
	list_for_each_safe(&unit_list, unit, _unit, list) {
		free(unit->name);
		free(unit);
	}
}
#else
static void create_units(struct nvmf_context *fctx,
		struct nvmf_discovery_log *log, uint64_t numrec)
{}
#endif

static int setup_common_context(struct libnvmf_context *fctx,
		struct nvmf_args *fa);

struct cb_fabrics_data {
	struct nvmf_args *fa;
	nvme_print_flags_t flags;
	bool quiet;
	bool units;
	char *raw;
	char **argv;
	FILE *f;
	struct libnvme_ctrl *ctrl;
};

static bool cb_decide_retry(struct libnvmf_context *fctx, int err,
		void *user_data)
{
	if (err == -EAGAIN || (err == -EINTR && !nvme_signal_received)) {
		print_debug("nvmf_add_ctrl returned '%s'\n", libnvme_strerror(-err));
		return true;
	}

	return false;
}

static void cb_connected(struct libnvmf_context *fctx,
		struct libnvme_ctrl *c, void *user_data)
{
	struct cb_fabrics_data *cfd = user_data;

	cfd->ctrl = c;

	if (cfd->quiet)
		return;

	if (cfd->flags == NORMAL) {
		printf("connecting to device: %s\n", libnvme_ctrl_get_name(c));
		return;
	}

#ifdef CONFIG_JSONC
	if (cfd->flags == JSON) {
		struct json_object *root;

		root = json_create_object();

		json_object_add_value_string(root, "device",
			libnvme_ctrl_get_name(c));

		json_print_object(root, NULL);
		printf("\n");
		json_free_object(root);
	}
#endif
}

static void cb_already_connected(struct libnvmf_context *fctx,
		struct libnvme_ctrl *c, void *user_data)
{
	struct cb_fabrics_data *cfd = user_data;

	cfd->ctrl = c;

	if (quiet)
		return;

	fprintf(stderr,	"already connected to hostnqn=%s,nqn=%s,transport=%s,traddr=%s,host_traddr=%s,host_iface=%s,trsvcid=%s\n",
		libnvmf_context_get_hostnqn(fctx),
		libnvme_ctrl_get_subsysnqn(c),
		libnvme_ctrl_get_transport(c),
		libnvme_ctrl_get_traddr(c),
		libnvmf_context_get_host_traddr(fctx),
		libnvmf_context_get_host_iface(fctx),
		libnvme_ctrl_get_trsvcid(c));
}

static void cb_discovery_log(struct libnvmf_context *fctx,
		bool connect, struct nvmf_discovery_log *log,
		uint64_t numrec, void *user_data)
{
	struct cb_fabrics_data *cfd = user_data;

	if (cfd->units)
		create_units(fctx, log, numrec);
	else if (cfd->raw)
		save_discovery_log(cfd->raw, log);
	else if (!connect)
		nvme_show_discovery_log(log, numrec, cfd->flags);
}

static int cb_parser_init(struct libnvmf_context *dctx, void *user_data)
{
	struct cb_fabrics_data *cfd = user_data;

	cfd->f = fopen(PATH_NVMF_DISC, "r");
	if (cfd->f == NULL) {
		fprintf(stderr, "No params given and no %s\n", PATH_NVMF_DISC);
		return -ENOENT;
	}

	cfd->argv = calloc(MAX_DISC_ARGS, sizeof(char *));
	if (!cfd->argv)
		return -1;

	cfd->argv[0] = "discover";

	return 0;
}

static void cb_parser_cleanup(struct libnvmf_context *fctx, void *user_data)
{
	struct cb_fabrics_data *cfd = user_data;

	free(cfd->argv);
	fclose(cfd->f);
}

static int set_fabrics_options(struct libnvmf_context *fctx,
		struct nvmf_args *fa)
{
	struct libnvme_fabrics_config *fcfg;

	fcfg = libnvmf_context_get_fabrics_config(fctx);
	if (!fcfg)
		return -EINVAL;

	libnvme_fabrics_config_set_nr_io_queues(fcfg, fa->nr_io_queues);
	libnvme_fabrics_config_set_reconnect_delay(fcfg, fa->reconnect_delay);
	libnvme_fabrics_config_set_ctrl_loss_tmo(fcfg, fa->ctrl_loss_tmo);
	libnvme_fabrics_config_set_fast_io_fail_tmo(fcfg, fa->fast_io_fail_tmo);
	libnvme_fabrics_config_set_keep_alive_tmo(fcfg, fa->keep_alive_tmo);
	libnvme_fabrics_config_set_nr_write_queues(fcfg, fa->nr_write_queues);
	libnvme_fabrics_config_set_nr_poll_queues(fcfg, fa->nr_poll_queues);
	libnvme_fabrics_config_set_tos(fcfg, fa->tos);
	libnvme_fabrics_config_set_keyring_id(fcfg, fa->keyring_id);
	libnvme_fabrics_config_set_tls_key_id(fcfg, fa->tls_key_id);
	libnvme_fabrics_config_set_tls_configured_key_id(fcfg, fa->tls_configured_key_id);
	libnvme_fabrics_config_set_duplicate_connect(fcfg, fa->duplicate_connect);
	libnvme_fabrics_config_set_disable_sqflow(fcfg, fa->disable_sqflow);
	libnvme_fabrics_config_set_hdr_digest(fcfg, fa->hdr_digest);
	libnvme_fabrics_config_set_data_digest(fcfg, fa->data_digest);
	libnvme_fabrics_config_set_tls(fcfg, fa->tls);
	libnvme_fabrics_config_set_concat(fcfg, fa->concat);

	return 0;
}

static int cb_parser_next_line(struct libnvmf_context *fctx, void *user_data)
{
	struct cb_fabrics_data *cfd = user_data;
	struct nvmf_args fa;
	char *ptr, *p, line[4096];
	int argc, ret = 0;
	bool force = false;

	NVMF_ARGS(opts, fa,
		  OPT_FLAG("persistent",   'p', &persistent, "persistent discovery connection"),
		  OPT_FLAG("force",          0, &force,      "Force persistent discovery controller creation"));

	memcpy(&fa, cfd->fa, sizeof(fa));
next:
	if (fgets(line, sizeof(line), cfd->f) == NULL)
		return -EOF;

	if (line[0] == '#' || line[0] == '\n')
		goto next;

	argc = 1;
	p = line;
	while ((ptr = strsep(&p, " =\n")) != NULL)
		cfd->argv[argc++] = ptr;
	cfd->argv[argc] = NULL;

	fa.subsysnqn = NVME_DISC_SUBSYS_NAME;
	ret = argconfig_parse(argc, cfd->argv, "config", opts);
	if (ret)
		goto next;
	if (!fa.transport && !fa.traddr)
		goto next;

	if (!fa.trsvcid)
		fa.trsvcid = libnvmf_get_default_trsvcid(fa.transport, true);

	ret = setup_common_context(fctx, &fa);
	if (ret)
		return ret;

	ret = set_fabrics_options(fctx, &fa);
	if (ret)
		return ret;

	return 0;
}

static int setup_common_context(struct libnvmf_context *fctx,
		struct nvmf_args *fa)
{
	int err;

	err = libnvmf_context_set_connection(fctx,
		fa->subsysnqn, fa->transport,
		fa->traddr, fa->trsvcid,
		fa->host_traddr, fa->host_iface);
	if (err)
		return err;

	err = libnvmf_context_set_hostnqn(fctx,
		fa->hostnqn, fa->hostid);
	if (err)
		return err;

	err = libnvmf_context_set_crypto(fctx,
		fa->hostkey, fa->ctrlkey,
		fa->keyring, fa->tls_key,
		fa->tls_key_identity);
	if (err)
		return err;

	return 0;
}

static int create_common_context(struct libnvme_global_ctx *ctx,
		bool persistent, struct nvmf_args *fa,
		void *user_data, struct libnvmf_context **fctxp)
{
	struct libnvmf_context *fctx;
	int err;

	err = libnvmf_context_create(ctx, cb_decide_retry, cb_connected,
		cb_already_connected, user_data, &fctx);
	if (err)
		return err;

	err = libnvmf_context_set_connection(fctx, fa->subsysnqn,
		fa->transport, fa->traddr, fa->trsvcid,
		fa->host_traddr, fa->host_iface);
	if (err)
		goto err;

	err = libnvmf_context_set_hostnqn(fctx, fa->hostnqn, fa->hostid);
	if (err)
		goto err;

	err = set_fabrics_options(fctx, fa);
	if (err)
		goto err;

	err = libnvmf_context_set_crypto(fctx, fa->hostkey, fa->ctrlkey,
		fa->keyring, fa->tls_key, fa->tls_key_identity);
	if (err)
		goto err;

	libnvmf_context_set_persistent(fctx, persistent);

	*fctxp = fctx;

	return 0;

err:
	libnvmf_context_free(fctx);
	return err;
}

static int create_discovery_context(struct libnvme_global_ctx *ctx,
		bool persistent, const char *device,
		struct nvmf_args *fa,
		void *user_data, struct libnvmf_context **fctxp)
{
	struct libnvmf_context *fctx;
	int err;

	err = create_common_context(ctx, persistent, fa, user_data,
		&fctx);
	if (err)
		return err;

	err = libnvmf_context_set_discovery_cbs(fctx, cb_discovery_log,
		cb_parser_init, cb_parser_cleanup, cb_parser_next_line);
	if (err)
		goto err;

	err = libnvmf_context_set_discovery_defaults(fctx, MAX_DISC_RETRIES,
		NVMF_DEF_DISC_TMO);
	if (err)
		goto err;

	libnvmf_context_set_device(fctx, device);

	*fctxp = fctx;
	return 0;

err:
	libnvmf_context_free(fctx);
	return err;
}

static int nvme_read_volatile_config(struct libnvme_global_ctx *ctx)
{
	char *filename, *ext;
	struct dirent *dir;
	DIR *d;
	int ret = -ENOENT;

	d = opendir(PATH_NVMF_RUNDIR);
	if (!d)
		return -ENOTDIR;

	while ((dir = readdir(d))) {
		if (dir->d_type != DT_REG)
			continue;

		ext = strchr(dir->d_name, '.');
		if (!ext || strcmp("json", ext + 1))
			continue;

		if (asprintf(&filename, "%s/%s", PATH_NVMF_RUNDIR, dir->d_name) < 0) {
			ret = -ENOMEM;
			break;
		}

		if (libnvme_read_config(ctx, filename))
			ret = 0;

		free(filename);
	}
	closedir(d);

	return ret;
}

static int nvme_read_config_checked(struct libnvme_global_ctx *ctx,
				    const char *filename)
{
	if (access(filename, F_OK))
		return -errno;

	return libnvme_read_config(ctx, filename);
}

static void load_nvme_fabrics_module(void)
{
#ifdef HAVE_LIBKMOD
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	int err, state;
	int timeout = 20; /* 2 seconds */

	ctx = kmod_new(NULL, NULL);
	if (!ctx)
		return;

	err = kmod_module_new_from_name(ctx, "nvme-fabrics", &mod);
	if (err)
		goto unref;

	state = kmod_module_get_initstate(mod);
	if (state != KMOD_MODULE_LIVE && state != KMOD_MODULE_BUILTIN) {
		err = kmod_module_probe_insert_module(mod,
			KMOD_PROBE_APPLY_BLACKLIST, NULL, NULL, NULL, NULL);
		if (err)
			goto mod_unref;

		while (timeout--) {
			state = kmod_module_get_initstate(mod);
			if (state == KMOD_MODULE_LIVE)
				goto mod_unref;

			/* 100 ms */
			usleep(100 * 1000);
		}
		err = -ENOENT;
	}

mod_unref:
	kmod_module_unref(mod);
unref:
	kmod_unref(ctx);

	if (err)
		fprintf(stderr, "Couldn't load the nvme-fabrics module\n");
#endif
}

#ifdef CONFIG_LIBUDEV
#include <libudev.h>
#include <systemd/sd-journal.h>

static int monitor_udev_ctrl(const char *device)
{
	__cleanup_free char *devname = NULL;
	struct udev_monitor *mon;
	struct udev *udev;
	int fd, err;

	udev = udev_new();
	mon = udev_monitor_new_from_netlink(udev, "udev");

	udev_monitor_filter_add_match_subsystem_devtype(mon, "nvme", NULL);
	udev_monitor_enable_receiving(mon);

	err = asprintf(&devname, "/dev/%s", device);
	if (err < 0) {
		err = -errno;
		goto out;
	}

	sd_journal_print(LOG_INFO, "monitoring udev events for %s\n", devname);

	fd = udev_monitor_get_fd(mon);
	while (!nvme_signal_received) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		select(fd + 1, &fds, NULL, NULL, NULL);

		if (FD_ISSET(fd, &fds)) {
			struct udev_device *dev =
				udev_monitor_receive_device(mon);
			const char *devnode = udev_device_get_devnode(dev);
			struct udev_list_entry *entry;

			if (!devnode || strcmp(devname, devnode))
				goto skip;

			udev_list_entry_foreach(entry,
		        udev_device_get_properties_list_entry(dev)) {
				const char *name = udev_list_entry_get_name(entry);
				const char *value = udev_list_entry_get_value(entry);

				sd_journal_print(LOG_INFO, "  %s=%s\n", name, value);
			}
			sd_journal_print(LOG_INFO, "----\n");
skip:
			udev_device_unref(dev);
		}
	}
out:
	udev_unref(udev);
	udev_monitor_unref(mon);

	return err;
}

static void call_fabrics_from_udev(struct udev_device *dev)
{
	const char *kernel = udev_device_get_sysname(dev);

	const char *trtype      = udev_device_get_property_value(dev, "NVME_TRTYPE");
	const char *traddr      = udev_device_get_property_value(dev, "NVME_TRADDR");
	const char *trsvcid     = udev_device_get_property_value(dev, "NVME_TRSVCID");
	const char *host_traddr = udev_device_get_property_value(dev, "NVME_HOST_TRADDR");
	const char *host_iface  = udev_device_get_property_value(dev, "NVME_HOST_IFACE");

	/* fallback like udev rule */
	if (!host_iface || !*host_iface)
		host_iface = "none";

	if (!kernel || !trtype || !traddr || !trsvcid || !host_traddr)
		return;

	char *argv[17];
	int argc = 0;

	argv[argc++] = "autoconnect";

	argv[argc++] = "--device";
	argv[argc++] = (char *)kernel;

	argv[argc++] = "--transport";
	argv[argc++] = (char *)trtype;

	argv[argc++] = "--traddr";
	argv[argc++] = (char *)traddr;

	argv[argc++] = "--trsvcid";
	argv[argc++] = (char *)trsvcid;

	argv[argc++] = "--host-traddr";
	argv[argc++] = (char *)host_traddr;

	argv[argc++] = "--host-iface";
	argv[argc++] = (char *)host_iface;

	argv[argc++] = "--units";

	argv[argc] = NULL;

	sd_journal_print(LOG_INFO, "Calling fabrics_discovery()\n");

	fabrics_discovery("udev-event", argc, argv, true);
}

static void call_fabrics_from_fc(struct udev_device *dev)
{
	const char *traddr      = udev_device_get_property_value(dev, "NVMEFC_TRADDR");
	const char *host_traddr = udev_device_get_property_value(dev, "NVMEFC_HOST_TRADDR");

	if (!traddr || !host_traddr)
		return;

	char *argv[17];
	int argc = 0;

	argv[argc++] = "autoconnect";

	argv[argc++] = "--device";
	argv[argc++] = "none";

	argv[argc++] = "--transport";
	argv[argc++] = "fc";

	argv[argc++] = "--traddr";
	argv[argc++] = (char *)traddr;

	argv[argc++] = "--trsvcid";
	argv[argc++] = "none";

	argv[argc++] = "--host-traddr";
	argv[argc++] = (char *)host_traddr;

	argv[argc++] = "--units";

	argv[argc] = NULL;

	fabrics_discovery("udev-fc-event", argc, argv, true);
}

static int monitor_nvme_udev_events(void)
{
	struct udev_monitor *mon;
	struct udev *udev;
	int fd;

	udev = udev_new();
	if (!udev)
		return -ENOMEM;

	mon = udev_monitor_new_from_netlink(udev, "udev");
	if (!mon)
		goto out;

	/* Match both nvme and fc like your rules */
	udev_monitor_filter_add_match_subsystem_devtype(mon, "nvme", NULL);
	udev_monitor_filter_add_match_subsystem_devtype(mon, "fc", NULL);
	udev_monitor_enable_receiving(mon);

	fd = udev_monitor_get_fd(mon);

	sd_journal_print(LOG_INFO, "Listening for NVMe udev events...\n");

	while (!nvme_signal_received) {
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		if (select(fd + 1, &fds, NULL, NULL, NULL) < 0)
			continue;

		if (!FD_ISSET(fd, &fds))
			continue;

		struct udev_device *dev =
			udev_monitor_receive_device(mon);
		if (!dev)
			continue;

		const char *subsys = udev_device_get_subsystem(dev);
		const char *action = udev_device_get_action(dev);

		/* Skip ACTION!="change" */
		if (!action || strcmp(action, "change"))
			goto skip;

		/* Read relevant environment properties */
		const char *aen        = udev_device_get_property_value(dev, "NVME_AEN");
		const char *event      = udev_device_get_property_value(dev, "NVME_EVENT");
		const char *trtype     = udev_device_get_property_value(dev, "NVME_TRTYPE");
		const char *traddr     = udev_device_get_property_value(dev, "NVME_TRADDR");
		const char *trsvcid    = udev_device_get_property_value(dev, "NVME_TRSVCID");
		const char *host_traddr= udev_device_get_property_value(dev, "NVME_HOST_TRADDR");
		const char *host_iface = udev_device_get_property_value(dev, "NVME_HOST_IFACE");

		const char *fc_event   = udev_device_get_property_value(dev, "FC_EVENT");
		const char *fc_traddr  = udev_device_get_property_value(dev, "NVMEFC_TRADDR");
		const char *fc_host_tr = udev_device_get_property_value(dev, "NVMEFC_HOST_TRADDR");

		/* ---- Rule 1: NVME_AEN == 0x70f002 ---- */
		if (subsys && !strcmp(subsys, "nvme") &&
		    aen && !strcmp(aen, "0x70f002") &&
		    trtype && traddr && trsvcid && host_traddr && host_iface) {

			sd_journal_print(LOG_INFO,
				"[MATCH] NVME_AEN discovery change event\n");
			    call_fabrics_from_udev(dev);
			    goto print;
		}

		/* ---- Rule 2: FC discovery (old-style) ---- */
		if (subsys && !strcmp(subsys, "fc") &&
		    fc_event && !strcmp(fc_event, "nvmediscovery") &&
		    fc_traddr && fc_host_tr) {

			sd_journal_print(LOG_INFO,
				"[MATCH] FC discovery event\n");
			call_fabrics_from_fc(dev);
			goto print;
		}

		/* ---- Rule 3: rediscover ---- */
		if (subsys && !strcmp(subsys, "nvme") &&
		    event && !strcmp(event, "rediscover") &&
		    trtype && traddr && trsvcid && host_traddr && host_iface) {

			sd_journal_print(LOG_INFO,
				"[MATCH] NVME rediscover event\n");
			    call_fabrics_from_udev(dev);
			    goto print;
		}

		goto skip;

print:
	{
		struct udev_list_entry *entry;

		udev_list_entry_foreach(entry,
			udev_device_get_properties_list_entry(dev)) {
			const char *name  = udev_list_entry_get_name(entry);
			const char *value = udev_list_entry_get_value(entry);

			sd_journal_print(LOG_INFO, "  %s=%s\n", name, value);
		}
		sd_journal_print(LOG_INFO, "----\n");
	}

skip:
		udev_device_unref(dev);
	}

	udev_monitor_unref(mon);
out:
	udev_unref(udev);
	return 0;
}
#else
static int monitor_udev_ctrl(const char *devname)
{
	while (!nvme_signal_received)
		pause();

	return 0;
}
static int monitor_nvme_udev_events(void)
{
}
#endif

#define NBFT_SYSFS_PATH		"/sys/firmware/acpi/tables"

int fabrics_discovery(const char *desc, int argc, char **argv, bool connect)
{
	char *config_file = PATH_NVMF_CONFIG;
	char *context = NULL;
	nvme_print_flags_t flags;
	__cleanup_nvme_global_ctx struct libnvme_global_ctx *ctx = NULL;
	__cleanup_nvmf_context struct libnvmf_context *fctx = NULL;
	int ret;
	struct nvmf_args fa = { .subsysnqn = NVME_DISC_SUBSYS_NAME };
	char *device = NULL;
	bool force = false;
	bool json_config = false;
	bool nbft = false, nonbft = false;
	bool units = false;
	bool monitor = false;
	char *nbft_path = NBFT_SYSFS_PATH;

	NVMF_ARGS(opts, fa,
		  OPT_STRING("device",     'd', "DEV", &device,       "use existing discovery controller device"),
		  OPT_FILE("raw",          'r', &raw,                 "save raw output to file"),
		  OPT_FLAG("persistent",   'p', &persistent,          "persistent discovery connection"),
		  OPT_FLAG("quiet",          0, &quiet,               "suppress already connected errors"),
		  OPT_STRING("config",     'J', "FILE", &config_file, nvmf_config_file),
		  OPT_FLAG("dump-config",  'O', &dump_config,         "Dump configuration file to stdout"),
		  OPT_FLAG("force",          0, &force,               "Force persistent discovery controller creation"),
		  OPT_FLAG("nbft",           0, &nbft,                "Only look at NBFT tables"),
		  OPT_FLAG("no-nbft",        0, &nonbft,              "Do not look at NBFT tables"),
		  OPT_STRING("nbft-path",    0, "STR", &nbft_path,    "user-defined path for NBFT tables"),
		  OPT_STRING("context",      0, "STR", &context,       nvmf_context),
		  OPT_FLAG("units",          0, &units,               "create systemd unit files"),
		  OPT_FLAG("monitor",        0, &monitor,             "monitor udev"));

	nvmf_default_args(&fa);

	load_nvme_fabrics_module();

	ret = argconfig_parse(argc, argv, desc, opts);
	if (ret)
		return ret;

	ret = validate_output_format(nvme_args.output_format, &flags);
	if (ret < 0) {
		nvme_show_error("Invalid output format");
		return ret;
		}

	if (monitor) {
		monitor_nvme_udev_events();
		return 0;
	}

	if (!strcmp(config_file, "none"))
		config_file = NULL;

	log_level = map_log_level(nvme_args.verbose, quiet);

	ctx = libnvme_create_global_ctx(stderr, log_level);
	if (!ctx) {
		fprintf(stderr, "Failed to create topology root: %s\n",
			libnvme_strerror(errno));
		return -ENOMEM;
	}
	if (context)
		libnvme_set_application(ctx, context);

	if (!nvme_read_config_checked(ctx, config_file))
		json_config = true;
	if (!nvme_read_volatile_config(ctx))
		json_config = true;

	libnvme_skip_namespaces(ctx);
	ret = libnvme_scan_topology(ctx, NULL, NULL);
	if (ret < 0) {
		fprintf(stderr, "Failed to scan topology: %s\n",
			libnvme_strerror(-ret));
		return ret;
	}

	if (device) {
		if (!strcmp(device, "none"))
			device = NULL;
		else if (!strncmp(device, "/dev/", 5))
			device += 5;
	}

	struct cb_fabrics_data dld = {
		.fa = &fa,
		.flags = flags,
		.units = units,
		.raw = raw,
	};
	ret = create_discovery_context(ctx, persistent, device, &fa,
		&dld, &fctx);
	if (ret)
		return ret;

	if (!device && !fa.transport && !fa.traddr) {
		if (!nonbft)
			ret = libnvmf_discovery_nbft(ctx, fctx,
				connect, nbft_path);
		if (nbft)
			goto out_free;

		if (json_config)
			ret = libnvmf_discovery_config_json(ctx, fctx,
				connect, force);
		if (ret || access(PATH_NVMF_DISC, F_OK))
			goto out_free;

		ret = libnvmf_discovery_config_file(ctx, fctx, connect, force);
		goto out_free;
	}

	ret = libnvmf_discovery(ctx, fctx, connect, force);

out_free:
	if (dump_config)
		libnvme_dump_config(ctx, STDOUT_FILENO);

	return ret;
}

int fabrics_connect(const char *desc, int argc, char **argv)
{
	__cleanup_free char *hnqn = NULL;
	__cleanup_free char *hid = NULL;
	char *config_file = NULL;
	char *context = NULL;
	bool disconnect_on_sigint = false;
	__cleanup_nvme_global_ctx struct libnvme_global_ctx *ctx = NULL;
	__cleanup_nvmf_context struct libnvmf_context *fctx = NULL;
	__cleanup_nvme_ctrl libnvme_ctrl_t c = NULL;
	int ret;
	nvme_print_flags_t flags;
	struct nvmf_args fa = { 0 };

	NVMF_ARGS(opts, fa,
		  OPT_STRING("config",             'J', "FILE", &config_file, nvmf_config_file),
		  OPT_FLAG("dump-config",          'O', &dump_config, "Dump JSON configuration to stdout"),
		  OPT_STRING("context",              0, "STR", &context,  nvmf_context),
		  OPT_FLAG("disconnect-on-sigint",   0, &disconnect_on_sigint, "Wait for Ctrl+C and disconnect the controller"));

	nvmf_default_args(&fa);

	load_nvme_fabrics_module();

	ret = argconfig_parse(argc, argv, desc, opts);
	if (ret)
		return ret;

	ret = validate_output_format(nvme_args.output_format, &flags);
	if (ret < 0) {
		nvme_show_error("Invalid output format");
		return ret;
	}

	if (config_file && strcmp(config_file, "none"))
		goto do_connect;

	if (!fa.subsysnqn) {
		fprintf(stderr,
			"required argument [--nqn | -n] not specified\n");
		return -EINVAL;
	}

	if (!fa.transport) {
		fprintf(stderr,
			"required argument [--transport | -t] not specified\n");
		return -EINVAL;
	}

	if (strcmp(fa.transport, "loop")) {
		if (!fa.traddr) {
			fprintf(stderr,
				"required argument [--traddr | -a] not specified for transport %s\n",
				fa.transport);
			return -EINVAL;
		}
	}

do_connect:
	log_level = map_log_level(nvme_args.verbose, quiet);

	ctx = libnvme_create_global_ctx(stderr, log_level);
	if (!ctx) {
		fprintf(stderr, "Failed to create topology root: %s\n",
			libnvme_strerror(errno));
		return -ENOMEM;
	}
	if (context)
		libnvme_set_application(ctx, context);

	libnvme_read_config(ctx, config_file);
	nvme_read_volatile_config(ctx);

	libnvme_skip_namespaces(ctx);
	ret = libnvme_scan_topology(ctx, NULL, NULL);
	if (ret < 0) {
		fprintf(stderr, "Failed to scan topology: %s\n",
			libnvme_strerror(-ret));
		return ret;
	}

	struct cb_fabrics_data cfd = {
		.flags = flags,
		.quiet = dump_config,
		.raw = raw,
	};
	ret = create_common_context(ctx, persistent, &fa, &cfd, &fctx);
	if (ret)
		return ret;

	if (config_file)
		return libnvmf_connect_config_json(ctx, fctx);

retry:
	ret = libnvmf_connect(ctx, fctx);
	switch (-ret) {
	case 0:
	case EALREADY:
	case ENVME_CONNECT_ALREADY:
		break;
	case ENVME_CONNECT_TIMEOUT:
		fprintf(stdout, "connection attempt failed with timeout. retry in 10s.\n");
		fflush(stdout);
		if (sleep(10))
			return 0;
		goto retry;
		break;
	default:
		fprintf(stderr, "failed to connect: %s (%d)\n",
			libnvme_strerror(-ret), ret);
		return ret;
	}

	if (dump_config)
		libnvme_dump_config(ctx, STDOUT_FILENO);

	if (cfd.ctrl && disconnect_on_sigint) {
		fflush(stdout);
		monitor_udev_ctrl(libnvme_ctrl_get_name(cfd.ctrl));
		libnvmf_disconnect_ctrl(cfd.ctrl);
	}

	return 0;
}

static libnvme_ctrl_t lookup_nvme_ctrl(struct libnvme_global_ctx *ctx,
				    const char *name)
{
	libnvme_host_t h;
	libnvme_subsystem_t s;
	libnvme_ctrl_t c;

	libnvme_for_each_host(ctx, h) {
		libnvme_for_each_subsystem(h, s) {
			libnvme_subsystem_for_each_ctrl(s, c) {
				if (!strcmp(libnvme_ctrl_get_name(c), name))
					return c;
			}
		}
	}
	return NULL;
}

static void nvmf_disconnect_nqn(struct libnvme_global_ctx *ctx, char *nqn)
{
	int i = 0;
	char *n = nqn;
	char *p;
	libnvme_host_t h;
	libnvme_subsystem_t s;
	libnvme_ctrl_t c;

	while ((p = strsep(&n, ",")) != NULL) {
		if (!strlen(p))
			continue;
		libnvme_for_each_host(ctx, h) {
			libnvme_for_each_subsystem(h, s) {
				if (strcmp(libnvme_subsystem_get_subsysnqn(s), p))
					continue;
				libnvme_subsystem_for_each_ctrl(s, c) {
					if (!libnvmf_disconnect_ctrl(c))
						i++;
				}
			}
		}
	}
	printf("NQN:%s disconnected %d controller(s)\n", nqn, i);
}

int fabrics_disconnect(const char *desc, int argc, char **argv)
{
	const char *desc_device = "nvme device handle";
	__cleanup_nvme_global_ctx struct libnvme_global_ctx *ctx = NULL;
	__cleanup_free struct libnvmf_context *fctx = NULL;
	struct nvmf_args fa = { 0 };
	char *device = NULL;
	char *nqn = NULL;
	libnvme_ctrl_t c;
	char *p;
	int ret;

	NVMF_ARGS(opts, fa,
		OPT_STRING("nqn",        'n', "NAME", &nqn,    nvmf_nqn),
		OPT_STRING("device",     'd', "DEV",  &device, desc_device));

	ret = argconfig_parse(argc, argv, desc, opts);
	if (ret)
		return ret;

	if (nqn && device) {
		fprintf(stderr,
			"Both device name [--device | -d] and NQN [--nqn | -n] are specified\n");
		return -EINVAL;
	}

	log_level = map_log_level(nvme_args.verbose, false);

	ctx = libnvme_create_global_ctx(stderr, log_level);
	if (!ctx) {
		fprintf(stderr, "Failed to create topology root: %s\n",
			libnvme_strerror(errno));
		return -ENOMEM;
	}
	libnvme_skip_namespaces(ctx);
	ret = libnvme_scan_topology(ctx, NULL, NULL);
	if (ret < 0) {
		/*
		 * Do not report an error when the modules are not
		 * loaded, this allows the user to unconditionally call
		 * disconnect.
		 */
		if (ret == -ENOENT)
			return 0;

		fprintf(stderr, "Failed to scan topology: %s\n",
			libnvme_strerror(-ret));
		return ret;
	}

	if (!device) {
		ret = create_common_context(ctx, persistent, &fa,
			NULL, &fctx);
		if (ret)
			return ret;

		ret = libnvmf_disconnect(ctx, fctx);
		if (ret)
			fprintf(stderr, "Disconnect failed: %s\n",
				libnvme_strerror(-ret));
		return ret;
	}

	if (nqn)
		nvmf_disconnect_nqn(ctx, nqn);

	if (device) {
		char *d;

		d = device;
		while ((p = strsep(&d, ",")) != NULL) {
			if (!strncmp(p, "/dev/", 5))
				p += 5;
			c = lookup_nvme_ctrl(ctx, p);
			if (!c) {
				fprintf(stderr,
					"Did not find device %s\n", p);
				return -ENODEV;
			}
			ret = libnvmf_disconnect_ctrl(c);
			if (ret)
				fprintf(stderr,
					"Failed to disconnect %s: %s\n",
					p, libnvme_strerror(-ret));
		}
	}

	return 0;
}

int fabrics_disconnect_all(const char *desc, int argc, char **argv)
{
	__cleanup_nvme_global_ctx struct libnvme_global_ctx *ctx = NULL;
	libnvme_host_t h;
	libnvme_subsystem_t s;
	libnvme_ctrl_t c;
	int ret;

	struct config {
		char *transport;
	};

	struct config cfg = { 0 };

	NVME_ARGS(opts,
		OPT_STRING("transport", 'r', "STR", (char *)&cfg.transport, nvmf_tport));

	ret = argconfig_parse(argc, argv, desc, opts);
	if (ret)
		return ret;

	log_level = map_log_level(nvme_args.verbose, false);

	ctx = libnvme_create_global_ctx(stderr, log_level);
	if (!ctx) {
		fprintf(stderr, "Failed to create topology root: %s\n",
			libnvme_strerror(errno));
		return -ENOMEM;
	}
	libnvme_skip_namespaces(ctx);
	ret = libnvme_scan_topology(ctx, NULL, NULL);
	if (ret < 0) {
		/*
		 * Do not report an error when the modules are not
		 * loaded, this allows the user to unconditionally call
		 * disconnect.
		 */
		if (ret == -ENOENT)
			return 0;

		fprintf(stderr, "Failed to scan topology: %s\n",
			libnvme_strerror(-ret));
		return ret;
	}

	libnvme_for_each_host(ctx, h) {
		libnvme_for_each_subsystem(h, s) {
			libnvme_subsystem_for_each_ctrl(s, c) {
				if (cfg.transport &&
				    strcmp(cfg.transport,
					   libnvme_ctrl_get_transport(c)))
					continue;
				else if (!strcmp(libnvme_ctrl_get_transport(c),
						 "pcie"))
					continue;
				if (libnvmf_disconnect_ctrl(c))
					fprintf(stderr,
						"failed to disconnect %s\n",
						libnvme_ctrl_get_name(c));
			}
		}
	}

	return 0;
}

int fabrics_config(const char *desc, int argc, char **argv)
{
	bool scan_tree = false, modify_config = false, update_config = false;
	__cleanup_nvme_global_ctx struct libnvme_global_ctx *ctx = NULL;
	char *config_file = PATH_NVMF_CONFIG;
	struct nvmf_args fa = { };
	int ret;

	NVMF_ARGS(opts, fa,
		  OPT_STRING("config",             'J', "FILE", &config_file, nvmf_config_file),
		  OPT_FLAG("scan",                 'R', &scan_tree,           "Scan current NVMeoF topology"),
		  OPT_FLAG("modify",               'M', &modify_config,       "Modify JSON configuration file"),
		  OPT_FLAG("dump",                 'O', &dump_config,         "Dump JSON configuration to stdout"),
		  OPT_FLAG("update",               'U', &update_config,       "Update JSON configuration file"));

	nvmf_default_args(&fa);

	ret = argconfig_parse(argc, argv, desc, opts);
	if (ret)
		return ret;

	if (!strcmp(config_file, "none"))
		config_file = NULL;

	log_level = map_log_level(nvme_args.verbose, quiet);

	ctx = libnvme_create_global_ctx(stderr, log_level);
	if (!ctx) {
		fprintf(stderr, "Failed to create topology root: %s\n",
			libnvme_strerror(errno));
		return -ENOMEM;
	}

	libnvme_read_config(ctx, config_file);

	if (scan_tree) {
		libnvme_skip_namespaces(ctx);
		ret = libnvme_scan_topology(ctx, NULL, NULL);
		if (ret < 0) {
			fprintf(stderr, "Failed to scan topology: %s\n",
				libnvme_strerror(-ret));
			return -ret;
		}
	}

	if (modify_config) {
		__cleanup_nvmf_context struct libnvmf_context *fctx = NULL;

		if (!fa.subsysnqn) {
			fprintf(stderr,
				"required argument [--nqn | -n] needed with --modify\n");
			return -EINVAL;
		}

		if (!fa.transport) {
			fprintf(stderr,
				"required argument [--transport | -t] needed with --modify\n");
			return -EINVAL;
		}

		ret = create_common_context(ctx, persistent, &fa, NULL, &fctx);
		if (ret)
			return ret;

		ret = libnvmf_config_modify(ctx, fctx);
		if (ret) {
			fprintf(stderr, "failed to update config\n");
			return ret;
		}
	}

	if (update_config) {
		__cleanup_fd int fd = -1;

		fd = open(config_file, O_RDONLY, 0);
		if (fd != -1)
			libnvme_dump_config(ctx, fd);
	}

	if (dump_config)
		libnvme_dump_config(ctx, STDOUT_FILENO);

	return 0;
}

static int dim_operation(libnvme_ctrl_t c, enum nvmf_dim_tas tas, const char *name)
{
	static const char * const task[] = {
		[NVMF_DIM_TAS_REGISTER]   = "register",
		[NVMF_DIM_TAS_DEREGISTER] = "deregister",
	};
	const char *t;
	int status;
	__u32 result;

	t = (tas > NVMF_DIM_TAS_DEREGISTER || !task[tas]) ? "reserved" : task[tas];
	status = libnvmf_register_ctrl(c, tas, &result);
	if (status == NVME_SC_SUCCESS) {
		printf("%s DIM %s command success\n", name, t);
	} else if (status < NVME_SC_SUCCESS) {
		fprintf(stderr, "%s DIM %s command error. Status:0x%04x - %s\n",
			name, t, status, libnvme_status_to_string(status, false));
	} else {
		fprintf(stderr, "%s DIM %s command error. Result:0x%04x, Status:0x%04x - %s\n",
			name, t, result, status, libnvme_status_to_string(status, false));
	}

	return libnvme_status_to_errno(status, true);
}

int fabrics_dim(const char *desc, int argc, char **argv)
{
	__cleanup_nvme_global_ctx struct libnvme_global_ctx *ctx = NULL;
	enum nvmf_dim_tas tas;
	libnvme_ctrl_t c;
	char *p;
	int ret;

	struct {
		char *nqn;
		char *device;
		char *tas;
	} cfg = { 0 };

	NVME_ARGS(opts,
		OPT_STRING("nqn",    'n', "NAME", &cfg.nqn,    "Comma-separated list of DC nqn"),
		OPT_STRING("device", 'd', "DEV",  &cfg.device, "Comma-separated list of DC nvme device handle."),
		OPT_STRING("task",   't', "TASK", &cfg.tas,    "[register|deregister]"));

	ret = argconfig_parse(argc, argv, desc, opts);
	if (ret)
		return ret;

	if (!cfg.nqn && !cfg.device) {
		fprintf(stderr,
			"Neither device name [--device | -d] nor NQN [--nqn | -n] provided\n");
		return -EINVAL;
	}

	if (!cfg.tas) {
		fprintf(stderr,
			"Task [--task | -t] must be specified\n");
		return -EINVAL;
	}

	/* Allow partial name (e.g. "reg" for "register" */
	if (strstarts("register", cfg.tas)) {
		tas = NVMF_DIM_TAS_REGISTER;
	} else if (strstarts("deregister", cfg.tas)) {
		tas = NVMF_DIM_TAS_DEREGISTER;
	} else {
		fprintf(stderr, "Invalid --task: %s\n", cfg.tas);
		return -EINVAL;
	}

	log_level = map_log_level(nvme_args.verbose, false);

	ctx = libnvme_create_global_ctx(stderr, log_level);
	if (!ctx) {
		fprintf(stderr, "Failed to create topology root: %s\n",
			libnvme_strerror(errno));
		return -ENODEV;
	}
	libnvme_skip_namespaces(ctx);
	ret = libnvme_scan_topology(ctx, NULL, NULL);
	if (ret < 0) {
		fprintf(stderr, "Failed to scan topology: %s\n",
			libnvme_strerror(-ret));
		return ret;
	}

	if (cfg.nqn) {
		libnvme_host_t h;
		libnvme_subsystem_t s;
		char *n = cfg.nqn;

		while ((p = strsep(&n, ",")) != NULL) {
			if (!strlen(p))
				continue;
			libnvme_for_each_host(ctx, h) {
				libnvme_for_each_subsystem(h, s) {
					if (strcmp(libnvme_subsystem_get_subsysnqn(s), p))
						continue;
					libnvme_subsystem_for_each_ctrl(s, c)
						ret = dim_operation(c, tas, p);
				}
			}
		}
	}

	if (cfg.device) {
		char *d = cfg.device;

		while ((p = strsep(&d, ",")) != NULL) {
			if (!strncmp(p, "/dev/", 5))
				p += 5;
			ret = libnvme_scan_ctrl(ctx, p, &c);
			if (ret) {
				fprintf(stderr,
					"Did not find device %s: %s\n",
					p, libnvme_strerror(ret));
				return ret;
			}
			ret = dim_operation(c, tas, p);
		}
	}

	return ret;
}
