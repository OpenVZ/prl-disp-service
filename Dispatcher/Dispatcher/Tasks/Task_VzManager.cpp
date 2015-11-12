///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzManager.cpp
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
/////////////////////////////////////////////////////////////////////////////////

#include "Libraries/Std/PrlAssert.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include "CDspProblemReportHelper.h"
#include "Dispatcher/Dispatcher/Tasks/Task_EditVm.h"
#include "Dispatcher/Dispatcher/Tasks/Task_CloneVm.h"
#include "Task_VzManager.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Dispatcher/Dispatcher/Tasks/Task_UpdateCommonPrefs.h"
#include "CDspVmStateSender.h"

#ifdef _LIN_
#include "vzctl/libvzctl.h"
#endif


Task_VzManager::Task_VzManager(const SmartPtr<CDspClient>& pClient,
		 const SmartPtr<IOPackage>& p) :
	CDspTaskHelper(pClient, p),
	m_pResponseCmd(CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), PRL_ERR_SUCCESS)),
	m_pVzOpHelper(new CVzOperationHelper(Task_VzManager::sendEvt, this))

{
	m_bProcessState = false;
	m_nState = VMS_UNKNOWN;
}

Task_VzManager::Task_VzManager(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p,
		const QString &sUuid, VIRTUAL_MACHINE_STATE nState) :
	CDspTaskHelper(pClient, p),
	m_pResponseCmd(CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), PRL_ERR_SUCCESS)),
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

	SmartPtr<IOPackage> p =	DispatcherPackage::createInstance( PVE::DspVmEvent, event,
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
	return CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( m_pResponseCmd );
}

void Task_VzManager::sendEvent(PRL_EVENT_TYPE type, const QString &sUuid)
{
	/**
	 * Notify all user: vm directory changed
	 */
	CVmEvent event( type, sUuid, PIE_DISPATCHER );
	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

	CDspService::instance()->getClientManager().sendPackageToAllClients( p );
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

	if (PRL_FAILED( lockResult))
	{
		switch (lockResult)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED:
			getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

		default:
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
		}

		return lockResult;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::create_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage());
	if ( ! cmd->IsValid() )
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmCreateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCreateCommand>(cmd);
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	SmartPtr<CVmConfiguration> pConfig(new CVmConfiguration);
	pConfig->fromString(pCmd->GetVmConfig());

	QString sPath = pCmd->GetVmHomePath();
	if (!sPath.isEmpty() && !QDir::isAbsolutePath( sPath) ) {
		WRITE_TRACE(DBG_FATAL, "Invalid path '%s'", QSTR2UTF8(sPath));
		return PRL_ERR_VMDIR_PATH_IS_NOT_ABSOLUTE;
	}

	QString vm_uuid = pConfig->getVmIdentification()->getVmUuid();
	QString vm_name = pConfig->getVmIdentification()->getVmName();
	QString vm_home;

	CVmDirectory::TemporaryCatalogueItem vmInfo( vm_uuid, vm_home, vm_name);
	// Lock
	PRL_RESULT res = checkAndLockRegisterParameters(&vmInfo);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->create_env(sPath, pConfig, pCmd->GetCommandFlags());
	if (PRL_SUCCEEDED(res)) {
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
		sendEvent(PET_DSP_EVT_VM_ADDED, vm_uuid);
	}

	return res;
}

