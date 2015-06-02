////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Task_CreateImage.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////
#include "CVmValidateConfig.h"
#include "Task_CreateImage.h"
#include "Task_CommonHeaders.h"
#include "Libraries/Std/PrlAssert.h"

//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include "Libraries/HostUtils/HostUtils.h"
#include "Build/Current.ver"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#ifndef PAGE_SIZE
	#define PAGE_SIZE	4096
#endif

// 2Gb free diskspace reservation
#define DISKSPACE_RESERVATION_MB	2 * 1024

// Initialize disk at creation (bootsector writing)
// #define WRITE_BOOTSECTOR_AT_CREATION

/**
 * @brief Callback function, called from CreateParallelsDisk
 * @param iDone
 * @param iTotal
 * @param pUserData
 * @return TRUE to continue HardDisk image creation
 */
PRL_BOOL HddCallbackToCreateImageTask ( PRL_INT32 iDone, PRL_INT32 iTotal, PRL_VOID_PTR pUserData )
{
	// Getting current class
	Task_CreateImage* pTaskOriginal = reinterpret_cast<Task_CreateImage*>(pUserData);

	Mixin_CreateHddSupport* pHddCreateHelper = pTaskOriginal;

	if (!HddCallbackHelperFunc(iDone, iTotal, pHddCreateHelper))
		return PRL_FALSE;

	return PRL_TRUE;
}

/**
 * Task_CreateImage constructor
 */
Task_CreateImage::Task_CreateImage (
	SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
	const SmartPtr<CVmConfiguration>& pVmConfig,
	const QString& image_config,
	bool flgRecreateIsAllowed,
	bool bForceQuestionsSign )

:	Mixin_CreateHddSupport(client, p, bForceQuestionsSign),
	m_pVmConfig(pVmConfig),
	m_sImageConfig(image_config),
	m_flgRecreateIsAllowed( flgRecreateIsAllowed )
{
	LOG_MESSAGE( DBG_INFO, "imageCOnfig=\n%s", QSTR2UTF8( m_sImageConfig ) );

	//////////////////////////////////////////////////////////////////////////
	setTaskParameters( m_pVmConfig->getVmIdentification()->getVmUuid(), m_sImageConfig, m_flgRecreateIsAllowed );
}

void Task_CreateImage::setTaskParameters( const QString& strVmUuid,
										const QString& image_config,
										bool flgRecreateIsAllowed )
{
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, strVmUuid, EVT_PARAM_DISP_TASK_CREATE_IMAGE_VM_UUID ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::CData, image_config, EVT_PARAM_DISP_TASK_CREATE_IMAGE_DEV_CONFIG ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::Boolean, QString("%1").arg( flgRecreateIsAllowed ), EVT_PARAM_DISP_TASK_CREATE_IMAGE_RECREATE_FLAG ) );
}

QString Task_CreateImage::ConvertToFullPath(const QString &sImagePath)
{
	if (!QFileInfo(sImagePath).isAbsolute())
	{
		QString sVmHomeDirPath = QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath();
		if (!QFileInfo(sVmHomeDirPath).isDir())
			sVmHomeDirPath = QFileInfo(sVmHomeDirPath).absolutePath();
		QFileInfo _fi(sVmHomeDirPath + '/' + sImagePath);
		return (_fi.absoluteFilePath());
	}

	return (sImagePath);
}

QString Task_CreateImage::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_CreateImage::run_body()
{

	PRL_RESULT ret = PRL_ERR_SUCCESS;

	bool flgImpersonated = false;
    try
    {
        // Let current thread impersonate the security context of a logged-on user
		if( ! getClient()->getAuthHelper().Impersonate() )
			throw PRL_ERR_IMPERSONATE_FAILED;
		flgImpersonated = true;

        /**
         * check parameters
         */

        if ( m_sImageConfig.isEmpty() )
            throw PRL_ERR_BAD_PARAMETERS;

        /**
         * parse image configuration XML
         */
        CVmHardDisk hard_disk;
        CVmFloppyDisk floppy_disk;
        PRL_DEVICE_TYPE dev_type = PDE_GENERIC_DEVICE;

        if (StringToElement(&floppy_disk, m_sImageConfig))
            dev_type = PDE_FLOPPY_DISK;
        else  if (StringToElement(&hard_disk, m_sImageConfig))
            dev_type = PDE_HARD_DISK;

        switch(dev_type)
        {
        case PDE_FLOPPY_DISK:
			{
				if (CVmValidateConfig::HasSysNameInvalidSymbol(floppy_disk.getSystemName()))
				{
					getLastError()->addEventParameter(new CVmEventParameter(
										PVE::String,
										CVmValidateConfig::GetFileNameFromInvalidPath(floppy_disk.getSystemName()),
										EVT_PARAM_MESSAGE_PARAM_0));
					throw PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_HAS_INVALID_SYMBOL;
				}

				PRL_RESULT ret = createFdd( floppy_disk );
				if( PRL_FAILED( ret ) )
					throw ret;
			}
            break;

        case PDE_HARD_DISK:
			{
				if (CVmValidateConfig::HasSysNameInvalidSymbol(hard_disk.getSystemName()))
				{
					getLastError()->addEventParameter(new CVmEventParameter(
											PVE::String,
											CVmValidateConfig::GetFileNameFromInvalidPath(hard_disk.getSystemName()),
											EVT_PARAM_MESSAGE_PARAM_0));
					throw PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_HAS_INVALID_SYMBOL;
				}

				PRL_RESULT ret = createHdd( hard_disk );
				if( PRL_FAILED( ret ) )
					throw ret;
			}
			break;

        default:
            // send error to user: can't parse image config
            throw PRL_ERR_BAD_PARAMETERS;
        }


		//////////////////////////////////////////////////////////////////////////
		// Terminates the impersonation of a user, return to Dispatcher access rights
		if( ! getClient()->getAuthHelper().RevertToSelf() ) // Don't throw because this thread already finished.
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

    }
    catch (PRL_RESULT code)
    {
        ret = code;
        getLastError()->setEventCode( code );

				if( flgImpersonated && ! getClient()->getAuthHelper().RevertToSelf() )
					WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

				WRITE_TRACE(DBG_FATAL, "Error occurred while creation image file with code [%#x][%s]",
					code, PRL_RESULT_TO_STRING( code ) );
    }

    return ret;
}

