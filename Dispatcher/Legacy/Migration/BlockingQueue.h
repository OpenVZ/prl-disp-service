///////////////////////////////////////////////////////////////////////////////
///
/// @file BlockingQueue.h
///
/// Copyright (c) 2016 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <QMutexLocker>
#include "Cancellation.h"

///////////////////////////////////////////////////////////////////////////////
// class BlockingQueue

template<class T, quint8 W = 10>
struct BlockingQueue
{
	BlockingQueue(): m_head(), m_size()
	{
	}

	bool empty()
	{
		return 0 == size();
	}
	quint8 size()
	{
		QMutexLocker g(&m_mutex);
		return m_size;
	}
	bool take(T& dst_, Cancellation::Token& token_)
	{
		m_mutex.lock();
		while (0 == m_size)
		{
			if (token_.isCancelled())
			{
				m_mutex.unlock();
				return false;
			}
			Cancellation::Sink a(m_mutex, m_hasData);
			bool y = a.connect(&token_, SIGNAL(cancel()), SLOT(do_()), Qt::DirectConnection);
			Q_ASSERT(y);
			Q_UNUSED(y);
			if (!token_.isCancelled())
				m_hasData.wait(&m_mutex);

			token_.disconnect(&a);
		}
		bool w = W == m_size;
		takeOne(dst_);
		m_mutex.unlock();
		if (w)
			m_hasSpace.wakeOne();

		return true;
	}
	bool add(const T& value_, Cancellation::Token& token_)
	{
		if (token_.isCancelled())
			return false;

		m_mutex.lock();
		while (W == m_size)
		{
			if (token_.isCancelled())
			{
				m_mutex.unlock();
				return false;
			}
			Cancellation::Sink a(m_mutex, m_hasSpace);
			bool y = a.connect(&token_, SIGNAL(cancel()), SLOT(do_()), Qt::DirectConnection);
			Q_ASSERT(y);
			Q_UNUSED(y);
			if (!token_.isCancelled())
				m_hasSpace.wait(&m_mutex);

			token_.disconnect(&a);
		}
		bool w = 0 == m_size;
		m_queue[advance(m_size++)] = value_;
		m_mutex.unlock();
		if (w)
			m_hasData.wakeOne();

		return true;
	}

private:
	quint8 advance(quint8 distance_) const
	{
		return (m_head + distance_) % W;
	}
	void takeOne(T& dst_)
	{
		dst_ = m_queue[m_head];
		m_queue[m_head] = T();
		m_head = advance(1);
		m_size--;
	}

	T m_queue[W];
	quint8 m_head;
	volatile quint8 m_size;
	QMutex m_mutex;
	QWaitCondition m_hasData;
	QWaitCondition m_hasSpace;
};

#endif // BLOCKINGQUEUE_H

