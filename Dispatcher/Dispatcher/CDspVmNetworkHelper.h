////////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmNetworkHelper.h
///
///	Definition of the class CDspVmNetworkHelper
///	This class implements functions to help manage vm network parameters
///
/// @author andreydanin
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
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
	PRL_RESULT create(const CVirtualNetwork& model_);
	PRL_RESULT remove(const CVirtualNetwork& model_);
	PRL_RESULT update(const CVirtualNetwork& model_);
	PRL_RESULT attachExisting(CVirtualNetwork model_, const QString& bridge_);

private:
	PRL_RESULT define(CVirtualNetwork network_);
	PRL_RESULT craftBridge(CVirtualNetwork& network_);

	Libvirt::Instrument::Agent::Network::List m_networks;
	Libvirt::Instrument::Agent::Interface::List m_interfaces;
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

#endif // __CDspVmNetworkHelper_H_

