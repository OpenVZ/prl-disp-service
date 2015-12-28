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
 * CAuth class. (Unix realization)
 */

#include "Interfaces/ParallelsQt.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdio.h>
#include <prlcommon/Logging/Logging.h>

#include <QFile>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>

// pam support
#ifdef _LIN_
#   include <security/pam_appl.h>
#   include <malloc.h>
#endif

#include <QString>
#include "CAuth.h"
#include "CAclHelper.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>

using namespace Parallels;

// System specific functions

// read PAM messages

const char* CAuth::s_defaultPamService = "prl_disp_service";

#ifdef _LIN_

static int pamConvers(int num, const struct pam_message **msgs, struct pam_response **resp, void *data) {

    int i;
    struct pam_response *respons = NULL;
    char *password = (char *)data;

    // check for invalid values
    if (num <= 0)
        return PAM_CONV_ERR;

    // Parase messages
    for (i = 0; i < num; i++) {
        switch (msgs[i]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
			//Allocate responses array just in case of known question
			if (!respons)
			{
				if (!(respons = (struct pam_response *)calloc(num, sizeof(struct pam_response)))) {
					WRITE_TRACE(DBG_FATAL, "Can't allocate memory for PAM responses");
					return PAM_CONV_ERR;
				}
			}

            if (!(respons[i].resp = strdup(password)))
                return PAM_CONV_ERR;
            break;
        default:
            WRITE_TRACE(DBG_FATAL, "Unknown message style: %d", msgs[i]->msg_style);
            return PAM_CONV_ERR;
        }
        respons[i].resp_retcode = 0;
    }

    *resp = respons;

    return PAM_SUCCESS;
}
#endif//_LIN_

// constructor
CAuth::CAuth(const QString & qsService)
: m_UserId(-1)
, m_UserPrimaryGroupId(-1)
, m_qsService(qsService)
#ifdef _LIN_
, m_bAuthAvailable(false)
#endif
{
}

// destructor
CAuth::~CAuth()
{}

bool CAuth::isUserMemberOfGroup(const QString& user, gid_t gid)
{
	setgrent();
	struct group grp, *pgrp = NULL;
	static const int GROUP_ENT_BUFFER_MAX_LENGTH = 4096;
	char buf[GROUP_ENT_BUFFER_MAX_LENGTH] = {0};
	int ret = getgrgid_r(gid,&grp, buf, GROUP_ENT_BUFFER_MAX_LENGTH, &pgrp);
	if (pgrp && ret == 0)
	{
		char **pGroupMembers = pgrp->gr_mem;
		while (pGroupMembers && pGroupMembers[0])
		{
			if (strcmp(*(pGroupMembers)++, user.toUtf8().constData()) == 0)
			{
				endgrent();
				return true;
			}
		}
	}

	endgrent();
	return false;
}

namespace {
bool CheckWhetherUserAtGroup(const QString &sUserName, struct group *pgrp)
{
	char **pGroupMembers = pgrp->gr_mem;
	QByteArray user = sUserName.toUtf8();
	while (pGroupMembers && pGroupMembers[0])
	{
		if (strcmp(*(pGroupMembers)++, user.constData()) == 0)
			return (true);
	}
	return (false);
}

}

bool CAuth::isLocalAdministrator(const QString& user) const
{
	// if user root - return true
	struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
	QByteArray _passwd_strings_buf;
	struct passwd userInfo;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	::getpwnam_r(QSTR2UTF8(user), &userInfo, _passwd_strings_buf.data(), _passwd_strings_buf.size(),
		&lpcUserInfo );
#else
	lpcUserInfo = getpwnam( QSTR2UTF8(user) );
#endif

	if ( lpcUserInfo == 0 )
		return false;

	// Is root ?
	if (lpcUserInfo->pw_uid == 0)
		return true;

	struct group grp, *pgrp = NULL;
	static const int GROUP_ENT_BUFFER_MAX_LENGTH = 4096;
	char buf[GROUP_ENT_BUFFER_MAX_LENGTH] = {0};
	int ret = getgrnam_r("root", &grp, buf, GROUP_ENT_BUFFER_MAX_LENGTH, &pgrp);

	if (pgrp && ret == 0)
		if (CheckWhetherUserAtGroup(user, pgrp))
			return (true);

	ret = getgrnam_r("admin",&grp,buf,GROUP_ENT_BUFFER_MAX_LENGTH,&pgrp);
	if( pgrp && ret == 0 )
		if(CheckWhetherUserAtGroup(user,pgrp))
			return (true);

	ret = getgrnam_r("wheel", &grp, buf, GROUP_ENT_BUFFER_MAX_LENGTH, &pgrp);
	if (pgrp && ret == 0)
		return (CheckWhetherUserAtGroup(user, pgrp));
	return (false);
}

