/////////////////////////////////////////////////////////////////////////////
///
///	@file CPrlDataSerializerTest.cpp
///
///	Tests fixture class for testing data serializer mech which alternative to QDom model serialization.
///
///	@author sandro
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
/////////////////////////////////////////////////////////////////////////////

#include "CPrlDataSerializerTest.h"
#include "Libraries/PrlDataSerializer/CPrlStringDataSerializer.h"
#include "Libraries/PrlDataSerializer/CPrlOpaqueTypeDataSerializer.h"
#include "Interfaces/ParallelsNamespace.h"
#include "XmlModel/HostHardwareInfo/CSystemStatistics.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/Messaging/CVmEventParameterList.h"
#include "XmlModel/Messaging/CVmBinaryEventParameter.h"
#include "Tests/CommonTestsUtils.h"
#include <QBuffer>
#include <QDataStream>

#define PREPARE_STREAM\
	QByteArray _byte_array;\
	QBuffer _buffer(&_byte_array);\
	QVERIFY(_buffer.open(QIODevice::ReadWrite));\
	QDataStream _data_stream(&_buffer);\
	_data_stream.setVersion(QDataStream::Qt_4_0);

void CPrlDataSerializerTest::testStringSerializerSerialize()
{
	PREPARE_STREAM

	QString sExpectedString = UTF8_2QSTR("Это тестовая строка - на выходе теста мы должны получить точно\nтакую же строку, надеюсь ;)");
	CPrlStringDataSerializer(sExpectedString).Serialize(_data_stream);

	QCOMPARE((quint32)_byte_array.size(), (quint32)(sizeof(quint32) + sExpectedString.toUtf8().size()));

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	quint32 nActualStringDataSize = 0;
	_data_stream>>nActualStringDataSize;

	QCOMPARE(nActualStringDataSize, (quint32)sExpectedString.toUtf8().size());

	QByteArray sDataBuf;
	sDataBuf.resize(nActualStringDataSize);

	QCOMPARE((quint32)_data_stream.readRawData(sDataBuf.data(), nActualStringDataSize), nActualStringDataSize);

	QString sActualString = UTF8_2QSTR(sDataBuf);

	QCOMPARE(sActualString, sExpectedString);
}

void CPrlDataSerializerTest::testStringSerializerDeserialize()
{
	PREPARE_STREAM

	QString sExpectedString = UTF8_2QSTR("Это тестовая строка - на выходе теста мы должны получить точно\nтакую же строку, надеюсь ;)");
	QByteArray sDataBuf = sExpectedString.toUtf8();

	_data_stream<<((quint32)sDataBuf.size());
	_data_stream.writeRawData(sDataBuf.constData(), sDataBuf.size());

	QVERIFY(_buffer.reset());//Go to the start of buffer in order to retrieve necessary data from it now

	QString sActualString;
	CPrlStringDataSerializer(sActualString).Deserialize(_data_stream);

	QCOMPARE(sActualString, sExpectedString);
}

void CPrlDataSerializerTest::testOpaqueTypeSerializerSerialize()
{
	PREPARE_STREAM

	PVE::ProductActivationState nExpectedValue = PVE::LicActivatedTrial;

	PrlOpaqueSerializer((quint32 &)PrlOpaqueTypeConverter(nExpectedValue)).Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	quint32 nActualValue = 0;
	_data_stream>>nActualValue;

	QCOMPARE(nActualValue, (quint32)nExpectedValue);
}

void CPrlDataSerializerTest::testOpaqueTypeSerializerDeserialize()
{
	PREPARE_STREAM

	PVE::ProductActivationState nExpectedValue = PVE::LicActivatedTrial;

	_data_stream<<((quint32)nExpectedValue);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	PVE::ProductActivationState nActualValue = PVE::LicStatusUnknown;
	PrlOpaqueSerializer((quint32 &)PrlOpaqueTypeConverter(nActualValue)).Deserialize(_data_stream);

	QCOMPARE((quint32)nActualValue, (quint32)nExpectedValue);
}

