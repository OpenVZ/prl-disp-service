///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirtQObject_p.h
///
/// QObjects for libvirt
///
/// @author shrike@
///
/// Copyright (c) 2020 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPLIBVIRTQOBJECT_P_H__
#define __CDSPLIBVIRTQOBJECT_P_H__

#include <QTimer>
#include <QObject>
#include <QEventLoop>
#include <QSharedPointer>
#include <prlsdk/PrlTypes.h>
#include <boost/function.hpp>

struct _virConnect;
typedef struct _virConnect virConnect;
typedef virConnect *virConnectPtr;

typedef void (*virEventHandleCallback)(int watch, int fd, int events, void *opaque);
typedef void (*virEventTimeoutCallback)(int timer, void *opaque);

namespace Libvirt
{
namespace Callback
{
namespace Transport
{
///////////////////////////////////////////////////////////////////////////////
// struct Visitor forward declaration

struct Visitor;

} // namespace Transport

namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Timeout

struct Timeout: QObject
{
public slots:
	virtual void handle() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Socket

struct Socket: QObject
{
public slots:
	virtual void read(int socket_) = 0;
	virtual void error(int socket_) = 0;
	virtual void write(int socket_) = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub: QObject
{
	Q_INVOKABLE virtual void add(int id_, virEventTimeoutCallback callback_) = 0;
	Q_INVOKABLE virtual void add(int id_, int socket_, virEventHandleCallback callback_) = 0;
	Q_INVOKABLE virtual void remove(int id_) = 0;
	Q_INVOKABLE virtual void setEvents(int id_, int value_) = 0;
	Q_INVOKABLE virtual void setInterval(int id_, int value_) = 0;
	Q_INVOKABLE virtual void setOpaque(int id_, Transport::Visitor* value_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Callback

namespace Monitor
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Link

struct Link: QObject
{
public slots:
	virtual void setOpen() = 0;
	virtual void setClosed() = 0;

signals:
	void connected(QSharedPointer<virConnect>);
	void disconnected();

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Domains

struct Domains: QObject
{
public slots:
	virtual void setConnected(QSharedPointer<virConnect>) = 0;
	virtual void setDisconnected() = 0;

private:
	Q_OBJECT
};

} // namespace Abstract

namespace Performance
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Broker

struct Broker: QObject
{
public slots:
	virtual void despatch() = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Performance

namespace Hardware
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit: QObject
{
public slots:
	virtual void react() = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Hardware
} // namespace Monitor

namespace Instrument
{
namespace Agent
{
namespace Vm
{
namespace Block
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Completion

struct Completion: QObject
{
signals:
	void done();

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Counter

struct Counter: QObject
{
	typedef boost::function<PRL_RESULT ()> product_type;

protected:
	Q_INVOKABLE virtual product_type read_() = 0;
	Q_INVOKABLE virtual void account_(QString one_, PRL_RESULT status_) = 0;
	Q_INVOKABLE virtual void reset_() = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Block

namespace Exec
{
///////////////////////////////////////////////////////////////////////////////
// struct Waiter

struct Waiter : QObject {
private slots:
	void stop()
	{
		m_loop.quit();
	}

public:
	void wait(int msecs)
	{
		QTimer::singleShot(msecs, this, SLOT(stop()));
		m_loop.exec();
	}

private:
	Q_OBJECT
	QEventLoop m_loop;
};


} // namespace Exec
} // namespace Vm
} // namespace Agent
} // namespace Instrument
} // namespace Libvirt

#endif // __CDSPLIBVIRTQOBJECT_P_H__

