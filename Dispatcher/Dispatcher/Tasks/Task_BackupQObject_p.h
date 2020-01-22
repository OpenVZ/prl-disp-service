///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackupQObject_p.h
///
/// QObjects for Vm backup tasks
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

#ifndef __TASK_BACKUPQOBJECT_P_H__
#define __TASK_BACKUPQOBJECT_P_H__

#include <QObject>
#include <QProcess>
#include "CDspTaskHelper.h"
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>

class CDspClient;

namespace Backup
{
using IOService::IOPackage;

namespace Tunnel
{
namespace Source
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject
{
public slots:
	virtual void reactReceive(const SmartPtr<IOPackage>& package_) = 0;

	virtual void reactAccept() = 0;

	virtual void reactDisconnect() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Agent

struct Agent: QObject
{
	Q_INVOKABLE virtual void cancel() = 0;
	Q_INVOKABLE virtual qint32 addStrand(quint16 spice_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Source

namespace Target
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject
{
public slots:
	virtual void reactReceive(const SmartPtr<IOPackage>& package_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Target
} // namespace Tunnel

namespace Work
{
namespace Push
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Thaw

struct Thaw: QObject
{
public slots:
	virtual void release() = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Push
} // namespace Work

namespace Process
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Driver

struct Driver: QProcess
{
	Q_INVOKABLE virtual void write_(SmartPtr<char> data_, quint32 size_) = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit: QObject
{
protected slots:
	virtual void reactFinish(int code_, QProcess::ExitStatus status_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Process

namespace Task
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Target

struct Target: CDspTaskHelper
{
	Target(const SmartPtr<CDspClient>& client_, const SmartPtr<IOPackage>& request_):
		CDspTaskHelper(client_, request_)
	{
	}

protected slots:
	virtual void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p) = 0;
	virtual void clientDisconnected(IOSender::Handle h) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Task

namespace Device
{
namespace Event
{
namespace Deferred
{
///////////////////////////////////////////////////////////////////////////////
// struct Base
// XXX: QObject-based class could not be a template class at the same time.
//      To workaround this obstacle, we'll inherit our template class from
//      a QObject-based class with pure virtual functions.

struct Base : QObject
{
public slots:
	virtual void execute(QString uuid_, QString deviceAlias_) = 0;

private:
	Q_OBJECT
};

} // namespace Deferred
} // namespace Event
} // namespace Device
} // namespace Backup

namespace Restore
{
namespace Target
{
namespace Escort
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Gear

struct Gear: QObject
{
protected slots:
	virtual void react(const SmartPtr<IOPackage> package_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Escort
} // namespace Target

namespace Task
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Source

struct Source: CDspTaskHelper
{
	Source(const SmartPtr<CDspClient>& client_, const SmartPtr<IOPackage>& request_):
		CDspTaskHelper(client_, request_)
	{
	}

protected slots:
	virtual void mountImage(const SmartPtr<IOPackage>& package_) = 0;
	virtual void clientDisconnected(IOSender::Handle h) = 0;
	virtual void handleABackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p) = 0;
	virtual void handleVBackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p) = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Target

struct Target: CDspTaskHelper
{
	Target(const SmartPtr<CDspClient>& client_, const SmartPtr<IOPackage>& request_):
		CDspTaskHelper(client_, request_)
	{
	}

protected slots:
	virtual void runV2V() = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Task
} // namespace Restore

#endif // __TASK_BACKUPQOBJECT_P_H__

