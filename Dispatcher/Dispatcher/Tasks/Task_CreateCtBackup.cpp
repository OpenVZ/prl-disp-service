///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateCtBackup.cpp
///
/// Source task for Container backup creation
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

#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include "prlcommon/Logging/Logging.h"

#include "CDspService.h"
#include "CDspVzHelper.h"
#ifdef _LIN_
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include "CDspBackupDevice.h"
#include "vzctl/libvzctl.h"
#endif

#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "Task_CreateVmBackup.h"

namespace Backup
{
namespace Work
{
#ifdef _CT_
namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// struct Spec

Spec::Spec(quint32 lastTib_): m_deviceIndex()
{
	m_lastTib << "--last-tib" << QString::number(lastTib_);
}

Spec& Spec::setOutFile(const QString& value_)
{
	m_outFile.clear();
	m_outFile << "--local" << "--disp-mode" << "--tib-out" << value_;
	return *this;
}

Spec& Spec::setSandbox(const QString& value_)
{
	m_sandbox.clear();
	if (!value_.isEmpty())
		m_sandbox << "--sandbox" << value_;

	return *this;
}

Spec& Spec::setArchive(const QString& archive_)
{
	m_archive = archive_;
	return *this;
}

QStringList Spec::getArguments() const
{
	return QStringList() << m_lastTib << m_outFile << m_sandbox;
}

///////////////////////////////////////////////////////////////////////////////
// struct Naked
// core implementation of a ct backup creation. builds a command line and runs
// the prl_backup_client.
//

struct Naked
{
	Naked(const SmartPtr<CVmConfiguration>& ct_, Task_CreateCtBackupHelper& context_):
		m_ct(ct_), m_context(&context_)
	{
	}

	QString uuid() const
	{
		return vm().getVmUuid();
	}
	bool isCanceled() const
	{
		return m_context->operationIsCancelled();
	}
	PRL_RESULT backup(const Spec& spec_)
	{
		return backup(vm().getHomePath(), spec_);
	}
	PRL_RESULT backup(const QString& path_, const Spec& spec_)
	{
		return backup(path_, ct().getMountPath(), spec_);
	}
	PRL_RESULT backup(const QString& path_, const QString& root_, const Spec& spec_);

private:
	const CCtSettings& ct() const
	{
		return *(m_ct->getCtSettings());
	}
	const CVmIdentification& vm() const
	{
		return *(m_ct->getVmIdentification());
	}

