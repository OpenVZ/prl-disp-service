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

#include "CDspService.h"
#include "CDspLibvirt_p.h"
#include <boost/function.hpp>
#include <QtCore/qmetatype.h>
#include "CDspVmStateSender.h"
#include "CDspVmNetworkHelper.h"
#include "Tasks/Task_CreateProblemReport.h"
#include "Tasks/Task_BackgroundJob.h"
#include <Libraries/PrlUuid/PrlUuid.h>
#include <Libraries/Transponster/Direct.h>
#include <Libraries/Transponster/Reverse.h>
#include <Libraries/PrlNetworking/netconfig.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>

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

PRL_RESULT Unit::changeMedia(const CVmOpticalDisk& device_)
{
	Transponster::Vm::Reverse::Cdrom u(device_);
	u();
	QByteArray b = u.getResult().toUtf8();
	return do_(m_domain.data(), boost::bind(&virDomainUpdateDeviceFlags, _1,
		b.data(), VIR_DOMAIN_AFFECT_LIVE | VIR_DOMAIN_AFFECT_CONFIG));
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
	return execute(QString("dump-guest-memory -p %1").arg(path), reply);
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

///////////////////////////////////////////////////////////////////////////////
// struct Domain

Domain::Domain(virDomainPtr model_, QSharedPointer<View::Domain> view_):
	m_agent(model_), m_view(view_)
{
	virDomainRef(model_);
}

void Domain::run()
{
	CVmConfiguration c;
	if (PRL_SUCCEEDED(m_agent.getConfig(c)))
		m_view->setConfig(c);	

	VIRTUAL_MACHINE_STATE s;
	if (PRL_SUCCEEDED(m_agent.getState(s)))
		m_view->setState(s);
}

namespace Breeding
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

void Vm::operator()(Agent::Hub& hub_)
{
	QList<Agent::Vm::Unit> a;
	hub_.vms().all(a);
	foreach (Agent::Vm::Unit m, a)
	{
		QString u;
		m.getUuid(u);
		QSharedPointer<View::Domain> v = m_view->add(u);
		if (v.isNull())
			v = m_view->find(u);

		if (!v.isNull())
			Domain(m, v).run();
	}
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

Subject::Subject(QSharedPointer<virConnect> libvirtd_, QSharedPointer<View::System> view_):
	m_vm(view_), m_network(ParallelsDirs::getNetworkConfigFilePath())
{
	m_hub.setLink(libvirtd_);
}

void Subject::run()
{
	m_vm(m_hub);
	m_network(m_hub);
}

} // namespace Breeding

///////////////////////////////////////////////////////////////////////////////
// struct Performance

void Performance::run()
{
	QList<Agent::Vm::Unit> a;
	m_agent.all(a);
	foreach (Agent::Vm::Unit m, a)
	{
		QString u;
		m.getUuid(u);
		QSharedPointer<View::Domain> v = m_view->find(u);
		if (v.isNull())
			continue;

		Agent::Vm::Performance p = m.getPerformance();
		VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
		m.getState(s);
		if (VMS_RUNNING == s)
		{
			quint64 u;
			p.getCpu(u);
			p.getMemory();
		}
			
	}
}

/*
void Performance::pull(Agent::Vm::Unit agent_)
{
	QString u;
	m.getUuid(u);
	QSharedPointer<View::Domain> d = m_view->find(domain_);
	if (d.isNull())
		return;

	int n;
	n = virDomainGetCPUStats(&domain_, NULL, 0, -1, 1, 0);
	if (0 < n)
	{
		QVector<virTypedParameter> q(n);
		virDomainGetCPUStats(&domain_, q.data(), n, -1, 1, 0);
		d->setCpuUsage();
	}
	n = virDomainMemoryStats(&domain_, NULL, 0,
		VIR_DOMAIN_MEMORY_STAT_UNUSED |
		VIR_DOMAIN_MEMORY_STAT_AVAILABLE |
		VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON);
	if (0 < n)
	{
		QVector<virDomainMemoryStatStruct> q(n);
		virDomainMemoryStats(&domain_, q.data(), n,
			VIR_DOMAIN_MEMORY_STAT_UNUSED |
			VIR_DOMAIN_MEMORY_STAT_AVAILABLE |
			VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON);
		d->setMemoryUsage();
	}
//	virDomainBlockStats();
//	virDomainInterfaceStats();
}
*/

namespace Traffic
{

///////////////////////////////////////////////////////////////////////////////
// struct Accounting

Accounting::Accounting(const QString& uuid_)
	: m_id(Uuid::toVzid(uuid_)), m_control("/dev/net/tun")
{
}

void Accounting::operator()(const QString& device_)
{
	if (!m_control.isOpen() && !m_control.open(QIODevice::ReadWrite))
	{
		WRITE_TRACE(DBG_FATAL, "failed to open %s: %s",
			QSTR2UTF8(m_control.fileName()),
			QSTR2UTF8(m_control.errorString()));
		return;
	}
	struct tun_acctid x;
	qstrncpy(x.ifname, QSTR2UTF8(device_), sizeof(x.ifname));
	x.acctid = m_id;
	if (::ioctl(m_control.handle(), TUNSETACCTID, &x) == -1)
	{
		WRITE_TRACE(DBG_FATAL, "ioctl(TUNSETACCTID, %s, %u) failed: %m",
			x.ifname, x.acctid);
	}
}

} // namespace Traffic
} // namespace Tools

