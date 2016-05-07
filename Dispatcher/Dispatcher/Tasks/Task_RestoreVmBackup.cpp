///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackup.cpp
///
/// Source task for Vm backup restoring
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

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <QProcess>

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include "Task_RestoreVmBackup_p.h"
#include "prlcommon/Logging/Logging.h"
#include "prlcommon/PrlUuid/Uuid.h"

#include "Task_RestoreVmBackup.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "CDspBackupDevice.h"
#ifdef _CT_
#include "vzctl/libvzctl.h"
#endif

/*******************************************************************************

 Backup restore task for server

********************************************************************************/
Task_RestoreVmBackupSource::Task_RestoreVmBackupSource(
		SmartPtr<CDspDispConnection> &pDispConnection,
		const CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_ioServer(CDspService::instance()->getIOServer()),
m_bBackupLocked(false)
{
	CVmBackupRestoreCommand *pCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupRestoreCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_nBackupNumber = PRL_BASE_BACKUP_NUMBER;
	m_nFlags = pCmd->GetFlags();
	m_nTotalSize = 0;
	m_hHandle = m_pDispConnection->GetConnectionHandle();
	m_nInternalFlags = 0;
	m_nRemoteVersion = pCmd->GetVersion();
	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_RestoreVmBackupSource::~Task_RestoreVmBackupSource()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle))
		);

	// #439777 to protect call handler for destroying object
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_RestoreVmBackupSource::prepareTask()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	VmItem cVmItem;

	if (m_sBackupId.isEmpty()) {
		if (m_sVmUuid.isEmpty()) {
			WRITE_TRACE(DBG_FATAL, "[%s] Vm uuid and backup id are empty", __FUNCTION__);
			nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
			goto exit;
		}
		/* restore from last incremental of last backup if backup id does not specified (#420596) */
		BackupItem* b = getLastBaseBackup(m_sVmUuid, &getClient()->getAuthHelper(), PRL_BACKUP_CHECK_MODE_READ);
		if (NULL == b) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sVmUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm %s", QSTR2UTF8(m_sVmUuid));
			goto exit;
		}
		m_sBackupUuid = b->getUuid();
		delete b;
		QList<unsigned> lstBackupNumber;
		getPartialBackupList(m_sVmUuid, m_sBackupUuid, lstBackupNumber);
		if (lstBackupNumber.size())
			/* it will restore from last partial backup */
			m_nBackupNumber = lstBackupNumber.last();
		else
			m_nBackupNumber = PRL_BASE_BACKUP_NUMBER;
	} else {
		if (PRL_FAILED(parseBackupId(m_sBackupId, m_sBackupUuid, m_nBackupNumber))) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Invalid backup id \"%s\"", QSTR2UTF8(m_sBackupId));
			goto exit;
		}
	}

	if (m_sVmUuid.isEmpty()) {
		if (PRL_FAILED(findVmUuidForBackupUuid(m_sBackupUuid, m_sVmUuid))) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Backup \"%s\" does not exist", QSTR2UTF8(m_sBackupUuid));
			goto exit;
		}
	}

	m_sBackupRootPath = QString("%1/%2/%3").arg(getBackupDirectory()).arg(m_sVmUuid).arg(m_sBackupUuid);
	if (m_nBackupNumber == PRL_BASE_BACKUP_NUMBER)
		m_sBackupPath = QString("%1/" PRL_BASE_BACKUP_DIRECTORY).arg(m_sBackupRootPath);
	else
		m_sBackupPath = QString("%1/%2").arg(m_sBackupRootPath).arg(m_nBackupNumber);

	/* to check access before */
	if (!CFileHelper::FileCanRead(m_sBackupRootPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "User %s have not permissions for restore backup %s",
			QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sBackupUuid));
		goto exit;
	}

	/* to get vmtype */
	if (PRL_FAILED(nRetCode = loadVmMetadata(m_sVmUuid, &cVmItem)))
		goto exit;
	m_sVmName = cVmItem.getName();
	if (cVmItem.getVmType() == PVBT_CT_VZFS)
		m_nInternalFlags |= PVM_CT_VZFS_BACKUP;
	else if (cVmItem.getVmType() == PVBT_CT_PLOOP)
		m_nInternalFlags |= PVM_CT_PLOOP_BACKUP;
	else if (cVmItem.getVmType() == PVBT_CT_VZWIN)
		m_nInternalFlags |= PVM_CT_VZWIN_BACKUP;

	/* to lock backup */
	if (PRL_FAILED(nRetCode = lockShared(m_sBackupUuid)))
		goto exit;
	m_bBackupLocked = true;
