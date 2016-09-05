/////////////////////////////////////////////////////////////////////////////
///
///	@file CDispToDispCommonProto.h
///
///	Implementation of Dispatcher-Dispatcher common protocol commands serializer helpers.
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

#ifndef CDispToDispCommonProto_H
#define CDispToDispCommonProto_H

#include <prlcommon/Interfaces/ParallelsDispToDispProto.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>

#ifdef _WIN_
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Parallels
{
using namespace IOService;

typedef CProtoCommandBaseTempl<IDispToDispCommands> CDispToDispCommand;
typedef SmartPtr<CDispToDispCommand> CDispToDispCommandPtr;

/**
 * Dispatcher-dispatcher common proto packages instantiation methods set
 */
class CDispToDispProtoSerializer
{
	public:
	/**
	 * Parses specified command string buffer and returns pointer to proto command object of corresponding type
	 * @param proto command type identifier
	 * @param package string buffer for parsing
	 */
	static CDispToDispCommandPtr ParseCommand(IDispToDispCommands nCmdIdentifier, const QString &sPackage);

	/**
	 * Same as above, just an overloaded method.
	 */
	static CDispToDispCommandPtr ParseCommand( const SmartPtr<IOService::IOPackage>& );

	/**
	 * Template class method that let to casting base proto command pointer to necessary command type
	 */
	template <typename T>
	static T *CastToDispToDispCommand(CDispToDispCommandPtr pCommand)
	{
		return (static_cast<T *>(pCommand.getImpl()));
	}

	/**
	 * Generates DispToDispCmdAuthorize protocol command
	 * @param user session UUID
	 */
	static CDispToDispCommandPtr CreateDispToDispAuthorizeCommand(
		const QString &sUserSessionId
	);

	/**
	 * Generates DispToDispCmdAuthorize protocol command
	 * @param user name
	 * @param password
	 */
	static CDispToDispCommandPtr CreateDispToDispAuthorizeCommand(
		const QString &sUserName, const QString &sPassword
	);

	/**
	 * Generates dispatcher-dispatcher protocol command without parameters
	 * @param command identifier
	 */
	static CDispToDispCommandPtr CreateDispToDispCommandWithoutParams(
		IDispToDispCommands nCmdIdentifier
	);

	/**
	 * Generates dispatcher-dispatcher protocol response
	 * @param request command identifier
	 * @param return code
	 * @param pointer to additional error information object (optional)
	 * @param response parameters list
	 */
	static CDispToDispCommandPtr CreateDispToDispResponseCommand(
		IDispToDispCommands nRequestCmdId,
		PRL_RESULT nRetCode,
		const CVmEvent *pErrorEvent = NULL,
		const QStringList &lstParams = QStringList()
	);

	/**
	 * Generates dispatcher-dispatcher protocol response
	 * @param pointer to request package object
	 * @param pointer to additional error information object
	 * @param response parameters list
	 */
	static CDispToDispCommandPtr CreateDispToDispResponseCommand(
		const SmartPtr<IOPackage> &pRequestPkg,
		const CVmEvent *pErrorEvent,
		const QStringList &lstParams = QStringList()
	);

	/**
	 * Generates dispatcher-dispatcher protocol response
	 * @param return code
	 * @param pointer to request package object
	 * @param response parameters list
	 */
	static CDispToDispCommandPtr CreateDispToDispResponseCommand(
		PRL_RESULT nRetCode,
		const SmartPtr<IOPackage> &pRequestPkg,
		const QStringList &lstParams = QStringList()
	);

	/**
	 * Generates VmMigrateCheckPreconditionsCmd protocol command
	 * @param migrating VM configuration XML representation string
	 * @param source host hardware info XML representation string
	 * @param target VM name string
	 * @param target VM home path string
	 * @param temporary file name on source VM home path (for shared VM check)
	 * @param VM home storage info (for iSCSI storage, as sample)
	 * @param requires disk space in bytes
	 * @param migration flags
	 * @param reserved flags
	 * @param previous migrating VM state
	 */
	static CDispToDispCommandPtr CreateVmMigrateCheckPreconditionsCommand(
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

	/**
	 * Generates CreateVmMigrateCheckPreconditionsReply protocol command
	 * @param result of preconditions check
	 * @param flags
	*/
	static CDispToDispCommandPtr CreateVmMigrateCheckPreconditionsReply(
		const QStringList &sCheckPreconditionsResult,
		const QStringList &lstNonSharedDisks,
		quint32 nFlags
	);

	/**
	 * Generates VmMigrateStartCmd protocol command
	 * @param migrating VM configuration XML representation string
	 * @param runtime VM configuration XML representation string
	 * @param target VM home path string
	 * @param snapshot UUID
	 * @param Vm bundle permissions
	 * @param Vm config permissions
	 * @param migration flags
	 * @param reserved flags
	 * @param previous migrating VM state (to restore it after migration will complete)
	 */
	static CDispToDispCommandPtr CreateVmMigrateStartCommand(
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

	/**
	 * Generates VmMigrateReply protocol command
	*/
	static CDispToDispCommandPtr CreateVmMigrateReply(
		const QString &sMemFilePath
	);

	/**
	 * Generates VmBackupCreateCmd protocol command
	 * @param Vm uuid
	 * @param backup uuid
	 * @param flags
	 */
	static CDispToDispCommandPtr CreateVmBackupCreateCommand(
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
		quint32 nInternalFlags = 0
	);

	static CDispToDispCommandPtr CreateVmBackupCreateLocalCommand(
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
		quint32 nInternalFlags = 0
	);

	/**
	 * Generates VmBackupCreateFirstReply protocol command
	 * @param acronis backup uuid
	 */
	static CDispToDispCommandPtr CreateVmBackupCreateFirstReply(
		const QString &sBackupUuid,
		quint32 nBackupNumber,
		const QString &sBackupRootPath,
		quint64 nFreeDiskSpace,
		quint32 nFlags);

	/**
	 * Generates VmBackupRestoreCmd protocol command
	 * @param Vm uuid
	 * @param backup uuid
	 * @param flags
	 */
	static CDispToDispCommandPtr CreateVmBackupRestoreCommand(
		const QString &sVmUuid,
		const QString &sBackupUuid,
		quint32 nFlags
	);

	/**
	 * Generates VmBackupRestoreFirstReply protocol command
	 * @param Vm config
	 * @param backup uuid
	 */
	static CDispToDispCommandPtr CreateVmBackupRestoreFirstReply(
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

	/**
	 * Generates VmBackupGetTree protocol command
	 * @param Vm Uuid (optional)
	 * @param flags
	 */
	static CDispToDispCommandPtr CreateGetBackupTreeCommand(
		const QString &sVmUuid,
		quint32 nFlags);

	/**
	 * Generates VmBackupGetTreeReply protocol command
	 * @param backup tree as string
	 */
	static CDispToDispCommandPtr CreateGetBackupTreeReply(const QString &sReply);

	/**
	 * Generates VmBackupRemoveCmd protocol command
	 * @param Vm uuid
	 * @param backup uuid
	 * @param flags
	 */
	static CDispToDispCommandPtr CreateVmBackupRemoveCommand(
		const QString &sVmUuid,
		const QString &sBackupUuid,
		quint32 nFlags);

	/**
	 * Generates CopyCtTemplateCmd protocol command
	 * @param template name
	 * @param os template name
	 * @param command flags
	 * @param reserved command flags
	 */
	static CDispToDispCommandPtr CreateCopyCtTemplateCommand(
		const QString &sTmplName,
		const QString &sOsTmplName,
		quint32 nFlags,
		quint32 nReservedFlags);

	/* Generates CopyCtTemplateReply protocol command */
	static CDispToDispCommandPtr CreateCopyCtTemplateReply();

	/**
	 * Generates VmBackupAttach command
	 * @param VM UUID
	 * @param disk configuration
	 * @param directory, where the disk bundle should be created
	 * @param flags
	 */
	static CDispToDispCommandPtr CreateVmBackupAttachCommand(
		const QString &sVmUuid,
		const QString &sDiskConfig,
		const QString &sDiskPath
	);

	/**
	 * Generates VmBackupConnectSource command
	 * @param VM UUID
	 * @param disk configuration
	 */
	static CDispToDispCommandPtr CreateVmBackupConnectSourceCommand(
		const QString &sVmUuid,
		const QString &sDiskConfig
	);
};

/**
 * Stub for DispToDispCmdUnknown
 */
class CDispToDispUnknownCommand : public CDispToDispCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CDispToDispUnknownCommand()
	: CDispToDispCommand(DispToDispUnknownCmd)
	{}
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid()
	{
		return (false);
	}
};

/**
 * Serializer helper class that let to generate basic protocol command contains no additional parameters
 */
class CDispToDispCommandWithoutParams : public CDispToDispCommand
{
public:
	/**
	 * Class default constructor.
	 * @param protocol command identifier
	 */
	CDispToDispCommandWithoutParams(IDispToDispCommands nCmdIdentifier)
	: CDispToDispCommand(nCmdIdentifier, false)
	{}
	/** Overridden method that let to determine whether protocol command valid */
	virtual bool IsValid()
	{
		return (true);
	}
};

/**
 * Serializer helper class that let to generate and process DispToDispAuthorizeCmd command
 */
class CDispToDispAuthorizeCommand : public CDispToDispCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CDispToDispAuthorizeCommand()
	: CDispToDispCommand(DispToDispAuthorizeCmd, false)
	{}
	/**
	 * Class constructor.
	 * @param user session UUID
	 */
	CDispToDispAuthorizeCommand(
		const QString &sUserSessionUuid
	);
	/**
	 * Class constructor.
	 * @param user name
	 * @param password
	 */
	CDispToDispAuthorizeCommand(
		const QString &sUserName, const QString &sPassword
	);

	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** determine whether authorizetion by session uuid or login */
	bool NeedAuthBySessionUuid();
	/** Returns user session UUID */
	QString GetUserSessionUuid();
	/** Returns user name */
	QString GetUserName();
	/** Returns password */
	QString GetPassword();

};

/**
 * Serializer helper class that let to generate and process DispToDispResponseCmd command
 */
class CDispToDispResponseCommand : public CDispToDispCommand
{
public:
	/**
	 * Class default constructor.
	 */
	CDispToDispResponseCommand()
	: CDispToDispCommand(DispToDispResponseCmd, false)
	{}
	/**
	 * Class constructor.
	 * @param request command identifier
	 * @param return code
	 * @param error additional info string
	 * @param response parameters list
	 */
	CDispToDispResponseCommand(
		IDispToDispCommands nRequestCmdId,
		PRL_RESULT nRetCode,
		const QString &sErrorInfo,
		const QStringList &lstParams = QStringList()
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	/** Returns request command id */
	IDispToDispCommands GetRequestCommandId();
	/** Returns return code info */
	PRL_RESULT GetRetCode();
	/** Returns pointer to additional error information object */
	SmartPtr<CVmEvent> GetErrorInfo();
	/**
	 * Adds new parameter to response command
	 * @param adding parameter value
	 */
	void AddParam(const QString &sParamValue);
	/** Returns reponse parameters list */
	QStringList GetParams();
};

namespace DispatcherPackage
{
inline SmartPtr<IOService::IOPackage> createInstance (
    int cmdNumber,
    const CDispToDispCommandPtr& cmd,
    const SmartPtr<IOService::IOPackage>& parent = SmartPtr<IOService::IOPackage>(),
    bool broadcastResponse = false )
{
    return createInstance( cmdNumber, cmd->GetCommand()->toString(),
                           parent, broadcastResponse );
}

/**
 * Overloaded method to prevent possibility of incorrect above method usage with non native pointers
 */
inline SmartPtr<IOService::IOPackage> createInstance (
    int cmdNumber,
    const CDispToDispCommand *pCmd,
    const SmartPtr<IOService::IOPackage>& parent = SmartPtr<IOService::IOPackage>(),
    bool broadcastResponse = false )
{
    return createInstance( cmdNumber, pCmd->GetCommand()->toString(),
                           parent, broadcastResponse );
}


}//DispatcherPackage

}//namespace Parallels

#endif
