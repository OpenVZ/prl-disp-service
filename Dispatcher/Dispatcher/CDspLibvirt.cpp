/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QHostAddress>
#include "CDspService.h"
#include "CDspLibvirt_p.h"
#include "CDspVmManager_p.h"
#include <QtCore/qmetatype.h>
#include "CDspVmStateSender.h"
#include "CDspVmNetworkHelper.h"
#include "CDspBackupDevice.h"
#include "Stat/CDspStatStorage.h"
#include <boost/algorithm/string.hpp>
#include "Tasks/Task_CreateProblemReport.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_EditVm.h"
#include <prlcommon/PrlUuid/PrlUuid.h>
#include <prlcommon/Std/PrlTime.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include <Libraries/StatesUtils/StatesHelper.h>
#include <Libraries/Transponster/Direct.h>
#include <Libraries/Transponster/Reverse.h>
#include <Libraries/Transponster/Reverse_p.h>

namespace Libvirt
{
namespace Callback
{
static Access g_access;

} // namespace Callback

namespace Instrument
{
namespace Pull
{
///////////////////////////////////////////////////////////////////////////////
// struct Network

void Network::run()
{
	if (m_network.isEmpty())
		return;

	QList<Agent::Vm::Unit> a;
	Libvirt::Kit.vms().all(a);
	foreach (Agent::Vm::Unit m, a)
	{
		VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
		if (m.getState().getValue(s).isFailed())
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get VM state");
			continue;
		}

		if (s != VMS_RUNNING && s != VMS_PAUSED)
			continue;

		QString u;
		if (m.getUuid(u).isFailed())
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get VM uuid");
			continue;
		}

		QSharedPointer<Model::System::entry_type> d = m_fine->find(u);
		if (d.isNull())
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get domain model");
			continue;
		}
		d->show(Callback::Reactor::Network(m_network, m));
	}
}

} // namespace Pull

namespace Agent
{
namespace Parameters
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

Builder::Builder() : m_pointer(NULL), m_size(0), m_capacity(0)
{
}

Builder::~Builder()
{
	virTypedParamsFree(m_pointer, m_size);
}

bool Builder::add(const char *key_, const QString& value_)
{
	return virTypedParamsAddString(&m_pointer, &m_size, &m_capacity, key_, QSTR2UTF8(value_)) == 0;
}

bool Builder::add(const char *key_, quint64 value_)
{
	return virTypedParamsAddULLong(&m_pointer, &m_size, &m_capacity, key_, value_) == 0;
}

bool Builder::add(const char *key_, qint64 value_)
{
	return virTypedParamsAddLLong(&m_pointer, &m_size, &m_capacity, key_, value_) == 0;
}

bool Builder::add(const char *key_, qint32 value_)
{
	return virTypedParamsAddInt(&m_pointer, &m_size, &m_capacity, key_, value_) == 0;
}

Result_type Builder::extract()
{
	Result_type r(QSharedPointer<virTypedParameter>(m_pointer, boost::bind(virTypedParamsFree, _1, m_size)),
		m_size);
	m_pointer = NULL;
	m_capacity = 0;
	m_size = 0;
	return r;
}

} // namespace Parameters

namespace Vm
{
namespace Completion
{
///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

Hotplug::Hotplug(virDomainPtr match_, const QString& device_,
	const feedback_type& feedback_): m_feedback(feedback_),
	m_match(Model::Coarse::getUuid(match_), device_)
{
}

void Hotplug::operator()(Registry::Reactor&)
{
	m_feedback->set_value();
}

void Hotplug::operator()(Model::Coarse* model_, domain_type domain_)
{
	model_->show(domain_.data(), *this);
}

int Hotplug::react(virConnectPtr, virDomainPtr domain_,
	const char* device_, void* opaque_)
{
	Hotplug* x = reinterpret_cast<Hotplug* >(opaque_);
	if (NULL == x)
		return -1;

	QPair<QString, QString> m(Model::Coarse::getUuid(domain_), QString());
	if (!x->m_match.second.isEmpty())
		m.second = device_;

	if (m == x->m_match)
	{
		virDomainRef(domain_);
		Callback::g_access.setOpaque(Callback::Mock::ID,
			new Callback::Transport::Event(boost::bind(*x, _1,
				domain_type(domain_, &virDomainFree))));
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// struct Migration

Migration::Migration(virDomainPtr match_, const feedback_type& feedback_):
	m_match(Model::Coarse::getUuid(match_)), m_feedback(feedback_)
{
}

int Migration::operator()(virTypedParameterPtr params_, int paramsCount_)
{
	quint64 v = 0;
	int output = virTypedParamsGetULLong(params_, paramsCount_,
			VIR_DOMAIN_JOB_DOWNTIME, &v);
	if (0 > output)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot extract migration downtime value");
		if (!m_feedback.isNull())
			m_feedback->set_value(~0);
	}
	else if (m_feedback.isNull())
		WRITE_TRACE(DBG_DEBUG, "Total migration downtime %-12llu ms", v);
	else
		m_feedback->set_value(v);
	
	return output;
}

int Migration::react(virConnectPtr, virDomainPtr domain_,
	virTypedParameterPtr params_, int paramsCount_, void *opaque_)
{
	Migration* x = reinterpret_cast<Migration* >(opaque_);
	if (NULL == x)
		return -1;

	int output = 0;
	if (Model::Coarse::getUuid(domain_) == x->m_match)
		output = (*x)(params_, paramsCount_);

	return output;
}

} // namespace Completion

namespace Migration
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic

Result Basic::addXml(const char* parameter_, Config agent_)
{
	Prl::Expected<QString, Error::Simple> x = agent_.fixup(*m_config);
	if (x.isFailed())
	{
		WRITE_TRACE(DBG_DEBUG, "VM configuration fixup failed");
		return x.error();
	}

	WRITE_TRACE(DBG_DEBUG, "migration target xml:\n%s", x.value().toUtf8().data());

	if (m_builder->add(parameter_, x.value()))
		return Result();

	return Failure(PRL_ERR_FAILURE);
}

Result Basic::addName()
{
	const QString n = m_config->getVmIdentification()->getVmName();
	if (m_builder->add(VIR_MIGRATE_PARAM_DEST_NAME, n))
		return Result();

	return Failure(PRL_ERR_FAILURE);
}

///////////////////////////////////////////////////////////////////////////////
// struct Compression

Result Compression::operator()(Parameters::Builder& builder_)
{
	if (!builder_.add(VIR_MIGRATE_PARAM_COMPRESSION, "xbzrle"))
		return Failure(PRL_ERR_FAILURE);
	if (!builder_.add(VIR_MIGRATE_PARAM_COMPRESSION, "mt"))
		return Failure(PRL_ERR_FAILURE);

	return Result();
}

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth

Result Bandwidth::operator()(Parameters::Builder& builder_)
{
	// Libvirt uses MiB/s value for bandwidth
	WRITE_TRACE(DBG_DEBUG, "Migration bandwidth set to '%llu' MiB/s", m_value / 1024 / 1024);
	if (!builder_.add(VIR_MIGRATE_PARAM_BANDWIDTH, m_value / 1024 / 1024))
		return Failure(PRL_ERR_FAILURE);

	return Result();
}

namespace Qemu
{
///////////////////////////////////////////////////////////////////////////////
// struct Disk

Result Disk::operator()(Parameters::Builder& builder_)
{
	foreach (CVmHardDisk* d, m_list)
	{
		QString t = Transponster::Device::Clustered::Model
			<CVmHardDisk>(*d).getTargetName();
		if (!builder_.add(VIR_MIGRATE_PARAM_MIGRATE_DISKS, t))
			return Failure(PRL_ERR_FAILURE);
	}
	if (m_port)
	{
		if (!builder_.add(VIR_MIGRATE_PARAM_DISKS_PORT, m_port.get()))
			return Failure(PRL_ERR_FAILURE);
		if (!builder_.add(VIR_MIGRATE_PARAM_LISTEN_ADDRESS,
			QHostAddress(QHostAddress::LocalHost).toString()))
			return Failure(PRL_ERR_FAILURE);
	}

	return Result();
}

///////////////////////////////////////////////////////////////////////////////
// struct State

Result State::operator()(Parameters::Builder& builder_)
{
	if (!builder_.add(VIR_MIGRATE_PARAM_URI,
		QString("tcp://%1:%2")
		.arg(QHostAddress(QHostAddress::LocalHost).toString())
		.arg(m_port)))
		return Failure(PRL_ERR_FAILURE);

	return Result();
}

} // namespace Qemu
} // namespace Migration
} // namespace Vm