#ifdef _LIN_
	/* check that the backup is not attached to some VM - otherwise we'll get
	 * acronis 'Access Denied' error */
	if (Backup::Device::Service::isAttached(m_sBackupUuid)) {
		WRITE_TRACE(DBG_FATAL, "backup '%s' is attached to VM, restore is not possible",
			QSTR2UTF8(m_sBackupUuid));
		nRetCode = PRL_ERR_BACKUP_RESTORE_PROHIBIT_WHEN_ATTACHED;
		getLastError()->setEventCode(nRetCode);
		getLastError()->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
	}
#endif // _LIN_
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

static bool excludeFunc(const QString &sRelPath)
{
	if (sRelPath == PRL_BACKUP_METADATA)
		return true; /* skip metadata */
	return false;
}

PRL_RESULT Task_RestoreVmBackupSource::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (m_nInternalFlags & PVM_CT_BACKUP)
		nRetCode = restoreCt();
	else
		nRetCode = restoreVm();

	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_RestoreVmBackupSource::finalizeTask()
{
	m_cABackupServer.kill();

	if (m_bBackupLocked)
		unlockShared(m_sBackupUuid);
	m_bBackupLocked = false;

	if (PRL_FAILED(getLastErrorCode()))
		m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
}

PRL_RESULT Task_RestoreVmBackupSource::sendFiles(IOSendJob::Handle& job_)
{
	/* wait client's reply for syncronization */
	if (m_ioServer.waitForResponse(job_, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "[%s] Package reading failure", __FUNCTION__);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
/* TODO : process response retcode */
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode = getEntryLists(m_sBackupPath, excludeFunc);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(m_ioServer, m_hHandle));
	m_pVmCopySource = SmartPtr<CVmFileListCopySource>(
			new CVmFileListCopySource(
				m_pSender.getImpl(),
				m_sVmUuid,
				m_sBackupPath,
				m_nTotalSize,
				getLastError(),
				m_nTimeout));

	nRetCode = m_pVmCopySource->Copy(m_DirList, m_FileList);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupSource::sendStartReply(const SmartPtr<CVmConfiguration>& ve_, IOSendJob::Handle& job_)
{
	qulonglong nOriginalSize = 0;
	quint32 nBundlePermissions = 0;
	PRL_RESULT code = Task_BackupHelper::getBackupParams(
		m_sVmUuid, m_sBackupUuid, m_nBackupNumber, nOriginalSize, nBundlePermissions);
	if (PRL_FAILED(code))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get backup OriginalSize %x", code);
		nOriginalSize = 0;
	}

	bool ok;
	// e.g. 6.10.24168.1187340 or 7.0.200
	int major = ve_->getAppVersion().split(".")[0].toInt(&ok);
	if (!ok)
	{
		WRITE_TRACE(DBG_FATAL, "Invalid AppVersion: %s",
					qPrintable(ve_->getAppVersion()));
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}

	quint32 protoVersion = major < 7 ? BACKUP_PROTO_V3 : BACKUP_PROTO_VERSION;
	CDispToDispCommandPtr pReply = CDispToDispProtoSerializer::CreateVmBackupRestoreFirstReply(
			m_sVmUuid,
			m_sVmName,
			ve_->toString(),
			m_sBackupUuid,
			m_nBackupNumber,
			m_sBackupRootPath,
			nOriginalSize,
			nBundlePermissions,
			m_nInternalFlags,
			protoVersion);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(),
			pReply->GetCommand()->toString(),
			getRequestPackage());

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	// XXX: this job is required because the target generates the ready
	// to copy files packet using the start reply package as a source.
	// that new packet is to sync transfering sides.
	job_ = m_ioServer.sendPackage(m_hHandle, pPackage);
	if (m_ioServer.waitForSend(job_, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "[%s] Package sending failure", __FUNCTION__);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupSource::restoreVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	PRL_RESULT code;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle job;
	QString sVmConfigPath;
	SmartPtr<CVmConfiguration> pVmConfig;
	QStringList args;
	bool bConnected;

	if (!CFileHelper::DirectoryExists(m_sBackupPath, &m_pDispConnection->getUserSession()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Backup directory \"%s\" does not exist", QSTR2UTF8(m_sBackupPath));
		goto exit;
	}
	/* load Vm config and send to client (as mininal, for getVmName()) */
	sVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sBackupPath);
	pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	code = CDspService::instance()->getVmConfigManager().loadConfig(
				pVmConfig, sVmConfigPath, getClient(), false, true);
	if (PRL_FAILED(code)) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while Vm config \"%s\" loading with code [%#x][%s]",
			__FUNCTION__, QSTR2UTF8(sVmConfigPath), code, PRL_RESULT_TO_STRING(code));
		goto exit;
	}
	bConnected = QObject::connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	nRetCode = sendStartReply(pVmConfig, job);
	if (PRL_FAILED(nRetCode))
		goto exit;

	args << QString(PRL_ABACKUP_SERVER);
	if (PRL_FAILED(nRetCode = m_cABackupServer.start(args, QStringList(), m_nRemoteVersion)))
		goto exit;

	if (PRL_FAILED(nRetCode = sendFiles(job)))
	{
		m_cABackupServer.kill();
		m_cABackupServer.waitForFinished();
	}
	else
	{
		// part two : backup of hdd's via acronis library.
		nRetCode = m_cABackupServer.waitForFinished();
	}

