///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspService.h
///
/// Dispatcher entry point
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

#ifndef __DSP_DISPATCHER_H__
#define __DSP_DISPATCHER_H__

#include <QCoreApplication>
#include <QAtomicInt>
#include <QList>

#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/PrlCommonUtilsBase/CommandLine.h>
#include "CDspDispConfigGuard.h"
#include "CDspTaskManager.h"
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "CDspUserHelper.h"
#include "CDspShellHelper.h"
#include "CDspVmDirHelper.h"
#include "CDspVmSnapshotStoreHelper.h"
#include "CDspVmDirManager.h"
#include "CDspAccessManager.h"
#include "CDspSync.h"
#include "DspMonitor.h"
#include "CDspVmMigrateHelper.h"
#include "CDspVmConfigurationChangesWatcher.h"
#include "CDspVmConfigManager.h"
#include "CDspBroadcastListener.h"
#include "CDspIOClientHandler.h"
#include "CDspIOCtClientHandler.h"
#include "CDspHaClusterHelper.h"
#include "CDspSettingsWrap.h"
#include "CDspBackupHelper.h"
#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#endif // _LIBVIRT_
#include "Stat/CDspStatCollectingThread.h"

#include "HwMonitor/CDspHwMonitorThread.h"

#include <prlcommon/IOService/IOCommunication/IOServer.h>
#include <prlcommon/IOService/IOCommunication/IOServerPool.h>
#include <prlcommon/IOService/IOCommunication/IORoutingTableHelper.h>
#include "Libraries/PrlNetworking/IpStatistics.h"
#include "Libraries/HostInfo/CHostInfo.h"
#include <prlcommon/PrlCommonUtilsBase/CFeaturesMatrix.h>

#include <prlxmlmodel/VmDirectory/CVmDirectories.h>
#include <prlxmlmodel/NetworkConfig/CParallelsNetworkConfig.h>

#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

#include <Libraries/Etrace/Etrace.h>

Q_DECLARE_METATYPE(VIRTUAL_MACHINE_STATE)

using namespace IOService;
using Parallels::CProtoCommand;
using Parallels::CProtoCommandPtr;
using Parallels::CDispToDispCommand;
using Parallels::CDispToDispCommandPtr;

class CDspClientManager;
class CDspVmManager;
class CDspHandler;
class CDspRecognitionMetadataMgr;
class CDspVmStateSender;
class CDspVmStateSenderThread;
class CDspVzHelper;
class ConnectionsStatisticsManager;
#ifdef ETRACE
class CEtraceStatic;
#endif
class CDspAsyncRequest;
class CDspDispConnectionsManager;

namespace Registry
{
struct Actual;
} // namespace Registry

class CDspService : public QObject
{
	Q_OBJECT
public:
	static CDspService* instance ();

	CDspService ();
	 ~CDspService ();

	/** Sends simple response event to IO client */
	IOSendJob::Handle sendSimpleResponseToClient ( const IOSender::Handle&,
												   const SmartPtr<IOPackage>&,
												   PRL_RESULT );

	/**
	 * Sends simple response event to dispatcher client
	 * @param handle to dispatcher client connection
	 * @param pointer to request package object
	 * @param return code
	 * @returns handle to transport job which can be used to maintain sending response process
	 */
	IOSendJob::Handle sendSimpleResponseToDispClient (
		const IOSender::Handle &h,
		const SmartPtr<IOPackage> &pRequestPkg,
		PRL_RESULT nRetCode
	);

	/**
	 * Sends prepared response package to IO client
	 * @param handle to the sender object
	 * @param pointer to prepared response package object
	 * @param pointer to initial request package object
	 */
	IOSendJob::Handle sendResponse(	const IOSender::Handle &h,
									const CProtoCommandPtr &pResponse,
									const SmartPtr<IOPackage> &pRequestPkg);

