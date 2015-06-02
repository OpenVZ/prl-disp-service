///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspCrashReportMonitor.cpp
///
/// Crash report monitor implementation.
///
/// @author romanp
/// @owner sergeym
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

// FIX ME remove it after fix #481526
// #define FORCE_LOGGING_ON
// #define FORCE_LOGGING_LEVEL DBG_DEBUG
#define FORCE_LOGGING_PREFIX "CrashMonitor"

#include <QMutexLocker>

#include "CDspCrashReportMonitor.h"
#include "CDspService.h"
#include "CDspDispConfigGuard.h"
#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"
#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include "CDspProblemReportHelper.h"

#include "Libraries/Logging/Logging.h"
#include "Libraries/Std/PrlAssert.h"

#include "Interfaces/ParallelsTypes.h"
#include "Build/Current.ver"
/*****************************************************************************/

namespace {

	const char* CrashDumpRegExpStr =
		"^(\\w+)(\\.(\\d+-\\d+-\\d+-\\d+))?(\\.(\\{[a-zA-Z0-9\\-]+\\})-"
		"(\\{[a-zA-Z0-9\\-]+\\}))?\\.(\\d+)\\.(lin|win|mac)\\.(dmp|lcore)$";

	const char* g_sPanicExt = ".panic";

	bool isLowMemoryReport( const QString& appName )
	{
		return appName.startsWith(g_low_memory_file);
	}

	bool isPanicReport( const QString& appName )
	{
		return appName.endsWith(g_sPanicExt);
	}
}

/*****************************************************************************/

CDspCrashReportMonitor::CrashReport::CrashReport () :
	pid(0)
{}

/*****************************************************************************/

CrashReportCatcherProxy::CrashReportCatcherProxy (
	CDspCrashReportMonitor* recv ) :
	m_recv(recv)
{
	PRL_ASSERT(m_recv);
}

void CrashReportCatcherProxy::onChangeDir( QString dir )
{
	m_recv->onChangeDir(dir);
}

void CrashReportCatcherProxy::onExploreReports ()
{
	m_recv->onExploreReports();
}

/*****************************************************************************/

CDspCrashReportMonitor::CDspCrashReportMonitor()
:	m_monitorState(CrashMonitorIsStopped)
{
	qRegisterMetaType< CDspCrashReportMonitor::CrashReport >( "CDspCrashReportMonitor::CrashReport" );
	qRegisterMetaType< CDspCrashReportMonitor::RepCache >( "CDspCrashReportMonitor::RepCache" );

	m_systemApps = CProblemReportUtils::GetCrashDumpsTemplates("", false);
}

CDspCrashReportMonitor::~CDspCrashReportMonitor ()
{
	stopMonitor();
}

bool CDspCrashReportMonitor::startMonitor ()
{
	QMutexLocker locker( &m_mutex );
	if ( m_monitorState != CrashMonitorIsStopped )
		return false;
	m_monitorState = CrashMonitorIsStarting;
	// We should wait for real stop if thread is stopping now
	QThread::wait();
	QThread::start();
	m_wait.wait( &m_mutex );
	return true;
}

bool CDspCrashReportMonitor::stopMonitor ()
{
	QMutexLocker locker( &m_mutex );
	if ( m_monitorState != CrashMonitorIsStarted )
		return false;
	m_monitorState = CrashMonitorIsStopping;
	QThread::exit(0);
	m_wait.wait( &m_mutex );
	QThread::wait();
	return true;
}

