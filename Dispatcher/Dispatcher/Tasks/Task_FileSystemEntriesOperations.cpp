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
///	Task_FileSystemEntriesOperations.cpp
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_FileSystemEntriesOperations.h"
#include "Task_CommonHeaders.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include "Libraries/Std/PrlAssert.h"
#include "Libraries/HostUtils/HostUtils.h"

Task_FileSystemEntriesOperations::Task_FileSystemEntriesOperations (
    SmartPtr<CDspClient>& user,
    const SmartPtr<IOPackage>& p )
	:CDspTaskHelper(user, p)
{
}

PRL_RESULT Task_FileSystemEntriesOperations::getFileSystemEntries(const QString& target, CHwFileSystemInfo* fs_info)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		/**
		 * check input parameters
		 */
		PRL_ASSERT(fs_info);
		if( !fs_info )
			throw PRL_ERR_BAD_PARAMETERS;

		if( target.isNull() || target.isEmpty() )
			throw PRL_ERR_NO_TARGET_PATH_SPECIFIED;

		CAuthHelper& ah = getClient()->getAuthHelper();

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &ah );

		// Should exists
		if( !CFileHelper::FileExists(target, &ah) )
		{
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
					target,
					EVT_PARAM_MESSAGE_PARAM_0));
			throw PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
		}

		// Should be a directory
		if( !CFileHelper::DirectoryExists(target, &ah) )
		{
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
					target,
					EVT_PARAM_MESSAGE_PARAM_0));
			throw PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
		}

		// User has permissions to get directory list
		if( !CFileHelper::CanGetDirectoryContents(target, &ah) )
			throw PRL_ERR_ACCESS_DENIED;

		QFileInfo fi_target( target );
		// Workaround of Qt4.1.1/2 bug in QFileInfo
		QDateTime lastModified = fi_target.lastModified();

		/**
		 * populate target directory data
		 */

		QDir dir_target( target );

		CHwFsItem* pFsItemData = new CHwFsItem( dir_target.absolutePath(), 0 /* size always =0 for directories */,
			PSE_DIRECTORY, lastModified );

		unsigned int permissions = 0;
		CAuth::AccessMode mask = ah.CheckFile( target );

		if ( mask & CAuth::fileMayRead )
			permissions = SET_FS_READ_ENABLED( permissions );
		if ( mask & CAuth::fileMayExecute )
			permissions = SET_FS_EXECUTE_ENABLED( permissions );
		if ( mask & CAuth::fileMayWrite )
			permissions = SET_FS_WRITE_ENABLED( permissions );

		pFsItemData->setPermissions( permissions );

		CHwFsItem* pOldFsItem = NULL;
		pOldFsItem = fs_info->getFsParentItem();
		if( pOldFsItem )
			delete pOldFsItem;
		pOldFsItem = NULL;

		fs_info->setFsParentItem( pFsItemData );

		/**
		* get file system type of target
		*/
		fs_info->setFileSystemType( HostUtils::GetFSType( target ) );

		/**
		 * get list of child entries
		 */

		// QDir(MacOs) doesn't show directories with sticky-bit w/o QDir::Hidden
		// ex. /Volumes drwxrwxrwt
		dir_target.setFilter( QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot );
		dir_target.setSorting( QDir::Name );

		QFileInfoList child_entries;

#ifdef _WIN_
		if (CFileHelper::isRemotePath( target ))
			entryInfoListOnNetworkShare(target, child_entries);
		else
#endif
			child_entries = dir_target.entryInfoList();

		for( int iEntryIndex = 0; iEntryIndex < child_entries.size(); ++iEntryIndex )
		{
			QFileInfo entry_info = child_entries.at( iEntryIndex );

			// #125021 to prevent infinity recursion by QT bug in QDir::entryInfoList()
			// #431558 compare paths by spec way to prevent errors with symlinks, unexisting files, ...
			if( CFileHelper::IsPathsEqual(dir_target.absolutePath(), entry_info.absoluteFilePath() ) )
				continue;

			// Workaround of Qt4.1.1/2 bug in QFileInfo
			QDateTime lastModified = entry_info.lastModified();

			CHwFsItem* pChildEntry = new CHwFsItem();

			pChildEntry->setName( entry_info.absoluteFilePath() );
			pChildEntry->setType( entry_info.isDir() ? PSE_DIRECTORY : PSE_FILE );
			pChildEntry->setSize( entry_info.isDir() ? 0 : entry_info.size() );
			pChildEntry->setModified( lastModified );

			unsigned int permissions = 0;
			CAuth::AccessMode mask = ah.CheckFile( entry_info.absoluteFilePath() );

			if ( mask & CAuth::fileMayRead )
				permissions = SET_FS_READ_ENABLED( permissions );
			if ( mask & CAuth::fileMayExecute )
				permissions = SET_FS_EXECUTE_ENABLED( permissions );
			if ( mask & CAuth::fileMayWrite )
				permissions = SET_FS_WRITE_ENABLED( permissions );

			pChildEntry->setPermissions( permissions );

			// add child entry
			fs_info->addFileSystemItem( pChildEntry );
		}

		/**
		 * cleanup and exit
		 */

		pFsItemData = NULL;
		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, target, EVT_PARAM_MESSAGE_PARAM_0));

		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while getting file system entries with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}
	return ret;
}

