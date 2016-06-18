////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2007-2015 Parallels IP Holdings GmbH
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
/// MODULE: StatesHelper.cpp
///
/// DESCRIPTION:
///		CStatesHelper class implementation file
///
/////////////////////////////////////////////////////////////////////////////

#include "StatesHelper.h"
#include "CFileHelper.h"
#include <QFileInfo>
#include <QDir>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Logging/Logging.h>
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team

#define EXT_SAV		".sav"
#define EXT_MEM		".mem"
#define MASK_MEM	"*mem"


/**
 * Construction
 */
CStatesHelper::CStatesHelper(const QString &base_file) : m_strBaseFile(base_file)
{

}

/**
* Generate full file name for suspended VM save file
*/
QString CStatesHelper::getSavFileName()
{
	return CFileHelper::GetFileRoot(m_strBaseFile) + "/" + CFileHelper::GetBaseFileName(m_strBaseFile)  + EXT_SAV;
}

/**
* Returns true if the save file exists
*/
bool CStatesHelper::savFileExists()
{
	return QFileInfo(getSavFileName()).exists();
}

/**
* Remove state files from disk, removing with current process access rights!
*/
bool CStatesHelper::dropStateFiles()
{
	return QFile::remove(getSavFileName());
}


/**
 * Copy file from oldFilePath to newFilePath
 */
bool CStatesHelper::CopyStateFile(const QString &oldFilePath, const QString &newFilePath)
{
	//same file, no need to copy
	if(oldFilePath.compare(newFilePath) == 0)
		return true;

	//load both files
	QFile oldFile(oldFilePath);
	QFile newFile(newFilePath);
	bool openOld = oldFile.open( QIODevice::ReadOnly );
	//if either file fails to open bail
	if(!openOld) { return false; }
	bool openNew = newFile.open( QIODevice::WriteOnly );
	if(!openNew) { oldFile.close(); return false; }

	//copy contents
	uint BUFFER_SIZE = 0x10000;
	char* buffer = new char[BUFFER_SIZE];
	bool bRet = true;
	while(!oldFile.atEnd())
	{
		__int64 len = oldFile.read( buffer, BUFFER_SIZE );
		if (newFile.write( buffer, len ) == -1)
		{
			bRet = false;
			break;
		}
	}

	// SetFreeShareOnFile(newFilePath);

	//deallocate buffer
	delete[] buffer;
	buffer = NULL;
	oldFile.close();
	newFile.close();
	return bRet;
}

/**
* get disk suspend state parameter
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
PRL_RESULT CStatesHelper::GetDiskSuspendState(const QString & /*strDisk*/,unsigned int & /*uiSuspendState*/)
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT res = PRL_ERR_SUCCESS;
//
//	/*IDisk* pDiskImage = IDisk::OpenDisk(strDisk,
//		PRL_DISK_DEFAULT_FLAG, NULL,
//		&res);*/
//
//	IDisk* pDiskImage = IDisk::OpenDisk(strDisk, (PRL_DISK_OPEN_FLAGS)(PRL_DISK_READ | PRL_DISK_FAKE_OPEN), &res);
//
//	if (pDiskImage)
//	{
//		QString strParam;
//		res = pDiskImage->GetUserParameter(SUSPEND_STATE_PARAM_NAME, strParam);
//
//		if(res == PRL_ERR_SUCCESS)
//			uiSuspendState = strParam.toUInt();
//
//		pDiskImage->Release();
//		pDiskImage = NULL;
//	}
//	return res;
	return PRL_ERR_FAILURE;
}

/**
* set disk suspend state parameter
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
PRL_RESULT CStatesHelper::SetDiskSuspendState(const QString & /*strDisk*/,const unsigned int & /*uiSuspendState*/)
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT res = PRL_ERR_SUCCESS;
//
//	IDisk* pDiskImage = IDisk::OpenDisk(strDisk,
//		(PRL_DISK_OPEN_FLAGS)(PRL_DISK_DEFAULT_FLAG | PRL_DISK_XML_CHANGE),
//		&res);
//
//	if (pDiskImage)
//	{
//		QString strChangeParam = QString("%1").arg(uiSuspendState);
//		res = pDiskImage->SetUserParameter(SUSPEND_STATE_PARAM_NAME, strChangeParam);
//
//		pDiskImage->Release();
//		pDiskImage = NULL;
//	}
//
//	return res;
	return PRL_ERR_FAILURE;
}