void CDspCrashReportMonitor::run ()
{
	PathsList sysRepPaths;
	// Get crash dumps
	sysRepPaths << ParallelsDirs::getCrashDumpsPath();

	// Current date time
	m_currDateTime = QDateTime::currentDateTime();

	// Get system apps
	QMutexLocker locker( &m_mutex );
	QStringList systemApps = m_systemApps;
	locker.unlock();

	// Get all crash dumps for last week
	m_fullSysRepCache = getCrashDumps
									  ( sysRepPaths,
										m_currDateTime.addDays(-7),
										systemApps,
										false // all reports/dumps
									  );

	CrashReportCatcherProxy catcher( this );
	bool connected = false;
	// Connect
	connected = QObject::connect( &m_fsWatcher,
								  SIGNAL(directoryChanged(QString)),
								  &catcher,
								  SLOT(onChangeDir(QString)) );
	PRL_ASSERT( connected );

	// Connect
	connected = QObject::connect( &m_fsWatcher,
								  SIGNAL(fileChanged(const QString&)),
								  &catcher,
								  SLOT(onExploreReports()) );
	PRL_ASSERT( connected );

	// Connect
	connected = QObject::connect( this,
								  SIGNAL(exploreReports()),
								  &catcher,
								  SLOT(onExploreReports()) );
	PRL_ASSERT( connected );

	// Change state to started and wake all
	locker.relock();

	// Add system paths to watch
	foreach( const QString& path, sysRepPaths )
		m_fsWatcher.addPath( path );

	m_sysRepCache = m_fullSysRepCache;
	m_monitorState = CrashMonitorIsStarted;
	m_wait.wakeAll();
	locker.unlock();

	// Enters thread event loop
	exec();

	// Lock
	locker.relock();

	QStringList files = m_fsWatcher.files();
	QStringList dirs = m_fsWatcher.directories();
	// Remove all files from the FS watcher
	if ( ! files.isEmpty() )
		m_fsWatcher.removePaths( files );
	// Remove all dirs from the FS watcher
	if ( ! dirs.isEmpty() )
		m_fsWatcher.removePaths( dirs );

	m_monitorState = CrashMonitorIsStopped;
	m_wait.wakeAll();
}

static void printDebugRepCache( CDspCrashReportMonitor::RepCache cache, const QString& msg )
{
	WRITE_TRACE( DBG_DEBUG, "%s ============ ", QSTR2UTF8(msg) );

	foreach( QString strkey, cache.keys() )
	{
		WRITE_TRACE( DBG_DEBUG, "report name %s", QSTR2UTF8( strkey ) );
		QHash<QDateTime, CDspCrashReportMonitor::CrashReport> value = cache[strkey];
		foreach( QDateTime time, value.keys() )
		{
			WRITE_TRACE( DBG_DEBUG, "report last monitoring time %s", QSTR2UTF8( time.toString() ) );
			WRITE_TRACE( DBG_DEBUG, "report path %s", QSTR2UTF8( value[time].reportPath ) );
		}
	}

	WRITE_TRACE( DBG_DEBUG, "============ end of %s", QSTR2UTF8(msg) );
}

void CDspCrashReportMonitor::onChangeDir( QString )
{
	onExploreReports();
}

static CDspCrashReportMonitor::RepCache getRepCacheFromDate(
	const CDspCrashReportMonitor::RepCache& fullCache, const QDateTime& lastMonitoring )
{
			typedef CDspCrashReportMonitor::RepCache RepCache;
			typedef CDspCrashReportMonitor::CrashReport CrashReport;

			RepCache sysUsrReps;

			// Cache sorted date/time
			QHash< QString, QList<QDateTime> > sysRepKeys;
			foreach ( QString key, fullCache.keys() ) {
				const QHash<QDateTime, CrashReport>& reports = fullCache[key];
				QList<QDateTime> repKeys = reports.keys();
				qSort(repKeys.begin(), repKeys.end());
				sysRepKeys[ key ] = repKeys;
			}

			QHash< QString, QList<QDateTime> >::Iterator i =
				sysRepKeys.begin();
			while ( i != sysRepKeys.end() ) {
				QList<QDateTime>& dtList = i.value();
				QList<QDateTime>::Iterator startI =
					qLowerBound(dtList.begin(), dtList.end(),
								lastMonitoring);
				if ( startI != dtList.end() ) {
					const QHash<QDateTime, CrashReport>& sysReps =
						fullCache[ i.key() ];
					QHash<QDateTime, CrashReport>& userReps =
						sysUsrReps[ i.key() ];
					while ( startI != dtList.end() ) {
						// We must skip equal date, so range must be
						// (begin, end), not [begin, end)
						if ( *startI == lastMonitoring ) {
							++startI;
							continue;
						}

						userReps[ *startI ] = sysReps[ *startI ];

						++startI;
					}// while ( startI !=
					// #PDFM-25308 ( skip report without dumps )
					if( sysUsrReps[ i.key() ].isEmpty() ){
						sysUsrReps.remove( i.key() );
					}
				}//  if ( startI !=
				++i;
			}// while ( i !=

	return sysUsrReps;
}

