///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspClientManager
///
/// Client manager and handler, which is responsible for all clients and
/// packages for these clients or packages from these clients.
///
/// @author romanp
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
///////////////////////////////////////////////////////////////////////////////

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "CDspClientManager.h"
#include "CDspService.h"
#include "Stat/CDspStatCollectingThread.h"
#include "Stat/CDspStatisticsGuard.h"
#include "CDspRouter.h"
#include "CDspVm.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_CreateProblemReport.h"
#include "Tasks/Task_BackgroundJob.h"
#include "CDspVmManager.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/CommandConvHelper.h>
#include "CDspService.h"
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "CDspVzHelper.h"
#include "CVmValidateConfig.h"

using namespace Parallels;

/*****************************************************************************/

CDspClientManager::CDspClientManager(CDspService& service_, const Backup::Task::Launcher& backup_):
	CDspHandler(IOService::IOSender::Client, "ClientHandler"),
	dsp_commands(NULL), dsp_error_commands(NULL), m_service(&service_),
	m_backup(backup_)
{
}

static inline counter_t* create_counter(const storage_descriptor_t &sd, const char *c_name, counter_ptr *result)
{
    if (*result)
        return *result ;

    int err = perf_add_counter(sd.storage, c_name, result) ;
    PRL_ASSERT( err == ERR_NO_ERROR ) ;
    PRL_ASSERT( *result ) ;

    return *result ;
}

void CDspClientManager::init ()
{
	/*****************************************************************************************
	 * $PERF$ "mgmt.commands"
	 * Total count of management commands, recieved by Dispatcher
	 *****************************************************************************************/
    create_counter(m_service->getBasePerfStorage(),
		PERF_COUNT_TYPE_INC "mgmt.commands", &dsp_commands) ;

	/*****************************************************************************************
	 * $PERF$ "mgmt.error_commands"
	 * Total count of failed management commands, recieved by Dispatcher
	 *****************************************************************************************/
    create_counter(m_service->getBasePerfStorage(),
		PERF_COUNT_TYPE_INC "mgmt.error_commands", &dsp_error_commands) ;
}

SmartPtr<CDspClient> CDspClientManager::getUserSession (
		const IOSender::Handle& h) const
{
	QReadLocker _lock(&m_rwLock);
	return (m_clients.value(h));
}

void CDspClientManager::deleteUserSession (
		const IOSender::Handle& h)
{
	m_service->getIOServer().disconnectClient(h);
}


void CDspClientManager::handleClientConnected ( const IOSender::Handle& )
{
	WRITE_TRACE( DBG_WARNING, "%s", __FUNCTION__ );
}

void CDspClientManager::handleClientDisconnected ( const IOSender::Handle& h  )
{
	WRITE_TRACE( DBG_WARNING, "%s", __FUNCTION__ );

	QWriteLocker locker( &m_rwLock );
	SmartPtr<CDspClient> pUser = m_clients.take(h);
	m_preAuthorizedSessions.remove(h);
	locker.unlock();  // unlock to prevent deadlocks


#ifdef SENTILLION_VTHERE_PLAYER
	if ( pUser.isValid() )
		m_service->getVmManager().shutdownVmsByClient(pUser, false);
#endif

	if (!m_service->isServerStopping())//https://bugzilla.sw.ru/show_bug.cgi?id=444674
	{
		if (pUser.isValid())
		{
			CDspStatCollectingThread::ClientDisconnected(pUser) ;

			QList< SmartPtr<CDspTaskHelper> >
				lstTasks = m_service->getTaskManager().getTaskListBySession(h);
			foreach( SmartPtr<CDspTaskHelper> pTask, lstTasks )
			{
				PRL_ASSERT(pTask);
				if( !pTask->shouldBeCanceled_OnClientDisconnect() )
					continue;
				WRITE_TRACE( DBG_WARNING
					, "Task with id=%s will be canceled because user session was disconnected. sessionId =  %s"
					, QSTR2UTF8( pTask->getJobUuid().toString() )
					, QSTR2UTF8( h ) );
				SmartPtr<IOPackage> p =
					IOPackage::createInstance( PVE::DspCmdUserCancelOperation, 0 );
				pTask->cancelOperation( pUser, p );
			}
		}

		m_service->getVmDirHelper().cleanupSessionVmLocks(h);
	}
}

