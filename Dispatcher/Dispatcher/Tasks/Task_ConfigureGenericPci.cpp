///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ConfigureGenericPci.cpp
///
/// Dispatcher task for configuration generic PCI devices.
///
/// @author myakhin@
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
/////////////////////////////////////////////////////////////////////////////////

#include "CProtoSerializer.h"
#include "CDspService.h"
#include "CDspDispConfigGuard.h"
#include "Task_ConfigureGenericPci.h"
#include "CVmValidateConfig.h"

#include <prlcommon/Std/PrlAssert.h>

using namespace Parallels;

bool Task_ConfigureGenericPci::m_bIsRunning = false;

Task_ConfigureGenericPci::Task_ConfigureGenericPci( SmartPtr<CDspClient> &pUser,
													const SmartPtr<IOPackage> &pRequestPkg )
: CDspTaskHelper(pUser, pRequestPkg, false, &m_bIsRunning)
, m_bIsOperationFailed( false )
, m_bNeedHostReboot( false )
{
}

PRL_RESULT Task_ConfigureGenericPci::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if ( ! lockToExecute() )
			throw PRL_ERR_CONFIGURE_GENERIC_PCI_TASK_ALREADY_RUN;

		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
		if( !cmd->IsValid() )
			throw PRL_ERR_INVALID_ARG;

		CProtoCommandWithOneStrParam* pParam
			= CProtoSerializer::CastToProtoCommand<CProtoCommandWithOneStrParam>(cmd);
		if( !pParam )
			throw PRL_ERR_INVALID_ARG;

		QString sDevList = pParam->GetFirstStrParam();

		m_Devices.fromString(sDevList);
		if (PRL_FAILED(m_Devices.m_uiRcInit))
			throw PRL_ERR_INVALID_ARG;


		// Create local copy of disp devices to prevent deadlocks with CDspHostInfo
		SmartPtr<CDispGenericPciDevices> pDispDevices( new CDispGenericPciDevices(
			CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()
			->getPciPreferences()->getGenericPciDevices() )
			);

		// Create local copy of disp devices to prevent deadlocks
		SmartPtr<GenericPciDevices> pHostDevices;
		{
			CDspLockedPointer<CDspHostInfo> hostInfo = CDspService::instance()->getHostInfo();
			hostInfo->refresh();
			pHostDevices = SmartPtr<GenericPciDevices>(
				new GenericPciDevices( hostInfo->data()->getGenericPciDevices() )
				);
		}

		foreach( CHwGenericPciDevice* pPciDevice, pHostDevices->m_lstGenericPciDevice )
		{
            // Store configuration since on finalizeTask the list will be cleared
			PciDeviceConfigure conf;
			conf.state = pPciDevice->getDeviceState();
			foreach( CDispGenericPciDevice* pDispDevice, pDispDevices->m_lstGenericPciDevices )
			{
				if( pDispDevice->getDeviceId() == pPciDevice->getDeviceId() )
				{
					conf.driverName  = pDispDevice->getNativeDriverName();
					conf.serviceName = pDispDevice->getNativeServiceName();
					break;
				}
			}

			m_mapHwDevices.insert( pPciDevice->getDeviceId(), conf );
		}

		foreach( CHwGenericPciDevice* pPciDevice, m_Devices.m_lstGenericPciDevice )
		{
			if ( ! m_mapHwDevices.contains(pPciDevice->getDeviceId()) )
			{
				getLastError()->addEventParameter(
						new CVmEventParameter(PVE::String,
						pPciDevice->getDeviceId(),
						EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_VTD_DEVICE_DOES_NOT_EXIST;
			}
		}
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while configure generic PCI devices with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	getLastError()->setEventCode( ret );

	return ret;
}

namespace {
/**
 * This simple helper generates proper virtual device type due common
 * generic PCI devices list maps into several devices lists at VM
 * configuration (i.e. video adapters have PDE_PCI_VIDEO_ADAPTER type;
 * network cards PDE_GENERIC_NETWORK_ADAPTER and etc.)
 */
PRL_DEVICE_TYPE GetGenericPciType(CHwGenericPciDevice *pPciDevice)
{
	if ( PGD_PCI_NETWORK == pPciDevice->getType() )
		return (PDE_GENERIC_NETWORK_ADAPTER);
	else if ( PGD_PCI_DISPLAY == pPciDevice->getType() )
		return (PDE_PCI_VIDEO_ADAPTER);
	else
		return (pPciDevice->getDeviceType());
}
}

PRL_RESULT Task_ConfigureGenericPci::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		bool bVtdHookUpdate = false;

		foreach( CHwGenericPciDevice* pPciDevice, m_Devices.m_lstGenericPciDevice )
		{
			int Bus = -1;
			int Dev = -1;
			int Fun = -1;
			bool bReboot = false;
			Q_UNUSED(bReboot);
			if ( ! getPciDeviceNumbers(pPciDevice->getDeviceId(), Bus, Dev, Fun) )
			{
				// Unexpected
				WRITE_TRACE(DBG_FATAL, "Unknown format device id = '%s' !", QSTR2UTF8(pPciDevice->getDeviceId()));
				continue;
			}

			PciDeviceConfigure conf = m_mapHwDevices.value(pPciDevice->getDeviceId());
			PRL_GENERIC_DEVICE_STATE nRealState = conf.state;
			PRL_GENERIC_DEVICE_STATE nNeedState = pPciDevice->getDeviceState();

			PRL_RESULT nResPerDevice = PRL_ERR_SUCCESS;

			// Note: Host hardware info returns for generic PCI devices only
			//       two states: PGS_CONNECTED_TO_HOST or PGS_CONNECTED_TO_VM !
			if ( nRealState == PGS_CONNECTED_TO_HOST && nNeedState == PGS_CONNECTED_TO_VM)
			{
				//Check whether generic PCI device currently using by some running VM
				//https://bugzilla.sw.ru/show_bug.cgi?id=423808
				bool bGenericPciUsedByRunningVm = false;
				CVmValidateConfig::IsDeviceInAnotherVm( GetGenericPciType(pPciDevice), pPciDevice->getDeviceId(), QString(), bGenericPciUsedByRunningVm );
				if ( bGenericPciUsedByRunningVm )
				{
					nResPerDevice = PRL_ERR_VTD_HOOK_DEVICE_CURRENTLY_IN_USE;
				}
			}
			else if ( nRealState == PGS_CONNECTED_TO_VM	&& nNeedState == PGS_CONNECTED_TO_HOST )
			{
				//Check whether generic PCI device currently using by some running VM
				//https://bugzilla.sw.ru/show_bug.cgi?id=423808
				bool bGenericPciUsedByRunningVm = false;
				CVmValidateConfig::IsDeviceInAnotherVm( GetGenericPciType(pPciDevice), pPciDevice->getDeviceId(), QString(), bGenericPciUsedByRunningVm );
				if ( bGenericPciUsedByRunningVm )
				{
					nResPerDevice = PRL_ERR_VTD_HOOK_DEVICE_CURRENTLY_IN_USE;
				}
			}

			if (PRL_FAILED(nResPerDevice))
			{
				// Here we should process PRL_ERR_VTD_HOOK_AFTER_INSTALL_NEED_REBOOT successfully

				switch(nResPerDevice)
				{
				case PRL_ERR_VTD_HOOK_AFTER_INSTALL_NEED_REBOOT:
				case PRL_ERR_VTD_HOOK_AFTER_REVERT_NEED_REBOOT:

					m_bNeedHostReboot = true;
					break;
				default:
					;
				}

				WRITE_TRACE(DBG_FATAL, "Driver operation for '%s' generic PCI device is failed with error %s ( %#x ) !",
							QSTR2UTF8(pPciDevice->getDeviceId()),
							PRL_RESULT_TO_STRING(nResPerDevice),
							nResPerDevice);

				m_bIsOperationFailed = true;
				ret = PRL_ERR_OPERATION_FAILED;

				CVmEvent evt;
				evt.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
				evt.setEventCode(nResPerDevice);
				evt.addEventParameter( new CVmEventParameter(PVE::String,
															 pPciDevice->getDeviceId(),
															 EVT_PARAM_MESSAGE_PARAM_0) );
				m_lstDevErrors.append( evt.toString() );
			}
		}

		if (bVtdHookUpdate)
		{
			ret = updateVtdHook();
			if (PRL_FAILED(ret))
				throw ret;
		}

		if (PRL_FAILED(ret))
			throw ret;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while configure generic PCI devices with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode( ret );

	return ret;
}

