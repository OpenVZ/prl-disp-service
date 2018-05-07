/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "CDspAccessManager.h"

#include "CDspService.h"
#include "CDspVmDirManager.h"

#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>

#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include <prlcommon/Std/PrlAssert.h>

CDspAccessManager::VmAccessRights::VmAccessRights( unsigned int mode )
:m_mode( mode )
{

}

CDspAccessManager::VmAccessRights::VmAccessRights( const CDspAccessManager::VmAccessRights& vmAccess )
:m_mode( vmAccess.m_mode )
{

}

bool CDspAccessManager::VmAccessRights::canRead() const
{
	return m_mode & arCanRead;
}

bool CDspAccessManager::VmAccessRights::canWrite() const
{
	return m_mode & arCanWrite;
}

bool CDspAccessManager::VmAccessRights::canExecute() const
{
	return m_mode & arCanExecute;
}

bool CDspAccessManager::VmAccessRights::isExists() const
{
	return m_mode & arVmIsExists;
}

PRL_SEC_AM CDspAccessManager::VmAccessRights::getVmAccessRights() const
{
	return m_mode;
}

PRL_SEC_AM CDspAccessManager::VmAccessRights::makeModeNO_ACCESS()
{
	PRL_SEC_AM	mode = arCanNone;
	return mode;
}
PRL_SEC_AM CDspAccessManager::VmAccessRights::makeModeR()
{
	PRL_SEC_AM	mode = arCanRead;
	return mode;
}
PRL_SEC_AM CDspAccessManager::VmAccessRights::makeModeRW()
{
	PRL_SEC_AM	mode = arCanRead | arCanWrite;
	return mode;
}
PRL_SEC_AM CDspAccessManager::VmAccessRights::makeModeRX()
{
	PRL_SEC_AM	mode = arCanRead | arCanExecute;
	return mode;
}
PRL_SEC_AM CDspAccessManager::VmAccessRights::makeModeRWX()
{
	PRL_SEC_AM	mode = arCanRead | arCanWrite | arCanExecute;
	return mode;
}

CDspAccessManager::CDspAccessManager()
{
	initAccessRights();
}

CDspAccessManager::~CDspAccessManager()
{
}

