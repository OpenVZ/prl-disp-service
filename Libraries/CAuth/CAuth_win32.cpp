/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

/********************************************************************
 * created:	2006/06/06
 * created:	6:6:2006   18:55
 * filename: 	C:\Projects\MainCC\artemr_ParallelsR3\PVSVob\a-Platform\Dispatcher\CAuth\CAuth\CAuth_win32.cpp
 * file path:	C:\Projects\MainCC\artemr_ParallelsR3\PVSVob\a-Platform\Dispatcher\CAuth\CAuth
 * file base:	CAuth_win32
 * file ext:	cpp
 * author:		artemr
 *
 * purpose:
 *********************************************************************/
/**
 * CAuth - class for check user permissions on host.
 * CAuth class. (Win32 realization)
 */

#ifdef _WIN_
#	define FORCE_UNICODE
#endif


#include <windows.h>
#include <Lm.h>

#define SECURITY_WIN32
#include <Sspi.h>

#include <QFileInfoList>
#include <QDir>

#include "ParallelsQt.h"
#include "CAuth.h"
#include "aclApi.h"
#include "AccCtrl.h"

#include "Shlwapi.h"

#include <atlsecurity.h>

#include "Libraries/Logging/Logging.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"

#include <authz.h>

#define MAX_SYSNAME_LENGTH	256
#define MAX_USER_NAME		MAX_SYSNAME_LENGTH
#define MAX_GROUP_NAME		MAX_SYSNAME_LENGTH
#define MAX_DOMAIN_NAME		MAX_SYSNAME_LENGTH

#define myheapalloc(x) (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x))
#define myheapfree(x)  (HeapFree(GetProcessHeap(), 0, x))


#define IS_FILE_NOT_FOUND_ERROR( err ) \
	( ( ERROR_FILE_NOT_FOUND == (err) || ERROR_PATH_NOT_FOUND == (err) ) ? true : false )

typedef struct _AUTH_SEQ {
	BOOL fInitialized;
	BOOL fHaveCredHandle;
	BOOL fHaveCtxtHandle;
	CredHandle hcred;
	struct _SecHandle hctxt;
} AUTH_SEQ, *PAUTH_SEQ;

namespace
{
	/**
	* Retrieves token information by specified class.
	* NOTE : use myheapfree() to free returned buffer
	*/
	LPVOID RetrieveTokenInformationClass(HANDLE hToken, TOKEN_INFORMATION_CLASS InfoClass, LPDWORD lpdwSize);

	/**
	* Returns SID by well-known RID
	* NOTE : use myheapfree() to free returned buffer
	*/
	PSID GetUserSidFromWellKnownRid(DWORD Rid);

	CAuth::AccessMode  ConvertToAccessMode( DWORD winAccessRights );
	bool ContainsSIDInGroup( PSID pMemberSid, PSID pGroupSid );

	/**
	 * Applies specified permissions to the file
	 */
	BOOL ApplyPermissionsToFile(const QString &sFilePath, PSECURITY_DESCRIPTOR pSd, PRL_RESULT* pOutError = 0)
	{
		HANDLE hFile = CreateFile((LPCWSTR)sFilePath.utf16(),
							WRITE_DAC|WRITE_OWNER,
							FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
							NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD dwErrorCode = GetLastError();

			if( pOutError && IS_FILE_NOT_FOUND_ERROR( dwErrorCode ) )
				*pOutError = PRL_ERR_FILE_NOT_FOUND;

			WRITE_TRACE(DBG_FATAL, "CreateFile error = %d file: '%s'", dwErrorCode, QSTR2UTF8(sFilePath));
			return (FALSE);
		}

		BOOL bRes = SetKernelObjectSecurity(hFile, DACL_SECURITY_INFORMATION, pSd);
		if (!bRes)
		{
			DWORD dwErrorCode = GetLastError();

			if( pOutError && IS_FILE_NOT_FOUND_ERROR( dwErrorCode ) )
				*pOutError = PRL_ERR_FILE_NOT_FOUND;

			WRITE_TRACE(DBG_FATAL, "SetKernelObjectSecurity error = %d file: '%s'", dwErrorCode, QSTR2UTF8(sFilePath));
		}
		CloseHandle(hFile);
		return (bRes);
	}

	/**
	* These functions are based on the Microsoft KB Sample 180548
	* http://support.microsoft.com/kb/180548
	*/

	/**
	* Logon() using SSPI
	*/
	BOOL WINAPI SSPLogonUser(LPTSTR szDomain, LPTSTR szUser, LPTSTR szPassword, PHANDLE phToken);

	BOOL IsGuest(HANDLE hToken);

	BOOL SSPGenServerContext(PAUTH_SEQ pAS, PVOID pIn, DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone);

	BOOL SSPGenClientContext(PAUTH_SEQ pAS, PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
		PVOID pIn, DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone);
}


/**
 * @params
 *		IN:
 * 		Out:
 * @brief Constructor
 *
 * @author artemr
 */

CAuth::CAuth()
{
	m_lphToken = NULL;
}

/**
 * @params
 *		IN:
 * 		Out:
 * @brief Destructor
 *
 * @author artemr
 */

CAuth::~CAuth()
{
	m_ImpersonateCounterStorage.setLocalData(0);
	if(m_lphToken)
		CloseHandle(m_lphToken);
}

/**
 * @params
 *		IN:	const QString & strUserName - QString object of username
 *			const QString & strPassword - QString object of password
 *			const QString strDomain - QString object of domain name
 * 		Out: bool - result of authentification
 * @brief Try to authentificate user in local system
 *
 * @author artemr
 */

bool CAuth::AuthUser(const QString & strUserName,
					 const QString & strPassword,
					 const QString & strDomain)
{
  PSID   pSid       = NULL;
  DWORD  cbSid      = 0;
  LPWSTR wszDomain  = NULL;
  DWORD  cchDomain  = 0;
  SID_NAME_USE sidType = SidTypeUser;

	if (!LookupAccountName(NULL,
			(LPWSTR)strUserName.utf16(),
			pSid, &cbSid, wszDomain, &cchDomain, &sidType))
	{
		int nError = GetLastError();

		//Insufficient buffer size is valid error in our case
		if (nError != ERROR_INSUFFICIENT_BUFFER)
		{
			WRITE_TRACE(DBG_FATAL, "LookupAccountName(): an error was occurred %d\n", nError);

			return false;
		}
	}

	if(!LogonUser((LPWSTR)strUserName.utf16(),
		(LPWSTR)strDomain.utf16(),
		(LPWSTR)strPassword.utf16(),
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_DEFAULT,
		&m_lphToken))
	{
		WRITE_TRACE(DBG_FATAL, "LogonUser(): an error was occurred %d\n", GetLastError());
		return false;
	}
	return true;
}

bool CAuth::AuthUser(quint32 ProcessId, QString& outUserName, QString& outUserDomain )
{
	HANDLE hRemoteProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, ProcessId);
	if (hRemoteProcess)
	{
		HANDLE UserId = NULL;
		if (OpenProcessToken(hRemoteProcess,
													TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
													&UserId))
		{
			CloseHandle(hRemoteProcess);

			if ( GetUserAndDomainByAuthToken ( UserId, outUserName, outUserDomain ) )
			{
				m_lphToken = UserId;
				return true;
			}
			WRITE_TRACE(DBG_FATAL, "Couldn't to GetUserAndDomainByAuthToken(). Error code: %d", GetLastError());
		}
		else
			WRITE_TRACE(DBG_FATAL, "Failed to open security token of the remote process. Error id: %d", GetLastError());
		CloseHandle(hRemoteProcess);
	}
	else
		WRITE_TRACE(DBG_FATAL, "Couldn't to open remote process handle. Error code: %d", GetLastError());
	return (false);
}

bool CAuth::AuthUserSSPI(const QString & userName,
						 const QString & password,
						 const QString & strDomain)
{
	PSID   pSid       = NULL;
	DWORD  cbSid      = 0;
	LPWSTR wszDomain  = NULL;
	DWORD  cchDomain  = 0;
	SID_NAME_USE sidType = SidTypeUser;

	if (!LookupAccountName(NULL, (LPWSTR)userName.utf16(), pSid, &cbSid, wszDomain, &cchDomain, &sidType))
	{
		DWORD nError = GetLastError();
		//Insufficient buffer size is valid error in our case
		if (nError != ERROR_INSUFFICIENT_BUFFER)
		{
			WRITE_TRACE(DBG_FATAL, "LookupAccountName(): an error was occurred %d\n", nError);
			return false;
		}
	}

	if (!SSPLogonUser(
		(LPWSTR)strDomain.utf16(),
		(LPWSTR)userName.utf16(),
		(LPWSTR)password.utf16(),
		&m_lphToken))
	{
		WRITE_TRACE(DBG_FATAL, "SSPLogonUser(): an error was occurred %d\n", GetLastError());
		return false;
	}

	return true;
}

/**
 * @params
 *		IN:	const QString & strUserName - QString object of username
 *			const QString & strPassword - QString object of password
 *			const QString strFileName - QString object with file name
 *			const QString strDomain - QString object of domain name
 * 		Out: bool - result of authentification
 * @brief Try to check file object for user access
 *
 * @author artemr
 */

