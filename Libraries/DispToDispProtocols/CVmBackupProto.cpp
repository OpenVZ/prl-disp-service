/////////////////////////////////////////////////////////////////////////////
///
///	@file CVmMigrationProto.cpp
///
///	Implementation of VM backup protocol commands serializer helpers.
///
///
/// Copyright (c) 2009-2017, Parallels International GmbH
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

#include "CVmBackupProto.h"

namespace Parallels {

CVmBackupProto::CVmBackupProto(
	Parallels::IDispToDispCommands nCmdIdentifier,
	quint32 nFlags,
	quint32 nInternalFlags,
	quint32 nVersion)
: CDispToDispCommand(nCmdIdentifier, false, nFlags)
{
	SetUnsignedIntParamValue(nVersion, EVT_PARAM_BACKUP_PROTO_VERSION);
	SetUnsignedIntParamValue(nInternalFlags, EVT_PARAM_BACKUP_CMD_INTERNAL_FLAGS);
}

quint32 CVmBackupProto::GetVersion()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_BACKUP_PROTO_VERSION));
}

void CVmBackupProto::setVersion(quint32 version)
{
	SetUnsignedIntParamValue(version, EVT_PARAM_BACKUP_PROTO_VERSION);
}

quint32 CVmBackupProto::GetInternalFlags()
{
	return GetUnsignedIntParamValue(EVT_PARAM_BACKUP_CMD_INTERNAL_FLAGS);
}

bool CVmBackupProto::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_PROTO_VERSION, PVE::UnsignedInt));
}

CVmBackupCommand::CVmBackupCommand(
	Parallels::IDispToDispCommands nCmdIdentifier,
	const QString &sVmUuid,
	quint32 nFlags,
	quint32 nInternalFlags,
	quint32 nVersion)
:CVmBackupProto(nCmdIdentifier, nFlags, nInternalFlags, nVersion)
{
	SetStringParamValue(sVmUuid, EVT_PARAM_BACKUP_CMD_VM_UUID);
}

QString CVmBackupCommand::GetVmUuid()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_VM_UUID));
}

quint32 CVmBackupCommand::GetFlags()
{
	return GetCommandFlags();
}

bool CVmBackupCommand::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_VM_UUID, PVE::String) &&
		CVmBackupProto::IsValid());
}

//**********************************************Create backup command implementation*********************************
CVmBackupCreateCommand::CVmBackupCreateCommand(
	const QString &sVmUuid,
	const QString &sVmName,
	const QString &sHost,
	const QString &sServerUuid,
	const QString &sDescription,
	const QString &sVmConfig,
	quint64 nOriginalSize,
	quint32 nBundlePermissions,
	const QStringList &lstBitmaps,
	quint32 nFlags,
	quint32 nInternalFlags
)
: CVmBackupCommand(VmBackupCreateCmd, sVmUuid, nFlags, nInternalFlags)
{
	SetStringParamValue(sVmName, EVT_PARAM_BACKUP_CMD_VM_NAME);
	SetStringParamValue(sHost, EVT_PARAM_BACKUP_CMD_HOST);
	SetStringParamValue(sServerUuid, EVT_PARAM_BACKUP_CMD_SERVER_UUID);
	SetStringParamValue(sDescription, EVT_PARAM_BACKUP_CMD_DESCRIPTION);
	SetStringParamValue(sVmConfig, EVT_PARAM_BACKUP_CMD_VM_CONFIG);
	SetUnsignedInt64ParamValue(nOriginalSize, EVT_PARAM_BACKUP_CMD_ORIGINAL_SIZE);
	SetUnsignedIntParamValue(nBundlePermissions, EVT_PARAM_BACKUP_CMD_BUNDLE_PERMISSIONS);
	SetStringListParamValue(lstBitmaps, EVT_PARAM_BACKUP_CMD_BITMAPS);
}

