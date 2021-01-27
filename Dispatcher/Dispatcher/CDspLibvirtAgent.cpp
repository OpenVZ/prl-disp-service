/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2020 Virtuozzo International GmbH. All rights reserved.
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

#include "CDspLibvirt.h"
#include "CDspLibvirtExec.h"
#include "CDspService.h"
#include "Stat/CDspStatStorage.h"
#include "Libraries/Transponster/NetFilter.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/combine.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/scope/let.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Libraries/Transponster/Direct.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <boost/phoenix/core/argument.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <Libraries/Transponster/Reverse.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <Libraries/PrlNetworking/netconfig.h>
#include <boost/phoenix/scope/local_variable.hpp>
#include <boost/phoenix/bind/bind_function_object.hpp>
#include "CDspLibvirt_p.h"
#include <vzctl/libvzctl.h>

Q_GLOBAL_STATIC(QMutex, getBoostJsonLock);

namespace Libvirt
{
Instrument::Agent::Hub Kit;

///////////////////////////////////////////////////////////////////////////////
// struct Failure

Failure::Failure(PRL_RESULT result_): Error::Simple(fabricate(result_))
{
#if (LIBVIR_VERSION_NUMBER > 1000004)
	const char* m = virGetLastErrorMessage();
	WRITE_TRACE(DBG_FATAL, "libvirt error %s", m ? : "unknown");
	details() = m;
#endif
}

PRL_RESULT Failure::fabricate(PRL_RESULT result_)
{
	if (result_ != PRL_ERR_FAILURE)
		return result_;

	virErrorPtr err = virGetLastError();
	if (NULL == err)
		return result_;

	switch(err->code)
	{
	case VIR_ERR_OPERATION_INVALID:
		return PRL_ERR_INVALID_ACTION_REQUESTED;
	case VIR_ERR_UNKNOWN_HOST:
		return PRL_ERR_CANT_RESOLVE_HOSTNAME;
	case VIR_ERR_OPERATION_FAILED:
		return PRL_ERR_OPERATION_FAILED;
	case VIR_ERR_DOM_EXIST:
		return PRL_ERR_VM_CONFIG_ALREADY_EXISTS;
	case VIR_ERR_OPERATION_DENIED:
		return PRL_ERR_ACCESS_DENIED;
	case VIR_ERR_NO_NETWORK:
		return PRL_NET_VIRTUAL_NETWORK_NOT_FOUND;
	case VIR_ERR_OPERATION_TIMEOUT:
		return PRL_ERR_TIMEOUT;
	case VIR_ERR_NO_DOMAIN_SNAPSHOT:
		return PRL_ERR_VM_SNAPSHOT_NOT_FOUND;
	case VIR_ERR_OPERATION_ABORTED:
		return PRL_ERR_OPERATION_WAS_CANCELED;
	case VIR_ERR_AGENT_UNRESPONSIVE:
	case VIR_ERR_AGENT_UNSYNCED:
		return PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE;
	}

	return result_;
}

namespace Agent
{
///////////////////////////////////////////////////////////////////////////////
// struct Failure

Failure::Failure(PRL_RESULT result_): Libvirt::Failure(result_),
	m_mainCode(), m_extraCode()
{
	virErrorPtr err = virGetLastError();
	if (err)
	{
		m_mainCode = err->code;
		m_extraCode = err->int1;
	}
}

bool Failure::isTransient() const
{
	return m_mainCode == VIR_ERR_AGENT_UNRESPONSIVE ||
	       m_mainCode == VIR_ERR_AGENT_UNSYNCED;
}

}

namespace Instrument
{
namespace Agent
{
typedef Prl::Expected<void, ::Libvirt::Agent::Failure> doResult_type;

template<class T, class U>
static doResult_type do_(T* handle_, U action_)
{
	if (NULL == handle_)
		return ::Libvirt::Agent::Failure(PRL_ERR_UNINITIALIZED);

	if (0 <= action_(handle_))
		return doResult_type();

	return ::Libvirt::Agent::Failure(PRL_ERR_FAILURE);
}

namespace Vm
{
namespace Migration
{
///////////////////////////////////////////////////////////////////////////////
// struct Agent

Result Agent::cancel()
{
	return do_(getDomain().data(), boost::bind(&virDomainAbortJob, _1));
}

Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple>
Agent::getProgress()
{
	virDomainJobInfo j;
	Result x = do_(getDomain().data(), boost::bind(&virDomainGetJobInfo, _1, &j));
	if (x.isFailed())
		return x.error();

	return std::make_pair(j.dataTotal, j.dataRemaining);
}

Result Agent::setDowntime(quint32 value_)
{
	return do_(getDomain().data(), boost::bind(&virDomainMigrateSetMaxDowntime, _1, 1000*value_, 0));
}

Result Agent::migrate(const CVmConfiguration& config_, unsigned int flags_,
	Parameters::Builder& parameters_)
{
	Result e;
	Basic b(config_, parameters_);
	e = b.addName();
	if (e.isFailed())
		return e;

	e = b.addXml(VIR_MIGRATE_PARAM_DEST_XML,
		Config(getDomain(), getLink(), VIR_DOMAIN_XML_MIGRATABLE));
	if (e.isFailed())
		return e;

	e = b.addXml(VIR_MIGRATE_PARAM_PERSIST_XML,
		Config(getDomain(), getLink(), VIR_DOMAIN_XML_INACTIVE | VIR_DOMAIN_XML_MIGRATABLE));
	if (e.isFailed())
		return e;

	if (getDomain().isNull())
		return Result(Error::Simple(PRL_ERR_UNINITIALIZED));

	Parameters::Result_type p = parameters_.extract();
	if (0 == (VIR_MIGRATE_PEER2PEER & flags_))
	{
		// shared to use cleanup callback only
		QSharedPointer<virConnect> c(virConnectOpen(qPrintable(m_uri)),
				virConnectClose);
		if (c.isNull())
			return Failure(PRL_ERR_FAILURE);

		virDomainPtr d = virDomainMigrate3(getDomain().data(), c.data(),
					p.first.data(), p.second, flags_);
		if (NULL == d)
			return Failure(PRL_ERR_FAILURE);

		virDomainFree(d);

		return Result();
	}
	return do_(getDomain().data(), boost::bind(&virDomainMigrateToURI3, _1,
		qPrintable(m_uri),p.first.data(), p.second, flags_));
}

///////////////////////////////////////////////////////////////////////////////
// struct Offline

Result Offline::operator()(const CVmConfiguration& config_)
{
	Parameters::Builder b;
	return migrate(config_, VIR_MIGRATE_PERSIST_DEST |
			VIR_MIGRATE_CHANGE_PROTECTION |
			VIR_MIGRATE_OFFLINE, b);
}

namespace Online
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

void Flavor::setDeep()
{
	m_custom = m_custom & ~VIR_MIGRATE_NON_SHARED_INC;
	m_custom = m_custom | VIR_MIGRATE_NON_SHARED_DISK;
}

void Flavor::setShallow()
{
	m_custom = m_custom & ~(VIR_MIGRATE_SHARED_WORKAROUND | VIR_MIGRATE_NON_SHARED_DISK |
		VIR_MIGRATE_PEER2PEER);
	m_custom = m_custom | VIR_MIGRATE_NON_SHARED_INC;
}

void Flavor::setShared()
{
	m_custom &= ~VIR_MIGRATE_NON_SHARED_DISK;
}

quint32 Flavor::getResult() const
{
	return VIR_MIGRATE_PERSIST_DEST | VIR_MIGRATE_CHANGE_PROTECTION |
		VIR_MIGRATE_LIVE | VIR_MIGRATE_AUTO_CONVERGE | m_custom;
}

///////////////////////////////////////////////////////////////////////////////
// struct Agent

Agent::Agent(const Migration::Agent& agent_): Migration::Agent(agent_),
	m_compression(new Compression())
{
}

void Agent::setQemuState(qint32 port_)
{
	m_qemuState = QSharedPointer<Qemu::State>(new Qemu::State(port_));
}

void Agent::setQemuDisk(const QList<CVmHardDisk* >& list_, qint32 port_)
{
	m_qemuDisk = QSharedPointer<Qemu::Disk>(new Qemu::Disk(port_, list_));
}

void Agent::setQemuDisk(const QList<CVmHardDisk* >& list_)
{
	m_qemuDisk = QSharedPointer<Qemu::Disk>(new Qemu::Disk(list_));
}

void Agent::setBandwidth(quint64 value_)
{
	m_bandwidth = QSharedPointer<Bandwidth>(new Bandwidth(value_));
}

Result Agent::migrate(const CVmConfiguration& config_, const flavor_type& flavor_)
{
	typedef QList<boost::function1<Result, Parameters::Builder&> > directorList_type;
	directorList_type d;
	if (!m_qemuDisk.isNull())
		d << boost::bind<Result>(boost::ref(*m_qemuDisk.data()), _1);

	if (!m_qemuState.isNull())
		d << boost::bind<Result>(boost::ref(*m_qemuState.data()), _1);

	if (!m_compression.isNull())
		d << boost::bind<Result>(boost::ref(*m_compression.data()), _1);

	if (!m_bandwidth.isNull())
		d << boost::bind<Result>(boost::ref(*m_bandwidth.data()), _1);

	Parameters::Builder b;
	foreach(directorList_type::value_type o, d)
	{
		Result e = o(b);
		if (e.isFailed())
			return e.error();
	}
	// 3s is approximation of live downtime requirements in PCS6
	Result e = setDowntime(3);
	if (e.isFailed())
		return e.error();

	return Migration::Agent::migrate(config_, flavor_.getResult(), b);
}

Agent::result_type Agent::operator()(const CVmConfiguration& config_, const flavor_type& flavor_)
{
	Vm::Completion::Migration::feedback_type p(new boost::promise<quint64>());
	boost::future<quint64> v = p->get_future();
	QScopedPointer<Vm::Completion::Migration> g(new Vm::Completion::Migration(getDomain().data(), p));
	int s = virConnectDomainEventRegisterAny(getLink().data(), getDomain().data(),
			VIR_DOMAIN_EVENT_ID_JOB_COMPLETED,
			VIR_DOMAIN_EVENT_CALLBACK(Vm::Completion::Migration::react),
			g.data(), &Callback::Plain::delete_<Vm::Completion::Migration>);
	if (-1 == s)
		return Failure(PRL_ERR_FAILURE);

	g.take();
	Result e = migrate(config_, flavor_);
	if (e.isFailed())
	{
		virConnectDomainEventDeregisterAny(getLink().data(), s);
		return e.error();
	}
	quint64 output = v.get();
	virConnectDomainEventDeregisterAny(getLink().data(), s);
	return output;
};

} // namespace Online
} // namespace Migration

namespace Limb
{
///////////////////////////////////////////////////////////////////////////////
// struct Abstract

Abstract::linkReference_type Abstract::getLink() const
{
	virConnectPtr x = virDomainGetConnect(m_domain.data());
	if (NULL == x)
		return linkReference_type();

	virConnectRef(x);
	return linkReference_type(x, &virConnectClose);
}

void Abstract::setDomain(virDomainPtr value_)
{
	m_domain = QSharedPointer<virDomain>(value_, &virDomainFree);
}

///////////////////////////////////////////////////////////////////////////////
// struct State

Result State::kill()
{
	doResult_type output = do_(getDomain().data(), boost::bind(&virDomainDestroy, _1));
	if (output.isFailed())
	{
		if (VIR_ERR_SYSTEM_ERROR == output.error().getMainCode() &&
			output.error().getExtraCode() == EBUSY)
			output = ::Libvirt::Agent::Failure(PRL_ERR_TIMEOUT);
	}

	return output;
}

Result State::shutdown()
{
	return do_(getDomain().data(), boost::bind
		(&virDomainShutdownFlags, _1, VIR_DOMAIN_SHUTDOWN_ACPI_POWER_BTN |
			VIR_DOMAIN_SHUTDOWN_GUEST_AGENT));
}

Result State::start_(unsigned int flags_)
{
	int s = VIR_DOMAIN_NOSTATE;
	if (-1 == virDomainGetState(getDomain().data(), &s, NULL, 0))
		return Failure(PRL_ERR_VM_GET_STATUS_FAILED);

	if (s == VIR_DOMAIN_CRASHED)
		kill();

	return do_(getDomain().data(), boost::bind(&virDomainCreateWithFlags, _1, flags_));
}

Result State::start()
{
	return start_(VIR_DOMAIN_START_FORCE_BOOT);
}

Result State::startPaused()
{
	return start_(VIR_DOMAIN_START_PAUSED);
}

Result State::reboot()
{
	return do_(getDomain().data(), boost::bind(&virDomainReboot, _1, 0));
}

Result State::reset()
{
	return do_(getDomain().data(), boost::bind(&virDomainReset, _1, 0));
}

Result State::resume(const QString& sav_)
{
	return do_(getLink().data(), boost::bind
		(&virDomainRestore, _1, qPrintable(sav_)));
}

Result State::pause()
{
	return do_(getDomain().data(), boost::bind(&virDomainSuspend, _1));
}

Result State::unpause()
{
	return do_(getDomain().data(), boost::bind(&virDomainResume, _1));
}

Result State::suspend(const QString& sav_)
{
	return do_(getDomain().data(), boost::bind
		(&virDomainSaveFlags, _1, qPrintable(sav_), (const char* )NULL,
			VIR_DOMAIN_SAVE_RUNNING | VIR_DOMAIN_SAVE_BYPASS_CACHE));
}

Result State::undefine()
{
	return do_(getDomain().data(), boost::bind(&virDomainUndefineFlags, _1,
		VIR_DOMAIN_UNDEFINE_SNAPSHOTS_METADATA |
		VIR_DOMAIN_UNDEFINE_KEEP_NVRAM));
}

Result State::getValue(VIRTUAL_MACHINE_STATE& dst_) const
{
	int s = VIR_DOMAIN_NOSTATE, r;
	if (-1 == virDomainGetState(getDomain().data(), &s, &r, 0))
		return Failure(PRL_ERR_VM_GET_STATUS_FAILED);

	switch (s)
	{
	case VIR_DOMAIN_RUNNING:
		dst_ = VMS_RUNNING;
		break;
	case VIR_DOMAIN_PAUSED:
	case VIR_DOMAIN_PMSUSPENDED:
		dst_ = VMS_PAUSED;
		break;
	case VIR_DOMAIN_CRASHED:
		if (r == VIR_DOMAIN_CRASHED_PANICKED)
		{
			dst_ = VMS_PAUSED;
			break;
		}
	case VIR_DOMAIN_SHUTDOWN:
		// is being shutdown, but not yet stopped
		dst_ = VMS_RUNNING;
		break;
	case VIR_DOMAIN_SHUTOFF:
		dst_ = VMS_STOPPED;
		break;
	default:
		dst_ = VMS_UNKNOWN;
	}
	return Result();
}

Migration::Agent State::migrate(const QString &uri_)
{
	return Migration::Agent(getDomain(), uri_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Maintenance

Result Maintenance::updateQemu()
{
	Result output = do_(getDomain().data(), boost::bind(&virDomainMigrateToURI3, _1,
			(const char *)NULL, (virTypedParameterPtr) NULL, 0,
			VIR_MIGRATE_PEER2PEER | VIR_MIGRATE_LOCAL |
			VIR_MIGRATE_LIVE | VIR_MIGRATE_POSTCOPY |
			VIR_MIGRATE_POSTCOPY_START | VIR_MIGRATE_RELEASE_RAM));
	if (output.isSucceed())
		emitQemuUpdated();

	return output;
}

Result Maintenance::adjustClock(qint64 offset_)
{
	return Config(getDomain(), getLink(), VIR_DOMAIN_XML_INACTIVE)
		.alter(Transponster::Vm::Reverse::Clock(offset_));
}

} // namespace Limb

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virDomainPtr domain_): Abstract(domainReference_type())
{
	setDomain(domain_);
}

Result Unit::rename(const QString& to_)
{
	return do_(getDomain().data(), boost::bind(&virDomainRename, _1,
		qPrintable(to_), 0));
}

Result Unit::getConfig(CVmConfiguration& dst_, bool runtime_) const
{
	Config config(getDomain(), getLink(), runtime_ ? 0 : VIR_DOMAIN_XML_INACTIVE);
	Result output(config.convert(dst_));
	if (output.isFailed())
		return output;

	CDspService* s = CDspService::instance();
	dst_.getVmIdentification()
		->setServerUuid(s->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid());
	CDspLockedPointer<CDspHostInfo> i(s->getHostInfo());
	if (!i.isValid())
		return output;

	const QList<CHwGenericPciDevice* >& a =
		i->data()->getGenericPciDevices()->m_lstGenericPciDevice;
	foreach (CVmGenericPciDevice* d, dst_.getVmHardwareList()->m_lstGenericPciDevices)
	{
		QString m(d->getSystemName().append(":"));
		foreach (CHwGenericPciDevice* g, a)
		{
			if (g->getDeviceId().startsWith(m))
				d->setUserFriendlyName(g->getDeviceName());
		}
	}
	return output;
}

Result Unit::getConfig(QString& dst_, bool runtime_) const
{
	Config config(getDomain(), getLink(), runtime_ ? 0 : VIR_DOMAIN_XML_INACTIVE);
	Prl::Expected<QString, Error::Simple> s = config.read();
	if (s.isFailed())
		return s.error();
	dst_ = s.value();
	return Result();
}

Result Unit::setConfig(const CVmConfiguration& value_)
{
	linkReference_type link_ = getLink();
	if (link_.isNull())
		return Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER);

