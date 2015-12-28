///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmMigrateHelper.cpp
///
/// VM migration helper class implementation
///
/// @author sandro
/// @owner sergeym
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

#include "CDspVmMigrateHelper.h"
#include "CDspService.h"
#include "CDspVm.h"
#include "CDspClientManager.h"
#include "Tasks/Task_MigrateVmTarget.h"
#ifdef _WIN_
# include "Tasks/Task_MigrateCt_win.h"
#else
# include "Tasks/Task_MigrateCtTarget.h"
#endif
#include "Tasks/Mixin_CreateVmSupport.h"

#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Interfaces/ParallelsNamespace.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"

SmartPtr<CDspTaskHelper> CDspVmMigrateHelper::findTask(QString sVmUuid)
{
	foreach(SmartPtr<CDspTaskHelper> pTask, CDspService::instance()->getTaskManager().getAllTasks())
	{
		switch (pTask->getRequestPackage()->header.type) {
		case VmMigrateCheckPreconditionsCmd:
		case VmMigrateStartCmd:
		case PVE::DspCmdDirVmMigrate:
			if (pTask->getVmUuid() == sVmUuid)
				return pTask;
			break;
		}
	}
	return SmartPtr<CDspTaskHelper>();
}

void CDspVmMigrateHelper::checkPreconditions(
	SmartPtr<CDspDispConnection> pDispConnection,
	const SmartPtr<IOPackage> &p
)
{
	IOSendJob::Handle hJob;

	PRL_ASSERT( pDispConnection.getImpl() );
	PRL_ASSERT( p.getImpl() );

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( ! pCmd->IsValid() )
	{
		pDispConnection->sendSimpleResponse( p, PRL_ERR_FAILURE );
		WRITE_TRACE(DBG_FATAL, "Wrong check preconditions package was received: [%s]",\
			p->buffers[0].getImpl());
		return;
	}

	CVmMigrateCheckPreconditionsCommand * pCheckCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);

	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration(pCheckCmd->GetVmConfig()));
	if (PRL_FAILED(pVmConfig->m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Wrong VM configuration was received: [%s]",\
			QSTR2UTF8(pCheckCmd->GetVmConfig()));
		hJob = pDispConnection->sendSimpleResponse( p, PRL_ERR_PARSE_VM_CONFIG );
		CDspService::instance()->getIOServer().waitForResponse( hJob );
		return;
	}
	QString sVmUuid = pVmConfig->getVmIdentification()->getVmUuid();

	/* does Task_MigrateVmTarget task for this Vm alrready exists? */
	if (findTask(sVmUuid).isValid())
	{
		WRITE_TRACE(DBG_FATAL, "migrate task for Vm %s already exists", QSTR2UTF8(sVmUuid));
		CVmEvent evt;
		evt.setEventCode(PRL_ERR_VM_MIGRATE_VM_ALREADY_MIGRATE_ON_TARGET);
		evt.addEventParameter(new CVmEventParameter(
				      PVE::String, sVmUuid, EVT_PARAM_MESSAGE_PARAM_0));
		pDispConnection->sendResponseError(evt, p);
		return;
	}

	CDspTaskHelper *task_helper;
	if (pCheckCmd->GetReservedFlags() & PVM_CT_MIGRATE)
#ifdef _CT_
		task_helper = new Task_MigrateCtTarget((QObject *)this, pDispConnection, pCmd, p);
#else
	{
		WRITE_TRACE(DBG_FATAL, "Linux containers are not implemented");
		pDispConnection->sendSimpleResponse( p, PRL_ERR_UNIMPLEMENTED );
		return;
	}
#endif
	else
		task_helper = new Task_MigrateVmTarget((QObject *)this, pDispConnection, pCmd, p);

	CDspService::instance()->getTaskManager().schedule(task_helper);
}

void CDspVmMigrateHelper::startMigration(SmartPtr<CDspDispConnection> pDispConnection, const SmartPtr<IOPackage> &p)
{
	PRL_ASSERT( pDispConnection.getImpl() );
	PRL_ASSERT( p.getImpl() );

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( ! pCmd->IsValid() )
	{
		pDispConnection->sendSimpleResponse( p, PRL_ERR_FAILURE );
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",\
			p->buffers[0].getImpl());
		return;
	}

	CVmMigrateStartCommand *pStartCommand =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);

	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration(pStartCommand->GetVmConfig()));
	if (PRL_FAILED(pVmConfig->m_uiRcInit))
	{
		pDispConnection->sendSimpleResponse( p, PRL_ERR_PARSE_VM_CONFIG );
		WRITE_TRACE(DBG_FATAL, "Wrong VM configuration was received: [%s]",\
			QSTR2UTF8(pStartCommand->GetVmConfig()));
		return;
	}

	emit onPackageReceived(pDispConnection, pVmConfig->getVmIdentification()->getVmUuid(), p);
	return;
}

void CDspVmMigrateHelper::cancelMigration(
		const SmartPtr<CDspClient> &pSession,
		const SmartPtr<IOPackage> &p,
		const QString& uuid_)
{
	PRL_ASSERT( pSession.getImpl() );
	PRL_ASSERT( p.getImpl() );
	PRL_ASSERT( !uuid_.isEmpty() );

	SmartPtr<CDspTaskHelper> pTask = findTask(uuid_);
	if (!pTask.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Migration task for Vm %s does not found", QSTR2UTF8(uuid_));
		return;
	}
	pTask->cancelOperation(pSession, p);
	return;
}

void CDspVmMigrateHelper::cancelMigration(
		const SmartPtr<CDspClient> &pSession,
		const SmartPtr<IOPackage> &p,
		const SmartPtr<CDspVm> &pVm)
{
	PRL_ASSERT( pVm.getImpl() );
	cancelMigration(pSession, p, pVm->getVmUuid());
}

