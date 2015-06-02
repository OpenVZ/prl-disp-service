/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
///		QtCoreTest.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing QtCore functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "QtCoreTest.h"

#include <QDir>
#include <QTemporaryFile>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <stdio.h>

#include "Interfaces/ParallelsQt.h"

#include "Libraries/Logging/Logging.h"
#include "Libraries/Std/PrlAssert.h"


namespace
{
bool detectInfinityRecursion_On_QDir_EntryList( const QString& path, int currDeepLevel )
{

	LOG_MESSAGE( DBG_DEBUG, "%s+%s\n", QSTR2UTF8( QString( 2*currDeepLevel, ' ' ) ), QSTR2UTF8( path ) );
	if( ! QFileInfo( path ).isDir() )
		return true;

	QDir::Filters
		dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

	QDir dir( path );
	QStringList sList = dir.entryList( dirFilter, QDir::DirsLast );

	foreach( QString fName, sList )
	{
		QString childPath = QString("%1/%2").arg(path).arg(fName);

		if( path == childPath )
		{
			WRITE_TRACE(DBG_FATAL, "recursion started !!! parent dir path == child_dir_path:  '%s'=='%s'"
				, QSTR2UTF8( path )
				, QSTR2UTF8( childPath )
			);

			return false;
		}

		if( !detectInfinityRecursion_On_QDir_EntryList( childPath, currDeepLevel+1 ) )
			return false;
	}
	return true;
}

bool detectInfinityRecursion_On_QDir_EntryInfoList( const QString& path, int currDeepLevel )
{

	LOG_MESSAGE( DBG_DEBUG, "%s+%s\n", QSTR2UTF8( QString( 2*currDeepLevel, ' ' ) ), QSTR2UTF8( path ) );
	if( ! QFileInfo( path ).isDir() )
		return true;

	QDir::Filters
		dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

	QDir dir( path );
	QFileInfoList fiList = dir.entryInfoList( dirFilter, QDir::DirsLast );

	foreach( QFileInfo fi2, fiList )
	{
		//if( path == fi2.absoluteFilePath() )
		if( QFileInfo(path) == fi2 )
		{
			WRITE_TRACE(DBG_FATAL, "recursion started !!! parent dir path == child_dir_path:  '%s'=='%s'"
				, QSTR2UTF8( path )
				, QSTR2UTF8( fi2.absoluteFilePath() )
			);
			return false;
		}

		if( !detectInfinityRecursion_On_QDir_EntryInfoList( fi2.absoluteFilePath(), currDeepLevel+1 ) )
			return false;
	}
	return true;
}

bool printEntries( const QFileInfo& fi, int currDeepLevel, int expectDeepLevel )
{
	if( currDeepLevel > expectDeepLevel + 5 )
	{
		WRITE_TRACE(DBG_FATAL, "Start to stack overflow. break test."  );
		return false;
	}

	printf( "%s+%s\n", QSTR2UTF8( QString( 2*currDeepLevel, ' ' ) ), QSTR2UTF8( fi.fileName() ) );
	if( !fi.isDir() )
		return true;

	QDir::Filters
		dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

	QDir dir( fi.absoluteFilePath() );
	QFileInfoList fiList = dir.entryInfoList( dirFilter, QDir::DirsLast );

	//LOG_MESSAGE( DBG_DEBUG, "abs path= '%s', list size=%d\n", QSTR2UTF8( fi.absoluteFilePath() ), fiList.size() );
	//foreach( QFileInfo fi2, fiList )
	//	LOG_MESSAGE( DBG_DEBUG, "--list entry='%s', path='%s'\n"
	//		, QSTR2UTF8( fi2.fileName() )
	//		, QSTR2UTF8( fi2.absoluteFilePath() ) );

	foreach( QFileInfo fi2, fiList )
	{
		if( fi.absoluteFilePath() == fi2.absoluteFilePath() )
			WRITE_TRACE(DBG_FATAL, "recursion started !!! parent dir path == child_dir_path:  '%s'=='%s'"
				, QSTR2UTF8( fi.absoluteFilePath() )
				, QSTR2UTF8( fi2.absoluteFilePath() )
			);

		if( !printEntries( fi2, currDeepLevel+1, expectDeepLevel ) )
			return false;
	}
	return true;
}

}