// Check file permissions
CAuth::AccessMode  CAuth::CheckFile(const QString&,
                               const QString& strFileName,
                               const QString,
                               const QString)
{
	CAuth::AccessMode mask = 0;
	// access for FAT file system
	if(IsFATFileSystem(strFileName))
	{
		QFileInfo file(strFileName);
		if ( file.isReadable() )
			mask |= CAuth::fileMayRead;
		if ( file.isWritable() )
			mask |= CAuth::fileMayWrite;
		// FIX ME IT internal - file.isExecutable-is works in windows
		// only on .com and .exe files
		//if ( file.isExecutable() )
		mask |= CAuth::fileMayExecute;
		return mask;

	}

	// Permission checks
	unsigned long uiDeninedmask = 0;
	unsigned long uiAllowedMask = 0;

	// get dacl information from file object
	QString outUserName;
	QString outUserDomain;
	PSECURITY_DESCRIPTOR sd = NULL;
	if ( !GetUserAndDomainByAuthToken ( m_lphToken, outUserName, outUserDomain ) )
		return mask;

	try
	{
		PACL pDacl = NULL;
		DWORD dwError = GetNamedSecurityInfo((LPTSTR)strFileName.utf16(),
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION,
			NULL,
			NULL,
			&pDacl,
			NULL,
			&sd);

		if(dwError != ERROR_SUCCESS)
			throw "GetNamedSecurityInfo() failed";

		// Fix crash by bug #4006
		// Note: GetAclInformation(NULL, ...) causes an access violation
		if ( !pDacl )
			throw "pDacl == NULL. Return to prevent access violation. FS may be not on ntfs.";

		//AccessCheck(sd,m_lphToken,0x00020000,)
		// try to get ace in acl
		ACL_SIZE_INFORMATION stAclInfo;
		stAclInfo.AceCount = 0; // Assume NULL DACL.
		stAclInfo.AclBytesFree = 0;
		stAclInfo.AclBytesInUse = sizeof(ACL);

		if(!GetAclInformation(pDacl,&stAclInfo,sizeof(ACL_SIZE_INFORMATION),AclSizeInformation))
			throw "GetAclInformation() failed";

		// cycl on all ace
		for(DWORD i = 0 ; i < stAclInfo.AceCount; i++)
		{
			PSID	pEntriesSid = NULL;
			void *	lpAce;

			if(!GetAce(pDacl,i,&lpAce))
				continue;

			// switch for all possible ace type and get it sids
			switch(((PACCESS_ALLOWED_ACE)(lpAce))->Header.AceType)
			{
			case ACCESS_ALLOWED_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
				break;
			case ACCESS_DENIED_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_DENIED_ACE)(lpAce))->SidStart);
				break;
			case SYSTEM_AUDIT_ACE_TYPE:
				pEntriesSid = (PSID)(&((PSYSTEM_AUDIT_ACE)(lpAce))->SidStart);
				break;
			case SYSTEM_ALARM_ACE_TYPE:
				pEntriesSid = (PSID)(&((PSYSTEM_ALARM_ACE)(lpAce))->SidStart);
				break;
			default:
				pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
			}//switch

			// check user membership in token
			BOOL bFound = false;

			// this operation try to check membership - it may be failed if user not impersonated
			// or impersonated user has not access to call it function
			if(!CheckTokenMembership(GetTokenHandle(),pEntriesSid,&bFound))
			{
				DWORD dwError = GetLastError();

				LOG_MESSAGE(DBG_FATAL, "Couldn't check token membership. Error code: %d, %s"
					, dwError
					, QSTR2UTF8( Prl::GetLastErrorAsString() ) );

				// if impersonated user have not permissions for call CheckTokenMembership -
				// try revert impersonation and impersonate again after call this code
				if( dwError != ERROR_ACCESS_DENIED && dwError != ERROR_NO_IMPERSONATION_TOKEN )
					continue;

				//////////////////////////////////////////////////////////////////////////
				// access denied - user full sucker or not impersonated
				//////////////////////////////////////////////////////////////////////////
				PTOKEN_USER  ptiUser  = NULL;
				DWORD        cbti     = 0;

				// Obtain the size of the user information in the token.
				GetTokenInformation(GetTokenHandle(), TokenUser, NULL, 0, &cbti);
				// Call should have failed due to zero-length buffer.
				if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
					continue;

				// Allocate buffer for user information in the token.
				ptiUser = (PTOKEN_USER) malloc(cbti);
				if (!ptiUser)
					continue;

				// Retrieve the user information from the token.
				if (!GetTokenInformation(GetTokenHandle(), TokenUser, ptiUser, cbti, &cbti))
				{
					free(ptiUser);
					continue;
				}

				if (EqualSid(pEntriesSid,ptiUser->User.Sid))
				{
					bFound = true;
				}
				else
				{
					PTOKEN_GROUPS pGroups = NULL;

					//RevertToSelf(); // revert

					DWORD dwGroupInfoNeeded = 0;
					if(!GetTokenInformation(GetTokenHandle(),TokenGroups,NULL,0,&dwGroupInfoNeeded))
						if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
						{
							//ImpersonateLoggedOnUser(GetTokenHandle());
							continue;
						}
						pGroups = (PTOKEN_GROUPS)malloc(dwGroupInfoNeeded);

						if(!GetTokenInformation(GetTokenHandle(),TokenGroups,pGroups,dwGroupInfoNeeded,&dwGroupInfoNeeded))
						{
							//ImpersonateLoggedOnUser(GetTokenHandle());
							free(pGroups);
							continue;
						}

						for (DWORD i = 0 ; i < pGroups->GroupCount ; i++)
						{
							if (EqualSid(pEntriesSid,pGroups->Groups[i].Sid) &&
								(pGroups->Groups[i].Attributes & (SE_GROUP_ENABLED)))
							{
								bFound = true;
								break;
							}

						}
						//ImpersonateLoggedOnUser(GetTokenHandle());
						free(pGroups);
				}
				free(ptiUser);
			}// if(!CheckTokenMembership

			if(bFound)
			{
				// we found user member group - set masks
				// check ace type and set access of this ace of
				if (((PACCESS_ALLOWED_ACE)(lpAce))->Header.AceType == ACCESS_DENIED_ACE_TYPE)
				{
					// check flags of access
					uiDeninedmask |= ((PACCESS_DENIED_ACE)(lpAce))->Mask;
				}
				// check ace type and set access of this ace of
				if (((PACCESS_ALLOWED_ACE)(lpAce))->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
				{
					// check flags of access
					uiAllowedMask |= ((PACCESS_ALLOWED_ACE)(lpAce))->Mask;
				}
			}// if(bFound)

		}// for

		// fill finally access mask of file object
		mask = 0;

		// fill first allowed mask
		mask |= ConvertToAccessMode( uiAllowedMask );

		// clear access mask bits if denine was present
		mask &= ~ ConvertToAccessMode( uiDeninedmask );
	}
	catch( const char* err )
	{
		WRITE_TRACE( DBG_INFO, "Error in CAuth::CheckFile() '%s' for file '%s'"
			, err? err : ""
			, QSTR2UTF8(strFileName)
			);
	}

	if (sd)
	{
		LocalFree(sd);
	}

	return mask;
}

/**
 * @params
 *		IN:	CAuth::AccessMode ulFilePermissions - file permition flags
 *			const QString & strUserName - QString object of username
 *			const QString & strPassword - QString object of password
 *			const QString strFileName - QString object with file name
 *			const QString strDomain - QString object of domain name
 * 		Out: bool - result of authentification
 * @brief Try to check file object for user access
 *
 * @author artemr
 */

