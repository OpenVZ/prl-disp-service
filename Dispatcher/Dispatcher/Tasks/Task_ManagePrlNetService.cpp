////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Task_ManagePrlNetService.cpp
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	sdmitry@parallels.com
///	SergeyT@parallels.com
///
////////////////////////////////////////////////////////////////////////////////

#include "../CDspService.h"
#include "XmlModel/DispConfig/CDispatcherConfig.h"
#include <XmlModel/DispConfig/CDispNetAdapter.h>
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/VmConfig/CVmNetworkRates.h"

#include "CDspClientManager.h"
#include "CDspVmManager.h"
#include "Task_ManagePrlNetService.h"
#include "Task_BackgroundJob.h"
#include "Libraries/Logging/Logging.h"
#include "Interfaces/ParallelsQt.h"
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include <Libraries/PrlNetworking/netconfig.h>
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "Libraries/PrlCommonUtilsBase/NetworkUtils.h"
#include "Libraries/Virtuozzo/CVzPrivateNetwork.h"
#include "Dispatcher/Dispatcher/CDspService.h"
#include "Dispatcher/Dispatcher/CDspVmNetworkHelper.h"

#include "Libraries/Std/PrlAssert.h"

#include "CDspVzHelper.h"
#ifdef _LIN_
#include <net/if.h>
#endif /* _LIN_ */

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include <memory>

using namespace Parallels;

static int getPrlAdapterIdx(CVirtualNetwork* pVirtualNetwork)
{
	if (   ! pVirtualNetwork->isEnabled()
		|| pVirtualNetwork->getNetworkType() == PVN_BRIDGED_ETHERNET
		|| ! pVirtualNetwork->getHostOnlyNetwork()->getParallelsAdapter()->isEnabled()
		)
	{
		return PAI_INVALID_ADAPTER;
	}
	int nAdapterIndex = pVirtualNetwork->getHostOnlyNetwork()->
		getParallelsAdapter()->getPrlAdapterIndex();
	if (nAdapterIndex < 0)
		return nAdapterIndex;
	return GET_PRL_ADAPTER_NUMBER(nAdapterIndex);
}

static int generatePrlAdapterIdx(CParallelsNetworkConfig* pNetworkConfig, bool bShared)
{
	QByteArray baVacants(PrlNet::getMaximumAdapterIndex() + 1, 0);

	foreach(CVirtualNetwork* pVirtNet, pNetworkConfig->getVirtualNetworks()->m_lstVirtualNetwork)
	{
		int nAdapterIndex = getPrlAdapterIdx(pVirtNet);
		if (nAdapterIndex >= 0 && nAdapterIndex < baVacants.size())
			baVacants[nAdapterIndex] = 1;
	}

	// index 0 reserved for shared networking
	if (bShared && baVacants[0] == (char)1)
		return PAI_INVALID_ADAPTER;
	if (!bShared)
		baVacants[0] = 1;
	int nGenIdx = baVacants.indexOf((char )0);
	if (nGenIdx >= 0 && nGenIdx <= PrlNet::getMaximumAdapterIndex())
		return nGenIdx;

	return PAI_INVALID_ADAPTER;
}

/**
 *  Task_ManagePrlNetService
 */

Task_ManagePrlNetService::Task_ManagePrlNetService(
    SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p ) :
	CDspTaskHelper( client, p )
{
	m_nCmd = getCmdNumber();
}

Task_ManagePrlNetService::Task_ManagePrlNetService(
    SmartPtr<CDspClient>& client, const SmartPtr<IOPackage>& p, PVE::IDispatcherCommands nCmd ) :
	CDspTaskHelper( client, p ), m_nCmd(nCmd)
{
}

PVE::IDispatcherCommands Task_ManagePrlNetService::getCmdNumber()
{
	return (PVE::IDispatcherCommands)getRequestPackage()->header.type;
}

PRL_RESULT Task_ManagePrlNetService::run_body()
{
	PRL_RESULT ret = PRL_ERR_FIXME;

	try
	{
		switch ( m_nCmd )
		{
		case PVE::DspCmdNetPrlNetworkServiceStart:
			// Issueing start for the already started PrlNetService will cause its restart
			ret = PrlNet::startPrlNetService ( ParallelsDirs::getParallelsApplicationDir(), PrlNet::SrvAction::Start );
			break;

		case PVE::DspCmdNetPrlNetworkServiceRestart:
			ret = restartPrlNetService();
			break;

		case PVE::DspCmdNetPrlNetworkServiceStop:
			ret = PrlNet::startPrlNetService ( ParallelsDirs::getParallelsApplicationDir(), PrlNet::SrvAction::Stop );
			break;

		case PVE::DspCmdNetPrlNetworkServiceRestoreDefaults:
			ret = cmdNetPrlNetworkServiceRestoreDefaults();
			break;

		case PVE::DspCmdAddNetAdapter:
			ret = cmdAddNetAdapter();
			break;

		case PVE::DspCmdDeleteNetAdapter:
			ret = cmdDeleteNetAdapter();
			break;

		case PVE::DspCmdUpdateNetAdapter:
			ret = cmdUpdateNetAdapter();
			break;

		case PVE::DspCmdAddVirtualNetwork:
			ret = cmdAddVirtualNetwork();
			break;

		case PVE::DspCmdUpdateVirtualNetwork:
			ret = cmdUpdateVirtualNetwork();
			break;

		case PVE::DspCmdDeleteVirtualNetwork:
			ret = cmdDeleteVirtualNetwork();
			break;
		case PVE::DspCmdUpdateNetworkClassesConfig:
			ret = cmdUpdateNetworkClassesConfig();
			break;
		case PVE::DspCmdUpdateNetworkShapingConfig:
			ret = cmdUpdateNetworkShapingConfig();
			break;
		case PVE::DspCmdRestartNetworkShaping:
			ret = cmdRestartNetworkShaping();
			break;
		case PVE::DspCmdAddIPPrivateNetwork:
			ret = cmdAddIPPrivateNetwork();
			break;

		case PVE::DspCmdUpdateIPPrivateNetwork:
			ret = cmdUpdateIPPrivateNetwork();
			break;

		case PVE::DspCmdRemoveIPPrivateNetwork:
			ret = cmdRemoveIPPrivateNetwork();
			break;
		default:
			ret = PRL_ERR_INVALID_ARG;
			break;
		}

		CVmEvent event( PET_DSP_EVT_COMMON_PREFS_CHANGED,
							QString(),
							PIE_DISPATCHER);

		SmartPtr<IOPackage> p =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

		CDspService::instance()->getClientManager().sendPackageToAllClients( p );

		//Update host info now in view of virtual network adapters list can be changed
		CDspService::instance()->getHostInfo()->refresh();
		event.setEventType(PET_DSP_EVT_HW_CONFIG_CHANGED);
		p = DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

		CDspService::instance()->getClientManager().sendPackageToAllClients( p );

		if ( PRL_FAILED ( ret ) )
			throw ret;

		getLastError()->setEventCode(ret);

	}
	catch ( PRL_RESULT code )
	{
		ret = code;
		getLastError()->setEventCode(ret);
		if ( ret == PRL_NET_SYSTEM_ERROR )
		{
			QString errNumStr = QString::number( PrlNet::getSysError(), 16 );
			errNumStr.prepend( "0x" );
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::UnsignedInt,
				errNumStr,
				EVT_PARAM_MESSAGE_PARAM_0));
		}
		WRITE_TRACE(DBG_FATAL, "error: %d was catched. reason %s", ret, QSTR2UTF8( PrlNet::getSysErrorText() ) );
	}

	return ret;
}

/**
 * @brief Sends Parallels Net service status.
 */
PRL_RESULT Task_ManagePrlNetService::getNetServiceStatus (
    PRL_SERVICE_STATUS_ENUM_PTR pnStatus )
{
		PRL_RESULT ret = PRL_ERR_FIXME;

		if ( !pnStatus )
			return ret;

		*pnStatus = PSS_UNKNOWN;

		PrlNet::SrvStatus::Status currStatus;
		ret = PrlNet::getPrlNetServiceStatus( &currStatus );
		if ( PRL_SUCCEEDED(ret) )
			*pnStatus = ( PRL_SERVICE_STATUS_ENUM )currStatus;

		return ret;
}


PRL_RESULT Task_ManagePrlNetService::cmdNetPrlNetworkServiceRestoreDefaults()
{
#if 0
	QString ParallelsDriversDir = ParallelsDirs::getParallelsDriversDir();

#if !defined(_WIN_)
	// Unconditionally try to load prl_netbridge.
	int err = PrlNet::loadPrlNetbridgeKext( ParallelsDriversDir );
	if (err)
	{
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to load Parallels Networking driver %d.", err );
	}
#endif

	// Create list of host network adapters.
	PrlNet::EthAdaptersList prlAdaptersList;

	PRL_RESULT prlResult = PrlNet::makePrlAdaptersList(prlAdaptersList);
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "[RestoreDefaults] Failed to create list of host ethernet adapters.");

