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
///  CVmValidateConfig.cpp
///
/// @brief
///  Implementation of the class CVmValidateConfig
///
/// @brief
///  This class implements section validation VM configuration
///
/// @author sergeyt
///  myakhin
///
/// @date
///  2008-03-11
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include "CVmValidateConfig.h"
#include "CVmValidateConfig_p.h"
#include "CDspService.h"
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/OsInfo.h>
#include <prlcommon/PrlCommonUtilsBase/CHardDiskHelper.h>
#include <prlcommon/VirtualDisk/Qcow2Disk.h>
#include "CDspClient.h"
#include "CDspVzHelper.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include "Tasks/Task_ManagePrlNetService.h"
#include "StringUtils.h"
#include <prlxmlmodel/ParallelsObjects/CXmlModelHelper.h>
#include "Interfaces/Config.h"
#include <prlcommon/Interfaces/ApiDevNums.h>
#include <boost/mpl/quote.hpp>
#include <boost/mpl/for_each.hpp>
#define SCSI_HOST_INDEX 7

static const QStringList g_UrlSchemeList = QStringList()
	<< "ftp://" << "http://" << "https://" << "smb://" << "nfs://";

namespace Validation
{
typedef boost::mpl::vector<VmNameEmpty, VmNameInvalidSymbols, VmNameLength> problem_types;

////////////////////////////////////////////////////////////////////////////////
// struct Traits

template <>
boost::optional<VmNameEmpty> Traits<VmNameEmpty>::check(const CVmConfiguration& vm_)
{
	if (!vm_.getVmIdentification()->getVmName().isEmpty())
		return boost::none;
	
	return VmNameEmpty();
}

template <>
boost::optional<VmNameInvalidSymbols> Traits<VmNameInvalidSymbols>::check(const CVmConfiguration& vm_)
{
	QString name = vm_.getVmIdentification()->getVmName();
	if (!name.contains(QRegExp("[\\/:*?\"<>|%]")))
		return boost::none;

	return VmNameInvalidSymbols(name);
}

template <>
boost::optional<VmNameLength> Traits<VmNameLength>::check(const CVmConfiguration& vm_)
{
	const int maxLength = 40;
	if (vm_.getVmIdentification()->getVmName().length() <= maxLength)
		return boost::none;

	return VmNameLength(maxLength);
}

template <>
QSet<QString> Traits<VmNameEmpty>::getIds(const CVmConfiguration& vm_)
{
	return QSet<QString>() << vm_.getVmIdentification()->getVmName_id();
}

template <>
QSet<QString> Traits<VmNameInvalidSymbols>::getIds(const CVmConfiguration& vm_)
{
	return QSet<QString>() << vm_.getVmIdentification()->getVmName_id();
}

template <>
QSet<QString> Traits<VmNameLength>::getIds(const CVmConfiguration& vm_)
{
	return QSet<QString>() << vm_.getVmIdentification()->getVmName_id();
}

template<>
PRL_RESULT Traits<VmNameEmpty>::getError()
{
	return PRL_ERR_VMCONF_VM_NAME_IS_EMPTY;
}

template<>
PRL_RESULT Traits<VmNameInvalidSymbols>::getError()
{
	return PRL_ERR_VMCONF_VM_NAME_HAS_INVALID_SYMBOL;
}
template<>
PRL_RESULT Traits<VmNameLength>::getError()
{
	return PRL_ERR_VMCONF_VM_NAME_IS_TOO_LONG;
}

////////////////////////////////////////////////////////////////////////////////
// struct Sink

template<class T>
void Sink::operator()(const T&)
{
	typename T::check_type r = T::check(*m_config);
	if (!r)
		return;

	m_results->append(T::getError());
	m_idsMap->insert(m_results->size(), T::getIds(*m_config));
	m_paramsMap->insert(m_results->size(), r->getParams());
}

} // namespace Validation

#ifdef E_SET
#error Error: E_SET macro using conflict !
#endif
#define E_SET			QSet<QString >()
#ifdef ADD_FID
#error Error: ADD_FID macro using conflict !
#endif
#define ADD_FID(set)	m_mapFullItemIds.insert(m_lstResults.size(), set)

CVmValidateConfig::CVmValidateConfig(SmartPtr<CDspClient> pUser,
			const SmartPtr<CVmConfiguration>& pVmConfig,
			const SmartPtr<CVmConfiguration>& pVmConfigOld)
: m_pClient(pUser),
  m_pVmConfig(pVmConfig),
  m_pVmConfigOld(pVmConfigOld),
  m_bCheckOnlyChanges(false)
{
}

QList<PRL_RESULT > CVmValidateConfig::CheckVmConfig(PRL_VM_CONFIG_SECTIONS nSection)
{
	if ( nSection == PVC_VALIDATE_CHANGES_ONLY )
	{
		if ( ! getOldVmConfig() )
			return m_lstResults;

		m_bCheckOnlyChanges = true;
		nSection = PVC_ALL;
	}

	CommonDevicesCheck( nSection );

	if (nSection == PVC_ALL || nSection == PVC_GENERAL_PARAMETERS)
	{
		CheckGeneralParameters();
	}

	if (nSection == PVC_ALL || nSection == PVC_BOOT_OPTION)
	{
		CheckBootOption();
	}

	if (nSection == PVC_ALL || nSection == PVC_REMOTE_DISPLAY)
	{
		CheckRemoteDisplay();
	}

	if (nSection == PVC_ALL || nSection == PVC_SHARED_FOLDERS)
	{
		CheckSharedFolders();
	}

	if (nSection == PVC_ALL || nSection == PVC_CPU)
	{
		CheckCpu();
	}

	if (nSection == PVC_ALL || nSection == PVC_MAIN_MEMORY)
	{
		CheckMainMemory();
	}

	if (nSection == PVC_ALL || nSection == PVC_VIDEO_MEMORY)
	{
		CheckVideoMemory();
	}

	if (nSection == PVC_ALL || nSection == PVC_FLOPPY_DISK)
	{
		CheckFloppyDisk();
	}

	if (nSection == PVC_ALL || nSection == PVC_CD_DVD_ROM)
	{
		CheckCdDvdRom();
	}

	if (nSection == PVC_ALL || nSection == PVC_HARD_DISK)
	{
		CheckHardDisk();
	}

	if (nSection == PVC_ALL || nSection == PVC_NETWORK_ADAPTER)
	{
		CheckNetworkAdapter();
	}

	if (nSection == PVC_ALL || nSection == PVC_SOUND)
	{
		CheckSound();
	}

	if (nSection == PVC_ALL || nSection == PVC_SERIAL_PORT)
	{
		CheckSerialPort();
	}

	if (nSection == PVC_ALL || nSection == PVC_PARALLEL_PORT)
	{
		CheckParallelPort();
	}

	if (nSection == PVC_ALL || nSection == PVC_IDE_DEVICES
		|| nSection == PVC_CD_DVD_ROM
		|| nSection == PVC_HARD_DISK)
	{
		CheckIdeDevices();
	}

	if (nSection == PVC_ALL || nSection == PVC_SCSI_DEVICES
		|| nSection == PVC_CD_DVD_ROM
		|| nSection == PVC_HARD_DISK)
	{
		CheckScsiDevices();
	}

	if (nSection == PVC_ALL || nSection == PVC_SATA_DEVICES
		|| nSection == PVC_CD_DVD_ROM
		|| nSection == PVC_HARD_DISK)
	{
		CheckSataDevices();
	}

	if (nSection == PVC_ALL
		|| nSection == PVC_CD_DVD_ROM
		|| nSection == PVC_HARD_DISK)
	{
		CheckVirtioBlockDevices();
	}

	if (nSection == PVC_ALL || nSection == PVC_GENERIC_PCI)
	{
		CheckGenericPci();
	}

	if (nSection == PVC_ALL || nSection == PVC_NETWORK_ADAPTER)
	{
		CheckNetworkShapingRates();
	}

	// Add logging.
	for( int idx = 0; idx < m_lstResults.size(); idx ++ )
	{
		if( 0 == idx )
			WRITE_TRACE(DBG_FATAL, "check config for section %d return %d errors:", nSection, m_lstResults.size() );

		WRITE_TRACE(DBG_FATAL, "\t %#x '%s'"
			, m_lstResults.at( idx )
			, PRL_RESULT_TO_STRING( m_lstResults.at( idx ) )
			);
	}

	leaveErrorsOnlyForChanges();

	return m_lstResults;
}

QList<PRL_RESULT > CVmValidateConfig::CheckCtConfig(PRL_VM_CONFIG_SECTIONS nSection)
{
	if ( nSection == PVC_VALIDATE_CHANGES_ONLY )
	{
		if ( ! getOldVmConfig() )
			return m_lstResults;

		m_bCheckOnlyChanges = true;
		nSection = PVC_ALL;
	}

	if (nSection == PVC_ALL || nSection == PVC_NETWORK_ADAPTER)
	{
		QList<CVmGenericNetworkAdapter* > lstNetworkAdapters = m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters;
		if (!lstNetworkAdapters.empty())
		{
			QSet<QString > setNA_ids;
			foreach(CVmGenericNetworkAdapter* na, lstNetworkAdapters)
			{
				if (na)
					setNA_ids << na->getFullItemId() << na->getNetAddresses_id();
			}

			CheckIPDuplicates(setNA_ids);
		}
	}

	// Add logging.
	for( int idx = 0; idx < m_lstResults.size(); idx ++ )
	{
		if( 0 == idx )
			WRITE_TRACE(DBG_FATAL, "check config for section %d return %d errors:", nSection, m_lstResults.size() );

		WRITE_TRACE(DBG_FATAL, "\t %#x '%s'"
			, m_lstResults.at( idx )
			, PRL_RESULT_TO_STRING( m_lstResults.at( idx ) )
			);
	}

	leaveErrorsOnlyForChanges();

	return m_lstResults;
}

void CVmValidateConfig::leaveErrorsOnlyForChanges()
{
	if ( ! m_bCheckOnlyChanges )
		return;

	PRL_ASSERT(m_pVmConfigOld.isValid());

	QStringList lstDiffFullItemIds;
	m_pVmConfig->diffDocuments<CVmConfiguration>(m_pVmConfigOld.getImpl(), lstDiffFullItemIds);

	QSet<QString> setDiffFullItemIds = lstDiffFullItemIds.toSet();

	for(int i = 0; i < m_lstResults.size(); ++i)
	{
		if ( m_mapFullItemIds[i + 1].intersect(setDiffFullItemIds).isEmpty() )
			m_lstResults[i] = PRL_ERR_SUCCESS;
	}
}

