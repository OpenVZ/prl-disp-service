///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_AttachVmBackup.cpp
///
/// Source and target tasks for attaching a VM backup as a block device
///
/// @author ibazhitov@
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
///////////////////////////////////////////////////////////////////////////////

#include "Task_CommonHeaders.h"
#include "Task_AttachVmBackup.h"
#include "Task_AttachVmBackup_p.h"

#include "CDspService.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/BackupTree.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"
//#include "Libraries/VirtualDisk/PrlDiskDescriptor.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "prlcommon/IOService/IOCommunication/Socket/Socket_p.h"
#include "Libraries/Buse/Buse.h"
#include "Mixin_CreateHddSupport.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

/**
 * Convert BUSE filesystem specific errors to this task specific errors
 *
 * @param res - error code
 * @return PRL_RESULT error code
 */
PRL_RESULT fromBuseError(PRL_RESULT res)
{
	if (res == PRL_ERR_SUCCESS || res == PRL_ERR_OUT_OF_MEMORY)
		return res;
	if (res == PRL_ERR_BUSE_NOT_MOUNTED)
		return PRL_ERR_ATTACH_BACKUP_BUSE_NOT_MOUNTED;
	if (res == PRL_ERR_BUSE_ENTRY_ALREADY_EXIST)
		return PRL_ERR_ATTACH_BACKUP_ALREADY_ATTACHED;
	return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
}

/**
 * Get the path to the ploop wrapper directory inside virtual HDD bundle
 *
 * @param path - full path to a HDD bundle
 * @return path to the ploop wrapper directory
 */
QString getPloopWrapperPath(const QString& path)
{
	return QDir(path).filePath("ploop-wrapper.hdd");
}

/**
 * Make a name for a virtual HDD bundle
 *
 * @param auth - auth helper
 * @param dir - path to a directory, where the virtual HDD bundle will be created
 * @return name of a virtual HDD bundle
 */
QString makeDiskName(CAuthHelper *auth, const QString& dir)
{
	QString res;
	int i = 1;
	do {
		res = QString("%1/backup%2.hdd").arg(dir).arg(i++);
	} while (CFileHelper::DirectoryExists(res, auth));
	return res;
}

/**
 * Create an empty virtual HDD bundle on disk
 *
 * @param auth - auth helper
 * @param path - full path to the virtual HDD bundle
 * @return PRL_RESULT error code
 */
