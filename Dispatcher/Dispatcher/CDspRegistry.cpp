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

	void setStatsPeriod(quint64 seconds_);

	QWeakPointer<Stat::Storage> getStorage()
	{
		return m_storage.toWeakRef();
	}

private:
	QMutex m_mutex;
	QSharedPointer<Stat::Storage> m_storage;
	QSharedPointer<Network::Routing> m_routing;
};

Vm::Vm(const QString& uuid_, const SmartPtr<CDspClient>& user_,
		const QSharedPointer<Network::Routing>& routing_):
	::Vm::State::Machine(uuid_, user_, routing_),
	m_storage(new Stat::Storage(uuid_)), m_routing(routing_)
{
	// Metadata
	// PERF_COUNT_TYPE_ABS - represents some absolute value,
	//                       like current CPU usage
	// PERF_COUNT_TYPE_INC - some value that constantly grows,
	//                       like I/O bytes count

	// Current balloon value (in kB).
	m_storage->addAbsolute("mem.guest_total");

	// The amount of memory used by the system (in kB).
	m_storage->addAbsolute("mem.guest_used");
}

void Vm::prepareToSwitch()
{
	QMutexLocker l(&m_mutex);

	process_event(::Vm::State::Switch());
}

void Vm::updateState(VIRTUAL_MACHINE_STATE value_)
{
	QMutexLocker l(&m_mutex);

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
#ifdef _LIBVIRT_
	Libvirt::Kit.vms().at(getUuid()).completeConfig(value_);
#endif // _LIBVIRT_

	boost::optional<CVmConfiguration> y = getConfig();
	if (y)
	{
		::Vm::Config::Repairer< ::Vm::Config::untranslatable_types>
			::type::do_(value_, y.get());
		
		if (is_flag_active< ::Vm::State::Running>())
			m_routing->reconfigure(y.get(), value_);
	}
	else
		WRITE_TRACE(DBG_DEBUG, "New VM registered directly from libvirt");

	if (value_.getVmIdentification()->getHomePath().isEmpty())
		value_.getVmIdentification()->setHomePath(getHome());
	else
		setHome(value_.getVmIdentification()->getHomePath());

	updateDirectory(value_.getVmType());

	::Vm::State::Machine::setConfig(value_);
}

void Vm::setStatsPeriod(quint64 seconds_)
{
#ifdef _LIBVIRT_
	qint64 p = qMax(seconds_, quint64(1));
	Libvirt::Kit.vms().at(getUuid()).getPerformance().setMemoryStatsPeriod(p);
#endif // _LIBVIRT_
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

void Access::updateConfig(CVmConfiguration& value_)
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

///////////////////////////////////////////////////////////////////////////////
// struct Public

Access Public::find(const QString& uuid_)
{
	QReadLocker l(&m_rwLock);

	vmMap_type::const_iterator p = m_vmMap.find(uuid_);
	if (m_vmMap.end() == p)
		return Access(QWeakPointer<Vm>());

	return Access(p.value().toWeakRef());
}

///////////////////////////////////////////////////////////////////////////////
// struct Actual

Actual::Actual(): Public(), m_routing(new Network::Routing())
{
}

Prl::Expected<Access, Error::Simple> Actual::add(const QString& uuid_)
{
	QWriteLocker l(&m_rwLock);

	CDspDispConfigGuard* c = &CDspService::instance()->getDispConfigGuard();

	SmartPtr<CDspClient> u;
	QString d = c->getDispWorkSpacePrefs()->getDefaultVmDirectory();
	foreach (CDispUser* s, c->getDispUserPreferences()->m_lstDispUsers)
	{
		if (d == s->getUserWorkspace()->getVmDirectory())
		{
			u = CDspClient::makeServiceUser(d);
			u->setUserSettings(s->getUserId(), s->getUserName());
			break;
		}
	}

	if (!u.isValid())
		return Error::Simple(PRL_ERR_FAILURE);

	QSharedPointer<Vm> v(new Vm(uuid_, u, m_routing));
	m_vmMap[uuid_] = QSharedPointer<Vm>(v);

	v->setStatsPeriod(c->getDispWorkSpacePrefs()->getVmGuestCollectPeriod());

	return Access(v.toWeakRef());
}

void Actual::remove(const QString& uuid_)
{
	QWriteLocker l(&m_rwLock);

	vmMap_type::iterator p = m_vmMap.find(uuid_);
	if (m_vmMap.end() == p)
		return;

	m_vmMap.erase(p);
}

} // namespace Registry
