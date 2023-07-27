///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EditVm.cpp
///
/// Edit VM configuration
///
/// @author myakhin@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2023 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////


#include "Task_EditVm.h"
#include "Task_EditVm_p.h"
#include <boost/tuple/tuple.hpp>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include "Libraries/PrlNetworking/netconfig.h"
#include <prlcommon/PrlCommonUtilsBase/CFileHelper.h>
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"
#include <Libraries/Transponster/Reverse.h>
#include <Libraries/Virtuozzo/OvmfHelper.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlxmlmodel/VirtuozzoObjects/CXmlModelHelper.h>
#include <prlxmlmodel/VirtuozzoObjects/CVmProfileHelper.h>
#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#include "CDspLibvirtExec.h"
#endif // _LIBVIRT_
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Tasks/Task_DiskImageResizer.h"
#include "Tasks/Mixin_CreateHddSupport.h"
#include "CDspCommon.h"
#include "CDspService.h"
#include "CDspVmDirHelper_p.h"
#include "CVmValidateConfig.h"
#include "CDspVmNetworkHelper.h"
#include "CDspVmGuestPersonality.h"
#include <algorithm>
#include <boost/bind.hpp>
#include "EditHelpers/CMultiEditMergeVmConfig.h"

#include "CDspVzHelper.h"
#include "CDspHaClusterHelper.h"
#include "CDspVmStateSender.h"
#include "CDspVm_p.h"
#include "CDspBackupDevice.h"
#include "CDspVmGuest.h"

namespace
{
// RAM, GiB     <16       <32        <64   ...
// factor        1         2          4
// maxNuma     RAM+4     RAM+8      RAM+16
// maxTotal  maxNuma+64 maxNuma+64 maxNuma+64

unsigned getMaxNumaRamMb(unsigned ram)
{
	unsigned factor = (ram/1024 + 15) / 16;
	return (ram + 4*factor*1024)/1024*1024;
}

unsigned getMaxTotalRamMb(unsigned ram)
{
	return getMaxNumaRamMb(ram) + 64*1024;
}
}


Task_EditVm::Task_EditVm(const SmartPtr<CDspClient>& pClient,
						 const SmartPtr<IOPackage>& p)
: CDspTaskHelper(pClient, p)
{
}

QString  Task_EditVm::getVmUuid()
{
	return m_sVmUuid;
}

void Task_EditVm::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pUserSession, p);

	SmartPtr< CDspVm > pVm = CDspVm::GetVmInstanceByUuid(getVmUuid(), getClient()->getVmDirectoryUuid() );
	if( !pVm )
		return;
	pVm->wakeupApplyConfigWaiters();
}

PRL_RESULT Task_EditVm::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		ret = editVm();
		if (PRL_FAILED(ret))
			throw ret;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while edit VM with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode( ret );

	return ret;
}

void Task_EditVm::finalizeTask()
{
	// Send response
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );

	if( PRL_SUCCEEDED(getLastErrorCode()) )
	{
		CDspVmDirHelper& vmDirHelper = CDspService::instance()->getVmDirHelper();

		PRL_RESULT error;
		SmartPtr<CVmConfiguration>
			pVmConfig = vmDirHelper.getVmConfigByUuid(getClient(), getVmUuid(), error);
		if( pVmConfig )
			vmDirHelper.fillOuterConfigParams( getClient(), pVmConfig );
		else
		{
			WRITE_TRACE( DBG_FATAL, "Can't load vm config for %s vm to make EditCommit response by err %s"
				, QSTR2UTF8( getVmUuid() )
				, PRL_RESULT_TO_STRING(error) );
			pVmConfig = ::List::Directory::Item::Inaccessible::Default(*CDspService::instance(), getClient())
				.craft(error, getVmUuid());
		}
		if( pVmConfig )
		{
			CProtoCommandDspWsResponse
				*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

			CDspService::instance()->getVmDirHelper().appendAdvancedParamsToVmConfig(getClient(), pVmConfig);

			QStringList lstParams( pVmConfig->toString() );
			pResponseCmd->SetParamsList( lstParams );
		}
	}
	else
	{
		CProtoCommandDspWsResponse
			*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
		pResponseCmd->SetError( getLastError()->toString() );

		QStringList lstParams;
		QList<CVmEventParameter *>::iterator _it = getLastError()->m_lstEventParameters.begin();
		while ( _it != getLastError()->m_lstEventParameters.end() )
		{
			CVmEventParameter* pParam = *_it;
			if ( pParam->getParamName() == EVT_PARAM_COMPLEX_EVENT )
			{
				lstParams += pParam->getParamValue();
				_it = getLastError()->m_lstEventParameters.erase(_it);
				delete pParam;
			}
			else
				++_it;
		}
		pResponseCmd->SetParamsList( lstParams );
	}

	getClient()->sendResponse( pCmd, getRequestPackage() );
}

