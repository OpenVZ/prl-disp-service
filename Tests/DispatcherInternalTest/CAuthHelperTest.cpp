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
///		CAuthHelperTest.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing CAuthHelper class functionality.
/// 		To get more info look at bug 1963 http://bugzilla.parallels.ru/show_bug.cgi?id=1963
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CAuthHelperTest.h"
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include <prlcommon/Logging/Logging.h>
#include "Tests/CommonTestsUtils.h"

#include <prlcommon/PrlCommonUtilsBase/SysError.h>

#ifdef _WIN_
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif



// get from SDK\Handles\PrlHandleServer.cpp
namespace {
	/**
	 * Returns currently logged in user id. Under Win platform it's access token of current process.
	 * Under Unix like platforms - UID.
	 */
	qlonglong PrlGetCurrentUserId()
	{
#ifdef _WIN_
		return ((qlonglong)GetCurrentProcessId());
#else
		return (getuid());
#endif
	}

}


CAuthHelperTest::CAuthHelperTest()
{

}

CAuthHelperTest::~CAuthHelperTest()
{

}

void CAuthHelperTest::cleanup()
{

}



QString CAuthHelperTest::getCurrentDomain()
{
	QString domain;
#ifdef _WIN_
	if ( !CAuth::getCurrentDomain( domain ) )
		WRITE_TRACE(DBG_FATAL, "ERROR!!!: CAuth::getCurrentDomain() return false" );
	else
#endif
	if ( domain.isEmpty() )
		domain = getLocalDomain();
	else
		domain = domain.toLower(); //normalize

	return domain;
}


QString CAuthHelperTest::getLocalDomain()
{
	return ".";
}

CAuthHelperTest::UserInfo CAuthHelperTest::getDomainUser()
{
	return UserInfo ( "prl_unit_test_user", "test" );
}

CAuthHelperTest::UserInfo CAuthHelperTest::getLocalUser()
{
	return UserInfo ( "prl_local_test_user", "1q2w3e4r5t" );
}

CAuthHelperTest::UserInfo CAuthHelperTest::getCurrentUser()
{
	UserInfo userInfo;

#ifdef _WIN_

	TCHAR pszUname[100];
	ZeroMemory(pszUname, sizeof(pszUname));
	DWORD dwSize = 100;
	if (GetUserName(pszUname, &dwSize))
		userInfo.name = UTF16SZ_2QSTR(pszUname, dwSize-1);
	else
		LOG_MESSAGE(DBG_FATAL, "Couldn't to extract user name. Error code: %d", GetLastError());

#else




#endif


	return userInfo;
}

bool CAuthHelperTest::isLocalUserName( const QString& userName)
{
#ifndef _WIN_
	return true;
#endif
	QString cmdline = QString (" net user %1 > NUL" ).arg( userName );
	int ret = system( QSTR2UTF8( cmdline ) );
	return ret == 0;
}

bool CAuthHelperTest::isHostInWindowsDomain()
{
	return isWinOs() && getCurrentDomain() != getLocalDomain();
}

bool CAuthHelperTest::isWinOs()
{
#	ifdef _WIN_
		return true;
#	else
		return false;
#	endif
}


quint32 CAuthHelperTest::GetId_ToLocalLogin()
{
	return PrlGetCurrentUserId();
}


void CAuthHelperTest::TestLocalLogin_AsLocalUser()
{
	QSKIP (" Not implemented: invalid implemented method createSimpleProcessAsUser() ", SkipAll);

	if ( !isWinOs() )
		QSKIP( "skipped on unix" , SkipSingle );

	UserInfo localUser = getLocalUser();
	QVERIFY ( isLocalUserName( localUser.name ) );

	CAuthHelper localAuth( localUser.name );
	QVERIFY( localAuth.AuthUser( localUser.password ) );

	Q_PID pid = createSimpleProcessAsUser ( localAuth );

	QVERIFY ( pid != 0 );

	UserInfo user = getLocalUser();
	QVERIFY ( isLocalUserName( user.name ) );

	CAuthHelper auth;
#	ifdef _WIN_
		QVERIFY ( auth.AuthUser( pid->dwProcessId ) );
#	endif

	QCOMPARE( auth.getUserName()		, user.name );
	QCOMPARE( auth.getUserDomain()	, getLocalDomain() );
}

void CAuthHelperTest::TestLocalLogin_AsDomainUser()
{
	QSKIP ("method is not implemented", SkipAll);

	UserInfo user = getCurrentUser();

	CAuthHelper auth;

	if ( isWinOs() && ! isHostInWindowsDomain() )
		QVERIFY ( !auth.AuthUser( GetId_ToLocalLogin() ) );
	else
	{
		QVERIFY ( auth.AuthUser( GetId_ToLocalLogin() ) );
		QCOMPARE( auth.getUserName()		, user.name );
		QCOMPARE( auth.getUserDomain()	, getCurrentDomain() );
	}
}

void CAuthHelperTest::TestRemoteLogin_AsLocalUser()
{
	if ( !isWinOs() )
		QSKIP( "skipped on unix" , SkipSingle );

	UserInfo user = getLocalUser();

	CAuthHelper auth( user.name );

	QVERIFY( isLocalUserName( user.name ) );

	QVERIFY ( auth.AuthUser( user.password ) );

	QCOMPARE( auth.getUserName()		, user.name );
	QCOMPARE( auth.getUserDomain()	, getLocalDomain() );
}

