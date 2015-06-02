////////////////////////////////////////////////////////////////////////////////
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
/// @author sergeyt
///	igor@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_ChangeSID.h"
#include "Task_CommonHeaders.h"

#include "CProtoSerializer.h"
#include "CDspClientManager.h"
#include "Libraries/HostUtils/HostUtils.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"
#define WAITTOOLS_TIMEOUT	(5 * 60 * 1000)
#define WAITINTERVAL		(2 * 1000)

Task_ChangeSID::Task_ChangeSID(
		const SmartPtr<CDspClient> &user,
		const SmartPtr<IOPackage> &p,
		const SmartPtr<CVmConfiguration> &pVmConfig,
		bool bStandAlone) :

	CDspTaskHelper(user, p),
	m_pVmConfig(pVmConfig),
	m_bStandAlone(bStandAlone)
{
	setLastErrorCode(PRL_ERR_SUCCESS);
}

Task_ChangeSID::~Task_ChangeSID()
{
}

QString Task_ChangeSID::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_ChangeSID::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

		if (!IS_OPERATION_SUCCEEDED(m_pVmConfig->m_uiRcInit))
			throw PRL_ERR_PARSE_VM_CONFIG;

		if (!(m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType() == PVS_GUEST_TYPE_WINDOWS &&
		      m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion() >= PVS_GUEST_VER_WIN_XP))
			throw PRL_ERR_CHANGESID_NOT_SUPPORTED;

		if (CDspVm::getVmToolsState( getVmUuid(), getClient()->getVmDirectoryUuid() ) == PTS_NOT_INSTALLED)
			throw PRL_ERR_CHANGESID_GUEST_TOOLS_NOT_AVAILABLE;

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		getLastError()->setEventCode(code);
		WRITE_TRACE(DBG_FATAL, "Error occurred on Task_ChangeSID prepare [%#x][%s]",
			code, PRL_RESULT_TO_STRING(code));
	}
	setLastErrorCode(ret);
	return ret;
}

void Task_ChangeSID::finalizeTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

	} catch (PRL_RESULT code) {
		ret = code;

		WRITE_TRACE(DBG_FATAL, "Error occurred on ChangeSID [%#x][%s]",
			code, PRL_RESULT_TO_STRING(code));
	}
	setLastErrorCode(ret);

	if ( m_bStandAlone )//Task was created as separate task so should send response to the client itself
	{
		if ( PRL_FAILED( getLastErrorCode() ) )
			getClient()->sendResponseError( getLastError(), getRequestPackage() );
		else
			getClient()->sendSimpleResponse( getRequestPackage(), PRL_ERR_SUCCESS );
	}
}

PRL_RESULT Task_ChangeSID::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	bool flgImpersonated = false;
	bool bVmStarted = false;
	bool bVmLocked = false;

	SmartPtr<CDspVm> pVm;
	SmartPtr<CDspClient> pFakeSession(new CDspClient(Uuid::createUuid(), "fake-user" ));
	try
	{
		if (!pFakeSession->getAuthHelper().AuthUserBySelfProcessOwner())
		{
			WRITE_TRACE(DBG_FATAL, "Failed to authorize fake user session");
			throw (PRL_ERR_LOCAL_AUTHENTICATION_FAILED);
		}

		// Lock Vm
		pFakeSession->setVmDirectoryUuid(getClient()->getVmDirectoryUuid());
		ret = CDspService::instance()->getVmDirHelper().lockVm(getVmUuid(), pFakeSession, getRequestPackage());
		if (PRL_FAILED(ret))
		{
			WRITE_TRACE(DBG_FATAL, "failed to lock Vm");
			throw ret;
		}
		bVmLocked = true;

		// Let current thread impersonate the security context of a logged-on user
		if (!getClient()->getAuthHelper().Impersonate())
		{
			getLastError()->setEventCode(PRL_ERR_IMPERSONATE_FAILED);
			throw PRL_ERR_IMPERSONATE_FAILED;
		}
		flgImpersonated = true;

		// temporary Vm conf
		SmartPtr<CVmConfiguration> tmpVmConfig(new CVmConfiguration(m_pVmConfig.getImpl()));
		// Disable devices
		tmpVmConfig->getVmHardwareList()->m_lstNetworkAdapters.clear();
		tmpVmConfig->getVmHardwareList()->m_lstOpticalDisks.clear();
		tmpVmConfig->getVmHardwareList()->m_lstSoundDevices.clear();
		tmpVmConfig->getVmHardwareList()->m_lstFloppyDisks.clear();
		tmpVmConfig->getVmHardwareList()->m_lstSerialPorts.clear();
		tmpVmConfig->getVmHardwareList()->m_lstParallelPorts.clear();
		/* This trick allows to preserve session uuid to start Vm under lockVm()
		 * the session uuid is changed in
		 * CDspVm::startVmProcess()
		 *   CDspVmDirHelper::getVmStartUser()
		 */
		tmpVmConfig->getVmSettings()->getVmStartupOptions()->setVmStartLoginMode(PLM_START_ACCOUNT);

		AutoUpdate *pAutoUpdate = tmpVmConfig->getVmSettings()->getVmTools()->getAutoUpdate();
		if (pAutoUpdate)
			pAutoUpdate->setEnabled(false);

		// allow to start template
		tmpVmConfig->getVmSettings()->getVmCommonOptions()->setTemplate(false);

		ret = save_config(tmpVmConfig);
		if (PRL_FAILED(ret))
			throw ret;

		jobProgressEvent(1);
		ret = start_vm(pFakeSession, pVm);
		if (PRL_FAILED(ret))
			throw ret;
		bVmStarted = true;

		ret = change_sid(pVm);
		if (PRL_FAILED(ret))
			throw ret;

		jobProgressEvent(90);
		ret = stop_vm(pFakeSession, pVm, true);
		if (PRL_FAILED(ret))
			throw ret;

		jobProgressEvent(100);
		bVmStarted = false;

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred on ChangeSID [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}
	// Restore original config
	save_config(m_pVmConfig);

	if (flgImpersonated && !getActualClient()->getAuthHelper().RevertToSelf())
		WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

	if (bVmStarted)
		stop_vm(pFakeSession, pVm, m_bStandAlone ? true : (ret == PRL_ERR_SUCCESS));

	if (bVmLocked)
		CDspService::instance()->getVmDirHelper().unlockVm(getVmUuid(), pFakeSession, getRequestPackage());

	setLastErrorCode(ret);

	return ret;
}

