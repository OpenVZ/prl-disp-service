/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef _PrlLibvzctlWrap_H__
#define _PrlLibvzctlWrap_H__

#define _USE_DLOPEN_

#include <vzctl/libvzctl.h>
#include <vzctl/vzctl_param.h>

#define FUNC_DECL(res, name, args) \
	typedef res (*wrap##name) args;

FUNC_DECL(int, vzctl2_get_env_status, (unsigned int veid, vzctl_env_status_t *status, int mask))
FUNC_DECL(vzctl_ids_t *, vzctl2_alloc_env_ids, (void))
FUNC_DECL(void, vzctl2_free_env_ids, (vzctl_ids_t *ids))
FUNC_DECL(int, vzctl2_get_env_ids_by_state, (vzctl_ids_t *veids, unsigned int mask))
FUNC_DECL(vzctl_env_handle_ptr, vzctl2_env_open, (unsigned int id, int flags, int *err))
FUNC_DECL(vzctl_env_handle_ptr, vzctl2_env_open_conf, (unsigned int id, const char *fname, int flags, int *err))
FUNC_DECL(void, vzctl2_env_close, (vzctl_env_handle_ptr h))
FUNC_DECL(int, vzctl2_apply_param, (vzctl_env_handle_ptr h, vzctl_env_param_ptr param, int flags))
FUNC_DECL(int, vzctl2_env_save, (vzctl_env_handle_ptr h))
FUNC_DECL(int, vzctl2_env_get_name, (vzctl_env_handle_ptr h, const char **name))
FUNC_DECL(int, vzctl2_get_name,(vzctl_env_handle_ptr h, const char **name))
FUNC_DECL(int, vzctl2_set_name,(vzctl_env_handle_ptr h, const char *name))
FUNC_DECL(vzctl_env_param_ptr, vzctl2_alloc_env_param,(unsigned int id))
FUNC_DECL(void, vzctl2_free_env_param,(vzctl_env_param_ptr env))
FUNC_DECL(vzctl_env_param_ptr, vzctl2_get_env_param,(vzctl_env_handle_ptr h))
FUNC_DECL(int, vzctl2_env_set_ub_resource,(vzctl_env_param_ptr env, int id, struct vzctl_2UL_res *res))
FUNC_DECL(int, vzctl2_env_get_ub_resource,(vzctl_env_param_ptr env, int id, struct vzctl_2UL_res *res))
FUNC_DECL(int, vzctl2_env_set_ramsize,(vzctl_env_param_ptr env, unsigned long ramsize))
FUNC_DECL(int, vzctl2_env_get_ramsize,(vzctl_env_param_ptr env, unsigned long *ramsize))
FUNC_DECL(int, vzctl2_env_set_diskspace,(vzctl_env_param_ptr env, struct vzctl_2UL_res *res))
FUNC_DECL(int, vzctl2_env_get_diskspace,(vzctl_env_param_ptr env, struct vzctl_2UL_res *res))
FUNC_DECL(int, vzctl2_env_set_diskinodes,(vzctl_env_param_ptr env, struct vzctl_2UL_res *res))
FUNC_DECL(int, vzctl2_env_get_diskinodes,(vzctl_env_param_ptr env, struct vzctl_2UL_res *res))
FUNC_DECL(int, vzctl2_env_set_quotaugidlimit,(vzctl_env_param_ptr env, unsigned long limit))
FUNC_DECL(int, vzctl2_env_get_quotaugidlimit,(vzctl_env_param_ptr env, unsigned long *limit))
FUNC_DECL(int, vzctl2_env_set_cpulimit,(vzctl_env_param_ptr env, struct vzctl_cpulimit_param *param))
FUNC_DECL(int, vzctl2_env_get_cpulimit,(vzctl_env_param_ptr env, struct vzctl_cpulimit_param *param))
FUNC_DECL(int, vzctl2_env_set_cpu_count, (vzctl_env_param_ptr env, unsigned long num))
FUNC_DECL(int, vzctl2_env_get_cpu_count, (vzctl_env_param_ptr env, unsigned long *num))
FUNC_DECL(int, vzctl2_env_set_cpuunits,(vzctl_env_param_ptr env, unsigned long units))
FUNC_DECL(int, vzctl2_env_get_cpuunits,(vzctl_env_param_ptr env, unsigned long *units))
FUNC_DECL(int, vzctl2_env_set_iolimit,(vzctl_env_param_ptr env, int limit))
FUNC_DECL(int, vzctl2_env_get_iolimit,(vzctl_env_param_ptr env, int *limit))
FUNC_DECL(int, vzctl2_env_set_ioprio,(vzctl_env_param_ptr env, int prio))
FUNC_DECL(int, vzctl2_env_get_ioprio,(vzctl_env_param_ptr env, int *prio))
FUNC_DECL(int, vzctl2_env_get_description,(vzctl_env_param_ptr env, const char **name))
FUNC_DECL(int, vzctl2_env_set_description,(vzctl_env_param_ptr env, const char *name))
FUNC_DECL(int, vzctl2_env_get_hostname,(vzctl_env_param_ptr env, const char **name))
FUNC_DECL(int, vzctl2_env_set_hostname,(vzctl_env_param_ptr env, const char *name))
FUNC_DECL(int, vzctl2_env_get_ve_private_path, (vzctl_env_param_ptr env, const char **path))
FUNC_DECL(int, vzctl2_env_get_ve_root_path, (vzctl_env_param_ptr env, const char **path))
FUNC_DECL(int, vzctl2_env_get_ostemplate, (vzctl_env_param_ptr env, const char **name))
FUNC_DECL(int, vzctl2_env_get_apptemplates, (vzctl_env_param_ptr env, const char **names))
FUNC_DECL(int, vzctl2_env_set_autostart, (struct vzctl_env_param *env, int enable))
FUNC_DECL(int, vzctl2_env_get_autostart, (struct vzctl_env_param *env, int *enable))
FUNC_DECL(int, vzctl2_env_get_bootorder, (struct vzctl_env_param *env, unsigned long *))
FUNC_DECL(int, vzctl2_env_get_ipstr,(vzctl_ip_iterator it, char *buf, int len))
FUNC_DECL(int, vzctl2_env_add_ipaddress,(vzctl_env_param_ptr env, const char *ipstr))
FUNC_DECL(int, vzctl2_env_del_ipaddress,(struct vzctl_env_param *env, const char *ipstr))
FUNC_DECL(vzctl_ip_iterator, vzctl2_env_get_ipaddress,(vzctl_env_param_ptr env, vzctl_ip_iterator it))
FUNC_DECL(int, vzctl2_env_get_veth_param, (vzctl_veth_dev_iterator itdev, struct vzctl_veth_dev_param *dev, int size))
FUNC_DECL(vzctl_veth_dev_iterator, vzctl2_create_veth_dev,(struct vzctl_veth_dev_param *dev, int size))
FUNC_DECL(void, vzctl2_free_veth_dev, (vzctl_veth_dev_iterator itdev))
FUNC_DECL(int, vzctl2_env_add_veth_ipaddress,(vzctl_veth_dev_iterator dev, const char *ipstr))
FUNC_DECL(int, vzctl2_env_del_veth_ipaddress,(vzctl_veth_dev_iterator dev, const char *ipstr))
FUNC_DECL(vzctl_ip_iterator, vzctl2_env_get_veth_ipaddress,(vzctl_veth_dev_iterator itdev, vzctl_ip_iterator itip))
FUNC_DECL(int, vzctl2_env_add_veth,(vzctl_env_param_ptr env, vzctl_veth_dev_iterator itdev))
FUNC_DECL(int, vzctl2_env_del_veth,(vzctl_env_param_ptr env, const char *ifname))
FUNC_DECL(vzctl_veth_dev_iterator, vzctl2_env_get_veth,(vzctl_env_param_ptr env, vzctl_veth_dev_iterator it))
FUNC_DECL(int, vzctl2_env_get_rate_param,(vzctl_rate_iterator itrate, struct vzctl_rate_param *rate))
FUNC_DECL(vzctl_rate_iterator, vzctl2_create_rate,(struct vzctl_rate_param *rate))
FUNC_DECL(int, vzctl2_env_add_rate,(vzctl_env_param_ptr env, vzctl_rate_iterator itrate))
FUNC_DECL(vzctl_rate_iterator, vzctl2_env_get_rate,(vzctl_env_param_ptr env, vzctl_rate_iterator it))
FUNC_DECL(int, vzctl2_env_set_ratebound,(vzctl_env_param_ptr env, int ratebound))
FUNC_DECL(int, vzctl2_env_get_ratebound,(vzctl_env_param_ptr env, int *ratebound))
FUNC_DECL(const char *, vzctl2_env_get_str_param,(vzctl_str_iterator it))
FUNC_DECL(int, vzctl2_env_add_nameserver,(vzctl_env_param_ptr env, const char *server))
FUNC_DECL(vzctl_str_iterator, vzctl2_env_get_nameserver,(vzctl_env_param_ptr env, vzctl_str_iterator it))
FUNC_DECL(int, vzctl2_env_add_searchdomain,(vzctl_env_param_ptr env, const char *domain))
FUNC_DECL(vzctl_str_iterator, vzctl2_env_get_searchdomain,(vzctl_env_param_ptr env, vzctl_str_iterator it))
FUNC_DECL(int, vzctl2_set_userpasswd, (unsigned int id, const char *user, const char *passwd))
FUNC_DECL(int, vzctl2_env_auth, (struct vzctl_env_handle *h, const char *user, const char *passwd,
			int gid, int flags))
FUNC_DECL(const char *, vzctl2_get_last_error,(void))
FUNC_DECL(int, vzctl2_init_log, (const char *progname))
FUNC_DECL(void, vzctl2_set_log_quiet, (int))
FUNC_DECL(int, vzctl2_lib_init,(void))
FUNC_DECL(int, vzctl2_env_cpustat, (unsigned veid, struct vzctl_cpustat *cpustat, int size))
FUNC_DECL(int, vzctl2_get_env_iostat, (unsigned int id, struct vzctl_iostat *stat, int size))
FUNC_DECL(int, vzctl2_set_ioprio, (unsigned int id, int prio))
FUNC_DECL(int, vzctl2_set_iolimit, (unsigned int id, int limit))
FUNC_DECL(int, vzctl2_register_evt, (vzevt_handle_t **h))
FUNC_DECL(int, vzctl2_unregister_evt, (vzevt_handle_t *h))
FUNC_DECL(int, vzctl2_get_state_evt, (vzevt_handle_t *h, struct vzctl_state_evt *evt, int size))
FUNC_DECL(int, vzctl2_send_state_evt, (unsigned int veid, int state))
FUNC_DECL(int, vzctl2_get_evt_fd, (vzevt_handle_t *h))
FUNC_DECL(int, vzctl2_env_reset_uptime, (vzctl_env_handle_ptr h))
FUNC_DECL(int, vzctl2_env_get_uptime, (vzctl_env_handle_ptr h,
		unsigned long long *uptime, unsigned long long *start_time))
FUNC_DECL(int, vzctl2_env_set_uptime, (vzctl_env_handle_ptr h,
		unsigned long long uptime, unsigned long long start_time))
FUNC_DECL(int, vzctl2_env_sync_uptime, (vzctl_env_handle_ptr h))
FUNC_DECL(int, vzctl2_get_free_envid, (unsigned int *newid, const char *dst, const char *))
FUNC_DECL(void, vzctl2_unlock_envid, (unsigned int id))
FUNC_DECL(int, vzctl2_env_register, (const char *path, unsigned veid, int flags))
FUNC_DECL(int, vzctl2_env_unregister, (const char *path, unsigned veid, int flags))
FUNC_DECL(cleanup_handler_t *, vzctl2_get_cleanup_handler, ())
FUNC_DECL(void, vzctl2_cancel_last_operation, (cleanup_handler_t *h))
FUNC_DECL(int, vzctl2_env_set_type, (struct vzctl_env_param *env, vzctl_env_type type))
FUNC_DECL(int, vzctl2_env_get_type, (struct vzctl_env_param *env, vzctl_env_type *type))
FUNC_DECL(int, vzctl2_get_env_conf_path, (unsigned veid, char *buf, int len))
FUNC_DECL(int, vzctl2_env_get_uuid, (struct vzctl_env_param *env, const char **uuid))
FUNC_DECL(int, vzctl2_env_set_uuid, (struct vzctl_env_param *env, const char *uuid))
FUNC_DECL(int, vzctl2_env_get_param, (vzctl_env_handle_ptr h, const char *name, const char **res))
FUNC_DECL(int, vzctl2_env_exec_async, (vzctl_env_handle_ptr h, int exec_mode,
        char *const argv[], char *const envp[], char *std_in, int timeout, int stdfd[3], int *err))
FUNC_DECL(int, vzctl2_env_exec_wait, (int pid, int *retcode))
FUNC_DECL(int, vzctl2_env_lock, (unsigned int id, const char *status))
FUNC_DECL(void, vzctl2_env_unlock, (unsigned int id, int lckfd))
FUNC_DECL(int, vzctl2_set_iopslimit, (unsigned int id, int limit))
FUNC_DECL(int, vzctl2_env_set_iopslimit, (struct vzctl_env_param *env, int limit))
FUNC_DECL(int, vzctl2_env_get_iopslimit, (struct vzctl_env_param *env, int *limit))
FUNC_DECL(int, vzctl2_env_get_layout, (struct vzctl_env_param *env, int *layout))
FUNC_DECL(int, vzctl2_env_set_layout, (struct vzctl_env_param *env, int layout, int flags))
FUNC_DECL(int, vzctl2_env_create_temporary_snapshot, (vzctl_env_handle_ptr h, const char *guid,
		struct vzctl_tsnapshot_param *tsnap, struct vzctl_snap_holder *holder))
FUNC_DECL(int, vzctl2_env_delete_tsnapshot, (vzctl_env_handle_ptr h, const char *guid,
		struct vzctl_snap_holder *holder))
FUNC_DECL(int, vzctl2_release_snap_holder, (struct vzctl_snap_holder *holder))
FUNC_DECL(int, vzctl2_mount_disk_snapshot, (const char *path, struct vzctl_mount_param *param))
FUNC_DECL(int, vzctl2_mount_snapshot, (vzctl_env_handle_ptr h, struct vzctl_mount_param *param))
FUNC_DECL(int, vzctl2_umount_snapshot, (vzctl_env_handle_ptr h, const char *guid, const char *component_name))
FUNC_DECL(int, vzctl2_umount_disk_snapshot, (const char *path, const char *guid, const char *component_name))
FUNC_DECL(int, vzctl2_merge_snapshot, (vzctl_env_handle_ptr h, const char *guid))
FUNC_DECL(int, vzctl2_umount_image_by_dev, (const char *dev))
FUNC_DECL(int, vzctl2_create_image, (const char *ve_private, struct vzctl_create_image_param *param))
FUNC_DECL(int, vzctl2_mount_image, (const char *ve_private, struct vzctl_mount_param *param))
FUNC_DECL(int, vzctl2_create_disk_image, (const char *path, struct vzctl_create_image_param *param))
FUNC_DECL(int, vzctl2_mount_disk_image, (const char *path, struct vzctl_mount_param *param))
FUNC_DECL(int, vzctl2_create_env_private, (const char *ve_private, int layout))
FUNC_DECL(int, vzctl2_set_vzlimits, (const char *name))
FUNC_DECL(int, vzctl2_env_set_cpumask, (struct vzctl_env_param *env, const char *str))
FUNC_DECL(int, vzctl2_env_get_cpumask, (struct vzctl_env_param *env, char *buf, int buflen))
FUNC_DECL(int, vzctl2_env_set_apply_iponly, (struct vzctl_env_param *env, int enable))
FUNC_DECL(int, vzctl2_env_get_apply_iponly, (struct vzctl_env_param *env, int *enable))
FUNC_DECL(int, vzctl2_get_env_meminfo, (unsigned id, struct vzctl_meminfo *out, int size))
FUNC_DECL(int, vzctl2_env_set_cap, (vzctl_env_handle_ptr h, struct vzctl_env_param *env, unsigned long capmask))
FUNC_DECL(int, vzctl2_env_get_cap, (struct vzctl_env_param *env, unsigned long *capmask))
FUNC_DECL(int, vzctl2_env_get_ha_enable, (struct vzctl_env_param *env, int *enable))
FUNC_DECL(int, vzctl2_env_set_ha_enable, (struct vzctl_env_param *env, int enable))
FUNC_DECL(int, vzctl2_env_get_ha_prio, (struct vzctl_env_param *env, unsigned long *prio))
FUNC_DECL(int, vzctl2_env_set_ha_prio, (struct vzctl_env_param *env, unsigned long prio))
FUNC_DECL(int, vzctl2_env_set_features, (struct vzctl_env_param *env, struct vzctl_feature_param *param))
FUNC_DECL(int, vzctl2_env_get_features, (struct vzctl_env_param *env, struct vzctl_feature_param *param))

/********************************/
FUNC_DECL(int, vzctl2_env_get_root_disk_param, (struct vzctl_env_param *env, struct vzctl_disk_param *out, int size))
FUNC_DECL(int, vzctl2_env_get_disk_param, (vzctl_disk_iterator it, struct vzctl_disk_param *out, int size))
FUNC_DECL(vzctl_disk_iterator, vzctl2_env_get_disk, (struct vzctl_env_param *env, vzctl_disk_iterator it))
FUNC_DECL(int, vzctl2_env_set_disk, (struct vzctl_env_handle *h, struct vzctl_disk_param *param))
FUNC_DECL(int, vzctl2_env_attach_disk, (struct vzctl_env_handle *h, struct vzctl_disk_param *param))
FUNC_DECL(int, vzctl2_env_get_autocompact, (struct vzctl_env_param *env, int *enabled))
FUNC_DECL(int, vzctl2_env_set_autocompact, (struct vzctl_env_param *env, int enabled))
FUNC_DECL(int, vzctl2_env_set_netfilter, (struct vzctl_env_param *env, unsigned mode))
FUNC_DECL(int, vzctl2_env_get_netfilter, (struct vzctl_env_param *env, unsigned *mode))
FUNC_DECL(int, vzctl2_env_del_disk, (vzctl_env_handle_ptr h, const char *guid, int flags))

#define LOAD_ALL_SYM() \
LOAD_SYM(vzctl2_get_env_status) \
LOAD_SYM(vzctl2_alloc_env_ids) \
LOAD_SYM(vzctl2_free_env_ids) \
LOAD_SYM(vzctl2_get_env_ids_by_state) \
LOAD_SYM(vzctl2_env_open) \
LOAD_SYM(vzctl2_env_open_conf) \
LOAD_SYM(vzctl2_env_close) \
LOAD_SYM(vzctl2_apply_param) \
LOAD_SYM(vzctl2_env_save) \
LOAD_SYM(vzctl2_env_get_name) \
LOAD_SYM(vzctl2_get_name) \
LOAD_SYM(vzctl2_set_name) \
LOAD_SYM(vzctl2_alloc_env_param) \
LOAD_SYM(vzctl2_free_env_param) \
LOAD_SYM(vzctl2_get_env_param) \
LOAD_SYM(vzctl2_env_set_ub_resource) \
LOAD_SYM(vzctl2_env_get_ub_resource) \
LOAD_SYM(vzctl2_env_set_ramsize) \
LOAD_SYM(vzctl2_env_get_ramsize) \
LOAD_SYM(vzctl2_env_set_diskspace) \
LOAD_SYM(vzctl2_env_get_diskspace) \
LOAD_SYM(vzctl2_env_set_diskinodes) \
LOAD_SYM(vzctl2_env_get_diskinodes) \
LOAD_SYM(vzctl2_env_set_quotaugidlimit) \
LOAD_SYM(vzctl2_env_get_quotaugidlimit) \
LOAD_SYM(vzctl2_env_set_cpulimit) \
LOAD_SYM(vzctl2_env_get_cpulimit) \
LOAD_SYM(vzctl2_env_set_cpu_count) \
LOAD_SYM(vzctl2_env_get_cpu_count) \
LOAD_SYM(vzctl2_env_set_cpuunits) \
LOAD_SYM(vzctl2_env_get_cpuunits) \
LOAD_SYM(vzctl2_env_set_iolimit) \
LOAD_SYM(vzctl2_env_get_iolimit) \
LOAD_SYM(vzctl2_env_get_ioprio) \
LOAD_SYM(vzctl2_env_set_ioprio) \
LOAD_SYM(vzctl2_env_get_description) \
LOAD_SYM(vzctl2_env_set_description) \
LOAD_SYM(vzctl2_env_set_hostname) \
LOAD_SYM(vzctl2_env_get_hostname) \
LOAD_SYM(vzctl2_env_get_ve_private_path) \
LOAD_SYM(vzctl2_env_get_ve_root_path) \
LOAD_SYM(vzctl2_env_get_ostemplate) \
LOAD_SYM(vzctl2_env_get_apptemplates) \
LOAD_SYM(vzctl2_env_set_autostart) \
LOAD_SYM(vzctl2_env_get_autostart) \
LOAD_SYM(vzctl2_env_get_bootorder) \
LOAD_SYM(vzctl2_env_get_ipstr) \
LOAD_SYM(vzctl2_env_add_ipaddress) \
LOAD_SYM(vzctl2_env_del_ipaddress) \
LOAD_SYM(vzctl2_env_get_ipaddress) \
LOAD_SYM(vzctl2_env_get_veth_param) \
LOAD_SYM(vzctl2_create_veth_dev) \
LOAD_SYM(vzctl2_free_veth_dev) \
LOAD_SYM(vzctl2_env_add_veth_ipaddress) \
LOAD_SYM(vzctl2_env_del_veth_ipaddress) \
LOAD_SYM(vzctl2_env_get_veth_ipaddress) \
LOAD_SYM(vzctl2_env_add_veth) \
LOAD_SYM(vzctl2_env_del_veth) \
LOAD_SYM(vzctl2_env_get_veth) \
LOAD_SYM(vzctl2_env_get_rate_param) \
LOAD_SYM(vzctl2_create_rate) \
LOAD_SYM(vzctl2_env_add_rate) \
LOAD_SYM(vzctl2_env_get_rate) \
LOAD_SYM(vzctl2_env_set_ratebound) \
LOAD_SYM(vzctl2_env_get_ratebound) \
LOAD_SYM(vzctl2_env_get_str_param) \
LOAD_SYM(vzctl2_env_add_nameserver) \
LOAD_SYM(vzctl2_env_get_nameserver) \
LOAD_SYM(vzctl2_env_add_searchdomain) \
LOAD_SYM(vzctl2_env_get_searchdomain) \
LOAD_SYM(vzctl2_get_last_error) \
LOAD_SYM(vzctl2_init_log) \
LOAD_SYM(vzctl2_set_log_quiet) \
LOAD_SYM(vzctl2_lib_init) \
LOAD_SYM(vzctl2_env_cpustat) \
LOAD_SYM(vzctl2_get_env_iostat) \
LOAD_SYM(vzctl2_set_ioprio) \
LOAD_SYM(vzctl2_set_iolimit) \
LOAD_SYM(vzctl2_register_evt) \
LOAD_SYM(vzctl2_unregister_evt) \
LOAD_SYM(vzctl2_get_state_evt) \
LOAD_SYM(vzctl2_send_state_evt) \
LOAD_SYM(vzctl2_get_evt_fd) \
LOAD_SYM(vzctl2_env_reset_uptime) \
LOAD_SYM(vzctl2_env_get_uptime) \
LOAD_SYM(vzctl2_env_set_uptime) \
LOAD_SYM(vzctl2_env_sync_uptime) \
LOAD_SYM(vzctl2_get_free_envid) \
LOAD_SYM(vzctl2_unlock_envid) \
LOAD_SYM(vzctl2_env_register) \
LOAD_SYM(vzctl2_env_unregister) \
LOAD_SYM(vzctl2_set_userpasswd) \
LOAD_SYM(vzctl2_env_auth) \
LOAD_SYM(vzctl2_get_cleanup_handler) \
LOAD_SYM(vzctl2_cancel_last_operation) \
LOAD_SYM(vzctl2_env_set_type) \
LOAD_SYM(vzctl2_env_get_type) \
LOAD_SYM(vzctl2_get_env_conf_path) \
LOAD_SYM(vzctl2_env_get_uuid) \
LOAD_SYM(vzctl2_env_set_uuid) \
LOAD_SYM(vzctl2_env_get_param) \
LOAD_SYM(vzctl2_env_exec_async) \
LOAD_SYM(vzctl2_env_exec_wait) \
LOAD_SYM(vzctl2_env_lock) \
LOAD_SYM(vzctl2_env_unlock) \
LOAD_SYM(vzctl2_set_iopslimit) \
LOAD_SYM(vzctl2_env_set_iopslimit) \
LOAD_SYM(vzctl2_env_get_iopslimit) \
LOAD_SYM(vzctl2_mount_disk_snapshot) \
LOAD_SYM(vzctl2_env_get_layout) \
LOAD_SYM(vzctl2_env_set_layout) \
LOAD_SYM(vzctl2_mount_snapshot) \
LOAD_SYM(vzctl2_umount_disk_snapshot) \
LOAD_SYM(vzctl2_umount_snapshot) \
LOAD_SYM(vzctl2_merge_snapshot) \
LOAD_SYM(vzctl2_umount_image_by_dev) \
LOAD_SYM(vzctl2_create_image) \
LOAD_SYM(vzctl2_create_disk_image) \
LOAD_SYM(vzctl2_mount_image) \
LOAD_SYM(vzctl2_mount_disk_image) \
LOAD_SYM(vzctl2_create_env_private) \
LOAD_SYM(vzctl2_set_vzlimits) \
LOAD_SYM(vzctl2_env_set_cpumask) \
LOAD_SYM(vzctl2_env_get_cpumask) \
LOAD_SYM(vzctl2_env_set_apply_iponly) \
LOAD_SYM(vzctl2_env_get_apply_iponly) \
LOAD_SYM(vzctl2_get_env_meminfo) \
LOAD_SYM(vzctl2_env_set_cap) \
LOAD_SYM(vzctl2_env_get_cap) \
LOAD_SYM(vzctl2_env_get_ha_enable) \
LOAD_SYM(vzctl2_env_set_ha_enable) \
LOAD_SYM(vzctl2_env_get_ha_prio) \
LOAD_SYM(vzctl2_env_set_ha_prio) \
LOAD_SYM(vzctl2_env_get_features) \
LOAD_SYM(vzctl2_env_set_features)

#define LOAD_ALL_SYM_V2() \
LOAD_SYM(vzctl2_env_get_root_disk_param) \
LOAD_SYM(vzctl2_env_get_disk_param) \
LOAD_SYM(vzctl2_env_get_disk) \
LOAD_SYM(vzctl2_release_snap_holder) \
LOAD_SYM(vzctl2_env_create_temporary_snapshot) \
LOAD_SYM(vzctl2_env_delete_tsnapshot) \
LOAD_SYM(vzctl2_env_set_disk) \
LOAD_SYM(vzctl2_env_attach_disk) \
LOAD_SYM(vzctl2_env_get_autocompact) \
LOAD_SYM(vzctl2_env_set_autocompact) \
LOAD_SYM(vzctl2_env_set_netfilter) \
LOAD_SYM(vzctl2_env_get_netfilter) \
LOAD_SYM(vzctl2_env_del_disk)

#endif


