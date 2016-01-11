/////////////////////////////////////////////////////////////////////////////
///
/// @file CFileHelper.cpp
///
/// A couple of crossplatform helper methods working with file system
///
/// @author sergeyt@
/// @owner sergeym@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include <stddef.h>

#if defined (_LIN_)
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <sys/errno.h>
#	include <pwd.h>
#	include <fcntl.h>
#	include <utime.h>
#	include <stdlib.h>
#	include <unistd.h>
#else
#	include <windows.h>
#	include <aclapi.h>
#	include <accctrl.h>
#	include <lmcons.h>
#	include <winerror.h>
#	include <WinIoCtl.h>
#	include <atlsecurity.h>
#endif//defined(_LIN_)

#ifdef _LIN_
#include <sys/statvfs.h>
#include <mntent.h>
#endif

#ifdef _WIN_
#include "WinJunctions.h"
#endif

#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlcommon/Interfaces/ParallelsQt.h>

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <prlcommon/HostUtils/HostUtils.h>

#include "CFileHelper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include "Libraries/CAuth/CAuth.h"
#include "CAuthHelper.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlsdk/PrlEnums.h>

#include <prlcommon/PrlCommonUtilsBase/CSimpleFileHelper.h>

#ifdef _LIN_
#include <prlcommon/Std/SmartPtr.h>
#endif

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


/**
 * @brief
 *		 Checks specified file exists or not
 *
 * @params
 *		 strFileName [in] - file name
 *
 * @return
 *		 TRUE - file exists
 *		 FALSE - file not exists
 */

bool CFileHelper::FileExists ( QString strFileName,
							   CAuthHelper *pAuthHelper )
{
	if ( strFileName.isEmpty() || !pAuthHelper)
	  return false;

#ifdef _WIN_
	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	return QFile::exists( strFileName );
#else
	CAuth::AccessMode mask = pAuthHelper->CheckFile(strFileName);
	return ! (mask & CAuth::permDenied || mask & CAuth::pathNotFound ||
			  mask & CAuth::otherError);
#endif
};


/**
 * @brief
 *		 Checks, can we write specified file
 *
 * @params
 *		 strFileName [in] - file name
 *
 * @return
 *		 TRUE - can write
 *		 FALSE - can't write
 */

bool CFileHelper::FileCanWrite ( QString strFileName,
								 CAuthHelper *pAuthHelper )
{
	if ( strFileName.isEmpty() || !pAuthHelper)
		return false;

	CAuth::AccessMode mask = pAuthHelper->CheckFile( strFileName );
	return mask & CAuth::fileMayWrite;

};

// FIX ME - temporal solution (FileCanWrite for directories works incorrectly on windows)
bool CFileHelper::DirCanWrite(QString strDirName, CAuthHelper *pAuthHelper)
{
	return CFileHelper::FileCanWrite( strDirName, pAuthHelper );
}
/**
 * @brief
 *		 Checks, can we write specified file
 *
 * @params
 *		 strFileName [in] - file name
 *
 * @return
 *		 TRUE - can write
 *		 FALSE - can't write
 */

bool CFileHelper::FileCanRead ( QString strFileName,
								CAuthHelper *pAuthHelper )
{
	if ( strFileName.isEmpty() || !pAuthHelper)
		return false;

	CAuth::AccessMode mask = pAuthHelper->CheckFile(strFileName);
	return mask & CAuth::fileMayRead;
};

/**
* @brief return possibility of read directory for user
* recursive check for all parent directory
* @param pSession	- user session object
* @param QString 	- directory path to check
**/

bool CFileHelper::CanReadAllDirsToRoot(const QString & strDirectoryPath, CAuthHelper *pAuthHelper)
{
	QDir dir(strDirectoryPath);

	do
	{
		if ( !CFileHelper::FileCanRead( dir.absolutePath(), pAuthHelper ))
			return false;

		if(!dir.cdUp())
			return false;

	} while (!dir.isRoot());

	return true;
}

/**
 * @brief
 *		 Checks, can we write specified file
 *
 * @params
 *		 strFileName [in] - file name
 *
 * @return
 *		 TRUE - can write
 *		 FALSE - can't write
 */

bool CFileHelper::FileCanExecute ( QString strFileName,
								   CAuthHelper *pAuthHelper )
{
	if ( strFileName.isEmpty() || !pAuthHelper)
		return false;

	CAuth::AccessMode mask = pAuthHelper->CheckFile(strFileName);
	return mask & CAuth::fileMayExecute;
};


/**
 * @brief
 *		 Checks specified directory exists or not
 *
 * @params
 *		 strPath [in] - full directory path
 *
 * @return
 *		 TRUE - exists
 *		 FALSE - not exists
 */

bool CFileHelper::DirectoryExists ( QString strPath, CAuthHelper *pAuthHelper )
{
	if ( pAuthHelper == 0 )
		return false;

	bool exists = FileExists( strPath, pAuthHelper );
	if ( ! exists )
		return false;

#ifdef _WIN_
	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	QFileInfo fi( strPath );
	return fi.isDir();
#else
	CAuth::AccessMode mask = pAuthHelper->CheckFile(strPath);
	return mask & CAuth::fileIsDirectory;
#endif
};


/**
 * @brief
 *		 Checks we can get list of directory contents
 *
 * @params
 *		 strPath [in] - full directory path
 *
 * @return
 *		 TRUE - we can
 *	 FALSE - otherwise
 */

bool CFileHelper::CanGetDirectoryContents( QString strPath,
										   CAuthHelper* pAuthHelper )
{
#ifdef _WIN_
	//FIXME: Win32 has some other special permissions for directory list
	//	   but for now we can use Linux variant
	// it is not need execute permissions for read attribute
	return (/*FileCanExecute(strPath, pAuthHelper) &&*/
			FileCanRead(strPath, pAuthHelper));
#else
	return (FileCanExecute(strPath, pAuthHelper) &&
			FileCanRead(strPath, pAuthHelper));
#endif
}


/**
 * @brief
 *		 Renames or moves entry
 *
 * @params
 *		 oldName [in] - old name of entry
 *		 newName [in] - new name of entry
 *
 * @return
 *		 TRUE - rename of move was successfull
 *		 FALSE - otherwise
 */

bool CFileHelper::RenameEntry(QString oldName, QString newName, CAuthHelper *pAuthHelper)
{
	if ( pAuthHelper == 0 )
		return false;

	QDir dir;
#ifdef _WIN_
	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	return dir.rename( oldName, newName );
#else
	bool exists = FileExists( oldName, pAuthHelper );
	if ( ! exists )
		return false;

	exists = FileExists( newName, pAuthHelper );
	if ( exists )
		return false;

	QString oldRootName = GetFileRoot( oldName );
	QString newRootName = GetFileRoot( newName );

	if ( ! FileCanWrite(newRootName, pAuthHelper) ||
		 ! FileCanExecute(newRootName, pAuthHelper) ||
				 ! FileCanWrite(oldName, pAuthHelper) )
		return false;

	if ( oldRootName != newRootName ) {
		// Try to move
		if ( ! FileCanWrite(oldRootName, pAuthHelper) )
			return false;
	}
	return dir.rename( oldName, newName );

#endif
}

#ifdef _WIN_
/**
 * Cleanups readonly attribute for specified file
 */
bool CFileHelper::CleanupReadOnlyAttr( const QString &sTargetPath )
{
	DWORD fileAttributes = GetFileAttributes(sTargetPath.utf16());
	if (fileAttributes & FILE_ATTRIBUTE_READONLY)
    {
		if ( !SetFileAttributes(sTargetPath.utf16(), fileAttributes ^ FILE_ATTRIBUTE_READONLY) )
		{
			DWORD dwError = GetLastError();
			WRITE_TRACE(DBG_FATAL, "Couldn't to unset readonly attribute for path '%s' with error: %d", QSTR2UTF8(sTargetPath), dwError);
			return false;
		}
	}
	return true;
}
#endif

/**
 * @brief
 *		 Removes entry
 *
 * @params
 *		 name [in] - entry name to remove
 *
 * @return
 *		 TRUE - successfully removed
 *		 FALSE - otherwise
 */

bool CFileHelper::RemoveEntry(QString target, CAuthHelper *pAuthHelper)
{
	if ( pAuthHelper == 0 )
		return false;

	QFileInfo info( target );
	bool deleteRes = false;

#ifdef _WIN_
	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	//Check whether readonly attribute presents and unset it
	CleanupReadOnlyAttr( target );

	if ( info.isDir() )
		deleteRes = QDir().rmdir( target );
	else
		deleteRes = QFile::remove( target );
	return deleteRes;
#else
	bool exists = FileExists( target, pAuthHelper );
	if ( ! exists )
		return false;

	QString rootTarget = GetFileRoot( target );

	if ( ! FileCanWrite(rootTarget, pAuthHelper)   ||
		 ! FileCanExecute(rootTarget, pAuthHelper) ||
		 ! FileCanWrite(target, pAuthHelper) )
		return false;

	if ( info.isDir() )
		deleteRes = QDir().rmdir( target );
	else
		deleteRes = QFile::remove( target );
	return deleteRes;
#endif
}


/**
 * @brief
 *		 Creates the directory path strPath (with all parent directories)
 *
 * @params
 *		 strPath [in] - full directory path
 *
 * @return
 *		 TRUE - directory created
 *		 FALSE - not created
 */

namespace {
	/** WriteDirectory() method helper - erases tail from the given path */
	bool ErasePathTail(QString &sPath) {
		if (sPath.size()) {
			quint32 nErasePos = sPath.size()-1;
			while (nErasePos && sPath[nErasePos] == '/') nErasePos--;
			if (nErasePos) {
				while (nErasePos && sPath[nErasePos] != '/') nErasePos--;
				if (nErasePos) {
					sPath.remove(nErasePos, sPath.size()-nErasePos);
					return (true);
				}
			}
		}
		return (false);
	}
}

bool CFileHelper::WriteDirectory ( QString strPath, CAuthHelper *pAuthHelper )
{
	if ( pAuthHelper == 0 )
		return false;

#ifdef _WIN_
	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	QDir dir;
		return (dir.mkpath(strPath) && DirectoryExists(strPath, pAuthHelper));
#else
	QDir dir( strPath );
		QString sPath = dir.absolutePath();
	QStringList paths;
	paths.prepend( sPath );
	while ( ErasePathTail(sPath) ) {
		paths.prepend( sPath );
	}
	QStringList createdDirs;
	foreach ( QString path, paths )
	{
		if ( DirectoryExists(path, pAuthHelper) )
			continue;

		// Do some more checks
		CAuth::AccessMode mask = pAuthHelper->CheckFile(path);

		// Create directory if really doesn't exist
		if ( mask & CAuth::pathNotFound &&
			 CreateDirectoryPath(path, pAuthHelper) )
		{
			createdDirs.append(path);
			continue;
		}

		if ( mask & CAuth::permDenied ||
			  mask & CAuth::otherError )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to create directory by invalid mask =%d(err: perm=%d, other=%d ), path='%s'"
						, (int)mask
						, (int)( mask & CAuth::permDenied )
						, (int)( mask & CAuth::otherError )
						, QSTR2UTF8( path )
			);
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "CreateDirectoryPath() return error for path='%s'"
				, QSTR2UTF8( path )
		  );
		}

		// Remove already created dirs
		QDir tmpDir;
		for ( int i = createdDirs.size() - 1; i >= 0; --i )
		{
				// All created dirs we can remove without
				// help of CAuth
				tmpDir.rmdir(createdDirs[i]);
		}

		return false;
	} // foreach path

	return true;
