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
/// @file
///	CDspShellHelper.cpp
///
/// @brief
///	Implementation of the class CDspShellHelper
///
/// @brief
///	This class implements various environment-related logic
///
/// @author sergeyt
///	SergeyM
///
/// @date
///	2006-04-04
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include "Build/Current.ver"

#include "Libraries/HostInfo/CHostInfo.h"

#ifdef _WIN_
#include "Libraries/WinUAC/ElevatedPriveleges/RunProcess.h"
#include "Libraries/WinSecurityChecks/SigningValidity/CheckCertificate.h"
#endif

#include "CDspCommon.h"
#include "CDspVm.h"
#include "CDspService.h"
#include "CDspShellHelper.h"
#include <prlxmlmodel/DispConfig/CDispWorkspacePreferences.h>
#include <prlxmlmodel/DispConfig/CDispVirtuozzoPreferences.h>
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/VmInfo/CVmInfo.h>
#include "CDspTaskHelper.h"
#include <prlxmlmodel/HostHardwareInfo/CHwFileSystemInfo.h>
#include "CProtoSerializer.h"
#include "CProtoCommands.h"
#include "CDspClientManager.h"
#include "CDspVmInfoDatabase.h"
#include "CDspVzLicense.h"

#include "Tasks/Task_UpdateCommonPrefs.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_GetFileSystemEntries.h"
#include "Tasks/Task_ConfigureGenericPci.h"
#include "Tasks/Task_PrepareForHibernate.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Tasks/Task_VmDataStatistic.h"

#include "CDspVzHelper.h"
#include "CDspVmNetworkHelper.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>

#include "Libraries/CpuFeatures/CCpuHelper.h"

#include <prlsdk/PrlApiDeprecated.h>

#include <prlcommon/Interfaces/ParallelsSdk.h>

#include <QMap>
#include <QMutex>

#include <typeinfo>
#include <algorithm>
#include <boost/bind.hpp>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

using namespace Parallels;

// constructor
CDspShellHelper::CDspShellHelper()
{
}

// destructor
CDspShellHelper::~CDspShellHelper()
{
}

#ifdef _WIN_
namespace {
PRL_RESULT LaunchOnBehalfOfSystem(const QString& launchString, HANDLE userHandle)
{
	QStringList params = launchString.split(USB_LIST_DIVIDER);
	if (params.count() < 1)
	{
		WRITE_TRACE(DBG_WARNING, "Incorrect command was sent");
		return PRL_ERR_BAD_PARAMETERS;
	}

	const QString filePath = params[0];
	if (!QFile(filePath).exists())
	{
		WRITE_TRACE(DBG_WARNING, "There is no file found at the specified location: %s",
			QSTR2UTF8(filePath));
		return PRL_ERR_FILE_NOT_FOUND;
	}

	// Lock the file until it is executed
	HANDLE hLock = CreateFileW(QSTR2UTF16(filePath),
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hLock)
		return PRL_ERR_FILE_NOT_FOUND;

	DWORD error;
	if (!CheckCertificateValidity(QSTR2UTF16(filePath), c_sParallelsSigner, &error))
	{
		WRITE_TRACE(DBG_WARNING, "The file %s is not signed with correct certificate (error = %d)",
			QSTR2UTF8(filePath), error);
		return PRL_ERR_FAILURE;
	}

	QString cmdLine = params.join(" ");
	DWORD execResult = RunProcessOnUserToken(
		userHandle,
		QSTR2UTF16(cmdLine), NULL);
	CloseHandle(hLock);
	if (ERROR_SUCCESS != execResult)
	{
		WRITE_TRACE(DBG_WARNING, "Failed to create process with win error: %d", execResult);
		return PRL_ERR_FAILURE;
	}
	return PRL_ERR_SUCCESS;
}
}
#endif // _WIN_

// cancel command
void CDspShellHelper::cancelOperation (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& pkg)
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! pCmd->IsValid() )
	{
		pUser->sendSimpleResponse(pkg, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}

	QString requestUuid = pCmd->GetFirstStrParam();

	QList< SmartPtr<CDspTaskHelper> >
		taskList = 	CDspService::instance()->getTaskManager().getTaskListByPacketUuid( requestUuid );
	if ( taskList.isEmpty() )
	{
		pUser->sendSimpleResponse(pkg, PRL_ERR_TASK_NOT_FOUND );
		return;
	}

	int nCount = 0;
	foreach( SmartPtr<CDspTaskHelper> pTask, taskList )
	{
		WRITE_TRACE(DBG_FATAL, "Going to cancel task '%s' with uuid = %s "
			, typeid( *pTask.getImpl() ).name()
			, QSTR2UTF8( pTask->getJobUuid().toString() )
			);

		if( pTask->getClient() == pUser )
		{
			WRITE_TRACE( DBG_FATAL, "Cancel was sent to task" );
			pTask->cancelOperation(pUser, pkg);
			nCount++;
		}
		else
			WRITE_TRACE( DBG_FATAL, "Cancel was NOT sent to task because task runner is different '%s'"
				, (!pTask->getClient())?"NULL":QSTR2UTF8(pTask->getClient()->getSessionUuid()) );
	}
	if(!nCount)
	{
		pUser->sendSimpleResponse(pkg, PRL_ERR_ACCESS_DENIED );
		return;
	}


	return;
}



/**
 * @brief Sends host hardware info.
 */
