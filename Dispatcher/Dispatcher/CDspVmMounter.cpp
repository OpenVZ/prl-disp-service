///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmMounter.cpp
///
/// Controller of VM mount/unmount using libguestfs.
///
/// @author mperevedentsev
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
/////////////////////////////////////////////////////////////////////////////////
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <QSet>
#include <guestfs.h>

#include "CDspVmMounter.h"
#include "CDspService.h"
#include "CDspVmStateSender.h"

namespace
{

const char GUESTUNMOUNT[] = "/usr/bin/guestunmount";
const char FINDMNT[] = "/usr/bin/findmnt";
enum {GFS_PROGRAM_ERROR = 1, GFS_CANNOT_UNMOUNT = 2, GFS_NOT_MOUNTED = 3};

} // namespace

///////////////////////////////////////////////////////////////////////////////
// MounterThread

void MounterThread::run()
{
	// It hangs here until guestfs_umount_local is called.
	if (guestfs_mount_local_run(m_gfsHandle)) {
		WRITE_TRACE(DBG_FATAL, "FUSE loop error in guestfs");
		m_result = PRL_ERR_FAILURE;
	} else
		m_result = PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_DEBUG, "Mounter thread finished with status %x", m_result);
}

///////////////////////////////////////////////////////////////////////////////
// GuestFS

