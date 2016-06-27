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
///	Task_UpdateCommonPrefs.cpp
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

#include "Task_UpdateCommonPrefs.h"
#include "Task_CommonHeaders.h"
#include <Build/Current.ver>

#include "CDspClientManager.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "CDspService.h"
#include "CDspVmManager.h"
#include "CDspCommon.h"
#include "CDspVzHelper.h"
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"

#include <prlcommon/Std/PrlAssert.h>

#include <prlsdk/PrlErrors.h>

#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <QUrl>


using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


QMutex Task_UpdateCommonPrefs::s_commonPrefsMutex( QMutex::Recursive );

Task_UpdateCommonPrefs::Task_UpdateCommonPrefs (
    SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
	const QString& sCommonPrefs
)
:	CDspTaskHelper(client, p),
	m_commonPrefsMutexLocked( false )
{
	m_pOldCommonPrefs = SmartPtr<CDispCommonPreferences>(new CDispCommonPreferences(
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs().getPtr() ));

	QList<PRL_ALLOWED_VM_COMMAND>
		oldCommonConfirmListFromVmDirectory = CDspService::instance()->getVmDirManager().getCommonConfirmatedOperations();
	m_pOldCommonPrefs->getLockedOperationsList()->setLockedOperations( oldCommonConfirmListFromVmDirectory );

	m_pNewCommonPrefs = SmartPtr<CDispCommonPreferences>(new CDispCommonPreferences());

	// Unite new and old format XML documents
	// It should be done for server products, but
	// we do it for desktop products too to prevent potential security hole
	//	 ( old plrsrvctl could cleanup critical sections)
	m_pNewCommonPrefs->uniteDocuments(m_pOldCommonPrefs->toString(), sCommonPrefs);
	if( !IS_OPERATION_SUCCEEDED( m_pNewCommonPrefs->m_uiRcInit ) )
	{
		setLastErrorCode( PRL_ERR_PARSE_COMMON_SERVER_PREFS );
		return;
	}

	fixReadOnlyInCommonPrefs();
	fixReadOnlyInWorkspace();
	fixReadOnlyInRemoteDisplay();
	fixReadOnlyInMemory();
	fixReadOnlyInNetwork();
	fixReadOnlyInPci();
	fixReadOnlyInDebug();
	// USB prefs are fixed under lock in saveCommonPrefs later @34326
	//fixReadOnlyInUsbPrefs();
	fixReadOnlyListenAnyAddr();
	fixReadOnlyInDispToDispPrefs();
}

Task_UpdateCommonPrefs::~Task_UpdateCommonPrefs()
{
}

/**
 * @brief method executed before thread started
 */
PRL_RESULT Task_UpdateCommonPrefs::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	/**
	 * get request parameters
	 */

	try
	{
		// may be changed in constructor
		if ( ! PRL_SUCCEEDED( getLastErrorCode() ) )
			throw getLastErrorCode();

		// try lock config from other user
		if ( ! s_commonPrefsMutex.tryLock() )
		{
			getLastError()->setEventCode(PRL_ERR_COMMON_SERVER_PREFS_BLOCKED_TO_CHANGE);
			throw PRL_ERR_COMMON_SERVER_PREFS_BLOCKED_TO_CHANGE;
		}
		m_commonPrefsMutexLocked = true;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while modification common server prefs with code [%#x]", code);
	}

	return ret;
}

#define WAITINTERVAL	1000

PRL_RESULT Task_UpdateCommonPrefs::run_body()
{
	PRL_RESULT ret = getLastErrorCode();
	if (!IS_OPERATION_SUCCEEDED(ret))
	{
		PRL_ASSERT (0);
		WRITE_TRACE(DBG_FATAL, "logic error! run_body() executed but error code is failed.");
		return setErrorCode(ret); // To properly copy-paste support :).
		// We do it instead 'return ret' to prevent ASSERT in TaskHelper.cpp after run_body().
	}

	if( PRL_FAILED(ret = checkHeadlessMode() ) )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to change HeadlessMode" );
		return setErrorCode(ret);
	}

	CDispWorkspacePreferences* pNewWP = m_pNewCommonPrefs->getWorkspacePreferences();

#ifndef _WIN_
	pNewWP->setListenAnyAddr( pNewWP->isAllowMultiplePMC() );