PRL_RESULT Task_VzManager::setupFirewall(SmartPtr<CVmConfiguration> &pConfig)
{
	CFirewallHelper fw(pConfig);

	PRL_RESULT ret = fw.Execute();
	if (PRL_FAILED(ret))
	{
		if ( ret == PRL_ERR_FIREWALL_TOOL_EXECUTED_WITH_ERROR )
		{
			getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );
			getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String,
						fw.GetErrorMessage(),
						EVT_PARAM_DETAIL_DESCRIPTION )
					);
		}
		return ret;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::start_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	if (CDspService::instance()->isServerStopping())
		return PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;

	QString sUuid = pCmd->GetVmUuid();

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	if (pConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_CANT_TO_START_VM_TEMPLATE;

	PRL_RESULT res = check_env_state(PVE::DspCmdVmStart, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->start_env(
		sUuid, CDspService::instance()->getHaClusterHelper()->getStartCommandFlags(pCmd));
	if (PRL_FAILED(res))
		return res;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::restart_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;
	if (CDspService::instance()->isServerStopping())
		return PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;

	QString sUuid = pCmd->GetVmUuid();

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
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
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage());
	if ( ! cmd->IsValid() )
		return PRL_ERR_FAILURE;

	CProtoVmCommandStop *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCommandStop>(cmd);
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	PRL_RESULT res;
	QString sUuid = cmd->GetVmUuid();

	res = check_env_state(PVE::DspCmdVmStop, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->stop_env(sUuid, pCmd->GetStopMode());
	return res;
}

PRL_RESULT Task_VzManager::mount_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage());
	if ( ! cmd->IsValid() )
		return PRL_ERR_FAILURE;

	PRL_RESULT res;
	QString sUuid = cmd->GetVmUuid();

	res = check_env_state(PVE::DspCmdVmMount, sUuid);
	if (PRL_FAILED(res))
		return res;

	bool infoMode = (cmd->GetCommandFlags() & PMVD_INFO);

	if (infoMode) {
		SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
		if (!pConfig)
			return PRL_ERR_VM_GET_CONFIG_FAILED;
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
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage());
	if ( ! cmd->IsValid() )
		return PRL_ERR_FAILURE;

	PRL_RESULT res;
	QString sUuid = cmd->GetVmUuid();

	res = check_env_state(PVE::DspCmdVmUmount, sUuid);
	if (PRL_FAILED(res))
		return res;

	return get_op_helper()->umount_env(sUuid);
}

PRL_RESULT Task_VzManager::suspend_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	PRL_RESULT res;
	QString sUuid = pCmd->GetVmUuid();

	res = check_env_state(PVE::DspCmdVmSuspend, sUuid);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->suspend_env(sUuid);
	return res;
}

PRL_RESULT Task_VzManager::resume_env()
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	PRL_RESULT res;
	QString sUuid = pCmd->GetVmUuid();

	res = check_env_state(PVE::DspCmdVmResume, sUuid);
	if (PRL_FAILED(res))
		return res;

	return get_op_helper()->resume_env(sUuid, pCmd->GetCommandFlags());
}

