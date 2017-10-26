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

#include "capability_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Capability::Xml::EVirYesNo>::data_type Enum<Capability::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Capability::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Capability::Xml::EFallback>::data_type Enum<Capability::Xml::EFallback>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EFallbackAllow, "allow"))
			(data_type::value_type(Capability::Xml::EFallbackForbid, "forbid"));
}

template<>
Enum<Capability::Xml::EPolicy>::data_type Enum<Capability::Xml::EPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EPolicyForce, "force"))
			(data_type::value_type(Capability::Xml::EPolicyRequire, "require"))
			(data_type::value_type(Capability::Xml::EPolicyOptional, "optional"))
			(data_type::value_type(Capability::Xml::EPolicyDisable, "disable"))
			(data_type::value_type(Capability::Xml::EPolicyForbid, "forbid"));
}

template<>
Enum<Capability::Xml::EUsable>::data_type Enum<Capability::Xml::EUsable>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EUsableYes, "yes"))
			(data_type::value_type(Capability::Xml::EUsableNo, "no"))
			(data_type::value_type(Capability::Xml::EUsableUnknown, "unknown"));
}

} // namespace Libvirt