void CDspCrashReportMonitor::onExploreReports ()
{
	// Always this thread
	PRL_ASSERT( this == QThread::currentThread() );

	// Lock
	QMutexLocker locker( &m_mutex );

	WRITE_TRACE( DBG_DEBUG, "Explore crash reports started" );

	if ( m_monitorState != CrashMonitorIsStarted )
		return;

	QStringList systemApps = m_systemApps;
	QStringList userApps = m_userApps;
	QList< SmartPtr<CDspClient> > users = m_users.values();
	QHash< SmartPtr<CDspClient>, RepCache > usersRepCache = m_usersRepCache;

	// Unlock
	locker.unlock();

	const QString LastSystemCrashMonitoringDate(
		"LastSystemCrashMonitoringDate" );
	const QString LastUserCrashMonitoringDate(
		"LastUserCrashMonitoringDate" );
	const QString ParallelsUsers( "ParallelsUsers" );
	const QString UserName( "UserName" );

	PathsList sysRepPaths;
	// Get crash dumps
	sysRepPaths << ParallelsDirs::getCrashDumpsPath();

	// Rewrite date/times
	QDateTime dtFrom = m_currDateTime;
	m_currDateTime = QDateTime::currentDateTime();

	RepCache sysPatch = patchReportsCache( sysRepPaths, m_fullSysRepCache,
										   dtFrom, systemApps );

	printDebugRepCache( sysPatch, "onExploreReports sysPatch" );
	printDebugRepCache( m_fullSysRepCache, "onExploreReports m_fullSysRepCache" );

	// Check if cache was patched
	if ( ! sysPatch.isEmpty() ) {
		locker.relock();
		m_sysRepCache = m_fullSysRepCache;
		locker.unlock();
	}

	//
	// Firstly, we read settings, and only after successfull
	// report emitting we set last monitoring date to settings.
	// So, we should prepare list of users to be saved to settings
	// and last monitoring date.
	//
	QDateTime lastMonDateToSettings = m_currDateTime;
	QHash<QString, QDateTime> allUsersLastMonToSettings;

	// Read disp settings and save users last monitoring dates to
	// this variable:
	QHash<QString, QDateTime> usersLastMonitoring;
	{
		CDspLockedPointer<QSettings> settings =
			CDspService::instance()->getQSettings();

		QDateTime systemLastMonitoring;

		if ( ! settings->contains(LastSystemCrashMonitoringDate) ) {
			systemLastMonitoring = m_currDateTime;
		}
		else {
			systemLastMonitoring =
				settings->value(LastSystemCrashMonitoringDate).toDateTime();
		}
		LOG_MESSAGE( DBG_DEBUG, "GLOBAL, lastMonDate %s"
						, QSTR2UTF8( systemLastMonitoring.toString() ) );


		// Read array
		int size = settings->beginReadArray(ParallelsUsers);
		for ( int i = 0; i < size; ++i ) {
			settings->setArrayIndex(i);
			usersLastMonitoring[ settings->value(UserName).toString() ] =
				settings->value(LastUserCrashMonitoringDate).toDateTime();
		}
		settings->endArray();

		// Make a copy of users from settings
		allUsersLastMonToSettings = usersLastMonitoring;
		QHash<QString, QDateTime> usersCopy = usersLastMonitoring;

		// Fill users hashes
		if ( users.size() > 0 ) {

			bool newUser = false;
			for ( int i = 0; i < users.size(); ++i ) {
				QString userName = users[i]->getUserName();
				if ( ! usersLastMonitoring.contains(userName) ) {
					usersLastMonitoring[userName] = systemLastMonitoring;
					usersCopy[userName] = systemLastMonitoring;
					newUser = true;
				}
				// Update online users
				allUsersLastMonToSettings[userName] = lastMonDateToSettings;
			}

			// We should add new user to settings with 'systemLastMonitoring'
			// So just write all users with updated date
			if ( newUser ) {
				PRL_ASSERT(usersCopy.size() > 0);

				// Write array
				settings->beginWriteArray( ParallelsUsers,
										   usersCopy.size() );

				// Fill users
				int i = 0;
				foreach ( QString userName, usersCopy.keys() ) {
					settings->setArrayIndex(i++);
					settings->setValue( UserName, userName );
					settings->setValue( LastUserCrashMonitoringDate,
										usersCopy[userName] );
					LOG_MESSAGE( DBG_DEBUG, "Store: user %s, lastMonDate %s"
						, QSTR2UTF8(userName)
						, QSTR2UTF8( usersCopy[userName].toString() ) );
				}

				settings->endArray();

				// Write all changes
				settings->sync();
			}
		}
	}

	// We must sync settings after successfull report emition
	bool wasReportEmition = false;

	// Get crash reports for every user
	foreach ( SmartPtr<CDspClient> user, users ) {
		PRL_ASSERT( usersLastMonitoring.contains(user->getUserName()) );
		QDateTime lastMonitoring =
			usersLastMonitoring[ user->getUserName() ];

		WRITE_TRACE( DBG_DEBUG, "user with name %s has last monitorin date = %s",
						QSTR2UTF8( user->getUserName() ),
						QSTR2UTF8( lastMonitoring.toString() ) );

		// Last monitoring date must be not older than 1 week
		if ( lastMonitoring < m_currDateTime.addDays(-7) )
			lastMonitoring = m_currDateTime.addDays(-7);

		bool firstUserLogin = false;

		// For _not_ Mac OS users reports/dumps are always empty!
		// System dumps are used instead
		RepCache userReports;

		//
		// We must pick up user reports only for Mac OS.
		// On other platforms we always pick up crash dumps from
		// single directory
		//
		//
		// For _not_ Mac OS platforms just set 'first user login' flag.
		// (use empty cache for key storage)
		//

		if ( ! usersRepCache.contains(user) ) {
			usersRepCache[ user ] = RepCache();

			// Save key with empty users cache
			locker.relock();
			m_usersRepCache = usersRepCache;
			locker.unlock();

			firstUserLogin = true;
		}
		else {
			firstUserLogin = false;
		}

		// Pick new system reports/dumps for this user

		RepCache sysUsrReps;

		// For first login just pick up all really new reports,
		// i.e. report date must > last monitoring date
		if ( firstUserLogin ) {
			sysUsrReps = getRepCacheFromDate( m_fullSysRepCache, lastMonitoring );
			userReports = getRepCacheFromDate( userReports, lastMonitoring );
		}// if ( firstUserLogin
		// Just unite with system patch
		else
			sysUsrReps = sysPatch;

		LOG_MESSAGE( DBG_DEBUG, "sysUsrReps.size() %d, userReports.size() %d"
			, sysUsrReps.size(), userReports.size() );

		if ( sysUsrReps.keys().size() > 0 ||
			 userReports.keys().size() > 0 ) {

			wasReportEmition = true;

			LOG_MESSAGE( DBG_DEBUG, "Report WAS emitted" );

	//
			// For _not_ Mac OS 'userReports' are empty!
			//
			emit onCrashReport( user, sysUsrReps, userReports,
								m_fullSysRepCache );
		}
		else
			LOG_MESSAGE( DBG_DEBUG, "Report WAS NOT emitted" );
	} // foreach m_users

	//
	// We must write new last monitoring date _after_ succcessfull
	// report emition even for 1 user! Or we can lose some report!
	//
	if ( wasReportEmition ) {
		CDspLockedPointer<QSettings> settings =
			CDspService::instance()->getQSettings();

		settings->setValue(LastSystemCrashMonitoringDate,
						   lastMonDateToSettings);

		LOG_MESSAGE( DBG_DEBUG, "Store: GLOBAL, lastMonDate %s"
						, QSTR2UTF8( lastMonDateToSettings.toString() ) );
		// Write array
		if ( allUsersLastMonToSettings.size() > 0 ) {

			settings->beginWriteArray( ParallelsUsers,
									   allUsersLastMonToSettings.size() );

			// Fill all users
			int i = 0;
			foreach ( QString userName,
					  allUsersLastMonToSettings.keys() ) {

				settings->setArrayIndex(i++);
				settings->setValue( UserName, userName );
				settings->setValue( LastUserCrashMonitoringDate,
									allUsersLastMonToSettings[userName] );
				LOG_MESSAGE( DBG_DEBUG, "Store: user %s, lastMonDate %s"
						, QSTR2UTF8(userName)
						, QSTR2UTF8( allUsersLastMonToSettings[userName].toString() ) );
		}

			settings->endArray();
		}

		// Write all changes
		settings->sync();
	}
}

