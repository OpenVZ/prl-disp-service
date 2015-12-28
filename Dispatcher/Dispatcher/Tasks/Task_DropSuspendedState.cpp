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
///  Task_DropSuspendedState.cpp
///
/// @brief
///  Definition of the class CDspTaskHelper
///
/// @brief
///  This class implements long running tasks helper class
///
/// @author sergeyt
///  ilya@
///
////////////////////////////////////////////////////////////////////////////////

#include "CProtoSerializer.h"
#include "CDspClientManager.h"

#include "Task_DropSuspendedState.h"
#include "Task_DeleteSnapshot.h"

#include "Task_CommonHeaders.h"

//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include <prlcommon/PrlCommonUtilsBase/SysError.h>

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/StatesUtils/StatesHelper.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

Task_DropSuspendedState::Task_DropSuspendedState (
    SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
    const QString& vm_config)	:

	CDspTaskHelper(client, p),
	m_flgExclusiveOperationWasRegistred( false )
{
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration( vm_config ));
	setTaskParameters( m_pVmConfig->getVmIdentification()->getVmUuid() );
}

Task_DropSuspendedState::~Task_DropSuspendedState()
{
}

void Task_DropSuspendedState::setTaskParameters( const QString& vm_uuid )
{
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, vm_uuid, EVT_PARAM_DISP_TASK_VM_UUID ) );
}

QString Task_DropSuspendedState::getVmUuid()
{
    return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_DropSuspendedState::prepareTask()
{
    PRL_RESULT ret = PRL_ERR_SUCCESS;

    try
    {
        /**
         * check parameters
         */
        if( !IS_OPERATION_SUCCEEDED( m_pVmConfig->m_uiRcInit ) )
        {
            // send error to user: can't parse VM config
            getLastError()->setEventCode(PRL_ERR_CANT_PARSE_VM_CONFIG);
            throw PRL_ERR_PARSE_VM_CONFIG;
        }

		//https://bugzilla.sw.ru/show_bug.cgi?id=441619
		//In any case stop VM suspend at first
		const SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdVmSuspendCancel );
		CDspVm::cancelSuspend( getClient(), p, m_pVmConfig->getVmIdentification()->getVmUuid(), this );

		ret = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			m_pVmConfig->getVmIdentification()->getVmUuid(),
			getClient()->getVmDirectoryUuid(),
			( PVE::IDispatcherCommands ) getRequestPackage()->header.type,
			getClient(),
			this->getJobUuid() );
		if( PRL_FAILED( ret ) )
			throw ret;
		m_flgExclusiveOperationWasRegistred = true;


        /**
         * find VM in global VM hash
         */
        QString vm_uuid = m_pVmConfig->getVmIdentification()->getVmUuid();
		QString vm_home;
		{
			CDspLockedPointer<CVmDirectoryItem> pDirectoryItem =
				CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid( getClient(), vm_uuid);

			if ( ! pDirectoryItem )
			{
				// send error to user: VM with given UUID is not found
				getLastError()->setEventCode(PRL_ERR_VM_UUID_NOT_FOUND);
				throw PRL_ERR_VM_UUID_NOT_FOUND;
			}

			vm_home = pDirectoryItem->getVmHome();
			m_pVmConfig->getVmIdentification()->setHomePath(vm_home);

			//////////////////////////////////////////////////////////////////////////
			// check if user is authorized to access this VM
			//////////////////////////////////////////////////////////////////////////
			CDspAccessManager::VmAccessRights
				permissionToVm = CDspService::instance()->getAccessManager()
				.getAccessRightsToVm( getClient(), pDirectoryItem.getPtr() );

			if( ! permissionToVm.canWrite() )
			{
				PRL_RESULT err = permissionToVm.isExists()
					? PRL_ERR_ACCESS_TO_VM_DENIED
					: PRL_ERR_VM_CONFIG_DOESNT_EXIST;

				// send error to user: user is not authorized to access this VM
				WRITE_TRACE(DBG_FATAL, ">>> User hasn't rights to  access this VM %#x, %s"
					, err, PRL_RESULT_TO_STRING( err ) );
				getLastError()->setEventCode( err );
				getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,	pDirectoryItem->getVmName(),
												EVT_PARAM_MESSAGE_PARAM_0 ) );
				getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,	pDirectoryItem->getVmHome(),
												EVT_PARAM_MESSAGE_PARAM_1 ) );

				throw err;
			}
		} // end bracket for CDspLockedPointer<CVmDirectoryItem>

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		getLastError()->setEventCode( code );
		WRITE_TRACE(DBG_FATAL, "Error occurred while deleting state files [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

void Task_DropSuspendedState::finalizeTask()
{
	if( m_flgExclusiveOperationWasRegistred )
	{
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_pVmConfig->getVmIdentification()->getVmUuid(),
			getClient()->getVmDirectoryUuid(),
			( PVE::IDispatcherCommands ) getRequestPackage()->header.type,
			getClient() );
	}

	// Send response
	if ( PRL_FAILED( getLastErrorCode() ) )
	{
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
	{
		if (m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->getUndoDisksModeEx() != PUD_DISABLE_UNDO_DISKS)
		{
			PRL_RESULT outCreateError = PRL_ERR_SUCCESS;
			// FIXME!!! It needs to check bNew value  (#123497)
			bool bNew = false;
			// Create fake VM for shanpshot mech
			SmartPtr<CDspVm> pVm = CDspVm::CreateInstance(getVmUuid(), getClient()->getVmDirectoryUuid(),
				outCreateError, bNew, getClient(), PVE::DspCmdVmDeleteSnapshot);
			if (pVm.isValid())
			{
				// Delete shapshot package for undo disks mode
				CProtoCommandPtr pRequest = CProtoSerializer::CreateDeleteSnapshotProtoCommand(
												getVmUuid(),
												QString(UNDO_DISKS_UUID),
												true);
				SmartPtr<IOPackage> pPackage
					= DispatcherPackage::duplicateInstance(getRequestPackage(), pRequest->GetCommand()->toString());
				pPackage->header.type = PVE::DspCmdVmDeleteSnapshot;

				// Start delete snapshot task
				SmartPtr<CDspClient> pClient = getClient();
				return (void)CDspService::instance()->getTaskManager().schedule(
						new Task_DeleteSnapshot( pClient, pPackage, VMS_STOPPED ));
			}
			else
			{
				setLastErrorCode( outCreateError );
				getClient()->sendResponseError( getLastError(), getRequestPackage() );
			}
		}

		// Generate "VM Stopped" event
		sendEventStopped();

		if ( PRL_SUCCEEDED( getLastErrorCode() ) )
		{
			getClient()->sendSimpleResponse( getRequestPackage(), getLastErrorCode() );
		}
	}
}

PRL_RESULT Task_DropSuspendedState::run_body()
{
	PRL_RESULT ret = getLastErrorCode();

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
		QStringList strListToDelete;
		CStatesHelper sh(m_pVmConfig->getVmIdentification()->getHomePath());
		strListToDelete << sh.getSavFileName();

		QString strVmDirPath = CFileHelper::GetFileRoot( m_pVmConfig->getVmIdentification()->getHomePath() );

		QString strMemFileName;
		if ( sh.extractMemFileName(sh.getSavFileName(), strMemFileName) )
		{
			QString strMemFilePath = strVmDirPath + "/" + strMemFileName;
			strListToDelete << strMemFilePath;
		}

		QString strPngFile = strVmDirPath + "/" + PRL_VM_SUSPENDED_SCREEN_FILE_NAME;
		strListToDelete << strPngFile;

		if(!strListToDelete.isEmpty())
		{
			if(!RemoveStateFiles(strListToDelete,lstNotRemovedFiles))
			{
				WRITE_TRACE(DBG_FATAL, "Not all files can be deleted. !RemoveStateFiles()");

				if(lstNotRemovedFiles.size() == 2)
				{
					foreach ( const QString& path, lstNotRemovedFiles )
					{
						WRITE_TRACE(DBG_FATAL, "file wasn't delete. path = [%s]",  QSTR2UTF8(path) );
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, path,
							EVT_PARAM_RETURN_PARAM_TOKEN));
					}
					getLastError()->setEventCode(PRL_ERR_UNABLE_DROP_SUSPENDED_STATE);
					ret = PRL_ERR_UNABLE_DROP_SUSPENDED_STATE;
				}
			}
		}

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

		WRITE_TRACE(DBG_FATAL, "Error occurred while deleting state files with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
    }

    return ret;
}


