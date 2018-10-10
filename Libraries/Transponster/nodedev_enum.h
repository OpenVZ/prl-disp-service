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

#ifndef __NODEDEV_ENUM_H__
#define __NODEDEV_ENUM_H__
#include "enum.h"

namespace Libvirt
{
namespace Nodedev
{
namespace Xml
{
enum EType
{
	ETypePciBridge,
	ETypeCardbusBridge
};

enum EValidity
{
	EValidityCap,
	EValiditySta
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

enum EMediaAvailable
{
	EMediaAvailable1,
	EMediaAvailable0
};

enum EType1
{
	EType1Primary,
	EType1Control,
	EType1Render
};


} // namespace Xml
} // namespace Nodedev
} // namespace Libvirt

#endif // __NODEDEV_ENUM_H__