class InitTestBackSlashOperationsInQT
{
public:
	InitTestBackSlashOperationsInQT();
	~InitTestBackSlashOperationsInQT();

	bool isValid() { return m_isValid; }
	QString getTestDir() { return m_testDir; }
private:
	bool m_isValid;
	QString m_testDir;
	QStack<QString>  m_filesToDel;
	QStack<QString>  m_dirsToDel;
};

InitTestBackSlashOperationsInQT::InitTestBackSlashOperationsInQT()
: m_isValid( false )
{
	// mkdir backSlash
	// create file "\"
	// create file "nornal"
	// create dir "\"
	// get ( with Nosymlink flag) & print it

	// mkdir backSlash
	QString sDirRoot = QDir::currentPath();
	QString sDirTest = QString("%1/%2").arg( sDirRoot).arg( ".backSlashTest" );
	QString sFileNornal = sDirTest + "/normal" ;
	QString sFileSlash = sDirTest + "/\\" ;
	QString sDirSlashParent =  sDirTest +  "/dirSlash";
	QString sDirSlash = QString("%1/%2").arg( sDirSlashParent ).arg( "/\\" );

	m_testDir = sDirTest;

	QVERIFY( QDir().mkdir( sDirTest ) );
	m_dirsToDel.push( sDirTest );

	// =====================
	// create file "nornal"
	QVERIFY( QFile( sFileNornal ).open( QIODevice::WriteOnly ) );
	m_filesToDel.push( sFileNornal );

	// create file "\"
	QVERIFY( QFile( sFileSlash ).open( QIODevice::WriteOnly ) );
	m_filesToDel.push( sFileSlash );

	// create dir "\"
	QVERIFY( QDir().mkdir( sDirSlashParent ) );
	m_dirsToDel.push( sDirSlashParent );

	QVERIFY( QDir().mkdir( sDirSlash ) );
	m_dirsToDel.push( sDirSlash );

	m_isValid = true;
}

InitTestBackSlashOperationsInQT::~InitTestBackSlashOperationsInQT()
{
	while( !m_filesToDel.empty() )
	{
		QString f = m_filesToDel.pop();
		PRL_ASSERT( QFile(f).remove() ) ;
	}
	while( !m_dirsToDel.empty() )
	{
		QString f = m_dirsToDel.pop();
		PRL_ASSERT( QDir(f).rmdir(f) ) ;
	}
}

void QtCoreTest::test_QDir_entryInfoList_withBackSlash()
{
	QSKIP("Skipping test untill Qt fixed", SkipAll);

	InitTestBackSlashOperationsInQT autoRemover;

	QVERIFY( autoRemover.isValid() );
	// get entries ( with Nosymlink flag) & print it
	QVERIFY( detectInfinityRecursion_On_QDir_EntryList( autoRemover.getTestDir(), 0 ) );
	QVERIFY( detectInfinityRecursion_On_QDir_EntryInfoList( autoRemover.getTestDir(), 0 ) );

	QFileInfo fi( autoRemover.getTestDir() );
	QVERIFY( printEntries( fi, 0, 20 ) );
}

////////////////////////////////////////////////////////////

WorkerThreadBase::~WorkerThreadBase()
{
	stop();
	if( !wait( 1000 ) )
		WRITE_TRACE( DBG_FATAL, "thread %p was terminated", this );
	terminate();
}

bool WorkerThreadBase::ping( unsigned long timeout )
{
	QMutexLocker lock( &m_mtx );
	m_bPingSign = true;
	return m_cond.wait( &m_mtx, timeout );
}

