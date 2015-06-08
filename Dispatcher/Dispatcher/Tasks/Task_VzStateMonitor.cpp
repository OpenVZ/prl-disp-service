////////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzStateMonitor.cpp
///
/// @author igor
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
////////////////////////////////////////////////////////////////////////////////

#include "CDspService.h"
#include "CDspVmDirManager.h"
#include "Tasks/Task_VzStateMonitor.h"
#include "Tasks/Task_VzManager.h"
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include "CDspVzHelper.h"

#include <vzctl/libvzctl.h>

Task_VzStateMonitor::Task_VzStateMonitor(const SmartPtr<CDspClient> &user,
		const SmartPtr<IOPackage> &p) :
	CDspTaskHelper(user, p)
{}

Task_VzStateMonitor::~Task_VzStateMonitor()
{
}

void Task_VzStateMonitor::cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &)
{
	m_monitor.stop();
}

void Task_VzStateMonitor::stateEventHandler(void *obj, const QString &uuid, int state)
{
	Task_VzStateMonitor *task = reinterpret_cast<Task_VzStateMonitor *> (obj);

	task->sendState(uuid, state);
}

void Task_VzStateMonitor::processRegisterEvt(const QString &ctid)
{
	PRL_RESULT res;

	SmartPtr<CVmConfiguration> pConfig = CDspService::instance()->getVzHelper()->
			getVzlibHelper().get_env_config_by_ctid(ctid);

	if (!pConfig)
		return;
	QString vm_uuid = pConfig->getVmIdentification()->getVmUuid();
	QString vm_name = pConfig->getVmIdentification()->getVmName();
	QString vm_home = pConfig->getVmIdentification()->getHomePath();

	WRITE_TRACE(DBG_INFO, "Register Ct uuid=%s name=%s home=%s",
			QSTR2UTF8(vm_uuid), QSTR2UTF8(vm_name), QSTR2UTF8(vm_home));

	CVmDirectory::TemporaryCatalogueItem vmInfo( vm_uuid, vm_home, vm_name);

	res = CDspService::instance()->getVmDirManager()
		.checkAndLockNotExistsExclusiveVmParameters(QStringList(), &vmInfo);
	if (PRL_FAILED(res))
		return;

	CDspService::instance()->getVzHelper()->insertVmDirectoryItem(pConfig);

	// delete temporary registration
	CDspService::instance()->getVmDirManager()
		.unlockExclusiveVmParameters(&vmInfo);

}

void Task_VzStateMonitor::processUnregisterEvt(const QString &sUuid)
{
	PRL_RESULT res;

	res = CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(m_sVzDirUuid, sUuid);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, ">>> Can't delete Ct %s from VmDirectory by error %#x, %s",
				QSTR2UTF8(sUuid), res, PRL_RESULT_TO_STRING( res) );
}

void Task_VzStateMonitor::processConfigChangedEvt(const QString &sUuid)
{
	// Handle Name change
	CDspLockedPointer< CVmDirectoryItem >
		pVmDirItem = CDspService::instance()->getVmDirManager()
		.getVmDirItemByUuid(CDspVmDirManager::getVzDirectoryUuid(), sUuid );
	if (!pVmDirItem)
		return;

	SmartPtr<CVmConfiguration> pConfig = CDspService::instance()->getVzHelper()->
		getCtConfig(CDspClient::makeServiceUser(), sUuid);
	if (!pConfig)
		return;

	QString sNewName = pConfig->getVmIdentification()->getVmName();
	if (pVmDirItem->getVmName() != sNewName && !sNewName.isEmpty()) {
		pVmDirItem->setVmName(sNewName);
		PRL_RESULT ret = CDspService::instance()->getVmDirManager().
			updateVmDirItem(pVmDirItem);
		if (PRL_FAILED(ret) )
			WRITE_TRACE(DBG_FATAL, "Can't update Container %s VmCatalogue by error: %s",
					QSTR2UTF8(sUuid), PRL_RESULT_TO_STRING(ret));
	}
}

void Task_VzStateMonitor::processNetConfigChangedEvt(const QString &sUuid)
{
	/* The VMS_UNKNOWN state will used to apply network configuration */
	processChangeCtState(sUuid, VMS_UNKNOWN);
}

