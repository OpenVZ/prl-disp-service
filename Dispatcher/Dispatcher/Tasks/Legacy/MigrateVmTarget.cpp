///////////////////////////////////////////////////////////////////////////////
///
/// @file MigrateVmTarget.cpp
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

#include "Interfaces/Debug.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <prlcommon/Logging/Logging.h>
#include "Libraries/StatesStore/SavedStateTree.h"
#include <prlcommon/Std/PrlTime.h>

#include "CDspVmDirHelper.h"
#include "MigrateVmTarget.h"
#include "Legacy/MigrationHandler.h"
//#include "Task_RegisterVm.h"
#include "Tasks/Task_CloneVm.h"
#include "Tasks/Task_ChangeSID.h"
#include "CDspVmStateSender.h"
#include "CDspService.h"
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif
#include "Tasks/Mixin_CreateVmSupport.h"
#include <prlcommon/IOService/IOCommunication/IOServerPool.h>
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_BackgroundJob.h"
#include "CDspService.h"
#include "Libraries/PrlCommonUtils/CVmMigrateHelper.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include "CVcmmdInterface.h"
#include "Legacy/VmConverter.h"
#include "CDspVmManager_p.h"
#include "CDspVm_p.h"

namespace Legacy
{
namespace Task
{

enum
{
	VM_MIGRATE_START_CMD_WAIT_TIMEOUT = 600 * 1000
};

enum _PRL_VM_MIGRATE_TARGET_STEP
{
	MIGRATE_STARTED			= (1 << 0),
	MIGRATE_VM_APP_STARTED		= (1 << 1),
	MIGRATE_VM_STORAGE_MOUNTED	= (1 << 2),
	MIGRATE_VM_EXCL_PARAMS_LOCKED	= (1 << 3),
	MIGRATE_HA_RESOURCE_REGISTERED	= (1 << 4),
};

MigrateVmTarget::MigrateVmTarget(
	Registry::Public& registry_,
	const QObject* parent,
	const SmartPtr<CDspDispConnection>& pDispConnection,
	CDispToDispCommandPtr pCmd,
	const SmartPtr<IOPackage>& p)
	: CDspTaskHelper(pDispConnection->getUserSession(), p),
	  Task_DispToDispConnHelper(getLastError()),
	  m_registry(registry_),
	  m_pParent(parent),
	  m_pCheckDispConnection(pDispConnection),
	  m_pCheckPackage(p),
	  m_cDstHostInfo(CDspService::instance()->getHostInfo()->data()),
	  m_nFlags(0),
	  m_nSteps(0),
	  m_nBundlePermissions(0),
	  m_nConfigPermissions(0)
{
	CVmMigrateCheckPreconditionsCommand* pCheckCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);

	m_hConnHandle = pDispConnection->GetConnectionHandle();

	m_sHaClusterId = pCheckCmd->GetStorageInfo();
	m_sSharedFileName = pCheckCmd->GetSharedFileName();
	m_nRequiresDiskSpace = pCheckCmd->GetRequiresDiskSpace();
	m_nMigrationFlags = pCheckCmd->GetMigrationFlags();
	m_nReservedFlags = pCheckCmd->GetReservedFlags();
	m_nVersion = pCheckCmd->GetVersion();
	m_sVmConfig = pCheckCmd->GetVmConfig();
	m_sVmName = pCheckCmd->GetTargetVmName();

	m_sSrcHostInfo = pCheckCmd->GetSourceHostHardwareInfo();
	if (m_nVersion >= MIGRATE_DISP_PROTO_V4)
		m_lstSharedFileNamesExt = pCheckCmd->GetSharedFileNamesExtra();
	if (m_nVersion >= MIGRATE_DISP_PROTO_V3)
		m_nPrevVmState = pCheckCmd->GetVmPrevState();
	if (pCheckCmd->GetTargetVmHomePath().isEmpty())
	{
		WRITE_TRACE(DBG_DEBUG, "use default vm home dir path");
		m_sVmDirPath = m_pCheckDispConnection->getUserSession()->getUserDefaultVmDirPath();
	}
	else
	{
		m_sVmDirPath = pCheckCmd->GetTargetVmHomePath();
		WRITE_TRACE(DBG_DEBUG, "use default vm home dir path '%s'", qPrintable(m_sVmDirPath));
	}
	/* initialize all vars from pCheckCmd - after exit from constructor package buffer will invalid */

	bool bConnected = QObject::connect(
				  &CDspService::instance()->getIOServer(),
				  SIGNAL(onClientDisconnected(IOSender::Handle)),
				  SLOT(clientDisconnected(IOSender::Handle)),
				  Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

MigrateVmTarget::~MigrateVmTarget()
{
	// #439777 protect handler from destroying object
	m_waiter.waitUnlockAndFinalize();
}

/* process VmMigrateCheckPreconditionsCommand */
PRL_RESULT MigrateVmTarget::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(&getClient()->getAuthHelper());

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sVmDirPath;

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - VM migrate rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration(m_sVmConfig));
	if (PRL_FAILED(m_pVmConfig->m_uiRcInit))
	{
		nRetCode = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Wrong VM condiguration was received: [%s]", QSTR2UTF8(m_sVmConfig));
		goto exit;
	}

	if (m_sVmName.isEmpty())
		m_sVmName = m_pVmConfig->getVmIdentification()->getVmName();

	m_pVmConfig->getVmIdentification()->setVmName(m_sVmName);

