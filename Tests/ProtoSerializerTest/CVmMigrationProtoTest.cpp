/////////////////////////////////////////////////////////////////////////////
///
///	@file CVmMigrationProtoTest.cpp
///
///	Tests fixture class for testing VM migration protocol serializer.
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

#include "CVmMigrationProtoTest.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include <prlcommon/PrlUuid/Uuid.h>
#include <prlsdk/PrlEnums.h>
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameterList.h"
#include "Tests/CommonTestsUtils.h"

using namespace Parallels;

#define CHECK_PRECONDS_CMD_PARAMS_DECLARE\
	QString sVmConfiguration = Uuid::createUuid().toString();\
	QString sHostHardwareInfo = Uuid::createUuid().toString();\
	QString sTargetVmName = Uuid::createUuid().toString();\
	QString sTargetVmHomePath = Uuid::createUuid().toString();\
	QString sSharedFileName = Uuid::createUuid().toString();\
	QStringList lstSharedFileNamesExtra = (QStringList() << \
			Uuid::createUuid().toString() << Uuid::createUuid().toString());\
	QString sStorageInfo = Uuid::createUuid().toString();\
	quint32 nMigrationFlags = PVMT_WARM_MIGRATION | PVMSL_NORMAL_SECURITY;\
	Q_UNUSED(nMigrationFlags);\
	quint32 nReservedFlags = 0;\
	Q_UNUSED(nReservedFlags);\
	VIRTUAL_MACHINE_STATE nPrevVmState = VMS_UNKNOWN;\
	Q_UNUSED(nPrevVmState);\

void CVmMigrationProtoTest::testCreateVmMigrateCheckPreconditionsCommand()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsCommand(
			sVmConfiguration,
			sHostHardwareInfo,
			sTargetVmName,
			sTargetVmHomePath,
			sSharedFileName,
			lstSharedFileNamesExtra,
			sStorageInfo,
			0,
			nMigrationFlags,
			nReservedFlags,
			nPrevVmState
		);
	QVERIFY(pCmd->IsValid());
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_PROTO_VERSION, PVE::UnsignedInt,\
		QString("%1").arg(MIGRATE_DISP_PROTO_VERSION))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_VM_CONFIG, PVE::String,\
		sVmConfiguration)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO, PVE::String,\
		sHostHardwareInfo)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME, PVE::String,\
		sTargetVmName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH, PVE::String,\
		sTargetVmHomePath)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt,\
                QString("%1").arg(nMigrationFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME, PVE::String, \
		sSharedFileName)
	CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA);
	QVERIFY(pParam != NULL);
	QVERIFY(pParam->isList());
	QCOMPARE(lstSharedFileNamesExtra, pParam->getValuesList());
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_STORAGE_INFO, PVE::String,\
		sStorageInfo)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS, PVE::UnsignedInt,\
		QString("%1").arg(nReservedFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE, PVE::UnsignedInt,\
		QString("%1").arg(nPrevVmState))
}

void CVmMigrationProtoTest::testParseVmMigrateCheckPreconditionsCommand()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(
									VmMigrateCheckPreconditionsCmd,
									_pkg->toString()
								);
	QVERIFY(pCmd->IsValid());
	CVmMigrateCheckPreconditionsCommand *pVmMigrateCheckPreconditionsCmd =
				CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);
	QCOMPARE((quint32)MIGRATE_DISP_PROTO_VERSION, pVmMigrateCheckPreconditionsCmd->GetVersion());
	QCOMPARE(sVmConfiguration, pVmMigrateCheckPreconditionsCmd->GetVmConfig());
	QCOMPARE(sHostHardwareInfo, pVmMigrateCheckPreconditionsCmd->GetSourceHostHardwareInfo());
	QCOMPARE(sTargetVmName, pVmMigrateCheckPreconditionsCmd->GetTargetVmName());
	QCOMPARE(sTargetVmHomePath, pVmMigrateCheckPreconditionsCmd->GetTargetVmHomePath());
	QCOMPARE(sSharedFileName, pVmMigrateCheckPreconditionsCmd->GetSharedFileName());
	QCOMPARE(lstSharedFileNamesExtra, pVmMigrateCheckPreconditionsCmd->GetSharedFileNamesExtra());
	QCOMPARE(sStorageInfo, pVmMigrateCheckPreconditionsCmd->GetStorageInfo());
	QCOMPARE((quint32)nMigrationFlags, (quint32)pVmMigrateCheckPreconditionsCmd->GetMigrationFlags());
	QCOMPARE((quint32)nReservedFlags, (quint32)pVmMigrateCheckPreconditionsCmd->GetReservedFlags());
	QCOMPARE(nPrevVmState, pVmMigrateCheckPreconditionsCmd->GetVmPrevState());
}

