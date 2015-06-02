/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
///		CFileHelperTest.cpp
///
/// @author
///		sandro
///
/// @brief
///		Tests fixture class for testing CFileHelper class functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "CFileHelperTest.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

#ifdef _WIN_
#	include "Libraries/PrlCommonUtils/EfsHelper.h"
#endif

#include <QDir>

#include "Libraries/PrlUuid/Uuid.h"
#include "Interfaces/ParallelsQt.h"

#ifndef _WIN_
#	include <unistd.h>
#	include <sys/types.h>
#	include <pwd.h>
#	include <sys/stat.h>
#endif

#include "Libraries/Logging/Logging.h"

#include "Libraries/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/Std/SmartPtr.h"

#include "Tests/CommonTestsUtils.h"

namespace
{
	const QString g_sParentDir("./test_dir");
	const QString g_sSubDir("./test_dir/test_subdir");
}

class AutoDelQFile
{
public:
	AutoDelQFile( const QString& fname )
		: f( fname ), flgExists( f.exists() )
	{
		f.open(  QIODevice::ReadWrite );
	}

	~AutoDelQFile()
	{
		if( !flgExists)
			QFile::remove( f.fileName() );
	}

private:
	QFile f;
	bool flgExists;
};

class AutoDelQDir
{
	/* NOTE This class can't remove created subdirs inside 'dirName'
	* user should do it by him self or use for it another instance of AutoDelQDir.
	*/

public:
	AutoDelQDir( const QString& dirName )
		: d( dirName ), flgExists( d.exists() )
	{
		if( !flgExists )
		{
			QString sub = d.dirName();
			d = QFileInfo( dirName ).dir();
			LOG_MESSAGE( DBG_DEBUG, "\n\tdir = '%s', sub = '%s' \n\tincommig path = '%s'"
				, QSTR2UTF8( d.dirName() )
				, QSTR2UTF8( sub )
				, QSTR2UTF8( dirName ) );
			if( ! d.mkdir( sub ) )
				WRITE_TRACE(DBG_FATAL, "some shit happends: unable to create dir" );
			if( ! d.cd( sub ) )
				WRITE_TRACE(DBG_FATAL, "some shit happends 2: unable to cd" );
		}
	}

	~AutoDelQDir()
	{
		if( !flgExists)
		{
			QDir::Filters
				dirFilter  = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks;

			QFileInfoList fiList = d.entryInfoList( dirFilter, QDir::DirsLast );
			foreach( QFileInfo fi, fiList )
			{
				if( !fi.isFile() )
					continue;

				if( !QFile(fi.canonicalFilePath()).remove() )
				{
					WRITE_TRACE( DBG_FATAL, "Unable to delete file %s by error %ld"
						, QSTR2UTF8( fi.canonicalFilePath() ), Prl::GetLastError() );
				}
			}

			QString sub = d.dirName();
			d.cdUp();
			if( ! d.rmdir( sub ) )
			{
				WRITE_TRACE(DBG_FATAL, "some shit happends 3: unable to rmdir() by errno=%lu for d = '%s', sub='%s'",
					Prl::GetLastError(),
					QSTR2UTF8( d.absolutePath() ), QSTR2UTF8(sub) );
				if( !QDir().rmdir( QString( "%1/%2" ).arg(d.absolutePath()).arg(sub) ) )
					WRITE_TRACE(DBG_FATAL, "some shit happends 4: unable to rmdir() again");
			}
		}
	}

private:
	QDir d;
	bool flgExists;
};

CFileHelperTest::CFileHelperTest() : m_pAuthHelper(NULL)
{
#ifndef _WIN_
	struct passwd *pPasswd = getpwuid(getuid());
	if (pPasswd)
		m_pAuthHelper = new CAuthHelper(pPasswd->pw_name);
#endif
	if (!m_pAuthHelper)
		m_pAuthHelper = new CAuthHelper;
}

CFileHelperTest::~CFileHelperTest()
{
	delete m_pAuthHelper;
	m_pAuthHelper = NULL;
}

