///////////////////////////////////////////////////////////////////////////////
///
/// @file Cancellation.cpp
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

#include "Cancellation.h"

namespace Cancellation
{
///////////////////////////////////////////////////////////////////////////////
// class Sink

Sink::Sink(QMutex& mutex_, QWaitCondition& condition_):
	m_mutex(&mutex_), m_condition(&condition_)
{
}

void Sink::do_()
{
	QMutexLocker g(m_mutex);
	m_condition->wakeAll();
}

///////////////////////////////////////////////////////////////////////////////
// class Token

void Token::signal()
{
	if (0 < m_is.fetchAndStoreAcquire(1))
		return;

	emit cancel();
}

} // namespace Cancellation