#if defined(_WIN_)
		return prlResult;
#else
		WRITE_TRACE(DBG_FATAL, "[RestoreDefaults]: Trying to restart network.");
		QString ParallelsInstallDir = QString(getenv(PVS_VM_EXECUTABLE_ENV));
		if( ParallelsInstallDir.isEmpty() )
		{
			ParallelsInstallDir = QCoreApplication::applicationDirPath();
		}

		prlResult = PrlNet::startNetworking(ParallelsInstallDir, ParallelsDriversDir);
		if (PRL_FAILED(prlResult))
		{
			WRITE_TRACE(DBG_FATAL, "[RestoreDefaults] Failed to start networking: error 0x%08x", prlResult);
		}

		prlResult = PrlNet::makePrlAdaptersList(prlAdaptersList);
		if (PRL_FAILED(prlResult))
		{
			WRITE_TRACE(DBG_FATAL, "[RestoreDefaults] Second try of create list of host ethernet adapters failed.");
			return prlResult;
		}
#endif // _WIN_
	}

	// Get networking configuration pointer.
	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	// Remember old config
	CParallelsNetworkConfig oldNetworkConfig = *pNetworkConfig.getPtr();

	// Fill networking params.
	QStringList RemovedNetworksIDs;
	QList<int> RemovedAdapters;
	CVirtualNetworks *pNetworks = pNetworkConfig->getVirtualNetworks();
	if (NULL == pNetworks)
	{
		pNetworks = new CVirtualNetworks;
		pNetworkConfig->setVirtualNetworks(pNetworks);
	}
	else
	{
		// Remove all networks
		while ( ! pNetworks->m_lstVirtualNetwork.empty() )
		{
			CVirtualNetwork* pVirtNet = pNetworks->m_lstVirtualNetwork.takeFirst();
			pNetworks->m_lstVirtualNetwork.removeAll( pVirtNet );
			if ( ! pVirtNet ) continue;

			int nAdapterIndex = getPrlAdapterIdx(pVirtNet);
			if (nAdapterIndex >= 0)
				RemovedAdapters.append(nAdapterIndex);
			RemovedNetworksIDs.append( pVirtNet->getNetworkID() );
			delete pVirtNet;
		}

	}

	// Make sure that default networks are present in configuration
	// with default params
	PrlNet::FillDefaultNetworks(pNetworks);

	// Cleanup preserved network names and adapters
	foreach(CVirtualNetwork *pVirtualNetwork, pNetworks->m_lstVirtualNetwork)
	{
		RemovedNetworksIDs.removeAll( pVirtualNetwork->getNetworkID() );
		RemovedAdapters.removeAll( getPrlAdapterIdx( pVirtualNetwork ) );
	}

	// synchronize config with VZ
#ifdef _LIN_
	if (CVzHelper::is_vz_running())
	{
		foreach(QString sNetworkID, RemovedNetworksIDs)
		{
			prlResult = getVzVNetHelper().DeleteVirtualNetwork(sNetworkID);
			if (PRL_FAILED(prlResult)
				&& prlResult != PRL_NET_VIRTUAL_NETWORK_ID_NOT_EXISTS)
				WRITE_TRACE(DBG_FATAL, "[RestoreDefaults] Failed to remove Vz"
					" Virtual Network '%s', error = 0x%08x",
					QSTR2UTF8(sNetworkID), prlResult);
		}

		bool bConfigChanged;
		PrlNet::VZSyncConfig(pNetworkConfig.getPtr(), &bConfigChanged);
	}
#endif

	PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	pNetworkConfig.unlock();  // NOTE:  unlock() should be called on last line!

	// Detach VMs from deleted networks
	foreach(QString sNetworkId, RemovedNetworksIDs)
		updateVmNetworkSettings(QString(), sNetworkId);

	restartPrlNetService();

	return PRL_ERR_SUCCESS;
#endif
	WRITE_TRACE(DBG_FATAL, "FIXME #116344: method %s Is NOT_IMPLEMENTED !!!!! ", __FUNCTION__ );
	return PRL_ERR_UNIMPLEMENTED;
}


static PRL_RESULT getFirstStrParam(const SmartPtr<IOService::IOPackage> &p, QString& sParam)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(p);
	if (!cmd->IsValid())
		return PRL_ERR_INVALID_ARG;

	CProtoCommandWithOneStrParam* pParam
		= CProtoSerializer::CastToProtoCommand<CProtoCommandWithOneStrParam>(cmd);
	if (!pParam)
		return PRL_ERR_INVALID_ARG;

	sParam = pParam->GetFirstStrParam();
	LOG_MESSAGE(DBG_WARNING, "Incoming params: %s", QSTR2UTF8(sParam));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdAddNetAdapter()
{
	WRITE_TRACE(DBG_FATAL, "FIXME #116344: method %s Is NOT_IMPLEMENTED !!!!! ", __FUNCTION__ );
	return PRL_ERR_UNIMPLEMENTED;
}


PRL_RESULT Task_ManagePrlNetService::cmdDeleteNetAdapter()
{
	WRITE_TRACE(DBG_FATAL, "FIXME #116344: method %s Is NOT_IMPLEMENTED !!!!! ", __FUNCTION__ );
	return PRL_ERR_UNIMPLEMENTED;
}


PRL_RESULT Task_ManagePrlNetService::cmdUpdateNetAdapter()
{
	QString sParam;
	PRL_RESULT prlResult = getFirstStrParam(getRequestPackage(), sParam);
	if (PRL_FAILED(prlResult))
		return prlResult;

	CDispNetAdapter oldNetAdapter;
	if( !StringToElement<CDispNetAdapter*>(&oldNetAdapter, sParam) )
		return PRL_ERR_INVALID_ARG;

	LOG_MESSAGE( DBG_WARNING, "Incoming params: %s", QSTR2UTF8( sParam ) );

	//get new network representation
	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();

	////////////////////////////
	//search virtual network by adapter
	CVirtualNetwork* pVirtualNetwork = PrlNet::GetNetworkByParallelsAdapter(
		pNetworkConfig->getVirtualNetworks(), (unsigned int)GET_PRL_ADAPTER_NUMBER(oldNetAdapter.getIndex()));

	if( !pVirtualNetwork )
	{
		WRITE_TRACE(DBG_FATAL, "pVirtualNetwork not found" );
		return PRL_ERR_INVALID_ARG; // FIXME for best error name
	}

	CVirtualNetwork oldVirtualNetwork;
	oldVirtualNetwork.fromString(pVirtualNetwork->toString());

	convertDispAdapterToVirtualNetwork(&oldNetAdapter, pVirtualNetwork);

	PRL_RESULT nRes = cmdUpdateVirtualNetwork(pVirtualNetwork, &oldVirtualNetwork, pNetworkConfig);

	return nRes;
}

