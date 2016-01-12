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
///		TestDspFSCommands.cpp
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
#include "TestDspFSCommands.h"

#include <QtTest>
#include <QDir>
#include <QTemporaryFile>

#include <stdio.h>

#include "SDK/Handles/PveControl.h"
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlxmlmodel/HostHardwareInfo/CHwFileSystemInfo.h>
#include <prlcommon/Logging/Logging.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlUuid/Uuid.h>

#include "Tests/CommonTestsUtils.h"

namespace {
#ifdef _WIN_
	QString g_sIncorrectValue = "#:/incorrect/filename/value";
#else
	QString g_sIncorrectValue = "///incorrect/////filename///////value";
#endif

#define CHECK_PERMISSION(expression, checkMacro) if (expression) {\
			if (!checkMacro(_permissions)) {\
				WRITE_TRACE(DBG_FATAL, "'%s': '%s': expected permission presents but actual not\n", #expression, qPrintable((*_entry)->getName()));\
				return (false);\
			}\
		} else if	(checkMacro(_permissions)){\
			WRITE_TRACE(DBG_FATAL, "'%s': '%s': expected permission not presents but actual presents\n", #expression, qPrintable((*_entry)->getName()));\
			return (false);\
		}

bool CheckPermissions(const QList<CHwFsItem *> &_lst) {
	CAuthHelper auth(TestConfig::getUserLogin());
	if (!auth.AuthUser(TestConfig::getUserPassword())) {
		WRITE_TRACE(DBG_FATAL, "can't auth user[%s] on localhost ", TestConfig::getUserLogin());
		return (false);
	}
	for (QList<CHwFsItem*>::const_iterator _entry = _lst.begin();	_entry != _lst.end(); ++_entry) {
		CAuth::AccessMode _mask = auth.CheckFile((*_entry)->getName());
		unsigned int _permissions = (*_entry)->getPermissions();
		CHECK_PERMISSION(_mask & CAuth::fileMayRead, IS_FS_READ_PERMITTED)
		CHECK_PERMISSION(_mask & CAuth::fileMayExecute, IS_FS_EXECUTE_PERMITTED)
		CHECK_PERMISSION(_mask & CAuth::fileMayWrite, IS_FS_WRITE_PERMITTED)
	}
	return (true);
}

}

//https://bugzilla.sw.ru/show_bug.cgi?id=438537
#define PLATFORM_INDEPEND_TEMP_PATH QDir::tempPath() + "/" + Uuid::createUuid().toString()

#ifndef _WIN_
#define GENERATE_TEMP_PATH "/tmp/" + Uuid::createUuid().toString()
#else
#define GENERATE_TEMP_PATH _auth.getHomePath() + "/" + Uuid::createUuid().toString()
#endif

#define INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sTmpPath)\
	CAuthHelper _auth(TestConfig::getUserLogin());\
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));\
	sTmpPath = GENERATE_TEMP_PATH;\
	QVERIFY(sTmpPath.size());\
	QVERIFY(!QFile::exists(sTmpPath));

void TestDspFSCommands::initTestCase() {
	InitializeTooLongString();
}

void TestDspFSCommands::cleanupTestCase() {
	CleanupTooLongString();
}

void TestDspFSCommands::init() {
	m_pPveControl = new CPveControl(false, m_pHandler);
	Login();
}

void TestDspFSCommands::cleanup() {
	Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl=0;
	CAuthHelper _auth(TestConfig::getUserLogin());
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));
	if (m_sDirName.size()) {
		if (!QDir().rmdir(m_sDirName)) {
			WRITE_TRACE(DBG_FATAL, "Couldn't to remove dir '%s'\n", qPrintable(m_sDirName));
		}
		m_sDirName = "";
	}
	if (m_sFileName.size()) {
		if (!QFile::remove(m_sFileName)) {
			WRITE_TRACE(DBG_FATAL, "Couldn't to remove file '%s'\n", qPrintable(m_sFileName));
		}
		m_sFileName = "";
	}
}

