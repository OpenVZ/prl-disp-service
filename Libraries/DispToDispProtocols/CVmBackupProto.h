/////////////////////////////////////////////////////////////////////////////
///
///	@file CVmBackupProto.h
///
///	Implementation of VM backup protocol commands serializer helpers.
///
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

#ifndef CVmBackupProto_H
#define CVmBackupProto_H

#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

namespace Parallels
{

typedef enum _PRL_BACKUP_INTERNAL_FLAGS
{
	PVM_CT_VZFS_BACKUP	= (1 << 1),
	PVM_CT_MOUNTED		= (1 << 2),
	PVM_CT_RUNNING		= (1 << 3),
	PVM_CT_PLOOP_BACKUP	= (1 << 4),
	PVM_CT_VZWIN_BACKUP	= (1 << 5),
	PVM_CT_BACKUP		= (PVM_CT_VZFS_BACKUP | PVM_CT_PLOOP_BACKUP | PVM_CT_VZWIN_BACKUP)
} PRL_BACKUP_INTERNAL_FLAGS;

/**
 * Base class for backup protocol
 */
class CVmBackupProto : public CDispToDispCommand
{
public:
	CVmBackupProto(
		Parallels::IDispToDispCommands nCmdIdentifier,
		quint32 nFlags = 0,
		quint32 nInternalFlags = 0,
		quint32 nVersion = BACKUP_PROTO_VERSION);
	~CVmBackupProto() {}
	/* Returns backup protocol version */
	quint32 GetVersion();
	void setVersion(quint32 version);
	quint32 GetInternalFlags();
	virtual bool IsValid();
};

/**
 * Base class for backup command
 */
class CVmBackupCommand : public CVmBackupProto
{
public:
	CVmBackupCommand(
		Parallels::IDispToDispCommands nCmdIdentifier,
		const QString &sVmUuid,
		quint32 nFlags,
		quint32 nInternalFlags = 0,
		quint32 nVersion = BACKUP_PROTO_VERSION);
	~CVmBackupCommand() {}
	/** Returns flags */
	quint32 GetFlags();
	QString GetVmUuid();
	virtual bool IsValid();
};

/**
 * Serializer helper class that let to generate and process VmBackupCreateCmd command
 */
class CVmBackupCreateCommand : public CVmBackupCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmBackupCreateCommand()
	: CVmBackupCommand(VmBackupCreateCmd, QString(), 0)
	{}
	/**
	 * Class constructor.
	 * @param VM uuid
	 * @param VM name
	 * @param server uuid
	 * @param backup decsription
	 * @param flags
	 * @param internal flags (this flags uses only for disp-disp communications
				and client can not see this flags)
	 */
	CVmBackupCreateCommand(
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
	);
	CVmBackupCreateCommand(
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
	);

	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	QString GetVmName();
	QString GetHost();
	QString GetServerUuid();
	QString GetVmConfig();
	QString GetDescription();
	quint64 GetOriginalSize();
	quint32 GetBundlePermissions();
	QStringList GetBitmaps();
};

/**
 * Serializer helper class that let to generate and process VmBackupCreateCmd command
 */
class CVmBackupCreateLocalCommand : public CVmBackupCreateCommand
{
public:
	CVmBackupCreateLocalCommand()
	: CVmBackupCreateCommand(VmBackupCreateLocalCmd,
		QString(), QString(), QString(), QString(), QString(), QString(), 0, 0, 0)
	{}

