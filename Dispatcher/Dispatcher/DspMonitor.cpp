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
///	DspMonitor.cpp
///
/// @brief
///	Implementation of the class CDspMonitor
///
/// @brief
///	This class implements request processing from Server Management Console
///
/// @author sergeyt
///	SergeyT
///
/// @date
///	2006-12-01
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include "DspMonitor.h"

#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/VmDirectory/CVmDirectoryItem.h"
#include "Libraries/Logging/Logging.h"
#include "CDspTaskHelper.h"
#include "CDspUserHelper.h"
#include "XmlModel/DispConfig/CDispUser.h"
#include "CDspService.h"
#include "CDspClientManager.h"
#include "CDspVmManager.h"
#include "CDspVm.h"
#include "CProtoCommands.h"
#include "CProtoSerializer.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/Std/PrlTime.h"
#include "Libraries/HostUtils/HostUtils.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


CDspMonitor::CDspMonitor ()
{
}

CDspMonitor::~CDspMonitor ()
{
}

bool CDspMonitor::CheckUserPermission ( SmartPtr<CDspClient>& pUser)
{
	const QString& settingsUuid = pUser->getUserSettingsUuid();

	CDspLockedPointer<CDispUser> dispUser = CDspService::instance()->getDispConfigGuard().getDispUserByUuid(settingsUuid);

	PRL_ASSERT(dispUser.isValid());

	CDispUserAccess*  pAccess = dispUser->getUserAccess();

	bool flgAllowAsSMC = (pAccess && pAccess->canUseServerManagementConsole());

	return flgAllowAsSMC;
}

void CDspMonitor::GetDispRTInfo ( SmartPtr<CDspClient>& pUser,
								  const SmartPtr<IOPackage>& p )
{
   if (!CheckUserPermission(pUser))
   {
      WRITE_TRACE(DBG_FATAL, ">>> User is not authorized to call SMC commands.");

      // Send error to user: access token is not valid
	  pUser->sendSimpleResponse( p, PRL_ERR_ACCESS_DENIED );

      return;
   }

   CVmEvent sessionCont;

   //////////////////////////////////////////////////
   //  fill sessions container
   /////////////////////////////////////////////////
   // get snapshot to synchronize access
	QHash< IOSender::Handle, SmartPtr<CDspClient> > syncHash =
		CDspService::instance()->getClientManager().getSessionsListSnapshot();

   QHashIterator< IOSender::Handle, SmartPtr<CDspClient> > itSessions( syncHash );
   while( itSessions.hasNext() )
   {
      SmartPtr<CDspClient> pUser = itSessions.next().value();
	  CVmEvent e;
         //e.setEventType(PVE::EventTypeResponse);
      e.addEventParameter( new CVmEventParameter(PVE::Uuid
         , pUser->getClientHandle()
         , EVT_PARAM_SESSION_UUID));
      e.addEventParameter( new CVmEventParameter(PVE::String
         , pUser->getAuthHelper().getUserName()
         , EVT_PARAM_SESSION_USERNAME));
      e.addEventParameter( new CVmEventParameter(PVE::String
         , pUser->getUserHostAddress()
         , EVT_PARAM_SESSION_HOSTNAME));
      e.addEventParameter( new CVmEventParameter(PVE::Integer
         , QString("%1").arg(pUser->getSessionUptimeInSec())
         , EVT_PARAM_SESSION_ACTIVITY_TIME));

      QList<QString> vmList = CDspService::instance()->getVmDirHelper().getVmList(pUser);
      QList<QString>::const_iterator vcit;
      for (vcit = vmList.constBegin(); vcit!=vmList.constEnd(); vcit++)
      {
         const QString& vmUuid = *vcit;
         e.addEventParameter(new CVmEventParameter( PVE::Uuid
            , vmUuid
            , EVT_PARAM_SESSION_ALLOWED_VM));
      }
      sessionCont.addEventParameter(new CVmEventParameter(PVE::String, e.toString(),EVT_PARAM_RETURN_PARAM_TOKEN));
   }//for userList

   //LOG_MESSAGE( DBG_FATAL,"debugAAA: CDspMonitor::GetDispRTInfo: sessionCont:\n %s", sessionCont.toString().toUtf8().data());

   /**
    *  fill vm container
    */
   CVmEvent vmCont;
   QMultiHash< QString, SmartPtr<CVmConfiguration> > vmHash = CDspService::instance()->getVmDirHelper().getAllVmList();
   foreach ( QString vmDirUuid, vmHash.uniqueKeys() )
   {
		foreach( SmartPtr<CVmConfiguration> pVmConfig, vmHash.values( vmDirUuid ) )
		{
			PRL_ASSERT( pVmConfig );
			if( !pVmConfig )
				continue;

			QString vmUuid = pVmConfig->getVmIdentification()->getVmUuid();
			QString vmName = pVmConfig->getVmIdentification()->getVmName();

			CVmEvent e;
			e.addEventParameter( new CVmEventParameter(PVE::Uuid
				, vmUuid
				, EVT_PARAM_VM_UUID));

			e.addEventParameter( new CVmEventParameter(PVE::String
				, vmName
				, EVT_PARAM_VM_NAME));

			VIRTUAL_MACHINE_STATE vmState = CDspVm::getVmState( vmUuid, vmDirUuid );
			e.addEventParameter( new CVmEventParameter(PVE::Integer
				, QString("%1").arg(vmState)
				, EVT_PARAM_VM_RUNTIME_STATUS ));

			vmCont.addEventParameter( new CVmEventParameter(PVE::String,
				e.toString(),
				EVT_PARAM_RETURN_PARAM_TOKEN));
		}
   }
   //LOG_MESSAGE( DBG_FATAL,"debugAAA: CDspMonitor::GetDispRTInfo: vmCont:\n %s", vmCont.toString().toUtf8().data());


   ///////////////////////////////////////
   CVmEvent final_event;
   //final_event.setEventType(PVE::CResultType);

   final_event.addEventParameter(new CVmEventParameter( PVE::String,
      sessionCont.toString(),
      EVT_PARAM_RETURN_PARAM_TOKEN));
   final_event.addEventParameter(new CVmEventParameter( PVE::String,
      vmCont.toString(),
      EVT_PARAM_RETURN_PARAM_TOKEN));

   CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);
   CProtoCommandDspWsResponse *pDspWsResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
   pDspWsResponseCmd->SetVmEvent(final_event.toString());

   pUser->sendResponse( pResponse, p );
}