PRL_RESULT Task_ManagePrlNetService::addHostOnlyVirtualNetwork(
	CVirtualNetwork *pVirtualNetwork,
	CDspLockedPointer<CParallelsNetworkConfig>& pNetworkConfig)
{
	// index of new Parallels Adapter
	int nAdapterIndex = getPrlAdapterIdx(pVirtualNetwork);

	////////////////////////////
	//search virtual network by adapter
	CVirtualNetworks *pNetworks = pNetworkConfig->getVirtualNetworks();
	if (nAdapterIndex >= 0) {
		CVirtualNetwork *pTmp = PrlNet::GetNetworkByParallelsAdapter(
				pNetworks, GET_PRL_ADAPTER_NUMBER(nAdapterIndex));
		if (pTmp)
		{
			QString qsAdapterName = pVirtualNetwork->getHostOnlyNetwork()
				->getParallelsAdapter()->getName();
			WRITE_TRACE(DBG_FATAL, "Unable to bind the network to"
				" Parallels Adapter %d because Virtual Network"
				" %s already uses this adapter.",
				nAdapterIndex, QSTR2UTF8(pTmp->getNetworkID()));
			return PRL_NET_ADAPTER_ALREADY_USED;
		}
	}

	CHostOnlyNetwork *pHostOnlyNet = pVirtualNetwork->getHostOnlyNetwork();
	bool bShared = (pHostOnlyNet->getNATServer()) ?
		pHostOnlyNet->getNATServer()->isEnabled() : false;
	if (bShared && !PrlNet::isSharedEnabled())
	{
		WRITE_TRACE(DBG_FATAL, "Failed to create the Virtual Network %s"
				" because it is a shared network.",
				QSTR2UTF8(pVirtualNetwork->getNetworkID()));
		return PRL_NET_VIRTUAL_NETWORK_SHARED_PROHIBITED;
	}

	if (nAdapterIndex == PAI_GENERATE_INDEX)
	{
		nAdapterIndex = generatePrlAdapterIdx(pNetworkConfig.getPtr(), bShared);
		if (nAdapterIndex != PAI_INVALID_ADAPTER)
		{
			pHostOnlyNet->getParallelsAdapter()->setPrlAdapterIndex((unsigned int)nAdapterIndex);
			PrlNet::FillDefaultVirtualNetworkParams(pVirtualNetwork,
				(unsigned int)nAdapterIndex, PrlNet::isSharedEnabled());
		}
		else if (bShared)
			return PRL_NET_VIRTUAL_NETWORK_SHARED_EXISTS;
		else
		{
			WRITE_TRACE(DBG_FATAL, "[AddVirtualNetwork] Failed to generate parallels adapter index.");
			return PRL_NET_INSTALL_FAILED;
		}
	}

	pNetworks->m_lstVirtualNetwork.append(pVirtualNetwork);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdAddVirtualNetwork()
{
	QString sVirtNet;
	PRL_RESULT e = getFirstStrParam(getRequestPackage(), sVirtNet);
	if (PRL_FAILED(e))
		return e;

	CVirtualNetwork k;
	k.fromString(sVirtNet);
	if (getRequestFlags() & PRL_USE_VNET_NAME_FOR_BRIDGE_NAME)
		e = Network::Dao(Libvirt::Kit).attachExisting(k, k.getNetworkID());
	else
		e = Network::Dao(Libvirt::Kit).create(k);

	if (PRL_FAILED(e))
	{
		getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
				k.getNetworkID(),
				EVT_PARAM_MESSAGE_PARAM_0));
		return e;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdUpdateVirtualNetwork()
{
	QString sVirtNet;
	PRL_RESULT e = getFirstStrParam(getRequestPackage(), sVirtNet);
	if (PRL_FAILED(e))
		return e;

	CVirtualNetwork VirtualNetwork;
	VirtualNetwork.fromString(sVirtNet);

	if (PRL_FAILED(VirtualNetwork.m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Virtual network parsing error!");
		return PRL_ERR_INVALID_ARG;
	}
	if (PRL_FAILED(e = Network::Dao(Libvirt::Kit).update(VirtualNetwork)))
	{
		getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
				VirtualNetwork.getNetworkID(),
				EVT_PARAM_MESSAGE_PARAM_0));
		return e;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdDeleteVirtualNetwork()
{
	QString sVirtNet;
	PRL_RESULT e = getFirstStrParam(getRequestPackage(), sVirtNet);
	if (PRL_FAILED(e))
		return e;

	CVirtualNetwork VirtualNetwork;
	VirtualNetwork.fromString(sVirtNet);

	if (PRL_FAILED(VirtualNetwork.m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Virtual network parsing error!");
		return PRL_ERR_INVALID_ARG;
	}
	if (PRL_FAILED(e = Network::Dao(Libvirt::Kit).remove(VirtualNetwork)))
	{
		getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
				VirtualNetwork.getNetworkID(),
				EVT_PARAM_MESSAGE_PARAM_0));
		return e;
	}
	return PRL_ERR_SUCCESS;
}

void Task_ManagePrlNetService::convertDispAdapterToVirtualNetwork(CDispNetAdapter* pOldNetAdapter,
																  CVirtualNetwork* pVirtualNetwork )
{
	PRL_ASSERT(pOldNetAdapter);
	PRL_ASSERT(pVirtualNetwork);

	CHostOnlyNetwork* pHostOnlyNet = pVirtualNetwork->getHostOnlyNetwork();
	CParallelsAdapter* pParallelsAdapter = pHostOnlyNet->getParallelsAdapter();

	QString adapterName = pOldNetAdapter->getName();
	pParallelsAdapter->setName(adapterName);
	//https://bugzilla.sw.ru/show_bug.cgi?id=438037
	pParallelsAdapter->setHiddenAdapter( pOldNetAdapter->isHiddenAdapter() );

	// IPv4
	CDispDhcpPreferences* pDhcp = pOldNetAdapter->getDhcpPreferences();
	if( !pDhcp )
		pHostOnlyNet->setDHCPServer( NULL );
	else
	{
		CDHCPServer* pCurrDHCP = pHostOnlyNet->getDHCPServer();
		if( !pCurrDHCP )
		{
			pCurrDHCP = new CDHCPServer();
			pHostOnlyNet->setDHCPServer( pCurrDHCP );
		}
		pCurrDHCP->setEnabled( pDhcp->isEnabled() );
		pCurrDHCP->setIPScopeStart( pDhcp->getDhcpScopeStartIp() );
		pCurrDHCP->setIPScopeEnd( pDhcp->getDhcpScopeEndIp() );

		pHostOnlyNet->setIPNetMask( pDhcp->getDhcpScopeMask() );

		quint32 scopeStart = pDhcp->getDhcpScopeStartIp().toIPv4Address();
		if ((scopeStart&0xff) == 0)
			scopeStart++;

		quint32 dhcpIP = scopeStart++;
		quint32 hostIP = scopeStart++;
		pHostOnlyNet->setDhcpIPAddress(QHostAddress(dhcpIP));
		pHostOnlyNet->setHostIPAddress(QHostAddress(hostIP));
	}//pDhcp

	// IPv6
	CDispDhcpPreferences* pDhcpV6 = pOldNetAdapter->getDhcpV6Preferences();
	if( !pDhcpV6 )
		pHostOnlyNet->setDHCPv6Server( NULL );
	else
	{
		CDHCPServer* pCurrDHCPv6 = pHostOnlyNet->getDHCPv6Server();
		if( !pCurrDHCPv6 )
		{
			pCurrDHCPv6 = new CDHCPServer();
			pHostOnlyNet->setDHCPv6Server( pCurrDHCPv6 );
		}
		pCurrDHCPv6->setEnabled( pDhcpV6->isEnabled() );
		pCurrDHCPv6->setIPScopeStart( pDhcpV6->getDhcpScopeStartIp() );
		pCurrDHCPv6->setIPScopeEnd( pDhcpV6->getDhcpScopeEndIp() );

		pHostOnlyNet->setIP6NetMask( pDhcpV6->getDhcpScopeMask() );

		Q_IPV6ADDR scopeStart = pCurrDHCPv6->getIPScopeStart().toIPv6Address();
		Q_IPV6ADDR dhcpIP = scopeStart;
		IPV6_ADD_VALUE_AT_POS(dhcpIP, 0, 1 );

		Q_IPV6ADDR hostIP = dhcpIP;
		IPV6_ADD_VALUE_AT_POS(hostIP, 0, 1 );

		pHostOnlyNet->setDhcpIP6Address( QHostAddress(dhcpIP) );
		pHostOnlyNet->setHostIP6Address( QHostAddress(hostIP) );

	}//pDhcpV6


	//////////////////////////
	// Fill PORT FORWARDING
	CDispPortForwarding* pPortForwarding = pOldNetAdapter->getPortForwarding();
	if( !pPortForwarding || pPortForwarding->m_lstPorts.empty() )
	{
		if( pHostOnlyNet->getNATServer() )
			pHostOnlyNet->getNATServer()->setPortForwarding( NULL );
	}
	else
	{
		if( !pHostOnlyNet->getNATServer() )
			pHostOnlyNet->setNATServer( new CNATServer );

		pHostOnlyNet->getNATServer()->setEnabled(pOldNetAdapter->getNetworkType() == PNA_SHARED);

		if (!pHostOnlyNet->getNATServer()->isEnabled())
		{
			WRITE_TRACE(DBG_FATAL, "WARNING! Configuring port forwarding for network %s with disabled NAT-server",
				QSTR2UTF8(pVirtualNetwork->getNetworkID()));
		}

		// cleanup all old values
		pHostOnlyNet->getNATServer()->setPortForwarding( new CPortForwarding );
		CPortForwarding*
			pNewPortForwarding = pHostOnlyNet->getNATServer()->getPortForwarding();
		pNewPortForwarding->setTCP( new CTCPForwardsList );
		pNewPortForwarding->setUDP( new CUDPForwardsList );

		for( int i=0; i< pPortForwarding->m_lstPorts.size(); i++ )
		{
			CDispPort* p = pPortForwarding->m_lstPorts[ i ];
			CPortForwardEntry* pEntry = new CPortForwardEntry;

			pEntry->setRuleName( QString( "fw rule %1").arg( i ) );
			pEntry->setIncomingPort( p->getPortNumIn() );
			pEntry->setRedirectIp( p->getIpAddress() );
			pEntry->setRedirectPort( p->getPortNumOut() );

			if( p->getPortType() == 0 )
				pNewPortForwarding->getTCP()->m_lstForwardEntry.append( pEntry );
			else if( p->getPortType() == 1 )
				pNewPortForwarding->getUDP()->m_lstForwardEntry.append( pEntry );
			else
				PRL_ASSERT( "SHIT HAPPENDS" == NULL );
		}//for

	}//else if
}

static bool isNetworkIDinUse(
		SmartPtr<CVmConfiguration> pVmConfig,
		const QString &sNetworkID)
{
	foreach( CVmGenericNetworkAdapter *pNetAdapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (pNetAdapter->getVirtualNetworkID() == sNetworkID)
			return true;
	}
	return false;
}

#if 0
static bool isVirtualNetworkChanged(
		const CVirtualNetwork *pNewVirtualNetwork,
		const CVirtualNetwork *pOldVirtualNetwork)
{
	bool bOldShared = (pOldVirtualNetwork->getHostOnlyNetwork()->getNATServer()) ?
		pOldVirtualNetwork->getHostOnlyNetwork()->getNATServer()->isEnabled() : false;
	bool bShared = (pNewVirtualNetwork->getHostOnlyNetwork()->getNATServer()) ?
		pNewVirtualNetwork->getHostOnlyNetwork()->getNATServer()->isEnabled() : false;

	if (pNewVirtualNetwork->getNetworkID() != pOldVirtualNetwork->getNetworkID())
		return true;
	if (pNewVirtualNetwork->getNetworkType() != pOldVirtualNetwork->getNetworkType())
		return true;
	if (bOldShared != bShared)
		return true;
	if (pNewVirtualNetwork->getNetworkType() == PVN_BRIDGED_ETHERNET)
	{
		if (pNewVirtualNetwork->getBoundCardMac() != pOldVirtualNetwork->getBoundCardMac())
			return true;
		if (pNewVirtualNetwork->getVLANTag() != pOldVirtualNetwork->getVLANTag())
			return true;
	}
	return false;
}
#endif

PRL_RESULT Task_ManagePrlNetService::updateVmNetwork(
		const QString sVmUuid,
		const QString svmDirUuid,
		const QString &sNewVirtualNetworkID,
		const QString &sOldVirtualNetworkID)
{
	SmartPtr< CDspVm > pVm = CDspVm::GetVmInstanceByUuid( sVmUuid, svmDirUuid );
	if (!pVm)
		return 0;

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspCmdNetworkPrefsChanged(
			CDspService::instance()->getNetworkConfig()->toString(),
			sOldVirtualNetworkID,
			sNewVirtualNetworkID);

	SmartPtr<IOPackage> pPkg = DispatcherPackage::createInstance( PVE::DspEvtNetworkPrefChanged, pCmd );
	// set sender to packet!
	pPkg->makeForwardRequest( getRequestPackage() );
	pVm->sendPackageToVm( pPkg );

	return 0;
}

PRL_RESULT Task_ManagePrlNetService::updateVmNetworkSettings(
			const QString &sNewVirtualNetworkID,
			const QString &sOldVirtualNetworkID)
{
	QMultiHash< QString, SmartPtr<CVmConfiguration> > vmHash =
		CDspService::instance()->getVmDirHelper().getAllVmList();
	foreach ( QString vmDirUuid, vmHash.uniqueKeys() )
	{
		foreach( SmartPtr<CVmConfiguration> pVmConfig, vmHash.values( vmDirUuid ) )
		{
			if ( !pVmConfig )
				continue;

			QString sVmUuid = pVmConfig->getVmIdentification()->getVmUuid();
			if (sOldVirtualNetworkID.isEmpty())
			{
				// Annonce new Network configuration on running Vm
				updateVmNetwork(sVmUuid, vmDirUuid, sNewVirtualNetworkID, sOldVirtualNetworkID);
				continue;
			}
			else if (isNetworkIDinUse(pVmConfig, sOldVirtualNetworkID))
			{
				CVmEvent _evt;
				_evt.addEventParameter( new CVmEventParameter(
							PVE::String,
							sNewVirtualNetworkID,
							EVT_PARAM_VMCFG_NEW_VNETWORK_ID) );
				_evt.addEventParameter( new CVmEventParameter(
							PVE::String,
							sOldVirtualNetworkID,
							EVT_PARAM_VMCFG_OLD_VNETWORK_ID) );
				// Update Vm configuration file
				if (CDspService::instance()->getVmDirHelper().atomicEditVmConfigByVm( vmDirUuid, sVmUuid,
							_evt, getClient() ))
				{
					// send event to GUI for changing the config params
					CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, sVmUuid, PIE_DISPATCHER );
					SmartPtr<IOPackage> pkgNew = DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage() );
					CDspService::instance()->getClientManager().sendPackageToVmClients( pkgNew, vmDirUuid, sVmUuid );

					// Update Network configuration on running Vm
					updateVmNetwork(sVmUuid, vmDirUuid, sNewVirtualNetworkID, sOldVirtualNetworkID);
					continue;
				}
			}
			// Update Network preferences
			// FIXME: network preferences have to be passed on AplyConfig action
			updateVmNetwork(sVmUuid, vmDirUuid, QString(), QString());
		}
	}
	return 0;
}

