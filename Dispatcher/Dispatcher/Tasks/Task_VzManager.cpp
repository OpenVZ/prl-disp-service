///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzManager.cpp
///
/// @author igor
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
/////////////////////////////////////////////////////////////////////////////////

#include "CDspVmBrand.h"
#include "CDspClientManager.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include "CDspProblemReportHelper.h"
#include "Dispatcher/Dispatcher/Tasks/Task_EditVm.h"
#include "Dispatcher/Dispatcher/Tasks/Task_CloneVm.h"
#include "Task_VzManager.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Dispatcher/Dispatcher/Tasks/Task_UpdateCommonPrefs.h"
#include "CDspVmStateSender.h"
#include "CDspTemplateFacade.h"
#include "CDspInstrument.h"

#ifdef _LIN_
#include "vzctl/libvzctl.h"
#include "CDspBackupDevice.h"
#endif


namespace Encryption
{

QString getFilter()
{
	return "Hardware\\.Hdd\\[\\d+\\]\\.Encryption";
}

struct Match
{
	Match(const CVmHardDisk *disk_) : m_old(disk_)
	{
	}

	bool operator()(const CVmHardDisk *new_)
	{
		if (new_->getUuid() != m_old->getUuid())
			return false;

		QString o = m_old->getEncryption() ? m_old->getEncryption()->getKeyId() : "";
		QString n = new_->getEncryption() ? new_->getEncryption()->getKeyId() : "";
		return n != o;
	}

private:
	const CVmHardDisk *m_old;
};

} // namespace Encryption

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Clone

struct Clone
{
	Clone(Task_VzManager *task, SmartPtr<CVmConfiguration> &config,
		SmartPtr<CVmConfiguration> &newConfig) :
		m_task(task), m_config(config), m_newConfig(newConfig)
	{}

	PRL_RESULT operator()(const QString &)
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(
						m_task->getRequestPackage());
		if (!cmd->IsValid())
			return PRL_ERR_UNRECOGNIZED_REQUEST;

		CProtoVmCloneCommand *pCmd = CProtoSerializer::
				CastToProtoCommand<CProtoVmCloneCommand>(cmd);

		return m_task->get_op_helper()->clone_env(m_config,
				pCmd->GetVmHomePath(),
				pCmd->GetVmName(),
				1, m_newConfig);
	}

	SmartPtr<CVmConfiguration> getConfig() const
	{
		return m_newConfig;
	}

private:
	Task_VzManager *m_task;
	SmartPtr<CVmConfiguration> m_config;
	SmartPtr<CVmConfiguration> m_newConfig;
};

///////////////////////////////////////////////////////////////////////////////
// struct Delete

struct Delete
{
	Delete(const QString &home, Template::Storage::Catalog *catalog) :
		m_home(home), m_catalog(catalog)
	{}

	PRL_RESULT do_()
	{
		PRL_RESULT e = m_catalog->unlink(QFileInfo(m_home).fileName());
		if (PRL_FAILED(e))
			return e;

		return m_catalog->commit();
	}

private:
	QString m_home;
	Template::Storage::Catalog *m_catalog;
};

namespace Move
{
///////////////////////////////////////////////////////////////////////////////
// struct Import

struct Import
{
	Import(Task_VzManager *task, SmartPtr<CVmConfiguration> &config,
			Template::Storage::Catalog *catalog) :
		m_task(task), m_config(config), m_catalog(catalog)
	{}
	
	PRL_RESULT execute()
	{
		QFileInfo s(m_config->getVmIdentification()->getHomePath());
		PRL_RESULT e = m_catalog->import(s.fileName(),
				boost::bind(&Import::do_, this, _1));
		if (PRL_FAILED(e))
			return e;

		return m_catalog->commit();
	}

	PRL_RESULT do_(const QFileInfo& target_)
	{
		return m_task->get_op_helper()->move_env(
			m_config->getVmIdentification()->getVmUuid(),
			target_.filePath());
	}

private:
	Task_VzManager *m_task;
	SmartPtr<CVmConfiguration> m_config;
	Template::Storage::Catalog *m_catalog;
};

} // namespace Move
} // namespace Command

Task_VzManager::Task_VzManager(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p,
		Vm::Directory::Ephemeral* ephemeral) :
	CDspTaskHelper(pClient, p),
	m_pResponseCmd(CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS)),
	m_pVzOpHelper(new CVzOperationHelper(Task_VzManager::sendEvt, this)),
	m_ephemeral(ephemeral)

{
	m_bProcessState = false;
	m_nState = VMS_UNKNOWN;
}

Task_VzManager::Task_VzManager(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p,
		const QString &sUuid, VIRTUAL_MACHINE_STATE nState) :
	CDspTaskHelper(pClient, p),
	m_pResponseCmd(CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS)),
	m_pVzOpHelper(new CVzOperationHelper())
{
	m_bProcessState = true;
	m_nState = nState;
	m_sVmUuid = sUuid;
}

void Task_VzManager::sendProgressEvt(PRL_EVENT_TYPE type, const QString &sUuid,
		const QString &sStage, int progress)
{
	CVmEvent event(type, sUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
				PVE::UnsignedInt,
				QString::number(progress),
				EVT_PARAM_PROGRESS_CHANGED));

	event.addEventParameter(new CVmEventParameter(
				PVE::String,
				sStage,
				EVT_PARAM_PROGRESS_STAGE));

	SmartPtr<IOPackage> p =	DispatcherPackage::createInstance(PVE::DspVmEvent, event,
			getRequestPackage());

	getClient()->sendPackage(p);
}

void Task_VzManager::sendEvt(void *obj, PRL_EVENT_TYPE type, const QString &sUuid,
		const QString &sStage, int data)
{
	Task_VzManager *task = reinterpret_cast<Task_VzManager *> (obj);

	task->sendProgressEvt(type, sUuid, sStage, data);
}

void Task_VzManager::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pUserSession, p);
	get_op_helper()->cancel_operation();
}

CProtoCommandDspWsResponse *Task_VzManager::getResponseCmd()
{
	return CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(m_pResponseCmd);
}

void Task_VzManager::sendEvent(PRL_EVENT_TYPE type, const QString &sUuid)
{
	/**
	 * Notify all user: vm directory changed
	 */
	CVmEvent event(type, sUuid, PIE_DISPATCHER);
	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());

	CDspService::instance()->getClientManager().sendPackageToAllClients(p);
}

PRL_RESULT Task_VzManager::check_env_state(PRL_UINT32 nCmd, const QString &sUuid)
{
	return getVzHelper()->check_env_state(nCmd, sUuid, getLastError());
}

