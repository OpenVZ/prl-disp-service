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

namespace Tools
{
namespace Agent
{
template<class T, class U>
static PRL_RESULT do_(T* handle_, U action_)
{
	if (NULL == handle_)
		return PRL_ERR_UNINITIALIZED;

	if (0 == action_(handle_))
		return PRL_ERR_SUCCESS;

#if (LIBVIR_VERSION_NUMBER > 1000004)
	const char* m = virGetLastErrorMessage();
	WRITE_TRACE(DBG_FATAL, "libvirt error %s", m ? : "unknown");
#endif
	return PRL_ERR_FAILURE;
}

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virDomainPtr domain_): m_domain(domain_, &virDomainFree)
{
}

PRL_RESULT Unit::kill()
{
	return do_(m_domain.data(), boost::bind(&virDomainDestroy, _1));
}

PRL_RESULT Unit::shutdown()
{
	return do_(m_domain.data(), boost::bind
		(&virDomainShutdownFlags, _1, VIR_DOMAIN_SHUTDOWN_ACPI_POWER_BTN |
			VIR_DOMAIN_SHUTDOWN_GUEST_AGENT));
}

PRL_RESULT Unit::start()
{
	int s = VIR_DOMAIN_NOSTATE;
	if (-1 == virDomainGetState(m_domain.data(), &s, NULL, 0))
		return PRL_ERR_VM_GET_STATUS_FAILED;

	if (s == VIR_DOMAIN_CRASHED)
		kill();

	return do_(m_domain.data(), boost::bind(&virDomainCreateWithFlags, _1, VIR_DOMAIN_START_FORCE_BOOT));
}

PRL_RESULT Unit::reboot()
{
	return do_(m_domain.data(), boost::bind(&virDomainReboot, _1, 0));
}

PRL_RESULT Unit::reset()
{
	return do_(m_domain.data(), boost::bind(&virDomainReset, _1, 0));
}

PRL_RESULT Unit::resume(const QString& sav_)
{
	virConnectPtr x = virDomainGetConnect(m_domain.data());
	if (NULL == x)
		return PRL_ERR_UNINITIALIZED;

	return do_(x, boost::bind
		(&virDomainRestore, _1, qPrintable(sav_)));
}

PRL_RESULT Unit::pause()
{
	return do_(m_domain.data(), boost::bind(&virDomainSuspend, _1));
}

PRL_RESULT Unit::unpause()
{
	return do_(m_domain.data(), boost::bind(&virDomainResume, _1));
}

PRL_RESULT Unit::suspend(const QString& sav_)
{
	return do_(m_domain.data(), boost::bind
		(&virDomainSaveFlags, _1, qPrintable(sav_), (const char* )NULL,
			VIR_DOMAIN_SAVE_RUNNING));
}

PRL_RESULT Unit::undefine()
{
	return do_(m_domain.data(), boost::bind(&virDomainUndefineFlags, _1,
		VIR_DOMAIN_UNDEFINE_SNAPSHOTS_METADATA | VIR_DOMAIN_UNDEFINE_NVRAM));
}

PRL_RESULT Unit::getState(VIRTUAL_MACHINE_STATE& dst_) const
{
	int s = VIR_DOMAIN_NOSTATE;
	if (-1 == virDomainGetState(m_domain.data(), &s, NULL, 0))
		return PRL_ERR_VM_GET_STATUS_FAILED;

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
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::getConfig(CVmConfiguration& dst_, bool runtime_) const
{
	char* x = getConfig(runtime_);
	if (NULL == x)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

//	WRITE_TRACE(DBG_FATAL, "xml:\n%s", x);
	Transponster::Vm::Direct u(x);
	if (PRL_FAILED(Transponster::Director::domain(u)))
		return PRL_ERR_PARSE_VM_CONFIG;
		
	CVmConfiguration* output = u.getResult();
	if (NULL == output)
		return PRL_ERR_FAILURE;

	output->getVmIdentification()
		->setServerUuid(CDspService::instance()
                        ->getDispConfigGuard().getDispConfig()
                        ->getVmServerIdentification()->getServerUuid());
	dst_ = *output;
	delete output;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::getConfig(QString& dst_, bool runtime_) const
{
	char* x = getConfig(runtime_);
	if (NULL == x)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	dst_ = x;
	free(x);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::setConfig(const CVmConfiguration& value_)
{
	virConnectPtr x = virDomainGetConnect(m_domain.data());
	if (NULL == x)
		return PRL_ERR_UNINITIALIZED;

	Transponster::Vm::Reverse::Vm u(value_);
	if (PRL_FAILED(Transponster::Director::domain(u)))
		return PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED;

/*
	virDomainPtr d = virDomainDefineXMLFlags(x, u.getResult().toUtf8().data(),
				VIR_DOMAIN_DEVICE_MODIFY_CONFIG |
				VIR_DOMAIN_DEVICE_MODIFY_CURRENT |
				VIR_DOMAIN_DEVICE_MODIFY_LIVE |
				VIR_DOMAIN_DEVICE_MODIFY_FORCE);
*/
	virDomainPtr d = virDomainDefineXML(x, u.getResult().toUtf8().data());
	if (NULL == d)
		return PRL_ERR_VM_OPERATION_FAILED;

	m_domain = QSharedPointer<virDomain>(d, &virDomainFree);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::completeConfig(CVmConfiguration& config_)
{
	if (m_domain.isNull())
		return PRL_ERR_UNINITIALIZED;
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
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::getUuid(QString& dst_) const
{
	char u[VIR_UUID_STRING_BUFLEN] = {};
	if (virDomainGetUUIDString(m_domain.data(), u))
		return PRL_ERR_FAILURE;

	PrlUuid x(u);
	dst_ = x.toString(PrlUuid::WithBrackets).c_str();
	return PRL_ERR_SUCCESS;
}

Runtime Unit::getRuntime() const
{
	return Runtime(m_domain);
}
///////////////////////////////////////////////////////////////////////////////
// struct Performance

PRL_RESULT Performance::getCpu(quint64& nanoseconds_) const
{
	int n = virDomainGetCPUStats(m_domain.data(), NULL, 0, -1, 1, 0);
	if (0 >= n)
		return PRL_ERR_FAILURE;

	QVector<virTypedParameter> q(n);
	if (0 > virDomainGetCPUStats(m_domain.data(), q.data(), n, -1, 1, 0))
		return PRL_ERR_FAILURE;

	nanoseconds_ = 0;
#if (LIBVIR_VERSION_NUMBER > 1000001)
	virTypedParamsGetULLong(q.data(), n, "cpu_time", &nanoseconds_);
#endif
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Performance::getDisk() const
{
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Performance::getMemory() const
{
	virDomainMemoryStatStruct x[7];
	int n = virDomainMemoryStats(m_domain.data(), x, 7, 0);
	if (0 >= n)
		return PRL_ERR_FAILURE;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Performance::getNetwork() const
{
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Guest

PRL_RESULT Guest::dumpMemory(const QString& path, QString& reply)
{
	return execute(QString("dump-guest-memory -z %1").arg(path), reply);
}

PRL_RESULT Guest::dumpState(const QString& path, QString& reply)
{
	return execute(QString("migrate -s \"exec:gzip -c > %1\"").arg(path), reply);
}

PRL_RESULT Guest::setUserPasswd(const QString& user_, const QString& passwd_)
{
	return do_(m_domain.data(), boost::bind
		(&virDomainSetUserPassword, _1, user_.toUtf8().constData(),
			passwd_.toUtf8().constData(), 0));
}

PRL_RESULT Guest::getCommandStatus(int pid, boost::optional<Guest::ExitStatus>& status)
{
	PRL_RESULT r;
	boost::property_tree::ptree cmd, params;

	params.put("pid", "pid-value"); // replace placeholder later

	cmd.put("execute", "guest-exec-status");
	cmd.add_child("arguments", params);

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, cmd, false);

	// boost json has no int varant, so...
	std::string s = ss.str();
	boost::replace_all<std::string>(s, "\"pid-value\"", boost::lexical_cast<std::string>(pid));

	QString reply;
	r = executeInAgent(QString::fromUtf8(s.c_str()), reply);
	if (PRL_FAILED(r))
		return r;

	std::istringstream is(reply.toUtf8().data());
	boost::property_tree::ptree result;
	boost::property_tree::json_parser::read_json(is, result);

	bool exited = result.get<bool>("return.exited");
	if (exited) {
		Guest::ExitStatus st;
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

		status = st;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Guest::runCommand(const QString& path, const QList<QString>& args,
	const QByteArray& stdIn, int& pid)
{
	PRL_RESULT r;
	boost::property_tree::ptree cmd, argv, params;

	params.put("path", QSTR2UTF8(path));
	params.put("capture-output", "capture-output-value"); // replace placeholder later

	if (stdIn.size() > 0) {
		params.put("input-data", stdIn.toBase64().data());
	}

	if (args.size() > 0) {
		foreach (const QString a, args) {
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

	QString reply;
	r = executeInAgent(QString::fromUtf8(s.c_str()), reply);
	if (PRL_FAILED(r))
		return r;

	std::istringstream is(reply.toUtf8().data());
	boost::property_tree::ptree result;
	boost::property_tree::json_parser::read_json(is, result);

	pid = result.get<int>("return.pid");

	return r;
}

PRL_RESULT Guest::execute(const QString& cmd, QString& reply)
{
	char* result = NULL;
	if (0 != virDomainQemuMonitorCommand(m_domain.data(),
			cmd.toUtf8().constData(),
			&result,
			VIR_DOMAIN_QEMU_MONITOR_COMMAND_HMP))
	{
		return PRL_ERR_FAILURE;
	}
	reply = QString::fromUtf8(result);
	free(result);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Guest::executeInAgent(const QString& cmd, QString& reply)
{
	WRITE_TRACE(DBG_DEBUG, "executeInAgent:\n%s", cmd.toUtf8().constData());
	char* s = virDomainQemuAgentCommand(m_domain.data(),
			cmd.toUtf8().constData(), -1, 0);
	if (s == NULL)
		return PRL_ERR_FAILURE;

	reply = QString::fromUtf8(s);
	WRITE_TRACE(DBG_DEBUG, "execute result:\n%s", s);
	free(s);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Runtime

PRL_RESULT Runtime::setIoLimit(const CVmHardDisk& disk_, quint32 limit_)
{
	return setBlockIoTune(disk_, VIR_DOMAIN_BLOCK_IOTUNE_TOTAL_BYTES_SEC, limit_);
}

PRL_RESULT Runtime::setIopsLimit(const CVmHardDisk& disk_, quint32 limit_)
{
	return setBlockIoTune(disk_, VIR_DOMAIN_BLOCK_IOTUNE_TOTAL_IOPS_SEC, limit_);
}

PRL_RESULT Runtime::setBlockIoTune(const CVmHardDisk& disk_, const char* param_, quint32 limit_)
{
	virTypedParameterPtr p = NULL;
	qint32 s = 0;
	qint32 m = 0;

	if (PRL_FAILED(do_(&p, boost::bind(&virTypedParamsAddULLong, _1,
					&s, &m, param_, limit_))))
		return PRL_ERR_SET_IOLIMIT;

	PRL_RESULT r = do_(m_domain.data(), boost::bind(&virDomainSetBlockIoTune, _1,
							QSTR2UTF8(disk_.getTargetDeviceName()),
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE));
	virTypedParamsFree(p, s);
	return r;
}

PRL_RESULT Runtime::setIoPriority(quint32 ioprio_)
{
	virTypedParameterPtr p(NULL);
	qint32 s(0);
	qint32 m(0);

	if (PRL_FAILED(do_(&p, boost::bind(&virTypedParamsAddUInt, _1,
					&s, &m, VIR_DOMAIN_BLKIO_WEIGHT, ioprio_))))
		return PRL_ERR_SET_IOPRIO;

	PRL_RESULT r(do_(m_domain.data(), boost::bind(&virDomainSetBlkioParameters, _1,
							p, s, VIR_DOMAIN_AFFECT_CURRENT |
							VIR_DOMAIN_AFFECT_CONFIG | VIR_DOMAIN_AFFECT_LIVE)));
	virTypedParamsFree(p, s);
	return r;
}

PRL_RESULT Runtime::changeMedia(const CVmOpticalDisk& device_)
{
	Transponster::Vm::Reverse::Cdrom u(device_);
	u();
	QByteArray b = u.getResult().toUtf8();
	return do_(m_domain.data(), boost::bind(&virDomainUpdateDeviceFlags, _1,
		b.data(), VIR_DOMAIN_AFFECT_LIVE | VIR_DOMAIN_AFFECT_CONFIG));
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

PRL_RESULT List::define(const CVmConfiguration& config_, Unit* dst_)
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	Transponster::Vm::Reverse::Vm u(config_);
	if (PRL_FAILED(Transponster::Director::domain(u)))
		return PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED;

	virDomainPtr d = virDomainDefineXML(m_link.data(), u.getResult().toUtf8().data());
	if (NULL == d)
		return PRL_ERR_VM_NOT_CREATED;

	Unit m(d);
	if (NULL != dst_)
		*dst_ = m;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT List::all(QList<Unit>& dst_)
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	virDomainPtr* a = NULL;
	int z = virConnectListAllDomains(m_link.data(), &a,
					VIR_CONNECT_LIST_DOMAINS_PERSISTENT);
	if (-1 == z)
		return PRL_ERR_FAILURE;

	for (int i = 0; i < z; ++i)
		dst_ << Unit(a[i]);

	free(a);
	return PRL_ERR_SUCCESS;
}

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(virDomainSnapshotPtr snapshot_): m_snapshot(snapshot_, &virDomainSnapshotFree)
{
}

PRL_RESULT Unit::getUuid(QString& dst_) const
{
	const char* n = virDomainSnapshotGetName(m_snapshot.data());
	if (NULL == n)
		return PRL_ERR_INVALID_HANDLE;

	QString x = n;
	if (!PrlUuid::isUuid(x.toStdString()))
		return PRL_ERR_INVALID_HANDLE;

	dst_ = x;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::getState(CSavedStateTree& dst_) const
{
	char* x = virDomainSnapshotGetXMLDesc(m_snapshot.data(), VIR_DOMAIN_XML_SECURE);
	if (NULL == x)
		return PRL_ERR_INVALID_HANDLE;

	Transponster::Snapshot::Direct y(x);
	if (PRL_FAILED(Transponster::Director::snapshot(y)))
		return PRL_ERR_PARSE_VM_DIR_CONFIG;

	dst_ = y.getResult();
	dst_.SetCurrent(1 == virDomainSnapshotIsCurrent(m_snapshot.data(), 0));
	return PRL_ERR_SUCCESS;
}

Unit Unit::getParent() const
{
	return Unit(virDomainSnapshotGetParent(m_snapshot.data(), 0));
}

PRL_RESULT Unit::revert()
{
	return do_(m_snapshot.data(), boost::bind
		(&virDomainRevertToSnapshot, _1, 0));
}

PRL_RESULT Unit::undefine()
{
	return do_(m_snapshot.data(), boost::bind
		(&virDomainSnapshotDelete, _1, 0));
}

PRL_RESULT Unit::undefineRecursive()
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

PRL_RESULT List::all(QList<Unit>& dst_) const
{
	virDomainSnapshotPtr* a = NULL;
	int n = virDomainListAllSnapshots(m_domain.data(), &a, 0);
	if (0 > n)
		return PRL_ERR_INVALID_HANDLE;

	for (int i = 0; i < n; ++i)
	{
		QString u;
		Unit o(a[i]);
		if (PRL_SUCCEEDED(o.getUuid(u)))
			dst_ << o;
	}
	free(a);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT List::define(const QString& uuid_, quint32 flags_, Unit* dst_)
{
	PRL_RESULT e;
	virDomainRef(m_domain.data());
	Vm::Unit m(m_domain.data());
	CVmConfiguration x;
	if (PRL_FAILED(e = m.getConfig(x)))
		return e;

	VIRTUAL_MACHINE_STATE s;
	if (PRL_FAILED(e = m.getState(s)))
		return e;

	Transponster::Snapshot::Reverse y(uuid_, x);
	if (PRL_FAILED(e = Transponster::Director::snapshot(y)))
		return e;

	if (VMS_RUNNING == s)
		y.setMemory();

	WRITE_TRACE(DBG_FATAL, "xml:\n%s", y.getResult().toUtf8().data());
	virDomainSnapshotPtr p = virDomainSnapshotCreateXML(m_domain.data(),
					y.getResult().toUtf8().data(), flags_);
	if (NULL == p)
		return PRL_ERR_FAILURE;

	Unit u(p);
	if (NULL != dst_)
		*dst_ = u;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT List::define(const QString& uuid_, Unit* dst_)
{
	return define(uuid_, VIR_DOMAIN_SNAPSHOT_CREATE_ATOMIC, dst_);
}

PRL_RESULT List::defineConsistent(const QString& uuid_, Unit* dst_)
{
	return define(uuid_, VIR_DOMAIN_SNAPSHOT_CREATE_QUIESCE |
			VIR_DOMAIN_SNAPSHOT_CREATE_ATOMIC, dst_);
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

PRL_RESULT Unit::stop()
{
	return do_(m_network.data(), boost::bind(&virNetworkDestroy, _1));
}

PRL_RESULT Unit::start()
{
	return do_(m_network.data(), boost::bind(&virNetworkCreate, _1));
}

PRL_RESULT Unit::undefine()
{
	return do_(m_network.data(), boost::bind(&virNetworkUndefine, _1));
}

PRL_RESULT Unit::getConfig(CVirtualNetwork& dst_) const
{
	if (m_network.isNull())
		return PRL_ERR_UNINITIALIZED;

	char* x = virNetworkGetXMLDesc(m_network.data(),
			VIR_NETWORK_XML_INACTIVE);
	if (NULL == x)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	WRITE_TRACE(DBG_FATAL, "xml:\n%s", x);
	Transponster::Network::Direct u(x, 0 < virNetworkIsActive(m_network.data()));
	if (PRL_FAILED(Transponster::Director::network(u)))
		return PRL_ERR_PARSE_VM_DIR_CONFIG;
		
	dst_ = u.getResult();
	CVZVirtualNetwork* z = dst_.getVZVirtualNetwork();
	if (NULL != z)
	{
		Libvirt::Tools::Agent::Interface::Bridge b;
		PRL_RESULT e = Libvirt::Kit.interfaces().find(z->getBridgeName(), b);
		dst_.getHostOnlyNetwork()->
			getParallelsAdapter()->setName(z->getBridgeName());
		if (PRL_SUCCEEDED(e))
		{
			dst_.setBoundCardMac(b.getMaster().getMacAddress());
			z->setMasterInterface(b.getMaster().getDeviceName());
		}
		else if (PRL_ERR_FILE_NOT_FOUND == e)
			dst_.setVZVirtualNetwork(NULL);
		else
			return e;
	}
	return PRL_ERR_SUCCESS;
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

PRL_RESULT List::all(QList<Unit>& dst_) const
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	virNetworkPtr* a = NULL;
	int z = virConnectListAllNetworks(m_link.data(), &a,
					VIR_CONNECT_LIST_NETWORKS_PERSISTENT);
	if (-1 == z)
		return PRL_ERR_FAILURE;

	for (int i = 0; i < z; ++i)
	{
		Unit u(a[i]);
		CVirtualNetwork x;
		if (PRL_SUCCEEDED(u.getConfig(x)))
			dst_ << u;
	}
	free(a);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT List::find(const QString& name_, Unit* dst_) const
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	virNetworkPtr n = virNetworkLookupByName(m_link.data(),
			name_.toUtf8().data());
	if (NULL == n)
		return PRL_ERR_FILE_NOT_FOUND;

	Unit u(n);
	if (1 != virNetworkIsPersistent(n))
		return PRL_ERR_FILE_NOT_FOUND;

	CVirtualNetwork x;
	if (PRL_FAILED(u.getConfig(x)))
		return PRL_ERR_FILE_NOT_FOUND;

	if (NULL != dst_)
		*dst_ = u;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT List::define(const CVirtualNetwork& config_, Unit* dst_)
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	Transponster::Network::Reverse u(config_);
	if (PRL_FAILED(Transponster::Director::network(u)))
		return PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED;

	WRITE_TRACE(DBG_FATAL, "xml:\n%s", u.getResult().toUtf8().data());
	virNetworkPtr n = virNetworkDefineXML(m_link.data(), u.getResult().toUtf8().data());
	if (NULL == n)
		return PRL_ERR_VM_NOT_CREATED;

	Unit m(n);
	if (0 != virNetworkSetAutostart(n, 1))
	{
		m.undefine();
		return PRL_ERR_FAILURE;
	}
	if (NULL != dst_)
		*dst_ = m;

	return PRL_ERR_SUCCESS;
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

PRL_RESULT Bridge::stop()
{
	return do_(m_interface.data(), boost::bind(&virInterfaceDestroy, _1, 0));
}

PRL_RESULT Bridge::start()
{
	return do_(m_interface.data(), boost::bind(&virInterfaceCreate, _1, 0));
}

PRL_RESULT Bridge::undefine()
{
	return do_(m_interface.data(), boost::bind(&virInterfaceUndefine, _1));
}

///////////////////////////////////////////////////////////////////////////////
// struct List

PRL_RESULT List::all(QList<Bridge>& dst_) const
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	PrlNet::EthAdaptersList m;
	PRL_RESULT e = PrlNet::makeBindableAdapterList(m, false, false);
	if (PRL_FAILED(e))
		return e;

	virInterfacePtr* a = NULL;
	int z = virConnectListAllInterfaces(m_link.data(), &a, 0);
	if (-1 == z)
		return PRL_ERR_FAILURE;

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
	return PRL_ERR_SUCCESS;
}

PRL_RESULT List::find(const QString& name_, Bridge& dst_) const
{
	if (!name_.startsWith("br"))
		return PRL_ERR_FILE_NOT_FOUND;

	QList<Bridge> a;
	PRL_RESULT e = all(a);
	if (PRL_FAILED(e))
		return e;

	foreach (const Bridge& b, a)
	{
		if (b.getName() == name_)
		{
			dst_ = b;
			return PRL_ERR_SUCCESS;
		}
	}
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT List::find(const CHwNetAdapter& eth_, Bridge& dst_) const
{
	QList<Bridge> a;
	PRL_RESULT e = all(a);
	if (PRL_FAILED(e))
		return e;

	foreach (const Bridge& b, a)
	{
		if (b.getMaster().getMacAddress() == eth_.getMacAddress())
		{
			dst_ = b;
			return PRL_ERR_SUCCESS;
		}
	}
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT List::find(const QString& mac_, CHwNetAdapter& dst_) const
{
	if (m_link.isNull())
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	virInterfacePtr f = virInterfaceLookupByMACString(m_link.data(), mac_.toUtf8().data());
	if (NULL != f)
	{
		Transponster::Interface::Physical::Direct u(
			virInterfaceGetXMLDesc(f, VIR_INTERFACE_XML_INACTIVE),
			0 < virInterfaceIsActive(f));
		virInterfaceFree(f);
		if (PRL_SUCCEEDED(Transponster::Director::physical(u)))
		{
			dst_ = u.getResult();
			return PRL_ERR_SUCCESS;
		}
	}
	QList<Bridge> a;
	PRL_RESULT e = all(a);
	if (PRL_FAILED(e))
		return e;

	foreach (const Bridge& b, a)
	{
		if (b.getMaster().getMacAddress() == mac_)
		{
			dst_ = b.getMaster();
			return PRL_ERR_SUCCESS;
		}
	}
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT List::define(const CHwNetAdapter& eth_, Bridge& dst_)
{
	QList<Bridge> a;
	PRL_RESULT e = all(a);
	if (PRL_FAILED(e))
		return e;

	uint x = ~0;
	foreach (const Bridge& b, a)
	{
		if (b.getMaster().getMacAddress() == eth_.getMacAddress())
			return PRL_ERR_ENTRY_ALREADY_EXISTS;

		x = qMax(x + 1, b.getName().mid(2).toUInt() + 1) - 1;
	}
	forever
	{
		QString n = QString("br%1").arg(++x);
		Transponster::Interface::Bridge::Reverse u(n, eth_);
		if (PRL_FAILED(Transponster::Director::bridge(u)))
			return PRL_ERR_BAD_VM_DIR_CONFIG_FILE_SPECIFIED;

		virInterfacePtr b = virInterfaceDefineXML(m_link.data(), u.getResult().toUtf8().data(), 0);
		if (NULL == b)
			continue;

		dst_ = Bridge(b, eth_);
		return PRL_ERR_SUCCESS;
	}
}

} // namespace Interface
} // namespace Agent
} // namespace Tools
} // namespace Libvirt
