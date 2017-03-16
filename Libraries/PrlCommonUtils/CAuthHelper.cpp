/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 1999-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file CAuthHelper.cpp
///
/// @brief
///         CAuthHelper class implementation file.
///
/////////////////////////////////////////////////////////////////////////////


#ifdef _WIN_
    #define FORCE_UNICODE
#endif

#include <prlcommon/Interfaces/ParallelsQt.h>

#include "CAuthHelper.h"
#include "CFileHelper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "Libraries/CAuth/CAuth.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

#ifdef _WIN_
#   include <windows.h>
#   include <Lmwksta.h>
#   include <Lm.h>
#	include <Userenv.h>
#	include <Aclapi.h.>
#	include <atlsecurity.h>
#else
#  include <sys/types.h>
#  include <unistd.h>
#  include <pwd.h>
#  include <sys/stat.h>
#  include <errno.h>
#endif

#include <prlcommon/PrlCommonUtilsBase/SysError.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#		define IS_FILE_NOT_FOUND_ERROR( err ) \
	( ( ERROR_FILE_NOT_FOUND == (err) || ERROR_PATH_NOT_FOUND == (err) ) ? true : false )


const QString CAuthHelper::s_strLocalDomain = ".";

namespace
{
}

#ifdef _WIN_
/**
 * Enables specified privilege for current process
 * @param privilege name which should be enabled
 * @param new privilege attributes value
 * @param pointer to variable to store previous attributes value (optional)
 * @return sign whether privilege was enabled successfully
 */
static bool EnablePrivilege( PWSTR privName, DWORD nNewAttrValue, DWORD *pnPrevAttrValue = NULL )
{
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tp = { 1 };

	if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken ) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to open current process token %u", GetLastError());
		return ( false );
	}
	if ( !LookupPrivilegeValue( NULL, privName,  &tp.Privileges[0].Luid ) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to lookup privilege '%s' value with error: %u",
						QSTR2UTF8(UTF16_2QSTR( privName )), GetLastError() );
		CloseHandle( hToken );
		return ( false );
	}

	if ( pnPrevAttrValue )
		*pnPrevAttrValue = tp.Privileges[0].Attributes;
	tp.Privileges[0].Attributes = nNewAttrValue;
	if ( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof(tp), NULL, NULL ) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to adjust token privileges for privilege '%s' with error: %u",
						QSTR2UTF8(UTF16_2QSTR( privName )), GetLastError() );
		CloseHandle( hToken );
		return ( false );
	}

	CloseHandle( hToken );
	return ( true );
}

namespace {
/**
 * Simple wrapper under privileges altering actions. Enables necessary
 * privileges for process (storing previous state of privileges) on
 * object instantiation and restores initial values after destroy.
 */
class TPrivilegesHelper
{
public:
	TPrivilegesHelper()
	: m_bSuccess( true )
	{
		SIZE_T i;
		static PWSTR privileges[] = {
			SE_BACKUP_NAME,
			SE_RESTORE_NAME,
			SE_SECURITY_NAME
		};

		for (i = 0; i < sizeof(privileges)/sizeof(*privileges); ++i)
		{
			DWORD nPrevValue = 0;
			if ( !EnablePrivilege( privileges[i], SE_PRIVILEGE_ENABLED, &nPrevValue ) )
			{
				WRITE_TRACE(DBG_FATAL, "Failed to enable privelege '%s'", QSTR2UTF8(UTF16_2QSTR( privileges[i] )) );
				m_bSuccess = false;
			}
			else
				m_PrivilegesValues[ UTF16_2QSTR( privileges[i] ) ] = nPrevValue;
		}
	}

	~TPrivilegesHelper()
	{
		QMap<QString, DWORD>::const_iterator _it = m_PrivilegesValues.begin();
		for ( ; _it != m_PrivilegesValues.end(); ++_it )
		{
			if ( !EnablePrivilege( (PWSTR)_it.key().utf16(), _it.value() ) )
			{
				WRITE_TRACE(DBG_FATAL, "Failed to restore privelege '%s'", QSTR2UTF8(_it.key()) );
			}
		}
	}