bool CVmValidateConfig::HasCriticalErrors(CVmEvent& evtResult,
										  PRL_UINT32 validateInternalFlags)
{
	CheckVmConfig(PVC_ALL);

	for(int i = 0; i < m_lstResults.size(); ++i)
	{
		switch(m_lstResults[i])
		{
		case PRL_ERR_VMCONF_CPU_ZERO_COUNT:
		case PRL_ERR_VMCONF_CPU_COUNT_MORE_MAX_CPU_COUNT:
		case PRL_ERR_VMCONF_CPU_COUNT_MORE_HOST_CPU_COUNT:
		case PRL_ERR_VMCONF_MAIN_MEMORY_ZERO_SIZE:
		case PRL_ERR_VMCONF_MAIN_MEMORY_OUT_OF_RANGE:
		case PRL_ERR_VMCONF_MAIN_MEMORY_NOT_4_RATIO_SIZE:
		case PRL_ERR_VMCONF_VIDEO_MEMORY_OUT_OF_RANGE:
		case PRL_ERR_VMCONF_INVALID_DEVICE_MAIN_INDEX:
		case PRL_ERR_VMCONF_DESKTOP_MODE_REMOTE_DEVICES:
		case PRL_ERR_VMCONF_BOOTCAMP_HARD_DISK_SMART_GUARD_NOT_ALLOW:
		case PRL_ERR_VMCONF_INCOMPAT_HARD_DISK_SMART_GUARD_NOT_ALLOW:
		case PRL_ERR_VMCONF_VIDEO_NOT_ENABLED:
		case PRL_ERR_VMCONF_GENERIC_PCI_WRONG_DEVICE:
		case PRL_ERR_VMCONF_GENERIC_PCI_VIDEO_WRONG_COUNT:
		case PRL_ERR_VMCONF_CPUUNITS_NOT_SUPPORTED:
		case PRL_ERR_VMCONF_CPULIMIT_NOT_SUPPORTED:
		case PRL_ERR_VMCONF_IOPRIO_NOT_SUPPORTED:
		case PRL_ERR_VMCONF_IOLIMIT_NOT_SUPPORTED:
		case PRL_ERR_VMCONF_IOPSLIMIT_NOT_SUPPORTED:
		case PRL_ERR_VMCONF_MAIN_MEMORY_MAX_BALLOON_SIZE_MORE_100_PERCENT:
		case PRL_ERR_VMCONF_MAIN_MEMORY_MQ_PRIOR_ZERO:
		case PRL_ERR_VMCONF_MAIN_MEMORY_MQ_PRIOR_OUT_OF_RANGE:
		case PRL_ERR_VMCONF_MAIN_MEMORY_MQ_INVALID_RANGE:
		case PRL_ERR_VMCONF_MAIN_MEMORY_MQ_MIN_LESS_VMM_OVERHEAD_VALUE:
		case PRL_ERR_VMCONF_MAIN_MEMORY_MQ_MIN_OUT_OF_RANGE:
		case PRL_ERR_VMCONF_EFI_UNSUPPORTED_GUEST:
		case PRL_ERR_REMOTE_DISPLAY_WRONG_PORT_NUMBER:
		case PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX:
		case PRL_ERR_VMCONF_SATA_DEVICES_DUPLICATE_STACK_INDEX:
		case PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX:
		case PRL_ERR_VMCONF_VIRTIO_BLOCK_DEVICES_DUPLICATE_STACK_INDEX:
		case PRL_ERR_UNSUPPORTED_DEVICE_TYPE:
		{
			evtResult.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
			evtResult.setEventCode(m_lstResults[i]);
			AddParameters(i, evtResult);
			return true;
		}
		break;
		case PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_MAC_ADDRESS:
		break;
		case PRL_ERR_VMCONF_CPU_MASK_INVALID_CPU_NUM:
			if ( PAM_SERVER == ParallelsDirs::getAppExecuteMode() )
			{
				evtResult.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
				evtResult.setEventCode(m_lstResults[i]);
				AddParameters(i, evtResult);
				return true;
			}
		break;
		case PRL_ERR_VMCONF_HARD_DISK_UNABLE_DELETE_DISK_WITH_SNAPSHOTS:
			if ( !(validateInternalFlags & PCVAL_ALLOW_DESTROY_HDD_BUNDLE_WITH_SNAPSHOTS) )
			{
				evtResult.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
				evtResult.setEventCode(m_lstResults[i]);
				AddParameters(i, evtResult);
				return true;
			}
		break;
		case PRL_ERR_VMCONF_VM_NAME_IS_TOO_LONG:
		{
			evtResult.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
			evtResult.setEventCode(m_lstResults[i]);
			AddParameters(i, evtResult);
			return true;
		}
		default:
			;
		}
	}
	return false;
}

bool CVmValidateConfig::HasCriticalErrorsForStartVm(CVmEvent& evtResult)
{
	if (HasError(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_NOT_CONNECTED))
	{
		WRITE_TRACE(DBG_FATAL, "Vm config contains not connected Vt-d devices !" );

		evtResult.setEventCode( PRL_ERR_CANT_START_VM_SINCE_NO_VTD_DRIVER );
		evtResult.addEventParameter(
			new CVmEventParameter(PVE::String,
								  GetParameter(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_NOT_CONNECTED, 1),
								  EVT_PARAM_MESSAGE_PARAM_0 ));
	}
	else if (HasError(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM)
			&& GetParameter(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM, 2).toInt() != 0)
	{
		WRITE_TRACE(DBG_FATAL, "Vm config contains Vt-d devices are used by others VM's !" );

		evtResult.setEventCode( PRL_ERR_CANT_START_VM_SINCE_VTD_DEVICE_ALREADY_USED );
		evtResult.addEventParameter(
			new CVmEventParameter(PVE::String,
								  GetParameter(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM, 1),
								  EVT_PARAM_MESSAGE_PARAM_0 ));
	}
	else if (HasError(PRL_ERR_VMCONF_HARD_DISK_WRONG_TYPE_BOOTCAMP_PARTITION)
				&& ! CDspService::isServerModePSBM())
	{
		WRITE_TRACE(DBG_FATAL, "Vm config contains not supported bootcamp partition(s) !" );

		evtResult.setEventCode( PRL_ERR_VMCONF_HARD_DISK_WRONG_TYPE_BOOTCAMP_PARTITION );
		evtResult.addEventParameter(
			new CVmEventParameter(PVE::String,
								  GetParameter(PRL_ERR_VMCONF_HARD_DISK_WRONG_TYPE_BOOTCAMP_PARTITION, 0),
								  EVT_PARAM_MESSAGE_PARAM_0 ));
		evtResult.addEventParameter(
			new CVmEventParameter(PVE::String,
								  GetParameter(PRL_ERR_VMCONF_HARD_DISK_WRONG_TYPE_BOOTCAMP_PARTITION, 1),
								  EVT_PARAM_MESSAGE_PARAM_1 ));
	}
	else if (HasError(PRL_ERR_VMCONF_HARD_DISK_MISS_BOOTCAMP_PARTITION)
				&& ! CDspService::isServerModePSBM())
	{
		WRITE_TRACE(DBG_FATAL, "Vm config contains missed bootcamp partition(s) !" );

		evtResult.setEventCode( PRL_ERR_VMCONF_HARD_DISK_MISS_BOOTCAMP_PARTITION );
		evtResult.addEventParameter(
			new CVmEventParameter(PVE::String,
								  GetParameter(PRL_ERR_VMCONF_HARD_DISK_MISS_BOOTCAMP_PARTITION, 0),
								  EVT_PARAM_MESSAGE_PARAM_0 ));
		evtResult.addEventParameter(
			new CVmEventParameter(PVE::String,
								  GetParameter(PRL_ERR_VMCONF_HARD_DISK_MISS_BOOTCAMP_PARTITION, 1),
								  EVT_PARAM_MESSAGE_PARAM_1 ));
	}
	else
	{
		return false;
	}

	return true;
}

void CVmValidateConfig::AddParameters(int nResultIndex, CVmEvent& evtResult)
{
	QStringList lstParameters = m_mapParameters.value(nResultIndex + 1);
	for(int i = 0; i < lstParameters.size() && i < 3; ++i)
	{
		QString sParamName;
		switch(i)
		{
		case 0:
			sParamName = EVT_PARAM_MESSAGE_PARAM_0;
			break;
		case 1:
			sParamName = EVT_PARAM_MESSAGE_PARAM_1;
			break;
		case 2:
			sParamName = EVT_PARAM_MESSAGE_PARAM_2;
			break;
		default:
			;
		}

		evtResult.addEventParameter(new CVmEventParameter(PVE::String, lstParameters[i], sParamName));
	}

	if (m_mapDevInfo.contains(nResultIndex + 1))
	{
		int nDevIndex = (int)m_mapDevInfo.value(nResultIndex + 1).m_devIdx;
		int nDevItemId = (int)m_mapDevInfo.value(nResultIndex + 1).m_devItemId;
		evtResult.addEventParameter(new CVmEventParameter(PVE::Integer, QString::number(nDevIndex),
															EVT_PARAM_VM_CONFIG_DEV_INDEX));
		evtResult.addEventParameter(new CVmEventParameter(PVE::Integer, QString::number(nDevItemId),
															EVT_PARAM_VM_CONFIG_DEV_ITEM_ID));
	}
	if (m_mapPciDevClasses.contains(nResultIndex + 1))
	{
		int nPciDevClass = m_mapPciDevClasses.value(nResultIndex + 1);
		evtResult.addEventParameter(new CVmEventParameter(PVE::Integer, QString::number(nPciDevClass),
															EVT_PARAM_DEV_GENERIC_PCI_TYPE));
	}
}

QString CVmValidateConfig::GetParameter(PRL_RESULT nResult, int nIndex) const
{

	int nResultIndex = m_lstResults.indexOf(nResult);
	if ( nResultIndex == -1 )
		return QString();

	QStringList lstParams = m_mapParameters.value(nResultIndex + 1);
	if (nIndex >= lstParams.size())
		return QString();

	return lstParams.at(nIndex);
}

bool CVmValidateConfig::HasError(PRL_RESULT nResult) const
{
	return m_lstResults.contains(nResult);
}

void CVmValidateConfig::validateSectionConfig(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg)
{
	// retrieve user parameters from request data

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(pkg);
	if (!cmd || !cmd->IsValid())
	{
		pUserSession->sendSimpleResponse(pkg, PRL_ERR_FAILURE);
		return;
	}

	QString vm_config = cmd->GetFirstStrParam();

	// parse VM configuration XML

	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration(vm_config));
	if (!IS_OPERATION_SUCCEEDED(pVmConfig->m_uiRcInit))
	{
		PRL_RESULT code = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Error occurred while checking VM configuration with code [%#x (%s)]"
			, code
			, PRL_RESULT_TO_STRING( code ) );
		pUserSession->sendSimpleResponse( pkg, code );
		return;
	}

	// check VM configuration

	CProtoCreateVmValidateConfigCommand* pVmValidateConfig =
		CProtoSerializer::CastToProtoCommand<CProtoCreateVmValidateConfigCommand>( cmd );
	PRL_VM_CONFIG_SECTIONS nSection = pVmValidateConfig->GetSection();

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pUserSession->getAuthHelper() );

	CVmValidateConfig vmValidateConfig(pUserSession, pVmConfig);

	QList<PRL_RESULT> lstResults;
	if (pVmConfig->getVmType() == PVT_CT)
		lstResults = vmValidateConfig.CheckCtConfig(nSection);
	else
		lstResults = vmValidateConfig.CheckVmConfig(nSection);

	// Send response

	if (lstResults.isEmpty())
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_SUCCESS );
		return;
	}

	QList<QString> lstErrors;
	for (int i = 0; i < lstResults.size(); i++)
	{
		if (lstResults[i] == PRL_ERR_SUCCESS)
			continue;

		CVmEvent result;
		result.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
		result.setEventCode(lstResults[i]);
		vmValidateConfig.AddParameters(i, result);

		lstErrors += result.toString();
	}

	if (lstErrors.isEmpty())
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_SUCCESS );
		return;
	}

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_VMCONF_VALIDATION_FAILED );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetParamsList(lstErrors);

	pUserSession->sendResponse(pCmd, pkg);
}

bool CVmValidateConfig::HasSysNameInvalidSymbol(const QString& qsSysName)
{
	for(int i = qsSysName.size() - 1; i >= 0 && qsSysName.at(i) != '\\' && qsSysName.at(i) != '/'; i--)
	{
		if (CFileHelper::GetInvalidPathSymbols().contains(qsSysName.at(i)))
		{
			return true;
		}
	}
	return false;
}

QString CVmValidateConfig::GetFileNameFromInvalidPath(const QString& qsSysName)
{
	int idx = qsSysName.lastIndexOf('/');
	if (idx == -1)
	{
		return qsSysName;
	}
	return qsSysName.mid(idx + 1);
}

bool CVmValidateConfig::IsUrlFormatSysName(const QString& qsSysName) const
{
	for(int i = 0; i < g_UrlSchemeList.size(); i++)
	{
		if (qsSysName.startsWith(g_UrlSchemeList[i]))
		{
			return true;
		}
	}
	return false;
}

