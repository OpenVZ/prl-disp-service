/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2021 Virtuozzo International GmbH. All rights reserved.
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

#include "filter_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Filter::Xml::EChain>::data_type Enum<Filter::Xml::EChain>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EChainRoot, "root"));
}

template<>
Enum<Filter::Xml::EActionType>::data_type Enum<Filter::Xml::EActionType>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EActionTypeDrop, "drop"))
			(data_type::value_type(Filter::Xml::EActionTypeAccept, "accept"))
			(data_type::value_type(Filter::Xml::EActionTypeReject, "reject"))
			(data_type::value_type(Filter::Xml::EActionTypeContinue, "continue"))
			(data_type::value_type(Filter::Xml::EActionTypeReturn, "return"));
}

template<>
Enum<Filter::Xml::EDirectionType>::data_type Enum<Filter::Xml::EDirectionType>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EDirectionTypeIn, "in"))
			(data_type::value_type(Filter::Xml::EDirectionTypeOut, "out"))
			(data_type::value_type(Filter::Xml::EDirectionTypeInout, "inout"));
}

template<>
Enum<Filter::Xml::EVirYesNo>::data_type Enum<Filter::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Filter::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Filter::Xml::EChoice5647>::data_type Enum<Filter::Xml::EChoice5647>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EChoice5647Arp, "arp"))
			(data_type::value_type(Filter::Xml::EChoice5647Rarp, "rarp"))
			(data_type::value_type(Filter::Xml::EChoice5647Ipv4, "ipv4"))
			(data_type::value_type(Filter::Xml::EChoice5647Ipv6, "ipv6"))
			(data_type::value_type(Filter::Xml::EChoice5647Vlan, "vlan"));
}

template<>
Enum<Filter::Xml::EBoolean>::data_type Enum<Filter::Xml::EBoolean>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EBooleanYes, "yes"))
			(data_type::value_type(Filter::Xml::EBooleanNo, "no"))
			(data_type::value_type(Filter::Xml::EBooleanTrue, "true"))
			(data_type::value_type(Filter::Xml::EBooleanFalse, "false"))
			(data_type::value_type(Filter::Xml::EBoolean1, "1"))
			(data_type::value_type(Filter::Xml::EBoolean0, "0"));
}

template<>
Enum<Filter::Xml::EChoice5674>::data_type Enum<Filter::Xml::EChoice5674>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Filter::Xml::EChoice5674Tcp, "tcp"))
			(data_type::value_type(Filter::Xml::EChoice5674Udp, "udp"))
			(data_type::value_type(Filter::Xml::EChoice5674Udplite, "udplite"))
			(data_type::value_type(Filter::Xml::EChoice5674Esp, "esp"))
			(data_type::value_type(Filter::Xml::EChoice5674Ah, "ah"))
			(data_type::value_type(Filter::Xml::EChoice5674Icmp, "icmp"))
			(data_type::value_type(Filter::Xml::EChoice5674Igmp, "igmp"))
			(data_type::value_type(Filter::Xml::EChoice5674Sctp, "sctp"))
			(data_type::value_type(Filter::Xml::EChoice5674Icmpv6, "icmpv6"));
}

} // namespace Libvirt