CDspCrashReportMonitor::RepCache CDspCrashReportMonitor::patchReportsCache (
	const CDspCrashReportMonitor::PathsList& pathsList,
	RepCache& repCache,
	const QDateTime& dtFrom,
	const QStringList& apps )
{
	RepCache cache = getCrashDumps
									(
									 pathsList, dtFrom,
									 apps,  true // only patch
									);

	// Do patching
	foreach ( QString app, cache.keys() ) {
		QHash<QDateTime, CrashReport>& newReps = cache[ app ];
		QHash<QDateTime, CrashReport>& mainReps = repCache[ app ];
		foreach ( QDateTime dt, newReps.keys() ) {
			mainReps[ dt ] = newReps[ dt ];
		}
	}
	return cache;
}

QFileInfoList CDspCrashReportMonitor::getNewFiles (
	const QString& path,
	const QStringList& apps,
	const QStringList& exts,
	const QStringList& appsWithExt,
	bool patching )
{
	QStringList appFilters;
	foreach ( QString ext, exts ) {
		foreach ( QString app, apps ) {
			appFilters.append( app + ext );
		}
	}

	appFilters += appsWithExt;

	QDir dir( path );
	dir.setFilter( QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden );
	dir.setNameFilters( appFilters );


	QFileInfoList reportList;
	QFileInfoList oldRepList;
	bool wasHere = m_repDirs.contains( path );
	QPair<QFileInfo, QFileInfoList>& dirInfo = m_repDirs[ path ];
	QFileInfo dirFileInfo( path );

	reportList = dir.entryInfoList();
	oldRepList = dirInfo.second;
	dirInfo.first = dirFileInfo;
	dirInfo.second = reportList;

	// Get only 'new' or 'changed' files if in patching mode
	if ( patching && wasHere ) {
		QFileInfoList changedList;
		QHash<QString, QFileInfo> hash;
		hash.reserve(reportList.size());
		foreach ( QFileInfo fileInfo, oldRepList ) {
			hash[ fileInfo.absoluteFilePath() ] = fileInfo;
		}
		foreach ( QFileInfo fileInfo, reportList ) {
			QString newFilePath = fileInfo.absoluteFilePath();
			if ( hash.contains(newFilePath) ) {
				QFileInfo& oldFileInfo = hash[ newFilePath ];
				if ( fileInfo.lastModified() != oldFileInfo.lastModified() ) {
					changedList.append( fileInfo );
				}
			}
			else {
				changedList.append( fileInfo );
			}
		}
		// Replace full file list with really changed files
		reportList = changedList;
	}

	return reportList;
}