#endif

	// Check "AllowMultiplePMC" and "AllowDirectMobile" options change
	// and update firewall/iptables params if options were changed.
	// https://jira.sw.ru:9443/browse/PWE-5622
	checkAndDisableFirewall();

	try
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=439457
		//Process log level
		if ( m_pNewCommonPrefs->getDebug()->isVerboseLogWasChanged() )
		{
			if (!getClient()->getAuthHelper().isLocalAdministrator())
				throw PRL_ERR_ONLY_ADMIN_CAN_SET_VERBOSE_LOGGING;

			bool bVerboseLogEnabled = m_pNewCommonPrefs->getDebug()->isVerboseLogEnabled();
			WRITE_TRACE( DBG_FATAL, "Applying new verbose log level. bVerboseLogEnabled=%d", (int)bVerboseLogEnabled );
			int log_level = (bVerboseLogEnabled ? DBG_DEBUG : -1);
			SetLogLevel(log_level);
			CDspService::instance()->getVmManager().changeLogLevelForActiveVMs( getClient(), bVerboseLogEnabled );
		}


		ret = saveCommonPrefs();
		if (PRL_FAILED(ret))
			throw ret;

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = setErrorCode(code);
	}//catch

	return ret;
}

PRL_RESULT Task_UpdateCommonPrefs::saveCommonPrefs()
{
	QList<PRL_ALLOWED_VM_COMMAND>
		oldCommonConfirmList = m_pOldCommonPrefs->getLockedOperationsList()->getLockedOperations();
	QList<PRL_ALLOWED_VM_COMMAND>
		newCommonConfirmList = m_pNewCommonPrefs->getLockedOperationsList()->getLockedOperations();

	// clear to prevent store it in dispatcher config ( on disk and in memory )
	m_pNewCommonPrefs->getLockedOperationsList()->setLockedOperations();

	// store CommonConfirmList values to vmdirectorylist.xml
	{
		CDspLockedPointer<CVmDirectories>
			pLockedVmCatalogue = CDspService::instance()->getVmDirManager().getVmDirCatalogue();
		pLockedVmCatalogue->getCommonLockedOperations()
			->setLockedOperations( newCommonConfirmList );
		PRL_RESULT save_rc = CDspService::instance()->getVmDirManager().saveVmDirCatalogue();
		if( PRL_FAILED(save_rc) )
		{
			// write trace was called in saveVmDirCatalogue()
			pLockedVmCatalogue->getCommonLockedOperations()->setLockedOperations( oldCommonConfirmList );
			return save_rc;
		}

	}

	// EDIT LOCK

	// check to change config from other user
	// NOTE: in this task m_pCommonPrefsEdit safed by 'm_commonPrefsMutex'
	QMutexLocker editLock( CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig() );

	CDspLockedPointer<CDispatcherConfig>
		pLockedDispConfig = CDspService::instance()->getDispConfigGuard().getDispConfig();

	SmartPtr<CDispatcherConfig > pDispConfigOld(
		new CDispatcherConfig( pLockedDispConfig.getPtr() ) );

	CDspLockedPointer<CDispCommonPreferences>
		pLockedDispCommonPrefs = CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();

	fixReadOnlyInUsbPrefs( pLockedDispCommonPrefs.getPtr() );
	pLockedDispCommonPrefs->fromString( m_pNewCommonPrefs->toString() );

	std::auto_ptr<CDispCpuPreferences> cpuMask(CCpuHelper::get_cpu_mask());
	if (!cpuMask.get())
		return PRL_ERR_FAILURE;
	pDispConfigOld->getDispatcherSettings()->
		getCommonPreferences()->getCpuPreferences()->setFeatures(*cpuMask);

	// MERGE
	SmartPtr<CDispatcherConfig > pDispConfigNew(
		new CDispatcherConfig( pLockedDispConfig.getPtr() ) );

	if ( ! CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()
				->tryToMerge(getClient()->getClientHandle(), pDispConfigNew, pDispConfigOld) )
		return PRL_ERR_COMMON_SERVER_PREFS_WERE_CHANGED;

	pLockedDispConfig->fromString(pDispConfigNew->toString());

	// Save dispatcher config file
	PRL_RESULT save_rc = CDspService::instance()->getDispConfigGuard().saveConfig();

	if (PRL_SUCCEEDED(save_rc))
	{
		CDspService::instance()->getDispConfigGuard().getMultiEditDispConfig()
			->registerCommit(getClient()->getClientHandle());
	}

	editLock.unlock();

	if( PRL_FAILED( save_rc ) )
	{
		WRITE_TRACE(DBG_FATAL, "Unable to write data to disp configuration file by error %s"
			, PRL_RESULT_TO_STRING(save_rc) );

		CDspService::instance()->getDispConfigGuard()
				.getDispCommonPrefs()->fromString( m_pOldCommonPrefs->toString() );

		pLockedDispCommonPrefs.unlock();
		{
			CDspLockedPointer<CVmDirectories>
				pLockedVmCatalogue = CDspService::instance()->getVmDirManager().getVmDirCatalogue();
			pLockedVmCatalogue->getCommonLockedOperations()->setLockedOperations( oldCommonConfirmList );
			CDspService::instance()->getVmDirManager().saveVmDirCatalogue();
		}

		return save_rc;
	}

	pLockedDispCommonPrefs.unlock();
	pLockedDispConfig.unlock();

	CDispCpuPreferences* o(pDispConfigOld->getDispatcherSettings()->getCommonPreferences()->getCpuPreferences());
	CDispCpuPreferences* n(pDispConfigNew->getDispatcherSettings()->getCommonPreferences()->getCpuPreferences());
	QStringList diff;
	o->diff(n, diff);
	if (o->isCpuFeaturesMaskValid() && n->isCpuFeaturesMaskValid() && diff.size() > 0) {
		PRL_RESULT ret = updateCpuFeaturesMask(*o, *n);
		if (PRL_FAILED(ret))
			return ret;
	}

	return PRL_ERR_SUCCESS;
}

