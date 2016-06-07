///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRegistry.cpp
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

#include <QMutex>
#include "CDspRegistry.h"
#include "CDspClient.h"
#include "CDspDispConfigGuard.h"
#include "CDspVmNetworkHelper.h"
#include "CDspVmStateMachine.h"
#include "Stat/CDspStatStorage.h"
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/core/argument.hpp>
#include <boost/phoenix/core/reference.hpp>
#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#endif // _LIBVIRT_

namespace Registry
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: ::Vm::State::Machine
{
	Vm(const QString& uuid_, const SmartPtr<CDspClient>& user_,
			const QSharedPointer<Network::Routing>& routing_);

	void prepareToSwitch();

	void updateState(VIRTUAL_MACHINE_STATE value_);

	void updateConfig(CVmConfiguration value_);

	QWeakPointer<Stat::Storage> getStorage()
	{
		return m_storage.toWeakRef();
	}

	const QString getDirectory() const
	{
		return getUser().getVmDirectoryUuid();
	}

private:
	void updateDirectory(PRL_VM_TYPE type_);

	QMutex m_mutex;
	QSharedPointer<Stat::Storage> m_storage;
	QSharedPointer<Network::Routing> m_routing;
};

Vm::Vm(const QString& uuid_, const SmartPtr<CDspClient>& user_,
		const QSharedPointer<Network::Routing>& routing_):
	::Vm::State::Machine(uuid_, user_, routing_),
	m_storage(new Stat::Storage(uuid_)), m_routing(routing_)
{
	// Current balloon value (in kB).
	m_storage->addAbsolute("mem.guest_total");

	// The amount of memory used by the system (in kB).
	m_storage->addAbsolute("mem.guest_used");

	// Sum of cpu times of all vcpu used by VE (in msec)
	m_storage->addAbsolute("cpu_time");
}

void Vm::prepareToSwitch()
{
	QMutexLocker l(&m_mutex);

	process_event(::Vm::State::Switch());
}

void Vm::updateState(VIRTUAL_MACHINE_STATE value_)
{
	QMutexLocker l(&m_mutex);

	if (getName().isEmpty())
	{
		WRITE_TRACE(DBG_DEBUG, "tries to update VM state before first define, that is wrong");
		return;
	}

	switch(value_)
	{
	case VMS_RUNNING:
		process_event(::Vm::State::Event<VMS_RUNNING>());
		break;
	case VMS_STOPPED:
		process_event(::Vm::State::Event<VMS_STOPPED>());
		break;
	case VMS_PAUSED:
		process_event(::Vm::State::Event<VMS_PAUSED>());
		break;
	case VMS_MOUNTED:
		process_event(::Vm::State::Event<VMS_MOUNTED>());
		break;
	case VMS_SUSPENDED:
		process_event(::Vm::State::Event<VMS_SUSPENDED>());
		break;
	default:
		process_event(::Vm::State::Event<VMS_UNKNOWN>());
		return;
	}
}

void Vm::updateConfig(CVmConfiguration value_)
{
	QMutexLocker l(&m_mutex);

	setName(value_.getVmIdentification()->getVmName());
	boost::signals2::signal<void (CVmConfiguration& )> s;
	s.connect(boost::bind(&::Vm::Config::Repairer< ::Vm::Config::untranslatable_types>
				::type::do_, boost::ref(value_), _1));
	if (is_flag_active< ::Vm::State::Running>())
	{
		s.connect(boost::bind(&Network::Routing::reconfigure,
			m_routing.data(), _1, boost::cref(value_)));
	}
	s.connect(boost::phoenix::placeholders::arg1 = boost::phoenix::cref(value_));
	if (PRL_ERR_VM_UUID_NOT_FOUND == getConfigEditor()(s))
	{
		WRITE_TRACE(DBG_DEBUG, "New VM registered directly from libvirt");
		QString h = QDir(getUser().getUserDefaultVmDirPath())
			.absoluteFilePath(::Vm::Config::getVmHomeDirName(getUuid()));
		setHome(QDir(h).absoluteFilePath(VMDIR_DEFAULT_VM_CONFIG_FILE));
		WRITE_TRACE(DBG_DEBUG, "update VM directory item");
		updateDirectory(value_.getVmType());
		value_.getVmIdentification()->setHomePath(getHome());

		SmartPtr<CDspClient> a(&getUser(), SmartPtrPolicy::DoNotReleasePointee);
		SmartPtr<CVmConfiguration> b(&value_, SmartPtrPolicy::DoNotReleasePointee);
		getService().getVmConfigManager().saveConfig(b, getHome(), a, true, true);
	}
	else if (!value_.getVmIdentification()->getHomePath().isEmpty())
		setHome(value_.getVmIdentification()->getHomePath());
}

