///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmMounter.h
///
/// Controller of VM mount/unmount using libguestfs.
///
/// @author mperevedentsev
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __DSPVMMOUNTER_H__
#define __DSPVMMOUNTER_H__

struct guestfs_h;
typedef struct guestfs_h guestfs_h;

#include <QObject>
#include <QThread>
#include <QMap>
#include <QMutex>

#include <prlsdk/PrlTypes.h>

#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>

#include <XmlModel/VmConfig/CVmConfiguration.h>
#include "CVmIdent.h"
#include "CDspLibvirt.h"

///////////////////////////////////////////////////////////////////////////////
// MounterThread

struct MounterThread: QThread
{
	Q_OBJECT

	// Hangs here until guestfs_umount_local is called.
	void run();

public:
	void setHandle(guestfs_h *gfsHandle)
	{
		m_gfsHandle = gfsHandle;
	}

	guestfs_h* getGfsHandle() const
	{
		return m_gfsHandle;
	}

	PRL_RESULT getResult() const
	{
		return m_result;
	}

private:
	guestfs_h *m_gfsHandle;
	PRL_RESULT m_result;
};

///////////////////////////////////////////////////////////////////////////////
// GuestFS

struct GuestFS
{
	GuestFS(guestfs_h *gfsHandle):
		m_gfsHandle(gfsHandle), m_curDevice("sda")
	{
	}

	// Add drive to handle. Must be called before launch().
	PRL_RESULT addDrive(const CVmHardDisk &hardDisk, bool readOnly);
	// Run guestfs appliance.
	PRL_RESULT launch() const;
	// Get partitions in format "/dev/sdXN" or "/dev/VG/LV".
	PRL_RESULT getMountablePartitions(QStringList &partitions);
	// Create mountpoint(s) and mount partition to it.
	PRL_RESULT mountPartition(const QString &partition);
	/* Initialize local-mount
	 * (calls to dir will hang until guestfs_mount_local_run is called).
	 */
	PRL_RESULT mountLocal(const QString &mountpoint, bool readOnly);
	void umountLocal();

	PRL_RESULT getMountInfo(const QString &partition, QString &info) const;

	// For launched handle
	void shutdown();
	// For non-launched handle
	void close();

private:
	PRL_RESULT makeMountpoint(const QString &mountpoint);
	PRL_RESULT createMountpoint(const QString &partition);

	guestfs_h *m_gfsHandle;
	// "sdb"
	QString m_curDevice;
	// "sda" -> hdd"0"
	QMap<QString, int> m_deviceToHdd;
	// hdd"0" -> "/vz/harddisk.hdd"
	QMap<int, QString> m_hddToPath;
	// "/dev/sda1" -> "ext3"
	QMap<QString, QString> m_partToFilesystem;
	// "/dev/sda1" -> "/hdd0/volume1"
	QMap<QString, QString> m_partToMountpoint;
	QSet<QString> m_createdMountpoints;
	QString m_localMountpoint;
};

///////////////////////////////////////////////////////////////////////////////
// CDspVmMount

struct CDspVmMount: public QObject
{
	Q_OBJECT

public:
	CDspVmMount(const CVmIdent &ident, const QString &mountPoint,
	            const QString &mountInfo, guestfs_h *gfsHandle):
		m_ident(ident), m_mountPoint(mountPoint), m_mountInfo(mountInfo)
	{
		Q_ASSERT(QObject::connect(
					&m_thread, SIGNAL(finished()), this, SLOT(onThreadFinished()),
					Qt::DirectConnection));
		Q_ASSERT(QObject::connect(
					&m_thread, SIGNAL(terminated()), this, SLOT(onThreadFinished()),
					Qt::DirectConnection));
		m_thread.setHandle(gfsHandle);
		m_thread.start();
	}

	/* Try to unmount by mountpoint.
	 * Usable in case of previous crash.
	 * Returns SUCCESS in case of successful release or non-mounted mountpoint.
	 */
	static PRL_RESULT releaseMountpoint(const QString &mountpoint);

	/* Unmount previously mounted vm filesystems. */
	PRL_RESULT umount();
	/* Do resource cleanup even if unmount failed. */
	void umountForce();

	bool isActive() const
	{
		return m_thread.isRunning();
	}

	QString getMountInfo() const
	{
		return m_mountInfo;
	}

	const CVmIdent& getVmIdent() const
	{
		return m_ident;
	}

public slots:
	void onThreadFinished();

signals:
	void mountFinished(const QString &uuid);

private:
	void cleanup();

	CVmIdent m_ident;
	QString m_mountPoint;
	QString m_mountInfo;
	MounterThread m_thread;
};

///////////////////////////////////////////////////////////////////////////////
// MountFinishedHandler

/* For async request processing */
struct MountFinishedHandler: QObject
{
	Q_OBJECT

public slots:
	void onMountFinished(const QString &uuid)
	{
		emit removeMount(uuid);
	}

signals:
	void removeMount(const QString &uuid);
};

///////////////////////////////////////////////////////////////////////////////
// CDspVmMountStorage

/* Thread-safe storage for mounts. */
struct CDspVmMountStorage
{
	/* Return mount info, or message about non-mounted VM if absent. */
	QString getInfo(const QString &uuid) const;

	/* Return mounter, or NULL if not present. */
	SmartPtr<CDspVmMount> beginPop(const QString &uuid);
	bool rollbackPop(const QString &uuid);
	bool commitPop(const QString &uuid);

	bool insert(const QString &uuid, const SmartPtr<CDspVmMount> &mount);

	QList<QString> getKeys() const;

private:
	QMap<QString, SmartPtr<CDspVmMount> > m_storage;
	QMap<QString, SmartPtr<CDspVmMount> > m_transStorage;
	mutable QMutex m_storageLock;
};

///////////////////////////////////////////////////////////////////////////////
// CDspVmMountRegistry

/* Interface to the mount subsystem.
 * Registry also tracks status of mounts and updates VMs if mount is interruped.
 * All operations are thread-safe.
 */
struct CDspVmMountRegistry: QObject
{
	Q_OBJECT

public:
	CDspVmMountRegistry();
	~CDspVmMountRegistry();

	Libvirt::Result mount(
			const SmartPtr<CVmConfiguration> &pVmConfig,
			const QString &dirUuid, const QString &mountPoint,
			bool readOnly);

	/* Return mount info, or message about non-mounted VM if absent. */
	QString getInfo(const QString &uuid) const
	{
		return m_storage.getInfo(uuid);
	}

	Libvirt::Result umount(const QString &uuid);

public slots:
	void umountForce(const QString &uuid);

private:
	bool startTracking(const QString &uuid, const SmartPtr<CDspVmMount> &mount);
	bool stopTracking(const QString &uuid, const SmartPtr<CDspVmMount> &mount);

	void umountAll();

	CDspVmMountStorage m_storage;
	MountFinishedHandler m_handler;
	QThread m_handlerThread;
};

#endif // __DSPVMMOUNTER_H__
