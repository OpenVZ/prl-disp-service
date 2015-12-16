/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		CVmConfigurationTest.cpp
///
/// @author
///		aleksandera
///
/// @brief
///		Tests fixture class for testing CVmConfiguration class functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "CVmConfigurationTest.h"

#include <QDomDocument>
#include "XmlModel/VmConfig/CVmSoundInputs.h"
#include "XmlModel/VmConfig/CVmSoundOutputs.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/Logging/Logging.h"


CVmConfigurationTest::CVmConfigurationTest()
: m_pVmConfiguration(NULL),
  m_ctAutoStop(PAO_VM_SHUTDOWN),
  m_pVaConfig(NULL),
  m_pFile(NULL)
{
}

#define INITIALIZE_CONFIG(sConfigFileName)	m_pFile = new QFile(sConfigFileName);\
	QVERIFY(m_pFile->open(QIODevice::ReadOnly));\
	m_pVmConfiguration = new CVmConfiguration(m_pFile);

void CVmConfigurationTest::testIsValidOnValidConfiguration() {
	INITIALIZE_CONFIG("./CVmConfigurationTest_valid_vm_config.xml")
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CVmConfigurationTest::testIsValidVaConfig() {
	m_pFile = new QFile("./CVaConfigTest_valid_va_config.xml");
	QVERIFY(m_pFile->open(QIODevice::ReadOnly));
	m_pVaConfig = new CVaConfig(m_pFile);
	QCOMPARE(m_pVaConfig->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CVmConfigurationTest::testIsValidOnIdentificationNotPresent() {
	INITIALIZE_CONFIG("./CVmConfigurationTest_vm_config_ident_not_present.xml")
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_PARSE_VM_CONFIG);
}

void CVmConfigurationTest::testIsValidOnSettingsNotPresent() {
	INITIALIZE_CONFIG("./CVmConfigurationTest_vm_config_settings_not_present.xml")
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_PARSE_VM_CONFIG);
}

void CVmConfigurationTest::testIsValidOnHwInfoNotPresent() {
	INITIALIZE_CONFIG("./CVmConfigurationTest_vm_config_hwinfo_not_present.xml")
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_PARSE_VM_CONFIG);
}

void CVmConfigurationTest::testIsValidOnThirdPartyXmlFile() {
	INITIALIZE_CONFIG("./CVmConfigurationTest_third_party.xml")
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_PARSE_VM_CONFIG);
}

