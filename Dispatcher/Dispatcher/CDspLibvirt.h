///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirt.h
///
/// Public interfaces of the libvirt interaction.
///
/// @author shrike
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

#ifndef __CDSPLIBVIRT_H__
#define __CDSPLIBVIRT_H__

#include <QThread>
#include <utility>
#include <QWeakPointer>
#include <prlsdk/PrlTypes.h>
#include <libvirt/libvirt.h>
#include <boost/optional.hpp>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>
#include <prlxmlmodel/VtInfo/VtInfo.h>
#include <prlcommon/Logging/Logging.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlxmlmodel/NetworkConfig/CVirtualNetwork.h>
#include <prlxmlmodel/HostHardwareInfo/CHwNetAdapter.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

class CSavedStateTree;

namespace Registry
{
struct Actual;
} // namespace Registry

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Failure

struct Failure: ::Error::Simple
{
	Failure(PRL_RESULT result_);
};

namespace Agent
{
///////////////////////////////////////////////////////////////////////////////
// struct Failure
//
struct Failure: ::Error::Simple
{
	Failure(PRL_RESULT result_);
	bool isTransient() const;
	int virErrorCode() const { return m_virErrorCode; }
private:
	int m_virErrorCode;
};

} // namespace Agent

typedef Prl::Expected<void, ::Error::Simple> Result;

namespace Instrument
{
namespace Agent
{
namespace Parameters
{
struct Builder;

} // namespace Parameters

namespace Vm
{
namespace Stat
{
///////////////////////////////////////////////////////////////////////////////
// struct Memory

struct Memory
{
	Memory(): baloonActual(0), rss(0), available(0), unused(0),
		swapIn(0), swapOut(0), minorFault(0), majorFault(0)
	{
	}

	quint64 baloonActual;
	quint64 rss;

	quint64 available;
	quint64 unused;

	quint64 swapIn;
	quint64 swapOut;

	quint64 minorFault;
	quint64 majorFault;
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface

struct Interface
{
	explicit Interface(const CVmGenericNetworkAdapter& adapter_):
		name(adapter_.getHostInterfaceName()), index(adapter_.getIndex()),
		bytesIn(0), packetsIn(0), bytesOut(0), packetsOut(0)
	{
	}

