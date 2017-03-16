///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackgroundJob.cpp
///
/// Dispatcher task for send question and wait answer from another thread
///
/// @author sergeyt
/// @owner sergeym
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
/////////////////////////////////////////////////////////////////////////////////

// #define FORCE_LOGGING_ON
// #define FORCE_LOGGING_LEVEL		DBG_DEBUG

#include <prlcommon/Logging/Logging.h>


#include "Task_BackgroundJob.h"
#include "Task_CommonHeaders.h"
#include "Task_ManagePrlNetService.h"
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>
#include "CDspVmManager.h"
#include "CDspTestConfig.h"

#include <QStringList>
#include <QHostAddress>
#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirsDefs.h>

#include "CDspVzHelper.h"


inline void doTraceError(PRL_RESULT _code)
{
	WRITE_TRACE(DBG_FATAL, "Error occurred while executing foreground job with code [%#x][%s]"
		, _code
		, PRL_RESULT_TO_STRING( _code ) );
}

using namespace Parallels;

Task_BackgroundJob::Task_BackgroundJob(
	const SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage> &pRequestPkg)
: CDspTaskHelper(pUser, pRequestPkg)
{}

PRL_RESULT Task_BackgroundJob::run_body()
{
	PRL_RESULT output = ConcreteDoBackgroundJob();
	setLastErrorCode(output);
	return output;
}

PRL_RESULT Task_BackgroundJob::ConcreteDoBackgroundJob()
{
	return (PRL_ERR_UNIMPLEMENTED);
}

void Task_BackgroundJob::finalizeTask()
{
	PRL_RESULT c = getLastErrorCode();
	if (PRL_FAILED(c))
	{
		doTraceError(c);
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
}

Task_StartVncServer::Task_StartVncServer(SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage> &pRequestPkg,
	const SmartPtr<CDspVm> &pVm,
	bool bIsRequestFromVm
)
:
Task_BackgroundJob(pUser, pRequestPkg),
m_pVm(pVm),
m_bIsRequestFromVm(bIsRequestFromVm)
{}

PRL_RESULT Task_StartVncServer::ConcreteDoBackgroundJob()
{
	m_pVm->startVNCServer(getClient(), getRequestPackage(), m_bIsRequestFromVm, false);
	return (PRL_ERR_SUCCESS);
}

Task_StopVncServer::Task_StopVncServer(SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage> &pRequestPkg,
	const SmartPtr<CDspVm> &pVm,
	bool bIsRequestFromVm
)
:
Task_StartVncServer(pUser, pRequestPkg, pVm, bIsRequestFromVm)
{}

PRL_RESULT Task_StopVncServer::ConcreteDoBackgroundJob()
{
	m_pVm->stopVNCServer(getClient(), getRequestPackage(), m_bIsRequestFromVm, false);
	return (PRL_ERR_SUCCESS);
}

void Task_StopVncServer::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	m_pVm->cancelStopVNCServerOp();
	CancelOperationSupport::cancelOperation(pUser, p);
}

Task_AuthUserWithGuestSecurityDb::Task_AuthUserWithGuestSecurityDb( const SmartPtr<CDspClient> &pUser,
    const SmartPtr<IOPackage> &pRequestPkg,
	const CProtoCommandPtr &pCmd
	)
:
Task_BackgroundJob(pUser, pRequestPkg),
m_pCmd(pCmd)
{}