	m_sOriginVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();
	m_sVmDirUuid = getClient()->getVmDirectoryUuid();
	m_sVmUuid = (!(m_nReservedFlags & (PVM_DONT_COPY_VM | PVM_ISCSI_STORAGE)) && (PVMT_CLONE_MODE & getRequestFlags()))
		    ? Uuid::createUuid().toString() : m_sOriginVmUuid;

	m_cSrcHostInfo.fromString(m_sSrcHostInfo);
	if (PRL_FAILED(m_cSrcHostInfo.m_uiRcInit))
	{
		nRetCode = PRL_ERR_PARSE_HOST_HW_INFO;
		WRITE_TRACE(DBG_FATAL, "Wrong source host hw info was received: [%s]", QSTR2UTF8(m_sSrcHostInfo));
		goto exit;
	}

	/*  to get target VM home directory path */
	m_sTargetVmHomePath = QString("%1/%2").arg(m_sVmDirPath).arg(::Vm::Config::getVmHomeDirName(m_sVmUuid));
	m_sTargetVmHomePath = QFileInfo(m_sTargetVmHomePath).absoluteFilePath();
	m_sVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath);
	m_pVmConfig->getVmIdentification()->setHomePath(m_sVmConfigPath);

	/* lock Vm exclusive parameters */
	m_pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
				m_sVmUuid, m_sVmConfigPath, m_sVmName));

	nRetCode = CDspService::instance()->getVmDirManager()
		   .checkAndLockNotExistsExclusiveVmParameters(QStringList(), m_pVmInfo.getImpl());
	if (PRL_FAILED(nRetCode))
	{
		switch (nRetCode)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
			WRITE_TRACE(DBG_FATAL, "UUID '%s' already registered", QSTR2UTF8(m_pVmInfo->vmUuid));
			CDspTaskFailure(*this).setCode(nRetCode).setToken(m_pVmInfo->vmUuid);
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(m_pVmInfo->vmXmlPath));
			CDspTaskFailure(*this).setCode(nRetCode)(m_pVmInfo->vmName, m_pVmInfo->vmXmlPath);
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
			WRITE_TRACE(DBG_FATAL, "name '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			CDspTaskFailure(*this).setCode(nRetCode)(m_pVmInfo->vmName);
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED:
			WRITE_TRACE(DBG_FATAL, "container '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			CDspTaskFailure(*this).setCode(nRetCode)(m_pVmInfo->vmName);
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				    QSTR2UTF8(m_pVmInfo->vmUuid), QSTR2UTF8(m_pVmInfo->vmName), QSTR2UTF8(m_pVmInfo->vmXmlPath));
			CDspTaskFailure(*this).setCode(nRetCode).setToken(m_pVmInfo->vmUuid)
				.setToken(m_pVmInfo->vmXmlPath).setToken(m_pVmInfo->vmName);
		}
		goto exit;
	}
	m_nFlags |= MIGRATE_VM_EXCL_PARAMS_LOCKED;

	if (!(PVMT_SWITCH_TEMPLATE & getRequestFlags()))
	{
		/* skip checking for copy to template case (https://jira.sw.ru/browse/PSBM-9597) */
		checkTargetCpusNumber();
	}

	foreach (const QString& checkFile, m_lstSharedFileNamesExt)
	{
		QFileInfo fi(checkFile);
		if (!fi.exists())
		{
			// we use independent dir separators in protocol
			m_lstNonSharedDisks.append(QDir::fromNativeSeparators(fi.absolutePath()));
			WRITE_TRACE(DBG_FATAL, "External disk %s is on private media.",
				    QSTR2UTF8(fi.absolutePath()));
		}
	}

	// check that external disk paths are not exist
	foreach (const QString& disk, m_lstNonSharedDisks)
	{
		if (!QDir(disk).exists())
			continue;
		nRetCode = PRL_ERR_VM_MIGRATE_EXT_DISK_DIR_ALREADY_EXISTS_ON_TARGET;
		WRITE_TRACE(DBG_FATAL,
			    "The directory for external disk '%s' already exists.", QSTR2UTF8(disk));
		CDspTaskFailure(*this)(nRetCode, disk);
		goto exit;
	}

	if (!(m_nMigrationFlags & PVMT_IGNORE_EXISTING_BUNDLE) && QDir(m_sTargetVmHomePath).exists())
	{
		nRetCode = PRL_ERR_VM_MIGRATE_VM_HOME_ALREADY_EXISTS_ON_TARGET;
		WRITE_TRACE(DBG_FATAL,
			    "The virtual machine home directory '%s' already exists.",
			    QSTR2UTF8(m_sTargetVmHomePath));
		CDspTaskFailure(*this)(nRetCode, m_sTargetVmHomePath);
		goto exit;
	}

	// create target directory before checkRequiresDiskSpace
	if (!QDir(m_sVmDirPath).exists(m_sVmDirPath) &&
		!CFileHelper::WriteDirectory(m_sVmDirPath, &getClient()->getAuthHelper()))
	{
		nRetCode = PRL_ERR_VM_MIGRATE_CANNOT_CREATE_DIRECTORY;
		CDspTaskFailure(*this)(nRetCode, m_sVmDirPath);
		goto exit;
	}

	checkRequiresDiskSpace();
	checkRemoteDisplay();
	checkEfiBoot();
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT MigrateVmTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	bool bConnected;
	QTimer* pTimer;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;

	if (operationIsCancelled())
	{
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	bConnected = QObject::connect(m_pParent,
				      SIGNAL(onPackageReceived(
						      const SmartPtr<CDspDispConnection>&,
						      const QString&,
						      const SmartPtr<IOPackage>&)),
				      SLOT(handleStartPackage(
						      const SmartPtr<CDspDispConnection>&,
						      const QString&,
						      const SmartPtr<IOPackage>&)),
				      Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	/* for protocol v.2 and later will send VmMigrateCheckPreconditionsReply */
	pReply = CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsReply(
			 m_lstCheckPrecondsErrors, m_lstNonSharedDisks, m_nFlags);
	pPackage =
		DispatcherPackage::createInstance(
			pReply->GetCommandId(), pReply->GetCommand()->toString(), m_pCheckPackage);
	hJob = m_pCheckDispConnection->sendPackage(pPackage);

	if (!hJob.isValid())
	{
		nRetCode = PRL_ERR_OPERATION_FAILED;
		goto exit;
	}

	/* set timer */
	pTimer = new QTimer();
	pTimer->setSingleShot(true);
	bConnected = QObject::connect(pTimer, SIGNAL(timeout()), SLOT(handleStartCommandTimeout()), Qt::DirectConnection);
	pTimer->start(VM_MIGRATE_START_CMD_WAIT_TIMEOUT);

	/* will wait StartMigration command */
	nRetCode = exec();
	pTimer->stop();
	if (bConnected)
		QObject::disconnect(pTimer, SIGNAL(timeout()), this, SLOT(handleStartCommandTimeout()));
	delete pTimer;

	QObject::disconnect(m_pParent,
			    SIGNAL(onPackageReceived(
					   const SmartPtr<CDspDispConnection>&,
					   const QString&,
					   const SmartPtr<IOPackage>&)),
			    this,
			    SLOT(handleStartPackage(
					 const SmartPtr<CDspDispConnection>&,
					 const QString&,
					 const SmartPtr<IOPackage>&)));

	if (PRL_FAILED(nRetCode))
		goto exit;

	if (operationIsCancelled())
	{
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if (!(m_nReservedFlags & (PVM_DONT_COPY_VM | PVM_ISCSI_STORAGE)))
	{
		/* to create Vm bundle */
		if (!CFileHelper::WriteDirectory(m_sTargetVmHomePath, &getClient()->getAuthHelper()))
		{
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CDspTaskFailure(*this)(nRetCode, m_sTargetVmHomePath);
			WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory",
				    __FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			goto exit;
		}
		/* set original permissions to Vm bundle (https://jira.sw.ru/browse/PSBM-8269) */
		if (m_nBundlePermissions)
		{
			QFile vmBundle(m_sTargetVmHomePath);
			if (!vmBundle.setPermissions((QFile::Permissions)m_nBundlePermissions))
			{
				WRITE_TRACE(DBG_FATAL,
					    "[%s] Cannot set permissions for Vm bundle \"%s\", will use default",
					    __FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			}
		}
	}

	// create directories for external disks
	foreach (const QString& disk, m_lstNonSharedDisks)
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Creating disk '%s'",
			    __FUNCTION__, QSTR2UTF8(disk));
		// first create try to create parent directories
		// then create disk dir itself, so if it is created meanwhile by
		// someone else we get an error
		if (CFileHelper::WriteDirectory(QFileInfo(disk).absolutePath(), &getClient()->getAuthHelper()) &&
			CFileHelper::CreateDirectoryPath(disk, &getClient()->getAuthHelper()))
			continue;

		nRetCode = PRL_ERR_VM_MIGRATE_CANNOT_CREATE_DIRECTORY;
		CDspTaskFailure(*this)(nRetCode, disk);
		WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory",
			    __FUNCTION__, QSTR2UTF8(disk));
		goto exit;
	}

	m_nSteps |= MIGRATE_STARTED;
	if (m_nPrevVmState == VMS_RUNNING)
	{
		nRetCode = migrateRunningVm();
	}
	else
	{
		nRetCode = PRL_ERR_UNIMPLEMENTED;
		goto exit;
		//nRetCode = migrateStoppedVm();
	}

	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle)));

	if (PRL_FAILED(nRetCode))
		goto exit;

	// update config for shared storage
	if ((m_nReservedFlags & PVM_DONT_COPY_VM) && !(PVMT_CLONE_MODE & getRequestFlags()))
		saveVmConfig();

	/* and set logged user by owner to all VM's files */
	Mixin_CreateVmSupport().setDefaultVmPermissions(getClient(), m_sVmConfigPath, true);
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void MigrateVmTarget::changeSID()
{
	// nobody requested SID change
	if (!(PVMT_CHANGE_SID & getRequestFlags()) &&
		!(PVMT_CLONE_MODE & getRequestFlags()))
		return;

	// VM is on shared storage
	if (m_nReservedFlags & (PVM_DONT_COPY_VM | PVM_ISCSI_STORAGE))
		return;

	// SID is Windows feature
	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType() != PVS_GUEST_TYPE_WINDOWS)
		return;

	// no need to change SID for templates - it will be changed on
	// deployment from it
	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return;

	CProtoCommandPtr pRequest = CProtoSerializer::CreateProtoBasicVmCommand(
					    PVE::DspCmdVmChangeSid,
					    m_pVmConfig->getVmIdentification()->getVmUuid(), 0);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmChangeSid, pRequest);
	CDspService::instance()->getTaskManager()
	.schedule(new Task_ChangeSID(getClient(), pPackage, m_pVmConfig)).wait();
}

