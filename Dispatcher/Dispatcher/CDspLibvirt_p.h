///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirt_p.h
///
/// Private interfaces of the libvirt interaction.
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

#ifndef __CDSPLIBVIRT_P_H__
#define __CDSPLIBVIRT_P_H__

#include <QTimer>
#include "CDspClient.h"
#include "CDspLibvirt.h"
#include "CDspRegistry.h"
#include <QSocketNotifier>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/Std/noncopyable.h>
#include <boost/ptr_container/ptr_map.hpp>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>

namespace Libvirt
{
namespace Callback
{
///////////////////////////////////////////////////////////////////////////////
// struct Base

struct Base: noncopyable
{
	explicit Base(int id_): m_id(id_), m_opaque(), m_free()
	{
	}
	~Base()
	{
		if (NULL != m_free)
			m_free(m_opaque);
	}

	int getId() const
	{
		return m_id;
	}
	void setOpaque(void* opaque_, virFreeCallback free_)
	{
		m_free = free_;
		m_opaque = opaque_;
	}

protected:
	void* getOpaque() const
	{
		return m_opaque;
	}

private:
	int m_id;
	void* m_opaque;
	virFreeCallback m_free;
};

///////////////////////////////////////////////////////////////////////////////
// struct Timeout

struct Timeout: QObject, Base
{
	Timeout(virEventTimeoutCallback impl_, int id_);
	~Timeout();

	void enable(int interval_);
	void disable()
	{
		enable(-1);
	}

public slots:
	void handle();

private:
	Q_OBJECT

	QTimer m_timer;
	virEventTimeoutCallback m_impl;
};

///////////////////////////////////////////////////////////////////////////////
// struct Socket

struct Socket: QObject, Base
{
	Socket(int socket_, virEventHandleCallback impl_, int id_);
	~Socket();

	void enable(int events_);
	void disable();

public slots:
	void read(int socket_);
	void error(int socket_);
	void write(int socket_);

private:
	Q_OBJECT

	virEventHandleCallback m_impl;
	QSocketNotifier m_read;
	QSocketNotifier m_write;
	QSocketNotifier m_error;
};

///////////////////////////////////////////////////////////////////////////////
// struct Sweeper

struct Sweeper: QObject
{
	explicit Sweeper(int id_): m_id(id_)
	{
	}

	void care(Socket* value_)
	{
		m_pet1.reset(value_);
	}
	void care(Timeout* value_)
	{
		m_pet2.reset(value_);
	}

protected:
	void timerEvent(QTimerEvent* );

private:
	Q_OBJECT

	int m_id;
	QScopedPointer<Socket> m_pet1;
	QScopedPointer<Timeout> m_pet2;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub: QObject
{
	Q_INVOKABLE void add(int id_, virEventTimeoutCallback callback_);
	Q_INVOKABLE void add(int id_, int socket_, virEventHandleCallback callback_);
	Q_INVOKABLE void remove(int id_);
	Q_INVOKABLE void setEvents(int id_, int value_);
	Q_INVOKABLE void setInterval(int id_, int value_);
	Q_INVOKABLE void setOpaque(int id_, void* opaque_, virFreeCallback free_);

private:
	Q_OBJECT

	boost::ptr_map<int, Socket> m_socketMap;
	boost::ptr_map<int, Timeout> m_timeoutMap;
	boost::ptr_map<int, Sweeper> m_sweeperMap;
};

///////////////////////////////////////////////////////////////////////////////
// struct Access

struct Access
{
	QSharedPointer<Hub> getHub() const
	{
		return m_hub.toStrongRef();
	}
	void setHub(const QSharedPointer<Hub>& hub_);
	int add(int interval_, virEventTimeoutCallback callback_, void* opaque_, virFreeCallback free_);
	int add(int socket_, int events_, virEventHandleCallback callback_, void* opaque_, virFreeCallback free_);
	void setEvents(int id_, int value_);
	void setInterval(int id_, int value_);
	int remove(int id_);

private:
	QAtomicInt m_generator;
	QWeakPointer<Hub> m_hub;
};

} // namespace Callback

namespace Model
{
///////////////////////////////////////////////////////////////////////////////
// struct Tray

struct Tray
{
	Tray(virDomainPtr domain_, const char* alias_);

