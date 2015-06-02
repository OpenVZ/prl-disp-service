///////////////////////////////////////////////////////////////////////////////
///
/// @file Monitor.cpp
///
/// This file contains definitions of stub fuctions needed for
/// testing Monitor/Std/
///
/// @author vtatarinov
/// @owner alexeyk
///
/// Copyright (c) 2007-2015 Parallels IP Holdings GmbH
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

#include <QtTest/QtTest>
#include "Tests/MonitorStdTest/Monitor.h"

int num_vcpus = 1;


void Abort( char* sFormatStr, ... )
{
	va_list Args;
	int		uResult;


	va_start(Args, sFormatStr);

    {
        static char pStrBuf[ 512 ];
		uResult = _vsnprintf(pStrBuf, sizeof(pStrBuf), sFormatStr, Args);
		if (uResult > 0)
			printf(" %s\n", pStrBuf);
    }

	va_end(Args);

	throw uResult;
}

void LogMessage( int iLogLevel, char* sFormatStr, ... )
{
	va_list Args;
	int		uResult;

	if( iLogLevel > DBG_LEVEL )
		return;


	va_start(Args, sFormatStr);

	{
		static char pStrBuf[ 512 ];
		uResult = _vsnprintf(pStrBuf, sizeof(pStrBuf), sFormatStr, Args);
		if (uResult > 0)
			printf(" %s\n", pStrBuf);
	}

	va_end(Args);
}

MON_STATE MonState;
