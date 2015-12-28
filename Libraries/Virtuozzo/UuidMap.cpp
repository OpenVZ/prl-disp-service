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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include <QMutex>

#define UUID_MAP_FILE	"/etc/parallels/uuid.map"

static int fd_uuidmap = -1;
static QMutex uuidmap_mutex;

struct uuidmap_t {
	Uuid_t uuid;
	unsigned int id;
};

static int file_open_uuidmap()
{
	if (fd_uuidmap != -1)
		return fd_uuidmap;

	uuidmap_mutex.lock();
	if (fd_uuidmap == -1) {
		fd_uuidmap = open(UUID_MAP_FILE, O_CREAT | O_RDWR, 0644);
		if (fd_uuidmap == -1)
			WRITE_TRACE(DBG_FATAL, "unable to open " UUID_MAP_FILE " : %m");
	}
	uuidmap_mutex.unlock();

	return fd_uuidmap;
}

static int file_lock(int fd)
{
	int ret;

	if ((ret = flock(fd, LOCK_EX)))
		WRITE_TRACE(DBG_FATAL, "unable to lock " UUID_MAP_FILE " : %m");
	return ret;
}

static int file_unlock(int fd)
{
	int ret;

	if ((ret = flock(fd, LOCK_UN)))
		WRITE_TRACE(DBG_FATAL, "unlock failed: %m");

	return ret;
}

int file_find_uuidmap(int fd, struct uuidmap_t *uuidmap)
{
	int r;
	struct uuidmap_t tmp;

	lseek(fd, 0, SEEK_SET);
	do {
		r = read(fd, &tmp, sizeof(struct uuidmap_t));
		if (r == 0)
			break;
		else if (r == -1) {
			WRITE_TRACE(DBG_FATAL, "failed to read from " UUID_MAP_FILE " %m");
			return -1;
		}
		if (memcmp(&tmp.uuid, &uuidmap->uuid, sizeof(Uuid_t)) == 0 &&
		    tmp.id == uuidmap->id)
			return 1;
	} while (r);

	return 0;
}

static int file_add_uuidmap(int fd, struct uuidmap_t *uuidmap)
{
	int ret;

	struct uuidmap_t tmp;
	memset(&tmp, 0, sizeof(struct uuidmap_t));

	if (file_find_uuidmap(fd, &tmp)) {
		if (lseek(fd, -((off_t) sizeof(struct uuidmap_t)), SEEK_CUR) == -1) {
			WRITE_TRACE(DBG_FATAL, "failed lseek in " UUID_MAP_FILE " %m");
			return -1;
		}
	}

	WRITE_TRACE(DBG_FATAL, "uuid=%s id=%d",
		Uuid::toString(uuidmap->uuid).toUtf8().constData(), uuidmap->id);

	if ((ret = write(fd, uuidmap, sizeof(struct uuidmap_t))) == -1)
		WRITE_TRACE(DBG_FATAL, "failed write to " UUID_MAP_FILE " %m");

	return ret;
}

static int file_del_uuidmap(int fd)
{
	int ret;

	if (lseek(fd, -((off_t) sizeof(struct uuidmap_t)), SEEK_CUR) == -1) {
		WRITE_TRACE(DBG_FATAL, "failed lseek in " UUID_MAP_FILE " %m");
		return -1;
	}

	struct uuidmap_t tmp;
	memset(&tmp, 0, sizeof(struct uuidmap_t));

	if ((ret = write(fd, &tmp, sizeof(struct uuidmap_t))) == -1)
		WRITE_TRACE(DBG_FATAL, "failed write to " UUID_MAP_FILE " %m");

	return ret;
}

static int file_update_uuidmap(struct uuidmap_t *uuidmap, bool del)
{
	int fd;
	int ret = 0;

	if ((fd = file_open_uuidmap()) == -1)
		return -1;
	if ((ret = file_lock(fd)))
		return ret;
	ret = file_find_uuidmap(fd, uuidmap);
	if (del) {
		if (ret)
			ret = file_del_uuidmap(fd);
	} else if (!ret)
		ret = file_add_uuidmap(fd, uuidmap);
	file_unlock(fd);

	return ret;
}

int update_uuidmap(Uuid_t uuid, unsigned int id, bool del)
{
	struct uuidmap_t uuidmap;

	uuidmap.id = id;
	memcpy(&uuidmap.uuid, uuid, sizeof(Uuid_t));

	return file_update_uuidmap(&uuidmap, del);
}

int init_uuidmap()
{
	int fd, ret;

	if ((fd = file_open_uuidmap()) == -1)
		return -1;
	if ((ret = ftruncate(fd, 0)) == -1)
		WRITE_TRACE(DBG_FATAL, "failed to truncate " UUID_MAP_FILE " %m");

	return ret;
}