	bool IsSucceeded() const { return ( m_bSuccess ); }

private:
	/** Initial privileges values */
	QMap<QString, DWORD> m_PrivilegesValues;
	/** Sign whether priveleges were set properly */
	bool m_bSuccess;
};
}
#endif // #ifdef _WIN_

CAuthHelper::OwnerWrapper::OwnerWrapper( CAuthHelper* pAuthHelper )
:m_pOwnersAuthHelper(pAuthHelper)
{
	PRL_ASSERT( pAuthHelper );
}
CAuthHelper::OwnerWrapper::OwnerWrapper( const QString& sOwnersFileName )
:m_pOwnersAuthHelper(0), m_sOwnersFileName(sOwnersFileName)
{
	PRL_ASSERT( !sOwnersFileName.isEmpty() );
}


/**
* *	on Windows checks if current user is in Local Administrators group
* * on Linux checks for user id, or ability to exec 'su' or 'sudo'.
* * on Darwin checks for user id, or ability to exec 'su' or 'sudo'.
**/
bool CAuthHelper::isLocalAdministrator()
{
	if (m_strUserName.isEmpty())
		return true;

	bool is = false;
	if (Impersonate())
	{
		is = m_pAuth->isLocalAdministrator(m_strUserName);
		RevertToSelf();
	}
	return is;
}

/**
 * @brief
 *         Class constructor for using from CDspInteractiveUser
 *
 * @params
 *         None
 */

CAuthHelper::CAuthHelper(QString strUserName)
{
    m_pAuth = new CAuth;

    QString userName, domainName;

    // parse strUserName
    int idx = strUserName.indexOf ( "/" ) ;
    if ( idx == -1 )
	    idx = strUserName.indexOf ( "\\" ) ;
	if ( idx != -1 )
    {
        domainName =    strUserName.left( idx );
        userName =      strUserName.mid ( idx+1 );
    }
	else
	{
	    idx = strUserName.indexOf ( "@" ) ;
		if ( idx != -1 )
		{
			userName =    strUserName.left( idx );
			domainName =      strUserName.mid ( idx+1 );
		}
	}

    if (idx == -1)
        userName = strUserName;

    setUserCreditals (userName, domainName);
    fillDefaultDomain();
}


/**
 * @brief
 *         Class constructor.
 *
 * @params
 *         None
 */

CAuthHelper::CAuthHelper()
{
    m_pAuth = new CAuth;

    setUserCreditals( "" ,"" );
    fillDefaultDomain();
}

CAuthHelper::CAuthHelper( const CAuthHelper &)
{}

bool CAuthHelper::CloneAuthObject(const CAuthHelper &auth_helper)
{
	if (this != &auth_helper)
	{
		m_strUserName = auth_helper.m_strUserName;
		m_strUserDomain = auth_helper.m_strUserDomain;
		m_strDefaultDomain = auth_helper.m_strDefaultDomain;
		delete m_pAuth;
		m_pAuth = auth_helper.m_pAuth->Clone();
		return (m_pAuth != NULL);
	}
	return (false);
}

/**
 * @brief
 *         Class destructor.
 *
 * @params
 *         None
 */

CAuthHelper::~CAuthHelper()
{
	delete m_pAuth;
};


/**
 * @brief
 *         Try to authenticate user
 *
 * @params
 *         password [in] - password
 *
 * @return
 *         TRUE - if operation is success
 */

bool CAuthHelper::AuthUser(const QString &password)
{
	bool res = true;
	do
	{
		// 1. Auth user in obviously defined domain
		if ( ! m_strUserDomain.isEmpty() )
		{
			res = m_pAuth->AuthUser(m_strUserName, password, m_strUserDomain);
			break;
		}

		// 2. Auth user without obviously defined domain in current domain ( windows )
		if ( m_pAuth->AuthUser( m_strUserName, password, m_strDefaultDomain ) )
			break;

		// 3. Try to login as local user (on windows hosts)
		if ( m_strDefaultDomain != s_strLocalDomain  // only on windows on computer in domain
			&& m_pAuth->AuthUser( m_strUserName, password, s_strLocalDomain )
		)
			break;

		res = false;
	}while(0);

	QString userName = m_strUserName;
	QString domainName = m_strUserDomain;
#ifdef _WIN_
	if( res && !CAuth::GetUserAndDomainByAuthToken ( GetAuth()->GetTokenHandle(), userName, domainName ) )
	{
		res = false;
		WRITE_TRACE(DBG_FATAL, "Couldn't to GetUserAndDomainByAuthToken(). Error code: %d", GetLastError());
	}
	if( res && domainName.isEmpty() )
		domainName = ".";
#endif

	if(res)
		setUserCreditals( userName, domainName );

    return res;
};