void MigrateVmTarget::finalizeTask()
{
	IOSendJob::Handle hJob;
	SmartPtr<IOPackage> pPackage;

	// delete temporary registration
	if (m_nFlags & MIGRATE_VM_EXCL_PARAMS_LOCKED)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		// remove temporatry file migrated within VM home
		QFile::remove(QDir(m_sTargetVmHomePath).absoluteFilePath(m_sSharedFileName));
		/* to unite statistic.log and statistic.log.migrate */
		if (m_nFlags & PVM_ISCSI_STORAGE)
		{
			/* To remove backup of source config. It's needs for iSCSI-based Vm migration :
			   on source side storage mounted by read-only during finalize part and source
			   server can not remove any in Vm home */
			QString sVmConfigBackup =
				QString("%1" VMDIR_DEFAULT_VM_MIGRATE_SUFFIX).arg(m_sVmConfigPath);
			QFile::remove(sVmConfigBackup);
		}

		PRL_EVENT_TYPE evtType;
		/* restore Vm previous state */
		switch (m_nPrevVmState)
		{
		case VMS_RUNNING:
			evtType = PET_DSP_EVT_VM_STARTED;
			break;
		case VMS_PAUSED:
			evtType = PET_DSP_EVT_VM_PAUSED;
			break;
		case VMS_SUSPENDED:
			evtType = PET_DSP_EVT_VM_SUSPENDED;
			break;
		case VMS_STOPPED:
		default:
			evtType = PET_DSP_EVT_VM_STOPPED;
			break;
		}
		CVmEvent cStateEvent(evtType, m_sVmUuid, PIE_DISPATCHER);
		SmartPtr<IOPackage> pUpdateVmStatePkg =
			DispatcherPackage::createInstance(PVE::DspVmEvent, cStateEvent.toString());
		CDspService::instance()->getClientManager().
		sendPackageToVmClients(pUpdateVmStatePkg, m_sVmDirUuid, m_sVmUuid);

		m_pCheckDispConnection->sendSimpleResponse(m_pCheckPackage, PRL_ERR_SUCCESS);
		/* migration initiator wait event from original Vm uuid */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_FINISHED, m_sOriginVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sOriginVmUuid);
	}
	else
	{
		/* It is not possible to get Vm client list after deleteVmDirectoryItem(), and
		   sendPackageToVmClients() does not work. Get list right now and will use
		   sendPackageToClientList() call (https://jira.sw.ru/browse/PSBM-9159) */
		QList< SmartPtr<CDspClient> > clientList =
			CDspService::instance()->getClientManager().
			getSessionListByVm(m_sVmDirUuid, m_sVmUuid).values();

		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		unregisterHaClusterResource();

		/* if migration was not started, do not cleanup anything -
		   we can remove 'already existed Vm' */
		if (m_nSteps & MIGRATE_STARTED)
		{
			if (PRL_FAILED(m_registry.undeclare(m_sVmUuid)))
				WRITE_TRACE(DBG_FATAL, "Unable to undeclare VM after migration fail");

			if (!CDspService::instance()->isServerStopping())
				CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(m_sTargetVmHomePath);
			if (!(m_nReservedFlags & (PVM_DONT_COPY_VM | PVM_ISCSI_STORAGE)))
				CFileHelper::ClearAndDeleteDir(m_sTargetVmHomePath);
			foreach (const QString& disk, m_lstNonSharedDisks)
				CFileHelper::ClearAndDeleteDir(disk);

			// Unregister VM dir item
			CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(m_sVmDirUuid, m_sVmUuid);

			CVmEvent cDelEvent(PET_DSP_EVT_VM_DELETED, m_sVmUuid, PIE_DISPATCHER);
			SmartPtr<IOPackage> pDelPackage =
				DispatcherPackage::createInstance(PVE::DspVmEvent, cDelEvent.toString());
			CDspService::instance()->getClientManager().sendPackageToClientList(
				pDelPackage, clientList);
		}
		/* migration initiator wait event from original Vm uuid */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED, m_sOriginVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToClientList(pPackage, clientList);

		if (m_pStartDispConnection.isValid())
			hJob = m_pStartDispConnection->sendResponseError(getLastError(), getRequestPackage());
		else
			hJob = m_pCheckDispConnection->sendResponseError(getLastError(), getRequestPackage());

		CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);
	}
}

