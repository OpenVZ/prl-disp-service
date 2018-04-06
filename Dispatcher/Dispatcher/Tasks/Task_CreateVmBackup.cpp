///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackup.cpp
///
/// Source task for Vm backup creation
///
/// @author krasnov@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <memory>

#include <QTemporaryFile>
#include "CDspClientManager.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"
#include "Libraries/StatesStore/SavedStateTree.h"
#include "prlcommon/Std/PrlTime.h"

#include "Task_CreateVmBackup.h"
#include "Task_BackupHelper_p.h"
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

///////////////////////////////////////////////////////////////////////////////
// class Task_CreateVmBackup

PRL_RESULT Task_CreateVmBackup::copyEscort(const ::Backup::Escort::Model& escort_,
	const QString& directory_, const QString& source_)
{
	CVmFileListCopySenderClient s(m_pIoClient);
	CVmFileListCopySource c(&s, m_sVmUuid, source_, 0, getLastError(),
			m_nTimeout);

	c.SetRequest(getRequestPackage());
	c.SetVmDirectoryUuid(directory_);
	c.SetProgressNotifySender(Backup::NotifyClientsWithProgress);

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	return c.Copy(escort_.getFolders(), escort_.getFiles());
}

PRL_RESULT Task_CreateVmBackup::backupHardDiskDevices(const ::Backup::Activity::Object::Model& activity_,
		::Backup::Work::object_type& variant_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	m_product->setStore(m_sBackupRootPath);
	if (BACKUP_PROTO_V4 > m_nRemoteVersion)
		return ::Backup::Work::Acronis::ACommand(*this, activity_).do_(variant_);

	::Backup::Work::Push::VCommand v(*this, activity_);
	m_product->setSuffix(::Backup::Suffix(getBackupNumber())());

	typedef ::Backup::Tunnel::Source::Factory factory_type;
	factory_type f(m_sServerHostname, getIoClient());
	factory_type::result_type r = f(m_nFlags);
	if (r.isFailed())
		return r.error();
	
	v.setTunnel(r.value());
	return v(m_urls, variant_);
}

/* send start request for remote dispatcher and wait reply from dispatcher */
PRL_RESULT Task_CreateVmBackup::sendStartRequest(const ::Backup::Activity::Object::Model& activity_,
		::Backup::Work::object_type& variant_)
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

	Prl::Expected<QStringList, PRL_RESULT> e =
		::Backup::Work::Push::Bitmap::Getter(*this, m_pVmConfig)(activity_, variant_);
	if (e.isFailed())
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;

	pBackupCmd = CDispToDispProtoSerializer::CreateVmBackupCreateCommand(
			m_sVmUuid,
			m_sVmName,
			QHostInfo::localHostName(),
			sServerUuid,
			m_sDescription,
			m_pVmConfig->toString(),
			m_nOriginalSize,
			(quint32)vmBundle.permissions(),
			e.value(),
			m_nFlags,
			getInternalFlags());

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
	if (BACKUP_PROTO_V4 > m_nRemoteVersion) {
		WRITE_TRACE(DBG_FATAL, "Error: Backup to legacy storage is not allowed!");
		return PRL_ERR_BACKUP_UNSUPPORTED_STORAGE_VERSION;
	}

	m_sBackupUuid = pCreateReply->GetBackupUuid();
	m_nBackupNumber = pCreateReply->GetBackupNumber();
	m_sBackupRootPath = pCreateReply->GetBackupRootPath();
	nFlags = pCreateReply->GetFlags();
	quint64 nFreeDiskSpace;

	if (pCreateReply->GetFreeDiskSpace(nFreeDiskSpace))
	{
		nRetCode = checkFreeDiskSpace(m_nOriginalSize, nFreeDiskSpace, true);
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

	for (unsigned i = 2; i < pReply->header.buffersNumber; i += 2) {
		QFileInfo x(UTF8_2QSTR(pReply->buffers[i-1].getImpl()));
		QString u(UTF8_2QSTR(pReply->buffers[i].getImpl()));
		m_urls.push_back(::Backup::Activity::Object::component_type(x, u));
	}

	return PRL_ERR_SUCCESS;
}