void Task_ConfigureGenericPci::finalizeTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	bool bNeedSendEvent = false;

	try
	{
		if ( PRL_FAILED( getLastErrorCode() ) && ! m_bIsOperationFailed )
			throw getLastErrorCode();

		// TODO: Finish here installation drivers for generic PCI devices

		// Save hot plugged devices in dispatcher config

		CDspLockedPointer<CDispCommonPreferences> pCommPrefs
			= CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
		CDispGenericPciDevices* pDispDevices = pCommPrefs->getPciPreferences()->getGenericPciDevices();

		QString qsNotChangedDevsXml = pDispDevices->toString();

// Edit or add to config devices

		foreach( CHwGenericPciDevice* pPciDevice, m_Devices.m_lstGenericPciDevice )
		{
			CDispGenericPciDevice* pEditDispDevice = 0;
			foreach( CDispGenericPciDevice* pDispDevice, pDispDevices->m_lstGenericPciDevices )
			{
				if (pDispDevice->getDeviceId() == pPciDevice->getDeviceId())
				{
					pEditDispDevice = pDispDevice;
					break;
				}
			}

			if ( ! pEditDispDevice )
			{
				pEditDispDevice = new CDispGenericPciDevice();
				pDispDevices->m_lstGenericPciDevices += pEditDispDevice;
			}

			pEditDispDevice->setDeviceId( pPciDevice->getDeviceId() );
			pEditDispDevice->setDeviceState( pPciDevice->getDeviceState() );
			pEditDispDevice->setNativeDriverName( m_mapHwDevices.value( pPciDevice->getDeviceId() ).driverName );
			pEditDispDevice->setNativeServiceName( m_mapHwDevices.value( pPciDevice->getDeviceId() ).serviceName );
		}

// Delete devices from config

		foreach( CDispGenericPciDevice* pDispDevice, pDispDevices->m_lstGenericPciDevices )
		{
			if ( ! m_mapHwDevices.contains(pDispDevice->getDeviceId()) )
			{
				pDispDevices->m_lstGenericPciDevices.removeAll(pDispDevice);
				delete pDispDevice;
			}
		}

// Save config if it was changed

		if ( qsNotChangedDevsXml != pDispDevices->toString() )
		{
			ret = CDspService::instance()->getDispConfigGuard().saveConfig(false);
			if (PRL_FAILED(ret))
				throw ret;

			bNeedSendEvent = true;
		}

		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while configure generic PCI devices with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode(ret);

	if (PRL_SUCCEEDED(ret) || bNeedSendEvent)
	{
		// Send event that host hardware info was changed
		CVmEvent event( PET_DSP_EVT_HW_CONFIG_CHANGED,
						Uuid().toString(),
						PIE_DISPATCHER );

		SmartPtr<IOPackage> p =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage() );
		CDspService::instance()->getClientManager().sendPackageToAllClients(p);
	}

	// Send response
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), ret );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

	pResponseCmd->SetParamsList( m_lstDevErrors );

	getClient()->sendResponse( pCmd, getRequestPackage() );

	do
	{
		if ( ! m_bNeedHostReboot )
			break;

		if (!getClient()->getAuthHelper().isLocalAdministrator())
		{
			WRITE_TRACE(DBG_FATAL, "Client has to be administrator for reboot host!");

			// Send warning about it
			CVmEvent event( PET_DSP_EVT_REBOOT_HOST_MANUAL_TO_INIT_VTD_DEVICES,
							Uuid().toString(),
							PIE_DISPATCHER );

			SmartPtr<IOPackage> p
				= DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage() );
			CDspService::instance()->getClientManager().sendPackageToAllClients(p);
			break;
		}

		WRITE_TRACE(DBG_FATAL, "Stop dispatcher with reboot host was canceled!");
	}
	while(0);

	unlockToExecute();
}

