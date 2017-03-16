///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_AttachVmBackup.h
///
/// Source and target tasks for Vm backup as a block device attaching
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

#ifndef __Task_AttachVmBackup_H_
#define __Task_AttachVmBackup_H_

#include <memory>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "Task_BackupHelper.h"
#include "prlxmlmodel/DispConfig/CDispBackupSourcePreferences.h"
#include "Libraries/Virtuozzo/CVzPloop.h"

namespace Attach {
namespace Source {

/**
 * Backup metadata needed to setup a data source for HDD
 */
struct BackupInfo {
	/**
	 * Constructor with params
	 *
	 * @param id - backup ID in the form: "{UUID}[.pit]"
	 * @param diskName - name of disk backup
	 */
	BackupInfo(const QString& id, const QString& diskName)
		: m_id(id), m_diskName(diskName), m_pit(1), m_version(0)
	{
	}

	/**
	 * Get the incremental backup number (point in time)
	 *
	 * @return point in time
	 */
	unsigned int getPit() const
	{
		return m_pit;
	}

	unsigned int getVersion() const
	{
		return m_version;
	}

	const QString& getId() const
	{
		return m_id;
	}

	const QString& getDiskName() const
	{
		return m_diskName;
	}

	const CVmHddEncryption *getEncryption() const
	{
		return m_encryption.data();
	}

	PRL_RESULT fromString(const QString& data, CVmEvent *e = NULL);
	QString getDiskPath() const;

private:
	/** backup ID */
	QString m_id;
	/** name of the backuped disk */
	QString m_diskName;
	/** incremental backup number (point in time) */
	unsigned int m_pit;
	/** UUID of the backuped VM */
	QString m_vmUuid;
	/** backup UUID */
	QString m_uuid;
	/** backup version */
	unsigned int m_version;
	/** disk encryption parameters */
	QScopedPointer<CVmHddEncryption> m_encryption;
};

} // namespace Source

namespace Wrap {

/** Image wrapper */
struct Image {
	explicit Image(const QString& path);
	Image(const QString& vmUuid, const QString& backupId,
		const QString& diskName);
	QString getPath() const;
	bool exists() const;
	PRL_RESULT remove() const;

	QString getName() const
	{
		return m_name;
	}

private:
	/** full path to the BUSE mountpoint */
	QString m_mountpoint;
	/** BUSE entry name */
	QString m_name;
};

/**
 * Ploop wrapper over a read-only raw disk image
 *
 * A read-only raw disk image could not be directly connected as a virtual disk
 * to a VM, because VM requires write access to the image file. Thus we wrap the
 * read-only image into a ploop and create a read-write snapshot.
 * Also, both CT and VM would need to replay and save journal of a FS, located
 * in the backup, if the FS is in inconsistent state - this, again, requires a
 * read-write access to data.
 */
struct Ploop {
	Ploop(const QString& path);
	QString getImage() const;
	PRL_RESULT setImage(const QString& path, const CVmHddEncryption *encryption_);
	void destroy() const;
	PRL_RESULT mount(QString& dev) const;
	PRL_RESULT umount() const;
	PRL_RESULT getMountedDevice(QString& dev) const;

	const QString& getPath() const
	{
		return m_path;
	}

private:
	/** full path to a directory with DiskDescriptor.xml */
	QString m_path;
	/** full path to a disk image file */
	mutable QString m_image;
	/** ploop wrapper */
	PloopImage::Image m_ploop;
};

/**
 * Virual hard disk wrapper over a block device
 */
struct VirtualDisk {
	VirtualDisk(const QString& path);

	const QString& getPath() const
	{
		return m_path;
	}

	void setPath(const QString& value)
	{
		m_path = value;
	}

	PRL_RESULT create(const QString& dev);
	void destroy();
	PRL_RESULT getDevice(QString& path);
	PRL_RESULT setDevice(const QString& path);

private:
	/** full path to the ploop device symlink */
	QString m_path;
};

/**
 * Virtual hard disk wrapper over a read-only image file
 *
 * Depending on VE type, the virtual hard disk wrapper would have different content:
 * - for CTs it consists of the simple ploop wrapper (over a read-only image), which
 *   is saved in CT config and is automatically mounted by vzctl during CT start.
 * - for VMs we have two levels of indirection, because the VirtualDisk library
 *   doesn't allow us to setup a virtual disk wrapper directly over a r/o image:
 *   VirtualDisk -> /dev/ploopXXXXX (rw) <- PloopWrapper -> Image (ro)
 */
struct Hdd {
	Hdd(const QString& vmUuid, PRL_VM_TYPE type, const QString& path,
		Ploop *ploop);
	Image& getImage();
	PRL_RESULT save(CVmHardDisk& disk);
	PRL_RESULT disable();
	PRL_RESULT destroy();
	PRL_RESULT enable();

private:
	/** VM type */
	PRL_VM_TYPE m_type;
	/** UUID of a VM, to which the disk is attached */
	QString m_vmUuid;
	/** ploop wrapper */
	std::auto_ptr<Ploop> m_ploop;
	/** path to a virtual HDD bundle */
	QString m_path;
	/** image wrapper */
	Image m_image;
};

struct Factory
{
	explicit Factory(const QString& uuid);
	PRL_RESULT create(const CVmHardDisk& disk, const QString& vmHome);
	PRL_RESULT create(const Source::BackupInfo& backup_,
		const QString& dir, CAuthHelper *auth);
	Factory& setVmUuid(const QString& uuid);

