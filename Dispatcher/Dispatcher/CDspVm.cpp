//////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVm.cpp
///
/// Class that wrapping Vm possible actions (start, stop and etc.)
///
/// @author sandro
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

//#define FORCE_LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "CDspVm.h"
#include "CDspVm_p.h"
#include "CDspService.h"
#include "CDspHandlerRegistrator.h"
#include "CDspClientManager.h"
#include "CDspVmManager.h"
#include "CDspCommon.h"
#include "CDspRouter.h"
#include "CDspBugPatcherLogic.h"
#include "CDspVmInfoDatabase.h"
#include "CDspVmDirHelper.h"

#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/PrlCommonUtilsBase/ParallelsDirs.h"
#include "Build/Current.ver"

#include "CDspVmStateSender.h"

#include "Tasks/Task_CreateSnapshot.h"
#include "Tasks/Task_SwitchToSnapshot.h"
#include "Tasks/Task_DeleteSnapshot.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_CommitUnfinishedDiskOp.h"
#include "Tasks/Task_EditVm.h"
#include "Tasks/Task_CloneVm.h"
#include "Tasks/Task_MoveVm.h"

#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"

#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "CVmValidateConfig.h"

#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtilsBase/Common.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"

//#include "Vm/CVmMigrateFilesCopyingStubs.h"

#include "Libraries/PrlCommonUtilsBase/CommandLine.h"
#include "Libraries/PrlCommonUtilsBase/OsInfo.h"
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include "Interfaces/ParallelsDomModel.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"

#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/Std/PrlTime.h"

#include <Libraries/PowerWatcher/PowerWatcher.h>

#include "EditHelpers/CMultiEditDispatcher.h"

#ifdef _WIN_
	#include <process.h>
	#define getpid _getpid
#endif

#if defined(_LIN_)
#include <sys/wait.h>
#include "Libraries/Virtuozzo/CCpuHelper.h"
#endif

#include "CDspVzHelper.h"

using namespace DspVm;
using namespace Parallels;

#include "Libraries/Logging/Logging.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define CHECK_WHETHER_VM_STARTED \
{\
	if (!isConnected())\
	{\
	pUser->sendSimpleResponse(p, PRL_ERR_VM_PROCESS_IS_NOT_STARTED);\
	return;\
	}\
}

#define CHECK_USER_ACCESS_RIGHTS( pUser, cmd, pkg ) \
{ \
	PRL_RESULT ret = checkUserAccessRightsAndSendResponseOnError( pUser, pkg, cmd ); \
	if( PRL_FAILED( ret ) ) \
		return;	\
}

#define FILL_ERROR_EVENT_WITH_ONE_STR_PARAM( _evt, errCode, strParam1 ) \
{ \
				WRITE_TRACE(DBG_FATAL, "Error: can't execute command, by error %s", \
						PRL_RESULT_TO_STRING(errCode) ); \
	\
				(_evt).setEventCode( errCode ); \
				(_evt).addEventParameter( new CVmEventParameter ( PVE::String, \
							strParam1, EVT_PARAM_MESSAGE_PARAM_0 ) ); \
}

#define SEND_ERROR_WITH_ONE_STR_PARAM(pPkg, errCode, strParam ) \
{ \
			CVmEvent _evt;	\
			FILL_ERROR_EVENT_WITH_ONE_STR_PARAM( _evt, errCode, strParam ); \
			pUser->sendResponseError( _evt, pPkg );	\
}

#define SEND_ERROR_BY_VM_WAKINGUP_STATE( pPkg ) \
	{ \
			CVmEvent evt;	\
			FILL_ERROR_EVENT_WITH_ONE_STR_PARAM( evt, PRL_ERR_VM_IN_WAKING_UP_STATE, getVmName() ); \
			pUser->sendResponseError( evt, pPkg );	\
	}

#define SEND_ERROR_BY_VM_FROZEN_STATE( pPkg ) \
	{ \
			CVmEvent evt;	\
			FILL_ERROR_EVENT_WITH_ONE_STR_PARAM( evt, PRL_ERR_VM_IN_FROZEN_STATE, getVmName() ); \
			pUser->sendResponseError( evt, pPkg );	\
	}

#define SEND_ERROR_BY_VM_IS_NOT_STOPPED( pPkg ) \
	{ \
			CVmEvent evt;	\
			FILL_ERROR_EVENT_WITH_ONE_STR_PARAM( evt, PRL_ERR_DISP_VM_IS_NOT_STOPPED, getVmName() ); \
			pUser->sendResponseError( evt, pPkg );	\
	}

#define SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( pPkg, state ) \
	{ \
			WRITE_TRACE(DBG_FATAL, "Error: can't execute %s command, m state is forbidden! (state = %#x)" \
			,	PVE::DispatcherCommandToString( pPkg->header.type )	\
				, state );	\
	\
			CVmEvent evt;	\
			evt.setEventCode( PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED ); \
			evt.addEventParameter( new CVmEventParameter (	\
				PVE::String,	\
				getVmName(),	\
				EVT_PARAM_MESSAGE_PARAM_0 ) );	\
			evt.addEventParameter( new CVmEventParameter (	\
				PVE::String,	\
				PRL_VM_STATE_TO_STRING( state ),	\
				EVT_PARAM_MESSAGE_PARAM_1 ) );	\
	\
			pUser->sendResponseError( evt, pPkg );	\
	}

bool CDspVm::isContinueSnapshotCmdFromPausedStateAllowed( VmPowerState nVmPowerState,
	SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
	CVmEvent* evt, bool bWaitResult)
{
		if( vpsPausedByHostSleep == nVmPowerState || vpsPausedByVmFrozen == nVmPowerState)
		{
			PRL_RESULT err = (vpsPausedByHostSleep == nVmPowerState)
				? PRL_ERR_VM_IN_WAKING_UP_STATE
				: PRL_ERR_VM_IN_FROZEN_STATE;

			if ( !bWaitResult )
				SEND_ERROR_WITH_ONE_STR_PARAM( p, err, getVmName() )
			else if (evt)
				FILL_ERROR_EVENT_WITH_ONE_STR_PARAM( *evt, err, getVmName() )
			return false;
		}
		return true;
}

//--------------------------------------Base VM helpers class implementation-------------------------------------------
QMutex *CDspVmBaseHelper::g_pVmHelpersListMutex = new QMutex(QMutex::Recursive);
QList<SmartPtr<CDspVmBaseHelper> > *CDspVmBaseHelper::g_pVmHelpersList = new QList<SmartPtr<CDspVmBaseHelper> >;

const char* CDspVm::VmPowerStateToString( VmPowerState st )
{
#define PST_TO_STR( stName ) case stName: return #stName;
	switch(st)
	{
	PST_TO_STR(vpsNormal);
	PST_TO_STR(vpsPausedByHostSleep);
	PST_TO_STR(vpsPausedByVmFrozen);
	default: return "vpsUnknown";
	}
#undef PST_TO_STR
}

CDspVmBaseHelper::CDspVmBaseHelper ( const QString &sVmUuid, const QString &sVmDirUuid )
: m_sVmUuid(sVmUuid), m_sVmDirUuid(sVmDirUuid)
{
	LOG_MESSAGE(DBG_DEBUG, "CDspVmBaseHelper::CDspVmBaseHelper() this=%p", this);
	cleanupObjects();
	QMutexLocker _lock(g_pVmHelpersListMutex);
	g_pVmHelpersList->append(SmartPtr<CDspVmBaseHelper>(this));
}

void CDspVmBaseHelper::cleanupObjects()
{
	QMutexLocker _lock(g_pVmHelpersListMutex);
	QList<SmartPtr<CDspVmBaseHelper> >::iterator _vm_helper = g_pVmHelpersList->begin();
	while (_vm_helper != g_pVmHelpersList->end())
	{
		if (!(*_vm_helper)->isRunning())//Thread finished it work and we can safely destroy it now
			_vm_helper = g_pVmHelpersList->erase(_vm_helper);
		else
			++_vm_helper;
	}
}

Storage* CDspVm::g_pStorage = new Storage;
QReadWriteLock *CDspVm::g_pVmsMapLock = new QReadWriteLock;

void CDspVm::UnregisterVmObject(const SmartPtr<CDspVm> &pVm)
{
	QWriteLocker _lock(g_pVmsMapLock);
	bool bFound = g_pStorage->unregister(pVm->ident());

	// #425324 Trace to understand case when dispatcher lost VM
	WRITE_TRACE( bFound? DBG_WARNING: DBG_FATAL
		, "UnregisterVm %s: uuid = %s, name = '%s'"
		, bFound? "succeeded" : "FAILED"
		, (!pVm)?"unknown": QSTR2UTF8(pVm->getVmUuid())
		, (!pVm)?"unknown": QSTR2UTF8(pVm->getVmName())
		);
}

void	CDspVm::UnregisterAllVmObjects()
{
	QWriteLocker g(g_pVmsMapLock);
	DspVm::Storage::snapshot_type s = g_pStorage->unregisterAll();
	g.unlock();
	s.clear();
}


SmartPtr<CDspVm> CDspVm::CreateInstance(
	const QString &sVmUuid
	, const QString &sVmDirUuid
	, PRL_RESULT& outCreateError
	, bool& bNew
	, const SmartPtr<CDspClient> &pUser
	, PVE::IDispatcherCommands cmd
	, const QString &sTaskId)
{
	return CreateInstance(MakeVmIdent(sVmUuid, sVmDirUuid), outCreateError, bNew, pUser, cmd, sTaskId) ;
}

SmartPtr<CDspVm> CDspVm::CreateInstance(
	const CVmIdent &vmIdent
	, PRL_RESULT& outCreateError
	, bool& bNew
	, const SmartPtr<CDspClient> &pUser
	, PVE::IDispatcherCommands cmd
	, const QString &sTaskId)
{
	bNew = false;
	outCreateError = PRL_ERR_SUCCESS;
	SmartPtr<CDspVm> pVm = GetVmInstanceByUuid(vmIdent);
	if (pVm)
		return (pVm);

	VIRTUAL_MACHINE_STATE nVmState = getVmState(vmIdent.first, vmIdent.second);

	QWriteLocker _lock(g_pVmsMapLock);
	//To fix synchronization hole we make search again under write mutex
	pVm = g_pStorage->finw(vmIdent);
	//Unlock in order to prevent deadlock from
	//https://bugzilla.sw.ru/show_bug.cgi?id=465431
	_lock.unlock();
	if (pVm)
		return pVm;// if on another thread create CDspVm
	try
	{
		SmartPtr<CDspVm> z(new CDspVm);
		outCreateError = z->initialize(vmIdent, pUser, cmd, nVmState, sTaskId);
		if (PRL_SUCCEEDED(outCreateError))
		{
			_lock.relock();
			SmartPtr<CDspVm> x = g_pStorage->enroll(z->m_pDetails);
			_lock.unlock();
			bNew = z->m_pDetails == x->m_pDetails;
			return x;
		}
	}
	catch( PRL_RESULT code )
	{
		outCreateError = code;
	}
	WRITE_TRACE(DBG_FATAL, "Can't create instance of CDSpVm by error %#x, %s",
		outCreateError, PRL_RESULT_TO_STRING( outCreateError ) );
	return pVm;
}

//static
SmartPtr<CDspVm> CDspVm::GetVmInstanceByUuid(const CVmIdent &vmIdent)
{
	QReadLocker _lock(g_pVmsMapLock);
	return g_pStorage->finw(vmIdent);
}

SmartPtr<CDspVm> CDspVm::GetVmInstanceByDispConnectionHandle(const IOSender::Handle &h)
{
	QReadLocker _lock(g_pVmsMapLock);
	return g_pStorage->finw(h);
}

SmartPtr<CDspVm> CDspVm::GetVmInstanceByUuid(const QString &sVmUuid, const QString &sVmDirUuid)
{
	return GetVmInstanceByUuid(MakeVmIdent(sVmUuid, sVmDirUuid)) ;
}

CDspLockedPointer<ProcPerfStoragesContainer> CDspVm::PerfStoragesContainer()
{
	quint32 pid = getVmProcessIdAsUint(); // to prevent potential deadlock

	QMutexLocker lock( &d().m_mtxPerfStoragesContainer );
	if(  d().m_perfstorage_container.IsEmpty() )
	{
		// We have not necessary to check to race when PerfStoragesContainer() was called before start vm process
		// (snapshot operations for example)
		// if pid == 0 m_perfstorage_container still empty and will initalized on next valid call.
		if ( pid > 0 )
			d().m_perfstorage_container.Refresh( pid );
	}
	return CDspLockedPointer<ProcPerfStoragesContainer>
		( &d().m_mtxPerfStoragesContainer, &d().m_perfstorage_container );
}

PRL_RESULT CDspVm::initialize(const CVmIdent& id_, const SmartPtr<CDspClient>& client_,
			PVE::IDispatcherCommands command_, VIRTUAL_MACHINE_STATE state_,
			const QString &sTaskId)
{
	switch (command_)
	{
	case PVE::DspCmdVmStart:
	case PVE::DspCmdVmStartEx:
	case PVE::DspCmdVmCreateSnapshot:
	case PVE::DspCmdVmSwitchToSnapshot:
	case PVE::DspCmdVmDeleteSnapshot:
	case PVE::DspCmdDirVmMigrate:
	case PVE::DspCmdDirVmMigrateClone:
		break;
	default:
		return PRL_ERR_INVALID_ARG;
	}
	SmartPtr<CVmConfiguration> pVmConfig;
	d(new DspVm::Details(id_, client_, command_, state_));
	PRL_RESULT res = vdh().registerExclusiveVmOperation(getVmUuid(),
			getVmDirUuid(),	d().m_nInitDispatcherCommand,
			d().m_hLockOwner, sTaskId );

	if ( PRL_FAILED( res ) )
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=439127
		return res;
	}
	d().m_bVmCmdWasExclusiveRegistered = true;

	{
		CDspLockedPointer<CVmDirectoryItem> pDirItem
			= vdm().getVmDirItemByUuid( getVmDirUuid(), getVmUuid() );
		if ( pDirItem )
		{
			d().m_sVmName = pDirItem->getVmName();
			d().m_strVmHome = pDirItem->getVmHome();
			if( ! turnOffSpotlight( CFileHelper::GetFileRoot( d().m_strVmHome ) ) )
				WRITE_TRACE(DBG_FATAL, "turnOffSpotlight() failed for vm = %s", QSTR2UTF8( getVmName() ) );
		}
	}

	d().m_bSafeMode = isSafeMode();

	return PRL_ERR_SUCCESS;
}

struct wait_helper_param
{
	Q_PID nPid;
	QString sUuid;

	wait_helper_param(Q_PID pid, const QString &uuid) :
		nPid(pid), sUuid(uuid)
	{}
};

#ifndef _WIN_
static void *wait_helper(void *param)
{
	struct wait_helper_param *_param = (struct wait_helper_param *)param;
	long pid = (long)_param->nPid;
	int res;

	do {
		res = ::waitpid(pid, NULL, 0);
		if (res < 0 && errno != EINTR)
			break;
	} while (res != pid);

	delete _param;
	return 0;
}
#endif

/**
* patch vm config for remote devices and save it on disk
*/
void CDspVm::disconnnectRemoteDevicesAndSaveConfig( )
{
	// Save config
	CVmEvent _evt;
	_evt.addEventParameter(new CVmEventParameter(PVE::String,
		QString(),
		EVT_PARAM_VMCFG_REMOTE_DEVICES ) );

	if (!CDspService::instance()->getVmDirHelper()
		.atomicEditVmConfigByVm( getVmDirUuid(), getVmUuid(), _evt, getVmRunner() ) )
	{
		WRITE_TRACE(DBG_FATAL, "error on saving vm %s configuration after stop", QSTR2UTF8( getVmName() ) );
	}
}

/**
 * Update VM uptime after VM stop
 */
void CDspVm::updateVmUptime()
{
	PRL_UINT64 nVmUptime = getVmUptimeInSecs();
	resetUptime();
	if (nVmUptime)
	{
		WRITE_TRACE(DBG_FATAL, "Updating VM '%s' uptime with new delta %llu.", QSTR2UTF8( getVmName() ), nVmUptime);
		// Save config
		CVmEvent _evt;
		_evt.addEventParameter(new CVmEventParameter(PVE::String,
			QString::number(nVmUptime),
			EVT_PARAM_VM_UPTIME_DELTA ) );

		if (!CDspService::instance()->getVmDirHelper()
			.atomicEditVmConfigByVm( getVmDirUuid(), getVmUuid(), _evt, getVmRunner() ) )
		{
			WRITE_TRACE(DBG_FATAL, "error on uptime of VM %s configuration after stop", QSTR2UTF8( getVmName() ) );
		}
	}
}

/**
 * Resets uptime for VM process
 */
void CDspVm::resetUptime()
{
	QWriteLocker _lock(&d().m_rwLock);
	if (d().m_nVmStartTicksCount)
	{
		WRITE_TRACE(DBG_FATAL, "Resetting uptime of VM process old value=%llu", d().m_nVmStartTicksCount);
		d().m_nVmStartTicksCount = PrlGetTickCount64();
	}
}

DspVm::Details& CDspVm::d()
{
	return *m_pDetails;
}

const DspVm::Details& CDspVm::d() const
{
	return *m_pDetails;
}

void CDspVm::d(const SmartPtr<DspVm::Details>& details_)
{
	m_pDetails = details_;
	if (details_.isValid())
		d().m_nUndoDisksMode = getUndoDisksMode();
}