	/**
	 * Sends prepared response package to dispatcher client
	 * @param handle to the sender object
	 * @param pointer to prepared response package object
	 * @param pointer to initial request package object
	 */
	IOSendJob::Handle sendResponseToDispClient(	const IOSender::Handle &h,
									const CDispToDispCommandPtr &pResponse,
									const SmartPtr<IOPackage> &pRequestPkg);

	/** Returns IO server pool instance */
	IOServerPool& getIOServer ();

	/** Returns listening IO server instance */
	IOServerInterface& getListeningIOServer ();

	/** Returns task manager */
	CDspTaskManager& getTaskManager ();

	/** Returns config vm watcher */
	CDspVmConfigurationChangesWatcher & getVmConfigWatcher ();

	/** Returns config vm manager */
	CDspVmConfigManager & getVmConfigManager();

	/** Returns user helper instance */
	CDspUserHelper& getUserHelper ();

	/** Returns vmDirHelper helper instance */
	CDspVmDirHelper& getVmDirHelper ();

#ifdef _CT_
	SmartPtr<CDspVzHelper> getVzHelper();
#endif

	/** Returns vmSnapshotStoreHelper helper instance */
	CDspVmSnapshotStoreHelper& getVmSnapshotStoreHelper ();

	/** Returns shell helper instance */
	CDspShellHelper& getShellServiceHelper ();

	/** Returns dispathcher monitor */
	CDspMonitor& getDispMonitor ();

	/////////////////////////////////////////////////////////
	/// Methods below are responsible for different configs
	/// or XML info classes
	/////////////////////////////////////////////////////////

	/** Returns reference to main dispatcher configuration gaurd object */
	CDspDispConfigGuard& getDispConfigGuard ();

	/** Returns locked pointer to network config object */
	CDspLockedPointer<CParallelsNetworkConfig> getNetworkConfig();

	/** Returns locked pointer to dispatcher QSettings object */
	/*  NOTE: Don't forget call sync() method to save your changes !; */
	CDspLockedPointer<QSettings> getQSettings();

	/** Returns host info instance */
	CDspLockedPointer<CDspHostInfo> getHostInfo ();

	/** Returns vm dir manager */
	CDspVmDirManager& getVmDirManager();

	/** Returns access checker */
	CDspAccessManager& getAccessManager();

	/** Returns client manager **/
	CDspClientManager&	getClientManager();

	/** Returns vm manager **/
	CDspVmManager&	getVmManager();

	CDspIOCtClientHandler& getIoCtClientManager();

	/** Returns VM migrate helper */
	CDspVmMigrateHelper &getVmMigrateHelper();

	/** Returns hw Monitor thread object*/
	CDspHwMonitorThread &getHwMonitorThread();

    /** Get the manager of the task downloading metadata for the recognition **/
    CDspRecognitionMetadataMgr& getRecognitionMetadataMgr();

	SmartPtr<CDspHaClusterHelper> getHaClusterHelper();

	/** Returns statistics manager **/
	ConnectionsStatisticsManager&	getConnStatManager();

	/**
		@returns sender of PET_DSP_EVT_VM_STATE_CHANGED event
		@note  NEED check return value: may be NULL ! (when CDspVmStateSender destroyed)
	**/
	CDspLockedPointer<CDspVmStateSender> getVmStateSender();

	/**
	 * Returns features list of current diapatcher instance
	 */
	CFeaturesMatrix getFeaturesMatrix();

	/**
	* Process migration events from Vm
	* @param pointer to vm
	* @param pointer to the initial request package
	*/
	void handleVmMigrateEvent(SmartPtr<CDspVm> pVm, const SmartPtr<IOPackage> &p);

	/**
	 * Starts NAT detection mechanism if necessary
	 * @param client connection mode (direct or proxy)
	 */
	void startNatDetection(IOSender::ConnectionMode mode);
	void updateNatStatistics(const struct NATStatistic &stat);

#ifdef ETRACE
	CDspLockedPointer<CEtraceStatic> getEtrace();
#endif


public:

// permission for created directories
	enum PermissionType { permDispConfigDir = 1, permVmDir, permBackupDir };

