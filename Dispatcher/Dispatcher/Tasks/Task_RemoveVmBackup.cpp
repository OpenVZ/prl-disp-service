///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RemoveVmBackup.cpp
///
/// Source and target tasks for Vm backup restoring
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

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"
#include "Libraries/StatesStore/SavedStateTree.h"

#include "Task_RemoveVmBackup.h"
#include "Tasks/Task_RegisterVm.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/BackupTree.h"

/*******************************************************************************

 Backup removing task for client

********************************************************************************/
Task_RemoveVmBackupSource::Task_RemoveVmBackupSource(
		SmartPtr<CDspClient> &client,
		CProtoCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
:Task_BackupHelper(client, p)
{
	CProtoRemoveVmBackupCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoRemoveVmBackupCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_sServerHostname = pCmd->GetServerHostname();
	m_nServerPort = pCmd->GetServerPort();
	m_sServerSessionUuid = pCmd->GetServerSessionUuid();
	m_nFlags = pCmd->GetFlags() | PBT_VM | PBT_CT; // by default for Vm and Ct
}

PRL_RESULT Task_RemoveVmBackupSource::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pCommand;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<IOPackage> pReply;
	CDispToDispCommandPtr pResponce;
	CDispToDispResponseCommand *pResponseCmd;

	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	if (operationIsCancelled())
	{
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if (m_sVmUuid.isEmpty())
	{
		QString sBackupTree;
		if (PRL_FAILED(nRetCode = GetBackupTreeRequest(m_sVmUuid, sBackupTree)))
		{
			nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
			WRITE_TRACE( DBG_FATAL, "Failed to get backup tree");
			goto exit;
		}

		QString sBackupUuid;
		unsigned nBackupNumber;

		if (PRL_FAILED(parseBackupId(m_sBackupId, sBackupUuid, nBackupNumber))) {
				nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
				CVmEvent *pEvent = getLastError();
				pEvent->setEventCode(nRetCode);
				pEvent->addEventParameter(new CVmEventParameter(
						PVE::String, m_sBackupId, EVT_PARAM_MESSAGE_PARAM_0));
				WRITE_TRACE(DBG_FATAL, "Failed to parse backupId \"%s\".",
					    QSTR2UTF8(m_sBackupId));
				goto exit;
		}

		BackupTree bTree;

		bTree.fromString(sBackupTree);
		foreach(VmItem *vm , bTree.m_lstVmItem)
		{
			foreach( BackupItem *backup, vm->m_lstBackupItem )
			{
				if ( backup->getId() != sBackupUuid )
					continue;

				m_sVmUuid = vm->getUuid();
				break;
			}

			if (!m_sVmUuid.isEmpty())
				break;
		}

		if (m_sVmUuid.isEmpty())
		{
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupId, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Failed to find vm for backupId \"%s\".",
				QSTR2UTF8(sBackupUuid));
			goto exit;
		}
	}

	pCommand = CDispToDispProtoSerializer::CreateVmBackupRemoveCommand(m_sVmUuid, m_sBackupId, m_nFlags);
	pPackage = DispatcherPackage::createInstance(pCommand->GetCommandId(), pCommand->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply)))
		goto exit;

	if (pReply->header.type != DispToDispResponseCmd) {
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x",
			pReply->header.type, DispToDispResponseCmd);
		nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
		goto exit;
	}

	pResponce = CDispToDispProtoSerializer::ParseCommand(
		DispToDispResponseCmd, UTF8_2QSTR(pReply->buffers[0].getImpl()));
	pResponseCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pResponce);
	if (PRL_FAILED(nRetCode = pResponseCmd->GetRetCode()))
		getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* Finalize task */
void Task_RemoveVmBackupSource::finalizeTask()
{
	if (isConnected()) {
		CDispToDispCommandPtr pCmd =
			CDispToDispProtoSerializer::CreateDispToDispCommandWithoutParams(DispToDispLogoffCmd);
		SmartPtr<IOPackage> pPackage =
			DispatcherPackage::createInstance(pCmd->GetCommandId(), pCmd->GetCommand()->toString());
		SendPkg(pPackage);
	}

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		QString sVmDirUuid = getClient()->getVmDirectoryUuid();
		bool isVmExists;

		SmartPtr<IOPackage> pPackage;
		CVmEvent event(PET_DSP_EVT_REMOVE_BACKUP_FINISHED, m_sVmUuid, PIE_DISPATCHER);
		event.addEventParameter( new CVmEventParameter( PVE::String,
														m_sBackupId,
														EVT_PARAM_BACKUP_CMD_BACKUP_UUID ) );
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		{
			CDspLockedPointer<CVmDirectoryItem> pLockedVmDirItem =
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid(sVmDirUuid, m_sVmUuid);
			isVmExists = ( pLockedVmDirItem );
		}
		if ( isVmExists )
		{
			CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirUuid, m_sVmUuid);
		}
		else
		{
			// VM is not exists on node - send event to task client only
			getClient()->sendPackage(pPackage);
		}

		getClient()->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	}
	else
		getClient()->sendResponseError(getLastError(), getRequestPackage());
}

