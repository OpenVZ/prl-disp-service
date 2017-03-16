///////////////////////////////////////////////////////////////////////////////
///
/// @file Buse.cpp
///
/// BUSE filesystem helper
///
/// @author ibazhitov@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <mntent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QDir>
#include <QFileInfo>
#include <prlcommon/Logging/Logging.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"

#include "Buse.h"

namespace {

inline void trace(const char *name, const char *path)
{
	WRITE_TRACE(DBG_FATAL, "syscall %s(%s) failed with error %d: %s",
		name, path, errno, ::strerror(errno));
}

} // anonymous namespace

namespace Buse {

Failure& Failure::code(PRL_RESULT code_)
{
	m_code = code_;
	return *this;
}

PRL_RESULT Failure::operator()()
{
	PRL_RESULT output = m_code;
	if (m_event)
		m_event->setEventCode(output);
	m_code = PRL_ERR_SUCCESS;
	return output;
}

PRL_RESULT Failure::operator()(const QString& first_)
{
	if (m_event)
		m_event->addEventParameter(
			new CVmEventParameter(PVE::String, first_, EVT_PARAM_MESSAGE_PARAM_0));
	return operator()();
}

/**
 * Get the name of the BUSE entry
 * @return name of the entry in the BUSE filesystem
 */
QString Entry::name() const
{
	return QFileInfo(m_path).fileName();
}

/**
 * Get the full path to the backup content (raw disk image)
 * @return path to the backup content
 */
QString Entry::content() const
{
	return QDir(m_path).filePath("content");
}

PRL_RESULT Entry::setParams(const QString& params)
{
	return set("params", params);
}

PRL_RESULT Entry::setFormat(const QString& fmt)
{
	return set("format", fmt);
}

PRL_RESULT Entry::setSource(const QString& path)
{
	return set("source", path);
}

/**
 * Write provided value into a given file
 *
 * @param file - path to the file, relative to entry folder
 * @param value - value to be set
 * @return PRL_RESULT error code
 */
PRL_RESULT Entry::set(const QString& file, const QString& value)
{
	QString path = QDir(m_path).filePath(file);
	int err;
	int fd = ::open(QSTR2UTF8(path), O_WRONLY);
	if (fd == -1) {
		trace("open", QSTR2UTF8(path));
		return m_failure(PRL_ERR_BUSE_ENTRY_INVALID, m_path);
	}

	ssize_t count;
	QByteArray v(value.toUtf8());
	do {
		count = ::write(fd, v.constData(), v.size());
		err = errno;
	} while (count == -1 && err == EINTR);
	::close(fd);
	if (count == -1) {
		trace("write", QSTR2UTF8(path));
		if (err == ENOENT) {
			WRITE_TRACE(DBG_INFO, "most likely the needed bused plugin is not found");
			return m_failure(PRL_ERR_BUSE_INTERNAL_ERROR);
		} else if (err == ENOMEM) {
			return m_failure(PRL_ERR_OUT_OF_MEMORY);
		} else if (err == EALREADY || err == EPERM) {
			WRITE_TRACE(DBG_INFO, "BUSE entry '%s' is already initialized", QSTR2UTF8(path));
			return m_failure(PRL_ERR_BUSE_ENTRY_ALREADY_INITIALIZED, m_path);
		}
		return m_failure(PRL_ERR_BUSE_ENTRY_IO_ERROR, m_path);
	}
	WRITE_TRACE(DBG_DEBUG, "successfully written value '%s' into '%s'",
		QSTR2UTF8(value), QSTR2UTF8(path));
	return PRL_ERR_SUCCESS;
}

/**
 * Read value from a file
 *
 * @param file - path to the file, relative to entry folder
 * @param[out] value - where to read the value
 * @return PRL_RESULT error code
 */
PRL_RESULT Entry::get(const QString& file, QString& value)
{
	QString path = QDir(m_path).filePath(file);
	int err;
	int fd = ::open(QSTR2UTF8(path), O_RDONLY);
	if (fd == -1) {
		trace("open", QSTR2UTF8(path));
		return m_failure(PRL_ERR_BUSE_ENTRY_INVALID, m_path);
	}

	const int chunk = 4096;
	QByteArray buf(chunk, 0);
	ssize_t count, off = 0;
	while (true) {
		do {
			count = ::read(fd, buf.data() + off, chunk);
			err = errno;
		} while (count == -1 && err == EINTR);
		if (count == -1) {
			trace("read", QSTR2UTF8(path));
			::close(fd);
			return m_failure(PRL_ERR_BUSE_ENTRY_IO_ERROR, m_path);
		}
		if (count < chunk) {
			buf.data()[off + count] = '\0';
			break;
		}
		off += chunk;
		buf.resize(buf.size() + chunk);
	}
	::close(fd);
	value = QString::fromUtf8(buf.constData());
	WRITE_TRACE(DBG_DEBUG, "successfully read value '%s' from '%s'",
		QSTR2UTF8(value), QSTR2UTF8(path));
	return PRL_ERR_SUCCESS;
}

/**
 * Constructor with parameters
 *
 * @param mountpoint - full path to where the BUSE filesystem is mounted; if
 * empty, then the mountpoint will be determined automatically
 */
Buse::Buse(const QString& mountpoint) :
	m_mountpoint(mountpoint.isEmpty() ? findMountpoint() : mountpoint)
{
	// TODO: check that the provided path is actually a BUSE mountpoint
}

/**
 * Set the event object, where the error details will be saved
 *
 * @param e - event
 * @return this object
 */
Buse& Buse::setErrorEvent(CVmEvent *e)
{
	m_failure.setTarget(e);
	return *this;
}

/**
 * Search for BUSE filesystem mountpoint
 * @return full path to where the BUSE filesystem is mounted or empty if error/not found
 */
QString Buse::findMountpoint()
{
	const char *mtab = "/proc/mounts";
	FILE *fp;
	char buffer[BUFSIZ];
	struct mntent mnt;
	QString rc;
	// TODO: use existing CFileHelper functionality
	if ((fp = ::setmntent(mtab, "r")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "setmntent(%s) error : %m", mtab);
		return rc;
	}

	while (::getmntent_r(fp, &mnt, buffer, sizeof(buffer)) != NULL) {
		if (strcmp(mnt.mnt_type, "fuse.bused"))
			continue;
		struct stat st;
		WRITE_TRACE(DBG_DEBUG, "found BUSE mount point at '%s'", mnt.mnt_dir);
		/* check, that the mount point is healthy */
		if (::stat(mnt.mnt_dir, &st) == -1) {
			trace("stat", mnt.mnt_dir);
		} else {
			rc = mnt.mnt_dir;
		}
		break;
	}
	::endmntent(fp);
	return rc;
}

/**
 * Check whether an entry with the given name already exists
 *
 * @param name - name of the entry
 * @return PRL_RESULT error code
 * @retval PRL_ERR_SUCCESS if an entry with given name exists
 * @retval PRL_ERR_ENTRY_INVALID if an entry with given name doesn't exist
 */
PRL_RESULT Buse::exists(const QString& name)
{
	if (m_mountpoint.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "BUSE filesystem mountpoint is not found");
		return m_failure(PRL_ERR_BUSE_NOT_MOUNTED);
	}
	if (QFileInfo(m_mountpoint, name).exists())
		return PRL_ERR_SUCCESS;
	return m_failure(PRL_ERR_BUSE_ENTRY_INVALID, QDir(m_mountpoint).filePath(name));
}