CDspCrashReportMonitor::RepCache CDspCrashReportMonitor::getCrashDumps (
	const CDspCrashReportMonitor::PathsList& pathsList,
	const QDateTime& dtFrom,
	const QStringList& apps,
	bool patching )
{
	QHash< QString, QHash<QDateTime, CrashReport> > res;
	QStringList exts = QStringList() <<
#if defined(_LIN_)
		".*.lin.lcore.desc"
#elif defined(_WIN_)
		".*.win.dmp.desc"
#else
#error Unsupported
#endif
		;

	QFileInfoList dumpsList;
	foreach( const QString& path, pathsList )
		dumpsList << getNewFiles( path, apps, exts, QStringList(), patching );
	QDateTime currentDateTime = QDateTime::currentDateTime();

	QRegExp crashDumpRx( CrashDumpRegExpStr );

	foreach ( QFileInfo file, dumpsList ) {

		QString appName = file.fileName();
		QString absAppPath = file.absoluteFilePath();
		// Remove trailing '.desc'
		appName.remove( QRegExp("\\.desc$") );
		absAppPath.remove( QRegExp("\\.desc$") );

		// Check crash dump name
		if ( crashDumpRx.indexIn(appName) == -1 ) {
			WRITE_TRACE(DBG_FATAL, "Error: file '%s' is not a crash dump!",
						QSTR2UTF8(absAppPath));
			continue;
		}
		// Save app name
		appName = crashDumpRx.cap(1);

		// Skip 'future' files
		// (modification date and current date can be different)
		// (this difference ~ < 5 secs)
		if ( file.lastModified() > currentDateTime.addSecs(5) ) {
			WRITE_TRACE(DBG_FATAL,
						"Error: future of modified date of file '%s' is not "
						"possible! Will be skipped!", QSTR2UTF8(absAppPath));
			continue;
		}

		// Skip old files
		// (modification date and report date can be different)
		// (this difference ~ < 5 secs)
		if ( file.lastModified() < dtFrom.addSecs(-5) ) {
			continue;
		}

		// Read dump
		QFile dump( absAppPath );
		bool o = dump.open( QFile::ReadOnly );
		if ( ! o ) {
			WRITE_TRACE(DBG_FATAL,
						"Error: can't open crash dump '%s' for reading!",
						QSTR2UTF8(absAppPath));
			continue;
		}
		QByteArray dumpBa = dump.readAll();
		dump.close();

		QString pid = crashDumpRx.cap(7);
		bool ok = false;

		CrashReport rep;
		rep.report = dumpBa;
		rep.pid = pid.toInt(&ok);
		rep.reportPath = absAppPath;
		rep.reportDt = file.lastModified();

		QHash<QDateTime, CrashReport>& appHash = res[ appName ];
		appHash[ rep.reportDt ] = rep;
	}

	return res;
}

