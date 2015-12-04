///////////////////////////////////////////////////////////////////////////////
///
/// @file VMNetworking.cpp
/// @author sdmitry
///
/// Implements CVMNetworking class
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////
#include <Build/Current.ver>
#include "PrlNetLibrary.h"
#include <Interfaces/ParallelsQt.h>
#include <Interfaces/ParallelsNamespace.h>
#include <Libraries/PrlCommonUtilsBase/ParallelsDirs.h>
#include <Libraries/PrlUuid/Uuid.h>
#include <XmlModel/DispConfig/CDispatcherConfig.h>
//#include <XmlModel/DispConfig/CDispDhcpPreferences.h>
#include "Libraries/Logging/Logging.h"
#include <memory>
#include <cassert>

#include "PrlNetInternal.h"

#if defined(_LIN_) || defined(_MAC_)
#include <syslog.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <net/if.h> // for IFF_UP
#else
#include <windows.h>
#endif

#if defined(_MAC_)
#include "unix/MacConfigureAdapter.h"
#endif

#include "Libraries/PrlCommonUtilsBase/Common.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtilsBase/CommandLine.h"

static void inline MODULE_STORE_SYSTEM_ERROR()
{
#if defined(_WIN_)
	PrlNet::setSysError(::GetLastError());
#else
	PrlNet::setSysError((unsigned int)errno);
#endif
}

static bool IsPrlAdapter(PrlNet::EthernetAdapter &ethAdapter )
{
	if (IS_PRL_ADAPTER_INDEX(ethAdapter._adapterIndex))
		return true;

	if (IS_VME_ADAPTER_INDEX(ethAdapter._adapterIndex))
		return true;

	// deprecated.
	if (ethAdapter._systemName.indexOf("vme") == 0)
		return true;

	return false;
}

/**
 * getAdapter() implementation
 */

// Converts EthIface to PrlNet::EthernetAdapter
static void EthIface2EthAdapter( const EthIface &ethIface, PrlNet::EthernetAdapter &ethAdapter )
{
	ethAdapter._name = ethIface._name;
	ethAdapter._systemName = ethIface._name;
	ethAdapter._adapterIndex = ethIface._nAdapter;

#ifdef _WIN_
	ethAdapter._adapterGuid = ethIface._adapterGuid;
#endif

	ethAdapter._bParallelsAdapter = IsPrlAdapter(ethAdapter);

#if defined(_WIN_)
	ethAdapter._bEnabled = true; // Adapter can't be disabled in Windows
#else
	ethAdapter._bEnabled = !!(ethIface._ifaceFlags&IFF_UP);
#endif

	ethAdapter._vlanTag = ethIface._vlanTag;

	memcpy( ethAdapter._macAddr, ethIface._macAddr, 6 );

#if defined(_MAC_)
	Mac_GetAdapterName( ethIface._name, ethAdapter._name );
#endif
}


PRL_RESULT PrlNet::makeBindableAdapterList( PrlNet::EthAdaptersList &adaptersList,
		bool bUpAdapters, bool bConfigured)
{
	adaptersList.clear();

	EthIfaceList ethList;
	if( !::makeEthIfacesList(ethList, bUpAdapters, bConfigured) )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet]  makeEthIfacesList returned error: %ld",
			PrlNet::getSysError() );
		return PRL_NET_ETHLIST_CREATE_ERROR;
	}

	for(EthIfaceList::iterator it = ethList.begin(); it != ethList.end(); ++it)
	{
		PrlNet::EthernetAdapter ethAdapter;
		EthIface2EthAdapter( *it, ethAdapter);
		adaptersList.append(ethAdapter);
	}

	return PRL_ERR_SUCCESS;
}


// return first non-parallels network adapter
PRL_RESULT PrlNet::getFirstAdapter(PrlNet::EthAdaptersList &adaptersList,
			PrlNet::EthAdaptersList::Iterator &itFirstAdapter)
{
	for(PrlNet::EthAdaptersList::Iterator it = adaptersList.begin();
		it != adaptersList.end();
		++it)
	{
		if (IS_PRL_ADAPTER_INDEX(it->_adapterIndex)
				|| !it->_bEnabled)
			continue;
		itFirstAdapter = it;
		return PRL_ERR_SUCCESS;
	}
	return PRL_NET_ERR_ETH_NO_BINDABLE_ADAPTER;
}


