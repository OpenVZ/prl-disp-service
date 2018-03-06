///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmNetworkHelper.cpp
///
///	This class implements functions to help manage vm network parameters
///
/// @author andreydanin
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include <QMutexLocker>

#include "CDspVmNetworkHelper.h"
#include "CDspService.h"
#include "Tasks/Task_ManagePrlNetService.h"

#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include <boost/bind.hpp>

#include <sys/socket.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>

namespace Network
{
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
	struct ifreq x;
	qstrncpy(x.ifr_name, QSTR2UTF8(device_), sizeof(x.ifr_name));
	x.ifr_acctid = m_id;
	if (::ioctl(m_control.handle(), TUNSETACCTID, &x) == -1)
	{
		WRITE_TRACE(DBG_FATAL, "ioctl(TUNSETACCTID, %s, %u) failed: %m",
			x.ifr_name, x.ifr_acctid);
	}
}

} // namespace Traffic
} // namespace Network;

void CDspVmNetworkHelper::getUsedHostMacAddresses(QSet< QString >& hostMacs)
{
	bool bHostMac = true;
	extractAllVmMacAddresses(hostMacs, bHostMac);
	Task_ManagePrlNetService::getHostMacAddresses(hostMacs);
}


void CDspVmNetworkHelper::extractMacAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config,
		QSet<QString>& macs,
		bool hostAddresses)
{
	QListIterator<CVmHardware*> itH(config->m_lstHardware);
	while (itH.hasNext())
	{
		CVmHardware* hard = itH.next();
		if (!hard)
			continue;

		QListIterator<CVmGenericNetworkAdapter*> itN(hard->m_lstNetworkAdapters);
		while (itN.hasNext())
		{
			CVmGenericNetworkAdapter* adapter(itN.next());
			if (!adapter)
				continue;

			QString mac = hostAddresses ? adapter->getHostMacAddress() : adapter->getMacAddress();
			if (!mac.isEmpty())
				macs.insert(mac.toUpper());
		}
	}
}


bool CDspVmNetworkHelper::extractAllVmMacAddresses(QSet<QString>& macs, bool hostAddresses)
{
	QMultiHash< QString, SmartPtr<CVmConfiguration> >
		vmTotalHash = CDspService::instance()->getVmDirHelper().getAllVmList();
	foreach( QString dirUuid, vmTotalHash.keys() )
	{
		QList< SmartPtr<CVmConfiguration> >
			vmList = vmTotalHash.values( dirUuid );
		for( int idx=0; idx<vmList.size(); idx++)
		{
			SmartPtr<CVmConfiguration> pConfig( vmList[idx] );
			if (!pConfig)
				continue;

			extractMacAddressesFromVMConfiguration(pConfig, macs, hostAddresses);
		}//for idx
	} //foreach( QString dirUuid

	return true;
}


bool CDspVmNetworkHelper::isMacAddrConflicts(const QString& mac,
		const QSet<QString>& used)
{
	foreach (const QString& usedMac, used)
	{
		if (PrlNet::isEqualEthAddress(mac, usedMac))
		{
			WRITE_TRACE(DBG_WARNING, "addr %s conflicts", QSTR2UTF8(mac));
			return true;
		}
	}
	return false;
}


bool CDspVmNetworkHelper::checkAndMakeUniqueMacAddr(QString& mac, const QSet<QString>& used,
		HostUtils::MacPrefixType prefix)
{
	if (mac.isEmpty())
		mac = HostUtils::generateMacAddress(prefix);

	int i = 0;
	while (isMacAddrConflicts(mac, used) && i++ < 20)
	{
		mac = HostUtils::generateMacAddress(prefix);
	}

	if (i > 20)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to gen vm hwaddr");
		return false;
	}

	return true;
}


void CDspVmNetworkHelper::updateHostMacAddresses(SmartPtr<CVmConfiguration> pVmConfig,
		QSet<QString>* pMacs, HostMacUpdateFlags flags)
{
	QSet<QString> localUsed;
	QSet<QString>* pUsed = (pMacs ? pMacs : &localUsed);

	getUsedHostMacAddresses(*pUsed);

	QListIterator<CVmHardware*> itH(pVmConfig->m_lstHardware);
	while (itH.hasNext())
	{
		CVmHardware* hard = itH.next();
		if (!hard)
			continue;

		QListIterator<CVmGenericNetworkAdapter*> itN(hard->m_lstNetworkAdapters);
		while (itN.hasNext())
		{
			CVmGenericNetworkAdapter* adapter(itN.next());
			updateHostMacAddress(pVmConfig, adapter, *pUsed, flags);
		}
	}
}