// VirtualDisk commented out by request from CP team
///*
// * Run through disk collection and get recovery partition name.
// * It is very slow... get full hardware info, serialize to string and
// * then deserialize it... OMG!
// */
//static QString GetMacRecovery()
//{
//	CHostHardwareInfo hwInfo;
//	{
//		CDspLockedPointer<CDspHostInfo> lockedHostInfo =
//			CDspService::instance()->getHostInfo();
//		hwInfo.fromString( lockedHostInfo->data()->toString() );
//	}
//
//	foreach( CHwHardDisk* disk, hwInfo.m_lstHardDisks )
//	{
//		foreach( CHwHddPartition* partition, disk->m_lstPartitions )
//		{
//			if ( partition->getType() == PRL_MACOS_BOOT_PARTITION )
//				return partition->getSystemName();
//		}
//	}
//
//	return QString();
//}

// VirtualDisk commented out by request from CP team
///**
// * @brief Initialize disk for guest Mac OS.
// * If the special flag specified - copy recovery partition from host.
// *
// * @param Disk class
// *
// * @return SDK Error code
// **/
//PRL_RESULT Task_CreateImage::InitializeMacDisk(IDisk* Disk)
//{
//	QString Recovery;
//
//	if (getRequestFlags() & PCDIF_CREATE_FROM_LION_RECOVERY_PARTITION)
//	{
//		Recovery = GetMacRecovery();
//		// Need to remove /dev/
//		Recovery.replace(QString("/dev/"), QString());
//
//		WRITE_TRACE(DBG_FATAL, "Asked for recovery copy. [%s]",
//					QSTR2UTF8(Recovery));
//	}
//
//	return IDiskConfigurator::InitMacDisk(Disk, Recovery);
//}

// VirtualDisk commented out by request from CP team
///**
// * @brief Initialize created disk (MBR, partitions
// *
// * @param Disk class
// * @param OS type
// * @param If Disk pointer is NULL, store here should disk be initialized or not
// *
// * @return SDK Error code
// **/
//PRL_RESULT Task_CreateImage::InitializeDisk(IDisk* pDisk, unsigned int uiOSType, bool* bNeedInit)
//{
//	// Return value
//	PRL_RESULT Err = PRL_ERR_DISK_CANT_INITIALIZE_IMAGE;
//	bool bInited = false;
//
//	// Check parameter
//	if (pDisk == NULL && bNeedInit == NULL)
//	{
//		WRITE_TRACE(DBG_FATAL, "[partitioning] pDisk is NULL" );
//		goto Exit;
//	}
//
//	Err = PRL_ERR_SUCCESS;
//
//	switch(uiOSType)
//	{
//	/*
//	* If it is SuSE linux, we should not add initial bootsector, due to
//	* strange installer logic. It will not write bootloader if it find
//    * that boot sector is not empty.
//    */
//	case PVS_GUEST_VER_LIN_SUSE:
//	case PVS_GUEST_VER_LIN_OPENSUSE:
//		break;
//
//	/*
//	* We need initialize Mac OS partitions for OS installation
//    */
//	case PVS_GUEST_VER_MACOS_TIGER:
//	case PVS_GUEST_VER_MACOS_LEOPARD:
//	case PVS_GUEST_VER_MACOS_SNOW_LEOPARD:
//		bInited = true;
//
//		if (!pDisk)
//			break;
//
//		Err = InitializeMacDisk(pDisk);
//		break;
//
//	default:
//#ifdef WRITE_BOOTSECTOR_AT_CREATION
//		bInited = true;
//
//		if (pDisk)
//			Err = IDiskConfigurator::InitializeBoot(pDisk);
//#else
//		Err = PRL_ERR_SUCCESS;
//#endif
//		break;
//	}
//
//Exit:
//	if (bNeedInit)
//		*bNeedInit = bInited;
//
//	return Err;
//}