#define CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CVmMigrateCheckPreconditionsCommand *pVmMigrateCheckPreconditionsCmd =\
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);\
	pVmMigrateCheckPreconditionsCmd->GetVersion();\
	pVmMigrateCheckPreconditionsCmd->GetVmConfig();\
	pVmMigrateCheckPreconditionsCmd->GetSourceHostHardwareInfo();\
	pVmMigrateCheckPreconditionsCmd->GetTargetVmName();\
	pVmMigrateCheckPreconditionsCmd->GetTargetVmHomePath();\
	pVmMigrateCheckPreconditionsCmd->GetSharedFileName();\
	pVmMigrateCheckPreconditionsCmd->GetStorageInfo();\
	pVmMigrateCheckPreconditionsCmd->GetMigrationFlags();\
	pVmMigrateCheckPreconditionsCmd->GetReservedFlags();\
	pVmMigrateCheckPreconditionsCmd->GetVmPrevState();\

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnEmptyPackage()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnVmConfigurationAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnVersionInfoAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnHardwareHostInfoAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnTargetVmHomePathAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

/*
The migration flag use EVT_PARAM_PROTO_CMD_FLAGS.
CProtoCommandBase::CProtoCommandBase() set this field as 0 by default.
Will skip this check.
void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnVmMigrateFlagsAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}
*/

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnReservedFlagsAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidFailedOnSharedFileNameAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidNotFailedOnStorageInfoAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidNotFailedOnPrevVmStateAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsCommandIsValidNotFailedOnTargetVmNameAbsent()
{
	CHECK_PRECONDS_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sHostHardwareInfo,
							EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSharedFileName,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSharedFileNamesExtra,
							EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sStorageInfo,
							EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsCmd, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define START_CMD_PARAMS_DECLARE\
	QString sVmConfiguration = Uuid::createUuid().toString();\
	QString sVmRuntimeConfiguration = Uuid::createUuid().toString();\
	QString sTargetVmHomePath = Uuid::createUuid().toString();\
	QString sSnapshotUuid = Uuid::createUuid().toString();\
	quint32 nBundlePermissions = 0;\
	Q_UNUSED(nBundlePermissions);\
	quint32 nConfigPermissions = 0;\
	Q_UNUSED(nConfigPermissions);\
	quint32 nMigrationFlags = PVMT_WARM_MIGRATION | PVMSL_NORMAL_SECURITY;\
	Q_UNUSED(nMigrationFlags);\
	quint32 nReservedFlags = 0;\
	Q_UNUSED(nReservedFlags);\
	VIRTUAL_MACHINE_STATE nPrevVmState = VMS_RUNNING;\
	Q_UNUSED(nPrevVmState);

void CVmMigrationProtoTest::testCreateVmMigrateStartCommand()
{
	START_CMD_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateVmMigrateStartCommand(
			sVmConfiguration,
			sVmRuntimeConfiguration,
			sTargetVmHomePath,
			sSnapshotUuid,
			nBundlePermissions,
			nConfigPermissions,
			nMigrationFlags,
			nReservedFlags,
			nPrevVmState
		);
	QVERIFY(pCmd->IsValid());
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_PROTO_VERSION, PVE::UnsignedInt,\
		QString("%1").arg(MIGRATE_DISP_PROTO_VERSION))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_VM_CONFIG, PVE::String,\
		sVmConfiguration)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_VM_RUNTIME_CONFIG, PVE::String,\
		sVmRuntimeConfiguration)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH, PVE::String,\
		sTargetVmHomePath)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID, PVE::String,\
		sSnapshotUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_BUNDLE_PERMISSIONS, PVE::UnsignedInt,\
		QString("%1").arg(nBundlePermissions))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_CONFIG_PERMISSIONS, PVE::UnsignedInt,\
		QString("%1").arg(nConfigPermissions))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt,\
		QString("%1").arg(nMigrationFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS, PVE::UnsignedInt,\
		QString("%1").arg(nReservedFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE, PVE::UnsignedInt,\
		QString("%1").arg(nPrevVmState))
}

