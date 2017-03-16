/////////////////////////////////////////////////////////////////////////////
///
///	@file ApiInitDeinitTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests suite for checking attempts on SDK methods call when PrlApi_Init wasn't called before.
///
///	@author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////

#include <QCoreApplication>
#include "ApiInitDeinitTest.h"

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include "Tests/CommonTestsUtils.h"

void ApiInitDeinitTest::test()
{
	const int nIterationsNum = 100;
	for (int i = 0; i < nIterationsNum; ++i)
	{
		CHECK_RET_CODE_EXP(PrlApi_InitEx(PARALLELS_API_VER, TestConfig::getApplicationMode(), TestConfig::getSdkInitFlags(), 0))
		{
			SdkHandleWrap hServer;
			CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
			SdkHandleWrap hJob(PrlSrv_LoginLocal(hServer, "", 0, PSL_HIGH_SECURITY));
			CHECK_JOB_RET_CODE(hJob)
		}
		CHECK_RET_CODE_EXP(PrlApi_Deinit())
		WRITE_TRACE(DBG_FATAL, "Iteration %d completed", i+1);
	}
}

void TestsExecuter::PushTestsExecution()
{
#ifdef DYN_API_WRAP
	if ( !SdkWrap_LoadLibFromStdPaths(true) )
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to initialize SDK lib");
		QCoreApplication::exit(-1);
		return;
	}
#endif

	ApiInitDeinitTest _tests_suite;
	int nRet = QTest::qExec(&_tests_suite, argc, argv);

	QCoreApplication::exit(nRet);
}

void DeinitSdk()
{
	PrlApi_Deinit();
}

int main(int argc, char *argv[])
{
	TestConfig::readTestParameters();
	int ret = 0;
	QCoreApplication a(argc, argv);

	TestsExecuter _tests_executer(argc, argv);
	QTimer::singleShot(0, &_tests_executer, SLOT(PushTestsExecution()));
	ret = a.exec();
#ifdef DYN_API_WRAP
	SdkWrap_Unload();
#endif
	return (ret);
}