bool CVmValidateConfig::IsDeviceInAnotherVm(PRL_DEVICE_TYPE nDevType,
											const QString& qsSysName,
											const QString& qsVmUuidToSkip,
											bool& bHasRunningAnotherVm)
{
	Vm::Directory::Dao::Locked d;
	bool bIsDeviceInAnotherVm = false;
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		if (i.second->getVmUuid() == qsVmUuidToSkip)
			continue;

		CVmConfiguration vm_config;
		QFile file(i.second->getVmHome());
		if (PRL_FAILED( vm_config.loadFromFile(&file) ))
			continue;

		CVmHardware* pHwList = vm_config.getVmHardwareList();
		QList<CVmDevice* >* pDevList = (QList<CVmDevice* >* )pHwList->m_aDeviceLists[nDevType];
		if ( ! pDevList )
			continue;

		foreach(CVmDevice* pDev, *pDevList)
		{
			if (pDev->getEnabled() == PVE::DeviceDisabled)
				continue;
			if (nDevType == PDE_GENERIC_NETWORK_ADAPTER
				&& pDev->getEmulatedType() != PDT_USE_DIRECT_ASSIGN)
				continue;

			bool bEqualSysNames = false;
			switch(nDevType)
			{
			case PDE_GENERIC_PCI_DEVICE:
			case PDE_GENERIC_NETWORK_ADAPTER:
			case PDE_PCI_VIDEO_ADAPTER:
				// Vt-d devices
				{
					QStringList lstDev1 = pDev->getSystemName().split(":");
					QStringList lstDev2 = qsSysName.split(":");
					if ( lstDev1.size() < 2 || lstDev2.size() < 2 )
						break;

					bEqualSysNames = (lstDev1[0] == lstDev2[0] && lstDev1[1] == lstDev2[1]);
				}
				break;
			default:
				bEqualSysNames = (pDev->getSystemName() == qsSysName);
			}

			if (bEqualSysNames)
			{
				bIsDeviceInAnotherVm = true;

				VIRTUAL_MACHINE_STATE nVmState = CDspVm::getVmState(i.second->getVmUuid(), i.first);
				if (nVmState != VMS_STOPPED && nVmState != VMS_SUSPENDED)
				{
					bHasRunningAnotherVm = true;
					return true;
				}
			}
		}
	}

	return bIsDeviceInAnotherVm;
}

bool CVmValidateConfig::getOldVmConfig()
{
	if ( m_pVmConfigOld )
		return true;

	PRL_ASSERT( m_pClient );

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	m_pVmConfigOld = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(m_pClient, m_pVmConfig->getVmIdentification()->getVmUuid(), ret );
	if (PRL_FAILED(ret))
	{
		m_pVmConfigOld = SmartPtr<CVmConfiguration>();
		m_lstResults += ret;
		return false;
	}

	return true;
}

void CVmValidateConfig::CheckGeneralParameters()
{
	if (!m_pVmConfig)
	{
		return;
	}

	Validation::Sink sink(*m_pVmConfig, m_lstResults, m_mapParameters, m_mapFullItemIds);

	boost::mpl::for_each<Validation::problem_types, boost::mpl::quote1<Validation::Traits> >(boost::ref(sink));

	unsigned int nOsType = m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType();
	unsigned int nOsVersion = m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion();
	switch(nOsType)
	{
	case PVS_GUEST_TYPE_MACOS:
		if ( !IS_VALID_MACOS_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_WINDOWS:
		if ( !IS_VALID_WINDOWS_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_LINUX:
		if ( !IS_VALID_LINUX_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_FREEBSD:
		if ( !IS_VALID_FREEBSD_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_OS2:
		if ( !IS_VALID_OS2_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_MSDOS:
		if ( !IS_VALID_MSDOS_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_NETWARE:
		if ( !IS_VALID_NETWARE_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_SOLARIS:
		if ( !IS_VALID_SOLARIS_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_CHROMEOS:
		if ( !IS_VALID_CHROMEOS_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_ANDROID:
		if ( !IS_VALID_ANDROID_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	case PVS_GUEST_TYPE_OTHER:
		if ( !IS_VALID_OTHER_OS_VERSION(nOsVersion) )
			m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_VERSION;
		break;
	default:
		m_lstResults += PRL_ERR_VMCONF_UNKNOWN_OS_TYPE;
	}

	QString qsOsVer_id = m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion_id();
	ADD_FID(E_SET << m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType_id() << qsOsVer_id);

	if (m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->getUndoDisksModeEx() != PUD_DISABLE_UNDO_DISKS)
		m_lstResults += PRL_ERR_UNIMPLEMENTED;

	if ( ! m_pVmConfig->getVmHardwareList()->getVideo()->isEnabled() )
	{
		QSet<QString > setPciVA_ids = E_SET << m_pVmConfig->getVmHardwareList()->getVideo()->getEnabled_id();
		bool bNoPciVideoAdapter = true;
		foreach( CVmPciVideoAdapter* pAdapter, m_pVmConfig->getVmHardwareList()->m_lstPciVideoAdapters )
		{
			if ( pAdapter->getEnabled() == PVE::DeviceEnabled )
			{
				bNoPciVideoAdapter = false;
				break;
			}
			setPciVA_ids << pAdapter->getFullItemId() << pAdapter->getEnabled_id();
		}

		if ( bNoPciVideoAdapter )
		{
			m_lstResults += PRL_ERR_VMCONF_VIDEO_NOT_ENABLED;
			if (m_pVmConfigOld.isValid())
			{
				foreach( CVmPciVideoAdapter* pAdapter, m_pVmConfigOld->getVmHardwareList()->m_lstPciVideoAdapters )
				{
					if ( ! setPciVA_ids.contains(pAdapter->getFullItemId()) )
						setPciVA_ids << pAdapter->getFullItemId();
				}
			}
			ADD_FID(setPciVA_ids);
		}
	}
	// Vz specific parameter on non Vz node
	bool bVzRunning = false;
#ifdef _CT_
	bVzRunning = CVzHelper::is_vz_running();
#endif

	if (m_pVmConfigOld && !bVzRunning)
	{
		if (m_pVmConfig->getVmHardwareList()->getCpu()->getCpuUnits() !=
				m_pVmConfigOld->getVmHardwareList()->getCpu()->getCpuUnits())
		{
			m_lstResults += PRL_ERR_VMCONF_CPUUNITS_NOT_SUPPORTED;
			ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getCpu()->getCpuUnits_id());
		}

		if (m_pVmConfig->getVmHardwareList()->getCpu()->getCpuLimit() !=
				m_pVmConfigOld->getVmHardwareList()->getCpu()->getCpuLimit() ||
			m_pVmConfig->getVmHardwareList()->getCpu()->getCpuLimitType() !=
				m_pVmConfigOld->getVmHardwareList()->getCpu()->getCpuLimitType() ||
			m_pVmConfig->getVmHardwareList()->getCpu()->getCpuLimitValue() !=
				m_pVmConfigOld->getVmHardwareList()->getCpu()->getCpuLimitValue())
		{
			m_lstResults += PRL_ERR_VMCONF_CPULIMIT_NOT_SUPPORTED;
			ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getCpu()->getCpuLimit_id()
				<< m_pVmConfig->getVmHardwareList()->getCpu()->getCpuLimitType_id()
				<< m_pVmConfig->getVmHardwareList()->getCpu()->getCpuLimitValue_id());
		}

		if (m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->getIoPriority() !=
				m_pVmConfigOld->getVmSettings()->getVmRuntimeOptions()->getIoPriority())
		{
			m_lstResults += PRL_ERR_VMCONF_IOPRIO_NOT_SUPPORTED;
			ADD_FID(E_SET << m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->getIoPriority_id());
		}
	}
}

void CVmValidateConfig::CheckBootOption()
{
	if (!m_pVmConfig)
	{
		return;
	}

	int i = -1;
	QMultiMap<PRL_DEVICE_TYPE , unsigned int > mapIndexes;

	CVmHardware* pHardware = m_pVmConfig->getVmHardwareList();

	BootingOrder* pBootingOrder = m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBootingOrder();
	QList<CVmBootDeviceBase* > lstBootDevices = pBootingOrder->m_lstBootDevice;
	for(i = 0; i < lstBootDevices.size(); i++)
	{
		BootDevice* pBootDevice = lstBootDevices[i];

		PRL_DEVICE_TYPE nType = pBootDevice->getType();
		unsigned int nIndex = pBootDevice->getIndex();

		switch(nType)
		{
		case PDE_FLOPPY_DISK:
		case PDE_HARD_DISK:
		case PDE_OPTICAL_DISK:
		case PDE_GENERIC_NETWORK_ADAPTER:
		case PDE_USB_DEVICE:
			{
				QSet<QString > setDev_ids = E_SET << pBootDevice->getFullItemId() << pBootDevice->getIndex_id();
				bool bExist = false;
				QList<CVmDevice* >* plstDevList = (QList<CVmDevice* >* )pHardware->m_aDeviceLists[nType];
				PRL_ASSERT( plstDevList );
				for(int j = 0; plstDevList && j < plstDevList->size(); j++)
				{
					CVmDevice* pDevice = plstDevList->at(j);
					if (pDevice && nIndex == pDevice->getIndex())
					{
						bExist = true;
						break;
					}
					setDev_ids << pDevice->getFullItemId() << pDevice->getIndex_id();
				}
				if (!bExist)
				{
					m_lstResults += PRL_ERR_VMCONF_BOOT_OPTION_DEVICE_NOT_EXISTS;
					if (m_pVmConfigOld.isValid())
					{
						QList<CVmDevice* >* plstDevList
							= (QList<CVmDevice* >* )m_pVmConfigOld->getVmHardwareList()->m_aDeviceLists[nType];
						PRL_ASSERT( plstDevList );
						for(int j = 0; plstDevList && j < plstDevList->size(); j++)
						{
							CVmDevice* pDevice = plstDevList->at(j);
							if ( ! pDevice ) continue;
							if ( ! setDev_ids.contains(pDevice->getFullItemId()) )
								setDev_ids << pDevice->getFullItemId();
						}
					}
					ADD_FID(setDev_ids);
				}
			}
			break;
		default:
			m_lstResults += PRL_ERR_VMCONF_BOOT_OPTION_INVALID_DEVICE_TYPE;
			ADD_FID(E_SET << pBootDevice->getType_id());
		}

		if (mapIndexes.values(nType).contains(nIndex))
		{
			m_lstResults += PRL_ERR_VMCONF_BOOT_OPTION_DUPLICATE_DEVICE;
			ADD_FID(E_SET << pBootDevice->getIndex_id() << pBootDevice->getType_id());
		}

		mapIndexes.insert(nType, nIndex);
	}

	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType() == PVS_GUEST_TYPE_WINDOWS
		&& m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion() < PVS_GUEST_VER_WIN_WINDOWS8
		&& m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled())
	{
		m_lstResults += PRL_ERR_VMCONF_EFI_UNSUPPORTED_GUEST;
	}
}

void CVmValidateConfig::CheckRemoteDisplay()
{
	if (!m_pVmConfig)
	{
		return;
	}

	CVmRemoteDisplay* pRemoteDisplay = m_pVmConfig->getVmSettings()->getVmRemoteDisplay();
	if (pRemoteDisplay->getMode() == PRD_DISABLED)
	{
		return;
	}

	QString qsRDM_id = pRemoteDisplay->getMode_id();

	if (pRemoteDisplay->getMode() == PRD_MANUAL
	    && pRemoteDisplay->getPortNumber() < PRL_VM_REMOTE_DISPAY_MIN_PORT)
	{
		m_lstResults += PRL_ERR_REMOTE_DISPLAY_WRONG_PORT_NUMBER;
		ADD_FID(E_SET << qsRDM_id << pRemoteDisplay->getPortNumber_id());
	}

	QHostAddress haIpAddress;
	QString sIpAddress = pRemoteDisplay->getHostName();
	if (!haIpAddress.setAddress(sIpAddress))
	{
		m_lstResults += PRL_ERR_VMCONF_REMOTE_DISPLAY_INVALID_HOST_IP_ADDRESS;
		ADD_FID(E_SET << qsRDM_id << pRemoteDisplay->getHostName_id());
	}

	if (pRemoteDisplay->getPassword().isEmpty())
	{
		m_lstResults += PRL_ERR_VMCONF_REMOTE_DISPLAY_EMPTY_PASSWORD;
		ADD_FID(E_SET << qsRDM_id << pRemoteDisplay->getPassword_id());
	}
}

void CVmValidateConfig::CheckSharedFolders()
{
	if (!m_pVmConfig)
	{
		return;
	}

	QSet<QString > setFolderNames;
	QSet<QString > setFolderPathes;

	CVmHostSharing* pHostSharing = m_pVmConfig->getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	if (!pHostSharing->isUserDefinedFoldersEnabled())
	{
		return;
	}

	QList<CVmSharedFolder* > lstSharedFolders = pHostSharing->m_lstSharedFolders;
	for(int i = 0; i < lstSharedFolders.size(); i++)
	{
		CVmSharedFolder* pSharedFolder = lstSharedFolders[i];
		if (!pSharedFolder->isEnabled())
			continue;

		QSet<QString > setIds = E_SET << pHostSharing->getUserDefinedFoldersEnabled_id()
			<< pSharedFolder->getFullItemId() << pSharedFolder->getEnabled_id();

		QString sFolderName = pSharedFolder->getName().toLower();
		QString sFolderPath = pSharedFolder->getPath();

		if (sFolderName.isEmpty())
		{
			m_lstResults += PRL_ERR_VMCONF_SHARED_FOLDERS_EMPTY_FOLDER_NAME;
			ADD_FID((E_SET << pSharedFolder->getName_id()) + setIds);
		}
		else if (setFolderNames.contains(sFolderName))
		{
			m_lstResults += PRL_ERR_VMCONF_SHARED_FOLDERS_DUPLICATE_FOLDER_NAME;
			ADD_FID((E_SET << pSharedFolder->getName_id()) + setIds);
		}

		QDir dir;
		if (!dir.exists(sFolderPath))
		{
			m_lstResults += PRL_ERR_VMCONF_SHARED_FOLDERS_INVALID_FOLDER_PATH;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
					<< sFolderName << sFolderPath);
			ADD_FID((E_SET << pSharedFolder->getPath_id()) + setIds);
		}

		if (setFolderPathes.contains(sFolderPath))
		{
			m_lstResults += PRL_ERR_VMCONF_SHARED_FOLDERS_DUPLICATE_FOLDER_PATH;
			ADD_FID((E_SET << pSharedFolder->getPath_id()) + setIds);
		}

		setFolderNames.insert(sFolderName);
		setFolderPathes.insert(sFolderPath);
	}
}

void CVmValidateConfig::CheckCpu()
{
	if (!m_pVmConfig)
	{
		return;
	}

	CVmCpu *pCpu = m_pVmConfig->getVmHardwareList()->getCpu();
	unsigned int nCpuCount = pCpu->getNumber();
	if (!nCpuCount)
	{
		m_lstResults += PRL_ERR_VMCONF_CPU_ZERO_COUNT;
		ADD_FID(E_SET << pCpu->getNumber_id());
	}
	else if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
	{
		CDspLockedPointer<CDspHostInfo> pDspHostInfo = CDspService::instance()->getHostInfo();
		if ( ! pDspHostInfo.isValid() )
			return;
		CHostHardwareInfo* pHostInfo = pDspHostInfo->data();
		if ( ! pHostInfo )
			return;

		CDspLockedPointer<CDispCommonPreferences> pCommonPreferences =
	                    CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
		unsigned int max = pCommonPreferences->getCpuPreferences()->getMaxVCpu();

		unsigned int nHostCpuCount = pHostInfo->getCpu()->getNumber();
		unsigned int maxAllowedCpuCount = qMin(nHostCpuCount, max > 0 ? max : PRL_MAX_CPU_COUNT);
		if (nCpuCount > maxAllowedCpuCount)
		{
			m_lstResults += PRL_ERR_VMCONF_CPU_COUNT_MORE_MAX_CPU_COUNT;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< QString::number(maxAllowedCpuCount));
			ADD_FID(E_SET << pCpu->getNumber_id());
		}

		if (nCpuCount > nHostCpuCount)
		{
			m_lstResults += PRL_ERR_VMCONF_CPU_COUNT_MORE_HOST_CPU_COUNT;
			ADD_FID(E_SET << pCpu->getNumber_id());
		}

		QString sMask = pCpu->getCpuMask();
		if (!sMask.isEmpty() && PAM_SERVER == ParallelsDirs::getAppExecuteMode())
		{
			char bMask[Parallels::MAX_NCPU];

			if (Parallels::parseCpuMask(sMask, nHostCpuCount, bMask, sizeof(bMask)))
			{
				m_lstResults += PRL_ERR_VMCONF_CPU_MASK_INVALID;
				ADD_FID(E_SET << pCpu->getCpuMask_id());
			} else {
				unsigned int nBits = getBitsCount(bMask, sizeof(bMask));

				if (nCpuCount < nHostCpuCount && nBits < nCpuCount) {
					m_lstResults += PRL_ERR_VMCONF_CPU_MASK_INVALID_CPU_NUM;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< QString::number(nBits));
					ADD_FID(E_SET << pCpu->getCpuMask_id() << pCpu->getNumber_id());
				}
			}
		}

		QString sNodeMask = pCpu->getNodeMask();
		if (!sNodeMask.isEmpty())
		{
			char bMask[Parallels::MAX_NCPU] = {};

			if (Parallels::parseNodeMask(sNodeMask, bMask, sizeof(bMask)))
			{
				m_lstResults += PRL_ERR_VMCONF_CPU_NODE_MASK_INVALID;
				ADD_FID(E_SET << pCpu->getNodeMask_id());
			}
		}
	}
}

void CVmValidateConfig::CheckMainMemory()
{
	if (!m_pVmConfig)
	{
		return;
	}

	unsigned int nMemSize = m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize();
	CDspLockedPointer<CDispCommonPreferences> pCommonPreferences =
	                    CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
	unsigned int uiMaxVmMemory = pCommonPreferences->getMemoryPreferences()->getMaxVmMemory();

	if (!nMemSize)
	{
		m_lstResults += PRL_ERR_VMCONF_MAIN_MEMORY_ZERO_SIZE;
		ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize_id());
	}
	else if (nMemSize < VM_MIN_MEM || nMemSize > uiMaxVmMemory)
	{
		m_lstResults += PRL_ERR_VMCONF_MAIN_MEMORY_OUT_OF_RANGE;
		m_mapParameters.insert(m_lstResults.size(), QStringList()
			<< QString::number(VM_MIN_MEM) << QString::number(uiMaxVmMemory));
		ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize_id());
	}
	else if ((nMemSize & 3) != 0)
	{
		m_lstResults += PRL_ERR_VMCONF_MAIN_MEMORY_NOT_4_RATIO_SIZE;
		ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize_id());
	}
	else
	{
		unsigned int nMaxRecommendSize = pCommonPreferences->getMemoryPreferences()->getRecommendedMaxVmMemory();

		unsigned int nOsVersion = m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion();
		unsigned int nMinRecommendSize = osInfo_getMinRecommendedOsMemorySize(nOsVersion);

		if (nMemSize < nMinRecommendSize || nMemSize > nMaxRecommendSize)
		{
			m_lstResults += PRL_ERR_VMCONF_MAIN_MEMORY_SIZE_NOT_EAQUAL_RECOMMENDED;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
				<< QString::number(nMemSize) << QString::number(nMinRecommendSize)
				<< QString::number(nMaxRecommendSize));
			ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize_id());
		}
	}

	// Validate non auto quota
	// TODO: the chwck is switched off now
}

