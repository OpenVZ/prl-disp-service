///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmTarget.cpp
///
/// Target task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "Interfaces/Debug.h"
#include "Interfaces/ParallelsQt.h"
#include "Interfaces/ParallelsNamespace.h"

#include "Libraries/Logging/Logging.h"
#include "Libraries/StatesStore/SavedStateTree.h"
#include "Libraries/Std/PrlTime.h"

#include "CDspVmDirHelper.h"
#include "Task_MigrateVmTarget.h"
#include "Task_RegisterVm.h"
#include "Task_CloneVm.h"
#include "Task_ChangeSID.h"
#include "CDspService.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif
#include "Mixin_CreateVmSupport.h"
#include "Task_ManagePrlNetService.h"
#include "Task_BackgroundJob.h"
#include "CDspService.h"
#include "Libraries/PrlCommonUtils/CVmMigrateHelper.h"

#define VM_MIGRATE_START_CMD_WAIT_TIMEOUT 600 * 1000

Task_MigrateVmTarget::Task_MigrateVmTarget(
		const QObject *parent,
		const SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p)
:CDspTaskHelper(pDispConnection->getUserSession(), p),
Task_DispToDispConnHelper(getLastError()),
m_pParent(parent),
m_pCheckDispConnection(pDispConnection),
m_pCheckPackage(p),
m_cDstHostInfo(CDspService::instance()->getHostInfo()->data()),
m_nFlags(0),
m_nSteps(0),
m_nBundlePermissions(0),
m_nConfigPermissions(0)
{
	CVmMigrateCheckPreconditionsCommand * pCheckCmd =
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
		m_sVmDirPath = m_pCheckDispConnection->getUserSession()->getUserDefaultVmDirPath();
	else
		m_sVmDirPath = pCheckCmd->GetTargetVmHomePath();
	/* initialize all vars from pCheckCmd - after exit from constructor package buffer will invalid */

	if (m_nVersion >= MIGRATE_DISP_PROTO_V3)
	{
		/* in m_nVersion < MIGRATE_DISP_PROTO_V3 dispatcher make disconnect after
		   prereq reply receiving, than vm_app make connect to target dispatche before start req sending
		   but target disp reattache this connection to new vn_app, so in old protocol target disp
		   have not any connection diring migration.
		   Since V3 source dispatcher close connection on finalizeTask() only, so
		   we can process disconnect from here
		   https://jira.sw.ru/browse/PSBM-6571
		*/
		bool bConnected = QObject::connect(
			&CDspService::instance()->getIOServer(),
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			SLOT(clientDisconnected(IOSender::Handle)),
			Qt::DirectConnection);
		PRL_ASSERT(bConnected);
	}

}

Task_MigrateVmTarget::~Task_MigrateVmTarget()
{
	// #439777 protect handler from destroying object
	m_waiter.waitUnlockAndFinalize();
}

