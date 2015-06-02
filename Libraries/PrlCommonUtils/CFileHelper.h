/////////////////////////////////////////////////////////////////////////////
///
/// @file CFileHelper.h
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

#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QString>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QDateTime>
#include <QStringList>

#include "ParallelsDomModel.h"
#include "ParallelsNamespace.h"

#ifndef _WIN_
#include <sys/stat.h>
#endif

#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtilsBase/CSimpleFileHelper.h"

#define MAX_BUFFER_LENGTH	10*512*1024
class CAuthHelper;

/**
 * @brief
 *		  Auxiliary class used for manipulation with file system objects e.g.
 *        files, folders, file pathes.
 */

class CFileHelper
{
private:
    CFileHelper ();

public:
	//////////////////////////////////////////////////////////////////////////
	//
	// file permission operations
	//
	//////////////////////////////////////////////////////////////////////////
	static PRL_RESULT GetSimplePermissionsToFile( const QString & strFileName
		, CAuth::AccessMode& ownerPermissions
		, CAuth::AccessMode& othersPermissions
		, bool& flgMixedOthersPermission
		, CAuthHelper& currentUser );

	/************************************************************************
	* Set permissions as mix of 'SimpleFilePermission'  to file( or dir )
	* @param path to file
	* @param reference to current owner CAuthHelperObject
	* @param  pOwnerPermissions  value of owner permission to setting
	*		   consist of set of  CAuth::FilePermissions
	* NOTE:   if NULL - owner permission doesnt' changed.
	* @param  pOthersPermissions  value of others permission ( group + others)
	*		   consist of set of  CAuth::FilePermissions
	* NOTE:   if NULL - others permission doesnt' changed.
	* @return PRL_ERR_SUCCESS or some error
	************************************************************************/
	static PRL_RESULT SetSimplePermissionsToFile( const QString & strFilePath
		,CAuthHelper &_current_owner
		,const CAuth::AccessMode* pOwnerPermissions
		,const CAuth::AccessMode* pOthersPermissions
		,bool bRecursive );

	// get/set owner of file
	static QString getOwner( const QString & strFileName, bool bUseSlash = false );
	static bool setOwner( const QString & strFileName, CAuthHelper* pAuthHelper, bool bRecursive);
	static bool setOwnerByTemplate( const QString& strFileName, const QString& strTemplateFileName,
		CAuthHelper& currentUser/* to prevent security hole */,
		bool bRecursive);

	// change permissions of destination file or directory to permissions of  source file or directory
	static PRL_RESULT ChangeFilePermissions(
		const QString& qsSource, const QString& qsDest, CAuthHelper* pAuthHelper );

	// change permissions of destination directory with its contents to permissions of  source file or directory
	static PRL_RESULT ChangeDirectoryPermissions(
		const QString& qsSource, const QString& qsDest, CAuthHelper* pAuthHelper );

    // Returns true if the file specified by strFileName exists
    static bool FileExists(QString strFileName, CAuthHelper *pAuthHelper);

    // Returns true if the user can write to the file
    static bool FileCanWrite(QString strFileName, CAuthHelper *pAuthHelper);

	// Returns true if the user can write to the directory
	static bool DirCanWrite(QString strDirName, CAuthHelper *pAuthHelper);

    // Returns true if the user can read file content
    static bool FileCanRead(QString strFileName, CAuthHelper *pAuthHelper);

	/**
	* @brief return possibility of read directory for user
	* recursive check for all parent directory
	* @param CAuthHelper * pAuthHelper	- user auth object
	* @param QString 	- directory path to check
	**/
	static bool CanReadAllDirsToRoot(const QString & strDirectoryPath ,CAuthHelper *pAuthHelper);

    // Returns true if the user can execute file
    static bool FileCanExecute(QString strFileName, CAuthHelper *pAuthHelper);

	static bool IsFileBusy(const QString & strFileName);
    // Returns true if the directory specified by strPath exists
    static bool DirectoryExists(QString strPath, CAuthHelper *pAuthHelper);

public:
	//////////////////////////////////////////////////////////////////////////
	//
	// everything file operations
	//
	//////////////////////////////////////////////////////////////////////////

	// Compare paths through symlinks / and paths on CaseInsensitive file systems.
	// FIXME: Rename to ArePathsEqual
	static bool IsPathsEqual( const QString & path1, const QString & path2 );

    // Returns true if we can get list of directory contents specified by
    // strPath
    static bool CanGetDirectoryContents(QString strPath, CAuthHelper *pAuthHelper);

    // Renames of moves file or directory
    static bool RenameEntry(QString oldName, QString newName, CAuthHelper *pAuthHelper);

	 // Copy file or directory
	 static bool CopyDirectory(const QString& strSourceDir, const QString& strTargetDir, CAuthHelper *pAuthHelper);

    // Removes entry
    static bool RemoveEntry(QString target, CAuthHelper *pAuthHelper);

    // Creates the directory path strPath (with all parent directories)
    static bool WriteDirectory(QString strPath, CAuthHelper *pAuthHelper);