	Config config(getDomain(), link_, VIR_DOMAIN_XML_INACTIVE);
	Prl::Expected<QString, Error::Simple> x = config.mixup(value_);
	if (x.isFailed())
		return x.error();

	Libvirt::Instrument::Agent::Filter::List filter_list(link_);
	Result ret = filter_list.define(value_.getVmHardwareList()->m_lstNetworkAdapters);

	virDomainPtr d = virDomainDefineXML(link_.data(), qPrintable(x.value()));
	if (NULL == d)
		return Failure(PRL_ERR_VM_APPLY_CONFIG_FAILED);

	setDomain(d);

	return ret;
}

Result Unit::completeConfig(CVmConfiguration& config_)
{
	if (getDomain().isNull())
		return Result(Error::Simple(PRL_ERR_UNINITIALIZED));
	foreach(CVmHardDisk *d, config_.getVmHardwareList()->m_lstHardDisks)
	{
		if (d->getEmulatedType() != PVE::HardDiskImage)
			continue;
		virDomainBlockInfo b;
		if (virDomainGetBlockInfo(getDomain().data(), QSTR2UTF8(d->getSystemName()),
			&b, 0) == 0)
		{
			d->setSize(b.capacity >> 20);
			d->setSizeOnDisk(b.physical >> 20);
		}
	}
	return Result();
}

Result Unit::getUuid(QString& dst_) const
{
	char u[VIR_UUID_STRING_BUFLEN] = {};
	if (virDomainGetUUIDString(getDomain().data(), u))
		return Failure(PRL_ERR_FAILURE);

	PrlUuid x(u);
	dst_ = x.toString(PrlUuid::WithBrackets).c_str();
	return Result();
}

List Unit::up() const
{
	QSharedPointer<virConnect> x = getLink();
	return x.isNull() ? Kit.vms() : List(x);
}

Guest Unit::getGuest() const
{
	return Guest(getDomain());
}

Editor Unit::getRuntime() const
{
	return Editor(getDomain(), VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE);
}

Editor Unit::getEditor() const
{
	return Editor(getDomain(), VIR_DOMAIN_AFFECT_CONFIG);
}

Result Unit::setMemoryStatsPeriod(qint64 seconds_)
{
	return do_(getDomain().data(), boost::bind(&virDomainSetMemoryStatsPeriod, _1,
		seconds_, VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE));
}

Snapshot::List Unit::getSnapshot() const
{
	return Snapshot::List(getDomain());
}

Limb::State Unit::getState() const
{
	return Limb::State(getDomain());
}

Limb::Maintenance Unit::getMaintenance() const
{
	return Limb::Maintenance(getDomain());
}

Block::Launcher Unit::getVolume() const
{
	return Block::Launcher(getDomain());
}

Block::Export Unit::getExport() const
{
	return Block::Export(getDomain());
}

namespace Performance
{

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(const virDomainStatsRecordPtr record_, const pin_type& pin_):
	m_pin(pin_), m_record(record_)
{
	if (NULL == record_)
		return;

	const char* s = NULL;

	unsigned nets = 0;
	virTypedParamsGetUInt(record_->params, record_->nparams, "net.count", &nets);
	for(unsigned i = 0; i < nets; ++i)
	{
		QString t = QString("net.%1.name").arg(i);
		if (1 == virTypedParamsGetString(record_->params, record_->nparams, qPrintable(t), &s))
			m_ifaces[QString(s)] = i;
	}

	unsigned disks = 0;
	virTypedParamsGetUInt(record_->params, record_->nparams, "block.count", &disks);
	for(unsigned i = 0; i < disks; ++i)
	{
		QString t = QString("block.%1.path").arg(i);
		if (1 == virTypedParamsGetString(record_->params, record_->nparams, qPrintable(t), &s))
			m_disks[QString(s)] = i;
	}
}

Result Unit::getUuid(QString& dst_) const
{
	virDomainRef(m_record->dom);
	return Vm::Unit(m_record->dom).getUuid(dst_);
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Unit::getCpu() const
{
	quint64 s = 0;
	Stat::CounterList_type r;
	if (getValue("cpu.time", s))
		r.append(Stat::Counter_type(::Stat::Name::Cpu::getName(), s / 1000));

	return r;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Unit::getVCpuList() const
{
	Stat::CounterList_type r;

	unsigned count = 0;
	if (1 != virTypedParamsGetUInt(m_record->params, m_record->nparams, "vcpu.current", &count))
	{
		WRITE_TRACE(DBG_DEBUG, "no vcpu statistics");
		return r;
	}

	for (unsigned i = 0; i < count; i++)
	{
		quint64 time;
		if (getValue(QString("vcpu.%1.time").arg(i), time))
			r.append(Stat::Counter_type(::Stat::Name::VCpu::getName(i), time));
	}
	return r;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Unit::getDisk(const CVmHardDisk& disk_) const
{
	Stat::CounterList_type r;

	if (!m_disks.contains(disk_.getSystemName()))
	{
		WRITE_TRACE(DBG_DEBUG, "no statistics for %s", qPrintable(disk_.getSystemName()));
		return r;
	}

	unsigned n = m_disks[disk_.getSystemName()];
	QString block = QString("block.%1.%2").arg(n);

	quint64 value = 0;
	if (getValue(block.arg("wr.reqs"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getWriteRequests(disk_), value));

	if (getValue(block.arg("wr.bytes"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getWriteTotal(disk_), value));

	if (getValue(block.arg("rd.reqs"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getReadRequests(disk_), value));

	if (getValue(block.arg("rd.bytes"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getReadTotal(disk_), value));

	if (getValue(block.arg("capacity"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getCapacity(disk_), value));

	if (getValue(block.arg("allocation"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getAllocation(disk_), value));

	if (getValue(block.arg("physical"), value))
		r.append(Stat::Counter_type(::Stat::Name::Hdd::getPhysical(disk_), value));

	return r;
}

Stat::CounterList_type Unit::getMemory() const
{
	quint64 v = 0;
	Stat::CounterList_type output;
	if (getValue("balloon.current", v = 0))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getBalloonActual(), v));
	}
	if (getValue("balloon.swap_in", v = 0))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getSwapIn(), v));
	}
	if (getValue("balloon.swap_out", v = 0))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getSwapOut(), v));
	}
	if (getValue("balloon.minor_fault", v = 0))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getMinorFault(), v));
	}
	if (getValue("balloon.major_fault", v = 0))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getMajorFault(), v));
	}
	quint64 total = 0;
	if (getValue("balloon.maximum", total))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getTotal(), total));
	}
	if (getValue("balloon.usable", v = 0))
	{
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getAvailable(), v));
	}
	if (getValue("balloon.unused", v = 0))
	{
		// new balloon: used = maximum - unused
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getUsed(), total - v));
	}
	else if (getValue("balloon.rss", v = 0))
	{
		// old balloon: used = rss
		output.append(Stat::Counter_type(
			::Stat::Name::Memory::getUsed(), v));
	}
	return output;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Unit::getInterface(const CVmGenericNetworkAdapter& iface_) const
{
	Stat::CounterList_type r;

	if (!m_ifaces.contains(iface_.getHostInterfaceName()))
	{
		WRITE_TRACE(DBG_DEBUG, "no statistics for %s", qPrintable(iface_.getHostInterfaceName()));
		return r;
	}

	unsigned n = m_ifaces[iface_.getHostInterfaceName()];
	QString iface = QString("net.%1.%2").arg(n);

	quint64 value = 0;
	if (getValue(iface.arg("rx.bytes"), value))
		r.append(Stat::Counter_type(::Stat::Name::Interface::getBytesIn(iface_), value));

	if (getValue(iface.arg("rx.pkts"), value))
		r.append(Stat::Counter_type(::Stat::Name::Interface::getPacketsIn(iface_), value));

	if (getValue(iface.arg("tx.bytes"), value))
		r.append(Stat::Counter_type(::Stat::Name::Interface::getBytesOut(iface_), value));

	if (getValue(iface.arg("tx.pkts"), value))
		r.append(Stat::Counter_type(::Stat::Name::Interface::getPacketsOut(iface_), value));

	return r;
}

bool Unit::getValue(const QString& name_, quint64& dst_) const
{
	return 1 == virTypedParamsGetULLong(m_record->params, m_record->nparams, qPrintable(name_), &dst_);
}

} // namespace Performance

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
//struct Future

Result Future::wait(int timeout_)
{
	if (m_status.empty())
		return Result();

	boost::property_tree::ptree c, r;
	c.put("execute","query-status");
	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, c, false);
	QString cmd = QString::fromUtf8(ss.str().c_str());

	// Wait befor migration is over and unpause VM
	Exec::Waiter w;
	do
	{
		w.wait(timeout_ >= 100 ? 100 : timeout_);
		timeout_ -= 100;

		Prl::Expected<QString, Error::Simple> res = m_guest.execute(cmd, false);
		if (res.isFailed())
			continue;
		QString state = res.value();

		std::istringstream is(state.toUtf8().data());

		// read_json is not thread safe
		QMutexLocker locker(getBoostJsonLock());
		try {
			boost::property_tree::json_parser::read_json(is, r);
		} catch (const boost::property_tree::json_parser::json_parser_error&) {
			WRITE_TRACE(DBG_DEBUG, "unable to parse query-status result: %s", qPrintable(state));
			return Error::Simple(PRL_ERR_FAILURE);
		}
		std::string status = r.get<std::string>("return.status", std::string("none"));

		// Is state changed?
		if (!r.get<bool>("return.running", false) && m_status != status)
		{
			WRITE_TRACE(DBG_DEBUG, "query-status result: %s", qPrintable(state));
			m_status.clear();
			return res;
		}
	}
	while(timeout_ > 0);

	return Error::Simple(PRL_ERR_TIMEOUT);
}

} // namespace Command

///////////////////////////////////////////////////////////////////////////////
// struct Screenshot

