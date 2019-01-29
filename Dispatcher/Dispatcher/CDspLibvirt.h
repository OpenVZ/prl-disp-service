///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirt.h
///
/// Public interfaces of the libvirt interaction.
///
/// @author shrike
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

#ifndef __CDSPLIBVIRT_H__
#define __CDSPLIBVIRT_H__

#include <QThread>
#include <utility>
#include <QWeakPointer>
#include <prlsdk/PrlTypes.h>
#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/future.hpp>
#include <boost/signals2/signal.hpp>
#include <prlxmlmodel/VtInfo/VtInfo.h>
#include <prlcommon/Logging/Logging.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <prlxmlmodel/NetworkConfig/CVirtualNetwork.h>
#include <prlxmlmodel/HostHardwareInfo/CHwNetAdapter.h>
#include <prlxmlmodel/BackupActivity/SnapshotComponent.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <prlxmlmodel/HostHardwareInfo/CHwGenericPciDevice.h>

struct _virDomain;
typedef struct _virDomain virDomain;
typedef virDomain *virDomainPtr;

struct _virConnect;
typedef struct _virConnect virConnect;
typedef virConnect *virConnectPtr;

struct _virDomainSnapshot;
typedef struct _virDomainSnapshot virDomainSnapshot;
typedef virDomainSnapshot *virDomainSnapshotPtr;

struct _virNetwork;
typedef struct _virNetwork virNetwork;
typedef virNetwork *virNetworkPtr;

struct _virInterface;
typedef struct _virInterface virInterface;
typedef virInterface *virInterfacePtr;

struct _virDomainStatsRecord;
typedef struct _virDomainStatsRecord virDomainStatsRecord;
typedef virDomainStatsRecord *virDomainStatsRecordPtr;

struct _virDomainBlockJobInfo;
typedef struct _virDomainBlockJobInfo virDomainBlockJobInfo;

struct _virDomainBlockSnapshotX;
typedef struct _virDomainBlockSnapshotX virDomainBlockSnapshotX;
typedef virDomainBlockSnapshotX* virDomainBlockSnapshotXPtr;

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
struct Failure: ::Libvirt::Failure
{
	Failure(PRL_RESULT result_);
	bool isTransient() const;

	int getMainCode() const
	{
		return m_mainCode;
	}
	int getExtraCode() const
	{
		return m_extraCode;
	}

private:
	int m_mainCode, m_extraCode;
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
	typedef QSharedPointer<virDomainStatsRecordPtr> pin_type;

	Unit(const virDomainStatsRecordPtr record_, const pin_type& pin_);

	Result getUuid(QString& dst_) const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getCpu() const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getVCpuList() const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getDisk(const CVmHardDisk& disk_) const;
	Stat::CounterList_type getMemory() const;
	Prl::Expected<Stat::CounterList_type, Error::Simple>
		getInterface(const CVmGenericNetworkAdapter& iface_) const;

private:
	bool getValue(const QString& name_, quint64& dst_) const;

	pin_type m_pin;
	const virDomainStatsRecord* m_record;
	QHash<QString, unsigned> m_ifaces;
	QHash<QString, unsigned> m_disks;
};

} // namespace Performance

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

struct Hotplug
{
	explicit Hotplug(const QSharedPointer<virDomain>& domain_);

	Hotplug& setApplyConfig(bool value_);
	Result attach(const QString& device_);
	Result detach(const QString& device_);
	Result update(const QString& device_);

private:
	quint32 m_flags;
	QSharedPointer<virDomain> m_domain;
};

namespace Limb
{
///////////////////////////////////////////////////////////////////////////////
// struct Abstract

struct Abstract
{
	typedef QSharedPointer<virDomain> domainReference_type;
	typedef QSharedPointer<virConnect> linkReference_type;

	explicit Abstract(const domainReference_type& domain_): m_domain(domain_)
	{
	}

	linkReference_type getLink() const;
	const domainReference_type& getDomain() const
	{
		return m_domain;
	}
	void setDomain(virDomainPtr value_);

private:
	QSharedPointer<virDomain> m_domain;
};

} // namespace Limb

namespace Block
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	// need this empty constructor for ability to store Unit in QHash
	Unit()
	{
	}
	Unit(const QSharedPointer<virDomain>& domain_, const QString& disk_):
		m_domain(domain_), m_disk(disk_)
	{
	}

	const QString& getDisk() const
	{
		return m_disk;
	}
	Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> getProgress() const;
	Result copy(const CVmHardDisk& target_) const;
	Result commit() const;
	Result rebase(const QString& base_) const;
	Result resize(quint64 bytes_) const;

	Result abort() const;
	Result finish() const;