PRL_RESULT Task_VzManager::delete_env()
{
	PRL_RESULT res, ret;

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!pCmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmDeleteCommand * pDeleteCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeleteCommand>(pCmd);
	QString sUuid = pDeleteCmd->GetVmUuid();

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

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	QString vm_uuid = pConfig->getVmIdentification()->getVmUuid();
	QString vm_name = pConfig->getVmIdentification()->getVmName();
	QString vm_home = pConfig->getVmIdentification()->getHomePath();

	CVmDirectory::TemporaryCatalogueItem vmInfo( vm_uuid, vm_home, vm_name);

	res = CDspService::instance()->getVmDirManager()
			.lockExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	if (PRL_SUCCEEDED(res)) {
		res = get_op_helper()->delete_env(sUuid);
		if (PRL_SUCCEEDED(res)) {
			// FIXME: rollback operation
			ret = CDspService::instance()->getVmDirHelper()
					.deleteVmDirectoryItem(m_sVzDirUuid, vm_uuid);
			if (PRL_FAILED(ret) && ret != PRL_ERR_ENTRY_DOES_NOT_EXIST)
				WRITE_TRACE(DBG_FATAL, "Can't delete Container %s from VmDirectory by error: %s",
						QSTR2UTF8(vm_uuid), PRL_RESULT_TO_STRING(ret) );
		}

		// delete temporary registration
		CDspService::instance()->getVmDirManager()
			.unlockExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	}

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
	VIRTUAL_MACHINE_STATE nState = VMS_UNKNOWN;

	// Start VNC
	if ( oldD->getMode() != newD->getMode() ) {
		res = CVzHelper::get_env_status(sUuid, nState);
		if (PRL_FAILED(res) || nState != VMS_RUNNING)
			return res;

		if (newD->getMode() == PRD_DISABLED) {
			res = stop_vnc_server(sUuid, false);
			if (res == PRL_ERR_VNC_SERVER_NOT_STARTED)
				res = PRL_ERR_SUCCESS;
		} else {
			res = start_vnc_server(sUuid, false);
		}
	}
	// VNC config has been changed
	else if ( newD->getMode() != PRD_DISABLED &&
			  (oldD->getHostName() != newD->getHostName() ||
			   oldD->getPortNumber() != newD->getPortNumber() ||
			   oldD->getPassword() != newD->getPassword()) ) {
		res = CVzHelper::get_env_status(sUuid, nState);
		if (PRL_FAILED(res) || nState != VMS_RUNNING)
			return res;

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

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if ( ! cmd->IsValid() )
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	SmartPtr<CVmConfiguration> pConfig( new CVmConfiguration( cmd->GetFirstStrParam() ) );
	if( !IS_OPERATION_SUCCEEDED( pConfig->m_uiRcInit ) )
	{
		PRL_RESULT code = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Error occurred while modification CT configuratione: %s",
				PRL_RESULT_TO_STRING( code ) );
		return code;
	}
	QString sUuid = pConfig->getVmIdentification()->getVmUuid();
	SmartPtr<CVmConfiguration> pOldConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
	if (!pOldConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

        QString oldname = pOldConfig->getVmIdentification()->getVmName();
        QString name = pConfig->getVmIdentification()->getVmName();

	// Handle the Firewall settings change on the running CT
	QStringList lstFullItemIds;
	pConfig->diffDocuments(pOldConfig.getImpl(), lstFullItemIds);
	QString qsDiff = lstFullItemIds.join(" ");
	if (qsDiff.contains(".Firewall.") || qsDiff.contains(".MAC") ||
			qsDiff.contains(".NetAddress")){
		VIRTUAL_MACHINE_STATE nState = VMS_UNKNOWN;
		PRL_RESULT res = getVzHelper()->getVzlibHelper().get_env_status(sUuid, nState);
		if (nState == VMS_RUNNING) {
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
		CVmDirectory::TemporaryCatalogueItem vmInfo( vm_uuid, vm_home, vm_name);
		res = checkAndLockRegisterParameters(&vmInfo);
		if (PRL_FAILED(res))
			return res;

		res = get_op_helper()->set_env_name(sUuid, name);
		if (PRL_SUCCEEDED(res)) {
			CDspLockedPointer< CVmDirectoryItem >
				pVmDirItem = CDspService::instance()->getVmDirManager()
				.getVmDirItemByUuid(m_sVzDirUuid, sUuid );

			if (!pVmDirItem) {
				WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem by vmUuid = %s",
						QSTR2UTF8(sUuid));
			} else {
				pVmDirItem->setVmName(name);
				PRL_RESULT ret = CDspService::instance()->getVmDirManager().updateVmDirItem(pVmDirItem);
				if (PRL_FAILED(ret) )
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

	// update High Availability Cluster resource
	if (pConfig->getVmSettings()->getHighAvailability()->toString() !=
			pOldConfig->getVmSettings()->getHighAvailability()->toString()
		&& CFileHelper::isSharedFS(pConfig->getVmIdentification()->getHomePath())) {
		res = CDspService::instance()->getHaClusterHelper()->updateClusterResourceParams(
				sUuid,
				pOldConfig->getVmSettings()->getHighAvailability(),
				pConfig->getVmSettings()->getHighAvailability(),
				pConfig->getVmIdentification()->getHomePath(),
				PVT_CT);
		if (PRL_FAILED(res))
			return res;
	}

	CVmRemoteDisplay* oldD = pOldConfig->getVmSettings()->getVmRemoteDisplay();
	CVmRemoteDisplay* newD = pConfig->getVmSettings()->getVmRemoteDisplay();

	if (oldD->getPassword() != newD->getPassword()) {
		if (newD->getPassword().length() > PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN) {
			WRITE_TRACE(DBG_FATAL, "The specified remote display password is too long.");
			getLastError()->addEventParameter(
				new CVmEventParameter(
					PVE::UnsignedInt,
					QString::number(PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN),
					EVT_PARAM_MESSAGE_PARAM_0));

			return PRL_ERR_VMCONF_REMOTE_DISPLAY_PASSWORD_TOO_LONG;
		}
	}
	res = get_op_helper()->apply_env_config(pConfig, pOldConfig, cmd->GetCommandFlags());
	if (PRL_FAILED(res))
		return res;
	// Invalidate cache
	CDspService::instance()->getVzHelper()->getConfigCache().
		remove(pConfig->getVmIdentification()->getHomePath());

	res = changeVNCServerState(pOldConfig, pConfig, sUuid);
	if (PRL_FAILED(res))
		return res;

	// Handle memory limit change
	unsigned int newRamSize = pConfig->getVmHardwareList()->getMemory()->getRamSize();
	unsigned int oldRamSize = pOldConfig->getVmHardwareList()->getMemory()->getRamSize();
	if (newRamSize != oldRamSize) {
		VIRTUAL_MACHINE_STATE nState = VMS_UNKNOWN;
		PRL_RESULT res = getVzHelper()->getVzlibHelper().get_env_status(sUuid, nState);
		if (PRL_SUCCEEDED(res) && (nState == VMS_RUNNING))
			adjustReservedMemLimit((long long)newRamSize - oldRamSize);
	}

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
	if ( PRL_SUCCEEDED(res) ) {
		SmartPtr<CVmConfiguration> pNewConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
		QStringList lstParams(pNewConfig ?
				pNewConfig->toString() : pConfig->toString());
		getResponseCmd()->SetParamsList( lstParams );

		sendEvent(PET_DSP_EVT_VM_CONFIG_CHANGED, sUuid);
	}

	return res;
}

PRL_RESULT Task_VzManager::register_env()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if ( ! cmd->IsValid() )
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sPath = cmd->GetFirstStrParam();

	if (!QFileInfo(sPath).exists())
	{
		getLastError()->addEventParameter(new CVmEventParameter(
						PVE::String,
						sPath,
						EVT_PARAM_MESSAGE_PARAM_0 ));
		return PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
	}
	SmartPtr<CVmConfiguration> pConfig;

	QString sUuid = cmd->GetVmUuid();
	if (cmd->GetCommandFlags() & PRVF_REGENERATE_VM_UUID)
		sUuid = Uuid::createUuid().toString();

	QString vm_uuid = sUuid;
	QString vm_name;
	QString vm_home = sPath + "/" + VMDIR_DEFAULT_VM_CONFIG_FILE;

	PRL_RESULT res;
	CVmDirectory::TemporaryCatalogueItem vmInfo( vm_uuid, vm_home, vm_name);
	// Lock
	res = checkAndLockRegisterParameters(&vmInfo);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->register_env(sPath, sUuid, cmd->GetCommandFlags(), pConfig);
	if (PRL_SUCCEEDED(res)) {
		res = getVzHelper()->insertVmDirectoryItem(pConfig);
		if (PRL_FAILED(res))
			get_op_helper()->unregister_env(
					pConfig->getVmIdentification()->getVmUuid(), 0);
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

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if ( ! cmd->IsValid() )
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString uuid = cmd->GetVmUuid();

	// Check state
	res = check_env_state(PVE::DspCmdDirUnregVm, uuid);
	if (PRL_FAILED(res))
		return res;

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), uuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	QString vm_name = pConfig->getVmIdentification()->getVmName();
	QString vm_home = pConfig->getVmIdentification()->getHomePath();

	// Lock by vm_uuid/vm_name/vm_home triade
	CVmDirectory::TemporaryCatalogueItem vmInfo(uuid, vm_home, vm_name);
	res = CDspService::instance()->getVmDirManager()
			.lockExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	if (PRL_SUCCEEDED(res)) {
		res = get_op_helper()->unregister_env(uuid, 0);
		if (PRL_SUCCEEDED(res)) {
			ret = CDspService::instance()->getVmDirHelper()
						.deleteVmDirectoryItem(m_sVzDirUuid, uuid);
			if (PRL_FAILED(ret) && ret != PRL_ERR_ENTRY_DOES_NOT_EXIST)
				WRITE_TRACE(DBG_FATAL, "Can't delete Container %s from VmDirectory by error: %s",
						QSTR2UTF8(uuid), PRL_RESULT_TO_STRING(ret) );
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

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

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

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmLoginInGuestCommand *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>(cmd);

	res = get_op_helper()->auth_env_user(pCmd->GetVmUuid(),
					pCmd->GetUserLoginName(),
					pCmd->GetUserPassword());
	return res;
}


PRL_RESULT Task_VzManager::clone_env()
{
	PRL_RESULT res;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmCloneCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCloneCommand>(cmd);

	QString sUuid = pCmd->GetVmUuid();
	QString sNewHome = pCmd->GetVmHomePath();
	QString sNewName = pCmd->GetVmName();
	unsigned int nFlags = pCmd->GetCommandFlags();

	if (sNewName.isEmpty())
		return PRL_ERR_VM_NAME_IS_EMPTY;

	res = check_env_state(PVE::DspCmdDirVmClone, sUuid);
	if (PRL_FAILED(res))
		return res;

	SmartPtr<CVmConfiguration> pNewConfig(new CVmConfiguration);

	QString vm_uuid = pNewConfig->getVmIdentification()->getVmUuid();

	CVmDirectory::TemporaryCatalogueItem vmInfo(vm_uuid, QString(), sNewName);

	res = checkAndLockRegisterParameters(&vmInfo);
	if (PRL_FAILED(res))
		return res;

	res = get_op_helper()->clone_env(sUuid, sNewHome, sNewName, nFlags, pNewConfig);
	if (PRL_SUCCEEDED(res)) {
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
			new CVmEventParameter( PVE::String,
				pNewConfig->getVmIdentification()->getVmUuid(),
				EVT_PARAM_DISP_TASK_CLONE_VM_UUID ) );
	pParams->addEventParameter(
			new CVmEventParameter( PVE::String,
				pNewConfig->getVmIdentification()->getVmName(),
				EVT_PARAM_DISP_TASK_CLONE_NEW_VM_NAME ) );
	pParams->addEventParameter(
			new CVmEventParameter( PVE::String,
				pNewConfig->getVmIdentification()->getHomePath(),
				EVT_PARAM_DISP_TASK_CLONE_NEW_VM_ROOT_DIR ) );
	pParams->addEventParameter(
			new CVmEventParameter( PVE::Boolean,
				QString("%1").arg(isTemplate),
				EVT_PARAM_DISP_TASK_CLONE_AS_TEMPLATE ) );
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
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
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

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), pCmd->GetVmUuid());
	if (!pConfig) {
		WRITE_TRACE(DBG_FATAL, "Unable to find CT by uuid %s",
				QSTR2UTF8(pCmd->GetVmUuid()));
		return PRL_ERR_VM_GET_CONFIG_FAILED;
	}

	if (disk.getUserFriendlyName().isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Unable to create disk: empty path");
		return PRL_ERR_BAD_PARAMETERS;
	}

	QString sPath = getFullPath(pConfig->getVmIdentification()->getHomePath(),
			disk.getUserFriendlyName());
	if (QFileInfo(sPath).exists()) {
		WRITE_TRACE(DBG_FATAL, "Disk image [%s] is already exist.", QSTR2UTF8(sPath));
		getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
					sPath,
					EVT_PARAM_MESSAGE_PARAM_0));
		return PRL_ERR_HDD_IMAGE_IS_ALREADY_EXIST;
	}

	/* Mbytes -> Bytes */
	quint64 size = (quint64) disk.getSize() << 20;

	return get_op_helper()->create_disk_image(sPath, size);
}

PRL_RESULT Task_VzManager::resize_env_disk()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmResizeDiskImageCommand *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmResizeDiskImageCommand>(cmd);

	QString sUuid = pCmd->GetVmUuid();
	unsigned int nNewSize = pCmd->GetSize();
	bool infoMode = (pCmd->GetFlags() & PRIF_DISK_INFO);

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

	SmartPtr<CVmConfiguration> x = getVzHelper()->getCtConfig(getClient(), sUuid);
	if (x.isValid())
	{
		getVzHelper()->getConfigCache()
			.remove(x->getVmIdentification()->getHomePath());
	}
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
		VIRTUAL_MACHINE_STATE state;
		if (PRL_SUCCEEDED(CVzHelper::get_env_status(getVmUuid(), state)))
			s->onVmStateChanged(state, m_nState, getVmUuid(), m_sVzDirUuid, false);
	}

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), getVmUuid());
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	unsigned int ramSize = pConfig->getVmHardwareList()->getMemory()->getRamSize();

	if ((m_nState == VMS_RUNNING) || (m_nState == VMS_RESUMING)) {
		adjustReservedMemLimit((long long)ramSize);

		CFirewallHelper fw(pConfig);
		res = fw.Execute();

		/* will ignore start VNC server error */
		start_vnc_server(getVmUuid(), true);
	} else if ((m_nState == VMS_STOPPED) || (m_nState == VMS_SUSPENDED)) {
		CFirewallHelper fw(pConfig, true);
		res = fw.Execute();

		/* will ignore start VNC server error */
		stop_vnc_server(getVmUuid(), true);
		adjustReservedMemLimit(-(long long)ramSize);
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
	if (!buffer.open( QIODevice::WriteOnly)) {
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

	CProtoCreateSnapshotCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateSnapshotCommand>(cmd);

	res = get_op_helper()->create_env_snapshot(cmd->GetVmUuid(), sSnapUuid,
			pCmd->GetName(), pCmd->GetDescription());
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
	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), sVmUuid);
	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;
	res = get_op_helper()->switch_env_snapshot(sVmUuid, pCmd->GetSnapshotUuid(),
			pCmd->GetCommandFlags());
	if (PRL_SUCCEEDED(res)) {
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

	SmartPtr<CVmConfiguration> pVmConfig = getVzHelper()->getCtConfig(getClient(), sCtUuid);
	if( !pVmConfig )
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	CVmRemoteDisplay *remDisplay = pVmConfig->getVmSettings()->getVmRemoteDisplay();

	PRL_VM_REMOTE_DISPLAY_MODE mode = remDisplay->getMode();
	if ( mode == PRD_DISABLED ) {
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

	PRL_RESULT e = x->Start(QString(vncServerApp), CVzHelper::get_ctid_by_uuid(sCtUuid), remDisplay,
				rdConfig->getBasePort());
	if (PRL_FAILED(e))
		return e;
	if (!getVzHelper()->addCtVNCServer(sCtUuid, x))
	{
		WRITE_TRACE(DBG_FATAL, "VNC server for Container UUID %s already started", QSTR2UTF8(sCtUuid));
		return PRL_ERR_VNC_SERVER_ALREADY_STARTED;
	}
	if ( mode == PRD_AUTO ) {
		// send event to GUI for changing the config params
		CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, sCtUuid, PIE_DISPATCHER );
		SmartPtr<IOPackage> pkgNew =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage() );
		CDspService::instance()->getClientManager().sendPackageToAllClients( pkgNew );
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

PRL_RESULT Task_VzManager::process_cmd()
{
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
	case PVE::DspCmdVmRestartGuest:
		ret = restart_env();
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
		if ( PRL_FAILED( getLastErrorCode() ) )
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
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode( ret );

	return ret;
}

void Task_VzManager::finalizeTask()
{
	if (!m_pResponseCmd.isValid())
		return;

	PRL_RESULT res = getLastErrorCode();
	// Send response
	if ( PRL_FAILED( res ) ) {
		if (res == PRL_ERR_VZ_OPERATION_FAILED) {
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, get_op_helper()->get_error_msg(),
					EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::UnsignedInt,
					QString::number(get_op_helper()->get_rc()),
					EVT_PARAM_OP_RC));
		}
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	} else {
		getClient()->sendResponse( m_pResponseCmd, getRequestPackage() );
	}
}

