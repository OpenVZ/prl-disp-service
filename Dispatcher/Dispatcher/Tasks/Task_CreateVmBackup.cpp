///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackup.cpp
///
/// Source task for Vm backup creation
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

#include <memory>

#include <QTemporaryFile>

#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"
#include "Libraries/StatesStore/SavedStateTree.h"
#include "prlcommon/Std/PrlTime.h"

#include "Task_CreateVmBackup.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "prlxmlmodel/BackupTree/BackupItem.h"
#include "prlxmlmodel/DiskDescriptor/CDiskXML.h"
#include "CDspVzHelper.h"
#include "CDspVmSnapshotStoreHelper.h"
#include "CDspVmBackupInfrastructure.h"

#ifdef _LIN_
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include "CDspBackupDevice.h"
#include "vzctl/libvzctl.h"
#endif

/*******************************************************************************

 Vm Backup creation task for client

********************************************************************************/
Task_CreateVmBackupSource::Task_CreateVmBackupSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p,
		::Backup::Activity::Service& service_)
: Task_BackupHelper(client, p),
m_pVmConfig(new CVmConfiguration()),
m_service(&service_)
{
	CProtoCreateVmBackupCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateVmBackupCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_sVmUuid = pCmd->GetVmUuid();
	m_sDescription = pCmd->GetDescription();
	m_sServerHostname = pCmd->GetServerHostname();
	m_nServerPort = pCmd->GetServerPort();
	m_sServerSessionUuid = pCmd->GetServerSessionUuid();
	m_nFlags = pCmd->GetFlags();
	/* if backup type undefined - set as incremental type */
	if ((m_nFlags & (PBT_INCREMENTAL | PBT_FULL)) == 0 )
		m_nFlags |= PBT_INCREMENTAL;
	m_sVmDirUuid = getClient()->getVmDirectoryUuid();
	m_nTotalSize = 0;
	m_nOriginalSize = 0;
}

Task_CreateVmBackupSource::~Task_CreateVmBackupSource()
{
}

PRL_RESULT Task_CreateVmBackupSource::prepareTask()
{
	// check params in existing VM
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	::Backup::Task::Director d(CDspService::instance()->getVmDirHelper(), *m_service,
		CDspService::instance()->getDispConfigGuard());
	::Backup::Activity::Vm::Builder b(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), *this);
	/*
	   connect to remote dispatcher _before_ snapshot creation:
	   backup initiator (PMC) can exit and close all connection
	   during snapshot creation (#460207), but in this case session UUID will invalid
	   and we cant to connect to dispather after snapshot creation
	*/
	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	if (PRL_FAILED(nRetCode = d(b)))
		goto exit;

	m_sVmName = b.getReference().getName();
	m_pVmConfig = b.getReference().getConfig();
	m_sSourcePath = b.getReference().getStore();
	m_sVmHomePath = b.getReference().getHome();

	if (operationIsCancelled())
	{
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackupSource::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> p;

	::Backup::Activity::Object::Model a;
	nRetCode = m_service->find(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), a);
	if (PRL_FAILED(nRetCode))
		goto exit;

/* TODO : remote check */

