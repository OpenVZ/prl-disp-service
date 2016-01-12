///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVm.h
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

#ifndef __CDSPVM_H__
#define __CDSPVM_H__

#include <QThread>
#include <QString>
#include <QReadWriteLock>
#include <QProcess>
#include "CDspClient.h"
#include "CDspStarter.h"
#include "CDspVNCStarter.h"
#include <prlcommon/Std/SmartPtr.h>
#include "CDspVmDirManager.h"
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "CDspDispConnection.h"
#include "CDspVmSuspendHelper.h"
#include <prlxmlmodel/DispConfig/CDispCpuPreferences.h>

#include "Libraries/PerfCount/PerfLib/PerfCounter.h"

using namespace IOService;

/** Header classes predefines */
class CDspVmBaseHelper;
class CDspVmProcessMonitor;
class CDspVm;
class CancelOperationSupport;
class CDspTaskHelper;
class CHostHardwareInfo;



/**
 * Base class for all VM process helpers
 */
class CDspVmBaseHelper : public QThread
{
public:
	/**
	 * Constructor - cleanups already completed tasks and registries helper at common list
	 * @param VM UUID
	 * @param parent VM dir UUID
	 */
	CDspVmBaseHelper( const QString &sVmUuid, const QString &sVmDirUuid );
	/**
	 * Virtual destructor for properly child objects destruction
	 */
	virtual ~CDspVmBaseHelper() {}

private:
	/** Pointer to monitoring objects set access synchronization object */
	static QMutex *g_pVmHelpersListMutex;
	/** Pointer to monitoring objects set object */
	static QList<SmartPtr<CDspVmBaseHelper> > *g_pVmHelpersList;

private:
	/**
	 * Cleanups already completed tasks objects
	 */
	static void cleanupObjects();

protected:
	/**
	 * Processing VM UUID
	 */
	QString m_sVmUuid;
	/**
	 * Processing VM directory UUID
	 */
	QString m_sVmDirUuid;
};

namespace DspVm
{
struct Details;
struct Storage;
namespace Start
{
struct Demand;
namespace Monitor
{
struct Routine;
} // namespace Monitor
} // namespace Start
} // namespace DspVm

/**
 * Class that wrapping Vm possible actions (start, stop and etc.)
 */
class CDspVm
{
	friend struct DspVm::Start::Monitor::Routine;
public:
	enum VmPowerState { vpsNormal=0, vpsPausedByHostSleep=1, vpsPausedByVmFrozen=2 };
	static const char* VmPowerStateToString( VmPowerState st );

public:
	explicit CDspVm(const SmartPtr<DspVm::Details>& details_);

	/** Vm destructor */
	~CDspVm ();

	/**
	 * Returns VM UUID
	 */
	QString getVmUuid() const { return ident().first ; }

	/**
	 * Returns parent VM directory UUID
	 */
	QString getVmDirUuid() const { return ident().second ; }

	CVmIdent getVmIdent() const
	{
		return ident();
	}

	/**
	* Returns VM NAME on starting moment
	*/
	QString getVmName() const;


	/**
	 * Returns VM process identifier structure
	 */
	Q_PID getVmProcessId() const;

	/**
	 * Returns VM process identifier as simple uint process identifier
	 */
	quint32 getVmProcessIdAsUint() const;

	/**
	 * Returns VM uptime start system ticks count
	 */
	PRL_UINT64 getVmUptimeTimestamp() const;

	/**
	 * Returns VM process start system ticks count
	 */
	PRL_UINT64 getStartTimestamp() const;

	/**
	 * Returns VM total uptime in seconds
	 */
	PRL_UINT64 getVmUptimeInSecs() const;

	/**
	 * Returns VM process from the last start uptime in seconds
	 */
	PRL_UINT64 getVmProcessUptimeInSecs() const;

	/**
	 * Handshakes with newly connected VM process
	 * @param VM connection object handle
	 */
	void handshakeWithVmProcess(const IOSender::Handle &h);

