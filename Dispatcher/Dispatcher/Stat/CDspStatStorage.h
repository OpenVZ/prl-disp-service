///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatStorage.h
///
/// High-level interface to performance counters storage.
///
/// @author alkurbatov
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPSTATSTORAGE_H__
#define __CDSPSTATSTORAGE_H__

#include <QPair>
#include <QHash>
#include <QString>
#include <QReadWriteLock>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

namespace Stat
{
typedef QPair<quint64, quint64> timedValue_type;
///////////////////////////////////////////////////////////////////////////////
// struct Storage

struct Storage: boost::noncopyable
{
	explicit Storage(const QString& id_);

	void addAbsolute(const QString& name_);

	void addIncremental(const QString& name_);

	timedValue_type read(const QString& name_);

	void write(const QString& name_, quint64 value_, quint64 time_);

private:
	typedef QHash<QString, timedValue_type> hash_type;

	QReadWriteLock m_rwLock;
	hash_type m_absolute;
	hash_type m_incremental;
};

namespace Name
{

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

struct Cpu
{
	static QString getName();
};

///////////////////////////////////////////////////////////////////////////////
// struct VCpu

struct VCpu
{
	static QString getName(unsigned index_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Memory

struct Memory
{
	static QString getUsed();

	static QString getCached();

	static QString getTotal();

	static QString getBalloonActual();

	static QString getSwapIn();

	static QString getSwapOut();

	static QString getMinorFault();

	static QString getMajorFault();
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface

struct Interface
{
	static QString getBytesIn(const CVmGenericNetworkAdapter& iface_);

	static QString getPacketsIn(const CVmGenericNetworkAdapter& iface_);

	static QString getBytesOut(const CVmGenericNetworkAdapter& iface_);

	static QString getPacketsOut(const CVmGenericNetworkAdapter& iface_);

private:
	static QString generate(const CVmGenericNetworkAdapter& iface_, const QString& stat_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Hdd

struct Hdd
{
	static QString getReadRequests(const CVmHardDisk& disk_);

	static QString getWriteRequests(const CVmHardDisk& disk_);

	static QString getReadTotal(const CVmHardDisk& disk_);

	static QString getWriteTotal(const CVmHardDisk& disk_);

private:
	static QString generate(const CVmHardDisk& disk_, const QString& stat_);

	static QString convert(PRL_MASS_STORAGE_INTERFACE_TYPE diskType_);
};

} // namespace Name
} // namespace Stat

#endif // __CDSPSTATSTORAGE_H__
