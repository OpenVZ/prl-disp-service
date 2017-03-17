///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCtTarget.cpp
///
/// Target task for Vm migration
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

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "CDspVmBrand.h"
#include "Interfaces/Debug.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <prlcommon/Logging/Logging.h>
#include "Libraries/StatesStore/SavedStateTree.h"
#include <prlcommon/Std/PrlTime.h>

#include "CDspVmDirHelper.h"
#include "Task_MigrateCtTarget.h"
#include "Task_CloneVm.h"
#include "CDspService.h"
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "CDspVzHelper.h"
#include "Tasks/Task_VzManager.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

#define VM_MIGRATE_START_CMD_WAIT_TIMEOUT 600 * 1000

#ifndef _LIN_
Task_MigrateCtTarget::Task_MigrateCtTarget(
		const QObject *parent,
		const SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p)
:CDspTaskHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection)
{
	Q_UNUSED(pCmd);
	Q_UNUSED(parent);
}

Task_MigrateCtTarget::~Task_MigrateCtTarget()
{
}

PRL_RESULT Task_MigrateCtTarget::run_body()
{
	setLastErrorCode(PRL_ERR_UNIMPLEMENTED);
	m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	return getLastErrorCode();
}

#else

Task_MigrateCtTarget::Task_MigrateCtTarget(
		const SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p)