void CDspShellHelper::sendHostHardwareInfo (
  const SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{

	SmartPtr<CHostHardwareInfo> hw_info(0);
	CDspLockedPointer<CDspHostInfo> hostInfo =
		CDspService::instance()->getHostInfo();
	hostInfo->refresh();

	hw_info = SmartPtr<CHostHardwareInfo>(
		new CHostHardwareInfo(hostInfo->data()));

	// #440246: always add for client default CD-ROM device
	if (hw_info->m_lstOpticalDisks.isEmpty())
	{
		hw_info->m_lstOpticalDisks.prepend( new CHwGenericDevice(
													PDE_OPTICAL_DISK,
													PRL_DVD_DEFAULT_DEVICE_NAME,
													PRL_DVD_DEFAULT_DEVICE_NAME) );
	}

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse* hostInfoCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	hostInfoCmd->SetHostHardwareInfo( hw_info->toString() );
	SmartPtr<IOPackage> response =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );

	pUser->sendPackage( response );
}

// Sends directory entries
void CDspShellHelper::sendDirectoryEntries (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	//
	// IMPORTANT! This operation can potentially take a long time to complete, because
	// one can be a directory, which contains a huge amount of files and/or directories.
	// That is why we're starting it in a separate thread
	//

	// get source directory path

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	QString source_dir = cmd->GetFirstStrParam();

	// trying to detect file system type (not a specific format (NTFS or ext3 or whatever else),
	// just the fact if it is Windows or Unix-like file system)
	//
	unsigned int fs_type = QDir::rootPath().toLower().startsWith("/") ?
		PFS_UNIX_LIKE_FS : PFS_WINDOWS_LIKE_FS;

   // prepare output data container
   CHwFileSystemInfo* fs_info = new CHwFileSystemInfo( fs_type );
   // prepare long running task helper
	CDspService::instance()->getTaskManager()
		.schedule(new Task_GetFileSystemEntries( pUser, p, source_dir, fs_info));
}


// Sends disk list
void CDspShellHelper::sendDiskEntries (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	// trying to detect file system type (not a specific format (NTFS or ext3 or whatever else),
	// just the fact if it is Windows or Unix-like file system)
	//
	unsigned int fs_type = QDir::rootPath().toLower().startsWith("/") ?
		PFS_UNIX_LIKE_FS : PFS_WINDOWS_LIKE_FS;

	// prepare output data container
	CHwFileSystemInfo* fs_info = new CHwFileSystemInfo( fs_type );

	// get disks entries
	Task_FileSystemEntriesOperations*
      task_helper = new Task_FileSystemEntriesOperations( pUser, p );
	PRL_RESULT i_rc = task_helper->getDisksEntries( fs_info );

	// check results
	if( IS_OPERATION_SUCCEEDED( i_rc ) )
	{
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* fsInfoCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		fsInfoCmd->SetHwFileSystemInfo( fs_info->toString() );
		SmartPtr<IOPackage> response =
			DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );

		pUser->sendPackage( response );
	}
	else
		pUser->sendResponseError( task_helper->getLastError(), p );

	// cleanup and exit
	delete fs_info;
	fs_info = NULL;

	delete task_helper;
	task_helper = NULL;

}


// Create directory
void CDspShellHelper::createDirectoryEntry (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	// get source directory path
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	QString target_dir = cmd->GetFirstStrParam();

	// trying to detect file system type (not a specific format (NTFS or ext3 or whatever else),
	// just the fact if it is Windows or Unix-like file system)
	//
	unsigned int fs_type = QDir::rootPath().toLower().startsWith("/") ?
		PFS_UNIX_LIKE_FS : PFS_WINDOWS_LIKE_FS;

	// prepare output data container
	CHwFileSystemInfo* fs_info = new CHwFileSystemInfo( fs_type );

	// create directory
	Task_FileSystemEntriesOperations*
      task_helper = new Task_FileSystemEntriesOperations( pUser, p );
	PRL_RESULT i_rc = task_helper->createDirectoryEntry( target_dir, fs_info );

	// check results
	if( IS_OPERATION_SUCCEEDED( i_rc ) )
	{
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* fsInfoCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		fsInfoCmd->SetHwFileSystemInfo( fs_info->toString() );
		SmartPtr<IOPackage> response =
			DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );

		pUser->sendPackage( response );
	}
	else
		pUser->sendResponseError( task_helper->getLastError(), p );

	// cleanup and exit
	delete fs_info;
	fs_info = NULL;

	delete task_helper;
	task_helper = NULL;

}


// Renames directory or file entry
void CDspShellHelper::renameFsEntry (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	// get source file to rename
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}

	CProtoFsRenameEntryCommand* renameCmd =
		CProtoSerializer::CastToProtoCommand<CProtoFsRenameEntryCommand>(cmd);

	QString oldFileName = renameCmd->GetOldEntryName();
	QString newFileName = renameCmd->GetNewEntryName();


	// trying to detect file system type (not a specific format (NTFS or ext3 or whatever else),
	// just the fact if it is Windows or Unix-like file system)
	//
	unsigned int fs_type = QDir::rootPath().toLower().startsWith("/") ?
		PFS_UNIX_LIKE_FS : PFS_WINDOWS_LIKE_FS;

	// prepare output data container
	CHwFileSystemInfo* fs_info = new CHwFileSystemInfo( fs_type );

	// rename directory or file entry
	Task_FileSystemEntriesOperations*
      task_helper = new Task_FileSystemEntriesOperations( pUser, p );
	PRL_RESULT i_rc = task_helper->renameFsEntry( oldFileName, newFileName, fs_info );

	// check results
	if( IS_OPERATION_SUCCEEDED( i_rc ) )
	{
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* fsInfoCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		fsInfoCmd->SetHwFileSystemInfo( fs_info->toString() );
		SmartPtr<IOPackage> response =
			DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );

		pUser->sendPackage( response );
	}
	else
		pUser->sendResponseError( task_helper->getLastError(), p );

	// cleanup and exit
	delete fs_info;
	fs_info = NULL;

	delete task_helper;
	task_helper = NULL;

}