void Task_UpdateCommonPrefs::checkAndDisableFirewall()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	CDispWorkspacePreferences* pOld = m_pOldCommonPrefs->getWorkspacePreferences();
	CDispWorkspacePreferences* pNew = m_pNewCommonPrefs->getWorkspacePreferences();

	bool bOldAllowPmcMc = pOld->isAllowMultiplePMC();
	bool bNewAllowPmcMc = pNew->isAllowMultiplePMC();

	// Check if something was changed
	if (bOldAllowPmcMc == bNewAllowPmcMc)
		return;

	// Update firewall/iptables settings
	CDspService::instance()->checkAndDisableFirewall(bNewAllowPmcMc);
}

/**
 * @brief deferred method implementation
 */
void Task_UpdateCommonPrefs::finalizeTask()
{
	if ( PRL_SUCCEEDED(getLastErrorCode()) )
		PRL_ASSERT(m_commonPrefsMutexLocked);

	if (m_commonPrefsMutexLocked)
	{
		s_commonPrefsMutex.unlock();
		m_commonPrefsMutexLocked = false;
	}

	if ( PRL_SUCCEEDED( getLastErrorCode() ) )
	{
		//Update host info now in view of virtual network adapters list can be changed
		CDspService::instance()->getHostInfo()->refresh();
		CVmEvent event(PET_DSP_EVT_HW_CONFIG_CHANGED, QString(), PIE_DISPATCHER);
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());

		CDspService::instance()->getClientManager().sendPackageToAllClients(p);

		CDspService::instance()->notifyConfigChanged(m_pOldCommonPrefs, m_pNewCommonPrefs);
	}

	// send response
	if ( ! PRL_SUCCEEDED( getLastErrorCode() ) )
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	else
		getClient()->sendSimpleResponse( getRequestPackage(), getLastErrorCode() );
}

void Task_UpdateCommonPrefs::fixReadOnlyInCommonPrefs()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	m_pNewCommonPrefs->setLockedSign( m_pOldCommonPrefs->isLockedSign() );
	m_pNewCommonPrefs->getFastRebootPreferences()->setFastReboot(
		m_pOldCommonPrefs->getFastRebootPreferences()->isFastReboot()
	);
	m_pNewCommonPrefs->getFastRebootPreferences()->setDefaultPramPath(
		m_pOldCommonPrefs->getFastRebootPreferences()->getDefaultPramPath()
	);
	bool bLogRotateEnabled = m_pNewCommonPrefs->getLogRotatePreferences()->isEnabled();
	m_pNewCommonPrefs->setLogRotatePreferences(
		new CDispLogRotatePreferences(m_pOldCommonPrefs->getLogRotatePreferences())
	);
	m_pNewCommonPrefs->getLogRotatePreferences()->setEnabled(bLogRotateEnabled);

}