// Rename directory or file entry
PRL_RESULT Task_FileSystemEntriesOperations::renameFsEntry ( QString oldFileName,
															QString newFileName,
															CHwFileSystemInfo* fs_info )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		CAuthHelper& ah = getClient()->getAuthHelper();

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &ah );

		bool isDir = QFileInfo(newFileName).isDir();
		if ( !CFileHelper::FileExists(oldFileName, &ah) )
			throw PRL_ERR_ENTRY_DOES_NOT_EXIST;

		if ( CFileHelper::FileExists(newFileName, &ah) )
		{
			if (isDir)
				throw PRL_ERR_ENTRY_DIR_ALREADY_EXISTS;
			else
				throw PRL_ERR_ENTRY_ALREADY_EXISTS;
		}

		if ( !CFileHelper::RenameEntry(oldFileName, newFileName, &ah) )
		{
			if (isDir)
				throw PRL_ERR_CANT_RENAME_DIR_ENTRY;
			else
				throw PRL_ERR_CANT_RENAME_ENTRY;
		}

		QFileInfo renamedFileInfo( newFileName );
		PRL_RESULT tmpResult = getFileSystemEntries( renamedFileInfo.absolutePath(), fs_info );

		if( PRL_FAILED( tmpResult) )
			throw tmpResult;
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
		switch ( code )
		{
		case PRL_ERR_ENTRY_DOES_NOT_EXIST:
		case PRL_ERR_ENTRY_ALREADY_EXISTS:
		case PRL_ERR_CANT_RENAME_ENTRY:
		case PRL_ERR_CANT_RENAME_DIR_ENTRY:
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, oldFileName, EVT_PARAM_MESSAGE_PARAM_0));
			break;
		default:
			break;
		};

		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while renaming file system entry with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}


// Remove directory or file entry
PRL_RESULT Task_FileSystemEntriesOperations::removeFsEntry ( QString target, CHwFileSystemInfo* fs_info )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		CAuthHelper& ah = getClient()->getAuthHelper();

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &ah );

		if ( !CFileHelper::FileExists(target, &ah) )
			throw PRL_ERR_ENTRY_DOES_NOT_EXIST;

		if ( !CFileHelper::RemoveEntry(target, &ah) )
		{
			if (QFileInfo(target).isDir())
				throw PRL_ERR_CANT_REMOVE_DIR_ENTRY;
			else
				throw PRL_ERR_CANT_REMOVE_ENTRY;
		}

		QFileInfo targetFileInfo( target );
		PRL_RESULT tmpResult = getFileSystemEntries( targetFileInfo.absolutePath(), fs_info );

		if( PRL_FAILED( tmpResult) )
			throw tmpResult;
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
		switch ( code )
		{
		case PRL_ERR_ENTRY_DOES_NOT_EXIST:
		case PRL_ERR_CANT_REMOVE_ENTRY:
		case PRL_ERR_CANT_REMOVE_DIR_ENTRY:
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, target, EVT_PARAM_MESSAGE_PARAM_0));
			break;
		default:
			break;
		};

		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while removing file system entry with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

// Get list of disks
PRL_RESULT Task_FileSystemEntriesOperations::getDisksEntries(CHwFileSystemInfo* fs_info)
{
	/**
	 * remove parent file system entry container since it is not used here
	 */

	CHwFsItem* pOldFsItem = NULL;
	pOldFsItem = fs_info->getFsParentItem();
	if( pOldFsItem )
		delete pOldFsItem;
	pOldFsItem = NULL;

	/**
	 * prepare list of drives
	 */

	CAuthHelper& ah = getClient()->getAuthHelper();

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &ah );

	QFileInfoList child_entries = QDir::drives();
	CHwFsItem* pChildEntry;

	for( int iEntryIndex = 0; iEntryIndex < child_entries.size(); ++iEntryIndex )
	{
		pChildEntry = new CHwFsItem();
		QFileInfo entry_info = child_entries.at( iEntryIndex );

		pChildEntry->setName( entry_info.filePath() );
		pChildEntry->setType( PSE_DRIVE );

		unsigned int permissions = 0;
		CAuth::AccessMode mask =
			ah.CheckFile( entry_info.absoluteFilePath() );

		if ( mask & CAuth::fileMayRead )
			permissions = SET_FS_READ_ENABLED( permissions );
		if ( mask & CAuth::fileMayExecute )
			permissions = SET_FS_EXECUTE_ENABLED( permissions );
		if ( mask & CAuth::fileMayWrite )
			permissions = SET_FS_WRITE_ENABLED( permissions );

		pChildEntry->setPermissions( permissions );

		// add child entry
		fs_info->addFileSystemItem( pChildEntry );
	}

	/**
	 * cleanup and exit
	 */

	pChildEntry = NULL;

	return PRL_ERR_SUCCESS;
}


