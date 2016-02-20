///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackup.cpp
///
/// Source and target tasks for Vm backup creation
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

#include "Task_CreateSnapshot.h"
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

namespace Backup
{
namespace Work
{
#ifdef _CT_
namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// struct Spec

struct Spec
{
	Spec(const QString& cache_, quint32 lastTib_);

	const QString& getArchive() const
	{
		return m_archive;
	}
	Spec& setArchive(const QString& archive_);
	quint32 getDeviceIndex() const
	{
		return m_deviceIndex;
	}
	Spec& setDeviceIndex(quint32 value_)
	{
		m_deviceIndex = value_;
		return *this;
	}
	Spec& noCache();
	QStringList getArguments() const;
	Spec& setOutFile(const QString& value_);
	Spec& setSandbox(const QString& value_);

private:
	quint32 m_deviceIndex;
	QString m_archive;
	QStringList m_cache;
	QStringList m_lastTib;
	QStringList m_outFile;
	QStringList m_sandbox;
};

Spec::Spec(const QString& cache_, quint32 lastTib_): m_deviceIndex()
{
	m_lastTib << "--last-tib" << QString::number(lastTib_);
	if (!cache_.isEmpty())
		m_cache << "--vzcache-dir" << cache_;
}

Spec& Spec::noCache()
{
	m_cache.clear();
	return *this;
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
	return QStringList() << m_lastTib << m_cache << m_outFile << m_sandbox;
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

#ifdef _LIN_

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

#elif defined(_WIN_) // _LIN_

///////////////////////////////////////////////////////////////////////////////
// struct Windows
// strategy of backing up a windows ct.
//

struct Windows
{
	Windows(const SmartPtr<CVmConfiguration>& ct_, const Naked& naked_);

	PRL_RESULT operator()(Spec spec_);

private:
	Naked m_naked;
	QString m_path;
	QString m_root;
	quint32 m_deviceIndex;
};

Windows::Windows(const SmartPtr<CVmConfiguration>& ct_, const Naked& naked_):
	m_naked(naked_), m_deviceIndex(0)
{
	CVmHardware * pVmHardware = ct_->getVmHardwareList();
	for (int i = 0; i < pVmHardware->m_lstHardDisks.size(); ++i) {
		QFileInfo fileInfo;
		CVzHardDisk *pHdd = dynamic_cast<CVzHardDisk*>(pVmHardware->m_lstHardDisks.at(i));
		if (pHdd == NULL)
			continue;
		m_root = pHdd->getSystemUuid();
		m_deviceIndex = pHdd->getIndex();
		fileInfo.setFile(pHdd->getSystemName());
		if (fileInfo.isRelative()) {
			m_path = QString("%1/%2")
				.arg(ct_->getVmIdentification()->getHomePath())
				.arg(fileInfo.fileName());
		} else {
			m_path = fileInfo.absoluteFilePath();
		}
		break;
	}
}

PRL_RESULT Windows::operator()(Spec spec_)
{
	return m_naked.backup(m_path, m_root,
		spec_.noCache().setDeviceIndex(m_deviceIndex)
			.setArchive(PRL_CT_BACKUP_TIB_FILE_NAME));
}

#endif // _LIN_
} // namespace Ct
#endif // _CT_
} // namespace Work
} // namespace Backup