void CPrlDataSerializerTest::testCpuStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CCpuStatistics _expected;
	_expected.setPercentsUsage(56);
	_expected.setTotalTime(100000000000ll);
	_expected.setUserTime(40000000000ll);
	_expected.setSystemTime(60000000000ll);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CCpuStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CCpuStatistics *>(&_actual, "statistics"), ElementToString<CCpuStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testDiskStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CDiskStatistics _expected;
	_expected.setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
	_expected.setFreeDiskSpace(30000000000ll);
	_expected.setUsageDiskSpace(10000000000ll);

	CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
	pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
	pPartition1->setFreeDiskSpace(15000000000ll);
	pPartition1->setUsageDiskSpace(5000000000ll);
	_expected.m_lstPartitionsStatistics.append(pPartition1);

	CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
	pPartition2->setFreeDiskSpace(15000000000ll);
	pPartition2->setUsageDiskSpace(5000000000ll);
	_expected.m_lstPartitionsStatistics.append(pPartition2);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CDiskStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CDiskStatistics *>(&_actual, "statistics"), ElementToString<CDiskStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testMemoryStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CMemoryStatistics _expected;
	_expected.setTotalSize(100000000000ll);
	_expected.setUsageSize(40000000000ll);
	_expected.setFreeSize(60000000000ll);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CMemoryStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CMemoryStatistics *>(&_actual, "statistics"), ElementToString<CMemoryStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testSwapStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CSwapStatistics _expected;
	_expected.setTotalSize(100000000000ll);
	_expected.setUsageSize(40000000000ll);
	_expected.setFreeSize(60000000000ll);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSwapStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CSwapStatistics *>(&_actual, "statistics"), ElementToString<CSwapStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testUptimeStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CUptimeStatistics _expected;
	_expected.setOsUptime(1565476889ll);
	_expected.setDispatcherUptime(44353545ll);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CUptimeStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CUptimeStatistics *>(&_actual, "statistics"), ElementToString<CUptimeStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testProcStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CProcInfoStatistics _expected;
	_expected.setCommandName(UTF8_2QSTR("какая-то команда"));
	_expected.setProcId(4435);
	_expected.setOwnerUser(UTF8_2QSTR("какой-то пользователь"));
	_expected.setTotalMemUsage(2400000000ll);
	_expected.setRealMemUsage(1200000000ll);
	_expected.setVirtualMemUsage(1200000000ll);
	_expected.setStartTime(4435787837383ll);
	_expected.setTotalTime(4000000000000ll);
	_expected.setUserTime(3000000000000ll);
	_expected.setSystemTime(1000000000000ll);
	_expected.setState(PPS_PROC_ZOMBIE);
	_expected.setPercentsUsage(38);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CProcInfoStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CProcInfoStatistics *>(&_actual, "statistics"), ElementToString<CProcInfoStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testProcStatisticsWithEmptyStringsValuesSerializeDeserialize()
{
	PREPARE_STREAM

	CProcInfoStatistics _expected;
	_expected.setProcId(4435);
	_expected.setTotalMemUsage(2400000000ll);
	_expected.setRealMemUsage(1200000000ll);
	_expected.setVirtualMemUsage(1200000000ll);
	_expected.setStartTime(4435787837383ll);
	_expected.setTotalTime(4000000000000ll);
	_expected.setUserTime(3000000000000ll);
	_expected.setSystemTime(1000000000000ll);
	_expected.setState(PPS_PROC_ZOMBIE);
	_expected.setPercentsUsage(38);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CProcInfoStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CProcInfoStatistics *>(&_actual, "statistics"), ElementToString<CProcInfoStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testProcStatisticsWithEmptyCommandNameSerializeDeserialize()
{
	PREPARE_STREAM

	CProcInfoStatistics _expected;
	_expected.setProcId(4435);
	_expected.setOwnerUser(UTF8_2QSTR("какой-то пользователь"));
	_expected.setTotalMemUsage(2400000000ll);
	_expected.setRealMemUsage(1200000000ll);
	_expected.setVirtualMemUsage(1200000000ll);
	_expected.setStartTime(4435787837383ll);
	_expected.setTotalTime(4000000000000ll);
	_expected.setUserTime(3000000000000ll);
	_expected.setSystemTime(1000000000000ll);
	_expected.setState(PPS_PROC_ZOMBIE);
	_expected.setPercentsUsage(38);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CProcInfoStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CProcInfoStatistics *>(&_actual, "statistics"), ElementToString<CProcInfoStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testProcStatisticsWithEmptyOwnerUserSerializeDeserialize()
{
	PREPARE_STREAM

	CProcInfoStatistics _expected;
	_expected.setCommandName(UTF8_2QSTR("какая-то команда"));
	_expected.setProcId(4435);
	_expected.setTotalMemUsage(2400000000ll);
	_expected.setRealMemUsage(1200000000ll);
	_expected.setVirtualMemUsage(1200000000ll);
	_expected.setStartTime(4435787837383ll);
	_expected.setTotalTime(4000000000000ll);
	_expected.setUserTime(3000000000000ll);
	_expected.setSystemTime(1000000000000ll);
	_expected.setState(PPS_PROC_ZOMBIE);
	_expected.setPercentsUsage(38);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CProcInfoStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CProcInfoStatistics *>(&_actual, "statistics"), ElementToString<CProcInfoStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testNetIfaceStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CNetIfaceStatistics _expected;
	_expected.setIfaceSystemName(UTF8_2QSTR("какое-то системное имя сетевого интерфейса"));
	_expected.setInDataSize(2400000000ll);
	_expected.setOutDataSize(1200000000ll);
	_expected.setInPkgsCount(120000ll);
	_expected.setOutPkgsCount(4435787ll);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CNetIfaceStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CNetIfaceStatistics *>(&_actual, "statistics"), ElementToString<CNetIfaceStatistics *>(&_expected, "statistics"));
}

void CPrlDataSerializerTest::testUserSessionStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CUserStatistics _expected;
	_expected.setUserName(UTF8_2QSTR("какое-то системное имя пользователя"));
	_expected.setServiceName(UTF8_2QSTR("какое-то наименование сервиса"));
	_expected.setHostName(UTF8_2QSTR("какое-то имя хоста"));
	_expected.setSessionTime(2400000000ll);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CUserStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(ElementToString<CUserStatistics *>(&_actual, "statistics"), ElementToString<CUserStatistics *>(&_expected, "statistics"));
}

