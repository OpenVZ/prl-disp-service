/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QXmlQuery>
#include <QHostAddress>
#include "CDspService.h"
#include <QXmlResultItems>
#include "Build/Current.ver"
#include "CDspVmManager_p.h"
#include <QtCore/qmetatype.h>
#include "CDspVmStateSender.h"
#include "CDspVmNetworkHelper.h"
#include "CDspBackupDevice.h"
#include "Stat/CDspStatStorage.h"
#include <boost/bind/protect.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/phoenix/statement.hpp>
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
#include <boost/functional/value_factory.hpp>
#include "CDspLibvirt_p.h"
#include <prlxmlmodel/ProblemReport/CProblemReport.h>
#include <boost/phoenix/bind/bind_function_object.hpp>

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

Result Config::alter(const Transponster::Vm::Reverse::Pipeline::action_type& transformer_)
{
	if (m_link.isNull())
		return Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER);

	Transponster::Vm::Reverse::Pipeline u(transformer_);
	PRL_RESULT e = Transponster::Director::marshalDirect(read_(), u);
	if (PRL_FAILED(e))
		return Error::Simple(e);

	virDomainPtr d = virDomainDefineXML(m_link.data(), u.getValue().toUtf8().data());
	if (NULL == d)
		return Failure(PRL_ERR_VM_APPLY_CONFIG_FAILED);

	m_domain = QSharedPointer<virDomain>(d, &virDomainFree);
	return Result();
}

} // namespace Agent

namespace Breeding
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

void Vm::operator()(Agent::Hub& hub_)
{
	QList<Agent::Vm::Unit> a;
	if (hub_.vms().all(a).isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot list VMs");
		return;
	}
	WRITE_TRACE(DBG_FATAL, "Got %d VMs", a.size());

	QHash<QString, Agent::Vm::Unit> b, c;
	foreach (Agent::Vm::Unit m, a)
	{
		QString u;
		m.getUuid(u);
		WRITE_TRACE(DBG_DEBUG, "persistent UUID %s", qPrintable(u));
		b[u] = m;
	}
	QStringList s = m_view->getHost().getRegistry().snapshot();
	foreach (const QString& u, s)
	{
		if (!validate(u))
			c[u] = b.take(u);
	}
	m_view->getHost().getRegistry().reset();
	foreach (const QString& u, b.keys())
	{
		if (m_view->add(u).isNull())
			b.remove(u);
		else
			s.removeOne(u);
	}
	foreach (const QString& u, s)
	{
		WRITE_TRACE(DBG_FATAL, "VM %s disappeared", qPrintable(u));

		// in order to properly remove the directory item, first we need to
		// add the VM to the registry and then remove it from there
		m_view->getHost().getRegistry().define(u);
		// at this point VM is not registered in m_view->m_domainMap
		// thus we need to manually remove the corresponding
		// directory item from Dispatcher
		m_view->getHost().getRegistry().undefine(u);
	}
	foreach (const QString& u, c.keys())
	{
		c[u].getState().undefine();
	}
	foreach (const QString& u, b.keys())
	{
		WRITE_TRACE(DBG_DEBUG, "update UUID %s", qPrintable(u));
		Registry::Access a = m_view->getHost().getRegistry().find(u);
		Callback::Reactor::Domain(b[u]).update(a);
	}
}

bool Vm::validate(const QString& uuid_)
{
	Registry::Access a = m_view->getHost().getRegistry().find(uuid_);
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
	return y->getServerUuid() == m_view->getHost().getRegistry().getServerUuid();
}

namespace Host
{
///////////////////////////////////////////////////////////////////////////////
// struct Script

void Script::pullPci()
{
	Prl::Expected<QList<CHwGenericPciDevice>, Error::Simple> f =
		m_hub->host().getAssignablePci();
	if (f.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot read host PCI devices: %s",
			PRL_RESULT_TO_STRING(f.error().code()));
		return;
	}
	CDspLockedPointer<CDspHostInfo> i(CDspService::instance()->getHostInfo());
	if (!i.isValid())
		return;

