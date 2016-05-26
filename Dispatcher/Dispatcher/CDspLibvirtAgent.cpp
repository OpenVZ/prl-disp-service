/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "CDspLibvirt.h"
#include "CDspLibvirt_p.h"
#include "CDspLibvirtExec.h"
#include "CDspService.h"
#include "Stat/CDspStatStorage.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Libraries/Transponster/Direct.h>
#include <Libraries/Transponster/Reverse.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <Libraries/PrlNetworking/netconfig.h>
#include <prlcommon/HostUtils/HostUtils.h>

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
	}

	return result_;
}

namespace Agent
{
///////////////////////////////////////////////////////////////////////////////
// struct Failure

Failure::Failure(PRL_RESULT result_): Error::Simple(result_), m_virErrorCode(0)
{
#if (LIBVIR_VERSION_NUMBER > 1000004)
	const char* m = virGetLastErrorMessage();
	WRITE_TRACE(DBG_FATAL, "libvirt error %s", m ? : "unknown");
	details() = m;
#endif
	virErrorPtr err = virGetLastError();
	if (err)
		m_virErrorCode = err->code;
}

bool Failure::isTransient() const
{
	return m_virErrorCode == VIR_ERR_AGENT_UNRESPONSIVE ||
	       m_virErrorCode == VIR_ERR_AGENT_UNSYNCED;
}

}

