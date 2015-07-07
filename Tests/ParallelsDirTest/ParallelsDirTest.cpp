/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2007-2015 Parallels IP Holdings GmbH
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
///		ParallelsDirTest.cpp
///
/// @author
///		sergeyt@
///
/// @brief
///		Tests fixture class for testing ParallelsDirs class functionality.
///
/// @brief
///
/// last version of specification is available on http://wiki/index.php/Paths_to_Configuration_Files
///
/////////////////////////////////////////////////////////////////////////////
#include "ParallelsDirTest.h"
#include "Libraries/PrlCommonUtilsBase/ParallelsDirs.h"
#include "Libraries/PrlCommonUtils/CAuthHelper.h"

#include "Interfaces/ParallelsQt.h"
#include "Libraries/Logging/Logging.h"
#include "Tests/CommonTestsUtils.h"
#include "Build/Current.ver"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

void ParallelsDirTest::testGetDispatcherConfigDir()
{
	QVERIFY (currentProcessHasRootPermission());

	QString expectedPath;
	QString path=ParallelsDirs::getDispatcherConfigDir();

	switch(getOsType())
	{
	case osWinXp: expectedPath="C:/Documents and Settings/All Users/Application Data/Parallels";
		break;
	case osWinVista: expectedPath="C:/ProgramData/Parallels";
		break;
	case osLinux: expectedPath="/etc/parallels";
		break;
	case osMac: expectedPath="/Library/Preferences/Parallels";
		break;
	default:
		QFAIL ("Someshit happend.");
	}

	QCOMPARE(path, expectedPath);
}
void ParallelsDirTest::testGetCallerUserPreferencesDir()
{
	QString userName;

	QVERIFY(currentProcessHasRootPermission());

	userName=TestConfig::getUserLogin();

	QString path;
	QString expectedPath = QDir::homePath();

	unixImpersonateTo(userName);
	path=ParallelsDirs::getCallerUserPreferencesDir();
	unixRevertToSelf();

	switch(getOsType())
	{
	case osWinXp: expectedPath=expectedPath + "/Application Data/Parallels";
		break;
	case osWinVista: expectedPath=expectedPath + "/AppData/Roaming/Parallels";
		break;
	case osLinux: expectedPath= linuxGetUserHomePath(userName) + "/.parallels";
		break;
	case osMac: expectedPath= "/Users/" + userName + "/Library/Preferences/Parallels";
		break;
	default:
		QFAIL ("Someshit happend.");
	}
	QCOMPARE(path, expectedPath);
}

void ParallelsDirTest::testGetDefaultVmCatalogue_serverMode()
{
	QVERIFY (currentProcessHasRootPermission());

	QString path;
	QString expectedPath;

	path=ParallelsDirs::getCommonDefaultVmCatalogue();

	switch(getOsType())
	{
	case osWinXp: expectedPath="C:/Documents and Settings/All Users/Documents/Public Parallels";
		break;
	case osWinVista: expectedPath="C:/Users/Public/Documents/Public Parallels";
		break;
	case osLinux: expectedPath="/var/parallels";
		break;
	case osMac: expectedPath="/Users/Shared/Parallels";
		break;
	default:
		QFAIL ("Someshit happend.");
	}
	QCOMPARE(path, expectedPath);
}


void ParallelsDirTest::testGetDefaultVmCatalogue_desktopMode()
{
	QVERIFY (currentProcessHasRootPermission());

	CAuthHelper authUser1(TestConfig::getUserLogin());
	CAuthHelper* authUser=&authUser1;

	QVERIFY(authUser->AuthUser(TestConfig::getUserPassword()));

	ParallelsDirs::UserInfo userInfo(TestConfig::getUserLogin()
		, authUser->getHomePath() );

	QString path;
	QString expectedPath;


	path=ParallelsDirs::getUserDefaultVmCatalogue(&userInfo);

	switch(getOsType())
	{
	case osWinVista:
		break;
	case osWinXp:
		break;
	case osLinux: expectedPath=linuxGetUserHomePath(TestConfig::getUserLogin())+"/parallels";
		break;
	case osMac: expectedPath=QString("/Users/")
						+ TestConfig::getUserLogin() + "/Documents/Parallels";
		break;
	default:
		QFAIL ("Someshit happend.");
	}
	QCOMPARE(path, expectedPath);
}