void CDspVm::d(DspVm::Details* details_)
{
	d(SmartPtr<DspVm::Details>(details_));
}

CDspVm::CDspVm()
{
}

CDspVm::CDspVm(const SmartPtr<DspVm::Details>& details_)
{
	d(details_);
}

CDspVm::~CDspVm()
{
	if (!m_pDetails.isValid())
		return;

	QWriteLocker g(g_pVmsMapLock);
	m_pDetails = g_pStorage->wipeShared(m_pDetails);
	g.unlock();
	if (!m_pDetails.isValid())
		return;

	//Moved all cleanup actions to the separate call
	//see https://bugzilla.sw.ru/show_bug.cgi?id=439127 for more details
	cleanupObject();
	g.relock();
	g_pStorage->wipePending(m_pDetails->m_VmIdent);
}

PRL_RESULT CDspVm::replaceInitDspCmd( PVE::IDispatcherCommands nNewCmd, const SmartPtr<CDspClient> &pUserSession )
{
	PRL_RESULT nRetCode = CDspService::instance()->getVmDirHelper().replaceExclusiveVmOperation(
								getVmUuid(),
								getVmDirUuid(),
								d().m_nInitDispatcherCommand,
								nNewCmd,
								pUserSession);
	if ( PRL_FAILED( nRetCode ) )
	{
		WRITE_TRACE( DBG_FATAL, "Failed to replace VM initial command from %s[%#x] on %s[%#x] due %.8X '%s' error",\
								PVE::DispatcherCommandToString(d().m_nInitDispatcherCommand),\
								d().m_nInitDispatcherCommand,\
								PVE::DispatcherCommandToString(nNewCmd),\
								nNewCmd,\
								nRetCode,\
								PRL_RESULT_TO_STRING(nRetCode) );
	}
	else
		d().m_nInitDispatcherCommand = nNewCmd;

	return ( nRetCode );
}

void CDspVm::cleanupObject()
{
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), nRetCode);

	wakeupApplyConfigWaiters();

	VIRTUAL_MACHINE_STATE state = getVmState();
	if ( (state == VMS_STOPPING || state == VMS_STOPPED)
		&& d().m_pUndoDisksUser.isValid()
	)
		setSafeMode(false, d().m_pUndoDisksUser);

	//remove static IPs from DHCP server
	Task_ManagePrlNetService::removeVmIPAddress(pVmConfig);
	Task_ManagePrlNetService::updateVmNetworking(pVmConfig, false);

	if ( CDspService::isServerModePSBM() )
	{
		// Delete firewall
		CFirewallHelper fw(pVmConfig, true);
		fw.Execute();
	}

	if ( d().m_bVmCmdWasExclusiveRegistered )
	{
		// call without checking because
		//		in error case - exception raises in constructor ( CDspVm::CDspVm )
		//		and destructor ( CDspVm::~CDspVm ) doesn't call.
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation( getVmUuid(),
			getVmDirUuid(),
			d().m_nInitDispatcherCommand,
			d().m_hLockOwner );
	}

	// patch config after stop vm
	disconnnectRemoteDevicesAndSaveConfig( );
	updateVmUptime();

	QString vmHome;
	{
		CDspLockedPointer<CVmDirectoryItem> pDirItem
			= CDspService::instance()->getVmDirManager().getVmDirItemByUuid( getVmDirUuid(), getVmUuid() );
		if ( pDirItem )
		{
			vmHome = pDirItem->getVmHome();
			if( ! turnOnSpotlight( CFileHelper::GetFileRoot( pDirItem->getVmHome() ) ) )
				WRITE_TRACE(DBG_FATAL, "turnOnSpotlight() failed for vm = %s", QSTR2UTF8( getVmName() ) );
		}
	}
	changeUsbState((PRL_EVENT_TYPE)PET_DSP_EVT_VM_STOPPED);

	// Send to all VM client new VM state
	// #464950 Should be last call in method to prevent races
	if ( CDspVm::suspendedFilesPresent( getVmUuid(), getVmDirUuid() ) )
		changeVmState(VMS_SUSPENDED);
	else
		changeVmState(VMS_STOPPED);

	processDeferredResponses();
	if ( !d().m_bFinishedByDispatcher )
		storeRunningState(false);
#ifndef _WIN_
	if (d().m_VmProcessId <=0 )
		return;

	if ( !d().m_bVmIsChild )
		return;

	pthread_t pt;
	struct wait_helper_param *param = new wait_helper_param(d().m_VmProcessId, getVmUuid());
	int ret = ::pthread_create(&pt, NULL, wait_helper, (void *)param);
	if (ret != 0) {
		delete param;
		return;	/* leak zombie. Nothing can be done */
	}
	::pthread_detach(pt);
#endif
}

void CDspVm::processDeferredResponses()
{
	QWriteLocker lock( &d().m_rwLock);

	DspVm::Details::RequestResponseHash::iterator stopIt = d().m_hashRequestResponse.begin();
	for( ; stopIt!= d().m_hashRequestResponse.end(); stopIt++ )
	{
		RequestInfo& info = *stopIt;

		PRL_ASSERT(info.pkgRequest);
		const PVE::IDispatcherCommands
			cmd = info.pkgRequest
				? (PVE::IDispatcherCommands)info.pkgRequest->header.type
				: PVE::DspIllegalCommand;

		SmartPtr<IOPackage> pResponse;
		if(info.pkgResponse)
			pResponse = info.pkgResponse;
		else
		{
			// Workaround for https://bugzilla.sw.ru/show_bug.cgi?id=111010
			WRITE_TRACE( DBG_FATAL, "Making fake response to avoid client waiting.");

			CProtoCommandPtr pResponseCommand =
				CProtoSerializer::CreateDspWsResponseCommand( info.pkgRequest, PRL_ERR_SUCCESS );
			pResponse =
				DispatcherPackage::createInstance( PVE::DspWsResponse, pResponseCommand, info.pkgRequest );
		}

		if (info.isActionByDispatcher)
			d().m_bFinishedByDispatcher = true;

		WRITE_TRACE( DBG_WARNING, "Send deferred response of command %s(%d) to client"
			, PVE::DispatcherCommandToString(cmd)
			, cmd);

		CDspHandler *pVmHandler = &CDspService::instance()->getVmManager();
		if( !CDspRouter::instance().routePackage( pVmHandler, getVmConnectionHandle(), pResponse ) )
			WRITE_TRACE( DBG_DEBUG, "Unable to route response package" );
	}//for
	d().m_hashRequestResponse.clear();
}


const CVmIdent& CDspVm::ident() const
{
	return d().m_VmIdent;
}

SmartPtr<IOPackage> CDspVm::getStartVmRequestPackage() const
{
	return (d().m_pStartVmPkg);
}

IOSendJob::Handle CDspVm::getStartVmJob() const
{
	return d().m_hStartVmJob;
}

void CDspVm::setMigrateVmRequestPackage(const SmartPtr<IOPackage> &p)
{
	d().m_pMigrateVmPkg = p;
}

SmartPtr<IOPackage> CDspVm::getMigrateVmRequestPackage() const
{
	return (d().m_pMigrateVmPkg);
}

void CDspVm::setMigrateVmConnection(const SmartPtr<CDspDispConnection> &pDispConnection)
{
	d().m_pMigratingVmConnection = pDispConnection;
}

SmartPtr<CDspClient> CDspVm::getVmRunner() const
{
	return d().m_pVmRunner;
}

void CDspVm::setQuestionPacket(const SmartPtr<IOPackage>& p)
{
	QWriteLocker _lock(&d().m_rwLock);
	d().m_pQuestionPacket = p;
}

const SmartPtr<IOPackage>& CDspVm::getQuestionPacket() const
{
	QReadLocker _lock(&d().m_rwLock);
	return d().m_pQuestionPacket;
}

void CDspVm::cancelStopVNCServerOp()
{
	d().m_vncStarter.Terminate();
};

void CDspVm::setRestoredVmProductVersion(const QString& qsVer)
{
	d().m_qsRestoredVmProductVersion = qsVer;
}

QString CDspVm::getRestoredVmProductVersion() const
{
	return d().m_qsRestoredVmProductVersion;
}

void CDspVm::disableStoreRunningState(bool bDisable)
{
	d().m_bStoreRunningStateDisabled = bDisable;
}

PRL_UINT32 CDspVm::getVNCPort()
{
	return d().m_vncStarter.GetPort();
}

SmartPtr<CDspVm> CDspVm::GetRoInstanceByUuid(const QString& vm_, const QString& dir_)
{
	QReadLocker g(g_pVmsMapLock);
	return g_pStorage->finr(MakeVmIdent(vm_, dir_));
}

VIRTUAL_MACHINE_STATE CDspVm::getVmState( const QString& sVmUuid, const QString &sVmDirUuid )
{
	CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
	return s.isValid() ? s->tell(MakeVmIdent(sVmUuid, sVmDirUuid)) : VMS_UNKNOWN;
}

VIRTUAL_MACHINE_STATE CDspVm::getVmState( const CVmIdent& vmIdent )
{
	return getVmState(vmIdent.first, vmIdent.second);
}

void CDspVm::cancelSuspend( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p, const QString &sVmUuid, CancelOperationSupport *pInitiator )
{
	PRL_ASSERT( pUser.getImpl() );
	QString sVmDirUuid = pUser->getVmDirectoryUuid();
	bool bWaitCompletion = false;
	{
		SmartPtr<CDspVm> pVm = GetVmInstanceByUuid(sVmUuid, sVmDirUuid);
		if ( pVm && pVm->getVmState() == VMS_SUSPENDING_SYNC )
			if ( pVm->cancelSuspend( pUser, p ) )
				bWaitCompletion = true;
	}

	if ( bWaitCompletion )
	{
		WRITE_TRACE( DBG_FATAL, "Began waiting to VM suspend cancel for uuid '%s'", QSTR2UTF8(sVmUuid) );
		while ( VMS_SUSPENDING_SYNC == CDspVm::getVmState( sVmUuid, sVmDirUuid ) )
		{
			HostUtils::Sleep(1000);
			if ( pInitiator->operationIsCancelled() )//Task was cancelled - terminate operation
				break;
		}
		WRITE_TRACE( DBG_FATAL, "VM suspend cancel wait was completed for uuid '%s'", QSTR2UTF8(sVmUuid) );
	}
}

VIRTUAL_MACHINE_ADDITION_STATE CDspVm::getVmAdditionState( const QString & sVmUuid,
														 const QString & sVmDirUuid,
														 const CDspTaskHelper * pTaskToExclude )
{
	int state = VMAS_NOSTATE;

	QList< SmartPtr< CDspTaskHelper > >
		taskList = CDspService::instance()->getTaskManager()
		.getAllTasks( );

	foreach( SmartPtr<CDspTaskHelper> pTask, taskList )
	{
		if ( pTask.getImpl() == pTaskToExclude )
			continue;

		// trying cast to needed tasks
		PVE::IDispatcherCommands
			cmd = (PVE::IDispatcherCommands)pTask->getRequestPackage()->header.type;
		switch( cmd )
		{
		case PVE::DspCmdDirVmClone:
			{
				Task_CloneVm * pClone = dynamic_cast<Task_CloneVm*>( pTask.getImpl() );
				if( !pClone )
					break;

				if ( ( pClone->getVmUuid() == sVmUuid ) &&
					( pClone->getClient()->getVmDirectoryUuid() == sVmDirUuid ) )
					state |= VMAS_CLONING;
			}
			break;
		case PVE::DspCmdDirVmMove:
			{
				Task_MoveVm * pMove = dynamic_cast<Task_MoveVm*>( pTask.getImpl() );
				if( !pMove )
					break;

				if ( ( pMove->getVmUuid() == sVmUuid ) &&
					( pMove->getClient()->getVmDirectoryUuid() == sVmDirUuid ) )
					state |= VMAS_MOVING;
			}
			break;
		default:
			break;
		}
	}
	return (VIRTUAL_MACHINE_ADDITION_STATE)state;
}

bool CDspVm::suspendedFilesPresent( const QString &sVmUuid, const QString &sVmDirUuid )
{
	CDspLockedPointer<CVmDirectoryItem> pDirectoryItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid( sVmDirUuid, sVmUuid );
	if ( pDirectoryItem.getPtr() )
	{
		CStatesHelper sh(pDirectoryItem->getVmHome());
		if (sh.savFileExists())
			return true;
	}
	return false;
}

bool CDspVm::IsVmRemoteDisplayRunning( const QString &sVmUuid, const QString &sVmDirUuid )
{
	SmartPtr<CDspVm> pVm = GetRoInstanceByUuid(sVmUuid, sVmDirUuid);
	if (!pVm)
		return false;
	return pVm->d().m_vncStarter.IsRunning();
}

VIRTUAL_MACHINE_STATE CDspVm::getVmState() const
{
	QReadLocker _lock(&d().m_rwLock);
	return getVmStateUnsync();
}

VIRTUAL_MACHINE_STATE CDspVm::getVmStateUnsync() const
{
	return (d().m_nVmState);
}

PRL_VM_TOOLS_STATE CDspVm::getVmToolsState( const CVmIdent &_vm_ident  )
{
	return getVmToolsState( _vm_ident.first, _vm_ident.second );
}

PRL_VM_TOOLS_STATE CDspVm::getVmToolsState( const QString &sVmUuid, const QString &sVmDirUuid )
{
	QString sToolsState= getVmToolsStateString( sVmUuid, sVmDirUuid );
	CVmEvent e(sToolsState);
	CVmEventParameter* pParam = e.getEventParameter(EVT_PARAM_VM_TOOLS_STATE);
	PRL_VM_TOOLS_STATE vmToolsState = PTS_UNKNOWN;
	if (pParam)
		vmToolsState = (PRL_VM_TOOLS_STATE)pParam->getParamValue().toInt();
	return (vmToolsState);
}

QString CDspVm::getVmToolsStateString( const QString& sVmUuid, const QString &sVmDirUuid )
{
	SmartPtr<CDspVm> pVm = GetRoInstanceByUuid(sVmUuid, sVmDirUuid);
	if (pVm)
		return (pVm->getVmToolsStateString());

	return DspVm::Details::getVmToolsStateStringOffline(MakeVmIdent(sVmUuid, sVmDirUuid));
}

QString CDspVm::getVmToolsStateString() const
{
	QReadLocker _lock(&d().m_rwLock);
	return (d().m_sVmToolsState);
}

QString CDspVm::getVmName() const
{
	return (d().m_sVmName);
}

Q_PID CDspVm::getVmProcessId() const
{
	return (d().m_VmProcessId);
}

quint32 CDspVm::getVmProcessIdAsUint() const
{
	quint32 nProcId = 0;
#ifdef _WIN_
	if (getVmProcessId())
		nProcId = getVmProcessId()->dwProcessId;
#else
	nProcId = getVmProcessId();
#endif
	return (nProcId);
}

PRL_UINT64 CDspVm::getVmUptimeTimestamp() const
{
	QReadLocker _lock(&d().m_rwLock);
	return (d().m_nVmStartTicksCount);
}

PRL_UINT64 CDspVm::getStartTimestamp() const
{
	QReadLocker _lock(&d().m_rwLock);
	return (d().m_nVmProcStartTicksCount);
}

PRL_UINT64 CDspVm::getVmUptimeInSecsInternal(PRL_UINT64 nStartTicksCount) const
{
	static PRL_UINT64 g_nTicksPerSecond = 0;
	if ( !g_nTicksPerSecond )
		g_nTicksPerSecond = PrlGetTicksPerSecond();
	if ( !nStartTicksCount )
		return ( 0 );
	PRL_ASSERT(nStartTicksCount <= PrlGetTickCount64());
	return ((PrlGetTickCount64() - nStartTicksCount)/g_nTicksPerSecond);
}

PRL_UINT64 CDspVm::getVmUptimeInSecs() const
{
	return ( getVmUptimeInSecsInternal( getVmUptimeTimestamp() ) );
}

PRL_UINT64 CDspVm::getVmProcessUptimeInSecs() const
{
	return ( getVmUptimeInSecsInternal( getStartTimestamp() ) );
}

/** VM connection object handle */
IOSender::Handle CDspVm::getVmConnectionHandle() const
{
	return d().m_VmConnectionHandle;
}

bool CDspVm::isConnected() const
{
	QReadLocker g(&d().m_rwLock);
	return getVmProcessId() && io().clientState(getVmConnectionHandle()) == IOSender::Connected;
}

PRL_RESULT CDspVm::checkUserAccessRights(
		SmartPtr<CDspClient> pUserSession,
		PVE::IDispatcherCommands cmd)
{
	return d().authorize(pUserSession, cmd).getEventCode();
}

PRL_RESULT CDspVm::checkUserAccessRightsAndSendResponseOnError(
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage> &p,
		PVE::IDispatcherCommands cmd)
{
	CVmEvent pErrorInfo = d().authorize(pUserSession, cmd);
	if (PRL_FAILED(pErrorInfo.getEventCode()))
		pUserSession->sendResponseError(pErrorInfo, p);

	return pErrorInfo.getEventCode();
}

