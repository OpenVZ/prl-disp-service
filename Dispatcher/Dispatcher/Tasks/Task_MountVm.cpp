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

#include <Libraries/Std/PrlAssert.h>
#include <Libraries/ProtoSerializer/CProtoSerializer.h>

//#include <MounterDaemonAPI/NamedObjects.h>  // VI commented out by request from CP team
//#include <MounterDaemonAPI/MounterDaemonAPI.h>  // VI commented out by request from CP team
//#include <MounterDaemonAPI/CommunicationHelpers.h>  // VI commented out by request from CP team
//#include <MounterDaemonAPI/LockFile.h>  // VI commented out by request from CP team

//#include <CommandProtocol/ConnectionManager.h>  // VI commented out by request from CP team
//#include <CommandProtocol/CommandDispatcher.h>  // VI commented out by request from CP team
//#include <CommandProtocol/CommunicatorIPCSock.h>  // VI commented out by request from CP team

#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Task_MountVm.h"
#include "Task_CommonHeaders.h"
#include "XmlModel/DispConfig/CDispMountPreferences.h"


using namespace Parallels;
// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


// VI commented out by request from CP team
//PRL_RESULT Task_MountVm::SendCommand(CCommandDispatcher& cmdDispatcher, CommandPtr cmd, QString &sOut)
//{
//	bool bOk;
//	CommandProtocol::CommandId nCmd = cmd->GetCommandId();
//	//DisableSignals();
//
//	bOk = cmdDispatcher.SendCommand(cmd);
//	if (!bOk)
//	{
//		//EnableSignals();
//		WRITE_TRACE(DBG_FATAL, "Can't send command to daemon");
//		return PRL_ERR_PERFORM_MOUNTER_COMMAND;
//	}
//
//	//////// receive answer
//	bOk = cmdDispatcher.WaitIncomingCommand();
//	//EnableSignals();
//	if (!bOk)
//	{
//		WRITE_TRACE(DBG_FATAL, "No response from daemon");
//		return PRL_ERR_PERFORM_MOUNTER_COMMAND;
//	}
//
//	// do the stuff
//	CommandPtr cmdReplyPtr;
//	cmdDispatcher.RecieveCommand(cmdReplyPtr);
//	if (cmdReplyPtr->GetCommandId() != ID_CMD_RESPONCE)
//	{
//		WRITE_TRACE(DBG_FATAL, "invalid command [%d] received from daemon",
//				cmdReplyPtr->GetCommandId());
//
//		return PRL_ERR_UNEXPECTED;
//	}
//
//	CResponceCommand *pCmd = static_cast<CResponceCommand*>(cmdReplyPtr.get());
//
//	// Handle List result
//	if (nCmd == CommandProtocol::ID_CMD_LIST)
//		sOut = pCmd->m_string;
//
//	if (PRL_FAILED(pCmd->m_prlErr))
//	{
//		WRITE_TRACE(DBG_FATAL, "mounter client: error=0x%08x %s",
//				pCmd->m_prlErr, QSTR2UTF8(pCmd->m_string));
//		sOut = pCmd->m_string;
//		// FIXME Convert error
//		if (nCmd == CommandProtocol::ID_CMD_MOUNT)
//			return PRL_ERR_VM_MOUNT;
//		if (nCmd == CommandProtocol::ID_CMD_UNMOUNT)
//			return PRL_ERR_VM_UNMOUNT;
//		else
//			return PRL_ERR_UNEXPECTED;
//	}
//
//	return PRL_ERR_SUCCESS;
//}

// VI commented out by request from CP team
//PRL_RESULT Task_MountVm::ProcessCommand(CommandPtr cmd, QString &sOut)
//{
//	// Locks
//	LockFilePtr lockFileBusy(CLockFile::CreateLockFile(MOUNTERDAEMON_BUSY_LOCKFILENAME));
//	LockFilePtr lockFileRunning(CLockFile::CreateLockFile(MOUNTERDAEMON_RUNNING_LOCKFILENAME));
//	if ( !lockFileBusy.get() || !lockFileRunning.get() )
//	{
//		WRITE_TRACE(DBG_FATAL, "Internal error: can't create lock file! "
//				"Check location & permissions for '%s' directory",
//				MOUNTERDAEMON_TEMP_DIRECTORY);
//		return PRL_ERR_PERFORM_MOUNTER_COMMAND;
//	}
//
//	lockFileBusy->Lock(); // wait while daemon is busy
//	if (lockFileRunning->TryLock())
//	{
//		WRITE_TRACE(DBG_FATAL, "Mounter daemon is not runnung.");
//		return PRL_ERR_CONNECT_TO_MOUNTER;
//	}
//
//	// Connect to daemon
//	CSock sock;
//	if (!ConnectToDaemon(sock))
//	{
//		WRITE_TRACE(DBG_FATAL, "Can't connect to mounter daemon.");
//		return PRL_ERR_CONNECT_TO_MOUNTER;
//	}
//
//	CommandFactoryPtr commandFactoryPtr( new CDaemonCommandFactory );
//	CommunicatorPtr communicatorPtr( new CCommunicatorIPCSock(sock) );
//	if (!commandFactoryPtr.get() || !communicatorPtr.get())
//		return PRL_ERR_MEMORY_ALLOC_ERROR;
//
//	CCommandDispatcher cmdDispatcher(communicatorPtr, commandFactoryPtr);
//	cmdDispatcher.StartProcessing();
//
//	PRL_RESULT res = SendCommand(cmdDispatcher, cmd, sOut);
//
//	//EnableSignals();
//	cmdDispatcher.StopProcessing();
//	cmdDispatcher.WaitProcessingFinished();
//
//	return res;
//}