PRL_RESULT Task_ManagePrlNetService::cmdUpdateVirtualNetwork(
			CVirtualNetwork* pNewVirtualNetwork,
			CVirtualNetwork* pOldVirtualNetwork,
			CDspLockedPointer<CParallelsNetworkConfig>& pNetworkConfig)
{
	Q_UNUSED(pNetworkConfig);
	PRL_ASSERT(pNewVirtualNetwork);
	PRL_ASSERT(pOldVirtualNetwork);

#if 0

	if (!pNewVirtualNetwork->isEnabled())
	{
		WRITE_TRACE(DBG_FATAL, "Warning: Configure called for disabled Virtual Network %s!"
			, QSTR2UTF8(pNewVirtualNetwork->getNetworkID()));
	}

	int nOldAdapterIndex = getPrlAdapterIdx(pOldVirtualNetwork);
	int nNewAdapterIndex = getPrlAdapterIdx(pNewVirtualNetwork);
	QString sNewNetworkID = pNewVirtualNetwork->getNetworkID();
	bool bOldShared = (pOldVirtualNetwork->getHostOnlyNetwork()->getNATServer()) ?
		pOldVirtualNetwork->getHostOnlyNetwork()->getNATServer()->isEnabled() : false;
	bool bShared = (pNewVirtualNetwork->getHostOnlyNetwork()->getNATServer()) ?
		pNewVirtualNetwork->getHostOnlyNetwork()->getNATServer()->isEnabled() : false;

	if (bShared && !PrlNet::isSharedEnabled())
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update the Virtual Network %s"
				" because it is a shared network.",
				QSTR2UTF8(pNewVirtualNetwork->getNetworkID()));
		return PRL_NET_VIRTUAL_NETWORK_SHARED_PROHIBITED;
	}

	// Set AdapterIndex on network type change
	if (bOldShared != bShared || (bShared && nNewAdapterIndex != 0)) {
		nNewAdapterIndex = generatePrlAdapterIdx(pNetworkConfig.getPtr(), bShared);
		if (nNewAdapterIndex != PAI_INVALID_ADAPTER)
		{
			pNewVirtualNetwork->getHostOnlyNetwork()
				->getParallelsAdapter()->setPrlAdapterIndex((unsigned int)nNewAdapterIndex);
			PrlNet::FillDefaultVirtualNetworkParams(pNewVirtualNetwork,
					(unsigned int)nNewAdapterIndex, PrlNet::isSharedEnabled());
		}
		else
			return PRL_NET_VIRTUAL_NETWORK_SHARED_EXISTS;
	}

	if (nOldAdapterIndex != nNewAdapterIndex && nNewAdapterIndex != PAI_INVALID_ADAPTER)
	{
// Add adapter
// NOTE: Can index be changed for the Parallels Adapter?
//       Should we delete and add new Parallels Adapter in this case?
//       Can index be changed in general? May be it is an SDK error?

		if (nNewAdapterIndex == PAI_GENERATE_INDEX)
		{
			nNewAdapterIndex = generatePrlAdapterIdx(pNetworkConfig.getPtr(), bShared);
			if (nNewAdapterIndex != PAI_INVALID_ADAPTER)
			{
				pNewVirtualNetwork->getHostOnlyNetwork()
					->getParallelsAdapter()->setPrlAdapterIndex((unsigned int)nNewAdapterIndex);
				PrlNet::FillDefaultVirtualNetworkParams(pNewVirtualNetwork,
					(unsigned int)nNewAdapterIndex, PrlNet::isSharedEnabled());
			}
			else if (bShared)
				return PRL_NET_VIRTUAL_NETWORK_SHARED_EXISTS;
			else
			{
				WRITE_TRACE(DBG_FATAL, "[AddVirtualNetwork] Failed to generate parallels adapter index.");
				return PRL_NET_INSTALL_FAILED;
			}
		}

		pNewVirtualNetwork = PrlNet::GetVirtualNetworkByID(pNetworkConfig->getVirtualNetworks(), sNewNetworkID);
		if ( ! pNewVirtualNetwork )
		{
			WRITE_TRACE(DBG_FATAL, "Update conflict: the network ID '%s' does not exist !",
									QSTR2UTF8(sNewNetworkID));
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
					sNewNetworkID,
					EVT_PARAM_MESSAGE_PARAM_0));
			return PRL_NET_VIRTUAL_NETWORK_ID_NOT_EXISTS;
		}
	}

