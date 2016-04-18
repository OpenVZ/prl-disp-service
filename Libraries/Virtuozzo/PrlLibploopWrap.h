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

#ifndef _PrlLibploopWrap_H__
#define _PrlLibploopWrap_H__

#define _USE_DLOPEN_

#include "vz/libploop.h"

#define FUNC_DECL(res, name, args) \
	typedef res (*wrap##name) args;

FUNC_DECL(int, ploop_mount_image, (struct ploop_disk_images_data *di, struct ploop_mount_param *param))
FUNC_DECL(int, ploop_umount_image, (struct ploop_disk_images_data *di))
FUNC_DECL(struct ploop_disk_images_data *, ploop_alloc_diskdescriptor, (void))
FUNC_DECL(void, ploop_free_diskdescriptor, (struct ploop_disk_images_data *di))
FUNC_DECL(int, ploop_read_diskdescriptor, (const char *dir, struct ploop_disk_images_data *di))
FUNC_DECL(int, ploop_open_dd, (struct ploop_disk_images_data **di, const char *fname))
FUNC_DECL(void, ploop_close_dd, (struct ploop_disk_images_data *di))
FUNC_DECL(int, ploop_create_snapshot, (struct ploop_disk_images_data *di, struct ploop_snapshot_param *param))
FUNC_DECL(int, ploop_get_dev, (struct ploop_disk_images_data *di, char *out, int len))
FUNC_DECL(int, ploop_create_dd, (const char *ddxml, struct ploop_create_param *param))
FUNC_DECL(int, ploop_set_max_delta_size, (struct ploop_disk_images_data *di, unsigned long long size))
FUNC_DECL(int, ploop_get_base_delta_fname, (struct ploop_disk_images_data *di, char *out, int len))

#define LOAD_ALL_SYM() \
LOAD_SYM(ploop_mount_image) \
LOAD_SYM(ploop_umount_image) \
LOAD_SYM(ploop_alloc_diskdescriptor) \
LOAD_SYM(ploop_free_diskdescriptor) \
LOAD_SYM(ploop_read_diskdescriptor) \
LOAD_SYM(ploop_open_dd) \
LOAD_SYM(ploop_close_dd) \
LOAD_SYM(ploop_create_snapshot) \
LOAD_SYM(ploop_get_dev) \
LOAD_SYM(ploop_create_dd) \
LOAD_SYM(ploop_set_max_delta_size) \
LOAD_SYM(ploop_get_base_delta_fname)

#endif