void CVmValidateConfig::CheckVideoMemory()
{
	if (!m_pVmConfig)
	{
		return;
	}

	unsigned int nVideoMemSize = m_pVmConfig->getVmHardwareList()->getVideo()->getMemorySize();
	if ( !(nVideoMemSize >= VM_MIN_VIDEO_MEM && nVideoMemSize <= VM_MAX_VIDEO_MEM) )
	{
		m_lstResults += PRL_ERR_VMCONF_VIDEO_MEMORY_OUT_OF_RANGE;
		m_mapParameters.insert(m_lstResults.size(), QStringList()
			<< QString::number(VM_MIN_VIDEO_MEM) << QString::number(VM_MAX_VIDEO_MEM));
		ADD_FID(E_SET << m_pVmConfig->getVmHardwareList()->getVideo()->getMemorySize_id());
	}
}

void CVmValidateConfig::CheckFloppyDisk()
{
	if (!m_pVmConfig)
	{
		return;
	}

	QList<CVmFloppyDisk* > lstFloppyDisk = m_pVmConfig->getVmHardwareList()->m_lstFloppyDisks;
	if (lstFloppyDisk.isEmpty())
	{
		return;
	}

	CVmFloppyDisk* pFloppyDisk = lstFloppyDisk[0];
	if ( ! pFloppyDisk || pFloppyDisk->getEnabled() != PVE::DeviceEnabled
            || pFloppyDisk->getConnected() != PVE::DeviceConnected )
		return;

	QSet<QString > setIds = E_SET << pFloppyDisk->getFullItemId()
		<< pFloppyDisk->getEnabled_id() << pFloppyDisk->getConnected_id();

	QString qsSysName = pFloppyDisk->getSystemName();
	if (qsSysName.isEmpty())
	{
		m_lstResults += PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_IS_EMPTY;
		ADD_FID((E_SET << pFloppyDisk->getSystemName_id()) + setIds);
		return;
	}

	setIds << pFloppyDisk->getEmulatedType_id();

	if (pFloppyDisk->getEmulatedType() == PVE::FloppyDiskImage)
	{
		setIds << pFloppyDisk->getSystemName_id();

		if (IsUrlFormatSysName(qsSysName))
		{
			m_lstResults += PRL_ERR_VMCONF_FLOPPY_DISK_URL_FORMAT_SYS_NAME;
			m_mapParameters.insert(m_lstResults.size(), QStringList() << qsSysName);
			ADD_FID(setIds);
		}
		else if (HasSysNameInvalidSymbol(qsSysName))
		{
			m_lstResults += PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_HAS_INVALID_SYMBOL;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< GetFileNameFromInvalidPath(qsSysName));
			ADD_FID(setIds);
		}
		else if (   !pFloppyDisk->isRemote()
				&& !CFileHelper::isRemotePath(qsSysName)
				&& !QFile::exists(qsSysName))
		{
			m_lstResults += PRL_ERR_VMCONF_FLOPPY_DISK_IMAGE_IS_NOT_EXIST;
			m_mapParameters.insert(m_lstResults.size(), QStringList() << qsSysName);
			ADD_FID((E_SET << pFloppyDisk->getRemote_id()) + setIds);
		}
	}
	else if (pFloppyDisk->getEmulatedType() == PVE::RealFloppyDisk)
	{
		CDspLockedPointer<CDspHostInfo> pDspHostInfo = CDspService::instance()->getHostInfo();
		if (pDspHostInfo.isValid())
		{
			CHostHardwareInfo* pHostInfo = pDspHostInfo->data();
			if (pHostInfo && pHostInfo->m_lstFloppyDisks.isEmpty())
			{
				m_lstResults += PRL_ERR_VMCONF_FLOPPY_DISK_IS_NOT_ACCESSIBLE;
				ADD_FID(setIds);
			}
		}
	}
}