#ifdef _LIN_
	if (CVzHelper::is_vz_running())
	{
		PRL_RESULT prlResult =
			getVzVNetHelper().UpdateVirtualNetwork(pNewVirtualNetwork,
					pOldVirtualNetwork);
		if (PRL_FAILED(prlResult))
		{
			if (prlResult == PRL_NET_VIRTUAL_NETWORK_ID_NOT_EXISTS)
			{
				QString sOldNetworkID =
					pOldVirtualNetwork->getNetworkID();
				getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
						sOldNetworkID,
						EVT_PARAM_MESSAGE_PARAM_0));
			}
			else if (prlResult == PRL_NET_DUPLICATE_VIRTUAL_NETWORK_ID)
			{
				getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
						sNewNetworkID,
						EVT_PARAM_MESSAGE_PARAM_0));
			}
			return prlResult;
		}
	}
#endif

	// Restart Parallels Adapter if configuration was changed

	// Will be set to true if change of ip address failed.
	// Fail of IP-address change should be remembered and notification passed to user
	// as warning.
	bool bIpAddressChangeFailed = false;

	// Clear bridged iface maping
	if (pNewVirtualNetwork->getNetworkType() == PVN_HOST_ONLY)
		pNewVirtualNetwork->setBoundCardMac();

	// reset index in network configuration on network type change
	if (nOldAdapterIndex >= 0 &&
		pNewVirtualNetwork->getNetworkType() == PVN_BRIDGED_ETHERNET)
		pNewVirtualNetwork->getHostOnlyNetwork()
			->getParallelsAdapter()->setPrlAdapterIndex((unsigned)PAI_GENERATE_INDEX);

	///////////////////
	// save
	PRL_RESULT prlResult = PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", (unsigned)prlResult);
	}
	pNetworkConfig.unlock();  // NOTE:  unlock() should be called on last line!

	// Update Vm configuration
	if (isVirtualNetworkChanged(pNewVirtualNetwork, pOldVirtualNetwork))
		updateVmNetworkSettings(pNewVirtualNetwork->getNetworkID(), pOldVirtualNetwork->getNetworkID());

	// Delete old adapter if required.
	restartPrlNetService();

	if (!bIpAddressChangeFailed)
		return PRL_ERR_SUCCESS;
	else
		return PRL_NET_IPADDRESS_MODIFY_FAILED_WARNING;
#endif

	return PRL_ERR_UNIMPLEMENTED;
}

PRL_RESULT Task_ManagePrlNetService::restartPrlNetService( )
{
	return PrlNet::notifyPrlNetService( ParallelsDirs::getParallelsApplicationDir() );
}


// type: 0 for tcp, 1 for udp
static void FillPortForwards(CDispPortForwarding *dst, QList<CPortForwardEntry* > &src, int forwardType)
{
	foreach(CPortForwardEntry *pEntry, src)
	{
		CDispPort *pPort = new CDispPort;
		pPort->setPortType(forwardType);
		pPort->setPortNumIn(pEntry->getIncomingPort());
		pPort->setPortNumOut(pEntry->getRedirectPort());
		pPort->setIpAddress(pEntry->getRedirectIp());
		dst->m_lstPorts.append(pPort);
	}
}


static CDispPortForwarding *ConvertPortForwarding(const CPortForwarding *src)
{
	if (NULL == src)
		return NULL;

	CDispPortForwarding *dst = new CDispPortForwarding;

	// 0 - tcp, 1 - udp
	CTCPForwardsList* tcpForwards = src->getTCP();
	if (tcpForwards)
	{
		FillPortForwards(dst, tcpForwards->m_lstForwardEntry, 0);
	}

	CUDPForwardsList* udpForwards = src->getUDP();
	if (udpForwards)
	{
		FillPortForwards(dst, udpForwards->m_lstForwardEntry, 1);
	}

	return dst;
}


SmartPtr<CDispNetworkPreferences> Task_ManagePrlNetService::convertNetworkConfig(
	SmartPtr<CParallelsNetworkConfig> pNetworkConfig )
{
	PRL_ASSERT( pNetworkConfig );
	if( !pNetworkConfig )
		return SmartPtr<CDispNetworkPreferences>(0);

	SmartPtr<CDispNetworkPreferences> pOldConfig( new CDispNetworkPreferences );

	QList<CVirtualNetwork *> lstHostOnlyNetworks = PrlNet::MakeHostOnlyNetworksList(pNetworkConfig.getImpl());
	foreach(CVirtualNetwork *pNetwork, lstHostOnlyNetworks)
	{
		if (!pNetwork->isEnabled())
			continue;

		CHostOnlyNetwork *pHostOnlyParams = pNetwork->getHostOnlyNetwork();
		if (NULL == pHostOnlyParams)
			continue;

		CParallelsAdapter *pAdapter = pHostOnlyParams->getParallelsAdapter();
		if (!pAdapter)
			continue;

		CDispNetAdapter *pOldAdapter = new CDispNetAdapter;
		pOldConfig->addNetAdapter(pOldAdapter);

		pOldAdapter->setEnabled( true );
		pOldAdapter->setIndex( GET_PRL_ADAPTER_NUMBER(pAdapter->getPrlAdapterIndex()) );
		pOldAdapter->setName(pAdapter->getName());
		//https://bugzilla.sw.ru/show_bug.cgi?id=438037
		pOldAdapter->setHiddenAdapter( pAdapter->isHiddenAdapter() );

		// to prevent generate random uuid
		// #424340 affects test DispFunctionalityTest::testCommonPrefsSetReadOnlyValues()
		pOldAdapter->setUuid( pNetwork->getUuid() );

		// Determine system name for adapter.

		// Configure DHCP IPv4
		CDispDhcpPreferences *pOldDhcp = new CDispDhcpPreferences;
		pOldAdapter->setDhcpPreferences(pOldDhcp);

		CDHCPServer *pDHCPServer = pHostOnlyParams->getDHCPServer();
		if (NULL == pDHCPServer)
			pOldDhcp->setEnabled(false);
		else
		{
			pOldDhcp->setEnabled(pDHCPServer->isEnabled());
			pOldDhcp->setDhcpScopeStartIp(pDHCPServer->getIPScopeStart());
			pOldDhcp->setDhcpScopeEndIp(pDHCPServer->getIPScopeEnd());
			pOldDhcp->setDhcpScopeMask(pHostOnlyParams->getIPNetMask());
		}

		// Configure DHCP IPv6
		CDispDhcpPreferences *pOldDhcpV6 = new CDispDhcpPreferences;
		pOldAdapter->setDhcpV6Preferences(pOldDhcpV6);

		CDHCPServer *pDhcpV6Server = pHostOnlyParams->getDHCPv6Server();
		if (NULL == pDhcpV6Server)
			pOldDhcpV6->setEnabled(false);
		else
		{
			pOldDhcpV6->setEnabled(pDhcpV6Server->isEnabled());
			pOldDhcpV6->setDhcpScopeStartIp(pDhcpV6Server->getIPScopeStart());
			pOldDhcpV6->setDhcpScopeEndIp(pDhcpV6Server->getIPScopeEnd());
			pOldDhcpV6->setDhcpScopeMask(pHostOnlyParams->getIP6NetMask());
		}

		// configure NAT
		CNATServer *pNATServer = pHostOnlyParams->getNATServer();
		if (NULL == pNATServer)
		{
			pOldAdapter->setNatEnabled(false);
			pOldAdapter->setNetworkType(PNA_HOST_ONLY);
		}
		else
		{
			bool bNATEnabled = pNATServer->isEnabled();
			if (bNATEnabled)
			{
				pOldAdapter->setNatEnabled(true);
				pOldAdapter->setNetworkType(PNA_SHARED);
			}
			else
			{
				pOldAdapter->setNatEnabled(false);
				pOldAdapter->setNetworkType(PNA_HOST_ONLY);
			}

			// Configure port forwarding
			CPortForwarding *pPortForwarding = pNATServer->getPortForwarding();
			CDispPortForwarding *pOldPortForwarding = ConvertPortForwarding(pPortForwarding);
			pOldAdapter->setPortForwarding(pOldPortForwarding);
		}
	}

	return pOldConfig;
}