void CFileHelperTest::testClearAndDeleteDir()
{
	QVERIFY(CFileHelper::CreateDirectoryPath(g_sParentDir, m_pAuthHelper));
	QVERIFY(CFileHelper::CreateDirectoryPath(g_sSubDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(g_sSubDir, m_pAuthHelper));
	QVERIFY(CFileHelper::ClearAndDeleteDir(g_sParentDir));
	QVERIFY(!CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(!CFileHelper::DirectoryExists(g_sSubDir, m_pAuthHelper));
}

void CFileHelperTest::testWriteDirectoryOnSubdir()
{
	QVERIFY(!CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(!CFileHelper::DirectoryExists(g_sSubDir, m_pAuthHelper));
	QVERIFY(CFileHelper::WriteDirectory(g_sSubDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(g_sSubDir, m_pAuthHelper));
}

void CFileHelperTest::testWriteDirectoryOnParentDir()
{
	QVERIFY(!CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(CFileHelper::WriteDirectory(g_sParentDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
}

void CFileHelperTest::testWriteDirectoryOnSubdirPathEndingWithSlash()
{
	QString sSubDir = g_sSubDir + '/';
	QVERIFY(!CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(!CFileHelper::DirectoryExists(sSubDir, m_pAuthHelper));
	QVERIFY(CFileHelper::WriteDirectory(sSubDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(g_sParentDir, m_pAuthHelper));
	QVERIFY(CFileHelper::DirectoryExists(sSubDir, m_pAuthHelper));
}

void CFileHelperTest::cleanup()
{
	CFileHelper::ClearAndDeleteDir(g_sParentDir);
}

void CFileHelperTest::testRenameEntryFile()
{
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sOldFileName = g_sParentDir + "/oldfile";
	QString sNewFileName = g_sParentDir + "/newfile";

	CAuthHelper _auth;
	QVERIFY(_auth.AuthUserBySelfProcessOwner());
	QVERIFY(CFileHelper::CreateBlankFile(sOldFileName, &_auth));
	QVERIFY(CFileHelper::RenameEntry(sOldFileName, sNewFileName, &_auth));

	QVERIFY(!QFile::exists(sOldFileName));
	QVERIFY(QFile::exists(sNewFileName));
}

void CFileHelperTest::testRenameEntryDir()
{
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sOldDirName = g_sParentDir + "/olddir";
	QString sNewDirName = g_sParentDir + "/newdir";

	CAuthHelper _auth;
	QVERIFY(_auth.AuthUserBySelfProcessOwner());
	QVERIFY(CFileHelper::CreateDirectoryPath(sOldDirName, &_auth));
	QVERIFY(CFileHelper::RenameEntry(sOldDirName, sNewDirName, &_auth));

	QVERIFY(!QFile::exists(sOldDirName));
	QVERIFY(QFile::exists(sNewDirName));
}

void CFileHelperTest::testRenameEntryForNonAccessEntry()
{
#ifdef _WIN_
	QSKIP("Skipping test sue permissions issues under Win platform", SkipAll);
#endif
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sOldDirName = g_sParentDir + "/olddir";
	QString sNewDirName = g_sParentDir + "/newdir";

	QVERIFY(QDir().mkdir(sOldDirName));
	QFile _dir(sOldDirName);
	QVERIFY(_dir.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

	CAuthHelper _auth(TestConfig::getUserLogin());
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));
	QVERIFY(!CFileHelper::RenameEntry(sOldDirName, sNewDirName, &_auth));

	QVERIFY(QFile::exists(sOldDirName));
	QVERIFY(!QFile::exists(sNewDirName));
}

void CFileHelperTest::testRemoveEntryFile()
{
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sFileName = g_sParentDir + "/test_file";
	CAuthHelper _auth;
	QVERIFY(_auth.AuthUserBySelfProcessOwner());
	QVERIFY(CFileHelper::CreateBlankFile(sFileName, &_auth));
	QVERIFY(QFile::exists(sFileName));

	QVERIFY(CFileHelper::RemoveEntry(sFileName, &_auth));
	QVERIFY(!QFile::exists(sFileName));
}

void CFileHelperTest::testRemoveEntryForNonAccessEntry()
{
#ifdef _WIN_
	QSKIP("Skipping test sue permissions issues under Win platform", SkipAll);
#endif
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sDirName = g_sParentDir + "/test_dir";

	QVERIFY(QDir().mkdir(sDirName));
	QFile _dir(sDirName);
	QVERIFY(_dir.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

	CAuthHelper _auth(TestConfig::getUserLogin());
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));
	QVERIFY(!CFileHelper::RemoveEntry(sDirName, &_auth));

	QVERIFY(QFile::exists(sDirName));
}

void CFileHelperTest::testCreateDirectoryAndRemoveEntryDir()
{
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sDirName = g_sParentDir + "/test_dir";

	CAuthHelper _auth;
	QVERIFY(_auth.AuthUserBySelfProcessOwner());

	QVERIFY(CFileHelper::CreateDirectoryPath(sDirName, &_auth));
	QVERIFY(QFile::exists(sDirName));

	QVERIFY(CFileHelper::RemoveEntry(sDirName, &_auth));
	QVERIFY(!QFile::exists(sDirName));
}

void CFileHelperTest::testWriteDirectoryAndRemoveEntryDir()
{
	QVERIFY(QDir().mkdir(g_sParentDir));
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));

	QString sDirName = g_sParentDir + "/test_dir";

	CAuthHelper _auth;
	QVERIFY(_auth.AuthUserBySelfProcessOwner());

	QVERIFY(CFileHelper::WriteDirectory(sDirName, &_auth));
	QVERIFY(QFile::exists(sDirName));

	QVERIFY(CFileHelper::RemoveEntry(sDirName, &_auth));
	QVERIFY(!QFile::exists(sDirName));
}

void CFileHelperTest::testGetSimplePermissionsToFile()
{
	QString fname = QString( "%1/%2" ).arg( QDir::currentPath() ).arg( Uuid::createUuid()  );
	AutoDelQFile f( fname );

	LOG_MESSAGE( DBG_INFO, "fname = '%s'", QSTR2UTF8( fname ) );

#ifdef _WIN_

#else

#	define CHECK_GET_PERM( _mode, _retOwnMode, _retOthMode, _retOthMixed ) \
	{	\
		mode_t mode = _mode       ; \
		CAuth::AccessMode retOwnMode = _retOwnMode       ; \
		CAuth::AccessMode retOthMode = _retOthMode       ; \
		bool	retOthMixed = _retOthMixed       ; \
	\
		CAuth::AccessMode ownerPermissions = 0    ;\
		CAuth::AccessMode othersPermissions = 0       ; \
		bool flgMixedOthersPermission = true       ; \
	\
		QVERIFY( 0 == chmod( QSTR2UTF8( fname ), mode ) )       ; \
		LOG_MESSAGE( DBG_INFO, "mode = %0#4o", mode )			;\
	\
		PRL_RESULT err = CFileHelper::GetSimplePermissionsToFile( \
			fname, ownerPermissions, othersPermissions, flgMixedOthersPermission, *m_pAuthHelper )       ; \
		LOG_MESSAGE( DBG_INFO, "ret = %s, own = %d (expect = %d)" \
			", oth = %d (expect %d), flgMixedOthersPermission = %s (expect = %s) "	\
			, PRL_RESULT_TO_STRING( err )	\
			, ownerPermissions, (_retOwnMode)	\
			, othersPermissions,(_retOthMode) \
			, flgMixedOthersPermission ?"true":"false"	\
			, (_retOthMixed)?"true":"false" );	\
	\
		QVERIFY( PRL_SUCCEEDED( err ) )       ; \
		QVERIFY( ownerPermissions == retOwnMode )       ; \
		QVERIFY( othersPermissions == retOthMode )       ; \
		QVERIFY( flgMixedOthersPermission == retOthMixed )       ; \
	}

	typedef CAuth CA;

	//////////////////////////////////////////////////////////////////////////
	// owner only
	//////////////////////////////////////////////////////////////////////////
	CHECK_GET_PERM( ( S_IRUSR ), ( CA::fileMayRead ), 0, false );
	CHECK_GET_PERM( ( S_IRUSR | S_IWUSR ), ( CA::fileMayRead | CA::fileMayWrite ), 0, false );
	CHECK_GET_PERM( ( S_IRUSR | S_IXUSR ), ( CA::fileMayRead | CA::fileMayExecute ), 0, false );
	CHECK_GET_PERM( ( S_IRWXU ), ( CA::fileMayRead | CA::fileMayWrite | CA::fileMayExecute ), 0, false );

	//////////////////////////////////////////////////////////////////////////
	// others only
	//////////////////////////////////////////////////////////////////////////
	CHECK_GET_PERM( ( S_IROTH ), 0, ( 0 ), true );
	CHECK_GET_PERM( ( S_IROTH | S_IWOTH ), 0, ( 0 ), true );
	CHECK_GET_PERM( ( S_IROTH | S_IXOTH ), 0, ( 0 ), true );
	CHECK_GET_PERM( ( S_IRWXO ), 0, ( 0 ), true );

	//////////////////////////////////////////////////////////////////////////
	// group only
	//////////////////////////////////////////////////////////////////////////
	CHECK_GET_PERM( ( S_IRGRP ), 0, ( 0 ), true );
	CHECK_GET_PERM( ( S_IRGRP | S_IWGRP ), 0, ( 0 ), true );
	CHECK_GET_PERM( ( S_IRGRP | S_IXGRP ), 0, ( 0 ), true );
	CHECK_GET_PERM( ( S_IRWXG ), 0, ( 0 ), true );

	//////////////////////////////////////////////////////////////////////////
	// group + others only
	//////////////////////////////////////////////////////////////////////////
	CHECK_GET_PERM( ( S_IRGRP | S_IROTH ), 0, ( CA::fileMayRead ), false );

	CHECK_GET_PERM( ( ( S_IRGRP | S_IWGRP ) | ( S_IROTH ) ), \
		0, ( CA::fileMayRead ), true );
	CHECK_GET_PERM( ( S_IRGRP | S_IWGRP ) | ( S_IROTH | S_IWOTH ), \
		0, ( CA::fileMayRead | CA::fileMayWrite ), false );
	CHECK_GET_PERM( ( S_IRGRP | S_IXGRP ) | ( S_IXOTH ), \
		0, ( CA::fileMayExecute ), true );
	CHECK_GET_PERM( ( S_IRGRP | S_IXGRP ) | ( S_IROTH | S_IXOTH ), \
		0, ( CA::fileMayRead | CA::fileMayExecute ), false );
	CHECK_GET_PERM( ( S_IRGRP | S_IXGRP ) | ( S_IWOTH ), \
		0, ( 0 ), true );

	CHECK_GET_PERM( ( S_IRWXG | S_IRWXO ), 0, ( CA::fileMayRead | CA::fileMayWrite | CA::fileMayExecute ), false );
	CHECK_GET_PERM( ( S_IRGRP | S_IRWXO ), 0, ( CA::fileMayRead ), true );
	CHECK_GET_PERM( ( S_IRWXG | S_IROTH | S_IWOTH ), 0, ( CA::fileMayRead | CA::fileMayWrite ), true );

	//////////////////////////////////////////////////////////////////////////
	// owner + others
	//////////////////////////////////////////////////////////////////////////
	CHECK_GET_PERM( ( S_IRUSR ) | ( S_IRWXG | S_IRWXO )
		, ( CA::fileMayRead ), ( CA::fileMayRead | CA::fileMayWrite | CA::fileMayExecute ), false );
	CHECK_GET_PERM( ( S_IRUSR | S_IWUSR ) | ( S_IRGRP | S_IROTH )
		, ( CA::fileMayRead | CA::fileMayWrite ), ( CA::fileMayRead ), false );
	CHECK_GET_PERM( ( S_IRUSR | S_IXUSR ) |  ( S_IRGRP | S_IXGRP ) | ( S_IROTH | S_IXOTH )
		, ( CA::fileMayRead | CA::fileMayExecute ), ( CA::fileMayRead | CA::fileMayExecute ), false );
	CHECK_GET_PERM( ( S_IRWXU ) | ( S_IRWXG | S_IRWXO ),
		( CA::fileMayRead | CA::fileMayWrite | CA::fileMayExecute ),
		( CA::fileMayRead | CA::fileMayWrite | CA::fileMayExecute ), false );

# undef CHECK_GET_PERM

#endif

}

void CFileHelperTest::testSetSimplePermissionsToFile()
{
	typedef  CAuth AC;

	QString fname = QString( "%1/%2" ).arg( QDir::currentPath() ).arg( Uuid::createUuid()  );
	AutoDelQFile f( fname );

	QString dirName = QString( "%1/%2" ).arg( QDir::currentPath() ).arg( Uuid::createUuid()  );
	AutoDelQDir d( dirName );

	LOG_MESSAGE( DBG_INFO, "\n\tfileName= '%s', \n\tdirName = '%s'", QSTR2UTF8( fname ), QSTR2UTF8( dirName ) );

#ifdef _WIN_

#else
	CAuthHelper _root;
	QVERIFY(_root.AuthUser(0));

	CAuth::AccessMode  own;
	CAuth::AccessMode  oth;
	bool flgCheckNotExists = false;

#define CHECK_SET_FILE_PERM( _fname, _pOwn, _pOth, expMode )	\
	do{	\
		CAuth::AccessMode* pOwn = (_pOwn);\
		CAuth::AccessMode* pOth = (_pOth);\
		mode_t expectedMode = expMode;\
		if ( (!pOth || !pOwn) && ! flgCheckNotExists  )\
		/*Permissions for others or for owner do not changing so let preserve them*/\
		{\
			struct stat fInfo;      \
			QVERIFY( 0 == stat( QSTR2UTF8( _fname ), &fInfo ) );\
			if (!pOth)\
				expectedMode |= fInfo.st_mode & ( S_IRWXG | S_IRWXO );\
			if (!pOwn)\
				expectedMode |= fInfo.st_mode & ( S_IRWXU );\
		}\
	\
		LOG_MESSAGE( DBG_DEBUG, "*pOwn=%d , *pOth=%d \n\tfile='%s'"	\
			, pOwn? *pOwn : -1 \
			, pOth? *pOth:-1	\
			, QSTR2UTF8( _fname ) );	\
		PRL_RESULT err = CFileHelper::SetSimplePermissionsToFile( _fname, _root, (pOwn), (pOth) , false );      \
		if( PRL_FAILED( err ) )	\
			LOG_MESSAGE( DBG_FATAL, "error: %#x, %s", err, PRL_RESULT_TO_STRING( err )  );	\
	\
		if( flgCheckNotExists ) \
		{ \
			QVERIFY ( ! PRL_SUCCEEDED( err ) );\
			QVERIFY ( err == PRL_ERR_FILE_NOT_FOUND );\
			break;      \
		}	\
	\
		QVERIFY ( PRL_SUCCEEDED( err ) );      \
		\
		struct stat fInfo;      \
		QVERIFY( 0 == stat( QSTR2UTF8( _fname ), &fInfo ) );      \
		/*  remove unused info from mode */	\
		mode_t mode = fInfo.st_mode & ( S_IRWXU | S_IRWXG | S_IRWXO );      \
		\
		LOG_MESSAGE( DBG_DEBUG, "mode = %#4o, expMode=%#4o" , mode, expectedMode ); \
		QVERIFY( mode == (expectedMode) );      \
	}while( 0 );


	//////////////////////////////////////////////////////////////////////////
	// check if file not exists
	//////////////////////////////////////////////////////////////////////////
	flgCheckNotExists = true;

	QString notExistsFile =  QString( "%1/%2" ).arg( QDir::currentPath() ).arg( Uuid::createUuid()  );

        CHECK_SET_FILE_PERM( notExistsFile
        , &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
        , NULL
        , S_IRWXU );

	flgCheckNotExists = false;
	//////////////////////////////////////////////////////////////////////////
	// set owners mode only
	//////////////////////////////////////////////////////////////////////////
	// test RWX
	CHECK_SET_FILE_PERM(fname
	, &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
	, NULL
	, S_IRWXU );

	// test RW
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite )
		, NULL
		, ( S_IRUSR | S_IWUSR )  );

	// test RX
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayExecute )
		, NULL
		, ( S_IRUSR | S_IXUSR )  );

	// test R
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead )
		, NULL
		, ( S_IRUSR ) );

	// test R for dir
	CHECK_SET_FILE_PERM( dirName
		, &( own = AC::fileMayRead )
		, NULL
		, ( S_IRUSR | S_IXUSR ) );
	// test recursive (?)

	//////////////////////////////////////////////////////////////////////////
	// set others mode only
	//////////////////////////////////////////////////////////////////////////
	// test RWX
	CHECK_SET_FILE_PERM( fname
		, NULL
		, &( oth = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, ( S_IRWXG | S_IRWXO ) );

	// test RW
	CHECK_SET_FILE_PERM( fname
		, NULL
		, &( oth = AC::fileMayRead | AC::fileMayWrite )
		, ( S_IRGRP | S_IWGRP ) |  ( S_IROTH | S_IWOTH )  );

	// test RX
	CHECK_SET_FILE_PERM( fname
		, NULL
		, &( oth = AC::fileMayRead | AC::fileMayExecute )
		, ( S_IRGRP | S_IXGRP ) |  ( S_IROTH | S_IXOTH )  );

	// test R
	CHECK_SET_FILE_PERM( fname
		, NULL
		, &( oth = AC::fileMayRead )
		, ( S_IRGRP | S_IROTH ) );

	// test R for dir
	CHECK_SET_FILE_PERM( dirName
		, NULL
		, &( oth = AC::fileMayRead )
		, ( S_IRGRP | S_IXGRP ) | ( S_IROTH | S_IXOTH ) );

	// test recursive (?)

	//////////////////////////////////////////////////////////////////////////
	// set owner  + others mode
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// test RWX + RWX
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, &( oth = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, ( S_IRWXU | S_IRWXG | S_IRWXO ) );

	// test RWX + NO_ACCESS
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, &( oth = 0 )
		, ( S_IRWXU ) );

	// test RWX + RW
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, &( oth = AC::fileMayRead | AC::fileMayWrite )
		, ( S_IRWXU | ( S_IRGRP | S_IWGRP ) |  ( S_IROTH | S_IWOTH ) ) );

	// test RWX + RX
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, &( oth = AC::fileMayRead | AC::fileMayExecute )
		, ( S_IRWXU | ( S_IRGRP | S_IXGRP ) |  ( S_IROTH | S_IXOTH ) ) );

	// test RWX + R
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute)
		, &( oth = AC::fileMayRead )
		, ( S_IRWXU |  S_IRGRP | S_IROTH ) );

	//////////////////////////////////////////////////////////////////////////
	// test RW + RW
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead | AC::fileMayWrite )
		, &( oth = AC::fileMayRead | AC::fileMayWrite )
		, ( S_IRUSR | S_IWUSR ) | ( S_IRGRP | S_IWGRP ) |  ( S_IROTH | S_IWOTH )  );

	//////////////////////////////////////////////////////////////////////////
	// test R + R
	CHECK_SET_FILE_PERM( fname
		, &( own = AC::fileMayRead )
		, &( oth = AC::fileMayRead )
		, ( S_IRUSR | S_IRGRP | S_IROTH ) );

	//////////////////////////////////////////////////////////////////////////
	// test R + R for dir
	CHECK_SET_FILE_PERM( dirName
		, &( own = AC::fileMayRead )
		, &( oth = AC::fileMayRead )
		, ( S_IRUSR | S_IXUSR ) | ( S_IRGRP | S_IXGRP ) | ( S_IROTH | S_IXOTH ) );

	// test recursive (?)
