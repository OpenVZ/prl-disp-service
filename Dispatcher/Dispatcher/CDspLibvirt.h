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

private:
       	static PRL_RESULT fabricate(PRL_RESULT result_);
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

typedef QPair<QString, quint64> Counter_type;
typedef QList<Counter_type> CounterList_type;

} // namespace Stat

namespace Performance
{

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(const virDomainStatsRecordPtr record_);

	Result getUuid(QString& dst_) const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getCpu() const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getVCpuList() const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getDisk(const CVmHardDisk& disk_) const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getMemory() const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getInterface(const CVmGenericNetworkAdapter& iface_) const;

private:
	bool getValue(const QString& name_, quint64& dst_) const;

	const virDomainStatsRecord* m_record;
	QHash<QString, unsigned> m_ifaces;
	QHash<QString, unsigned> m_disks;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List: QList<Unit>
{
	explicit List(virDomainStatsRecordPtr* records_): m_data(records_, &virDomainStatsRecordListFree)
	{
	}

private:
	QSharedPointer<virDomainStatsRecordPtr> m_data;
};

} // namespace Performance

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

namespace Block
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(const QSharedPointer<virDomain>& domain_, const QString& disk_, const QString& path_):
		m_domain(domain_), m_disk(disk_), m_path(path_)
	{
	}

	Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> getProgress() const;
	Result abort() const;
	Result finish() const;

private:
	QSharedPointer<virDomain> m_domain;
	QString m_disk;
	QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
// struct Future

struct Future: QObject
{
	explicit Future(const QList<Unit>& units_): m_units(units_)
	{
		m_timer = startTimer(0);
	}

	void wait();
	void cancel();

signals:
	void finished();

private slots:
	void exit();

private:
	void timerEvent(QTimerEvent*);

	Q_OBJECT

	QList<Unit> m_units;
	QList<Unit> m_completed;
	int m_timer;
	QEventLoop m_loop;
};

} // namespace Block

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
	Result getConfig(CVmConfiguration& dst_) const;
	Result revert();
	Result undefine();
	Result undefineRecursive();

private:
	QSharedPointer<virConnect> getLink() const;

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
	Result createExternal(const QString& uuid_, const QList<CVmHardDisk*>& disks_);
	Prl::Expected<Block::Unit, ::Error::Simple> merge(const CVmHardDisk& disk_);

private:
	Prl::Expected<Unit, ::Error::Simple>
		define(const QString& uuid_, const QString& description_, quint32 flags_);

	static Result translate
		(const Prl::Expected<Unit, ::Error::Simple>& result_, Unit* dst_);

	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Merge

struct Merge
{
	Merge(const List& agent_, const QList<CVmHardDisk>& disks): m_agent(agent_), m_disks(disks), m_units()
	{
	}

	QSharedPointer<Block::Future> start();
	void stop();

private:
	List m_agent;
	QList<CVmHardDisk> m_disks;
	QList<Block::Unit> m_units;
};

} // namespace Snapshot

namespace Exec
{

struct AuxChannel;

} // namespace Exec

namespace Migration
{
struct Basic;
struct Compression;
struct Bandwidth;

namespace Qemu
{
struct Disk;
struct State;
} // namespace Qemu

///////////////////////////////////////////////////////////////////////////////
// struct Agent

struct Agent
{
	Agent(QSharedPointer<virDomain> domain_, QSharedPointer<virConnect> link_,
		const QString& uri_):
		m_domain(domain_), m_link(link_), m_uri(uri_)
	{
	}

	Result cancel();
	Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> getProgress();

protected:
	Result setDowntime(quint32 value_);
	Result migrate(const CVmConfiguration& config_, unsigned int flags_,
		Parameters::Builder& parameters_);

private:
	QSharedPointer<virDomain> m_domain;
	QSharedPointer<virConnect> m_link;
	QString m_uri;
};

///////////////////////////////////////////////////////////////////////////////
// struct Online

struct Online: Agent
{
	explicit Online(const Agent& agent_);

	void setQemuState(qint32 port_);
	void setQemuDisk(const QList<CVmHardDisk* >& list_, qint32 port_);
	void setQemuDisk(const QList<CVmHardDisk* >& list_);
	void setUncompressed()
	{
		m_compression.clear();
	}
	void setBandwidth(quint64 value_);

	Result operator()(const CVmConfiguration& config_);

private:
	QSharedPointer<Qemu::Disk> m_qemuDisk;
	QSharedPointer<Qemu::State> m_qemuState;
	QSharedPointer<Compression> m_compression;
	QSharedPointer<Bandwidth> m_bandwidth;
};

///////////////////////////////////////////////////////////////////////////////
// struct Offline

struct Offline: Agent
{
	explicit Offline(const Agent& agent_): Agent(agent_)
	{
	}

	Result operator()(const CVmConfiguration& config_);
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
	Result startPaused();
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
	Result setMemoryStatsPeriod(qint64 seconds_);

	Migration::Agent migrate(const QString& uri_);

	List up() const;
	Guest getGuest() const;
	Snapshot::List getSnapshot() const
	{
		return Snapshot::List(m_domain);
	}
	Runtime getRuntime() const;
	Exec::AuxChannel* getChannel(const QString& path_) const;

private:
	QSharedPointer<virConnect> getLink() const;
	Result start_(unsigned int flags_);

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
	Performance::List getPerformance();

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

