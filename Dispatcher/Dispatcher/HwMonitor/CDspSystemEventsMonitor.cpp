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

#include "CDspSystemEventsMonitor.h"

#define FORCE_LOGGING_LEVEL DBG_DEBUG
#include <prlcommon/Logging/Logging.h>

#include "CDspService.h"
#include "CDspHwMonitorThread.h"
#include <Libraries/PowerWatcher/PowerWatcher.h>
#include <prlsdk/PrlErrors.h>

#include <QtCore/QCoreApplication>
#include <QScopedPointer>

/**
* Class constructor
*/
CDspSystemEventsMonitor::CDspSystemEventsMonitor()
{
}

/**
* Starts handle device configuration changes
*/
void CDspSystemEventsMonitor::startMonitor()
{
	WRITE_TRACE(DBG_FATAL,
		"System events monitor: %s",
		QThread::isRunning() ? "already started" : "starting");

	QThread::start();
}

/**
* Stop handling device configuration changes
*/
void CDspSystemEventsMonitor::stopMonitor()
{
	WRITE_TRACE(DBG_FATAL,
		"System events monitor: %s",
		QThread::isRunning() ? "stopping" : "already stopped");

	QThread::quit();
	QThread::wait();

	WRITE_TRACE(DBG_FATAL, "System events monitor: was stopped");
}

/*
* Overridden method of thread working body
*/
void CDspSystemEventsMonitor::run()
{
	// create signal receiver in context of _this_ thread for proper event
	// processing
	QScopedPointer<QObject> pHandler( createSystemEventsHandler() );

	// start power monitoring
	if (!CPowerWatcher::startWatcher(pHandler.data(), SLOT(onHostSleep()), SLOT(onHostWakeup())))
	{
		WRITE_TRACE(DBG_FATAL,
			"System events monitor: power monitoring start failed");
		return;
	}

	// initialization complete, start event loop
	WRITE_TRACE(DBG_FATAL, "System events monitor: started");
	QThread::exec();

	// drop all connections and stop monitoring
	CPowerWatcher::disconnectWatcher(pHandler.data());
	CPowerWatcher::stopWatcher();
}

QObject * CDspSystemEventsMonitor::createSystemEventsHandler()
{
	return new CDspSystemEventsHandler;
}


//
// CPmSystemEventsHandler impl.
//

void CDspSystemEventsHandler::onHostSleep()
{
	WRITE_TRACE(DBG_FATAL, "System events monitor: host going to sleep, prepare");

#ifdef _WIN_
	onWinHostSleep();
#endif

	WRITE_TRACE(DBG_FATAL, "System events monitor: host going to sleep, done");
}

void CDspSystemEventsHandler::onHostWakeup()
{
	WRITE_TRACE(DBG_FATAL, "System events monitor: host was wakeup, prepare");

	// process all pending hardware change events
	PRL_ASSERT( CDspService::instance() );
	CDspService::instance()->getHwMonitorThread().forceCheckHwChanges();

#ifdef _WIN_
	onWinHostWakeup();
#endif

	WRITE_TRACE(DBG_FATAL, "System events monitor: host was wakeup, done");
}


