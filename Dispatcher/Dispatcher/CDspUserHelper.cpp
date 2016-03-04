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
///  CDspUserHelper.cpp
///
/// @brief
///  Implementation of the class CDspUserHelper
///
/// @brief
///  This class implements user dispatching logic
///
/// @author sergeyt
///  SergeyM
///
/// @date
///  2006-04-04
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include <QDir>
#include <QTimer>

#include <prlcommon/Interfaces/ParallelsQt.h>

#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "CProtoSerializer.h"
#include "CProtoCommands.h"
#include "CDspService.h"
#include "CDspClientManager.h"
#include "CDspUserHelper.h"

#include "Libraries/PrlCommonUtils/CFileHelper.h"

#include <prlxmlmodel/DispConfig/CDispWorkspacePreferences.h>
#include "CDspCommon.h"

#include <prlcommon/IOService/IOCommunication/IOSSLInterface.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Logging/Logging.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/HostHardwareInfo/CHwOsVersion.h>
#include <prlxmlmodel/UserInformation/ParallelsUserInformation.h>
#include <prlxmlmodel/DispConfig/CDispUserSettings.h>
#include <prlxmlmodel/Messaging/CVmBinaryEventParameter.h>

#include <prlcommon/Std/PrlAssert.h>
#include "Build/Current.ver"
#include <prlcommon/PrlCommonUtilsBase/CGuestOsesHelper.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>

#define LOGIN_LOCAL_TIMEOUT_SEC 30

using namespace Parallels;

///////////////////////////////////////////////////////////////////////////////

CUserLoginLocalHelper::CUserLoginLocalHelper (
    SmartPtr<CDspClient>& pUser, quint64 processId )
    /* throw (const char *) */
	: m_pUser(pUser), m_pid( processId )
{
	QString
		authDirPath
#ifndef	_WIN_
		= QDir::tempPath();


#else
		// #265833 To prevent unable to authorize the user on some Vista with enabled UAC (from HP for example).
			//	QDir::tempPath() always returns WINDOWS/Temp
			// and dispatcher can't create file under Impersonate in this case.
		= ParallelsDirs::getDispatcherConfigDir();
#endif

	m_sCheckFilePath =  authDirPath  + "/" + Uuid::createUuid().toString();
	WRITE_TRACE(DBG_DEBUG, "Authorization file path is '%s'", QSTR2UTF8(m_sCheckFilePath));
	m_sCheckData = Uuid::createUuid().toString();

	if (CFileHelper::CreateBlankFile(m_sCheckFilePath, &pUser->getAuthHelper()))
    {
        if (!QFile(m_sCheckFilePath).setPermissions(QFile::ReadOwner | QFile::WriteOwner))
		{
			QFile::remove(m_sCheckFilePath);
			WRITE_TRACE(DBG_FATAL, "Couldn't to set necessary permissions to authorization file '%s'"
				, QSTR2UTF8(m_sCheckFilePath));
            throw PRL_ERR_COULDNT_SET_PERMISSIONS_TO_AUTHORIZATION_FILE;
		}
    }
    else
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to create an authorization file '%s'", QSTR2UTF8(m_sCheckFilePath));
        throw PRL_ERR_COULDNT_CREATE_AUTHORIZATION_FILE;
	}
}

CUserLoginLocalHelper::~CUserLoginLocalHelper()
{
    QFile::remove(m_sCheckFilePath);
}

bool CUserLoginLocalHelper::CheckValidity()
{
    QFile _file(m_sCheckFilePath);
    if (_file.open(QIODevice::ReadOnly))
    {
        if (_file.size() == m_sCheckData.size())
        {
            QTextStream _stream(&_file);
            QString sActualData = _stream.readAll();
            if (m_sCheckData == sActualData)
                return (true);
            else
                WRITE_TRACE(DBG_FATAL, "File data do not match necessary check data: expected '%s' actual '%s'"
					, m_sCheckData.toUtf8().data(), sActualData.toUtf8().data());
        }
        else
            LOG_MESSAGE(DBG_FATAL, "");
    }
    else
        WRITE_TRACE(DBG_FATAL, "Couldn't to open check user validity file '%s'", m_sCheckFilePath.toUtf8().data());
    return (false);
}

///////////////////////////////////////////////////////////////////////////////

CDspUserHelper::CDspUserHelper ()
{
	bool bConnected = connect( this, SIGNAL(chargeLoginLocalHelpersCleaner())
		, SLOT(onChargeLoginLocalHelpersCleaner()), Qt::QueuedConnection );
	PRL_ASSERT(bConnected);
}

CDspUserHelper::~CDspUserHelper ()
{
}

/**
* @brief Setup user's defaults.
* @param p_user
*/
bool CDspUserHelper::setupUserDefaults ( CDispUser* pDispUser,
												CAuthHelper& authHelper)
{

    ////////////////////////////////////////////////////////////////////////
    // initialize user default settings
    ////////////////////////////////////////////////////////////////////////


	// make snapshot to prevent deadlock between getDispConfigGuard() mutex and CDspVmDirManager mutex.
    SmartPtr<CDispWorkspacePreferences> pDispPreferences( new CDispWorkspacePreferences(
		CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs().getPtr() ));

	CDispUserWorkspace *pDispUserWorkspace = pDispUser->getUserWorkspace();
    if ( pDispUserWorkspace )
    {
        if (!pDispPreferences->isDistributedDirectory())
            pDispUserWorkspace->setVmDirectory(pDispPreferences->getDefaultVmDirectory());
        else
        {
            ParallelsDirs::UserInfo info = authHelper.getParallelsDirUserInfo();

            QString vmdir=ParallelsDirs::getUserDefaultVmCatalogue(&info);

			// try to create when last dir in the path does not exists
			if( vmdir.isEmpty() || !CFileHelper::DirectoryExists(vmdir, &authHelper) )
            {
				bool bVmDirCreated = false;
				QString parentDir = QFileInfo( vmdir ).dir().absolutePath();
				if( !vmdir.isEmpty() && CFileHelper::DirectoryExists( parentDir, &authHelper ) )
				{
					if( CFileHelper::CreateDirectoryPath( vmdir, &authHelper ) )
						bVmDirCreated = true;
					else
					{
						WRITE_TRACE(DBG_FATAL, "Unable to create vm directory for user %s by path %s"
							,	QSTR2UTF8( authHelper.getUserFullName() )
							,	QSTR2UTF8( vmdir )
							);
					}
				}

				if( !bVmDirCreated )
				{
					// BUGFIX: #115878 * Fix to allow logon user without home folder.

					WRITE_TRACE(DBG_FATAL, "Default vm directory for user %s is empty, "
						"or not exist or user hasn't access to it. path = '%s' "
						"Default vm directory will be placed in common vm directory."
						, QSTR2UTF8( authHelper.getUserFullName() )
						, QSTR2UTF8( vmdir )
						);

					CDspLockedPointer<CVmDirectory>
						pCommonVmDir = CDspService::instance()->getVmDirManager()
							.getVmDirectory( pDispPreferences->getDefaultVmDirectory() );
					PRL_ASSERT( pCommonVmDir );
					if( !pCommonVmDir )
						return false;
					vmdir = pCommonVmDir->getDefaultVmFolder();
				}//if( !bVmDirCreated )
            }

			QString vmDirUuid = CDspService::instance()->getVmDirHelper()
				.CreateVmDirCatalogueEntry( vmdir, authHelper.getUserName() + " vm home");
            if ( vmDirUuid.isEmpty() )
				return false;

            pDispUserWorkspace->setVmDirectory( vmDirUuid );
        }
    }

    CDispUserAccess *pDispUserAccess = pDispUser->getUserAccess();
    if (pDispUserAccess)
    {
        pDispUserAccess->setUseServerManagementConsoleFlag( pDispPreferences->isDefaultUseConsole() );
        pDispUserAccess->setChangeServerSettingsFlag( pDispPreferences->isDefaultChangeSettings() );
    }

#ifdef _LIN_
	pDispUser->getProxySettings()->setUseProxy( false );
#endif

#ifdef _WIN_
	pDispUser->getProxySettings()->getSystemSettings()->setEnabled( true );
#endif
	return true;
}