#undef CHECK_SET_FILE_PERM

#endif

}

FileBlinker::FileBlinker( const QString& sDirName, int fileCount )
:m_sDirName(sDirName)
, m_bStop(false)
, m_nCreated( 0 )
, m_nRemoved( 0 )
{
	for(int i=0; i<fileCount; i++)
		m_lstFiles << makeFileName();
}

FileBlinker::~FileBlinker()
{
	stop();
	wait();
}

void FileBlinker::stop()
{
	m_bStop = true;
}

QString FileBlinker::makeFileName()
{
	return QString( "%1/%2" ).arg( m_sDirName ).arg( Uuid::createUuid().toString() );
}

void FileBlinker::run()
{
	bool isWIN = false;
#ifdef _WIN_
	isWIN = true;
#endif

	while( !m_bStop )
	{
		for(int i=0; i<m_lstFiles.size(); i++)
		{
			QString& sFName = m_lstFiles[i];
			// two cases:
			// even: test for the same file ( do create/remove)
			// odd: test for deleted file (recreate new file on every iteration)

			if( ! QFile(sFName).exists(sFName) )
			{ } // do nothing
			else if( QFile(sFName).remove() )
				m_nRemoved++;
			else
			{
				if( isWIN )
				{  // Skip error - other thread opens this file. It is normal case for windows.
					continue;
				}

				WRITE_TRACE( DBG_FATAL, "Unable to delete file %s by error %ld"
					, QSTR2UTF8( sFName ), Prl::GetLastError() );
				m_bStop = true;
				break;
			}

			QThread::yieldCurrentThread();

			sFName = makeFileName();

			if( QFile::exists(sFName) )
			{} // do nothing
			else if ( QFile(sFName).open( QIODevice::ReadWrite ) )
				m_nCreated++;
			else
			{
				if( isWIN )
				{  // Skip error - other thread opens this file. It is normal case for windows.
					continue;
				}

				WRITE_TRACE( DBG_INFO, "ERROR: Unable create file %s, error %ld"
					, QSTR2UTF8( sFName ), Prl::GetLastError() );
				m_bStop = true;
				break;
			}
			QThread::yieldCurrentThread();

		}//for(int i=0;
	}//while
}

