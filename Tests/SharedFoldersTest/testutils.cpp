/*
* Copyright (c) 2005-2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
*/
#include <stdio.h>
#include <windows.h>
#include "testutils.h"

namespace
{
	void geterrmsg(int e, char *msg)
	{
		LPWSTR p;
		DWORD u = FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&p, 0, 0);
		if (u == 0)
			p = 0;

		if (p) {
			wsprintfA(msg, int(e) < 0? "%#x: %ws": "%d: %ws", e, p);
			LocalFree(p);
		}
		else {
			wsprintfA(msg, int(e) < 0? "Error %#x": "Error %d", e);
		}
	}
}

Win32Err::Win32Err(int e)
	: err(e? e: GetLastError())
	, fn(0)
	, ln(0)
{
	geterrmsg(err, msg);
}


Win32Err::Win32Err(const char *file, unsigned line)
	: err(GetLastError())
	, fn(file)
	, ln(line)
{
	geterrmsg(err, msg);
}



File::File(LPCWSTR fn, DWORD access, DWORD sharem, DWORD dispos, DWORD flags)
{
	h = CreateFileW(fn,
		access,
		sharem,
		0, dispos, flags,
		0);

	if (h == INVALID_HANDLE_VALUE)
		throw Win32Err();
}


size_t File::write(const void *data, size_t size, bool allowLess)
{
	DWORD w;
	if (!WriteFile(h, data, size, &w, 0))
		throw Win32Err();

	if (!allowLess && (w < size))
		throw ErrorBase("written less");

	return w;
}


size_t File::read(void *data, size_t size, bool allowLess)
{
	DWORD r;
	if (!ReadFile(h, data, size, &r, 0))
		throw Win32Err();

	if (!allowLess && (r < size))
		throw ErrorBase("read less");

	return r;
}


void File::flush()
{
	if (!FlushFileBuffers(h))
		throw Win32Err();
}


void File::seteof(size_t size) const
{
	setpos(size, FILE_BEGIN);
	if (!SetEndOfFile(h))
		throw Win32Err();
}


ULONGLONG File::setpos(LONGLONG p, DWORD method) const
{
	LARGE_INTEGER l, n;
	l.QuadPart = p;
	if (!SetFilePointerEx(h, l, &n, method))
		throw Win32Err();
	return n.QuadPart;
}


ULONGLONG File::geteof() const
{
	LARGE_INTEGER l;
	l.LowPart = GetFileSize(h, (LPDWORD)&l.HighPart);
	return l.QuadPart;
}



void createFileWithData(LPCWSTR fileName, const char *data, size_t size)
{
	File(fileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		CREATE_NEW).write(data, size);
}


bool checkFileContents(LPCWSTR fileName, const char *data, size_t size)
{
	MemBuff mem(size + 1);
	*((char *)mem.ptr() + size) = 0;

	File(fileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		OPEN_EXISTING).read(mem.ptr(), size);

	bool fMatch = memcmp(mem.ptr(), data, size) == 0;
	return fMatch;
}