	bool checkExistAndCreateDirectory ( const QString& strDirPath,
										CAuthHelper& rootAuth,
										PermissionType perm );
public:

	void initPrivateNetworks();

	void emitCleanupOnUserSessionDestroy( QString sessionUuid );

	PRL_RESULT updateCommonPreferences(const boost::function1<void, CDispCommonPreferences&>& action_);
	void notifyConfigChanged(const SmartPtr<CDispCommonPreferences>&, const SmartPtr<CDispCommonPreferences>&);

public slots:

	/**
	 * Stops any addr listening.
	 */
	void stopListeningAnyAddr ();

	/**
	 * Tries to up custom listening interface
	 * @param address for listening
	 */
	void upCustomListeningInterface( const QString & sListenAddr );

	/**
	* Enable or disable firewall incoming connections for service listen port
	* @param true means disable firewall (make allow rule, exception) and unblock connections.
	*/
	void checkAndDisableFirewall( bool );

signals:
	void onConfigChanged(const SmartPtr<CDispCommonPreferences>, const SmartPtr<CDispCommonPreferences>);
	void onDoStopFromMainThread();
	void onDoNatDetectFromMainThread();
	void onUpCustomListeningInterface(const QString &);

	void cleanupOnUserSessionDestroy(QString sessionUuid);

	/* retransmit Vm migration events to migration task */
	void onVmMigrateEventReceived(const QString &, const SmartPtr<IOPackage> &);

private slots:
    /** Do dispatcher stop from main thread */
    void stopFromMainThread ();

    /** Slot for new clients */
    void clientConnected ( IOSender::Handle );

    /** Slot for disconnection */
    void clientDisconnected ( IOSender::Handle );

    /** Slot for new package from client */
    void packageReceived ( IOSender::Handle,
                           const SmartPtr<IOPackage> );

    /** Slot for client stat change */
    void clientStateChanged ( IOSender::Handle, IOSender::State );

    /** Slot for client detaching */
	void clientDetached ( IOSender::Handle h,
						  const IOCommunication::DetachedClient dc );

	void onClientStateChanged( IOServerInterface*, IOSender::Handle, IOSender::State );

	/** Slot for doing work when user credentials become available */
	void onCredentialsBecomeAvailable();

	void onCleanupOnUserSessionDestroy( QString sessionUuid );

	void onTerminateSignalReceived();

private:
	bool init();
	bool initIOServer ();
	bool initHostInfo ();
	bool setupDispEnv ();
	bool initDispConfig ();
	bool initVmDirCatalogue ();
	bool checkConfigsIntegrity ();
	void initHypervisor();
	void precacheVmConfigs();
	void reconnectToRunningVms();

	bool createDispConfig ();
	bool updateDispConfig ();

	void patchDirCatalogue();
	bool initAllConfigs();
	bool recoverAllConfigs();
	void initVmStateSender();
	void initSyncVmUptimeTask();

	void initPlugins();

	void restoreAfterCrash();

	// Reload networking configuration.
	// @param bNotifyNetworkService true if we should always notify networking service after
	// reading configuration. Otherwise, networking service will be notified only if dispatcher
	// have somehow fixed configuration.
	bool initNetworkConfig(bool bNotifyNetworkService = false);
	void initNetworkPreferences(CDispCommonPreferences& config_);

	bool checkVmPermissions();
	void checkRunningVms();

	QString makeBackupSuffix();
	void printHorrorLogMessage ( const QString&, PRL_RESULT );


	void createIOServers( quint32 listenPort, PRL_SECURITY_LEVEL securityLevel );

	virtual void timerEvent(QTimerEvent* te);

	enum StopPhase {SP_START_STOPPING, SP_STOP_PENDING};
	void stopServiceStatus(StopPhase phase);

	void enableConfigsCrashSafeMech();