void CAuthHelperTest::TestRemoteLogin_AsDomainUser()
{
	UserInfo user = getDomainUser();

	CAuthHelper auth( user.name );

	QVERIFY( !isWinOs() || ! isLocalUserName( user.name ) );

	if ( isWinOs() && ! isHostInWindowsDomain())
		QVERIFY ( ! auth.AuthUser( user.password ) ); //reject
	else
	{
		QVERIFY ( auth.AuthUser( user.password ) );
		QCOMPARE( auth.getUserName()		, user.name );
		QCOMPARE( auth.getUserDomain()	, getCurrentDomain() );
	}
}

void CAuthHelperTest::TestRemoteLogin_AsDomainUserWithDomainName()
{
	UserInfo user = getDomainUser();

	CAuthHelper auth( QString ( "%1/%2" )
		.arg( getCurrentDomain() )
		.arg ( user.name ) );

	if ( isWinOs() && ! isHostInWindowsDomain())
		QVERIFY ( ! auth.AuthUser( user.password ) ); //reject
	else
	{
		QVERIFY ( auth.AuthUser( user.password ) );
		QCOMPARE( auth.getUserName()		, user.name );
		QCOMPARE( auth.getUserDomain()	, getCurrentDomain() );
	}
}

void CAuthHelperTest::TestRemoteLogin_AsDomainUserWithDomainName_WithBackspash()
{
	UserInfo user = getDomainUser();

	CAuthHelper auth( QString ( "%1\\%2" )
		.arg( getCurrentDomain() )
		.arg ( user.name ) );

	if ( isWinOs() && ! isHostInWindowsDomain())
		QVERIFY ( ! auth.AuthUser( user.password ) ); //reject
	else
	{
		QVERIFY ( auth.AuthUser( user.password ) );
		QCOMPARE( auth.getUserName()		, user.name );
		QCOMPARE( auth.getUserDomain()	, getCurrentDomain() );
	}
}


//////////////////////////////

Q_PID CAuthHelperTest::createSimpleProcessAsUser( CAuthHelper& auth )
{
#	ifndef _WIN_
	Q_UNUSED( auth );
	return 0;
#else

	PROCESS_INFORMATION* pid = new PROCESS_INFORMATION;
	try
	{
		void *pToken = NULL;
		if ( ! DuplicateTokenEx( auth.GetAuth()->GetTokenHandle(),	MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &pToken) )
			throw "DuplicateTokenEx failed";

		memset(pid, 0, sizeof(PROCESS_INFORMATION));

		DWORD dwCreationFlags = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;

		QString args = "sleep 2";
		wchar_t szCmd[512] = { 0 };
		args.toWCharArray( szCmd );

		QStringList params;
		params.append( "2" );

		//QString args ;= create_commandline ( "sleep", params);
		//LPWSTR* szCmdline=wcsdup(L"sleep 2");
		//WCHAR args[]= L"sleep 2";
		if ( ! CreateProcessAsUserW( pToken,0, szCmd , 0, 0, FALSE, dwCreationFlags, 0, 0, 0, pid) )
			throw "CreateProcessAsUserW() failed";

		return pid;
	}
	catch (const char* err)
	{
		LOG_MESSAGE( DBG_FATAL, "error catched [%s], err=[%s]", err, Prl::GetLastErrorAsString() );
		Q_UNUSED(err);
		return 0;
	}

#endif
}

QString CAuthHelperTest::create_commandline ( const QString& program,
									 const QStringList& arguments )
{
	QString programName = program;
	if (!programName.startsWith(QLatin1Char('\"')) &&
		!programName.endsWith(QLatin1Char('\"')) && programName.contains(" "))
		programName = "\"" + programName + "\"";
	programName.replace("/", "\\");

	QString args;
	// add the prgram as the first arrg ... it works better
	args = programName + " ";
	for (int i=0; i<arguments.size(); ++i) {
		QString tmp = arguments.at(i);
		// in the case of \" already being in the string
		// the \ must also be escaped
		tmp.replace( "\\\"", "\\\\\"" );
		// escape a single " because the arguments will be parsed
		tmp.replace( "\"", "\\\"" );
		if (tmp.isEmpty() || tmp.contains(' ') || tmp.contains('\t')) {
			// The argument must not end with a \ since this would be
			// interpreted as escaping the quote -- rather put the \ behind
			// the quote: e.g. rather use "foo"\ than "foo\"
			QString endQuote("\"");
			int i = tmp.length();
			while (i>0 && tmp.at(i-1) == '\\') {
				--i;
				endQuote += "\\";
			}
			args += QString(" \"") + tmp.left(i) + endQuote;
		} else {
			args += ' ' + tmp;
		}
	}
	return args;
}


void CAuthHelperTest::testGetOwnerOfFile()
{

}
void CAuthHelperTest::testSetOwnerOfFile()
{

}
void CAuthHelperTest::testIsOwnerOfFile()
{

}

void CAuthHelperTest::testLoginAsTestUser()
{
	CAuthHelper auth( TestConfig::getUserLogin() );
	QVERIFY( auth.AuthUser( TestConfig::getUserPassword() ) );
}

void CAuthHelperTest::testLoginAsTestUser_WithWrongLoginName()
{
	CAuthHelper auth( TestConfig::getUserLogin()
		+ QString("-%1").arg(qrand()) );
	QVERIFY( !auth.AuthUser( TestConfig::getUserPassword()) );
}

void CAuthHelperTest::testLoginAsTestUser_WithWrongPassword()
{
	CAuthHelper auth( TestConfig::getUserLogin() );
	QVERIFY( !auth.AuthUser( TestConfig::getUserPassword()
		+ QString("-%1").arg(qrand()) ) );
}