bool CAuth::SetFilePermissions(CAuth::AccessMode ulFilePermissions,
						const QString & strUserName,
						const QString & strFileName,
						const QString strPassword,
						const QString strDomain,
						uint uiInheritanceFlags)
{
	PACL pDacl					= NULL;
	PSECURITY_DESCRIPTOR sd		= NULL;
	PSID				pUserSid = NULL;
	bool	bUserHasAccess = false;
	DWORD dwError = 0;
	BOOL	bUser = TRUE;

	// try to auth user
	HANDLE	phToken;
	BOOL bGroup = false;
   bool flg_phTokenInited=false;

   // must be throw while not tested. bug#153 [Dispatcher crashes when VM directory had been removed]
	LOG_MESSAGE (DBG_FATAL, "Its not tested method. application can crashes in call SetEntriesInAcl(). look bug#153 for more info");
	throw "Its dangerous method: application can crashes";

	if (m_lphToken)
	{
		phToken = m_lphToken;
	}
	else
	{
		if(strPassword.isEmpty())
			bGroup = true;
		else
		{
			if(!LogonUser((LPWSTR)strUserName.utf16(),
				(LPWSTR)strDomain.utf16(),
				(LPWSTR)strPassword.utf16(),
				LOGON32_LOGON_INTERACTIVE,
				LOGON32_PROVIDER_DEFAULT,
				&phToken))
			{
				return false;
			}
         flg_phTokenInited=true;
		}
	}

	// get dacl information from file object
	dwError = GetNamedSecurityInfo((LPTSTR)strFileName.utf16(),
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		&pDacl,
		NULL,
		&sd);

	if(dwError != ERROR_SUCCESS)
		return false;

	// try to get ace in acl
	ACL_SIZE_INFORMATION stAclInfo;
	stAclInfo.AceCount = 0; // Assume NULL DACL.
	stAclInfo.AclBytesFree = 0;
	stAclInfo.AclBytesInUse = sizeof(ACL);

	// Fix crash by bug #4006
	// Note: GetAclInformation(NULL, ...) causes an access violation
	if ( !pDacl )
	{
		WRITE_TRACE(DBG_FATAL, "pDacl == NULL. Return to prevent access violation."
			"file [%s] (may be not on ntfs) "
			, QSTR2UTF8( strFileName ) );
		return false;
	}
	if(!GetAclInformation(pDacl,&stAclInfo,sizeof(ACL_SIZE_INFORMATION),AclSizeInformation))
	{
		return false;
	}
	// cycl on all ace
	for(DWORD i = 0 ; i < stAclInfo.AceCount; i++)
	{
		TCHAR	szUser[MAX_USER_NAME];
		TCHAR	szDomain[MAX_DOMAIN_NAME];
		PSID	pEntriesSid = NULL;
		DWORD	dwUserLength;
		DWORD	dwDomainLength;
		void *	lpAce;
		SID_NAME_USE	SidType;

		if(!GetAce(pDacl,i,&lpAce))
			return false;

		dwUserLength = sizeof(szUser);
		dwDomainLength = sizeof(szDomain);
		// switch for all possible ace type and get it sids
		switch(((PACCESS_ALLOWED_ACE)(lpAce))->Header.AceType)
		{
			case ACCESS_ALLOWED_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
				break;
			case ACCESS_DENIED_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_DENIED_ACE)(lpAce))->SidStart);
				break;
			case SYSTEM_AUDIT_ACE_TYPE:
				pEntriesSid = (PSID)(&((PSYSTEM_AUDIT_ACE)(lpAce))->SidStart);
				break;
			case SYSTEM_ALARM_ACE_TYPE:
				pEntriesSid = (PSID)(&((PSYSTEM_ALARM_ACE)(lpAce))->SidStart);
				break;
			default:
				pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
				break;
		}
		// get other parameters from sid
		if(LookupAccountSid(NULL,
			pEntriesSid,
			szUser,
			&dwUserLength,
			szDomain,
			&dwDomainLength,
			&SidType))
		{
			// if it user sid
			if(SidType == SidTypeUser)
			{
				if( strUserName == UTF16_2QSTR( szUser ) )
				{
					bUserHasAccess = true;
					pUserSid = pEntriesSid;
					bUser = TRUE;
					break;
				}
			}
			else
			{
				// check user membership in group or domain
				BOOL bMember;

				if (bGroup)
				{
					WCHAR Name[256+1];
					WCHAR DomainName[256+1];
					unsigned long cchName = 256+1;
					unsigned long cchDomainName = 256+1;
					SID_NAME_USE snu;

					BOOL bSuccess = LookupAccountSidW(
						NULL,
						pEntriesSid,
						Name,
						&cchName,
						DomainName,
						&cchDomainName,
						&snu
						);

					if (bSuccess && (strUserName == UTF16_2QSTR((const ushort*)Name)))
					{
						bUserHasAccess = true;
						pUserSid = pEntriesSid;
						bUser = FALSE;
						break;
					}
					else
						continue;
				}

				if(!CheckTokenMembership(phToken,pEntriesSid,&bMember))
				{
					dwError = GetLastError();
					return false;
				}
				else
					if(bMember)
					{
						bUserHasAccess = true;
						pUserSid = pEntriesSid;
						bUser = FALSE;
					}
			}
		}
	}

	if(!bUserHasAccess)
		return false;

	// start adding new ace to acl

	EXPLICIT_ACCESS		ea;
	// fill EXPLICIT_ACCESS structure with data
	// set access permitions for new ace
	ea.grfAccessPermissions = ((ulFilePermissions&fileMayExecute) << 26)|
		((ulFilePermissions&fileMayRead) << 30)|
		((ulFilePermissions&fileMayWrite) << 28);
	// set access mode
	ea.grfAccessMode		= SET_ACCESS;
	// set inhertance
	ea.grfInheritance		= uiInheritanceFlags;
	// set form of data - SID
	ea.Trustee.TrusteeForm	= TRUSTEE_IS_SID;
	// set type of data - User
	ea.Trustee.TrusteeType	= bUser?TRUSTEE_IS_USER:TRUSTEE_IS_GROUP;
	// set SID
	ea.Trustee.ptstrName	= (LPTSTR)pUserSid;

	PACL	pNewAcl = NULL;
	BOOL	bRes;

	// save new ace in current Dacl
	bRes = SetEntriesInAcl(1,&ea,pDacl,&pNewAcl);
	if(bRes != ERROR_SUCCESS)
		return false;
	PSECURITY_DESCRIPTOR	pSd = NULL;
	pSd = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if(!pSd)
		return false;
	// initialize descriptor
	bRes = InitializeSecurityDescriptor(pSd,SECURITY_DESCRIPTOR_REVISION);
	if(!bRes)
		return false;
	// set Dacl into Descriptor
	bRes = SetSecurityDescriptorDacl(pSd,TRUE,pNewAcl,FALSE);
	if(!bRes)
		return false;
	// set new permitions for file object
	bRes = ApplyPermissionsToFile(strFileName, pSd);
	if(!bRes)
		return false;

	// free all data
	LocalFree(pSd);
	LocalFree(sd);
	LocalFree(pNewAcl);
	if(flg_phTokenInited && !strPassword.isEmpty())
		CloseHandle(phToken);

	return true;
}

typedef BOOL (WINAPI *SetSecurityDescriptorControlFnPtr)(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
	IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet);

/**
 * @params
 *		IN: QString & lpszFileName - QString object with file name string
 *			QString & lpszAccountName - QString object with Account Name string
 *			uint dwAccessMask - flags to add rights see ACCESS_MASK struct
 * 		Out:
 *			bool - function result
 * @brief
 *			function adds access rights of user to file object
 * @author artemr
 */