/**
 * @brief
 *         Try to authenticate user by specified user id
 *
 * @params
 *         UserId [in] - user id
 *
 * @return
 *         TRUE - if operation is success
 */

bool CAuthHelper::AuthUser(int UserId)
{
    QString userName, domainName;
    if ( m_pAuth->AuthUser((quint32)UserId, userName, domainName) )
    {
        if( domainName.isEmpty() )
            domainName = ".";

        setUserCreditals ( userName, domainName );
        return true;
    }

    return false;
};

bool CAuthHelper::AuthUserBySelfProcessOwner()
{
	return AuthUser(
#ifdef _WIN_
					_getpid()
#else
					getuid()
#endif
					);

}


/**
 * @brief
 *         Check file permissions
 *
 * @params
 *         strFileName [in] - file name
 *
 * @return
 *         [R][W][E] - access mask
 */

CAuth::AccessMode  CAuthHelper::CheckFile(const QString & strFileName)
{
   CAuth::AccessMode  ret=m_pAuth->CheckFile(m_strUserName, strFileName, "", "");

   return ret;
};

const QString& CAuthHelper::getUserName () const
{
    return m_strUserName;
}

const QString& CAuthHelper::getUserDomain () const
{
    return m_strUserDomain;
}

QString CAuthHelper::getUserFullName () const
{
	return QString( "%1@%2" )
		.arg( getUserName() )
		.arg( getUserDomain() );
}


QString CAuthHelper::getHomePath () const
{
#ifdef _WIN_

   HANDLE hToken=m_pAuth->GetTokenHandle();
   wchar_t profileDir[2*1024];
   DWORD cchSize = sizeof(profileDir)/sizeof(TCHAR);

   if (!GetUserProfileDirectory(hToken, profileDir, &cchSize))
   {
      WRITE_TRACE(DBG_FATAL, "Unable to get user home dir for user [%s]. "
		  "Possible solution: User should login to Windows OS as this user "
		  " and Windows OS automatically creates home dir."
		  , m_strUserName.toUtf8().data() );
      return "";
   }

   QString home=UTF16SZ_2QSTR(profileDir, cchSize);
   home.replace("\\", "/");
   return home.toUtf8();

#else

	struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
	QByteArray _passwd_strings_buf;
	struct passwd userInfo;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	::getpwnam_r(QSTR2UTF8(m_strUserName), &userInfo, _passwd_strings_buf.data(),
						_passwd_strings_buf.size(),	&lpcUserInfo );
#else
	lpcUserInfo = getpwnam( QSTR2UTF8(m_strUserName) );
#endif
	if (!lpcUserInfo)
	{
		WRITE_TRACE(DBG_FATAL, "can't get info for user [%s]", m_strUserName.toUtf8().data());
		return "";
	}
	if (!lpcUserInfo->pw_dir)
		return "";

	return UTF8_2QSTR(lpcUserInfo->pw_dir);
#endif
}


bool CAuthHelper::isDefaultAppUser () const
{
    return m_strUserName.isEmpty();
}

bool CAuthHelper::Impersonate()
{
	bool result = true;

#ifdef _WIN_
	result = GetAuth()->Impersonate();
#endif // _WIN_

	return result;
}

bool CAuthHelper::RevertToSelf()
{
	bool result = true;

#ifdef _WIN_
	result = GetAuth()->RevertToSelf();
#endif

	return result;
}

ParallelsDirs::UserInfo CAuthHelper::getParallelsDirUserInfo()
{
ParallelsDirs::UserInfo userInfo(
#   ifdef _WIN_
        GetAuth()->GetTokenHandle(),
#   else
        getUserName(),
#   endif
		getHomePath() );

    return userInfo;
}