PRL_RESULT Task_VzManager::checkAndLockRegisterParameters(
			CVmDirectory::TemporaryCatalogueItem *pVmInfo)
{
	PRL_RESULT lockResult = CDspService::instance()->getVmDirManager()
			.checkAndLockNotExistsExclusiveVmParameters(QStringList(), pVmInfo);

	if (PRL_FAILED(lockResult))
	{
		CDspTaskFailure f(*this);
		f.setCode(lockResult);
		switch (lockResult)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
			return f(pVmInfo->vmUuid);

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			return f(pVmInfo->vmName, pVmInfo->vmXmlPath);

		case PRL_ERR_VM_ALREADY_REGISTERED:
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
			return f(pVmInfo->vmName);

		case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default
		default:
			return f.setToken(pVmInfo->vmUuid).setToken(pVmInfo->vmXmlPath)
				.setToken(pVmInfo->vmName)();
		}
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::create_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmCreateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCreateCommand>(cmd);
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	SmartPtr<CVmConfiguration> pConfig(new CVmConfiguration);
	pConfig->fromString(pCmd->GetVmConfig());

	QString sPath = pCmd->GetVmHomePath();
	if (!sPath.isEmpty() && !QDir::isAbsolutePath(sPath)) {
		WRITE_TRACE(DBG_FATAL, "Invalid path '%s'", QSTR2UTF8(sPath));
		return PRL_ERR_VMDIR_PATH_IS_NOT_ABSOLUTE;
	}

	QString vm_uuid = pConfig->getVmIdentification()->getVmUuid();
	QString vm_name = pConfig->getVmIdentification()->getVmName();
	QString vm_home;

	CVmDirectory::TemporaryCatalogueItem vmInfo(vm_uuid, vm_home, vm_name);
	// Lock
	PRL_RESULT res = checkAndLockRegisterParameters(&vmInfo);
	if (PRL_FAILED(res))
		return res;

	CDspService::instance()->getVmDirHelper().resetAdvancedParamsFromVmConfig(pConfig);
	res = get_op_helper()->create_env(sPath, pConfig, pCmd->GetCommandFlags());
	if (PRL_SUCCEEDED(res)) {
		sPath = pConfig->getVmIdentification()->getHomePath();
		res = ::Vm::Private::Brand(sPath, getClient()).stamp(CDspTaskFailure(*this));
		if (PRL_SUCCEEDED(res))
			res = getVzHelper()->insertVmDirectoryItem(pConfig);
		if (PRL_FAILED(res))
			get_op_helper()->delete_env(
					pConfig->getVmIdentification()->getVmUuid());
	}
	// Unlock temporary registration
	CDspService::instance()->getVmDirManager()
		.unlockExclusiveVmParameters(&vmInfo);

	if (PRL_SUCCEEDED(res)) {
		getResponseCmd()->SetVmConfig(pConfig->toString());
		sendEvent(PET_DSP_EVT_VM_CREATED, vm_uuid);
	}

	return res;
}

