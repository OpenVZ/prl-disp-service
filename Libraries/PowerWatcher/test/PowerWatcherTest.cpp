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
/// @file PowerWatcherTest.cpp
///
/// @brief
///   System power state handling and control, test application
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

#include "PowerWatcherTest.h"

/////////////////////////////////////////////////////////////////////////////

// Use QtTest framework

QTEST_MAIN(CPowerWatcherTest)

/////////////////////////////////////////////////////////////////////////////

CPowerWatcherTest::CPowerWatcherTest()
	: m_countOnStarted(0)
	, m_countOnStopped(0)
	, m_countOnSleep(0)
	, m_countOnHasWakeup(0)
	, m_countOnWillWakeup(0)
	, m_countOnCancelSleep(0)
	, m_testIsStarted(false)
	, m_testIsSleeping(false)
{

}

void CPowerWatcherTest::onStarted()
{
	m_countOnStarted++;
	qDebug("Signal CPowerWatcher::onStarted() received: %d", m_countOnStarted);

	if (m_testIsStarted)
		QVERIFY(CPowerWatcher::isStarted());
}

void CPowerWatcherTest::onStopped()
{
	m_countOnStopped++;
	qDebug("Signal CPowerWatcher::onStopped() received: %d", m_countOnStopped);

	if (m_testIsStarted)
		QVERIFY(!CPowerWatcher::isStarted());
}

void CPowerWatcherTest::onSleep()
{
	m_countOnSleep++;
	qDebug("Signal CPowerWatcher::onSleep() received: %d", m_countOnSleep);

	if (m_testIsSleeping)
		QVERIFY(CPowerWatcher::isSleeping());
}

void CPowerWatcherTest::onHasWakeup()
{
	m_countOnHasWakeup++;
	qDebug("Signal CPowerWatcher::onHasWakeup() received: %d", m_countOnHasWakeup);

	if (m_testIsSleeping)
		QVERIFY(!CPowerWatcher::isSleeping());
}

void CPowerWatcherTest::onWillWakeup()
{
	m_countOnWillWakeup++;
	qDebug("Signal CPowerWatcher::onWillWakeup() received: %d", m_countOnWillWakeup);

	if (m_testIsSleeping)
		QVERIFY(CPowerWatcher::isSleeping());
}

void CPowerWatcherTest::onCancelSleep()
{
	m_countOnCancelSleep++;
	qDebug("Signal CPowerWatcher::onCancelSleep() received: %d", m_countOnCancelSleep);
}

/////////////////////////////////////////////////////////////////////////////

void CPowerWatcherTest::init()
{
	// check initial state
	QVERIFY(!CPowerWatcher::isStarted());
	QVERIFY(!CPowerWatcher::isSleeping());
	QVERIFY(!CPowerWatcher::isNoSleep());

	// clear signal counters
	m_countOnStarted		= 0;
	m_countOnStopped		= 0;
	m_countOnSleep			= 0;
	m_countOnHasWakeup		= 0;
	m_countOnWillWakeup		= 0;
	m_countOnCancelSleep	= 0;

	// start watcher and connect to all signals
	QVERIFY(CPowerWatcher::startWatcher(this,
										SLOT(onSleep()),
										SLOT(onHasWakeup()),
										SLOT(onWillWakeup()),
										SLOT(onCancelSleep()),
										SLOT(onStarted()),
										SLOT(onStopped())));
}

void CPowerWatcherTest::cleanup()
{
	// common cleanup: stop monitoring and disconnect from this object
	CPowerWatcher::stopWatcher();
	CPowerWatcher::disconnectWatcher(this);
}

/////////////////////////////////////////////////////////////////////////////

