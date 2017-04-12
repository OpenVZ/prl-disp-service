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
///  Task_DeleteVm.cpp
///
/// @brief
///  Definition of the class CDspTaskHelper
///
/// @brief
///  This class implements long running tasks helper class
///
/// @author sergeyt
///  SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "CDspClientManager.h"
#include "CDspBugPatcherLogic.h"

#include "Task_DeleteVm.h"

#include "Task_CommonHeaders.h"

//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include <prlcommon/PrlCommonUtilsBase/SysError.h>

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include "CDspBackupDevice.h"
#include "CDspVm_p.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

namespace
{
	const QStringList g_LIST_OF_PARALLELS_FILESGARBAGE_EXTENTION =
		( QStringList() << "*.mem" << "*.sav" <<"*.xml" << "*.txt" << "*.pvs" << QString("*%1")
										.arg(VM_INFO_FILE_SUFFIX) << "parallels.log*"
		<< PRL_VM_NVRAM_FILE_NAME
		<< PRL_VM_SUSPENDED_SCREEN_FILE_NAME
		<< "{*-*-*-*-*}.png" /* #125852 to support old suspended screenshot format ( temporally  )*/
		<< "*.pvs.lock" << "*.pvs.backup"
#if defined(_WIN_)
		<< "pvs*.tmp"
#else
		<< ".vmm*"
#endif
		);

const QStringList g_LIST_OF_PARALLELS_IMAGES_EXTENTION = ( QStringList() << "*.iso" << "*.hdd" << "*.fdd" );
const QString g_sSNAPSHOTS_DIR_NAME	= "Snapshots";

}

Task_DeleteVm::Task_DeleteVm (
    SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
    const QString& vm_config,
	PRL_UINT32 flags,
	const QStringList & strFilesToDelete)	:

	CDspTaskHelper(client, p),
	m_flags(flags),
	m_flgVmWasDeletedFromSystemTables(false),
	m_flgExclusiveOperationWasRegistred( false ),
	m_flgLockRegistred(false),
	m_pVmInfo(0)
{
	m_strListToDelete = strFilesToDelete;
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration( vm_config ));

	//////////////////////////////////////////////////////////////////////////
	setTaskParameters( m_pVmConfig->getVmIdentification()->getVmUuid() );
}

Task_DeleteVm::~Task_DeleteVm()
{
	delete m_pVmInfo;
}

void Task_DeleteVm::setTaskParameters( const QString& vm_uuid )
{
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, vm_uuid, EVT_PARAM_DISP_TASK_VM_DELETE_VM_UUID ) );
}

bool Task_DeleteVm::doUnregisterOnly()
{
    return m_flags & PVD_UNREGISTER_ONLY;
}

