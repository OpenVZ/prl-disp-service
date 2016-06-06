///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspService
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

#include <QFile>
#include <QDir>
#include <QMutableListIterator>

#include <prlcommon/Interfaces/ParallelsQt.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "CDspService.h"
#include "CDspRouter.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include <prlcommon/IOService/IOCommunication/IOSSLInterface.h>
#include "CDspHandlerRegistrator.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include "Libraries/PrlCommonUtils/PrlQSettings.h"
#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include "CDspStarter.h"
#include "CDspVm_p.h"
#include "CDspClientManager.h"
#include "CDspVmManager.h"
#include "CDspBugPatcherLogic.h"
#include "CDspProblemReportHelper.h"
#include "CDspAsyncRequest.h"
#include "CDspRegistry.h"

#include "CDspCommon.h"
#include "CDspTestConfig.h"
#include "CDspDispConnectionsManager.h"

#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/DispConfig/CDispNetAdapter.h>
#include <prlxmlmodel/DispConfig/CDispDhcpPreferences.h>
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Std/PrlTime.h>
#include "Libraries/Virtuozzo/CVzPrivateNetwork.h"
#include "EditHelpers/CMultiEditMergeVmConfig.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team

#include "CDspVmStateSender.h"

#include "Tasks/Task_AutoStart.h"
#include "Tasks/Task_UpdateCommonPrefs.h"
#include "Tasks/Task_SyncVmsUptime.h"
#include "Tasks/Task_BackgroundJob.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"
#include "Interfaces/Config.h"
#include <prlcommon/Interfaces/ApiDevNums.h>

#include "Build/Current.ver"

#include <systemd/sd-daemon.h>

#ifndef _WIN_
 #include <signal.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <sys/file.h>
#else
 #include <accctrl.h>
 #include <lmcons.h>
 #include <windows.h>
 #include <process.h>
 #include <stdlib.h>
 #define getpid _getpid
#endif

#ifndef _WIN_
	#include "Libraries/PrlCommonUtils/CUnixSignalHandler.h"
#endif

#ifdef _LIN_
 #include "Tasks/Task_MountVm.h"
 #include <errno.h>
 #include <malloc.h>
#endif

#include "CDspVzHelper.h"

#if defined(_LIN_)
#include "Libraries/PrlCommonUtils/RLimits.h"
#endif

using namespace Parallels;


#define QSETTINGS_TAG_MODE_SERVER	"server"
#define QSETTINGS_TAG_MODE_DESKTOP	"desktop"
#define QSETTINGS_TAG_SERVER_UUID	"dispatcher_uuid"

// #429897  This names should be used because paths in QSettings are based of its.
// It need to provide compatibility with qsettings which created in our old products
//	( Paralles Workstastion Extreme / Parallels Desktop / Parallels Server for Mac ).
//	( before this patch for #429897 )
// TODO: Need implement mirgating this settings to new names in future.

#define DISP_APPLICATION_NAME_SERVER		"server"

/*****************************************************************************/

#define WAIT_FOR_INIT_COMPLETION_TIMEOUT		60 * 1000	// 1 min

namespace
{
const char* g_sPidFileName_DEFAULT =
	#ifndef _WIN_
		"/var/run/prl_disp_service.pid";
	#else
		0;
	#endif

IOSendJob::Handle convey(IOSendJob::Handle job_, IOServerPool& io_)
{
	if (IOSendJob::SendQueueIsFull == io_.getSendResult(job_))
	{
		WRITE_TRACE(DBG_FATAL, "cannot send the response: the output queue is full");
	}
	return job_;
}

inline IOServer* setup(IOServer* server_)
{
	server_->getJobManager()->setActiveJobsLimit(2000);
	return server_;
}

#ifdef _LIN_
int handle(const storage_descriptor_t* unit_, void* data_)
{
	QStringList* d = reinterpret_cast<QStringList* >(data_);
	if (NULL != d && unit_ != NULL)
	{
		QString p("/proc/self/fd/");
		p += QString::number(reinterpret_cast<intptr_t>(unit_->internal));
		QString t = QFile::symLinkTarget(p);
		if (!t.isEmpty())
			*d << t;
	}
	return ENUM_CONTINUE;
}
#endif // _LIN_
} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct CDspPid

CDspPid::CDspPid(const QString& name_)
{
	m_file.setFileName(name_);
}

bool CDspPid::attach()
{
	if (m_file.fileName().isEmpty())
		return true;

	if (!m_file.open(QIODevice::WriteOnly))
	{
		WRITE_TRACE(DBG_FATAL, "Can't create the pid file by error %s",
			QSTR2UTF8(m_file.errorString()));
		return false;
	}
	QString d = QString::number(getpid()).append("\n");
#ifndef _WIN_
	while (0 != flock(m_file.handle(), LOCK_EX | LOCK_NB))
	{
		if (EWOULDBLOCK == errno)
		{
			WRITE_TRACE(DBG_FATAL, "Another instance of service is running");
			goto error;
		}
		if (EINTR != errno)
		{
			WRITE_TRACE(DBG_FATAL, "Cannot lock the pid file");
			goto error;
		}
	}
	if (0 != chmod(QSTR2UTF8(m_file.fileName()), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))
	{
		WRITE_TRACE(DBG_FATAL, "Can't change permission of the pid file [%s]. error=[%s]"
			, QSTR2UTF8(m_file.fileName()), strerror(errno));
	}
#endif // _WIN_
	if (-1 == m_file.write(d.toAscii()))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot write the process ID into the pid file");
		goto error;
	}
	if (!m_file.flush())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot flush data into the pid file");
		goto error;
	}
	return true;
error:
	m_file.close();
	return false;
}

void CDspPid::detach()
{
	if (m_file.isOpen())
	{
		m_file.close();
		m_file.remove();
	}
}

bool CMainDspService::g_bSkipVmVersionCheck = false;
bool CMainDspService::g_bLaunchdMode = false;
CMainDspService *CMainDspService::g_instance = NULL;

/*****************************************************************************/

bool CDspService::isServerMode()
{
	return ParallelsDirs::getAppExecuteMode() == PAM_SERVER;
}

bool CDspService::isServerModePSBM()
{
	return ParallelsDirs::isServerModePSBM();
}

/*****************************************************************************/

CMainDspService* CMainDspService::instance ()
{
	return g_instance;
}

static void setup_rlimit()
{
	// addinfo #7821
#if defined(_LIN_)
	rlim_t nAppliedValue = 0;
	if ( SetMax_RLIMIT_NOFILE( &nAppliedValue ) )
	{
		WRITE_TRACE(DBG_FATAL, "RLIMIT_NOFILE changed to maximum: %u", (unsigned)nAppliedValue);
	}
	else
	{
		int nError = errno;
		WRITE_TRACE(DBG_FATAL, "warning: Can't change RLIMIT_NOFILE: %d, nAppliedValue=%u"
			, nError, (unsigned)nAppliedValue);
	}
#endif

}

CMainDspService::CMainDspService ( int argc, char** argv )
{
	Q_ASSERT(!g_instance);
	g_instance = this;

	m_application.reset(new QCoreApplication(argc, argv));

	SetLogFileName(GetDefaultLogFilePath(), "prl-disp.log");
	initializeInternalParameters(argc,argv);

	srand( (unsigned)PrlGetTickCount() );

	// #PDFM-36906 Fix to prevent dispatcher crash on QT-side when pipe(2) returns EMFILE / ENFILE errno.
	setup_rlimit();
}

CMainDspService::~CMainDspService ()
{
	doStop();

#ifdef _DEBUG

	QString tmpDir = ParallelsDirs::getSystemTempDir();

	if ( ! tmpDir.isEmpty() )
	{
		// need to test bug #2293 [Dispatcher crashes on stop]
		QString strFinishFileName = QString( "%1/%2" ).arg( tmpDir ).arg( QString( "parallels.finish.info" ) );
		// to prevent using this file as exploit ( bug #8267 )
		if( ! QFileInfo( strFinishFileName ).isSymLink() )
		{
			QFile   f( strFinishFileName );
			if ( f.open(QIODevice::WriteOnly) )
				f.write( QString("%1").arg( time( NULL ) ).toUtf8() );
		}
	}
#endif // _DEBUG

	//////////////////////////////////////////////////////////////////////////
	// get server product version
	WRITE_TRACE(DBG_FATAL, "Dispatcher service deinited. [ version = %s [ mode = %s ] ]"
		, QSTR2UTF8( CVmConfiguration::makeAppVersion() )
		, ParallelsDirs::getAppExecuteModeAsCString() );
}

CDspService* CMainDspService::serviceInstance ()
{
	return m_service.data();
}

int CMainDspService::exec()
{
	::umask(027);
	// Run start() asynchroniously
	QTimer::singleShot(0, this, SLOT(onStart()));
	// Run event loop
	return QCoreApplication::exec();
}

void CMainDspService::start ()
{
	// First of all we need to reset logging file on Dispatcher startup
	Logger::ResetLogFile();

	if ( PAM_UNKNOWN == ParallelsDirs::getAppExecuteMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong execute mode for application.");
				goto Error;
	}

	WRITE_TRACE(DBG_FATAL, "Execute mode is %s.", ParallelsDirs::getAppExecuteModeAsCString() );

#ifndef _WIN_
	// #PDFM-23800 [Parallels Service should be started as ROOT to prevent startup problems]
	if( geteuid() )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to start dispatcher under not-root user( euid: %d, uid: %d ) ",
								geteuid(), getuid() );
		goto Error;
	}
#endif
	if (!m_pid.isNull())
	{
		if (!m_pid->attach())
			goto Error;
		m_pid->connect(m_application.data(), SIGNAL(aboutToQuit()), SLOT(detach()));
	}
	m_service.reset(new CDspService());
	m_service->start();
	sd_notify(0, "READY=1");
	return;

Error:
	sd_notifyf(0, "ERRNO=%d", EINVAL);
	QCoreApplication::exit(-1);
}

void CMainDspService::stop ()
{
	// Stop directly in main thread
	if ( QThread::currentThread() == QCoreApplication::instance()->thread() ) {
		doStop();
	}
	else
		m_service->doStopFromMainThread();
}

void CMainDspService::sessionChange(unsigned reason, unsigned sessionId)
{
	(void)reason; (void)sessionId;
}

void CMainDspService::stopWithHostReboot ()
{
	m_service->prepareRebootHost ();
	stop();
}

void CMainDspService::doStop ()
{
	if (!m_service.isNull())
	{
		m_service->stop(CDspService::SM_BEGIN_STOP);
	}
}

void CMainDspService::initializeInternalParameters (
	int argc, char **argv )
{
	CommandLine::Parser* parser=0;

#ifdef _WIN_
	if (argc==1)
	{
		QString cmdLine=UTF8_2QSTR(argv[0])+" ";

		QString regKey="HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\" DISPATCHER_SERVICE_COMMON_NAME;
		QSettings settings(regKey, QSettings::NativeFormat);

		settings.beginGroup("Parameters");
		cmdLine+=settings.value("AppParameters").toString();
		settings.endGroup();

		WRITE_TRACE(DBG_FATAL, "cmdline='%s'", QSTR2UTF8( cmdLine ));
		parser=new CommandLine::Parser(cmdLine);

	}else
#endif
	{
		parser=new CommandLine::Parser(argc, argv);
	}

	processCommandLineArgs(*parser);
	if (parser)
		delete parser;
}


void CMainDspService::processCommandLineArgs (
	const CommandLine::Parser& parser)
{
	//////////////////////////////////////////////////////////////////////////
	// set pid file name
	QString value =
		parser.getValueByKey( CommandLine::g_strCommonKeyName_PidFileName );
	{
		//make full fname from local
		QString fname;
		if (!value.isEmpty())
		{
			fname= QDir(value).absolutePath();
			fname = QDir::cleanPath(fname);
		}

		if (!fname.isEmpty())
			m_pid.reset(new CDspPid(fname));
		else if (g_sPidFileName_DEFAULT)
			m_pid.reset(new CDspPid(g_sPidFileName_DEFAULT));
	}

	//////////////////////////////////////////////////////////////////////////
	// set execute mode
	value = parser.getValueByKey( CommandLine::g_strCommonKeyName_ModeName );

	value = CommandLine::g_strCommonValue_ModeName_PS;

	QCoreApplication::setOrganizationName( VER_COMPANYNAME_REGISTRYBRANCH_STR );
	QCoreApplication::setOrganizationDomain( VER_WEBSITE_STR );

	if ( value == CommandLine::g_strCommonValue_ModeName_PS )
	{
		ParallelsDirs::Init( PAM_SERVER );
		QCoreApplication::setApplicationName( DISP_APPLICATION_NAME_SERVER );
	}
	else
	{
		ParallelsDirs::Init( PAM_UNKNOWN );
	}

	CMainDspService::g_bSkipVmVersionCheck = parser.hasKey( CommandLine::g_strSkipVmVersionCheck );
}