#endif
};


/**
 * @brief
 *		 Returns the absolute path of given strFilePath
 *
 * @params
 *		 strFilePath [in] - full path
 *
 * @return
 *		 QString - absolute path
 */

QString CFileHelper::GetFileRoot ( QString strFilePath )
{
	QFileInfo fi(strFilePath);
	return fi.dir().absolutePath();
};


QString CFileHelper::GetMountPoint(const QString& sFilePath)
{
#ifdef _WIN_

	static const uint MAX_SZ = 2048;
	PRL_ASSERT( sFilePath.size()<MAX_SZ );
	if(sFilePath.size()>MAX_SZ)
		return "";
	wchar_t lpszPath[MAX_SZ] = {0};
	sFilePath.toWCharArray( lpszPath );

	wchar_t lpszMountPath[MAX_SZ] = {0};

	BOOL bRet = GetVolumePathName( lpszPath, lpszMountPath, sizeof(lpszMountPath)/sizeof(wchar_t) );
	if( bRet )
		return UTF16_2QSTR(lpszMountPath);

	WRITE_TRACE( DBG_FATAL, "GetVolumePathName() for %s fails by error %d"
		, QSTR2UTF8(sFilePath), GetLastError() );
	return "";

#else

	PRL_ASSERT( QFileInfo(sFilePath).isAbsolute() );
	if( !QFileInfo(sFilePath).isAbsolute() )
		return "";

	struct stat64 fStat;
	if ( stat64(QSTR2UTF8( sFilePath ), &fStat) )
	{
		WRITE_TRACE( DBG_WARNING, "stat64() for %s failed by error %d"
			, QSTR2UTF8(sFilePath), errno );
		return "";
	}
	dev_t fileDeviceId = fStat.st_dev;

	QStringList lstParentDirs;

	QFileInfo fi(sFilePath);
	if( !fi.isDir() )
		fi.setFile( fi.absolutePath() );

	for( ; !fi.isRoot() && !fi.fileName().isEmpty() ; fi.setFile( fi.canonicalPath() ) )
	{
		if( !fi.canonicalFilePath().isEmpty() )
			lstParentDirs.push_front( fi.canonicalFilePath() );
	}
	lstParentDirs.push_front("/");

	foreach( QString sDir, lstParentDirs )
	{
		struct stat64 fStat;
		if ( stat64(QSTR2UTF8(sDir), &fStat) )
		{
			WRITE_TRACE( DBG_WARNING, "stat64() for %s failed by error %d"
				, QSTR2UTF8(sDir), errno );
			return "";
		}

		if( fStat.st_dev != fileDeviceId )
			continue;

		return sDir;
	}

	PRL_ASSERT( 0 );
	return "";



#endif
}

/**
 * @brief
 *		 Creates the empty file
 *
 * @params
 *		 strFilePath [in] - file path
 *
 * @return
 *		 TRUE - file created
 *		 FALSE - can't create file
 */

bool CFileHelper::CreateBlankFile ( QString strFilePath,
									CAuthHelper *pAuthHelper )
{
	if ( pAuthHelper == 0 )
		return false;

#ifndef _WIN_
	uid_t	userId = (UINT)-1;
	gid_t   groupId = (UINT)-1;
	if ( ! pAuthHelper->isDefaultAppUser() ) {
		struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
		QByteArray _passwd_strings_buf;
		struct passwd userInfo;
		_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

		::getpwnam_r(QSTR2UTF8(pAuthHelper->getUserName()), &userInfo, _passwd_strings_buf.data(),
						_passwd_strings_buf.size(),	&lpcUserInfo );
#else
		lpcUserInfo = getpwnam( QSTR2UTF8(pAuthHelper->getUserName()) );
#endif
		if ( lpcUserInfo == 0 ) {
			WRITE_TRACE(DBG_FATAL, "Can't create blank file '%s', user not found: %s",
					  QSTR2UTF8(strFilePath),
					  QSTR2UTF8(pAuthHelper->getUserName()) );
			return false;
		}
		userId = lpcUserInfo->pw_uid;
		groupId = lpcUserInfo->pw_gid;
	}
#endif

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	if ( !WriteDirectory(GetFileRoot(strFilePath), pAuthHelper) )
		return false;

#ifndef _WIN_
	// For Unix based Os, we need check permissions for parent catalog
	if ( ! FileCanWrite(GetFileRoot(strFilePath), pAuthHelper) ||
		 ! FileCanExecute(GetFileRoot(strFilePath), pAuthHelper) )
		return false;
#else
	// For Win32, we topically trying to create file
	if ( ! FileCanWrite( GetFileRoot( strFilePath ), pAuthHelper) )
		return false;
#endif // _WIN_

	QFile file(strFilePath);
	file.open(QIODevice::ReadWrite);

	bool bCanWrite = file.isWritable();
	file.close();

#ifndef _WIN_
	// Set file owner if user is not admin (root, system etc)
	if ( ! pAuthHelper->isDefaultAppUser() )
	{
		if (::chown( QSTR2UTF8(strFilePath), userId, groupId) == -1)
			WRITE_TRACE(DBG_FATAL, "chown call was failed for file '%s' with error code %d", QSTR2UTF8(strFilePath), errno);
	}
#endif

	return bCanWrite;
};


/**
 * @brief
 *		 Creates directory
 *
 * @params
 *		 strFilePath [in] - dir path
 *
 * @return
 *		 TRUE - dir created
 *		 FALSE - can't create dir
 */

bool CFileHelper::CreateDirectoryPath ( QString strDirPath,
									CAuthHelper *pAuthHelper )
{
	if ( pAuthHelper == 0 )
		return false;

#ifndef _WIN_
	uid_t	userId = (UINT)-1;
	gid_t   groupId = (UINT)-1;
	if ( ! pAuthHelper->isDefaultAppUser() )
	{
		// Note: Don't use userInfo after this block. (bug #1783 for more info)
		// This struct would be free in other call getpw*() below.
		// We store only uid and gid to the future using.
		struct passwd * lpcUserInfo = 0;
#ifdef _LIN_
		QByteArray _passwd_strings_buf;
		struct passwd userInfo;
		_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

		::getpwnam_r(QSTR2UTF8(pAuthHelper->getUserName()), &userInfo, _passwd_strings_buf.data(),
						_passwd_strings_buf.size(),	&lpcUserInfo );
#else
		lpcUserInfo = getpwnam( QSTR2UTF8(pAuthHelper->getUserName()) );
#endif
		if ( lpcUserInfo == 0 ) {
			LOG_MESSAGE( DBG_FATAL, "Can't create directory '%s', user not found: %s",
					  QSTR2UTF8(strDirPath),
					  QSTR2UTF8(pAuthHelper->getUserName()) );
			return false;
		}
		userId = lpcUserInfo->pw_uid;
		groupId = lpcUserInfo->pw_gid;
	}
	if ( ! DirectoryExists(GetFileRoot(strDirPath), pAuthHelper) )
		return false;
	if ( ! FileCanWrite(GetFileRoot(strDirPath), pAuthHelper) ||
		 ! FileCanExecute(GetFileRoot(strDirPath), pAuthHelper) )
		return false;
#endif

	CAuthHelperImpersonateWrapper _wrapper(pAuthHelper);

	PRL_ASSERT( _wrapper.wasImpersonated() );
	if( ! _wrapper.wasImpersonated() )
		return false;

	QDir dir;
	if ( !dir.mkdir( strDirPath) )
		return false;

#ifndef _WIN_
	// Set file owner if user is not admin (root, system etc)
	if ( ! pAuthHelper->isDefaultAppUser() ) {
		int ret = ::chown( QSTR2UTF8(strDirPath), userId, groupId);
		UNUSED_PARAM(ret);
	}
#endif

	return true;
};


/**
 * @brief
 *		 returns file name
 *
 * @params
 *		 strFilePath [in] - full file path
 *
 * @return
 *		 QString - only name of file
 *
 */

QString CFileHelper::GetFileName(QString & strFilePath)
{
	QFileInfo cFileInfo(strFilePath);
	return cFileInfo.fileName();
}

/**
* @brief
*		 returns base file name
*
* @params
*		 strFilePath [in] - full file path
*
* @return
*		 QString - base name of file
*
*/

QString CFileHelper::GetBaseFileName(QString & strFilePath)
{
	QFileInfo cFileInfo(strFilePath);
	return cFileInfo.baseName();
}

/**
 * @brief
 *		 delete files from list. Directories must be sorting!
 *		  At early indexes must be more nesting dirs in ierarhian structure
 *
 * @params
 *		  QStringList & cList - list of strings to delete
 * @return
 *		 bool - result
 *
 */

bool CFileHelper::DeleteFilesFromList(QStringList & cList)
{
	// removed files first
	for(int i = 0;i < cList.size();i++)
	{
		QFileInfo cFileInfo(cList.at(i));
		if(!cFileInfo.isDir())
		{
			QFile cFile(cList.at(i));
			cFile.remove();
		}
	}
	// removed dirs
	for(int i = 0;i < cList.size();i++)
	{
		QFileInfo cFileInfo(cList.at(i));
		if(cFileInfo.isDir())
		{
			QDir cDir;
			cDir.setPath(cList.at(i));
			cDir.rmdir(cList.at(i));

		}
	}

	return true;
}

/**
 * @brief
 *		 remove directory with all contained objects
 *		 It is only windows variant tested,check it on linux
 *
 * @params
 *		  QString & strDir - directory for delete
 * @return
 *		 bool - result
 *
 */


bool CFileHelper::ClearAndDeleteDir(const QString & strDir)
{
	// will do maximum 3 attempts to remove directory
	// to force case when other process or thread create file in dir concurrently
	// https://jira.sw.ru/browse/PSBM-12160
	return CSimpleFileHelper::ClearAndDeleteDir( strDir, 3 );
}