namespace {
bool FillUserIdentifier( const QString & userName, uid_t & userId, gid_t & userPrimaryGroupId )
{
	//Fill user identifier
	struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
	QByteArray _passwd_strings_buf;
	struct passwd userInfo;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	::getpwnam_r(QSTR2UTF8(userName), &userInfo, _passwd_strings_buf.data(), _passwd_strings_buf.size(),
		&lpcUserInfo );
#else
       lpcUserInfo = getpwnam( QSTR2UTF8(userName) );
#endif

	if (!lpcUserInfo)
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't identify user. getpwnam() call returned error code: %d", errno);
		return false;
	}

	userId = lpcUserInfo->pw_uid;
	userPrimaryGroupId = lpcUserInfo->pw_gid;

	return true;
}

}

#ifdef _LIN_

// Try to authenticate user
bool CAuth::AuthUser(const QString & userName,
         const QString & password,
         const QString & unused)
 {
	Q_UNUSED( unused );

	m_bAuthAvailable = true;

    int ret;
    char *str = strdup(password.toAscii().data());
    struct pam_conv pamConv = { pamConvers, str };
    pam_handle_t* pamh;

    // Init library
    ret = pam_start(m_qsService.toAscii().constData(), userName.toUtf8().constData(), &pamConv, &pamh);
    if (ret != PAM_SUCCESS) {
		m_bAuthAvailable = false;
        WRITE_TRACE(DBG_FATAL, "Can't init PAM library: %s", pam_strerror(pamh, ret));
        free(str);
        return false;
    }

    // Auth user
    if ((ret = pam_authenticate(pamh, 0)) != PAM_SUCCESS) {
		// PAM_MODULE_UNKNOWN is error that indicates problem with pam modules. This problem
		// can be found on x64 linux usualy.
		m_bAuthAvailable = (PAM_MODULE_UNKNOWN != ret);
        WRITE_TRACE(DBG_FATAL, "Auth failed: %s [%d]", pam_strerror(pamh, ret), ret);
        if ((ret = pam_end(pamh, ret)) != PAM_SUCCESS)
            WRITE_TRACE(DBG_FATAL, "Can't deinit PAM library ? : %s", pam_strerror(pamh, ret));
        free(str);
        return false;
    }

    // Unload library
    if ((ret = pam_end(pamh, ret)) != PAM_SUCCESS) {
        WRITE_TRACE(DBG_FATAL, "Can't deinit PAM library ? : %s", pam_strerror(pamh, ret));
        free(str);
        return false;
    }

    free(str);

	return FillUserIdentifier( userName, m_UserId, m_UserPrimaryGroupId );
}
#endif//_LIN_

bool CAuth::AuthUser(quint32 UserId, QString &sUserName, QString & unused)
{
	Q_UNUSED( unused );
	errno = 0;

	struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
	QByteArray _passwd_strings_buf;
	struct passwd userInfo;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	::getpwuid_r((uid_t)UserId, &userInfo, _passwd_strings_buf.data(), _passwd_strings_buf.size(),
		&lpcUserInfo );
#else
	lpcUserInfo = getpwuid( (uid_t)UserId );
#endif
	if (lpcUserInfo)
	{
		sUserName = UTF8_2QSTR(lpcUserInfo->pw_name);
		m_UserId = (uid_t)UserId;
		m_UserPrimaryGroupId = lpcUserInfo->pw_gid;
		return (true);
	}
	WRITE_TRACE(DBG_FATAL, "Couldn't to find user with id %d, err = %d", (uid_t)UserId, errno);
	return (false);
}

uid_t CAuth::GetUserId() const
{
	return (m_UserId);
}