namespace Callback
{
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
// struct Hub

void Hub::add(int id_, virEventTimeoutCallback callback_)
{
	m_timeoutMap.insert(id_, new Timeout(callback_, id_));
}

void Hub::add(int id_, int socket_, virEventHandleCallback callback_)
{
	m_socketMap.insert(id_, new Socket(socket_, callback_, id_));
}

void Hub::setOpaque(int id_, void* opaque_, virFreeCallback free_)
{
	boost::ptr_map<int, Socket>::iterator a = m_socketMap.find(id_);
	if (m_socketMap.end() != a)
		return a->second->setOpaque(opaque_, free_);

	boost::ptr_map<int, Timeout>::iterator b = m_timeoutMap.find(id_);
	if (m_timeoutMap.end() != b)
		return b->second->setOpaque(opaque_, free_);
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
	qRegisterMetaType<virEventHandleCallback>("virEventHandleCallback");
	qRegisterMetaType<virEventTimeoutCallback>("virEventTimeoutCallback");
	m_generator = QAtomicInt(1);
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
	QMetaObject::invokeMethod(h.data(), "setOpaque", Q_ARG(int, output), Q_ARG(void*, opaque_),
		Q_ARG(virFreeCallback, free_));
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
	QMetaObject::invokeMethod(h.data(), "setOpaque", Q_ARG(int, output), Q_ARG(void*, opaque_),
		Q_ARG(virFreeCallback, free_));
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

int Access::remove(int id_)
{
	QSharedPointer<Hub> h = m_hub.toStrongRef();
	if (h.isNull())
		return -1;

	QMetaObject::invokeMethod(h.data(), "remove", Q_ARG(int, id_));
	return 0;
}

static Access g_access;

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

namespace Plain
{
template<class T>
void delete_(void* opaque_)
{
        delete (T* )opaque_;
}

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
	View::Coarse* v = (View::Coarse* )opaque_;
	v->setState(domain_, VMS_RUNNING);
	return 0;
}

int reboot(virConnectPtr connect_, virDomainPtr domain_, void* opaque_)
{
	return wakeUp(connect_, domain_, 0, opaque_);
}

int deviceConnect(virConnectPtr , virDomainPtr domain_, const char *device_,
	void *opaque_)
{
	Q_UNUSED(device_);
	Q_UNUSED(domain_);
	Q_UNUSED(opaque_);
/*
	// XXX: enable this for vme* devices when network device hotplug is fixed
	View::Coarse* v = (View::Coarse* )opaque_;
	QSharedPointer<View::Domain> d = v->access(domain_);
	if (!d.isNull())
	{
		CVmConfiguration c = d->getConfig();
		Tools::Traffic::Accounting(c.getVmIdentification()->getVmUuid())(device_);
	}
*/
	return 0;
}

int deviceDisconnect(virConnectPtr , virDomainPtr domain_, const char* device_,
                        void* opaque_)
{
	Q_UNUSED(device_);
	Q_UNUSED(domain_);
	Q_UNUSED(opaque_);
/*
        Adapter::System* b = (Adapter::System* )opaque_;
        Adapter::Domain a = b.getDomain(*domain_);
        SmartPtr<CVmConfiguration> x = a.getAdaptee().getConfig();
        // update the device state
	a.getAdaptee().setConfig(x);
*/
	return 0;
}

int lifecycle(virConnectPtr , virDomainPtr domain_, int event_,
                int detail_, void* opaque_)
{

	QSharedPointer<View::Domain> d;
	View::Coarse* v = (View::Coarse* )opaque_;
	switch (event_)
	{
	case VIR_DOMAIN_EVENT_DEFINED:
		d = v->access(domain_);
		if (!d.isNull())
		{
			QRunnable* q = new Tools::Domain(domain_, d);
			q->setAutoDelete(true);
			QThreadPool::globalInstance()->start(q);
		}
		break;
	case VIR_DOMAIN_EVENT_UNDEFINED:
		v->remove(domain_);
		break;
	case VIR_DOMAIN_EVENT_STARTED:
	case VIR_DOMAIN_EVENT_RESUMED:
		v->setState(domain_, VMS_RUNNING);
		break;
	case VIR_DOMAIN_EVENT_SUSPENDED:
		switch (detail_)
		{
		case VIR_DOMAIN_EVENT_SUSPENDED_PAUSED:
		case VIR_DOMAIN_EVENT_SUSPENDED_MIGRATED:
		case VIR_DOMAIN_EVENT_SUSPENDED_IOERROR:
		case VIR_DOMAIN_EVENT_SUSPENDED_WATCHDOG:
		case VIR_DOMAIN_EVENT_SUSPENDED_RESTORED:
		case VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT:
		case VIR_DOMAIN_EVENT_SUSPENDED_API_ERROR:
			v->setState(domain_, VMS_PAUSED);
			break;
		}
		break;
	case VIR_DOMAIN_EVENT_PMSUSPENDED:
		switch (detail_)
		{
		case VIR_DOMAIN_EVENT_PMSUSPENDED_MEMORY:
			v->setState(domain_, VMS_PAUSED);
			break;
		case VIR_DOMAIN_EVENT_PMSUSPENDED_DISK:
			v->setState(domain_, VMS_SUSPENDED);
			break;
		}
		break;
	case VIR_DOMAIN_EVENT_STOPPED:
		switch (detail_)
		{
		case VIR_DOMAIN_EVENT_STOPPED_SAVED:
			v->setState(domain_, VMS_SUSPENDED);
			break;
		default:
			v->setState(domain_, VMS_STOPPED);
			break;
		}
		break;
	case VIR_DOMAIN_EVENT_SHUTDOWN:
		v->setState(domain_, VMS_STOPPED);
		break;
	case VIR_DOMAIN_EVENT_CRASHED:
		switch (detail_)
		{
		case VIR_DOMAIN_EVENT_CRASHED_PANICKED:
			WRITE_TRACE(DBG_FATAL, "VM \"%s\" got guest panic.", virDomainGetName(domain_));
			v->sendProblemReport(domain_);
			break;
		default:
			v->setState(domain_, VMS_STOPPED);
			break;
		}
		break;
	}
	return 0;
}

void error(void* opaque_, virErrorPtr value_)
{
	Q_UNUSED(value_);
	Q_UNUSED(opaque_);
	WRITE_TRACE(DBG_FATAL, "connection error: %s", value_->message);
}

} // namespace Plain
} // namespace Callback