// atomic rename file. It uses in crash-safe save mechs
// NOTE: sFrom and sTo should be on one hdd partiotion!
bool CFileHelper::AtomicMoveFile( const QString& sFrom, const QString& sTo )
{
	// copy past from CBaseNode::saveToFile()
	bool bError = false;

#ifdef	_WIN_
	DWORD errCode = 0;
	if( ! MoveFileEx(
		(LPWSTR)sFrom.utf16()
		, (LPWSTR)sTo.utf16()
		, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH  )
		)
	{
		bError = true;
		errCode = GetLastError();
	}
#else
	int errCode = 0;
	if( rename( QSTR2UTF8( sFrom ), QSTR2UTF8( sTo ) ) )
	{
		bError = true;
		errCode = errno;
	}
#endif

	if( bError )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to move file by error %#x. sFrom==>sTo:'%s' ==>'%s'"
			, errCode
			, QSTR2UTF8( sFrom )
			, QSTR2UTF8( sTo ) );
		return false;
	}
	return true;
}


QString CFileHelper::getOwner( const QString & strFileName, bool bUseSlash )
{
	//Implement CAuthHelper user format : username@domainName

	return CAuthHelper::GetOwnerOfFile( strFileName, bUseSlash );
}

bool CFileHelper::setOwnerByTemplate(
	const QString& strFileName,
	const QString& strTemplateFileName,
	CAuthHelper& currentUser,
	bool bRecursive)
{
	// TODO: Add check for FAT32/ remote /	like CDspAccessManager::isOwnerOfVm() if required

	QFileInfo fi( strFileName );
	if( !fi.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "%s: file does not exists (path=%s)"
			, __FUNCTION__ , QSTR2UTF8( strFileName  ) );
		return false;
	}
	QFileInfo fiTemplate( strTemplateFileName );
	if( !fiTemplate.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "%s: template file does not exists (path=%s)"
			, __FUNCTION__ , QSTR2UTF8( strTemplateFileName ) );
		return false;
	}

	PRL_RESULT err;
#ifndef _WIN_
	Q_UNUSED( currentUser );
	struct stat fStat;
	if ( stat(QSTR2UTF8(strTemplateFileName), &fStat) )
	{
		WRITE_TRACE(DBG_FATAL, "%s: stat() failed with errno = %d for template file (path=%s)"
			, __FUNCTION__ , errno, QSTR2UTF8( strTemplateFileName  ) );
		return false;
	}

	err = setRawFileOwner( fi, fStat.st_uid, fStat.st_gid, bRecursive );
#else
	// we should set owner under impersonate to prevent security hole ( through 'junction' for example )
	CAuthHelperImpersonateWrapperPtr pImpersonate( 0 );
	DWORD dwAttr = GetFileAttributes( (LPCWSTR)fi.absoluteFilePath().utf16() );
	if( dwAttr & FILE_ATTRIBUTE_REPARSE_POINT )
		pImpersonate = CAuthHelperImpersonateWrapper::create( &currentUser );
	err = setRawFileOwner( fi, CAuthHelper::OwnerWrapper(strTemplateFileName), bRecursive );
#endif

	return PRL_SUCCEEDED(err);
}

// set owner of file
bool CFileHelper::setOwner( const QString & strFileName, CAuthHelper* pAuthHelper, bool bRecursive)
{
	if ( pAuthHelper->isDefaultAppUser() )
		return false;

	QFileInfo fi( strFileName );
	if( !fi.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "%s: file does not exists (path=%s)"
			, __FUNCTION__, QSTR2UTF8( strFileName  ) );
		return false;
	}

#ifndef _WIN_

		struct passwd* pResultUserInfo = NULL;
#	if defined(_LIN_)
		QByteArray _passwd_strings_buf;
		_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));
		struct passwd _userInfo;
		::getpwnam_r(	QSTR2UTF8(pAuthHelper->getUserName()),
						&_userInfo,
						_passwd_strings_buf.data(),
						_passwd_strings_buf.size(),
						&pResultUserInfo );
#	endif //#ifdef _LIN_
		if ( pResultUserInfo == 0 )
		{
			WRITE_TRACE(DBG_FATAL, "getpwnam() failed for user '%s' with error code %d",\
						QSTR2UTF8(pAuthHelper->getUserName()), errno );
			return false;
		}
#endif // #ifndef _WIN_


	PRL_RESULT err;
#ifndef _WIN_
	err = setRawFileOwner( fi, pResultUserInfo->pw_uid, pResultUserInfo->pw_gid, bRecursive );
#else // _WIN_
	err = setRawFileOwner( fi, CAuthHelper::OwnerWrapper(pAuthHelper), bRecursive );
#endif

	return PRL_SUCCEEDED(err);

}

PRL_RESULT CFileHelper::GetDirSize( const QString& strDirPath, quint64 *pSize )
{
	return CSimpleFileHelper::GetDirSize( strDirPath, pSize );
}

/**
* Gets free space of a disk that contains a file named by a path argument.
* @param qsPath [in] A path on the disk.
* @param pAvailableSpace [out] The total number of free bytes on a disk that are available to the user.
* @param pTotalSpace [out] The total number of bytes on a disk.
* @param pFreeSpace [out] The total number of free bytes on a disk.
* @return Error code
*/
PRL_RESULT CFileHelper::GetDiskAvailableSpace(
	const QString& qsPath, quint64 *pAvailableSpace, quint64* pTotalSpace, quint64 *pFreeSpace )
{
	*pAvailableSpace = 0;
	if ( pTotalSpace )
		*pTotalSpace = 0;
	if ( pFreeSpace )
		*pFreeSpace = 0;

#ifdef _WIN_
	ULARGE_INTEGER FreeBytesAvailable;
	ULARGE_INTEGER TotalNumberOfBytes;
	ULARGE_INTEGER TotalNumberOfFreeBytes;

	SetLastError(0);
	if ( GetDiskFreeSpaceEx(qsPath.utf16(), &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes) )
	{
		*pAvailableSpace = ( ((quint64)FreeBytesAvailable.HighPart) << 32 ) + FreeBytesAvailable.LowPart;
		if ( pTotalSpace )
			*pTotalSpace = ( ((quint64)TotalNumberOfBytes.HighPart) << 32 ) + TotalNumberOfBytes.LowPart;
		if ( pFreeSpace )
			*pFreeSpace = ( ((quint64)TotalNumberOfFreeBytes.HighPart) << 32 ) + TotalNumberOfFreeBytes.LowPart;
	}
	else
	{
		DWORD dwError = GetLastError();
		switch ( dwError )
		{
			case ERROR_ACCESS_DENIED:		// Access is denied
				return PRL_ERR_ACCESS_DENIED;
			case ERROR_PATH_NOT_FOUND:	// The system cannot find the path specified
			case ERROR_INVALID_NAME:			// The filename, directory name, or volume label syntax is incorrect
			case ERROR_DIRECTORY:				// The directory name is invalid
				return PRL_ERR_INCORRECT_PATH;
			default:
			{
				WRITE_TRACE( DBG_FATAL,
					"CFileHelper::GetDiskAvailableSpace() : GetDiskFreeSpaceEx() for '%s' failed. Error code: %d",
					QSTR2UTF8(qsPath), dwError );
				return PRL_ERR_GET_DISK_FREE_SPACE_FAILED;
			}
		}
	}
#else // _WIN_
	struct statvfs64 StatFs;

	if ( !statvfs64(QSTR2UTF8(qsPath), &StatFs) )
	{
		*pAvailableSpace = ( quint64 )( (double)StatFs.f_bavail * (double)StatFs.f_bsize );
		if ( pTotalSpace )
			*pTotalSpace = ( quint64 )( (double)StatFs.f_blocks * (double)StatFs.f_bsize );
		if ( pFreeSpace )
			*pFreeSpace = ( quint64 )( (double)StatFs.f_bfree * (double)StatFs.f_bsize );
	}
	else
	{
		int err = errno;
		WRITE_TRACE(DBG_FATAL,
				"CFileHelper::GetDiskAvailableSpace() : statfs() for '%s' failed. Error code = %d (%s)",
				QSTR2UTF8(qsPath), err, strerror(err) );
		switch ( err )
		{
			case EACCES:	// Permission denied
				return PRL_ERR_ACCESS_DENIED;
			case ENOENT:	// No such file or directory
			case ENOTDIR:	// Not a directory
				return PRL_ERR_INCORRECT_PATH;
			case EAGAIN:
			case EINTR:
				return PRL_ERR_TRY_AGAIN;
			default:
				return PRL_ERR_GET_DISK_FREE_SPACE_FAILED;
		}
	}
#endif // _LIN_
	return PRL_ERR_SUCCESS;
}

/**
* @brief
*		 check directory on files presence
*
*
* @params
*		  QString & strDir - directory for check
* @return
*		 bool - result
*
*/
bool CFileHelper::IsDirHasFiles(const QString & strDir)
{
	QDir	cDir(strDir);
	cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
	cDir.setSorting(QDir::Name | QDir::DirsFirst);
	QFileInfoList cFileList = cDir.entryInfoList();


	// recursive search dirs
	for(int i = 0; i < cFileList.size();i++ )
	{
		if( cFileList.at(i).isDir() )
		{
			if(cFileList.at(i).isSymLink())
				return true;
			else
			{
				// #125021 to prevent infinity recursion by QT bug in QDir::entryInfoList()
				if( QFileInfo(strDir) != cFileList.at(i) )
					return IsDirHasFiles(cFileList.at(i).filePath());
			}
		}
		else
				return true;
	}

	return false;
}

/**
 * Removes non valid symbols from path
 * @param processing string value
 * @return well formed path part string value
 */
QString CFileHelper::ReplaceNonValidPathSymbols(const QString &sOriginalString)
{
	QString sPathString = sOriginalString;
	QByteArray invalidSymbols( GetInvalidPathSymbols().toUtf8() );
	foreach( const char c, invalidSymbols )
		sPathString.remove( c );

	sPathString = sPathString.trimmed();
	return (sPathString);
}

