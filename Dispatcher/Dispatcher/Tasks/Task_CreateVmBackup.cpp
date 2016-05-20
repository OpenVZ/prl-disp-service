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

	::Backup::Work::object_type o =
		::Backup::Work::object_type(::Backup::Work::Vm(*this));
	return doBackup(m_sSourcePath, o);
}

/* Finalize task */
void Task_CreateVmBackupSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	Task_BackupHelper::finalizeTask();
}