bool CDspVm::start(SmartPtr<CDspClient> pUser,
				   const SmartPtr<IOPackage> &p,
				   PRL_VM_START_MODE nStartMode)
{
	PRL_RESULT rc = checkUserAccessRightsAndSendResponseOnError(pUser, p, PVE::DspCmdVmStart );
	if ( PRL_FAILED( rc ) )
		return false;

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
		pUser->sendSimpleResponse(p, PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
		return false;
	}

	bool bHostIsSleeppingNow = CPowerWatcher::isSleeping();

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_STOPPED     : ;
	case VMS_SUSPENDED   : ;
	case VMS_SUSPENDING_SYNC   : ;
	{
		if ( VMS_SUSPENDING_SYNC != state )
		{
			PRL_ASSERT( !getVmProcessId() ); //VM not started yet - let do it now
			if( getVmProcessId() )
				return false;
			d().m_stateBeforeHandshake = state;
		}
		if( VMS_SUSPENDED == state || VMS_SUSPENDING_SYNC == state )
			changeVmState(VMS_RESUMING);
		else
			changeVmState(VMS_STARTING);

		_wLock.unlock();

		PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
		SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(pUser, nRetCode);

		if (state == VMS_STOPPED)
		{
			//add static IPs to network configuration
			Task_ManagePrlNetService::addVmIPAddress(pVmConfig);

		}
		{
			if (nStartMode == PSM_VM_SAFE_START)
			{
				if ( CDspService::isServerModePSBM() )
				{
					WRITE_TRACE( DBG_FATAL, "Start VM: Safe mode is switched off in PSBM !" );
					pUser->sendSimpleResponse(p, PRL_ERR_START_SAFE_MODE_UNSUPPORTED );
					return false;
				}

				if ( VMS_SUSPENDING_SYNC != state )
					setSafeMode(true, pUser);
				else
				{
					WRITE_TRACE( DBG_FATAL, "Start VM: %s, uuid=%s in safe mode cannot be done"
											" due suspend synchronization phase!",
											QSTR2UTF8(getVmName()), QSTR2UTF8(getVmUuid()) );
					pUser->sendSimpleResponse(p, PRL_ERR_SAFE_MODE_START_DURING_SUSPENDING_SYNC);
					return false;
				}
			}

			d().m_pUndoDisksPkg = p;
			d().m_pUndoDisksUser = pUser;

			d().m_nUndoDisksMode = getUndoDisksMode();
			if (   d().m_nUndoDisksMode != PUD_DISABLE_UNDO_DISKS
				&& state == VMS_STOPPED)
			{
				// Create shapshot package for undo disks mode

				disableNoUndoDisksQuestion();

				CProtoCommandPtr pRequest = CProtoSerializer::CreateCreateSnapshotProtoCommand(
												getVmUuid(),
												getVmName(),	// as default name undo disks snapshot
												QString(),
												QString(UNDO_DISKS_UUID) );
				SmartPtr<IOPackage> pPackage
					= DispatcherPackage::duplicateInstance(p, pRequest->GetCommand()->toString());
				pPackage->header.type = PVE::DspCmdVmCreateSnapshot;

				// Prepare and start long running task helper
				changeVmState(VMS_SNAPSHOTING);
				CDspService::instance()->getTaskManager().schedule(new Task_CreateSnapshot( pUser, pPackage, VMS_STOPPED ));
			}
			else if (VMS_SUSPENDING_SYNC == state)
			{
				sendPackageToVmEx(p, state);
			}
			else if(!startVmAfterCommitUnfunishedDiskOp(pUser, p))
			{
				WRITE_TRACE(DBG_FATAL, "Unable to start VM");
				pUser->sendSimpleResponse(p, PRL_ERR_VM_START_FAILED);
				return false;
			}
		}
		break;
	}
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
	case VMS_PAUSED      : ;
		//#PDFM-28812 Skip unpause when host is not ready to work ( host is in sleeping(==waking up) state yet )
		if(	vpsPausedByHostSleep == d().m_nVmPowerState // 1. VM was paused itself by host sleep
			|| bHostIsSleeppingNow // 2. VM was paused by user before host sleep( #PDFM-29265 )
		)
		{
			SEND_ERROR_BY_VM_WAKINGUP_STATE( p );
			return false;
		}

		if( vpsPausedByVmFrozen == d().m_nVmPowerState )
		{
			PRL_ASSERT( CDspService::isServerMode() );

			SEND_ERROR_BY_VM_FROZEN_STATE( p );
			return false;
		}
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_CONTINUING	 : ;
	case VMS_PAUSING	 : ;
		changeVmState( VMS_CONTINUING );
		_wLock.unlock();

		sendPackageToVmEx(p, state);
		break;
	case VMS_RUNNING:
		SEND_ERROR_BY_VM_IS_NOT_STOPPED( p );
		return false;
	//case VMS_MIGRATING: ;
	//case VMS_DELETING_STATE: ;
	default:
		{
			_wLock.unlock();
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return false;
		}//default
	}//switch

	return true;
}

void CDspVm::restartGuest(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmRestartGuest, p);

	sendPackageToVm(p);
}

bool CDspVm::handshakeWasCalled() const
{
	return !d().m_VmConnectionHandle.isEmpty();
}

void CDspVm::abortVmStart(const IOSender::Handle &h,
		SmartPtr<IOPackage> &p, PRL_RESULT res)
{
	CVmEvent evt;

	CDspService::instance()->getIOServer().disconnectClient(h);

	evt.setEventCode(res);
	SmartPtr<CDspClient> pUser = getVmRunner();
	if (pUser && p)
		pUser->sendResponseError(evt, p);
}

void CDspVm::handshakeWithVmProcess(const IOSender::Handle &h)
{
	PRL_RESULT res = PRL_ERR_FAILURE;

	QWriteLocker _lock(&d().m_rwLock);
	d().m_VmConnectionHandle = h;
	SmartPtr<IOPackage> pStartVmRequest = d().m_pStartVmPkg;
	d().m_pStartVmPkg = SmartPtr<IOPackage>();
	SmartPtr<IOPackage> pMigrateVmRequest = d().m_pMigrateVmPkg;
	_lock.unlock();

	if (pMigrateVmRequest.getImpl())//VM process was started at VM migration service mode
	{
		// Do Detach
		bool rc =
			CDspService::instance()->getIOServer().detachClient(
				d().m_pMigratingVmConnection->GetConnectionHandle(),
				getVmProcessIdAsUint(),
				pMigrateVmRequest
			);
		if ( ! rc ) {
			WRITE_TRACE(DBG_FATAL, "Can't detach migration connection!");
			d().m_pMigratingVmConnection->sendSimpleResponse(
				pMigrateVmRequest,
				PRL_ERR_VM_MIGRATE_COULDNT_DETACH_TARGET_CONNECTION
			);
			return;
		}
	}
	else // start/resume/restore Vm
	{
		if (!pStartVmRequest)
		{
			WRITE_TRACE(DBG_FATAL, "FATAL error: handshake procedure performed before start VM request initialization");
			abortVmStart(h, pStartVmRequest, res);
			return;
		}

		//Now send VM necessary initialization data
		CProtoCommandPtr pStartVmCmd = CProtoSerializer::CreateProtoBasicVmCommand(PVE::DspCmdVmStart, getVmUuid());

		// add vm home path to bug #2709
		PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
		SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(getVmRunner(), nRetCode);
		if (!pVmConfig)
		{
			WRITE_TRACE(DBG_FATAL, "Handshake procedure failed: Couldn't to find VM configuration "\
						"for '%s' VM UUID which belongs to '%s' VM dir",\
							getVmUuid().toUtf8().constData(), getVmDirUuid().toUtf8().constData());
			abortVmStart(h, pStartVmRequest, res);
			return;
		}

		QString sVmHome;
		{
			CDspLockedPointer<CVmDirectoryItem> pDirectoryItem =
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid( getVmDirUuid(), getVmUuid() );
			if ( pDirectoryItem.getPtr() )
				sVmHome = pDirectoryItem->getVmHome();
			else
			{
				WRITE_TRACE(DBG_FATAL, "Couldn't to find VM dir item for VM with id '%s' which belongs to '%s' VM dir",\
					getVmUuid().toUtf8().constData(),	getVmDirUuid().toUtf8().constData());
				abortVmStart(h, pStartVmRequest, res);
				return;
			}
		}

		pVmConfig->getVmIdentification()->setHomePath( sVmHome );
		CHostHardwareInfo hostInfo;
		{
			CDspLockedPointer<CDspHostInfo> lockedHostInfo =
				CDspService::instance()->getHostInfo();
			hostInfo.fromString( lockedHostInfo->data()->toString() );
		}

		fixConfigBeforeStartVm( hostInfo, pVmConfig );
		if (CDspService::isServerModePSBM())
		{
			res = Task_EditVm::configureVzParameters(pVmConfig);
			if (PRL_FAILED(res))
			{
				WRITE_TRACE(DBG_FATAL, "Failed to configure Virtuozzo parameters %s [%x]",
					PRL_RESULT_TO_STRING(res),
					res);
				abortVmStart(h, pStartVmRequest, res);
				return;
			}
#ifdef _LIN_
			// set CPU features mask (https://jira.sw.ru/browse/PSBM-11171)
			if (!CCpuHelper::update(*pVmConfig))
				abortVmStart(h, pStartVmRequest, PRL_ERR_FAILURE);
#endif
		}
		pStartVmCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
			, pVmConfig->toString()
			, EVT_PARAM_VM_CONFIG));
		pStartVmCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
			, CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->toString()
			, EVT_PARAM_DISP_COMMON_PREFERENSES));
		pStartVmCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
			, CDspService::instance()->getNetworkConfig()->toString()
			, EVT_PARAM_NETWORK_PREFERENSES));
		pStartVmCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
			, CDspService::instance()->getHostInfo()->data()->toString()
			, EVT_PARAM_HOST_HW_INFO));

		const QString sPath = ParallelsDirs::getVmInfoPath( CFileHelper::GetFileRoot( sVmHome ) );
		SmartPtr<CVmInfo> pVmInfo = CDspVmInfoDatabase::readVmInfo( sPath );
		QString sVmInfo;
		if ( pVmInfo )
			sVmInfo = pVmInfo->toString();

		pStartVmCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
			, sVmInfo
			, EVT_PARAM_VM_INFO));

		// If the cause of VM start - switching to snapshot,
		// we must add "snapshot uuid" and "task uuid" parameters
		if (pStartVmRequest->header.type == PVE::DspCmdVmSwitchToSnapshot)
		{
			pStartVmRequest->header.type = PVE::DspCmdVmStart;

			CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSwitchToSnapshot, UTF8_2QSTR(pStartVmRequest->buffers[0].getImpl()));
			CProtoSwitchToSnapshotCommand *pSwitchCmd = CProtoSerializer::CastToProtoCommand<CProtoSwitchToSnapshotCommand>(cmd);
			pStartVmCmd->GetCommand()->addEventParameter( new CVmEventParameter(PVE::String,
															pSwitchCmd->GetSnapshotUuid(),
															EVT_PARAM_SWITCH_TO_SNAPSHOT_UUID) );

			CVmEventParameter *pTaskUuid = pSwitchCmd->GetCommand()->getEventParameter(EVT_PARAM_DISP_TASK_UUID);

			PRL_ASSERT( pTaskUuid );

			pStartVmCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
				, pTaskUuid->getParamValue()
				, EVT_PARAM_DISP_TASK_UUID));
		}

		SmartPtr<IOPackage>
			pStartVmPkg = DispatcherPackage::duplicateInstance( pStartVmRequest
			, pStartVmCmd->GetCommand()->toString() );

		d().m_hStartVmJob = sendPackageToVmEx( pStartVmPkg, d().m_stateBeforeHandshake );
	}
}

SmartPtr<IOPackage> CDspVm::CreateVmMigrationPackageWithAdditionalInfo(
	CProtoCommandPtr pMigrateVmCmd,
	const SmartPtr<IOPackage> &pMigrateVmRequest,
	VIRTUAL_MACHINE_STATE nPrevVmStateBeforeMigration)
{
	//Insert to migrate VM package additional service info
	pMigrateVmCmd->GetCommand()->addEventParameter(
		new CVmEventParameter(PVE::String
		, CDspService::instance()->getHostInfo()->data()->toString()
		, EVT_PARAM_HOST_HARDWARE_INFO
		)
	);
	pMigrateVmCmd->GetCommand()->addEventParameter(
		new CVmEventParameter(PVE::UnsignedInt
		, QString("%1").arg(nPrevVmStateBeforeMigration)
		, EVT_PARAM_VMINFO_VM_STATE
		)
	);
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig;
	if ( getVmRunner() )
	{
		pVmConfig = CDspService::instance()->getVmDirHelper().
			getVmConfigByUuid(getVmRunner(),
				getVmUuid(),
				nRetCode);
			/* this method always will load config with absolute path */
		/* migration should use config only with _relative_ path (##456619,456716) */
		if (pVmConfig)
			pVmConfig->setRelativePath();
	}
	else
	{
		pVmConfig = CDspService::instance()->getVmDirHelper().
			getVmConfigByUuid(getVmDirUuid(),
				getVmUuid(),
				nRetCode,
				false /* will load config with relative path */);
	}
	if (!pVmConfig)
	{
		WRITE_TRACE(DBG_FATAL, "Handshake procedure failed: Couldn't to find VM configuration for '%s'"\
					"VM UUID which belongs to '%s' VM dir",\
						getVmUuid().toUtf8().constData(), getVmDirUuid().toUtf8().constData());
		return (SmartPtr<IOPackage>());
	}
	CHostHardwareInfo hostInfo;
	{
		CDspLockedPointer<CDspHostInfo> lockedHostInfo =
			CDspService::instance()->getHostInfo();
		hostInfo.fromString( lockedHostInfo->data()->toString() );
	}
	CDspLockedPointer< CVmDirectoryItem > pVmDirItem =
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid(getVmDirUuid(), getVmUuid());
	if (!pVmDirItem)
	{
		WRITE_TRACE(DBG_FATAL, "Handshake procedure failed: Couldn't to find VM directory for '%s'"\
					"VM UUID which belongs to '%s' VM dir",\
						getVmUuid().toUtf8().constData(), getVmDirUuid().toUtf8().constData());
		return (SmartPtr<IOPackage>());
	}
	pVmConfig->getVmIdentification()->setHomePath( pVmDirItem->getVmHome() );

	fixConfigBeforeStartVm( hostInfo, pVmConfig );
	//Update VM uptime
	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
	PRL_UINT64 nVmUptime = getVmUptimeInSecsInternal(d().m_nVmStartTicksCount);
	if (nVmUptime)
		pVmConfig->getVmIdentification()->setVmUptimeInSeconds(
			pVmConfig->getVmIdentification()->getVmUptimeInSeconds() +
			nVmUptime
		);
	pMigrateVmCmd->GetCommand()->addEventParameter(
		new CVmEventParameter(PVE::String
		, pVmConfig->toString()
		, EVT_PARAM_VM_CONFIG
		)
	);

	return (DispatcherPackage::duplicateInstance(
			pMigrateVmRequest,
			pMigrateVmCmd->GetCommand()->toString()
		));
}

void CDspVm::startVNCServer (SmartPtr<CDspClient> pUser,
							 const SmartPtr<IOPackage> &pkg,
							 bool bIsRequestFromVm,
							 bool bIsCallFromNativeThread)
{
	if ( bIsCallFromNativeThread )//Fork execution context here to prevent blocking of transport thread
	{
		SmartPtr<CDspVm>
			pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getVmDirUuid() );
		if ( !pVm )
		{
			WRITE_TRACE(DBG_FATAL, "Abnormal situation: '%s' '%s' VM is not started but start VNC server called",\
				QSTR2UTF8( getVmName() ), QSTR2UTF8( getVmUuid() ) );
			CVmEvent _error;
			_error.setEventCode( PRL_ERR_DISP_VM_IS_NOT_STARTED );
			_error.addEventParameter( new CVmEventParameter (
				PVE::String,
				getVmName(),
				EVT_PARAM_MESSAGE_PARAM_0 )
				);
			if ( pkg )
				pUser->sendResponseError( _error, pkg );
			return;
		}
		CDspService::instance()->getTaskManager().schedule(new Task_StartVncServer( pUser, pkg, pVm, bIsRequestFromVm ));
		return;
	}

	if ( !bIsRequestFromVm )
	{
		PRL_RESULT rc = checkUserAccessRights(pUser, PVE::DspCmdVmStartVNCServer );
		if ( PRL_FAILED( rc ) )
		{
			WRITE_TRACE(DBG_FATAL, "Unauthorized attempt on start VNC server from user session '%s' '%s' %.8X '%s'",\
				QSTR2UTF8( pUser->getClientHandle() ) , QSTR2UTF8( pUser->getAuthHelper().getUserFullName() ),\
				rc, PRL_RESULT_TO_STRING( rc ) );
			if ( pkg )
				pUser->sendSimpleResponse( pkg, rc );
			return;
		}
	}

	PRL_VM_REMOTE_DISPLAY_MODE mode = PRD_DISABLED;

	try
	{
		if ( d().m_vncStarter.IsRunning() )
		{
			if ( !bIsRequestFromVm )
				throw PRL_ERR_VNC_SERVER_ALREADY_STARTED;
			else
			{
				//https://bugzilla.sw.ru/show_bug.cgi?id=465896
				//VNC server already started - do nothing
				return;
			}
		}

		PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
		SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(pUser, nRetCode);
		if( !pVmConfig )
			throw nRetCode;

		CVmSettings* settings = pVmConfig->getVmSettings();
		CVmRemoteDisplay* remDisplay = settings->getVmRemoteDisplay();

		mode = remDisplay->getMode();
		if ( mode == PRD_DISABLED )
			throw PRL_ERR_VNC_SERVER_DISABLED;

		CDispRemoteDisplayPreferences* rdConfig =
			CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getRemoteDisplayPreferences();

		nRetCode = d().m_vncStarter.Start( getVmUuid(), remDisplay, rdConfig->getBasePort());
		if ( PRL_FAILED( nRetCode ) )
			throw nRetCode;

		if ( mode == PRD_AUTO )
		{
			// send event to GUI for changing the config params
			CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, getVmUuid(), PIE_DISPATCHER );
			SmartPtr<IOPackage> pkgNew = DispatcherPackage::createInstance( PVE::DspVmEvent, event, pkg );
			CDspService::instance()->getClientManager()
				.sendPackageToVmClients( pkgNew, getVmDirUuid(), getVmUuid() );

		}

		if ( pkg && !bIsRequestFromVm )
			CDspService::instance()->sendSimpleResponseToClient( pUser->getClientHandle(), pkg, PRL_ERR_SUCCESS );
	}
	catch (PRL_RESULT nRetCode)
	{
		WRITE_TRACE(DBG_FATAL, "Start VNC server failed with error code: %.8X '%s'", nRetCode, PRL_RESULT_TO_STRING( nRetCode ) );

		// #129342
		if( bIsRequestFromVm )
		{
			CVmEvent event( PET_DSP_EVT_VM_MESSAGE
				, getVmUuid()
				, PIE_DISPATCHER
				, PRL_WARN_FAILED_TO_START_VNC_SERVER );

			SmartPtr<IOPackage> pWarn = DispatcherPackage::createInstance(
				PVE::DspVmEvent, event, getStartVmRequestPackage() );

			SmartPtr<CDspClient> pVmRunner = getVmRunner();
			if( pVmRunner )
				pVmRunner->sendPackage( pWarn );
		}

		if ( pkg )
			CDspService::instance()->sendSimpleResponseToClient( pUser->getClientHandle(), pkg, nRetCode );
	}
}