void CAuthHelper::NormalizeCreditals ( QString& strSomeName )
{
#   ifdef _WIN_
        strSomeName = strSomeName.toLower();
#   else
        Q_UNUSED ( strSomeName );
#   endif
}

void CAuthHelper::setUserCreditals( const QString& userName, const QString& userDomain )
{
    Q_UNUSED( userDomain );

    m_strUserName = userName;
    CAuthHelper::NormalizeCreditals( m_strUserName );

#   ifdef _WIN_
    m_strUserDomain = userDomain;
    CAuthHelper::NormalizeCreditals( m_strUserDomain );
#   else
    m_strUserDomain = CAuthHelper::s_strLocalDomain;
#   endif
}


void CAuthHelper::fillDefaultDomain()
{
    m_strDefaultDomain = CAuthHelper::s_strLocalDomain;

    // In Windows we need determine current domain name or '.' if hasn't domain,
    // in Linux case we use "." as domain name.

#ifdef _WIN_
    QString buff;
    if ( CAuth::getCurrentDomain( buff ) && ! buff.isEmpty() )
        m_strDefaultDomain = buff;
#endif // _WIN_

    NormalizeCreditals ( m_strDefaultDomain );
}


#ifdef _WIN_
bool CAuthHelper::LookupAliasFromRid(const QString &strTargetComputer,
                        void *pSia,
                        unsigned long ulRidType,
                        unsigned long ulRid,
                        QString &strAlias)
{
    SID_IDENTIFIER_AUTHORITY sia = *((SID_IDENTIFIER_AUTHORITY *)pSia);
    SID_NAME_USE snu;
    PSID pSid;
    WCHAR DomainName[DNLEN+1];
    WCHAR Name[UNLEN+1];
	DWORD cchName = UNLEN;
    DWORD cchDomainName = DNLEN;
    bool bSuccess = FALSE;

    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //

    if(AllocateAndInitializeSid(
        &sia,
        ulRidType == SECURITY_BUILTIN_DOMAIN_RID ? 2:1,
        ulRidType,
        ulRid,
        0, 0, 0, 0, 0, 0,
        &pSid
        ))
	{
				bSuccess = LookupAccountSidW(
                (LPWSTR)strTargetComputer.utf16(),
                pSid,
                Name,
                &cchName,
                DomainName,
                &cchDomainName,
                &snu
                );

            FreeSid(pSid);

			strAlias = QString( "%1\\%2" )
				.arg( UTF16_2QSTR( DomainName ) )
				.arg( UTF16_2QSTR( Name ) );
            //strAlias.setUtf16((ushort*)Name, wcslen(Name));
    }

    return bSuccess;
};
#endif // _WIN_

QString CAuthHelper::GetOwnerOfFile( const QString& fileName, bool bUseSlash )
{
	QString user, domain;

	if( !GetOwnerOfFile( fileName, user, domain ) )
		return QString();

	if (bUseSlash)
		return (QString("%1\\%2").arg(domain).arg(user));
	else
		return QString("%1@%2").arg( user ).arg( domain );
}