	void open(Registry::Reactor& vm_);

	void close(Registry::Reactor& vm_);

private:
	boost::optional<CVmOpticalDisk> find() const;

	QString m_alias;
	Instrument::Agent::Vm::Unit m_agent;
};

///////////////////////////////////////////////////////////////////////////////
// struct Domain

struct Domain: QObject
{
	typedef boost::function1<void, Registry::Reactor> reaction_type;

	explicit Domain(const Registry::Access& access_);

	void setPid(quint32 value_)
	{
		m_pid = value_;
	}
	Q_INVOKABLE void show(reaction_type reaction_);
	Q_INVOKABLE void setState(VIRTUAL_MACHINE_STATE value_);
	Q_INVOKABLE void crash();

	boost::optional<CVmConfiguration> getConfig();
	void setConfig(CVmConfiguration value_);

	void setCounters(const Instrument::Agent::Vm::Stat::CounterList_type& src_);

private:
	Q_OBJECT


	quint32 m_pid;
	Registry::Access m_access;
	QList<quint64> m_crashes;
};

///////////////////////////////////////////////////////////////////////////////
// struct System

struct System: QObject
{
	explicit System(Registry::Actual& registry_);

	void remove(const QString& uuid_);
	QSharedPointer<Domain> add(const QString& uuid_);
	QSharedPointer<Domain> find(const QString& uuid_);

private:
	Q_OBJECT

	typedef QHash<QString, QSharedPointer<Domain> > domainMap_type;
	domainMap_type m_domainMap;
	Registry::Actual& m_registry;
};

///////////////////////////////////////////////////////////////////////////////
// struct Coarse

struct Coarse
{
	explicit Coarse(QSharedPointer<System> fine_): m_fine(fine_)
	{
	}

	void setState(virDomainPtr domain_, VIRTUAL_MACHINE_STATE value_);
	void prepareToSwitch(virDomainPtr domain_);
	bool show(virDomainPtr domain_,
		const Domain::reaction_type& reaction_);
	void remove(virDomainPtr domain_);
	void pullInfo(virDomainPtr domain_);

	static QString getUuid(virDomainPtr domain_);
	void disconnectDevice(virDomainPtr domain_, const QString& alias_);
	void adjustClock(virDomainPtr domain_, qint64 offset_);
	void onCrash(virDomainPtr domain_);
	void updateInterfaces(virNetworkPtr domain_);

private:
	QSharedPointer<System> m_fine;
};

} // namespace Model

namespace Monitor
{
enum
{
	RECONNECT_TIMEOUT = 1000,
	PERFORMANCE_TIMEOUT = 10000
};

///////////////////////////////////////////////////////////////////////////////
// struct Link

struct Link: QObject
{
	explicit Link(int timeout_ = RECONNECT_TIMEOUT);

public slots:
	void setOpen();
	void setClosed();

signals:
	void connected(QSharedPointer<virConnect>);
	void disconnected();

private:
	Q_OBJECT

	static void disconnect(virConnectPtr , int , void* );

	QTimer m_timer;
	QSharedPointer<virConnect> m_libvirtd;
};

///////////////////////////////////////////////////////////////////////////////
// struct Domains

struct Domains: QObject
{
	explicit Domains(Registry::Actual& registry_);

public slots:
	void setConnected(QSharedPointer<virConnect>);
	void setDisconnected();

private:
	Q_OBJECT

	int m_eventState;
	int m_eventReboot;
	int m_eventWakeUp;
	int m_eventDeviceConnect;
	int m_eventDeviceDisconnect;
	int m_eventTrayChange;
	int m_eventRtcChange;
	int m_eventAgent;
	int m_eventNetworkLifecycle;
	Registry::Actual* m_registry;
	QWeakPointer<virConnect> m_libvirtd;
	QSharedPointer<Model::System> m_view;
};

namespace Performance
{
///////////////////////////////////////////////////////////////////////////////
// struct Miner

struct Miner: QObject
{
	Miner(const Instrument::Agent::Vm::List& agent_, const QWeakPointer<Model::System>& view_):
		m_agent(agent_), m_view(view_)
	{
	}