/*****************************************************************************/

CDspService* CDspService::instance ()
{
	CMainDspService* mainService = CMainDspService::instance();
	if ( mainService )
		return mainService->serviceInstance();
	else
		return 0;
}

namespace{
	bool g_bSIGTERM_was_received = false;

bool waitUntilDriversLoaded( int timeoutMsecs )
{
	(void) timeoutMsecs;
	return true;
}

} // namespace

CDspService::CDspService () :
m_bInitWasDone(false),
m_bStopWasSentOnInitPhase(false),
m_nTimestampTraceTimerId(-1),
m_bServerStopping(true),
m_nStopTimeout(0),
m_nStopTimerId(-1),
m_bRebootHost(false),
m_hostInfoMutex( QMutex::Recursive ),
m_bWaitForInitCompletion( false ),
m_bFirstInitPhaseCompleted( false ),
m_pVmManagerHandler( CDspHandlerRegistrator::instance().findHandler( IOSender::Vm ) ),
m_pIoHandler( CDspHandlerRegistrator::instance().findHandler( IOSender::IOClient ) ),
m_pIoCtHandler( CDspHandlerRegistrator::instance().findHandler( IOSender::IOCtClient ) ),
m_pHwMonitorThread( new CDspHwMonitorThread ),
m_pSystemEventsMonitor( new CDspSystemEventsMonitor ),
m_pHaClusterHelper( new CDspHaClusterHelper ),
m_pTaskManager(new CDspTaskManager()),
m_strHostOsVersion ( CDspHostInfo::GetOsVersionStringRepresentation() )
{
	qRegisterMetaType<SmartPtr<NATStatistic> >("SmartPtr<NATStatistic>");
	qRegisterMetaType<SmartPtr<CDspClient> >("SmartPtr<CDspClient>");
	qRegisterMetaType<SmartPtr<IOPackage> >("SmartPtr<IOPackage>");
	qRegisterMetaType<VIRTUAL_MACHINE_STATE >("VIRTUAL_MACHINE_STATE");
#ifdef ETRACE
	CEtraceStatic::get_instance()->init(true);
#endif

	m_registry.reset(new Registry::Actual(*this));
	m_vmDirHelper.reset(new CDspVmDirHelper(*m_registry));
	m_vmMigrateHelper.reset(new CDspVmMigrateHelper(*m_registry));
	m_pReconnectTimer = new QTimer(this);
	m_pReconnectTimer->setSingleShot(true);

	Backup::Task::Launcher b(*m_registry, m_pTaskManager, m_backup);
	m_AppSettings.init(QCoreApplication::applicationName());
	m_clientManager.reset(new CDspClientManager(*this, b));
	m_dispConnectionsManager.reset(new CDspDispConnectionsManager(*this, b));
#ifdef _CT_
	m_pVzHelper = SmartPtr<CDspVzHelper>(new CDspVzHelper(*this, b));
#endif

	// initialize storage with NULL, it will be created on demand
	m_base_perfstorage.storage = NULL ;

	m_strServerUuidFromCorruptedDispConfig.clear();
	PRL_ASSERT( m_pVmManagerHandler && dynamic_cast<CDspVmManager*>( m_pVmManagerHandler.getImpl()) );
	PRL_ASSERT( m_pIoCtHandler&& dynamic_cast<CDspIOCtClientHandler *>( m_pIoCtHandler.getImpl()) );


	// Handle stop from main thread
	bool bConnected = QObject::connect( this,
		SIGNAL(onDoStopFromMainThread()),
		SLOT(stopFromMainThread()),
		Qt::QueuedConnection );
	PRL_ASSERT(bConnected);

	// Handle any addr start/stop signals
	bConnected = QObject::connect( this,
								  SIGNAL(onStartOrStopListeningAnyAddr(bool)),
								  SLOT(startOrStopListeningAnyAddr(bool)),
								  Qt::BlockingQueuedConnection );
	PRL_ASSERT(bConnected);

	bConnected = QObject::connect( this,
								  SIGNAL(onUpCustomListeningInterface(const QString &)),
								  SLOT(upCustomListeningInterface(const QString &)),
								  Qt::BlockingQueuedConnection);
	PRL_ASSERT(bConnected);

	bConnected = connect( this, SIGNAL(cleanupOnUserSessionDestroy(QString))
						 , SLOT(onCleanupOnUserSessionDestroy(QString)), Qt::QueuedConnection );
	PRL_ASSERT(bConnected);

	enableConfigsCrashSafeMech();
}

CDspService::~CDspService ()
{
	stop(SM_FORCE_STOP);
#ifdef ETRACE
	CEtraceStatic::get_instance()->deinit();
#endif
}

void CDspService::enableConfigsCrashSafeMech()
{
	getDispConfigGuard().enableCrashSafeMech();
	getNetworkConfig()->enableCrashSafeSaving();
	getVmDirManager().enableCrashSafeMech();
}


IOSendJob::Handle CDspService::sendSimpleResponseToClient (
	const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p,
	PRL_RESULT rc )
{
	if ( m_ioServerPool->clientSenderType(h) != IOSender::Client )
		return IOSendJob::Handle();

	CProtoCommandPtr pResponse =
		CProtoSerializer::CreateDspWsResponseCommand( p, rc );
	SmartPtr<IOPackage> response =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );
	return convey(m_ioServerPool->sendPackage( h, response ), *m_ioServerPool);
}

IOSendJob::Handle CDspService::sendSimpleResponseToDispClient (
	const IOSender::Handle &h,
	const SmartPtr<IOPackage> &pRequestPkg,
	PRL_RESULT nRetCode
	)
{
	if ( m_ioServerPool->clientSenderType(h) != IOSender::Dispatcher )
		return IOSendJob::Handle();

	CDispToDispCommandPtr pResponse =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand( nRetCode, pRequestPkg );
	SmartPtr<IOPackage> response =
		DispatcherPackage::createInstance( DispToDispResponseCmd, pResponse, pRequestPkg );
	return convey(m_ioServerPool->sendPackage( h, response ), *m_ioServerPool);
}

IOSendJob::Handle CDspService::sendResponse(const IOSender::Handle &h, const CProtoCommandPtr &pResponse, const SmartPtr<IOPackage> &pRequestPkg)
{
	if ( m_ioServerPool->clientSenderType(h) != IOSender::Client )
		return IOSendJob::Handle();

	SmartPtr<IOPackage> pResponsePkg =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, pRequestPkg );
	return convey(m_ioServerPool->sendPackage(h, pResponsePkg), *m_ioServerPool);
}

IOSendJob::Handle CDspService::sendResponseToDispClient(
	const IOSender::Handle &h,
	const CDispToDispCommandPtr &pResponse,
	const SmartPtr<IOPackage> &pRequestPkg
	)
{
	if ( m_ioServerPool->clientSenderType(h) != IOSender::Dispatcher )
		return IOSendJob::Handle();

	SmartPtr<IOPackage> pResponsePkg =
		DispatcherPackage::createInstance( DispToDispResponseCmd, pResponse, pRequestPkg );
	return convey(m_ioServerPool->sendPackage(h, pResponsePkg), *m_ioServerPool);
}

IOServerPool& CDspService::getIOServer ()
{
	PRL_ASSERT( m_ioServerPool );
	return *m_ioServerPool.getImpl();
}

IOServerInterface& CDspService::getListeningIOServer ()
{
	PRL_ASSERT( m_ioListeningServer );
	return *m_ioListeningServer.getImpl();
}

CDspTaskManager& CDspService::getTaskManager ()
{
	return *m_pTaskManager.getImpl();
}

CDspVmConfigManager & CDspService::getVmConfigManager()
{
	return m_configManager;
}

CDspVmConfigurationChangesWatcher & CDspService::getVmConfigWatcher ()
{
	return (*m_pVmConfigWatcher.getImpl());
}

CDspUserHelper& CDspService::getUserHelper ()
{
	return *m_pUserHelper.getImpl();
}

CDspDispConfigGuard& CDspService::getDispConfigGuard ()
{
	return m_dispConfigGuard;
}

CFeaturesMatrix CDspService::getFeaturesMatrix()
{
	return m_FeaturesMatrix;
}

CDspLockedPointer<CParallelsNetworkConfig> CDspService::getNetworkConfig()
{
	return CDspLockedPointer< CParallelsNetworkConfig > (& m_networkConfigMutex, &m_networkConfig );

}

CDspLockedPointer<QSettings> CDspService::getQSettings()
{
	return m_AppSettings.getPtr();
}

CDspLockedPointer<CDspHostInfo> CDspService::getHostInfo ()
{
	return CDspLockedPointer<CDspHostInfo>(&m_hostInfoMutex, &m_hostInfo);
}

CDspVmDirManager& CDspService::getVmDirManager ()
{
	return m_vmDirManager;
}

CDspAccessManager& CDspService::getAccessManager ()
{
	return m_vmAccessManager;
}

CDspClientManager& CDspService::getClientManager()
{
	PRL_ASSERT(!m_clientManager.isNull());
	return *m_clientManager;
}

CDspVmManager&	CDspService::getVmManager ()
{
	PRL_ASSERT( m_pVmManagerHandler );

	CDspVmManager* pVmManager = dynamic_cast<CDspVmManager*>( m_pVmManagerHandler.getImpl() );
	PRL_ASSERT( pVmManager );

	return *pVmManager;
}

CDspIOCtClientHandler& CDspService::getIoCtClientManager ()
{
	PRL_ASSERT( m_pIoCtHandler );

	CDspIOCtClientHandler* pIoCtClientManager = dynamic_cast<CDspIOCtClientHandler*>( m_pIoCtHandler.getImpl() );
	PRL_ASSERT( pIoCtClientManager);

	return *pIoCtClientManager;
}

CDspVmDirHelper& CDspService::getVmDirHelper ()
{
	return *m_vmDirHelper;
}

CDspVmSnapshotStoreHelper& CDspService::getVmSnapshotStoreHelper ()
{
	return m_vmSnapshotStoreHelper;
}

CDspShellHelper& CDspService::getShellServiceHelper ()
{
	return m_shellHelper;
}

CDspMonitor& CDspService::getDispMonitor ()
{
	return m_dispMonitor;
}

CDspVmMigrateHelper& CDspService::getVmMigrateHelper ()
{
	return *m_vmMigrateHelper;
}

#ifdef _CT_
SmartPtr<CDspVzHelper> CDspService::getVzHelper()
{
	return m_pVzHelper;
}
#endif

SmartPtr<CDspHaClusterHelper> CDspService::getHaClusterHelper()
{
	return m_pHaClusterHelper;
}

CDspHwMonitorThread &CDspService::getHwMonitorThread()
{
	return (*m_pHwMonitorThread.getImpl());
}

CDspLockedPointer<CDspVmStateSender> CDspService::getVmStateSender()
{
	PRL_ASSERT( m_pVmStateSenderThread );
	if( !m_pVmStateSenderThread )
	{
		static QMutex mtx;
		return CDspLockedPointer<CDspVmStateSender>( &mtx, 0 );
	}
	return m_pVmStateSenderThread->getVmStateSender();
}

#ifdef ETRACE
CDspLockedPointer<CEtraceStatic> CDspService::getEtrace()
{
	return CDspLockedPointer<CEtraceStatic>(&m_etraceMutex, CEtraceStatic::get_instance());
}
#endif


void CDspService::startOrStopListeningAnyAddr ( bool listenAnyAddr )
{
	// Always run from service thread
	if ( QThread::currentThread() !=  QObject::thread() ) {
		emit onStartOrStopListeningAnyAddr( listenAnyAddr );
		return;
	}

	// Check that we are not in server mode
	if ( isServerMode() ) {
		WRITE_TRACE(DBG_FATAL, "Can't start/stop listening any addr in server mode!");
		return;
	}

#ifdef _WIN_
	// We can't start/stop listening any addr on windows platform to prevent disconnect user sessions.
	// On unix we use unix-sockets to local sessions instead tcp loopback interface as on windows.
	WRITE_TRACE(DBG_FATAL, "Can't start/stop listening any addr on windows platform!");
	return;
#endif

	// Start listening any addr
	if ( listenAnyAddr )
	{
		// #484389
		// stop to prevent fail when loopback interface was already added by CDspService::upCustomListeningInterface()
		stopListeningAnyAddr();

		quint32 listenPort = getDispConfigGuard().getDispWorkSpacePrefs()->getDispatcherPort();
		PRL_SECURITY_LEVEL
			securityLevel = getDispConfigGuard().getDispWorkSpacePrefs()->getMinimalSecurityLevel();

		SmartPtr<IOServerInterface> pServer = SmartPtr<IOServerInterface>(setup(new IOServer(
				IORoutingTableHelper::GetServerRoutingTable(securityLevel),
				IOSender::Dispatcher,
				IOService::AnyAddr, listenPort )));

		// Append to server pool
		bool res = m_ioServerPool->addServer( pServer );
		PRL_ASSERT(res);

		PRL_ASSERT(!m_ioAnyAddrServer);
		m_ioAnyAddrServer = pServer;

		// Start listening
		IOSender::State st = pServer->listen();

		WRITE_TRACE(DBG_FATAL, "Any addr server on state=%d added ", st);
		if ( st != IOSender::Connected )
		{
			pServer->disconnectServer();
			PRL_ASSERT(m_ioAnyAddrServer == pServer);
			m_ioAnyAddrServer = SmartPtr<IOServerInterface>(0);
			m_ioServerPool->removeServer( pServer );
		}
		else
		{
			setCredentialsForServers(IOServerList() << m_ioAnyAddrServer);
		}
	}
	// Stop listening any addr
	else {
		stopListeningAnyAddr();
	}
}