namespace View
{
///////////////////////////////////////////////////////////////////////////////
// struct Domain

Domain::Domain(const QString& uuid_, const SmartPtr<CDspClient>& user_):
	m_pid(), m_uuid(uuid_), m_user(user_), m_state(VMS_UNKNOWN),
	m_formerState(VMS_UNKNOWN)
{
}

void Domain::setState(VIRTUAL_MACHINE_STATE value_)
{
	m_formerState = m_state;
	m_state = value_;
	CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
	if (s.isValid())
	{
		s->onVmStateChanged(m_formerState, m_state, m_uuid,
			m_user->getVmDirectoryUuid(), false);
	}
}

void Domain::setConfig(CVmConfiguration& value_)
{
	// NB. there is no home in a libvirt VM config. it is still required
	// by different activities. now we put it into the default VM folder
	// from a user profile. later this behaviour would be re-designed.
	QString n = value_.getVmIdentification()->getVmName();
	QString h = QDir(m_user->getUserDefaultVmDirPath())
		.absoluteFilePath(QString(n).append(VMDIR_DEFAULT_BUNDLE_SUFFIX));
	m_home = QDir(h).absoluteFilePath(VMDIR_DEFAULT_VM_CONFIG_FILE);
	value_.getVmIdentification()->setHomePath(m_home);
	QScopedPointer<CVmDirectoryItem> x(new CVmDirectoryItem());
	x->setVmUuid(m_uuid);
	x->setVmName(n);
	x->setVmHome(m_home);
	x->setVmType(value_.getVmType());
	x->setValid(PVE::VmValid);
	x->setRegistered(PVE::VmRegistered);
	PRL_RESULT e = CDspService::instance()->getVmDirHelper()
			.insertVmDirectoryItem(m_user->getVmDirectoryUuid(), x.data());
	if (PRL_SUCCEEDED(e))
		x.take();

	boost::optional<CVmConfiguration> y = getConfig();
	if (y)
	{
		Vm::Config::Repairer<Vm::Config::untranslatable_types>
			::type::do_(value_, y.get());
	}
	Kit.vms().at(m_uuid).completeConfig(value_);
	CDspService::instance()->getVmConfigManager().saveConfig(
		SmartPtr<CVmConfiguration>(&value_, SmartPtrPolicy::DoNotReleasePointee),
		m_home, m_user, true, false);
}

boost::optional<CVmConfiguration> Domain::getConfig() const
{
	PRL_RESULT e = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> x = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(m_user, m_uuid, e);
	if (PRL_FAILED(e) || !x.isValid())
		return boost::none;

	return *x;
}

void Domain::setCpuUsage()
{
}

void Domain::setDiskUsage()
{
}

void Domain::setMemoryUsage()
{
}

void Domain::setNetworkUsage()
{
}

///////////////////////////////////////////////////////////////////////////////
// struct System

System::System(): m_configGuard(&CDspService::instance()->getDispConfigGuard())
{
}

void System::remove(const QString& uuid_)
{
	domainMap_type::iterator p = m_domainMap.find(uuid_);
	if (m_domainMap.end() == p)
		return;

	p.value()->setState(VMS_UNKNOWN);
	m_domainMap.erase(p);
}

QSharedPointer<Domain> System::add(const QString& uuid_)
{
	if (uuid_.isEmpty() || m_domainMap.contains(uuid_))
		return QSharedPointer<Domain>();

	SmartPtr<CDspClient> u;
	QString d = m_configGuard->getDispWorkSpacePrefs()->getDefaultVmDirectory();
	foreach (CDispUser* s, m_configGuard->getDispUserPreferences()->m_lstDispUsers)
	{
		if (d == s->getUserWorkspace()->getVmDirectory())
		{
			u = CDspClient::makeServiceUser(d);
			u->setUserSettings(s->getUserId(), s->getUserName());
			break;
		}
	}
	if (!u.isValid())
		return QSharedPointer<Domain>();

	QSharedPointer<Domain> x(new Domain(uuid_, u));
	return m_domainMap[uuid_] = x;
}

QSharedPointer<Domain> System::find(const QString& uuid_) const
{
	domainMap_type::const_iterator p = m_domainMap.find(uuid_);
	if (m_domainMap.end() == p)
		return QSharedPointer<Domain>();

	return p.value();
}

///////////////////////////////////////////////////////////////////////////////
// struct Coarse

QString Coarse::getUuid(virDomainPtr domain_)
{
	QString output;
	virDomainRef(domain_);
	Tools::Agent::Vm::Unit(domain_).getUuid(output);
	return output;
}

void Coarse::setState(virDomainPtr domain_, VIRTUAL_MACHINE_STATE value_)
{
	QSharedPointer<Domain> d = m_fine->find(getUuid(domain_));
	if (!d.isNull())
		d->setState(value_);
}

void Coarse::remove(virDomainPtr domain_)
{
	m_fine->remove(getUuid(domain_));
}

void Coarse::sendProblemReport(virDomainPtr domain_)
{
	CProtoCommandPtr cmd(new CProtoSendProblemReport(QString(), getUuid(domain_), 0));
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdSendProblemReport, cmd);
	QString vmDir = CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->getDefaultVmDirectory();
	SmartPtr<CDspClient> c = CDspClient::makeServiceUser(vmDir);
	CDspService::instance()->getTaskManager().schedule(new Task_CreateProblemReport(c, p));
}