void Task_VzStateMonitor::processChangeCtState(QString uuid, VIRTUAL_MACHINE_STATE vm_state)
{
	SmartPtr<CDspClient> pFakeSession(new CDspClient(Uuid::createUuid(), "fake-user" ));
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdCtlDispatherFakeCommand );
	CDspService::instance()->getTaskManager().schedule(new Task_VzManager(pFakeSession, p, uuid, vm_state));
}

void Task_VzStateMonitor::sendState(const QString &ctid, int state)
{
	WRITE_TRACE( DBG_INFO, "vzevent: state=%d, envid=%s",
			state, QSTR2UTF8(ctid));

#ifdef _LIN_
	// Node events
	if (state == VZCTL_NET_SHAPING_CONFIG_CHANGED) {
		CDspService::instance()->getVmDirHelper().restartNetworkShaping(
						true, getClient(), getRequestPackage());
		return;
	}
#endif

	// Container events
	if (state == VZCTL_ENV_CREATED ||
			state == VZCTL_ENV_REGISTERED)
	{
		processRegisterEvt(ctid);
	}

	QString uuid = CVzHelper::get_uuid_by_ctid(ctid);
	if (uuid.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Unable to find Container uuid by id=%s",
				QSTR2UTF8(ctid));
		return;
	}

	CVmEvent event( PET_DSP_EVT_VM_STATE_CHANGED, uuid, PIE_DISPATCHER );
	if (state == VZCTL_ENV_CREATED ||
			state == VZCTL_ENV_REGISTERED)
	{
		if (state == VZCTL_ENV_REGISTERED) {
			VIRTUAL_MACHINE_STATE nState;
			PRL_RESULT res = CDspService::instance()->getVzHelper()->getVzlibHelper().get_env_status(uuid, nState);
			if (PRL_FAILED(res)) {
				WRITE_TRACE(DBG_FATAL, "Unable to get state of the container with id=%s", QSTR2UTF8(uuid));
				return;
			}

			if (nState == VMS_RUNNING)
				processChangeCtState(uuid, VMS_RUNNING);
		}

		event.setEventType(PET_DSP_EVT_VM_ADDED);
		event.addEventParameter( new CVmEventParameter (PVE::Integer
				, QString("%1").arg((int)PVT_CT)
				, EVT_PARAM_VMINFO_VM_TYPE ) );

	}
	else if (state == VZCTL_ENV_UNREGISTERED ||
			state == VZCTL_ENV_DELETED)
	{
		processUnregisterEvt(uuid);
		event.setEventType(PET_DSP_EVT_VM_DELETED);
	}
#ifdef _LIN_
	else if (state == VZCTL_ENV_CONFIG_CHANGED)
	{
		processConfigChangedEvt(uuid);
		event.setEventType(PET_DSP_EVT_VM_CONFIG_CHANGED);
	}
	else if (state == VZCTL_ENV_NET_CONFIG_CHANGED)
	{
		processNetConfigChangedEvt(uuid);
		event.setEventType(PET_DSP_EVT_VM_CONFIG_CHANGED);
	}
#endif
	else if (state == VZCTL_ENV_STARTED ||
			state == VZCTL_ENV_STOPPED ||
			state == VZCTL_ENV_SUSPENDED)
	{
		VIRTUAL_MACHINE_STATE vm_state;
		switch (state) {
		case VZCTL_ENV_STARTED:
			vm_state = VMS_RUNNING;
			break;
		case VZCTL_ENV_STOPPED:
			vm_state = VMS_STOPPED;
			break;
		case VZCTL_ENV_SUSPENDED:
			vm_state = VMS_SUSPENDED;
			break;
		default:
			return;
		}
		processChangeCtState(uuid, vm_state);

		event.addEventParameter( new CVmEventParameter (PVE::Integer
				, QString("%1").arg((int)vm_state)
				, EVT_PARAM_VMINFO_VM_STATE ) );
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "unknown Ct state %d", state);
		return;
	}
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );

	CDspService::instance()->getClientManager()
		.sendPackageToAllClients(p);
}

PRL_RESULT Task_VzStateMonitor::startMonitor()
{
	m_monitor.start(Task_VzStateMonitor::stateEventHandler, this);
	WRITE_TRACE(DBG_FATAL, "Stopping Vz event monitor");
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzStateMonitor::prepareTask()
{
	m_sVzDirUuid = CDspVmDirManager::getVzDirectoryUuid();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzStateMonitor::run_body()
{
	return startMonitor();
}

void Task_VzStateMonitor::finalizeTask()
{
}

