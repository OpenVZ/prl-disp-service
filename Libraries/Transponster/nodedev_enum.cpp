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

#include "nodedev_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Nodedev::Xml::EType>::data_type Enum<Nodedev::Xml::EType>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Nodedev::Xml::ETypePciBridge, "pci-bridge"))
			(data_type::value_type(Nodedev::Xml::ETypeCardbusBridge, "cardbus-bridge"));
}

template<>
Enum<Nodedev::Xml::EValidity>::data_type Enum<Nodedev::Xml::EValidity>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Nodedev::Xml::EValidityCap, "cap"))
			(data_type::value_type(Nodedev::Xml::EValiditySta, "sta"));
}

template<>
Enum<Nodedev::Xml::EState>::data_type Enum<Nodedev::Xml::EState>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Nodedev::Xml::EStateUnknown, "unknown"))
			(data_type::value_type(Nodedev::Xml::EStateNotpresent, "notpresent"))
			(data_type::value_type(Nodedev::Xml::EStateDown, "down"))
			(data_type::value_type(Nodedev::Xml::EStateLowerlayerdown, "lowerlayerdown"))
			(data_type::value_type(Nodedev::Xml::EStateTesting, "testing"))
			(data_type::value_type(Nodedev::Xml::EStateDormant, "dormant"))
			(data_type::value_type(Nodedev::Xml::EStateUp, "up"));
}

template<>
Enum<Nodedev::Xml::EMediaAvailable>::data_type Enum<Nodedev::Xml::EMediaAvailable>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Nodedev::Xml::EMediaAvailable1, "1"))
			(data_type::value_type(Nodedev::Xml::EMediaAvailable0, "0"));
}

template<>
Enum<Nodedev::Xml::EType1>::data_type Enum<Nodedev::Xml::EType1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Nodedev::Xml::EType1Primary, "primary"))
			(data_type::value_type(Nodedev::Xml::EType1Control, "control"))
			(data_type::value_type(Nodedev::Xml::EType1Render, "render"));
}

} // namespace Libvirt