namespace Instrument
{
namespace Agent
{

template<class T, class U>
static Result do_(T* handle_, U action_)
{
	if (NULL == handle_)
		return Result(Error::Simple(PRL_ERR_UNINITIALIZED));

	if (0 <= action_(handle_))
		return Result();

	return Failure(PRL_ERR_FAILURE);
}

namespace Vm
{
namespace Migration
{
///////////////////////////////////////////////////////////////////////////////
// struct Agent

Result Agent::cancel()
{
	return do_(m_domain.data(), boost::bind(&virDomainAbortJob, _1));
}

Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple>
Agent::getProgress()
{
	virDomainJobInfo j;
	Result x = do_(m_domain.data(), boost::bind(&virDomainGetJobInfo, _1, &j));
	if (x.isFailed())
		return x.error();

	return std::make_pair(j.dataTotal, j.dataRemaining);
}

Result Agent::setDowntime(quint32 value_)
{
	return do_(m_domain.data(), boost::bind(&virDomainMigrateSetMaxDowntime, _1, 1000*value_, 0));
}

Result Agent::migrate(const CVmConfiguration& config_, unsigned int flags_,
	Parameters::Builder& parameters_)
{
	Result e = Basic(Config(m_domain, m_link, 0), config_)(parameters_);
	if (e.isFailed())
		return e;

	// shared to use cleanup callback only
	QSharedPointer<virConnect> c(virConnectOpen(QSTR2UTF8(m_uri)),
			virConnectClose);
	if (c.isNull())
		return Failure(PRL_ERR_FAILURE);

	if (NULL == m_domain.data())
		return Result(Error::Simple(PRL_ERR_UNINITIALIZED));

	Parameters::Result_type p = parameters_.extract();
	virDomainPtr d = virDomainMigrate3(m_domain.data(), c.data(),
				p.first.data(), p.second, flags_);
	if (NULL == d)
		return Failure(PRL_ERR_FAILURE);

	virDomainFree(d);
	return Result();
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

///////////////////////////////////////////////////////////////////////////////
// struct Online

Online::Online(const Agent& agent_): Agent(agent_), m_compression(new Compression())
{
}

void Online::setQemuState(qint32 port_)
{
	m_qemuState = QSharedPointer<Qemu::State>(new Qemu::State(port_));
}

void Online::setQemuDisk(qint32 port_, const QList<CVmHardDisk* >& list_)
{
	m_qemuDisk = QSharedPointer<Qemu::Disk>(new Qemu::Disk(port_, list_));
}

Result Online::operator()(const CVmConfiguration& config_)
{
	unsigned int f = VIR_MIGRATE_PERSIST_DEST |
			VIR_MIGRATE_CHANGE_PROTECTION |
			VIR_MIGRATE_LIVE |
			VIR_MIGRATE_AUTO_CONVERGE;

	typedef QList<boost::function1<Result, Parameters::Builder&> > directorList_type;
	directorList_type d;
	if (!m_qemuDisk.isNull())
	{
		f |= VIR_MIGRATE_NON_SHARED_INC;
		d << boost::bind<Result>(boost::ref(*m_qemuDisk.data()), _1);
	}
	if (!m_qemuState.isNull())
		d << boost::bind<Result>(boost::ref(*m_qemuState.data()), _1);

	if (!m_compression.isNull())
		d << boost::bind<Result>(boost::ref(*m_compression.data()), _1);

	Parameters::Builder b;
	foreach(directorList_type::value_type o, d)
	{
		Result e = o(b);
		if (e.isFailed())
			return e;
	}
	// 3s is approximation of live downtime requirements in PCS6
	Result e = setDowntime(3);
	if (e.isFailed())
		return e;

	return migrate(config_, f, b);
};

} // namespace Migration

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virDomainPtr domain_): m_domain(domain_, &virDomainFree)
{
}

Result Unit::kill()
{
	return do_(m_domain.data(), boost::bind(&virDomainDestroy, _1));
}

Result Unit::shutdown()
{
	return do_(m_domain.data(), boost::bind
		(&virDomainShutdownFlags, _1, VIR_DOMAIN_SHUTDOWN_ACPI_POWER_BTN |
			VIR_DOMAIN_SHUTDOWN_GUEST_AGENT));
}

Result Unit::start_(unsigned int flags_)
{
	int s = VIR_DOMAIN_NOSTATE;
	if (-1 == virDomainGetState(m_domain.data(), &s, NULL, 0))
		return Failure(PRL_ERR_VM_GET_STATUS_FAILED);

	if (s == VIR_DOMAIN_CRASHED)
		kill();

	return do_(m_domain.data(), boost::bind(&virDomainCreateWithFlags, _1, flags_));
}

Result Unit::start()
{
	return start_(VIR_DOMAIN_START_FORCE_BOOT);
}

Result Unit::startPaused()
{
	return start_(VIR_DOMAIN_START_PAUSED);
}

Result Unit::reboot()
{
	return do_(m_domain.data(), boost::bind(&virDomainReboot, _1, 0));
}

Result Unit::reset()
{
	return do_(m_domain.data(), boost::bind(&virDomainReset, _1, 0));
}

Result Unit::rename(const QString& to_)
{
	return do_(m_domain.data(), boost::bind(&virDomainRename, _1,
		qPrintable(to_), 0));
}

Result Unit::resume(const QString& sav_)
{
	return do_(getLink().data(), boost::bind
		(&virDomainRestore, _1, qPrintable(sav_)));
}

Result Unit::pause()
{
	return do_(m_domain.data(), boost::bind(&virDomainSuspend, _1));
}

Result Unit::unpause()
{
	return do_(m_domain.data(), boost::bind(&virDomainResume, _1));
}

Result Unit::suspend(const QString& sav_)
{
	return do_(m_domain.data(), boost::bind
		(&virDomainSaveFlags, _1, qPrintable(sav_), (const char* )NULL,
			VIR_DOMAIN_SAVE_RUNNING));
}

Result Unit::undefine()
{
	return do_(m_domain.data(), boost::bind(&virDomainUndefineFlags, _1,
		VIR_DOMAIN_UNDEFINE_SNAPSHOTS_METADATA | VIR_DOMAIN_UNDEFINE_NVRAM));
}

Result Unit::getState(VIRTUAL_MACHINE_STATE& dst_) const
{
	int s = VIR_DOMAIN_NOSTATE, r;
	if (-1 == virDomainGetState(m_domain.data(), &s, &r, 0))
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

Exec::AuxChannel* Unit::getChannel(const QString& path_) const
{
	virStreamPtr stream = virStreamNew(getLink().data(), VIR_STREAM_NONBLOCK);
	if (!stream)
		return NULL;

	int ret = virDomainOpenChannel(m_domain.data(), QSTR2UTF8(path_), stream, 0);
	if (ret) {
		virStreamFree(stream);
		return NULL;
	}

	return new Exec::AuxChannel(stream);
}

QSharedPointer<virConnect> Unit::getLink() const
{
	QSharedPointer<virConnect> output;
	virConnectPtr x = virDomainGetConnect(m_domain.data());
	if (NULL == x)
		return QSharedPointer<virConnect>();

	virConnectRef(x);
	return QSharedPointer<virConnect>(x, &virConnectClose);
}

Result Unit::getConfig(CVmConfiguration& dst_, bool runtime_) const
{
	Config config(m_domain, getLink(), runtime_ ? 0 : VIR_DOMAIN_XML_INACTIVE);
	return config.convert(dst_);
}

Result Unit::getConfig(QString& dst_, bool runtime_) const
{
	Config config(m_domain, getLink(), runtime_ ? 0 : VIR_DOMAIN_XML_INACTIVE);
	Prl::Expected<QString, Error::Simple> s = config.read();
	if (s.isFailed())
		return s.error();
	dst_ = s.value();
	return Result();
}

Result Unit::setConfig(const CVmConfiguration& value_)
{
	QSharedPointer<virConnect> link_ = getLink();
	if (link_.isNull())
		return Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER);

	Config config(m_domain, link_, VIR_DOMAIN_XML_INACTIVE);
	Prl::Expected<QString, Error::Simple> x = config.mixup(value_);
	if (x.isFailed())
		return x.error();

	virDomainPtr d = virDomainDefineXML(link_.data(), x.value().toUtf8().data());
	if (NULL == d)
		return Failure(PRL_ERR_VM_APPLY_CONFIG_FAILED);

	m_domain = QSharedPointer<virDomain>(d, &virDomainFree);
	return Result();
}

Result Unit::completeConfig(CVmConfiguration& config_)
{
	if (m_domain.isNull())
		return Result(Error::Simple(PRL_ERR_UNINITIALIZED));
	foreach(CVmHardDisk *d, config_.getVmHardwareList()->m_lstHardDisks)
	{
		if (d->getEmulatedType() != PVE::HardDiskImage)
			continue;
		virDomainBlockInfo b;
		if (virDomainGetBlockInfo(m_domain.data(), QSTR2UTF8(d->getSystemName()),
			&b, 0) == 0)
		{
			d->setSize(b.capacity >> 20);
			d->setSizeOnDisk(b.allocation >> 20);
		}
	}
	return Result();
}

Migration::Agent Unit::migrate(const QString &uri_)
{
	return Migration::Agent(m_domain, getLink(), uri_);
}

Result Unit::getUuid(QString& dst_) const
{
	char u[VIR_UUID_STRING_BUFLEN] = {};
	if (virDomainGetUUIDString(m_domain.data(), u))
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
	return Guest(m_domain);
}

Runtime Unit::getRuntime() const
{
	return Runtime(m_domain);
}

///////////////////////////////////////////////////////////////////////////////
// struct Performance

Result Performance::setMemoryStatsPeriod(qint64 seconds_)
{
	return do_(m_domain.data(), boost::bind(&virDomainSetMemoryStatsPeriod, _1,
		seconds_, VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE));
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Performance::getCpu() const
{
	int n = virDomainGetCPUStats(m_domain.data(), NULL, 0, -1, 1, 0);
	if (0 >= n)
		return Failure(PRL_ERR_FAILURE);

	QVector<virTypedParameter> q(n);
	if (0 > virDomainGetCPUStats(m_domain.data(), q.data(), n, -1, 1, 0))
		return Failure(PRL_ERR_FAILURE);

	quint64 s = 0;
 #if (LIBVIR_VERSION_NUMBER > 1000001)
	virTypedParamsGetULLong(q.data(), n, "cpu_time", &s);
 #endif
	Stat::CounterList_type r;

	r.append(Stat::Counter_type(::Stat::Name::Cpu::getName(), s / 1000));

	return r;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Performance::getVCpuList() const
{
	int n = virDomainGetVcpusFlags(m_domain.data(), VIR_DOMAIN_VCPU_CURRENT);
	if (0 >= n)
		return Failure(PRL_ERR_FAILURE);

	QVector<virVcpuInfo> c(n);
	if (0 > virDomainGetVcpus(m_domain.data(), c.data(), n, NULL, 0))
		return Failure(PRL_ERR_FAILURE);

	Stat::CounterList_type r;
	foreach (const virVcpuInfo& i, c)
	{
		r.append(Stat::Counter_type(::Stat::Name::VCpu::getName(i.number), i.cpuTime));
	}
	return r;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Performance::getDisk(const CVmHardDisk& disk_) const
{
	virDomainBlockStatsStruct s;
	if (0 > virDomainBlockStats(m_domain.data(), qPrintable(disk_.getTargetDeviceName()), &s, sizeof(s)))
		return Failure(PRL_ERR_FAILURE);

	Stat::CounterList_type r;

	// -1 means the counter is unsupported
	r.append(Stat::Counter_type(
		::Stat::Name::Hdd::getWriteRequests(disk_),
		s.wr_req >= 0 ? s.wr_req : 0));
	r.append(Stat::Counter_type(
		::Stat::Name::Hdd::getWriteTotal(disk_),
		s.wr_bytes >= 0 ? s.wr_bytes : 0));
	r.append(Stat::Counter_type(
		::Stat::Name::Hdd::getReadRequests(disk_),
		s.rd_req >= 0 ? s.rd_req : 0));
	r.append(Stat::Counter_type(
		::Stat::Name::Hdd::getReadTotal(disk_),
		s.rd_bytes >= 0 ? s.rd_bytes : 0));

	return r;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Performance::getMemory() const
{
	unsigned s = VIR_DOMAIN_MEMORY_STAT_NR - 1;
	virDomainMemoryStatStruct x[s];

	int n = virDomainMemoryStats(m_domain.data(), x, s, 0);
	if (0 >= n)
		return Failure(PRL_ERR_FAILURE);

	QHash<int, quint64> v;
	for (int i = 0; i < n; ++i)
		v[x[i].tag] = x[i].val;

	Stat::CounterList_type r;

	quint64 u = v[VIR_DOMAIN_MEMORY_STAT_RSS];
	if (v.contains(VIR_DOMAIN_MEMORY_STAT_UNUSED))
		u = v.value(VIR_DOMAIN_MEMORY_STAT_AVAILABLE) -
			v.value(VIR_DOMAIN_MEMORY_STAT_UNUSED);

	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getUsed(), u));
	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getTotal(),
		v.value(VIR_DOMAIN_MEMORY_STAT_AVAILABLE)));
	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getSwapIn(),
		v.value(VIR_DOMAIN_MEMORY_STAT_SWAP_IN)));
	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getSwapOut(),
		v.value(VIR_DOMAIN_MEMORY_STAT_SWAP_OUT)));
	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getMinorFault(),
		v.value(VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT)));
	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getMajorFault(),
		v.value(VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT)));
	r.append(Stat::Counter_type(
		::Stat::Name::Memory::getBalloonActual(),
		v.value(VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)));

	return r;
}