void CDspService::stopListeningAnyAddr ()
{
	// Always run from service thread
	PRL_ASSERT( QThread::currentThread() ==  QObject::thread() );

	if(!m_ioAnyAddrServer)
		return;

	m_ioAnyAddrServer->disconnectServer();
	m_ioServerPool->removeServer( m_ioAnyAddrServer );
	m_ioAnyAddrServer = SmartPtr<IOServerInterface>(0);
}

void CDspService::upCustomListeningInterface( const QString &sListenAddr )
{
	// Always run from service thread
	if ( QThread::currentThread() !=  QObject::thread() ) {
		emit onUpCustomListeningInterface( sListenAddr );
		return;
	}

	if( m_ioAnyAddrServer )
	{
			WRITE_TRACE(DBG_FATAL, "Any addr already listening");
			return;
	}

	// Start listening custom addr
	quint32 listenPort = getDispConfigGuard().getDispWorkSpacePrefs()->getDispatcherPort();
	PRL_SECURITY_LEVEL
		securityLevel = getDispConfigGuard().getDispWorkSpacePrefs()->getMinimalSecurityLevel();

	QHash<QString, QString> addrInUse;
	foreach( SmartPtr<IOServerInterface> pServer, m_ioServerPool->getServers())
	{
		IPvBindingInfo binding;
		PRL_ASSERT(pServer.isValid());
		pServer->serverBindingInfo( binding );
		foreach ( IPVersion key, binding.keys() ) {
			QPair<QString, quint16>& addr = binding[key];
			addrInUse[addr.first] = addr.first;
		}
	}

	if ( sListenAddr == IOService::LoopbackAddr )
	{
		if ( addrInUse.contains( "127.0.0.1" ) || addrInUse.contains( "::1" ) )
		{
			WRITE_TRACE(DBG_FATAL, "Localhost already listening");
			return;
		}
	}
	if ( addrInUse.contains( sListenAddr ) )
	{
		WRITE_TRACE(DBG_FATAL, "Address '%s' already listening", qPrintable(sListenAddr));
		return;
	}

	SmartPtr<IOServerInterface> server = SmartPtr<IOServerInterface>(setup(new IOServer(
				IORoutingTableHelper::GetServerRoutingTable(securityLevel),
				IOSender::Dispatcher,
				sListenAddr, listenPort )));

	// Append to server pool
	bool res = m_ioServerPool->addServer( server );
	PRL_ASSERT(res);

	// Start listening
	IOSender::State st = server->listen();

	WRITE_TRACE(DBG_FATAL, "Any addr server on addr=%s, state=%d", qPrintable(sListenAddr), st);
	if ( st != IOSender::Connected ) {
		server->disconnectServer();
		m_ioServerPool->removeServer( server );
		WRITE_TRACE(DBG_FATAL, "Listen operation failed for address '%s'", qPrintable(sListenAddr));
		return;
	}

	PRL_ASSERT(!m_ioAnyAddrServer);
	m_ioAnyAddrServer = server;

	WRITE_TRACE(DBG_FATAL, "Successfully up interface on address '%s'", qPrintable(sListenAddr == IOService::LoopbackAddr ? QString("localhost") : sListenAddr));
}

void CDspService::checkAndDisableFirewall(bool)
{
	// Check mode, should be workstation
}

void CDspService::start ()
{
	::qsrand( IOService::msecsFromEpoch() );

	m_bServerStopping = false;

	COMMON_TRY
	{
		m_serviceStartTime = IOService::msecsFromEpoch();
		m_serviceStartTimeMonotonic = PrlGetTickCount64();
		printTimeStamp();

		m_pUserHelper = SmartPtr<CDspUserHelper>(new CDspUserHelper);
		m_pVmConfigWatcher = SmartPtr<CDspVmConfigurationChangesWatcher>(new CDspVmConfigurationChangesWatcher());
		m_pVmStateSenderThread = SmartPtr<CDspVmStateSenderThread>( new CDspVmStateSenderThread );
		bool bConnected;
		Q_UNUSED(bConnected);

		if( !init() )
		{
			PRL_ASSERT(CMainDspService::instance());
			CMainDspService::instance()->stop();
			WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher DOES NOT STARTED !!!" );
			return;
		}
#ifndef _WIN_
	// System signals handler.
	CUnixSignalHandler* pSigTermHandler = CUnixSignalHandler::installHandler(SIGTERM);
	if ( !pSigTermHandler )
		WRITE_TRACE( DBG_FATAL, "Can't setup handler for SIGTERM signal" );
	else
	{
		bConnected = QObject::connect( pSigTermHandler, SIGNAL(signalReceived()), SLOT(onTerminateSignalReceived()) );
		PRL_ASSERT(bConnected);
	}

	if( g_bSIGTERM_was_received )
	{
		PRL_ASSERT(CMainDspService::instance());
		CMainDspService::instance()->stop();
		WRITE_TRACE( DBG_FATAL, "SIGTERM was recieved before dispatcher object was inited." );
		return;
	}

#endif

		//Instantiate UDP listener after files initialization in order to prevent server answers with empty data
		if ( isServerMode() )// Up UDP interface just for server
			m_pBroadcastMsgsProcessingService = SmartPtr<CDspBroadcastListener>(new CDspBroadcastListener);

			precacheVmConfigs();
			checkVmPermissions();
			if ( CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->isVmConfigWatcherEnabled() )
			{
				getVmConfigWatcher().setEnabled(true);
				getVmConfigWatcher().update();
			}

			CDspStatCollectingThread::start(*m_registry);
			m_pHwMonitorThread->start( QThread::NormalPriority ); //QThread::LowPriority );

		m_pSystemEventsMonitor->startMonitor();

		m_bInitWasDone = true;
		if(	m_bStopWasSentOnInitPhase )
		{
			WRITE_TRACE(DBG_FATAL, "Server begin to stop: StopWasSentOnInitPhase=true" );
			PRL_ASSERT(CMainDspService::instance());
			CMainDspService::instance()->doStop();
		}

		m_nTimestampTraceTimerId = startTimer( 24*60*60*1000 );
	}
	COMMON_CATCH;

	if (m_bInitWasDone)
	{
		// #8380
		WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher started" );
	}
	else
		CMainDspService::instance()->doStop();
}

void CDspService::stop (CDspService::StopMode stop_mode)
{
	if( m_bServerStopping && stop_mode != SM_END_STOP )
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher now in stopping state. This 'stop' would be ignored." );
		return;
	}
	m_bServerStopping = true;

	switch(stop_mode)
	{
	case SM_BEGIN_STOP:
		if (m_bRebootHost)
		{
			// Send event that host will be done reboot
			CVmEvent event( PET_DSP_EVT_REBOOT_HOST,
							Uuid().toString(),
							PIE_DISPATCHER );

			SmartPtr<IOPackage> p =
				DispatcherPackage::createInstance( PVE::DspVmEvent, event );
			getClientManager().sendPackageToAllClients(p);
		}

		break;
	case SM_FORCE_STOP:
	case SM_END_STOP:
		break;
	}

	m_pHwMonitorThread->FinalizeThreadWork();
	m_pHwMonitorThread->wait();

	m_pSystemEventsMonitor->stopMonitor();
	m_pSystemEventsMonitor->wait();

	// Stops listening any addr
	stopListeningAnyAddr();

	m_dispMonitor.SendShutdownCompleteResponses( 3000 /* wait max 3 secs to send responses */ );

	// Stops listening server
	if( m_ioListeningServer ) {
		m_ioListeningServer->disconnectServer();
		m_ioServerPool->removeServer( m_ioListeningServer );
	}

#ifndef _WIN_
	// Stops listening AF_UNIX server
	if ( m_ioLocalUnixListeningServer )
	{
		m_ioLocalUnixListeningServer->disconnectServer();
		m_ioServerPool->removeServer( m_ioLocalUnixListeningServer );
	}
#endif

	wakeAllInitCompletedWaiters(false);

	CDspRouter::instance().cleanRoutes();
	CDspHandlerRegistrator::instance().cleanHandlers();
	CDspStatCollectingThread::stop();
	killTimer(m_nStopTimerId);



	// #PDFM-30119 Dispatcher crash during stopping when any vm was running
	// We call cleanupAllStaticObjects() twice because several tasks can create static objects
	//		(CDspVm objects in static hash for example )
	cleanupAllStaticObjects();

	m_pTaskManager->deinit();
	m_pTaskManager = SmartPtr<CDspTaskManager>(0);

	// #PDFM-30119 second call to clear all static object after tasks finished.
	cleanupAllStaticObjects();

	if ( m_pVmStateSenderThread->isRunning() )
	{
		m_pVmStateSenderThread->exit();
		WRITE_TRACE(DBG_FATAL, "Wait to stop VmStateSenderThread" );
		m_pVmStateSenderThread->wait();
		WRITE_TRACE(DBG_FATAL, "VmStateSenderThread finished" );
	}

	if( m_nTimestampTraceTimerId != -1)
		killTimer( m_nTimestampTraceTimerId );

	m_pVmConfigWatcher->unregisterAll();

	if (CDspService::isServerMode())
	{
#ifdef _CT_
		// stop vnc servers for container's
		getVzHelper()->removeAllCtVNCServer();
#endif
#ifdef _LIN_
#ifdef _LIBVIRT_
		if (!m_hypervisor.isNull())
		{
			m_hypervisor->quit();
			m_hypervisor->wait();
			m_hypervisor.reset();
		}
#endif // _LIBVIRT_
#endif
	}

	// #126543, #132528  fix to prevent deadlock on destroy QFileSystemWatcher.
	// delete all QFileSystemWatcher objects before stop main event loop
	m_pVmConfigWatcher = SmartPtr<CDspVmConfigurationChangesWatcher>(0);

#ifndef _WIN_
	// revert sighandler
	CUnixSignalHandler::removeHandler(SIGTERM);
#endif

	deinitSSLLibrary();

	QCoreApplication::exit(0);

	if (m_bRebootHost)
	{
		rebootHost();
	}
}

void CDspService::timerEvent(QTimerEvent* te)
{
	if( te->timerId() == m_nStopTimerId )
	{
		if (!getVmManager().hasAnyRunningVms() || m_nStopTimeout <= 0)
		{
			if (m_nStopTimeout <= 0)
			{
				WRITE_TRACE(DBG_FATAL, "Timeout %d seconds of stopping virtual machines is over!",
					getDispConfigGuard().getDispWorkSpacePrefs()->getVmTimeoutOnShutdown());
			}

			stop(SM_END_STOP);
		}
		else
		{
			m_nStopTimeout -= ST_STEP_TIMER_AT_STOPPING;
			stopServiceStatus(SP_STOP_PENDING);

			if ((m_nStopTimeout % 5000) == 0)
			{
				WRITE_TRACE(DBG_FATAL, "Stopping VMs in progress...");
			}
		}
	}
	else if( te->timerId() == m_nTimestampTraceTimerId )
		printTimeStamp();
}

void CDspService::printTimeStamp()
{
	WRITE_TRACE(DBG_FATAL, "DAILY TIMESTAMP: %s  ( version = %s )"
		, QSTR2UTF8( QDateTime::currentDateTime().toString( XML_DATETIME_FORMAT ) )
		, QSTR2UTF8( CVmConfiguration::makeAppVersion() )
		);
}

bool CDspService::isServerStopping() const
{
	return m_bServerStopping;
}

void CDspService::prepareRebootHost ()
{
	m_bRebootHost = true;
}

void CDspService::rebootHost()
{
	WRITE_TRACE(DBG_FATAL, "Reboot host now!");
#ifdef _WIN_

	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	// Get a token for this process.

	if ( ! OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
	{
		WRITE_TRACE(DBG_FATAL, "Cannot open process token for adjust privileges. Error %d.", GetLastError());
		return;
	}

	// Get the LUID for the shutdown privilege.

	if ( ! LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid) )
	{
		WRITE_TRACE(DBG_FATAL, "Cannot look up privilege value. Error %d.", GetLastError());
		return;
	}

	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get the shutdown privilege for this process.

	if ( ! AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES )NULL, 0) )
	{
		DWORD dwLastError = GetLastError();
		if (dwLastError != ERROR_SUCCESS)
		{
			WRITE_TRACE(DBG_FATAL, "Cannot set shutdown privilage. Error %d.", dwLastError);
			return;
		}
	}

   // Shut down the system and force all applications to close.

	BOOL bRes = ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, SHTDN_REASON_MAJOR_OTHER /* == 0 */);
	if ( ! bRes )
	{
		WRITE_TRACE(DBG_FATAL, "Cannot reboot host! Last error %d.", GetLastError());
	}