private:
	Result getInfo(virDomainBlockJobInfo& dst_) const;

	QSharedPointer<virDomain> m_domain;
	QString m_disk;
};

///////////////////////////////////////////////////////////////////////////////
// struct Completion

struct Completion: QObject
{
	Completion()
	{
		m_future = m_promise.get_future();
	}

	void wait()
	{
		m_future.wait();
	}
	void occur()
	{
		m_promise.set_value();
		emit done();
	}

signals:
	void done();

private:
	Q_OBJECT

	boost::future<void> m_future;
	boost::promise<void> m_promise;
};

///////////////////////////////////////////////////////////////////////////////
// struct Counter

struct Counter: QObject
{
	typedef boost::function<PRL_RESULT ()> product_type;

	template<class T>
	Counter(const QList<T>& componentList_, Completion& receiver_):
		m_receiver(&receiver_)
	{
		foreach(const T& c, componentList_)
		{
			m_pending << c.getTarget();
		}
		if (m_pending.isEmpty())
			m_receiver->occur();
	}

	void account(const QString& one_, PRL_RESULT status_);
	void reset();
	product_type read();

protected:
	void account_(QString one_);
	Q_INVOKABLE virtual product_type read_() = 0;
	Q_INVOKABLE virtual void account_(QString one_, PRL_RESULT status_) = 0;
	Q_INVOKABLE void reset_();

private:
	Q_OBJECT


	Completion* m_receiver;
	QSet<QString> m_pending;
};

///////////////////////////////////////////////////////////////////////////////
// struct Callback

struct Callback
{
	Callback(QSharedPointer<Counter> counter_, int filter_):
		m_filter(filter_), m_counter(counter_)
	{
	}
	~Callback();

	void do_(int event_, const QString& object_, PRL_RESULT status_);

private:
	int m_filter;
	QSharedPointer<Counter> m_counter;
};

///////////////////////////////////////////////////////////////////////////////
// struct Tracker

struct Tracker: boost::noncopyable
{
	explicit Tracker(QSharedPointer<virDomain> domain_);

	Result start(QSharedPointer<Counter> counter_, int event_);
	Prl::Expected<Counter::product_type, ::Error::Simple> stop();

private:
	static void react(virConnectPtr, virDomainPtr, const char * disk_,
		int type_, int status_, void * opaque_);
	static void free(void* opaque_);

	int m_ticket;
	int m_ticket2;
	QSharedPointer<Counter> m_counter;
	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Activity

struct Activity
{
	Activity()
	{
	}
	explicit Activity(const QSharedPointer<Tracker>& tracker_):
		m_tracker(tracker_)
	{
	}

	PRL_RESULT stop();

private:
	QSharedPointer<Tracker> m_tracker;
};

///////////////////////////////////////////////////////////////////////////////
// struct Launcher

struct Launcher: private Limb::Abstract
{
	typedef QList<CVmHardDisk> imageList_type;

	explicit Launcher(const domainReference_type& domain_):
		Limb::Abstract(domain_)
	{
	}

	Activity copy(const QHash<QString, CVmHardDisk>& imageList_, Completion& completion_);
	Activity merge(const imageList_type& imageList_, Completion& completion_);
	Activity rebase(const imageList_type& imageList_, Completion& completion_);

private:
	template<class T, class U>
	Activity start(T strategy_, const QList<U>& componentList_, Completion& completion_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Export

struct Export: private Limb::Abstract
{
	typedef boost::tuple<QString, QString, QUrl> component_type;
	typedef QList<component_type> componentList_type;

	explicit Export(const domainReference_type& domain_):
		Limb::Abstract(domain_)
	{
	}

	Result stop(const componentList_type& request_);
	Result start(const componentList_type& request_);
	Prl::Expected<componentList_type, ::Error::Simple> list();
};

} // namespace Block

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request
{
	Request();

	const QString& getDescription() const
	{
		return m_description;
	}

	void setDescription(const QString& description_)
	{
		m_description = description_;
	}

	quint32 getFlags() const
	{
		return m_flags;
	}

	void setDiskOnly();
	void setConsistent();

private:
	QString m_description;
	quint32 m_flags;
};

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
// struct Block

struct Block
{
	Block(virDomainBlockSnapshotXPtr snapshot_, const QString& map_);