void CFileHelperTest::testSetSimplePermission_ToDir_WithDeletedContent()
{
#ifdef _WIN_
	QSKIP("Skipping test under Win platform till reopen BUGS #448412/#427686 for windows.", SkipAll);
#endif
	// create directory with many files.
	// thread-1: make N iteration to set recursive permission to directory
	// thread-2: delete/create any files in directory

	typedef  CAuth AC;

	CAuthHelper auth;
	QVERIFY( auth.AuthUserBySelfProcessOwner() );
	CAuth::AccessMode  mode = AC::fileMayRead | AC::fileMayWrite | AC::fileMayExecute;

	QString tmpDir = QDir::currentPath();
	QString dirName = QString( "%1/%2" ).arg( tmpDir ).arg( GEN_VM_NAME_BY_TEST_FUNCTION() );

	WRITE_TRACE( DBG_DEBUG, "dirName = %s", QSTR2UTF8( dirName ) );

	AutoDelQDir d( dirName );

	// thread-2
	FileBlinker fb( dirName, 10 );
	fb.start();


	// thread-1
	for( int i=0; i< 5000; i++ )
	{
		//if( i%100 == 0 )
		WRITE_TRACE( DBG_DEBUG, "ITERATION %d", i );

		CHECK_RET_CODE_EXP( CFileHelper::SetSimplePermissionsToFile( dirName, auth, &mode, &mode , true ) );
		QThread::yieldCurrentThread();

		QVERIFY( fb.isRunning() );
	}
	fb.stop();
	fb.wait();

	QVERIFY( fb.wereFilesBlinked() );
}

