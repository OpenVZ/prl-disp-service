///////////////////////////////////////////////////////////////////////////////
///
/// @file ServiceControl.h
/// @author sdmitry
///
/// Simple class for controlling Win32-services
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <windows.h>

class CServiceControl
{
public:
	CServiceControl();
	~CServiceControl();

	bool operator !();

	static void RegisterApplicationLog( LPCTSTR ServiceName, LPCTSTR ServiceFile );
	static void DeregisterApplicationLog( LPCTSTR ServiceName );

	BOOL InstallService( LPCTSTR ServiceName, LPCTSTR ServiceFile, LPCTSTR ServiceDesc, DWORD ServiceType, DWORD StartType );
	BOOL RemoveService( LPCTSTR ServiceName );
	BOOL StartService( LPCTSTR ServiceName );
	BOOL StopService( LPCTSTR ServiceName );
	BOOL QueryServiceStatus(LPCTSTR ServiceName, SERVICE_STATUS *pServiceStatus);
	BOOL ControlService(LPCTSTR ServiceName, DWORD dwControlCode);

private:
	SC_HANDLE m_hScm;
};