bool Task_EditVm::atomicEditVmConfigByVm(
	const QString &vmDirUuid,
	const QString& vmUuid,
	const CVmEvent& evtFromVm,
	SmartPtr<CDspClient> pUserSession
	)
{
	const IOSender::Handle
		hFakeClientHandle = QString("%1-%2").arg( vmDirUuid ).arg( vmUuid );


	//////////////////////////////////////////////////////////////////////////
	// NOTE:	TO EXCLUDE DEADLOCK m_pVmConfigEdit mutex
	//			SHOULD be locked BEFORE CDspLockedPointer<..> from getVmDirManager().getXX().
	//////////////////////////////////////////////////////////////////////////

	LOG_MESSAGE( DBG_DEBUG, "atomicEdit() for was called for vm  %s", QSTR2UTF8( vmUuid ) );

	PRL_RESULT nRetCode = CDspService::instance()->getVmDirHelper()
		.registerExclusiveVmOperation( vmUuid, vmDirUuid, PVE::DspCmdDirVmEditCommit, pUserSession);
	if (PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to register exclusive operation on atomic VM config edit: %.8X '%s'",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return (false);
	}
	QMutexLocker lock(CDspService::instance()->getVmDirHelper().getMultiEditDispatcher());

	bool retValue = false;
	QString vmName;
	try
	{
		bool flgConfigChanged = false;
		bool flgApplyConfig = false; // if true - propagate the change to libvirt
		bool flgNoChangeAllowed = false;

		//////////////////////////////////////////////////////////////////////////
		// get actually vm configuration
		//////////////////////////////////////////////////////////////////////////
		PRL_RESULT	outError = PRL_ERR_FIXME;
		SmartPtr<CVmConfiguration>  pVmConfig
			= CDspService::instance()->getVmDirHelper().getVmConfigByUuid ( vmDirUuid, vmUuid, outError );
		if( ! pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED( outError ) );
			if( !PRL_FAILED( outError ) )
				outError = PRL_ERR_FAILURE;
			WRITE_TRACE(DBG_FATAL, "Failed to read vm configuration" );
			throw outError;
		}

		SmartPtr<CVmConfiguration> pVmConfigOld
			= SmartPtr<CVmConfiguration>(new CVmConfiguration(pVmConfig.getImpl()));

		vmName = pVmConfig->getVmIdentification()->getVmName();

		CDspService::instance()->getVmDirHelper().getMultiEditDispatcher()
			->registerBeginEdit( vmUuid, hFakeClientHandle, pVmConfig );

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_GUEST_OS_TYPE
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_GUEST_OS_TYPE ) )
		{
			const unsigned int osType = pParam->getParamValue().toUInt();
			const CVmCommonOptions *const vmCommon = pVmConfig->getVmSettings()->getVmCommonOptions();
			if (vmCommon->getOsType() != osType)
			{
				pVmConfig->getVmSettings()->getVmCommonOptions()->setOsType( osType );
				flgConfigChanged = true;
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_GUEST_OS_TYPE
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_GUEST_OS_VERSION
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_GUEST_OS_VERSION ) )
		{
			const unsigned int osVersion = pParam->getParamValue().toUInt();
			CVmCommonOptions* vmCommon = pVmConfig->getVmSettings()->getVmCommonOptions();
			if (vmCommon->getOsVersion() != osVersion)
			{
				QList<unsigned int > lstPrevOsVersions = vmCommon->getPrevOsVersions();
				lstPrevOsVersions.prepend( vmCommon->getOsVersion() );
				vmCommon->setPrevOsVersions( lstPrevOsVersions );

				pVmConfig->getVmSettings()->getVmCommonOptions()->setOsVersion( osVersion );
				flgConfigChanged = true;

				patchConfigOnOSChanged(pVmConfig, pVmConfigOld);
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_GUEST_OS_VERSION
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_DEVICE_CONFIG_WITH_NEW_STATE
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_DEVICE_CONFIG_WITH_NEW_STATE ) )
		{
			CVmDevice* pChangedDevice = NULL;
			pChangedDevice = CVmDevice::getDeviceInstanceFromString( pParam->getParamValue() );

			if( !pChangedDevice || PRL_FAILED( pChangedDevice->m_iParseRc ) )
			{
				WRITE_TRACE(DBG_FATAL, "Failed to parse EVT_PARAM_VMCFG_DEVICE_CONFIG_WITH_NEW_STATE %s"
					, QSTR2UTF8(pParam->getParamValue()) );
			}
			else if( pChangedDevice->getDeviceType() < PDE_MAX )
			{
				SmartPtr<CVmConfiguration> pOldVmConfig;

				if (pChangedDevice->getDeviceType()== PDE_GENERIC_NETWORK_ADAPTER)
					pOldVmConfig =
						SmartPtr<CVmConfiguration>(new CVmConfiguration( pVmConfig.getImpl() ));

				// 1. search device in config  by unique key: KEY={ TYPE, INDEX }
				QList<CVmDevice*>* p_lstDevices = reinterpret_cast< QList<CVmDevice*>* >
					( pVmConfig->getVmHardwareList()->m_aDeviceLists[ pChangedDevice->getDeviceType() ] );

				PRL_ASSERT( p_lstDevices );
				for ( int j = 0; p_lstDevices && j < p_lstDevices->size(); j++ )
				{
					CVmDevice* pCurrDevice = p_lstDevices->at(j);
					if( pCurrDevice->getIndex() != pChangedDevice->getIndex() )
						continue;

					// 2. update device
					if( ! pCurrDevice->fromString( pChangedDevice->toString() ) )
					{
						flgConfigChanged = true;
						flgApplyConfig = true;
					}
					else
					{
						WRITE_TRACE(DBG_FATAL, "Unable to update device vm=%s sysname=%s"
							, QSTR2UTF8( vmUuid )
							, QSTR2UTF8( pChangedDevice->getSystemName() ) );
					}
					break;

				}//for

				if (flgConfigChanged && pChangedDevice->getDeviceType()== PDE_GENERIC_NETWORK_ADAPTER)
					updateNetworkSettings(pVmConfig, pOldVmConfig) ;

			}// else ( ! pChangedDevice )

			delete pChangedDevice;
			pChangedDevice = NULL;
		}// if( CVmEventParameter*
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_DEVICE_CONFIG_WITH_NEW_STATE
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_NEW_DEVICE_CONFIG
		//////////////////////////////////////////////////////////////////////////
		if (CVmEventParameter* pParam = evtFromVm.getEventParameter(EVT_PARAM_VMCFG_NEW_DEVICE_CONFIG))
		{
			CVmDevice* newDevice(CVmDevice::getDeviceInstanceFromString(pParam->getParamValue()));

			if (!newDevice)
			{
				WRITE_TRACE(DBG_FATAL, "Failed to parse EVT_PARAM_VMCFG_NEW_DEVICE_CONFIG %s",
						QSTR2UTF8(pParam->getParamValue()));
			}
			else if (newDevice->getDeviceType() < PDE_MAX)
			{
				QList<void* >* l = (QList<void* >* )pVmConfig->getVmHardwareList()->m_aDeviceLists[newDevice->getDeviceType()];
				if (l)
				{
					l->push_back(newDevice);
					flgConfigChanged = true;
					flgApplyConfig = true;
					newDevice = NULL;
				}
			}
			delete newDevice;
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_NEW_DEVICE_CONFIG
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_SERVER_UUID
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_SERVER_UUID ) )
		{
			QString qsServerUuid = pParam->getParamValue();
			Uuid check_uuid(qsServerUuid);
			if (!check_uuid.isNull())
			{
				pVmConfig->getVmIdentification()->setServerUuid(qsServerUuid);
				flgConfigChanged = true;
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed to convert EVT_PARAM_VMCFG_SERVER_UUID for vm %s"
					, QSTR2UTF8( vmUuid )
					);
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_SERVER_UUID
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_INSTALLED_SOFTWARE_ID
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam =
			evtFromVm.getEventParameter(EVT_PARAM_VMCFG_INSTALLED_SOFTWARE_ID) )
		{
			bool bOk;
			unsigned int nSoftwareId = pParam->getParamValue().toUInt( &bOk );
			if ( bOk && nSoftwareId > 0 )
			{
				pVmConfig->setInstalledSoftware( pVmConfig->getInstalledSoftware() | nSoftwareId );
				flgConfigChanged = true;
			}
			else
			{
				WRITE_TRACE( DBG_FATAL, "Unexpected Installed Software ID : %d.", nSoftwareId );
				PRL_ASSERT( false );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_INSTALLED_SOFTWARE_ID
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_REMOTE_DEVICES
		//////////////////////////////////////////////////////////////////////////
		// remote device must be disconnected and saved on vm stopping
		// #429716 #429855 #110571
		if( evtFromVm.getEventParameter(EVT_PARAM_VMCFG_REMOTE_DEVICES) )
		{
			flgNoChangeAllowed = true;
			for ( int i = 0; i < PDE_MAX; i++ )
			{
				if ( QList<CVmDevice*>* lstDevices =
					reinterpret_cast< QList<CVmDevice*>* >
					(pVmConfig->getVmHardwareList()->m_aDeviceLists[i] ) )
				{
					for ( int j = 0; j < lstDevices->size(); j++ )
						if ( lstDevices->at(j)->isRemote()
									&&
							( lstDevices->at(j)->getConnected() == PVE::DeviceConnected )
							)
						{
							lstDevices->at(j)->setConnected( PVE::DeviceDisconnected );
							flgConfigChanged = true;
						}
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// finish EVT_PARAM_VMCFG_REMOTE_DEVICES
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_COMPACT_HDD_MASK
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_COMPACT_HDD_MASK ) )
		{
			bool flgOk = true;
			unsigned int uiCompactHddMask = pParam->getParamValue().toUInt( &flgOk );
			if (flgOk)
			{
				pVmConfig->getVmSettings()->getVmRuntimeOptions()->setCompactHddMask(uiCompactHddMask);
				flgConfigChanged = true;
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed to convert EVT_PARAM_VMCFG_COMPACT_HDD_MASK for vm %s"
					, QSTR2UTF8( vmUuid )
					);
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_COMPACT_HDD_MASK
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_NEW_VNETWORK_ID
		//////////////////////////////////////////////////////////////////////////
		CVmEventParameter* pNewParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_NEW_VNETWORK_ID );
		CVmEventParameter* pOldParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_OLD_VNETWORK_ID );
		if (pNewParam && pOldParam)
		{
			QString sNewNetworkID = pNewParam->getParamValue();
			QString sOldNetworkID = pOldParam->getParamValue();
			foreach( CVmGenericNetworkAdapter *pNetAdapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
			{
				if (pNetAdapter->getVirtualNetworkID() == sOldNetworkID)
				{
					if (sNewNetworkID.isEmpty())
						// Disconnect device on network removal
						pNetAdapter->setConnected(PVE::DeviceDisconnected);
					else
						pNetAdapter->setVirtualNetworkID(sNewNetworkID);
					flgConfigChanged = true;
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VM_UPTIME_DELTA
		//////////////////////////////////////////////////////////////////////////
		// VM uptime counter should be updated with new value
		// #464218
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter(EVT_PARAM_VM_UPTIME_DELTA) )
		{
			flgConfigChanged = true;
			quint64 nDelta = pParam->getParamValue().toULongLong();
			WRITE_TRACE(DBG_FATAL, "Updating VM uptime with delta=%llu old values: %llu '%s'",
				nDelta,
				pVmConfig->getVmIdentification()->getVmUptimeInSeconds(),
				QSTR2UTF8(pVmConfig->getVmIdentification()->getVmUptimeStartDateTime().toString(XML_DATETIME_FORMAT))
			);
			if ( nDelta )
				pVmConfig->getVmIdentification()->setVmUptimeInSeconds(
					pVmConfig->getVmIdentification()->getVmUptimeInSeconds() +
					nDelta
				);
			else//Request on reset VM uptime
			{
				pVmConfig->getVmIdentification()->setVmUptimeInSeconds( 0 );
				pVmConfig->getVmIdentification()->setVmUptimeStartDateTime();
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// finish EVT_PARAM_VM_UPTIME_DELTA
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VM_FAST_REBOOT
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter(EVT_PARAM_VM_FAST_REBOOT) )
		{
			bool flgOk = true;

			int fastReboot = pParam->getParamValue().toInt( &flgOk );
			CVmEventParameter* pParamUser =
				evtFromVm.getEventParameter(EVT_PARAM_VM_FAST_REBOOT_USER);
			if (flgOk && pParamUser)
			{
				flgConfigChanged = true;
				WRITE_TRACE(DBG_INFO, "Updating VM (%s) FastReboot value to %d",
						QSTR2UTF8( vmName ), fastReboot);
				pVmConfig->getVmSettings()->
					getVmStartupOptions()->setFastReboot(fastReboot ? true : false );
				pVmConfig->getVmSettings()->
					getVmStartupOptions()->setVmFastRebootUser( pParamUser->getParamValue() );
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed to get EVT_PARAM_VM_FAST_REBOOT "
						"value for VM (%s)",
						QSTR2UTF8( vmName ));
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// finish EVT_PARAM_VM_FAST_REBOOT
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_NETWORK_ADAPTER_TYPE
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_NETWORK_ADAPTER_TYPE ) )
		{
			bool flgOk = true;
			PRL_VM_NET_ADAPTER_TYPE mAdapterType
				= (PRL_VM_NET_ADAPTER_TYPE )pParam->getParamValue().toUInt( &flgOk );
			if (flgOk)
			{
				foreach(CVmGenericNetworkAdapter* pNet, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
				{
					pNet->setAdapterType(mAdapterType);
					flgConfigChanged = true;
				}
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed to convert EVT_PARAM_NETWORK_ADAPTER_TYPE for vm %s"
					, QSTR2UTF8( vmUuid )
					);
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// finish EVT_PARAM_NETWORK_ADAPTER_TYPE
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VM_TOOLS_VERSION
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VM_TOOLS_VERSION ) )
		{
			const QString v = pParam->getParamValue();
			const QString o = pVmConfig->getVmSettings()->getVmTools()->getAgentVersion();
			if (v != o) {
				WRITE_TRACE(DBG_INFO, "Updating tools version %s from %s for VM %s",
						qPrintable(v), qPrintable(o), qPrintable(vmUuid));
				pVmConfig->getVmSettings()->getVmTools()->setAgentVersion(v);
				flgConfigChanged = true;
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VM_TOOLS_VERSION
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_CPU_FEATURES_MASK
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_CPU_FEATURES_MASK ) )
		{
			CDispCpuPreferences cpuMask;
			cpuMask.fromString(pParam->getParamValue());

			CCpuHelper::update(*pVmConfig, cpuMask);

			flgConfigChanged = true;
			flgApplyConfig = true;
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_CPU_FEATURES_MASK
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// EVT_PARAM_VMCFG_REMOTE_DISPLAY_PORT
		//////////////////////////////////////////////////////////////////////////
		if( CVmEventParameter* pParam = evtFromVm.getEventParameter( EVT_PARAM_VMCFG_REMOTE_DISPLAY_PORT ))
		{
			bool OnOffRemotePort =  pParam->getParamValue().toInt();

			if ( !OnOffRemotePort )
			{
				pVmConfig->getVmSettings()->getVmRemoteDisplay()->setMode();

				flgConfigChanged = true;
				flgApplyConfig = true;
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// Finish EVT_PARAM_VMCFG_REMOTE_DISPLAY_PORT
		//////////////////////////////////////////////////////////////////////////

		if( ! flgConfigChanged )
		{
			if( flgNoChangeAllowed )
				throw PRL_ERR_SUCCESS;
			WRITE_TRACE(DBG_FATAL, "%s:  method was called, but vm config doesn't changed", __FUNCTION__ );
			throw PRL_ERR_FAILURE;
		}

		CDspLockedPointer<CVmDirectoryItem>
			pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid( vmDirUuid, vmUuid );
		if( ! pVmDirItem )
		{
			WRITE_TRACE(DBG_FATAL, "Can't get vm dir item" );
			throw PRL_ERR_VM_UUID_NOT_FOUND;
		}

		PRL_RESULT res =
			CDspService::instance()->getVmConfigManager().saveConfig(pVmConfig,
			pVmDirItem->getVmHome(),
			pUserSession,
			true,
			true);
		if( PRL_FAILED( res ) )
		{
			WRITE_TRACE(DBG_FATAL, "Can't save vm config" );
			throw res;
		}

#ifdef _LIBVIRT_
		if (flgApplyConfig)
		{
			Edit::Vm::Config::Update()(Edit::Vm::Config::Update::load_type
				(Libvirt::Kit.vms().at(vmUuid), pVmConfig.getImpl()));
		}
#endif // _LIBVIRT_
		CXmlModelHelper::printConfigDiff(*pVmConfig, *pVmConfigOld, DBG_WARNING, "VmCfgAtomicEditDiff");

		CDspService::instance()->getVmDirHelper()
			.getMultiEditDispatcher()->registerCommit( vmUuid, hFakeClientHandle );

		retValue = true;
	}
	catch ( const PRL_RESULT& err )
	{
		retValue = (bool)PRL_SUCCEEDED( err );
		if( !retValue )
		{
			WRITE_TRACE(DBG_FATAL, "atomicEditVmConfiguration( vmName = %s, vmUuid= %s ) failed by error %#x ( %s ) "
				, QSTR2UTF8( vmName )
				, QSTR2UTF8( vmUuid )
				, err
				, PRL_RESULT_TO_STRING( err )
				);
		}

		//Uregister begin edit mark in order do not allow memleaks here
		CDspService::instance()->getVmDirHelper()
			.getMultiEditDispatcher()->cleanupBeginEditMark( vmUuid, hFakeClientHandle );
	}

	lock.unlock();
	CDspService::instance()->getVmDirHelper()
		.unregisterExclusiveVmOperation( vmUuid, vmDirUuid, PVE::DspCmdDirVmEditCommit, pUserSession );

	return retValue;
}

void Task_EditVm::patchConfigOnOSChanged(SmartPtr<CVmConfiguration> pVmConfig,
										 SmartPtr<CVmConfiguration> pVmConfigOld)
{
	PRL_ASSERT(pVmConfig.isValid());
	if ( ! pVmConfig )
		return;
	PRL_ASSERT(pVmConfigOld.isValid());
	if ( ! pVmConfigOld )
		return;

	if ( pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion()
		 == PVS_GUEST_VER_WIN_WINDOWS8
		 || pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion()
		 == PVS_GUEST_VER_WIN_WINDOWS8_1 )
	{
		WRITE_TRACE(DBG_WARNING, "Guest OS changed : sticky mouse was enabled");
		pVmConfig->getVmSettings()->getVmRuntimeOptions()->setStickyMouse(true);

		WRITE_TRACE(DBG_WARNING, "Guest OS changed : native look was disabled");
		pVmConfig->getVmSettings()->getVmTools()->getNativeLook()->setEnabled(false);

		if (pVmConfigOld->getVmSettings()->getVmCommonOptions()->getOsVersion()
			<= PVS_GUEST_VER_WIN_VISTA)
		{
			WRITE_TRACE(DBG_WARNING, "Guest OS changed : high resolution drawing was enabled");
			pVmConfig->getVmHardwareList()->getVideo()->setEnableHiResDrawing(true);
			WRITE_TRACE(DBG_WARNING, "Guest OS changed : use high resolution in guest was enabled");
			pVmConfig->getVmHardwareList()->getVideo()->setUseHiResInGuest(true);
		}
	}
}

void Task_EditVm::beginEditVm(const IOSender::Handle& sender,
								  SmartPtr<CDspClient> pUserSession,
								  const SmartPtr<IOPackage>& pkg )
{

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	QString vm_uuid = cmd->GetVmUuid();

	// AccessCheck
	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	CVmEvent evt;

	rc = CDspService::instance()->getAccessManager().checkAccess( pUserSession, PVE::DspCmdDirVmEditBegin
		, vm_uuid, &bSetNotValid, &evt );
	if ( ! PRL_SUCCEEDED(rc) )
	{
		CDspService::instance()->getVmDirHelper()
			.sendNotValidState(pUserSession, rc, vm_uuid, bSetNotValid);
		pUserSession->sendResponseError(evt, pkg);
		return;
	}

	CVmIdent x(vm_uuid, DspVm::vdh().getVmDirectoryItemByUuid(pUserSession, vm_uuid).first);
	if (x.second.isEmpty())
		return (void)pUserSession->sendSimpleResponse(pkg, PRL_ERR_NO_VM_DIR_CONFIG_FOUND);

	// We cannot edit suspended or suspending VM
	VIRTUAL_MACHINE_STATE nState = CDspVm::getVmState(x);
	if (nState == VMS_SUSPENDED || nState == VMS_SUSPENDING || nState == VMS_SUSPENDING_SYNC)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot edit VM configuration in suspended or suspending state!");
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_CANT_EDIT_SUSPENDED_VM );
		return;
	}

	PRL_RESULT error = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig
		= CDspService::instance()->getVmDirHelper().getVmConfigByUuid(pUserSession, vm_uuid, error);

	try
	{
		if( ! pVmConfig )
			throw error;

		// append additional parameters from VM directory
		// (VM home, last change date, last modification date, ... )
		CDspService::instance()->getVmDirHelper()
			.appendAdvancedParamsToVmConfig( pUserSession, pVmConfig, true );

		// clear suspend flag for disks
		CDspService::instance()->getVmDirHelper().getMultiEditDispatcher()->lock();
		CStatesHelper::SetSuspendParameterForAllDisks(pVmConfig.getImpl(),0);
		CDspService::instance()->getVmDirHelper().getMultiEditDispatcher()->unlock();
		////////////////////////////////////////////////////////////////////////
		// prepare response
		////////////////////////////////////////////////////////////////////////

		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse *pResponseCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

		CDspService::instance()->getVmDirHelper().UpdateHardDiskInformation(pVmConfig);

		CDspService::instance()->getVmDirHelper().getMultiEditDispatcher()->lock();
		CDspService::instance()->getVmDirHelper()
			.getMultiEditDispatcher()->registerBeginEdit(vm_uuid, pUserSession->getClientHandle(), pVmConfig );
		CDspService::instance()->getVmDirHelper().getMultiEditDispatcher()->unlock();

		pResponseCmd->SetVmConfig( pVmConfig->toString() );

		SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
		CDspService::instance()->getIOServer().sendPackage( sender, responsePkg );
	}
	catch (PRL_RESULT code)
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while getting VM configuration with code [%#x] (%s)"
			, code, PRL_RESULT_TO_STRING( code ) );
		CDspService::instance()->sendSimpleResponseToClient( sender, pkg, code );
	}
}

bool Task_EditVm::isNetworkRatesChanged(const CVmNetworkRates *oldRates, const CVmNetworkRates *newRates)
{
	if (!oldRates || !newRates)
		return false;

	if (oldRates->isRateBound() != newRates->isRateBound())
		return true;
	if (oldRates->m_lstNetworkRates.count() != newRates->m_lstNetworkRates.count())
		return true;

	foreach (CVmNetworkRate old_rate, oldRates->m_lstNetworkRates)
	{
		foreach (CVmNetworkRate new_rate, newRates->m_lstNetworkRates)
		{
			if (old_rate.getClassId() == new_rate.getClassId() &&
					old_rate.getRate() != new_rate.getRate())
				return true;
		}
	}
	return false;
}


PRL_RESULT Task_EditVm::mergeSampleConfig(
		SmartPtr<CVmConfiguration> pConfigNew,
		SmartPtr<CVmConfiguration> pConfigOld)
{
	PRL_RESULT res;

	QString sFile = VirtuozzoDirs::getVmConfigurationSamplePath(
			pConfigNew->getVmSettings()->getVmCommonOptions()->getConfigSampleName());

	if (!QFile::exists(sFile))
	{
		WRITE_TRACE(DBG_FATAL, "Sample Configuration file '%s' does not exist",
				QSTR2UTF8(sFile));
		return PRL_ERR_SAMPLE_CONFIG_NOT_FOUND;
	}

	SmartPtr<CVmConfiguration> pSampleConfig( new CVmConfiguration() );

	PRL_RESULT rc = CDspService::instance()->getVmConfigManager().loadConfig(
			pSampleConfig,
			sFile,
			SmartPtr<CDspClient>(0),
			true,
			false
			);
	if (!IS_OPERATION_SUCCEEDED( rc ))
		return PRL_ERR_PARSE_VM_CONFIG;

	/** Merge only limited set of sectionss **/
	SmartPtr<CVmConfiguration> pTmpConfig( new CVmConfiguration(pConfigOld.getImpl()));

	// HDD
	foreach(CVmHardDisk *pHddSample, pSampleConfig->getVmHardwareList()->m_lstHardDisks)
	{
		foreach(CVmHardDisk *pHdd, pConfigNew->getVmHardwareList()->m_lstHardDisks)
		{
			if( pHdd->getIndex() == pHddSample->getIndex() &&
					pHdd->getSize() != pHddSample->getSize() )
			{
				res = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
						pConfigNew->getVmIdentification()->getVmUuid(),
						getClient()->getVmDirectoryUuid(),
						PVE::DspCmdVmResizeDisk,
						getClient(),
						this->getJobUuid());
				if (PRL_FAILED(res))
					return res;

				SmartPtr<CDspClient> user = getClient();

				CProtoCommandPtr pRequest = CProtoSerializer::CreateVmResizeDiskImageProtoCommand(
						pConfigNew->getVmIdentification()->getVmUuid(),
						pHdd->getUserFriendlyName(),
						pHddSample->getSize(),
						0);
				SmartPtr<IOPackage> pPackage = DispatcherPackage::duplicateInstance(getRequestPackage(),
						pRequest->GetCommand()->toString());
				pPackage->header.type = PVE::DspCmdVmResizeDisk;

				SmartPtr<CDspTaskHelper> pTask(new Task_DiskImageResizer(user, pPackage,
							TASK_SKIP_LOCK | TASK_SKIP_SEND_RESPONSE));
				res = runExternalTask(pTask.get());

				/* unlock */
				CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
						getVmUuid(),
						getClient()->getVmDirectoryUuid(),
						PVE::DspCmdVmResizeDisk, getClient() );

				if( PRL_FAILED(res) )
				{
					getLastError()->fromString(pTask->getLastError()->toString());
					return res;
				}
			}

			if( operationIsCancelled() )
				return PRL_ERR_OPERATION_WAS_CANCELED;
		}
	}

	// CPU
	pTmpConfig->getVmHardwareList()->getCpu()->fromString(
			pSampleConfig->getVmHardwareList()->getCpu()->toString());
	// MEMORY
	pTmpConfig->getVmHardwareList()->getMemory()->fromString(
			pSampleConfig->getVmHardwareList()->getMemory()->toString());
	// VIDEO
	pTmpConfig->getVmHardwareList()->getVideo()->fromString(
			pSampleConfig->getVmHardwareList()->getVideo()->toString());
	// IOPS limit
	pTmpConfig->getVmSettings()->getVmRuntimeOptions()->setIopsLimit(
			pSampleConfig->getVmSettings()->getVmRuntimeOptions()->getIopsLimit());
	// IO Limit
	CVmIoLimit* l = pSampleConfig->getVmSettings()->getVmRuntimeOptions()->getIoLimit();

	if (l != NULL)
	{
		pTmpConfig->getVmSettings()->getVmRuntimeOptions()->setIoLimit(new CVmIoLimit(l));
		foreach(CVmHardDisk *h, pTmpConfig->getVmHardwareList()->m_lstHardDisks)
		{
			h->setIoLimit(new CVmIoLimit(l));
			h->setIopsLimit(pTmpConfig->getVmSettings()->getVmRuntimeOptions()->getIopsLimit());
		}
	}

	rc = pConfigNew->mergeDocuments(pTmpConfig.getImpl(), pConfigOld.getImpl());
	if (PRL_FAILED(rc))
		WRITE_TRACE( DBG_FATAL, "Merge sample finished with result %#x %s, '%s'",
				rc,
				PRL_RESULT_TO_STRING(rc ),
				QSTR2UTF8(pConfigNew->GetErrorMessage()) );

	return rc;
}

namespace
{

// glossary
// /ssd/vm1.pvm/disk1
//
// vm1.pvm - bundle name(or just name)
// /ssd - bundle location
// /ssd/vm1.pvm - bundle path(or just path)
// /ssd/vm1.pvm/disk1 - component path
// disk1 - component name
struct Bundle {

	Bundle(const QString &name);

	bool isComponent(const QString &path) const;
	// bundle path
	QString buildPath(const QString &location) const;
	// component path
	QString buildPath(const QPair<QString, QString> &parts) const;
	QString getVmName() const;
	QPair<QString, QString> splitComponent(const QString &path) const;
	QString getLocation(const QString &path) const;

	static Bundle createFromPath(const QString &path);
	// component path to bundle location and component name

	QString name;
};

Bundle::Bundle(const QString &a_name) : name(a_name)
{
}

Bundle Bundle::createFromPath(const QString &path)
{
	return Bundle(QFileInfo(path).fileName());
}

bool Bundle::isComponent(const QString &path) const
{
	QDir dir = QFileInfo(path).dir();
	return dir.dirName() == name;
}

QString Bundle::buildPath(const QString &location) const
{
	return location + QDir::separator() + name;
}

QString Bundle::getLocation(const QString &path) const
{
	return splitComponent(path).first;
}

QPair<QString, QString> Bundle::splitComponent(const QString &path) const
{
	PRL_ASSERT(isComponent(path));
	QPair<QString, QString> r;

	QDir dir = QFileInfo(path).dir();
	dir.cdUp();
	r.first = dir.absolutePath();

	r.second = QFileInfo(path).fileName();

	return r;
}

void CorrectDevicePathsInVmConfigCommon(
	CVmConfiguration *pVmConfig,
	const QString &sOldVmHomePath,
	const QString &sNewVmHomePath);

void CorrectDevicePaths(CVmDevice &vmDevice, const QString &sOldVmHomePath, const QString &sNewVmHomePath);

PRL_RESULT UpdateClusterResourceVm(
	SmartPtr<CVmConfiguration> &pVmConfigOld,
	SmartPtr<CVmConfiguration> &pVmConfigNew,
	const QString &newDirName,
	bool bVmWasRenamed)
{
	PRL_RESULT ret;
	CVmHighAvailability oldHa(pVmConfigOld->getVmSettings()->getHighAvailability()),
			    newHa(pVmConfigNew->getVmSettings()->getHighAvailability());

	// We consider only HA-enabled non-template VMs.
	oldHa.setEnabled(oldHa.isEnabled() &&
			 !pVmConfigOld->getVmSettings()->getVmCommonOptions()->isTemplate());
	newHa.setEnabled(newHa.isEnabled() &&
			 !pVmConfigNew->getVmSettings()->getVmCommonOptions()->isTemplate());

	const QString &oldVmName = pVmConfigOld->getVmIdentification()->getVmName(),
		      &newVmName = pVmConfigNew->getVmIdentification()->getVmName();

	// Below we use only the new name so we must rename it before.
	if (bVmWasRenamed && oldHa.isEnabled())
	{
		ret = CDspService::instance()->getHaClusterHelper()->renameClusterResource(
				oldVmName, &oldHa, newVmName, newDirName);
		if (PRL_FAILED(ret))
			return ret;
	}

	if (oldHa.toString() != newHa.toString())
	{
		ret = CDspService::instance()->getHaClusterHelper()->updateClusterResourceParams(
				newVmName, &oldHa, &newHa, newDirName);
		if (PRL_FAILED(ret))
			return ret;
	}
	return PRL_ERR_SUCCESS;
}

} // anonymous namespace


PRL_RESULT Task_EditVm::editVm()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	bool bNeedVNCStart = false;
	bool bNeedVNCStop = false;
	bool bNeedVNCReload = false;
	bool cloudCdRemoved = false;

	QString qsOldDirName;
	QString qsNewDirName;
	bool bVmWasRenamed = false;

	getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );

	// get userName to prevent deadlock;
	QString userName;
	{
		CDspLockedPointer< CDispUser >
			pLockedDispUser = CDspService::instance()->getDispConfigGuard()
			.getDispUserByUuid( getClient()->getUserSettingsUuid() );

		if( ! pLockedDispUser )
			return PRL_ERR_PARSE_USER_PROFILE;

		userName = pLockedDispUser->getUserName();
	}

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if ( ! cmd->IsValid() )
		return PRL_ERR_FAILURE;

	QString vm_config = cmd->GetFirstStrParam();

	//////////////////////////////////////////////////////////////////////////
	// parse VM configuration XML
	//////////////////////////////////////////////////////////////////////////
	SmartPtr<CVmConfiguration> pVmConfigNew( new CVmConfiguration( vm_config ) );
	SmartPtr<CVmConfiguration> pVmConfigOld;
	if( !IS_OPERATION_SUCCEEDED( pVmConfigNew->m_uiRcInit ) )
	{
		PRL_RESULT code = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Error occurred while modification VM configuration with code [%#x (%s)]"
			, code
			, PRL_RESULT_TO_STRING( code ) );
		return code;
	}

	// Extract VM Uuid
	QString vm_uuid = m_sVmUuid = pVmConfigNew->getVmIdentification()->getVmUuid();

	WRITE_TRACE( DBG_FATAL, "Commit vm configuration: vm_uuid='%s'" , QSTR2UTF8( vm_uuid ) );

	// Extract operations with confirmation
	QList<PRL_ALLOWED_VM_COMMAND> lstNewLockedOperations = pVmConfigNew->getVmSecurity()
		->getLockedOperationsList()->getLockedOperations();
	// Restrict editing hash
	QString newLockDownHash = pVmConfigNew->getVmSettings()->getLockDown()->getHash();

	// AccessCheck
	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	rc = CDspService::instance()->getAccessManager().checkAccess( getClient(), PVE::DspCmdDirVmEditCommit
		, vm_uuid, &bSetNotValid, getLastError() );
	if ( ! PRL_SUCCEEDED(rc) )
	{
		CDspVmDirHelper::sendNotValidState(getClient(), rc, vm_uuid, bSetNotValid);
		return rc;
	}
	CVmIdent ident(CDspVmDirHelper::getVmIdentByVmUuid(vm_uuid, getClient()));

	// We cannot edit suspended or suspending VM
	VIRTUAL_MACHINE_STATE nState = CDspVm::getVmState(ident);
	if (nState == VMS_SUSPENDED || nState == VMS_SUSPENDING || nState == VMS_SUSPENDING_SYNC)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot edit VM configuration in suspended or suspending state!");
		return PRL_ERR_CANT_EDIT_SUSPENDED_VM;
	}

	bool flgExclusiveOperationWasRegistered = false;
	bool flgExclusiveHardwareChangedWasRegistered = false;
	bool flgExclusiveFirewallChangedWasRegistered = false;

	try
	{
		ret = DspVm::vdh().registerExclusiveVmOperation(ident.first, ident.second,
			( PVE::IDispatcherCommands ) getRequestPackage()->header.type,
			getClient(),
			this->getJobUuid());

		if( PRL_FAILED( ret ) )
			throw ret;
		flgExclusiveOperationWasRegistered = true;

		pVmConfigOld = DspVm::vdh().getVmConfigByUuid(getClient(), vm_uuid, ret);
		if( ! pVmConfigOld )
		{
			PRL_ASSERT( PRL_FAILED( ret ) );
			if( !PRL_FAILED( ret) )
				ret = PRL_ERR_FAILURE;

			throw ret;
		}

		// Unite new and old format XML documents
		pVmConfigNew->uniteDocuments(pVmConfigOld->toString(), vm_config);

		// Check VM custom profile
		int res = CVmProfileHelper::check_vm_custom_profile(*CDspService::instance()->getHostInfo()->data(),
															*pVmConfigNew.getImpl(),
															*pVmConfigOld.getImpl());
		if (res >= 0)
			pVmConfigNew->getVmSettings()->getVmCommonOptions()->getProfile()->setCustom(res > 0);

		if (!pVmConfigNew->getVmSettings()->getVmCommonOptions()->getConfigSampleName().isEmpty())
		{

			ret = mergeSampleConfig(pVmConfigNew, pVmConfigOld);

			if (PRL_FAILED(ret))
				throw ret;
		}

		pVmConfigNew->getVmIdentification()->setCreationDate(
			pVmConfigOld->getVmIdentification()->getCreationDate() );

		//////////////////////////////////////////////////////////////////////////
		//  BEGIN: Rollback read only values.
		//////////////////////////////////////////////////////////////////////////

		// Clear sample config name, it is needed only during applyconfig operation.
		pVmConfigNew->getVmSettings()->getVmCommonOptions()->setConfigSampleName("");

		//Do not let change VM uptime through VM edit
		//https://bugzilla.sw.ru/show_bug.cgi?id=464218
		//XXX: seems we have here potential race with internal atomic changes
		pVmConfigNew->getVmIdentification()->setVmUptimeStartDateTime(
			pVmConfigOld->getVmIdentification()->getVmUptimeStartDateTime()
			);
		pVmConfigNew->getVmIdentification()->setVmUptimeInSeconds(
			pVmConfigOld->getVmIdentification()->getVmUptimeInSeconds()
			);
		//fast reboot
		pVmConfigNew->getVmSettings()->getVmStartupOptions()->setFastReboot(
			pVmConfigOld->getVmSettings()->getVmStartupOptions()->isFastReboot()
			);
		pVmConfigNew->getVmSettings()->getVmStartupOptions()->setVmFastRebootUser(
			pVmConfigOld->getVmSettings()->getVmStartupOptions()->getVmFastRebootUser()
			);
		//////////////////////////////////////////////////////////////////////////
		//  END: Rollback read only values.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		//  FINALIZE PREPARE PART. BEGIN TO CHECK CONFIG
		//////////////////////////////////////////////////////////////////////////

		bool bOldTemplate = pVmConfigOld->getVmSettings()->getVmCommonOptions()->isTemplate();
		if (bOldTemplate)
			nState = VMS_STOPPED;

		bool bNewTemplate = pVmConfigNew->getVmSettings()->getVmCommonOptions()->isTemplate();
		if ( ! bOldTemplate && bNewTemplate && nState != VMS_STOPPED )
		{
			throw PRL_ERR_VM_EDIT_UNABLE_CONVERT_TO_TEMPLATE;
		}

		CVmMemory *new_mem = pVmConfigNew->getVmHardwareList()->getMemory();
		unsigned new_mem_sz = new_mem->getRamSize();

		if (new_mem->isEnableHotplug() && new_mem_sz < 1024)
		{
			WRITE_TRACE(DBG_FATAL, "VM %s must have at least 1GB of RAM for memory hot-plugging support to be enabled.",
				qPrintable(vm_uuid));
			throw PRL_ERR_VMCONF_NEED_MORE_MEMORY_TO_ENABLE_HOTPLUG;
		}

		new_mem->setMaxNumaRamSize(getMaxNumaRamMb(new_mem_sz));
		new_mem->setMaxRamSize(getMaxTotalRamMb(new_mem_sz));

		PRL_UNDO_DISKS_MODE nOldUndoDisksMode
				= pVmConfigOld->getVmSettings()->getVmRuntimeOptions()->getUndoDisksMode();
		PRL_UNDO_DISKS_MODE nNewUndoDisksMode
				= pVmConfigNew->getVmSettings()->getVmRuntimeOptions()->getUndoDisksMode();
		if (   nOldUndoDisksMode == PUD_DISABLE_UNDO_DISKS
			&& nOldUndoDisksMode != nNewUndoDisksMode)
		{
			throw PRL_ERR_UNIMPLEMENTED;
		}

		//IP is changed?
		if (pVmConfigOld->getVmHardwareList()
			&& pVmConfigNew->getVmHardwareList())
		{
			foreach(CVmGenericNetworkAdapter *new_adapter, pVmConfigNew->getVmHardwareList()->m_lstNetworkAdapters)
			{
				if (!new_adapter)
					continue;

				//search adapter with the same index
				CVmGenericNetworkAdapter * old_adapter =
						PrlNet::GetAdapterByIndex(pVmConfigOld->getVmHardwareList()->m_lstNetworkAdapters,
										new_adapter->getIndex());

				if (!old_adapter)
				{
					new_adapter->setHostMacAddress();
					new_adapter->setHostInterfaceName(HostUtils::generateHostInterfaceName(new_adapter->getMacAddress()));
					continue;
				}

				if (old_adapter->getMacAddress() != new_adapter->getMacAddress())
				{
					if (VMS_STOPPED != nState)
						throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;

					new_adapter->setHostMacAddress();

					if (new_adapter->getHostInterfaceName() == HostUtils::generateHostInterfaceName(old_adapter->getMacAddress()))
					{
						new_adapter->setHostInterfaceName
							(HostUtils::generateHostInterfaceName(new_adapter->getMacAddress()));
					}
				}

				if (new_adapter->getHostInterfaceName().isEmpty())
				{
					new_adapter->setHostInterfaceName
							(HostUtils::generateHostInterfaceName(new_adapter->getMacAddress()));
				}

				// Remove duplicate IPs
				QStringList new_ips(new_adapter->getNetAddresses());

				new_ips.removeDuplicates();
				new_adapter->setNetAddresses(new_ips);

				if (!(old_adapter->getNetAddresses() == new_adapter->getNetAddresses())) {
					WRITE_TRACE(DBG_INFO, "IP will be changed for VM %s",
							QSTR2UTF8(vm_uuid));
				}
			}

			// #PSBM-13394
			CDspVmNetworkHelper::updateHostMacAddresses(pVmConfigNew, NULL, HMU_NONE);
		}

		//////////////////////////////////////////////////////////////////////////
		// check if output files of ports exist if no then try to create it
		//
		// NOTE:  We should create output files of ports before renaming vm,
		//        because inside it we compare old and new pathes to output files.
		// NOTE2: not throw by error
		//////////////////////////////////////////////////////////////////////////
		ret = createPortsOutputFiles( getClient(), pVmConfigNew, pVmConfigOld, getLastError(), PDE_SERIAL_PORT );
		if ( PRL_FAILED(ret) )
			WRITE_TRACE(DBG_FATAL, "Can't create createPortsOutputFiles (PDE_SERIAL_PORT) " );

		ret = createPortsOutputFiles( getClient(), pVmConfigNew, pVmConfigOld, getLastError(), PDE_PARALLEL_PORT );
		if ( PRL_FAILED(ret) )
			WRITE_TRACE(DBG_FATAL, "Can't create createPortsOutputFiles (PDE_PARALLEL_PORT) " );

		QString oldVmName;
		QString strVmHome;
		QString vmHomePath;
		CVmRemoteDisplay oldRemDisplay(pVmConfigOld->getVmSettings()->getVmRemoteDisplay());
		CVmRemoteDisplay* newRemDisplay = pVmConfigNew->getVmSettings()->getVmRemoteDisplay();
		{
			//
			// NOTE:	TO EXCLUDE DEADLOCK m_pVmConfigEdit mutex
			//			SHOULD be locked BEFORE CDspLockedPointer<..> from getVmDirManager().getXX().
			//
			QMutexLocker editLock(DspVm::vdh().getMultiEditDispatcher());

			// check to change config from other user
			PRL_RESULT nErrCode;
			if (DspVm::vdh().getMultiEditDispatcher()->configWasChanged(vm_uuid, getClient()->getClientHandle(), nErrCode))
			{
				if( PRL_FAILED(nErrCode) )
					throw nErrCode;

				QDateTime dtVmUptimeStartDateTime = pVmConfigOld->getVmIdentification()->getVmUptimeStartDateTime();
				quint64	  ullVmUptimeInSeconds = pVmConfigOld->getVmIdentification()->getVmUptimeInSeconds();

				DspVm::vdh().appendAdvancedParamsToVmConfig( getClient(), pVmConfigOld );

				//#464218 Do not let change VM uptime through VM edit
				pVmConfigOld->getVmIdentification()->setVmUptimeStartDateTime( dtVmUptimeStartDateTime );
				pVmConfigOld->getVmIdentification()->setVmUptimeInSeconds( ullVmUptimeInSeconds );

				if(!DspVm::vdh().getMultiEditDispatcher()
						->tryToMerge(vm_uuid, getClient()->getClientHandle(), pVmConfigNew, pVmConfigOld))
					throw PRL_ERR_VM_CONFIG_WAS_CHANGED;

				// cleanup params added before merge
				DspVm::vdh().resetAdvancedParamsFromVmConfig(pVmConfigOld); // todo  fix setVmFilesLocation()!!!!

				WRITE_TRACE( DBG_FATAL, "Config changes were successfully merged. vm = %s", QSTR2UTF8( vm_uuid ) );
			}

			////////////////////(((((((((((((((((//////////////////////////////////////////////////////
			// find VM in global VM hash
			//////////////////////////////////////////////////////////////////////////

			{
				//https://bugzilla.sw.ru/show_bug.cgi?id=267152
				CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

				CDspLockedPointer<CVmDirectoryItem>
					pVmDirItem = DspVm::vdm().getVmDirItemByUuid(ident);
				if ( ! pVmDirItem )
					throw PRL_ERR_VM_UUID_NOT_FOUND;

				// Since we use uuid in VM directory name EditVM is unable to change VmHome
				qsOldDirName = vmHomePath = CFileHelper::GetFileRoot(strVmHome = pVmDirItem->getVmHome());
				pVmDirItem.unlock();

				oldVmName = pVmConfigOld->getVmIdentification()->getVmName();

				// bug #8627
				if ( pVmConfigOld->getVmHardwareList()->toString()
					!= pVmConfigNew->getVmHardwareList()->toString() )
				{
					if ( nState == VMS_PAUSED )
					{
						throw CDspTaskFailure(*this)
							(PRL_ERR_CANNOT_EDIT_HARDWARE_FOR_PAUSED_VM,
							pVmConfigOld->getVmIdentification()->getVmName());
					}

					CVmHardware* pHardware_old = pVmConfigOld->getVmHardwareList();
					CVmHardware* pHardware_new = pVmConfigNew->getVmHardwareList();

					// PSBM-128392
					if (pHardware_old->getCpu()->getSockets() != pHardware_new->getCpu()->getSockets())
						flgExclusiveHardwareChangedWasRegistered = true;

					for (uint nType = PDE_GENERIC_DEVICE; nType < PDE_MAX; nType++ )
					{
						// bug #PSBM-5339
						if (nType == PDE_USB_DEVICE)
							continue;

						QList<void* >* devList_old = (QList<void* >* )pHardware_old->m_aDeviceLists[nType];
						QList<void* >* devList_new = (QList<void* >* )pHardware_new->m_aDeviceLists[nType];

						if ( (devList_old == NULL) != (devList_new == NULL) )
							flgExclusiveHardwareChangedWasRegistered = true;
						else if  ( devList_old == NULL || devList_new == NULL )
							continue;
						else
						{
							//Prevent attempts to either change read only during runtime settings of device or disconnect/connect non SATA HDDs
							QList<CVmDevice *> *pOldLstDevs = reinterpret_cast<QList<CVmDevice *> *>( devList_old );
							QList<CVmDevice *> *pNewLstDevs = reinterpret_cast<QList<CVmDevice *> *>( devList_new );
							foreach( CVmDevice *pOldDevice, *pOldLstDevs )
							{
								CVmDevice *pNewDevice = CXmlModelHelper::IsElemInList( pOldDevice, *pNewLstDevs );
								if ( ! pNewDevice )//Device was removed
								{
									if (PDE_OPTICAL_DISK == nType)
									{
										if (::Personalize::isCloudConfigCd(pOldDevice)) {
											::Personalize::Configurator(*pVmConfigNew).clean();
											cloudCdRemoved = true;
										}
									}

									if ( PVE::DeviceDisconnected == pOldDevice->getConnected() )//OK to remove disconnected device either for running VM or stopped
										continue;

									if ( PDE_PARALLEL_PORT == nType )
									{
										CVmParallelPort* pPrinter = dynamic_cast<CVmParallelPort *>( pOldDevice );
										PRL_ASSERT(pPrinter);
										if ( pPrinter )
										{
											// connected USB printer can be removed from either running VM or stopped
											if ( PRN_USB_DEVICE == pPrinter->getPrinterInterfaceType() )
												continue;
										}
									}
								}
								else
								{
									if ( CXmlModelHelper::IsEqual( pOldDevice, pNewDevice ) )
										continue;

									if ( CXmlModelHelper::JustConnectedPropWasChanged( pOldDevice, pNewDevice ) )
									{
										if (PDE_OPTICAL_DISK == nType &&
												::Personalize::isCloudConfigCd(pOldDevice) &&
												pNewDevice->getConnected() == PVE::DeviceDisconnected) {
												cloudCdRemoved = true;
										}
										if ( PDE_HARD_DISK != nType )
										//OK to change connected state for all devices without additional checkings except hard disks
											continue;

										CVmClusteredDevice *pClusteredDev = dynamic_cast<CVmClusteredDevice *>( pOldDevice );
										PRL_ASSERT(pClusteredDev);
										if ( pClusteredDev )
										{
											PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = pClusteredDev->getInterfaceType();
											// VIRTIO and SCSI devices can be disconnected either from a running VM or stopped
											if (PMS_VIRTIO_BLOCK_DEVICE == nIfaceType ||
													nIfaceType == PMS_SCSI_DEVICE)
												continue;
										}
									}
									else//Check that device state disconnected and readonly props weren't changed during VM runtime
									{
										if ( PVE::DeviceDisconnected == pOldDevice->getConnected() )
										{
											if ( pOldDevice->getEnabled() == pNewDevice->getEnabled() )//First non editable prop
											{
												CVmClusteredDevice *pOldClusteredDev = dynamic_cast<CVmClusteredDevice *>( pOldDevice );
												CVmClusteredDevice *pNewClusteredDev = dynamic_cast<CVmClusteredDevice *>( pNewDevice );
												if ( ! pOldClusteredDev || ! pNewClusteredDev )
													continue;

												if ( pOldClusteredDev->getStackIndex() == pNewClusteredDev->getStackIndex() &&//Second - stack index
													 pOldClusteredDev->getInterfaceType() == pNewClusteredDev->getInterfaceType() )//Third - interface type
													 continue;//All rest properties can be changed either VM running or stopped
											}
										}
									}
								}
								flgExclusiveHardwareChangedWasRegistered = true;
							}
							//Now check new devices addition - only SATA or USB devices can be added while VM running
							foreach( CVmDevice *pNewDevice, *pNewLstDevs )
							{
								if ( CXmlModelHelper::IsElemInList( pNewDevice, *pOldLstDevs ) )//Old device - ignore checks
									continue;

								// network adapters can be added either on running VM or stopped
								if (PDE_GENERIC_NETWORK_ADAPTER == nType)
									continue;

								if (nType == PDE_HARD_DISK)
								{
									CVmHardDisk *d = dynamic_cast<CVmHardDisk*>(pNewDevice);
									PRL_ASSERT(d);
									if (d && d->getSerialNumber().isEmpty() && d->getPassthrough() != PVE::PassthroughEnabled)
										d->setSerialNumber(Virtuozzo::generateDiskSerialNumber());

									// VIRTIO and SCSI devices can be added either on running VM or stopped
									if (PMS_VIRTIO_BLOCK_DEVICE == d->getInterfaceType() ||
											d->getInterfaceType() == PMS_SCSI_DEVICE)
										continue;
								}

								if ( PVE::DeviceDisconnected == pNewDevice->getConnected() )//Added disconnected device - ok
									continue;

								if (PDE_OPTICAL_DISK == nType)
								{
									CVmClusteredDevice *pClusteredDev = dynamic_cast<CVmClusteredDevice *>( pNewDevice );
									if (NULL == pClusteredDev)
									{
										PRL_ASSERT(false);
										continue;
									}
									if (VMS_STOPPED == nState)
										continue;
								}

								if ( PDE_PARALLEL_PORT == nType )
								{
									CVmParallelPort* pPrinter = dynamic_cast<CVmParallelPort *>( pNewDevice );
									PRL_ASSERT(pPrinter);
									if ( pPrinter )
									{
										// USB printer can be added to either running VM or stopped
										if ( PRN_USB_DEVICE == pPrinter->getPrinterInterfaceType() )
											continue;
									}
								}
								flgExclusiveHardwareChangedWasRegistered = true;
							}
						}
					}
					if (flgExclusiveHardwareChangedWasRegistered)
					{
						ret = DspVm::vdh().registerExclusiveVmOperation(
							ident.first, ident.second,
							PVE::DspCmdCtlVmEditWithHardwareChanged, getClient());
						if (PRL_FAILED(ret))
						{
							flgExclusiveHardwareChangedWasRegistered = false;
							throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
						}
					}
				}

				PRL_VM_START_LOGIN_MODE newLoginMode =
					pVmConfigNew->getVmSettings()->getVmStartupOptions()->getVmStartLoginMode();
				PRL_VM_START_LOGIN_MODE oldLoginMode =
					pVmConfigOld->getVmSettings()->getVmStartupOptions()->getVmStartLoginMode();

				if ((newLoginMode != PLM_START_ACCOUNT) && (newLoginMode != oldLoginMode))
				{
					WRITE_TRACE(DBG_FATAL, "An attempt to use Start-As-User feature!");
					throw PRL_ERR_UNIMPLEMENTED;
				}

			}

			// If EFI boot is enabled we need to have NVRAM full path.
			// New configuration may not have NVRAM path then old path is used.
			// Default name is used if old NVRAM path is empty.
			CVmStartupBios* bios = pVmConfigNew->getVmSettings()->getVmStartupOptions()->getBios();
			if (bios->isEfiEnabled() && bios->getNVRAM().isEmpty())
			{
				QString nvram = pVmConfigOld->getVmSettings()
					->getVmStartupOptions()->getBios()
					->getNVRAM();

				if (nvram.isEmpty())
					nvram = QDir(vmHomePath).absoluteFilePath(VZ_VM_NVRAM_FILE_NAME);
				else if (QFileInfo(nvram).isRelative())
					nvram = QDir(vmHomePath).absoluteFilePath(nvram);

				bios->setNVRAM(nvram);
			}


			//////////////////////////////////////////////////////////////////////////
			// if VM was renamed then it needs to rename VM home directory
			// if old VM home directory is the same old VM name
			//////////////////////////////////////////////////////////////////////////

			QString newVmName = pVmConfigNew->getVmIdentification()->getVmName();

			// if we are trying to rename VM - VM must be stopped first.
			if (newVmName != oldVmName)
			{
				if (nState != VMS_STOPPED && nState != VMS_SUSPENDED)
					throw PRL_ERR_VM_MUST_BE_STOPPED_BEFORE_RENAMING;

				WRITE_TRACE( DBG_FATAL, "Vm '%s' will be renamed to '%s'."
					, QSTR2UTF8(oldVmName),  QSTR2UTF8(newVmName) );

				ret = DspVm::vdh().registerExclusiveVmOperation(ident.first, ident.second,
					PVE::DspCmdCtlVmEditWithRename, getClient());
				if (PRL_FAILED(ret))
					throw ret;
				bVmWasRenamed = true;
			}

			if (newVmName != oldVmName)
			{

				// If directory placed in root of one of windows drives, like "C:/Windows 2003",
				// GetFileRoot() returns path with additional symbol "/", we need normalize path
				qsOldDirName.replace("//", "/");

				//lock to protect using new name on cloneVm/createVm/ another editVm operations
				SmartPtr<CVmDirectory::TemporaryCatalogueItem>
					pVmInfo( new CVmDirectory::TemporaryCatalogueItem( "" , "", newVmName) );

				PRL_RESULT lockResult = DspVm::vdm().checkAndLockNotExistsExclusiveVmParameters(
					getClient()->getVmDirectoryUuidList(),
					pVmInfo.getImpl()
					);

				if( PRL_FAILED( lockResult ) )
				{
					CDspTaskFailure f(*this);
					f.setCode(lockResult);
					switch ( lockResult )
					{
					case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
						throw f.setToken(pVmInfo->vmUuid)();

					case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
						throw f(pVmInfo->vmName, pVmInfo->vmXmlPath);

					case PRL_ERR_VM_ALREADY_REGISTERED:
					case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
						throw f(pVmInfo->vmName);

					case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default
					default:
						throw f.setToken(pVmInfo->vmUuid)
							.setToken(pVmInfo->vmXmlPath)
							.setToken(pVmInfo->vmName)();
					} //switch
				}

				DspVm::vdm().unlockExclusiveVmParameters(pVmInfo.getImpl());
			}


			//////////////////////////////////////////////////////////////////////////
			// Changed OS version
			//////////////////////////////////////////////////////////////////////////
			if ( pVmConfigNew->getVmSettings()->getVmCommonOptions()->getOsVersion()
				!= pVmConfigOld->getVmSettings()->getVmCommonOptions()->getOsVersion() )
			{
				QList<unsigned int > lstPrevOsVersions
					= pVmConfigNew->getVmSettings()->getVmCommonOptions()->getPrevOsVersions();
				lstPrevOsVersions.prepend( pVmConfigOld->getVmSettings()->getVmCommonOptions()->getOsVersion() );
				pVmConfigNew->getVmSettings()->getVmCommonOptions()->setPrevOsVersions( lstPrevOsVersions );
			}

			//////////////////////////////////////////////////////////////////////////
			// validate VM configuration
			//////////////////////////////////////////////////////////////////////////
			{
				//https://bugzilla.sw.ru/show_bug.cgi?id=267152
				CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

				CVmValidateConfig vc(getClient(), pVmConfigNew, pVmConfigOld);
				CVmEvent evtResult;

				if (vc.HasCriticalErrors(evtResult,
							(getRequestFlags() & PVCF_DESTROY_HDD_BUNDLE_FORCE) ?
								CVmValidateConfig::PCVAL_ALLOW_DESTROY_HDD_BUNDLE_WITH_SNAPSHOTS : 0))
				{
					WRITE_TRACE(DBG_FATAL, "Configuration validation failed" );
					LOG_MESSAGE( DBG_INFO, "CVmValidateConfig::validate() return true\n");

					// send reply to user
					getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
						evtResult.toString(),
						EVT_PARAM_COMPLEX_EVENT));
					throw PRL_ERR_INCONSISTENCY_VM_CONFIG;
				}

				//////////////////////////////////////////////////////////////////////////
				// check if user is authorized to access this VM
				//////////////////////////////////////////////////////////////////////////
				if ( ! CFileHelper::FileExists(strVmHome, &getClient()->getAuthHelper() ) ||
					! CFileHelper::FileCanWrite(strVmHome, &getClient()->getAuthHelper()) )
				{
					WRITE_TRACE(DBG_FATAL, ">>> User is not authorized to access this VM");
					throw PRL_ERR_ACCESS_DENIED;
				}

				//////////////////////////////////////////////////////////////////////////
				// check changing NUMA nodes and current VM parameters of CPU and memory
				//////////////////////////////////////////////////////////////////////////
				if (pVmConfigNew->getVmHardwareList()->getCpu()->getNumaNodes() != pVmConfigOld->getVmHardwareList()->getCpu()->getNumaNodes())
				{
					if (nState != VMS_STOPPED)
					{
						WRITE_TRACE(DBG_FATAL, "Unable to edit NUMA Nodes preferences for running VM %s.",
							qPrintable(vm_uuid));
						throw PRL_ERR_VMCONF_NUMANODES_VM_MUST_BE_STOPPED;
					}
				}
			}

			//https://jira.sw.ru/browse/PSBM-5219
			//Destroy removed HDDs bundles now - all necessary checkings whether VM running and etc
			//were done above where flgExclusiveHardwareChangedWasRegistered was set
			{
				Backup::Device::Event::EditVm *v = new Backup::Device::Event::EditVm(
					new Backup::Device::Agent::Unit(*this),
					getClient()->getVmDirectoryUuid());

				ret = Backup::Device::Service(pVmConfigOld)
					.setVmHome(qsNewDirName)
					.setVisitor(v)
					.setTopic(getLastError())
					.setDifference(pVmConfigNew);
				if (PRL_FAILED(ret))
					throw ret;

				QList<CVmHardDisk *> &lstNewHardDisks = pVmConfigNew->getVmHardwareList()->m_lstHardDisks;
				QList<CVmHardDisk *> &lstOldHardDisks = pVmConfigOld->getVmHardwareList()->m_lstHardDisks;
				foreach( CVmHardDisk *pOldHdd, lstOldHardDisks )
				{
					if ( ! CXmlModelHelper::IsElemInList( pOldHdd, lstNewHardDisks ) &&
						 (PVE::HardDiskEmulatedType)PDT_USE_IMAGE_FILE == pOldHdd->getEmulatedType() )
					{
						QString strDiskBundlePath = pOldHdd->getSystemName();

						if (PVCF_DESTROY_HDD_BUNDLE_FORCE & getRequestFlags())
						{
							ret = CDspService::instance()->getVmSnapshotStoreHelper().
									removeHddFromSnapshotTree(getClient(),
												vm_uuid, pOldHdd
												);
							if ( PRL_FAILED(ret) )
								throw ret;
						}

						// Remove from cache
						CDspService::instance()->getVmConfigManager().
							getHardDiskConfigCache().remove(strDiskBundlePath);

						if ( ((PVCF_DESTROY_HDD_BUNDLE|PVCF_DESTROY_HDD_BUNDLE_FORCE) & getRequestFlags()) )
						{
							CAuthHelper* a = &getClient()->getAuthHelper();
							WRITE_TRACE(DBG_FATAL, "Destroying HDD image '%s'", QSTR2UTF8(strDiskBundlePath));
							CFileHelper::RemoveEntry(strDiskBundlePath, a);
							Bundle b = Bundle::createFromPath(qsOldDirName);
							if (b.isComponent(strDiskBundlePath))
							{
								CFileHelper::RemoveEntry(b.buildPath(b.getLocation(strDiskBundlePath)),
										a);
							}
						}
					}

				}
			}

			//////////////////////////////////////////////////////////////////////////
			// reset additional parameters in VM configuration
			// (VM home, last change date, last modification date - never store in VM configuration itself!)
			// They were received from customer as new config
			//////////////////////////////////////////////////////////////////////////
			DspVm::vdh().resetAdvancedParamsFromVmConfig( pVmConfigNew );

			resetNetworkAddressesFromVmConfig( pVmConfigNew, pVmConfigOld );

			switch (newRemDisplay->getMode())
			{
			case PRD_DISABLED:
				bNeedVNCStop = oldRemDisplay.getMode() != PRD_DISABLED;
				break;
			case PRD_AUTO:
				if (PRD_AUTO == oldRemDisplay.getMode())
				{
					newRemDisplay->setPortNumber(oldRemDisplay.getPortNumber());
					newRemDisplay->setWebSocketPortNumber(oldRemDisplay.getWebSocketPortNumber());
				}
			default:
				switch (oldRemDisplay.getMode())
				{
				case PRD_DISABLED:
					bNeedVNCStart = true;
					break;
				//MANUAL mode allows to change VNC port in RUNNING VM. 
				//MANUAL and AUTO modes do not allow to change Hostname and Password for VNC
				case PRD_MANUAL:
				case PRD_AUTO:
					bNeedVNCStart = bNeedVNCStop = bNeedVNCStop ||
						oldRemDisplay.getHostName() != newRemDisplay->getHostName() ||
						oldRemDisplay.getPassword() != newRemDisplay->getPassword();
				}
			}

			if (newRemDisplay->getPortNumber() != oldRemDisplay.getPortNumber() &&
					newRemDisplay->getMode() != PRD_DISABLED && oldRemDisplay.getMode() != PRD_DISABLED)
				bNeedVNCReload = true;

			if (nState != VMS_STOPPED)
			{
				if (bVmWasRenamed)
				{
					WRITE_TRACE(DBG_FATAL, "Unable to change name for running VM %s.",
						qPrintable(vm_uuid));
					throw PRL_ERR_VM_MUST_BE_STOPPED_BEFORE_RENAMING;
				}
				/* We can't stop VNC server here, because main thread
				 * may issue cleanupAllBeginEditMarksByAccessToken, which
				 * will try to occure MultiEditDispatcher lock, but it is
				 * locked until we exit from section with editLock held.
				 * But function, which terminate VNC process wait for
				 * signal from main thread, it wait for timeout and
				 * exit with error. */
				if (bNeedVNCStop || bNeedVNCStart)
				{
					WRITE_TRACE(DBG_FATAL, "Unable to edit VNC preferences for running VM %s.",
						qPrintable(vm_uuid));
					throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
				}
			}

			quint32 t(CDspService::instance()->getDispConfigGuard().getDispConfig()
					->getDispatcherSettings()->getCommonPreferences()
					->getWorkspacePreferences()->getVmGuestCpuLimitType());

			pVmConfigNew->getVmHardwareList()->getCpu()->setGuestLimitType(t);

			//Do not let change VM uptime through VM edit
			//https://bugzilla.sw.ru/show_bug.cgi?id=464218
			//XXX: seems we have here potential race with internal atomic changes
			pVmConfigNew->getVmIdentification()->setVmUptimeStartDateTime(
				pVmConfigOld->getVmIdentification()->getVmUptimeStartDateTime()
			);
			pVmConfigNew->getVmIdentification()->setVmUptimeInSeconds(
				pVmConfigOld->getVmIdentification()->getVmUptimeInSeconds()
			);

			//Fill server UUID field because it information needing at VM check
			//access procedure
			pVmConfigNew->getVmIdentification()->setServerUuid(
				CDspService::instance()->getDispConfigGuard().getDispConfig()->
				getVmServerIdentification()->getServerUuid()
				);

			// High Availability Cluster
			//
			// handle VM only on shared FS - nfs, gfs, gfs2, pcs
			if (CFileHelper::isSharedFS(strVmHome))
			{
				ret = UpdateClusterResourceVm(pVmConfigOld, pVmConfigNew, vmHomePath, bVmWasRenamed);
				if (PRL_FAILED(ret))
					throw ret;
			}

			// save VM configuration to file
			//////////////////////////////////////////////////////////////////////////

			pVmConfigNew->getVmIdentification()->setHomePath(strVmHome);
			PRL_RESULT save_rc =
				CDspService::instance()->getVmConfigManager().saveConfig(pVmConfigNew,
				strVmHome,
				getClient(),
				true,
				true);

			if (!IS_OPERATION_SUCCEEDED(save_rc))
			{
				WRITE_TRACE(DBG_FATAL, "Dispatcher unable to save configuration "
					"of the VM %s to file %s. Reason: %ld(%s)",
					QSTR2UTF8( vm_uuid ),
					QSTR2UTF8( strVmHome ),
					Prl::GetLastError(),
					QSTR2UTF8( Prl::GetLastErrorAsString() )
					);

				// check error code - it may be not free space for save config
				if ( save_rc == PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_XML_SAVE )
					throw save_rc;

				throw CDspTaskFailure(*this).setCode(PRL_ERR_SAVE_VM_CONFIG)
					(newVmName, QFileInfo(strVmHome).path());
			}

			CXmlModelHelper::printConfigDiff( *pVmConfigNew, *pVmConfigOld, DBG_WARNING, "VmCfgCommitDiff" );

			// clear flags suspend and change for HDD images
			// clear suspend flag for disks
			CStatesHelper::SetSuspendParameterForAllDisks(pVmConfigNew.getImpl(),0);
			DspVm::vdh().getMultiEditDispatcher()->registerCommit(vm_uuid, getClient()->getClientHandle());
		}

