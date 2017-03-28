//////////////////////////////////////////////////////////////////////////////
///
/// @file Task_UpdateVm.cpp
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
//////////////////////////////////////////////////////////////////////////////

#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlsdk/PrlErrorsValues.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Messaging/CVmEventParameterList.h>

#include "CDspService.h"
#include "CDspLibvirt.h"
#include "Task_UpdateVm.h"

///////////////////////////////////////////////////////////////////////////////
// class Task_UpdateVm

Task_UpdateVm::Task_UpdateVm(const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p) :	CDspTaskHelper(client, p)
{
	CProtoVmMigrateCommand *pCmd = CProtoSerializer::
				CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_sVmUuid = pCmd->GetVmUuid();
}

PRL_RESULT Task_UpdateVm::prepareTask()
{
	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - VM migrate rejected!");
		return CDspTaskFailure (*this)(PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
	}

	VIRTUAL_MACHINE_STATE s = CDspVm::getVmState(m_sVmUuid,
			getClient()->getVmDirectoryUuid());
	if (s != VMS_RUNNING && s != VMS_PAUSED)
	{
		CDspTaskFailure f(*this);
		f(m_sVmUuid, PRL_VM_STATE_TO_STRING(s));
		return f(PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED);
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_UpdateVm::run_body()
{
	if (PRL_FAILED(getLastErrorCode()))
		return getLastErrorCode();

	PRL_RESULT e = CDspService::instance()->getVmDirHelper().
		registerExclusiveVmOperation(m_sVmUuid,
				getClient()->getVmDirectoryUuid(),
				PVE::DspCmdDirVmMigrate, getClient());
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "registerExclusiveVmOperation failed. Reason: %#x (%s)",
				e, PRL_RESULT_TO_STRING(e));
		
		return CDspTaskFailure(*this)(e);
	}

	Libvirt::Result r = Libvirt::Kit.vms().at(m_sVmUuid).getMaintenance().updateQemu();
	if (r.isFailed())
		getLastError()->fromString(r.error().convertToEvent().toString());

	CDspService::instance()->getVmDirHelper().
		unregisterExclusiveVmOperation(m_sVmUuid,
				getClient()->getVmDirectoryUuid(),
				PVE::DspCmdDirVmMigrate, getClient());

	return getLastErrorCode();
}

void Task_UpdateVm::finalizeTask()
{
	if (PRL_FAILED(getLastErrorCode()))
	{
		getClient()->sendResponseError(getLastError(),
				getRequestPackage());
	}
	else
	{
		getClient()->sendSimpleResponse(getRequestPackage(),
				PRL_ERR_SUCCESS);
	}
}