void Task_ManagePrlNetService::updateAdapter(SmartPtr<CVmConfiguration> pVmConfig,
		CVmGenericNetworkAdapter *pAdapter, bool bEnable)
{
	Q_UNUSED(pVmConfig);
	QString vnic_name = pAdapter->getHostInterfaceName();

#if 0
	if (CVzHelper::is_vz_running() && bEnable) {
		// no sense to detach if !bEnable - nic already destroyed
		getVzVNetHelper().DetachVmdev(vnic_name);
	}
#endif
	if (pAdapter->getEmulatedType() == PNA_ROUTED)
	{
		if (bEnable)
		{
			if (!PrlNet::setIfFlags(vnic_name.toAscii(), IFF_UP, 0))
			{
				WRITE_TRACE(DBG_FATAL, "Error enabling adapter '%s': setIfFlags()",
						QSTR2UTF8(vnic_name));
				return;
			}

			PrlNet::setAdapterIpAddress(vnic_name, DEFAULT_HOSTROUTED_GATEWAY);
			PrlNet::setAdapterIpAddress(vnic_name, DEFAULT_HOSTROUTED_GATEWAY6);
		}

		QStringList ips = pAdapter->getNetAddresses();
		if (ips.size() == 0)
			return;

		// set up routes
		foreach (QString ip_mask, ips)
		{
			QString ip;
			if (!NetworkUtils::ParseIpMask(ip_mask, ip))
				continue;

			WRITE_TRACE(DBG_DEBUG, "Set route on node for IP '%s'", ip.toUtf8().constData());
			bool rc = PrlNet::SetRouteToDevice(ip, vnic_name, bEnable);
			if (!rc)
				WRITE_TRACE(DBG_FATAL, "Failed to set route to %s device %s",
					QSTR2UTF8(ip), QSTR2UTF8(vnic_name));

			WRITE_TRACE(DBG_DEBUG, "Set arp for ip '%s'", ip.toUtf8().constData());
			rc = PrlNet::SetArpToNodeDevices(ip, QString(), bEnable, false);
			if (!rc)
				WRITE_TRACE(DBG_FATAL, "Failed to set arp %s", QSTR2UTF8(ip) );
		}
	}
	else if (pAdapter->getEmulatedType() != PNA_DIRECT_ASSIGN)
	{
		if (!bEnable)
			return;
#if 0
		// connect adapter to bridge
		QString virtualNetworkID = pAdapter->getVirtualNetworkID();
		if (!virtualNetworkID.isEmpty())
		{
			getVzVNetHelper().AddVmdevToVirtualNetwork(virtualNetworkID, vnic_name);
		}
#endif
	}
}

/*
 * Set nessary settings on host for vm networking
*/
void Task_ManagePrlNetService::updateVmNetworking(SmartPtr<CVmConfiguration> pVmConfig, bool bEnable)
{
	Q_UNUSED( pVmConfig );
	Q_UNUSED( bEnable );

#if 0
#ifdef _LIN_
	if (!pVmConfig)
		return;

	if (!CDspService::isServerModePSBM())
		return;

	foreach(CVmGenericNetworkAdapter *adapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (!adapter)
			continue;

		updateAdapter(pVmConfig, adapter, bEnable);
	}
#endif
#endif
}

void Task_ManagePrlNetService::addVmIPAddress(SmartPtr<CVmConfiguration> pVmConfig)
{
	CDspLockedPointer<CParallelsNetworkConfig>
				pNetworkConfig = CDspService::instance()->getNetworkConfig();

	if (!pNetworkConfig || !pVmConfig)
		return;

	if (!pVmConfig->getVmHardwareList())
		return;

	if (pVmConfig->getVmHardwareList()->m_lstNetworkAdapters.isEmpty())
		return;

	bool is_changed = false;

	foreach(CVmGenericNetworkAdapter *adapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (!adapter)
			continue;
		if (adapter->getNetAddresses().empty())
			continue;

		QString mac = adapter->getMacAddress().toLower();

		CVirtualNetwork *pNetworking = PrlNet::GetVirtualNetworkForAdapter(pNetworkConfig.getPtr(),
																		   adapter->getEmulatedType(),
																		   adapter->getVirtualNetworkID());
		if (!pNetworking)
			continue;

		if (!adapter->getVirtualNetworkID().isEmpty())
		{
			if (pNetworking->getNetworkType() != PVN_HOST_ONLY)
				continue;
		}else{
			if (adapter->getEmulatedType() != PNA_HOST_ONLY && adapter->getEmulatedType() != PNA_SHARED)
				continue;
		}

		CIPReservations *pIPReservations = PrlNet::GetSafeDHCPIPReservations(pNetworking);
		if (!pIPReservations)
			continue;

		/* FIXME: We are set only first IPv4 address. (DHCP limitation) */
		QString ip;
		QHostAddress haddr;
		bool is_found = false;
		foreach (QString ip_mask, adapter->getNetAddresses())
		{
			if (!NetworkUtils::ParseIpMask(ip_mask, ip))
			{
				WRITE_TRACE(DBG_INFO, "%s, Warning: IP/MASK '%s' is wrong",
						__FUNCTION__, QSTR2UTF8(ip_mask));
				continue;
			}
			haddr = QHostAddress(ip);
			if (haddr.protocol() == QAbstractSocket::IPv4Protocol)
			{
				is_found = true;
				break;
			}
		}

		// no IPv4 addresses to add to IP reservations
		if (!is_found)
			continue;

		if (PrlNet::ContainsIPReservation(pIPReservations, haddr, mac))
		{
			WRITE_TRACE(DBG_INFO, "%s, Warning: %s or %s is in static lease of virtual network",
					__FUNCTION__, QSTR2UTF8(ip), QSTR2UTF8(mac));
			continue;
		}

		CIPReservation* pIPReservation = new CIPReservation();
		pIPReservation->setIPAddress(haddr);
		pIPReservation->setMacAddress(mac);
		//TODO: set hostname. how to filter string to normal?
		WRITE_TRACE(DBG_INFO, "%s add to virtual network ip %s mac %s",
				__FUNCTION__, QSTR2UTF8(ip), QSTR2UTF8(mac));
		pIPReservations->m_lstIPReservation.append(pIPReservation);
		is_changed = true;
	}

	if (is_changed)
		PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());

	pNetworkConfig.unlock();
	if (is_changed)
		PrlNet::notifyPrlNetService( ParallelsDirs::getParallelsApplicationDir() );

	return;
}

void Task_ManagePrlNetService::removeVmIPAddress(SmartPtr<CVmConfiguration> pVmConfig)

{
	CDspLockedPointer<CParallelsNetworkConfig>
				pNetworkConfig = CDspService::instance()->getNetworkConfig();

	if (!pNetworkConfig || !pVmConfig)
		return;
	if (!pVmConfig->getVmHardwareList())
		return;
	if (pVmConfig->getVmHardwareList()->m_lstNetworkAdapters.isEmpty())
		return;

	bool is_changed = false;

	foreach(CVmGenericNetworkAdapter *adapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (!adapter)
			continue;

		if (adapter->getNetAddresses().empty())
			continue;

		CVirtualNetwork *pNetworking = PrlNet::GetVirtualNetworkForAdapter(pNetworkConfig.getPtr(),
																		   adapter->getEmulatedType(),
																		   adapter->getVirtualNetworkID());
		if (!pNetworking)
			continue;

		if (!adapter->getVirtualNetworkID().isEmpty())
		{
			if (pNetworking->getNetworkType() != PVN_HOST_ONLY)
				continue;
		}else{
			if (adapter->getEmulatedType() != PNA_HOST_ONLY && adapter->getEmulatedType() != PNA_SHARED)
				continue;
		}

		CIPReservations *pIPReservations = PrlNet::GetDHCPIPReservations(pNetworking);
		if (!pIPReservations)
			continue;

		for (int i = 0; i < pIPReservations->m_lstIPReservation.size(); i++)
		{
			CIPReservation *pLease = pIPReservations->m_lstIPReservation[i];
			if (!pLease)
				continue;

			if (PrlNet::isEqualEthAddress(pLease->getMacAddress(), adapter->getMacAddress()))
			{
				//remove lease from global network config
				pIPReservations->m_lstIPReservation.removeAt(i);
				i--;
				is_changed = true;
			}
		}
	}

	if (is_changed)
		PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());

	pNetworkConfig.unlock();
	if (is_changed)
		PrlNet::notifyPrlNetService( ParallelsDirs::getParallelsApplicationDir() );

	return;
}



