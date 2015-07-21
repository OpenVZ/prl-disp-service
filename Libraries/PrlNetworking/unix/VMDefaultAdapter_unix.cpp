/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "../PrlNetInternal.h"
#include "ethlist.h"

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <sys/ioctl.h>

#if defined(_LIN_)
#include <net/if.h>
#include <net/route.h>
#include <linux/types.h>
#include <list>
#include <Libraries/PrlCommonUtilsBase/netutils.h>

#elif defined(_MAC_)
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <mach/mach.h>
#include <mach/mach_port.h>

#include <IOKit/IOKitLib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SCPreferences.h>
#include <SystemConfiguration/SCDynamicStoreKey.h>
#include <SystemConfiguration/SCSchemaDefinitions.h>
#endif


#include <cassert>
#include <memory>
#include <boost/bind.hpp>
#include <Libraries/Logging/Logging.h>
#include <Interfaces/ParallelsQt.h>

#ifdef _DEBUG
#include <iostream>
#endif

#if defined(_LIN_)

static EthIfaceList::const_iterator match(const EthIfaceList& eths_, const QString& name_)
{
	if (!QDir(QString("/sys/class/net/%1/bridge").arg(name_)).exists())
	{
		return std::find_if(eths_.begin(), eths_.end(),
			boost::bind(&EthIface::_name, _1) == boost::cref(name_));
	}
	QFile f(QString("/sys/class/net/%1/address").arg(name_));
	if (!f.open(QFile::ReadOnly | QFile::Text))
		return eths_.end();

	QString x = QTextStream(&f).readAll().trimmed().toUpper();
	return std::find_if(eths_.begin(), eths_.end(),
		boost::bind(&PrlNet::ethAddressToString,
			boost::bind(&EthIface::_macAddr, _1)) == boost::cref(x));
}

// returns iterator within IfEntryLinList to default adapter
// @param sock [in] ControlSocket
// @param ethList [in] List with interfaces
static EthIfaceList::const_iterator
getDefaultRouteAdapter(const EthIfaceList &ethList)
{
	char buf[1024];
	EthIfaceList::const_iterator itDefaultIface = ethList.end();

	// todo: search also IPv6 adapters
	FILE *f = fopen("/proc/net/route", "r");
	if (NULL == f) {
		WRITE_TRACE(DBG_WARNING, "failed to open /proc/net/route");
		return itDefaultIface;
	}

	// skip first line;
	if (NULL == fgets(buf, 1023, f)) {
		if (feof(f))
			WRITE_TRACE(DBG_WARNING, "empty /proc/net/route");
		else if (ferror(f))
			WRITE_TRACE(DBG_WARNING, "failed to read /proc/net/route");

		fclose(f);
		return itDefaultIface;
	}

	while (fgets(buf, 1023, f)) {
		char iface[17];
		UINT32 dest, gw, mask;
		UINT32 iflags, metric, refcnt, use;
		iflags = 0;
		// iface dest gw flags refcnt use metric mask XXX
		(void)sscanf(buf, "%16s %08x %08x %x %x %x %x %08x",
					iface, &dest, &gw, &iflags, &refcnt, &use,
					&metric, &mask);
		if ((iflags & (RTF_UP | RTF_GATEWAY)) != (RTF_UP | RTF_GATEWAY))
			continue;
		if (mask != 0 || gw == 0)
			continue;

		iface[16] = '\0';
		// note: it is possible to search using lowest metric but this will
		// require iteration through all records in file.
		itDefaultIface = match(ethList, iface);
		if (ethList.end() != itDefaultIface)
			goto found;

		WRITE_TRACE(DBG_FATAL, "Default-GW Interface %s (ip %08x) is not bindable",
			iface, htonl(gw));
	}
found:
	fclose(f);
	return itDefaultIface;
}

// Obtains default bridged interface
PRL_RESULT
PrlNetInternal::getDefaultBridgedAdapter(
	const EthIfaceList &ethList, EthIfaceList::const_iterator &itAdapter)
{
	itAdapter = ethList.end();

	itAdapter = getDefaultRouteAdapter(ethList);
	if (itAdapter != ethList.end())
		return PRL_ERR_SUCCESS;

	return PRL_ERR_FAILURE;
};

#elif defined(_MAC_)

static EthIfaceList::const_iterator
getPrimaryIface(const EthIfaceList &ethList)
{
	SCDynamicStoreRef store = SCDynamicStoreCreate(0, CFSTR("Parallels"), 0, 0);
	if (NULL == store) {
		WRITE_TRACE(DBG_FATAL, "getPrimaryIface: SCDynamicStoreCreate() Failed");
		return ethList.end();
	}

	CFStringRef key = SCDynamicStoreKeyCreateNetworkGlobalEntity(
		0, kSCDynamicStoreDomainState, kSCEntNetIPv4);
	CFDictionaryRef dict =
		(CFDictionaryRef)SCDynamicStoreCopyValue(store,  key);
	CFRelease(key);
	CFRelease(store);

	if (!dict) {
		WRITE_TRACE(DBG_FATAL, "getPrimaryIface: "
					"SCDynamicStoreKeyCreateNetworkGlobalEntity() Failed");
		return ethList.end();
	}

	EthIfaceList::const_iterator itDefaultAdapter = ethList.end();

	const void *v = CFDictionaryGetValue(dict, kSCDynamicStorePropNetPrimaryInterface);
	do {
		if (!v || CFGetTypeID(v) != CFStringGetTypeID()) {
			WRITE_TRACE(DBG_FATAL, "getPrimaryIface: "
						"No key for NetPrimaryInterface in DynamicStore");
			break;
		}

		char buf[256];
		CFStringRef primary = (CFStringRef)v;
		const char *pif = CFStringGetCStringPtr(primary, CFStringGetFastestEncoding(primary));
		if (NULL == pif) {
			if (CFStringGetCString(primary, buf, sizeof(buf), kCFStringEncodingMacRoman))
				pif = buf;
		}

		if (NULL == pif) {
			WRITE_TRACE(DBG_FATAL, "getPrimaryIface: "
						"Failed to convert primary-iface to utf8");
			break;
		}

		for (EthIfaceList::const_iterator it = ethList.begin(); it != ethList.end(); ++it ) {
			if (it->_name == pif) {
				itDefaultAdapter = it;
				break;
			}
		}

		if (itDefaultAdapter == ethList.end())
			WRITE_TRACE(DBG_FATAL, "getPrimaryAdapter: "
						"primary iface (%s) is not an ethernet-iface", pif);
	} while (0);

	CFRelease(dict);
	return itDefaultAdapter;
}


// Obtains default bridged interface
PRL_RESULT
PrlNetInternal::getDefaultBridgedAdapter(
	const EthIfaceList &ethList, EthIfaceList::const_iterator &itAdapter)
{
	itAdapter = getPrimaryIface(ethList);
	if (itAdapter != ethList.end())
		return PRL_ERR_SUCCESS;
	return PRL_ERR_FAILURE;
}
#else
#error "Trouble with operating system macros!"
#endif
