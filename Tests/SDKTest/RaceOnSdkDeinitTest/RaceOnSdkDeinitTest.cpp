/////////////////////////////////////////////////////////////////////////////
///
///	@file RaceOnSdkDeinitTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests suite for checking attempts on SDK methods call when PrlApi_Init wasn't called before.
///
///	@author sandro
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
/////////////////////////////////////////////////////////////////////////////

#include "RaceOnSdkDeinitTest.h"

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include "Tests/CommonTestsUtils.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>

namespace {
class CSdkOperator :  public QThread{
public:
	CSdkOperator(QWaitCondition &_cond, QMutex &_mutex)
	: m_cond(_cond), m_mutex(_mutex)
	{}

private:
	void run()
	{
		SdkHandleWrap hServer, hLoginLocalJob, hResult, hResultParam;
		{
			#define CHECK_EXPRESSION(expression)\
			{\
				PRL_RESULT nExprRetCode = expression;\
				if (PRL_FAILED(nExprRetCode))\
				{\
					WRITE_TRACE(DBG_FATAL, "Expression '%s' failed with ret_code=%.8X '%s'", #expression, nExprRetCode, PRL_RESULT_TO_STRING(nExprRetCode));\
					m_cond.wakeAll();\
					return;\
				}\
			}

			QMutexLocker _lock(&m_mutex);
			PrlApi_InitEx(PARALLELS_API_VER, TestConfig::getApplicationMode(), TestConfig::getSdkInitFlags(), 0);
			CHECK_EXPRESSION(PrlSrv_Create(hServer.GetHandlePtr()))
			hLoginLocalJob.reset(PrlSrv_LoginLocal(hServer, "", 0, PSL_HIGH_SECURITY));
			CHECK_EXPRESSION(PrlJob_Wait(hLoginLocalJob, PRL_JOB_WAIT_TIMEOUT))
			PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
			CHECK_EXPRESSION(PrlJob_GetRetCode(hLoginLocalJob, &nRetCode))
			CHECK_EXPRESSION(nRetCode)

			SdkHandleWrap hJob(PrlSrv_Logoff(hServer));
			CHECK_EXPRESSION(PrlJob_Wait(hLoginLocalJob, PRL_JOB_WAIT_TIMEOUT))
			m_cond.wakeAll();
		}
		CHECK_EXPRESSION(PrlJob_GetResult(hLoginLocalJob, hResult.GetHandlePtr()))
		CHECK_EXPRESSION(PrlResult_GetParam(hResult, hResultParam.GetHandlePtr()))
		CHECK_EXPRESSION(PrlResult_GetParamByIndex(hResult, 0, hResultParam.GetHandlePtr()))
	}

private:
	QWaitCondition &m_cond;
	QMutex &m_mutex;
};

class CSdkDeiniter :  public QThread
{
public:
	CSdkDeiniter(QWaitCondition &_cond, QMutex &_mutex)
	: m_cond(_cond), m_mutex(_mutex)
	{}

private:
	void run()
	{
		{
			QMutexLocker _lock(&m_mutex);
			m_cond.wakeAll();
			m_cond.wait(&m_mutex);
		}
		PrlApi_Deinit();
	}

private:
	QWaitCondition &m_cond;
	QMutex &m_mutex;
};

}

void RaceOnSdkDeinitTest::test()
{
	QWaitCondition _cond;
	QMutex _mutex;
	CSdkDeiniter _sdk_deiniter(_cond, _mutex);
	CSdkOperator _sdk_operator(_cond, _mutex);
	{
		QMutexLocker _lock(&_mutex);
		_sdk_deiniter.start();
		_cond.wait(&_mutex);
	}
	_sdk_operator.start();
	_sdk_deiniter.wait();
	_sdk_operator.wait();
}

int main(int argc, char *argv[])
{
	TestConfig::readTestParameters(argv[0]);
#ifdef DYN_API_WRAP
	if ( !SdkWrap_LoadLibFromStdPaths() )
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to load SDK lib");
		return (-1);
	}
#endif

	RaceOnSdkDeinitTest _tests_suite;
	int nRet = QTest::qExec(&_tests_suite, argc, argv);

#ifdef DYN_API_WRAP
	SdkWrap_Unload();
#endif
	return (nRet);
}

