///////////////////////////////////////////////////////////////////////////////
///
/// @file ethlist.h
///
/// Ethernet interfaces enumerating functions for Unix systems declarations
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
#ifndef nat_ethlist_h__
#define nat_ethlist_h__

#include <QString>
#include <list>

#include "../PrlNetworkingConstants.h"

struct EthIface
{
	QString			_name;			///< Name of the interface

	int				_ifaceFlags;	///< flags of the interface

	unsigned int	_bcastIpAddr;	///< Broadcast IP Address of the interface
	unsigned char	_macAddr[6];	///< Macaddr of the interface
	unsigned short	_vlanTag;       ///< VLAN Tag (0xffff if not a vlan)

	int	_nAdapter;		///< Index of the adapter
						///< Note: Prl adapters starts from index PRL_ADAPTER_START_INDEX
	int	_nType;			///< Network adapter type (PRL_NET_ADAPTER_TYPE)

	EthIface();
	EthIface(const QString &name, int ifaceFlags,
			 unsigned int bcastIpAddr, const unsigned char macAddr[6],
			 unsigned short vlanTag, int nAdapter, int nType);
	EthIface( const EthIface& eth );
};


// PrlAdapter is always greater then eth adapter.
// Or compare using index.
inline bool operator < (const EthIface& a, const EthIface& b)
{
	return a._nAdapter<b._nAdapter;
}


typedef std::list<EthIface> EthIfaceList;

/// creates a list of available ethernet interfaces with valid parameters
/// @param ethList - ouput list of the interfaces
/// @param bUpOnly - list adapters with UP state only
/// @param bConfigured - list only configured adapters (with ip addrsses)
bool makeEthIfacesList( EthIfaceList &ethList, bool bUpOnly, bool bConfigured = false );

/// creates a list of bindable ethernet interfaces with valid parameters
/// @param ethList - ouput list of the interfaces
/// @param bUpOnly - list adapters with UP state only
/// @param bConfigured - list only configured adapters (with ip addrsses)
bool makeBindableEthIfacesList(EthIfaceList &ethList, bool bUpOnly, bool bConfigured = false);

#endif //nat_ethlist_h__
