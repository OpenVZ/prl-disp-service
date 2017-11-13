///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmTarget.h
///
/// Dispatcher target-side task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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

#ifndef __Task_MigrateVmTarget_H_
#define __Task_MigrateVmTarget_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspRegistry.h"
#include "Task_DispToDispConnHelper.h"
#include "CDspClient.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "CDspDispConnection.h"
#include "CDspVm.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"

enum _PRL_VM_MIGRATE_TARGET_STEP {
	MIGRATE_STARTED			= (1 << 0),
	MIGRATE_VM_APP_STARTED		= (1 << 1),
	MIGRATE_VM_STORAGE_MOUNTED	= (1 << 2),
	MIGRATE_VM_EXCL_PARAMS_LOCKED	= (1 << 3),
	MIGRATE_HA_RESOURCE_REGISTERED	= (1 << 4),
};

class Task_MigrateVmTarget : public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

public:
	Task_MigrateVmTarget(Registry::Public&,
		const SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateVmTarget();
	virtual QString  getVmUuid() {return m_sOriginVmUuid;}

	PRL_RESULT sendStartConfirmation();
	QList<CVmHardDisk> getImagesToCreate();
	std::pair<CVmFileListCopySender*, CVmFileListCopyTarget*> createCopier();
	bool isTemplate() const
	{
		return m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate();
	}

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);

private:
	/**
	 * Checks whether enough CPUs number on target host to migrate VM
	 */
	void checkTargetCpusNumber();
	/* compare available and requires disk spaces */
	void checkRequiresDiskSpace();
	/**
	 * Checks whether target VM home path resides on the shared storage
	 */
	PRL_RESULT checkSharedStorage();

	PRL_RESULT registerVmBeforeMigration();
	PRL_RESULT saveVmConfig();
	void DeleteSnapshot();
	void changeSID();
	PRL_RESULT adjustStartVmCommand(SmartPtr<IOPackage> &pPackage);
	PRL_RESULT registerHaClusterResource();
	void unregisterHaClusterResource();
	bool isSharedDisk(const QString& name) const;

private:
	Registry::Public& m_registry;
	/* from old servers Check & Start commands send from differents connections */
	SmartPtr<CDspDispConnection> m_dispConnection;
	SmartPtr<IOPackage> m_pCheckPackage;
	SmartPtr<IOPackage> m_pStartPackage;
	SmartPtr<CVmConfiguration> m_pVmConfig;
	CHostHardwareInfo m_cSrcHostInfo;
	CHostHardwareInfo m_cDstHostInfo;
	QString m_sTargetVmHomePath;
	QString m_sVmConfigPath;
	QStringList m_lstCheckPrecondsErrors;
	QString m_sVmDirUuid;
	QString m_sOriginVmUuid;
	QString m_sVmUuid;
	QString m_sVmName;
	IOSender::Handle m_hConnHandle;

	QString m_sStorageInfo;
	QString m_sSharedFileName;
	QStringList m_lstCheckFilesExt;
	QStringList m_lstNonSharedDisks;
	quint32 m_nMigrationFlags;
	quint32 m_nReservedFlags;
	QString m_sSnapshotUuid;
	quint32 m_nVersion;
	VIRTUAL_MACHINE_STATE m_nPrevVmState;
	QString m_sVmConfig;
	QString m_sSrcHostInfo;
	QString	m_sVmDirPath;
	quint32 m_nFlags;
	quint32 m_nSteps;
	quint32 m_nBundlePermissions;
	quint32 m_nConfigPermissions;
	PRL_UINT64 m_nRequiresDiskSpace;
	QString m_sHaClusterId;

	SmartPtr<CVmDirectory::TemporaryCatalogueItem> m_pVmInfo;

	PRL_RESULT reactStart(const SmartPtr<IOPackage> &package);
	PRL_RESULT preconditionsReply();

signals:
	void cancel();

private slots:
	void handleVmMigrateEvent(const QString &sVmUuid, const SmartPtr<IOPackage> &p);
};

#endif //__Task_MigrateVm_H_
