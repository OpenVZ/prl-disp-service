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
#include <CVmIdent.h>
#include <QSharedPointer>
#include <QReadWriteLock>
#include <prlsdk/PrlEnums.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>

class CDspClient;
class CDspService;

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
	Access(const QString& uuid_, QWeakPointer<Vm> vm_):
		m_uuid(uuid_), m_vm(vm_)
	{
	}

	const QString& getUuid() const
	{
		return m_uuid;
	}

	boost::optional<CVmConfiguration> getConfig();

	void prepareToSwitch();

	void updateState(VIRTUAL_MACHINE_STATE value_);

	void updateConfig(const CVmConfiguration& value_);

	QWeakPointer<Stat::Storage> getStorage();

private:
	QString m_uuid;
	QWeakPointer<Vm> m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Public

struct Public
{
	Access find(const QString& uuid_);

	PRL_RESULT declare(const CVmIdent& ident_, const QString& home_);

	PRL_RESULT undeclare(const QString& uuid_);

protected:
	typedef QPair<CVmIdent, QString> booking_type;
	typedef QHash<QString, QSharedPointer<Vm> > vmMap_type;

	QReadWriteLock m_rwLock;
	vmMap_type m_definedMap;
	vmMap_type m_undeclaredMap;
	QHash<QString, booking_type> m_bookingMap;
};

///////////////////////////////////////////////////////////////////////////////
// struct Actual

struct Actual: Public
{
	explicit Actual(CDspService& service_);

	Prl::Expected<Access, Error::Simple> define(const QString& uuid_);

	void undefine(const QString& uuid_);

	void reset();

	QStringList snapshot();

private:
	Prl::Expected<QSharedPointer<Vm>, Error::Simple>
		craft(const QString& uuid_, const QString& directory_);

	CDspService* m_service;
	QSharedPointer<Network::Routing> m_routing;
};

} // namespace Registry

#endif // __CDSPREGISTRY_H__