Prl::Expected<Stat::CounterList_type, Error::Simple>
Performance::getInterface(const CVmGenericNetworkAdapter& iface_) const
{
	virDomainInterfaceStatsStruct x;
	int n = virDomainInterfaceStats(m_domain.data(), qPrintable(iface_.getHostInterfaceName()), &x, sizeof(x));
	if (0 > n)
		return Failure(PRL_ERR_FAILURE);

	Stat::CounterList_type r;

	// -1 means the counter is unsupported
	r.append(Stat::Counter_type(
		::Stat::Name::Interface::getBytesIn(iface_),
		x.rx_bytes >= 0 ? x.rx_bytes : 0));
	r.append(Stat::Counter_type(
		::Stat::Name::Interface::getPacketsIn(iface_),
		x.rx_packets >= 0 ? x.rx_packets : 0));
	r.append(Stat::Counter_type(
		::Stat::Name::Interface::getBytesOut(iface_),
		x.tx_bytes >= 0 ? x.tx_bytes : 0));
	r.append(Stat::Counter_type(
		::Stat::Name::Interface::getPacketsOut(iface_),
		x.tx_packets >= 0 ? x.tx_packets : 0));

	return r;
}

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
			return res;
		QString state = res.value();

		std::istringstream is(state.toUtf8().data());

		// read_json is not thread safe
		QMutexLocker locker(getBoostJsonLock());
		try {
			boost::property_tree::json_parser::read_json(is, r);
		} catch (const boost::property_tree::json_parser::json_parser_error&) {
			return Error::Simple(PRL_ERR_FAILURE);
		}
		std::string status = r.get<std::string>("return.status", std::string("none"));

		// Is state changed?
		if (status != m_status)
		{
			m_status.clear();
			return res;
		}
	}
	while(timeout_ > 0);

	return Error::Simple(PRL_ERR_TIMEOUT);
}

} // namespace Command

///////////////////////////////////////////////////////////////////////////////
// struct Guest

Result Guest::traceEvents(bool enable_)
{
	return execute(QString("trace-event * %1").arg(enable_ ? "on" : "off"));
}

Result Guest::dumpScreen(const QString& path)
{
	return execute(QString("screendump %1").arg(path));
}

