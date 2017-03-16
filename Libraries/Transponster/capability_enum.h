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

#ifndef __CAPABILITY_ENUM_H__
#define __CAPABILITY_ENUM_H__
#include "enum.h"

namespace Libvirt
{
namespace Capability
{
namespace Xml
{
enum EArchnames
{
	EArchnamesAarch64,
	EArchnamesAlpha,
	EArchnamesArmv7l,
	EArchnamesCris,
	EArchnamesI686,
	EArchnamesIa64,
	EArchnamesLm32,
	EArchnamesM68k,
	EArchnamesMicroblaze,
	EArchnamesMicroblazeel,
	EArchnamesMips,
	EArchnamesMipsel,
	EArchnamesMips64,
	EArchnamesMips64el,
	EArchnamesOpenrisc,
	EArchnamesParisc,
	EArchnamesParisc64,
	EArchnamesPpc,
	EArchnamesPpc64,
	EArchnamesPpcemb,
	EArchnamesS390,
	EArchnamesS390x,
	EArchnamesSh4,
	EArchnamesSh4eb,
	EArchnamesSparc,
	EArchnamesSparc64,
	EArchnamesUnicore32,
	EArchnamesX8664,
	EArchnamesXtensa,
	EArchnamesXtensaeb
};

enum EUriTransport
{
	EUriTransportEsx,
	EUriTransportTcp,
	EUriTransportXenmigr
};

enum EOsType
{
	EOsTypeXen,
	EOsTypeLinux,
	EOsTypeHvm,
	EOsTypeExe,
	EOsTypeUml
};

enum EWordsize
{
	EWordsize31,
	EWordsize32,
	EWordsize64
};

enum EType
{
	ETypeQemu,
	ETypeKqemu,
	ETypeKvm,
	ETypeXen,
	ETypeUml,
	ETypeLxc,
	ETypeOpenvz,
	ETypeTest
};

enum EVirYesNo
{
	EVirYesNoYes,
	EVirYesNoNo
};

enum EVirOnOff
{
	EVirOnOffOn,
	EVirOnOffOff
};


} // namespace Xml
} // namespace Capability
} // namespace Libvirt

#endif // __CAPABILITY_ENUM_H__