:Task_VzMigrate(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_nFlags(0)
{
	CVmMigrateCheckPreconditionsCommand * pCheckCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);

	m_hConnHandle = pDispConnection->GetConnectionHandle();

	m_sSharedFileName = pCheckCmd->GetSharedFileName();
	m_nMigrationFlags = pCheckCmd->GetMigrationFlags();
	m_nReservedFlags = pCheckCmd->GetReservedFlags();
	m_nVersion = pCheckCmd->GetVersion();
	m_nPrevVmState = pCheckCmd->GetVmPrevState();
	m_sVmConfig = pCheckCmd->GetVmConfig();
	m_sSrcHostInfo = pCheckCmd->GetSourceHostHardwareInfo();

	// Check name, treat it as CTID if it contain valid CTID,
	// treat it as containers name otherwise.
	m_sCtNewId = CVzHelper::parse_ctid(pCheckCmd->GetTargetVmName());
	if (m_sCtNewId.isEmpty())
		m_sCtNewName = pCheckCmd->GetTargetVmName();

	m_sCtNewPrivate = pCheckCmd->GetTargetVmHomePath();
	/* initialize all vars from pCheckCmd - after exit from constructor package buffer will invalid */

	m_bExclusiveVmParametersLocked = false;

	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_MigrateCtTarget::~Task_MigrateCtTarget()
{
	m_waiter.waitUnlockAndFinalize();
}

/* process VmMigrateCheckPreconditionsCommand */
PRL_RESULT Task_MigrateCtTarget::prepareTask()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CVzHelper & VzHelper = CDspService::instance()->getVzHelper()->getVzlibHelper();
	VIRTUAL_MACHINE_STATE nState;
	QString ctid;
	bool bCtExists = false;

	{
		CDspLockedPointer<CVmDirectory> pDir = CDspService::instance()->getVmDirManager().getVzDirectory();
		if (!pDir) {
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
			goto exit;
		}
		m_sVzDirUuid = pDir->getUuid();
	}

	m_cVmConfig.fromString(m_sVmConfig);
	if (PRL_FAILED(m_cVmConfig.m_uiRcInit))
	{
		nRetCode = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Wrong VM condiguration was received: [%s]", QSTR2UTF8(m_sVmConfig));
		goto exit;
	}
	m_sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()->
			getVmServerIdentification()->getServerUuid();
	if (m_nVersion < MIGRATE_DISP_PROTO_V7) {
		m_cVmConfig.getVmIdentification()->setCtId(
			QString::number(m_cVmConfig.getVmIdentification()->getEnvId()));
	}
	m_sCtOrigId = m_cVmConfig.getVmIdentification()->getCtId();
	m_sCtOrigUuid = m_cVmConfig.getVmIdentification()->getVmUuid();

	if (m_nMigrationFlags & PVMT_CLONE_MODE) {
		// need to generate new uuid, ctid and path if cloned
		m_sCtUuid = Uuid::createUuid().toString();
		if (Uuid::isUuid(m_sCtOrigId) && m_sCtNewId.isEmpty())
			m_sCtNewId = m_sCtUuid;
	} else
		m_sCtUuid = m_sCtOrigUuid;
	m_sSrcCtUuid = m_cVmConfig.getVmIdentification()->getVmUuid();

	if (m_sCtNewName.isEmpty()) {
		// Check name from config, treat it as CTID if it contain valid CTID,
		// treat it as containers name otherwise.
		QString vmName = m_cVmConfig.getVmIdentification()->getVmName();
		if (CVzHelper::parse_ctid(vmName).isEmpty())
			m_sCtNewName = vmName;
	}

	if (!m_sCtNewId.isEmpty()) {
		ctid = m_sCtNewId;
		m_cVmConfig.getVmIdentification()->setCtId(m_sCtNewId);
	} else
		ctid = m_sCtOrigId;

	nRetCode = VzHelper.get_env_status_by_ctid(ctid, nState);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Failed to retrieve information about CT #%s status."
			" Reason: %#x (%s)", QSTR2UTF8(ctid), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	// check if CT with m_sCtOrigId exists locally.
	bCtExists = (nState != VMS_UNKNOWN);

	do {
		if (bCtExists || m_sCtNewPrivate.isEmpty()) {
			break;
		}
		bCtExists = CFileHelper::DirectoryExists(m_sCtNewPrivate, &getClient()->getAuthHelper());
		if (!bCtExists) {
			break;
		}
#ifdef _LIN_
		int z = 0;
		QFileInfo x(QDir(m_sCtNewPrivate), VZ_CT_CONFIG_FILE);
		SmartPtr<CVmConfiguration> y = CVzHelper::get_env_config_from_file(x.absoluteFilePath(), z);
		if (y.isValid()) {
			bCtExists = m_sSrcCtUuid != y->getVmIdentification()->getVmUuid();
		}
#endif // _LIN_
	} while(false);

	if (bCtExists) {
		QString sPrivate;
		if (!m_sCtNewPrivate.isEmpty()) {
			// the format is always vz_root/m_sCtOrigId. see Task_MigrateCtSource.cpp.
			sPrivate = QFileInfo(m_sCtNewPrivate).path().append("/$VEID");
		}

		/* CT with such ID already exists */
		if (!m_sCtNewId.isEmpty() || m_sCtNewName.isEmpty()) {
			/* failure */
			nRetCode = PRL_ERR_CT_MIGRATE_ID_ALREADY_EXIST;
			getLastError()->addEventParameter(new CVmEventParameter(
				PVE::String, ctid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "[%s] Container with ID %s already exist",
				__FUNCTION__, QSTR2UTF8(ctid));
			goto exit;

		} else {
			/* CT name defined - use containers UUID as CTID if it is not
			 * already exist as well. */
			QString normUuid = CVzHelper::build_ctid_from_uuid(m_sCtUuid);
			if (ctid == normUuid) {
				nRetCode = PRL_ERR_CT_MIGRATE_ID_ALREADY_EXIST;
				goto exit;
			}
			m_sCtNewId = normUuid;
			m_cVmConfig.getVmIdentification()->setCtId(m_sCtNewId);
		}

		if (!m_sCtNewPrivate.isEmpty() && !m_sCtNewId.isEmpty()) {
			m_sCtNewPrivate = QString("%1/%2").arg(QFileInfo(m_sCtNewPrivate).path())
					.arg(m_sCtNewId);
		}
	}

	/* register CT on dispatcher : we can not get CT target private, will use empty */
	m_pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
			m_sCtUuid, QString(), m_sCtNewName));

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
	m_bExclusiveVmParametersLocked = true;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MigrateCtTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	bool bConnected;
	QTimer *pTimer;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;
	QStringList lstArgs;
	QString sVmUuid;
	SmartPtr<CVmConfiguration> pConfig;
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdCtlDispatherFakeCommand);

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	/* set signal handler before reply - to avoid race */
	bConnected = this->connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(
			IOSender::Handle,
			const SmartPtr<IOPackage>)),
		SLOT(handlePackage(
			IOSender::Handle,
			const SmartPtr<IOPackage>&)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	m_nFlags |= PVM_CT_MIGRATE;
	/* and send reply on Precond Check command */
	pReply = CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsReply(
			m_lstCheckPrecondsErrors, QStringList(), m_nFlags);
	pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(), pReply->GetCommand()->toString(), getRequestPackage());

	/* set timer */
	pTimer = new QTimer();
	pTimer->setSingleShot(true);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(handleStartCommandTimeout()));
	pTimer->start(VM_MIGRATE_START_CMD_WAIT_TIMEOUT);

	/* send reply and will wait StartMigration command */
	m_pDispConnection->sendPackage(pPackage);
	nRetCode = exec();
	pTimer->stop();
	delete pTimer;

	m_pDispConnection->disconnect(
		SIGNAL(onPackageReceived(
			IOSender::Handle,
			const SmartPtr<IOPackage>)), this,
		SLOT(handlePackage(
			IOSender::Handle,
			const SmartPtr<IOPackage>&)));

	if (PRL_FAILED(nRetCode))
		goto exit;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if (m_nPrevVmState == VMS_RUNNING)
		lstArgs.append("--online");
	/* to force nfs -> non-nfs migration */
	lstArgs.append("--nonsharedfs");

	lstArgs.append("localhost");
	lstArgs.append(m_sCtOrigId);
	if (!m_sCtNewId.isEmpty())
		lstArgs.append(QString("--new-id=%1").arg(m_sCtNewId));
	if (m_sCtNewPrivate.size())
		lstArgs.append(QString("--new-private=%1").arg(m_sCtNewPrivate));

	nRetCode = startVzMigrate(PRL_CT_MIGRATE_SERVER, lstArgs);
	if (PRL_FAILED(nRetCode))
		goto exit;

	/* send reply */
	pReply = CDispToDispProtoSerializer::CreateVmMigrateReply(QString());
	pPackage = DispatcherPackage::createInstance(
		pReply->GetCommandId(), pReply->GetCommand()->toString(), m_pStartPackage);
	hJob = m_pDispConnection->sendPackage(pPackage);

	nRetCode = execVzMigrate(m_pStartPackage, &CDspService::instance()->getIOServer(), hJob);
	if (PRL_FAILED(nRetCode))
		goto exit;

	pConfig = CDspService::instance()->getVzHelper()->getVzlibHelper().
			get_env_config_by_ctid(m_cVmConfig.getVmIdentification()->getCtId());
	if (!pConfig) {
		WRITE_TRACE(DBG_FATAL, "Can't get config for CT %s [%s]",
			QSTR2UTF8(m_sCtUuid),
			QSTR2UTF8(m_cVmConfig.getVmIdentification()->getCtId()));
		nRetCode = PRL_ERR_VM_GET_CONFIG_FAILED;
		goto exit;
	}

	if ( m_nMigrationFlags & PVMT_CLONE_MODE )
	{
		SmartPtr<CVmConfiguration> pNewConfig;
		::Vm::Private::Brand b(pConfig->getVmIdentification()->getHomePath(), getClient());
		b.remove();
		if (PRL_FAILED(nRetCode = b.stamp()))
			goto exit;

		pNewConfig = SmartPtr<CVmConfiguration>( new CVmConfiguration );
		pNewConfig->fromString(pConfig->toString());
		if (PRL_FAILED(nRetCode = pNewConfig->m_uiRcInit)) {
			WRITE_TRACE(DBG_FATAL, "CT config copy error");
			goto exit;
		}
		/* change UUID on ve.conf */
		pNewConfig->getVmIdentification()->setVmUuid(m_sCtUuid);
		pNewConfig->getVmIdentification()->setVmName(m_sCtNewName);

		if ( m_nMigrationFlags & PVMT_SWITCH_TEMPLATE )
			pNewConfig->getVmSettings()->getVmCommonOptions()->setTemplate(
				!pNewConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );

		Task_CloneVm::ResetNetSettings(pNewConfig);
		/* call apply_env_config() before pConfig->getVmIdentification()->setVmUuid(m_sCtUuid), otherwise
		   pNewConfig == pConfig and apply_env_config() will not update config file */
		CDspService::instance()->getVzHelper()->getVzlibHelper().
			update_ctid_map(m_sCtUuid, m_cVmConfig.getVmIdentification()->getCtId());
		get_op_helper().update_env_uuid(pNewConfig, pConfig);
		get_op_helper().apply_env_config(pNewConfig, pConfig, 0);
	}

	// insert new item in user's VM Directory
	pConfig->getVmIdentification()->setVmUuid(m_sCtUuid);
	if (PRL_FAILED(CDspService::instance()->getVzHelper()->insertVmDirectoryItem(pConfig))) {
		WRITE_TRACE(DBG_FATAL, "Can't insert vm to VmDirectory by error %#x, %s",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
	}

	/*
	   Send event to vzevent handler. It's workaround of https://jira.sw.ru/browse/PSBM-9446
	   Kernel should send ve-start event with ... | ENV_STATUS_SUSPENDED mask
	   (see https://jira.sw.ru/browse/PSBM-9469)
	*/
	CDspService::instance()->getTaskManager().schedule(new Task_VzManager
		(m_pDispConnection->getUserSession(), p, m_sCtUuid, VMS_RESUMING));
	/* don't wait this task - we will ignore result in any case */

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MigrateCtTarget::finalizeTask()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle)));
	IOSendJob::Handle hJob;

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		// Notify clients that new CT appeared.
		CVmEvent event(PET_DSP_EVT_VM_ADDED, m_sCtUuid, PIE_DISPATCHER);
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event);
		CDspService::instance()->getClientManager().sendPackageToAllClients(p);
		hJob = m_pDispConnection->sendSimpleResponse(getRequestPackage(), getLastErrorCode());
	} else {
		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		hJob = m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	}
	CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);

	// delete temporary registration
	if (m_bExclusiveVmParametersLocked)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());
}