// Removes directory or file entry
void CDspShellHelper::removeFsEntry (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	// get file name to remove
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	QString fileName = cmd->GetFirstStrParam();

	// trying to detect file system type (not a specific format (NTFS or ext3 or whatever else),
	// just the fact if it is Windows or Unix-like file system)
	//
	unsigned int fs_type = QDir::rootPath().toLower().startsWith("/") ?
		PFS_UNIX_LIKE_FS : PFS_WINDOWS_LIKE_FS;

	// prepare output data container
	CHwFileSystemInfo* fs_info = new CHwFileSystemInfo( fs_type );

	// remove directory or file entry
	Task_FileSystemEntriesOperations*
      task_helper = new Task_FileSystemEntriesOperations( pUser, p );
	PRL_RESULT i_rc = task_helper->removeFsEntry( fileName, fs_info );

	// check results
	if( IS_OPERATION_SUCCEEDED( i_rc ) )
	{
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* fsInfoCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		fsInfoCmd->SetHwFileSystemInfo( fs_info->toString() );
		SmartPtr<IOPackage> response =
			DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );

		pUser->sendPackage( response );
	}
	else
		pUser->sendResponseError( task_helper->getLastError(), p );

	// cleanup and exit
	delete fs_info;
	fs_info = NULL;

	delete task_helper;
	task_helper = NULL;

}

// Checks if an user can create a file
void CDspShellHelper::canCreateFile (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	// get file name to check creating
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	QString fileName = cmd->GetFirstStrParam();

	// trying to detect file system type (not a specific format (NTFS or ext3 or whatever else),
	// just the fact if it is Windows or Unix-like file system)
	//
	// unsigned int fs_type = QDir::rootPath().toLower().startsWith("/") ?
	//	PFS_UNIX_LIKE_FS : PFS_WINDOWS_LIKE_FS;
    //

	SmartPtr<Task_FileSystemEntriesOperations>
		task_helper( new Task_FileSystemEntriesOperations( pUser, p ) );
	PRL_RESULT i_rc = task_helper->canCreateFile( fileName );

	// check results
	if( IS_OPERATION_SUCCEEDED( i_rc ) )
		pUser->sendSimpleResponse( p, PRL_ERR_SUCCESS );
	else
		pUser->sendResponseError( task_helper->getLastError(), p );
}

// Generates entry name for specified directory
void CDspShellHelper::generateFsEntryName (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	// parse command package
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	CProtoCommandDspCmdFsGenerateEntryName
		*pDspCmdFsGenerateEntryNameCmd
			= CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdFsGenerateEntryName>(cmd);
	QString sDirPath = pDspCmdFsGenerateEntryNameCmd->GetDirPath();
	QString sFilenamePrefix = pDspCmdFsGenerateEntryNameCmd->GetFilenamePrefix();
	QString sFilenameSuffix = pDspCmdFsGenerateEntryNameCmd->GetFilenameSuffix();
	QString sIndexDelimiter = pDspCmdFsGenerateEntryNameCmd->GetIndexDelimiter();

	QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot;
	QStringList _dir_entries = QDir(sDirPath).entryList( filters );

	QString sGeneratedEntryName = Parallels::GenerateFilename(
										_dir_entries,
										sFilenamePrefix,
										sFilenameSuffix,
										sIndexDelimiter
										);
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);
	CProtoCommandDspWsResponse
		*pDspWsResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	pDspWsResponseCmd->AddStandardParam(sGeneratedEntryName);

	pUser->sendResponse( pResponse, p );
}


// Sends host common info
void CDspShellHelper::sendHostCommonInfo (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse* hostInfoCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

	SmartPtr<CDispCommonPreferences> pCommonPrefs( new CDispCommonPreferences(
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs().getPtr()
		) );

	//FIXME: bug #2242 [ SDK ] May be need to add support of  multiple VmDirectories to SDK.
	// change VmDirUuid to VmDirPath
	QString vmDirPath = CDspService::instance()->getVmDirManager().getVmDirectory(
		pUser->getVmDirectoryUuid())->getDefaultVmFolder();
	pCommonPrefs->getWorkspacePreferences()->setDefaultVmDirectory( vmDirPath );

	pCommonPrefs->getWorkspacePreferences()->setHeadlessModeEnabled(
		isHeadlessModeEnabled() );

#ifdef _CT_
	pCommonPrefs->getDispVirtuozzoPreferences()->setDefaultCtDirectory(
			CVzHelper::getVzPrivateDir() );
#endif
#ifdef _LIN_
	if (CDspService::isServerModePSBM())
	{
		std::auto_ptr<CDispCpuPreferences> cpuMask(CCpuHelper::get_cpu_mask());

		if (!cpuMask.get())
		{
			pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
			return;
		}

		std::auto_ptr<CCpuPoolInfo> poolInfo(CCpuHelper::getPoolInfo());
		if (!poolInfo.get())
		{
			pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
			return;
		}

		pCommonPrefs->setCpuPreferences(cpuMask.release());
		pCommonPrefs->setCpuPoolInfo(poolInfo.release());
	}
#endif

	//clean password in backup prefs
	pCommonPrefs->getBackupSourcePreferences()->setPassword(QString());

	SmartPtr<CParallelsNetworkConfig>
		pNetworkConfig( new CParallelsNetworkConfig(
			CDspService::instance()->getNetworkConfig().getPtr() )
			);

	// #436109 ( we store this values in vmdirectory list to prevent deadlock in CDspAccessManager::checkAccess() )
	pCommonPrefs->getLockedOperationsList()->setLockedOperations(
		CDspService::instance()->getVmDirManager().getCommonConfirmatedOperations());

	SmartPtr<CDispNetworkPreferences>
		pOldNetConfig = Task_ManagePrlNetService::convertNetworkConfig( pNetworkConfig );
	PRL_ASSERT( pOldNetConfig );
	if( pOldNetConfig )
		pCommonPrefs->setNetworkPreferences( new CDispNetworkPreferences( pOldNetConfig.getImpl() ) );

	// Patch UsbIdentity's list of associations to contains only ones for current user Vm directory
	foreach (CDispUsbIdentity * ui, pCommonPrefs->getUsbPreferences()->m_lstAuthenticUsbMap) {
		QMutableListIterator<CDispUsbAssociation*> i(ui->m_lstAssociations);
		while (i.hasNext())
		{
			CDispUsbAssociation * ua = i.next();
			if( ua->getDirUuid() != pUser->getVmDirectoryUuid() )
			{
				i.remove();
				delete ua;
			}
		}
	}

	hostInfoCmd->SetHostCommonInfo( pCommonPrefs->toString() , pNetworkConfig->toString() );

	SmartPtr<IOPackage> response =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );

	pUser->sendPackage( response );
}

