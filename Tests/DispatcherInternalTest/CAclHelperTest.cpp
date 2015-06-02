///////////////////////////////////////////////////////////////////////////////
///
/// @file CAclHelperTest.h
///
/// Tests suite for ACL helper class
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

#include "CAclHelperTest.h"

#include <QFile>
#include <QDir>

#include "Tests/CommonTestsUtils.h"
#include "Tests/AclTestsUtils.h"

#include "Libraries/PrlUuid/Uuid.h"
#include "Interfaces/ParallelsQt.h"

#include "Libraries/PrlCommonUtils/CAuthHelper.h"

void CAclHelperTest::init()
{
	m_sTestFileName = QDir::tempPath() + Uuid::createUuid().toString();
	QVERIFY(QFile(m_sTestFileName).open(QIODevice::WriteOnly));
	QVERIFY(QFile::exists(m_sTestFileName));
#ifdef _MAC_
	//FIXME: workaround due acl_get_file() system call never returns ENOTSUP and we do not
	//have legacy method to understand whether ACLs supported om filesystem or not
	AddAcl(m_sTestFileName, "root", ALLOW, WRITE | READ | EXECUTE);
#endif
	m_sTestFileName2 = QDir::tempPath() + Uuid::createUuid().toString();
	QVERIFY(QFile(m_sTestFileName2).open(QIODevice::WriteOnly));
	QVERIFY(QFile::exists(m_sTestFileName2));
}

void CAclHelperTest::cleanup()
{
	QFile::remove(m_sTestFileName);
	QFile::remove(m_sTestFileName2);
}

namespace {
quint32 PlatformIndependGetUserId(const QString &sUserName, const QString &sUserPassword)
{
#ifndef _WIN_
	CAuthHelper _auth_helper(sUserName);
	if(_auth_helper.AuthUser(sUserPassword))
		return (_auth_helper.GetAuth()->GetUserId());
#endif
	Q_UNUSED(sUserName);
	Q_UNUSED(sUserPassword);
	return quint32(-1);
}

}

#define CHECK_ACL_INFO(acl_info, owner_name, owner_password, owner_type, bAllowed, bWriteSet, bReadSet, bExecuteSet)\
	{\
		QCOMPARE(acl_info.OwnerName(), owner_name);\
		QCOMPARE(quint32(acl_info.OwnerType()), quint32(owner_type));\
		if (owner_type == CAclHelper::User)\
			QCOMPARE(quint32(acl_info.OwnerId()), PlatformIndependGetUserId(owner_name, owner_password));\
		QVERIFY(acl_info.IsAllow() == bAllowed);\
		QVERIFY(acl_info.IsDeny() == !bAllowed);\
		QVERIFY(acl_info.IsWrite() == bWriteSet);\
		QVERIFY(acl_info.IsRead() == bReadSet);\
		QVERIFY(acl_info.IsExecute() == bExecuteSet);\
	}

void CAclHelperTest::testGetFileAclsForUser()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());
	CAclSet _user_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin());
	QCOMPARE(quint32(_user_acls.size()), quint32(1));
	CAclInfo _acl = _user_acls.front();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											true, true, true, true)
}

void CAclHelperTest::testGetFileAclsForUserThatDoNotHasAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());
	CAclSet _user_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin2());
	QVERIFY(_user_acls.size() == 0);
}

void CAclHelperTest::testGetFileAclsForBothUsers()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), DENY, WRITE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), ALLOW, READ | EXECUTE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());
	CAclSet _user_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin());
	QCOMPARE(quint32(_user_acls.size()), quint32(1));
	CAclInfo _acl = _user_acls.front();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											true, true, true, true)

	_user_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin2());
	QCOMPARE(quint32(_user_acls.size()), quint32(2));
	_acl = _user_acls.front();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin2()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											false, true, false, false)
	_acl = _user_acls.back();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin2()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											true, false, true, true)
}