void TestDspFSCommands::TestDspCmdFsGetDiskList() {
	CALL_CMD(m_pPveControl->DspCmdFsGetDiskList(), PVE::DspCmdFsGetDiskList)
	CResult _result=m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

#ifdef _WIN_
	QSKIP("Skip the rest part of test due unexplainable issues of QDir::drives() method behaviour", SkipSingle );
#endif
	QList<QString> actualDrivesList;
	CHwFileSystemInfo _fs_info;
	QVERIFY(_result.m_hashResultSet.contains(PVE::DspCmdFsGetDiskList_strDiskList));
	QCOMPARE(int(_fs_info.fromString(_result.m_hashResultSet[PVE::DspCmdFsGetDiskList_strDiskList])), int(HostHwInfoParser::RcSuccess));
	for (QList<CHwFsItem*>::const_iterator _drive = _fs_info.m_lstFileSystemItems.begin();
				_drive != _fs_info.m_lstFileSystemItems.end(); ++_drive) {
		QVERIFY((*_drive)->isDrive());
		actualDrivesList.push_back((*_drive)->getName());
	}

	QFileInfoList expectedDrivesList = QDir::drives();
	QCOMPARE(actualDrivesList.size(), expectedDrivesList.size());
	for (QFileInfoList::const_iterator _drive = expectedDrivesList.begin(); _drive != expectedDrivesList.end(); ++_drive)
		QVERIFY(CheckElementPresentsInList(actualDrivesList, _drive->absoluteFilePath()));

	QVERIFY(CheckPermissions(_fs_info.m_lstFileSystemItems));
}

void TestDspFSCommands::TestDspCmdFsGetDirectoryEntries() {
#ifdef _WIN_
	QSKIP("Skip the test due it became invalid after CAuth::CheckFile() functionality was changed", SkipSingle );
#endif
	QString sTargetDir = QDir::rootPath();

	CALL_CMD(m_pPveControl->DspCmdFsGetDirectoryEntries(sTargetDir.toUtf8().data()), PVE::DspCmdFsGetDirectoryEntries)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	QList<QString> actualEntriesList;
	CHwFileSystemInfo _fs_info;
	QVERIFY(_result.m_hashResultSet.contains(PVE::DspCmdFsGetDirectoryEntries_strEntriesList));
	QCOMPARE(int(_fs_info.fromString(_result.m_hashResultSet[PVE::DspCmdFsGetDirectoryEntries_strEntriesList])), int(HostHwInfoParser::RcSuccess));
	for (QList<CHwFsItem*>::const_iterator _entry = _fs_info.m_lstFileSystemItems.begin();
				_entry != _fs_info.m_lstFileSystemItems.end(); ++_entry) {
		QVERIFY((*_entry)->isDir() || (*_entry)->isFile());
		actualEntriesList.push_back((*_entry)->getName());
	}

	QDir _target_dir(sTargetDir);
	_target_dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
	_target_dir.setSorting(QDir::Name);
	QFileInfoList expectedEntriesList = _target_dir.entryInfoList();
	QCOMPARE(actualEntriesList.size(), expectedEntriesList.size());
	for (QFileInfoList::const_iterator _entry = expectedEntriesList.begin(); _entry != expectedEntriesList.end(); ++_entry)
		QVERIFY(CheckElementPresentsInList(actualEntriesList, _entry->absoluteFilePath()));

	QVERIFY(CheckPermissions(_fs_info.m_lstFileSystemItems));
}

