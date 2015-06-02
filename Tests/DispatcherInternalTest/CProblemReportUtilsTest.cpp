///////////////////////////////////////////////////////////////////////////////
///
/// @file CProblemReportUtilsTest.cpp
///
/// Tests suite for problem report utilities
///
/// @author sandro
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

#include <QDir>
#include <QFileInfo>

#include "CProblemReportUtilsTest.h"
#include "Libraries/PrlUuid/Uuid.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"

void CProblemReportUtilsTest::init()
{
	QVERIFY(m_Auth.AuthUserBySelfProcessOwner());
	m_sTargetDirPath = QDir::tempPath() + "/" + Uuid::createUuid().toString() + "/";
	QVERIFY(CFileHelper::WriteDirectory( m_sTargetDirPath, &m_Auth ));
	QVERIFY(CFileHelper::WriteDirectory( m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER, &m_Auth ));
	QVERIFY(CFileHelper::WriteDirectory( m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER, &m_Auth ));
}

void CProblemReportUtilsTest::cleanup()
{
	CFileHelper::ClearAndDeleteDir( m_sTargetDirPath );
}

void CProblemReportUtilsTest::testSL_10_6_x_x86_32_kernel_case()
{
	QString sExpectedPath = m_sTargetDirPath + "panic.log";
	QVERIFY(CFileHelper::CreateBlankFile( sExpectedPath, &m_Auth ));
	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( sExpectedPath ));
}

void CProblemReportUtilsTest::testSL_10_6_x_x86_64_kernel_case()
{
	QString sExpectedPath = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201410_MacBook-Pro-Alexander-Aplemakh.panic";
	QVERIFY(CFileHelper::CreateBlankFile( sExpectedPath, &m_Auth ));
	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( sExpectedPath ));
}

void CProblemReportUtilsTest::testSL_10_5_x_case()
{
	QString sExpectedPath = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143451.panic";
	QVERIFY(CFileHelper::CreateBlankFile( sExpectedPath, &m_Auth ));
	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( sExpectedPath ));
}

void CProblemReportUtilsTest::testMixedCase_10_6_x_x86_32_kernel_panic_newest()
{
	QString s10_6_x_x86_32_panic = m_sTargetDirPath + "panic.log";
	QString s10_6_x_x86_64_panic = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201410_MacBook-Pro-Alexander-Aplemakh.panic";
	QString s10_5_x_panic = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143451.panic";
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_32_panic, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_64_panic, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_5_x_panic, &m_Auth ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_6_x_x86_32_panic, QDateTime::currentDateTime().addSecs( 10 ) ));

	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( s10_6_x_x86_32_panic ));
}

void CProblemReportUtilsTest::testMixedCase_10_6_x_x86_64_kernel_panic_newest()
{
	QString s10_6_x_x86_32_panic = m_sTargetDirPath + "panic.log";
	QString s10_6_x_x86_64_panic = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201410_MacBook-Pro-Alexander-Aplemakh.panic";
	QString s10_5_x_panic = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143451.panic";
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_32_panic, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_64_panic, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_5_x_panic, &m_Auth ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_6_x_x86_64_panic, QDateTime::currentDateTime().addSecs( 10 ) ));

	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( s10_6_x_x86_64_panic ));
}

void CProblemReportUtilsTest::testMixedCase_10_5_x_panic_newest()
{
	QString s10_6_x_x86_32_panic = m_sTargetDirPath + "panic.log";
	QString s10_6_x_x86_64_panic = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201410_MacBook-Pro-Alexander-Aplemakh.panic";
	QString s10_5_x_panic = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143451.panic";
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_32_panic, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_64_panic, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_5_x_panic, &m_Auth ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_5_x_panic, QDateTime::currentDateTime().addSecs( 10 ) ));

	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( s10_5_x_panic ));
}

void CProblemReportUtilsTest::testSL_10_6_x_x86_64_several_panics()
{
	QString s10_6_x_x86_64_panic_1 = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201410_MacBook-Pro-Alexander-Aplemakh.panic";
	QString s10_6_x_x86_64_panic_2 = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201420_MacBook-Pro-Alexander-Aplemakh.panic";
	QString s10_6_x_x86_64_panic_3 = m_sTargetDirPath + PRL_MAC_OS_10_6_PANICS_SUBFOLDER + "/Kernel_2010-04-21-201430_MacBook-Pro-Alexander-Aplemakh.panic";
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_64_panic_1, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_64_panic_2, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_6_x_x86_64_panic_3, &m_Auth ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_6_x_x86_64_panic_2, QDateTime::currentDateTime().addSecs( 10 ) ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_6_x_x86_64_panic_3, QDateTime::currentDateTime().addSecs( 20 ) ));

	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( s10_6_x_x86_64_panic_3 ));
}

void CProblemReportUtilsTest::testSL_10_5_x_several_panics()
{
	QString s10_5_x_panic_1 = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143451.panic";
	QString s10_5_x_panic_2 = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143501.panic";
	QString s10_5_x_panic_3 = m_sTargetDirPath + PRL_MAC_OS_10_5_PANICS_SUBFOLDER + "/2009-10-19-143511.panic";
	QVERIFY(CFileHelper::CreateBlankFile( s10_5_x_panic_1, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_5_x_panic_2, &m_Auth ));
	QVERIFY(CFileHelper::CreateBlankFile( s10_5_x_panic_3, &m_Auth ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_5_x_panic_2, QDateTime::currentDateTime().addSecs( 10 ) ));
	QVERIFY(CFileHelper::SetFileModificationTime( s10_5_x_panic_3, QDateTime::currentDateTime().addSecs( 20 ) ));

	QString sActualPath = CProblemReportUtils::GetLastPanicLogPath( m_sTargetDirPath );
	QCOMPARE(QFileInfo( sActualPath ), QFileInfo( s10_5_x_panic_3 ));
}