/*******************************************************************************

 Backup removing task for server

********************************************************************************/
Task_RemoveVmBackupTarget::Task_RemoveVmBackupTarget(
		SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection)
{
	CVmBackupRemoveCommand *pCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupRemoveCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_nFlags = pCmd->GetFlags();
	m_nBackupNumber = 0;
	m_nLocked = 0;
}

PRL_RESULT Task_RemoveVmBackupTarget::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QStringList lstBackupUuid;
	QString sVmPath;

	if (m_sBackupId.isEmpty()) {
		if (m_sVmUuid.isEmpty()) {
			nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
			WRITE_TRACE(DBG_FATAL, "[%s] Invalid parameters : empty backup Uuid and Vm Uuid", __FUNCTION__);
			goto exit;
		}
		sVmPath = QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid);
		/* to remove all backups of this client for this Vm */
		if (PRL_FAILED(nRetCode = removeAllBackupsForVm(sVmPath)))
			goto exit;
	} else {
		if (PRL_FAILED(parseBackupId(m_sBackupId, m_sBackupUuid, m_nBackupNumber))) {
				nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
				CVmEvent *pEvent = getLastError();
				pEvent->setEventCode(nRetCode);
				pEvent->addEventParameter(new CVmEventParameter(
						PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
				WRITE_TRACE(DBG_FATAL, "Backup \"%s\" does not exist", QSTR2UTF8(m_sBackupId));
				goto exit;
		}
		/*don't need it. we search vmUuid by backupId on Source.
		  save it for compatibility with older servers. */
		if (m_sVmUuid.isEmpty()) {
			/* will find for all users */
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

		sVmPath = QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid);
		QString sBackupPath = QString("%1/%2").arg(sVmPath).arg(m_sBackupUuid);
		if (m_nBackupNumber == PRL_BASE_BACKUP_NUMBER)
			m_cTargetDir.setFile(sBackupPath);
		else
			m_cTargetDir.setFile(QString("%1/%2").arg(sBackupPath).arg(m_nBackupNumber));

		if (!m_cTargetDir.exists()) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Backup \"%s\" does not exist", QSTR2UTF8(m_sBackupId));
			goto exit;
		}

		/* to check access before */
		if (!CFileHelper::FileCanWrite(sBackupPath, &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_REMOVE_PERMISSIONS_DENIED;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "User %s does not have permissions to remove the backup %s in %s",
					QSTR2UTF8(getClient()->getAuthHelper().getUserName()),
					QSTR2UTF8(m_sBackupUuid),
					QSTR2UTF8(sBackupPath));
			goto exit;
		}
		if (m_nBackupNumber == PRL_BASE_BACKUP_NUMBER) {
			if (PRL_FAILED(nRetCode = removeBaseBackup()))
				goto exit;
		} else {
			if (PRL_FAILED(nRetCode = removeIncrementalBackup(sBackupPath)))
				goto exit;
		}
		/* remove BackupUuid directory if it is empty */
		if (isBackupDirEmpty(m_sVmUuid, m_sBackupUuid))
			CFileHelper::ClearAndDeleteDir(sBackupPath);
	}

	/* and remove VmUuid directory if it is empty - check it as root */
	getBaseBackupList(m_sVmUuid, lstBackupUuid);
	if (lstBackupUuid.isEmpty())
		CFileHelper::ClearAndDeleteDir(sVmPath);
exit:
	if ((nRetCode == PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND) && (m_nFlags & PBT_IGNORE_NOT_EXISTS))
		nRetCode = PRL_ERR_SUCCESS;

	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_RemoveVmBackupTarget::finalizeTask()
{
	if (m_nLocked)
		for (int i = 0; (i < m_nLocked) && (i < m_lstBackupUuid.size()); ++i)
			unlockExclusive(m_lstBackupUuid.at(i));
	m_nLocked = 0;

	if (PRL_FAILED(getLastErrorCode()))
		m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	else
		m_pDispConnection->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	return;
}