void CDspShellHelper::sendHostStatistics (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	CDspStatCollectingThread::SendHostStatistics( pUser, p );
}

void CDspShellHelper::sendGuestStatistics (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(p);
	if (!pCmd->IsValid())
	{
		pUser->sendSimpleResponse(p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}

	// AccessCheck
	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	CVmEvent evt;
	rc = CDspService::instance()->getAccessManager().checkAccess( pUser, PVE::DspCmdVmGetStatistics
		, pCmd->GetVmUuid(), &bSetNotValid, &evt);
	if ( ! PRL_SUCCEEDED(rc) )
	{
		CDspVmDirHelper::sendNotValidState(pUser, rc, pCmd->GetVmUuid(), bSetNotValid);
		pUser->sendResponseError( evt, p );
		return;
	}

	if ( pCmd->GetCommandFlags() & PVMSF_HOST_DISK_SPACE_USAGE_ONLY )
	{
		return (void)CDspService::instance()->getTaskManager()
				.schedule(new Task_VmDataStatistic(pUser, p));
	}

	if ( pCmd->GetCommandFlags() & PVMSF_GUEST_OS_INFORMATION_ONLY )
	{
		CVmIdent vmIdent = MakeVmIdent(pCmd->GetVmUuid(), pUser->getVmDirectoryUuid());
		QString qsVmPath = QFileInfo(
							CDspService::instance()->getVmDirManager().getVmHomeByUuid( vmIdent )
							).path();

		SmartPtr<CVmInfo> pVmInfo = CDspVmInfoDatabase::readVmInfo(qsVmPath + "/"VM_INFO_FILE_NAME );
		if ( ! pVmInfo )
		{
			pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
			return;
		}

		CSystemStatistics vmStatistic;
		vmStatistic.m_GuestOsInformation = *pVmInfo->getGuestOsInformation();

		CProtoCommandPtr pResponse
			= CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);

		CProtoCommandDspWsResponse* pDspWsResponseCmd
			= CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		pDspWsResponseCmd->SetSystemStatistics( vmStatistic.toString() );

		pUser->sendResponse(pResponse, p);
		return;
	}

	CDspStatCollectingThread::SendVmGuestStatistics( pCmd->GetVmUuid(), pUser, p );
}

PRL_RESULT CDspShellHelper::checkAccessForHostCommonInfoEdit(SmartPtr<CDspClient>& pUser,
										 const SmartPtr<IOPackage>& p)
{
	//AccessCheck
	{ //LOCK
		CDspLockedPointer<CDispUser> dispUser = CDspService::instance()->getDispConfigGuard()
			.getDispUserByUuid( pUser->getUserSettingsUuid() );

		PRL_ASSERT( dispUser );
		if ( ! dispUser )
			return PRL_ERR_FAILURE;

		// addinfo #8036
		if ( ! pUser->getAuthHelper().isLocalAdministrator()
			&& ! dispUser->getUserAccess()->canChangeServerSettings()
			)
		{
			return PRL_ERR_USER_NO_AUTH_TO_EDIT_SERVER_SETTINGS;
		}

	}// UNLOCK

	//#436109 check for command confirmations
	PRL_RESULT err = CDspService::instance()->getAccessManager()
		.checkAccess( pUser, (PVE::IDispatcherCommands)p->header.type );
	if( PRL_FAILED(err) )
		return err;

	return PRL_ERR_SUCCESS;
}


void CDspShellHelper::hostCommonInfoBeginEdit (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{

	PRL_RESULT err = checkAccessForHostCommonInfoEdit( pUser, p );
	if( PRL_FAILED(err) )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to change common info by error [%#x][%s]"
			, err, PRL_RESULT_TO_STRING(err) );
		pUser->sendSimpleResponse( p, err );
		return;
	}

	CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()->lock();

	SmartPtr<CDispatcherConfig > pDispConfigPrev(
		new CDispatcherConfig( CDspService::instance()->getDispConfigGuard().getDispConfig().getPtr() ) );

#ifdef _LIN_
	if (CDspService::isServerModePSBM())
	{
		std::auto_ptr<CDispCpuPreferences> cpuMask(CCpuHelper::get_cpu_mask());
		if (!cpuMask.get())
		{
			pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
			return;
		}
		pDispConfigPrev->getDispatcherSettings()->getCommonPreferences()->setCpuPreferences(cpuMask.release());
	}
#endif

	CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()
		->registerBeginEdit( pUser->getClientHandle(), pDispConfigPrev );

	CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()->unlock();

	pUser->sendSimpleResponse( p, PRL_ERR_SUCCESS );
}