static void NotifyClientsWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sVmDirectoryUuid,
		const QString &sVmUuid,
		int nPercents)
{
	CVmEvent event(PET_DSP_EVT_BACKUP_PROGRESS_CHANGED, sVmUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);

	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirectoryUuid, sVmUuid);
}

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
	if (CDspService::instance()->getShellServiceHelper().isLocalAddress(m_sServerHostname))
	{
		WRITE_TRACE(DBG_FATAL, "Enable Backup local mode");
		m_bLocalMode = true;
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
	m_pVmCopySource->SetProgressNotifySender(NotifyClientsWithProgress);

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

	if (m_bLocalMode) {
		SmartPtr<IOPackage> respPkg;
		IOSendJob::Response pResponse;
		bool bExited = false;

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
	} else {

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

	if (m_bLocalMode)
		pBackupCmd = CDispToDispProtoSerializer::CreateVmBackupCreateLocalCommand(
			m_sVmUuid,
			m_sVmName,
			QHostInfo::localHostName(),
			sServerUuid,
			m_sDescription,
			m_sVmHomePath,
			QString(),
			m_pVmConfig->toString(),
			m_nOriginalSize,
			m_nFlags);
	else
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
	bool x = -1 != args_.indexOf("--local");
	return startABackupClient(ct_.getVmName(), args_, getLastError(),
				ct_.getVmUuid(), deviceIndex_, x, m_nBackupTimeout);
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
	if (m_nInternalFlags & PVM_CT_VZFS_BACKUP)
	{
		Backup::Work::Ct::Naked n(config_, *this);
		return n.backup(spec_.setArchive(PRL_CT_BACKUP_TIB_FILE_NAME));
	}
	else if (m_nInternalFlags & PVM_CT_VZWIN_BACKUP)
	{
#ifdef _WIN_
		Backup::Work::Ct::Naked n(config_, *this);
		return Backup::Work::Ct::Windows(config_, n)(spec_);
#else // _WIN_
		return PRL_ERR_UNIMPLEMENTED;
#endif // _WIN_
	}
	else if (NULL == m_service)
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
		::Backup::Device::Dao(m_pCtConfig).deleteAll();
		setInternalFlags(getInternalFlags() | PVM_CT_VZFS_BACKUP);
	}
	/* check existing Ct state */
	nRetCode = s->getVzHelper()->getVzlibHelper().get_env_status(m_sVmUuid, state);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "get_env_status() failer. Reason: %#x (%s)",
				nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	/* (linux only) we need mounted Container for effective backup #PSBM-7479 */
	if ((state == VMS_SUSPENDED || state == VMS_STOPPED) &&
		(getInternalFlags() & PVM_CT_VZFS_BACKUP)) {
		int res = m_VzOpHelper.mount_env(m_sVmUuid);
		if (res) {
			WRITE_TRACE(DBG_FATAL, "Fail to create backup: Can not mount"
				" Container '%s', err = %d", QSTR2UTF8(m_sVmUuid), res);
			nRetCode = PRL_ERR_VM_MOUNT;
			goto exit;
		}
		setInternalFlags(getInternalFlags() | PVM_CT_MOUNTED);
	} else if (state == VMS_RUNNING)
		setInternalFlags(getInternalFlags() | PVM_CT_RUNNING);

	if (getInternalFlags() & (PVM_CT_VZFS_BACKUP | PVM_CT_VZWIN_BACKUP))
	{
		nRetCode = s->getVmDirHelper().registerExclusiveVmOperation(
			m_sVmUuid, getVzDirectory(), PVE::DspCmdCreateVmBackup, getClient(),
			this->getJobUuid());
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
					__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
			goto exit;
		}
		m_nSteps |= BACKUP_REGISTER_EX_OP;
		if (operationIsCancelled()) {
			nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
			goto exit;
		}
	}
	else
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
	QString sVzCacheDir;

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
	m_pVmCopySource->SetProgressNotifySender(NotifyClientsWithProgress);

	if (getInternalFlags() & (PVM_CT_VZFS_BACKUP | PVM_CT_VZWIN_BACKUP))
	{
		if (!m_bLocalMode) {
			nRetCode = handleCtPrivateBeforeBackup(m_sVmUuid, m_sCtHomePath, m_sBackupUuid, sVzCacheDir);
			if (PRL_FAILED(nRetCode))
				goto exit;
		}
		if (PRL_FAILED(nRetCode = fillCopyContent()))
			goto exit;
	}
	else
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
#ifdef _WIN_
	/* remove zip archive filled in fillCopyContent() */
	QFile::remove(m_sCtHomePath + "\\" PRL_CT_BACKUP_ZIP_FILE_NAME);