#ifdef _LIBVIRT_
		Edit::Vm::driver_type(*this)(pVmConfigOld, pVmConfigNew);
		if(!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();
#endif // _LIBVIRT_

		// Edit firewall
		ret = editFirewall(pVmConfigNew, pVmConfigOld, nState, flgExclusiveFirewallChangedWasRegistered);
		if (PRL_FAILED(ret))
			throw ret;

		updateNetworkSettings(pVmConfigNew, pVmConfigOld);

		//////////////////////////////////////////////////////////////////////////
		// configure Vz specific parameters
		/////////////////////////////////////////////////////////////////////////
		configureVzParameters(pVmConfigNew, pVmConfigOld);
	}
	catch (PRL_RESULT code)
	{
		CDspTaskFailure f(*this);
		f.setCode(code);
		WRITE_TRACE(DBG_FATAL, "Error occurred while modification VM configuration with code [%#x (%s)]"
			, code
			, PRL_RESULT_TO_STRING( code ) );
		LOG_MESSAGE( DBG_INFO, "ws_error event:\n%s",
			QSTR2UTF8( getLastError()->toString() )
			);

		ret = code;

		switch (code)
		{
		case PRL_ERR_DISP_VM_IS_NOT_STOPPED:
		case PRL_ERR_VM_MUST_BE_STOPPED_BEFORE_RENAMING:
			f(pVmConfigOld->getVmIdentification()->getVmName());
		break;
		default:;
		}


		// NOTE:  DO NOT unregister begin edit mark BECAUSE client doesn't send BeginEdit again if commit fails
		// CDspService::instance()->getVmDirHelper()
		//	.getMultiEditDispatcher()->cleanupBeginEditMark( vm_uuid, getClient()->getClientHandle() );
	}

	if (flgExclusiveHardwareChangedWasRegistered)
	{
		DspVm::vdh().unregisterExclusiveVmOperation(ident.first, ident.second,
				PVE::DspCmdCtlVmEditWithHardwareChanged, getClient());
	}
	if (bVmWasRenamed)
	{
		DspVm::vdh().unregisterExclusiveVmOperation(ident.first, ident.second,
				PVE::DspCmdCtlVmEditWithRename, getClient());
	}

	if (flgExclusiveFirewallChangedWasRegistered)
	{
		DspVm::vdh().unregisterExclusiveVmOperation(ident.first, ident.second,
				PVE::DspCmdCtlVmEditFirewall, getClient());
	}
	if (flgExclusiveOperationWasRegistered)
	{
		DspVm::vdh().unregisterExclusiveVmOperation(ident.first, ident.second,
			(PVE::IDispatcherCommands)getRequestPackage()->header.type,
			getClient());
	}

	// Try to apply the new VM config if the VM is running
	if (PRL_SUCCEEDED(ret))
	{
		applyVmConfig(pVmConfigOld, pVmConfigNew);
		ret = getLastErrorCode();
		if (!cloudCdRemoved && CDspVm::getVmState(ident) == VMS_STOPPED) {
			CDspService::instance()->getVmStateSender()->onVmPersonalityChanged(
				ident.second, ident.first);
		}
	}

	//send signal about updated QEMU scheme for FRONTEND
	if (bNeedVNCReload)
		Libvirt::Kit.vms().at(vm_uuid).getMaintenance().emitQemuUpdated();

	//////////////////////////////////////////////////////////////////////////
	// Notify users that VM configuration was changed
	//////////////////////////////////////////////////////////////////////////

	// Generate "VM changed" event
	DspVm::vdh().sendVmConfigChangedEvent(ident, getRequestPackage());

	// If everything is OK, clenaup unused filters
