///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatStorage.cpp
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

#include <QPair>
#include <boost/foreach.hpp>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CDspStatStorage.h"

namespace Stat
{
///////////////////////////////////////////////////////////////////////////////
// struct Storage

Storage::Storage(const QString& id_)
{
	Q_UNUSED(id_);
}

void Storage::addAbsolute(const QString& name_)
{
	QWriteLocker l(&m_rwLock);

	if (m_absolute.contains(name_) || m_incremental.contains(name_))
		return;

	m_absolute.insert(name_, timedValue_type());
}

void Storage::addIncremental(const QString& name_)
{
	QWriteLocker l(&m_rwLock);

	if (m_incremental.contains(name_) || m_absolute.contains(name_))
		return;

	m_incremental.insert(name_, timedValue_type());
}

timedValue_type Storage::read(const QString& name_)
{
	QReadLocker l(&m_rwLock);

	if (m_absolute.contains(name_))
		return m_absolute[name_];

	if (m_incremental.contains(name_))
		return m_incremental[name_];

	return timedValue_type();
}

void Storage::write(const QString& name_, quint64 value_, quint64 time_)
{
	QWriteLocker l(&m_rwLock);

	if (m_absolute.contains(name_))
		m_absolute[name_] = timedValue_type(value_, time_);
	else if (m_incremental.contains(name_))
		m_incremental[name_] = timedValue_type(value_, time_);
}

namespace Name
{

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

QString Cpu::getName()
{
	return "cpu_time";
}

///////////////////////////////////////////////////////////////////////////////
// struct VCpu

QString VCpu::getName(unsigned index_)
{
	return QString("guest.vcpu%1.time").arg(index_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory

QString Memory::getUsed()
{
	return "mem.guest_used";
}

QString Memory::getCached()
{
	return "mem.guest_cached";
}

QString Memory::getTotal()
{
	return "mem.guest_total";
}

QString Memory::getBalloonActual()
{
	return "kernel.ws.balloon_size";
}

QString Memory::getSwapIn()
{
	return "mem.guest_swap_in";
}

QString Memory::getSwapOut()
{
	return "mem.guest_swap_out";
}

QString Memory::getMinorFault()
{
	return "mem.guest_minor_fault";
}

QString Memory::getMajorFault()
{
	return "mem.guest_major_fault";
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface

QString Interface::getBytesIn(const CVmGenericNetworkAdapter& iface_)
{
	return generate(iface_, "bytes_in");
}

QString Interface::getPacketsIn(const CVmGenericNetworkAdapter& iface_)
{
	return generate(iface_, "pkts_in");
}

QString Interface::getBytesOut(const CVmGenericNetworkAdapter& iface_)
{
	return generate(iface_, "bytes_out");
}

QString Interface::getPacketsOut(const CVmGenericNetworkAdapter& iface_)
{
	return generate(iface_, "pkts_out");
}

QString Interface::generate(const CVmGenericNetworkAdapter& iface_, const QString& stat_)
{
	return QString("net.nic%1.%2").
		arg(iface_.getIndex()).
		arg(stat_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hdd

QString Hdd::getReadRequests(const CVmHardDisk& disk_)
{
	return generate(disk_, "read_requests");
}

QString Hdd::getWriteRequests(const CVmHardDisk& disk_)
{
	return generate(disk_, "write_requests");
}

QString Hdd::getReadTotal(const CVmHardDisk& disk_)
{
	return generate(disk_, "read_total");
}

QString Hdd::getWriteTotal(const CVmHardDisk& disk_)
{
	return generate(disk_, "write_total");
}

QString Hdd::getCapacity(const CVmHardDisk& disk_)
{
	return generate(disk_, "capacity");
}

QString Hdd::getAllocation(const CVmHardDisk& disk_)
{
	return generate(disk_, "allocation");
}

QString Hdd::generate(const CVmHardDisk& disk_, const QString& stat_)
{
	return QString("devices.%1%2.%3").
		arg(convert(disk_.getInterfaceType())).
		arg(disk_.getStackIndex()).
		arg(stat_);
}

QString Hdd::convert(PRL_MASS_STORAGE_INTERFACE_TYPE diskType_)
{
	switch (diskType_)
	{
	case PMS_IDE_DEVICE:
		return "ide";
	case PMS_SCSI_DEVICE:
		return "scsi";
	case PMS_SATA_DEVICE:
		return "sata";
	case PMS_VIRTIO_BLOCK_DEVICE:
		return "virtio";
	default:
		return "unknown";
	}
}

} // namespace Name
} // namespace Stat
