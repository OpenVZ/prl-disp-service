///////////////////////////////////////////////////////////////////////////////
///
/// @file CDispatcherConfigTest.cpp
///
/// Tests fixture class for testing CDispatcherConfig class functionality.
///
/// @author van
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

#include "CDispatcherConfigTest.h"

#include <QDomDocument>

CDispatcherConfigTest::CDispatcherConfigTest() : m_pDispatcherConfig(NULL), m_pFile(NULL) {
}

#define INITIALIZE_CONFIG(sConfigFileName)	m_pFile = new QFile(sConfigFileName);\
	QVERIFY(m_pFile->open(QIODevice::ReadOnly));\
	m_pDispatcherConfig = new CDispatcherConfig(m_pFile);

void CDispatcherConfigTest::testIsValidOnValidDispConfig() {
	INITIALIZE_CONFIG("./CDispatcherConfigTest_valid_disp_config.xml")
	QCOMPARE(m_pDispatcherConfig->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CDispatcherConfigTest::testInitializeFromString() {
	QFile _file("./CDispatcherConfigTest_valid_disp_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QDomDocument _xmldoc;
	QVERIFY(_xmldoc.setContent(&_file));
	_file.close();
	m_pDispatcherConfig = new CDispatcherConfig(_xmldoc.toString());
	QCOMPARE(m_pDispatcherConfig->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CDispatcherConfigTest::cleanup() {
	delete m_pDispatcherConfig;
	m_pDispatcherConfig = NULL;
	delete m_pFile;
	m_pFile = NULL;
}

void CDispatcherConfigTest::testChangePropertiesMech()
{
	testInitializeFromString();

	bool bValueChanged = false;

// 1 change property by path
	unsigned int nHostRamSize = 254375436;
	QVERIFY( m_pDispatcherConfig->setPropertyValue(
				"ServerSettings.CommonPreferences.MemoryPreferences.HostRamSize",
				QVariant(nHostRamSize), &bValueChanged) );
	QCOMPARE(bValueChanged, true);

	QVERIFY( m_pDispatcherConfig->setPropertyValue(
				"ServerSettings.CommonPreferences.MemoryPreferences.HostRamSize",
				QVariant(nHostRamSize), &bValueChanged) );
	QCOMPARE(bValueChanged, false);

	QCOMPARE( m_pDispatcherConfig->getPropertyValue(
				"ServerSettings.CommonPreferences.MemoryPreferences.HostRamSize")
				.toUInt(), nHostRamSize );
	QCOMPARE( m_pDispatcherConfig->getDispatcherSettings()
				->getCommonPreferences()->getMemoryPreferences()->getHostRamSize(),
				nHostRamSize );

// 2 wrong path for set property
	QVERIFY( ! m_pDispatcherConfig->setPropertyValue(
				"ServerSettings.CommonPreferences.abc.HostRamSize",
				QVariant(nHostRamSize)) );

// 3 wrong path for get property
	QVERIFY( ! m_pDispatcherConfig->getPropertyValue(
				"ServerSettings.CommonPreferences.MemoryPreferences.zzz")
				.isValid() );

// 4 enum as qlonglong
	m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
		->getWorkspacePreferences()->setUsbConnectionType(PUD_CONNECT_TO_GUEST_OS);

	bool bOk = false;
	QCOMPARE(m_pDispatcherConfig
				->getPropertyValue("ServerSettings.CommonPreferences.WorkspacePreferences.UsbConnectionType")
				.toLongLong(&bOk),
				(qlonglong )PUD_CONNECT_TO_GUEST_OS);
	QVERIFY(bOk);
}

void CDispatcherConfigTest::testChangeListPropertiesMech()
{
	m_pDispatcherConfig = new CDispatcherConfig;

// Prepare list

	QList<CDispUser* >& lstDispUsers
		= m_pDispatcherConfig->getDispatcherSettings()->getUsersPreferences()->m_lstDispUsers;

	for(int i = 0; i < 5; ++i)
		lstDispUsers += new CDispUser;
	m_pDispatcherConfig->syncItemIds();

	m_pDispatcherConfig->getDispatcherSettings()->getUsersPreferences()->ClearLists();

	for(int i = 0; i < 5; ++i)
	{
		lstDispUsers += new CDispUser;
		lstDispUsers.last()->setUserName(QString("user %1").arg(i + 1));
	}
	m_pDispatcherConfig->syncItemIds();

// Now id = [5, 6, 7, 8, 9]

// Check reserved fields

	QList<int > lstIds = QList<int >() << 5 << 6 << 7 << 8 << 9;

	QCOMPARE( m_pDispatcherConfig->getPropertyValue(
				QString("ServerSettings.UsersPreferences.ParallelsUser[7].itemId")).toInt(),
			 7 );
	QCOMPARE( m_pDispatcherConfig->getPropertyValue(
				QString("ServerSettings.UsersPreferences.ParallelsUser.maxItemId")).toInt(),
			 9 );
	QVERIFY( m_pDispatcherConfig->getPropertyValue(
				QString("ServerSettings.UsersPreferences.ParallelsUser.listItemIds")).value<QList<int > >()
			 == lstIds );

// 1 get list properties

	for(int i = 0; i < 5; ++i)
	{
		QString qsUserName =
			m_pDispatcherConfig->getPropertyValue(
				QString("ServerSettings.UsersPreferences.ParallelsUser[%1].Name").arg(5 + i)
				).toString();
		QCOMPARE(qsUserName, QString("user %1").arg(i + 1));
	}

// 2 set list properties

	for(int i = 0; i < 5; ++i)
	{
		m_pDispatcherConfig->setPropertyValue(
			QString("ServerSettings.UsersPreferences.ParallelsUser[%1].Name").arg(5 + i)
			, QVariant(QString("new user %1").arg(i + 1))
			);

		QString qsUserName =
			m_pDispatcherConfig->getPropertyValue(
				QString("ServerSettings.UsersPreferences.ParallelsUser[%1].Name").arg(5 + i)
				).toString();
		QCOMPARE(qsUserName, QString("new user %1").arg(i + 1));
	}

// 3 wrong path

	QVERIFY( ! m_pDispatcherConfig->setPropertyValue(
			"ServerSettings.UsersPreferences.ParallelsUser[4].Name"
			, QVariant("abc")) );
	QVERIFY( ! m_pDispatcherConfig->setPropertyValue(
			"ServerSettings.UsersPreferences.ParallelsUser[10].Name"
			, QVariant("abc")) );

	QVERIFY( ! m_pDispatcherConfig->getPropertyValue(
			"ServerSettings.UsersPreferences.ParallelsUser[4].Name")
			.isValid() );
	QVERIFY( ! m_pDispatcherConfig->getPropertyValue(
			"ServerSettings.UsersPreferences.ParallelsUser[10].Name")
			.isValid() );
}

void CDispatcherConfigTest::testChangeListMech()
{
	m_pDispatcherConfig = new CDispatcherConfig;

// Prepare list

	QList<CDispUser* >& lstDispUsers
		= m_pDispatcherConfig->getDispatcherSettings()->getUsersPreferences()->m_lstDispUsers;

	for(int i = 0; i < 10; ++i)
		lstDispUsers += new CDispUser;
	m_pDispatcherConfig->syncItemIds();

// 1 add list item

	int nItemId = m_pDispatcherConfig->addListItem("ServerSettings.UsersPreferences.ParallelsUser");
	QCOMPARE(nItemId, 10);

	QVERIFY( m_pDispatcherConfig->setPropertyValue(
			"ServerSettings.UsersPreferences.ParallelsUser[10].Name", QVariant("User 10")) );

	nItemId = m_pDispatcherConfig->addListItem("ServerSettings.UsersPreferences.ParallelsUser");
	QCOMPARE(nItemId, 11);

// 2 add list item with error

	nItemId = m_pDispatcherConfig->addListItem("ServerSettings.UsersPreferences.NotParallelsUser");
	QCOMPARE(nItemId, -1);

// 3 delete list item

	QVERIFY( m_pDispatcherConfig->deleteListItem("ServerSettings.UsersPreferences.ParallelsUser[4]") );

// 4 delete list item with error

	QVERIFY( ! m_pDispatcherConfig->deleteListItem("ServerSettings.UsersPreferences.ParallelsUser[4]") );
	QVERIFY( ! m_pDispatcherConfig->deleteListItem("ServerSettings.UsersPreferences.ParallelsUser[12]") );

// 5 more complex addition

	nItemId = m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity");
	QCOMPARE(nItemId, 0);
	nItemId = m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[0].Association" );
	QCOMPARE(nItemId, 0);
	QVERIFY( ! m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
				->getUsbPreferences()->m_lstAuthenticUsbMap.isEmpty() );
	QVERIFY( ! m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
				->getUsbPreferences()->m_lstAuthenticUsbMap[0]->m_lstAssociations.isEmpty() );

// 6 more complex deletition

	QVERIFY( m_pDispatcherConfig->deleteListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[0].Association[0]") );
	QVERIFY( m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
				->getUsbPreferences()->m_lstAuthenticUsbMap[0]->m_lstAssociations.isEmpty() );
}

void CDispatcherConfigTest::testFullItemId()
{
	m_pDispatcherConfig = new CDispatcherConfig;

// Prepare complex path

	QCOMPARE(m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity")
			 , 0);
	QVERIFY( m_pDispatcherConfig->deleteListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[0]") );
	QCOMPARE(m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity")
			 , 1);
	QVERIFY( m_pDispatcherConfig->deleteListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[1]") );
	QCOMPARE(m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity")
			 , 2);
	QVERIFY( m_pDispatcherConfig->deleteListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[2]") );
////
	QCOMPARE(m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity")
			 , 3);
	QCOMPARE(m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity")
			 , 4);
	QCOMPARE(m_pDispatcherConfig->addListItem("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity")
			 , 5);
////
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 0);
	QVERIFY(m_pDispatcherConfig->deleteListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association[0]" ) );
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 1);
	QVERIFY(m_pDispatcherConfig->deleteListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association[1]" ) );
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 2);
	QVERIFY(m_pDispatcherConfig->deleteListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association[2]" ) );
////
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 3);
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 4);
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 5);
	QCOMPARE(m_pDispatcherConfig->addListItem(
		"ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association" ), 6);

// 1 check serialization case

	QString qsVmUuid = Uuid::createUuid().toString();

	m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
		->getUsbPreferences()->m_lstAuthenticUsbMap[1]->m_lstAssociations[2]->setVmUuid(qsVmUuid);

	QString qsDoc = m_pDispatcherConfig->toString();
	m_pDispatcherConfig->syncItemIds();

	QCOMPARE( m_pDispatcherConfig->getVersion_id(), QString("Version"));

	QString qsFullId = m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
		->getUsbPreferences()->m_lstAuthenticUsbMap[1]->m_lstAssociations[2]->getVmUuid_id();
	QCOMPARE(qsFullId,
			QString("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association[5].VmUuid"));

	QCOMPARE(m_pDispatcherConfig->getPropertyValue(qsFullId).toString(), qsVmUuid);

// 2 check deserialization case

	delete m_pDispatcherConfig;
	m_pDispatcherConfig = new CDispatcherConfig;
	m_pDispatcherConfig->fromString(qsDoc);

	qsFullId = m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()
		->getUsbPreferences()->m_lstAuthenticUsbMap[1]->m_lstAssociations[2]->getVmUuid_id();
	QCOMPARE(qsFullId,
		QString("ServerSettings.CommonPreferences.UsbPreferences.UsbIdentity[4].Association[5].VmUuid"));

	QCOMPARE(m_pDispatcherConfig->getPropertyValue(qsFullId).toString(), qsVmUuid);

}

QTEST_MAIN(CDispatcherConfigTest)
