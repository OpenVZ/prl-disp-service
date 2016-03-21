///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRegistry.h
///
/// Collection of Vm-related information grouped into a repo.
///
/// @author alkurbatov
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

#ifndef __CDSPREGISTRY_H__
#define __CDSPREGISTRY_H__

#include <QHash>
#include <QString>
#include <QSharedPointer>
#include <QReadWriteLock>
#include <prlsdk/PrlEnums.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>

class CDspClient;

namespace Stat
{
struct Storage;
} // namespace Stat

namespace Network
{
struct Routing;
} // namespace Network

namespace Registry
{
struct Vm;
struct Actual;

///////////////////////////////////////////////////////////////////////////////
// struct Access

struct Access
{
	explicit Access(QWeakPointer<Vm> vm_): m_vm(vm_)
	{
	}

	void prepareToSwitch();

	void updateState(VIRTUAL_MACHINE_STATE value_);

	void updateConfig(CVmConfiguration& value_);

	QWeakPointer<Stat::Storage> getStorage();

private:
	QWeakPointer<Vm> m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Public

struct Public
{
	Access find(const QString& uuid_);

protected:
	typedef QHash<QString, QSharedPointer<Vm> > vmMap_type;

	QReadWriteLock m_rwLock;
	vmMap_type m_vmMap;
};

///////////////////////////////////////////////////////////////////////////////
// struct Actual

struct Actual: Public
{
	Actual();

	Prl::Expected<Access, Error::Simple> add(const QString& uuid_);

	void remove(const QString& uuid_);

private:
	QSharedPointer<Network::Routing> m_routing;
};

} // namespace Registry

#endif // __CDSPREGISTRY_H__