PRL_RESULT Screenshot::save(const QString& to_, const QString& format_) const
{
	if (format_ != "PNG")
		return PRL_ERR_UNIMPLEMENTED;

	// Convert to PNG and attach
	QStringList cmds("pnmtopng");
	if (!m_size.isNull())
	{
		QString s("pamscale");
		if (m_size.width())
			s.append(QString(" -width %1").arg(m_size.width()));
		if (m_size.height())
			s.append(QString(" -height %1").arg(m_size.height()));
		cmds.prepend(s);
	}

	Instrument::Pipeline::result_type r = Instrument::Pipeline(cmds)(m_data, to_);
	if (r.isFailed()) {
		WRITE_TRACE(DBG_FATAL, "Failed to convert to PNG, command '%s' is failed",
				QSTR2UTF8(r.error()));
		return PRL_ERR_FAILURE;
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Guest

Result Guest::traceEvents(bool enable_)
{
	return execute(QString("trace-event * %1").arg(enable_ ? "on" : "off"));
}

Prl::Expected<Screenshot, Error::Simple> Guest::dumpScreen()
{
	QTemporaryFile a;
	if (!a.open())
		return Failure(PRL_ERR_FAILURE);

	// QEMU generates screenshot in PNM format
	Result e = execute(QString("screendump %1").arg(a.fileName()));
	if (e.isFailed())
		return e.error();

	return Screenshot(a.readAll());
}

Prl::Expected<QString, Error::Simple>
Guest::dumpMemory(const QString& path)
{
	return execute(QString("dump-guest-memory -a %1").arg(path));
}

Prl::Expected<Command::Future, Error::Simple>
Guest::dumpState(const QString& path_)
{
	int s = VIR_DOMAIN_NOSTATE;

	if (-1 == virDomainGetState(m_domain.data(), &s, NULL, 0))
		return Failure(PRL_ERR_VM_GET_STATUS_FAILED);

	// Cannot get state for stopped VM.
	if (s == VIR_DOMAIN_SHUTDOWN || s == VIR_DOMAIN_SHUTOFF)
		return Error::Simple(PRL_ERR_VM_PROCESS_IS_NOT_STARTED);

	Result res = execute(QString("migrate -s \"exec:gzip -c > %1\"").arg(path_));

	if (res.isFailed())
		return res.error();

	// "finish-migrate" - guest is paused to finish the migration process
	return Command::Future(m_domain, std::string("finish-migrate"));
}

Result Guest::setUserPasswd(const QString& username_, const QString& password_, bool crypted_)
{
	unsigned int f = (crypted_) ? VIR_DOMAIN_CREATE_USER | VIR_DOMAIN_PASSWORD_ENCRYPTED :
					VIR_DOMAIN_CREATE_USER;
	return do_(m_domain.data(), boost::bind
		(&virDomainSetUserPassword, _1, username_.toUtf8().constData(),
			password_.toUtf8().constData(), f));
}

Result Guest::freezeFs()
{
	return do_(m_domain.data(), boost::bind
		(&virDomainFSFreeze, _1, (const char **)NULL, 0, 0));
}

Result Guest::thawFs()
{
	return do_(m_domain.data(), boost::bind
		(&virDomainFSThaw, _1, (const char **)NULL, 0, 0));
}

Prl::Expected<std::pair<bool,bool>, ::Error::Simple>
Guest::authenticate(const QString& username_, const QString& password_)
{
	using namespace boost::property_tree;
	ptree c, a;

	a.put("username", qPrintable(username_));
	a.put("password", qPrintable(password_));
	c.put("execute", "guest-auth-user");
	c.add_child("arguments", a);

	std::ostringstream os;
	json_parser::write_json(os, c);
	
	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		executeInAgent(os.str().c_str());
	if (r.isFailed())
		return r.error();
	std::istringstream is(r.value().toUtf8().data());
	try {
		ptree pt;
		json_parser::read_json(is, pt);

		bool a = pt.get<bool>("return.authenticated");
		bool b = pt.get<bool>("return.is-admin");

		return std::make_pair(a, b);
	} catch (const std::exception &) {
		return Failure(PRL_ERR_FAILURE);
	}
}

Prl::Expected< QList<boost::tuple<quint64,quint64,QString,QString,QString> >, ::Error::Simple>
Guest::getFsInfo()
{
	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		executeInAgent(QString("{\"execute\":\"guest-get-fsinfo\"}"));
	if (r.isFailed())
		return r.error();
	std::istringstream is(r.value().toUtf8().data());
	boost::property_tree::ptree pt;
	QList<boost::tuple<quint64,quint64,QString,QString,QString> > l;
	try {
		boost::property_tree::json_parser::read_json(is, pt);
		BOOST_FOREACH(boost::property_tree::ptree::value_type& v, pt.get_child("return")) {
			uint64_t a = v.second.get<uint64_t>("total");
			uint64_t b = v.second.get<uint64_t>("free");
			QString  c = QString::fromStdString(v.second.get<std::string>("name"));
			QString  d = QString::fromStdString(v.second.get<std::string>("type"));
			QString  e = QString::fromStdString(v.second.get<std::string>("mountpoint"));
			l << boost::make_tuple(a, b, c, d, e);
		}
	} catch (const std::exception &) {
		return Failure(PRL_ERR_FAILURE);
	}
	return l;
}

Result Guest::checkAgent()
{
       return executeInAgent(QString("{\"execute\":\"guest-ping\"}"));
}

Prl::Expected<QString, Libvirt::Agent::Failure>
Guest::getAgentVersion(int retries)
{
	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		executeInAgent(QString("{\"execute\":\"guest-info\"}"), retries);
	if (r.isFailed())
		return r.error();

	std::istringstream is(r.value().toUtf8().data());

	// read_json is not thread safe
	QMutexLocker locker(getBoostJsonLock());
	try {
		boost::property_tree::ptree result;
		boost::property_tree::json_parser::read_json(is, result);
		return QString::fromStdString(result.get<std::string>("return.version"));
	} catch (const std::exception &) {
		return Libvirt::Agent::Failure(PRL_ERR_FAILURE);
	}
}

Prl::Expected<QString, Error::Simple>
Guest::execute(const QString& cmd, bool isHmp)
{
	char* s = NULL;
	if (0 != virDomainQemuMonitorCommand(m_domain.data(),
			cmd.toUtf8().constData(),
			&s,
			isHmp ? VIR_DOMAIN_QEMU_MONITOR_COMMAND_HMP : 0))
	{
		return Failure(PRL_ERR_FAILURE);
	}
	QString reply(s);
	free(s);
	return reply;
}

Prl::Expected<QSharedPointer<Exec::Unit>, Error::Simple>
Guest::getExec() const
{
	if (m_domain.isNull())
		return Error::Simple(PRL_ERR_UNINITIALIZED);

	return QSharedPointer<Exec::Unit>(new Exec::Unit(m_domain));
}

Prl::Expected<Guest::result_type, Error::Simple>
Guest::runProgram(const Exec::Request& request_)
{
	Libvirt::Result e;
	Exec::Unit x(m_domain);
	e = x.start(request_);
	if (e.isFailed())
		return e.error();

	e = x.wait();
	if (e.isFailed())
		return e.error();

	if (x.getResult().isFailed())
		return Error::Simple(x.getResult().error());

	result_type output(x.getResult().value());
	output.stdOut = x.getStdout()->readAll();
	output.stdErr = x.getStderr()->readAll();

	return output;
}

Prl::Expected<QString, Libvirt::Agent::Failure>
Guest::executeInAgent(const QString& cmd, int retries)
{
	if (m_domain.isNull())
		return Libvirt::Agent::Failure(PRL_ERR_UNINITIALIZED);

	char *s;
	
	/*
	 * Retry unsynced guest agent error.
	 * This error was returned when agent did not response in time
	 * for previous command and host-guest virtio serial
	 * contains data from previous operation.
	 * Usually the second or third command consumes unexpected data
	 * and the agent becomes in sync and successfully completes
	 * next command.
	 * Requires patched libvirt (with VIR_ERR_AGENT_UNSYNCED err code).
	 */
	for (int i = 0; ;) {
		WRITE_TRACE(DBG_INFO, "executeInAgent: %s", qPrintable(cmd));

		s = virDomainQemuAgentCommand(m_domain.data(),
				qPrintable(cmd), 30, 0);
		if (s != NULL)
			break;

		Libvirt::Agent::Failure r(PRL_ERR_FAILURE);
		if (!r.isTransient())
			return r;

		if (retries != INFINITE && ++i > retries)
			return r;

		HostUtils::Sleep(1000);
	}

	QString reply = QString::fromUtf8(s);
	free(s);
	return reply;
}

namespace Exec
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

void Request::setRunInShell()
{
	m_flags |= VIR_DOMAIN_COMMAND_X_EXEC_SHELL;
}

void Request::setRunInTerminal()
{
	m_flags |= VIR_DOMAIN_COMMAND_X_EXEC_TERMINAL;
}

///////////////////////////////////////////////////////////////////////////////
// struct ReadDevice

ReadDevice::ReadDevice(virStreamPtr stream_):
	m_finished(NULL == stream_), m_stream(stream_, &virStreamFree)
{
}

void ReadDevice::close()
{
	QSharedPointer<virStream> s;
	{
		QMutexLocker l(&m_lock);
		m_finished = true;
		m_stream.swap(s);
		QIODevice::close();
	}
	if (!s.isNull())
	{
		virStreamEventRemoveCallback(s.data());
		virStreamAbort(s.data());
	}
}

void ReadDevice::setEof()
{
	QSharedPointer<virStream> s;
	{
		QMutexLocker l(&m_lock);
		m_finished = true;
		m_stream.swap(s);
	}
	if (!s.isNull())
	{
		virStreamEventRemoveCallback(s.data());
		virStreamFinish(s.data());
		emit readChannelFinished();
	}
}

bool ReadDevice::open(QIODevice::OpenMode mode_)
{
	if (mode_ & QIODevice::WriteOnly)
		return false;

	QMutexLocker l(&m_lock);
	return QIODevice::open(mode_);
}

bool ReadDevice::atEnd()
{
	QMutexLocker l(&m_lock);
	return (m_finished && m_data.isEmpty());
}

qint64 ReadDevice::bytesAvailable()
{
	QMutexLocker l(&m_lock);
	return m_data.size() + QIODevice::bytesAvailable();
}

void ReadDevice::appendData(const QByteArray &data_)
{
	QMutexLocker l(&m_lock);
	m_data.append(data_);
	l.unlock();
	emit readyRead();
}

qint64 ReadDevice::readData(char *data_, qint64 maxSize_)
{
	QMutexLocker l(&m_lock);
	quint64 output = qMin<quint64>(maxSize_, m_data.size());
	::memcpy(data_, m_data.constData(), output);
	m_data.remove(0, output);
	return output;
}

qint64 ReadDevice::writeData(const char *data_, qint64 len_)
{
	Q_UNUSED(data_);
	Q_UNUSED(len_);
	return -1;
}

Libvirt::Result ReadDevice::track(Callback* tracker_)
{
	if (NULL == tracker_)
		return Error::Simple(PRL_ERR_INVALID_ARG);

	QMutexLocker l(&m_lock);
	return do_(m_stream.data(), boost::bind(&virStreamEventAddCallback, _1,
		VIR_STREAM_EVENT_HANGUP | VIR_STREAM_EVENT_ERROR | VIR_STREAM_EVENT_READABLE,
		&Callback::react, tracker_, &Libvirt::Callback::Plain::delete_<Callback>));
}

///////////////////////////////////////////////////////////////////////////////
// struct WriteDevice

WriteDevice::WriteDevice(virStreamPtr stream_): m_stream(stream_, &virStreamFree)
{
}

bool WriteDevice::open(QIODevice::OpenMode mode_)
{
	if (mode_ & QIODevice::ReadOnly)
		return false;

	QMutexLocker l(&m_lock);
	return QIODevice::open(mode_);
}

void WriteDevice::close()
{
	QSharedPointer<virStream> s;
	{
		QMutexLocker l(&m_lock);
		m_stream.swap(s);
		QIODevice::close();
	}
	virStreamFinish(s.data());
}

qint64 WriteDevice::readData(char *data_, qint64 maxSize_)
{
	Q_UNUSED(data_);
	Q_UNUSED(maxSize_);
	return -1;
}

qint64 WriteDevice::writeData(const char *data_, qint64 len_)
{
	qint64 output = 0;
	for (qint32 n = 0; output < len_; output += n)
	{
		QSharedPointer<virStream> s;
		{
			QMutexLocker l(&m_lock);
			s = m_stream;
		}
		n = virStreamSend(s.data(), data_ + output, len_ - output);
		if (n < 0)
		{
			virStreamAbort(s.data());
			return n;
		}
	}

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Workbench

int Workbench::calculateTimeout(int index_) const
{
	switch (index_ / 10)
	{
	case 0: 
		return 100;
	case 1: 
		return 1000;
	default:
		return 10000;
	}
}

const char** Workbench::translate(const QStringList& src_, QVector<char* >& dst_) const
{
	if (src_.isEmpty())
		return NULL;

	dst_.resize(src_.size() + 1);
	for (int i = 0; i < src_.size(); ++i)
	{
		dst_[i] = strdup(qPrintable(src_.at(i)));
	}
	return (const char** )dst_.data();
}

///////////////////////////////////////////////////////////////////////////////
// struct Launcher

Launcher::Launcher(const QSharedPointer<virDomain>& domain_):
	m_stdin(), m_stdout(), m_stderr(), m_domain(domain_)
{
}

Prl::Expected<int, Libvirt::Agent::Failure>
Launcher::operator()(const Request& request_) const
{
	Workbench w;
	QVector<char* > a, e;
	int output = virDomainCommandXExec(m_domain.data(),
			qPrintable(request_.getPath()),
			w.translate(request_.getArgs(), a),
			request_.getArgs().size(),
			w.translate(request_.getEnv(), e),
			request_.getEnv().size(), NULL, 0,
			m_stdin, m_stdout, m_stderr,
			request_.getFlags());
	std::for_each(a.begin(), a.end(), &free);
	std::for_each(e.begin(), e.end(), &free);
	if (0 > output)
		return Libvirt::Agent::Failure(PRL_ERR_FAILURE);

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(const QSharedPointer<virDomain>& domain_):
	m_waitFailures(), m_pid(-1), m_result(PRL_ERR_INVALID_HANDLE),
	m_domain(domain_)
{
	virConnectPtr c = virDomainGetConnect(m_domain.data());
	virStreamPtr i = virStreamNew(c, 0);
	if (NULL == i)
		return;

	virStreamPtr o = virStreamNew(c, VIR_STREAM_NONBLOCK);
	if (NULL == o)
	{
		virStreamFree(i);
		return;
	}
	virStreamPtr e = virStreamNew(c, VIR_STREAM_NONBLOCK);
	if (NULL == e)
	{
		virStreamFree(i);
		virStreamFree(o);
		return;
	}
	m_launcher = Launcher(m_domain).setStdin(i).setStdout(o).setStderr(e);
	m_stdin = QSharedPointer<WriteDevice>(new WriteDevice(i));
	m_stdout = QSharedPointer<ReadDevice>(new ReadDevice(o));
	m_stderr = QSharedPointer<ReadDevice>(new ReadDevice(e));
	m_finished = QSharedPointer<QSemaphore>(new QSemaphore());
	m_result = result_type(PRL_ERR_UNINITIALIZED);
}

Libvirt::Result Unit::start(const Request& request_)
{
	if (!m_launcher)
		return Error::Simple(PRL_ERR_INVALID_HANDLE);

	if (!m_stdin->isOpen() && !m_stdin->open(QIODevice::WriteOnly))
		return Error::Simple(PRL_ERR_OPEN_FAILED);

	if (!m_stdout->isOpen() && !m_stdout->open(QIODevice::ReadOnly))
		return Error::Simple(PRL_ERR_OPEN_FAILED);

	if (!m_stderr->isOpen() && !m_stderr->open(QIODevice::ReadOnly))
		return Error::Simple(PRL_ERR_OPEN_FAILED);

	Prl::Expected<int, Libvirt::Agent::Failure> x((*m_launcher)(request_));
	if (x.isFailed())
		return x.error();

	m_pid = x.value();
	m_launcher = boost::none;
	m_stdout->track(new Callback(m_stdout, m_finished));
	m_stderr->track(new Callback(m_stderr, m_finished));

	return Libvirt::Result();
}

Libvirt::Result Unit::wait(int timeout_)
{
	if (m_result.isSucceed())
		return Libvirt::Result();

	if (-1 == m_pid.operator int())
		return Error::Simple(PRL_ERR_INVALID_HANDLE);

	enum { MAX_TRANSIENT_FAILS = 10 };

	if (!m_finished.isNull()) 
	{
		QElapsedTimer timer;
		timer.start();
		if (!m_finished->tryAcquire(2, timeout_))
			return Error::Simple(PRL_ERR_TIMEOUT);
		m_finished.clear();
		if (timeout_ > 0)
			timeout_ = qMax<int>(0, timeout_ - timer.elapsed());
	}

	Waiter w;
	for (int i = 0, t = 0;; ++i)
	{
		Prl::Expected<void, Libvirt::Agent::Failure> x = query();
		if (x.isFailed())
		{
			if (!x.error().isTransient() ||
			    ++m_waitFailures >= MAX_TRANSIENT_FAILS)
				return x.error();
		}
		else
		{
			m_waitFailures = 0;
			if (m_result.isSucceed())
				return Libvirt::Result();
		}
		if (timeout_ == 0)
			return Error::Simple(PRL_ERR_TIMEOUT);

		int y = calculateTimeout(i);
		w.wait(y);
		t += y;
		if (timeout_ != -1 && timeout_ > t)
			return Error::Simple(PRL_ERR_TIMEOUT);
	}
}

void Unit::cancel()
{
	int p = m_pid;
	if (-1 != p)
	{
		do_(m_domain.data(), boost::bind(&virDomainCommandXTerminate,
			_1, p, 0));
	}
}

Prl::Expected<void, ::Libvirt::Agent::Failure> Unit::query()
{
	virDomainCommandXStatus s;
	Instrument::Agent::doResult_type output = do_(m_domain.data(),
		boost::bind(&virDomainCommandXGetStatus, _1, m_pid.operator int(), &s, 0));
	if (output.isSucceed() && s.exited)
	{
		Result x;
		if (-1 != s.signal)
		{
			x.exitcode = s.signal;
			x.exitStatus = QProcess::CrashExit;
		}
		else if (-1 != s.code)
		{
			x.exitcode = s.code;
			x.exitStatus = QProcess::NormalExit;
		}
		m_pid = -1;
		m_result = result_type(x);
		m_stdin->close();
	}

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Callback

void Callback::operator()(virStreamPtr stream_, int events_)
{
	QSharedPointer<ReadDevice> t = m_target.toStrongRef();
	QSharedPointer<QSemaphore> s = m_sem.toStrongRef();
	if (t.isNull() || s.isNull())
	{
		WRITE_TRACE(DBG_FATAL, "The channel target is disconnected");
		return;
	}
	if (events_ & (VIR_STREAM_EVENT_HANGUP | VIR_STREAM_EVENT_ERROR))
	{
		t->close();
		s->release();
		return;
	}
	if (0 == (events_ & VIR_STREAM_EVENT_READABLE))
		return;

	forever
	{
		char b[4096];
		int x = virStreamRecv(stream_, b, sizeof(b));
		// NB: -2 is a magical constant which means no data
		if (-2 == x)
			return;
		if (0 < x)
		{
			t->appendData(QByteArray(b, x));
			if (int(sizeof(b)) > x)
				return;

			continue;
		}

		if (0 == x)
			t->setEof();
		else
			t->close();

		return s->release();
	}
}

void Callback::react(virStreamPtr stream_, int events_, void *opaque_)
{
	(*(Callback* )opaque_)(stream_, events_);
}

}; //namespace Exec

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

Hotplug::Hotplug(const QSharedPointer<virDomain>& domain_):
	m_flags(VIR_DOMAIN_AFFECT_CURRENT | VIR_DOMAIN_AFFECT_LIVE),
	m_domain(domain_)
{
}

Result Hotplug::attach(const QString& device_)
{
	WRITE_TRACE(DBG_DEBUG, "attach device: \n%s", qPrintable(device_));
	return do_(m_domain.data(), boost::bind(virDomainAttachDeviceFlags, _1,
			qPrintable(device_), m_flags));
}

Result Hotplug::detach(const QString& device_)
{
	WRITE_TRACE(DBG_DEBUG, "detach device: \n%s", qPrintable(device_));
	return do_(m_domain.data(), boost::bind(virDomainDetachDeviceFlags, _1,
			qPrintable(device_), m_flags));
}

Result Hotplug::update(const QString& device_)
{
	WRITE_TRACE(DBG_DEBUG, "update device: \n%s", qPrintable(device_));
	return do_(m_domain.data(), boost::bind(virDomainUpdateDeviceFlags, _1,
			qPrintable(device_), m_flags |
			VIR_DOMAIN_DEVICE_MODIFY_FORCE));
}

Hotplug& Hotplug::setApplyConfig(bool value_)
{
	if (value_)
		m_flags |= VIR_DOMAIN_DEVICE_MODIFY_CONFIG;
	else
		m_flags &= ~VIR_DOMAIN_DEVICE_MODIFY_CONFIG;

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// struct Editor

Result Editor::setPerCpuLimit(quint32 limit_, quint32 period_)
{
	return setCpuLimit(0, limit_, period_);
}

Result Editor::setGlobalCpuLimit(quint32 limit_, quint32 period_)
{
	return setCpuLimit(limit_, 0, period_);
}

Result Editor::setCpuLimit(quint32 globalLimit_, quint32 limit_,
		quint32 period_)
{
	Parameters::Builder b;

	if (!b.add(VIR_DOMAIN_SCHEDULER_GLOBAL_PERIOD, static_cast<quint64>(period_)))
		return Failure(PRL_ERR_SET_CPULIMIT);
	if (!b.add(VIR_DOMAIN_SCHEDULER_VCPU_PERIOD, static_cast<quint64>(period_)))
		return Failure(PRL_ERR_SET_CPULIMIT);

	qint64 l(globalLimit_ == 0? static_cast<qint64>(-1) : globalLimit_);
	if (!b.add(VIR_DOMAIN_SCHEDULER_GLOBAL_QUOTA, l))
		return Failure(PRL_ERR_SET_CPULIMIT);

	l = (limit_ == 0? static_cast<qint64>(-1) : limit_);
	if (!b.add(VIR_DOMAIN_SCHEDULER_VCPU_QUOTA, l))
		return Failure(PRL_ERR_SET_CPULIMIT);

	Parameters::Result_type p(b.extract());
	Result r(do_(getDomain().data(), boost::bind(&virDomainSetSchedulerParametersFlags, _1,
							p.first.data(), p.second, m_flags)));
	return r;
}

Result Editor::setIoLimit(const CVmHardDisk& disk_, quint32 limit_)
{
	return setBlockIoTune(disk_, VIR_DOMAIN_BLOCK_IOTUNE_TOTAL_BYTES_SEC, limit_);
}

Result Editor::setIopsLimit(const CVmHardDisk& disk_, quint32 limit_)
{
	return setBlockIoTune(disk_, VIR_DOMAIN_BLOCK_IOTUNE_TOTAL_IOPS_SEC, limit_);
}

Result Editor::setBlockIoTune(const CVmHardDisk& disk_, const char* param_, quint32 limit_)
{
	Parameters::Builder b;

	if (!b.add(param_, static_cast<quint64>(limit_)))
		return Failure(PRL_ERR_SET_IOLIMIT);
	if (!b.add(VIR_DOMAIN_BLOCK_IOTUNE_GROUP_NAME, QString("virtuozzo")))
		return Failure(PRL_ERR_SET_IOLIMIT);

	Parameters::Result_type p(b.extract());
	return Result(do_(getDomain().data(), boost::bind(&virDomainSetBlockIoTune, _1,
			qPrintable(disk_.getTargetDeviceName()),
			p.first.data(), p.second, VIR_DOMAIN_AFFECT_CURRENT |
				VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));
}

Result Editor::setIoPriority(quint32 ioprio_)
{
	virTypedParameterPtr p(NULL);
	qint32 s(0);
	qint32 m(0);

	if (do_(&p, boost::bind(&virTypedParamsAddUInt, _1,
					&s, &m, VIR_DOMAIN_BLKIO_WEIGHT, HostUtils::convertIoprioToWeight(ioprio_))).isFailed())
		return Failure(PRL_ERR_SET_IOPRIO);

	Result r(do_(getDomain().data(), boost::bind(&virDomainSetBlkioParameters, _1,
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));
	virTypedParamsFree(p, s);
	return r;
}


Result Editor::setCpuUnits(quint32 units_)
{
	virTypedParameterPtr p(NULL);
	qint32 s(0);
	qint32 m(0);

	if (do_(&p, boost::bind(&virTypedParamsAddULLong, _1,
					&s, &m, VIR_DOMAIN_SCHEDULER_CPU_SHARES, units_)).isFailed())
		return Failure(PRL_ERR_SET_CPUUNITS);

	Result r(do_(getDomain().data(), boost::bind(&virDomainSetSchedulerParametersFlags, _1,
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));

	virTypedParamsFree(p, s);
	return r;
}

Result Editor::setCpuCount(quint32 units_)
{
	return do_(getDomain().data(), boost::bind(&virDomainSetVcpus, _1, units_));
}

Result Editor::setCpuMask(quint32 ncpus_, const QString& mask_)
{
	unsigned long cpumap[64];

	if (vzctl2_bitmap_parse(qPrintable(mask_), cpumap, sizeof(cpumap)))
		return Failure(PRL_ERR_SET_CPUMASK);

	for (unsigned n = 0; n < ncpus_; ++n)
	{
		if (do_(getDomain().data(),
				boost::bind(&virDomainPinVcpuFlags,
					_1, n, (unsigned char*)cpumap, sizeof(cpumap),
					VIR_DOMAIN_VCPU_LIVE)).isFailed())
			return Failure(PRL_ERR_SET_CPUMASK);
	}

	return Result();
}

Result Editor::setNodeMask(const QString& mask_)
{
	Libvirt::Instrument::Agent::Parameters::Builder param;

	param.add(VIR_DOMAIN_NUMA_NODESET, mask_);
	Parameters::Result_type p = param.extract();

	return do_(getDomain().data(), boost::bind(&virDomainSetNumaParameters,
			_1, p.first.data(), p.second, VIR_DOMAIN_AFFECT_LIVE));
}

Result Editor::setMemGuarantee(const CVmMemory& memory_)
{
	Libvirt::Instrument::Agent::Parameters::Builder param;

	if (memory_.getMemGuaranteeType() == PRL_MEMGUARANTEE_AUTO)
	{
		param.add(VIR_DOMAIN_MEMORY_MIN_GUARANTEE_VZ_AUTO, true);
	}
	else
	{
		quint64 g = memory_.getRamSize();
		g = (g << 10) * memory_.getMemGuarantee() / 100;
		param.add(VIR_DOMAIN_MEMORY_MIN_GUARANTEE, g);
	}

	Parameters::Result_type p = param.extract();
	return do_(getDomain().data(), boost::bind(&virDomainSetMemoryParameters,
			_1, p.first.data(), p.second, VIR_DOMAIN_AFFECT_LIVE));

}

template<class T>
Result Editor::plugAndWait(const T& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<T>
			::getPlugXml(device_);
	if (x.isFailed())
		return x.error();

	Vm::Completion::Hotplug::feedback_type p(new boost::promise<void>());
	boost::future<void> w = p->get_future();
	QScopedPointer<Vm::Completion::Hotplug> g(
		new Vm::Completion::Hotplug(getDomain().data(), QString(), p));
	int m = virConnectDomainEventRegisterAny(getLink().data(),
		getDomain().data(), VIR_DOMAIN_EVENT_ID_DEVICE_ADDED,
		VIR_DOMAIN_EVENT_CALLBACK(Vm::Completion::Hotplug::react),
		g.data(), &Callback::Plain::delete_<Vm::Completion::Hotplug>);
	if (-1 == m)
		return Failure(PRL_ERR_FAILURE);

	g.take();
	Result output = Hotplug(getDomain()).attach(x.value());
	if (output.isSucceed())
		w.get();

	virConnectDomainEventDeregisterAny(getLink().data(), m);
	return output;
}

template<class T>
Result Editor::plug(const T& device_)
{
	return plugAndWait<T>(device_);
}
template Result Editor::plug<CVmHardDisk>(const CVmHardDisk& device_);
template Result Editor::plug<CVmSerialPort>(const CVmSerialPort& device_);
template Result Editor::plug<CVmGenericNetworkAdapter>(const CVmGenericNetworkAdapter& device_);

template<>
Result Editor::plug<Transponster::Vm::Reverse::Dimm>(const Transponster::Vm::Reverse::Dimm& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<Transponster::Vm::Reverse::Dimm>
			::getPlugXml(device_);
	if (x.isFailed())
		return x.error();

	return Hotplug(getDomain()).attach(x.value());
}

template<>
Result Editor::plug<CHwUsbDevice>(const CHwUsbDevice& device_)
{
	typedef Transponster::Vm::Reverse::Usb::Operator transformer_type;
	transformer_type t(device_);
	Result e = Config(getDomain(), getLink(), VIR_DOMAIN_XML_INACTIVE)
		.alter(boost::bind(&transformer_type::plug, &t, _1));
	if (e.isFailed())
		return e;

	e = plugAndWait<CHwUsbDevice>(device_);
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot connect USB device %s to VM %s",
			qPrintable(device_.getDeviceId()),
			virDomainGetName(getDomain().data()));
	}
	return Result();
}

Result Editor::unplugAndWait(const QString& xml_, const QString& alias_)
{
	Vm::Completion::Hotplug::feedback_type p(new boost::promise<void>());
	boost::future<void> w = p->get_future();
	QScopedPointer<Vm::Completion::Hotplug> g(new Vm::Completion::Hotplug
		(getDomain().data(), alias_, p));
	int m = virConnectDomainEventRegisterAny(getLink().data(),
		getDomain().data(), VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED,
		VIR_DOMAIN_EVENT_CALLBACK(Vm::Completion::Hotplug::react),
		g.data(), &Callback::Plain::delete_<Vm::Completion::Hotplug>);
	if (-1 == m)
		return Failure(PRL_ERR_FAILURE);

	g.take();
	Result output = Hotplug(getDomain()).detach(xml_);
	if (output.isSucceed())
		w.get();

	virConnectDomainEventDeregisterAny(getLink().data(), m);
	return output;
}

template<class T>
Result Editor::unplug(const T& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<T>
			::getPlugXml(device_);
	if (x.isFailed())
		return x.error();

	return unplugAndWait(x.value(), device_.getAlias());
}
template Result Editor::unplug<CVmHardDisk>(const CVmHardDisk& device_);
template Result Editor::unplug<CVmSerialPort>(const CVmSerialPort& device_);
template Result Editor::unplug<CVmGenericNetworkAdapter>(const CVmGenericNetworkAdapter& device_);

template<>
Result Editor::unplug<CHwUsbDevice>(const CHwUsbDevice& device_)
{
	Result e = unplugAndWait(Transponster::Vm::Reverse::Device<CHwUsbDevice>::getPlugXml(device_),
			Transponster::Vm::Reverse::Device<CHwUsbDevice>::getAlias(device_));
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot disconnect USB device %s from VM %s",
			qPrintable(device_.getDeviceId()),
			virDomainGetName(getDomain().data()));
	}
	typedef Transponster::Vm::Reverse::Usb::Operator transformer_type;
	transformer_type t(device_);
	return Config(getDomain(), getLink(), VIR_DOMAIN_XML_INACTIVE)
		.alter(boost::bind(&transformer_type::unplug, &t, _1));
}

template<class T>
Result Editor::update(const T& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<T>
			::getUpdateXml(device_);
	if (x.isFailed())
		return x.error();

	return Hotplug(getDomain()).update(x.value());
}
template Result Editor::update<CVmFloppyDisk>(const CVmFloppyDisk& device_);
template Result Editor::update<CVmOpticalDisk>(const CVmOpticalDisk& device_);
template Result Editor::update<CVmGenericNetworkAdapter>
	(const CVmGenericNetworkAdapter& device_);

Result Editor::setImageSize(const CVmHardDisk& disk_, quint64 bytes_)
{
	return Block::Unit(getDomain(), disk_.getSystemName()).resize(bytes_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Grub

Grub::Grub(const Limb::Abstract::linkReference_type& link_, const CVmConfiguration& image_):
	m_image(new CVmConfiguration(image_)), m_link(link_)
{
}

Grub::result_type Grub::spawnPaused()
{
	return wrap(boost::bind(&virDomainCreateXML, _1, _2,
		VIR_DOMAIN_START_PAUSED | VIR_DOMAIN_START_AUTODESTROY));
}

Grub::result_type Grub::spawnRunning()
{
	return wrap(boost::bind(&virDomainCreateXML, _1, _2,
		VIR_DOMAIN_START_AUTODESTROY));
}

Grub::result_type Grub::spawnPersistent()
{
	return wrap(boost::bind(&virDomainDefineXML, _1, _2));
}

Grub::result_type Grub::wrap(const decorated_type& decorated_) const
{
	if (decorated_.empty())
		return Error::Simple(PRL_ERR_INVALID_ARG);

	if (m_link.isNull())
		return Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER);

	Prl::Expected<VtInfo, Error::Simple> i = Host(m_link).getVt();
	if (i.isFailed())
		return i.error();

	Transponster::Vm::Reverse::Vm u(*m_image);
	if (PRL_FAILED(Transponster::Director::domain(u, i.value())))
		return Error::Simple(PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED);

	QByteArray x = u.getResult().toUtf8();
	WRITE_TRACE(DBG_DEBUG, "temporary xml:\n%s", x.data());

	virDomainPtr d = decorated_(m_link.data(), x.data());
	if (NULL == d)
		return Failure(PRL_ERR_VM_NOT_CREATED);

	return Unit(d);
}

///////////////////////////////////////////////////////////////////////////////
// struct List

Unit List::at(const QString& uuid_) const
{
	if (m_link.isNull())
		return Unit(NULL);

	PrlUuid x(uuid_.toUtf8().data());
	virDomainPtr d = virDomainLookupByUUIDString(m_link.data(),
			x.toString(PrlUuid::WithoutBrackets).data());
	if (NULL != d && virDomainIsPersistent(d) == 1)
		return Unit(d);

	virDomainFree(d);
	return Unit(NULL);
}

Grub List::getGrub(const CVmConfiguration& image_)
{
	return Grub(m_link, image_);
}

Result List::all(QList<Unit>& dst_)
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	virDomainPtr* a = NULL;
	int z = virConnectListAllDomains(m_link.data(), &a,
					VIR_CONNECT_LIST_DOMAINS_PERSISTENT);
	if (-1 == z)
		return Failure(PRL_ERR_FAILURE);

	for (int i = 0; i < z; ++i)
		dst_ << Unit(a[i]);

	free(a);
	return Result();
}

QList<Performance::Unit> List::getPerformance()
{
	QList<Performance::Unit> output;
	virDomainStatsRecordPtr* s = NULL;
	int z = virConnectGetAllDomainStats(m_link.data(), 0, &s, VIR_CONNECT_GET_ALL_DOMAINS_STATS_PERSISTENT);
	Performance::Unit::pin_type p(s, &virDomainStatsRecordListFree);
	if (0 > z)
	{
		WRITE_TRACE(DBG_FATAL, "unable to get perfomance statistics for all domains (return %d)", z);
		return output;
	}

	for (int i = 0; i < z; ++i)
	{
		if (s[i] == NULL)
			continue;

		output << Performance::Unit(s[i], p);
	}

	return output;
}

namespace Block
{
namespace Generic
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

QString Flavor::getTarget() const
{
	return Transponster::Vm::Reverse::Device<CVmHardDisk>::getTargetName(getXml());
}

void Flavor::commit(signal_type& batch_) const
{
	batch_.connect(boost::bind(&Unit::finish, getAgent()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Meter

template<class T>
Counter::product_type Meter<T>::read_()
{
	PRL_RESULT e = PRL_ERR_UNINITIALIZED;
	boost::shared_ptr<batch_type> b(new batch_type());
	foreach(const T& c, m_componentList)
	{
		QString t(c.getTarget());
		if (!m_journal.contains(t))
			continue;

		PRL_RESULT s = m_journal[t];
		if (PRL_FAILED(e))
			e = s;

		m_strategy.connect(c, s, *b);
	}

	namespace bp = boost::phoenix;
	return bp::let(bp::local_names::_a = bp::val(b), bp::local_names::_b = bp::val(e))
		[bp::bind(bp::ref(*bp::local_names::_a)), bp::val(bp::local_names::_b)];
}

template<class T>
void Meter<T>::account_(QString one_, PRL_RESULT status_)
{
	m_strategy.setStatus(one_, status_, m_journal);
	if (m_componentList.size() == m_journal.size())
	{
		typedef typename journal_type::key_type key_type;
		foreach(const key_type& k, m_journal.keys())
		{
			Counter::account__(k);
		}
	}
	else
		Counter::account__(one_);
}

namespace Strategy
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit<Block::Flavor::Copy>

bool Unit<Block::Flavor::Copy>::launch
	(const Block::Flavor::Copy& flavor_, Counter& counter_) const
{
	QString t(flavor_.getTarget());
	Result e = flavor_.launch(m_map[t]);
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "unable to copy the disk '%s'",
			qPrintable(t));
	}

	return treat(counter_, e);
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit<Block::Flavor::Commit>

void Unit<Block::Flavor::Commit>::setStatus(const QString& component_,
	PRL_RESULT value_, QHash<QString, PRL_RESULT>& journal_) const
{
	// NB. success means the job is ready
	if (PRL_SUCCEEDED(value_))
		journal_[component_] = PRL_ERR_SUCCESS;
}

bool Unit<Block::Flavor::Commit>::launch
	(const Block::Flavor::Commit& flavor_, Counter& counter_) const
{
	Result e = flavor_.launch();
	if (e.isFailed())
	{
		QString t(flavor_.getTarget());
		counter_.account(t, e.error().code());
		WRITE_TRACE(DBG_FATAL, "unable to merge the disk '%s'",
			qPrintable(t));
	}

	return false;
}

void Unit<Block::Flavor::Commit>::connect
	(const Block::Flavor::Commit& component_, PRL_RESULT status_, batch_type& batch_)
{
	if (PRL_SUCCEEDED(status_))
		component_.commit(batch_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit<Block::Flavor::Rebase>

bool Unit<Block::Flavor::Rebase>::launch
	(const Block::Flavor::Rebase& flavor_, Counter& counter_) const
{
	Result e = flavor_.launch();
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "unable to rebase the disk '%s'",
			qPrintable(flavor_.getTarget()));
	}

	return treat(counter_, e);
}

} // namespace Strategy
} // namespace Generic

namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Commit

Result Commit::launch() const
{
	return getAgent().commit();
}

void Commit::commit(signal_type& batch_) const
{
	Generic::Flavor::commit(batch_);
	batch_.connect(boost::bind(&Commit::sweep, getImage()));
}

void Commit::sweep(const QString& path_)
{
	// VIR_DOMAIN_BLOCK_COMMIT_DELETE is not implemented for qemu so we need this
	if (!QFile::remove(path_))
	{
		WRITE_TRACE(DBG_FATAL, "unable to remove file %s",
			qPrintable(path_));
	}
}

int Commit::getEvent()
{
	return VIR_DOMAIN_BLOCK_JOB_TYPE_ACTIVE_COMMIT;
}

///////////////////////////////////////////////////////////////////////////////
// struct Copy

Result Copy::launch(const QString& target_) const
{
	CVmHardDisk d(getXml());
	d.setSystemName(target_);
	d.setUserFriendlyName(target_);
	return getAgent().copy(d);
}

void Copy::abort(signal_type& batch_) const
{
	batch_.connect(boost::bind(&Unit::abort, getAgent()));
}

int Copy::getEvent()
{
	return VIR_DOMAIN_BLOCK_JOB_TYPE_COPY;
}

///////////////////////////////////////////////////////////////////////////////
// struct Rebase

QString Rebase::getImage() const
{
	int z = getXml().m_lstPartition.size();
	if (2 > z)
		return QString();

	return getXml().m_lstPartition[z - 2]->getSystemName();
}

Result Rebase::launch() const
{
	return getAgent().rebase(getImage(), getXml().isAutoCompressEnabled());
}

void Rebase::abort(signal_type& batch_) const
{
	batch_.connect(boost::bind(&Unit::abort, getAgent()));
}

int Rebase::getEvent()
{
	return VIR_DOMAIN_BLOCK_JOB_TYPE_PULL;
}

} // namespace Flavor

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> Unit::getProgress() const
{
	if (m_domain.isNull())
		return Failure(PRL_ERR_TASK_NOT_FOUND);

	virDomainBlockJobInfo info;
	Result e = getInfo(info);
	if (e.isFailed())
		return e.error();
	
	return std::make_pair(info.cur, info.end);
}

Result Unit::copy(const CVmHardDisk& target_) const
{
	Prl::Expected<QString, ::Error::Simple> t =
		Transponster::Vm::Reverse::Device<CVmHardDisk>
			::getPlugXml(target_);
	if (t.isFailed())
		return t.error();

	quint32 flags = VIR_DOMAIN_BLOCK_COPY_SHALLOW |
		VIR_DOMAIN_BLOCK_COPY_TRANSIENT_JOB | VIR_DOMAIN_BLOCK_COPY_REUSE_EXT;
	WRITE_TRACE(DBG_DEBUG, "copy blocks for the disk %s", qPrintable(m_disk));
	WRITE_TRACE(DBG_DEBUG, "the copy target is\n%s", qPrintable(t.value()));
	if (0 != virDomainBlockCopy(m_domain.data(), qPrintable(m_disk),
		qPrintable(t.value()), NULL, 0, flags))
	{
		WRITE_TRACE(DBG_FATAL, "failed to copy blocks for the disk %s",
			qPrintable(m_disk));
		return Failure(PRL_ERR_FAILURE);
	}

	return Result();
}

Result Unit::commit() const
{
	quint32 flags = VIR_DOMAIN_BLOCK_COMMIT_ACTIVE | VIR_DOMAIN_BLOCK_COMMIT_SHALLOW;

	WRITE_TRACE(DBG_DEBUG, "commit blocks for disk %s", qPrintable(m_disk));
	if (0 != virDomainBlockCommit(m_domain.data(), m_disk.toUtf8().data(), NULL, NULL, 0, flags))
	{
		WRITE_TRACE(DBG_FATAL, "failed to commit blocks for disk %s", qPrintable(m_disk));
		return Failure(PRL_ERR_FAILURE);
	}
	return Result();
}

Result Unit::rebase(const QString& base_, bool compress_) const
{
	const char* b = NULL;
	QByteArray z = base_.toUtf8();
	if (!base_.isEmpty())
		b = z.data();

	WRITE_TRACE(DBG_DEBUG, "rebase blocks of the disk %s", qPrintable(m_disk));
	if (0 != virDomainBlockRebase(m_domain.data(), qPrintable(m_disk), b, 0,
		compress_ * VIR_DOMAIN_BLOCK_REBASE_X_COMPRESS))
	{
		WRITE_TRACE(DBG_FATAL, "failed to rebase blocks of the disk %s",
			qPrintable(m_disk));
		return Failure(PRL_ERR_FAILURE);
	}

	return Result();
}

Result Unit::resize(quint64 bytes_) const
{
	const quint32 flags = VIR_DOMAIN_BLOCK_RESIZE_BYTES;

	WRITE_TRACE(DBG_DEBUG, "resize disk %s to %llu", qPrintable(m_disk), bytes_);
	if (0 != virDomainBlockResize(m_domain.data(), m_disk.toUtf8().data(), bytes_, flags))
	{
		WRITE_TRACE(DBG_FATAL, "failed to change size of the disk %s to %llu",
			qPrintable(m_disk), bytes_);
		return Failure(PRL_ERR_FAILURE);
	}
	return Result();
}

Result Unit::abort() const
{
	return do_(m_domain.data(), boost::bind
		(&virDomainBlockJobAbort, _1, m_disk.toUtf8().data(), 0));
}

Result Unit::finish() const
{
	virDomainBlockJobInfo x;
	Result e = getInfo(x);
	if (e.isFailed())
	{
		if (PRL_ERR_TASK_NOT_FOUND == e.error().code())
			return Result();

		return e.error();
	}
	int f = 0;
	switch (x.type)
	{
	case VIR_DOMAIN_BLOCK_JOB_TYPE_COPY:
	case VIR_DOMAIN_BLOCK_JOB_TYPE_ACTIVE_COMMIT:
		f = VIR_DOMAIN_BLOCK_JOB_ABORT_PIVOT;
	default:
		break;
	}
	WRITE_TRACE(DBG_DEBUG, "tries to finish the block job");
	if (0 != virDomainBlockJobAbort(m_domain.data(), m_disk.toUtf8().data(), f))
		return Failure(PRL_ERR_FAILURE);

	return Result();
}

Result Unit::getInfo(virDomainBlockJobInfo& dst_) const
{
	WRITE_TRACE(DBG_DEBUG, "get block job info for disk %s", qPrintable(m_disk));

	int n = virDomainGetBlockJobInfo(m_domain.data(), m_disk.toUtf8().data(), &dst_, 0);

	// -1 in case of failure, 0 when nothing found, 1 when info was found.
	switch (n)
	{
	case 1:
		return Result();
	case 0:
		return Failure(PRL_ERR_TASK_NOT_FOUND);
	case -1:
	default:
		return Failure(PRL_ERR_FAILURE);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Counter

void Counter::account(const QString& one_, PRL_RESULT status_)
{
	QMetaObject::invokeMethod(this, "account_", Qt::AutoConnection,
		Q_ARG(QString, one_), Q_ARG(PRL_RESULT, status_));
}

void Counter::reset()
{
	QMetaObject::invokeMethod(this, "reset_", Qt::AutoConnection);
}

Counter::product_type Counter::read()
{
	product_type output;
	bool x = QMetaObject::invokeMethod(this, "read_",
			Qt::BlockingQueuedConnection, Q_RETURN_ARG(product_type, output));
	Q_UNUSED(x);
	return output;
}

void Counter::account__(QString one_)
{
	if (!m_pending.remove(one_))
		return;

	if (m_pending.isEmpty())
		m_receiver->occur();
}

void Counter::reset_()
{
	if (m_pending.isEmpty())
		return;

	QSet<QString> a = m_pending;
	foreach (const QString& b, a)
	{
		this->account_(b, PRL_ERR_OPERATION_WAS_CANCELED);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Callback

Callback::~Callback()
{
	m_counter->reset();
}

void Callback::do_(int event_, const QString& object_, PRL_RESULT status_)
{
	if (m_filter == event_)
		m_counter->account(object_, status_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Tracker

Tracker::Tracker(QSharedPointer<virDomain> domain_):
	m_ticket(-1), m_ticket2(-1), m_domain(domain_)
{
}

Result Tracker::start(QSharedPointer<Counter> counter_, int event_)
{
	if (counter_.isNull())
		return Error::Simple(PRL_ERR_INVALID_ARG);

	if (m_domain.isNull())
		return Error::Simple(PRL_ERR_INVALID_HANDLE);

	if (!(-1 == m_ticket && m_ticket2 == -1))
		return Error::Simple(PRL_ERR_INVALID_HANDLE);

	virConnectPtr c = virDomainGetConnect(m_domain.data());
	m_ticket = virConnectDomainEventRegisterAny(c, m_domain.data(),
			VIR_DOMAIN_EVENT_ID_BLOCK_JOB,
			VIR_DOMAIN_EVENT_CALLBACK(&react),
			new Callback(counter_, event_), &free);
	if (-1 == m_ticket)
		return Failure(PRL_ERR_FAILURE);

	m_ticket2 = virConnectDomainEventRegisterAny(c, m_domain.data(),
			VIR_DOMAIN_EVENT_ID_BLOCK_JOB_2,
			VIR_DOMAIN_EVENT_CALLBACK(&react),
			new Callback(counter_, event_), &free);
	if (-1 == m_ticket2)
	{
		stop();
		return Failure(PRL_ERR_FAILURE);
	}
	m_counter = counter_;

	return Result();
}

Prl::Expected<Counter::product_type, ::Error::Simple> Tracker::stop()
{
	if (-1 == m_ticket && m_ticket2 == -1)
		return Error::Simple(PRL_ERR_INVALID_HANDLE);

	virConnectPtr c = virDomainGetConnect(m_domain.data());
	virConnectDomainEventDeregisterAny(c, m_ticket);
	m_ticket = -1;
	virConnectDomainEventDeregisterAny(c, m_ticket2);
	m_ticket2 = -1;
	if (m_counter.isNull())
		return Error::Simple(PRL_ERR_UNINITIALIZED);

	Counter::product_type output = m_counter->read();
	m_counter.clear();

	return output;
}

void Tracker::react(virConnectPtr, virDomainPtr, const char * disk_, int type_, int status_, void * opaque_)
{
	WRITE_TRACE(DBG_DEBUG, "block job event: disk '%s' got status %d", disk_, status_);
	switch (status_)
	{
	case VIR_DOMAIN_BLOCK_JOB_READY:
	case VIR_DOMAIN_BLOCK_JOB_COMPLETED:
		return ((Callback* )opaque_)->do_(type_, disk_, PRL_ERR_SUCCESS);
	case VIR_DOMAIN_BLOCK_JOB_FAILED:
		return ((Callback* )opaque_)->do_(type_, disk_, PRL_ERR_FAILURE);
	case VIR_DOMAIN_BLOCK_JOB_CANCELED:
		return ((Callback* )opaque_)->do_(type_, disk_, PRL_ERR_OPERATION_WAS_CANCELED);
	}
}

void Tracker::free(void* callback_)
{
	delete (Callback* )callback_;
}

///////////////////////////////////////////////////////////////////////////////
// struct Activity

PRL_RESULT Activity::stop()
{
	if (m_tracker.isNull())
		return PRL_ERR_UNINITIALIZED;

	Prl::Expected<Counter::product_type, ::Error::Simple> t =
		m_tracker->stop();
	m_tracker.clear();
	if (t.isFailed())
		return t.error().code();

	return t.value()();
}

///////////////////////////////////////////////////////////////////////////////
// struct Launcher

Activity Launcher::copy(const QHash<QString, CVmHardDisk>& imageList_, Completion& completion_)
{
	typedef Generic::Strategy::Unit<Flavor::Copy> strategy_type;

	QList<Flavor::Copy> f;
	strategy_type::map_type m;
	BOOST_FOREACH(const QString& k, imageList_.keys())
	{
		f << Flavor::Copy(Generic::Flavor(imageList_[k], getDomain()));
		m[f.back().getTarget()] = k;
	}
	return start(strategy_type(f, m), f, completion_);
}

Activity Launcher::merge(const imageList_type& imageList_, Completion& completion_)
{
	QList<Flavor::Commit> f;
	BOOST_FOREACH(const CVmHardDisk& v, imageList_)
	{
		f << Flavor::Commit(Generic::Flavor(v, getDomain()));
	}
	return start(Generic::Strategy::Unit<Flavor::Commit>(), f, completion_);
}

Activity Launcher::rebase(const imageList_type& imageList_, Completion& completion_)
{
	QList<Flavor::Rebase> f;
	BOOST_FOREACH(const CVmHardDisk& v, imageList_)
	{
		f << Flavor::Rebase(Generic::Flavor(v, getDomain()));
	}
	return start(Generic::Strategy::Unit<Flavor::Rebase>(f), f, completion_);
}

template<class T, class U>
Activity Launcher::start(T strategy_, const QList<U>& componentList_, Completion& completion_)
{
	QSharedPointer<Counter> c(new Generic::Meter<U>(componentList_, strategy_, completion_),
			&QObject::deleteLater);
	c->moveToThread(QCoreApplication::instance()->thread());

	QSharedPointer<Tracker> t(new Tracker(getDomain()));
	t->start(c, U::getEvent());

	foreach(const U& f, componentList_)
	{
		if (strategy_.launch(f, *c))
			break;
	}
	return Activity(t);
}

///////////////////////////////////////////////////////////////////////////////
// struct Export

Result Export::stop(const componentList_type& request_)
{
	Transponster::Snapshot::Export::Request u;
	Prl::Expected<QString, PRL_RESULT> x = Transponster::Director::marshalReverse(request_, u);
	if (x.isFailed())
		return Error::Simple(x.error());

	return do_(getDomain().data(), boost::bind(&virDomainBlockExportXStop,
		_1, qPrintable(x.value()), 0));
}

Result Export::start(const componentList_type& request_)
{
	Transponster::Snapshot::Export::Request u;
	Prl::Expected<QString, PRL_RESULT> x = Transponster::Director::marshalReverse(request_, u);
	if (x.isFailed())
		return Error::Simple(x.error());

	WRITE_TRACE(DBG_DEBUG, "x-blockexport xml:\n%s", qPrintable(x.value()));
	return do_(getDomain().data(), boost::bind(&virDomainBlockExportXStart,
		_1, qPrintable(x.value()), 0));
}

Prl::Expected<Export::componentList_type, ::Error::Simple> Export::list()
{
	Config x(getDomain(), getLink(), 0);
	Prl::Expected<QString, Error::Simple> c = x.read();
	if (c.isFailed())
		return c.error();

	Transponster::Snapshot::Export::View u;
	PRL_RESULT e = Transponster::Director::marshalDirect(c.value(), u);
	if (PRL_FAILED(e))
		return Failure(e);

	return u.getValue();
}

} // namespace Block

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

Request::Request() : m_flags(VIR_DOMAIN_SNAPSHOT_CREATE_ATOMIC)
{
}

void Request::setDiskOnly()
{
	m_flags |= VIR_DOMAIN_SNAPSHOT_CREATE_DISK_ONLY;
}

void Request::setConsistent()
{
	m_flags |= VIR_DOMAIN_SNAPSHOT_CREATE_QUIESCE;
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virDomainSnapshotPtr snapshot_): m_snapshot(snapshot_, &virDomainSnapshotFree)
{
}

Result Unit::getUuid(QString& dst_) const
{
	const char* n = virDomainSnapshotGetName(m_snapshot.data());
	if (NULL == n)
		return Failure(PRL_ERR_INVALID_HANDLE);

	QString x = n;
	if (!PrlUuid::isUuid(x.toStdString()))
		return Failure(PRL_ERR_INVALID_HANDLE);

	dst_ = x;
	return Result();
}

Result Unit::getState(CSavedStateTree& dst_) const
{
	char* x = virDomainSnapshotGetXMLDesc(m_snapshot.data(), VIR_DOMAIN_XML_SECURE);
	if (NULL == x)
		return Failure(PRL_ERR_INVALID_HANDLE);

	Transponster::Snapshot::Direct y(x);
	if (PRL_FAILED(Transponster::Director::snapshot(y)))
		return Failure(PRL_ERR_PARSE_VM_DIR_CONFIG);

	dst_ = y.getResult();
	dst_.SetCurrent(1 == virDomainSnapshotIsCurrent(m_snapshot.data(), 0));
	return Result();
}

QSharedPointer<virConnect> Unit::getLink() const
{
	virConnectPtr x = virDomainSnapshotGetConnect(m_snapshot.data());
	if (NULL == x)
		return QSharedPointer<virConnect>();

	virConnectRef(x);
	return QSharedPointer<virConnect>(x, &virConnectClose);
}

Result Unit::getConfig(CVmConfiguration& dst_) const
{
	char* x = virDomainSnapshotGetXMLDesc(m_snapshot.data(), VIR_DOMAIN_XML_SECURE);
	if (NULL == x)
		return Failure(PRL_ERR_INVALID_HANDLE);

	Prl::Expected<VtInfo, Error::Simple> i = Host(getLink()).getVt();
	if (i.isFailed())
		return i.error();

	Transponster::Snapshot::Vm y(x);
	if (PRL_FAILED(Transponster::Director::domain(y, i.value())))
		return Failure(PRL_ERR_PARSE_VM_DIR_CONFIG);

	CVmConfiguration* output = y.getResult();
	if (NULL == output)
		return Error::Simple(PRL_ERR_FAILURE);

	dst_ = *output;
	delete output;
	return Result();
}

Unit Unit::getParent() const
{
	return Unit(virDomainSnapshotGetParent(m_snapshot.data(), 0));
}

Result Unit::revert()
{
	return do_(m_snapshot.data(), boost::bind
		(&virDomainRevertToSnapshot, _1, VIR_DOMAIN_SNAPSHOT_REVERT_FORCE));
}

Result Unit::undefine()
{
	return do_(m_snapshot.data(), boost::bind
		(&virDomainSnapshotDelete, _1, 0));
}

Result Unit::undefineRecursive()
{
	return do_(m_snapshot.data(), boost::bind
		(&virDomainSnapshotDelete, _1,  VIR_DOMAIN_SNAPSHOT_DELETE_CHILDREN));
}

///////////////////////////////////////////////////////////////////////////////
// struct Block

Block::Block(virDomainBlockSnapshotXPtr snapshot_, const QString& map_):
	m_map(map_), m_object(snapshot_, &virDomainBlockSnapshotXFree)
{
}

Result Block::undefine()
{
	return do_(m_object.data(),
		boost::bind(&virDomainBlockSnapshotXDelete, _1, 0));
}

Result Block::getUuid(QString& dst_) const
{
	const char* u = virDomainBlockSnapshotXGetName(m_object.data());
	if (NULL == u)
		return Failure(PRL_ERR_INVALID_HANDLE);

	dst_ = u;
	return Result();
}

Result Block::deleteMap()
{
	virDomainPtr y = virDomainBlockSnapshotXGetDomain(m_object.data());
	return do_(y, boost::bind(&virDomainBlockCheckpointXRemove, _1,
		qPrintable(m_map), 0));
}

///////////////////////////////////////////////////////////////////////////////
// struct List

Unit List::at(const QString& uuid_) const
{
	return Unit(virDomainSnapshotLookupByName(m_domain.data(), qPrintable(uuid_), 0));
}

Result List::all(QList<Unit>& dst_) const
{
	virDomainSnapshotPtr* a = NULL;
	int n = virDomainListAllSnapshots(m_domain.data(), &a, 0);
	if (0 > n)
		return Failure(PRL_ERR_INVALID_HANDLE);

	for (int i = 0; i < n; ++i)
	{
		QString u;
		Unit o(a[i]);
		if (o.getUuid(u).isSucceed())
			dst_ << o;
	}
	free(a);
	return Result();
}

Prl::Expected<Unit, Error::Simple>
	List::define_(const QString& uuid_, const Request& req_)
{
	CVmConfiguration x;
	virDomainRef(m_domain.data());
	Vm::Unit m(m_domain.data());
	Result e = m.getConfig(x);
	if (e.isFailed())
		return e.error();

	VIRTUAL_MACHINE_STATE s;
	if ((e = m.getState().getValue(s)).isFailed())
		return e.error();

	Transponster::Snapshot::Reverse y(uuid_, req_.getDescription(), x);
	PRL_RESULT f = Transponster::Director::snapshot(y);
	if (PRL_FAILED(f))
		return Error::Simple(f);

	if (VMS_RUNNING == s || VMS_PAUSED == s)
		y.setMemory();

	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", y.getResult().toUtf8().data());
	virDomainSnapshotPtr p = virDomainSnapshotCreateXML(m_domain.data(),
			y.getResult().toUtf8().data(),
			req_.getFlags());
	if (NULL == p)
		return Failure(PRL_ERR_FAILURE);

	return Unit(p);
}

Result List::translate(const Prl::Expected<Unit, Error::Simple>& result_, Unit* dst_)
{
	if (result_.isFailed())
		return result_.error();
	if (NULL != dst_)
		*dst_ = result_.value();

	return Result();
}

Result List::define(const QString& uuid_, const Request& req_, Unit* dst_)
{
	return translate(define_(uuid_, req_), dst_);
}

Prl::Expected<Block, ::Error::Simple>
	List::defineBlock(const QString& uuid_, const QList<SnapshotComponent>& components_)
{
	BackupSnapshot s;
	quint32 f = VIR_DOMAIN_BLOCK_SNAPSHOT_X_CREATE_AUTODELETE;
	if (uuid_.isEmpty())
		s.setUuid(Uuid::createUuid().toString(/*PrlUuid::WithoutBrackets*/));
	else
	{
		s.setUuid(uuid_);
		f |= VIR_DOMAIN_BLOCK_SNAPSHOT_X_CREATE_CHECKPOINT;
	}
	foreach (const SnapshotComponent c, components_)
	{
		s.m_lstSnapshotComponents << new SnapshotComponent(c);
	}
	Transponster::Snapshot::Block x;
	Prl::Expected<QString, PRL_RESULT> y = Transponster::Director::marshalReverse(s, x);
	if (y.isFailed())
		return Error::Simple(y.error());

	WRITE_TRACE(DBG_DEBUG, "x-blocksnapshot xml:\n%s", qPrintable(y.value()));
	virDomainBlockSnapshotXPtr b = virDomainBlockSnapshotXCreateXML
		(m_domain.data(), qPrintable(y.value()), f);
	if (NULL == b)
		return Failure(PRL_ERR_FAILURE);

	return Block(b, uuid_);
}

Result List::createExternal(const QString& uuid_, const QList<CVmHardDisk*>& disks_)
{
	quint32 flags = VIR_DOMAIN_SNAPSHOT_CREATE_DISK_ONLY | VIR_DOMAIN_SNAPSHOT_CREATE_NO_METADATA;
	CVmConfiguration x;
	virDomainRef(m_domain.data());
	Vm::Unit m(m_domain.data());
	Result e = m.getConfig(x);
	if (e.isFailed())
		return e.error();

	QStringList disks;
	std::transform(disks_.begin(), disks_.end(), std::back_inserter(disks),
		boost::bind(&CVmHardDisk::getSerialNumber, _1));

	Transponster::Snapshot::Reverse y(uuid_, "", x);
	y.setPolicy(boost::bind(Transponster::Snapshot::External(disks, uuid_), _1));
	PRL_RESULT f = Transponster::Director::snapshot(y);
	if (PRL_FAILED(f))
		return Error::Simple(f);

	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", y.getResult().toUtf8().data());
	virDomainSnapshotPtr p = virDomainSnapshotCreateXML(m_domain.data(),
					y.getResult().toUtf8().data(), flags);
	if (NULL == p)
		return Failure(PRL_ERR_FAILURE);
	virDomainSnapshotFree(p);
	return Result();
}

Result List::countSnapshotsNum(int& res) const
{
	res = virDomainSnapshotNum(m_domain.data(), 0);

	if (res < 0)
		return Failure(PRL_ERR_INVALID_HANDLE);

	return Result();
}

} // namespace Snapshot
} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virNetworkPtr network_, const read_type& read_):
	m_read(read_), m_network(network_, &virNetworkFree)
{
}

Result Unit::stop()
{
	return do_(m_network.data(), boost::bind(&virNetworkDestroy, _1));
}

Result Unit::start()
{
	return do_(m_network.data(), boost::bind(&virNetworkCreate, _1));
}

Result Unit::undefine()
{
	return do_(m_network.data(), boost::bind(&virNetworkUndefine, _1));
}

Result Unit::getConfig(CVirtualNetwork& dst_) const
{
	if (m_read.empty())
		return Failure(PRL_ERR_UNINITIALIZED);

	virConnectPtr c = virNetworkGetConnect(m_network.data());
	virConnectRef(c);

	Prl::Expected<CVirtualNetwork, ::Error::Simple> x = m_read(m_network.data(),
		Interface::List::Backend(QSharedPointer<virConnect>(c, &virConnectClose)));
	if (x.isFailed())
		return x.error();

	dst_ = x.value();
	return Result();
}

///////////////////////////////////////////////////////////////////////////////
// struct List

Prl::Expected<CVirtualNetwork, ::Error::Simple>
	List::read(virNetworkPtr network_, const Interface::List::Backend& bridges_)
{
	if (network_ == NULL)
		return Failure(PRL_ERR_UNINITIALIZED);

	char* x = virNetworkGetXMLDesc(network_,
			VIR_NETWORK_XML_INACTIVE);
	if (NULL == x)
		return Failure(PRL_ERR_VM_GET_CONFIG_FAILED);

	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", x);
	Transponster::Network::Direct u(x, 0 < virNetworkIsActive(network_));
	if (PRL_FAILED(Transponster::Director::network(u)))
		return Failure(PRL_ERR_PARSE_VM_DIR_CONFIG);

	CVirtualNetwork output = u.getResult();
	CVZVirtualNetwork* z = output.getVZVirtualNetwork();
	if (NULL != z)
	{
		Prl::Expected<Libvirt::Instrument::Agent::Interface::Bridge, ::Error::Simple>
			b = bridges_.findByName(z->getBridgeName());
		output.getHostOnlyNetwork()->
			getVirtuozzoAdapter()->setName(z->getBridgeName());
		if (b.isSucceed())
		{
			output.setBoundCardMac(b.value().getMaster().getMacAddress());
			output.setVLANTag(b.value().getMaster().getVLANTag());
			z->setMasterInterface(b.value().getMaster().getDeviceName());
		}
		else if (PRL_ERR_NETWORK_ADAPTER_NOT_FOUND == b.error().code())
			output.setVZVirtualNetwork(NULL);
		else
			return b.error();
	}
	return output;
}

Unit List::craft(virNetworkPtr network_) const
{
	return Unit(network_, boost::bind(&List::read, _1, _2));
}

Unit List::at(const QString& uuid_) const
{
	if (m_link.isNull())
		return Unit();

	PrlUuid x(uuid_.toUtf8().data());
	virNetworkPtr n = virNetworkLookupByUUIDString(m_link.data(),
			x.toString(PrlUuid::WithoutBrackets).data());
	if (NULL != n && virNetworkIsPersistent(n) == 1)
		return craft(n);

	virNetworkFree(n);
	return Unit();
}

Result List::all(QList<Unit>& dst_) const
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	virNetworkPtr* a = NULL;
	int z = virConnectListAllNetworks(m_link.data(), &a,
					VIR_CONNECT_LIST_NETWORKS_PERSISTENT);
	if (-1 == z)
		return Failure(PRL_ERR_FAILURE);

	Interface::List::Backend b(m_link);
	for (int i = 0; i < z; ++i)
	{
		if (read(a[i], b).isSucceed())
			dst_ << craft(a[i]);
		else
			virNetworkFree(a[i]);
	}
	free(a);
	return Result();
}

Result List::all(QList<CVirtualNetwork>& dst_) const
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	virNetworkPtr* a = NULL;
	int z = virConnectListAllNetworks(m_link.data(), &a,
					VIR_CONNECT_LIST_NETWORKS_PERSISTENT);
	if (-1 == z)
		return Failure(PRL_ERR_FAILURE);

	Interface::List::Backend b(m_link);
	for (int i = 0; i < z; ++i)
	{
		Prl::Expected<CVirtualNetwork, ::Error::Simple> x = read(a[i], b);
		if (x.isSucceed())
			dst_ << x.value();
		else
			virNetworkFree(a[i]);
	}
	free(a);
	return Result();
}

Result List::find(const QString& name_, Unit* dst_) const
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	virNetworkPtr n = virNetworkLookupByName(m_link.data(),
			name_.toUtf8().data());
	if (NULL == n)
		return Failure(PRL_ERR_FILE_NOT_FOUND);

	Unit u(craft(n));
	if (1 != virNetworkIsPersistent(n))
		return Failure(PRL_ERR_FILE_NOT_FOUND);

	CVirtualNetwork x;
	if (u.getConfig(x).isFailed())
		return Result(Error::Simple(PRL_ERR_FILE_NOT_FOUND));

	if (NULL != dst_)
		*dst_ = u;

	return Result();
}

Result List::define(const CVirtualNetwork& config_, Unit* dst_)
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	Transponster::Network::Reverse u(config_);
	if (PRL_FAILED(Transponster::Director::network(u)))
		return Result(Error::Simple(PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED));

	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", u.getResult().toUtf8().data());
	virNetworkPtr n = virNetworkDefineXML(m_link.data(), u.getResult().toUtf8().data());
	if (NULL == n)
		return Failure(PRL_ERR_VM_NOT_CREATED);

	Unit m(craft(n));
	if (0 != virNetworkSetAutostart(n, 1))
	{
		m.undefine();
		return Failure(PRL_ERR_FAILURE);
	}
	if (NULL != dst_)
		*dst_ = m;

	return Result();
}
} // namespace Network

namespace Filter
{

Unit::Unit(virNWFilterPtr filter_) : m_filter(filter_, &virNWFilterFree)
{
}

Result Unit::undefine()
{
	doResult_type r = do_(m_filter.data(), boost::bind(&virNWFilterUndefine, _1));
	if (r.isFailed() && r.error().getMainCode() == VIR_ERR_NO_NWFILTER)
		return Result();
	return r;
}

bool Unit::operator==(const Unit &other) const
{
	return m_filter == other.m_filter;
}

// struct List

Result List::define(const CVmGenericNetworkAdapter &adapter_)
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	Transponster::Filter::Reverse u(adapter_);

	QString filter_name = Transponster::Filter::Reverse::getVzfilterName(adapter_);
	virNWFilterPtr existing_filter = virNWFilterLookupByName(m_link.data(),
															 qPrintable(filter_name));
	if (NULL != existing_filter)
	{
		char existing_filter_uuid[VIR_UUID_STRING_BUFLEN];
		virNWFilterGetUUIDString(existing_filter, existing_filter_uuid);
		u.setUuid(existing_filter_uuid);
		virNWFilterFree(existing_filter);
	}

	virNWFilterPtr n = virNWFilterDefineXML(m_link.data(),
											qPrintable(u.getResult()));
	if (NULL == n)
		return Failure(PRL_ERR_VM_APPLY_FILTER_CONFIG_FAILED);

	return Result();
}

Result List::define(const QList<CVmGenericNetworkAdapter *> &adapters)
{
	Result ret;
	foreach(const CVmGenericNetworkAdapter *adapter, adapters)
	{
		CVmGenericNetworkAdapter copy = PrlNet::fixMacFilter(*adapter, adapters);
		if ((ret = define(copy)).isFailed())
		{
			undefine(adapters, true);
			return ret;
		}
	}
	return Result();
}

Result List::undefine(const CVmGenericNetworkAdapter &config_)
{
	QString name = Transponster::Filter::Reverse::getVzfilterName(config_);
	return at(name).undefine();
}

Result List::undefine(const QList<CVmGenericNetworkAdapter *> &adapters,
					  bool ignore_errors)
{
	Result last_ret;
	Result ret;
	foreach(const CVmGenericNetworkAdapter *adapter, adapters)
	{
		if ((last_ret = undefine(*adapter)).isFailed())
		{
			ret = last_ret;
			if (!ignore_errors)
				return ret;
		}
	}
	return ret;
}

Result List::undefine_unused(const QList<CVmGenericNetworkAdapter *> &old_adapters,
							 const QList<CVmGenericNetworkAdapter *> &new_adapters,
							 bool ignore_errors_)
{
	QSet<QString> used_mac_addresses;
	foreach(const CVmGenericNetworkAdapter* adapter, new_adapters)
		used_mac_addresses.insert(Transponster::Filter::Reverse::getVzfilterName(*adapter));
	

	QList<CVmGenericNetworkAdapter *> unused_adapters;

	for (QList<CVmGenericNetworkAdapter *>::const_iterator adapter = old_adapters.begin();
		 adapter != old_adapters.end(); ++adapter)
	{
		if (!used_mac_addresses.contains(
				Transponster::Filter::Reverse::getVzfilterName(*adapter)))
			unused_adapters.append(*adapter);
	}
	
	return undefine(unused_adapters, ignore_errors_);
}

Unit List::at(const QString& name_) const
{
	return Unit(virNWFilterLookupByName(m_link.data(), qPrintable(name_)));
}
} // namespace Filter