#define ADD_CPU_STATISTICS_ELEMENT(nPercents, nTotalTime, nUserTime, nSystemTime)\
	{\
		CCpuStatistics *_element = new CCpuStatistics;\
		_element->setPercentsUsage(nPercents);\
		_element->setTotalTime(nTotalTime);\
		_element->setUserTime(nUserTime);\
		_element->setSystemTime(nSystemTime);\
		_expected.m_lstCpusStatistics.append(_element);\
	}

#define ADD_PROC_INFO_STATISTICS_ELEMENT(sCommandName, nProcId, sOwnerUser, nTotalMemUsage, nRealMemUsage, nVirtualMemUsage, nStartTime, nTotalTime, nUserTime, nSystemTime, nProcState, nPercents)\
	{\
		CProcInfoStatistics *_element = new CProcInfoStatistics;\
		_element->setCommandName(sCommandName);\
		_element->setProcId(nProcId);\
		_element->setOwnerUser(sOwnerUser);\
		_element->setTotalMemUsage(nTotalMemUsage);\
		_element->setRealMemUsage(nRealMemUsage);\
		_element->setVirtualMemUsage(nVirtualMemUsage);\
		_element->setStartTime(nStartTime);\
		_element->setTotalTime(nTotalTime);\
		_element->setUserTime(nUserTime);\
		_element->setSystemTime(nSystemTime);\
		_element->setState(nProcState);\
		_element->setPercentsUsage(nPercents);\
		_expected.m_lstProcessesStatistics.append(_element);\
	}

#define ADD_NET_IFACE_STATISTICS_ELEMENT(sIfaceName, nInDataSize, nOutDataSize, nInPkgsCount, nOutPkgsCount)\
	{\
		CNetIfaceStatistics *_element = new CNetIfaceStatistics;\
		_element->setIfaceSystemName(sIfaceName);\
		_element->setInDataSize(nInDataSize);\
		_element->setOutDataSize(nOutDataSize);\
		_element->setInPkgsCount(nInPkgsCount);\
		_element->setOutPkgsCount(nOutPkgsCount);\
		_expected.m_lstNetIfacesStatistics.append(_element);\
	}

#define ADD_USER_SESSION_STATISTICS_ELEMENT(sUserName, sServiceName, sHostName, nSessionDurationTime)\
	{\
		CUserStatistics *_element = new CUserStatistics;\
		_element->setUserName(sUserName);\
		_element->setServiceName(sServiceName);\
		_element->setHostName(sHostName);\
		_element->setSessionTime(nSessionDurationTime);\
		_expected.m_lstUsersStatistics.append(_element);\
	}