namespace {
/**
 * Searches correspond virtual hard disk item for specified boot device
 * @param pointer to the VM configuration object
 * @param pointer to the boot device object
 * @return pointer to the found virtual hard disk obejct (or NULL otherwise)
 */
CVmHardDisk *GetMatchHardDisk(SmartPtr<CVmConfiguration> pVmConfig, CVmStartupOptions::CVmBootDevice *pBootDevice)
{
	QList<CVmHardDisk *> lstHardDisks =
		pVmConfig->getVmHardwareList()->m_lstHardDisks;
	foreach(CVmHardDisk *pHardDisk, lstHardDisks)
	{
		if (pHardDisk->getIndex() == pBootDevice->getIndex())
			return (pHardDisk);
	}
	return (NULL);
}
/**
 * Less than operator for boot order items
 */
bool BootItemsLessThen(CVmStartupOptions::CVmBootDevice *pBootItem1, CVmStartupOptions::CVmBootDevice *pBootItem2)
{
	return (pBootItem1->getBootingNumber() < pBootItem2->getBootingNumber());
}
/**
 * Simple helper that creates list contains paths to VM hard disks images
 * @param pointer to VM configuration object
 */
static QStringList getDiskNames(const SmartPtr<CVmConfiguration>& vm_)
{
	QStringList lstResult;

	QList<CVmStartupOptions::CVmBootDevice *> lstBootDevices =
		vm_->getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList;
	qSort(lstBootDevices.begin(), lstBootDevices.end(), BootItemsLessThen);
	foreach(CVmStartupOptions::CVmBootDevice *pBootDevice, lstBootDevices)
	{
		if (PDE_HARD_DISK != pBootDevice->getType())
			continue;
		CVmHardDisk *pHardDisk = GetMatchHardDisk(vm_, pBootDevice);
		if (!pHardDisk)
		{
			WRITE_TRACE(DBG_FATAL, "Inconsistent configuration: couldn't to find correspond hard disk with index %d",\
				pBootDevice->getIndex());
			continue;
		}
		if (PVE::HardDiskImage == pHardDisk->getEmulatedType() ||
			pHardDisk->getEmulatedType() == PVE::RealHardDisk)
			lstResult += pHardDisk->getSystemName();
	}

	foreach(CVmHardDisk *pHardDisk, vm_->getVmHardwareList()->m_lstHardDisks)
	{
		if ((PVE::HardDiskImage == pHardDisk->getEmulatedType() ||
			pHardDisk->getEmulatedType() == PVE::RealHardDisk) &&
			!lstResult.contains(pHardDisk->getSystemName()))
			lstResult += pHardDisk->getSystemName();
	}

	return (lstResult);
}

}

#define PRL_VM_AUTH_START_TIMEOUT 10000
#define PRL_VM_AUTH_WORK_TIMEOUT  15*60*1000

PRL_RESULT Task_AuthUserWithGuestSecurityDb::ConcreteDoBackgroundJob()
{
	try
	{
		CProtoVmLoginInGuestCommand	*pAuthCmd =
			CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>(m_pCmd);
		PRL_ASSERT(pAuthCmd);

		QString sVmUuid = pAuthCmd->GetVmUuid();

		PRL_RESULT err = PRL_ERR_FIXME;
		SmartPtr<CVmConfiguration> pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
			getClient()
			, sVmUuid
			, err
			, getLastError()
			);
		if( !pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED(err) );
			if( !PRL_FAILED(err) )
				err = PRL_ERR_FAILURE;
			throw err;
		}

		QStringList lstHdds = getDiskNames(pVmConfig);
		if (lstHdds.isEmpty())
		{
			WRITE_TRACE(DBG_FATAL, "VM '%s' '%s' doesn't contain any disk images that can be processed",\
				QSTR2UTF8(pVmConfig->getVmIdentification()->getVmName()),\
				QSTR2UTF8(pVmConfig->getVmIdentification()->getVmUuid()));
			throw PRL_ERR_VM_USER_AUTHENTICATION_FAILED;
		}

		QStringList lstArgs;
		foreach(QString sDirPath, lstHdds)
		{
			lstArgs += "-d";
			lstArgs += sDirPath;
		}
		lstArgs += pAuthCmd->GetUserLoginName();

		// Get VM auth utility execution path
		QString strVmAuthExecutableDir = UTF8_2QSTR(getenv(PVS_VM_EXECUTABLE_ENV));
		QString vmAuthProcName = "/usr/sbin/vmauth";

		QProcess _vm_auth_proc;
		_vm_auth_proc.start(vmAuthProcName, lstArgs);
		if (!_vm_auth_proc.waitForStarted(PRL_VM_AUTH_START_TIMEOUT))
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to start VM auth utility. Error: %d", _vm_auth_proc.error());
			throw PRL_ERR_VM_USER_AUTHENTICATION_FAILED;
		}

		QString sPassword = pAuthCmd->GetUserPassword() + '\n';
		_vm_auth_proc.write(sPassword.toUtf8());

		if (!_vm_auth_proc.waitForFinished(PRL_VM_AUTH_WORK_TIMEOUT))
		{
			WRITE_TRACE(DBG_FATAL, "VM auth tool not responding. Terminate it now.");
			_vm_auth_proc.terminate();
			if (!_vm_auth_proc.waitForFinished(PRL_VM_AUTH_WORK_TIMEOUT))
			{
				_vm_auth_proc.kill();
				_vm_auth_proc.waitForFinished(PRL_VM_AUTH_WORK_TIMEOUT);
			}
			throw PRL_ERR_VM_USER_AUTHENTICATION_FAILED;
		}

		if (0 != _vm_auth_proc.exitCode())
		{
			WRITE_TRACE(DBG_FATAL, "VM auth utility failed with code: %d", _vm_auth_proc.exitCode());
			throw PRL_ERR_VM_USER_AUTHENTICATION_FAILED;
		}
	}
	catch (PRL_RESULT code)
	{
		WRITE_TRACE(DBG_FATAL, "failed by error %.8X '%s'", code, PRL_RESULT_TO_STRING(code) );
		return code;
	}

	getClient()->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	return PRL_ERR_SUCCESS;
}