	void rebootHost();
	void cleanupAllStaticObjects();
	/**
	 * Fills features matrix for current service mode
	 */
	void initFeaturesList();

	bool isDispVersionChanged();

	typedef QList< SmartPtr<IOServerInterface> > IOServerList;

	bool setCredentialsForServers(const IOServerList & lstServers);
	bool ensureHostIdAvailable();
public:
	static bool isDispMajorVersionChanged();

	static quint32 getDefaultListenPort();

	void start ();
	enum StopMode {SM_FORCE_STOP, SM_BEGIN_STOP, SM_END_STOP};
	void stop (StopMode stop_mode);
	bool isServerStopping() const;

	void doStopFromMainThread ();

	ProcPerfStoragesContainer* getPerfStorageContainer() ;

	storage_descriptor_t getBasePerfStorage() ;

	// #128020
	QString getHostOsVersion() const;

	void prepareRebootHost ();

	void printTimeStamp();

	bool waitForInitCompletion();
	bool isFirstInitPhaseCompleted() const;

	quint64 getServiceStartTime() const;
	quint64 getServiceStartTimeMonotonic() const;

private:

	void wakeAllInitCompletedWaiters( bool bCompleted );
private:

	// to prevent dispatcher crash on SIGTERM before init done.
	// #121765, #121764
	bool m_bInitWasDone;
	bool m_bStopWasSentOnInitPhase;
public:
	// TODO: Need move code with 'waitForInitCompletion' here.
	bool isServerStartedCompletely() { return m_bInitWasDone; }

private:
#ifdef ETRACE
	QMutex m_etraceMutex;
#endif

	// timer to trace current date time every day (need to investigate problems by long logs).
	int m_nTimestampTraceTimerId;

	bool m_bServerStopping;
	enum StopTimeout { ST_STEP_TIMER_AT_STOPPING = 1000 };
	int m_nStopTimeout;
	int m_nStopTimerId;
	bool m_bRebootHost;

	QString m_sStatisticReporterTaskUuid;

	ProcPerfStoragesContainer m_perfstorage_container ;
	storage_descriptor_t       m_base_perfstorage ;

	SmartPtr<IOServerInterface> m_ioListeningServer;
	SmartPtr<IOServerInterface> m_ioLocalUnixListeningServer;
	SmartPtr<IOServerInterface> m_ioAnyAddrServer;
	SmartPtr<IOServerPool> m_ioServerPool;

	SmartPtr<CDspVmStateSenderThread> m_pVmStateSenderThread;

		// mutex should be defined before its data. ( to destroy after data )
	QMutex m_hostInfoMutex;
	CDspHostInfo m_hostInfo;

	CDspSettingsWrap m_AppSettings;

	QMutex	m_networkConfigMutex;
	CParallelsNetworkConfig m_networkConfig;

	SmartPtr<ConnectionsStatisticsManager> m_pConnectionsStatManager;

	QReadWriteLock	m_rwlWaitForInitCompletion;
	QWaitCondition	m_wcWaitForInitCompletion;
	bool			m_bWaitForInitCompletion;
	bool			m_bFirstInitPhaseCompleted;

	CDspVmConfigManager m_configManager;

	CDspAccessManager	m_vmAccessManager;		// should be defined before m_taskManager ( bug #6042 )
	CDspVmDirManager	m_vmDirManager;		// should be defined before m_taskManager ( bug #6042 )
	CDspDispConfigGuard m_dispConfigGuard;  // should be defined before m_taskManager ( bug #6042 )

	SmartPtr<CDspHandler> m_pVmManagerHandler;
	SmartPtr<CDspHandler> m_pIoHandler;
	SmartPtr<CDspHandler> m_pIoCtHandler;
	QScopedPointer<CDspClientManager> m_clientManager;
	QScopedPointer<CDspDispConnectionsManager> m_dispConnectionsManager;

	/** Broadcast messages processing service */
	SmartPtr<CDspBroadcastListener> m_pBroadcastMsgsProcessingService;
	/** Host hardware changes monitoring thread */
	SmartPtr<CDspHwMonitorThread> m_pHwMonitorThread;

