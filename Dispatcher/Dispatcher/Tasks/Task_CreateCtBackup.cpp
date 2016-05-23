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

/*******************************************************************************

 Ct Backup creation task for client

********************************************************************************/
Task_CreateCtBackupSource::Task_CreateCtBackupSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p,
		::Backup::Activity::Service& service_)
	: Task_BackupHelper(client, p)
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
	m_service = &service_;
}

Task_CreateCtBackupSource::~Task_CreateCtBackupSource()
{
}

PRL_RESULT Task_CreateCtBackupSource::prepareTask()
{
	// check params in existing VM
	CDspService* s = CDspService::instance();
	SmartPtr<CDspClient> client = getClient();
	VIRTUAL_MACHINE_STATE state;
	PRL_RESULT nRetCode;

	{
		CDspLockedPointer<CVmDirectory> d = s->getVmDirManager().getVzDirectory();
		if (!d.isValid())
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			goto exit;
		}
		m_sVmDirUuid = d->getUuid();
	}

	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	m_pVmConfig = s->getVzHelper()->getCtConfig(client, m_sVmUuid);
	if (!m_pVmConfig.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}
	if (VZCTL_LAYOUT_4 < m_pVmConfig->getCtSettings()->getLayout())
		setInternalFlags((getInternalFlags() & ~PVM_CT_BACKUP) | PVM_CT_PLOOP_BACKUP);
	else
	{
		WRITE_TRACE(DBG_FATAL, "Unsupported Container's layout (%d)",
				m_pVmConfig->getCtSettings()->getLayout());
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
			b(::Backup::Activity::Builder(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), *this),
				::Backup::Snapshot::Ct::Flavor::Mount(m_sVmUuid, CVzOperationHelper()),
				s->getVzHelper());
		if (PRL_FAILED(nRetCode = d(b)))
			goto exit;
	}
	m_sVmName = m_pVmConfig->getVmIdentification()->getVmName();
	m_sVmHomePath = m_pVmConfig->getVmIdentification()->getHomePath();

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
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
	::Backup::Work::object_type o =
		::Backup::Work::object_type(::Backup::Work::Ct(*this));
	doBackup(m_sVmHomePath, o);
	return waitForTargetFinished();
}