Task_ApplyVMNetworking::Task_ApplyVMNetworking
(
	const SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage> &p,
	const SmartPtr<CVmConfiguration> &pVmConfig,
	bool bPauseMode
)
:
Task_BackgroundJob(pUser, p),
m_pVmConfig(pVmConfig),
m_bPauseMode(bPauseMode)
{
	PRL_ASSERT(m_pVmConfig);
}



#define PRL_CMD_WORK_TIMEOUT  60*1000
namespace {

QString GetAdapterSysName(CParallelsNetworkConfig *pNetworkConfig,
		PrlNet::EthAdaptersList &ethList, const CVmGenericNetworkAdapter &vmAdapter, PRL_RESULT &nRetCode )
{
	PrlNet::EthAdaptersList::Iterator itAdapter;
	nRetCode = PrlNet::GetAdapterForVM(ethList, pNetworkConfig, vmAdapter, itAdapter);
	if ( PRL_FAILED(nRetCode) )
		return (QString());
	return (itAdapter->_systemName);
}

} // anonymous namespace

PRL_RESULT Task_ApplyVMNetworking::ConcreteDoBackgroundJob()
{
	/* In PauseMode the STOPPED VM state emulated */
	if (!m_bPauseMode)
		Task_ManagePrlNetService::updateVmNetworking(m_pVmConfig, true);
	return PRL_ERR_SUCCESS;
}


PRL_RESULT announceMacAddresses(SmartPtr<CVmConfiguration> &pVmConfig)
{
	QString ip("0.0.0.0");

	PrlNet::EthAdaptersList ethList;
	PRL_RESULT nRetCode = PrlNet::makeBindableAdapterList(ethList, true, true);
	if ( PRL_FAILED(nRetCode) )
		return nRetCode;

	foreach(CVmGenericNetworkAdapter *adapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (adapter == NULL)
			continue;
		if (adapter->getEmulatedType() == PNA_ROUTED ||
			adapter->getEmulatedType() == PNA_DIRECT_ASSIGN ||
			adapter->getEmulatedType() == PNA_BRIDGE)
			continue;
		PRL_RESULT nResult;
		QString sAdapterSysName;
		PRL_NET_VIRTUAL_NETWORK_TYPE nNetType;
		{
			CDspLockedPointer<CParallelsNetworkConfig>
				pNetworkConfig = CDspService::instance()->getNetworkConfig();
			nResult = PrlNet::GetNetworkTypeForAdapter(pNetworkConfig.getPtr(), adapter, &nNetType);
			if (PRL_FAILED(nResult))
				continue;
			// annonce for BRIDGED only
			if (PVN_BRIDGED_ETHERNET != nNetType)
				continue;
			sAdapterSysName = GetAdapterSysName(pNetworkConfig.getPtr(), ethList, adapter, nResult);
			if (sAdapterSysName.isEmpty())
			{
				WRITE_TRACE(DBG_FATAL, "Unable to get bridged adapter system name");
				continue;
			}
		}
		PrlNet::updateArp(ip, adapter->getMacAddress(), sAdapterSysName);
	}
	return PRL_ERR_SUCCESS;
}

Task_NetworkShapingManagement::Task_NetworkShapingManagement(
	const SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage> &p,
	const SmartPtr<CVmConfiguration> &pVmConfig)
:
Task_BackgroundJob(pUser, p),
m_pVmConfig(pVmConfig)
{
}