/* process VmMigrateCheckPreconditionsCommand */
PRL_RESULT Task_MigrateVmTarget::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

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
	m_sVmUuid = (!(m_nReservedFlags & PVM_DONT_COPY_VM) && (PVMT_CLONE_MODE & getRequestFlags()))
			? Uuid::createUuid().toString() : m_sOriginVmUuid;

	m_cSrcHostInfo.fromString(m_sSrcHostInfo);
	if (PRL_FAILED(m_cSrcHostInfo.m_uiRcInit))
	{
		nRetCode = PRL_ERR_PARSE_HOST_HW_INFO;
		WRITE_TRACE(DBG_FATAL, "Wrong source host hw info was received: [%s]", QSTR2UTF8(m_sSrcHostInfo));
		goto exit;
	}

	/*  to get target VM home directory path */
	m_sTargetVmHomePath = QString("%1/%2%3").arg(m_sVmDirPath).arg(m_sVmName).arg(VMDIR_DEFAULT_BUNDLE_SUFFIX);
	m_sTargetVmHomePath = QFileInfo(m_sTargetVmHomePath).absoluteFilePath();
	m_sVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath);

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
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
			WRITE_TRACE(DBG_FATAL, "name '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED:
			WRITE_TRACE(DBG_FATAL, "container '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				QSTR2UTF8(m_pVmInfo->vmUuid), QSTR2UTF8(m_pVmInfo->vmName), QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
		}
		goto exit;
	}
	m_nFlags |= MIGRATE_VM_EXCL_PARAMS_LOCKED;

	if (!(PVMT_SWITCH_TEMPLATE & getRequestFlags())) {
		/* skip checking for copy to template case (https://jira.sw.ru/browse/PSBM-9597) */
		checkTargetCpusNumber();
		checkTargetCpuCompatibility();
	}

	foreach (const QString & checkFile, m_lstSharedFileNamesExt) {
		QFileInfo fi(checkFile);
		if (!fi.exists()) {
			// we use independent dir separators in protocol
			m_lstNonSharedDisks.append(QDir::fromNativeSeparators(fi.absolutePath()));
			WRITE_TRACE(DBG_FATAL, "External disk %s is on private media.",
				    QSTR2UTF8(fi.absolutePath()));
		}
	}

	// check that external disk paths are not exist
	foreach (const QString & disk, m_lstNonSharedDisks) {
		if (!QDir(disk).exists())
			continue;
		nRetCode = PRL_ERR_VM_MIGRATE_EXT_DISK_DIR_ALREADY_EXISTS_ON_TARGET;
		WRITE_TRACE(DBG_FATAL,
			"The directory for external disk '%s' already exists.", QSTR2UTF8(disk));
		getLastError()->setEventCode(PRL_ERR_VM_MIGRATE_EXT_DISK_DIR_ALREADY_EXISTS_ON_TARGET);
		getLastError()->addEventParameter(new CVmEventParameter(PVE::String, disk,
			EVT_PARAM_MESSAGE_PARAM_0));
		goto exit;
	}

	{
		/*
		   Shared storage should be checked before target VM home existence
		   because latest will fail if storage is shared.
		   checkSharedStorage can return PRL_ERR_SUCCESS or PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED only.
		   In last case event with code PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED will added into m_lstCheckPrecondsErrors.
		   Source side will check m_lstCheckPrecondsErrors.
		*/
		if (PRL_ERR_SUCCESS == checkSharedStorage())
		{
			/* it is not shared based Vm */
			if (!(m_nMigrationFlags & PVMT_IGNORE_EXISTING_BUNDLE) && QDir(m_sTargetVmHomePath).exists())
			{
				nRetCode = PRL_ERR_VM_MIGRATE_VM_HOME_ALREADY_EXISTS_ON_TARGET;
				WRITE_TRACE(DBG_FATAL,
					"The virtual machine home directory '%s' already exists.",
					QSTR2UTF8(m_sTargetVmHomePath));
				CVmEvent *pEvent = getLastError();
				pEvent->setEventCode(PRL_ERR_VM_MIGRATE_VM_HOME_ALREADY_EXISTS_ON_TARGET);
				pEvent->addEventParameter(new CVmEventParameter(PVE::String,
					m_sTargetVmHomePath,
					EVT_PARAM_MESSAGE_PARAM_0));
				goto exit;
			}
			checkRequiresDiskSpace();
		}
	}
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MigrateVmTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	bool bConnected;
	QTimer *pTimer;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	bConnected = QObject::connect(m_pParent,
		SIGNAL(onPackageReceived(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)),
		SLOT(handleStartPackage(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	/* and send reply on Precond Check command */
	if (m_nVersion < MIGRATE_DISP_PROTO_V2)
	{
		/* for proto v1 send DispToDispResponseCmd */
		if (m_lstCheckPrecondsErrors.size())
		{
			pReply = CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
					PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED,
					m_pCheckPackage,
					m_lstCheckPrecondsErrors
				);
			hJob = m_pCheckDispConnection->sendResponse(pReply, m_pCheckPackage);
		}
		else
		{
			//Drop checking step on success execution
			hJob = m_pCheckDispConnection->sendSimpleResponse(m_pCheckPackage, PRL_ERR_SUCCESS);
		}
	}
	else
	{
		/* for protocol v.2 and later will send VmMigrateCheckPreconditionsReply */
		pReply = CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsReply(
				m_lstCheckPrecondsErrors, m_lstNonSharedDisks, m_nFlags);
		SmartPtr<IOPackage> pPackage =
			DispatcherPackage::createInstance(
				pReply->GetCommandId(), pReply->GetCommand()->toString(), m_pCheckPackage);
		hJob = m_pCheckDispConnection->sendPackage(pPackage);
	}
	if (!hJob.isValid()) {
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
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)),
		this,
		SLOT(handleStartPackage(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)));

	if (PRL_FAILED(nRetCode))
		goto exit;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) ) {
		/* to create Vm bundle */
		if (!CFileHelper::WriteDirectory(m_sTargetVmHomePath, &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(
				new CVmEventParameter(PVE::String, m_sTargetVmHomePath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory",
					__FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			goto exit;
		}
		/* set original permissions to Vm bundle (https://jira.sw.ru/browse/PSBM-8269) */
		if (m_nBundlePermissions) {
			QFile vmBundle(m_sTargetVmHomePath);
			if (!vmBundle.setPermissions((QFile::Permissions)m_nBundlePermissions)) {
				WRITE_TRACE(DBG_FATAL,
					"[%s] Cannot set permissions for Vm bundle \"%s\", will use default",
					__FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			}
		}
	}

	// create directories for external disks
	foreach (const QString & disk, m_lstNonSharedDisks) {
		WRITE_TRACE(DBG_FATAL, "[%s] Creating disk '%s'",
					__FUNCTION__, QSTR2UTF8(disk));
		// first create try to create parent directories
		// then create disk dir itself, so if it is created meanwhile by
		// someone else we get an error
		if (CFileHelper::WriteDirectory(QFileInfo(disk).absolutePath(), &getClient()->getAuthHelper()) &&
			CFileHelper::CreateDirectoryPath(disk, &getClient()->getAuthHelper()))
			continue;

		nRetCode = PRL_ERR_VM_MIGRATE_CANNOT_CREATE_DIRECTORY;
		getLastError()->setEventCode(nRetCode);
		getLastError()->addEventParameter(
			new CVmEventParameter(PVE::String, disk, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory",
				__FUNCTION__, QSTR2UTF8(disk));
		goto exit;
	}

	m_nSteps |= MIGRATE_STARTED;
	if ((m_nPrevVmState == VMS_RUNNING) || (m_nPrevVmState == VMS_PAUSED)) {
		nRetCode = migrateRunningVm();
	} else {
		nRetCode = migrateStoppedVm();
	}
	if (m_nVersion >= MIGRATE_DISP_PROTO_V3)
	{
		/* migration completed and connection state is indifferent for us
		   https://jira.sw.ru/browse/PSBM-8925 */
		QObject::disconnect(
			&CDspService::instance()->getIOServer(),
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			this,
			SLOT(clientDisconnected(IOSender::Handle)));
	}
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

void Task_MigrateVmTarget::changeSID()
{
	// nobody requested SID change
	if (!(PVMT_CHANGE_SID & getRequestFlags()) &&
		!(PVMT_CLONE_MODE & getRequestFlags()))
		return;

	// VM is on shared storage
	if (m_nReservedFlags & PVM_DONT_COPY_VM)
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

void Task_MigrateVmTarget::finalizeTask()
{
	IOSendJob::Handle hJob;
	SmartPtr<IOPackage> pPackage;

	// delete temporary registration
	if (m_nFlags & MIGRATE_VM_EXCL_PARAMS_LOCKED)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		PRL_ASSERT(m_pVm);

		if (m_sSnapshotUuid.size())
			DeleteSnapshot();

		// leave previously running VM in suspended state
		if (m_nPrevVmState == VMS_RUNNING && (m_nMigrationFlags & PVM_DONT_RESUME_VM))
			m_nPrevVmState = VMS_SUSPENDED;

		if (m_nPrevVmState == VMS_RUNNING)
		{
			Task_ManagePrlNetService::addVmIPAddress(m_pVmConfig);
			Task_ManagePrlNetService::updateVmNetworking(m_pVmConfig, true);
			announceMacAddresses(m_pVmConfig);
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
			DispatcherPackage::createInstance( PVE::DspVmEvent, cStateEvent.toString());
		m_pVm->changeVmState(pUpdateVmStatePkg);
		CDspService::instance()->getClientManager().
			sendPackageToVmClients(pUpdateVmStatePkg, m_sVmDirUuid, m_sVmUuid);

		if (m_nPrevVmState == VMS_STOPPED || m_nPrevVmState == VMS_SUSPENDED) {
			/*
			 * The operation is registered in m_pVm's constructor and will be
			 * unregistered as a part of m_pVm's destructor. Doing it here
			 * produces error message.
			 * See https://jira.sw.ru/browse/PSBM-13523.
			 */
			CDspVm::UnregisterVmObject(m_pVm);
			// to call ~CDspVm (and unregisterExclusineOperation()) before sendResponce()
			// https://jira.sw.ru/browse/PSBM-15151
			m_pVm->disableStoreRunningState(true);
			m_pVm = SmartPtr<CDspVm>(0);

			/* This task should be run with destroyed m_pVm
			 * (for correct guest tools state retrieval from
			 * the disk) and with unlocked VM */
			 changeSID();
		} else {
			/* lock running Vm (https://jira.sw.ru/browse/PSBM-7682) */
			/* will do it via initial Vm creation command
			   (https://jira.sw.ru/browse/PSBM-8222) */
			m_pVm->replaceInitDspCmd(PVE::DspCmdVmStart, getClient());

			if (m_nPrevVmState == VMS_RUNNING &&
				(m_pVmConfig->getVmSettings()->getVmRemoteDisplay()->getMode() != PRD_DISABLED))
			{
				/* migration skip VNC start on Vm resuming (#476822) so will
				   to start VNC right after migration unregister */
				pPackage = IOPackage::createInstance( IOSender::UnknownType, 0 );
				m_pVm->startVNCServer(getClient(), pPackage, false, true);
			}
			m_pVm->setMigrateVmRequestPackage(SmartPtr<IOPackage>());
			m_pVm->setMigrateVmConnection(SmartPtr<CDspDispConnection>());
		}

		if (m_nVersion >= MIGRATE_DISP_PROTO_V3) {
			/* notify source task about target task finish,
			   will send reply to check precondition request. https://jira.sw.ru/browse/PSBM-9596 */
			m_pCheckDispConnection->sendSimpleResponse(m_pCheckPackage, PRL_ERR_SUCCESS);
		}
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

			if (m_nSteps & MIGRATE_VM_APP_STARTED && m_pVm.getImpl())
			{
				/* stop Vm */
				CProtoCommandPtr pCmd =
					CProtoSerializer::CreateProtoVmCommandStop(m_sVmUuid, PSM_KILL, 0);
				pPackage = DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
				m_pVm->stop(getClient(), pPackage, PSM_KILL, true);
				// Wait until VM stopped
				while (m_pVm->isConnected())
				{
					if (operationIsCancelled() && CDspService::instance()->isServerStopping())
						break;
					QThread::msleep(1000);
				}
			}
			if (m_pVm.getImpl()) {
				CDspVm::UnregisterVmObject(m_pVm);
				m_pVm = SmartPtr<CDspVm>(0);
			}

			if (!CDspService::instance()->isServerStopping())
				CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(m_sTargetVmHomePath);
			if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
				CFileHelper::ClearAndDeleteDir(m_sTargetVmHomePath);
			foreach (const QString & disk, m_lstNonSharedDisks)
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

PRL_RESULT Task_MigrateVmTarget::migrateStoppedVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;

#if _LIN_
	CVzHelper vz;
	if (vz.set_vziolimit("VZ_TOOLS"))
		WRITE_TRACE(DBG_FATAL, "Warning: Ignore IO limit parameters");
#endif

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM))
		if (PRL_FAILED(nRetCode = saveVmConfig()))
			return nRetCode;

	/* will register resource on HA cluster just before
	   register CT on node : better to have a broken resource than
	   losing a valid */
	nRetCode = registerHaClusterResource();
	if ( PRL_FAILED( nRetCode ) )
		return nRetCode;

	nRetCode = registerVmBeforeMigration();
	if ( PRL_FAILED( nRetCode ) )
		return nRetCode;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(
			CDspService::instance()->getIOServer(),
			m_pStartDispConnection->GetConnectionHandle()));
	/* CVmFileListCopyTarget will use Vm uuid in progress messages for clients, so will use original Vm uuid */
	m_pVmMigrateTarget = SmartPtr<CVmFileListCopyTarget>(
		new CVmFileListCopyTarget(m_pSender.getImpl(), m_sOriginVmUuid, m_sTargetVmHomePath, NULL, m_nTimeout));
/*
TODO - send START CANCEL FINISH events from this task
	m_pVmMigrateTarget->SetCancelNotifySender((m_nReservedFlags & PVM_DONT_COPY_VM)?
						  NotifyClientWithCancelShared:
						  NotifyClientWithCancel);
	m_pVmMigrateTarget->SetFinishNotifySender((m_nReservedFlags & PVM_DONT_COPY_VM)?
						  NotifyClientWithFinishShared:
						  NotifyClientWithFinish);
*/

	// Connect to handle traffic report package
	bool bConnected = QObject::connect(
		m_pStartDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	UINT64 uMemSize = m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize()*1024*1024 +
	           m_pVmConfig->getVmHardwareList()->getVideo()->getMemorySize()*1024*1024;

	QString sMemFilePath = ParallelsDirs::getVmMemoryFileLocation(
		m_sVmUuid,
		m_sTargetVmHomePath,
		m_pVmConfig->getVmSettings()->getVmCommonOptions()->getSwapDir(),
		CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->getSwapPathForVMOnNetworkShares(),
		false,
		uMemSize);
	if (sMemFilePath == m_sTargetVmHomePath)
		// set empty string for default path
		pReply = CDispToDispProtoSerializer::CreateVmMigrateReply(QString());
	else
		pReply = CDispToDispProtoSerializer::CreateVmMigrateReply(sMemFilePath);
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

PRL_RESULT Task_MigrateVmTarget::migrateRunningVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> pPackage;

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM))
		if (PRL_FAILED(nRetCode = saveVmConfig()))
			return nRetCode;

	/* will register resource on HA cluster just before
	   register CT on node : better to have a broken resource than
	   losing a valid */
	nRetCode = registerHaClusterResource();
	if ( PRL_FAILED( nRetCode ) )
		return nRetCode;

	nRetCode = registerVmBeforeMigration();
	if ( PRL_FAILED( nRetCode ) )
		return nRetCode;

	nRetCode = adjustStartVmCommand(pPackage);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	/* set signal handler before Vm start - to avoid race */
	if (!QObject::connect(CDspService::instance(),
		SIGNAL(onVmMigrateEventReceived(const QString &, const SmartPtr<IOPackage> &)),
		SLOT(handleVmMigrateEvent(const QString &, const SmartPtr<IOPackage> &)),
		Qt::DirectConnection))
	{
		WRITE_TRACE(DBG_FATAL, "QObject::connect() failed");
		return PRL_ERR_OPERATION_FAILED;
	}

	/* for handshake with vm_app. ! before startMigratedVm() */
	m_pVm->setMigrateVmRequestPackage(pPackage);
	m_pVm->setMigrateVmConnection(m_pStartDispConnection);

	nRetCode = CDspService::instance()->getAccessManager().checkAccess(
				getClient(), PVE::DspCmdCtlMigrateTarget, m_sVmUuid);
	if (PRL_FAILED(nRetCode))
	{
		if ((nRetCode == PRL_ERR_ACCESS_TO_VM_DENIED) && (m_nReservedFlags & PVM_DONT_COPY_VM))
		{
			/* user has not permissions to start shared Vm */
			nRetCode = PRL_ERR_VM_MIGRATE_ACCESS_TO_VM_DENIED;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, getClient()->getAuthHelper().getUserName(), EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL,
				"User %s has not enough permissions in shared Vm bundle %s to start online migration",
				QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sTargetVmHomePath));
		}
		goto cleanup;
	}

	if (!m_pVm->startMigratedVm(getClient(), pPackage))
	{
		WRITE_TRACE(DBG_FATAL, "Vm %s start failed", QSTR2UTF8(m_sVmUuid));
		nRetCode = PRL_ERR_VM_START_FAILED;
		goto cleanup;
	}
	m_nSteps |= MIGRATE_VM_APP_STARTED;
	/* m_pStartDispConnection is not valid */
	nRetCode = exec();

cleanup:
	QObject::disconnect(CDspService::instance(),
		SIGNAL(onVmMigrateEventReceived(const QString &, const SmartPtr<IOPackage> &)),
		this,
		SLOT(handleVmMigrateEvent(const QString &, const SmartPtr<IOPackage> &)));

	return nRetCode;

}

void Task_MigrateVmTarget::handleVmMigrateEvent(const QString &sVmUuid, const SmartPtr<IOPackage> &p)
{
	if (operationIsCancelled())
		return;

	if (sVmUuid != m_sVmUuid)
		return;

	if (p->header.type != PVE::DspVmEvent) {
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

void Task_MigrateVmTarget::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;
	PRL_ASSERT(m_pVmMigrateTarget.getImpl());

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pStartDispConnection->GetConnectionHandle())
		return;

	if (IS_FILE_COPY_PACKAGE(p->header.type)) {
		nRetCode = m_pVmMigrateTarget->handlePackage(p, &bExit);
		if (bExit)
			QThread::exit(nRetCode);
	} else if (p->header.type == VmMigrateCancelCmd) {
		WRITE_TRACE(DBG_DEBUG, "Migration was cancelled");
		QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
	} else {
		WRITE_TRACE(DBG_FATAL, "Invalid package type %d, ignored", p->header.type);
	}
}

void Task_MigrateVmTarget::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (m_nSteps & MIGRATE_VM_APP_STARTED)
	{
		if (m_pVm->getVmConnectionHandle() != h)
			return;
	}
	else if (h != m_hConnHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	cancelOperation(nullClient, getRequestPackage());
}

void Task_MigrateVmTarget::handleStartPackage(
		const SmartPtr<CDspDispConnection> &pDispConnection,
		const QString &sVmUuid,
		const SmartPtr<IOPackage> &p)
{
// ??? LOCK
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (operationIsCancelled())
		return;

	/* this function will handle StartMigrateCommand with _original_ Vm uuid */
	if (m_sOriginVmUuid != sVmUuid)
		return;

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( !pCmd->IsValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			p->buffers[0].getImpl());
		QThread::exit(PRL_ERR_FAILURE);
	}

	CVmMigrateStartCommand *pStartCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);
	if ( NULL == pStartCmd )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			p->buffers[0].getImpl());
		QThread::exit(PRL_ERR_FAILURE);
	}
	m_nMigrationFlags = pStartCmd->GetMigrationFlags();
	m_nReservedFlags = pStartCmd->GetReservedFlags();
	m_sSnapshotUuid = pStartCmd->GetSnapshotUuid();
	if (m_nVersion < MIGRATE_DISP_PROTO_V3) {
		m_nPrevVmState = pStartCmd->GetVmPrevState();
	} else {
		m_nBundlePermissions = pStartCmd->GetBundlePermissions();
		m_nConfigPermissions = pStartCmd->GetConfigPermissions();
	}