PRL_RESULT MigrateVmTarget::migrateStoppedVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;

	CVzHelper vz;
	if (vz.set_vziolimit("VZ_TOOLS"))
		WRITE_TRACE(DBG_FATAL, "Warning: Ignore IO limit parameters");

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM))
	{
		if (PRL_FAILED(nRetCode = saveVmConfig()))
			return nRetCode;
	}

	/* will register resource on HA cluster just before
	   register CT on node : better to have a broken resource than
	   losing a valid */
	nRetCode = registerHaClusterResource();
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	nRetCode = registerVmBeforeMigration();
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(
				CDspService::instance()->getIOServer(),
				m_pStartDispConnection->GetConnectionHandle()));
	/* CVmFileListCopyTarget will use Vm uuid in progress messages for clients, so will use original Vm uuid */
	m_pVmMigrateTarget = SmartPtr<CVmFileListCopyTarget>(
				     new CVmFileListCopyTarget(m_pSender.getImpl(), m_sOriginVmUuid, m_sTargetVmHomePath, NULL, m_nTimeout));

	// Connect to handle traffic report package
	bool bConnected = QObject::connect(
				  m_pStartDispConnection.getImpl(),
				  SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
				  SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
				  Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	pReply = CDispToDispProtoSerializer::CreateVmMigrateReply(QString());

	pPackage = DispatcherPackage::createInstance(
			   pReply->GetCommandId(), pReply->GetCommand()->toString(), m_pStartPackage);
	m_pStartDispConnection->sendPackage(pPackage);

	nRetCode = QThread::exec();

	QObject::disconnect(
		m_pStartDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));

	if (PRL_FAILED(nRetCode))
		return nRetCode;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT MigrateVmTarget::migrateRunningVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> pPackage;

	Legacy::Vm::Converter().convertHardware(m_pVmConfig);

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM) && !(m_nReservedFlags & PVM_ISCSI_STORAGE))
		if (PRL_FAILED(nRetCode = saveVmConfig()))
			return nRetCode;

	/* will register resource on HA cluster just before
	   register CT on node : better to have a broken resource than
	   losing a valid */
	nRetCode = registerHaClusterResource();
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	nRetCode = registerVmBeforeMigration();
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	nRetCode = adjustStartVmCommand(pPackage);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	nRetCode = CDspService::instance()->getAccessManager().checkAccess(
			   getClient(), PVE::DspCmdCtlMigrateTarget, m_sVmUuid);
	if (PRL_FAILED(nRetCode))
	{
		if ((nRetCode == PRL_ERR_ACCESS_TO_VM_DENIED) && (m_nReservedFlags & PVM_DONT_COPY_VM))
		{
			/* user has not permissions to start shared Vm */
			nRetCode = PRL_ERR_VM_MIGRATE_ACCESS_TO_VM_DENIED;
			CDspTaskFailure(*this)(nRetCode, getClient()->getAuthHelper().getUserName());
			WRITE_TRACE(DBG_FATAL,
				    "User %s has not enough permissions in shared Vm bundle %s to start online migration",
				    QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sTargetVmHomePath));
		}
		return nRetCode;
	}

	// Registry
	WRITE_TRACE(DBG_DEBUG, "declare VM UUID:%s, Dir UUID:%s, Config:%s",
		    qPrintable(m_sVmUuid), qPrintable(m_sVmDirUuid), qPrintable(m_sVmConfigPath));
	if (PRL_FAILED(nRetCode = m_registry.declare(CVmIdent(m_sVmUuid, m_sVmDirUuid), m_sVmConfigPath)))
		return nRetCode;

	QSharedPointer<Legacy::Vm::Migration::Convoy> c
		(new Legacy::Vm::Migration::Convoy(m_pStartDispConnection, pPackage));

	Legacy::Vm::Migration::Handler h(CDspService::instance()->getIOServer(), c);

	if (!connect(c.data(), SIGNAL(finished(int, QProcess::ExitStatus)), this,
		     SLOT(handleConversionFinished(int, QProcess::ExitStatus)), Qt::DirectConnection))
	{
		WRITE_TRACE(DBG_FATAL, "unable to connect process finished");
		return PRL_ERR_OPERATION_FAILED;
	}

	if (!c->appoint(m_pVmConfig))
		return PRL_ERR_OPERATION_FAILED;

	m_nSteps |= MIGRATE_VM_APP_STARTED;

	return exec();
}