///////////////////////////////////////////////////////////////////////////////
// struct Config

Config::Config(QSharedPointer<virDomain> domain_,
		QSharedPointer<virConnect> link_,
		unsigned int flags_) :
	m_domain(domain_), m_link(link_), m_flags(flags_)
{
}

char* Config::read_() const
{
	return virDomainGetXMLDesc(m_domain.data(), m_flags | VIR_DOMAIN_XML_SECURE);
}

Prl::Expected<QString, Error::Simple> Config::read() const
{
	char* x = read_();
	if (NULL == x)
		return Error::Simple(PRL_ERR_VM_GET_CONFIG_FAILED);

	QString s = x;
	free(x);
	return s;
}

Result Config::convert(CVmConfiguration& dst_) const
{
	Prl::Expected<VtInfo, Error::Simple> i = Host(m_link).getVt();
	if (i.isFailed())
		return i.error();

	char* x = read_();
	if (NULL == x)
		return Error::Simple(PRL_ERR_VM_GET_CONFIG_FAILED);

//	WRITE_TRACE(DBG_FATAL, "xml:\n%s", x);
	Transponster::Vm::Direct::Vm u(x);
	if (PRL_FAILED(Transponster::Director::domain(u, i.value())))
		return Error::Simple(PRL_ERR_PARSE_VM_CONFIG);

	CVmConfiguration* output = u.getResult();
	if (NULL == output)
		return Error::Simple(PRL_ERR_FAILURE);

	output->getVmIdentification()
		->setServerUuid(CDspService::instance()
                        ->getDispConfigGuard().getDispConfig()
                        ->getVmServerIdentification()->getServerUuid());
	dst_ = *output;
	delete output;
	return Result();
}

Prl::Expected<QString, Error::Simple> Config::mixup(const CVmConfiguration& value_) const
{
	Prl::Expected<VtInfo, Error::Simple> i = Host(m_link).getVt();
	if (i.isFailed())
		return i.error();

	Transponster::Vm::Reverse::Mixer u(value_, read_());
	PRL_RESULT res = Transponster::Director::domain(u, i.value());
	if (PRL_FAILED(res))
		return Error::Simple(res);

	return u.getResult();
}

Prl::Expected<QString, Error::Simple> Config::fixup(const CVmConfiguration& value_) const
{
	Prl::Expected<VtInfo, Error::Simple> i = Host(m_link).getVt();
	if (i.isFailed())
		return i.error();

	Transponster::Vm::Reverse::Fixer u(value_, read_());
	PRL_RESULT res = Transponster::Director::domain(u, i.value());
	if (PRL_FAILED(res))
		return Error::Simple(res);

	return u.getResult();
}

Prl::Expected<QString, Error::Simple> Config::adjustClock(qint64 offset_) const
{
	Transponster::Vm::Reverse::Pipeline u(read_());
	PRL_RESULT res = u(Transponster::Vm::Reverse::Clock(offset_));
	if (PRL_FAILED(res))
		return Error::Simple(res);

	return u.getResult();
}

} // namespace Agent

namespace Breeding
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

void Vm::operator()(Agent::Hub& hub_)
{
	QStringList s = m_registry->snapshot();
	Agent::Vm::List l = hub_.vms();
	foreach (const QString& u, s)
	{
		if (!validate(u))
		{
			// unregister VM from libvirt
			l.at(u).getState().undefine();
			// at this point VM is not registered in m_view->m_domainMap
			// thus we need to manually remove the corresponding
			// directory item from Dispatcher
			m_registry->undefine(u);
			s.removeOne(u);
		}
	}

	m_registry->reset();

	QList<Agent::Vm::Unit> a;
	l.all(a);
	foreach (Agent::Vm::Unit m, a)
	{
		QString u;
		m.getUuid(u);
		QSharedPointer<Model::System::entry_type> v = m_view->add(u);
		if (!v.isNull())
		{
			s.removeOne(u);
			Registry::Access r = m_registry->find(u);
			Callback::Reactor::Domain(m).update(r);	
		}
	}
	foreach (const QString& u, s)
	{
		// in order to properly remove the directory item, first we need to
		// add the VM to the registry and then remove it from there
		m_registry->define(u);
		m_registry->undefine(u);
	}
}

bool Vm::validate(const QString& uuid_)
{
	Registry::Access a = m_registry->find(uuid_);
	boost::optional<CVmConfiguration> c = a.getConfig();
	if (!c)
		return true;
	CVmIdentification *y = c->getVmIdentification();
	if (!y)
		return true;
	// Assume this node is registered in the cluster and had just crashed.
	// shaman had relocated a VM from this node and registered it on another
	// node in the cluster. After that, this node had been rebooted and is
	// back online.
	// Here is what we have on Dispatcher start:
	// - The VM is registered both in libvirt and Dispatcher.
	// - Server UUID of the VM differs from our server UUID.
	// Need to unregister the VM from this node.
	return y->getServerUuid() == m_registry->getServerUuid();
}