/**
* get disk change state parameter
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
PRL_RESULT CStatesHelper::GetDiskChangeState(const QString & /*strDisk*/,unsigned int & /*uiChangeState*/)
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT res = PRL_ERR_SUCCESS;
//
//	/*IDisk* pDiskImage = IDisk::OpenDisk(strDisk,
//		PRL_DISK_DEFAULT_FLAG, NULL,
//		&res);*/
//
//	// Try to create
//	IDisk* pDiskImage = IDisk::OpenDisk(strDisk, (PRL_DISK_OPEN_FLAGS)(PRL_DISK_READ | PRL_DISK_FAKE_OPEN), &res);
//
//	if (pDiskImage)
//	{
//		QString strParam;
//		res = pDiskImage->GetUserParameter(CHANGE_STATE_PARAM_NAME, strParam);
//
//		if(res == PRL_ERR_SUCCESS)
//			uiChangeState = strParam.toUInt();
//
//		pDiskImage->Release();
//		pDiskImage = NULL;
//	}
//	return res;
	return PRL_ERR_FAILURE;
}

/**
* get disk change state parameter
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
PRL_RESULT CStatesHelper::SetDiskChangeState(const QString & /*strDisk*/,const unsigned int & /*uiChangeState*/)
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT res = PRL_ERR_SUCCESS;
//
//	IDisk* pDiskImage = IDisk::OpenDisk(strDisk,
//		(PRL_DISK_OPEN_FLAGS)(PRL_DISK_DEFAULT_FLAG | PRL_DISK_XML_CHANGE),
//		&res);
//
//	if (pDiskImage)
//	{
//		QString strChangeParam = QString("%1").arg(uiChangeState);
//		res = pDiskImage->SetUserParameter(CHANGE_STATE_PARAM_NAME, strChangeParam);
//
//		pDiskImage->Release();
//		pDiskImage = NULL;
//	}
//	return res;
	return PRL_ERR_FAILURE;
}