	Result undefine();
	Result getUuid(QString& dst_) const;
	Result deleteMap();

private:
	const QString m_map;
	QSharedPointer<virDomainBlockSnapshotX> m_object;
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
	Result define(const QString& uuid_, const Request& req_,
		Unit* dst_ = NULL);
	Result createExternal(const QString& uuid_, const QList<CVmHardDisk*>& disks_);
	Prl::Expected<Block, ::Error::Simple>
		defineBlock(const QString& uuid_, const QList<SnapshotComponent>& disks_);
	
private:
	Prl::Expected<Unit, ::Error::Simple>
		define_(const QString& uuid_, const Request& req_);

	static Result translate
		(const Prl::Expected<Unit, ::Error::Simple>& result_, Unit* dst_);

	QSharedPointer<virDomain> m_domain;
};

} // namespace Snapshot

namespace Exec
{

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

struct Agent: protected Limb::Abstract
{
	Agent(const domainReference_type& domain_, const QString& uri_):
		Limb::Abstract(domain_), m_uri(uri_)
	{
	}

	Result cancel();
	Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> getProgress();

protected:
	Result setDowntime(quint32 value_);
	Result migrate(const CVmConfiguration& config_, unsigned int flags_,
		Parameters::Builder& parameters_);

private:
	QString m_uri;
};

///////////////////////////////////////////////////////////////////////////////
// struct Online

struct Online: Agent
{
	typedef Prl::Expected<quint64, ::Error::Simple> result_type;

	explicit Online(const Agent& agent_);

	void setQemuState(qint32 port_);
	void setQemuDisk(const QList<CVmHardDisk* >& list_, qint32 port_);
	void setQemuDisk(const QList<CVmHardDisk* >& list_);
	void setUncompressed()
	{
		m_compression.clear();
	}
	void setBandwidth(quint64 value_);

	result_type operator()(const CVmConfiguration& config_);

private:
	Result migrate(const CVmConfiguration& config_);

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

namespace Limb
{
///////////////////////////////////////////////////////////////////////////////
// struct State

struct State: private Abstract
{
	explicit State(const domainReference_type& domain_): Abstract(domain_)
	{
	}

	Result kill();
	Result shutdown();
	Result start();
	Result startPaused();
	Result reboot();
	Result reset();
	Result pause();
	Result unpause();
	Result resume(const QString& sav_);
	Result suspend(const QString& sav_);
	Result undefine();
	Migration::Agent migrate(const QString& uri_);
	Result getValue(VIRTUAL_MACHINE_STATE& dst_) const;

private:
	Result start_(unsigned int flags_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Maintenance

struct Maintenance: private Abstract
{
	explicit Maintenance(const domainReference_type& domain_): Abstract(domain_)
	{
	}

	void emitDefined();
	void emitRestored();
	void emitQemuUpdated();
	void emitAgentCorollary(PRL_VM_TOOLS_STATE state_);
	Result updateQemu();
	Result adjustClock(qint64 adjusment_);

private:
	void emitLifecycle(int category_, int type_);
};

} // namespace Limb

///////////////////////////////////////////////////////////////////////////////
// struct Editor

struct Editor: private Limb::Abstract
{
	explicit Editor(const domainReference_type& domain_, quint32 flags_)
		: Limb::Abstract(domain_), m_flags(flags_)
	{
	}

	Result setPerCpuLimit(quint32 limit_, quint32 period_);
	Result setGlobalCpuLimit(quint32 limit_, quint32 period_);
	Result setImageSize(const CVmHardDisk& disk_, quint64 bytes_);
	Result setIoLimit(const CVmHardDisk& disk_, quint32 limit_);
	Result setIopsLimit(const CVmHardDisk& disk_, quint32 limit_);
	Result setIoPriority(quint32 ioprio_);
	Result setCpuUnits(quint32 units_);
	Result setCpuCount(quint32 count_);
	Result setCpuMask(quint32 ncpus_, const QString& mask_);
	Result setNodeMask(const QString& mask_);

	template<class T>
	Result plug(const T& device_);
	template<class T>
	Result unplug(const T& device_);
	template<class T>
	Result update(const T& device_);

private:
	template<class T>
	Result plugAndWait(const T& device_);
	Result unplugAndWait(const QString& xml_, const QString& alias_);
	Result setCpuLimit(quint32 globalLimit_, quint32 limit_, quint32 period_);
	Result setBlockIoTune(const CVmHardDisk& disk_, const char* param_, quint32 limit_);