bool CAuth::AddAccessRights(const QString & lpszFileName, const QString & lpszAccountName,
					 uint dwAccessMask,uint uiInheritanceFlags,bool bUserOrGroup, PRL_RESULT* pOutError)
{
	Q_UNUSED(bUserOrGroup);
	Q_UNUSED(uiInheritanceFlags);
	// cant add access for FAT
	if (IsFATFileSystem(lpszFileName))
		return true;

	CSid sid;
	if (!sid.LoadAccount((LPWSTR)lpszAccountName.utf16()))
		{
		WRITE_TRACE(DBG_FATAL, "error: get account \"%s\" (%d)", QSTR2UTF8(lpszAccountName), GetLastError());
		return false;
		}

	CDacl dacl;

	if (!AtlGetDacl(
		(LPCWSTR)lpszFileName.utf16(),
		SE_FILE_OBJECT,
		&dacl))
	{
		WRITE_TRACE(DBG_FATAL, "error: get Dacl for \"%s\"  (%d)"
			, QSTR2UTF8(lpszFileName), GetLastError() );
		if( pOutError && IS_FILE_NOT_FOUND_ERROR( GetLastError() ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;
		return false;
	}

	if (!dacl.RemoveAces(sid))
	{
		// It is valid case when dacl hasn't ace with this SID.
		LOG_MESSAGE( DBG_WARNING, "error: remove all ace for \"%s\"(account=\"%s\") (%d)"
			, QSTR2UTF8(lpszFileName), QSTR2UTF8(lpszAccountName), GetLastError());
//		return false;
	}

	if (!dacl.AddAllowedAce(sid, dwAccessMask, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERITED_ACE))
	{
		WRITE_TRACE(DBG_FATAL, "error: add allowed ace for \"%s\" (account=\"%s\") (%d)"
			, QSTR2UTF8(lpszFileName), QSTR2UTF8(lpszAccountName), GetLastError());
		if( pOutError && IS_FILE_NOT_FOUND_ERROR( GetLastError() ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;
		return false;
	}

	if (!AtlSetDacl(
		(LPCWSTR)lpszFileName.utf16(),
		SE_FILE_OBJECT,
		dacl,
		PROTECTED_DACL_SECURITY_INFORMATION))
	{
		WRITE_TRACE(DBG_FATAL, "error: set Dacl for \"%s\" (%d)", QSTR2UTF8(lpszFileName), GetLastError());

		if( pOutError && IS_FILE_NOT_FOUND_ERROR( GetLastError() ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;
		return false;
	}
	return true;
}

/**
 * @params
 *		IN: QString & strFileName - QString object with file name string
 * 		Out:
 *			bool - function result
 * @brief
 *			function clears list of users from file object
 * @author artemr
 */

bool CAuth::ClearAccessList(const QString & strFileName, PRL_RESULT* pOutError )
{
	// cant clear list for FAT
	if (IsFATFileSystem(strFileName))
		return true;

	PACL pDacl					= NULL;
	PSECURITY_DESCRIPTOR sd		= NULL;
	PSID	pSidOwner=NULL;

	// get dacl information from file object
	DWORD dwError = GetNamedSecurityInfo((LPWSTR)strFileName.utf16(),
		SE_FILE_OBJECT,
		OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
		&pSidOwner,
		NULL,
		&pDacl,
		NULL,
		&sd);

	if(dwError != ERROR_SUCCESS)
	{
		if( pOutError && IS_FILE_NOT_FOUND_ERROR( dwError ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;

		return false;
	}

	// try to get ace in acl
	ACL_SIZE_INFORMATION stAclInfo;

	// Fix crash by bug #4006
	// Note: GetAclInformation(NULL, ...) causes an access violation
	if ( !pDacl || !pSidOwner )
	{
		WRITE_TRACE(DBG_FATAL, "pDacl == NULL || pSidOwner == NULL. Return to prevent access violation."
			"file [%s] (may be not on ntfs) "
			, QSTR2UTF8( strFileName ) );
		if (sd)
		{
			LocalFree(sd);
			sd = NULL;
		}
		return false;
	}
	if(!GetAclInformation(pDacl,&stAclInfo,sizeof(stAclInfo),AclSizeInformation))
	{
		if( pOutError && IS_FILE_NOT_FOUND_ERROR( GetLastError() ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;

		if (sd)
		{
			LocalFree(sd);
			sd = NULL;
		}
		return false;
	}
	bool bRes = false;

	CSid _admins_group_sid = Sids::Admins(), _local_system_sid = Sids::System();

	for(int i = (int)stAclInfo.AceCount-1 ; i >= 0; i--)
	{
		PSID	pEntriesSid = NULL;
		void *	lpAce = NULL;

		if(GetAce(pDacl,i,&lpAce))
		{
			// switch for all possible ace type and get it sids
			switch(((PACCESS_ALLOWED_ACE)(lpAce))->Header.AceType)
			{
			case ACCESS_ALLOWED_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
				break;
			case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_ALLOWED_OBJECT_ACE)(lpAce))->SidStart);
				break;
			case ACCESS_DENIED_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_DENIED_ACE)(lpAce))->SidStart);
				break;
			case ACCESS_DENIED_OBJECT_ACE_TYPE:
				pEntriesSid = (PSID)(&((PACCESS_DENIED_OBJECT_ACE)(lpAce))->SidStart);
				break;
			default:
				goto delete_ace;
			}//switch

			//////////////////////////////////////////////////////////////////////////
			//
			//  Check whether processing ACE belongs to file owner and skip it deletion in this case
			//
			//////////////////////////////////////////////////////////////////////////
			if( EqualSid(pSidOwner, pEntriesSid) ||
				EqualSid(const_cast<SID *>(_admins_group_sid.GetPSID()), pEntriesSid ) ||
				EqualSid(const_cast<SID *>(_local_system_sid.GetPSID()), pEntriesSid) ||
				ContainsSIDInGroup( pEntriesSid, const_cast<SID *>(_admins_group_sid.GetPSID()) ) )
				continue;//Ignore ACE info
		}//if(GetAce(pDacl,i,&lpAce))
	delete_ace:
		bRes =	DeleteAce(pDacl,i);
	}

	PSECURITY_DESCRIPTOR	pSd = NULL;
	pSd = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if(!pSd)
	{
		if (sd)
		{
			LocalFree(sd);
			sd = NULL;
		}
		return false;
	}
	// initialize descriptor
	bRes = InitializeSecurityDescriptor(pSd,SECURITY_DESCRIPTOR_REVISION);
	if(!bRes)
	{
		if (sd)
		{
			LocalFree(sd);
			sd = NULL;
		}
		LocalFree(pSd);
		return false;
	}
	// set Dacl into Descriptor
	bRes = SetSecurityDescriptorDacl(pSd,TRUE,pDacl,FALSE);
	if(!bRes)
	{
		if( pOutError && IS_FILE_NOT_FOUND_ERROR( GetLastError() ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;

		if (sd)
		{
			LocalFree(sd);
			sd = NULL;
		}
		LocalFree(pSd);
		return false;
	}
	// set new permitions for file object
	bRes = ApplyPermissionsToFile(strFileName, pSd, pOutError );
	if(!bRes)
	{
		if (sd)
		{
			LocalFree(sd);
			sd = NULL;
		}
		LocalFree(pSd);
		return false;
	}
	// free all data
	if (sd)
	{
		LocalFree(sd);
		sd = NULL;
	}

	LocalFree(pSd);
	return true;
}

/**
 * @params
 *		IN:
 * 		Out:
 *			void*  - pointer to user token
 * @brief
 *			function returns user token
 * @author artemr
 */

void * CAuth::GetTokenHandle() const
{
	return m_lphToken;
}

bool CAuth::AddAccessRights(const QString & lpszFileName,
					 const QString & lpszAccountName,
					 uint dwAccessMask,
					 uint uiInheritanceFlags,
					 QString & strPassword,
					 QString & strDomain)
{
	Q_UNUSED(strDomain);
	Q_UNUSED(strPassword);
	Q_UNUSED(uiInheritanceFlags);

	/*if(!bInherent)
		return AddAccessRights(lpszFileName,lpszAccountName,dwAccessMask);
	// if it directory set permissions on child object
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	BOOL fFinished = FALSE;
	QString	strTemp;
	strTemp = lpszFileName + "\\*";

	hFind = FindFirstFile((LPCWSTR)strTemp.utf16(), &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		while (!fFinished)
		{
			strTemp = lpszFileName + "\\";
			strTemp += QString().fromUtf16((USHORT*)FindFileData.cFileName);
			AddAccessRights(strTemp,lpszAccountName,dwAccessMask);


			if (!FindNextFile(hFind, &FindFileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
				{
					fFinished = TRUE;
				}
				else
				{
					return false;
				}
			}
		}

		// Close the search handle.

		FindClose(hFind);
	}*/
	bool bRes = AddAccessRights(lpszFileName,lpszAccountName,dwAccessMask);
	DWORD dwPermitions = 0;
	if (bRes)
	{

		if (dwAccessMask & GENERIC_WRITE)
			dwPermitions |= fileMayWrite;
		if (dwAccessMask & GENERIC_READ)
			dwPermitions |= fileMayRead;
		if (dwAccessMask & GENERIC_EXECUTE)
			dwPermitions |= fileMayExecute;
		if (dwAccessMask & GENERIC_ALL)
			dwPermitions = fileMayWrite|fileMayRead|fileMayExecute;

		// must be commented for fix bug#153 [Dispatcher crashes when VM directory had been removed]
		//bRes = SetFilePermissions(dwPermitions,lpszAccountName,lpszFileName,strPassword,strDomain, uiInheritanceFlags);
	}
	return bRes;
}

bool CAuth::getComputerName( QString& outComputerName )
{
	WCHAR buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD nSize = sizeof(buf)/sizeof(buf[0]);

	if ( ::GetComputerNameW(buf, &nSize) )
	{
		outComputerName = UTF16SZ_2QSTR(buf, nSize);
		return true;
	}

	WRITE_TRACE(DBG_FATAL, "Can't get Current Computer Name with the last error code %d",
					GetLastError());
	return false;
}

/**
 * method getCurrentDomain()  Get name of domain.
 * @param outDomainName - [out]	name of domain of current host (if host included to domain)
 * or EMPTY string if current host not included in domain.
 *
 * @return  false if error occurred
 * true	if all ok.
 */

bool CAuth::getCurrentDomain( QString& outDomainName)
{
	bool result = false;

	LPWSTR lpNameBuffer;
	NETSETUP_JOIN_STATUS bufferType;
	NET_API_STATUS nas = NetGetJoinInformation ( NULL, &lpNameBuffer, &bufferType);

	if( nas != NERR_Success )
		WRITE_TRACE(DBG_FATAL, "Can't get Current Domain by error %d", GetLastError() );
	else
	{
		result = true;
		if ( NetSetupDomainName == bufferType )
			outDomainName.setUtf16( lpNameBuffer, (int)wcslen( lpNameBuffer ) );
		else
			outDomainName.clear();
		NetApiBufferFree ( lpNameBuffer );
	}

	return result;
};


/**
 *                the user name and domain name for the user account
 *                associated with the calling thread.
 *
 *  @params   outUserName - a buffer that receives the user name
 *                outUserDomain buffer that receives the domain name
 *
 *	 NOTE:			if token user located on localhst -  outUserDomain is empty
 *
 *  @return TRUE if the function succeeds. Otherwise, FALSE and
 *                GetLastError() will return the failure reason.
 */

bool CAuth::GetUserAndDomainByAuthToken( void* hToken, QString& outUserName, QString& outUserDomain )
{
	TCHAR szUser[1024];
	unsigned long cchUser = sizeof(szUser)/sizeof(TCHAR);

	TCHAR szDomain[1024];
	unsigned long cchDomain = sizeof(szDomain)/sizeof(TCHAR) ;

	ZeroMemory(szUser, sizeof(szUser));
	ZeroMemory(szDomain, sizeof(szDomain));

	bool         fSuccess = false;
	PTOKEN_USER  ptiUser  = NULL;
	DWORD        cbti     = 0;
	SID_NAME_USE snu;

	try
	{
		// Obtain the size of the user information in the token.
		if (GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti)) {

			// Call should have failed due to zero-length buffer.
			throw 1;

		} else {

			// Call should have failed due to zero-length buffer.
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				throw 2;
		}

		// Allocate buffer for user information in the token.
		ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);
		if (!ptiUser)
			throw 3;

		// Retrieve the user information from the token.
		if (!GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti))
			throw 4;

		// Retrieve user name and domain name based on user's SID.
		if (!LookupAccountSid(NULL, ptiUser->User.Sid, (LPWSTR)szUser, &cchUser,
			(LPWSTR)szDomain, &cchDomain, &snu))
			throw 5;

		outUserName = UTF16SZ_2QSTR( szUser, cchUser  );
		outUserDomain = UTF16SZ_2QSTR(szDomain , cchDomain );

		QString computerName;
		if ( ! getComputerName( computerName ) )
			throw 7;

		if ( 0==computerName.compare(outUserDomain, Qt::CaseInsensitive )/* is the same */ )
			outUserDomain.clear();

		fSuccess = true;

	}catch( int err )
	{
		WRITE_TRACE(DBG_FATAL, "error %d, catched on point %d", GetLastError(), err);
	}

	// Free resources.
	if (ptiUser)
		HeapFree(GetProcessHeap(), 0, ptiUser);

	return fSuccess;
};

bool CAuth::IsFATFileSystem(const QString & strFilePath)
{

#define FILESYSNAMEBUFSIZE 256
	DWORD dwSysFlags;							// flags that describe the file system
	TCHAR FileSysNameBuf[FILESYSNAMEBUFSIZE];	// buffer for file system name
	TCHAR FATSysNameBuf[] = L"FAT";
	TCHAR FAT32SysNameBuf[] = L"FAT32";
	TCHAR exFATSysNameBuf[] = L"exFAT";

	QString strDrive = strFilePath.left(3);
	GetVolumeInformation((LPCTSTR)(strDrive.utf16()),
									NULL,
									0,
									NULL,
									NULL,
									&dwSysFlags,
									FileSysNameBuf,
									FILESYSNAMEBUFSIZE);

	/*
	* Check, is it FAT16 and image file greater than 2Gb
	*/
	if ( UTF16_2QSTR( FileSysNameBuf ) == UTF16_2QSTR( FATSysNameBuf ) )
		return true;

	if ( UTF16_2QSTR( FileSysNameBuf ) ==UTF16_2QSTR( FAT32SysNameBuf ) )
		return true;

	if ( UTF16_2QSTR( FileSysNameBuf ) ==UTF16_2QSTR( exFATSysNameBuf ) )
		return true;

	return false;
}

bool CAuth::DeleteUserFromAcl(const QString & strFileName,const QString & strAccountName)
{
	PACL pDacl = NULL;
	PSECURITY_DESCRIPTOR sd = NULL;
	DWORD dwError = GetNamedSecurityInfo((LPTSTR)strFileName.utf16(),
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		&pDacl,
		NULL,
		&sd);

	if(dwError != ERROR_SUCCESS)
		return false;

	//AccessCheck(sd,m_lphToken,0x00020000,)
	// try to get ace in acl
	ACL_SIZE_INFORMATION stAclInfo;
	stAclInfo.AceCount = 0; // Assume NULL DACL.
	stAclInfo.AclBytesFree = 0;
	stAclInfo.AclBytesInUse = sizeof(ACL);

	if ( !pDacl )
	{
		if (sd)
		{
			LocalFree(sd);
		}
		return false;
	}
	if(!GetAclInformation(pDacl,&stAclInfo,sizeof(ACL_SIZE_INFORMATION),AclSizeInformation))
	{
		if (sd)
		{
			LocalFree(sd);
		}
		return false;
	}
	// cycl on all ace
	for(int i = (int)stAclInfo.AceCount-1 ; i >= 0; i--)
	{
		TCHAR	szUser[MAX_USER_NAME];
		TCHAR	szDomain[MAX_DOMAIN_NAME];
		PSID	pEntriesSid = NULL;
		DWORD	dwUserLength;
		DWORD	dwDomainLength;
		void *	lpAce;
		SID_NAME_USE	SidType;

		if(!GetAce(pDacl,i,&lpAce))
			continue;

		dwUserLength = sizeof(szUser);
		dwDomainLength = sizeof(szDomain);
		// switch for all possible ace type and get it sids
		switch(((PACCESS_ALLOWED_ACE)(lpAce))->Header.AceType)
		{
		case ACCESS_ALLOWED_ACE_TYPE:
			pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
			break;
		case ACCESS_DENIED_ACE_TYPE:
			pEntriesSid = (PSID)(&((PACCESS_DENIED_ACE)(lpAce))->SidStart);
			break;
		case SYSTEM_AUDIT_ACE_TYPE:
			pEntriesSid = (PSID)(&((PSYSTEM_AUDIT_ACE)(lpAce))->SidStart);
			break;
		case SYSTEM_ALARM_ACE_TYPE:
			pEntriesSid = (PSID)(&((PSYSTEM_ALARM_ACE)(lpAce))->SidStart);
			break;
		default:
			pEntriesSid = (PSID)(&((PACCESS_ALLOWED_ACE)(lpAce))->SidStart);
			break;
		}
		// get other parameters from sid
		if(LookupAccountSid(NULL,
			pEntriesSid,
			szUser,
			&dwUserLength,
			szDomain,
			&dwDomainLength,
			&SidType))
		{
				if( strAccountName == UTF16_2QSTR( szUser ) )
				{

					bool bRes = DeleteAce(pDacl,i);
					PSECURITY_DESCRIPTOR	pSd = NULL;
					pSd = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
					if(!pSd)
					{
						if (sd)
						{
							LocalFree(sd);
							sd = NULL;
						}
						return false;
					}
					// initialize descriptor
					bRes = InitializeSecurityDescriptor(pSd,SECURITY_DESCRIPTOR_REVISION);
					if(!bRes)
					{
						if (sd)
						{
							LocalFree(sd);
							sd = NULL;
						}
						LocalFree(pSd);
						return false;
					}
					// set Dacl into Descriptor
					bRes = SetSecurityDescriptorDacl(pSd,TRUE,pDacl,FALSE);
					if(!bRes)
					{
						if (sd)
						{
							LocalFree(sd);
							sd = NULL;
						}
						LocalFree(pSd);
						return false;
					}
					// set new permitions for file object
					bRes = ApplyPermissionsToFile(strFileName, pSd);
					if(!bRes)
					{
						if (sd)
						{
							LocalFree(sd);
							sd = NULL;
						}
						LocalFree(pSd);
						return false;
					}
					// free all data
					if (sd)
					{
						LocalFree(sd);
						sd = NULL;
					}

					LocalFree(pSd);
					return bRes;

				}
		}

	}

	if (sd)
	{
		LocalFree(sd);
	}
	return false;
}

bool CAuth::isLocalAdministrator(const QString&) const
{
	BOOL bMember = false;
	PSID pSID = NULL;
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;

	if (TRUE == AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSID))
	{
		if (FALSE == CheckTokenMembership(NULL, pSID, &bMember))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_ACCESS_DENIED)
			{
				PTOKEN_GROUPS groups = NULL;
				DWORD size = 0;
				if (FALSE == GetTokenInformation(GetTokenHandle(), TokenGroups, NULL, 0, &size)
					&& GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					groups = (PTOKEN_GROUPS) new char[size];
					if (TRUE == GetTokenInformation(GetTokenHandle(), TokenGroups, groups, size, &size))
					{
						for (DWORD i = 0 ; i < groups->GroupCount ; i++)
						{
							if (EqualSid(pSID, groups->Groups[i].Sid) &&
								(groups->Groups[i].Attributes & SE_GROUP_ENABLED) != 0)
								bMember = true;
						}
					}
					delete [] groups;
				}
			}
		}

		if (bMember == FALSE)
		{
			// #110191
			// if the current token does not contain admin SID, it does not mean
			// that the current user is not admin. In Vista by default the token of
			// users in administrator group has the the admin SID filtered. We nee
			// to use the unfiltered token to do the check.

			OSVERSIONINFO osver = {sizeof(OSVERSIONINFO)};

			// XP and below, we are done.
			if ( (GetVersionEx(&osver) == TRUE) && (osver.dwMajorVersion >= 6) )
			{
				CAccessToken token;
				token.Attach( GetTokenHandle() );
				CSid _user_sid;
				bool rc = token.GetUser(&_user_sid);
				token.Detach();

				if ( rc && ContainsSIDInGroup( (PSID)_user_sid.GetPSID(), pSID ) )
					bMember = TRUE;
			}
		}

		FreeSid(pSID);
	}
	return (bMember == TRUE) ? true : false;
}