void CPrlDataSerializerTest::testSystemStatisticsSerializeDeserialize()
{
	PREPARE_STREAM

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)
	{
		CDiskStatistics *_element = new CDiskStatistics;
		_element->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
		_element->setFreeDiskSpace(30000000000ll);
		_element->setUsageDiskSpace(10000000000ll);

		CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
		pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
		pPartition1->setFreeDiskSpace(15000000000ll);
		pPartition1->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition1);

		CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
		pPartition2->setFreeDiskSpace(15000000000ll);
		pPartition2->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition2);
		_expected.m_lstDisksStatistics.append(_element);
	}
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc1", 1234, "user1", 2400000000ll, 1200000000ll, 1200000000ll, 4435787837383ll, 4000000000000ll, 3000000000000ll, 1000000000000ll, PPS_PROC_ZOMBIE, 38)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc2", 1235, "user3", 4800000000ll, 2400000000ll, 2400000000ll, 45787837383ll, 4000000000000ll, 1000000000000ll, 3000000000000ll, PPS_PROC_RUN, 12)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc3", 1236, "user1", 3600000000ll, 1800000000ll, 1800000000ll, 35787837383ll, 4000000000000ll, 2000000000000ll, 2000000000000ll, PPS_PROC_SLEEP, 5)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc4", 1237, "user2", 2000000000ll, 1000000000ll, 1000000000ll, 5787837383ll, 8000000000000ll, 5000000000000ll, 3000000000000ll, PPS_PROC_IDLE, 23)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth0", 2400000000ll, 1200000000ll, 120000ll, 4435787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth1", 3600000000ll, 2400000000ll, 240000ll, 3544787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth2", 4800000000ll, 3600000000ll, 270000ll, 7844357ll)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user1", "service1", "host1", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user2", "service1", "host2", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user3", "service1", "host3", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user4", "service1", "host4", 54)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user5", "service2", "host5", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user6", "service3", "host6", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user7", "service4", "host7", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user8", "service5", "host8", 5454)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSystemStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(_actual.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testSystemStatisticsSerializeDeserializeWithoutCpuStat()
{
	PREPARE_STREAM

	CSystemStatistics _expected;
	{
		CDiskStatistics *_element = new CDiskStatistics;
		_element->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
		_element->setFreeDiskSpace(30000000000ll);
		_element->setUsageDiskSpace(10000000000ll);

		CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
		pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
		pPartition1->setFreeDiskSpace(15000000000ll);
		pPartition1->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition1);

		CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
		pPartition2->setFreeDiskSpace(15000000000ll);
		pPartition2->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition2);
		_expected.m_lstDisksStatistics.append(_element);
	}
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc1", 1234, "user1", 2400000000ll, 1200000000ll, 1200000000ll, 4435787837383ll, 4000000000000ll, 3000000000000ll, 1000000000000ll, PPS_PROC_ZOMBIE, 38)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc2", 1235, "user3", 4800000000ll, 2400000000ll, 2400000000ll, 45787837383ll, 4000000000000ll, 1000000000000ll, 3000000000000ll, PPS_PROC_RUN, 12)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc3", 1236, "user1", 3600000000ll, 1800000000ll, 1800000000ll, 35787837383ll, 4000000000000ll, 2000000000000ll, 2000000000000ll, PPS_PROC_SLEEP, 5)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc4", 1237, "user2", 2000000000ll, 1000000000ll, 1000000000ll, 5787837383ll, 8000000000000ll, 5000000000000ll, 3000000000000ll, PPS_PROC_IDLE, 23)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth0", 2400000000ll, 1200000000ll, 120000ll, 4435787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth1", 3600000000ll, 2400000000ll, 240000ll, 3544787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth2", 4800000000ll, 3600000000ll, 270000ll, 7844357ll)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user1", "service1", "host1", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user2", "service1", "host2", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user3", "service1", "host3", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user4", "service1", "host4", 54)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user5", "service2", "host5", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user6", "service3", "host6", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user7", "service4", "host7", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user8", "service5", "host8", 5454)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSystemStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(_actual.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testSystemStatisticsSerializeDeserializeWithoutDiskStat()
{
	PREPARE_STREAM

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc1", 1234, "user1", 2400000000ll, 1200000000ll, 1200000000ll, 4435787837383ll, 4000000000000ll, 3000000000000ll, 1000000000000ll, PPS_PROC_ZOMBIE, 38)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc2", 1235, "user3", 4800000000ll, 2400000000ll, 2400000000ll, 45787837383ll, 4000000000000ll, 1000000000000ll, 3000000000000ll, PPS_PROC_RUN, 12)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc3", 1236, "user1", 3600000000ll, 1800000000ll, 1800000000ll, 35787837383ll, 4000000000000ll, 2000000000000ll, 2000000000000ll, PPS_PROC_SLEEP, 5)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc4", 1237, "user2", 2000000000ll, 1000000000ll, 1000000000ll, 5787837383ll, 8000000000000ll, 5000000000000ll, 3000000000000ll, PPS_PROC_IDLE, 23)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth0", 2400000000ll, 1200000000ll, 120000ll, 4435787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth1", 3600000000ll, 2400000000ll, 240000ll, 3544787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth2", 4800000000ll, 3600000000ll, 270000ll, 7844357ll)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user1", "service1", "host1", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user2", "service1", "host2", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user3", "service1", "host3", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user4", "service1", "host4", 54)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user5", "service2", "host5", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user6", "service3", "host6", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user7", "service4", "host7", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user8", "service5", "host8", 5454)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSystemStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(_actual.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testSystemStatisticsSerializeDeserializeWithoutProcsStat()
{
	PREPARE_STREAM

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)
	{
		CDiskStatistics *_element = new CDiskStatistics;
		_element->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
		_element->setFreeDiskSpace(30000000000ll);
		_element->setUsageDiskSpace(10000000000ll);

		CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
		pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
		pPartition1->setFreeDiskSpace(15000000000ll);
		pPartition1->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition1);

		CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
		pPartition2->setFreeDiskSpace(15000000000ll);
		pPartition2->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition2);
		_expected.m_lstDisksStatistics.append(_element);
	}
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth0", 2400000000ll, 1200000000ll, 120000ll, 4435787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth1", 3600000000ll, 2400000000ll, 240000ll, 3544787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth2", 4800000000ll, 3600000000ll, 270000ll, 7844357ll)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user1", "service1", "host1", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user2", "service1", "host2", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user3", "service1", "host3", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user4", "service1", "host4", 54)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user5", "service2", "host5", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user6", "service3", "host6", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user7", "service4", "host7", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user8", "service5", "host8", 5454)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSystemStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(_actual.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testSystemStatisticsSerializeDeserializeWithoutNetIfacesStat()
{
	PREPARE_STREAM

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)
	{
		CDiskStatistics *_element = new CDiskStatistics;
		_element->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
		_element->setFreeDiskSpace(30000000000ll);
		_element->setUsageDiskSpace(10000000000ll);

		CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
		pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
		pPartition1->setFreeDiskSpace(15000000000ll);
		pPartition1->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition1);

		CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
		pPartition2->setFreeDiskSpace(15000000000ll);
		pPartition2->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition2);
		_expected.m_lstDisksStatistics.append(_element);
	}
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc1", 1234, "user1", 2400000000ll, 1200000000ll, 1200000000ll, 4435787837383ll, 4000000000000ll, 3000000000000ll, 1000000000000ll, PPS_PROC_ZOMBIE, 38)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc2", 1235, "user3", 4800000000ll, 2400000000ll, 2400000000ll, 45787837383ll, 4000000000000ll, 1000000000000ll, 3000000000000ll, PPS_PROC_RUN, 12)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc3", 1236, "user1", 3600000000ll, 1800000000ll, 1800000000ll, 35787837383ll, 4000000000000ll, 2000000000000ll, 2000000000000ll, PPS_PROC_SLEEP, 5)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc4", 1237, "user2", 2000000000ll, 1000000000ll, 1000000000ll, 5787837383ll, 8000000000000ll, 5000000000000ll, 3000000000000ll, PPS_PROC_IDLE, 23)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user1", "service1", "host1", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user2", "service1", "host2", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user3", "service1", "host3", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user4", "service1", "host4", 54)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user5", "service2", "host5", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user6", "service3", "host6", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user7", "service4", "host7", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user8", "service5", "host8", 5454)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSystemStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(_actual.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testSystemStatisticsSerializeDeserializeWithoutUsersStat()
{
	PREPARE_STREAM

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)
	{
		CDiskStatistics *_element = new CDiskStatistics;
		_element->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
		_element->setFreeDiskSpace(30000000000ll);
		_element->setUsageDiskSpace(10000000000ll);

		CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
		pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
		pPartition1->setFreeDiskSpace(15000000000ll);
		pPartition1->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition1);

		CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
		pPartition2->setFreeDiskSpace(15000000000ll);
		pPartition2->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition2);
		_expected.m_lstDisksStatistics.append(_element);
	}
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc1", 1234, "user1", 2400000000ll, 1200000000ll, 1200000000ll, 4435787837383ll, 4000000000000ll, 3000000000000ll, 1000000000000ll, PPS_PROC_ZOMBIE, 38)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc2", 1235, "user3", 4800000000ll, 2400000000ll, 2400000000ll, 45787837383ll, 4000000000000ll, 1000000000000ll, 3000000000000ll, PPS_PROC_RUN, 12)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc3", 1236, "user1", 3600000000ll, 1800000000ll, 1800000000ll, 35787837383ll, 4000000000000ll, 2000000000000ll, 2000000000000ll, PPS_PROC_SLEEP, 5)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc4", 1237, "user2", 2000000000ll, 1000000000ll, 1000000000ll, 5787837383ll, 8000000000000ll, 5000000000000ll, 3000000000000ll, PPS_PROC_IDLE, 23)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth0", 2400000000ll, 1200000000ll, 120000ll, 4435787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth1", 3600000000ll, 2400000000ll, 240000ll, 3544787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth2", 4800000000ll, 3600000000ll, 270000ll, 7844357ll)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	_expected.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CSystemStatistics _actual;
	_actual.Deserialize(_data_stream);

	QCOMPARE(_actual.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testVmEventSerializeDeserialize()
{
	PREPARE_STREAM

	CVmEvent _expected_event(	PET_DSP_EVT_VM_STARTED, "{b6255764-49f6-4eca-8ce0-31026f13aab9}", PIE_VIRTUAL_MACHINE, PRL_ERR_ACCESS_DENIED,
						PVE::EventRespRequired, "какой-то источник события", PVE::EventLevel2);
	_expected_event.setInitRequestId("{b6255764-7afb-5eca-9ce0-41026f13aab9}");

	CVmEventParameter *pParam = new CVmEventParameter(PVE::String, UTF8_2QSTR("некое строковое значение"), "param1");
	_expected_event.m_lstEventParameters.append(pParam);

	pParam = new CVmEventParameter(PVE::CData, "", "param2");
	pParam->setCdata(UTF8_2QSTR("некоторые данные"));
	_expected_event.m_lstEventParameters.append(pParam);

	pParam = new CVmEventParameter(PVE::UnsignedInt, QString::number(UINT_MAX), "param3");
	_expected_event.m_lstEventParameters.append(pParam);

	QStringList _list;
	_list.append(UTF8_2QSTR("некоторое строковое значение1"));
	_list.append(UTF8_2QSTR("некоторое строковое значение2"));
	_list.append(UTF8_2QSTR("некоторое строковое значение3"));
	_list.append(UTF8_2QSTR("некоторое строковое значение4"));
	_list.append(UTF8_2QSTR("некоторое строковое значение5"));
	_list.append(UTF8_2QSTR("некоторое строковое значение6"));
	_list.append(UTF8_2QSTR("некоторое строковое значение7"));
	pParam = new CVmEventParameterList(PVE::String, _list, "param4");
	_expected_event.m_lstEventParameters.append(pParam);

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)

	CVmBinaryEventParameter *pBinaryEventParam = new CVmBinaryEventParameter("param5");
	_expected.Serialize(*pBinaryEventParam->getBinaryDataStream().getImpl());
	_expected_event.m_lstEventParameters.append(pBinaryEventParam);

	_expected_event.m_lstEventParameters.append(new CVmEventParameter(PVE::String, UTF8_2QSTR("некое строковое значение"), "param6"));
	_expected_event.m_lstEventParameters.append(new CVmBinaryEventParameter("param7"));
	_expected_event.m_lstEventParameters.append(new CVmEventParameter(PVE::String, UTF8_2QSTR("некое строковое значение"), "param8"));

	_expected_event.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CVmEvent _actual_event;
	_actual_event.Deserialize(_data_stream);

	QCOMPARE(_actual_event.toString(), _expected_event.toString());

	QVERIFY(_actual_event.m_lstEventParameters.size() == _expected_event.m_lstEventParameters.size());

	foreach(CVmEventParameter *pExpectedParam, _expected_event.m_lstEventParameters)
	{
		CVmEventParameter *pActualParam = _actual_event.getEventParameter(pExpectedParam->getParamName());
		QVERIFY(pActualParam);
		QCOMPARE(pActualParam->getParamValue(), pExpectedParam->getParamValue());
		QVERIFY(pActualParam->getEventParameterClassType() == pExpectedParam->getEventParameterClassType());
		QCOMPARE(pActualParam->getCdata(), pExpectedParam->getCdata());
		QVERIFY(pActualParam->getParamType() == pExpectedParam->getParamType());
	}

	pParam = _actual_event.getEventParameter("param5");
	QVERIFY(pParam);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(pParam);
	QVERIFY(pBinaryEventParam);

	CSystemStatistics _actual_stat;
	_actual_stat.Deserialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	QCOMPARE(_actual_stat.toString(), _expected.toString());

	pParam = _actual_event.getEventParameter("param7");
	QVERIFY(pParam);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(pParam);
	QVERIFY(pBinaryEventParam);
}