//	https://jira.sw.ru/browse/PSBM-22124
//	m_nOriginalSize = calcOriginalSize();

	if (PRL_FAILED(nRetCode = sendStartRequest()))
		goto exit;

	/* part one : plain copy of config files */
	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderClient(m_pIoClient));
	m_pVmCopySource = SmartPtr<CVmFileListCopySource>(
			new CVmFileListCopySource(
				m_pSender.getImpl(),
				m_sVmUuid,
				m_sSourcePath,
				m_nTotalSize,
				getLastError(),
				m_nTimeout));


	m_pVmCopySource->SetRequest(getRequestPackage());
	m_pVmCopySource->SetVmDirectoryUuid(getClient()->getVmDirectoryUuid());
	m_pVmCopySource->SetProgressNotifySender(Backup::NotifyClientsWithProgress);

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	nRetCode = m_pVmCopySource->Copy(a.getEscort().getFolders(), a.getEscort().getFiles());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	/* part two : backup of hdd's via acronis library */
	nRetCode = backupHardDiskDevices(a);
	/*
	   Now target side wait new acronis proxy commands due to acronis have not call to close connection.
	   To fix it will send command to close connection from here.
	   Pay attention: on success and on failure both we will wait reply from target.
	   */
	if (PRL_FAILED(nRetCode)) {
		p = IOPackage::createInstance(ABackupProxyCancelCmd, 0);
		WRITE_TRACE(DBG_FATAL, "send ABackupProxyCancelCmd command");
		SendPkg(p);
	} else {
		p = IOPackage::createInstance(ABackupProxyFinishCmd, 0);
		nRetCode = SendPkg(p);
		// TODO:	nRetCode = SendReqAndWaitReply(p);
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* Finalize task */
void Task_CreateVmBackupSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	if (m_pVmConfig.isValid())
		m_service->finish(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), getClient());

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		CVmEvent event(PET_DSP_EVT_CREATE_BACKUP_FINISHED, m_sVmUuid, PIE_DISPATCHER);
		event.addEventParameter(
			new CVmEventParameter( PVE::String,
					(m_nFlags&PBT_FULL) ?
					m_sBackupUuid :
					QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber),
					EVT_PARAM_BACKUP_CMD_BACKUP_UUID) );
		event.addEventParameter( new CVmEventParameter( PVE::String,
					m_sDescription,
					EVT_PARAM_BACKUP_CMD_DESCRIPTION ) );
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage,
				getClient()->getVmDirectoryUuid(), m_sVmUuid);

		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		CProtoCommandDspWsResponse *pDspWsResponseCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		pDspWsResponseCmd->AddStandardParam(m_sVmUuid);
		pDspWsResponseCmd->AddStandardParam(
			(m_nFlags&PBT_FULL) ? m_sBackupUuid : QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber));
		getClient()->sendResponse(pResponse, getRequestPackage());
	} else {
		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
}

quint64 Task_CreateVmBackupSource::calcOriginalSize()
{
	if (operationIsCancelled())
		return 0;

	::Backup::Object::Model m(m_pVmConfig);
	if (m.isBad()) {
		return 0;
	}
 	quint64 nSize = 0;
	foreach (CVmHardDisk* pHdd, m.getImages()) {
		QFileInfo fileInfo(pHdd->getSystemName());
		if (fileInfo.isAbsolute())
		{
			// NB. skip the external HDDs from the required disk space
			// calculation to maintain compatibility.
			continue;
		}
		QString sDiskImageDir = QString("%1/%2").arg(m_sVmHomePath).arg(fileInfo.fileName());
		quint64 nDirSize;
		if (PRL_FAILED(CFileHelper::GetDirSize(sDiskImageDir, &nDirSize)))
		{
			WRITE_TRACE(DBG_FATAL, "failed to calc the %s directory size", QSTR2UTF8(sDiskImageDir));
			continue;
		}
		quint64 nDiskSize = pHdd->getSize();
		nDiskSize *= (1024 * 1024);
		// NB. this way igor@ tries to workaround imprecise space
		// calculation in presense of snapshots.
		if (nDirSize > nDiskSize)
			nSize += nDiskSize;
		else
			nSize += nDirSize;
		WRITE_TRACE(DBG_DEBUG, "%s nDiskSize=%llu nDirSize=%llu",
			QSTR2UTF8(sDiskImageDir), nDiskSize, nDirSize);
        }
	return nSize;
}