	PRL_RESULT operator()();
	Miner* clone() const;

protected:
	void timerEvent(QTimerEvent* );

private:
	Q_OBJECT

	static void superfuse(const Instrument::Agent::Vm::Performance::Unit& source_,
		Model::Domain& sink_);

	Instrument::Agent::Vm::List m_agent;
	QWeakPointer<Model::System> m_view;
};

///////////////////////////////////////////////////////////////////////////////
// struct Task

struct Task: QRunnable
{
	explicit Task(Miner* miner_): m_miner(miner_)
	{
	}

	void run();

private:
	QScopedPointer<Miner> m_miner;
};

} // namespace Performance
} // namespace Monitor

namespace Instrument
{
namespace Pull
{
///////////////////////////////////////////////////////////////////////////////
// struct Crash

struct Crash: QRunnable
{
	Crash(const QSharedPointer<Model::Domain>& domain_, const CVmConfiguration& config_):
		m_domain(domain_), m_config(config_)
	{
		setAutoDelete(true);
	}

	void run();
	void sendProblemReport();

private:
	QSharedPointer<Model::Domain> m_domain;
	CVmConfiguration m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct State

struct State
{
	State();

	void read(Agent::Vm::Unit agent_);
	void apply(const QSharedPointer<Model::Domain>& domain_);
	VIRTUAL_MACHINE_STATE getValue() const
	{
		return m_value;
	}

private:
	VIRTUAL_MACHINE_STATE m_value;
};

///////////////////////////////////////////////////////////////////////////////
// struct Config

struct Config: QRunnable
{
	Config(const Agent::Vm::Unit& agent_, QSharedPointer<Model::Domain> view_):
		m_agent(agent_), m_view(view_)
	{
		setAutoDelete(true);
	}

	void run();

private:
	Agent::Vm::Unit m_agent;
	QSharedPointer<Model::Domain> m_view;
};

///////////////////////////////////////////////////////////////////////////////
// struct Everything

struct Everything: Config
{
	Everything(const Agent::Vm::Unit& agent_, QSharedPointer<Model::Domain> view_):
		Config(agent_, view_), m_agent(agent_), m_view(view_)
	{
	}

	void run();

private:
	Agent::Vm::Unit m_agent;
	QSharedPointer<Model::Domain> m_view;
};

///////////////////////////////////////////////////////////////////////////////
// struct Acquaintance

struct Acquaintance: Config
{
	Acquaintance(const Agent::Vm::Unit& agent_, QSharedPointer<Model::Domain> view_):
		Config(agent_, view_), m_agent(agent_), m_view(view_)
	{
	}

	void run();

private:
	Agent::Vm::Unit m_agent;
	QSharedPointer<Model::Domain> m_view;
};

///////////////////////////////////////////////////////////////////////////////
// struct Switch

struct Switch: Config
{
	Switch(const Agent::Vm::Unit& agent_, QSharedPointer<Model::Domain> view_):
		Config(agent_, view_), m_agent(agent_), m_view(view_)
	{
	}

	void run();

private:
	Agent::Vm::Unit m_agent;
	QSharedPointer<Model::Domain> m_view;
};

///////////////////////////////////////////////////////////////////////////////
// struct Network

struct Network: QRunnable
{
	Network(const QString& network_, const QSharedPointer<Model::System>& fine_)
		: m_network(network_), m_fine(fine_)
	{
	}

	void run();

private:
	QString m_network;
	QSharedPointer<Model::System> m_fine;
};

} // namespace Pull

namespace Agent
{
///////////////////////////////////////////////////////////////////////////////
// struct Config

struct Config
{
	Config(QSharedPointer<virDomain> domain_,
		QSharedPointer<virConnect> link_,
		unsigned int flags_);

	Prl::Expected<QString, Error::Simple> read() const;