namespace
{
	CAuth::AccessMode  ConvertToAccessMode( DWORD  winAccessRights )
	{
		CAuth::AccessMode mask = 0;
		// fill finally access mask of file object
		// fill first allowed mask
		if( ! winAccessRights )
			return mask;

		// check first specific rights
		if (winAccessRights & FILE_WRITE_DATA) // write permissions
			mask |= CAuth::fileMayWrite;

		if (winAccessRights & FILE_READ_DATA) // read permissions
			mask |= CAuth::fileMayRead;

		if (winAccessRights & FILE_EXECUTE) // read&execute permissions
			mask |= CAuth::fileMayExecute;

		//mask |= CAuth::fileMayExecute; // this flag for windows always allowed! FIX ME!

		if (winAccessRights & GENERIC_ALL)
			mask |= CAuth::fileMayRead | CAuth::fileMayWrite | CAuth::fileMayExecute;

		if (winAccessRights & GENERIC_READ)
			mask |= CAuth::fileMayRead;

		if (winAccessRights & GENERIC_WRITE)
			mask |= CAuth::fileMayWrite;

		return mask;
	}
} //namespace


/**
 * This method is based on the Microsoft KB Sample 132958
 * http://support.microsoft.com/kb/132958
 *
 * @params
 *		IN:	LPTSTR SystemName - pointer to system name string
 *			LPTSTR AccountName - pointer to Account name string
 *			PSID *Sid - pointer to SID structure
 * 		Out:
 * @brief
 *
 * @author artemr
 */
#ifdef WIN_TEST
BOOL
GetAccountSid(
			  LPTSTR SystemName,
			  LPTSTR AccountName,
			  PSID *Sid
			  )
{
	LPTSTR ReferencedDomain=NULL;
	DWORD cbSid=128;    // initial allocation attempt
	DWORD cbReferencedDomain=16; // initial allocation size
	SID_NAME_USE peUse;
	BOOL bSuccess=FALSE; // assume this function will fail

	__try {

		//
		// initial memory allocations
		//
		if((*Sid=HeapAlloc(
			GetProcessHeap(),
			0,
			cbSid
			)) == NULL) __leave;

		if((ReferencedDomain=(LPTSTR)HeapAlloc(
			GetProcessHeap(),
			0,
			cbReferencedDomain*sizeof(TCHAR)
			)) == NULL) __leave;

		//
		// Obtain the SID of the specified account on the specified system.
		//
		while(!LookupAccountName(
			SystemName,         // machine to lookup account on
			AccountName,        // account to lookup
			*Sid,               // SID of interest
			&cbSid,             // size of SID
			ReferencedDomain,   // domain account was found on
			&cbReferencedDomain,
			&peUse
			)) {
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					//
					// reallocate memory
					//
					if((*Sid=HeapReAlloc(
						GetProcessHeap(),
						0,
						*Sid,
						cbSid
						)) == NULL) __leave;

					if((ReferencedDomain=(LPTSTR)HeapReAlloc(
						GetProcessHeap(),
						0,
						ReferencedDomain,
						cbReferencedDomain
						)) == NULL) __leave;
				}
				else __leave;
		}

		//
		// Indicate success.
		//
		bSuccess=TRUE;

	} // finally
	__finally {

		//
		// Cleanup and indicate failure, if appropriate.
		//

		HeapFree(GetProcessHeap(), 0, ReferencedDomain);

		if(!bSuccess) {
			if(*Sid != NULL) {
				HeapFree(GetProcessHeap(), 0, *Sid);
				*Sid = NULL;
			}
		}

	} // finally

	return bSuccess;
}

/**
 * it is temporal functions for testing dispatcher with start as process
 */
bool CAuth::AddUserToAdminGroup(QString & strUser)
{
	LSA_HANDLE	hLsa;
	OpenPolicy(NULL,POLICY_ALL_ACCESS,&hLsa);
	PSID	pSid;
	GetAccountSid(NULL,strUser.utf16(),&pSid);
	NetLocalGroupAddMember(0,L"Administrators",pSid);
	HeapFree(GetProcessHeap(), 0, pSid);
	LsaClose(hLsa);
}

bool CAuth::RemoveUserFromAdminGroup(QString & strUser)
{
	LSA_HANDLE	hLsa;
	OpenPolicy(NULL,POLICY_ALL_ACCESS,&hLsa);
	PSID	pSid;
	GetAccountSid(NULL,strUser.utf16(),&pSid);
	NetLocalGroupDelMember(0,L"Administrators",pSid);
	HeapFree(GetProcessHeap(), 0, Sid);
	LsaClose(hLsa);
}

#endif

namespace{

