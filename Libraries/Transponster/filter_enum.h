/*
 * Copyright (c) 2020 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __FILTER_ENUM_H__
#define __FILTER_ENUM_H__
#include "enum.h"

namespace Libvirt
{
namespace Filter
{
namespace Xml
{
enum EChain
{
	EChainRoot
};

enum EActionType
{
	EActionTypeDrop,
	EActionTypeAccept,
	EActionTypeReject,
	EActionTypeContinue,
	EActionTypeReturn
};

enum EDirectionType
{
	EDirectionTypeIn,
	EDirectionTypeOut,
	EDirectionTypeInout
};

enum EVirYesNo
{
	EVirYesNoYes,
	EVirYesNoNo
};

enum EChoice5647
{
	EChoice5647Arp,
	EChoice5647Rarp,
	EChoice5647Ipv4,
	EChoice5647Ipv6,
	EChoice5647Vlan
};

enum EBoolean
{
	EBooleanYes,
	EBooleanNo,
	EBooleanTrue,
	EBooleanFalse,
	EBoolean1,
	EBoolean0
};

enum EChoice5674
{
	EChoice5674Tcp,
	EChoice5674Udp,
	EChoice5674Udplite,
	EChoice5674Esp,
	EChoice5674Ah,
	EChoice5674Icmp,
	EChoice5674Igmp,
	EChoice5674Sctp,
	EChoice5674Icmpv6
};


} // namespace Xml
} // namespace Filter
} // namespace Libvirt

#endif // __FILTER_ENUM_H__