void CVmValidateConfig::CheckCdDvdRom()
{
	if (!m_pVmConfig)
	{
		return;
	}

	QStringList lstSysNames;

	QList<CVmOpticalDisk* > lstOpticalDisks = m_pVmConfig->getVmHardwareList()->m_lstOpticalDisks;
	for(int i = 0; i < lstOpticalDisks.size(); i++)
	{
		CVmOpticalDisk* pOpticalDisk = lstOpticalDisks[i];
		if ( !pOpticalDisk || ( pOpticalDisk->getEnabled() != PVE::DeviceEnabled )
			|| ( pOpticalDisk->getConnected() != PVE::DeviceConnected ) )
			continue;

		QSet<QString > setIds = E_SET << pOpticalDisk->getFullItemId()
			<< pOpticalDisk->getEnabled_id() << pOpticalDisk->getConnected_id();

		// check sata for old chipset
		if ( ( pOpticalDisk->getInterfaceType() == PMS_SATA_DEVICE ) &&
			!CHardDiskHelper::isSataSupportedForChipset( m_pVmConfig->getVmHardwareList()->getChipset()->getType() ) )
		{
			m_lstResults += PRL_ERR_VMCONF_CD_DVD_ROM_SET_SATA_FOR_OLD_CHIPSET;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList()<<QString::number(pOpticalDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pOpticalDisk->getIndex(), pOpticalDisk->getItemId()));
			ADD_FID((E_SET << pOpticalDisk->getInterfaceType_id()
				<< m_pVmConfig->getVmHardwareList()->getChipset()->getType_id()) + setIds);
		}

		// check sata for unsupported os
		if ( ( pOpticalDisk->getInterfaceType() == PMS_SATA_DEVICE ) &&
			!CHardDiskHelper::isSataSupportedForOs( m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType(),
											m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion() ) )
		{
			m_lstResults += PRL_ERR_VMCONF_CD_DVD_ROM_SET_SATA_FOR_UNSUPPORTED_OS;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList()<<QString::number(pOpticalDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pOpticalDisk->getIndex(), pOpticalDisk->getItemId()));
			ADD_FID((E_SET << pOpticalDisk->getInterfaceType_id()
				<< m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType_id()
				<< m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion_id()) + setIds);
		}

		setIds << pOpticalDisk->getSystemName_id();

		QString qsSysName = pOpticalDisk->getSystemName();
		if	(qsSysName.isEmpty())
			continue;
		else if (   IsUrlFormatSysName(qsSysName)
				 && pOpticalDisk->getEmulatedType() == PVE::CdRomImage)
		{
			m_lstResults += PRL_ERR_VMCONF_CD_DVD_ROM_URL_FORMAT_SYS_NAME;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
				<< qsSysName << QString::number(pOpticalDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pOpticalDisk->getIndex(), pOpticalDisk->getItemId()));
			ADD_FID((E_SET << pOpticalDisk->getEmulatedType_id()) + setIds);
		}
		else if (lstSysNames.contains(qsSysName) && pOpticalDisk->getEmulatedType() == PVE::RealCdRom)
		{
			m_lstResults += PRL_ERR_VMCONF_CD_DVD_ROM_DUPLICATE_SYS_NAME;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList()<<QString::number(pOpticalDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pOpticalDisk->getIndex(), pOpticalDisk->getItemId()));
			ADD_FID((E_SET << pOpticalDisk->getEmulatedType_id()) + setIds);
		}
		else
		{
			if ( pOpticalDisk->getEmulatedType() == PVE::RealCdRom )
				lstSysNames += qsSysName;

			if (pOpticalDisk->getEmulatedType() == PVE::CdRomImage
				&& !QFile::exists(qsSysName)
				&& !pOpticalDisk->isRemote()
				&& !CFileHelper::isRemotePath(qsSysName))
			{
				m_lstResults += PRL_ERR_VMCONF_CD_DVD_ROM_IMAGE_IS_NOT_EXIST;
				m_mapParameters.insert(m_lstResults.size(), QStringList()
					<< qsSysName << QString::number(pOpticalDisk->getIndex() + 1));
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pOpticalDisk->getIndex(), pOpticalDisk->getItemId()));
				ADD_FID((E_SET << pOpticalDisk->getEmulatedType_id()
					<< pOpticalDisk->getRemote_id()) + setIds);
			}
		}
	}
}

void CVmValidateConfig::CheckHardDisk()
{
	if ( !m_pVmConfig )
		return;

	QStringList lstAllPartitions;
	QStringList lstSupportedPartitions;
	GetAllSupportedPartitionSysNames(lstAllPartitions, lstSupportedPartitions);

	QHash<QString, uint> sysNamesToDevIdxMap;

	QList<CVmHardDisk* > lstHardDisks = m_pVmConfig->getVmHardwareList()->m_lstHardDisks;
	for(int i = 0; i < lstHardDisks.size(); i++)
	{
		CVmHardDisk* pHardDisk = lstHardDisks[i];
		if ( !pHardDisk || ( pHardDisk->getEnabled() != PVE::DeviceEnabled ) )
			continue;

		QSet<QString > setIds = E_SET << pHardDisk->getFullItemId() << pHardDisk->getEnabled_id();

		// check sata for old chipset
		if( ( pHardDisk->getInterfaceType() == PMS_SATA_DEVICE ) &&
			!CHardDiskHelper::isSataSupportedForChipset( m_pVmConfig->getVmHardwareList()->getChipset()->getType() ) )
		{
			m_lstResults += PRL_ERR_VMCONF_HARD_DISK_SET_SATA_FOR_OLD_CHIPSET;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList()<<QString::number(pHardDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
			ADD_FID((E_SET << pHardDisk->getInterfaceType_id()
				<< m_pVmConfig->getVmHardwareList()->getChipset()->getType_id()) + setIds);
		}

		// check sata for unsupported os
		if ( ( pHardDisk->getInterfaceType() == PMS_SATA_DEVICE ) &&
			!CHardDiskHelper::isSataSupportedForOs( m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType(),
			m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion() ) )
		{
			m_lstResults += PRL_ERR_VMCONF_HARD_DISK_SET_SATA_FOR_UNSUPPORTED_OS;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList()<<QString::number(pHardDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
			ADD_FID((E_SET << pHardDisk->getInterfaceType_id()
				<< m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType_id()
				<< m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion_id()) + setIds);
		}

		QString qsSN_id = pHardDisk->getSystemName_id();
		QString qsET_id = pHardDisk->getEmulatedType_id();

		QString qsSysName = pHardDisk->getSystemName();
		if (qsSysName.isEmpty())
		{
			m_lstResults += PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_IS_EMPTY;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList() << QString::number(pHardDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
			ADD_FID((E_SET << qsSN_id) + setIds);
		}
		else if (   IsUrlFormatSysName(qsSysName)
				 && pHardDisk->getEmulatedType() == PVE::HardDiskImage)
		{
			m_lstResults += PRL_ERR_VMCONF_HARD_DISK_URL_FORMAT_SYS_NAME;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
				<< qsSysName << QString::number(pHardDisk->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
			ADD_FID((E_SET << qsSN_id << qsET_id) + setIds);
		}
		else if (   HasSysNameInvalidSymbol(qsSysName)
				 && pHardDisk->getEmulatedType() == PVE::HardDiskImage)
		{
			m_lstResults += PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_HAS_INVALID_SYMBOL;
			m_mapParameters.insert(m_lstResults.size(), QStringList() << GetFileNameFromInvalidPath(qsSysName));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
			ADD_FID((E_SET << qsSN_id << qsET_id) + setIds);
		}
		else if (sysNamesToDevIdxMap.contains(qsSysName))
		{
			m_lstResults += PRL_ERR_VMCONF_HARD_DISK_DUPLICATE_SYS_NAME;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList() << QString::number(sysNamesToDevIdxMap[qsSysName]));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
			ADD_FID((E_SET << qsSN_id) + setIds);
		}
		else
		{
			sysNamesToDevIdxMap[qsSysName] = pHardDisk->getIndex();

			if (pHardDisk->getEmulatedType() == PVE::HardDiskImage
				&& !pHardDisk->isRemote()
				&& !CFileHelper::isRemotePath(qsSysName))
			{
				if ( !QFile::exists(qsSysName) )
				{
					m_lstResults += PRL_ERR_VMCONF_HARD_DISK_IMAGE_IS_NOT_EXIST;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< qsSysName << QString::number(pHardDisk->getIndex() + 1));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
					ADD_FID((E_SET << qsSN_id << qsET_id << pHardDisk->getRemote_id()) + setIds);
				}
				else if (!VirtualDisk::Qcow2::isValid(qsSysName))
				{
					m_lstResults += PRL_ERR_VMCONF_HARD_DISK_IMAGE_IS_NOT_VALID;
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
					ADD_FID((E_SET << qsSN_id << qsET_id << pHardDisk->getRemote_id()) + setIds);
				}
			}
		}
	}

    if (m_pVmConfigOld.isValid()) {
        QList<CVmHardDisk* > &lstOldHardDisks = m_pVmConfigOld->getVmHardwareList()->m_lstHardDisks;
        foreach( CVmHardDisk *pOldHdd, lstOldHardDisks )
        {
            if ( (PVE::HardDiskEmulatedType)PDT_USE_IMAGE_FILE == pOldHdd->getEmulatedType() &&
                    ! CXmlModelHelper::IsElemInList( pOldHdd, lstHardDisks ))
                CheckSnapshotsOfDisk(*pOldHdd);
        }
    }
}

void CVmValidateConfig::GetAllSupportedPartitionSysNames(QStringList& lstAllPartitions,
														 QStringList& /*lstSupportedPartitions*/) const
{
	do
	{
		CDspLockedPointer<CDspHostInfo> pDspHostInfo = CDspService::instance()->getHostInfo();
		if ( ! pDspHostInfo.isValid() )
			break;

		CHostHardwareInfo* pHostInfo = pDspHostInfo->data();
		if ( ! pHostInfo )
			break;

		foreach(CHwHardDisk* pHardDisk, pHostInfo->getHardDisks()->m_lstHardDisk)
		{
			if (pHardDisk->isRemovable())
				continue;

			QList<CHwHddPartition* > lstPartitions = pHardDisk->getAllPartitions(pHardDisk->m_lstPartitions);
			foreach(CHwHddPartition* pPart, lstPartitions)
			{
				lstAllPartitions += pPart->getSystemName();
			}
		}
	}
	while(0);
}

void CVmValidateConfig::CheckNetworkAdapter()
{
	if (!m_pVmConfig || m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
	{
		return;
	}

	QList<CVmGenericNetworkAdapter* > lstNetworkAdapters = m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters;
	if (lstNetworkAdapters.empty())
		return;

	QSet<QString > setNA_ids;

	do
	{
		CDspLockedPointer<CParallelsNetworkConfig>
			pNetworkConfig = CDspService::instance()->getNetworkConfig();
		if (!pNetworkConfig.isValid())
			break;

		PrlNet::EthAdaptersList ethAdaptersList;
		PRL_RESULT prlResult = PrlNet::makeBindableAdapterList(ethAdaptersList, true);
		if( PRL_FAILED(prlResult) )
		{
			m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_ETHLIST_CREATE_ERROR;
			break;
		}

		for(int i = 0; i < lstNetworkAdapters.size(); i++)
		{
			CVmGenericNetworkAdapter* pNetAdapter = lstNetworkAdapters[i];
			if ( ! pNetAdapter )
				continue;

			setNA_ids << pNetAdapter->getFullItemId() << pNetAdapter->getNetAddresses_id();
			if (pNetAdapter->getEnabled() != PVE::DeviceEnabled
				|| pNetAdapter->getEmulatedType() == PNA_DIRECT_ASSIGN
				|| pNetAdapter->getEmulatedType() == PNA_ROUTED)
				continue;

			QSet<QString > setIds = E_SET << pNetAdapter->getFullItemId()
				<< pNetAdapter->getEnabled_id() << pNetAdapter->getEmulatedType_id();

			if (PrlNet::getMode() == PRL_NET_MODE_VME &&
				!pNetAdapter->getVirtualNetworkID().isEmpty())
			{
				CVirtualNetwork *pNetwork = PrlNet::GetVirtualNetworkByID(
						pNetworkConfig.getPtr()->getVirtualNetworks(),
						pNetAdapter->getVirtualNetworkID());
				if ( !pNetwork )
				{
					WRITE_TRACE(DBG_FATAL, "Virtual network %s does not exist.",
							QSTR2UTF8(pNetAdapter->getVirtualNetworkID()));
					m_lstResults += PRL_NET_VMDEVICE_VIRTUAL_NETWORK_NOT_EXIST;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< pNetAdapter->getSystemName()
						<< pNetAdapter->getVirtualNetworkID());
					ADD_FID(setIds);
				}
			}
			else if (PrlNet::getMode() != PRL_NET_MODE_VME ||
				pNetAdapter->getEmulatedType() == PNA_BRIDGED_ETHERNET)
			{
				PrlNet::EthAdaptersList::Iterator itEthAdapter;
				prlResult = PrlNet::GetAdapterForVM( ethAdaptersList,
						pNetworkConfig.getPtr(), *pNetAdapter, itEthAdapter );
				if (PRL_FAILED(prlResult))
				{
					m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_BOUND_INDEX;
					m_mapParameters.insert(m_lstResults.size(),
						QStringList() << QString::number(i + 1));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
					ADD_FID(setIds);
				}
			}
		}
	} while(0);

	QList<QString > lstMacAddresses;
	QList<QHostAddress > lstIPAddresses;

	for(int i = 0; i < lstNetworkAdapters.size(); i++)
	{
		CVmGenericNetworkAdapter* pNetAdapter = lstNetworkAdapters[i];
		if (!pNetAdapter || pNetAdapter->getEnabled() != PVE::DeviceEnabled
			|| (PRL_VM_DEV_EMULATION_TYPE )pNetAdapter->getEmulatedType() == PDT_USE_DIRECT_ASSIGN)
			continue;

		QSet<QString > setIds = E_SET << pNetAdapter->getFullItemId()
			<< pNetAdapter->getEnabled_id() << pNetAdapter->getEmulatedType_id();

		PRL_RESULT errCode = PRL_ERR_SUCCESS;
		QString sMacAddress = pNetAdapter->getMacAddress();
		bool bCheckPrlAddress = false;

		if ( !HostUtils::checkMacAddress(sMacAddress, bCheckPrlAddress ) )
			errCode = PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_MAC_ADDRESS;

		if ( errCode != PRL_ERR_SUCCESS )
		{
			m_lstResults += errCode;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
				<< QString::number(lstNetworkAdapters[i]->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
			ADD_FID((E_SET << pNetAdapter->getMacAddress_id()) + setIds);
		}

		// NOTE: if this error will ever need a parameter, move it upper before
		// if and use the errCode var like above
		if (lstMacAddresses.contains(sMacAddress))
		{
			m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_MAC_ADDRESS;
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
			ADD_FID((E_SET << pNetAdapter->getMacAddress_id()) + setIds);
		}
		lstMacAddresses += sMacAddress;

		QString qsAA_id = pNetAdapter->getAutoApply_id();
		QString qsCWD_id = pNetAdapter->getConfigureWithDhcp_id();

		if ( pNetAdapter->isAutoApply() )
		{
			if ( !pNetAdapter->isConfigureWithDhcp() )
			{
				// Validate Default Gateway IP for:
				QString qsGatewayIp;

				//		- Valid IP
				if ( ! pNetAdapter->getDefaultGateway().isEmpty()
					&& ! NetworkUtils::ParseIpMask(pNetAdapter->getDefaultGateway(), qsGatewayIp) )
				{
					m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_GATEWAY_IP_ADDRESS;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< pNetAdapter->getDefaultGateway()
						<< QString::number(pNetAdapter->getIndex() + 1));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
					ADD_FID((E_SET << qsAA_id << qsCWD_id << pNetAdapter->getDefaultGateway_id()) + setIds);
				}

				//		- in existing Subnet
				bool bGatewayInSubnet = false;
				QList<QString> adapterIps = pNetAdapter->getNetAddresses();
				foreach (QString ip_mask, adapterIps)
				{
					QString ip;
					QString mask;

					if ( ! NetworkUtils::ParseIpMask(ip_mask, ip, mask) )
					{
						m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_IP_ADDRESS;
						m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< ip_mask << QString::number(pNetAdapter->getIndex() + 1));
						m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
						ADD_FID((E_SET << qsAA_id << qsCWD_id << pNetAdapter->getNetAddresses_id()) + setIds);
						continue;
					}

					quint32 uiIP = QHostAddress(ip).toIPv4Address();
					quint32 uiMask = QHostAddress(mask).toIPv4Address();

					if (QHostAddress("224.0.0.0").toIPv4Address() <= uiIP
						&& uiIP <= QHostAddress("239.255.255.255").toIPv4Address())
					{
						m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_MULTICAST_IP_ADDRESS;
						m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< ip_mask << QString::number(pNetAdapter->getIndex() + 1));
						m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
						ADD_FID((E_SET << qsAA_id << qsCWD_id << pNetAdapter->getNetAddresses_id()) + setIds);
					}
					else if ( ( uiIP & (~uiMask) ) == (~uiMask) )
					{
						m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_BROADCAST_IP_ADDRESS;
						m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< ip_mask << QString::number(pNetAdapter->getIndex() + 1));
						m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
						ADD_FID((E_SET << qsAA_id << qsCWD_id << pNetAdapter->getNetAddresses_id()) + setIds);
					}

					if ( ! qsGatewayIp.isEmpty()
						&& ! bGatewayInSubnet
						&& (QHostAddress(qsGatewayIp).toIPv4Address() & uiMask) == (uiIP & uiMask) )
					{
						bGatewayInSubnet = true;
					}

					QHostAddress ipAddress(ip);

					if (lstIPAddresses.contains(ipAddress))
					{
						m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_IP_ADDRESS;
						m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
						ADD_FID((E_SET << qsAA_id << qsCWD_id << pNetAdapter->getNetAddresses_id()) + setIds);
					} else {
						lstIPAddresses += ipAddress;
					}

				}

				if (pNetAdapter->getEmulatedType() != PNA_ROUTED
					&& ! qsGatewayIp.isEmpty()
					&& ! bGatewayInSubnet
					&& ! adapterIps.isEmpty() )
				{
					m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_GATEWAY_NOT_IN_SUBNET;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< qsGatewayIp << QString::number(pNetAdapter->getIndex() + 1));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
					ADD_FID((E_SET << qsAA_id << qsCWD_id << pNetAdapter->getNetAddresses_id()
						<< pNetAdapter->getDefaultGateway_id()) + setIds);
				}
			}

			// Validate DNS IP Addresses
			QList<QString > lstDnsIPs = pNetAdapter->getDnsIPAddresses();
			foreach (QString ip_mask, lstDnsIPs)
			{
				QString ip;

				if ( ! NetworkUtils::ParseIpMask(ip_mask, ip) )
				{
					m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_DNS_IP_ADDRESS;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< ip_mask << QString::number(pNetAdapter->getIndex() + 1));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
					ADD_FID((E_SET << qsAA_id << pNetAdapter->getDnsIPAddresses_id()) + setIds);
				}
			}

			// Validate Search Domains
			QRegExp re(     "(([a-z]|[A-Z])(([a-z]|[A-Z]|\\d|\\-)*([a-z]|[A-Z]|\\d))*)*"
						"(\\.(([a-z]|[A-Z])(([a-z]|[A-Z]|\\d|\\-)*([a-z]|[A-Z]|\\d))*)+)*");
			QList<QString > lstSearchDomains = pNetAdapter->getSearchDomains();
			foreach (QString qsDomain, lstSearchDomains)
			{
				if ( ! re.exactMatch(qsDomain) )
				{
					m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_SEARCH_DOMAIN_NAME;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< qsDomain<< QString::number(pNetAdapter->getIndex() + 1));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(), pNetAdapter->getItemId()));
					ADD_FID((E_SET << qsAA_id << pNetAdapter->getSearchDomains_id()) + setIds);
				}
			}
		}

		if (pNetAdapter->getEmulatedType() == PNA_ROUTED)
		{
			bool dhcp = pNetAdapter->isConfigureWithDhcp() || pNetAdapter->isConfigureWithDhcpIPv6();
			CVmGenericNetworkAdapter* old = NULL;
			if (m_pVmConfigOld.isValid())
				old = CXmlModelHelper::IsElemInList(pNetAdapter,
							m_pVmConfigOld->getVmHardwareList()->m_lstNetworkAdapters);
			if (dhcp || (pNetAdapter->getNetAddresses().empty() && (NULL == old || old->getNetAddresses().empty())))
			{
				m_lstResults += PRL_ERR_VMCONF_NETWORK_ADAPTER_ROUTED_NO_STATIC_ADDRESS;
				m_mapParameters.insert(m_lstResults.size(), QStringList()
							<< QString::number(pNetAdapter->getIndex()+1));
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pNetAdapter->getIndex(),
							pNetAdapter->getItemId()));
				ADD_FID(E_SET << pNetAdapter->getEmulatedType_id() << pNetAdapter->getNetAddresses_id());
			}
		}
	}
	CheckIPDuplicates(setNA_ids);
}

