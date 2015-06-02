///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_PrepareForHibernate.cpp
///
/// Dispatcher task for configuration generic PCI devices.
///
/// @author myakhin@
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

#include "Task_PrepareForHibernate.h"

#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "CDspService.h"
#include "Libraries/Std/PrlTime.h"
#include "Libraries/Std/PrlAssert.h"


using namespace Parallels;


#define SBG_TIMEOUT_PER_VM_MSEC		10		// miliseconds

#define SBG_TIMEOUT_FOR_SUSPEND_VM_IN_SEC	( 60 )
#define SBG_TIMEOUT_FOR_TOTAL_IN_SEC		( 100 )

bool Task_PrepareForHibernate::m_bIsRunning = false;

Task_PrepareForHibernate::Task_PrepareForHibernate(SmartPtr<CDspClient> &pUser,
											 const SmartPtr<IOPackage> &pRequestPkg)
: CDspTaskHelper(pUser, pRequestPkg, false, &m_bIsRunning)
{
}

QList<CVmIdent> Task_PrepareForHibernate::getListVmWithVtd()
{
	QList<CVmIdent> lst;

	QMultiHash<QString, SmartPtr<CVmConfiguration> > hashVms
		= CDspService::instance()->getVmDirHelper().getAllVmList();

	QMultiHash<QString, SmartPtr<CVmConfiguration> >::iterator itDir;
	for(itDir = hashVms.begin(); itDir != hashVms.end(); ++itDir)
	{
		SmartPtr<CVmConfiguration> pVmConfig = itDir.value();

		bool bTestFlagEnabled = false;
		if( pVmConfig->getVmSettings()->getVmRuntimeOptions()
			->getSystemFlags().contains("vm.suspend_as_with_vtd=1")
			)
		{
			WRITE_TRACE( DBG_FATAL, "Vm = %s contains test flag and was processed as with Vm with VT-d"
				, QSTR2UTF8(pVmConfig->getVmIdentification()->getVmUuid()) );
			bTestFlagEnabled = true;
		}

		if (bTestFlagEnabled || pVmConfig->hasEnabledGenericPciDevices())
		{
			lst.append( MakeVmIdent( pVmConfig->getVmIdentification()->getVmUuid(), itDir.key()) );
		}
	}

	return lst;
}


