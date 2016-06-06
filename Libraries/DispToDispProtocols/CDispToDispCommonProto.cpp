/////////////////////////////////////////////////////////////////////////////
///
///	@file CDispToDispCommonProto.cpp
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

#include "CDispToDispCommonProto.h"
#include "CVmMigrationProto.h"
#include "CVmBackupProto.h"
#include "CCtTemplateProto.h"
#include <prlxmlmodel/Messaging/CVmEventParameterList.h>

namespace Parallels
{

//*****************************************CDispToDispProtoSerializer implementation***********************************
CDispToDispCommandPtr CDispToDispProtoSerializer::ParseCommand(
	IDispToDispCommands nCmdIdentifier,
	const QString &sPackage
)
{
	CDispToDispCommand* pCommand;
	switch (nCmdIdentifier)
	{
		case DispToDispAuthorizeCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CDispToDispAuthorizeCommand);
		break;

		case DispToDispLogoffCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CDispToDispCommandWithoutParams(nCmdIdentifier));
		break;

		case DispToDispResponseCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CDispToDispResponseCommand);
		break;

		case VmMigrateCheckPreconditionsCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmMigrateCheckPreconditionsCommand);
		break;

		case VmMigrateCheckPreconditionsReply:
			pCommand = static_cast<CDispToDispCommand *>(new CVmMigrateCheckPreconditionsReply);
		break;

		case VmMigrateStartCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmMigrateStartCommand);
		break;

		case VmMigrateReply:
			pCommand = static_cast<CDispToDispCommand *>(new CVmMigrateReply);
		break;

		case VmBackupCreateCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupCreateCommand);
		break;

		case VmBackupCreateLocalCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupCreateLocalCommand);
		break;

		case VmBackupRestoreCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupRestoreCommand);
		break;

		case VmBackupRestoreFirstReply:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupRestoreFirstReply);
		break;

		case VmBackupGetTreeCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupCommand(VmBackupGetTreeCmd, QString(), 0));
		break;

		case VmBackupGetTreeReply:
			pCommand = static_cast<CDispToDispCommand *>(new CGetBackupTreeReply);
		break;

		case VmBackupRemoveCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupRemoveCommand);
		break;
		case CopyCtTemplateCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CCopyCtTemplateCommand);
		break;
		case CopyCtTemplateReply:
			pCommand = static_cast<CDispToDispCommand *>(new CCopyCtTemplateReply);
		break;
		case VmBackupAttachCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupAttachCommand);
			break;
		case VmBackupConnectSourceCmd:
			pCommand = static_cast<CDispToDispCommand *>(new CVmBackupConnectSourceCommand);
			break;

		default: pCommand = static_cast<CDispToDispCommand *>(new CDispToDispUnknownCommand); break;
	}
	pCommand->ParsePackage(sPackage);
	return CDispToDispCommandPtr(pCommand);
}