bool CAuthHelper::GetOwnerOfFile( const QString& fileName, QString& outUserName, QString& outDomainName )
{
	outUserName = outDomainName = "";

#ifdef _LIN_

	QFileInfo fi( fileName );
	outUserName  = fi.owner();
	outDomainName = s_strLocalDomain;

	return true;

#else // _WIN_

	// cant get owner for FAT
	if ( CAuth::IsFATFileSystem( fileName ))
		return false;

	PSID psidOwner				= NULL;
	PSECURITY_DESCRIPTOR sd		= NULL;

	// get dacl information from file object
	DWORD dwError = GetNamedSecurityInfo((LPWSTR)fileName.utf16(),
		SE_FILE_OBJECT,
		OWNER_SECURITY_INFORMATION,
		&psidOwner,
		NULL,
		NULL,
		NULL,
		&sd);

	if(dwError != ERROR_SUCCESS)
	{
		WRITE_TRACE(DBG_FATAL, "[%s] GetNamedSecurityInfo( '%s' ) failed whith error %d"
			, __FUNCTION__
			, QSTR2UTF8( fileName )
			, dwError );

		return false;
	}

	PRL_ASSERT( psidOwner );
	PRL_ASSERT( sd );
	if( !psidOwner || !sd )
		return false;

	const int BUFF_SIZE = 1024;
	TCHAR lpName[ BUFF_SIZE ];
	DWORD cchName = sizeof( lpName ) / sizeof( TCHAR ) ;
	TCHAR lpDomainName[ BUFF_SIZE ] ;
	DWORD cchDomainName = sizeof( lpName ) / sizeof( TCHAR ) ;
	SID_NAME_USE sidType;

	if( ! LookupAccountSid( NULL, psidOwner, lpName, &cchName, lpDomainName, &cchDomainName, &sidType ) )
	{
		WRITE_TRACE(DBG_FATAL, "[%s] LookupAccountSid failed whith error %d"
			, __FUNCTION__
			, GetLastError() );


		LocalFree( sd );
		return false;
	}

	outUserName = UTF16_2QSTR( lpName );
	outDomainName = UTF16_2QSTR( lpDomainName );

	LocalFree( sd );
	return true;
#endif
}

#ifdef _WIN_

bool CAuthHelper::SetOwnerOfFile( const QString& fileName, OwnerWrapper& owner, PRL_RESULT* pOutError)
{
	if( owner.getAuthHelper() )
		return SetOwnerOfFile( fileName, owner.getAuthHelper(), pOutError );

	return SetOwnerOfFile( fileName, owner.getFileName(), pOutError );
}

bool CAuthHelper::SetOwnerOfFile( const QString& fileName, CAuthHelper* pAuthHelper, PRL_RESULT* pOutError )
{
	PRL_ASSERT( pAuthHelper );
	if( ! pAuthHelper )
		return false;

	PRL_ASSERT( pAuthHelper->GetAuth()->GetTokenHandle() );
	if( ! pAuthHelper->GetAuth()->GetTokenHandle() )
		return false;

	HANDLE hToken = pAuthHelper->GetAuth()->GetTokenHandle();

	CAccessToken token;
	token.Attach(hToken);
	CSid _user_sid;
	if (!token.GetUser(&_user_sid))
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to extract user SID with CAccessToken::GetUser() ATL call.\
					Error code: %d", GetLastError());
		token.Detach();
		return (false);
	}

	CSid _curr_owner_sid;
	if (!AtlGetOwnerSid((LPWSTR)fileName.utf16(), SE_FILE_OBJECT, &_curr_owner_sid))
		WRITE_TRACE(DBG_FATAL, "Couldn't to extract current owner sid: %d", GetLastError());
	else if (_user_sid == _curr_owner_sid)//Already this owner
	{
		token.Detach();
		return (true);
	}

	if (!AtlSetOwnerSid((LPWSTR)fileName.utf16(), SE_FILE_OBJECT, _user_sid))
	{
		DWORD dwError = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Couldn't to apply owner SID with AtlSetOwnerSid() ATL call."
			"Error code: (%d)  . Try to do it through Impersonate."
			, dwError );

		//Couldn't to change owner of file maybe it's privileged user (Administrator)
		//try to do it under owner itself
		pAuthHelper->Impersonate();
		if (!AtlSetOwnerSid((LPWSTR)fileName.utf16(), SE_FILE_OBJECT, _user_sid))
		{
			dwError = GetLastError();

			pAuthHelper->RevertToSelf();

			if( pOutError && IS_FILE_NOT_FOUND_ERROR( dwError ) )
				*pOutError = PRL_ERR_FILE_NOT_FOUND;

			WRITE_TRACE(DBG_FATAL, "Couldn't to apply owner SID with AtlSetOwnerSid() ATL call."
					"Error code: %d", dwError );
			token.Detach();
			return (false);
		}
		pAuthHelper->RevertToSelf();
	}
	token.Detach();

	return true;
}