PRL_RESULT Task_CreateImage::checkOnFATSupport(const QString strFullPath, PRL_UINT64 uiFileSizeMb)
{
	PRL_RESULT rc = PRL_ERR_SUCCESS;
	QString strExistPathToCheck = CFileHelper::GetFileRoot( strFullPath );
	switch(HostUtils::GetFSType(strExistPathToCheck))
	{
	case PRL_FS_FAT:
		if (uiFileSizeMb < 2048)
			return PRL_ERR_SUCCESS;

		WRITE_TRACE(DBG_FATAL, "file of disk image [%s] on FAT FS is more "
					"then 2048 mb..", QSTR2UTF8( strFullPath ) );
		rc = PRL_ERR_CANTS_CREATE_DISK_IMAGE_ON_FAT;
		break;

	case PRL_FS_FAT32:
		if (uiFileSizeMb < 4096)
			return PRL_ERR_SUCCESS;

		WRITE_TRACE(DBG_FATAL, "file of disk image [%s] on FAT32 FS is more "
					"then 2048 mb..", QSTR2UTF8( strFullPath ) );
		rc = PRL_ERR_CANTS_CREATE_DISK_IMAGE_ON_FAT32;
		break;

	default:
		return PRL_ERR_SUCCESS;
	}

	getLastError()->addEventParameter(
		new CVmEventParameter( PVE::String,
		strFullPath,
		EVT_PARAM_MESSAGE_PARAM_0));
	return rc;
}

PRL_RESULT Task_CreateImage::createFdd( const CVmFloppyDisk& floppy_disk )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
        QString strFullPath = "";
        QString strDir = "";
// VirtualDisk commented out by request from CP team
//		// Disk parameters
//		PARALLELS_DISK_PARAMETERS diskParameters;


		/**
		* Validate Floppy Disk full path
		*/

		strFullPath = ConvertToFullPath(floppy_disk.getUserFriendlyName());

		if ( (PRL_VM_DEV_EMULATION_TYPE)floppy_disk.getEmulatedType() == PDT_USE_IMAGE_FILE)
		{
			// If we need to use existence floppy disk image
			//				if (!floppy_disk.getToBeRemoved())
			// Check whether disk image already exist
			if (CFileHelper::FileExists(strFullPath, &getClient()->getAuthHelper()) )
			{
				if ( m_flgRecreateIsAllowed )
				{
					// check if file was busy
					if(CFileHelper::IsFileBusy(strFullPath))
					{
						WRITE_TRACE(DBG_FATAL, "file of disk image [%s] is busy now .", QSTR2UTF8( strFullPath ) );
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String,
							QObject::tr("floppy"),
							EVT_PARAM_MESSAGE_PARAM_0));

						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String,
							QObject::tr("floppy"),
							EVT_PARAM_MESSAGE_PARAM_1));

						throw PRL_ERR_DISK_IMAGE_BUSY;
					}
					if( ! CFileHelper::FileCanWrite( strFullPath, &getClient()->getAuthHelper() ) )
					{
						WRITE_TRACE(DBG_FATAL, "User hasn't access to fdd image [%s].", QSTR2UTF8( strFullPath ) );

						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String,
							QObject::tr("floppy"),
							EVT_PARAM_MESSAGE_PARAM_0));

						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String,
							strFullPath,
							EVT_PARAM_MESSAGE_PARAM_1));

						throw PRL_ERR_ACCESS_DENIED_TO_DISK_IMAGE;
					}

					WRITE_TRACE(DBG_FATAL, "Floppy image would be recreate. path = [%s]", QSTR2UTF8( strFullPath ) );
				}
				else
				{
					// Floppy disk image file already exist
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,
						strFullPath,
						EVT_PARAM_MESSAGE_PARAM_0));
					throw PRL_ERR_FLOPPY_IMAGE_ALREADY_EXIST;
				}
			}

			/**
			* Create new floppy image
			*/

			// Check directory
			QString strDir = CFileHelper::GetFileRoot(strFullPath);

			if (!CFileHelper::DirectoryExists(strDir, &getClient()->getAuthHelper()))
			{
				if (!CFileHelper::WriteDirectory(strDir, &getClient()->getAuthHelper()))
				{
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,
						strDir,
						EVT_PARAM_MESSAGE_PARAM_0));
					throw PRL_ERR_MAKE_DIRECTORY;
				}
			}

			bool bFileCreated = CFileHelper::CreateBlankFile(strFullPath, &getClient()->getAuthHelper());
			QFile floppy(strFullPath);
			// Write floppy header (boot sector and 3 additional bytes)
