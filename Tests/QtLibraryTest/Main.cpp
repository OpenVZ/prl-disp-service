/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///		Main.cpp
///
/// @author
///		sandro
///
/// @brief
///		Dispatcher internal tests entry point.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include <QCoreApplication>
#include <QtTest/QtTest>

#include "Tests/DispatcherTestsUtils.h"

#include "QtCoreTest.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	TestConfig::readTestParameters();

	int nRet = 0;
	EXECUTE_TESTS_SUITE(QtCoreTest);

	return nRet;
}
