/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef _CVzCmd_win_H__
#define _CVzCmd_win_H__

static QString get_error_info();

template <class TCmd> class VzCmd
{
public:
	VzCmd(VZC_COMMAND_TYPE cmd) {
		if (pVZC_NewActivity) {
			pVZC_NewActivity(&jobId);
			pVZC_GenerateLogContext();
			pCmd = (TCmd*)pVZC_AllocCommand(cmd);
		} else {
			pCmd = NULL;
		}
	}
	~VzCmd() {
		if (pCmd)
			pVZC_FreeCommand(pCmd);
	};
	TCmd* operator->() const {
		return pCmd;
	}
	bool operator==(const void * ptr) const { /* for checking with NULL */
		return pCmd == ptr;
	}
	bool operator!=(const void * ptr) const { /* for checking with NULL */
		return pCmd != ptr;
	}
	PRL_RESULT Execute(QString &errMsg = *(QString*)0) {
		PRL_RESULT res;
		if (!pCmd) {
			res = PRL_ERR_API_WASNT_INITIALIZED;
		} else {
			VZC_RESULT r = pVZC_ExecCommand(jobId, pCmd);
			if (VZC_ERROR(r)) {
				QString t = get_error_info();
				if (&errMsg != 0) {
					errMsg = t;
					res = PRL_ERR_VZ_OPERATION_FAILED;
				} else {
					res = PRL_ERR_OPERATION_FAILED;
				}
			} else {
				res = PRL_ERR_SUCCESS;
			}
			VZC_IncrementCommand(&jobId);
		}
		return res;
	};
private:
	VZC_JOBID		jobId;
	TCmd*			pCmd;
};

class VzCfg
{
public:
	VzCfg(unsigned int ctid) {
		pCfg = pVZCfg_ReadConfigVps(ctid);
	}
	VzCfg(unsigned int ctid, PCWSTR path) {
		pCfg = pVZCfg_ReadConfigVps(ctid);
		PVZCFG pOldCfg = pVZCfg_ReadConfigFile(path);
		if (pOldCfg) {
			if (!pVZCfg_CopyConfig(pOldCfg, pCfg)) {
				pVZCfg_FreeConfig(pCfg);
				pCfg = NULL;
			}
			pVZCfg_FreeConfig(pOldCfg);
		} else {
			pVZCfg_FreeConfig(pCfg);
			pCfg = NULL;
		}
	}
	VzCfg(PCWSTR path) {
		pCfg = pVZCfg_ReadConfigFile(path);
	}
	~VzCfg() {
		if (pCfg)
			pVZCfg_FreeConfig(pCfg);
	}
	BOOL GetValueStrListW(const char * key, PWSTR **val, int *count) {
		return pCfg ? pVZCfg_GetValueStrListW(pCfg, key, val, count) : FALSE;
	}
	BOOL GetValueStrW(const char * key, PWSTR  * val) {
		return pCfg ? pVZCfg_GetValueStrW(pCfg, key, val) : FALSE;
	}
	BOOL GetValueDword(const char *key, DWORD  * val) {
		return pCfg ? pVZCfg_GetValueDword(pCfg, key, val) : FALSE;
	}
	BOOL GetValueQword(const char *key, ULONGLONG  *val) {
		return pCfg ? pVZCfg_GetValueQword(pCfg, key, val) : FALSE;
	}
	BOOL SetValueDword(const char *key, DWORD val) {
		return pCfg ? pVZCfg_SetValueDword(pCfg, key, val) : FALSE;
	}
	const WCHAR * GetValue(const char * key) {
		return pCfg ? pVZCfg_GetValue(pCfg, key) : FALSE;
	}
	BOOL SetValue(const char *key, const WCHAR* val) {
		return pCfg ? pVZCfg_SetValue(pCfg, key, val) : FALSE;
	}
	BOOL DeleteValue(const char * key) {
		return pCfg ? pVZCfg_DeleteValue(pCfg, key) : FALSE;
	}
	BOOL WriteConfig(PCWSTR path) {
		if (!pCfg)
			return FALSE;
		PVZCFG pNewCfg = pVZCfg_ReadCreateConfigFile(path);
		if (!pNewCfg)
			return FALSE;
		BOOL success = TRUE;
		if (!pVZCfg_CopyConfig(pCfg, pNewCfg))
			success = FALSE;
		pVZCfg_FreeConfig(pNewCfg);
		return success;
	}
	BOOL DeleteConfig(void) {
		if (!pCfg)
			return FALSE;
		return pVZCfg_DeleteConfig(pCfg);
	}
	bool operator==(const void * ptr) const { /* for checking with NULL */
		return pCfg == ptr;
	}
	bool operator!=(const void * ptr) const { /* for checking with NULL */
		return pCfg != ptr;
	}
private:
	PVZCFG			pCfg;
};

#endif