#endif

	if (m_bLocalMode) {
		nRetCode = waitForTargetFinished();
	} else {
		if (operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		nRetCode = do_(m_pCtConfig, Backup::Work::Ct::Spec(sVzCacheDir, m_nBackupNumber));
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
	cleanupPrivateArea();

	if (m_nSteps & BACKUP_REGISTER_EX_OP) {
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_sVmUuid, getVzDirectory(), PVE::DspCmdCreateVmBackup, getClient());
	}

#ifdef _LIN_ /* VZWIN can backup unmounted CT */
	/* umount mounted Container */
	if (getInternalFlags() & PVM_CT_MOUNTED) {
		int res = m_VzOpHelper.umount_env(m_sVmUuid);
		if (res)
			WRITE_TRACE(DBG_FATAL, "Fail to cleanup after backup: Can not umount"
				" Container '%s', err = %d", QSTR2UTF8(m_sVmUuid), res);
	}
	foreach(const QString& path, m_tmpCopy)
		CFileHelper::RemoveEntry(path, &getClient()->getAuthHelper());
#endif
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

PRL_RESULT Task_CreateCtBackupSource::fillCopyContent()
{
	m_DirList.clear();
	m_FileList.clear();
#ifdef _WIN_
	// VZWIN conf
	QFileInfo config1(CVzHelper::getCtConfPath(m_pCtConfig->getVmIdentification()->getEnvId()));
	m_FileList.append(qMakePair(config1, QString(VZ_CT_CONFIG_FILE)));
	// PSBM XML conf
	QFileInfo config2(QString("%1/" VZ_CT_XML_CONFIG_FILE).arg(m_sCtHomePath));
	m_FileList.append(qMakePair(config2, QString(VZ_CT_XML_CONFIG_FILE)));

	// Now zip most private files to zip archive
	QString zipPath = CVzHelper::getVzInstallDir() + "\\bin\\zip.exe";
	QString arcPath = m_sCtHomePath + "\\" PRL_CT_BACKUP_ZIP_FILE_NAME;
	QProcess proc;
	QStringList args;

	// exclude args copied from vza backup
	args += "-!";
	args += "-r";
	args += "-S";
	args += arcPath;
	args += "*.*";
	args += "-x";
	args += "root/";
	args += "root/*.*";
	args += "*.efd";
	args += VZ_CT_XML_CONFIG_FILE;
	args += VZ_CT_CONFIG_FILE;

	proc.setWorkingDirectory(m_sCtHomePath);
	proc.start(zipPath, args);

	if (!proc.waitForFinished(ZIP_WORK_TIMEOUT)) {
		WRITE_TRACE(DBG_FATAL, "backup zip utility is not responding. Terminate it now.");
		proc.terminate();
		QFile::remove(arcPath);
	} else if (proc.exitCode() != 0) {
		WRITE_TRACE(DBG_FATAL, "backup zip utility failed code %u: %s",
			    proc.exitCode(),
			    proc.readAllStandardOutput().constData());
		QFile::remove(arcPath);
	} else {
		QFileInfo filePrivate(arcPath);
		m_FileList.append(qMakePair(filePrivate, filePrivate.fileName()));
	}
#else
	PRL_RESULT res;
	QString tmpPath;
	QString confPath = QString("%1/" VZ_CT_CONFIG_FILE).arg(m_sCtHomePath);
	if (PRL_FAILED(res = makeTemporaryCopy(confPath, tmpPath)))
		return res;
	SmartPtr<CVmConfiguration> origCtConfig(CDspService::instance()->getVzHelper()->getCtConfig(getClient(),
												    m_sVmUuid));
	if (!origCtConfig.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s", QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_VM_UUID_NOT_FOUND;
	}
	if (m_VzOpHelper.remove_disks_from_env_config(m_pCtConfig, origCtConfig, tmpPath))
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	m_FileList.append(qMakePair(QFileInfo(tmpPath), QFileInfo(confPath).fileName()));

	// PSBM XML conf
	confPath = QString("%1/" VZ_CT_XML_CONFIG_FILE).arg(m_sCtHomePath);
	if (QFileInfo(confPath).exists()) {
		if (PRL_FAILED(res = makeTemporaryCopy(confPath, tmpPath)))
			return res;
		if (PRL_FAILED(res = CDspService::instance()->getVmConfigManager().modifyConfig(tmpPath,
			getClient(), removeAttachedBackups, false, true, true, true)))
			return res;
		m_tmpCopy << tmpPath + ".backup";
		m_FileList.append(qMakePair(QFileInfo(tmpPath), QFileInfo(confPath).fileName()));
	}

#endif
#ifdef _LIN_
	if (VZCTL_LAYOUT_5 > m_pCtConfig->getCtSettings()->getLayout())
		return PRL_ERR_SUCCESS;
#endif

	QDir home(m_sCtHomePath);
	QFileInfoList infos;
	QList<QPair<QFileInfo, QString> > top;
	infos = home.entryInfoList(QStringList("scripts"), QDir::Dirs);
	if (!infos.isEmpty())
		top.append(qMakePair(infos.at(0), infos.at(0).fileName()));

	infos = home.entryInfoList(QStringList("templates"), QDir::Dirs | QDir::Files);
	if (!infos.isEmpty())
	{
		if (!infos.at(0).isSymLink())
			top.append(qMakePair(infos.at(0), infos.at(0).fileName()));
		else
		{
			QFileInfo y(infos.at(0).symLinkTarget());
			if (y.exists())
				top.append(qMakePair(y, infos.at(0).fileName()));
		}
	}
	QDir::Filters f = QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
			QDir::Hidden | QDir::NoSymLinks;
	for (int i = 0; i < top.size(); ++i)
	{
		QList<QPair<QFileInfo, QString> > subtree;
		subtree.append(top.at(i));
		QDir top(subtree.at(0).first.absoluteFilePath());
		for (int j = 0; j < subtree.size(); ++j)
		{
			infos = QDir(subtree.at(j).first.absoluteFilePath()).entryInfoList(f);
			for (int k = 0; k < infos.size(); ++k)
			{
				const QFileInfo& x = infos.at(k);
				QString r = top.dirName().append('/')
						.append(top.relativeFilePath(x.absoluteFilePath()));
				if (x.isDir())
					subtree.append(qMakePair(x, r));
				else
					m_FileList.append(qMakePair(x, r));
			}
		}
		m_DirList.append(subtree);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateCtBackupSource::makeTemporaryCopy(const QString& orig, QString& copy)
{
	QString tmp;
	{
		QTemporaryFile t(orig + ".XXXXXX");
		if (!t.open()) {
			WRITE_TRACE(DBG_FATAL, "Failed to create temporary file for %s", QSTR2UTF8(orig));
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
		tmp = t.fileName();
		t.close();
	}
	if (!QFile::copy(orig, tmp)) {
		WRITE_TRACE(DBG_FATAL, "Failed to copy %s -> %s", QSTR2UTF8(orig), QSTR2UTF8(tmp));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	m_tmpCopy << tmp;
	copy = tmp;
	return PRL_ERR_SUCCESS;
}

#endif // _CT_

/*******************************************************************************

 Vm & Ct Backup creation task for server

********************************************************************************/
Task_CreateVmBackupTarget::Task_CreateVmBackupTarget(
		SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p,
		::Backup::Activity::Service& service_)
:Task_CreateCtBackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_pVmConfig(new CVmConfiguration()),
m_bBackupLocked(false),
m_service(&service_)
{
	CVmBackupCreateCommand *pStartCommand;
	m_bLocalMode = (p->header.type == VmBackupCreateLocalCmd);

	pStartCommand = CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupCreateCommand>(pCmd);
	m_sVmUuid = pStartCommand->GetVmUuid();
	m_sVmName = pStartCommand->GetVmName();
	m_sSourceHost = pStartCommand->GetHost();
	m_sServerUuid = pStartCommand->GetServerUuid();
	m_sDescription = pStartCommand->GetDescription();
	m_nFlags = pStartCommand->GetFlags();
	m_nOriginalSize = pStartCommand->GetOriginalSize();
	setInternalFlags(pStartCommand->GetInternalFlags());
	m_hConnHandle = pDispConnection->GetConnectionHandle();
	m_nFreeDiskSpace = ~0;
	m_bABackupFirstPacket = true;
	m_bStorageRegistered = false;
	m_nRemoteVersion = pStartCommand->GetVersion();

	m_nBundlePermissions = 0;
	if (m_bLocalMode) {
		CVmBackupCreateLocalCommand *pStartLocalCommand =
				CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupCreateLocalCommand>(pCmd);
		m_pVmConfig->fromString(pStartCommand->GetVmConfig());
		m_sVmHomePath = pStartLocalCommand->GetStorage();
		QFileInfo vmBundle(m_sVmHomePath);
		m_nBundlePermissions = vmBundle.permissions();
	} else {
		if (m_nRemoteVersion >= BACKUP_PROTO_V2)
	 		m_nBundlePermissions = pStartCommand->GetBundlePermissions();
		if (m_nRemoteVersion >= BACKUP_PROTO_V3)
			m_pVmConfig->fromString(pStartCommand->GetVmConfig());
	}

	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_CreateVmBackupTarget::~Task_CreateVmBackupTarget()
{
	// #439777 protect handler from destroying object
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_CreateVmBackupTarget::validateBackupDir(const QString &sPath)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QDir backupsdir(sPath);

	/* if directory does not exist - to create */
	if (!backupsdir.exists()) {
		if (!backupsdir.mkdir(sPath)) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, sPath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(sPath));
			return nRetCode;
		}
	}

	/* and fix permissions */
	QFile fbackupsdir(QString("%1/.").arg(sPath));
	QFile::Permissions nPermissions =
			QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
			QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
			QFile::ReadOther | QFile::WriteOther | QFile::ExeOther;

	if (fbackupsdir.permissions() != nPermissions) {
		if (!fbackupsdir.setPermissions(nPermissions)) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_SET_PERMISSIONS;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, sPath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot set permissions for directory \"%s\"", QSTR2UTF8(sPath));
			return nRetCode;
		}
	}
#ifndef _WIN_
	/* set sticky bit for directory with perms 777.
	   from stat man page:
	   The `sticky' bit (S_ISVTX) on a directory means that a file in that directory can be renamed
	   or deleted only by the owner of the file, by the owner of the directory, and by a privileged process.
	*/
	struct stat st;
	if (stat(QSTR2UTF8(sPath), &st) || chmod(QSTR2UTF8(sPath), st.st_mode | S_ISVTX))
		WRITE_TRACE(DBG_FATAL, "Cannot set 'sticky' bit for directory \"%s\"", QSTR2UTF8(sPath));
#endif
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackupTarget::guessBackupType()
{
	bool f = 0 != (m_nFlags & PBT_FULL);
	do {
		if (f) {
			break;
		}
		if ((f = !m_lastBase.isValid())) {
			WRITE_TRACE(DBG_FATAL,
				"This Vm has no full backup, will create the "
				"first full backup instead of incremental");
			break;
		}
		m_sBackupUuid = m_lastBase->getUuid();
		setBackupRoot(QString("%1/%2/%3").arg(getBackupDirectory())
				.arg(m_sVmUuid).arg(m_sBackupUuid));
		if (0 != (getInternalFlags() & PVM_CT_VZFS_BACKUP)) {
			break;
		}
		PRL_RESULT e = wasHddListChanged(&f);
		if (PRL_FAILED(e))
			return e;
		if (f) {
			WRITE_TRACE(DBG_FATAL, "HDD list was changed since last base backup,"
						" will to create full backup instead of incremental");
		}
	} while(false);
	if (f) {
		m_nFlags &= ~PBT_INCREMENTAL;
		m_nFlags |= PBT_FULL;
		/* create new base backup */
		m_sBackupUuid = Uuid::createUuid().toString();
		m_nBackupNumber = 0;
		setBackupRoot(QString("%1/%2/%3").arg(getBackupDirectory()).arg(m_sVmUuid).arg(m_sBackupUuid));
		m_sTargetPath = QString("%1/" PRL_BASE_BACKUP_DIRECTORY).arg(getBackupRoot());
	} else {
		m_nBackupNumber = getNextPartialBackup(m_sVmUuid, m_sBackupUuid);
		m_sTargetPath = QString("%1/%2").arg(getBackupRoot()).arg(m_nBackupNumber);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateVmBackupTarget::prepareTask()
{
	PRL_RESULT nRetCode = Task_CreateCtBackupHelper::prepareTask();
	if (PRL_FAILED(nRetCode))
		goto exit;

	m_lastBase = SmartPtr<BackupItem>(getLastBaseBackup(m_sVmUuid,
						&getClient()->getAuthHelper(),
						PRL_BACKUP_CHECK_MODE_WRITE));
	if (PRL_FAILED(nRetCode = guessBackupType()))
		goto exit;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(
		CDspService::instance()->getIOServer(),
		m_pDispConnection->GetConnectionHandle()));
	m_pVmCopyTarget = SmartPtr<CVmFileListCopyTarget>(
		new CVmFileListCopyTarget(m_pSender.getImpl(), m_sVmUuid, m_sTargetPath, getLastError(), m_nTimeout));

	if (CFileHelper::DirectoryExists(m_sTargetPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_DIRECTORY_ALREADY_EXIST;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, m_sTargetPath, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Target directory \"%s\" already exist", QSTR2UTF8(m_sTargetPath));
		goto exit;
	}

	//Check whether backup directory exist and create it otherwise
	{
		// Helper for root users.
		CAuthHelper rootAuth;

		if ( ! CDspService::instance()->checkExistAndCreateDirectory( getBackupDirectory(), rootAuth, CDspService::permBackupDir ) )
		{
			WRITE_TRACE( DBG_FATAL, "Can't create parallels backup directory '%s'", QSTR2UTF8( getBackupDirectory() ) );
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(PVE::String, getBackupDirectory(), EVT_PARAM_MESSAGE_PARAM_0));

			goto exit;
		}
	}

	CFileHelper::GetDiskAvailableSpace(getBackupDirectory(), &m_nFreeDiskSpace);

	if (m_nFlags & PBT_FULL) {
		/* Create /var/parallels/backups/VM_UUID */
		nRetCode = validateBackupDir(QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid));
		if (nRetCode != PRL_ERR_SUCCESS)
			goto exit;

		/* Create /var/parallels/backups/VM_UUID/BACKUP_UUID with owner = client */
		if (!CFileHelper::CreateDirectoryPath(getBackupRoot(), &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, getBackupRoot(), EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(getBackupRoot()));
			goto exit;
		}
		/* and set next perms : all operations - for owner only, list and restore - for group */
		QFile fdir(QString("%1/.").arg(getBackupRoot()));
		if (!fdir.setPermissions(QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|QFile::ReadGroup))
		{
			nRetCode = PRL_ERR_BACKUP_CANNOT_SET_PERMISSIONS;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, getBackupRoot(), EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot set permissions for directory \"%s\"", QSTR2UTF8(getBackupRoot()));
			goto exit;
		}
	} else {
		/* to check access before */
		if (!CFileHelper::FileCanWrite(getBackupRoot(), &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "User %s have not permissions for create incremental backup for %s",
				QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sBackupUuid));
			goto exit;
		}
	}
	if (!CFileHelper::WriteDirectory(m_sTargetPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, m_sTargetPath, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(m_sTargetPath));
		goto exit;
	}

	/*
	   To lock _full_ backup, but exclusive for full backup and shared for incremental:
	   in last case we must ban base backup removing, but allow list of base backup and
	   restore from base and other incremental backups. To avoid list of backup on creation
	   state, will skip backup without .metadata file (backup create this file on last step)
	   https://jira.sw.ru/browse/PSBM-8198
	 */
	if (m_nFlags & PBT_FULL)
		nRetCode = lockExclusive(m_sBackupUuid);
	else
		nRetCode = lockShared(m_sBackupUuid);
	if (PRL_FAILED(nRetCode))
		goto exit;
	m_bBackupLocked = true;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackupTarget::loadTibFiles()
{
	QFile file(m_sABackupOutFile);
	/* try to load tib files list */
	if (file.open(QIODevice::ReadOnly)) {
		QByteArray array;
		while (1) {
			array = file.readLine();
			if (array.isEmpty())
				break;
			array.replace('\n', '\0');
			m_lstTibFileList.append(QString(array.data()));
		}
		file.close();
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateVmBackupTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;
	bool bConnected;
	QTemporaryFile tmpFile;
	QStringList args;

	/* to lock mutex to avoid ABackup packages processing before backup server start */
	QMutexLocker locker(&m_cABackupMutex);

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	pReply = CDispToDispProtoSerializer::CreateVmBackupCreateFirstReply(
				m_sBackupUuid, m_nBackupNumber, getBackupRoot(), m_nFreeDiskSpace, m_nFlags);
	pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(),
			pReply->GetCommand()->toString(),
			getRequestPackage());

	/* set signal handler before reply - to avoid race (#467221) */
	bConnected = QObject::connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);

	PRL_ASSERT(bConnected);
	// send reply to client
	hJob = m_pDispConnection->sendPackage(pPackage);
	if (CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
		goto exit;
	}

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	/* part one : plain copy of config files */
	nRetCode = exec();
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Backup Plain copy stage failed: %s",
				PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	/* create temporary file for abackup server */
	tmpFile.setFileTemplate(QString("%1/ABackupOutFile.XXXXXX").arg(m_sTargetPath));
	/* open() & closed() to get real file name */
	if (!tmpFile.open()) {
		WRITE_TRACE(DBG_FATAL, "QTemporaryFile::open() error: %s",
				QSTR2UTF8(tmpFile.errorString()));
		nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
		goto exit;
	}
	/* for QTemporaryFile file name exist after open() and before close() only */
	m_sABackupOutFile = tmpFile.fileName();
	tmpFile.close();

	if (m_bLocalMode) {
		locker.unlock();
		if (getInternalFlags() & PVM_CT_BACKUP)
			nRetCode = backupCtPrivate();
		else
			nRetCode = backupHardDiskDevices();
		if (PRL_FAILED(nRetCode))
			goto exit;
	} else {
		/* for QTemporaryFile file name exist after open() and before close() only */
		args.append(m_sABackupOutFile);

		if (PRL_FAILED(nRetCode = m_cABackupServer.start(this, QString(PRL_ABACKUP_SERVER), args, m_nBackupTimeout)))
			goto exit;
		locker.unlock();

		nRetCode = m_cABackupServer.waitForFinished();
	}
	loadTibFiles();

	QObject::disconnect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));
	if (PRL_FAILED(nRetCode))
		goto exit;

	if (PRL_FAILED(nRetCode = saveMetadata()))
		goto exit;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CreateVmBackupTarget::finalizeTask()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle)));

	IOSendJob::Handle hJob;

	m_cABackupServer.kill();

	if (m_bBackupLocked) {
		if (m_nFlags & PBT_FULL)
			unlockExclusive(m_sBackupUuid);
		else
			unlockShared(m_sBackupUuid);
	}
	m_bBackupLocked = false;

	cleanupPrivateArea();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		hJob = m_pDispConnection->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	} else {
		/* remove all tib files of this backup */
		for (int i = 0; i < m_lstTibFileList.size(); ++i)
			QFile::remove(QString("%1/%2").arg(getBackupRoot()).arg(m_lstTibFileList.at(i)));

		/* remove current backup directory */
		if (m_sTargetPath.size()) {
			QString path;
			CFileHelper::ClearAndDeleteDir(m_sTargetPath);

			/* remove BackupUuid directory if it is empty */
			if (isBackupDirEmpty(m_sVmUuid, m_sBackupUuid)) {
				CFileHelper::ClearAndDeleteDir(getBackupRoot());
			}
			/* and remove VmUuid directory if it is empty - check it as root */
			QStringList lstBackupUuid;
			getBaseBackupList(m_sVmUuid, lstBackupUuid);
			if (lstBackupUuid.isEmpty()) {
				path = QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid);
				CFileHelper::ClearAndDeleteDir(path);
			}
		}
		hJob = m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	}
	CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);

	m_lastBase.reset();
}

