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
///		CommonTestsUtils.cpp
///
/// @author
///		sergeyt, aleksandera
///
/// @brief
///		Common utils using at all dispatcher API tests suites classes.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include <QtTest/QtTest>
#include <QDateTime>

#include "Build/Current.ver"
#include "ParallelsQt.h"
#include "ParallelsDomModel.h"
#include "CommonTestsUtils.h"
#include "Libraries/PrlCommonUtilsBase/CommandLine.h"
#include "Libraries/PrlCommonUtilsBase/ParallelsDirs.h"
#include "Libraries/Logging/Logging.h"

#include "Libraries/Std/PrlAssert.h"

#ifdef _WIN_
	#define strdup _strdup
#endif

const int CONNECTION_TIMEOUT=4000;

namespace {
   char g_sUserName[] = "prl_unit_test_user";
   char g_sUserName2[] = "prl_unit_test_user2";
   char g_sPassword[] = "test";
   char g_sRemoteHostName[] = "127.0.0.1";
   char g_sLocalHostName[] = "xxx";
	char *g_sLongString = NULL;
	bool g_bLongStringAlreadyInitialized = false;

	TestConfig::InitRandom randomInit;
}

PRL_APPLICATION_MODE TestConfig::g_executeMode = PAM_UNKNOWN;
bool TestConfig::g_CtMode = false;
namespace{
	ParallelsDirs::InitOptions g_nInitOptions	= 0;
}


TestConfig::InitRandom::InitRandom()
{
	qsrand( QDateTime::currentDateTime().toTime_t() );
}

void TestConfig::readTestParameters()
{
	TestConfig::g_executeMode = ParallelsDirs::getBuildExecutionMode();
	if( PAM_UNKNOWN != TestConfig::g_executeMode )
	{
		WRITE_TRACE(DBG_INFO, "Run in execute mode: %s",
				ParallelsDirs::getAppExecuteModeAsCString( TestConfig::g_executeMode ) );
		PRL_ASSERT( ParallelsDirs::Init( TestConfig::g_executeMode, g_nInitOptions ) );
		return;
	}

	WRITE_TRACE(DBG_FATAL, "Failed to detect execute mode.");
	PRL_ASSERT(0);
}

bool TestConfig::isExternalBuild()
{
	#ifdef EXTERNALLY_AVAILABLE_BUILD
		return true;
	#else
		return false;
	#endif
}

bool TestConfig::isServerMode()
{
    if (TestConfig::g_executeMode != PAM_SERVER) {
        WRITE_TRACE(DBG_FATAL, "Unsupported value of  TestConfig::g_executeMode." );
        PRL_ASSERT(0);
        throw ("error!");
    }
    return TRUE;
}

bool TestConfig::isServerModePSBM()
{
#ifdef _LIN_
	return PAM_SERVER == TestConfig::g_executeMode;
#endif
	return false;
}

PRL_APPLICATION_MODE TestConfig::getApplicationMode()
{
	return (g_executeMode);
}

PRL_UINT32	TestConfig::getSdkInitFlags()
{
	PRL_UINT32 nFlags = 0;
	if( g_nInitOptions & ParallelsDirs::smAppStoreMode )
		nFlags |= PAIF_INIT_AS_APPSTORE_CLIENT;
	return nFlags;
}

namespace QTest {
template<>
char *toString ( const PVE::IDispatcherCommands &value) {
	return strdup(PVE::DispatcherCommandToString(value));
}

}//namespace QTest

char* TestConfig::getUserLogin()
{
   return g_sUserName;
}

char* TestConfig::getUserLogin2()
{
   return g_sUserName2;
}

char*  TestConfig::getUserPassword()
{
   return g_sPassword;
}

char* TestConfig::getRemoteHostName()
{
   return g_sRemoteHostName;
}

char* TestConfig::getLocalHostName()
{
   return g_sLocalHostName;
}


QString TestConfig::getPathToDispatcherConfig()
{
	QString path=ParallelsDirs::getDispatcherConfigFilePath();
	if ( path.isEmpty() )
		WRITE_TRACE(DBG_FATAL, "Can't get dispatcher config dir");

	return path;
}

bool CheckElementPresentsInList(const QList<QString> &_list, const QString &_elem) {
	for (QList<QString>::const_iterator _it = _list.begin(); _it != _list.end(); ++_it)
		if (_elem == *_it)
			return (true);
	return (false);
}

void InitializeTooLongString() {
	const unsigned short nStrSize = USHRT_MAX;
	if (!g_bLongStringAlreadyInitialized) {
		g_sLongString = new char[nStrSize];
		memset(g_sLongString, 'a', nStrSize);
		g_sLongString[nStrSize-1] = 0;
		g_bLongStringAlreadyInitialized = true;
	}
}

void CleanupTooLongString() {
	if (g_sLongString) {
		delete [] g_sLongString;
		g_sLongString = NULL;
		g_bLongStringAlreadyInitialized = false;
	}
}

char *GetTooLongString() {
	return (g_sLongString);
}

/**
 * Supports "*" and "?" wildcards
 * taken from Libraries/Logging/Logging.cpp
 */
static bool WildcardCompare(const char *s, const char *p)
{
	if (0 == s || 0 == p) return (s == p);

	while ('\0' != *s && '*' != *p)
	{
		if (*s != *p && '?' != *p) return false;
		++s, ++p;
	}

	const char *pm = 0, *sm = 0;

	while ('\0' != *s)
	{
		if ('*' == *p)
		{
			if ('\0' == *++p) return true;
			pm = p;
			sm = s + 1;
		}
		else if (*s == *p || '?' == *p)
		{
			++s, ++p;
		}
		else
		{
			p = pm;
			s = sm++;
		}
	}

	while ('*' == *p) ++p;

	return ('\0' == *p);
}

/*
supported argv format:
TestSet1 TestSet2
TestSet* NewTest
TestSet1 "NewTest3:testName1 testName2"
*/
bool checkArguments(int argc, char **argv, const QString testName, QStringList &testArgsList)
{
	if (argc <= 1) //no arguments
		return true;

	argv ++;

	bool bFound = false;
	//search testName in argv
	while (*argv != NULL)
	{
		QString test( *argv );
		QStringList testArgs = test.split(":", QString::SkipEmptyParts);

		QString testArgsName = testArgs[0];

		if (!WildcardCompare(testName.toUtf8().constData(), testArgsName.toUtf8().constData()))
		{
			argv++;
			continue;
		}

		if (testArgs.size() == 1)
		{
			testArgsList = QStringList();
			return true;
		}

		QString testArgsArgs = testArgs[1];
		//clean args
		testArgsArgs = testArgsArgs.trimmed();

		testArgsList += testArgsArgs.split(" ", QString::SkipEmptyParts);
		if (!bFound)
			testArgsList.insert(0, testName);

		bFound = true;
		argv ++;
	}

	return bFound;
}

