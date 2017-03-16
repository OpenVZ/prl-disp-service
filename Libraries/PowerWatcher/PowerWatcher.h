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
/// @file PowerWatcher.h
///
/// @brief
///   System power state handling and control, public interface
///
/////////////////////////////////////////////////////////////////////////////
#if !defined(POWER_WATCHER__H)
#define POWER_WATCHER__H

/////////////////////////////////////////////////////////////////////////////

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#ifdef _WIN_
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////

class CPowerWatcher : public QThread
{
	Q_OBJECT

private:
	// NOTE: CPowerWatcher have virtual destructor (inherited from QThread).
	//   This is cause the problem at destruction of static object of this
	//   class (trivial singleton implementation via static object)
	//   To prevent that we use simple helper class that wrap real instance and
	//   uses explict new-delete logic that don't have problems with virtual
	//   destructors.
	class CPowerWatcherInstance
	{
	private:
		QMutex			m_lock;
		CPowerWatcher	*m_instance;

	public:
		CPowerWatcher *instance()
		{
			QMutexLocker _l(&m_lock);
			if (m_instance == NULL)
				m_instance = new CPowerWatcher();
			return m_instance;
		}

		CPowerWatcherInstance()
			: m_instance(NULL) {}
		~CPowerWatcherInstance() { delete m_instance; }
	};

	static CPowerWatcherInstance s_instance;

protected:
	// this is singleton
	CPowerWatcher();
	virtual ~CPowerWatcher();
	CPowerWatcher(const CPowerWatcher&);
	CPowerWatcher& operator=(const CPowerWatcher&);

	// return singleton instance
	static CPowerWatcher* instance() { return s_instance.instance(); }

	// internal implementation (platform specific)
	virtual void run();
	bool _startWatcher();
	void _stopWatcher();
	bool _connectWatcher(QObject		*receiver,
							const char	*slotOnSleep,
							const char	*slotOnHasWakeup,
							const char	*slotOnWillWakeup,
							const char	*slotOnCancelSleep,
							const char	*slotOnStarted,
							const char	*slotOnStopped);
	bool _isStarted();
	bool _isSleeping();
	bool _lockNoSleep();
	void _unlockNoSleep();
	bool _isNoSleep();

	// generic simple ("fake") implementation for some platforms
	bool _simple_startWatcher();
	void _simple_stopWatcher();
	bool _simple_connectWatcher(QObject				*receiver,
								const char			*slotOnSleep,
								const char			*slotOnHasWakeup,
								const char			*slotOnWillWakeup,
								const char			*slotOnCancelSleep,
								const char			*slotOnStarted,
								const char			*slotOnStopped,
								Qt::ConnectionType	type);
	bool _simple_isStarted();
	bool _simple_lockNoSleep();
	void _simple_unlockNoSleep();
	bool _simple_isNoSleep();

	bool m_isStarted;
	int m_countNoSleep;
	bool m_isSleeping;

#ifdef _WIN_
	HWND m_hwnd;

	static LRESULT CALLBACK _wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	LRESULT  wndProc(UINT msg, WPARAM wp, LPARAM lp);
#endif

public:
    enum DarkWakeMode
    {
        darkWakeForCurrent = 0,
        darkWakeForExternal = 1,
        darkWakeForBattery = 2,
    };
	// start power state monitoring
	// optionally connect to CPowerWatcher signals with proper attributes
	// return true on success
	static bool startWatcher(QObject	*receiver			= NULL,
							const char	*slotOnSleep		= NULL,
							const char	*slotOnHasWakeup	= NULL,
							const char	*slotOnWillWakeup	= NULL,
							const char	*slotOnCancelSleep	= NULL,
							const char	*slotOnStarted		= NULL,
							const char	*slotOnStopped		= NULL);

	// stop power state monitoring
	static void stopWatcher();

	// connect to signals emmited by CPowerWatcher with proper connection
	// attributes
	static bool connectWatcher(QObject	*receiver,
								const char	*slotOnSleep,
								const char	*slotOnHasWakeup,
								const char	*slotOnWillWakeup,
								const char	*slotOnCancelSleep,
								const char	*slotOnStarted,
								const char	*slotOnStopped);

	// drop all connections with specified object
	static bool disconnectWatcher(QObject *receiver);

	// return true if state monitoring started
	static bool isStarted();

	// return true if system sleeping (between onSleep() and onHasWakeup())
	static bool isSleeping();

    // sets period and working time for dark wake mode
    // return true if parameters are applyed
    // currently implemented only for debug
    static bool setDarkWakeParameter(unsigned period, unsigned time);

    // turns on/off dark wake (disable by default)
    static bool setDarkWakeEnable(bool enable);

    // check dark wake status for specified mode
    static bool isDarkWakeEnabled(DarkWakeMode mode = darkWakeForCurrent);

    // currently is actual only for Mac OS >= 10.8 with PowerNAP
    // return true if host wakes up in dark mode (with no TV)
    static bool isDarkWake();

	// force system to sleep
	// NOTE: may be not supported (or ignored) by platform (return false)
	static bool forceSleep();

	// schedule system wakeup (delay in seconds from current moment)
	// NOTE: may be not supported (or ignored) by platform (return false)
	static bool scheduleWakeup(unsigned delay);

	// try disable sleep, can be nested
	// return true if we already sleepeing (no-effect)
	// NOTE: this is "soft" guard, some kind of sleeps can't be
	//   disabled (a.k.a FORCED sleep).
	static bool lockNoSleep();

	// re-enable sleep
	static void unlockNoSleep();

	// return true if sleep disabled by lockNoSleep()
	static bool isNoSleep();

	// return true if power monitoring not supported by this platform
	// i.e. no way to get state notifications (signals):
	// onSleep(), onHasWakeup(), onWillWakeup() and onCancelSleep()
	// _but_ PowerWatcher can be started (fake): isStarted() call and
	// onStarted(), onStopped() sinlas will be worked.
	static bool isNotSupported();

	// helper ("locker") class to lockNoSleep()/unlockNoSleep()
	class CNoSleepLocker
	{
	private:
		bool m_isSleeping;
	public:
		CNoSleepLocker() { m_isSleeping = lockNoSleep(); }
		~CNoSleepLocker() { unlockNoSleep(); }
		bool isSleeping() { return m_isSleeping; }
	};

signals:
	// emitted when system starting sleep transition
	void onSleep();
	// emitted when system is completely wakeup
	void onHasWakeup();
	// emitted when system starting wakeup
	void onWillWakeup();
	// emitted in case of system try to enter sleep, but this is canceled
	void onCancelSleep();

	// emitted when monitoring has started by startWatcher()
	void onStarted();
	// emitted when monitoring has stopped by stopWatcher()
	void onStopped();
};

/////////////////////////////////////////////////////////////////////////////

#endif // POWER_WATCHER__H
//EOF