void MigrateVmTarget::handleVmMigrateEvent(const QString& sVmUuid, const SmartPtr<IOPackage>& p)
{
	if (operationIsCancelled())
		exit(PRL_ERR_FAILURE);

	if (sVmUuid != m_sVmUuid)
		return;

	if (p->header.type != PVE::DspVmEvent)
	{
		WRITE_TRACE(DBG_FATAL, "Unexpected package with type %d, ignored", p->header.type);
		return;
	}

	CVmEvent event(UTF8_2QSTR(p->buffers[0].getImpl()));
	switch (event.getEventType())
	{
	case PET_DSP_EVT_VM_MIGRATE_CANCELLED_DISP:
		exit(PRL_ERR_FAILURE);
		break;
	case PET_DSP_EVT_VM_MIGRATE_FINISHED_DISP:
		exit(PRL_ERR_SUCCESS);
		break;
	default:
		WRITE_TRACE(DBG_FATAL, "Unexpected event with type %d, ignored", event.getEventType());
		break;
	}
}

void MigrateVmTarget::handleConversionFinished(int code_, QProcess::ExitStatus status_)
{
	WRITE_TRACE(DBG_DEBUG, "prl_migration_app is over");
	if (code_ != 0 || status_ != QProcess::NormalExit)
		return QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
	QThread::exit(PRL_ERR_SUCCESS);
}

void MigrateVmTarget::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;
	PRL_ASSERT(m_pVmMigrateTarget.getImpl());

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock(m_waiter);
	if (!lock.isLocked())
		return;

	if (h != m_pStartDispConnection->GetConnectionHandle())
		return;

	if (IS_FILE_COPY_PACKAGE(p->header.type))
	{
		nRetCode = m_pVmMigrateTarget->handlePackage(p, &bExit);
		if (bExit)
			QThread::exit(nRetCode);
	}
	else if (p->header.type == VmMigrateCancelCmd)
	{
		WRITE_TRACE(DBG_DEBUG, "Migration was cancelled");
		QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "Invalid package type %d, ignored", p->header.type);
	}
}

void MigrateVmTarget::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock(m_waiter);
	if (!lock.isLocked())
		return;

	WRITE_TRACE(DBG_DEBUG, "some client was disconnected");
	if (m_nSteps & MIGRATE_VM_APP_STARTED)
		return;

	if (h != m_hConnHandle)
		return;

	SmartPtr<CDspClient> nullClient;
	cancelOperation(nullClient, getRequestPackage());
}