void Task_CreateVmBackupTarget::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (IS_FILE_COPY_PACKAGE(p->header.type)) {
		PRL_ASSERT(m_pVmCopyTarget);
		nRetCode = m_pVmCopyTarget->handlePackage(p, &bExit);
		if (bExit)
			exit(nRetCode);
	} else if (IS_ABACKUP_PROXY_PACKAGE(p->header.type)) {
		/* Do not process _first_ incoming ABackup packages before backup server start
		   Wait m_nTimeout only because client will disconnect after this timeout */
		if (m_bABackupFirstPacket) {
			if (!m_cABackupMutex.tryLock(m_nTimeout)) {
				WRITE_TRACE(DBG_FATAL, "QMutex::tryLock(%d) error, package with type %d ignored",
						m_nTimeout, p->header.type);
				return;
			}
			m_bABackupFirstPacket = false;
			m_cABackupMutex.unlock();
			if (PRL_FAILED(getLastErrorCode()))
				/* but we got error before backup server start */
				return;
		}

		if (PRL_FAILED(Task_BackupHelper::handleABackupPackage(m_pDispConnection, p, m_nBackupTimeout)))
			m_cABackupServer.kill();
	}
}

quint64 Task_CreateVmBackupTarget::getBackupSize()
{
	quint64 nSize = 0;

	if (m_nFlags & PBT_FULL)
	{
		// Total directory size for full backup.
		CFileHelper::GetDirSize(getBackupRoot(), &nSize);
	}
	else
	{
		// Config size + tlb's for incremental
		CFileHelper::GetDirSize(m_sTargetPath, &nSize);
		for (int i = 0; i < m_lstTibFileList.size(); ++i)
		{
			QFileInfo fi(QString("%1/%2").arg(getBackupRoot()).arg(m_lstTibFileList.at(i)));
			nSize += fi.size();
		}
	}
	return nSize;
}