#ifdef _LIN_
			if (bFileCreated && floppy.open(QIODevice::ReadWrite) &&

				( floppy.resize(CFG_144_IMAGE_SIZE) // bug #424264
									||
				(CFileHelper::isRemotePath( strFullPath ) && (CFG_144_IMAGE_SIZE == floppy.size()))
									) &&
#else// _LIN_
			if (bFileCreated && floppy.open(QIODevice::WriteOnly) &&
				floppy.resize(CFG_144_IMAGE_SIZE) &&
#endif // _LIN_
				floppy.write((char *)&floppy_header_data, sizeof(floppy_header_data)) == sizeof(floppy_header_data)
				)
			{
				floppy.close();
			}
			else
			{
				if (floppy.isOpen())
					floppy.close();

				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String,
					strFullPath,
					EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_CANT_CREATE_FLOPPY_IMAGE;
			}
		}
		else
		{
			// Use real device
		}
	}
	catch( PRL_RESULT err )
	{
		ret = err;
	}

	return ret;
}

PRL_RESULT Task_CreateImage::createHdd(const CVmHardDisk& dto_)
{
	QString strFullPath = ConvertToFullPath(dto_.getUserFriendlyName());

	WRITE_TRACE(DBG_INFO, "Start to create hdd disk image:%s", QSTR2UTF8( strFullPath ) );

	PRL_RESULT output = hddStep2_CheckConditions(dto_);
	if (PRL_FAILED(output))
		return output;

	QStringList a;
	a << "create" << "-f" << "qcow2" << strFullPath;
	a << QString::number(dto_.getSize()).append("M");
	int r = QProcess::execute("qemu-img", a);
	if (0 != r)
	{
		getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
			strFullPath,
			EVT_PARAM_MESSAGE_PARAM_0));
		getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
			QString("Internal error: %1").arg(r),
			EVT_PARAM_DETAIL_DESCRIPTION));

		output = PRL_ERR_CANT_CREATE_HDD_IMAGE;
	}
	WRITE_TRACE(DBG_INFO, "hdd disk image creation finished with code %#x %s"
		, output, PRL_RESULT_TO_STRING(output) );

	return output;
}