void MigrateVmTarget::handleStartPackage(
	const SmartPtr<CDspDispConnection>& pDispConnection,
	const QString& sVmUuid,
	const SmartPtr<IOPackage>& p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock(m_waiter);
	if (!lock.isLocked())
		return;

	if (operationIsCancelled())
		return;

	/* this function will handle StartMigrateCommand with _original_ Vm uuid */
	if (m_sOriginVmUuid != sVmUuid)
		return;

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if (!pCmd->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			    p->buffers[0].getImpl());
		QThread::exit(PRL_ERR_FAILURE);
	}

	CVmMigrateStartCommand* pStartCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);
	if (NULL == pStartCmd)
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			    p->buffers[0].getImpl());
		QThread::exit(PRL_ERR_FAILURE);
	}
	m_nMigrationFlags = pStartCmd->GetMigrationFlags();
	m_nReservedFlags = pStartCmd->GetReservedFlags();
	m_sSnapshotUuid = pStartCmd->GetSnapshotUuid();
	if (m_nVersion < MIGRATE_DISP_PROTO_V3)
	{
		m_nPrevVmState = pStartCmd->GetVmPrevState();
	}
	else
	{
		m_nBundlePermissions = pStartCmd->GetBundlePermissions();
		m_nConfigPermissions = pStartCmd->GetConfigPermissions();
	}

	/* TODO : compare m_nFlags  and m_nReservedFlags */

	/* reassign task to new connection and package and return to main thread */
	SmartPtr<CDspClient> pClient = pDispConnection->getUserSession();
	reassignTask(pClient, p);
	m_pStartDispConnection = pDispConnection;
	/* attn: do not use getRequestPackage() after this - reassignTask() changed type of new package */
	m_pStartPackage = IOPackage::duplicateInstance(p);

	QThread::exit(PRL_ERR_SUCCESS);
}

// cancel command
void MigrateVmTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	if (m_pVmMigrateTarget.isValid())
		m_pVmMigrateTarget->cancelOperation();

	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

void MigrateVmTarget::handleStartCommandTimeout()
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock(m_waiter);
	if (!lock.isLocked())
		return;

	WRITE_TRACE(DBG_DEBUG, "StartMigrateCommand timeout");
	QThread::exit(PRL_ERR_TIMEOUT);
}

void MigrateVmTarget::checkTargetCpusNumber()
{
	if (m_pVmConfig->getVmHardwareList()->getCpu()->getNumber() > m_cDstHostInfo.getCpu()->getNumber())
	{
		//Fill additional error info
		CVmEvent cEvent;
		cEvent.setEventCode(PRL_ERR_VM_MIGRATE_NOT_ENOUGH_CPUS_ON_TARGET);
		//Add required by VM CPUs number
		cEvent.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
					 QString("%1").arg(m_pVmConfig->getVmHardwareList()->getCpu()->getNumber()),
					 EVT_PARAM_MESSAGE_PARAM_0));
		//Add available at host CPUs number
		cEvent.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
					 QString("%1").arg(m_cDstHostInfo.getCpu()->getNumber()),
					 EVT_PARAM_MESSAGE_PARAM_1));
		m_lstCheckPrecondsErrors.append(cEvent.toString());
		WRITE_TRACE(DBG_FATAL,
			    "Not enough CPUs : VM requires %d CPUs, target server has %d only.",
			    m_pVmConfig->getVmHardwareList()->getCpu()->getNumber(),
			    m_cDstHostInfo.getCpu()->getNumber());
	}
}

