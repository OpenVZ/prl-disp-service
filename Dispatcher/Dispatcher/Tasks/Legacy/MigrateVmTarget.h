///////////////////////////////////////////////////////////////////////////////
///
/// @file MigrateVmTarget.h
///
/// Target task for legacy Vm migration
///
/// Copyright (c) 2010-2016 Parallels IP Holdings GmbH
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

#ifndef __LEGACY_TASK_MIGRATEVMTARGET_H_
#define __LEGACY_TASK_MIGRATEVMTARGET_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspRegistry.h"
#include "../Task_DispToDispConnHelper.h"
#include "CDspClient.h"
#include "CDspService.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "CDspDispConnection.h"
#include "CDspVm.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"

namespace Legacy
{
namespace Task
{

class MigrateVmTarget : public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

public:
	MigrateVmTarget(
		Registry::Public&,
		const QObject* parent,
		const SmartPtr<CDspDispConnection>&,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage>&);
	~MigrateVmTarget();
	virtual QString  getVmUuid()
	{
		return m_sOriginVmUuid;
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
	/**
	 * Checks whether CPUs on source and target hosts are compatible
	 */
	void checkTargetCpuCompatibility();
	/* compare available and requires disk spaces */
	void checkRequiresDiskSpace();
	void checkRemoteDisplay();
	void checkEfiBoot();
	/**
	 * Checks whether target VM home path resides on iscsi storage
	 */
	PRL_RESULT registerVmBeforeMigration();
	PRL_RESULT saveVmConfig();
	void DeleteSnapshot();
	void changeSID();
	PRL_RESULT adjustStartVmCommand(SmartPtr<IOPackage>& pPackage);

	PRL_RESULT migrateStoppedVm();
	PRL_RESULT migrateRunningVm();

	PRL_RESULT registerHaClusterResource();
	void unregisterHaClusterResource();
	PRL_RESULT resurrectVm();
private:
	Registry::Public& m_registry;
	const QObject* m_pParent;
	WaiterTillHandlerUsingObject m_waiter;

	/* from old servers Check & Start commands send from differents connections */
	SmartPtr<CDspDispConnection> m_pCheckDispConnection;
	SmartPtr<CDspDispConnection> m_pStartDispConnection;
	SmartPtr<IOPackage> m_pCheckPackage;
	SmartPtr<IOPackage> m_pStartPackage;
	SmartPtr<CVmConfiguration> m_pVmConfig;
	CHostHardwareInfo m_cSrcHostInfo;
	CHostHardwareInfo m_cDstHostInfo;
	QString m_sTargetVmHomePath;
	QString m_sVmConfigPath;
	QStringList m_lstCheckPrecondsErrors;
	QString m_sStorageID;
	QString m_sVmDirUuid;
	QString m_sOriginVmUuid;
	QString m_sVmUuid;
	QString m_sVmName;
	IOSender::Handle m_hConnHandle;

	QString m_sStorageInfo;
	QString m_sSharedFileName;
	QStringList m_lstSharedFileNamesExt;
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
	SmartPtr<CDspVm> m_pVm;
	PRL_UINT64 m_nRequiresDiskSpace;
	QString m_sHaClusterId;

	/* smart pointer to object for data receiving. m_pVmMigrateTarget will use it */
	SmartPtr<CVmFileListCopySender> m_pSender;
	/* Pointer to migrate VM target object */
	SmartPtr<CVmFileListCopyTarget> m_pVmMigrateTarget;

	SmartPtr<CVmDirectory::TemporaryCatalogueItem> m_pVmInfo;

private slots:
	void clientDisconnected(IOSender::Handle h);
	void handleStartPackage(
		const SmartPtr<CDspDispConnection>&,
		const QString&,
		const SmartPtr<IOPackage>&);
	void handleStartCommandTimeout();
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void handleVmMigrateEvent(const QString& sVmUuid, const SmartPtr<IOPackage>& p);
	void handleConversionFinished(int, QProcess::ExitStatus);
};

} // namespace Task
} // namespace Legacy

#endif // __LEGACY_TASK_MIGRATEVMTARGET_H_