bool CAuthHelper::SetOwnerOfFile( const QString& fileName, const QString& fileNameWithOwner, PRL_RESULT* pOutError )
{
	PRL_ASSERT( !fileNameWithOwner.isEmpty() );
	if( pOutError )
		*pOutError = PRL_ERR_SUCCESS;

	try
	{
		CSid sidOwnerOfTemplate;
		if (!AtlGetOwnerSid((LPWSTR)fileNameWithOwner.utf16(), SE_FILE_OBJECT, &sidOwnerOfTemplate))
			throw "AtlGetOwnerSid( templateOwner ) failed";

		CSid sidOwnerOfFile;
		if (!AtlGetOwnerSid((LPWSTR)fileName.utf16(), SE_FILE_OBJECT, &sidOwnerOfFile))
			WRITE_TRACE(DBG_FATAL, "Couldn't to extract current owner sid: %d", GetLastError());
		else if (sidOwnerOfTemplate == sidOwnerOfFile)//Already this owner
			return (true);

		//Enable secure privileges
		TPrivilegesHelper _priveleges_helper;
		if ( !_priveleges_helper.IsSucceeded() )
		{
			WRITE_TRACE(DBG_FATAL, "Failed to enable security privileges - potentially we won't be able to take ownership gracefully");
		}
		
		if (!AtlSetOwnerSid((LPWSTR)fileName.utf16(), SE_FILE_OBJECT, sidOwnerOfTemplate))
		{
			DWORD dwError = GetLastError();

			if( pOutError && IS_FILE_NOT_FOUND_ERROR( dwError ) )
				*pOutError = PRL_ERR_FILE_NOT_FOUND;

			SetLastError( dwError );
			throw "AtlSetOwnerSid() failed";
		}

		return true;
	}
	catch( const char* strError )
	{
		DWORD dwLastError = GetLastError();
		WRITE_TRACE(DBG_FATAL, "SetOwnerOfFile() failed by reason '%s'."
			" Error code: %d"
			" fileName = '%s' "
			" templateFileName = '%s'"
			, strError
			, dwLastError
			, QSTR2UTF8( fileName )
			, QSTR2UTF8( fileNameWithOwner )
			);
	}
	return false;
}

#endif

bool CAuthHelper::isOwnerOfFile( const QString& path )
{
	bool ret = false;
	try
	{
		if( isDefaultAppUser() )
			throw "isDefaultAppUser()";
#ifdef _WIN_
	if( ! m_pAuth->GetTokenHandle() )
		return false;

	HANDLE hToken = m_pAuth->GetTokenHandle();

	CAccessToken token;
	token.Attach(hToken);
	CSid _user_sid, _owner_sid;
	if (!token.GetUser(&_user_sid))
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to extract user SID with CAccessToken::GetUser() ATL call.\
					Error code: %d", GetLastError());
		token.Detach();
		throw "Failed to call CAccessToken::GetUser()";
	}

	if (!AtlGetOwnerSid((LPWSTR)path.utf16(), SE_FILE_OBJECT, &_owner_sid))
	{
		//Couldn't to get owner of file maybe it's privileged user (Administrator)
		//try to do it under owner itself
		Impersonate();
		if (!AtlGetOwnerSid((LPWSTR)path.utf16(), SE_FILE_OBJECT, &_owner_sid))
		{
			RevertToSelf();
			WRITE_TRACE(DBG_FATAL, "Couldn't to get owner SID with AtlGetOwnerSid() ATL call.\
					Error code: %d", GetLastError());
			token.Detach();
			throw "Failed to call AtlGetOwnerSid()";
		}
		RevertToSelf();
	}
	token.Detach();
	if (_user_sid == _owner_sid)
		ret = true;
#else

#ifdef _LIN_
		struct stat64 _stat_info;
		if (!stat64(QSTR2UTF8(path), &_stat_info))
#else
		struct stat _stat_info;
		if (!stat(QSTR2UTF8(path), &_stat_info))
#endif
		{
			if (_stat_info.st_uid == m_pAuth->GetUserId())
				ret = true;
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "stat() cal returned an error: %d", errno);
			throw "stat() system call failed";
		}

#endif
	}
	catch ( const char* err )
	{
		WRITE_TRACE(DBG_FATAL, "%s error: %s", __FUNCTION__, err );
		ret=false;
	}

	return ret;
}

