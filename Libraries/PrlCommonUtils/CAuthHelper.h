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
/// @file CAuthHelper.h
///
/// @brief
///			CAuthHelper class header file.
///
/////////////////////////////////////////////////////////////////////////////

#ifndef AUTHHELPER_H
#define AUTHHELPER_H

#include <QString>

#include "Libraries/CAuth/CAuth.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/Std/SmartPtr.h>

/**
 * @brief
 *		  Class-wrapper for CAuth - basic authentication service
 */

class CAuthHelper;
class CAuthHelper
{
	static const QString s_strLocalDomain;


public:
	struct OwnerWrapper
	{
		OwnerWrapper( CAuthHelper* pAuthHelper );
		OwnerWrapper( const QString& sOwnersFileName );

		CAuthHelper* getAuthHelper() { return m_pOwnersAuthHelper; }
		QString		getFileName() { return m_sOwnersFileName; }
	private:
		CAuthHelper* m_pOwnersAuthHelper;
		// FIXME:
		// NEED REFACTORE THIS PIECE TO STORE OWNER-SID INSTEAD FILENAME
		// AND RECALCULTE OWNER-SID IN RECURSION CALL
		QString		m_sOwnersFileName;
	};
public:

	// Constructor
	CAuthHelper();

	CAuthHelper(QString strUserName);

	// Destructor
	~CAuthHelper();

	// Try to authenticate user
	bool AuthUser(const QString & password);

	// Try to authenticate local user
	bool AuthUser(int UserId);

	// Authenticate local user by this process
	bool AuthUserBySelfProcessOwner();

	// Check file permissions
	CAuth::AccessMode CheckFile(const QString & strFileName);

	CAuth *GetAuth() { return m_pAuth; }

	const QString& getUserName () const;
	const QString& getUserDomain () const;
	// getUserFullName = UserName@domainName
	QString getUserFullName() const;

	QString getHomePath () const;

		  /**
			* On Linux 'root' user represented by an empty string.
			* On others OSes should be the same. So the method helps
			* us to determine if user root (i.e. system, etc.)
			*/
	bool isDefaultAppUser() const;

	bool Impersonate();

	bool RevertToSelf();

	ParallelsDirs::UserInfo getParallelsDirUserInfo();

	/**
	 * *	on Windows convert strUserName, Domain to lower case
	 * * on Unix do nothing.
	 **/
	static void NormalizeCreditals ( QString& strSomeName);

	/**
	* *	on Windows checks if current user is in Local Administrators group
	* * on Linux checks for user id, or ability to exec 'su' or 'sudo'.
	* * on Darwin checks for user id, or ability to exec 'su' or 'sudo'.
	**/
	bool isLocalAdministrator();

	bool isOwnerOfFile( const QString& path );

	// returns user@domain if success or "" if failed
	static QString GetOwnerOfFile( const QString& fileName, bool bUseSlash = false );

	//	@params pOutError - optional parameter to store simple error if method failed
	//      	NOTE: now pOutError we set only for case "file not found" and IGNORE ANY OTHER ERRORS.
	static bool SetOwnerOfFile( const QString& fileName, OwnerWrapper& owner, PRL_RESULT* pOutError = 0 );
#ifdef _WIN_

	static bool LookupAliasFromRid(const QString &strTargetComputer,
		void *pSia,
		unsigned long ulRidType,
		unsigned long ulRid,
		QString &strAlias );

	/**
	 * Generic method for add access rights. Lets to add rights for specified account name
	 * @param path to setup access rights
	 * @param account name for which access rights should be added
	 * @return sign whether rights added successfully
	 */
	bool AddAccessRightsGeneric( const QString &sPath, const QString &sAccountName );
	/**
	 * Adds access rights for Administrators group to specified path
	 * (if path is a directory - rights will be added recursively).
	 * Current process should have take ownership privilege.
	 *
	 * Sample of usage:
	 * CAuthHelper _auth;
	 * if ( !_auth.AuthUserBySelfProcessOwner() ) {...}
	 * if ( !_auth.AddRightsForAdministrators( path ) ) ) {...}
	 *
	 * @param path to the target file or directory
	 * @return sign whether permissions were successfully added to file
	 *
	 * WARNING! This method operates on global process privileges:
	 * SE_BACKUP_NAME, SE_RESTORE_NAME, SE_SECURITY_NAME. Be aware
	 * do not use this call simultaneously at different threads - it
	 * can lead to inconsistent process privileges state.
	 */
	bool AddAccessRightsForAdministrators( const QString &sPath );
	/**
	 * Provided for convenience. Adds access rights for local group Users.
	 */
	bool AddAccessRightsForEveryone( const QString &sPath );

#endif // _WIN_

	/**
	 * Clones authority object into internal own
	 */
	bool CloneAuthObject(const CAuthHelper &auth_helper);

private:
	// Hide copy constructor
	CAuthHelper( const CAuthHelper &obj);

	// hide operator=
		  CAuthHelper& operator=( const CAuthHelper& auth);

	void setUserCreditals( const QString& userName, const QString& userDomain );
	void fillDefaultDomain();

	static bool GetOwnerOfFile( const QString& fileName, QString& outUserName, QString& DomainName );

	static bool SetOwnerOfFile( const QString& fileName, CAuthHelper* pAuthHelper, PRL_RESULT* pOutError = 0 );
	static bool SetOwnerOfFile( const QString& fileName, const QString& sOwnersFile, PRL_RESULT* pOutError = 0 );

private:

	QString m_strUserName; // filled in AuthUser()
	QString m_strUserDomain; // filled in AuthUser()

	QString m_strDefaultDomain;

	CAuth *m_pAuth;

};

/** Simple wrapper around CAuthHelper pointer */

struct CAuthHelperImpersonateWrapper;
typedef SmartPtr<CAuthHelperImpersonateWrapper> CAuthHelperImpersonateWrapperPtr;

struct CAuthHelperImpersonateWrapper {
	CAuthHelperImpersonateWrapper(CAuthHelper *pAuthHelper);
	~CAuthHelperImpersonateWrapper();

	static CAuthHelperImpersonateWrapperPtr create( CAuthHelper *pAuthHelper );

	bool wasImpersonated() const;
private:
	CAuthHelper *m_pAuthHelper;
};

#endif // AUTHHELPER_H
