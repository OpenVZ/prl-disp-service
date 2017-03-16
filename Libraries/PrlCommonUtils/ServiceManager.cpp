/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 1999-2017, Parallels International GmbH
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
/// @file ServiceManager.cpp
///
/// @brief
///         ServiceManager class implementation file.
///
/////////////////////////////////////////////////////////////////////////////

#include <aclapi.h>
#include <prlcommon/Std/PrlAssert.h>

#include "ServiceManager.h"

ServiceManager::ServiceManager( const wchar_t* szSvcName )
:	schSCManager(NULL)
,	schService( NULL )
{
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // servicesActive database
		SC_MANAGER_ALL_ACCESS);  // full access rights

	if (NULL == schSCManager)
	{
		WRITE_TRACE( DBG_FATAL, "OpenSCManager failed (%d)", GetLastError());
		return;
	}

	// Get a handle to the service.
	schService = OpenService(
		schSCManager,         // SCM database
		szSvcName,            // name of service
		SERVICE_ALL_ACCESS);  // full access

	if (schService == NULL)
	{
		WRITE_TRACE( DBG_FATAL, "OpenService failed (%d)", GetLastError());
		return;
	}
}

ServiceManager::~ServiceManager()
{
	if( schService )
		CloseServiceHandle(schService);

	if(schSCManager)
		CloseServiceHandle(schSCManager);
}

bool ServiceManager::inited( bool bTraceWhenFail )
{
	bool ret = schSCManager && schService;
	if( !ret && bTraceWhenFail )
	{
		WRITE_TRACE( DBG_FATAL, "ServiceManager doesn't initialized! schSCManager = %p, schService = %p"
			, schSCManager, schService );
	}
	return ret;
}

bool ServiceManager::getStatus(SERVICE_STATUS_PROCESS& ssStatus)
{
	if( !inited() )
		return false;

	DWORD dwBytesNeeded = 0;
	if (!QueryServiceStatusEx(
		schService,                     // handle to service
		SC_STATUS_PROCESS_INFO,         // information level
		(LPBYTE) &ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded ) )              // size needed if buffer is too small
	{
		WRITE_TRACE( DBG_FATAL, "getStatus(): QueryServiceStatusEx failed (%d)", GetLastError());
		return false;
	}
	return true;
}

bool ServiceManager::restartService()
{
	if( !inited() )
		return false;

	SERVICE_STATUS_PROCESS ssStatus;
	if( !getStatus( ssStatus ) )
		return false;

	bool bRet = false;
	switch( ssStatus.dwCurrentState )
	{
	case SERVICE_START_PENDING:
		if( !waitTillStarted() )
			break;
		// goto stop without break
	case SERVICE_RUNNING:
		if( !stopService() )
			break;
	case SERVICE_STOP_PENDING:
		if( !waitTillStopped() )
			break;
		// goto start without break
	case SERVICE_STOPPED:
		bRet = startService();
		break;

	case SERVICE_CONTINUE_PENDING:
	case SERVICE_PAUSE_PENDING:
	case SERVICE_PAUSED:
	default:
		WRITE_TRACE( DBG_FATAL, "Unprocessed state %#x", ssStatus.dwCurrentState );
	}

	WRITE_TRACE( DBG_FATAL, "service was %s restarted.", bRet?"":"not" );
	return bRet;
}


bool ServiceManager::startService()
{
	if( !inited() )
		return false;

	// Attempt to start the service.
	if (!StartService(
		schService,  // handle to service
		0,           // number of arguments
		NULL) )      // no arguments
	{
		WRITE_TRACE( DBG_FATAL, "StartService failed (%d)", GetLastError());
		return false;
	}
	else
		WRITE_TRACE( DBG_INFO, "Service start pending...");

	return waitTillStarted();
}

bool ServiceManager::stopService()
{
	if( !inited() )
		return false;

	SERVICE_STATUS_PROCESS ssStatus;
	if ( !ControlService(
		schService,
		SERVICE_CONTROL_STOP,
		(LPSERVICE_STATUS) &ssStatus ) )
	{
		WRITE_TRACE( DBG_FATAL, "ControlService failed (%d)", GetLastError() );
		return false;
	}
	else
		WRITE_TRACE( DBG_INFO, "Service stop pending...");

	return waitTillStopped();
}

bool ServiceManager::waitTillStarted()
{
	return wait( weWaitToStart );
}

bool ServiceManager::waitTillStopped()
{
	return wait( weWaitToStop );
}

bool ServiceManager::wait( WaitEvent evt )
{
	if( !inited() )
		return false;

	DWORD dwPendingState = 0, dwTargetState = 0;
	if( evt == weWaitToStart )
	{
		dwTargetState = SERVICE_RUNNING;
		dwPendingState = SERVICE_START_PENDING;
	}
	else if (evt == weWaitToStop )
	{
		dwTargetState = SERVICE_STOPPED;
		dwPendingState = SERVICE_STOP_PENDING;
	}
	else
		PRL_ASSERT( "Ops" == NULL );

	SERVICE_STATUS_PROCESS ssStatus;
	if( !getStatus( ssStatus ) )
		return false;

	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out
	DWORD dwWaitTime;

	while ( ssStatus.dwCurrentState == dwPendingState )
	{
		// Do not wait longer than the wait hint. A good interval is
		// one-tenth of the wait hint but not less than 1 second
		// and not more than 10 seconds.

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if( dwWaitTime < 1000 )
			dwWaitTime = 1000;
		else if ( dwWaitTime > 10000 )
			dwWaitTime = 10000;

		Sleep( dwWaitTime );

		if( !getStatus( ssStatus ) )
			return false;

		if ( GetTickCount() - dwStartTime > dwTimeout )
		{
			WRITE_TRACE( DBG_FATAL, "Timed out.\n");
			break;
		}
	}

	if (ssStatus.dwCurrentState == dwTargetState )
	{
		WRITE_TRACE( DBG_FATAL, "Service %s successfully.", (evt == weWaitToStart)?"started":"stopped" );
		return true;
	}

	WRITE_TRACE( DBG_FATAL, "Service is unable to %s.",  (evt == weWaitToStart)?"start":"stop");
	return false;
}