#ifdef _LIBVIRT_
	if (PRL_SUCCEEDED(ret))
		ret = updateNetworkFilter(pVmConfigNew, pVmConfigOld);
#endif // _LIBVIRT_

	return ret;
}

PRL_RESULT Task_EditVm::updateNetworkFilter(
	const SmartPtr<CVmConfiguration> pNewVmConfig,
	const SmartPtr<CVmConfiguration> pOldVmConfig)
{
	if ( ! pNewVmConfig || ! pOldVmConfig )
		return PRL_ERR_SUCCESS;

	if ( pNewVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() &&
		pOldVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() )
		return PRL_ERR_SUCCESS;

	::Libvirt::Instrument::Agent::Filter::List filter_list(Libvirt::Kit.getLink());
	const QList<CVmGenericNetworkAdapter* >& old_adapters =
		pOldVmConfig->getVmHardwareList()->m_lstNetworkAdapters; 
	const QList<CVmGenericNetworkAdapter* >& new_adapters =
		pNewVmConfig->getVmHardwareList()->m_lstNetworkAdapters; 
	::Libvirt::Result filter_ret;

	// undefine all filters when VM is converted to template
	if ( pNewVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() )
	{
		if((filter_ret = filter_list.undefine(old_adapters)).isFailed())
			return filter_ret.error().code();
		return PRL_ERR_SUCCESS;
	}

	// define all filters when VM is converted from template
	if ( pOldVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() )
	{
		if ((filter_ret = filter_list.define(new_adapters)).isFailed())
			return filter_ret.error().code();
		return PRL_ERR_SUCCESS;
	}

	// undefine filters for removed adapters
	if ((filter_ret = filter_list.undefine_unused(old_adapters, new_adapters)).isFailed())
		return filter_ret.error().code();
	return PRL_ERR_SUCCESS;
}