// VirtualDisk commented out by request from CP team
//PRL_RESULT Task_CreateImage::hddStep1_RemoveExistingHdd( const CVmHardDisk& /*hard_disk*/ )
//{
//	PRL_RESULT ret_value = PRL_ERR_SUCCESS;
//	try
//	{
//		QString strFullPath = ConvertToFullPath(hard_disk.getUserFriendlyName());
//
//		//Workaround for https://bugzilla.sw.ru/show_bug.cgi?id=120216
//		QFileInfo _fi(strFullPath);
//		if (!_fi.fileName().endsWith(".hdd") || !_fi.filePath().endsWith(".hdd"))
//		{
//			getLastError()->addEventParameter(
//				new CVmEventParameter( PVE::String,
//				strFullPath,
//				EVT_PARAM_MESSAGE_PARAM_0));
//			throw PRL_ERR_DISK_INVALID_FORMAT;
//		}
//
//		PARALLELS_DISK_PARAMETERS hddParameters;
//		PRL_RESULT err = IDisk::GetDiskInfo( strFullPath, hddParameters );
//		if( PRL_SUCCEEDED(err) )
//		{
//			typedef std::list<struct __STORAGE_PARAMETERS>::const_iterator CIT;
//			for( CIT it = hddParameters.m_Storages.begin();
//				it != hddParameters.m_Storages.end(); it++ )
//			{
//				QString strFullFileName = strFullPath + "/" + it->FileName;
//				// check if file was busy
//				if(CFileHelper::IsFileBusy(strFullFileName))
//				{
//					WRITE_TRACE(DBG_FATAL,
//						"file of disk image [%s] is busy now .",
//						QSTR2UTF8( strFullPath ) );
//
//					getLastError()->addEventParameter(
//						new CVmEventParameter( PVE::String,
//						QObject::tr("hard disk"),
//						EVT_PARAM_MESSAGE_PARAM_0));
//
//					getLastError()->addEventParameter(
//						new CVmEventParameter( PVE::String,
//						QObject::tr("hard disk"),
//						EVT_PARAM_MESSAGE_PARAM_1));
//
//					throw PRL_ERR_DISK_IMAGE_BUSY;
//				}
//				if( ! CFileHelper::FileCanWrite( strFullFileName, &getClient()->getAuthHelper() ) )
//				{
//					WRITE_TRACE(DBG_FATAL,
//						"User hasn't access to part of disk image [%s].",
//						QSTR2UTF8( strFullFileName ) );
//
//					getLastError()->addEventParameter(
//						new CVmEventParameter( PVE::String,
//						QObject::tr("hard disk"),
//						EVT_PARAM_MESSAGE_PARAM_0));
//
//					getLastError()->addEventParameter(
//						new CVmEventParameter( PVE::String,
//						strFullPath,
//						EVT_PARAM_MESSAGE_PARAM_1));
//
//					throw PRL_ERR_ACCESS_DENIED_TO_DISK_IMAGE;
//				}
//			}//for CIT
//
//		}
//		else if (	PRL_ERR_DISK_XML_INVALID != err &&
//			PRL_ERR_DISK_XML_OPEN_FAILED != err &&
//			PRL_ERR_DISK_INVALID_FORMAT != err)
//		{
//			WRITE_TRACE(DBG_FATAL, "IDisk::GetDiskInfo failed with error %#x( %s ), for [%s]."
//				, err
//				, PRL_RESULT_TO_STRING( err )
//				, QSTR2UTF8( strFullPath )
//				);
//			getLastError()->addEventParameter(
//				new CVmEventParameter( PVE::String,
//				strFullPath,
//				EVT_PARAM_MESSAGE_PARAM_0));
//			throw err;
//		}
//
//		BOOL bRemoveSuccess = FALSE;
//
//		if (PRL_ERR_DISK_INVALID_FORMAT != err)
//		{
//			err = IDisk::Remove( strFullPath );
//		}
//
//		if ( PRL_FAILED(err) )
//		{
//			if ( (	PRL_ERR_DISK_XML_INVALID == err ||
//				PRL_ERR_DISK_XML_OPEN_FAILED == err) &&
//				CFileHelper::ClearAndDeleteDir(strFullPath)
//				)
//			{
//				bRemoveSuccess = TRUE;
//			}
//			else
//			{
//				// PRL_ERR_DISK_IDENTIFY_FAILED == err
//
//				// Check, did we attempt open directory or file?
//				QFileInfo fileInfo;
//				fileInfo.setFile(strFullPath);
//
//				// is file exists?
//				if (	fileInfo.exists() &&
//					fileInfo.isFile() &&
//					QFile::remove( fileInfo.absoluteFilePath() )
//					)
//				{
//					bRemoveSuccess = TRUE;
//				}
//			}
//
//			if ( !bRemoveSuccess )
//			{
//				WRITE_TRACE(DBG_FATAL,
//					"Can't remove disk image [%s] by error %x.",
//					QSTR2UTF8( strFullPath ), err );
//
//				getLastError()->addEventParameter(
//					new CVmEventParameter( PVE::String,
//					strFullPath,
//					EVT_PARAM_MESSAGE_PARAM_0));
//
//				getLastError()->addEventParameter(
//					new CVmEventParameter( PVE::String,
//					QString("%1").arg( err ),
//					EVT_PARAM_MESSAGE_PARAM_1));
//				throw PRL_ERR_CANT_DELETE_FILE;
//			}
//		}
//	}
//	catch( PRL_RESULT err )
//	{
//		ret_value = err;
//	}
//
//	return ret_value;
//}