void ParallelsDirTest::testGetDefaultVmCatalogue_desktopMode2()
{
	QVERIFY (currentProcessHasRootPermission());

	CAuthHelper authUser1(TestConfig::getUserLogin2());
	CAuthHelper* authUser=&authUser1;

	QVERIFY(authUser->AuthUser(TestConfig::getUserPassword()));


	ParallelsDirs::UserInfo userInfo(TestConfig::getUserLogin()
		, authUser->getHomePath() );

	QString path;
	QString expectedPath;


	path=ParallelsDirs::getUserDefaultVmCatalogue(&userInfo);

	switch(getOsType())
	{
	case osWinVista:
		break;
	case osWinXp:
		break;
	case osLinux: expectedPath=linuxGetUserHomePath(TestConfig::getUserLogin())+"/parallels";
		break;
	case osMac: expectedPath=QString("/Users/")
						+ TestConfig::getUserLogin() + "/Documents/Parallels";
		break;
	default:
		QFAIL ("Someshit happend.");
	}
	QCOMPARE(path, expectedPath);
}


void ParallelsDirTest::testGetToolsBaseImagePath()
{
	// NOTE: windows path depends from product installation path
	PRL_APPLICATION_MODE mode;
	QString path;
	QString expectedPath;

	// Server
	mode = PAM_SERVER;
	path = ParallelsDirs::getToolsBaseImagePath(mode);
	expectedPath = "/usr/share/parallels-server/tools/";
	QCOMPARE(path, expectedPath);
}


void ParallelsDirTest::testGetToolsImage()
{
	PRL_APPLICATION_MODE mode = PAM_SERVER;
	unsigned int nOsVersion;
	QString path;
	QString expectedPath;

	nOsVersion = PVS_GUEST_VER_LIN_OTHER;
	path = ParallelsDirs::getToolsImage(mode, nOsVersion);
	expectedPath = "/usr/share/parallels-server/tools/prl-tools-lin.iso";
	QCOMPARE(path, expectedPath);

	nOsVersion = PVS_GUEST_VER_WIN_OTHER;
	path = ParallelsDirs::getToolsImage(mode, nOsVersion);
	expectedPath = "/usr/share/parallels-server/tools/prl-tools-win.iso";
	QCOMPARE(path, expectedPath);
}


void ParallelsDirTest::testGetLinReconfigImage()
{
	QString expectedPath;
	QString path = ParallelsDirs::getLinReconfigImage(PAM_UNKNOWN);

	expectedPath = "/usr/share/parallels-reconfiguration/reconfiguration.iso";

	QCOMPARE(path, expectedPath);
}


void ParallelsDirTest::testGetProductPath()
{
	QString expectedPath;
	QString path = ParallelsDirs::getProductPath();

	expectedPath = "/usr/share/parallels-desktop/";

	QCOMPARE(path, expectedPath);
}


bool ParallelsDirTest::currentProcessHasRootPermission()
{
	return 0==getuid();
}

ParallelsDirTest::OsType ParallelsDirTest::getOsType()
{
	return osLinux;
}

void ParallelsDirTest::unixImpersonateTo(const QString& userName)
{
	m_last_euid=geteuid();
	struct passwd* pwd=getpwnam(QSTR2UTF8(userName));
	if (!pwd)
	{
		LOG_MESSAGE(DBG_FATAL, "getpwnam() failed by error %d", errno);
		return;
	}
	seteuid(pwd->pw_uid);
}

void ParallelsDirTest::unixRevertToSelf()
{
	seteuid(m_last_euid);
}

QString ParallelsDirTest::linuxGetUserHomePath(const QString& userName)
{
	m_last_euid=geteuid();
	struct passwd* pwd=getpwnam(QSTR2UTF8(userName));
	if (!pwd)
	{
		LOG_MESSAGE(DBG_FATAL, "getpwnam() failed by error %d", errno);
		return "";
	}
	return UTF8_2QSTR(pwd->pw_dir);
}

QString ParallelsDirTest::winGetUserHomePathPrefix(CAuthHelper *pUser)
{
	QString sUserHomePrefix = pUser->getHomePath();
	if (!sUserHomePrefix.size())
	{
		switch (getOsType())
		{
			case osWinXp:
				sUserHomePrefix = QString("C:/Documents and Settings/") + pUser->getUserName();
			break;
			case osWinVista:
				sUserHomePrefix = QString("C:/Users/") + pUser->getUserName();
			break;
			default:
			break;
		}
	}
	return (sUserHomePrefix);
}