bool CDspUserHelper::fillUserPreferences (
    const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p,
    SmartPtr<CDspClient>& p_NewUser )
{
    QString user_name = p_NewUser->getAuthHelper().getUserFullName();

    //////////////////////////////////////////////////////////////////////////

    // set user state (access token is generated automatically)
    p_NewUser->setUserState( PVE::UserConnectedLoggedOn );

    /**
     * trying to find user's registration record in the Dispatcher's config
     */

	SmartPtr<CDispUser> pDispUser(0);
	{
		CDspLockedPointer< CDispUser >
			pLockedDispUser = CDspService::instance()->getDispConfigGuard().getDispUserByName( user_name );
		if( pLockedDispUser )
			pDispUser = SmartPtr<CDispUser>( new CDispUser (pLockedDispUser.getPtr() ) );
	}

	if( pDispUser ) // user found
	{
		p_NewUser->setUserSettings( pDispUser->getUserId(), user_name );
		p_NewUser->setVmDirectoryUuid( pDispUser->getUserWorkspace()->getVmDirectory() );

		// #445937 rename path to users vm directory default folder if it neccessery
		QString sCommonVmDirUuid = CDspService::instance()->getDispConfigGuard()
			.getDispWorkSpacePrefs()->getDefaultVmDirectory();

		QString sUsersVmDirUuid = pDispUser->getUserWorkspace()->getVmDirectory();
		if( sCommonVmDirUuid != sUsersVmDirUuid )
		{
			// it's not common vm directory ( it may be users vm directory )
			CDspLockedPointer<CVmDirectory> pCommonVmDir = CDspService::instance()->getVmDirManager()
				.getVmDirectory( sCommonVmDirUuid );
			CDspLockedPointer<CVmDirectory> pUserVmDir = CDspService::instance()->getVmDirManager()
				.getVmDirectory( sUsersVmDirUuid );

			PRL_ASSERT( pCommonVmDir );
			PRL_ASSERT( pUserVmDir );

			if( pCommonVmDir->getDefaultVmFolder() != pUserVmDir->getDefaultVmFolder() )
			{
				ParallelsDirs::UserInfo info = p_NewUser->getAuthHelper().getParallelsDirUserInfo();
				QString newVmDirPath = ParallelsDirs::getUserDefaultVmCatalogue(&info);
				if( !newVmDirPath.isEmpty() && newVmDirPath != pUserVmDir->getDefaultVmFolder() )
				{
					WRITE_TRACE( DBG_FATAL, "Patch path for user vm catalogue (%s):\n"
						"'%s' ==> '%s'"
						, QSTR2UTF8( pUserVmDir->getUserFriendlyName() )
						, QSTR2UTF8( pUserVmDir->getDefaultVmFolder() )
						, QSTR2UTF8( newVmDirPath )
						);
					pUserVmDir->setDefaultVmFolder( newVmDirPath );
				}

			}
		}
	}
	else // user not found (a new user came out)
    {
        // create a new record for the user in registry
        CDispUser* p_UserItem = new CDispUser();
        p_UserItem->setUserName( user_name );
		p_UserItem->setUserId( Uuid::createUuid().toString() );

        // setup user defaults
        if(!setupUserDefaults( p_UserItem, p_NewUser->getAuthHelper() ) )
        {
            WRITE_TRACE(DBG_FATAL, "setupUserDefaults() for user [%s] return failed.", QSTR2UTF8(user_name));
            delete p_UserItem;

			// Send error
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_UNEXPECTED );
            return false;
        }

        // adjust user settings
        p_NewUser->setUserSettings( p_UserItem->getUserId(), user_name );
		p_NewUser->setVmDirectoryUuid( p_UserItem->getUserWorkspace()->getVmDirectory() );

		//LOCK dispConfig
		CDspLockedPointer< CDispUser >
			pLockedDispUser = CDspService::instance()->getDispConfigGuard().getDispUserByName( user_name );
		// check to exist user ( may be added from another thread )
		if( pLockedDispUser )
		{
			//FOUND !!!
			p_NewUser->setUserSettings( pLockedDispUser->getUserId(), user_name );
		}
		else
		{
			// add new record to Dispatcher's configuration
			CDspService::instance()->getDispConfigGuard().getDispSettings()
				->getUsersPreferences()->addUser( p_UserItem );

			// save config file
			PRL_RESULT save_rc = CDspService::instance()->getDispConfigGuard().saveConfig();

			if( PRL_FAILED( save_rc ) )
			{
				WRITE_TRACE(DBG_FATAL, "Unable to write data to disp configuration file by error %s"
					, PRL_RESULT_TO_STRING(save_rc) );

				CDspService::instance()->getDispConfigGuard().getDispSettings()->getUsersPreferences()
					->removeUser(p_UserItem);
				delete p_UserItem;

				// Send error
				CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_DISP_CONFIG_WRITE_ERR );
				return false;
			}
		}
    }

    /**
     * Try to create default VM directory for the user
     */
    if( ! CDspService::instance()->getVmDirHelper().createUsersVmDirectory( p_NewUser ) )
    {
        WRITE_TRACE(DBG_FATAL, "WARNING: Unable to create VM directory for user [%s]",
					  user_name.toUtf8().data() );

		// #444668 Don't send error  - allow to start session
    }

    return true;
}

void CDspUserHelper::processLoginLocalHash ()
{
    QMutexLocker _lock(&m_LoginLocalHelpersMutex);
    QList<QString> _timedout_helpers;
    QHash<QString, CUserLoginLocalHelper *>::const_iterator _helper = m_LoginLocalHelpers.begin();
    for(; _helper != m_LoginLocalHelpers.end(); ++_helper)
    {
        if (_helper.value()->GetUser()->getSessionUptimeInSec() >= LOGIN_LOCAL_TIMEOUT_SEC)
            _timedout_helpers.append(_helper.key());
    }
    foreach(QString sHelperId, _timedout_helpers)
        SmartPtr<CUserLoginLocalHelper> _helper( m_LoginLocalHelpers.take(sHelperId) );
}