	i->data()->ClearList<CHwGenericPciDevice>(i->data()->m_lstGenericPciDevices);
	foreach (const CHwGenericPciDevice& d, f.value())
	{
		i->data()->addGenericPciDevice(new CHwGenericPciDevice(d));
	}
}

void Script::syncNetwork(const QFileInfo& config_)
{
	QFileInfo d(config_.absoluteDir(), QString("digested.").append(config_.fileName()));
	if (d.exists())
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
	QString p(d.absoluteFilePath()); 
	QTemporaryFile t(p);
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
		::Network::Dao d(*m_hub);
		foreach (CVirtualNetwork* k, s->m_lstVirtualNetwork)
		{
			if (NULL != k && k->isEnabled())
				d.create(*k);
		}
	}
	if (!QFile::rename(t.fileName(), p))
	{
		WRITE_TRACE(DBG_FATAL, "cannot rename %s -> %s",
			QSTR2UTF8(t.fileName()), QSTR2UTF8(p));
		return;
	}
	t.setAutoRemove(false);
}

PRL_RESULT Script::syncCpu(const QString& config_)
{
	QFile f(config_);
	if (!f.open(QIODevice::ReadOnly))
	{                       
		WRITE_TRACE(DBG_FATAL, "cannot open %s",
			qPrintable(config_));
		return PRL_ERR_FILE_READ_ERROR;
	}
	QXmlQuery q;
	if (!q.setFocus(&f))
	{
		WRITE_TRACE(DBG_FATAL, "cannot parse the XML file %s",
			qPrintable(config_));
		return PRL_ERR_READ_XML_CONTENT;
	}
	QXmlResultItems m;
	q.setQuery("cpus/arch[1]/feature[cpuid]");
	q.evaluateTo(&m);

	Cpu b;
	forever
	{
		QXmlItem i(m.next());
		if (i.isNull())
			break;
		if (i.isNode())
		{
			q.setFocus(i);
			b(q);
		}
	}
	CCpuHelper::setCatalog(b.getResult());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

void Cpu::operator()(QXmlQuery& query_)
{
	static const char* a[] = {"eax", "ebx", "ecx", "edx"};
	quint32 i = 0, X = 0;
	for (quint32 c = boost::size(a); i < c; ++i)
	{
		QString x = evaluate(query_, QString("string(cpuid/@%1)").arg(a[i]));
		if (!x.isEmpty())
		{
			X = quint32(ffs(x.toUInt(NULL, 16)));
			break;
		}
	}
	PRL_CPU_FEATURES_EX k = translate(i, query_);
	if (PCFE_MAX == k)
		return;

	mapped_type& m = (*this)[k];
	while (m.size() < X)
	{
		m.push_back(QString());
	}
	m[X - 1] = evaluate(query_, "string(@name)");
}

QString Cpu::evaluate(QXmlQuery& query_, const QString& xpath_)
{
	QString x;
	query_.setQuery(xpath_);
	query_.evaluateTo(&x);

	return x.trimmed();
}

PRL_CPU_FEATURES_EX Cpu::translate(int register_, QXmlQuery& query_)
{
	quint32 a = evaluate(query_, "string(cpuid/@eax_in)").toUInt(NULL, 16);
	switch (register_)
	{
	case 0: 
		switch (a)
		{
		case 0x0d:
			return PCFE_EXT_0000000D_EAX;
		}
		break;
	case 1: 
		switch (a)
		{
		case 0x07:
			return PCFE_EXT_00000007_EBX;
		}
		break;
	case 2: 
		switch (a)
		{
		case 0x01:
			return PCFE_EXT_FEATURES;
		case 0x80000001:
			return PCFE_EXT_80000001_ECX;
		}
		break;
	case 3: 
		switch (a)
		{
		case 0x01:
			return PCFE_FEATURES;
		case 0x07:
			return PCFE_EXT_00000007_EDX;
		case 0x80000001:
			return PCFE_EXT_80000001_EDX;
		}
		break;
	}
	return PCFE_MAX;
}

} // namespace Host

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(QSharedPointer<virConnect> link_, QSharedPointer<Model::System> view_):
	m_vm(view_), m_target(view_->thread()), m_link(link_), m_system(view_)
{
}