void TestDspFSCommands::TestDspCmdFsGetDirectoryEntriesForNonAccessTargetDir() {
#ifdef _WIN_
	QSKIP("Skip the rest part of test due unexplainable issues of QFile::setPermissions() method behaviour", SkipSingle );
#endif

	m_sDirName = PLATFORM_INDEPEND_TEMP_PATH;
	QVERIFY(!QFile::exists(m_sDirName));
	QVERIFY(QDir().mkdir(m_sDirName));
	QFile _dir(m_sDirName);
	QVERIFY(_dir.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

	CALL_CMD(m_pPveControl->DspCmdFsGetDirectoryEntries(m_sDirName.toUtf8().data()), PVE::DspCmdFsGetDirectoryEntries)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsGetDirectoryEntriesEmptyDirValue() {
	QString sEmptyDirValue;

	CALL_CMD(m_pPveControl->DspCmdFsGetDirectoryEntries(sEmptyDirValue.toUtf8().data()), PVE::DspCmdFsGetDirectoryEntries)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsGetDirectoryEntriesIncorrectPathValue() {
	CALL_CMD(m_pPveControl->DspCmdFsGetDirectoryEntries(g_sIncorrectValue.toUtf8().data()), PVE::DspCmdFsGetDirectoryEntries)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsGetDirectoryEntriesTooLongPathValue() {
	CALL_CMD(m_pPveControl->DspCmdFsGetDirectoryEntries(GetTooLongString()), PVE::DspCmdFsGetDirectoryEntries)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsCreateDirectory() {
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(m_sDirName)

	CALL_CMD(m_pPveControl->DspCmdFsCreateDirectory(m_sDirName.toUtf8().data()), PVE::DspCmdFsCreateDirectory)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	QVERIFY(QFile::exists(m_sDirName));
}

void TestDspFSCommands::TestDspCmdFsCreateDirectoryForNonAccessTargetDir() {

#ifdef _WIN_
	QSKIP("Skip the rest part of test due unexplainable issues of QFile::setPermissions() method behaviour", SkipSingle );
#endif

	m_sDirName = PLATFORM_INDEPEND_TEMP_PATH;
	QVERIFY(m_sDirName.size());
	QVERIFY(!QFile::exists(m_sDirName));
	QVERIFY(QDir().mkdir(m_sDirName));
	QFile _dir(m_sDirName);

	QFile::Permissions mask = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
	QVERIFY(_dir.setPermissions( mask ));

	QString sNewDirName = m_sDirName + "/" + Uuid::createUuid().toString();

	CALL_CMD(m_pPveControl->DspCmdFsCreateDirectory(sNewDirName.toUtf8().data()), PVE::DspCmdFsCreateDirectory)
	CResult _result = m_pHandler->GetResult();
	QVERIFY( PRL_FAILED(_result.getReturnCode()) );

	//  __asm int 3 ;
	QVERIFY(!QFile::exists(sNewDirName));
}

void TestDspFSCommands::TestDspCmdFsCreateDirectoryEmptyDirValue() {
	QString sEmptyDirValue;

	CALL_CMD(m_pPveControl->DspCmdFsCreateDirectory(sEmptyDirValue.toUtf8().data()), PVE::DspCmdFsCreateDirectory)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsCreateDirectoryIncorrectPathValue() {
	CALL_CMD(m_pPveControl->DspCmdFsCreateDirectory(g_sIncorrectValue.toUtf8().data()), PVE::DspCmdFsCreateDirectory)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsCreateDirectoryTooLongPathValue() {
	CALL_CMD(m_pPveControl->DspCmdFsCreateDirectory(GetTooLongString()), PVE::DspCmdFsCreateDirectory)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryFileEntry() {
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(m_sFileName)

	QString sOldFileName;
	{
		sOldFileName = GENERATE_TEMP_PATH;
		QVERIFY(CFileHelper::CreateBlankFile(sOldFileName, &_auth));
		QVERIFY(QFile::exists(sOldFileName));
	}

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(sOldFileName.toUtf8().data(), m_sFileName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	QVERIFY(!QFile::exists(sOldFileName));
	QVERIFY(QFile::exists(m_sFileName));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryDirEntry() {
	QString sOldDirName;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sOldDirName)

	QVERIFY(CFileHelper::CreateDirectoryPath(sOldDirName, &_auth));
	QVERIFY(QFile::exists(sOldDirName));

	{
		m_sDirName = GENERATE_TEMP_PATH;
		QVERIFY(m_sDirName.size());
		QVERIFY(!QFile::exists(m_sDirName));
	}

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(sOldDirName.toUtf8().data(), m_sDirName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	QVERIFY(!QFile::exists(sOldDirName));
	QVERIFY(QFile::exists(m_sDirName));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryForNonAccessEntry() {
	m_sDirName = PLATFORM_INDEPEND_TEMP_PATH;
	QVERIFY(!QFile::exists(m_sDirName));
	QVERIFY(QDir().mkdir(m_sDirName));
	QFile _dir(m_sDirName);
	QVERIFY(_dir.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

	QString sRenamePath;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sRenamePath)

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(m_sDirName.toUtf8().data(), sRenamePath.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));

	QVERIFY(!QFile::exists(sRenamePath));
	QVERIFY(QFile::exists(m_sDirName));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryEmptyOldName() {
	QString sOldFileName;
	QString sNewFileName;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sNewFileName)

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(sOldFileName.toUtf8().data(), sNewFileName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryEmptyNewName() {
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(m_sFileName)

	QVERIFY(CFileHelper::CreateBlankFile(m_sFileName, &_auth));
	QVERIFY(QFile::exists(m_sFileName));

	QString sNewFileName;

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(m_sFileName.toUtf8().data(), sNewFileName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryEmptyBothNames() {
	QString sOldFileName, sNewFileName;

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(sOldFileName.toUtf8().data(), sNewFileName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryIncorrectOldName() {
	QString sNewFileName;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sNewFileName)

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(g_sIncorrectValue.toUtf8().data(),	sNewFileName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryIncorrectNewName() {
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(m_sFileName)
	QVERIFY(CFileHelper::CreateBlankFile(m_sFileName, &_auth));
	QVERIFY(QFile::exists(m_sFileName));

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(m_sFileName.toUtf8().data(), g_sIncorrectValue.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryIncorrectBothNames() {
	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(g_sIncorrectValue.toUtf8().data(),	g_sIncorrectValue.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryTooLongOldName() {
	QString sNewFileName;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sNewFileName)

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(GetTooLongString(), sNewFileName.toUtf8().data()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryTooLongNewName() {
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(m_sFileName)
	QVERIFY(CFileHelper::CreateBlankFile(m_sFileName, &_auth));
	QVERIFY(QFile::exists(m_sFileName));

	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(m_sFileName.toUtf8().data(), GetTooLongString()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRenameEntryTooLongBothNames() {
	CALL_CMD(m_pPveControl->DspCmdFsRenameEntry(GetTooLongString(), GetTooLongString()), PVE::DspCmdFsRenameEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRemoveEntryFileEntry() {
	QString sFileName;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sFileName)

	QVERIFY(CFileHelper::CreateBlankFile(sFileName, &_auth));
	QVERIFY(QFile::exists(sFileName));

	CALL_CMD(m_pPveControl->DspCmdFsRemoveEntry(sFileName.toUtf8().data()), PVE::DspCmdFsRemoveEntry)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	QVERIFY(!QFile::exists(sFileName));
}

void TestDspFSCommands::TestDspCmdFsRemoveEntryDirEntry() {
	QString sDirName;
	INITIALIZE_TMP_NAME_FOR_NATIVE_USER(sDirName)

	QVERIFY(CFileHelper::CreateDirectoryPath(sDirName, &_auth));
	QVERIFY(QFile::exists(sDirName));

	CALL_CMD(m_pPveControl->DspCmdFsRemoveEntry(sDirName.toUtf8().data()), PVE::DspCmdFsRemoveEntry)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	QVERIFY(!QFile::exists(sDirName));
}

void TestDspFSCommands::TestDspCmdFsRemoveEntryForNonAccessEntry() {
	m_sDirName = PLATFORM_INDEPEND_TEMP_PATH;
	QVERIFY(!QFile::exists(m_sDirName));
	QVERIFY(QDir().mkdir(m_sDirName));
	QFile _dir(m_sDirName);
	QVERIFY(_dir.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

	CALL_CMD(m_pPveControl->DspCmdFsRemoveEntry(m_sDirName.toUtf8().data()), PVE::DspCmdFsRemoveEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));

	QVERIFY(QFile::exists(m_sDirName));
}

void TestDspFSCommands::TestDspCmdFsRemoveEntryEmptyName() {
	QString sFileName;

	CALL_CMD(m_pPveControl->DspCmdFsRemoveEntry(sFileName.toUtf8().data()), PVE::DspCmdFsRemoveEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRemoveEntryIncorrectName() {
	CALL_CMD(m_pPveControl->DspCmdFsRemoveEntry(g_sIncorrectValue.toUtf8().data()), PVE::DspCmdFsRemoveEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspFSCommands::TestDspCmdFsRemoveEntryTooLongName() {
	CALL_CMD(m_pPveControl->DspCmdFsRemoveEntry(GetTooLongString()), PVE::DspCmdFsRemoveEntry)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}