void CVmValidateConfig::CheckIPDuplicates(const QSet<QString>& setNA_ids_)
{
	QList< SmartPtr<CVmConfiguration> > allVEs;
	CDspService::instance()->getVzHelper()->getCtConfigList(m_pClient, 0, allVEs);
	allVEs.append(CDspService::instance()->getVmDirHelper().getAllVmList().values());

	QSet<QHostAddress> duplicates = Task_ManagePrlNetService::checkIPAddressDuplicates(m_pVmConfig, allVEs);
	if (!duplicates.isEmpty())
	{
		QString ips;
		foreach(QHostAddress ip, duplicates)
		{
			ips += ip.toString();
			ips += " ";
		}
		m_lstResults += PRL_ERR_VMCONF_DUPLICATE_IP_ADDRESS;
		m_mapParameters.insert(m_lstResults.size(), QStringList() << ips);
		ADD_FID(setNA_ids_);
	}
}

void CVmValidateConfig::CheckSound()
{
	if (!m_pVmConfig)
	{
		return;
	}

	QList<CVmSoundDevice* > x = m_pVmConfig->getVmHardwareList()->m_lstSoundDevices;
	if (!x.isEmpty())
	{
		m_lstResults += PRL_ERR_UNSUPPORTED_DEVICE_TYPE;
	}
}

void CVmValidateConfig::CheckSerialPort()
{
	if (!m_pVmConfig)
	{
		return;
	}

	QList<CVmSerialPort* > lstSerialPorts = m_pVmConfig->getVmHardwareList()->m_lstSerialPorts;
	for(int i = 0; i < lstSerialPorts.size(); i++)
	{
		CVmSerialPort* pSerialPort = lstSerialPorts[i];
		if (   !pSerialPort
			|| pSerialPort->getEnabled() != PVE::DeviceEnabled
			|| pSerialPort->getConnected() != PVE::DeviceConnected)
			continue;

		QSet<QString > setIds = E_SET << pSerialPort->getFullItemId()
			<< pSerialPort->getEnabled_id() << pSerialPort->getConnected_id()
			<< pSerialPort->getSystemName_id();

		QString qsSysName = pSerialPort->getSystemName();
		if( qsSysName.isEmpty() )
		{
			m_lstResults += PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_IS_EMPTY;
			m_mapParameters.insert(m_lstResults.size(),
				QStringList()<<QString::number(pSerialPort->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pSerialPort->getIndex(), pSerialPort->getItemId()));
			ADD_FID(setIds);
			continue;
		}

		setIds << pSerialPort->getEmulatedType_id();

		if (   IsUrlFormatSysName(qsSysName)
				 && pSerialPort->getEmulatedType() == PVE::SerialOutputFile)
		{
			m_lstResults += PRL_ERR_VMCONF_SERIAL_PORT_URL_FORMAT_SYS_NAME;
			m_mapParameters.insert(m_lstResults.size(), QStringList()
				<< qsSysName << QString::number(pSerialPort->getIndex() + 1));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pSerialPort->getIndex(), pSerialPort->getItemId()));
			ADD_FID(setIds);
		}
		else if (   HasSysNameInvalidSymbol(qsSysName)
				 && pSerialPort->getEmulatedType() == PVE::SerialOutputFile)
		{
			m_lstResults += PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_HAS_INVALID_SYMBOL;
			m_mapParameters.insert(m_lstResults.size(), QStringList() << GetFileNameFromInvalidPath(qsSysName));
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pSerialPort->getIndex(), pSerialPort->getItemId()));
			ADD_FID(setIds);
		}
		else if ( pSerialPort->getEmulatedType() == PVE::SerialOutputFile
				&& !QFile::exists(qsSysName)
				&& !pSerialPort->isRemote()
				&& !CFileHelper::isRemotePath(qsSysName))
		{
			m_lstResults += PRL_ERR_VMCONF_SERIAL_PORT_IMAGE_IS_NOT_EXIST;
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pSerialPort->getIndex(), pSerialPort->getItemId()));
			ADD_FID((E_SET << pSerialPort->getRemote_id()) + setIds);
		}
	}
}