void Task_UpdateCommonPrefs::fixReadOnlyInWorkspace()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	CDispWorkspacePreferences* pOld = m_pOldCommonPrefs->getWorkspacePreferences();
	CDispWorkspacePreferences* pNew = m_pNewCommonPrefs->getWorkspacePreferences();

	pNew->setDefaultVmDirectory		( pOld->getDefaultVmDirectory() );
	pNew->setDistributedDirectory	( pOld->isDistributedDirectory() );
	pNew->setDispatcherPort			( pOld->getDispatcherPort() );
	pNew->setDefaultCommandHistorySize( pOld->getDefaultCommandHistorySize() );
	pNew->setVmTimeoutOnShutdown	( pOld->getVmTimeoutOnShutdown() );
	// #424340
	pNew->setAllowUseNetworkShares	(  pOld->isAllowUseNetworkShares() );
	pNew->getLimits()->setMaxLogonActions( pOld->getLimits()->getMaxLogonActions() );
	pNew->getLimits()->setMaxAuthAttemptsPerMinute( pOld->getLimits()->getMaxAuthAttemptsPerMinute() );

	pNew->setConfirmationModeByDefault( pOld->getConfirmationModeByDefault() );
	pNew->setVmUptimeSyncTimeoutInMinutes( pOld->getVmUptimeSyncTimeoutInMinutes() );

	pNew->setDefaultPlainDiskAllowed( pOld->isDefaultPlainDiskAllowed() );

	pNew->setVmConfigWatcherEnabled( pOld->isVmConfigWatcherEnabled() );

	pNew->setVmConfigCacheEnabled( pOld->isVmConfigCacheEnabled() );
}

void Task_UpdateCommonPrefs::fixReadOnlyInRemoteDisplay()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );
}

void Task_UpdateCommonPrefs::fixReadOnlyInMemory()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	//////////////////////////////////////////////////////////////////////////
	// #269185  safe read only memory values
	bool bIsAdjustMemAuto
		= m_pNewCommonPrefs->getMemoryPreferences()->isAdjustMemAuto();
	unsigned int nReservedMemoryLimit
		= m_pNewCommonPrefs->getMemoryPreferences()->getReservedMemoryLimit();

	m_pNewCommonPrefs->setMemoryPreferences(
		new CDispMemoryPreferences( m_pOldCommonPrefs->getMemoryPreferences() )
		);

	// restore not-read-only values
	m_pNewCommonPrefs->getMemoryPreferences()->setAdjustMemAuto( bIsAdjustMemAuto );
	m_pNewCommonPrefs->getMemoryPreferences()->setReservedMemoryLimit( nReservedMemoryLimit );
	//
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// round to 4:
	if( nReservedMemoryLimit % 4 != 0 )
	{
		unsigned int nTail = nReservedMemoryLimit % 4;
		WRITE_TRACE(DBG_FATAL, "ReservedMemoryLimit = %d will aligned to %d"
			, nReservedMemoryLimit
			, nReservedMemoryLimit - nTail );
		m_pNewCommonPrefs->getMemoryPreferences()->setReservedMemoryLimit( nReservedMemoryLimit - nTail );
	}
}

void Task_UpdateCommonPrefs::fixReadOnlyInNetwork()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	// #116344: new network config support
	//		disable any changes in network configuration as obsolete.
	m_pNewCommonPrefs->setNetworkPreferences(
		new CDispNetworkPreferences( m_pOldCommonPrefs->getNetworkPreferences() )
		);
}

void Task_UpdateCommonPrefs::fixReadOnlyInPci()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	m_pNewCommonPrefs->getPciPreferences()->setPrimaryVgaAllowed(
		m_pOldCommonPrefs->getPciPreferences()->isPrimaryVgaAllowed() );

	// Generic PCI device preferences has not to be changed here
	// Hence restore old values
	m_pNewCommonPrefs->getPciPreferences()->getGenericPciDevices()->fromString(
		m_pOldCommonPrefs->getPciPreferences()->getGenericPciDevices()->toString() );
}

void Task_UpdateCommonPrefs::fixReadOnlyInDebug()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	bool bVerboseLogLevel = m_pNewCommonPrefs->getDebug()->isVerboseLogEnabled();
	bool bVerboseLogLevelWasChanged = m_pNewCommonPrefs->getDebug()->isVerboseLogWasChanged();
	m_pNewCommonPrefs->setDebug(
		new CDspDebug( m_pOldCommonPrefs->getDebug() )
	);
	m_pNewCommonPrefs->getDebug()->setVerboseLogEnabled( bVerboseLogLevel );
	m_pNewCommonPrefs->getDebug()->setVerboseLogWasChanged( bVerboseLogLevelWasChanged );
}