PRL_RESULT Task_NetworkShapingManagement::getVmNetworkRates(const CVmConfiguration &config_, CVmNetworkRates &lstRates)
{
	CDspLockedPointer<CDispCommonPreferences> p = CDspService::instance()
		->getDispConfigGuard().getDispCommonPrefs();
	CNetworkClassesConfig *pClasses = p->getNetworkPreferences()->getNetworkClassesConfig();
	CNetworkShapingConfig *pShpCfg = p->getNetworkPreferences()->getNetworkShapingConfig();

	if (!pShpCfg->isEnabled())
		return PRL_ERR_SUCCESS;

	CVmNetworkRates *lst = config_.getVmSettings()->getGlobalNetwork()->getNetworkRates();

	lstRates.setRateBound(lst->isRateBound());
	if (!lst->m_lstNetworkRates.empty())
	{
		// use Vm settings
		foreach(CVmNetworkRate* pRate, lst->m_lstNetworkRates)
		{
			if (!pClasses->isClassConfigured(pRate->getClassId())) {
				WRITE_TRACE(DBG_FATAL,
					"NetworkClassId %d is not configured, rate entry skipped",
					pRate->getClassId());
				continue;
			}

			CVmNetworkRate *rate = new CVmNetworkRate;
			rate->setClassId(pRate->getClassId());
			rate->setRate(pRate->getRate());
			lstRates.m_lstNetworkRates += rate;
		}
	} else {
		// use global settings
		foreach(CNetworkShaping *entry, pShpCfg->m_lstNetworkShaping)
		{
			if (entry->getRate() == 0)
				continue;
			if (!pClasses->isClassConfigured(entry->getClassId())) {
				WRITE_TRACE(DBG_FATAL,
					"NetworkClassId %d is not configured, rate entry skipped",
					entry->getClassId());
				continue;
			}

			CVmNetworkRate *rate = new CVmNetworkRate;
			rate->setClassId(entry->getClassId());
			rate->setRate(entry->getRate());
			lstRates.m_lstNetworkRates += rate;
		}
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_NetworkShapingManagement::setNetworkRate(const CVmConfiguration &config_)
{
	CVmNetworkRates lstRates;
	getVmNetworkRates(config_, lstRates);

	if (lstRates.m_lstNetworkRates.empty())
		return PRL_ERR_SUCCESS;
	return CVzHelper::set_rate(config_, lstRates);
}

PRL_RESULT Task_NetworkShapingManagement::ConcreteDoBackgroundJob()
{
	return setNetworkRate(*m_pVmConfig);
}

Task_CalcVmSize::Task_CalcVmSize( const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &p )
: Task_BackgroundJob( pUser, p ), m_nVmsHddsSize( 0 )
{}


PRL_RESULT Task_CalcVmSize::getVmSize(
	const SmartPtr<CVmConfiguration> &pVmCfg,
	const SmartPtr<CDspClient> &pUser,
	PRL_UINT64 &nVmSize,
	PRL_UINT32 /*nFlags = 0*/
	)
{
	quint64 uiConeSize = 0;

	QString strDirPath;
	{
		CVmIdent i(CDspVmDirHelper::getVmIdentByVmUuid(pVmCfg->getVmIdentification()->getVmUuid(), pUser));
		CDspLockedPointer<CVmDirectoryItem> pItem(
				CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid(i.second, i.first));
		if (!pItem)
			return PRL_ERR_VM_UUID_NOT_FOUND;
		strDirPath = pItem->getVmHome();
	}
	strDirPath	= CFileHelper::GetFileRoot(strDirPath);

	// Get size of VM directory
	PRL_RESULT ret = CSimpleFileHelper::GetDirSize(strDirPath,&uiConeSize);
	if ( PRL_FAILED(ret) )
		return ret;

	nVmSize = (PRL_UINT64)uiConeSize;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CalcVmSize::ConcreteDoBackgroundJob()
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
	if ( ! cmd->IsValid() )
		return PRL_ERR_FAILURE;

	CProtoDspCmdVmStorageSetValueCommand*
		pCmd = CProtoSerializer::CastToProtoCommand<CProtoDspCmdVmStorageSetValueCommand>(cmd);

	QString sKey = pCmd->GetKey();
	QString sValue = pCmd->GetValue();
	QString sVmUuid = pCmd->GetVmUuid();

	Q_UNUSED(sKey);

	//Please do not remove this code - uses for test purposes
	{
		bool bOk = false;
		PRL_UINT64 nVmsHddsSize = sValue.toULongLong( &bOk );
		if ( bOk && nVmsHddsSize )
		{
			m_nVmsHddsSize = nVmsHddsSize;
			return PRL_ERR_SUCCESS;
		}
	}

	// Get size of directory for clone
	PRL_RESULT rc;
	SmartPtr<CVmConfiguration> pVmCfg =
		CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
		getClient()->getVmDirectoryUuid(), sVmUuid, rc );

	if ( pVmCfg && PRL_SUCCEEDED(rc) )
	{
		rc = Task_CalcVmSize::getVmSize(
			pVmCfg
			, getClient()
			, m_nVmsHddsSize
			, getRequestFlags()
			);
		if ( PRL_FAILED(rc) )
			return rc;
	}
	else
		return rc;

	return PRL_ERR_SUCCESS;
}

void Task_CalcVmSize::finalizeTask()
{
	if ( PRL_FAILED(getLastError()->getEventCode()) )
	{
		Task_BackgroundJob::finalizeTask();
	}
	else
	{
		CProtoCommandPtr pCmd =
			CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );

		CProtoCommandDspWsResponse
			*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
		pResponseCmd->AddStandardParam( QString::number( m_nVmsHddsSize ) );

		getClient()->sendResponse( pCmd, getRequestPackage() );
	}
}