PRL_RESULT Task_VzManager::move_env()
{
	PRL_RESULT res;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	CProtoVmMoveCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMoveCommand>(cmd);

	QString sUuid = pCmd->GetVmUuid();
	QString sNewHome = pCmd->GetNewHomePath();

	if (sNewHome.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "New container home path path is empty");
		return PRL_ERR_INVALID_PARAM;
	}
	res = check_env_state(PVE::DspCmdDirVmMove, sUuid);
	if (PRL_FAILED(res))
		return res;

	SmartPtr<CVmConfiguration> pConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
	if (!pConfig) {
		WRITE_TRACE(DBG_FATAL, "Can not get container ID for UUID %s", QSTR2UTF8(sUuid));
		return PRL_ERR_CT_NOT_FOUND;
	}

	QString sName = pConfig->getVmIdentification()->getVmName();
	CVmDirectory::TemporaryCatalogueItem vmInfo( sUuid, QString(), sName );

	res = CDspService::instance()->getVmDirManager()
			.lockExistingExclusiveVmParameters(m_sVzDirUuid, &vmInfo);
	if (PRL_FAILED(res))
	{
		switch (res)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(vmInfo.vmXmlPath));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, vmInfo.vmName, EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, vmInfo.vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
			break;
		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				QSTR2UTF8(vmInfo.vmUuid), QSTR2UTF8(vmInfo.vmName), QSTR2UTF8(vmInfo.vmXmlPath));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, vmInfo.vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, vmInfo.vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, vmInfo.vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
		}
		return res;
	}

	res = get_op_helper()->move_env(sUuid, sNewHome, sName);

	// Get updated config
	if (PRL_SUCCEEDED(res)) {
		pConfig = getVzHelper()->getCtConfig(getClient(), sUuid);
		if (!pConfig) {
			WRITE_TRACE(DBG_FATAL, "Can not get container ID for UUID %s after move",
				QSTR2UTF8(sUuid));
			res = PRL_ERR_CT_NOT_FOUND;
		}
	}

	// Update Vm directory item
	if (PRL_SUCCEEDED(res)) {

		CDspLockedPointer< CVmDirectoryItem >
			pVmDirItem = CDspService::instance()->getVmDirManager()
			.getVmDirItemByUuid(m_sVzDirUuid, sUuid );

		if (!pVmDirItem) {
			WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem by vmUuid = %s",
					QSTR2UTF8(sUuid));
		} else {
			QString newConfPath = pConfig->getVmIdentification()->getHomePath() +
				"/" + VMDIR_DEFAULT_VM_CONFIG_FILE;
			pVmDirItem->setVmHome(newConfPath);
			pVmDirItem->setCtId(pConfig->getVmIdentification()->getCtId());
			res = CDspService::instance()->getVmDirManager().updateVmDirItem(pVmDirItem);
			if (PRL_FAILED(res) )
				WRITE_TRACE(DBG_FATAL, "Can't update Container %s VmCatalogue by error: %s",
					QSTR2UTF8(sUuid), PRL_RESULT_TO_STRING(res));
		}
	}

	// Delete temporary registration
	CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(&vmInfo);

	if (PRL_FAILED(res))
		return res;

	getResponseCmd()->SetVmConfig(pConfig->toString());

	// Set some parameters in the response
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();
	pParams->addEventParameter(
			new CVmEventParameter( PVE::String,
				pConfig->getVmIdentification()->getVmUuid(),
				EVT_PARAM_DISP_TASK_VM_UUID ) );
	pParams->addEventParameter(
			new CVmEventParameter( PVE::String,
				pConfig->getVmIdentification()->getHomePath(),
				EVT_PARAM_DISP_TASK_MOVE_NEW_HOME_PATH) );

	// send event to GUI for changing the config params
	CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, sUuid, PIE_DISPATCHER );
	SmartPtr<IOPackage> pkg =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage() );
	CDspService::instance()->getClientManager().sendPackageToAllClients( pkg );
	return res;
}

