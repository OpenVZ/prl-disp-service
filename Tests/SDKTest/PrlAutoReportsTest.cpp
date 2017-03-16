/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///		PrlAutoReportsTest.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Test dispatcher auto reports functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

// #define FORCE_LOGGING_ON
// #define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <prlcommon/Logging/Logging.h>

#include "PrlAutoReportsTest.h"
#include "SimpleServerWrapper.h"

#include "Tests/CommonTestsUtils.h"

#include "AutoHelpers.h"

#include <prlsdk/PrlApiDeprecated.h>
#include <prlxmlmodel/DispConfig/CDispUser.h>

#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include <prlcommon/Std/SmartPtr.h>

#define CHECKS_TO_SKIP( _dumpLocation )\
{	\
	if( !isMacPlatform() ) \
		QSKIP( " Test implemented only for Apple platform now.", SkipAll ); \
  \
	if( _dumpLocation == dlSystem && !isSystemScopeAllowed() ) \
		QSKIP( "User must be root to test in SystemScope", SkipAll ); \
	\
	if( _dumpLocation == dlUser && !isMacPlatform() ) \
		QSKIP( "UserScope uses only on Mac.", SkipAll ); \
	\
	if( hasUserActiveSessions() ) \
		QSKIP( "Test will be skipped because user has active sessions (GUI, etc. )." \
			" Test should executed exclusive !", SkipAll ); \
	\
}


namespace{
	const bool CLEANUP_TMP_REPORT_DIR = true; // CAN BE USED FOR DEBUG
	const int AUTOREPORT_WAIT_TIMEOUT_MSECS = 3*60*1000;
};

static bool isMacPlatform()
{
#ifdef _MAC_
	return true;
#endif
	return false;
}
static bool isSystemScopeAllowed()
{
#ifdef _MAC_
	if( geteuid() )
		return false;
#endif
	return true;
}

void PrlAutoReportsTest::init()
{
}
void PrlAutoReportsTest::cleanup()
{
}

struct InfoStruct
{
	QWaitCondition m_Condition;
	QMutex m_Mutex;

	void storeReportHandle( const SdkHandleWrap& h );
	void checkExpectedDumps( bool& bRes, const QStringList& lstExpectedDumps );
private:
	SmartPtr<CPackedProblemReport> m_rep;
};

void InfoStruct::storeReportHandle( const SdkHandleWrap& hEvent )
{
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP( PrlEvent_GetParamByName(hEvent, EVT_PARAM_VM_PROBLEM_REPORT, hProblemReport.GetHandlePtr()) );

	QByteArray _buffer;
	PRL_UINT32 nBufferSize = 0;
	CHECK_RET_CODE_EXP( PrlEvtPrm_GetBuffer(hProblemReport, 0, &nBufferSize) );
	QVERIFY( nBufferSize > 0 );

	_buffer.resize( nBufferSize );
	CHECK_RET_CODE_EXP( PrlEvtPrm_GetBuffer(hProblemReport, (PRL_VOID_PTR)_buffer.data(), &nBufferSize) );

	QString strTempDir;
	GET_EVENT_PARAM_BY_NAME_AS_STRING( hEvent, EVT_PARAM_VM_PROBLEM_REPORT_DIR_PATH, strTempDir );

	CPackedProblemReport * pReport = 0;
	CPackedProblemReport::createInstance( CPackedProblemReport::ClientSide,
											&pReport,
											_buffer,
											strTempDir );

	QVERIFY( pReport );
	pReport->setCleanupTempDir( CLEANUP_TMP_REPORT_DIR );
	m_rep = SmartPtr<CPackedProblemReport>( pReport );
}