	Hdd* getResult()
	{
		return m_result.take();
	}

private:
	template<class T>
	PRL_RESULT setResult(T builder);

private:
	QString m_uuid;
	PRL_VM_TYPE m_type;
	QScopedPointer<Hdd> m_result;
};

} // namespace Wrap

namespace Source {

/**
 * Resource, which is used as a data source
 */
struct Resource {
	Resource();
	PRL_RESULT setURL(const QUrl& url);
	PRL_RESULT setCredentials();
	const QString& getHostname() const;
	bool isLocal() const;

	/**
	 * Set session UUID of a local dispatcher connection
	 *
	 * @param sessionUuid - session UUID
	 */
	void setSession(const QString& sessionUuid)
	{
		m_sessionUuid = sessionUuid;
	}

	/**
	 * Get the port number, on which the Dispatcher is listening
	 *
	 * @return port number
	 */
	quint32 getPort() const
	{
		return m_port;
	}

	/**
	 * Get the backup identifier
	 *
	 * @return backup identifier
	 */
	const QString& getBackupId() const
	{
		return m_backupId;
	}

	/**
	 * Get the backuped disk identifier
	 *
	 * @return disk identifier
	 */
	const QString& getDiskId() const
	{
		return m_diskId;
	}

	/**
	 * Get session UUID of a local dispatcher connection
	 *
	 * @return session UUID
	 */
	const QString& getSession() const
	{
		return m_sessionUuid;
	}

	/**
	 * Get the login name for a remote server
	 *
	 * @return login name
	 */
	const QString& getLogin() const
	{
		return m_login;
	}

	/**
	 * Get the password for a remote server
	 *
	 * @return password
	 */
	const QString& getPassword() const
	{
		return m_password;
	}

private:
	void setDefaultHostname() const;

private:
	/** backup ID in the form {UUID}[.pit] */
	QString m_backupId;
	/** ID of a backuped disk */
	QString m_diskId;
	/** name of the server, where the backup is located */
	mutable QString m_hostname;
	/** the port number, on which the Dispatcher is listening */
	quint32 m_port;
	/** session UUID of a local Dispatcher connection */
	QString m_sessionUuid;
	/** login with this name to a remote server */
	QString m_login;
	/** use this password to login to a remote server */
	QString m_password;
	/** is the resource located on the localhost or on a remote server */
	bool m_local;
};

} // namespace Source
} // namespace Attach

/**
 * Common routines for backup attaching tasks
 */
class Task_AttachVmBackupHelper : public Task_BackupHelper
{
	Q_OBJECT
public:
	Task_AttachVmBackupHelper(SmartPtr<CDspClient>& client, const SmartPtr<IOPackage>& p);

protected:
	template <class Cmd> PRL_RESULT prepareTask(Cmd *cmd);
	void finalizeTask();
	PRL_RESULT makeImage();
	PRL_RESULT getVmHome(QString& home);

private:
	PRL_RESULT fetchBackupInfo(QString& result);
	PRL_RESULT doMakeImage();

protected:
	/** resource descriptor */
	Attach::Source::Resource m_resource;
	/** virtual disk params */
	CVmHardDisk m_disk;
	/** backup metadata */
	QScopedPointer<Attach::Source::BackupInfo> m_backup;
};

/**
 * Attach backup to a VM, creating a new virtual disk
 */
class Task_AttachVmBackup : public Task_AttachVmBackupHelper
{
	Q_OBJECT
public:
	Task_AttachVmBackup(SmartPtr<CDspClient>& client, const SmartPtr<IOPackage>& p);

	Attach::Wrap::Hdd *getResult()
	{
		return m_hdd.data();
	}

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();

private:
	VIRTUAL_MACHINE_STATE getVmState();

private:
	/** directory, where virtual disk files should be put */
	QString m_diskDir;
	/** hard disk wrapper */
	QScopedPointer<Attach::Wrap::Hdd> m_hdd;
};

/**
 * Connect an already existing virtual disk to a backup
 */
class Task_ConnectVmBackupSource : public Task_AttachVmBackupHelper
{
	Q_OBJECT
public:
	Task_ConnectVmBackupSource(SmartPtr<CDspClient>& client, const SmartPtr<IOPackage>& p);

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
};

#endif //__Task_AttachVmBackup_H_
