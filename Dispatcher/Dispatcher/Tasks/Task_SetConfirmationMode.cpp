///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_SetConfirmationMode.cpp
///
/// Dispatcher task for
///
/// @author sergeyt
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
/////////////////////////////////////////////////////////////////////////////////

#include "Task_SetConfirmationMode.h"
#include "Task_CommonHeaders.h"
#include "CProtoSerializer.h"
#include <prlcommon/Std/PrlAssert.h>
//#include "Interfaces/ParallelsNamespace.h"
#include "Interfaces/ParallelsSdkPrivate.h"
#include "Build/Current.ver"

#define FLAG_CONTAINS_IN_BITMASK( flag, bitmask )  ( (flag & bitmask) == flag )

using namespace Parallels;

Task_SetConfirmationMode::Task_SetConfirmationMode(SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg)
: CDspTaskHelper(pUser, pRequestPkg)
{}

PRL_RESULT Task_SetConfirmationMode::run_body()
{
	QString user;
	QString pswd;
	CProtoCommandPtr pCmd;
	PRL_RESULT result = PRL_ERR_SUCCESS;

	try
	{
		pCmd = CProtoSerializer::ParseCommand( getRequestPackage() );
		if ( ! pCmd->IsValid() )
			throw PRL_ERR_FAILURE;

		CProtoCommandDspCmdSetSessionConfirmationMode*
			pCmdConfirm = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdSetSessionConfirmationMode>(pCmd);

		user = pCmdConfirm->GetUserLoginName();
		pswd = pCmdConfirm->GetPassword();
		bool bEnableConfirmationMode = pCmdConfirm->GetEnabledSign();

		if( getClient()->isConfirmationEnabled() && bEnableConfirmationMode )
			throw PRL_ERR_CONFIRMATION_MODE_ALREADY_ENABLED;
		if( !getClient()->isConfirmationEnabled() && !bEnableConfirmationMode )
			throw PRL_ERR_CONFIRMATION_MODE_ALREADY_DISABLED;

		if( ! bEnableConfirmationMode )
		do
		{
			CAuthHelper auth( user );
			if( !auth.AuthUser( pswd ) )
				throw PRL_ERR_AUTHENTICATION_FAILED;

			if( !auth.isLocalAdministrator() )
				throw PRL_ERR_CONFIRMATION_MODE_UNABLE_CHANGE_BY_NOT_ADMIN;
		}while(0);

		getClient()->setConfirmationMode( bEnableConfirmationMode );

		WRITE_TRACE( DBG_WARNING, "Confirmation mode was %s for this session"
			, bEnableConfirmationMode ? "ENABLED" : "DISABLED" );
	}
	catch( PRL_RESULT err )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to change confirmation mode with user '%s' by error %s ( %#x )"
			, QSTR2UTF8( user )
			, PRL_RESULT_TO_STRING(err)
			, err );
		result = err;
	}

	setLastErrorCode( result );
	return result;
}

void Task_SetConfirmationMode::finalizeTask()
{
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->AddStandardParam(QString("%1").arg( getClient()->isConfirmationEnabled() ));

	getClient()->sendResponse( pCmd, getRequestPackage() );
}