void CVmMigrationProtoTest::testParseVmMigrateStartCommand()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmRuntimeConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_RUNTIME_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nBundlePermissions),
							EVT_PARAM_MIGRATE_CMD_BUNDLE_PERMISSIONS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nConfigPermissions),
							EVT_PARAM_MIGRATE_CMD_CONFIG_PERMISSIONS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(
									VmMigrateStartCmd,
									_pkg->toString()
								);
	QVERIFY(pCmd->IsValid());
	CVmMigrateStartCommand *pVmMigrateStartCmd =
				CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);
	QCOMPARE((quint32)MIGRATE_DISP_PROTO_VERSION, pVmMigrateStartCmd->GetVersion());
	QCOMPARE(sVmConfiguration, pVmMigrateStartCmd->GetVmConfig());
	QCOMPARE(sVmRuntimeConfiguration, pVmMigrateStartCmd->GetVmRuntimeConfig());
	QCOMPARE(sTargetVmHomePath, pVmMigrateStartCmd->GetTargetVmHomePath());
	QCOMPARE((quint32)nBundlePermissions, (quint32)pVmMigrateStartCmd->GetBundlePermissions());
	QCOMPARE((quint32)nConfigPermissions, (quint32)pVmMigrateStartCmd->GetConfigPermissions());
	QCOMPARE((quint32)nMigrationFlags, (quint32)pVmMigrateStartCmd->GetMigrationFlags());
	QCOMPARE((quint32)nReservedFlags, (quint32)pVmMigrateStartCmd->GetReservedFlags());
	QCOMPARE((quint32)nPrevVmState, (quint32)pVmMigrateStartCmd->GetVmPrevState());
}

#define CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CVmMigrateStartCommand *pVmMigrateStartCmd =\
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);\
	pVmMigrateStartCmd->GetVersion();\
	pVmMigrateStartCmd->GetVmConfig();\
	pVmMigrateStartCmd->GetTargetVmHomePath();\
	pVmMigrateStartCmd->GetSnapshotUuid();\
	pVmMigrateStartCmd->GetBundlePermissions();\
	pVmMigrateStartCmd->GetConfigPermissions();\
	pVmMigrateStartCmd->GetMigrationFlags();\
	pVmMigrateStartCmd->GetReservedFlags();\
	pVmMigrateStartCmd->GetVmPrevState();

void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnEmptyPackage()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnVersionAbsent()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, QString("%1").arg(sTargetVmHomePath),
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnVmConfigurationAbsent()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnTargetServerVmHomePathAbsent()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

/*
The migration flag use EVT_PARAM_PROTO_CMD_FLAGS.
CProtoCommandBase::CProtoCommandBase() set this field as 0 by default.
Will skip this check.
void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnMigrationFlagsAbsent()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, QString("%1").arg(sTargetVmHomePath),
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}
*/

void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnReservedFlagsAbsent()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nPrevVmState),
							EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateStartCommandIsValidFailedOnPrevVmStateAbsent()
{
	START_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfiguration,
							EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
							EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSnapshotUuid,
							EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
							EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateStartCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_START_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}


