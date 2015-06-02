/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 1999-2015 Parallels IP Holdings GmbH
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
/// @file PowerWatcher.cpp
///
/// @brief
///   System power state handling and control, implementation (Linux specific)
///
/////////////////////////////////////////////////////////////////////////////
//
// Logging engine initialization
//   if defined FORCE_LOGGING_ON debug log output will appear in RELEASE builds,
//   be careful
// NOTE: log level > DBG_INFO can produce large logs
// NOTE: please don't remove defines below,
//   just (un)comment or edit they as needed
#include "Libraries/Logging/LoggingConfig.h"
// #define FORCE_LOGGING_ON
// #define LOGGING_ON
// DBG_FATAL, DBG_WARNING, DBG_INFO, DBG_DEBUG, DBG_TRACE
// #define FORCE_LOGGING_LEVEL		DBG_TRACE
// #define FORCE_LOGGING_PREFIX	"[]"
#include "Libraries/Logging/Logging.h"

/////////////////////////////////////////////////////////////////////////////

#include "PowerWatcher.h"

/////////////////////////////////////////////////////////////////////////////

bool CPowerWatcher::isNotSupported()
{
	// TODO: not implemented
	return true;
}

bool CPowerWatcher::forceSleep()
{
	// TODO: not implemented
	return false;
}

bool CPowerWatcher::scheduleWakeup(unsigned delay)
{
	// TODO: not implemented
	Q_UNUSED(delay);
	return false;
}

/////////////////////////////////////////////////////////////////////////////

void CPowerWatcher::run()
{
	// TODO: not implementd
	QThread::run();
}

bool CPowerWatcher::_startWatcher()
{
	// TODO: not implementd
	return _simple_startWatcher();
}

void CPowerWatcher::_stopWatcher()
{
	// TODO: not implementd
	return _simple_stopWatcher();
}

bool CPowerWatcher::_connectWatcher(QObject		*receiver,
									const char	*slotOnSleep,
									const char	*slotOnHasWakeup,
									const char	*slotOnWillWakeup,
									const char	*slotOnCancelSleep,
									const char	*slotOnStarted,
									const char	*slotOnStopped)
{
	// TODO: not implementd
	return _simple_connectWatcher(receiver,
									slotOnSleep,
									slotOnHasWakeup,
									slotOnWillWakeup,
									slotOnCancelSleep,
									slotOnStarted,
									slotOnStopped,
									Qt::AutoConnection);
}

bool CPowerWatcher::_isStarted()
{
	// TODO: not implementd
	return _simple_isStarted();
}

bool CPowerWatcher::_isSleeping()
{
	// TODO: not implementd
	return false;
}

bool CPowerWatcher::_lockNoSleep()
{
	// TODO: not implementd
	return _simple_lockNoSleep();
}

void CPowerWatcher::_unlockNoSleep()
{
	// TODO: not implementd
	_simple_unlockNoSleep();
}

bool CPowerWatcher::_isNoSleep()
{
	// TODO: not implementd
	return _simple_isNoSleep();
}