Prl::Expected<QString, Error::Simple>
Guest::dumpMemory(const QString& path)
{
	return execute(QString("dump-guest-memory -z %1").arg(path));
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

Result Guest::setUserPasswd(const QString& user_, const QString& passwd_, bool crypted_)
{
	unsigned int f = (crypted_) ? VIR_DOMAIN_CREATE_USER | VIR_DOMAIN_PASSWORD_ENCRYPTED :
					VIR_DOMAIN_CREATE_USER;
	return do_(m_domain.data(), boost::bind
		(&virDomainSetUserPassword, _1, user_.toUtf8().constData(),
			passwd_.toUtf8().constData(), f));
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

Result Guest::checkAgent()
{
       return Exec::Exec(m_domain).executeInAgent(QString("{\"execute\":\"guest-ping\"}"));
}

Prl::Expected<QString, Error::Simple>
Guest::getAgentVersion()
{
	Prl::Expected<QString, Error::Simple> r =
		Exec::Exec(m_domain).executeInAgent(QString("{\"execute\":\"guest-info\"}"));
	if (r.isFailed())
		return r.error();

	std::istringstream is(r.value().toUtf8().data());

	// read_json is not thread safe
	QMutexLocker locker(getBoostJsonLock());
	boost::property_tree::ptree result;
	try {
		boost::property_tree::json_parser::read_json(is, result);
	} catch (const boost::property_tree::json_parser::json_parser_error&) {
		return Error::Simple(PRL_ERR_FAILURE);
	}

	return QString::fromStdString(result.get<std::string>("return.version"));
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

Prl::Expected<Exec::Future, Error::Simple>
Guest::startProgram(const Exec::Request& req)
{
	Exec::Exec e(m_domain);
	Prl::Expected<int, Error::Simple> r = e.runCommand(req);
	if (r.isFailed())
		return r.error();
	return Exec::Future(m_domain, r.value());
}

Prl::Expected<Exec::Result, Error::Simple>
Guest::runProgram(const Exec::Request& req)
{
	Prl::Expected<Exec::Future, Error::Simple> f = startProgram(req);
	if (f.isFailed())
		return f.error();
	Result r = f.value().wait();
	if (r.isFailed())
		return r.error();
	return f.value().getResult().get();
}

///////////////////////////////////////////////////////////////////////////////
// struct Exec

Prl::Expected<boost::optional<Exec::Result>, Libvirt::Agent::Failure>
Exec::Exec::getCommandStatus(int pid)
{
	boost::property_tree::ptree cmd, params;

	params.put("pid", "pid-value"); // replace placeholder later

	cmd.put("execute", "guest-exec-status");
	cmd.add_child("arguments", params);

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, cmd, false);

	// boost json has no int varant, so...
	std::string s = ss.str();
	boost::replace_all<std::string>(s, "\"pid-value\"", boost::lexical_cast<std::string>(pid));

	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		executeInAgent(QString::fromUtf8(s.c_str()));
	if (r.isFailed())
		return r.error();

	std::istringstream is(r.value().toUtf8().data());

	// read_json is not thread safe
	QMutexLocker locker(getBoostJsonLock());
	boost::property_tree::ptree result;
	try {
		boost::property_tree::json_parser::read_json(is, result);
	} catch (const boost::property_tree::json_parser::json_parser_error&) {
		return Libvirt::Agent::Failure(PRL_ERR_FAILURE);
	}

	bool exited = result.get<bool>("return.exited");
	if (exited) {
		Result st;
		st.exitcode = result.get<int>("return.signal", -1);
		if (st.exitcode != -1) {
			st.signaled = true;
		} else {
			st.exitcode = result.get<int>("return.exitcode", -1);
		}

		std::string s;
		s = result.get<std::string>("return.out-data", "");
		st.stdOut = QByteArray::fromBase64(s.c_str());
		s = result.get<std::string>("return.err-data", "");
		st.stdErr = QByteArray::fromBase64(s.c_str());

		return boost::optional<Result>(st);
	}
	return boost::optional<Result>();
}

Prl::Expected<void, Libvirt::Agent::Failure>
Exec::Exec::terminate(int pid)
{
	boost::property_tree::ptree cmd, params;

	params.put("pid", "pid-value"); // replace placeholder later

	cmd.put("execute", "guest-exec-terminate");
	cmd.add_child("arguments", params);

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, cmd, false);

	// boost json has no int varant, so...
	std::string s = ss.str();
	boost::replace_all<std::string>(s, "\"pid-value\"", boost::lexical_cast<std::string>(pid));

	return executeInAgent(QString::fromUtf8(s.c_str()));
}

Prl::Expected<int, Libvirt::Agent::Failure>
Exec::Exec::runCommand(const Libvirt::Instrument::Agent::Vm::Exec::Request& req)
{
	Prl::Expected<QString, Libvirt::Agent::Failure> r = 
		executeInAgent(req.getJson());
	if (r.isFailed())
		return r.error();

	const char * s = r.value().toUtf8().data();
	std::istringstream is(s);

	// read_json is not thread safe
	QMutexLocker locker(getBoostJsonLock());
	boost::property_tree::ptree result;
	try {
		boost::property_tree::json_parser::read_json(is, result);
	} catch (const boost::property_tree::json_parser::json_parser_error&) {
		return Libvirt::Agent::Failure(PRL_ERR_FAILURE);
	}

	return result.get<int>("return.pid");
}

QString Exec::Request::getJson() const
{
	boost::property_tree::ptree cmd, argv, env, params;

	params.put("path", QSTR2UTF8(m_path));
	params.put("capture-output", "capture-output-value"); // replace placeholder later
	params.put("execute-in-shell", "execute-in-shell-value"); // replace placeholder later

	if (m_channels.size() > 0) {
		QPair<int, int> c;
		foreach (c, m_channels) {
			if (c.first == 0)
				params.put("cid-in", "cid-in-value");
			else if (c.first == 1)
				params.put("cid-out", "cid-out-value");
			else if (c.first == 2)
				params.put("cid-err", "cid-err-value");
		}
	}

	if (m_env.size() > 0) {
		foreach (const QString& a, m_env) {
			boost::property_tree::ptree e;
			e.put_value(a.toStdString());
			env.push_back(std::make_pair("", e));
		}
		params.add_child("env", env);
	}

	if (m_args.size() > 0) {
		foreach (const QString& a, m_args) {
			boost::property_tree::ptree e;
			e.put_value(a.toStdString());
			argv.push_back(std::make_pair("", e));
		}
		params.add_child("arg", argv);
	}

	cmd.put("execute", "guest-exec");
	cmd.add_child("arguments", params);

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, cmd, false);

	// boost json has no int varant, so...
	std::string s = ss.str();
	boost::replace_all<std::string>(s, "\"capture-output-value\"", "true");
	boost::replace_all<std::string>(s, "\"execute-in-shell-value\"", m_runInShell ? "true" : "false");
	if (m_channels.size() > 0) {
		QPair<int, int> c;
		foreach (c, m_channels) {
			std::string k;
			if (c.first == 0)
				k = "\"cid-in-value\"";
			else if (c.first == 1)
				k = "\"cid-out-value\"";
			else if (c.first == 2)
				k = "\"cid-err-value\"";
			boost::replace_all<std::string>(s, k,  QString::number(c.second).toStdString());
		}
	}


	return QString::fromStdString(s);
}

Prl::Expected<QString, Libvirt::Agent::Failure>
Exec::Exec::executeInAgent(const QString& cmd)
{
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
	for (int i = 0; ; ++i) {
		s = virDomainQemuAgentCommand(m_domain.data(),
				cmd.toUtf8().constData(), 30, 0);
		if (s != NULL)
			break;

		Libvirt::Agent::Failure r(PRL_ERR_FAILURE);
		if (r.virErrorCode() != VIR_ERR_AGENT_UNSYNCED)
			return r;

		if (i == 30)
			return r;

		HostUtils::Sleep(1000);
	}

	QString reply = QString::fromUtf8(s);
	free(s);
	return reply;
}

///////////////////////////////////////////////////////////////////////////////
// struct Future

int Exec::Future::calculateTimeout(int i) const
{
	switch (i / 10) {
		case 0:
			return 100;
		case 1:
			return 1000;
		default:
			return 10000;
	}
}

Libvirt::Result
Exec::Future::wait(int timeout)
{
	if (m_status)
		return Libvirt::Result();

	Waiter waiter;
	int msecs, total = 0;
	for (int i=0; ; i++) {
		result_type st = Exec(m_domain).getCommandStatus(m_pid);
		if (st.isFailed()) {
			if (!st.error().isTransient() ||
			    ++m_failcnt >= MAX_TRANSIENT_FAILS)
				return st.error();
		} else {
			m_failcnt = 0;
			if (st.value()) {
				m_status = st.value();
				return Libvirt::Result();
			}
		}
		if (timeout == 0)
			return Error::Simple(PRL_ERR_TIMEOUT);
		msecs = calculateTimeout(i);
		waiter.wait(msecs);
		total += msecs;
		if (timeout != -1 && timeout > total)
			return Error::Simple(PRL_ERR_TIMEOUT);
	}
}

void Exec::Future::cancel()
{
	if (m_status)
		return;

	if (m_pid) {
		WRITE_TRACE(DBG_FATAL, "Trying to cancel the guest process %d", m_pid);
		Exec(m_domain).terminate(m_pid);
	}
}