void CDspUserHelper::processUserLoginLocal (
    const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_FAILURE );
		return;
	}

	CProtoCommandDspCmdUserLoginLocal* loginCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdUserLoginLocal>(cmd);

    QMutexLocker _lock(&m_LoginLocalHelpersMutex);
    if (m_LoginLocalHelpers.contains(h))
    {
		// Send error
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_ACCESS_TOKEN_INVALID );
        return;
    }

    /**
     * create an instance of user's object
     */

    SmartPtr<CDspClient> p_NewUser( new CDspClient( h ) );
    /**
     * check if user is valid and authorized
     */

    if (!p_NewUser->getAuthHelper().AuthUser(loginCmd->GetUserId()))
    {
        WRITE_TRACE(DBG_FATAL, "Can't authorize user with id [%d].",
					  loginCmd->GetUserId());

		// Send error
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_LOCAL_AUTHENTICATION_FAILED );
        return;
    }
    try
    {
        p_NewUser->setLocal( true );
	p_NewUser->setNonInteractive(loginCmd->GetCommandFlags() & PACF_NON_INTERACTIVE_MODE);
		CUserLoginLocalHelper *pLoginLocalHelper =
			new CUserLoginLocalHelper(p_NewUser, loginCmd->GetProcessId() );
        m_LoginLocalHelpers.insert(h, pLoginLocalHelper);
		emit chargeLoginLocalHelpersCleaner();


        /**
         * reply to user
         */

		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* response =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

		response->AddStandardParam( pLoginLocalHelper->GetCheckFilePath() );
		response->AddStandardParam( pLoginLocalHelper->GetCheckData() );
		SmartPtr<IOPackage> responsePkg =
			DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );
		CDspService::instance()->getIOServer().sendPackage( h, responsePkg );
    }
    catch (PRL_RESULT e)
    {
        WRITE_TRACE(DBG_FATAL, "Error occured on user check validity preparation: '%.8X'", e);
		// Send error
		CDspService::instance()->sendSimpleResponseToClient( h, p, e );
        return;
    }
}

void CDspUserHelper::onChargeLoginLocalHelpersCleaner()
{
	QTimer::singleShot(LOGIN_LOCAL_TIMEOUT_SEC * 1000, this, SLOT(processLoginLocalHash()));
}

SmartPtr<CDspClient> CDspUserHelper::processUserLoginLocalStage2 (
    const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_FAILURE );
		return SmartPtr<CDspClient>();
	}

	CProtoCommandWithOneStrParam* loginCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandWithOneStrParam>(cmd);
	QString prevSessionUuid = loginCmd->GetFirstStrParam();

#ifdef SENTILLION_VTHERE_PLAYER
	if ( ! isSentillionClient(prevSessionUuid) )
	{
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_NOT_SENTILLION_CLIENT );
		return SmartPtr<CDspClient>();
	}
#endif	// SENTILLION_VTHERE_PLAYER

	SmartPtr<CDspClient> user(0);
	quint64 userPid = 0;
	{
		QMutexLocker _lock(&m_LoginLocalHelpersMutex);
		if (!m_LoginLocalHelpers.contains(h))
		{
			// Send error
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_ACCESS_TOKEN_INVALID );
			return SmartPtr<CDspClient>();
		}

		SmartPtr<CUserLoginLocalHelper> pLoginLocalHelper( m_LoginLocalHelpers.take(h) );
		if (!pLoginLocalHelper->CheckValidity())
		{
				// Send error
				CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_LOCAL_AUTHENTICATION_FAILED );
				return SmartPtr<CDspClient>();
		}
		// FIXME - may be it need . it comment because old client wait disconnect signal from
		// transport and new client may try to connect early
		/*if ( CDspService::instance()->getClientManager().getUserSession( prevSessionUuid ) )
		{
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_PREV_SESSION_IS_ACTIVE );
			return SmartPtr<CDspClient>();
		}*/
		user = pLoginLocalHelper->GetUser();
		userPid = pLoginLocalHelper->GetClientProcessId();
	}

	user->setPrevSessionUuid( prevSessionUuid );
	user->storeClientProcessInfo( (Q_PID)userPid );

	bool res = fillUserPreferences( h, p, user );

	if ( !res )
		return SmartPtr<CDspClient>();

	PRL_ASSERT( user );
	CDspLockedPointer<CDispUser>
		pLockedDispUser = CDspService::instance()->getDispConfigGuard()
		.getDispUserByUuid( user->getUserSettingsUuid() );
	if( pLockedDispUser )
	{
		WRITE_TRACE(DBG_FATAL, "Parallels user [%s] successfully logged on( LOCAL ). [sessionId = %s ]",
			QSTR2UTF8( pLockedDispUser->getUserName() ),
			QSTR2UTF8( user->getClientHandle() ) );
	}
	pLockedDispUser.unlock();

	//////////////////////////////////////////////////////////////////////////
	//
	//  NOTE: PRL_ERR_SUCCESS response should be sent from caller
	//
	//////////////////////////////////////////////////////////////////////////

	return user;
}

/**
* @brief Process dispacher's logon request.
* @param pRequestParams
* @return
*/
SmartPtr<CDspClient> CDspUserHelper::processDispacherLogin (
    const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p )
{
    /**
     * retrieve parameters from request data
     */

	CDispToDispCommandPtr cmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( ! cmd->IsValid() )
	{
		CDspService::instance()->sendSimpleResponseToDispClient( h, p, PRL_ERR_FAILURE );
		return SmartPtr<CDspClient>();
	}

	CDispToDispAuthorizeCommand *authCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(cmd);

	return processLogin(authCmd->GetUserName(), authCmd->GetPassword(), "", h , p);
}

/**
* @brief Process user's logon request.
* @param pRequestParams
* @return
*/
SmartPtr<CDspClient> CDspUserHelper::processUserLogin (
    const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p,
	bool &bWasPreAuthorized )
{
    /**
     * retrieve user parameters from request data
     */

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_FAILURE );
		return SmartPtr<CDspClient>();
	}

	CProtoCommandDspCmdUserLogin* loginCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdUserLogin>(cmd);

	return processLogin(
			loginCmd->GetUserLoginName(), loginCmd->GetPassword(), loginCmd->GetPrevSessionUuid(), h , p,
			loginCmd->GetCommandFlags(), &bWasPreAuthorized );
}