// reset IP addresses for templates
void Task_EditVm::resetNetworkAddressesFromVmConfig(
	SmartPtr<CVmConfiguration> pNewVmConfig, SmartPtr<CVmConfiguration> pOldVmConfig )
{
	if ( ! pNewVmConfig || ! pOldVmConfig )
		return;

	if ( ! pNewVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() ||
		pNewVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() ==
		pOldVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() )
		return;

	for ( int i = 0; i < pNewVmConfig->getVmHardwareList()->m_lstNetworkAdapters.size(); i++ )
		pNewVmConfig->getVmHardwareList()->m_lstNetworkAdapters[i]->setNetAddresses();
}

// apply new networking setting on host if adapters are changed
void Task_EditVm::updateNetworkSettings(
	const SmartPtr<CVmConfiguration> pNewVmConfig,
	const SmartPtr<CVmConfiguration> pOldVmConfig)
{
	if ( ! pNewVmConfig || ! pOldVmConfig )
		return;

	// Change IP in DHCP server configurations, if necessary
	if (Task_ManagePrlNetService::extractHostOnlyIPAddressesFromVMConfiguration(pOldVmConfig) !=
		Task_ManagePrlNetService::extractHostOnlyIPAddressesFromVMConfiguration(pNewVmConfig))
	{
		Task_ManagePrlNetService::removeVmIPAddress(pOldVmConfig);
		Task_ManagePrlNetService::addVmIPAddress(pNewVmConfig);
	}

#ifdef _LIN_
	QList<CVmGenericNetworkAdapter* > &lstNew = pNewVmConfig->getVmHardwareList()->m_lstNetworkAdapters;
	QList<CVmGenericNetworkAdapter* > &lstOld = pOldVmConfig->getVmHardwareList()->m_lstNetworkAdapters;
	if ( lstNew == lstOld )
		return;

	QList<CVmGenericNetworkAdapter* > lstDrop;
	QList<CVmGenericNetworkAdapter* > lstAdd;
	int i;
	for ( i = 0; i < lstOld.size(); i++ )
	{
		bool bFound = false;
		foreach(CVmGenericNetworkAdapter* pAdapter, lstNew)
		{
			if (*pAdapter == *lstOld[i])
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			lstDrop.push_back(lstOld[i]);
	}

	for ( i = 0; i < lstNew.size(); i++ )
	{
		bool bFound = false;
		foreach(CVmGenericNetworkAdapter* pAdapter, lstOld)
		{
			if (*pAdapter == *lstNew[i])
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			lstAdd.push_back(lstNew[i]);
	}

	foreach(CVmGenericNetworkAdapter* pAdapter, lstDrop)
		Task_ManagePrlNetService::updateAdapter(*pAdapter, false);
	foreach(CVmGenericNetworkAdapter* pAdapter, lstAdd)
		Task_ManagePrlNetService::updateAdapter(*pAdapter, true);
#endif
}


/**
* Prepare applying a new config to a running VM.
*/
void Task_EditVm::applyVmConfig(SmartPtr<CVmConfiguration> old_, SmartPtr<CVmConfiguration> new_)
{
	CVmConfiguration x;
	Libvirt::Result e = Libvirt::Kit.vms().at(getVmUuid()).getConfig(x, true);
	if (e.isFailed())
		return;

	::Vm::Config::Patch::Runtime::drawAliases(*new_, x);
	DspVm::vdh().appendAdvancedParamsToVmConfig(getClient(), new_);
	Edit::Vm::Runtime::Driver(*this)(old_, new_);
}

PRL_RESULT Task_EditVm::configureVzParameters(SmartPtr<CVmConfiguration> pNewVmConfig,
					SmartPtr<CVmConfiguration> pOldVmConfig)
{
	bool bSet = !pOldVmConfig;

	QString sVmUuid = pNewVmConfig->getVmIdentification()->getVmUuid();
	// Configure for running VM only
	VIRTUAL_MACHINE_STATE s;
	Libvirt::Result r(Libvirt::Kit.vms().at(sVmUuid).getState().getValue(s));
	if (r.isFailed())
		return r.error().code();
	if (s != VMS_RUNNING)
		return PRL_ERR_SUCCESS;

	// Rate
	bool bSetRate = bSet;
	if (pOldVmConfig)
		bSetRate = isNetworkRatesChanged(pOldVmConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates(),
					pNewVmConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates());
	if (bSetRate)
		Task_NetworkShapingManagement::setNetworkRate(*pNewVmConfig);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_EditVm::createPortsOutputFiles(
	SmartPtr<CDspClient> pUserSession,
	SmartPtr<CVmConfiguration> pVmConfigNew,
	SmartPtr<CVmConfiguration> pVmConfigOld,
	CVmEvent  *pWSError,
	PRL_DEVICE_TYPE portType)
{
	PRL_ASSERT( PDE_SERIAL_PORT==portType || PDE_PARALLEL_PORT==portType );

	PRL_RESULT errCantCreate = PRL_ERR_FIXME;
	if( PDE_SERIAL_PORT==portType )
		errCantCreate = PRL_ERR_CANT_CREATE_SERIAL_PORT_IMAGE;
	else if( PDE_PARALLEL_PORT==portType )
		errCantCreate = PRL_ERR_CANT_CREATE_PARALLEL_PORT_IMAGE;
	else
		return PRL_ERR_INVALID_ARG;

	typedef QString			FullPathType;
	QMultiHash< FullPathType, CVmPort* > oldPorts;
	if( QList<CVmPort* >* pList = (QList<CVmPort* >* )pVmConfigOld->getVmHardwareList()->m_aDeviceLists[ portType ] )
	{
		foreach( CVmPort* p, *pList )
		{
			PRL_ASSERT( p );
			oldPorts.insertMulti( p->getSystemName(), p );
		}
	}

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pUserSession->getAuthHelper() );

	if( QList<CVmPort* >* pList = (QList<CVmPort* >* )pVmConfigNew->getVmHardwareList()->m_aDeviceLists[ portType ]  )
	{
		foreach( CVmPort* pPort, *pList )
		{
			if( pPort->getEnabled() != PVE::DeviceEnabled )
				continue;

			if( PDE_SERIAL_PORT==portType )
			{
				if( ((CVmSerialPort*)pPort)->getEmulatedType() != PVE::SerialOutputFile )
					continue;
			}
			else if( PDE_PARALLEL_PORT==portType )
			{
				if( ((CVmParallelPort*)pPort)->getEmulatedType() != PVE::ParallelOutputFile )
					continue;
			}else
				PRL_ASSERT("Some shit" == NULL );

			/* Validate ports output file path */
			QString strFullPath = pPort->getSystemName();

			PRL_RESULT ret = createPortOutputFile( strFullPath, pUserSession );
			if ( PRL_FAILED(ret) )
			{
				bool flgPortNotChanged = false;
				if( oldPorts.contains( strFullPath ) )
				{
					const QString newPort = pPort->toString();
					foreach( const CVmPort* port, oldPorts.values( strFullPath ) )
					{
						if( newPort == port->toString() )
						{
							flgPortNotChanged = true;
							break;
						}
					}
				}
				if( flgPortNotChanged )
				{
					WRITE_TRACE(DBG_FATAL, "Port not changed, but createPortOutputFile() failed. "
						"This error would be ignored. For vm=(uuid=%s), port_path='%s' error=%#x(%s)"
						, QSTR2UTF8( pVmConfigNew->getVmIdentification()->getVmUuid() )
						, QSTR2UTF8( strFullPath )
						, ret
						, PRL_RESULT_TO_STRING( ret )
						);
					continue;
				}

				if ( ret == PRL_ERR_OPERATION_FAILED )
				{
					ret = errCantCreate;

					pWSError->addEventParameter(
						new CVmEventParameter(PVE::String, strFullPath, EVT_PARAM_MESSAGE_PARAM_0) );
				}

				pWSError->setEventCode( ret );
				return ret;
			}//if
		}//foreach
	}//if
	return PRL_ERR_SUCCESS;
}

/**
* Create a port output file.
* @param strFullPath An output file full path
* @param pUserSession An user creating file
* @return Error code
*/
PRL_RESULT Task_EditVm::createPortOutputFile( const QString& strFullPath
												 , SmartPtr<CDspClient> pUserSession )
{
	if ( !CFileHelper::FileExists(strFullPath, &pUserSession->getAuthHelper()) )
	{
		// Check directory
		QString strDir = CFileHelper::GetFileRoot( strFullPath );

		if ( !CFileHelper::DirectoryExists(strDir, &pUserSession->getAuthHelper()) )
		{
			if ( !CFileHelper::WriteDirectory(strDir, &pUserSession->getAuthHelper()) )
			{
				WRITE_TRACE(DBG_FATAL,
					"CDspVmDirHelper::createPortOutputFile() : Can not create directory [%s].",
					QSTR2UTF8(strDir) );
				return PRL_ERR_MAKE_DIRECTORY;
			}
		}
		// Try to create blank file
		if ( !CFileHelper::CreateBlankFile(strFullPath, &pUserSession->getAuthHelper()) )
		{
			WRITE_TRACE(DBG_FATAL,
				"CDspVmDirHelper::createPortOutputFile() : Can not create a file [%s].",
				QSTR2UTF8(strFullPath) );
			return PRL_ERR_OPERATION_FAILED;
		}
	}
	return PRL_ERR_SUCCESS;
}

namespace {
	/**
	* Corrects specified path with new VM name
	* @param path to correct
	* @param old VM home path
	* @param new VM home path
	* @return corrected value
	*/
	QString BuildDevicePath(const QString &sDevicePath, const QString &sOldVmHomePath, const QString &sNewVmHomePath)
	{
		if (!QFileInfo(sDevicePath).isAbsolute())
			return sDevicePath;
		QString sCorrectedPath = QFileInfo(sDevicePath).absoluteFilePath();
		QString sNormalizedOldVmHomePath = QFileInfo(sOldVmHomePath).absoluteFilePath();
		QString sNormalizedNewVmHomePath = QFileInfo(sNewVmHomePath).absoluteFilePath();
		Qt::CaseSensitivity caseSens = Qt::CaseSensitive;

#ifndef _LIN_
		caseSens = Qt::CaseInsensitive;
#endif

		if (sCorrectedPath.startsWith(sNormalizedOldVmHomePath, caseSens))
			sCorrectedPath =
				sCorrectedPath.replace(0, sNormalizedOldVmHomePath.size(), sNormalizedNewVmHomePath);
		return (sCorrectedPath);
	}

	void CorrectEfiPath(CVmStartupBios& bios_, const QString& oldHome_, const QString& newHome_)
	{
		bios_.setNVRAM(BuildDevicePath(bios_.getNVRAM(), oldHome_, newHome_));
	}

	/**
	* Corrects device paths in view of VM name was changed
	* @param pointer to device object
	* @param old VM home path
	* @param new VM home path
	*/
	void CorrectDevicePaths(CVmDevice &vmDevice, const QString &sOldVmHomePath, const QString &sNewVmHomePath)
	{
		// #127869
		bool bIsRealHdd = ( vmDevice.getDeviceType() == PDE_HARD_DISK)
				&& ( vmDevice.getEmulatedType() == PDT_USE_REAL_HDD);

		if ( bIsRealHdd
			|| vmDevice.getEmulatedType() == PDT_USE_IMAGE_FILE
			|| vmDevice.getEmulatedType() == PDT_USE_OUTPUT_FILE
			|| (vmDevice.getEmulatedType() == PVE::SerialSocket &&
				vmDevice.getDeviceType() == PDE_SERIAL_PORT))
		{
			if( !bIsRealHdd )
				vmDevice.setUserFriendlyName(BuildDevicePath(vmDevice.getUserFriendlyName(),
							sOldVmHomePath, sNewVmHomePath));

			vmDevice.setSystemName(BuildDevicePath(vmDevice.getSystemName(), sOldVmHomePath, sNewVmHomePath));
		}
	}

	void CorrectDevicePathsInVmConfigCommon(
		CVmConfiguration* pVmConfig,
		const QString &sOldVmHomePath,
		const QString &sNewVmHomePath)
	{
		//Process hardware list now in order to change target devices paths
		foreach(CVmOpticalDisk *pOpticalDisk, pVmConfig->getVmHardwareList()->m_lstOpticalDisks)
			CorrectDevicePaths(*pOpticalDisk, sOldVmHomePath, sNewVmHomePath);
		foreach(CVmFloppyDisk *pFloppyDisk, pVmConfig->getVmHardwareList()->m_lstFloppyDisks)
			CorrectDevicePaths(*pFloppyDisk, sOldVmHomePath, sNewVmHomePath);
		foreach(CVmSerialPort *pSerialPort, pVmConfig->getVmHardwareList()->m_lstSerialPorts)
			CorrectDevicePaths(*pSerialPort, sOldVmHomePath, sNewVmHomePath);
		foreach(CVmParallelPort *pParallelPort, pVmConfig->getVmHardwareList()->m_lstParallelPorts)
			CorrectDevicePaths(*pParallelPort, sOldVmHomePath, sNewVmHomePath);
		CorrectEfiPath(*(pVmConfig->getVmSettings()->getVmStartupOptions()->getBios()),
				sOldVmHomePath, sNewVmHomePath);
	}

} // namespace

/**
* Corrects device paths in view of VM name was changed
* @param old VM name
* @param new VM name
*/
void Task_EditVm::CorrectDevicePathsInVmConfig(
	SmartPtr<CVmConfiguration> pVmConfig,
	const QString &sOldVmHomePath,
	const QString &sNewVmHomePath)
{
	CorrectDevicePathsInVmConfigCommon(pVmConfig.get(), sOldVmHomePath, sNewVmHomePath);
	foreach(CVmHardDisk *pHardDisk, pVmConfig->getVmHardwareList()->m_lstHardDisks)
		CorrectDevicePaths(*pHardDisk, sOldVmHomePath, sNewVmHomePath);
}

PRL_RESULT Task_EditVm::editFirewall(SmartPtr<CVmConfiguration> pVmConfigNew,
									 SmartPtr<CVmConfiguration> pVmConfigOld,
									 VIRTUAL_MACHINE_STATE nState,
									 bool& flgExclusiveFirewallChangedWasRegistered)
{
	QStringList lstFullItemIds;
	pVmConfigNew->diffDocuments(pVmConfigOld.getImpl(), lstFullItemIds);
	QString qsDiff = lstFullItemIds.join(" ");
	if ( ! qsDiff.contains(".Firewall.") )
		return PRL_ERR_SUCCESS;

	if (nState != VMS_RUNNING && nState != VMS_PAUSED)
	{
		PRL_RESULT ret = CDspService::instance()->getVmDirHelper()
			.registerExclusiveVmOperation(pVmConfigNew->getVmIdentification()->getVmUuid(),
											getClient()->getVmDirectoryUuid(),
			PVE::DspCmdCtlVmEditFirewall, getClient());
		if (PRL_FAILED(ret))
			return PRL_ERR_CANNOT_EDIT_FIREWALL_AT_TRANS_VM_STATE;
		flgExclusiveFirewallChangedWasRegistered = true;

		return PRL_ERR_SUCCESS;
	}

	CFirewallHelper fw(*pVmConfigNew);

	PRL_RESULT ret = fw.Execute();
	if (PRL_FAILED(ret))
	{
		if ( ret == PRL_ERR_FIREWALL_TOOL_EXECUTED_WITH_ERROR )
		{
			getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String,
				fw.GetErrorMessage(),
				EVT_PARAM_DETAIL_DESCRIPTION )
				);
		}
		return ret;
	}

	return PRL_ERR_SUCCESS;
}