QString Task_DeleteVm::getVmUuid()
{
    return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_DeleteVm::prepareTask()
{
	CDspTaskFailure f(*this);
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		/**
		* check parameters
		*/
		if( !IS_OPERATION_SUCCEEDED( m_pVmConfig->m_uiRcInit ) )
		{
			// send error to user: can't parse VM config
			throw f(PRL_ERR_CANT_PARSE_VM_CONFIG);
		}

		m_vmDirectoryUuid = CDspVmDirHelper::getVmDirUuidByVmUuid(
		m_pVmConfig->getVmIdentification()->getVmUuid(), getClient());
		if (m_vmDirectoryUuid.isEmpty())
			throw PRL_ERR_VM_UUID_NOT_FOUND;

		//https://bugzilla.sw.ru/show_bug.cgi?id=441619
		if ( doUnregisterOnly() )
		{
			//In unregister VM case we should to wait until suspending sync state completed
			QString sVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();
			VIRTUAL_MACHINE_STATE nInitialState = CDspVm::getVmState( sVmUuid, m_vmDirectoryUuid );
			if ( VMS_SUSPENDING_SYNC == nInitialState )
				WRITE_TRACE(DBG_FATAL, "Started wait for VM suspending sync phase");
			while ( VMS_SUSPENDING_SYNC == CDspVm::getVmState( sVmUuid, m_vmDirectoryUuid ) )
			{
				HostUtils::Sleep(1000);
				if ( operationIsCancelled() )//Task was cancelled - terminate operation
					throw PRL_ERR_OPERATION_WAS_CANCELED;
			}
			if ( VMS_SUSPENDING_SYNC == nInitialState )
				WRITE_TRACE(DBG_FATAL, "Finished wait for VM suspending sync phase");
		}

	// TODO fix lock managment and remove this state check
	if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
	{
		VIRTUAL_MACHINE_STATE s;
		Libvirt::Result r = Libvirt::Kit.vms().at(getVmUuid()).getState().getValue(s);
		if (r.isSucceed() && s != VMS_STOPPED && s != VMS_SUSPENDED)
			return f(PRL_ERR_DISP_VM_IS_NOT_STOPPED, getVmUuid());
	}
	if (!(m_flags & PVD_SKIP_VM_OPERATION_LOCK))
	{
		ret = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			m_pVmConfig->getVmIdentification()->getVmUuid(),
			m_vmDirectoryUuid,
			( PVE::IDispatcherCommands ) getRequestPackage()->header.type,
			getClient() );
		if( PRL_FAILED( ret ) )
			throw ret;
		m_flgExclusiveOperationWasRegistred = true;
	}


        /**
         * find VM in global VM hash
         */
        QString vm_name = m_pVmConfig->getVmIdentification()->getVmName();
        QString vm_uuid = m_pVmConfig->getVmIdentification()->getVmUuid();

		{
			CDspLockedPointer<CVmDirectoryItem> pDirectoryItem =
				CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid( m_vmDirectoryUuid, vm_uuid);

			if ( ! pDirectoryItem )
			{
				// send error to user: VM with given UUID is not found
				throw f(PRL_ERR_VM_UUID_NOT_FOUND);
			}

			m_sVmHomePath = pDirectoryItem->getVmHome();

			ret = checkUserAccess( pDirectoryItem );
			if (PRL_FAILED( ret ))
				throw ret;

		} // end bracket for CDspLockedPointer<CVmDirectoryItem>


		//LOCK vm directory before deleting and locking.
		CDspLockedPointer< CVmDirectory >
			pLockedVmDir = CDspService::instance()->getVmDirManager()
				.getVmDirectory(m_vmDirectoryUuid);


        /**
         * check if such VM is not already registered in user's prepare_VM directory
         */
        m_pVmInfo =  new CVmDirectory::TemporaryCatalogueItem(vm_uuid, m_sVmHomePath, vm_name);
        PRL_ASSERT (m_pVmInfo);

        m_flgLockRegistred=false;

		PRL_RESULT lockResult = CDspService::instance()->getVmDirManager()
			.lockExistingExclusiveVmParameters(m_vmDirectoryUuid, m_pVmInfo);

		if (!PRL_SUCCEEDED(lockResult))
		{
			throw f.setToken(m_pVmInfo->vmUuid).setToken(m_pVmInfo->vmXmlPath)
				.setToken(m_pVmInfo->vmName)(lockResult);
		}

		m_flgLockRegistred=true;

		//////////////////////////////////////////////////////////////////////////
		// remove VM from VM Directory
		//////////////////////////////////////////////////////////////////////////
		ret = CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(
				m_vmDirectoryUuid, vm_uuid);
		if ( ! PRL_SUCCEEDED( ret ) )
		{
			WRITE_TRACE(DBG_FATAL, ">>> Can't delete vm from VmDirectory by error %#x, %s",
				ret, PRL_RESULT_TO_STRING( ret) );
			throw ret;
		}
		if (!(m_flags & PVD_SKIP_HA_CLUSTER))
			CDspService::instance()->getHaClusterHelper()->
					removeClusterResource(m_pVmInfo->vmName);

		ret = PRL_ERR_SUCCESS;
		m_flgVmWasDeletedFromSystemTables=true;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while deleting VM configuration with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}
	getLastError()->setEventCode( ret );

	return ret;
}