void CDspVmNetworkHelper::updateHostMacAddress(SmartPtr<CVmConfiguration> pVmConfig,
		CVmGenericNetworkAdapter *pAdapter, const QSet<QString>& used, HostMacUpdateFlags flags)
{
	if (!pAdapter)
		return;

	QString mac = pAdapter->getHostMacAddress();
	if (!mac.isEmpty() && !(flags & HMU_CHECK_NONEMPTY))
		return;

	// Form own mac address list
	QSet<QString> own;
	bool hostAddresses = true;
	CDspVmNetworkHelper::extractMacAddressesFromVMConfiguration(pVmConfig, own, hostAddresses);
	own.insert(pAdapter->getMacAddress()); // add guiest mac to used list

	HostUtils::MacPrefixType prefix = HostUtils::MAC_PREFIX_VM;
	switch (pVmConfig->getVmType()) {
	case PVT_CT:
		prefix = HostUtils::MAC_PREFIX_CT;
		break;
	case PVT_VM:
		prefix = HostUtils::MAC_PREFIX_VM;
		break;
	}

	if (!checkAndMakeUniqueMacAddr(mac, own + used, prefix))
		return;

	WRITE_TRACE(DBG_FATAL, "Update host hwaddr %s of adapter[%d] vm %s (%s)",
			QSTR2UTF8(mac), pAdapter->getIndex(),
			QSTR2UTF8(pVmConfig->getVmIdentification()->getVmName()),
			QSTR2UTF8(pVmConfig->getVmIdentification()->getVmUuid()));

	pAdapter->setHostMacAddress(mac);
}

namespace Network
{

///////////////////////////////////////////////////////////////////////////////
// struct Dao

Dao::Dao(Libvirt::Instrument::Agent::Hub& libvirt_):
	m_networks(libvirt_.networks()), m_interfaces(libvirt_.interfaces())
{
}

PRL_RESULT Dao::list(QList<CVirtualNetwork>& dst_)
{
	QList<Libvirt::Instrument::Agent::Network::Unit> a;
	Libvirt::Result e = m_networks.all(a);
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot list networks!");
		return PRL_ERR_UNEXPECTED;
	}
	QStringList x;
	foreach(Libvirt::Instrument::Agent::Network::Unit n, a)
	{
		CVirtualNetwork y;
		e = n.getConfig(y);
		if (e.isFailed())
		{
			WRITE_TRACE(DBG_FATAL, "Cannot get the network config!");
			return e.error().code();
		}
		dst_ << y;
	}
	return PRL_ERR_SUCCESS;
}

Libvirt::Result Dao::define(CVirtualNetwork model_)
{
	Libvirt::Result z;
	if (PVN_HOST_ONLY == model_.getNetworkType())
	{
		PRL_RESULT e = PrlNet::ValidateHostOnlyNetworkParams(&model_);
		if (PRL_FAILED(e))
			z = Error::Simple(e);
	}
	else
		z = craftBridge(model_);

	if (z.isFailed())
		return z;

	Libvirt::Instrument::Agent::Network::Unit u;
	if ((z = m_networks.define(model_, &u)).isFailed())
		return z;

	u.stop();
	u.start();
	return Libvirt::Result();
}

Libvirt::Result Dao::create(const CVirtualNetwork& model_)
{
	QString x = model_.getNetworkID();
	if (m_networks.find(x).isSucceed())
	{
		WRITE_TRACE(DBG_FATAL, "Duplicated new network ID '%s' !", QSTR2UTF8(x));
		return Error::Simple(PRL_NET_DUPLICATE_VIRTUAL_NETWORK_ID);
	}
	if (PVN_HOST_ONLY == model_.getNetworkType())
		return define(model_);

	QList<Libvirt::Instrument::Agent::Network::Unit> a;
	Libvirt::Result r = m_networks.all(a);
	if (r.isFailed())
		return r;

	foreach(const Libvirt::Instrument::Agent::Network::Unit &u, a)
	{
		CVirtualNetwork n;
		if ((r = u.getConfig(n)).isFailed())
			return r;

		if (model_.getBoundCardMac() == n.getBoundCardMac() &&
			model_.getVLANTag() == n.getVLANTag())
			return Error::Simple(PRL_NET_ADAPTER_ALREADY_USED);
	}

	return define(model_);
}