bool CDspCrashReportMonitor::extractUuidsFromVmDumpPath (
	const CDspCrashReportMonitor::CrashReport& report,
	QString& vmDirUuid,
	QString& vmUuid )
{
	QString reportPath = QFileInfo(report.reportPath).fileName();
	QRegExp crashDumpRx( CrashDumpRegExpStr );
	if ( crashDumpRx.indexIn(reportPath) == -1 ) {
		return false;
	}
	vmDirUuid = crashDumpRx.cap(5);
	vmUuid = crashDumpRx.cap(6);

	return true;
}

void CDspCrashReportMonitor::markVmCrashDumpAsCreated (
	const QString& vmDirUuid,
	const QString& vmUuid )
{
	QString key = vmDirUuid + vmUuid;
	QMutexLocker locker( &m_mutex );
	if ( ! m_createdDumps.contains(key) ) {
		QPair<QWaitCondition*, bool>& pair = m_createdDumps[key];
		pair.first = 0;
		pair.second = true;
		return;
	}
	else {
		QPair<QWaitCondition*, bool>& pair = m_createdDumps[key];
		pair.second = true;
		if ( pair.first )
			pair.first->wakeAll();
		return;
	}
}

bool CDspCrashReportMonitor::waitForVmCrashDump (
	const QString& vmDirUuid,
	const QString& vmUuid,
	quint32 msecs )
{
	QString key = vmDirUuid + vmUuid;
	QMutexLocker locker( &m_mutex );
	if ( ! m_createdDumps.contains(key) ) {
		QPair<QWaitCondition*, bool>& pair = m_createdDumps[key];
		pair.first = new QWaitCondition;
		pair.second = false;
	}
	QPair<QWaitCondition*, bool>& pair = m_createdDumps[key];
	// Check if report was marked as created
	if ( pair.second ) {
		// Remove key
		m_createdDumps.remove( key );
		return true;
	}
	// Wait for crash dump
	pair.first->wait( &m_mutex, msecs );

	// Save result, remove key
	if ( m_createdDumps.contains(key) ) {
		QPair<QWaitCondition*, bool>& pair = m_createdDumps[key];
		bool res = pair.second;
		delete pair.first;
		m_createdDumps.remove( key );
		return res;
	}
	// Abnormal case
	else {
		PRL_ASSERT(0);
		return false;
	}
}


