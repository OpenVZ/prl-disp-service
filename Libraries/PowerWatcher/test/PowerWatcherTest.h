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
/// @file PowerWatcherTest.h
///
/// @brief
///   System power state handling and control, test application
///
/////////////////////////////////////////////////////////////////////////////
#if !defined(POWER_WATCHER_TEST__H)
#define POWER_WATCHER_TEST__H

/////////////////////////////////////////////////////////////////////////////

#include "Libraries/PowerWatcher/PowerWatcher.h"

#include <QtTest/QtTest>

/////////////////////////////////////////////////////////////////////////////

class CPowerWatcherTest : public QObject
{
	Q_OBJECT

private:
	int m_countOnStarted;
	int m_countOnStopped;
	int m_countOnSleep;
	int m_countOnHasWakeup;
	int m_countOnWillWakeup;
	int m_countOnCancelSleep;

	bool m_testIsStarted;
	bool m_testIsSleeping;

public:
	CPowerWatcherTest();

public slots:
	void onStarted();
	void onStopped();

	void onSleep();
	void onHasWakeup();
	void onWillWakeup();

	void onCancelSleep();

private slots:
	// called before each test
	void init();
	// called after each test
	void cleanup();

	void testSignalsConnect();
	void testSignalsConnect_data();

	void testStartStop();
	void testSleepWake();
	void testNoSleep();
    void testDarkWake();
};

/////////////////////////////////////////////////////////////////////////////

// Extension for QtTest's QVERIFY: test expression in loop until
// timeout expired or expression _successed_.
//   * test expression may be evaluated more than once
//   * check period is 100 ms
#define QVERIFY_WAIT_SUCCESS(expr, timeout)				\
	for (int t = 0; !(expr); t += 100)	{					\
		if (t >= timeout) {									\
			QVERIFY2(expr, "TIMEOUT");						\
			break;											\
		};													\
		QTest::qWait((timeout < 100) ? timeout : 100);		\
	}

// Extension for QtTest's QVERIFY: test expression in loop until
// timeout expired or expression _failed_.
//   * test expression evaluated more than once (continuously)
//   * check period is 20 ms
#define QVERIFY_WAIT_FAILURE(expr, timeout)			\
	for (int t = 0; t < timeout; t += 20)	 {			\
		QVERIFY(expr);									\
		QTest::qWait((timeout < 20) ? timeout : 20);	\
	}

/////////////////////////////////////////////////////////////////////////////

#endif // POWER_WATCHER_TEST__H
//EOF