void CDspVm::stopVNCServer ( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p, bool bIsRequestFromVm, bool bIsCallFromNativeThread )
{
	if ( bIsCallFromNativeThread )//Fork execution context here to prevent blocking of transport thread
	{
		SmartPtr<CDspVm>
			pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getVmDirUuid() );
		if ( !pVm )
		{
			WRITE_TRACE(DBG_FATAL, "Abnormal situation: '%s' '%s' VM is not started but stop VNC server called",\
				QSTR2UTF8( getVmName() ), QSTR2UTF8( getVmUuid() ) );
			CVmEvent _error;
			_error.setEventCode( PRL_ERR_DISP_VM_IS_NOT_STARTED );
			_error.addEventParameter( new CVmEventParameter (
				PVE::String,
				getVmName(),
				EVT_PARAM_MESSAGE_PARAM_0 )
				);
			pUser->sendResponseError( _error, p );
			return;
		}
		CDspService::instance()->getTaskManager().schedule(new Task_StopVncServer( pUser, p, pVm, bIsRequestFromVm ));
		return;
	}

	if ( !bIsRequestFromVm )
	{
		PRL_RESULT rc = checkUserAccessRightsAndSendResponseOnError(pUser, p,  PVE::DspCmdVmStopVNCServer );
		if ( PRL_FAILED( rc ) )
		{
			WRITE_TRACE(DBG_FATAL, "Unauthorized attempt on stop VNC server from user session '%s' '%s' %.8X '%s'",\
				QSTR2UTF8( pUser->getClientHandle() ) , QSTR2UTF8( pUser->getAuthHelper().getUserFullName() ),\
				rc, PRL_RESULT_TO_STRING( rc ) );
			return;
		}
	}

	PRL_RESULT nRetCode = d().m_vncStarter.Terminate();
	if ( PRL_FAILED( nRetCode ) )
		WRITE_TRACE(DBG_FATAL, "Failed to stop VNC server with error code: %.8X '%s'", nRetCode, PRL_RESULT_TO_STRING( nRetCode ) );

	CDspService::instance()->sendSimpleResponseToClient( pUser->getClientHandle(), p, nRetCode );
}

void CDspVm::dropLimits()
{
	PRL_CPULIMIT_DATA cpuLimit;

	cpuLimit.value = 0;
	cpuLimit.type = PRL_CPULIMIT_PERCENTS;


	Task_EditVm::SetCpuLimit(getVmUuid(), &cpuLimit);
}

void CDspVm::stop(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p, PRL_UINT32 nStopMode, bool bActionByDispatcher)
{
	PRL_RESULT rc = checkUserAccessRights(pUser, PVE::DspCmdVmStart );
	if ( PRL_FAILED( rc ) && !bActionByDispatcher )
	{
		// for bug #109285 https://bugzilla.sw.ru/show_bug.cgi?id=109285
		switch (rc)
		{
			case PRL_ERR_VM_CONFIG_DOESNT_EXIST:
			case PRL_ERR_PARSE_VM_CONFIG:
			case PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID:
			case PRL_ERR_VM_CONFIG_INVALID_VM_UUID:
				break;
			default:
				pUser->sendSimpleResponse( p, rc );
				return;
		}
	}

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
			// case VMS_STOPPED     : ;
		case VMS_STARTING    : ;
		case VMS_RESUMING: ;
		case VMS_RESTORING   : ;

		case VMS_RUNNING     : ;
		case VMS_PAUSED      : ;
		case VMS_SUSPENDING  : ;
			// case VMS_STOPPING    : ;
			// case VMS_COMPACTING  : ;
			// case VMS_SUSPENDED   : ;
			// case VMS_SUSPENDING_SYNC   : ;
			// case VMS_SNAPSHOTING : ;
			// case VMS_RESETTING	 : ;
		case VMS_CONTINUING	 : ;
		case VMS_PAUSING	 : ;
		case VMS_MIGRATING: ;
			if ( nStopMode == PSM_KILL )
			{
				changeVmState(VMS_STOPPING);
				break;
			}

			if ( VMS_RUNNING == state )
			{
				// #PDFM-31258: Frozen state must be the substate of Runnig state for PD
				if( vpsPausedByVmFrozen == d().m_nVmPowerState )
				{
					PRL_ASSERT( !CDspService::isServerMode() );

					SEND_ERROR_BY_VM_FROZEN_STATE( p );
					return;
				}
				break;
			}
			// else --> cause warning treated as error on Linux; // go to default: ( reject 'shutdown' request )

		//case VMS_DELETING_STATE: ;
		default:
		{
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return;
		}//default
	}//switch
	_wLock.unlock();

	if (isUndoDisksMode())
	{
		d().m_pUndoDisksPkg = p;
		d().m_pUndoDisksUser = pUser;
	}

	if (bActionByDispatcher)
	{
		dropLimits();
	}

	sendPackageToVmEx(p, state, !bActionByDispatcher, bActionByDispatcher);

}

PRL_RESULT CDspVm::sendProblemReport(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	if (!isConnected())
		return PRL_ERR_VM_PROCESS_IS_NOT_STARTED;

	PRL_RESULT ret = checkUserAccessRights( pUser, PVE::DspCmdVmGetProblemReport );

	IOSendJob::Handle handle;
	QString strValid;

	if( PRL_FAILED( ret ) )
	{
		bool bForceSend = ( ret == PRL_ERR_VM_CONFIG_DOESNT_EXIST ) && ( getVmState() == VMS_RUNNING );
		if ( !bForceSend )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to send problem report package to VM by error %s"
				, PRL_RESULT_TO_STRING(ret) );
			return PRL_ERR_FAILURE;
		}
	}

	handle = sendPackageToVm(p);
	strValid = handle.isValid()?"Valid":"Invalid";
	WRITE_TRACE(DBG_FATAL, "%s Problem report Package was posted to VM", QSTR2UTF8( strValid ) );
	// if handle is invalid - request was posted by timeout later!
	return PRL_ERR_SUCCESS;
}

void CDspVm::reset(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmReset, p);

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_PAUSED      : ;
		if( vpsPausedByHostSleep == d().m_nVmPowerState )
		{
			SEND_ERROR_BY_VM_WAKINGUP_STATE( p );
			return;
		}
		if( vpsPausedByVmFrozen == d().m_nVmPowerState )
		{
			PRL_ASSERT( CDspService::isServerMode() );
			SEND_ERROR_BY_VM_FROZEN_STATE( p );
			return;
		}
		break;

		// case VMS_STOPPED     : ;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
	case VMS_RUNNING     : ;
		if( vpsPausedByVmFrozen == d().m_nVmPowerState )
		{
			PRL_ASSERT( ! CDspService::isServerMode() );
			SEND_ERROR_BY_VM_FROZEN_STATE( p );
			return;
		}
		break;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
	case VMS_CONTINUING	 : ;
	case VMS_PAUSING	 : ;
			break;
	//case VMS_MIGRATING: ;
	//case VMS_DELETING_STATE: ;
	default:
		{
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return;
		}//default
	}//switch

	changeVmState(VMS_RESETTING);
	_wLock.unlock();

	sendPackageToVmEx(p, state);
}

void CDspVm::InternalCmd(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmInternal, p);

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_RUNNING     : ;
	case VMS_PAUSED      : ;
	case VMS_CONTINUING	 : ;
	case VMS_PAUSING	 : ;
	case VMS_RESETTING   : ;
	case VMS_STOPPING    : ;
			break;
	default:
		{
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return;
		}//default
	}//switch
	_wLock.unlock();

	sendPackageToVmEx(p, state);
}

void CDspVm::pause(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmPause, p);

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
		// case VMS_STOPPED     : ;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
	case VMS_RUNNING     : ;
		if( vpsPausedByVmFrozen == d().m_nVmPowerState )
		{
			PRL_ASSERT( ! CDspService::isServerMode() );
			SEND_ERROR_BY_VM_FROZEN_STATE( p );
			return;
		}

		//case VMS_PAUSED      : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_PAUSING	 : ;
		// case VMS_CONTINUING	 : ;

		changeVmState( VMS_PAUSING );
		break;
	//case VMS_MIGRATING: ;
	//case VMS_DELETING_STATE: ;
	default:
		{
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return;
		}//default
	}//switch
	_wLock.unlock();

	sendPackageToVmEx(p, state);

	// #7499
	storeRunningState(false);
}

void CDspVm::suspend(SmartPtr<CDspClient> pUser
	, const SmartPtr<IOPackage> &p
	, bool bActionByDispatcher
	, SuspendMode nMode
	)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmSuspend, p);

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
		// case VMS_STOPPED     : ;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
	case VMS_PAUSED      : ;
		if( vpsPausedByHostSleep == d().m_nVmPowerState )
		{
			SEND_ERROR_BY_VM_WAKINGUP_STATE( p );
			return;
		}
		if( vpsPausedByVmFrozen == d().m_nVmPowerState )
		{
			PRL_ASSERT( CDspService::isServerMode() );

			SEND_ERROR_BY_VM_FROZEN_STATE( p );
			return;
		}
		break;

	case VMS_RUNNING     : ;
		if( vpsPausedByVmFrozen == d().m_nVmPowerState )
		{
			PRL_ASSERT( ! CDspService::isServerMode() );

			SEND_ERROR_BY_VM_FROZEN_STATE( p );
			return;
		}

		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_PAUSING	 : ;
		// case VMS_CONTINUING	 : ;

		break;
	//case VMS_MIGRATING: ;
	//case VMS_DELETING_STATE: ;
	default:
		{
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return;
		}//default
	}//switch

	changeVmState(VMS_SUSPENDING);
	_wLock.unlock();

	sendPackageToVmEx(p, state, !bActionByDispatcher, bActionByDispatcher);

	// #9873
	if ( bActionByDispatcher )
	{
		d().m_suspendByDispatcher = true;
		d().m_suspendMode = nMode;
	}
}

bool CDspVm::createSnapshot(
	SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
	CVmEvent* evt , bool bWaitResult)
{
	PRL_RESULT ret = checkUserAccessRights( pUser, PVE::DspCmdVmCreateSnapshot );
	if( PRL_FAILED( ret ) )
	{
		if (evt)
			evt->setEventCode( ret );
		else
			pUser->sendSimpleResponse( p, ret );
		return false;
	}

	if ( d().m_nUndoDisksMode != PUD_DISABLE_UNDO_DISKS)
	{
		bool bSafeMode = isSafeMode();
		WRITE_TRACE(DBG_FATAL, "Cannot create snapshot in %s mode!",
			bSafeMode ? "safe" : "undo disks" );
		if ( bSafeMode )
			ret = PRL_ERR_VM_SNAPSHOT_IN_SAFE_MODE;
		else
			ret = PRL_ERR_VM_SNAPSHOT_IN_UNDO_DISKS_MODE;
		if (evt)
			evt->setEventCode( ret );
		else
			pUser->sendSimpleResponse( p, ret );
		return false;
	}

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_PAUSED      : ;
	case VMS_RUNNING     : ;
		if( !isContinueSnapshotCmdFromPausedStateAllowed( d().m_nVmPowerState, pUser, p, evt, bWaitResult ) )
			return false; // error was send/filled

	case VMS_STOPPED     : ;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
	case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_PAUSING	 : ;
		// case VMS_CONTINUING	 : ;
		changeVmState(VMS_SNAPSHOTING);
		break;
	default:
		{
			if ( !bWaitResult )
				SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state )
			else if (evt)
			{
				WRITE_TRACE(DBG_FATAL, "Error: can't execute %s command,"
						" vm state is forbidden! (state = %#x)",
						PVE::DispatcherCommandToString( p->header.type ),
						state );

				evt->setEventCode( PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED );
				evt->addEventParameter( new CVmEventParameter ( PVE::String,
							getVmName(),
							EVT_PARAM_MESSAGE_PARAM_0 ) );
				evt->addEventParameter( new CVmEventParameter ( PVE::String,
							PRL_VM_STATE_TO_STRING( state ),
							EVT_PARAM_MESSAGE_PARAM_1 ) );
			}
			return false;
		}//default
	}//switch
	_wLock.unlock();

	// Prepare and start long running task helper

	CDspTaskHelper
		*task_helper = new Task_CreateSnapshot( pUser, p, state );

	SetSnapshotRequestParams(p, state, pUser, task_helper->getJobUuid());
	CDspService::instance()->getTaskManager().schedule(task_helper)
		.wait(bWaitResult).getResult(evt);
	return true;
}

bool CDspVm::switchToSnapshot(
		SmartPtr<CDspClient> pUser,
		const SmartPtr<IOPackage> &p,
		CVmEvent* evt,
		bool bWaitResult)
{
	PRL_RESULT ret = checkUserAccessRightsAndSendResponseOnError( pUser, p, PVE::DspCmdVmSwitchToSnapshot );
	if( PRL_FAILED( ret ) )
		return false;

	if ( d().m_nUndoDisksMode != PUD_DISABLE_UNDO_DISKS)
	{
		if (isSafeMode())
		{
			WRITE_TRACE(DBG_FATAL, "Cannot switch to snapshot in safe mode!");
			pUser->sendSimpleResponse(p, PRL_ERR_VM_SNAPSHOT_IN_SAFE_MODE);
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Cannot switch to snapshot in undo disks mode!");
			pUser->sendSimpleResponse(p, PRL_ERR_VM_SNAPSHOT_IN_UNDO_DISKS_MODE);
		}
		return false;
	}

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_PAUSED      : ;
	case VMS_RUNNING     : ;
		if( !isContinueSnapshotCmdFromPausedStateAllowed( d().m_nVmPowerState, pUser, p, evt, bWaitResult ) )
			return false; // error was send/filled

	case VMS_STOPPED     : ;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
	case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_PAUSING	 : ;
		// case VMS_CONTINUING	 : ;
		changeVmState(VMS_RESTORING);
		break;
	default:
		{
			if ( !bWaitResult )
				SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return false;
		}//default
	}//switch
	_wLock.unlock();

	// Prepare and start long running task helper

	CDspTaskHelper
		*task_helper = new Task_SwitchToSnapshot( pUser, p, state );

	SetSnapshotRequestParams(p, state, pUser, task_helper->getJobUuid());
	CDspService::instance()->getTaskManager().schedule(task_helper)
		.wait(bWaitResult).getResult(evt);
	return true;
}

bool CDspVm::deleteSnapshot(
	SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
	CVmEvent* evt, bool bWaitResult)
{
	PRL_RESULT ret = checkUserAccessRightsAndSendResponseOnError( pUser, p, PVE::DspCmdVmDeleteSnapshot );
	if( PRL_FAILED( ret ) )
		return false;

	if ( d().m_nUndoDisksMode != PUD_DISABLE_UNDO_DISKS)
	{
		if (isSafeMode())
		{
			WRITE_TRACE(DBG_FATAL, "Cannot delete snapshot in safe mode!");
			pUser->sendSimpleResponse(p, PRL_ERR_VM_SNAPSHOT_IN_SAFE_MODE);
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Cannot delete snapshot in undo disks mode!");
			pUser->sendSimpleResponse(p, PRL_ERR_VM_SNAPSHOT_IN_UNDO_DISKS_MODE);
		}
		return false;
	}

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_PAUSED      : ;
	case VMS_RUNNING     : ;
		if( !isContinueSnapshotCmdFromPausedStateAllowed( d().m_nVmPowerState, pUser, p, evt, bWaitResult ) )
			return false; // error was send/filled

	case VMS_STOPPED     : ;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
	case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_PAUSING	 : ;
		// case VMS_CONTINUING	 : ;
		changeVmState(VMS_DELETING_STATE);
		break;
	default:
		{
			if ( !bWaitResult )
				SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return false;
		}//default
	}//switch
	_wLock.unlock();

	CDspTaskHelper *task_helper = new Task_DeleteSnapshot( pUser, p, state );
	SetSnapshotRequestParams(p, state, pUser, task_helper->getJobUuid());
	CDspService::instance()->getTaskManager().schedule(task_helper)
		.wait(bWaitResult).getResult(evt);
	return true;
}