void Subject::run()
{
	(Monitor::Hardware::Launcher(m_target))(m_link.toWeakRef(), m_system->getHost());

	Agent::Hub u;
	u.setLink(m_link);
	Host::Script h(u);
	h.syncCpu("/usr/share/libvirt/cpu_map.xml");
	h.syncNetwork(ParallelsDirs::getNetworkConfigFilePath());
	m_vm(u);
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
		WRITE_TRACE(DBG_FATAL, "event without a model");
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
	QMetaObject::invokeMethod(h.data(), "add", Qt::QueuedConnection, Q_ARG(int, output),
		Q_ARG(virEventTimeoutCallback, callback_));
	setOpaque(output, new Transport::Opaque(opaque_, free_));
	QMetaObject::invokeMethod(h.data(), "setInterval", Qt::QueuedConnection, Q_ARG(int, output), Q_ARG(int, interval_));
	return output;
}

int Access::add(int socket_, int events_, virEventHandleCallback callback_, void* opaque_, virFreeCallback free_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return -1;

	int output = m_generator.fetchAndAddOrdered(1);
	QMetaObject::invokeMethod(h.data(), "add", Qt::QueuedConnection, Q_ARG(int, output), Q_ARG(int, socket_),
		Q_ARG(virEventHandleCallback, callback_));
	setOpaque(output, new Transport::Opaque(opaque_, free_));
	QMetaObject::invokeMethod(h.data(), "setEvents", Qt::QueuedConnection, Q_ARG(int, output), Q_ARG(int, events_));
	return output;
}

void Access::setEvents(int id_, int value_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return;

	QMetaObject::invokeMethod(h.data(), "setEvents", Qt::QueuedConnection, Q_ARG(int, id_), Q_ARG(int, value_));
}

void Access::setInterval(int id_, int value_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return;

	QMetaObject::invokeMethod(h.data(), "setInterval", Qt::QueuedConnection, Q_ARG(int, id_), Q_ARG(int, value_));
}

void Access::setOpaque(int id_, Transport::Visitor* value_)
{
	QScopedPointer<Transport::Visitor> g(value_);
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return;

	QMetaObject::invokeMethod(h.data(), "setOpaque", Qt::QueuedConnection, Q_ARG(int, id_),
		Q_ARG(Transport::Visitor*, g.take()));
}