void CVmConfigurationTest::testInitializeFromString() {
	QFile _file("./CVmConfigurationTest_valid_vm_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QDomDocument _xmldoc;
	QVERIFY(_xmldoc.setContent(&_file));
	_file.close();
	m_pVmConfiguration = new CVmConfiguration(_xmldoc.toString());
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CVmConfigurationTest::testInitializeFromStringWithXmlns() {
	QFile _file("./CVmConfigurationTest_valid_config_with_xmlns.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _xml_str = _stream.readAll();
	_file.close();
	m_pVmConfiguration = new CVmConfiguration(_xml_str);
	QCOMPARE(m_pVmConfiguration->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CVmConfigurationTest::init()
{
	if ( ! m_qsTestPath.isEmpty() && QFile::exists( m_qsTestPath ) )
		CFileHelper::ClearAndDeleteDir( m_qsTestPath );
	m_qsTestPath.clear();
}

void CVmConfigurationTest::cleanup() {
	delete m_pVmConfiguration;
	m_pVmConfiguration = NULL;
	delete m_pVaConfig;
	m_pVaConfig = NULL;
	delete m_pFile;
	m_pFile = NULL;

	if ( ! m_qsTestPath.isEmpty() && QFile::exists( m_qsTestPath ) )
		CFileHelper::ClearAndDeleteDir( m_qsTestPath );
	m_qsTestPath.clear();
}

void CVmConfigurationTest::testFloppyDeviceConvertation()
{
	CVmFloppyDisk floppy1;
	floppy1.setIndex( 0 );
	floppy1.setEmulatedType( PVE::FloppyDiskImage );
	floppy1.setEnabled( PVE::DeviceEnabled );
	floppy1.setConnected( PVE::DeviceConnected );
	floppy1.setUserFriendlyName( "some path" );
	QString floppyXMLstr = ElementToString<CVmFloppyDisk*>(&floppy1, XML_VM_CONFIG_EL_HARDWARE );

	CVmFloppyDisk floppy2;
	QVERIFY(StringToElement<CVmFloppyDisk *>(&floppy2, floppyXMLstr));
	QCOMPARE(ElementToString<CVmFloppyDisk*>(&floppy1, XML_VM_CONFIG_EL_HARDWARE ), floppyXMLstr);
}

void CVmConfigurationTest::testSharedAppInDockDefaultValue()
{
	INITIALIZE_CONFIG("./CVmConfigurationTest_valid_vm_config.xml")
	QCOMPARE(quint32(m_pVmConfiguration->getVmSettings()->getVmTools()->\
		getVmSharedApplications()->getApplicationInDock()), quint32(PDM_APP_IN_DOCK_ALWAYS));
}

void CVmConfigurationTest::testEnableSpotlightDefaultValue()
{
	INITIALIZE_CONFIG("./CVmConfigurationTest_valid_vm_config.xml")
	QVERIFY(!m_pVmConfiguration->getVmSettings()->getVmTools()->getVmSharing()->\
		getGuestSharing()->isEnableSpotlight());
}

void CVmConfigurationTest::testValidateSignalsMech()
{
	testIsValidOnValidConfiguration();

// Test signals

	// Standard type
	bool bOk = connect( m_pVmConfiguration->getVmIdentification(),
						SIGNAL(signalVmNameChanged(QString )),
						SLOT(testVmName(QString )),
						Qt::QueuedConnection);
	QVERIFY(bOk);

	QVERIFY(m_qsVmName.isEmpty());
	m_pVmConfiguration->getVmIdentification()->setVmName("vm name");
	qApp->processEvents();
	QVERIFY(m_pVmConfiguration->getVmIdentification()->getVmName() == m_qsVmName);

	// Custom type
	bOk = connect(m_pVmConfiguration->getVmSettings()->getShutdown(),
				  SIGNAL(signalAutoStopChanged(PRL_VM_AUTOSTOP_OPTION )),
				  SLOT(testAutoStop(PRL_VM_AUTOSTOP_OPTION )),
				  Qt::QueuedConnection);
	QVERIFY(bOk);

	QVERIFY(m_ctAutoStop == PAO_VM_SHUTDOWN);
	m_pVmConfiguration->getVmSettings()->getShutdown()->setAutoStop(PAO_VM_STOP);
	qApp->processEvents();
	QVERIFY(m_pVmConfiguration->getVmSettings()->getShutdown()->getAutoStop() == m_ctAutoStop);
}

void CVmConfigurationTest::testVmName(QString v)
{
	m_qsVmName = v;
}

void CVmConfigurationTest::testAutoStop(PRL_VM_AUTOSTOP_OPTION v)
{
	m_ctAutoStop = v;
}

/*
 * N - new config (changed original config by our user),
 * C - current config (config on disk and changed by another user),
 * P - previous config (got as original config by our user)
 * --------------------------------------------
 * | \ | N, C | N, P | C, P | Result | Action |
 * --------------------------------------------
 * | 1 | ==   | any  | any  | OK     | nope   |
 * --------------------------------------------
 * | 2 | !=   | ==   | any  | OK     | N = C  |
 * --------------------------------------------
 * | 3 | !=   | !=   | ==   | OK     | nope   |
 * --------------------------------------------
 * | 4 | !=   | !=   | !=   | ERROR  | return |
 * --------------------------------------------
 * New config as result of merge
 */

void CVmConfigurationTest::testValidateMergeMechForField()
{
	testIsValidOnValidConfiguration();

	CVmConfiguration cfgNew;
	CVmConfiguration cfgCur;
	CVmConfiguration cfgPrev;

	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

// 1
	cfgNew.getVmSettings()->getVmCommonOptions()->setVmDescription("abc");
	cfgCur.getVmSettings()->getVmCommonOptions()->setVmDescription("abc");
	cfgPrev.getVmSettings()->getVmCommonOptions()->setVmDescription("qwert");

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

	QCOMPARE(cfgNew.getVmSettings()->getVmCommonOptions()->getVmDescription(),
				QString("abc"));
// 2
	cfgNew.getVmSettings()->getVmCommonOptions()->setVmDescription("asdfgh");
	cfgCur.getVmSettings()->getVmCommonOptions()->setVmDescription("12345");
	cfgPrev.getVmSettings()->getVmCommonOptions()->setVmDescription("asdfgh");

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

	QCOMPARE(cfgNew.getVmSettings()->getVmCommonOptions()->getVmDescription(),
				QString("12345"));
// 3
	cfgNew.getVmSettings()->getVmCommonOptions()->setVmDescription("zxcv");
	cfgCur.getVmSettings()->getVmCommonOptions()->setVmDescription("tyuty");
	cfgPrev.getVmSettings()->getVmCommonOptions()->setVmDescription("tyuty");

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

	QCOMPARE(cfgNew.getVmSettings()->getVmCommonOptions()->getVmDescription(),
				QString("zxcv"));
// 4
	cfgNew.getVmSettings()->getVmCommonOptions()->setVmDescription("ghkjhjk");
	cfgCur.getVmSettings()->getVmCommonOptions()->setVmDescription("mskALJ");
	cfgPrev.getVmSettings()->getVmCommonOptions()->setVmDescription("324SDFUJJmkm");

	QVERIFY( cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QCOMPARE( cfgNew.GetErrorMessage(),
				QString("ParallelsVirtualMachine.Settings.General.VmDescription") );
}

void CVmConfigurationTest::testValidateMergeMechForList()
{
	testIsValidOnValidConfiguration();

	m_pVmConfiguration->getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();
	m_pVmConfiguration->getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();
	QCOMPARE(m_pVmConfiguration->getVmHardwareList()->m_lstHardDisks.size(), 4);
	QCOMPARE(m_pVmConfiguration->getVmHardwareList()->m_lstOpticalDisks.size(), 2);

	CVmConfiguration cfgNew;
	CVmConfiguration cfgCur;
	CVmConfiguration cfgPrev;

// 4
	// a) different list size and different id sets
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	delete cfgNew.getVmHardwareList()->m_lstHardDisks[1];
	cfgNew.getVmHardwareList()->m_lstHardDisks.removeAt(1);
	cfgCur.getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();

	QVERIFY( cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QCOMPARE( cfgNew.GetErrorMessage(),
				QString("ParallelsVirtualMachine.Hardware.Hdd") );

	// b) the same list size and different id sets
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	delete cfgNew.getVmHardwareList()->m_lstHardDisks[2];
	cfgNew.getVmHardwareList()->m_lstHardDisks.removeAt(2);
	delete cfgCur.getVmHardwareList()->m_lstHardDisks[0];
	cfgCur.getVmHardwareList()->m_lstHardDisks.removeAt(0);
	delete cfgPrev.getVmHardwareList()->m_lstHardDisks[1];
	cfgPrev.getVmHardwareList()->m_lstHardDisks.removeAt(1);

	QVERIFY( cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QCOMPARE( cfgNew.GetErrorMessage(),
				QString("ParallelsVirtualMachine.Hardware.Hdd") );

// 2
	int i = -1;

	// a)
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgCur.getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();

	delete cfgCur.getVmHardwareList()->m_lstHardDisks[2];
	cfgCur.getVmHardwareList()->m_lstHardDisks.removeAt(2);
	delete cfgCur.getVmHardwareList()->m_lstHardDisks[1];
	cfgCur.getVmHardwareList()->m_lstHardDisks.removeAt(1);

	cfgCur.syncItemIds();

	QCOMPARE(cfgCur.getVmHardwareList()->m_lstHardDisks.size(), 3);

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

	QCOMPARE(cfgNew.getVmHardwareList()->m_lstHardDisks.size(),
				cfgCur.getVmHardwareList()->m_lstHardDisks.size());
	for(i = 0; i < cfgNew.getVmHardwareList()->m_lstHardDisks.size(); i++)
	{
		QCOMPARE(cfgNew.getVmHardwareList()->m_lstHardDisks[i]->getItemId(),
					cfgCur.getVmHardwareList()->m_lstHardDisks[i]->getItemId());
	}

	// b)
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	delete cfgCur.getVmHardwareList()->m_lstHardDisks[3];
	cfgCur.getVmHardwareList()->m_lstHardDisks.removeAt(3);

	cfgCur.getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();
	cfgCur.getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();

	QCOMPARE(cfgCur.getVmHardwareList()->m_lstHardDisks.size(), 5);
		// Check max id generation mech
		QCOMPARE(cfgCur.toString(), cfgCur.toString());
		cfgCur.syncItemIds();
		QCOMPARE(cfgCur.getVmHardwareList()->m_lstHardDisks[3]->getItemId(), 4);
		QCOMPARE(cfgCur.getVmHardwareList()->m_lstHardDisks[4]->getItemId(), 5);

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

	QCOMPARE(cfgNew.getVmHardwareList()->m_lstHardDisks.size(),
				cfgCur.getVmHardwareList()->m_lstHardDisks.size());
	for(i = 0; i < cfgNew.getVmHardwareList()->m_lstHardDisks.size(); i++)
	{
		QCOMPARE(cfgNew.getVmHardwareList()->m_lstHardDisks[i]->getItemId(),
					cfgCur.getVmHardwareList()->m_lstHardDisks[i]->getItemId());
	}

// 1, 3
	// a) OK list item merge
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgNew.getVmHardwareList()->m_lstHardDisks += new CVmHardDisk();
	cfgNew.getVmHardwareList()->m_lstHardDisks.last()->setDescription("abc");

	cfgNew.syncItemIds();

	cfgCur.getVmHardwareList()->m_lstHardDisks[2]->setDescription("qwerty");
	cfgCur.getVmHardwareList()->m_lstOpticalDisks[1]->setDescription("zxcvb");

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

	QVERIFY(cfgNew.getVmHardwareList()->m_lstHardDisks.size()
			> cfgCur.getVmHardwareList()->m_lstHardDisks.size());
	QCOMPARE(cfgNew.getVmHardwareList()->m_lstHardDisks.last()->getDescription(),
				QString("abc"));
	QCOMPARE(cfgNew.getVmHardwareList()->m_lstHardDisks[2]->getDescription(),
				QString("qwerty"));
	QCOMPARE(cfgNew.getVmHardwareList()->m_lstOpticalDisks[1]->getDescription(),
				QString("zxcvb"));

	// b) ERROR list item merge
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgNew.getVmHardwareList()->m_lstHardDisks[3]->setDescription("dsfsdf");
	cfgCur.getVmHardwareList()->m_lstHardDisks[3]->setDescription("asd");
	cfgPrev.getVmHardwareList()->m_lstHardDisks[3]->setDescription("ioipoi");

	QVERIFY( cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QCOMPARE( cfgNew.GetErrorMessage(),
				QString("ParallelsVirtualMachine.Hardware.[id=%1]Hdd.DeviceDescription")
				.arg(cfgNew.getVmHardwareList()->m_lstHardDisks[3]->getItemId()) );
}

void CVmConfigurationTest::testValidateRestrictMergeMech()
{
	testIsValidOnValidConfiguration();

	if (m_pVmConfiguration->getVmHardwareList()->m_lstFloppyDisks.isEmpty())
		m_pVmConfiguration->getVmHardwareList()->m_lstFloppyDisks += new CVmFloppyDisk;

	CVmConfiguration cfgNew;
	CVmConfiguration cfgCur;
	CVmConfiguration cfgPrev;

// 1 not restricted merge
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgNew.getVmHardwareList()->m_lstFloppyDisks[0]->setSystemName("my system name");
	cfgCur.getVmHardwareList()->m_lstFloppyDisks[0]->setUserFriendlyName("his user friendly name");

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());

// 2 restricted merge
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgNew.getVmHardwareList()->m_lstFloppyDisks[0]->setSystemName("my system name");
	cfgCur.getVmHardwareList()->m_lstFloppyDisks[0]->setUserFriendlyName("his user friendly name");

	QVERIFY( cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev, moEnableRestrictSections ) );
	QCOMPARE( cfgNew.GetErrorMessage(),
				QString("ParallelsVirtualMachine.Hardware.[id=%1]Fdd")
				.arg(cfgNew.getVmHardwareList()->m_lstFloppyDisks[0]->getItemId()) );
}

void CVmConfigurationTest::testValidateFixedFieldMergeMech()
{
	testIsValidOnValidConfiguration();

	if (m_pVmConfiguration->getVmHardwareList()->m_lstHardDisks.isEmpty())
		m_pVmConfiguration->getVmHardwareList()->m_lstHardDisks += new CVmHardDisk;

	CVmConfiguration cfgNew;
	CVmConfiguration cfgCur;
	CVmConfiguration cfgPrev;

// 1 merge without fixed fields (conflict)
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgNew.getVmHardwareList()->m_lstHardDisks[0]->setSizeOnDisk(1000);
	cfgCur.getVmHardwareList()->m_lstHardDisks[0]->setSizeOnDisk(2000);
	cfgPrev.getVmHardwareList()->m_lstHardDisks[0]->setSizeOnDisk(3000);

	QVERIFY( cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev ) );
	QCOMPARE( cfgNew.GetErrorMessage(),
				QString("ParallelsVirtualMachine.Hardware.[id=%1]Hdd.SizeOnDisk")
				.arg(cfgNew.getVmHardwareList()->m_lstHardDisks[0]->getItemId()) );

// 2 merge with fixed fields (no conflict)
	cfgNew.fromString(m_pVmConfiguration->toString());
	cfgCur.fromString(m_pVmConfiguration->toString());
	cfgPrev.fromString(m_pVmConfiguration->toString());

	cfgNew.getVmHardwareList()->m_lstHardDisks[0]->setSizeOnDisk(1000);
	cfgCur.getVmHardwareList()->m_lstHardDisks[0]->setSizeOnDisk(2000);
	cfgPrev.getVmHardwareList()->m_lstHardDisks[0]->setSizeOnDisk(3000);

	QVERIFY( ! cfgNew.mergeDocuments<CVmConfiguration>( &cfgCur, &cfgPrev, moEnableFixedFields ) );
	QVERIFY(cfgNew.GetErrorMessage().isEmpty());
	QCOMPARE( cfgNew.getVmHardwareList()->m_lstHardDisks[0]->getSizeOnDisk(),
				(unsigned long )2000 );
}

void CVmConfigurationTest::testThrowOffMilisecondsInTimeFields()
{
	CVmConfiguration cfg;
	QDateTime dt = QDateTime::currentDateTime();
	if ( ! dt.time().msec() )
		dt.addMSecs(345);

// 1 check setter of time type

	cfg.getVmIdentification()->setCreationDate(dt);
	QCOMPARE(cfg.getVmIdentification()->getCreationDate().time().msec(), 0);

// 2 check formated value via serialization

	dt = cfg.getVmIdentification()->getCreationDate();
	cfg.fromString( cfg.toString() );
	QCOMPARE(cfg.getVmIdentification()->getCreationDate(), dt);
}

void CVmConfigurationTest::testValidateDiffMech()
{
	CVmConfiguration old_cfg;
	for(int i = 0; i < 5; ++i)
	{
		old_cfg.getVmHardwareList()->m_lstOpticalDisks += new CVmOpticalDisk;
		old_cfg.getVmHardwareList()->m_lstOpticalDisks.last()->setItemId(i + 5);
	}

	CVmConfiguration new_cfg;

	QStringList lstDiffFullItemIds;

// 1 difference for fields

	new_cfg.fromString(old_cfg.toString());

	new_cfg.getVmIdentification()->setVmName("My VM");
	new_cfg.getVmSettings()->getVmCommonOptions()->setTemplate(
		! old_cfg.getVmSettings()->getVmCommonOptions()->isTemplate() );

	new_cfg.diffDocuments<CVmConfiguration>(&old_cfg, lstDiffFullItemIds);

	QVERIFY(lstDiffFullItemIds.contains("Identification.VmName"));
	QVERIFY(lstDiffFullItemIds.contains("Settings.General.IsTemplate"));

// 2 difference for fields in list

	new_cfg.fromString(old_cfg.toString());

	new_cfg.getVmHardwareList()->m_lstOpticalDisks[1]->setSystemName("My CD");
	new_cfg.getVmHardwareList()->m_lstOpticalDisks[3]->setRemote(
		! old_cfg.getVmHardwareList()->m_lstOpticalDisks[3]->isRemote() );

	new_cfg.diffDocuments<CVmConfiguration>(&old_cfg, lstDiffFullItemIds);

	QVERIFY(lstDiffFullItemIds.contains("Hardware.CdRom[6].SystemName"));
	QVERIFY(lstDiffFullItemIds.contains("Hardware.CdRom[8].Remote"));

// 3 difference for lists

	new_cfg.fromString(old_cfg.toString());

	delete new_cfg.getVmHardwareList()->m_lstOpticalDisks.takeAt( 2 );
	new_cfg.getVmHardwareList()->m_lstOpticalDisks += new CVmOpticalDisk;

	new_cfg.diffDocuments<CVmConfiguration>(&old_cfg, lstDiffFullItemIds);

	QVERIFY(lstDiffFullItemIds.contains("Hardware.CdRom[7]"));
	QVERIFY(lstDiffFullItemIds.contains("Hardware.CdRom[10]"));

}

void CVmConfigurationTest::testParallelPortWithOldVmConfig()
{
	INITIALIZE_CONFIG("./CVmConfigurationTest_virt_print.xml");

	QCOMPARE(m_pVmConfiguration->getVmHardwareList()->m_lstParallelPorts.size(), 0);
	QCOMPARE(m_pVmConfiguration->getVmHardwareList()->m_lstParallelPortOlds.size(), 4);
}

void CVmConfigurationTest::testSoundComparisonOperator()
{
	CVmSoundDevice vmSoundDev;
	CVmSoundDevice vmSoundDev1;
	CVmSoundDevice vmSoundDev2;

	vmSoundDev.getSoundInputs()->m_lstSoundDevices += new CVmSoundDevice;
	vmSoundDev.getSoundOutputs()->m_lstSoundDevices += new CVmSoundDevice;

	vmSoundDev.getSoundInputs()->m_lstSoundDevices[0]->setSystemName("/dev/in");
	vmSoundDev.getSoundOutputs()->m_lstSoundDevices[0]->setSystemName("/dev/out");
////
	vmSoundDev1.fromString(vmSoundDev.toString());
	vmSoundDev2.fromString(vmSoundDev.toString());

	QVERIFY(vmSoundDev1 == vmSoundDev2);
////
	vmSoundDev1.getSoundInputs()->m_lstSoundDevices[0]->setSystemName("/dev/in_abc");

	QVERIFY( ! (vmSoundDev1 == vmSoundDev2) );
////
	vmSoundDev1.fromString(vmSoundDev.toString());

	vmSoundDev2.getSoundOutputs()->m_lstSoundDevices[0]->setSystemName("/dev/out_dfg");

	QVERIFY( ! (vmSoundDev2 == vmSoundDev1) );
}

void CVmConfigurationTest::testConfigureWithDhcpFlagOnPostRead()
{
	CVmConfiguration _vm_conf;
	CVmGenericNetworkAdapter *pNetAdapter = new CVmGenericNetworkAdapter();
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

// Check auto switch dhcp to false for IPv4
	pNetAdapter->setConfigureWithDhcp(true);
	//set ipv6 only
	QStringList lstNewNetAddresses = QStringList()<<"ccc::1:1/100"<<"1::a:1/33";
	pNetAdapter->setNetAddresses(lstNewNetAddresses);
	//create copy
	CVmConfiguration _vm_conf_tmp;
	_vm_conf_tmp.fromString(_vm_conf.toString());
	QCOMPARE(_vm_conf_tmp.getVmHardwareList()->m_lstNetworkAdapters[0]->isConfigureWithDhcp(), true);

	//set ipv6 & ipv4
	lstNewNetAddresses = QStringList()<<"ccc::1:1/100"<<"1::a:1/33"<<"1.1.1.1/20";
	pNetAdapter->setNetAddresses(lstNewNetAddresses);
	//create copy
	_vm_conf_tmp.fromString(_vm_conf.toString());
	QCOMPARE(_vm_conf_tmp.getVmHardwareList()->m_lstNetworkAdapters[0]->isConfigureWithDhcp(), false);

// Check auto switch dhcp to false for IPv6
	pNetAdapter->setConfigureWithDhcpIPv6(true);
	//set ipv4 only
	lstNewNetAddresses = QStringList()<<"4.4.4.4./10"<<"10.20.30.40/24";
	pNetAdapter->setNetAddresses(lstNewNetAddresses);
	//create copy
	_vm_conf_tmp.fromString(_vm_conf.toString());
	QCOMPARE(_vm_conf_tmp.getVmHardwareList()->m_lstNetworkAdapters[0]->isConfigureWithDhcpIPv6(), true);

	//set ipv6 & ipv4
	lstNewNetAddresses = QStringList()<<"ccc::1:1/100"<<"1::a:1/33"<<"1.1.1.1/20";
	pNetAdapter->setNetAddresses(lstNewNetAddresses);
	//create copy
	_vm_conf_tmp.fromString(_vm_conf.toString());
	QCOMPARE(_vm_conf_tmp.getVmHardwareList()->m_lstNetworkAdapters[0]->isConfigureWithDhcpIPv6(), false);
}

void CVmConfigurationTest::testPatchMechViaPropertyCall()
{
	CVmConfiguration cfg;

	QVERIFY(cfg.setPropertyValue("Settings.Tools.Coherence.GroupAllWindows.patch_stamp",
								 QVariant("my text")));
	QCOMPARE(cfg.getPropertyValue("Settings.Tools.Coherence.GroupAllWindows.patch_stamp").toString(),
			 QString("my text"));

	cfg.fromString(cfg.toString());
	QCOMPARE(cfg.getPropertyValue("Settings.Tools.Coherence.GroupAllWindows.patch_stamp").toString(),
			 QString("my text"));
}

void CVmConfigurationTest::testAssignOperator()
{
	testIsValidOnValidConfiguration();

	QString qsVmConfigOrig = m_pVmConfiguration->toString();

	CVmConfiguration vmConfigCopy;
	vmConfigCopy = *m_pVmConfiguration;

	QCOMPARE(vmConfigCopy.toString(), qsVmConfigOrig);
}

void CVmConfigurationTest::testAssignOperatorOptimization()
{
	ParallelsDirs::Init(PAM_SERVER);

	testIsValidOnValidConfiguration();
	CVmConfiguration vmConfigCopy;

	WRITE_TRACE(DBG_FATAL, "####################### B: STR %s", QSTR2UTF8(QTime::currentTime().toString("HH:mm:ss.zzz")));
	for(int i = 0; i < 1000; i++)
	{
		vmConfigCopy.fromString(m_pVmConfiguration->toString());
	}
	WRITE_TRACE(DBG_FATAL, "####################### E: STR %s", QSTR2UTF8(QTime::currentTime().toString("HH:mm:ss.zzz")));

	WRITE_TRACE(DBG_FATAL, "####################### B: = %s", QSTR2UTF8(QTime::currentTime().toString("HH:mm:ss.zzz")));
	for(int i = 0; i < 1000; i++)
	{
		vmConfigCopy = *m_pVmConfiguration;
	}
	WRITE_TRACE(DBG_FATAL, "####################### E: = %s", QSTR2UTF8(QTime::currentTime().toString("HH:mm:ss.zzz")));
}

static QStringList s_lstDevTags = QStringList()
	<< "Fdd" << "CdRom" << "Hdd" << "Serial" << "Printer";

#define IS_DEV_PORT(qsDevTag) \
	(qsDevTag == "Serial" || qsDevTag  == "Printer")

void CVmConfigurationTest::testTransformationDevicesPathsInsideVmBundle()
{
	m_qsTestPath = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	QString qsVmDir = m_qsTestPath;

	QVERIFY(QDir().mkpath(qsVmDir));
	QString qsConfFile = qsVmDir + "/"VMDIR_DEFAULT_VM_CONFIG_FILE;

	CVmConfiguration cfg;

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->addListItem(qsDevTag), 0);
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].EmulatedType",
						IS_DEV_PORT(qsDevTag) ? PDT_USE_OUTPUT_FILE : PDT_USE_IMAGE_FILE));
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].SystemName",
														  qsDevTag + ".img"));
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].UserFriendlyName",
														  qsDevTag + ".img"));
	}

	QFile cf(qsConfFile);

	for(int i = 0; i < 2; i++)
	{
		QVERIFY( ! cfg.saveToFile(&cf, true, false) );

// Check relative devices paths without transformation

		QVERIFY( ! cfg.loadFromFile(&cf, false) );

		foreach(QString qsDevTag, s_lstDevTags)
		{
			QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
															  qsDevTag + ".img");
			QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
															  qsDevTag + ".img");
		}