PRL_RESULT Task_CreateVmBackupTarget::saveMetadata()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	VmItem iVm;

	/* rewrote metadata in Vm directory */
	if (getInternalFlags() & PVM_CT_VZFS_BACKUP)
		iVm.setVmType(PVBT_CT_VZFS);
	else if (getInternalFlags() & PVM_CT_PLOOP_BACKUP)
		iVm.setVmType(PVBT_CT_PLOOP);
	else if (getInternalFlags() & PVM_CT_VZWIN_BACKUP)
		iVm.setVmType(PVBT_CT_VZWIN);
	else
		iVm.setVmType(PVBT_VM);

	iVm.setUuid(m_sVmUuid);
	iVm.setName(m_sVmName);
	nRetCode = iVm.saveToFile(QString("%1/%2/" PRL_BACKUP_METADATA).arg(getBackupDirectory()).arg(m_sVmUuid));

	/* create metadata in backup directory */
	if (m_nFlags & PBT_FULL) {
		BackupItem cBackup;
		cBackup.setUuid(m_sBackupUuid);
		cBackup.setId(m_sBackupUuid);
		cBackup.setHost(m_sSourceHost);
		cBackup.setServerUuid(m_sServerUuid);
		QDateTime x = QDateTime::currentDateTime();
		if (m_lastBase.isValid() && x.secsTo(m_lastBase->getDateTime()) >= 0) {
			x = m_lastBase->getDateTime().addSecs(1);
		}
		cBackup.setDateTime(x);
		cBackup.setCreator(getClient()->getAuthHelper().getUserName());
		cBackup.setSize(getBackupSize());
		cBackup.setType(PRL_BACKUP_FULL_TYPE);
		cBackup.setDescription(m_sDescription);
		cBackup.setTibFileList(m_lstTibFileList);
		cBackup.setOriginalSize(m_nOriginalSize);
		cBackup.setBundlePermissions(m_nBundlePermissions);
		nRetCode = cBackup.saveToFile(QString("%1/" PRL_BACKUP_METADATA).arg(m_sTargetPath));
	} else {
		PartialBackupItem cBackup;
		cBackup.setNumber(m_nBackupNumber);
		cBackup.setId(QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber));
		cBackup.setHost(m_sSourceHost);
		cBackup.setServerUuid(m_sServerUuid);
		cBackup.setDateTime(QDateTime::currentDateTime());
		cBackup.setCreator(getClient()->getAuthHelper().getUserName());
		cBackup.setSize(getBackupSize());
		if (m_nFlags & PBT_INCREMENTAL)
			cBackup.setType(PRL_BACKUP_INCREMENTAL_TYPE);
		else
			cBackup.setType(PRL_BACKUP_DIFFERENTIAL_TYPE);
		cBackup.setDescription(m_sDescription);
		cBackup.setTibFileList(m_lstTibFileList);
		cBackup.setOriginalSize(m_nOriginalSize);
		cBackup.setBundlePermissions(m_nBundlePermissions);
		nRetCode = cBackup.saveToFile(QString("%1/" PRL_BACKUP_METADATA).arg(m_sTargetPath));
	}
	return nRetCode;
}