void MigrateVmTarget::checkRequiresDiskSpace()
{
	PRL_RESULT nRetCode;
	PRL_UINT64 nFreeSpace = 0;

	if (m_nRequiresDiskSpace == 0)
		return;

	nRetCode = CFileHelper::GetDiskAvailableSpace(m_sVmDirPath, &nFreeSpace);
	if (PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL,
			    "CFileHelper::GetDiskAvailableSpace(%s) return %#x[%s], disk space check will skipped",
			    QSTR2UTF8(m_sTargetVmHomePath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return;
	}

	if (nFreeSpace < m_nRequiresDiskSpace)
	{
		WRITE_TRACE(DBG_FATAL,
			    "Not enough disk space on the destination server: requires %llu MB, available %llu MB",
			    m_nRequiresDiskSpace / 1024 / 1024, nFreeSpace / 1024 / 1024);
		//Fill additional error info
		CVmEvent cEvent;
		cEvent.setEventCode(PRL_ERR_VM_MIGRATE_NOT_ENOUGH_DISK_SPACE_ON_TARGET);
		//Add required by VM disk space in megabytes
		cEvent.addEventParameter(new CVmEventParameter(PVE::UInt64,
					 QString("%1").arg(m_nRequiresDiskSpace / 1024 / 1024),
					 EVT_PARAM_MESSAGE_PARAM_0));
		// Add available disk space in megabytes
		cEvent.addEventParameter(new CVmEventParameter(PVE::UInt64,
					 QString("%1").arg(nFreeSpace / 1024 / 1024),
					 EVT_PARAM_MESSAGE_PARAM_1));
		m_lstCheckPrecondsErrors.append(cEvent.toString());
	}
}

PRL_RESULT MigrateVmTarget::registerVmBeforeMigration()
{
	PRL_RESULT nRetCode;
	CVmDirectoryItem* pVmDirItem = new CVmDirectoryItem;
	//bool bNewVmInstance = false;

	pVmDirItem->setVmUuid(m_sVmUuid);
	pVmDirItem->setRegistered(PVE::VmRegistered);
	pVmDirItem->setValid(PVE::VmValid);
	pVmDirItem->setVmHome(m_sVmConfigPath);
	pVmDirItem->setVmName(m_sVmName);
	// FIXME set private according to actual flag value within VM config
	pVmDirItem->setIsPrivate(PVE::VmPublic);
	pVmDirItem->setRegisteredBy(getClient()->getUserName());
	pVmDirItem->setRegDateTime(QDateTime::currentDateTime());
	pVmDirItem->setChangedBy(getClient()->getUserName());
	pVmDirItem->setChangeDateTime(QDateTime::currentDateTime());

	if (PVMT_SWITCH_TEMPLATE & getRequestFlags())
		pVmDirItem->setTemplate(!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate());
	else
		pVmDirItem->setTemplate(m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate());

	nRetCode = DspVm::vdh().insertVmDirectoryItem(m_sVmDirUuid, pVmDirItem);
	if (PRL_FAILED(nRetCode))
	{
		CDspTaskFailure(*this).setCode(PRL_ERR_VM_MIGRATE_REGISTER_VM_FAILED)(m_sVmName, PRL_RESULT_TO_STRING(nRetCode));
		WRITE_TRACE(DBG_FATAL,
			    "Error occurred while register Vm %s with code [%#x][%s]",
			    QSTR2UTF8(m_sVmUuid), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		WRITE_TRACE(DBG_FATAL, "Can't insert vm %s into VmDirectory by error %#x, %s",
			    QSTR2UTF8(m_sVmUuid), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return PRL_ERR_VM_MIGRATE_REGISTER_VM_FAILED;
	}

	/* Notify clients that new VM appeared */
	CVmEvent event(PET_DSP_EVT_VM_ADDED, m_sVmUuid, PIE_DISPATCHER);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event);
	CDspService::instance()->getClientManager().sendPackageToVmClients(p, m_sVmDirUuid, m_sVmUuid);

	/* Notify clients that VM migration started - and clients wait message with original Vm uuid */
	SmartPtr<CVmEvent> pEvent = SmartPtr<CVmEvent>(
					    new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sOriginVmUuid, PIE_DISPATCHER));
	pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "false", EVT_PARAM_MIGRATE_IS_SOURCE));
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());
	/* change Vm state to VMS_MIGRATING */
	CDspService::instance()->getVmStateSender()->
	onVmStateChanged(VMS_STOPPED, VMS_MIGRATING, m_sVmUuid, m_sVmDirUuid, false);
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sOriginVmUuid);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT MigrateVmTarget::saveVmConfig()
{
	PRL_RESULT nRetCode;

	m_pVmConfig->getVmIdentification()->setVmUuid(m_sVmUuid);
	if (PVMT_CLONE_MODE & getRequestFlags())
		m_pVmConfig->getVmIdentification()->setSourceVmUuid(m_sOriginVmUuid);

	QString sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()
			      ->getVmServerIdentification()->getServerUuid();
	QString sLastServerUuid = m_pVmConfig->getVmIdentification()->getServerUuid();
	m_pVmConfig->getVmIdentification()->setServerUuid(sServerUuid);
	m_pVmConfig->getVmIdentification()->setLastServerUuid(sLastServerUuid);

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM))
	{
		if (PVMT_CLONE_MODE & getRequestFlags())
		{
			if (PVMT_SWITCH_TEMPLATE & getRequestFlags())
			{
				m_pVmConfig->getVmSettings()->getVmCommonOptions()->setTemplate(
					!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate());
			}

			Task_CloneVm::ResetNetSettings(m_pVmConfig);
		}

		// Try to create empty configuration file
		if (!CFileHelper::CreateBlankFile(m_sVmConfigPath, &getClient()->getAuthHelper()))
		{
			WRITE_TRACE(DBG_FATAL,
				    "Couldn't to create blank VM config by path '%s'", QSTR2UTF8(m_sVmConfigPath));
			return PRL_ERR_SAVE_VM_CONFIG;
		}
	}

	/**
	* reset additional parameters in VM configuration
	* (VM home, last change date, last modification date - never store in VM configuration itself!)
	*/
	CDspService::instance()->getVmDirHelper().resetAdvancedParamsFromVmConfig(m_pVmConfig);

	nRetCode = CDspService::instance()->getVmConfigManager().saveConfig(
			   m_pVmConfig, m_sVmConfigPath, getClient(), true, true);
	if (PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL, "Can't save VM config by error %#x, %s",
			    nRetCode, PRL_RESULT_TO_STRING(nRetCode));

		return CDspTaskFailure(*this).setCode(PRL_ERR_SAVE_VM_CONFIG)(m_sVmUuid, m_sTargetVmHomePath);
	}
	/* set original permissions of Vm config (https://jira.sw.ru/browse/PSBM-8333) */
	if (m_nConfigPermissions)
	{
		QFile vmConfig(m_sVmConfigPath);
		if (!vmConfig.setPermissions((QFile::Permissions)m_nConfigPermissions))
		{
			WRITE_TRACE(DBG_FATAL,
				    "[%s] Cannot set permissions for Vm config \"%s\", will use default",
				    __FUNCTION__, QSTR2UTF8(m_sVmConfigPath));
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT MigrateVmTarget::adjustStartVmCommand(SmartPtr<IOPackage>& pPackage)
{
	CVmMigrateStartCommand* pStartCmd;
	CVmEventParameter* pEventParam;

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(m_pStartPackage);
	if (!pCmd->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			    m_pStartPackage->buffers[0].getImpl());
		return PRL_ERR_FAILURE;
	}

	pStartCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);
	if (NULL == pStartCmd)
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			    m_pStartPackage->buffers[0].getImpl());
		return PRL_ERR_FAILURE;
	}

	/* to set path where Vm home directory will be create */
	pEventParam = pStartCmd->GetCommand()->getEventParameter(EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH);
	if (pEventParam)
		pEventParam->setParamValue(m_sVmDirPath);

	/* will send to Vm config from StartCmd, not from CheckCmd,
	   so it's config with valid runtime params (https://jira.sw.ru/browse/PSBM-11335) */
	QString sVmConfig = pStartCmd->GetVmRuntimeConfig();
	if (!sVmConfig.length())
	{
		WRITE_TRACE(DBG_WARNING, "source node didn't send us runtime config "
			    "(old version of software on the source node)");
		WRITE_TRACE(DBG_WARNING, "it is not an error, but you may experience migration "
			    "failures when on-disk VM config and runtime one are not in sync");
		sVmConfig = pStartCmd->GetVmConfig();
	}
	SmartPtr<CVmConfiguration> pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration(sVmConfig));
	/* rewrote request with config with real pathes */
	pVmConfig->getVmIdentification()->setVmName(m_sVmName);
	pVmConfig->getVmIdentification()->setHomePath(m_sVmConfigPath);
	WRITE_TRACE(DBG_DEBUG, "set home path '%s'", qPrintable(m_sVmConfigPath));
	pVmConfig->getVmHardwareList()->RevertDevicesPathToAbsolute(m_sTargetVmHomePath);

	pEventParam = pStartCmd->GetCommand()->getEventParameter(EVT_PARAM_MIGRATE_CMD_VM_CONFIG);
	if (pEventParam)
	{
		WRITE_TRACE(DBG_INFO, "set updated config to the package");
		pEventParam->setParamValue(pVmConfig->toString());
	}

	/* add network config into request */
	pEventParam = pStartCmd->GetCommand()->getEventParameter(EVT_PARAM_MIGRATE_CMD_NETWORK_CONFIG);
	if (pEventParam)
		pEventParam->setParamValue(CDspService::instance()->getNetworkConfig()->toString());

	/* to add dispatcher config */
	pEventParam = pStartCmd->GetCommand()->getEventParameter(EVT_PARAM_MIGRATE_CMD_DISPATCHER_CONFIG);
	if (pEventParam)
		pEventParam->setParamValue(
			CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->toString());

	pPackage = DispatcherPackage::duplicateInstance(m_pStartPackage, pStartCmd->GetCommand()->toString());

	return PRL_ERR_SUCCESS;
}