void CDspShellHelper::hostCommonInfoCommit (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	PRL_RESULT err = checkAccessForHostCommonInfoEdit( pUser, p );
	if( PRL_FAILED(err) )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to change common info by error [%#x][%s]"
			, err, PRL_RESULT_TO_STRING(err) );
		pUser->sendSimpleResponse( p, err );
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// get request parameters
	//////////////////////////////////////////////////////////////////////////

	// XML configuration of the VM to be edit
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	QString sCommonPrefs = cmd->GetFirstStrParam();

	// Prepare and start long running task helper
	CDspService::instance()->getTaskManager()
		.schedule(new Task_UpdateCommonPrefs( pUser, p, sCommonPrefs ));
}

void CDspShellHelper::managePrlNetService (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{

	PRL_RESULT err = checkAccessForHostCommonInfoEdit( pUser, p );
	if( PRL_FAILED(err) )
	{
	    WRITE_TRACE( DBG_FATAL, "Unable to configure network by error [%#x][%s]"
				, err, PRL_RESULT_TO_STRING(err) );
	    pUser->sendSimpleResponse( p, err );
	    return;
	}

	// Prepare and start long running task helper
	CDspService::instance()->getTaskManager()
		.schedule(new Task_ManagePrlNetService( pUser, p));
}

/**
* @brief Sends Parallels Network Service status.
*/
void CDspShellHelper::sendNetServiceStatus (
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	pUser->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
}

void CDspShellHelper::attachToLostTask(
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	QString taskUuid = cmd->GetFirstStrParam();

	SmartPtr< CDspTaskHelper > pTask = CDspService::instance()->getTaskManager()
		.findTaskByUuid( taskUuid );

	PRL_RESULT result = PRL_ERR_FIXME;
	try
	{
		if( ! pTask )
			throw PRL_ERR_TASK_NOT_FOUND;

		if( pTask->getClient()->getClientHandle() != pUser->getPrevSessionUuid() )
			throw PRL_ERR_ATTACH_TO_TASK_BAD_SESSION;

		// replace Client, and Packet  for  Task
		pTask->reassignTask( pUser, p );

		result = PRL_ERR_SUCCESS;
		WRITE_TRACE(DBG_FATAL, "Task with uuid=%s was reassigned to new session from prev_session %s"
			,taskUuid.toAscii().constData()
			,pUser->getPrevSessionUuid().toAscii().constData());
	}
	catch ( PRL_RESULT err )
	{
		result = err;
		WRITE_TRACE(DBG_FATAL, "Can't Attach to task[%s] by reason %d [%s]"
			, QSTR2UTF8( taskUuid )
			, err
			, PRL_RESULT_TO_STRING( err ) );
	}

	pUser->sendSimpleResponse( p, result );
}

void CDspShellHelper::sendVirtualNetworkList(SmartPtr<CDspClient>& pUser,
											 const SmartPtr<IOPackage>& p)
{
#ifdef _LIBVIRT_
	QList<CVirtualNetwork> a;
	PRL_RESULT e = Network::Dao(Libvirt::Kit).list(a);
	if (PRL_FAILED(e))
		return (void)pUser->sendSimpleResponse(p, e);

	// Default bridged network is first.
	CVirtualNetwork *bridged = PrlNet::GetBridgedNetwork(
			CDspService::instance()->getNetworkConfig().getPtr());
	if (bridged != NULL)
	{
		QList<CVirtualNetwork>::iterator it = std::find_if(a.begin(), a.end(),
				boost::bind(&CVirtualNetwork::getNetworkID, _1) == bridged->getNetworkID());
		if (it != a.end())
			std::iter_swap(it, a.begin());
	}

	QStringList x;
	foreach(const CVirtualNetwork& y, a)
	{
		x += y.toString();
	}
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );

	pResponseCmd->SetParamsList(x);

	pUser->sendResponse( pResponse, p );
#else // _LIBVIRT_
	pUser->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
#endif // _LIBVIRT_
}

void CDspShellHelper::sendNetworkClassesConfig(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	CNetworkClassesConfig *pClassesCfg;

	CDspLockedPointer<CParallelsNetworkConfig> pNetCfg = CDspService::instance()->getNetworkConfig();
	if ( ! pNetCfg.isValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Network config doesn't exist!" );
		pUser->sendSimpleResponse(p, PRL_ERR_UNEXPECTED);
		return;
	}

	pClassesCfg = pNetCfg->getNetworkClassesConfig();

#ifdef _CT_
	if (CVzHelper::get_network_classes_config(*pClassesCfg) != 0) {
		pUser->sendSimpleResponse( p, PRL_ERR_OPERATION_FAILED );
		return;
	}
#endif
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );

	pResponseCmd->AddStandardParam(pClassesCfg->toString());

	pUser->sendResponse( pResponse, p );
}

void CDspShellHelper::sendNetworkShapingConfig(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	CNetworkShapingConfig *pShapingCfg;
	CDspLockedPointer<CParallelsNetworkConfig> pNetCfg = CDspService::instance()->getNetworkConfig();
	if ( ! pNetCfg.isValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Network config doesn't exist!" );
		pUser->sendSimpleResponse(p, PRL_ERR_UNEXPECTED);
		return;
	}
	pShapingCfg = pNetCfg->getNetworkShapingConfig();

#ifdef _LIN_
	if (CVzHelper::get_network_shaping_config(*pShapingCfg) != 0) {
		pUser->sendSimpleResponse( p, PRL_ERR_OPERATION_FAILED );
		return;
	}
#endif

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );

	pResponseCmd->AddStandardParam(pShapingCfg->toString());

	pUser->sendResponse( pResponse, p );
}