void Vm::updateDirectory(PRL_VM_TYPE type_)
{
	typedef CVmDirectory::TemporaryCatalogueItem item_type;

	CDspVmDirManager& m = getService().getVmDirManager();
	QScopedPointer<item_type> t(new item_type(getUuid(), getHome(), getName()));
	PRL_RESULT e = m.checkAndLockNotExistsExclusiveVmParameters
				(QStringList(), t.data());
	if (PRL_FAILED(e))
		return;

	QScopedPointer<CVmDirectoryItem> x(new CVmDirectoryItem());
	x->setVmUuid(getUuid());
	x->setVmName(getName());
	x->setVmHome(getHome());
	x->setVmType(type_);
	x->setValid(PVE::VmValid);
	x->setRegistered(PVE::VmRegistered);
	e = getService().getVmDirHelper().insertVmDirectoryItem(getDirectory(), x.data());
	if (PRL_SUCCEEDED(e))
		x.take();

	m.unlockExclusiveVmParameters(t.data());
}

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

Visitor::Visitor(CDspService& service_): m_service(&service_),
	m_routing(new Network::Routing())
{
}

Visitor::result_type Visitor::operator()(const CVmIdent& ident_) const
{
	SmartPtr<CDspClient> u;
	CDspDispConfigGuard& c = m_service->getDispConfigGuard();
	foreach (CDispUser* s, c.getDispUserPreferences()->m_lstDispUsers)
	{
		if (ident_.second == s->getUserWorkspace()->getVmDirectory())
		{
			u = CDspClient::makeServiceUser(ident_.second);
			u->setUserSettings(s->getUserId(), s->getUserName());
			break;
		}
	}
	if (!u.isValid())
		return Error::Simple(PRL_ERR_FAILURE);

	return Public::bin_type(new Vm(ident_.first, u, m_routing));
}

Visitor::result_type Visitor::operator()
	(const boost::mpl::at_c<Public::booking_type::types, 0>::type& variant_) const
{
	result_type output = (*this)(variant_.first);
	if (output.isSucceed())
		output.value()->setHome(variant_.second);

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Access

boost::optional<CVmConfiguration> Access::getConfig()
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return boost::none;

	return x->getConfig();
}

void Access::prepareToSwitch()
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return;

	x->prepareToSwitch();
}

void Access::updateState(VIRTUAL_MACHINE_STATE value_)
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return;

	x->updateState(value_);
}

void Access::updateConfig(const CVmConfiguration& value_)
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return;

	x->updateConfig(value_);
}

QWeakPointer<Stat::Storage> Access::getStorage()
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return QWeakPointer<Stat::Storage>();

	return x->getStorage();
}

boost::optional< ::Vm::Config::Edit::Atomic> Access::getConfigEditor() const
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return boost::none;

	return x->getConfigEditor();
}

///////////////////////////////////////////////////////////////////////////////
// struct Public

