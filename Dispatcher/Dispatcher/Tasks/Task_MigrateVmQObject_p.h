///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmQObject_p.h
//
/// QT signals and slots employed by migration
///
/// @author shrike@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_MIGRATEVMQOBJECT_P_H__
#define __TASK_MIGRATEVMQOBJECT_P_H__

#include <QObject>
#include "CDspLibvirt.h"
#include "CDspService.h"
#include "boost/function.hpp"
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include <prlcommon/IOService/IOCommunication/IOSendJob.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include <prlcommon/IOService/IOCommunication/IOClientInterface.h>
#include <prlcommon/IOService/IOCommunication/IOServerInterface.h>

class CDspDispConnection;
namespace Migrate
{
namespace Vm
{
using IOService::IOClient;
using IOService::IOPackage;
using IOService::IOSendJob;
using IOService::IOClientInterface;
using IOService::IOServerInterface;

namespace Flop
{
struct Event;

} // namespace Flop

namespace Pipeline
{
///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
public slots:
	virtual void timeout() = 0;

private:
	Q_OBJECT
};

} // namespace Pipeline

namespace Pump
{
///////////////////////////////////////////////////////////////////////////////
// struct IO

struct IO: QObject
{
	virtual IOSendJob::Handle sendPackage(const SmartPtr<IOPackage>&) = 0;
	virtual IOSendJob::Result getSendResult(const IOSendJob::Handle& job_) = 0;

signals:
	void onReceived(const SmartPtr<IOPackage>& package_);
	void onSent(const SmartPtr<IOPackage>& package_);
	void disconnected();

private:
	Q_OBJECT
};

namespace Push
{
///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
public slots:
	virtual void onSent(const SmartPtr<IOPackage>&) = 0;
	virtual void readyRead() = 0;
	virtual void readChannelFinished() = 0;

private:
	Q_OBJECT
};

} // namespace Push

namespace Pull
{
///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
public slots:
	virtual void reactBytesWritten(qint64 value_) = 0;

private:
	Q_OBJECT
};

} // namespace Pull
} // namespace Pump

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
protected slots:
	virtual void reactFinished(int, QProcess::ExitStatus) = 0;

private:
	Q_OBJECT
};

} // namespace Libvirt

namespace Source
{
namespace Shadow
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector_

struct Connector_: QObject
{
public slots:
	virtual void reactSuccess() = 0;
	virtual void reactFailure(const Flop::Event&) = 0;

private:
	Q_OBJECT
};

struct Connector;

///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
signals:
        void failed(const Flop::Event&);
        void completed();

public slots:
        virtual void reactFinish() = 0;

private:
	Q_OBJECT
};

} // namespace Shadow

namespace Libvirt
{
namespace Trick
{
namespace Online
{
///////////////////////////////////////////////////////////////////////////////
// struct Carrier

struct Carrier: QObject
{
	typedef Shadow::Connector* target_type;

	Carrier(target_type target_, QObject* parent_):
		QObject(parent_), m_target(target_)
	{
	}

	Q_INVOKABLE void haul(quint64 downtime_);

private:
	Q_OBJECT

	target_type m_target;
};

} // namespace Online
} // namespace Trick

///////////////////////////////////////////////////////////////////////////////
// struct Progress

struct Progress: QObject
{
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Agent agent_type;
	typedef boost::function1<void, int> reporter_type;
	
	Progress(const agent_type& agent_, const reporter_type& reporter_):
		m_last(~0), m_agent(agent_), m_reporter(reporter_)
	{
	}

	void report(quint16 value_);

protected:
	void timerEvent(QTimerEvent* event_);

private:
	Q_OBJECT

	quint16 m_last;
	agent_type m_agent;
	reporter_type m_reporter;
};

} // namespace Libvirt

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct IO

struct IO: Vm::Pump::IO
{
public:
	explicit IO(IOClient &io);
	~IO();

	IOSendJob::Handle sendPackage(const SmartPtr<IOPackage>& package_);
	IOSendJob::Result getSendResult(const IOSendJob::Handle& job_);

private slots:
	void reactReceived(const SmartPtr<IOPackage>& package);

	void reactSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage>);

	void reactChange(IOSender::State value_);

private:
	Q_OBJECT

	IOClient *m_io;
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector_

struct Connector_: QObject
{
	Connector_(): m_service()
	{
	}

	void setService(IO* value_)
	{
		m_service = value_;
	}

public slots:
	virtual void acceptLibvirt() = 0;

	virtual void acceptQemuDisk() = 0;

	virtual void acceptQemuState() = 0;

protected:
	IO* getService() const
	{
		return m_service;
	}

private:
	Q_OBJECT

	IO* m_service;
};

} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector_

struct Connector_: QObject
{
public slots:
	virtual void cancel() = 0;

	virtual void react(const SmartPtr<IOPackage>& package_) = 0;

private:
	Q_OBJECT
};

} // namespace Source

namespace Target
{
namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector_

struct Connector_: QObject
{
public slots:
	virtual void reactState(unsigned, unsigned, QString, QString) = 0;

private:
	Q_OBJECT
};

} // namespace Libvirt

namespace Commit
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector_

struct Connector_: QObject
{
public slots:
	virtual void reactFinished() = 0;

private:
	Q_OBJECT
};

} // namespace Commit

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct IO

struct IO: Vm::Pump::IO
{
	explicit IO(CDspDispConnection& io_);
	~IO();

	IOSendJob::Handle sendPackage(const SmartPtr<IOPackage>& package_);
	IOSendJob::Result getSendResult(const IOSendJob::Handle& job_);

private slots:
	void reactReceived(IOSender::Handle handle_, const SmartPtr<IOPackage>& package_);

	void reactDisconnected(IOSender::Handle handle);

	void reactSend(IOServerInterface*, IOSender::Handle handle_,
		    IOSendJob::Result, const SmartPtr<IOPackage>);

private:
	Q_OBJECT

	CDspDispConnection *m_io;
};

namespace Connector
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic_

struct Basic_: QObject
{
	Basic_(): m_service()
	{
	}

	void setService(IO* value_)
	{
		m_service = value_;
	}

public slots:
	virtual void reactConnected() = 0;

	virtual void reactDisconnected() = 0;

protected:
	IO* getService() const
	{
		return m_service;
	}

private:
	Q_OBJECT

	IO* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Tcp_

struct Tcp_: Basic_
{
public slots:
	virtual void reactError(QAbstractSocket::SocketError) = 0;

private:
	Q_OBJECT
};

} // namespace Connector
} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector_

struct Connector_: QObject
{
public slots:
	virtual void cancel() = 0;

	virtual void disconnected() = 0;

	virtual void react(const SmartPtr<IOPackage>& package_) = 0;

private:
	Q_OBJECT
};

} // namespace Target
} // namespace Vm
} // namespace Migrate

#endif // __TASK_MIGRATEVMQOBJECT_P_H__