#else
	if (system("nohup /sbin/shutdown -r now &") < 0)
		WRITE_TRACE(DBG_FATAL, "Cannot reboot host! Last error %d.", errno);
#endif
}

void CDspService::onTerminateSignalReceived()
{
	WRITE_TRACE(DBG_FATAL, "SIGTERM was caught by handler2. Stop started..." );
	doStopFromMainThread();
}

void CDspService::doStopFromMainThread ()
{
	if( m_bInitWasDone )
		emit onDoStopFromMainThread();
	else
		m_bStopWasSentOnInitPhase = true;
}

void CDspService::stopFromMainThread ()
{
	PRL_ASSERT( QThread::currentThread() ==
		QCoreApplication::instance()->thread() );
	PRL_ASSERT(CMainDspService::instance());
	CMainDspService::instance()->doStop();
}

#ifdef _WIN_
static DWORD WINAPI HandlerEx(DWORD , DWORD , LPVOID , LPVOID )
{
	return NO_ERROR;
}
#endif

void CDspService::stopServiceStatus(StopPhase phase)
{
	// #112030 bug
#ifdef _WIN_

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot open service manager handle! Error: %d", GetLastError());
		return;
	}

	DWORD cbBytesNeeded = 0;
	DWORD dwServicesReturned = 0;
	DWORD dwResumeHandle = 0;

	EnumServicesStatus(	hSCManager,
		SERVICE_WIN32,
		SERVICE_STATE_ALL,
		NULL,
		0,
		&cbBytesNeeded,
		&dwServicesReturned,
		&dwResumeHandle);

	LPENUM_SERVICE_STATUS lpServices = new ENUM_SERVICE_STATUS[cbBytesNeeded];

	try
	{
		if (!EnumServicesStatus(hSCManager,
			SERVICE_WIN32,
			SERVICE_STATE_ALL,
			lpServices,
			cbBytesNeeded,
			&cbBytesNeeded,
			&dwServicesReturned,
			&dwResumeHandle
			))
		{
			throw "enumeration service status error";
		}

		PRL_ASSERT(CMainDspService::instance());
		QString qsServiceName = CMainDspService::instance()->serviceName();
		LPENUM_SERVICE_STATUS pService = 0;
		for(DWORD i = 0; i < dwServicesReturned; ++i)
		{
			if (UTF16_2QSTR(lpServices[i].lpServiceName) == qsServiceName)
			{
				pService = &lpServices[i];
				break;
			}
		}

		if (!pService)
		{
			throw "dispatcher is not service";
		}

		DWORD dwData = 0;
		SERVICE_STATUS_HANDLE hServiceStatus =
			RegisterServiceCtrlHandlerEx(pService->lpServiceName, HandlerEx, &dwData);

		if (hServiceStatus)
		{
			switch(phase)
			{
			case SP_START_STOPPING:
				pService->ServiceStatus.dwWaitHint =
					(DWORD )(1000 * getDispConfigGuard().getDispCommonPrefs()
					->getWorkspacePreferences()->getVmTimeoutOnShutdown());
				pService->ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
				pService->ServiceStatus.dwCheckPoint = 1;
				pService->ServiceStatus.dwWin32ExitCode = NO_ERROR;
				break;
			case SP_STOP_PENDING:
				pService->ServiceStatus.dwCheckPoint++;
				break;
			}

			if (!SetServiceStatus(hServiceStatus, &pService->ServiceStatus))
			{
				throw "cannot set service status";
			}
		}
		else
		{
			throw "cannot register service status handler";
		}
	}
	catch(const char* reason)
	{
		WRITE_TRACE(DBG_FATAL, "Service error: %s. Error: %d", reason, GetLastError());
	}

	delete [] lpServices;
	CloseServiceHandle(hSCManager);
#else
	Q_UNUSED(phase);
#endif
}

bool CDspService::init()
{
	try
	{
		if (! initSSLLibrary())
			throw 0;

		if ( ! setupDispEnv() )
			throw 0;

		TestConfig::instance();

		if ( ! initAllConfigs() && ! recoverAllConfigs() )
			throw 0;

		initFeaturesList();

		initVmStateSender();

		// It should be done before vm start and after load dispatcher.xml
		initPlugins();

		if( ! initIOServer() )
			throw 0;

		if( ! initHostInfo() )
			throw 0;

		// update Dispatcher's config with latest host info
		updateDispConfig();

		initNetworkConfig();

#ifdef _CT_
		if ( isServerMode() )
		{
			getVmDirManager().initVzDirCatalogue();
			getVmDirManager().initTemplatesDirCatalogue();
			getVzHelper()->initVzStateMonitor();
		}
#endif

#ifdef _LIBVIRT_
		::Vm::Directory::Dao::Locked d(getVmDirManager());
		foreach (const ::Vm::Directory::Item::List::value_type& i, d.getItemList())
		{
			m_registry->declare(MakeVmIdent(i.second->getVmUuid(), i.first),
				i.second->getVmHome());
			m_registry->define(i.second->getVmUuid());
		}
#else
		patchDirCatalogue();
#endif // _LIBVIRT_

		if( isServerMode() )
		{
			initNetworkShapingConfiguration();
			initPrivateNetworks();
		}

		initSyncVmUptimeTask();

		// patch vm configurations ( only when dispatcher version chnaged )
		if( isDispVersionChanged() )
		{
			CDspBugPatcherLogic logic( *CDspService::instance()->getHostInfo()->data() );
			logic.patchVmConfigs();
		}

		initHypervisor(); // before start any vm!

		autoStartCt();

		m_bFirstInitPhaseCompleted = true;
	}
	catch ( ... )
	{
		WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher initialization FAILED !!!" );
	}

	if( m_bFirstInitPhaseCompleted )
		WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher initialization finished" );

	wakeAllInitCompletedWaiters( true );

	return m_bFirstInitPhaseCompleted;
}

void CDspService::initFeaturesList()
{
	QSet<PRL_FEATURES_MATRIX> _features;

		_features.insert( PFSM_IPV6_SUPPORT );
		_features.insert( PFSM_VM_CONFIG_MERGE_SUPPORT );
		if( getDispConfigGuard().getDispWorkSpacePrefs()->isDefaultPlainDiskAllowed() )
			_features.insert( PFSM_DEFAULT_PLAINDISK_ALLOWED );
		_features.insert( PFSM_NIC_CHANGE_ALLOWED );
		if( isServerMode() )
		{
			_features.insert( PFSM_SATA_HOTPLUG_SUPPORT );
			_features.insert( PFSM_AUTOSTART_VM_AS_OWNER );
		}
		else
			_features.insert( PFSM_USB_PRINTER_SUPPORT );

	if( ParallelsDirs::isServerModePSBM() )
	{
		_features.insert( PFSM_PSBM5 );
		_features.insert( PFSM_NO_SHARED_NETWORKING );
		_features.insert( PFSM_DISK_IO_LIMITS );
		_features.insert( PFSM_FINE_CPU_LIMITS );
		_features.insert( PFSM_ROUTED_NETWORKING );
		_features.insert( PFSM_RAM_HOTPLUG_SUPPORT );
		_features.insert( PFSM_CPU_HOTPLUG_SUPPORT );
	}

	m_FeaturesMatrix.Initialize( _features );
}

bool CDspService::initIOServer()
{
	PRL_ASSERT( ! m_ioServerPool );
	if (!CDspHandlerRegistrator::instance().registerHandler(SmartPtr<CDspHandler>
		(m_clientManager.data(), SmartPtrPolicy::DoNotReleasePointee)))
	{
		WRITE_TRACE(DBG_FATAL, "client manager registration failed!");
		return false;
	}
	if (!CDspHandlerRegistrator::instance().registerHandler(SmartPtr<CDspHandler>
		(m_dispConnectionsManager.data(), SmartPtrPolicy::DoNotReleasePointee)))
	{
		WRITE_TRACE(DBG_FATAL, "disp-disp connections manager registration failed!");
		return false;
	}
	quint32 listenPort = getDispConfigGuard().getDispWorkSpacePrefs()->getDispatcherPort();
	PRL_SECURITY_LEVEL
		securityLevel = getDispConfigGuard().getDispWorkSpacePrefs()->getMinimalSecurityLevel();

	createIOServers( listenPort, securityLevel );

	CDspHandlerRegistrator::instance().doHandlersInit();

	// Listen should be called after doHandlersInit, because of
	// possible reset of routing table for IO server union
	if ( m_ioListeningServer )
	{
		IOSender::State state = m_ioListeningServer->listen();
		if ( state != IOSender::Connected )
		{
			WRITE_TRACE(DBG_FATAL, "Listen operation failed!");
			return false;
		}
	}

#ifndef _WIN_
	if ( m_ioLocalUnixListeningServer )
	{
		IOSender::State state = m_ioLocalUnixListeningServer->listen();
		if ( state != IOSender::Connected )
		{
			WRITE_TRACE(DBG_FATAL, "Listen operation for local UNIX socket failed!");
			return false;
		}
	}
#endif

	// Start listening to any addr if enabled
	bool anyAddr = getDispConfigGuard().getDispWorkSpacePrefs()->isListenAnyAddr();
	if ( ! isServerMode() && anyAddr )
		startOrStopListeningAnyAddr( anyAddr );

	// Enable or disable firewall and block/unblock incoming connections,
	// depending on "AllowMultiplePMC" option
	bool bAllowPmcMc = getDispConfigGuard().getDispWorkSpacePrefs()->isAllowMultiplePMC();
	checkAndDisableFirewall(bAllowPmcMc);

	WRITE_TRACE( DBG_FATAL, "Dispatcher is ready to accept incoming connections." );

	return true;
}

bool CDspService::initHostInfo()
{
	///////////////////////////////
	// STEP-1: Wait drivers loaded for PDFM as single bundle application
	///////////////////////////////

	// NOTE-1: We need load kexts before start HostInfo collection because HostInfo logic communicate with Hypervisor.

	if( g_bSIGTERM_was_received )
	{
		WRITE_TRACE( DBG_FATAL, "SIGTERM was recieved on initHostInfo(), before dispatcher object was inited." );
		return false;
	}

	///////////////////////////////
	// STEP-2: Get HostInfo
	///////////////////////////////
	WRITE_TRACE( DBG_FATAL, "initHostInfo started." );

	if( getDispConfigGuard().getDispCommonPrefs()->getPciPreferences()->isPrimaryVgaAllowed() )
		getHostInfo()->setRefreshFlags( CDspHostInfo::rfShowPrimaryPciVga );

	quint64	nInitFlags = HI_UPDATE_ALL;
	getHostInfo()->updateData( nInitFlags );
	WRITE_TRACE( DBG_FATAL, "initHostInfo finished." );
	return true;
}

void CDspService::initVmStateSender()
{
	m_pVmStateSenderThread->start( QThread::HighPriority );
}

void CDspService::initPlugins()
{
	if( !getDispConfigGuard().getDispWorkSpacePrefs()->isPluginsAllowed() )
		return;
}

void CDspService::initNetworkShapingConfiguration()
{
#ifdef _CT_
	// Sync with Virtuozzo shaping configuration on Dispatcher start
	CDspLockedPointer<CParallelsNetworkConfig> pNetCfg = CDspService::instance()->getNetworkConfig();
	if ( ! pNetCfg.isValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Network config doesn't exist!" );
		return;
	}
	CNetworkClassesConfig *pClassesCfg = pNetCfg->getNetworkClassesConfig();
	if (CVzHelper::get_network_classes_config(*pClassesCfg))
		return;

	CNetworkShapingConfig *pShapingCfg = pNetCfg->getNetworkShapingConfig();
	if (CVzHelper::get_network_shaping_config(*pShapingCfg))
		return;

	WRITE_TRACE(DBG_FATAL, "Init network shaping configuration");
	PRL_RESULT prlResult = PrlNet::WriteNetworkConfig(*pNetCfg.getPtr());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", (unsigned)prlResult);
	}
#endif
}

void CDspService::initPrivateNetworks()
{
#ifdef _CT_
	CDspLockedPointer<CParallelsNetworkConfig> pNetCfg = CDspService::instance()->getNetworkConfig();
	if ( ! pNetCfg.isValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Network config doesn't exist!" );
		return;
	}

	WRITE_TRACE(DBG_INFO, "Init private networks configuration");
	CPrivateNetworks *pNetworks = pNetCfg->getPrivateNetworks();
	foreach(CPrivateNetwork *pNetwork, pNetworks->m_lstPrivateNetwork)
	{
		PRL_RESULT prlResult = CVzPrivateNetwork::UpdatePrivateNetwork(pNetwork);
		if (PRL_FAILED(prlResult))
		{
			WRITE_TRACE(DBG_FATAL, "Failed to set up IP private network '%s',"
				" error = %x", QSTR2UTF8(pNetwork->getName()), prlResult);
		}
	}
#endif
}