/**
* Changes permissions of destination file or directory to permissions of  source file or directory.
* @param qsSource A source file or directory.
* @param qsDist A distination file or directory.
* @return Error code
*/
PRL_RESULT CFileHelper::ChangeFilePermissions(
	const QString& qsSource, const QString& qsDest, CAuthHelper* pAuthHelper )
{
	PRL_RESULT nRet = PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS;

#ifdef _WIN_
	Q_UNUSED( pAuthHelper );

	HANDLE Client;
	TOKEN_PRIVILEGES *Priv;
	static TCHAR * P[] =
	{
		SE_BACKUP_NAME,
		SE_RESTORE_NAME,
		SE_SECURITY_NAME
	};

	//
	// Give ourselves privileges for manipulating security
	//
	if ( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Client) )
	{
		WRITE_TRACE(DBG_FATAL,
			"CFileHelper::ChangeFilePermissions() : OpenProcessToken() failed. Error code: %d",
			GetLastError() );
		return nRet;
	}

	BYTE pBuf[4096];
	Priv = ( PTOKEN_PRIVILEGES )pBuf;
	Priv->PrivilegeCount = 0;
	for ( int j = 0; j < sizeof P/sizeof P[0]; ++j )
	{
		++Priv->PrivilegeCount;
		if ( !LookupPrivilegeValue(NULL, P[j], &Priv->Privileges[j].Luid) )
		{
			WRITE_TRACE(DBG_FATAL,
			"CFileHelper::ChangeFilePermissions() : LookupPrivilegeValue() failed. Error code: %d",
				GetLastError() );
			return nRet;
		}
		Priv->Privileges[j].Attributes = SE_PRIVILEGE_ENABLED;
	}

	if ( !AdjustTokenPrivileges(Client, FALSE, Priv, 0, NULL, NULL) )
	{
		WRITE_TRACE(DBG_FATAL,
			"CFileHelper::ChangeFilePermissions() : AdjustTokenPrivileges() failed. Error code: %d",
			GetLastError() );
		return nRet;
	}

	DWORD dwRes = 0;
	PSID psidOwner = NULL;
	PSID psidGroup = NULL;
	PACL pDacl = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;

	//
	// Change permissions of destination file or directory to permissions of  source file or directory
	//

	dwRes = GetNamedSecurityInfo( (LPTSTR)qsSource.utf16(),
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION |	GROUP_SECURITY_INFORMATION,
		&psidOwner,
		&psidGroup,
		&pDacl,
		NULL,
		&pSD );
	if ( ERROR_SUCCESS != dwRes )
	{
		WRITE_TRACE(DBG_FATAL,
			"CFileHelper::ChangeFilePermissions() : GetNamedSecurityInfo() failed. Error code: %d", dwRes );
		return nRet;
	}
	else
	{
		HANDLE hFile = CreateFile((LPCWSTR)qsDest.utf16(),
							WRITE_DAC|WRITE_OWNER,
							FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
							NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD dwErrorCode = GetLastError();
			WRITE_TRACE(DBG_FATAL, "CreateFile error = %d file: '%s'", dwErrorCode, QSTR2UTF8(qsDest));
		}

		BOOL bRes = SetKernelObjectSecurity(hFile,
											DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
											pSD);
		if ( !bRes )
			dwRes = GetLastError();
		CloseHandle(hFile);
		if ( !bRes )
		{
			WRITE_TRACE(DBG_FATAL,
				"CFileHelper::ChangeFilePermissions() : SetKernelObjectSecurity() failed. Error code: %d", dwRes );
			if( pSD != NULL	)
				LocalFree( pSD );
			return nRet;
		}
	}

	if( pSD )
		LocalFree( pSD );

	return PRL_ERR_SUCCESS;
#else
	Q_UNUSED( qsSource );
	if ( setOwner(qsDest, pAuthHelper, false ) )
		return PRL_ERR_SUCCESS;
#endif

	return nRet;
}

/**
* Changes permissions of destination directory with its contents
* to permissions of  source file or directory.
* @param qsSource A source file or directory.
* @param qsDist A distination directory.
* @return Error code
*/
PRL_RESULT CFileHelper::ChangeDirectoryPermissions(
	const QString& qsSource, const QString& qsDest, CAuthHelper* pAuthHelper )
{
	PRL_ASSERT( ! qsDest.isEmpty() );
	if ( qsDest.isEmpty() )
		return PRL_ERR_FAILURE;

	PRL_RESULT ret = CFileHelper::ChangeFilePermissions( qsSource, qsDest, pAuthHelper );
	if ( PRL_FAILED(ret) )
		return ret;

	QDir	cDir( qsDest );
	cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

	cDir.setSorting( QDir::Name | QDir::DirsLast );
	QFileInfoList cFileList = cDir.entryInfoList();

	// recursive changing permissions
	for ( int i = 0; i < cFileList.size(); i++  )
	{
		if ( cFileList.at(i).isDir() )
		{
			// #125021 to prevent infinity recursion by QT bug in QDir::entryInfoList()
			if( QFileInfo(qsDest) == cFileList.at(i) )
				continue;

			QString strTemp = cFileList.at( i ).filePath();
			CFileHelper::ChangeDirectoryPermissions( qsSource, strTemp, pAuthHelper );
		}
		else
		{
			ret = CFileHelper::ChangeFilePermissions( qsSource, cFileList.at(i).filePath(), pAuthHelper );
			if ( PRL_FAILED(ret) )
				return ret;
		}
	}
	return PRL_ERR_SUCCESS;
}


PRL_RESULT CFileHelper::GetSimplePermissionsToFile( const QString & strFileName
	 , CAuth::AccessMode& ownerPermissions
	 , CAuth::AccessMode& othersPermissions
	 , bool& flgMixedOthersPermission
	, CAuthHelper& currentUser )
{
	bool ret = CAuth::GetCommonFilePermissions(
		strFileName
		, ownerPermissions
		, othersPermissions
		, flgMixedOthersPermission
		, currentUser.GetAuth()
		);
	return ret ? PRL_ERR_SUCCESS : PRL_ERR_CANT_GET_FILE_PERMISSIONS;
}


PRL_RESULT CFileHelper::SetSimplePermissionsToFile( const QString & strFileName
   , CAuthHelper &_current_owner
   , const CAuth::AccessMode*  pOwnerPermissions
   , const CAuth::AccessMode*  pOthersPermissions
   , bool bRecursive
)
{
	LOG_MESSAGE( DBG_INFO, "Try to set permissions: owner:'%0#o' others: '%#o' to file '%s'"
		, pOwnerPermissions ? *pOwnerPermissions : (-1)
		, pOthersPermissions ? *pOthersPermissions : (-1)
		, QSTR2UTF8( strFileName )
		);

	RawFilePermissions rawPerm = makeRawPermission(pOwnerPermissions, pOthersPermissions);

	QFileInfo fi( strFileName );

	if( !fi.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "%s: file does not exists (path=%s)"
			, __FUNCTION__
			, QSTR2UTF8( strFileName  ) );
		return PRL_ERR_FILE_NOT_FOUND;
	}

	return setRawPermission( fi, _current_owner, rawPerm, bRecursive );
}

CFileHelper::RawFilePermissions CFileHelper::makeRawPermission(
												const CAuth::AccessMode*  pOwnerPermissions,
												const CAuth::AccessMode*  pOthersPermissions)
{
	RawFilePermissions rawPerm;

	// prepare calculations
#	ifndef _WIN_

	mode_t ownerMode = 0, othersMode = 0;

	if( pOwnerPermissions )
	{
		ownerMode = ( *pOwnerPermissions & CAuth::fileMayRead ? S_IRUSR  : 0 )
			|	( *pOwnerPermissions & CAuth::fileMayWrite ? S_IWUSR  : 0 )
			|	( *pOwnerPermissions & CAuth::fileMayExecute ? S_IXUSR  : 0 );
		rawPerm.setPerm( CFileHelper::RawFilePermissions::subjOwner, ownerMode );
	}

	if( pOthersPermissions )
	{
		othersMode = ( *pOthersPermissions & CAuth::fileMayRead ? S_IRGRP | S_IROTH  : 0 )
			|	( *pOthersPermissions & CAuth::fileMayWrite ? S_IWGRP | S_IWOTH  : 0 )
			|	( *pOthersPermissions & CAuth::fileMayExecute ? S_IXGRP | S_IXOTH  : 0 );
		rawPerm.setPerm( CFileHelper::RawFilePermissions::subjOthers, othersMode );
	}


#	else //WINDOWS

	DWORD	dwOwnerMode = 0, dwOthersMode = 0;

	if( pOwnerPermissions )
	{
		dwOwnerMode = ( *pOwnerPermissions & CAuth::fileMayRead ? GENERIC_READ : 0 )
			|	( *pOwnerPermissions & CAuth::fileMayWrite ? GENERIC_WRITE : 0 )
			|	( *pOwnerPermissions & CAuth::fileMayExecute ? GENERIC_EXECUTE : 0 );

		// convert
		if( dwOwnerMode == ( GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE ) )
			dwOwnerMode = GENERIC_ALL;

		rawPerm.setPerm( CFileHelper::RawFilePermissions::subjOwner, dwOwnerMode );
	}

	if( pOthersPermissions )
	{
		dwOthersMode = ( *pOthersPermissions & CAuth::fileMayRead ? GENERIC_READ : 0 )
			|	( *pOthersPermissions & CAuth::fileMayWrite ? GENERIC_WRITE : 0 )
			|	( *pOthersPermissions & CAuth::fileMayExecute ? GENERIC_EXECUTE :0 );
		// convert
		if( dwOthersMode == ( GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE ) )
			dwOthersMode = GENERIC_ALL;

		rawPerm.setPerm( CFileHelper::RawFilePermissions::subjOthers, dwOthersMode );
	}

#	endif

	return rawPerm;
}

bool CFileHelper::IsFileBusy(const QString & strFileName)
{
	QFile file(strFileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (file.error() == QFile::OpenError)
			return true;

	}
	file.close();
	return false;
}

bool CFileHelper::ReadfromCorruptedXmlFileParam(const QString & strFilePath,
										  const QString & strParamName,
										  QString & strParamData)
{
	QFile fileToRead(strFilePath);
	QString strLineData;
	if (fileToRead.open(QFile::ReadOnly))
	{
		QTextStream out(&fileToRead);
		do
		{
			strLineData = out.readLine();
			int iParamPos = strLineData.indexOf(strParamName);
			if (iParamPos != -1)// if found
			{
				int iParamPosValueBegin = strLineData.indexOf("{",iParamPos);
				int iParamPosValueEnd = strLineData.indexOf("}",iParamPosValueBegin);
				if((iParamPosValueBegin != -1) && (iParamPosValueEnd != -1))
				{
					strParamData = strLineData.mid(iParamPosValueBegin,iParamPosValueEnd-iParamPosValueBegin+1);
					return true;
				}
			}
		} while (!strLineData.isNull());
	}
	return false;
}


#define PRL_CHECK_FILE_EXISTS(errCode)\
	{\
		if ( QFileInfo(fi.filePath()).exists() )\
			return errCode;\
		else\
			return PRL_ERR_SUCCESS;\
	}

#ifdef _WIN_
PRL_RESULT CFileHelper::setRawFileOwner( const QFileInfo& fi,
							CAuthHelper::OwnerWrapper& ownerWrapper, bool bRecursive )
#else
	PRL_RESULT CFileHelper::setRawFileOwner( const QFileInfo& fi, uid_t pw_uid, gid_t pw_gid, bool bRecursive )