    // Returns the absolute path of given strFilePath
    static QString GetFileRoot(QString strFilePath);

	// Returns the mount point of strFilePath or empty string for error
	static QString GetMountPoint(const QString& strFilePath);

    // Creates empty file
    static bool CreateBlankFile(QString strFilePath, CAuthHelper *pAuthHelper);

    // Creates directory
    static bool CreateDirectoryPath(QString strFilePath, CAuthHelper *pAuthHelper);

	// get only name of file
	static QString GetFileName(QString & strFilePath);

	// get base name of file
	static QString GetBaseFileName(QString & strFilePath);

	// delete files or directories from list of it pathes
	static bool DeleteFilesFromList(QStringList & cList);

	// remove directory with all contained objects
	static bool ClearAndDeleteDir(const QString & strDir);

	// atomic rename file. It uses in crash-safe save mechs
	// NOTE: sFrom and sTo should be on one hdd partiotion!
	static bool AtomicMoveFile( const QString& sFrom, const QString& sTo );

	// check is dir empty
	static bool IsDirHasFiles(const QString & strDir);

	// calculate total size of content of directory (recursive with subdirs)
	static PRL_RESULT GetDirSize( const QString& strDirPath, quint64 *pSize );

	// get free space of a disk that contains a file named by a path argument
	static PRL_RESULT GetDiskAvailableSpace(const QString& qsPath,
											quint64* pAvailableSpace,
											quint64* pTotalSpace = NULL,
											quint64* pFreeSpace = NULL );

	/**
	 * Removes non valid symbols from path
	 * @param processing string value
	 * @return well formed path part string value
	 */
	static QString ReplaceNonValidPathSymbols(const QString &sOriginalString);

	/**
	* Returns true if path is placed on shared filesystem
	* @param path
	*/
	static bool isSharedFS(const QString& fileName);

	/**
	* Returns true if path is placed on device which support file permissions and file owner
	* @param path
	* FIXME : check of fat and fat32 on windows not implemented yet
	*/
	static bool isFsSupportPermsAndOwner( const QString & strPath );

	/**
	* Returns true if path is remote
	* @param path
	* @return true if path is remote
	*/
	static bool isRemotePath( const QString & strPath );

	/**
	* Returns true if path is local (non external) file system
	* @param path
	* @return true if path is local
	*/
	static bool isLocalPath ( const QString & path );

	/**
	* Returns true if path belongs to USB drive
	* @param path
	* @return true if path belongs to USB drive
	*/
	static bool isUsbDrive ( const QString & path );

	static bool isUsbDevice(const QString& name);

	/**
	* Returns true if path belongs to FireWire drive
	* @param path
	* @return true if path belongs to FireWire drive
	*/
	static bool isFireWireDrive ( const QString & path );

	static bool isFireWireDevice(const QString& name);

	/**
	* Returns true if path belongs to SSD drive
	* @param path
	* @return true if path belongs to SSD drive
	*/
	static bool isSsdDrive(const QString & path);

	static bool isSsdDevice(const QString &name);
#ifdef _LIN_
	/**
	* Returns true if path is on NTFS partition
	* @param path
	* @return true if path is on NTFS partition
	*/
	static bool isNtfsPartition ( const QString & path );
#endif

	/**
	* Read from corrupted xml file parameter value
	* @param File Path to read
	* @param const QString & strParamName - name of xml parameter
	* @param QString & strParamData - ot data,
	* @return bool - operation result
	*/
	static bool ReadfromCorruptedXmlFileParam(const QString & strFilePath,
					const QString & strParamName,
					QString & strParamData);

	// Saves XML object data to file. XML object should have
    // 'QDomDocument getXml ( bool )' method.
    template <class T>
    static bool SaveToFile ( T xmlObj, QString strFilePath,
                             CAuthHelper* pAuthHelper )
    {
        if ( !CreateBlankFile(strFilePath, pAuthHelper) )
            return false;

        QFile file( strFilePath );
	if( !file.open(QFile::WriteOnly | QFile::Truncate) ) {
            QFile::remove( strFilePath );
            return false;
        }

	QTextStream out( &file );
	xmlObj->getXml( XML_DOC_INCLUDE_XML_NODE ).save( out, 3 );

	// finalize
	out.flush();
	file.close();

        return true;
    }

	/**
	 * Get invalid path symbols
	 */
	static QString GetInvalidPathSymbols()
	{
		return "\\/:*?\"<>|%";
	}

#ifdef _WIN_
	/**
	* Create junction point to destDir at mountDir
	*/
	static bool CreateJunctionPoint(const QString &mountDir, const QString &destDir);

	/**
	* Delete junction point from mountDir
	*/
	static bool DeleteJunctionPoint(const QString &mountDir);

	/**
	 * Cleanups readonly attribute for specified file
	 */
	static bool CleanupReadOnlyAttr( const QString &sTargetPath );