PRL_RESULT GuestFS::addDrive(const CVmHardDisk &hardDisk, bool readOnly)
{
	// m_curDevice = "sdX";
	if (m_curDevice[m_curDevice.length() - 1] > 'z') {
		WRITE_TRACE(DBG_FATAL, "Too many disks");
		return PRL_ERR_FAILURE;
	}
	if ((readOnly && guestfs_add_drive_ro(
					m_gfsHandle, QSTR2UTF8(hardDisk.getUserFriendlyName()))) ||
		(!readOnly && guestfs_add_drive(
					m_gfsHandle, QSTR2UTF8(hardDisk.getUserFriendlyName())))) {
		return PRL_ERR_FAILURE;
	}
	m_deviceToHdd[m_curDevice] = hardDisk.getIndex();
	m_hddToPath[hardDisk.getIndex()] = hardDisk.getUserFriendlyName();
	// "sda" -> "sdb" etc.
	m_curDevice[m_curDevice.length() - 1] = QChar(
			m_curDevice[m_curDevice.length() - 1].toLatin1() + 1);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT GuestFS::launch() const
{
	if (guestfs_launch(m_gfsHandle)) {
		WRITE_TRACE(DBG_FATAL, "Failed to launch guestfs appliance");
		return PRL_ERR_FAILURE;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT GuestFS::getMountablePartitions(QStringList &partitions)
{
	partitions.clear();
	char **fs = guestfs_list_filesystems(m_gfsHandle);
	if (fs == NULL) {
		// Error
		WRITE_TRACE(DBG_FATAL, "Failed to list filesystems");
		return PRL_ERR_FAILURE;
	}

	for (char **cur = fs; *cur != NULL; cur += 2) {
		// Mount only partitions and LV, not e.g. snapshots
		if (QString(*cur).startsWith("/dev/"))
		{
			QString fs(*(cur + 1));
			m_partToFilesystem[*cur] = fs;
			if (fs != "unknown" && fs != "swap")
				partitions << *cur;
		}
		free(*cur);
		free(*(cur + 1));
	}
	free(fs);

	if (partitions.length() == 0) {
		WRITE_TRACE(DBG_FATAL, "No mountable partitions found");
		return PRL_ERR_FAILURE;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT GuestFS::createMountpoint(const QString &partition)
{
	// /dev/sdxN
	QRegExp partRE("^/dev/(sd[a-z])([\\d+])$");
	// /dev/vg/lv
	QRegExp lvRE("^/dev/([^/]+)/([^/]+)$");
	QString mountpoint;
	if (partRE.indexIn(partition) != -1) {
		QMap<QString, int>::const_iterator it;
		if ((it = m_deviceToHdd.find(partRE.cap(1))) == m_deviceToHdd.end()) {
			// The device we have not added?
			Q_ASSERT(false);
			return PRL_ERR_FAILURE;
		}
		mountpoint = QString("/hdd%1/volume%2").arg(*it).arg(partRE.cap(2));
	} else if (lvRE.indexIn(partition) != -1) {
		mountpoint = QString("/%1/%2").arg(lvRE.cap(1), lvRE.cap(2));
	} else {
		Q_ASSERT(false);
		return PRL_ERR_FAILURE;
	}

	m_partToMountpoint[partition] = mountpoint;
	return makeMountpoint(mountpoint);
}

PRL_RESULT GuestFS::makeMountpoint(const QString &mountpoint)
{
	QStringList parts = mountpoint.split("/", QString::SkipEmptyParts);
	QString currentPath;
	// /hdd0 ; /hdd0/volume1 etc.
	for (int i = 0; i < parts.length(); ++i) {
		currentPath += "/" + parts[i];
		if (!m_createdMountpoints.contains(currentPath)) {
			if (guestfs_mkmountpoint(m_gfsHandle, QSTR2UTF8(currentPath))) {
				WRITE_TRACE(DBG_FATAL, "Unable to make mountpoint '%s'",
							QSTR2UTF8(currentPath));
				return PRL_ERR_FAILURE;
			}
			m_createdMountpoints.insert(currentPath);
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT GuestFS::mountPartition(const QString &partition)
{
	PRL_RESULT res;
	if (PRL_FAILED(res = createMountpoint(partition)))
		return res;

	if (guestfs_mount(m_gfsHandle, QSTR2UTF8(partition),
					  QSTR2UTF8(m_partToMountpoint[partition]))) {
		WRITE_TRACE(DBG_FATAL, "Unable to mount filesystem '%s' at '%s'",
					QSTR2UTF8(partition), QSTR2UTF8(m_partToMountpoint[partition]));
		return PRL_ERR_FAILURE;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT GuestFS::mountLocal(const QString &mountpoint, bool readOnly)
{
	if (guestfs_mount_local(m_gfsHandle, QSTR2UTF8(mountpoint),
							GUESTFS_MOUNT_LOCAL_READONLY, (int)readOnly,
							GUESTFS_MOUNT_LOCAL_OPTIONS, "allow_other,kernel_cache",
							-1)) {
		WRITE_TRACE(DBG_FATAL, "Unable to local-mount guestfs filesystem");
		return PRL_ERR_FAILURE;
	}
	m_localMountpoint = mountpoint;
	return PRL_ERR_SUCCESS;
}

void GuestFS::umountLocal()
{
	guestfs_umount_local(m_gfsHandle, -1);
}

PRL_RESULT GuestFS::getMountInfo(const QString &partition, QString &info) const
{
	if (m_localMountpoint.isEmpty() || !m_partToMountpoint.contains(partition))
		return PRL_ERR_FAILURE;

	// Image path is undefined for LVs
	QString path("N/A");
	// /dev/sdxN
	QRegExp partRE("^/dev/(sd[a-z])([\\d+])$");
	if (partRE.indexIn(partition) != -1) {
		QMap<QString, int>::const_iterator it;
		if ((it = m_deviceToHdd.find(partRE.cap(1))) == m_deviceToHdd.end()) {
			// The device we have not added?
			Q_ASSERT(false);
			return PRL_ERR_FAILURE;
		}
		path = m_hddToPath[it.value()];
	}

	struct guestfs_statvfs *stat = guestfs_statvfs(
			m_gfsHandle, QSTR2UTF8(m_partToMountpoint[partition]));
	if (!stat)
		return PRL_ERR_FAILURE;

	// TODO: Total/free space.
	info = Parallels::formatMountInfo(
			m_partToMountpoint[partition], path,
			QDir::cleanPath(m_localMountpoint + '/' + m_partToMountpoint[partition]),
			m_partToFilesystem[partition],
			stat->blocks * stat->frsize, stat->bfree * stat->frsize);

	guestfs_free_statvfs(stat);
	return PRL_ERR_SUCCESS;
}

void GuestFS::close()
{
	if (m_gfsHandle)
		guestfs_close(m_gfsHandle);
	m_gfsHandle = NULL;
}

void GuestFS::shutdown()
{
	if (m_gfsHandle) {
		if (guestfs_shutdown(m_gfsHandle))
			WRITE_TRACE(DBG_FATAL, "guestfs_shutdown failed");
		close();
	}
}
///////////////////////////////////////////////////////////////////////////////
// CDspVmMount

PRL_RESULT CDspVmMount::umount()
{
	PRL_RESULT ret;
	if (PRL_FAILED(ret = releaseMountpoint(m_mountPoint)))
		return ret;

	cleanup();
	return PRL_ERR_SUCCESS;
}

void CDspVmMount::umountForce()
{
	if (PRL_FAILED(releaseMountpoint(m_mountPoint))) {
		WRITE_TRACE(DBG_FATAL, "Failed to release mountpount %s",
		            QSTR2UTF8(m_mountPoint));
	}

	cleanup();
}

PRL_RESULT CDspVmMount::releaseMountpoint(const QString &mountpoint)
{
	QString cmdLine = QString("%1 --no-retry %2").arg(GUESTUNMOUNT, mountpoint), out;
	// It may hang.
	QProcess proc;
	if (!HostUtils::RunCmdLineUtility(cmdLine, out, -1, &proc)) {
		WRITE_TRACE(DBG_FATAL, "Cannot unmount filesystem: %d", proc.exitCode());
		// Check if dir is mounted (buggy, not working for now!)
		if (proc.exitCode() == GFS_NOT_MOUNTED)
			return PRL_ERR_SUCCESS;

		// Workaround for bug in libguestfs: maybe dir is not mounted at all?
		QStringList args;
		args << FINDMNT << "-n" << mountpoint << "-o" << "FSTYPE";
		if (!HostUtils::RunCmdLineUtility(args.join(" "), out) ||
			out.split("\n")[0].trimmed() != "fuse")
			return PRL_ERR_SUCCESS;

		return PRL_ERR_FAILURE;
	}
	// Successful unmount or not mounted.
	return PRL_ERR_SUCCESS;
}

void CDspVmMount::onThreadFinished()
{
	// Remove from registry. Mounter will be destructed.
	emit mountFinished(m_ident.first);
}

void CDspVmMount::cleanup()
{
	if (isActive() && !m_thread.wait(5 * 1000)) {
		WRITE_TRACE(DBG_FATAL, "FUSE loop did not exit in time");
		m_thread.terminate();
		m_thread.wait();
	}
	GuestFS(m_thread.getGfsHandle()).shutdown();
}


namespace
{

/* Local-mount vm partitions with filesystems.
 * If something failed, cleanup is performed.
 * You must NOT call umount() in this case.
 */
Prl::Expected<SmartPtr<CDspVmMount>, PRL_RESULT> mountVm(
		const SmartPtr<CVmConfiguration> &pVmConfig,
		const QString &dirUuid, const QString &mountPoint, bool readOnly)
{
	guestfs_h *gfsHandle = guestfs_create();
	if (!gfsHandle)
		return PRL_ERR_FAILURE;

	GuestFS gfs(gfsHandle);
	PRL_RESULT res;

	do {
		CVmHardware* pHwList = pVmConfig->getVmHardwareList();
		foreach(const CVmHardDisk *pHardDisk, pHwList->m_lstHardDisks) {
			if (PRL_FAILED(res = gfs.addDrive(*pHardDisk, readOnly)))
				break;
		}

		if (PRL_FAILED(res = gfs.launch()))
			break;

		QStringList partitions;
		if (PRL_FAILED(res = gfs.getMountablePartitions(partitions)))
			break;

		foreach(const QString &partition, partitions) {
			if (PRL_FAILED(res = gfs.mountPartition(partition)))
				break;
		}

		if (PRL_FAILED(res = gfs.mountLocal(mountPoint, readOnly)))
			break;

		QStringList infos;
		foreach(const QString &partition, partitions) {
			QString info;
			if (PRL_FAILED(res = gfs.getMountInfo(partition, info))) {
				// If we just close the handle, local mountpoint will
				// throw 'endpoint is not connected'.
				gfs.umountLocal();
				break;
			}
			infos << info;
		}
		QString mountInfo = infos.join("\n");

		return SmartPtr<CDspVmMount>(new CDspVmMount(
					MakeVmIdent(pVmConfig->getVmIdentification()->getVmUuid(),dirUuid),
					mountPoint, mountInfo, gfsHandle));
	} while (0);

	gfs.shutdown();
	return res;
}

void updateState(const CVmIdent &ident,
                 VIRTUAL_MACHINE_STATE before,
                 VIRTUAL_MACHINE_STATE after)
{
	CDspLockedPointer<CDspVmStateSender> sender =
		CDspService::instance()->getVmStateSender();
	if (!sender)
		return;

	VIRTUAL_MACHINE_STATE state = sender->tell(ident);
	if (state == before)
		sender->onVmStateChanged(state, after, ident.first, ident.second, false);
}

PRL_RESULT lockVm(const CVmIdent &ident)
{
	SmartPtr<CDspClient> fakeClient = CDspClient::makeServiceUser(ident.second);
	return CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			ident.first, ident.second, PVE::DspCmdVmMount, fakeClient);
}

void unlockVm(const CVmIdent &ident)
{
	SmartPtr<CDspClient> fakeClient = CDspClient::makeServiceUser(ident.second);
	CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			ident.first, ident.second, PVE::DspCmdVmMount, fakeClient);
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// CDspVmMountStorage

SmartPtr<CDspVmMount> CDspVmMountStorage::beginPop(const QString &uuid)
{
	QMutexLocker locker(&m_storageLock);
	SmartPtr<CDspVmMount> mount = m_storage.take(uuid);
	if (mount)
		m_transStorage.insert(uuid, mount);
	return mount;
}

bool CDspVmMountStorage::rollbackPop(const QString &uuid)
{
	QMutexLocker locker(&m_storageLock);
	if (!m_transStorage.contains(uuid) ||
		m_storage.contains(uuid))
		return false;
	SmartPtr<CDspVmMount> mount = m_transStorage.take(uuid);
	m_storage.insert(uuid, mount);
	return true;
}

bool CDspVmMountStorage::commitPop(const QString &uuid)
{
	QMutexLocker locker(&m_storageLock);
	return m_transStorage.remove(uuid);
}

bool CDspVmMountStorage::insert(
		const QString &uuid, const SmartPtr<CDspVmMount> &mount)
{
	QMutexLocker locker(&m_storageLock);
	m_storage.insert(uuid, mount);
	if (!mount->isActive()) {
		m_storage.remove(uuid);
		return false;
	}
	return true;
}

QString CDspVmMountStorage::getInfo(const QString &uuid) const
{
	QMutexLocker locker(&m_storageLock);
	if (m_storage.contains(uuid))
		return m_storage[uuid]->getMountInfo();
	return "VM is not mounted\n";
}

QList<QString> CDspVmMountStorage::getKeys() const
{
	QMutexLocker locker(&m_storageLock);
	return m_storage.keys();
}

///////////////////////////////////////////////////////////////////////////////
// CDspVmMountRegistry

CDspVmMountRegistry::CDspVmMountRegistry()
{
	m_handler.moveToThread(&m_handlerThread);
	PRL_ASSERT(QObject::connect(
			&m_handler,
			SIGNAL(removeMount(const QString&)),
			this,
			SLOT(umountForce(const QString&)),
			Qt::DirectConnection));
	m_handlerThread.start();
}

Libvirt::Result CDspVmMountRegistry::mount(
		const SmartPtr<CVmConfiguration> &pVmConfig,
		const QString &dirUuid, const QString &mountPoint,
		bool readOnly)
{
	QString uuid = pVmConfig->getVmIdentification()->getVmUuid();
	CVmIdent ident = MakeVmIdent(uuid, dirUuid);

	PRL_RESULT lockResult = lockVm(ident);
	if (PRL_FAILED(lockResult)) {
		WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
			__FUNCTION__, lockResult, PRL_RESULT_TO_STRING(lockResult));
		return Error::Simple(PRL_ERR_VM_MOUNT, "VM is already mounted");
	}

	Error::Simple err(PRL_ERR_SUCCESS);
	do {
		Prl::Expected<SmartPtr<CDspVmMount>, PRL_RESULT> res = mountVm(
				pVmConfig, dirUuid, mountPoint, readOnly);
		if (res.isFailed()) {
			err = Error::Simple(PRL_ERR_VM_MOUNT);
			break;
		}
		if (!startTracking(uuid, res.value())) {
			err = Error::Simple(PRL_ERR_VM_MOUNT, "Mount died unexpectedly");
			break;
		}
		return Libvirt::Result();
	} while (0);

	unlockVm(ident);
	return err;
}

Libvirt::Result CDspVmMountRegistry::umount(const QString &uuid)
{
	SmartPtr<CDspVmMount> mount = m_storage.beginPop(uuid);
	if (!mount) {
		WRITE_TRACE(DBG_FATAL, "VM is not mounted");
		return Error::Simple(PRL_ERR_VM_UNMOUNT, "VM is not mounted");
	}

	PRL_RESULT res = mount->umount();
	if (PRL_FAILED(res)) {
		m_storage.rollbackPop(uuid);
		return Error::Simple(PRL_ERR_VM_UNMOUNT);
	}

	stopTracking(uuid, mount);
	unlockVm(mount->getVmIdent());
	return Libvirt::Result();
}

void CDspVmMountRegistry::umountForce(const QString &uuid)
{
	SmartPtr<CDspVmMount> mount = m_storage.beginPop(uuid);
	if (!mount)
		return;

	WRITE_TRACE(DBG_FATAL, "Removing mount for %s", QSTR2UTF8(uuid));
	mount->umountForce();
	stopTracking(uuid, mount);
	unlockVm(mount->getVmIdent());
}

bool CDspVmMountRegistry::startTracking(
		const QString &uuid, const SmartPtr<CDspVmMount> &mount)
{
	PRL_ASSERT(QObject::connect(
				mount.get(),
				SIGNAL(mountFinished(const QString&)),
				&m_handler,
				SLOT(onMountFinished(const QString&)),
				Qt::QueuedConnection));

	if (!m_storage.insert(uuid, mount)) {
		PRL_ASSERT(QObject::disconnect(
				mount.get(),
				SIGNAL(mountFinished(const QString&)),
				&m_handler,
				SLOT(onMountFinished(const QString&))));
		return false;
	}
	updateState(mount->getVmIdent(), VMS_STOPPED, VMS_MOUNTED);
	return true;
}

bool CDspVmMountRegistry::stopTracking(
		const QString &uuid, const SmartPtr<CDspVmMount> &mount)
{
	if (!m_storage.commitPop(uuid))
		return false;

	PRL_ASSERT(QObject::disconnect(
			mount.get(),
			SIGNAL(mountFinished(const QString&)),
			&m_handler,
			SLOT(onMountFinished(const QString&))));

	updateState(mount->getVmIdent(), VMS_MOUNTED, VMS_STOPPED);
	return true;
}

void CDspVmMountRegistry::umountAll()
{
	WRITE_TRACE(DBG_FATAL, "Unmounting all VMs");
	foreach(const QString &uuid, m_storage.getKeys()) {
		umountForce(uuid);
	}
}

CDspVmMountRegistry::~CDspVmMountRegistry()
{
	umountAll();
	WRITE_TRACE(DBG_FATAL, "CDspVmMountRegistry destroyed");
	m_handlerThread.quit();
	m_handlerThread.wait();
	PRL_ASSERT(QObject::disconnect(
			&m_handler,
			SIGNAL(removeMount(const QString&)),
			this,
			SLOT(umountForce(const QString&))));
}

