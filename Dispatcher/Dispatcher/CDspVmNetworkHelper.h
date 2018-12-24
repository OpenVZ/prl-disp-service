////////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmNetworkHelper.h
///
///	Definition of the class CDspVmNetworkHelper
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
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmNetworkHelper_H_
#define __CDspVmNetworkHelper_H_

#include <QSet>
#include <QString>
#include "CDspLibvirt.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include <prlcommon/Std/SmartPtr.h>
#include <prlxmlmodel/VmConfig/CVmGenericNetworkAdapter.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/HostUtils/HostUtils.h>

namespace Network
{
namespace Traffic
{
///////////////////////////////////////////////////////////////////////////////
// struct Accounting

struct Accounting
{
	explicit Accounting(const QString& uuid_);
	void operator()(const QString& device_);

private:
	quint32 m_id;
	QFile m_control;
};

} // namespace Traffic
} // namespace Network

enum HostMacUpdateFlags {
	HMU_NONE = 0,
	HMU_CHECK_NONEMPTY = (1 << 0),
};

class CDspVmNetworkHelper
{
	public:
		static void extractMacAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config,
															QSet<QString>& macs,
															bool hostAddresses);
		static QMultiMap<QString, QString> extractAllVmMacAddresses(bool hostAddresses);
		static bool extractAllVmMacAddresses(QSet<QString>& macs, bool hostAddresses);

		/// returns true if mac-address conflicts with some interface in the list/vm mac.
		static bool isMacAddrConflicts(const QString& mac, const QSet<QString>& used);

		/// Check and correct mac address to be unique
		static bool checkAndMakeUniqueMacAddr(QString& mac, const QSet<QString>& used,
				HostUtils::MacPrefixType prefix = HostUtils::MAC_PREFIX_VM);

		/// get mac addresses of (a) physical/virtual host adapters and
		// (b) VM's host macs
		static void getUsedHostMacAddresses(QSet< QString >& hostMacs);

		/// Update host mac address for all vm net adapters
		static void updateHostMacAddresses(SmartPtr<CVmConfiguration> pVmConfig,
			QSet<QString>* usedMacs, HostMacUpdateFlags flags);

	private:
		/// Update host mac address for specified vm net adapter
		static void updateHostMacAddress(SmartPtr<CVmConfiguration> pVmConfig,
			CVmGenericNetworkAdapter *pAdapter, const QSet<QString>& used, HostMacUpdateFlags flags);
};

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Dao

struct Dao
{
	explicit Dao(Libvirt::Instrument::Agent::Hub& libvirt_);

	PRL_RESULT list(QList<CVirtualNetwork>& dst_);
	Libvirt::Result create(const CVirtualNetwork& model_);
	PRL_RESULT remove(const CVirtualNetwork& model_);
	Libvirt::Result update(const CVirtualNetwork& model_);
	Libvirt::Result attachExisting(CVirtualNetwork model_, const QString& bridge_);

private:
	Libvirt::Result define(CVirtualNetwork network_);
	Libvirt::Result craftBridge(CVirtualNetwork& network_);

	Libvirt::Instrument::Agent::Network::List m_networks;
	Libvirt::Instrument::Agent::Interface::List::Frontend m_interfaces;
};

///////////////////////////////////////////////////////////////////////////////
// struct Paver

struct Paver: QRunnable
{
	Paver(const CVmGenericNetworkAdapter& adapter_, bool enable_): m_adapter(adapter_), m_enable(enable_)
	{
	}

	void run()
	{
		Task_ManagePrlNetService::updateAdapter(m_adapter, m_enable);
	}

private:
	CVmGenericNetworkAdapter m_adapter;
	bool m_enable;
};

///////////////////////////////////////////////////////////////////////////////
// struct Routing

struct Routing
{
	Routing()
	{
		m_thread.setMaxThreadCount(1);
	}

	void up(const CVmConfiguration& config_)
	{
		configure(config_, true);
	}
	void down(const CVmConfiguration& config_)
	{
		configure(config_, false);
	}
	void configure(const CVmConfiguration& config_, bool enable_);
	void reconfigure(const CVmConfiguration& old_, const CVmConfiguration& new_);

private:
	static bool find(const CVmGenericNetworkAdapter* adapter_, const QList<CVmGenericNetworkAdapter*>& search_);
	void execute(const CVmGenericNetworkAdapter& adapter_, bool enable_)
	{
		QRunnable* q = new Paver(adapter_, enable_);
		q->setAutoDelete(true);
		m_thread.start(q);
	}

	QThreadPool m_thread;
};

} // namespace Network

namespace Network
{
typedef CVmGlobalNetwork general_type;
typedef CVmGenericNetworkAdapter device_type;

namespace Config
{

///////////////////////////////////////////////////////////////////////////////
// struct Dao

struct Dao
{
	typedef QList<device_type* > list_type;

	explicit Dao(const list_type& dataSource_);

	const list_type& getEligible() const
	{
		return m_eligible;
	}
	device_type* findDefaultGwIp4Bridge() const;
	device_type* findDefaultGwIp6Bridge() const;
	device_type* find(const QString& name_, quint32 index_) const;

private:
	list_type m_eligible;
	list_type m_dataSource;
};

} // namespace Config

///////////////////////////////////////////////////////////////////////////////
// struct Bridge

struct Bridge
{
	explicit Bridge(const QString& mac_): m_mac(mac_)
	{
	}

	const QString& getMac() const
	{
		return m_mac;
	}

private:
	QString m_mac;
};

///////////////////////////////////////////////////////////////////////////////
// struct Routed

struct Routed
{
	Routed(const QString& mac_, const device_type* defaultGwIp4Bridge_,
		const device_type* defaultGwIp6Bridge_);

	const QString& getMac() const
	{
		return m_mac;
	}
	std::pair<QString, QString> getIp4Defaults() const;
	QString getIp6Gateway() const;
	static QString getIp6DefaultGateway();

private:
	QString m_mac;
	const device_type* m_defaultGwIp4Bridge;
	const device_type* m_defaultGwIp6Bridge;
};

///////////////////////////////////////////////////////////////////////////////
// struct Address

struct Address
{
	explicit Address(const device_type& device_);

	QStringList operator()(const Routed& mode_);
	QStringList operator()(const Bridge& mode_);

private:
	QStringList m_v4;
	QStringList m_v6;
	const device_type* m_device;
};

namespace Difference
{
///////////////////////////////////////////////////////////////////////////////
// struct SearchDomain

struct SearchDomain
{
	SearchDomain(const general_type& general_, const Config::Dao& devices_);

	QStringList calculate(const general_type& general_, const Config::Dao& devices_);

private:
	QStringList m_general;
	QList<device_type* > m_devices;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device
{
	Device(const general_type& general_, const Config::Dao& devices_);

	QStringList calculate(const general_type& general_, const Config::Dao& devices_);

private:
	static bool isEqual(const device_type* first_, const device_type* second_);

	Config::Dao m_devices;
	const general_type* m_general;
	const device_type* m_defaultGwIp4Bridge;
	const device_type* m_defaultGwIp6Bridge;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	explicit Vm(const CVmConfiguration& cfg_);

	QStringList calculate(const general_type& general_, const Config::Dao& devices_);
	QStringList calculate(const CVmConfiguration& start_, unsigned int osType_);

private:
	Device m_device;
	boost::optional<QString> m_hostname;
	boost::optional<SearchDomain> m_searchDomain;
};

} // namespace Difference

} // namespace Network

#endif // __CDspVmNetworkHelper_H_