// Create directory entry
PRL_RESULT Task_FileSystemEntriesOperations::createDirectoryEntry(QString target, CHwFileSystemInfo* fs_info)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		/**
		* check input parameters
		*/
		if( !fs_info )
			throw PRL_ERR_BAD_PARAMETERS;

		if( target.isNull() || target.isEmpty() )
			throw PRL_ERR_NO_TARGET_PATH_SPECIFIED;

		CAuthHelper& ah = getClient()->getAuthHelper();

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &ah );

		if ( CFileHelper::FileExists(target, &ah) )
			throw PRL_ERR_TARGET_NAME_ALREADY_OCCUPIED;

		QFileInfo fi_target( target );
		QDir dir_target( target );

		// create target directory if it does not exist

		// FIXME setup permissions for caller user

		if( !CFileHelper::WriteDirectory(target, &ah) )
			throw PRL_ERR_MAKE_DIRECTORY;

		// reload info about newly created directory
		fi_target = QFileInfo( target );

		/**
		 * populate target directory data
		 */

		CHwFsItem* pFsItemData = new CHwFsItem( dir_target.absolutePath(), 0 /* size always =0 for directories */,
			PSE_DIRECTORY, fi_target.lastModified() );

		unsigned int permissions = 0;
		CAuth::AccessMode mask = ah.CheckFile( target );

		if ( mask & CAuth::fileMayRead )
			permissions = SET_FS_READ_ENABLED( permissions );
		if ( mask & CAuth::fileMayExecute )
			permissions = SET_FS_EXECUTE_ENABLED( permissions );
		if ( mask & CAuth::fileMayWrite )
			permissions = SET_FS_WRITE_ENABLED( permissions );

		pFsItemData->setPermissions( permissions );

		CHwFsItem* pOldFsItem = NULL;
		pOldFsItem = fs_info->getFsParentItem();
		if( pOldFsItem )
			delete pOldFsItem;
		pOldFsItem = NULL;

		fs_info->setFsParentItem( pFsItemData );

		/**
		 * cleanup and exit
		 */

		pFsItemData = NULL;
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, target, EVT_PARAM_MESSAGE_PARAM_0));

		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while creating directory with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;

}

// Check if an user can create a file
PRL_RESULT Task_FileSystemEntriesOperations::canCreateFile( const QString& qsFullPath )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if( qsFullPath.isEmpty() )
			throw PRL_ERR_NO_TARGET_PATH_SPECIFIED;

		QString qsDir = QFileInfo(qsFullPath).dir().absolutePath();
		CAuthHelper& ah = getClient()->getAuthHelper();

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &ah );

		if ( !CFileHelper::DirectoryExists(qsDir, &ah) )
		{
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
					qsDir,
					EVT_PARAM_MESSAGE_PARAM_0));
			throw PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
		}

#ifndef _WIN_
		// For Unix based Os, we need check permissions for parent catalog
		if ( !CFileHelper::FileCanWrite(CFileHelper::GetFileRoot(qsFullPath), &ah) ||
			!CFileHelper::FileCanExecute(CFileHelper::GetFileRoot(qsFullPath), &ah) )
			throw PRL_ERR_ACCESS_DENIED;
#else
		if ( !CFileHelper::FileCanWrite(CFileHelper::GetFileRoot(qsFullPath), &ah) )
			throw PRL_ERR_ACCESS_DENIED;
#endif // _WIN_
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, qsFullPath, EVT_PARAM_MESSAGE_PARAM_0));

		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while checking 'canCreateFile' directory with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

#ifdef _WIN_
// only for windows for network share!
void Task_FileSystemEntriesOperations::entryInfoListOnNetworkShare( const QString & strDir, QFileInfoList & lstFileInfo)
{
	// clear list before fill
	lstFileInfo.clear();

	WIN32_FIND_DATA Find;
	HANDLE hFind;
	QString strDot = ".";
	QString strDotDot = "..";


	// need add "/*" to search on network shares
	hFind = FindFirstFile(QString(strDir + "/*").toStdWString().c_str(), &Find);
	if( hFind != INVALID_HANDLE_VALUE )
	{
		QString strFileName = QString::fromStdWString(std::wstring(Find.cFileName));
		// skip dot and dot
		if (strFileName != strDot &&
					strFileName != strDotDot )
		{
			// add qfileinfo object!
			lstFileInfo << QFileInfo( strDir, strFileName );
		}


		while(FindNextFile(hFind,&Find))
		{
			strFileName = QString::fromStdWString(std::wstring(Find.cFileName));
			if (strFileName == strDot ||
					strFileName == strDotDot )
				continue;

			// fill qfileinfo object!
			lstFileInfo << QFileInfo( strDir, strFileName );
		}
	}
	FindClose(hFind);
}
#endif
