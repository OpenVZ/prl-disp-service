///////////////////////////////////////////////////////////////////////////////
///
/// @file EfsHelper.h
///
/// Library of the EFS-related operations
///
/// @author sergeyt
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __EFS_HELPER_H__
#define __EFS_HELPER_H__

#include <QThread>
#include <QStringList>
#include <windows.h>

class EfsUsersDetector;
class EfsFileSearcher;

class EfsFileSearcher: public QThread
{
	/*
	*	This class search any encrypted files on NTFS and emit signal when found.
	*	By default it skip search inside encrypted folders.
	*/
	Q_OBJECT

public:
	EfsFileSearcher( const QStringList& lstRootDirs
		, DWORD dwAttr = (FILE_ATTRIBUTE_ENCRYPTED)
		, bool bSkipSearchInEncryptedFolders = true );
	virtual ~EfsFileSearcher();

	// use inherited start() call to begin search;
	void stop();

// When search finished:
	bool finishedByError();
	QStringList getFoundList();

//internal:
	void emitFoundSignal( QString sPath );
	bool doSkipSearchInEncryptedFolders();

signals:
	void encryptedFileFound( QString sPath ) const;

private:
	virtual void run();

private:
	QStringList m_lstRootDirs;
	DWORD m_dwAttr;

	QStringList m_lstFoundFiles;
	bool m_bSkipSearchInEncryptedFolders;

	volatile bool m_bCancelSearch;
	bool m_bRes;

};


class EfsUsersDetector
{
	/*
	* This class detects users which use Encrypted File System.
	* It loads all users profiles to HKU to the Registry and check key
	*	"Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS\\CurrentKeys"
	*/

public:
	typedef QStringList UserList;

	EfsUsersDetector();
	~EfsUsersDetector();

	// returns false if error occurs.
	// returns true and list of SID for users who has EFS keys
	// May be zero.
	bool getUsersOfEncryptedFileSystem( UserList& outEfsUsers );
private:
	bool enumUsers( UserList& outEfsUsers, const QString& sPrefixForLoadedKeys );

private:
	class HKUAutoLoader
	{
		// Load and unload automatically all local profiles to HKEY_USERS before search EFS keys
	public:
		HKUAutoLoader( const QString& sHivePrefix );
		~HKUAutoLoader();
	private:
		//void enablePrivileges();
		//void disablePrivileges();
	private:
		QString m_sHivePrefix;
		QStringList m_lstLoadedHives;
	};
private:
	HKEY m_hKey;
};


#endif // __EFS_HELPER_H__