namespace Edit
{
namespace Vm
{
namespace vm = Libvirt::Instrument::Agent::Vm;

///////////////////////////////////////////////////////////////////////////////
// struct Action

Action::~Action()
{
}

bool Action::execute(CDspTaskFailure& feedback_)
{
	return m_next.isNull() || m_next->execute(feedback_);
}

Action& Action::getTail()
{
	Action* a = this;
	while (!a->m_next.isNull())
		a = a->m_next.data();

	return *a;
}

///////////////////////////////////////////////////////////////////////////////
// struct Reconnect

bool Reconnect::execute(CDspTaskFailure& feedback_)
{
	Libvirt::Instrument::Agent::Network::Unit u;
	Libvirt::Result r = Libvirt::Kit.networks().find(m_network, &u);

	if (r.isFailed())
	{
		feedback_(r.error().convertToEvent());
		return false;
	}

	// find network
	CVirtualNetwork n;
	r = u.getConfig(n);
	if (r.isFailed())
	{
		feedback_(r.error().convertToEvent());
		return false;
	}

	// need network's bridge name CVirtuozzoAdapter::getName()
	CVirtuozzoAdapter* a;
	if (NULL == n.getHostOnlyNetwork()
		|| (a = n.getHostOnlyNetwork()->getVirtuozzoAdapter()) == NULL)
	{
		feedback_(PRL_NET_VIRTUAL_NETWORK_NOT_FOUND);
		return false;
	}

	// connect iface to bridge
	if (!PrlNet::connectInterface(m_adapter, a->getName()))
	{
		feedback_(PRL_ERR_SET_NETWORK_SETTINGS_FAILED);
		return false;
	}

	return Action::execute(feedback_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Request

Request::Request(Task_EditVm& task_, const config_type& start_, const config_type& final_)
	: m_task(&task_), m_start(start_), m_final(final_)
{
	CVmIdentification* x = final_->getVmIdentification();
	if (NULL == x)
		return;

	QString u = x->getVmUuid();
	SmartPtr<CDspClient> a = task_.getClient();
	if (a.isValid())
		m_object = CDspVmDirHelper::getVmIdentByVmUuid(u, a);
	else
		m_object = MakeVmIdent(u, QString());
}

namespace Config
{
///////////////////////////////////////////////////////////////////////////////
// struct Magic

Magic::detector_type* Magic::craftDetector(agent_type agent_)
{
	QString u;
	Libvirt::Result e = agent_.getUuid(u);
	if (e.isFailed())
		return NULL;

	return new detector_type(u);
}

///////////////////////////////////////////////////////////////////////////////
// struct Novel

Novel::result_type Novel::operator()(load_type load_)
{
	if (NULL == load_.second)
		return Libvirt::Result(PRL_ERR_INVALID_ARG);

	return Libvirt::Kit.vms().getGrub(*load_.second).spawnPersistent();
}

Novel::detector_type* Novel::craftDetector(const load_type& load_)
{
	if (NULL == load_.second)
		return NULL;

	return new detector_type
		(load_.second->getVmIdentification()->getVmUuid());
}

///////////////////////////////////////////////////////////////////////////////
// struct Update

Update::result_type Update::operator()(load_type load_)
{
	if (NULL == load_.second)
		return Libvirt::Result(PRL_ERR_INVALID_ARG);

	return load_.first.setConfig(*load_.second);
}

Update::detector_type* Update::craftDetector(const load_type& load_)
{
	return Magic::craftDetector(load_.first);
}

///////////////////////////////////////////////////////////////////////////////
// struct Event

Event::result_type Event::operator()(load_type load_)
{
	load_.getMaintenance().emitDefined();
	return result_type();
}

} // namespace Config

///////////////////////////////////////////////////////////////////////////////
// struct Transfer

bool Transfer::execute(CDspTaskFailure& feedback_)
{
	QString vmDir(CDspService::instance()->getDispConfigGuard()
			.getDispWorkSpacePrefs()->getDefaultVmDirectory());
	QScopedPointer<CVmDirectoryItem> t;
	{
		CDspLockedPointer<CVmDirectoryItem> vmItem(
			DspVm::vdh().getVmDirectoryItemByUuid
				(m_object.second, m_object.first));
		if (!vmItem) {
			WRITE_TRACE(DBG_FATAL, "vmItem %s is absent", QSTR2UTF8(m_object.first));
			return Action::execute(feedback_);
		}
		t.reset(new CVmDirectoryItem(*vmItem));
	}
	PRL_RESULT res(DspVm::vdh().deleteVmDirectoryItem(m_object.second, m_object.first));
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "Unable to remove directory item (%s)", PRL_RESULT_TO_STRING(res));
		return Action::execute(feedback_);
	}
	DspVm::vdh().sendVmEventToAll(m_object, PET_DSP_EVT_VM_UNREGISTERED);

	t->setCtId(m_target == DspVm::vdm().getTemplatesDirectoryUuid() ? m_target : QString());
	t->setTemplate(m_target == DspVm::vdm().getTemplatesDirectoryUuid());
	res = DspVm::vdh().insertVmDirectoryItem(m_target, t.data());
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "Unable to add directory item (%s)", PRL_RESULT_TO_STRING(res));
		return Action::execute(feedback_);
	}