void Task_MigrateCtTarget::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_hConnHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	cancelOperation(nullClient, getRequestPackage());
	// quit event loop
	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

void Task_MigrateCtTarget::handlePackage(
		IOSender::Handle handle_,
		const SmartPtr<IOPackage>& package_)
{
	if (handle_ != m_pDispConnection->GetConnectionHandle())
		return;
	if (VmMigrateStartCmd != package_->header.type)
		return;

	CDispToDispCommandPtr d = CDispToDispProtoSerializer::ParseCommand(package_);
	if (!d->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Invalid start migration package is received: [%s]",\
			package_->buffers[0].getImpl());
		return QThread::exit(PRL_ERR_FAILURE);
	}
	CVmMigrateStartCommand* s =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(d);
	if (!s->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Invalid start migration package is received: [%s]",\
			package_->buffers[0].getImpl());
		return QThread::exit(PRL_ERR_FAILURE);
	}
	CVmConfiguration c(s->GetVmConfig());
	if (PRL_FAILED(c.m_uiRcInit))
	{
		m_pDispConnection->sendSimpleResponse(package_, PRL_ERR_PARSE_VM_CONFIG);
		WRITE_TRACE(DBG_FATAL, "Invalid VM configuration was received: [%s]",\
			QSTR2UTF8(s->GetVmConfig()));
		return QThread::exit(PRL_ERR_FAILURE);
	}

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock(m_waiter);
	if(!lock.isLocked())
		return;

	if (operationIsCancelled())
		return;

	if (m_sSrcCtUuid != c.getVmIdentification()->getVmUuid())
		return;

	m_nMigrationFlags = s->GetMigrationFlags();
	m_nReservedFlags = s->GetReservedFlags();
	m_pStartPackage	= IOPackage::duplicateInstance(package_);
	QThread::exit(PRL_ERR_SUCCESS);
}

// cancel command
void Task_MigrateCtTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	terminateVzMigrate();
	terminateHandleDispPackageTask();

	CancelOperationSupport::cancelOperation(pUser, p);
	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

void Task_MigrateCtTarget::handleStartCommandTimeout()
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	WRITE_TRACE( DBG_DEBUG, "StartMigrateCommand timeout" );
	QThread::exit(PRL_ERR_TIMEOUT);
}

/* send package with migration data to target dispatcher */
PRL_RESULT Task_MigrateCtTarget::sendDispPackage(SmartPtr<IOPackage> &pPackage)
{
	IOSendJob::Handle hJob;

	hJob = m_pDispConnection->sendPackage(pPackage);
	if (CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}
#endif