void Task_UpdateCommonPrefs::fixReadOnlyInUsbPrefs(
	CDispCommonPreferences * pOldCommonPrefs)
{
	PRL_ASSERT( pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	QString dirUuid = getClient()->getVmDirectoryUuid();

	// USB permanent assignement could be changed here
	QHash<QString,QString> savedUuids;
	QHash<QString,PRL_USB_DEVICE_AUTO_CONNECT_OPTION> savedActions;
	foreach (CDispUsbIdentity * ui, m_pNewCommonPrefs->getUsbPreferences()->m_lstAuthenticUsbMap)
	{
		if( ui->m_lstAssociations.isEmpty() )
			continue;
		savedUuids.insert( ui->getSystemName(), ui->m_lstAssociations[0]->getVmUuid() );
		savedActions.insert( ui->getSystemName(), ui->m_lstAssociations[0]->getAction() );
	}

	m_pNewCommonPrefs->getUsbPreferences()->fromString(
		pOldCommonPrefs->getUsbPreferences()->toString() );

	foreach (CDispUsbIdentity * ui, m_pNewCommonPrefs->getUsbPreferences()->m_lstAuthenticUsbMap)
	{
		if( savedActions.contains( ui->getSystemName() ) )
		{
			CDispUsbAssociation * ua = NULL;
			foreach (ua, ui->m_lstAssociations)
				if( ua->getDirUuid() == dirUuid )
					break;
			if( !ua )
			{
				ua = new CDispUsbAssociation;
				ua->setDirUuid( dirUuid );
				ui->m_lstAssociations.append( ua );
			}
			ua->setAction( savedActions[ ui->getSystemName() ] );
			if( savedUuids.contains( ui->getSystemName() ) )
				ua->setVmUuid( savedUuids[ ui->getSystemName() ] );
		} else
		{
			foreach (CDispUsbAssociation * ua, ui->m_lstAssociations) {
				if ( ua->getDirUuid() == dirUuid )
				{
					if ( ui->m_lstAssociations.removeOne( ua ) )
						delete ua;
					break;
				}
			}
		}
	}
}

void Task_UpdateCommonPrefs::fixReadOnlyListenAnyAddr()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	m_pNewCommonPrefs->getWorkspacePreferences()->setListenAnyAddr(
		m_pOldCommonPrefs->getWorkspacePreferences()->isListenAnyAddr() );
}

void Task_UpdateCommonPrefs::fixReadOnlyInDispToDispPrefs()
{
	PRL_ASSERT( m_pOldCommonPrefs );
	PRL_ASSERT( m_pNewCommonPrefs );

	m_pNewCommonPrefs->getDispToDispPreferences()->setConnectionTimeout(
		m_pOldCommonPrefs->getDispToDispPreferences()->getConnectionTimeout()
	);
	m_pNewCommonPrefs->getDispToDispPreferences()->setSendReceiveTimeout(
		m_pOldCommonPrefs->getDispToDispPreferences()->getSendReceiveTimeout()
	);
}

PRL_RESULT Task_UpdateCommonPrefs::setErrorCode(PRL_RESULT nResult)
{
	getLastError()->setEventCode(nResult);
	WRITE_TRACE(DBG_FATAL, "Error occurred while modification common server prefs with code [%#x]", nResult);
	return nResult;
}

bool Task_UpdateCommonPrefs::isHostIdChanged() const
{
	return false;
}

PRL_RESULT Task_UpdateCommonPrefs::updateHostId()
{
	return PRL_ERR_CANT_CHANGE_HOST_ID;
}

PRL_RESULT Task_UpdateCommonPrefs::checkHeadlessMode()
{
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_UpdateCommonPrefs::updateCpuFeaturesMask(
	const CDispCpuPreferences &oldMask, const CDispCpuPreferences &newMask)
{
	if (CCpuHelper::isMasksEqual(oldMask, newMask))
		return PRL_ERR_SUCCESS;

	PRL_RESULT ret = CCpuHelper::maskUpdate(newMask);
	if (PRL_FAILED(ret))
		return ret;

	QString mask(newMask.toString());

	QMultiHash<QString, SmartPtr<CVmConfiguration> > vms =
		CDspService::instance()->getVmDirHelper().getAllVmList();
	foreach(QString dirUuid, vms.uniqueKeys())
	{
		foreach(SmartPtr<CVmConfiguration> pVmConfig, vms.values(dirUuid))
		{
			if(!pVmConfig)
				continue;

			CVmEvent e;
			e.addEventParameter(new CVmEventParameter(PVE::String,
				mask, EVT_PARAM_VMCFG_CPU_FEATURES_MASK));

			QString vmUuid = pVmConfig->getVmIdentification()->getVmUuid();

			if (CDspService::instance()->getVmDirHelper().atomicEditVmConfigByVm(
				dirUuid, vmUuid, e, getClient()))
			{
				CVmEvent event(PET_DSP_EVT_VM_CONFIG_CHANGED, vmUuid, PIE_DISPATCHER);
				SmartPtr<IOPackage> pkg = DispatcherPackage::createInstance(
					PVE::DspVmEvent, event, getRequestPackage());
				CDspService::instance()->getClientManager().sendPackageToVmClients(
					pkg, dirUuid, vmUuid);
			}
		}
	}

	return PRL_ERR_SUCCESS;
}