namespace Interface
{
///////////////////////////////////////////////////////////////////////////////
// struct Bridge

Bridge::Bridge(virInterfacePtr interface_, const CHwNetAdapter& master_):
	m_master(master_), m_interface(interface_, &virInterfaceFree)
{
}

QString Bridge::getName() const
{
	const char* n = virInterfaceGetName(m_interface.data());
	return QString(NULL == n ? "" : n);
}

Result Bridge::stop()
{
	return do_(m_interface.data(), boost::bind(&virInterfaceDestroy, _1, 0));
}

Result Bridge::start()
{
	return do_(m_interface.data(), boost::bind(&virInterfaceCreate, _1, 0));
}

Result Bridge::undefine()
{
	return do_(m_interface.data(), boost::bind(&virInterfaceUndefine, _1));
}

namespace List
{
///////////////////////////////////////////////////////////////////////////////
// struct Backend

Backend::Backend(const QSharedPointer<virConnect>& link_):
	m_all(Error::Simple(PRL_ERR_UNINITIALIZED)), m_link(link_)
{
	reload();
}

Backend& Backend::reload()
{
	m_all = Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER);
	if (m_link.isNull())
		return *this;

	PrlNet::EthAdaptersList m;
	PRL_RESULT e = PrlNet::makeBindableAdapterList(m, false, false);
	if (PRL_FAILED(e))
	{
		m_all = Error::Simple(e);
		return *this;
	}
	virInterfacePtr* a = NULL;
	int z = virConnectListAllInterfaces(m_link.data(), &a, 0);
	if (-1 == z)
	{
		m_all = Error::Simple(PRL_ERR_FAILURE);
		return *this;
	}
	m_all = QList<Bridge>();
	for (int i = 0; i < z; ++i)
	{
		char* xml = virInterfaceGetXMLDesc(a[i], VIR_INTERFACE_XML_INACTIVE);
		if (!QString::fromUtf8(xml).contains("type='bridge'"))
		{
			virInterfaceFree(a[i]);
			free(xml);
			continue;
		}
		Transponster::Interface::Bridge::Direct u(xml, 0 < virInterfaceIsActive(a[i]));
		if (PRL_FAILED(Transponster::Director::bridge(u)))
		{
			virInterfaceFree(a[i]);
			continue;
		}
		CHwNetAdapter h = u.getMaster();
		if (h.getMacAddress().isEmpty())
		{
			PrlNet::EthAdaptersList::iterator e = m.end();
			PrlNet::EthAdaptersList::iterator p =
				std::find_if(m.begin(), e,
					boost::bind(&PrlNet::EthAdaptersList::value_type::_name, _1)
						== h.getDeviceName());
			if (e == p)
			{
				virInterfaceFree(a[i]);
				continue;
			}
			h.setMacAddress(PrlNet::ethAddressToString(p->_macAddr));
		}
		Bridge b(a[i], h);
		if (!b.getName().startsWith("virbr"))
			m_all.value() << b;
	}
	free(a);
	return *this;
}

Prl::Expected<Bridge, ::Error::Simple> Backend::findByName(const QString& name_) const
{
	if (name_.startsWith("virbr"))
		return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);