void WorkerThreadBase::tryAnswerToPing()
{
	do
	{
		if( !m_bPingSign )
			continue;

		QMutexLocker lock( &m_mtx );
		if( !m_bPingSign )
			continue;
		m_bPingSign = false;
		m_cond.wakeAll();

	}while(0);
}
///////////////////////////////////////////////////////////////
void WorkerThreadFSW_removePath::run()
{
	while( !m_bStop )
	{
		m_pFsw->addPath(m_pFile->fileName());
		for (int i = 0; i < 1025; ++i)
			m_pFsw->removePath(m_pFile->fileName());

		tryAnswerToPing();
	}//while
	WRITE_TRACE( DBG_INFO, "thread finished");
}

void QtCoreTest::test_QFileSystemWatcher_hangsInRemovePath_afterMultipleCall()
{
	QFileSystemWatcher _fsw;

	QTemporaryFile _file;
	QVERIFY( _file.open() );

	WorkerThreadFSW_removePath wtRemover( &_fsw, &_file );

	wtRemover.start();

	QDateTime dtFinishTime = QDateTime::currentDateTime().addSecs(20);
	while( QDateTime::currentDateTime() < dtFinishTime )
	{
		static unsigned long uPingTimeoutMsec = 1000;

		if( !wtRemover.ping( uPingTimeoutMsec ) )
		{
			WRITE_TRACE( DBG_FATAL, "REMOVER thread hangs" );
			break;
		}

		WRITE_TRACE( DBG_INFO, "Test still alive ... ");

		QTest::qSleep( uPingTimeoutMsec );
	}

	QVERIFY( QDateTime::currentDateTime() >= dtFinishTime );
}

void WorkerThreadFSW_addPath::run()
{
	while( !m_bStop )
	{
		if( m_type == tAdder )
		{
			/*
			// TRY TO SKIP QT WARNING
			QStringList files = m_pFsw->files();
			files.detach();
			if( !files.contains(m_pFile->fileName() ) )
			*/
			m_pFsw->addPath(m_pFile->fileName());
		}
		else
			m_pFsw->removePath(m_pFile->fileName());

		tryAnswerToPing();
	}//while
	WRITE_TRACE( DBG_FATAL, "thread '%s' finished", (m_type == tAdder)?"ADDER":"REMOVER");
}

void QtCoreTest::test_QFileSystemWatcher_hangsInRemovePath_afterAddPath()
{
#ifdef _WIN_
	QSKIP("Skip till https://bugzilla.sw.ru/show_bug.cgi?id=466136 will be fixed", SkipAll);
#endif
	QFileSystemWatcher _fsw;

	QTemporaryFile _file;
	QVERIFY( _file.open() );

	WorkerThreadFSW_addPath wtAdder( &_fsw, &_file, WorkerThreadFSW_addPath::tAdder );
	WorkerThreadFSW_addPath wtRemover( &_fsw, &_file, WorkerThreadFSW_addPath::tRemover );

	wtAdder.start();
	wtRemover.start();

	QDateTime dtFinishTime = QDateTime::currentDateTime().addSecs(20);
	while( QDateTime::currentDateTime() < dtFinishTime )
	{
		static unsigned long uPingTimeoutMsec = 1000;
		if( !wtAdder.ping( uPingTimeoutMsec ) )
		{
			WRITE_TRACE( DBG_FATAL, "ADDER thread hangs" );
			break;
		}

		if( !wtRemover.ping( uPingTimeoutMsec ) )
		{
			WRITE_TRACE( DBG_FATAL, "REMOVER thread hangs" );
			break;
		}

		WRITE_TRACE( DBG_FATAL, "Test still alive ...");

		QTest::qSleep( uPingTimeoutMsec );
	}

	QVERIFY( QDateTime::currentDateTime() >= dtFinishTime );
}

