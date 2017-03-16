///////////////////////////////////////////////////////////////////////////////
///
/// @file ServiceControl.cpp
/// @author sdmitry
///
/// Implementation of simple class for controlling Win32-services
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

/**
 * Based on the work "NT Service App Wizard"
 * published at http://www.codeguru.com/cpp/w-p/system/ntservices/article.php/c2847/
 */

/////////////////////////////////////////////////////////////////////////////
//// Copyright (C) 1997 by Joerg Koenig
//// All rights reserved
////
//// Distribute freely, except: don't remove my name from the source or
//// documentation (don't take credit for my work), mark your changes (don't
//// get me blamed for your possible bugs), don't alter or remove this
//// notice.
//// No warrantee of any kind, express or implied, is included with this
//// software; use at your own risk, responsibility for damages (if any) to
//// anyone resulting from the use of this software rests entirely with the
//// user.
////
//// Send bug reports, bug fixes, enhancements, requests, flames, etc., and
//// I'll try to keep a version up to date.  I can be reached as follows:
////    J.Koenig@adg.de                 (company site)
////    Joerg.Koenig@rhein-neckar.de    (private site)
///////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
#define _UNICODE
#endif

#include "ServiceControl.h"
#include <tchar.h>


CServiceControl::CServiceControl()
{
	m_hScm = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
}


CServiceControl::~CServiceControl()
{
	if( NULL != m_hScm )
	{
		::CloseServiceHandle(m_hScm);
		m_hScm = NULL;
	}
}

bool CServiceControl::operator !()
{
	return (m_hScm == NULL);
}

BOOL CServiceControl::InstallService( LPCTSTR ServiceName, LPCTSTR ServiceFile, LPCTSTR ServiceDesc, DWORD ServiceType, DWORD StartType )
{
	SC_HANDLE Service = ::CreateService(
		m_hScm,
		ServiceName,             // service name
		ServiceName,             // service display name
		SERVICE_ALL_ACCESS,
		ServiceType,
		StartType,
		SERVICE_ERROR_NORMAL,
		ServiceFile,            // path to the executable
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
		);

	DWORD err = ::GetLastError();
	if ( Service == NULL ) // neudacha
	{
		if ( err == ERROR_SERVICE_EXISTS )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}

	}

	// setup service description
	if( NULL != ServiceDesc)
	{
		SERVICE_DESCRIPTION sd;
		sd.lpDescription = (LPTSTR)ServiceDesc;
		::ChangeServiceConfig2(Service, SERVICE_CONFIG_DESCRIPTION, (LPVOID)&sd);
	}

	::CloseServiceHandle( Service );

	return TRUE;
}


BOOL CServiceControl::RemoveService( LPCTSTR ServiceName )
{
	SC_HANDLE Service = ::OpenService( m_hScm, ServiceName, SERVICE_ALL_ACCESS );
	if ( Service == NULL )
	{
		return FALSE;
	}

	BOOL ret = ::DeleteService (Service);

	DWORD err = ::GetLastError();
	if( err == ERROR_SERVICE_MARKED_FOR_DELETE )
	{
		err = 0;
	}

	::CloseServiceHandle( Service );
	::SetLastError(err);

	return ret;
}


BOOL CServiceControl::StartService( LPCTSTR ServiceName )
{
	SC_HANDLE Service = ::OpenService( m_hScm, ServiceName, SERVICE_ALL_ACCESS );
	if ( Service == NULL )
	{
		return FALSE;
	}

	BOOL ret = ::StartService( Service, 0, NULL );

	if ( !ret )
	{
		DWORD err = ::GetLastError();
		if ( err == ERROR_SERVICE_ALREADY_RUNNING )
		{
			ret = TRUE; // OK, drajver uzhe rabotaet!
		}
	}

	DWORD err = ::GetLastError();
	::CloseServiceHandle( Service );
	::SetLastError(err);

	return ret;
}


BOOL CServiceControl::ControlService(LPCTSTR ServiceName, DWORD dwControlCode)
{
	SC_HANDLE Service = ::OpenService(m_hScm, ServiceName, SERVICE_ALL_ACCESS );
	if (Service == NULL)
	{
		return FALSE;
	}

	SERVICE_STATUS serviceStatus;

	BOOL ret = ::ControlService(Service, dwControlCode, &serviceStatus);

	DWORD err = ::GetLastError();
	::CloseServiceHandle( Service );
	::SetLastError(err);

	return ret;

}


