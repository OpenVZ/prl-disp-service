///////////////////////////////////////////////////////////////////////////////
///
/// @file UacActions.cpp
///
/// Library of the UAC-related operations (elevation, unelevation),
/// applicable for Windows Vista and Windows 7 OSs.
///
/// @author maximk
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "UacActions.h"

/* Required libraries:
 *	- Advapi32.lib
 *	- shell32.lib
 */

#include <shlobj.h>

/**
 * UAC "unalevation" idea is based on the MSDN article description:
 * "Windows Vista for Developers - User Account Control"
 * http://weblogs.asp.net/kennykerr/archive/2006/09/29/Windows-Vista-for-Developers-_1320_-Part-4-_1320_-User-Account-Control.aspx
 */

//*****************************************************************************
//
//      Class:          WellKnownSid
//      Author:         Kenny Kerr
//      Description:    Utility class to easily form "well-known" SIDs.
//
//*****************************************************************************

struct WellKnownSid : SID
{
public:
    WellKnownSid(
		BYTE authority,
		DWORD firstSubAuthority,
		DWORD secondSubAuthority = 0) {

        ::ZeroMemory(this, sizeof (WellKnownSid));

        Revision = SID_REVISION;
        SubAuthorityCount = (0 != secondSubAuthority ? 2 : 1);
        IdentifierAuthority.Value[5] = authority;
        SubAuthority[0] = firstSubAuthority;
        SubAuthority[1] = secondSubAuthority;
    }

    BYTE GetAuthority() const {
        return IdentifierAuthority.Value[5];
    }

    DWORD GetFirstSubAuthority() const {
        return SubAuthority[0];
    }

    DWORD GetSecondSubAuthority() const {
        return SubAuthority[1];
    }

    static WellKnownSid Everyone() {
		return WellKnownSid(
			WorldAuthority,
			SECURITY_WORLD_RID);
    }

    static WellKnownSid Administrators() {
        return WellKnownSid(
			NtAuthority,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS);
    }

    static WellKnownSid Users() {
        return WellKnownSid(
			NtAuthority,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_USERS);
    }

    enum WellKnownAuthorities {
		NullAuthority,
		WorldAuthority,
		LocalAuthority,
		CreatorAuthority,
		NonUniqueAuthority,
		NtAuthority,
		MandatoryLabelAuthority = 16
    };

private:
    DWORD m_secondSubAuthority;
};


/**
 * High-level wrapper above the CreateProcessAsUser to "unelevate" permissions.
 * @param The name of the module to be executed.
 * @param The command line to be executed.
 * @param Process startup info
 * @param An identifier of the created process
 * @return If the function succeeds, the return value is nonzero.
 */
BOOL CreateLimitedProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine,
			  LPSTARTUPINFO lpStartupInfo,
			  LPDWORD pProcessId) {

	BOOL bResult = FALSE;

	WellKnownSid administratorsSid = WellKnownSid::Administrators();

	HANDLE hProcessToken = INVALID_HANDLE_VALUE;
	HANDLE hRestrictedToken = INVALID_HANDLE_VALUE;

	PROCESS_INFORMATION processInfo;

	// Creating process token
	if (!::OpenProcessToken(::GetCurrentProcess(),
		TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY,
		&hProcessToken)) {
		goto _exit;
	}

	SID_AND_ATTRIBUTES sidsToDisable[] = {
	    &administratorsSid, 0
	};

	// Creating restricted token, based on the process token
	if (!::CreateRestrictedToken(hProcessToken,
		0, // flags
		1, // number of entries in the SIDs array
		sidsToDisable, // SIDs array
		0, // number of privileges to delete
		0, // no privileges to delete
		0, // number of SIDs to restrict,
		0, // no SIDs to restrict,
		&hRestrictedToken)) {
		goto _exit;
	}

	if (!::CreateProcessAsUser(hRestrictedToken,
		lpApplicationName,
		lpCommandLine,
		0, // process attributes
		0, // thread attributes
		FALSE, // don't inherit handles
		0, // flags
		0, // inherit environment
		0, // inherit current directory
		lpStartupInfo,
		&processInfo)) {
		goto _exit;
	}

	if ( pProcessId )
		(*pProcessId) = processInfo.dwProcessId;

	// Don't forget to clean PROCESS_INFORMATION members
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	bResult = TRUE;

_exit:
	if (hProcessToken != INVALID_HANDLE_VALUE) {
		CloseHandle(hProcessToken);
		hProcessToken = INVALID_HANDLE_VALUE;
	}

	if (hRestrictedToken != INVALID_HANDLE_VALUE) {
		CloseHandle(hRestrictedToken);
		hRestrictedToken = INVALID_HANDLE_VALUE;
	}

	return bResult;
}

/**
 * Create a process like with "Run as Administrator" command.
 * @param The name of the module to be executed.
 * @param The command line to be executed.
 * @return If the function succeeds, the return value is nonzero.
 */
BOOL CreateElevatedProcess(LPCTSTR lpApplicationName, LPCTSTR lpCommandLine) {

	// If you want to create an elevated process regardless of what application
	// information is available then you can specify the little-known "runas" verb
	// with ShellExecute. This has the effect of requesting elevation regardless
	// of what an application's manifest and compatibility information might prescribe.

	HINSTANCE hInstance = ::ShellExecuteW(0, // owner window
		L"runas", // Trick for requesting elevation
		lpApplicationName,
		lpCommandLine,
		0, // directory
		SW_SHOWNORMAL);

	// If the function succeeds, it returns a value greater than 32
	if ((int)hInstance <= 32) {
		return FALSE;
	}

	return TRUE;
}
