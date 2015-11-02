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
#include <QSocketNotifier>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <Libraries/Std/SmartPtr.h>
#include <Libraries/Std/noncopyable.h>
#include <boost/ptr_container/ptr_map.hpp>

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

namespace View
{
///////////////////////////////////////////////////////////////////////////////
// struct Domain

struct Domain
{
	Domain(const QString& uuid_, const SmartPtr<CDspClient>& user_);

	void setPid(quint32 value_)
	{
		m_pid = value_;
	}
	void setState(VIRTUAL_MACHINE_STATE value_);
	void setConfig(CVmConfiguration& value_);
	void setCpuUsage();
	void setDiskUsage();
	void setMemoryUsage();
	void setNetworkUsage();
	boost::optional<CVmConfiguration> getConfig() const;

private:
	quint32 m_pid;
	QString m_home;
	QString m_uuid;
	SmartPtr<CDspClient> m_user;
	VIRTUAL_MACHINE_STATE m_state;
	VIRTUAL_MACHINE_STATE m_formerState;
};

///////////////////////////////////////////////////////////////////////////////
// struct System

struct System
{
	System();

	void remove(const QString& uuid_);
	QSharedPointer<Domain> add(const QString& uuid_);
	QSharedPointer<Domain> find(const QString& uuid_) const;
private:
	typedef QHash<QString, QSharedPointer<Domain> > domainMap_type;

	domainMap_type m_domainMap;
	CDspDispConfigGuard* m_configGuard;
};

///////////////////////////////////////////////////////////////////////////////
// struct Coarse

struct Coarse
{
	explicit Coarse(QSharedPointer<System> fine_): m_fine(fine_)
	{
	}

	void setState(virDomainPtr domain_, VIRTUAL_MACHINE_STATE value_);
	void remove(virDomainPtr domain_);
	void sendProblemReport(virDomainPtr domain_);
	QSharedPointer<Domain> access(virDomainPtr domain_);

	static QString getUuid(virDomainPtr domain_);

private:
	QSharedPointer<System> m_fine;
};

} // namespace View

namespace Monitor
{
enum
{
	DEFAULT_TIMEOUT = 1000
};

struct State: QObject
{
	State(QSharedPointer<View::System> system_);

public slots:
	void updateConfig(unsigned oldState_, unsigned newState_, QString vmUuid_, QString dirUuid_);

private:
	Q_OBJECT

	QSharedPointer<View::System> m_system;
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
	explicit Domains(int timeout_ = DEFAULT_TIMEOUT);

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
	int m_eventDeviceDisconnect;
	QWeakPointer<virConnect> m_libvirtd;
	QSharedPointer<View::System> m_view;
	State m_stateWatcher;
};

} // namespace Monitor

namespace Tools
{
///////////////////////////////////////////////////////////////////////////////
// struct Domain

struct Domain: QRunnable
{
	Domain(const Agent::Vm::Unit& agent_, QSharedPointer<View::Domain> view_):
		m_agent(agent_), m_view(view_)
	{
	}
	Domain(virDomainPtr model_, QSharedPointer<View::Domain> view_);

	void run();

private:
	Agent::Vm::Unit m_agent;
	QSharedPointer<View::Domain> m_view;
};

namespace Breeding
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	explicit Vm(const QSharedPointer<View::System>& view_): m_view(view_)
	{
	}

	void operator()(Agent::Hub& hub_);

private:
	QSharedPointer<View::System> m_view;
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
	Subject(QSharedPointer<virConnect> , QSharedPointer<View::System> );

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
	Performance(QSharedPointer<virConnect> libvirtd_, QSharedPointer<View::System> view_):
		m_agent(libvirtd_), m_view(view_)
	{
	}

	void run();

private:
	void pull(Agent::Vm::Unit vm_);

	Agent::Vm::List m_agent;
	QSharedPointer<View::System> m_view;
};

} // namespace Tools
} // namespace Libvirt

#endif // __CDSPLIBVIRT_P_H__