PRL_RESULT Task_CreateVmBackupSource::backupHardDiskDevices(const ::Backup::Activity::Object::Model& activity_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString b = CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()
		->getBackupSourcePreferences()->getSandbox();
	::Backup::Product::Model p(::Backup::Object::Model(m_pVmConfig), m_sVmHomePath);
	p.setStore(m_sBackupRootPath);
	foreach (const ::Backup::Product::component_type& t,
		p.getVmTibs())
	{
		const QFileInfo* f = NULL;
		nRetCode = PRL_ERR_UNEXPECTED;
		foreach (const ::Backup::Product::component_type& c,
				activity_.getSnapshot().getComponents())
		{
			if (t.first.getFolder() == c.first.getFolder())
			{
				f = &c.second;
				break;
			}
		}
		if (NULL == f)
			break;

		QStringList backupArgs;
		if (m_nFlags & PBT_INCREMENTAL)
			backupArgs << "append";
		else
			backupArgs << "create";

		backupArgs << t.first.getFolder() << p.getStore().absolutePath()
			<< t.second.absoluteFilePath()
			<< activity_.getSnapshot().getUuid()
			<< f->absoluteFilePath()
			<< "--sandbox" << b;
		if (m_nFlags & PBT_UNCOMPRESSED)
			backupArgs.append("--uncompressed");

		if (PRL_FAILED(nRetCode = startABackupClient(m_sVmName, backupArgs, getLastError(),
							m_sVmUuid, t.first.getDevice().getIndex(),
							false, m_nBackupTimeout)))
			break;
	}
	return nRetCode;
}

/* send start request for remote dispatcher and wait reply from dispatcher */
PRL_RESULT Task_CreateVmBackupSource::sendStartRequest()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pBackupCmd;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<IOPackage> pReply;
	quint32 nFlags;
	QString sServerUuid;
	QFileInfo vmBundle(m_sVmHomePath);

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	{
		sServerUuid = CDspService::instance()->getDispConfigGuard().
			getDispConfig()->getVmServerIdentification()->getServerUuid();
	}

	pBackupCmd = CDispToDispProtoSerializer::CreateVmBackupCreateCommand(
			m_sVmUuid,
			m_sVmName,
			QHostInfo::localHostName(),
			sServerUuid,
			m_sDescription,
			m_pVmConfig->toString(),
			m_nOriginalSize,
			(quint32)vmBundle.permissions(),
			m_nFlags);

	pPackage = DispatcherPackage::createInstance(
			pBackupCmd->GetCommandId(),
			pBackupCmd->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply, m_hJob)))
		return nRetCode;

	if ((pReply->header.type != VmBackupCreateFirstReply) && (pReply->header.type != DispToDispResponseCmd)) {
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x or %x",
			pReply->header.type, DispToDispResponseCmd, VmBackupCreateFirstReply);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	CDispToDispCommandPtr pDspReply = CDispToDispProtoSerializer::ParseCommand(
		(Parallels::IDispToDispCommands)pReply->header.type, UTF8_2QSTR(pReply->buffers[0].getImpl()));

	if (pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pDspReply);

		nRetCode = pResponseCmd->GetRetCode();
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "sendStartRequest response failed: %s ",
				PRL_RESULT_TO_STRING(nRetCode));
			getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
			return nRetCode;
		}
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	CVmBackupCreateFirstReply *pCreateReply =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupCreateFirstReply>(pDspReply);

	m_nRemoteVersion = pCreateReply->GetVersion();
	m_sBackupUuid = pCreateReply->GetBackupUuid();
	m_nBackupNumber = pCreateReply->GetBackupNumber();
	m_sBackupRootPath = pCreateReply->GetBackupRootPath();
	nFlags = pCreateReply->GetFlags();
	quint64 nFreeDiskSpace;

	if (pCreateReply->GetFreeDiskSpace(nFreeDiskSpace))
	{
		nRetCode = checkFreeDiskSpace(m_sVmUuid, m_nOriginalSize, nFreeDiskSpace, true);
		if (PRL_FAILED(nRetCode))
			return nRetCode;
	}

	if ((m_nFlags & PBT_INCREMENTAL) && (nFlags & PBT_FULL)) {
		CVmEvent event(PET_DSP_EVT_VM_MESSAGE, m_sVmUuid, PIE_DISPATCHER, PRL_WARN_BACKUP_HAS_NOT_FULL_BACKUP);
		SmartPtr<IOPackage> pPackage =
			DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		getClient()->sendPackage(pPackage);
	}
	m_nFlags = nFlags;

	return PRL_ERR_SUCCESS;
}

// cancel command
void Task_CreateVmBackupSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	if (m_pVmCopySource.getImpl())
		m_pVmCopySource->cancelOperation();
	killABackupClient();
	SmartPtr<IOClient> x = getIoClient();
	if (x.isValid())
	{
		x->urgentResponseWakeUp(m_hJob);
		x->disconnectClient();
	}
}