// Check absolute devices paths with transformation

		QVERIFY( ! cfg.loadFromFile(&cf, true) );

		foreach(QString qsDevTag, s_lstDevTags)
		{
			QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
															  qsVmDir + "/" + qsDevTag + ".img");
			QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
															  qsVmDir + "/" + qsDevTag + ".img");
		}

		QVERIFY( ! cfg.saveToFile(&cf, true, true) );

// Check relative devices paths after transformation

		QVERIFY( ! cfg.loadFromFile(&cf, false) );

		foreach(QString qsDevTag, s_lstDevTags)
		{
			QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
															  qsDevTag + ".img");
			QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
															  qsDevTag + ".img");
		}

		cfg.getExternalConfigInfo()->setType(PEVT_VMWARE);
	}
}

void CVmConfigurationTest::testTransformationDevicesPathsOnShadowVm()
{
	m_qsTestPath = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	QString qsShadowVmDir = m_qsTestPath;
	QString qsVmDir = qsShadowVmDir + "/" + Uuid::createUuid().toString();

	QVERIFY(QDir().mkpath(qsVmDir));
	QString qsConfFile = qsVmDir + "/"VMDIR_DEFAULT_VM_CONFIG_FILE;

	CVmConfiguration cfg;
	cfg.getExternalConfigInfo()->setType(PEVT_VMWARE);

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->addListItem(qsDevTag), 0);
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].EmulatedType",
						IS_DEV_PORT(qsDevTag) ? PDT_USE_OUTPUT_FILE : PDT_USE_IMAGE_FILE));
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].SystemName",
														  "../a/b/c/" + qsDevTag + ".img"));
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].UserFriendlyName",
														  "../a/b/c/" + qsDevTag + ".img"));
	}

	QFile cf(qsConfFile);
	QVERIFY( ! cfg.saveToFile(&cf, true, false) );