void Task_CreateVmBackupTarget::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_hConnHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s : backup client was disconnected", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	CancelOperationSupport::cancelOperation(nullClient, getRequestPackage());

	if (m_pVmCopyTarget.isValid())
		m_pVmCopyTarget->cancelOperation();

	if (m_bLocalMode) {
		killABackupClient();
	} else {
		m_cABackupServer.kill();
	}
	// quit event loop
	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

PRL_RESULT Task_CreateVmBackupTarget::backupHardDiskDevices()
{
	::Backup::Activity::Object::Model a;
	PRL_RESULT nRetCode = m_service->find(getClient()->getVmIdent(m_sVmUuid), a);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	QString b = CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()
		->getBackupSourcePreferences()->getSandbox();
	::Backup::Product::Model p(::Backup::Object::Model(m_pVmConfig), m_sVmHomePath);
	p.setStore(getBackupRoot());
	foreach (const ::Backup::Product::component_type& t,
		p.getVmTibs())
	{
		const QFileInfo* f = NULL;
		nRetCode = PRL_ERR_UNEXPECTED;
		foreach (const ::Backup::Product::component_type& c,
				a.getSnapshot().getComponents())
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
			<< t.second.absoluteFilePath() << a.getSnapshot().getUuid()
			<< f->absoluteFilePath() << "--local"
			<< "--disp-mode" << "--tib-out" << m_sABackupOutFile
			<< "--sandbox" << b;
		if (m_nFlags & PBT_UNCOMPRESSED)
			backupArgs.append("--uncompressed");

		backupArgs << "--last-tib" << QString::number(m_nBackupNumber);
		WRITE_TRACE(DBG_FATAL, "Start backup client: %s", QSTR2UTF8(backupArgs.join(" ")));
		nRetCode = startABackupClient(m_sVmName, backupArgs, getLastError(),
				m_sVmUuid, t.first.getDevice().getIndex(), true, m_nBackupTimeout);
		if (PRL_FAILED(nRetCode))
			break;
	}
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackupTarget::backupCtPrivate()
{
#ifdef _CT_
#ifdef _LIN_
	::Backup::Device::Dao(m_pVmConfig).deleteAll();
#endif
	QString sVzCacheDir;
	if (getInternalFlags() & PVM_CT_PLOOP_BACKUP)
		setService(*m_service);
	else
	{
		PRL_RESULT e = handleCtPrivateBeforeBackup(m_sVmUuid, m_sVmHomePath, m_sBackupUuid, sVzCacheDir);
		if (PRL_FAILED(e))
			return e;
	}
	return do_(m_pVmConfig, Backup::Work::Ct::Spec(sVzCacheDir, m_nBackupNumber)
		.setOutFile(m_sABackupOutFile));
#else
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

// compare hdd lists from current VM config and from VM config of last base backup
PRL_RESULT Task_CreateVmBackupTarget::wasHddListChanged(bool *pbWasChanged)
{
	std::map<QString, unsigned long> ls0, ls1;
	QFileInfo fi;

	// load hdd list from current config
	foreach(CVmHardDisk *pHdd, ::Backup::Object::Model(m_pVmConfig).getImages()) {
		ls0[pHdd->getSystemName()] = pHdd->getSize();
	}

	// load hdd list from last full backup config
	QString x;
	CVmConfiguration VmConfig;
	SmartPtr<CVmConfiguration> p;
	QFileInfo b(getBackupRoot(), PRL_BASE_BACKUP_DIRECTORY);
	if (0 == (getInternalFlags() & PVM_CT_PLOOP_BACKUP))
	{
		if (0 == (getInternalFlags() & PVM_CT_VZWIN_BACKUP))
			x = QDir(b.absoluteFilePath()).absoluteFilePath(VMDIR_DEFAULT_VM_CONFIG_FILE);
		else
			x = QDir(b.absoluteFilePath()).absoluteFilePath(VZ_CT_XML_CONFIG_FILE);

		QFile file(x);
		// load config from base backup with relative path
		if (PRL_SUCCEEDED(VmConfig.loadFromFile(&file, false)))
			p = SmartPtr<CVmConfiguration>(&VmConfig, SmartPtrPolicy::DoNotReleasePointee);
	}
	else
	{
		int y = 0;
		x = QDir(b.absoluteFilePath()).absoluteFilePath(VZ_CT_CONFIG_FILE);
		p = CVzHelper::get_env_config_from_file(x, y, VZCTL_LAYOUT_5, true);
	}
	if (!p.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Can not load config file %s", QSTR2UTF8(x));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	foreach(CVmHardDisk *pHdd, ::Backup::Object::Model(p).getImages()) {
		ls1[pHdd->getSystemName()] = pHdd->getSize();
	}
	*pbWasChanged = !(ls1.size() == ls0.size() && std::equal(ls1.begin(), ls1.end(), ls0.begin()));
	return PRL_ERR_SUCCESS;
}