#endif
{
	QDir::Filters
		dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

	if( !fi.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "%s: file does not exists (path=%s)"
			, __FUNCTION__
			, QSTR2UTF8( fi.filePath() ) );
		return PRL_ERR_FILE_NOT_FOUND;
	}

	PRL_ASSERT(fi.isFile() || fi.isDir() || !QFileInfo( fi.filePath() ).exists() );
	if( ! fi.isFile() && ! fi.isDir() )
		PRL_CHECK_FILE_EXISTS(PRL_ERR_INVALID_ARG)

	if( fi.isDir() && bRecursive )
	{
		QDir dir( fi.absoluteFilePath() );
		QFileInfoList fiList = dir.entryInfoList( dirFilter , QDir::DirsLast );

		foreach( QFileInfo fi2, fiList )
		{
			// #125021, #124821 to prevent infinity recursion by QT bug in QDir::entryInfoList()
			if( fi == fi2 )
				continue;

			PRL_RESULT err;
#ifdef _WIN_
			err = setRawFileOwner( fi2, ownerWrapper, bRecursive );
#else
			err = setRawFileOwner( fi2, pw_uid, pw_gid, bRecursive );
#endif

			if( PRL_FAILED( err) )
			{
				bool bExists = fi2.exists(); // #427686, #427175 skip files, that removed during recursion call

				// #427686 , #448412 - check that setRawFileOwner() call was failed by 'file deleted' reason.
				bExists = PRL_ERR_FILE_NOT_FOUND != err;

				if( bExists )
					return err;
			}
		}//foreach
	}

#ifdef _WIN_

	CSid sidOldOwner;
	CSid sidNewOwner;

	if (!AtlGetOwnerSid((LPWSTR)fi.absoluteFilePath().utf16(), SE_FILE_OBJECT, &sidNewOwner))
		return QFileInfo(fi.absoluteFilePath()).exists() ? PRL_ERR_FAILURE : PRL_ERR_FILE_NOT_FOUND;

	if (ownerWrapper.getAuthHelper())
	{
		if( ! ownerWrapper.getAuthHelper()->GetAuth()->GetTokenHandle() )
			return PRL_ERR_FAILURE;

		HANDLE hToken = ownerWrapper.getAuthHelper()->GetAuth()->GetTokenHandle();

		CAccessToken token;
		token.Attach(hToken);
		if (!token.GetUser(&sidOldOwner))
			return PRL_ERR_FAILURE;
	}
	else
	{
		if (!AtlGetOwnerSid((LPWSTR)ownerWrapper.getFileName().utf16(), SE_FILE_OBJECT, &sidOldOwner))
			return PRL_ERR_FAILURE;
	}

	if (sidOldOwner == sidNewOwner)
		return PRL_ERR_SUCCESS;
#else

	struct stat fStat;
	if ( stat(QSTR2UTF8(fi.absoluteFilePath()), &fStat) )
	{
		WRITE_TRACE(DBG_FATAL, "%s: stat() failed with errno = %d for file (path=%s)"
			, __FUNCTION__ , errno, QSTR2UTF8( fi.absoluteFilePath() ) );
		return QFileInfo(fi.absoluteFilePath()).exists() ? PRL_ERR_FAILURE : PRL_ERR_FILE_NOT_FOUND;
	}

	if ( pw_uid == fStat.st_uid && pw_gid == fStat.st_gid )
		return PRL_ERR_SUCCESS;

#endif

#ifdef _WIN_
	PRL_RESULT nSetOwnerError = PRL_ERR_SUCCESS;
	if( !CAuthHelper::SetOwnerOfFile( fi.absoluteFilePath(), ownerWrapper, &nSetOwnerError ) )
	{
		WRITE_TRACE(DBG_FATAL, "Change owner failed for '%s' by error %d, nSetOwnerError = %#x"
			, QSTR2UTF8( fi.absoluteFilePath() ), Prl::GetLastError(), nSetOwnerError );

		if( PRL_ERR_FILE_NOT_FOUND == nSetOwnerError )
			return PRL_ERR_FILE_NOT_FOUND;

		return PRL_ERR_FAILURE;
	}
#else // ifndef_WIN_

	//https://bugzilla.sw.ru/show_bug.cgi?id=433462
	//Check num of hard links on file
	//Do not work with symlinks as well
	int nLargeFileOpenFlag = 0;
#ifdef _LIN_
	nLargeFileOpenFlag = O_LARGEFILE;
#endif
	int fd = open( QSTR2UTF8( fi.absoluteFilePath() ), O_RDONLY | O_NOFOLLOW | nLargeFileOpenFlag );
	if ( -1 == fd )
	{
		int nErrorNo = errno; // store errno to prevent overwrite by WRITE_TRACE call
		WRITE_TRACE( DBG_FATAL, "Couldn't to open file '%s' due error %d"
			, QSTR2UTF8( fi.absoluteFilePath() ), nErrorNo );

		if ( ENOENT == nErrorNo )
			return PRL_ERR_FILE_NOT_FOUND;
		return PRL_ERR_FAILURE;
	}
	if ( !fi.isDir() )
	{
		struct stat fInfo;
  		if( !fstat( fd, &fInfo ) )
		{
			if ( fInfo.st_nlink > 1 )
			{
				WRITE_TRACE( DBG_DEBUG, "Change owner: skipping file '%s' due it has %lu of hard links", QSTR2UTF8( fi.absoluteFilePath() ),
							 (unsigned long)fInfo.st_nlink );
				close( fd );
				return PRL_ERR_SUCCESS;
			}
		}
	}

	if ( ::fchown( fd, pw_uid, pw_gid ) )
	{
		int nErrorNo = errno; // store errno to prevent overwrite by WRITE_TRACE call
		if ( ENOENT == nErrorNo )
        {
			close( fd );
			return PRL_ERR_FILE_NOT_FOUND;
        }

		if (!(getuid() == 0 && EPERM == nErrorNo))// if no supported on FS-ignore error
		{
			WRITE_TRACE(DBG_FATAL, "Change owner failed for '%s' by error %d"
				, QSTR2UTF8( fi.absoluteFilePath() )
				, nErrorNo );
			close( fd );

			return PRL_ERR_FAILURE;
		}
	}
	close( fd );
#endif

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CFileHelper::setRawPermission( const QFileInfo& fi,
										 CAuthHelper &_auth_helper,
										 const CFileHelper::RawFilePermissions& rawSysPerm,
										 bool bRecursive )
{
	QDir::Filters
		dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

	if( ! fi.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "%s: file does not exists (path=%s)"
			, __FUNCTION__
			, QSTR2UTF8( fi.filePath() ) );
		return PRL_ERR_FILE_NOT_FOUND;
	}

	PRL_ASSERT(fi.isFile() || fi.isDir() || !QFileInfo( fi.filePath() ).exists() );
	if( ! fi.isFile() && ! fi.isDir() )
		PRL_CHECK_FILE_EXISTS(PRL_ERR_INVALID_ARG)

	if( fi.isDir() && bRecursive )
	{
		QDir dir( fi.absoluteFilePath() );
		QFileInfoList fiList = dir.entryInfoList( dirFilter, QDir::DirsLast );
		foreach( QFileInfo fi2, fiList )
		{
			// #125021, #124821 to prevent infinity recursion by QT bug in QDir::entryInfoList()
			if( fi == fi2 )
				continue;

			PRL_RESULT err = setRawPermission( fi2, _auth_helper, rawSysPerm , bRecursive );
			if( PRL_FAILED( err ) )
			{
				bool bExists = fi2.exists(); // #427686, #427175 skip files, that removed during recursion call

				// #427686 , #448412 - check that setRawPermission() call was failed by 'file deleted' reason.
				bExists = PRL_ERR_FILE_NOT_FOUND != err;

				if( bExists )
					return err;
			}
		}
	}

	CAuth::AccessMode ownerPermissions;
	CAuth::AccessMode othersPermissions;
	bool flgMixedOthersPermission;
	RawFilePermissions::perm_t outPerm;

	PRL_RESULT ret = GetSimplePermissionsToFile(fi.absoluteFilePath(),
												ownerPermissions,
												othersPermissions,
												flgMixedOthersPermission,
												_auth_helper);
	if (PRL_FAILED(ret))
		return QFileInfo(fi.absoluteFilePath()).exists() ? ret : PRL_ERR_FILE_NOT_FOUND;

	RawFilePermissions rawOldPerm = makeRawPermission(
		rawSysPerm.getPerm(RawFilePermissions::subjOwner, outPerm)  ? &ownerPermissions :  NULL,
		rawSysPerm.getPerm(RawFilePermissions::subjOthers, outPerm) ? &othersPermissions : NULL);

	if (rawOldPerm == rawSysPerm)
		return PRL_ERR_SUCCESS;

	CFileHelper::RawFilePermissions sysPerm = rawSysPerm;
	if( fi.isDir() )
	{
		RawFilePermissions::perm_t ownerPerm;
		bool bHasOwnerPerm = sysPerm.getPerm(RawFilePermissions::subjOwner, ownerPerm);
		if (bHasOwnerPerm)
		{
#ifndef _WIN_
			ownerPerm |= S_IXUSR;
#else
			ownerPerm |= GENERIC_EXECUTE;
#endif
			sysPerm.setPerm(RawFilePermissions::subjOwner, ownerPerm);
		}

		RawFilePermissions::perm_t othersPerm;
		bool bHasOthersPerm = sysPerm.getPerm(RawFilePermissions::subjOthers, othersPerm);
		if (bHasOthersPerm)
		{
#ifndef _WIN_
			othersPerm |= (S_IXGRP | S_IXOTH);
#else
			othersPerm |= GENERIC_EXECUTE;
#endif
			sysPerm.setPerm(RawFilePermissions::subjOthers, othersPerm);
		}
	}

#ifndef _WIN_

	//////////////////////////////////////////////////////////////////////////
	// get current  permission
	//////////////////////////////////////////////////////////////////////////
#ifdef _LIN_
	struct stat64 fInfo;
	if( stat64( QSTR2UTF8( fi.absoluteFilePath() ), &fInfo ) )
#else
	struct stat fInfo;
	if( stat( QSTR2UTF8( fi.absoluteFilePath() ), &fInfo ) )
#endif
	{
		int nErrorNo = errno; // store errno to prevent overwrite by WRITE_TRACE call
		WRITE_TRACE(DBG_FATAL, "setRawPermission(): stat( '%s' ) return error %ld (%s)"
			, QSTR2UTF8( fi.absoluteFilePath() )
			, Prl::GetLastError()
			, QSTR2UTF8( Prl::GetLastErrorAsString() ) );
		if ( ENOENT == nErrorNo )
			return PRL_ERR_FILE_NOT_FOUND;
		return PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS;
	}

	mode_t	currOwnerMode	= fInfo.st_mode & S_IRWXU;
	mode_t	currGroupMode	= fInfo.st_mode & S_IRWXG ;
	mode_t	currOthersMode	= fInfo.st_mode & S_IRWXO ;

	//////////////////////////////////////////////////////////////////////////
	// extract raw  permission
	//////////////////////////////////////////////////////////////////////////
	mode_t mode = 0, mode_tmp = 0;
	if( sysPerm.getPerm( CFileHelper::RawFilePermissions::subjOwner, mode_tmp ) )
		mode |= mode_tmp;
	else
		mode |= currOwnerMode;

	if( sysPerm.getPerm( CFileHelper::RawFilePermissions::subjOthers, mode_tmp ) )
		mode |= mode_tmp;
	else
		mode |= currGroupMode | currOthersMode;

	//////////////////////////////////////////////////////////////////////////
	// add execute permission to dirs
	//////////////////////////////////////////////////////////////////////////
	mode_t dir_mode = 0;
	if( fi.isDir() )
	{
		dir_mode |= (mode & S_IRUSR )? S_IXUSR : 0;
		dir_mode |= (mode & ( S_IRGRP | S_IROTH ) )? (S_IXGRP | S_IXOTH) : 0;
	}

	//https://bugzilla.sw.ru/show_bug.cgi?id=433462
	//Check num of hard links on file
	//Do not work with symlinks as well

	int nLargeFileOpenFlag = 0;