	if (m_all.isFailed())
		return m_all.error();

	foreach (const Bridge& b, m_all.value())
	{
		if (b.getName() == name_)
			return b;
	}
	return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);
}

Prl::Expected<Bridge, ::Error::Simple> Backend::findByMasterName(const QString& name_) const
{
	if (m_all.isFailed())
		return m_all.error();

	foreach (const Bridge& b, m_all.value())
	{
		if (b.getMaster().getDeviceName() == name_)
			return b;
	}
	return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);
}

Prl::Expected<Bridge, ::Error::Simple> Backend::findByMasterMacAndVlan(const QString& mac_, int tag_) const
{
	if (m_all.isFailed())
		return m_all.error();

	foreach (const Bridge& b, m_all.value())
	{
		if (b.getMaster().getMacAddress() == mac_ &&
			b.getMaster().getVLANTag() == tag_)
			return b;
	}
	return Error::Simple(PRL_ERR_BRIDGE_NOT_FOUND_FOR_NETWORK_ADAPTER);
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

Result Frontend::all(QList<Bridge>& dst_) const
{
	Backend b(m_link);
	if (b.getAll().isFailed())
		return b.getAll().error();

	dst_ = b.getAll().value();
	return Result();
}

Result Frontend::find(const QString& name_, Bridge& dst_) const
{
	Prl::Expected<Bridge, ::Error::Simple> x = Backend(m_link).findByName(name_);
	if (x.isFailed())
		return x.error();

	dst_ = x.value();
	return Result();
}

Result Frontend::find(const CHwNetAdapter& eth_, Bridge& dst_) const
{
	Prl::Expected<Bridge, ::Error::Simple> x = Backend(m_link)
		.findByMasterMacAndVlan(eth_.getMacAddress(), eth_.getVLANTag());
	if (x.isFailed())
		return x.error();

	dst_ = x.value();
	return Result();
}

Result Frontend::find(const QString& mac_, unsigned short vlan_, CHwNetAdapter& dst_) const
{
	QString name = PrlNet::findAdapterName(mac_, vlan_);

	if (vlan_ == PRL_INVALID_VLAN_TAG)
		return find(name, dst_);
	
	return findBridge(name, dst_);
}

Result Frontend::find(const QString& name_, CHwNetAdapter& dst_) const
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	virInterfacePtr f = virInterfaceLookupByName(m_link.data(), name_.toUtf8().data());

	if (NULL != f)
	{
		Transponster::Interface::Physical::Direct u(0 < virInterfaceIsActive(f));
		char* x = virInterfaceGetXMLDesc(f, VIR_INTERFACE_XML_INACTIVE);
		virInterfaceFree(f);

		if (PRL_SUCCEEDED(Transponster::Director::marshalDirect(x, u)))
		{
			dst_ = u.getValue();
			return Result();
		}
	}

	return findBridge(name_, dst_);
}

