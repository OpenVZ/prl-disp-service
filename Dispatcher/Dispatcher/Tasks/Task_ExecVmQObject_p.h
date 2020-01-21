///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ExecVmQObject_p.h
///
/// QObjects for Vm exec
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

#ifndef __TASK_EXECVMQOBJECT_P_H__
#define __TASK_EXECVMQOBJECT_P_H__

#include "CDspTaskHelper.h"
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>

namespace Exec
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Join - exits loop when all added objects finished their job

struct Join: QObject
{
public slots:
	virtual void slotFinished() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Mediator - mediator between devices and Task_ExecVm. Forwards data to client.

struct Mediator: QObject
{
signals:
	void finished();

public slots:
	virtual void slotSendData() = 0;
	virtual void slotEof() = 0;

private:
	Q_OBJECT
};

namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Auxiliary

struct Auxiliary: CDspTaskHelper
{
	Auxiliary(const SmartPtr<CDspClient>& client_, const SmartPtr<IOService::IOPackage>& request_):
		CDspTaskHelper(client_, request_)
	{
	}

public slots:
	virtual void slotProcessStdin(const SmartPtr<IOService::IOPackage>& p) = 0;
	virtual void slotProcessFin() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Main

struct Main: CDspTaskHelper
{
	Main(const SmartPtr<CDspClient>& client_, const SmartPtr<IOService::IOPackage>& request_):
		CDspTaskHelper(client_, request_)
	{
	}

protected slots:
	virtual void reactDisconnected(IOSender::Handle handle) = 0;

private:
	Q_OBJECT
};

} // namespace Task
} // namespace Abstract
} // namespace Exec

#endif // __TASK_EXECVMQOBJECT_P_H__