void CPowerWatcherTest::testStartStop()
{
	// enable checks inside signal handlers
	m_testIsStarted = true;

	// we should be started here (by test's init())
	QVERIFY(CPowerWatcher::isStarted());
	qDebug("Waiting onStart() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnStarted == 1, 1000);

	// check for spurious signals
	QVERIFY(m_countOnStopped == 0);
	QVERIFY(m_countOnSleep == 0);
	QVERIFY(m_countOnHasWakeup == 0);
	QVERIFY(m_countOnWillWakeup == 0);
	QVERIFY(m_countOnCancelSleep == 0);

	// nested start (should be ignored)
	qDebug("Start (nested, already started) watcher");
	QVERIFY(CPowerWatcher::startWatcher());
	qDebug("Waiting for spurious onStart() signal");
	QVERIFY_WAIT_FAILURE(m_countOnStarted == 1, 1000);

	// check for spurious signals
	QVERIFY(m_countOnStopped == 0);
	QVERIFY(m_countOnSleep == 0);
	QVERIFY(m_countOnHasWakeup == 0);
	QVERIFY(m_countOnWillWakeup == 0);
	QVERIFY(m_countOnCancelSleep == 0);

	// stop monitoring
	qDebug("Stop watcher");
	CPowerWatcher::stopWatcher();
	QVERIFY(!CPowerWatcher::isStarted());
	qDebug("Waiting onStopped() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnStopped == 1, 1000);

	// check for spurious signals
	QVERIFY(m_countOnStarted == 1);
	QVERIFY(m_countOnSleep == 0);
	QVERIFY(m_countOnHasWakeup == 0);
	QVERIFY(m_countOnWillWakeup == 0);
	QVERIFY(m_countOnCancelSleep == 0);

	// nested stop
	qDebug("Stop (nested, already stopped) watcher");
	CPowerWatcher::stopWatcher();
	qDebug("Waiting for spurious onStopped() signal");
	QVERIFY_WAIT_FAILURE(m_countOnStopped == 1, 1000);

	// check for spurious signals
	QVERIFY(m_countOnStarted == 1);
	QVERIFY(m_countOnSleep == 0);
	QVERIFY(m_countOnHasWakeup == 0);
	QVERIFY(m_countOnWillWakeup == 0);
	QVERIFY(m_countOnCancelSleep == 0);
}

void CPowerWatcherTest::testSleepWake()
{
	if (CPowerWatcher::isNotSupported())
		QSKIP("Monitoring not supported", SkipAll);

	// enable checks inside signal handlers
	m_testIsSleeping = true;

	// force system to sleep and schedule automatic wakeup...
	qDebug("Force to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup needed (timeout: 120 s)");
	if (!CPowerWatcher::forceSleep())
		QWARN("Manual action for sleep needed (timeout: 60 s)");

	// going to sleep...
	qDebug("Waiting onSleep() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnSleep == 1, 60000);
	QVERIFY(CPowerWatcher::isSleeping());

	qDebug("Sleeping");

	// wakeup...
	qDebug("Waiting onWillWakeup() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnWillWakeup == 1, 120000);
	qDebug("Waiting onHasWakeup() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnHasWakeup == 1, 120000);

	// check for spurious signals
	QVERIFY(m_countOnStarted == 1);
	QVERIFY(m_countOnStopped == 0);
	QVERIFY(m_countOnSleep == 1);
	QVERIFY(m_countOnHasWakeup == 1);
	QVERIFY(m_countOnWillWakeup == 1);
	QVERIFY(m_countOnCancelSleep == 0);
}

void CPowerWatcherTest::testNoSleep()
{
	if (CPowerWatcher::isNotSupported())
		QSKIP("Monitoring not supported", SkipAll);

	// disable system sleep
	qDebug("Disable sleep");
	CPowerWatcher::lockNoSleep();

	// try force system to sleep (should not sleep)....
	// force system to sleep and schedule automatic wakeup...
	qDebug("Try to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup may be needed");
	CPowerWatcher::forceSleep();
	qDebug("Waiting for spurious sleep (onSleep() signal, 60 s)");
	QVERIFY_WAIT_FAILURE(m_countOnSleep == 0, 60000);

	// check for spurious signals
	QVERIFY(m_countOnStarted == 1);
	QVERIFY(m_countOnStopped == 0);
	QVERIFY(m_countOnSleep == 0);
	QVERIFY(m_countOnHasWakeup == 0);
	QVERIFY(m_countOnWillWakeup == 0);

	// XXX: this signal not emmiteed on some platforms
	if (m_countOnCancelSleep == 0)
		QWARN("onCancelSleep() signal not supported");
	else
		QVERIFY(m_countOnCancelSleep == 1);

	// enable system sleep
	qDebug("Enable sleep");
	CPowerWatcher::unlockNoSleep();

	// check that sleep enabled (go to sleep and wakeup)
	qDebug("Force to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup needed (timeout: 120 s)");
	if (!CPowerWatcher::forceSleep())
		QWARN("Manual action for sleep needed (timeout: 60 s)");
	qDebug("Waiting onSleep() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnSleep == 1, 60000);
	qDebug("Waiting onHasWakeup() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnHasWakeup == 1, 120000);

	// disable system sleep by "locker" helper
	{
		qDebug("Disable sleep (by \"locker\")");
		CPowerWatcher::CNoSleepLocker locker;

		// try force system to sleep (should not sleep)....
		qDebug("Try to sleep");
		if (!CPowerWatcher::scheduleWakeup(90))
			QWARN("Manual action for wakeup may be needed");
		CPowerWatcher::forceSleep();
		qDebug("Waiting for spurious sleep (onSleep() signal, 60 s)");
		QVERIFY_WAIT_FAILURE(m_countOnSleep == 1, 60000);
	}
	qDebug("Enable sleep (by \"locker\")");

	// check that sleep enabled (go to sleep and wakeup)
	qDebug("Force to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup needed (timeout: 120 s)");
	if (!CPowerWatcher::forceSleep())
		QWARN("Manual action for sleep needed (timeout: 60 s)");
	qDebug("Waiting onSleep() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnSleep == 2, 60000);
	qDebug("Waiting onHasWakeup() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnHasWakeup == 2, 120000);

	// nested sleep disable
	qDebug("Disable sleep (nested, 2-levels)");
	CPowerWatcher::lockNoSleep();
	CPowerWatcher::lockNoSleep();

	// try force system to sleep (should not sleep)....
	qDebug("Try to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup may be needed");
	CPowerWatcher::forceSleep();
	qDebug("Waiting for spurious sleep (onSleep() signal, 60 s)");
	QVERIFY_WAIT_FAILURE(m_countOnSleep == 2, 60000);

	// level-up
	qDebug("Semi-enable sleep (nesting level-up)");
	CPowerWatcher::unlockNoSleep();

	// try force system to sleep (should not sleep)....
	qDebug("Try to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup may be needed");
	CPowerWatcher::forceSleep();
	qDebug("Waiting for spurious sleep (onSleep() signal, 60 s)");
	QVERIFY_WAIT_FAILURE(m_countOnSleep == 2, 60000);

	// enable sleep
	qDebug("Enable sleep");
	CPowerWatcher::unlockNoSleep();

	// check that sleep enabled (go to sleep and wakeup)
	qDebug("Force to sleep");
	if (!CPowerWatcher::scheduleWakeup(90))
		QWARN("Manual action for wakeup needed (timeout: 120 s)");
	if (!CPowerWatcher::forceSleep())
		QWARN("Manual action for sleep needed (timeout: 60 s)");
	qDebug("Waiting onSleep() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnSleep == 3, 60000);
	qDebug("Waiting onHasWakeup() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnHasWakeup == 3, 120000);

	// check for spurious signals
	QVERIFY(m_countOnStarted == 1);
	QVERIFY(m_countOnStopped == 0);
	QVERIFY(m_countOnSleep == 3);
	QVERIFY(m_countOnHasWakeup == 3);
	QVERIFY(m_countOnWillWakeup == 3);

	// XXX: this signal not emmiteed on some platforms
	if (m_countOnCancelSleep == 0)
		QWARN("onCancelSleep() signal not supported");
	else
		QVERIFY(m_countOnCancelSleep == 4);
}

void CPowerWatcherTest::testSignalsConnect_data()
{
	QTest::addColumn<int>("useThis");
	QTest::addColumn<int>("useOnSleep");
	QTest::addColumn<int>("useOnHasWakeup");
	QTest::addColumn<int>("useOnWillWakeup");
	QTest::addColumn<int>("useOnCancelSleep");
	QTest::addColumn<int>("useOnStarted");
	QTest::addColumn<int>("useOnStopped");

	QTest::addColumn<bool>("rcDisconnect");

	QTest::newRow("null")				<<0<<0<<0<<0<<0<<0<<0 << false;
	QTest::newRow("no slots")			<<1<<0<<0<<0<<0<<0<<0 << false;
	QTest::newRow("all slots")			<<1<<1<<1<<1<<1<<1<<1 << true;
	QTest::newRow("no this")			<<0<<1<<1<<1<<1<<1<<1 << false;
	QTest::newRow("no onSleep")			<<1<<0<<1<<1<<1<<1<<1 << true;
	QTest::newRow("no onHasWakeup")		<<1<<1<<0<<1<<1<<1<<1 << true;
	QTest::newRow("no onWillWakeup")	<<1<<1<<1<<0<<1<<1<<1 << true;
	QTest::newRow("no onCancelSleep")	<<1<<1<<1<<1<<0<<1<<1 << true;
	QTest::newRow("no onStarted")		<<1<<1<<1<<1<<1<<0<<1 << true;
	QTest::newRow("no onStopped")		<<1<<1<<1<<1<<1<<1<<0 << true;
}

void CPowerWatcherTest::testSignalsConnect()
{
	QFETCH(int, useThis);
	QFETCH(int, useOnSleep);
	QFETCH(int, useOnHasWakeup);
	QFETCH(int, useOnWillWakeup);
	QFETCH(int, useOnCancelSleep);
	QFETCH(int, useOnStarted);
	QFETCH(int, useOnStopped);
	QFETCH(bool, rcDisconnect);

	// cleanup state, drop all connections and stop watcher
	CPowerWatcher::disconnectWatcher(this);
	CPowerWatcher::stopWatcher();

	// check connect-before-start
	if (useThis) {
		qDebug("Connect-before-start");
		QVERIFY(CPowerWatcher::connectWatcher(this,
					useOnSleep			? SLOT(onSleep())			: NULL,
					useOnHasWakeup		? SLOT(onHasWakeup())		: NULL,
					useOnWillWakeup		? SLOT(onWillWakeup())		: NULL,
					useOnCancelSleep	? SLOT(onCancelSleep())		: NULL,
					useOnStarted		? SLOT(onStarted())			: NULL,
					useOnStopped		? SLOT(onStopped())			: NULL));
	}

	// check connect-on-start
	qDebug("Connect-on-start");
	QVERIFY(CPowerWatcher::startWatcher(
					useThis				? this						: NULL,
					useOnSleep			? SLOT(onSleep())			: NULL,
					useOnHasWakeup		? SLOT(onHasWakeup())		: NULL,
					useOnWillWakeup		? SLOT(onWillWakeup())		: NULL,
					useOnCancelSleep	? SLOT(onCancelSleep())		: NULL,
					useOnStarted		? SLOT(onStarted())			: NULL,
					useOnStopped		? SLOT(onStopped())			: NULL));

	// check connect-after-start
	if (useThis) {
		qDebug("Connect-after-start");
		QVERIFY(CPowerWatcher::connectWatcher(this,
					useOnSleep			? SLOT(onSleep())			: NULL,
					useOnHasWakeup		? SLOT(onHasWakeup())		: NULL,
					useOnWillWakeup		? SLOT(onWillWakeup())		: NULL,
					useOnCancelSleep	? SLOT(onCancelSleep())		: NULL,
					useOnStarted		? SLOT(onStarted())			: NULL,
					useOnStopped		? SLOT(onStopped())			: NULL));
	}

	// check disconnect
	QVERIFY(rcDisconnect == CPowerWatcher::disconnectWatcher(this));
}

void CPowerWatcherTest::testDarkWake()
{
    int workingTime = 1*60; // 1 minutes of working time
    int wakePeriod = 4*60; // 4 minutes of wake period;

    if (CPowerWatcher::isNotSupported())
		QSKIP("Monitoring not supported", SkipAll);

	if (!CPowerWatcher::setDarkWakeEnable(true))
		QSKIP("Dark wake not supported",  SkipSingle);

	if (!CPowerWatcher::isDarkWakeEnabled())
		QSKIP("Dark wake is turning off for current supply mode",  SkipSingle);

    // check that settings are applyed
    QVERIFY(CPowerWatcher::setDarkWakeParameter(wakePeriod, workingTime));

	// enable checks inside signal handlers
	m_testIsSleeping = true;

	// force system to sleep and schedule automatic full wakeup...
	qDebug("Force to sleep");
	if (!CPowerWatcher::scheduleWakeup((wakePeriod*3)/2))
		QWARN("Manual action for wakeup needed");

	if (!CPowerWatcher::forceSleep())
		QWARN("Manual action for sleep needed (timeout: 60 s)");

	// going to sleep...
	qDebug("Waiting onSleep() signal");
	QVERIFY_WAIT_SUCCESS(m_countOnSleep == 1, 60000);
	QVERIFY(CPowerWatcher::isSleeping());

	qDebug("Sleeping");

	// wait for dark wake
    QVERIFY_WAIT_SUCCESS(CPowerWatcher::isDarkWake(), 2*60*1000);
    // if dark wake occure, skip >workingTime, than wait for full wake up
    if (CPowerWatcher::isDarkWake())
    {
        qDebug("Dark wake");
        // emulate dark work
        QTest::qWait((workingTime*4/3)*1000);
    }

    // wait for full wakeup
    QVERIFY_WAIT_SUCCESS(m_countOnHasWakeup == 2, 60000);
    QVERIFY(!CPowerWatcher::isDarkWake());
    if (!CPowerWatcher::isDarkWake())
        qDebug("Full wake up");

    qDebug("now disable dark wake and check, that is really disabled");
    QVERIFY(CPowerWatcher::setDarkWakeEnable(false));

 	qDebug("Force to sleep");
	if (!CPowerWatcher::scheduleWakeup(wakePeriod*2))
		QWARN("Manual action for wakeup needed");

	if (!CPowerWatcher::forceSleep())
		QWARN("Manual action for sleep needed (timeout: 60 s)");

    QVERIFY_WAIT_SUCCESS(m_countOnHasWakeup == 3, 60000);
    QVERIFY(!CPowerWatcher::isDarkWake());

	// check for spurious signals
	QVERIFY(m_countOnStarted == 1);
	QVERIFY(m_countOnStopped == 0);
	QVERIFY(m_countOnSleep == 3);
	QVERIFY(m_countOnHasWakeup == 3);
	QVERIFY(m_countOnWillWakeup == 3);
	QVERIFY(m_countOnCancelSleep == 0);
}
