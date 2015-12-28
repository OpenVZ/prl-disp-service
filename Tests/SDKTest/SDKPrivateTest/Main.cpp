/////////////////////////////////////////////////////////////////////////////
///
///	@file Main.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	SDK API tests entry point.
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
#include <QCoreApplication>
#include <QtTest/QtTest>

#include <prlsdk/Parallels.h>

#include <prlcommon/Logging/Logging.h>
#include "Tests/CommonTestsUtils.h"

#include "PrlHandleSmartPtrTest.h"
#include "PrlApiTest.h"
#include "PrlHandleServerJobTest.h"
#include "PrlApiEventsTest.h"
#include "PrlHandleVmDeviceTest.h"
#include "PrlHandleVmTest.h"
#include "PrlQuestionsListTest.h"
#include "MigrationFlagsMacrosesTest.h"
#include "PrlHandlePluginInfoTest.h"

int main(int argc, char *argv[])
{
	PrlApi_Init(PARALLELS_API_VER);

	int nRet = 0;
	EXECUTE_TESTS_SUITE(PrlHandleSmartPtrTest)
	EXECUTE_TESTS_SUITE(PrlApiTest)
	EXECUTE_TESTS_SUITE(PrlHandleServerJobTest)
	EXECUTE_TESTS_SUITE(PrlApiEventsTest)
	EXECUTE_TESTS_SUITE(PrlHandleVmDeviceTest)
	EXECUTE_TESTS_SUITE(PrlHandleVmTest)
	EXECUTE_TESTS_SUITE(PrlQuestionsListTest)
	EXECUTE_TESTS_SUITE(MigrationFlagsMacrosesTest)
	EXECUTE_TESTS_SUITE(PrlHandlePluginInfoTest)
	PrlApi_Deinit();
	PRL_UINT32 nHandlesNum = 0;
	PrlDbg_GetHandlesNum(&nHandlesNum, PHT_ERROR);
	LOG_MESSAGE(DBG_FATAL, "Non cleaned handles instances %d", nHandlesNum);
	for (PRL_HANDLE_TYPE i = PHT_SERVER; i <= PHT_LAST; ++(unsigned int &)i)
		OUTPUT_HANDLES_NUM_FOR_TYPE(i);
	return nRet;
}
