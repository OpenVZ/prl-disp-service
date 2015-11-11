///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmManager_p.h
///
/// Public interfaces of the libvirt interaction.
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

#ifndef __CDSPVMMANAGER_P_H__
#define __CDSPVMMANAGER_P_H__

#include "CDspLibvirt.h"

namespace Command
{
namespace Vm
{
namespace Shutdown
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

struct Handler: QObject
{
	Handler(const QString& uuid_, QEventLoop& loop_): m_uuid(uuid_), m_loop(&loop_)
	{
	}

public slots:
	void react(unsigned oldState_, unsigned newState_, QString vmUuid_, QString dirUuid_);

protected:
	void timerEvent(QTimerEvent* event_);

private:
	Q_OBJECT

	QString m_uuid;
	QEventLoop* m_loop;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit: QObject
{
	Unit(const QString& uuid_, quint32 timeout_):
		m_uuid(uuid_), m_timeout(timeout_)
	{
	}

	Libvirt::Result operator()();

protected:
	void timerEvent(QTimerEvent* event_);

private:
	Q_OBJECT

	QEventLoop m_loop;
	const QString m_uuid;
	const quint32 m_timeout;
};

} // namespace Shutdown
} // namespace Vm
} // namespace Command

#endif // __CDSPVMMANAGER_P_H__

