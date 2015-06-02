/////////////////////////////////////////////////////////////////////////////
///
///	@file RaceOnSdkDeinitTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests suite for checking PrlApi_Init() PrlApi_Deinit() multiple calls.
///
///	@author sandro
/// @owner  sergeym
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

#ifndef RaceOnSdkDeinitTest_H
#define RaceOnSdkDeinitTest_H

#include <QtTest/QtTest>

class RaceOnSdkDeinitTest : public QObject
{
Q_OBJECT

private slots:
	void test();
};

#endif