void CPrlDataSerializerTest::testVmEventCopyConstructor()
{
	CVmEvent _expected_event(	PET_DSP_EVT_VM_STARTED, "{b6255764-49f6-4eca-8ce0-31026f13aab9}", PIE_VIRTUAL_MACHINE, PRL_ERR_ACCESS_DENIED,
						PVE::EventRespRequired, "какой-то источник события", PVE::EventLevel2);
	_expected_event.setInitRequestId("{b6255764-7afb-5eca-9ce0-41026f13aab9}");

	CVmEventParameter *pParam = new CVmEventParameter(PVE::String, UTF8_2QSTR("некое строковое значение"), "param1");
	_expected_event.m_lstEventParameters.append(pParam);

	pParam = new CVmEventParameter(PVE::CData, "", "param2");
	pParam->setCdata(UTF8_2QSTR("некоторые данные"));
	_expected_event.m_lstEventParameters.append(pParam);

	pParam = new CVmEventParameter(PVE::UnsignedInt, QString::number(UINT_MAX), "param3");
	_expected_event.m_lstEventParameters.append(pParam);

	QStringList _list;
	_list.append(UTF8_2QSTR("некоторое строковое значение1"));
	_list.append(UTF8_2QSTR("некоторое строковое значение2"));
	_list.append(UTF8_2QSTR("некоторое строковое значение3"));
	_list.append(UTF8_2QSTR("некоторое строковое значение4"));
	_list.append(UTF8_2QSTR("некоторое строковое значение5"));
	_list.append(UTF8_2QSTR("некоторое строковое значение6"));
	_list.append(UTF8_2QSTR("некоторое строковое значение7"));
	pParam = new CVmEventParameterList(PVE::String, _list, "param4");
	_expected_event.m_lstEventParameters.append(pParam);

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)

	CVmBinaryEventParameter *pBinaryEventParam = new CVmBinaryEventParameter("param5");
	_expected.Serialize(*pBinaryEventParam->getBinaryDataStream().getImpl());
	_expected_event.m_lstEventParameters.append(pBinaryEventParam);

	_expected_event.m_lstEventParameters.append(new CVmEventParameter(PVE::String, UTF8_2QSTR("некое строковое значение"), "param6"));
	_expected_event.m_lstEventParameters.append(new CVmBinaryEventParameter("param7"));
	_expected_event.m_lstEventParameters.append(new CVmEventParameter(PVE::String, UTF8_2QSTR("некое строковое значение"), "param8"));

	CVmEvent _actual_event(&_expected_event);

	QCOMPARE(_actual_event.toString(), _expected_event.toString());

	QVERIFY(_actual_event.m_lstEventParameters.size() == _expected_event.m_lstEventParameters.size());

	foreach(CVmEventParameter *pExpectedParam, _expected_event.m_lstEventParameters)
	{
		CVmEventParameter *pActualParam = _actual_event.getEventParameter(pExpectedParam->getParamName());
		QVERIFY(pActualParam);
		QCOMPARE(pActualParam->getParamValue(), pExpectedParam->getParamValue());
		QVERIFY(pActualParam->getEventParameterClassType() == pExpectedParam->getEventParameterClassType());
		QCOMPARE(pActualParam->getCdata(), pExpectedParam->getCdata());
		QVERIFY(pActualParam->getParamType() == pExpectedParam->getParamType());
	}

	pParam = _actual_event.getEventParameter("param5");
	QVERIFY(pParam);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(pParam);
	QVERIFY(pBinaryEventParam);

	CSystemStatistics _actual_stat;
	_actual_stat.Deserialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	QCOMPARE(_actual_stat.toString(), _expected.toString());

	pParam = _actual_event.getEventParameter("param7");
	QVERIFY(pParam);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(pParam);
	QVERIFY(pBinaryEventParam);
}