QSet<QHostAddress> Task_ManagePrlNetService::checkIPAddressDuplicates(SmartPtr<CVmConfiguration> pVmConfig,
                                        const QMultiHash< QString, SmartPtr<CVmConfiguration> > &vmTotalHash)
{
	QSet<QHostAddress> duplicates;
	QSet<QHostAddress> ips;

	if (!pVmConfig)
		return duplicates;

	extractIPAddressesFromVMConfiguration(pVmConfig, ips);

	if (ips.isEmpty())
		return duplicates;

	foreach( QString dirUuid, vmTotalHash.keys() )
	{
		QList< SmartPtr<CVmConfiguration> >
			vmList = vmTotalHash.values( dirUuid );
		for( int idx=0; idx<vmList.size(); idx++)
		{
			SmartPtr<CVmConfiguration> pConfig( vmList[idx] );
			if (!pConfig)
				continue;

			if(pConfig->getVmIdentification()->getVmUuid()  ==
						pVmConfig->getVmIdentification()->getVmUuid())
				continue; //Same VM

			QSet<QHostAddress> used;
			extractIPAddressesFromVMConfiguration(pConfig, used);

			QSet<QHostAddress>  preDuplicates = used.intersect(ips);
			if( preDuplicates.isEmpty() )
				continue;

			duplicates += preDuplicates;
		}//for idx
	} //foreach( QString dirUuid

	return duplicates;
}

void Task_ManagePrlNetService::extractIPAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config,
									QSet<QHostAddress>& IPs)
{
	if (!config)
		return;
	if (!config->getVmHardwareList())
		return;

	QListIterator<CVmGenericNetworkAdapter*> itN(config->getVmHardwareList()->m_lstNetworkAdapters);
	while (itN.hasNext())
	{
		CVmGenericNetworkAdapter* adapter(itN.next());
		if (!adapter)
			continue;
		foreach (QString ip_mask, adapter->getNetAddresses())
		{
			QString ip;
			if (!NetworkUtils::ParseIpMask(ip_mask, ip))
				continue;
			IPs.insert(QHostAddress(ip));
		}
	}
}

QSet<QString> Task_ManagePrlNetService::extractIPAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config,
									PRL_NET_VIRTUAL_NETWORK_TYPE type)
{
	QSet<QString> lstNetAddresses;

	if (!config)
		return (lstNetAddresses);
	if (!config->getVmHardwareList())
		return (lstNetAddresses);

	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	QListIterator<CVmGenericNetworkAdapter*> itN(config->getVmHardwareList()->m_lstNetworkAdapters);
	while (itN.hasNext())
	{
		CVmGenericNetworkAdapter* adapter(itN.next());
		if (!adapter)
			continue;
		if (adapter->getEmulatedType() == PNA_ROUTED ||
			adapter->getEmulatedType() == PNA_DIRECT_ASSIGN)
			continue;
		PRL_NET_VIRTUAL_NETWORK_TYPE nNetType;
		PRL_RESULT nResult = PrlNet::GetNetworkTypeForAdapter(pNetworkConfig.getPtr(), adapter, &nNetType);
		if (PRL_FAILED(nResult))
			continue;
		if (type == nNetType)
		{
			foreach (QString ip, adapter->getNetAddresses())
			{
				lstNetAddresses.insert(ip);
			}
		}
	}

	return (lstNetAddresses);
}


QSet<QString> Task_ManagePrlNetService::extractHostOnlyIPAddressesFromVMConfiguration(
									SmartPtr<CVmConfiguration> config )
{
	return extractIPAddressesFromVMConfiguration(config, PVN_HOST_ONLY);
}

QSet<QString> Task_ManagePrlNetService::extractRoutedIPAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config)
{
	QSet<QString> lstNetAddresses;

	if (!config)
		return (lstNetAddresses);
	if (!config->getVmHardwareList())
		return (lstNetAddresses);

	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	QListIterator<CVmGenericNetworkAdapter*> itN(config->getVmHardwareList()->m_lstNetworkAdapters);
	while (itN.hasNext())
	{
		CVmGenericNetworkAdapter* adapter(itN.next());
		if (!adapter)
			continue;
		if (adapter->getEmulatedType() != PNA_ROUTED)
			continue;
		foreach (QString ip, adapter->getNetAddresses())
		{
			lstNetAddresses.insert(ip);
		}
	}

	return (lstNetAddresses);
}

PRL_RESULT Task_ManagePrlNetService::cmdUpdateNetworkClassesConfig()
{
#ifndef _CT_
	return PRL_ERR_UNIMPLEMENTED;
#else
	if ( !CDspService::instance()->isServerMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Skip network classes setup in non server mode");
		return PRL_ERR_UNIMPLEMENTED;
	}

	QString sXml;
	PRL_RESULT prlResult = getFirstStrParam(getRequestPackage(), sXml);
	if (PRL_FAILED(prlResult))
		return prlResult;

	CNetworkClassesConfig conf;
	conf.fromString(sXml);

	if (PRL_FAILED(conf.m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Network class parsing error!");
		return PRL_ERR_INVALID_ARG;
	}

	if (CVzHelper::update_network_classes_config(conf))
		return PRL_ERR_FAILURE;

	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	pNetworkConfig->setNetworkClassesConfig(new CNetworkClassesConfig(conf));
	prlResult = PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", (unsigned)prlResult);
	}

	return PRL_ERR_SUCCESS;
#endif
}

QMutex *Task_ManagePrlNetService::g_pRestartShapingMtx = new QMutex(QMutex::Recursive);
bool Task_ManagePrlNetService::g_restartShaping = false;

PRL_RESULT Task_ManagePrlNetService::cmdRestartNetworkShaping()
{
#ifndef _CT_
	return PRL_ERR_UNIMPLEMENTED;
#else
	if ( !CDspService::instance()->isServerMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Skip network shaping restart in non server mode");
		return PRL_ERR_UNIMPLEMENTED;
	}

	{
		CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
		if (!pNetworkConfig->getNetworkShapingConfig()->isEnabled())
			return PRL_ERR_SUCCESS;

		QMutexLocker _lock(g_pRestartShapingMtx);
		if (g_restartShaping)
		{
			WRITE_TRACE(DBG_FATAL, "Shaping restart in progress");
			return PRL_ERR_SUCCESS;
		}

		g_restartShaping = true;
	}

	WRITE_TRACE(DBG_FATAL, "Restart network shaping");
	QList< SmartPtr<CDspVm> > lstVms = CDspService::instance()->getVmManager().getAllRunningVms();
	foreach(SmartPtr<CDspVm> pVm, lstVms)
	{
		PRL_RESULT nRetCode;
		SmartPtr<CVmConfiguration> pVmCfg = CDspService::instance()->getVmDirHelper().
						getVmConfigByUuid(pVm->getVmDirUuid(), pVm->getVmUuid(), nRetCode);
		if (!pVmCfg || !PRL_SUCCEEDED(nRetCode))
			continue;

		Task_NetworkShapingManagement::setNetworkRate(pVmCfg);
	}
	QMutexLocker _lock(g_pRestartShapingMtx);
	g_restartShaping = false;
	return PRL_ERR_SUCCESS;
#endif
}

PRL_RESULT Task_ManagePrlNetService::cmdUpdateNetworkShapingConfig()
{
#ifndef _CT_
	return PRL_ERR_UNIMPLEMENTED;
#else
	if ( !CDspService::instance()->isServerMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Skip network classes setup in non server mode");
		return PRL_ERR_UNIMPLEMENTED;
	}

	QString sXml;
	PRL_RESULT prlResult = getFirstStrParam(getRequestPackage(), sXml);
	if (PRL_FAILED(prlResult))
		return prlResult;

	CNetworkShapingConfig conf;
	conf.fromString(sXml);

	if (PRL_FAILED(conf.m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Network class parsing error!");
		return PRL_ERR_INVALID_ARG;
	}
	// FIXME: validate shaping configuration on on network classes basis.
	if (CVzHelper::update_network_shaping_config(conf))
		return PRL_ERR_FAILURE;

#ifdef _LIN_
	// VZWIN shaper does not require restart
	cmdRestartNetworkShaping();
#endif

	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	pNetworkConfig->setNetworkShapingConfig(new CNetworkShapingConfig(conf));
	prlResult = PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", (unsigned)prlResult);
	}

	return PRL_ERR_SUCCESS;
#endif
}