// add/move resource on HA cluster
PRL_RESULT MigrateVmTarget::registerHaClusterResource()
{
	PRL_RESULT nRetCode;

	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_SUCCESS;

	CVmHighAvailability* pHighAvailability = m_pVmConfig->getVmSettings()->getHighAvailability();

	if (!pHighAvailability->isEnabled())
		return PRL_ERR_SUCCESS;

	if (m_nReservedFlags & PVM_HA_MOVE_VM)
	{
		// move resource from source node
		nRetCode = CDspService::instance()->getHaClusterHelper()->moveFromClusterResource(
				   m_sVmName, m_sHaClusterId);
	}
	else
	{
		// add resource
		nRetCode = CDspService::instance()->getHaClusterHelper()->addClusterResource(
				   m_sVmName, pHighAvailability, m_sTargetVmHomePath);
	}
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	m_nSteps |= MIGRATE_HA_RESOURCE_REGISTERED;
	return PRL_ERR_SUCCESS;
}

// rollback resource on HA cluster
void MigrateVmTarget::unregisterHaClusterResource()
{
	if (!m_pVmConfig->getVmSettings()->getHighAvailability()->isEnabled())
		return;

	if (!(m_nSteps & MIGRATE_HA_RESOURCE_REGISTERED))
		return;

	if (m_nReservedFlags & PVM_HA_MOVE_VM)
	{
		// resource was moved - return to source node
		CDspService::instance()->getHaClusterHelper()->moveToClusterResource(m_sVmName, m_sHaClusterId);
	}
	else
	{
		// resource was added
		CDspService::instance()->getHaClusterHelper()->removeClusterResource(m_sVmName, false);
	}
}

void MigrateVmTarget::checkRemoteDisplay()
{
	CVmRemoteDisplay* r = m_pVmConfig->getVmSettings()->getVmRemoteDisplay();
	if (NULL == r || r->getMode() != PRD_MANUAL || r->getPortNumber() >= PRL_VM_REMOTE_DISPAY_MIN_PORT)
		return;

	CVmEvent cEvent;
	cEvent.setEventCode(PRL_ERR_VZ_OPERATION_FAILED);
	cEvent.addEventParameter(new CVmEventParameter(PVE::String,
		QString("Virtuozzo %1 doesn't support VNC port less than %2.")
		.arg(VER_FULL_BUILD_NUMBER_STR)
		.arg(PRL_VM_REMOTE_DISPAY_MIN_PORT),
		EVT_PARAM_MESSAGE_PARAM_0));
	m_lstCheckPrecondsErrors.append(cEvent.toString());
}

void MigrateVmTarget::checkEfiBoot()
{
	if (!m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios()
		|| !m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled()
		|| m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion() != PVS_GUEST_VER_WIN_2008)
		return;

	CVmEvent cEvent;
	cEvent.setEventCode(PRL_ERR_VZ_OPERATION_FAILED);
	cEvent.addEventParameter(new CVmEventParameter(PVE::String,
		QString("The requested VM has EFI boot enabled. "
			"Migration of EFI bootloader to Virtuozzo %1 is not supported.")
		.arg(VER_FULL_BUILD_NUMBER_STR),
		EVT_PARAM_MESSAGE_PARAM_0));
	m_lstCheckPrecondsErrors.append(cEvent.toString());
}

} // namespace Task
} // namespace Legacy
