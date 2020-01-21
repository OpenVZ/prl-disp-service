///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuest_p.h
///
/// Class for VM activity running inside guest agent
///
/// @author shrike
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPVMGUEST_P_H__
#define __CDSPVMGUEST_P_H__

#include <QObject>
#include <QString>

namespace Vm
{
namespace State
{
struct Frontend;

} // namespace State

namespace Guest
{
namespace Abstract
{
///////////////////////////////////////////////////////////////////////////////
// class Actor

class Actor: public QObject
{
	Q_OBJECT

public slots:
	virtual void setToolsVersionSlot(const QString v_) = 0;
	virtual void configureNetworkSlot(const QString v_) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// class Watcher

class Watcher: public QObject
{
	Q_OBJECT

public:
	Q_INVOKABLE virtual void respin() = 0;

signals:
	void guestToolsStarted(const QString v_);
	
};

} // namespace Abstract
} // namespace Guest
} // namespace Vm

#endif // __CDSPVMGUEST_P_H__
