/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef PrlNet_PrlNetInternal_h__
#define PrlNet_PrlNetInternal_h__

#include "PrlNetLibrary.h"

#if defined(_WIN_)
#	include "win/ethlist.h"
#else
#	include "unix/ethlist.h"
#endif

#include "PrlNetworkingConstants.h"

namespace PrlNetInternal
{

/// Obtains the default bridged interface.
/// Fills itAdapter with iterator from ethList to default adapter.
/// @param ethList [in] List of up ethernet interfaces
/// @param itAdapter [out] Iterator to the default adapter in list
/// @return 0 on success, PRL_ERR_FAILURE if failed to detect
PRL_RESULT getDefaultBridgedAdapter(const EthIfaceList &ethList,
	EthIfaceList::const_iterator &itAdapter);


#if defined(_WIN_)
/// Obtains parameters of Prl adapter as it is configured in the
/// host operating system. This is required for the adapter
/// for which DHCP is disabled but NAT is enabled
/// @param prlAdapterIndex [in] index of the Prl adapter
/// @param ipaddr [out] ip address of the adapter
/// @param netmask [out] netmask of the adapter
PRL_RESULT  getPrlSystemAdapterParams(
	int prlAdapterIndex,
	unsigned int &ipaddr,
	unsigned int &netmask );
#endif

} // namespace PrlNet


#endif //PrlNet_VMDefaultAdapter_h__
