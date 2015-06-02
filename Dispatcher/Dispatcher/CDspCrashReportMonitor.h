///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspCrashReportMonitor.h
///
/// Crash report monitor header.
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

#ifndef CDSPCRASHREPORTMONITOR_H
#define CDSPCRASHREPORTMONITOR_H

#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QFileSystemWatcher>

#include "CDspClient.h"
#include "CDspVm.h"
#include "XmlModel/ProblemReport/CProblemReport.h"

class CDspCrashReportMonitor : protected QThread
{
	Q_OBJECT
public:
	class CrashReport
	{
	public:
		CrashReport ();

		QByteArray report;
		QString reportPath;
		quint32 pid;
		QDateTime reportDt;
		QString identifier;	// optional
	};

	/**< app -> reports cache */
	typedef QHash< QString, QHash<QDateTime, CrashReport> > RepCache;

	typedef QSet<QString> PathsList;
	typedef QList< SmartPtr<CDspClient> > UsersList;

	/** Constructor */
	CDspCrashReportMonitor();
	/** Destructor */
	~CDspCrashReportMonitor ();

	/** Starts monitor */
	bool startMonitor ();
	/** Stops monitor */
	bool stopMonitor ();

	/**
	 * Returns true if '.*\.{uuid}-{uuid}\..*' is found in report.reportPath
	 */
	bool extractUuidsFromVmDumpPath ( const CrashReport& report,
									  QString& vmDirUuid,
									  QString& vmUuid );

	/**
	 * Marks Vm crash dump as created
	 */
	void markVmCrashDumpAsCreated ( const QString& vmDirUuid,
									const QString& vmUuid );
	/**
	 * Waits for crash dump creation
	 */
	bool waitForVmCrashDump ( const QString& vmDirUuid,
							  const QString& vmUuid,
							  quint32 msecs );

signals:
	/** Crash reports signal */
	void onCrashReport ( SmartPtr<CDspClient>,
						 CDspCrashReportMonitor::RepCache sysReps,
						 CDspCrashReportMonitor::RepCache usrReps,
						 CDspCrashReportMonitor::RepCache fullUserReps
					   );

protected:
	void run ();

signals:
	void exploreReports ();

private slots:
	void onChangeDir( QString dir );
	void onExploreReports ();

private:
	RepCache patchReportsCache ( const PathsList& pathsList,
								 RepCache& repCache,
								 const QDateTime& dtFrom,
								 const QStringList& apps );

    RepCache getCrashDumps ( const PathsList& pathsList,
							 const QDateTime& dtAfter,
							 const QStringList& apps,
							 bool patchingMode );

	QFileInfoList getNewFiles (const QString& path,
							   const QStringList& apps,
							   const QStringList& exts,
							   const QStringList& appsWithExts,
							   bool patchingMode );

	// used for connect/disconnect user
	void addPathsToMonitoring( const SmartPtr<CDspClient>& pUser );
	void removePathsFromMonitoring( const SmartPtr<CDspClient>& pUser );
	// used in onDirChanged() event
	void addPathsToMonitoring( const QList< SmartPtr<CDspClient> >& lstUsers, const QString& changedDir );



private:
	enum CrashMonitorState {
		CrashMonitorIsStopping = 0,
		CrashMonitorIsStopped,
		CrashMonitorIsStarting,
		CrashMonitorIsStarted,
	};

	class SavedReport
	{
	public:
		SmartPtr<CDspClient> user;
		QString vmUuid;
		QString vmDirUuid;
		CrashReport crashRep;
	};

	QDateTime m_currDateTime;
	QFileSystemWatcher m_fsWatcher;
	QWaitCondition m_wait;
	QMutex m_mutex;
	QStringList m_systemApps;
	QStringList m_userApps;
	QHash<QString, QPair<QFileInfo, QFileInfoList> > m_repDirs;
	QHash< SmartPtr<CDspClient>, SmartPtr<CDspClient> > m_users;
	volatile CrashMonitorState m_monitorState;
	RepCache m_sysRepCache;
	RepCache m_fullSysRepCache;
	QHash< SmartPtr<CDspClient>, RepCache > m_usersRepCache;
	// key   -> 'vmDirUuid + vmUuid'
	// value -> ''
	QHash< QString, QPair<QWaitCondition*, bool> > m_createdDumps;

private:
	friend class CrashReportCatcherProxy;
};

class CrashReportCatcherProxy : public QObject
{
Q_OBJECT
public:
	CrashReportCatcherProxy ( CDspCrashReportMonitor* );

public slots:
	void onChangeDir( QString dir );
	void onExploreReports ();

private:
	CDspCrashReportMonitor* m_recv;
};

class CrashHandlerWrap
{
public:
	static void crashReportHandler (
		SmartPtr<CDspClient> pUser,
		CDspCrashReportMonitor::RepCache sysRep,
		CDspCrashReportMonitor::RepCache usrRep,
		CDspCrashReportMonitor::RepCache fullRepCache );
};

inline quint32 qHash ( const QDateTime& dt )
{
    return qHash( dt.toString(XML_DATETIME_FORMAT_LONG) );
}

#endif //CDSPCRASHREPORTMONITOR_H