void CProblemReportUtilsTest::testParseAppVersion_fromMacCrashReport()
{

#define CHECK_VER( expRes, expBuild, expRev, line )	\
	do{ \
		QString sBuild, sRev; \
		bool res =  CProblemReportUtils::getAppVersionFromLineOfMacCrashDump( \
			line, sBuild, sRev ); \
		QCOMPARE( res, expRes ); \
		if( !res ) break; \
		QCOMPARE( sBuild, QString(expBuild) ); \
		QCOMPARE( sRev, QString(expRev) ); \
	}while(0);

	CHECK_VER( true, "22444", "903761", "Version: 9.0 (22444.903761)" );
	CHECK_VER( true, "22444", "", "Version: 10.0.0 (22444)" );
	CHECK_VER( false, "", "", " Version: 9.0 (22444.903761)");
	CHECK_VER( false, "", "", "OS Version:      Mac OS X 10.8.4 (12E55)");
	CHECK_VER( false, "", "", "Report Version:  10" );

#undef CHECK_VER
}

void CProblemReportUtilsTest::testParseStubVersion_fromMacCrashReport()
{
#define CHECK_VER( expRes, expMajorVer, line )	\
	do{ \
		QString sMajorVer; \
		bool res =  CProblemReportUtils::getStubVersionFromLineOfMacCrashDump( \
			line, sMajorVer ); \
		QCOMPARE( res, expRes ); \
		if( !res ) break; \
		QCOMPARE( sMajorVer, QString(expMajorVer) ); \
	}while(0);

	CHECK_VER( true, "9", "Version:         Windows 7 x64 Ee (9.0.22495.913787)" );
	CHECK_VER( true, "10", "Version:         Windows 7 x64 Ee (10.0.22495)" );
	CHECK_VER( false, "", "Version: 9.0 (23036.915397)");

#undef CHECK_VER
}

void CProblemReportUtilsTest::testParseDrvVersion_fromMacPanicReport()
{

#define CHECK_VER( expRes, expBuild, expRev, line )	\
	do{ \
		QString sBuild, sRev; \
		bool res =  CProblemReportUtils::getDrvVersionFromLineOfMacPanicDump( \
			line, sBuild, sRev ); \
		QCOMPARE( res, expRes ); \
		if( !res ) break; \
		QCOMPARE( sBuild, QString(expBuild) ); \
		QCOMPARE( sRev, QString(expRev) ); \
	}while(0);

	CHECK_VER( true, "11828", "615184" \
		, "com.parallels.kext.prl_vnic	6.0 11828.615184" );

	CHECK_VER( true, "12094", "676494" \
		, "com.parallels.kext.prl_hid_hook	6.0 12094.676494" );

	CHECK_VER( true, "23456", "" \
		, "com.parallels.kext.prl_hid_hook	10.0.0 23456" );
}

void CProblemReportUtilsTest::testParseLowMemoryDumpOfMobileApp_isJettisoned()
{

#define CHECK_APP( expRes, line )	\
	do{ \
		bool res =  CProblemReportUtils::isJettisonedMobileApp_fromLineOfLowMemoryDump( \
			"RemoteClient", line ); \
		QCOMPARE( res, expRes ); \
	}while(0);


	// 1. TEST "TRUE" cases
	CHECK_APP( true \
		, "RemoteClient <76fae65507e83dc5b2fe01de4126ade6>    6543 (jettisoned)" );
	// add tabs before
	CHECK_APP( true \
		, "		RemoteClient <76fae65507e83dc5b2fe01de4126ade6>    6543 (jettisoned)" );
	// add spaces before
	CHECK_APP( true \
		, "  RemoteClient <76fae65507e83dc5b2fe01de4126ade6>    6543 (jettisoned)" );
	// add "(active)"
	CHECK_APP( true \
		, "     RemoteClient <da377209df383c908f2466b856748f12> 11858 (jettisoned) (active)" );

	// TEST FALSE CASES

	// no jettisoned
	CHECK_APP( false \
		, "   RemoteClient <da377209df383c908f2466b856748f12> 11858 (active)" );
	CHECK_APP( false \
		, "      RemoteClient <da377209df383c908f2466b856748f12> 11858 " );
	// another app
	CHECK_APP( false \
		, "MobileStorageMou <da377209df383c908f2466b856748f12> 11858 (jettisoned) (active)" );
	CHECK_APP( false \
		, "        lsd <3fafa485b73836acb973d50feabd963a> 53" );

#undef CHECK_APP
}

void CProblemReportUtilsTest::testExtractPCSIsoVersion()
{
	QString sInput("Parallels Cloud Server 6.0.6 (1992)");
	QString sResult = CProblemReportUtils::extractIsoVersion(sInput);

	QCOMPARE(sResult, QString("6.0.6-1992"));
}