PRL_RESULT Dao::remove(const CVirtualNetwork& model_)
{
	QString x = model_.getNetworkID();
	Libvirt::Instrument::Agent::Network::Unit u;
	if (m_networks.find(x, &u).isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "The network ID '%s' does not exist !",
			QSTR2UTF8(x));
		return PRL_NET_VIRTUAL_NETWORK_ID_NOT_EXISTS;
	}
	u.stop();
	Libvirt::Result r = u.undefine();
	return (r.isFailed()? r.error().code(): PRL_ERR_SUCCESS);
}

Libvirt::Result Dao::update(const CVirtualNetwork& model_)
{
	CVirtualNetwork w;
	QString x = model_.getNetworkID();
	Libvirt::Instrument::Agent::Network::Unit u = m_networks.at(model_.getUuid());
	if (u.getConfig(w).isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "The network ID '%s' was not found by uuid!", QSTR2UTF8(x));
		return Error::Simple(PRL_NET_VIRTUAL_NETWORK_NOT_FOUND);
	}
	if (w.getNetworkID() != x)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot change the network ID '%s' !", QSTR2UTF8(x));
		return Error::Simple(PRL_NET_DUPLICATE_VIRTUAL_NETWORK_ID);
	}

	PRL_NET_VIRTUAL_NETWORK_TYPE t = w.getNetworkType();
	w = model_;

	if (w.getNetworkType() == PVN_HOST_ONLY && w.getNetworkType() != t)
	{
		w.setBoundCardMac("");
		w.setVZVirtualNetwork(NULL);
		w.setVLANTag();
	}

	return define(w);
}

Libvirt::Result Dao::craftBridge(CVirtualNetwork& network_)
{
	if (PVN_HOST_ONLY == network_.getNetworkType())
		return Error::Simple(PRL_ERR_INVALID_PARAM);

	Libvirt::Result r;
	CHwNetAdapter m;
	if (network_.isBridgedToDefaultAdapter())
	{
		PrlNet::EthAdaptersList a;
		PrlNet::EthAdaptersList::Iterator p;
		PRL_RESULT e = PrlNet::getDefaultBridgedAdapter(a, p);
		if (a.end() != p)
		{
			m.setDeviceName(p->_name);
			m.setMacAddress(PrlNet::ethAddressToString(p->_macAddr));
		}
		else if (PRL_SUCCEEDED(e))
			r = Error::Simple(PRL_ERR_FILE_NOT_FOUND);
	}
	else
	{
		r = m_interfaces.find(network_.getBoundCardMac(), network_.getVLANTag(), m);
	}

	if (r.isFailed())
		return r;

	Libvirt::Instrument::Agent::Interface::Bridge b;
	if ((r = m_interfaces.find(m, b)).isFailed())
	{
		if (r.error().code() == PRL_ERR_BRIDGE_NOT_FOUND_FOR_NETWORK_ADAPTER)
		{
			WRITE_TRACE(DBG_FATAL, "Bridge not found for %s", QSTR2UTF8(m.getDeviceName()));
			return Error::Simple(r.error().code(), m.getDeviceName());
		}
		return r;
	}
	else
		b.start();
	CVZVirtualNetwork* z = new CVZVirtualNetwork();
	z->setBridgeName(b.getName());
	z->setMasterInterface(m.getDeviceName());
	network_.setVZVirtualNetwork(z);
	return Libvirt::Result();
}

Libvirt::Result Dao::attachExisting(CVirtualNetwork model_,
	const QString& bridge_)
{
	if (PVN_HOST_ONLY == model_.getNetworkType())
		return Error::Simple(PRL_ERR_INVALID_ARG);

	CVZVirtualNetwork* z = new CVZVirtualNetwork();
	z->setBridgeName(bridge_);
	model_.setVZVirtualNetwork(z);
	Libvirt::Instrument::Agent::Network::Unit u;
	Libvirt::Result e = m_networks.define(model_, &u);
	if (e.isFailed())
		return e;

	u.start();
	return Libvirt::Result();
}

