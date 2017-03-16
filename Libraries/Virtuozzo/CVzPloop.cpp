///////////////////////////////////////////////////////////////////////////////
///
/// @file Ploop.cpp
///
/// Implementation for imagetool lib interface:
///  - ploop attach/detach logic
///
/// @author wolf
///
/// Copyright (c) 2012-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QProcess>
#include <prlsdk/PrlErrors.h>
#include <prlcommon/Logging/Logging.h>
#include <dlfcn.h>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include "CVzPloop.h"

#ifndef _DYN_VZLIB_
	#include "PrlLibploopWrap.h"

	static void *s_plooplib_handle = NULL;

	//  allocate library functions ptr
	#define LOAD_SYM(name) \
		static wrap##name name;

	LOAD_ALL_SYM()

	#define QUOTENAME(x) #x
#else
	#include <ploop/libploop.h>
#endif	// _DYN_VZLIB_

using namespace std;

static QMutex s_lock;

static bool GetProcStdOutput(const QStringList& slEnv,
				const QString& command,
				QString& stdOut)
{
	QProcess proc;
	proc.setEnvironment(slEnv);
	proc.start(command);

	if (!proc.waitForStarted())
	{
		WRITE_TRACE(DBG_FATAL, "Error : process \"%s\" start failed.",
			qPrintable(command));
		return false;
	}

	if (!proc.waitForFinished(15 * 1000))
	{
		WRITE_TRACE(DBG_FATAL, "%s tool not responding err=%d. Terminate it now.",
			qPrintable(command), proc.error());
		proc.terminate();
		if (!proc.waitForFinished(60 * 1000))
		{
			proc.kill();
			proc.waitForFinished(-1);
		}
		return false;
	}

	Q_ASSERT(QProcess::NotRunning == proc.state());
	stdOut = proc.readAllStandardOutput();
	return proc.exitCode() == 0 ? true : false;
}

namespace {

PRL_RESULT loadPloopLibrary()
{
#ifndef _DYN_VZLIB_

	QMutexLocker locker(&s_lock);
	// Is it initialized already?
	if (s_plooplib_handle)
		return PRL_ERR_SUCCESS;

	// Is the ploop lib available?
	if (!QFileInfo(PLOOPLIB).exists())
	{
		WRITE_TRACE(DBG_FATAL, "The ploop library %s doesn't exist!", PLOOPLIB);
		return PRL_ERR_UNEXPECTED;
	}

	WRITE_TRACE(DBG_DEBUG, "Loading ploop library");

	s_plooplib_handle = dlopen(PLOOPLIB, RTLD_LAZY);
	if (s_plooplib_handle == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to load %s: %s",
				PLOOPLIB, dlerror());
		return PRL_ERR_UNEXPECTED;
	}

#if defined LOAD_SYM
	#undef LOAD_SYM
#endif
#define LOAD_SYM(name) \
	do { \
		name = (wrap##name) dlsym(s_plooplib_handle, QUOTENAME(name)); \
		if (name == NULL) { \
			WRITE_TRACE(DBG_FATAL, "Failed to load %s: %s", QUOTENAME(name), dlerror()); \
			dlclose(s_plooplib_handle); \
			s_plooplib_handle = NULL; \
			return PRL_ERR_UNEXPECTED; \
		} \
	} while (0);

	LOAD_ALL_SYM()

#undef LOAD_SYM
#endif	// _DYN_VZLIB_
	return PRL_ERR_SUCCESS;
}

} // anonymous namespace

Ploop::Ploop(std::string szDiskPath)
	: m_szPloopDiskPath(szDiskPath),
	m_szPloopDeviceName(""),
	m_di(NULL)
{
	WRITE_TRACE(DBG_DEBUG, "Created for %s", szDiskPath.c_str());
}

Ploop::~Ploop()
{
	Detach();
}

PRL_RESULT Ploop::Init()
{
	std::string path = m_szPloopDiskPath;
	QFileInfo fi(m_szPloopDiskPath.c_str());

	if (!fi.exists())
	{
		WRITE_TRACE(DBG_FATAL, "Given path %s doesn't exist",
			m_szPloopDiskPath.c_str());
		return PRL_ERR_UNEXPECTED;
	}

	/* We accept the Parallels HDD directory or
	   path to DISKDESCRIPTOR_XML */
	if (fi.isDir())
	{
		path = path + "/" + DISKDESCRIPTOR_XML;
		fi = QFileInfo(path.c_str());

		if (!fi.exists())
		{
			WRITE_TRACE(DBG_FATAL, "%s is not found in %s",
				DISKDESCRIPTOR_XML, m_szPloopDiskPath.c_str());
			return PRL_ERR_UNEXPECTED;
		}

	}

	m_szPloopDiskPath = path;

	// Is the parted available?
	if (!QFileInfo(PARTED).exists())
	{
		WRITE_TRACE(DBG_FATAL, "The %s utility doesn't exist!", PARTED);
		return PRL_ERR_UNEXPECTED;
	}

	return loadPloopLibrary();
}

