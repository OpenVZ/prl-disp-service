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
///		TestDspFSCommands.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdUserLogin dispatcher API of working with remote file system.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDspFSCommands_H
#define H_TestDspFSCommands_H

#include "TestDispatcherBase.h"

class TestDspFSCommands : public TestDispatcherBase {

Q_OBJECT

private slots:
	void initTestCase();
	void init();
	void cleanup();
	void cleanupTestCase();

private slots:
	void TestDspCmdFsGetDiskList();
	void TestDspCmdFsGetDirectoryEntries();
	void TestDspCmdFsGetDirectoryEntriesForNonAccessTargetDir();
	void TestDspCmdFsGetDirectoryEntriesEmptyDirValue();
	void TestDspCmdFsGetDirectoryEntriesIncorrectPathValue();
	void TestDspCmdFsGetDirectoryEntriesTooLongPathValue();
	void TestDspCmdFsCreateDirectory();
	void TestDspCmdFsCreateDirectoryForNonAccessTargetDir();
	void TestDspCmdFsCreateDirectoryEmptyDirValue();
	void TestDspCmdFsCreateDirectoryIncorrectPathValue();
	void TestDspCmdFsCreateDirectoryTooLongPathValue();
	void TestDspCmdFsRenameEntryFileEntry();
	void TestDspCmdFsRenameEntryDirEntry();
	void TestDspCmdFsRenameEntryForNonAccessEntry();
	void TestDspCmdFsRenameEntryEmptyOldName();
	void TestDspCmdFsRenameEntryEmptyNewName();
	void TestDspCmdFsRenameEntryEmptyBothNames();
	void TestDspCmdFsRenameEntryIncorrectOldName();
	void TestDspCmdFsRenameEntryIncorrectNewName();
	void TestDspCmdFsRenameEntryIncorrectBothNames();
	void TestDspCmdFsRenameEntryTooLongOldName();
	void TestDspCmdFsRenameEntryTooLongNewName();
	void TestDspCmdFsRenameEntryTooLongBothNames();
	void TestDspCmdFsRemoveEntryFileEntry();
	void TestDspCmdFsRemoveEntryDirEntry();
	void TestDspCmdFsRemoveEntryForNonAccessEntry();
	void TestDspCmdFsRemoveEntryEmptyName();
	void TestDspCmdFsRemoveEntryIncorrectName();
	void TestDspCmdFsRemoveEntryTooLongName();

private:
	QString m_sDirName, m_sFileName;
};

#endif //H_TestDspFSCommands_H