	// server uuid if cannot parse current Dispatcher.xml
	QString		m_strServerUuidFromCorruptedDispConfig;

	//VM migration helper object
	QScopedPointer<CDspVmMigrateHelper> m_vmMigrateHelper;

	// Helpers
	SmartPtr<CDspUserHelper> m_pUserHelper;
	QScopedPointer<CDspShellHelper> m_shellHelper;
	QScopedPointer<CDspVmDirHelper> m_vmDirHelper;
	CDspVmSnapshotStoreHelper m_vmSnapshotStoreHelper;

	SmartPtr<CDspVmConfigurationChangesWatcher> m_pVmConfigWatcher;

#ifdef _CT_
	SmartPtr<CDspVzHelper> m_pVzHelper;
#endif

	// HA cluster helper object
	SmartPtr<CDspHaClusterHelper> m_pHaClusterHelper;

	QTimer * m_pReconnectTimer;

	// Re-patch flag
	QString m_qsOldDispVersion;

	/** Dispatcher start time in msecs since Epoch 01-01-1970 (wall clock time) */
	quint64 m_serviceStartTime;
	/** Dispatcher start time in msecs (monotonic clock time) */
	quint64 m_serviceStartTimeMonotonic;

	//////////////////////////////////////////////////////////////////////////
	// Complex objects should be last in  definition list:
	// Reason:  Complex objects should be destroyed BEFORE their dependencies
	// Bug: #6042 http://bugzilla.parallels.com/show_bug.cgi?id=6042
	//
	//////////////////////////////////////////////////////////////////////////
	SmartPtr<CDspTaskManager> m_pTaskManager;
	CDspMonitor m_dispMonitor;

	// #128020
	QString		m_strHostOsVersion;

	CFeaturesMatrix m_FeaturesMatrix;
#ifdef _LIBVIRT_
	QScopedPointer<Libvirt::Host> m_hypervisor;
#endif // _LIBVIRT_
	QScopedPointer<Registry::Actual> m_registry;
	Backup::Activity::Service m_backup;
};

///////////////////////////////////////////////////////////////////////////////
// struct CDspPid

class CDspPid: public QObject
{
	Q_OBJECT
public:
	explicit CDspPid(const QString& name_);

	bool attach();
public slots:
	void detach();

private:
	QFile m_file;
};

// Dispatcher service class
class CMainDspService : public QObject
{
	Q_OBJECT

	friend class CDspService; //to create pidfile
	friend void sigterm_handler(int signum); // to access to stop()
public:
	static CMainDspService* instance ();

	CMainDspService ( int argc, char **argv );
	~CMainDspService();

	CDspService* serviceInstance ();

	// Custom execution routine
	int exec();

	// Overridden start service routine
	void start ();

	// Stop routine
	void stop ();

	// Session change
	void sessionChange(unsigned reason, unsigned sessionId);

	// Stop with reboot host
	void stopWithHostReboot ();

	/**
	 * Should be called only from main thread.
	 * Use #stop variant if you use not from main thread
	 */
	void doStop ();

	static bool allowSkipVersionCheckOnVmReconnect()
	{ return g_bSkipVmVersionCheck; }

	static bool isLaunchdMode()
		{ return g_bLaunchdMode; }

public slots:
	/* For async execution. */
	void onStart()
	{
		start();
	}

private:
	QScopedPointer<CDspService>  m_service;

	//--------------- parameters storage --------------
	QScopedPointer<CDspPid> m_pid;
	static bool g_bSkipVmVersionCheck;
	static bool g_bLaunchdMode;
	QScopedPointer<QCoreApplication> m_application;

	static CMainDspService *g_instance;

private:
	 void initializeInternalParameters ( int argc, char **argv );
	 void processCommandLineArgs ( const CommandLine::Parser& parser );
};

#endif // __DSP_DISPATCHER_H__