void CAclHelperTest::testGetFileAclsForBothUsersAndForGroup()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QString sGroupName = "wheel";

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), DENY, WRITE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), ALLOW, READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, sGroupName, ALLOW, READ));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());
	CAclSet _owner_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin());
	QCOMPARE(quint32(_owner_acls.size()), quint32(1));
	CAclInfo _acl = _owner_acls.front();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											true, true, true, true)

	_owner_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin2());
	QCOMPARE(quint32(_owner_acls.size()), quint32(2));
	_acl = _owner_acls.front();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin2()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											false, true, false, false)
	_acl = _owner_acls.back();
	CHECK_ACL_INFO(_acl, UTF8_2QSTR(TestConfig::getUserLogin2()), UTF8_2QSTR(TestConfig::getUserPassword()), CAclHelper::User,\
											true, false, true, true)

	_owner_acls = _acls.GetAclsByOwnerName(CAclHelper::Group, sGroupName);
	QCOMPARE(quint32(_owner_acls.size()), quint32(1));
	_acl = _owner_acls.front();
	CHECK_ACL_INFO(_acl, sGroupName, "", CAclHelper::Group,	true, false, true, false)
}

void CAclHelperTest::testGetEffectiveRightsForUser()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));

	CAuth::AccessMode nEffectiveRights = CAclHelper::GetEffectiveRightsForUser(m_sTestFileName, TestConfig::getUserLogin());
	QVERIFY(nEffectiveRights & CAuth::fileMayWrite);
	QVERIFY(nEffectiveRights & CAuth::fileMayRead);
	QVERIFY(nEffectiveRights & CAuth::fileMayExecute);
}

void CAclHelperTest::testGetEffectiveRightsForGroup()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QString sGroupName = "wheel";

	QVERIFY(AddAcl(m_sTestFileName, sGroupName, ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, sGroupName, DENY, WRITE));

	CAuth::AccessMode nEffectiveRights = CAclHelper::GetEffectiveRightsForGroup(m_sTestFileName, sGroupName);
	QVERIFY((nEffectiveRights & CAuth::fileMayWrite) == 0);
	QVERIFY(nEffectiveRights & CAuth::fileMayRead);
	QVERIFY(nEffectiveRights & CAuth::fileMayExecute);
}

void CAclHelperTest::testGetEffectiveRightsForUser2()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QString sGroupName = "wheel";

	QVERIFY(AddAcl(m_sTestFileName, sGroupName, ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), DENY, WRITE));

	CAuth::AccessMode nEffectiveRights = CAclHelper::GetEffectiveRightsForUser(m_sTestFileName, TestConfig::getUserLogin2());
	QVERIFY((nEffectiveRights & CAuth::fileMayWrite) == 0);
	QVERIFY(nEffectiveRights & CAuth::fileMayRead);
	QVERIFY(nEffectiveRights & CAuth::fileMayExecute);
}

void CAclHelperTest::testApplyAclsToFile()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QString sGroupName = "wheel";

	QVERIFY(AddAcl(m_sTestFileName, sGroupName, ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), DENY, WRITE));

	CAclSet _acls1 = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls1.size() != 0);

	QVERIFY(CAclHelper::ApplyAclsToFile(m_sTestFileName2, _acls1));

	CAclSet _acls2 = CAclHelper::GetFileAcls(m_sTestFileName2);

	QCOMPARE(quint32(_acls2.size()), quint32(_acls1.size()));

	for(int i = 0; i < _acls1.size(); ++i)
	{
		CAclInfo _acl1 = _acls1.at(i);
		CAclInfo _acl2 = _acls2.at(i);
		QVERIFY(_acl1 == _acl2);
	}
}

void CAclHelperTest::testApplyAclsToFileWithEmptyAclsList()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QString sGroupName = "wheel";

	QVERIFY(AddAcl(m_sTestFileName, sGroupName, ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin2(), DENY, WRITE));

	CAclSet _acls1 = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls1.size() != 0);

	QVERIFY(CAclHelper::ApplyAclsToFile(m_sTestFileName, CAclSet()));

	CAclSet _acls2 = CAclHelper::GetFileAcls(m_sTestFileName);

	QCOMPARE(quint32(_acls2.size()), quint32(0));
}

void CAclHelperTest::testGetFileAclsOnFileWithoutAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	CAclSet _acls1 = CAclHelper::GetFileAcls(m_sTestFileName2);
	QVERIFY(_acls1.isEmpty());
}

