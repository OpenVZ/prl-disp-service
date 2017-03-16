/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandleServerJobTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing server job handle.
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
#include "PrlHandleServerJobTest.h"

#include "SDK/Handles/Disp/PrlHandleServerJob.h"

#include <prlcommon/PrlUuid/Uuid.h>

PrlHandleServerJobTest::PrlHandleServerJobTest()
{}

void PrlHandleServerJobTest::cleanup()
{
	m_Handle.reset();
}

void PrlHandleServerJobTest::testJobGetRetCodeOnWrongRetCodePtr()
{
	PrlHandleBase *pHandle = new PrlHandleServerJob(PrlHandleServerPtr(0), Uuid::createUuid(), PJOC_UNKNOWN);
	m_Handle.reset(pHandle->GetHandle());
	QVERIFY(PrlJob_GetRetCode(m_Handle, 0) == PRL_ERR_INVALID_ARG);
}
