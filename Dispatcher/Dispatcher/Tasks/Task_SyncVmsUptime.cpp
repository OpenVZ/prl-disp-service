////////////////////////////////////////////////////////////////////////////////
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
/// Task implementation which updates periodically VM uptime values a their configurations
///
/// @owner sergeym@
/// @author	sandro@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_SyncVmsUptime.h"
#include "CDspService.h"
#include "CDspVmManager.h"
#include <prlcommon/Std/PrlAssert.h>

#include "CDspVzHelper.h"

#include <QTimer>

Task_SyncVmsUptime::Task_SyncVmsUptime(const SmartPtr<CDspClient> &user,	const SmartPtr<IOPackage> &p) :
CDspTaskHelper(user, p)
{}

Task_SyncVmsUptime::~Task_SyncVmsUptime()
{}

void Task_SyncVmsUptime::cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &)
{
	exit(0);
}

void Task_SyncVmsUptime::onTimeoutEvent()
{
	CDspService::instance()->getVmManager().syncVMsUptime();
#ifdef _LIN_
	CDspService::instance()->getVzHelper()->syncCtsUptime();
#endif
}

PRL_RESULT Task_SyncVmsUptime::run_body()
{
	quint32 nTimeout =
		CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->getVmUptimeSyncTimeoutInMinutes();
	if ( ! nTimeout )
	{
		WRITE_TRACE(DBG_FATAL, "Synchronization up time task was not started due to timeout is zero !");
		return (PRL_ERR_SUCCESS);
	}

	SmartPtr<QTimer> pTimer(new QTimer);
	bool bConnected = connect(pTimer.getImpl(), SIGNAL(timeout()), SLOT(onTimeoutEvent()));
	PRL_ASSERT(bConnected);
	pTimer->start(nTimeout*60*1000);
	exec();
	return (PRL_ERR_SUCCESS);
}