	CVmBackupCreateLocalCommand(
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
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	QString GetStorage();
	QString GetSnapshotUuid();
};

/**
 * serializer helper class that let to generate and process VmBackupCreateFirstReply
 */
class CVmBackupCreateFirstReply : public CVmBackupProto
{
public:
	CVmBackupCreateFirstReply()
	:CVmBackupProto(VmBackupCreateFirstReply) {}
	CVmBackupCreateFirstReply(
		const QString &sBackupUuid,
		quint32 nBackupNumber,
		const QString &sBackupRootPath,
		quint64 nFreeDiskSpace,
		quint32 nFlags
	);
	bool IsValid();
	QString GetBackupUuid();
	quint32 GetBackupNumber();
	QString GetBackupRootPath();
	quint32 GetFlags();
	bool GetFreeDiskSpace(quint64 &nFreeDIskSpace);
};

/**
 * Serializer helper class that let to generate and process VmBackupRestoreCmd command
 */
class CVmBackupRestoreCommand : public CVmBackupCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmBackupRestoreCommand()
	: CVmBackupCommand(VmBackupRestoreCmd, QString(), 0)
	{}
	/**
	 * Class constructor.
	 * @param VM uuid
	 * @param backup uuid
	 * @param flags
	 */
	CVmBackupRestoreCommand(
		const QString &sVmUuid,
		const QString &sBackupUuid,
		quint32 nFlags
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	QString GetBackupUuid();
};

/**
 * serializer helper class that let to generate and process VmBackupRestoreFirstReply
 */
class CVmBackupRestoreFirstReply : public CVmBackupCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmBackupRestoreFirstReply()
	:CVmBackupCommand(VmBackupRestoreFirstReply, QString(), 0)
	{}
	/**
	 * Class constructor.
	 * @param VM uuid
	 * @param backup uuid
	 * @param flags
	 */
	CVmBackupRestoreFirstReply(
		const QString &sVmUuid,
		const QString &sVmName,
		const QString &sVmConfiguration,
		const QString &sBackupUuid,
		quint32 nBackupNumber,
		const QString &sBackupRootPath,
		quint64 nOriginalSize,
		quint32 nBundlePermissions,
		quint32 nInternalFlags,
		quint32 nVersion = BACKUP_PROTO_VERSION
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** Returns migrating VM configuration XML representation string */
	QString GetVmName();
	QString GetVmConfig();
	QString GetBackupUuid();
	quint32 GetBackupNumber();
	QString GetBackupRootPath();
	quint64 GetOriginalSize();
	quint32 GetBundlePermissions();
};

/**
 * Serializer helper class that let to generate and process VmBackupRemoveCmd command
 */
class CVmBackupRemoveCommand : public CVmBackupCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmBackupRemoveCommand()
	: CVmBackupCommand(VmBackupRemoveCmd, QString(), 0)
	{}
	/**
	 * Class constructor.
	 * @param VM uuid
	 * @param backup uuid
	 * @param flags
	 */
	CVmBackupRemoveCommand(
		const QString &sVmUuid,
		const QString &sBackupUuid,
		quint32 nFlags
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	QString GetBackupUuid();
};

typedef class CVmBackupCommand CGetBackupTreeCommand;

/**
 * serializer helper class that let to generate and process VmBackupRestoreFirstReply
 */
class CGetBackupTreeReply : public CVmBackupProto
{
public:
	/**
	 * Class default constructor.
	 */
	CGetBackupTreeReply()
	:CVmBackupProto(VmBackupGetTreeReply)
	{}
	/**
	 * Class constructor.
	 * @param backup tree in reply
	 */
	CGetBackupTreeReply(const QString &sReply);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** Returns backup tree representation string */
	QString GetBackupTree();
};

/**
 * Serializer helper class that let to generate VmBackupAttachCmd protocol command
 */
class CVmBackupAttachCommand : public CVmBackupCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmBackupAttachCommand()
	: CVmBackupCommand(VmBackupAttachCmd, QString(), 0)
	{}
	/**
	 * Class constructor.
	 * @param VM UUID
	 * @param disk configuration
	 * @param directory, where the disk bundle should be created
	 */
	CVmBackupAttachCommand(
		const QString &sVmUuid,
		const QString &sDiskConfig,
		const QString &sDiskDir
	);
	/** Overridden method that let to determine whether protocol command valid */
	virtual bool IsValid();
	/** Return disk configuration */
	QString GetDiskConfig();
	/** Return the full path to the directory, where the disk bundle should be created */
	QString GetDiskDir();
};

/**
 * Serializer helper class that let to generate DspCmdCtlConnectVmBackupSource protocol command
 */
class CVmBackupConnectSourceCommand : public CVmBackupCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CVmBackupConnectSourceCommand()
	: CVmBackupCommand(VmBackupConnectSourceCmd, QString(), 0)
	{}
	/**
	 * Class constructor.
	 * @param VM UUID
	 * @param disk configuration
	 */
	CVmBackupConnectSourceCommand(
		const QString &sVmUuid,
		const QString &sDiskConfig
	);
	/** Overridden method that let to determine whether protocol command valid */
	virtual bool IsValid();
	/** Return disk configuration */
	QString GetDiskConfig();
};

} //Parallels
#endif // CVmBackupProto_H
