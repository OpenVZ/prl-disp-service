///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmSource.h
///
/// Dispatcher source-side task for Vm migration
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

#ifndef __Task_MigrateVmSource_H_
#define __Task_MigrateVmSource_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "Task_DispToDispConnHelper.h"
#include "CDspClient.h"
#include "CDspRegistry.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "CDspVm.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"

enum _PRL_VM_MIGRATE_STEP {
	MIGRATE_UNREGISTER_VM_WATCH	= (1 << 0),
	MIGRATE_SUSPENDED_VM		= (1 << 1),
	MIGRATE_VM_EXCL_PARAMS_LOCKED	= (1 << 2),
	MIGRATE_CONFIGSAV_BACKUPED	= (1 << 3),
	MIGRATE_VM_APP_STARTED		= (1 << 4),
	MIGRATE_VM_STATE_CHANGED	= (1 << 5),
};

namespace Migrate
{
namespace Vm
{
namespace Source
{
namespace Content
{
struct Copier;

} // namespace Content
} // namespace Source
} // namespace Vm
} // namespace Migrate


class Task_MigrateVmSource : public CDspTaskHelper, public Task_DispToDispConnHelper, public Toll::Choke
{
	Q_OBJECT

public:
	typedef QList<QPair<QFileInfo, QString> > itemList_type;

	Task_MigrateVmSource(
		Registry::Public &,
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateVmSource();
	virtual QString  getVmUuid() {return m_sVmUuid;}

	quint32 getFlags() const
	{
		return m_nMigrationFlags;
	}
	const QString& getTargetAddress() const
	{
		return m_sServerHostname;
	}
	const CVmConfiguration *getTargetConfig() const
	{
		return m_targetConfig.data();
	}
	const CVmConfiguration* getVmConfig() const
	{
		return m_pVmConfig.get();
	}
	VIRTUAL_MACHINE_STATE getOldState() const
	{
		return m_nPrevVmState;
	}
	PRL_RESULT confirmFinish();
	QList<CVmHardDisk* > getVmUnsharedDisks() const;
	Migrate::Vm::Source::Content::Copier* createCopier();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual bool isCancelled() { return operationIsCancelled(); }
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);

private:
	void DisconnectExternalImageDevice(CVmDevice* pDevice);
	PRL_RESULT CheckVmDevices();
	PRL_RESULT CheckVmMigrationPreconditions();
	PRL_RESULT migrateStoppedVm();
	void releaseLocks();

	PRL_RESULT prepareStart();
	PRL_RESULT reactCheckReply(const SmartPtr<IOPackage>& package_);
	PRL_RESULT reactStartReply(const SmartPtr<IOPackage>& package_);

signals:
	void cancel();

private:
	QString m_sVmUuid;
	QString m_sVmDirUuid;
	QString m_sServerHostname;
	quint32 m_nServerPort;
	QString m_sServerSessionUuid;
	QString m_sTargetServerVmName;
	QString m_sTargetServerVmHomePath;
	quint32 m_nMigrationFlags;
	quint32 m_nReservedFlags;
	QString m_sSnapshotUuid;
	/* remote client protocol version */
	quint32 m_nRemoteVersion;
	QString m_sTargetMemFilePath;
	QFileInfo m_cSavFile;
	QFileInfo m_cSavFileCopy;
	QFileInfo m_cLocalMemFile;
	QString m_sConfigSavBackup;

	SmartPtr<CDspVm> m_pVm;
	SmartPtr<CVmConfiguration> m_pVmConfig;
	bool m_bNewVmInstance;

	/* preconditions errors set */
	QStringList m_lstCheckPrecondsErrors;
	CHostHardwareInfo m_cHostInfo;
	VIRTUAL_MACHINE_STATE m_nPrevVmState;
	QString m_sVmName;
	QString m_sVmConfigPath;
	QString m_sVmHomePath;
	QStringList m_lstAllCheckFiles;
	QStringList m_lstNonSharedDisks;
	QScopedPointer<CVmConfiguration> m_targetConfig;

	quint32 m_nSteps;

	/* VM size */
	itemList_type m_dList;
	itemList_type m_fList;

	PVE::IDispatcherCommands m_nRegisterCmd;
	Registry::Public& m_registry;
};

#endif //__Task_MigrateVmSource_H_
