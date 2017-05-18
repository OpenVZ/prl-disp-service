///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRegistry.h
///
/// Collection of Vm-related information grouped into a repo.
///
/// @author alkurbatov
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

#ifndef __CDSPREGISTRY_H__
#define __CDSPREGISTRY_H__

#include <QHash>
#include <QString>
#include <CVmIdent.h>
#include <QSharedPointer>
#include <QReadWriteLock>
#include <boost/mpl/at.hpp>
#include <prlsdk/PrlEnums.h>
#include "CDspVmConfigManager.h"
#include <prlcommon/Messaging/CVmEvent.h>
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
// struct Reactor

struct Reactor
{
	explicit Reactor(const QWeakPointer<Vm>& vm_): m_vm(vm_)
	{
	}

	void prepareToSwitch();

	void reboot();

	void connectAgent();

	void disconnectAgent();

	void proceed(VIRTUAL_MACHINE_STATE destination_);

	void updateNetwork(const QString& network_);

	void updateConnected(const QString& device_, PVE::DeviceConnectedState value_);

private:
	template<class T>
	void forward(const T& event_);

	QWeakPointer<Vm> m_vm;
};

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

	void updateConfig(const CVmConfiguration& value_);

	QWeakPointer<Stat::Storage> getStorage();

	boost::optional< ::Vm::Config::Edit::Atomic> getConfigEditor() const;

	PRL_VM_TOOLS_STATE getToolsState();

	Reactor getReactor() const
	{
		return Reactor(m_vm);
	}

private:
	QString m_uuid;
	QWeakPointer<Vm> m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Public

struct Public
{
	typedef QSharedPointer<Vm> bin_type;
	typedef QPair<CVmIdent, QString> declaration_type;
	typedef boost::variant<declaration_type, bin_type> booking_type;

	Access find(const QString& uuid_);

	PRL_RESULT declare(const CVmIdent& ident_, const QString& home_);

	PRL_RESULT undeclare(const QString& uuid_);

protected:
	typedef QHash<QString, bin_type> vmMap_type;

	QReadWriteLock m_rwLock;
	vmMap_type m_definedMap;
	vmMap_type m_undeclaredMap;
	QHash<QString, booking_type> m_bookingMap;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor: boost::static_visitor<Prl::Expected<Public::bin_type, Error::Simple> >
{
	explicit Visitor(CDspService& service_);

	result_type operator()
		(const boost::mpl::at_c<Public::booking_type::types, 0>::type& variant_) const;

	result_type operator()
		(const boost::mpl::at_c<Public::booking_type::types, 1>::type& variant_) const
	{
		return variant_;
	}
	result_type operator()(const CVmIdent& ident_) const;

private:
	CDspService* m_service;
	QSharedPointer<Network::Routing> m_routing;
};

///////////////////////////////////////////////////////////////////////////////
// struct Actual

struct Actual: Public
{
	explicit Actual(CDspService& service_):
		m_conductor(service_), m_service(&service_)
	{
	}

	Prl::Expected<Access, Error::Simple> define(const QString& uuid_);

	void undefine(const QString& uuid_);

	void reset();

	QStringList snapshot();
	QString getServerUuid() const;

private:
	Visitor m_conductor;
	CDspService* m_service;
};

} // namespace Registry

#endif // __CDSPREGISTRY_H__

