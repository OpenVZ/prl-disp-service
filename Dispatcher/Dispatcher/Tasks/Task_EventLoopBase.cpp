///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EventLoopBase.cpp
///
/// Task for quick implementation of the task with its own event loop.
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

#include <QMetaObject>
#include <prlcommon/Std/PrlAssert.h>
#include "Task_EventLoopBase.h"


Task_EventLoopBase::Task_EventLoopBase(
    const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &pRequestPkg)
    : CDspTaskHelper(pUser, pRequestPkg)
{
    moveToThread(this);
}

PRL_RESULT Task_EventLoopBase::run_body()
{
    bool invoked = true;
    invoked &= QMetaObject::invokeMethod(this, "onFirstEvent", Qt::QueuedConnection);
    PRL_ASSERT(invoked);

    QThread::exec();
    return getLastErrorCode();
}

void Task_EventLoopBase::cancelOperation(
    SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg)
{
    bool invoked = true;
    invoked &= QMetaObject::invokeMethod(this, "taskCancel", Qt::QueuedConnection);
    PRL_ASSERT(invoked);

    CDspTaskHelper::cancelOperation(user, pkg);
}

void Task_EventLoopBase::exitRunLoop()
{
    QThread::exit();
}