///////////////////////////////////////////////////////////////////////////////
// struct Routing

void Routing::configure(const CVmConfiguration& config_, bool enable_)
{
	foreach(const CVmGenericNetworkAdapter* a, config_.getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (NULL == a || a->getEmulatedType() != PNA_ROUTED)
			continue;

		execute(*a, enable_);
	}
}

void Routing::reconfigure(const CVmConfiguration& old_, const CVmConfiguration& new_)
{
	QList<CVmGenericNetworkAdapter*> o = old_.getVmHardwareList()->m_lstNetworkAdapters;
	QList<CVmGenericNetworkAdapter*> n = new_.getVmHardwareList()->m_lstNetworkAdapters;

	QList<CVmGenericNetworkAdapter*>::iterator d = std::partition(o.begin(), o.end(), 
			boost::bind(&Routing::find, _1, boost::cref(n)));

	std::for_each(d, o.end(), boost::bind(&Routing::execute, this, _1, false));

	QList<CVmGenericNetworkAdapter*>::iterator e = std::partition(n.begin(), n.end(),
			boost::bind(&Routing::find, _1, boost::cref(o)));

	std::for_each(e, n.end(), boost::bind(&Routing::execute, this, _1, true));
}

bool Routing::find(const CVmGenericNetworkAdapter* adapter_, const QList<CVmGenericNetworkAdapter*>& search_)
{
	QList<CVmGenericNetworkAdapter*>::const_iterator it
		= std::find_if(search_.constBegin(), search_.constEnd(),
			boost::bind(&CVmGenericNetworkAdapter::getHostInterfaceName, _1)
				== adapter_->getHostInterfaceName());

	return it != search_.constEnd() && (*it)->getEmulatedType() == PNA_ROUTED
			&& (*it)->getNetAddresses() == adapter_->getNetAddresses();
}