Task_SendSnapshotTree::Task_SendSnapshotTree( const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &p )
:Task_BackgroundJob( pUser, p )
{
}

PRL_RESULT Task_SendSnapshotTree::ConcreteDoBackgroundJob()
{
	return CDspService::instance()->getVmSnapshotStoreHelper()
		.sendSnapshotsTree( getClient(), getRequestPackage() );
}

Task_AutoconnectUsbDevice::Task_AutoconnectUsbDevice(SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& pRequestPkg, const SmartPtr<CDspVm>& pVm
)
:
Task_BackgroundJob(pUser, pRequestPkg),
	m_pVm(pVm)
{}

PRL_RESULT Task_AutoconnectUsbDevice::ConcreteDoBackgroundJob()
{
//	m_pVm->connectDevice( getClient(), getRequestPackage() );
	return (PRL_ERR_SUCCESS);
}

Task_AutoconnectUsbDevice * Task_AutoconnectUsbDevice::createTask( SmartPtr<CDspVm> &pVm,
	QString id, QString frendlyName )
{
	PRL_RESULT nErr;

	PRL_ASSERT(pVm);
	if(!pVm.isValid())
		return 0;

	SmartPtr<CVmConfiguration> pVmConfig = pVm->getVmConfig(pVm->getVmRunner(),nErr);
	if( !pVmConfig.isValid() )
		return 0;

	if ( pVmConfig->getVmHardwareList()->m_lstUsbDevices.isEmpty() )
		return 0; // No Usb in specified VM. Do we need to clear assignement?

	QString devXMLstr = ElementToString<CVmUsbDevice*>(
		pVmConfig->getVmHardwareList()->m_lstUsbDevices[0],
		XML_VM_CONFIG_EL_USB_DEVICE
		);
	CVmUsbDevice usb;
	StringToElement<CVmUsbDevice*>(&usb,XML_VM_CONFIG_EL_USB_DEVICE);
	usb.setEmulatedType( PDT_USE_REAL_DEVICE );
	usb.setSystemName( id );
	usb.setUserFriendlyName( frendlyName );
	usb.setRemote( false );
	devXMLstr = ElementToString<CVmUsbDevice*>(&usb, XML_VM_CONFIG_EL_USB_DEVICE);
	CProtoVmDeviceCommand cmd(PVE::DspCmdVmDevConnect,
		pVm->getVmUuid(),
		PDE_USB_DEVICE,
		0,
		devXMLstr
		);
	SmartPtr<IOPackage> p =	DispatcherPackage::createInstance(
		PVE::DspCmdVmDevConnect, &cmd
		);
	SmartPtr<CDspClient> client(pVm->getVmRunner());
	return new Task_AutoconnectUsbDevice( client, p, pVm );
}

/****************************************************************************/
void Task_RunVmAction::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
        CDspTaskHelper::cancelOperation(pUserSession, p);
}

Task_RunVmAction::Task_RunVmAction(
		SmartPtr<CDspClient>& pUser,
		const SmartPtr<IOPackage>& pRequestPkg,
		const SmartPtr<CDspVm> &pVm,
		PRL_VM_ACTION nAction,
		const QString &sUserName)
:
	Task_BackgroundJob(pUser, pRequestPkg),
	m_pVm(pVm),
	m_nAction(nAction),
	m_sUserName(sUserName)
{
}