QHash< IOSender::Handle, SmartPtr<CDspClient> >
CDspClientManager::getSessionsListSnapshot( const QString& vmDirUuid, CDspClient::CLIENT_TYPE nClientType ) const
{
	QHash< IOSender::Handle, SmartPtr<CDspClient> >  result;
	{
		QReadLocker locker( &m_rwLock );
		if( vmDirUuid.isEmpty()
			&& nClientType == CDspClient::CT_ALL_CLIENTS )
			return m_clients;
		result = m_clients;
	}

	QMutableHashIterator< IOSender::Handle, SmartPtr<CDspClient> >
		it( result );
	while( it.hasNext() )
	{
		SmartPtr<CDspClient>
			pClient = it.next().value();
		if (   ( ! vmDirUuid.isEmpty() && pClient->getVmDirectoryUuid() != vmDirUuid)
			|| ! (nClientType & pClient->getClientType())
			)
			it.remove();
	}

	return (result);
}

QHash< IOSender::Handle, SmartPtr<CDspClient> > CDspClientManager::getSessionListByVm(
	const CVmIdent& vmIdent, bool bSearchAsSharedVm, int accessRights) const
{
	QHash< IOSender::Handle, SmartPtr<CDspClient> > lst;

	QList< QString > dirList;
	if( !bSearchAsSharedVm )
		dirList << vmIdent.second;
	else
	{
		CDspLockedPointer<CVmDirectoryItem>
			pVmDirItem = m_service->getVmDirManager().getVmDirItemByUuid(vmIdent);
		if( !pVmDirItem )
			return lst;
		dirList = m_service->getVmDirManager().findVmDirItemsInCatalogue(
			pVmDirItem->getVmUuid()
			,pVmDirItem->getVmHome()
			).keys();
	}

	foreach( QString sDirId, dirList )
		lst.unite( getSessionListByVm( sDirId, vmIdent.first, accessRights ) );
	return lst;
}

QHash< IOSender::Handle, SmartPtr<CDspClient> > CDspClientManager::getSessionListByVm(
		const QString& vmDirUuid, const QString& vmUuid, int accessRights) const
{
	bool bAllClients = vmDirUuid == m_service->
				getVmDirManager().getVzDirectoryUuid();

	QHash< IOSender::Handle, SmartPtr<CDspClient> > sessions =
				bAllClients ? getSessionsListSnapshot( ) :
					getSessionsListSnapshot( vmDirUuid );

	CDspLockedPointer<CVmDirectoryItem>
		pLockedVmDirItem = m_service->getVmDirManager().getVmDirItemByUuid(vmDirUuid, vmUuid);

	if ( !pLockedVmDirItem )
	{
		WRITE_TRACE(DBG_FATAL, "CDspClientManager::getSessionsListByVm: Can't found vm by vmId=%s, dirId=%s)",
				QSTR2UTF8( vmUuid ),
				QSTR2UTF8( vmDirUuid )
			   );
		return QHash< IOSender::Handle, SmartPtr<CDspClient> > ();
	}


	QMutableHashIterator< IOSender::Handle, SmartPtr<CDspClient> > it( sessions );
	while( it.hasNext() )
	{
		SmartPtr<CDspClient> pClient = it.next().value();
		CDspAccessManager::VmAccessRights vmAccess = m_service->getAccessManager()
			.getAccessRightsToVm( pClient, pLockedVmDirItem.getPtr() );

		// #8179
		// vm is invalid (permission is absent), but information present in map
		if( vmAccess.isExists()
			&& ((unsigned int )(vmAccess.getVmAccessRights() & accessRights) != (unsigned int )accessRights) )
			it.remove();
	}

	return sessions;
}

static bool isCommandAllowedWithoutInitComplete( PVE::IDispatcherCommands cmd )
{
	switch(cmd)
	{
	case PVE::DspCmdUserLogin:
	case PVE::DspCmdUserEasyLoginLocal:
	case PVE::DspCmdUserLogoff:
	case PVE::DspCmdAllHostUsers:
	case PVE::DspCmdUserGetLicenseInfo:
	case PVE::DspCmdDirGetVmList:
	case PVE::DspCmdVmGetConfig:
	case PVE::DspCmdGetVmConfigById:
	case PVE::DspCmdGetVmInfo:
		return true;
	default:
		return false;
	}
}