void CAclHelperTest::testAclInfoComparisionOperatorOnEqualAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), DENY, WRITE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());

	CAclSet _user_acls1 = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin());
	QCOMPARE(quint32(_user_acls1.size()), quint32(2));
	CAclInfo _acl1 = _user_acls1.front();

	CAclSet _user_acls2 = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin());
	QCOMPARE(quint32(_user_acls2.size()), quint32(2));
	CAclInfo _acl2 = _user_acls2.front();

	QVERIFY(_acl1 == _acl2);
}

void CAclHelperTest::testAclInfoComparisionOperatorOnNonEqualAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), DENY, WRITE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());

	CAclSet _user_acls = _acls.GetAclsByOwnerName(CAclHelper::User, TestConfig::getUserLogin());
	QCOMPARE(quint32(_user_acls.size()), quint32(2));

	CAclInfo _acl1 = _user_acls.front();
	CAclInfo _acl2 = _user_acls.back();

	QVERIFY(_acl1 != _acl2);
}

void CAclHelperTest::testGetEffectivePermissionsForUserOnFileWithoutAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	CAuth::AccessMode nAccessMode = CAclHelper::GetEffectiveRightsForUser(m_sTestFileName2, TestConfig::getUserLogin());
	QVERIFY(nAccessMode == 0);
}

void CAclHelperTest::testGetEffectivePermissionsForGroupOnFileWithoutAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	CAuth::AccessMode nAccessMode = CAclHelper::GetEffectiveRightsForUser(m_sTestFileName2, "wheel");
	QVERIFY(nAccessMode == 0);
}

void CAclHelperTest::testIsAclSupportedOnFileWithoutAcls()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	QVERIFY(!CAclHelper::IsAclSupported(m_sTestFileName2));
}

void CAclHelperTest::testApplyAclsToFileOnNonExistenFile()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	QVERIFY(!CAclHelper::ApplyAclsToFile(Uuid::createUuid().toString(), CAclSet()));
}

void CAclHelperTest::testGetFileAclsOnNonExistenFile()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	QVERIFY(CAclHelper::GetFileAcls(Uuid::createUuid().toString()).isEmpty());
}

void CAclHelperTest::testGetEffectivePermissionsForUserOnNonExistenFile()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	QVERIFY(CAclHelper::GetEffectiveRightsForUser(Uuid::createUuid().toString(), TestConfig::getUserLogin()) == 0);
}

void CAclHelperTest::testGetEffectivePermissionsForGroupOnNonExistenFile()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	QVERIFY(CAclHelper::GetEffectiveRightsForGroup(Uuid::createUuid().toString(), "wheel") == 0);
}

void CAclHelperTest::testIsAclSupportedOnNonExistenFile()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	QVERIFY(!CAclHelper::IsAclSupported(Uuid::createUuid().toString()));
}

void CAclHelperTest::testGetEffectivePermissionsForUserOnWrongUserName()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), DENY, WRITE));

	QVERIFY(CAclHelper::GetEffectiveRightsForUser(m_sTestFileName, Uuid::createUuid().toString()) == 0);
}

void CAclHelperTest::testGetEffectivePermissionsForGroupOnWrongGroupName()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), DENY, WRITE));

	QVERIFY(CAclHelper::GetEffectiveRightsForGroup(m_sTestFileName, Uuid::createUuid().toString()) == 0);
}

void CAclHelperTest::testGetAclsByOwnerNameOnWronOnWrongUserName()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), DENY, WRITE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());

	QVERIFY(_acls.GetAclsByOwnerName(CAclHelper::User, Uuid::createUuid().toString()).isEmpty());
}

void CAclHelperTest::testGetAclsByOwnerNameOnWronOnWrongGroupName()
{
#ifndef _MAC_
	QSKIP("Currently do not have implementation for the rest systems", SkipAll);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFileName))
		QSKIP("ACL not supported on file system so skipping", SkipAll);

	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFileName, TestConfig::getUserLogin(), DENY, WRITE));

	CAclSet _acls = CAclHelper::GetFileAcls(m_sTestFileName);
	QVERIFY(_acls.size());

	QVERIFY(_acls.GetAclsByOwnerName(CAclHelper::Group, Uuid::createUuid().toString()).isEmpty());
}
