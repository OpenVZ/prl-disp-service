////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Mixin_CreateVmSupport.h
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

#include "Tasks/Mixin_CreateVmSupport.h"
#include "CDspVmDirManager.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include "CDspService.h"

#include <QProcess>

PRL_RESULT Mixin_CreateVmSupport::setDefaultVmPermissions(
	SmartPtr<CDspClient> pUser,
	QString vmPathToConfig,
	bool bKeepOthersPermissions
)
{
	PRL_ASSERT( pUser );
	PRL_ASSERT( ! vmPathToConfig.isEmpty() );

	if( ! pUser || vmPathToConfig.isEmpty() )
		return PRL_ERR_INVALID_ARG;

	// FIXME: isFsSupportPermsAndOwner() should be moved near setFullAccessRightsToVm() and united with it.
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &pUser->getAuthHelper() );

		// # https://bugzilla.sw.ru/show_bug.cgi?id=112901
		// https://jira.sw.ru/browse/PSBM-9040
		if( !CFileHelper::isFsSupportPermsAndOwner( vmPathToConfig ) )
		{
			WRITE_TRACE(DBG_FATAL, "File system does not support permissions. Setting default Vm permission will be ignored. (path=%s)"
				, QSTR2UTF8( vmPathToConfig ) );
			return PRL_ERR_SUCCESS;
		}
	}

	// get vm directory path

	// To prevent stupid errors see https://bugzilla.sw.ru/show_bug.cgi?id=483700 for more details
	PRL_ASSERT(QFileInfo( vmPathToConfig ).isFile());
	QString strVmDirPath;
	if ( QFileInfo( vmPathToConfig ).isFile() )
		strVmDirPath = CFileHelper::GetFileRoot( vmPathToConfig );
	else
	{
		strVmDirPath = vmPathToConfig;
		vmPathToConfig = QString("%1/%2").arg(strVmDirPath).arg(VMDIR_DEFAULT_VM_CONFIG_FILE);
	}

	if ( !CDspAccessManager::setOwner( strVmDirPath, &pUser->getAuthHelper(), true )  )
	{
		WRITE_TRACE(DBG_FATAL, "Can't change owner to vm dir files [%s]. error=[%s]"
			, QSTR2UTF8( strVmDirPath )
			, QSTR2UTF8( Prl::GetLastErrorAsString() ) );
		return PRL_ERR_CANT_CHANGE_OWNER_OF_FILE;
	}

	//////////////////////////////////////////////////////////////////////////
	// set default access rigths
	//////////////////////////////////////////////////////////////////////////
	CVmDirectoryItem vmDirItem;
	vmDirItem.setVmHome( vmPathToConfig );
	PRL_SEC_AM ownerAccess = CDspAccessManager::VmAccessRights::makeModeRWX() ;
	PRL_SEC_AM otherAccess = CDspAccessManager::VmAccessRights::makeModeNO_ACCESS() ;

	PRL_RESULT err = CDspService::instance()->getAccessManager()
		.setFullAccessRightsToVm( pUser, &vmDirItem, &ownerAccess,
		// Do not change others permissions during registration at desktop mode
		// https://bugzilla.sw.ru/show_bug.cgi?id=120191
		 ! bKeepOthersPermissions ? &otherAccess : NULL );
	if( PRL_FAILED( err ) )
	{
		WRITE_TRACE(DBG_FATAL, "Can't change permission to vm dir files [%s]. error=[%s]"
			, QSTR2UTF8( strVmDirPath )
			, QSTR2UTF8( Prl::GetLastErrorAsString() ) );
		return PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS;
	}

	return PRL_ERR_SUCCESS;
}