bool CDspVm::startSnapshotedVm(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	PRL_RESULT rc = checkUserAccessRightsAndSendResponseOnError(pUser, p, PVE::DspCmdVmStart );
	if ( PRL_FAILED( rc ) )
		return false;

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
		pUser->sendSimpleResponse(p, PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
		return false;
	}

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
	case VMS_RESTORING   : ;
	// case VMS_STOPPED     : ;
		PRL_ASSERT( !getVmProcessId() ); //VM not started yet - let do it now
		if( getVmProcessId() )
			return false;
		changeVmState(VMS_RESTORING);

		_wLock.unlock();

		if( ! startVmProcess( pUser, p ) )
		{
			return false;
		}
		break;
		// case VMS_STARTING    : ;
		//case VMS_RESUMING: ;
		// case VMS_RUNNING     : ;
		// case VMS_PAUSED      : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_CONTINUING	 : ;
		// case VMS_PAUSING	 : ;
	default:
		{
			_wLock.unlock();
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
			return true;; // return true to prevent delete vm object in this case
		}//default
	}//switch

	return true;
}

bool CDspVm::startMigratedVm(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
		pUser->sendSimpleResponse(p, PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
		return false;
	}

	QWriteLocker _wLock( &d().m_rwLock );
	changeVmState(VMS_STARTING);
	_wLock.unlock();
	return startVmProcess( pUser, p );
}

void CDspVm::installUtility(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmInstallUtility, p);

	sendPackageToVm(p);
}

void CDspVm::installTools(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmInstallTools, p);

	sendPackageToVm(p);
}

void CDspVm::updateToolsSection(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmUpdateToolsSection, p);

	sendPackageToVm(p);
}

void CDspVm::runCompressor(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmRunCompressor, p);

	sendPackageToVm(p);
}

void CDspVm::cancelCompressor(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmCancelCompressor, p);

	sendPackageToVm(p);
}

void CDspVm::InitiateDevStateNotifications(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmInitiateDevStateNotifications, p);

	sendPackageToVm(p);
}

void CDspVm::connectDevice(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmDevConnect, p);

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevConnect,
		UTF8_2QSTR(p->buffers[0].getImpl()));

	SmartPtr<IOPackage> pPackageToSend = p;
	if (pCmd->IsValid())
	{
		CProtoVmDeviceCommand *pDeviceCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeviceCommand>(pCmd);
		if (pDeviceCmd)
		{
			switch( pDeviceCmd -> GetDeviceType() )
			{
				case PDE_USB_DEVICE:
					{
						CVmUsbDevice	vmUsbDevice;
						// Deserialize device configuration object
						StringToElement<CVmUsbDevice*>(&vmUsbDevice, pDeviceCmd->GetDeviceConfig());

						CDspService::instance()->getHwMonitorThread().setUsbDeviceManualConnected(
									vmUsbDevice.getSystemName(),
									true );
					}
					break;
				case PDE_OPTICAL_DISK:
					{
						CHostHardwareInfo hostInfo;
						{
							CDspLockedPointer<CDspHostInfo> lockedHostInfo =
								CDspService::instance()->getHostInfo();
							hostInfo.fromString( lockedHostInfo->data()->toString() );
						}

						SmartPtr<IOPackage> pFixedPackage =
							getConnectionFixedPackageForDvd( hostInfo, p, pDeviceCmd, PVE::DspCmdVmDevConnect );
						if ( pFixedPackage.isValid() )
							pPackageToSend = pFixedPackage;
					}
					break;
				default:
					break;
			}
		}
	}
	sendPackageToVm( pPackageToSend );
}

void CDspVm::disconnectDevice(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmDevDisconnect, p);

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevDisconnect,
		UTF8_2QSTR(p->buffers[0].getImpl()));

	SmartPtr<IOPackage> pPackageToSend = p;
	if (pCmd->IsValid())
	{
		CProtoVmDeviceCommand *pDeviceCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeviceCommand>(pCmd);
		if (pDeviceCmd)
		{
			switch( pDeviceCmd -> GetDeviceType() )
			{
			case PDE_USB_DEVICE:
				{
					CVmUsbDevice	vmUsbDevice;
					// Deserialize device configuration object
					StringToElement<CVmUsbDevice*>(&vmUsbDevice, pDeviceCmd->GetDeviceConfig());

					CDspService::instance()->getHwMonitorThread().setUsbDeviceManualConnected(
								vmUsbDevice.getSystemName(),
								false );
				}
				break;
			case PDE_OPTICAL_DISK:
				{
					CHostHardwareInfo hostInfo;
					{
						CDspLockedPointer<CDspHostInfo> lockedHostInfo =
							CDspService::instance()->getHostInfo();
						hostInfo.fromString( lockedHostInfo->data()->toString() );
					}

					SmartPtr<IOPackage> pFixedPackage =
						getConnectionFixedPackageForDvd( hostInfo, p, pDeviceCmd, PVE::DspCmdVmDevDisconnect );
					if ( pFixedPackage.isValid() )
						pPackageToSend = pFixedPackage;

				}
				break;
			default:
				break;
			}
		}
	}
	sendPackageToVm( pPackageToSend );
}

void CDspVm::sendAnswerToVm(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CHECK_WHETHER_VM_STARTED;
	CHECK_USER_ACCESS_RIGHTS(pUser, PVE::DspCmdVmAnswer, p);

	sendPackageToVm(p);
	pUser->sendSimpleResponse(p, PRL_ERR_SUCCESS);
}

void CDspVm::changeVmState(VIRTUAL_MACHINE_STATE nVmNewState, CDspVm::VmPowerState nVmPowerState )
{
	VIRTUAL_MACHINE_STATE nVmCurrentState = getVmStateUnsync();

	bool bVmStateChanged = false;
	{
		QWriteLocker _lock(&d().m_rwLock);

		// forbid transition to VMS_RECONNECTING from any but VMS_STOPPED state
		bool forbidTransition = ((nVmNewState == VMS_RECONNECTING) && (nVmCurrentState != VMS_STOPPED));

		if (forbidTransition)
		{
			WRITE_TRACE(DBG_FATAL, "Unable to change vm state from %s to %s for vm %s (name='%s')"
				, PRL_VM_STATE_TO_STRING( nVmCurrentState )
				, PRL_VM_STATE_TO_STRING( nVmNewState )
				, QSTR2UTF8( getVmUuid() )
				, QSTR2UTF8( getVmName() )
				);
			return;
		}

		WRITE_TRACE(DBG_FATAL, "Vm state was changed from %s to %s for vm %s (name='%s'), to powerState %d(%s)"
			, PRL_VM_STATE_TO_STRING( nVmCurrentState )
			, PRL_VM_STATE_TO_STRING( nVmNewState )
			, QSTR2UTF8( getVmUuid() )
			, QSTR2UTF8( getVmName() )
			, nVmPowerState, VmPowerStateToString(nVmPowerState)
			);

		bVmStateChanged = (nVmCurrentState != nVmNewState);

		d().m_nPrevVmState = nVmCurrentState;
		d().m_nVmState = nVmNewState;

		// check forbidden transition vpsPausedByHostSleep < == > vpsPausedByVmFrozen
		// It should be guaranteed by vm_app state-machine.
		PRL_ASSERT( nVmPowerState == vpsNormal || d().m_nVmPowerState == vpsNormal  );

		d().m_nVmPowerState = nVmPowerState;
	}

	if (bVmStateChanged)
	{
		CDspLockedPointer<CDspVmStateSender>
			pLockedVmStateSender = CDspService::instance()->getVmStateSender();
		if( pLockedVmStateSender )
		{
			pLockedVmStateSender->onVmStateChanged(
				  nVmCurrentState
				, nVmNewState
				, getVmUuid()
				, getVmDirUuid()
				, (getVmProcessIdAsUint() != 0) && (nVmNewState != VMS_RECONNECTING)
				);
		}
	}
}

void CDspVm::changeVmToolsState(const QString &sVmToolsState)
{
	QWriteLocker _lock(&d().m_rwLock);
	d().m_sVmToolsState = sVmToolsState;
	_lock.unlock();
	//https://bugzilla.sw.ru/show_bug.cgi?id=440056
	CVmEvent e(sVmToolsState);
	CVmEventParameter* pParam = e.getEventParameter(EVT_PARAM_VM_TOOLS_STATE);
	if (pParam)
	{
		unsigned int nVmToolsState = (PRL_VM_TOOLS_STATE)pParam->getParamValue().toUInt();
		WRITE_TRACE( DBG_FATAL, "VM tools changed state on %u %.8X", nVmToolsState, nVmToolsState );
	}
}

static bool doStorePackageBeforeSendToVm( const SmartPtr<IOPackage> &p )
{
	PRL_ASSERT( p );
	PVE::IDispatcherCommands cmd = (PVE::IDispatcherCommands)p->header.type;

	// Skip events and internal command between dispatcher and vm
	// because they never return response( + they never used by user )
	// It causes next things:
	//	1. stored packages  eats lot of memory until vm destroyed
	//	2. event flood happens when on vm object  destroy - #PDFM-30692
	//	3. also this event flood may be cause to lose stop/suspend responses
	//		 ( IOService send-buffer overflowed - #PDFM-30616)


	// Skip events and internal command 'DspEvtHwChanged' / 'DspVmEvent'
	if( cmd > PVE::DspVmToClientCommandRangeStart && cmd < PVE::DspVmToClientCommandRangeEnd )
		return false;
	// Skip internal dispatcher commands like 'DspCmdCtlApplyVmConfig'
	if( cmd > PVE::DspCtlCommandRangeStart && cmd < PVE::DspCtlCommandRangeEnd )
		return false;

	return true;
}

IOSendJob::Handle CDspVm::sendPackageToVmEx(const SmartPtr<IOPackage> &p
	, VIRTUAL_MACHINE_STATE nPrevVmState
	, bool bEnableDeferredResponse /*=false*/
	, bool bFinishedByDispatcher /*=false*/
	)
{
	PRL_ASSERT( p );
	PRL_ASSERT( !bFinishedByDispatcher || p->header.type == PVE::DspCmdVmStop ||
		p->header.type == PVE::DspCmdVmSuspend );

	if ( Uuid::toUuid( p->header.uuid ).isNull() )
		Uuid::createUuid( p->header.uuid );


	if( doStorePackageBeforeSendToVm(p) )
	{
		QWriteLocker lock( &d().m_rwLock);

		DspVm::Details::RequestResponseHash::iterator
			it = d().m_hashRequestResponse.find( Uuid::toString(p->header.uuid) );
		if( d().m_hashRequestResponse.end() == it )
			it = d().m_hashRequestResponse.insert(
			Uuid::toString(p->header.uuid), RequestInfo(p, bFinishedByDispatcher) );
		it.value().nPrevVmState = nPrevVmState;

		if( bEnableDeferredResponse )
			it.value().isDeferred = true;
	}

	return (CDspService::instance()->getIOServer().sendPackage(d().m_VmConnectionHandle, p));
}


IOSendJob::Handle CDspVm::sendPackageToVm(const SmartPtr<IOPackage> &p)
{
	return sendPackageToVmEx(p, VMS_UNKNOWN, false);
}

void CDspVm::changeVmState(const SmartPtr<IOPackage> &p )
{
	bool fakeNeedRouteFlag = true;
	changeVmState( p, fakeNeedRouteFlag );
}

void CDspVm::checkAutoconnectUsb()
{
	CDispUsbPreferences _usb_prefs(
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getUsbPreferences()
	);
	foreach(CDispUsbIdentity * pUi, _usb_prefs.m_lstAuthenticUsbMap)
	{	// Look for the device being connected
		// Found
		foreach(CDispUsbAssociation * pUa, pUi->m_lstAssociations)
		{
			if( pUa->getAction() != PUD_CONNECT_TO_GUEST_OS )
				continue;
			if( pUa->getVmUuid() !=  getVmUuid() )
				continue;
			if( pUa->getDirUuid() != getVmDirUuid() )
				continue;

			SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getVmDirUuid() );
			if( !pVm.isValid() )
				continue;

			CDspTaskHelper * pTask = Task_AutoconnectUsbDevice::createTask( pVm,
				pUi->getSystemName(), pUi->getFriendlyName());
			if( pTask == 0 ) // No USB support in current VM
				return;

			CDspService::instance()->getTaskManager().schedule(pTask);
		}
	}
}

void CDspVm::applyVMNetworkSettings(VIRTUAL_MACHINE_STATE nNewState)
{
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;

	if (!CDspService::isServerMode())
		return;

	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), nRetCode);

	if (!pVmConfig || PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL, "Error: failed to get Vm Configuration for network settings");
		return;
	}

		SmartPtr<CDspClient> pUser = getVmRunner();
		if (!pUser.isValid())
				pUser = SmartPtr<CDspClient>(new CDspClient(IOSender::Handle()));

	const SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdCtlDispatherFakeCommand);
	CDspService::instance()->getTaskManager()
		.schedule(new Task_ApplyVMNetworking(pUser, p, pVmConfig, (nNewState == VMS_PAUSED)));
}