void CDspService::initHypervisor()
{
#ifdef _LIBVIRT_
	if (!m_hypervisor.isNull())
		return;

	m_hypervisor.reset(new Libvirt::Host(*m_registry));
	m_hypervisor->moveToThread(m_hypervisor.data());
	m_hypervisor->QThread::start();
#endif // _LIBVIRT_
}

void CDspService::precacheVmConfigs()
{
		// Precache mode
		// Bug 116916 - "cold PD4.0 app start takes 2x longer then Fusion"
		QHash< SmartPtr<CVmConfiguration>, QString > vmList;

		SmartPtr<CDspClient> pFakeUserSession = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
		pFakeUserSession->getAuthHelper().AuthUserBySelfProcessOwner();

		QMultiHash< QString, SmartPtr<CVmConfiguration> > vmHash =
			CDspService::instance()->getVmDirHelper().getAllVmList();
		foreach ( QString vmDirUuid, vmHash.uniqueKeys() )
		{
			foreach( SmartPtr<CVmConfiguration> pVmConfig, vmHash.values( vmDirUuid ) )
			{
				PRL_ASSERT( pVmConfig );
				if( !pVmConfig )
					continue;

				if (pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
					continue;

				CDspVmDirHelper::UpdateHardDiskInformation(pVmConfig);
			}
		}
}

void CDspService::autoStartCt()
{
	PRL_APPLICATION_MODE mode = ParallelsDirs::getAppExecuteMode();
	if( mode != PAM_SERVER )
		return;

	SmartPtr<CDspClient> pUser( new CDspClient(IOSender::Handle()) );
	pUser->getAuthHelper().AuthUserBySelfProcessOwner();

	const SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdVmStart );
	CDspService::instance()->getTaskManager().schedule(new Task_AutoStart( pUser, p ));
}


void CDspService::initSyncVmUptimeTask()
{
	SmartPtr<CDspClient> pUser( new CDspClient(IOSender::Handle()) );
	pUser->getAuthHelper().AuthUserBySelfProcessOwner();

	const SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspCmdCtlDispatherFakeCommand );

	CDspService::instance()->getTaskManager()
		.schedule(new Task_SyncVmsUptime( pUser, p ));
}


bool CDspService::setupDispEnv ()
{
	QString strDispConfigDir = ParallelsDirs::getDispatcherConfigDir();
	QString strDefaultCommonVmCatalogueDir
		= ParallelsDirs::getCommonDefaultVmCatalogue();
	QString strDefaultSwapPathForVMOnNetworkShares = ParallelsDirs::getDefaultSwapPathForVMOnNetworkShares();

	// Helper for root users.
	CAuthHelper rootAuth;

	//////////////////////////////////////////////////////////////////////
	// create system config directory
	if ( ! checkExistAndCreateDirectory(strDispConfigDir, rootAuth,
		CDspService::permDispConfigDir) )
	{
		QString msg = QString( "\nCan't create parallels configs directory '%1'." )
			.arg( strDispConfigDir );
		printHorrorLogMessage( msg, PRL_ERR_FAILURE );

		return false;
	}

	//////////////////////////////////////////////////////////////////////
	// create default vm directory
	if ( ! checkExistAndCreateDirectory(strDefaultCommonVmCatalogueDir,
		rootAuth,
		CDspService::permVmDir) )
	{
		QString msg = QString( "\n Can't create parallels vm directory '%1'.\n %2\n %3" )
			.arg( strDefaultCommonVmCatalogueDir )
			.arg( "Need try recreate it manually" )
			.arg( "But Dispatcher will continue initialization.");
		printHorrorLogMessage( msg, PRL_ERR_SUCCESS );

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// create default swap vm directory
	if ( ! checkExistAndCreateDirectory(strDefaultSwapPathForVMOnNetworkShares,
		rootAuth,
		CDspService::permVmDir) )
	{
		QString msg = QString( "\n Can't create swap directory for VMs on network storage '%1'.\n %2\n %3" )
			.arg( strDefaultSwapPathForVMOnNetworkShares )
			.arg( "Need try recreate it manually" )
			.arg( "But Dispatcher will continue initialization.");
		printHorrorLogMessage( msg, PRL_ERR_SUCCESS );
	}
#ifdef _LIN_
	QStringList h;
	perf_enum_storages(&handle, &h, perf_get_storage_lock());
	perf_release_storage_lock();
	bool (* f)(const QString& ) = &QFile::remove;
	std::for_each(h.begin(), h.end(), f);
#endif // _LIN_
	return true;
}

void CDspService::patchDirCatalogue()
{
	QString sServerUuid = CDspDispConfigGuard::getServerUuid();
	QList<CVmIdent> lst;
	{
		SmartPtr<CDspClient> pFakeUserSession = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
		pFakeUserSession->getAuthHelper().AuthUserBySelfProcessOwner();
		Vm::Directory::Dao::Locked d;

		foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
		{
			if (i.second->getVmType() != PVT_VM)
				continue;

			SmartPtr<CVmConfiguration> pVmConfig( new CVmConfiguration() );
			PRL_RESULT res = CDspService::instance()->getVmConfigManager().loadConfig(pVmConfig,
					i.second->getVmHome(), pFakeUserSession);
			if (PRL_FAILED(res))
				continue;

			// set template mark in dir items
			if (pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
			{
				i.second->setTemplate(true);
				continue;
			}

#ifdef _CT_
			if ( !isServerMode() )
				continue;

			// cleanup Vm not registered on node
			if (sServerUuid == pVmConfig->getVmIdentification()->getServerUuid())
				continue;

			WRITE_TRACE(DBG_FATAL, "Owner check failed: unregister Vm %s path=%s serveruuid=%s",
					QSTR2UTF8(i.second->getVmUuid()),
					QSTR2UTF8(i.second->getVmHome()),
					QSTR2UTF8(pVmConfig->getVmIdentification()->getServerUuid()));
			lst += MakeVmIdent(i.second->getVmUuid(), i.first);
#endif
		}
	}
	foreach(CVmIdent vmIdent, lst)
	{
		CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(
				vmIdent.second, vmIdent.first);
	}
}

bool CDspService::initAllConfigs()
{
	bool ret = false;
	try
	{
		if( ! initDispConfig() )
			throw 0;

		if ( ! initVmDirCatalogue() )
			throw 0;

		if ( ! checkConfigsIntegrity() )
			throw 0;

		// #119311  save server uuid to permanent storage
		{
			CDspLockedPointer<QSettings>
				pSettings = CDspService::instance()->getQSettings();
			QString key = QString( "%1/%2" )
				.arg( isServerMode()? QSETTINGS_TAG_MODE_SERVER : QSETTINGS_TAG_MODE_DESKTOP )
				.arg( QSETTINGS_TAG_SERVER_UUID );

			QString server_id = getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid();
			PRL_ASSERT( !Uuid( server_id ).isNull() );

			pSettings->setValue( key, server_id	);
			pSettings->sync();
		}

			//////////////////////////////////////////////////////////////////////
			// create entry for swap directory for VMs on network shares/storages
			CDispWorkspacePreferences* pWorkspace = getDispConfigGuard().getDispConfig()->getDispatcherSettings()
				->getCommonPreferences()->getWorkspacePreferences();

			QString swapPath = pWorkspace->getSwapPathForVMOnNetworkShares();
			if ( swapPath.isEmpty() ){

				QString strDefaultSwapPathForVMOnNetworkShares =
									ParallelsDirs::getDefaultSwapPathForVMOnNetworkShares();

				WRITE_TRACE(DBG_FATAL, "SwapPathForVMOnNetworkShares is empty in dispatcher config,"
					" inited to default." );

				//add to dispatcher xml
				pWorkspace->setSwapPathForVMOnNetworkShares( strDefaultSwapPathForVMOnNetworkShares );
			}

		ret = true;
	}
	catch (...)
	{
		ret = false;
	}

	return ret;
}


bool CDspService::recoverAllConfigs()
{
	//////////////////////////////////////////////////////////////////////////
	// Now we only rename config files
	// Now: bug #5898  [Dispatcher should start in any case, even with invalid configs.]
	//	http://bugzilla/show_bug.cgi?id=5898
	// TODO: Implement as defined in bug #3201 http://bugzilla/show_bug.cgi?id=3201

	QString strDispConfigFile = ParallelsDirs::getDispatcherConfigFilePath();
	QString strVmDirCatalogueFile = ParallelsDirs::getDispatcherVmCatalogueFilePath();

	QString backupSuffix = makeBackupSuffix();

	QString warn_msg = QString( "Dispatcher can not load config files \n"
		" and try to start with default configurations.\n"
		" Current configs would be  renamed and stored with suffix %1.\n "
		"config1 = '%2'\n"
		"config2 = '%3'\n"
		).arg( backupSuffix )
		.arg( strDispConfigFile )
		.arg( strVmDirCatalogueFile );

	printHorrorLogMessage( warn_msg, PRL_ERR_FAILURE );

	if( QFile::exists(strDispConfigFile)
		&& !QFile::rename( strDispConfigFile, strDispConfigFile + backupSuffix ) )
	{
		WRITE_TRACE(DBG_FATAL, "ERROR in DispConfig recovering: Can't rename '%s' ==> '%s'"
			, QSTR2UTF8( strDispConfigFile )
			, QSTR2UTF8( strDispConfigFile + backupSuffix ) );
		return false;
	}

	if( QFile::exists(strVmDirCatalogueFile)
		&& ! QFile::rename( strVmDirCatalogueFile, strVmDirCatalogueFile + backupSuffix ) )
	{
		WRITE_TRACE(DBG_FATAL, "ERROR in VmDirConfig recovering: Can't rename '%s' ==> '%s'"
			, QSTR2UTF8( strVmDirCatalogueFile )
			, QSTR2UTF8( strVmDirCatalogueFile + backupSuffix ) );
		return false;
	}
	// try to load server uuid from corrupted config
	QString strParamData;
	CFileHelper::ReadfromCorruptedXmlFileParam(strDispConfigFile + backupSuffix,XML_VM_CONFIG_ND_SERVERID,strParamData);
	QUuid ss(strParamData);
	bool isValid = !ss.isNull();
	if(isValid)
		m_strServerUuidFromCorruptedDispConfig = strParamData;
	else
		m_strServerUuidFromCorruptedDispConfig.clear();

	return initAllConfigs();
}

bool CDspService::checkVmPermissions()
{
	SmartPtr<CDspClient> pFakeUserSession = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
	pFakeUserSession->getAuthHelper().AuthUserBySelfProcessOwner();
	Vm::Directory::Dao::Locked d;
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		PRL_SEC_AM ownerAccess = 0;
		PRL_SEC_AM othersAccess = 0;
		bool flgOthersAccessIsMixed = false;
#ifdef _CT_
		if (i.second->getVmType() != PVT_VM)
			continue;
#endif

		PRL_RESULT res = getAccessManager()
			.getFullAccessRightsToVm( i.second, ownerAccess, othersAccess, flgOthersAccessIsMixed, pFakeUserSession->getAuthHelper() );

		if( PRL_FAILED( res ) )
		{
			WRITE_TRACE(DBG_FATAL, "getFullAccessRightsToVm() failed: %#x(%s) vm_path = '%s'"
				, res
				, PRL_RESULT_TO_STRING( res )
				, QSTR2UTF8( i.second->getVmHome() )
				);
			continue;
		}

		if( flgOthersAccessIsMixed )
		{
			WRITE_TRACE(DBG_FATAL, "VM permission for 'others'(= %#o) IS MIXED. VM=( vm_name='%s' vm_uuid=%s, path='%s' )."
				, othersAccess
				, QSTR2UTF8( i.second->getVmName() )
				, QSTR2UTF8( i.second->getVmUuid() )
				, QSTR2UTF8( i.second->getVmHome() )
				);
		}// if flgOthersAccessIsMixed
	}//foreach( CVmDirectory*

	return true;
}

bool CDspService::isFirstInitPhaseCompleted() const
{
	return m_bFirstInitPhaseCompleted;
}

void	CDspService::wakeAllInitCompletedWaiters( bool bCompleted )
{
	QWriteLocker locker( &m_rwlWaitForInitCompletion );
	m_bWaitForInitCompletion = bCompleted;
	m_wcWaitForInitCompletion.wakeAll();
}

bool CDspService::waitForInitCompletion()
{
	if ( m_bWaitForInitCompletion )
		return m_bFirstInitPhaseCompleted;

	QReadLocker	locker( &m_rwlWaitForInitCompletion );

	if ( m_bWaitForInitCompletion )
		return m_bFirstInitPhaseCompleted;

	WRITE_TRACE(DBG_FATAL, "Wait until service initialization finished" );
	if ( ! m_wcWaitForInitCompletion
			.wait( &m_rwlWaitForInitCompletion, WAIT_FOR_INIT_COMPLETION_TIMEOUT ) )
	{
		WRITE_TRACE(DBG_FATAL, "Timeout %d is over ! Service initialization was not done !"
			, WAIT_FOR_INIT_COMPLETION_TIMEOUT );
		return false;
	}
	WRITE_TRACE(DBG_FATAL, "Service initialization finished. Continue. "
		"FirstInitPhaseCompleted = %d", m_bFirstInitPhaseCompleted );

	return m_bFirstInitPhaseCompleted;
}

bool CDspService::initDispConfig ()
{
	////////////////////////////////////////////////////////////////////////
	//
	// We expect to see Dispatcher's configuration XML file in
	// $PARALLELS_CONFIG_DIR/dispatcher.xml,
	// where $PARALLELS_CONFIG_DIR is the environment variable, which points
	// to the Parallels home directory. If $PARALLELS_CONFIG_DIR is not set,
	// or/and assume default path, as it
	// is returned by ParallelsDirs::getDispatcherConfigDir() + DISPATCHER_PARALLELS_DIR_NAME.
	//
	////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////
	// Check Parallels home directory
	////////////////////////////////////////////////////////////////////////

	try
	{
		// disp config file name
		QString strDispConfigFile =
			ParallelsDirs::getDispatcherConfigFilePath();

		// check Dispatcher's config XML exists
		QFile f_in_cfg( strDispConfigFile );
		bool bDispConfExists = f_in_cfg.exists();
		if( !bDispConfExists && !createDispConfig() )
		{
			WRITE_TRACE(DBG_FATAL, "Can't create Parallels Dispatcher's configuration file %s",
				strDispConfigFile.toUtf8().data() );
			throw 0;
		}

		WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher is processing configuration file %s",
			strDispConfigFile.toUtf8().data() );

		////////////////////////////////////////////////////////////////////////
		// Dispatcher's config XML exists: try to read and interpret
		////////////////////////////////////////////////////////////////////////
		PRL_RESULT load_rc = getDispConfigGuard().getDispConfig()->loadFromFile( &f_in_cfg );

		if( !IS_OPERATION_SUCCEEDED( load_rc ) )
		{
			QString msg = QString( "Dispatcher config XML LOAD FAILED. "
				"Config file = %1, Error in line=%2 on pos=%3, "
				" error message = '%4' "
				). arg( f_in_cfg.fileName() )
				.arg( getDispConfigGuard().getDispConfig()->m_iErrLine )
				.arg( getDispConfigGuard().getDispConfig()->m_iErrCol )
				.arg( getDispConfigGuard().getDispConfig()->m_szErrMsg );

			printHorrorLogMessage ( msg, load_rc );
			throw 0;
		}

		////////////////////////////////////////////////////////////////////////
		// Dispatcher's config XML load succeeded
		////////////////////////////////////////////////////////////////////////

		CDspDispConfigGuard::storeConstantValue_ServerUuid(
			getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid() );

		WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher ID=%s configuration loaded successfully.",
						QSTR2UTF8(CDspDispConfigGuard::getServerUuid()) );

		//https://bugzilla.sw.ru/show_bug.cgi?id=439457
		//Apply process log level from configuration
		bool bVerboseLogEnabled = m_dispConfigGuard.getDispCommonPrefs()->getDebug()->isVerboseLogEnabled();
		WRITE_TRACE( DBG_FATAL, "Applying initial verbose log level. bVerboseLogEnabled=%d", (int)bVerboseLogEnabled );
		int log_level = (bVerboseLogEnabled ? DBG_DEBUG : -1);
		SetLogLevel(log_level);

		bool bConfirmationModeEnabled = false;
		CDspDispConfigGuard::storeConstantValue_ConfirmationModeEnabledByDefault(
			bConfirmationModeEnabled );

		bool bConfigCacheEnabled = m_dispConfigGuard.getDispWorkSpacePrefs()->isVmConfigCacheEnabled();
		CDspDispConfigGuard::storeConstantValue_setConfigCacheEnabled( bConfigCacheEnabled	);
		WRITE_TRACE( DBG_FATAL, "Vm Config Cache mech is %s", bConfigCacheEnabled?"ENABLED":"DISABLED" );

		////////////////////////////////////////////////////////////////////////
		// Dispatcher's version
		////////////////////////////////////////////////////////////////////////

		if ( bDispConfExists )
			m_qsOldDispVersion = getDispConfigGuard().getDispConfig()->getVersion();
		else
			m_qsOldDispVersion = VER_FULL_BUILD_NUMBER_STR;

		getDispConfigGuard().getDispConfig()->setVersion(VER_FULL_BUILD_NUMBER_STR);
		if( m_qsOldDispVersion != VER_FULL_BUILD_NUMBER_STR )
		{
			WRITE_TRACE( DBG_WARNING, "NOTE: Dispatcher which ran before had different version:"
				" previous_version '%s', current_version '%s' "
				, QSTR2UTF8(m_qsOldDispVersion)
				, VER_FULL_BUILD_NUMBER_STR );
		}

#ifdef _WIN_
		// Listen any address alwayes ON for support lightweight clients
		getDispConfigGuard().getDispConfig()->getDispatcherSettings()
			->getCommonPreferences()->getWorkspacePreferences()->setListenAnyAddr(true);
#endif
		WRITE_TRACE(DBG_FATAL, "initDispConfig() completed.");
	}
	catch (...)
	{
		return false;
	}

	return true;
}