/**
 * Create an entry in the BUSE filesystem
 *
 * @param name - name of the entry to be created
 * @param evt - event object, where the error details will be saved
 * @return new entry object, could be NULL if an error occurs
 */
Entry *Buse::create(const QString& name, CVmEvent *evt)
{
	if (m_mountpoint.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "BUSE filesystem mountpoint is not found");
		m_failure(PRL_ERR_BUSE_NOT_MOUNTED);
		return NULL;
	}

	std::auto_ptr<Entry> e(new(std::nothrow) Entry(name, m_mountpoint, evt));
	if (!e.get()) {
		m_failure(PRL_ERR_OUT_OF_MEMORY);
		return NULL;
	}

	QString path = QDir(m_mountpoint).filePath(name);
	if (::mkdir(QSTR2UTF8(path), 0700) == -1) {
		int err = errno;
		trace("mkdir", QSTR2UTF8(path));
		if (err == EEXIST) {
			m_failure(PRL_ERR_BUSE_ENTRY_ALREADY_EXIST, path);
		} else if (err == ENOMEM) {
			m_failure(PRL_ERR_OUT_OF_MEMORY);
		} else {
			m_failure(PRL_ERR_BUSE_ENTRY_IO_ERROR, path);
		}
		return NULL;
	}

	WRITE_TRACE(DBG_DEBUG, "created BUSE entry '%s'", QSTR2UTF8(path));
	return e.release();
}

