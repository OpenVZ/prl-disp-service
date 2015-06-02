///////////////////////////////////////////////////////////////////////////////
///
/// @file main.c
///
/// @author ilyab
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
///////////////////////////////////////////////////////////////////////////////


#include "CLoginHelper.h"


#include "stdafx.h"
#include <QString>



int Usage(_TCHAR *strApp)
{
	_TCHAR *s = _tcsrchr(strApp,_T('\\'));
	if ( s == NULL ){
		s = strApp;
	}else{
		s++;
	}
	_tprintf(TEXT("Usage: %s enable/disable\n"),s);
	return 0;
}

void print_status()
{
	_tprintf(TEXT("AUTOLOGIN status: [%s]\n"), CLoginHelper::IsAutologonEnabled() ? _T("ENABLED") : _T("DISABLED"));
}


void test_enable_disable(QString userName, QString password)
{
	print_status();

	// Wizard
	_tprintf(TEXT("EnableAutoLogin(userName, password)\n"));
	getchar();
	CLoginHelper::EnableAutoLogin(userName, password);
	print_status();

	// Guest tools - begin install
	_tprintf(TEXT("EnableAutoLogin(NULL, NULL)\n"));
	getchar();
	CLoginHelper::EnableAutoLogin(NULL, NULL);
	print_status();

	// Guest tools - end install
	_tprintf(TEXT("RestoreAutoLogin()\n"));
	getchar();
	CLoginHelper::RestoreAutoLogin();
	print_status();
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		_tprintf(TEXT("Usage: autlogin_test.exe user password"));
		return -1;
	}
	test_enable_disable(UTF16_2QSTR(argv[1]), UTF16_2QSTR(argv[2]));

	return 0;
}

