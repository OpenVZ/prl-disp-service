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
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include "Build/Current.ver"

#include "CDspVmStateSender.h"

#include "Tasks/Task_BackgroundJob.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_EditVm.h"
#include "Tasks/Task_CloneVm.h"
#include "Tasks/Task_MoveVm.h"

#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>

#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include "CVmValidateConfig.h"

#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/Common.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"

//#include "Vm/CVmMigrateFilesCopyingStubs.h"

#include <prlcommon/PrlCommonUtilsBase/CommandLine.h>
#include <prlcommon/PrlCommonUtilsBase/OsInfo.h>
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>

#include <Libraries/PowerWatcher/PowerWatcher.h>

#ifdef _WIN_
	#include <process.h>
	#define getpid _getpid
#endif

#include <sys/wait.h>
#include "Libraries/CpuFeatures/CCpuHelper.h"

#include "CDspVzHelper.h"
#include "CDspLibvirtExec.h"

using namespace DspVm;
using namespace Parallels;

#include <prlcommon/Logging/Logging.h>

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

	changeUsbState((PRL_EVENT_TYPE)PET_DSP_EVT_VM_STOPPED);

	// Send to all VM client new VM state
	// #464950 Should be last call in method to prevent races
	if ( CDspVm::suspendedFilesPresent( getVmUuid(), getVmDirUuid() ) )
		changeVmState(VMS_SUSPENDED);
	else
		changeVmState(VMS_STOPPED);

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

const CVmIdent& CDspVm::ident() const
{
	return d().m_VmIdent;
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

void CDspVm::changeVmState(const SmartPtr<IOPackage> &p, bool&)
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
					changeVmState(VMS_RUNNING);
					break;
				}
				default:
					applyVMNetworkSettings(VMS_RUNNING);
					changeVmState(VMS_RUNNING);
				}
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
	CDspService::instance()->getVmDirHelper().UpdateHardDiskInformation(pConfig);

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