namespace Exec
{

///////////////////////////////////////////////////////////////////////////////
// struct Device

bool Device::open(QIODevice::OpenMode mode_)
{
	m_channel->addIoChannel(*this);
	if (!m_channel->isOpen()) {
		m_channel->removeIoChannel(m_client);
		return false;
	}

	return QIODevice::open(mode_);
}

void Device::close()
{
	m_channel->removeIoChannel(m_client);
	QIODevice::close();
}

///////////////////////////////////////////////////////////////////////////////
// struct ReadDevice

bool ReadDevice::open(QIODevice::OpenMode mode_)
{
	if (mode_ & QIODevice::WriteOnly)
		return false;

	QMutexLocker l(&m_lock);
	return Device::open(mode_);
}

void ReadDevice::close()
{
	setEof();
	Device::close();
}

void ReadDevice::setEof()
{
	QMutexLocker l(&m_lock);
	m_finished = true;
	l.unlock();
	emit readChannelFinished();
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
	quint64 c = (maxSize_ > m_data.size()) ? m_data.size() : maxSize_;
	::memcpy(data_, m_data.constData(), c);
	m_data.remove(0, c);
	return c;
}

qint64 ReadDevice::writeData(const char *data_, qint64 len_)
{
	Q_UNUSED(data_);
	Q_UNUSED(len_);
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// struct WriteDevice

bool WriteDevice::open(QIODevice::OpenMode mode_)
{
	if (mode_ & QIODevice::ReadOnly)
		return false;

	return Device::open(mode_);
}

void WriteDevice::close()
{
	if (m_channel->isOpen()) {
		// write EOF
		m_channel->writeMessage(QByteArray(), m_client);
	}
	Device::close();
}

qint64 WriteDevice::readData(char *data_, qint64 maxSize_)
{
	Q_UNUSED(data_);
	Q_UNUSED(maxSize_);
	return -1;
}

qint64 WriteDevice::writeData(const char *data_, qint64 len_)
{
	QByteArray s = QByteArray(data_, len_);
	return m_channel->writeMessage(s, m_client);
}

///////////////////////////////////////////////////////////////////////////////
// struct AuxChannel

enum {
	AUX_MAGIC_NUMBER = 0x4B58B9CA
};

AuxChannel::AuxChannel(virStreamPtr stream_)
	: m_stream(stream_), m_read(0), m_ioChannelCounter(0)
{
	virStreamEventAddCallback(m_stream,
		VIR_STREAM_EVENT_HANGUP | VIR_STREAM_EVENT_ERROR | VIR_STREAM_EVENT_READABLE,
		&reactEvent, this, NULL);
}

void AuxChannel::readMessage(const QByteArray& data_)
{
	if (m_read < (int)sizeof(AuxMessageHeader)) {
		if (!data_.size())
			return;

		// fill in header first
		int l = qMin((uint32_t)data_.size(), (uint32_t)sizeof(AuxMessageHeader) - m_read);
		memcpy((char *)&m_readHdr + m_read, data_.constData(), l);
		m_read += l;
		return readMessage(data_.mid(l));
	}

	if (m_readHdr.magic != AUX_MAGIC_NUMBER) {
		// trash in the channel, skip till valid header
		return skipTrash(data_);
	}

	ReadDevice *d = static_cast<ReadDevice *>(m_ioChannels.value(m_readHdr.cid));
	if (!d) {
		WRITE_TRACE(DBG_FATAL, "Unknown channel id (%d), dropping header",
				m_readHdr.cid);
		restartRead();
		return readMessage(data_);
	}

	if (!m_readHdr.length) {
		d->setEof(); // eof
		restartRead();
		return readMessage(data_);
	}

	int l = qMin((uint32_t)data_.size(), m_readHdr.length -
				m_read + (uint32_t)sizeof(AuxMessageHeader));
	d->appendData(data_.left(l));
	m_read += l;
	if (m_read == m_readHdr.length + (uint32_t)sizeof(AuxMessageHeader))
		restartRead();
	if (l < data_.size())
		readMessage(data_.mid(l));
}

void AuxChannel::reactEvent(virStreamPtr st_, int events_, void *opaque_)
{
	Q_UNUSED(st_);
	AuxChannel* c = (AuxChannel *)opaque_;
	c->processEvent(events_);
}

void AuxChannel::processEvent(int events_)
{
	if (events_ & (VIR_STREAM_EVENT_HANGUP | VIR_STREAM_EVENT_ERROR)) {
		close();
		return;
	}
	if (events_ & VIR_STREAM_EVENT_READABLE) {
		char buf[1024];
		int got = 0;
		do {
			QMutexLocker l(&m_lock);
			got = virStreamRecv(m_stream, buf, sizeof(buf));
			if (got > 0) {
				readMessage(QByteArray(buf, got));
			} else if (got == 0 || got == -1) {
				l.unlock();
				close();
				return;
			}
		} while (got == sizeof(buf));
	}
}

void AuxChannel::restartRead()
{
	m_read = 0;
}

void AuxChannel::addIoChannel(Device& device_)
{
	QMutexLocker l(&m_lock);
	m_ioChannels.insert(++m_ioChannelCounter, &device_);
	device_.setClient(m_ioChannelCounter);
}

void AuxChannel::removeIoChannel(int id_)
{
	QMutexLocker l(&m_lock);
	m_ioChannels.remove(id_);
	if ((int)m_readHdr.cid == id_)
		restartRead();
}

bool AuxChannel::isOpen()
{
	return (m_stream != NULL);
}

void AuxChannel::close()
{
	QMutexLocker l(&m_lock);
	virStreamEventRemoveCallback(m_stream);
	virStreamFinish(m_stream);
	virStreamFree(m_stream);
	m_stream = NULL;
	QList<Device*> q = m_ioChannels.values();
	l.unlock();
	/* close devices, m_stream should be null for isOpen() to be false */
	foreach (Device* d, q)
		d->close();
}

void AuxChannel::skipTrash(const QByteArray& data_)
{
	unsigned int p = AUX_MAGIC_NUMBER;
	QByteArray x((const char *)&p, (int)sizeof(unsigned int));
	QByteArray d((char *)&m_readHdr, (int)sizeof(AuxMessageHeader));
	d.append(data_);

	int y = d.indexOf(x);
	if (y == -1) {
		// as magic is multibyte, preserve data which 1-byte off
		// comparing to its length.
		m_read = (int)sizeof(AuxMessageHeader) - 1;
		::memcpy((char *)&m_readHdr, d.right(m_read).constData(), m_read);
	} else {
		restartRead();
		readMessage(d.mid(y));
	}
}

int AuxChannel::writeMessage(const QByteArray& data_, int client_)
{
	AuxMessageHeader h;
	h.magic = AUX_MAGIC_NUMBER;
	h.cid = client_;
	h.length = data_.size();

	QByteArray d((const char *)&h, sizeof(AuxMessageHeader));
	d.append(data_);

	QMutexLocker l(&m_lock);
	int sent = virStreamSend(m_stream, d.constData(), d.size());
	if (sent < 0) {
		l.unlock();
		close();
		return -1;
	}
	return sent;
}

AuxChannel::~AuxChannel()
{
	close();
}

}; //namespace Exec

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

Result Hotplug::attach(const QString& device_)
{
	WRITE_TRACE(DBG_DEBUG, "attach device: \n%s", qPrintable(device_));
	return do_(m_domain.data(), boost::bind(virDomainAttachDeviceFlags, _1,
			qPrintable(device_), VIR_DOMAIN_AFFECT_CURRENT |
			VIR_DOMAIN_AFFECT_LIVE));
}

Result Hotplug::detach(const QString& device_)
{
	WRITE_TRACE(DBG_DEBUG, "detach device: \n%s", qPrintable(device_));
	return do_(m_domain.data(), boost::bind(virDomainDetachDeviceFlags, _1,
			qPrintable(device_), VIR_DOMAIN_AFFECT_CURRENT |
			VIR_DOMAIN_AFFECT_LIVE));
}

Result Hotplug::update(const QString& device_)
{
	WRITE_TRACE(DBG_DEBUG, "update device: \n%s", qPrintable(device_));
	return do_(m_domain.data(), boost::bind(virDomainUpdateDeviceFlags, _1,
			qPrintable(device_),
			VIR_DOMAIN_AFFECT_CURRENT |
			VIR_DOMAIN_AFFECT_LIVE |
			VIR_DOMAIN_DEVICE_MODIFY_FORCE));
}

///////////////////////////////////////////////////////////////////////////////
// struct Runtime

Result Runtime::setIoLimit(const CVmHardDisk& disk_, quint32 limit_)
{
	return setBlockIoTune(disk_, VIR_DOMAIN_BLOCK_IOTUNE_TOTAL_BYTES_SEC, limit_);
}

Result Runtime::setIopsLimit(const CVmHardDisk& disk_, quint32 limit_)
{
	return setBlockIoTune(disk_, VIR_DOMAIN_BLOCK_IOTUNE_TOTAL_IOPS_SEC, limit_);
}

Result Runtime::setBlockIoTune(const CVmHardDisk& disk_, const char* param_, quint32 limit_)
{
	virTypedParameterPtr p = NULL;
	qint32 s = 0;
	qint32 m = 0;

	if (do_(&p, boost::bind(&virTypedParamsAddULLong, _1,
					&s, &m, param_, limit_)).isFailed())
		return Failure(PRL_ERR_SET_IOLIMIT);

	Result r = do_(m_domain.data(), boost::bind(&virDomainSetBlockIoTune, _1,
							QSTR2UTF8(disk_.getTargetDeviceName()),
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE));
	virTypedParamsFree(p, s);
	return r;
}

Result Runtime::setIoPriority(quint32 ioprio_)
{
	virTypedParameterPtr p(NULL);
	qint32 s(0);
	qint32 m(0);

	if (do_(&p, boost::bind(&virTypedParamsAddUInt, _1,
					&s, &m, VIR_DOMAIN_BLKIO_WEIGHT, ioprio_)).isFailed())
		return Failure(PRL_ERR_SET_IOPRIO);

	Result r(do_(m_domain.data(), boost::bind(&virDomainSetBlkioParameters, _1,
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));
	virTypedParamsFree(p, s);
	return r;
}

Result Runtime::setPerCpuLimit(quint32 limit_, quint32 period_)
{
	return setCpuLimit(0, limit_, period_);
}

Result Runtime::setGlobalCpuLimit(quint32 limit_, quint32 period_)
{
	return setCpuLimit(limit_, 0, period_);
}

Result Runtime::setCpuLimit(quint32 globalLimit_, quint32 limit_,
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
	Result r(do_(m_domain.data(), boost::bind(&virDomainSetSchedulerParametersFlags, _1,
							p.first.data(), p.second, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));
	return r;
}

Result Runtime::setCpuUnits(quint32 units_)
{
	virTypedParameterPtr p(NULL);
	qint32 s(0);
	qint32 m(0);

	if (do_(&p, boost::bind(&virTypedParamsAddULLong, _1,
					&s, &m, VIR_DOMAIN_SCHEDULER_CPU_SHARES, units_)).isFailed())
		return Failure(PRL_ERR_SET_CPUUNITS);

	Result r(do_(m_domain.data(), boost::bind(&virDomainSetSchedulerParametersFlags, _1,
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));

	virTypedParamsFree(p, s);
	return r;
}

Result Runtime::setCpuCount(quint32 units_)
{
	return do_(m_domain.data(), boost::bind(&virDomainSetVcpus, _1, units_));
}

template<class T>
Result Runtime::plug(const T& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<T>
			::getPlugXml(device_);
	if (x.isFailed())
		return x.error();

	return Hotplug(m_domain).attach(x.value());
}
template Result Runtime::plug<CVmHardDisk>(const CVmHardDisk& device_);
template Result Runtime::plug<CVmSerialPort>(const CVmSerialPort& device_);
template Result Runtime::plug<Transponster::Vm::Reverse::Dimm>
	(const Transponster::Vm::Reverse::Dimm& device_);

template<class T>
Result Runtime::unplug(const T& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<T>
			::getPlugXml(device_);
	if (x.isFailed())
		return x.error();

	return Hotplug(m_domain).detach(x.value());
}
template Result Runtime::unplug<CVmHardDisk>(const CVmHardDisk& device_);
template Result Runtime::unplug<CVmSerialPort>(const CVmSerialPort& device_);

template<class T>
Result Runtime::update(const T& device_)
{
	Prl::Expected<QString, ::Error::Simple> x =
		Transponster::Vm::Reverse::Device<T>
			::getUpdateXml(device_);
	if (x.isFailed())
		return x.error();

	return Hotplug(m_domain).update(x.value());
}
template Result Runtime::update<CVmFloppyDisk>(const CVmFloppyDisk& device_);
template Result Runtime::update<CVmOpticalDisk>(const CVmOpticalDisk& device_);
template Result Runtime::update<CVmGenericNetworkAdapter>
	(const CVmGenericNetworkAdapter& device_);

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

Result List::define(const CVmConfiguration& config_, Unit* dst_)
{
	if (m_link.isNull())
		return Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER);

	Prl::Expected<VtInfo, Error::Simple> i = Host(m_link).getVt();
	if (i.isFailed())
		return i.error();

	Transponster::Vm::Reverse::Vm u(config_);
	if (PRL_FAILED(Transponster::Director::domain(u, i.value())))
		return Error::Simple(PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED);

	QString x = u.getResult();
	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", x.toUtf8().data());
	virDomainPtr d = virDomainDefineXML(m_link.data(), x.toUtf8().data());
	if (NULL == d)
		return Failure(PRL_ERR_VM_NOT_CREATED);

	Unit m(d);
	if (NULL != dst_)
		*dst_ = m;

	return Result();
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

namespace Block
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> Unit::getProgress() const
{
	if (m_domain.isNull())
		return Failure(PRL_ERR_TASK_NOT_FOUND);

	WRITE_TRACE(DBG_DEBUG, "get block job info for disk %s", qPrintable(m_disk));

	virDomainBlockJobInfo info;
	int n = virDomainGetBlockJobInfo(m_domain.data(), m_disk.toUtf8().data(), &info, 0);

	// -1 in case of failure, 0 when nothing found, 1 when info was found.
	switch (n)
	{
	case 1:
		return std::make_pair(info.cur, info.end);
	case 0:
		return Failure(PRL_ERR_TASK_NOT_FOUND);
	case -1:
	default:
		return Failure(PRL_ERR_FAILURE);
	}
}

Result Unit::abort() const
{
	if (0 != virDomainBlockJobAbort(m_domain.data(), m_disk.toUtf8().data(), 0))
		return Failure(PRL_ERR_FAILURE);

	return Result();
}

Result Unit::finish() const
{
	WRITE_TRACE(DBG_DEBUG, "tries to finish block commit");
	if (0 != virDomainBlockJobAbort(m_domain.data(), m_disk.toUtf8().data(), VIR_DOMAIN_BLOCK_JOB_ABORT_PIVOT))
		return Failure(PRL_ERR_FAILURE);

	// VIR_DOMAIN_BLOCK_COMMIT_DELETE is not implemented for qemu so we need this
	if (!QFile::remove(m_path))
		WRITE_TRACE(DBG_FATAL, "unable to remove file %s", qPrintable(m_path));

	return Result();
}

///////////////////////////////////////////////////////////////////////////////
// struct Future

void Future::wait()
{
	if (m_timer == -1)
		return;

	if (!connect(this, SIGNAL(finished()), SLOT(exit())))
	{
		WRITE_TRACE(DBG_FATAL, "can't connect");
		return;
	}
	m_loop.exec();
}

void Future::cancel()
{
	foreach(const Unit& u, m_units)
		u.abort();

	foreach(const Unit& u, m_completed)
		u.abort();

	m_units.clear();
	m_completed.clear();
}

void Future::exit()
{
	m_loop.quit();
}

void Future::timerEvent(QTimerEvent* event_)
{
	killTimer(event_->timerId());

	QList<Unit> units;
	foreach (const Unit& unit, m_units)
	{
		Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> r =	unit.getProgress();
		if (r.isFailed())
		{
			WRITE_TRACE(DBG_FATAL, "unable to check job progress");
			// jobs failed
			continue;
		}

		WRITE_TRACE(DBG_DEBUG, "check job progress %llu, %llu", r.value().first, r.value().second);
		if (r.value().first < r.value().second)
		{
			// check next time
			units << unit;
			continue;
		}

		WRITE_TRACE(DBG_DEBUG, "job completed");
		m_completed << unit;
	}

	m_units = units;

	if (units.isEmpty())
	{
		m_timer = -1;
		emit finished();
		return;
	}

	m_timer = startTimer(500);
}

} // namespace Block

namespace Snapshot
{
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
	List::define(const QString& uuid_, const QString& description_, quint32 flags_)
{
	CVmConfiguration x;
	virDomainRef(m_domain.data());
	Vm::Unit m(m_domain.data());
	Result e = m.getConfig(x);
	if (e.isFailed())
		return e.error();

	VIRTUAL_MACHINE_STATE s;
	if ((e = m.getState(s)).isFailed())
		return e.error();

	Transponster::Snapshot::Reverse y(uuid_, description_, x);
	PRL_RESULT f = Transponster::Director::snapshot(y);
	if (PRL_FAILED(f))
		return Error::Simple(f);

	if (VMS_RUNNING == s || VMS_PAUSED == s)
		y.setMemory();

	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", y.getResult().toUtf8().data());
	virDomainSnapshotPtr p = virDomainSnapshotCreateXML(m_domain.data(),
					y.getResult().toUtf8().data(), flags_);
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

Result List::define(const QString& uuid_, const QString& description_, Unit* dst_)
{
	return translate(define(uuid_, description_,
		VIR_DOMAIN_SNAPSHOT_CREATE_ATOMIC), dst_);
}

Result List::defineConsistent(const QString& uuid_, const QString& description_, Unit* dst_)
{
	return translate(define(uuid_, description_,
		VIR_DOMAIN_SNAPSHOT_CREATE_QUIESCE | VIR_DOMAIN_SNAPSHOT_CREATE_ATOMIC), dst_);
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

Prl::Expected<Block::Unit, ::Error::Simple> List::merge(const CVmHardDisk& disk_)
{
	quint32 flags = VIR_DOMAIN_BLOCK_COMMIT_ACTIVE;

	QString target = Transponster::Vm::Reverse::Device<CVmHardDisk>::getTargetName(disk_);

	WRITE_TRACE(DBG_DEBUG, "commit blocks for disk %s", qPrintable(target));
	if (0 != virDomainBlockCommit(m_domain.data(), target.toUtf8().data(), NULL, NULL, 0, flags))
	{
		WRITE_TRACE(DBG_DEBUG, "failed to commit blocks for disk %s", qPrintable(target));
		return Failure(PRL_ERR_FAILURE);
	}

	return Block::Unit(m_domain, target, disk_.getSystemName());
}

///////////////////////////////////////////////////////////////////////////////
// struct Merge

QSharedPointer<Block::Future> Merge::start()
{
	foreach (const CVmHardDisk& d, m_disks)
	{
		Prl::Expected<Block::Unit, ::Error::Simple> r = m_agent.merge(d);
		if (r.isSucceed())
		{
			m_units << r.value();
		}
		else
			WRITE_TRACE(DBG_FATAL, "unable to merge disk", qPrintable(d.getSystemName()));
	}

	return QSharedPointer<Block::Future>(new Block::Future(m_units));
}

void Merge::stop()
{
	foreach(const Block::Unit& u, m_units)
		u.finish();
	m_units.clear();
}

} // namespace Snapshot
} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virNetworkPtr network_): m_network(network_, &virNetworkFree)
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
	if (m_network.isNull())
		return Failure(PRL_ERR_UNINITIALIZED);

	char* x = virNetworkGetXMLDesc(m_network.data(),
			VIR_NETWORK_XML_INACTIVE);
	if (NULL == x)
		return Failure(PRL_ERR_VM_GET_CONFIG_FAILED);

	WRITE_TRACE(DBG_DEBUG, "xml:\n%s", x);
	Transponster::Network::Direct u(x, 0 < virNetworkIsActive(m_network.data()));
	if (PRL_FAILED(Transponster::Director::network(u)))
		return Failure(PRL_ERR_PARSE_VM_DIR_CONFIG);

	dst_ = u.getResult();
	CVZVirtualNetwork* z = dst_.getVZVirtualNetwork();
	if (NULL != z)
	{
		Libvirt::Instrument::Agent::Interface::Bridge b;
		Libvirt::Result e = Libvirt::Kit.interfaces().find(z->getBridgeName(), b);
		dst_.getHostOnlyNetwork()->
			getParallelsAdapter()->setName(z->getBridgeName());
		if (e.isSucceed())
		{
			dst_.setBoundCardMac(b.getMaster().getMacAddress());
			dst_.setVLANTag(b.getMaster().getVLANTag());
			z->setMasterInterface(b.getMaster().getDeviceName());
		}
		else if (PRL_ERR_NETWORK_ADAPTER_NOT_FOUND == e.error().code())
			dst_.setVZVirtualNetwork(NULL);
		else
			return e;
	}
	return Result();
}

///////////////////////////////////////////////////////////////////////////////
// struct List

Unit List::at(const QString& uuid_) const
{
	if (m_link.isNull())
		return Unit(NULL);

	PrlUuid x(uuid_.toUtf8().data());
	virNetworkPtr n = virNetworkLookupByUUIDString(m_link.data(),
			x.toString(PrlUuid::WithoutBrackets).data());
	if (NULL != n && virNetworkIsPersistent(n) == 1)
		return Unit(n);

	virNetworkFree(n);
	return Unit(NULL);
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

	for (int i = 0; i < z; ++i)
	{
		Unit u(a[i]);
		CVirtualNetwork x;
		if (u.getConfig(x).isSucceed())
			dst_ << u;
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

	Unit u(n);
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

	Unit m(n);
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

///////////////////////////////////////////////////////////////////////////////
// struct List

Result List::all(QList<Bridge>& dst_) const
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	PrlNet::EthAdaptersList m;
	PRL_RESULT e = PrlNet::makeBindableAdapterList(m, false, false);
	if (PRL_FAILED(e))
		return Result(Error::Simple(e));

	virInterfacePtr* a = NULL;
	int z = virConnectListAllInterfaces(m_link.data(), &a, 0);
	if (-1 == z)
		return Failure(PRL_ERR_FAILURE);

	for (int i = 0; i < z; ++i)
	{
		Transponster::Interface::Bridge::Direct u(
			virInterfaceGetXMLDesc(a[i], VIR_INTERFACE_XML_INACTIVE),
			0 < virInterfaceIsActive(a[i]));
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
			dst_ << b;
	}
	free(a);
	return Result();
}

Result List::find(const QString& name_, Bridge& dst_) const
{
	if (name_.startsWith("virbr"))
		return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);

	QList<Bridge> a;
	Result e = all(a);
	if (e.isFailed())
		return e;

	foreach (const Bridge& b, a)
	{
		if (b.getName() == name_)
		{
			dst_ = b;
			return Result();
		}
	}
	return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);
}

Result List::find(const CHwNetAdapter& eth_, Bridge& dst_) const
{
	QList<Bridge> a;
	Result e = all(a);
	if (e.isFailed())
		return e;

	foreach (const Bridge& b, a)
	{
		if (b.getMaster().getMacAddress() == eth_.getMacAddress() &&
			b.getMaster().getVLANTag() == eth_.getVLANTag())
		{
			dst_ = b;
			return Result();
		}
	}
	return Error::Simple(PRL_ERR_BRIDGE_NOT_FOUND_FOR_NETWORK_ADAPTER);
}

Result List::find(const QString& mac_, unsigned short vlan_, CHwNetAdapter& dst_) const
{
	QString name = PrlNet::findAdapterName(mac_, vlan_);

	if (vlan_ == PRL_INVALID_VLAN_TAG)
		return find(name, dst_);
	
	return findBridge(name, dst_);
}

Result List::find(const QString& name_, CHwNetAdapter& dst_) const
{
	if (m_link.isNull())
		return Result(Error::Simple(PRL_ERR_CANT_CONNECT_TO_DISPATCHER));

	virInterfacePtr f = virInterfaceLookupByName(m_link.data(), name_.toUtf8().data());

	if (NULL != f)
	{
		Transponster::Interface::Physical::Direct u(
			virInterfaceGetXMLDesc(f, VIR_INTERFACE_XML_INACTIVE),
			0 < virInterfaceIsActive(f));
		virInterfaceFree(f);

		if (PRL_SUCCEEDED(Transponster::Director::physical(u)))
		{
			dst_ = u.getResult();
			return Result();
		}
	}

	return findBridge(name_, dst_);
}

Result List::findBridge(const QString& name_, CHwNetAdapter& dst_) const
{
	QList<Bridge> a;
	Result e = all(a);
	if (e.isFailed())
		return e;

	foreach (const Bridge& b, a)
	{
		if (b.getMaster().getDeviceName() == name_)
		{
			dst_ = b.getMaster();
			return Result();
		}
	}
	return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);
}

