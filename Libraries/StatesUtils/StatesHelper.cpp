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
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "Libraries/Logging/Logging.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include "SaReShared.h"

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
* Extract string option from .sav file
*/
bool CStatesHelper::extractStringOption(const QString &strSavFilePath, UINT uOptId, QString &strOption)
{
	QFile file(strSavFilePath);
	SARE_FILE_HDR  SaReHdr;

	SaReHdr.uSaReMagicNum = -1;

	if(!file.open(QIODevice::ReadOnly))
		return false;

	SaReFileReadHeader(file, &SaReHdr);
	if (SaReHdr.uSaReMagicNum == SARE_FILE_MAGIC_NUMBER)
	{
		quint64 pos;
		INT iOptLen = SaReFileFindOption(file, uOptId, pos);
		if (iOptLen > 0)
		{
			QByteArray OptData(iOptLen, 0);
			SaReFileReadOption(file, uOptId, OptData.data(), (UINT*)&iOptLen);
			strOption = UTF8_2QSTR(OptData.data());
			file.close();
			return true;
		}
	}
	file.close();
	return false;
}

/**
* Extract memory file from .sav file
*/
bool CStatesHelper::extractMemFileName(const QString &strSavFilePath, QString &strMemFileName)
{
	return extractStringOption(strSavFilePath, SARE_MEMORY_FILE_NAME_HDR_OPT,strMemFileName);
}

/**
* Extract memory file path from .sav file
*/
bool CStatesHelper::extractMemFilePath(const QString &strSavFilePath, QString &strMemFilePath)
{
	return  extractStringOption(strSavFilePath, SARE_MEMORY_FILE_PATH_HDR_OPT, strMemFilePath);
}

bool CStatesHelper::writeStringOptionExpand(const QString &strSavFilePath, UINT uOptId, const QString &strOption)
{
	UINT uCurOption;
	bool ret = false;
	QString strSavFilePathNew = strSavFilePath + "_new";
	QFile oldFile(strSavFilePath);
	QFile newFile(strSavFilePathNew);
	SARE_FILE_HDR  SaReHdr;
	bool openOld = false, openNew = false;
	char* buffer = NULL;
	uint BUFFER_SIZE = 0x100000;
	bool bRewritten = false;
	UINT uHdrSize;

	openOld = oldFile.open( QIODevice::ReadOnly );
	if(!openOld) {
		goto finish;
	}

	openNew = newFile.open( QIODevice::ReadWrite | QIODevice::Truncate );
	if(!openNew) {
		goto finish;
	}

	buffer = new (std::nothrow) char[BUFFER_SIZE];
	if(NULL == buffer){
		goto finish;
	}

	uHdrSize = SaReFileGetHeaderSize(oldFile);

	//
	// Rewrite header
	//
	if(!SaReFileReadHeader(oldFile, &SaReHdr)){
		goto finish;
	}

	if(!SaReFileWriteHeader(newFile, &SaReHdr)){
		goto finish;
	}

	for(uCurOption = 0; uCurOption < (UINT)SARE_NOTDECLARED_OPT;uCurOption++){

		UINT ActualLen  = BUFFER_SIZE;

		if (uCurOption != SaReFileReadOption(oldFile, uCurOption, buffer, &ActualLen)){

			if(ActualLen > BUFFER_SIZE){

				delete [] buffer;
				buffer = new (std::nothrow) char[ActualLen];
				if(NULL == buffer){
					goto finish;
				}
				if (uCurOption != SaReFileReadOption(oldFile, uCurOption, buffer, &ActualLen)){
					goto finish;
				}
				BUFFER_SIZE = ActualLen;
			}else
				continue;
		}

		if(uCurOption != uOptId){

			if(!SaReFileWriteOption(newFile, uCurOption, buffer, ActualLen)){
				goto finish;
			}

			continue;
		}

		bRewritten = true;

		if(strOption.isEmpty())
			continue;

		if(!SaReFileWriteOption(newFile, uCurOption, strOption.toUtf8().data(), (UINT)strOption.toUtf8().size()+1)){
			goto finish;
		}
	}

	if(!bRewritten && !strOption.isEmpty()){

		if(!SaReFileWriteOption(newFile, uOptId, strOption.toUtf8().data(), (UINT)strOption.toUtf8().size()+1)){
			goto finish;
		}
	}

	if(!oldFile.seek(uHdrSize)){

		goto finish;
	}

	uHdrSize = SaReFileGetHeaderSize(newFile);
	if(!newFile.seek(uHdrSize)){

		goto finish;
	}

	//
	// copy contents
	//

	while(!oldFile.atEnd())
	{
		__int64 len = oldFile.read( buffer, BUFFER_SIZE );
		if (newFile.write( buffer, len ) == -1)
		{
			goto finish;
		}
	}

	newFile.close(); openNew = false;
	oldFile.close(); openOld = false;

	QFile::remove(strSavFilePath);
	QFile::rename(strSavFilePathNew, strSavFilePath);

	ret = true;

finish:

	if(openNew)
		newFile.close();
	if(openOld)
		oldFile.close();

	if(!ret)
		QFile::remove(strSavFilePathNew);

	delete[] buffer;
	buffer = NULL;
	return ret;
}

bool CStatesHelper::writeStringOption(const QString &strSavFilePath, UINT uOptId, const QString &strOption)
{
	bool ret = true;
	SARE_FILE_HDR  SaReHdr;
	quint64 pos;
	INT iOptLen = 0;

	QFile file(strSavFilePath);

	if(!file.open(QIODevice::ReadWrite))
		return false;

	SaReHdr.uSaReMagicNum = -1;

	SaReFileReadHeader(file, &SaReHdr);
	if (SaReHdr.uSaReMagicNum == SARE_FILE_MAGIC_NUMBER)
	{
		iOptLen = SaReFileFindOption(file, uOptId, pos);
		if (iOptLen <= 0 && strOption.isEmpty())
		{
			ret = true;
			goto finish;
		}
	}

	if(iOptLen != strOption.toUtf8().size()+1){

		file.close();
		//
		// expand the file
		//
		return writeStringOptionExpand(strSavFilePath, uOptId, strOption);
	}

	if(!file.seek(pos - sizeof(SARE_FILE_HDR_OPTION))){

		WRITE_TRACE(DBG_FATAL, "writeStringOption failed. sav [%s], str [%s], optid %u",
				qPrintable(strSavFilePath), qPrintable(strOption), uOptId);
		ret = false;
		goto finish;
	}

	if( !SaReFileWriteOption(file, uOptId,
		strOption.toUtf8().data(), (UINT)strOption.toUtf8().size()+1) )
	{
		WRITE_TRACE(DBG_FATAL, "writeStringOption failed. sav [%s], str [%s], optid %u",
			qPrintable(strSavFilePath), qPrintable(strOption), uOptId);
		ret = false;
	}

finish:

	file.close();
	return ret;
}

/**
* Write memory file name into .sav file
*/
bool CStatesHelper::writeMemFileName(const QString &strSavFilePath, const QString &strMemFileName)
{
	return writeStringOption(strSavFilePath, SARE_MEMORY_FILE_NAME_HDR_OPT, strMemFileName);
}

/**
* Write memory file path into .sav file
*/
bool CStatesHelper::writeMemFilePath(const QString &strSavFilePath, const QString &strMemFilePath)
{
	return writeStringOption(strSavFilePath, SARE_MEMORY_FILE_PATH_HDR_OPT, strMemFilePath);
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