	QString name;
	unsigned index;
	quint64 bytesIn;
	quint64 packetsIn;
	quint64 bytesOut;
	quint64 packetsOut;
};

typedef QList<Interface> Network;

typedef QPair<unsigned, quint64> VCpu_type;
typedef QList<VCpu_type> VCpuList_type;

} // namespace Stat

///////////////////////////////////////////////////////////////////////////////
// struct Performance

struct Performance
{
	explicit Performance(const QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	Result setMemoryStatsPeriod(qint64 seconds_);

	Result getCpu(quint64& nanoseconds_) const;
	Prl::Expected<Stat::VCpuList_type, Error::Simple>
		getVCpuList() const;
	Result getDisk() const;
	Result getMemory(Stat::Memory& dst_) const;
	Result getInterface(Stat::Interface& dst_) const;

private:
	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

struct Hotplug
{
	explicit Hotplug(const QSharedPointer<virDomain>& domain_):
		m_domain(domain_)
	{
	}

	Result attach(const QString& device_);
	Result detach(const QString& device_);
	Result update(const QString& device_);

private:
	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Runtime

struct Runtime
{
	explicit Runtime(const QSharedPointer<virDomain>& domain_)
		: m_domain(domain_)
	{
	}

	Result setIoLimit(const CVmHardDisk& disk_, quint32 limit_);
	Result setIopsLimit(const CVmHardDisk& disk_, quint32 limit_);
	Result setIoPriority(quint32 ioprio_);
	Result setPerCpuLimit(quint32 limit_, quint32 period_);
	Result setGlobalCpuLimit(quint32 limit_, quint32 period_);
	Result setCpuUnits(quint32 units_);
	Result setCpuCount(quint32 count_);

	template<class T>
	Result plug(const T& device_);
	template<class T>
	Result unplug(const T& device_);
	template<class T>
	Result update(const T& device_);

private:
	Result setCpuLimit(quint32 globalLimit_, quint32 limit_, quint32 period_);
	Result setBlockIoTune(const CVmHardDisk& disk_, const char* param_, quint32 limit_);

	QSharedPointer<virDomain> m_domain;
};

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(virDomainSnapshotPtr snapshot_);

	Unit getParent() const;
	Result getUuid(QString& dst_) const;
	Result getState(CSavedStateTree& dst_) const;
	Result revert();
	Result undefine();
	Result undefineRecursive();

private:
	QSharedPointer<virDomainSnapshot> m_snapshot;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(const QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	Unit at(const QString& uuid_) const;
	Result all(QList<Unit>& dst_) const;
	Result define(const QString& uuid_, const QString& description_,
		Unit* dst_ = NULL);
	Result defineConsistent(const QString& uuid_, const QString& description_,
		Unit* dst_ = NULL);

private:
	Prl::Expected<Unit, ::Error::Simple>
		define(const QString& uuid_, const QString& description_, quint32 flags_);

	static Result translate
		(const Prl::Expected<Unit, ::Error::Simple>& result_, Unit* dst_);

	QSharedPointer<virDomain> m_domain;
};

} // namespace Snapshot

namespace Exec
{

struct AuxChannel;

} // namespace Exec

namespace Migration
{
///////////////////////////////////////////////////////////////////////////////
// struct Task

struct Task
{
	Task(QSharedPointer<virDomain> domain_,
		QSharedPointer<virConnect> link_,
		const QString& uri_)
		: m_domain(domain_), m_link(link_), m_uri(uri_)
	{
	}

	Result doOnline(const CVmConfiguration &config_, const QList<CVmHardDisk*>& disks_);
	Result doOffline(const CVmConfiguration &config_);
	Result cancel();
	Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple>
		getProgress();

private:
	Result doInternal(Parameters::Builder& parameters_, unsigned int flags_);

	QSharedPointer<virDomain> m_domain;
	QSharedPointer<virConnect> m_link;
	QString m_uri;
};

} // namespace Migration

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct List;
struct Guest;
struct Unit
{
	explicit Unit(virDomainPtr domain_ = NULL);

	Result getUuid(QString& dst_) const;
	Result kill();
	Result shutdown();
	Result start();
	Result reboot();
	Result reset();
	Result pause();
	Result unpause();
	Result rename(const QString& to_);
	Result resume(const QString& sav_);
	Result suspend(const QString& sav_);
	Result undefine();
	Result getState(VIRTUAL_MACHINE_STATE& dst_) const;
	Result getConfig(CVmConfiguration& dst_, bool runtime_ = false) const;
	Result getConfig(QString& dst_, bool runtime_ = false) const;
	Result setConfig(const CVmConfiguration& value_);
	Result completeConfig(CVmConfiguration& config_);

	Migration::Task migrate(const QString& uri_);

	List up() const;
	Performance getPerformance() const
	{
		return Performance(m_domain);
	}
	Guest getGuest() const;
	Snapshot::List getSnapshot() const
	{
		return Snapshot::List(m_domain);
	}
	Runtime getRuntime() const;
	Exec::AuxChannel* getChannel(const QString& path_) const;

private:
	QSharedPointer<virConnect> getLink() const;

	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Unit at(const QString& uuid_) const;
	Result all(QList<Unit>& dst_);
	Result define(const CVmConfiguration& config_, Unit* dst_ = NULL);

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(virNetworkPtr network_ = NULL);

	Result stop();
	Result start();
	Result undefine();
	Result getConfig(CVirtualNetwork& dst_) const;

private:
	QSharedPointer<virNetwork> m_network;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Unit at(const QString& uuid_) const;
	Result all(QList<Unit>& dst_) const;
	Result find(const QString& name_, Unit* dst_ = NULL) const;
	Result define(const CVirtualNetwork& config_, Unit* dst_ = NULL);

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Network

namespace Interface
{
///////////////////////////////////////////////////////////////////////////////
// struct Bridge

struct Bridge
{
	Bridge()
	{
	}
	Bridge(virInterfacePtr interface_, const CHwNetAdapter& master_);

	QString getName() const;
	const CHwNetAdapter& getMaster() const
	{
		return m_master;
	}
	Result stop();
	Result start();
	Result undefine();

private:
	CHwNetAdapter m_master;
	QSharedPointer<virInterface> m_interface;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Result all(QList<Bridge>& dst_) const;
	Result find(const QString& mac_, unsigned short vlan_, CHwNetAdapter& dst_) const;
	Result find(const QString& name_, CHwNetAdapter& dst_) const;
	Result findBridge(const QString& name_, CHwNetAdapter& dst_) const;
	Result find(const QString& name_, Bridge& dst_) const;
	Result find(const CHwNetAdapter& eth_, Bridge& dst_) const;
	Result define(const CHwNetAdapter& eth_, Bridge& dst_);

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Interface

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host
{
	explicit Host(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Prl::Expected<VtInfo, ::Error::Simple> getVt() const;

private:
	QSharedPointer<virConnect> m_link;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub
{
	Vm::List vms()
	{
		return Vm::List(m_link.toStrongRef());
	}
	Network::List networks()
	{
		return Network::List(m_link.toStrongRef());
	}
	Interface::List interfaces()
	{
		return Interface::List(m_link.toStrongRef());
	}

	void setLink(QSharedPointer<virConnect> value_);

	Host host()
	{
		return Host(m_link);
	}
	QSharedPointer<Vm::Exec::AuxChannel> addAsyncExec(const Vm::Unit& unit_);

private:
	QWeakPointer<virConnect> m_link;
	QMap<QString, QWeakPointer<Vm::Exec::AuxChannel> > m_execs;
};

} // namespace Agent
} // namespace Instrument

extern Instrument::Agent::Hub Kit;

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host: QThread
{
	explicit Host(Registry::Actual& registry_):
		QThread(), m_registry(registry_)
	{
	}

protected:
	void run();

private:
	Q_OBJECT

	Registry::Actual& m_registry;
};

} // namespace Libvirt

#endif // __CDSPLIBVIRT_H__