void CDspClientManager::handleToDispatcherPackage (
		const IOSender::Handle& h,
		const SmartPtr<IOPackage>& p )
{
	static LogRateLimit rlr = {1, (unsigned int)-1};
	static unsigned int saved_type = PVE::DspIllegalCommand;

	WRITE_TRACE( (p->header.type == PVE::DspIllegalCommand
				  || p->header.type != saved_type
			|| LogCheckModifyRate(&rlr))? DBG_FATAL: DBG_DEBUG,
				 "Processing command '%s' %d (%s)",
				 PVE::DispatcherCommandToString(p->header.type),
				 p->header.type,
				 PRL_JOC_TO_STRING(
					 DispatcherCmdsToJobTypeConverter::Convert((PVE::IDispatcherCommands )p->header.type))
				 );
	saved_type = p->header.type;

	do
	{
		if(m_service->isFirstInitPhaseCompleted())
			break;

		if (!m_service->waitForInitCompletion())
		{
			WRITE_TRACE(DBG_FATAL, "Timeout is over ! Service initialization was not done !" );
			m_service->sendSimpleResponseToClient( h, p, PRL_ERR_TIMEOUT);
			return;
		}
		break;

		if( isCommandAllowedWithoutInitComplete( (PVE::IDispatcherCommands)p->header.type ) )
			break;

		SmartPtr<CDspClient> pClient = getUserSession(h);
		PRL_ASSERT( pClient ); // always should be
		if( !pClient )
		{
			WRITE_TRACE(DBG_FATAL, "Client %s not found.", QSTR2UTF8(h) );
			m_service->sendSimpleResponseToClient(h, p, PRL_ERR_FAILURE);
			return;
		}

		m_service->getTaskManager().schedule(new Task_PendentClientRequest( pClient, p ));
		return;

	}while(0);

 	PERF_COUNT_ATOMIC_INC( dsp_commands ) ;

	switch( p->header.type )
	{
		case PVE::DspCmdUserLogin:
		{
			if (m_service->isServerStopping())
			{
				WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
				m_service->sendSimpleResponseToClient( h, p, PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
				return;
			}

			m_rwLock.lockForRead();
			bool cliExists = m_clients.contains(h);
			m_rwLock.unlock();

			if ( cliExists )
				m_service->sendSimpleResponseToClient(h, p, PRL_ERR_USER_IS_ALREADY_LOGGED);
			else
			{
				if (!CaptureLogonClient(h))
				{
					WRITE_TRACE(DBG_FATAL, "Client logon actions reached up limit!");
					m_service->
						sendSimpleResponseToClient( h, p, PRL_ERR_DISP_LOGON_ACTIONS_REACHED_UP_LIMIT);
					return;
				}

				bool bWasPreAuthorized = false;
				SmartPtr<CDspClient> pClient =
					m_service->getUserHelper().processUserLogin(h, p, bWasPreAuthorized);
				if ( pClient )
				{
					m_rwLock.lockForWrite();
					if (m_clients.isEmpty()
 						/* Check to prevent lock HwInfo mutex before dispatcher init completed */
						 && m_service->isFirstInitPhaseCompleted()
					)
					{
						m_service->getHwMonitorThread().forceCheckHwChanges();
					}
					m_clients[h] = pClient;
					//Erase client from pre authorized queue if any
					m_preAuthorizedSessions.remove(h);
					m_rwLock.unlock();

					// Register user for change permissions monitoring
					m_service->getVmConfigWatcher().addUserToMonitoringPermChanges( h );

					SmartPtr<IOPackage> response = m_service->getUserHelper()
						.makeLoginResponsePacket(pClient, p);
					pClient->sendPackage( response );
					WRITE_TRACE(DBG_FATAL, "Session with uuid[ %s ] was started.", QSTR2UTF8( h ) );
				}
				else if ( bWasPreAuthorized )
				{
					m_rwLock.lockForWrite();
					m_preAuthorizedSessions.insert(h);
					m_rwLock.unlock();
					m_service->sendSimpleResponseToClient(h, p, PRL_ERR_SUCCESS);
				}
				else
				{
					// Error response should be sent by user helper,
					// so do nothing!
				}

				ReleaseLogonClient(h);

				return;
			}
		}
		break;
		case PVE::DspCmdUserEasyLoginLocal:
			{
				m_rwLock.lockForRead();
				bool cliExists = m_clients.contains(h);
				m_rwLock.unlock();

				if ( cliExists )
					m_service->sendSimpleResponseToClient(h, p, PRL_ERR_USER_IS_ALREADY_LOGGED);
				else
				{
					if (!CaptureLogonClient(h))
					{
						WRITE_TRACE(DBG_FATAL, "Client logon actions reached up limit!");
						m_service->
							sendSimpleResponseToClient( h, p, PRL_ERR_DISP_LOGON_ACTIONS_REACHED_UP_LIMIT);
						return;
					}

					SmartPtr<CDspClient> client =
						m_service->getUserHelper().processUserLoginLocal( h, p );
					if ( client )
					{
						m_rwLock.lockForWrite();
						if (m_clients.isEmpty()
 						/* Check to prevent lock HwInfo mutex before dispatcher init completed */
						 && m_service->isFirstInitPhaseCompleted()
						)
						{
							m_service->getHwMonitorThread().forceCheckHwChanges();
						}
						m_clients[h] = client;
						m_rwLock.unlock();

						// Register user for change permissions monitoring
						m_service->getVmConfigWatcher().addUserToMonitoringPermChanges(h);

						// bug#9058
						// m_service->getVmDirHelper().recoverMixedVmPermission( client );

						SmartPtr<IOPackage> response = m_service->getUserHelper()
							.makeLoginResponsePacket(client, p);
						client->sendPackage( response );

						WRITE_TRACE(DBG_FATAL, "Session with uuid[ %s ] was started.", QSTR2UTF8( h ) );
					}
					else
					{
						// Error response should be sent by user helper,
						// so do nothing!
					}

					ReleaseLogonClient(h);
				}

				return;
			}
	}// switch

	// Package should be authorized!
	m_rwLock.lockForRead();
	bool cliExists = m_clients.contains(h);
	bool inPreAuthorizedHash = m_preAuthorizedSessions.contains(h);

	// Check authorization
	if ( ! cliExists ) {
		m_rwLock.unlock();

		if ( inPreAuthorizedHash && PVE::DspCmdAllHostUsers == p->header.type )
		{
			m_service->getUserHelper().sendAllHostUsers(h, p);
		}
		else
		{
			// Send error
			m_service->sendSimpleResponseToClient( h, p, PRL_ERR_USER_OPERATION_NOT_AUTHORISED );
		}
		return;
	}

	// #455781 under read access (QReadLocker) we should call only const methods
	// to prevent app crash after simultaneously call QT_CONT::detach_helper().
	SmartPtr<CDspClient> client = m_clients.value(h);

	m_rwLock.unlock();

#ifdef _CT_
	if (m_service->getVzHelper()->handlePackage(h, client, p))
		return;
#endif

	////////////////////////////////////////////////////////////////////////////
	// check in LogicGuard
	////////////////////////////////////////////////////////////////////////////
	CVmEvent errParams;
	PRL_RESULT ret = m_logicGuard.isCommandAllowed( client, p, errParams );
	if ( PRL_FAILED( ret ) )
	{
		if ( errParams.getEventCode() != ret )
			WRITE_TRACE(DBG_FATAL, "errParams.getEventCode(): %.8X '%s' and ret is: %.8X '%s'",\
							errParams.getEventCode(), PRL_RESULT_TO_STRING(errParams.getEventCode()),\
							ret, PRL_RESULT_TO_STRING(ret) );
		PRL_ASSERT( errParams.getEventCode() == ret );
		if( errParams.getEventCode() != ret )
			errParams.setEventCode( ret );
		WRITE_TRACE(DBG_FATAL, "Command processing stopped by reason %s (error = %#x)", PRL_RESULT_TO_STRING( ret ), ret );
		client->sendResponseError( errParams, p );
		return;
	}


	// Ok, client has been authed!
	// Do next checks ...
	switch( p->header.type )
	{
	///////////////////////////////////////////////
	case PVE::DspCmdUserLogoff:
		return (void)m_service->getUserHelper().processUserLogoff(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdUserGetHostHwInfo:
		return (void)m_service->getTaskManager()
			.schedule(new Task_SendHostHardwareInfo(client, p));
	///////////////////////////////////////////////
	case PVE::DspCmdUserGetProfile:
		return (void)m_service->getUserHelper().sendUserProfile(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdUserInfoList:
		return (void)m_service->getUserHelper().sendUserInfoList(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdGetVirtualNetworkList:
		return (void)m_service->getShellServiceHelper().sendVirtualNetworkList(client, p);
	case PVE::DspCmdGetNetworkClassesConfig:
		return (void)m_service->getShellServiceHelper().sendNetworkClassesConfig(client, p);
	case PVE::DspCmdGetNetworkShapingConfig:
		return (void)m_service->getShellServiceHelper().sendNetworkShapingConfig(client, p);
	case PVE::DspCmdGetIPPrivateNetworksList:
		return (void)m_service->getShellServiceHelper().sendIPPrivateNetworksList(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdUserInfo:
		return (void)m_service->getUserHelper().sendUserInfo(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdUserProfileBeginEdit:
		return (void)m_service->getUserHelper().userProfileBeginEdit(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdUserProfileCommit:
		return (void)m_service->getUserHelper().userProfileCommit(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdGetHostCommonInfo:
		return (void)m_service->getShellServiceHelper().sendHostCommonInfo(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdUserUpdateLicense:
		return (void)m_service->getShellServiceHelper().updateLicense(client, p);
	case PVE::DspCmdUserGetLicenseInfo:
		return (void)m_service->getShellServiceHelper().sendLicenseInfo(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirGetVmList:
		return (void)m_service->getVmDirHelper().sendVmList(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdGetVmConfigById:
		return (void)m_service->getVmDirHelper().findVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmGetConfig:
		return (void)m_service->getVmDirHelper().sendVmConfig(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdGetVmInfo:
		return (void)m_service->getVmDirHelper().sendVmInfo(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdGetVmToolsInfo:
		return (void)m_service->getVmDirHelper().sendVmToolsInfo(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdGetVmVirtDevInfo:
		// deprecated call, older PMC accepts this failure silently
		return (void)m_service->sendSimpleResponseToClient(h, p, PRL_ERR_UNIMPLEMENTED);
	///////////////////////////////////////////////
	case PVE::DspCmdDirVmCreate:
		return (void)m_service->getVmDirHelper().createNewVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirRegVm:
		return (void)m_service->getVmDirHelper().registerVm(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirReg3rdPartyVm: {
		CVmEvent evt;
		evt.setEventCode( PRL_ERR_UNIMPLEMENTED );
		client->sendResponseError( evt, p );
		return;
	}
	///////////////////////////////////////////////
	case PVE::DspCmdDirRestoreVm:
		return (void)m_service->getVmDirHelper().restoreVm(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirVmClone:
		return (void)m_service->getVmDirHelper().cloneVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirVmMigrate:
		return (void)m_service->getVmDirHelper().migrateVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdCreateVmBackup:
		return (void)m_backup.startCreateVmBackupSourceTask(client, p);
	case PVE::DspCmdBeginVmBackup:
		return (void)m_backup.launchBeginVmBackup(client, p);
	case PVE::DspCmdEndVmBackup:
		return (void)m_backup.launchEndVeBackup(client, p);
	case PVE::DspCmdRestoreVmBackup:
		return (void)m_backup.startRestoreVmBackupTargetTask(client, p);
	case PVE::DspCmdGetBackupTree:
		return (void)m_backup.startGetBackupTreeSourceTask(client, p);
	case PVE::DspCmdRemoveVmBackup:
		return (void)m_backup.startRemoveVmBackupSourceTask(client, p);
	case PVE::DspCmdDirVmDelete:
		return (void)m_service->getVmDirHelper().deleteVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirUnregVm:
		return (void)m_service->getVmDirHelper().unregVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirCreateImage:
		return (void)m_service->getVmDirHelper().createNewImage(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirCopyImage:
		return (void)m_service->getVmDirHelper().copyImage(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirVmEditBegin:
		return (void)m_service->getVmDirHelper().beginEditVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdDirVmEditCommit:
		return (void)m_service->getVmDirHelper().editVm(h, client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdStartSearchConfig:
		return (void)m_service->getVmDirHelper().searchLostConfigs(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmLock:
		return (void)m_service->getVmDirHelper().lockVm(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmUnlock:
		return (void)m_service->getVmDirHelper().unlockVm(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmResizeDisk:
		return (void)m_service->getVmDirHelper().resizeDiskImage(client, p);
	case PVE::DspCmdConvertOldHdd:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	case PVE::DspCmdUpdateDeviceInfo:
		return (void)m_service->getVmDirHelper().UpdateDeviceInfo(h,client, p);
	case PVE::DspCmdVmCreateUnattendedFloppy:
	case PVE::DspCmdCreateUnattendedCd:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	case PVE::DspCmdVmUpdateSecurity:
		return (void)m_service->getVmDirHelper().updateVmSecurityInfo(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdSubscribeToHostStatistics: {
		CDspStatCollectingThread::SubscribeToHostStatistics(client);
		client->sendSimpleResponse(p, PRL_ERR_SUCCESS);
		return;
	}
	///////////////////////////////////////////////
	case PVE::DspCmdUnsubscribeFromHostStatistics: {
		CDspStatCollectingThread::UnsubscribeFromHostStatistics(client);
		client->sendSimpleResponse(p, PRL_ERR_SUCCESS);
		return;
	}
	///////////////////////////////////////////////
	case PVE::DspCmdGetHostStatistics:
		return (void)m_service->getShellServiceHelper().sendHostStatistics( client, p );
	///////////////////////////////////////////////
	case PVE::DspCmdVmSubscribeToGuestStatistics: {
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSubscribeToGuestStatistics,
			UTF8_2QSTR(p->buffers[0].getImpl()));
		if (!pCmd->IsValid())
		{
			client->sendSimpleResponse(p, PRL_ERR_UNRECOGNIZED_REQUEST);
			return;
		}
		PRL_RESULT rc = PRL_ERR_FAILURE;
		rc = CDspStatCollectingThread::SubscribeToVmGuestStatistics(pCmd->GetVmUuid(), client);
		client->sendSimpleResponse(p, rc);
		return;
	}
	///////////////////////////////////////////////
	case PVE::DspCmdVmUnsubscribeFromGuestStatistics: {
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUnsubscribeFromGuestStatistics,
			UTF8_2QSTR(p->buffers[0].getImpl()));
		if (!pCmd->IsValid())
		{
			client->sendSimpleResponse(p, PRL_ERR_UNRECOGNIZED_REQUEST);
			return;
		}
		PRL_RESULT rc = PRL_ERR_FAILURE;
		rc = CDspStatCollectingThread::UnsubscribeFromVmGuestStatistics(pCmd->GetVmUuid(), client);
		client->sendSimpleResponse(p, rc);
		return;
	}
	///////////////////////////////////////////////
	case PVE::DspCmdVmGetStatistics:
		return (void)m_service->getShellServiceHelper().sendGuestStatistics(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdPerfomanceStatistics:
		return (void)CDspStatCollectingThread::ProcessPerfStatsCommand(client, p) ;
	///////////////////////////////////////////////
	case PVE::DspCmdHostCommonInfoBeginEdit:
		return (void)m_service->getShellServiceHelper().hostCommonInfoBeginEdit(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdHostCommonInfoCommit:
		return (void)m_service->getShellServiceHelper().hostCommonInfoCommit(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdNetPrlNetworkServiceStart:
	case PVE::DspCmdNetPrlNetworkServiceStop:
	case PVE::DspCmdNetPrlNetworkServiceRestart:
	case PVE::DspCmdNetPrlNetworkServiceRestoreDefaults:
	case PVE::DspCmdAddNetAdapter:
	case PVE::DspCmdDeleteNetAdapter:
	case PVE::DspCmdUpdateNetAdapter:
	case PVE::DspCmdAddVirtualNetwork:
	case PVE::DspCmdUpdateVirtualNetwork:
	case PVE::DspCmdDeleteVirtualNetwork:
	case PVE::DspCmdUpdateNetworkClassesConfig:
	case PVE::DspCmdUpdateNetworkShapingConfig:
	case PVE::DspCmdRestartNetworkShaping:
	case PVE::DspCmdAddIPPrivateNetwork:
	case PVE::DspCmdUpdateIPPrivateNetwork:
	case PVE::DspCmdRemoveIPPrivateNetwork:
		return (void)m_service->getShellServiceHelper().managePrlNetService(client, p);
	case PVE::DspCmdGetNetServiceStatus:
		return (void)m_service->getShellServiceHelper().sendNetServiceStatus(client, p);
	case PVE::DspCmdUserCancelOperation:
		return (void)m_service->getShellServiceHelper().cancelOperation(client, p);

	///////////////////////////////////////////////
	/// Fs commands
	///////////////////////////////////////////////
	case PVE::DspCmdFsGetDiskList:
		return (void)m_service->getShellServiceHelper().sendDiskEntries(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdFsGetDirectoryEntries:
		return (void)m_service->getShellServiceHelper().sendDirectoryEntries(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdFsCreateDirectory:
		return (void)m_service->getShellServiceHelper().createDirectoryEntry(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdFsRenameEntry:
		return (void)m_service->getShellServiceHelper().renameFsEntry(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdFsRemoveEntry:
		return (void)m_service->getShellServiceHelper().removeFsEntry(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdFsCanCreateFile:
		return (void)m_service->getShellServiceHelper().canCreateFile(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdFsGenerateEntryName:
		return (void)m_service->getShellServiceHelper().generateFsEntryName(client, p);
	case PVE::DspCmdAttachToLostTask:
		return (void)m_service->getShellServiceHelper().attachToLostTask(client, p);
	///////////////////////////////////////////////
	//// SMC commands
	///////////////////////////////////////////////
	case PVE::DspCmdSMCGetDispatcherRTInfo:
		return (void)m_service->getDispMonitor().GetDispRTInfo(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdSMCGetCommandHistoryByVm:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	///////////////////////////////////////////////
	case PVE::DspCmdSMCGetCommandHistoryByUser:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	///////////////////////////////////////////////
	case PVE::DspCmdSMCDisconnectUser:
		return (void)m_service->getDispMonitor().ForceDisconnectUser(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdSMCDisconnectAllUsers:
		return (void)m_service->getDispMonitor().ForceDisconnectAllUsers(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdSMCCancelUserCommand: {
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( PVE::DspCmdSMCCancelUserCommand, UTF8_2QSTR(p->buffers[0].getImpl()) );
		if ( ! pCmd->IsValid() )
		{
			client->sendSimpleResponse(p, PRL_ERR_UNRECOGNIZED_REQUEST);
			return;
		}
		QString taskUuid = pCmd->GetFirstStrParam();
		return (void)m_service->getDispMonitor().ForceCancelCommandOfUser(client, p, taskUuid);
	}
	///////////////////////////////////////////////
	case PVE::DspCmdSMCShutdownDispatcher:
		return (void)m_service->getDispMonitor().ShutdownDispatcher(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmGetProblemReport:
	case PVE::DspCmdVmGetPackedProblemReport:
	case PVE::DspCmdSendProblemReport:
		return (void)m_service->getTaskManager()
			.schedule(new Task_CreateProblemReport( client, p ));
	///////////////////////////////////////////////
	case PVE::DspCmdVmSectionValidateConfig:
		return (void)CVmValidateConfig::validateSectionConfig(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmGetSuspendedScreen:
		return (void)m_service->getVmDirHelper().getSuspendedVmScreen(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdVmGetSnapshotsTree:
		return (void)m_service->getTaskManager()
			.schedule(new Task_SendSnapshotTree(client, p));
	///////////////////////////////////////////////
	case PVE::DspCmdVmUpdateSnapshotData:
		return (void)m_service->getVmSnapshotStoreHelper().updateSnapshotData(client, p);
	///////////////////////////////////////////////
	case PVE::DspCmdSetNonInteractiveSession:
		return (void)m_service->getUserHelper().setNonInteractiveSession(client, p);
	case PVE::DspCmdSetSessionConfirmationMode:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	case PVE::DspCmdStorageSetValue:
		return (void)m_service->getShellServiceHelper().changeServerInternalValue(client, p);
	case PVE::DspCmdUpdateUsbAssocList:
		return (void)m_service->getShellServiceHelper().updateUsbAssociationsList(client, p);
	case PVE::DspCmdStartClusterService:
	case PVE::DspCmdStopClusterService:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	case PVE::DspCmdVmConvertDisks:
		return (void)m_service->getVmDirHelper().startConvertDisks(client, p);
	case PVE::DspCmdVmMount:
		return (void)m_service->getVmDirHelper().mountVm(client, p);
	case PVE::DspCmdVmUmount:
		return (void)m_service->getVmDirHelper().umountVm(client, p);
	case PVE::DspCmdGetDiskFreeSpace:
		return (void)m_service->getShellServiceHelper().sendDiskFreeSpace(client, p);
	case PVE::DspCmdDirVmMove:
		return (void)m_service->getVmDirHelper().moveVm(client, p);
	case PVE::DspCmdGetCPUPoolsList:
		return m_service->getShellServiceHelper().sendCPUPoolsList(client, p);
	case PVE::DspCmdMoveToCPUPool:
		return m_service->getShellServiceHelper().moveToCPUPool(client, p);
	case PVE::DspCmdRecalculateCPUPool:
		return m_service->getShellServiceHelper().recalculateCPUPool(client, p);
	case PVE::DspCmdJoinCPUPool:
		return m_service->getShellServiceHelper().joinCPUPool(client, p);
	case PVE::DspCmdLeaveCPUPool:
		return m_service->getShellServiceHelper().leaveCPUPool(client, p);
	case PVE::DspCmdVmCommitEncryption:
		return (void)client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
	} //switch( p->header.type )
	///////////////////////////////////////////////

	if (!CDspRouter::instance().routePackage(this, h, p))
	{
		// Send error
		m_service->sendSimpleResponseToClient(h, p, PRL_ERR_UNIMPLEMENTED);
		PERF_COUNT_ATOMIC_INC(dsp_error_commands);
		return;
	}
}

void CDspClientManager::handleFromDispatcherPackage (
	const SmartPtr<CDspHandler> &,
	const IOSender::Handle&,
	const SmartPtr<IOPackage> &p)
{
	//TODO add check on VMs access here
	sendPackageToAllClients(p);
}

void CDspClientManager::handleFromDispatcherPackage (
	const SmartPtr<CDspHandler>&,
	const IOSender::Handle&,
	const IOSender::Handle &hReceiver,
	const SmartPtr<IOPackage> &p )
{
	m_service->getIOServer().sendPackage(hReceiver, p);
}

void CDspClientManager::handleClientStateChanged ( const IOSender::Handle&,
												  IOSender::State  )
{
}

void CDspClientManager::handleDetachClient (
	const IOSender::Handle&,
	const IOCommunication::DetachedClient& )
{
	// Never should be called for this handler
	PRL_ASSERT(0);
}

QList< IOSendJob::Handle > CDspClientManager::sendPackageToVmClients(
	const SmartPtr<IOPackage>& p, const QString& vmDirUuid, const QString& vmUuid )
{
	QList< SmartPtr<CDspClient> >
		clientList = getSessionListByVm( vmDirUuid, vmUuid ).values();

	return sendPackageToClientList( p, clientList );
}

QList< IOSendJob::Handle > CDspClientManager::sendPackageToAllClients( const SmartPtr<IOPackage>& p )
{
	QList< SmartPtr<CDspClient> > sessions = getSessionsListSnapshot().values();
	return sendPackageToClientList( p, sessions );
}

QList< IOSendJob::Handle > CDspClientManager::sendPackageToClientList(
	const SmartPtr<IOPackage>& p,
	QList< SmartPtr<CDspClient> >& clientList )
{
	QList<IOSendJob::Handle> jobs;
	QList< SmartPtr<CDspClient> >::iterator client = clientList.begin();
	for (; client != clientList.end(); ++client)
	{
		IOSendJob::Handle job = (*client)->sendPackage(p);
		jobs.append( job );
	}

	return jobs;
}

bool CDspClientManager::CaptureLogonClient(const IOSender::Handle& h)
{
	unsigned int nLimit = m_service->getDispConfigGuard()
		.getDispWorkSpacePrefs()->getLimits()->getMaxLogonActions();

	QWriteLocker locker( &m_rwLock );

	if (m_setLogonClients.size() >= (int )nLimit)
	{
		return false;
	}

	m_setLogonClients.insert(h);
	return true;
}

void CDspClientManager::ReleaseLogonClient(const IOSender::Handle& h)
{
	QWriteLocker locker( &m_rwLock );
	m_setLogonClients.remove(h);
}

bool CDspClientManager::isPreAuthorized(const IOSender::Handle& h)
{
	QReadLocker locker( &m_rwLock );
	return m_preAuthorizedSessions.contains(h);
}

/*****************************************************************************/