// this function correctly removed files from list from input dir
bool Task_DropSuspendedState::RemoveStateFiles(const QStringList & strImagesList,QStringList & lstNotRemoved)
{
	lstNotRemoved.clear();
	bool bRes = true;
	for(int i = 0; i < strImagesList.size();i++ )
	{
		QString strCurImage = strImagesList[i];
		if (QFileInfo(strCurImage).isFile())
		{
			bool bCurDelete = QFile::remove(strCurImage);
			if(!bCurDelete)
				lstNotRemoved << strCurImage;
			bRes &= bCurDelete;

		}
	}
	return bRes;
}

void Task_DropSuspendedState::sendEventStopped()
{
	QString dirUuid = getClient()->getVmDirectoryUuid();
	QString vmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();


	QList< SmartPtr<CVmEvent> >  lstToSend;
	// Send to all VM client new VM state
	SmartPtr<CVmEvent> pEvent( new CVmEvent( PET_DSP_EVT_VM_STATE_CHANGED, vmUuid, PIE_DISPATCHER ) ) ;
	pEvent->addEventParameter(
		new CVmEventParameter (
			PVE::Integer
			, QString("%1").arg((int)VMS_STOPPED)
			, EVT_PARAM_VMINFO_VM_STATE
			)
		);
	lstToSend.append( pEvent );


	// THIS EVENT NEED ONLY TO PROVIDE COMPATIBILITY WITH OLD CLIENTS ( based on revision < 194322  )
	// BUG #122476
	pEvent = SmartPtr<CVmEvent> (
		new CVmEvent( PET_DSP_EVT_VM_STOPPED, vmUuid, PIE_DISPATCHER )
		);
	lstToSend.append( pEvent );

	foreach( pEvent, lstToSend )
	{
		SmartPtr<IOPackage>
			pUpdateVmStatePkg = DispatcherPackage::createInstance(
				PVE::DspVmEvent, pEvent.getImpl(), getRequestPackage()
				);

		CDspService::instance()->getClientManager().sendPackageToVmClients(
			pUpdateVmStatePkg,
			dirUuid,
			vmUuid);
	}
}
