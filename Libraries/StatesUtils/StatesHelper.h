//////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2007-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// MODULE: StatesHelper.h
///
/// DESCRIPTION:
///		A couple of crossplatform helper methods working with Saved States files
///
/////////////////////////////////////////////////////////////////////////////

#if !defined(STATES_HELPER_H)
#define STATES_HELPER_H

#include <QString>
#include <prlsdk/PrlErrors.h>
#include <prlcommon/Interfaces/VirtuozzoTypes.h>
class CVmConfiguration;

class CStatesHelper
{
public:

	CStatesHelper(const QString &base_file);

	/**
	* Generate full file name for suspended VM save file
	*/
	QString getSavFileName();

	/**
	* Returns true if the save file exists
	*/
	bool savFileExists();

	/**
	* Remove state files from disk
	*/
	bool dropStateFiles();

	/**
	* set suspend flag for all disks in config
	*/
	static PRL_RESULT SetSuspendParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int uiSuspendState);

	/**
	* get suspend flag for all disks in config
	*/
	static void GetSuspendParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int & uiSuspendState);

	/**
	* set suspend flag for all disks in config
	*/
	static PRL_RESULT SetChangeParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int uiChangeState);

	/**
	* get suspend flag for all disks in config
	*/
	static void GetChangeParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int & uiChangeState);

	/**
	* get disk suspend state parameter
	*/
	static PRL_RESULT GetDiskSuspendState(const QString & strDisk,unsigned int & uiSuspendState);

	/**
	* get disk suspend state parameter
	*/
	static PRL_RESULT SetDiskSuspendState(const QString & strDisk,const unsigned int & uiSuspendState);

	/**
	* get disk change state parameter
	*/
	static PRL_RESULT GetDiskChangeState(const QString & strDisk,unsigned int & uiChangeState);

	/**
	* set disk change state parameter
	*/
	static PRL_RESULT SetDiskChangeState(const QString & strDisk,const unsigned int & uiChangeState);

	/**
	* Generate full file name for .pvc configuration file
	*/
	static QString MakeCfgFileName(const QString &dir_path, const QString &file_name);

	/**
	* Generate full file name for backup configuration file
	*/
	static QString MakeBackupCfgFileName(const QString &base_file_name);

	/**
	* Save current VM configuration
	*/
	static void SaveVmConfig(const QString &old_name, const QString &new_name, const bool &trim_last_dir = true);

	/**
	* Copy file from oldFilePath to newFilePath
	*/
	static bool CopyStateFile(const QString &oldFilePath, const QString &newFilePath);

	/**
	* Save NVRAM file
	*/
	static void SaveNVRAM(const QString &strVmDir, const QString &strStateUuid);

	/**
	* Get name of NVRAM file
	*/
	static QString GetNVRAMFileName(const QString &strVmDir, const QString &strStateUuid);

	/**
	* Restore NVRAM file
	*/
	static bool RestoreNVRAM(const QString &strVmDir, const QString &strStateUuid);

	/**
	* Remove state file
	*/
	static void RemoveStateFile(const QString &strVmDir, const QString &strStateUuid,
				const QString &strExt, bool bRemoveSnapshotsDir = false);


private:

	/**
	* Base file name, store full patch to VM configuration file
	*/
	QString m_strBaseFile;

};

#endif // STATES_HELPER_H
