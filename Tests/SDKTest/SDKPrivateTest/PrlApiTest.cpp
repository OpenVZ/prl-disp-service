/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlApiTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing SDK API calls.
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
#include "PrlApiTest.h"
#include "SDK/Handles/Disp/PrlHandleServerJob.h"
#include "Tests/CommonTestsUtils.h"

#include <prlcommon/PrlUuid/Uuid.h>
#include <prlcommon/Interfaces/ParallelsDomModel.h>

PrlApiTest::PrlApiTest()
{}

void PrlApiTest::cleanup()
{
	if (m_JobHandle)
	{
		m_JobHandle.reset(PrlSrv_Logoff(m_Handle));
		PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	}
	m_Handle.reset();
	m_JobHandle.reset();
}

void PrlApiTest::testLogoffOnNonServerHandle() {
	PrlHandleBase *pHandle = new PrlHandleServerJob(PrlHandleServerPtr(0), Uuid::createUuid(), PJOC_UNKNOWN);
	m_Handle.reset(pHandle->GetHandle());
	CHECK_ASYNC_OP_FAILED(PrlSrv_Logoff(m_Handle), PRL_ERR_INVALID_ARG);
}

void PrlApiTest::testLoginOnNonServerHandle()
{
	PrlHandleBase *pHandle = new PrlHandleServerJob(PrlHandleServerPtr(0), Uuid::createUuid(), PJOC_UNKNOWN);
	m_Handle.reset(pHandle->GetHandle());
	CHECK_ASYNC_OP_FAILED(PrlSrv_Login(m_Handle, TestConfig::getRemoteHostName(),
					TestConfig::getUserLogin(), TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY), PRL_ERR_INVALID_ARG);
}