void InfoStruct::checkExpectedDumps( bool& bRes, const QStringList& lstExpectedDumps )
{
	bRes = false;
	QVERIFY(m_rep);
	LOG_MESSAGE( DBG_DEBUG, "DEBUG: RepPath %s\n"
		"lstExpectedDumps: %s"
		, QSTR2UTF8(m_rep->getTempDirPath())
		, QSTR2UTF8(lstExpectedDumps.join(" ,")) );

	QDir dir(m_rep->getTempDirPath());
	QFileInfoList fiList = dir.entryInfoList( lstExpectedDumps, QDir::Files|QDir::Hidden|QDir::System );

	QSet<QString> repFiles;
	foreach( QFileInfo fi, fiList )
		repFiles << fi.fileName();

	bRes = true;
	foreach( QString dumpName, lstExpectedDumps )
	{
		if( repFiles.contains(dumpName) )
			continue;

		bRes = false;
		WRITE_TRACE( DBG_FATAL, "report doesn't contain dump %s", QSTR2UTF8(dumpName) );
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void EventCallbackWrap( PRL_RESULT& outRes, PRL_HANDLE _handle, void *pData);
PRL_RESULT EventCallback(PRL_HANDLE _handle, void *pData)
{
	PRL_RESULT outRes = PRL_ERR_FAILURE;
	EventCallbackWrap( outRes, _handle, pData );
	return outRes;
}

void EventCallbackWrap( PRL_RESULT& outRes, PRL_HANDLE _handle, void *pData)
{
		outRes = PRL_ERR_FAILURE;

		SdkHandleWrap hEvent(_handle);
		PRL_HANDLE_TYPE _type;
		CHECK_RET_CODE_EXP( PrlHandle_GetType(hEvent, &_type) );
		if(_type != PHT_EVENT)
			{ outRes=PRL_ERR_SUCCESS; return; }

		InfoStruct *pInfo = static_cast<InfoStruct*>(pData);

		PRL_EVENT_TYPE _event_type;
		CHECK_RET_CODE_EXP(PrlEvent_GetType(hEvent, &_event_type) );
		if(_event_type != PET_DSP_EVT_VM_PROBLEM_REPORT_CREATED )
			{ outRes=PRL_ERR_SUCCESS; return; }

		QMutexLocker _lock(&pInfo->m_Mutex);

		pInfo->storeReportHandle( SdkHandleWrap(_handle) );

		pInfo->m_Condition.wakeAll();

		outRes=PRL_ERR_SUCCESS;
}

void PrlAutoReportsTest::copyCrashDump( bool& bRes, DumpLocation dl, DumpKind dk, QStringList& outDumpsList )
{
	bRes = false;
	(void)dl; (void)dk; (void)outDumpsList;
#ifndef _MAC_
	return;
#endif

#define CHECK_DIR_AND_CREATE( _path, chown_user, chown_group ) \
do{ \
	if( QFileInfo(_path).exists() )	\
		break;	\
	QVERIFY( QDir("/").mkdir( _path ) ); \
	if( !strlen(chown_user) )	\
		break; 	\
	QString cmd = QString( "chown %1:%2" )	\
		.arg(chown_user).arg(chown_group);	\
	QVERIFY( system( QSTR2UTF8(cmd) ) );	\
}while(0);

	QString sTail = "Library/Logs/CrashReporter";
	QString dirTo;
	switch(dl)
	{
		case dlSystem:
			dirTo= QString("/%1").arg(sTail);
			CHECK_DIR_AND_CREATE( dirTo, "root", "admin" );
			break;
		case dlUser:
			dirTo= QString("%1/%2")
				.arg(ParallelsDirs::getCurrentUserHomeDir() )
				.arg(sTail);
			CHECK_DIR_AND_CREATE( dirTo, "", "" );
			break;
	}
#undef CHECK_DIR_AND_CREATE

	QString pathFrom;
	switch(dk)
	{
		case dkPrlApp:
			pathFrom = "./SDKTest_MacOSX_Crashdump_example.txt";
			break;
		case dkVmApp:
		case dkLowMemory:
			QVERIFY(0); // doesn't supported now
	}

	QString pathTo = QString( "%1/%2-%3.crash" )
		.arg(dirTo)
		.arg( "WinAppHelper_2011-07-23-112454_SDKTEST" )
		.arg( time(0) );
	LOG_MESSAGE( DBG_DEBUG, "pathFrom '%s', pathTo '%s'", QSTR2UTF8(pathFrom), QSTR2UTF8(pathTo) );
	QVERIFY( QFile::copy( pathFrom, pathTo ) );

	outDumpsList << QFileInfo(pathTo).fileName();
	bRes = true;
}

void PrlAutoReportsTest::cleanupOldReports()
{
	SimpleServerWrapper fakeSessionToCleanupOldReports(0, false);
}

bool PrlAutoReportsTest::hasUserActiveSessions( )
{
	bool bRes = false;
	hasUserActiveSessions( bRes );
	return bRes;
}
void PrlAutoReportsTest::hasUserActiveSessions( bool& bRes )
{
	SimpleServerWrapper hServer(0, false);
	// get userUuid
	SdkHandleWrap hUserProfile;
	RECEIVE_USER_PROFILE( hServer, hUserProfile);
	CDispUser xmlUser;
	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hUserProfile, xmlUser );
	QString sCurrentUserId = xmlUser.getUserId();

	// get all sessions
	SdkHandleWrap hJob(PrlSrv_GetUserInfoList(hServer));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	SdkHandleWrap hUserInfo;
	QString sessionId;
	for(PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		hUserInfo.reset();
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hUserInfo.GetHandlePtr()));

		PRL_EXTRACT_STRING_VALUE( sessionId, hUserInfo, PrlUsrInfo_GetUuid );

		if( sessionId == sCurrentUserId )
			break;
	}
	QCOMPARE( sessionId, sCurrentUserId );
	PRL_UINT32 nSessionCount = 0;
	CHECK_RET_CODE_EXP(PrlUsrInfo_GetSessionCount(hUserInfo, &nSessionCount));
	LOG_MESSAGE( DBG_DEBUG, "Session count %d, for user %s"
		, nSessionCount, QSTR2UTF8(sCurrentUserId) );
	QVERIFY(nSessionCount > 0);

	bRes = nSessionCount > 1;
}