bool Task_ConfigureGenericPci::getPciDeviceNumbers(const QString& qsDeviceId, int& nNum1, int& nNum2, int& nNum3)
{
	QStringList lstNums = qsDeviceId.split(':');
	if (lstNums.size() < 3)
		return false;

	bool bIsParseOk = false;

	nNum1 = lstNums[0].toInt( &bIsParseOk );
	if ( ! bIsParseOk )
		return false;
	nNum2 = lstNums[1].toInt( &bIsParseOk );
	if ( ! bIsParseOk )
		return false;
	nNum3 = lstNums[2].toInt( &bIsParseOk );
	if ( ! bIsParseOk )
		return false;

	return true;
}

PRL_RESULT Task_ConfigureGenericPci::updateVtdHook()
{
#ifdef _LIN_
	QProcess proc;

	WRITE_TRACE(DBG_FATAL, "Start Vtd hook update script...");

	QString qsScriptPath
		= ParallelsDirs::getParallelsScriptsDir()
			+ "/vtdhook-update";

	proc.start(qsScriptPath);
	if ( ! proc.waitForStarted() )
	{
		// OK. No vtd update script is success situation,
		// i.e. it does not necessary on this host
		WRITE_TRACE(DBG_WARNING, "Vtd hook update script not installed");
		return PRL_ERR_SUCCESS;
	}
	proc.waitForFinished(-1);

	if (proc.exitCode())
		return PRL_ERR_VTD_HOOK_UPDATE_SCRIPT_EXECUTE;

	WRITE_TRACE(DBG_FATAL, "Vtd hook update script finished");

#endif
	return PRL_ERR_SUCCESS;
}