#ifdef _LIN_
	nLargeFileOpenFlag = O_LARGEFILE;
#endif
	int fd = open( QSTR2UTF8( fi.absoluteFilePath() ), O_RDONLY | O_NOFOLLOW | nLargeFileOpenFlag );
	if ( -1 != fd )
	{
		if ( !fi.isDir() )
		{
			struct stat fInfo;
			if( !fstat( fd, &fInfo ) )
			{
				if ( fInfo.st_nlink > 1 )
				{
					WRITE_TRACE( DBG_DEBUG, "Change perms: skipping file '%s' due it has %lu of hard links", QSTR2UTF8( fi.absoluteFilePath() ),
								 (unsigned long)fInfo.st_nlink );
					close( fd );
					return PRL_ERR_SUCCESS;
				}
			}
		}

		if ( ::fchmod( fd, mode | dir_mode ) )
		{
			int nErrorNo = errno;
            if ( ENOENT == nErrorNo )
            {
                close( fd );
                return PRL_ERR_FILE_NOT_FOUND;
            }

			if (!(getuid() == 0 && EPERM == errno))// if no supported on FS-ignore error
			{
				WRITE_TRACE(DBG_FATAL, "Can't change permissions (mode %0#4o, dir_mode %0#4o) to file '%s' by error %d ('%s')"
					, mode
					, dir_mode
					, QSTR2UTF8( fi.absoluteFilePath() )
					, nErrorNo
					, QSTR2UTF8( Prl::GetLastErrorAsString() ) );
				close( fd );

				return PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS;
			}
		}
		close( fd );
	}
#else // #ifndef _WIN_
	PRL_RESULT nPrlErr = PRL_ERR_SUCCESS;
#	define ADD_ACCESS_RIGTHS( path, RID, perm  ) \
		{	\
			SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY; \
			QString			strAlias; \
			DWORD rid= RID; \
			DWORD ridType= SECURITY_BUILTIN_DOMAIN_RID; \
	\
			if( SECURITY_LOCAL_SYSTEM_RID == RID || SECURITY_INTERACTIVE_RID == RID ) \
			{	\
				rid = 0; \
				ridType = RID; \
			} \
	\
			if ( ! CAuthHelper::LookupAliasFromRid("", &SIDAuthNT, ridType, rid, strAlias) ) \
				throw QString( "CAuthHelper::LookupAliasFromRid "#RID ); \
	\
			if ( ! CAuth::AddAccessRights( path , strAlias, perm, SUB_CONTAINERS_AND_OBJECTS_INHERIT \
												, false, &nPrlErr) ) \
				throw QString( "AddAccessRights failed for %1" ).arg( strAlias ); \
		}

#define CONVERT_VM_ACCESS_RIGHTS_INTO_NATIVE_RIGHTS(vm_access_rights, target_native_rights)\
		(target_native_rights) = ( (vm_access_rights) & CAuth::fileMayRead ? GENERIC_READ : 0 )\
			|	( (vm_access_rights) & CAuth::fileMayWrite ? GENERIC_WRITE : 0 )\
			|	( (vm_access_rights) & CAuth::fileMayExecute ? GENERIC_EXECUTE : 0 );\
		if( (target_native_rights) == ( GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE ) )\
			target_native_rights = GENERIC_ALL;

	QString path = fi.absoluteFilePath();

	try
	{
		// store to restore
		CAuth::AccessMode prevOwnerMode = 0, prevOtherMode = 0;
		bool flgTmp;
		if( ! CAuth::GetCommonFilePermissions(
			path, prevOwnerMode, prevOtherMode, flgTmp, _auth_helper.GetAuth(), &nPrlErr ) )
			throw QString( "CAuth::GetCommonFilePermissions() failed" );

		CONVERT_VM_ACCESS_RIGHTS_INTO_NATIVE_RIGHTS(prevOwnerMode, prevOwnerMode)
		CONVERT_VM_ACCESS_RIGHTS_INTO_NATIVE_RIGHTS(prevOtherMode, prevOtherMode)

		// Helper for root users.
		//////////////////////////////////////////////////////////////////////////
		// Setup permissions for owner
		//////////////////////////////////////////////////////////////////////////
		{
			QString	 strAlias = getOwner( path, true );

			if ( ! CAuth::AddAccessRights(
				path , strAlias, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT, false, &nPrlErr) )
				throw QString( "AddAccessRights failed for %1. PrlError = %2" ).
				arg( strAlias ).
				arg ( PRL_RESULT_TO_STRING(nPrlErr) );
		}

		//////////////////////////////////////////////////////////////////////////
		// Setup permissions for system and admins
		//////////////////////////////////////////////////////////////////////////
		{
			//////////////////////////////////////////////////////////////////////////
			// Set permission to "Local administrators" group
			ADD_ACCESS_RIGTHS( path, DOMAIN_ALIAS_RID_ADMINS, GENERIC_ALL );

			//////////////////////////////////////////////////////////////////////////
			// Set permission to "SYSTEM"
			ADD_ACCESS_RIGTHS( path, SECURITY_LOCAL_SYSTEM_RID, GENERIC_ALL );
		}
	}
	catch( const QString err )
	{
		//  store GetLastError value,
		//	NOTE: it's no warrant that it is last error, because we do bit code after error,
		//	but it is very simple solution.

		DWORD lastError = GetLastError();

		WRITE_TRACE(DBG_FATAL, "[%s] Can't change permissions to file '%s' by error : %s;"
			"possibly errno = (%d) %s "
			, __FUNCTION__
			, QSTR2UTF8( fi.absoluteFilePath() )
			, QSTR2UTF8( err )
			, lastError
			, QSTR2UTF8( Prl::GetLastErrorAsString() )
			);

		if ( nPrlErr == PRL_ERR_FILE_NOT_FOUND )
			return PRL_ERR_FILE_NOT_FOUND;

		return PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS;
	}//catch

#	undef ADD_ACCESS_RIGTHS
#endif

	return PRL_ERR_SUCCESS;
}


void CFileHelper::RawFilePermissions::setPerm( Subject s, perm_t p)
{
	m_hashPerm[ s ] = p;
}

bool CFileHelper::RawFilePermissions::getPerm( const Subject s,  perm_t& outPerm ) const
{
	if( ! m_hashPerm.contains( s ) )
		return false;

	outPerm =  m_hashPerm[ s ];
	return true;

}

#ifdef _LIN_

#define PATH_FILE_MTAB "/etc/mtab"

namespace
{
	class filesystem
	{
	public:
		filesystem(QString dir, QString fsname, QString type): mnt_dir(dir), mnt_fsname(fsname), mnt_type(type) {};
		~filesystem(){};

		QString mnt_dir;
		QString mnt_fsname;
		QString mnt_type;
	};

	typedef SmartPtr<filesystem> filesystem_t;

	bool sort_filesystem(const filesystem_t &s1, const filesystem_t &s2)
	{
		return s1->mnt_dir > s2->mnt_dir;
	}

	static int getMountEntries(QList<filesystem_t>& mntents)
	{
		FILE* fd = setmntent(PATH_FILE_MTAB, "r");
		if ( fd == NULL )
			return 1;

		struct mntent ent;
		char buf[BUFSIZ];
		while (getmntent_r(fd, &ent, buf, sizeof(buf)) != NULL)
			mntents.append( filesystem_t (new filesystem(ent.mnt_dir,
				ent.mnt_fsname, ent.mnt_type)));

		endmntent(fd);

		qSort(mntents.begin(), mntents.end(), sort_filesystem);
		return 0;
	}
}
#endif

bool CFileHelper::isFsSupportPermsAndOwner( const QString & strPath )
{
	// FIXME : check of fat and fat32 on windows not implemented yet

	if ( isRemotePath(strPath, QStringList() << "nfs") )
		return false;

	return isLocalFsSupportsPermsManagment(strPath);
}

bool CFileHelper::isRemotePath( const QString & strPath )
{
	return isRemotePath(strPath, QStringList());
}

bool CFileHelper::isRemotePath( const QString & strPath, QStringList lstFsExcludes )
{
	bool isRemote = false;

#ifndef _WIN_
	QString fromname;
	QString fstypename;

#if defined(_LIN_)

	QList<filesystem_t> mntents;
	if (getMountEntries(mntents))
	{
		WRITE_TRACE(DBG_FATAL, "[%s] setmntent '%s' error: %d (%s)"
			, __FUNCTION__
			, QSTR2UTF8(strPath)
			, errno
			, strerror(errno) );
		return isRemote;
	}

	QFileInfo fi(strPath);
	QString canonicalPath = fi.canonicalFilePath();
	filesystem_t mntent;
	foreach(mntent, mntents)
	{
		if ( !canonicalPath.startsWith(mntent->mnt_dir) )
			continue;

		fromname = QString( mntent->mnt_fsname );
		fstypename = QString( mntent->mnt_type );
		break;
	}

#endif // _LIN_

	LOG_MESSAGE( DBG_DEBUG, "[%s] f_fstypename: %s f_mntfromname: %s"
		, __FUNCTION__
		, QSTR2UTF8(fromname)
		, QSTR2UTF8(fstypename) );

	if (lstFsExcludes.contains(fstypename))
		return isRemote;

	if (
		// ftp, nfs  mount
		fromname.contains(":")
		// smb/cifs mount
		|| ( fromname.startsWith("//") && ( ( fstypename == "smbfs" ) || ( fstypename == "cifs" ) ) )
		// mac afp mount
		|| ( fstypename == "afpfs" )
#ifdef _LIN_
		// fuse mount (for example, NTFS over fuse - #PSBM-7665)
		|| ( fstypename == "fuseblk" )
#endif
		)
		isRemote = true;

#else // _WIN_
	if ( strPath.startsWith("\\\\")
		|| strPath.startsWith("//")
		|| GetDriveType((strPath.mid(0, 2) + '\\').utf16()) == DRIVE_REMOTE )
		isRemote = true;
#endif
	LOG_MESSAGE( DBG_DEBUG, "isRemotePath %s: %d", QSTR2UTF8(strPath), isRemote );
	return isRemote;
}