/* to remove all backups of this client for this Vm */
PRL_RESULT Task_RemoveVmBackupTarget::removeAllBackupsForVm(QString sVmPath)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sPath;
	int i;

	/* get base backup list for this user */
	getBaseBackupList(m_sVmUuid, m_lstBackupUuid, &getClient()->getAuthHelper(), PRL_BACKUP_CHECK_MODE_WRITE);
	if (m_lstBackupUuid.isEmpty()) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, m_sVmUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm \"%s\"", QSTR2UTF8(m_sVmUuid));
		return nRetCode;
	}

	/* to lock all backups */
	for (m_nLocked = 0; m_nLocked < m_lstBackupUuid.size(); ++m_nLocked)
		if (PRL_FAILED(nRetCode = lockExclusive(m_lstBackupUuid.at(m_nLocked))))
			goto exit;

	for (i = 0; i < m_lstBackupUuid.size(); ++i) {
		sPath = QString("%1/%2").arg(sVmPath).arg(m_lstBackupUuid.at(i));
		if (!CFileHelper::ClearAndDeleteDir(sPath)) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_REMOVE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, sPath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Can't remove \"%s\" directory", QSTR2UTF8(sPath));
			return nRetCode;
		}
	}
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* to remove base backup */
PRL_RESULT Task_RemoveVmBackupTarget::removeBaseBackup()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	m_lstBackupUuid.append(m_sBackupUuid);

	/* to lock all backups */
	for (m_nLocked = 0; m_nLocked < m_lstBackupUuid.size(); ++m_nLocked)
		if (PRL_FAILED(nRetCode = lockExclusive(m_lstBackupUuid.at(m_nLocked))))
			goto exit;

	if (!CFileHelper::ClearAndDeleteDir(m_cTargetDir.absoluteFilePath())) {
		nRetCode = PRL_ERR_BACKUP_CANNOT_REMOVE_DIRECTORY;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_cTargetDir.absoluteFilePath(), EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Can't remove \"%s\" directory", QSTR2UTF8(m_cTargetDir.absoluteFilePath()));
		goto exit;
	}
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* to remove N incremental backup : remove all >= N */
PRL_RESULT Task_RemoveVmBackupTarget::removeIncrementalBackup(QString sBackupPath)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QList<unsigned> lstPartialBackupNumber;
	PartialBackupItem cPartialBackupItem;
	int i, j;
	QStringList lstTibFile, lstTmp;

	m_lstBackupUuid.append(m_sBackupUuid);

	getPartialBackupList(m_sVmUuid, m_sBackupUuid, lstPartialBackupNumber);
	for (i = 0; i < lstPartialBackupNumber.size(); ++i) {
		if (m_nBackupNumber > lstPartialBackupNumber.at(i))
			continue;
		if (PRL_SUCCEEDED(loadPartialBackupMetadata(
				m_sVmUuid, m_sBackupUuid, lstPartialBackupNumber.at(i), &cPartialBackupItem)))
		{
			lstTmp = cPartialBackupItem.getTibFileList();
			/* remove tib files for this backup */
			for (j = 0; j < lstTmp.size(); ++j)
				lstTibFile.append(lstTmp.at(j));
		}
	}

	/* to lock all backups */
	for (m_nLocked = 0; m_nLocked < m_lstBackupUuid.size(); ++m_nLocked)
		if (PRL_FAILED(nRetCode = lockExclusive(m_lstBackupUuid.at(m_nLocked))))
			goto exit;

	for (i = 0; i < lstPartialBackupNumber.size(); ++i) {
		if (m_nBackupNumber > lstPartialBackupNumber.at(i))
			continue;
		m_cTargetDir.setFile(QString("%1/%2").arg(sBackupPath).arg(lstPartialBackupNumber.at(i)));

		if (!CFileHelper::ClearAndDeleteDir(m_cTargetDir.absoluteFilePath())) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_REMOVE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_cTargetDir.absoluteFilePath(), EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Can't remove \"%s\" directory",
				QSTR2UTF8(m_cTargetDir.absoluteFilePath()));
			goto exit;
		}
	}
	/* now remove tib files for this backups */
	for (j = 0; j < lstTibFile.size(); ++j) {
		QFile file(QString("%1/%2").arg(sBackupPath).arg(lstTibFile.at(j)));
		if (!file.remove())
			WRITE_TRACE(DBG_FATAL, "[%s] QFile(%s)::remove error: %s",
				__FUNCTION__, QSTR2UTF8(file.fileName()), QSTR2UTF8(file.errorString()));
	}
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}
