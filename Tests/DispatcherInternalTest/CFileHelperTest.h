/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///		CFileHelper.h
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
#ifndef CFileHelperTest_H
#define CFileHelperTest_H

#include <QtTest/QtTest>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"

class CFileHelperTest : public QObject
{

Q_OBJECT

public:
	CFileHelperTest();
	~CFileHelperTest();

private slots:
	void cleanup();
	void testClearAndDeleteDir();
	void testWriteDirectoryOnSubdir();
	void testWriteDirectoryOnSubdirPathEndingWithSlash();
	void testWriteDirectoryOnParentDir();
	void testRenameEntryFile();
	void testRenameEntryDir();
	void testRenameEntryForNonAccessEntry();
	void testRemoveEntryFile();
	void testRemoveEntryForNonAccessEntry();
	void testCreateDirectoryAndRemoveEntryDir();
	void testWriteDirectoryAndRemoveEntryDir();

	void testGetSimplePermissionsToFile();
	void testSetSimplePermissionsToFile();

	// https://bugzilla.sw.ru/show_bug.cgi?id=448412
	// https://bugzilla.sw.ru/show_bug.cgi?id=427686
	void testSetSimplePermission_ToDir_WithDeletedContent();
	void testSetOwner_ToDir_WithDeletedContent();

	void example_Get_EFS_Users();

	void slot_testSearchFilesByAttribute( QString sFile );
	void testSearchFilesByAttribute();
	void testSetModificationTime();

public:
	void example_EfsFileSearcher();
	void slot_example_EfsFileSearcher( QString sFile );

	void example_findFilesByAttr();
private:
	CAuthHelper *m_pAuthHelper;
	QStringList m_lst_testSearchFilesByAttribute;

};

class FileBlinker: public QThread
{
public:
	FileBlinker( const QString& sDirName, int fileCount );
	~FileBlinker();
	void stop();

	bool wereFilesBlinked() { return m_nCreated && m_nRemoved; }
private:
	void run();
	QString makeFileName();
private:
	QString m_sDirName;
	bool m_bStop;
	int m_nCreated;
	int m_nRemoved;

	QList<QString> m_lstFiles;
};

#endif