	void _DisplayAccessMask( ACCESS_MASK Mask )
	{
		QString qsMask = "";
		if ( ((Mask & GENERIC_ALL) == GENERIC_ALL) ||
			((Mask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS) )
		{
			WRITE_TRACE( DBG_DEBUG, "Effective Allowed Access Mask : 0x%08X %s", Mask, "Full control" );
			return;
		}
		if ( ((Mask & GENERIC_READ) == GENERIC_READ) ||
			((Mask & FILE_GENERIC_READ) == FILE_GENERIC_READ) )
			qsMask += "Read ";
		if ( ((Mask & GENERIC_WRITE) == GENERIC_WRITE) ||
			((Mask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) )
			qsMask += "Write ";
		if ( ((Mask & GENERIC_EXECUTE) == GENERIC_EXECUTE) ||
			((Mask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) )
			qsMask += "Execute";
		WRITE_TRACE( DBG_DEBUG, "Effective Allowed Access Mask : 0x%08X %s", Mask, QSTR2UTF8(qsMask) );
	}

	DWORD _GetAccessMask(
		AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient,
		PSECURITY_DESCRIPTOR pSd,
		PACCESS_MASK /*OUT*/ permMask )
	{
		AUTHZ_ACCESS_REQUEST AccessRequest = { 0 };
		AUTHZ_ACCESS_REPLY AccessReply = { 0 };
		BYTE Buffer[1024];
		BOOL bRes = FALSE;  // assume error
		DWORD dwLastError = ERROR_SUCCESS;

		//  Do AccessCheck.
		AccessRequest.DesiredAccess = MAXIMUM_ALLOWED;
		AccessRequest.PrincipalSelfSid = NULL;
		AccessRequest.ObjectTypeList = NULL;
		AccessRequest.ObjectTypeListLength = 0;
		AccessRequest.OptionalArguments = NULL;

		RtlZeroMemory( Buffer, sizeof(Buffer) );
		AccessReply.ResultListLength = 1;
		AccessReply.GrantedAccessMask = ( PACCESS_MASK )( Buffer );
		AccessReply.Error = ( PDWORD )( Buffer + sizeof(ACCESS_MASK) );

		bRes = AuthzAccessCheck( 0,
							  hAuthzClient,
							  &AccessRequest,
							  NULL,
							  pSd,
							  NULL,
							  0,
							  &AccessReply,
							  NULL );
		if ( bRes )
			*permMask = *( PACCESS_MASK )( AccessReply.GrantedAccessMask );
		else
		{
			dwLastError = GetLastError();
			WRITE_TRACE( DBG_FATAL, "AuthzAccessCheck failed with %d", dwLastError );
		}
		return dwLastError;
	}

	DWORD _GetEffectiveRights(
		AUTHZ_RESOURCE_MANAGER_HANDLE hManager,
		PSECURITY_DESCRIPTOR pSd,
		PSID pSid,
		PACCESS_MASK /*OUT*/ permMask )
	{
		BOOL bRes = FALSE;
		DWORD dwLastError = ERROR_SUCCESS;
		LUID unusedId = { 0 };
		AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext = NULL;

		if ( pSid != NULL )
		{
			bRes = AuthzInitializeContextFromSid(
								0,
								pSid,
								hManager,
								NULL,
								unusedId,
								NULL,
								&hAuthzClientContext );
			if ( bRes )
			{
				dwLastError = _GetAccessMask( hAuthzClientContext, pSd, permMask );
				AuthzFreeContext( hAuthzClientContext );
			}
			else
			{
				dwLastError = GetLastError();
				WRITE_TRACE( DBG_FATAL, "AuthzInitializeContextFromSid failed with %d", GetLastError() );
			}
	   }
	   return dwLastError;
	}

	DWORD getEffectiveRightsBySid(
		PSECURITY_DESCRIPTOR pSd,
		PSID pSid,
		PACCESS_MASK /*OUT*/ permMask )
	{
		AUTHZ_RESOURCE_MANAGER_HANDLE hManager;
		BOOL bRes = FALSE;
		DWORD dwLastError = ERROR_SUCCESS;

		bRes = AuthzInitializeResourceManager(
								AUTHZ_RM_FLAG_NO_AUDIT,
								NULL, NULL, NULL, NULL, &hManager );

	   if ( bRes )
	   {
			dwLastError =_GetEffectiveRights( hManager, pSd, pSid, permMask );
			AuthzFreeResourceManager( hManager );
			_DisplayAccessMask( *permMask );
	   }
	   else
		{
			dwLastError = GetLastError();
			WRITE_TRACE( DBG_FATAL, "AuthzInitializeResourceManager failed with %d", dwLastError );
		}
	   return dwLastError;
	}

	DWORD getEffectiveRightsForUser(
		PSECURITY_DESCRIPTOR pSd,
		PACCESS_MASK /*OUT*/ permMask,
		CAuth* pAuth )
	{
		DWORD dwAccessDesired = MAXIMUM_ALLOWED;
		HANDLE hToken;
		PRIVILEGE_SET PrivilegeSet;
		DWORD dwPrivSetSize = sizeof( PRIVILEGE_SET );
		GENERIC_MAPPING GenericMap;
		BOOL fAccessGranted=FALSE;
		DWORD dwLastError = ERROR_SUCCESS;

		if ( !pAuth->Impersonate() )
			return GetLastError();

		// Get an impersonation token with the client's security context.
		if ( !OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken ) )
		{
			dwLastError = GetLastError();
			WRITE_TRACE( DBG_FATAL, "OpenThreadToken failed with %d", dwLastError );
			goto Cleanup;
		}

		// Use the GENERIC_MAPPING structure to convert any
		// generic access rights to object-specific access rights.
		MapGenericMask( &dwAccessDesired, &GenericMap );

		// Check the client's access rights.
		 if( !AccessCheck(
					pSd,                 // security descriptor to check
					hToken,              // impersonation token
					dwAccessDesired,     // requested access rights
					&GenericMap,            // pointer to GENERIC_MAPPING
					&PrivilegeSet,       // receives privileges used in check
					&dwPrivSetSize,      // size of PrivilegeSet buffer
					permMask,    // receives mask of allowed access rights
					&fAccessGranted) )   // receives results of access check
		{
			dwLastError = GetLastError();
			WRITE_TRACE( DBG_FATAL, "AccessCheck failed with %d", dwLastError );
			goto Cleanup;
		 }

	Cleanup:

		 pAuth->RevertToSelf();

		if ( hToken != INVALID_HANDLE_VALUE )
			CloseHandle( hToken );

		_DisplayAccessMask( *permMask );

		return dwLastError;
	}
}

bool CAuth::GetCommonFilePermissions( const QString& fileName
									, CAuth::AccessMode& ownerPermissions
									, CAuth::AccessMode& othersPermissions
									, bool& flgMixedOthersPermission
									, CAuth* pCurrentUser
									, PRL_RESULT* pOutError )
{
	PRL_ASSERT( pCurrentUser );
	if ( !pCurrentUser )
		return false;

	SetLastError(ERROR_SUCCESS);

	DWORD dwLastError = ERROR_SUCCESS;
	bool retValue = false;

	ownerPermissions = othersPermissions = 0;
	flgMixedOthersPermission = true;

	// access for FAT file system
	if(IsFATFileSystem(fileName))
	{
		AccessMode mask = 0;

		QFileInfo file(fileName);
		if ( file.isReadable() )
			mask |= CAuth::fileMayRead;
		if ( file.isWritable() )
			mask |= CAuth::fileMayWrite;
		// FIX ME IT internal - file.isExecutable-is works in windows
		// only on .com and .exe files
		//if ( file.isExecutable() )
		mask |= CAuth::fileMayExecute;

		ownerPermissions = othersPermissions = mask;

		return true;
	}

	PSID	pSidOwner=NULL;
	PSID	pSidGroup=NULL;
	PACL	pDacl = NULL;
	PSECURITY_DESCRIPTOR sd = NULL;

	try
	{

		// get security information from file object
		dwLastError = GetNamedSecurityInfo((LPTSTR)fileName.utf16(),
			SE_FILE_OBJECT,
			OWNER_SECURITY_INFORMATION |  GROUP_SECURITY_INFORMATION  | DACL_SECURITY_INFORMATION,
			&pSidOwner,
			&pSidGroup,
			&pDacl,
			NULL,
			&sd);

		if(dwLastError != ERROR_SUCCESS)
			throw "GetNamedSecurityInfo() failed";

		ACL_SIZE_INFORMATION stAclInfo;
		stAclInfo.AceCount = 0; // Assume NULL DACL.
		stAclInfo.AclBytesFree = 0;
		stAclInfo.AclBytesInUse = sizeof(ACL);

		// Fix crash by bug #4006
		// Note: GetAclInformation(NULL, ...) causes an access violation
		PRL_ASSERT( pSidOwner );
		PRL_ASSERT( pSidGroup );
		PRL_ASSERT( pDacl );

		if ( !pSidOwner || !pSidGroup || !pDacl )
			throw "!pSidOwner || !pSidGroup || !pDacl";

		WRITE_TRACE( DBG_DEBUG, "File name = %s", QSTR2UTF8(fileName) );

		DWORD dwOwnerPermsMask = 0;
		dwLastError = getEffectiveRightsForUser( sd, &dwOwnerPermsMask, pCurrentUser );
		if ( dwLastError != ERROR_SUCCESS )
			throw "getEffectiveRightsForUser() failed";
		ownerPermissions = ConvertToAccessMode( dwOwnerPermsMask );
		WRITE_TRACE( DBG_DEBUG, "Current user" );

		retValue = true;
	}
	catch( const char* err )
	{
		SetLastError(dwLastError);

		WRITE_TRACE(DBG_FATAL, "%s error: %s, errno = %d, path = %s"
			, __FUNCTION__, err, dwLastError, QSTR2UTF8(fileName) );

		if( pOutError && IS_FILE_NOT_FOUND_ERROR( dwLastError ) )
			*pOutError = PRL_ERR_FILE_NOT_FOUND;

		retValue = false;
	}

	if( sd )
		LocalFree( sd );

	return retValue;
}

namespace
{
	bool ContainsSIDInGroup( PSID pMemberSid, PSID pGroupSid )
	{
		bool flgFound = false;

		PRL_ASSERT( pMemberSid );
		PRL_ASSERT( pGroupSid );

		try
		{
			TCHAR  userName[ MAX_SYSNAME_LENGTH +1 ];
			TCHAR  userDomainName [ MAX_SYSNAME_LENGTH +1 ];
			TCHAR  groupName [ MAX_SYSNAME_LENGTH + 1];
			TCHAR  groupDomainName [ MAX_SYSNAME_LENGTH +1 ];

			DWORD dwLen1, dwLen2 ;
			SID_NAME_USE sidMemberType, sidGroupType;

			dwLen1 = dwLen2 = MAX_SYSNAME_LENGTH;
			if( ! LookupAccountSid( NULL, pMemberSid, userName, &dwLen1, userDomainName, &dwLen2, &sidMemberType ) )
				throw "LookupAccountSid( pMemberSid ) failed";
			if( sidMemberType != SidTypeUser )
				throw "sidMemberType != SidTypeUser";

			dwLen1 = dwLen2 = MAX_SYSNAME_LENGTH;
			if( ! LookupAccountSid( NULL, pGroupSid, groupName, &dwLen1, groupDomainName, &dwLen2, &sidGroupType ) )
				throw "LookupAccountSid( pGroupSid ) failed";

			QString fullUserName = QString( "%1" )//QString( "%1@%2" )
				.arg( UTF16_2QSTR( userName ) );
//				.arg( UTF16_2QSTR( userDomainName ) );

			QString fullGroupName = QString( "%1" )//QString( "%1@%2" )
				.arg( UTF16_2QSTR( groupName ) );
//				.arg( UTF16_2QSTR( groupDomainName ) );

			switch( sidGroupType )
			{
			case SidTypeGroup:;
			case SidTypeAlias:;
			case SidTypeWellKnownGroup:;
				break;
			default:
				throw "Invalid sidGroupType";

			};//case

			DWORD entriesRead = 0, totalEntries = 0;

			LPLOCALGROUP_USERS_INFO_0 bufptr = NULL ;

			NET_API_STATUS netError = NetUserGetLocalGroups(
				NULL		//__in     LPCWSTR servername,
				, userName	//, __in     LPCWSTR username,
				, 0			//__in     DWORD level,
				, LG_INCLUDE_INDIRECT //__in   DWORD flags,
				, (LPBYTE*)&bufptr	//__out    LPBYTE* bufptr,
				, MAX_PREFERRED_LENGTH	// __in     DWORD prefmaxlen,
				, &entriesRead			//__out    LPDWORD entriesread,
				, &totalEntries			//__out    LPDWORD totalentries,
				);

			if( netError != NERR_Success )
			{
				if( netError == ERROR_MORE_DATA )
					NetApiBufferFree( (LPBYTE)bufptr );

				LOG_MESSAGE( DBG_DEBUG, "netError = %d", netError );
				throw "NetGroupGetUsers() failed";
			}

			PRL_ASSERT( bufptr );

			LPLOCALGROUP_USERS_INFO_0 pTmpBuf;
			if ((pTmpBuf = bufptr) != NULL)
			{
				for (DWORD i = 0; (i < entriesRead) && (pTmpBuf != NULL); i++,pTmpBuf++)
				{
					PRL_ASSERT(pTmpBuf);
					if (pTmpBuf == NULL)
					{
						LOG_MESSAGE( DBG_DEBUG, "An access violation has occurred");
						break;
					}

					QString grpMember = UTF16_2QSTR( pTmpBuf->lgrui0_name );

					LOG_MESSAGE( DBG_DEBUG, " User '%s' in group = '%s', group to check '%s' "
						, QSTR2UTF8( fullUserName ), QSTR2UTF8( grpMember ), QSTR2UTF8( fullGroupName ) );

					if( fullGroupName == grpMember )
					{
						flgFound = true;
						LOG_MESSAGE( DBG_WARNING, " User '%s' is member of group '%s' "
							, QSTR2UTF8( fullUserName )
							, QSTR2UTF8( fullGroupName ) );
						break;
					}
				}
			}
			if( entriesRead != totalEntries )
				WRITE_TRACE(DBG_FATAL, "warning: NetUserGetGroups(): entriesRead=%d != totalEntries=%d"
								, entriesRead, totalEntries );

			if (bufptr != NULL)
				NetApiBufferFree((LPBYTE)bufptr);
		}
		catch ( const char* err )
		{
			LOG_MESSAGE( DBG_WARNING, "error %d(%s) in %s", GetLastError()
				, QSTR2UTF8( Prl::GetLastErrorAsString() ), err );
			Q_UNUSED( err );
		}

		LOG_MESSAGE( DBG_DEBUG, "found = %s", flgFound? "true" : "false" );
		return flgFound;
	}
} //namespace

/**
 * Clones authorization object
 */
CAuth *CAuth::Clone() const
{
	if (m_lphToken)
	{
		HANDLE lphToken = NULL;
		if (!DuplicateTokenEx(	m_lphToken,
								MAXIMUM_ALLOWED,
								NULL,
								SecurityImpersonation,
								TokenPrimary,
								&lphToken))
		{
			int nError = GetLastError();
			WRITE_TRACE(DBG_FATAL, "DuplicateTokenEx(): an error was occurred %d", nError);
			return (NULL);
		}
		CAuth *pAuthObject = new CAuth;
		pAuthObject->m_lphToken = lphToken;
		return (pAuthObject);
	}
	return (NULL);
}

namespace
{
	/**
	* These functions are based on the Microsoft KB Sample 180548
	* http://support.microsoft.com/kb/180548
	*/

	/**
	* Logon() using SSPI
	*/
	BOOL WINAPI SSPLogonUser(LPTSTR szDomain, LPTSTR szUser, LPTSTR szPassword, PHANDLE phToken)
	{
		AUTH_SEQ    asServer   = {0};
		AUTH_SEQ    asClient   = {0};
		BOOL        fDone      = FALSE;
		BOOL        fResult    = FALSE;
		DWORD       cbOut      = 0;
		DWORD       cbIn       = 0;
		DWORD       cbMaxToken = 0;
		PVOID       pClientBuf = NULL;
		PVOID       pServerBuf = NULL;
		PSecPkgInfo pSPI       = NULL;
		SEC_WINNT_AUTH_IDENTITY ai;

		do
		{
			// Get max token size
			QuerySecurityPackageInfo(_T("NTLM"), &pSPI);
			cbMaxToken = pSPI->cbMaxToken;
			FreeContextBuffer(pSPI);

			// Allocate buffers for client and server messages
			pClientBuf = myheapalloc(cbMaxToken);
			pServerBuf = myheapalloc(cbMaxToken);

			if (!pClientBuf || !pServerBuf)
			{
				WRITE_TRACE(DBG_FATAL, "Memory allocation (HeapAlloc) failed");
				break;
			}

			// Initialize auth identity structure
			ZeroMemory(&ai, sizeof(ai));
#if defined(UNICODE) || defined(_UNICODE)
			ai.Domain = szDomain;
			ai.DomainLength = lstrlen(szDomain);
			ai.User = szUser;
			ai.UserLength = lstrlen(szUser);
			ai.Password = szPassword;
			ai.PasswordLength = lstrlen(szPassword);
			ai.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
			ai.Domain = (unsigned char *)szDomain;
			ai.DomainLength = lstrlen(szDomain);
			ai.User = (unsigned char *)szUser;
			ai.UserLength = lstrlen(szUser);
			ai.Password = (unsigned char *)szPassword;
			ai.PasswordLength = lstrlen(szPassword);
			ai.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

			// Prepare client message (negotiate)
			cbOut = cbMaxToken;
			if (!SSPGenClientContext(&asClient, &ai, NULL, 0, pClientBuf, &cbOut, &fDone))
				break;

			// Prepare server message (challenge)
			cbIn = cbOut;
			cbOut = cbMaxToken;
			if (!SSPGenServerContext(&asServer, pClientBuf, cbIn, pServerBuf, &cbOut, &fDone))
			{
				// Most likely failure: AcceptServerContext fails with SEC_E_LOGON_DENIED
				// in the case of bad szUser or szPassword.
				// Unexpected Result: Logon will succeed if you pass in a bad szUser and
				// the guest account is enabled in the specified domain.
				break;
			}

			// Prepare client message (authenticate) .
			cbIn = cbOut;
			cbOut = cbMaxToken;
			if (!SSPGenClientContext(&asClient, &ai, pServerBuf, cbIn, pClientBuf, &cbOut, &fDone))
				break;

			// Prepare server message (authentication) .
			cbIn = cbOut;
			cbOut = cbMaxToken;
			if (!SSPGenServerContext(&asServer, pClientBuf, cbIn, pServerBuf, &cbOut, &fDone))
				break;

			HANDLE hToken = NULL;
			if (QuerySecurityContextToken(&asServer.hctxt, &hToken) != SEC_E_OK)
				break;

			if (IsGuest(hToken))
			{
				WRITE_TRACE(DBG_FATAL, "SSP logon failed, logged in as Guest");
				CloseHandle(hToken);
				break;
			}

			*phToken = hToken;

			fResult = TRUE;
		}
		while (false);

		// Clean up resources
		if (asClient.fHaveCtxtHandle)
			DeleteSecurityContext(&asClient.hctxt);

		if (asClient.fHaveCredHandle)
			FreeCredentialsHandle(&asClient.hcred);

		if (asServer.fHaveCtxtHandle)
			DeleteSecurityContext(&asServer.hctxt);

		if (asServer.fHaveCredHandle)
			FreeCredentialsHandle(&asServer.hcred);

		if (pClientBuf)
			myheapfree(pClientBuf);

		if (pServerBuf)
			myheapfree(pServerBuf);

		return fResult;
	}

	BOOL IsGuest(HANDLE hToken)
	{
		BOOL fGuest = FALSE;

		PSID pGuestSid = GetUserSidFromWellKnownRid(DOMAIN_USER_RID_GUEST);
		if (pGuestSid)
		{
			// Get user information
			DWORD dwSize = 0;
			TOKEN_USER* pUserInfo = (TOKEN_USER*)RetrieveTokenInformationClass(hToken, TokenUser, &dwSize);
			if (pUserInfo)
			{
				fGuest = EqualSid(pGuestSid, pUserInfo->User.Sid);
				myheapfree(pUserInfo);
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed to retrieve token information with error %d", GetLastError());
			}

			myheapfree(pGuestSid);
		}

		return fGuest;
	}

	/**
	* Retrieves token information by specified class.
	* NOTE : use myheapfree() to free returned buffer
	*/
	LPVOID RetrieveTokenInformationClass(HANDLE hToken, TOKEN_INFORMATION_CLASS InfoClass, LPDWORD lpdwSize)
	{
		// Determine the size of the buffer needed

		DWORD dwSize = 0;
		GetTokenInformation(hToken, InfoClass, NULL, 0, &dwSize);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return NULL;

		// Allocate a buffer for getting token information
		LPVOID pInfo = myheapalloc(dwSize);
		if (!pInfo)
			return NULL;

		// Get token info
		if (!GetTokenInformation(hToken, InfoClass, pInfo, dwSize, &dwSize))
		{
			WRITE_TRACE(DBG_FATAL, "GetTokenInformation failed, error %d", GetLastError());
			myheapfree(pInfo);
			return NULL;
		}

		if (lpdwSize)
			*lpdwSize = dwSize;

		return pInfo;
	}

	/**
	* Returns SID by well-known RID
	* NOTE : use myheapfree() to free returned buffer
	*/
	PSID GetUserSidFromWellKnownRid(DWORD Rid)
	{
		PUSER_MODALS_INFO_2 umi2;
		NET_API_STATUS nas = NetUserModalsGet(NULL, 2/*level*/, (LPBYTE*)&umi2);
		if (nas != NERR_Success)
		{
			WRITE_TRACE(DBG_FATAL, "NetUserModalsGet failed with error code : [%d]", nas);
			SetLastError(nas);
			return NULL;
		}

		UCHAR subAuthCnt = *GetSidSubAuthorityCount(umi2->usrmod2_domain_id);

		// Allocate storage for new Sid. account domain Sid + account Rid
		PSID pSid = (PSID)myheapalloc(GetSidLengthRequired((UCHAR)(subAuthCnt + 1)));
		if (pSid)
		{
			if (InitializeSid(pSid, GetSidIdentifierAuthority(umi2->usrmod2_domain_id), (BYTE)(subAuthCnt + 1)))
			{
				// Copy existing subauthorities from account domain Sid into new Sid
				for (DWORD index = 0; index < subAuthCnt ; ++index)
					*GetSidSubAuthority(pSid, index) = *GetSidSubAuthority(umi2->usrmod2_domain_id, index);

				// Append Rid to new Sid
				*GetSidSubAuthority(pSid, subAuthCnt) = Rid;
			}
		}

		NetApiBufferFree(umi2);

		return pSid;
	}

	/**
	* Routine Description:
	*
	* Takes an input buffer coming from the client and returns a buffer
	* to be sent to the client.  Also returns an indication of whether or
	* not the context is complete.
	*
	* Return Value:
	*
	* Returns TRUE if successful; otherwise FALSE.
	*/
	BOOL SSPGenServerContext(PAUTH_SEQ pAS, PVOID pIn, DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone)
	{
		SECURITY_STATUS ss;
		TimeStamp       tsExpiry;
		SecBufferDesc   sbdOut;
		SecBuffer       sbOut;
		SecBufferDesc   sbdIn;
		SecBuffer       sbIn;
		ULONG           fContextAttr;

		if (!pAS->fInitialized)
		{

			ss = AcquireCredentialsHandle(NULL, _T("NTLM"),
				SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, &pAS->hcred,
				&tsExpiry);
			if (ss < 0)
			{
				WRITE_TRACE(DBG_FATAL, "AcquireCredentialsHandle failed with %08X", ss);
				return FALSE;
			}

			pAS->fHaveCredHandle = TRUE;
		}

		// Prepare output buffer
		sbdOut.ulVersion = 0;
		sbdOut.cBuffers = 1;
		sbdOut.pBuffers = &sbOut;

		sbOut.cbBuffer = *pcbOut;
		sbOut.BufferType = SECBUFFER_TOKEN;
		sbOut.pvBuffer = pOut;

		// Prepare input buffer
		sbdIn.ulVersion = 0;
		sbdIn.cBuffers = 1;
		sbdIn.pBuffers = &sbIn;

		sbIn.cbBuffer = cbIn;
		sbIn.BufferType = SECBUFFER_TOKEN;
		sbIn.pvBuffer = pIn;

		ss = AcceptSecurityContext(&pAS->hcred,
			pAS->fInitialized ? &pAS->hctxt : NULL, &sbdIn, 0,
			SECURITY_NATIVE_DREP, &pAS->hctxt, &sbdOut, &fContextAttr,
			&tsExpiry);
		if (ss < 0)
		{
			WRITE_TRACE(DBG_FATAL, "AcceptSecurityContext failed with %08X", ss);
			return FALSE;
		}

		pAS->fHaveCtxtHandle = TRUE;

		// If necessary, complete token
		if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE)
		{
			ss = CompleteAuthToken(&pAS->hctxt, &sbdOut);
			if (ss < 0)
			{
				WRITE_TRACE(DBG_FATAL, "CompleteAuthToken failed with %08X", ss);
				return FALSE;
			}
		}

		*pcbOut = sbOut.cbBuffer;

		if (!pAS->fInitialized)
			pAS->fInitialized = TRUE;

		*pfDone = !(ss == SEC_I_CONTINUE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE);

		return TRUE;
	}

	/**
	* Routine Description:
	*
	* Optionally takes an input buffer coming from the server and returns
	* a buffer of information to send back to the server.  Also returns
	* an indication of whether or not the context is complete.
	*
	* Return Value:
	*
	* Returns TRUE if successful; otherwise FALSE.
	*/
	BOOL SSPGenClientContext(PAUTH_SEQ pAS, PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
		PVOID pIn, DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone)
	{
		SECURITY_STATUS ss;
		TimeStamp       tsExpiry;
		SecBufferDesc   sbdOut;
		SecBuffer       sbOut;
		SecBufferDesc   sbdIn;
		SecBuffer       sbIn;
		ULONG           fContextAttr;

		if (!pAS->fInitialized)
		{

			ss = AcquireCredentialsHandle(NULL, _T("NTLM"),
				SECPKG_CRED_OUTBOUND, NULL, pAuthIdentity, NULL, NULL,
				&pAS->hcred, &tsExpiry);
			if (ss < 0)
			{
				WRITE_TRACE(DBG_FATAL, "AcquireCredentialsHandle failed with %08X", ss);
				return FALSE;
			}

			pAS->fHaveCredHandle = TRUE;
		}

		// Prepare output buffer
		sbdOut.ulVersion = 0;
		sbdOut.cBuffers = 1;
		sbdOut.pBuffers = &sbOut;

		sbOut.cbBuffer = *pcbOut;
		sbOut.BufferType = SECBUFFER_TOKEN;
		sbOut.pvBuffer = pOut;

		// Prepare input buffer
		if (pAS->fInitialized)
		{
			sbdIn.ulVersion = 0;
			sbdIn.cBuffers = 1;
			sbdIn.pBuffers = &sbIn;

			sbIn.cbBuffer = cbIn;
			sbIn.BufferType = SECBUFFER_TOKEN;
			sbIn.pvBuffer = pIn;
		}

		ss = InitializeSecurityContext(&pAS->hcred,
			pAS->fInitialized ? &pAS->hctxt : NULL, NULL, 0, 0,
			SECURITY_NATIVE_DREP, pAS->fInitialized ? &sbdIn : NULL,
			0, &pAS->hctxt, &sbdOut, &fContextAttr, &tsExpiry);
		if (ss < 0)
		{
			// <winerror.h>
			WRITE_TRACE(DBG_FATAL, "InitializeSecurityContext failed with %08X", ss);
			return FALSE;
		}

		pAS->fHaveCtxtHandle = TRUE;

		// If necessary, complete token
		if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE)
		{
			ss = CompleteAuthToken(&pAS->hctxt, &sbdOut);
			if (ss < 0)
			{
				WRITE_TRACE(DBG_FATAL, "CompleteAuthToken failed with %08X", ss);
				return FALSE;
			}
		}

		*pcbOut = sbOut.cbBuffer;

		if (!pAS->fInitialized)
			pAS->fInitialized = TRUE;

		*pfDone = !(ss == SEC_I_CONTINUE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE);

		return TRUE;
	}
}