namespace Difference
{

///////////////////////////////////////////////////////////////////////////////
// struct SearchDomain

SearchDomain::SearchDomain(const general_type& general_, const Config::Dao& devices_):
	m_general(general_.getSearchDomains()), m_devices(devices_.getEligible())
{
}

QStringList SearchDomain::calculate(const general_type& general_, const Config::Dao& devices_)
{
	// add global search domains
	bool y = false;
	QStringList x = m_general;

	// add per adapter search domains to global list for now - due to
	// absence of support in guest parts
	foreach(device_type* a, m_devices)
	{
		device_type* o = devices_.find(a->getSystemName(), a->getIndex());
		if (NULL == o || a->getSearchDomains() != o->getSearchDomains())
			y = true;
		x << a->getSearchDomains();
	}

	// set searchDomains only if something changed
	if ((y || m_general != general_.getSearchDomains()) && !x.isEmpty())
		return x;

	return QStringList();
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

Device::Device(const general_type& general_, const Config::Dao& devices_):
	m_devices(devices_), m_general(&general_),
	m_defaultGwIp4Bridge(devices_.findDefaultGwIp4Bridge()),
	m_defaultGwIp6Bridge(devices_.findDefaultGwIp6Bridge())
{
}

bool Device::isEqual(const device_type* first_, const device_type* second_)
{
	if (first_ == NULL || second_ == NULL)
		return first_ == second_;

	return	first_->getConnected() == second_->getConnected() &&
		first_->getEnabled() == second_->getEnabled() &&
		first_->isAutoApply() == second_->isAutoApply() &&
		first_->getEmulatedType() == second_->getEmulatedType() &&
		PrlNet::isEqualEthAddress(first_->getMacAddress(), second_->getMacAddress()) &&
		first_->getNetAddresses() == second_->getNetAddresses() &&
		first_->getDefaultGateway() == second_->getDefaultGateway() &&
		first_->getDefaultGatewayIPv6() == second_->getDefaultGatewayIPv6() &&
		first_->isConfigureWithDhcp() == second_->isConfigureWithDhcp() &&
		first_->isConfigureWithDhcpIPv6() == second_->isConfigureWithDhcpIPv6() &&
		first_->getDnsIPAddresses() == second_->getDnsIPAddresses() &&
		first_->getVirtualNetworkID() == second_->getVirtualNetworkID();
}

QStringList Device::calculate(const general_type& general_, const Config::Dao& devices_)
{
	QStringList output;
	device_type* v = devices_.findDefaultGwIp4Bridge();
	device_type* w = devices_.findDefaultGwIp6Bridge();
	bool g = ((NULL == m_defaultGwIp4Bridge) != (v == NULL)) ||
		((NULL == m_defaultGwIp6Bridge) != (w == NULL));

	foreach(device_type* a, m_devices.getEligible())
	{
		device_type* o = devices_.find(a->getSystemName(), a->getIndex());
		if (isEqual(o, a) && m_general->isAutoApplyIpOnly() == general_.isAutoApplyIpOnly() &&
				m_general->getDnsIPAddresses() == general_.getDnsIPAddresses() &&
				!(a->getEmulatedType() == PNA_ROUTED && g))
			continue;

		QString m = a->getMacAddress();
		if (!PrlNet::convertMacAddress(m))
			continue;

		// DHCP has no sense with Routed adapters
		if (a->getEmulatedType() == PNA_ROUTED)
			output << Address(*a)(Routed(m, m_defaultGwIp4Bridge, m_defaultGwIp6Bridge));
		else
			output << Address(*a)(Bridge(m));

		// merge per adaper and global dns lists
		// "remove" should be completed only if comes from both
		QStringList d = a->getDnsIPAddresses() +  m_general->getDnsIPAddresses();
		d.removeDuplicates();
		if (d.contains("remove") && d.size() > 1)
			d.removeAll("remove");
		// ipOnly autoApply skips all except ip/route/gw args
		// currently this is just dns ips args
		if (!m_general->isAutoApplyIpOnly() && !d.isEmpty())
		{
			output << "--dns" << m << d.join(" ");
		}
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(const CVmConfiguration& cfg_)
	: m_device(*cfg_.getVmSettings()->getGlobalNetwork(),
		Network::Config::Dao(cfg_.getVmHardwareList()->m_lstNetworkAdapters))
{
	const Network::general_type* a = cfg_.getVmSettings()->getGlobalNetwork();
	Network::Config::Dao x(cfg_.getVmHardwareList()->m_lstNetworkAdapters);

	if (a->isAutoApplyIpOnly())
		return;

	m_searchDomain = SearchDomain(*a, x);
	QString h = a->getHostName();
	if (!h.isEmpty())
		m_hostname = h;
}

QStringList Vm::calculate(const general_type& general_, const Config::Dao& devices_)
{
	QStringList a;
	if (m_searchDomain)
	{
		QStringList x = m_searchDomain.get().calculate(general_, devices_);
		if (!x.isEmpty())
			a << "--search-domain" << x.join(" ");
	}
	if (m_hostname)
	{
		if (m_hostname.get() != general_.getHostName())
			a << "--hostname" << m_hostname.get();
	}
	return a << m_device.calculate(general_, devices_);
}

QStringList Vm::calculate(const CVmConfiguration& start_, unsigned int osType_)
{
	const Network::general_type* a = start_.getVmSettings()->getGlobalNetwork();
	Network::Config::Dao x(start_.getVmHardwareList()->m_lstNetworkAdapters);

	QStringList d = calculate(*a, x);
	if (!d.isEmpty()) {
		d.insert(0, "set");

		QString u((osType_ == PVS_GUEST_TYPE_WINDOWS) ?
				"%programfiles%\\Qemu-ga\\prl_nettool.exe" : "prl_nettool");
		d.insert(0, u);
	}
	return d;
}

} // namespace Difference


namespace Config
{

///////////////////////////////////////////////////////////////////////////////
// struct Dao

Dao::Dao(const list_type& dataSource_): m_dataSource(dataSource_)
{
	foreach (device_type* a, m_dataSource)
	{
		if (a->isAutoApply() && a->getEnabled() == PVE::DeviceEnabled &&
				a->getConnected() == PVE::DeviceConnected)
			m_eligible.push_back(a);
	}
}

device_type* Dao::findDefaultGwIp4Bridge() const
{
	foreach (device_type* a, m_eligible)
	{
		if (a->getEmulatedType() == PNA_ROUTED)
			continue;
		if (a->isConfigureWithDhcp() || !a->getDefaultGateway().isEmpty())
			return a;
	}
	return NULL;
}

device_type* Dao::findDefaultGwIp6Bridge() const
{
	foreach (device_type* a, m_eligible)
	{
		if (a->getEmulatedType() == PNA_ROUTED)
			continue;
		if (a->isConfigureWithDhcpIPv6() || !a->getDefaultGatewayIPv6().isEmpty())
			return a;
	}
	return NULL;
}

device_type* Dao::find(const QString& name_, quint32 index_) const
{
	foreach(device_type* a, m_dataSource)
	{
		if (!name_.isEmpty() && name_ == a->getSystemName())
			return a;
		if (index_ == a->getIndex())
			return a;
	}
	return NULL;
}

} // namespace Config

///////////////////////////////////////////////////////////////////////////////
// struct Routed

Routed::Routed(const QString& mac_, const device_type* defaultGwIp4Bridge_,
	const device_type* defaultGwIp6Bridge_):
	m_mac(mac_), m_defaultGwIp4Bridge(defaultGwIp4Bridge_),
	m_defaultGwIp6Bridge(defaultGwIp6Bridge_)
{
}

std::pair<QString, QString> Routed::getIp4Defaults() const
{
	QString x = QString(DEFAULT_HOSTROUTED_GATEWAY).remove(QRegExp("/.*$")) + " ";
	if (NULL != m_defaultGwIp4Bridge)
		return std::make_pair("remove ", x);

	return std::make_pair(x, "");
}

QString Routed::getIp6DefaultGateway()
{
	return QString(DEFAULT_HOSTROUTED_GATEWAY6).remove(QRegExp("/.*$"));
}

QString Routed::getIp6Gateway() const
{
	return NULL != m_defaultGwIp6Bridge ? "removev6 " : getIp6DefaultGateway() + " ";
}

///////////////////////////////////////////////////////////////////////////////
// struct Address

Address::Address(const device_type& device_): m_device(&device_)
{
	boost::tie(m_v4, m_v6) = NetworkUtils::ParseIps(m_device->getNetAddresses());
}

QStringList Address::operator()(const Routed& mode_)
{
	QString g, r;
	QStringList output, a;
	if (!m_v4.isEmpty())
	{
		a << m_v4;
		boost::tie(g, r) = mode_.getIp4Defaults();
	}
	if (!m_v6.isEmpty())
	{
		a << m_v6;
		g += mode_.getIp6Gateway();
		// Install rules with metric lower than auto-installed
		foreach(const QString &ip, m_v6)
			r += QString("%1=%2m100 ").arg(ip, mode_.getIp6DefaultGateway());
	}
	if (a.isEmpty())
	{
		//ipv6 and ipv4 are configured to DHCP - say prl_nettool
		// remove ips in routed network adapter #PSBM-8099
		output << "--ip" << mode_.getMac() << "remove";
	}
	else
		output << "--ip" << mode_.getMac() << a.join(" ");

	if (!g.isEmpty())
		output << "--gateway" << mode_.getMac() << g;

	if (!r.isEmpty())
		output << "--route" << mode_.getMac() << r;
	else if (g.isEmpty())
		output << "--route" << mode_.getMac() << "remove";

	return output;
}

QStringList Address::operator()(const Bridge& mode_)
{
	QString g;
	QStringList output, a;
	if (m_device->isConfigureWithDhcp())
		output << "--dhcp" << mode_.getMac();
	else if (!m_v4.isEmpty())
	{
		a << m_v4;
		QString d = m_device->getDefaultGateway();
		g = d.isEmpty() ? "remove " : d + " ";
	}
	else if (m_v6.isEmpty() && !m_device->isConfigureWithDhcpIPv6())
	{
		// automatic switch to DHCP if list of IPs empty (#427177)
		output << "--dhcp" << mode_.getMac();
	}
	if (m_device->isConfigureWithDhcpIPv6())
		output << "--dhcpv6" << mode_.getMac();
	else if (!m_v6.isEmpty())
	{
		a << m_v6;
		QString d = m_device->getDefaultGatewayIPv6();
		g += d.isEmpty() ? "removev6 " : d + " ";
	}
	if (a.isEmpty())
		a << "remove";
	output << "--ip" << mode_.getMac() << a.join(" ");

	if (g.isEmpty())
	{
		// bridged && dhcp && dhcp6
		output << "--route" << mode_.getMac() << "remove";
	}
	else
		output << "--gateway" << mode_.getMac() << g;

	return output;
}

} // namespace Network
