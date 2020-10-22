///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRegistry.cpp
///
/// Collection of Vm-related information grouped into a repo.
///
/// @author alkurbatov
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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

#include <QMutex>
#include "CDspRegistry.h"
#include "CDspClient.h"
#include "CDspInstrument.h"
#include "CDspTemplateFacade.h"
#include "CDspDispConfigGuard.h"
#include "CDspVmNetworkHelper.h"
#include "CDspVmStateMachine.h"
#include "Stat/CDspStatStorage.h"
#include <boost/phoenix/operator.hpp>
#include <boost/functional/factory.hpp>
#include <boost/phoenix/core/argument.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <prlxmlmodel/VirtuozzoObjects/CXmlModelHelper.h>
#include "CDspVmGuestPersonality.h"
#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#endif // _LIBVIRT_

namespace Registry
{
namespace Device
{
///////////////////////////////////////////////////////////////////////////////
// struct State

struct State
{
	typedef void result_type;

	State(const QString& alias_, PVE::DeviceConnectedState value_,
		const CVmConfiguration& runtime_):
		m_alias(alias_), m_runtime(runtime_), m_value(value_)
	{
	}

	void operator()(CVmConfiguration& config_) const;

private:
	template<class T>
	bool update(const QList<T* >& list_) const;

	const QString m_alias;
	const CVmConfiguration m_runtime;
	const PVE::DeviceConnectedState m_value;
};

template<class T>
bool State::update(const QList<T* >& list_) const
{
	typedef typename QList<T* >::const_iterator iterator_type;

	iterator_type e = list_.end();
	iterator_type p = std::find_if(list_.begin(), e,
		boost::bind(&T::getAlias, _1) == boost::cref(m_alias));
	if (p == e)
		return false;

	(*p)->setConnected(m_value);
	return true;
}

void State::operator()(CVmConfiguration& config_) const
{
	::Vm::Config::Repairer< ::Vm::Config::revise_types>
		::type::do_(config_, m_runtime);
	CVmHardware *h = config_.getVmHardwareList();
	if (update(h->m_lstHardDisks))
		return;
	if (update(h->m_lstSerialPorts))
		return;
	if (update(h->m_lstOpticalDisks))
		return;
}

} // namespace Device

///////////////////////////////////////////////////////////////////////////////
// struct Vm declaration

struct Vm: ::Vm::State::Machine
{
	Vm(const QString& uuid_, const SmartPtr<CDspClient>& user_,
			const QSharedPointer<Network::Routing>& routing_);

	template<class T>
	void react(const T& event_)
	{
		QMutexLocker l(&m_mutex);

		if (getName().isEmpty())
		{
			WRITE_TRACE(DBG_DEBUG, "attempt to update VM state before first define, that is wrong");
			return;
		}
		process_event(event_);
	}

	void updateConfig(CVmConfiguration value_);

	QWeakPointer<Stat::Storage> getStorage()
	{
		return m_storage.toWeakRef();
	}

	const QString getDirectory() const
	{
		return getUser().getVmDirectoryUuid();
	}
	PRL_VM_TOOLS_STATE getToolsState();

private:
	QMutex m_mutex;
	QSharedPointer<Stat::Storage> m_storage;
	QSharedPointer<Network::Routing> m_routing;
};

namespace Update
{
///////////////////////////////////////////////////////////////////////////////
// struct Chain

template<class T>
struct Chain: Instrument::Chain::Lsp<T, PRL_RESULT>
{
	typedef Instrument::Chain::Lsp<T, PRL_RESULT> base_type;

	explicit Chain(const typename base_type::redo_type& redo_): base_type(redo_)
	{
	}

