///////////////////////////////////////////////////////////////////////////////
///
/// @file CAclHelper_win.cpp
///
/// Windows implementation of ACLs support
///
/// @author sandro
/// @owner sergeym
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CAclHelper.h"

namespace Virtuozzo {

namespace CAclHelper {

bool IsAclSupported(const QString &sFilePath)
{
	Q_UNUSED(sFilePath);
	return (false);
}

CAclSet GetFileAcls(const QString &sFilePath)
{
	Q_UNUSED(sFilePath);
	return (CAclSet());
}

bool ApplyAclsToFile(const QString &sFilePath, const CAclSet &_acls)
{
	Q_UNUSED(sFilePath);
	Q_UNUSED(_acls);
	return (false);
}

}//namespace CAclHelper

}//namespace Virtuozzo