	quint32 m_flags;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct List;
struct Guest;
struct Unit: private Limb::Abstract
{
	explicit Unit(virDomainPtr domain_ = NULL);

	Result getUuid(QString& dst_) const;
	Result rename(const QString& to_);
	Result getConfig(CVmConfiguration& dst_, bool runtime_ = false) const;
	Result getConfig(QString& dst_, bool runtime_ = false) const;
	Result setConfig(const CVmConfiguration& value_);
	Result completeConfig(CVmConfiguration& config_);
	Result setMemoryStatsPeriod(qint64 seconds_);

	List up() const;
	Guest getGuest() const;
	Snapshot::List getSnapshot() const;
	Editor getRuntime() const;
	Editor getEditor() const;

	Limb::State getState() const;
	Limb::Maintenance getMaintenance() const;
	Block::Launcher getVolume() const;
	Block::Export getExport() const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Grub

struct Grub
{
	typedef Prl::Expected<Unit, ::Error::Simple> result_type;

	Grub(const Limb::Abstract::linkReference_type& link_, const CVmConfiguration& image_);

	result_type spawnPaused();
	result_type spawnRunning();
	result_type spawnPersistent();

private:
	typedef boost::function<virDomainPtr (virConnectPtr, char* )>
		decorated_type;

	result_type wrap(const decorated_type& decorated_) const;

	QSharedPointer<CVmConfiguration> m_image;
	Limb::Abstract::linkReference_type m_link;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Unit at(const QString& uuid_) const;
	Grub getGrub(const CVmConfiguration& image_);
	Result all(QList<Unit>& dst_);
	QList<Performance::Unit> getPerformance();

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Vm

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
	QString m_name;
};

namespace List
{
///////////////////////////////////////////////////////////////////////////////
// struct Backend

struct Backend
{
	typedef Prl::Expected<QList<Bridge>, ::Error::Simple> all_type;

	explicit Backend(const QSharedPointer<virConnect>& link_);

	const all_type& getAll() const
	{
		return m_all;
	}
	Prl::Expected<Bridge, ::Error::Simple> findByName(const QString& name_) const;
	Prl::Expected<Bridge, ::Error::Simple> findByMasterName(const QString& name_) const;
	Prl::Expected<Bridge, ::Error::Simple> findByMasterMacAndVlan(const QString& mac_, int tag_) const;
	Backend& reload();

private:
	all_type m_all;
	QSharedPointer<virConnect> m_link;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend
{
	explicit Frontend(const QSharedPointer<virConnect>& link_): m_link(link_)
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

} // namespace List
} // namespace Interface

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	typedef boost::function<Prl::Expected<CVirtualNetwork, ::Error::Simple>
		(virNetworkPtr, const Interface::List::Backend&)> read_type;

	Unit()
	{
	}
	Unit(virNetworkPtr network_, const read_type& read_);

	Result stop();
	Result start();
	Result undefine();
	Result getConfig(CVirtualNetwork& dst_) const;

private:
	read_type m_read;
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
	Result all(QList<CVirtualNetwork>& dst_) const;
	Result find(const QString& name_, Unit* dst_ = NULL) const;
	Result define(const CVirtualNetwork& config_, Unit* dst_ = NULL);

private:
	Unit craft(virNetworkPtr network_) const;
	static Prl::Expected<CVirtualNetwork, ::Error::Simple>
		read(virNetworkPtr network_, const Interface::List::Backend& bridges_);

	QSharedPointer<virConnect> m_link;
};

} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host
{
	explicit Host(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Prl::Expected<VtInfo, ::Error::Simple> getVt() const;
	Prl::Expected<QList<CHwGenericPciDevice>, ::Error::Simple>
		getAssignablePci() const;

private:
	QSharedPointer<virConnect> m_link;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub
{
	Hub();

	Vm::List vms()
	{
		return Vm::List(m_link.toStrongRef());
	}
	Network::List networks()
	{
		return Network::List(m_link.toStrongRef());
	}
	Interface::List::Frontend interfaces()
	{
		return Interface::List::Frontend(m_link.toStrongRef());
	}

	void setLink(QSharedPointer<virConnect> value_);

	Host host()
	{
		return Host(m_link);
	}

private:
	QMutex m_mutex;
	QWeakPointer<virConnect> m_link;
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