int Access::remove(int id_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return -1;

	QMetaObject::invokeMethod(h.data(), "remove", Qt::QueuedConnection, Q_ARG(int, id_));
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
// supplementary function to decode libvirt events

const char * humanReadableEvent(int event_, int subtype_)
{
	switch(event_)
	{
		case VIR_DOMAIN_EVENT_DEFINED:
			switch (subtype_)
			{
				case VIR_DOMAIN_EVENT_DEFINED_ADDED:
					return "Defined/newConfigFile";
				case VIR_DOMAIN_EVENT_DEFINED_UPDATED:
					return "Defined/changedConfigFile";
				case VIR_DOMAIN_EVENT_DEFINED_RENAMED:
					return "Defined/domainRenamed";
				case VIR_DOMAIN_EVENT_DEFINED_FROM_SNAPSHOT:
					return "Defined/configRestoredFromSnapshot";
				default:
					return "Defined/unknown";
			}
		case VIR_DOMAIN_EVENT_UNDEFINED:
			switch (subtype_)
			{
				case VIR_DOMAIN_EVENT_UNDEFINED_REMOVED:
					return "Undefined/deletedConfigFile";
				case VIR_DOMAIN_EVENT_UNDEFINED_RENAMED:
					return "Undefined/domainRenamed";
				default:
					return "Undefined/unknown";
			}
		case VIR_DOMAIN_EVENT_STARTED:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_STARTED_BOOTED:
					return "Started/normalBoot";
				case VIR_DOMAIN_EVENT_STARTED_MIGRATED:
					return "Started/incomingMigration";
				case VIR_DOMAIN_EVENT_STARTED_RESTORED:
					return "Started/restoredFromStateFile";
				case VIR_DOMAIN_EVENT_STARTED_FROM_SNAPSHOT:
					return "Started/restoredFromSnapshot";
				case VIR_DOMAIN_EVENT_STARTED_WAKEUP:
					return "Started/startedDueToWakeup";
				default:
					return "Started/unknown";
			}
		case VIR_DOMAIN_EVENT_SUSPENDED:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_SUSPENDED_PAUSED:
					return "Suspended/suspendedNormallyAdminPause";
				case VIR_DOMAIN_EVENT_SUSPENDED_MIGRATED:
					return "Suspended/suspendedOfflineMigration";
				case VIR_DOMAIN_EVENT_SUSPENDED_IOERROR:
					return "Suspended/suspendedDiskIoError";
				case VIR_DOMAIN_EVENT_SUSPENDED_WATCHDOG:
					return "Suspended/suspendedWatchdog";
				case VIR_DOMAIN_EVENT_SUSPENDED_RESTORED:
					return "Suspended/restoredFromStateFile";
				case VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT:
					return "Suspended/restoredFromSnapshot";
				case VIR_DOMAIN_EVENT_SUSPENDED_API_ERROR:
					return "Suspended/suspendedDueToLibvirtApiError";
				case VIR_DOMAIN_EVENT_SUSPENDED_POSTCOPY:
					return "Suspended/suspendedForPostCopyMigration";
				case VIR_DOMAIN_EVENT_SUSPENDED_POSTCOPY_FAILED:
					return "Suspended/suspendedAfterFailedPostCopy";
				default:
					return "Suspended/unknown";
			}
		case VIR_DOMAIN_EVENT_RESUMED:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_RESUMED_UNPAUSED:
					return "Resumed/normalAdminUnpause";
				case VIR_DOMAIN_EVENT_RESUMED_MIGRATED:
					return "Resumed/resumedMigrationCompletion";
				case VIR_DOMAIN_EVENT_RESUMED_FROM_SNAPSHOT:
					return "Resumed/resumedFromSnapshot";
				case VIR_DOMAIN_EVENT_RESUMED_POSTCOPY:
					return "Resumed/resumedWhilePostCopyMigrationRunning";
				default:
					return "Resumed/unknown";
			}
		case VIR_DOMAIN_EVENT_STOPPED:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN:
					return "Stopped/normalShutdown";
				case VIR_DOMAIN_EVENT_STOPPED_DESTROYED:
					return "Stopped/forcedPoweroffFromHost";
				case VIR_DOMAIN_EVENT_STOPPED_CRASHED:
					return "Stopped/guestCrashed";
				case VIR_DOMAIN_EVENT_STOPPED_MIGRATED:
					return "Stopped/migratedToAnotherHost";
				case VIR_DOMAIN_EVENT_STOPPED_SAVED:
					return "Stopped/savedToStateFile";
				case VIR_DOMAIN_EVENT_STOPPED_FAILED:
					return "Stopped/hostEmulatorFailed";
				case VIR_DOMAIN_EVENT_STOPPED_FROM_SNAPSHOT:
					return "Stopped/offlineSnapshotLoaded";
				default:
					return "Stopped/unknown";
			}
		case VIR_DOMAIN_EVENT_SHUTDOWN:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_SHUTDOWN_FINISHED:
					return "Shutdown/guestFinishedShutdown";
				case VIR_DOMAIN_EVENT_SHUTDOWN_GUEST:
					return "Shutdown/domainFinishedShutdownAfterRequestFromGuest";
				case VIR_DOMAIN_EVENT_SHUTDOWN_HOST:
					return "Shutdown/domainFinishedShutdownAfterRequestFromHost";
				default:
					return "/unknown";
			}
		case VIR_DOMAIN_EVENT_PMSUSPENDED:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_PMSUSPENDED_MEMORY:
					return "PmSuspended/guestPmSuspendedToMemory";
				case VIR_DOMAIN_EVENT_PMSUSPENDED_DISK:
					return "PmSuspended/guestPmSuspendedToDisk";
				default:
					return "PmSuspended/unknown";
			}
		case VIR_DOMAIN_EVENT_CRASHED:
			switch(subtype_)
			{
				case VIR_DOMAIN_EVENT_CRASHED_PANICKED:
					return "Crashed/guestPanicked";
				default:
					return "Crashed/unknown";
			}
		default:
			return "Unknown";
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct State

