////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Task_GetFileSystemEntries.cpp
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_GetFileSystemEntries.h"
#include "Task_CommonHeaders.h"

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include <prlcommon/Std/PrlAssert.h>
//#include <prlcommon/ProtoSerializer/CProtoCommands.h>

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

Task_GetFileSystemEntries::Task_GetFileSystemEntries (
    SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
	const QString& target,
	CHwFileSystemInfo* fs_info)
:Task_FileSystemEntriesOperations(client, p),
   fs_info(fs_info), target(target)
{
}

CHwFileSystemInfo* Task_GetFileSystemEntries::getResult()
{
   return fs_info;
}

PRL_RESULT Task_GetFileSystemEntries::run_body()
{
	PRL_RESULT err = PRL_ERR_SUCCESS;
	err = Task_FileSystemEntriesOperations::getFileSystemEntries(target, fs_info);
	if ( PRL_FAILED(err) )
	{
		getLastError()->setEventCode(err);
	}
	return err;
}

void Task_GetFileSystemEntries::finalizeTask()
{
	CHwFileSystemInfo* pFsInfo = getResult();

	if( IS_OPERATION_SUCCEEDED( getLastErrorCode() ) && pFsInfo )
	{
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* wsResponse = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		wsResponse->SetHwFileSystemInfo( pFsInfo->toString() );
		getClient()->sendResponse( pResponse, getRequestPackage() );
	}
	else
	{
		PRL_ASSERT(pFsInfo);
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}

	if( pFsInfo )
	{
		delete pFsInfo;
		pFsInfo = NULL;
	}
}