CAuthHelperImpersonateWrapper::CAuthHelperImpersonateWrapper(CAuthHelper *pAuthHelper)
: m_pAuthHelper(pAuthHelper)
{
	if (m_pAuthHelper && ! m_pAuthHelper->Impersonate() )
	{
		WRITE_TRACE(DBG_FATAL, "CAuthHelperImpersonateWrapper: Impersonate failed! " );
		m_pAuthHelper = 0; // to check in wasImpersonated() and prevent call RevertToSelf().
	}
}

bool CAuthHelperImpersonateWrapper::wasImpersonated() const
{
	return m_pAuthHelper != 0 ;
}

CAuthHelperImpersonateWrapper::~CAuthHelperImpersonateWrapper()
{
	if (m_pAuthHelper)
		m_pAuthHelper->RevertToSelf();
}

CAuthHelperImpersonateWrapperPtr CAuthHelperImpersonateWrapper::create( CAuthHelper *pAuthHelper )
{
	return CAuthHelperImpersonateWrapperPtr( new CAuthHelperImpersonateWrapper( pAuthHelper ) );
}

#ifdef _WIN_
/**
 * Helper which lets to safely get information of current file owner even if last one has rights just for owner.
 * @param path to file
 * @param buffer to retrieve owner SID
 * @return sign whether operation was completed successfully
 */
static bool GetSecureOwnerSid( const QString &sPath, CSid *pSid )
{
	HANDLE hFile = CreateFileW(
		(LPWSTR)sPath.utf16(),
		GENERIC_READ,
		FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,
		0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to open file %u '%s'", GetLastError(), QSTR2UTF8(sPath));
		return ( false );
	}
	bool bRet = AtlGetOwnerSid( hFile, SE_FILE_OBJECT, pSid );
	if ( !bRet )
		WRITE_TRACE(DBG_FATAL, "Failed to get owner SID with error: %u for path: '%s'", GetLastError(), QSTR2UTF8(sPath));
	CloseHandle( hFile );
	return ( bRet );
}

/**
 * Helper which lets to safely take ownership of file even if last one has rights just for owner.
 * @param path to file
 * @param applying user SID (new owner)
 * @return sign whether operation was completed successfully
 */
static bool SetSecureOwnerSid( const QString &sPath, const CSid &_sid )
{
	HANDLE hFile = CreateFileW(
		(LPWSTR)sPath.utf16(),
		WRITE_OWNER,
		FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,
		0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to open file %u '%s'", GetLastError(), QSTR2UTF8(sPath));
		return ( false );
	}
	bool bRet = AtlSetOwnerSid( hFile, SE_FILE_OBJECT, _sid );
	if ( !bRet )
		WRITE_TRACE(DBG_FATAL, "Failed to set owner SID with error: %u for path: '%s'", GetLastError(), QSTR2UTF8(sPath));
	CloseHandle( hFile );
	return ( bRet );
}

/**
 * Helper of add access rigths for admin group action.
 * Adds access rights for local administrators group to specified entry.
 * Recursively processes child entries in case if specified entry is directory.
 * @param system name of local administrators group
 * @param current user account SID
 * @param path to processing file system entry
 */