void CDspShellHelper::sendIPPrivateNetworksList(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	CDspLockedPointer<CParallelsNetworkConfig> pNetCfg = CDspService::instance()->getNetworkConfig();
	if ( ! pNetCfg.isValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Network config doesn't exist!" );
		pUser->sendSimpleResponse(p, PRL_ERR_UNEXPECTED);
		return;
	}

	QStringList lstPrivNet;
	CPrivateNetworks *pNets = pNetCfg->getPrivateNetworks();
	foreach( CPrivateNetwork* pPrivNet, pNets->m_lstPrivateNetwork )
	{
		lstPrivNet += pPrivNet->toString();
	}

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );

	pResponseCmd->SetParamsList( lstPrivNet );

	pUser->sendResponse( pResponse, p );
}

void CDspShellHelper::configureGenericPci(SmartPtr<CDspClient>& pUser,
										  const SmartPtr<IOPackage>& p)
{
	CDspService::instance()->getTaskManager()
		.schedule(new Task_ConfigureGenericPci(pUser, p));
}

void CDspShellHelper::beforeHostSuspend( SmartPtr<CDspClient>& pUser,
										const SmartPtr<IOPackage>& p)
{
	CDspService::instance()->getTaskManager()
		.schedule(new Task_PrepareForHibernate(pUser, p));
}

void CDspShellHelper::afterHostResume(
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p)
{

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
		if ( ! cmd->IsValid() )
			throw PRL_ERR_FAILURE;

		if ( !pUser->getAuthHelper().isLocalAdministrator() )
			throw PRL_ERR_ACCESS_DENIED;

		ret = afterHostResume();
		if( PRL_FAILED(ret) )
			throw ret;
	}
	catch (PRL_RESULT err)
	{
		ret=err;
		WRITE_TRACE( DBG_FATAL, "Error during process command afterHostResume: %#x %s",
			err, PRL_RESULT_TO_STRING(err)
			);
	}
	pUser->sendSimpleResponse( p, ret );
}

PRL_RESULT CDspShellHelper::afterHostResume()
{
	//WRITE_TRACE(DBG_FATAL, "Failed to send VT-d resume ioctl");
	//return PRL_ERR_UNABLE_SEND_REQUEST;
	return PRL_ERR_SUCCESS;
}

static QMutex* gs_pmtxUserAuth = new QMutex;

void CDspShellHelper::changeServerInternalValue(
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
		if ( ! cmd->IsValid() )
			throw PRL_ERR_FAILURE;

		CProtoDspCmdStorageSetValueCommand*
			pCmd = CProtoSerializer::CastToProtoCommand<CProtoDspCmdStorageSetValueCommand>(cmd);

		QString sKey = pCmd->GetKey();
		QString sValue = pCmd->GetValue();

		if ( sKey == PRL_KEY_TO_AUTH_USER )
		{
			QStringList params = sValue.split( USB_LIST_DIVIDER );
			if ( params.size() != 2 || params.at( 0 ).isEmpty() || params.at( 1 ).isEmpty() )
				throw PRL_ERR_INVALID_ARG;

			unsigned int nLimit = CDspService::instance()->getDispConfigGuard()
									.getDispWorkSpacePrefs()->getLimits()->getMaxAuthAttemptsPerMinute();

			gs_pmtxUserAuth->lock();
			static unsigned int nAttemptCount = 0;

			if (++nAttemptCount > nLimit)
			{
				static PRL_UINT64 nAttemptTimeout = 0;

				PRL_UINT64 nTickCount = PrlGetTickCount64();
				if ( ! nAttemptTimeout )
				{
					nAttemptTimeout = nTickCount + (PRL_UINT64 )(PrlGetTicksPerSecond() * 60);
				}
				else if ( nTickCount >= nAttemptTimeout )
				{
					nAttemptTimeout = 0;
					nAttemptCount = 1;
				}

				if (nAttemptCount > nLimit)
					throw PRL_ERR_DISP_LOGON_ACTIONS_REACHED_UP_LIMIT;
			}
			gs_pmtxUserAuth->unlock();

			CAuthHelper helper( params.at( 0 ) );
			bool bAuth = helper.AuthUser( params.at( 1 ) );
			if ( ! bAuth )
				throw PRL_ERR_AUTHENTICATION_FAILED;

			pUser->setAdminAuthWasPassed( bAuth && helper.isLocalAdministrator() );

			CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
			CProtoCommandDspWsResponse* pResponseCmd =
				CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
			pResponseCmd->AddStandardParam( pUser->isAdminAuthWasPassed() ? "1" : "0" );

			pUser->sendResponse( pCmd, p );
		}
		else if ( sKey == PRL_KEY_SET_VNC_ENCRYPTION_DATA )
		{
			if (!pUser->getAuthHelper().isLocalAdministrator())
				throw PRL_ERR_ACCESS_DENIED;

			CDspLockedPointer<QSettings> q =
				CDspService::instance()->getQSettings();
			Vnc::Encryption u(*q.getPtr());
			CVmEventParameter *pEventParam;
			SmartPtr<CVmEvent> pEvent(new CVmEvent(sValue));
			pEventParam = pEvent->getEventParameter(EVT_PARAM_VNC_SSL_CERTIFICATE);
			if (pEventParam) {
				ret = u.setCertificate(pEventParam->getParamValue());
				if (ret)
					throw ret;
			}
			pEventParam = pEvent->getEventParameter(EVT_PARAM_VNC_SSL_PRIVATE_KEY);
			if (pEventParam) {
				ret = u.setKey(pEventParam->getParamValue());
				if (ret)
					throw ret;
			}
		}
#if defined(_WIN_)
		else if (sKey == PRL_KEY_TO_LAUNCH_ON_BEHALF_OF_SYSTEM)
		{
			PRL_RESULT res = PRL_ERR_SUCCESS;
			if (PRL_FAILED(res = LaunchOnBehalfOfSystem(sValue,
				pUser->getAuthHelper().GetAuth()->GetTokenHandle())))
			{
				// Error are reported inside the function
				throw res;
			}
		}
#endif
		else
		{
			throw PRL_ERR_UNIMPLEMENTED;
		}

	}
	catch (PRL_RESULT err)
	{
		ret=err;
		WRITE_TRACE( DBG_FATAL, "Error during process command changeInternalValue: %#x %s",
			err, PRL_RESULT_TO_STRING(err)
			);
	}
	pUser->sendSimpleResponse( p, ret );
}


