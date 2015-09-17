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

#include "network_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Network::Xml::EVirYesNo>::data_type Enum<Network::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Network::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Network::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Network::Xml::EVirOnOff>::data_type Enum<Network::Xml::EVirOnOff>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Network::Xml::EVirOnOffOn, "on"))
			(data_type::value_type(Network::Xml::EVirOnOffOff, "off"));
}

template<>
Enum<Network::Xml::EMode>::data_type Enum<Network::Xml::EMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Network::Xml::EModeNat, "nat"))
			(data_type::value_type(Network::Xml::EModeRoute, "route"))
			(data_type::value_type(Network::Xml::EModeBridge, "bridge"))
			(data_type::value_type(Network::Xml::EModePassthrough, "passthrough"))
			(data_type::value_type(Network::Xml::EModePrivate, "private"))
			(data_type::value_type(Network::Xml::EModeVepa, "vepa"))
			(data_type::value_type(Network::Xml::EModeHostdev, "hostdev"));
}

template<>
Enum<Network::Xml::EName>::data_type Enum<Network::Xml::EName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Network::Xml::ENameKvm, "kvm"))
			(data_type::value_type(Network::Xml::ENameVfio, "vfio"));
}

template<>
Enum<Network::Xml::ENativeMode>::data_type Enum<Network::Xml::ENativeMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Network::Xml::ENativeModeTagged, "tagged"))
			(data_type::value_type(Network::Xml::ENativeModeUntagged, "untagged"));
}

template<>
Enum<Network::Xml::EState>::data_type Enum<Network::Xml::EState>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Network::Xml::EStateUp, "up"))
			(data_type::value_type(Network::Xml::EStateDown, "down"));
}

} // namespace Libvirt
