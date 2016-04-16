///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackup.h
///
/// Dispatcher source-side task for Vm backup creation
///
/// @author krasnov@
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

#ifndef __Task_CreateVmBackup_H_
#define __Task_CreateVmBackup_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"
#include "CDspVm.h"
#include "prlxmlmodel/VmConfig/CVmHardware.h"

#ifdef _CT_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

namespace Backup
{
namespace Activity
{
struct Service;
namespace Object
{
struct Model;
} // namespace Object
} // namespace Activity

namespace Work
{
namespace Ct
{

struct Spec
{
	Spec(const QString& cache_, quint32 lastTib_);

	const QString& getArchive() const
	{
		return m_archive;
	}
	Spec& setArchive(const QString& archive_);
	quint32 getDeviceIndex() const
	{
		return m_deviceIndex;
	}
	Spec& setDeviceIndex(quint32 value_)
	{
		m_deviceIndex = value_;
		return *this;
	}
	Spec& noCache();
	QStringList getArguments() const;
	Spec& setOutFile(const QString& value_);
	Spec& setSandbox(const QString& value_);

private:
	quint32 m_deviceIndex;
	QString m_archive;
	QStringList m_cache;
	QStringList m_lastTib;
	QStringList m_outFile;
	QStringList m_sandbox;
};

} // namespace Ct
} // namespace Work
} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackupSource

class Task_CreateVmBackupSource : public Task_BackupHelper
{
	Q_OBJECT

public:
	Task_CreateVmBackupSource(
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &,
		Backup::Activity::Service& );
	virtual ~Task_CreateVmBackupSource();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	PRL_RESULT sendStartRequest();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& pkg);
	/* to build copy of Vm home directory */
	PRL_RESULT backupHardDiskDevices(const Backup::Activity::Object::Model& );

	quint64 calcOriginalSize();

private:
	SmartPtr<CVmConfiguration> m_pVmConfig;
	IOSendJob::Handle m_hJob;
	QString m_sDescription;
	QString m_sVmName;
	QString m_sBackupUuid;
	unsigned m_nBackupNumber;
	QString m_sVmDirUuid;
	QString m_sVmHomePath;
	QString m_sSourcePath;
	quint64 m_nTotalSize;
	quint64 m_nOriginalSize;
	SmartPtr<CVmFileListCopySource> m_pVmCopySource;
	SmartPtr<CVmFileListCopySender> m_pSender;
	/* full backup path */
	QString m_sBackupRootPath;
	QString m_sSnapshotPath;
	Backup::Activity::Service* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateCtBackupHelper

struct Task_CreateCtBackupHelper: Task_BackupHelper
{
	Task_CreateCtBackupHelper(const SmartPtr<CDspClient> &,
		const SmartPtr<IOPackage> &);

	bool isRunning() const;
	quint32 getFlags() const
	{
		return m_nFlags;
	}
	const QString& getVzDirectory() const
	{
		return m_vzDirectory;
	}
	const QString& getBackupRoot() const
	{
		return m_sBackupRootPath;
	}
	int execute(const CVmIdentification& ct_, quint32 deviceIndex_,
			const QStringList& args_);

protected:
	PRL_RESULT prepareTask();
	quint32 getInternalFlags() const
	{
		return m_nInternalFlags;
	}
	void setInternalFlags(quint32 internalFlags_)
	{
		m_nInternalFlags = internalFlags_;
	}
	void setBackupRoot(const QString& backupRoot_)
	{
		m_sBackupRootPath = backupRoot_;
	}
	void setService(Backup::Activity::Service& value_);
	PRL_RESULT do_(const SmartPtr<CVmConfiguration>& config_,
			Backup::Work::Ct::Spec spec_);

private:
	Q_OBJECT

	/* full backup path */
	QString m_vzDirectory;
	QString m_sBackupRootPath;
	quint32 m_nInternalFlags;
	Backup::Activity::Service* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackupSource

class Task_CreateCtBackupSource : public Task_CreateCtBackupHelper
{
	Q_OBJECT

public:
	Task_CreateCtBackupSource(
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &,
		Backup::Activity::Service& );
	virtual ~Task_CreateCtBackupSource();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	PRL_RESULT waitForTargetFinished();
#ifdef _CT_
	PRL_RESULT fillCopyContent();
	PRL_RESULT sendStartRequest();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& pkg);
	PRL_RESULT makeTemporaryCopy(const QString& orig, QString& copy);
#endif

private:
	SmartPtr<CVmConfiguration> m_pCtConfig;
	IOSendJob::Handle m_hJob;
	QString m_sDescription;
	QString m_sCtName;
	QString m_sBackupUuid;
	unsigned m_nBackupNumber;
	QString m_sCtHomePath;
	quint64 m_nTotalSize;
	quint64 m_nOriginalSize;
	bool m_bLocalMode;

	SmartPtr<CVmFileListCopySource> m_pVmCopySource;
	SmartPtr<CVmFileListCopySender> m_pSender;
#ifdef _CT_
	CVzOperationHelper m_VzOpHelper;
#endif
	/** Temporary file copies that will be removed upon task completion */
	QStringList m_tmpCopy;
	Backup::Activity::Service* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackupTarget

class Task_CreateVmBackupTarget : public Task_CreateCtBackupHelper
{
	Q_OBJECT

private:
	quint64 getBackupSize();
	PRL_RESULT saveMetadata();
	PRL_RESULT validateBackupDir(const QString &);
	PRL_RESULT backupHardDiskDevices();
	PRL_RESULT backupCtPrivate();
	PRL_RESULT buildTibFile(const QString&);
	PRL_RESULT loadTibFiles();
	PRL_RESULT wasHddListChanged(bool *pbWasChanged);
	PRL_RESULT guessBackupType();
public:
	Task_CreateVmBackupTarget(
		SmartPtr<CDspDispConnection> &,
		CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &,
		Backup::Activity::Service& );
	virtual ~Task_CreateVmBackupTarget();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	SmartPtr<CDspDispConnection> m_pDispConnection;
	SmartPtr<CVmConfiguration> m_pVmConfig;
	QString m_sVmName;
	QString m_sBackupUuid;
	unsigned m_nBackupNumber;
	QString m_sDescription;
	QString m_sSourceHost;
	QString m_sServerUuid;
	quint64 m_nOriginalSize;
	quint64 m_nFreeDiskSpace;
	quint32 m_nBundlePermissions;

	IOSender::Handle m_hConnHandle;
	SmartPtr<CVmFileListCopyTarget> m_pVmCopyTarget;
	/* path where config files will copy for all backup types */
	QString m_sTargetPath;
	SmartPtr<CVmFileListCopySender> m_pSender;
	bool m_bBackupLocked;
	QList<QString> m_lstTibFileList;
	QString m_sABackupOutFile;
	QString m_sVmHomePath;
	bool m_bLocalMode;
	bool m_bStorageRegistered;
	SmartPtr<BackupItem> m_lastBase;

private:
	WaiterTillHandlerUsingObject m_waiter;
	QMutex m_cABackupMutex;
	bool m_bABackupFirstPacket;
	Backup::Activity::Service* m_service;
	QStringList m_createdTibs;

private slots:
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void clientDisconnected(IOSender::Handle h);

};

#endif //__Task_CreateVmBackup_H_