	t.take();

	DspVm::vdh().sendVmEventToAll(m_object, PET_DSP_EVT_VM_ADDED);

	return Action::execute(feedback_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Layout

Layout::Layout(const CVmConfiguration& vm_):
	m_home(vm_.getVmIdentification()->getHomePath())
{
}

QString Layout::getHome() const
{
	return QFileInfo(m_home).absolutePath();
}

QString Layout::getItemPath(const QString& item_) const
{
	if (QFileInfo(item_).isRelative())
		return QDir(getHome()).absoluteFilePath(item_);

	return item_;
}

///////////////////////////////////////////////////////////////////////////////
// struct Patch

Patch::Patch(const Request& input_): Layout(input_.getFinal()),
	m_name(input_.getFinal().getVmIdentification()->getVmName()),
	m_ident(input_.getObject())
{
	CDspLockedPointer<CDispUser> u = CDspService::instance()->getDispConfigGuard()
		.getDispUserByUuid(input_.getTask().getClient()->getUserSettingsUuid());

	if(u.isValid())
		m_editor = u->getUserName();
}

bool Patch::execute(CDspTaskFailure& feedback_)
{
	{
		CDspVmDirManager& dirManager = DspVm::vdm();
		CDspLockedPointer<CVmDirectoryItem> dirItem = dirManager
			.getVmDirItemByUuid(m_ident.second, m_ident.first);

		if (!dirItem)
		{
			feedback_(PRL_ERR_VM_DIRECTORY_NOT_EXIST);
			return false;
		}

		// #441667 - set the same parameters for shared vm
		CDspVmDirManager::VmDirItemsHash sharedVmHash = dirManager
			.findVmDirItemsInCatalogue(dirItem->getVmUuid(), dirItem->getVmHome());

		foreach(CDspLockedPointer<CVmDirectoryItem> dirSharedItem, sharedVmHash)
		{
			dirSharedItem->setChangedBy(m_editor);
			dirSharedItem->setChangeDateTime(QDateTime::currentDateTime());
			dirSharedItem->setVmName(m_name);
			dirSharedItem->setVmHome(getConfig());

			/* old code
			pVmDirSharedItem->getLockedOperationsList()->setLockedOperations(lstNewLockedOperations);
			pVmDirSharedItem->getLockDown()->setEditingPasswordHash(newLockDownHash);
			*/
		}
		dirItem->setVmHome(getConfig());
		dirItem->setVmName(m_name);

		PRL_RESULT ret = dirManager.updateVmDirItem(dirItem);
		if (PRL_FAILED(ret))
		{
			WRITE_TRACE(DBG_FATAL, "Can't update VmCatalogue by error %#x, %s", ret, PRL_RESULT_TO_STRING(ret));
			feedback_(ret);
			return false;
		}
	}
	return Vm::Action::execute(feedback_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Apply

Action* Apply::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* n = NULL, *output = new Patch(input_);
	bool a = input_.getStart().getVmSettings()->getVmCommonOptions()->isTemplate();
	bool b = input_.getFinal().getVmSettings()->getVmCommonOptions()->isTemplate();
	if (a != b)
	{
		QString t;
		if (b)
		{
			n = f.craftState(boost::bind(&vm::Limb::State::undefine, _1));
			t = DspVm::vdm().getTemplatesDirectoryUuid();
		}
		else
		{
			n = f.craft(Config::Facade<Config::Novel>(input_.getFinal()));
			t = input_.getTask().getClient()->getVmDirectoryUuid();
		}
		output->setNext(new Transfer(input_.getObject(), t));
		output->getTail().setNext(n);
		return output;
	}
	if (b)
		return output;

	QString x = input_.getStart().getVmIdentification()->getVmName();
	QString y = input_.getFinal().getVmIdentification()->getVmName();
	n = f.craft(Config::Facade<Config::Update>(input_.getFinal()));
	if (x != y)
		output->setNext(f.craft(boost::bind(&vm::Unit::rename, _1, y)));

	output->getTail().setNext(n);
	return output;
}

namespace Create
{
///////////////////////////////////////////////////////////////////////////////
// struct Nvram

Vm::Action* Nvram::operator()(const Request& input_) const
{
	CVmStartupBios* n = input_.getFinal().getVmSettings()->getVmStartupOptions()->getBios();

	if (n == NULL || !n->isEfiEnabled())
		return NULL;

	CVmStartupBios* o = input_.getStart().getVmSettings()->getVmStartupOptions()->getBios();

	if (o->isEfiEnabled() && o->getNVRAM() == n->getNVRAM())
		return NULL;

	return new Action<CVmStartupBios>(*n, input_.getFinal());
}

///////////////////////////////////////////////////////////////////////////////
// struct Action

template <>
PRL_RESULT Action<CVmStartupBios>::execute()
{
	QString file = this->getItemPath(m_data.getNVRAM());
	if (QFile::exists(file))
		return PRL_ERR_SUCCESS;

	const unsigned int c = m_config.getVmHardwareList()->getChipset()->getType();

	QString templ = OVMF::getTemplate(static_cast<Chipset_type>(c));

	if (0 != QProcess::execute(QEMU_IMG_BIN, QStringList() << "convert"<< "-O" << "qcow2" << templ << file))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to create NVRAM image with '%s'", qPrintable(file));
		return PRL_ERR_NVRAM_FILE_COPY;
	}

	return PRL_ERR_SUCCESS;
}

template <>
bool Action<CVmStartupBios>::execute(CDspTaskFailure& feedback_)
{
	PRL_RESULT r = execute();

	if (PRL_SUCCEEDED(r))
		return Vm::Action::execute(feedback_);

	const unsigned int c = m_config.getVmHardwareList()->getChipset()->getType();

	QString templ = OVMF::getTemplate(static_cast<Chipset_type>(c));

	feedback_.setCode(r)(templ, this->getItemPath(m_data.getNVRAM()));
	return false;
}

} // namespace Create

namespace Runtime
{
///////////////////////////////////////////////////////////////////////////////
// struct NotApplied

bool NotApplied::execute(CDspTaskFailure& feedback_)
{
	CVmEvent e(PET_DSP_EVT_VM_CONFIG_APPLIED, m_vmUuid, PIE_VIRTUAL_MACHINE);
	e.setEventCode(PRL_ERR_VM_APPLY_CONFIG_NEEDS_REBOOT);
	e.addEventParameter(new CVmEventParameter(PVE::String, m_vmUuid, EVT_PARAM_VM_UUID));
	e.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
				QString::number((qint32)PRL_ERR_VM_APPLY_CONFIG_NEEDS_REBOOT),
				EVT_PARAM_OP_RC));

	m_session->sendPackage(DispatcherPackage::createInstance(PVE::DspVmEvent, e.toString()));

	return Action::execute(feedback_);
}

///////////////////////////////////////////////////////////////////////////////
// struct ChangeableMedia

template<>
QList<CVmOpticalDisk* > ChangeableMedia<CVmOpticalDisk>::getList(const CVmHardware* hardware_)
{
	QList<CVmOpticalDisk* > output = hardware_->m_lstOpticalDisks;
	QList<CVmOpticalDisk* >::iterator b = std::partition(output.begin(), output.end(),
		boost::bind(&CVmOpticalDisk::getEmulatedType, _1) == PVE::CdRomImage);
	output.erase(b, output.end());

	return output;
}

template<>
QList<CVmFloppyDisk* > ChangeableMedia<CVmFloppyDisk>::getList(const CVmHardware* hardware_)
{
	QList<CVmFloppyDisk* > output = hardware_->m_lstFloppyDisks;
	QList<CVmFloppyDisk* >::iterator b = std::partition(output.begin(), output.end(),
		boost::bind(&CVmFloppyDisk::getEmulatedType, _1) == PVE::FloppyDiskImage);
	output.erase(b, output.end());

	return output;
}

template<class T>
Action* ChangeableMedia<T>::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* output = NULL;
	QList<T* > o = getList(input_.getStart().getVmHardwareList());
	foreach (T* d, getList(input_.getFinal().getVmHardwareList()))
	{
		T* x = CXmlModelHelper::IsElemInList(d, o);
		if (NULL == x)
			continue;

		Action* a = NULL;
		if (x->getSystemName() != d->getSystemName())
		{
			a = f.craftRuntime(boost::bind
				(&vm::Editor::update<T>, _1, *d));
		}
		else if (x->getConnected() != d->getConnected())
		{
			a = f.craftRuntime(boost::bind
				(&vm::Editor::update<T>, _1, *d));
		}
		else
			continue;

		a->setNext(output);
		output = a;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct RemoteDesktop
Action* RemoteDesktop::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* output = NULL;

	CVmRemoteDisplay* oldVnc = input_.getStart().getVmSettings()->getVmRemoteDisplay();
	CVmRemoteDisplay* newVnc = input_.getFinal().getVmSettings()->getVmRemoteDisplay();

	//check changes, it should be only change of port number
	{
		if (!oldVnc || !newVnc)
			return output;

		if (newVnc->getMode() != PRD_MANUAL)
			return output;

		if (newVnc->getPortNumber() == oldVnc->getPortNumber())
			return output;

		if (oldVnc->getPassword() != newVnc->getPassword() || oldVnc->getHostName() != newVnc->getHostName())
			return output;
	}

	Action* a(f.craftRuntime(boost::bind(&vm::Editor::updateVncPort, _1, newVnc)));
	a->setNext(output);
	output = a;
	
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Adapter

Action* Adapter::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* output = NULL;
	QList<CVmGenericNetworkAdapter*> o = input_.getStart().getVmHardwareList()->m_lstNetworkAdapters;
	foreach (CVmGenericNetworkAdapter* d, input_.getFinal().getVmHardwareList()->m_lstNetworkAdapters)
	{
		// works without any action because post-configuration deletes iface from bridge
		if (d->getEmulatedType() == PNA_ROUTED)
			continue;

		CVmGenericNetworkAdapter* x = CXmlModelHelper::IsElemInList(d, o);
		if (NULL == x)
			continue;

		Action* a;
		// Libvirt doesn't allow to change the emulation type in runtime
		if (x->getEmulatedType() == d->getEmulatedType() && !(*x == *d))
		{
			CVmGenericNetworkAdapter copy = PrlNet::fixMacFilter(
					*d, input_.getFinal().getVmHardwareList()->m_lstNetworkAdapters);
			a = f.craftRuntime(boost::bind(&vm::Editor::update
				<CVmGenericNetworkAdapter>, _1, copy));
		}
		// but we can attach 'routed' interface to network's bridge without libvirt
		else if (x->getEmulatedType() == PNA_ROUTED && d->getEmulatedType() == PNA_BRIDGED_NETWORK)
			a = new Reconnect(*d);
		else
			continue;

		a->setNext(output);
		output = a;
	}
	return output;
}

namespace
{
bool isHardDisksSystemPathEqual(const CVmHardDisk* lhs_, const CVmHardDisk* rhs_)
{
	return CFileHelper::IsPathsEqual(lhs_->getSystemName(), rhs_->getSystemName());
}

} // namespace

/////////////////////////////////////////////////////////////////////////////
// struct Disk

Vm::Action* Disk::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* output = NULL;
	QList<CVmHardDisk* > o = input_.getStart().getVmHardwareList()->m_lstHardDisks;
	foreach (CVmHardDisk* d, input_.getFinal().getVmHardwareList()->m_lstHardDisks)
	{
		// If disk was disconnected or disabled, its config is absent in libvirt cfg
		// All runtime parameters will be applied automatically with new device cfg
		if (isDiskIoUntunable(d))
			continue;

		QList<CVmHardDisk *>::const_iterator x =
			std::find_if(o.begin(), o.end(), boost::bind(isHardDisksSystemPathEqual, _1, d));
		if (x == o.end())
			continue;
		if (isDiskIoUntunable(*x))
			continue;

		CVmIoLimit* l(d->getIoLimit());
		if (l != NULL) {
			CVmIoLimit* p((*x)->getIoLimit());
			if (NULL == p ? l->getIoLimitValue() > 0 : l->getIoLimitValue() != p->getIoLimitValue())
			{
				Action* a(f.craftRuntime(boost::bind
					(&vm::Editor::setIoLimit, _1, d, l->getIoLimitValue())));
				a->setNext(output);
				output = a;
			}
		}

		if (d->getIopsLimit() != (*x)->getIopsLimit()) {
			Action* a(f.craftRuntime(boost::bind
				(&vm::Editor::setIopsLimit, _1, d, d->getIopsLimit())));
			a->setNext(output);
			output = a;
		}
	}
	return output;
}