	Result convert(CVmConfiguration& dst_) const;
	Prl::Expected<QString, Error::Simple> mixup(const CVmConfiguration& value_) const;
	Prl::Expected<QString, Error::Simple> fixup(const CVmConfiguration& value_) const;
	Prl::Expected<QString, Error::Simple> adjustClock(qint64 offset_) const;

private:
	char* read_() const;

	QSharedPointer<virDomain> m_domain;
	QSharedPointer<virConnect> m_link;
	unsigned int m_flags;
};

namespace Parameters
{
typedef QPair<QSharedPointer<virTypedParameter>, qint32> Result_type;

///////////////////////////////////////////////////////////////////////////////
// struct Builder

// reuseable, you can add many cycles of "add, add, ..., extract"
struct Builder: noncopyable
{
	Builder();
	~Builder();

	bool add(const char *key_, const QString& value_);
	bool add(const char *key_, quint64 value_);
	bool add(const char *key_, qint64 value_);
	bool add(const char *key_, qint32 value_);
	Result_type extract();

private:
	virTypedParameterPtr m_pointer;
	qint32 m_size;
	qint32 m_capacity;
};

} // namespace Parameters

namespace Vm
{
namespace Migration
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic

struct Basic
{
	Basic(const CVmConfiguration& config_, Parameters::Builder& builder_):
		m_builder(&builder_), m_config(&config_)
	{
	}

	Result addXml(const char* parameter_, Config agent_);

	Result addName();

private:
	Parameters::Builder* m_builder;
	const CVmConfiguration* m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Compression

struct Compression
{
	Result operator()(Parameters::Builder& builder_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth

struct Bandwidth
{
	explicit Bandwidth(quint64 value_): m_value(value_)
	{
	}

	Result operator()(Parameters::Builder& builder_);

private:
	quint64 m_value;
};

namespace Qemu
{
///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk
{
	Disk(qint32 port_, const QList<CVmHardDisk* >& list_):
		m_port(port_), m_list(list_)
	{
	}
	explicit Disk(const QList<CVmHardDisk* >& list_):
		m_list(list_)
	{
	}

	Result operator()(Parameters::Builder& builder_);

private:
	boost::optional<qint32> m_port;
	QList<CVmHardDisk* > m_list;
};

///////////////////////////////////////////////////////////////////////////////
// struct State

struct State
{
	explicit State(qint32 port_): m_port(port_)
	{
	}

	Result operator()(Parameters::Builder& builder_);

private:
	qint32 m_port;
};

} // namespace Qemu
} // namespace Migration
} // namespace Vm
} // namespace Agent

namespace Breeding
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	Vm(const QSharedPointer<Model::System>& view_, Registry::Actual& registry_):
		m_registry(&registry_), m_view(view_)
	{
	}

	void operator()(Agent::Hub& hub_);
private:
	bool validate(const QString& uuid_);

	Registry::Actual* m_registry;
	QSharedPointer<Model::System> m_view;
};

///////////////////////////////////////////////////////////////////////////////
// struct Network

struct Network
{
	explicit Network(const QFileInfo& config_);

	void operator()(Agent::Hub& hub_);

private:
	QFileInfo m_digested;
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject: QRunnable
{
	Subject(QSharedPointer<virConnect> , QSharedPointer<Model::System> ,
		Registry::Actual& );

	void run();

private:
	Vm m_vm;
	Network m_network;
	Agent::Hub m_hub;
};

} // namespace Breeding

///////////////////////////////////////////////////////////////////////////////
// struct Pipeline

struct Pipeline
{
	typedef Prl::Expected<void, QString> result_type;
	typedef QPair<QSharedPointer<QProcess>, QString> proc_type;

	explicit Pipeline(const QStringList &cmds_):
		m_cmds(cmds_)
	{
	}

	result_type operator()(const QByteArray &input_ = QByteArray(),
						   const QString &outFile_ = QString()) const;

private:
	QList<proc_type> build() const;

	QStringList m_cmds;
};

} // namespace Instrument
} // namespace Libvirt

#endif // __CDSPLIBVIRT_P_H__