Result Frontend::findBridge(const QString& name_, CHwNetAdapter& dst_) const
{
	Prl::Expected<Bridge, ::Error::Simple> x = Backend(m_link)
		.findByMasterName(name_);
	if (x.isFailed())
		return x.error();

	dst_ = x.value().getMaster();
	return Result();
}

Result Frontend::define(const CHwNetAdapter& eth_, Bridge& dst_)
{
	Backend d(m_link);
	if (d.getAll().isFailed())
		return d.getAll().error();

	Prl::Expected<Bridge, ::Error::Simple> m = d
		.findByMasterName(eth_.getDeviceName());
	if (m.isSucceed() && m.value().getMaster().getMacAddress() == eth_.getMacAddress())
		return Result(Error::Simple(PRL_ERR_ENTRY_ALREADY_EXISTS));

	uint x = ~0;
	foreach (const Bridge& b, d.getAll().value())
	{
		x = qMax(x + 1, b.getName().mid(2).toUInt() + 1) - 1;
	}
	forever
	{
		QString n = QString("br%1").arg(++x);
		Transponster::Interface::Bridge::Reverse u(n, eth_);
		if (PRL_FAILED(Transponster::Director::bridge(u)))
			return Result(Error::Simple(PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED));

		virInterfacePtr b = virInterfaceDefineXML(m_link.data(), u.getResult().toUtf8().data(), 0);
		if (NULL == b)
			continue;

		dst_ = Bridge(b, eth_);
		return Result();
	}
}

} // namespace List
} // namespace Interface