void CDspAccessManager::initAccessRights()
{
#define N VmAccessRights::arCanNone
#define R VmAccessRights::arCanRead
#define W VmAccessRights::arCanWrite
#define X VmAccessRights::arCanExecute

#define PAIR( rigths, cmdAlias )   \
	qMakePair( PRL_SEC_AM( rigths )	, cmdAlias );

	m_accessRights[PVE::DspCmdVmStart]                = PAIR( R | X	, PAR_VM_START_ACCESS );
	m_accessRights[PVE::DspCmdVmStartEx]              = PAIR( R | W | X	, PAR_VM_START_EX_ACCESS );
	m_accessRights[PVE::DspCmdVmRestartGuest]         = PAIR( R | X	, PAR_VM_RESTART_GUEST_ACCESS );
	m_accessRights[PVE::DspCmdVmStop]                 = PAIR( R | X	, PAR_VM_STOP_ACCESS );
	m_accessRights[PVE::DspCmdVmGetConfig]            = PAIR( R		, PAR_VM_GETCONFIG_ACCESS );
	m_accessRights[PVE::DspCmdGetVmInfo]              = PAIR( R		, PAR_VM_GET_VMINFO_ACCESS );
	m_accessRights[PVE::DspCmdVmReset]                = PAIR( R | X	, PAR_VM_RESET_ACCESS );
	m_accessRights[PVE::DspCmdVmPause]                = PAIR( R | X	, PAR_VM_PAUSE_ACCESS );
	m_accessRights[PVE::DspCmdVmSuspend]              = PAIR( R | X	, PAR_VM_SUSPEND_ACCESS );
	m_accessRights[PVE::DspCmdVmSuspendCancel]        = PAIR( R | X	, PAR_VM_SUSPEND_ACCESS );
	m_accessRights[PVE::DspCmdVmGetSuspendedScreen]   = PAIR( R		, PAR_VM_GET_SUSPENDED_SCREEN_ACCESS );
	m_accessRights[PVE::DspCmdVmResume]               = PAIR( R | X	, PAR_VM_RESUME_ACCESS );
	m_accessRights[PVE::DspCmdVmDropSuspendedState]   = PAIR( R | X	, PAR_VM_DROPSUSPENDEDSTATE_ACCESS );
	m_accessRights[PVE::DspCmdVmCreateSnapshot]       = PAIR( R | W | X, PAR_VM_CREATE_SNAPSHOT_ACCESS );
	m_accessRights[PVE::DspCmdVmSwitchToSnapshot]     = PAIR( R | W | X, PAR_VM_SWITCH_TO_SNAPSHOT_ACCESS );
	m_accessRights[PVE::DspCmdVmInternal]             = PAIR( R | W | X, PAR_VM_INTERNAL_CMD_ACCESS );
	m_accessRights[PVE::DspCmdVmDeleteSnapshot]       = PAIR( R | W | X, PAR_VM_DELETE_SNAPSHOT_ACCESS );
	m_accessRights[PVE::DspCmdVmDevConnect]           = PAIR( R | W | X, PAR_VMDEV_CONNECT_ACCESS );
	m_accessRights[PVE::DspCmdVmDevDisconnect]        = PAIR( R | W | X, PAR_VMDEV_DISCONNECT_ACCESS );
	m_accessRights[PVE::DspCmdVmInstallUtility]       = PAIR( R | W	| X, PAR_VM_INSTALL_UTILITY_ACCESS );
	m_accessRights[PVE::DspCmdVmInstallTools]         = PAIR( R | W	| X, PAR_VM_INSTALL_TOOLS_ACCESS );
	m_accessRights[PVE::DspCmdVmRunCompressor]        = PAIR( R | W	| X, PAR_VM_RUN_COMPRESSOR_ACCESS );
	m_accessRights[PVE::DspCmdVmCancelCompressor]     = PAIR( R | W	| X, PAR_VM_CANCEL_COMPRESSOR_ACCESS );
	m_accessRights[PVE::DspCmdDirRegVm]               = PAIR( R | W	, PAR_VM_REGISTER_ACCESS );
	m_accessRights[PVE::DspCmdDirRestoreVm]           = PAIR( R | W	, PAR_VM_RESTORE_ACCESS );
	m_accessRights[PVE::DspCmdDirVmDelete]            = PAIR( R | W	, PAR_VM_DELETE_ACCESS );
	m_accessRights[PVE::DspCmdDirVmClone]             = PAIR( R		, PAR_VM_CLONE_ACCESS );
	m_accessRights[PVE::DspCmdDirVmMigrate]           = PAIR( R | W | X	, PAR_VM_MIGRATE_ACCESS );
	m_accessRights[PVE::DspCmdCtlMigrateTarget]       = PAIR( R | W | X	, PAR_VM_MIGRATE_ACCESS );
	m_accessRights[PVE::DspCmdCtlStartMigratedVm]     = PAIR( R | X	, PAR_VM_START_ACCESS );
	m_accessRights[PVE::DspCmdCreateVmBackup]         = PAIR( R, PAR_VM_CREATE_BACKUP_ACCESS );
	m_accessRights[PVE::DspCmdRestoreVmBackup]        = PAIR( R | W | X	, PAR_VM_RESTORE_BACKUP_ACCESS );
	m_accessRights[PVE::DspCmdDirVmEditBegin]         = PAIR( R | W	, PAR_VM_BEGINEDIT_ACCESS );
	m_accessRights[PVE::DspCmdDirVmEditCommit]        = PAIR( R | W	, PAR_VM_COMMIT_ACCESS );
	m_accessRights[PVE::DspCmdDirCreateImage]         = PAIR( R | W	, PAR_VMDEV_CREATEIMAGE_ACCESS );
	m_accessRights[PVE::DspCmdDirCopyImage]           = PAIR( R		, PAR_VMDEV_COPY_IMAGE_ACCESS );
	m_accessRights[PVE::DspCmdDirUnregVm]             = PAIR( R | W	, PAR_VM_UNREG_ACCESS );
	m_accessRights[PVE::DspCmdVmGetStatistics]        = PAIR( R		, PAR_VM_GETSTATISTICS_ACCESS );
	m_accessRights[PVE::DspCmdVmSubscribeToGuestStatistics]
                                                      = PAIR( R		, PAR_VM_STATISTICS_SUBSCRIPTION_ACCESS );
	m_accessRights[PVE::DspCmdVmUnsubscribeFromGuestStatistics]
                                                      = PAIR( R, PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS );
	m_accessRights[PVE::DspCmdVmGetProblemReport]     = PAIR( R		, PAR_VM_GETPROBLEMREPORT_ACCESS );
	m_accessRights[PVE::DspCmdVmGetPackedProblemReport]     = PAIR( R		, PAR_VM_GETPROBLEMREPORT_ACCESS );
    m_accessRights[PVE::DspCmdSendProblemReport]     = PAIR( R      , PAR_SRV_SENDPROBLEMREPORT_ACCESS );
	m_accessRights[PVE::DspCmdVmInitiateDevStateNotifications]
                                                      = PAIR( R, PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS );
	m_accessRights[PVE::DspCmdVmAnswer]               = PAIR( R | X		, PAR_VM_SEND_ANSWER_ACCESS );
	m_accessRights[PVE::DspCmdVmCompact]				= PAIR( R | W | X, PAR_VM_COMPACT_ACCESS );
	m_accessRights[PVE::DspCmdVmCancelCompact]    = PAIR( R | W | X	, PAR_VM_CANCEL_COMPACT_ACCESS ) ;
	m_accessRights[PVE::DspCmdVmConvertDisks]		  = PAIR( R | W , PAR_VM_CONVERT_DISKS_ACCESS );

	m_accessRights[PVE::DspCmdPerfomanceStatistics]   = PAIR( R         , PAR_VM_PERFSTAT_ACCESS ) ;
	m_accessRights[PVE::DspCmdVmUpdateToolsSection]   = PAIR( R | W | X	, PAR_VM_UPDATE_TOOLS_SECTION ) ;

	m_accessRights[PVE::DspCmdVmStartVNCServer]       = PAIR( R | W | X	, PAR_VM_START_STOP_VNC_SERVER ) ;
	m_accessRights[PVE::DspCmdVmStopVNCServer]        = PAIR( R | W | X	, PAR_VM_START_STOP_VNC_SERVER ) ;

	m_accessRights[PVE::DspCmdVmLock]                 = PAIR( R | W | X	, PAR_VM_LOCK_ACCESS ) ;
	m_accessRights[PVE::DspCmdVmUnlock]                = PAIR( R | W | X	, PAR_VM_UNLOCK_ACCESS ) ;
	m_accessRights[PVE::DspCmdVmResizeDisk]		  = PAIR( R | W		, PAR_VM_RESIZE_DISK_ACCESS) ;
	m_accessRights[PVE::DspCmdVmChangeSid]		  = PAIR( R | X		, PAR_VM_CHANGE_SID_ACCESS) ;
	m_accessRights[PVE::DspCmdVmResetUptime]	  = PAIR( R | W | X	, PAR_VM_RESET_UPTIME_ACCESS) ;
	m_accessRights[PVE::DspCmdGetVmToolsInfo]	  = PAIR( R		    , PAR_VM_GET_TOOLS_INFO) ;
	m_accessRights[PVE::DspCmdVmSetProtection]=		PAIR( R	| W	    , PAR_VM_SET_PROTECTION_ACCESS ) ;
	m_accessRights[PVE::DspCmdVmRemoveProtection]=	PAIR( R	| W		, PAR_VM_REMOVE_PROTECTION_ACCESS ) ;

	// not vm access:  #436109 check for command confirmations
	m_accessRights[PVE::DspCmdDirVmCreate]		  = PAIR( N	, PAR_VM_CREATE_ACCESS);
	m_accessRights[PVE::DspCmdUserProfileBeginEdit]		  = PAIR( N	, PAR_SRV_USER_PROFILE_BEGINEDIT_ACCESS);
	m_accessRights[PVE::DspCmdUserProfileCommit]		  = PAIR( N	, PAR_SRV_USER_PROFILE_COMMIT_ACCESS);
	m_accessRights[PVE::DspCmdHostCommonInfoBeginEdit]		  = PAIR( N	, PAR_SRV_SERVER_PROFILE_BEGINEDIT_ACCESS);
	m_accessRights[PVE::DspCmdHostCommonInfoCommit]		  = PAIR( N	, PAR_SRV_SERVER_PROFILE_COMMIT_ACCESS);

	// to internal usage
	m_accessRights[PVE::DspCmdDirGetVmList]         = PAIR( R, PAR_MAX );
	m_accessRights[PVE::DspCmdVmStorageSetValue]    = PAIR( R | W, PAR_MAX );
	m_accessRights[PVE::DspCmdVmChangeLogLevel]     = PAIR( R | X, PAR_MAX );

	m_accessRights[PVE::DspCmdVmMount]		= PAIR( R | W | X     , PAR_VM_MOUNT_ACCESS);
	m_accessRights[PVE::DspCmdVmUmount]		= PAIR( R | W | X     , PAR_VM_MOUNT_ACCESS);
	m_accessRights[PVE::DspCmdDirVmMove]            = PAIR( R | W | X	, PAR_VM_MOVE_ACCESS );
	m_accessRights[PVE::DspCmdVmCommitEncryption]   = PAIR(R | W , PAR_VM_COMMIT_ACCESS);

#undef N
#undef R
#undef W
#undef X
}

