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
///   System power state handling and control, implementation (all platforms)
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

CPowerWatcher::CPowerWatcherInstance CPowerWatcher::s_instance;

/////////////////////////////////////////////////////////////////////////////

CPowerWatcher::CPowerWatcher()
	: m_isStarted(false)
	, m_countNoSleep(0)
	, m_isSleeping(false)
{
#ifdef _WIN_
	m_hwnd = NULL;
#endif
}

CPowerWatcher::~CPowerWatcher()
{
	// stop monitoring if not stopped before
	// NOTE: avoid calls to instance() method here to avoid deadlock
	if (_isStarted())
		_stopWatcher();
}

bool CPowerWatcher::startWatcher(QObject	*receiver,
								const char	*slotOnSleep,
								const char	*slotOnHasWakeup,
								const char	*slotOnWillWakeup,
								const char	*slotOnCancelSleep,
								const char	*slotOnStarted,
								const char	*slotOnStopped)
{
	// nested call, nothing to do
	if (isStarted())
		return true;

	if (receiver) {
		if (!connectWatcher(receiver,
							slotOnSleep,
							slotOnHasWakeup,
							slotOnWillWakeup,
							slotOnCancelSleep,
							slotOnStarted,
							slotOnStopped))
			return false;
	}

	// forward to platform specific code
	return instance()->_startWatcher();
}

void CPowerWatcher::stopWatcher()
{
	// nested call, nothing to do
	if (!isStarted())
		return;

	// forward to platform specific code
	instance()->_stopWatcher();
}

bool CPowerWatcher::connectWatcher(QObject	*receiver,
								const char	*slotOnSleep,
								const char	*slotOnHasWakeup,
								const char	*slotOnWillWakeup,
								const char	*slotOnCancelSleep,
								const char	*slotOnStarted,
								const char	*slotOnStopped)
{
	// forward to platform specific code
	return instance()->_connectWatcher(receiver,
										slotOnSleep,
										slotOnHasWakeup,
										slotOnWillWakeup,
										slotOnCancelSleep,
										slotOnStarted,
										slotOnStopped);
}

bool CPowerWatcher::disconnectWatcher(QObject *receiver)
{
	Q_ASSERT(receiver);
	return instance()->disconnect(receiver);
}

bool CPowerWatcher::isStarted()
{
	// forward to platform specific code
	return instance()->_isStarted();
}

bool CPowerWatcher::isSleeping()
{
	// forward to platform specific code
	return instance()->_isSleeping();
}

#ifndef _MAC_

bool CPowerWatcher::isDarkWakeEnabled(DarkWakeMode mode)
{
    Q_UNUSED(mode);
    return false;
}

bool CPowerWatcher::isDarkWake()
{
    return false;
}

bool CPowerWatcher::setDarkWakeParameter(unsigned period, unsigned time)
{
    Q_UNUSED(period);
    Q_UNUSED(time);
    return false;
}

bool CPowerWatcher::setDarkWakeEnable(bool enable)
{
    Q_UNUSED(enable);
    return false;
}
#endif

bool CPowerWatcher::lockNoSleep()
{
	// forward to platform specific code
	return instance()->_lockNoSleep();
}

void CPowerWatcher::unlockNoSleep()
{
	// forward to platform specific code
	instance()->_unlockNoSleep();
}

bool CPowerWatcher::isNoSleep()
{
	// forward to platform specific code
	return instance()->_isNoSleep();
}

/////////////////////////////////////////////////////////////////////////////

bool CPowerWatcher::_simple_startWatcher()
{
	m_isStarted = true;
	emit onStarted();
	return true;
}

void CPowerWatcher::_simple_stopWatcher()
{
	m_isStarted = false;
	emit onStopped();
}

bool CPowerWatcher::_simple_connectWatcher(QObject				*receiver,
											const char			*slotOnSleep,
											const char			*slotOnHasWakeup,
											const char			*slotOnWillWakeup,
											const char			*slotOnCancelSleep,
											const char			*slotOnStarted,
											const char			*slotOnStopped,
											Qt::ConnectionType	type)
{
	Q_ASSERT(receiver);

	if (slotOnSleep) {
		if (!receiver->connect(this, SIGNAL(onSleep()), slotOnSleep, type))
			return false;
	}
	if (slotOnHasWakeup) {
		if (!receiver->connect(this, SIGNAL(onHasWakeup()), slotOnHasWakeup, type))
			return false;
	}
	if (slotOnWillWakeup) {
		if (!receiver->connect(this, SIGNAL(onWillWakeup()), slotOnWillWakeup, type))
			return false;
	}
	if (slotOnCancelSleep) {
		if (!receiver->connect(this, SIGNAL(onCancelSleep()), slotOnCancelSleep, type))
			return false;
	}
	if (slotOnStarted) {
		if (!receiver->connect(this, SIGNAL(onStarted()), slotOnStarted, type))
			return false;
	}
	if (slotOnStopped) {
		if (!receiver->connect(this, SIGNAL(onStopped()), slotOnStopped, type))
			return false;
	}

	return true;
}

bool CPowerWatcher::_simple_isStarted()
{
	return m_isStarted;
}

bool CPowerWatcher::_simple_lockNoSleep()
{
	m_countNoSleep++;
	return isSleeping();
}

void CPowerWatcher::_simple_unlockNoSleep()
{
	m_countNoSleep--;
}

bool CPowerWatcher::_simple_isNoSleep()
{
	return m_countNoSleep > 0;
}