bool Ploop::isLoaded()
{
#ifndef _DYN_VZLIB_
	QMutexLocker locker(&s_lock);
	return (s_plooplib_handle != NULL);
#else
	return true;
#endif
}

PRL_RESULT Ploop::Attach(bool bReadOnly)
{
	if (!isLoaded())
	{
		WRITE_TRACE(DBG_FATAL, "Ploop library is not loaded!");
		return PRL_ERR_UNEXPECTED;
	}

	if (m_szPloopDiskPath == "")
	{
		WRITE_TRACE(DBG_FATAL, "%s is not found!", DISKDESCRIPTOR_XML);
		return PRL_ERR_UNEXPECTED;
	}

	if (ploop_open_dd(&m_di, m_szPloopDiskPath.c_str())) {
		WRITE_TRACE(DBG_FATAL, "Can't read ploop disk descriptor %s",
			m_szPloopDiskPath.c_str());
		ploop_close_dd(m_di);
		m_di = NULL;
		return PRL_ERR_UNEXPECTED;
	}

	/* Fill default parameters */
	struct ploop_mount_param mountopts;
	memset(&mountopts, 0, sizeof(mountopts));
	mountopts.ro = bReadOnly ? 1 : 0;

	if (ploop_mount_image(m_di, &mountopts))
	{
		WRITE_TRACE(DBG_FATAL, "Can't mount ploop %s",
			m_szPloopDiskPath.c_str());
		ploop_close_dd(m_di);
		m_di = NULL;
		return PRL_ERR_UNEXPECTED;
	}

	m_szPloopDeviceName = mountopts.device;

	/* Because of PCS limitation, udev doesn't create all
	   partition-related devices, so need to do it. parted creates
	   these devices + we get nice debug output */

	QString parted_out;
	QString parted_command;
	parted_command += PARTED;
	parted_command += " -s ";
	parted_command += m_szPloopDeviceName.c_str();
	parted_command += " print";

	if (!GetProcStdOutput(QProcess::systemEnvironment(), parted_command, parted_out))
	{
		// Ignore this: parted always fails for empty hdds
		WRITE_TRACE(DBG_INFO, "Error : Failed to run command \"%s\" and get std output.",
			qPrintable(parted_command));
		return PRL_ERR_SUCCESS;
	}

	WRITE_TRACE(DBG_DEBUG, "Parted output:\n %s\n", parted_out.toStdString().c_str());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Ploop::Detach()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;

	if (!isLoaded())
	{
		WRITE_TRACE(DBG_FATAL, "Ploop library is not loaded!");
		return PRL_ERR_UNEXPECTED;
	}

	if (m_di == NULL)
	{
		WRITE_TRACE(DBG_DEBUG, "Ploop is not attached");
		return PRL_ERR_SUCCESS;
	}

	if (ploop_umount_image(m_di))
	{
		WRITE_TRACE(DBG_FATAL, "Can't mount ploop %s",
			m_szPloopDiskPath.c_str());
		res = PRL_ERR_UNEXPECTED;
	}

	ploop_close_dd(m_di);
	m_szPloopDeviceName = "";
	m_di = NULL;

	return res;
}

