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

#include "capability_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Capability::Xml::EArchnames>::data_type Enum<Capability::Xml::EArchnames>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EArchnamesAarch64, "aarch64"))
			(data_type::value_type(Capability::Xml::EArchnamesAlpha, "alpha"))
			(data_type::value_type(Capability::Xml::EArchnamesArmv7l, "armv7l"))
			(data_type::value_type(Capability::Xml::EArchnamesCris, "cris"))
			(data_type::value_type(Capability::Xml::EArchnamesI686, "i686"))
			(data_type::value_type(Capability::Xml::EArchnamesIa64, "ia64"))
			(data_type::value_type(Capability::Xml::EArchnamesLm32, "lm32"))
			(data_type::value_type(Capability::Xml::EArchnamesM68k, "m68k"))
			(data_type::value_type(Capability::Xml::EArchnamesMicroblaze, "microblaze"))
			(data_type::value_type(Capability::Xml::EArchnamesMicroblazeel, "microblazeel"))
			(data_type::value_type(Capability::Xml::EArchnamesMips, "mips"))
			(data_type::value_type(Capability::Xml::EArchnamesMipsel, "mipsel"))
			(data_type::value_type(Capability::Xml::EArchnamesMips64, "mips64"))
			(data_type::value_type(Capability::Xml::EArchnamesMips64el, "mips64el"))
			(data_type::value_type(Capability::Xml::EArchnamesOpenrisc, "openrisc"))
			(data_type::value_type(Capability::Xml::EArchnamesParisc, "parisc"))
			(data_type::value_type(Capability::Xml::EArchnamesParisc64, "parisc64"))
			(data_type::value_type(Capability::Xml::EArchnamesPpc, "ppc"))
			(data_type::value_type(Capability::Xml::EArchnamesPpc64, "ppc64"))
			(data_type::value_type(Capability::Xml::EArchnamesPpcemb, "ppcemb"))
			(data_type::value_type(Capability::Xml::EArchnamesS390, "s390"))
			(data_type::value_type(Capability::Xml::EArchnamesS390x, "s390x"))
			(data_type::value_type(Capability::Xml::EArchnamesSh4, "sh4"))
			(data_type::value_type(Capability::Xml::EArchnamesSh4eb, "sh4eb"))
			(data_type::value_type(Capability::Xml::EArchnamesSparc, "sparc"))
			(data_type::value_type(Capability::Xml::EArchnamesSparc64, "sparc64"))
			(data_type::value_type(Capability::Xml::EArchnamesUnicore32, "unicore32"))
			(data_type::value_type(Capability::Xml::EArchnamesX8664, "x86_64"))
			(data_type::value_type(Capability::Xml::EArchnamesXtensa, "xtensa"))
			(data_type::value_type(Capability::Xml::EArchnamesXtensaeb, "xtensaeb"));
}

template<>
Enum<Capability::Xml::EUriTransport>::data_type Enum<Capability::Xml::EUriTransport>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EUriTransportEsx, "esx"))
			(data_type::value_type(Capability::Xml::EUriTransportTcp, "tcp"))
			(data_type::value_type(Capability::Xml::EUriTransportXenmigr, "xenmigr"));
}

template<>
Enum<Capability::Xml::EOsType>::data_type Enum<Capability::Xml::EOsType>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EOsTypeXen, "xen"))
			(data_type::value_type(Capability::Xml::EOsTypeLinux, "linux"))
			(data_type::value_type(Capability::Xml::EOsTypeHvm, "hvm"))
			(data_type::value_type(Capability::Xml::EOsTypeExe, "exe"))
			(data_type::value_type(Capability::Xml::EOsTypeUml, "uml"));
}

template<>
Enum<Capability::Xml::EWordsize>::data_type Enum<Capability::Xml::EWordsize>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EWordsize31, "31"))
			(data_type::value_type(Capability::Xml::EWordsize32, "32"))
			(data_type::value_type(Capability::Xml::EWordsize64, "64"));
}

template<>
Enum<Capability::Xml::EType>::data_type Enum<Capability::Xml::EType>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::ETypeQemu, "qemu"))
			(data_type::value_type(Capability::Xml::ETypeKqemu, "kqemu"))
			(data_type::value_type(Capability::Xml::ETypeKvm, "kvm"))
			(data_type::value_type(Capability::Xml::ETypeXen, "xen"))
			(data_type::value_type(Capability::Xml::ETypeUml, "uml"))
			(data_type::value_type(Capability::Xml::ETypeLxc, "lxc"))
			(data_type::value_type(Capability::Xml::ETypeOpenvz, "openvz"))
			(data_type::value_type(Capability::Xml::ETypeTest, "test"));
}

template<>
Enum<Capability::Xml::EVirYesNo>::data_type Enum<Capability::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Capability::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Capability::Xml::EVirOnOff>::data_type Enum<Capability::Xml::EVirOnOff>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Capability::Xml::EVirOnOffOn, "on"))
			(data_type::value_type(Capability::Xml::EVirOnOffOff, "off"));
}

} // namespace Libvirt