///////////////////////////////////////////////////////////////////////////////
// struct Host

Prl::Expected<VtInfo, Error::Simple> Host::getVt() const
{
	VtInfo v;
	CVCpuInfo* i = v.getQemuKvm()->getVCpuInfo();
	qint32 x = virConnectGetMaxVcpus(m_link.data(), "kvm");
	if (-1 == x)
		return Failure(PRL_ERR_VM_UNABLE_GET_GUEST_CPU);

	virNodeInfo h;
	if (do_(m_link.data(), boost::bind(&virNodeGetInfo, _1, &h)).isFailed())
		return Failure(PRL_ERR_CANT_INIT_REAL_CPUS_INFO);

	i->setMaxVCpu(std::min<quint32>(x, h.cpus));
	i->setDefaultPeriod(100000);
	if (!CDspService::instance()->getHostInfo()->data()->getCpu())
		return Failure(PRL_ERR_CANT_INIT_REAL_CPUS_INFO);
	i->setMhz(CDspService::instance()->getHostInfo()->data()->getCpu()->getSpeed());

	v.setGlobalCpuLimit(PRL_VM_CPULIMIT_FULL == CDspService::instance()
			->getDispConfigGuard().getDispConfig()
			->getDispatcherSettings()->getCommonPreferences()
			->getWorkspacePreferences()->getVmGuestCpuLimitType());

	Transponster::Host::Capabilities d;
	char *caps = virConnectGetDomainCapabilities(m_link.data(),
		NULL, NULL, NULL, NULL, 0);
	if (PRL_FAILED(Transponster::Director::marshalDirect(caps, d)))
		return Failure(PRL_ERR_FAILURE);

	v.setCpuFeatures(d.getCpuFeatures());
	v.setCpuModel(d.getCpuModel());

	return v;
}