void CDspVm::changeVmState(const SmartPtr<IOPackage> &p, bool& outNeedRoute  )
{
	if (p->header.type == PVE::DspVmEvent)
	{
		CVmEvent _evt(UTF8_2QSTR(p->buffers[0].getImpl()));
		const PRL_EVENT_TYPE evtType = (PRL_EVENT_TYPE)_evt.getEventType();

		// to skip log flood like 'progress changed'
		switch(evtType)
		{
		case PET_DSP_EVT_VM_MIGRATE_PROGRESS_CHANGED:
		case PET_JOB_SUSPEND_PROGRESS_CHANGED:
		case PET_JOB_RESUME_PROGRESS_CHANGED:
		case PET_DSP_EVT_VM_COMPACT_PROCESSING:
		case PET_DSP_EVT_VM_IO_CLIENT_STATISTICTS:
			break;

		default:
			WRITE_TRACE(DBG_FATAL, "Recieved event %s (%d) from vm %s (name='%s')"
				, PRL_EVENT_TYPE_TO_STRING( _evt.getEventType() )
				, _evt.getEventType()
				, QSTR2UTF8( getVmUuid() )
				, QSTR2UTF8( getVmName() )
				);
		}

		changeUsbState( (PRL_EVENT_TYPE)_evt.getEventType() );
		VIRTUAL_MACHINE_STATE nNewVmState = VMS_RUNNING;
		switch(evtType)
		{
			/* Change internal VM state */
			case PET_DSP_EVT_VM_STARTED:
				if (VMS_RUNNING == getVmState())
					break;

				if (!d().m_pMigrateVmPkg.isValid())
					applyVMNetworkSettings(VMS_RUNNING);

				changeVmState(VMS_RUNNING);
				break;
			case PET_DSP_EVT_VM_STOPPED:
			case PET_DSP_EVT_VM_ABORTED:
			{
				QWriteLocker lock( &d().m_rwLock );
				VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
				if( VMS_RESTORING != state )
					changeVmState( VMS_STOPPING );
			}
			break;
			case PET_DSP_EVT_VM_RESETED:
				changeVmState(VMS_RESETTING);
				break;
			case PET_DSP_EVT_VM_SUSPENDING:
				changeVmState(VMS_SUSPENDING); break;
			case PET_DSP_EVT_VM_SUSPENDED:
				{
					//see https://bugzilla.parallels.com/show_bug.cgi?id=464867
					//by some reasons VM_SUSPENDED event can income even if not suspend
					//operation was performed
					VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
					if ( state == VMS_SUSPENDING //#PDFM-39381 [Degradation] strange and confusing "suspending" state i see while reverting to suspended state snapshot
						&& CDspVm::suspendedFilesPresent( getVmUuid(), getVmDirUuid() ) )
					{
						changeVmState(VMS_SUSPENDING_SYNC);
						// clear Hdd flags on suspend!
						PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
						SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(getVmRunner(), nRetCode);

						if( pVmConfig )
							CStatesHelper::SetSuspendParameterForAllDisks(pVmConfig.getImpl(),1);
					}

					break;
				}
			case PET_DSP_EVT_VM_RESUMED:
				/* obsolete - replaced by PET_DSP_EVT_VM_CONTINUED */
				changeVmState(VMS_RUNNING);
				break;

			case PET_DSP_EVT_VM_FROZEN:
			case PET_DSP_EVT_VM_PAUSED_BY_HOST_SLEEP:
				{
					QReadLocker _wLock( &d().m_rwLock );
					if( getVmStateUnsync() == VMS_PAUSED )
					{
						WRITE_TRACE( DBG_FATAL, "VM is already in PAUSED state.Event %s will be skipped"
							, PRL_EVENT_TYPE_TO_STRING(evtType) );
						break;
					}
					// continue to setup VMS_PAUSED state
				}
			case PET_DSP_EVT_VM_PAUSED:
				{
					VIRTUAL_MACHINE_STATE curVmState = getVmState();
					VmPowerState pst = vpsNormal;
					if( evtType==PET_DSP_EVT_VM_PAUSED_BY_HOST_SLEEP )
						pst = vpsPausedByHostSleep;
					if( evtType == PET_DSP_EVT_VM_FROZEN )
					{
						pst = vpsPausedByVmFrozen;

						if( !CDspService::isServerMode() )
						{ // #PDFM-31258: Frozen state must be the substate of Runnig state for PD
							QWriteLocker _wLock( &d().m_rwLock );
							WRITE_TRACE( DBG_FATAL, "vm power state was changed from (%d) %s to (%d) %s"
								, d().m_nVmPowerState,  VmPowerStateToString(d().m_nVmPowerState)
								, pst,  VmPowerStateToString(pst) );
							d().m_nVmPowerState = pst;
							break;
						}
						PRL_ASSERT( CDspService::isServerMode() );
					}

					if ( curVmState != VMS_DELETING_STATE && curVmState != VMS_SNAPSHOTING )
						applyVMNetworkSettings(VMS_PAUSED);
					changeVmState(VMS_PAUSED, pst );
					break;
				}
			case PET_DSP_EVT_VM_UNFROZEN:
				if (CDspService::isServerMode())
				{
					// Hack to revert to VMS_MIGRATING state
					QReadLocker g(&d().m_rwLock);
					nNewVmState = d().m_nPrevVmState;
				}
				else
				{
					QWriteLocker _wLock( &d().m_rwLock );
					// #PDFM-31258: Frozen state must be the substate of Runnig state for PD
					WRITE_TRACE( DBG_FATAL, "vm power state was changed from (%d) %s to (%d) %s"
						, d().m_nVmPowerState,  VmPowerStateToString(d().m_nVmPowerState)
						, vpsNormal,  VmPowerStateToString(vpsNormal) );
					d().m_nVmPowerState = vpsNormal;
					break;
				}
			case PET_DSP_EVT_VM_CONTINUED_BY_HOST_WAKEUP:
			{
				QWriteLocker g(&d().m_rwLock);
				if (VMS_PAUSED != getVmStateUnsync())
				{
					// Workaround:
					//	1) to stop VM without races ( when something hangs )
					//	2) Also it workarounded finalizing of snapshot operations
					//		 ( they calls changeVmState( pkg ) method ).
					WRITE_TRACE(DBG_FATAL
						, "VM-state was changed to %s (from PAUSED) during VM waking up."
						" So Event %s will be skipped to prevent races of set vm-state."
						, PRL_VM_STATE_TO_STRING(getVmStateUnsync())
						, PRL_EVENT_TYPE_TO_STRING(evtType));
					d().m_nVmPowerState = vpsNormal;
					break;
				}
				g.unlock();
				applyVMNetworkSettings(nNewVmState);
				changeVmState(nNewVmState);
				break;
			}
			case PET_DSP_EVT_VM_CONTINUED:
				if (d().m_pMigrateVmPkg.isValid())
					break;

				switch (getVmState())
				{
				case VMS_RUNNING:
				case VMS_RESTORING:
					// NB.
					// there is no need to change the vm state
					// to running when it is already running.
					// also, the switch to snapshot task reverts
					// the vm state back to running in the very
					// end anyway and that switch would result
					// in all the associated actions. thus it's
					// of no use to treat the continued event as
					// a trigger of the restoring-running
					// transition.
					break;
				case VMS_SNAPSHOTING:
				case VMS_DELETING_STATE:
					// on the oposite the vm doesn't require all
					// the actions associated with transition to
					// the running state after a snapshot or a
					// delete state job completion. they use a
					// continued event to indicate the operation
					// finish and require a simple state switch
					// to running.
					changeVmState(VMS_RUNNING);
					break;
				case VMS_RESUMING:
				{
					applyVMNetworkSettings(VMS_RUNNING);
					PRL_RESULT e = PRL_ERR_UNINITIALIZED;
					if (!Snapshot::getUnfinishedOps(getVmConfig(getVmRunner(), e))
						.isEmpty())
						Task_CommitUnfinishedDiskOp::pushVmCommit(*this);

					changeVmState(VMS_RUNNING);
					break;
				}
				default:
					applyVMNetworkSettings(VMS_RUNNING);
					changeVmState(VMS_RUNNING);
				}
				break;
			/* Change internal VM tools state */
			case PET_DSP_EVT_VM_TOOLS_STATE_CHANGED:
				changeVmToolsState(_evt.toString());
				break;
			/* Compacting states */
			case PET_DSP_EVT_VM_COMPACT_PROCESSING:
				changeVmState(VMS_COMPACTING); break;
			case PET_DSP_EVT_VM_COMPACT_FINISHED:
				changeVmState(VMS_STOPPING);
				break;

			//Migration events:
			case PET_DSP_EVT_VM_MIGRATE_STARTED:
				/* for Task_MigrateVmSource */
				changeVmState(VMS_MIGRATING);
				break;
			/* Snapshot states */
			case PET_DSP_EVT_VM_RESTORED:
			{
				CVmEventParameter *pTaskUuid = _evt.getEventParameter(EVT_PARAM_DISP_TASK_UUID);
				if (pTaskUuid)
				{
					SmartPtr< CDspTaskHelper > pTask = CDspService::instance()->getTaskManager()
						.findTaskByUuid( pTaskUuid->getParamValue() );
					if (pTask)
					{
						Task_SwitchToSnapshot* pTaskSwitch = reinterpret_cast<Task_SwitchToSnapshot*>(pTask.getImpl());
						pTaskSwitch->handleVmEvents(p);
					}
				}
			}
			break;

			case PET_DSP_EVT_VM_ABOUT_TO_START_DEINIT:
			{
				QWriteLocker lock( &d().m_rwLock );
				VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
				if( VMS_RUNNING == state )
					changeVmState( VMS_STOPPING );

			}
			break;

			default: break;
		}
	}
	else if (p->header.type == PVE::DspWsResponse)
	{
		QString requestUuid = Uuid::toString( p->header.parentUuid  );
		CVmEvent _evt(UTF8_2QSTR(p->buffers[0].getImpl()));

		SmartPtr<IOPackage> pkgRequest(0);
		VIRTUAL_MACHINE_STATE nPrevVmState = VMS_UNKNOWN;
		PVE::IDispatcherCommands cmd = PVE::DspIllegalCommand;

		QWriteLocker lock( &d().m_rwLock);

		DspVm::Details::RequestResponseHash::iterator
			it = d().m_hashRequestResponse.find( requestUuid );

		if( d().m_hashRequestResponse.end() != it )
			cmd = (PVE::IDispatcherCommands)it.value().pkgRequest->header.type;

		WRITE_TRACE(DBG_FATAL, "Received response %s (%#x) for %s(%d) request from vm %s (name='%s')"
			, PRL_RESULT_TO_STRING( _evt.getEventCode() )
			, _evt.getEventCode()
			, PVE::DispatcherCommandToString( cmd )
			, cmd
			, QSTR2UTF8( getVmUuid() )
			, QSTR2UTF8( getVmName() )
			);
		WRITE_TRACE( DBG_DEBUG, "requestId=%s", QSTR2UTF8(requestUuid) );

		if( d().m_hashRequestResponse.end() == it )
			WRITE_TRACE( DBG_DEBUG, "Received response for unknown request. requestId=%s", QSTR2UTF8(requestUuid) );
		else
		{
			bool needEraseIt = true;
			outNeedRoute = PVE::DspCmdDirVmMigrate != cmd;

			RequestInfo& info = it.value();
			pkgRequest = info.pkgRequest;
			if( PRL_SUCCEEDED(_evt.getEventCode()) )
			{
				//////////////////////////////////////////////////////////////////////////
				//
				//  set deferred response
				//
				//////////////////////////////////////////////////////////////////////////
				if( info.isDeferred )
				{
					WRITE_TRACE( DBG_WARNING, "Catching successfully 'stop' response to deferred send." );
					outNeedRoute = false;
					info.pkgResponse = p;

					needEraseIt = false;
				}
				if (info.isActionByDispatcher)
					d().m_bFinishedByDispatcher = true;
			}
			else // ! PRL_SUCCEEDED(_evt.getEventCode())
			{
				if( info.nPrevVmState != VMS_UNKNOWN )
					nPrevVmState = info.nPrevVmState;
			}

			// cleanup hash after receive response
			if(needEraseIt)
				d().m_hashRequestResponse.erase(it);

		}// if( m_hashRequestResponse.end()!=it
		lock.unlock();

		WRITE_TRACE(DBG_DEBUG, "VM %s State %s, cmd %s, refs %d ",
						QSTR2UTF8( getVmName() ),
						PRL_VM_STATE_TO_STRING( getVmStateUnsync() ) ,
						PVE::DispatcherCommandToString( cmd ),
						d().m_pSuspendMounter.countRefs()	);
		if (cmd == PVE::DspCmdVmStart)
		{
			//destroy suspend helper
			d().m_pSuspendMounter = SmartPtr<CDspVmSuspendMounter>();
		}

		//////////////////////////////////////////////////////////////////////////
		//
		// rollback vm state to previous state
		// (to exclude DspCmdDirVmMigrate command: CDspVmMigrateProcessesCleaner task will do it)
		//
		//////////////////////////////////////////////////////////////////////////
		if( PRL_FAILED( _evt.getEventCode() ) && (nPrevVmState != VMS_UNKNOWN) && (cmd != PVE::DspCmdDirVmMigrate))
		{
			WRITE_TRACE(DBG_FATAL, "Command %s(%d) was failed. Rollback VM to previous state %s(%#x)"
				"VmUuid=%s (name=%s)"
				, PVE::DispatcherCommandToString( cmd )
				, cmd
				, PRL_VM_STATE_TO_STRING( nPrevVmState)
				, nPrevVmState
				, QSTR2UTF8( getVmUuid() )
				, QSTR2UTF8( getVmName() )
				);
			if( VMS_STOPPED == nPrevVmState )
				nPrevVmState = VMS_STOPPING;
			else if( VMS_SUSPENDED == nPrevVmState )
				//See https://bugzilla.sw.ru/show_bug.cgi?id=466300
				//when resume failed we have two possible scenarios:
				// * VM restarting
				// * VM process finalize it work and VM became again suspended
				//for both situations state RESETTING will be proper way than SUSPENDING
				nPrevVmState = VMS_RESETTING;

			changeVmState( nPrevVmState );
		}

		//////////////////////////////////////////////////////////////////////////
		//
		// process failed responses
		//
		//////////////////////////////////////////////////////////////////////////
		if( PRL_SUCCEEDED( _evt.getEventCode() ) )
			processResponseForGuestOsSessionsCmds( p );
		else //PRL_FAILED()
		{
			// #271042
			if( (cmd == PVE::DspCmdVmStart || cmd == PVE::DspCmdVmStartEx)
				&& isUndoDisksMode())
			{
				d().m_bNoUndoDisksQuestion = true;
			}

			// #9875
			if( cmd == PVE::DspCmdVmSuspend && d().m_suspendByDispatcher )
			{
				SmartPtr<CDspClient> pUser( new CDspClient(IOSender::Handle()) );
				pUser->getAuthHelper().AuthUserBySelfProcessOwner();
				pUser->setVmDirectoryUuid(getVmDirUuid());

				switch (d().m_suspendMode)
				{
				case SM_SUSPEND_ON_FAILURE:
					WRITE_TRACE(DBG_FATAL, "TRY TO SUSPEND");
					suspend(pUser,
							DispatcherPackage::createInstance( PVE::DspCmdVmSuspend ),
							true,
							SM_STOP_ON_FAILURE);
					break;
				case SM_STOP_ON_FAILURE:
				{
					WRITE_TRACE(DBG_FATAL, "TRY TO STOP");
					CProtoCommandPtr pCmd =
						CProtoSerializer::CreateProtoVmCommandStop(getVmUuid(), PSM_KILL, 0);
					const SmartPtr<IOPackage> _p =
						DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
					stop(pUser, _p, PSM_KILL, true);
					break;
				}
				}
			}// if( cmd == PVE::DspCmdVmSuspend
		}
	}// else if (p->header.type == PVE::DspWsResponse)
}

void CDspVm::GetSnapshotRequestParams(SmartPtr<IOPackage> &pRequest, VIRTUAL_MACHINE_STATE &vmState,
									  SmartPtr<CDspClient> &pUser, QString &sSnapshotTaskUuid)
{
	QMutexLocker _lock(&d().m_SnapshotRequestMutex);
	pRequest = d().m_pSnapshotRequest;
	vmState = d().m_SnapshotVmState;
	pUser = d().m_pSnapshotUser;
	sSnapshotTaskUuid = d().m_sSnapshotTaskUuid;
}

void CDspVm::SetSnapshotRequestParams(const SmartPtr<IOPackage> &pRequest, VIRTUAL_MACHINE_STATE vmState,
									  const SmartPtr<CDspClient> &pUser, const QString &sSnapshotTaskUuid)
{
	QMutexLocker _lock(&d().m_SnapshotRequestMutex);
	d().m_pSnapshotRequest = pRequest;
	d().m_SnapshotVmState = vmState;
	d().m_pSnapshotUser = pUser;
	d().m_sSnapshotTaskUuid = sSnapshotTaskUuid;
}

template<class T>
bool CDspVm::startVmProcess(DspVm::Start::Demand demand_, T monitor_)
{
	do
	{
		DspVm::Start::Setup s(demand_, d());
		if (!s.check())
			break;
		if (!s.prepare())
			break;
		SmartPtr<CDspClient> u = s.getUser();
		SmartPtr<CVmConfiguration> c(s.getConfig());
		if (!u.isValid() || !c.isValid())
			break;
		QScopedPointer<DspVm::Start::Process> q(DspVm::Start::Process::yield(ident()));
		if (q.isNull())
		{
			demand_.reject(PRL_ERR_VM_PROCESS_IS_NOT_STARTED);
			break;
		}
		q->setMode(*c);
		//https://bugzilla.sw.ru/show_bug.cgi?id=484118
		d().m_pStartVmPkg = demand_.getRequest();
		// Fix race on vm start
		// https://jira.sw.ru/browse/PDFM-23008
		d().m_pVmRunner = demand_.getActor();
		d().m_bVmIsChild = true;
		d().m_VmProcessId = ::createProcess(q->getBinary(), q->getArguments(),
						q->getWorkingDir(), q->getEnvironment(*u),
						u->getAuthHelper(), c, q->getSocket(),
						q->getMode());
		if (0 == d().m_VmProcessId)
		{
			WRITE_TRACE(DBG_FATAL, "Error: can't create process!");
			demand_.reject(PRL_ERR_VM_PROCESS_IS_NOT_STARTED);
			break;
		}
		else
		{
			QWriteLocker g(&d().m_rwLock);
			d().m_nVmStartTicksCount = PrlGetTickCount64();
			d().m_nVmProcStartTicksCount = PrlGetTickCount64();
			d().m_bFinishedByDispatcher = false;
			DspVm::Start::Monitor::Task<T>::schedule(monitor_);
		}
		// #7499
		storeRunningState(true);
		return true;
	} while(false);
	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE currState = getVmStateUnsync();
	if( VMS_STARTING == currState
		|| VMS_RESUMING == currState
		|| VMS_RESTORING == currState
		|| VMS_SNAPSHOTING == currState
		// #460157 Check to STOPPING to prevent assert when "Stop" was sent immediately after "Start"
		|| VMS_STOPPING == currState
		)
	{
		changeVmState( VMS_STOPPING );
	}
	else
		PRL_ASSERT( VMS_STOPPED == currState );

	return false;
}

bool CDspVm::startVmProcess(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	using namespace DspVm::Start;
	return startVmProcess(Demand(pUser, p), Monitor::Standard(ident()));
}

bool CDspVm::startVmAfterCommitUnfunishedDiskOp(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CVmEvent v;
	using namespace DspVm::Start;
	Demand d(pUser, p);
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &pUser->getAuthHelper() );

		if(VMS_RESUMING != state
			&& checkDiskSpaceToStartVm(v) == PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM
			)
		{
			PRL_ASSERT( v.getEventCode() == PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM );
			d.reject(v);
			return false;
		}
	}
	Monitor::Standard m = Monitor::Standard(ident());
	PRL_RESULT e = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> x = getVmConfig(SmartPtr<CDspClient>(0), e);
	if (PRL_FAILED(e) || !x.isValid())
		return startVmProcess(d, m);
	if (Snapshot::CommitUnfinished::Diagnostic(x).getDevices().isEmpty())
		return startVmProcess(d, m);

	return startVmProcess(d, Monitor::Extended(m));
}

bool CDspVm::startVmAfterCreatingUndoDisks(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CVmEvent outNoSpaceEvent;
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &pUser->getAuthHelper() );

		if( checkDiskSpaceToStartVm( outNoSpaceEvent ) == PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM )
		{
			PRL_ASSERT( outNoSpaceEvent.getEventCode() == PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM );
			pUser->sendResponseError( outNoSpaceEvent, p );
			return false;
		}
	}

	return startVmProcess(pUser, p);
}

bool CDspVm::turnOffSpotlight( const QString& vmDirPath )
{
	Q_UNUSED( vmDirPath );
	return true;
}

bool CDspVm::turnOnSpotlight( const QString& vmDirPath )
{
	Q_UNUSED( vmDirPath );
	return true;
}