CDispToDispCommandPtr CDispToDispProtoSerializer::ParseCommand( const SmartPtr<IOService::IOPackage> &p)
{
    if ( ! p.isValid() )
        return CDispToDispCommandPtr(new CDispToDispUnknownCommand);

    return ParseCommand( (IDispToDispCommands)p->header.type,
						 UTF8_2QSTR( p->buffers[0].getImpl() ) );
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateDispToDispAuthorizeCommand(
		const QString &sUserSessionId
	)
{
	return CDispToDispCommandPtr(new CDispToDispAuthorizeCommand(
									sUserSessionId
									));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateDispToDispAuthorizeCommand(
		const QString &sUserName, const QString &sPassword
	)
{
	return CDispToDispCommandPtr(new CDispToDispAuthorizeCommand(
									sUserName, sPassword
									));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateDispToDispCommandWithoutParams(
		IDispToDispCommands nCmdIdentifier
	)
{
	return CDispToDispCommandPtr(new CDispToDispCommandWithoutParams(nCmdIdentifier));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
	IDispToDispCommands nRequestCmdId,
	PRL_RESULT nRetCode,
	const CVmEvent *pErrorEvent,
	const QStringList &lstParams
)
{
	return CDispToDispCommandPtr(new CDispToDispResponseCommand(nRequestCmdId, nRetCode,
		(pErrorEvent ? pErrorEvent->toString() : QString()), lstParams));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
	const SmartPtr<IOPackage> &pRequestPkg,
	const CVmEvent *pErrorEvent,
	const QStringList &lstParams
)
{
	Q_ASSERT(pErrorEvent != NULL);
	return CDispToDispCommandPtr(new CDispToDispResponseCommand(
		(IDispToDispCommands)pRequestPkg->header.type,
		pErrorEvent->getEventCode(),
		pErrorEvent->toString(),
		lstParams));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
	PRL_RESULT nRetCode,
	const SmartPtr<IOPackage> &pRequestPkg,
	const QStringList &lstParams
)
{
	return CDispToDispCommandPtr(new CDispToDispResponseCommand(
		(IDispToDispCommands)pRequestPkg->header.type,
		nRetCode,
		QString(),
		lstParams));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsCommand(
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
{
	return CDispToDispCommandPtr(new CVmMigrateCheckPreconditionsCommand(
				sVmConfiguration,
				sHostHardwareInfo,
				sTargetVmName,
				sTargetVmHomePath,
				sSharedFileName,
				lstSharedFileNamesExtra,
				sStorageInfo,
				nRequiresDiskSpace,
				nMigrationFlags,
				nReservedFlags,
				nPrevVmState
			));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsReply(
	const QStringList &sCheckPreconditionsResult,
	const QStringList &lstNonSharedDisks,
	quint32 nFlags
)
{
	return CDispToDispCommandPtr(new CVmMigrateCheckPreconditionsReply(
				sCheckPreconditionsResult,
				lstNonSharedDisks,
				nFlags));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmMigrateStartCommand(
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
{
	return CDispToDispCommandPtr(new CVmMigrateStartCommand(
				sVmConfiguration,
				sVmRuntimeConfiguration,
				sTargetVmHomePath,
				sSnapshotUuid,
				nBundlePermissions,
				nConfigPermissions,
				nMigrationFlags,
				nReservedFlags,
				nPrevVmState
			));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmMigrateReply(
	const QString &sMemFilePath
)
{
	return CDispToDispCommandPtr(new CVmMigrateReply(sMemFilePath));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupCreateCommand(
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
{
	return CDispToDispCommandPtr(new CVmBackupCreateCommand(
			sVmUuid,
			sVmName,
			sHost,
			sServerUuid,
			sDescription,
			sVmConfig,
			nOriginalSize,
			nBundlePermissions,
			lstBitmaps,
			nFlags,
			nInternalFlags
		));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupCreateLocalCommand(
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
{
	return CDispToDispCommandPtr(new CVmBackupCreateLocalCommand(
			sVmUuid,
			sVmName,
			sHost,
			sServerUuid,
			sDescription,
			sStorage,
			sSnapshotUuid,
			sVmConfig,
			nOriginalSize,
			nFlags,
			nInternalFlags
		));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupCreateFirstReply(
	const QString &sBackupUuid,
	quint32 nBackupNumber,
	const QString &sBackupRootPath,
	quint64 nFreeDiskSpace,
	quint32 nFlags)
{
	return CDispToDispCommandPtr(
		new CVmBackupCreateFirstReply(sBackupUuid, nBackupNumber, sBackupRootPath, nFreeDiskSpace, nFlags));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupRestoreCommand(
	const QString &sVmUuid,
	const QString &sBackupUuid,
	quint32 nFlags
)
{
	return CDispToDispCommandPtr(new CVmBackupRestoreCommand(
			sVmUuid,
			sBackupUuid,
			nFlags
		));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupRestoreFirstReply(
			const QString &sVmUuid,
			const QString &sVmName,
			const QString &sVmConfiguration,
			const QString &sBackupUuid,
			quint32 nBackupNumber,
			const QString &sBackupRootPath,
			quint64 nOriginalSize,
			quint32 nBundlePermissions,
			quint32 nInternalFlags,
			quint32 nVersion)
{
	return CDispToDispCommandPtr(new CVmBackupRestoreFirstReply(
			sVmUuid,
			sVmName,
			sVmConfiguration,
			sBackupUuid,
			nBackupNumber,
			sBackupRootPath,
			nOriginalSize,
			nBundlePermissions,
			nInternalFlags,
			nVersion));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateGetBackupTreeCommand(const QString &sVmUuid, quint32 nFlags)
{
	return CDispToDispCommandPtr(new CVmBackupCommand(VmBackupGetTreeCmd, sVmUuid, nFlags));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateGetBackupTreeReply(const QString &sReply)
{
	return CDispToDispCommandPtr(new CGetBackupTreeReply(sReply));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupRemoveCommand(
	const QString &sVmUuid,
	const QString &sBackupUuid,
	quint32 nFlags
)
{
	return CDispToDispCommandPtr(new CVmBackupRemoveCommand(
			sVmUuid,
			sBackupUuid,
			nFlags
		));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateCopyCtTemplateCommand(
	const QString &sTmplName,
	const QString &sOsTmplName,
	quint32 nFlags,
	quint32 nReservedFlags
)
{
	return CDispToDispCommandPtr(new CCopyCtTemplateCommand(sTmplName, sOsTmplName, nFlags, nReservedFlags));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateCopyCtTemplateReply()
{
	return CDispToDispCommandPtr(new CCopyCtTemplateReply());
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupAttachCommand(
	const QString &sVmUuid,
	const QString &sDiskConfig,
	const QString &sDiskDir
)
{
	return (CDispToDispCommandPtr(new CVmBackupAttachCommand(sVmUuid, sDiskConfig, sDiskDir)));
}

CDispToDispCommandPtr CDispToDispProtoSerializer::CreateVmBackupConnectSourceCommand(
	const QString &sVmUuid,
	const QString &sDiskConfig
)
{
	return (CDispToDispCommandPtr(new CVmBackupConnectSourceCommand(sVmUuid, sDiskConfig)));
}

//*****************************************CDispToDispAuthorizeCommand implementation**********************************
CDispToDispAuthorizeCommand::CDispToDispAuthorizeCommand(
		const QString &sUserSessionUuid
	)
: CDispToDispCommand(DispToDispAuthorizeCmd)
{
	SetStringParamValue( sUserSessionUuid, EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_SESSION_UUID );
}

CDispToDispAuthorizeCommand::CDispToDispAuthorizeCommand(
		const QString &sUserName, const QString &sPassword
	)
: CDispToDispCommand(DispToDispAuthorizeCmd)
{
	SetStringParamValue( sUserName, EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_NAME );
	SetStringParamValue( sPassword, EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_PASSWORD );
}

bool CDispToDispAuthorizeCommand::IsValid()
{
	bool present = CheckWhetherParamPresents(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_SESSION_UUID, PVE::String);
	if (!present)
		present = CheckWhetherParamPresents(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_NAME, PVE::String)
			&& CheckWhetherParamPresents(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_PASSWORD, PVE::String);
	return present;
}

bool CDispToDispAuthorizeCommand::NeedAuthBySessionUuid()
{
	return CheckWhetherParamPresents(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_SESSION_UUID, PVE::String);
}

QString CDispToDispAuthorizeCommand::GetUserSessionUuid()
{
	return (GetStringParamValue(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_SESSION_UUID));
}

QString CDispToDispAuthorizeCommand::GetUserName()
{
	return (GetStringParamValue(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_NAME));
}

QString CDispToDispAuthorizeCommand::GetPassword()
{
	return (GetStringParamValue(EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_PASSWORD));
}



//*****************************************CDispToDispResponseCommand implementation***********************************
CDispToDispResponseCommand::CDispToDispResponseCommand(
	IDispToDispCommands nRequestCmdId,
	PRL_RESULT nRetCode,
	const QString &sErrorInfo,
	const QStringList &lstParams
)
: CDispToDispCommand(DispToDispResponseCmd, false)
{
	SetStringParamValue( sErrorInfo, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO );
	SetUnsignedIntParamValue( nRequestCmdId, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID );
	SetStringListParamValue( lstParams, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST );
	m_pProtoPackage->setEventCode(nRetCode);
}

bool CDispToDispResponseCommand::IsValid()
{
	CVmEventParameter *pParam = m_pProtoPackage->getEventParameter(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST);
	return (pParam && pParam->isList()
		&& CheckWhetherParamPresents(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO, PVE::String)
		&& CheckWhetherParamPresents(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID, PVE::UnsignedInt));
}

IDispToDispCommands CDispToDispResponseCommand::GetRequestCommandId()
{
	return ((IDispToDispCommands)GetUnsignedIntParamValue(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID));
}

PRL_RESULT CDispToDispResponseCommand::GetRetCode()
{
	return (m_pProtoPackage->getEventCode());
}

SmartPtr<CVmEvent> CDispToDispResponseCommand::GetErrorInfo()
{
	SmartPtr<CVmEvent> pVmEvent(new CVmEvent);
	pVmEvent->fromString(GetStringParamValue(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO));
	return (pVmEvent);
}

void CDispToDispResponseCommand::AddParam(const QString &sParamValue)
{
	CVmEventParameter *pParam = m_pProtoPackage->getEventParameter(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST);
	if (pParam && pParam->isList())
	{
		QStringList lst = pParam->getValuesList()<<sParamValue;

		m_pProtoPackage->m_lstEventParameters.removeAll(pParam);
		delete pParam;

		SetStringListParamValue( lst, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST);
	}
	// FIXME to sandro: ??? WHAT DO NOTHING WHEN pParam does not exists ?
}

QStringList CDispToDispResponseCommand::GetParams()
{
	CVmEventParameter *pParam = m_pProtoPackage->getEventParameter(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST);
	if (pParam && pParam->isList())
		return (pParam->getValuesList());
	return (QStringList());
}

}//namespace Parallels