// Validate IP private network net addresses.
PRL_RESULT Task_ManagePrlNetService::validateIPPrivateNetwork(CPrivateNetwork *pNetwork)
{
	QList<QString> IPs = pNetwork->getNetAddresses();
	foreach (QString ip_mask, IPs)
	{
		QString ip, mask;
		bool bError = false;

		// special value: grant access to all IPs not covered by
		// any private network.
		if (ip_mask == QString("*"))
			continue;

		if (!NetworkUtils::ParseIpMask(ip_mask, ip, mask))
			bError = true;
		else
		{
			// specification of IP address without prefix is prohibited for
			// IPv6 due to performance reasons
			QHostAddress ip_addr(ip);
			if (ip_addr.protocol() == QAbstractSocket::IPv6Protocol &&
				(!ip_mask.contains("/") || mask == QString(128)))
				bError = true;
		}

		if (bError)
		{
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, ip_mask,
						EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, pNetwork->getName(),
						EVT_PARAM_MESSAGE_PARAM_1));
			return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdAddIPPrivateNetwork()
{
	if ( !CDspService::instance()->isServerMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Skip private network setup in non server mode");
		return PRL_ERR_UNIMPLEMENTED;
	}

	QString sPrivNet;
	PRL_RESULT prlResult = getFirstStrParam(getRequestPackage(), sPrivNet);
	if (PRL_FAILED(prlResult))
		return prlResult;

	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	if (pNetworkConfig.getPtr() == NULL)
		return PRL_ERR_UNEXPECTED;

	CPrivateNetwork* pPrivateNetwork = new CPrivateNetwork();
	pPrivateNetwork->fromString( sPrivNet );

	CPrivateNetwork *pOldNet = NULL;

	// search for duplicates
	QString sName = pPrivateNetwork->getName();
	CPrivateNetworks *pNetworks = pNetworkConfig->getPrivateNetworks();
	foreach(CPrivateNetwork *pNetwork, pNetworks->m_lstPrivateNetwork)
	{
		if (pNetwork->getName() == sName)
		{
			WRITE_TRACE(DBG_FATAL, "Duplicated new network name '%s'!",
					QSTR2UTF8(sName));
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String, sName,
						EVT_PARAM_MESSAGE_PARAM_0));
			delete pPrivateNetwork;
			return PRL_NET_DUPLICATE_IPPRIVATE_NETWORK_NAME;
		}
		if (pPrivateNetwork->getNetworkID() == pNetwork->getNetworkID())
			pOldNet = pNetwork;
	}

	prlResult = validateIPPrivateNetwork(pPrivateNetwork);
	if (PRL_FAILED(prlResult))
	{
		delete pPrivateNetwork;
		return prlResult;
	}

#ifdef _CT_
	prlResult = CVzPrivateNetwork::AddPrivateNetwork(pPrivateNetwork,
			pNetworks, getLastError());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to add IP private network '%s',"
			" error = %x", QSTR2UTF8(pPrivateNetwork->getName()), prlResult);

		// Restore old settings
		CVzPrivateNetwork::RemovePrivateNetwork(pPrivateNetwork);
		if (pOldNet != NULL) {
			if (PRL_FAILED(CVzPrivateNetwork::AddPrivateNetwork(pOldNet, NULL, NULL)))
				WRITE_TRACE(DBG_FATAL, "Failed to restore original private network");
		}

		delete pPrivateNetwork;
		return prlResult;
	}
#endif

	pNetworks->m_lstPrivateNetwork.append(pPrivateNetwork);
	prlResult = PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", (unsigned)prlResult);
		return prlResult;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdUpdateIPPrivateNetwork()
{
	if ( !CDspService::instance()->isServerMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Skip private network setup in non server mode");
		return PRL_ERR_UNIMPLEMENTED;
	}

	QString sPrivNet;
	PRL_RESULT prlResult = getFirstStrParam(getRequestPackage(), sPrivNet);
	if (PRL_FAILED(prlResult))
		return prlResult;

	CPrivateNetwork PrivateNetwork;
	PrivateNetwork.fromString(sPrivNet);

	if (PRL_FAILED(PrivateNetwork.m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Private network parsing error!");
		return PRL_ERR_INVALID_ARG;
	}

	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	if (pNetworkConfig.getPtr() == NULL)
		return PRL_ERR_UNEXPECTED;

	// Check on existing virtual network and duplicate the new network id
	QString sName = PrivateNetwork.getName();
	unsigned int id = PrivateNetwork.getNetworkID();
	CPrivateNetwork *pOldNet = NULL;
	CPrivateNetworks *pNetworks = pNetworkConfig->getPrivateNetworks();
	foreach(CPrivateNetwork *pNetwork, pNetworks->m_lstPrivateNetwork)
	{
		if ((id != PRL_PRIVNET_GENERATE_ID && pNetwork->getNetworkID() == id) ||
			(id == PRL_PRIVNET_GENERATE_ID && pNetwork->getName() == sName))
		{
			pOldNet = pNetwork;
			break;
		}
	}
	if (!pOldNet)
	{
		WRITE_TRACE(DBG_FATAL, "No IP private network for update!");
		return PRL_NET_IPPRIVATE_NETWORK_DOES_NOT_EXIST;
	}

	// Check that new name is not used already
	foreach(CPrivateNetwork *pNetwork, pNetworks->m_lstPrivateNetwork)
	{
		if (pNetwork != pOldNet && pNetwork->getName() == sName)
		{
			WRITE_TRACE(DBG_FATAL, "Duplicated new network name '%s'!",
					QSTR2UTF8(sName));
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, sName,
						EVT_PARAM_MESSAGE_PARAM_0));
			return PRL_NET_DUPLICATE_IPPRIVATE_NETWORK_NAME;
		}
	}

	prlResult = validateIPPrivateNetwork(&PrivateNetwork);
	if (PRL_FAILED(prlResult))
		return prlResult;

#ifdef _CT_
	prlResult = CVzPrivateNetwork::UpdatePrivateNetwork(&PrivateNetwork, pOldNet,
			getLastError());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update IP private network '%s',"
			" error = %x", QSTR2UTF8(PrivateNetwork.getName()), prlResult);
		// Restore old settings
		CVzPrivateNetwork::RemovePrivateNetwork(&PrivateNetwork);
		if (pOldNet != NULL) {
			if (PRL_FAILED(CVzPrivateNetwork::AddPrivateNetwork(pOldNet, NULL, NULL)))
				WRITE_TRACE(DBG_FATAL, "Failed to restore original private network");
		}

		return prlResult;
	}
#endif

	*pOldNet = PrivateNetwork;
	prlResult = PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", (unsigned)prlResult);
		return prlResult;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ManagePrlNetService::cmdRemoveIPPrivateNetwork()
{
	if ( !CDspService::instance()->isServerMode() )
	{
		WRITE_TRACE(DBG_FATAL, "Skip private network setup in non server mode");
		return PRL_ERR_UNIMPLEMENTED;
	}

	QString sPrivNet;
	PRL_RESULT prlResult = getFirstStrParam(getRequestPackage(), sPrivNet);
	if (PRL_FAILED(prlResult))
		return prlResult;

	CPrivateNetwork PrivateNetwork;
	PrivateNetwork.fromString(sPrivNet);
	if (PRL_FAILED(PrivateNetwork.m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Private network parsing error!");
		return PRL_ERR_INVALID_ARG;
	}

	CDspLockedPointer<CParallelsNetworkConfig> pNetworkConfig =
					CDspService::instance()->getNetworkConfig();
	if (!pNetworkConfig.getPtr())
		return PRL_ERR_UNEXPECTED;

	QString sName = PrivateNetwork.getName();
	CPrivateNetworks* pNetworks = pNetworkConfig->getPrivateNetworks();
	CPrivateNetwork* pPrivateNetwork = NULL;
	foreach(CPrivateNetwork *pNetwork, pNetworks->m_lstPrivateNetwork)
	{
		if (pNetwork->getName() == sName)
		{
			pPrivateNetwork = pNetwork;
			break;
		}
	}

	if (!pPrivateNetwork)
	{
		WRITE_TRACE(DBG_FATAL, "The IP private network '%s' does not exist !", QSTR2UTF8(sName));
		getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
				sName,
				EVT_PARAM_MESSAGE_PARAM_0));
		return PRL_NET_IPPRIVATE_NETWORK_DOES_NOT_EXIST;
	}

#ifdef _CT_
	prlResult = CVzPrivateNetwork::RemovePrivateNetwork(&PrivateNetwork, getLastError());
	if (PRL_FAILED(prlResult))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to remove IP private network '%s',"
			" error = %x", QSTR2UTF8(PrivateNetwork.getName()), prlResult);
		return prlResult;
	}
#endif

	pNetworks->m_lstPrivateNetwork.removeAll(pPrivateNetwork);
	delete pPrivateNetwork;

	prlResult = PrlNet::WriteNetworkConfig(*pNetworkConfig.getPtr());
	if (PRL_FAILED(prlResult))
		WRITE_TRACE(DBG_FATAL, "Failed to update Parallels Network Config: %x", prlResult);

	return PRL_ERR_SUCCESS;
}

bool Task_ManagePrlNetService::getHostMacAddresses(QSet< QString >& hostMacs)
{
	PrlNet::EthAdaptersList ethList;
	PRL_RESULT err = PrlNet::makeBindableAdapterList(ethList, false);
	if (PRL_FAILED(err))
	{
		WRITE_TRACE(DBG_FATAL, "makeBindableAdapterList failed with 0x%x", err);
		return false;
	}

	foreach (const PrlNet::EthernetAdapter &eth, ethList)
	{
		QString ethMac = PrlNet::ethAddressToString(eth._macAddr);
		hostMacs.insert(ethMac);
	}

	return true;
}