void CDspVm::changeUsbState( PRL_EVENT_TYPE nEventType )
{
	switch ( nEventType )
	{
		case PET_DSP_EVT_VM_STARTED:
			checkAutoconnectUsb();
			break;

		case PET_DSP_EVT_VM_STOPPED:
		case PET_DSP_EVT_VM_ABORTED:
		case PET_DSP_EVT_VM_SUSPENDED:
			CDspService::instance()->getHwMonitorThread().processVmStop( ident(),
				CDspService::instance()->getHostInfo() );
			CDspService::instance()->getHwMonitorThread().removeVmOnDeinitialization( ident() );
			break;

		case PET_DSP_EVT_VM_RESETED:
			CDspService::instance()->getHwMonitorThread().removeVmOnDeinitialization( ident() );
			break;

		case PET_DSP_EVT_VM_ABOUT_TO_START_DEINIT:
			CDspService::instance()->getHwMonitorThread().addVmOnDeinitialization( ident() );
			break;

		default:
			break;
	}
}

PRL_UNDO_DISKS_MODE CDspVm::getUndoDisksMode()
{
	if (CDspService::isServerModePSBM())
		return PUD_DISABLE_UNDO_DISKS;

	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), nRetCode);
	if ( pVmConfig && PRL_SUCCEEDED(nRetCode) )
	{
		return pVmConfig->getVmSettings()->getVmRuntimeOptions()->getUndoDisksModeEx();
	}
	return PUD_DISABLE_UNDO_DISKS;
}

bool CDspVm::isUndoDisksMode() const
{
	return d().m_nUndoDisksMode != PUD_DISABLE_UNDO_DISKS;
}

bool CDspVm::isNoUndoDisksQuestion() const
{
	return d().m_bNoUndoDisksQuestion;
}

void CDspVm::disableNoUndoDisksQuestion()
{
	d().m_bNoUndoDisksQuestion = false;
}

bool CDspVm::startUndoDisksRevertOrCommitTask()
{
	PRL_ASSERT(d().m_nUndoDisksMode != PUD_DISABLE_UNDO_DISKS);
	PRL_ASSERT(d().m_pUndoDisksUser.isValid());
	PRL_ASSERT(d().m_pUndoDisksPkg.isValid());

	if (d().m_nUndoDisksMode == PUD_DISABLE_UNDO_DISKS
		|| !d().m_pUndoDisksUser.isValid()
		|| !d().m_pUndoDisksPkg.isValid())
	{
		disableNoUndoDisksQuestion();
		return false;
	}

	// Check on invalid VM
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), nRetCode);
	if (   PRL_ERR_PARSE_VM_CONFIG == nRetCode
		|| ( pVmConfig && PRL_SUCCEEDED(nRetCode) && PRL_FAILED(pVmConfig->getValidRc()) )
		)
	{
		disableNoUndoDisksQuestion();

		PRL_RESULT nRes = isSafeMode() ? PRL_ERR_CANNOT_PROCESSING_SAFE_MODE_FOR_INVALID_VM
									   : PRL_ERR_CANNOT_PROCESSING_UNDO_DISKS_FOR_INVALID_VM;

		CVmEvent event1( PET_DSP_EVT_VM_MESSAGE, getVmUuid(), PIE_DISPATCHER, nRes );
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event1 );
		CDspService::instance()->getClientManager().sendPackageToVmClients( p, getVmDirUuid(), getVmUuid());

		CVmEvent event2( PET_DSP_EVT_VM_STOPPED, getVmUuid(), PIE_DISPATCHER );
		p = DispatcherPackage::createInstance( PVE::DspVmEvent, event2 );

		changeVmState(p);

		return false;
	}

	// Delete shapshot package for undo disks mode
	CProtoCommandPtr pRequest = CProtoSerializer::CreateDeleteSnapshotProtoCommand(
									getVmUuid(),
									QString(UNDO_DISKS_UUID),
									true);
	SmartPtr<IOPackage> pPackage
				= DispatcherPackage::duplicateInstance(d().m_pUndoDisksPkg, pRequest->GetCommand()->toString());
	pPackage->header.type = PVE::DspCmdVmDeleteSnapshot;

	// Start delete snapshot task
	changeVmState( VMS_DELETING_STATE );
	CDspService::instance()->getTaskManager().schedule(new Task_DeleteSnapshot( d().m_pUndoDisksUser, pPackage, VMS_STOPPED ));
	return true;
}

void CDspVm::setSafeMode(bool bSafeMode, SmartPtr<CDspClient> pUserSession)
{
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(pUserSession, nRetCode);
	if ( !pVmConfig || PRL_FAILED(nRetCode) )
	{
		return;
	}

	CVmRunTimeOptions* pRuntime = pVmConfig->getVmSettings()->getVmRuntimeOptions();
	if ( bSafeMode == pRuntime->isSafeMode() )
	{
		return;
	}

	// Switch on/off safe mode
	pRuntime->setSafeMode(bSafeMode);
	d().m_bSafeMode = bSafeMode;

	if (bSafeMode)
	{
		CVmValidateConfig vc(pVmConfig);

		vc.CheckVmConfig(PVC_GENERAL_PARAMETERS, pUserSession);

		if ( vc.HasError(PRL_ERR_VMCONF_NO_HD_IMAGES_IN_SAFE_MODE) )
		{
			CVmEvent event( PET_DSP_EVT_VM_MESSAGE, getVmUuid(), PIE_DISPATCHER,
								PRL_ERR_VMCONF_NO_HD_IMAGES_IN_SAFE_MODE );
			SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
			CDspService::instance()->getClientManager().sendPackageToVmClients( p, getVmDirUuid(), getVmUuid());

			pRuntime->setSafeMode(false);

			return;
		}
	}

	// Save config
	CVmEvent _evt;
	_evt.addEventParameter(new CVmEventParameter(PVE::String,
												 QString::number((int )pRuntime->isSafeMode()),
												 EVT_PARAM_VMCFG_SAFE_MODE ));

	if (CDspService::instance()->getVmDirHelper()
				.atomicEditVmConfigByVm( getVmDirUuid(), getVmUuid(), _evt, pUserSession ))
	{
			CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, getVmUuid(), PIE_DISPATCHER );
			SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
			CDspService::instance()->getClientManager().sendPackageToVmClients( p, getVmDirUuid(), getVmUuid());
	}
}

bool CDspVm::isSafeMode() const
{
	if (CDspService::isServerModePSBM())
		return false;

	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), nRetCode);
	if ( !pVmConfig || PRL_FAILED(nRetCode) )
	{
		return d().m_bSafeMode;
	}
	return pVmConfig->getVmSettings()->getVmRuntimeOptions()->isSafeMode();
}

namespace {
/**
 * Simple helper that let to determine whether opening user session
 * related to special modes or not
 * @param pointer to the command
 */
bool IsSpecialSessionMode( const CProtoCommandPtr &pCmd )
{
	CProtoVmLoginInGuestCommand *pLoginInGuestCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>( pCmd );
	if ( pLoginInGuestCmd )
		return ( PRL_PRIVILEGED_GUEST_OS_SESSION == pLoginInGuestCmd->GetUserLoginName() ||
				PRL_CURRENT_GUEST_OS_SESSION == pLoginInGuestCmd->GetUserLoginName() );

	return (false);
}

}

PRL_RESULT CDspVm::checkUserAccessRightsOnGuestConsoleCmd( SmartPtr<CDspClient> pUser, const CProtoCommandPtr &pCmd )
{
	if ( PVE::DspCmdVmLoginInGuest == pCmd->GetCommandId() )
	{
		//At first collect necessary info about user session access to VM
		bool bIsOwnerOfVm = false, bIsEnoughAccessRights = false, bIsHostAdministrator = false;
		{
			CDspLockedPointer<CVmDirectoryItem>	pVmDirItem =
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid( getVmDirUuid(), getVmUuid() );
			PRL_ASSERT( pVmDirItem.getPtr() );
			bIsOwnerOfVm = CDspService::instance()->getAccessManager().isOwnerOfVm( pUser, pVmDirItem.getPtr() );
			bIsHostAdministrator = pUser->getAuthHelper().isLocalAdministrator();
			PRL_SEC_AM _rights =
				CDspService::instance()->getAccessManager().getAccessRightsToVm( pUser, pVmDirItem.getPtr() ).getVmAccessRights();
			bIsEnoughAccessRights = ( _rights & CDspAccessManager::VmAccessRights::arCanRead ) &&
									( _rights & CDspAccessManager::VmAccessRights::arCanWrite ) &&
									( _rights & CDspAccessManager::VmAccessRights::arCanExecute );
		}

		//Now lets check whether creating session belongs to the special guest OS
		//sessions modes
		if (  IsSpecialSessionMode( pCmd ) && !bIsOwnerOfVm && !bIsHostAdministrator && !bIsEnoughAccessRights )
		{
			WRITE_TRACE(DBG_FATAL, "User '%s' do not have enough rights to create special mode of guest OS session"\
						 " in VM with UUID '%s' name '%s'", QSTR2UTF8( pUser->getAuthHelper().getUserFullName() ),\
						 QSTR2UTF8( getVmUuid() ), QSTR2UTF8( getVmName() ) );
			return ( PRL_ERR_ONLY_ADMIN_OR_VM_OWNER_CAN_OPEN_THIS_SESSION );
		}
		else if (  PVE::DspCmdVmLoginInGuest == pCmd->GetCommandId() && !bIsEnoughAccessRights )
		{
			WRITE_TRACE(DBG_FATAL, "User '%s' do not have enough rights to create custom guest OS session"\
						 " in VM with UUID '%s' name '%s'", QSTR2UTF8( pUser->getAuthHelper().getUserFullName() ),\
						 QSTR2UTF8( getVmUuid() ), QSTR2UTF8( getVmName() ) );
			return ( PRL_ERR_ACCESS_DENIED );
		}
	}
	else
	{
		CProtoBasicVmGuestCommand *pVmGuestCmd =
			CProtoSerializer::CastToProtoCommand<CProtoBasicVmGuestCommand>( pCmd );
		if ( pVmGuestCmd )
		{
			if ( !isGuestOsSessionExists( pUser, pVmGuestCmd->GetVmSessionUuid() ) )
			//Guest session not present for specified user session
				return ( PRL_ERR_VM_GUEST_SESSION_EXPIRED );

			bool bNeedAdminConfirmation =
				( CDspService::instance()->getVmDirManager()
					.getVmDirCatalogue()->getCommonLockedOperations()
						->getLockedOperations().contains(PAR_VM_CHANGE_GUEST_OS_PASSWORD_ACCESS)
				||
				  CDspService::instance()->getVmDirManager()
					.getVmDirItemByUuid(getVmDirUuid(), getVmUuid())->getLockedOperationsList()
						->getLockedOperations().contains(PAR_VM_CHANGE_GUEST_OS_PASSWORD_ACCESS)
				);

			if (   PVE::DspCmdVmGuestSetUserPasswd == pCmd->GetCommandId()
				&& ! pUser->isAdminAuthWasPassed()
				&& bNeedAdminConfirmation
				 )
				return PRL_ERR_ADMIN_CONFIRMATION_IS_REQUIRED_FOR_VM_OPERATION;
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Wrong command received for guest OS functionality: [%s]",\
				QSTR2UTF8( pCmd->GetCommand()->toString() ) );
			return (PRL_ERR_UNRECOGNIZED_REQUEST);
		}
	}

	return ( PRL_ERR_SUCCESS );
}

void CDspVm::processGuestOsSessionCmd(SmartPtr<CDspClient> pUser,
									const CProtoCommandPtr &pCmd, const SmartPtr<IOPackage> &p)
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( pCmd.getImpl() );
	PRL_ASSERT( p.getImpl() );

	PRL_RESULT rc = checkUserAccessRightsOnGuestConsoleCmd( pUser, pCmd );
	if ( PRL_FAILED( rc ) )
	{
		pUser->sendSimpleResponse( p, rc );
		return;
	}

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
		// case VMS_STOPPED     : ;
		// case VMS_SUSPENDED   : ;
		// case VMS_SUSPENDING_SYNC   : ;
		// case VMS_STARTING    : ;
		// case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
		case VMS_RUNNING     : ;
			if( vpsPausedByVmFrozen == d().m_nVmPowerState )
			{
				PRL_ASSERT( ! CDspService::isServerMode() );

				SEND_ERROR_BY_VM_FROZEN_STATE( p );
				return;
			}

		// Accepting guest OSes commands just in running VM state
			_wLock.unlock();
			if ( PVE::DspCmdVmLoginInGuest == pCmd->GetCommandId() )
				registerNewGuestSession( pUser, p );
			else if ( PVE::DspCmdVmGuestLogout == pCmd->GetCommandId() )
			{
				CProtoBasicVmGuestCommand *pVmGuestCmd =
					CProtoSerializer::CastToProtoCommand<CProtoBasicVmGuestCommand>( pCmd );
				PRL_ASSERT( pVmGuestCmd );
				unregisterGuestSession( pUser, pVmGuestCmd->GetVmSessionUuid() );
			}
			sendPackageToVm( p );
		break;
		// case VMS_PAUSED      : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_CONTINUING	 : ;
		// case VMS_PAUSING	 : ;
		// case VMS_MIGRATING: ;
		// case VMS_DELETING_STATE: ;
	default:
		{
			_wLock.unlock();
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
		}//default
	}//switch
}

PRL_RESULT CDspVm::checkDiskSpaceToStartVm( CVmEvent& outNoSpaceErrorParams )
{
	PRL_RESULT outErr = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), outErr);

	if( !pVmConfig )
	{
		WRITE_TRACE(DBG_FATAL, "Unable to load config for vm %s (%s) by error %s"
			, QSTR2UTF8( getVmName() )
			, QSTR2UTF8( getVmUuid() )
			, PRL_RESULT_TO_STRING( outErr )
			);
		return outErr;
	}

	quint64
		nRequiredSpace = (quint64)(
				pVmConfig->getVmHardwareList()->getMemory()->getRamSize()
				+ pVmConfig->getVmHardwareList()->getVideo()->getMemorySize()
				+ (SIZE_OF_RESERVED_HDD_FREE_SPACE_TO_VM_START>>20)
				) << 20 ;

	quint64 nFreeSpace = 0;
	QString strDir;

	{
		CDspLockedPointer< CVmDirectoryItem >
			pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getVmDirUuid(), getVmUuid() );
		if( !pVmDirItem )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get dirItem for vm %s", QSTR2UTF8( getVmUuid() ) );
			return PRL_ERR_FAILURE;
		}

		// get vm dir path
		strDir = QFileInfo(pVmDirItem->getVmHome() ).path();
		if ( CFileHelper::isRemotePath( strDir ) )
		{
			WRITE_TRACE(DBG_FATAL, "VM '%s' created on network share so memory file will be create on local FS", QSTR2UTF8(strDir));
			return PRL_ERR_SUCCESS;
		}
	}


	// Try for several time to hold EGAIN-result
	for( UINT uTryAgainLoops = 0; uTryAgainLoops < 5; uTryAgainLoops++ )
	{
		outErr = CFileHelper::GetDiskAvailableSpace( strDir, &nFreeSpace);
		if( outErr != PRL_ERR_TRY_AGAIN )
			break;

		WRITE_TRACE(DBG_FATAL, "GetDiskAvailableSpace wants to try again" );
		HostUtils::Sleep(300);
	}

	if( PRL_FAILED(outErr) )
	{
		WRITE_TRACE(DBG_FATAL, "Unable to GetDiskAvailableSpace()" );
		return outErr;
	}

	if( nFreeSpace >= nRequiredSpace )
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Not enough disk space to start VM %s: required size=%llu, available size=%llu"
		, QSTR2UTF8( getVmUuid() )
		, nRequiredSpace
		, nFreeSpace);

	outErr = PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM;
	CVmEvent& evt = outNoSpaceErrorParams;
	evt.setEventCode( outErr );
	evt.addEventParameter( new CVmEventParameter (
		PVE::String,
		getVmName(),
		EVT_PARAM_MESSAGE_PARAM_0 ) );


	// Round up by 1Mb
	// Example: 0.2Mb -> 1Mb, 0.6Mb -> 1Mb, 1.6Mb -> 2Mb
	evt.addEventParameter( new CVmEventParameter (
		PVE::String,
		QString("%1").arg( (nRequiredSpace - nFreeSpace + (1 << 20) - 1) >> 20 ),
		EVT_PARAM_MESSAGE_PARAM_1 ) );

	return outErr;
}

void CDspVm::registerNewGuestSession( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p )
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );
	QMutexLocker _lock( &d().m_GuestSessionsMtx );
	PRL_ASSERT( !d().m_GuestSessions[ pUser->getClientHandle() ].contains( Uuid::toString( p->header.uuid ) ) );
	d().m_GuestSessions[ pUser->getClientHandle() ].insert( Uuid::toString( p->header.uuid ) );
}

void CDspVm::checkWhetherResponseOnOpenGuestOsSessionCmd( const CProtoCommandPtr &pCmd, const SmartPtr<IOPackage> &p )
{
	QString sParentPackageId = Uuid::toString( p->header.parentUuid );
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	PRL_ASSERT( pResponseCmd );
	quint32 nInstances = 0;
	QMutexLocker _lock( &d().m_GuestSessionsMtx );
	QMap<IOSender::Handle, QSet<QString> >::iterator _user = d().m_GuestSessions.begin();
	for ( ; _user != d().m_GuestSessions.end(); ++_user )
	{
		if ( _user.value().contains( sParentPackageId ) )
		{
			nInstances++;
			_user.value().remove( sParentPackageId );
			if ( PRL_ERR_SUCCESS == pResponseCmd->GetRetCode() )
			{
				PRL_ASSERT( pResponseCmd->GetStandardParamsCount() == 1 );
				QString sGuestSessionId = pResponseCmd->GetStandardParam( 0 );
				PRL_ASSERT( !_user.value().contains( sGuestSessionId ) );
				_user.value().insert( sGuestSessionId );
			}
		}
	}
	PRL_ASSERT( nInstances <= 1 );
}