/**
* @brief Process user logon via user & password
* @param user
* @param password
* @param prevSessionUuid
* @return
*/
SmartPtr<CDspClient> CDspUserHelper::processLogin (
    const QString _user, const QString password, const QString prevSessionUuid,
	const IOSender::Handle& h, const SmartPtr<IOPackage>& p, quint32 nFlags,
	bool *
	)
{
	QString user = _user;
	WRITE_TRACE(DBG_FATAL, "Login request from user [%s]", QSTR2UTF8( user ) );

	////////////////////////////////////////////////////////////////////////
	// create an instance of user's object
	////////////////////////////////////////////////////////////////////////

	SmartPtr<CDspClient> p_NewUser( new CDspClient( h, user, nFlags ) );
	p_NewUser->setPrevSessionUuid( prevSessionUuid );
	p_NewUser->setNonInteractive(nFlags & PACF_NON_INTERACTIVE_MODE);

		if (user.isEmpty() || !p_NewUser->getAuthHelper().AuthUser(password))
		{
			WRITE_TRACE(DBG_FATAL, "Can't authorize user [%s].", user.toUtf8().data());

			// Client wasn't preauthprized as well - send error
			CDspService::instance()->sendSimpleResponseToClient( h, p, PRL_ERR_AUTHENTICATION_FAILED );
			return SmartPtr<CDspClient>();
		}

	bool res = fillUserPreferences( h, p, p_NewUser );

	if ( !res )
		return SmartPtr<CDspClient>();

	PRL_ASSERT( p_NewUser );
	CDspLockedPointer<CDispUser>
		pLockedDispUser = CDspService::instance()->getDispConfigGuard()
		.getDispUserByUuid( p_NewUser->getUserSettingsUuid() );
	if( pLockedDispUser )
	{
		WRITE_TRACE(DBG_FATAL, "Parallels user [%s] successfully logged on. [sessionId = %s ]",
			QSTR2UTF8( pLockedDispUser->getUserName() ),
			QSTR2UTF8( p_NewUser->getClientHandle() ) );
	}
	pLockedDispUser.unlock();

	//////////////////////////////////////////////////////////////////////////
	//
	//  NOTE: PRL_ERR_SUCCESS response should be sent from caller
	//
	//////////////////////////////////////////////////////////////////////////

	return p_NewUser;
}


#ifdef SENTILLION_VTHERE_PLAYER
bool CDspUserHelper::isSentillionClient(const QString& qsSessionId) const
{
	QByteArray baSessionId = QByteArray::fromBase64(SENTILLION_CLIENT_AUTH_SESSION_UUID_BASE64);
	bool bSentillionClient = (qsSessionId == UTF8_2QSTR(baSessionId));
	baSessionId.fill('0');

	return bSentillionClient;
}
#endif	// SENTILLION_VTHERE_PLAYER

/**
* @brief Process user's logoff request.
* @param pRequestParams
* @return
*/
bool CDspUserHelper::processUserLogoff (
										SmartPtr<CDspClient>& pUser,
										const SmartPtr<IOPackage>& p )
{
	{
		CDspLockedPointer<CDispUser>
			pLockedDispUser = CDspService::instance()->getDispConfigGuard()
			.getDispUserByUuid( pUser->getUserSettingsUuid() );
		if( pLockedDispUser )
		{
			WRITE_TRACE(DBG_FATAL, "Parallels user [%s] successfully logged off. [sessionId = %s ]",
				QSTR2UTF8( pLockedDispUser->getUserName() ),
				QSTR2UTF8( pUser->getClientHandle() ) );

		}
	}

	CDspService::instance()->getVmDirHelper().cleanupSessionVmLocks(pUser->getClientHandle());

	IOSendJob::Handle hJob = pUser->sendSimpleResponse( p, PRL_ERR_SUCCESS );
	CDspService::instance()->getIOServer().waitForSend(hJob);

	CDspService::instance()->getClientManager().deleteUserSession( pUser->getClientHandle() );

	return true;

}

void CDspUserHelper::setNonInteractiveSession ( SmartPtr<CDspClient>& pUser,
												const  SmartPtr<IOPackage>& p )
{
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( p );
	if ( ! pCmd->IsValid() )
	{
		pUser->sendSimpleResponse( p, PRL_ERR_INVALID_ARG );
		return;
	}

	CProtoCommandWithOneStrParam* pParam
		= CProtoSerializer::CastToProtoCommand<CProtoCommandWithOneStrParam>(pCmd);
	if ( ! pParam )
	{
		pUser->sendSimpleResponse( p, PRL_ERR_INVALID_ARG );
		return;
	}

	bool bNonInteractive = (pParam->GetFirstStrParam().toInt() != 0);

	WRITE_TRACE(DBG_FATAL, "Session was %s, session now is %s",
		pUser->isNonInteractive() ? "non-interactive" : "interactive",
		bNonInteractive ? "non-interactive" : "interactive");

	pUser->setNonInteractive(bNonInteractive);

	pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->AddStandardParam(QString("%1").arg(bNonInteractive));

	pUser->sendResponse( pCmd, p );
}

// Send user's profile
void CDspUserHelper::sendUserProfile (
									  const IOSender::Handle& h,
									  SmartPtr<CDspClient>& user,
									  const SmartPtr<IOPackage>& p)
{
	QString sUserProfile = CDspService::instance()->getDispConfigGuard()
		.getDispUserByUuidAsString(user->getUserSettingsUuid());
	if (sUserProfile.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to find user profile for UUID '%s'", user->getUserSettingsUuid().toUtf8().constData());
		user->sendSimpleResponse(p, PRL_ERR_OPERATION_FAILED);
		return;
	}

	CDispUser userProfile( sUserProfile );

	//FIXME: bug #2242 [ SDK ] May be need to add support of  multiple VmDirectories to SDK.
	// change VmDirUuid to VmDirPath
	QString vmDirPath = CDspService::instance()->getVmDirManager().getVmDirectory(user->getVmDirectoryUuid())
		->getDefaultVmFolder();
	userProfile.getUserWorkspace()->setVmDirectory( vmDirPath );

	//Fill user home path
	userProfile.getUserWorkspace()->setUserHomeFolder(user->getAuthHelper().getHomePath());

	// fill user cached proxy data
	// only for local host users!
	if ( user->isLocal() )
	{
		SmartPtr<CDispUserCachedData> pData(new CDispUserCachedData() );
		loadCachedProxyData( pData, userProfile.getUserId() );
		userProfile.getUserCachedData()->fromString( pData->toString());

		SmartPtr<CDispUserSettings> pUserSet( new CDispUserSettings( *userProfile.getProxySettings()->getUserSettings() ) );
		addUserDefinedProxySecure( pUserSet, userProfile.getUserId() );
		pUserSet->setSave( true );
		userProfile.getProxySettings()->getUserSettings()->fromString( pUserSet->toString() );
	}
	else
		userProfile.getUserCachedData()->setSave( false );

	// addinfo #8036
	if (user->getAuthHelper().isLocalAdministrator())
	{
		userProfile.getUserAccess()->setChangeServerSettingsFlag(true);
		userProfile.getUserAccess()->setLocalAdministrator(true);
	}
	else
		userProfile.getUserAccess()->setLocalAdministrator(false);

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse* response =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

	response->SetUserProfile( userProfile.toString() );
	SmartPtr<IOPackage> responsePkg =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );
	CDspService::instance()->getIOServer().sendPackage( h, responsePkg );
}

