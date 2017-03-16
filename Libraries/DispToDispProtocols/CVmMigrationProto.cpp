/////////////////////////////////////////////////////////////////////////////
///
///	@file CVmMigrationProto.cpp
///
///	Implementation of VM migration protocol commands serializer helpers.
///
///	@author sandro
/// @owner sergeym@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////

#include "CVmMigrationProto.h"

namespace Parallels {

//**************************************************************************
CVmMigrateProto::CVmMigrateProto(Parallels::IDispToDispCommands nCmdIdentifier, quint32 nFlags)
:CDispToDispCommand(nCmdIdentifier, false, nFlags)
{
	SetUnsignedIntParamValue(MIGRATE_DISP_PROTO_VERSION, EVT_PARAM_MIGRATE_PROTO_VERSION);
}

bool CVmMigrateProto::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_MIGRATE_PROTO_VERSION, PVE::UnsignedInt) &&
		CheckWhetherParamPresents(EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt));
}

quint32 CVmMigrateProto::GetVersion()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_MIGRATE_PROTO_VERSION));
}

//*************************** Base migrate command *************************
CVmMigrateCommand::CVmMigrateCommand(
	Parallels::IDispToDispCommands nCmdIdentifier,
	const QString &sVmConfiguration,
	const QString &sTargetVmHomePath,
	quint32 nMigrationFlags,
	quint32 nReservedFlags,
	VIRTUAL_MACHINE_STATE nPrevVmState
)
:CVmMigrateProto(nCmdIdentifier, nMigrationFlags)
{
	SetStringParamValue(sVmConfiguration, EVT_PARAM_MIGRATE_CMD_VM_CONFIG);
	SetStringParamValue(sTargetVmHomePath, EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH);
	SetUnsignedIntParamValue(nReservedFlags, EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS);
	SetUnsignedIntParamValue(nPrevVmState, EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE);
}

bool CVmMigrateCommand::IsValid()
{
	return (CVmMigrateProto::IsValid() &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CMD_VM_CONFIG, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS, PVE::UnsignedInt));
/*
	Will not check EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE here : this field is absent in
	CVmMigrateCheckPreconditionsCommand in PSBM-4 & PSfM-4
	For CVmMigrateStartCommand will check in CVmMigrateStartCommand::IsValid()
*/
}

QString CVmMigrateCommand::GetVmConfig()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_VM_CONFIG));
}

QString CVmMigrateCommand::GetTargetVmHomePath()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
}

quint32 CVmMigrateCommand::GetMigrationFlags()
{
	return GetCommandFlags();
}

quint32 CVmMigrateCommand::GetReservedFlags()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
}

VIRTUAL_MACHINE_STATE CVmMigrateCommand::GetVmPrevState()
{
	return (VIRTUAL_MACHINE_STATE(GetUnsignedIntParamValue(EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE)));
}

//**********************************************Check preconditions command implementation*****************************

CVmMigrateCheckPreconditionsCommand::CVmMigrateCheckPreconditionsCommand(
	const QString &sVmConfiguration,
	const QString &sHostHardwareInfo,
	const QString &sTargetVmName,
	const QString &sTargetVmHomePath,
	const QString &sSharedFileName,
	const QStringList &lstSharedFileNamesExtra,
	const QString &sStorageInfo,
	PRL_UINT64 nRequiresDiskSpace,
	quint32 nMigrationFlags,
	quint32 nReservedFlags,
	VIRTUAL_MACHINE_STATE nPrevVmState
)
:CVmMigrateCommand(
	VmMigrateCheckPreconditionsCmd,
	sVmConfiguration,
	sTargetVmHomePath,
	nMigrationFlags,
	nReservedFlags,
	nPrevVmState)
{
	SetStringParamValue(sSharedFileName, EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME);
	SetStringParamValue(sHostHardwareInfo, EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO);
	SetStringParamValue(sTargetVmName, EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME);
	SetStringParamValue(sStorageInfo, EVT_PARAM_MIGRATE_CMD_STORAGE_INFO);
	SetUnsignedInt64ParamValue(nRequiresDiskSpace, EVT_PARAM_MIGRATE_REQUIRES_DISK_SPACE);
	SetStringListParamValue(lstSharedFileNamesExtra, EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA);
}

bool CVmMigrateCheckPreconditionsCommand::IsValid()
{
	// EVT_PARAM_MIGRATE_REQUIRES_DISK_SPACE is not mandatory field
	// EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA is not mandatory field
	// EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME is not mandatory field
	// to keep compatibility
	return (CVmMigrateCommand::IsValid() &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME, PVE::String));
}

QString CVmMigrateCheckPreconditionsCommand::GetSourceHostHardwareInfo()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_HOST_HARDWARE_INFO));
}

QString CVmMigrateCheckPreconditionsCommand::GetTargetVmName()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
}

QString CVmMigrateCheckPreconditionsCommand::GetSharedFileName()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAME));
}

QStringList CVmMigrateCheckPreconditionsCommand::GetSharedFileNamesExtra()
{
	return (GetStringListParamValue(EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));
}

