///////////////////////////////////////////////////////////////////////////////
///
/// @file condwait.h
///
/// Lightweight cross platform condition variable helper
///
/// @author olegv@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _WIN_
#include <pthread.h>
#else
#include <QMutex>
#include <QWaitCondition>
#endif

/*
Though the Qt classes implement mutex and condition on the top of pthread they are not
pure pthread mutex and condition. The QMutex have some spinning 'optimization' so before calling
pthread_cond_wait the Qt code unlock QMutex and lock another hidden mutex. The situation is even worse
when wake is called since Qt code is always locking that hidden mutex before signalling condition.
This unnecessary complication leads to noticable performance degradation. The purpose of this class
is to provide more simple and fast condition variable implementation.
*/

class CPrlWaitCondition {
public:
#ifndef _WIN_
		CPrlWaitCondition() {
			pthread_mutex_init(&m_mutex, 0);
			pthread_cond_init(&m_cond, 0);
		}
		~CPrlWaitCondition() {
			pthread_cond_destroy(&m_cond);
			pthread_mutex_destroy(&m_mutex);
		}
	void lock()		{ pthread_mutex_lock(&m_mutex); }
	void unlock()	{ pthread_mutex_unlock(&m_mutex); }
	void wait()		{ pthread_cond_wait(&m_cond, &m_mutex); }
	void timedwait(unsigned long ms_tout);
	void wakeOne()	{ pthread_cond_signal(&m_cond); }
	void wakeAll()	{ pthread_cond_broadcast(&m_cond); }
#else
	void lock()		{ m_mutex.lock(); }
	void unlock()	{ m_mutex.unlock(); }
	void wait()		{ m_cond.wait(&m_mutex); }
	void timedwait(unsigned long ms_tout) { m_cond.wait(&m_mutex, ms_tout); }
	void wakeOne()	{ m_cond.wakeOne(); }
	void wakeAll()	{ m_cond.wakeAll(); }
#endif

	class locker {
	public:
		locker(CPrlWaitCondition& wc, bool locked = true)
			: m_wc(wc), m_locked(locked) {
				if (locked) m_wc.lock();
			}
		~locker() { unlock(); }
		void relock() { if (!m_locked) { m_wc.lock(); m_locked = true; } }
		void unlock() { if (m_locked) { m_wc.unlock(); m_locked = false; } }
	private:
		CPrlWaitCondition& m_wc;
		bool m_locked;
	};

private:
#ifndef _WIN_
	pthread_mutex_t	m_mutex;
	pthread_cond_t	m_cond;
#else
	QMutex			m_mutex;
	QWaitCondition	m_cond;
#endif
};
