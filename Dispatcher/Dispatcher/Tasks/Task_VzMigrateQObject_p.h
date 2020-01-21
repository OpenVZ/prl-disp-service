///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzMigrateQObject_p.h
///
/// QObjects for Vz migration
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

#ifndef __TASK_VZMIGRATEQOBJECT_P_H__
#define __TASK_VZMIGRATEQOBJECT_P_H__

#include <QObject>
#include <QString>
#include <QThread>
#include <prlsdk/PrlTypes.h>

namespace Migrate
{
namespace Ct
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Pump

struct Pump: QObject
{
public slots:
	virtual void reactBytesWritten(qint64 value_) = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub: QObject
{
public slots:
	virtual void reactBytesWritten(qint64 value_) = 0;

signals:
	void vacant();

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Task

struct Task: QThread
{
public slots:
	virtual void spin() = 0;
	virtual void read() = 0;

signals:
	void onDispPackageHandlerFailed(PRL_RESULT nRetCode, const QString &sErrInfo);

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Ct
} // namespace Migrate

#endif // __TASK_VZMIGRATEQOBJECT_P_H__