PRL_RESULT Task_VzManager::setupFirewall(SmartPtr<CVmConfiguration> &pConfig)
{
	CFirewallHelper fw(pConfig);

	PRL_RESULT ret = fw.Execute();
	if (PRL_FAILED(ret))
	{
		if (ret == PRL_ERR_FIREWALL_TOOL_EXECUTED_WITH_ERROR)
		{
			getLastError()->setEventType(PET_DSP_EVT_ERROR_MESSAGE);
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
						fw.GetErrorMessage(),
						EVT_PARAM_DETAIL_DESCRIPTION)
					);
		}
		return ret;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::start_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sUuid = pCmd->GetVmUuid();

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	if (pConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_CANT_TO_START_VM_TEMPLATE;

	VIRTUAL_MACHINE_STATE nState;
	PRL_RESULT res = CVzHelper::get_env_status(sUuid, nState);
	if (PRL_FAILED(res))
		return res;
	else if (nState == VMS_RUNNING)
	{
		return CDspTaskFailure(*this)(PRL_ERR_DISP_VM_IS_NOT_STOPPED,
			pConfig->getVmIdentification()->getVmName());
	}
	else if (nState == VMS_SUSPENDED)
	{
		res = resume_env();
		if (PRL_SUCCEEDED(res))
			return PRL_ERR_SUCCESS;
	}

	Backup::Device::Service(pConfig).setContext(*this).enable();

	res = get_op_helper()->start_env(
		sUuid, CDspService::instance()->getHaClusterHelper()->getStartCommandFlags(pCmd));
	if (PRL_FAILED(res))
		return res;

	if (nState == VMS_PAUSED)
	{
		CDspService::instance()->getVmStateSender()->
			onVmStateChanged(VMS_PAUSED, VMS_RUNNING, sUuid,
					m_sVzDirUuid, false);
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::pause_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sUuid = pCmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	PRL_RESULT res = check_env_state(PVE::DspCmdVmPause, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->pause_env(sUuid);
	if (PRL_FAILED(res))
		return res;

	CDspService::instance()->getVmStateSender()->
		onVmStateChanged(VMS_RUNNING, VMS_PAUSED, sUuid, m_sVzDirUuid, false);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::reset_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	QString sUuid = pCmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	PRL_RESULT res = check_env_state(PVE::DspCmdVmStop, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->stop_env(sUuid, PSM_KILL);
	if (PRL_FAILED(res))
		return res;

	return get_op_helper()->start_env(sUuid, 0);
}

PRL_RESULT Task_VzManager::restart_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sUuid = pCmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	if (pConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_CANT_TO_START_VM_TEMPLATE;

	PRL_RESULT res = get_op_helper()->restart_env(sUuid);
	if (PRL_FAILED(res))
		return res;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::stop_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_FAILURE;

	CProtoVmCommandStop *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCommandStop>(cmd);
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sUuid = cmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	PRL_RESULT res = check_env_state(PVE::DspCmdVmStop, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->stop_env(sUuid, pCmd->GetStopMode());

	Backup::Device::Service(pConfig).disable();

	return res;
}

PRL_RESULT Task_VzManager::mount_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_FAILURE;

	PRL_RESULT res;
	QString sUuid = cmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	res = check_env_state(PVE::DspCmdVmMount, sUuid);
	if (PRL_FAILED(res))
		return res;

	bool infoMode = (cmd->GetCommandFlags() & PMVD_INFO);

	if (infoMode) {
		Prl::Expected<QString, PRL_RESULT> info =
			get_op_helper()->get_env_mount_info(pConfig);
		if (info.isFailed())
			return info.error();
		getResponseCmd()->AddStandardParam(info.value());
	} else {
		res = get_op_helper()->mount_env(sUuid);
	}
	return res;
}

PRL_RESULT Task_VzManager::umount_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_FAILURE;

	PRL_RESULT res;
	QString sUuid = cmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	res = check_env_state(PVE::DspCmdVmUmount, sUuid);
	if (PRL_FAILED(res))
		return res;

	return get_op_helper()->umount_env(sUuid);
}

PRL_RESULT Task_VzManager::suspend_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	PRL_RESULT res;
	QString sUuid = pCmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	res = check_env_state(PVE::DspCmdVmSuspend, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->suspend_env(sUuid);
	if (PRL_SUCCEEDED(res))
	{
		CDspService::instance()->getVmStateSender()->
			onVmStateChanged(VMS_RUNNING, VMS_SUSPENDED, sUuid,
					m_sVzDirUuid, false);
	}

	Backup::Device::Service(pConfig).disable();

	return res;
}

PRL_RESULT Task_VzManager::resume_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	PRL_RESULT res;
	QString sUuid = pCmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	res = check_env_state(PVE::DspCmdVmResume, sUuid);
	if (PRL_FAILED(res))
		return res;

	Backup::Device::Service(pConfig).setContext(*this).enable();

	return get_op_helper()->resume_env(sUuid, pCmd->GetCommandFlags());
}

PRL_RESULT Task_VzManager::delete_env()
{
	PRL_RESULT res;

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmDeleteCommand * pDeleteCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeleteCommand>(pCmd);
	QString sUuid = pDeleteCmd->GetVmUuid();

	QString dirUuid = CDspVmDirHelper::getVmDirUuidByVmUuid(sUuid, getClient());

	if (dirUuid.isEmpty())
		return PRL_ERR_VM_UUID_NOT_FOUND;

	SmartPtr<CVmConfiguration> pConfig = CDspService::instance()->
		getVmDirHelper().getVmConfigByUuid(getClient(), sUuid, res, NULL);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	QString vm_home = pConfig->getVmIdentification()->getHomePath();
	Template::Storage::Dao::pointer_type c;
	Template::Storage::Dao d(getClient()->getAuthHelper());
	res = d.findForEntry(vm_home, c);
	if (c.isNull()) {
		VIRTUAL_MACHINE_STATE nState;
		res = getVzHelper()->getVzlibHelper().get_env_status(sUuid, nState);
		if (PRL_FAILED(res))
			return res;

		if (nState == VMS_MOUNTED) {
			res = get_op_helper()->umount_env(sUuid);
			if (PRL_FAILED(res))
				return res;
		}

		res = check_env_state(PVE::DspCmdDirVmDelete, sUuid);
		if (PRL_FAILED(res))
			return res;
	}

	QString vm_uuid = pConfig->getVmIdentification()->getVmUuid();
	QString vm_name = pConfig->getVmIdentification()->getVmName();

	CVmDirectory::TemporaryCatalogueItem vmInfo(vm_uuid, vm_home, vm_name);

	res = CDspService::instance()->getVmDirManager()
			.lockExclusiveVmParameters(dirUuid, &vmInfo);
	if (PRL_FAILED(res))
		return res;

	if (c.isNull()) {
		Backup::Device::Service(pConfig).teardown();
		res = get_op_helper()->delete_env(sUuid);
	} else {
		Command::Delete d(vm_home, c.data());
		res = d.do_();
	}

	if (PRL_SUCCEEDED(res)) {
		// FIXME: rollback operation
		res = CDspService::instance()->getVmDirHelper()
			.deleteVmDirectoryItem(dirUuid, vm_uuid);
		if (PRL_FAILED(res) && res != PRL_ERR_ENTRY_DOES_NOT_EXIST) {
			WRITE_TRACE(DBG_FATAL, "Can't delete Container %s from VmDirectory by error: %s",
					QSTR2UTF8(vm_uuid), PRL_RESULT_TO_STRING(res));
		}

		sendEvent(PET_DSP_EVT_VM_DELETED, sUuid);
	}

	// delete temporary registration
	CDspService::instance()->getVmDirManager()
		.unlockExclusiveVmParameters(dirUuid, &vmInfo);

	return res;
}

PRL_RESULT Task_VzManager::changeVNCServerState(SmartPtr<CVmConfiguration> pOldConfig,
						SmartPtr<CVmConfiguration> pNewConfig, QString sUuid)
{
	CVmSettings* oldSettings = pOldConfig->getVmSettings();
	CVmSettings* newSettings = pNewConfig->getVmSettings();
	CVmRemoteDisplay* oldD = oldSettings->getVmRemoteDisplay();
	CVmRemoteDisplay* newD = newSettings->getVmRemoteDisplay();
	PRL_RESULT res = PRL_ERR_SUCCESS;
	tribool_type run;

	// Start VNC
	if (oldD->getMode() != newD->getMode()) {
		run = CVzHelper::is_env_running(sUuid);
		if (boost::logic::indeterminate(run))
			return PRL_ERR_OPERATION_FAILED;
		if (!run)
			return PRL_ERR_SUCCESS;

		if (newD->getMode() == PRD_DISABLED) {
			res = stop_vnc_server(sUuid, false);
			if (res == PRL_ERR_VNC_SERVER_NOT_STARTED)
				res = PRL_ERR_SUCCESS;
		} else {
			res = start_vnc_server(sUuid, false);
		}
	}
	// VNC config has been changed
	else if (newD->getMode() != PRD_DISABLED &&
		  (oldD->getHostName() != newD->getHostName() ||
		  (newD->getMode() == PRD_MANUAL &&
			oldD->getPortNumber() != newD->getPortNumber()) ||
		   oldD->getPassword() != newD->getPassword())) {
		run = CVzHelper::is_env_running(sUuid);
		if (boost::logic::indeterminate(run))
			return PRL_ERR_OPERATION_FAILED;
		if (!run)
			return PRL_ERR_SUCCESS;

		res = stop_vnc_server(sUuid, false);
		if (res == PRL_ERR_VNC_SERVER_NOT_STARTED)
			res = PRL_ERR_SUCCESS;
		if (PRL_FAILED(res))
			return res;
		res = start_vnc_server(sUuid, false);
	}
	return res;
}

PRL_RESULT Task_VzManager::editConfig()
{
	PRL_RESULT res;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	SmartPtr<CVmConfiguration> pConfig(new CVmConfiguration(cmd->GetFirstStrParam()));
	if(!IS_OPERATION_SUCCEEDED(pConfig->m_uiRcInit))
	{
		PRL_RESULT code = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Error occurred while modification CT configuration: %s",
				PRL_RESULT_TO_STRING(code));
		return code;
	}
	QString sUuid = pConfig->getVmIdentification()->getVmUuid();
	SmartPtr<CVmConfiguration> pOldConfig = getVzHelper()->getCtConfig(getClient(), sUuid, QString(), true);
	if (!pOldConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

        QString oldname = pOldConfig->getVmIdentification()->getVmName();
        QString name = pConfig->getVmIdentification()->getVmName();

	QStringList lstFullItemIds;
	pConfig->diffDocuments(pOldConfig.getImpl(), lstFullItemIds);

	// Code below prohibits all other than Hdd and Network devices for Containers
	if (!lstFullItemIds.filter(QRegExp("Hardware\\.(?!Hdd|Network|Cpu|Memory)")).isEmpty())
		return PRL_ERR_ACTION_NOT_SUPPORTED_FOR_CT;
	if (!lstFullItemIds.filter(QRegExp(Encryption::getFilter())).isEmpty())
		return PRL_ERR_ENCRYPTION_COMMIT_PROHIBITED;
	// Handle the Firewall settings change on the running CT
	if (!lstFullItemIds.filter(QRegExp("\\.(?=Firewall\\.|MAC|NetAddress|PktFilter)")).isEmpty()) {
		tribool_type run = CVzHelper::is_env_running(sUuid);
		if (run) {
			res = setupFirewall(pConfig);
			if (PRL_FAILED(res))
				return res;
		}
	}

	// Handle Name change
	if (oldname != name) {
		QString vm_uuid; // Skip uuid check
		QString vm_name = name;
		QString vm_home;

		// Lock the new name in the VmDirectory
		CVmDirectory::TemporaryCatalogueItem vmInfo(vm_uuid, vm_home, vm_name);
		res = checkAndLockRegisterParameters(&vmInfo);
		if (PRL_FAILED(res))
			return res;

		res = get_op_helper()->set_env_name(sUuid, name);
		if (PRL_SUCCEEDED(res)) {
			CDspLockedPointer< CVmDirectoryItem >
				pVmDirItem = CDspService::instance()->getVmDirManager()
				.getVmDirItemByUuid(m_sVzDirUuid, sUuid);

			if (!pVmDirItem) {
				WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem by vmUuid = %s",
						QSTR2UTF8(sUuid));
			} else {
				pVmDirItem->setVmName(name);
				PRL_RESULT ret = CDspService::instance()->getVmDirManager().updateVmDirItem(pVmDirItem);
				if (PRL_FAILED(ret))
					WRITE_TRACE(DBG_FATAL, "Can't update Container %s VmCatalogue by error: %s",
						QSTR2UTF8(sUuid), PRL_RESULT_TO_STRING(ret));
			}
		}

		// delete temporary registration
		CDspService::instance()->getVmDirManager()
			.unlockExclusiveVmParameters(&vmInfo);

		if (PRL_FAILED(res))
			return res;
	}

	// reset IP addresses for CT templates
	Task_EditVm::resetNetworkAddressesFromVmConfig(pConfig, pOldConfig);
	CDspService::instance()->getVmDirHelper().resetAdvancedParamsFromVmConfig(pConfig);

	CVmRemoteDisplay* oldD = pOldConfig->getVmSettings()->getVmRemoteDisplay();
	CVmRemoteDisplay* newD = pConfig->getVmSettings()->getVmRemoteDisplay();

	if (oldD->getPassword() != newD->getPassword()) {
		if (newD->getPassword().length() > PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN) {
			WRITE_TRACE(DBG_FATAL, "The specified remote display password is too long.");
			return CDspTaskFailure(*this)
				(PRL_ERR_VMCONF_REMOTE_DISPLAY_PASSWORD_TOO_LONG,
				QString::number(PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN));
		}
	}

	Backup::Device::Service service(pOldConfig);
	service.setContext(*this).setVmHome(pConfig->getVmIdentification()->getHomePath());
	Backup::Device::Details::Transition t = service.getTransition(pConfig);
	res = t.plant();
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->apply_env_config(pConfig, pOldConfig, cmd->GetCommandFlags());
	if (PRL_FAILED(res))
		return res;

	res = t.remove();
	if (PRL_FAILED(res))
		return res;

	// Invalidate cache
	CDspService::instance()->getVzHelper()->getConfigCache().
		remove(pConfig->getVmIdentification()->getHomePath());

	res = changeVNCServerState(pOldConfig, pConfig, sUuid);
	if (PRL_FAILED(res))
		return res;

	QStringList lstAdd, lstDel;
	// Handle application templates
	QStringList newAppTemplates = pConfig->getCtSettings()->getAppTemplate();
	QStringList oldAppTemplates = pOldConfig->getCtSettings()->getAppTemplate();

	for (int i = 0; i < newAppTemplates.size(); i++) {
		if (newAppTemplates.at(i).startsWith('.'))
			newAppTemplates[i].remove(0, 1);
	}
	for (int i = 0; i < oldAppTemplates.size(); i++) {
		if (oldAppTemplates.at(i).startsWith('.'))
			oldAppTemplates[i].remove(0, 1);
	}

	if (newAppTemplates == oldAppTemplates)
		goto ok;

	foreach(QString str, newAppTemplates)
		if (!oldAppTemplates.contains(str))
			lstAdd.append(str);
	foreach(QString str, oldAppTemplates)
		if (!newAppTemplates.contains(str))
			lstDel.append(str);

	do {
		CVzTemplateHelper TmplHelper = getVzHelper()->getVzTemplateHelper();
		res = TmplHelper.remove_templates_env(sUuid, lstDel);
		if (PRL_FAILED(res))
			return res;

		res = TmplHelper.install_templates_env(sUuid, lstAdd);
	} while (0);

ok:
	if (PRL_SUCCEEDED(res)) {
		SmartPtr<CVmConfiguration> pNewConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
		QStringList lstParams(pNewConfig ?
				pNewConfig->toString() : pConfig->toString());
		getResponseCmd()->SetParamsList(lstParams);

		sendEvent(PET_DSP_EVT_VM_CONFIG_CHANGED, sUuid);
	}

	return res;
}

