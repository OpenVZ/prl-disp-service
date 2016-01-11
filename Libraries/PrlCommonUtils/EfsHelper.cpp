///////////////////////////////////////////////////////////////////////////////
///
/// @file EfsHelper.cpp
///
/// Library of the EFS-related operations
/// applicable for Windows Vista and Windows 7 OSs.
///
/// @author sergeyt
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

#include "EfsHelper.h"
#include <QProcess>

#include "CFileHelper.cpp"

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/Common.h>
#include <prlcommon/Interfaces/ParallelsQt.h>

#define EFS_MAX_KEY_LENGTH 255

namespace{

bool getSubKeyList( HKEY hKey, QStringList& outSubKeyList )
{
	PRL_ASSERT(hKey);
	if( !hKey )
		return false;
	outSubKeyList.clear();

	// NOTE: CP from 	// http://msdn.microsoft.com/en-us/library/ms724256(VS.85).aspx

	TCHAR    achKey[EFS_MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string
	DWORD    cSubKeys=0;               // number of subkeys

	DWORD i, retCode;

	// Get the class name and the value count.
	retCode = RegQueryInfoKey(
		hKey,                    // key handle
		NULL,                // buffer for class name
		NULL,           // size of class string
		NULL,                    // reserved
		&cSubKeys,               // number of subkeys
		NULL,            // longest subkey size
		NULL,            // longest class string
		NULL,			          // number of values for this key
		NULL,		            // longest value name
		NULL,         // longest value data
		NULL,   // security descriptor
		NULL);       // last write time

	if( retCode != ERROR_SUCCESS )
	{
		WRITE_TRACE( DBG_FATAL, "RegQueryInfoKey() failed by error %d", retCode );
		return false;
	}

	if(!cSubKeys)
		return true;
	for (i=0; i<cSubKeys; i++)
	{
		cbName = EFS_MAX_KEY_LENGTH;
		retCode = RegEnumKeyEx(hKey, i,
			achKey,
			&cbName,
			NULL,
			NULL,
			NULL,
			NULL);
		if (retCode != ERROR_SUCCESS)
		{
			WRITE_TRACE(DBG_FATAL, "RegEnumKeyEx() for index = %d failed by error %d", i, retCode );
			continue;
		}
		QString subKey = UTF16_2QSTR( achKey );
		outSubKeyList << subKey;
	}//for i

	return true;
}

} // namespace

//////////////////////////////////////////////////////////////////////////

EfsUsersDetector::EfsUsersDetector()
:m_hKey(NULL)
{
	LONG err = RegOpenKeyEx( HKEY_USERS, NULL, 0, KEY_WOW64_64KEY | KEY_READ , &m_hKey);
	if( err != ERROR_SUCCESS )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to open HKEY_USERS keys. Error %d", err );
		m_hKey = NULL;
	}
}

EfsUsersDetector::~EfsUsersDetector()
{
	if( m_hKey )
		RegCloseKey(m_hKey);
}

bool EfsUsersDetector::getUsersOfEncryptedFileSystem( EfsUsersDetector::UserList& outEfsUsers )
{
	PRL_ASSERT( m_hKey );
	if( !m_hKey )
		return false;
	outEfsUsers.clear();

	QString sParallelsTmpPrefix = "ParallelsTmp_";

	HKUAutoLoader autoLoader(sParallelsTmpPrefix);
	Q_UNUSED(autoLoader);
	return enumUsers( outEfsUsers, sParallelsTmpPrefix );
}