// cancel command
void Task_CreateVmBackup::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	killABackupClient();
	SmartPtr<IOClient> x = getIoClient();
	if (x.isValid())
	{
		x->urgentResponseWakeUp(m_hJob);
		x->disconnectClient();
	}
}

PRL_RESULT Task_CreateVmBackup::waitForTargetFinished(int cmd_, QString& error_)
{
	/*
	   Now target side wait new acronis proxy commands due to acronis have not call to close connection.
	   To fix it will send command to close connection from here.
	   Pay attention: on success and on failure both we will wait reply from target.
	*/
	SmartPtr<IOPackage> p  = IOPackage::createInstance(cmd_, 0);
	if (PRL_FAILED(SendPkg_(p))) {
		WRITE_TRACE(DBG_FATAL, "Final package sending failure");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	// Handle reply from target
	bool bExited = false;
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	while (!bExited) {
		if (getIoClient()->waitForResponse(m_hJob) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
			nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
			break;
		}
		IOSendJob::Response pResponse = getIoClient()->takeResponse(m_hJob);
		if (pResponse.responseResult != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Create Vm backup failed to take response: %x",
					pResponse.responseResult);
			nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
			break;
		}

		foreach(SmartPtr<IOPackage> respPkg, pResponse.responsePackages) {
			if (respPkg->header.type == PVE::DspVmEvent) {
				// FIXME: handle progress
				continue;
			} else if (respPkg->header.type == DispToDispResponseCmd) {
				// Task finished
				CDispToDispCommandPtr pCmd  = CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd,
						UTF8_2QSTR(respPkg->buffers[0].getImpl()));
				CDispToDispResponseCommand *pResponseCmd =
					CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
				error_ = pResponseCmd->GetErrorInfo()->toString();
				nRetCode = pResponseCmd->GetRetCode();
				bExited = true;
				break;
			} else {
				WRITE_TRACE(DBG_FATAL, "Unexpected package with type %d",
						respPkg->header.type);
			}
		}
	}
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackup::doBackup(const QString& source_, ::Backup::Work::object_type& variant_)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString e;

	m_product = SmartPtr< ::Backup::Product::Model>(
		new ::Backup::Product::Model(::Backup::Object::Model(m_pVmConfig), m_sVmHomePath));

	::Backup::Activity::Object::Model a;
	nRetCode = m_service->find(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), a);
	if (PRL_FAILED(nRetCode))
		goto exit;

	if (PRL_FAILED(nRetCode = sendStartRequest(a, variant_)))
		goto exit;

	/* part one : plain copy of config files */
	nRetCode = copyEscort(a.getEscort(), m_sVmDirUuid, source_);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (!m_urls.isEmpty())
		nRetCode = backupHardDiskDevices(a, variant_);

	if (PRL_FAILED(nRetCode))
		(void)waitForTargetFinished(ABackupProxyCancelCmd, e);
	else {
		nRetCode = waitForTargetFinished(ABackupProxyFinishCmd, e);
		if (PRL_FAILED(nRetCode) && !e.isEmpty())
			getLastError()->fromString(e);
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CreateVmBackup::finalizeTask()
{
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
				m_sVmDirUuid, m_sVmUuid);

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
/*******************************************************************************

 Vm Backup creation task for client

********************************************************************************/
Task_CreateVmBackupSource::Task_CreateVmBackupSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p,
		::Backup::Activity::Service& service_)
: Task_CreateVmBackup(client, p)
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
	m_service = &service_;
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
	::Backup::Activity::Vm::Builder< ::Backup::Snapshot::Vm::Push::Subject> b
		(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), *this);

	if (!QFile::exists(VZ_BACKUP_CLIENT))
	{
		return CDspTaskFailure(*this)
			(PRL_ERR_VZ_OPERATION_FAILED,
			 QString("Backup client is not installed"));
	}

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

	::Backup::Work::object_type o =
		::Backup::Work::object_type(::Backup::Work::Vm(*this));
	return doBackup(m_sSourcePath, o);
}

/* Finalize task */
void Task_CreateVmBackupSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	Task_CreateVmBackup::finalizeTask();
}