PRL_RESULT Task_CreateImage::hddStep2_CheckConditions( const CVmHardDisk& hard_disk )
{
	PRL_RESULT ret_value = PRL_ERR_SUCCESS;
	try
	{
		QString strFullPath = ConvertToFullPath(hard_disk.getUserFriendlyName());

		// Type of the disk
		PRL_IMAGE_TYPE DiskType = getHddDiskType( hard_disk );

		QString strDir = CFileHelper::GetFileRoot(strFullPath);

		// Check directory

		if (!CFileHelper::DirectoryExists(strDir, &getClient()->getAuthHelper()))
		{
			if (!CFileHelper::WriteDirectory(strDir, &getClient()->getAuthHelper()))
			{
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String,
					strDir,
					EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_MAKE_DIRECTORY;
			}
		}
		else
		{
			if( ! CFileHelper::DirCanWrite( strDir, &getClient()->getAuthHelper() ) )
			{
				WRITE_TRACE(DBG_FATAL, "User hasn't access to parent directory of new disk image[%s].",
					QSTR2UTF8( strDir ) );
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String,
					strFullPath,
					EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_ACCES_DENIED_FILE_TO_PARENT_PARENT_DIR;
			}
		}

		// checks on FAT file systems max hdd size
		PRL_UINT64 uiHddSize = hard_disk.getSize();

		if( !uiHddSize )
		{
			WRITE_TRACE( DBG_FATAL, "Wrong size %lld to create hdd.", uiHddSize );
			throw PRL_ERR_CREATE_HARD_DISK_WITH_ZERO_SIZE;
		}

		PRL_RESULT ret = PRL_ERR_SUCCESS;

		if(!hard_disk.isSplitted())
			ret = checkOnFATSupport( strFullPath, uiHddSize );

		if ( ret != PRL_ERR_SUCCESS )
			throw ret;

		// Check if required disk size is more then disk free space if the disk is plain
		quint64 nFreeSpace = 0;
		QString sNewVmLocation = QFileInfo(strFullPath).path();
		ret = CFileHelper::GetDiskAvailableSpace( sNewVmLocation, &nFreeSpace );
		if ( ret == PRL_ERR_GET_DISK_FREE_SPACE_FAILED )
		{
			if ( !getForceQuestionsSign() )//If interactive mode then send question to user
			{
				QList<PRL_RESULT> lstChoices;
				lstChoices.append( PET_ANSWER_YES );
				lstChoices.append( PET_ANSWER_NO );

				QList<CVmEventParameter*> lstParams;
				lstParams.append(new CVmEventParameter(PVE::String,
					m_pVmConfig->getVmIdentification()->getVmUuid(),
					EVT_PARAM_VM_UUID )
					);

				PRL_RESULT nAnswer = getClient()
					->sendQuestionToUser( PRL_QUESTION_CAN_NOT_GET_DISK_FREE_SPACE
						, lstChoices, lstParams, getRequestPackage() );

				if( nAnswer != PET_ANSWER_YES )
				{
					throw PRL_ERR_OPERATION_WAS_CANCELED;
				}
			}
		}
		else
		{
			if ( PRL_FAILED(ret) )
			{
				if (ret == PRL_ERR_INCORRECT_PATH)
				{
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, sNewVmLocation, EVT_PARAM_MESSAGE_PARAM_0));
				}
				else
				{
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, strFullPath, EVT_PARAM_MESSAGE_PARAM_0));
				}
				throw ret;
			}

			PRL_UINT64 nFreeSpaceInMb = ( PRL_UINT64 )( ((nFreeSpace)/1024)/1024 );
			uiHddSize = (DiskType == PRL_IMAGE_PLAIN) ?
					 hard_disk.getSize() : DISKSPACE_RESERVATION_MB;
			if ( uiHddSize >= nFreeSpaceInMb )
			{
				if (!getForceQuestionsSign())//If interactive mode then send question to user
				{
					QList<PRL_RESULT> lstChoices;
					lstChoices.append( PET_ANSWER_YES );
					lstChoices.append( PET_ANSWER_NO );

					QList<CVmEventParameter*> lstParams;
					lstParams.append(new CVmEventParameter(PVE::String,
						m_pVmConfig->getVmIdentification()->getVmUuid(),
						EVT_PARAM_VM_UUID )
						);

					PRL_RESULT nAnswer = getClient()
						->sendQuestionToUser( PET_QUESTION_FREE_SIZE_FOR_COMPRESSED_DISK
							, lstChoices, lstParams,getRequestPackage());

					if( nAnswer != PET_ANSWER_YES )
					{
						throw PRL_ERR_OPERATION_WAS_CANCELED;
					}
				} else {
					WRITE_TRACE(DBG_FATAL, "Task_CreateImage: There is not enough disk free space! needed=%lluMb free=%lluMb",
							uiHddSize, nFreeSpaceInMb);
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,
						QString::number(nFreeSpaceInMb),
						EVT_PARAM_MESSAGE_PARAM_0));
					throw PRL_ERR_NOT_ENOUGH_DISK_FREE_SPACE;
				}
			}
		}// else if ( ret == PRL_ERR_GET_DISK_FREE_SPACE_FAILED )
	}
	catch( PRL_RESULT err )
	{
		ret_value = err;
	}

	return ret_value;
}