void Task_ChangeSID::jobProgressEvent(unsigned int progress)
{
	CVmEvent event( PET_DSP_EVT_JOB_PROGRESS_CHANGED,
			getVmUuid(),
			PIE_UNKNOWN);
	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
				QString::number(progress),
				EVT_PARAM_PROGRESS_CHANGED));
	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

	CDspService::instance()->getClientManager().sendPackageToVmClients( p,
			getClient()->getVmDirectoryUuid(), getVmUuid());
}

PRL_RESULT Task_ChangeSID::save_config(SmartPtr<CVmConfiguration> &pVmConfig)
{
	PRL_RESULT ret;

	CDspLockedPointer< CVmDirectoryItem >
		pVmDirItem = CDspService::instance()->getVmDirManager()
		.getVmDirItemByUuid(getClient()->getVmDirectoryUuid(), getVmUuid());
	if (!pVmDirItem)
		return PRL_ERR_VM_UUID_NOT_FOUND;

	ret = CDspService::instance()->getVmConfigManager().saveConfig(pVmConfig,
			pVmDirItem->getVmHome(),
			getClient(),
			true,
			true);
	if (PRL_FAILED(ret))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to save configuration of the VM to file %s. Reason: %ld: %s",
				pVmConfig->getOutFileName().toUtf8().data(),
				Prl::GetLastError(),
				QSTR2UTF8( Prl::GetLastErrorAsString())
			   );
		return ret;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ChangeSID::start_vm(SmartPtr<CDspClient> &pFakeSession, SmartPtr<CDspVm> &pVm)
{
	PRL_RESULT ret;

	bool bVmInstanceCreated = false;
	pVm = CDspVm::CreateInstance(
			getVmUuid(),
			pFakeSession->getVmDirectoryUuid(),
			ret, bVmInstanceCreated, pFakeSession, PVE::DspCmdVmStart );

	if (pVm.getImpl() == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "Can't create instance of Vm '%s' "
				"which belongs to '%s' VM dir by error %#x",
				QSTR2UTF8(getVmUuid()),
				QSTR2UTF8(pFakeSession->getVmDirectoryUuid()),
				ret);

		return ret;
	}

	// Start VM
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoBasicVmCommand(PVE::DspCmdVmStart, pVm->getVmUuid());
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmStart, pCmd);
	if (!pVm->start(pFakeSession, pPackage))
	{
		if (bVmInstanceCreated)
			CDspVm::UnregisterVmObject(pVm);
		WRITE_TRACE(DBG_FATAL, "Vm start failed");
		return PRL_ERR_CHANGESID_VM_START_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ChangeSID::stop_vm(SmartPtr<CDspClient> &pFakeSession, SmartPtr<CDspVm> &pVm, bool bGraceful)
{
	if (!pVm.isValid())
	{
		pVm = CDspVm::GetVmInstanceByUuid(getVmUuid(), pFakeSession->getVmDirectoryUuid());
		if (!pVm.isValid())
			return PRL_ERR_SUCCESS;
	}
	PRL_UINT32 nStopMode = (bGraceful ? PSM_SHUTDOWN : PSM_KILL) | (m_bStandAlone ? PSF_NOFORCE : 0);
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoVmCommandStop(getVmUuid(), nStopMode, PSF_FORCE);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmStop, pCmd);

	pVm->stop(pFakeSession, pPackage, nStopMode, true);

	// Hack to get STOPPED state
	pVm = SmartPtr<CDspVm>();
	for (unsigned int i = 0; i < WAITTOOLS_TIMEOUT / WAITINTERVAL; i++)
	{
		if (operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		VIRTUAL_MACHINE_STATE state = CDspVm::getVmState(getVmUuid(), pFakeSession->getVmDirectoryUuid());
		if (state == VMS_STOPPED)
			break;
		HostUtils::Sleep(WAITINTERVAL);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ChangeSID::change_sid(SmartPtr<CDspVm> &pVm)
{
	int vmToolsState = PTS_UNKNOWN;
	bool bStarted = false;
	unsigned int i;

	WRITE_TRACE(DBG_FATAL, "Wait for tools...");
	jobProgressEvent(5);
	for (i = 0; i < WAITTOOLS_TIMEOUT / WAITINTERVAL; i++)
	{
		HostUtils::Sleep(WAITINTERVAL);
		if (operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		vmToolsState = CDspVm::getVmToolsState( pVm->getVmIdent() );
		if (vmToolsState == PTS_INSTALLED || vmToolsState == PTS_OUTDATED)
		{
			WRITE_TRACE(DBG_WARNING, "Tools started");
			break;
		}

		// handle Vm start failure
		VIRTUAL_MACHINE_STATE state = pVm->getVmState();
		if (state == VMS_RUNNING || state == VMS_STARTING)
		{
			bStarted = true;
		}
		else if (bStarted && state == VMS_STOPPING)
		{
			WRITE_TRACE(DBG_FATAL, "Vm start failed, state changed to stopped");
			return PRL_ERR_CHANGESID_VM_START_FAILED;
		}
	}

	jobProgressEvent(50);
	for (; i < WAITTOOLS_TIMEOUT / WAITINTERVAL; i++)
	{
		PRL_RESULT ret = run_changeSID_cmd(pVm);
		if (PRL_SUCCEEDED(ret) ||
				ret != PRL_ERR_CHANGESID_NOT_AVAILABLE)
			return ret;

		HostUtils::Sleep(WAITINTERVAL);
	}

	return PRL_ERR_CHANGESID_GUEST_TOOLS_NOT_AVAILABLE;
}

PRL_RESULT Task_ChangeSID::run_changeSID_cmd(SmartPtr<CDspVm> &pVm)
{
	CProtoCommandPtr pCmd = CProtoSerializer::CreateBasicVmGuestProtoCommand(PVE::DspCmdVmGuestChangeSID,
			getVmUuid(), QString());
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmGuestChangeSID, pCmd);


	/* run prl_newsid aplication in the the VM and get result */
	IOSendJob::Handle hJob = pVm->sendPackageToVm(pPackage);
	if (!hJob.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "sendPackageToVm with commnad PVE::DspCmdVmGuestChangeSID failed");
		return PRL_ERR_CHANGESID_FAILED;
	}
	CDspService::instance()->getIOServer().waitForSend(hJob);
	/* change SID response loop */
	for (unsigned int i = 0; i < CHANGESID_TIMEOUT / WAITINTERVAL; i++)
	{
		if (operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;


		IOSendJob::Result nResult = CDspService::instance()->getIOServer().waitForResponse(hJob, WAITINTERVAL);
		if (nResult == IOSendJob::Timeout)
			continue;

		if (nResult == IOSendJob::Success)
		{
			IOSendJob::Response resp = CDspService::instance()->getIOServer().takeResponse(hJob);
			if (resp.responseResult != IOSendJob::Success)
			{
				WRITE_TRACE(DBG_FATAL, "TakeResponse failed: IOSendJob::Response=%d",
							resp.responseResult);
				return PRL_ERR_CHANGESID_FAILED;
			}

			SmartPtr<IOPackage> respPkg = resp.responsePackages[0];
			if (!respPkg.isValid())
			{
				WRITE_TRACE(DBG_FATAL, "response package in sot valid");
				return PRL_ERR_CHANGESID_FAILED;
			}
			CVmEvent responseEvent(UTF8_2QSTR(respPkg->buffers[0].getImpl()));
			if (responseEvent.getEventCode() != PRL_ERR_SUCCESS)
			{
				WRITE_TRACE(DBG_FATAL, "DspCmdVmGuestChangeSID: received response code=%d",
					responseEvent.getEventCode());
				if (responseEvent.getEventCode() == PRL_ERR_CHANGESID_NOT_AVAILABLE)
					return PRL_ERR_CHANGESID_NOT_AVAILABLE;
				else
					return PRL_ERR_CHANGESID_FAILED;
			}
			return PRL_ERR_SUCCESS;
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "failed to waitForResponse nResult=%d", nResult);
			return PRL_ERR_CHANGESID_FAILED;
		}
	}

	return PRL_ERR_CHANGESID_FAILED;
}

