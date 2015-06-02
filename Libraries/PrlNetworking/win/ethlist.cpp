///////////////////////////////////////////////////////////////////////////////
///
/// @file ethlist.cpp
///
/// Ethernet interfaces enumerating functions for Unix systems implementations
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

#include <windows.h>

#include "ethlist.h"

#include "../prlnet.h"

#include <ctype.h>

#include <QtAlgorithms>
#include <QString>
#include <prlsdk/PrlEnums.h>
#include <Libraries/Logging/Logging.h>
#include <memory>

EthIface::EthIface( const QString &name,
				    const QString &adapterGuid,
					int nAdapter,
					unsigned short vlanTag,
					const unsigned char macAddr[6] )
	: _name(name)
	, _adapterGuid(adapterGuid)
	, _nAdapter(nAdapter)
	, _vlanTag(vlanTag)
{
	memcpy( _macAddr, macAddr, 6 );
}


//
// converts unicode wStr to the QString
// @return number of characters processed inside wStr or -1 if error
//
static int toQString( QString &result, LPCWSTR wStr, int maxSize )
{
	int iStringLen = 0;
	result.clear();

	while( maxSize )
	{
		if( wStr[iStringLen] == 0 )
			break;
		++iStringLen;
		maxSize--;
	}

	result.setUtf16(wStr, iStringLen);

	return iStringLen;
}


static int GetVNicNumber(const QString &adapterName)
{
	// To be correct, this function must obtain NetworkComponent interface, open using this interface
	// component params registry key and read index using this param.
	// This is too complex operation for now, but while I will have time, I will do so.
	// For now - just return index from the name of the adapter..
	int len = adapterName.length();
	if( len < 1 )
		return -1;

	int pos = adapterName.indexOf("Parallels Virtual NIC");
	if( pos < 0 )
	{
		return -1;
	}

	pos += sizeof("Parallels Virtual NIC")-1;
	unsigned char s = '#';
	while( adapterName.at(pos) != 0 )
	{
		if( isdigit( adapterName.at(pos).toAscii() ) )
		{
			s = adapterName.at(pos).toAscii();
			break;
		}
		pos++;
	}

	if( s <'0' || s > '9' )
		return 255; // this is Parallels adapter, but wrong-named

	return s - '0';
}


bool makeEthIfacesList( EthIfaceList &ethList, bool unusedParam, bool unusedParam2 )
{
	(void)unusedParam;
	(void)unusedParam2;

	std::auto_ptr<IPrlNet> pvsnet(IPrlNet::create_prlnet(IPrlNet::PVSNET));
	if (!pvsnet.get())
		return false;

	DWORD err = pvsnet->open_prlnet();
	if (0 != err) {
		WRITE_TRACE(DBG_FATAL,
			"makeEthIfacesList: error %u opening driver! Parallels driver is probably not installed!",
			err);
		return false;
	}

	ethList.clear();
	UINT32 listSize = 1024, listUsed;
	LPWSTR list = (LPWSTR)_alloca(listSize);

	// obtain list of currently bridged adapters
	for (;;)
	{
		DWORD err = pvsnet->enum_devices(list, listSize, &listUsed);

		if( 0 == err )
			break;

		if (err != ERROR_INSUFFICIENT_BUFFER)
			return false;

		list = (LPWSTR)_alloca(listSize *= 2);
	}

	int nAdapterCounter = 0; // counter for the ethernet adapters indexes

	//
	// parse adapter list
	// list terminates with an empty unicode string
	//
	for( LPWSTR ptr = list; *ptr && 0 != listUsed; )
	{
		// first part - adapter GUID, second - adapter name, next 6 bytes - MAC address
		QString adapterGuid;
		int len = toQString(adapterGuid, ptr, listUsed/2 );
		if( len < 0 )
			return false;

		ptr += len + 1;
		listUsed -= 2*(len - 1);

		QString adapterName;
		len = toQString(adapterName, ptr, listUsed/2 );
		if( len < 0 )
			return false;

		ptr += len + 1;
		listUsed -= 2*(len - 1);

		if( listUsed < 6 )
			return false;

		const unsigned char *macAddr = (unsigned char *)ptr;
		ptr += 3;
		listUsed -= 6;

#if 0
		if( std::string::npos != adapterName.find("SWSoft")
			|| std::string::npos != adapterName.find("VMware") )
		{
			// ignore SWSoft and VMWare adapters
			continue;
		}
#endif

		int nAdapterNum = 0;
		if( -1 != adapterName.indexOf("Parallels") )
		{
			nAdapterNum = GetVNicNumber(adapterName);
			if( nAdapterNum < 0 || nAdapterNum > 32 )
			{
				continue; // Skip 3.0 adapters
			}
			nAdapterNum |= PRL_ADAPTER_START_INDEX;
		}
		else
		{
			nAdapterNum = nAdapterCounter++;
		}

		ethList.push_back(
				EthIface(
					adapterName, adapterGuid,
					nAdapterNum,
					PRL_INVALID_VLAN_TAG, // not implemented
					macAddr
				) );
	}

	ethList.sort(); // sort adapters

	return true;
}