PRL_RESULT createDiskBundle(CAuthHelper *auth, const QString& path)
{
	if (!CFileHelper::WriteDirectory(path, auth)) {
		WRITE_TRACE(DBG_FATAL, "failed to create directory '%s'", QSTR2UTF8(path));
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	if (PRL_FAILED(CFileHelper::ChangeDirectoryPermissions(CFileHelper::GetFileRoot(path), path, auth))) {
		WRITE_TRACE(DBG_FATAL, "failed to set permissions for directory '%s'", QSTR2UTF8(path));
		CFileHelper::ClearAndDeleteDir(path);
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

PRL_VM_TYPE getVmType(const QString& uuid)
{
	PRL_VM_TYPE res = PVT_VM;
	CDspService::instance()->getVmDirManager().getVmTypeByUuid(uuid, res);
	return res;
}

} // anonymous namespace

namespace Attach {
namespace Wrap {

/**
 * Constructor with parameters
 *
 * @param path - full path to a directory which (will) contain DiskDescriptor.xml
 */
Ploop::Ploop(const QString& path) : m_path(path), m_ploop(path)
{
}

/**
 * Get the path to a disk image file
 *
 * @return full path to a disk image file
 */
QString Ploop::getImage() const
{
	if (m_image.isEmpty()) {
		PloopImage::DiskDescriptor dd;
		if (PRL_SUCCEEDED(m_ploop.getDiskDescriptor(dd))) {
			m_image = dd.images[0].file;
		} else {
			WRITE_TRACE(DBG_FATAL, "failed to read disk descriptor from '%s'",
				QSTR2UTF8(m_path));
		}
	}
	return m_image;
}

/**
 * Create ploop wrapper from image
 *
 * @param path - full path to a disk image file
 * @return PRL_RESULT error code
 */
PRL_RESULT Ploop::setImage(const QString& path)
{
	PRL_RESULT res = m_ploop.createDiskDescriptor(path, QFileInfo(path).size());
	if (PRL_FAILED(res))
		return res;
	/* to be able to replay and save FS journal, we need the image to be writable
	 * thus create a rw snapshot over a ro image file */
	res = m_ploop.createSnapshot(m_path);
	if (PRL_SUCCEEDED(res)) {
		/* set 256Mb limit by default */
		if (PRL_FAILED(m_ploop.setActiveDeltaLimit(512 * 1024)))
			WRITE_TRACE(DBG_WARNING, "failed to set active delta limit for "
				"a ploop device in '%s'", QSTR2UTF8(m_path));
	}
	m_image = path;
	return res;
}

/**
 * Destroy the ploop wrapper
 *
 * @return PRL_RESULT error code
 */
void Ploop::destroy() const
{
	CFileHelper::ClearAndDeleteDir(m_path);
}

/**
 * Connect the ploop wrapper to a block device
 *
 * @param[out] dev - path to the created block device file
 * @return PRL_RESULT error code
 */
PRL_RESULT Ploop::mount(QString& dev) const
{
	return PRL_SUCCEEDED(m_ploop.mount(dev))
		? PRL_ERR_SUCCESS : PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
}

/**
 * Disconnect the ploop wrapper from a block device
 *
 * @return PRL_RESULT error code
 */
PRL_RESULT Ploop::umount() const
{
	return PRL_SUCCEEDED(m_ploop.umount())
		? PRL_ERR_SUCCESS : PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
}

/**
 * Get the block device to which the ploop wrapper is attached
 *
 * @param[out] dev - path to a block device file, or empty if not mounted
 * @return PRL_RESULT error code
 */
PRL_RESULT Ploop::getMountedDevice(QString& dev) const
{
	return PRL_SUCCEEDED(m_ploop.getMountedDevice(dev))
		? PRL_ERR_SUCCESS : PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
}

VirtualDisk::VirtualDisk(const QString& path)
	: m_path(QDir(path).filePath("__" + QFileInfo(path).fileName().remove(QRegExp("[aeiouy]"))))
{
}

/**
 * Create virtual disk wrapped over a block device
 *
 * @param dev - path to a block device
 * @param auth - auth helper
 * @return PRL_RESULT error code
 */
PRL_RESULT VirtualDisk::create(const QString& dev, CAuthHelper *auth)
{
	CVmEvent e;
	return PRL_FAILED(Mixin_CreateHddSupport::ConfigurePhysical(m_path, dev, auth, &e))
		? PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR : PRL_ERR_SUCCESS;
}

/**
 * Destroy virtual disk
 */
void VirtualDisk::destroy()
{
	CFileHelper::ClearAndDeleteDir(m_path);
}

/**
 * Obtain a path to a block device which is used as an image
 *
 * @param path - path to a block device
 * @return PRL_RESULT error code
 */
PRL_RESULT VirtualDisk::getDevice(QString& path)
{
/*
	PrlDiskDescriptor dd;
	IDiskDescriptor::DiskInfo di;
	PRL_RESULT res = dd.OpenDescriptor(m_path, PRL_DISK_READ
		| PRL_DISK_NO_ERROR_CHECKING | PRL_DISK_FAKE_OPEN, di);
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "failed to open virtual disk descriptor %s: %d",
				QSTR2UTF8(m_path), res);
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	path = di.m_StoragesInfo.front().m_ImageInfoList.front().m_ImageName;
	dd.CloseDescriptor();
*/
	Q_UNUSED(path);
	return PRL_ERR_SUCCESS;
}

/**
 * Store a path to a block device which is used as an image
 *
 * @param path - path to a block device
 * @return PRL_RESULT error code
 */
PRL_RESULT VirtualDisk::setDevice(const QString& path)
{
/*
	PrlDiskDescriptor dd;
	IDiskDescriptor::DiskInfo di;
	PRL_RESULT res = dd.OpenDescriptor(m_path, PRL_DISK_DEFAULT_FLAG
		| PRL_DISK_XML_CHANGE, di);
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "failed to open virtual disk descriptor %s: %d",
				QSTR2UTF8(m_path), res);
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	IDiskDescriptor::DiskImageInfo ii = di.m_StoragesInfo.front().m_ImageInfoList.front();
	ii.m_ImageName = path;
	res = dd.SetImageFileName(ii);
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "failed to set image path in virtual disk descriptor %s: %d",
				QSTR2UTF8(m_path), res);
		dd.CloseDescriptor();
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	res = dd.SaveDescriptor();
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "failed to save virtual disk descriptor %s: %d",
				QSTR2UTF8(m_path), res);
	}
	dd.CloseDescriptor();
*/
	Q_UNUSED(path);
	PRL_RESULT res = PRL_ERR_UNIMPLEMENTED;
	return PRL_SUCCEEDED(res) ? PRL_ERR_SUCCESS : PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
}

/**
 * Constructor with params
 *
 * @param path - full path to the read only image file
 */
Image::Image(const QString& path)
{
	Buse::Buse buse;
	m_mountpoint = buse.mountpoint();
	/* XXX: here we rely on the fact that getPath() returns full path to content
	 * file inside the BUSE entry directory, so we could simply get the name of
	 * image's directory */
	m_name = Buse::Buse::getEntryName(path);
}

/**
 * Constructor with params
 *
 * @param vmUuid - VM UUID to which the backup will be attached
 * @param backupId - backup ID in the form: "{UUID}[.pit]"
 * @param diskName - name of disk backup
 */
Image::Image(const QString& vmUuid, const QString& backupId,
	const QString& diskName)
	: m_mountpoint(Buse::Buse().mountpoint())
{
	m_name = QString("%1_%2_%3").arg(vmUuid, CFileHelper::ReplaceNonValidPathSymbols(backupId), diskName);
}

QString Image::getPath() const
{
	return Buse::Entry(m_name, m_mountpoint).content();
}

bool Image::exists() const
{
	return PRL_SUCCEEDED(Buse::Buse(m_mountpoint).exists(m_name));
}

PRL_RESULT Image::remove() const
{
	return fromBuseError(Buse::Buse(m_mountpoint).remove(m_name));
}

/**
 * Constructor with params
 *
 * @param vmUuid - VM UUID to which the disk is attached
 * @param type - type of VM
 * @param path - path to the virtual HDD bundle
 * @param ploop - ploop wrapper
 */
Hdd::Hdd(const QString& vmUuid, PRL_VM_TYPE type, const QString& path, Ploop *ploop)
	: m_type(type), m_vmUuid(vmUuid), m_ploop(ploop), m_path(path),
	  m_image(ploop->getImage())
{
}

/**
 * Get the read-only image wrapper
 *
 * @return the read-only image wrapper
 */
Image& Hdd::getImage()
{
	return m_image;
}

/**
 * Save virtual disk parameters
 *
 * @param disk - where to save parameters
 * @return PRL_RESULT error code
 */
PRL_RESULT Hdd::save(CVmHardDisk& disk)
{
	if (m_type == PVT_CT) {
		disk.setEmulatedType(PDT_USE_IMAGE_FILE);
		disk.setUserFriendlyName(m_ploop->getPath());
		return PRL_ERR_SUCCESS;
	}

	disk.setEmulatedType(PDT_USE_REAL_HDD);
	QString path = VirtualDisk(m_path).getPath();
	disk.setSystemName(path);
	disk.setUserFriendlyName(path);
	return PRL_ERR_SUCCESS;
}

/**
 * Temporarily disable the virtual hard disk
 *
 * @return PRL_RESULT error code
 */
PRL_RESULT Hdd::disable()
{
	if (m_type == PVT_CT)
		return PRL_ERR_SUCCESS;
	m_ploop->umount();
	return PRL_ERR_SUCCESS;
}

/**
 * Destroy the virtual hard disk files
 *
 * @return PRL_RESULT error code
 */
PRL_RESULT Hdd::destroy()
{
	m_ploop->destroy();
	if (m_type == PVT_VM)
		VirtualDisk(m_path).destroy();
	CFileHelper::ClearAndDeleteDir(m_path);
	return PRL_ERR_SUCCESS;
}

/**
 * Enable an already existing virtual hard disk
 *
 * @return PRL_RESULT error code
 */
PRL_RESULT Hdd::enable()
{
	if (m_type == PVT_CT)
		return PRL_ERR_SUCCESS;
	VirtualDisk vd(m_path);
	QString savedDev;
	PRL_RESULT res;
	if (PRL_FAILED(res = vd.getDevice(savedDev)))
		return res;
	if (QFileInfo(savedDev).exists()) {
		/* check that the existing ploop device actually sits on top of image */
		QString dev;
		if (PRL_FAILED(res = m_ploop->getMountedDevice(dev)))
			return res;
		if (dev == savedDev)
			return PRL_ERR_SUCCESS;
	}

	QString dev;
	res = m_ploop->mount(dev);
	if (PRL_FAILED(res))
		return res;
	if (dev != savedDev) {
		res = vd.setDevice(dev);
		if (PRL_FAILED(res)) {
			m_ploop->umount();
			return res;
		}
	}
	return PRL_ERR_SUCCESS;
}

Hdd *Mixin::getResult(const QString& uuid, PRL_VM_TYPE type)
{
	if (m_path.isEmpty() || !m_ploop.get())
		return NULL;
	return new(std::nothrow) Hdd(uuid, type, m_path, m_ploop.release());
}

namespace Init {

Vm::Vm(const CVmHardDisk& disk, const QString& home) : m_home(home)
{
	QDir vd(disk.getSystemName());
	if (!vd.exists() || !vd.cdUp()) {
		WRITE_TRACE(DBG_FATAL, "invalid virtual disk directory '%s'", QSTR2UTF8(vd.path()));
		return;
	}
	QString path = vd.path();
	m_path = QFileInfo(path).isAbsolute() ? path : QDir(m_home).filePath(path);
}

QString Vm::getPloop() const
{
	return getPloopWrapperPath(m_path);
}

template <class T>
PRL_RESULT Builder<T>::setPath()
{
	m_path = m_policy.getPath();
	if (m_path.isEmpty())
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;

	return PRL_ERR_SUCCESS;
}

template <class T>
PRL_RESULT Builder<T>::setPloop()
{
	m_ploop.reset();
	QString path = m_policy.getPloop();
	if (path.isEmpty())
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;

	std::auto_ptr<Ploop> ploop(new(std::nothrow) Ploop(path));
	if (!ploop.get())
		return PRL_ERR_OUT_OF_MEMORY;
	if (ploop->getImage().isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "ploop wrapper image is not set");
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	m_ploop = ploop;
	return PRL_ERR_SUCCESS;
}

template <class T>
PRL_RESULT Builder<T>::prepareDevice()
{
	PRL_RESULT res;
	QString dev;
	if (PRL_FAILED(res = VirtualDisk(m_path).getDevice(dev)))
		return res;
	if (dev.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "virtual disk device is not set");
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

} // namespace Init

namespace Create
{

PRL_RESULT Common::setPloop()
{
	m_ploop.reset();
	if (m_image.isEmpty())
		return PRL_ERR_ATTACH_BACKUP_BUSE_NOT_MOUNTED;
	std::auto_ptr<Ploop> ploop(new(std::nothrow) Ploop(m_wrapper));
	if (!ploop.get())
		return PRL_ERR_OUT_OF_MEMORY;

	PRL_RESULT res;
	if (PRL_FAILED(res = ploop->setImage(m_image)))
		return res;
	m_ploop = ploop;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Common::prepareDevice()
{
	PRL_RESULT res;
	QString dev;
	if (PRL_FAILED(res = m_ploop->mount(dev))) {
		m_ploop->destroy();
		return res;
	}
	if (PRL_FAILED(res = VirtualDisk(m_path).create(dev, m_auth))) {
		m_ploop->umount();
		m_ploop->destroy();
		return res;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setPath()
{
	QString path = getPloopWrapperPath(m_path);
	PRL_RESULT res;
	if (PRL_FAILED(res = createDiskBundle(getAuth(), path)))
		return res;
	setWrapper(path);
	return PRL_ERR_SUCCESS;
}

} // namespace Create

Factory::Factory(const QString& uuid) : m_uuid(uuid), m_type(getVmType(uuid))
{
}

template<class T>
PRL_RESULT Factory::setResult(T builder)
{
	m_result.reset();
	if (m_uuid.isEmpty())
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	PRL_RESULT res;
	if (PRL_FAILED(res = builder.setPath()))
		return res;
	if (PRL_FAILED(res = builder.setPloop()))
		return res;
	if (m_type == PVT_VM && PRL_FAILED(res = builder.prepareDevice()))
		return res;

	m_result.reset(builder.getResult(m_uuid, m_type));
	if (m_result.isNull())
		return PRL_ERR_OUT_OF_MEMORY;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Factory::create(const CVmHardDisk& disk, const QString& vmHome)
{
	using namespace Init;
	switch(m_type)
	{
	case PVT_VM:
		return setResult(Builder<Vm>(Vm(disk, vmHome)));
	case PVT_CT:
		return setResult(Builder<Ct>(Ct(disk)));
	default:
		m_result.reset();
		return PRL_ERR_UNEXPECTED;
	}
}

PRL_RESULT Factory::create(const QString& backupId, const QString& diskName,
	const QString& dir, CAuthHelper *auth)
{
	QString path = makeDiskName(auth, dir);
	PRL_RESULT res = createDiskBundle(auth, path);
	if (PRL_FAILED(res))
		return res;

	QString image = Image(m_uuid, backupId, diskName).getPath();
	switch(m_type)
	{
	case PVT_VM:
		res = setResult(Create::Vm(path, image, *auth));
		break;
	case PVT_CT:
		res = setResult(Create::Ct(path, image));
		break;
	default:
		m_result.reset();
		res = PRL_ERR_UNEXPECTED;
	}
	if (PRL_FAILED(res))
		CFileHelper::ClearAndDeleteDir(path);
	return res;
}

} // namespace Wrap

namespace Source {

/**
 * Initialize backup info from string
 *
 * @param data - XML representation of backup metadata
 * @param e - where to save error event
 * @return PRL_RESULT error code
 */
PRL_RESULT BackupInfo::fromString(const QString& data, CVmEvent *e)
{
	do {
		BackupTree tree;
		QList<QString> disks;
		tree.fromString(data);
		if (tree.m_lstVmItem.size() != 1) {
			WRITE_TRACE(DBG_FATAL, "GetBackupTree reply contain %d VM UUIDs",
				tree.m_lstVmItem.size());
			break;
		}
		VmItem *vm = tree.m_lstVmItem[0];
		m_vmUuid = vm->getUuid();

		if (vm->m_lstBackupItem.size() != 1) {
			WRITE_TRACE(DBG_FATAL, "GetBackupTree reply contain %d backup UUIDs",
				vm->m_lstBackupItem.size());
			break;
		}

		BackupItem *backup = vm->m_lstBackupItem[0];
		m_uuid = backup->getUuid();
		QString id;
		if (backup->m_lstPartialBackupItem.size()) {
			if (backup->m_lstPartialBackupItem.size() != 1) {
				WRITE_TRACE(DBG_FATAL, "GetBackupTree reply contain %d partial backup UUIDs",
					backup->m_lstPartialBackupItem.size());
				break;
			}
			id = backup->m_lstPartialBackupItem[0]->getId();
			m_pit = backup->m_lstPartialBackupItem[0]->getNumber();
			if (!backup->m_lstPartialBackupItem[0]->getBackupDisks()) {
				WRITE_TRACE(DBG_FATAL, "GetBackupTree reply doesn't contain disk list");
				break;
			}
			foreach(CBackupDisk *disk, backup->m_lstPartialBackupItem[0]->getBackupDisks()->m_lstBackupDisks)
				disks << disk->getName();
		} else {
			id = backup->getId();
			if (!backup->getBackupDisks()) {
				WRITE_TRACE(DBG_FATAL, "GetBackupTree reply doesn't contain disk list");
				break;
			}
			foreach(CBackupDisk *disk, backup->getBackupDisks()->m_lstBackupDisks)
				disks << disk->getName();
		}
		if (id != m_id) {
			WRITE_TRACE(DBG_FATAL, "GetBackupTree reply contain backup id '%s' instead of '%s'",
				QSTR2UTF8(id), QSTR2UTF8(m_id));
			break;
		}
		if (disks.indexOf(m_diskName) == -1) {
			WRITE_TRACE(DBG_FATAL, "GetBackupTree reply doesn't contain disk '%s'",
				QSTR2UTF8(m_diskName));
			break;
		}
		return PRL_ERR_SUCCESS;
	} while (0);

	if (e) {
		e->setEventCode(PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND);
		e->addEventParameter(new CVmEventParameter(
			PVE::String, m_id, EVT_PARAM_MESSAGE_PARAM_0));
	}
	return PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
}

/**
 * Get the path to the backuped disk relatively to backups directory
 *
 * @return relative path to the backuped disk
 */
QString BackupInfo::getDiskPath() const
{
	return QString("%1/%2/%3").arg(m_vmUuid, m_uuid, m_diskName);
}

Resource::Resource() : m_port(CDspService::getDefaultListenPort())
{
}

/**
 * Set storage URL
 *
 * @param url - storage URL
 * @return PRL_RESULT error code
 */
PRL_RESULT Resource::setURL(const QUrl& url)
{
	/* backup://[server:port]/{BACKUP_UUID}.pit/diskname */
	/* empty "server:port" stands for localhost */
	if (!url.isValid()) {
		WRITE_TRACE(DBG_FATAL, "invalid storage URL '%s'", QSTR2UTF8(url.toString()));
		return PRL_ERR_ATTACH_BACKUP_INVALID_STORAGE_URL;
	}
	if (url.scheme() != "backup") {
		WRITE_TRACE(DBG_FATAL, "URL scheme '%s' is not supported", QSTR2UTF8(url.scheme()));
		return PRL_ERR_ATTACH_BACKUP_INVALID_STORAGE_URL;
	}
	m_hostname = url.host();

	if (!isLocal()) {
		WRITE_TRACE(DBG_INFO, "attaching backup located on a remote server (%s) "
			"is not implemented", QSTR2UTF8(m_hostname));
		return PRL_ERR_ATTACH_BACKUP_CUSTOM_BACKUP_SERVER_NOT_SUPPORTED;
	}

	if (url.port() != -1)
		m_port = url.port();
	QStringList path = url.path().split("/", QString::SkipEmptyParts);
	if (path.size() != 2) {
		WRITE_TRACE(DBG_FATAL, "invalid path to a backup in storage URL '%s'",
			QSTR2UTF8(url.toString()));
		return PRL_ERR_ATTACH_BACKUP_INVALID_STORAGE_URL;
	}
	m_backupId = path[0];
	m_diskId = path[1];
	return PRL_ERR_SUCCESS;
}

/**
 * Set credentials for a remote server from backup server preferences
 */
PRL_RESULT Resource::setCredentials()
{
	CDispBackupSourcePreferences *prefs = CDspService::instance()->getDispConfigGuard()
		.getDispCommonPrefs()->getBackupSourcePreferences();
	if (prefs->getLogin().isEmpty() || !prefs->isUsePassword())
		return PRL_ERR_BACKUP_REQUIRE_LOGIN_PASSWORD;
	m_login = prefs->getLogin();
	m_password = prefs->getPassword();
	return PRL_ERR_SUCCESS;
}

/**
 * Get the name of the server, where the backup is located
 *
 * @return hostname
 */
const QString& Resource::getHostname() const
{
	if (m_hostname.isEmpty())
		setDefaultHostname();
	return m_hostname;
}

/**
 * Is the resource located on the localhost or on a remote server
 *
 * @return true if resource is local, false - if remote
 */
bool Resource::isLocal() const
{
	return CDspService::instance()->getShellServiceHelper().isLocalAddress(getHostname());
}

/**
 * Set the default value for the server hostname from backup server preferences
 */
void Resource::setDefaultHostname() const
{
	if (m_hostname.isEmpty()) {
		WRITE_TRACE(DBG_INFO, "using localhost as a backup server");
		m_hostname = "127.0.0.1";
	}
}

/**
 * Constructor with params
 *
 * @param entry - BUSE entry to which a backup will be connected
 * @param backup - backup description
 */
Flavor::Flavor(Buse::Entry *entry, const BackupInfo& backup)
	: m_entry(entry), m_path(backup.getDiskPath()), m_format(0, backup.getPit())
{
}

/**
 * Attach a backup located on the local filesystem
 *
 * @param path - full path to the backups directory
 * @return PRL_RESULT error code
 */
PRL_RESULT Flavor::attachLocal(const QString& path)
{
	PRL_RESULT res;
	Buse::Local local(m_format, path);
	if (PRL_FAILED(res = m_entry->setParams(local.params()))
		|| PRL_FAILED(res = m_entry->setFormat(local.format()))
		|| PRL_FAILED(res = m_entry->setSource(local.source(m_path))))
		WRITE_TRACE(DBG_FATAL, "failed to initialize BUSE entry");
	return fromBuseError(res);
}

/**
 * Attach a backup located on a remote server
 *
 * @param client - connected IO client
 * @param task - parent task
 * @return PRL_RESULT error code
 */
PRL_RESULT Flavor::attachRemote(SmartPtr<IOClient> client, CDspTaskHelper *task)
{
	Q_UNUSED(client);
	Q_UNUSED(task);
	/* TODO:
	 * 1. trigger start of Task_ConnectVmBackupTarget and wait its answer
	 * 2. detach the disp-to-disp connection, established in Task_AttachVmBackupHelper::connect
	 * 3. send the detached connection to bused remote plugin via unix socket
	 * 4. wait for BUSE entry to be created
	 */
	WRITE_TRACE(DBG_FATAL, "attaching backup located on a remote server is not implemented yet");
	return PRL_ERR_ATTACH_BACKUP_CUSTOM_BACKUP_SERVER_NOT_SUPPORTED;
}

void Flavor::mangle()
{
	/* We need to prepend the actual disk data with at least 2 logical sectors
	 * (512 bytes/sector), filled with zeros, to mangle the MBR and the GPT header.
	 * Also, we need to append at least one zeroed sector to mangle the secondary
	 * GPT header. At the same time, a mangled image must be aligned on a 1Mb boundary
	 * (2048 sectors * 512 bytes/sector) since ploop wouldn't mount snapshots made
	 * for an unaligned image. */
	const unsigned int total = 2048, head = 1024;
	m_format.setHead(head);
	m_format.setTail(total - head);
}

} // namespace Source
} // namespace Attach

Task_AttachVmBackupHelper::Task_AttachVmBackupHelper(SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p)
	:Task_BackupHelper(client, p)
{
	m_nFlags = 0;
	m_nServerPort = 0;
}

template <class Cmd>
PRL_RESULT Task_AttachVmBackupHelper::prepareTask(Cmd *cmd)
{
	m_sVmUuid = cmd->GetVmUuid();

	if (!StringToElement<CVmHardDisk*>(&m_disk, cmd->GetDiskConfig())) {
		WRITE_TRACE(DBG_FATAL, "received invalid hard disk config");
		return PRL_ERR_INVALID_ARG;
	}

	PRL_RESULT res;
	if (PRL_FAILED(res = m_resource.setURL(m_disk.getStorageURL())))
		return res;
	if (m_resource.isLocal())
		m_resource.setSession(getActualClient()->getClientHandle());
	else
		res = m_resource.setCredentials();
	return res;
}

void Task_AttachVmBackupHelper::finalizeTask()
{
	/* XXX: This stub is needed to override CDspTaskHelper::finalizeTask(),
	 * which sends a response on success using getClient(). If this task is
	 * called from another task and we send an OK response from here, then
	 * the SDK code would think that the whole parent task had been completed
	 * and would prematurely return from PrlJob_Wait().
	 * See #PSBM-28855
	 */
}

/**
 * Fetch backup metadata from the backup server
 *
 * @param[out] result - where to save the backup metadata
 * @return PRL_RESULT error code
 */
PRL_RESULT Task_AttachVmBackupHelper::fetchBackupInfo(QString& result)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;
	m_nFlags |= PBT_VM | PBT_CT | PBT_BACKUP_ID;
	PRL_RESULT res;
	if (PRL_FAILED(res = GetBackupTreeRequest(m_resource.getBackupId(), result)))
		WRITE_TRACE(DBG_FATAL, "failed to retrieve the backup tree for backup '%s'",
			QSTR2UTF8(m_resource.getBackupId()));
	return res == PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR ? PRL_ERR_ATTACH_BACKUP_PROTO_ERROR : res;
}

PRL_RESULT Task_AttachVmBackupHelper::doMakeImage()
{
	PRL_RESULT res;
	QString b;
	if (PRL_FAILED(res = fetchBackupInfo(b)))
		return res;
	Attach::Source::BackupInfo backup(m_resource.getBackupId(), m_resource.getDiskId());
	if (PRL_FAILED(res = backup.fromString(b, getLastError())))
		return res;
	Buse::Buse buse;
	Attach::Wrap::Image image(m_sVmUuid, m_resource.getBackupId(), m_resource.getDiskId());
	res = buse.exists(image.getName());
	if (PRL_SUCCEEDED(res)) {
		/* Simultaneously mounting multiple ploops, that are backed
		 * by a single (even a read-only) image, is not allowed. */
		WRITE_TRACE(DBG_FATAL, "BUSE entry '%s' already exists", QSTR2UTF8(image.getName()));
		return PRL_ERR_ATTACH_BACKUP_ALREADY_ATTACHED;
	} else if (res != PRL_ERR_BUSE_ENTRY_INVALID) {
		return fromBuseError(res);
	}

	unsigned int osType = PVS_GUEST_TYPE_OTHER;
	PRL_VM_TYPE vmType = getVmType(m_sVmUuid);
	if (vmType == PVT_VM) {
		SmartPtr<CVmConfiguration> conf = CDspService::instance()->getVmDirHelper()
			.getVmConfigByUuid(getClient(), m_sVmUuid, res);
		if (!conf) {
			WRITE_TRACE(DBG_FATAL, "failed to get VM config by UUID");
			return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
		}
		osType = conf->getVmSettings()->getVmCommonOptions()->getOsType();
	}

	CVmEvent e;
	std::auto_ptr<Buse::Entry> entry(buse.create(image.getName(), &e));
	if (!entry.get())
		return fromBuseError(e.getEventCode());
	Attach::Source::Flavor f(entry.get(), backup);
	/* for Linux guests we need to mangle the MBR/GPT, so that the disk partitions
	 * would not be automatically processed by udev, see #PSBM-28706 */
	if (vmType == PVT_VM && osType == PVS_GUEST_TYPE_LINUX)
		f.mangle();
	if (m_resource.isLocal())
		res = f.attachLocal(getBackupDirectory());
	else
		res = f.attachRemote(getIoClient(), this);
	if (PRL_FAILED(res))
		buse.remove(entry->name());
	return res;
}

/**
 * Make a raw disk image from the backup
 *
 * @return PRL_RESULT error code
 */
PRL_RESULT Task_AttachVmBackupHelper::makeImage()
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;
	PRL_RESULT res;
	if (PRL_FAILED(res = Connect(m_resource.getHostname(), m_resource.getPort(),
		m_resource.getSession(), m_resource.getLogin(), m_resource.getPassword(), 0)))
		return res;
	res = doMakeImage();
	Disconnect();
	return res;
}

PRL_RESULT Task_AttachVmBackupHelper::getVmHome(QString& home)
{
	PRL_VM_TYPE type = getVmType(m_sVmUuid);
	CVmIdent ident;
	if (type == PVT_CT) {
		ident = MakeVmIdent(m_sVmUuid, CDspService::instance()->getVmDirManager().getVzDirectoryUuid());
	} else if (type == PVT_VM) {
		ident = getVmIdent();
	} else {
		WRITE_TRACE(DBG_FATAL, "can not handle VM type %d (uuid '%s')", type, QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_UNEXPECTED;
	}
	QString res = CDspVmDirManager::getVmHomeByUuid(ident);
	if (res.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Unable to find VM home by uuid '%s'", QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_ATTACH_BACKUP_INTERNAL_ERROR;
	}
	if (type == PVT_VM)
		res = CFileHelper::GetFileRoot(res);
	home = res;
	return PRL_ERR_SUCCESS;
}

Task_AttachVmBackup::Task_AttachVmBackup(SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p)
	: Task_AttachVmBackupHelper(client, p)
{
}

PRL_RESULT Task_AttachVmBackup::prepareTask()
{
	PRL_RESULT res;
	do {
		CDispToDispCommandPtr pcmd = CDispToDispProtoSerializer::ParseCommand(getRequestPackage());
		if (!pcmd->IsValid()) {
			WRITE_TRACE(DBG_FATAL, "received invalid package");
			res = PRL_ERR_INVALID_ARG;
			break;
		}

		CVmBackupAttachCommand *cmd = static_cast<CVmBackupAttachCommand *>(pcmd.get());
		if (PRL_FAILED(res = Task_AttachVmBackupHelper::prepareTask<CVmBackupAttachCommand>(cmd)))
			break;
		m_diskDir = cmd->GetDiskDir();
		if (m_diskDir.isEmpty()) {
			QString home;
			if (PRL_FAILED(res = getVmHome(home)))
				break;
			m_diskDir = home;
		}
	} while(0);
	setLastErrorCode(res);
	return res;
}

VIRTUAL_MACHINE_STATE Task_AttachVmBackup::getVmState()
{
	VIRTUAL_MACHINE_STATE state = VMS_UNKNOWN;
	switch (getVmType(m_sVmUuid)) {
	case PVT_VM:
		state = CDspVm::getVmState(m_sVmUuid, getClient()->getVmDirectoryUuid());
		break;
	case PVT_CT:
		CVzHelper::get_env_status(m_sVmUuid, state);
		break;
	}
	return state;
}

PRL_RESULT Task_AttachVmBackup::run_body()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;
	CAuthHelper *auth = &getClient()->getAuthHelper();

	do {
		if (PRL_FAILED(res = makeImage()))
			break;
		Attach::Wrap::Image image(m_sVmUuid, m_resource.getBackupId(),
			m_resource.getDiskId());
		Attach::Wrap::Factory f(m_sVmUuid);
		if (PRL_FAILED(res = f.create(m_resource.getBackupId(),
			m_resource.getDiskId(), m_diskDir, auth))) {
			image.remove();
			break;
		}
		m_hdd.reset(f.getResult());
		if (getVmState() != VMS_RUNNING) {
			m_hdd->disable();
			image.remove();
		}
	} while (0);
	setLastErrorCode(res);
	return res;
}

Task_ConnectVmBackupSource::Task_ConnectVmBackupSource(SmartPtr<CDspClient>& session,
	const SmartPtr<IOPackage>& p)
	: Task_AttachVmBackupHelper(session, p)
{
}

PRL_RESULT Task_ConnectVmBackupSource::prepareTask()
{
	PRL_RESULT res;
	do {
		CDispToDispCommandPtr pcmd = CDispToDispProtoSerializer::ParseCommand(getRequestPackage());
		if (!pcmd->IsValid()) {
			WRITE_TRACE(DBG_FATAL, "wrong package received");
			res = PRL_ERR_INVALID_ARG;
			break;
		}
		CVmBackupConnectSourceCommand *cmd = static_cast<CVmBackupConnectSourceCommand *>(pcmd.get());
		res = Task_AttachVmBackupHelper::prepareTask<CVmBackupConnectSourceCommand>(cmd);
	} while(0);
	setLastErrorCode(res);
	return res;
}

PRL_RESULT Task_ConnectVmBackupSource::run_body()
{
	PRL_RESULT res;

	do {
		if (!Attach::Wrap::Image(m_sVmUuid, m_resource.getBackupId(),
			m_resource.getDiskId()).exists()) {
			if (PRL_FAILED(res = makeImage()))
				break;
		}
		Attach::Wrap::Factory f(m_sVmUuid);
		QString home;
		if (PRL_FAILED(res = getVmHome(home)) || PRL_FAILED(res = f.create(m_disk, home))) {
			Attach::Wrap::Image(m_sVmUuid, m_resource.getBackupId(),
				m_resource.getDiskId()).remove();
			break;
		}
		QScopedPointer<Attach::Wrap::Hdd> hdd(f.getResult());
		if (PRL_FAILED(res = hdd->enable())) {
			hdd->getImage().remove();
			break;
		}
	} while (0);
	setLastErrorCode(res);
	return res;
}

void Task_ConnectVmBackupSource::finalizeTask()
{
	if (PRL_SUCCEEDED(getLastErrorCode()))
		return;
	if (getVmType(m_sVmUuid) != PVT_VM)
		return;
	QString path = m_disk.getSystemName();
	if (path.isEmpty())
		return;
	/* reset the block device path on failure, so that VM wouldn't use
	 * existing, but not mounted ploop device as disk image */
	Attach::Wrap::VirtualDisk vd("");
	vd.setPath(path);
	vd.setDevice("/dev/null");
}