BOOL CServiceControl::StopService( LPCTSTR ServiceName )
{
	return ControlService(ServiceName, SERVICE_CONTROL_STOP);
}


BOOL CServiceControl::QueryServiceStatus(LPCTSTR ServiceName, SERVICE_STATUS *pServiceStatus)
{
	SC_HANDLE Service = ::OpenService(m_hScm, ServiceName, SERVICE_ALL_ACCESS );
	if (Service == NULL)
	{
		return FALSE;
	}

	BOOL bRes = ::QueryServiceStatus(Service, pServiceStatus);

	DWORD err = ::GetLastError();
	::CloseServiceHandle( Service );
	::SetLastError(err);

	return bRes;
}

static LPCTSTR gszAppRegKey = TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");

void CServiceControl::RegisterApplicationLog( LPCTSTR ServiceName, LPCTSTR lpszFileName )
{
	DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
	TCHAR szKey[256];
	_tcscpy_s(szKey, _countof(szKey), gszAppRegKey);
	_tcscat_s(szKey, _countof(szKey), ServiceName);

	HKEY hKey = 0;
	LONG lRet = ERROR_SUCCESS;

	// Create a key for that application and insert values for
	// "EventMessageFile" and "TypesSupported"
	if( ::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) == ERROR_SUCCESS ) {
		lRet =	::RegSetValueEx(
			hKey,						// handle of key to set value for
			TEXT("EventMessageFile"),	// address of value to set
			0,							// reserved
			REG_EXPAND_SZ,				// flag for value type
			(CONST BYTE*)lpszFileName,	// address of value data
			sizeof(TCHAR)*(_tcslen(lpszFileName) + 1)	// size of value data
			);

		// Set the supported types flags.
		lRet =	::RegSetValueEx(
			hKey,					// handle of key to set value for
			TEXT("TypesSupported"),	// address of value to set
			0,						// reserved
			REG_DWORD,				// flag for value type
			(CONST BYTE*)&dwTypes,	// address of value data
			sizeof(DWORD)			// size of value data
			);
		::RegCloseKey(hKey);
	}

	// Add the service to the "Sources" value

	lRet =	::RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,	// handle of open key
		gszAppRegKey,		// address of name of subkey to open
		0,					// reserved
		KEY_ALL_ACCESS,		// security access mask
		&hKey				// address of handle of open key
		);
	if( lRet == ERROR_SUCCESS ) {
		DWORD dwSize;

		// retrieve the size of the needed value
		lRet =	::RegQueryValueEx(
			hKey,			// handle of key to query
			TEXT("Sources"),// address of name of value to query
			0,				// reserved
			0,				// address of buffer for value type
			0,				// address of data buffer
			&dwSize			// address of data buffer size
			);

		if( lRet == ERROR_SUCCESS ) {
			DWORD dwType;

			DWORD  dwAllocSize = dwSize + sizeof(TCHAR)*(_tcslen(ServiceName)+1)
						+ 2*sizeof(TCHAR); // RegQueryValueEx may return non null-terminated string

			LPBYTE Buffer = LPBYTE(::GlobalAlloc(GPTR, dwAllocSize)); // note: allocated mem is zeroed

			lRet =	::RegQueryValueEx(
				hKey,			// handle of key to query
				TEXT("Sources"),// address of name of value to query
				0,				// reserved
				&dwType,		// address of buffer for value type
				Buffer,			// address of data buffer
				&dwSize			// address of data buffer size
				);
			if( lRet == ERROR_SUCCESS ) {

				register LPTSTR p = LPTSTR(Buffer);

				// check that value is terminated with 2 zeroes. Also need prevent growth of key-size.
				// note: allocated buf was zeroed
				int tchar_count = dwSize / sizeof(TCHAR);
				DWORD dwNewSize = dwSize + sizeof(TCHAR)*(_tcslen(ServiceName)+1);
				if (tchar_count < 2 || p[tchar_count - 1] != 0 || p[tchar_count - 2] != 0)
					dwNewSize += 2 * sizeof(TCHAR);

				// check whether this service is already a known source
				for(; *p; p += _tcslen(p)+1 ) {
					if( _tcscmp(p, ServiceName) == 0 )
						break;
				}
				if( ! * p ) {
					// We're standing at the end of the stringarray
					// and the service does still not exist in the "Sources".
					// Now insert it at this point.
					// Note that we have already enough memory allocated
					// (see GlobalAlloc() above). We also don't need to append
					// an additional '\0'. This is done in GlobalAlloc() above
					// too.
					_tcsncpy(p, ServiceName, dwNewSize/sizeof(TCHAR) - (p - (LPTSTR)Buffer));

					// OK - now store the modified value back into the
					// registry.
					lRet =	::RegSetValueEx(
						hKey,			// handle of key to set value for
						TEXT("Sources"),// address of value to set
						0,				// reserved
						dwType,			// flag for value type
						Buffer,			// address of value data
						dwNewSize		// size of value data
						);
				}
			}

			::GlobalFree(HGLOBAL(Buffer));
		}

		::RegCloseKey(hKey);
	}
}


