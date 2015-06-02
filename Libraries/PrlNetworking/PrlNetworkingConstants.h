///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlNetworkingConstants.h
/// @author sdmitry
///
/// Common for Unix and Windows constants
///
/// Copyright (c) 2007-2015 Parallels IP Holdings GmbH
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

#ifndef PrlNetworkingConstants_h__
#define PrlNetworkingConstants_h__

// every PRL adapter has this bit set
#define PRL_ADAPTER_START_INDEX 0x10000000
// VME-adapters of VMs has this bit set
#define PRL_VME_ADAPTER_START_INDEX 0x20000000
// mask to apply to adapter index to obtain adapter number
#define PRL_ADAPTER_INDEX_MASK	0x0fffffff
// true if adapter is a Parallels adapter
#define IS_PRL_ADAPTER_INDEX(idx) ( 0 != ((idx)&PRL_ADAPTER_START_INDEX) )
// true if adapter is a VME adapter
#define IS_VME_ADAPTER_INDEX(idx) ( 0 != ((idx)&PRL_VME_ADAPTER_START_INDEX) )
// return number of Parallels adapter using index
#define GET_PRL_ADAPTER_NUMBER(idx) ((idx)&PRL_ADAPTER_INDEX_MASK)

enum PRL_NET_MODE
{
	PRL_NET_MODE_VNIC = 0,
	PRL_NET_MODE_VME = 1,
};

#define DEFAULT_HOSTROUTED_GATEWAY	"169.255.30.1/16"
#define DEFAULT_HOSTROUTED_GATEWAY6	"fe80::ffff:1:1/64"

#endif //PrlNetworkingConstants_h__
