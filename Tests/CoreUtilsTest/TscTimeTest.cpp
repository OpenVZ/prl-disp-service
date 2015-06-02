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
///		TscTimeTest.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing TscTime functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "TscTimeTest.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/Std/PrlTime.h"
#include "Libraries/Logging/Logging.h"

void TscTimeTest::test()
{
#ifdef _WIN_
	QSKIP("Skip until https://jira.sw.ru/browse/PDFWL-7115 will be fixed", SkipAll);
#endif
	PRL_UINT64	timeout_sec = 2;
	PRL_UINT64  tsc1 = PrlTicks();
	PRL_UINT64  tsc2;

	HostUtils::Sleep(timeout_sec * 1000);

	PRL_UINT64 tscDelta = PrlTicksToSeconds(PrlTicksDelta(tsc1, &tsc2));

	QVERIFY(tsc2 > tsc1);
	QCOMPARE(tscDelta, timeout_sec);
}
