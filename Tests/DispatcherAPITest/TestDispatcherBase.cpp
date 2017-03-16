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
///		TestDispatcherBase.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Base tests fixture class for all tests suites of dispatcher API commands functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDispatcherBase.h"
#include <QtTest/QtTest>

#include <prlcommon/Interfaces/ParallelsQt.h>

#include "SDK/Handles/PveControl.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/DispConfig/CDispUser.h>
#include <prlxmlmodel/DispConfig/CDispUserWorkspace.h>

#include <prlxmlmodel/VmDirectory/CVmDirectories.h>
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>

#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>


#include "Tests/CommonTestsUtils.h"

TestDispatcherBase::TestDispatcherBase()
:m_pPveControl(0), m_pHandler(new CMockPveEventsHandler())
{}

TestDispatcherBase::~TestDispatcherBase()
{}

bool TestDispatcherBase::IsValid()
{
   return m_pPveControl && m_pHandler;
}

bool TestDispatcherBase::Login()
{
   CResult _result;
   try
   {
	   if(!IsValid())
         throw "object isn't valid";
	   CALL_CMD(m_pPveControl->DspCmdUserLogin( TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
												TestConfig::getUserPassword(), NULL, PSL_HIGH_SECURITY,
												CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	   _result = m_pHandler->GetResult();
	   if (PRL_FAILED(_result.getReturnCode()))
         throw "bad result returnCode";
   }
   catch (const char* s)
   {
      WRITE_TRACE(DBG_FATAL, "error in Login() [%s] [retCode=%.8X]", s?s:"", _result.getReturnCode());
      return false;
   }

   return true;
}

bool TestDispatcherBase::Logoff()
{
   CResult _result;
   try
   {
	   if(!IsValid())
         throw "object isn't valid";
	  CALL_CMD(m_pPveControl->DspCmdUserLogoff(), PVE::DspCmdUserLogoff)
      _result = m_pHandler->GetResult();
	   if(PRL_FAILED(_result.getReturnCode()))
         throw "bad result returnCode";
   }
   catch (const char* s)
   {
      WRITE_TRACE(DBG_FATAL, "error in Logoff() [%s] [retCode=%.8X]", s?s:"", _result.getReturnCode());
      return false;
   }
   return true;

}

SmartPtr<CVmDirectory> TestDispatcherBase::GetUserVmDirectory()
{

	//FIXME: bug #2242 getUserProfile() return path intsead uuid of VmDirectory
	//  ==> we use direct access to dispatcher xml
	SmartPtr<CVmDirectory> pVmDir;
	try
	{
		QFile fileVmDirs( ParallelsDirs::getDispatcherVmCatalogueFilePath() );
		CVmDirectories vmDirs ( &fileVmDirs );
		if ( PRL_FAILED( vmDirs.m_uiRcInit ) )
		{
			WRITE_TRACE(DBG_FATAL, " CVmDirectories init failed");
			throw PRL_RESULT_TO_STRING( vmDirs.m_uiRcInit );
		}

		QFile fileDispConfig( ParallelsDirs::getDispatcherConfigFilePath() );
		CDispatcherConfig dispConf ( &fileDispConfig );
		if ( PRL_FAILED( dispConf.m_uiRcInit ) )
		{
			WRITE_TRACE(DBG_FATAL, " CDispatcherConfig init failed");
			throw PRL_RESULT_TO_STRING( dispConf.m_uiRcInit );
		}

		SmartPtr<CDispUser> pUser = GetUserProfile();
		if ( ! pUser )
			throw "GetUserProfile() failed.";
		QString userUuid = pUser->getUserId();

		QString VmDirId;
		foreach( CDispUser* p, dispConf.getDispatcherSettings()
			->getUsersPreferences()->m_lstDispUsers)
		{
			if ( p->getUserId() != userUuid )
				continue;
			VmDirId = p->getUserWorkspace()->getVmDirectory();
			break;
		}

		if ( VmDirId.isEmpty() )
			throw "Can't found VmDirId for current user";

		foreach( CVmDirectory* p, *vmDirs.getVmDirectoriesList() )
		{
			if ( p->getUuid() != VmDirId )
				continue;

			pVmDir = SmartPtr<CVmDirectory>(new CVmDirectory( p ));
			if ( PRL_FAILED( pVmDir->m_uiRcInit ) )
				throw "PRL_FAILED( pVmDir->m_uiRcInit )";
			break;
		}

		if ( ! pVmDir )
			throw "Can't found vmdir by uuid ";
	}
	catch ( const char* s)
	{
		WRITE_TRACE(DBG_FATAL, "error catched: [ %s ]", s );
		return SmartPtr<CVmDirectory>();
	}

	return pVmDir;
}

SmartPtr<CDispUser> TestDispatcherBase::GetUserProfile()
{
	SmartPtr<CDispUser> pUser;

	try
	{
		CALL_CMD(m_pPveControl->DspCmdUserGetProfile(), PVE::DspCmdUserGetProfile)

		CResult _result = m_pHandler->GetResult();
		if ( PRL_FAILED( _result.getReturnCode() ) )
			throw PRL_RESULT_TO_STRING( _result.getReturnCode() );

		pUser = SmartPtr<CDispUser>(new CDispUser(
                            _result.m_hashResultSet[PVE::DspCmdUserGetProfile_strUserProfile] ));
		if ( PRL_FAILED( pUser->m_uiRcInit ) )
			throw PRL_RESULT_TO_STRING( pUser->m_uiRcInit );

	}
	catch ( const char* s)
	{
		WRITE_TRACE(DBG_FATAL, "error catched: [ %s ]", s);
		return SmartPtr<CDispUser>();
	}
	return pUser;
}

QString TestDispatcherBase::QueryUserVmDirPath()
{
	QString sRes;

	SmartPtr<CDispUser> pUserProfile = GetUserProfile();
	if ( ! pUserProfile )
		return "";

	//FIXME: bug #2242 getUserProfile() return path intsead uuid of VmDirectory
	QString strVmDirCommon = pUserProfile->getUserWorkspace()->getVmDirectory();
	QString strVmDirUser = pUserProfile->getUserWorkspace()->getDefaultVmFolder();
	sRes = strVmDirUser.isEmpty()
		? strVmDirCommon
		: strVmDirUser;

	return sRes;
}