/**
* set suspend flag for all disks in config
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
PRL_RESULT CStatesHelper::SetSuspendParameterForAllDisks(const CVmConfiguration * pVmConfig,
		unsigned int uiSuspendState)
{
	if (!pVmConfig)
		return PRL_ERR_INVALID_ARG;

	PRL_RESULT res = PRL_ERR_SUCCESS;
	CVmHardware * lpcHardware = pVmConfig->getVmHardwareList();
	int i = 0;
	unsigned int ui_current_state = 0;
	for (i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
	{
		if (!lpcHardware->m_lstHardDisks[i]->getEnabled())
			continue;
		res = GetDiskSuspendState(lpcHardware->m_lstHardDisks[i]->getSystemName(),ui_current_state);
		if( PRL_FAILED(res) || uiSuspendState != ui_current_state )
		{
			res = SetDiskSuspendState(lpcHardware->m_lstHardDisks[i]->getSystemName(),uiSuspendState);
			if (PRL_FAILED(res))
				break;
		}
	}

	return res;
}

/**
* set change flag for all disks in config
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
PRL_RESULT CStatesHelper::SetChangeParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int uiChangeState)
{
	if (!pVmConfig)
		return PRL_ERR_INVALID_ARG;

	PRL_RESULT res = PRL_ERR_SUCCESS;
	CVmHardware * lpcHardware = pVmConfig->getVmHardwareList();
	int i = 0;
	unsigned int ui_current_state = 0;
	for (i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
	{
		if (!lpcHardware->m_lstHardDisks[i]->getEnabled())
			continue;
		res = GetDiskChangeState(lpcHardware->m_lstHardDisks[i]->getSystemName(),ui_current_state);
		if (PRL_FAILED(res) || uiChangeState != ui_current_state)
		{
			res = SetDiskChangeState(lpcHardware->m_lstHardDisks[i]->getSystemName(),uiChangeState);
			if (PRL_FAILED(res))
				break;
		}
	}

	return res;
}

/**
* check that change flag present on any disk in config
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
void CStatesHelper::GetChangeParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int & uiChangeState)
{
	if (!pVmConfig)
		return;

	CVmHardware * lpcHardware = pVmConfig->getVmHardwareList();
	uiChangeState = 0;
	int i = 0;
	for (i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
	{
		if ( lpcHardware->m_lstHardDisks[i]->getEnabled())
		{
			PRL_RESULT res = GetDiskChangeState(lpcHardware->m_lstHardDisks[i]->getSystemName(),
					uiChangeState);
			if (PRL_FAILED(res))
				continue;
		}

		if(uiChangeState == 1)
			return;
	}

	return;
}

/**
* check that suspend flag present on any disk in config
*
* @param File name
* @param incoming suspend state 0-not set, 1-set
*
* @author Artemr@
*/
void CStatesHelper::GetSuspendParameterForAllDisks(const CVmConfiguration * pVmConfig,unsigned int & uiSuspendState)
{
	if (!pVmConfig)
		return;

	CVmHardware * lpcHardware = pVmConfig->getVmHardwareList();
	uiSuspendState = 0;
	int i = 0;
	for (i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
	{
		if( lpcHardware->m_lstHardDisks[i]->getEnabled())
		{
			PRL_RESULT res = GetDiskSuspendState(lpcHardware->m_lstHardDisks[i]->getSystemName(),
					uiSuspendState);
			if (PRL_FAILED(res))
				continue;
		}

		if(uiSuspendState == 1)
			return;
	}

}

/**
* Generate full file name for .pvc configuration file
*/
QString CStatesHelper::MakeCfgFileName(const QString &dir_path, const QString &file_name)
{
	return dir_path + "/Snapshots/" + file_name + ".pvc";
}

/**
* Generate full file name for backup configuration file
*/
QString CStatesHelper::MakeBackupCfgFileName(const QString &base_file_name)
{
	QString dir_path = QFileInfo(base_file_name).dir().path();

	QString ret = dir_path + "/" + QFileInfo(base_file_name).completeBaseName() + ".bak";

	return ret;
}

/**
* Save VM configuration
*/
void CStatesHelper::SaveVmConfig(const QString &old_name, const QString &new_name, const bool &trim_last_dir /* = true */)
{
	Q_UNUSED(trim_last_dir);

	// Create folder
	QFileInfo qfi(new_name);
	QDir dir;
	QString dir_path = qfi.dir().path();
	if (!dir.exists(dir_path))
	{
		dir.mkpath(dir_path);
	}

	// Save to configuration file
	QFile f(old_name);
	f.copy(new_name);

	// SetFreeShareOnDirAndChildObjects(dir_path);
}

QString CStatesHelper::GetNVRAMFileName(const QString &strVmDir, const QString &strStateUuid)
{
	QString strDestFile = strVmDir + "/Snapshots/" + strStateUuid + ".dat";
	return strDestFile;
}

/**
* Save NVRAM file
*/
void CStatesHelper::SaveNVRAM(const QString &strVmDir, const QString &strStateUuid)
{
	QString strSourceFile = strVmDir + "/" + PRL_VM_NVRAM_FILE_NAME;

	QString strDestFile = GetNVRAMFileName(strVmDir, strStateUuid);

	CStatesHelper::CopyStateFile(strSourceFile, strDestFile);
}

/**
* Restore NVRAM file
*/
bool CStatesHelper::RestoreNVRAM(const QString &strVmDir, const QString &strStateUuid)
{
	QString strSourceFile = GetNVRAMFileName(strVmDir, strStateUuid);
	QString strDestFile = strVmDir + "/" + PRL_VM_NVRAM_FILE_NAME;
	return CStatesHelper::CopyStateFile(strSourceFile, strDestFile);
}

/**
* Remove state file
*/
void CStatesHelper::RemoveStateFile(const QString &strVmDir, const QString &strStateUuid,
									const QString &strExt, bool bRemoveSnapshotsDir /* = false */)
{
	QString strDestFile =  strVmDir + "/Snapshots/" + strStateUuid + strExt;
	QFile f(strDestFile);

	// Check that file present
	if (f.exists())
	{
		QFile().remove(strDestFile);
	}

	if (bRemoveSnapshotsDir)
	{
		QDir().rmdir(strVmDir + "/Snapshots/");
	}
}
