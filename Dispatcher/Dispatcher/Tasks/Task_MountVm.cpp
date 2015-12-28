///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MountVm.cpp
///
/// Dispatcher task for mounting Hdd files
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

#include <prlcommon/Std/PrlAssert.h>
#include <Libraries/ProtoSerializer/CProtoSerializer.h>
#include <Libraries/PrlCommonUtils/CFileHelper.h>
#include <prlcommon/PrlUuid/Uuid.h>

#include "Task_MountVm.h"
#include "Task_CommonHeaders.h"
#include "CDspVmMounter.h"
#include "CDspLibvirt.h"
#include "CDspVzHelper.h"

using namespace Parallels;
// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


Task_MountVm::Task_MountVm(
		SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &p,
		PVE::IDispatcherCommands nCmd, SmartPtr<CDspVmMountRegistry> &registry)
	: CDspTaskHelper(pUser, p)
	, m_nCmd(nCmd), m_registry(registry)
{}


QString Task_MountVm::getVmUuid()
{
	if (m_pVmConfig)
		return m_pVmConfig->getVmIdentification()->getVmUuid();
	else
		return "";
}

PRL_RESULT Task_MountVm::prepareTask()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;

	try
	{
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(m_nCmd,
				UTF8_2QSTR(getRequestPackage()->buffers[0].getImpl()));
		if (!pCmd->IsValid())
			throw PRL_ERR_UNRECOGNIZED_REQUEST;


		m_nFlags = pCmd->GetCommandFlags();
		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
				getClient(), pCmd->GetVmUuid(), res);
		if (!m_pVmConfig)
		{
			if (!PRL_FAILED(res))
				res = PRL_ERR_VM_GET_CONFIG_FAILED;
			throw res;
		}
		res = CDspService::instance()->getAccessManager().checkAccess(getClient(), m_nCmd);
		if (PRL_FAILED(res))
			throw res;

		if (m_sMountPoint.isEmpty()) {
			int ret = CDspService::instance()->getVzHelper()->
				getVzlibHelper().get_vz_config_param("VE_ROOT", m_sMountPoint);
			if (ret != 0)
				m_sMountPoint = "/vz/root/$VEID";
			QString uuid = Uuid(getVmUuid()).toStringWithoutBrackets();
			m_sMountPoint.replace("$VEID", uuid);
		}
		if (!CFileHelper::WriteDirectory(m_sMountPoint, &getClient()->getAuthHelper())) {
			// Maybe it remains from mount on crashed dispatcher?
			CDspVmMount::releaseMountpoint(m_sMountPoint);
			if (!CFileHelper::WriteDirectory(m_sMountPoint, &getClient()->getAuthHelper()))
				throw PRL_ERR_MAKE_DIRECTORY;
		}
	}
	catch (PRL_RESULT code)
	{
		res = code;
		setLastErrorCode(res);
		WRITE_TRACE(DBG_FATAL, "Error occurred on Task_MountVm prepare [%#x][%s]",
				code, PRL_RESULT_TO_STRING( code));
	}

	return res;
}

PRL_RESULT Task_MountVm::run_body()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;
	try
	{
		switch(m_nCmd) {
		case PVE::DspCmdVmUmount :
			res = UmountVm();
			break;
		case PVE::DspCmdVmMount:
			if (m_nFlags & PMVD_INFO) {
				MountVmInfo();
				res = PRL_ERR_SUCCESS;
			} else
				res = MountVm();
			break;
		default:
			res = PRL_ERR_UNEXPECTED;
			break;
		}
		if (PRL_FAILED(res))
			throw res;
	}
	catch (PRL_RESULT code)
	{
		res = code;
		setLastErrorCode(res);
		WRITE_TRACE(DBG_FATAL, "Error occurred on Task_MountVm [%#x][%s]",
				code, PRL_RESULT_TO_STRING( code));
	}

	return res;
}

void Task_MountVm::finalizeTask()
{
	PRL_RESULT res = getLastErrorCode();

	if( PRL_SUCCEEDED( res ) )
	{
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(
				getRequestPackage(), PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse *pResponseCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

		pResponseCmd->AddStandardParam(m_sMountInfo);
		getClient()->sendResponse( pCmd, getRequestPackage() );
	}
	else
	{
		getClient()->sendResponseError(getLastError(), getRequestPackage());
	}
}


PRL_RESULT Task_MountVm::UmountVm()
{
	QString sVmHome = CDspVmDirManager::getVmHomeByUuid(getVmIdent());
	WRITE_TRACE(DBG_FATAL, "Unmounting mnt='%s' home='%s'",
			QSTR2UTF8(m_sMountPoint), QSTR2UTF8(sVmHome));

	Libvirt::Result result = m_registry->umount(getVmUuid());
	if (result.isFailed()) {
		return CDspTaskFailure(*this)
			(result.error().convertToEvent(EVT_PARAM_MESSAGE_PARAM_0));
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_MountVm::MountVm()
{
	QString sVmHome = CDspVmDirManager::getVmHomeByUuid(getVmIdent());

	WRITE_TRACE(DBG_FATAL, "Mounting home='%s' mnt='%s'",
			QSTR2UTF8(sVmHome), QSTR2UTF8(m_sMountPoint));

	/* Deny rw for running or suspended. */
	VIRTUAL_MACHINE_STATE s;
	if (Libvirt::Kit.vms().at(getVmUuid()).getState(s).isFailed())
		return PRL_ERR_FAILURE;
	if (VMS_RUNNING == s && !(m_nFlags & PMVD_READ_ONLY)) {
		WRITE_TRACE(DBG_FATAL, "Cannot mount running VM in read-write mode");
		return PRL_ERR_DISP_VM_IS_NOT_STOPPED;
	}
	if (VMS_SUSPENDED == s && !(m_nFlags & PMVD_READ_ONLY)) {
		WRITE_TRACE(DBG_FATAL, "Cannot mount suspended VM in read-write mode");
		return PRL_ERR_CANT_EDIT_SUSPENDED_VM;
	}

	Libvirt::Result result = m_registry->mount(
			m_pVmConfig, getClient()->getVmDirectoryUuid(),
			m_sMountPoint, (bool)(m_nFlags & PMVD_READ_ONLY));

	if (result.isFailed()) {
		return CDspTaskFailure(*this)
			(result.error().convertToEvent(EVT_PARAM_MESSAGE_PARAM_0));
	}
	return PRL_ERR_SUCCESS;
}

void Task_MountVm::MountVmInfo()
{
	m_sMountInfo = m_registry->getInfo(getVmUuid());
}