bool CFileHelper::isSharedFS(const QString& fileName)
{
	PRL_FILE_SYSTEM_FS_TYPE fstype = HostUtils::GetFSType(fileName);
	// PRL_FS_FUSE == PCS
	return ((PRL_FS_NFS == fstype)
		|| (PRL_FS_GFS == fstype)
#ifdef _LIN_
		|| (PRL_FS_FUSE == fstype) // for pstorage only
#endif
		|| (PRL_FS_SMBFS == fstype)
		// FIXME: Need add afpfs fstype for Mac.
		);
}

bool CFileHelper::isLocalFsSupportsPermsManagment(const QString & strPath)
{
	PRL_FILE_SYSTEM_FS_TYPE nFsType = HostUtils::GetFSType(strPath);

	if (   nFsType == PRL_FS_FAT
		|| nFsType == PRL_FS_FAT32
#ifndef _WIN_
		|| nFsType == PRL_FS_NTFS
#endif
		)
		return false;

	return true;
}

#ifdef _LIN_
bool CFileHelper::isNtfsPartition ( const QString & strPath )
{
	QList<filesystem_t> mntents;
	if (getMountEntries(mntents))
	{
		WRITE_TRACE(DBG_FATAL, "[%s] setmntent '%s' error: %d (%s)"
			, __FUNCTION__
			, QSTR2UTF8(strPath)
			, errno
			, strerror(errno) );
		return (false);
	}

	filesystem_t mntent;
	foreach(mntent, mntents)
	{
		if ( !strPath.startsWith(mntent->mnt_dir) )
			continue;

		if ( "fuseblk" == mntent->mnt_type )
			return (true);
		break;
	}

	return (false);
}
#endif

namespace
{
	QString getTargetPath(const QString & path)
	{
		QString target;
		QFileInfo fi(path);
		if( !fi.exists() )
			target = fi.absoluteFilePath();
		else
		{
			target = fi.canonicalFilePath();
			if( target.isEmpty() ) // symlink is wrong
				target = fi.absoluteFilePath();
		}
		return target;
	}
}

bool CFileHelper::IsPathsEqual( const QString & path1, const QString & path2 )
{
	if( path1.isEmpty() || path2.isEmpty() )
		return false;

	//////////////////////////////////////////////////////////////////////////
	// check paths with symlinks
	QString target1 = getTargetPath(path1);
	QString target2 = getTargetPath(path2);

	//////////////////////////////////////////////////////////////////////////
	// check paths CaseInsensitive filesystems
	if( target1.toLower() != target2.toLower() )
		return false;

	if( QFileInfo(target1).exists() && QFileInfo(target2).exists() )
		return QFileInfo(target1) == QFileInfo(target2);
	else
	{
		// Calculate default FileSystem behavior for different platform.
		// It is not the best way but it is maximum that we can do without
		// make request to Filesystems to understand it CaseSensitive or not.

		// NOTE: It may works wrong for remote and external HDD and local filesystem,
		//		which have different for this platform CaseSensitive behavior.
		//
		// But don't forget that this code will processed only to compare nonexistent paths.
		Qt::CaseSensitivity caseSign = Qt::CaseSensitive;

#if defined(_WIN_)
		caseSign = Qt::CaseInsensitive;
#endif
		return 0 == QString::compare(target1, target2, caseSign );
	}
}


#ifdef _WIN_
static bool isSSDDeviceWin( const QString &devName )
{
	CAuthHelper auth;
	PRL_RESULT res;
	unsigned int rate;

	if (auth.AuthUserBySelfProcessOwner() && auth.isLocalAdministrator()) {
		res = HostUtils::GetAtaDriveRate(devName, rate);
	}
	else {
		// FIXME: IPCFileOpener is not implemented for Windows
		// Linking of CIPCFileOpener in SDK is broken. Comment out this
		// for now
#if 0
		FILE_HANDLE h;
		// Super user opener
		CIPCFileOpen FOpener;

		// Try to open file
		FOpener.InitAsClient();

		// Ask for the file open
		res = FOpener.ReceiveDescriptor(devName, 0, &h);

		// Close anyway
		FOpener.Close();

		if (PRL_FAILED(res)) {
			WRITE_TRACE(DBG_FATAL, "Open: '%s' SUO device open error. (0x%x)",
					QSTR2UTF8(devName), res);
			return false;
		}

		res = HostUtils::GetAtaDriveRate(h, rate);
		CloseHandle(h);
#endif
	}

	if (PRL_FAILED(res))
		return false;

	return (rate==1);
}
 #endif

bool CFileHelper::isSsdDevice(const QString &name)
{
#ifdef _WIN_
	return isSSDDeviceWin(name);
#else
	Q_UNUSED(name);
	return false;
#endif
}

bool CFileHelper::isLocalPath ( const QString & path )
{
	//FIXME: https://bugzilla.sw.ru/show_bug.cgi?id=438743
	return ( !isRemotePath( path ) && !isUsbDrive( path ) && !isFireWireDrive( path ) );
}

bool CFileHelper::isUsbDrive ( const QString & path )
{
	Q_UNUSED( path );
	return (false);
}

bool CFileHelper::isUsbDevice(const QString& name)
{
	Q_UNUSED(name);
	return false;
}


bool CFileHelper::isFireWireDrive ( const QString & path )
{
	Q_UNUSED( path );
	return (false);
}

bool CFileHelper::isFireWireDevice(const QString& name)
{
	Q_UNUSED(name);
	return false;
}


bool CFileHelper::isSsdDrive ( const QString & path )
{
#ifdef _WIN_
	if (isRemotePath(path))
		return false;
	QString volume = HostUtils::GetVolumeID(path);
	if (path == QString("Undefined"))
		return false;
	QString volumeGUID = HostUtils::GetVolumeGUID(volume);
	if (volumeGUID.isEmpty())
		return false;
	if (volumeGUID.endsWith("\\"))
		volumeGUID.truncate(volumeGUID.lastIndexOf("\\"));
	return isSsdDevice(volumeGUID);
#else
	Q_UNUSED(path);
	return false;
#endif
}

#ifdef _WIN_
bool CFileHelper::CreateJunctionPoint(const QString &mountDir, const QString &destDir)
{
	bool bOk = createJunctionPoint(mountDir.utf16(), destDir.utf16());
	if (!bOk)
	{
		WRITE_TRACE(DBG_FATAL, "createJunctionPoint(\"%s\", \"%s\") returns an error: %d",
					QSTR2UTF8(mountDir), QSTR2UTF8(destDir), GetLastError());
	}
	return bOk;
}

bool CFileHelper::DeleteJunctionPoint(const QString &mountDir)
{
	bool bOk = deleteJunctionPoint(mountDir.utf16());
	if (!bOk)
	{
		WRITE_TRACE(DBG_FATAL, "deleteJunctionPoint(\"%s\") returns an error: %d",
					QSTR2UTF8(mountDir), GetLastError());
	}
	return bOk;
}
#endif

#ifdef _WIN_
QString CFileHelper::moduleFilePath(void *const hm)
{
	QString ret;
	DWORD c;
	wchar_t appDir[MAX_PATH];
	c = GetModuleFileNameW((HMODULE)hm, appDir, _countof(appDir));
	if (0 == c)
	{
		WRITE_TRACE(DBG_FATAL, "GetModuleFileNameW() err %u, hm=0x%p",
					(unsigned)GetLastError(), (void *)hm);
		goto end;
	}
	if (_countof(appDir) <= c)
	{
		WRITE_TRACE(DBG_FATAL, "Module file path too long, hm=0x%p",
					(void *)hm);
		goto end;
	}
	ret = QString::fromWCharArray(appDir, c);
end:
	return ret;
}
#endif

bool CFileHelper::calcFileMd5(const QString& filePath,
							  QString& md5Hex,
							  int (*cancel)(void* ),
							  void* cancel_param)
{
	QCryptographicHash	md5(QCryptographicHash::Md5);

	QFile file(filePath);
	if ( ! file.open(QIODevice::ReadOnly) )
		return false;

	const qint64 buf_size = 0x1000000; // 16 Mb

	qint64 pos = 0;
	while(pos < file.size())
	{
		if (cancel && cancel(cancel_param) > 0)
			return false;

		qint64 read_size = buf_size;
		if (pos + read_size > file.size())
			read_size = file.size() - pos;

		uchar* buf = file.map( pos, read_size );
		if ( ! buf )
			return false;

		md5.addData((const char* )buf, (int )read_size);

		file.unmap( buf );

		pos += read_size;
	}

	md5Hex = UTF8_2QSTR(md5.result().toHex()).toLower();

	return true;
}

QString CFileHelper::calcMd5(const QString& s )
{
	QCryptographicHash	md5(QCryptographicHash::Md5);
	md5.addData( s.toUtf8() );
	return UTF8_2QSTR( md5.result().toHex() ).toLower();
}

#ifdef _WIN_
bool CFileHelper::searchFilesByAttributes(const QFileInfo& fiRootDirPath,
		uint/* DWORD */ dwRequiredAttributeMask,
		QStringList& outFoundFilesList,
		CFileHelper::SearchFilesCallback pCallback,
		void* pUserParam, // send to callback
		volatile bool* pCancelSearchFlg,
		quint64* pOutCountOfProcessedDirs,
		quint64* pOutCountOfProcessedFiles,
		quint64* pOutCountOfErrors)
{
	PRL_ASSERT( fiRootDirPath.isDir() );

	bool bRet = true;

	QString dbgPath = fiRootDirPath.absoluteFilePath();

	//FIXME: DEBUG
#if 0
	printf( "\r%s %s", QSTR2UTF8( dbgPath ), "              " );
#endif

	QDir dir( dbgPath );
	QDir::Filters
		dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

	QFileInfoList fiList = dir.entryInfoList( dirFilter , QDir::DirsLast );
	foreach( QFileInfo fi2, fiList )
	{
		// #125021, #124821 to prevent infinity recursion by QT bug in QDir::entryInfoList()
		if( fiRootDirPath == fi2 )
			continue;

		if( pCancelSearchFlg && *pCancelSearchFlg )
		{
			bRet = false;
			WRITE_TRACE( DBG_FATAL, "Search was stopped by user cancel" );
			goto bStopSearch;
		}

		if( fi2.isDir() )
		{
			if( pOutCountOfProcessedDirs )
				(*pOutCountOfProcessedDirs)++;
		}
		else
		{
			if( pOutCountOfProcessedFiles )
				(*pOutCountOfProcessedFiles)++;
		}

		bool bMatch = true;
		DWORD dwAttr = GetFileAttributes( fi2.absoluteFilePath().utf16() );
		if( dwAttr == INVALID_FILE_ATTRIBUTES )
		{
			if( pOutCountOfErrors )
				(*pOutCountOfErrors)++;
		}
		else if( (dwAttr & dwRequiredAttributeMask) == dwRequiredAttributeMask )
			outFoundFilesList << fi2.absoluteFilePath();
		else
			bMatch = false;

		bool bSkipInsideDir = false;
		if( bMatch )
		{
			SFACallbackResult cbRes = cbrContinue;
			if( pCallback )
				cbRes = pCallback( fi2, (dwAttr == INVALID_FILE_ATTRIBUTES), pUserParam );

			switch( cbRes )
			{
			case cbrContinueNotDeeper:
				bSkipInsideDir = true;
				break;
			case cbrStopSearch:
				bRet = false;
				WRITE_TRACE( DBG_FATAL, "Search was stopped after by callback decision for file '%s'. LastError = %d"
					, QSTR2UTF8( fi2.absoluteFilePath() )
					, (dwAttr == INVALID_FILE_ATTRIBUTES)?GetLastError():0 );
				goto bStopSearch;
			default:
				break;
			}
		}

		if( !fi2.isDir() )
			continue;

		if( !bSkipInsideDir && !searchFilesByAttributes( fi2, dwRequiredAttributeMask, outFoundFilesList,
				pCallback, pUserParam, pCancelSearchFlg,
				pOutCountOfProcessedDirs, pOutCountOfProcessedFiles, pOutCountOfErrors )
				)
		{
			bRet = false;
			goto bStopSearch;
		}

	}// foreach

bStopSearch:
	return bRet;
}