/* TODO : compare m_nFlags  and m_nReservedFlags */

	/* reassign task to new connection and package and return to main thread */
	SmartPtr<CDspClient> pClient = pDispConnection->getUserSession();
	reassignTask( pClient, p );
	m_pStartDispConnection = pDispConnection;
	/* attn: do not use getRequestPackage() after this - reassignTask() changed type of new package */
	m_pStartPackage = IOPackage::duplicateInstance(p);

	QThread::exit(0);
}

// cancel command
void Task_MigrateVmTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	if (m_pVmMigrateTarget.isValid())
		m_pVmMigrateTarget->cancelOperation();

	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

void Task_MigrateVmTarget::handleStartCommandTimeout()
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	WRITE_TRACE( DBG_DEBUG, "StartMigrateCommand timeout" );
	QThread::exit(PRL_ERR_TIMEOUT);
}

void Task_MigrateVmTarget::checkTargetCpusNumber()
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

void Task_MigrateVmTarget::checkTargetCpuCompatibility()
{
	if (	m_cSrcHostInfo.getCpu()->getModel().contains("Intel") !=
		m_cDstHostInfo.getCpu()->getModel().contains("Intel"))
	{
		//Fill additional error info
		CVmEvent cEvent;
		cEvent.setEventCode(PRL_ERR_VM_MIGRATE_NON_COMPATIBLE_CPU_ON_TARGET);
		//Add source host CPU model
		cEvent.addEventParameter(new CVmEventParameter(PVE::String,
			m_cSrcHostInfo.getCpu()->getModel(),
			EVT_PARAM_MESSAGE_PARAM_0));
		//Add target host CPU model
		cEvent.addEventParameter(new CVmEventParameter(PVE::String,
			m_cDstHostInfo.getCpu()->getModel(),
			EVT_PARAM_MESSAGE_PARAM_1));
		m_lstCheckPrecondsErrors.append(cEvent.toString());
		WRITE_TRACE(DBG_FATAL,
			"non compatible CPUs : %s on source, %s on target",
			QSTR2UTF8(m_cSrcHostInfo.getCpu()->getModel()),
			QSTR2UTF8(m_cDstHostInfo.getCpu()->getModel()));
	}
}

