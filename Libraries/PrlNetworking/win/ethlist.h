///////////////////////////////////////////////////////////////////////////////
///
/// @file ethlist.h
///
/// Ethernet interfaces enumerating functions
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
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
	QString			_name;			///< User-friendly name of the interface
	QString			_adapterGuid;	///< Guid of the adapter.

	int	_nAdapter;		///< Index of the adapter
						///< Note: Prl adapters starts from index PRL_ADAPTER_START_INDEX
	unsigned char	_macAddr[6];	///< Macaddr of the interface

	unsigned short  _vlanTag; ///< VLAN tag; PRL_INVALID_VLAN_TAG if not a vlan.

	EthIface( const QString &name,
			  const QString &adapterGuid,
			  int nAdapter,
			  unsigned short vlanTag,
			  const unsigned char macAddr[6] );
};

// PrlAdapter is always greater then eth adapter.
// Or compare using index.
inline bool operator < (const EthIface& a, const EthIface& b)
{
	return a._nAdapter<b._nAdapter;
}


typedef std::list<EthIface> EthIfaceList;

/// creates a list of available ethernet interfaces with UP state and valid parameters
/// @param unusedParam unused parameter for interface compatibility with Unix implementation
bool makeEthIfacesList( EthIfaceList &ethList, bool unusedParam, bool unusedParam2 = false );

#endif //nat_ethlist_h__
