/////////////////////////////////////////////////////////////////////////////
///
///	@file CVmMigrationProtoTest.h
///
///	Tests fixture class for testing VM migration protocol serializer.
///
///	@author sandro
/// @owner sergeym
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////

#ifndef CVmMigrationProtoTest_H
#define CVmMigrationProtoTest_H

#include <QtTest/QtTest>

class CVmMigrationProtoTest : public QObject
{

Q_OBJECT

private slots:
	void testCreateVmMigrateCheckPreconditionsCommand();
	void testParseVmMigrateCheckPreconditionsCommand();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnEmptyPackage();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnVersionInfoAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnVmConfigurationAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnHardwareHostInfoAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnTargetVmHomePathAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnReservedFlagsAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidFailedOnSharedFileNameAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidNotFailedOnStorageInfoAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidNotFailedOnPrevVmStateAbsent();
	void testVmMigrateCheckPreconditionsCommandIsValidNotFailedOnTargetVmNameAbsent();
	void testCreateVmMigrateStartCommand();
	void testParseVmMigrateStartCommand();
	void testVmMigrateStartCommandIsValidFailedOnVersionAbsent();
	void testVmMigrateStartCommandIsValidFailedOnEmptyPackage();
	void testVmMigrateStartCommandIsValidFailedOnVmConfigurationAbsent();
	void testVmMigrateStartCommandIsValidFailedOnTargetServerVmHomePathAbsent();
	void testVmMigrateStartCommandIsValidFailedOnReservedFlagsAbsent();
	void testVmMigrateStartCommandIsValidFailedOnPrevVmStateAbsent();
	void testCreateVmMigrateCheckPreconditionsReply();
	void testParseVmMigrateCheckPreconditionsReply();
	void testVmMigrateCheckPreconditionsReplyIsValidFailedOnEmptyPackage();
	void testVmMigrateCheckPreconditionsReplyIsValidFailedOnVersionAbsent();
	void testVmMigrateCheckPreconditionsReplyIsValidFailedOnCheckPreconditionsResultAbsent();
	void testCreateVmMigrateReply();
	void testParseVmMigrateReply();
	void testVmMigrateReplyIsValidFailedOnEmptyPackage();
};

#endif
