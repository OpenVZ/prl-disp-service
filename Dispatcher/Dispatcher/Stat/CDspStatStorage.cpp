///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatStorage.cpp
///
/// High-level interface to performance counters storage.
///
/// @author alkurbatov
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include <QPair>
#include <boost/foreach.hpp>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CDspStatStorage.h"

namespace Stat
{
///////////////////////////////////////////////////////////////////////////////
// struct Storage

Storage::Storage(const QString& id_):
	m_vessel(new CounterStorageT(QSTR2UTF8(id_)))
{
}

void Storage::add(const QString& type_, const QString& name_)
{
	QWriteLocker l(&m_rwLock);

	if (m_counters.contains(name_))
		return;

	m_counters.insert(name_, m_vessel->add_counter(QSTR2UTF8(type_ + name_)));
}

quint64 Storage::read(const QString& name_)
{
	QReadLocker l(&m_rwLock);

	if (!m_counters.contains(name_))
		return 0;

	return PERF_COUNT_GET(m_counters.value(name_));
}

void Storage::write(const QString& name_, quint64 value_)
{
	QWriteLocker l(&m_rwLock);

	if (!m_counters.contains(name_))
		return;

	PERF_COUNT_SET(m_counters.value(name_), value_);
}

} // namespace Stat
