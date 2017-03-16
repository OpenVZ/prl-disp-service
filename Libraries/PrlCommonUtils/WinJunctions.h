///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2004-2017, Parallels International GmbH
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

#pragma once

// This struct is part of DDK. To us don't need anything else from DDK and
// therefore just duplicate it here.
// For details look at http://msdn.microsoft.com/en-us/library/ms791514.aspx

#pragma pack(push, 1)
typedef struct _REPARSE_DATA_BUFFER {
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	};
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#pragma pack(pop)


extern inline bool createJunctionPoint(LPCWSTR mountDir, LPCWSTR destDir)
{
	char buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
	REPARSE_DATA_BUFFER &rdb = *reinterpret_cast<PREPARSE_DATA_BUFFER>(buf);

	// The documentation somewhere states "if this string is NULL-terminated", making us to think
	// that we may place strings without a terminating null character.
	// In fact strings still must have an extra character. Looks like it isn't required to be NULL,
	// but it must be accounted in the offsets/sizes. So, we reserve extra characters
	// for two strings: one is for SubstituteName and another is for PrintName.
	const size_t cchMax = (sizeof(buf) -
		FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer)) / sizeof(*destDir) - 2;
	const size_t cchDest = wcsnlen(destDir, cchMax + 1);
	if (cchDest > cchMax) {
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return false;
	}

	HANDLE hDir = CreateFileW(mountDir, GENERIC_READ | GENERIC_WRITE, 0,
		0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, 0);
	if (INVALID_HANDLE_VALUE == hDir)
		return false;

	const size_t cbDest = (cchDest + 1) * sizeof(*destDir);
	rdb.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
	rdb.ReparseDataLength = cbDest + sizeof(rdb.MountPointReparseBuffer);
	rdb.Reserved = 0;

	// Setting up SubstituteName
	rdb.MountPointReparseBuffer.SubstituteNameOffset = 0;
	rdb.MountPointReparseBuffer.SubstituteNameLength = cbDest - sizeof(*destDir);
	memcpy(rdb.MountPointReparseBuffer.PathBuffer, destDir, cbDest);
	// Setting up PrintName
	rdb.MountPointReparseBuffer.PrintNameOffset = static_cast<USHORT>(cbDest);
	rdb.MountPointReparseBuffer.PrintNameLength = 0;
	rdb.MountPointReparseBuffer.PathBuffer[cbDest / sizeof(wchar_t)] = 0;

	DWORD dwBytes;
	const BOOL bOK = DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, (LPVOID)&rdb,
		rdb.ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE, NULL, 0, &dwBytes, 0);
	CloseHandle(hDir);
	return !!bOK;
}


extern inline bool deleteJunctionPoint(LPCWSTR mountDir)
{
	HANDLE hDir = CreateFileW(mountDir, GENERIC_READ | GENERIC_WRITE, 0, 0,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, 0);
	if (INVALID_HANDLE_VALUE == hDir)
		return false;

	REPARSE_GUID_DATA_BUFFER rgdb = {0};
	rgdb.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
	DWORD dwBytes;
	const BOOL bOK = DeviceIoControl(hDir, FSCTL_DELETE_REPARSE_POINT, &rgdb,
		REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0, &dwBytes, 0);
	CloseHandle(hDir);
	return !!bOK;
}