/*****************************************************************************/

void CrashHandlerWrap::crashReportHandler (
	SmartPtr<CDspClient> pUser,
	CDspCrashReportMonitor::RepCache sysRep,
	CDspCrashReportMonitor::RepCache usrRep,
	CDspCrashReportMonitor::RepCache fullRepCache )
{
	// We don't use it on _not_ Mac OS
	(void)fullRepCache;

	PRL_ASSERT( pUser.isValid() );

	// Skip LIGHTWEIGHT clients
	if ( pUser->getFlags() & PCF_LIGHTWEIGHT_CLIENT )
		return;

	printDebugRepCache( sysRep, "crashReportHandler sysRep");
	printDebugRepCache( usrRep, "crashReportHandler usrRep");

	// Unite reports
	CDspCrashReportMonitor::RepCache repCache = sysRep.unite( usrRep );

	//
	// Only on Mac OS every Vm report must contain old Vm reports
	// (MaxDaysForVmAppReps days).

	QString vmUuid;
	QString vmDirUuid;
	QStringList lstAppNames = QStringList()
		<< g_prl_vm_app
		;
	//
	// Every Vm report must be sent separetly from other reports.
	// After all Vm reports have been sent, other (_not_ Vm) reports
	// must be sent in a single package.
	//
	foreach(QString appName, lstAppNames)
	{
		if ( ! repCache.contains(appName) )
			continue;

		// TODO for Mac OS:
		//	we should handle server (not desktop) case:
		//		- user starts vm
		//		- user disconnects from dispatcher
		//		- vm dies e.g. by sigsegv
		//		- user connects to dispatcher
		//		- user wants to receive crash dump
		//	in this situation vm process does not exist, but report must be
		//	created in general way, but not registered !!!

		// Take Vm reports
		QHash<QDateTime, CDspCrashReportMonitor::CrashReport> vmReps =
			repCache.take(appName);

		// Get all dates of 'new' reports
		QList<QDateTime> dtList = vmReps.keys();
		qSort( dtList.begin(), dtList.end() );


		// Create report for every 'new' crash
		foreach ( QDateTime dt, dtList )
		{
			CDspCrashReportMonitor::CrashReport& vmRep = vmReps[ dt ];

			// Vm report cache must be contain only 1 report for
			// _not_ Mac OS platforms
			CDspCrashReportMonitor::RepCache vmRepCache;

			// Add 'new' next crash
			vmRepCache[ appName ][ dt ] = vmRep;

			QString crashPath = vmRep.reportPath;
			bool ok = CDspService::instance()->getCrashReportMonitor().
				extractUuidsFromVmDumpPath( vmRep, vmDirUuid, vmUuid );

			if ( ! ok ) {
				WRITE_TRACE(DBG_FATAL, "Error: Can't extract Vm uuids from Vm crash path");
				continue;
			}

			//
			// ??? Why don't we check 'vmDirUuid' for Mac OS?
			// ??? It can be empty?
			//
			if ( vmDirUuid.isEmpty() )
			{
				WRITE_TRACE(DBG_FATAL, "Error: Vm dir uuid is invalid while Problem "
							"Report creation!");
				continue;
			}

			if ( vmUuid.isEmpty() )
			{
				WRITE_TRACE(DBG_FATAL, "Error: Vm uuid is invalid while Problem Report "
					"creation!");
				continue;
			}

			// Transport thread (where disconnection happens) will send
			// report if dump creation fails. So we must mark dump, that
			// it was successfully created.
			CDspService::instance()->getCrashReportMonitor().
				markVmCrashDumpAsCreated( vmDirUuid, vmUuid );

			CPackedProblemReport * pTmpReport = NULL;

			CPackedProblemReport::createInstance( CPackedProblemReport::DispSide,
													&pTmpReport );
			SmartPtr<CPackedProblemReport> pReport(pTmpReport);

			if( pReport )
			{
				// try get vm config
				PRL_RESULT res = PRL_ERR_SUCCESS;
				SmartPtr<CVmConfiguration> pVmConfig =
					CDspService::instance()->getVmDirHelper().getVmConfigByUuid (vmDirUuid,
					vmUuid,
					res);
				if( pVmConfig )
					CDspProblemReportHelper::FormProblemReportDataForDisconnect( *pReport.getImpl(), pVmConfig );
				else
				{
					WRITE_TRACE(DBG_FATAL, "Unable to get vm config for vm uuid %s, error %s"
						, QSTR2UTF8( vmUuid )
						, PRL_RESULT_TO_STRING( res ) );
				}

				// Fill problem report with Vm report cache
				CDspProblemReportHelper::FillProblemReportData( *pReport.getImpl(), pUser, vmDirUuid, vmRepCache );

				pReport->saveMainXml();

				SmartPtr<IOPackage> p;
				if ( ! CDspProblemReportHelper::isOldProblemReportClient(pUser) )
				{
					p = CDspProblemReportHelper::createProblemReportEventPackage( *pReport.getImpl() );
				}
				else
				{
					SmartPtr<CProblemReport> pOldReport = pReport->convertToProblemReportOldFormat();
					p = CDspProblemReportHelper::createProblemReportOldFormatEventPackage( *pOldReport.getImpl() );
				}

				// Check rights
				QHash< IOSender::Handle, SmartPtr<CDspClient> > vmUsers =
						CDspService::instance()->getClientManager().getSessionListByVm( vmDirUuid, vmUuid );

				if ( vmUsers.contains(pUser->getClientHandle()) )
					pUser->sendPackage( p );

			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "cannot create report instance!");
				PRL_ASSERT(false);
			}
		}
	}

	// Try to send _not_ Vm reports in a single package
	if ( repCache.size() > 0 )
	{
		CPackedProblemReport * tmpprobRep = NULL;
		CPackedProblemReport::createInstance( CPackedProblemReport::DispSide, &tmpprobRep );
		SmartPtr<CPackedProblemReport> probRep( tmpprobRep );
		if ( probRep)
		{
			probRep->setReportType( PRT_AUTOMATIC_DETECTED_REPORT );
			// Fill problem report with report cache
			CDspProblemReportHelper::FillProblemReportData( *probRep.getImpl(),
				pUser,
				vmDirUuid,
				repCache );
			probRep->saveMainXml();

			SmartPtr<IOPackage> p;
			if ( ! CDspProblemReportHelper::isOldProblemReportClient(pUser) )
			{
				p = CDspProblemReportHelper::createProblemReportEventPackage( *probRep.getImpl() );
			}
			else
			{
				SmartPtr<CProblemReport> pOldReport = probRep->convertToProblemReportOldFormat();
				p = CDspProblemReportHelper::createProblemReportOldFormatEventPackage( *pOldReport.getImpl() );
			}

			pUser->sendPackage( p );
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "cannot create report instance!");
			PRL_ASSERT(false);
		}
	}
}
/*****************************************************************************/