// VirtualDisk commented out by request from CP team
//PRL_RESULT Task_CreateImage::hddStep3_PrepareParameters(
//	const CVmHardDisk& hard_disk
//	, PARALLELS_DISK_PARAMETERS& diskParameters )
//{
//	QString strFullPath = ConvertToFullPath(hard_disk.getUserFriendlyName());
//
//	// Type of the disk
//	PRL_IMAGE_TYPE DiskType = getHddDiskType( hard_disk );
//	// Part size, for splitted disks
//	PRL_UINT64 ulPartSize = 0;
//	// Parts count
//	PRL_UINT64 uiPartsCount = 0;
//
//	// Parameters for storages
//	STORAGE_PARAMETERS Storage;
//	PRL_UINT64 uiHddSize = hard_disk.getSize(); // in megabytes
//	// <<20 then >>9 i.e. to bytes, to sectors
//	uiHddSize <<= 11; // in sectors
//
//	// Initialize disk parameters
//	diskParameters.clear();
//
//	diskParameters.m_BlockSize = hard_disk.getBlockSize() != 0 ?
//			hard_disk.getBlockSize() : IDisk::GetDefaultBlockSize();
//	diskParameters.m_SizeInSectors = uiHddSize;
//	// Round up disk size
//	diskParameters.m_SizeInSectors /= diskParameters.m_BlockSize;
//	diskParameters.m_SizeInSectors *= diskParameters.m_BlockSize;
//
//	IDiskImage::CHSData CHS;
//	IDiskImage::ConvertToCHS(diskParameters.m_SizeInSectors, CHS);
//
//	diskParameters.m_Heads = CHS.Heads;
//	diskParameters.m_Sectors = CHS.Sectors;
//	diskParameters.m_Cylinders = CHS.Cylinders;
//	diskParameters.m_PhysSectorSize = PRL_PHYS_SECTOR_4096;
//
//	// Set global padding value
//	diskParameters.m_Padding = getPaddingValue();
//
//	if (hard_disk.isSplitted())
//		// Disk is splitted
//		ulPartSize = IDisk::GetSplitPartSize();
//	else
//		// Disk is not splitted -> part size = full disk size
//		ulPartSize = diskParameters.m_SizeInSectors;
//
//	// Count of parts
//	PRL_ASSERT( ulPartSize );
//	uiPartsCount = diskParameters.m_SizeInSectors / ulPartSize;
//	if ( uiPartsCount * ulPartSize < diskParameters.m_SizeInSectors )
//		uiPartsCount++;
//
//	for (PRL_UINT64 i = 0; i < uiPartsCount; i++)
//	{
//		// Initialize split part
//		Storage.uStart = ulPartSize * i;
//		if ( i == uiPartsCount - 1 )
//			Storage.uSize = diskParameters.m_SizeInSectors - ( ulPartSize * i );
//		else
//			Storage.uSize = ulPartSize;
//		Storage.Type = DiskType;
//		// Store
//		try {
//			diskParameters.m_Storages.push_back(Storage);
//		}
//		catch(std::bad_alloc&)
//		{
//			WRITE_TRACE(DBG_FATAL, "disk parameters pushback failure!");
//			return PRL_ERR_OUT_OF_MEMORY;
//		}
//	}
//
//	/*
//	* Check whether created image placed on SSD drive
//	* If true, we shall report SSD identifier within disk identify data
//	* Here we are building disk name and adding with "SSD" identifier string
//	*/
//	if ( CFileHelper::isSsdDrive(QFileInfo(strFullPath).absolutePath()) ) {
//		// Drop any old values
//		diskParameters.Name.clear();
//		diskParameters.Name = IDisk::BuildDiskName(strFullPath, diskParameters);
//		diskParameters.Name.append(" SSD");
//	}
//
//	return PRL_ERR_SUCCESS;
//}

// Remove image
PRL_RESULT Task_CreateImage::removeImage(const QString& /*strPath*/)
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT Err = IDisk::Remove( strPath );
//
//	if ( PRL_FAILED(Err) )
//	{
//		WRITE_TRACE(DBG_FATAL,
//			"Can't remove disk image [%s] by error %x.",
//			QSTR2UTF8(strPath), Err );
//
//		if ( !CFileHelper::ClearAndDeleteDir(strPath) )
//			WRITE_TRACE(DBG_FATAL,
//			"CFileHelper::ClearAndDeleteDir() can't remove disk image [%s].",
//			QSTR2UTF8(strPath) );
//	}
//
//	return Err;
	return PRL_ERR_FAILURE;
}