void CPrlDataSerializerTest::testVmBinaryEventInteroperate()
{
	PREPARE_STREAM

	CVmEvent _expected_event(PET_DSP_EVT_VM_STARTED, "{b6255764-49f6-4eca-8ce0-31026f13aab9}", PIE_VIRTUAL_MACHINE, PRL_ERR_ACCESS_DENIED,
						PVE::EventRespRequired, "какой-то источник события", PVE::EventLevel2);
	_expected_event.setInitRequestId("{b6255764-7afb-5eca-9ce0-41026f13aab9}");

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)
	{
		CDiskStatistics *_element = new CDiskStatistics;
		_element->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя девайса"));
		_element->setFreeDiskSpace(30000000000ll);
		_element->setUsageDiskSpace(10000000000ll);

		CDiskPartStatistics *pPartition1 = new CDiskPartStatistics;
		pPartition1->setDeviceSystemName(UTF8_2QSTR("какое-то системное имя партиции"));
		pPartition1->setFreeDiskSpace(15000000000ll);
		pPartition1->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition1);

		CDiskPartStatistics *pPartition2 = new CDiskPartStatistics;
		pPartition2->setFreeDiskSpace(15000000000ll);
		pPartition2->setUsageDiskSpace(5000000000ll);
		_element->m_lstPartitionsStatistics.append(pPartition2);
		_expected.m_lstDisksStatistics.append(_element);
	}
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc1", 1234, "user1", 2400000000ll, 1200000000ll, 1200000000ll, 4435787837383ll, 4000000000000ll, 3000000000000ll, 1000000000000ll, PPS_PROC_ZOMBIE, 38)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc2", 1235, "user3", 4800000000ll, 2400000000ll, 2400000000ll, 45787837383ll, 4000000000000ll, 1000000000000ll, 3000000000000ll, PPS_PROC_RUN, 12)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc3", 1236, "user1", 3600000000ll, 1800000000ll, 1800000000ll, 35787837383ll, 4000000000000ll, 2000000000000ll, 2000000000000ll, PPS_PROC_SLEEP, 5)
	ADD_PROC_INFO_STATISTICS_ELEMENT("proc4", 1237, "user2", 2000000000ll, 1000000000ll, 1000000000ll, 5787837383ll, 8000000000000ll, 5000000000000ll, 3000000000000ll, PPS_PROC_IDLE, 23)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth0", 2400000000ll, 1200000000ll, 120000ll, 4435787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth1", 3600000000ll, 2400000000ll, 240000ll, 3544787ll)
	ADD_NET_IFACE_STATISTICS_ELEMENT("eth2", 4800000000ll, 3600000000ll, 270000ll, 7844357ll)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user1", "service1", "host1", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user2", "service1", "host2", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user3", "service1", "host3", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user4", "service1", "host4", 54)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user5", "service2", "host5", 454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user6", "service3", "host6", 5454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user7", "service4", "host7", 45454)
	ADD_USER_SESSION_STATISTICS_ELEMENT("user8", "service5", "host8", 5454)
	_expected.getMemoryStatistics()->setTotalSize(400000000000000ll);
	_expected.getMemoryStatistics()->setUsageSize(50000000000000ll);
	_expected.getMemoryStatistics()->setFreeSize(350000000000000ll);
	_expected.getMemoryStatistics()->setRealSize(300000000000000ll);
	_expected.getSwapStatistics()->setTotalSize(800000000000000ll);
	_expected.getSwapStatistics()->setUsageSize(400000000000000ll);
	_expected.getSwapStatistics()->setFreeSize(400000000000000ll);
	_expected.getUptimeStatistics()->setOsUptime(400000000000000ll);
	_expected.getUptimeStatistics()->setDispatcherUptime(40000);

	CVmBinaryEventParameter *pBinaryEventParam = new CVmBinaryEventParameter;
	_expected.Serialize(*pBinaryEventParam->getBinaryDataStream().getImpl());
	_expected_event.m_lstEventParameters.append(pBinaryEventParam);

	_expected_event.Serialize(_data_stream);

	QVERIFY(_buffer.seek(0));//Go to the start of buffer in order to retrieve necessary data from it now

	CVmEvent _actual_event;
	_actual_event.Deserialize(_data_stream);
	CSystemStatistics _actual_system_stat;
	QVERIFY(_actual_event.m_lstEventParameters.size() == 1);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(_actual_event.m_lstEventParameters.front());
	QVERIFY(pBinaryEventParam != NULL);
	_actual_system_stat.Deserialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	QCOMPARE(_actual_event.toString(), _expected_event.toString());
	QCOMPARE(_actual_system_stat.toString(), _expected.toString());
}

