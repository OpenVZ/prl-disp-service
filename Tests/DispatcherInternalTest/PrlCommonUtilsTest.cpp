/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlCommonUtilsTest.cpp
///
///	Tests fixture class for testing common utilities library.
///
///	@author sandro
/// @domain sergeym
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
/////////////////////////////////////////////////////////////////////////////


#include "PrlCommonUtilsTest.h"
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>

using namespace Parallels;

void PrlCommonUtilsTest::testGenerateFilenameTargetPresentsAtEntries()
{
	QStringList _dir_entries;
	_dir_entries.append("file.txt");
	_dir_entries.append("file2.txt");
	_dir_entries.append("file3.txt");
	QCOMPARE(GenerateFilename(_dir_entries, "file", ".txt"), QString("file1.txt"));
}

void PrlCommonUtilsTest::testGenerateFilenameOnEmptyEntriesList()
{
	QCOMPARE(GenerateFilename(QStringList(), "file", ".txt"), QString("file.txt"));
}

void PrlCommonUtilsTest::testGenerateFilenameTargetNotPresentsAtEntries()
{
	QStringList _dir_entries;
	_dir_entries.append("file1.txt");
	_dir_entries.append("file2.txt");
	_dir_entries.append("file3.txt");
	QCOMPARE(GenerateFilename(_dir_entries, "file", ".txt"), QString("file.txt"));
}

void PrlCommonUtilsTest::testGenerateFilenameOnEmptySuffix()
{
	QStringList _dir_entries;
	_dir_entries.append("file1");
	_dir_entries.append("file");
	_dir_entries.append("file3");
	QCOMPARE(GenerateFilename(_dir_entries, "file"), QString("file2"));
}

void PrlCommonUtilsTest::testGenerateFilenameOnEmptyPrefix()
{
	QStringList _dir_entries;
	_dir_entries.append("tmpfile.txt");
	_dir_entries.append("tmpfile1.txt");
	_dir_entries.append("tmpfile2.txt");
	_dir_entries.append("tmpfile3.txt");
	_dir_entries.append("tmpfile5.txt");
	QCOMPARE(GenerateFilename(_dir_entries, "", ".txt"), QString("tmpfile4.txt"));
}

void PrlCommonUtilsTest::testGenerateFilenameOnEmptyPrefixAndSuffix()
{
	QStringList _dir_entries;
	_dir_entries.append("tmpfile");
	_dir_entries.append("tmpfile1");
	_dir_entries.append("tmpfile2");
	_dir_entries.append("tmpfile3");
	_dir_entries.append("tmpfile4");
	_dir_entries.append("tmpfile5");
	_dir_entries.append("tmpfile6");
	_dir_entries.append("tmpfile7");
	_dir_entries.append("tmpfile8");
	_dir_entries.append("tmpfile9");
	_dir_entries.append("tmpfile10");
	_dir_entries.append("tmpfile11");
	QCOMPARE(GenerateFilename(_dir_entries), QString("tmpfile12"));
}

void PrlCommonUtilsTest::testGenerateFilenameOnAllParamsEmpty()
{
	QCOMPARE(GenerateFilename(QStringList()), QString("tmpfile"));
}

void PrlCommonUtilsTest::testGenerateFilenameOnIndexDelimiterSpecified()
{
	QStringList _dir_entries;
	_dir_entries.append("Win2003");
	QCOMPARE(GenerateFilename(_dir_entries, "Win2003", "", " "), QString("Win2003 1"));
}