PRL_RESULT Task_VzManager::register_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sPath = cmd->GetFirstStrParam();

	if (!QFileInfo(sPath).exists())
		return CDspTaskFailure(*this)(PRL_ERR_DIRECTORY_DOES_NOT_EXIST, sPath);

	QString sUuid = cmd->GetVmUuid();
	if (cmd->GetCommandFlags() & PRVF_REGENERATE_VM_UUID)
		sUuid = Uuid::createUuid().toString();

	QString vm_uuid = sUuid;
	QString vm_name;
	QString vm_home = sPath;

	PRL_RESULT res;
	CVmDirectory::TemporaryCatalogueItem vmInfo(vm_uuid, vm_home, vm_name);
	// Lock
	res = checkAndLockRegisterParameters(&vmInfo);
	if (PRL_FAILED(res))
		return res;

	SmartPtr<CVmConfiguration> pConfig;
	res = get_op_helper()->register_env(sPath, QString(), sUuid, vm_name,
			cmd->GetCommandFlags(), pConfig);
	if (PRL_SUCCEEDED(res)) {
		if (sUuid.isEmpty())
			sUuid = pConfig->getVmIdentification()->getVmUuid();

		res = getVzHelper()->insertVmDirectoryItem(pConfig);
		if (PRL_FAILED(res))
			get_op_helper()->unregister_env(
					pConfig->getVmIdentification()->getVmUuid(), 0);
		sendEvent(PET_DSP_EVT_VM_ADDED, sUuid);
	}
	// delete temporary registration
	CDspService::instance()->getVmDirManager()
		.unlockExclusiveVmParameters(&vmInfo);

	if (PRL_SUCCEEDED(res))
		getResponseCmd()->SetVmConfig(pConfig->toString());

	return res;
}

PRL_RESULT Task_VzManager::unregister_env()
{
	PRL_RESULT res, ret;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString uuid = cmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), uuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	// Check state
	res = check_env_state(PVE::DspCmdDirUnregVm, uuid);
	if (PRL_FAILED(res))
		return res;

	QString vm_name = pConfig->getVmIdentification()->getVmName();
	QString vm_home = pConfig->getVmIdentification()->getHomePath();

	// Lock by vm_uuid/vm_name/vm_home triade
	CVmDirectory::TemporaryCatalogueItem vmInfo(uuid, vm_home, vm_name);
	res = CDspService::instance()->getVmDirManager()
			.lockExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	if (PRL_SUCCEEDED(res)) {
		res = get_op_helper()->unregister_env(uuid, 0);
		if (PRL_SUCCEEDED(res)) {
			Backup::Device::Service(pConfig).disable();

			ret = CDspService::instance()->getVmDirHelper()
						.deleteVmDirectoryItem(m_sVzDirUuid, uuid);
			if (PRL_FAILED(ret) && ret != PRL_ERR_ENTRY_DOES_NOT_EXIST)
				WRITE_TRACE(DBG_FATAL, "Can't delete Container %s from VmDirectory by error: %s",
						QSTR2UTF8(uuid), PRL_RESULT_TO_STRING(ret));
			sendEvent(PET_DSP_EVT_VM_DELETED, uuid);
		}
		// delete temporary registration
		CDspService::instance()->getVmDirManager()
			.unlockExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	}

	return res;
}

PRL_RESULT Task_VzManager::set_env_userpasswd()
{
	int res;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), cmd->GetVmUuid());
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	CProtoVmGuestSetUserPasswdCommand *pCmd =
			CProtoSerializer::CastToProtoCommand<CProtoVmGuestSetUserPasswdCommand>(cmd);

	res = get_op_helper()->set_env_userpasswd(pCmd->GetVmUuid(),
					pCmd->GetUserLoginName(),
					pCmd->GetUserPassword(),
					pCmd->GetCommandFlags());
	return res;
}

PRL_RESULT Task_VzManager::auth_env_user()
{
	int res;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmLoginInGuestCommand *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>(cmd);
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), pCmd->GetVmUuid());
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	res = get_op_helper()->auth_env_user(pCmd->GetVmUuid(),
					pCmd->GetUserLoginName(),
					pCmd->GetUserPassword());
	return res;
}