Prl::Expected<QList<CHwGenericPciDevice>, ::Error::Simple>
	Host::getAssignablePci() const
{
	virNodeDevicePtr* a = NULL;
	int n = virConnectListAllNodeDevices(m_link.data(), &a,
		VIR_CONNECT_LIST_NODE_DEVICES_CAP_PCI_DEV);
	if (-1 == n)
		return Failure(PRL_ERR_FAILURE);

	Prl::Expected<QList<CHwGenericPciDevice>, ::Error::Simple> output;
	for (int i = 0; i < n; ++i)
	{
		Transponster::Host::Pci u;
		char* x = virNodeDeviceGetXMLDesc(a[i], 0);
		if (PRL_FAILED(Transponster::Director::marshalDirect(x, u)))
		{
			output = Failure(PRL_ERR_FAILURE);
			break;
		}
		if (u.getValue())
			output.value() << u.getValue().get();
	}
	for (int i = 0; i < n; virNodeDeviceFree(a[i++]));
	free(a);

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Hub

Hub::Hub()
{
}

void Hub::setLink(QSharedPointer<virConnect> value_)
{
	m_link = value_.toWeakRef();
}

} // namespace Agent

///////////////////////////////////////////////////////////////////////////////
// struct Pipeline

Pipeline::result_type Pipeline::operator()(
		const QByteArray &input_, const QString &outFile_) const
{
	if (m_cmds.isEmpty())
		return result_type();

	QList<proc_type> procs = build();

	if (!outFile_.isEmpty())
		procs.last().first->setStandardOutputFile(outFile_);
	foreach(const proc_type &p, procs)
	{
		p.first->start(p.second);
	}

	if (!input_.isEmpty())
	{
		procs.first().first->write(input_);
		procs.first().first->closeWriteChannel();
	}

	foreach(const proc_type &p, procs)
	{
		if (!p.first->waitForStarted() || !p.first->waitForFinished(10000))
			return p.second;
	}
	return result_type();
}

QList<Pipeline::proc_type> Pipeline::build() const
{
	QList<proc_type> procs;
	foreach(const QString &cmd, m_cmds)
	{
		procs << proc_type(QSharedPointer<QProcess>(new QProcess), cmd);
	}

	// Setup a pipeline
	QSharedPointer<QProcess> last;
	foreach(const proc_type &p, procs)
	{
		if (last)
			last->setStandardOutputProcess(p.first.data());
		last = p.first;
	}
	return procs;
}

} // namespace Instrument
} // namespace Libvirt
