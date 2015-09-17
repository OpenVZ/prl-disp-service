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

#include "iface_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Iface::Xml::EMode>::data_type Enum<Iface::Xml::EMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::EModeOnboot, "onboot"))
			(data_type::value_type(Iface::Xml::EModeNone, "none"))
			(data_type::value_type(Iface::Xml::EModeHotplug, "hotplug"));
}

template<>
Enum<Iface::Xml::EState>::data_type Enum<Iface::Xml::EState>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::EStateUnknown, "unknown"))
			(data_type::value_type(Iface::Xml::EStateNotpresent, "notpresent"))
			(data_type::value_type(Iface::Xml::EStateDown, "down"))
			(data_type::value_type(Iface::Xml::EStateLowerlayerdown, "lowerlayerdown"))
			(data_type::value_type(Iface::Xml::EStateTesting, "testing"))
			(data_type::value_type(Iface::Xml::EStateDormant, "dormant"))
			(data_type::value_type(Iface::Xml::EStateUp, "up"));
}

template<>
Enum<Iface::Xml::EVirYesNo>::data_type Enum<Iface::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Iface::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Iface::Xml::EVirOnOff>::data_type Enum<Iface::Xml::EVirOnOff>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::EVirOnOffOn, "on"))
			(data_type::value_type(Iface::Xml::EVirOnOffOff, "off"));
}

template<>
Enum<Iface::Xml::EMode1>::data_type Enum<Iface::Xml::EMode1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::EMode1BalanceRr, "balance-rr"))
			(data_type::value_type(Iface::Xml::EMode1ActiveBackup, "active-backup"))
			(data_type::value_type(Iface::Xml::EMode1BalanceXor, "balance-xor"))
			(data_type::value_type(Iface::Xml::EMode1Broadcast, "broadcast"))
			(data_type::value_type(Iface::Xml::EMode18023ad, "802.3ad"))
			(data_type::value_type(Iface::Xml::EMode1BalanceTlb, "balance-tlb"))
			(data_type::value_type(Iface::Xml::EMode1BalanceAlb, "balance-alb"));
}

template<>
Enum<Iface::Xml::ECarrier>::data_type Enum<Iface::Xml::ECarrier>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::ECarrierIoctl, "ioctl"))
			(data_type::value_type(Iface::Xml::ECarrierNetif, "netif"));
}

template<>
Enum<Iface::Xml::EValidate>::data_type Enum<Iface::Xml::EValidate>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Iface::Xml::EValidateNone, "none"))
			(data_type::value_type(Iface::Xml::EValidateActive, "active"))
			(data_type::value_type(Iface::Xml::EValidateBackup, "backup"))
			(data_type::value_type(Iface::Xml::EValidateAll, "all"));
}

} // namespace Libvirt