int State::react(virConnectPtr, virDomainPtr domain_, int event_,
	int subtype_, void* opaque_)
{
	QSharedPointer<Model::System::entry_type> d;
	Model::Coarse* v = (Model::Coarse* )opaque_;
	WRITE_TRACE(DBG_FATAL, "libvirtEvent: VM \"%s\" received (%d/%d) %s",
		virDomainGetName(domain_), event_, subtype_, humanReadableEvent(event_, subtype_));
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
			v->reactStart(domain_);
		case VIR_DOMAIN_EVENT_STARTED_MIGRATED:
			// This event means that live migration is started, but VM has
			// not been defined yet. Ignore it.
			break;
		}
		return 0;
	case VIR_DOMAIN_EVENT_RESUMED:
		v->setState(domain_, VMS_RUNNING);
		return 0;
	case VIR_DOMAIN_EVENT_SUSPENDED:
		switch (subtype_)
		{
		case VIR_DOMAIN_EVENT_SUSPENDED_PAUSED:
		case VIR_DOMAIN_EVENT_SUSPENDED_MIGRATED:
		case VIR_DOMAIN_EVENT_SUSPENDED_IOERROR:
		case VIR_DOMAIN_EVENT_SUSPENDED_WATCHDOG:
		case VIR_DOMAIN_EVENT_SUSPENDED_RESTORED:
		case VIR_DOMAIN_EVENT_SUSPENDED_API_ERROR:
		case VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT:
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
			(&Registry::Reactor::updateAgent, _1, boost::none));
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
	CDspLockedPointer<CDspVmStateSender> x(CDspService::instance()->getVmStateSender());
	if (!x.isValid())
		return;

	x->onVmConfigChanged(QString(), access_.getUuid());
	if (PVE::DeviceDisconnected == m_state)
		x->onVmDeviceDetached(access_.getUuid(), m_alias);
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
	CDspLockedPointer<CDspVmStateSender> x(CDspService::instance()->getVmStateSender());
	if (x.isValid())
		x->onVmConfigChanged(QString(), access_.getUuid());
}

void Domain::update(Registry::Access& access_)
{
	updateConfig(access_);
	m_state(access_);
}

void Domain::insert(Registry::Access& access_)
{
	boost::optional<CVmConfiguration> c = access_.getConfig();
	if (c)
		return update(access_);

	updateConfig(access_);
	c = access_.getConfig();
	if (c && c->getVmIdentification()->getSourceVmUuid().isEmpty())
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

void Domain::switch_(Registry::Access& access_)
{
	updateConfig(access_);
	m_agent.getMaintenance().emitRestored();
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
	m_bench->getPool().start(q);
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

Demonstrator::Demonstrator(const Registry::Access& access_, Workbench& bench_):
	m_bench(&bench_), m_access(access_), m_queue(new Shell::queue_type()),
	m_crash(new crash_type())
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
	m_bench->getPool().start(q);
}

void Demonstrator::showCrash()
{
	show(Callback::Reactor::Crash::Shell(m_crash.toWeakRef(), *m_bench));
}

} // namespace Reaction

