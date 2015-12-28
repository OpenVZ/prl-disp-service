///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspSystemEventsMonitor_win.cpp
///
/// system events monitor thread implementation
///
/// @author sergeyt
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
///////////////////////////////////////////////////////////////////////////////

#define FORCE_LOGGING_LEVEL DBG_DEBUG
#include <prlcommon/Logging/Logging.h>

#include "CDspSystemEventsMonitor.h"

// for define VTD_HOOK_EVENT_NAME
#include "Monitor/Interfaces/ApiVtd.h"

#include "CDspClient.h"
#include "CDspService.h"
#include "CDspCommon.h"

#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <prlcommon/Std/SmartPtr.h>
#include "Tasks/Task_PrepareForHibernate.h"

#include <windows.h>


void CDspSystemEventsHandler::onWinHostSleep()
{
	// create some event to block hibernate continuing
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, L"Global\\" VTD_HOOK_EVENT_NAME );
	if (hEvent == INVALID_HANDLE_VALUE)
	{
		WRITE_TRACE( DBG_FATAL,
			"System events monitor: failed to open VTd hook event (0x%08x)",
			GetLastError() );

		// FIXME: May be need stop all VMs instead return
		return;
	}

	PRL_RESULT ret = PRL_ERR_UNEXPECTED;
	COMMON_TRY
	{
		SmartPtr<CDspClient> pUser( new CDspClient(IOSender::Handle()) );
		pUser->getAuthHelper().AuthUserBySelfProcessOwner();
		const SmartPtr<IOPackage> p( DispatcherPackage::createInstance( PVE::DspCmdCtlDispatherFakeCommand ) );

		Task_PrepareForHibernate task( pUser, p );
		ret = task.prepareForHostSuspend( false );
	}
	COMMON_CATCH;

	SetEvent(hEvent);
	CloseHandle(hEvent);

	if ( PRL_FAILED( ret ) )
	{
		WRITE_TRACE( DBG_FATAL,
				"System events monitor: prepare for Win host sleep failed (%s)",
				PRL_RESULT_TO_STRING(ret), ret);
	}
}

void CDspSystemEventsHandler::onWinHostWakeup()
{
	PRL_ASSERT( CDspService::instance() );
	PRL_RESULT ret = CDspService::instance()->getShellServiceHelper().afterHostResume();

	if ( PRL_FAILED( ret ) )
	{
		WRITE_TRACE( DBG_FATAL,
				"System events monitor: prepare for Win host wakeup failed (%s)",
				PRL_RESULT_TO_STRING(ret));
	}
}