CVmBackupCreateCommand::CVmBackupCreateCommand(
	Parallels::IDispToDispCommands nCmdIdentifier,
	const QString &sVmUuid,
	const QString &sVmName,
	const QString &sHost,
	const QString &sServerUuid,
	const QString &sDescription,
	const QString &sVmConfig,
	quint64 nOriginalSize,
	quint32 nFlags,
	quint32 nInternalFlags
)
: CVmBackupCommand(nCmdIdentifier, sVmUuid, nFlags, nInternalFlags)
{
	SetStringParamValue(sVmName, EVT_PARAM_BACKUP_CMD_VM_NAME);
	SetStringParamValue(sHost, EVT_PARAM_BACKUP_CMD_HOST);
	SetStringParamValue(sServerUuid, EVT_PARAM_BACKUP_CMD_SERVER_UUID);
	SetStringParamValue(sDescription, EVT_PARAM_BACKUP_CMD_DESCRIPTION);
	SetStringParamValue(sVmConfig, EVT_PARAM_BACKUP_CMD_VM_CONFIG);
	SetUnsignedInt64ParamValue(nOriginalSize, EVT_PARAM_BACKUP_CMD_ORIGINAL_SIZE);
}

bool CVmBackupCreateCommand::IsValid()
{
	/* will skip EVT_PARAM_BACKUP_CMD_DESCRIPTION field check
	   : a) for Beta compatibility, b) this field is not a mandatory */
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_VM_NAME, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_HOST, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_SERVER_UUID, PVE::String) &&
		CVmBackupCommand::IsValid());
}

QString CVmBackupCreateCommand::GetVmName()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_VM_NAME));
}

QString CVmBackupCreateCommand::GetHost()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_HOST));
}

QString CVmBackupCreateCommand::GetServerUuid()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_SERVER_UUID));
}

QString CVmBackupCreateCommand::GetDescription()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_DESCRIPTION));
}

quint64 CVmBackupCreateCommand::GetOriginalSize()
{
	return (GetUnsignedInt64ParamValue(EVT_PARAM_BACKUP_CMD_ORIGINAL_SIZE));
}

quint32 CVmBackupCreateCommand::GetBundlePermissions()
{
	return (GetUnsignedInt64ParamValue(EVT_PARAM_BACKUP_CMD_BUNDLE_PERMISSIONS));
}

QStringList CVmBackupCreateCommand::GetBitmaps()
{
	return (GetStringListParamValue(EVT_PARAM_BACKUP_CMD_BITMAPS));
}

QString CVmBackupCreateCommand::GetVmConfig()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_VM_CONFIG));
}

//**********************************************Create Local backup command implementation*********************************
CVmBackupCreateLocalCommand::CVmBackupCreateLocalCommand(
	const QString &sVmUuid,
	const QString &sVmName,
	const QString &sHost,
	const QString &sServerUuid,
	const QString &sDescription,
	const QString &sStorage,
	const QString &sSnapshotUuid,
	const QString &sVmConfig,
	quint64 nOriginalSize,
	quint32 nFlags,
	quint32 nInternalFlags
)
: CVmBackupCreateCommand(VmBackupCreateLocalCmd, sVmUuid, sVmName, sHost, sServerUuid,
			sDescription, sVmConfig, nOriginalSize, nFlags, nInternalFlags)
{
	SetStringParamValue(sStorage, EVT_PARAM_ISCSI_STORAGE);
	SetStringParamValue(sSnapshotUuid, EVT_PARAM_SNAPSHOT_UUID);
}

bool CVmBackupCreateLocalCommand::IsValid()
{
	return (CVmBackupCreateCommand::IsValid());
}

QString CVmBackupCreateLocalCommand::GetSnapshotUuid()
{
	return (GetStringParamValue(EVT_PARAM_SNAPSHOT_UUID));
}

QString CVmBackupCreateLocalCommand::GetStorage()
{
	return (GetStringParamValue(EVT_PARAM_ISCSI_STORAGE));
}

//*******************************************Create backup first reply implementation*********************************
CVmBackupCreateFirstReply::CVmBackupCreateFirstReply(
	const QString &sBackupUuid,
	quint32 nBackupNumber,
	const QString &sBackupRootPath,
	quint64 nFreeDiskSpace,
	quint32 nFlags)