bool CDspService::isDispVersionChanged()
{
		return  m_qsOldDispVersion != VER_FULL_BUILD_NUMBER_STR;
}

bool CDspService::isDispMajorVersionChanged()
{
	QStringList oldVersion = CDspService::instance()->m_qsOldDispVersion.split("."),
			currVersion = QString(VER_FULL_BUILD_NUMBER_STR).split(".");
	PRL_ASSERT( currVersion.size() >= 2 );
	return (oldVersion.size() < 2) ||
			(oldVersion[0] != currVersion[0]) || (oldVersion[1] != currVersion[1]);
}

bool CDspService::initVmDirCatalogue ()
{
	// Helper for root users.
	CAuthHelper rootAuth;

	try
	{
		// get VmDirCatalogue file name
		QString strVmDirCatalogueFile = ParallelsDirs::getDispatcherVmCatalogueFilePath();

		//////////////////////////////////////////////////////////////////////
		// create  vm directory list config.
		QFile f_in_cfg( strVmDirCatalogueFile );
		if( !f_in_cfg.exists() )
		{
			WRITE_TRACE(DBG_FATAL, "Vm Dir Catalogue file does not exist. We are trying to create file ... [%s]",
				QSTR2UTF8( strVmDirCatalogueFile ) );

			SmartPtr<CVmDirectories> pVmDirCatalogue( new CVmDirectories() );
			pVmDirCatalogue->enableCrashSafeSaving();
			pVmDirCatalogue->setVmServerIdentification(
				new CVmServerIdentification( getDispConfigGuard().getDispConfig()->getVmServerIdentification() ) );

			PRL_RESULT rc = pVmDirCatalogue->saveToFile( &f_in_cfg );
			if ( PRL_FAILED (rc) )
			{
				WRITE_TRACE(DBG_FATAL, "Can't create m_strVmDirCatalogueFile configuration file %s",
					QSTR2UTF8 ( strVmDirCatalogueFile ) );
				throw 0;
			}

		}

		//////////////////////////////////////////////////////////////////////
		// load  vm directory list
		PRL_RESULT rc = getVmDirManager().setCatalogueFileName(strVmDirCatalogueFile);
		if ( PRL_FAILED (rc) )
		{
			WRITE_TRACE(DBG_FATAL, "Can't set the VmDirManager catalog file name %s",
				QSTR2UTF8 ( strVmDirCatalogueFile ) );
			throw 0;
		}
		else
		{
			//////////////////////////////////////////////////////////////////////
			// store changes in vm catalogue
			rc = getVmDirManager().saveVmDirCatalogue();
			if ( PRL_FAILED (rc) )
			{
				WRITE_TRACE(DBG_FATAL, "Can't save configuration file %s",
					QSTR2UTF8 ( strVmDirCatalogueFile ) );
				throw 0;
			}

		}

		//////////////////////////////////////////////////////////////////////
		// synchronize server uuid
		QString qsServerUuid = getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid();
		QString qsVmDirServerUuid = getVmDirManager().getVmDirCatalogue()
			->getVmServerIdentification()->getServerUuid();
		if (qsServerUuid != qsVmDirServerUuid)
		{
			getVmDirManager().getVmDirCatalogue()->getVmServerIdentification()->setServerUuid(qsServerUuid);

			//////////////////////////////////////////////////////////////////////
			// store changes in vm catalogue1

			rc = getVmDirManager().saveVmDirCatalogue();
			if ( PRL_FAILED (rc) )
			{
				WRITE_TRACE(DBG_FATAL, "Can't save configuration file %s",
					QSTR2UTF8 ( strVmDirCatalogueFile ) );
				throw 0;
			}

			WRITE_TRACE(DBG_FATAL, "%s server uuid in VmDirectories XML file was changed to \
						%s server uuid from DispConfig XML file",
						QSTR2UTF8(qsVmDirServerUuid), QSTR2UTF8(qsServerUuid));
		}

		//////////////////////////////////////////////////////////////////////
		// create entry for default vm directory
		CDispWorkspacePreferences* pWorkspace = getDispConfigGuard().getDispConfig()->getDispatcherSettings()
			->getCommonPreferences()->getWorkspacePreferences();

		QString defaultVmDir = pWorkspace->getDefaultVmDirectory();
		if ( defaultVmDir.isEmpty() )
		{
			WRITE_TRACE(DBG_FATAL, "DefaultVmDirectory is empty in dispatcher config,"
				" start to create default vmDirectory." );

			QString vmDirUuid = Uuid::createUuid().toString();

			CVmDirectory* pVmDir = new CVmDirectory(
				vmDirUuid
				, ParallelsDirs::getCommonDefaultVmCatalogue()
				, "Default"
				);

			//add to vm dir catalogue xml
			getVmDirManager().getVmDirCatalogue()->addVmDirectory( pVmDir );

			//add to dispatcher xml
			pWorkspace->setDefaultVmDirectory( vmDirUuid );

			//////////////////////////////////////////////////////////////////////
			// store changes in vm catalogue
			rc = getVmDirManager().getVmDirCatalogue()->saveToFile( &f_in_cfg );
			if ( PRL_FAILED (rc) )
			{
				WRITE_TRACE(DBG_FATAL, "Can't create m_strVmDirCatalogueFile configuration file %s",
					QSTR2UTF8 ( strVmDirCatalogueFile ) );
				throw 0;
			}

			//////////////////////////////////////////////////////////////////////
			// store changes in dispatcher config
			rc = getDispConfigGuard().saveConfig();
			if ( PRL_FAILED (rc) )
			{
				WRITE_TRACE(DBG_FATAL, "Can't create disp configuration file by error %s"
					, PRL_RESULT_TO_STRING(rc) );
				throw 0;
			}
		}

		WRITE_TRACE(DBG_FATAL, "initVmDirCatalogue() completed.");
	}
	catch (...)
	{
		return false;
	}

	return true;
}

bool CDspService::checkConfigsIntegrity ()
{
	bool flgError = false;

	try
	{
		QSet<QString> hashVmDirId;
		{
			//////////////////////////////////////////////////////////////////////
			// check unique for vmdir id
			// fill check set.
			Vm::Directory::Dao::Locked d;
			foreach (const CVmDirectory& d, d.getList())
			{
				if ( ! hashVmDirId.contains( d.getUuid() ) )
					hashVmDirId.insert ( d.getUuid() );
				else
				{
					flgError = true;
					WRITE_TRACE(DBG_FATAL, "ERROR: vmdirlist contains not unique id [%s]", QSTR2UTF8( d.getUuid() ) );
				}
			}

			if ( flgError )
				throw 0;
		}
		//////////////////////////////////////////////////////////////////////
		// check default vm directory
		QString defVmDir = getDispConfigGuard().getDispWorkSpacePrefs()->getDefaultVmDirectory();
		if ( !hashVmDirId.contains(defVmDir) )
		{
			WRITE_TRACE(DBG_FATAL, "ERROR: default Vm dir UUID  not exist in vmdirlist [%s]", QSTR2UTF8( defVmDir ) );
			QString msg = "default Vm dir UUID  not exist in vmdirlist";
			printHorrorLogMessage ( msg, PRL_ERR_UNEXPECTED );

			throw 0;
		}

		QSet<QString> usersIds;
		//////////////////////////////////////////////////////////////////////
		// check users vm directories
		foreach ( CDispUser* p, getDispConfigGuard().getDispSettings()->getUsersPreferences()->m_lstDispUsers )
		{
			if ( ! hashVmDirId.contains( p->getUserWorkspace()->getVmDirectory() ) )
			{
				flgError = true;
				WRITE_TRACE(DBG_FATAL, "ERROR: vmdirlist doesn't contains vm_dir_id [%s]"
					" for user [%s]"
					, QSTR2UTF8(  p->getUserWorkspace()->getVmDirectory() )
					, QSTR2UTF8(  p->getUserName() ) );
			}

			usersIds.insert( p->getUserId() );
		}

		if ( flgError )
			throw 0;

		//////////////////////////////////////////////////////////////////////
		//check unique for user id
		if ( usersIds.size() != getDispConfigGuard().getDispSettings()->getUsersPreferences()->m_lstDispUsers.size() )
		{
			WRITE_TRACE(DBG_FATAL, "ERROR: dispatcher config contains users with not unique uuid!" );
			throw 0;
		}

		// common params only
		QString currVersion = getDispConfigGuard().getDispConfig()->getVersion();
		CDspBugPatcherLogic::patchCommon( m_qsOldDispVersion, currVersion );
	}
	catch( int )
	{
		return false;
	}
	return true;
}