void Task_MigrateVmTarget::checkRequiresDiskSpace()
{
	PRL_RESULT nRetCode;
	PRL_UINT64 nFreeSpace = 0;

	if (m_nRequiresDiskSpace == 0)
		return;

	nRetCode = CFileHelper::GetDiskAvailableSpace( m_sVmDirPath, &nFreeSpace );
	if ( PRL_FAILED(nRetCode) ) {
		WRITE_TRACE(DBG_FATAL,
			"CFileHelper::GetDiskAvailableSpace(%s) return %#x[%s], disk space check will skipped",
			QSTR2UTF8(m_sTargetVmHomePath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return;
	}

	if (nFreeSpace < m_nRequiresDiskSpace)
	{
		WRITE_TRACE(DBG_FATAL,
			"Not enough disk space on the destination server: requires %llu MB, available %llu MB",
			m_nRequiresDiskSpace/1024/1024, nFreeSpace/1024/1024);
		//Fill additional error info
		CVmEvent cEvent;
		cEvent.setEventCode(PRL_ERR_VM_MIGRATE_NOT_ENOUGH_DISK_SPACE_ON_TARGET);
		//Add required by VM disk space in megabytes
		cEvent.addEventParameter(new CVmEventParameter(PVE::UInt64,
			QString("%1").arg(m_nRequiresDiskSpace/1024/1024),
			EVT_PARAM_MESSAGE_PARAM_0));
		// Add available disk space in megabytes
		cEvent.addEventParameter(new CVmEventParameter(PVE::UInt64,
			QString("%1").arg(nFreeSpace/1024/1024),
			EVT_PARAM_MESSAGE_PARAM_1));
		m_lstCheckPrecondsErrors.append(cEvent.toString());
	}
}

PRL_RESULT Task_MigrateVmTarget::checkSharedStorage()
{
	if (m_sSharedFileName.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL,"Missed filename for shared storage check!");
		return PRL_ERR_SUCCESS;
	}

	QFileInfo sharedFileInfo(QString("%1/%2")
				 .arg(m_sTargetVmHomePath)
				 .arg(m_sSharedFileName));

	if( !sharedFileInfo.exists() )
	{
		if (CVmMigrateHelper::isInsideSharedVmPrivate(m_sTargetVmHomePath))
		{
			CVmEvent e;
			e.setEventCode(PRL_ERR_VM_MIGRATE_TARGET_INSIDE_SHARED_VM_PRIVATE);
			m_lstCheckPrecondsErrors.append(e.toString());
			return e.getEventCode();
		}
		WRITE_TRACE(DBG_FATAL,
			    "Found no %s file on target server. Storage is NOT shared.",
			    QSTR2UTF8(sharedFileInfo.fileName()));
		return PRL_ERR_SUCCESS;
	}

	WRITE_TRACE(DBG_FATAL,
		"Found %s file on target server. Storage IS shared.", QSTR2UTF8(sharedFileInfo.fileName()));

	//Fill additional error info
	CVmEvent cEvent;
	cEvent.setEventCode(PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED);
	m_lstCheckPrecondsErrors.append(cEvent.toString());

	return PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED;
}

PRL_RESULT Task_MigrateVmTarget::registerVmBeforeMigration()
{
	PRL_RESULT nRetCode;
	CVmDirectoryItem *pVmDirItem = new CVmDirectoryItem;
	bool bNewVmInstance = false;

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

	if ( PVMT_SWITCH_TEMPLATE & getRequestFlags() )
		pVmDirItem->setTemplate( !m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );
	else
		pVmDirItem->setTemplate( m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );

	nRetCode = CDspService::instance()->getVmDirHelper().insertVmDirectoryItem(m_sVmDirUuid, pVmDirItem);
	if (PRL_FAILED(nRetCode))
	{
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(PRL_ERR_VM_MIGRATE_REGISTER_VM_FAILED);
		pEvent->addEventParameter(new CVmEventParameter(
			PVE::String, m_sVmName, EVT_PARAM_MESSAGE_PARAM_0));
		pEvent->addEventParameter(new CVmEventParameter(
			PVE::String, PRL_RESULT_TO_STRING(nRetCode), EVT_PARAM_MESSAGE_PARAM_1));
		WRITE_TRACE(DBG_FATAL,
			"Error occurred while register Vm %s with code [%#x][%s]",
			QSTR2UTF8(m_sVmUuid), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		WRITE_TRACE(DBG_FATAL, "Can't insert vm %s into VmDirectory by error %#x, %s",
			QSTR2UTF8(m_sVmUuid), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return PRL_ERR_VM_MIGRATE_REGISTER_VM_FAILED;
	}

	m_pVm = CDspVm::CreateInstance(m_sVmUuid, getClient()->getVmDirectoryUuid(),
				nRetCode, bNewVmInstance, getClient(), PVE::DspCmdDirVmMigrate);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while CDspVm::CreateInstance() with code [%#x][%s]",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}
	if (!m_pVm.getImpl()) {
		WRITE_TRACE(DBG_FATAL, "[%s] Unknown CDspVm::CreateInstance() error", __FUNCTION__);
		return PRL_ERR_OPERATION_FAILED;
	}
	if (!bNewVmInstance) {
		WRITE_TRACE(DBG_FATAL, "Migrate: New CDspVm instance already exists!");
		return PRL_ERR_OPERATION_FAILED;
	}

	/* Notify clients that new VM appeared */
	CVmEvent event(PET_DSP_EVT_VM_ADDED, m_sVmUuid, PIE_DISPATCHER );
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event);
	CDspService::instance()->getClientManager().sendPackageToVmClients(p, m_sVmDirUuid, m_sVmUuid);

 	/* Notify clients that VM migration started - and clients wait message with original Vm uuid */
	SmartPtr<CVmEvent> pEvent = SmartPtr<CVmEvent>(
		new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sOriginVmUuid, PIE_DISPATCHER));
	pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "false", EVT_PARAM_MIGRATE_IS_SOURCE));
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());
	/* change Vm state to VMS_MIGRATING */
	m_pVm->changeVmState(pPackage);
	/* and notify clients about VM migration start event */
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sOriginVmUuid);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_MigrateVmTarget::saveVmConfig()
{
	PRL_RESULT nRetCode;

	m_pVmConfig->getVmIdentification()->setVmUuid( m_sVmUuid );
	if ( PVMT_CLONE_MODE & getRequestFlags() )
		m_pVmConfig->getVmIdentification()->setSourceVmUuid( m_sOriginVmUuid );

	QString sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid();
	QString sLastServerUuid = m_pVmConfig->getVmIdentification()->getServerUuid();
	m_pVmConfig->getVmIdentification()->setServerUuid(sServerUuid);
	m_pVmConfig->getVmIdentification()->setLastServerUuid(sLastServerUuid);

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM)) {
		if ( PVMT_CLONE_MODE & getRequestFlags() )
		{
			if ( PVMT_SWITCH_TEMPLATE & getRequestFlags() )
				m_pVmConfig->getVmSettings()->getVmCommonOptions()->setTemplate(
					!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );

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
			    nRetCode, PRL_RESULT_TO_STRING(nRetCode) );

		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(PRL_ERR_SAVE_VM_CONFIG);
		pEvent->addEventParameter(new CVmEventParameter(
				      PVE::String, m_sVmUuid, EVT_PARAM_MESSAGE_PARAM_0));
		pEvent->addEventParameter(new CVmEventParameter(
				      PVE::String, m_sTargetVmHomePath, EVT_PARAM_MESSAGE_PARAM_1));
		return PRL_ERR_SAVE_VM_CONFIG;
	}
	/* set original permissions of Vm config (https://jira.sw.ru/browse/PSBM-8333) */
	if (m_nConfigPermissions) {
		QFile vmConfig(m_sVmConfigPath);
		if (!vmConfig.setPermissions((QFile::Permissions)m_nConfigPermissions)) {
			WRITE_TRACE(DBG_FATAL,
				"[%s] Cannot set permissions for Vm config \"%s\", will use default",
				__FUNCTION__, QSTR2UTF8(m_sVmConfigPath));
		}
	}
	return PRL_ERR_SUCCESS;
}

