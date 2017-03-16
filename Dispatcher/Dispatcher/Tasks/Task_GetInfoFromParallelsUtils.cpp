////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Task_GetInfoFromParallelsUtils.cpp
///
/// @brief
///	Implementation of the class Task_GetInfoFromParallelsUtils
///
/// @brief
///	This class implements Parallels Utitlites functions Information requests
///
/// @author sergeyt
///	Artemr@
///
////////////////////////////////////////////////////////////////////////////////
#include "Task_GetInfoFromParallelsUtils.h"

#include <prlcommon/Std/PrlAssert.h>
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include "Libraries/StatesUtils/StatesHelper.h"
//#include "Libraries/DiskImage/DiskImage.h"  // DiskImage commented out by request from CP team
#include "Task_CommonHeaders.h"
#include "CDspVmConfigManager.h"

Task_GetInfoFromParallelsUtils::Task_GetInfoFromParallelsUtils(
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& pkg)
:CDspTaskHelper( pUser, pkg )
{

}

Task_GetInfoFromParallelsUtils::~Task_GetInfoFromParallelsUtils()
{

}

PRL_RESULT Task_GetInfoFromParallelsUtils::GetDiskImageInformation(
		const QString & strPathToFile,
		CVmHardDisk & cHardDisk,
		SmartPtr<CDspClient> pUserSession)
{
// VirtualDisk commented out by request from CP team
//	PARALLELS_DISK_PARAMETERS DiskParameters;
//	PRL_RESULT nRetVal = PRL_ERR_SUCCESS;

	LOG_MESSAGE(DBG_DEBUG,
				"Task_GetInfoFromParallelsUtils::GetDiskImageInformation(%s)",
				strPathToFile.toUtf8().data());

	// Pointer to argument is not valid
	if (strPathToFile.isEmpty())
		return PRL_ERR_VM_GET_HDD_IMG_INVALID_ARG;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pUserSession->getAuthHelper() );

	QString strDirPathToFile = strPathToFile;
	if (!QFileInfo(strPathToFile).isDir() && !strPathToFile.endsWith(".vmdk"))
		strDirPathToFile = CFileHelper::GetFileRoot(strPathToFile);

	SmartPtr<CVmHardDisk> pCachedHardDisk =
		CDspService::instance()->getVmConfigManager().getHardDiskConfigCache().
				getConfig(strDirPathToFile, pUserSession );
	if( pCachedHardDisk )
	{
		// Fill only Disk specific fields
		cHardDisk.setVersion(pCachedHardDisk->getVersion());
		cHardDisk.setSystemName(pCachedHardDisk->getSystemName());
		cHardDisk.setUserFriendlyName(pCachedHardDisk->getUserFriendlyName());
		cHardDisk.setEmulatedType(pCachedHardDisk->getEmulatedType());
		cHardDisk.setSplitted(pCachedHardDisk->isSplitted());
		cHardDisk.setDiskType(pCachedHardDisk->getDiskType());
		cHardDisk.setBlockSize(pCachedHardDisk->getBlockSize());
		cHardDisk.setSize(pCachedHardDisk->getSize());
		cHardDisk.setCompatLevel(pCachedHardDisk->getCompatLevel());
		cHardDisk.setSizeOnDisk(pCachedHardDisk->getSizeOnDisk());

		/* FIXME: get size by demand */
		// set size on disk in MB
		quint64 ui64Size = 0;
		CSimpleFileHelper::GetDirSize(strDirPathToFile, &ui64Size);
		cHardDisk.setSizeOnDisk(ui64Size/(1024*1024));

		return PRL_ERR_SUCCESS;
	}