:CVmBackupProto(VmBackupCreateFirstReply, nFlags)
{
	SetStringParamValue(sBackupUuid, EVT_PARAM_BACKUP_CMD_BACKUP_UUID);
	SetUnsignedIntParamValue(nBackupNumber, EVT_PARAM_BACKUP_CMD_BACKUP_NUMBER);
	SetStringParamValue(sBackupRootPath, EVT_PARAM_BACKUP_CMD_BACKUP_ROOT_PATH);
	SetUnsignedInt64ParamValue(nFreeDiskSpace, EVT_PARAM_BACKUP_CMD_FREE_DISK_SPACE);
}

bool CVmBackupCreateFirstReply::IsValid()
{
	return (
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_UUID, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_NUMBER, PVE::UnsignedInt) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_ROOT_PATH, PVE::String) &&
		CVmBackupProto::IsValid());
}

QString CVmBackupCreateFirstReply::GetBackupUuid()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_UUID));
}

quint32 CVmBackupCreateFirstReply::GetBackupNumber()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_NUMBER));
}

QString CVmBackupCreateFirstReply::GetBackupRootPath()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_ROOT_PATH));
}

quint32 CVmBackupCreateFirstReply::GetFlags()
{
	return CDispToDispCommand::GetCommandFlags();
}

bool CVmBackupCreateFirstReply::GetFreeDiskSpace(quint64 &nFreeDiskSpace)
{
	if (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_FREE_DISK_SPACE, PVE::UInt64))
	{
		nFreeDiskSpace = (GetUnsignedInt64ParamValue(EVT_PARAM_BACKUP_CMD_FREE_DISK_SPACE));
		return true;
	}
	return false;
}
//**********************************************Restore backup command implementation*********************************
CVmBackupRestoreCommand::CVmBackupRestoreCommand(
	const QString &sVmUuid,
	const QString &sBackupUuid,
	quint32 nFlags
)
: CVmBackupCommand(VmBackupRestoreCmd, sVmUuid, nFlags)
{
	SetStringParamValue(sBackupUuid, EVT_PARAM_BACKUP_CMD_BACKUP_UUID);
}

bool CVmBackupRestoreCommand::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_UUID, PVE::String) &&
		CVmBackupCommand::IsValid());
}

QString CVmBackupRestoreCommand::GetBackupUuid()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_UUID));
}

//*******************************************Restore backup first reply implementation*********************************
CVmBackupRestoreFirstReply::CVmBackupRestoreFirstReply(
	const QString &sVmUuid,
	const QString &sVmName,
	const QString &sVmConfiguration,
	const QString &sBackupUuid,
	quint32 nBackupNumber,
	const QString &sBackupRootPath,
	quint64 nOriginalSize,
	quint32 nBundlePermissions,
	quint32 nInternalFlags,
	quint32 nVersion
)
:CVmBackupCommand(VmBackupRestoreFirstReply, sVmUuid, 0, nInternalFlags, nVersion)
{
	SetStringParamValue(sVmName, EVT_PARAM_BACKUP_CMD_VM_NAME);
	SetStringParamValue(sVmConfiguration, EVT_PARAM_BACKUP_CMD_VM_CONFIG);
	SetStringParamValue(sBackupUuid, EVT_PARAM_BACKUP_CMD_BACKUP_UUID);
	SetUnsignedIntParamValue(nBackupNumber, EVT_PARAM_BACKUP_CMD_BACKUP_NUMBER);
	SetStringParamValue(sBackupRootPath, EVT_PARAM_BACKUP_CMD_BACKUP_ROOT_PATH);
	SetUnsignedInt64ParamValue(nOriginalSize, EVT_PARAM_BACKUP_CMD_ORIGINAL_SIZE);
	SetUnsignedIntParamValue(nBundlePermissions, EVT_PARAM_BACKUP_CMD_BUNDLE_PERMISSIONS);
}

bool CVmBackupRestoreFirstReply::IsValid()
{
/* do not call CVmBackupCommand::IsValid() - VmUuid field is not a mandatory due to compatibility
   with psbm-4 & psfm */
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_VM_CONFIG, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_UUID, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_NUMBER, PVE::UnsignedInt) &&
		CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_ROOT_PATH, PVE::String) &&
		CVmBackupProto::IsValid());
}

QString CVmBackupRestoreFirstReply::GetVmName()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_VM_NAME));
}

QString CVmBackupRestoreFirstReply::GetVmConfig()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_VM_CONFIG));
}

