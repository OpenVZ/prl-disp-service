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
#include <Libraries/Logging/Logging.h>
#include "testutils.h"



int testOneNote(LPCWSTR fn)
{
	const int len = 10;

	createFileWithData(fn, "abcdefghij", len);

	// Open for read
	File f0(fn,
		FILE_GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		OPEN_EXISTING);


	// Read something to enforce caching
	BYTE buf[2];
	f0.read(buf, sizeof(buf));


	// Open the file one more time, but for read/write
	File f1(fn,
		FILE_GENERIC_READ | FILE_GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		OPEN_EXISTING);


	// Truncating some bytes. Meaning of this step isn't clear, but without it
	// the bug is not reproducible
	f1.seteof(len - 1);

	// Write somewhere
	f1.setpos(-3, FILE_END);

	try {
		// A paging write will come on first SrvOpen, which is opened for read-only
		// On unfixed prl_fs this causes "invalid handle" error
		BYTE buf1[2] = {'A', 'B'};
		f1.write(buf1, sizeof(buf1));
		DeleteFile(fn);
	} catch(...) {
		DeleteFile(fn);
		throw;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	LPCWSTR fileName = L"\\\\psf\\Home\\Desktop\\testfile.txt";

	try {
		testOneNote(fileName);
	} catch (Win32Err &e) {
		WRITE_TRACE(DBG_FATAL, e.text());
		return 1;
	}
	WRITE_TRACE(DBG_INFO, "SharedFoldersTest OK");
	return 0;
}