PRL_RESULT PrlNet::getDefaultBridgedAdapter( PrlNet::EthAdaptersList &adaptersList, PrlNet::EthAdaptersList::Iterator &defaultAdapter)
{
	defaultAdapter = adaptersList.end();

	EthIfaceList ethList;
	if( !::makeEthIfacesList(ethList, true) ) // only UP adapters
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet]  makeEthIfacesList returned error: %ld",
			PrlNet::getSysError() );
		return PRL_NET_ETHLIST_CREATE_ERROR;
	}

	EthIfaceList::const_iterator itDefaultAdapter;
	PRL_RESULT prlResult = PrlNetInternal::getDefaultBridgedAdapter( ethList, itDefaultAdapter);
	if (PRL_FAILED(prlResult)) {
		WRITE_TRACE(DBG_FATAL, "getDefaultBridgedAdapter() failed with 0x%x",
			prlResult);
		return prlResult;
	}

	// find out which adapter in adaptersList is a itDefaultAdapter
	for(PrlNet::EthAdaptersList::Iterator it = adaptersList.begin();
			it != adaptersList.end();
			++it )
	{
		if (itDefaultAdapter->_vlanTag == it->_vlanTag
			&& 0 == memcmp(itDefaultAdapter->_macAddr, it->_macAddr, sizeof(it->_macAddr)))
		{
			defaultAdapter = it;
			return PRL_ERR_SUCCESS;
		}
	}

	WRITE_TRACE(DBG_FATAL, "Internal error: getDefaultBridgedAdapter():"
				"Passed list of adapters doesn't contain adaptersList.");
	return PRL_ERR_UNEXPECTED;
}


unsigned char PrlNet::getIPv6PrefixFromMask(const Q_IPV6ADDR *mask)
{
	const unsigned *m = (const unsigned *)mask;
	unsigned char prefix = 0;
	unsigned i, m2;

	/* fast path - calc octets first */
	for (i = 0; i < sizeof(*mask) / sizeof(*m); i++) {
		if (m[i] != 0xffffffff)
			goto calc_rest;
		prefix += 32;
	}

	return prefix;
calc_rest:
	/* slow path - calc the rest */
	m2 = ntohl(m[i]);
	while (m2) {
		m2 = m2 << 1;
		prefix++;
	}

	return prefix;
}

QHostAddress PrlNet::getIPv4MaskFromPrefix(quint32 prefix4_)
{
	return QHostAddress(quint32(-1) << (32 - prefix4_));
}

QHostAddress PrlNet::getIPv6MaskFromPrefix(quint32 prefix6_)
{
	quint8 a[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	int p = 128 - prefix6_;
	for (int i = 0; p > 0; i++, p -= 8)
		a[i] = a[i] << p;

	return QHostAddress(a);
}

#if defined(_LIN_) || defined(_MAC_)
// #133154  fake call to link prl_net_start  with PrlNetworking library without errors for linux / MAC OS X Tiger
// more info: https://bugzilla.sw.ru/show_bug.cgi?id=133154#c13
void PrlNet::FakeInitCall( )
{
	int i = 0;
	Q_UNUSED( i );
}
#endif

PRL_NET_MODE PrlNet::getMode()
{
#ifdef _LIN_
	struct stat st;
	if (0 == stat("/proc/vz/vmth", &st) || errno != ENOENT)
		return PRL_NET_MODE_VME;
#endif
	return PRL_NET_MODE_VNIC;
}

QStringList PrlNet::makePhysicalAdapterList()
{
	QStringList names;
	QList<QNetworkInterface> l =  QNetworkInterface::allInterfaces();
	foreach(const QNetworkInterface& i, l)
	{
		if ((QNetworkInterface::IsUp & i.flags())
				&& QDir(QString("/sys/class/net/%1/device").arg(i.name())).exists())
			names << i.name();
	}
	return names;
}