void CDspVm::globalCleanupGuestOsSessions( const IOSender::Handle &h )
{
	QReadLocker _lock(g_pVmsMapLock);
	DspVm::Storage::snapshot_type s = g_pStorage->snapshot();
	_lock.unlock();
	foreach (SmartPtr<CDspVm> pVm, s)
		pVm->cleanupGuestOsSessions( h );
}

void CDspVm::cleanupGuestOsSessions( const IOSender::Handle &h )
{
	QMutexLocker _lock(&d().m_GuestSessionsMtx);
	QMap<IOSender::Handle, QSet<QString> >::iterator _user_sessions = d().m_GuestSessions.find( h );
	if ( _user_sessions != d().m_GuestSessions.end() )
	{
		foreach( QString sVmSessionUuid, _user_sessions.value() )
		{
			CProtoCommandPtr pCmd =
				CProtoSerializer::CreateBasicVmGuestProtoCommand( PVE::DspCmdVmGuestLogout,
					getVmUuid(), sVmSessionUuid, 0 );
			SmartPtr<IOPackage> p =
				DispatcherPackage::createInstance( PVE::DspCmdVmGuestLogout, pCmd->GetCommand()->toString() );
			sendPackageToVm( p );
		}
		d().m_GuestSessions.erase( _user_sessions );
	}
}

void CDspVm::processResponseForGuestOsSessionsCmds( const SmartPtr<IOPackage> &p )
{
	PRL_ASSERT( p.getImpl() );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( p );
	PRL_ASSERT( PVE::DspWsResponse == pCmd->GetCommandId() );
	checkWhetherResponseOnOpenGuestOsSessionCmd( pCmd, p );
}

bool CDspVm::isGuestOsSessionExists( const SmartPtr<CDspClient> &pUser, const QString &sVmSessionUuid )
{
	PRL_ASSERT( pUser.getImpl() );
	QMutexLocker _lock(&d().m_GuestSessionsMtx);
	QMap<IOSender::Handle, QSet<QString> >::const_iterator _user_sessions =
		d().m_GuestSessions.find( pUser->getClientHandle() );
	if ( _user_sessions != d().m_GuestSessions.end() )
		return ( _user_sessions.value().contains( sVmSessionUuid ) );

	return ( false );
}

void CDspVm::unregisterGuestSession( const SmartPtr<CDspClient> &pUser, const QString &sVmSessionUuid )
{
	PRL_ASSERT( pUser.getImpl() );
	QMutexLocker _lock(&d().m_GuestSessionsMtx);
	QMap<IOSender::Handle, QSet<QString> >::iterator _user_sessions =
			d().m_GuestSessions.find( pUser->getClientHandle() );
	if ( _user_sessions != d().m_GuestSessions.end() )
		_user_sessions.value().remove( sVmSessionUuid );
}

bool CDspVm::cancelSuspend( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p )
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );

	PRL_RESULT ret = checkUserAccessRightsAndSendResponseOnError( pUser, p, PVE::DspCmdVmSuspendCancel );
	if( PRL_FAILED( ret ) )
		return false;

	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();
	switch( state )
	{
		// case VMS_STOPPED     : ;
		// case VMS_SUSPENDED   : ;
		case VMS_SUSPENDING_SYNC   : ;
			_wLock.unlock();
			sendPackageToVm( p );
			return true;
		break;
		// case VMS_STARTING    : ;
		// case VMS_RESUMING: ;
		// case VMS_RESTORING   : ;
		// case VMS_RUNNING     : ;
		// case VMS_PAUSED      : ;
		// case VMS_SUSPENDING  : ;
		// case VMS_STOPPING    : ;
		// case VMS_COMPACTING  : ;
		// case VMS_SNAPSHOTING : ;
		// case VMS_RESETTING	 : ;
		// case VMS_CONTINUING	 : ;
		// case VMS_PAUSING	 : ;
		//case VMS_MIGRATING: ;
		// case VMS_DELETING_STATE: ;
	default:
		{
			_wLock.unlock();
			SEND_ERROR_BY_CANT_EXECUTED_VM_COMMAND( p, state );
		}//default
	}//switch
	return false;
}

/**
* fix vm configuration parameters before start vm
*/
void CDspVm::fixConfigBeforeStartVm( const CHostHardwareInfo & hostInfo, SmartPtr<CVmConfiguration> pConfig )
{
	return fixConfigBeforeStartVm( hostInfo, pConfig, getVmRunner() );
}

void CDspVm::fixConfigBeforeStartVm( const CHostHardwareInfo & hostInfo, SmartPtr<CVmConfiguration> pConfig
	, SmartPtr<CDspClient> pUser )
{
	CVmHardware * pHardware = pConfig->getVmHardwareList();
	QString strDevId, strDevName;

	PRL_ASSERT( pUser );
	CDspService::instance()->getVmDirHelper().UpdateHardDiskInformation( pHardware->m_lstHardDisks, pUser );

	bool bForceDisconnect = false;

	if( hostInfo.m_lstOpticalDisks.size() )
	{
		strDevId =
			hostInfo.m_lstOpticalDisks[0]->getDeviceId();

		strDevName =
			hostInfo.m_lstOpticalDisks[0]->getDeviceName();
	}
	else
	{
		bForceDisconnect = true;
	}

	foreach( CVmOpticalDisk * pDisk, pHardware->m_lstOpticalDisks )
	{
		if ( (pDisk->getEmulatedType() == PVE::RealCdRom) &&
			( pDisk->getUserFriendlyName() == PRL_DVD_DEFAULT_DEVICE_NAME ) )
		{
			pDisk->setSystemName( strDevId );
			if (bForceDisconnect)
			{
				pDisk->setConnected(PVE::DeviceDisconnected);
			}
		}
	}

	if (CDspService::isServerModePSBM())
	{
		foreach( CVmHardDisk* pDisk, pHardware->m_lstHardDisks )
		{
			if ( pDisk->getEmulatedType() == PVE::BootCampHardDisk )
				pDisk->setConnected(PVE::DeviceDisconnected);
		}

		QList<CVmDevice*> lstPciDevices = *((QList<CVmDevice*>* )&pHardware->m_lstGenericPciDevices);
		lstPciDevices += *((QList<CVmDevice*>* )&pHardware->m_lstNetworkAdapters);
		lstPciDevices += *((QList<CVmDevice*>* )&pHardware->m_lstPciVideoAdapters);

		for(int i = 0; i < lstPciDevices.size(); i++)
		{
			CVmDevice* pPciDev = lstPciDevices[i];
			if ( pPciDev->getEnabled() != PVE::DeviceEnabled )
				continue;

			if (   pPciDev->getDeviceType() != PDE_GENERIC_NETWORK_ADAPTER
				||
				   (pPciDev->getDeviceType() == PDE_GENERIC_NETWORK_ADAPTER
					&& pPciDev->getEmulatedType() == PDT_USE_DIRECT_ASSIGN)
				)
			{
				pPciDev->setEnabled(PVE::DeviceDisabled);
			}
		}
	}
}

/**
* check DVD device command to default dvd rom present and
* repack package with patched parameters.
* @param CProtoCommandPtr * pDeviceCmd - device command object
* @return - fixed package to post
*/
SmartPtr<IOPackage> CDspVm::getConnectionFixedPackageForDvd( const CHostHardwareInfo & hostInfo,
															const SmartPtr<IOPackage> &p,
															CProtoVmDeviceCommand * pDeviceCmd,
															PVE::IDispatcherCommands cmd )
{
	// change default DVD to first physical
	CVmOpticalDisk vmDvdDevice;

	SmartPtr<IOPackage> pFixedPackage;
	// Deserialize device configuration object
	vmDvdDevice.fromString( pDeviceCmd->GetDeviceConfig() );
	if ( vmDvdDevice.getUserFriendlyName() == PRL_DVD_DEFAULT_DEVICE_NAME )
	{
		if( hostInfo.m_lstOpticalDisks.size() )
		{
			QString strDevId =
				hostInfo.m_lstOpticalDisks[0]->getDeviceId();

			vmDvdDevice.setSystemName( strDevId );

			QString strDev = vmDvdDevice.toString();
			CProtoCommandPtr pRequest
				= CProtoSerializer::CreateVmDeviceProtoCommand(cmd,
				pDeviceCmd->GetVmUuid(),
				pDeviceCmd->GetDeviceType(),
				pDeviceCmd->GetDeviceIndex(),
				strDev
				);

			pFixedPackage
				= DispatcherPackage::duplicateInstance( p, pRequest->GetCommand()->toString());
			pFixedPackage->header.type = pRequest->GetCommandId();
			return pFixedPackage;
		}
	}
	return SmartPtr<IOPackage>();
}

bool CDspVm::hasUnansweredRequestForSession( const QString& qsSessionId ) const
{
	QReadLocker lock( &d().m_rwLock);

	DspVm::Details::RequestResponseHash::const_iterator it;
	for(it = d().m_hashRequestResponse.begin(); it != d().m_hashRequestResponse.end(); ++it)
	{
		RequestInfo reqInfo = it.value();
		PRL_ASSERT(reqInfo.pkgRequest.isValid());
		if (   qsSessionId == Uuid::toString(reqInfo.pkgRequest->header.senderUuid)
			&& ! reqInfo.pkgResponse.isValid())
			return true;
	}

	return false;
}

void CDspVm::changeLogLevel( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p )
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );

	CHECK_USER_ACCESS_RIGHTS( pUser, PVE::DspCmdVmChangeLogLevel, p )

	if ( handshakeWasCalled() )
		sendPackageToVm( p );
}

SmartPtr<CVmConfiguration> CDspVm::getVmConfig( SmartPtr<CDspClient> pUser, PRL_RESULT &nOutError) const
{
	if ( !pUser.isValid() )
	{
		if ( getVmRunner().isValid() )
		{
			return CDspService::instance()->getVmDirHelper().
						getVmConfigByUuid( getVmRunner(), getVmUuid(), nOutError );
		}
		else
		{
			return CDspService::instance()->getVmDirHelper().
						getVmConfigByUuid( getVmDirUuid(), getVmUuid(), nOutError );
		}
	}

	return CDspService::instance()->getVmDirHelper().
				getVmConfigByUuid( pUser, getVmUuid(), nOutError );
}

void CDspVm::storeRunningState(bool bRunning)
{
	if (d().m_bStoreRunningStateDisabled)
		return;

	// Set running state in the VmDirectory
	QString sVmHome;
	{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem =
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid( getVmDirUuid(), getVmUuid() );
		if ( !pVmDirItem )
			return;
		pVmDirItem->setLastRunningState(bRunning);
		CDspService::instance()->getVmDirManager().updateVmDirItem( pVmDirItem );
		sVmHome = pVmDirItem->getVmHome();
	}

	if (CDspService::isServerMode())
	{
		// Set running state in the Vm config
		PRL_RESULT err;

		SmartPtr<CVmConfiguration> pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
				getVmDirUuid(),
				getVmUuid(),
				err, true);
		if (PRL_FAILED(err))
			return;

		if (pVmConfig->getVmSettings()->getClusterOptions()->isRunning() == bRunning)
			return;

		pVmConfig->getVmSettings()->getClusterOptions()->setRunning(bRunning);

		SmartPtr<CDspClient> pUser = getVmRunner();
		if (!pUser.isValid())
			pUser = CDspClient::makeServiceUser();
		err = CDspService::instance()->getVmConfigManager().saveConfig(
				pVmConfig,
				sVmHome,
				pUser,
				true,
				true);
	}
	return;
}

void CDspVm::storeFastRebootState(bool bFastReboot,
			SmartPtr<CDspClient> pUserSession /*=SmartPtr<CDspClient>()*/)
{
	if (!pUserSession.isValid())
		pUserSession = getVmRunner();

	QString user;
	if (bFastReboot)
		user = pUserSession->getAuthHelper().getUserName();

	WRITE_TRACE(DBG_DEBUG, "Updating VM '%s' fast reboot %u, user '%s'",
			QSTR2UTF8( getVmName() ), bFastReboot, QSTR2UTF8( user ));

	// Save config
	CVmEvent _evt;
	_evt.addEventParameter(new CVmEventParameter(PVE::String,
		QString::number(bFastReboot),
		EVT_PARAM_VM_FAST_REBOOT ) );
	_evt.addEventParameter(new CVmEventParameter(PVE::String,
		QString(),
		EVT_PARAM_VM_FAST_REBOOT_USER ) );

	if (!CDspService::instance()->getVmDirHelper()
		.atomicEditVmConfigByVm( getVmDirUuid(), getVmUuid(), _evt, pUserSession ) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to set fast reboot of VM %s", QSTR2UTF8( getVmName() ) );
		return;
	}

	CDspVmDirHelper::saveFastRebootData( getVmUuid(), user );

	CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, getVmUuid(), PIE_DISPATCHER );
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
	CDspService::instance()->getClientManager().sendPackageToVmClients( p, getVmDirUuid(), getVmUuid());
}

void CDspVm::restoreVmProcess(VIRTUAL_MACHINE_STATE state,
							  Q_PID VmProcessId,
							  SmartPtr<CDspClient> pUser)
{
	PVE::IDispatcherCommands cmd = PVE::DspCmdVmStart;
	if (state == VMS_MIGRATING)
		cmd = PVE::DspCmdDirVmMigrate;
	if (state == VMS_COMPACTING)
		cmd = PVE::DspCmdVmStartEx;

	d().m_nInitDispatcherCommand = cmd;

	CProtoCommandPtr pStartRequest = CProtoSerializer::CreateProtoBasicVmCommand( cmd, getVmUuid() );
	d().m_pStartVmPkg = DispatcherPackage::createInstance(
		PVE::DspVmRestoreState, pStartRequest->GetCommand()->toString() );

	if (pUser.isValid())
		d().m_pVmRunner = pUser;

	if (state == VMS_RECONNECTING)
	{
		changeVmState(state);
		return;
	}

	d().m_VmProcessId = VmProcessId;
	d().m_bVmIsChild = false;

	QWriteLocker _lock(&d().m_rwLock);
	d().m_nVmStartTicksCount = PrlGetTickCount64();
	d().m_nVmProcStartTicksCount = PrlGetTickCount64();
	_lock.unlock();//To prevent deadlocks with further actions

	storeRunningState(true);

	changeVmState(state);
}

void CDspVm::wakeupApplyConfigWaiters()
{
	QMutexLocker lock( &d().m_applyConfigMutex);
	d().m_applyConfigCondition.wakeAll();
}

bool CDspVm::waitForApplyConfig(unsigned long timeout)
{
	QMutexLocker lock( &d().m_applyConfigMutex);
	return d().m_applyConfigCondition.wait( &d().m_applyConfigMutex, timeout );
}

PRL_RESULT CDspVm::runActionScript(PRL_VM_ACTION nAction, const SmartPtr<CDspVm> &pVm, bool bWaitForResult)
{
	const SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspCmdCtlDispatherFakeCommand );

	SmartPtr<CDspClient> pUser = getVmRunner();
	if (!pUser.isValid())
		pUser = SmartPtr<CDspClient>(new CDspClient(IOSender::Handle()));

	PRL_RESULT ret;
	SmartPtr<CDspClient> pRunUser = pUser;
	SmartPtr<CVmConfiguration> vmConfig = getVmConfig(SmartPtr<CDspClient>(0), ret);
	if (PRL_FAILED(ret) || !vmConfig.isValid())
	{
		WRITE_TRACE( DBG_FATAL, "Invalid VM config" );
		return PRL_ERR_INCONSISTENCY_VM_CONFIG;
	}

	ret = CDspService::instance()->getVmDirHelper().getVmStartUser(vmConfig, pRunUser);
	if (PRL_FAILED(ret))
		return ret;

	QString sUserName = pRunUser->getAuthHelper().getUserName();
	CDspService::instance()->getTaskManager().schedule(new Task_RunVmAction(pUser, p, pVm, nAction, sUserName))
		.wait(bWaitForResult);
	return 0;
}

QString CDspVm::prepareFastReboot(bool suspend)
{
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = getVmConfig(SmartPtr<CDspClient>(0), nRetCode);
	if ( !pVmConfig || PRL_FAILED(nRetCode) )
		return QString();

	PRL_ASSERT( ! d().m_pSuspendMounter );

	d().m_pSuspendMounter.reset();
	CDspVmManager *pVmManager = &CDspService::instance()->getVmManager();
	if (suspend)
		d().m_pSuspendMounter = pVmManager->getSuspendHelper()->prepareVmFastReboot(pVmConfig);
	else //resume
	{
		d().m_pSuspendMounter = pVmManager->getSuspendHelper()->fastRebootMount(false);
		//set fake path
		if (d().m_pSuspendMounter)
			d().m_pSuspendMounter->setPath("mounted");
	}

	if (d().m_pSuspendMounter)
		return d().m_pSuspendMounter->getPath();

	return QString();
}

PRL_RESULT CDspVm::changeVmStateToCompacting()
{
	QWriteLocker _wLock( &d().m_rwLock );
	VIRTUAL_MACHINE_STATE state = getVmStateUnsync();

	switch(state)
	{
	case VMS_STOPPED:
		changeVmState(VMS_COMPACTING);
		return PRL_ERR_SUCCESS;
	default:
		;
	}

	return PRL_ERR_COMPACT_WRONG_VM_STATE;
}