bool CAuth::Impersonate()
{
	bool result = true;

	if( ! m_ImpersonateCounterStorage.hasLocalData() )
		m_ImpersonateCounterStorage.setLocalData( new ImpersonateCounter( 0 ) );

	ImpersonateCounter*& pImpersonateCounter = m_ImpersonateCounterStorage.localData();
	if( 0 == (*pImpersonateCounter)++ )
    {
		if( m_lphToken == NULL )
			LOG_MESSAGE( DBG_DEBUG, "Don't impersonate user with sign 'isDefaultAppUser()' " );
		else if (!ImpersonateLoggedOnUser(GetTokenHandle()))
		{
            WRITE_TRACE(DBG_FATAL, "Impersonate failed with GetLastError()=%d", GetLastError());
			(*pImpersonateCounter)--; // revert
			result = false;
		}
    }
	LOG_MESSAGE( DBG_DEBUG, "ImpersonateCounter =  %d", *pImpersonateCounter );
	return result;
}

bool CAuth::RevertToSelf()
{
	bool result = true;

	ImpersonateCounter*& pImpersonateCounter = m_ImpersonateCounterStorage.localData();

	PRL_ASSERT( 0 != pImpersonateCounter );
	if( 0 == pImpersonateCounter )
	{
		WRITE_TRACE(DBG_FATAL, "Internal error: [not initialized] calling RevertToSelf without Impersonate()	" );
		return false;
	}

	PRL_ASSERT( *pImpersonateCounter > 0 );
	if( !(*pImpersonateCounter > 0)  )
		WRITE_TRACE(DBG_FATAL, "Internal error: count of call RevertToSelf more then call of Impersonate()	" );

    if ( *pImpersonateCounter && 0 == --(*pImpersonateCounter) )
    {
		if( m_lphToken == NULL )
			LOG_MESSAGE( DBG_DEBUG, "Don't revert impersonate for user with sign 'isDefaultAppUser()' " );
        else if (!::RevertToSelf())
		{
            WRITE_TRACE(DBG_FATAL, "RevertToSelf failed with GetLastError()=%d", GetLastError());
			(*pImpersonateCounter)++; // revert
			result = false;
		}
    }
	LOG_MESSAGE( DBG_DEBUG, "ImpersonateCounter =  %d", *pImpersonateCounter );
	return result;
}
