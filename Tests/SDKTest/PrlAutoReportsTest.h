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
///		PrlAutoReportsTest.h
///
/// @author
///		sergeyt
///
/// @brief
///		Test dispatcher auto reports functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include <QObject>
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlAutoReportsTest: public QObject
{

   Q_OBJECT
public:

private slots:
	void init();
	void cleanup();

private slots:
	void	testReceiveAutoReport_whenGuiWorking_byCrashAppInSystemScope();
	void	testReceiveAutoReport_whenGuiWorking_byCrashAppInUserScope();
	void	testReceiveAutoReport_onGuiLogin_byCrashAppInUserScope();
	void	testReceiveAutoReport_onGuiLogin_byCrashAppInSystemScope();

private:
	// TODO:
	void testReceiveAutoReport_withoutDuplicates();
	void testReceiveAutoReport_oneVmDumpPerReport();
	void testReceiveAutoReport_allNotVmDumps_InOneReport();
	void testReceiveAutoReport_inSubDirectories_iPhoneAppCrash();
	void testReceiveAutoReport_inSubDirectories_LowMemoryFile();
public:
	enum DumpLocation { dlSystem, dlUser };
	enum DumpKind { dkVmApp, dkPrlApp, dkLowMemory };

private:
	void testReceiveAutoReport_whenAppWorking( DumpLocation, DumpKind );
	void testReceiveAutoReport_onLogin(DumpLocation dumpLocation, DumpKind dumpKind );

	static void copyCrashDump( bool& bRes, DumpLocation dl, DumpKind dk, QStringList& outDumpsList );
	static void cleanupOldReports();

	static bool hasUserActiveSessions( );
	static void hasUserActiveSessions( bool& bRes); //GUI , etc
};
typedef PrlAutoReportsTest::DumpLocation DumpLocation;
typedef PrlAutoReportsTest::DumpKind DumpKind;