void CDspUserHelper::sendUserInfoList (
									   const IOSender::Handle& h,
									   SmartPtr<CDspClient>& user,
									   const SmartPtr<IOPackage>& p)
{
	QString sUserId = user->getUserSettingsUuid();
	{
		CDspLockedPointer<CDispUser> pDispUser =
			CDspService::instance()->getDispConfigGuard().getDispUserByUuid(sUserId);
		if (!pDispUser.isValid())
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to find user for UUID '%s'",	QSTR2UTF8(sUserId));
			user->sendSimpleResponse(p, PRL_ERR_USER_NOT_FOUND);
			return;
		}
	}

	QStringList lstUserInfo = getUserInfoList();

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

	pResponseCmd->SetParamsList( lstUserInfo );

	SmartPtr<IOPackage> responsePkg =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponseCmd, p );
	CDspService::instance()->getIOServer().sendPackage( h, responsePkg );
}

void CDspUserHelper::sendUserInfo ( const IOSender::Handle& h,
								   SmartPtr<CDspClient>& user,
								   const SmartPtr<IOPackage>& p)
{
	QString sUserId = user->getUserSettingsUuid();
	{
		CDspLockedPointer<CDispUser> pDispUser =
			CDspService::instance()->getDispConfigGuard().getDispUserByUuid(sUserId);
		if (!pDispUser.isValid())
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to find user for UUID '%s'",	QSTR2UTF8(sUserId));
			user->sendSimpleResponse(p, PRL_ERR_USER_NOT_FOUND);
			return;
		}
	}

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( p );
	if( !pCmd->IsValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Parsing protocol serializer command is failed %s",
			QSTR2UTF8(user->getUserSettingsUuid()));
		user->sendSimpleResponse(p, PRL_ERR_OPERATION_FAILED);
		return;
	}

	sUserId = pCmd->GetFirstStrParam();
	QStringList lstUserInfo = getUserInfoList(sUserId);

	if (lstUserInfo.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to find user for UUID '%s'",	QSTR2UTF8(sUserId));
		user->sendSimpleResponse(p, PRL_ERR_USER_NOT_FOUND);
		return;
	}

	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse* response =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

	response->SetUserInfo( lstUserInfo.first() );

	SmartPtr<IOPackage> responsePkg =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, p );
	CDspService::instance()->getIOServer().sendPackage( h, responsePkg );
}

QStringList CDspUserHelper::getUserInfoList(const QString& sUserId)
{
	CDispUsersPreferences UsersPreferences(
		CDspService::instance()->getDispConfigGuard().getDispUserPreferences().getPtr());

	QStringList lstUserInfo;

	QList<CDispUser* > lstDispUsers = UsersPreferences.m_lstDispUsers;
	for(int i = 0; i < lstDispUsers.size(); ++i)
	{
		CDispUser* pUser = lstDispUsers[i];

		UserInfo userInfo;
		if (sUserId.isEmpty() || sUserId == pUser->getUserId())
		{
			userInfo.setName(pUser->getUserName());
			userInfo.setUuid(pUser->getUserId());
			userInfo.setChangeServerSettings(pUser->getUserAccess()->isChangeServerSettingsFlag());

			// Fill session info list
			QList< SmartPtr<CDspClient> > lstClients = getClientSessionByUserUuid(pUser->getUserId());
			for(int j = 0; j < lstClients.size(); j++)
			{
				userInfo.getSessionInfoList()->m_lstSessionInfo
					+= new SessionInfo(lstClients[j]->getSessionInfo().getImpl());
			}
			userInfo.setSessionCount(userInfo.getSessionInfoList()->m_lstSessionInfo.size());

			QString sDefaultVmFolder = pUser->getUserWorkspace()->getDefaultVmFolder();
			if (sDefaultVmFolder.isEmpty())
			{
				QString vmDirUuid = pUser->getUserWorkspace()->getVmDirectory();
				CDspLockedPointer<CVmDirectory> pVmDirectory =
					CDspService::instance()->getVmDirManager().getVmDirectory(vmDirUuid);

				PRL_ASSERT(pVmDirectory.isValid());
				if (pVmDirectory.isValid())
					sDefaultVmFolder = pVmDirectory->getDefaultVmFolder();
			}
			userInfo.setDefaultVmFolder(sDefaultVmFolder);

			lstUserInfo += userInfo.toString();

			if (!sUserId.isEmpty())
			{
				break;
			}
		}
	}

	return lstUserInfo;
}

#define USER_ID_EXT		" : user profile"

void CDspUserHelper::userProfileBeginEdit (
	const IOSender::Handle&,
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p )
{
	PRL_RESULT err = CDspService::instance()->getAccessManager()
		.checkAccess( pUser, (PVE::IDispatcherCommands)p->header.type );
	if( PRL_FAILED(err) )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to change user profile by error [%#x][%s]"
			, err, PRL_RESULT_TO_STRING(err) );
		pUser->sendSimpleResponse( p, err );
		return;
	}

	CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()->lock();

	SmartPtr<CDispatcherConfig > pDispConfigPrev(
		new CDispatcherConfig( CDspService::instance()->getDispConfigGuard().getDispConfig().getPtr() ) );

	CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()
		->registerBeginEdit( pUser->getClientHandle(), pDispConfigPrev, USER_ID_EXT );

	CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()->unlock();

	pUser->sendSimpleResponse( p, PRL_ERR_SUCCESS );
}