void CVmValidateConfig::CheckParallelPort()
{
	if (!m_pVmConfig)
	{
		return;
	}

	QList<CVmParallelPort* > lstParallelPorts = m_pVmConfig->getVmHardwareList()->m_lstParallelPorts;
	if (!lstParallelPorts.isEmpty())
		m_lstResults += PRL_ERR_UNSUPPORTED_DEVICE_TYPE;

}

void CVmValidateConfig::CheckMassStorageDevices(PRL_MASS_STORAGE_INTERFACE_TYPE type)
{
	if (!m_pVmConfig)
	{
		return;
	}


	int i = -1;
	int nCountDevices = 0;
	QList<unsigned int > lstStackIndexes;
	QSet<QString > set_ids;
	PRL_RESULT ErrorDup = PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX;

	if (type == PMS_SCSI_DEVICE)
	{
		// Technically device with index 7 is present - host controller
		nCountDevices = 1;
		lstStackIndexes += SCSI_HOST_INDEX;
		ErrorDup = PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX;
	}
	else if (type == PMS_SATA_DEVICE)
		ErrorDup = PRL_ERR_VMCONF_SATA_DEVICES_DUPLICATE_STACK_INDEX;
	else if (type == PMS_VIRTIO_BLOCK_DEVICE)
		ErrorDup = PRL_ERR_VMCONF_VIRTIO_BLOCK_DEVICES_DUPLICATE_STACK_INDEX;

	QList<CVmHardDisk* > lstHardDisks = m_pVmConfig->getVmHardwareList()->m_lstHardDisks;
	for(i = 0; i < lstHardDisks.size(); i++)
	{
		CVmHardDisk* pHardDisk = lstHardDisks[i];
		if (pHardDisk && pHardDisk->getEnabled() == PVE::DeviceEnabled
			&& pHardDisk->getConnected() == PVE::DeviceConnected
			&& pHardDisk->getInterfaceType() == type)
		{
			set_ids << pHardDisk->getFullItemId();

			nCountDevices++;
			unsigned int nStackIndex = pHardDisk->getStackIndex();
			if (lstStackIndexes.contains(nStackIndex))
			{
				m_lstResults += ErrorDup;
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pHardDisk->getIndex(), pHardDisk->getItemId()));
				ADD_FID(E_SET << pHardDisk->getFullItemId() << pHardDisk->getEnabled_id()
					<< pHardDisk->getInterfaceType_id() << pHardDisk->getStackIndex_id());
			}
			lstStackIndexes += nStackIndex;
		}
	}

	QList<CVmOpticalDisk* > lstOpticalDisks = m_pVmConfig->getVmHardwareList()->m_lstOpticalDisks;
	for(i = 0; i < lstOpticalDisks.size(); i++)
	{
		CVmOpticalDisk* pOpticalDisk = lstOpticalDisks[i];
		if (pOpticalDisk && pOpticalDisk->getEnabled() == PVE::DeviceEnabled
			&& pOpticalDisk->getInterfaceType() == type)
		{
			set_ids << pOpticalDisk->getFullItemId();

			nCountDevices++;
			unsigned int nStackIndex = pOpticalDisk->getStackIndex();
			if (lstStackIndexes.contains(nStackIndex))
			{
				m_lstResults += ErrorDup;
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pOpticalDisk->getIndex(), pOpticalDisk->getItemId()));
				ADD_FID(E_SET << pOpticalDisk->getFullItemId() << pOpticalDisk->getEnabled_id()
					<< pOpticalDisk->getInterfaceType_id() << pOpticalDisk->getStackIndex_id());
			}
			lstStackIndexes += nStackIndex;
		}
	}

	if (type == PMS_SCSI_DEVICE)
	{
		QList<CVmGenericScsiDevice* > lstGenericScsiDevices =
			m_pVmConfig->getVmHardwareList()->m_lstGenericScsiDevices;
		for(i = 0; i < lstGenericScsiDevices.size(); i++)
		{
			CVmGenericScsiDevice* pGenericScsiDevice = lstGenericScsiDevices[i];
			if (!pGenericScsiDevice || pGenericScsiDevice->getEnabled() != PVE::DeviceEnabled
				|| pGenericScsiDevice->getInterfaceType() != type)
				continue;

			set_ids << pGenericScsiDevice->getFullItemId();

			nCountDevices++;
			unsigned int nStackIndex = pGenericScsiDevice->getStackIndex();
			if (lstStackIndexes.contains(nStackIndex))
			{
				m_lstResults += ErrorDup;
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pGenericScsiDevice->getIndex(),
							pGenericScsiDevice->getItemId()));
				ADD_FID(E_SET << pGenericScsiDevice->getFullItemId() << pGenericScsiDevice->getEnabled_id()
						<< pGenericScsiDevice->getInterfaceType_id()
						<< pGenericScsiDevice->getStackIndex_id());
			}
			lstStackIndexes += nStackIndex;
		}
	}

	bool bErrorDetected = false;
	if (type == PMS_IDE_DEVICE && nCountDevices > PRL_MAX_IDE_DEVICES_NUM)
	{
		m_lstResults += PRL_ERR_VMCONF_IDE_DEVICES_COUNT_OUT_OF_RANGE;
		m_mapParameters.insert(m_lstResults.size(),
			QStringList(QString::number(PRL_MAX_IDE_DEVICES_NUM)));
		bErrorDetected = true;
	}
	else if (type == PMS_SATA_DEVICE && nCountDevices > PRL_MAX_SATA_DEVICES_NUM)
	{
		m_lstResults += PRL_ERR_VMCONF_SATA_DEVICES_COUNT_OUT_OF_RANGE;
		m_mapParameters.insert(m_lstResults.size(),
				QStringList(QString::number(PRL_MAX_SATA_DEVICES_NUM)));
		bErrorDetected = true;
	}
	else if (type == PMS_SCSI_DEVICE && nCountDevices > PRL_MAX_SCSI_DEVICES_NUM)
	{
		m_lstResults += PRL_ERR_VMCONF_SCSI_DEVICES_COUNT_OUT_OF_RANGE;
		m_mapParameters.insert(m_lstResults.size(),
				QStringList(QString::number(PRL_MAX_SCSI_DEVICES_NUM)));
		bErrorDetected = true;
	}
	if (bErrorDetected)
	{
		if (m_pVmConfigOld)
		{
			foreach(CVmHardDisk* pHardDisk, m_pVmConfigOld->getVmHardwareList()->m_lstHardDisks)
				if (pHardDisk && ! set_ids.contains(pHardDisk->getFullItemId()))
					set_ids << pHardDisk->getFullItemId();
			foreach(CVmOpticalDisk* pOpticalDisk, m_pVmConfigOld->getVmHardwareList()->m_lstOpticalDisks)
				if (pOpticalDisk && ! set_ids.contains(pOpticalDisk->getFullItemId()))
					set_ids << pOpticalDisk->getFullItemId();
			if (type == PMS_SCSI_DEVICE)
			{
				foreach(CVmGenericScsiDevice* pScsiDev,
						m_pVmConfigOld->getVmHardwareList()->m_lstGenericScsiDevices)
					if (pScsiDev && ! set_ids.contains(pScsiDev->getFullItemId()))
						set_ids << pScsiDev->getFullItemId();
			}
		}
		ADD_FID(set_ids);
	}
}

void CVmValidateConfig::CheckIdeDevices()
{
	CheckMassStorageDevices(PMS_IDE_DEVICE);
}

void CVmValidateConfig::CheckSataDevices()
{
	CheckMassStorageDevices(PMS_SATA_DEVICE);
}

void CVmValidateConfig::CheckScsiDevices()
{
	CheckMassStorageDevices(PMS_SCSI_DEVICE);
}

void CVmValidateConfig::CheckVirtioBlockDevices()
{
	CheckMassStorageDevices(PMS_VIRTIO_BLOCK_DEVICE);
}

namespace {
/**
 * Simple helper that returns merged general PCI device state (actual host hardware
 * info state with state specified at dispatcher common preferences)
 */
PRL_GENERIC_DEVICE_STATE GetMergedPciDevState( CHwGenericPciDevice* pDev )
{
	if ( pDev->getDeviceState() == PGS_CONNECTED_TO_VM )
		return ( PGS_CONNECTED_TO_VM );

	CDspLockedPointer<CDispCommonPreferences> pCommonPrefs =
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
	foreach( CDispGenericPciDevice *pDispPciDev, pCommonPrefs->getPciPreferences()->getGenericPciDevices()->m_lstGenericPciDevices )
	{
		if ( pDispPciDev->getDeviceId() == pDev->getDeviceId() )
		{
			break;
		}
	}

	return ( pDev->getDeviceState() );
}

}

QString CVmValidateConfig::GetPCIDeviceSysNameMainPart(const QString& qsDeviceId)
{
	// We use for compare PCI address only first 3 digit = BUS:DEV:FUN
	QStringList lstDev = qsDeviceId.split(":");
	if (lstDev.size() < 3)
		return qsDeviceId;

	return lstDev[0] + ":" + lstDev[1] + ":" + lstDev[2];
}