///////////////////////////////////////////////////////////////////////////////
// struct Network

Network::Network(const QFileInfo& config_):
	m_digested(config_.absoluteDir(), QString("digested.").append(config_.fileName()))
{
}

void Network::operator()(Agent::Hub& hub_)
{
	if (m_digested.exists())
		return;

	CParallelsNetworkConfig f;
	PRL_RESULT e = PrlNet::ReadNetworkConfig(f);
	if (PRL_ERR_FILE_NOT_FOUND == e)
		PrlNet::FillDefaultConfiguration(&f);
	else if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "cannot read the networks config: %s",
			PRL_RESULT_TO_STRING(e));
		return;
	}
	QTemporaryFile t(m_digested.absoluteFilePath());
	if (!t.open())
	{
		WRITE_TRACE(DBG_FATAL, "cannot create a temporary file");
		return;
	}
	if (PRL_FAILED(e = f.saveToFile(&t)))
	{
		WRITE_TRACE(DBG_FATAL, "cannot save the xml model into a temporary file: %s",
			PRL_RESULT_TO_STRING(e));
		return;
	}
	CVirtualNetworks* s = f.getVirtualNetworks();
	if (NULL != s)
	{
		foreach (CVirtualNetwork* k, s->m_lstVirtualNetwork)
		{
			if (NULL != k && k->isEnabled())
				::Network::Dao(hub_).create(*k);
		}
	}
	if (!QFile::rename(t.fileName(), m_digested.absoluteFilePath()))
	{
		WRITE_TRACE(DBG_FATAL, "cannot rename %s -> %s",
			QSTR2UTF8(t.fileName()),
			QSTR2UTF8(m_digested.absoluteFilePath()));
		return;
	}
	t.setAutoRemove(false);
}

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(QSharedPointer<virConnect> libvirtd_, QSharedPointer<Model::System> view_,
	Registry::Actual& registry_):
	m_vm(view_, registry_), m_network(ParallelsDirs::getNetworkConfigFilePath())
{
	m_hub.setLink(libvirtd_);
}

void Subject::run()
{
	m_vm(m_hub);
	m_network(m_hub);
	QProcess::startDetached("/usr/libexec/vz_systemd");
}

} // namespace Breeding
} // namespace Instrument

