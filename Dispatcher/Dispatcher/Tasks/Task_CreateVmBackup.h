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
} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackup

class Task_CreateVmBackup : public Task_BackupHelper
{
	Q_OBJECT

public:
	Task_CreateVmBackup(const SmartPtr<CDspClient>& c_, const SmartPtr<IOPackage>& p_)
		: Task_BackupHelper(c_, p_) {}

protected:
	PRL_RESULT sendStartRequest();
	void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);
	PRL_RESULT copyEscort(const ::Backup::Escort::Model& escort_, const QString& directory_,
		const QString& source_);
	PRL_RESULT backupHardDiskDevices(const ::Backup::Activity::Object::Model& activity_,
		::Backup::Work::object_type& variant_);
	PRL_RESULT doBackup(const QString& source_, ::Backup::Work::object_type& variant_);
	virtual void finalizeTask();

	IOSendJob::Handle m_hJob;
};

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackupSource

class Task_CreateVmBackupSource : public Task_CreateVmBackup
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
	QString m_sSourcePath;
	quint64 m_nTotalSize;
	QString m_sSnapshotPath;
};

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackupSource

class Task_CreateCtBackupSource : public Task_CreateVmBackup
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

private:
	PRL_RESULT waitForTargetFinished();

private:
	quint64 m_nTotalSize;

#ifdef _CT_
	CVzOperationHelper m_VzOpHelper;
#endif
};

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackupTarget

class Task_CreateVmBackupTarget : public Task_BackupHelper
{
	Q_OBJECT

private:
	quint64 getBackupSize();
	PRL_RESULT saveMetadata();
	PRL_RESULT validateBackupDir(const QString &);
	PRL_RESULT buildTibFiles();
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
	QString m_sSourceHost;
	QString m_sServerUuid;
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
	bool m_bStorageRegistered;
	SmartPtr<BackupItem> m_lastBase;

private:
	WaiterTillHandlerUsingObject m_waiter;
	QMutex m_cABackupMutex;
	bool m_bABackupFirstPacket;
	QStringList m_createdTibs;

private slots:
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void clientDisconnected(IOSender::Handle h);

};

#endif //__Task_CreateVmBackup_H_