void CDspUserHelper::userProfileCommit (
										const IOSender::Handle& ,
										SmartPtr<CDspClient>& pUser,
										const SmartPtr<IOPackage>& pkg )
{
	CVmEvent ws_error;

	/**
	* get request parameters
	*/

	// XML configuration of the VM to be edit
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( pkg );
	if( !pCmd->IsValid() )
	{
		pUser->sendSimpleResponse(pkg, PRL_ERR_OPERATION_FAILED);
		return;
	}

	QString sUserProfile = pCmd->GetFirstStrParam();

	try
	{
		PRL_RESULT err = CDspService::instance()->getAccessManager()
			.checkAccess( pUser, (PVE::IDispatcherCommands)pkg->header.type );
		if( PRL_FAILED(err) )
			throw err;

		SmartPtr<CDispUser> pCurrentUserProfile( new CDispUser( CDspService::instance()->getDispConfigGuard()
			.getDispUserByUuidAsString( pUser->getUserSettingsUuid() ) ));

		//////////////////////////////////////////////////////////////////////////
		// parse pUser profile XML
		//////////////////////////////////////////////////////////////////////////

		SmartPtr<CDispUser> pUserProfileChecker( new CDispUser(sUserProfile) );
		if( !IS_OPERATION_SUCCEEDED( pUserProfileChecker->m_uiRcInit ) )
			throw PRL_ERR_PARSE_USER_PROFILE;

		if ( pUserProfileChecker->getUserId() != pCurrentUserProfile->getUserId() )
		{
			WRITE_TRACE(DBG_FATAL, "user(%s)  try to change userName(%s)",
				QSTR2UTF8( pCurrentUserProfile->getUserName() ),
				QSTR2UTF8( pUserProfileChecker->getUserName() )
				);
			throw PRL_ERR_USER_CANT_CHANGE_READ_ONLY_VALUE;

		}
		else if ( pUserProfileChecker->getUserName() != pCurrentUserProfile->getUserName() )
		{
			WRITE_TRACE(DBG_FATAL, "user(%s)  try to change userUuid(%s)",
				QSTR2UTF8( pCurrentUserProfile->getUserName() ),
				QSTR2UTF8( pUserProfileChecker->getUserId() )
				);
			throw PRL_ERR_USER_CANT_CHANGE_READ_ONLY_VALUE;
		}
		else
		{
			//FIXME: bug #2242 [ SDK ] May be need to add support of  multiple VmDirectories to SDK.
			// change VmDirUuid to VmDirPath
			QString vmDirPath = CDspService::instance()->getVmDirManager()
				.getVmDirectory( pUser->getVmDirectoryUuid() )->getDefaultVmFolder();

			// TODO: need fix VmDirectory field change. (need change on client side)
			if ( 0 && pUserProfileChecker->getUserWorkspace()->getVmDirectory()
				!= vmDirPath )
			{
				WRITE_TRACE(DBG_FATAL, "user(%s)  try to change VmDirectory(%s) to new value(%s) ",
					QSTR2UTF8( pCurrentUserProfile->getUserName() ),
					QSTR2UTF8( vmDirPath ),
					QSTR2UTF8( pUserProfileChecker->getUserWorkspace()->getVmDirectory() )
					);
				throw PRL_ERR_USER_CANT_CHANGE_READ_ONLY_VALUE;
			}

		}

		//FIXME: bug #2242 [ SDK ] May be need to add support of  multiple VmDirectories to SDK.
		//   restore VmDirectory field to Uuid
		pUserProfileChecker->getUserWorkspace()->setVmDirectory(
			pCurrentUserProfile->getUserWorkspace()->getVmDirectory() );

		////////////////////////////////////////////////////////////////////////////////
		// check if non authorized attempt of editing pUser profile access part occuring
		////////////////////////////////////////////////////////////////////////////////

		if (pCurrentUserProfile->getUserAccess()->canUseServerManagementConsole() !=
			pUserProfileChecker->getUserAccess()->canUseServerManagementConsole())
		{
			WRITE_TRACE(DBG_FATAL, ">>> User couldn't to edit profile access part");
			throw PRL_ERR_USER_CANT_CHANGE_ACCESS_PROFILE;
		}

		// addinfo #8036
		if (pCurrentUserProfile->getUserAccess()->canChangeServerSettings() !=
			pUserProfileChecker->getUserAccess()->canChangeServerSettings())
		{
			if (!pUser->getAuthHelper().isLocalAdministrator())
			{
				WRITE_TRACE(DBG_FATAL, ">>> User couldn't to edit profile access part");
				throw PRL_ERR_USER_CANT_CHANGE_ACCESS_PROFILE;
			}
			pUserProfileChecker->getUserAccess()->setChangeServerSettingsFlag(
				pCurrentUserProfile->getUserAccess()->canChangeServerSettings() );
		}

		//////////////////////////////////////////////////////////////////////////
		// check if pUser can write into default VM folder
		//////////////////////////////////////////////////////////////////////////

		QString qsDir = pUserProfileChecker->getUserWorkspace()->getDefaultVmFolder();
		// If qsDir is empty it means that VMs will save in pUser home directory
		if ( !qsDir.isEmpty() )
		{
			CAuthHelper* pAuthHelper = &pUser->getAuthHelper();
			if ( !CFileHelper::DirectoryExists(qsDir, pAuthHelper) )
			{
				WRITE_TRACE(DBG_FATAL,
					"CDspUserHelper::userProfileCommit() : Directory [%s] does not exist.",
					QSTR2UTF8(qsDir));
				ws_error.addEventParameter(
					new CVmEventParameter( PVE::String, qsDir, EVT_PARAM_MESSAGE_PARAM_0 )
					);
				throw PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
			}

			QString qsFullPath = QDir::toNativeSeparators(qsDir + "/prl_test_can_write");

#ifndef _WIN_
			// For Unix based Os, we need check permissions for parent catalog
			if ( !CFileHelper::FileCanWrite(CFileHelper::GetFileRoot(qsFullPath), pAuthHelper) ||
				!CFileHelper::FileCanExecute(CFileHelper::GetFileRoot(qsFullPath), pAuthHelper) )
				throw PRL_ERR_USER_NO_AUTH_TO_SAVE_FILES;
#else
			// For Win32, we topically trying to create file
			if ( !CFileHelper::FileCanWrite(CFileHelper::GetFileRoot(qsFullPath), pAuthHelper) )
				throw PRL_ERR_USER_NO_AUTH_TO_SAVE_FILES;
#endif // _WIN_

		}

		// save to qsettings proxy pass cache
		// only for local host user!
		if ( pUser->isLocal() )
		{
			SmartPtr<CDispUserCachedData> pCachedData( new CDispUserCachedData() );
			pCachedData->fromString( pUserProfileChecker->getUserCachedData()->toString());
			saveCachedProxyData( pCachedData, pUserProfileChecker->getUserId() );
			pUserProfileChecker->getUserCachedData()->ClearLists();

			// save user defined secure data
			SmartPtr<CDispUserSettings> pUserSet( new CDispUserSettings() );
			pUserSet->fromString( pUserProfileChecker->getProxySettings()->getUserSettings()->toString() );
			saveUserDefinedProxySecure( pUserSet, pUserProfileChecker->getUserId() );
		}
		pUserProfileChecker->getUserCachedData()->setSave( false );

		{// EDIT LOCK
			QMutexLocker editLock( CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig() );

			//////////////////////////////////////////////////////////////////////////
			// applying new pUser settings
			//////////////////////////////////////////////////////////////////////////
			CDspLockedPointer<CDispatcherConfig>
				pLockedDispConfig = CDspService::instance()->getDispConfigGuard().getDispConfig();

			SmartPtr<CDispatcherConfig > pDispConfigOld(
				new CDispatcherConfig( pLockedDispConfig.getPtr() ) );

			CDspLockedPointer<CDispUser>
				pLockedCurrUser = CDspService::instance()->getDispConfigGuard()
				.getDispUserByUuid( pUser->getUserSettingsUuid() );

			//Storing settings to have possibility to restore it
			QString sOldSettings = pLockedCurrUser->toString();
			pLockedCurrUser->fromString( pUserProfileChecker->toString() );

			// do not save proxy secure parameters
			pLockedCurrUser->getProxySettings()->getUserSettings()->setSave( false );
			pLockedCurrUser->getProxySettings()->getSystemSettings()->getUserSettings()->setSave( false);

			// MERGE
			SmartPtr<CDispatcherConfig > pDispConfigNew(
				new CDispatcherConfig( pLockedDispConfig.getPtr() ) );

			if ( ! CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()
						->tryToMerge(pUser->getClientHandle(), pDispConfigNew, pDispConfigOld, USER_ID_EXT)
				)
				throw PRL_ERR_USER_PROFILE_WAS_CHANGED;

			pLockedDispConfig->fromString(pDispConfigNew->toString());

			// TODO: Need hide this dispFilePath (DispConfig->save( dispFilePath ) ) into DispConfigGuard.
			QString dispFilePath = ParallelsDirs::getDispatcherConfigFilePath();

			// Save dispatcher config file
			PRL_RESULT save_rc = CDspService::instance()->getDispConfigGuard().saveConfig();
			if( PRL_FAILED( save_rc ) )
			{
				WRITE_TRACE(DBG_FATAL, "Unable to write data to disp configuration file by error %s"
					, PRL_RESULT_TO_STRING(save_rc) );

				pLockedCurrUser->fromString( sOldSettings );
				ws_error.addEventParameter(
					new CVmEventParameter( PVE::String, dispFilePath, EVT_PARAM_RETURN_PARAM_TOKEN)
					);
				throw save_rc;
			}

			CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()
				->registerCommit( pUser->getClientHandle(), USER_ID_EXT );

		}// EDIT UNLOCK

	     //////////////////////////////////////////////////////////////////////////
        // Notify users that pUser profile was changed
        //////////////////////////////////////////////////////////////////////////

		// Generate "VM added" event
		CVmEvent event( PET_DSP_EVT_USER_PROFILE_CHANGED,
			pCurrentUserProfile->getUserName(),
			PIE_DISPATCHER );

		SmartPtr<IOPackage> pkgEvt =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event, pkg);

		QList< SmartPtr<CDspClient> >
			sameUserSessions = getClientSessionByUserUuid( pUser->getUserSettingsUuid() );

		CDspService::instance()->getClientManager().sendPackageToClientList( pkgEvt , sameUserSessions );
	}
    catch (PRL_RESULT code)
    {
		ws_error.setEventCode( code );

		WRITE_TRACE(DBG_FATAL, "Error occurred while modification pUser profile with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
    }

    //////////////////////////////////////////////////////////////////////////
    // send result to pUser
    //////////////////////////////////////////////////////////////////////////

	pUser->sendResponseError(ws_error, pkg);
}

