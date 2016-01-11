/////////////////////////////////////////////////////////////////////////////
///
///	@file CVmMigrationProto.h
///
///	Implementation of VM migration protocol commands serializer helpers.
///
///	@author sandro
/// @owner sergeym@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////

#ifndef CVmMigrationProto_H
#define CVmMigrationProto_H

#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

#define DISK_MGRT_PARAM_ID "MigrationInstanceId"

namespace Parallels
{

#include <prlcommon/Interfaces/packed.h>
struct CVmMigrateMemPage {
	UINT nPageNo;
	char sPage[PAGE_SIZE];
};

typedef struct
{
	PRL_UINT64 lba;
	PRL_UINT32 nsect;
	PRL_GUID disk_id;
} CVmMigrateDiskBlock_t;
#include <prlcommon/Interfaces/unpacked.h>

class CVmMigrateProto : public CDispToDispCommand
{
public:
	CVmMigrateProto(Parallels::IDispToDispCommands nCmdIdentifier, quint32 nFlags = 0);
	quint32 GetVersion();
	bool IsValid();
};

typedef enum _PRL_VM_RESERVED_FLAGS
{
	PVM_DONT_COPY_VM	= (1 << 1),
	PVM_ISCSI_STORAGE	= (1 << 2),
	PVM_FULL_DISP_TASK	= (1 << 3),
	PVM_CT_MIGRATE		= (1 << 4),
	PVM_HA_MOVE_VM		= (1 << 5),
} PRL_VM_RESERVED_FLAGS;

/**
 * Base migrate command class
 */
class CVmMigrateCommand : public CVmMigrateProto
{
public:
	/**
	 * Class default constructor.
	 */
	CVmMigrateCommand(Parallels::IDispToDispCommands nCmdIdentifier)
	: CVmMigrateProto(nCmdIdentifier)
	{}
	/**
	 * Class constructor.
	 * @param migrating VM configuration XML representation string
	 * @param source host hardware info XML representation string
	 * @param target VM home path string
	 * @param migration flags
	 * @param reserved flags
	 */
	CVmMigrateCommand(
		Parallels::IDispToDispCommands nCmdIdentifier,
		const QString &sVmConfiguration,
		const QString &sTargetVmHomePath,
		quint32 nMigrationFlags,
		quint32 nReservedFlags,
		VIRTUAL_MACHINE_STATE nPrevVmState
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** Returns migrating VM configuration XML representation string */
	QString GetVmConfig();
	/** Returns target Vm home path string */
	QString GetTargetVmHomePath();
	/** Returns migration flags */
	quint32 GetMigrationFlags();
	/** Returns reserved flags */
	quint32 GetReservedFlags();
	/** Returns previous migrating VM state */
	VIRTUAL_MACHINE_STATE GetVmPrevState();
};

/**
 * Serializer helper class that let to generate and process VmMigrateCheckPreconditionsCmd command
 */
class CVmMigrateCheckPreconditionsCommand : public CVmMigrateCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmMigrateCheckPreconditionsCommand()
	: CVmMigrateCommand(VmMigrateCheckPreconditionsCmd)
	{}
	/**
	 * Class constructor.
	 * @param migrating VM configuration XML representation string
	 * @param source host hardware info XML representation string
	 * @param target VM name string
	 * @param target VM home path string
	 * @param temporary file name to detect shared storage
	 * @param VM home storage info (for iSCSI storage, as sample)
	 * @param migration flags
	 * @param reserved flags
	 */
	CVmMigrateCheckPreconditionsCommand(
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
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** Returns source host hardware info XML representation string */
	QString GetSourceHostHardwareInfo();
	/** Returns name that will be assigned to Vm on target server*/
	QString GetTargetVmName();
	/** Returns name of the temporary file used to detect shared storage */
	QString GetSharedFileName();
	/** Returns list of temporary filenames used to detect shared storage
	 * for external disk */
	QStringList GetSharedFileNamesExtra();
	/** Returns storage info */
	QString GetStorageInfo();
	/** Returns requires disk space */
	PRL_UINT64 GetRequiresDiskSpace();
};

/**
 * Serializer helper class that let to generate and process VmMigrateStartCmd command
 */
class CVmMigrateStartCommand : public CVmMigrateCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmMigrateStartCommand()
	:CVmMigrateCommand(VmMigrateStartCmd)
	{}
	/**
	 * Class constructor.
	 * @param migrating VM configuration XML representation string
	 * @param runtime VM configuration XML representation string
	 * @param target VM home path string
	 * @param shapshot uuid
	 * @param Vm bundle permissions
	 * @param Vm config permissions
	 * @param migration flags
	 * @param reserved flags
	 * @param previous migrating VM state (to restore it after migration complete)
	 */
	CVmMigrateStartCommand(
		const QString &sVmConfiguration,
		const QString &sVmRuntimeConfiguration,
		const QString &sTargetVmHomePath,
		const QString &sSnapshotUuid,
		quint32 nBundlePermissions,
		quint32 nConfigPermissions,
		quint32 nMigrationFlags,
		quint32 nReservedFlags,
		VIRTUAL_MACHINE_STATE nPrevVmState
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** Returns migrating VM runtime configuration XML representation string */
	QString GetVmRuntimeConfig();
	QString GetNetworkConfig();
	QString GetDispatcherConfig();
	/** Returns shanpshot UUID (start from MIGRATE_DISP_PROTO_V2) */
	QString GetSnapshotUuid();
	quint32 GetBundlePermissions();
	quint32 GetConfigPermissions();
};

/**
 * Serializer helper class that let to generate and process VmMigrateCheckPreconditionsReply
 */
class CVmMigrateCheckPreconditionsReply : public CVmMigrateProto
{
public:
	/**
	 * Class default constructor.
	 */
	CVmMigrateCheckPreconditionsReply()
	:CVmMigrateProto(VmMigrateCheckPreconditionsReply)
	{}
	/**
	 * Class constructor.
	 * @param result of preconditions check
	 * @param internal flags
	 */
	CVmMigrateCheckPreconditionsReply(
		const QStringList &sCheckPreconditionsResult,
		const QStringList &lstNonSharedDisks,
		quint32 nFlags
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	QStringList GetCheckPreconditionsResult();
	QStringList GetNonSharedDisks();
};

/**
 * Serializer helper class that let to generate and process VmMigrateReply
 */
class CVmMigrateReply : public CVmMigrateProto
{
public:
	/**
	 * Class default constructor.
	 */
	CVmMigrateReply()
	:CVmMigrateProto(VmMigrateReply)
	{}
	/**
	 * Class constructor.
	 * @param memory swap file path
	 */
	CVmMigrateReply(
		const QString &sMemFilePath
	);
	QString GetMemFilePath();
};

}//namespace Parallels

#endif
