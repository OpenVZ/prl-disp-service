/*
 * Copyright (c) 2005-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#pragma once

#include <windows.h>
#include <prlcommon/Std/noncopyable.h>

class CDllFile : public noncopyable
{
public:
	CDllFile(const char *const dll):
		m_h(LoadLibraryA(dll)) {}
	~CDllFile()
	{
		if(0 != m_h) FreeLibrary(m_h);
	}
	bool ok() const { return (0 != m_h); }
	HMODULE handle() const { return m_h; }
	UINT_PTR ptr() const { return (UINT_PTR)m_h; }

private:
	HMODULE m_h;
};

template <class F>
class CDllFileProc : public CDllFile
{
public:
	CDllFileProc(const char *const dll, const char *const proc):
		CDllFile(dll)
	{
		m_f = (CDllFile::ok()) ? (F)GetProcAddress(handle(), proc): 0;
	}
	~CDllFileProc() {}
	F operator *() const { return m_f; }
	F f() const { return m_f; }
	bool ok() const { return (0 != m_f); }

private:
	F m_f;
};
