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

#ifndef __IFACE_ENUM_H__
#define __IFACE_ENUM_H__
#include "enum.h"

namespace Libvirt
{
namespace Iface
{
namespace Xml
{
enum EMode
{
	EModeOnboot,
	EModeNone,
	EModeHotplug
};

enum EState
{
	EStateUnknown,
	EStateNotpresent,
	EStateDown,
	EStateLowerlayerdown,
	EStateTesting,
	EStateDormant,
	EStateUp
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

enum EMode1
{
	EMode1BalanceRr,
	EMode1ActiveBackup,
	EMode1BalanceXor,
	EMode1Broadcast,
	EMode18023ad,
	EMode1BalanceTlb,
	EMode1BalanceAlb
};

enum ECarrier
{
	ECarrierIoctl,
	ECarrierNetif
};

enum EValidate
{
	EValidateNone,
	EValidateActive,
	EValidateBackup,
	EValidateAll
};


} // namespace Xml
} // namespace Iface
} // namespace Libvirt

#endif // __IFACE_ENUM_H__