void CDspMonitor::ShutdownDispatcher (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}

	if (!CheckUserPermission(pUser) || !pUser->getAuthHelper().isLocalAdministrator())
	{
		WRITE_TRACE(DBG_FATAL, ">>> User [%s] is not authorized to call SMC commands.", QSTR2UTF8(pUser->getAuthHelper().getUserName()));

		// Send error to user: access token is not valid
		pUser->sendSimpleResponse( p, PRL_ERR_ACCESS_DENIED );

		return;
	}

	CProtoCommandWithOneStrParam* pShutdownCmd
		= CProtoSerializer::CastToProtoCommand<CProtoCommandWithOneStrParam>(cmd);

	bool bForceShutdown = pShutdownCmd->GetFirstStrParam().toInt() != 0;
	bForceShutdown |= (pShutdownCmd->GetCommandFlags() & PSHF_FORCE_SHUTDOWN) !=0 ;

	if (!bForceShutdown)
	{
		//More then one session presents decline
		if (CDspService::instance()->getClientManager().getSessionsListSnapshot().size() > 1)
		{
			WRITE_TRACE(DBG_FATAL, "There are more than one user session presents - decline shutdown procedure");

			pUser->sendSimpleResponse(p, PRL_ERR_ANOTHER_USER_SESSIONS_PRESENT );
			return;
		}

		if (CDspService::instance()->getVmManager().hasAnyRunningVms())//Some VMs currently running
		{
			WRITE_TRACE(DBG_FATAL, "There are some VMs currently running - decline shutdown procedure");

			pUser->sendSimpleResponse(p, PRL_ERR_SOME_VMS_RUNNING );
			return;
		}
	}

	QString sServerUuid = CDspService::instance()->getDispConfigGuard()
		 .getDispConfig()->getVmServerIdentification()->getServerUuid();

	quint32 nFlags = pShutdownCmd->GetCommandFlags();
	if( CDspService::isServerModePSBM() && (nFlags & PSHF_SUSPEND_VM_TO_PRAM) )
	{
		/* set flag FastReboot in dispatcher config.
		 * To be processed in CDspVmManager::shutdownVms()
		 */
		CDspLockedPointer<CDispCommonPreferences> pCommonPrefs =
			CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
		pCommonPrefs->getFastRebootPreferences()->setFastReboot(true);
		CDspService::instance()->getDispConfigGuard().saveConfig();
	}

	QList< SmartPtr<CDspClient> > lstClients
		= CDspService::instance()->getClientManager().getSessionsListSnapshot().values();

	CVmEvent evt(PET_DSP_EVT_DISP_SHUTDOWN, sServerUuid, PIE_DISPATCHER);
	SmartPtr<IOPackage> pkg = DispatcherPackage::createInstance(PVE::DspVmEvent, evt, p);

	CDspService::instance()->getClientManager().sendPackageToClientList( pkg, lstClients );


	if( nFlags & PSHF_DONT_WAIT_TO_COMPLETE)
	{
		pUser->sendSimpleResponse(p, PRL_ERR_SUCCESS );
		WRITE_TRACE(DBG_FATAL, "Response of shutdown command was sent immediately by user request." );
	}
	else
		StoreShutdownRequest( pUser, p );

	WRITE_TRACE(DBG_FATAL, "Shutdown service by user [%s] %srequest!!!",
			QSTR2UTF8(pUser->getAuthHelper().getUserName()),
			(nFlags & PSHF_SUSPEND_VM_TO_PRAM) ? "fast " : "");

	PRL_ASSERT(CMainDspService::instance());
	CMainDspService::instance()->stop();
}

