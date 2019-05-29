///////////////////////////////////////////////////////////////////////////////
///
/// @file CAclHelperTest.h
///
/// Tests suite for ACL helper class
///
/// @author sandro
/// @owner sergeym
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef CAclHelperTest_H
#define CAclHelperTest_H

#include <QtTest/QtTest>
#include "Libraries/CAuth/CAclHelper.h"

class CAclHelperTest : public QObject
{

Q_OBJECT

private slots:
	void init();
	void cleanup();

	void testGetFileAclsForUser();
	void testGetFileAclsForUserThatDoNotHasAcls();
	void testGetFileAclsForBothUsers();
	void testGetFileAclsForBothUsersAndForGroup();

	void testGetEffectiveRightsForUser();
	void testGetEffectiveRightsForGroup();
	void testGetEffectiveRightsForUser2();

	void testApplyAclsToFile();
	void testApplyAclsToFileWithEmptyAclsList();

	void testAclInfoComparisionOperatorOnEqualAcls();
	void testAclInfoComparisionOperatorOnNonEqualAcls();

	void testGetFileAclsOnFileWithoutAcls();
	void testGetEffectivePermissionsForUserOnFileWithoutAcls();
	void testGetEffectivePermissionsForGroupOnFileWithoutAcls();
	void testIsAclSupportedOnFileWithoutAcls();

	void testApplyAclsToFileOnNonExistenFile();
	void testGetFileAclsOnNonExistenFile();
	void testGetEffectivePermissionsForUserOnNonExistenFile();
	void testGetEffectivePermissionsForGroupOnNonExistenFile();
	void testIsAclSupportedOnNonExistenFile();

	void testGetEffectivePermissionsForUserOnWrongUserName();
	void testGetEffectivePermissionsForGroupOnWrongGroupName();
	void testGetAclsByOwnerNameOnWronOnWrongUserName();
	void testGetAclsByOwnerNameOnWronOnWrongGroupName();

private:
	QString m_sTestFileName, m_sTestFileName2;
};

#endif
