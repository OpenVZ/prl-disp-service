/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmExecFunctionalityTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM manipulating SDK API.
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

#ifndef PrlVmExecFunctionalityTest_H
#define PrlVmExecFunctionalityTest_H

#include <QtTest/QtTest>

class PrlVmExecFunctionalityTest : public QObject
{

Q_OBJECT

private slots:
	void testVmLoginInGuestByAdminUserOnNonOwnerAndNonHostAdministrator();
	void testVmLoginInGuestByCurrentActiveSessionOnNonOwnerAndNonHostAdministrator();
	void testVmLoginInGuestByAdminUserOnVmOwner();
	void testVmLoginInGuestByCurrentActiveSessionOnVmOwner();
	void testVmLoginInGuestByAdminUserOnHostAdministrator();
	void testVmLoginInGuestByCurrentActiveSessionOnHostAdministrator();
	void testVmLoginInGuestByCustomCredsOnUserWithInsufficientRights();
	void testVmLoginInGuestByCustomCredsOnUserWithInsufficientRights2();
	void testVmLoginInGuestByCustomCredsOnUserWithSufficientRights();
	void testVmLoginInGuestByCustomCredsOnVmOwner();
	void testVmLoginInGuestByCustomCredsOnHostAdministrator();
	void testVmLoginInGuestOnNullVmHandle();
	void testVmLoginInGuestOnNonVmHandle();
	void testVmLoginInGuestOnNullUserLogin();
	void testVmLoginInGuestOnEmptyUserLogin();
	void testVmGuestLogoutOnNullVmSessionHandle();
	void testVmGuestLogoutOnNonVmSessionHandle();
	void testVmGuestLogoutOnFakeGuestSession();
	void testVmGuestLogoutTryToLogoutOnAlreadyClosedSession();
	void testVmGuestLogoutTryToUseSessionAfterLostConnection();
	void testLoginInGuestOnStoppedVm();
	void testVmGuestLogoutOnStoppedVm();
	void testVmGuestRunProgramOnOrdinalCase();
	void testVmGuestRunProgramOnOrdinalCase2();
	void testVmGuestRunProgramOnJustStdoutRequested();
	void testVmGuestRunProgramOnJustStderrRequested();
	void testVmGuestRunProgramOnNoOutputRequested();
	void testVmGuestRunProgramOnJustProgramStartRequested();
	void testVmGuestRunProgramOnStoppedVm();
	void testVmGuestRunProgramOnInvalidVmGuestHandle();
	void testVmGuestRunProgramOnNonVmGuestHandle();
	void testVmGuestRunProgramOnNullAppString();
	void testVmGuestRunProgramOnEmptyAppString();
	void testVmGuestRunProgramOnNonStringsListHandleForArgs();
	void testVmGuestRunProgramOnNonStringsListHandleForEnvs();
	void testVmGuestRunProgramOnFileDescriptorsUsageIoChannelAbsent();
	void testVmGuestRunProgramCheckPassedArgsAndEnvs();
	void testVmGuestRunProgramOnNonExistsAppName();
	void testVmGuestRunProgramUsingFileDescriptors();
	void testVmGuestGetNetworkSettings();
	void testVmGuestGetNetworkSettingsOnStoppedVm();
	void testVmGuestGetNetworkSettingsOnInvalidVmGuestHandle();
	void testVmGuestGetNetworkSettingsOnNonVmGuestHandle();
	void testVmGuestSetUserPasswd();
	void testVmGuestSetUserPasswdOnStoppedVm();
	void testVmGuestSetUserPasswdOnInvalidVmGuestHandle();
	void testVmGuestSetUserPasswdOnNonVmGuestHandle();
	void testVmGuestSetUserPasswdOnNonKnownUserName();
};

#endif