Result List::define(const CHwNetAdapter& eth_, Bridge& dst_)
{
	QList<Bridge> a;
	Result e = all(a);
	if (e.isFailed())
		return e;

	uint x = ~0;
	foreach (const Bridge& b, a)
	{
		if (b.getMaster().getMacAddress() == eth_.getMacAddress()
			&& b.getMaster().getDeviceName() == eth_.getDeviceName())
			return Result(Error::Simple(PRL_ERR_ENTRY_ALREADY_EXISTS));

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
	i->setMhz(h.mhz);

	v.setGlobalCpuLimit(PRL_VM_CPULIMIT_FULL == CDspService::instance()
			->getDispConfigGuard().getDispConfig()
			->getDispatcherSettings()->getCommonPreferences()
			->getWorkspacePreferences()->getVmGuestCpuLimitType());

	return v;
}

///////////////////////////////////////////////////////////////////////////////
// struct Hub

void Hub::setLink(QSharedPointer<virConnect> value_)
{
	m_link = value_.toWeakRef();
	m_execs.clear();
}

QSharedPointer<Vm::Exec::AuxChannel> Hub::addAsyncExec(const Vm::Unit &unit_)
{
	QString u;
	if (unit_.getUuid(u).isFailed())
		return QSharedPointer<Vm::Exec::AuxChannel>(NULL);
	if (m_execs.contains(u)) {
		QSharedPointer<Vm::Exec::AuxChannel> c = m_execs.value(u).toStrongRef();
		if (c)
			return c;
		m_execs.remove(u);
	}

	QSharedPointer<Vm::Exec::AuxChannel> p(
			unit_.getChannel(QString("org.qemu.guest_agent.1")));
	if (p)
		m_execs.insert(u, p.toWeakRef());
	return p;
}

} // namespace Agent
} // namespace Instrument
} // namespace Libvirt