Task_MountVm::Task_MountVm(SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &p, PVE::IDispatcherCommands nCmd )
	: CDspTaskHelper(pUser, p)
	, m_nCmd(nCmd)
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

		if (m_sMountPoint.isEmpty())
		{
			CDispMountPreferences *pPref =
				CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getMountPreferences();

			m_sMountPoint = pPref->getDefaultMountPath();
			if (m_sMountPoint.isEmpty())
				m_sMountPoint = "/vz/mnt/";
			 m_sMountPoint += getVmUuid();
		}
		if (!CFileHelper::WriteDirectory(m_sMountPoint, &getClient()->getAuthHelper()))
			throw PRL_ERR_MAKE_DIRECTORY;
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
			if (m_nFlags & PMVD_INFO)
				res = MountVmInfo();
			else
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

	if ((PRL_FAILED(res) && m_nCmd == PVE::DspCmdVmMount) ||
	    (PRL_SUCCEEDED(res) && m_nCmd == PVE::DspCmdVmUmount))
	{
		// Remove all submounts
		QDir cDir(m_sMountPoint);
		cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
		QFileInfoList cFileList = cDir.entryInfoList();
		for (int i = 0; i < cFileList.size(); i++)
		{
			QDir().rmdir(cFileList.at(i).filePath());
		}
		QDir().rmdir(m_sMountPoint);
	}

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
		getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
					m_sError,
					EVT_PARAM_MESSAGE_PARAM_0));

		getClient()->sendResponseError(getLastError(), getRequestPackage());
	}
}


PRL_RESULT Task_MountVm::UmountVm()
{
// VI commented out by request from CP team
//	// Prepare command
//	CUnmountCommand* pCmd = new CUnmountCommand;
//	CommandPtr cmdPtr(pCmd);
//
//	QString sVmHome = CDspVmDirManager::getVmHomeByUuid( getVmIdent() );
//
//	WRITE_TRACE(DBG_FATAL, "Unmounting mnt='%s' home='%s'",
//			QSTR2UTF8(m_sMountPoint), QSTR2UTF8(sVmHome));
//
//	pCmd->m_type = CUnmountCommand::UMNT_BY_FILEOBJECT_ALL;
//	pCmd->m_file = sVmHome;
//	pCmd->m_isConfig = true;
//	return ProcessCommand(cmdPtr, m_sError);
	return PRL_ERR_FAILURE;
}

PRL_RESULT Task_MountVm::UmountVmAll()
{
// VI commented out by request from CP team
//	// Prepare command
//	CUnmountCommand* pCmd = new CUnmountCommand;
//	CommandPtr cmdPtr(pCmd);
//
//	WRITE_TRACE(DBG_INFO, "Unmounting all");
//
//	pCmd->m_type = CUnmountCommand::UNMT_ALL;
//	QString sError;
//	return ProcessCommand(cmdPtr, sError);
	return PRL_ERR_FAILURE;
}

PRL_RESULT Task_MountVm::MountVm()
{
// VI commented out by request from CP team
//	// Prepare command
//	CMountCommand* pCmd = new CMountCommand;
//	CommandPtr cmdPtr(pCmd);
//
//	QString sVmHome = CDspVmDirManager::getVmHomeByUuid( getVmIdent() );
//
//	WRITE_TRACE(DBG_FATAL, "Mounting home='%s' mnt='%s'",
//			QSTR2UTF8(sVmHome), QSTR2UTF8(m_sMountPoint));
//
//	bool ro = false;
//
//	if (m_nFlags & PMVD_READ_ONLY)
//		ro = true;
//	pCmd->m_mountOpts.readOnly = ro;
//	// FIXME: deny rw for suspended
//	pCmd->m_mountOpts.writeSuspended = false;
//
//	pCmd->m_mountAll = true;
//	pCmd->m_isConfig = true;
//	pCmd->m_file = sVmHome;
//	// FIXME give mount point from parameters
//	pCmd->m_mountPoint = m_sMountPoint;
//
//	return ProcessCommand(cmdPtr, m_sError);
	return PRL_ERR_FAILURE;
}

PRL_RESULT Task_MountVm::MountVmInfo()
{
// VI commented out by request from CP team
//	// Prepare command
//	CListCommand* pCmd = new CListCommand;
//	CommandPtr cmdPtr(pCmd);
//
//	QString sVmHome = CDspVmDirManager::getVmHomeByUuid( getVmIdent() );
//
//	pCmd->m_action = CListCommand::LIST_VOLUMES;
//	pCmd->m_listBy = CListCommand::LIST_BY_FILEOBJECT;
//	pCmd->m_file = sVmHome;
//	pCmd->m_showInfo = true;
//	pCmd->m_isConfig = true;
//
//	return ProcessCommand(cmdPtr, m_sMountInfo);
	return PRL_ERR_FAILURE;
}
