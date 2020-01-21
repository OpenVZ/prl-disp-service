///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmManagerQObject_p.h
///
/// QObjects for Vm Manager
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

#ifndef __CDSPVMMANAGERQOBJECT_P_H__
#define __CDSPVMMANAGERQOBJECT_P_H__

#include <QObject>
#include <QString>
#include <prlcommon/Std/SmartPtr.h>
#include <prlxmlmodel/DispConfig/CDispCommonPreferences.h>

namespace Command
{
namespace Vm
{
namespace Fork
{
///////////////////////////////////////////////////////////////////////////////
// struct Reactor

struct Reactor: QObject
{
public slots:
	virtual void react() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Detector

struct Detector: Reactor
{
	void react()
	{
		emit detected();
	}

signals:
	void detected();

private:
	Q_OBJECT
};

namespace State
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Detector

struct Detector: Fork::Detector
{
public slots:
	virtual void react(unsigned oldState_, unsigned newState_, QString vmUuid_, QString dirUuid_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace State

namespace Config
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// struct Detector

struct Detector: Fork::Detector
{
public slots:
	virtual void react(QString, QString uid_) = 0;

private:
	Q_OBJECT
};

} // namespace Abstract
} // namespace Config

namespace Timeout
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

struct Handler: Fork::Reactor
{
signals:
	void finish();

private:
	Q_OBJECT
};

} // namespace Timeout
} // namespace Fork
} // namespace Vm
} // namespace Command

namespace Preference
{
namespace Applying
{
///////////////////////////////////////////////////////////////////////////////
// struct Reactor

struct Reactor: QObject
{
public slots:
	virtual void react(const SmartPtr<CDispCommonPreferences> old_,
		const SmartPtr<CDispCommonPreferences> new_) = 0;

private:
	Q_OBJECT
};

} // namespace Applying
} // namespace Preference

#endif // __CDSPVMMANAGERQOBJECT_P_H__

