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
#pragma once


class ErrorBase {
public:
	ErrorBase() {msg[0] = 0;}
	ErrorBase(const char *s) {strcpy(msg, s);}
	const char *text() const {return msg;}

protected:
	char msg[128];
};


class Win32Err: public ErrorBase {
public:
	Win32Err(int e = 0);
	Win32Err(const char *file, unsigned line);

private:
	const DWORD err;
	const char *const fn;
	const unsigned ln;
};



class File {
public:
	File(LPCWSTR fn, DWORD access, DWORD sharem, DWORD dispos, DWORD flags = FILE_ATTRIBUTE_NORMAL);
	~File() {
		close();
	}

	ULONGLONG setpos(LONGLONG p, DWORD method) const;
	size_t write(const void *data, size_t size, bool allowLess = false);
	size_t read(void *data, size_t size, bool allowLess = false);
	void flush();
	void close() {
		if (h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

	void seteof(size_t size) const;
	ULONGLONG geteof() const;

private:
	File(const File &);
	HANDLE h;
};


class MemBuff {
public:
	MemBuff(size_t size)
		: m(new char[size])
		, s(size) {
	}

	~MemBuff() {
		delete[] m;
	}

	void *ptr() const {return m;}
	size_t size() const {return s;}

private:
	void *const m;
	size_t s;
};


void createFileWithData(LPCWSTR fileName, const char *data, size_t size);
bool checkFileContents(LPCWSTR fileName, const char *data, size_t size);