static void AddAccessRightsHelper( const QString &sAdminsGroupName, const CSid &_user_sid, const QString &sPath )
{
	bool bRestoreOwner = false, bOwnerRetrieved = false;
	CSid _owner_sid;
	if ( !GetSecureOwnerSid( sPath, &_owner_sid ) )
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to extract current owner sid from path '%s'", QSTR2UTF8(sPath));
	}
	else
	{
		bOwnerRetrieved = true;
	}

	if ( bOwnerRetrieved && _owner_sid != _user_sid )//Owner is not current account - let's try to take ownership
	{
		if ( !SetSecureOwnerSid( sPath, _user_sid) )
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to take ownership for path '%s'", QSTR2UTF8(sPath));
		}
		else
		{
			bRestoreOwner = bOwnerRetrieved;
		}
	}

	//Try to add access rights for administrators
	PRL_RESULT nPrlErr = PRL_ERR_UNINITIALIZED;
	if ( !CAuth::AddAccessRights( sPath, sAdminsGroupName, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT, false, &nPrlErr) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to add access rights for local administrators for path '%s' with error: '%s' %.8X",
						QSTR2UTF8(sPath),
						PRL_RESULT_TO_STRING(nPrlErr),
						nPrlErr);
	}

	//Restore file owner
	if ( bRestoreOwner )
	{
		if ( !SetSecureOwnerSid( sPath, _owner_sid) )
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to restore owner for path '%s'", QSTR2UTF8(sPath));
		}
	}

	//Process directory case
	QFileInfo _parent_dir( sPath );
	bool bIsReparsePoint = false;
	// Check for NTFS junction and symlink to dir
	DWORD dwAttr = GetFileAttributes( (LPCWSTR)_parent_dir.absoluteFilePath().utf16() );
	if( dwAttr & FILE_ATTRIBUTE_REPARSE_POINT )
		bIsReparsePoint = true;

	// Check if dir - enter the dir
	if ( _parent_dir.isDir()
			&& !_parent_dir.isSymLink()
			&& !bIsReparsePoint )
	{
		QDir _dir( sPath );
		QFileInfoList _entries = _dir.entryInfoList( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsLast );
		foreach( const QFileInfo &_file_info, _entries )
		{
			if ( _parent_dir == _file_info ) continue;
			AddAccessRightsHelper( sAdminsGroupName, _user_sid, _file_info.absoluteFilePath() );
		}
	}
}

bool CAuthHelper::AddAccessRightsGeneric( const QString &sPath, const QString &sAccountName )
{
	if ( !isLocalAdministrator() )
	{
		WRITE_TRACE(DBG_FATAL, "Account should be a local administrator to add rights for admin group");
		return ( false );
	}

	if ( !CFileHelper::FileExists( sPath, this ) )
	{
		WRITE_TRACE(DBG_FATAL, "Path '%s' doesn't exist", QSTR2UTF8(sPath));
		return ( false );
	}

	if ( CFileHelper::isRemotePath( sPath ) )
	{
		WRITE_TRACE(DBG_FATAL, "Skipped '%s' path due remote path", QSTR2UTF8(sPath));
		return (false);
	}

	PRL_FILE_SYSTEM_FS_TYPE _fs_type = HostUtils::GetFSType( sPath );
	if ( PRL_FS_NTFS != _fs_type )
	{
		WRITE_TRACE(DBG_FATAL, "Skipped '%s' path due non NTFS file system", QSTR2UTF8(sPath));
		return (false);
	}

	//Get current user SID for take ownership operations
	HANDLE hToken = m_pAuth->GetTokenHandle();
	CSid _user_sid;
	CAccessToken token;
	token.Attach( hToken );
	if ( !token.GetUser( &_user_sid ) )
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to extract user SID with CAccessToken::GetUser() ATL call.\
					Error code: %d", GetLastError());
		token.Detach();
		return (false);
	}

	//Enable secure privileges
	TPrivilegesHelper _priveleges_helper;
	if ( !_priveleges_helper.IsSucceeded() )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to enable security privileges - potentially we won't be able to take ownership gracefully");
	}

	AddAccessRightsHelper( sAccountName, _user_sid, sPath );
	token.Detach();
	return ( true );
}

bool CAuthHelper::AddAccessRightsForAdministrators( const QString &sPath )
{
	//Get alias for local administrators group
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	QString			strAlias;
	DWORD rid= DOMAIN_ALIAS_RID_ADMINS;
	DWORD ridType= SECURITY_BUILTIN_DOMAIN_RID;

	if ( !LookupAliasFromRid( "", &SIDAuthNT, ridType, rid, strAlias ) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to lookup alias for local administrators RID");
		return ( false );
	}

	return ( AddAccessRightsGeneric( sPath, strAlias ) );
}

bool CAuthHelper::AddAccessRightsForEveryone( const QString &sPath )
{
	//Get alias for local Users group
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	QString			strAlias;
	DWORD rid= DOMAIN_ALIAS_RID_USERS;
	DWORD ridType= SECURITY_AUTHENTICATED_USER_RID;

	if ( !LookupAliasFromRid( "", &SIDAuthNT, ridType, rid, strAlias ) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to lookup alias for local users RID");
		return ( false );
	}

	return ( AddAccessRightsGeneric( sPath, strAlias ) );
}

#endif

