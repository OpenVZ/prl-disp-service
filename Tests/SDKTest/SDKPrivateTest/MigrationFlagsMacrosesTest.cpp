/////////////////////////////////////////////////////////////////////////////
///
///	@file MigrationFlagsMacrosesTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing VM migration flags processing macroses.
///
///	@author sandro
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
/////////////////////////////////////////////////////////////////////////////

#include "MigrationFlagsMacrosesTest.h"
#include <prlsdk/PrlTypes.h>
#include <prlsdk/PrlEnums.h>

#define TEST_EXTRACTING_MIGRATION_FLAG_VALUE(migration_flag_value, extracting_value_macro, expecting_value)\
	{\
		PRL_UINT32 nMigrationFlags = 0 | migration_flag_value;\
		QCOMPARE(quint32(extracting_value_macro(nMigrationFlags)), quint32(expecting_value));\
	}

void MigrationFlagsMacrosesTest::testExtractVmMigrationSecurityLevelValues()
{
	TEST_EXTRACTING_MIGRATION_FLAG_VALUE(PVMSL_LOW_SECURITY, PVM_GET_SECURITY_LEVEL, PSL_LOW_SECURITY)
	TEST_EXTRACTING_MIGRATION_FLAG_VALUE(PVMSL_NORMAL_SECURITY, PVM_GET_SECURITY_LEVEL, PSL_NORMAL_SECURITY)
	TEST_EXTRACTING_MIGRATION_FLAG_VALUE(PVMSL_HIGH_SECURITY, PVM_GET_SECURITY_LEVEL, PSL_HIGH_SECURITY)
}

void MigrationFlagsMacrosesTest::testExtractValuesFromMixedFlagsMask()
{
	PRL_UINT32 nMigrationFlags = PVMT_WARM_MIGRATION | PVMSL_HIGH_SECURITY;
	QCOMPARE(quint32(PVM_GET_SECURITY_LEVEL(nMigrationFlags)), quint32(PSL_HIGH_SECURITY));
}