void CFileHelperTest::testSetOwner_ToDir_WithDeletedContent()
{
#ifdef _WIN_
	QSKIP("Skipping test under Win platform till reopen BUGS #448412/#427686 for windows.", SkipAll);
#endif
	// create directory with many files.
	// thread-1: make N iteration to set recursive owner to directory
	// thread-2: delete/create any files in directory

	//CAuthHelper auth1;
	//QVERIFY( auth1.AuthUserBySelfProcessOwner() );
	CAuthHelper auth1( TestConfig::getUserLogin2() );
	QVERIFY( auth1.AuthUser( TestConfig::getUserPassword() ) );

	CAuthHelper auth2( TestConfig::getUserLogin() );
	QVERIFY( auth2.AuthUser( TestConfig::getUserPassword() ) );

	QString tmpDir = QDir::currentPath();
	QString dirName = QString( "%1/%2" ).arg( tmpDir ).arg( GEN_VM_NAME_BY_TEST_FUNCTION() );

	WRITE_TRACE( DBG_DEBUG, "dirName = %s", QSTR2UTF8( dirName ) );

	AutoDelQDir d( dirName );

	// thread-2
	FileBlinker fb( dirName, 10 );
	fb.start();

	// thread-1
	for( int i=0; i< 5000; i++ )
	{
		//if( i%100 == 0 )
		WRITE_TRACE( DBG_DEBUG, "ITERATION %d", i );

		CAuthHelper* pAuth = (i%2) ? &auth2: &auth1;

		QVERIFY( CFileHelper::setOwner(dirName, pAuth, true) ) ;
		QThread::yieldCurrentThread();

		QVERIFY( fb.isRunning() );
	}
	fb.stop();
	fb.wait();

	QVERIFY( fb.wereFilesBlinked() );
}

