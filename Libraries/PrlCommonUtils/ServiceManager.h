/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 1999 - 2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file ServiceManager.h
///
/// @brief
///			ServiceManager class is wrap to Win API ещ control windows services
///
/////////////////////////////////////////////////////////////////////////////

#ifndef ServiceManager_H
#define ServiceManager_H

#include <windows.h>

struct SC_HANDLE__ ;
class ServiceManager
{
	typedef enum WaitEvent { weWaitToStart, weWaitToStop };

public:
	ServiceManager( const wchar_t* szSvcName );
	~ServiceManager();

	bool inited( bool bTraceWhenFail = true );
	bool restartService();

	bool startService();
	bool stopService();

protected:
	bool waitTillStarted();
	bool waitTillStopped();

private:
	bool wait(WaitEvent evt );
	bool getStatus(SERVICE_STATUS_PROCESS& ssStatus);

private:
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
};

#endif //ServiceManager_H
