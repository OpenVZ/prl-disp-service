///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EditVm.cpp
///
/// Edit VM configuration
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


#include "Task_EditVm.h"
#include "Task_EditVm_p.h"
#include <boost/tuple/tuple.hpp>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include "Libraries/PrlNetworking/netconfig.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlxmlmodel/ParallelsObjects/CXmlModelHelper.h>
#include <prlxmlmodel/ParallelsObjects/CVmProfileHelper.h>
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
template<class T, class U> T ceilDiv(T lhs_, U rhs_)
{
	return (lhs_ + rhs_ - 1) / rhs_;
}

// RAM, GiB     <16       <32        <64   ...
// factor        1         2          4
// maxNuma     RAM+4     RAM+8      RAM+16
// maxTotal  maxNuma+64 maxNuma+64 maxNuma+64

unsigned getMaxNumaRamMb(unsigned ram)
{
	unsigned factor = ceilDiv(ram/1024, 16);
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
			pVmConfig = vmDirHelper.CreateDefaultVmConfigByRcValid( getClient(), error, getVmUuid() );
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
	QMutexLocker lock(CDspService::instance()->getVmDirHelper().getMultiEditDispatcher());

	LOG_MESSAGE( DBG_DEBUG, "atomicEdit() for was called for vm  %s", QSTR2UTF8( vmUuid ) );

	PRL_RESULT nRetCode = CDspService::instance()->getVmDirHelper()
		.registerExclusiveVmOperation( vmUuid, vmDirUuid, PVE::DspCmdDirVmEditCommit, pUserSession);
	if (PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to register exclusive operation on atomic VM config edit: %.8X '%s'",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return (false);
	}

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
			Task_EditVm task(pUserSession,
				IOPackage::createInstance(IOSender::UnknownType, 0));
			(Edit::Vm::driver_type(task))(pVmConfigOld, pVmConfig);
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

	// We cannot edit suspended or suspending VM
	VIRTUAL_MACHINE_STATE nState = CDspVm::getVmState(vm_uuid, pUserSession->getVmDirectoryUuid());
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

	QString sFile = ParallelsDirs::getVmConfigurationSamplePath(
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

QString Bundle::buildPath(const QPair<QString, QString> &parts) const
{
	return parts.first + QDir::separator() + name + QDir::separator() + parts.second;
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

QString Bundle::getVmName() const
{
	QString s(name);
	if (s.endsWith(VMDIR_DEFAULT_BUNDLE_SUFFIX))
		s.resize(s.size() - QString(VMDIR_DEFAULT_BUNDLE_SUFFIX).size());
	return s;
}

QString buildExtDevicePath(const QString &sDevicePath, const QString &sOldVmHomePath, const QString &sNewVmHomePath)
{
	Bundle b = Bundle::createFromPath(sOldVmHomePath);

	if (!b.isComponent(sDevicePath))
		return sDevicePath;

	Bundle n = Bundle::createFromPath(sNewVmHomePath);
	return n.buildPath(b.splitComponent(sDevicePath));
}

class ExternalDiskBundlesRenamer
{

public:
	ExternalDiskBundlesRenamer(CAuthHelper *authHelper);
	void addDisk(const QString & path);
	PRL_RESULT rename(const Bundle &oldBundle, const Bundle &newBundle, CVmEvent &error);
	void rollback();

private:

	// hold absolute paths of disks
	QStringList m_allDisks;
	// hold absolute paths of bundles
	QList<QPair<QString, QString> > m_renamedBundles;
	CAuthHelper *m_authHelper;
};

ExternalDiskBundlesRenamer::ExternalDiskBundlesRenamer(CAuthHelper *authHelper) :
	m_authHelper(authHelper)
{
}

void ExternalDiskBundlesRenamer::addDisk(const QString &path)
{
	m_allDisks.append(path);
}

PRL_RESULT ExternalDiskBundlesRenamer::rename(const Bundle &oldBundle, const Bundle &newBundle, CVmEvent &error)
{
	QSet<QString> bundleParents;

	// create list of external disks bundle parents
	foreach(const QString &diskPath, m_allDisks)
	{
		if (oldBundle.isComponent(diskPath))
			bundleParents.insert(oldBundle.getLocation(diskPath));
	}
	m_allDisks.clear();

	// for every parent check that bundle with new vm name is not occupied
	foreach(const QString &bp, bundleParents)
	{
		QString newPath = newBundle.buildPath(bp);
		if (CFileHelper::DirectoryExists(newPath, m_authHelper))
		{
			error.addEventParameter(
				new CVmEventParameter( PVE::String, oldBundle.getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
			error.addEventParameter(
				new CVmEventParameter( PVE::String, newPath, EVT_PARAM_MESSAGE_PARAM_1));
			return PRL_ERR_EXT_DISK_ALREADY_EXISTS;
		}
	}

	// rename bundles
	foreach(const QString &bp, bundleParents)
	{
		QString oldPath = oldBundle.buildPath(bp);
		QString newPath = newBundle.buildPath(bp);
		if (!CFileHelper::RenameEntry(oldPath, newPath, m_authHelper))
		{
			error.addEventParameter(
				new CVmEventParameter( PVE::String, oldPath, EVT_PARAM_MESSAGE_PARAM_0));
			error.addEventParameter(
				new CVmEventParameter( PVE::String, newPath, EVT_PARAM_MESSAGE_PARAM_1));
			return PRL_ERR_EXT_DISK_CANT_RENAME;
		}
		m_renamedBundles.append(qMakePair(oldPath, newPath));
	}

	return PRL_ERR_SUCCESS;
}

void ExternalDiskBundlesRenamer::rollback()
{
	typedef QPair<QString, QString> RenamePair;
	foreach(const RenamePair &bp, m_renamedBundles)
	{
		const QString& oldPath = bp.first;
		const QString& newPath = bp.second;

		if (!CFileHelper::RenameEntry(newPath, oldPath, m_authHelper))
		{
			WRITE_TRACE(DBG_FATAL,
				"CDspVmDirHelper::editVm() : Cannot rollback renaming [%s] to [%s]",
				QSTR2UTF8(oldPath), QSTR2UTF8(newPath) );
		}
	}
	m_renamedBundles.clear();
}

void CorrectDevicePathsInVmConfigCommon(
	CVmConfiguration *pVmConfig,
	const QString &sOldVmHomePath,
	const QString &sNewVmHomePath);

void CorrectDevicePaths(CVmDevice &vmDevice, const QString &sOldVmHomePath, const QString &sNewVmHomePath);
void CorrectHddPaths(CVmHardDisk &hardDisk, const QString &sOldVmHomePath, const QString &sNewVmHomePath);

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
	bool cloudCdRemoved = false;

	QString qsOldDirName;
	QString qsNewDirName;
	bool bVmWasRenamed = false;
	ExternalDiskBundlesRenamer diskRenamer(&getClient()->getAuthHelper());

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

	WRITE_TRACE( DBG_FATAL, "Commit vm configuartion: vm_uuid='%s'" , QSTR2UTF8( vm_uuid ) );

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

	bool flgVmWasUnregisteredInConfigWatcher = false;

	bool flgExclusiveOperationWasRegistered = false;
	bool flgExclusiveRenameOperationWasRegistered = false;
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

		if( pVmConfigNew->getVmIdentification()->getVmName()
			!= pVmConfigOld->getVmIdentification()->getVmName() )
		{
			CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(
				CDspVmDirManager::getVmHomeByUuid( getClient()->getVmIdent(vm_uuid) ));
			flgVmWasUnregisteredInConfigWatcher = true;
		}


		//////////////////////////////////////////////////////////////////////////
		//  FINALIZE PREPARE PART. BEGIN TO CHECK CONFIG
		//////////////////////////////////////////////////////////////////////////

		bool bOldTemplate = pVmConfigOld->getVmSettings()->getVmCommonOptions()->isTemplate();
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
				vmHomePath = CFileHelper::GetFileRoot(pVmDirItem->getVmHome());

				strVmHome = pVmDirItem->getVmHome();

				oldVmName = pVmConfigOld->getVmIdentification()->getVmName();
				qsOldDirName = CFileHelper::GetFileRoot( strVmHome );

				// bug #8627
				if ( pVmConfigOld->getVmHardwareList()->toString()
					!= pVmConfigNew->getVmHardwareList()->toString() )
				{
					if ( nState == VMS_PAUSED )
					{
						getLastError()->addEventParameter(
							new CVmEventParameter(
								PVE::String,
								pVmConfigOld->getVmIdentification()->getVmName(),
								EVT_PARAM_MESSAGE_PARAM_0));
						throw PRL_ERR_CANNOT_EDIT_HARDWARE_FOR_PAUSED_VM;
					}

					CVmHardware* pHardware_old = pVmConfigOld->getVmHardwareList();
					CVmHardware* pHardware_new = pVmConfigNew->getVmHardwareList();

					for (uint nType = PDE_GENERIC_DEVICE; nType < PDE_MAX; nType++ )
					{
						// bug #PSBM-5339
						if (nType == PDE_USB_DEVICE)
							continue;

						QList<void* >* devList_old = (QList<void* >* )pHardware_old->m_aDeviceLists[nType];
						QList<void* >* devList_new = (QList<void* >* )pHardware_new->m_aDeviceLists[nType];

						if ( (devList_old == NULL) != (devList_new == NULL) )
						{
							ret = DspVm::vdh().registerExclusiveVmOperation(ident.first, ident.second,
								PVE::DspCmdCtlVmEditWithHardwareChanged, getClient());
							if (PRL_FAILED(ret))
								throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
							flgExclusiveHardwareChangedWasRegistered = true;
							break;
						}
						else if  ( devList_old == NULL || devList_new == NULL )
							continue;
						else
						{
							//Prevent attempts to either change read only during runtime settings of device or disconnect/connect non SATA HDDs
							QList<CVmDevice *> *pOldLstDevs = reinterpret_cast<QList<CVmDevice *> *>( devList_old );
							QList<CVmDevice *> *pNewLstDevs = reinterpret_cast<QList<CVmDevice *> *>( devList_new );
							bool bDoParentLoopBreak = false;
							foreach( CVmDevice *pOldDevice, *pOldLstDevs )
							{
								CVmDevice *pNewDevice = CXmlModelHelper::IsElemInList( pOldDevice, *pNewLstDevs );
								if ( ! pNewDevice )//Device was removed
								{
									if (PDE_OPTICAL_DISK == nType)
									{
										if (pOldDevice->getDescription() == ::Personalize::getCdLabel()) {
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
												pNewDevice->getDescription() == ::Personalize::getCdLabel() &&
												pNewDevice->getConnected() == PVE::DeviceDisconnected) {
												cloudCdRemoved = true;
										}
										if ( PDE_HARD_DISK != nType )
										//OK to change connected state for all devices without additional checkings except hard disks
											continue;

										//Check that hard disk non IDE and SCSI
										CVmClusteredDevice *pClusteredDev = dynamic_cast<CVmClusteredDevice *>( pOldDevice );
										PRL_ASSERT(pClusteredDev);
										if ( pClusteredDev )
										{
											PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = pClusteredDev->getInterfaceType();
											if ( PMS_SATA_DEVICE == nIfaceType )//OK - SATA devices can be connected/disconnected either VM stopped or not
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
								ret = DspVm::vdh().registerExclusiveVmOperation(ident.first, ident.second,
											PVE::DspCmdCtlVmEditWithHardwareChanged, getClient());
								if (PRL_FAILED(ret))
									throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
								flgExclusiveHardwareChangedWasRegistered = true;
								//Exclusive lock was got - can break check procedure now
								bDoParentLoopBreak = true;
								break;
							}
							if ( bDoParentLoopBreak )
								break;

							//Now check new devices addition - only SATA or USB devices can be added while VM running
							foreach( CVmDevice *pNewDevice, *pNewLstDevs )
							{
								if ( CXmlModelHelper::IsElemInList( pNewDevice, *pOldLstDevs ) )//Old device - ignore checks
									continue;
								if (nType == PDE_HARD_DISK)
								{
									CVmHardDisk *d = dynamic_cast<CVmHardDisk*>(pNewDevice);
									PRL_ASSERT(d);
									if (d && d->getSerialNumber().isEmpty())
										d->setSerialNumber(Parallels::generateDiskSerialNumber());

									// VIRTIO devices can be added either on running VM or stopped
									if (PMS_VIRTIO_BLOCK_DEVICE == d->getInterfaceType())
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
									if (VMS_STOPPED != nState)
										throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
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

								ret = DspVm::vdh().registerExclusiveVmOperation(
									ident.first, ident.second,
									PVE::DspCmdCtlVmEditWithHardwareChanged, getClient());
								if (PRL_FAILED(ret))
									throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
								flgExclusiveHardwareChangedWasRegistered = true;
								//Exclusive lock was got - can break check procedure now
								bDoParentLoopBreak = true;
								break;
							}
							if ( bDoParentLoopBreak )
								break;
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
					nvram = QDir(vmHomePath).absoluteFilePath(PRL_VM_NVRAM_FILE_NAME);
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
				flgExclusiveRenameOperationWasRegistered = true;
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
					switch ( lockResult )
					{
					case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
						break;

					case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
						break;

					case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
						break;

					case PRL_ERR_VM_ALREADY_REGISTERED:
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
						break;

					case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

					default:
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
					} //switch

					throw lockResult;
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
					getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );

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

			// Synchronization SCSI devices subtype
			PRL_CLUSTERED_DEVICE_SUBTYPE pcdSubtype = CXmlModelHelper::syncScsiDevicesSubType(
				pVmConfigNew.getImpl(), pVmConfigOld.getImpl());
			if ( pcdSubtype == PCD_BUSLOGIC &&
				pVmConfigNew->getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled() )
			{
				// new configuration has a Buslogic SCSI controller, which is not
				// supposed to work together with EFI firmware, see #PSBM-21357
				WRITE_TRACE(DBG_FATAL, "The BusLogic SCSI controller cannot be used with the EFI firmware.");
				throw PRL_ERR_VMCONF_SCSI_BUSLOGIC_WITH_EFI_NOT_SUPPORTED;
			}

			//////////////////////////////////////////////////////////////////////////
			// reset additional parameters in VM configuration
			// (VM home, last change date, last modification date - never store in VM configuration itself!)
			// They were received from customer as new config
			//////////////////////////////////////////////////////////////////////////
			DspVm::vdh().resetAdvancedParamsFromVmConfig( pVmConfigNew );

			resetNetworkAddressesFromVmConfig( pVmConfigNew, pVmConfigOld );

			//Fill server UUID field because it information needing at VM check
			//access procedure
			pVmConfigNew->getVmIdentification()->setServerUuid(
				CDspService::instance()->getDispConfigGuard().getDispConfig()->
				getVmServerIdentification()->getServerUuid()
				);


			// cannot save remote device to connected state!
			// #429716 #429855 #110571

			if(nState == VMS_STOPPED)
			for ( int i = 0; i < PDE_MAX; i++ )
			{
				if ( QList<CVmDevice*>* lstDevices =
					reinterpret_cast< QList<CVmDevice*>* >
					(pVmConfigNew->getVmHardwareList()->m_aDeviceLists[i] ) )
				{
					for ( int j = 0; j < lstDevices->size(); j++ )
						if ( lstDevices->at(j)->isRemote()
									&&
							( lstDevices->at(j)->getConnected() == PVE::DeviceConnected )
							)
						{
							getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );
							getLastError()->addEventParameter(
								new CVmEventParameter( PVE::String,
								lstDevices->at(j)->getUserFriendlyName(),
								EVT_PARAM_MESSAGE_PARAM_0 )
								);

							WRITE_TRACE(DBG_FATAL,
								"connected remote device %s was found",
								QSTR2UTF8(lstDevices->at(j)->getUserFriendlyName()));

							throw PRL_ERR_CANNOT_SAVE_REMOTE_DEVICE_STATE;
						}
				}
			}

			CVmRemoteDisplay* oldRemDisplay = pVmConfigOld->getVmSettings()->getVmRemoteDisplay();
			CVmRemoteDisplay* newRemDisplay = pVmConfigNew->getVmSettings()->getVmRemoteDisplay();

			if (oldRemDisplay->getPassword() != newRemDisplay->getPassword())
			{
				if (newRemDisplay->getPassword().length() > PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN)
				{
					WRITE_TRACE(DBG_FATAL, "The specified remote display password is too long.");
					getLastError()->addEventParameter(
						new CVmEventParameter(
							PVE::UnsignedInt,
							QString::number(PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN),
							EVT_PARAM_MESSAGE_PARAM_0));

					throw PRL_ERR_VMCONF_REMOTE_DISPLAY_PASSWORD_TOO_LONG;
				}
			}

			// Check VNC config difference for running VM
			do {
				/* We can't stop VNC server here, because main thread
				 * may issue cleanupAllBeginEditMarksByAccessToken, which
				 * will try to occure MultiEditDispatcher lock, but it is
				 * locked until we exit from section with editLock held.
				 * But function, which terminate VNC process wait for
				 * signal from main thread, it wait for timeout and
				 * exit with error. */
				// Start VNC
				if ( oldRemDisplay->getMode() == PRD_DISABLED &&
					 newRemDisplay->getMode() != PRD_DISABLED ) {
					bNeedVNCStart = true;
				}
				// Stop VNC
				else if ( oldRemDisplay->getMode() != PRD_DISABLED &&
						  newRemDisplay->getMode() == PRD_DISABLED ) {
					bNeedVNCStop = true;
				}
				// VNC config has been changed
				else if ( newRemDisplay->getMode() != PRD_DISABLED &&
						  oldRemDisplay->getMode() != PRD_DISABLED &&
						  (oldRemDisplay->getHostName() !=
						   newRemDisplay->getHostName() ||
						   (oldRemDisplay->getPortNumber() !=
						    newRemDisplay->getPortNumber() &&
						    oldRemDisplay->getMode() == PRD_MANUAL) ||
						   oldRemDisplay->getPassword() !=
						   newRemDisplay->getPassword()) ) {
					bNeedVNCStart = true;
					bNeedVNCStop = true;
				}


			} while (0);

			quint32 t(CDspService::instance()->getDispConfigGuard().getDispConfig()
					->getDispatcherSettings()->getCommonPreferences()
					->getWorkspacePreferences()->getVmGuestCpuLimitType());

			pVmConfigNew->getVmHardwareList()->getCpu()->setGuestLimitType(t);

			if (nState != VMS_STOPPED && (bNeedVNCStop || bNeedVNCStart))
			{
				WRITE_TRACE(DBG_FATAL, "Unable to edit VNC preferences for running VM %s.",
					qPrintable(vm_uuid));
				throw PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
			}

			if (nState != VMS_STOPPED && bVmWasRenamed)
			{
				WRITE_TRACE(DBG_FATAL, "Unable to change name for running VM %s.",
					qPrintable(vm_uuid));
				throw PRL_ERR_VM_MUST_BE_STOPPED_BEFORE_RENAMING;
			}

			//Do not let change VM uptime through VM edit
			//https://bugzilla.sw.ru/show_bug.cgi?id=464218
			//XXX: seems we have here potential race with internal atomic changes
			pVmConfigNew->getVmIdentification()->setVmUptimeStartDateTime(
				pVmConfigOld->getVmIdentification()->getVmUptimeStartDateTime()
			);
			pVmConfigNew->getVmIdentification()->setVmUptimeInSeconds(
				pVmConfigOld->getVmIdentification()->getVmUptimeInSeconds()
			);

			// High Availability Cluster
			//
			// handle VM only on shared FS - nfs, gfs, gfs2, pcs
			if (CDspService::isServerModePSBM() && CFileHelper::isSharedFS(strVmHome) )
			{
				ret = UpdateClusterResourceVm(pVmConfigOld, pVmConfigNew, vmHomePath, bVmWasRenamed);
				if (PRL_FAILED(ret))
					throw ret;
			}

			// save VM configuration to file
			//////////////////////////////////////////////////////////////////////////

			PRL_RESULT save_rc =
				CDspService::instance()->getVmConfigManager().saveConfig(pVmConfigNew,
				strVmHome,
				getClient(),
				true,
				true);

			if (!IS_OPERATION_SUCCEEDED(save_rc))
			{
				WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher unable to save configuration "
					"of the VM %s to file %s. Reason: %ld(%s)",
					QSTR2UTF8( vm_uuid ),
					QSTR2UTF8( strVmHome ),
					Prl::GetLastError(),
					QSTR2UTF8( Prl::GetLastErrorAsString() )
					);

				// check error code - it may be not free space for save config
				if ( save_rc == PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_XML_SAVE )
					throw save_rc;

				getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, newVmName,
					EVT_PARAM_MESSAGE_PARAM_0 ) );
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, QFileInfo( strVmHome ).path(),
					EVT_PARAM_MESSAGE_PARAM_1 ) );

				throw PRL_ERR_SAVE_VM_CONFIG;
			}

#ifdef _LIBVIRT_
			pVmConfigNew->getVmIdentification()->setHomePath(strVmHome);
			Edit::Vm::driver_type(*this)(pVmConfigOld, pVmConfigNew);
			if(!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
				throw getLastErrorCode();
#endif // _LIBVIRT_

			CXmlModelHelper::printConfigDiff( *pVmConfigNew, *pVmConfigOld, DBG_WARNING, "VmCfgCommitDiff" );

			// clear flags suspend and change for HDD images
			// clear suspend flag for disks
			DspVm::vdh().getMultiEditDispatcher()->lock();
			CStatesHelper::SetSuspendParameterForAllDisks(pVmConfigNew.getImpl(),0);
			DspVm::vdh().getMultiEditDispatcher()->unlock();
			DspVm::vdh().getMultiEditDispatcher()->registerCommit(vm_uuid, getClient()->getClientHandle());
		}

		do {
			SmartPtr< CDspVm > pVm = CDspVm::GetVmInstanceByUuid(ident);
			if ( ! pVm )
				break;

			// XXX
			// we create this fake package which will be sent
			// to every client only for dispatcher stability purposes!
			// Dispatcher does not check null pointers, does not check
			// validness of data, does not check anything and can be
			// crashed at any time (ask sergeyt@ about this default
			// dispatcher behaviour), to omit this behaviour we should
			// make this ugly cheat.
			SmartPtr<IOPackage> fakePkg =
					IOPackage::createInstance( IOSender::UnknownType, 0 );
			if (bNeedVNCStop)
				pVm->stopVNCServer(getClient(), fakePkg, false, false);
			if (bNeedVNCStart)
				pVm->startVNCServer(getClient(), fakePkg, false, true);
		} while (0);

		// Edit firewall
		ret = editFirewall(pVmConfigNew, pVmConfigOld, nState, flgExclusiveFirewallChangedWasRegistered);
		if (PRL_FAILED(ret))
			throw ret;

		updateNetworkSettings(pVmConfigNew, pVmConfigOld);

		//////////////////////////////////////////////////////////////////////////
		// Notify users that VM configuration was changed
		//////////////////////////////////////////////////////////////////////////

		// Generate "VM changed" event
		DspVm::vdh().sendVmConfigChangedEvent(ident, getRequestPackage());

		//////////////////////////////////////////////////////////////////////////
		// configure Virtuozzo specific parameters
		/////////////////////////////////////////////////////////////////////////
		configureVzParameters(pVmConfigNew, pVmConfigOld);
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
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
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String,
				pVmConfigOld->getVmIdentification()->getVmName(),
				EVT_PARAM_MESSAGE_PARAM_0)
				);
		break;
		default:;
		}


		// NOTE:  DO NOT unregister begin edit mark BECAUSE client doesn't send BeginEdit again if commit fails
		// CDspService::instance()->getVmDirHelper()
		//	.getMultiEditDispatcher()->cleanupBeginEditMark( vm_uuid, getClient()->getClientHandle() );
	}

	// NOTE: register to vm watcher should be called before unregistration for any exclusive operations
	//  It should be done to prevent possible races with removing vm or migrationg
	if (flgVmWasUnregisteredInConfigWatcher)
	{
		CDspService::instance()->getVmConfigWatcher().registerVmToWatch(
				CDspVmDirManager::getVmHomeByUuid(ident), ident);
	}

	if (flgExclusiveHardwareChangedWasRegistered)
	{
		DspVm::vdh().unregisterExclusiveVmOperation(ident.first, ident.second,
				PVE::DspCmdCtlVmEditWithHardwareChanged, getClient());
	}
	if (flgExclusiveRenameOperationWasRegistered)
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
		applyVmConfig( getClient(), pVmConfigNew, pVmConfigOld, getRequestPackage() );
		ret = getLastErrorCode();
		if (!cloudCdRemoved && CDspVm::getVmState(ident) == VMS_STOPPED) {
			CDspService::instance()->getVmStateSender()->onVmPersonalityChanged(
				ident.second, ident.first);
		}
	}

	return ret;
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

	if (!CDspService::isServerMode())
		return;

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
void Task_EditVm::applyVmConfig(SmartPtr<CDspClient> pUserSession,
								SmartPtr<CVmConfiguration> pVmConfigNew,
								SmartPtr<CVmConfiguration> pVmConfigOld,
								const SmartPtr<IOPackage>& pkg)
{
	Q_UNUSED(pkg);
	CDspService::instance()->getVmDirHelper()
		.appendAdvancedParamsToVmConfig( pUserSession, pVmConfigNew );
	Edit::Vm::Runtime::Driver(*this)(pVmConfigOld, pVmConfigNew);
}