gid_t CAuth::GetUserPrimaryGroupId() const
{
	return m_UserPrimaryGroupId;
}


//////////////////////////////////////////////////////////////////////////

// Check file permissions
CAuth::AccessMode CAuth::CheckFile(const QString & user,
         const QString & fileName,
         const QString __attribute__((unused)) unused1,
         const QString __attribute((unused)) unused2)
{
    // !!! If user name is empty assumming it is a 'root' user
    QString userName = (user.isEmpty() ? "root" : user);

    struct stat64 fStat;
    CAuth::AccessMode mask = 0;

    // Get file status structure
    if ( stat64(QSTR2UTF8(fileName), &fStat) ) {
        if ( EACCES == errno )
            mask |= permDenied;
        else if ( ENOENT == errno ||
                  EFAULT == errno ||
                  ENOTDIR == errno )
            mask |= pathNotFound;
        else
            mask |= otherError;

        WRITE_TRACE(DBG_FATAL, "stat64() has returned error: %s, %s", strerror(errno),
                 QSTR2UTF8(fileName) );
        return mask;
    }

    // Check uid/gid and permissions on file
    mask = 0;

    //Process ACLs
    if (CAclHelper::IsAclSupported(fileName))
        mask |= CAclHelper::GetEffectiveRightsForUser(fileName, user);

    // Get uid/gid
	struct passwd * lpcUserInfo = 0;
	int eno, status = 0;
#ifdef _LIN_
	QByteArray _passwd_strings_buf;
	struct passwd userInfo;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	errno = 0;
	status = ::getpwnam_r(QSTR2UTF8(userName), &userInfo, _passwd_strings_buf.data(),
						_passwd_strings_buf.size(), &lpcUserInfo );
	eno = errno;
#else
	errno = 0;
	lpcUserInfo = getpwnam( QSTR2UTF8(userName) );
	eno = errno;
#endif

    if ( lpcUserInfo == 0 ) {
        // User was not found
        WRITE_TRACE(DBG_FATAL, "CAuthCheckFile %s failed to get the user %s password record status=%d errno=%d",
			QSTR2UTF8(fileName), QSTR2UTF8(userName), status, eno);
        return userNotFound;
    }

    // Check if directory
    if (fStat.st_mode & S_IFDIR)
        mask |= CAuth::fileIsDirectory;


    // Is root ?
    if (lpcUserInfo->pw_uid == 0) {
        mask |= (CAuth::fileMayRead | CAuth::fileMayWrite | CAuth::fileMayExecute);
        return mask;
    }

    bool bForceOwnerShip = false;

    // If owner uid is equal
    if (fStat.st_uid == lpcUserInfo->pw_uid || bForceOwnerShip) {
        // Check for read
        if (fStat.st_mode & S_IRUSR)
            mask |= CAuth::fileMayRead;

        // Check for write
        if (fStat.st_mode & S_IWUSR)
            mask |= CAuth::fileMayWrite;

        // Check for execute
        if (fStat.st_mode & S_IXUSR)
            mask |= CAuth::fileMayExecute;

		return mask;
	}

    // If owner gid is equal
    if ((fStat.st_gid == lpcUserInfo->pw_gid) ||
			isUserMemberOfGroup(userName,fStat.st_gid) ||
			bForceOwnerShip
		)
	{
        // Check for read
        if (fStat.st_mode & S_IRGRP)
            mask |= CAuth::fileMayRead;

        // Check for write
        if (fStat.st_mode & S_IWGRP)
            mask |= CAuth::fileMayWrite;

        // Check for execute
        if (fStat.st_mode & S_IXGRP)
            mask |= CAuth::fileMayExecute;
		return mask;
	}

    // Another world check
    // Check for read
    if (fStat.st_mode & S_IROTH)
        mask |= CAuth::fileMayRead;

    // Check for write
    if (fStat.st_mode & S_IWOTH)
        mask |= CAuth::fileMayWrite;

    // Check for execute
    if (fStat.st_mode & S_IXOTH)
        mask |= CAuth::fileMayExecute;

    return mask;
}