PRL_RESULT Task_VzManager::clone_env()
{
	PRL_RESULT res;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmCloneCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCloneCommand>(cmd);

	QString sUuid = pCmd->GetVmUuid();
	QString sNewHome = pCmd->GetVmHomePath();
	QString sNewName = pCmd->GetVmName();
	unsigned int nFlags = pCmd->GetCommandFlags();

	if (sNewName.isEmpty())
		return PRL_ERR_VM_NAME_IS_EMPTY;

	PRL_RESULT e;
	SmartPtr<CVmConfiguration> pConfig = CDspService::instance()->
		getVmDirHelper().getVmConfigByUuid(getClient(),
				pCmd->GetVmUuid(), e, NULL);
	if (!pConfig) {
		WRITE_TRACE(DBG_FATAL, "Unable to find CT by uuid %s",
				qPrintable(pCmd->GetVmUuid()));
		return PRL_ERR_VM_GET_CONFIG_FAILED;
	}

	Template::Storage::Dao::pointer_type c;
	Template::Storage::Dao d(getClient()->getAuthHelper());
	if (PRL_SUCCEEDED(d.findByRoot(sNewHome, c)))
		return PRL_ERR_VM_REQUEST_NOT_SUPPORTED;

	SmartPtr<CVmConfiguration> pNewConfig(new CVmConfiguration);
	if (!pCmd->GetNewVmUuid().isEmpty())
		pNewConfig->getVmIdentification()->setVmUuid(pCmd->GetNewVmUuid());

	CVmDirectory::TemporaryCatalogueItem vmInfo(
			pNewConfig->getVmIdentification()->getVmUuid(),
			QString(),
			sNewName);

	res = checkAndLockRegisterParameters(&vmInfo);
	if (PRL_FAILED(res))
		return res;

	const QString &home = pConfig->getVmIdentification()->getHomePath();
	if (PRL_SUCCEEDED(d.findForEntry(home, c))) {
		Command::Clone x(this, pConfig, pNewConfig);
		res = c->export_(home, boost::bind<PRL_RESULT>(boost::ref(x),
					boost::ref(sNewHome)));
		if (PRL_SUCCEEDED(res))
			res = c->commit();
		if (PRL_SUCCEEDED(res))
			pNewConfig = x.getConfig();
	} else {
		res = check_env_state(PVE::DspCmdDirVmClone, sUuid);
		if (PRL_FAILED(res))
			return res;

		res = get_op_helper()->clone_env(pConfig, sNewHome, sNewName,
				0, pNewConfig);
	}
	if (PRL_SUCCEEDED(res)) {
		::Vm::Private::Brand b(sNewHome, getClient());
		b.remove();
		if (PRL_SUCCEEDED(res = b.stamp()))
			res = getVzHelper()->insertVmDirectoryItem(pNewConfig);
		if (PRL_FAILED(res))
			get_op_helper()->delete_env(
				pNewConfig->getVmIdentification()->getVmUuid());
	}
	// delete temporary registration
	CDspService::instance()->getVmDirManager()
		.unlockExclusiveVmParameters(&vmInfo);

	if (PRL_FAILED(res) || !pNewConfig.isValid())
		return res;

	SmartPtr<CVmConfiguration> pOldConfig(new CVmConfiguration(pNewConfig->toString()));

	bool isTemplate = (nFlags & PCVF_CLONE_TO_TEMPLATE);
	pNewConfig->getVmSettings()->getVmCommonOptions()->setTemplate(isTemplate);

	Task_CloneVm::ResetNetSettings(pNewConfig);
	Backup::Device::Dao(pNewConfig).deleteAll();

	get_op_helper()->apply_env_config(pNewConfig, pOldConfig, PVCF_DESTROY_HDD_BUNDLE);

	getResponseCmd()->SetVmConfig(pNewConfig->toString());
	{
	       CVmEvent event(PET_DSP_EVT_VM_ADDED,
			       pNewConfig->getVmIdentification()->getVmUuid(),
			       PIE_DISPATCHER);
	       SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event);
	       CDspService::instance()->getClientManager().sendPackageToAllClients(p);
	}
	// Set some parameters in the response (see Task_CloneVm)
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
			new CVmEventParameter(PVE::String,
				pNewConfig->getVmIdentification()->getVmUuid(),
				EVT_PARAM_DISP_TASK_CLONE_VM_UUID));
	pParams->addEventParameter(
			new CVmEventParameter(PVE::String,
				pNewConfig->getVmIdentification()->getVmName(),
				EVT_PARAM_DISP_TASK_CLONE_NEW_VM_NAME));
	pParams->addEventParameter(
			new CVmEventParameter(PVE::String,
				pNewConfig->getVmIdentification()->getHomePath(),
				EVT_PARAM_DISP_TASK_CLONE_NEW_VM_ROOT_DIR));
	pParams->addEventParameter(
			new CVmEventParameter(PVE::Boolean,
				QString("%1").arg(isTemplate),
				EVT_PARAM_DISP_TASK_CLONE_AS_TEMPLATE));
	return PRL_ERR_SUCCESS;
}

static QString getFullPath(const QString &sHomePath, const QString &sPath)
{
	if (QFileInfo(sPath).isAbsolute())
		return sPath;

	return QFileInfo(sHomePath + '/' + sPath).absoluteFilePath();
}

PRL_RESULT Task_VzManager::create_env_disk()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoCreateImageCommand *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCreateImageCommand>(cmd);

	/* TODO: handle bRecreateIsAllowed
	bool bRecreateIsAllowed = pCmd->IsRecreateAllowed();
	*/

	CVmHardDisk disk;
	if (!StringToElement(&disk, pCmd->GetImageConfig())) {
		WRITE_TRACE(DBG_FATAL, "%s", QSTR2UTF8(pCmd->GetImageConfig()));
		return PRL_ERR_BAD_PARAMETERS;
	}

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), pCmd->GetVmUuid());
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	if (disk.getUserFriendlyName().isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Unable to create disk: empty path");
		return PRL_ERR_BAD_PARAMETERS;
	}

	QString sPath = getFullPath(pConfig->getVmIdentification()->getHomePath(),
			disk.getUserFriendlyName());
	if (!pCmd->IsRecreateAllowed() && QFileInfo(sPath).exists()) {
		WRITE_TRACE(DBG_FATAL, "Disk image [%s] is already exist.", QSTR2UTF8(sPath));
		return CDspTaskFailure(*this)(PRL_ERR_HDD_IMAGE_IS_ALREADY_EXIST, sPath);
	}

	PRL_RESULT ret = get_op_helper()->create_env_disk(pCmd->GetVmUuid(),
			disk, pCmd->IsRecreateAllowed());
	if (PRL_FAILED(ret))
		return ret;

	getVzHelper()->getConfigCache()
		.remove(pConfig->getVmIdentification()->getHomePath());

	return PRL_ERR_SUCCESS;
}

static CVmHardDisk *find_disk_by_fname(const SmartPtr<CVmConfiguration> &pConfig,
	const QString &sName)
{
	foreach(CVmHardDisk *p, pConfig->getVmHardwareList()->m_lstHardDisks) {
		if (p->getUserFriendlyName() == sName)
			return p;
	}

	return NULL;
}

PRL_RESULT Task_VzManager::resize_env_disk()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmResizeDiskImageCommand *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmResizeDiskImageCommand>(cmd);

	unsigned int nNewSize = pCmd->GetSize();
	bool infoMode = (pCmd->GetFlags() & PRIF_DISK_INFO);

	QString sUuid = pCmd->GetVmUuid();
	SmartPtr<CVmConfiguration> x = getVzHelper()->getCtConfig(
			getClient(), sUuid);
	if (!x)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	CVmHardDisk *disk = find_disk_by_fname(x, pCmd->GetDiskImage());
	if (disk && Backup::Device::Details::Finding(*disk).isKindOf()) {
		WRITE_TRACE(DBG_FATAL, "Resize of attached backup is prohibited");
		return PRL_ERR_DISK_RESIZE_FAILED;
	}

	if (infoMode) {
		CDiskImageInfo di;
		res = get_op_helper()->get_resize_env_info(sUuid, di);
		if (PRL_FAILED(res))
			return res;
		getResponseCmd()->AddStandardParam(di.toString());
	} else {
		res = get_op_helper()->resize_env_disk(sUuid, pCmd->GetDiskImage(),
				nNewSize, pCmd->GetFlags());
	}
	if (PRL_FAILED(res))
		return res;

	getVzHelper()->getConfigCache()
		.remove(x->getVmIdentification()->getHomePath());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::prepareTask()
{
	m_sVzDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::process_state()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;

	CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
	if (s && m_nState != VMS_UNKNOWN)
	{
		s->onVmStateChanged(VMS_UNKNOWN, m_nState, getVmUuid(), m_sVzDirUuid, false);
	}
	s.unlock();

	PRL_RESULT e;
	SmartPtr<CVmConfiguration> pConfig = CDspService::instance()->
		getVmDirHelper().getVmConfigByUuid(getClient(), getVmUuid(), e, NULL);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	if ((m_nState == VMS_RUNNING) || (m_nState == VMS_RESUMING)) {
		CFirewallHelper fw(pConfig);
		res = fw.Execute();

		/* will ignore start VNC server error */
		start_vnc_server(getVmUuid(), true);
	} else if ((m_nState == VMS_STOPPED) || (m_nState == VMS_SUSPENDED)) {
		CFirewallHelper fw(pConfig, true);
		res = fw.Execute();

		/* will ignore start VNC server error */
		stop_vnc_server(getVmUuid(), true);
	} else if (m_nState == VMS_UNKNOWN) {
		/* Network configuration changed */
		CFirewallHelper fw(pConfig);
		res = fw.Execute();
	}

	return res;
}