void CDspShellHelper::updateUsbAssociationsList(SmartPtr<CDspClient>& client,
							   const SmartPtr<IOPackage>& p)
{
	// for AlexKod FIX ME set here handler for update usb list!
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( p );
	try
	{
		if ( ! pCmd->IsValid() )
			throw PRL_ERR_FAILURE;

		CProtoUpdateUsbAssocListCommand*
			pUpdListCmd = CProtoSerializer::CastToProtoCommand<CProtoUpdateUsbAssocListCommand>(pCmd);
		if( !pUpdListCmd )
			throw PRL_ERR_FAILURE;

		QStringList lstUsb;

		quint32 iVer = pUpdListCmd->GetListVersion();
		WRITE_TRACE(DBG_INFO, "Update usb association list %d -> %d", iVer, ULV_VER_END);

		if (iVer == ULV_VER0)
		{
			UINT i = 0;
			// Convert QStringList to QList <CHwGenericDevice*>
			lstUsb = pUpdListCmd->GetUsbList();
			QList <CHwUsbDevice*> old_dev_lst;
			foreach(const QString &str, lstUsb)
			{
				// Split string into friendly & system name
				QStringList frn_sys_name = str.split(USB_LIST_DIVIDER);
				if( frn_sys_name.size() < 2 )
				{
					// invalid sys name!
					WRITE_TRACE( DBG_INFO, "cannot parse usb associations list pair '%s'",
									QSTR2UTF8(str) );
					old_dev_lst.append(new CHwUsbDevice());
					continue;
				}
				// Split system name
				QStringList sys_name = frn_sys_name[1].split('|');
				if ( sys_name.size() < 6 )
				{
					// invalid sys name!
					WRITE_TRACE( DBG_INFO, "Invalid name from client '%s' recieved",
									QSTR2UTF8(str) );
					old_dev_lst.append(new CHwUsbDevice());
					continue;
				}
				bool ok1 = true;
				bool ok2 = true;
				UINT vid = sys_name[1].toUInt(&ok1, 16);
				UINT pid = sys_name[2].toUInt(&ok2, 16);
				if( !ok1 || !ok2 )
				{
					// invalid sys name!
					WRITE_TRACE( DBG_INFO, "Invalid conversion to uint from usb associations list pair '%s'"
						, QSTR2UTF8(str) );

					old_dev_lst.append(new CHwUsbDevice());
					continue;
				}
				// Correct serial number
				QString sn(USB_NUM_EMPTY);
				if (sys_name[5] != "NONE")
					sn = sys_name[5];
				// Re-create system name
				QString new_sys_name =
					CDspHostInfo::CreateUsbSystemName(sys_name[0], vid, pid, USB_SPD_UNKNOWN, sys_name[4], sn);

				WRITE_TRACE(DBG_INFO, "OLD_LIST[%d] <%s> <%s>", i++,
						QSTR2UTF8(frn_sys_name[0]), QSTR2UTF8(new_sys_name));
				CHwUsbDevice * pDev = new CHwUsbDevice();
				pDev->setDeviceName( frn_sys_name[0] );
				pDev->setDeviceId( new_sys_name );
				old_dev_lst.append( pDev );
			}

			// Iterate throw all elements and patch them
			QList <CHwUsbDevice*> new_dev_lst;
			// create copy for safe list iterate
			QList <CHwUsbDevice*> lstOldListCopy = old_dev_lst;

			foreach(CHwUsbDevice *dev, lstOldListCopy)
			{
				// in this cycle we move pointers from old_dev_lst to new_dev_lst without deleting it
				// If found incorrect entry -> just skip
				if (dev->getDeviceId() == "")
				{
					new_dev_lst.append(dev);
					old_dev_lst.removeAll(dev);
				}
				else
				{
					CDspService::instance()->getHostInfo()->
						LookupOrCreateUSBDevice(old_dev_lst, new_dev_lst,
								dev->getDeviceId(), dev->getDeviceName());
				}
			}
			if ( !old_dev_lst.isEmpty() )
			{
				PRL_ASSERT( false );
				WRITE_TRACE( DBG_FATAL, "old_dev_lst not empty after conversion" );
			}
			// Convert QList<CHwGenericDevice*> to QStringList
			i = 0;
			lstUsb.clear();
			foreach(CHwUsbDevice *dev, new_dev_lst)
			{
				lstUsb.append(dev->getDeviceName()+QString( USB_LIST_DIVIDER )+dev->getDeviceId());
				WRITE_TRACE(DBG_INFO, "NEW_LIST[%d] <%s> <%s>", i++,
							QSTR2UTF8(dev->getDeviceName()),
							QSTR2UTF8(dev->getDeviceId()));
				delete dev;
			}
		}

		pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
		if ( !pCmd->IsValid() )
			throw PRL_ERR_FAILURE;

		CProtoCommandDspWsResponse
			*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
		if( !pResponseCmd )
			throw PRL_ERR_FAILURE;

		pResponseCmd->SetParamsList( lstUsb );
		client->sendResponse( pCmd, p );
		return;
	}
	catch (PRL_RESULT err)
	{
		ret=err;
		WRITE_TRACE( DBG_FATAL, "Error during process command updateUsbAssociationsList: %#x %s",
			err, PRL_RESULT_TO_STRING(err)
			);
	}
	catch ( ... )
	{
		ret = PRL_ERR_UNEXPECTED;
		WRITE_TRACE( DBG_FATAL, "Unexpected Error during process command updateUsbAssociationsList: %#x %s",
			ret, PRL_RESULT_TO_STRING(ret)
			);
	}

	client->sendSimpleResponse( p, ret );
}