QSharedPointer<Domain> Coarse::access(virDomainPtr domain_)
{
	QString u = getUuid(domain_);
	QSharedPointer<Domain> output = m_fine->find(u);
	if (output.isNull())
		output = m_fine->add(u);

	return output;
}

} // namespace View

namespace Monitor
{
///////////////////////////////////////////////////////////////////////////////
// struct State

State::State(QSharedPointer<View::System> system_): m_system(system_)
{
	CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
	if (s.isValid())
	{
		this->connect(s.getPtr(),
			SIGNAL(signalVmStateChanged(unsigned, unsigned, QString, QString)),
			SLOT(updateConfig(unsigned, unsigned, QString, QString)));
		this->connect(s.getPtr(),
			SIGNAL(signalVmStateChanged(unsigned, unsigned, QString, QString)),
			SLOT(tuneTraffic(unsigned, unsigned, QString, QString)));
	}
}

void State::updateConfig(unsigned oldState_, unsigned newState_, QString vmUuid_, QString dirUuid_)
{
	Q_UNUSED(oldState_);
	Q_UNUSED(dirUuid_);

	if (VMS_RUNNING != newState_)
		return;

	QSharedPointer<View::Domain> d = m_system->find(vmUuid_);
	if (d.isNull())
		return;

	boost::optional<CVmConfiguration> y = d->getConfig();
	if (!y)
		return;

	CVmConfiguration runtime;
	Tools::Agent::Vm::Unit v = Kit.vms().at(vmUuid_);
	if (PRL_FAILED(v.getConfig(runtime, true)))
		return;

	Vm::Config::Repairer<Vm::Config::revise_types>::type::do_(y.get(), runtime);
	d->setConfig(y.get());
}

void State::tuneTraffic(unsigned oldState_, unsigned newState_,
	QString vmUuid_, QString dirUuid_)
{
	Q_UNUSED(oldState_);
	Q_UNUSED(dirUuid_);

	if (newState_ != VMS_RUNNING)
		return;
	QSharedPointer<View::Domain> d = m_system->find(vmUuid_);
	if (d.isNull())
		return;
	boost::optional<CVmConfiguration> c = d->getConfig();
	if (!c)
		return;
	Tools::Traffic::Accounting x(vmUuid_);
	foreach (CVmGenericNetworkAdapter *a, c->getVmHardwareList()->m_lstNetworkAdapters)
	{
		x(QSTR2UTF8(a->getHostInterfaceName()));
	}
	Task_NetworkShapingManagement::setNetworkRate(*c);
}

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

Domains::Domains(int timeout_): m_eventState(-1), m_eventReboot(-1),
	m_eventWakeUp(-1), m_eventDeviceConnect(-1), m_eventDeviceDisconnect(-1),
	m_view(new View::System()), m_stateWatcher(m_view)
{
	m_timer.stop();
	m_timer.setInterval(timeout_);
	m_timer.setSingleShot(false);
	this->connect(&m_timer, SIGNAL(timeout()), SLOT(getPerformance()));
}

void Domains::setConnected(QSharedPointer<virConnect> libvirtd_)
{
	m_libvirtd = libvirtd_.toWeakRef();
	Kit.setLink(libvirtd_);
	m_timer.start();
	m_eventState = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_LIFECYCLE,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::Plain::lifecycle),
							new View::Coarse(m_view),
							&Callback::Plain::delete_<View::Coarse>);
	m_eventReboot = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_REBOOT,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::Plain::reboot),
							new View::Coarse(m_view),
							&Callback::Plain::delete_<View::Coarse>);
	m_eventWakeUp = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_PMWAKEUP,
							VIR_DOMAIN_EVENT_CALLBACK(&Callback::Plain::wakeUp),
							new View::Coarse(m_view),
							&Callback::Plain::delete_<View::Coarse>);
	m_eventDeviceConnect = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_DEVICE_ADDED,
							VIR_DOMAIN_EVENT_CALLBACK(Callback::Plain::deviceConnect),
							new View::Coarse(m_view),
							&Callback::Plain::delete_<View::Coarse>);
	m_eventDeviceDisconnect = virConnectDomainEventRegisterAny(libvirtd_.data(),
							NULL,
							VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED,
							VIR_DOMAIN_EVENT_CALLBACK(Callback::Plain::deviceDisconnect),
							new View::Coarse(m_view),
							&Callback::Plain::delete_<View::Coarse>);
	QRunnable* q = new Tools::Breeding::Subject(m_libvirtd, m_view);
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

void Domains::getPerformance()
{
	QSharedPointer<virConnect> x = m_libvirtd.toStrongRef();
	if (x.isNull())
		return;

	QRunnable* q = new Tools::Performance(x, m_view);
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

void Domains::setDisconnected()
{
	m_timer.stop();
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
	m_libvirtd.clear();
}

} // namespace Monitor

///////////////////////////////////////////////////////////////////////////////
// struct Host

void Host::run()
{
	Monitor::Link a;
	Monitor::Domains b;
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

} // namespace Libvirt