PRL_RESULT Task_PrepareForHibernate::prepareTask()
{
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_PrepareForHibernate::run_body()
{
	return prepareForHostSuspend( true );
}

PRL_RESULT Task_PrepareForHibernate::prepareForHostSuspend( bool bAllowCancelHostSuspend )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		const PVE::IDispatcherCommands defaultCommand =
#ifdef _WIN_
			PVE::DspCmdCtlVmStandByGuest;
#else
			PVE::DspCmdVmSuspend;
#endif
		WRITE_TRACE( DBG_FATAL, "Start to prepare for host suspend. Allow to cancel host suspend = '%s'"
			, bAllowCancelHostSuspend?"true":"false" );

		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		if ( ! lockToExecute() )
			throw PRL_ERR_PREPARE_FOR_HIBERNATE_TASK_ALREADY_RUN;

		if ( ! getClient()->getAuthHelper().isLocalAdministrator() )
			throw PRL_ERR_ACCESS_DENIED;

		QSet<CVmIdent> setFailedVm;
		QList<CVmIdent> listVtdVm = getListVmWithVtd();
		typedef QList<CVmIdent>::iterator  LIST_IT;

		//////////////////////////////////////////////////////////////////////////
		// Check choosen VM's on running/paused state and having tools
		//////////////////////////////////////////////////////////////////////////
		for(LIST_IT it = listVtdVm.begin(); it != listVtdVm.end(); ++it)
		{
			SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( *it );
			if ( ! pVm.isValid() )
				continue;

			try
			{
				if ( pVm->getVmState() != VMS_RUNNING && pVm->getVmState() != VMS_PAUSED )
					throw PRL_ERR_PREPARE_FOR_HIBERNATE_VM_WRONG_STATE;

				CVmEvent evt;
				if ( PRL_FAILED(evt.fromString(pVm->getVmToolsStateString())) )
					throw PRL_ERR_UNEXPECTED;

				CVmEventParameter* pParam = evt.getEventParameter( EVT_PARAM_VM_TOOLS_STATE );
				if ( ! pParam )
					throw PRL_ERR_UNEXPECTED;

				PRL_VM_TOOLS_STATE nToolsState = (PRL_VM_TOOLS_STATE )pParam->getParamValue().toInt();
				if ( nToolsState != PTS_INSTALLED && nToolsState != PTS_OUTDATED )
					throw PRL_ERR_PREPARE_FOR_HIBERNATE_VM_WITHOUT_TOOLS;
			}
			catch(PRL_RESULT code)
			{
				WRITE_TRACE(DBG_DEBUG, "Cannot do suspend for VM with uuid = %s", QSTR2UTF8(it->first));
				if( bAllowCancelHostSuspend )
					throw code;

				setFailedVm.insert( *it );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Send suspend guest command to VM's
		//////////////////////////////////////////////////////////////////////////

		WRITE_TRACE(DBG_FATAL, "Send suspend commands to guests");

		QMap< CVmIdent, JobInfo > mapJobs;

		for(LIST_IT it = listVtdVm.begin(); it != listVtdVm.end(); ++it)
		{
			SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( *it );
			if ( ! pVm.isValid() )
				continue;

			PVE::IDispatcherCommands cmdNo = defaultCommand;
			if( !bAllowCancelHostSuspend && setFailedVm.contains( *it ) )
			{
				cmdNo = PVE::DspCmdVmStop;

				WRITE_TRACE( DBG_FATAL, "Unable to send suspend "
					"so send 'DspCmdVmStop' to VM with uuid = %s"
					, QSTR2UTF8(it->first) );

				sendWarningBeforeVmStop( *it );
			}

			CProtoCommandPtr
				pCmd = CProtoSerializer::CreateProtoVmCommandWithAcpiSign(
					cmdNo,
					it->first,
					false);

			SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance( cmdNo, pCmd );

			IOSendJob::Handle hJob = pVm->sendPackageToVm( pPackage );
			mapJobs.insert( *it, JobInfo(hJob, cmdNo) );
		}

// Wait for responses

		WRITE_TRACE(DBG_FATAL, "Wait for responses after sending suspend / stop commands to %d guests...",
								mapJobs.size());

		PRL_UINT64 nTimeOutSuspendFinished =
			PrlGetTickCount64() + (PRL_UINT64 )PrlGetTicksPerSecond() * SBG_TIMEOUT_FOR_SUSPEND_VM_IN_SEC;
		PRL_UINT64 nTimeOutFinished =
			PrlGetTickCount64() + (PRL_UINT64 )PrlGetTicksPerSecond() * SBG_TIMEOUT_FOR_TOTAL_IN_SEC;

		PRL_ASSERT( nTimeOutSuspendFinished < nTimeOutFinished );

		do
		{
			if ( operationIsCancelled() )
				throw PRL_ERR_OPERATION_WAS_CANCELED;

			if( PrlGetTickCount64() > nTimeOutFinished )
			{
				WRITE_TRACE(DBG_FATAL, "Timeout of wait guest prepared reached!");
				throw PRL_ERR_TIMEOUT;
			}

			bool bSuspendTimeoutElapsed = false;
			if( PrlGetTickCount64() > nTimeOutSuspendFinished )
			{
				if( !bSuspendTimeoutElapsed ) // to trace once
					WRITE_TRACE(DBG_FATAL, "Timeout of wait suspend of Vm reached! VmStop command will sended!");
				bSuspendTimeoutElapsed = true;
			}

			QMap<CVmIdent, JobInfo>::iterator job_it;
			for(job_it = mapJobs.begin(); job_it != mapJobs.end(); ++job_it)
			{
				PRL_RESULT nResponseRetCode = PRL_ERR_SUCCESS;
				bool bNeedSendVmStop = false;

				if( bSuspendTimeoutElapsed
					&& !job_it.value().bSuspendTimeoutElapsed
					&& job_it.value().cmd == defaultCommand )
				{
					job_it.value().bSuspendTimeoutElapsed = true;
					bNeedSendVmStop = true;
				}
				else
				{
					IOSendJob::Handle hJob = job_it.value().hJob;
					IOSendJob::Result nResult = CDspService::instance()->getIOServer()
						.waitForResponse( hJob, SBG_TIMEOUT_PER_VM_MSEC );

					if ( nResult != IOSendJob::Success )
						continue;

					IOSendJob::Response resp = CDspService::instance()->getIOServer().takeResponse( hJob );
					if( resp.responseResult != IOSendJob::Success )
					{
						WRITE_TRACE(DBG_FATAL, "Connection to vm may be lost: "
							"Unable to take Response for VM with uuid = %s",
							QSTR2UTF8(job_it.key().first )
							);

						job_it = mapJobs.erase(job_it);
						continue;
					}

					//////////////////////////////////////////////////////////////////////////
					// ( resp.responseResult == IOSendJob::Success )
					//////////////////////////////////////////////////////////////////////////
					SmartPtr<IOPackage> respPkg = resp.responsePackages[0];
					CProtoCommandPtr pResponse
						= CProtoSerializer::ParseCommand(PVE::DspWsResponse,
						UTF8_2QSTR(respPkg->buffers[0].getImpl()));
					CProtoCommandDspWsResponse *pResponseCmd
						= CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

					nResponseRetCode = pResponseCmd->GetRetCode();

					WRITE_TRACE(DBG_FATAL, "Response [%#x][%s] has been returned for VM with uuid = %s",
						nResponseRetCode,
						PRL_RESULT_TO_STRING( nResponseRetCode ),
						QSTR2UTF8(job_it.key().first ) );
				}

				if( bAllowCancelHostSuspend && !bNeedSendVmStop )
				{
					job_it = mapJobs.erase(job_it);

					if( PRL_FAILED(nResponseRetCode) )
						throw PRL_ERR_PREPARE_FOR_HIBERNATE_VM_CANNOT_STAND_BY;
				}
				else // !bAllowCancelHostSuspend
				{
					SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( job_it.key() );
					const PVE::IDispatcherCommands command = job_it.value().cmd;
					if( command == defaultCommand
						&&  ( PRL_FAILED(nResponseRetCode) || bNeedSendVmStop )
						&& pVm
						)
					{
						WRITE_TRACE(DBG_FATAL, "Because suspend fails "
							"we should send Stop command to Vm."
							);

						sendWarningBeforeVmStop( job_it.key() );

						PVE::IDispatcherCommands cmdStop = PVE::DspCmdVmStop;
						CProtoCommandPtr
							pCmd = CProtoSerializer::CreateProtoVmCommandWithAcpiSign(
							cmdStop,
							job_it.key().first,
							false);

						SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance( cmdStop, pCmd );
						job_it.value().hJob = pVm->sendPackageToVm( pPackage );
						job_it.value().cmd = cmdStop;
					}
					else
						job_it = mapJobs.erase(job_it);
				}
			}// for
		}while( !mapJobs.isEmpty() );


		WRITE_TRACE( DBG_FATAL, "All VMs prepared. Going to prepare Hypervisor." );

		ret = prepareHypervisor();
		if(PRL_FAILED(ret))
			throw ret;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while prepare vm for host suspend with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode( ret );

	unlockToExecute();
	return ret;
}

PRL_RESULT Task_PrepareForHibernate::prepareHypervisor()
{
	//WRITE_TRACE(DBG_FATAL, "Failed to send VT-d suspend ioctl");
	//return PRL_ERR_UNABLE_SEND_REQUEST;
	return PRL_ERR_SUCCESS;
}

void Task_PrepareForHibernate::finalizeTask()
{
// Send response
	if ( PRL_FAILED( getLastErrorCode() ) )
	{
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
	{
		CProtoCommandPtr pCmd
			= CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
		getClient()->sendResponse( pCmd, getRequestPackage() );
		getLastError()->setEventCode( PRL_ERR_SUCCESS );
	}
}

bool Task_PrepareForHibernate::isTaskRunning()
{
	return m_bIsRunning;
}

void Task_PrepareForHibernate::sendWarningBeforeVmStop(const CVmIdent& vmIdent)
{
	CVmEvent event(
		PET_DSP_EVT_VM_MESSAGE,
		vmIdent.first,
		PIE_DISPATCHER,
		PRL_WARN_PREPARE_FOR_HIBERNATE_UNABLE_SUSPEND_DO_STOP);

	QString vmName;
	{
		CDspLockedPointer<CVmDirectoryItem>
			pVmDirItem = CDspService::instance()->getVmDirManager()
				.getVmDirItemByUuid( vmIdent.second, vmIdent.first );
		if(pVmDirItem)
			vmName = pVmDirItem->getVmName();
	}

	event.addEventParameter(
		new CVmEventParameter(PVE::String,
		vmName,
		EVT_PARAM_MESSAGE_PARAM_0));

	SmartPtr<IOPackage> pkgNew = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
	CDspService::instance()->getClientManager().sendPackageToVmClients( pkgNew, vmIdent.second, vmIdent.first );
}