void Task_DeleteVm::finalizeTask()
{
	QString sVmUuid = m_pVmConfig
		? m_pVmConfig->getVmIdentification()->getVmUuid()
		: "";

	if (PRL_SUCCEEDED(getLastErrorCode()))
		CDspBugPatcherLogic::cleanVmPatchMarks(m_vmDirectoryUuid, sVmUuid);

	if( doUnregisterOnly() && !(m_flags & PVD_NOT_MODIFY_VM_CONFIG) && PRL_SUCCEEDED(getLastErrorCode()) )
	{
		CDspVmDirManager::VmDirItemsHash
			sharedVmHash = CDspService::instance()->getVmDirManager()
				.findVmDirItemsInCatalogue( sVmUuid ,m_sVmHomePath );

		QString server_id(m_pVmConfig->getVmIdentification()->getServerUuid());
		QString local_id(CDspService::instance()->getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid());
		if(Uuid(server_id) == Uuid(local_id) && sharedVmHash.isEmpty() )
		{
			m_pVmConfig->getVmIdentification()->setServerUuid();

			// fix #121634, #121636 - Skip Copy/Move message when VM already registered on same server
			m_pVmConfig->getVmIdentification()->setLastServerUuid(local_id);
		}

		// if vm is in invalid state do not save it
		PRL_RESULT validRc = (PRL_RESULT)m_pVmConfig->getValidRc();
		if ( 	validRc != PRL_ERR_PARSE_VM_CONFIG
			&& validRc != PRL_ERR_VM_CONFIG_DOESNT_EXIST
			&& validRc != PRL_ERR_ACCESS_TO_VM_DENIED // #427453
		)
		{
			m_pVmConfig->setValidRc( PRL_ERR_SUCCESS );
			CDspService::instance()->getVmConfigManager().saveConfig(m_pVmConfig,
				m_sVmHomePath,
				getClient(),
				true,
				true);
		}
	}

	if( PRL_SUCCEEDED(getLastErrorCode()) )
	{
		CDspService::instance()->getVmConfigManager().getHardDiskConfigCache().remove( m_pVmConfig );
		CDspService::instance()->getVmConfigManager().removeFromCache( m_pVmInfo->vmXmlPath );
	}

	if( m_flgExclusiveOperationWasRegistred )
	{
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_pVmConfig->getVmIdentification()->getVmUuid(),
			m_vmDirectoryUuid,
			( PVE::IDispatcherCommands ) getRequestPackage()->header.type,
			getClient() );
	}

	// delete temporary registration
	if (m_pVmInfo && m_flgLockRegistred)
	{
		CDspService::instance()->getVmDirManager()
			.unlockExclusiveVmParameters(m_vmDirectoryUuid, m_pVmInfo);
	}

	postVmDeletedEvent();

	// send response
	if ( PRL_FAILED( getLastErrorCode() ) )
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	else
		getClient()->sendSimpleResponse( getRequestPackage(), getLastErrorCode() );
}