Access Public::find(const QString& uuid_)
{
	QReadLocker g(&m_rwLock);

	vmMap_type::const_iterator p = m_definedMap.find(uuid_);
	if (m_definedMap.end() == p)
		return Access(uuid_, QWeakPointer<Vm>());

	return Access(uuid_, p.value().toWeakRef());
}

PRL_RESULT Public::declare(const CVmIdent& ident_, const QString& home_)
{
	const QString& u = ident_.first;
	QWriteLocker g(&m_rwLock);
	if (m_definedMap.contains(u))
	{
		// defined implies declared
		return PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID;
	}
	if (m_undeclaredMap.contains(u))
	{
		m_definedMap.insert(u, m_undeclaredMap.take(u));
		return PRL_ERR_SUCCESS;
	}
	if (m_bookingMap.contains(u))
		return PRL_ERR_DOUBLE_INIT;

	m_bookingMap.insert(u, declaration_type(ident_, home_));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Public::undeclare(const QString& uuid_)
{
	QWriteLocker g(&m_rwLock);
	vmMap_type::mapped_type m = m_definedMap.take(uuid_);
	if (m.isNull())
	{
		// what should we do if a booking is pending for the uuid?
		// should we drop it as well?
		// definetly should not move to undeclared if yes
		if (0 < m_bookingMap.remove(uuid_))
			return PRL_ERR_SUCCESS;

		return PRL_ERR_FILE_NOT_FOUND;
	}
	// NB. there are no pending bookings here
	m_undeclaredMap.insert(uuid_, m);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Actual

Prl::Expected<Access, Error::Simple> Actual::define(const QString& uuid_)
{
	QWriteLocker l(&m_rwLock);
	if (m_definedMap.contains(uuid_) || m_undeclaredMap.contains(uuid_))
		return Error::Simple(PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID);

	Visitor::result_type m;
	if (m_bookingMap.contains(uuid_))
	{
		booking_type b = m_bookingMap.take(uuid_);
		m = boost::apply_visitor(m_conductor, b);
	}
	else
	{
		CDspDispConfigGuard& c = m_service->getDispConfigGuard();
		m = m_conductor(MakeVmIdent(uuid_, c.getDispWorkSpacePrefs()->getDefaultVmDirectory()));
	}
	if (m.isFailed())
		return m.error();

	m_definedMap[uuid_] = m.value();
	return Access(uuid_, m.value().toWeakRef());
}

void Actual::undefine(const QString& uuid_)
{
	QWriteLocker l(&m_rwLock);
	QSharedPointer<Vm> m = m_definedMap.take(uuid_);
	if (m.isNull())
	{
		m_undeclaredMap.remove(uuid_);
		return;
	}
	PRL_RESULT e = m_service->getVmDirHelper()
		.deleteVmDirectoryItem(m->getDirectory(), uuid_);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_DEBUG, "Unregistering of a VM from the directory failed: %s",
			PRL_RESULT_TO_STRING(e));
	}
}

void Actual::reset()
{
	vmMap_type b;
	{
		QWriteLocker g(&m_rwLock);
		std::swap(b, m_definedMap);
// XXX. do we need to drop VM that are undeclared???
//		m_undeclaredMap.clear();
	}
	::Vm::Directory::Dao::Locked d(m_service->getVmDirManager());
	foreach (const ::Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		QString u = i.second->getVmUuid();
		vmMap_type::mapped_type m = b[u];
		if (m.isNull())
			declare(MakeVmIdent(i.second->getVmUuid(), i.first), i.second->getVmHome());
		else
		{
			m->setHome(i.second->getVmHome());
			QWriteLocker g(&m_rwLock);
			m_bookingMap.insert(u, m);
		}
	}
}

QStringList Actual::snapshot()
{
	QReadLocker g(&m_rwLock);
	return QStringList() << m_definedMap.keys() << m_undeclaredMap.keys();
}

QString Actual::getServerUuid() const
{
	return m_service->getDispConfigGuard().getServerUuid();
}

} // namespace Registry
