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
#include "CDspService.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Libraries/Transponster/Direct.h>
#include <Libraries/Transponster/Reverse.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <Libraries/PrlNetworking/netconfig.h>

namespace Libvirt
{
Tools::Agent::Hub Kit;

///////////////////////////////////////////////////////////////////////////////
// struct Failure

Failure::Failure(PRL_RESULT result_): Error::Simple(result_)
{
#if (LIBVIR_VERSION_NUMBER > 1000004)
	const char* m = virGetLastErrorMessage();
	WRITE_TRACE(DBG_FATAL, "libvirt error %s", m ? : "unknown");
	details() = m;
#endif
}

namespace Tools
{
namespace Agent
{

template<class T, class U>
static Result do_(T* handle_, U action_)
{
	if (NULL == handle_)
		return Result(Error::Simple(PRL_ERR_UNINITIALIZED));

	if (0 == action_(handle_))
		return Result();

	return Failure(PRL_ERR_FAILURE);
}

namespace Vm
{
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

Result Unit::start()
{
	int s = VIR_DOMAIN_NOSTATE;
	if (-1 == virDomainGetState(m_domain.data(), &s, NULL, 0))
		return Failure(PRL_ERR_VM_GET_STATUS_FAILED);

	if (s == VIR_DOMAIN_CRASHED)
		kill();

	return do_(m_domain.data(), boost::bind(&virDomainCreateWithFlags, _1, VIR_DOMAIN_START_FORCE_BOOT));
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
	int s = VIR_DOMAIN_NOSTATE;
	if (-1 == virDomainGetState(m_domain.data(), &s, NULL, 0))
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
	case VIR_DOMAIN_SHUTDOWN:
	case VIR_DOMAIN_SHUTOFF:
		dst_ = VMS_STOPPED;
		break;
	default:
		dst_ = VMS_UNKNOWN;
	}
	return Result();
}

char* Unit::getConfig(bool runtime_) const
{
	return virDomainGetXMLDesc(m_domain.data(), VIR_DOMAIN_XML_SECURE
		| (runtime_ ? 0 : VIR_DOMAIN_XML_INACTIVE));
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
	Prl::Expected<VtInfo, Error::Simple> i = Host(getLink()).getVt();
	if (i.isFailed())
		return i.error();

	char* x = getConfig(runtime_);
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

Result Unit::getConfig(QString& dst_, bool runtime_) const
{
	char* x = getConfig(runtime_);
	if (NULL == x)
		return Result(Error::Simple(PRL_ERR_VM_GET_CONFIG_FAILED));

	dst_ = x;
	free(x);
	return Result();
}

Result Unit::setConfig(const CVmConfiguration& value_)
{
	return List(getLink()).define(value_, this);
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
			d->setSizeOnDisk(b.physical >> 20);
		}
	}
	return Result();
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

Runtime Unit::getRuntime() const
{
	return Runtime(m_domain);
}

///////////////////////////////////////////////////////////////////////////////
// struct Performance

Result Performance::getCpu(quint64& nanoseconds_) const
{
	int n = virDomainGetCPUStats(m_domain.data(), NULL, 0, -1, 1, 0);
	if (0 >= n)
		return Failure(PRL_ERR_FAILURE);

	QVector<virTypedParameter> q(n);
	if (0 > virDomainGetCPUStats(m_domain.data(), q.data(), n, -1, 1, 0))
		return Failure(PRL_ERR_FAILURE);

	nanoseconds_ = 0;
#if (LIBVIR_VERSION_NUMBER > 1000001)
	virTypedParamsGetULLong(q.data(), n, "cpu_time", &nanoseconds_);
#endif
	return Result();
}

Result Performance::getDisk() const
{
	return Result();
}

Result Performance::getMemory() const
{
	virDomainMemoryStatStruct x[7];
	int n = virDomainMemoryStats(m_domain.data(), x, 7, 0);
	if (0 >= n)
		return Failure(PRL_ERR_FAILURE);

	return Result();
}

Result Performance::getNetwork() const
{
	return Result();
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
		boost::property_tree::json_parser::read_json(is, r);
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
	boost::property_tree::ptree result;
	boost::property_tree::json_parser::read_json(is, result);

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

Prl::Expected<boost::optional<Exec::Result>, Error::Simple>
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

	Prl::Expected<QString, Error::Simple> r =
		executeInAgent(QString::fromUtf8(s.c_str()));
	if (r.isFailed())
		return r.error();

	std::istringstream is(r.value().toUtf8().data());
	boost::property_tree::ptree result;
	boost::property_tree::json_parser::read_json(is, result);

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

Prl::Expected<int, Error::Simple>
Exec::Exec::runCommand(const Libvirt::Tools::Agent::Vm::Exec::Request& req)
{
	Prl::Expected<QString, Error::Simple> r = 
		executeInAgent(req.getJson());
	if (r.isFailed())
		return r.error();

	std::istringstream is(r.value().toUtf8().data());
	boost::property_tree::ptree result;
	boost::property_tree::json_parser::read_json(is, result);

	return result.get<int>("return.pid");
}

QString Exec::Request::getJson() const
{
	boost::property_tree::ptree cmd, argv, params;

	params.put("path", QSTR2UTF8(m_path));
	params.put("capture-output", "capture-output-value"); // replace placeholder later
	params.put("execute-in-shell", "execute-in-shell-value"); // replace placeholder later

	if (m_stdin.size() > 0) {
		params.put("input-data", m_stdin.toBase64().data());
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

	return QString::fromStdString(s);
}

Prl::Expected<QString, Error::Simple>
Exec::Exec::executeInAgent(const QString& cmd)
{
	char* s = virDomainQemuAgentCommand(m_domain.data(),
			cmd.toUtf8().constData(), -1, 0);
	if (s == NULL)
		return Failure(PRL_ERR_FAILURE);

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

	Prl::Expected<boost::optional<Result>, Error::Simple> st;
	Waiter waiter;
	int msecs, total = 0;
	for (int i=0; ; i++) {
		Exec e(m_domain);
		st = e.getCommandStatus(m_pid);
		if (st.isFailed())
			return st.error();
		if (st.value()) {
			m_status = st.value();
			return Libvirt::Result();
		}
		msecs = calculateTimeout(i);
		waiter.wait(msecs);
		total += msecs;
		if (timeout && timeout > total)
			return Error::Simple(PRL_ERR_TIMEOUT);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

Result Hotplug::attach(const QString& device_)
{
	return do_(m_domain.data(), boost::bind(virDomainAttachDeviceFlags, _1,
			qPrintable(device_), VIR_DOMAIN_AFFECT_CURRENT |
			VIR_DOMAIN_AFFECT_LIVE));
}

Result Hotplug::detach(const QString& device_)
{
	return do_(m_domain.data(), boost::bind(virDomainDetachDeviceFlags, _1,
			qPrintable(device_), VIR_DOMAIN_AFFECT_CURRENT |
			VIR_DOMAIN_AFFECT_LIVE));
}

Result Hotplug::update(const QString& device_)
{
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

Result Runtime::setCpuLimit(quint32 limit_, quint32 period_)
{
	virTypedParameterPtr p(NULL);
	qint32 s(0);
	qint32 m(0);

	if (do_(&p, boost::bind(&virTypedParamsAddULLong, _1,
					&s, &m, VIR_DOMAIN_SCHEDULER_VCPU_PERIOD, period_)).isFailed())
		return Failure(PRL_ERR_SET_CPULIMIT);

	qint32 l = (limit_ == 0? -1 : period_ * limit_ / 100);
	if (do_(&p, boost::bind(&virTypedParamsAddLLong, _1,
					&s, &m, VIR_DOMAIN_SCHEDULER_VCPU_QUOTA, l)).isFailed())
		return Failure(PRL_ERR_SET_CPULIMIT);

	Result r(do_(m_domain.data(), boost::bind(&virDomainSetSchedulerParametersFlags, _1,
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));

	virTypedParamsFree(p, s);
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

Result Runtime::setMemory(quint32 memsize_)
{
	virDomainInfo info;
	Result r = do_(m_domain.data(), boost::bind(&virDomainGetInfo, _1, &info));
	if (r.isFailed())
		return r;

	unsigned newMemSize = memsize_<<10;
	unsigned oldMemSize = info.memory;
	unsigned maxNumaSize = info.maxMem;
	if (oldMemSize > newMemSize)
	{
		if (oldMemSize > maxNumaSize)
			return Error::Simple(PRL_ERR_FAILURE);

		return do_(m_domain.data(), boost::bind(&virDomainSetMemoryFlags, _1,
			newMemSize,
			VIR_DOMAIN_AFFECT_LIVE));
	}
	if (newMemSize <= maxNumaSize)
	{
		return do_(m_domain.data(), boost::bind(&virDomainSetMemoryFlags, _1,
			newMemSize,
			VIR_DOMAIN_AFFECT_LIVE));
	}
	r = do_(m_domain.data(), boost::bind(&virDomainSetMemoryFlags, _1,
		maxNumaSize,
		VIR_DOMAIN_AFFECT_LIVE));
	if (r.isFailed())
		return r;

	return plug(Transponster::Vm::Reverse::Dimm(0, newMemSize - maxNumaSize));
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

	virDomainPtr d = virDomainDefineXML(m_link.data(), u.getResult().toUtf8().data());
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

Unit Unit::getParent() const
{
	return Unit(virDomainSnapshotGetParent(m_snapshot.data(), 0));
}

Result Unit::revert()
{
	return do_(m_snapshot.data(), boost::bind
		(&virDomainRevertToSnapshot, _1, 0));
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

	if (VMS_RUNNING == s)
		y.setMemory();

	WRITE_TRACE(DBG_FATAL, "xml:\n%s", y.getResult().toUtf8().data());
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

	WRITE_TRACE(DBG_FATAL, "xml:\n%s", x);
	Transponster::Network::Direct u(x, 0 < virNetworkIsActive(m_network.data()));
	if (PRL_FAILED(Transponster::Director::network(u)))
		return Failure(PRL_ERR_PARSE_VM_DIR_CONFIG);

	dst_ = u.getResult();
	CVZVirtualNetwork* z = dst_.getVZVirtualNetwork();
	if (NULL != z)
	{
		Libvirt::Tools::Agent::Interface::Bridge b;
		Libvirt::Result e = Libvirt::Kit.interfaces().find(z->getBridgeName(), b);
		dst_.getHostOnlyNetwork()->
			getParallelsAdapter()->setName(z->getBridgeName());
		if (e.isSucceed())
		{
			dst_.setBoundCardMac(b.getMaster().getMacAddress());
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

	WRITE_TRACE(DBG_FATAL, "xml:\n%s", u.getResult().toUtf8().data());
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
		if (b.getName().startsWith("br"))
			dst_ << b;
	}
	free(a);
	return Result();
}

Result List::find(const QString& name_, Bridge& dst_) const
{
	if (!name_.startsWith("br"))
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
		if (b.getMaster().getMacAddress() == eth_.getMacAddress())
		{
			dst_ = b;
			return Result();
		}
	}
	return Error::Simple(PRL_ERR_NETWORK_ADAPTER_NOT_FOUND);
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
	return v;
}

} // namespace Agent
} // namespace Tools
} // namespace Libvirt
