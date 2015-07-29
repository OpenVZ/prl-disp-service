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

#include "CProtoSerializer.h"
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
#include "CDspRequestsToVmHandler.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtilsBase/CommandConvHelper.h"

#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "CDspVzHelper.h"

using namespace Parallels;

REGISTER_HANDLER( IOService::IOSender::Client,
		"ClientHandler",
		CDspClientManager );

/*****************************************************************************/

CDspClientManager::CDspClientManager ( IOSender::Type type, const char* name ):
	CDspHandler(type, name),
	dsp_commands(NULL), dsp_error_commands(NULL)
{
}

CDspClientManager::~CDspClientManager ()
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
    create_counter(CDspService::instance()->getBasePerfStorage(),
		PERF_COUNT_TYPE_INC "mgmt.commands", &dsp_commands) ;

	/*****************************************************************************************
	 * $PERF$ "mgmt.error_commands"
	 * Total count of failed management commands, recieved by Dispatcher
	 *****************************************************************************************/
    create_counter(CDspService::instance()->getBasePerfStorage(),
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
	CDspService::instance()->getIOServer().disconnectClient( h );
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
		CDspService::instance()->getVmManager().shutdownVmsByClient( pUser, false );
#endif

	if ( !CDspService::instance()->isServerStopping() )//https://bugzilla.sw.ru/show_bug.cgi?id=444674
	{
		if (pUser.isValid())
		{
			CDspStatCollectingThread::ClientDisconnected(pUser) ;

			QList< SmartPtr<CDspTaskHelper> >
				lstTasks = CDspService::instance()->getTaskManager().getTaskListBySession( h );
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

			CDspService::instance()->getVmManager().checkToSendDefaultAnswer();
			CDspVm::globalCleanupGuestOsSessions( h );
			CDspService::instance()->getVmDirHelper().cleanupSessionVmLocks(h);
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
			pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid(vmIdent);
		if( !pVmDirItem )
			return lst;
		dirList = CDspService::instance()->getVmDirManager().findVmDirItemsInCatalogue(
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
	bool bAllClients = vmDirUuid == CDspService::instance()->
				getVmDirManager().getVzDirectoryUuid();

	QHash< IOSender::Handle, SmartPtr<CDspClient> > sessions =
				bAllClients ? getSessionsListSnapshot( ) :
					getSessionsListSnapshot( vmDirUuid );

	CDspLockedPointer<CVmDirectoryItem>
		pLockedVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid( vmDirUuid, vmUuid );

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
		CDspAccessManager::VmAccessRights vmAccess = CDspService::instance()->getAccessManager()
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
	case PVE::DspCmdUserLoginLocal:
	case PVE::DspCmdUserLoginLocalStage2:
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
		if( CDspService::instance()->isFirstInitPhaseCompleted() )
			break;

		if ( !CDspService::instance()->waitForInitCompletion() )
		{
			WRITE_TRACE(DBG_FATAL, "Timeout is over ! Service initialization was not done !" );
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_TIMEOUT);
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
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_FAILURE);
			return;
		}

		CDspService::instance()->getTaskManager().schedule(new Task_PendentClientRequest( pClient, p ));
		return;

	}while(0);

 	PERF_COUNT_ATOMIC_INC( dsp_commands ) ;

	switch( p->header.type )
	{
		case PVE::DspCmdUserLogin:
		{
			if (CDspService::instance()->isServerStopping())
			{
				WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
				CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
				return;
			}

			m_rwLock.lockForRead();
			bool cliExists = m_clients.contains(h);
			m_rwLock.unlock();

			if ( cliExists )
				CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_USER_IS_ALREADY_LOGGED );
			else
			{
				if (!CaptureLogonClient(h))
				{
					WRITE_TRACE(DBG_FATAL, "Client logon actions reached up limit!");
					CDspService::instance()->
						sendSimpleResponseToClient( h, p, PRL_ERR_DISP_LOGON_ACTIONS_REACHED_UP_LIMIT);
					return;
				}

				bool bWasPreAuthorized = false;
				SmartPtr<CDspClient> pClient =
					CDspService::instance()->getUserHelper().processUserLogin( h, p, bWasPreAuthorized );
				if ( pClient )
				{
					m_rwLock.lockForWrite();
					if (m_clients.isEmpty()
 						/* Check to prevent lock HwInfo mutex before dispatcher init completed */
						 && CDspService::instance()->isFirstInitPhaseCompleted()
					)
					{
						CDspService::instance()->getHwMonitorThread().forceCheckHwChanges();
					}
					m_clients[h] = pClient;
					//Erase client from pre authorized queue if any
					m_preAuthorizedSessions.remove(h);
					m_rwLock.unlock();

					// Register user for change permissions monitoring
					CDspService::instance()->getVmConfigWatcher().addUserToMonitoringPermChanges( h );

					SmartPtr<IOPackage> response = CDspService::instance()->getUserHelper()
						.makeLoginResponsePacket( pClient, p);
					pClient->sendPackage( response );
					WRITE_TRACE(DBG_FATAL, "Session with uuid[ %s ] was started.", QSTR2UTF8( h ) );

					sendQuestionToClient(pClient);
				}
				else if ( bWasPreAuthorized )
				{
					m_rwLock.lockForWrite();
					m_preAuthorizedSessions.insert(h);
					m_rwLock.unlock();
					CDspService::instance()->
						sendSimpleResponseToClient( h, p, PRL_ERR_SUCCESS);
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
		case PVE::DspCmdUserLoginLocal:
			{
				m_rwLock.lockForRead();
				bool cliExists = m_clients.contains(h);
				m_rwLock.unlock();

				if ( cliExists )
					CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_USER_IS_ALREADY_LOGGED );
				else
				{
					if (!CaptureLogonClient(h))
					{
						WRITE_TRACE(DBG_FATAL, "Client logon actions reached up limit!");
						CDspService::instance()->
							sendSimpleResponseToClient( h, p, PRL_ERR_DISP_LOGON_ACTIONS_REACHED_UP_LIMIT);
						return;
					}

					CDspService::instance()->getUserHelper().processUserLoginLocal( h, p );

					ReleaseLogonClient(h);
				}
				return;
			}
			break;
		case PVE::DspCmdUserLoginLocalStage2:
			{
				if (CDspService::instance()->isServerStopping())
				{
					WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
					CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
					return;
				}

				m_rwLock.lockForRead();
				bool cliExists = m_clients.contains(h);
				m_rwLock.unlock();

				if ( cliExists )
					CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_USER_IS_ALREADY_LOGGED );
				else
				{
					if (!CaptureLogonClient(h))
					{
						WRITE_TRACE(DBG_FATAL, "Client logon actions reached up limit!");
						CDspService::instance()->
							sendSimpleResponseToClient( h, p, PRL_ERR_DISP_LOGON_ACTIONS_REACHED_UP_LIMIT);
						return;
					}

					SmartPtr<CDspClient> client =
						CDspService::instance()->getUserHelper().processUserLoginLocalStage2( h, p );
					if ( client )
					{
						m_rwLock.lockForWrite();
						if (m_clients.isEmpty()
 						/* Check to prevent lock HwInfo mutex before dispatcher init completed */
						 && CDspService::instance()->isFirstInitPhaseCompleted()
						)
						{
							CDspService::instance()->getHwMonitorThread().forceCheckHwChanges();
						}
						m_clients[h] = client;
						m_rwLock.unlock();

						// Register user for change permissions monitoring
						CDspService::instance()->getVmConfigWatcher().addUserToMonitoringPermChanges( h );

						// bug#9058
						// CDspService::instance->getVmDirHelper().recoverMixedVmPermission( client );

						SmartPtr<IOPackage> response = CDspService::instance()->getUserHelper()
							.makeLoginResponsePacket( client, p);
						client->sendPackage( response );

						WRITE_TRACE(DBG_FATAL, "Session with uuid[ %s ] was started.", QSTR2UTF8( h ) );

						sendQuestionToClient(client);
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
			CDspService::instance()->getUserHelper().sendAllHostUsers( h, p );
		}
		else
		{
			// Send error
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_USER_OPERATION_NOT_AUTHORISED );
		}
		return;
	}

	// #455781 under read access (QReadLocker) we should call only const methods
	// to prevent app crash after simultaneously call QT_CONT::detach_helper().
	SmartPtr<CDspClient> client = m_clients.value(h);

	m_rwLock.unlock();

#ifdef _CT_
	if (CDspService::instance()->getVzHelper()->handlePackage(h, client, p))
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
	case PVE::DspCmdUserLogoff: {
		CDspService::instance()->getUserHelper().processUserLogoff( client, p );
		return;
								}
								///////////////////////////////////////////////
	case PVE::DspCmdUserGetHostHwInfo:
		return (void)CDspService::instance()->getTaskManager()
			.schedule(new Task_SendHostHardwareInfo( client, p ));
									   ///////////////////////////////////////////////
	case PVE::DspCmdUserGetProfile: {
		CDspService::instance()->getUserHelper().sendUserProfile( h, client, p );
		return;
									}
									///////////////////////////////////////////////
	case PVE::DspCmdUserInfoList: {
		CDspService::instance()->getUserHelper().sendUserInfoList( h, client, p );
		return;
								  }
								  ///////////////////////////////////////////////
	case PVE::DspCmdGetVirtualNetworkList: {
		CDspService::instance()->getShellServiceHelper().sendVirtualNetworkList( client, p );
		return;
		}
	case PVE::DspCmdGetNetworkClassesConfig: {
		CDspService::instance()->getShellServiceHelper().sendNetworkClassesConfig( client, p );
		return;
		}

	case PVE::DspCmdGetNetworkShapingConfig: {
		CDspService::instance()->getShellServiceHelper().sendNetworkShapingConfig( client, p );
		return;
		}
	case PVE::DspCmdGetIPPrivateNetworksList: {
		CDspService::instance()->getShellServiceHelper().sendIPPrivateNetworksList( client, p );
		return;
		}
								  ///////////////////////////////////////////////
	case PVE::DspCmdUserInfo: {
		CDspService::instance()->getUserHelper().sendUserInfo( h, client, p );
		return;
							  }
							  ///////////////////////////////////////////////
	case PVE::DspCmdUserProfileBeginEdit: {
		CDspService::instance()->getUserHelper().userProfileBeginEdit( h, client, p );
		return;
										  }
										  ///////////////////////////////////////////////
	case PVE::DspCmdUserProfileCommit: {
		CDspService::instance()->getUserHelper().userProfileCommit( h, client, p );
		return;
									   }
									   ///////////////////////////////////////////////
	case PVE::DspCmdGetHostCommonInfo: {
		CDspService::instance()->getShellServiceHelper().sendHostCommonInfo( client, p );
		return;
									   }
									   ///////////////////////////////////////////////
	case PVE::DspCmdUserUpdateLicense:
	case PVE::DspCmdUserGetLicenseInfo: {
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_UNIMPLEMENTED );
		return;
										}
									   ///////////////////////////////////////////////
	case PVE::DspCmdDirGetVmList: {
		CDspService::instance()->getVmDirHelper().sendVmList( h, client, p );
		return;
								  }
								  ///////////////////////////////////////////////
	case PVE::DspCmdGetVmConfigById: {
		CDspService::instance()->getVmDirHelper().findVm( h, client, p );
		return;
								  }
								  ///////////////////////////////////////////////
	case PVE::DspCmdVmGetConfig: {
		CDspService::instance()->getVmDirHelper().sendVmConfig( h, client, p);
		return;
								 }
								 ///////////////////////////////////////////////
	case PVE::DspCmdGetVmInfo: {
		CDspService::instance()->getVmDirHelper().sendVmInfo( h, client, p);
		return;
							   }
							   ///////////////////////////////////////////////
	case PVE::DspCmdGetVmToolsInfo: {
		CDspService::instance()->getVmDirHelper().sendVmToolsInfo( h, client, p);
		return;
									}
									///////////////////////////////////////////////
	case PVE::DspCmdGetVmVirtDevInfo: {
		// deprecated call, older PMC accepts this failure silently
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_UNIMPLEMENTED );
		return;
									}
									///////////////////////////////////////////////
	case PVE::DspCmdDirVmCreate: {
		CDspService::instance()->getVmDirHelper().createNewVm( h, client, p);
		return;
								 }
								 ///////////////////////////////////////////////
	case PVE::DspCmdDirRegVm: {
		CDspService::instance()->getVmDirHelper().registerVm( client, p);
		return;
							  }
							  ///////////////////////////////////////////////
	case PVE::DspCmdDirReg3rdPartyVm: {
		CVmEvent evt;
		evt.setEventCode( PRL_ERR_UNIMPLEMENTED );
		client->sendResponseError( evt, p );
		return;
							  }
							  ///////////////////////////////////////////////
	case PVE::DspCmdDirRestoreVm: {
		CDspService::instance()->getVmDirHelper().restoreVm( client, p);
		return;
							  }
							  ///////////////////////////////////////////////
	case PVE::DspCmdDirVmClone: {
		CDspService::instance()->getVmDirHelper().cloneVm( h, client, p);
		return;
								}
								///////////////////////////////////////////////
	case PVE::DspCmdDirVmMigrate: {
		CDspService::instance()->getVmDirHelper().migrateVm( h, client, p);
		return;
								  }
								  ///////////////////////////////////////////////
	case PVE::DspCmdCreateVmBackup:
	case PVE::DspCmdRestoreVmBackup:
	case PVE::DspCmdGetBackupTree:
	case PVE::DspCmdRemoveVmBackup:
		client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED );
		return;

	case PVE::DspCmdDirVmDelete: {
		CDspService::instance()->getVmDirHelper().deleteVm( h, client, p);
		return;
								 }
								 ///////////////////////////////////////////////
	case PVE::DspCmdDirUnregVm: {
		CDspService::instance()->getVmDirHelper().unregVm( h, client, p);
		return;
								}
								///////////////////////////////////////////////
	case PVE::DspCmdDirCreateImage: {
		CDspService::instance()->getVmDirHelper().createNewImage( h, client, p);
		return;
									}
									///////////////////////////////////////////////
	case PVE::DspCmdDirCopyImage: {
		CDspService::instance()->getVmDirHelper().copyImage( client, p);
		return;
									}
									///////////////////////////////////////////////
	case PVE::DspCmdDirVmEditBegin: {
		CDspService::instance()->getVmDirHelper().beginEditVm( h, client, p);
		return;
									}
									///////////////////////////////////////////////
	case PVE::DspCmdDirVmEditCommit: {
		CDspService::instance()->getVmDirHelper().editVm( h, client, p);
		return;
									 }
									 ///////////////////////////////////////////////
	case PVE::DspCmdStartSearchConfig: {
		CDspService::instance()->getVmDirHelper().searchLostConfigs( client, p);
		return;
									   }
									 ///////////////////////////////////////////////
	case PVE::DspCmdVmLock: {
		CDspService::instance()->getVmDirHelper().lockVm( client, p);
		return;
									   }
									 ///////////////////////////////////////////////
	case PVE::DspCmdVmUnlock: {
		CDspService::instance()->getVmDirHelper().unlockVm( client, p);
		return;
									   }
									 ///////////////////////////////////////////////
	case PVE::DspCmdVmResizeDisk:
		{
		CDspService::instance()->getVmDirHelper().resizeDiskImage( client, p);
		return;
		}
	case PVE::DspCmdConvertOldHdd:
		client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
		return;

	case PVE::DspCmdUpdateDeviceInfo:
		{
			CDspService::instance()->getVmDirHelper().UpdateDeviceInfo( h,client, p);
			return;
		}
	case PVE::DspCmdVmCreateUnattendedFloppy:
	case PVE::DspCmdCreateUnattendedCd:
		client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED);
		return;

	case PVE::DspCmdVmUpdateSecurity: {
		CDspService::instance()->getVmDirHelper().updateVmSecurityInfo( client, p );
		return;
									  }
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
	case PVE::DspCmdGetHostStatistics: {
		CDspService::instance()->getShellServiceHelper().sendHostStatistics( client, p );
		return;
									   }
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
	case PVE::DspCmdVmGetStatistics: {
		CDspService::instance()->getShellServiceHelper().sendGuestStatistics( client, p );
		return;
									 }
									 ///////////////////////////////////////////////
	case PVE::DspCmdPerfomanceStatistics: {
		CDspStatCollectingThread::ProcessPerfStatsCommand(client, p) ;
		return;
										  }
										  ///////////////////////////////////////////////
	case PVE::DspCmdHostCommonInfoBeginEdit: {
		CDspService::instance()->getShellServiceHelper().hostCommonInfoBeginEdit( client, p );
		return;
											 }
											 ///////////////////////////////////////////////
	case PVE::DspCmdHostCommonInfoCommit: {
		CDspService::instance()->getShellServiceHelper().hostCommonInfoCommit( client, p );
		return;
										  }
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
		CDspService::instance()->getShellServiceHelper().managePrlNetService( client, p );
		return;
	case PVE::DspCmdGetNetServiceStatus: {
		CDspService::instance()->getShellServiceHelper().sendNetServiceStatus( client, p );
		return;
										 }
	case PVE::DspCmdUserCancelOperation: {
		CDspService::instance()->getShellServiceHelper().cancelOperation( client, p );
		return;
										 }

										 ///////////////////////////////////////////////
										 /// Fs commands
										 ///////////////////////////////////////////////
	case PVE::DspCmdFsGetDiskList: {
		CDspService::instance()->getShellServiceHelper().sendDiskEntries( client, p );
		return;
								   }
								   ///////////////////////////////////////////////
	case PVE::DspCmdFsGetDirectoryEntries: {
		CDspService::instance()->getShellServiceHelper().sendDirectoryEntries( client, p );
		return;
										   }
										   ///////////////////////////////////////////////
	case PVE::DspCmdFsCreateDirectory: {
		CDspService::instance()->getShellServiceHelper().createDirectoryEntry( client, p );
		return;
									   }
									   ///////////////////////////////////////////////
	case PVE::DspCmdFsRenameEntry: {
		CDspService::instance()->getShellServiceHelper().renameFsEntry( client, p );
		return;
								   }
								   ///////////////////////////////////////////////
	case PVE::DspCmdFsRemoveEntry: {
		CDspService::instance()->getShellServiceHelper().removeFsEntry( client, p );
		return;
								   }
								   ///////////////////////////////////////////////
	case PVE::DspCmdFsCanCreateFile: {
		CDspService::instance()->getShellServiceHelper().canCreateFile( client, p );
		return;
									 }
									 ///////////////////////////////////////////////
	case PVE::DspCmdFsGenerateEntryName: {
		CDspService::instance()->getShellServiceHelper().generateFsEntryName( client, p );
		return;
										 }
	case PVE::DspCmdAttachToLostTask: {
		CDspService::instance()->getShellServiceHelper().attachToLostTask( client, p );
		return;
									  }
									  ///////////////////////////////////////////////
									  //// SMC commands
									  ///////////////////////////////////////////////
	case PVE::DspCmdSMCGetDispatcherRTInfo: {
		CDspService::instance()->getDispMonitor().GetDispRTInfo(client, p);
		return;
											}
											///////////////////////////////////////////////
	case PVE::DspCmdSMCGetCommandHistoryByVm: {
		client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED );
		return;
											  }
											  ///////////////////////////////////////////////
	case PVE::DspCmdSMCGetCommandHistoryByUser:	{
		client->sendSimpleResponse(p, PRL_ERR_UNIMPLEMENTED );
		return;
												}
												///////////////////////////////////////////////
	case PVE::DspCmdSMCDisconnectUser: {
		CDspService::instance()->getDispMonitor().ForceDisconnectUser(client, p);
		return;
									   }
									   ///////////////////////////////////////////////
	case PVE::DspCmdSMCDisconnectAllUsers: {
		CDspService::instance()->getDispMonitor().ForceDisconnectAllUsers(client, p);
		return;
										   }
										   ///////////////////////////////////////////////
	case PVE::DspCmdSMCCancelUserCommand: {
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( PVE::DspCmdSMCCancelUserCommand, UTF8_2QSTR(p->buffers[0].getImpl()) );
		if ( ! pCmd->IsValid() )
		{
			client->sendSimpleResponse(p, PRL_ERR_UNRECOGNIZED_REQUEST);
			return;
		}
		QString taskUuid = pCmd->GetFirstStrParam();
		CDspService::instance()->getDispMonitor().ForceCancelCommandOfUser(client, p, taskUuid);
		return;
										  }
										  ///////////////////////////////////////////////
	case PVE::DspCmdSMCShutdownDispatcher: {
		CDspService::instance()->getDispMonitor().ShutdownDispatcher(client, p);
		return;
										   }
										   //////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdVmGetProblemReport:
	case PVE::DspCmdVmGetPackedProblemReport:
	case PVE::DspCmdSendProblemReport:
		return (void)CDspService::instance()->getTaskManager()
			.schedule(new Task_CreateProblemReport( client, p ));
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdVmDropSuspendedState:
		{
			CDspService::instance()->getVmDirHelper().dropSuspendedState(h, client, p);
			return;
		}
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdVmSectionValidateConfig:
		{
			CDspService::instance()->getVmDirHelper().validateSectionVmConfig(client, p);
			return;
		}
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdVmGetSuspendedScreen:
		{
			CDspService::instance()->getVmDirHelper().getSuspendedVmScreen(client, p);
			return;
		}
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdVmGetSnapshotsTree:
		return (void)CDspService::instance()->getTaskManager()
			.schedule(new Task_SendSnapshotTree( client, p ));
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdVmUpdateSnapshotData:
		{
			CDspService::instance()->getVmSnapshotStoreHelper().updateSnapshotData(client, p);
			return;
		}
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdConfigureGenericPci:
		{
			CDspService::instance()->getShellServiceHelper().configureGenericPci(client, p);
			return;
		}
	case PVE::DspCmdPrepareForHibernate:
		{
			CDspService::instance()->getShellServiceHelper().beforeHostSuspend(client, p);
			return;
		}
	case PVE::DspCmdAfterHostResume:
		{
			CDspService::instance()->getShellServiceHelper().afterHostResume(client, p);
			return;
		}
		//////////////////////////////////////////////////////////////////////////
	case PVE::DspCmdSetNonInteractiveSession:
		{
			CDspService::instance()->getUserHelper().setNonInteractiveSession(client, p);
			return;
		}
	case PVE::DspCmdSetSessionConfirmationMode:
		{
			CDspService::instance()->getUserHelper().changeSessionConfirmationMode(client, p);
			return;
		}
	case PVE::DspCmdStorageSetValue:
		{
			CDspService::instance()->getShellServiceHelper().changeServerInternalValue(client, p);
			return;
		}
	case PVE::DspCmdUpdateUsbAssocList:
		{
			CDspService::instance()->getShellServiceHelper().updateUsbAssociationsList( client, p );
			return;
		}
	case PVE::DspCmdStartClusterService:
	case PVE::DspCmdStopClusterService:
		{
			client->sendSimpleResponse( p, PRL_ERR_UNIMPLEMENTED);
			return;
		}
	case PVE::DspCmdVmConvertDisks:
		{
			CDspService::instance()->getVmDirHelper().startConvertDisks( client, p );
			return;
		}
	case PVE::DspCmdVmMount:
		{
			CDspService::instance()->getVmDirHelper().mountVm( client, p );
			return;
		}
	case PVE::DspCmdVmUmount:
		{
			CDspService::instance()->getVmDirHelper().umountVm( client, p );
			return;
		}
	case PVE::DspCmdGetDiskFreeSpace:
		{
			CDspService::instance()->getShellServiceHelper().sendDiskFreeSpace( client, p );
			return;
		}
	case PVE::DspCmdDirVmMove: {
		CDspService::instance()->getVmDirHelper().moveVm(client, p);
		return;
		}
	case PVE::DspCmdGetCPUPoolsList:
		return CDspService::instance()->getShellServiceHelper().sendCPUPoolsList( client, p );
	case PVE::DspCmdMoveToCPUPool:
		return CDspService::instance()->getShellServiceHelper().moveToCPUPool( client, p );
	case PVE::DspCmdRecalculateCPUPool:
		return CDspService::instance()->getShellServiceHelper().recalculateCPUPool( client, p );
	} //switch( p->header.type )
	///////////////////////////////////////////////

	if (!CDspRouter::instance().routePackage(this, h, p))
	{
		// Send error
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_UNIMPLEMENTED );
		PERF_COUNT_ATOMIC_INC( dsp_error_commands ) ;
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
	CDspService::instance()->getIOServer().sendPackage(hReceiver, p);
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