    /**
     * Processes command of start VM
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	bool start( SmartPtr<CDspClient> pUser,
				const SmartPtr<IOPackage> &p,
				PRL_VM_START_MODE nStartMode = PSM_VM_START);

	/**
     * Processes command of restart VM guest OS
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	void restartGuest(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

    /**
     * Processes command of stop VM
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
	 * @param isAcpi indicates if this is a shut down operation with ACPI or general stop command
	 * @param actionByDispatcher indicates if action caused by user
     */
	void stop(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
             PRL_UINT32 nStopMode, bool actionByDispatcher = false) ;

    /**
     * Processes command of start VNC server
	 * @param pointer to the user session object (can be NULL in case when
	 *		request from VM was received)
	 * @param pointer to request package object
	 * @param sign whether method performs by VM process
	 * @param sign whether method called from native transport thread
     */
	void startVNCServer (SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p, bool bIsRequestFromVm, bool bIsCallFromNativeThread);

    /**
     * Processes command of stop VNC server
	 * @param pointer to the user session object (can be NULL in case when
	 *		request from VM was received)
	 * @param pointer to request package object
	 * @param sign whether method performs by VM process
	 * @param sign whether method called from native transport thread
     */
    void stopVNCServer (SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p, bool bIsRequestFromVm, bool bIsCallFromNativeThread);

	/*
	 * Cancel stop of VNC server command
	 */
	void cancelStopVNCServerOp();

    /**
     * Processes command of get VM problem report
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	PRL_RESULT sendProblemReport(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

    /**
     * Processes command of reset VM
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	void reset(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

    /**
     * Processes command of pause VM
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	void pause(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/* Suspend failure flags	*/
	typedef enum SuspendMode {
		SM_STOP_ON_FAILURE	= 0,
		SM_SUSPEND_ON_FAILURE	= 1,
	} SuspendMode;

    /**
     * Processes command of suspend VM
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
	 * @param actionByDispatcher indicates if action caused by user
	 * @param allowSendQuestion indicates that suspend calls not from main user thread
	 *		( and allow to send question without hangs main user thread )
	 * @param nMode
     */
	void suspend(SmartPtr<CDspClient> pUser
			,const SmartPtr<IOPackage> &p
			,bool actionByDispatcher = false
			,SuspendMode nMode = SM_STOP_ON_FAILURE);
	/**
	* Processes command of create snapshot
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	* @param if it is true we wait finishing thread and return task result
	* @param task result
	*/
	bool createSnapshot(
		SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
		CVmEvent* evt = NULL, bool bWaitResult = false);

	/**
	* Processes command of switch to snapshot
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	bool switchToSnapshot(
		SmartPtr<CDspClient> pUser,
		const SmartPtr<IOPackage> &p,
		CVmEvent* evt = NULL,
		bool bWaitResult = false);

	/**
	* Processes command of delete snapshot
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	* @param if it is true we wait finishing thread and return task result
	* @param task result
	*/
	bool deleteSnapshot(
		SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
		CVmEvent* evt = NULL, bool bWaitResult = false);