bool Disk::isDiskIoUntunable(const CVmHardDisk* disk_) const
{
	bool r = false;
	r = (disk_->getEmulatedType() != PVE::HardDiskImage);
	r |= (disk_->getEnabled() == PVE::DeviceDisabled);
	r |= (disk_->getConnected() == PVE::DeviceDisconnected);
	return r;
}

///////////////////////////////////////////////////////////////////////////////
// struct Blkiotune

Vm::Action* Blkiotune::operator()(const Request& input_) const
{
	quint32 o(input_.getStart().getVmSettings()->getVmRuntimeOptions()->getIoPriority());
	quint32 n(input_.getFinal().getVmSettings()->getVmRuntimeOptions()->getIoPriority());
	if (o == n)
		return NULL;

	return Forge(input_).craftRuntime(boost::bind(&vm::Editor::setIoPriority, _1, n));
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory

Vm::Action* Memory::operator()(const Request& input_) const
{
	const CVmMemory* o = input_.getStart().getVmHardwareList()->getMemory();
	const CVmMemory* n = input_.getFinal().getVmHardwareList()->getMemory();
	if (NULL == n)
		return NULL;

	Forge f(input_);
	Action* output = NULL;
	bool ram_changed = false;
	if (n->getRamSize() != o->getRamSize())
	{
		Action* a = NULL;
		CVmConfiguration conf;
		vm::Unit u = Libvirt::Kit.vms().at(input_.getFinal()
			.getVmIdentification()->getVmUuid());
		Libvirt::Result r = u.getConfig(conf, true);
		quint64 maxMemory = u.getMaxMemory() >> 10; // in mbytes
		if (r.isFailed() || !maxMemory)
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get VM runtime configuration (%d,%llu)",
				r.error().code(), maxMemory);
			return output;
		}

		if (n->getRamSize() > maxMemory)
		{
			// We must use active configuration from libvirt
			// because only it has exact state of hotplug
			CVmMemory* lv = conf.getVmHardwareList()->getMemory();
			if (lv->isEnableHotplug())
			{
				// Create a new memory DIMM for hotplug
				const qint64 multiple = 1 << 10; // Round up DIMM size to gbyte
				const qint64 size = (((n->getRamSize() - maxMemory) +
					multiple - 1) / multiple) * multiple;
				const Transponster::Vm::Reverse::Dimm dimm(0, size << 10); // in kbytes
				WRITE_TRACE(DBG_WARNING, "Add to VM %s new DIMM of memory, size %lldm",
					QSTR2UTF8(input_.getFinal().getVmIdentification()->getVmUuid()), size);

				output = f.craftRuntime(boost::bind(
					&vm::Editor::plug<Transponster::Vm::Reverse::Dimm>,
					_1, dimm));
			}
			else
			{
				a = new Edit::Vm::Runtime::NotApplied(input_);
			}
		}

		if (!a)
		{
			a = f.craftRuntime(boost::bind(&vm::Editor::setMemory,
				_1, n->getRamSize()));
			ram_changed = true;
		}

		if (output)
			output->getTail().setNext(a);
		else
		{
			a->setNext(output);
			output = a;
		}
	}

	if (ram_changed || n->getMemGuaranteeType() != o->getMemGuaranteeType() ||
		n->getMemGuarantee() != o->getMemGuarantee())
	{
		Action* a(f.craftRuntime(boost::bind(&vm::Editor::setMemGuarantee, _1, n)));
		if (output)
			output->getTail().setNext(a);
		else
		{
			a->setNext(output);
			output = a;
		}
	}

	return output;
}

namespace Cpu
{
///////////////////////////////////////////////////////////////////////////////
// struct Factory

Vm::Action* Factory::operator()(const Request& input_) const
{
	Action* output = NULL;
	Forge f(input_);
	bool w = false;
	CVmCpu* oldCpu(input_.getStart().getVmHardwareList()->getCpu());
	CVmCpu* newCpu(input_.getFinal().getVmHardwareList()->getCpu());
	if (oldCpu->getCpuUnits() != newCpu->getCpuUnits())
		output = f.craftRuntime(boost::bind(&vm::Editor::setCpuUnits, _1, newCpu->getCpuUnits()));

	w = (oldCpu->isEnableHotplug() != newCpu->isEnableHotplug());

	if (oldCpu->getNumber() < newCpu->getNumber()) {
		if (oldCpu->isEnableHotplug() && newCpu->isEnableHotplug()) {
			Action* a(f.craftRuntime(boost::bind(&vm::Editor::setCpuCount, _1, newCpu->getNumber())));
			a->setNext(output);
			output = a;
		} else
			w = true;
	} else if (oldCpu->getNumber() > newCpu->getNumber()) {
		// unplug is not supported
		w = true;
	}
	if (w) {
		Action* a = new Edit::Vm::Runtime::NotApplied(input_);
		a->setNext(output);
		output = a;
	}

	Action* a(craftLimit(input_));
	if (a) {
		a->setNext(output);
		output = a;
	}

	if (oldCpu->getCpuMask() != newCpu->getCpuMask())
	{
		Action* a(f.craftRuntime(boost::bind(&vm::Editor::setCpuMask,
			_1, newCpu->getNumber(), newCpu->getCpuMask())));
		a->setNext(output);
		output = a;
	}

	if (oldCpu->getNodeMask() != newCpu->getNodeMask())
	{
		Action* a(f.craftRuntime(boost::bind(&vm::Editor::setNodeMask,
			_1, newCpu->getNodeMask())));
		a->setNext(output);
		output = a;
	}

	return output;
}

Vm::Action* Factory::craftLimit(const Request& input_) const
{
	CVmCpu* oldCpu(input_.getStart().getVmHardwareList()->getCpu());
	CVmCpu* newCpu(input_.getFinal().getVmHardwareList()->getCpu());

	if (oldCpu->getCpuLimitType() == newCpu->getCpuLimitType() &&
			oldCpu->getCpuLimitValue() == newCpu->getCpuLimitValue() &&
			oldCpu->getNumber() == newCpu->getNumber())
		return NULL;

	if (newCpu->getCpuLimitType() != PRL_CPULIMIT_MHZ
			&& newCpu->getCpuLimitType() != PRL_CPULIMIT_PERCENTS)
		return NULL;

	quint32 t(CDspService::instance()->getDispConfigGuard().getDispConfig()
			->getDispatcherSettings()->getCommonPreferences()
			->getWorkspacePreferences()->getVmGuestCpuLimitType());

	Forge f(input_);
	return f.craftRuntime(::Vm::Config::Edit::Cpu::Limit::Any(*newCpu, t));
}

} // namespace Cpu

namespace NetworkParams
{
///////////////////////////////////////////////////////////////////////////////
// struct Factory
Vm::Action* Factory::operator()(const Request& input_) const
{
	Action* output = nullptr;

	CVmGlobalNetwork* oldNet = input_.getStart().getVmSettings()->getGlobalNetwork();

	CVmGlobalNetwork* newNet = input_.getFinal().getVmSettings()->getGlobalNetwork();

	Forge f(input_);

	if (oldNet->getDnsIPAddresses() != newNet->getDnsIPAddresses())
	{
		QList<QString> dnsList = newNet->getDnsIPAddresses();

		output = f.craftRuntime([dnsList](Libvirt::Instrument::Agent::Vm::Editor& e)
		{
		  return e.setDns(dnsList);
		});
	}

	if (oldNet->getSearchDomains() != newNet->getSearchDomains())
	{
		QList<QString> searchList = newNet->getSearchDomains();

		// Do not ignore searchdomain value, send it through #PSBM-122697
		if (searchList.isEmpty())
			searchList.append("");

		Action* a(f.craftRuntime([searchList](Libvirt::Instrument::Agent::Vm::Editor&e)
		{
					  return e.setSearchDomains(searchList);
		}));

		a->setNext(output);
		output = a;
	}

	if (oldNet->getHostName() != newNet->getHostName())
	{
		QString hostname(newNet->getHostName());

		Action* a(f.craftRuntime([hostname](Libvirt::Instrument::Agent::Vm::Editor&e)
		{
			return e.setHostname(hostname);
		}));

		a->setNext(output);
		output = a;
	}

	return output;
}
} // namespace NetworkParams

namespace Hotplug
{
namespace Traits
{
///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
bool Generic<T>::canPlug(const device_type& novel_)
{
	return PVE::DeviceEnabled == novel_.getEnabled() &&
		novel_.getConnected() == PVE::DeviceConnected;
}

template<class T>
bool Generic<T>::canPlug(const device_type& original_, const device_type& update_)
{
	return (PVE::DeviceEnabled == update_.getEnabled() &&
		PVE::DeviceConnected == update_.getConnected() &&
		(update_.getEnabled() != original_.getEnabled() ||
		update_.getConnected() != original_.getConnected()));
}

template<class T>
typename Generic<T>::device_type* Generic<T>::match(device_type* needle_, const haystack_type& haystack_)
{
	return CXmlModelHelper::IsElemInList(needle_, haystack_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Specific<CVmHardDisk>

Specific<CVmHardDisk>::device_type*
Specific<CVmHardDisk>::match(device_type* needle_, const haystack_type& haystack_)
{
	typedef haystack_type::const_iterator iterator_type;

	iterator_type e = haystack_.end();
	iterator_type x = std::find_if(haystack_.begin(), e,
		boost::bind(isHardDisksSystemPathEqual, _1, needle_));
	if (e == x)
		return NULL;

	return *x;
}

///////////////////////////////////////////////////////////////////////////////
// struct Specific<CVmGenericNetworkAdapter>

bool Specific<CVmGenericNetworkAdapter>::canPlug(const device_type& novel_)
{
	return PVE::DeviceEnabled == novel_.getEnabled();
}

bool Specific<CVmGenericNetworkAdapter>
	::canPlug(const device_type& original_, const device_type& update_)
{
	return (PVE::DeviceEnabled == update_.getEnabled() &&
		update_.getEnabled() != original_.getEnabled());
}

bool Specific<CVmGenericNetworkAdapter>
	::canUpdate(const device_type& original_, const device_type& update_)
{
	return (PVE::DeviceEnabled == update_.getEnabled() &&
		update_.getEnabled() == original_.getEnabled() &&
		update_.getConnected() != original_.getConnected());
}

} // namespace Traits

///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
Action* Generic<T>::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* output = NULL;
	haystack_type o = traits_type::point(input_.getStart().getVmHardwareList());
	haystack_type n = traits_type::point(input_.getFinal().getVmHardwareList());
	foreach (device_type* d, n)
	{
		device_type* x = traits_type::match(d, o);
		if ((NULL == x) ? traits_type::canPlug(*d) : traits_type::canPlug(*x, *d))
		{
			Action* a = f.craftRuntime(boost::bind(
				&Libvirt::Instrument::Agent::Vm::Editor::plug<T>,
				_1, *d));
			a->setNext(output);
			output = a;
		}
	}
	foreach (device_type* d, o)
	{
		device_type* x = traits_type::match(d, n);
		if ((NULL == x) ? traits_type::canPlug(*d) : traits_type::canPlug(*x, *d))
		{
			Action* a = f.craftRuntime(boost::bind(
				&Libvirt::Instrument::Agent::Vm::Editor::unplug<T>,
				_1, *d));
			a->setNext(output);
			output = a;
		}
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Factory<CVmGenericNetworkAdapter>

Action* Factory<CVmGenericNetworkAdapter>::operator()(const Request& input_) const
{
	Forge f(input_);
	QList<Action* > c;
	Action* output = generic_type::operator()(input_);
	haystack_type o = traits_type::point(input_.getStart().getVmHardwareList());
	haystack_type n = traits_type::point(input_.getFinal().getVmHardwareList());
	foreach (device_type* d, n)
	{
		device_type* x = traits_type::match(d, o);
		if (NULL == x || !traits_type::canUpdate(*x, *d))
			continue;

		c << f.craftRuntime(boost::bind(
			&Libvirt::Instrument::Agent::Vm::Editor::update<CVmGenericNetworkAdapter>,
			_1, *d));
	}
	if (!c.isEmpty())
	{
		c.push_front(f.craft(Config::Facade<Config::Event>()));
		foreach (Action* a, c)
		{
			a->setNext(output);
			output = a;
		}
	}
	return output;
}

} // namespace Hotplug

///////////////////////////////////////////////////////////////////////////////
// struct Driver

Action* Driver::prime(const Request& input_) const
{
	VIRTUAL_MACHINE_STATE s;
	if (Libvirt::Kit.vms().at(input_.getObject().first).getState().getValue(s).isFailed())
		return NULL;
	if (VMS_RUNNING != s)
		return NULL;

	return Gear<Driver, probeList_type>::prime(input_);
}

} // namespace Runtime
} // namespace Vm
} // namespace Edit