	/**
	* Returns file path of specified module (.exe/.dll for example)
	* Parameter hm must be of type HMODULE, (void *) is used not to
	* include large windows.h in this cross platform header.
	* On error returns empty string.
	*/
	static QString moduleFilePath(void *const hm);

	/**
	* [WIN] Search files witch attributes exists in required mask by recursion mech
	* Mask makes from File Attributes Values as described in
	* http://msdn.microsoft.com/en-us/library/ee332330(VS.85).aspx
	* PARAMETERS:
	* @param	fiRootDirPath			- path to dir to start search
	* @param	dwRequiredAttributeMask	- attributes mask ( all attributes in mask should be coincided with file )
	* @param	outFoundFilesList		- [out] list with found files( abs pathes )
	* @param	SearchFilesCallback		- in pointer to callback function
	* @param	bContinueThroughErrors			- if true - skip occurred error and continue
	* @param	pOutCountOfProcessedDirs - pointer to get statistic for dirs
	* @param	pOutCountOfProcessedFiles- pointer to get statistic for files
	* @param	pOutCountOfErrors - pointer to get statistic about errors
	* RETURN
	* @return   return false if search was canceled by callback.
	*/

	enum SFACallbackResult  { cbrContinue, cbrStopSearch, cbrContinueNotDeeper };
	typedef SFACallbackResult (*SearchFilesCallback)( const QFileInfo& fiFoundEntry, bool bError, void* pUserParam );

	static SFACallbackResult SearchFilesCallbackDefault( const QFileInfo&, bool, void* )
	{ return cbrContinue; }

	static bool searchFilesByAttributes( const QFileInfo& fiRootDirPath,
		uint/* DWORD */ dwRequiredAttributeMask,
		QStringList& outFoundFilesList,
		SearchFilesCallback pCallback = SearchFilesCallbackDefault,
		void* pUserParam = 0, // send to callback
		volatile bool* pCancelSearchFlg = 0,
		quint64* pOutCountOfProcessedDirs = 0,
		quint64* pOutCountOfProcessedFiles = 0,
		quint64* pOutCountOfErrors = 0);

	/**
	 * Lets to check whether specified path belongs to the dynamic disk
	 * (refer to the http://msdn.microsoft.com/en-us/library/aa363785(VS.85).aspx
	 * for more details about dynamic disks)
	 * @param checking path on filesystem
	 * @return sign whether specified path belongs to dynamic disk
	 */
	static bool CheckForDynamicDisk( const QString &sPath );
#endif

	/**
	 * Calculate md5 for given file
	 */
	static bool calcFileMd5(const QString& filePath,
							QString& md5Hex,
							int (*cancel)(void* ) = 0,
							void* cancel_param = 0);

	/**
	* Calculate md5 for string
	*/
	static QString calcMd5(const QString& s );

	/**
	 * Changes file last modification time on specified one
	 * @param path to the target file
	 * @param setting modification time value
	 * @return sign whether time was successfully changed
	 */
	static bool SetFileModificationTime( const QString &sTargetFilePath, const QDateTime &_new_time );

	static PRL_VM_LOCATION GetVmFilesLocationType( const QString &sVmConfigPath );

	/**
	 * Returns home file path which can be different from QDir::homePath()
	 * when when proper platform-specific environment variables are not
	 * defined
	 * There should be smth like /Users/username for Mac OS and
	 * c:\Users\username for Windows OS.
	 */
	static QString homePath();

	static bool isThunderboltDrive(const QString &path);
	static bool isThunderboltDevice(const QString &name);

	static bool isRootDevice(const QString &name);

private:

	class RawFilePermissions
	{
	public:

		enum Subject { subjOwner = 1, subjOthers = 2 };

	// define perm_t type;
#	ifdef _WIN_
		typedef DWORD	perm_t;
#	else
		typedef mode_t	perm_t;
#	endif

		bool operator==(const RawFilePermissions& rawPerm) const
		{
			return m_hashPerm == rawPerm.m_hashPerm;
		}

	public:
		void setPerm( const Subject , const perm_t );
		bool getPerm( const Subject,  perm_t& outPerm ) const ;
	private:
		QHash< Subject, perm_t > m_hashPerm;
	};

private:
#	ifdef _WIN_
		static PRL_RESULT setRawFileOwner( const QFileInfo& fi,
							CAuthHelper::OwnerWrapper& ownerWrapper, bool bRecursive );
#	else
		static PRL_RESULT setRawFileOwner( const QFileInfo& fi, uid_t pw_uid, gid_t pw_gid, bool bRecursive );
#	endif
	static PRL_RESULT setRawPermission(
		const QFileInfo& fi,
		CAuthHelper &_auth_helper,
		const RawFilePermissions& rawSysPerm ,
		bool bRecursive );

	static bool isRemotePath( const QString & strPath, QStringList lstFsExcludes );
	static bool isLocalFsSupportsPermsManagment(const QString & strPath);

	static RawFilePermissions makeRawPermission(const CAuth::AccessMode*  pOwnerPermissions,
												const CAuth::AccessMode*  pOthersPermissions);
};

#endif // FILEHELPER_H