void Task_MigrateVmTarget::DeleteSnapshot()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	nRetCode = m_pVm->replaceInitDspCmd(PVE::DspCmdVmDeleteSnapshot, getClient());
	if ( PRL_FAILED(nRetCode) ) {
		WRITE_TRACE(DBG_FATAL, "[%s] CDspVm::replaceInitDspCmd() failed. Reason: %#x (%s)",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return;
	}

	CProtoCommandPtr pRequest =
		CProtoSerializer::CreateDeleteSnapshotProtoCommand(m_sVmUuid, m_sSnapshotUuid, false);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmDeleteSnapshot, pRequest);
	CVmEvent evt;
	if (!m_pVm->deleteSnapshot(getClient(), pPackage, &evt, true)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Unknown error occurred while snapshot deleting", __FUNCTION__);
	} else if (PRL_FAILED(evt.getEventCode())) {
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while snapshot deleting with code [%#x][%s]",
			__FUNCTION__, evt.getEventCode(), PRL_RESULT_TO_STRING(evt.getEventCode()));
	}
}

PRL_RESULT Task_MigrateVmTarget::adjustStartVmCommand(SmartPtr<IOPackage> &pPackage)
{
	CVmMigrateStartCommand *pStartCmd;
	CVmEventParameter *pEventParam;

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(m_pStartPackage);
	if ( !pCmd->IsValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			m_pStartPackage->buffers[0].getImpl());
		return PRL_ERR_FAILURE;
	}

	pStartCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);
	if ( NULL == pStartCmd )
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
	pVmConfig->getVmIdentification()->setHomePath(m_sVmConfigPath);
	pVmConfig->getVmHardwareList()->RevertDevicesPathToAbsolute(m_sTargetVmHomePath);

	pEventParam = pStartCmd->GetCommand()->getEventParameter(EVT_PARAM_MIGRATE_CMD_VM_CONFIG);
	if (pEventParam)
		pEventParam->setParamValue(pVmConfig->toString());

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
PRL_RESULT Task_MigrateVmTarget::registerHaClusterResource()
{
	PRL_RESULT nRetCode;

	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_SUCCESS;

	CVmHighAvailability* pHighAvailability = m_pVmConfig->getVmSettings()->getHighAvailability();

	if (!pHighAvailability->isEnabled())
		return PRL_ERR_SUCCESS;

	if (m_nReservedFlags & PVM_HA_MOVE_VM) {
		// move resource from source node
		nRetCode = CDspService::instance()->getHaClusterHelper()->moveFromClusterResource(
					m_sVmName, m_sHaClusterId);
	} else {
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
void Task_MigrateVmTarget::unregisterHaClusterResource()
{
	if (!m_pVmConfig->getVmSettings()->getHighAvailability()->isEnabled())
		return;

	if (!(m_nSteps & MIGRATE_HA_RESOURCE_REGISTERED))
		return;

	if (m_nReservedFlags & PVM_HA_MOVE_VM) {
		// resource was moved - return to source node
		CDspService::instance()->getHaClusterHelper()->moveToClusterResource(m_sVmName, m_sHaClusterId);
	} else {
		// resource was added
		CDspService::instance()->getHaClusterHelper()->removeClusterResource(m_sVmName, false);
	}
}