PRL_RESULT Task_RunVmAction::RunScript(const QString &sScript, const QStringList &lstEnv)
{
#ifdef _LIN_
	m_proc.setUser(m_sUserName);
#else
	return PRL_ERR_UNIMPLEMENTED;
#endif
	WRITE_TRACE(DBG_INFO, "run action script %s", QSTR2UTF8(sScript));

	m_proc.setEnvironment(lstEnv);
	m_proc.start(sScript);

	if (!m_proc.waitForStarted(-1))
	{
		WRITE_TRACE(DBG_FATAL, "Can not start %s", QSTR2UTF8(sScript));
		return PRL_ERR_OPERATION_FAILED;
	}
	bool bOk = m_proc.waitForFinished(-1);
	if (!bOk)
	{
		WRITE_TRACE(DBG_FATAL, "%s tool not responding err=%d. Terminate it now.",
				QSTR2UTF8(sScript), m_proc.error());
		m_proc.kill();
		m_proc.waitForFinished(-1);
		return PRL_ERR_OPERATION_FAILED;
	}
	if (m_proc.exitStatus() != QProcess::NormalExit) {
		WRITE_TRACE(DBG_FATAL, "'%s command crashed", QSTR2UTF8(sScript));
		return PRL_ERR_OPERATION_FAILED;
	}
	int ret = m_proc.exitCode();
	if (ret != 0)
	{
		WRITE_TRACE(DBG_FATAL, "%s utility failed: [%d]",
				QSTR2UTF8(sScript),
				m_proc.exitCode());
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RunVmAction::RunAction(
		const QString &sVmUuid,
		const QString &sVmDirUuid,
		PRL_VM_ACTION nAction)
{
	QStringList lstEnv;

	lstEnv += QString("%1=%2").arg("UUID").arg(sVmUuid);
	QString sVmHome;
	{
		CDspLockedPointer<CVmDirectoryItem> pItem =
			CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid(
				sVmDirUuid, sVmUuid);
		if (!pItem) {
			WRITE_TRACE(DBG_FATAL, "Vm action script skipped vmuuid %s vmdiruuid %s",
					QSTR2UTF8(sVmUuid), QSTR2UTF8(sVmDirUuid));
			return 0;
		}
		sVmHome = CFileHelper::GetFileRoot(pItem->getVmHome());
		lstEnv += QString("%1=%2").arg("VM_HOME").arg(sVmHome);
	}
	QString sScript = ParallelsDirs::getVmActionScriptPath(sVmHome, nAction);

	QFileInfo fi(sScript);
	if (!fi.exists())
		return 0;

	return RunScript(sScript, lstEnv);
}

PRL_RESULT Task_RunVmAction::ConcreteDoBackgroundJob()
{
	return RunAction(m_pVm->getVmUuid(), m_pVm->getVmDirUuid(), m_nAction);
}

void Task_RunVmAction::finalizeTask()
{
}

Task_SendHostHardwareInfo::Task_SendHostHardwareInfo( const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &p )
:Task_BackgroundJob( pUser, p )
{
}

PRL_RESULT Task_SendHostHardwareInfo::ConcreteDoBackgroundJob()
{
	CDspService::instance()->getShellServiceHelper()
		.sendHostHardwareInfo( getClient(), getRequestPackage() );
	return PRL_ERR_SUCCESS;
}

Task_PendentClientRequest::Task_PendentClientRequest( const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &p )
:Task_BackgroundJob( pUser, p )
{
}

PRL_RESULT Task_PendentClientRequest::ConcreteDoBackgroundJob()
{
	PVE::IDispatcherCommands cmd = (PVE::IDispatcherCommands)getRequestPackage()->header.type;
	WRITE_TRACE( DBG_FATAL, "Command '%s' %d processing was held up until dispatcher inited completelly.",
			PVE::DispatcherCommandToString(cmd),
			cmd );

	if( !CDspService::instance()->waitForInitCompletion( ) )
	{
		WRITE_TRACE(DBG_FATAL, "Timeout is over ! Service initialization was not done !" );
		getClient()->sendSimpleResponse(getRequestPackage(), PRL_ERR_TIMEOUT);
		return PRL_ERR_TIMEOUT;
	}

	WRITE_TRACE( DBG_FATAL, "Command '%s' %d processing will be continued",
			PVE::DispatcherCommandToString(cmd),
			cmd );

	CDspService::instance()->getClientManager().handleToDispatcherPackage(
		getClient()->getClientHandle(), getRequestPackage() );

	return PRL_ERR_SUCCESS;
}