PRL_RESULT Task_DeleteVm::run_body()
{
	PRL_RESULT ret = getLastErrorCode();
	if (PRL_SUCCEEDED(ret) && !m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
	{
#ifdef _LIBVIRT_
		Libvirt::Result r(Libvirt::Kit.vms().at(m_pVmConfig->getVmIdentification()->getVmUuid())
				.getState().undefine());
		ret = (r.isFailed()? r.error().code() : PRL_ERR_SUCCESS);
#endif // _LIBVIRT_
	}
	if ( doUnregisterOnly() )
	{
		Backup::Device::Service(m_pVmConfig)
			.setVmHome(CFileHelper::GetFileRoot(m_sVmHomePath))
			.disable();
		setLastErrorCode(ret);
		return ret;
	}

	CDspTaskFailure f(*this);
	bool flgImpersonated = false;
	try
	{
		// Let current thread impersonate the security context of a logged-on user
		if( ! getClient()->getAuthHelper().Impersonate() )
			throw PRL_ERR_IMPERSONATE_FAILED;
		flgImpersonated = true;

		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

		/**
		* remove VM resources from disk (if required)
		*/

		QStringList lstNotRemovedFiles;
		QString strVmHomeDir = CFileHelper::GetFileRoot(m_sVmHomePath);

		if(!m_strListToDelete.isEmpty())
		{
			if(!RemoveListOfFiles(m_strListToDelete,
				lstNotRemovedFiles,
				strVmHomeDir,
				false)
				)
			{
				WRITE_TRACE(DBG_FATAL, ">>> Not all files can be delete. !removeVmResources( pVmConfig )");

				if(!lstNotRemovedFiles.isEmpty())
				{
					foreach ( const QString& path, lstNotRemovedFiles )
					{
						WRITE_TRACE(DBG_FATAL, "file wasn't delete. path = [%s]",  QSTR2UTF8(path) );
						f.setToken(path);
					}
					ret = f(PRL_ERR_NOT_ALL_FILES_WAS_DELETED);
				}
			}

		}
		else
		do {
			Backup::Device::Service(m_pVmConfig)
				.setVmHome(CFileHelper::GetFileRoot(m_sVmHomePath))
				.teardown();
			// for server mode delete all files from vm directory #270686
			// common logic for console clients such as prlctl for all modes #436939
			{
				PRL_ASSERT(QFileInfo(strVmHomeDir).isDir());
				if ( QFileInfo(strVmHomeDir).isDir() && CFileHelper::ClearAndDeleteDir( strVmHomeDir ) )
					break;

				ret = f(PRL_ERR_NOT_ALL_FILES_WAS_DELETED);
			}

			if(!removeVmResources( m_pVmConfig, lstNotRemovedFiles))
			{
				WRITE_TRACE(DBG_FATAL, ">>> Not all files can be delete. !removeVmResources( pVmConfig )");

				if(!lstNotRemovedFiles.isEmpty())
				{
					foreach ( const QString& path, lstNotRemovedFiles )
					{
						WRITE_TRACE(DBG_FATAL, "file wasn't delete. path = [%s]",  QSTR2UTF8(path) );
						f.setToken(path);
					}
					ret = f(PRL_ERR_NOT_ALL_FILES_WAS_DELETED);
				}
			} //if(!removeVmResources

			/**
			* remove VM configuration file
			*/


			QFile config_file( m_sVmHomePath );
			if ( CFileHelper::FileExists(m_sVmHomePath, &getClient()->getAuthHelper())
					&& !config_file.remove() )
			{
				WRITE_TRACE(DBG_FATAL, ">>> Not all files can be delete. can't remove config_file [%s] by error %ld [%s]"
					, QSTR2UTF8( m_sVmHomePath )
					, Prl::GetLastError()
					, QSTR2UTF8( Prl::GetLastErrorAsString() )
					);

				ret = f(PRL_ERR_NOT_ALL_FILES_WAS_DELETED);
			}

			// finally remove all prl file entries

			if (QFileInfo(strVmHomeDir).isDir())
			{
				// clear garbage - this is not handling error massage!
				removeGarbageDirs(strVmHomeDir);
				removeGarbageFiles(strVmHomeDir);

				// search prl files in vm home directory
				QStringList list;
				searchParallelsImagesInsideVmHome(strVmHomeDir,list);
				list << strVmHomeDir; // add vm dir to deleting list
				if(!RemoveListOfFiles(list,lstNotRemovedFiles,strVmHomeDir))
				{
					WRITE_TRACE(DBG_FATAL, ">>> Not all files can be delete. !removeVmResources( pVmConfig )");

					if(!lstNotRemovedFiles.isEmpty())
					{
						foreach ( const QString& path, lstNotRemovedFiles )
						{
							WRITE_TRACE(DBG_FATAL, "file wasn't delete. path = [%s]",  QSTR2UTF8(path) );
							f.setToken(path);
						}
						ret = f(PRL_ERR_NOT_ALL_FILES_WAS_DELETED);
					}
				}
			}

		}while(0);

		//////////////////////////////////////////////////////////////////////////
		// Terminates the impersonation of a user
		if( ! getClient()->getAuthHelper().RevertToSelf() ) // Don't throw because this thread already finished.
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

	}
	catch (PRL_RESULT code)
	{
		ret = code; //PRL_ERR_OPERATION_FAILED;

		if( flgImpersonated && ! getClient()->getAuthHelper().RevertToSelf() )
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

		WRITE_TRACE(DBG_FATAL, "Error occurred while deleting VM configuration with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	/**
	* finalize and cleanup
	*/
	setLastErrorCode(ret);
	return ret;

}

template<class T>
void Task_DeleteVm::removeDevices( const QList<T*> & lstDevices, QStringList& outLstNotRemovedFiles)
{
	for( int iNdex = 0; iNdex < lstDevices.size(); iNdex++ )
	{
		T* pDevice = lstDevices.at( iNdex );
		if(( (int)pDevice->getEmulatedType() == (int)PDT_USE_IMAGE_FILE) ||
			((int)pDevice->getEmulatedType() == (int)PDT_USE_OUTPUT_FILE))
		{
			// outside vm home don't removed
			// skip not existing files
			QString strDevPath = pDevice->getUserFriendlyName();
			if( strDevPath.isEmpty() ||
				!CFileHelper::FileExists( strDevPath, &getClient()->getAuthHelper()) ||
				!isFileInsideVmHome(strDevPath,m_sVmHomePath))
				continue;

			QFile resource_file( strDevPath );
			if (!CFileHelper::FileCanWrite( strDevPath,
				&getClient()->getAuthHelper())
				|| !resource_file.remove() )
			{
				outLstNotRemovedFiles.append( strDevPath );
				WRITE_TRACE(DBG_FATAL, "file [%s] not removed by error %ld [%s]"
					, QSTR2UTF8( strDevPath )
					, Prl::GetLastError()
					, QSTR2UTF8( Prl::GetLastErrorAsString() )
					);

			}
		}
	}
}

// Remove VM resources from disk
bool Task_DeleteVm::removeVmResources( const SmartPtr<CVmConfiguration>& p_VmConfig,
									  QStringList& outLstNotRemovedFiles
									  )
{
	outLstNotRemovedFiles.clear();

	if (!p_VmConfig)
	{
		WRITE_TRACE(DBG_FATAL, "p_VmConfig==0");
		return false;
	}


	/**
	* get VM hardware list accessor
	*/

	CVmHardware* p_VmHardware = p_VmConfig->getVmHardwareList();
	if( !p_VmHardware )
		return false;

	/**
	* process floppy disk images
	*/

	removeDevices(p_VmHardware->m_lstFloppyDisks, outLstNotRemovedFiles );

	/**
	* process hard disk images
	*/
	for( int iNdex = 0; iNdex < p_VmHardware->m_lstHardDisks.size(); iNdex++ )
    {
        CVmHardDisk* pDevice = p_VmHardware->m_lstHardDisks.at( iNdex );
        if( ( pDevice->getEmulatedType() == PVE::HardDiskImage ) )
        {
            // task #603 [Change mechanism of delete Hdd/cdrom/floppy when user remove VM]
            //      hdd images outside vm catalogue don't removed
			QString strDevPath = pDevice->getUserFriendlyName();
			if(strDevPath.isEmpty() ||
				!CFileHelper::FileExists( strDevPath, &getClient()->getAuthHelper()) ||
				!isFileInsideVmHome( strDevPath, m_sVmHomePath))
				continue;


            QFile resource_file( strDevPath );

			//if (!CFileHelper::FileCanWrite( pDevice->getUserFriendlyName(), getUser()->getAuthHelper())
			//  || !resource_file.remove() )
			// FIXME: Friendly? Really? May be system ?

// VirtualDisk commented out by request from CP team
//            PRL_RESULT err = IDisk::Remove( strDevPath );
//            if  ( PRL_ERR_SUCCESS != err )
//            {
//                if(!CFileHelper::ClearAndDeleteDir( strDevPath ))
//				{
//					outLstNotRemovedFiles.append( strDevPath );
//					WRITE_TRACE(DBG_FATAL, "file [%s] not removed by error %d"
//						, QSTR2UTF8( strDevPath )
//						, err
//						);
//				}
//			}
		}
		else
		{
// VirtualDisk commented out by request from CP team
//			PRL_RESULT err = IDisk::Remove( pDevice->getSystemName() );
//			if  ( PRL_ERR_SUCCESS != err )
//			{
//				outLstNotRemovedFiles.append( pDevice->getSystemName() );
//				WRITE_TRACE(DBG_FATAL, "file [%s] not removed by error %d"
//					, QSTR2UTF8( pDevice->getSystemName() )
//					, err
//					);
//			}
		}
	}

	/**
	* process CD/DVD-ROMs disk images
	*/

	removeDevices( p_VmHardware->m_lstOpticalDisks, outLstNotRemovedFiles );

	/**
	* process serial ports output files
	*/

	removeDevices( p_VmHardware->m_lstSerialPorts, outLstNotRemovedFiles );

	/**
	* process parallel ports output files
	*/

	removeDevices( p_VmHardware->m_lstParallelPorts, outLstNotRemovedFiles );

	/**
     * following items have no any output files, which can be deleted
     */

	// network adapters
	//p_VmHardware->m_lstNetworkAdapters

	// sound devices
	//p_VmHardware->m_lstSoundDevices

	// USB controllers
	//p_VmHardware->m_lstUsbDevices
	return outLstNotRemovedFiles.isEmpty();

}

void Task_DeleteVm::removeGarbageFiles(const QString & strDir)
{
	QDir	cDir(strDir);
	cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
	cDir.setNameFilters( g_LIST_OF_PARALLELS_FILESGARBAGE_EXTENTION );
	cDir.setSorting(QDir::Name | QDir::DirsLast);
	QFileInfoList cFileList = cDir.entryInfoList();

	for(int i = 0; i < cFileList.size();i++ )
	{
		if(cFileList.at(i).isFile())
		{
			QString strFileExtention = cFileList.at(i).suffix();
			// check file extention
			if(strFileExtention == "xml")
			{
				if (cFileList.at(i).baseName() != g_sSNAPSHOTS_DIR_NAME )
					continue;
			}
			QFile::remove(cFileList.at(i).filePath());
		}

	}

}

// this function search prl dirs in input directory and remove it
bool Task_DeleteVm::removeGarbageDirs(const QString & strDir)
{
	// clear garbage from directory
	// if we have temporal directory "Windows Applications"
	// form dir path
	bool bRes = true;

	QString strDirForClear = ParallelsDirs::getMappingApplicationsDir(strDir);
	bRes &= CFileHelper::ClearAndDeleteDir(strDirForClear);
	strDirForClear = ParallelsDirs::getMappingDisksDir(strDir);
	bRes &= CFileHelper::ClearAndDeleteDir(strDirForClear);
	strDirForClear = ParallelsDirs::getSnapshotsDir(strDir);
	bRes &= CFileHelper::ClearAndDeleteDir(strDirForClear);
	return bRes;
}

// this function searches .hdd,.fdd and .iso files from vm dir
bool Task_DeleteVm::searchParallelsImagesInsideVmHome(const QString & strDir,
													  QStringList & strImagesList)
{
	QDir	cDir(strDir);
	cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
	cDir.setNameFilters( g_LIST_OF_PARALLELS_IMAGES_EXTENTION );
	cDir.setSorting(QDir::Name | QDir::DirsLast);
	QFileInfoList cFileList = cDir.entryInfoList();

	for(int i = 0; i < cFileList.size();i++ )
	{
		if(CFileHelper::FileCanWrite(cFileList.at(i).filePath(),
			&getClient()->getAuthHelper())
			)
			strImagesList.append(cFileList.at(i).filePath());
	}
	return !strImagesList.isEmpty();
}
// this function correctly removed files from list and delete vm directory at the end
bool Task_DeleteVm::RemoveListOfFiles(const QStringList & strImagesList,
									  QStringList & lstNotRemoved,
									  const QString & strVmDir,
									  bool bPostProgressEvents)
{
	lstNotRemoved.clear();
	bool isNeedToDeleteVmDir = false;
	bool bRes = true;
	for(int i = 0; i < strImagesList.size();i++ )
	{
		if (bPostProgressEvents)
		{
			postDeleteProgressEvent(i*(100/strImagesList.size()));
			HostUtils::Sleep(100);
		}

		QString strCurImage = strImagesList[i];
		QFileInfo curFileInfo(strCurImage);
		// skip not existing files
		if (!CFileHelper::FileExists(strCurImage,&getClient()->getAuthHelper()))
			continue;

		if (curFileInfo.isFile())
		{
			bool bCurDelete = QFile::remove(strCurImage);
			if(!bCurDelete)
				lstNotRemoved << strCurImage;
			bRes &= bCurDelete;

		}
		else //
		{
			if (strVmDir == strCurImage)// vm dir must be deleted at the end
			{
				isNeedToDeleteVmDir = true;
				continue;
			}
			// try to delete incoming directory if it internal directory of vm dir
			if(curFileInfo.isDir())
			{
				// check if parent == vm directory
				if (strCurImage.contains(".hdd"))// for hdd deleting - only it may be out of vm dir
				{
// VirtualDisk commented out by request from CP team
//					PRL_RESULT err = IDisk::Remove(strCurImage);
//					if( err != PRL_ERR_SUCCESS )
//					{
//						WRITE_TRACE(DBG_FATAL, "IDisk::Remove %s failed - error %d"
//							, QSTR2UTF8( strCurImage )
//							, err
//							);
//						// try to delete invalid disk image
//						if(!CFileHelper::ClearAndDeleteDir(strCurImage))
//						{
//							bRes = false;
//							lstNotRemoved << strImagesList[i];
//						}
//					}
				}
				else
				{
					// delete only child directory to prevent damage
					// #431558 compare paths by spec way to prevent errors with symlinks, unexisting files, ...
					if(CFileHelper::IsPathsEqual(curFileInfo.dir().path(), strVmDir))

					{
						if (!CFileHelper::ClearAndDeleteDir(strCurImage))
						{
							bRes = false;
							lstNotRemoved << strImagesList[i];
						}
					}
				}
			}
		}
	}

	if(isNeedToDeleteVmDir)
	{
		QDir().rmdir(strVmDir);
	}
	if (bPostProgressEvents)
		postDeleteProgressEvent(100);
	return bRes;
}

/**
* check if user is authorized to access this VM
*/
PRL_RESULT Task_DeleteVm::checkUserAccess( CDspLockedPointer<CVmDirectoryItem> pDirectoryItem )
{
	if ( ! doUnregisterOnly() )
	{
		CDspAccessManager::VmAccessRights
			permissionToVm = CDspService::instance()->getAccessManager()
			.getAccessRightsToVm( getClient(), pDirectoryItem.getPtr() );

		if( ! permissionToVm.canWrite() )
		{
			PRL_RESULT err = permissionToVm.isExists()
				? PRL_ERR_ACCESS_TO_VM_DENIED
				: PRL_ERR_VM_CONFIG_DOESNT_EXIST;

			if (err != PRL_ERR_VM_CONFIG_DOESNT_EXIST)
			{

				// send error to user: user is not authorized to access this VM
				WRITE_TRACE(DBG_FATAL, ">>> User hasn't rights to  access this VM %#x, %s"
					, err, PRL_RESULT_TO_STRING( err ) );
				CDspTaskFailure f(*this);
				if (!pDirectoryItem.isValid())
					return f(err);

				return f.setCode(err)
					(pDirectoryItem->getVmName(), pDirectoryItem->getVmHome());
			}
		}
	} // if ( ! doUnregisterOnly() )

	return PRL_ERR_SUCCESS;
}

/**
* Notify all users that VM was removed
*/
void Task_DeleteVm::postVmDeletedEvent()
{
	if (m_flgVmWasDeletedFromSystemTables ) //&& sharedVmHash.isEmpty() )
	{
		DspVm::vdh().sendVmRemovedEvent(
			MakeVmIdent(m_pVmConfig->getVmIdentification()->getVmUuid(), m_vmDirectoryUuid),
			(doUnregisterOnly()) ? PET_DSP_EVT_VM_UNREGISTERED : PET_DSP_EVT_VM_DELETED,
			 getRequestPackage());
	}

}

/**
* Notify deletion caller about progress
*/
void Task_DeleteVm::postDeleteProgressEvent(uint uiProgress)
{
	// Create event for client
	CVmEvent event(	PET_JOB_DELETE_VM_PROGRESS_CHANGED,
		Uuid().toString(),
		PIE_DISPATCHER );

	/**
	* Add event parameters
	*/

	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
		QString::number(uiProgress),
		EVT_PARAM_PROGRESS_CHANGED));


	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event,
		getRequestPackage());

	getClient()->sendPackage( p );
}