exit:
	QObject::disconnect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)));
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupSource::restoreCt()
{
#ifdef _CT_
	int y = 0;
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle job;
	QStringList args;
	QString sVmConfigPath;
	SmartPtr<CVmConfiguration> pConfig;
	bool bConnected;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit_0;
	}
	if (m_nInternalFlags & PVM_CT_PLOOP_BACKUP && m_nRemoteVersion < BACKUP_PROTO_V3)
	{
		WRITE_TRACE(DBG_FATAL, "Unsupported protocol version %d. >= %d is expected",
			m_nRemoteVersion, BACKUP_PROTO_V3);
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
		goto exit_0;
	}
	if (!CFileHelper::DirectoryExists(m_sBackupPath, &m_pDispConnection->getUserSession()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Backup directory \"%s\" does not exist", QSTR2UTF8(m_sBackupPath));
		goto exit_0;
	}
	sVmConfigPath = QString("%1/" VZ_CT_CONFIG_FILE).arg(m_sBackupPath);
	pConfig = CVzHelper::get_env_config_from_file(sVmConfigPath, y, 
			(0 != (m_nInternalFlags & PVM_CT_PLOOP_BACKUP)) * VZCTL_LAYOUT_5,
			true);
	if (!pConfig) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while Ct config \"%s\" loading",
			__FUNCTION__, QSTR2UTF8(sVmConfigPath));
		goto exit_0;
	}
	bConnected = QObject::connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	nRetCode = sendStartReply(pConfig, job);
	if (PRL_FAILED(nRetCode))
		goto exit_1;

	args << QString(PRL_ABACKUP_SERVER);
	if (PRL_FAILED(nRetCode = m_cABackupServer.start(args, QStringList(), m_nRemoteVersion)))
		goto exit_1;

	if (m_nInternalFlags & (PVM_CT_PLOOP_BACKUP|PVM_CT_VZWIN_BACKUP))
	{
		if (PRL_FAILED(nRetCode = sendFiles(job)))
		{
			m_cABackupServer.kill();
			m_cABackupServer.waitForFinished();
			goto exit_1;
		}
	}
	nRetCode = m_cABackupServer.waitForFinished();
exit_1:
	QObject::disconnect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)));
exit_0:
	setLastErrorCode(nRetCode);
	return nRetCode;
#else
	WRITE_TRACE(DBG_FATAL, "Linux containers does not implemented");
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

void Task_RestoreVmBackupSource::handleABackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (PRL_FAILED(Task_BackupHelper::handleABackupPackage(m_pDispConnection, p, m_nBackupTimeout)))
		m_cABackupServer.kill();
}

void Task_RestoreVmBackupSource::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_hHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	CancelOperationSupport::cancelOperation(nullClient, getRequestPackage());
	if (m_pVmCopySource.getImpl())
		m_pVmCopySource->cancelOperation();
	m_cABackupServer.kill();
}