bool CAuth::SetFilePermissions(CAuth::AccessMode ulFilePermissions,
                               const QString & strUserName,
                               const QString & strFileName,
                               const QString __attribute__((unused)) unused1,
                               const QString __attribute((unused)) unused2)
{
    struct stat64 fStat;
    mode_t  mode = 0;

    // Get file status structure
    if ( stat64(QSTR2UTF8(strFileName), &fStat) )  {
        WRITE_TRACE(DBG_FATAL, "stat64() return error: %s", strerror(errno));
        return 0;
    }

    // Get uid/gid
	struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
	QByteArray _passwd_strings_buf;
	struct passwd userInfo;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	::getpwnam_r(QSTR2UTF8(strUserName), &userInfo, _passwd_strings_buf.data(),
						_passwd_strings_buf.size(),	&lpcUserInfo );
#else
	lpcUserInfo = getpwnam( QSTR2UTF8(strUserName) );
#endif

	if (!lpcUserInfo)
	{
		WRITE_TRACE(DBG_FATAL, "An error %d occured on extracting info for user '%s'", errno, QSTR2UTF8(strUserName));
		return (false);
	}

    // Check uid/gid and permissions on file

    // Is root ?
    if (lpcUserInfo->pw_uid == 0)
        return true;

    // If owner uid is equal
    if (fStat.st_uid == lpcUserInfo->pw_uid)  {
        // form permitions for chmode
        if(ulFilePermissions & fileMayRead)
            mode |= S_IRUSR;
        if(ulFilePermissions & fileMayWrite)
            mode |= S_IWUSR;
        if(ulFilePermissions & fileMayExecute)
            mode |= S_IXUSR;
        ::chmod(strFileName.toAscii().constData(),mode);
        return  true;
    }

    // If owner gid is equal
    if (fStat.st_gid == lpcUserInfo->pw_gid)  {
        // form permitions for chmode
        if(ulFilePermissions & fileMayRead)
            mode |= S_IRGRP;
        if(ulFilePermissions & fileMayWrite)
            mode |= S_IWGRP;
        if(ulFilePermissions & fileMayExecute)
            mode |= S_IXGRP;
        ::chmod(strFileName.toAscii().constData(),mode);
        return true;
    }
    return false;
}

bool CAuth::GetCommonFilePermissions( const QString& fileName
			, CAuth::AccessMode& ownerPermissions
			, CAuth::AccessMode& othersPermissions
			, bool& flgMixedOthersPermission
			, CAuth*
			, PRL_RESULT* )
{
	ownerPermissions = othersPermissions = 0;
	flgMixedOthersPermission = false;

	struct stat fInfo;

	if( stat( QSTR2UTF8( fileName ), &fInfo ) )
	{
		WRITE_TRACE(DBG_FATAL, "GetSimplePermissionsToFile(): stat( '%s' ) return error %ld (%s)"
			, QSTR2UTF8( fileName )
			, Prl::GetLastError()
			, QSTR2UTF8( Prl::GetLastErrorAsString() ) );
		return false;
	}

	mode_t	ownerMode	= fInfo.st_mode & S_IRWXU;
	mode_t	groupMode	= fInfo.st_mode & S_IRWXG ;
	mode_t	othersMode	= fInfo.st_mode & S_IRWXO ;

	//convert to S_IRWXU mask to compare and convert:
	groupMode = groupMode << 3;
	othersMode = othersMode << 6;

	flgMixedOthersPermission = ( groupMode != othersMode );

#	define MODE_TO_PERM( mode ) \
	( ( (mode) & S_IRUSR ? CAuth::fileMayRead :0 ) \
	| ( (mode) & S_IWUSR ? CAuth::fileMayWrite :0 ) \
	| ( (mode) & S_IXUSR ? CAuth::fileMayExecute :0 ) )

	ownerPermissions	= MODE_TO_PERM( ownerMode );
	othersPermissions	= MODE_TO_PERM( groupMode & othersMode );	// return minimal subset

#	undef MODE_TO_PERM

	return true;

}


bool CAuth::getCurrentDomain( QString& outDomainName)
{
	outDomainName.clear();
	return true;
}

CAuth *CAuth::Clone() const
{
	CAuth *pAuthObject = new CAuth;
	pAuthObject->m_UserId = this->m_UserId;
	pAuthObject->m_UserPrimaryGroupId = this->m_UserPrimaryGroupId;
	return (pAuthObject);
}
