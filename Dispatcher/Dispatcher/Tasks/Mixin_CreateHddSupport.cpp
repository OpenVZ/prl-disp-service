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
///	Mixin_CreateHddSupport.h
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

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "Mixin_CreateHddSupport.h"
#include "Task_CommonHeaders.h"

#include <prlcommon/PrlCommonUtilsBase/SysError.h>
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
// #include "Libraries/PrlCommonUtils/CDvdHelper.h"
#include <prlcommon/PrlCommonUtilsBase/CHardDiskHelper.h>
#include <prlcommon/PrlCommonUtilsBase/CGuestOsesHelper.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


using namespace Parallels;

/**
* @brief Helper for Callback functions, called from CreateParallelsDisk
* @param iDone
* @param iTotal
* @param pTaskHelper
* @param pHddCreateHelper
* @return TRUE to continue HardDisk image creation
*/
bool HddCallbackHelperFunc ( int iDone,
							int iTotal,
							Mixin_CreateHddSupport* pHddCreateHelper )
{
	LOG_MESSAGE( DBG_FATAL,"ConfigHddPageCallbackFunc Complete = [%d] of [%d]", iDone, iTotal);

	if (!pHddCreateHelper )
	{
		WRITE_TRACE(DBG_FATAL, "bad object type");
		pHddCreateHelper->setHddErrorCode( PRL_ERR_UNEXPECTED );
		return false;
	}

	if (iDone < 0)
	{
		// Error occurred in HardDisk image filling thread
		// Error code must be stored as [-iDone]
		WRITE_TRACE(DBG_FATAL, "Error occurred [%d]", -iDone);
		if ( iDone != PRL_ERR_DISK_USER_INTERRUPTED )
			pHddCreateHelper->setHddErrorCode( PRL_ERR_CANT_CREATE_HDD_IMAGE );
		pHddCreateHelper->setHddAdvancedError( Prl::GetLastErrorAsString() );
		return false;
	}

	/**
	* Send current progress to user
	*/

	int nCurrentPercent = (100 * iDone) / iTotal;

	if (pHddCreateHelper->getHddCurrentPercent() != nCurrentPercent)
	{
		pHddCreateHelper->setHddCurrentPercent(nCurrentPercent);

		// Create event for client
		CVmEvent event(	PET_JOB_HDD_CREATE_PROGRESS_CHANGED,
			pHddCreateHelper->getVmUuid(),
			PIE_DISPATCHER );

		/**
		* Add event parameters
		*/

		event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
			QString::number(nCurrentPercent),
			EVT_PARAM_PROGRESS_CHANGED));

		event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
			QString::number(PDE_HARD_DISK),
			EVT_PARAM_DEVICE_TYPE));

		event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
			QString::number(0),
			EVT_PARAM_DEVICE_INDEX));

		SmartPtr<IOPackage> p =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event,
			pHddCreateHelper->getRequestPackage());

		pHddCreateHelper->getClient()->sendPackage( p );
	}

	if ( pHddCreateHelper->operationIsCancelled() )
	{
		WRITE_TRACE(DBG_FATAL, "Image creation cancelled by user");
		// Image creation cancelled by user
		pHddCreateHelper->setHddErrorCode( PRL_ERR_OPERATION_WAS_CANCELED );
		pHddCreateHelper->wakeTask();

		return false;
	}

	if (iDone >= iTotal)
	{
		// Operation complete
		pHddCreateHelper->wakeTask();
	}

	return true;
};


Mixin_CreateHddSupport::Mixin_CreateHddSupport (
	SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
	bool bForceQuestionsSign ) :

CDspTaskHelper(client, p, bForceQuestionsSign),
m_nCurrentPercent(0),
m_HddError(PRL_ERR_SUCCESS)
{
}

Mixin_CreateHddSupport::~Mixin_CreateHddSupport()
{
}

QString Mixin_CreateHddSupport::getVmUuid()
{
	return Uuid().toString();
}

void Mixin_CreateHddSupport::wait ()
{
	m_semaphore.acquire();
}

void Mixin_CreateHddSupport::wakeTask ()
{
	m_semaphore.release();
}

int Mixin_CreateHddSupport::getHddCurrentPercent()
{
	return m_nCurrentPercent;
}

void Mixin_CreateHddSupport::setHddCurrentPercent(int i)
{
	m_nCurrentPercent=i;
}

PRL_RESULT	Mixin_CreateHddSupport::getHddErrorCode()
{
	return m_HddError;
}

void Mixin_CreateHddSupport::setHddErrorCode( PRL_RESULT errCode )
{
	m_HddError = errCode;
}

QString Mixin_CreateHddSupport::getHddAdvancedError()
{
	return m_strAdvancedErrorInfo;
}
void Mixin_CreateHddSupport::setHddAdvancedError( const QString& strAdvancedErrorInfo )
{
	m_strAdvancedErrorInfo = strAdvancedErrorInfo;
}

/**
* Create a XML file for a physical drive
* It needs to have admin privileges to use this method
* @param qsFullPath A XML file path for the physical drive description file
* @param qsSysDiskName A system name of the physical disk
* @return Error code
*/
PRL_RESULT Mixin_CreateHddSupport::ConfigurePhysical(
	const QString& /*qsFullPath*/,
	const QString& /*qsDiskName*/,
	CAuthHelper* /*pAuthHelper*/,
	CVmEvent* /*pOutErrParams*/ )
{
#ifdef _WIN_
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( pAuthHelper );
		if (CFileHelper::isRemotePath(qsFullPath))
		{
			WRITE_TRACE(DBG_FATAL, "We can't to configure physical HDD for VM placed on network share"\
					" due it's impossible to have at the same time local system account permissions"\
					" and impersonated user token");
			return PRL_ERR_CANT_CONFIGURE_PHYSICAL_HDD;
		}
	}
#endif

// VirtualDisk commented out by request from CP team
//	PRL_RESULT ret = IDiskConfigurator::ConfigurePhysical( qsFullPath, qsDiskName);
//	if ( PRL_FAILED(ret) )
//	{
//		WRITE_TRACE(DBG_FATAL,
//			"Mixin_CreateHddSupport::ConfigurePhysical(): "
//			"Can not configure physical drive %s by error %#x, [%s]. Path = %s"
//				, QSTR2UTF8( qsDiskName )
//				, ret
//				, PRL_RESULT_TO_STRING(ret)
//				, QSTR2UTF8( qsFullPath )
//			);
//		if( pOutErrParams )
//		{
//			QString sDetails = QString( "Error: %1 (0x%2) " "\nDiskName: '%3'" "\nPath: '%4'"  )
//				.arg( PRL_RESULT_TO_STRING(ret) )
//				.arg( (PRL_UINT32)ret, 0, 16 )
//				.arg( qsDiskName )
//				.arg( qsFullPath );
//
//			pOutErrParams->addEventParameter(
//				new CVmEventParameter( PVE::String,
//				QString("%1").arg( sDetails ),
//				EVT_PARAM_DETAIL_DESCRIPTION));
//		}
//		ret = PRL_ERR_CANT_CONFIGURE_PHYSICAL_HDD;
//	}
//	else
//	{	// physical disk descriptor is created with admin permissions
//		// but we have to set permissions of VM owner so we try to change permissions
//		QString qsDir = CFileHelper::GetFileRoot( qsFullPath );
//		ret = CFileHelper::ChangeDirectoryPermissions( qsDir, qsFullPath, pAuthHelper );
//	}
//	return ret;
	return PRL_ERR_FAILURE;
}