void CDspMonitor::ForceCancelCommandOfUser (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& pkg,
	const QString& taskUuid )
{
   if (!CheckUserPermission(pUser))
   {
      WRITE_TRACE(DBG_FATAL, ">>> User is not authorized to call SMC commands.");

      // Send error to user: access token is not valid
	  pUser->sendSimpleResponse( pkg, PRL_ERR_ACCESS_DENIED );

      return;
   }

	SmartPtr<CDspTaskHelper> pTask = CDspService::instance()->getTaskManager().findTaskByUuid( taskUuid );
	if ( ! pTask )
	{
		pUser->sendSimpleResponse(pkg, PRL_ERR_TASK_NOT_FOUND );
		return;
	}

	pTask->cancelOperation(pUser, pkg);

	//////////////////////////////
	// send notification to user
	if( pTask->getClient() && pTask->getRequestPackage() )
		 pTask->getClient()->sendSimpleResponse( pTask->getRequestPackage(), PET_DSP_EVT_SMC_CANCEL_USER_COMMAND );

	pUser->sendSimpleResponse(pkg, PRL_ERR_SUCCESS );
}


void CDspMonitor::ForceDisconnectUser (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
   if (!CheckUserPermission(pUser))
   {
      WRITE_TRACE(DBG_FATAL, ">>> User is not authorized to call SMC commands.");

      // Send error to user: access token is not valid
	  pUser->sendSimpleResponse( p, PRL_ERR_ACCESS_DENIED );

      return;
   }


	pUser->sendSimpleResponse( p, PET_DSP_EVT_SMC_USER_FORCE_DISCONNECTED );

	// disconnect
	CDspService::instance()->getUserHelper().processUserLogoff(pUser, p);
}


void CDspMonitor::ForceDisconnectAllUsers (
    SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
   if (!CheckUserPermission(pUser))
   {
      WRITE_TRACE(DBG_FATAL, ">>> User is not authorized to call SMC commands.");

      // Send error to user: access token is not valid
	  pUser->sendSimpleResponse( p, PRL_ERR_ACCESS_DENIED );

      return;
   }

	// get snapshot to synchronize access
	QHash< IOSender::Handle, SmartPtr<CDspClient> > syncHash =
		CDspService::instance()->getClientManager().getSessionsListSnapshot();

   QHashIterator< IOSender::Handle, SmartPtr<CDspClient> > itSessions( syncHash );

   while( itSessions.hasNext() )
   {
		SmartPtr<CDspClient> user = itSessions.next().value();
        if (pUser == user)
          continue;
      ForceDisconnectUser(pUser, p);
   }
}

void CDspMonitor::StoreShutdownRequest( SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p )
{
	QMutexLocker lock(&m_listMutex);
	m_shutdownRequests << qMakePair( pUser->getClientHandle(), p );
}

void CDspMonitor::SendShutdownCompleteResponses( PRL_UINT32 nSendTimeoutInMSecs )
{
	QList< IOSendJob::Handle > sentJobs;
	QList< ShutdownRequestInfo > requestsList;

	// copy to prevent wait on locked mutex
	QMutexLocker lock(&m_listMutex);
	requestsList = m_shutdownRequests;
	lock.unlock();

	if( !requestsList.size() )
		return;

	WRITE_TRACE( DBG_FATAL, "Going to send %d responses to shutdown request. Timeout %d secs"
		, requestsList.size(), nSendTimeoutInMSecs );

	QPair< IOSender::Handle, SmartPtr<IOPackage> > sendInfo;
	foreach( sendInfo, requestsList )
	{
		sentJobs << CDspService::instance()->sendSimpleResponseToClient(
			sendInfo.first, sendInfo.second, PRL_ERR_SUCCESS);
	}
	PRL_UINT64 breakTimestamp = PrlGetTickCount64() + PrlGetTicksPerSecond()*nSendTimeoutInMSecs/1000;
	while( nSendTimeoutInMSecs== UINT_MAX || PrlGetTickCount64() < breakTimestamp )
	{
		QList< IOSendJob::Handle >::iterator it;
		for( it=sentJobs.begin(); it!=sentJobs.end(); )
		{
			static const quint32 checkTimeoutInMSec = 20;
			IOSendJob::Result
				nResult = CDspService::instance()->getIOServer().waitForSend( *it, checkTimeoutInMSec );
			if( nResult == IOSendJob::Success )
				it = sentJobs.erase(it);
			else
				it++;
		}//for

		if( !sentJobs.size() )
			break;
		HostUtils::Sleep( 50 /* msecs */ );
	}//while

	WRITE_TRACE( DBG_FATAL, "Done. %d responses ( of %d ) was successfully sent."
		, requestsList.size() - sentJobs.size()
		, requestsList.size() );
}