#ifdef _WIN_
static bool SetAttr4testSearchFilesByAttribute( const QString& fName, DWORD dwAddAttr)
{
	DWORD dwAttr = GetFileAttributes( fName.utf16() );
	if( dwAttr == INVALID_FILE_ATTRIBUTES )
		return false;

	if( !SetFileAttributes( fName.utf16(), (dwAttr | dwAddAttr ) ))
	{
		WRITE_TRACE( DBG_FATAL, "Unable to set attribute for file %s", QSTR2UTF8(fName) );
		return false;
	}

	return true;
}
#endif // _WIN_

void CFileHelperTest::slot_testSearchFilesByAttribute( QString sFile )
{
	m_lst_testSearchFilesByAttribute << sFile;
	WRITE_TRACE( DBG_DEBUG, "Found %s", QSTR2UTF8( sFile ) );
}

void CFileHelperTest::testSearchFilesByAttribute()
{
#ifndef _WIN_
	QSKIP("Skipping test under unix platform.", SkipAll);
#else
	// create folder
	// create 2 files, set "Hidden" attribute for one.
	// start search check result.

	DWORD TEST_ATTRIB = FILE_ATTRIBUTE_HIDDEN;
	int nWaitTimeoutInMSec = 20*1000;


	QString tmpDir = QDir::currentPath();
	QString dirName = QString( "%1/%2" ).arg( tmpDir ).arg( GEN_VM_NAME_BY_TEST_FUNCTION() );
	WRITE_TRACE( DBG_DEBUG, "dirName = %s", QSTR2UTF8( dirName ) );
	AutoDelQDir d( dirName );

	QString dirName2 = QString( "%1/%2" ).arg( dirName ).arg( Uuid::createUuid() );

	AutoDelQDir d2( dirName2 );

	QFile f1( QString( "%1/%2" ).arg( dirName).arg(Uuid::createUuid()) );
	QFile f2( QString( "%1/%2" ).arg( dirName2).arg(Uuid::createUuid()) );

	QVERIFY( f1.open(QIODevice::ReadWrite));
	f1.close();
	QVERIFY( f2.open(QIODevice::ReadWrite));
	f2.close();

	QString pathToStart = dirName;

	SmartPtr<EfsFileSearcher> pFs;
	bool bConnected;

	//test-A
	// skip search inside compressed directory
	pFs = SmartPtr<EfsFileSearcher>( new EfsFileSearcher( QStringList() << pathToStart, TEST_ATTRIB ) );
	bConnected = QObject::connect( pFs.getImpl(),
		SIGNAL( encryptedFileFound( QString ) ),
		SLOT( slot_testSearchFilesByAttribute( QString ) ),
		Qt::DirectConnection );
	PRL_ASSERT( bConnected );

	//test-A.1 search for nothing
	m_lst_testSearchFilesByAttribute.clear();
	pFs->start();
	QVERIFY( pFs->wait( nWaitTimeoutInMSec ) );
	QCOMPARE( m_lst_testSearchFilesByAttribute.size(), 0 );
	QCOMPARE( m_lst_testSearchFilesByAttribute, pFs->getFoundList() );

	//test-A.2 search for one file
	m_lst_testSearchFilesByAttribute.clear();
	QVERIFY( SetAttr4testSearchFilesByAttribute( f2.fileName(), TEST_ATTRIB ) );
	pFs->start();
	QVERIFY( pFs->wait( nWaitTimeoutInMSec ) );
	QCOMPARE( m_lst_testSearchFilesByAttribute.size(), 1 );
	QCOMPARE( QFileInfo( m_lst_testSearchFilesByAttribute.at(0)), QFileInfo(f2.fileName()) );
	QCOMPARE( m_lst_testSearchFilesByAttribute, pFs->getFoundList() );

	//test-A.3 search for dir and file (  file should be skipped, because we skip search inside compressed directory )
	m_lst_testSearchFilesByAttribute.clear();
	QVERIFY( SetAttr4testSearchFilesByAttribute( dirName2, TEST_ATTRIB ) );
	pFs->start();
	QVERIFY( pFs->wait( nWaitTimeoutInMSec ) );
	QCOMPARE( m_lst_testSearchFilesByAttribute.size(), 1 );
	QCOMPARE( QFileInfo( m_lst_testSearchFilesByAttribute.at(0)), QFileInfo(dirName2) );
	QCOMPARE( m_lst_testSearchFilesByAttribute, pFs->getFoundList() );

	//test-B //search inside compressed directory
	pFs = SmartPtr<EfsFileSearcher>( new EfsFileSearcher( QStringList() << pathToStart, TEST_ATTRIB, false ) );
	bConnected = QObject::connect( pFs.getImpl(),
		SIGNAL( encryptedFileFound( QString ) ),
		SLOT( slot_testSearchFilesByAttribute( QString ) ),
		Qt::DirectConnection );
	PRL_ASSERT( bConnected );

	//test-B.1 //search inside compressed directory
	m_lst_testSearchFilesByAttribute.clear();
	pFs->start();
	QVERIFY( pFs->wait( nWaitTimeoutInMSec ) );
	QCOMPARE( m_lst_testSearchFilesByAttribute.size(), 2 );
	QCOMPARE( m_lst_testSearchFilesByAttribute, pFs->getFoundList() );
	QVERIFY( m_lst_testSearchFilesByAttribute.contains( dirName2 ) );
	QVERIFY( m_lst_testSearchFilesByAttribute.contains( f2.fileName() ) );

	//QSKIP("Not implemented yet.", SkipAll);
#endif
}

