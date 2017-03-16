///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCt_vzwin.h
///
/// Dispatcher task for VZWIN CT template migration
///
/// @author yur@
///
/// Copyright (c) 2011-2017, Parallels International GmbH
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

#ifndef __Task_MigrateCt_win_H_
#define __Task_MigrateCt_win_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "CDspDispConnection.h"
#include "CDspVm.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include "Task_DispToDispConnHelper.h"
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"
#include "Libraries/Virtuozzo/CVzHelper.h"

class Task_MigrateCtSource : public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

private:
	QString m_sVmUuid;
	QString m_sVzDirUuid;
	QString m_sVmName;
	QString m_sVmConfigPath;
	QString m_sVmHomePath;
	QString m_sServerHostname;
	QString m_sServerSessionUuid;
	QString m_sTargetServerCtHomePath;

	SmartPtr<CDspVm> m_pVm;
	SmartPtr<CVmConfiguration> m_pVmConfig;

	VIRTUAL_MACHINE_STATE m_nPrevVmState;
	quint32 m_nCtid;
	quint32 m_nMigrationFlags;
	quint32 m_nReservedFlags;
	quint32 m_nServerPort;
	quint32 m_nSteps;
	quint64 m_nTotalSize;
	bool m_bNewVmInstance;
	bool m_bExVmOperationRegistered;

	/* preconditions errors set */
	QStringList m_lstCheckPrecondsErrors;
	CHostHardwareInfo m_cHostInfo;

	SmartPtr<IOPackage> m_pReply;
	SmartPtr<CVmFileListCopySender> m_pSender;
	SmartPtr<CVmFileListCopySource> m_pVmCopySource;

	IOSendJob::Handle m_hCheckReqJob;
	IOSendJob::Handle m_hStartReqJob;

	void * m_pOpaqueLock;

	PRL_RESULT CheckVmMigrationPreconditions();
	PRL_RESULT SendStartRequest();
	PRL_RESULT sendDispPackage(SmartPtr<IOPackage> &pPackage);
	PRL_RESULT migrateStoppedCt();

public:
	Task_MigrateCtSource(
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateCtSource();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual bool isCancelled() { return operationIsCancelled(); }
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);

};

class Task_MigrateCtTarget : public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

public:
	Task_MigrateCtTarget(
		const QObject *,
		const SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateCtTarget();
	virtual QString  getVmUuid() {return m_sOriginVmUuid;}
	CVzOperationHelper &get_op_helper() { return m_VzOpHelper; }

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);

private:
	//void checkTargetCpusNumber();
	//void checkTargetCpuCompatibility();

	PRL_RESULT migrateStoppedVm();
	//PRL_RESULT migrateRunningVm();


private:
	WaiterTillHandlerUsingObject m_waiter;

	const QObject *m_pParent;
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
	QString m_sVzDirUuid;
	QString m_sOriginVmUuid;
	QString m_sVmUuid;
	QString m_sVmName;
	QString m_sSrcHostInfo;
	QString m_sVmConfig;
	QString	m_sVmDirPath;

	VIRTUAL_MACHINE_STATE m_nPrevVmState;
	quint32 m_nCtid;
	quint32 m_nOriginCtid;
	quint32 m_nVersion;
	quint32 m_nMigrationFlags;
	quint32 m_nReservedFlags;
	quint32 m_nSteps;
	quint32 m_nBundlePermissions;
	quint32 m_nConfigPermissions;

	IOSender::Handle m_hConnHandle;
	SmartPtr<CDspVm> m_pVm;

	/* smart pointer to object for data receiving. m_pVmMigrateTarget will use it */
	SmartPtr<CVmFileListCopySender> m_pSender;
	/* Pointer to migrate VM target object */
	SmartPtr<CVmFileListCopyTarget> m_pVmMigrateTarget;
	SmartPtr<CVmDirectory::TemporaryCatalogueItem> m_pVmInfo;

	void * m_pOpaqueLock;
	CVzOperationHelper m_VzOpHelper;

private slots:
	void clientDisconnected(IOSender::Handle h);
	void handleStartPackage(const SmartPtr<CDspDispConnection>&, const QString&, const SmartPtr<IOPackage>&);
	void handleStartCommandTimeout();
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void handleVmMigrateEvent(const QString &sVmUuid, const SmartPtr<IOPackage> &p);
};


#endif //__Task_MigrateCt_win_H_