QList< SmartPtr<CDspClient> > CDspUserHelper::getClientSessionByUserUuid( const QString& userSettingsUuid )
{
	QList< SmartPtr<CDspClient> > retList;

	QHashIterator< IOSender::Handle, SmartPtr<CDspClient> >
		itSessions = CDspService::instance()->
			getClientManager().getSessionsListSnapshot ();
	while( itSessions.hasNext() )
	{
		SmartPtr<CDspClient> pSession = itSessions.next().value();
		if( pSession->getUserSettingsUuid() == userSettingsUuid )
			retList.append( pSession );
	}
	return retList;
}



SmartPtr<IOPackage> CDspUserHelper::makeLoginResponsePacket(
	const SmartPtr<CDspClient>& pSession
	, const SmartPtr<IOPackage>& pkg)
{
	CVmEvent loginResponse;

	PRL_ASSERT( pkg && pSession );

	//////////////////////////////////////////////////////////////////////////
	// 1. add current session Id
	//////////////////////////////////////////////////////////////////////////
	loginResponse.addEventParameter(
		new CVmEventParameter(
			PVE::String
			, pSession->getClientHandle()
			, EVT_PARAM_RESPONSE_LOGIN_CMD_SESSION
			)
		);

	//////////////////////////////////////////////////////////////////////////
	// 1.2. add host OS version, product version and server UUID info
	//////////////////////////////////////////////////////////////////////////
	QString sOsVersion = CDspService::instance()->getHostOsVersion();
	if (sOsVersion.isEmpty())
		sOsVersion = "Unknown";
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, sOsVersion
		, EVT_PARAM_PRL_SERVER_INFO_OS_VERSION ) );
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, CDspService::instance()->getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid()
		, EVT_PARAM_PRL_SERVER_INFO_SERVER_UUID ) );
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, VER_FULL_BUILD_NUMBER_STR
		, EVT_PARAM_PRL_SERVER_INFO_PRODUCT_VERSION ) );
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, QString("%1").arg( pSession->isConfirmationEnabled() )
		, EVT_PARAM_PRL_SERVER_INFO_CONFIRMATION_MODE ) );
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, QString("%1").arg( (quint32)ParallelsDirs::getAppExecuteMode() )
		, EVT_PARAM_PRL_SERVER_INFO_APP_EXECUTE_MODE ) );
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, QString("%1").arg( CDspService::instance()->getServiceStartTime() )
		, EVT_PARAM_PRL_SERVER_INFO_START_TIME ) );
	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
		, QString("%1").arg( CDspService::instance()->getServiceStartTimeMonotonic() )
		, EVT_PARAM_PRL_SERVER_INFO_START_TIME_MONOTONIC ) );

	loginResponse.addEventParameter( new CVmEventParameter( PVE::String
			, QString("%1").arg( CMainDspService::isLaunchdMode() )
			, EVT_PARAM_PRL_SERVER_INFO_IS_LAUNCHD_MODE ) );



	//////////////////////////////////////////////////////////////////////////
	// 2. fill task list
	//////////////////////////////////////////////////////////////////////////
	QList< SmartPtr< CDspTaskHelper > >
		taskList = CDspService::instance()->getTaskManager()
			.getTaskListBySession( pSession->getPrevSessionUuid() );
	foreach( SmartPtr<CDspTaskHelper> pTask, taskList )
	{
		CVmEvent taskInfo;
		taskInfo.addEventParameter(
			new CVmEventParameter( PVE::String
			, pTask->getJobUuid()
			, EVT_PARAM_RESPONSE_LOGIN_CMD_TASK_UUID )
			);
		taskInfo.addEventParameter(
			new CVmEventParameter( PVE::UnsignedInt
			, QString( "%1" ).arg( pTask->getRequestPackage()->header.type )
			, EVT_PARAM_RESPONSE_LOGIN_CMD_TASK_TYPE )
			);

		CDspLockedPointer<CVmEvent> pTaskParams = pTask->getTaskParameters();

		PRL_ASSERT( pTaskParams );
		if( pTaskParams )
		{
			taskInfo.addEventParameter(
				new CVmEventParameter( PVE::String
				, pTaskParams->toString()
				, EVT_PARAM_RESPONSE_LOGIN_CMD_TASK_PARAMS )
				);
			//pack to main event
			loginResponse.addEventParameter(
				new CVmEventParameter( PVE::CData
				, taskInfo.toString()
				, EVT_PARAM_RESPONSE_LOGIN_CMD_TASK_INFO )
				);
		}
	} //for


	//https://bugzilla.sw.ru/show_bug.cgi?id=449210
	SmartPtr<IOPackage> responsePkg;
	IOCommunication::ProtocolVersion _proto_version;
	memset(&_proto_version, 0, sizeof(IOCommunication::ProtocolVersion));
	if ( CDspService::instance()->getIOServer().clientProtocolVersion( pSession->getClientHandle(), _proto_version ) &&
		 IOPROTOCOL_BINARY_RESPONSE_SUPPORT(_proto_version) )
	{
		//Add supported OSes matrix
		TOpaqueTypeList<PRL_UINT8> _oses_types = CGuestOsesHelper::GetSupportedOsesTypes(PHO_UNKNOWN);
		COsesMatrix _matrix;
		foreach(const PRL_UINT8 &nOsType, _oses_types.GetContainer())
			_matrix.AddOsType(PHO_UNKNOWN, nOsType);

		CVmBinaryEventParameter *pEventParam = new CVmBinaryEventParameter(EVT_PARAM_PRL_SUPPORTED_OSES_LIST);
		_matrix.Serialize(*pEventParam->getBinaryDataStream().getImpl());
		loginResponse.addEventParameter(pEventParam);

		//Add supported features matrix
		pEventParam = new CVmBinaryEventParameter(EVT_PARAM_PRL_FEATURES_MATRIX);
		CDspService::instance()->getFeaturesMatrix().Serialize(*pEventParam->getBinaryDataStream().getImpl());
		loginResponse.addEventParameter(pEventParam);

		CProtoCommandPtr pResponseCmd = CProtoSerializer::CreateDspWsResponseCommand(pkg, PRL_ERR_SUCCESS);
		QByteArray _byte_array;
		QBuffer _buffer(&_byte_array);
		bool bRes = _buffer.open(QIODevice::ReadWrite);
		PRL_ASSERT(bRes);
		QDataStream _data_stream(&_buffer);
		_data_stream.setVersion(QDataStream::Qt_4_0);

		pResponseCmd->GetCommand()->Serialize(_data_stream);
		loginResponse.Serialize(_data_stream);
		_buffer.reset();

		responsePkg = DispatcherPackage::createInstance(PVE::DspWsBinaryResponse, _data_stream, _byte_array.size(), pkg);
	}
	else
	{
		//Old clients support
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse* response =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);

		response->SetLoginResponse( loginResponse.toString() );

		responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, pkg );
	}

	return responsePkg;
}