QString CVmMigrateCheckPreconditionsCommand::GetStorageInfo()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_STORAGE_INFO));
}

PRL_UINT64 CVmMigrateCheckPreconditionsCommand::GetRequiresDiskSpace()
{
	return (GetUnsignedInt64ParamValue(EVT_PARAM_MIGRATE_REQUIRES_DISK_SPACE));
}


//**********************************************Check preconditions reply implementation*****************************
CVmMigrateCheckPreconditionsReply::CVmMigrateCheckPreconditionsReply(
	const QStringList &sCheckPreconditionsResult,
	const QStringList &lstNonSharedDisks,
	quint32 nFlags
)
:CVmMigrateProto(VmMigrateCheckPreconditionsReply, nFlags)
{
	SetStringListParamValue(sCheckPreconditionsResult, EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_RESULT);
	SetStringListParamValue(lstNonSharedDisks, EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_NON_SHARED);
}

bool CVmMigrateCheckPreconditionsReply::IsValid()
{
	bool res = (CVmMigrateProto::IsValid() &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_RESULT, PVE::String));
	return res;
}

QStringList CVmMigrateCheckPreconditionsReply::GetCheckPreconditionsResult()
{
	return (GetStringListParamValue(EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_RESULT));
}

QStringList CVmMigrateCheckPreconditionsReply::GetNonSharedDisks()
{
	return (GetStringListParamValue(EVT_PARAM_MIGRATE_CHECK_PRECONDITIONS_NON_SHARED));
}

QString CVmMigrateCheckPreconditionsReply::GetConfig()
{
	return GetStringParamValue("migrate_check_preconditions_config");
}

void CVmMigrateCheckPreconditionsReply::SetConfig(const QString& config)
{
	SetStringParamValue(config, "migrate_check_preconditions_config");
}

//**********************************************Start migration command implementation*********************************

CVmMigrateStartCommand::CVmMigrateStartCommand(
	const QString &sVmConfiguration,
	const QString &sVmRuntimeConfiguration,
	const QString &sTargetVmHomePath,
	const QString &sSnapshotUuid,
	quint32 nBundlePermissions,
	quint32 nConfigPermissions,
	quint32 nMigrationFlags,
	quint32 nReservedFlags,
	VIRTUAL_MACHINE_STATE nPrevVmState
)
:CVmMigrateCommand(
	VmMigrateStartCmd,
	sVmConfiguration,
	sTargetVmHomePath,
	nMigrationFlags,
	nReservedFlags,
	nPrevVmState)
{
	SetStringParamValue(sVmRuntimeConfiguration, EVT_PARAM_MIGRATE_CMD_VM_RUNTIME_CONFIG);
	/* init network config by empty string, real config will added on dst dispatcher */
	SetStringParamValue("", EVT_PARAM_MIGRATE_CMD_NETWORK_CONFIG);
	/* init dispatcher config */
	SetStringParamValue("", EVT_PARAM_MIGRATE_CMD_DISPATCHER_CONFIG);
	SetStringParamValue( sSnapshotUuid, EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID);
	SetUnsignedIntParamValue(nBundlePermissions, EVT_PARAM_MIGRATE_CMD_BUNDLE_PERMISSIONS);
	SetUnsignedIntParamValue(nConfigPermissions, EVT_PARAM_MIGRATE_CMD_CONFIG_PERMISSIONS);
}

bool CVmMigrateStartCommand::IsValid()
{
	return (CVmMigrateCommand::IsValid() &&
		CheckWhetherParamPresents(EVT_PARAM_MIGRATE_CMD_VM_PREV_STATE, PVE::UnsignedInt));
}

QString CVmMigrateStartCommand::GetVmRuntimeConfig()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_VM_RUNTIME_CONFIG));
}

QString CVmMigrateStartCommand::GetNetworkConfig()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_NETWORK_CONFIG));
}

QString CVmMigrateStartCommand::GetDispatcherConfig()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_DISPATCHER_CONFIG));
}

QString CVmMigrateStartCommand::GetSnapshotUuid()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_CMD_SNAPSHOT_UUID));
}

quint32 CVmMigrateStartCommand::GetBundlePermissions()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_MIGRATE_CMD_BUNDLE_PERMISSIONS));
}

quint32 CVmMigrateStartCommand::GetConfigPermissions()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_MIGRATE_CMD_CONFIG_PERMISSIONS));
}

//**********************************************Start migration reply implementation*********************************
CVmMigrateReply::CVmMigrateReply(
	const QString &sMemFilePath
)
:CVmMigrateProto(VmMigrateReply)
{
	SetStringParamValue(sMemFilePath, EVT_PARAM_MIGRATE_MEMORY_FILE_PATH);
}

/* will use base IsValid() function since EVT_PARAM_MIGRATE_MEMORY_FILE_PATH field may be absent */

QString CVmMigrateReply::GetMemFilePath()
{
	return (GetStringParamValue(EVT_PARAM_MIGRATE_MEMORY_FILE_PATH));
}

}//namespace Parallels
