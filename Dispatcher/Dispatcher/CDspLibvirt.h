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
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Performance

struct Performance
{
	explicit Performance(const QSharedPointer<virDomain>& domain_):
		m_domain(domain_)
	{
	}

	Result getCpu(quint64& nanoseconds_) const;
	Result getDisk() const;
	Result getMemory() const;
	Result getNetwork() const;

private:
	QSharedPointer<virDomain> m_domain;
};

namespace Command
{

struct Future;

} // namespace Command

namespace Exec
{

struct Future;
struct Result;
struct Request;
struct AsyncExecDevice;
struct AuxChannel;

} // namespace Exec

///////////////////////////////////////////////////////////////////////////////
// struct Guest

struct Guest
{
	explicit Guest(const  QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	Result traceEvents(bool enable_); 
	Result dumpScreen(const QString& path);
	Prl::Expected<QString, ::Error::Simple>
		dumpMemory(const QString& path);
	Prl::Expected<Command::Future, ::Error::Simple>
		dumpState(const QString& path_);
	Result setUserPasswd(const QString& user_, const QString& passwd_, bool crypted_);
	Result checkAgent();
	Prl::Expected<QString, Error::Simple> getAgentVersion();
	Prl::Expected<Exec::Future, Error::Simple> startProgram(const Exec::Request& r);
	Prl::Expected<Exec::Result, Error::Simple> runProgram(const Exec::Request& r);
	Prl::Expected<QString, Error::Simple>
		execute(const QString& cmd, bool isHmp = true);

private:
	QSharedPointer<virDomain> m_domain;
};

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Future

struct Future
{
	Future(const QSharedPointer<virDomain>& domain_,
		const std::string& status_ = std::string())
	: m_guest(domain_), m_status(status_)
	{
	}

	Result wait(int timeout_);

private:
	Guest m_guest;
	std::string m_status;
};

} // namespace Command

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
	Result setCpuLimit(quint32 limit_, quint32 period_);
	Result setCpuUnits(quint32 units_);
	Result setCpuCount(quint32 count_);

	template<class T>
	Result plug(const T& device_);
	template<class T>
	Result unplug(const T& device_);
	template<class T>
	Result update(const T& device_);

private:
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

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct List;
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
	List up() const;
	Performance getPerformance() const
	{
		return Performance(m_domain);
	}
	Guest getGuest() const
	{
		return Guest(m_domain);
	}
	Snapshot::List getSnapshot() const
	{
		return Snapshot::List(m_domain);
	}
	Runtime getRuntime() const;
	Result addAsyncExec(Exec::AsyncExecDevice& device_);

private:
	char* getConfig(bool runtime_ = false) const;
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
	void setLink(QSharedPointer<virConnect> value_)
	{
		m_link = value_.toWeakRef();
	}
	Host host()
	{
		return Host(m_link);
	}
	int addAsyncExec(const QString & uuid_, QSharedPointer<virDomain> domain_,
			Vm::Exec::AsyncExecDevice& device_);

private:
	QWeakPointer<virConnect> m_link;
	QMap<QString, QSharedPointer<Vm::Exec::AuxChannel> > m_execs;
};

} // namespace Agent
} // namespace Instrument

extern Instrument::Agent::Hub Kit;

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host: QThread
{
protected:
        void run();

private:
        Q_OBJECT
};

} // namespace Libvirt

#endif // __CDSPLIBVIRT_H__