PRL_RESULT Task_VzManager::send_network_settings()
{
	PRL_RESULT res = PRL_ERR_UNIMPLEMENTED;
#if 0
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(getRequestPackage());
	if (!cmd->IsValid())
		return PRL_ERR_UNRECOGNIZED_REQUEST;

	QString sUuid = cmd->GetVmUuid();
	CProtoCommandDspWsResponse *pResponseCmd = getResponseCmd();
	SmartPtr<CHostHardwareInfo> pHostHwInfo(new CHostHardwareInfo);

	res = getVzHelper()->getVzVNetHelper().GetIPAddrs(sUuid,
				pHostHwInfo->m_lstNetworkAdapters);
	if (PRL_FAILED(res))
		return res;

	pResponseCmd->AddStandardParam(pHostHwInfo->toString());;

	SmartPtr<IOPackage> pkg =
		DispatcherPackage::createInstance(PVE::DspWsResponse, pResponseCmd, getRequestPackage());
	getClient()->sendPackage(pkg);
#endif
	return res;
}

PRL_RESULT Task_VzManager::send_problem_report()
{
	CDspProblemReportHelper::getProblemReport( getClient(), getRequestPackage() );
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VzManager::adjustReservedMemLimit(long long delta)
{
	CDspLockedPointer<CDispCommonPreferences> lpCommonPrefs =
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
	CDspService::instance()->getVzHelper()->adjustTotalRunningCtMemory(delta);
	return PRL_ERR_SUCCESS;
}