void CDspUserHelper::sendAllHostUsers (const IOSender::Handle& h,
									   const SmartPtr<IOPackage>& p)
{
	CDspService::instance()->sendSimpleResponseToClient(h, p, PRL_ERR_UNIMPLEMENTED);
	return;
}

// save to dispatcher qsettings password cached paroxy data
PRL_RESULT CDspUserHelper::saveCachedProxyData( SmartPtr<CDispUserCachedData> /*pData*/,
											   const QString & /*strUserId*/ )
{
	return PRL_ERR_UNIMPLEMENTED;
}

// save to dispatcher qsettings password cached proxy data
PRL_RESULT CDspUserHelper::loadCachedProxyData( SmartPtr<CDispUserCachedData> /*pData*/,
											   const QString & /*strUserId*/ )
{
	return PRL_ERR_UNIMPLEMENTED;
}

// save to dispatcher qsettings password cached proxy data
PRL_RESULT CDspUserHelper::saveUserDefinedProxySecure( SmartPtr<CDispUserSettings> /*pData*/,
													  const QString & /*strUserId*/ )
{
	return PRL_ERR_UNIMPLEMENTED;
}

// save to dispatcher qsettings password cached paroxy data
PRL_RESULT CDspUserHelper::addUserDefinedProxySecure( SmartPtr<CDispUserSettings> /*pData*/,
													 const QString & /*strUserId*/ )
{
	return PRL_ERR_UNIMPLEMENTED;
}

bool CDspUserHelper::searchCachedProxyPassword( const QString & strProxyName,
										  CredentialsList& creds )
{
	//////////////////////////////////////////////////////////////////////
	// check users vm directories
	CDspLockedPointer<CDispatcherSettings> pDispSettings = CDspService::instance()->getDispConfigGuard().getDispSettings();
	foreach ( CDispUser* p,	pDispSettings->getUsersPreferences()->m_lstDispUsers )
	{
		SmartPtr<CDispUserCachedData> pData( new CDispUserCachedData() );
		if ( PRL_SUCCEEDED(loadCachedProxyData( pData, p->getUserId())) )
		{
			foreach( CDispUserCachedPasword * pPass, pData->m_lstUserCachedPaswords )
			{
				if ( pPass->getProxyServerName() == strProxyName )
				{
					creds << Credentials( pPass->getProxyServerUser(),
										  pPass->getProxyServerPassword() );
				}
			}
		}
	}

	return creds.length() > 0;
}

bool CDspUserHelper::searchUserDefinedProxySettings(  const QString & /*strUrl*/,
													const QList<QString> & ignoreUsers,
													QString & host,
													uint & port,
													QString & user,
													QString & /*password*/,
													QString & userId,
													bool bSecure )
{
	//////////////////////////////////////////////////////////////////////
	// check users vm directories
	foreach ( CDispUser* p,
		CDspService::instance()->getDispConfigGuard().getDispSettings()->getUsersPreferences()->m_lstDispUsers )
	{
		if ( ignoreUsers.contains( p->getUserId() ) )
			continue;

		// save user id
		userId = p->getUserId();

		// create copy
		SmartPtr<CDispProxySettings> pSettings( new CDispProxySettings( *p->getProxySettings() ) );
		SmartPtr<CDispUserSettings> pUserDefinedSettings( new CDispUserSettings( *pSettings->getUserSettings() ) );
		if( pUserDefinedSettings->isEnabled() )
		{
			// load secure data from registry
			addUserDefinedProxySecure( pUserDefinedSettings, p->getUserId() );
			if ( bSecure && pUserDefinedSettings->isUseCommonSettings() )
			{
				host = pUserDefinedSettings->getHttpSslProxyName();
				port = pUserDefinedSettings->getHttpSslProxyPort();
				user = pUserDefinedSettings->getHttpsProxyUser();
			}
			else
			{
				host = pUserDefinedSettings->getHttpProxyName();
				port = pUserDefinedSettings->getHttpProxyPort();
				user = pUserDefinedSettings->getHttpProxyUser();
			}
			return true;
		}
		if ( pSettings->getSystemSettings()->isEnabled() )
		{
			CDispInternetExplorerSettings * pIeSettings = pSettings->getSystemSettings();
			if ( pIeSettings->getUserSettings()->isEnabled() )
			{
				if ( bSecure )
				{
					host = pIeSettings->getUserSettings()->getHttpSslProxyName();
					port = pIeSettings->getUserSettings()->getHttpSslProxyPort();
					user = pIeSettings->getUserSettings()->getHttpsProxyUser();
				}
				else
				{
					host = pIeSettings->getUserSettings()->getHttpProxyName();
					port = pIeSettings->getUserSettings()->getHttpProxyPort();
					user = pIeSettings->getUserSettings()->getHttpProxyUser();
				}
			}

		}
		return true;
	}
	return false;
}

/*****************************************************************************/