	SmartPtr<CVmConfiguration> m_ct;
	Task_CreateCtBackupHelper* m_context;
};

PRL_RESULT Naked::backup(const QString& path_, const QString& root_, const Spec& spec_)
{
	if (isCanceled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	QStringList a;
	if (m_context->getFlags() & PBT_INCREMENTAL)
		a << "append_ct";
	else
		a << "create_ct";
	// <private area> <backup path> <tib> <ct_id> <ve_root> <is_running>
	a << path_
		<< m_context->getBackupRoot()
		<< spec_.getArchive()
		<< vm().getCtId()
		<< root_
		<< (m_context->isRunning() ? "1" : "0");

	if (m_context->getFlags() & PBT_UNCOMPRESSED)
		a << "--uncompressed";

	return m_context->execute(vm(), spec_.getDeviceIndex(), a << spec_.getArguments());
}

///////////////////////////////////////////////////////////////////////////////
// struct Image
// strategy of backing up a ploop based ct.
//

struct Image
{
	typedef Task_CreateCtBackupHelper task_type;
	typedef ::Backup::Product::componentList_type componentList_type;

	Image(const SmartPtr<CVmConfiguration>& ct_,
		const componentList_type& snapshotList_, task_type& task_);

	PRL_RESULT operator()(const Spec& spec_);

private:
        Naked m_naked;
        componentList_type m_tibList;
        componentList_type m_snapshotList;
};

Image::Image(const SmartPtr<CVmConfiguration>& ct_, const componentList_type& snapshotList_,
        task_type& task_): m_naked(ct_, task_), m_snapshotList(snapshotList_)
{
	::Backup::Product::Model p(::Backup::Object::Model(ct_), ct_->getVmIdentification()->getHomePath());
	p.setStore(task_.getBackupRoot());
	m_tibList = p.getCtTibs();
}

PRL_RESULT Image::operator()(const Spec& spec_)
{
	foreach (const ::Backup::Product::component_type& t, m_tibList)
	{
		if (m_naked.isCanceled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		const QFileInfo* f = NULL;
		foreach (const ::Backup::Product::component_type& c, m_snapshotList)
		{
			if (t.first.getFolder() == c.first.getFolder())
			{
				f = &c.second;
				break;
			}
		}
		Spec s = spec_;
		PRL_RESULT e = m_naked.backup(f->absoluteFilePath(),
					s.setDeviceIndex(t.first.getDevice().getIndex())
					.setArchive(t.second.absoluteFilePath()));
		if (PRL_FAILED(e))
			return e;
	}
	return PRL_ERR_SUCCESS;
}

} // namespace Ct
#endif // _CT_
} // namespace Work
} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// struct Task_CreateCtBackupHelper

Task_CreateCtBackupHelper::Task_CreateCtBackupHelper(const SmartPtr<CDspClient>& client_,
	const SmartPtr<IOPackage>& request_):
	Task_BackupHelper(client_, request_), m_nInternalFlags()
{
}

int Task_CreateCtBackupHelper::execute(const CVmIdentification& ct_, quint32 deviceIndex_,
					const QStringList& args_)
{
	return startABackupClient(ct_.getVmName(), args_, ct_.getVmUuid(), deviceIndex_);
}

bool Task_CreateCtBackupHelper::isRunning() const
{
	return m_nInternalFlags & PVM_CT_RUNNING;
}

PRL_RESULT Task_CreateCtBackupHelper::prepareTask()
{
	CDspLockedPointer<CVmDirectory> d = CDspService::instance()
					->getVmDirManager().getVzDirectory();
	if (!d.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
		return PRL_ERR_VM_UUID_NOT_FOUND;
	}
	m_vzDirectory = d->getUuid();
	return PRL_ERR_SUCCESS;
}

void Task_CreateCtBackupHelper::setService(Backup::Activity::Service& value_)
{
	m_service = &value_;
	m_nInternalFlags = (m_nInternalFlags & ~PVM_CT_BACKUP) | PVM_CT_PLOOP_BACKUP;
}

PRL_RESULT Task_CreateCtBackupHelper::do_
	(const SmartPtr<CVmConfiguration>& config_, Backup::Work::Ct::Spec spec_)
{
	spec_.setSandbox(CDspService::instance()->getDispConfigGuard()
		.getDispCommonPrefs()->getBackupSourcePreferences()->getSandbox());
	if (NULL == m_service)
		return PRL_ERR_UNEXPECTED;

	::Backup::Activity::Object::Model a;
	PRL_RESULT e = m_service->find(MakeVmIdent(m_sVmUuid, m_vzDirectory), a);
	if (PRL_FAILED(e))
		return e;

	return Backup::Work::Ct::Image
		(config_, a.getSnapshot().getComponents(), *this)(spec_);
}

/*******************************************************************************

 Ct Backup creation task for client

********************************************************************************/
Task_CreateCtBackupSource::Task_CreateCtBackupSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p,
		::Backup::Activity::Service& service_)
:
Task_CreateCtBackupHelper(client, p),
m_bLocalMode(false),
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
	m_nTotalSize = 0;
	m_nOriginalSize = 0;
}

Task_CreateCtBackupSource::~Task_CreateCtBackupSource()
{
}