QString CVmBackupRestoreFirstReply::GetBackupUuid()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_UUID));
}

quint32 CVmBackupRestoreFirstReply::GetBackupNumber()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_NUMBER));
}

QString CVmBackupRestoreFirstReply::GetBackupRootPath()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_ROOT_PATH));
}

quint64 CVmBackupRestoreFirstReply::GetOriginalSize()
{
	return (GetUnsignedInt64ParamValue(EVT_PARAM_BACKUP_CMD_ORIGINAL_SIZE));
}

quint32 CVmBackupRestoreFirstReply::GetBundlePermissions()
{
	return (GetUnsignedInt64ParamValue(EVT_PARAM_BACKUP_CMD_BUNDLE_PERMISSIONS));
}

//**********************************************Remove backup command implementation*********************************
CVmBackupRemoveCommand::CVmBackupRemoveCommand(
	const QString &sVmUuid,
	const QString &sBackupUuid,
	quint32 nFlags
)
: CVmBackupCommand(VmBackupRemoveCmd, sVmUuid, nFlags)
{
	SetStringParamValue(sBackupUuid, EVT_PARAM_BACKUP_CMD_BACKUP_UUID);
}

bool CVmBackupRemoveCommand::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_BACKUP_UUID, PVE::String) &&
		CVmBackupCommand::IsValid());
}

QString CVmBackupRemoveCommand::GetBackupUuid()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_BACKUP_UUID));
}

//********************************************** Get backup tree reply implementation*********************************
CGetBackupTreeReply::CGetBackupTreeReply(const QString &sReply)
:CVmBackupProto(VmBackupGetTreeReply)
{
	SetStringParamValue(sReply, EVT_PARAM_BACKUP_CMD_TREE);
}

bool CGetBackupTreeReply::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_BACKUP_CMD_TREE, PVE::String) &&
		CVmBackupProto::IsValid());
}

QString CGetBackupTreeReply::GetBackupTree()
{
	return (GetStringParamValue(EVT_PARAM_BACKUP_CMD_TREE));
}

//**********************************************Attach backup command implementation*********************************

CVmBackupAttachCommand::CVmBackupAttachCommand(
	const QString &sVmUuid,
	const QString &sDiskConfig,
	const QString &sDiskDir
)
: CVmBackupCommand(VmBackupAttachCmd, sVmUuid, 0)
{
	SetStringParamValue( sDiskConfig, EVT_PARAM_ATTACH_VM_BACKUP_DISK_CONFIG );
	SetStringParamValue( sDiskDir, EVT_PARAM_ATTACH_VM_BACKUP_DISK_DIR );
}

bool CVmBackupAttachCommand::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_ATTACH_VM_BACKUP_DISK_CONFIG, PVE::String)
			&& CheckWhetherParamPresents(EVT_PARAM_ATTACH_VM_BACKUP_DISK_DIR, PVE::String)
			&& CVmBackupCommand::IsValid());
}

QString CVmBackupAttachCommand::GetDiskConfig()
{
	return (GetStringParamValue(EVT_PARAM_ATTACH_VM_BACKUP_DISK_CONFIG));
}

QString CVmBackupAttachCommand::GetDiskDir()
{
	return (GetStringParamValue(EVT_PARAM_ATTACH_VM_BACKUP_DISK_DIR));
}

//**********************************************Connect backup on source side command implementation*********************************

CVmBackupConnectSourceCommand::CVmBackupConnectSourceCommand(
	const QString &sVmUuid,
	const QString &sDiskConfig
)
: CVmBackupCommand(VmBackupConnectSourceCmd, sVmUuid, 0)
{
	SetStringParamValue( sDiskConfig, EVT_PARAM_CONNECT_VM_BACKUP_SOURCE_DISK_CONFIG );
}

bool CVmBackupConnectSourceCommand::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_CONNECT_VM_BACKUP_SOURCE_DISK_CONFIG, PVE::String)
			&& CVmBackupCommand::IsValid());
}

QString CVmBackupConnectSourceCommand::GetDiskConfig()
{
	return (GetStringParamValue(EVT_PARAM_CONNECT_VM_BACKUP_SOURCE_DISK_CONFIG));
}

} //parallels namespace