bool CDspService::createDispConfig ()
{
	///////////////////////////////////
	// Generate new configuration file
	///////////////////////////////////
	SmartPtr<CDispatcherConfig> pConf( new CDispatcherConfig() );
	pConf->enableCrashSafeSaving();
	CVmServerIdentification *pServIdent = pConf->getVmServerIdentification();


	QString log_message;
	if( ! m_strServerUuidFromCorruptedDispConfig.isEmpty() )
	{
		pServIdent->setServerUuid(m_strServerUuidFromCorruptedDispConfig);
		log_message = "Server uuid was  recovered from corrupted config";
	}
	else
	{
		// #119311
		CDspLockedPointer<QSettings>
			pSettings = CDspService::instance()->getQSettings();

		QString key = QString( "%1/%2" )
			.arg( isServerMode()? QSETTINGS_TAG_MODE_SERVER : QSETTINGS_TAG_MODE_DESKTOP )
			.arg( QSETTINGS_TAG_SERVER_UUID );
		if( pSettings->contains(key) && !Uuid( pSettings->value( key ).toString() ).isNull() )
		{
			pServIdent->setServerUuid( pSettings->value( key ).toString() );
			log_message = "Server uuid was recovered from previous installation";
		}
		else
		{
			pServIdent->setServerUuid(Uuid::createUuid().toString());
			log_message = "Server uuid was created.";
		}
	}
	WRITE_TRACE(DBG_FATAL, "%s. server_uuid= %s", QSTR2UTF8( log_message ), QSTR2UTF8( pServIdent->getServerUuid() ) );

	CDispatcherSettings *pSettings = pConf->getDispatcherSettings();
	CDispCommonPreferences *pPreferences = pSettings->getCommonPreferences();
	CDispWorkspacePreferences *pWorkspacePreferences = pPreferences->getWorkspacePreferences();
	CDispMemoryPreferences *pMemoryPreferences = pPreferences->getMemoryPreferences();
	CDispCpuPreferences *pCpuPreferences = pPreferences->getCpuPreferences();

	pCpuPreferences->setMaxVCpu(VM_MAX_VCPU);

	// Setup Workspace preferences
	pWorkspacePreferences->setDefaultVmDirectory("");
	pWorkspacePreferences->setDispatcherPort( getDefaultListenPort() );
	pWorkspacePreferences->setDistributedDirectory( CDspService::isServerMode()? false: true );
	// #423517 Enable network shares both in server and workstation
	pWorkspacePreferences->setAllowUseNetworkShares( true );
	pWorkspacePreferences->setDefaultChangeSettings( CDspService::isServerMode()? false : true );

	// Setup Memory preferences
	unsigned int uiMemTotal = CDspHostInfo::getMemTotal();
	unsigned int uiMaxMemory = CDspHostInfo::getMaxVmMem();
	pMemoryPreferences->setHostRamSize(uiMemTotal);
	pMemoryPreferences->setMaxVmMemory(uiMaxMemory);
	pMemoryPreferences->setMinVmMemory(VM_MIN_MEM);

	// Setup default backup path
	pPreferences->getBackupTargetPreferences()->setDefaultBackupDirectory(
			ParallelsDirs::getDefaultBackupDir());

	// Max reserved memory limit is calculated for maximum VMM size allowed for this
	// platform (i.e. if platform supports both 32 and 64 bit VMMs, calculate reserved
	// memory by using 64 bit VMM).
	// It's switched off because this is valid only for VMs

	QString strDispConfigFile = ParallelsDirs::getDispatcherConfigFilePath();

	// Try to save new configuration file
	CDispNetworkPreferences*
		pNetwork = pConf->getDispatcherSettings()
		->getCommonPreferences()->getNetworkPreferences();

	pNetwork->setNoSaveFlag(true);
	if (IS_OPERATION_SUCCEEDED(pConf->saveToFile(strDispConfigFile)))
	{
		pNetwork->setNoSaveFlag(false);

#       ifdef _WIN_
		// FIXME: Setup file permissions
#       endif // _WIN_

		return true;
	}
	return false;
}

bool CDspService::updateDispConfig ()
{
	////////////////////////////////////////////////////////////////////////
	// update Dispatcher's config with latest host info
	////////////////////////////////////////////////////////////////////////


	// host RAM size
	getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->setHostRamSize(
		getHostInfo()->data()->getMemorySettings()->getHostRamSize() );

	// min VM memory
	getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->setMinVmMemory(
		getHostInfo()->data()->getMemorySettings()->getMinVmMemory() );

	// recommended max VM memory
	getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->setRecommendedMaxVmMemory(
		getHostInfo()->data()->getMemorySettings()->getRecommendedMaxVmMemory() );

	// max reserved memory limit
	getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->setMaxReservedMemoryLimit(
		getHostInfo()->data()->getMemorySettings()->getMaxReservedMemoryLimit() );

	unsigned int nReservedMemoryLimit
		= getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->getReservedMemoryLimit();
	if( nReservedMemoryLimit % 4 != 0 )
	{
		nReservedMemoryLimit = nReservedMemoryLimit - nReservedMemoryLimit % 4;
		getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()
			->setReservedMemoryLimit( nReservedMemoryLimit );
	}

	unsigned int nMinReservedMemoryLimit
		= getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->getMinReservedMemoryLimit();
	unsigned int nMaxReservedMemoryLimit
		= getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->getMaxReservedMemoryLimit();

	if( nReservedMemoryLimit < nMinReservedMemoryLimit )
		getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()
			->setReservedMemoryLimit( nMinReservedMemoryLimit );
	if( nReservedMemoryLimit > nMaxReservedMemoryLimit )
		getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()
			->setReservedMemoryLimit( nMaxReservedMemoryLimit );

	////////////////////////////////////////////////////////////////////////
	// Force USB connect mode to connect to host os in Player mode
	////////////////////////////////////////////////////////////////////////
#ifdef SENTILLION_VTHERE_PLAYER
	if ( isPlayerMode() )
		getDispConfigGuard().getDispWorkSpacePrefs()->setUsbConnectionType( PUD_CONNECT_TO_PRIMARY_OS );
#endif

	////////////////////////////////////////////////////////////////////////
	// Initialize generic PCI devices defaults on first dispatcher start
	// https://bugzilla.sw.ru/show_bug.cgi?id=270818
	////////////////////////////////////////////////////////////////////////

	QMap<QString , PRL_GENERIC_DEVICE_STATE > mapDevStates;
	foreach( CHwGenericPciDevice* pPciDevice, getHostInfo()->data()->getGenericPciDevices()->m_lstGenericPciDevice )
	{
		mapDevStates.insert(pPciDevice->getDeviceId(), pPciDevice->getDeviceState());
	}

	CDspLockedPointer<CDispCommonPreferences> pCommonPrefs = getDispConfigGuard().getDispCommonPrefs();
	CDispGenericPciDevices* pDispDevices = pCommonPrefs->getPciPreferences()->getGenericPciDevices();
	foreach(CDispGenericPciDevice* pDispDevice, pDispDevices->m_lstGenericPciDevices)
	{
		if ( ! (pDispDevice->getDeviceState() >= PGS_CONNECTED_TO_HOST
				&& pDispDevice->getDeviceState() <= PGS_RESERVED) )
		{
			if ( mapDevStates.contains(pDispDevice->getDeviceId()) )
			{
				pDispDevice->setDeviceState(mapDevStates.value(pDispDevice->getDeviceId()));
			}
			else
			{
				pDispDevice->setDeviceState(PGS_CONNECTED_TO_HOST);
			}
		}
	}


	// notify clients about changes
	CVmEvent event( PET_DSP_EVT_COMMON_PREFS_CHANGED, QString(), PIE_DISPATCHER);
	SmartPtr<IOPackage> p =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event );
	CDspService::instance()->getClientManager().sendPackageToAllClients( p );


	////////////////////////////////////////////////////////////////////////
	// save config file
	////////////////////////////////////////////////////////////////////////
	WRITE_TRACE( DBG_FATAL, "Dispacther config was successfully updated." );

	QString strDispConfigFile = ParallelsDirs::getDispatcherConfigFilePath();

	PRL_RESULT save_rc = getDispConfigGuard().saveConfig( strDispConfigFile );
	if( PRL_FAILED( save_rc ) )
	{
		WRITE_TRACE(DBG_FATAL,
			"Unable to write  updated data to disp configuration file by error %s."
			" It will stored in memory only !"
			, PRL_RESULT_TO_STRING(save_rc) );

		return false;
	}
	return true;
}

void CDspService::printHorrorLogMessage ( const QString& reason, PRL_RESULT rcCode )
{
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "##############################################");
	WRITE_TRACE(DBG_FATAL, "##############################################");
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "!!                                                  !!" );
	WRITE_TRACE(DBG_FATAL, "!!             !!! ERROR !!!!                       !!" );
	WRITE_TRACE(DBG_FATAL, "!!                                                  !!" );
	WRITE_TRACE(DBG_FATAL, "!!                                                  !!" );
	WRITE_TRACE(DBG_FATAL, "!!                                                  !!" );
	WRITE_TRACE(DBG_FATAL, "REASON: %s",                        QSTR2UTF8( reason ) );
	WRITE_TRACE(DBG_FATAL, "RETURN CODE: %#x [%s]",  rcCode,  PRL_RESULT_TO_STRING ( rcCode ) );
	WRITE_TRACE(DBG_FATAL, "!!                                                  !!" );
	WRITE_TRACE(DBG_FATAL, "!!                                                  !!" );
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "##############################################");
	WRITE_TRACE(DBG_FATAL, "##############################################");
	WRITE_TRACE(DBG_FATAL, "##############################################");
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
}