namespace PloopImage
{

Image::Image(const QString& path)
{
	m_path = QDir(path).filePath(DISK_DESCRIPTOR_XML);
}

void Image::trace(const char *call, int res) const
{
	WRITE_TRACE(DBG_FATAL, "%s(%s) failed: %d", call, QSTR2UTF8(m_path), res);
}

PRL_RESULT Image::createDiskDescriptor(const QString& imagePath, unsigned long long size) const
{
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;

	struct ploop_create_param param;
	memset(&param, 0, sizeof(param));
	param.size = size / 512 + (size % 512 ? 1 : 0);
	param.mode = PLOOP_RAW_MODE;
	QByteArray path(imagePath.toUtf8());
	param.image = path.data();

	int res = ploop_create_dd(QSTR2UTF8(m_path), &param);
	if (res)
	{
		trace("ploop_create_dd", res);
		return PRL_ERR_UNEXPECTED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::createSnapshot(const QString& path) const
{
	ploop_disk_images_data *di;
	ploop_snapshot_param param;
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;
	int res = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (res)
	{
		trace("ploop_open_dd", res);
		return PRL_ERR_UNEXPECTED;
	}
	memset(&param, 0, sizeof(param));
	QByteArray buf;
	if (!path.isEmpty())
	{
		buf = path.toUtf8();
		param.snap_dir = buf.data();
	}
	res = ploop_create_snapshot(di, &param);
	if (res)
		trace("ploop_create_snapshot", res);
	ploop_close_dd(di);
	return res ? PRL_ERR_UNEXPECTED : PRL_ERR_SUCCESS;
}

PRL_RESULT Image::setEncryptionKeyid(const QString& keyid) const
{
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;

	ploop_disk_images_data *di;
	rc = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (rc)
	{
		trace("ploop_open_dd", rc);
		return PRL_ERR_UNEXPECTED;
	}

	rc = ploop_set_encryption_keyid(di, QSTR2UTF8(keyid));
	if (rc)
		trace("ploop_create_snapshot", rc);

	ploop_close_dd(di);
	return rc ? PRL_ERR_UNEXPECTED : PRL_ERR_SUCCESS;
}

PRL_RESULT Image::updateDiskInformation(CVmHardDisk& disk) const
{
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;

	ploop_disk_images_data *di;
	rc = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (rc)
	{
		trace("ploop_open_dd", rc);
		return PRL_ERR_UNEXPECTED;
	}

	rc = ploop_read_dd(di);
	if (rc)
	{
		trace("ploop_read_dd", rc);
		ploop_close_dd(di);
		return PRL_ERR_UNEXPECTED;
	}

	QString keyid;
	if (di->enc && di->enc->keyid)
		keyid = di->enc->keyid;

	CVmHddEncryption* e = disk.getEncryption();
	if (!e) {
		// don't create instance if key id was not set
		if (!keyid.isEmpty()) {
			e = new CVmHddEncryption();
			e->setKeyId(keyid);
			disk.setEncryption(e);
		}
	} else
		e->setKeyId(keyid);

	disk.setSizeInBytes(di->size << 9); /* Sectors -> bytes */

	quint64 size = 0;
	for (int i = 0; i < di->nimages; i++)
		size += QFileInfo(di->images[i]->file).size();

	disk.setSizeOnDisk(size >> 20); /* bytes To Mbytes */
	ploop_close_dd(di);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::setActiveDeltaLimit(unsigned long long lim) const
{
	ploop_disk_images_data *di;
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;
	int res = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (res)
	{
		trace("ploop_open_dd", res);
		return PRL_ERR_UNEXPECTED;
	}

	res = ploop_set_max_delta_size(di, lim);
	if (res)
		trace("ploop_set_max_delta_size", res);
	ploop_close_dd(di);
	return res ? PRL_ERR_UNEXPECTED : PRL_ERR_SUCCESS;
}

PRL_RESULT Image::getBaseDeltaFilename(QString& path) const
{
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;
	ploop_disk_images_data *di;
	int res = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (res)
	{
		trace("ploop_open_dd", res);
		return PRL_ERR_UNEXPECTED;
	}
	QByteArray buf(PATH_MAX + 1, 0);
	res = ploop_get_base_delta_fname(di, buf.data(), buf.size() - 1);
	ploop_close_dd(di);
	if (res)
	{
		trace("ploop_get_base_delta_fname", res);
		return PRL_ERR_UNEXPECTED;
	}
	path = QString::fromUtf8(buf.constData());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::mount(QString& devName) const
{
	ploop_disk_images_data *di;
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;
	int res = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (res)
	{
		trace("ploop_open_dd", res);
		return PRL_ERR_UNEXPECTED;
	}

	ploop_mount_param param;
	::memset(&param, 0, sizeof(param));

	res = ploop_mount_image(di, &param);
	if (res)
		trace("ploop_mount_image", res);
	ploop_close_dd(di);
	if (res == 0)
		devName = param.device;
	return res ? PRL_ERR_UNEXPECTED : PRL_ERR_SUCCESS;
}

PRL_RESULT Image::umount() const
{
	ploop_disk_images_data *di;
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;
	int res = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (res)
	{
		trace("ploop_open_dd", res);
		return PRL_ERR_UNEXPECTED;
	}
	res = ploop_umount_image(di);
	ploop_close_dd(di);
	if (res)
	{
		if (res == 40 /* SYSEXIT_DEV_NOT_MOUNTED */)
		{
			WRITE_TRACE(DBG_DEBUG, "ploop device is not mounted");
			return PRL_ERR_SUCCESS;
		}
		trace("ploop_umount_image", res);
	}
	return res ? PRL_ERR_UNEXPECTED : PRL_ERR_SUCCESS;
}

PRL_RESULT Image::getMountedDevice(QString& device) const
{
	ploop_disk_images_data *di;
	PRL_RESULT rc = loadPloopLibrary();
	if (PRL_FAILED(rc))
		return rc;
	int res = ploop_open_dd(&di, QSTR2UTF8(m_path));
	if (res)
	{
		trace("ploop_open_dd", res);
		return PRL_ERR_UNEXPECTED;
	}
	QByteArray buf(PATH_MAX + 1, 0);
	res = ploop_get_dev(di, buf.data(), buf.size() - 1);
	ploop_close_dd(di);
	if (res)
	{
		if (res == 1)
		{
			WRITE_TRACE(DBG_DEBUG, "ploop device is not mounted");
			device.clear();
			return PRL_ERR_SUCCESS;
		}
		trace("ploop_get_dev", res);
		return PRL_ERR_UNEXPECTED;
	}
	device = QString::fromUtf8(buf.constData());
	return PRL_ERR_SUCCESS;
}

} // namespace PloopImage
