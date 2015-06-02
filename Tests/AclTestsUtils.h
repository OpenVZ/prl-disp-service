///////////////////////////////////////////////////////////////////////////////
///
/// @file AclTestsUtils.h
///
/// This header file contains necessary defines and utils for using ACLs support at
/// tests
///
/// @author sandro
/// @owner sergeym
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef AclTestsUtils_H
#define AclTestsUtils_H

#include "Libraries/CAuth/CAclHelper.h"
#include "Interfaces/ParallelsQt.h"
using namespace Parallels;

#define ALLOW true
#define DENY false

#define WRITE		1<<0
#define READ		1<<1
#define EXECUTE	1<<2

namespace {
/**
 * Tests helper: adds specified ACL to the file
 * @param path to the file where ACL should be added
 * @param ACL owner name
 * @param sign whether ACL should be allowed or deny
 * @param ACL permissions mask
 */
bool AddAcl(const QString &sFilePath, const QString &sOwnerName, bool bIsAllowed, quint32 nPermissions)
{
#ifdef _MAC_
#define PROCESS_PERMISSION(permission, permission_name)\
	if (nPermissions & permission)\
	{\
		QString sAclCommand = sAclCommandTemplate\
														.arg(sOwnerName)\
														.arg(bIsAllowed ? "allow" : "deny")\
														.arg(permission_name)\
														.arg(sFilePath);\
		int nRes = system(QSTR2UTF8(sAclCommand));\
		if (nRes != 0)\
		{\
			WRITE_TRACE(DBG_FATAL, "Failed to add %s permission. Return code: %d. Applied command: '%s'", permission_name, nRes, QSTR2UTF8(sAclCommand));\
			return (false);\
		}\
	}

	QString sAclCommandTemplate("chmod +a \"%1 %2 %3\" %4");
	PROCESS_PERMISSION(WRITE, "write")
	PROCESS_PERMISSION(READ, "read")
	PROCESS_PERMISSION(EXECUTE, "execute")
	return (true);
#else
	Q_UNUSED(sFilePath);
	Q_UNUSED(sOwnerName);
	Q_UNUSED(bIsAllowed);
	Q_UNUSED(nPermissions);

	return (false);
#endif
}

}//namespace

#endif//AclTestsUtils_H