CDspAccessManager::VmAccessRights
CDspAccessManager::getAccessRightsToVm(SmartPtr<CDspClient> pSession_, const QString& vmUuid_)
{
	PRL_ASSERT(pSession_);
	if(!pSession_)
		return CDspAccessManager::VmAccessRights(CDspAccessManager::VmAccessRights::arCanNone);

	CDspLockedPointer<CVmDirectoryItem> pVmDirItem(CDspService::instance()->getVmDirManager()
		.getVmDirItemByUuid(CDspVmDirHelper::getVmIdentByVmUuid(vmUuid_, pSession_)));

	if (pVmDirItem)
		return getAccessRightsToVm(pSession_, pVmDirItem.getPtr());

	WRITE_TRACE(DBG_FATAL, "Can't found pVmDirItem by vmUuid '%s', dirUuid = '%s'",
			QSTR2UTF8(vmUuid_), QSTR2UTF8(pSession_->getVmDirectoryUuid()));
	return CDspAccessManager::VmAccessRights(CDspAccessManager::VmAccessRights::arCanNone);
}

CDspAccessManager::VmAccessRights
CDspAccessManager::getAccessRightsToVm(SmartPtr<CDspClient> pSession, const CVmDirectoryItem* pVmDirItem)
{
	PRL_SEC_AM mode = CDspAccessManager::VmAccessRights::arCanNone;

	PRL_ASSERT( pSession );
	PRL_ASSERT( pVmDirItem );

	if( !pVmDirItem || !pSession )
		return mode;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pSession->getAuthHelper() );

	QString strVmDirPath = CFileHelper::GetFileRoot(pVmDirItem->getVmHome());
	QString strVmConfigPath = pVmDirItem->getVmHome();

	CAuthHelper rootUser;
	if ( CFileHelper::FileExists( strVmConfigPath, &rootUser ) ||
		 //https://bugzilla.sw.ru/show_bug.cgi?id=267152
		 CFileHelper::FileExists( strVmConfigPath, &pSession->getAuthHelper() ) )
		mode |= CDspAccessManager::VmAccessRights::arVmIsExists;
	else
		return mode;

	if ( !CFileHelper::isFsSupportPermsAndOwner( strVmConfigPath ) )
		/*
		  See https://bugzilla.sw.ru/show_bug.cgi?id=269013
		  and https://jira.sw.ru/browse/PSBM-9040
		  On some remote file system we can't determine adequate
		  file permissions.
		*/
		return (CDspAccessManager::VmAccessRights::arVmIsExists |
				CDspAccessManager::VmAccessRights::arCanRead |
				CDspAccessManager::VmAccessRights::arCanWrite |
				CDspAccessManager::VmAccessRights::arCanExecute);


	// set READ flag
	if ( CFileHelper::FileCanRead( strVmConfigPath, &pSession->getAuthHelper() )
		// check if all parent directories has read access
#ifndef _WIN_
		&&	CFileHelper::CanReadAllDirsToRoot(strVmDirPath,&pSession->getAuthHelper() )
#else
		&& CFileHelper::FileCanRead( strVmDirPath, &pSession->getAuthHelper() )
#endif
		)
	{
		mode |= CDspAccessManager::VmAccessRights::arCanRead;

		// set WRITE flag
		if ( CFileHelper::FileCanWrite( strVmConfigPath, &pSession->getAuthHelper() )
			&&	CFileHelper::DirCanWrite( strVmDirPath, &pSession->getAuthHelper() )
			)
		{
			mode |= CDspAccessManager::VmAccessRights::arCanWrite;
		}

		// set EXECUTE flag
		if ( (CFileHelper::FileCanExecute( strVmConfigPath, &pSession->getAuthHelper() ) ||
			CFileHelper::FileCanWrite( strVmConfigPath, &pSession->getAuthHelper() ))
			&& CFileHelper::DirCanWrite( strVmDirPath, &pSession->getAuthHelper() )
			)
		{
			mode |= CDspAccessManager::VmAccessRights::arCanExecute;
		}

	}
	return mode;
}