void CServiceControl::DeregisterApplicationLog( LPCTSTR ServiceName )
{
	TCHAR szKey[256];
	_tcscpy_s(szKey, _countof(szKey), gszAppRegKey);
	_tcscat_s(szKey, _countof(szKey), ServiceName);
	HKEY hKey = 0;
	LONG lRet = ERROR_SUCCESS;

	lRet = ::RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);

	// now we have to delete the application from the "Sources" value too.
	lRet =	::RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,	// handle of open key
		gszAppRegKey,		// address of name of subkey to open
		0,					// reserved
		KEY_ALL_ACCESS,		// security access mask
		&hKey				// address of handle of open key
		);

	if( lRet == ERROR_SUCCESS )
	{
		DWORD dwSize;

		// retrieve the size of the needed value
		lRet =	::RegQueryValueEx(
			hKey,			// handle of key to query
			TEXT("Sources"),// address of name of value to query
			0,				// reserved
			0,				// address of buffer for value type
			0,				// address of data buffer
			&dwSize			// address of data buffer size
			);

		if( lRet == ERROR_SUCCESS ) {
			DWORD dwType;

			DWORD dwAllocSize = dwSize + 2 * sizeof(TCHAR); // need to ensure that value is NULL-terminated
			LPBYTE Buffer = LPBYTE(::GlobalAlloc(GPTR, dwAllocSize));
			LPBYTE NewBuffer = LPBYTE(::GlobalAlloc(GPTR, dwAllocSize));

			lRet =	::RegQueryValueEx(
				hKey,			// handle of key to query
				TEXT("Sources"),// address of name of value to query
				0,				// reserved
				&dwType,		// address of buffer for value type
				Buffer,			// address of data buffer
				&dwSize			// address of data buffer size
				);
			if( lRet == ERROR_SUCCESS ) {

				// check whether this service is already a known source
				register LPTSTR p = LPTSTR(Buffer);
				register LPTSTR pNew = LPTSTR(NewBuffer);

				// check that value is terminated with 2 zeroes.
				int tchar_count = dwSize / sizeof(TCHAR);
				if (tchar_count < 2 || p[tchar_count - 1] != 0 || p[tchar_count - 2] != 0)
					dwSize += 2 * sizeof(TCHAR);

				BOOL bNeedSave = FALSE;	// assume the value is already correct
				for(; *p; p += _tcslen(p)+1) {
					// except ourself: copy the source string into the destination
					if( _tcscmp(p, ServiceName) != 0 ) {
						_tcsncpy(pNew, p, dwSize/sizeof(TCHAR) - (pNew - LPTSTR(NewBuffer)));
						pNew += _tcslen(pNew)+1;
					} else {
						bNeedSave = TRUE;		// *this* application found
						dwSize -= sizeof(TCHAR)*(_tcslen(p)+1);	// new size of value
					}
				}
				if( bNeedSave ) {
					// OK - now store the modified value back into the
					// registry.
					lRet =	::RegSetValueEx(
						hKey,			// handle of key to set value for
						TEXT("Sources"),// address of value to set
						0,				// reserved
						dwType,			// flag for value type
						NewBuffer,		// address of value data
						dwSize			// size of value data
						);
				}
			}

			::GlobalFree(HGLOBAL(Buffer));
			::GlobalFree(HGLOBAL(NewBuffer));
		}

		::RegCloseKey(hKey);
	}
}