// VirtualDisk commented out by request from CP team
//	IDisk* pDisk = IDisk::OpenDisk( strDirPathToFile,
//			PRL_DISK_READ | PRL_DISK_FAKE_OPEN,
//			&nRetVal);
//	if ( !pDisk )
//	{
//		switch (IDisk::IsDiskValid(strPathToFile))
//		{
//			case PRL_CHECKED_DISK_VALID:
//				// Can not happen
//				break;
//			case PRL_CHECKED_DISK_OLD_VERSION:
//				cHardDisk.setVersion(PVE::HardDiskV1);
//				return PRL_ERR_SUCCESS;
//			case PRL_CHECKED_DISK_INVALID:
//				cHardDisk.setVersion(PVE::HardDiskInvalidVersion);
//				return PRL_ERR_SUCCESS;
//			default:
//				cHardDisk.setVersion(PVE::HardDiskInvalidVersion);
//				return PRL_ERR_SUCCESS;
//		}
//
//		WRITE_TRACE(DBG_FATAL, "Error opening disk %s to get info."
//				" Received NULL reference. Error 0x%x",
//				QSTR2UTF8(strPathToFile), nRetVal);
//		return PRL_ERR_VM_GET_HDD_IMG_NOT_OPEN;
//	}
//
//	nRetVal = pDisk->GetParameters(DiskParameters);
//	if (PRL_FAILED(nRetVal)) {
//		pDisk->Close();
//		pDisk->Release();
//		return PRL_ERR_VM_GET_HDD_IMG_NOT_OPEN;
//	}
//
//	LOG_MESSAGE(DBG_FATAL, "Cyl %u Head %u SPT %u\n",
//		DiskParameters.m_Cylinders,
//		DiskParameters.m_Heads,
//		DiskParameters.m_Sectors);
//
//	cHardDisk.setVersion(PVE::HardDiskV2);
//	cHardDisk.setSystemName(strDirPathToFile);
//	cHardDisk.setUserFriendlyName(strDirPathToFile);
//	cHardDisk.setEmulatedType(PVE::HardDiskImage);
//	cHardDisk.setSplitted(DiskParameters.m_Storages.size() > 1);
//
//	switch ((*DiskParameters.m_Storages.begin()).Type)
//	{
//		case PRL_IMAGE_PLAIN:
//		case PRL_IMAGE_VMDK_MONOLITHIC_FLAT:
//		case PRL_IMAGE_VMDK_EXTENT_2GB_FLAT:
//			cHardDisk.setDiskType(PHD_PLAIN_HARD_DISK);
//			break;
//		case PRL_IMAGE_COMPRESSED:
//		case PRL_IMAGE_VMDK_MONOLITHIC_SPARSE:
//		case PRL_IMAGE_VMDK_EXTENT_2GB_SPARSE:
//			cHardDisk.setDiskType(PHD_EXPANDING_HARD_DISK);
//			break;
//		default:
//			nRetVal = PRL_ERR_INVALID_HDD_GEOMETRY;
//			break;
//	}
//
//	cHardDisk.setBlockSize(DiskParameters.m_BlockSize);
//	// virtual size of hdd image
//	// size in MB - one sector 512 byte
//	cHardDisk.setSize(DiskParameters.m_SizeInSectors/2048);
//	// set size on disk in MB
//	quint64 ui64Size = 0;
//	CSimpleFileHelper::GetDirSize(strDirPathToFile,&ui64Size);
//	cHardDisk.setSizeOnDisk(ui64Size/(1024*1024));
//
//	// check suspend and change state flags on disk
//	if (cHardDisk.getVersion() == PVE::HardDiskV2)
//	{
//		QString strParam;
//
//		PRL_RESULT res = pDisk->GetUserParameter(SUSPEND_STATE_PARAM_NAME, strParam);
//		if (PRL_SUCCEEDED(res) && strParam.toUInt())
//			cHardDisk.setVersion(PVE::HardDiskV2Suspended);
//	}
//
//	if (PRL_SUCCEEDED(nRetVal))
//	{
//		QString qsCompatLevel;
//		PRL_RESULT Err = pDisk->GetUserParameter(QString(COMPAT_LEVEL_PARAM_NAME), qsCompatLevel);
//		if (PRL_SUCCEEDED(Err))
//			cHardDisk.setCompatLevel(qsCompatLevel);
//	}
//
//	pDisk->Close();
//	pDisk->Release();
//
//	CDspService::instance()->getVmConfigManager().
//		getHardDiskConfigCache().update(strDirPathToFile,
//				SmartPtr<CVmHardDisk>(new CVmHardDisk(cHardDisk)),
//				pUserSession);
//
//	return nRetVal;
	return PRL_ERR_FAILURE;
}