PRL_RESULT
CDspAccessManager::checkAccess( SmartPtr<CDspClient> pSession, PVE::IDispatcherCommands cmd
							   , const QString& vmUuid, bool *bSetNotValid, CVmEvent *pErrorInfo )
{
	PRL_ASSERT(pSession);
	try
	{
		if (!pSession)
			throw PRL_ERR_ACCESS_TO_VM_DENIED;

		CVmIdent ident(CDspVmDirHelper::getVmIdentByVmUuid(vmUuid, pSession));
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem(CDspService::instance()->getVmDirManager()
				.getVmDirItemByUuid(ident));
		if (pVmDirItem)
			return checkAccess(pSession, cmd, pVmDirItem.getPtr(), bSetNotValid, pErrorInfo);

		WRITE_TRACE(DBG_FATAL, "CDspAccessManager::checkAccess: "
			"Can't found pVmDirItem by vmUuid '%s'" , QSTR2UTF8(vmUuid));
		throw PRL_ERR_VM_UUID_NOT_FOUND;
	}
	catch (PRL_RESULT nErrCode)
	{
		if (pErrorInfo)
			pErrorInfo->setEventCode(nErrCode);
		return nErrCode;
	}
}

PRL_RESULT
CDspAccessManager::checkAccess( SmartPtr<CDspClient> pSession, PVE::IDispatcherCommands cmd
	, CVmDirectoryItem* pVmDirItem, bool *bSetNotValid, CVmEvent *pErrorInfo)
{
	PRL_ASSERT( pSession );
	PRL_ASSERT( pVmDirItem );

	QString v(CDspVmDirHelper::getVmDirUuidByVmUuid(pVmDirItem->getVmUuid(), pSession));
	try
	{
		if( !pVmDirItem || !pSession )
			throw PRL_ERR_ACCESS_TO_VM_DENIED;

		//Check whether VM is exclusively locked
		IOSender::Handle hSession = CDspService::instance()->getVmDirHelper().
				getVmLockerHandle(pVmDirItem->getVmUuid(), v);
		if ( !hSession.isEmpty() && pSession->getClientHandle() != hSession )
		{
			switch ( cmd )
			{
			case PVE::DspCmdVmUnlock:
				throw PRL_ERR_NOT_LOCK_OWNER_SESSION_TRIES_TO_UNLOCK;
			case PVE::DspCmdVmUnsubscribeFromGuestStatistics:
			case PVE::DspCmdVmSubscribeToGuestStatistics:
			case PVE::DspCmdVmGetStatistics:
			case PVE::DspCmdPerfomanceStatistics:
				break;
			default:
				throw PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED;
			}
		}

		if ( ! m_accessRights.contains(cmd) )
		{
			WRITE_TRACE(DBG_FATAL, "accessRights doesn't have information about rights");
			throw PRL_ERR_ACCESS_TO_VM_DENIED;
		}

		VmAccessRights rightsToVm = getAccessRightsToVm( pSession, pVmDirItem );
		PRL_SEC_AM access_rights = m_accessRights[cmd].first;

		if ( ( access_rights & rightsToVm.getVmAccessRights() ) != (access_rights) )
		{
			WRITE_TRACE(DBG_FATAL, "Access denied for user '%s' to VM with UUID '%s' required %.8X actual %.8X command is '%s'"
				, QSTR2UTF8( pSession->getAuthHelper().getUserFullName() )
				, QSTR2UTF8( pVmDirItem->getVmUuid() )
				, access_rights
				, rightsToVm.getVmAccessRights()
				, PVE::DispatcherCommandToString(cmd));

			if (rightsToVm.isExists())
				throw PRL_ERR_ACCESS_TO_VM_DENIED;
			else
			{
				// #8986
				if (bSetNotValid && pVmDirItem->getValid() != PVE::VmNotValid)
				{
					*bSetNotValid = true;
					pVmDirItem->setValid(PVE::VmNotValid);
				}
				if (pErrorInfo)
				{
					pErrorInfo->addEventParameter(
						new CVmEventParameter( PVE::String, pVmDirItem->getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
					pErrorInfo->addEventParameter(
						new CVmEventParameter( PVE::String, pVmDirItem->getVmHome(), EVT_PARAM_MESSAGE_PARAM_1));
				}
				throw PRL_ERR_VM_CONFIG_DOESNT_EXIST;
			}
		}

		// load VM configuration from file
		//////////////////////////////////////////////////////////////////////////
		SmartPtr<CVmConfiguration> pVmConfig( new CVmConfiguration() );
		PRL_RESULT res = CDspService::instance()->getVmConfigManager().loadConfig( pVmConfig,
				pVmDirItem->getVmHome(), pSession);

		if( PRL_FAILED(res) )
			throw PRL_ERR_PARSE_VM_CONFIG;
		PRL_ASSERT(pVmConfig);

		if ( pVmConfig->getVmIdentification()->getVmUuid() != pVmDirItem->getVmUuid() )
		{
			if (pErrorInfo)
			{
				pErrorInfo->addEventParameter(
					new CVmEventParameter( PVE::String, pVmDirItem->getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
			}
			throw PRL_ERR_VM_CONFIG_INVALID_VM_UUID;
		}

		if (CDspDispConfigGuard::getServerUuid() != pVmConfig->getVmIdentification()->getServerUuid())
		{
			switch (cmd)
			{
			case PVE::DspCmdDirUnregVm:
			case PVE::DspCmdCtlMigrateTarget:
			case PVE::DspCmdCtlStartMigratedVm:
				break;
			case PVE::DspCmdDirRegVm:
			case PVE::DspCmdDirVmClone:
			case PVE::DspCmdDirVmDelete:
				/* allow Clone & Reg & Delete for multiple registered templates */
				if (pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
					break;
			default:
				if (NULL != pErrorInfo)
				{
					pErrorInfo->addEventParameter(
						new CVmEventParameter( PVE::String, pVmDirItem->getVmName(),
									EVT_PARAM_MESSAGE_PARAM_0));
				}
				throw PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// check confirmation logic
		if( pSession->isConfirmationEnabled() && m_accessRights.contains( cmd ) )
		{
			PRL_ALLOWED_VM_COMMAND sdkCmd = m_accessRights.value( cmd ).second;

			//////////////////////////////////////////////////////////////////////////
			// check common part
			PRL_RESULT err = checkAccess( pSession, cmd );
			if( PRL_FAILED(err) )
				throw err;

			//////////////////////////////////////////////////////////////////////////
			// check per-vm part
			QList<PRL_ALLOWED_VM_COMMAND> confirmList = pVmDirItem->getLockedOperationsList()->getLockedOperations();
			if( confirmList.contains(sdkCmd) )
				throw PRL_ERR_ADMIN_CONFIRMATION_IS_REQUIRED_FOR_VM_OPERATION;
		}

	}
	catch (PRL_RESULT nErrCode)
	{
		if (pErrorInfo)
			pErrorInfo->setEventCode(nErrCode);
		return (nErrCode);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspAccessManager::checkAccess( SmartPtr<CDspClient> pSession, PVE::IDispatcherCommands cmd )
{
	PRL_ASSERT( pSession );
	if( !pSession )
		return PRL_ERR_FAILURE;

	if( pSession->isConfirmationEnabled() && m_accessRights.contains( cmd ) )
	{
		PRL_ALLOWED_VM_COMMAND sdkCmd = m_accessRights.value( cmd ).second;

		QList<PRL_ALLOWED_VM_COMMAND> confirmList = CDspService::instance()
			->getVmDirManager().getCommonConfirmatedOperations();

		if( confirmList.contains(sdkCmd) )
			return PRL_ERR_ADMIN_CONFIRMATION_IS_REQUIRED_FOR_OPERATION;
	}

	return PRL_ERR_SUCCESS;
}

QList< PRL_ALLOWED_VM_COMMAND >
CDspAccessManager::getAllowedVmCommands( SmartPtr<CDspClient> pSession, const CVmDirectoryItem* pVmDirItem )
{
	QList< PRL_ALLOWED_VM_COMMAND > lstAllowed;
	VmAccessRights accessRights = getAccessRightsToVm( pSession, pVmDirItem );

	QHashIterator< PVE::IDispatcherCommands, AccessRigthsPair > it( m_accessRights );
	while( it.hasNext() )
	{
		it.next();

		LOG_MESSAGE( DBG_DEBUG, "key = %#x(%s), val = (%d, %d)"
			,it.key()
			, PVE::DispatcherCommandToString( it.key() )
			, it.value().first
			, it.value().second
			);

		PRL_SEC_AM accessToCmd = it.value().first;

		if (accessRights.isExists())
		{
			if ( ( accessToCmd & accessRights.getVmAccessRights() ) != accessToCmd )
				continue;
		}
		else
		{
			// VM is unexist but its information present in vmdirectory.
			// operation possible for client.
			if (it.key() != PVE::DspCmdDirUnregVm && it.key() != PVE::DspCmdVmStop)
				continue;
		}
		lstAllowed.append( it.value().second );
	}//while

	if( isOwnerOfVm( pSession, pVmDirItem ) || pSession->getAuthHelper().isLocalAdministrator() )
	{
		lstAllowed.append( PAR_VM_UPDATE_SECURITY_ACCESS );
		if ( !lstAllowed.contains( PAR_VM_RESET_UPTIME_ACCESS ) )
			lstAllowed.append( PAR_VM_RESET_UPTIME_ACCESS );
	}

	return lstAllowed;
}


PRL_RESULT
CDspAccessManager::getFullAccessRightsToVm(
	const CVmDirectoryItem* pVmDirItem
	, PRL_SEC_AM& ownerAccess
	, PRL_SEC_AM& othersAccess
	, bool& flgOthersAccessIsMixed
	, CAuthHelper &_current_user)
{
	PRL_RESULT err = PRL_ERR_FAILURE;
	flgOthersAccessIsMixed = false;
	// 1. get permission to config.pvs
	// 2. get permission to vm directory
	// 3. make AND operation

	ownerAccess = othersAccess = VmAccessRights::arCanNone;

	PRL_ASSERT( pVmDirItem );
	if( !pVmDirItem )
		return PRL_ERR_INVALID_ARG;

	do
	{
		QString strVmConfigPath, strVmDirPath;
		{
			//https://bugzilla.sw.ru/show_bug.cgi?id=267152
			CAuthHelperImpersonateWrapper _impersonate( &_current_user );

			strVmConfigPath = pVmDirItem->getVmHome();
			strVmDirPath = CFileHelper::GetFileRoot(pVmDirItem->getVmHome());

			CAuthHelper rootUser;
			if ( CFileHelper::FileExists( strVmConfigPath, &rootUser ) ||
				 CFileHelper::FileExists( strVmConfigPath, &_current_user ) )
				ownerAccess = othersAccess |= CDspAccessManager::VmAccessRights::arVmIsExists;
			else
				break;

			if ( !CFileHelper::isFsSupportPermsAndOwner( strVmConfigPath ) )
			{
				//Can't to retrieve adequate rigths from network share
				ownerAccess =	CDspAccessManager::VmAccessRights::arCanRead |
						CDspAccessManager::VmAccessRights::arCanWrite |
						CDspAccessManager::VmAccessRights::arCanExecute;
				break;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// get permissions
		//////////////////////////////////////////////////////////////////////////
		CAuth::AccessMode fileOwnerPerm = 0, fileOthersPerm = 0;
		CAuth::AccessMode  dirOwnerPerm = 0, dirOthersPerm = 0;
		bool fileOthersPermIsMixed = false, dirOthersPermIsMixed = false;

		err = CFileHelper::GetSimplePermissionsToFile( strVmConfigPath
			, fileOwnerPerm, fileOthersPerm, fileOthersPermIsMixed, _current_user );

		if( PRL_FAILED( err ) )
		{
			WRITE_TRACE(DBG_FATAL, "GetSimplePermissionsToFile() failed for file '%s', error %s"
				, QSTR2UTF8( strVmConfigPath )
				, PRL_RESULT_TO_STRING( err ) );
			break;
		}

		err = CFileHelper::GetSimplePermissionsToFile( strVmDirPath
			, dirOwnerPerm, dirOthersPerm, dirOthersPermIsMixed, _current_user );

		if( PRL_FAILED( err ) )
		{
			WRITE_TRACE(DBG_FATAL, "GetSimplePermissionsToFile() failed for dir '%s', error %s"
				, QSTR2UTF8( strVmDirPath )
				, PRL_RESULT_TO_STRING( err ) );
			break;
		}


		//////////////////////////////////////////////////////////////////////////
		// analyze
		//////////////////////////////////////////////////////////////////////////
		ownerAccess = ConvertCAuthPermToVmPerm( fileOwnerPerm, dirOwnerPerm );
		othersAccess = ConvertCAuthPermToVmPerm( fileOthersPerm, dirOthersPerm );

		err = PRL_ERR_SUCCESS;
		flgOthersAccessIsMixed = fileOthersPermIsMixed ||  dirOthersPermIsMixed;
	} while(0);

	LOG_MESSAGE( DBG_INFO, "permission is '%#o' / '%#o' (others mixed= %s )for vm with vm_uuid='%s' path='%s'"
		, ownerAccess
		, othersAccess
		, flgOthersAccessIsMixed ? "true":"false"
		, QSTR2UTF8( pVmDirItem->getVmUuid() )
		, QSTR2UTF8( pVmDirItem->getVmHome() )
		);

	return err;
}

PRL_RESULT
CDspAccessManager::setFullAccessRightsToVm( SmartPtr<CDspClient> pSession
	, const CVmDirectoryItem* pVmDirItem
	, const PRL_SEC_AM* ownerAccess
	, const PRL_SEC_AM* othersAccess)
{
	PRL_ASSERT( pSession );
	PRL_ASSERT( pVmDirItem );
	if( !pSession || !pVmDirItem )
		return PRL_ERR_INVALID_ARG;

	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &pSession->getAuthHelper() );

		if( !CFileHelper::FileExists(pVmDirItem->getVmHome(), &pSession->getAuthHelper()) )
			return PRL_ERR_VM_CONFIG_DOESNT_EXIST;

		// FIXME:
		// 1. check CFileHelper::isFsSupportPermsAndOwner() should return with success.
		// 2. callers should drop isFsSupportPermsAndOwner().

		// # https://bugzilla.sw.ru/show_bug.cgi?id=112901
		// will set real rigths for nfs (https://jira.sw.ru/browse/PSBM-9040)
		if( !CFileHelper::isFsSupportPermsAndOwner( pVmDirItem->getVmHome() ) )
		{
			WRITE_TRACE(DBG_FATAL, "File system does not support permissions. setFullAccessRightsToVm will be ignored. (path=%s)"
				, QSTR2UTF8( pVmDirItem->getVmHome() ) );
			return PRL_ERR_CANT_TO_CHANGE_PERMISSIONS_ON_REMOTE_LOCATION;
		}

	}

	PRL_RESULT result = PRL_ERR_FAILURE;
	try
	{
		LOG_MESSAGE( DBG_INFO, "Try to set permission to own='%#o' oth='%#o' for vm with vm_uuid='%s' path='%s'"
			, ownerAccess? *ownerAccess :-1
			, othersAccess? *othersAccess: -1
			, QSTR2UTF8( pVmDirItem->getVmUuid() )
			, QSTR2UTF8( pVmDirItem->getVmHome() )
			);

		if( !isOwnerOfVm( pSession, pVmDirItem ) && !pSession->getAuthHelper().isLocalAdministrator() )
			throw PRL_ERR_ACCESS_DENIED_TO_CHANGE_PERMISSIONS;

		CAuth::AccessMode *pOwn = 0, *pOwnDir = 0
			, *pOth = 0, *pOthDir = 0
			, owner_mode = 0, other_mode = 0
			, ownerDir_mode = 0, otherDir_mode = 0;

		if( ownerAccess )
		{
			ConvertVmPermToCAuthPerm( *ownerAccess, owner_mode, ownerDir_mode );

			pOwn = &owner_mode;
			pOwnDir = &ownerDir_mode;
		}

		if( othersAccess )
		{
			ConvertVmPermToCAuthPerm( *othersAccess, other_mode, otherDir_mode );

			pOth = &other_mode;
			pOthDir = &otherDir_mode;
		}

		QString strVmDirPath = CFileHelper::GetFileRoot(pVmDirItem->getVmHome());

		// set perm to dir
		PRL_RESULT err = CFileHelper::SetSimplePermissionsToFile( strVmDirPath
								,pSession->getAuthHelper(), pOwnDir, pOthDir, true );

		if( PRL_FAILED( err ) )
			throw err;

		// set perm to scripts
		QString scriptsDir = ParallelsDirs::getVmScriptsDir( strVmDirPath );
		if( CFileHelper::DirectoryExists(scriptsDir, &pSession->getAuthHelper()) )
		{
			CAuth::AccessMode otherScripts = CAuth::fileMayRead;

			err = CFileHelper::SetSimplePermissionsToFile( scriptsDir
							, pSession->getAuthHelper(), pOwn, &otherScripts, true );

			if( PRL_FAILED( err ) )
				throw err;
		}

		// set perm to config.pvs
		err = CFileHelper::SetSimplePermissionsToFile( pVmDirItem->getVmHome()
							, pSession->getAuthHelper(), pOwn, pOth, false );

		if( PRL_FAILED( err ) )
			throw err;

		result = PRL_ERR_SUCCESS;
	}
	catch( PRL_RESULT err )
	{
		WRITE_TRACE(DBG_FATAL, "[%s] error raised %#x, %s"
			, __FUNCTION__
			, err
			, PRL_RESULT_TO_STRING( err )
			);
		result = err;
	}

	return result;
}

PRL_RESULT
CDspAccessManager::setOwnerAndAccessRightsToPathAccordingVmRights(
	const QString& sPath
	, SmartPtr<CDspClient> pSession
	, const CVmDirectoryItem* pVmDirItem
	, bool bRecursive)
{
	//////////////////////////////////////////////////////////////////////////
	// 1. get vm Access Rigths
	// 2. convert rigths to file helper mode
	// 3. set AccessRights to path
	//////////////////////////////////////////////////////////////////////////

	PRL_ASSERT( pSession );
	PRL_ASSERT( pVmDirItem );
	if( !pSession || !pVmDirItem )
		return PRL_ERR_INVALID_ARG;

	PRL_RESULT err = PRL_ERR_FAILURE;
	struct ErrorMessage
	{
		PRL_RESULT err;
		QString errStr;

		ErrorMessage( PRL_RESULT err, const QString& errStr )
			:err(err), errStr( errStr)
		{}
	};
	try
	{

		{
			//https://bugzilla.sw.ru/show_bug.cgi?id=267152
			CAuthHelperImpersonateWrapper _impersonate( &pSession->getAuthHelper() );

			if( !CFileHelper::FileExists(pVmDirItem->getVmHome(), &pSession->getAuthHelper()) )
				return PRL_ERR_VM_CONFIG_DOESNT_EXIST;

			// # https://bugzilla.sw.ru/show_bug.cgi?id=112901
			// will set real owner & rigths for nfs (https://jira.sw.ru/browse/PSBM-9040)
			if( !CFileHelper::isFsSupportPermsAndOwner( pVmDirItem->getVmHome() ) )
			{
				WRITE_TRACE(DBG_FATAL, "File system does not support permissions. "
					"setOwnerAndAccessRightsToPathAccordingVmRights will be ignored. (path=%s)"
					, QSTR2UTF8( pVmDirItem->getVmHome() ) );
				return PRL_ERR_CANT_TO_CHANGE_PERMISSIONS_ON_REMOTE_LOCATION;
			}

		}

		//////////////////////////////////////////////////////////////////////////
		// 0. set vm original owner
		err = CDspAccessManager::setOwnerByTemplate( sPath, pVmDirItem->getVmHome(), pSession->getAuthHelper(), bRecursive);
		if( PRL_FAILED(err) )
			throw ErrorMessage( err, "setOwnerByTemplate failed." );

		//////////////////////////////////////////////////////////////////////////
		// 1. get vm Access Rigths
		PRL_SEC_AM ownerAccess = 0;
		PRL_SEC_AM othersAccess = 0;
		bool flgOthersAccessIsMixed = false;
		err = getFullAccessRightsToVm( pVmDirItem, ownerAccess, othersAccess, flgOthersAccessIsMixed, pSession->getAuthHelper() );
		if( PRL_FAILED( err ) )
			throw ErrorMessage( err, "getFullAccessRightsToVm() failed." );

		//////////////////////////////////////////////////////////////////////////
		// 2. convert rights to file helper mode
		CAuth::AccessMode mode_owner = 0, mode_other = 0, mode_fake = 0;
		// get dir permission
		ConvertVmPermToCAuthPerm( ownerAccess, mode_fake, mode_owner );
		ConvertVmPermToCAuthPerm( othersAccess, mode_fake, mode_other );

		//////////////////////////////////////////////////////////////////////////
		// 3. set AccessRights to path
		err = CFileHelper::SetSimplePermissionsToFile( sPath
			,pSession->getAuthHelper(), &mode_owner, &mode_other, bRecursive );
		if( PRL_FAILED( err ) )
			throw ErrorMessage( err, "CFileHelper::SetSimplePermissionsToFile() failed" );

	}
	catch ( ErrorMessage& errMsg )
	{
		WRITE_TRACE(DBG_FATAL, "Error: error %#x( %s ) on %s"
			, errMsg.err
			, PRL_RESULT_TO_STRING( errMsg.err )
			, QSTR2UTF8( errMsg.errStr ) );
		return errMsg.err;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspAccessManager::setOwnerByTemplate(
	const QString& sPath
	, const QString& sTemplateVmConfigPath
	, CAuthHelper& authHelper
	, bool bRecursive)
{

	{
		CAuthHelperImpersonateWrapper _impersonate(&authHelper);
		if( !CFileHelper::isFsSupportPermsAndOwner( sPath ) )
		{
			WRITE_TRACE(DBG_DEBUG, "File system does not support permissions."
				" Setting default Vm permission will be ignored. (path=%s)"
				, QSTR2UTF8( sPath ) );
			return true;
		}
	}

	return CFileHelper::setOwnerByTemplate( sPath, sTemplateVmConfigPath, authHelper, bRecursive )
		? PRL_ERR_SUCCESS
		: PRL_ERR_CANT_CHANGE_OWNER_OF_FILE;
}


bool CDspAccessManager::setOwner( const QString & strFileName, CAuthHelper* pAuthHelper, bool bRecursive)
{
	PRL_ASSERT(pAuthHelper);

	{
		CAuthHelperImpersonateWrapper _impersonate(pAuthHelper);
		if( !CFileHelper::isFsSupportPermsAndOwner( strFileName ) )
		{
			WRITE_TRACE(DBG_DEBUG, "File system does not support permissions."
				" Setting default Vm permission will be ignored. (path=%s)"
				, QSTR2UTF8( strFileName ) );
			return true;
		}
	}

	return CFileHelper::setOwner( strFileName, pAuthHelper, bRecursive );
}


QString CDspAccessManager::getOwnerOfVm( const CVmDirectoryItem* pVmDirItem )
{
	PRL_ASSERT( pVmDirItem );
	if( !pVmDirItem )
		return "";

	// # https://bugzilla.sw.ru/show_bug.cgi?id=112901
	if( !CFileHelper::isFsSupportPermsAndOwner( pVmDirItem->getVmHome() ) )
	{
		LOG_MESSAGE( DBG_INFO, "File system does not support permissions. getOwner will be ignored. (path=%s)"
			, QSTR2UTF8( pVmDirItem->getVmHome() ) );
		return "";
	}

	return CFileHelper::getOwner( pVmDirItem->getVmHome() );
}

bool CDspAccessManager::isOwnerOfVm( SmartPtr<CDspClient> pSession, const CVmDirectoryItem* pVmDirItem )
{
	//////////////////////////////////////////////////////////////////////////
	// 1. check owner of vm config
	// 2. check owner of vm dir
	// 3. make AND operation
	// 4. return result

	PRL_ASSERT( pSession );
	PRL_ASSERT( pVmDirItem );
	if( !pSession || !pVmDirItem )
		return false;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pSession->getAuthHelper() );

	QString strVmDirPath = CFileHelper::GetFileRoot(pVmDirItem->getVmHome());

	// https://bugzilla.sw.ru/show_bug.cgi?id=112901
	if( !CFileHelper::isFsSupportPermsAndOwner( pVmDirItem->getVmHome() ) )
	{
		LOG_MESSAGE( DBG_INFO, "File system does not support permissions. getOwner will be ignored. (path=%s)"
			, QSTR2UTF8( pVmDirItem->getVmHome() ) );
		return true;
	}

#ifdef _LIN_
	// https://bugzilla.sw.ru/show_bug.cgi?id=268812
	if ( CFileHelper::isNtfsPartition( pVmDirItem->getVmHome() ) )
	{
		LOG_MESSAGE( DBG_INFO, "Path to vm is on NTFS partition. Actually any user is owner of VM in this case. (path=%s)"
			, QSTR2UTF8( pVmDirItem->getVmHome() ) );
		return true;
	}

	// https://bugzilla.sw.ru/show_bug.cgi?id=427598
	PRL_FILE_SYSTEM_FS_TYPE _fs_type = HostUtils::GetFSType( pVmDirItem->getVmHome() );
	if ( ( PRL_FS_FAT == _fs_type || PRL_FS_FAT32 == _fs_type ) && pSession->getAuthHelper().isLocalAdministrator() )
	{
		LOG_MESSAGE( DBG_INFO, "Path to vm is on FAT partition and current session belongs to root. "\
								"Skip ownership checking. (path=%s)"\
								, QSTR2UTF8( pVmDirItem->getVmHome() ) );
		return true;
	}
#endif

	return pSession->getAuthHelper().isOwnerOfFile( pVmDirItem->getVmHome() )
		&& pSession->getAuthHelper().isOwnerOfFile( strVmDirPath );
}

PRL_SEC_AM CDspAccessManager::ConvertCAuthPermToVmPerm(
	const CAuth::AccessMode& vmConfigMode,
	const CAuth::AccessMode& vmDirMode)
{
	PRL_SEC_AM mode = CDspAccessManager::VmAccessRights::arCanNone;

	if( vmConfigMode & CAuth::pathNotFound )
		return mode;

	mode =  CDspAccessManager::VmAccessRights::arVmIsExists;

	// Read mode
	if(	( vmConfigMode & CAuth::fileMayRead )
		&& ( vmDirMode & CAuth::fileMayRead )
		)
	{
		mode |= VmAccessRights::arCanRead;

		// Write mode
		if(	( vmConfigMode & CAuth::fileMayWrite )
			&& ( vmDirMode & CAuth::fileMayWrite )
			)
		{
			mode |= VmAccessRights::arCanWrite;
		}

		// Execute mode
		if(	(( vmConfigMode & CAuth::fileMayExecute ) || ( vmConfigMode & CAuth::fileMayWrite ))
			&& ( vmDirMode & CAuth::fileMayWrite )
			)
		{
			mode |= VmAccessRights::arCanExecute;
		}
	}

	return mode;
}

void CDspAccessManager::ConvertVmPermToCAuthPerm(
	const PRL_SEC_AM& vmPerm,
	CAuth::AccessMode& vmConfigMode,
	CAuth::AccessMode& vmDirMode)
{
	vmDirMode = vmConfigMode = 0;

	typedef VmAccessRights VR;

	if( !(vmPerm & VR::arCanRead) )
		return;

	vmConfigMode = vmDirMode = CAuth::fileMayRead;

	if( vmPerm & VR::arCanWrite )
	{
		vmDirMode		|= CAuth::fileMayWrite;
		vmConfigMode	|=  CAuth::fileMayWrite;
	}

	if( vmPerm & VR::arCanExecute )
	{
		vmDirMode		|= CAuth::fileMayWrite;
		vmConfigMode	|=  CAuth::fileMayExecute;
	}

	return;
}