#define CHECK_PRECONDS_REPLY_PARAMS_DECLARE\
	QStringList sCheckResult = (QStringList() << Uuid::createUuid().toString() << Uuid::createUuid().toString());\
	QStringList sNonShared = (QStringList() << Uuid::createUuid().toString() << Uuid::createUuid().toString());\
	quint32 nFlags = 0;\
	Q_UNUSED(nFlags);\

void CVmMigrationProtoTest::testCreateVmMigrateCheckPreconditionsReply()
{
	CHECK_PRECONDS_REPLY_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsReply(sCheckResult, sNonShared, nFlags);
	QVERIFY(pCmd->IsValid());
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_PROTO_VERSION, PVE::UnsignedInt,\
		QString("%1").arg(MIGRATE_DISP_PROTO_VERSION))
	CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_RESULT);
	QVERIFY(pParam != NULL);
	QVERIFY(pParam->isList());
	QCOMPARE(sCheckResult, pParam->getValuesList());
	pParam = pEvent->getEventParameter(EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_NON_SHARED);
	QVERIFY(pParam != NULL);
	QVERIFY(pParam->isList());
	QCOMPARE(sNonShared, pParam->getValuesList());
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt, QString("%1").arg(nFlags))
}

void CVmMigrationProtoTest::testParseVmMigrateCheckPreconditionsReply()
{
	CHECK_PRECONDS_REPLY_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, sCheckResult,
							EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_RESULT));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, sNonShared,
							EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_NON_SHARED));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsReply, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CVmMigrateCheckPreconditionsReply *pReply =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsReply>(pCmd);
	QCOMPARE((quint32)MIGRATE_DISP_PROTO_VERSION, pReply->GetVersion());
	QCOMPARE(sCheckResult, pReply->GetCheckPreconditionsResult());
	QCOMPARE(sNonShared, pReply->GetNonSharedDisks());
	QCOMPARE(nFlags, pReply->GetCommandFlags());
}

#define CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_REPLY_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CVmMigrateCheckPreconditionsReply *pReply =\
			CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsReply>(pCmd);\
	pReply->GetVersion();\
	pReply->GetCheckPreconditionsResult();\
	pReply->GetCommandFlags();\

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsReplyIsValidFailedOnEmptyPackage()
{
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsReply, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_REPLY_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsReplyIsValidFailedOnVersionAbsent()
{
	CHECK_PRECONDS_REPLY_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, sCheckResult,
							EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_RESULT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, sNonShared,
							EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_NON_SHARED));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsReply, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_REPLY_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CVmMigrationProtoTest::testVmMigrateCheckPreconditionsReplyIsValidFailedOnCheckPreconditionsResultAbsent()
{
	CHECK_PRECONDS_REPLY_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
							EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, sNonShared,
							EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_NON_SHARED));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateCheckPreconditionsReply, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_MIGRATE_CHECK_PRECONDITIONS_REPLY_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

/*
will skip CVmMigrationProtoTest::testVmMigrateCheckPreconditionsReplyIsValidNotFailedOnFlagsAbsent() :
The migration flag use EVT_PARAM_PROTO_CMD_FLAGS.
CProtoCommandBase::CProtoCommandBase() set this field as 0 by default.
*/

void CVmMigrationProtoTest::testCreateVmMigrateReply()
{
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::CreateVmMigrateReply(QString());
	QVERIFY(pCmd->IsValid());
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_PROTO_VERSION, PVE::UnsignedInt,\
		QString("%1").arg(MIGRATE_DISP_PROTO_VERSION))
}

void CVmMigrationProtoTest::testParseVmMigrateReply()
{
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(MIGRATE_DISP_PROTO_VERSION),
							EVT_PARAM_MIGRATE_PROTO_VERSION));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateReply, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CVmMigrateReply *pReply =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateReply>(pCmd);
	QCOMPARE((quint32)MIGRATE_DISP_PROTO_VERSION, pReply->GetVersion());
}

void CVmMigrationProtoTest::testVmMigrateReplyIsValidFailedOnEmptyPackage()
{
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(VmMigrateReply, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CVmMigrateReply *pReply =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateReply>(pCmd);
	pReply->GetVersion();
}