bool EfsUsersDetector::enumUsers( EfsUsersDetector::UserList& outEfsUsers, const QString& sPrefixForLoadedKeys )
{
	outEfsUsers.clear();

	QStringList lstSubKeys;
	if( !getSubKeyList( m_hKey, lstSubKeys ) )
		return false;

	WRITE_TRACE( DBG_INFO, "Number of subkeys: %d", lstSubKeys.size() );

	QStringList::iterator it;
	for( it=lstSubKeys.begin(); it!=lstSubKeys.end(); it++)
	{
		QString subKey = *it;
		if( subKey.endsWith("_Classes", Qt::CaseInsensitive ) )
			it = lstSubKeys.erase(it);
	}

	WRITE_TRACE( DBG_INFO, "Next %d unique users were detected:\n%s",
		lstSubKeys.size(), QSTR2UTF8( lstSubKeys.join("\n") ) );

	QString sEfsKeyTemplate = "%1\\Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS\\CurrentKeys";
	// check that users has efs cert.
	foreach( QString sSid, lstSubKeys )
	{
		QString sKey = QString(sEfsKeyTemplate).arg(sSid);

		HKEY hKey;
		LONG err = RegOpenKeyEx( HKEY_USERS, sKey.utf16(), 0, KEY_WOW64_64KEY | KEY_READ | KEY_QUERY_VALUE, &hKey);
		if( err != ERROR_SUCCESS )
			continue;
		err = RegQueryValueEx( hKey, TEXT("CertificateHash"), 0, NULL, NULL, NULL );
		if( err != ERROR_FILE_NOT_FOUND )
		{
			if( sSid.startsWith( sPrefixForLoadedKeys ) )
				sSid.remove(0, sPrefixForLoadedKeys.size() );

			outEfsUsers << sSid;
		}

		RegCloseKey(hKey);
	}

	WRITE_TRACE( DBG_INFO, "Next %d users like use EFS:\n%s"
		, outEfsUsers.size(), QSTR2UTF8( outEfsUsers.join("\n") ) );

	return true;
}

EfsUsersDetector::HKUAutoLoader::HKUAutoLoader( const QString& sHivePrefix )
:m_sHivePrefix(sHivePrefix)
{
	// 1. get local user profiles
	// 2. load each to special hive and store load path in registry
	//////////////////////////////////////////////////////////////////////////
	// 1. get local user profiles
	HKEY hKey;
	LONG err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
		0, KEY_WOW64_64KEY | KEY_READ, &hKey);
	if( err != ERROR_SUCCESS )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to open HKEY_LOCAL_MACHINE keys. Error %d", err );
		return;
	}

	QStringList lstSubKeys;
	if( !getSubKeyList( hKey, lstSubKeys ) )
		goto cleanup;

	// 2. load each to special hive and store load path in registry
	foreach( QString sSid, lstSubKeys )
	{
		QString sKey = QString("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%1")
			.arg(sSid);

		HKEY hKey2;
		err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, sKey.utf16(), 0, KEY_WOW64_64KEY | KEY_READ | KEY_QUERY_VALUE, &hKey2);
		if( err != ERROR_SUCCESS )
		{
			WRITE_TRACE( DBG_FATAL, "RegOpenKeyEx() fails by error %d, for key %s", err, QSTR2UTF8( sKey ) );
			continue;
		}

		DWORD dwType;
		TCHAR achPath[MAX_PATH];
		DWORD cbData = MAX_PATH;

		err = RegQueryValueEx( hKey2, TEXT("ProfileImagePath"), 0, &dwType, (LPBYTE)achPath, &cbData );
		if( err != ERROR_SUCCESS )
			WRITE_TRACE( DBG_FATAL, "RegQueryValueEx() fails by error %d for path %s ", err, QSTR2UTF8(sKey) );
		else
		{
			achPath[MAX_PATH-1]=0;
			QString path = UTF16_2QSTR( achPath );
			path += QString( "\\%1").arg( "ntuser.dat");

			// patch for envinroment variables in the path ( like %SystemDrive% )
			QString pathOrig = path;
			Prl::ProcessEnvVariables( path );

			QString sLoadedHive = m_sHivePrefix + sSid;

			QProcess proc;
			proc.start( "reg.exe", QStringList() << "LOAD" << QString( "HKU\\%1").arg(sLoadedHive) << path );
			bool bFinished = proc.waitForFinished();
			if( bFinished && proc.exitCode() == 0 )
				m_lstLoadedHives << sLoadedHive;
			else
			{
				if( !bFinished )
					proc.kill();

				WRITE_TRACE( DBG_FATAL, "RegLoadKey() failed by %s for key '%s' from path '%s' ( orig '%s' )"
					, bFinished?"error":"timeout", QSTR2UTF8( sLoadedHive )
					, QSTR2UTF8( path ), QSTR2UTF8( pathOrig ) );
				WRITE_TRACE( DBG_FATAL, "output: %s %s"
					, QSTR2UTF8( UTF8_2QSTR( proc.readAllStandardOutput() ) )
					, QSTR2UTF8( UTF8_2QSTR( proc.readAllStandardError() ) )
					);
			}
			/*
			err = RegLoadKey(HKEY_USERS, sLoadedHive.utf16(), path.utf16() );
			if( ERROR_SUCCESS == err )
			m_lstLoadedHives << sLoadedHive;
			else
			{
			WRITE_TRACE( DBG_FATAL, "RegLoadKey() failed by error %d for key '%s' from path '%s'"
			, err, QSTR2UTF8( sLoadedHive ), QSTR2UTF8( path ) );
			}
			*/
		}
		RegCloseKey(hKey2);
	} //foreach( QString sSid

	WRITE_TRACE( DBG_INFO, "Loaded hives:\n%s", QSTR2UTF8( m_lstLoadedHives.join("\n") ) );