// VirtualDisk commented out by request from CP team
//PRL_RESULT Task_CreateImage::hddStep4_CreateImage(
//	const CVmHardDisk& hard_disk,
//	const PARALLELS_DISK_PARAMETERS& diskParameters
//	)
//{
//	// Error code for disk creation
//	PRL_RESULT DiskError = PRL_ERR_SUCCESS;
//	// Catchable error code
//	PRL_RESULT Err = PRL_ERR_SUCCESS;
//	bool NeedInit = false;
//	PRL_DISK_OPEN_FLAGS Flags = PRL_DISK_DEFAULT_FLAG;
//	PRL_UINT32 osType = m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion();
//	QString strFullPath = ConvertToFullPath(hard_disk.getUserFriendlyName());
//	// Pointer to disk interface
//	IDisk *pDisk = NULL;
//
//	// We are not interested in error
//	InitializeDisk(pDisk, osType, &NeedInit);
//
//	if (!NeedInit)
//		Flags |= PRL_DISK_XML_CHANGE;
//
//	pDisk = IDisk::CreateDisk(strFullPath, diskParameters, Flags,
//		HddCallbackToCreateImageTask, (void*)this, &DiskError);
//
//	// Wait inside.
//	Err = this->getHddErrorCode();
//
//	if (!pDisk ||
//		(DiskError != PRL_ERR_SUCCESS) ||
//		(Err == PRL_ERR_OPERATION_WAS_CANCELED) )
//	{
//		WRITE_TRACE(DBG_FATAL, "Can't create disk image [%s]", QSTR2UTF8( strFullPath ) );
//
//		removeImage(strFullPath);
//
//		if (Err == PRL_ERR_OPERATION_WAS_CANCELED)
//			return Err;
//
//		getLastError()->addEventParameter( new CVmEventParameter( PVE::String,
//			strFullPath,
//			EVT_PARAM_MESSAGE_PARAM_0));
//
//		if (DiskError == PRL_ERR_DISK_INSUFFICIENT_SPACE)
//			return PRL_ERR_CANT_CREATE_HDD_IMAGE_NO_SPACE;
//
//		getLastError()->addEventParameter( new CVmEventParameter( PVE::String,
//			QString("Internal error: %1").arg(PRL_RESULT_TO_STRING(DiskError)),
//			EVT_PARAM_DETAIL_DESCRIPTION));
//
//		return PRL_ERR_CANT_CREATE_HDD_IMAGE;
//	}
//
//	// Tune real disk space allocation on HFS
//	WRITE_TRACE(DBG_INFO, "Start to tune hdd" );
//	// Initialize disk
//	if (NeedInit)
//		DiskError = InitializeDisk( pDisk, osType, NULL);
//
//	// After disk is being created destroy previously created object
//	pDisk->Release();
//	pDisk = NULL;
//
//	if ( PRL_FAILED(DiskError) )
//	{
//		WRITE_TRACE(DBG_FATAL, "Couldn't to initialize disk image [%s]", QSTR2UTF8( strFullPath ) );
//		getLastError()->addEventParameter( new CVmEventParameter( PVE::String,
//			strFullPath,
//			EVT_PARAM_MESSAGE_PARAM_0));
//
//		getLastError()->addEventParameter( new CVmEventParameter( PVE::String,
//			QString("%1").arg(Prl::GetLastError()),
//			EVT_PARAM_DETAIL_DESCRIPTION));
//
//		return Err;
//	}
//
//	if( ! CDspAccessManager::setOwner ( strFullPath,  &getClient()->getAuthHelper(), true ) )
//	{
//		WRITE_TRACE(DBG_FATAL, "Can't change owner of disk image [%s]", QSTR2UTF8( strFullPath ));
//
//		if ( removeImage( strFullPath ) != PRL_ERR_SUCCESS )
//		{
//			WRITE_TRACE(DBG_FATAL, "Can't delete disk image [%s] by error %ld [%s]"
//				, QSTR2UTF8( strFullPath )
//				, Prl::GetLastError()
//				, QSTR2UTF8 ( Prl::GetLastErrorAsString() )
//				);
//			getLastError()->addEventParameter(
//				new CVmEventParameter( PVE::String,
//				strFullPath,
//				EVT_PARAM_MESSAGE_PARAM_0));
//			return PRL_ERR_CANT_DELETE_FILE;
//		}
//
//		getLastError()->addEventParameter(
//			new CVmEventParameter( PVE::String,
//			strFullPath,
//			EVT_PARAM_MESSAGE_PARAM_0));
//
//		return PRL_ERR_CANT_CHANGE_OWNER_OF_DISK_IMAGE_FILE;
//	}
//	return PRL_ERR_SUCCESS;
//}

PRL_IMAGE_TYPE Task_CreateImage::getHddDiskType( const CVmHardDisk& hard_disk )
{
	if (hard_disk.getDiskType() == PHD_EXPANDING_HARD_DISK)
		return PRL_IMAGE_COMPRESSED;

	return PRL_IMAGE_PLAIN;
}

/**
 * calculate padding to align read/writes
 *
 * @author antonz@
 *
 * @param Guest OS type
 *
 * @return Padding (1 sector for DOS/OS2/Win pre Vista)
 */
PRL_UINT32 Task_CreateImage::getPaddingValue()
{
	PRL_UINT32 OS = m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion();

	if (IS_MACOS(OS) ||
		IS_LINUX(OS) ||
		IS_OTHER(OS) ||
		IS_FREEBSD(OS))
	{
		return 0;
	}

	if (IS_OS2(OS) ||
		IS_MSDOS(OS) ||
		IS_NETWARE(OS) ||
		IS_SOLARIS(OS))
	{
		return 1;
	}

	// Only windows must be here
	if (!IS_WINDOWS(OS))
	{
		WRITE_TRACE(DBG_FATAL, "Unknown OS type (0x%x) The padding value can't be count properly", OS);
		return 0;
	}

	// Check the OS type and warn!
	if (((OS > PVS_GUEST_VER_WIN_2012 ) &&
		 (OS != PVS_GUEST_VER_WIN_OTHER)))
	{
		WRITE_TRACE(DBG_FATAL, "Unknown Windows OS (0x%x)", OS);
		return 0;
	}

	if ((OS == PVS_GUEST_VER_WIN_VISTA) ||
		(OS == PVS_GUEST_VER_WIN_2008) ||
		(OS == PVS_GUEST_VER_WIN_WINDOWS7) ||
		(OS == PVS_GUEST_VER_WIN_WINDOWS8) ||
		(OS == PVS_GUEST_VER_WIN_2012) ||
		(OS == PVS_GUEST_VER_WIN_WINDOWS8_1))
		return 0;

	return 1;
}
