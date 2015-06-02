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

/**
 * CAuth - class for check user permissions on host.
 * CAuth class.
 */

#ifndef __CAUTH_H__
#define __CAUTH_H__

#ifdef _LIN_
#include <unistd.h>
#endif

#include <QString>
#include <QThreadStorage>

#include <time.h>
#include <prlsdk/PrlErrors.h>

// Class declaration
class CAuth {
#	ifndef _WIN_
		static const char* s_defaultPamService;
#	endif
	//////////////////////////////////////////////////////////////////////////
	//
	// FIXME: NEED REFACTORING:
	//		Required split class CAuth to 2: CAuth and CFilePerm
	//		and move to class 'CFilePerm' all file operations
	//
	//////////////////////////////////////////////////////////////////////////

public:

	typedef unsigned long AccessMode;

    // Permissions
    enum FilePermissions {
        fileMayRead     = 1<<1,  // File can be read
        fileMayWrite    = 1<<2,  // File can be written
        fileMayExecute  = 1<<3,  // File can be executed
        fileIsDirectory = 1<<4, // File is a directory
    };

    enum CheckErrors {
        permDenied   = 1<<5,
        pathNotFound = 1<<6,
        userNotFound = 1<<7,
        otherError   = 1<<16
    };

	CAuth(
#ifndef _WIN_
			const QString & qsService = s_defaultPamService
#endif // not _WIN_
		);

	virtual ~CAuth();

#ifdef _LIN_
	// Call this function only after AuthUser() call, otherwise
	// result is undefined (can be invalid).
	bool isAuthAvailable() const { return m_bAuthAvailable; }
#endif

	// Try to authenticate user
    bool AuthUser(const QString & userName,
				  const QString & password ,
				  const QString & strDomain);

	/**
	 * Authenticates user by specified user id
	 * @param [in] initiator process id
	 * @param [out] variable for storing user name
	 * @param [out] variable for storing user domain
	 * @return sign whether user successfully was authenticate
	 */
	bool AuthUser(quint32 ProcessId, QString& outUserName, QString& outUserDomain );

#ifdef _WIN_
	// Minimum supported : Windows 2000
	bool AuthUserSSPI(const QString & userName,
		const QString & password,
		const QString & strDomain);
#endif

	/**
	* *	on Windows checks if current user is in Local Administrators group
	* * on Linux checks for user id, or ability to exec 'su' or 'sudo'.
	* * on Darwin checks for user id, or ability to exec 'su' or 'sudo'.
	**/
	bool isLocalAdministrator(const QString& user = "") const;

	/*
	 * method getCurrentDomain()  Get name of domain.
	 *	@param outDomainName - [out]	name of domain of current host (if host included to domain)
	 *		or EMPTY string if current host not included in domain.
	 *		Note: for unix return EMPTY string always
	 *  @return  false if error occurred
	 *				true	if all ok.
	 */
	static bool getCurrentDomain( QString& outDomainName);

#ifdef _WIN_
	/*
	 * method getComputerName()  Get name of computer on windows.
	 *	@param outComputerName - [out]	name of current computer
	 *  @return  false if error occurred
	 *		true	if all ok.
	 */
		static bool getComputerName( QString& outComputerName );

		void * GetTokenHandle() const;
		bool Impersonate();
		bool RevertToSelf();
#else
	/**
	 * Returns user id for current user
	 */
	uid_t GetUserId() const;

	/**
	 * Returns primary group id for current user.
	 */
	gid_t GetUserPrimaryGroupId() const;

#endif //_WIN_

    // Check file permissions
    AccessMode CheckFile(const QString & strUserName,
		const QString & strFileName,
		const QString strPassword = "",
		const QString strDomain = "."
		);

	/*
	*	@params   pOutError - optional parameter to store simple error if method failed
	*				NOTE: now pOutError we set only for case "file not found" and IGNORE ANY OTHER ERRORS.
	*/
	static bool GetCommonFilePermissions( const QString& fileName
		, AccessMode& ownerMode
		, AccessMode& othersMode
		, bool& flgMixedOthersPermission
		, CAuth* pCurrentUser
		, PRL_RESULT* pOutError = 0);

	bool SetFilePermissions(AccessMode ulFilePermissions,
		const QString & strUserName,
		const QString & strFileName,
		const QString strPassword = "",
		const QString strDomain = "."
#ifdef _WIN_
		,uint uiInheritanceFlags = 0
#endif
		);
#ifdef _WIN_
	/**
	 *                the user name and domain name for the user account
	 *                associated with the calling thread.
	 *
	 *  @params   outUserName - a buffer that receives the user name
	 *                outUserDomain buffer that receives the domain name
	 *	@params pOutError - optional parameter to store simple error if method failed
	 *			NOTE: now pOutError we set only for case "file not found" and IGNORE ANY OTHER ERRORS.
	 *
	 *  NOTE:			if token user located on localhst -  outUserDomain is empty
	 *
	 *  @return TRUE if the function succeeds. Otherwise, FALSE and
	 *                GetLastError() will return the failure reason.
	 */
	static bool GetUserAndDomainByAuthToken( void* lphToken, QString& outUserName, QString& outUserDomain );

	static bool AddAccessRights(const QString & lpszFileName
					,const QString & lpszAccountName
					,uint dwAccessMask
					,uint uiInheritanceFlags = 0/*NO_INHERITANCE*/
					,bool bUserOrGroup = false
					,PRL_RESULT* pOutError = 0);
	static bool AddAccessRights(const QString & lpszFileName,
					const QString & lpszAccountName,
					uint dwAccessMask,
					uint uiInheritanceFlags,
					QString & strPassword = QString(""),
					QString & strDomain = QString("."));

	//	@params pOutError - optional parameter to store simple error if method failed
	//      	NOTE: now pOutError we set only for case "file not found" and IGNORE ANY OTHER ERRORS.
	static bool ClearAccessList(const QString & strFileName, PRL_RESULT* pOutError = 0);

	static bool DeleteUserFromAcl(const QString & strFileName,const QString & lpszAccountName);

	static bool IsFATFileSystem(const QString & strFilePath);

#endif

#ifdef WIN_TEST
	bool AddUserToAdminGroup(QString & strUser);
	bool RemoveUserFromAdminGroup(QString & strUser);
#endif

	/**
	 * Clones auth object. Returns NULL pointer in case when auth object clone failed
	 */
	CAuth *Clone() const;

#ifndef _WIN_
	/**
	* *	on unix like systems checks if user a member of group
	* * @param [in] user utf8 name
	* * @param [in] group id
	**/
	static bool isUserMemberOfGroup(const QString& user, gid_t gid);
#endif

protected:
#ifdef _WIN_
	/** Internal per-thread counters for preventing subcalls of Impersonate()/RevertToSelf() methods
	* * bug http://bugzilla/show_bug.cgi?id=968
	**/
	typedef quint32 ImpersonateCounter;
	QThreadStorage< ImpersonateCounter* > m_ImpersonateCounterStorage;

	void *	m_lphToken;
#else
	/** Storing user id for UNIX based systems */
	uid_t m_UserId;

	/** Storing user primary group id for UNIX based systems */
	gid_t m_UserPrimaryGroupId;

	/** Pam authorization service for UNIX based systems */
	QString m_qsService;
#endif

#ifdef _LIN_
	bool m_bAuthAvailable;
#endif
};

#endif // __CAUTH_H__