PRL_RESULT Task_CreateCtBackupSource::prepareTask()
{
#ifdef _CT_
	// check params in existing VM
	CDspService* s = CDspService::instance();
	SmartPtr<CDspClient> client = getClient();
	VIRTUAL_MACHINE_STATE state;
	PRL_RESULT nRetCode = Task_CreateCtBackupHelper::prepareTask();
	if (PRL_FAILED(nRetCode))
		goto exit;

	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	m_pCtConfig = s->getVzHelper()->getCtConfig(client, m_sVmUuid);
	if (!m_pCtConfig.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}
	if (VZCTL_LAYOUT_4 < m_pCtConfig->getCtSettings()->getLayout())
		setService(*m_service);
	else
	{
		WRITE_TRACE(DBG_FATAL, "Unsupported Container's layout (%d)",
				m_pCtConfig->getCtSettings()->getLayout());
		nRetCode = PRL_ERR_UNIMPLEMENTED;
		goto exit;
	}
	/* check existing Ct state */
	nRetCode = s->getVzHelper()->getVzlibHelper().get_env_status(m_sVmUuid, state);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "get_env_status() failer. Reason: %#x (%s)",
				nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	if (state == VMS_RUNNING)
		setInternalFlags(getInternalFlags() | PVM_CT_RUNNING);

	{
		::Backup::Task::Director d(CDspService::instance()->getVmDirHelper(), *m_service,
			CDspService::instance()->getDispConfigGuard());
		::Backup::Activity::Ct::Builder< ::Backup::Snapshot::Ct::Flavor::Mount>
			b(::Backup::Activity::Builder(MakeVmIdent(m_sVmUuid, getVzDirectory()), *this),
				::Backup::Snapshot::Ct::Flavor::Mount(m_sVmUuid, CVzOperationHelper()),
				s->getVzHelper());
		if (PRL_FAILED(nRetCode = d(b)))
			goto exit;
	}
	m_sCtName = m_pCtConfig->getVmIdentification()->getVmName();
	m_sCtHomePath = m_pCtConfig->getVmIdentification()->getHomePath();

	if (s->getShellServiceHelper().isLocalAddress(m_sServerHostname))
	{
		WRITE_TRACE(DBG_FATAL, "Enable Backup local mode");
		m_bLocalMode = true;
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
#else
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

PRL_RESULT Task_CreateCtBackupSource::waitForTargetFinished()
{
	SmartPtr<IOPackage> respPkg;
	IOSendJob::Response pResponse;
	bool bExited = false;
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	// Handle reply from target
	while (!bExited) {
		if (getIoClient()->waitForResponse(m_hJob) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
			nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
			break;
		}
		pResponse = getIoClient()->takeResponse(m_hJob);
		if (pResponse.responseResult != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Create Vm backup failed to take response: %x",
					pResponse.responseResult);
			nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
			break;
		}

		foreach(respPkg, pResponse.responsePackages) {
			if (respPkg->header.type == PVE::DspVmEvent) {
				// FIXME: handle progress
				continue;
			} else if (respPkg->header.type == DispToDispResponseCmd) {
				// Task finished
				CDispToDispCommandPtr pCmd  = CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd,
						UTF8_2QSTR(respPkg->buffers[0].getImpl()));
				CDispToDispResponseCommand *pResponseCmd =
					CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
				getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
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

PRL_RESULT Task_CreateCtBackupSource::run_body()
{
#ifdef _CT_
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> p;

/* TODO : remote check */

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if (PRL_FAILED(nRetCode = sendStartRequest()))
		goto exit;

	/* part one : plain copy of config files */
	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderClient(m_pIoClient));
	m_pVmCopySource = SmartPtr<CVmFileListCopySource>(
			new CVmFileListCopySource(
				m_pSender.getImpl(),
				m_sVmUuid,
				m_sCtHomePath,
				m_nTotalSize,
				getLastError(),
				m_nTimeout));

	m_pVmCopySource->SetRequest(getRequestPackage());
	m_pVmCopySource->SetVmDirectoryUuid(getVzDirectory());
	m_pVmCopySource->SetProgressNotifySender(Backup::NotifyClientsWithProgress);

	{
		::Backup::Activity::Object::Model a;
		nRetCode = m_service->find(MakeVmIdent(m_sVmUuid, getVzDirectory()), a);
		if (PRL_FAILED(nRetCode))
			goto exit;

		m_DirList = a.getEscort().getFolders();
		m_FileList = a.getEscort().getFiles();
	}
	nRetCode = m_pVmCopySource->Copy(m_DirList, m_FileList);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	if (m_bLocalMode) {
		nRetCode = waitForTargetFinished();
	} else {
		if (operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		nRetCode = do_(m_pCtConfig, Backup::Work::Ct::Spec(m_nBackupNumber));
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
		nRetCode = waitForTargetFinished();
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
#else
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

/* Finalize task */
void Task_CreateCtBackupSource::finalizeTask()
{
#ifdef _CT_
	if (m_nSteps & BACKUP_REGISTER_EX_OP) {
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_sVmUuid, getVzDirectory(), PVE::DspCmdCreateVmBackup, getClient());
	}

	/* umount mounted Container */
	if (getInternalFlags() & PVM_CT_MOUNTED) {
		int res = m_VzOpHelper.umount_env(m_sVmUuid);
		if (res)
			WRITE_TRACE(DBG_FATAL, "Fail to cleanup after backup: Can not umount"
				" Container '%s', err = %d", QSTR2UTF8(m_sVmUuid), res);
	}
	if ((getInternalFlags() & PVM_CT_PLOOP_BACKUP) && !m_sCtHomePath.isEmpty())
		m_service->finish(MakeVmIdent(m_sVmUuid, getVzDirectory()), getClient());

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
				getVzDirectory(), m_sVmUuid);

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
#else
	WRITE_TRACE(DBG_FATAL, "Linux containers does not implemented");
	setLastErrorCode(PRL_ERR_UNIMPLEMENTED);
	getClient()->sendResponseError(getLastError(), getRequestPackage());
#endif
}

#ifdef _CT_
/* send start request for remote dispatcher and wait reply from dispatcher */
PRL_RESULT Task_CreateCtBackupSource::sendStartRequest()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pBackupCmd;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<IOPackage> pReply;
	quint32 nFlags;
	QString sServerUuid;
	QFileInfo ctBundle(m_sCtHomePath);

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	{
		sServerUuid = CDspService::instance()->getDispConfigGuard().
			getDispConfig()->getVmServerIdentification()->getServerUuid();
	}

	if (m_bLocalMode)
		pBackupCmd = CDispToDispProtoSerializer::CreateVmBackupCreateLocalCommand(
			m_sVmUuid,
			m_sCtName,
			QHostInfo::localHostName(),
			sServerUuid,
			m_sDescription,
			m_sCtHomePath,
			QString(),
			m_pCtConfig->toString(),
			m_nOriginalSize,
			m_nFlags,
			getInternalFlags());
	else
		pBackupCmd = CDispToDispProtoSerializer::CreateVmBackupCreateCommand(
			m_sVmUuid,
			m_sCtName,
			QHostInfo::localHostName(),
			sServerUuid,
			m_sDescription,
			m_pCtConfig->toString(),
			m_nOriginalSize,
			(quint32)ctBundle.permissions(),
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
	if (getInternalFlags() & PVM_CT_PLOOP_BACKUP && m_nRemoteVersion < BACKUP_PROTO_V3)
	{
		WRITE_TRACE(DBG_FATAL, "Unsupported protocol version %d. >= %d is expected",
			m_nRemoteVersion, BACKUP_PROTO_V3);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	m_sBackupUuid = pCreateReply->GetBackupUuid();
	m_nBackupNumber = pCreateReply->GetBackupNumber();
	setBackupRoot(pCreateReply->GetBackupRootPath());
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
void Task_CreateCtBackupSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
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

#ifdef _LIN_
namespace {

bool removeAttachedBackups(SmartPtr<CVmConfiguration>& conf)
{
	return ::Backup::Device::Dao(conf).deleteAll();
}

} // anonymous namespace
#endif // _LIN_

#endif // _CT_