cleanup:
	if( hKey )
		RegCloseKey( hKey );
}

EfsUsersDetector::HKUAutoLoader::~HKUAutoLoader()
{
	foreach( QString sLoadedHive, m_lstLoadedHives)
	{
		QProcess proc;
		proc.start( "reg.exe", QStringList() << "UNLOAD"<< QString( "HKU\\%1").arg(sLoadedHive) );
		bool bFinished = proc.waitForFinished();
		if( bFinished && proc.exitCode() == 0 )
		{}
		else
		{
			if( !bFinished )
				proc.kill();

			WRITE_TRACE( DBG_FATAL, "RegUnLoadKey() failed by %s for key '%s'"
				, bFinished?"error":"timeout", QSTR2UTF8( sLoadedHive ) );
			WRITE_TRACE( DBG_FATAL, "output: %s %s"
				, QSTR2UTF8( UTF8_2QSTR( proc.readAllStandardOutput() ) )
				, QSTR2UTF8( UTF8_2QSTR( proc.readAllStandardError() ) )
				);
		}


		/*
		// FIXME: NEED enable SE_RESTORE_NAME and SE_BACKUP_NAME privileges.

		LONG err = RegUnLoadKey(HKEY_USERS, sLoadedHive.utf16() );
		if( err != ERROR_SUCCESS )
		{
		WRITE_TRACE( DBG_FATAL, "RegUnLoadKey() failed by error %d for key '%s'"
		, err, QSTR2UTF8( sLoadedHive ));
		}
		*/
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
EfsFileSearcher::EfsFileSearcher(const QStringList& lstRootDirs
	, DWORD dwAttr
	, bool bSkipSearchInEncryptedFolders )
: m_lstRootDirs(lstRootDirs)
, m_dwAttr( dwAttr )
, m_bSkipSearchInEncryptedFolders( bSkipSearchInEncryptedFolders )
, m_bCancelSearch( false )
, m_bRes( true )
{

}

EfsFileSearcher::~EfsFileSearcher()
{
	stop();
	wait();
}

void EfsFileSearcher::emitFoundSignal( QString sPath )
{
	emit encryptedFileFound( sPath );
}

static	CFileHelper::SFACallbackResult
	SearchEcryptedCallback( const QFileInfo& fi, bool bError, void* pUserParam )
{
	if( bError )
		return CFileHelper::cbrContinue;

	EfsFileSearcher* pObj = reinterpret_cast<EfsFileSearcher*>(pUserParam);
	if( pObj )
	{
		pObj->emitFoundSignal( fi.absoluteFilePath() );
		if( fi.isDir() && pObj->doSkipSearchInEncryptedFolders() )
			return CFileHelper::cbrContinueNotDeeper;
	}

	return CFileHelper::cbrContinue;
}

void EfsFileSearcher::run()
{
	m_bRes = true;
	m_lstFoundFiles.clear();
	m_bCancelSearch = false;

	foreach( QString sRootDir, m_lstRootDirs)
	{
		WRITE_TRACE( DBG_INFO, "Begin EFS search in '%s'", QSTR2UTF8( sRootDir) );

		QStringList lst;
		m_bRes &= CFileHelper::searchFilesByAttributes( QFileInfo(sRootDir)
			, m_dwAttr
			, lst
			, SearchEcryptedCallback
			, this
			, &m_bCancelSearch );

		m_lstFoundFiles << lst;
	}

	WRITE_TRACE( DBG_INFO, "EFS Search finished %s, found %d files"
		, m_bRes? "succeded":"failed", m_lstFoundFiles.size() );
}

bool EfsFileSearcher::finishedByError()
{
	return m_bRes;
}

void EfsFileSearcher::stop()
{
	m_bCancelSearch = true;
}

QStringList EfsFileSearcher::getFoundList()
{
	return m_lstFoundFiles;
}

bool EfsFileSearcher::doSkipSearchInEncryptedFolders()
{
	return m_bSkipSearchInEncryptedFolders;
}