namespace Callback
{
namespace Transport
{
///////////////////////////////////////////////////////////////////////////////
// struct Visitor

Visitor::~Visitor()
{
}

void Visitor::complain(const char* name_)
{
	WRITE_TRACE(DBG_FATAL, "%s is a bad target for the transport", name_);
}

} // namespace Transport

///////////////////////////////////////////////////////////////////////////////
// struct Timeout

Timeout::Timeout(virEventTimeoutCallback impl_, int id_): Base(id_), m_impl(impl_)
{
	this->connect(&m_timer, SIGNAL(timeout()), SLOT(handle()));
}

Timeout::~Timeout()
{
	m_timer.disconnect(this);
}

void Timeout::enable(int interval_)
{
	m_timer.stop();
	if (0 <= interval_)
		m_timer.start(1000 * interval_);
}

void Timeout::handle()
{
	m_impl(getId(), getOpaque());
}

///////////////////////////////////////////////////////////////////////////////
// struct Socket

Socket::Socket(int socket_, virEventHandleCallback impl_, int id_):
	Base(id_), m_impl(impl_), m_read(socket_, QSocketNotifier::Read),
	m_write(socket_, QSocketNotifier::Write),
	m_error(socket_, QSocketNotifier::Exception)
{
	m_read.setEnabled(false);
	this->connect(&m_read, SIGNAL(activated(int)), SLOT(read(int)));
	m_error.setEnabled(false);
	this->connect(&m_error, SIGNAL(activated(int)), SLOT(error(int)));
	m_write.setEnabled(false);
	this->connect(&m_write, SIGNAL(activated(int)), SLOT(write(int)));
}

Socket::~Socket()
{
	m_read.disconnect(this);
	m_error.disconnect(this);
	m_write.disconnect(this);
}

void Socket::enable(int events_)
{
	m_read.setEnabled(VIR_EVENT_HANDLE_READABLE & events_);
	m_error.setEnabled(true);
	m_write.setEnabled(VIR_EVENT_HANDLE_WRITABLE & events_);
}

void Socket::disable()
{
	m_read.setEnabled(false);
	m_error.setEnabled(false);
	m_write.setEnabled(false);
}

void Socket::read(int socket_)
{
	m_impl(getId(), socket_, VIR_EVENT_HANDLE_READABLE, getOpaque());
}

void Socket::error(int socket_)
{
	m_impl(getId(), socket_, VIR_EVENT_HANDLE_ERROR, getOpaque());
}

void Socket::write(int socket_)
{
	m_impl(getId(), socket_, VIR_EVENT_HANDLE_WRITABLE, getOpaque());
}

///////////////////////////////////////////////////////////////////////////////
// struct Mock

void Mock::fire(const eventHandler_type& event_)
{
	if (m_model.isNull())
	{
		WRITE_TRACE(DBG_FATAL, "event without a model");
		event_(NULL);
	}
	else
	{
		Model::Coarse m(m_model);
		event_(&m);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Hub

void Hub::add(int id_, virEventTimeoutCallback callback_)
{
	m_timeoutMap.insert(id_, new Timeout(callback_, id_));
}

void Hub::add(int id_, int socket_, virEventHandleCallback callback_)
{
	m_socketMap.insert(id_, new Socket(socket_, callback_, id_));
}

void Hub::setOpaque(int id_, Transport::Visitor* value_)
{
	QScopedPointer<Transport::Visitor> g(value_);
	if (g.isNull())
		return;

	if (Mock::ID == id_)
		return g->visit(m_mock);

	boost::ptr_map<int, Socket>::iterator a = m_socketMap.find(id_);
	if (m_socketMap.end() != a)
		return g->visit(*a->second);

	boost::ptr_map<int, Timeout>::iterator b = m_timeoutMap.find(id_);
	if (m_timeoutMap.end() != b)
		return g->visit(*b->second);
}

void Hub::setEvents(int id_, int value_)
{
	boost::ptr_map<int, Socket>::iterator p = m_socketMap.find(id_);
	if (m_socketMap.end() != p)
		return p->second->enable(value_);
}

void Hub::setInterval(int id_, int value_)
{
	boost::ptr_map<int, Timeout>::iterator p = m_timeoutMap.find(id_);
	if (m_timeoutMap.end() != p)
		return p->second->enable(value_);
}

void Hub::remove(int id_)
{
	boost::ptr_map<int, Sweeper>::iterator p = m_sweeperMap.find(id_);
	if (m_sweeperMap.end() != p)
	{
		m_sweeperMap.release(p).release()->deleteLater();
		return;
	}
	Sweeper* s = new Sweeper(id_);
	s->startTimer(0);
	boost::ptr_map<int, Socket>::iterator x = m_socketMap.find(id_);
	if (m_socketMap.end() != x)
	{
		boost::ptr_map<int, Socket>::auto_type p = m_socketMap.release(x);
		p->disable();
		s->care(p.release());
	}
	boost::ptr_map<int, Timeout>::iterator y = m_timeoutMap.find(id_);
	if (m_timeoutMap.end() != y)
	{
		boost::ptr_map<int, Timeout>::auto_type p = m_timeoutMap.release(y);
		p->disable();
		s->care(p.release());
	}
	m_sweeperMap.insert(id_, s);
}

///////////////////////////////////////////////////////////////////////////////
// struct Access

void Access::setHub(const QSharedPointer<Hub>& hub_)
{
	qRegisterMetaType<virFreeCallback>("virFreeCallback");
	qRegisterMetaType<Transport::Visitor* >("Transport::Visitor* ");
	qRegisterMetaType<virEventHandleCallback>("virEventHandleCallback");
	qRegisterMetaType<virEventTimeoutCallback>("virEventTimeoutCallback");
	m_generator = QAtomicInt(Mock::ID + 1);
	m_hub = hub_;
}

int Access::add(int interval_, virEventTimeoutCallback callback_, void* opaque_, virFreeCallback free_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return -1;

	int output = m_generator.fetchAndAddOrdered(1);
	QMetaObject::invokeMethod(h.data(), "add", Q_ARG(int, output),
		Q_ARG(virEventTimeoutCallback, callback_));
	setOpaque(output, new Transport::Opaque(opaque_, free_));
	QMetaObject::invokeMethod(h.data(), "setInterval", Q_ARG(int, output), Q_ARG(int, interval_));
	return output;
}

int Access::add(int socket_, int events_, virEventHandleCallback callback_, void* opaque_, virFreeCallback free_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return -1;

	int output = m_generator.fetchAndAddOrdered(1);
	QMetaObject::invokeMethod(h.data(), "add", Q_ARG(int, output), Q_ARG(int, socket_),
		Q_ARG(virEventHandleCallback, callback_));
	setOpaque(output, new Transport::Opaque(opaque_, free_));
	QMetaObject::invokeMethod(h.data(), "setEvents", Q_ARG(int, output), Q_ARG(int, events_));
	return output;
}

void Access::setEvents(int id_, int value_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return;

	QMetaObject::invokeMethod(h.data(), "setEvents", Q_ARG(int, id_), Q_ARG(int, value_));
}

void Access::setInterval(int id_, int value_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return;

	QMetaObject::invokeMethod(h.data(), "setInterval", Q_ARG(int, id_), Q_ARG(int, value_));
}

void Access::setOpaque(int id_, Transport::Visitor* value_)
{
	QScopedPointer<Transport::Visitor> g(value_);
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return;

	QMetaObject::invokeMethod(h.data(), "setOpaque", Q_ARG(int, id_),
		Q_ARG(Transport::Visitor*, g.take()));
}

int Access::remove(int id_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return -1;

	QMetaObject::invokeMethod(h.data(), "remove", Q_ARG(int, id_));
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// struct Sweeper

void Sweeper::timerEvent(QTimerEvent* event_)
{
	if (NULL != event_)
		killTimer(event_->timerId());

	m_pet1.reset();
	m_pet2.reset();
	QSharedPointer<Hub> h = g_access.getHub();
	if (!h.isNull())
		h->remove(m_id);	
}

///////////////////////////////////////////////////////////////////////////////
// struct State

int State::react(virConnectPtr, virDomainPtr domain_, int event_,
	int subtype_, void* opaque_)
{
	QSharedPointer<Model::System::entry_type> d;
	Model::Coarse* v = (Model::Coarse* )opaque_;
	switch (event_)
	{
	case VIR_DOMAIN_EVENT_DEFINED:
		if (subtype_ == VIR_DOMAIN_EVENT_DEFINED_FROM_SNAPSHOT)
		{
			v->prepareToSwitch(domain_);
			return 0;
		}
		break;
	case VIR_DOMAIN_EVENT_UNDEFINED:
		if (VIR_DOMAIN_EVENT_UNDEFINED_REMOVED == subtype_)
			v->remove(domain_);

		return 0;
	case VIR_DOMAIN_EVENT_STARTED:
		switch (subtype_)
		{
		case -1:
			v->show(domain_, boost::bind(&Registry::Reactor::upgrade, _1));
			break;
		default:
			v->setState(domain_, VMS_RUNNING);
		case VIR_DOMAIN_EVENT_STARTED_FROM_SNAPSHOT:
			// state updated on defined from snapshot
		case VIR_DOMAIN_EVENT_STARTED_MIGRATED:
			// This event means that live migration is started, but VM has
			// not been defined yet. Ignore it.
			break;
		}
		return 0;
	case VIR_DOMAIN_EVENT_RESUMED:
		if (subtype_ == VIR_DOMAIN_EVENT_RESUMED_FROM_SNAPSHOT)
			// state updated on defined from snapshot
			return 0;

		v->setState(domain_, VMS_RUNNING);
		return 0;
	case VIR_DOMAIN_EVENT_SUSPENDED:
		if (subtype_ == VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT)
			// state updated on defined from snapshot
			return 0;

		switch (subtype_)
		{
		case VIR_DOMAIN_EVENT_SUSPENDED_PAUSED:
		case VIR_DOMAIN_EVENT_SUSPENDED_MIGRATED:
		case VIR_DOMAIN_EVENT_SUSPENDED_IOERROR:
		case VIR_DOMAIN_EVENT_SUSPENDED_WATCHDOG:
		case VIR_DOMAIN_EVENT_SUSPENDED_RESTORED:
		case VIR_DOMAIN_EVENT_SUSPENDED_API_ERROR:
			v->setState(domain_, VMS_PAUSED);
			break;
		}
		return 0;
	case VIR_DOMAIN_EVENT_PMSUSPENDED:
		switch (subtype_)
		{
		case VIR_DOMAIN_EVENT_PMSUSPENDED_MEMORY:
			v->setState(domain_, VMS_PAUSED);
			break;
		case VIR_DOMAIN_EVENT_PMSUSPENDED_DISK:
			v->setState(domain_, VMS_SUSPENDED);
			break;
		}
		return 0;
	case VIR_DOMAIN_EVENT_STOPPED:
		if (subtype_ == VIR_DOMAIN_EVENT_STOPPED_FROM_SNAPSHOT)
			// state updated on defined from snapshot
			return 0;

		switch (subtype_)
		{
		case VIR_DOMAIN_EVENT_STOPPED_SAVED:
			v->setState(domain_, VMS_SUSPENDED);
			break;
		default:
			v->setState(domain_, VMS_STOPPED);
			break;
		}
		return 0;
	case VIR_DOMAIN_EVENT_SHUTDOWN:
		v->setState(domain_, VMS_STOPPING);
		return 0;
	case VIR_DOMAIN_EVENT_CRASHED:
		switch (subtype_)
		{
		case VIR_DOMAIN_EVENT_CRASHED_PANICKED:
			WRITE_TRACE(DBG_FATAL, "VM \"%s\" got guest panic.", virDomainGetName(domain_));
			v->onCrash(domain_);
			break;
		default:
			v->setState(domain_, VMS_STOPPED);
			break;
		}
		return 0;

	default:
		return 0;
	}

	// update vm configuration and state
	v->pullInfo(domain_);
	return 0;
}

namespace Plain
{
int addSocket(int socket_, int events_, virEventHandleCallback callback_,
                void* opaque_, virFreeCallback free_)
{
	int output = g_access.add(socket_, events_, callback_, opaque_, free_);
	WRITE_TRACE(DBG_FATAL, "add socket %d %d", socket_, output);
	return output;
//	return g_access.add(socket_, events_, callback_, opaque_, free_);
}

void setSocket(int id_, int events_)
{
	return g_access.setEvents(id_, events_);
}

int addTimeout(int interval_, virEventTimeoutCallback callback_,
                void* opaque_, virFreeCallback free_)
{
	return g_access.add(interval_, callback_, opaque_, free_);
}

void setTimeout(int id_, int interval_)
{
	return g_access.setInterval(id_, interval_);
}

int remove(int id_)
{
	return g_access.remove(id_);
}

int wakeUp(virConnectPtr , virDomainPtr domain_, int , void* opaque_)
{
	Model::Coarse* v = (Model::Coarse* )opaque_;
	v->setState(domain_, VMS_RUNNING);
	return 0;
}

int reboot(virConnectPtr , virDomainPtr domain_, void* opaque_)
{
	Model::Coarse* v = (Model::Coarse* )opaque_;
	v->show(domain_, boost::bind(&Registry::Reactor::reboot, _1));
	return 0;
}

int connectAgent(virConnectPtr , virDomainPtr domain_, int state_, int /*reason_*/, void* opaque_)
{
	Model::Coarse* v = (Model::Coarse* )opaque_;
	switch (state_) 
	{
	case VIR_CONNECT_DOMAIN_EVENT_AGENT_LIFECYCLE_STATE_CONNECTED:
		v->show(domain_, boost::bind
			(&Registry::Reactor::connectAgent, _1));
		break;
	case VIR_CONNECT_DOMAIN_EVENT_AGENT_LIFECYCLE_STATE_DISCONNECTED:
		v->show(domain_, boost::bind
			(&Registry::Reactor::disconnectAgent, _1));
		break;
	}
	return 0;
}

int deviceConnect(virConnectPtr , virDomainPtr domain_, const char *device_,
	void *opaque_)
{
	Model::Coarse* v = (Model::Coarse* )opaque_;
	v->updateConnected(domain_, device_, PVE::DeviceConnected);
	return 0;
}

int deviceDisconnect(virConnectPtr , virDomainPtr domain_, const char* device_,
                        void* opaque_)
{
	Model::Coarse* v = (Model::Coarse* )opaque_;
	v->updateConnected(domain_, device_, PVE::DeviceDisconnected);
	return 0;
}

int trayChange(virConnectPtr connect_, virDomainPtr domain_,
		const char* device_, int reason_, void* opaque_)
{
	const char PREFIX[] = "drive-";
	if (boost::starts_with(device_, PREFIX))
		device_ += strlen(PREFIX);

	switch (reason_)
	{
	case VIR_DOMAIN_EVENT_TRAY_CHANGE_OPEN:
		deviceDisconnect(connect_, domain_, device_, opaque_);
		break;
	case VIR_DOMAIN_EVENT_TRAY_CHANGE_CLOSE:
		deviceConnect(connect_, domain_, device_, opaque_);
		break;
	default:
		WRITE_TRACE(DBG_WARNING, "Unknown trayChange reason %d", reason_);
		break;
	}
	return 0;
}

int rtcChange(virConnectPtr , virDomainPtr domain_,
		qint64 utcoffset_, void* opaque_)
{
	Model::Coarse* v = (Model::Coarse* )opaque_;
	v->adjustClock(domain_, utcoffset_);
	return 0;
}

int networkLifecycle(virConnectPtr, virNetworkPtr net_, int event_, int, void* opaque_)
{
	if (event_ != VIR_NETWORK_EVENT_STARTED)
		return 0;

	Model::Coarse* v = (Model::Coarse* )opaque_;
	v->updateInterfaces(net_);
	return 0;
}

void error(void* opaque_, virErrorPtr value_)
{
	Q_UNUSED(value_);
	Q_UNUSED(opaque_);
	WRITE_TRACE(DBG_DEBUG, "connection error: %s", value_->message);
}

} // namespace Plain

namespace Reactor
{
///////////////////////////////////////////////////////////////////////////////
// struct Performance

void Performance::operator()(Registry::Access& access_)
{
	Prl::Expected<Instrument::Agent::Vm::Stat::CounterList_type, Error::Simple>
		c = m_source.getCpu();
	if (c.isSucceed())
		account(access_, c.value());

	account(access_, m_source.getMemory());

	boost::optional<CVmConfiguration> x = access_.getConfig();
	if (x)
	{
		foreach (const CVmGenericNetworkAdapter* a, x->getVmHardwareList()->m_lstNetworkAdapters)
		{
			if (a->getEnabled() != PVE::DeviceEnabled ||
				a->getConnected() != PVE::DeviceConnected)
				continue;

			Prl::Expected<Instrument::Agent::Vm::Stat::CounterList_type, Error::Simple>
				s = m_source.getInterface(a);
			if (s.isSucceed())
				account(access_, s.value());
		}

		foreach (const CVmHardDisk* d, x->getVmHardwareList()->m_lstHardDisks)
		{
			if (d->getEnabled() != PVE::DeviceEnabled ||
				d->getConnected() != PVE::DeviceConnected)
				continue;

			Prl::Expected<Instrument::Agent::Vm::Stat::CounterList_type, Error::Simple>
				s = m_source.getDisk(d);
			if (s.isSucceed())
				account(access_, s.value());
		}
	}

	Prl::Expected<Instrument::Agent::Vm::Stat::CounterList_type, Error::Simple>
		vc = m_source.getVCpuList();
	if (vc.isSucceed())
		account(access_, vc.value());
}

void Performance::account(Registry::Access& access_, const data_type& data_)
{
	QSharedPointer<Stat::Storage> s = access_.getStorage();
	if (s.isNull())
		return;

	foreach (const Instrument::Agent::Vm::Stat::Counter_type& c, data_)
	{
		s->addAbsolute(c.first);
		s->write(c.first, c.second, PrlGetTimeMonotonic());
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct State

void State::read(agent_type agent_)
{
	m_value = VMS_UNKNOWN;
	if (agent_.getState().getValue(m_value).isFailed())
		WRITE_TRACE(DBG_FATAL, "Unable to get VM state");
}

void State::operator()(Registry::Access& access_)
{
	value_type v = m_value;
	if (VMS_STOPPED == v)
	{
		boost::optional<CVmConfiguration> c = access_.getConfig();
		if (c)
		{
			CStatesHelper h(c.get().getVmIdentification()->getHomePath());
			if (h.savFileExists())
				v = VMS_SUSPENDED;
		}
	}
	access_.getReactor().proceed(v);
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

void Device::operator()(Registry::Access& access_)
{
	CVmConfiguration r;
	if (m_agent.getConfig(r, true).isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get VM runtime configuration");
		return;
	}
	access_.getReactor().updateConnected(m_alias, m_state, r);
	if (PVE::DeviceDisconnected != m_state)
		return;

	QString u;
	m_agent.getUuid(u);
	CDspService::instance()->getVmStateSender()->onVmDeviceDetached(u, m_alias);
}

///////////////////////////////////////////////////////////////////////////////
// struct Domain

void Domain::updateConfig(Registry::Access& access_)
{
	m_state.read(m_agent);
	CVmConfiguration c;
	if (m_agent.getConfig(c).isFailed())
		return;

	CVmConfiguration runtime;
	if ((m_state.getValue() == VMS_RUNNING || m_state.getValue() == VMS_PAUSED)
		&& m_agent.getConfig(runtime, true).isSucceed())
		Vm::Config::Repairer<Vm::Config::revise_types>::type::do_(c, runtime);

	m_agent.completeConfig(c);
	access_.updateConfig(c);
}

void Domain::update(Registry::Access& access_)
{
	updateConfig(access_);
	m_state(access_);
}

void Domain::switch_(Registry::Access& access_)
{
	updateConfig(access_);
	switch (m_state.getValue())
	{
	case VMS_PAUSED:
	case VMS_STOPPED:
		return m_state(access_);
	default:
		break;
	}
}

void Domain::insert(Registry::Access& access_)
{
	boost::optional<CVmConfiguration> c = access_.getConfig();
	if (c)
		return update(access_);

	updateConfig(access_);
	c = access_.getConfig();
	if (!c) 
		WRITE_TRACE(DBG_DEBUG, "Unable to redefine configuration for new VM");
	else
	{
		Libvirt::Result e = m_agent.setConfig(c.get());
		if (e.isFailed())
		{
			WRITE_TRACE(DBG_DEBUG, "Unable to redefine configuration for new VM: %s",
				qPrintable(e.error().convertToEvent().toString()));
		}
	}
	m_state(access_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Network

void Network::operator()(Registry::Access& access_)
{
	boost::optional<CVmConfiguration> c = access_.getConfig();
	if (!c)
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get VM config");
		return;
	}

	foreach(CVmGenericNetworkAdapter *a, c->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (a->getVirtualNetworkID() != m_id)
		continue;

		m_agent.getRuntime().update(*a);
	}
}

namespace Crash
{
///////////////////////////////////////////////////////////////////////////////
// struct Primary

void Primary::operator()(Registry::Access& access_, CVmConfiguration snapshot_)
{
	CVmOnCrash *o = snapshot_.getVmSettings()->getVmRuntimeOptions()->getOnCrash();
	if (o->getMode() != POCA_RESTART || !(o->getOptions() & POCO_REPORT))
		return;

	m_pits.push_back(PrlGetTimeMonotonic());
	quint32 s = m_pits.size();
	if (s < o->getLimit())
		return;

	if (s > o->getLimit())
		m_pits.pop_front();

	quint64 d = m_pits.back() - m_pits.front();
	// 24 * 3600 * 1000000
	if (d < 86400000000ULL)
	{
		o->setMode(POCA_PAUSE);
		access_.updateConfig(snapshot_);

		m_pits.clear();
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Secondary

void Secondary::run()
{
	if (m_config.getOptions() & POCO_REPORT)
	{
		CProblemReport r;
		r.setReportType(PRT_AUTOMATIC_VM_GENERATED_REPORT);
		CProtoCommandPtr cmd(new CProtoSendProblemReport(r.toString(), m_uuid, 0));
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdSendProblemReport, cmd);
		QString vmDir = CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->getDefaultVmDirectory();
		SmartPtr<CDspClient> c = CDspClient::makeServiceUser(vmDir);
		CDspTaskFuture<Task_CreateProblemReport> ft = CDspService::instance()->getTaskManager().schedule(new Task_CreateProblemReport(c, p));
		if (POCA_RESTART == m_config.getMode())
			ft.wait();
	}
	if (POCA_RESTART == m_config.getMode())
	{
		Command::Vm::Gear<Command::Tag::State<Command::Vm::Resurrect,
			Command::Vm::Fork::State::Strict<VMS_RUNNING> > >::run(m_uuid);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Shell

void Shell::operator()(Registry::Access& access_)
{
	QSharedPointer<Primary> p = m_primary.toStrongRef();
	if (p.isNull())
		return;

	boost::optional<CVmConfiguration> c = access_.getConfig();
	if (!c)
		return;

	(*p)(access_, c.get());
	QRunnable* q = new Secondary(c->getVmIdentification()->getVmUuid(),
		c->getVmSettings()->getVmRuntimeOptions()->getOnCrash());
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

} // namespace Crash
} // namespace Reactor
} // namespace Callback

namespace Reaction
{
///////////////////////////////////////////////////////////////////////////////
// struct Shell

void Shell::run()
{
	if (m_queue.isNull())
		return;

	forever
	{
		QMutexLocker g(&m_queue->second);
		if (m_queue->first.isEmpty())
			return;

		reaction_type y = m_queue->first.head();
		g.unlock();
		y(m_access);
		g.relock();
		(void)m_queue->first.dequeue();
		if (m_queue->first.isEmpty())
			return;
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Demonstrator

Demonstrator::Demonstrator(const Registry::Access& access_):
	m_access(access_), m_queue(new Shell::queue_type()), m_crash(new crash_type())
{
}

void Demonstrator::show(const Shell::reaction_type& reaction_)
{
	QMutexLocker g(&m_queue->second);
	bool x = m_queue->first.isEmpty();
	m_queue->first.enqueue(reaction_);
	if (!x) 
		return;

	QRunnable* q = new Shell(m_queue, m_access);
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

void Demonstrator::showCrash()
{
	show(Callback::Reactor::Crash::Shell(m_crash.toWeakRef()));
}

} // namespace Reaction

namespace Model
{
///////////////////////////////////////////////////////////////////////////////
// struct System

System::System(Registry::Actual& registry_): m_registry(registry_)
{
}

void System::remove(const QString& uuid_)
{
	domainMap_type::iterator p = m_domainMap.find(uuid_);
	if (m_domainMap.end() == p)
		return;

	domainMap_type::mapped_type x = p.value();
	x->show(Callback::Reactor::State(VMS_UNKNOWN));
	x->show(boost::bind(&Registry::Actual::undefine, &m_registry, uuid_));

	m_domainMap.erase(p);
}

QSharedPointer<System::entry_type> System::add(const QString& uuid_)
{
	if (uuid_.isEmpty() || m_domainMap.contains(uuid_))
		return QSharedPointer<System::entry_type>();

	Prl::Expected<Registry::Access, Error::Simple> a = m_registry.define(uuid_);
	if (a.isFailed())
		return QSharedPointer<System::entry_type>();

	QSharedPointer<System::entry_type> x(new System::entry_type(a.value()));
	return m_domainMap[uuid_] = x;
}

QSharedPointer<System::entry_type> System::find(const QString& uuid_)
{
	domainMap_type::const_iterator p = m_domainMap.find(uuid_);
	if (m_domainMap.end() == p)
		return QSharedPointer<System::entry_type>();

	return p.value();
}

///////////////////////////////////////////////////////////////////////////////
// struct Coarse

QString Coarse::getUuid(virDomainPtr domain_)
{
	QString output;
	virDomainRef(domain_);
	Instrument::Agent::Vm::Unit(domain_).getUuid(output);
	return output;
}

void Coarse::emitDefined(virDomainPtr domain_)
{
	namespace agent = Libvirt::Instrument::Agent::Vm::Limb;
	virDomainRef(domain_);
	agent::Maintenance(agent::Abstract::domainReference_type(domain_, &virDomainFree))
		.emitDefined();
}

bool Coarse::show(virDomainPtr domain_, const reaction_type& reaction_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (d.isNull())
		return false;

	d->show(Callback::Reactor::Show(reaction_));
	return true;
}

void Coarse::setState(virDomainPtr domain_, VIRTUAL_MACHINE_STATE value_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (!d.isNull())
		d->show(Callback::Reactor::State(value_));
}

void Coarse::prepareToSwitch(virDomainPtr domain_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (d.isNull())
		return;

	virDomainRef(domain_);
	d->show(Callback::Reactor::Show(boost::bind(&Registry::Reactor::prepareToSwitch, _1)));
	Instrument::Agent::Vm::Unit a(domain_);
	Callback::Reactor::Domain r(a);
	d->show(boost::bind(&Callback::Reactor::Domain::switch_, r, _1));
}

void Coarse::remove(virDomainPtr domain_)
{
	m_fine->remove(getUuid(domain_));
}

void Coarse::onCrash(virDomainPtr domain_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (d.isNull())
		return;

	setState(domain_, VMS_PAUSED);
	d->showCrash();
}

void Coarse::pullInfo(virDomainPtr domain_)
{
	virDomainRef(domain_);
	QString u = getUuid(domain_);
	Instrument::Agent::Vm::Unit a(domain_);
	Callback::Reactor::Domain r(a);
	QSharedPointer<System::entry_type> d = m_fine->find(u);
	if (!d.isNull())
		d->show(boost::bind(&Callback::Reactor::Domain::updateConfig, r, _1));
	else
	{
		if ((d = m_fine->add(u)).isNull())
		{
			WRITE_TRACE(DBG_DEBUG, "Unable to add new domain to the list");
			return;
		}
		d->show(boost::bind(&Callback::Reactor::Domain::insert, r, _1));
	}
}

void Coarse::updateConnected(virDomainPtr domain_, const QString& alias_,
	PVE::DeviceConnectedState value_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (d.isNull())
		return;

	virDomainRef(domain_);
	Instrument::Agent::Vm::Unit a(domain_);
	d->show(Callback::Reactor::Device(alias_, value_, a));
}

void Coarse::adjustClock(virDomainPtr domain_, qint64 offset_)
{
	virDomainRef(domain_);
	Instrument::Agent::Vm::Unit a(domain_);
	a.getMaintenance().adjustClock(offset_);
}

void Coarse::updateInterfaces(virNetworkPtr net_)
{
	QThreadPool::globalInstance()->start
		(new Instrument::Pull::Network(virNetworkGetName(net_), m_fine));
}

} // namespace Model

namespace Monitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Link

Link::Link(int timeout_)
{
	this->connect(&m_timer, SIGNAL(timeout()), SLOT(setOpen()));
	m_timer.start();
	m_timer.setInterval(timeout_);
}

void Link::setOpen()
{
	WRITE_TRACE(DBG_FATAL, "libvirt connect");
	if (!m_libvirtd.isNull())
		return;

	virConnectPtr c = virConnectOpen("qemu+unix:///system");
	if (NULL == c)
		return setClosed();

	int e = virConnectRegisterCloseCallback(c, &disconnect, this, NULL);
	if (0 != e)
	{
		virConnectClose(c);
		return setClosed();
	}
	m_timer.stop();
	m_libvirtd = QSharedPointer<virConnect>(c, &virConnectClose);
	emit connected(m_libvirtd);
}

void Link::setClosed()
{
	WRITE_TRACE(DBG_FATAL, "libvirt reconnect");
	m_libvirtd.clear();
	m_timer.start();
}

void Link::disconnect(virConnectPtr libvirtd_, int reason_, void* opaque_)
{
	WRITE_TRACE(DBG_FATAL, "libvirt connection is lost");
	Q_UNUSED(reason_);
	Q_UNUSED(libvirtd_);
	Link* x = (Link* )opaque_;
	emit x->disconnected();
	if (QThread::currentThread() == x->thread())
		x->setClosed();
	else
		QMetaObject::invokeMethod(x, "setClosed");
}

///////////////////////////////////////////////////////////////////////////////
// struct Domains

Domains::Domains(Registry::Actual& registry_):
	m_eventState(-1), m_eventReboot(-1),
	m_eventWakeUp(-1), m_eventDeviceConnect(-1), m_eventDeviceDisconnect(-1),
	m_eventTrayChange(-1), m_eventRtcChange(-1), m_eventAgent(-1),
	m_eventNetworkLifecycle(-1), m_registry(&registry_)
{
}

void Domains::setConnected(QSharedPointer<virConnect> libvirtd_)
{
	Callback::Mock::model_type v(new Model::System(*m_registry));
	m_libvirtd = libvirtd_.toWeakRef();
	Callback::g_access.setOpaque(Callback::Mock::ID, new Callback::Transport::Model(v));
	Kit.setLink(libvirtd_);
	(new Performance::Miner(Kit.vms(), v.toWeakRef()))
		->startTimer(PERFORMANCE_TIMEOUT);
	m_eventState = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_LIFECYCLE,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::State::react),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventReboot = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_REBOOT,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::Plain::reboot),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventWakeUp = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_PMWAKEUP,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::Plain::wakeUp),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventDeviceConnect = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_DEVICE_ADDED,
							VIR_DOMAIN_EVENT_CALLBACK(Callback::Plain::deviceConnect),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventDeviceDisconnect = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED,
							VIR_DOMAIN_EVENT_CALLBACK(Callback::Plain::deviceDisconnect),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventTrayChange = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_TRAY_CHANGE,
							VIR_DOMAIN_EVENT_CALLBACK(Callback::Plain::trayChange),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventRtcChange = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_RTC_CHANGE,
							VIR_DOMAIN_EVENT_CALLBACK(Callback::Plain::rtcChange),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventAgent = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_AGENT_LIFECYCLE,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::Plain::connectAgent),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventNetworkLifecycle = virConnectNetworkEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_NETWORK_EVENT_ID_LIFECYCLE,
							VIR_NETWORK_EVENT_CALLBACK(Callback::Plain::networkLifecycle),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	QRunnable* q = new Instrument::Breeding::Subject(m_libvirtd, v, *m_registry);
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

void Domains::setDisconnected()
{
	QSharedPointer<virConnect> x;
	Kit.setLink(x);
	x = m_libvirtd.toStrongRef();
	virConnectDomainEventDeregisterAny(x.data(), m_eventState);
	m_eventState = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventReboot);
	m_eventReboot = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventWakeUp);
	m_eventWakeUp = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventDeviceConnect);
	m_eventDeviceConnect = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventDeviceDisconnect);
	m_eventDeviceDisconnect = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventTrayChange);
	m_eventTrayChange = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventRtcChange);
	m_eventRtcChange = -1;
	virConnectDomainEventDeregisterAny(x.data(), m_eventAgent);
	m_eventAgent = -1;
	virConnectNetworkEventDeregisterAny(x.data(), m_eventNetworkLifecycle);
	m_eventNetworkLifecycle = -1;
	Callback::g_access.setOpaque(Callback::Mock::ID, new Callback::Transport::Model());
	m_libvirtd.clear();
}

namespace Performance
{
///////////////////////////////////////////////////////////////////////////////
// struct Broker

void Broker::despatch()
{
	if (m_view.isNull())
		return;

	foreach (const unit_type& p, m_list)
	{
		QString u;
		if (p.getUuid(u).isFailed())
			continue;

		QSharedPointer<Model::System::entry_type> v = m_view->find(u);
		if (!v.isNull())
			v->show(Callback::Reactor::Performance(p));
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Miner

PRL_RESULT Miner::operator()()
{
	QSharedPointer<Model::System> x = m_view.toStrongRef();
	if (x.isNull())
		return PRL_ERR_UNINITIALIZED;

	Broker* b = new Broker(m_agent.getPerformance(), x);
	QTimer* t = new QTimer();
	t->setSingleShot(true);
	b->setParent(t);
	t->moveToThread(x->thread());
	b->connect(t, SIGNAL(timeout()), SLOT(despatch()));
	t->connect(t, SIGNAL(timeout()), SLOT(deleteLater()));
	QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection);

	return PRL_ERR_SUCCESS;
}

Miner* Miner::clone() const
{
	return new Miner(m_agent, m_view);
}

void Miner::timerEvent(QTimerEvent* event_)
{
	killTimer(event_->timerId());
	QRunnable* q = new Task(this);
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

///////////////////////////////////////////////////////////////////////////////
// struct Task

void Task::run()
{
	if (m_miner.isNull())
		return;

	if (PRL_ERR_UNINITIALIZED != (*m_miner)())
	{
		Miner* m = m_miner->clone();
		if (NULL != m)
		{
			m->startTimer(PERFORMANCE_TIMEOUT);
			m->moveToThread(m_miner->thread());
		}
	}
	m_miner.take()->deleteLater();
}

} // namespace Performance
} // namespace Monitor

///////////////////////////////////////////////////////////////////////////////
// struct Host

void Host::run()
{
	Monitor::Link a;
	Monitor::Domains b(m_registry);
	b.connect(&a, SIGNAL(disconnected()), SLOT(setDisconnected()));
	b.connect(&a, SIGNAL(connected(QSharedPointer<virConnect>)),
		SLOT(setConnected(QSharedPointer<virConnect>)));

	QSharedPointer<Callback::Hub> h(new Callback::Hub());
	Callback::g_access.setHub(h);
	virSetErrorFunc(NULL, &Callback::Plain::error);
	virEventRegisterImpl(&Callback::Plain::addSocket,
			&Callback::Plain::setSocket,
			&Callback::Plain::remove,
			&Callback::Plain::addTimeout,
			&Callback::Plain::setTimeout,
			&Callback::Plain::remove);
	exec();
	a.setClosed();
}

namespace Instrument
{
namespace Agent
{
namespace Vm
{
namespace Limb
{
///////////////////////////////////////////////////////////////////////////////
// struct Maintenance

void Maintenance::emitLifecycle(int category_, int type_)
{
	Callback::g_access.setOpaque(Callback::Mock::ID,
		new Callback::Transport::Event(boost::bind
			(Callback::State(getLink(), getDomain()), category_, type_, _1)));
}

void Maintenance::emitDefined()
{
	emitLifecycle(VIR_DOMAIN_EVENT_DEFINED, 0);
}

void Maintenance::emitQemuUpdated()
{
	emitLifecycle(VIR_DOMAIN_EVENT_STARTED, -1);
}

} // namespace Limb
} // namespace Vm
} // namespace Agent
} // namespace Instrument
} // namespace Libvirt