void CDspShellHelper::sendDiskFreeSpace(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	PRL_RESULT nRetCode;
	PRL_UINT64 timestamp = 0;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		WRITE_TRACE(DBG_FATAL, "Invalid command");
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}

	CProtoCommandWithOneStrParam* pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandWithOneStrParam>(cmd);
	QString sPath = pCmd->GetFirstStrParam();
	quint64 nAvailableSpace;
	QString sAvailableSpace;

	nRetCode = CFileHelper::GetDiskAvailableSpace(sPath, &nAvailableSpace);
	if (PRL_FAILED(nRetCode)) {
		CVmEvent evt;

		WRITE_TRACE(DBG_FATAL, "CFileHelper::GetDiskAvailableSpace(%s) failed : %#x %s",
			QSTR2UTF8(sPath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));

		evt.addEventParameter(
			new CVmEventParameter( PVE::String, sPath, EVT_PARAM_MESSAGE_PARAM_0));
		if( nRetCode != PRL_ERR_INCORRECT_PATH )
			nRetCode = PRL_ERR_GET_DISK_FREE_SPACE_FAILED;
		evt.setEventCode( nRetCode );
		pUser->sendResponseError( evt, p );
		return;
	}

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse* pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	pResponseCmd->AddStandardParam(sAvailableSpace.setNum(nAvailableSpace));

	/* to add timestamp in msecs (https://jira.sw.ru/browse/PSBM-7661) */
#ifdef _LIN_
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
		timestamp = (PRL_UINT64)ts.tv_sec * 1000 + ts.tv_nsec / (1000*1000);
#else
	/* TODO : change to gettimeofday() analog */
	timestamp = PrlGetTickCount64();
#endif
	pResponseCmd->AddStandardParam(QString::number(timestamp));
	pUser->sendResponse( pResponse, p );
}

bool CDspShellHelper::isLocalAddress(const QString &sHost)
{
	if (sHost == "localhost")
		return true;

	// is sHost full or short hostname ?
	QString sLocalHostName = QHostInfo::localHostName();
	QString sLocalDomainName = QHostInfo::localDomainName();
	if (sHost == sLocalHostName)
		return true;
	if (sLocalHostName == sHost + "." + sLocalDomainName)
		return true;

	QHostAddress address;
	if (!address.setAddress(sHost))
		return false;
	if (address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6)
		return true;

	if (QNetworkInterface::allAddresses().contains(address))
		return true;
	return false;
}

void CDspShellHelper::sendSuccessResponseWithStandardParam(SmartPtr<CDspClient>& pUser,
														   const SmartPtr<IOPackage>& p,
														   const QString & param)
{
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse* pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->AddStandardParam( param );
	pUser->sendResponse( pCmd, p );
}

bool CDspShellHelper::isHeadlessModeEnabled()
{
	return true;
}

PRL_RESULT CDspShellHelper::enableHeadlessMode( bool bEnable )
{
	(void)bEnable;
	return PRL_ERR_UNIMPLEMENTED;
}

void CDspShellHelper::sendCPUPoolsList(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	QList<CCpuPool> pools;
	QStringList poolsStr;

	if (!CCpuHelper::loadPoolsList(pools))
	{
		pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
		return;
	}
	foreach(const CCpuPool& p, pools)
		poolsStr += p.toString();

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);
	CProtoCommandDspWsResponse *pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

	pResponseCmd->SetParamsList(poolsStr);

	pUser->sendResponse(pResponse, p);
}

void CDspShellHelper::moveToCPUPool(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(p);
	if (!cmd->IsValid())
	{
		pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
		return;
	}
	QString poolName = cmd->GetFirstStrParam();
	pUser->sendSimpleResponse(p, CCpuHelper::moveToPool(QSTR2UTF8(poolName)));
}

void CDspShellHelper::recalculateCPUPool(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(p);
	if (!cmd->IsValid())
	{
		pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
		return;
	}
	QString poolName = cmd->GetFirstStrParam();
	pUser->sendSimpleResponse(p, CCpuHelper::recalcPool(QSTR2UTF8(poolName)));
}

void CDspShellHelper::sendLicenseInfo(
		SmartPtr<CDspClient>& pUser,
		const SmartPtr<IOPackage>& p)
{
	CDspVzLicense lic;
	lic.load();
	QString sLicense = lic.getVmEvent()->toString();

	CProtoCommandPtr pResponse =
		CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);
	CProtoCommandDspWsResponse* pDspWsResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	pDspWsResponseCmd->SetVmEvent(sLicense);

	pUser->sendResponse(pResponse, p);
}

void CDspShellHelper::updateLicense(
		SmartPtr<CDspClient>& pUser,
		const SmartPtr<IOPackage>& p)
{
	CDspVzLicense lic;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(p);
	if (!cmd->IsValid()) {
		pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
		return;
	}

	CProtoSerialNumCommand* updateCmd =
		CProtoSerializer::CastToProtoCommand<CProtoSerialNumCommand>(cmd);

	PRL_UINT32 nFlags = updateCmd->GetCommandFlags();
	Prl::Expected<void, Error::Simple> r;

	if (nFlags & PUPLF_KICK_TO_UDPATE_CURR_FILE_LICENSE)
		r = lic.update();
	else
		r = lic.install(updateCmd->GetSerialNumber());

	if (r.isFailed())
		pUser->sendResponseError(r.error().convertToEvent(), p);
	else
		pUser->sendSimpleResponse(p, PRL_ERR_SUCCESS);
}
