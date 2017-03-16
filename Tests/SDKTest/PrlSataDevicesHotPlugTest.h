/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlSataDevicesHotPlugTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing SATA devices hot plug feature.
///
///	@author sandro
///
/// Copyright (c) 1999-2017, Parallels International GmbH
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

#ifndef PrlSataDevicesHotPlugTest_H
#define PrlSataDevicesHotPlugTest_H

#include <QtTest/QtTest>
#include <QString>

class PrlSataDevicesHotPlugTest : public QObject
{

Q_OBJECT

private slots:
	void cleanup();
	void testAddSataHddOnRunningVm();
	void testAddSeveralSataHddsOnRunningVm();
	void testAddSeveralSataHddsThenRemoveThemOnRunningVm();
	void testAddAndRemoveSataHddsSimultaneouslyOnRunningVm();
	void testDisconnectSataHddOnRunningVm();
	void testDisconnectSataHddOnRunningVmViaEditConfig();
	void testTryDisconnectIdeHddOnRunningVm();
	void testTryDisconnectScsiHddOnRunningVm();
	void testTryDisconnectIdeHddOnRunningVmViaEditConfig();
	void testTryDisconnectScsiHddOnRunningVmViaEditConfig();
	void testAddSataHddOnStoppedVm();
	void testAddSeveralSataHddsOnStoppedVm();
	void testDisconnectSataHddOnStoppedVmViaEditConfig();
	void testTryDisconnectIdeHddOnStoppedVmViaEditConfig();
	void testTryDisconnectScsiHddOnStoppedVmViaEditConfig();
	void testAddSeveralSataHddsThenRemoveThemOnStoppedVm();
	void testAddAndRemoveSataHddsSimultaneouslyOnStoppedVm();
	void testRemoveSataHddOnRunningVm();
	void testTryToRemoveNotDisconnectedSataHddOnRunningVm();
	void testRemoveSataHddOnStoppedVm();
	void testTryToRemoveNotDisconnectedSataHddOnStoppedVm();
	void testChangeSystemAndFriendlyNamesForCdromOnRunningVm();
	void testDisconnectSerialPortOnRunningVm();

private:
	QString m_sImageFilePath;
};

#endif