PRL_RESULT Task_VzManager::send_snapshot_tree()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	QString sVmUuid = cmd->GetVmUuid();

	QString sVmHome = CDspVmDirManager::getVmHomeByUuid(MakeVmIdent(sVmUuid, m_sVzDirUuid));
	if (sVmHome.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Unable to find VmHome by uuid %s",
				QSTR2UTF8(sVmUuid));
		return PRL_ERR_FAILURE;
	}

	QBuffer buffer;
	if (!buffer.open(QIODevice::WriteOnly)) {
		WRITE_TRACE(DBG_FATAL, "Can't open internal buffer");
		return PRL_ERR_FAILURE;
	}

	CSavedStateStore snapTree;
	QString sSnapXml = sVmHome + "/" + VM_GENERATED_SNAPSHOTS_CONFIG_FILE;
	if (QFileInfo(sSnapXml).exists()) {
		SnapshotParser::SnapshotReturnCode rc = snapTree.Load(sSnapXml);
		if (rc == 0) {
			snapTree.Save(buffer);
		} else if (rc != SnapshotParser::EmptySnapshotTree) {
			WRITE_TRACE(DBG_FATAL, "failed to parse %s [%d]",
					QSTR2UTF8(sSnapXml), rc);
			return PRL_ERR_FAILURE;
		}
	}
	getResponseCmd()->SetSnapshotsTree(UTF8_2QSTR(buffer.data()));

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::create_env_snapshot()
{
	PRL_RESULT res = PRL_ERR_UNIMPLEMENTED;

#ifdef _LIN_
	QString sSnapUuid = Uuid::createUuid().toString();

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), cmd->GetVmUuid());
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	CProtoCreateSnapshotCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateSnapshotCommand>(cmd);

	res = get_op_helper()->create_env_snapshot(cmd->GetVmUuid(), sSnapUuid,
			pCmd->GetName(), pCmd->GetDescription(),
			pCmd->GetCommandFlags());
	if (PRL_SUCCEEDED(res)) {
		getResponseCmd()->AddStandardParam(sSnapUuid);
		sendEvent(PET_DSP_EVT_VM_SNAPSHOTED, cmd->GetVmUuid());
		sendEvent(PET_DSP_EVT_VM_SNAPSHOTS_TREE_CHANGED, cmd->GetVmUuid());
	}
#endif
	return res;
}

PRL_RESULT Task_VzManager::delete_env_snapshot()
{
	PRL_RESULT res = PRL_ERR_UNIMPLEMENTED;
#if _LIN_
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), cmd->GetVmUuid());
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	CProtoDeleteSnapshotCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoDeleteSnapshotCommand>(cmd);

	res =  get_op_helper()->delete_env_snapshot(cmd->GetVmUuid(),
			pCmd->GetSnapshotUuid(), pCmd->GetChild());
	if (PRL_SUCCEEDED(res)) {
		sendEvent(PET_DSP_EVT_VM_STATE_DELETED, cmd->GetVmUuid());
		sendEvent(PET_DSP_EVT_VM_SNAPSHOTS_TREE_CHANGED, cmd->GetVmUuid());
	}
#endif
	return res;
}

PRL_RESULT Task_VzManager::switch_env_snapshot()
{
	PRL_RESULT res = PRL_ERR_UNIMPLEMENTED;
#ifdef _LIN_
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	CProtoSwitchToSnapshotCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoSwitchToSnapshotCommand>(cmd);

	QString sVmUuid = cmd->GetVmUuid();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sVmUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	res = get_op_helper()->switch_env_snapshot(sVmUuid, pCmd->GetSnapshotUuid(),
			pCmd->GetCommandFlags());
	if (PRL_SUCCEEDED(res)) {
		::Backup::Device::Service(pConfig).teardown();
		// Invalidate cache
		CDspService::instance()->getVzHelper()->getConfigCache().
			remove(CDspVmDirManager::getVmHomeByUuid(MakeVmIdent(sVmUuid, m_sVzDirUuid)));

		sendEvent(PET_DSP_EVT_VM_RESTORED, cmd->GetVmUuid());
		sendEvent(PET_DSP_EVT_VM_SNAPSHOTS_TREE_CHANGED, cmd->GetVmUuid());
	}
#endif
	return res;
}

PRL_RESULT Task_VzManager::start_vnc_server(QString sCtUuid, bool onCtStart)
{
#ifdef _LIN_
	QString vncServerApp = "prl_vzvncserver_app";

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), sCtUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	CVmRemoteDisplay *remDisplay = pConfig->getVmSettings()->getVmRemoteDisplay();

	PRL_VM_REMOTE_DISPLAY_MODE mode = remDisplay->getMode();
	if (mode == PRD_DISABLED) {
		if (onCtStart)
			return PRL_ERR_SUCCESS;
		WRITE_TRACE(DBG_FATAL, "VNC server for Container UUID %s is disabled", QSTR2UTF8(sCtUuid));
		return PRL_ERR_VNC_SERVER_DISABLED;
	}
	if (getVzHelper()->isCtVNCServerRunning(sCtUuid))
	{
		WRITE_TRACE(DBG_FATAL, "VNC server for Container UUID %s already started", QSTR2UTF8(sCtUuid));
		return PRL_ERR_VNC_SERVER_ALREADY_STARTED;
	}
	CDspVzHelper::vncServer_type x(new CDspVNCStarter());
	CDispRemoteDisplayPreferences* rdConfig =
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getRemoteDisplayPreferences();

	Vnc::range_type r(rdConfig->getMinPort(), rdConfig->getMaxPort());
	PRL_RESULT e = x->Start(QString(vncServerApp),
			CVzHelper::get_ctid_by_uuid(sCtUuid), remDisplay, r);
	if (PRL_FAILED(e))
		return e;
	if (!getVzHelper()->addCtVNCServer(sCtUuid, x))
	{
		WRITE_TRACE(DBG_FATAL, "VNC server for Container UUID %s already started", QSTR2UTF8(sCtUuid));
		return PRL_ERR_VNC_SERVER_ALREADY_STARTED;
	}
	if (mode == PRD_AUTO) {
		// send event to GUI for changing the config params
		CVmEvent event(PET_DSP_EVT_VM_CONFIG_CHANGED, sCtUuid, PIE_DISPATCHER);
		SmartPtr<IOPackage> pkgNew =
			DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		CDspService::instance()->getClientManager().sendPackageToAllClients(pkgNew);
	}
	return PRL_ERR_SUCCESS;
