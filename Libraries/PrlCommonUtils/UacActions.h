///////////////////////////////////////////////////////////////////////////////
///
/// @file UacActions.h
///
/// Library of the UAC-related operations (elevation, unelevation),
/// applicable for Windows Vista and Windows 7 OSs.
///
/// @author maximk
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __UAC_ACTIONS_H__
#define __UAC_ACTIONS_H__

#include <windows.h>
#include <tchar.h>

/**
 * High-level wrapper above the CreateProcessAsUser to "unelevate" permissions.
 * @param [in] The name of the module to be executed.
 * @param [in] The command line to be executed.
 * @param [in] Process startup info
 * @param [out] An identifier of the created process
 * @return If the function succeeds, the return value is nonzero.
 */
BOOL CreateLimitedProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine,
			  LPSTARTUPINFO lpStartupInfo,
			  LPDWORD lpProcessId );

/**
 * Create a process like with "Run as Administrator" command.
 * @param The name of the module to be executed.
 * @param The command line to be executed.
 * @return If the function succeeds, the return value is nonzero.
 */
BOOL CreateElevatedProcess(LPCTSTR lpApplicationName, LPCTSTR lpCommandLine);

#endif // __UAC_ACTIONS_H__
