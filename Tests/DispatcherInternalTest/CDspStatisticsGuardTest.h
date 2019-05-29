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
///		CDspStatisticsGuardTest.h
///
/// @author
///		sandro
///
/// @brief
///		Tests fixture class for testing dispatcher host statistics guardian class.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CDspStatisticsGuardTest_H
#define CDspStatisticsGuardTest_H

#include <QtTest/QtTest>

class CDspStatisticsGuardTest : public QObject
{
Q_OBJECT

private slots:
	void testStoreCpusStatWithoutPrevValues();
	void testStoreCpusStatWithPrevValues();
	void testStoreCpusStatWithSeveralCpus();
	void testStoreDisksStat();
	void testStoreRamStat();
	void testStoreSwapStat();
	void testStoreUptimeStat();
	void testStoreProcessesStat();
	void testStoreUsersStat();
	void testStoreIfacesStat();
};

#endif
