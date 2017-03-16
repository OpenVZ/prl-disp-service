/////////////////////////////////////////////////////////////////////////////
///
///	@file Main.cpp
///
///	This file is the part of parallels protocols serialization tests suite.
///	Protocols serialization tests entry point.
///
///	@author sandro
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
#include <QCoreApplication>
#include <QtTest/QtTest>

#include "CProtoSerializerTest.h"
#include "CDispToDispProtoSerializerTest.h"
#include "CVmMigrationProtoTest.h"

#include "../CommonTestsUtils.h"

int main(int argc, char *argv[])
{
	int nRet = 0;
	EXECUTE_TESTS_SUITE(CProtoSerializerTest)
	EXECUTE_TESTS_SUITE(CDispToDispProtoSerializerTest)
	EXECUTE_TESTS_SUITE(CVmMigrationProtoTest)

	return (nRet);
}
