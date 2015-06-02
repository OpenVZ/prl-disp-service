///////////////////////////////////////////////////////////////////////////////
///
/// @file BackupErrors.h
///
/// Backup error codes.
///
/// @author krasnov
/// @owner lenkor
///
/// Copyright (c) 2009-2015 Parallels IP Holdings GmbH
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

#ifndef __BACKUPERRORS_H__
#define __BACKUPERRORS_H__

enum bckp_error
{
	BCKP_OK                = 0,
	BCKP_READ_ERROR        = 1,
	BCKP_EOF               = 2,
	BCKP_WRITE_ERROR       = 3,
	BCKP_DISK_FULL         = 4,
	BCKP_SEEK_ERROR        = 5,
	BCKP_OUT_OF_MEMORY     = 6,
	BCKP_OPEN_ERROR        = 7,
	BCKP_REMOVE_ERROR      = 8,
	BCKP_RENAME_ERROR      = 9,
	BCKP_CREATE_ERROR      = 10,
	BCKP_NOT_READY         = 11,
	BCKP_READONLY          = 12,
	BCKP_CORRUPTED         = 13,
	BCKP_UNSUPPORTED       = 14,
	BCKP_CANCELED          = 15,
	BCKP_DEVICE_BUSY       = 16,
	BCKP_NOT_FOUND         = 17,
	BCKP_NOT_LAST_VOLUME   = 18,
	BCKP_ALREADY_EXIST     = 19,
	BCKP_ACCESS_DENIED     = 20,
	BCKP_NETWORK_ERROR     = 21,
	BCKP_NETWORK_TIMEOUT   = 22,
	BCKP_LOCK_ERROR        = 23,
	BCKP_FORMAT_DISK       = 24,
	BCKP_BACKUP_ERROR      = 25,
	BCKP_RESTORE_ERROR     = 26,
	BCKP_DECRYPT_ERROR     = 27,
	BCKP_UMOUNT_ERROR      = 28,
	BCKP_NO_CONTEXT        = 30,
	BCKP_NO_CUR_DISK       = 31,
	BCKP_NO_BUILDER        = 32,
	BCKP_MINMAX            = 33,
	BCKP_PROPERTY          = 34,
	BCKP_NO_CUR_PART       = 35,
	BCKP_RESIZE            = 36,
	BCKP_PARAM             = 37,
	BCKP_NO_DISK           = 38,
	BCKP_NO_ROOM           = 39,
	BCKP_LABEL             = 40,
	BCKP_STACK             = 41,
	BCKP_CHECK             = 42,
	BCKP_INTERNAL          = 43,
	BCKP_TYPE              = 44,
	BCKP_NOT_FILLED        = 45,
	BCKP_NO_CUR_COMP       = 46,
	BCKP_CONSISTENCY       = 47,
	BCKP_USER_PROPERTY     = 48,
	BCKP_WIPE              = 49,
	BCKP_STRID_MORE        = 50,
	BCKP_STRID_ROUGH       = 51,
	BCKP_STRID_MORE_ROUGH  = 52,
	BCKP_NON_TMP_OP        = 53,
	BCKP_SYSTEM_RESTORE    = 54,
	BCKP_OP_UNKNOWN        = 55,
	BCKP_OP_NO_MIN_SIZE    = 56,
	BCKP_OP_LOW_SIZE       = 57,
	BCKP_OP_ERR_SPARSE     = 58,
	BCKP_OP_ERR_SYM_LINK   = 59,
	BCKP_OP_ERR_CRYPTED    = 60,
	BCKP_OP_PREPARE_FAIL   = 61,
	BCKP_LDM_NOT_SUPPORTED = 62,
	BCKP_LDM_ERROR         = 63,
	BCKP_UNKNOWN_ERROR     = 64,
	BCKP_INVALID_ARG       = 65,
};
#endif // __BACKUPERRORS_H__
