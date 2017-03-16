/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
///		AutoHelper.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Helpers to auto delete vm/ restore settings etc.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "AutoHelpers.h"

void AutoRestorer::renameDirBack( SmartPtr<CVmEvent> pParams)
{
	QDir vmDir;
	CVmEventParameter* pFrom = pParams->getEventParameter( "renamed" );
	CVmEventParameter* pTo = pParams->getEventParameter( "original" );
	PRL_ASSERT(pFrom);
	PRL_ASSERT(pTo);

	QVERIFY( vmDir.rename( pFrom->getParamValue(), pTo->getParamValue() ) );
}

void RestoreCommonPrefs::operation()
{
	SdkHandleWrap hJob(PrlSrv_CommonPrefsBeginEdit(m_hServerHandle));
	CHECK_JOB_RET_CODE(hJob);
	hJob.reset(PrlSrv_CommonPrefsCommit(m_hServerHandle, m_hDispConfig));
	CHECK_JOB_RET_CODE(hJob);

}

AutoRestoreCommonPrefs::AutoRestoreCommonPrefs( SdkHandleWrap hServerHandle )
{
	SdkHandleWrap hDispConfigToRevert;

	GET_RESULT_AFTER_ASYNC_CALL( (PrlSrv_GetCommonPrefs(hServerHandle)), hDispConfigToRevert );

	SmartPtr<RollbackOperation> pRollback( new RestoreCommonPrefs(hServerHandle, hDispConfigToRevert));

	m_autoRestorer.addRollback( pRollback );
}

RestoreLicenseCreds::RestoreLicenseCreds( SdkHandleWrap hServerHandle )
	:m_hServerHandle(hServerHandle), username(""), company("")
{
	SdkHandleWrap m_ServerHandle( hServerHandle );

	GET_LICENSE_INFO;

	QByteArray FieldValue;
	PRL_UINT32 BufferSize = 0;
	PRL_RESULT nRetCode = 0;

	nRetCode = PrlLic_GetUserName(hLicense, 0, &BufferSize);
	if ( PRL_SUCCEEDED( nRetCode ) && BufferSize ) {
		FieldValue.resize( BufferSize );
		nRetCode = PrlLic_GetUserName(hLicense, FieldValue.data(), &BufferSize);
		if ( PRL_SUCCEEDED( nRetCode ) )
			username = UTF8_2QSTR(FieldValue);
	}

	FieldValue.clear();
	BufferSize = 0;

	nRetCode = PrlLic_GetCompanyName(hLicense, 0, &BufferSize);
	if ( PRL_SUCCEEDED( nRetCode ) && BufferSize ) {
		FieldValue.resize( BufferSize );
		nRetCode = PrlLic_GetCompanyName(hLicense, FieldValue.data(), &BufferSize);
		if ( PRL_SUCCEEDED( nRetCode ) )
			company = UTF8_2QSTR(FieldValue);
	}
}

void RestoreLicenseCreds::operation()
{
	SET_LICENSE_USER_AND_COMPANY(m_hServerHandle, username, company)
}

AutoRestoreLicenseCreds::AutoRestoreLicenseCreds( SdkHandleWrap hServerHandle )
{
	SmartPtr<RollbackOperation> pRollback( new RestoreLicenseCreds(hServerHandle));

	m_autoRestorer.addRollback( pRollback );
}

AutoDirRemover::~AutoDirRemover()
{
	foreach( QString path, m_lstToDelete )
	{
		if( !CFileHelper::ClearAndDeleteDir( path ) )
			WRITE_TRACE(DBG_FATAL, "Unable to delete dir '%s'", QSTR2UTF8(path) );
		;
	}
}

bool AutoDirRemover::add( const QString& dirPath )
{
	PRL_ASSERT( QFileInfo( dirPath ).isDir() );
	if( !QFileInfo( dirPath ).isDir() )
		return false;

	PRL_ASSERT( !m_lstToDelete.contains( dirPath ) );
	m_lstToDelete.append( dirPath );

	return true;
}

bool AutoDirRemover::del( const QString& dirPath )
{
	foreach( QString path, m_lstToDelete )
	{
		if( dirPath != path )
			continue;
		m_lstToDelete.removeAll( path );
		return true;
	}
	return false;
}

void AutoStopVm::finalize()
{
		PRL_RESULT nRetCode;
		SdkHandleWrap hResult, hJob, hVmInfo;
		////////////////////////////////////////////////////
		do
		{
			nRetCode = PRL_ERR_FAILURE;
			// check vm state to prevent test failure
			hJob.reset(PrlVm_GetState(m_vmHandle));
			CHECK_JOB_RET_CODE(hJob);
			CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()));
			VIRTUAL_MACHINE_STATE nVmState;
			CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState));
			if( nVmState != VMS_STOPPED && nVmState != VMS_SUSPENDED )
			{
				LOG_MESSAGE( DBG_INFO, "AutoStopVm: state = %s", PRL_VM_STATE_TO_STRING( nVmState ) );
				hJob.reset(PrlVm_Stop(m_vmHandle, PRL_FALSE));
				GET_JOB_RET_CODE_EX(hJob, nRetCode);
			}
			else
				break;
		}while( PRL_FAILED(nRetCode) );
}

//////////////////////////////////////////////////////////////////////////
void AutoRestorer::restoreUserVmFolder( SmartPtr<CVmEvent> pParams)
{
	SdkHandleWrap hServer;
	SdkHandleWrap hJob;

	CVmEventParameter* pParam = pParams->getEventParameter("");
	PRL_ASSERT( pParam );
	PRL_ASSERT( pParam->getParamType() == PVE::String );
	QString vmRootDir = pParam->getParamValue();

	PRL_ASSERT( vmRootDir.isEmpty() || QDir(vmRootDir).exists() );

	///////////////
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()));
	///////////////////////////////////////////////////////////
	// 6. login
	hJob.reset(PrlSrv_LoginLocal(hServer, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 3. register folder A as user defined vm folder
	hJob.reset(PrlSrv_UserProfileBeginEdit( hServer ));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hUserProfile;
	RECEIVE_USER_PROFILE( hServer, hUserProfile);

	QVERIFY( PRL_SUCCEEDED(PrlUsrCfg_SetDefaultVmFolder( hUserProfile, QSTR2UTF8(vmRootDir) ) ) );

	hJob.reset(PrlSrv_UserProfileCommit( hServer, hUserProfile ));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 4. logoff
	hJob.reset(PrlSrv_Logoff(hServer));
	CHECK_JOB_RET_CODE(hJob);
}