bool CDspService::checkExistAndCreateDirectory ( const QString& strDirPath,
												CAuthHelper& rootAuth,
												PermissionType perm )
{
#	ifndef _WIN_
	mode_t modeDispConfigDir =  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ;
	mode_t modeVmDir = S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX;
	mode_t modeBackupDir = S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX;
#	endif
	if (CFileHelper::DirectoryExists(strDirPath, &rootAuth))
	{
#	ifndef _WIN_
		// #7539 TRACE INVALID PERMISSION of dispatcher directories
		struct stat currDirStat;
		stat( QSTR2UTF8(strDirPath), &currDirStat );
		if(perm==permDispConfigDir && currDirStat.st_mode != (modeDispConfigDir | S_IFDIR ) )
		{
			WRITE_TRACE(DBG_FATAL, "WARNING: Invalid permission for dispConfigDir: '%s' mode: %#o instead %#o "
				, QSTR2UTF8(strDirPath)
				, currDirStat.st_mode
				, modeDispConfigDir|S_IFDIR  );

			if( !chmod( QSTR2UTF8(strDirPath),modeDispConfigDir ) )
				WRITE_TRACE(DBG_FATAL, "Right permission to dispConfigDir was restored" );
		}
		else if(perm==permVmDir && currDirStat.st_mode != ( modeVmDir | S_IFDIR ) )
		{
			WRITE_TRACE(DBG_FATAL, "WARNING: Invalid permission for vmDir: '%s' mode: %#o instead %#o "
				,QSTR2UTF8(strDirPath)
				,currDirStat.st_mode
				,modeVmDir|S_IFDIR  );

			if( !chmod( QSTR2UTF8(strDirPath), modeVmDir ) )
				WRITE_TRACE(DBG_FATAL, "Right permission to vmDir was restored" );
		}
		else if(perm==permBackupDir && currDirStat.st_mode != ( modeBackupDir | S_IFDIR ) )
		{
			WRITE_TRACE(DBG_FATAL, "WARNING: Invalid permission for BackupDir: '%s' mode: %#o instead %#o "
				,QSTR2UTF8(strDirPath)
				,currDirStat.st_mode
				,modeVmDir|S_IFDIR  );

			if( !chmod( QSTR2UTF8(strDirPath), modeBackupDir ) )
				WRITE_TRACE(DBG_FATAL, "Right permission to BackupDir was restored" );
		}


#	else
		// FIXME: Need add check for windows
#	endif
		return true;
	}
	// //////////////////////////////////////////
	// Try to create directory
#   ifndef _WIN_
	mode_t  mask = umask( 0022  ); // store umask
#   endif

	bool ret = CFileHelper::WriteDirectory(strDirPath, &rootAuth);

#   ifndef _WIN_
	umask( mask ); // revert umask
#   endif

	if ( !ret )
	{
		WRITE_TRACE(DBG_FATAL, "Can't create Parallels directory %s",
			QSTR2UTF8(strDirPath));
		return false;
	}

#   ifndef _WIN_

	// Setup permissions for parallels directory
	if(perm==permDispConfigDir)
		chmod( QSTR2UTF8(strDirPath),modeDispConfigDir );
	else if(perm==permVmDir)
		chmod( QSTR2UTF8(strDirPath), modeVmDir );
	else if(perm==permBackupDir)
		chmod( QSTR2UTF8(strDirPath), modeBackupDir );
	else
		return false;

#   else  // _WIN_

	// FIXME: rootAuth.GetAuth()->ClearAccessList(strDirPath);

	QString strAlias;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

	// Lookup "SYSTEM" account name
	if (CAuthHelper::LookupAliasFromRid("", &SIDAuthNT, SECURITY_LOCAL_SYSTEM_RID,
		0, strAlias))
	{
		// Setup permissions for "SYSTEM" account
		// FIXME: CAuth::AddAccessRights(strDirPath, strAlias, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT);
	}

	// Lookup "Local administrators" group name
	if (CAuthHelper::LookupAliasFromRid("", &SIDAuthNT, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, strAlias))
	{
		// Setup permissions for "Local administrators" group
		CAuth::AddAccessRights(strDirPath, strAlias, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT, false);
	}

	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	// Lookup "Everyone" group name
	if (CAuthHelper::LookupAliasFromRid("", &SIDAuthWorld, SECURITY_WORLD_RID,
		0, strAlias))
	{
		// Setup permissions for "Everyone" group
		// FIXME: CAuth::AddAccessRights(strDirPath, strAlias, GENERIC_READ, SUB_CONTAINERS_AND_OBJECTS_INHERIT);
	}

	Q_UNUSED(perm);
#   endif // _WIN_

	return true;
}

/*********************** SLOTS ***********************************************/

void CDspService::clientConnected ( IOSender::Handle h )
{
	COMMON_TRY
	{
		IOSender::Type senderType = m_ioServerPool->clientSenderType( h );
		SmartPtr<CDspHandler> handler =
			CDspHandlerRegistrator::instance().findHandler( senderType );
		if ( ! handler.isValid() )
			return;

		handler->handleClientConnected( h );
	}
	COMMON_CATCH_WITH_INT_PARAM( m_ioServerPool->clientSenderType( h ) );
}

void CDspService::clientDisconnected ( IOSender::Handle h )
{
	COMMON_TRY
	{
		IOSender::Type senderType = m_ioServerPool->clientSenderType( h );
		SmartPtr<CDspHandler> handler =
			CDspHandlerRegistrator::instance().findHandler( senderType );
		if ( ! handler.isValid() )
			return;

		handler->handleClientDisconnected( h );
	}
	COMMON_CATCH_WITH_INT_PARAM( m_ioServerPool->clientSenderType( h ) );
}

void CDspService::onClientStateChanged( IOServerInterface*, IOSender::Handle, IOSender::State )
{
}

void CDspService::packageReceived ( IOSender::Handle h ,
								   const SmartPtr<IOPackage> p )
{
	COMMON_TRY
	{
		IOSender::Type senderType = m_ioServerPool->clientSenderType( h );
		SmartPtr<CDspHandler> handler =
			CDspHandlerRegistrator::instance().findHandler( senderType );
		if ( ! handler.isValid() )
			return;

		handler->handleToDispatcherPackage( h, p );
	}
	COMMON_CATCH_WITH_INT_PARAM( m_ioServerPool->clientSenderType( h ) );
}

void CDspService::onCredentialsBecomeAvailable()
{
	setCredentialsForServers(IOServerList()
							 << m_ioAnyAddrServer
#ifdef _WIN_
							 << m_ioListeningServer
#endif
							 );
}

void CDspService::clientStateChanged ( IOSender::Handle h,
									  IOSender::State st )
{
	COMMON_TRY
	{
		IOSender::Type senderType = m_ioServerPool->clientSenderType( h );
		SmartPtr<CDspHandler> handler =
			CDspHandlerRegistrator::instance().findHandler( senderType );
		if ( ! handler.isValid() )
			return;

		handler->handleClientStateChanged( h, st );
	}
	COMMON_CATCH_WITH_INT_PARAM( m_ioServerPool->clientSenderType( h ) );
}

void CDspService::clientDetached ( IOSender::Handle h,
								  const IOCommunication::DetachedClient dc )
{
	COMMON_TRY
	{
		IOSender::Type senderType = m_ioServerPool->clientSenderType( h );
		SmartPtr<CDspHandler> handler =
			CDspHandlerRegistrator::instance().findHandler( senderType );
		if ( ! handler.isValid() )
			return;

		handler->handleDetachClient( h, dc );
	}
	COMMON_CATCH_WITH_INT_PARAM( m_ioServerPool->clientSenderType( h ) );
}

/****************************************************************************/

quint32 CDspService::getDefaultListenPort()
{
	return PRL_DISPATCHER_LISTEN_PORT;
}

void CDspService::createIOServers ( quint32 listenPort, PRL_SECURITY_LEVEL securityLevel )
{
	if( 0 == listenPort || listenPort >= USHRT_MAX )
	{
		WRITE_TRACE(DBG_FATAL, "Invalid listen port %d. Would be set to default %d", listenPort, getDefaultListenPort() );
		listenPort = getDefaultListenPort();
	}

	if( securityLevel < PSL_LOW_SECURITY || securityLevel > PSL_HIGH_SECURITY )
	{
		WRITE_TRACE(DBG_FATAL, "Invalid security level %d. Would be set to default %d", securityLevel, PSL_LOW_SECURITY );
		securityLevel = PSL_LOW_SECURITY;
	}

	IOCredentials credentials;
	WRITE_TRACE(DBG_FATAL, "Starting old-style connection mode ...");

	// Create IO server pool
	m_ioServerPool = SmartPtr<IOServerPool>(new IOServerPool);
	bool bRes = false;
#ifndef _WIN_
	m_ioLocalUnixListeningServer = SmartPtr<IOServerInterface>(setup(new IOServer(
		IORoutingTableHelper::GetServerRoutingTable(securityLevel),
		IOSender::Dispatcher,
		ParallelsDirs::getDispatcherLocalSocketPath(),
		0, true )));
	bRes = m_ioServerPool->addServer( m_ioLocalUnixListeningServer );
	PRL_ASSERT(bRes);
#endif

	// Listens to anyaddr if server mode
	if (isServerMode())
	{
		m_ioListeningServer = SmartPtr<IOServerInterface>(setup(new IOServer(
			IORoutingTableHelper::GetServerRoutingTable(securityLevel),
			IOSender::Dispatcher,
			IOService::AnyAddr, listenPort, false, credentials )));

		bRes = m_ioServerPool->addServer( m_ioListeningServer );
		PRL_ASSERT(bRes);
	}
	else
	{
#ifndef _WIN_
		m_ioListeningServer = m_ioLocalUnixListeningServer;
		m_ioLocalUnixListeningServer = SmartPtr<IOServerInterface>( 0 );
#else
		m_ioListeningServer = SmartPtr<IOServerInterface>(setup(new IOServer(
			IORoutingTableHelper::GetServerRoutingTable(securityLevel),
			IOSender::Dispatcher,
			IOService::LoopbackAddr, listenPort )));
		bRes = m_ioServerPool->addServer( m_ioListeningServer );
		PRL_ASSERT(bRes);
#endif
	}

	bool bConnected = false;

	//
	// Connect to IO server pool
	//

	// Signal connect
	bConnected = QObject::connect( m_ioServerPool.getImpl(),
		SIGNAL(onClientConnected(IOSender::Handle)),
		SLOT(clientConnected(IOSender::Handle)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Signal connect
	bConnected = QObject::connect( m_ioServerPool.getImpl(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Signal connect
	bConnected = QObject::connect( m_ioServerPool.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle,
		const SmartPtr<IOPackage>)),
		SLOT(packageReceived(IOSender::Handle,
		const SmartPtr<IOPackage>)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Signal connect
	bConnected = QObject::connect( m_ioServerPool.getImpl(),
		SIGNAL(onClientStateChanged(IOSender::Handle,
		IOSender::State)),
		SLOT(clientStateChanged(IOSender::Handle,
		IOSender::State)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Signal connect
	bConnected = QObject::connect( m_ioServerPool.getImpl(),
		SIGNAL(onDetachClient(IOSender::Handle,
		const IOCommunication::DetachedClient)),
		SLOT(clientDetached(IOSender::Handle,
		const IOCommunication::DetachedClient)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Signal connect
	bConnected = QObject::connect( m_ioServerPool.getImpl(),
		SIGNAL(onClientStateChanged(IOServerInterface *,
		IOSender::Handle,
		IOSender::State)),
		SLOT(onClientStateChanged(IOServerInterface *,
		IOSender::Handle,
		IOSender::State)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	Q_UNUSED(bConnected);
}


bool CDspService::initNetworkConfig( bool bNotifyNetworkService )
{
	PRL_RESULT
		prlResult = PrlNet::InitNetworkConfig( *getNetworkConfig().getPtr() , bNotifyNetworkService);
	if( PRL_FAILED(prlResult) )
	{
		return false;
	}

	PrlNet::InitConfigLibrary(getNetworkConfig().getPtr());

	WRITE_TRACE( DBG_FATAL, "initNetworkConfig() completed.");

	return true;
}


QString CDspService::makeBackupSuffix()
{
	return QString( ".BACKUP." ) + QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss" );
}

ProcPerfStoragesContainer* CDspService::getPerfStorageContainer()
{
	return &m_perfstorage_container ;
}

storage_descriptor_t CDspService::getBasePerfStorage()
{
	if (!m_base_perfstorage.storage)
		m_base_perfstorage = m_perfstorage_container.AddNewStorage("disp_server") ;
	return m_base_perfstorage ;
}

QString CDspService::getHostOsVersion() const
{
	return m_strHostOsVersion;
}


void CDspService::emitCleanupOnUserSessionDestroy( QString sessionUuid )
{
	if( !m_bServerStopping )
		emit cleanupOnUserSessionDestroy(sessionUuid);
}

void CDspService::onCleanupOnUserSessionDestroy( QString sessionUuid )
{
	// Always run from service thread
	PRL_ASSERT( QThread::currentThread() ==  QObject::thread() );

	if( !m_bServerStopping )
		CDspService::instance()->getVmDirHelper().getMultiEditDispatcher()
			->cleanupAllBeginEditMarksByAccessToken( sessionUuid );
}

void CDspService::handleVmMigrateEvent(SmartPtr<CDspVm> pVm, const SmartPtr<IOPackage> &p)
{
	emit onVmMigrateEventReceived(pVm->getVmUuid(), p);
}

/*****************************************************************************/

#ifndef _WIN_
void sigterm_handler ( int signum )
{
	if ( SIGTERM == signum )
	{
	PUT_RAW_MESSAGE( "==== SIGTERM was caught in dispatcher. Stop will be emitted ...\n" );
		g_bSIGTERM_was_received = true;

		// NOTE: WE DO NOT USE ::exit(3) to prevent deadlocks as defined in #PDFM-20651
	}
}
#endif

#if defined(_LIN_)
// empty signal handler to allow interruption of
// active system calls (like poll() etc)
static void sigusr_handler(int signum)
{
	(void)signum;
}
#endif

int main_part(int argc, char** argv)
{
#ifndef _WIN_
	signal(SIGTERM, sigterm_handler);
#endif

#if defined(_LIN_)
	mallopt(M_CHECK_ACTION, 2);
	signal(SIGUSR1, sigusr_handler);
#endif
	int result = 64;
	COMMON_TRY
	{
		CMainDspService service( argc, argv );
		result = service.exec();
	}
	COMMON_CATCH;

	return result;
}

int main ( int argc, char** argv )
{
	return main_part(argc, argv);
}

void CDspService::cleanupAllStaticObjects()
{
	CDspVm::UnregisterAllVmObjects();
}

/*****************************************************************************/

quint64 CDspService::getServiceStartTime() const
{
	return m_serviceStartTime;
}

quint64 CDspService::getServiceStartTimeMonotonic() const
{
	return m_serviceStartTimeMonotonic;
}

bool CDspService::setCredentialsForServers(const IOServerList & lstServers)
{
	Q_UNUSED(lstServers);
	return false;
}
