///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmNetworkHelper.cpp
///
///	This class implements functions to help manage vm network parameters
///
/// @author andreydanin
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

#include <QMutexLocker>

#include "CDspVmNetworkHelper.h"
#include "CDspService.h"
#include "CDspVmDirHelper.h"
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
#ifdef TUNSETACCTID
	struct ifreq x;
	qstrncpy(x.ifr_name, QSTR2UTF8(device_), sizeof(x.ifr_name));
	x.ifr_acctid = m_id;
	if (::ioctl(m_control.handle(), TUNSETACCTID, &x) == -1)
	{
		WRITE_TRACE(DBG_FATAL, "ioctl(TUNSETACCTID, %s, %u) failed: %m",
			x.ifr_name, x.ifr_acctid);
	}
#else // TUNSETACCTID
	Q_UNUSED(device_);
#endif // TUNSETACCTID
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

QMultiMap<QString, QString> CDspVmNetworkHelper::extractAllVmMacAddresses(bool hostAddresses)
{
	Vm::Directory::Dao::Locked d;
	QMultiMap<QString, QString> output;
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		PRL_RESULT e;
		SmartPtr<CVmConfiguration> c = CDspService::instance()->getVmDirHelper()
			.getVmConfigByDirectoryItem(SmartPtr<CDspClient>(NULL), i.second, e, true, false);
		if (!c.isValid())
			continue;

		QSet<QString> x;
		CDspVmNetworkHelper::extractMacAddressesFromVMConfiguration(c, x, hostAddresses);
		foreach (const QString& s, x)
		{
			output.insert(s, i.second->getVmUuid());
		}
	}

	return output;
}

bool CDspVmNetworkHelper::extractAllVmMacAddresses(QSet<QString>& macs, bool hostAddresses)
{
	macs.unite(extractAllVmMacAddresses(hostAddresses).keys().toSet());
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
		mac = HostUtils::generateMacAddress(prefix).toUpper();

	int i = 0;
	while (isMacAddrConflicts(mac, used) && i++ < 20)
	{
		mac = HostUtils::generateMacAddress(prefix).toUpper();
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

	CVmHardware* hard = pVmConfig->getVmHardwareList();
	if (NULL != hard)
	{
		std::for_each(hard->m_lstNetworkAdapters.begin(), hard->m_lstNetworkAdapters.end(),
			boost::bind(&updateHostMacAddress, pVmConfig, _1, boost::cref(*pUsed), flags));
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
	CDspVmNetworkHelper::extractMacAddressesFromVMConfiguration(pVmConfig, own, true);
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
	Libvirt::Result e = m_networks.all(dst_);
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot list networks!");
		return PRL_ERR_UNEXPECTED;
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

	QList<CVirtualNetwork> a;
	Libvirt::Result r = m_networks.all(a);
	if (r.isFailed())
		return r;

	foreach(const CVirtualNetwork &n, a)
	{
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
} // namespace Network
