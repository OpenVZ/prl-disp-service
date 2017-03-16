///////////////////////////////////////////////////////////////////////////////
///
/// @file Cancellation.h
///
/// Copyright (c) 2016-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef CANCELLATION_H
#define CANCELLATION_H

#include <QMutex>
#include <QObject>
#include <QAtomicInt>
#include <QWaitCondition>

namespace Cancellation
{
///////////////////////////////////////////////////////////////////////////////
// class Sink

class Sink: public QObject
{
	Q_OBJECT

public:
	Sink(QMutex& , QWaitCondition& );

public slots:
	void do_();

private:
	QMutex* m_mutex;
	QWaitCondition* m_condition;
};

///////////////////////////////////////////////////////////////////////////////
// class Token

class Token: public QObject
{
	Q_OBJECT

public:
	bool isCancelled() const
	{
		return 0 < m_is.operator int();
	}
	void signal();

signals:
	void cancel();

private:
	QAtomicInt m_is;
};

} // namespace Cancellation

#endif // CANCELLATION_H