	typename base_type::result_type handle(typename base_type::argument_type request_)
	{
		Q_UNUSED(request_);
		T& t = static_cast<T& >(*this);
		t.execute();

		return PRL_ERR_SUCCESS;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Workbench

struct Workbench: Template::Facade::Workbench
{
	typedef Template::Facade::Workbench base_type;
	typedef CDspClient user_type;
	typedef ::Vm::State::Machine machine_type;

	Workbench(machine_type& machine_, user_type& user_, const base_type& base_):
		base_type(base_), m_user(&user_), m_machine(&machine_)
	{
	}

	user_type& getUser() const
	{
		return *m_user;
	}
	machine_type& getMachine() const
	{
		return *m_machine;
	}

private:
	user_type* m_user;
	machine_type* m_machine;
};

///////////////////////////////////////////////////////////////////////////////
// struct Adoption

struct Adoption: Chain<Adoption>, private Workbench
{
	Adoption(CVmConfiguration& orphan_, const Workbench& workbench_, const redo_type& redo_):
		Chain<Adoption>(redo_), Workbench(workbench_), m_orphan(&orphan_)
	{
	}

	bool filter(argument_type request_) const;
	void execute();

private:
	CVmConfiguration* m_orphan;
};

bool Adoption::filter(argument_type request_) const
{
	return PRL_ERR_VM_UUID_NOT_FOUND == request_ &&
		getMachine().getHome().isEmpty();
}

void Adoption::execute()
{
	WRITE_TRACE(DBG_DEBUG, "New VM registered directly from libvirt");

	QString u(m_orphan->getVmIdentification()->getVmUuid());
	QString d(QDir(getUser().getUserDefaultVmDirPath())
		.absoluteFilePath(::Vm::Config::getVmHomeDirName(u)));
	QString f(QDir(d).absoluteFilePath(VMDIR_DEFAULT_VM_CONFIG_FILE));
	getMachine().setHome(f);
	m_orphan->getVmIdentification()->setHomePath(f);

	WRITE_TRACE(DBG_DEBUG, "update VM directory item");

	Template::Facade::Registrar r(getUser().getVmDirectoryUuid(), *m_orphan, *this);
	PRL_RESULT e = r.begin();
	if (PRL_SUCCEEDED(e))
	{
		if (PRL_FAILED(e = r.execute()))
			r.rollback();
		else
			r.commit();
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Compulsion

struct Compulsion: Chain<Compulsion>, private Workbench
{
	typedef boost::signals2::signal<void (CVmConfiguration& )> script_type;

	Compulsion(script_type& script_, const Workbench& workbench_, const redo_type& redo_):
		Chain<Compulsion>(redo_), Workbench(workbench_), m_script(&script_)
	{
	}

	bool filter(argument_type request_) const;
	void execute();

private:
	script_type* m_script;
};

bool Compulsion::filter(argument_type request_) const
{
	return PRL_ERR_VM_UUID_NOT_FOUND == request_ &&
		!getMachine().getHome().isEmpty();
}

void Compulsion::execute()
{
	QString h(getMachine().getHome());
	SmartPtr<CDspClient> a(&getUser(), SmartPtrPolicy::DoNotReleasePointee);
	SmartPtr<CVmConfiguration> b(new CVmConfiguration());
	PRL_RESULT e = getConfigManager().loadConfig(b, h, a, true, false);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_DEBUG, "cannot load VM config from %s: %s",
			qPrintable(h), PRL_RESULT_TO_STRING(e));
	}
	else
	{
		(*m_script)(*b);
		getConfigManager().saveConfig(b, h, a, true, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Complement

struct Complement: Instrument::Command::Base
{
	Complement(CVmConfiguration& config_, Workbench::machine_type& machine_):
		m_config(&config_), m_machine(&machine_)
	{
	}

	result_type operator()(PRL_RESULT request_);

private:
	CVmConfiguration* m_config;
	Workbench::machine_type* m_machine;
};

Complement::result_type Complement::operator()(PRL_RESULT request_)
{
	if (PRL_FAILED(request_))
		return request_;

	QString a(m_machine->getHome());
	QString b(m_config->getVmIdentification()->getHomePath());
	if (!b.isEmpty() && a != b)
		m_machine->setHome(b);

	return PRL_ERR_SUCCESS;
}

} // namespace Update

///////////////////////////////////////////////////////////////////////////////
// struct Vm definition

Vm::Vm(const QString& uuid_, const SmartPtr<CDspClient>& user_,
		const QSharedPointer<Network::Routing>& routing_):
	::Vm::State::Machine(uuid_, user_, routing_),
	m_storage(new Stat::Storage(uuid_)), m_routing(routing_)
{
#ifdef __USE_ISOCXX11
	typedef ::Vm::State::Started::factory_type factory_type;

	factory_type f = [this](QWeakPointer<QAtomicInt> incarnation_) -> factory_type::result_type {
		return new ::Vm::Guest::Connector(this->getDirectory(), *this, incarnation_);
	};
	::Vm::State::Machine::Running r(boost::msm::back::states_ <<
		::Vm::State::Started(getConfigEditor(), f));
#else // __USE_ISOCXX11
	::Vm::State::Machine::Running r(boost::msm::back::states_ <<
		::Vm::State::Started(getConfigEditor(),
			boost::bind(boost::factory< ::Vm::Guest::Connector* >(),
				getDirectory(), boost::ref(*this), _1)),
					boost::ref(*this));
#endif // __USE_ISOCXX11

	set_states(boost::msm::back::states_ << r);

	// Current balloon value (in kB).
	m_storage->addAbsolute("mem.guest_total");

	// The amount of memory used by the system (in kB).
	m_storage->addAbsolute("mem.guest_used");

	// Sum of cpu times of all vcpu used by VE (in msec)
	m_storage->addAbsolute("cpu_time");
}

void Vm::updateConfig(CVmConfiguration value_)
{
	QMutexLocker l(&m_mutex);

	setName(value_.getVmIdentification()->getVmName());
	boost::signals2::signal<void (CVmConfiguration& )> s;
	if (value_.getVmIdentification()->getHomePath().isEmpty())
	{
		// NB. libvirt config doesn't contain VM home. for other sources
		// there is no need to merge.
		s.connect(boost::bind(&::Vm::Config::Repairer< ::Vm::Config::untranslatable_types>
					::type::do_, boost::ref(value_), _1));
	}
	if (is_flag_active< ::Vm::State::Running>())
	{
		s.connect(boost::bind(&Network::Routing::reconfigure,
			m_routing.data(), _1, boost::cref(value_)));
	}
	s.connect(boost::phoenix::placeholders::arg1 = boost::phoenix::cref(value_));
	Update::Workbench w(*this, getUser(), Update::Workbench::base_type(getService()));
	Update::Adoption(value_, w,
		Update::Compulsion(s, w,
			Update::Complement(value_, *this)))(getConfigEditor()(s));
}

PRL_VM_TOOLS_STATE Vm::getToolsState()
{
	::Vm::State::Machine::Running& r = get_state< ::Vm::State::Machine::Running& >();
	::Vm::State::Started& s = r.get_state< ::Vm::State::Started& >();
	if (s.getTools())
		return s.getTools().get();

	boost::optional<CVmConfiguration> c = getConfig();
	if (!c)
		return PTS_NOT_INSTALLED;
	QString v = c->getVmSettings()->getVmTools()->getAgentVersion();
	return v.isEmpty() ? PTS_NOT_INSTALLED : PTS_POSSIBLY_INSTALLED;
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
// struct Reactor

void Reactor::prepareToSwitch()
{
	forward(::Vm::State::Switch());
}

void Reactor::reboot()
{
	forward(::Vm::State::Reboot());
}

void Reactor::upgrade()
{
	forward(::Vm::State::Upgrade());
}

void Reactor::connectAgent()
{
	forward(::Vm::State::Agent());
}

void Reactor::updateAgent(const boost::optional<PRL_VM_TOOLS_STATE>& state_)
{
	forward(boost::phoenix::val(state_));
}

void Reactor::proceed(VIRTUAL_MACHINE_STATE destination_)
{
	switch(destination_)
	{
	case VMS_RUNNING:
		return forward(::Vm::State::Conventional<VMS_RUNNING>());
	case VMS_STOPPING:
		return forward(::Vm::State::Conventional<VMS_STOPPING>());
	case VMS_STOPPED:
		return forward(::Vm::State::Conventional<VMS_STOPPED>());
	case VMS_PAUSED:
		return forward(::Vm::State::Conventional<VMS_PAUSED>());
	case VMS_MOUNTED:
		return forward(::Vm::State::Conventional<VMS_MOUNTED>());
	case VMS_SUSPENDED:
		return forward(::Vm::State::Conventional<VMS_SUSPENDED>());
	case VMS_DELETING_STATE:
		return forward(::Vm::State::Conventional<VMS_DELETING_STATE>());
	default:
		return forward(::Vm::State::Conventional<VMS_UNKNOWN>());
	}
}

template<class T>
void Reactor::forward(const T& event_)
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (!x.isNull())
		x->react(event_);
}

void Reactor::updateConnected(const QString& device_, PVE::DeviceConnectedState value_,
	const CVmConfiguration& runtime_)
{
	return forward(::Vm::Configuration::update_type
		(boost::bind(Device::State(device_, value_, runtime_), _1)));
}

///////////////////////////////////////////////////////////////////////////////
// struct Access

boost::optional<CVmConfiguration> Access::getConfig() const
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return boost::none;

	return x->getConfig();
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

PRL_VM_TOOLS_STATE Access::getToolsState()
{
	QSharedPointer<Vm> x = m_vm.toStrongRef();
	if (x.isNull())
		return PTS_NOT_INSTALLED;
	return x->getToolsState();
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
	l.unlock();
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
		if (!i.second->getCtId().isEmpty() || i.second->isTemplate() ||
			i.first == CDspVmDirManager::getTemplatesDirectoryUuid())
			continue;

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