void CFileHelperTest::example_Get_EFS_Users()
{
#ifndef _WIN_
	QSKIP("Skipping test under unix platform.", SkipAll);
#else
	QStringList outEfsUsers;
	EfsUsersDetector detector;
	QVERIFY( detector.getUsersOfEncryptedFileSystem( outEfsUsers ) );
#endif
}

void CFileHelperTest::slot_example_EfsFileSearcher( QString sFile )
{
	WRITE_TRACE( DBG_FATAL, "Found %s", QSTR2UTF8( sFile ) );
}

void CFileHelperTest::example_EfsFileSearcher()
{
#ifndef _WIN_
	QSKIP("Skipping test under unix platform.", SkipAll);
#else
	QString pathToStart = "C:\\_atmp";
	EfsFileSearcher fs( QStringList() << pathToStart, FILE_ATTRIBUTE_COMPRESSED, true );
	bool bConnected = QObject::connect( &fs,
		SIGNAL( encryptedFileFound( QString ) ),
		SLOT( slot_example_EfsFileSearcher( QString ) ),
		Qt::DirectConnection );

	PRL_ASSERT( bConnected );
	fs.start();
	QVERIFY( fs.wait( 10*1000 ) );
	QVERIFY( !fs.finishedByError() );
#endif
}

void CFileHelperTest::example_findFilesByAttr()
{

#ifdef _WIN_
	QString pathToStart = "C:\\";
	DWORD dwAttr = FILE_ATTRIBUTE_COMPRESSED; ;

	QStringList lstArgs;
	LPWSTR *szArglist;
	int nArgs;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if(NULL != szArglist)
	{
		for(int i=0; i<nArgs; i++)
			lstArgs.push_back(UTF16_2QSTR((ushort*)szArglist[i]));

		LocalFree(szArglist);
	}

	if( !szArglist || lstArgs.size() != 3 || lstArgs[2].length() <1 )
	{
		printf( "Wrong arguments. This utility need 2 args: \n"
			"first: path to dir to start\n"
			"second: attribute types ( c | e  ), where c- compressed, e - encrypted\n");
		exit( 1 );
	}

	pathToStart = lstArgs[1];
	if( lstArgs[2][0] == 'c' )
		dwAttr = FILE_ATTRIBUTE_COMPRESSED;
	else if( lstArgs[2][0] == 'e' )
		dwAttr = FILE_ATTRIBUTE_ENCRYPTED;
	else
	{
		printf( "wrong arguments. call it without parameters to get help.\n");
		exit(2);
	}

	printf( "Start search in '%s' for attr %#x:\n", QSTR2UTF8( pathToStart ), dwAttr );

	DWORD tsStart = GetTickCount();

	quint64 outDirsCount = 0;
	quint64 outFilesCount = 0;
	quint64 outErrorsCount = 0;

	QStringList lstFound;

	//searchFiles( QFileInfo(pathToStart), dwAttr, outDirsCount, outFilesCount, lstFound);
	CFileHelper::searchFilesByAttributes( QFileInfo(pathToStart), dwAttr, lstFound
		, CFileHelper::SearchFilesCallbackDefault, 0, 0
		, &outDirsCount, &outFilesCount, &outErrorsCount);

	DWORD tsFinished = GetTickCount();

	//	(void)( (tsFinished - tsStart)/1000 ) ;
	printf( "\nsearch in '%s' for attr %#x was done:\n"
		"processed %llu files, %llu folders, %llu errors\n"
		"time = %u sec\n"
		"found %u files:\n"
		"%s\n",
		QSTR2UTF8( pathToStart ),
		dwAttr,
		outFilesCount, outDirsCount, outErrorsCount,
		(tsFinished - tsStart)/1000,
		lstFound.size(),
		QSTR2UTF8( lstFound.join("\n") )
		);

#endif //_WIN_
}

void CFileHelperTest::testSetModificationTime()
{
	QString sTestFile = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	AutoDelQFile _test_file( sTestFile );
	QDateTime _orig_created = QFileInfo( sTestFile ).created();
	QDateTime _orig_accessed = QFileInfo( sTestFile ).lastRead();
	QDateTime _orig_modified = QFileInfo( sTestFile ).lastModified();

	QDateTime _new_modified = QDateTime::currentDateTime().addSecs( 10 );
	QVERIFY(CFileHelper::SetFileModificationTime( sTestFile, _new_modified ));
	QVERIFY(_orig_created == QFileInfo( sTestFile ).created());
	QVERIFY(_orig_accessed == QFileInfo( sTestFile ).lastRead());
	QCOMPARE(_new_modified.toTime_t(), QFileInfo( sTestFile ).lastModified().toTime_t());
}

#if 0 // to get example change "#if 0" to "#if 1"
int main()
{
	CFileHelperTest test;
	test.example_findFilesByAttr();
	return 0;
}
#endif