void PrlAutoReportsTest::testReceiveAutoReport_whenGuiWorking_byCrashAppInSystemScope()
{
	testReceiveAutoReport_whenAppWorking( dlSystem, dkPrlApp );
}

void PrlAutoReportsTest::testReceiveAutoReport_whenGuiWorking_byCrashAppInUserScope()
{
	testReceiveAutoReport_whenAppWorking( dlUser, dkPrlApp );
}

void PrlAutoReportsTest::testReceiveAutoReport_whenAppWorking(
	DumpLocation dumpLocation, DumpKind dumpKind )
{
	// 0. login to dispatcher
	// 1. copy crash dump to dump directory
	// 2. wait PET_DSP_EVT_VM_PROBLEM_REPORT_CREATED event
	// 3. parse it, do checks
	//	3.1 check that crashdump added

	CHECKS_TO_SKIP(dumpLocation);

	SdkHandleWrap hJob;
	bool bRes;

	cleanupOldReports();

	SimpleServerWrapper session(0, false);

	SmartPtr<InfoStruct> pInfo( new InfoStruct );
	QMutexLocker _lock(&pInfo->m_Mutex);

	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler( session, EventCallback, pInfo.getImpl()));

	QStringList lstDumpsFNames;
	QVERIFY_BRES( bRes, copyCrashDump( bRes, dumpLocation, dumpKind, lstDumpsFNames ) );
	QVERIFY(!lstDumpsFNames.isEmpty());

	QVERIFY( pInfo->m_Condition.wait(&pInfo->m_Mutex, AUTOREPORT_WAIT_TIMEOUT_MSECS) );

	CHECK_RET_CODE_EXP(PrlSrv_UnregEventHandler(session, EventCallback, pInfo.getImpl())) ;
	QVERIFY_BRES( bRes, pInfo->checkExpectedDumps( bRes, lstDumpsFNames ) );
}

void PrlAutoReportsTest::testReceiveAutoReport_onGuiLogin_byCrashAppInUserScope()
{
	testReceiveAutoReport_onLogin( dlUser, dkPrlApp );
}

void PrlAutoReportsTest::testReceiveAutoReport_onGuiLogin_byCrashAppInSystemScope()
{
	testReceiveAutoReport_onLogin( dlSystem, dkPrlApp );
}

void PrlAutoReportsTest::testReceiveAutoReport_onLogin(
	DumpLocation dumpLocation, DumpKind dumpKind )
{
	// 0. copy crash dump to dump directory
	// 1. login to dispatcher
	// 2. wait PET_DSP_EVT_VM_PROBLEM_REPORT_CREATED event
	// 3. parse it, do checks
	//	3.1 check that crashdump added

	CHECKS_TO_SKIP(dumpLocation);

	cleanupOldReports();

	SdkHandleWrap hJob;
	bool bRes;

	SmartPtr<InfoStruct> pInfo( new InfoStruct );
	QMutexLocker _lock(&pInfo->m_Mutex);

	QStringList lstDumpsFNames;
	QVERIFY_BRES( bRes, copyCrashDump( bRes, dumpLocation, dumpKind, lstDumpsFNames ) );
	QVERIFY(!lstDumpsFNames.isEmpty());

	SimpleServerWrapper session(0, false);
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler( session, EventCallback, pInfo.getImpl()));

	QVERIFY( pInfo->m_Condition.wait(&pInfo->m_Mutex, AUTOREPORT_WAIT_TIMEOUT_MSECS) );

	QVERIFY_BRES( bRes, pInfo->checkExpectedDumps( bRes, lstDumpsFNames ) );
}

void PrlAutoReportsTest::testReceiveAutoReport_withoutDuplicates()
{
	QSKIP( "Not implemented yet", SkipAll );
}
void PrlAutoReportsTest::testReceiveAutoReport_oneVmDumpPerReport()
{
	QSKIP( "Not implemented yet", SkipAll );
}
void PrlAutoReportsTest::testReceiveAutoReport_allNotVmDumps_InOneReport()
{
	QSKIP( "Not implemented yet", SkipAll );
}
void PrlAutoReportsTest::testReceiveAutoReport_inSubDirectories_iPhoneAppCrash()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void PrlAutoReportsTest::testReceiveAutoReport_inSubDirectories_LowMemoryFile()
{
	QSKIP( "Not implemented yet", SkipAll );
}



