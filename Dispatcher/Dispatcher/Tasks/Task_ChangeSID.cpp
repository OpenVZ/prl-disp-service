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

	Libvirt::Tools::Agent::Vm::Unit u = Libvirt::Kit.vms().at(getVmUuid());

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

		VIRTUAL_MACHINE_STATE state = VMS_UNKNOWN;
		
		Libvirt::Result e = u.getState(state);
		if (e.isFailed())
			throw e.error().code();

		jobProgressEvent(1);
		if (state != VMS_RUNNING) {
			e = u.start();
			if (e.isFailed())
				throw e.error().code();
			bVmStarted = true;
		}

		ret = change_sid(u);
		if (PRL_FAILED(ret))
			throw ret;

		if (bVmStarted) {
			jobProgressEvent(90);
			e = u.shutdown();
			if (e.isFailed())
				throw e.error().code();
			bVmStarted = false;
		}

		jobProgressEvent(100);

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
		!m_bStandAlone && PRL_FAILED(ret) ? u.kill() : u.shutdown();

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

PRL_RESULT Task_ChangeSID::change_sid(Libvirt::Tools::Agent::Vm::Unit& u)

{
	unsigned int i;

	WRITE_TRACE(DBG_DEBUG, "Wait for tools...");
	jobProgressEvent(5);
	for (i = 0; i < WAITTOOLS_TIMEOUT / WAITINTERVAL; i++)
	{
		if (operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		// handle Vm start failure
		VIRTUAL_MACHINE_STATE state = VMS_UNKNOWN;
		u.getState(state);
		if (state != VMS_RUNNING) {
			WRITE_TRACE(DBG_FATAL, "Vm start failed, state changed to stopped");
			return PRL_ERR_CHANGESID_VM_START_FAILED;
		}

		Prl::Expected<QString, Libvirt::Error::Simple> e =
			u.getGuest().getAgentVersion();
		if (e.isSucceed()) {
			WRITE_TRACE(DBG_DEBUG, "Tools ready");
			break;
		}

		HostUtils::Sleep(WAITINTERVAL);
	}

	jobProgressEvent(50);

	PRL_RESULT ret = run_changeSID_cmd(u);
	if (PRL_SUCCEEDED(ret) ||
			ret != PRL_ERR_CHANGESID_NOT_AVAILABLE)
		return ret;

	return PRL_ERR_CHANGESID_GUEST_TOOLS_NOT_AVAILABLE;
}

PRL_RESULT Task_ChangeSID::run_changeSID_cmd(Libvirt::Tools::Agent::Vm::Unit& u)
{
	Prl::Expected
		<Libvirt::Tools::Agent::Vm::Exec::Result,
			Libvirt::Error::Simple> e =
		u.getGuest().runProgram(
			Libvirt::Tools::Agent::Vm::Exec::Request("prl_newsid.exe",  
				QList<QString>(), QByteArray()));

	QString uuid;
	u.getUuid(uuid);

	if (e.isFailed()) {
		return CDspTaskFailure(*this)(e.error().convertToEvent());
	}

	if (e.value().exitcode != 0) {
		QString err = QString("prl_newsid return error %1 for VM '%2' message '%3'")
				.arg(e.value().exitcode)
				.arg(uuid)
				.arg(QString::fromUtf8(e.value().stdErr));

		WRITE_TRACE(DBG_FATAL, qPrintable(err));

		return CDspTaskFailure(*this)(PRL_ERR_CHANGESID_FAILED, err);
	}

	return PRL_ERR_SUCCESS;
}

