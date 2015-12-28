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
///   System power state handling and control, implementation (Windows specific)
///
/////////////////////////////////////////////////////////////////////////////
//
// Logging engine initialization
//   if defined FORCE_LOGGING_ON debug log output will appear in RELEASE builds,
//   be careful
// NOTE: log level > DBG_INFO can produce large logs
// NOTE: please don't remove defines below,
//   just (un)comment or edit they as needed
#include <prlcommon/Logging/LoggingConfig.h>
// #define FORCE_LOGGING_ON
// #define LOGGING_ON
// DBG_FATAL, DBG_WARNING, DBG_INFO, DBG_DEBUG, DBG_TRACE
// #define FORCE_LOGGING_LEVEL		DBG_TRACE
// #define FORCE_LOGGING_PREFIX	"[]"
#include <prlcommon/Logging/Logging.h>

/////////////////////////////////////////////////////////////////////////////

#include "PowerWatcher.h"

#include <QtCore/QCoreApplication>
#include <QtGui/qwindowdefs_win.h>

#include <powrprof.h>

/////////////////////////////////////////////////////////////////////////////

bool CPowerWatcher::isNotSupported()
{
	return false;
}

bool CPowerWatcher::forceSleep()
{
	if (isNoSleep())
		return false;

	if (!SetSuspendState(FALSE, TRUE, FALSE))
		return false;

	// XXX: workaround for lost PBT_APMSUSPEND
	PostMessage(instance()->m_hwnd, WM_USER, 0, 0);
	return true;
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
	HINSTANCE	hi = qWinAppInst();
	WNDCLASSW	wc;
	HWND		hwnd;
	MSG			msg;

	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc		= _wndProc;
	wc.hInstance		= hi;
	wc.lpszClassName    = L"PowerWatcher";

	if (!RegisterClassW(&wc))
		return;

	hwnd = CreateWindowW(wc.lpszClassName, wc.lpszClassName,
						0, 0, 0, 0, 0, 0, 0, hi, 0);
	if (!hwnd)
		goto fini;

	m_hwnd = hwnd;
	emit onStarted();

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	m_hwnd = NULL;
	emit onStopped();

	DestroyWindow(hwnd);

fini:
	UnregisterClassW(wc.lpszClassName, hi);
}

bool CPowerWatcher::_startWatcher()
{
	QThread::start();

	// wait until run loop really started or thread stopped by error
	do {
		if (IsWindow(m_hwnd))
			return true;

		// NOTE: we should wait and _process_ events to avoid deadlock with
		//   onStarted() signal
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	} while(QThread::isRunning());

	return false;
}

void CPowerWatcher::_stopWatcher()
{
	PostMessage(m_hwnd, WM_QUIT, 0, 0);
//	QThread::quit();

	// NOTE: we should wait and _process_ events to avoid deadlock with
	//   onStopped() signal
	do {
		QCoreApplication::processEvents();
	} while(!QThread::wait(100));
	return;
}

bool CPowerWatcher::_connectWatcher(QObject		*receiver,
									const char	*slotOnSleep,
									const char	*slotOnHasWakeup,
									const char	*slotOnWillWakeup,
									const char	*slotOnCancelSleep,
									const char	*slotOnStarted,
									const char	*slotOnStopped)
{
	// use common variant, but use Qt::BlockingQueuedConnection -- all
	// signals emitted from worker thread
	return _simple_connectWatcher(receiver,
									slotOnSleep,
									slotOnHasWakeup,
									slotOnWillWakeup,
									slotOnCancelSleep,
									slotOnStarted,
									slotOnStopped,
									Qt::BlockingQueuedConnection);
}

bool CPowerWatcher::_isStarted()
{
	return IsWindow(m_hwnd);
}

bool CPowerWatcher::_isSleeping()
{
	return m_isSleeping;
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

/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CPowerWatcher::_wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CPowerWatcher *self = instance();
	if (self->m_hwnd == hwnd)
		return self->wndProc(msg, wp, lp);

	return  DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT CPowerWatcher::wndProc(UINT msg, WPARAM wp, LPARAM lp)
{
	// XXX: workaround for lost PBT_APMSUSPEND
	if (msg == WM_USER) {
		// good, PBT_APMSUSPEND already received
		if (m_isSleeping)
			return 0;

		m_isSleeping = true;
		emit onSleep();
		return 0;
	}

	if (msg != WM_POWERBROADCAST)
		return  DefWindowProc(m_hwnd, msg, wp, lp);

	switch (wp) {
	case PBT_APMQUERYSUSPEND:
		if (m_countNoSleep == 0 || m_isSleeping)
			return TRUE;

		emit onCancelSleep();
		return BROADCAST_QUERY_DENY;

	case PBT_APMSUSPEND:
		// XXX: workaround for lost PBT_APMSUSPEND, WM_USER arrived first
		if (m_isSleeping)
			return TRUE;

		m_isSleeping = true;
		emit onSleep();
		return TRUE;

	case PBT_APMRESUMEAUTOMATIC:
		if (!m_isSleeping) {
			// Ups, we/system lost PBT_APMSUSPEND, try to recover state
			m_isSleeping = true;
			// XXX: may be this is bad idea, but this is recover signal sequence
			emit onSleep();
		}

		emit onWillWakeup();
		m_isSleeping = false;
		emit onHasWakeup();
		return TRUE;
	}

	return  DefWindowProc(m_hwnd, msg, wp, lp);
}
