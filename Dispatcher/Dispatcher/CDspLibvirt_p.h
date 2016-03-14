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

class CDspService;
class CDspDispConfigGuard;

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
// struct Domain

struct Domain: QObject
{
	explicit Domain(const Registry::Access& access_);

	void setPid(quint32 value_)
	{
		m_pid = value_;
	}
	Q_INVOKABLE void setState(VIRTUAL_MACHINE_STATE value_);
	Q_INVOKABLE void prepareToSwitch();

	boost::optional<CVmConfiguration> getConfig();
	void setConfig(CVmConfiguration& value_);
	void setCpuUsage();
	void setDiskUsage();
	void setMemoryUsage(const Instrument::Agent::Vm::Stat::Memory& src_);
	void setInterfaceUsage(const Instrument::Agent::Vm::Stat::Interface& iface_);

private:
	Q_OBJECT


	quint32 m_pid;
	Registry::Access m_access;
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

	Registry::Actual& m_registry;

	typedef QHash<QString, QSharedPointer<Domain> > domainMap_type;
	domainMap_type m_domainMap;
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
	void remove(virDomainPtr domain_);
	void sendProblemReport(virDomainPtr domain_);
	QSharedPointer<Domain> access(virDomainPtr domain_);

	static QString getUuid(virDomainPtr domain_);
	void disconnectCd(virDomainPtr domain_, const QString& alias_);

private:
	QSharedPointer<System> m_fine;
};

} // namespace Model

namespace Monitor
{
enum
{
	DEFAULT_TIMEOUT = 1000
};

///////////////////////////////////////////////////////////////////////////////
// struct Link

struct Link: QObject
{
	explicit Link(int timeout_ = DEFAULT_TIMEOUT);

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
	explicit Domains(Registry::Actual& registry_,
		int timeout_ = DEFAULT_TIMEOUT);

public slots:
	void getPerformance();
	void setConnected(QSharedPointer<virConnect>);
	void setDisconnected();

private:
	Q_OBJECT

	QTimer m_timer;
	int m_eventState;
	int m_eventReboot;
	int m_eventWakeUp;
	int m_eventDeviceConnect;
	int m_eventDeviceDisconnect;
	int m_eventTrayChange;
	QWeakPointer<virConnect> m_libvirtd;
	QSharedPointer<Model::System> m_view;
};

} // namespace Monitor

namespace Instrument
{
///////////////////////////////////////////////////////////////////////////////
// struct Domain

struct Domain: QRunnable
{
	Domain(const Agent::Vm::Unit& agent_, QSharedPointer<Model::Domain> view_):
		m_agent(agent_), m_view(view_)
	{
	}
	Domain(virDomainPtr model_, QSharedPointer<Model::Domain> view_);

	void run();

private:
	Agent::Vm::Unit m_agent;
	QSharedPointer<Model::Domain> m_view;
};

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
// struct Offline

struct Offline
{
	Offline(const Config& agent_, const CVmConfiguration& config_):
		m_agent(agent_), m_config(&config_)
	{
	}

	Result operator()(Parameters::Builder& builder_);

private:
	Config m_agent;
	const CVmConfiguration* m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Online

struct Online: private Offline
{
	Online(const Config& agent_, const CVmConfiguration& config_,
		const QList<CVmHardDisk* >& disks_):
		Offline(agent_, config_), m_disks(disks_)
	{
	}

	Result operator()(Parameters::Builder& builder_);

private:
	QList<CVmHardDisk* > m_disks;
};

} // namespace Migration
} // namespace Vm
} // namespace Agent

namespace Breeding
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	explicit Vm(const QSharedPointer<Model::System>& view_): m_view(view_)
	{
	}

	void operator()(Agent::Hub& hub_);

private:
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
	Subject(QSharedPointer<virConnect> , QSharedPointer<Model::System> );

	void run();

private:
	Vm m_vm;
	Network m_network;
	Agent::Hub m_hub;
};

} // namespace Breeding

///////////////////////////////////////////////////////////////////////////////
// struct Performance

struct Performance: QRunnable
{
	Performance(QSharedPointer<virConnect> libvirtd_, QSharedPointer<Model::System> view_):
		m_agent(libvirtd_), m_view(view_)
	{
	}

	void run();

private:
	void pull(Agent::Vm::Unit vm_);

	Agent::Vm::List m_agent;
	QSharedPointer<Model::System> m_view;
};

} // namespace Instrument
} // namespace Libvirt

#endif // __CDSPLIBVIRT_P_H__