#else
	(void)sCtUuid;
	(void)onCtStart;
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

PRL_RESULT Task_VzManager::stop_vnc_server(QString sCtUuid, bool onCtStop)
{
	PRL_RESULT nRetCode = PRL_ERR_UNIMPLEMENTED;
#ifdef _LIN_
	nRetCode = getVzHelper()->removeCtVNCServer(sCtUuid, onCtStop);
#else
	(void)sCtUuid;
	(void)onCtStop;
#endif
	return nRetCode;
}

PRL_RESULT Task_VzManager::reinstall_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(
							getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoCommandWithTwoStrParams *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandWithTwoStrParams>(cmd);

	QString uuid = pCmd->GetFirstStrParam();
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(
			getClient(), uuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	return get_op_helper()->reinstall_env(uuid,
				pCmd->GetSecondStrParam(),
				pCmd->GetCommandFlags());
}

PRL_RESULT Task_VzManager::process_cmd()
{
	if (CDspService::instance()->isServerStopping())
		return PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	PRL_UINT32 nCmd = getRequestPackage()->header.type;
	switch(nCmd) {
	case PVE::DspCmdDirVmCreate:
		ret = create_env();
		break;
	case PVE::DspCmdVmStartEx:
	case PVE::DspCmdVmStart:
		ret = start_env();
		break;
	case PVE::DspCmdVmPause:
		ret = pause_env();
		break;
	case PVE::DspCmdVmRestartGuest:
		ret = restart_env();
		break;
	case PVE::DspCmdVmReset:
		ret = reset_env();
		break;
	case PVE::DspCmdVmStop:
		ret = stop_env();
		break;
	case PVE::DspCmdVmSuspend:
		ret = suspend_env();
		break;
	case PVE::DspCmdVmResume:
		ret = resume_env();
		break;
	case PVE::DspCmdDirVmDelete:
		ret = delete_env();
		break;
	case PVE::DspCmdDirVmEditCommit:
		ret = editConfig();
		break;
	case PVE::DspCmdDirRegVm:
		ret = register_env();
		break;
	case PVE::DspCmdDirUnregVm:
		ret = unregister_env();
		break;
	case PVE::DspCmdVmGuestSetUserPasswd:
		ret = set_env_userpasswd();
		break;
	case PVE::DspCmdVmAuthWithGuestSecurityDb:
		ret = auth_env_user();
		break;
	case PVE::DspCmdDirVmClone:
		ret = clone_env();
		break;
	case PVE::DspCmdDirCreateImage:
		ret = create_env_disk();
		break;
	case PVE::DspCmdVmResizeDisk:
		ret = resize_env_disk();
		break;
	case PVE::DspCmdVmMount:
		ret = mount_env();
		break;
	case PVE::DspCmdVmUmount:
		ret = umount_env();
		break;
	case PVE::DspCmdVmGetSnapshotsTree:
		ret = send_snapshot_tree();
		break;
	case PVE::DspCmdVmCreateSnapshot:
		ret = create_env_snapshot();
		break;
	case PVE::DspCmdVmDeleteSnapshot:
		ret = delete_env_snapshot();
		break;
	case PVE::DspCmdVmSwitchToSnapshot:
		ret = switch_env_snapshot();
		break;
	case PVE::DspCmdVmStartVNCServer:
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
		if (cmd->IsValid())
			ret = start_vnc_server(cmd->GetVmUuid(), false);
		else
			ret = PRL_ERR_UNRECOGNIZED_REQUEST;
		break;
	}
	case PVE::DspCmdDirVmMove:
		ret = move_env();
		break;
	case PVE::DspCmdVmStopVNCServer:
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
		if (cmd->IsValid())
			ret = stop_vnc_server(cmd->GetVmUuid(), false);
		else
			ret = PRL_ERR_UNRECOGNIZED_REQUEST;
		break;
	}
	case PVE::DspCmdVmGuestGetNetworkSettings:
		ret = send_network_settings();
		break;
	case PVE::DspCmdVmGetPackedProblemReport:
	case PVE::DspCmdVmGetProblemReport:
		ret = send_problem_report();
		break;
	case PVE::DspCmdVmCommitEncryption:
		ret = commit_encryption();
		break;
	case PVE::DspCmdCtReinstall:
		ret = reinstall_env();
		break;
	default:
		ret = PRL_ERR_UNIMPLEMENTED;
	}

	return ret;
}

PRL_RESULT Task_VzManager::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if (PRL_FAILED(getLastErrorCode()))
			throw getLastErrorCode();

		if (m_bProcessState) {
			ret = process_state();
		} else {
			ret = process_cmd();
		}
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Action failed with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING(ret));
	}

	setLastErrorCode(ret);

	return ret;
}

void Task_VzManager::finalizeTask()
{
	if (!m_pResponseCmd.isValid())
		return;

	PRL_RESULT res = getLastErrorCode();
	// Send response
	if (PRL_FAILED(res)) {
		if (res == PRL_ERR_VZ_OPERATION_FAILED ||
				res == PRL_ERR_VZCTL_OPERATION_FAILED)
		{
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, get_op_helper()->get_error_msg(),
					EVT_PARAM_DETAIL_DESCRIPTION));
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::UnsignedInt,
					QString::number(get_op_helper()->get_rc()),
					EVT_PARAM_OP_RC));
		}
		getClient()->sendResponseError(getLastError(), getRequestPackage());
	} else {
		getClient()->sendResponse(m_pResponseCmd, getRequestPackage());
	}
}


PRL_RESULT Task_VzManager::move_ephemeral(
		Template::Storage::Catalog *catalog,
		const QString &target,
		SmartPtr<CVmConfiguration> &config)
{
	if (!config->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_VM_REQUEST_NOT_SUPPORTED;

	QString uuid = config->getVmIdentification()->getVmUuid();
	boost::optional<QString> tgDirUuid = m_ephemeral->find(target);
	if (!tgDirUuid) {
		WRITE_TRACE(DBG_FATAL, "Unable to find dir uuid for %s",
				qPrintable(target));
		return PRL_ERR_UNEXPECTED;
	}

	SmartPtr<CVmConfiguration> newConfig(new CVmConfiguration(config.getImpl()));

	QFileInfo x(target, CVzHelper::build_ctid_from_uuid(uuid)); 
	newConfig->getVmIdentification()->setHomePath(x.filePath());

	Instrument::Command::Batch b;
	Template::Facade::Workbench w;
	Template::Facade::Registrar reg(tgDirUuid.get(), *newConfig, w);
	b.addItem(boost::bind(&Template::Facade::Registrar::begin, &reg),
			boost::bind(&Template::Facade::Registrar::rollback, &reg));
	Command::Move::Import i(this, newConfig, catalog);
	b.addItem(boost::bind(&Command::Move::Import::execute, i));
	b.addItem(boost::bind(&CDspVmDirHelper::deleteVmDirectoryItem,
				&w.getDirectoryHelper(), m_sVzDirUuid, uuid));
	b.addItem(boost::bind(&Template::Facade::Registrar::execute, &reg));
	b.addItem(boost::bind(&Template::Facade::Registrar::commit, &reg));

	return b.execute();
}

PRL_RESULT Task_VzManager::move_regular(const QString &target,
		SmartPtr<CVmConfiguration> &config)
{
	QString uuid = config->getVmIdentification()->getVmUuid();
	QString name = config->getVmIdentification()->getVmName();

	CVmDirectory::TemporaryCatalogueItem vmInfo(uuid, target, name);
	PRL_RESULT res = CDspService::instance()->getVmDirManager()
		.lockExistingExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	if (PRL_FAILED(res)) {
		CDspTaskFailure f(*this);
		f.setCode(res);
		switch (res)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered",
					QSTR2UTF8(vmInfo.vmXmlPath));
			return f(vmInfo.vmName, vmInfo.vmXmlPath);

		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
					QSTR2UTF8(vmInfo.vmUuid), QSTR2UTF8(vmInfo.vmName), QSTR2UTF8(vmInfo.vmXmlPath));
			return f.setToken(vmInfo.vmUuid).setToken(vmInfo.vmXmlPath)
				.setToken(vmInfo.vmName)();
		}
	}

	do {
		res = get_op_helper()->move_env(uuid, target, name);
		if (PRL_FAILED(res)) 
			break;

		config = CVzHelper::get_env_config(uuid);
		if (!config) {
			res = PRL_ERR_CT_NOT_FOUND;
			break;
		}

		// Update Vm directory item
		CDspLockedPointer< CVmDirectoryItem >
			pVmDirItem = CDspService::instance()->getVmDirManager()
			.getVmDirItemByUuid(m_sVzDirUuid, uuid);
		if (!pVmDirItem) {
			WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem by vmUuid = %s",
					QSTR2UTF8(uuid));
			res = PRL_ERR_CT_NOT_FOUND;
			break;
		}

		pVmDirItem->setVmHome(config->getVmIdentification()->getHomePath());
		pVmDirItem->setCtId(config->getVmIdentification()->getCtId());
		res = CDspService::instance()->getVmDirManager().updateVmDirItem(pVmDirItem);
		if (PRL_FAILED(res))
			WRITE_TRACE(DBG_FATAL, "Can't update Container %s VmCatalogue by error: %s",
					QSTR2UTF8(uuid), PRL_RESULT_TO_STRING(res));
	} while (0);

	CDspService::instance()->getVmDirManager().
		unlockExclusiveVmParameters(&vmInfo);

	return res;
}