/**
 * Get the name of entry from a full path
 *
 * @param path - full path, containing the entry name (e.g. full path to the content file)
 * @param[out] name - name of the BUSE entry (first-level folder name)
 * @return PRL_RESULT error code
 */
PRL_RESULT Buse::getEntryName(const QString& path, QString& name)
{
	if (m_mountpoint.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "BUSE filesystem mountpoint is not found");
		return m_failure(PRL_ERR_BUSE_NOT_MOUNTED);
	}
	if (QFileInfo(path).isRelative())
		return m_failure(PRL_ERR_BUSE_ENTRY_INVALID, path);
	/* XXX: CFileHelper::GetMountPoint() aborts if the last path element is a symlink */
	QString p = QFileInfo(path).canonicalFilePath();
	QString mp = CFileHelper::GetMountPoint(p);
	if (mp != QFileInfo(m_mountpoint).canonicalFilePath()) {
		WRITE_TRACE(DBG_FATAL, "'%s' is not a BUSE entry", QSTR2UTF8(p));
		return m_failure(PRL_ERR_BUSE_ENTRY_INVALID, p);
	}
	/* the first path element under the mountpoint is our entry */
	name = p.mid(mp.size()).split("/", QString::SkipEmptyParts)[0];
	return PRL_ERR_SUCCESS;
}

/**
 * Obtain the name of entry from a full path to a file in entry folder
 *
 * @param path - full path to a file (e.g. 'content'), located in the entry folder
 * @return name of entry
 */
QString Buse::getEntryName(const QString& path)
{
	return QFileInfo(path).dir().dirName();
}

/**
 * Remove an entry from the BUSE filesystem
 *
 * @param name - name of the entry (first level dir name) or full path, containing
 * the entry name (e.g. full path to the content file)
 * @return PRL_RESULT error code
 */
PRL_RESULT Buse::remove(const QString& name)
{
	if (m_mountpoint.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "BUSE filesystem mountpoint is not found");
		return m_failure(PRL_ERR_BUSE_NOT_MOUNTED);
	}

	QString n = name;
	if (QFileInfo(name).isAbsolute()) {
		PRL_RESULT res;
		if (PRL_FAILED(res = getEntryName(name, n)))
			return res;
	}
	QString path = QDir(m_mountpoint).filePath(n);
	if (::rmdir(QSTR2UTF8(path)) == -1) {
		trace("rmdir", QSTR2UTF8(path));
		return m_failure(PRL_ERR_BUSE_ENTRY_INVALID, path);
	}
	WRITE_TRACE(DBG_DEBUG, "deleted BUSE entry '%s'", QSTR2UTF8(path));
	return PRL_ERR_SUCCESS;
}

/**
 * Find entries that contain provided substring in their names
 *
 * @param what - the substring that will be searched in entries names
 * @param[out] found - list of entries names matching the search criteria
 * @return PRL_RESULT error code
 */
PRL_RESULT Buse::find(const QString& what, QStringList& found)
{
	if (m_mountpoint.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "BUSE filesystem mountpoint is not found");
		return m_failure(PRL_ERR_BUSE_NOT_MOUNTED);
	}
	found = QDir(m_mountpoint).entryList(QDir::Dirs | QDir::NoDotAndDotDot).filter(what);
	return PRL_ERR_SUCCESS;
}

} // namespace Buse