void CVmValidateConfig::CheckGenericPci()
{
	if ( ! m_pVmConfig || CDspService::isServerModePSBM() )
	{
		return;
	}

	QMap<QString , PRL_GENERIC_DEVICE_STATE > mapDevStates;
	QMap<QString , QString > mapFriendlyNames;
	QMap<QString , PRL_GENERIC_PCI_DEVICE_CLASS > mapPciClasses;
	bool bVtdSupported = false;
	{
		CDspLockedPointer<CDspHostInfo> pDspHostInfo = CDspService::instance()->getHostInfo();
		if ( ! pDspHostInfo.isValid() )
			return;

		CHostHardwareInfo* pHostInfo = pDspHostInfo->data();
		if ( ! pHostInfo )
			return;

		bVtdSupported = pHostInfo->isVtdSupported();

		foreach(CHwGenericPciDevice* pDev, pHostInfo->getGenericPciDevices()->m_lstGenericPciDevice)
		{
			QString qsSysName = GetPCIDeviceSysNameMainPart(pDev->getDeviceId());

			mapDevStates.insert(qsSysName, GetMergedPciDevState(pDev));
			mapFriendlyNames.insert(qsSysName, pDev->getDeviceName());
			mapPciClasses.insert(qsSysName, pDev->getType());
		}
	}

	QSet<QString > setSysNames;
	int	nPciVideoCount = 0;

	CVmHardware* pHwList = m_pVmConfig->getVmHardwareList();

	QList<CVmDevice*> lstPciDevices = *((QList<CVmDevice*>* )&pHwList->m_lstGenericPciDevices);
	lstPciDevices += *((QList<CVmDevice*>* )&pHwList->m_lstNetworkAdapters);
	lstPciDevices += *((QList<CVmDevice*>* )&pHwList->m_lstPciVideoAdapters);

	for(int i = 0; i < lstPciDevices.size(); i++)
	{
		CVmDevice* pPciDev = lstPciDevices[i];
		if ( pPciDev->getEnabled() != PVE::DeviceEnabled )
			continue;

		QSet<QString > setIds = E_SET << pPciDev->getFullItemId() << pPciDev->getEnabled_id();

		if (pPciDev->getDeviceType() == PDE_GENERIC_NETWORK_ADAPTER)
		{
			if (pPciDev->getEmulatedType() != PDT_USE_DIRECT_ASSIGN)
				continue;
			setIds << pPciDev->getEmulatedType_id();
		}

		if ( ! bVtdSupported )
		{
			m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_CANNOT_BE_ADDED;
			ADD_FID(setIds);
			return;
		}
		else
		{
			if (pPciDev->getDeviceType() == PDE_PCI_VIDEO_ADAPTER)
			{
				nPciVideoCount++;
				if (nPciVideoCount > 2)
				{
					m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_VIDEO_WRONG_COUNT;
					ADD_FID(setIds);
					return;
				}
			}

			setIds << pPciDev->getSystemName_id();

			QString qsSysName = GetPCIDeviceSysNameMainPart(pPciDev->getSystemName());
			if (mapDevStates.contains(qsSysName))
			{
				if (mapDevStates.value(qsSysName) != PGS_CONNECTED_TO_VM)
				{
					m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_NOT_CONNECTED;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< qsSysName << mapFriendlyNames.value(qsSysName));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pPciDev->getIndex(), pPciDev->getItemId()));
					m_mapPciDevClasses.insert(m_lstResults.size(), mapPciClasses.value(qsSysName));
					ADD_FID(setIds);
				}

				bool bWrongDevice = false;
				switch(pPciDev->getDeviceType())
				{
				case PDE_GENERIC_NETWORK_ADAPTER:
					if (mapPciClasses.value(qsSysName) != PGD_PCI_NETWORK)
						bWrongDevice = true;
					break;
				case PDE_PCI_VIDEO_ADAPTER:
					if (mapPciClasses.value(qsSysName) != PGD_PCI_DISPLAY)
						bWrongDevice = true;
					break;
				case PDE_GENERIC_PCI_DEVICE:
					if (mapPciClasses.value(qsSysName) != PGD_PCI_SOUND
						&& mapPciClasses.value(qsSysName) != PGD_PCI_OTHER)
						bWrongDevice = true;
					break;
				default:
					;
				}

				if ( bWrongDevice )
				{
					m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_WRONG_DEVICE;
					m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< qsSysName << mapFriendlyNames.value(qsSysName));
					m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pPciDev->getIndex(), pPciDev->getItemId()));
					m_mapPciDevClasses.insert(m_lstResults.size(), mapPciClasses.value(qsSysName));
					ADD_FID(setIds);
				}
			}
			else
			{
				QMap<PRL_DEVICE_TYPE, PRL_GENERIC_PCI_DEVICE_CLASS> mapCfgTypeToHwType;
				mapCfgTypeToHwType.insert( PDE_GENERIC_NETWORK_ADAPTER, PGD_PCI_NETWORK );
				mapCfgTypeToHwType.insert( PDE_PCI_VIDEO_ADAPTER, PGD_PCI_DISPLAY );
				mapCfgTypeToHwType.insert( PDE_GENERIC_PCI_DEVICE, PGD_PCI_OTHER );

				m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_NOT_FOUND;
				m_mapParameters.insert(m_lstResults.size(), QStringList() << pPciDev->getUserFriendlyName());
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pPciDev->getIndex(), pPciDev->getItemId()));
				m_mapPciDevClasses.insert(m_lstResults.size(), mapCfgTypeToHwType.value(pPciDev->getDeviceType()));
				ADD_FID(setIds);
			}

			if (setSysNames.contains(qsSysName))
			{
				m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_DUPLICATE_SYS_NAME;
				m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< qsSysName << mapFriendlyNames.value(qsSysName));
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pPciDev->getIndex(), pPciDev->getItemId()));
				m_mapPciDevClasses.insert(m_lstResults.size(), mapPciClasses.value(qsSysName));
				ADD_FID(setIds);
			}
			else
			{
				setSysNames.insert(qsSysName);
			}

			bool bHasRunningAnotherVm = false;
			if ( IsDeviceInAnotherVm(pPciDev->getDeviceType(),
									 qsSysName,
									 m_pVmConfig->getVmIdentification()->getVmUuid(),
									 bHasRunningAnotherVm) )
			{
				// Note: Only for existing PCI device bHasRunningAnotherVm may be true
				bHasRunningAnotherVm = mapDevStates.contains(qsSysName) ? bHasRunningAnotherVm : false;

				m_lstResults += PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM;
				m_mapParameters.insert(m_lstResults.size(), QStringList()
						<< qsSysName << mapFriendlyNames.value(qsSysName)
						<< QString::number(bHasRunningAnotherVm));
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pPciDev->getIndex(), pPciDev->getItemId()));
				m_mapPciDevClasses.insert(m_lstResults.size(), mapPciClasses.value(qsSysName));
				ADD_FID(setIds);
			}
		}
	}
}

void CVmValidateConfig::CheckNetworkShapingRates()
{
#ifdef _LIN_
	if (CVzHelper::is_vz_running() != 1)
		return;
	if (!m_pVmConfig || m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return;
	CDspLockedPointer<CParallelsNetworkConfig>
		pNetCfg = CDspService::instance()->getNetworkConfig();
	CNetworkShapingConfig *pCfg = pNetCfg->getNetworkShapingConfig();
	foreach(CVmNetworkRate *rate, m_pVmConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates()->m_lstNetworkRates)
	{
		bool bFound = false;
		foreach(CNetworkShaping *entry, pCfg->m_lstNetworkShaping)
		{
			if (entry->getClassId() == rate->getClassId())
			{
				bFound = true;
				break;
			}
		}
		if (!bFound) {
			m_lstResults += PRL_ERR_VMCONF_NO_CONFIGURED_TOTALRATE_FOR_NETWORK_CLASS;
			m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(rate->getClassId(), rate->getItemId()) );
			ADD_FID(E_SET << rate->getFullItemId() << rate->getClassId_id());
		}
	}
#endif
}

void CVmValidateConfig::CommonDevicesCheck( PRL_VM_CONFIG_SECTIONS nSection )
{
	if (!m_pVmConfig)
		return;

	// calculate type
	QSet< PRL_DEVICE_TYPE > typesSet;
	switch( nSection )
	{
	case PVC_FLOPPY_DISK:
		typesSet.insert( PDE_FLOPPY_DISK ); break;
	case PVC_CD_DVD_ROM:
		typesSet.insert(  PDE_OPTICAL_DISK ); break;
	case PVC_HARD_DISK:
		typesSet.insert(  PDE_HARD_DISK ); break;
	case PVC_NETWORK_ADAPTER:
		typesSet.insert(  PDE_GENERIC_NETWORK_ADAPTER ); break;
	case PVC_SOUND:
		typesSet.insert(  PDE_SOUND_DEVICE ); break;
	case PVC_SERIAL_PORT:
		typesSet.insert(  PDE_SERIAL_PORT ); break;
	case PVC_PARALLEL_PORT:
		typesSet.insert(  PDE_PARALLEL_PORT ); break;
	case PVC_IDE_DEVICES:
	case PVC_SCSI_DEVICES:
	case PVC_SATA_DEVICES:
		typesSet.insert(  PDE_HARD_DISK ); break;
		typesSet.insert(  PDE_OPTICAL_DISK ); break;
		break;
	case PVC_ALL:
		typesSet.insert( PDE_MAX );
		break;
	default:
		return;
	}//switch

	CheckDeviceIndexes( typesSet, nSection );
}

void CVmValidateConfig::CheckDeviceIndexes( QSet< PRL_DEVICE_TYPE > deviceTypes, PRL_VM_CONFIG_SECTIONS nSection )
{
	if (!m_pVmConfig)
	{
		return;
	}

	for( uint type=0; type < PDE_MAX; type++  )
	{
		if( ! deviceTypes.contains( PDE_MAX ) && ! deviceTypes.contains( (PRL_DEVICE_TYPE)type ) )
			continue;

		QList<CVmDevice*>* p_lstDevices = reinterpret_cast< QList<CVmDevice*>* >
			( m_pVmConfig->getVmHardwareList()->m_aDeviceLists[ type ] );

		if( ! p_lstDevices )
			continue;

		QSet< uint > indexSet;
		for( int i=0; i< p_lstDevices->size(); i++ )
		{
			CVmDevice* pDev = p_lstDevices->at( i );
			PRL_ASSERT( pDev );
			if( !pDev )
				continue;

			if (pDev->getIndex() > PRL_MAX_DEVICE_INDEX)
			{
				WRITE_TRACE(DBG_FATAL, "Error: Device (type %d) index is big (> %d)", type, PRL_MAX_DEVICE_INDEX);
				m_lstResults += PRL_ERR_VMCONF_INVALID_DEVICE_MAIN_INDEX;
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pDev->getIndex(), pDev->getItemId()));
				continue;
			}

			if( CVmMassStorageDevice* pMassDev = dynamic_cast< CVmMassStorageDevice* >( pDev ) )
			{
				if (pMassDev->getInterfaceType() == PMS_IDE_DEVICE &&
					(PVC_SCSI_DEVICES == nSection || PVC_SATA_DEVICES == nSection))
					continue;
				if (pMassDev->getInterfaceType() == PMS_SCSI_DEVICE &&
					(PVC_IDE_DEVICES == nSection || PVC_SATA_DEVICES == nSection))
					continue;
				if (pMassDev->getInterfaceType() == PMS_SATA_DEVICE &&
					(PVC_IDE_DEVICES == nSection || PVC_SCSI_DEVICES == nSection))
					continue;
			}

			if( ! indexSet.contains( pDev->getIndex() ) )
			{
				indexSet.insert( pDev->getIndex() );
			}
			else
			{
				WRITE_TRACE(DBG_FATAL,
					"INVALID_DEVICE_MAIN_INDEX: Duplicated index of device %d:%d, curr_type %d",
					(int)pDev->getDeviceType(), (int)pDev->getIndex(), (int)type);
				m_lstResults += PRL_ERR_VMCONF_INVALID_DEVICE_MAIN_INDEX;
				m_mapDevInfo.insert(m_lstResults.size(), DeviceInfo(pDev->getIndex(), pDev->getItemId()));
				ADD_FID(E_SET << pDev->getFullItemId() << pDev->getIndex_id());
			}
		}//for i

	}//for type
}

// VirtualDisk commented out by request from CP team
//QString CVmValidateConfig::GetSnapshotNamesForDisk(
//		const SNAPTREE_ELEMENT& sntElem,
//		CSavedStateTree *savedStateTree,
//		const Uuid current)
//{
//	QString s;
//
//	CSavedStateTree *state =
//			savedStateTree->FindByUuid(sntElem.m_Uid.toString());
//
//	if (!sntElem.m_Uid.isNull()) {
//		if (!state || state->GetName().isEmpty())
//			s += sntElem.m_Uid.toString();
//		else
//			s += state->GetName();
//	}
//
//	std::list<SNAPTREE_ELEMENT>::const_iterator i = (sntElem.m_Children).begin();
//	while (i != (sntElem.m_Children).end()) {
//		const SNAPTREE_ELEMENT &child = *i;
//		i++;
//		/* current temporary state is a last element in the list of children of
//		 * 'current snapshot', we shouldn't show it */
//		if (!(i == (sntElem.m_Children).end() && sntElem.m_Uid == current)) {
//			if (!s.isEmpty())
//				s += ", ";
//			s += GetSnapshotNamesForDisk(child, savedStateTree, current);
//		}
//	}
//
//	return s;
//}

void CVmValidateConfig::CheckSnapshotsOfDisk(const CVmHardDisk &/*hardDisk*/)
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT ret = PRL_ERR_SUCCESS;
//	Uuid dirUuid = m_pClient->getVmDirectoryUuid();
//	Uuid vmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();
//	CSavedStateStore savedStateStore;
//	CSavedStateTree *pSavedStateTree;
//
//	if (!CDspVmSnapshotStoreHelper().getSnapshotsTree(
//				CVmIdent(vmUuid, dirUuid), &savedStateStore))
//		return;
//
//	pSavedStateTree = savedStateStore.GetSavedStateTree();
//
//	IDisk* pDisk = IDisk::OpenDisk(hardDisk.getSystemName(),
//			PRL_DISK_NO_ERROR_CHECKING | PRL_DISK_READ | PRL_DISK_FAKE_OPEN,
//			&ret);
//	if (PRL_FAILED(ret))
//		return;
//	if ( !pDisk)
//		return;
//	if ( pDisk->GetSnapshotsCount() < 2 )
//	{
//		pDisk->Release();
//		return;
//	}
//
//	SNAPTREE_ELEMENT sntElem;
//	ret = pDisk->GetSnapshotsTree(&sntElem);
//	Uuid current = pDisk->GetCurrentUID();
//
//	pDisk->Release();
//	pDisk = NULL;
//	if ( PRL_FAILED(ret) )
//		return;
//
//	m_lstResults += PRL_ERR_VMCONF_HARD_DISK_UNABLE_DELETE_DISK_WITH_SNAPSHOTS;
//
//	QStringList lstParams;
//	lstParams << hardDisk.getUserFriendlyName();
//	lstParams << GetSnapshotNamesForDisk(sntElem, pSavedStateTree, current);
//	m_mapParameters.insert(m_lstResults.size(), lstParams);
//
//	ADD_FID(E_SET << hardDisk.getFullItemId());
}