PRL_RESULT Task_EditVm::configureVzParameters(SmartPtr<CVmConfiguration> pNewVmConfig,
					SmartPtr<CVmConfiguration> pOldVmConfig)
{
	bool bSet = !pOldVmConfig;

	if (!CDspService::isServerModePSBM())
		return PRL_ERR_SUCCESS;

	QString sVmUuid = pNewVmConfig->getVmIdentification()->getVmUuid();
	// Configure for running VM only
	VIRTUAL_MACHINE_STATE s;
	Libvirt::Result r(Libvirt::Kit.vms().at(sVmUuid).getState(s));
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
					/* ignore this error by review @2313 */
					/* http://review.parallels.com:8080/index.jsp?page=ReviewDisplay&reviewid=2313 */
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

	void CorrectHddPaths(CVmHardDisk &hardDisk, const QString &sOldVmHomePath, const QString &sNewVmHomePath)
	{
		if (hardDisk.getEmulatedType() == PVE::HardDiskImage)
		{
			QString newPath;
			if (hardDisk.getUserFriendlyName().startsWith(sOldVmHomePath))
				newPath = BuildDevicePath(hardDisk.getUserFriendlyName(), sOldVmHomePath, sNewVmHomePath);
			else
				newPath = buildExtDevicePath(hardDisk.getUserFriendlyName(), sOldVmHomePath, sNewVmHomePath);
			hardDisk.setUserFriendlyName(newPath);
			hardDisk.setSystemName(newPath);
		}
		else
		{
			CorrectDevicePaths(hardDisk, sOldVmHomePath, sNewVmHomePath);
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
	if ( ! CDspService::isServerModePSBM() )
		return PRL_ERR_SUCCESS;

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

	CFirewallHelper fw(pVmConfigNew);

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

	// need network's bridge name CParallelsAdapter::getName()
	CParallelsAdapter* a;
	if (NULL == n.getHostOnlyNetwork()
		|| (a = n.getHostOnlyNetwork()->getParallelsAdapter()) == NULL)
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
// struct VcmmdAction

bool VcmmdAction::execute(CDspTaskFailure& feedback_)
{
	PRL_RESULT e = m_vcmmd.update(m_limit, m_guarantee);
	if (PRL_FAILED(e))
	{
		feedback_(e);
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

	res = DspVm::vdh().insertVmDirectoryItem(m_target, t.data());
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "Unable to add directory item (%s)", PRL_RESULT_TO_STRING(res));
		return Action::execute(feedback_);
	}

	t.take();

	return Action::execute(feedback_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Apply

Action* Apply::operator()(const Request& input_) const
{
	Forge f(input_);
	bool a = input_.getStart().getVmSettings()->getVmCommonOptions()->isTemplate();
	bool b = input_.getFinal().getVmSettings()->getVmCommonOptions()->isTemplate();
	if (a != b)
	{
		QString t;
		Action* n = NULL;
		if (b)
		{
			n = f.craft(boost::bind(&vm::Unit::undefine, _1));
			t = DspVm::vdm().getTemplatesDirectoryUuid();
		}
		else
		{
			n = f.craft(boost::bind(&define, _1, boost::cref
				(input_.getFinal())));
			t = input_.getTask().getClient()->getVmDirectoryUuid();
		}
		Action *output(new Transfer(input_.getObject(), t));
		output->setNext(n);
		return output;
	}
	if (b)
		return NULL;

	QString x = input_.getStart().getVmIdentification()->getVmName();
	QString y = input_.getFinal().getVmIdentification()->getVmName();
	if (x == y)
	{
		return f.craft(boost::bind(&vm::Unit::setConfig, _1,
				boost::cref(input_.getFinal())));
	}
	Action* output = f.craft(boost::bind(&vm::Unit::rename, _1, y));
	output->setNext(f.craft(boost::bind(&vm::Unit::setConfig, _1,
				boost::cref(input_.getFinal()))));

	return output;
}

Libvirt::Result Apply::define(vm::Unit agent_, const CVmConfiguration& config_)
{
	return agent_.up().define(config_);
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
	QString file = m_data.getNVRAM();

	if (QFileInfo(file).isRelative())
		file = QDir(m_path).absoluteFilePath(file);

	if (QFile::exists(file))
		return PRL_ERR_SUCCESS;

	if (0 != QProcess::execute("qemu-img", QStringList() << "convert"
		<< "-O" << "qcow2" << "/usr/share/OVMF/OVMF_VARS.fd" << file))
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

	feedback_(r);
	return false;
}

} // namespace Create

namespace Update
{

///////////////////////////////////////////////////////////////////////////////
// struct Directory

Vm::Action* Directory::operator()(const Request& input_) const
{
	return new Action(input_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Action

Action::Action(const Request& input_): m_vmIdent(input_.getObject()),
	m_vmName(input_.getFinal().getVmIdentification()->getVmName()),
	m_isTemplate(input_.getFinal().getVmSettings()->getVmCommonOptions()->isTemplate())
{
	CDspLockedPointer<CDispUser> u = CDspService::instance()->getDispConfigGuard()
		.getDispUserByUuid(input_.getTask().getClient()->getUserSettingsUuid());

	if(u.isValid())
		m_userName = u->getUserName();

	if (m_vmName != input_.getStart().getVmIdentification()->getVmName())
		m_vmHome = input_.getFinal().getVmIdentification()->getHomePath();
}

bool Action::execute(CDspTaskFailure& feedback_)
{
	CDspVmDirManager& dirManager = DspVm::vdm();
	CDspLockedPointer<CVmDirectoryItem> dirItem = dirManager
		.getVmDirItemByUuid(m_vmIdent.second, m_vmIdent.first);

	if (!dirItem)
	{
		feedback_(PRL_ERR_VM_DIRECTORY_NOT_EXIST);
		return false;
	}

	dirItem->setTemplate(m_isTemplate);

	// #441667 - set the same parameters for shared vm
	CDspVmDirManager::VmDirItemsHash sharedVmHash = dirManager
		.findVmDirItemsInCatalogue(dirItem->getVmUuid(), dirItem->getVmHome());

	foreach(CDspLockedPointer<CVmDirectoryItem> dirSharedItem, sharedVmHash)
	{
		dirSharedItem->setChangedBy(m_userName);
		dirSharedItem->setChangeDateTime(QDateTime::currentDateTime());
		dirSharedItem->setVmName(m_vmName);
		if (m_vmHome)
			dirSharedItem->setVmHome(m_vmHome.get());

		/* old code
		pVmDirSharedItem->getLockedOperationsList()->setLockedOperations(lstNewLockedOperations);
		pVmDirSharedItem->getLockDown()->setEditingPasswordHash(newLockDownHash);
		*/
	}

	PRL_RESULT ret = dirManager.updateVmDirItem(dirItem);
	if (PRL_FAILED(ret))
	{
		WRITE_TRACE(DBG_FATAL, "Can't update VmCatalogue by error %#x, %s", ret, PRL_RESULT_TO_STRING(ret));
		feedback_(ret);
		return false;
	}
	return Vm::Action::execute(feedback_);
}

} // namespace Update

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
		else if (x->getEmulatedType() == PNA_ROUTED && d->getEmulatedType() == PNA_BRIDGED_ETHERNET)
			a = new Reconnect(*d);
		else
			continue;

		a->setNext(output);
		output = a;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory

Action* Memory::operator()(const Request& input_) const
{
	CVmMemory* o = input_.getStart().getVmHardwareList()->getMemory();
	CVmMemory* n = input_.getFinal().getVmHardwareList()->getMemory();
	if (NULL == n)
		return NULL;

	Vcmmd::Api a(input_.getObject().first);
	// Although vcmmd.update would succeed if we provide the same values
	// we still want to read configuration first, to get vcmmd status
	Prl::Expected<std::pair<quint64, quint64>, PRL_RESULT> x =
		a.getConfig();
	if (x.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Can't get current vcmmd configuration. Skipping runtime update.");

		// Report an error only if we detect that RAM size change is requested.
		// Otherwise we would swear on vcmmd status if something unrelated to memory is changed
		if (o == NULL || n->getRamSize() == o->getRamSize())
			return NULL;
		else
			return new Flop(x.error());
	}
	quint64 l = n->getRamSize();
	quint64 g = ::Vm::Config::MemGuarantee(*n)(l) << 20;
	l = l << 20;
	// No use in updating configuration if it doesn't differ from current
	if (l == x.value().first && x.value().second == g)
		return NULL;

	return new VcmmdAction(a, l, g);
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
			if (p == NULL || l->getIoLimitValue() != p->getIoLimitValue())
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

namespace Network
{

// struct Factory

Vm::Action* Factory::operator()(const Request& input_) const
{
	::Network::Difference::Vm v(input_.getFinal());
	unsigned int t = input_.getFinal().getVmSettings()->getVmCommonOptions()->getOsType();
	QStringList d = v.calculate(input_.getStart(), t);
	if (d.isEmpty())
		return NULL;

	QString c = d.takeFirst();
	Libvirt::Instrument::Agent::Vm::Exec::Request request(c, d);
	request.setRunInShell(t == PVS_GUEST_TYPE_WINDOWS);
	QString uuid = input_.getFinal().getVmIdentification()->getVmUuid();
	return Forge(input_).craftGuest(boost::bind(&::Vm::Guest::runProgram, _1, uuid, request));
}

} // namespace Network

namespace Cpu
{

namespace Limit
{
///////////////////////////////////////////////////////////////////////////////
// struct Percents

Libvirt::Result Percents::operator()(const vm::Editor& agent_) const
{
	Prl::Expected<VtInfo, Error::Simple> v = Libvirt::Kit.host().getVt();
	if (v.isFailed())
		return v.error();

	quint32 p(v.value().getQemuKvm()->getVCpuInfo()->getDefaultPeriod());
	return m_setter(agent_, m_value * p / 100, p);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mhz

Libvirt::Result Mhz::operator()(const vm::Editor& agent_) const
{
	Prl::Expected<VtInfo, Error::Simple> v = Libvirt::Kit.host().getVt();
	if (v.isFailed())
		return v.error();

	/* get cpu limit*/
	quint32 p(v.value().getQemuKvm()->getVCpuInfo()->getDefaultPeriod());
	quint32 l = ceilDiv(static_cast<quint64>(m_value) * p,
			v.value().getQemuKvm()->getVCpuInfo()->getMhz());
	return m_setter(agent_, l, p);
}

///////////////////////////////////////////////////////////////////////////////
// struct Any

Libvirt::Result Any::operator()(const vm::Editor& agent_) const
{
	quint32 n(m_cpu.getCpuLimitValue());
	Limit::setter_type s(boost::bind(&vm::Editor::setGlobalCpuLimit, _1, _2, _3));
	if (PRL_VM_CPULIMIT_GUEST == m_type) {
		n = ceilDiv(n, m_cpu.getNumber());
		s = boost::bind(&vm::Editor::setPerCpuLimit, _1, _2, _3);
	}

	if (m_cpu.getCpuLimitType() == PRL_CPULIMIT_MHZ)
		return Limit::Mhz(n, s)(agent_);
	else if (m_cpu.getCpuLimitType() == PRL_CPULIMIT_PERCENTS)
		return Limit::Percents(n, s)(agent_);

	return Error::Simple(PRL_ERR_FAILURE, "Unknown type of CPU limit");
}

} // namespace Limit

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
	return f.craftRuntime(Limit::Any(*newCpu, t));
}

} // namespace Cpu

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

template<>
QList<CVmHardDisk* > Hotplug<CVmHardDisk>::getList(const CVmHardware* hardware_)
{
	return hardware_->m_lstHardDisks;
}

template<>
QList<CVmSerialPort* > Hotplug<CVmSerialPort>::getList(const CVmHardware* hardware_)
{
	return hardware_->m_lstSerialPorts;
}

template<class T>
Action* Hotplug<T>::operator()(const Request& input_) const
{
	Forge f(input_);
	Action* output = NULL;
	QList<T* > o = getList(input_.getStart().getVmHardwareList());
	QList<T* > n = getList(input_.getFinal().getVmHardwareList());
	foreach (T* d, getDifference(n, o))
	{
		Action* a = f.craftRuntime(boost::bind(
			&Libvirt::Instrument::Agent::Vm::Editor::plug<T>,
			_1, *d));
		a->setNext(output);
		output = a;
	}
	foreach (T* d, getDifference(o, n))
	{
		Action* a = f.craftRuntime(boost::bind(
			&Libvirt::Instrument::Agent::Vm::Editor::unplug<T>,
			_1, *d));
		a->setNext(output);
		output = a;
	}
	return output;
}

template<class T>
QList<T* > Hotplug<T>::getDifference(const QList<T* >& first_,
				const QList<T* >& second_)
{
	QList<T* > output;
	foreach (T* d, first_)
	{
		T* x = CXmlModelHelper::IsElemInList(d, second_);
		if (NULL == x)
		{
			if (PVE::DeviceEnabled == d->getEnabled() &&
				PVE::DeviceConnected == d->getConnected())
				output << d;
		}
		else if (PVE::DeviceEnabled == d->getEnabled() &&
			PVE::DeviceConnected == d->getConnected() &&
			(d->getEnabled() != x->getEnabled() ||
			d->getConnected() != x->getConnected()))
		{
			output << d;
		}
	}
	return output;
}

template<>
QList<CVmHardDisk* > Hotplug<CVmHardDisk>::getDifference(const QList<CVmHardDisk* >& first_,
				const QList<CVmHardDisk* >& second_)
{
	QList<CVmHardDisk* > output;
	foreach (CVmHardDisk* d, first_)
	{
		QList<CVmHardDisk *>::const_iterator x =
			std::find_if(second_.begin(), second_.end(), boost::bind(isHardDisksSystemPathEqual, _1, d));
		if (x == second_.end())
		{
			if (PVE::DeviceEnabled == d->getEnabled() &&
				PVE::DeviceConnected == d->getConnected())
				output << d;
		}
		else if (PVE::DeviceEnabled == d->getEnabled() &&
			PVE::DeviceConnected == d->getConnected() &&
			(d->getEnabled() != (*x)->getEnabled() ||
			d->getConnected() != (*x)->getConnected()))
		{
			output << d;
		}
	}
	return output;
}
///////////////////////////////////////////////////////////////////////////////
// struct Driver

Action* Driver::prime(const Request& input_) const
{
	VIRTUAL_MACHINE_STATE s;
	if (Libvirt::Kit.vms().at(input_.getObject().first).getState(s).isFailed())
		return NULL;
	if (VMS_RUNNING != s)
		return NULL;

	return Gear<Driver, probeList_type>::prime(input_);
}

} // namespace Runtime
} // namespace Vm
} // namespace Edit