/**
 * Check whether specified partition belongs to dynamic disk
 */
bool IsDynamicPartition( PARTITION_INFORMATION_EX &_partition_info )
{
	if ( PARTITION_STYLE_MBR == _partition_info.PartitionStyle )
	{
		BYTE nPartitionType = nPartitionType = _partition_info.Mbr.PartitionType;
		return ( PARTITION_LDM == nPartitionType ||
			 	 PARTITION_NTFT == nPartitionType ||
			 	 VALID_NTFT == PARTITION_LDM );
	}
	else if ( PARTITION_STYLE_GPT == _partition_info.PartitionStyle )
	{
		static GUID PRL_PARTITION_LDM_METADATA_GUID = {0x5808C8AAL, 0x7E8F, 0x42E0, 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3};
		static GUID PRL_PARTITION_LDM_DATA_GUID = {0xAF9B60A0L, 0x1431, 0x4F62, 0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD};
		const GUID &nPartitionType = _partition_info.Gpt.PartitionType;
		return ( PRL_PARTITION_LDM_DATA_GUID == nPartitionType ||
				 PRL_PARTITION_LDM_METADATA_GUID == nPartitionType );
	}
	else//Unknown partition format
		return ( false );
}

//Optimal maximal partitions count - correspond to MSDN
#define MAX_OPTIMAL_PARTITIONS 32

namespace {
class TSimpleHandleWrap
{
public:
	TSimpleHandleWrap( HANDLE hHandle )
	: m_hHandle( hHandle )
	{}
	~TSimpleHandleWrap()
	{
		if ( INVALID_HANDLE_VALUE != m_hHandle )
			CloseHandle( m_hHandle );
	}
	operator HANDLE() const
	{
		return ( m_hHandle );
	}
	operator bool() const
	{
		return ( INVALID_HANDLE_VALUE != m_hHandle );
	}

private:
	HANDLE m_hHandle;
};
}

/**
 * Tries to determine physical disk number for specified volume name
 * @param requesting volume name
 * @return physical disk number
 */
static DWORD GetDiskNumber( TCHAR *sVolumeName )
{
	TSimpleHandleWrap hVolume = CreateFileW(
		QString( "\\\\.\\%1" ).arg( UTF16_2QSTR( sVolumeName ).remove( "\\" ) ).utf16(),
		GENERIC_READ,
		FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT,
		0);

	if ( !hVolume )
	{
		DWORD nResult = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Failed to open disk handle for volume '%s' with error %u",
						QSTR2UTF8(UTF16_2QSTR( sVolumeName )),
						nResult );
		return ( -1 );
	}

	DWORD nBytesReturned = 0;
	QByteArray _buffer;
	_buffer.resize( sizeof( VOLUME_DISK_EXTENTS ) + MAX_OPTIMAL_PARTITIONS * sizeof( DISK_EXTENT) );
	if ( !DeviceIoControl( hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
							(LPVOID)_buffer.data(), (DWORD)_buffer.size(), &nBytesReturned, NULL) )
	{
		DWORD nResult = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Failed to retireve disk extents for volume '%s' with error %u",
						QSTR2UTF8(UTF16_2QSTR( sVolumeName )),
						nResult);
		return ( -1 );
	}
	PVOLUME_DISK_EXTENTS pDiskExtents = (PVOLUME_DISK_EXTENTS)_buffer.constData();
	if ( 0 == pDiskExtents->NumberOfDiskExtents )
	{
		WRITE_TRACE(DBG_FATAL, "Unexpected error: 0 disk extents for volume '%s'",
						QSTR2UTF8(UTF16_2QSTR( sVolumeName )));
		return ( -1 );
	}
	const DISK_EXTENT &_disk_extent = pDiskExtents->Extents[0];
	return ( _disk_extent.DiskNumber );
}

bool CFileHelper::CheckForDynamicDisk( const QString &sPath )
{
	TCHAR sVolumeName[256];
	if ( !GetVolumePathName( QDir::toNativeSeparators( sPath ).utf16(), sVolumeName, sizeof( sVolumeName )/sizeof( TCHAR ) ) )
	{
		DWORD nResult = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Failed to get volume name for path '%s' with error %u", QSTR2UTF8(sPath), nResult);
		return ( false );
	}

	DWORD nDiskNumber = GetDiskNumber( sVolumeName );
	if ( -1 == nDiskNumber )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to retrieve disk number for path '%s' volume '%s'",
						QSTR2UTF8(sPath),
						QSTR2UTF8(UTF16_2QSTR( sVolumeName )));
	}

	TSimpleHandleWrap hDisk = CreateFileW(
		QString( "\\\\?\\PhysicalDrive%1" ).arg( nDiskNumber ).utf16(),
		GENERIC_READ,
		FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT,
		0);
	if ( !hDisk )
	{
		DWORD nResult = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Failed to open disk handle for volume '%s' with error %u",
						QSTR2UTF8(UTF16_2QSTR( sVolumeName )),
						nResult );
		return (false);
	}

	DWORD nBytesReturned = 0;
	QByteArray _buffer;
	_buffer.resize( sizeof( DRIVE_LAYOUT_INFORMATION_EX ) + MAX_OPTIMAL_PARTITIONS * sizeof( PARTITION_INFORMATION_EX ) );
	DeviceIoControl( hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0,
                      (LPVOID) _buffer.data(), (DWORD) _buffer.size(), &nBytesReturned, NULL );
	DWORD nResult = GetLastError();
	if ( ERROR_SUCCESS != nResult )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to get drive layout information for path '%s' disk volume '%s' with error %u",
						QSTR2UTF8(sPath),
						QSTR2UTF8(UTF16_2QSTR( sVolumeName )),
						nResult);
		return ( false );
	}
	PDRIVE_LAYOUT_INFORMATION_EX pDriveInfo = (PDRIVE_LAYOUT_INFORMATION_EX)_buffer.constData();
	for ( DWORD i = 0; i < pDriveInfo->PartitionCount; ++i )
	{
		PARTITION_INFORMATION_EX &_partition_info = pDriveInfo->PartitionEntry[ i ];
		if ( IsDynamicPartition( _partition_info ) )
			return ( true );
	}
	return ( false );
}

#endif

bool CFileHelper::SetFileModificationTime( const QString &sTargetFilePath, const QDateTime &_new_time )
{
#ifdef _WIN_
	//At first try to open file handle
	TSimpleHandleWrap hFile = CreateFileW(
		sTargetFilePath.utf16(),
		FILE_WRITE_ATTRIBUTES,
		FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT,
		0);
	if ( INVALID_HANDLE_VALUE == hFile )
	{
		DWORD dwError = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Failed to open file '%s' with error: %d", QSTR2UTF8(sTargetFilePath), dwError);
		return false;
	}

	//Algorithm of convert time_t to FILETIME was used from here:
	//http://support.microsoft.com/kb/167296
	FILETIME _new_file_time = {0, 0};
	LONGLONG ll = Int32x32To64(_new_time.toTime_t(), 10000000) + 116444736000000000;
	_new_file_time.dwLowDateTime = (DWORD)ll;
	_new_file_time.dwHighDateTime = ll >> 32;

	if ( !SetFileTime( hFile, NULL, NULL, &_new_file_time ) )
	{
		DWORD dwError = GetLastError();
		WRITE_TRACE(DBG_FATAL, "Failed to change modification time of file '%s' with error: %d", QSTR2UTF8(sTargetFilePath), dwError);
		return false;
	}
	return true;
#else
	struct utimbuf _time_value = { QFileInfo( sTargetFilePath ).lastRead().toTime_t(), _new_time.toTime_t() };
	if ( 0 != utime( QSTR2UTF8(sTargetFilePath), &_time_value ) )
	{
		int nError = errno;
		WRITE_TRACE(DBG_FATAL, "Failed to change modification time of file '%s' with error: %d", QSTR2UTF8(sTargetFilePath), nError);
		return false;
	}
	return true;
#endif
}

PRL_VM_LOCATION CFileHelper::GetVmFilesLocationType( const QString &sVmConfigPath )
{
	if ( !QFile::exists( sVmConfigPath ) )
		return ( PVL_UNKNOWN );

	if ( CFileHelper::isLocalPath( sVmConfigPath ) )
		return ( PVL_LOCAL_FS );
	else if ( CFileHelper::isRemotePath( sVmConfigPath ) )
		return ( PVL_REMOTE_FS );
	else if ( CFileHelper::isUsbDrive( sVmConfigPath ) )
		return ( PVL_USB_DRIVE );
	else if ( CFileHelper::isFireWireDrive( sVmConfigPath ) )
		return ( PVL_FIREWIRE_DRIVE );

	return ( PVL_UNKNOWN );
}

QString CFileHelper::homePath()
{
#if defined(_WIN_)
	const wchar_t *h;
	const QString p =
	(0 != (h = _wgetenv(L"HOMEDRIVE"))) ? (QString::fromWCharArray(h) +
										   QString::fromWCharArray(_wgetenv(L"HOMEPATH"))) : QDir::homePath();
	return p;
#else
	const char *h = getenv("HOME");
	if (0 != h)
		return QFile::decodeName(h);

	struct passwd *pw = getpwuid(getuid());
	if (0 == pw)
		return QFile::decodeName("/");

	h = pw->pw_dir;
	if (0 == h)
		h = "/";
	return QFile::decodeName(h);
#endif
}

bool CFileHelper::isThunderboltDrive(const QString & path)
{
	Q_UNUSED(path);
	return false;
}

bool CFileHelper::isThunderboltDevice(const QString &name)
{
	Q_UNUSED(name);
	return false;
}

bool CFileHelper::isRootDevice(const QString &name)
{
	Q_UNUSED(name);
	return false;
}