// Check relative devices paths without transformation

	QVERIFY( ! cfg.loadFromFile(&cf, false) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  "../a/b/c/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  "../a/b/c/" + qsDevTag + ".img");
	}

// Check absolute devices paths with transformation

	QVERIFY( ! cfg.loadFromFile(&cf, true) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  qsShadowVmDir + "/a/b/c/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  qsShadowVmDir + "/a/b/c/" + qsDevTag + ".img");
	}

	QVERIFY( ! cfg.saveToFile(&cf, true, true) );

// Check relative devices paths after transformation

	QVERIFY( ! cfg.loadFromFile(&cf, false) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  "../a/b/c/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  "../a/b/c/" + qsDevTag + ".img");
	}
}

void CVmConfigurationTest::testTransformationDevicesPathsOnExtDir()
{
	m_qsTestPath = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	QString qsExtDir = m_qsTestPath;
	QString qsVmDir = qsExtDir + "/" + Uuid::createUuid().toString() + "/" + Uuid::createUuid().toString();

	QVERIFY(QDir().mkpath(qsVmDir));
	QString qsConfFile = qsVmDir + "/"VMDIR_DEFAULT_VM_CONFIG_FILE;

	CVmConfiguration cfg;

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->addListItem(qsDevTag), 0);
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].EmulatedType",
						IS_DEV_PORT(qsDevTag) ? PDT_USE_OUTPUT_FILE : PDT_USE_IMAGE_FILE));
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].SystemName",
														  qsExtDir + "/" + qsDevTag + ".img"));
		QVERIFY(cfg.getVmHardwareList()->setPropertyValue(qsDevTag + "[0].UserFriendlyName",
														  qsExtDir + "/" + qsDevTag + ".img"));
	}

	QFile cf(qsConfFile);
	QVERIFY( ! cfg.saveToFile(&cf, true, true) );

// Check absolute devices paths after transformation

	QVERIFY( ! cfg.loadFromFile(&cf, false) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
	}

	QVERIFY( ! cfg.loadFromFile(&cf, true) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
	}

// Check absolute devices paths after transformation for shadow VM

	cfg.getExternalConfigInfo()->setType(PEVT_VMWARE);
	QVERIFY( ! cfg.saveToFile(&cf, true, true) );

	QVERIFY( ! cfg.loadFromFile(&cf, false) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
	}

	QVERIFY( ! cfg.loadFromFile(&cf, true) );

	foreach(QString qsDevTag, s_lstDevTags)
	{
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].SystemName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
		QCOMPARE(cfg.getVmHardwareList()->getPropertyValue(qsDevTag + "[0].UserFriendlyName").toString(),
														  qsExtDir + "/" + qsDevTag + ".img");
	}
}


QTEST_MAIN(CVmConfigurationTest)
