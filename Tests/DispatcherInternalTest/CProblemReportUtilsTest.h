///////////////////////////////////////////////////////////////////////////////
///
/// @file CProblemReportUtilsTest.h
///
/// Tests suite for problem report utilities
///
/// @author sandro
/// @owner sergeym
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
///////////////////////////////////////////////////////////////////////////////

#ifndef CProblemReportUtilsTest_H
#define CProblemReportUtilsTest_H

#include <QtTest/QtTest>
#include <QString>

#include "Libraries/PrlCommonUtils/CAuthHelper.h"

class CProblemReportUtilsTest : public QObject
{

Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testSL_10_6_x_x86_32_kernel_case();
	void testSL_10_6_x_x86_64_kernel_case();
	void testSL_10_5_x_case();
	void testMixedCase_10_6_x_x86_32_kernel_panic_newest();
	void testMixedCase_10_6_x_x86_64_kernel_panic_newest();
	void testMixedCase_10_5_x_panic_newest();
	void testSL_10_6_x_x86_64_several_panics();
	void testSL_10_5_x_several_panics();
	void testParseAppVersion_fromMacCrashReport();
	void testParseStubVersion_fromMacCrashReport();
	void testParseDrvVersion_fromMacPanicReport();
	void testParseLowMemoryDumpOfMobileApp_isJettisoned();
	void testExtractPCSIsoVersion();

private:
	QString m_sTargetDirPath;
	CAuthHelper m_Auth;
};

#endif