PRL_RESULT Task_VzManager::move_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmMoveCommand *pCmd = CProtoSerializer::
				CastToProtoCommand<CProtoVmMoveCommand>(cmd);
	QString sUuid = pCmd->GetVmUuid();
	QString target = QFileInfo(pCmd->GetNewHomePath()).absoluteFilePath();
	if (target.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "New container home path path is empty");
		return PRL_ERR_INVALID_PARAM;
	}

	PRL_RESULT res = check_env_state(PVE::DspCmdDirVmMove, sUuid);
	if (PRL_FAILED(res))
		return res;

	PRL_RESULT e;
	SmartPtr<CVmConfiguration> pConfig = CDspService::instance()->
		getVmDirHelper().getVmConfigByUuid(getClient(), sUuid, e, NULL);
	if (!pConfig) {
		WRITE_TRACE(DBG_FATAL, "Can not get container ID for UUID %s",
				QSTR2UTF8(sUuid));
		return PRL_ERR_CT_NOT_FOUND;
	}

	QString home = pConfig->getVmIdentification()->getHomePath();
	QString sName = pConfig->getVmIdentification()->getVmName();
	Template::Storage::Dao::pointer_type c;
	Template::Storage::Dao d(getClient()->getAuthHelper());
	res = d.findByRoot(target, c);
	if (PRL_SUCCEEDED(res)) {
		res = move_ephemeral(c.data(), target, pConfig);
	} else {
		res = d.findForEntry(home, c);
		if (PRL_SUCCEEDED(res))
			res = PRL_ERR_VM_REQUEST_NOT_SUPPORTED;
		else
			res = move_regular(target, pConfig);
	}

	if (PRL_SUCCEEDED(res))
		getVzHelper()->getConfigCache().remove(home);

	if (PRL_FAILED(res))
		return res;

	getResponseCmd()->SetVmConfig(pConfig->toString());

	// Set some parameters in the response
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();
	pParams->addEventParameter(
			new CVmEventParameter(PVE::String,
				pConfig->getVmIdentification()->getVmUuid(),
				EVT_PARAM_DISP_TASK_VM_UUID));
	pParams->addEventParameter(
			new CVmEventParameter(PVE::String,
				pConfig->getVmIdentification()->getHomePath(),
				EVT_PARAM_DISP_TASK_MOVE_NEW_HOME_PATH));

	// send event to GUI for changing the config params
	CVmEvent event(PET_DSP_EVT_VM_CONFIG_CHANGED, sUuid, PIE_DISPATCHER);
	SmartPtr<IOPackage> pkg =
		DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
	CDspService::instance()->getClientManager().sendPackageToAllClients(pkg);
	return res;
}

PRL_RESULT Task_VzManager::send_network_settings()
{
	PRL_RESULT res = PRL_ERR_UNIMPLEMENTED;
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sUuid = cmd->GetVmUuid();
	CProtoCommandDspWsResponse *pResponseCmd = getResponseCmd();
	SmartPtr<CHostHardwareInfo> pHostHwInfo(new CHostHardwareInfo);

	res = get_op_helper()->get_env_netinfo(sUuid,
			pHostHwInfo->m_lstNetworkAdapters);
	if (PRL_FAILED(res))
		return res;

	pResponseCmd->AddStandardParam(pHostHwInfo->toString());;

	SmartPtr<IOPackage> pkg =
		DispatcherPackage::createInstance(PVE::DspWsResponse, pResponseCmd, getRequestPackage());
	getClient()->sendPackage(pkg);

	return res;
}

PRL_RESULT Task_VzManager::send_problem_report()
{
	CDspProblemReportHelper::getProblemReport(getClient(), getRequestPackage());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::commit_encryption()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CVmConfiguration config(cmd->GetFirstStrParam());
	if (!IS_OPERATION_SUCCEEDED(config.m_uiRcInit))
		return PRL_ERR_PARSE_VM_CONFIG;

	QString uuid = config.getVmIdentification()->getVmUuid();
	SmartPtr<CVmConfiguration> old = getVzHelper()->getCtConfig(getClient(), uuid);
	if (!old)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	PRL_RESULT res = check_env_state(PVE::DspCmdVmCommitEncryption, uuid);
	if (PRL_FAILED(res))
		return res;

	QStringList x;
	config.diffDocuments(old.get(), x);
	// Settings.Runtime.InternalVmInfo stores CVmEvent which is always changed
	// during config commit
	if (!x.filter(QRegExp(QString("^(?:(?!") + Encryption::getFilter() +
		"|Settings\\.Runtime\\.InternalVmInfo"
		"|Settings\\.RemoteDisplay\\.WebSocketPortNumber")).isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "encryption changes were rejected:\n%s",
			qPrintable(x.join("\n")));
		return PRL_ERR_ENCRYPTION_COMMIT_REJECTED;
	}

	// XXX: Only changes in disk encryption settings are handled here.
	//      New encrypted disks are added in editConfig()
	const QList<CVmHardDisk *> &n = config.getVmHardwareList()->m_lstHardDisks;
	foreach (const CVmHardDisk *od, old->getVmHardwareList()->m_lstHardDisks)
	{
		Encryption::Match m(od);
		QList<CVmHardDisk *>::const_iterator nd = std::find_if(
			n.constBegin(), n.constEnd(), m);
		if (nd != n.constEnd())
		{
			res = get_op_helper()->set_disk_encryption(uuid,
				**nd, cmd->GetCommandFlags());
			if (PRL_FAILED(res))
				return res;
		}
	}

	CDspService::instance()->getVzHelper()->getConfigCache().
		remove(config.getVmIdentification()->getHomePath());

	SmartPtr<CVmConfiguration> c = getVzHelper()->getCtConfig(getClient(), uuid, QString(), true);
	if (!c)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	getResponseCmd()->SetParamsList(QStringList() << c->toString());
	sendEvent(PET_DSP_EVT_VM_CONFIG_CHANGED, uuid);
	return PRL_ERR_SUCCESS;
}