void CDspClientManager::sendQuestionToClient(SmartPtr<CDspClient> pClient)
{
	//Skip questions for non interactive clients
	//https://jira.sw.ru/browse/PSBM-9110
	if ( pClient && pClient->isNonInteractive() )
		return;

	QList< SmartPtr<IOPackage> > lstQuestions = CDspService::instance()->getVmManager().getVmQuestions(pClient);
	if (lstQuestions.isEmpty())
	{
		// Ok, no more questions
		return;
	}

	for(int i = 0; i < lstQuestions.size(); i++)
	{
		SmartPtr<IOPackage> pQuestionPacket = lstQuestions[i];

		CVmEvent eventQuestion(UTF8_2QSTR(pQuestionPacket->buffers[0].getImpl()));

		// Check permissions
		CDspAccessManager::VmAccessRights accessRights = CDspService::instance()->getAccessManager()
			.getAccessRightsToVm(pClient, eventQuestion.getEventIssuerId());

		if ( !(accessRights.canRead() && accessRights.canExecute()) )
		{
			continue;
		}

		// Send question packet to the client
		WRITE_TRACE(DBG_FATAL, "Sending question %.8X '%s' to the user session '%s'", eventQuestion.getEventCode(),\
						PRL_RESULT_TO_STRING(eventQuestion.getEventCode()), QSTR2UTF8(pClient->getUserHostAddress()));
		pClient->sendPackage(pQuestionPacket);
	}
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
	unsigned int nLimit = CDspService::instance()->getDispConfigGuard()
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