	/**
	* Processes command of start snapshoted vm
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	bool startSnapshotedVm(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of start migrated vm
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	bool startMigratedVm(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of Install Utility
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void installUtility(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of Install Tools
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void installTools(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of Update Tools Section
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void updateToolsSection(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of Run Compressor
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void runCompressor(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of Cancel Compressor
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void cancelCompressor(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Initiates sending notifications about devices states by VM
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void InitiateDevStateNotifications(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

   /**
     * Processes command of connect VM device
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	void connectDevice(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

    /**
     * Processes command of disconnect VM device
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	void disconnectDevice(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

    /**
     * Processes command of send answer to VM
     * @param pointer to the user session object that initialized request
	 * @param pointer to request package object
     */
	void sendAnswerToVm(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Processes command of run internal VM command
	* @param pointer to the user session object that initialized request
	* @param pointer to request package object
	*/
	void InternalCmd(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	 * Sends specified package to VM
	 * @param pointer to the sending to VM package
	 * @return handle to performed job
	 */
	IOSendJob::Handle sendPackageToVm(const SmartPtr<IOPackage> &p);

	/**
	* Sends specified package to VM  and provide rollback mech to state machine
	* @param pointer to the sending to VM package
	* @param vmState to rollback when operation fails.
	* @param bEnableDeferredResponse - to defer send response to client till destructor called
	*	( don't route response package immediately after recieve)
	* @param bFinishedByDispatcher - means that it is dispatcher shutdown
	*	and we don't need to change running state.
	* @return handle to performed job
	* @ note SHOULD BE USED for command, that calls changeVmState() before sends package to VM.
	*/
	IOSendJob::Handle sendPackageToVmEx(const SmartPtr<IOPackage> &p
		, VIRTUAL_MACHINE_STATE nPrevVmState
		, bool bEnableDeferredResponse = false
		, bool bFinishedByDispatcher = false );

	/**
	 * Changes VM internal state by processing specified VM event package
	 * @param pointer to the processing package
	 */
	void changeVmState(const SmartPtr<IOPackage> &p);
	void changeVmState(const SmartPtr<IOPackage> &p, bool& outNeedRoute);

	/**
	 * Returns VM state
	 */
	VIRTUAL_MACHINE_STATE getVmState() const;

	/**
	 * Returns pointer to start VM request operation package
	 */
	SmartPtr<IOPackage> getStartVmRequestPackage() const;
	/**
	 * Returns job result object after starting VM process
	 */
	IOSendJob::Handle getStartVmJob() const;
	/**
	 * Set pointer to migrate VM request operation package
	 */
	void setMigrateVmRequestPackage(const SmartPtr<IOPackage> &p);
	/**
	 * Returns pointer to migrate VM request operation package
	 */
	SmartPtr<IOPackage> getMigrateVmRequestPackage() const;
	/**
	 * Set pointer to migrate VM dispatcher connection
	 */
	void setMigrateVmConnection(const SmartPtr<CDspDispConnection> &pDispConnection);
	/**
	 * Returns handle to requester start VM operation
	 * NOTE: may be NULL
	 */
	SmartPtr<CDspClient> getVmRunner() const;

	/**
	 * Save question packet
	 */
	void setQuestionPacket(const SmartPtr<IOPackage>& p);
	/**
	 * Return question packet
	 */
	const SmartPtr<IOPackage>& getQuestionPacket() const;

	/** VM connection object handle */
	IOSender::Handle getVmConnectionHandle() const;

	/** return true when handshake was called ( without any result of handshake ) */
	bool handshakeWasCalled() const;

	CDspLockedPointer<ProcPerfStoragesContainer> PerfStoragesContainer() ;
	/**
	 * Processes command related to the guest OS sessions functionality.
	 *
	 */
	void processGuestOsSessionCmd(SmartPtr<CDspClient> pUser,
									const CProtoCommandPtr &pCmd, const SmartPtr<IOPackage> &p);
	/**
     * Lets to cancel VM suspending sync phase
	 * see https://bugzilla.sw.ru/show_bug.cgi?id=441619 for more details
     * @param pointer to the user session object that initialized request
     * @param pointer to request package object
	 * @return sign whether operation was performed successfully
    */
	bool cancelSuspend( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p );

	/**
     * Lets to change VM process log level in runtime
	 * see https://bugzilla.sw.ru/show_bug.cgi?id=439457 for more details
     * @param pointer to the user session object that initialized request
     * @param pointer to request package object
	 * @return sign whether operation was performed successfully
    */
	void changeLogLevel( SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p );

	/**
	 * Resets uptime for VM process
	 */
	void resetUptime();

	/**
	 * Synchronizes VM uptime configuration parameter correspond with runtime value
	 */
	void updateVmUptime();

	/**
	 * Change VM state to VMS_COMPACTING
	 */
	PRL_RESULT changeVmStateToCompacting();

public://Global methods processing VM commands wrappers list
	/**
	 * Unregisteries specified VM wrapper object from global list
	 */
	static void UnregisterVmObject(const SmartPtr<CDspVm> &pVm);
	static void UnregisterAllVmObjects();
	/**
	 * Instantiates VM wrapper object
	 * @param wrapping VM UUID
	 * @param UUID of parent VM directory
	 * @param outError as reason of failure ( fill if method returns SmartPtr<NULL> )
	 * @param bNew is TRUE if new VM instance is created and false if VM is in g_pVmsMap or creating failed
	 * @param pointer to the user session initializing request
	 * @param cmd - command type (to avoid conflict with VM start exclusive operation)
	 * @return pointer to instantiated VM object or SmartPtr<NULL> if error.
	 */
	static SmartPtr<CDspVm> CreateInstance(const QString &sVmUuid, const QString &sVmDirUuid,
				PRL_RESULT& outCreateError,
				bool& bNew,
				const SmartPtr<CDspClient> &pUser,
				PVE::IDispatcherCommands cmd = PVE::DspCmdVmStart,
				const QString &sTaskId = QString());
	static SmartPtr<CDspVm> CreateInstance(const CVmIdent &vmIdent,
				PRL_RESULT& outCreateError,
				bool& bNew,
				const SmartPtr<CDspClient> &pUser,
				PVE::IDispatcherCommands cmd = PVE::DspCmdVmStart,
				const QString &sTaskId = QString());

	/**
	 * Returns VM object by specified VM and parent VM dir UUIDs
	 * @param VM UUID
	 * @param parent VM dir UUID
	 * @return pointer to necessary VM wrapper object (NULL pointer means that VM wrapper not found)
	 */
    static SmartPtr<CDspVm> GetVmInstanceByUuid(const QString &sVmUuid, const QString &sVmDirUuid) ;

	/**
	 * Returns VM object by specified dispatcher connection handle (in VM migration mode)
	 * @param dispatcher connection handle
	 * @return pointer to necessary VM wrapper object (NULL pointer means that VM wrapper not found)
	 */
	static SmartPtr<CDspVm> GetVmInstanceByDispConnectionHandle(const IOSender::Handle &h);

    static SmartPtr<CDspVm> GetVmInstanceByUuid(const CVmIdent &vmIdent) ;


public://Convenient global methods
	/**
	 * Returns state of specified VM
	 * @param VM UUID
	 * @param parent VM dir UUID
	 * @return actual VM state
	 */
	static VIRTUAL_MACHINE_STATE getVmState( const QString &sVmUuid, const QString &sVmDirUuid );

	/**
	 * Returns state of specified VM
	 * @param VM identification
	 * @return actual VM state
	 */
	static VIRTUAL_MACHINE_STATE getVmState( const CVmIdent& vmIdent );

	/**
	* Returns VM Addition state with task to exclude search
	*/
	static VIRTUAL_MACHINE_ADDITION_STATE getVmAdditionState( const QString & sVmUuid,
															const QString & sVmDirUuid,
															const CDspTaskHelper * pTaskToExclude = NULL );

	/**
	* Check that suspended files present
	* @param VM UUID
	* @param parent VM dir UUID
	* @return sign
	*/
	static bool suspendedFilesPresent( const QString &sVmUuid, const QString &sVmDirUuid );

	/**
	* Returns state of VM tools
	* @param VM UUID
	* @param parent VM dir UUID
	* @return actual VM state and version
	*/
	static PRL_VM_TOOLS_STATE getVmToolsState( const QString &sVmUuid, const QString &sVmDirUuid,
						   QString *sVersion = NULL );
	/**
	 * Method which was provided for convenience. Let's to get VM tool state as is
	 * (not wrapped into CVmEvent string representation) by VM identity
	 * @param VM identity
	 * @return VM tools state and version
	 */
	static PRL_VM_TOOLS_STATE getVmToolsState( const CVmIdent &_vm_ident, QString *sVersion = NULL );

	/**
	 * Returns remote display state of specified VM
	 * @param VM UUID
	 * @param parent VM dir UUID
	 * @return actual VM state
	 */
	static bool IsVmRemoteDisplayRunning( const QString &sVmUuid, const QString &sVmDirUuid );

	/**
	 * Globally (for all VM processes instances) cleanups guest OS sessions which
	 * belong to disconnected user session handle
	 * @param handle to disconnected user session
	 */
	static void globalCleanupGuestOsSessions( const IOSender::Handle &h );

	/**
	 * Cancels suspending sync phase for specified VM
	 * see https://bugzilla.sw.ru/show_bug.cgi?id=441619 for more details
	 * @param pointer to the user session object
	 * @param pointer to the initial request package
	 * @param VM UUID
	 * @param pointer to initiate object task to process task cancel
	 * @return actual VM state
	 */
	static void cancelSuspend(
		SmartPtr<CDspClient> pUser,
		const SmartPtr<IOPackage> &p,
		const QString &sVmUuid,
		CancelOperationSupport *pInitiator );

	void GetSnapshotRequestParams(SmartPtr<IOPackage> &pRequest,
		VIRTUAL_MACHINE_STATE &vmState,
		SmartPtr<CDspClient> &pUser,
		QString &sSnapshotTaskUuid);

	bool startVmAfterCreatingUndoDisks(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);
	bool startVmAfterCommitUnfunishedDiskOp(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);
	bool isUndoDisksMode() const;
	bool isSafeMode() const;
	bool isNoUndoDisksQuestion() const;
	void disableNoUndoDisksQuestion();
	bool startUndoDisksRevertOrCommitTask();

	bool hasUnansweredRequestForSession( const QString& qsSessionId ) const;

	/**
	 * Replaces initial object command
	 * NOTE: dirty hack - do this just in extremal situations such as:
	 * https://bugzilla.sw.ru/show_bug.cgi?id=446383
	 * @param new command identifier
	 * @param pointer to the user session object
	 * @return result of operation
	 */
	PRL_RESULT replaceInitDspCmd( PVE::IDispatcherCommands nNewCmd, const SmartPtr<CDspClient> &pUserSession );

	/**
	 * Extracts VM configuration
	 * @param pointer to th user session object (can be NULL - in this case m_pVmRunner uses; if last one also NULL
	 *        VM configuration extracts without impersonation)
	 * @param reference to the error on VM configuration extraction
	 * @return pointer to VM configuration object
	 */
	SmartPtr<CVmConfiguration> getVmConfig( SmartPtr<CDspClient> pUser, PRL_RESULT &nOutError) const;

	/**
	 * Restore VM process
	 */
	void restoreVmProcess(	VIRTUAL_MACHINE_STATE state,
							Q_PID VmProcessId = 0,
							SmartPtr<CDspClient> pUser = SmartPtr<CDspClient>());

	/**
	 * Restored VM product version
	 */
	void setRestoredVmProductVersion(const QString& qsVer);
	QString getRestoredVmProductVersion() const;

	/**
	 * VM migration helper - generates start VM migration request package
	 * with necessary additional information (host hardware info, VM configuration and etc.)
	 * @param pointer to start VM migration command
	 * @param pointer to original migrate VM request package
	 * @return pointer to newly generated VM migration package
	 */
	SmartPtr<IOPackage> CreateVmMigrationPackageWithAdditionalInfo(
		CProtoCommandPtr pMigrateVmCmd,
		const SmartPtr<IOPackage> &pMigrateVmRequest,
		VIRTUAL_MACHINE_STATE nPrevVmStateBeforeMigration
	);

	void applyVMNetworkSettings(VIRTUAL_MACHINE_STATE nNewState);
	void disableStoreRunningState(bool bDisable);

	void wakeupApplyConfigWaiters();
	bool waitForApplyConfig(unsigned long timeout);
	PRL_RESULT runActionScript(PRL_VM_ACTION nAction, const SmartPtr<CDspVm> &pVm, bool bWaitForResult = false);

	/**
	 * Store sign to restore Vm to the previous state before reboot
	 */
	void storeFastRebootState(bool bFastReboot, SmartPtr<CDspClient> pUserSession = SmartPtr<CDspClient>());

	QString prepareFastReboot(bool suspend);

	PRL_UINT32 getVNCPort();

	bool isConnected() const;

	/**
	* fix vm configuration parameters before start vm
	* @param hostInfo  - host hw config object
	* @param SmartPtr<CVmConfiguration> pConfig - config pointer object
	* @param SmartPtr<CDspClient> pUser - user session object - must be valid
	*/
	static void fixConfigBeforeStartVm(	const CHostHardwareInfo & hostInfo
		, SmartPtr<CVmConfiguration> pConfig
		, SmartPtr<CDspClient> pUser );

private://Global VM wrappers objects map
	/** Global VM wrappers objects map */
	static DspVm::Storage* g_pStorage;
	/** Global VM wrappers objects map access synchronization object */
	static QReadWriteLock *g_pVmsMapLock;

private:
	SmartPtr<DspVm::Details> m_pDetails;

	CDspVm();

	DspVm::Details& d();
	const DspVm::Details& d() const;
	void d(DspVm::Details* details_);
	void d(const SmartPtr<DspVm::Details>& details_);
	const CVmIdent& ident() const;
	/**
	* Class constructor. Private to prevent direct class objects instatination
	* @param VM identity
	* @param pointer to the client session intializing request
	* @param cmd - command type (to avoid conflict with VM start exclusive operation)
	* @param state - initial state of a VM.
	*/
	PRL_RESULT initialize(const CVmIdent& , const SmartPtr<CDspClient>& ,
				PVE::IDispatcherCommands , VIRTUAL_MACHINE_STATE, const QString &sTaskId);
	static SmartPtr<CDspVm> GetRoInstanceByUuid(const QString& vm_, const QString& dir_);
private:
	/**
	 * Closed function of CDspVm object memebers cleanup
	 */
	void cleanupObject();

	/**
	 * Applying new VM state
	 * @param setting VM state value
	 * @param setting VM power state value
	 */
	void changeVmState(VIRTUAL_MACHINE_STATE nVmState, VmPowerState powerState = vpsNormal);

	/**
	* Send deferred stop response in destructor of CDspVm
	* https://bugzilla.sw.ru/show_bug.cgi?id=267057
	*/
	void processDeferredResponses();

	/**
	* Check user permissions to execute command.
	* @param pUserSession
	* @param cmd
	* NOTE: Also check vm_config integrity,
	*	if vm is in invalid state then
	*	1) send notification to all permitted users and
	*	2) returns correspond error_code.
	* @return PRL_ERR_SUCCESS if OK or err_code when error
	*/
	PRL_RESULT checkUserAccessRights(SmartPtr<CDspClient> pUserSession, PVE::IDispatcherCommands cmd);
	/**
	* Check user permissions to execute command and send response if check failed
	*/
	PRL_RESULT checkUserAccessRightsAndSendResponseOnError(SmartPtr<CDspClient> pUserSession,
			const SmartPtr<IOPackage> &p,
			PVE::IDispatcherCommands cmd);
	/**
	 * Additional helper on checking access rights to the guest OS sessions commands
	 * @param pointer to the user session object
	 * @param pointer to the processing guest OS session command
	 * @return PRL_ERR_SUCCESS if command accepted to specified user session and negative error otherwise
	 */
	PRL_RESULT checkUserAccessRightsOnGuestConsoleCmd( SmartPtr<CDspClient> pUser, const CProtoCommandPtr &pCmd );

	template<class T>
	bool startVmProcess(DspVm::Start::Demand demand_, T monitor_);
	bool startVmProcess(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* Returns VM state without m_rwLock lock
	*   NOTE: m_rwLock MUST be locked before this call !
	*/
	VIRTUAL_MACHINE_STATE getVmStateUnsync() const;

	/**
	*  Turn On /Turn Off  Spotlight on MacOSX for vm folder
	*  https://bugzilla.sw.ru/show_bug.cgi?id=113144
	**/
	bool turnOnSpotlight( const QString& vmDirPath );
	bool turnOffSpotlight( const QString& vmDirPath );

	/**
	* Change USB state (auto connect logic) by VM state event
	* @param VM state value
	*/
	void changeUsbState( PRL_EVENT_TYPE nEventType );

	/**
	 * Get undo disks mode
	 * @return undo disks mode enumaration
	 */
	PRL_UNDO_DISKS_MODE getUndoDisksMode();

	/**
	 * Set safe mode
	 */
	void setSafeMode(bool bSafeMode, SmartPtr<CDspClient> pUserSession);

	void SetSnapshotRequestParams(const SmartPtr<IOPackage> &pRequest,
		VIRTUAL_MACHINE_STATE vmState,
		const SmartPtr<CDspClient> &pUser,
		const QString &sSnapshotTaskUuid);

	/**
	* Checks available disk space to start VM
	* @return PRL_ERR_SUCCESS when disk space is available
	* @return PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM
	*	when disk space was checked and it doesn't available
	* @return lot of another errors when we unable to check disk space
	* @param [out]outNoSpaceErrorParams - CVmeEvent that contains parameters
	*		for PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_START_VM message
	*/
	PRL_RESULT checkDiskSpaceToStartVm( CVmEvent& outNoSpaceErrorParams );

// Guest OS sessions list manipulations helpers set
	/**
	 * Registries new guest session entry at guest sessions list
	 * @param pointer to the user session object that creating guest OS session (can't be NULL)
	 * @param pointer to initiate package with command on guest OS session creation (can't be NULL)
	 */
	void registerNewGuestSession(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);
	/**
	 * Checks whether specified response package relative to open guest OS session request
	 * (DspCmdVmLoginInGuest command) and replaces storing at guest OS sessions list request
	 * package UUID with correspond guest OS session UUID.
	 * @param pointer to the response command object
	 * @param pointer to the response package object
	 */
	void checkWhetherResponseOnOpenGuestOsSessionCmd( const CProtoCommandPtr &pCmd, const SmartPtr<IOPackage> &p );
	/**
	 * Cleanups guest OS sessions which were opened by specified user session connection handle
	 * @param handle to the user session connection
	 */
	void cleanupGuestOsSessions( const IOSender::Handle &h );
	/**
	 * Processes received response package from VM process on guest OS sessions routines.
	 * @param pointer to the response package object
	 */
	void processResponseForGuestOsSessionsCmds( const SmartPtr<IOPackage> &p );
	/**
	 * Checks whether requested guest OS session exists for specified user session
	 * @param pointer to the user session object requesting guest OS session
	 * @param guest OS session UUID
	 * @return sign whether reuqested guest OS session available
	 */
	bool isGuestOsSessionExists( const SmartPtr<CDspClient> &pUser, const QString &sVmSessionUuid );
	/**
	 * Unregistries specified guest OS session from active user guest OS sessions list
	 * @param pointer to the user session object which own guest OS session
	 * @param guest OS session UUID
	 */
	void unregisterGuestSession( const SmartPtr<CDspClient> &pUser, const QString &sVmSessionUuid );

	/**
	* fix vm configuration parameters before start vm
	* @param hostInfo  - host hw config object
	* @param SmartPtr<CVmConfiguration> pConfig - config pointer object
	* @param SmartPtr<CDspClient> pUser - user session object - must be valid
	*/
	void fixConfigBeforeStartVm( const CHostHardwareInfo & hostInfo, SmartPtr<CVmConfiguration> pConfig );

	/**
	* check DVD device command to default dvd rom present and
	* repack package with patched parameters.
	* @param CProtoCommandPtr * pDeviceCmd - device command object
	* @return - fixed package to post
	*/
	SmartPtr<IOPackage> getConnectionFixedPackageForDvd( const CHostHardwareInfo & hostInfo,
							const SmartPtr<IOPackage> &p,
							CProtoVmDeviceCommand * pDeviceCmd,
							PVE::IDispatcherCommands cmd );

	/**
	* patch vm config for remote devices and save it on disk
	*/
	void disconnnectRemoteDevicesAndSaveConfig( );

	/**
	 * Internal helper which calculates VM uptime based on start time measurement
	 * @param VM process start time in ticks
	 * @return VM process uptime in seconds
	 */
	PRL_UINT64 getVmUptimeInSecsInternal(PRL_UINT64 nStartTicksCount) const;

	/**
	 * Store running sign to restore Vm to the previous state on dispatcher failover
	 */
	void storeRunningState(bool bRunning);

	void checkAutoconnectUsb();
	/* internal code to check VmPowerState and send error */
	bool	isContinueSnapshotCmdFromPausedStateAllowed( VmPowerState nVmPowerState,
	SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p,
	CVmEvent* evt, bool bWaitResult);

	void abortVmStart(const IOSender::Handle &h, SmartPtr<IOPackage> &p, PRL_RESULT res);

};

#endif // __CDSPVM_H__