namespace Model
{
///////////////////////////////////////////////////////////////////////////////
// struct Entry

Entry::Entry(const Registry::Access& access_, Workbench& bench_):
	Reaction::Demonstrator(access_, bench_), m_last(VMS_UNKNOWN)
{
}

void Entry::setState(VIRTUAL_MACHINE_STATE value_)
{
	if (value_ == m_last)
	{
		WRITE_TRACE(DBG_FATAL, "duplicate status %s is detected for a VM. ignore",
			PRL_VM_STATE_TO_STRING(value_));
	}
	else
	{
		switch (m_last = value_)
		{
		case VMS_RESTORING:
			return show(Callback::Reactor::Show(
				boost::bind(&Registry::Reactor::prepareToSwitch, _1)));

		default:
			show(Callback::Reactor::State(value_));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct System

System::System(Workbench& bench_): m_bench(&bench_)
{
}

void System::remove(const QString& uuid_)
{
	WRITE_TRACE(DBG_FATAL, "REMOVE %s %p %d", qPrintable(uuid_), this, m_domainMap.size());
	domainMap_type::iterator p = m_domainMap.find(uuid_);
	if (m_domainMap.end() == p)
		return;

	domainMap_type::mapped_type x = p.value();
	x->setState(VMS_UNKNOWN);
	x->show(boost::bind(&Registry::Actual::undefine, &m_bench->getRegistry(), uuid_));

	m_domainMap.erase(p);
}

QSharedPointer<System::entry_type> System::add(const QString& uuid_)
{
	WRITE_TRACE(DBG_FATAL, "ADD1 %s", qPrintable(uuid_));
	if (uuid_.isEmpty() || m_domainMap.contains(uuid_))
		return QSharedPointer<System::entry_type>();

	Prl::Expected<Registry::Access, Error::Simple> a = m_bench->getRegistry().define(uuid_);
	if (a.isFailed())
		return QSharedPointer<System::entry_type>();

	WRITE_TRACE(DBG_FATAL, "ADD2 %s %p %d", qPrintable(uuid_), this, m_domainMap.size());
	QSharedPointer<System::entry_type> x(new System::entry_type(a.value(), *m_bench));
	return m_domainMap[uuid_] = x;
}

QSharedPointer<System::entry_type> System::find(const QString& uuid_)
{
	WRITE_TRACE(DBG_FATAL, "FIND %s %p %d", qPrintable(uuid_), this, m_domainMap.size());
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

void Coarse::reactStart(virDomainPtr domain_)
{
	QString u = getUuid(domain_);
	QSharedPointer<System::entry_type> d = m_fine->find(u);
	if (d.isNull())
		return;

	switch (d->getLast())
	{
	case VMS_PAUSED:
		break;
	default:
		d->setState(VMS_RUNNING);
	}
}

void Coarse::setState(virDomainPtr domain_, VIRTUAL_MACHINE_STATE value_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (!d.isNull())
		d->setState(value_);
}

void Coarse::prepareToSwitch(virDomainPtr domain_)
{
	QSharedPointer<System::entry_type> d = m_fine->find(getUuid(domain_));
	if (d.isNull())
		return;

	virDomainRef(domain_);
	d->setState(VMS_RESTORING);
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
	namespace vm = Instrument::Agent::Vm;

	virDomainRef(domain_);
	boost::function<Result ()> x(boost::bind(&vm::Limb::Maintenance::adjustClock,
		vm::Unit(domain_).getMaintenance(), offset_));
	show(domain_, boost::phoenix::bind(x));
}

void Coarse::updateInterfaces(virNetworkPtr net_)
{
	m_fine->getHost().getPool().start
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

Domains::Domains(Workbench& bench_):
	m_eventState(-1), m_eventReboot(-1),
	m_eventWakeUp(-1), m_eventDeviceConnect(-1), m_eventDeviceDisconnect(-1),
	m_eventTrayChange(-1), m_eventRtcChange(-1), m_eventAgent(-1),
	m_eventNetworkLifecycle(-1), m_eventHardwareLifecycle(-1), m_bench(&bench_)
{
}

void Domains::setConnected(QSharedPointer<virConnect> libvirtd_)
{
	Callback::Mock::model_type v(new Model::System(*m_bench));
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
							VIR_NETWORK_EVENT_CALLBACK(&Callback::Plain::networkLifecycle),
							new Model::Coarse(v),
							&Callback::Plain::delete_<Model::Coarse>);
	m_eventHardwareLifecycle = virConnectNodeDeviceEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_NODE_DEVICE_EVENT_ID_LIFECYCLE,
							VIR_NODE_DEVICE_EVENT_CALLBACK(&Hardware::Usb::Shell::react),
							new Hardware::Usb::Shell(*m_bench),
							&Callback::Plain::delete_<Hardware::Usb::Shell>);
	QRunnable* q = new Instrument::Breeding::Subject(m_libvirtd, v);
	q->setAutoDelete(true);
	m_bench->getPool().start(q);
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
	virConnectNetworkEventDeregisterAny(x.data(), m_eventHardwareLifecycle);
	m_eventHardwareLifecycle = -1;
	Callback::g_access.setOpaque(Callback::Mock::ID, new Callback::Transport::Model());
	m_libvirtd.clear();
	m_bench->getPool().waitForDone();
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
	QSharedPointer<Model::System> x = m_view.toStrongRef();
	if (x.isNull())
		return;

	QRunnable* q = new Task(this);
	q->setAutoDelete(true);
	x->getHost().getPool().start(q);
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

namespace Hardware
{
namespace Usb
{
///////////////////////////////////////////////////////////////////////////////
// struct Job

void Job::run()
{
	CUsbAuthenticNameList x;
	{
		CDspLockedPointer<CDspHostInfo> i = m_service->getHostInfo();
		i->refresh(CDspHostInfo::uhiUsb);
		x.SetUsbPreferences(i->getUsbAuthentic()->GetUsbPreferences());
	}
	m_service->updateCommonPreferences(boost::bind<PRL_RESULT>
		(*this, boost::cref(x), _1));
}

PRL_RESULT Job::operator()(const CUsbAuthenticNameList& update_, CDispCommonPreferences& dst_) const
{
	CDispUsbPreferences w(dst_.getUsbPreferences());
	QHash<QString, QList<CDispUsbAssociation* > > m;
	foreach(CDispUsbIdentity* i, w.m_lstAuthenticUsbMap)
	{
		if (!i->m_lstAssociations.isEmpty())
		{
			m[i->getSystemName()].append(i->m_lstAssociations);
			i->m_lstAssociations.clear();
		}
	}
	CDispUsbPreferences u(update_.GetUsbPreferences());
	foreach(CDispUsbIdentity* i, u.m_lstAuthenticUsbMap)
	{
		i->ClearList(i->m_lstAssociations);
		QList<QString> k = m.keys();
		// Check if we have stored any associations for i->getSystemName;
		QList<QString>::const_iterator e = k.end(), p = k.begin();
		p = std::find_if(p, e, boost::bind(&CUsbAuthenticNameList::IsTheSameDevice,
			&update_, boost::cref(i->getSystemName()), _1));
		if (e != p)
			i->m_lstAssociations = m.take(*p);

	}
	if (!m.isEmpty())
		return PRL_ERR_UNEXPECTED;

	dst_.getUsbPreferences()->fromString(u.toString());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Shell

void Shell::react(virConnectPtr link_, virNodeDevicePtr device_, int event_, int detail_, void* opaque_)
{
	Q_UNUSED(link_);
	Q_UNUSED(event_);
	Q_UNUSED(detail_);

	const char* n = virNodeDeviceGetName(device_);
	if (NULL != n && QString(n).startsWith("usb_"))
		((Shell* )opaque_)->operator()();
}

void Shell::operator()()
{
	Job* j = new Job(m_bench->getContainer());
	j->setAutoDelete(true);
	m_bench->getPool().start(j);
}

} // namespace Usb

///////////////////////////////////////////////////////////////////////////////
// struct Pci

Pci::Pci(const QWeakPointer<virConnect>& link_, Workbench& bench_):
	m_bench(&bench_), m_link(link_)
{
	CDspLockedPointer<CDspHostInfo> i = m_bench->getContainer().getHostInfo();
	i->adopt(boost::bind(boost::value_factory<CDspHostInfo::pciBin_type>(),
		&m_sentinel, &m_list));
}

Pci::~Pci()
{
	CDspLockedPointer<CDspHostInfo> i = m_bench->getContainer().getHostInfo();
	i->adopt(CDspHostInfo::pciStrategy_type());
}

void Pci::run()
{
	Prl::Expected<QList<CHwGenericPciDevice>, Error::Simple> f =
		Libvirt::Instrument::Agent::Host(m_link.toStrongRef())
			.getAssignablePci();
	if (f.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot read host PCI devices: %s",
			PRL_RESULT_TO_STRING(f.error().code()));
	}
	else
	{
		QMutexLocker g(&m_sentinel);
		m_list = f.value();
	}
	m_bench->getContainer().getHostInfo()->refresh(CDspHostInfo::uhiPci);
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(const QWeakPointer<virConnect>& link_): m_bench(), m_link(link_)
{
	d_ptr->sendChildEvents = false;
}

Unit::Unit(const QWeakPointer<virConnect>& link_, Workbench& bench_):
	m_bench(&bench_), m_pci(new Pci(link_, bench_)), m_link(link_)
{
	m_pci->setAutoDelete(false);
	d_ptr->sendChildEvents = false;
}

Unit* Unit::clone() const
{
	Unit* output = new Unit(m_link);
	output->m_pci = m_pci;
	output->m_bench = m_bench;

	return output;
}

void Unit::react()
{
	QSharedPointer<virConnect> x = m_link.toStrongRef();
	if (!x.isNull())
	{
		m_bench->getPool().start(m_pci.data());
		Launcher(this->thread())(*this);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Launcher

void Launcher::operator()(const QWeakPointer<virConnect>& link_, Workbench& bench_) const
{
	Unit* m = new Unit(link_, bench_);
	do_(*m, 0);
}

void Launcher::operator()(const Unit& monitor_) const
{
	Unit* m = monitor_.clone();
	if (NULL != m)
		do_(*m, 5 * 60 * 1000);
}

void Launcher::do_(Unit& object_, int timeout_) const
{
	QTimer* t = new QTimer();
	t->moveToThread(m_target);
	t->setInterval(timeout_);

	object_.moveToThread(m_target);
	object_.setParent(t);
	object_.connect(t, SIGNAL(timeout()), SLOT(react()));
	t->connect(t, SIGNAL(timeout()), SLOT(deleteLater()));
	QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection);
}

} // namespace Hardware
} // namespace Monitor

///////////////////////////////////////////////////////////////////////////////
// struct Workbench

Workbench::Workbench(CDspService& container_, Registry::Actual& registry_):
	m_container(&container_), m_registry(&registry_), m_pool(new QThreadPool())
{
}

///////////////////////////////////////////////////////////////////////////////
// struct Host

void Host::run()
{
	Workbench B(*CDspService::instance(), *m_registry);
	Monitor::Link a;
	Monitor::Domains b(B);
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
	B.getPool().waitForDone();
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

void Maintenance::emitRestored()
{
	VIRTUAL_MACHINE_STATE s;
	Result e = State(getDomain()).getValue(s);
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot get the restored VM state");
		return;
	}
	switch (s)
	{
	case VMS_STOPPED:
		return emitLifecycle(VIR_DOMAIN_EVENT_STOPPED,
			VIR_DOMAIN_EVENT_STOPPED_FROM_SNAPSHOT);
	case VMS_PAUSED:
		return emitLifecycle(VIR_DOMAIN_EVENT_SUSPENDED,
			VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT);
	case VMS_RUNNING:
		return emitLifecycle(VIR_DOMAIN_EVENT_STARTED,
			VIR_DOMAIN_EVENT_STARTED_FROM_SNAPSHOT);
	default:
		WRITE_TRACE(DBG_FATAL, "The restored VM state %s is not "
			"eligible for a state event emission",
			PRL_VM_STATE_TO_STRING(s));
	}
}

void Maintenance::emitQemuUpdated()
{
	emitLifecycle(VIR_DOMAIN_EVENT_STARTED, -1);
}

void Maintenance::emitAgentCorollary(PRL_VM_TOOLS_STATE state_)
{
	Callback::g_access.setOpaque(Callback::Mock::ID,
		new Callback::Transport::Event(boost::bind(&Model::Coarse::show, _1,
			boost::bind(&domainReference_type::data, getDomain()),
			boost::protect(boost::bind(&Registry::Reactor::updateAgent, _1, state_)))));
}

} // namespace Limb
} // namespace Vm
} // namespace Agent
} // namespace Instrument
} // namespace Libvirt