void CPrlDataSerializerTest::testVmBinaryEventInteroperateSeveralTimes()
{
	PREPARE_STREAM

	CVmEvent _expected_event(PET_DSP_EVT_VM_STARTED, "{b6255764-49f6-4eca-8ce0-31026f13aab9}", PIE_VIRTUAL_MACHINE, PRL_ERR_ACCESS_DENIED,
						PVE::EventRespRequired, "some event issuer", PVE::EventLevel2);
	_expected_event.setInitRequestId("{b6255764-7afb-5eca-9ce0-41026f13aab9}");

	CSystemStatistics _expected;
	ADD_CPU_STATISTICS_ELEMENT(56, 100000000000ll, 40000000000ll, 60000000000ll)
	ADD_CPU_STATISTICS_ELEMENT(12, 80000000000ll, 20000000000ll, 60000000000ll)

	CVmBinaryEventParameter *pBinaryEventParam = new CVmBinaryEventParameter;
	_expected.Serialize(*pBinaryEventParam->getBinaryDataStream().getImpl());
	_expected_event.m_lstEventParameters.append(pBinaryEventParam);

	_expected_event.Serialize(_data_stream);

	QVERIFY(_buffer.reset());//Go to the start of buffer in order to retrieve necessary data from it now

	CVmEvent _actual_event;
	_actual_event.Deserialize(_data_stream);

	QCOMPARE(_actual_event.toString(), _expected_event.toString());

	CSystemStatistics _actual_system_stat;
	QVERIFY(_actual_event.m_lstEventParameters.size() == 1);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(_actual_event.m_lstEventParameters.front());
	QVERIFY(pBinaryEventParam != NULL);
	_actual_system_stat.Deserialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	QCOMPARE(_actual_system_stat.toString(), _expected.toString());

	CSystemStatistics _actual_system_stat2;
	_actual_system_stat2.Deserialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	QCOMPARE(_actual_system_stat2.toString(), _expected.toString());

	//Check event copy constructor
	CVmEvent _event_copy(&_expected_event);
	QCOMPARE(_event_copy.toString(), _expected_event.toString());

	CSystemStatistics _actual_system_stat3;
	QVERIFY(_event_copy.m_lstEventParameters.size() == 1);
	pBinaryEventParam = dynamic_cast<CVmBinaryEventParameter *>(_event_copy.m_lstEventParameters.front());
	QVERIFY(pBinaryEventParam != NULL);
	_actual_system_stat3.Deserialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	QCOMPARE(_actual_system_stat.toString(), _expected.toString());
}

QTEST_MAIN(CPrlDataSerializerTest)

