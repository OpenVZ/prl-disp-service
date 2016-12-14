///////////////////////////////////////////////////////////////////////////////
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
///	Task_RegisterVm.cpp
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

#include "Task_CreateImage.h"
#include "Task_RegisterVm.h"
#include "Task_CommonHeaders.h"
#include "Task_EditVm.h"
#include "Task_EditVm_p.h"
#include "Task_ManagePrlNetService.h"
#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#endif // _LIBVIRT_
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "CDspClientManager.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"
#include "CVmValidateConfig.h"
#include "CDspBugPatcherLogic.h"
#include "CDspVmManager_p.h"
#include "CDspBackupDevice.h"
#include <ctime>

#include <Libraries/PrlNetworking/netconfig.h>

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

const quint64 SIZE_64GB	= 64ULL*1024*1024*1024;		// 64 Gb
const quint64 SIZE_32GB = 32ULL*1024*1024*1024;		// 32 Gb

#define QSET_TO_UPPER_CASE( qset ) \
	do \
{	\
	QSet<QString> newSet; \
	for( QSet<QString>::iterator it=qset.begin(); it != qset.end(); it++ )	\
			newSet.insert( it->toUpper() );	\
	qset = newSet; \
	} while(0)

/**
 * @brief Callback function, called from CreateParallelsDisk
 * @param iDone
 * @param iTotal
 * @param pUserData
 * @return TRUE to continue HardDisk image creation
 */
bool HddCallbackToRegisterVmTask(int iDone, int iTotal, void *pUserData)
{
	// Getting current class
	Task_RegisterVm*			pTaskOriginal = reinterpret_cast<Task_RegisterVm*>(pUserData);

	Mixin_CreateHddSupport* pHddCreateHelper = pTaskOriginal;

	return HddCallbackHelperFunc(iDone, iTotal, pHddCreateHelper);
}

/**
 *  Task Create New VM
 */
Task_RegisterVm::Task_RegisterVm(
	Registry::Public& registry_,
	SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
	const  QString& vm_config,
	const  QString& vm_rootDir,
	int nFlags)
:	Mixin_CreateHddSupport(client, p, nFlags & PACF_NON_INTERACTIVE_MODE),
	m_vmRootDir(vm_rootDir),
	m_pVmInfo(0),
	m_pVmConfig( new CVmConfiguration( vm_config ) ),
	m_dirUuid(client->getVmDirectoryUuid()),
	m_flgLockRegistred(false),
	m_mtxWaitCreateImage(QMutex::Recursive),
	m_lpcCreateImageCurrentTask( NULL ),
	m_bVmRegisterNameWasEmpty(false),
	m_nFlags(nFlags),
	m_nInternalParamsAsMasks(0),
	m_registry(registry_)
{
	m_flgRegisterOnly = false;
	m_bForceRegisterOnSharedStorage = false;

	WRITE_TRACE( DBG_FATAL, "Begin to create VM '%s' / %s in [%s]"
		, QSTR2UTF8( m_pVmConfig->getVmIdentification()->getVmName() )
		, QSTR2UTF8( m_pVmConfig->getVmIdentification()->getVmUuid() )
		, m_vmRootDir.isEmpty()? "default path" : QSTR2UTF8( m_vmRootDir )
	);
	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		m_dirUuid = CDspVmDirManager::getTemplatesDirectoryUuid();
}

/**
 *  Task Register Vm
 */
Task_RegisterVm::Task_RegisterVm (
	Registry::Public& registry_,
	SmartPtr<CDspClient>& client,
	const SmartPtr<IOPackage>& p,
	const QString& strPathToVmDirToRegister,
	int nFlags,
	const QString& strCustomVmUuid,
	const QString& strApplianceId,
	unsigned int nInternalParamsAsMask)
:	Mixin_CreateHddSupport(client, p, nFlags & PACF_NON_INTERACTIVE_MODE),
	m_vmRootDir(""),
	m_pVmInfo(0),
	m_pVmConfig(0),
	m_dirUuid(client->getVmDirectoryUuid()),
	m_flgLockRegistred(false),
	m_mtxWaitCreateImage(QMutex::Recursive),
	m_lpcCreateImageCurrentTask( NULL ),
	m_bVmRegisterNameWasEmpty(false),
	m_nFlags(nFlags),
	m_strApplianceId(strApplianceId),
	m_nInternalParamsAsMasks(nInternalParamsAsMask),
	m_sCustomVmUuid(strCustomVmUuid),
	m_registry(registry_)
{
	m_flgRegisterOnly = true;
	if (!strPathToVmDirToRegister.isEmpty())
		m_strPathToVmDirToRegister = QDir(strPathToVmDirToRegister).absolutePath();
	m_bForceRegisterOnSharedStorage = false;

	if ( ! strPathToVmDirToRegister.isEmpty() && ! QDir::isAbsolutePath( strPathToVmDirToRegister ) )
		setLastErrorCode ( PRL_ERR_VMDIR_PATH_IS_NOT_ABSOLUTE );

	QString vm_xml_path=QString("%1/%2")
		.arg( m_strPathToVmDirToRegister )
		.arg(VMDIR_DEFAULT_VM_CONFIG_FILE);

	// load VM configuration from file
	//////////////////////////////////////////////////////////////////////////
	QFile	file( vm_xml_path );

	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration( &file ));
	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		m_dirUuid = CDspVmDirManager::getTemplatesDirectoryUuid();

	//////////////////////////////////////////////////////////////////////////
	setTaskParameters( m_strPathToVmDirToRegister );

	WRITE_TRACE( DBG_FATAL, "Begin to register VM from path [%s] customuuid [%s]",
			QSTR2UTF8( m_strPathToVmDirToRegister ),
			QSTR2UTF8( m_sCustomVmUuid ) );
}

Task_RegisterVm::~Task_RegisterVm()
{
	if (m_pVmInfo)
		delete m_pVmInfo;
}

void Task_RegisterVm::setTaskParameters( const  QString& vm_config,
										const  QString& vm_rootDir )
{
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
		new CVmEventParameter( PVE::CData, vm_config, EVT_PARAM_DISP_TASK_VM_CREATE_CONFIG ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, vm_rootDir, EVT_PARAM_DISP_TASK_VM_CREATE_ROOT_DIR ) );
}

void Task_RegisterVm::setTaskParameters( const QString& strPathToVmDirToRegister )
{
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, strPathToVmDirToRegister, EVT_PARAM_DISP_TASK_VM_REGISTER_PATH ) );
}

bool Task_RegisterVm::doRegisterOnly()
{
	return m_flgRegisterOnly;
}

QString Task_RegisterVm::getVmConfig()
{
	if ( m_pVmConfig )
		return m_pVmConfig->toString();
	return "";
}


QString Task_RegisterVm::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

void Task_RegisterVm::extractMACAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config, QSet<QString>& macs)
{
	QListIterator<CVmHardware*> itH(config->m_lstHardware);
	while (itH.hasNext())
	{
		CVmHardware* hard = itH.next();
		if (hard)
		{
			QListIterator<CVmGenericNetworkAdapter*> itN(hard->m_lstNetworkAdapters);
			while (itN.hasNext())
			{
				CVmGenericNetworkAdapter* adapter(itN.next());
				if (adapter)
					macs.insert(adapter->getMacAddress().toUpper());
			}
		}
	}
}

void Task_RegisterVm::createMACAddress(SmartPtr<CVmConfiguration> config,
										QSet<QString> duplicates,
										bool bCheckEmpty /* = false*/)
{
	QSET_TO_UPPER_CASE( duplicates );

	QListIterator<CVmHardware*> itH(config->m_lstHardware);
	while (itH.hasNext())
	{
		CVmHardware* hard = itH.next();
		if (hard)
		{
			QListIterator<CVmGenericNetworkAdapter*> itN(hard->m_lstNetworkAdapters);
			while (itN.hasNext())
			{
				CVmGenericNetworkAdapter* adapter(itN.next());

				bool bNeedCreate	= ( bCheckEmpty && adapter && adapter->getMacAddress().isEmpty() )
						|| ( !bCheckEmpty && adapter &&
						( ( duplicates.isEmpty() && !adapter->isStaticAddress() )
						 || duplicates.contains( adapter->getMacAddress().toUpper() )
						) );

					if( bNeedCreate )
					{
						if(adapter->getHostInterfaceName() == HostUtils::generateHostInterfaceName(adapter->getMacAddress()))
							adapter->setHostInterfaceName();

						adapter->setMacAddress(HostUtils::generateMacAddress());
						adapter->setHostMacAddress();
					}
			}

			// #PSBM-13394
			CDspVmNetworkHelper::updateHostMacAddresses(config, NULL, HMU_NONE);
		}
	}
}

QString Task_RegisterVm::getUniqueVmName(const QString& sVmNamePattern, const QString& vmDirUuid)
{
	CDspLockedPointer<CVmDirectory> pVmDir =
					CDspService::instance()->getVmDirManager().getVmDirectory(vmDirUuid);

	PRL_ASSERT(pVmDir);
	QList<CVmDirectoryItem* > lstVmDirItems = pVmDir->m_lstVmDirectoryItems;
	// Get all VM names in VM directory

	QStringList lstVmNames;
	for(int i = 0; i < lstVmDirItems.size(); ++i)
	{
		CVmDirectoryItem* pVmDirItem = lstVmDirItems[i];
		lstVmNames += pVmDirItem->getVmName();
	}

	lstVmNames += sVmNamePattern + " ()";	// exclude name without index

	QString sVmName = Parallels::GenerateFilename(lstVmNames, sVmNamePattern,
											  QString(), QString());
	if (sVmName == sVmNamePattern)
	{
		return sVmName;
	}

	// Generate unique VM name like file name
	return Parallels::GenerateFilename(lstVmNames, sVmNamePattern + " (",
										  QString(")"), QString());
}


void Task_RegisterVm::checkOperationPermission()
{
	if (!doRegisterOnly())
	{
		//#436109 check for command confirmations
		PRL_RESULT rc = CDspService::instance()->getAccessManager().checkAccess(
			getClient(), PVE::DspCmdDirVmCreate );
		if( PRL_FAILED(rc) )
			throw rc;
	}
	else //doRegisterOnly()
	{
		// check rights to access vm files
		// AccessCheck
		PRL_RESULT rc = PRL_ERR_FAILURE;
		SmartPtr<CVmDirectoryItem>	pItem( new CVmDirectoryItem() );

		QString vm_xml_path = QString( "%1/%2" )
			.arg( m_strPathToVmDirToRegister )
			.arg( VMDIR_DEFAULT_VM_CONFIG_FILE );

		CVmEvent evt;
		pItem->setVmHome( vm_xml_path );
		rc = CDspService::instance()->getAccessManager().checkAccess(
				getClient(), PVE::DspCmdDirRegVm, pItem.getImpl(), NULL, &evt );
		if ( PRL_FAILED(rc) )
		{
			switch( rc )
			{
			case PRL_ERR_VM_CONFIG_INVALID_VM_UUID:
			case PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID:
				// skip it for registration
				break;
			default:
				getLastError()->fromString(evt.toString());
				throw rc;
			}
		}
	}//doRegisterOnly()
}

bool Task_RegisterVm::isVMConfigurationHasDynamicMacAddress(SmartPtr<CVmConfiguration> config)
{
	QListIterator<CVmHardware*> itH(config->m_lstHardware);
	while (itH.hasNext())
	{
		CVmHardware* hard = itH.next();
		if (hard)
		{
			QListIterator<CVmGenericNetworkAdapter*> itN(hard->m_lstNetworkAdapters);
			while (itN.hasNext())
			{
				CVmGenericNetworkAdapter* adapter(itN.next());
				if (adapter && !adapter->isStaticAddress())
					return true;
			}
		}
	}
	return false;
}

void Task_RegisterVm::checkWhereFromRegisteredVm()
{
	bool bUpdateMacInConfig = isApplianceRegistered();
	bool bUpdateVmUuid = false;
	bool bConfigHasDynamicMac = isVMConfigurationHasDynamicMacAddress(m_pVmConfig);

	if ( bUpdateMacInConfig && bConfigHasDynamicMac )
		createMACAddress(m_pVmConfig);

	if ( bUpdateVmUuid )
	{
		WRITE_TRACE( DBG_FATAL, "vm uuid will be regenerated by user choice on registration!" );
		regenerateVmUuid();
	}
}

void Task_RegisterVm::checkDynamicMACAddress()
{
	//////////////////////////////////////////////////////////////////////////
	// STEP 1
	// search and fix conflicts in mac addresses  with registered vms
	if( doRegisterOnly() )
	{
		QSet<QString> macs;
		extractMACAddressesFromVMConfiguration(m_pVmConfig, macs);
		QSET_TO_UPPER_CASE( macs );

		QSet<QString> duplicates;

		// 1. + store MultiHash <Mac,vmIdent>
		//MultiHash< QString, CVmIdent > hashMacAddrToVmId;

		QMultiHash< QString, SmartPtr<CVmConfiguration> >
			vmTotalHash = CDspService::instance()->getVmDirHelper().getAllVmList();
		foreach( QString dirUuid, vmTotalHash.keys() )
		{
			QList< SmartPtr<CVmConfiguration> >
				vmList = vmTotalHash.values( dirUuid );
			for( int idx=0; idx<vmList.size(); idx++)
			{
				SmartPtr<CVmConfiguration> pConfig( vmList[idx] );
				if (!pConfig)
					continue;

				QSet<QString> used;
				extractMACAddressesFromVMConfiguration(pConfig, used);
				QSET_TO_UPPER_CASE( used );

				QSet<QString>  preDuplicates = used.intersect(macs);
				if( !preDuplicates.isEmpty() )
				{
					CDspLockedPointer< CVmDirectoryItem >
						pDirItem = CDspService::instance()->getVmDirManager()
							.getVmDirItemByUuid( dirUuid, pConfig->getVmIdentification()->getVmUuid() );
					if( !pDirItem )
						continue;

					PRL_ASSERT( m_pVmInfo );
					if( m_pVmInfo && pDirItem->getVmUuid() == m_pVmInfo->vmUuid
						&& pDirItem->getVmHome() == m_pVmInfo->vmXmlPath
						)
					{
						WRITE_TRACE( DBG_DEBUG, "Same VM !  ignore DynamicMacAddr conflict" );
						continue;
					}
				}

				// FIXME: need  cycle to end to make all duplicates
				duplicates += preDuplicates;
			}//for idx
		} //foreach( QString dirUuid
	}
}

void Task_RegisterVm::checkVMOnOtherServerUuid( bool *pbServerUuidWasChanged /* out */ )
{
	PRL_ASSERT( pbServerUuidWasChanged );
	*pbServerUuidWasChanged = true;

	// retrieve server UUID to check if VM is used on another server, ask user to override it.
	QString server_id
		= m_pVmConfig->getVmIdentification()->getServerUuid();
	// fix #121634, #121636 - Skip Copy/Move message when VM already registered on same server
	QString lastServer_id
		= m_pVmConfig->getVmIdentification()->getLastServerUuid();
	QString local_id
		= CDspService::instance()->getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid();

	*pbServerUuidWasChanged =
		( server_id.isEmpty() && Uuid(lastServer_id)!=Uuid(local_id) )
		|| ( !server_id.isEmpty() && Uuid(server_id) != Uuid(local_id) );

	// note: CVmIdentification::setDefaults() creates fake uuid. be sure that incoming config
	// (from VPS file or from GUI) contains right server uuid).
	if (doRegisterOnly() && !server_id.isEmpty() && Uuid(server_id) != Uuid(local_id))
	{
		bool sharedFS = CFileHelper::isSharedFS(m_pVmInfo->vmXmlPath);
		if (sharedFS &&
			!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		{
			if (getRequestFlags() & PRCF_FORCE)
			{
				m_bForceRegisterOnSharedStorage = true;
				WRITE_TRACE(DBG_FATAL, "Attention: performing forced registration of VM on "
									   "shared storage by user request.");
				WRITE_TRACE(DBG_FATAL, "This will lead to revoking all access to VM "
									   "from the original server and unregistering it from HA clusters.");
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed to register the Virtual Machine on shared storage.");
				WRITE_TRACE(DBG_FATAL, "You can force the registration, but this will revoke "
										"all access to the Virtual Mashine from the original server "
										"and HA clusters, if the VM was registered there.");
				getLastError()->setEventCode(PRL_ERR_FORCE_REG_ON_SHARED_STORAGE_IS_NEEDED);
				getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
					m_pVmConfig->getVmIdentification()->getVmName(),
					EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_FORCE_REG_ON_SHARED_STORAGE_IS_NEEDED;
			}
		}
	}

	// override server uuid with right value.
	m_pVmConfig->getVmIdentification()->setServerUuid(local_id);

	// fix #121634, #121636 - Skip Copy/Move message when VM already registered on same server
	m_pVmConfig->getVmIdentification()->setLastServerUuid();
}

PRL_RESULT Task_RegisterVm::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		/**
		 * check parameters
		 */
		if (PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		if ( !doRegisterOnly() )
		{
			//https://bugzilla.sw.ru/show_bug.cgi?id=267152
			CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

			if ( ! m_vmRootDir.isEmpty() && ! QDir::isAbsolutePath( m_vmRootDir ) )
				throw PRL_ERR_VMDIR_PATH_IS_NOT_ABSOLUTE;

			if( !m_vmRootDir.isEmpty() &&
				!CFileHelper::DirectoryExists( m_vmRootDir, &getClient()->getAuthHelper() ) )
			{
				getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
					m_vmRootDir,
					EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
			}

			if( m_vmRootDir.isEmpty() )
			{
				m_vmRootDir= CDspVmDirHelper::getVmRootPathForUser( getClient() );

				// #127473 to prevent create directory on external unmounted disk
				if( !QDir(m_vmRootDir).exists() )
				{
					WRITE_TRACE(DBG_FATAL, "Parallels VM Directory %s does not exists." , QSTR2UTF8( m_vmRootDir ) );
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, m_vmRootDir,
						EVT_PARAM_MESSAGE_PARAM_0 ) );
					throw PRL_ERR_VM_DIRECTORY_FOLDER_DOESNT_EXIST;
				}
			}

			if (m_vmRootDir.endsWith('/') || m_vmRootDir.endsWith('\\'))
				m_vmRootDir.chop(1);

			//////////////////////////////////////////////////////////////////////////
			setTaskParameters( m_pVmConfig->toString(), m_vmRootDir );

			LOG_MESSAGE( DBG_FATAL, "Need to register VM: root_dir='%s' from config: \n%s",
				QSTR2UTF8( m_vmRootDir ),
				QSTR2UTF8( m_pVmConfig->toString() )
			);
		}//if (!doRegisterOnly)

		if( PRL_FAILED( m_pVmConfig->m_uiRcInit ) )
		{
			if ( ! doRegisterOnly() )
				throw PRL_ERR_PARSE_VM_CONFIG;
			else
			{
				//#462940 restoreConfigPermissions() should NOT be called under Impersonate
				ret = tryToRestoreVmConfig( m_strPathToVmDirToRegister );
				if( PRL_FAILED(ret) )
					throw ret;
			}
		}

		QString originalVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();
		if (doRegisterOnly() && !m_sCustomVmUuid.isEmpty())
		{
			if (!Uuid::isUuid(m_sCustomVmUuid) ||
					(m_nFlags & PRVF_REGENERATE_VM_UUID) ||
					(m_nInternalParamsAsMasks & REG_SKIP_VM_PARAMS_LOCK))
			{
				WRITE_TRACE(DBG_FATAL, "Incorrect parameter for custom registration"
					" uuid=%s flags=%x InternalParamsAsMasks=%x",
					QSTR2UTF8(m_sCustomVmUuid), m_nFlags, m_nInternalParamsAsMasks);
				throw PRL_ERR_INVALID_ARG;
			}
			m_pVmConfig->getVmIdentification()->setVmUuid(m_sCustomVmUuid);
		}

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		// #116916 MAC address may be empty;
		createMACAddress(m_pVmConfig, QSet<QString>(), true);

		// retrieve VM UUID from VM config
		QString vm_uuid = m_pVmConfig->getVmIdentification()->getVmUuid();

		// recreate vm_uuid ONLY if it is incorrect
		// NOTE: regenerate vm_uuid shouldn't located in this place
		//		to prevent skipping error PRL_ERR_VM_ALREADY_REGISTERED
		//		( and already register same vm ( #438525, #436867 ) )
		if( vm_uuid.isEmpty() || Uuid( vm_uuid ).isNull() )
			vm_uuid = regenerateVmUuid();

		if (!Uuid(originalVmUuid).isNull() && ((vm_uuid != originalVmUuid) || (m_nFlags & PRVF_REGENERATE_VM_UUID)))
		{
			::Backup::Device::Service(m_pVmConfig).teardown();
			::Backup::Device::Dao(m_pVmConfig).deleteAll();
		}
		//On VM registering check whether source VM UUID field empty and fill it properly
		if ( doRegisterOnly() &&
				(m_pVmConfig->getVmIdentification()->getSourceVmUuid().isEmpty() ||
					Uuid( m_pVmConfig->getVmIdentification()->getSourceVmUuid() ).isNull()) )
		{
			if (!vm_uuid.isEmpty() && !Uuid( vm_uuid ).isNull())
				m_pVmConfig->getVmIdentification()->setSourceVmUuid(vm_uuid);
		}

		//Store source VM UUID if new VM creating
		if (!doRegisterOnly())
			m_pVmConfig->getVmIdentification()->setSourceVmUuid(vm_uuid);

		QString vm_name= m_pVmConfig->getVmIdentification()->getVmName();
		if( vm_name.isEmpty())
		{
			vm_name = PVS_GUEST_TO_STRING(
				m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion());

			m_pVmConfig->getVmIdentification()->setVmName( vm_name );
			m_bVmRegisterNameWasEmpty = true;
		}

		QString sNewVmDirectoryPath = m_strPathToVmDirToRegister;

		////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////
		QString vm_xml_path;
		QString vm_root_dir;

		if ( doRegisterOnly() )
		{
			vm_xml_path = QString( "%1/%2" )
				.arg( sNewVmDirectoryPath )
				.arg( VMDIR_DEFAULT_VM_CONFIG_FILE );

			vm_root_dir = QFileInfo(sNewVmDirectoryPath).path();

			// Vm directory name MUST NOT be renamed during registration ( see #6281 )
		}
		else
		{
			vm_xml_path = QString( "%1/%2/%3" )
				.arg( m_vmRootDir )
				.arg(Vm::Config::getVmHomeDirName(vm_uuid))
				.arg( VMDIR_DEFAULT_VM_CONFIG_FILE );

			vm_root_dir = m_vmRootDir;
		}

		if( CFileHelper::isRemotePath(vm_root_dir) )
		{
			if( ! CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->isAllowUseNetworkShares() )
			{
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, vm_root_dir, EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_UNSUPPORTED_NETWORK_FILE_SYSTEM;
			}
		}

		/**
		 * check if such VM is not already registered in user's prepare_VM directory
		 */
		m_pVmInfo =  new CVmDirectory::TemporaryCatalogueItem( vm_uuid, vm_xml_path, vm_name );

		m_flgLockRegistred=false;
		PRL_RESULT lockResult = PRL_ERR_SUCCESS;
		if ( !(m_nInternalParamsAsMasks & REG_SKIP_VM_PARAMS_LOCK))
		{
			QList<PRL_RESULT> lockErrorsList;
			lockResult = CDspService::instance()->getVmDirManager()
				.checkAndLockNotExistsExclusiveVmParameters(
						getClient()->getVmDirectoryUuidList(),
						m_pVmInfo,
						&lockErrorsList);

		// this while only for registration case.
		if ( doRegisterOnly() )
		{
			// VM already registered in VmDirectory
			if ( !m_sCustomVmUuid.isEmpty() && lockErrorsList.contains(PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH) )
			{
				getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, vm_name, EVT_PARAM_MESSAGE_PARAM_0));
				getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, vm_root_dir, EVT_PARAM_MESSAGE_PARAM_1));
				throw PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH;
			}

			QString qsVmDirName = QFileInfo(sNewVmDirectoryPath).fileName();
			bool bVmUuidAndVmDirAreDifferent = qsVmDirName !=
				Vm::Config::getVmHomeDirName(m_pVmInfo->vmUuid);

			QString qsGenVmDirName = getUniqueVmName(qsVmDirName, m_dirUuid);
			if( qsGenVmDirName != qsVmDirName
				&& (PRL_SUCCEEDED(lockResult) || lockErrorsList.contains(PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME) )
				)
			{
				if ( PRL_SUCCEEDED(lockResult) )
				{
					CDspService::instance()->getVmDirManager()
						.unlockExclusiveVmParameters( m_pVmInfo );
				}
				lockResult = PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS;
				lockErrorsList << PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS;
				qsVmDirName = qsGenVmDirName;
			}

			while(   PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME == lockResult
				  || PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH == lockResult
				  || PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS == lockResult
				  || PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID == lockResult)
			{
				if (PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID == lockResult)
				{
					if (!m_sCustomVmUuid.isEmpty())
						break;
					m_pVmInfo->vmUuid = regenerateVmUuid();
					lockErrorsList.removeAll( PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID );
				}

				if(lockErrorsList.count()>0)
				{
					//change vm name to resolve conflict on register vm
					qsVmDirName = getUniqueVmName(qsVmDirName, m_dirUuid);
					m_pVmInfo->vmName = getUniqueVmName(m_pVmInfo->vmName, m_dirUuid);
					m_pVmConfig->getVmIdentification()->setVmName( m_pVmInfo->vmName );

					if (   PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH == lockResult
						|| PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS == lockResult)
					{
						if (bVmUuidAndVmDirAreDifferent
							|| PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS == lockResult)
						{
							m_pVmInfo->vmXmlPath = QString( "%1/%2/%3" )
								.arg( vm_root_dir )
								.arg( qsVmDirName )
								.arg( VMDIR_DEFAULT_VM_CONFIG_FILE );
						}
						else
						{
							m_pVmInfo->vmXmlPath = QString( "%1/%2/%3" )
								.arg( vm_root_dir )
								.arg(Vm::Config::getVmHomeDirName(m_pVmInfo->vmUuid))
								.arg( VMDIR_DEFAULT_VM_CONFIG_FILE );
						}
						m_pVmConfig->getVmIdentification()->setHomePath(m_pVmInfo->vmXmlPath);
					}
				}

				lockResult = CDspService::instance()->getVmDirManager()
					.checkAndLockNotExistsExclusiveVmParameters(
					getClient()->getVmDirectoryUuidList(),
					m_pVmInfo,
					&lockErrorsList);

			}//while
		}
		}

		////////////////////////////////////////////
		//
		// Auto selection unigue VM NAME on Create VM
		//   was disabled by bug #267227
		// https://bugzilla.sw.ru/show_bug.cgi?id=267227
		//
		////////////////////////////////////////////

		if( PRL_FAILED( lockResult) )
		{
			switch ( lockResult )
			{
			case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, vm_name, EVT_PARAM_MESSAGE_PARAM_0));
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, vm_root_dir, EVT_PARAM_MESSAGE_PARAM_1));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

			default:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
			}//switch

			throw lockResult;
		}

		m_flgLockRegistred=true;

		bool bServerUuidWasChanged = true;
		checkVMOnOtherServerUuid( &bServerUuidWasChanged );
		checkOperationPermission();
		checkWhereFromRegisteredVm();
		checkDynamicMACAddress( );
		validateNewConfiguration();
		patchNewConfiguration();
		if( m_nFlags & PRVF_REGENERATE_VM_UUID )
			regenerateVmUuid();
		if( m_nFlags & PRVF_REGENERATE_SRC_VM_UUID )
			regenerateSrcVmUuid();

		// validate config for critical errors


		// clear suspend parameter for disks!
		CStatesHelper::SetSuspendParameterForAllDisks(m_pVmConfig.getImpl(),0);
		//https://jira.sw.ru/browse/PSBM-5293
		//Patch network adapters with empty virtual network IDs fields
		PatchNetworkAdapters();

		// DEBUG LOGS FOR #PSBM-44712
		if (m_pVmConfig->getVmSettings() != NULL && m_pVmConfig->getVmSettings()->getVmCommonOptions() != NULL)
			WRITE_TRACE(DBG_DEBUG, "#PSBM-44712 OsType: %d, OsVersion: %d",
				m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType(),
				m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion());
		else
			WRITE_TRACE(DBG_DEBUG, "#PSBM-44712 NO OS INFO");

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		getLastError()->setEventCode( code );
		ret = code;

		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

void Task_RegisterVm::PatchNetworkAdapters()
{
	CDspLockedPointer<CParallelsNetworkConfig>
		pNetworkConfig = CDspService::instance()->getNetworkConfig();
	CVirtualNetwork *pHostOnlyNet = PrlNet::GetHostOnlyNetwork(
					pNetworkConfig.getPtr(), PRL_DEFAULT_HOSTONLY_INDEX );
	CVirtualNetwork *pBridgedNet = PrlNet::GetBridgedNetwork( pNetworkConfig.getPtr() );
	foreach(CVmGenericNetworkAdapter *pNetAdapter, m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		if (pNetAdapter->getHostInterfaceName().isEmpty())
			pNetAdapter->setHostInterfaceName(HostUtils::generateHostInterfaceName(pNetAdapter->getMacAddress()));

		QString sNetworkId = pNetAdapter->getVirtualNetworkID();
		if ( ! sNetworkId.isEmpty() )//Virtual network ID already configured - skip
			continue;
		if ( pNetAdapter->isVtdDevice() )//Skip VT-d devices
			continue;
		switch ( pNetAdapter->getEmulatedType() )
		{
			case PNA_SHARED:
			case PNA_HOST_ONLY:
				if ( pHostOnlyNet )
					pNetAdapter->setVirtualNetworkID( pHostOnlyNet->getNetworkID() );
				else
					pNetAdapter->setConnected( PVE::DeviceDisconnected );
			break;

			case PNA_BRIDGED_NETWORK:
				if ( pBridgedNet )
					pNetAdapter->setVirtualNetworkID( pBridgedNet->getNetworkID() );
				else
					pNetAdapter->setConnected( PVE::DeviceDisconnected );
			break;

			default: continue;
		}
	}
}

void Task_RegisterVm::finalizeTask()
{
	//!!! NOTE: FIXME !!!!
	//
	// need add rollback(remove all devices in vm directory and vm directory)
	//
	///////////////////////////////////////////////
	bool bHaClusterResourceAdded = false;

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

		if( operationIsCancelled() )
			throw getCancelResult();

		setTaskCompleted();

		PRL_ASSERT (m_pVmInfo);
		/**
		 * create new VM Directory item
		 */

		CVmDirectoryItem*  pVmDirItem=new CVmDirectoryItem();
		QString userName;
		{
			CDspLockedPointer<CDispUser> dispUser = CDspService::instance()->getDispConfigGuard()
				.getDispUserByUuid( getClient()->getUserSettingsUuid() );
			PRL_ASSERT( dispUser );
			if( dispUser )
				userName = dispUser->getUserName();
		}

		pVmDirItem->setVmUuid( m_pVmInfo->vmUuid );
		pVmDirItem->setRegistered( PVE::VmRegistered );
		pVmDirItem->setValid( PVE::VmValid );
		pVmDirItem->setVmHome( m_pVmInfo->vmXmlPath );
		pVmDirItem->setVmName( m_pVmInfo->vmName );
		pVmDirItem->setTemplate( m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );

		// FIXME set private according to actual flag value within VM config
		pVmDirItem->setIsPrivate( PVE::VmPublic );


		pVmDirItem->setRegisteredBy( userName );
		pVmDirItem->setRegDateTime(m_pVmConfig->getVmIdentification()->getVmUptimeStartDateTime());
		pVmDirItem->setChangedBy( userName );

		pVmDirItem->setChangeDateTime(m_pVmConfig->getVmIdentification()->getVmUptimeStartDateTime());

		if (
			!(getRequestFlags() & PRVF_IGNORE_HA_CLUSTER) &&
			!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		{
			ret = CDspService::instance()->getHaClusterHelper()->addClusterResource(
					m_pVmInfo->vmName,
					m_pVmConfig->getVmSettings()->getHighAvailability(),
					CFileHelper::GetFileRoot(m_pVmInfo->vmXmlPath));
			if(PRL_FAILED(ret))
				throw ret;
			bHaClusterResourceAdded = true;
		}

		//
		// insert new item in user's VM Directory
		//
		PRL_RESULT insertRes = CDspService::instance()->getVmDirHelper().insertVmDirectoryItem(
						m_dirUuid, pVmDirItem );
		if( ! PRL_SUCCEEDED( insertRes ) )
		{
			WRITE_TRACE(DBG_FATAL, ">>> Can't insert vm to VmDirectory by error %#x, %s",
				insertRes, PRL_RESULT_TO_STRING( insertRes ) );

			throw insertRes;
		}

		// #441667 - set the same parameters for shared vm
		if( doRegisterOnly() )
		{
			CDspLockedPointer<CVmDirectoryItem>
				pAddedItem = CDspService::instance()->getVmDirManager()
					.getVmDirItemByUuid(m_dirUuid, pVmDirItem->getVmUuid() );

			CDspVmDirManager::VmDirItemsHash
				sharedVmHash = CDspService::instance()->getVmDirManager()
					.findVmDirItemsInCatalogue( pVmDirItem->getVmUuid(), pVmDirItem->getVmHome() );

			// delete added vm
			sharedVmHash.remove(m_dirUuid);
			if( !sharedVmHash.empty() )
			{
				WRITE_TRACE( DBG_WARNING, "Its shared vm. We will copy shared properties." );
				CDspLockedPointer<CVmDirectoryItem> pVmDirSharedItem = sharedVmHash.begin().value();
				pAddedItem->setLockedSign( pVmDirSharedItem->isLockedSign() );
				pAddedItem->getLockedOperationsList()
					->setLockedOperations( pVmDirSharedItem->getLockedOperationsList()->getLockedOperations() );

				PRL_RESULT ret = CDspService::instance()->getVmDirManager().updateVmDirItem( pAddedItem );
				if ( PRL_FAILED( ret ) )
				{
					WRITE_TRACE(DBG_FATAL, "Can't update VmCatalogue for shared Vm by error %#x, %s"
						, ret, PRL_RESULT_TO_STRING( ret ) );
					// note - don't throw because this Vm was added.
				}
				pAddedItem->getLockDown()->setEditingPasswordHash(
							pVmDirSharedItem->getLockDown()->getEditingPasswordHash()
						);
			}// if( !sharedVmHash.empty() )
		}//if( doRegisterOnly() )

		WRITE_TRACE( DBG_FATAL, "VM '%s' successfully added to system. ( uuid = %s / path = '%s' )"
			, QSTR2UTF8( m_pVmConfig->getVmIdentification()->getVmName() )
			, QSTR2UTF8( m_pVmConfig->getVmIdentification()->getVmUuid() )
			, !m_pVmInfo ? "" : QSTR2UTF8( m_pVmInfo->vmXmlPath )
		);

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		if (bHaClusterResourceAdded)
			CDspService::instance()->getHaClusterHelper()->removeClusterResource(m_pVmInfo->vmName);
		rollback();
		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);

	// delete temporary registration
	if (m_pVmInfo && m_flgLockRegistred)
	{
		CDspService::instance()->getVmDirManager()
			.unlockExclusiveVmParameters( m_pVmInfo );
	}

	// send response
	if (PRL_FAILED(getLastErrorCode()))
	{
		return (void)getClient()->sendResponseError
			(getLastError(), getRequestPackage());
	}
	// #PDFM-27714: Workaround to avoid dispatcher crash

	/**
	 * Notify all user: vm directory changed
	 */
	if(!doRegisterOnly())
	{
		CDspService::instance()->getVmStateSender()
			->onVmCreated(m_dirUuid, m_pVmInfo->vmUuid);
	}
	QString n;
	if (m_bVmRegisterNameWasEmpty)
		n = m_pVmInfo->vmName;

	CDspService::instance()->getVmStateSender()
		->onVmRegistered(m_dirUuid, m_pVmInfo->vmUuid, n);

	// send response
	CProtoCommandPtr pCmd
		= CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse*
		pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration(m_pVmConfig->toString()));
	CDspService::instance()->getVmDirHelper().appendAdvancedParamsToVmConfig(getClient(), pVmConfig);
	pResponseCmd->SetVmConfig( pVmConfig->toString() );

	getClient()->sendResponse( pCmd, getRequestPackage() );
}

PRL_RESULT Task_RegisterVm::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	bool flgImpersonated = false;
	try
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152

		// Let current thread impersonate the security context of a logged-on user
		if( ! getClient()->getAuthHelper().Impersonate() )
		{
			getLastError()->setEventCode(PRL_ERR_IMPERSONATE_FAILED);
			throw PRL_ERR_IMPERSONATE_FAILED;
		}
		flgImpersonated = true;


		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();
		PRL_ASSERT(m_pVmInfo);

		/**
		 * if VM is not registered - go register it
		 */

		/// Check vm_home (path is valid and accessible, no file by name exists)

		bool		flgPathExist = CFileHelper::FileExists(m_pVmInfo->vmXmlPath, &getClient()->getAuthHelper());
		QString	strVmHomeDir = CFileHelper::GetFileRoot(m_pVmInfo->vmXmlPath);
		QString strVmHomePath = m_pVmInfo->vmXmlPath;

		if ( doRegisterOnly() )
		{
			if ( ! flgPathExist )
			{
				if ( CFileHelper::GetFileRoot(m_pVmInfo->vmXmlPath) != m_strPathToVmDirToRegister )
				{
					QString newVmHome = CFileHelper::GetFileRoot(m_pVmInfo->vmXmlPath);

					if ( CFileHelper::FileExists(newVmHome, &getClient()->getAuthHelper()) )
					{
						getLastError()->setEventCode(PRL_ERR_FILE_OR_DIR_ALREADY_EXISTS);
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, m_pVmConfig->getVmIdentification()->getVmName(),
							EVT_PARAM_MESSAGE_PARAM_0));
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, newVmHome,
							EVT_PARAM_MESSAGE_PARAM_1));

						throw PRL_ERR_FILE_OR_DIR_ALREADY_EXISTS;
					}
					else if ( !CFileHelper::RenameEntry(m_strPathToVmDirToRegister,
                                            newVmHome,
                                            &getClient()->getAuthHelper()) )
					{
						getLastError()->setEventCode( PRL_ERR_CANT_RENAME_DIR_ENTRY );
						getLastError()->addEventParameter(
							new CVmEventParameter( PVE::String, m_strPathToVmDirToRegister,
													EVT_PARAM_MESSAGE_PARAM_0));
						throw PRL_ERR_CANT_RENAME_DIR_ENTRY;
					}

					Task_EditVm::CorrectDevicePathsInVmConfig(m_pVmConfig, m_strPathToVmDirToRegister, newVmHome);
					m_strRenamedVmHome = newVmHome;
				}
				else
				{
					getLastError()->setEventCode( PRL_ERR_VM_CONFIG_DOESNT_EXIST );
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,
						m_pVmConfig->getVmIdentification()->getVmName(),
	                    EVT_PARAM_MESSAGE_PARAM_0 ) );
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String,
						strVmHomePath,
	                    EVT_PARAM_MESSAGE_PARAM_1 ) );

					throw PRL_ERR_VM_CONFIG_DOESNT_EXIST;
				}
			}
		}
		else // ! doRegisterOnly()
		{
			if ( flgPathExist )
			{
				getLastError()->setEventCode(PRL_ERR_VM_CONFIG_ALREADY_EXISTS);
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath,
                                EVT_PARAM_RETURN_PARAM_TOKEN));
				throw PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS;
			}

			// Check directory
			if (!CFileHelper::DirectoryExists(strVmHomeDir, &getClient()->getAuthHelper()))
			{
				if (!CFileHelper::WriteDirectory(strVmHomeDir, &getClient()->getAuthHelper()))
				{
					getLastError()->setEventCode(PRL_ERR_USER_NO_AUTH_TO_CREATE_VM_IN_DIR);
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, strVmHomeDir,
                                EVT_PARAM_MESSAGE_PARAM_0));
					throw PRL_ERR_USER_NO_AUTH_TO_CREATE_VM_IN_DIR;
				}
				else
					m_lstCreatedDirs.append( strVmHomeDir );
			}
		} //! doRegisterOnly()

		if (m_bForceRegisterOnSharedStorage)
		{
			PRL_FILE_SYSTEM_FS_TYPE fstype = HostUtils::GetFSType(m_strPathToVmDirToRegister);
			if (fstype == PRL_FS_FUSE)
			{
				// revoke leases on pstorage
				ret = CDspService::instance()->getHaClusterHelper()->revokeLeases(m_strPathToVmDirToRegister);
				if ( PRL_FAILED( ret ) )
					throw ret;
			}
			// remove VM from cluster for all shared FS types
			ret = CDspService::instance()->getHaClusterHelper()->removeClusterResource(
					m_pVmConfig->getVmIdentification()->getVmName(), true);
			if ( PRL_FAILED( ret ) )
				throw ret;
		}

		// Update Shared Windows Guest Applications bundles (set new uids)
		////////////////////////////////////////////////////////////////////////////
		updateSharedWindowsApplications();

		// Forcebly set performance optimization for PS
		m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->setOptimizePowerConsumptionMode(PVE::OptimizePerformance);

		// FIXME validate VM configuration here
		////////////////////////////////////////////////////////////////////////////


		// create/check VM configuration items
		////////////////////////////////////////////////////////////////////////////
		LOG_MESSAGE( DBG_FATAL,"##########  Try to create/check configuration...  ##########");

		QString sOldVmHomePathValue =  m_pVmConfig->getVmIdentification()->getHomePath();
		//Passing path to VM dir - it's necessary for CreateImage task for relative
		//devices pathes convertion
		m_pVmConfig->getVmIdentification()->setHomePath( strVmHomePath );

		ret = createConfigItems();
		//Restore home path value now in previous state
		m_pVmConfig->getVmIdentification()->setHomePath( sOldVmHomePathValue );

		if ( ! PRL_SUCCEEDED( ret ) )
			throw ret;

		quint32 t(CDspService::instance()->getDispConfigGuard().getDispConfig()
					->getDispatcherSettings()->getCommonPreferences()
					->getWorkspacePreferences()->getVmGuestCpuLimitType());

		m_pVmConfig->getVmHardwareList()->getCpu()->setGuestLimitType(t);

		LOG_MESSAGE( DBG_FATAL,"##########  Configuration created/checked. ##########");

		//Reset VM uptime counter values
		//https://bugzilla.sw.ru/show_bug.cgi?id=464813
		m_pVmConfig->getVmIdentification()->setVmUptimeInSeconds();
		m_pVmConfig->getVmIdentification()->setVmUptimeStartDateTime();
		// #PDFM-20418
		if(!doRegisterOnly())
		{
			m_pVmConfig->getVmIdentification()->setCreationDate
				(m_pVmConfig->getVmIdentification()->getVmUptimeStartDateTime());
		}
		// always patch old config to enable shared folsers #465074
		m_pVmConfig->getVmSettings()->getVmTools()->getVmSharing()->
						getHostSharing()->setUserDefinedFoldersEnabled(true);

		// #459056, #...
		// We do not set permission to Vm, which already register in system in another VmDirectory.
		// ( to prevent lost this Vm by user1 )
		// ( We allow to register already registered Vm because this user( user2 ) has RW permission to Vm )

		// check: is shared Vm ?
		bool bSkipToSetPermission = true;
		if( doRegisterOnly() )
		{
			CDspVmDirManager::VmDirItemsHash sharedVmHash = CDspService::instance()->getVmDirManager()
				.findVmDirItemsInCatalogue( m_pVmInfo->vmUuid, m_pVmInfo->vmXmlPath );
			if( !sharedVmHash.isEmpty() )
			{
				bSkipToSetPermission = true;
				WRITE_TRACE( DBG_FATAL,
					"This Vm is already registered in the system but under different accounts."
					" SO WE SKIP REASSIGN OWNER AND PERMISSIONS TO VM."
					"\nVmUuid = %s, VmHome='%s'"
					"\nVmDirectories with this vm: %s "
					, QSTR2UTF8( m_pVmInfo->vmUuid )
					, QSTR2UTF8( m_pVmInfo->vmXmlPath )
					, QSTR2UTF8( QStringList( sharedVmHash.keys() ).join( ", ") )
					);
			}
		}

		// Terminates the impersonation of a user, return to Dispatcher access rights
		if( ! getActualClient()->getAuthHelper().RevertToSelf() ) // Don't throw because this thread already finished.
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );
		else
			flgImpersonated = false;

		// Remove bug patcher logic as useless in OpenDisp
		//#PDFM-26405 should be before called saveConfig because this patch changes
		//CDspBugPatcherLogic logic( *CDspService::instance()->getHostInfo()->data() );
		//
		//CVmDirectoryItem fakeDirItem;
		//fakeDirItem.setVmUuid( m_pVmInfo->vmUuid );
		//fakeDirItem.setVmHome( m_pVmInfo->vmXmlPath );
		//
		//logic.patchVmBootCamp( m_pVmConfig, &fakeDirItem );

		////////////////////////////////////////////////////////////////////////////
		// save VM configuration
		// NOTE: It need as SYSTEM user ( after revertToSelf ) because saveVmConfig() sets permissions.
		////////////////////////////////////////////////////////////////////////////

		if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		{
			QString s(QFileInfo(CFileHelper::GetFileRoot(m_pVmInfo->vmXmlPath), VM_PERSONALITY_DIR).filePath());
			if (!CFileHelper::WriteDirectory(s, &getClient()->getAuthHelper()))
				throw CDspTaskFailure(*this)(PRL_ERR_DISK_DIR_CREATE_ERROR, s);
			m_lstCreatedDirs.append(s);
		}
		CCpuHelper::update(*m_pVmConfig);

		ret = saveVmConfig( );
		if( PRL_FAILED( ret ) )
			throw ret;

		////////////////////////////////////////////////////////////////////////////
		// Set default permissions to vm files
		// NOTE: It need as SYSTEM user ( after revertToSelf )
		////////////////////////////////////////////////////////////////////////////
		ret = PRL_ERR_SUCCESS;
		if( !bSkipToSetPermission)
		{
			bool bKeepOthersPermissions = false;
			if (doRegisterOnly())
			{
				bKeepOthersPermissions = ((getRequestFlags() & PRVF_KEEP_OTHERS_PERMISSIONS) != 0);
			}
// https://jira.sw.ru/browse/PWE-3795
// under windows we do not manipulate of ACLs, we follow the OS security policy
#ifdef _WIN_
			if ( !doRegisterOnly() )
#endif
				ret = setDefaultVmPermissions(getClient(), m_pVmInfo->vmXmlPath, bKeepOthersPermissions);
		}
		if( PRL_FAILED( ret ) )
		{
			if (ret == PRL_ERR_CANT_CHANGE_OWNER_OF_FILE)
			{
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, strVmHomeDir,
					EVT_PARAM_MESSAGE_PARAM_0));

				throw PRL_ERR_CANT_CHANGE_OWNER_OF_VM_FILE;
			}
			if (ret == PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS)
			{
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName,
					EVT_PARAM_MESSAGE_PARAM_0));
			}
			throw ret;
		}

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		setLastErrorCode( ret );

		// Terminates the impersonation of a user, return to Dispatcher access rights
		if( flgImpersonated && ! getActualClient()->getAuthHelper().RevertToSelf() )
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

void Task_RegisterVm::updateSharedWindowsApplications()
{
}


PRL_RESULT	Task_RegisterVm::createConfigItems( )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		// Create floppy disk
		ret = createFloppyDisks();
		if ( PRL_FAILED(ret) )
			throw ret;

		// Create serial ports output files
		ret = createSerialPorts();
		if ( PRL_FAILED(ret) )
			throw ret;

		// Create parallel ports output files
		ret = createParallelPorts();
		if ( PRL_FAILED(ret) )
			throw ret;

		if(operationIsCancelled())
			throw getCancelResult();

		// Create hard disks
		ret = createHardDisks();
		if ( PRL_FAILED(ret) )
			throw ret;

		ret = createNVRAM();
		if ( PRL_FAILED(ret) )
			throw ret;
	}
	catch ( PRL_RESULT code )
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code ) );
	}
	return ret;
}

PRL_RESULT Task_RegisterVm::saveVmConfig( )
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	QString strVmDirPath;
	{	// get correct path for network storages
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
		strVmDirPath = CFileHelper::GetFileRoot(m_pVmInfo->vmXmlPath);
	}
	do
	{
		/**
		 * reset additional parameters in VM configuration
		 */
		CDspService::instance()->getVmDirHelper().resetAdvancedParamsFromVmConfig( m_pVmConfig );

		//Fill server UUID field because it information needing at VM check access procedure
		m_pVmConfig->getVmIdentification()->setServerUuid(
			CDspService::instance()->getDispConfigGuard().getDispConfig()->
				getVmServerIdentification()->getServerUuid()
		);
		m_pVmConfig->getVmIdentification()->setHomePath(m_pVmInfo->vmXmlPath);
		ret = CDspService::instance()->getVmConfigManager()
				.saveConfig(m_pVmConfig, m_pVmInfo->vmXmlPath, getClient(), true, true);
		if (IS_OPERATION_SUCCEEDED(ret))
		{
			if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
				break;
#ifdef _LIBVIRT_
			m_registry.declare(CVmIdent(getVmUuid(), getClient()->getVmDirectoryUuid()),
				m_pVmInfo->vmXmlPath);
			Libvirt::Result r(Command::Vm::Gear<Command::Tag::State
				<Command::Vm::Registrator, Command::Vm::Fork::State::Plural
					<boost::mpl::vector_c<unsigned, VMS_STOPPED, VMS_SUSPENDED> > > >::run(*m_pVmConfig));
			ret = (r.isFailed()? r.error().code(): PRL_ERR_SUCCESS);
#endif // _LIBVIRT_
			break;
		}

		WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher unable to save configuration of the VM %s to file %s. Reason: %ld: %s",
			QSTR2UTF8(m_pVmInfo->vmName),
			QSTR2UTF8(m_pVmConfig->getOutFileName()),
			Prl::GetLastError(),
			QSTR2UTF8(Prl::GetLastErrorAsString())
		);

		// check error code - it may be not free space for save config
		if (PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_XML_SAVE == ret)
			break;

		// send error to user: can't save VM config to file
		CDspTaskFailure f(*this);
		ret = f.setCode(PRL_ERR_SAVE_VM_CONFIG)(m_pVmInfo->vmName, strVmDirPath);	
	} while (false);

	if (PRL_FAILED(ret))
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING(ret));
	}
	return ret;
}

QString Task_RegisterVm::ConvertToFullPath(const QString &sImagePath)
{
	if (!QFileInfo(sImagePath).isAbsolute())
	{
		QFileInfo _fi(QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath() + '/' + sImagePath);
		return (_fi.absoluteFilePath());
	}

	return (sImagePath);
}

/**
* Check and create floppy disk images
* @return Returns error code.
*/
PRL_RESULT	Task_RegisterVm::createFloppyDisks()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	QList<CVmFloppyDisk*> &lstFloppyDisks = m_pVmConfig->getVmHardwareList()->m_lstFloppyDisks;
	QList<CVmFloppyDisk*>::iterator it_fdd;

	int fddNum = 0;
	for ( it_fdd = lstFloppyDisks.begin(); it_fdd != lstFloppyDisks.end(); ++it_fdd, fddNum++ )
	{
		CVmFloppyDisk *pFloppyDisk = *it_fdd;

		// Validate Floppy Disk full path
		QString strFullPath = ConvertToFullPath(pFloppyDisk->getUserFriendlyName());

		if ( (PRL_VM_DEV_EMULATION_TYPE)pFloppyDisk->getEmulatedType() == PDT_USE_IMAGE_FILE )
		{
			// Check disk image existence if we register VM only
			if ( doRegisterOnly() )
				continue;

			// we have only base name ParallelsDirs::getFddToolsImageBaseName( );
			// replace  it on server side to correct installation os\2 oses
			if ( m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType() == PVS_GUEST_TYPE_OS2
				 && strFullPath.contains(ParallelsDirs::getFddToolsImageBaseName(PVS_GUEST_TYPE_OS2))
				)
			{
				// get exec mode
				strFullPath =
					ParallelsDirs::getFddToolsImage( ParallelsDirs::getAppExecuteMode(), PVS_GUEST_TYPE_OS2 );
				pFloppyDisk->setSystemName( strFullPath );
				pFloppyDisk->setUserFriendlyName( strFullPath );

				if( !QFile::exists(strFullPath) )
				{
					WRITE_TRACE(DBG_FATAL, "Required fdd image '%s' does not found, try to send question to user"
						, QSTR2UTF8( strFullPath )
						);
				}

				// continue in any case because this parallels image and it shouldn't created.
				continue;
			}

			if (QFile::exists(strFullPath))//File exists so it's not necessary to create it
				continue;

			//////////////////////////////////
			// Create new floppy image
			//////////////////////////////////

			QString floppyXMLstr = ElementToString<CVmFloppyDisk*>(
				pFloppyDisk, XML_VM_CONFIG_EL_FLOPPY_DISK );

			SmartPtr<CDspClient> dspClient = getClient();
			SmartPtr<IOService::IOPackage> requestPackage = getRequestPackage();
			Task_CreateImage taskCreateImage(
				dspClient, requestPackage, m_pVmConfig, floppyXMLstr, false, true);

			m_lpcCreateImageCurrentTask = &taskCreateImage;
			PRL_RESULT ret = taskCreateImage.run_body();
			m_mtxWaitCreateImage.lock();
			m_lpcCreateImageCurrentTask = NULL;
			m_mtxWaitCreateImage.unlock();

			if ( PRL_SUCCEEDED(ret) )
				m_lstCreatedFiles.append( strFullPath );
			else
			{
				getLastError()->setEventCode( PRL_ERR_CANT_CREATE_FLOPPY_IMAGE );
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, strFullPath, EVT_PARAM_MESSAGE_PARAM_0));
				return PRL_ERR_CANT_CREATE_FLOPPY_IMAGE;
			}
		}
		else
		{
			// Use real device
		}
	}
	return ret;
}

/**
* Check and create serial port output files
* @return Returns error code.
*/
PRL_RESULT	Task_RegisterVm::createSerialPorts()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	QList<CVmSerialPort*> &lstSerialPorts = m_pVmConfig->getVmHardwareList()->m_lstSerialPorts;
	QList<CVmSerialPort*>::iterator it_com;

	for ( it_com = lstSerialPorts.begin(); it_com != lstSerialPorts.end(); ++it_com )
	{
		CVmSerialPort *pSerialPort = *it_com;

		// Validate serial ports output file path
		QString strFullPath = ConvertToFullPath(pSerialPort->getUserFriendlyName());

		if ( (PRL_VM_DEV_EMULATION_TYPE)pSerialPort->getEmulatedType() == PDT_USE_OUTPUT_FILE )
		{
			// We musn't to break registration of VM if a port output file does not exist
			// It will check and creation during starting VM
			if ( doRegisterOnly() )
				continue;

			if (QFile::exists(strFullPath))//File exists so it's not necessary to create it
				continue;

			///////////////////////////////////////
			// Create new serial port output file
			///////////////////////////////////////

			// Check directory
			QString strDir = CFileHelper::GetFileRoot( strFullPath );

			if ( !CFileHelper::DirectoryExists(strDir, &getClient()->getAuthHelper()) )
			{
				if ( !CFileHelper::WriteDirectory(strDir, &getClient()->getAuthHelper()) )
				{
					getLastError()->setEventCode( PRL_ERR_MAKE_DIRECTORY );
					getLastError()->addEventParameter(
						new CVmEventParameter(PVE::String, strDir, EVT_PARAM_MESSAGE_PARAM_0) );
					return PRL_ERR_MAKE_DIRECTORY;
				}
			}
			// Try to create blank file
			if ( !CFileHelper::CreateBlankFile(strFullPath, &getClient()->getAuthHelper()) )
			{
				getLastError()->setEventCode( PRL_ERR_ACCES_DENIED_FILE_TO_PARENT_PARENT_DIR );
				getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String, strFullPath, EVT_PARAM_MESSAGE_PARAM_0) );
				return PRL_ERR_ACCES_DENIED_FILE_TO_PARENT_PARENT_DIR;
			}
			else
				m_lstCreatedFiles.append( strFullPath );
		}
		else
		{
			// Use real device or PIPE
		}
	}
	return ret;
}

/**
* Check and create parallel port output files
* @return Returns error code.
*/
PRL_RESULT	Task_RegisterVm::createParallelPorts()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	QList<CVmParallelPort*> &lstParallelPorts = m_pVmConfig->getVmHardwareList()->m_lstParallelPorts;
	QList<CVmParallelPort*>::iterator it_lpt;

	for ( it_lpt = lstParallelPorts.begin(); it_lpt != lstParallelPorts.end(); ++it_lpt )
	{
		CVmParallelPort *pParallelPort = *it_lpt;

		// Validate parallel ports output file path
		QString strFullPath = ConvertToFullPath(pParallelPort->getUserFriendlyName());

		if ( (PRL_VM_DEV_EMULATION_TYPE)pParallelPort->getEmulatedType() == PDT_USE_OUTPUT_FILE )
		{
			// We musn't to break registration of VM if a port output file does not exist
			// It will check and creation during starting VM
			if ( doRegisterOnly() )
				continue;

			if (QFile::exists(strFullPath))//File exists so it's not necessary to create it
				continue;

			///////////////////////////////////////
			// Create new parallel port output file
			///////////////////////////////////////

			// Check directory
			QString strDir = CFileHelper::GetFileRoot( strFullPath );

			if ( !CFileHelper::DirectoryExists(strDir, &getClient()->getAuthHelper()) )
			{
				if ( !CFileHelper::WriteDirectory(strDir, &getClient()->getAuthHelper()) )
				{
					getLastError()->setEventCode(PRL_ERR_MAKE_DIRECTORY);
					getLastError()->addEventParameter(
						new CVmEventParameter(PVE::String, strDir,	EVT_PARAM_MESSAGE_PARAM_0) );
					return PRL_ERR_MAKE_DIRECTORY;
				}
			}
			// Try to create blank file
			if ( !CFileHelper::CreateBlankFile(strFullPath, &getClient()->getAuthHelper()) )
			{
				getLastError()->setEventCode( PRL_ERR_CANT_CREATE_PARALLEL_PORT_IMAGE );
				getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String, strFullPath, EVT_PARAM_MESSAGE_PARAM_0) );
				return PRL_ERR_CANT_CREATE_PARALLEL_PORT_IMAGE;
			}
			else
				m_lstCreatedFiles.append( strFullPath );
		}
		else
		{
			// Use real device...
		}
	}
	return ret;
}

/**
* Check and create hard disk images
* @return Returns error code.
*/
PRL_RESULT	Task_RegisterVm::createHardDisks()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	QList<CVmHardDisk*> &lstHardDisks = m_pVmConfig->getVmHardwareList()->m_lstHardDisks;

	LOG_MESSAGE( DBG_FATAL,"HardDisk count [%d]", lstHardDisks.count() );

	QList<CVmHardDisk*>::iterator it_hdd;

	int hddNum = 0;
	for ( it_hdd = lstHardDisks.begin(); it_hdd != lstHardDisks.end(); ++it_hdd, hddNum++ )
	{
		CVmHardDisk *pHardDisk = *it_hdd;
		// LOG_MESSAGE( DBG_FATAL,"########## Current disk [%s]", pHardDisk->getUserFriendlyName().toUtf8().data());

		if (pHardDisk->getSerialNumber().isEmpty())
			pHardDisk->setSerialNumber(Parallels::generateDiskSerialNumber());
		// FIXME: Validate HardDisk full path
		QString strFullPath;
		if ( doRegisterOnly() )
		{
			if ( pHardDisk->getEmulatedType() == PVE::BootCampHardDisk ||
					 pHardDisk->getEmulatedType() == PVE::RealHardDisk)
			{
#ifndef _WIN_
				if ( pHardDisk->getSystemName().startsWith("/dev/") )
				{
					WRITE_TRACE(DBG_FATAL, "Seems hard disk system name contains physical device name: '%s'", QSTR2UTF8(pHardDisk->getSystemName()));
					strFullPath = QString( "%1/%2.hdd" ).
                    				arg( QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath() ).
                    				arg( pHardDisk->getUserFriendlyName().remove("/") );
				}
				else
#endif
				strFullPath = ConvertToFullPath( pHardDisk->getSystemName() );
			}
			else
				continue;
		}
		else
		{
			if ( pHardDisk->getEmulatedType() == PVE::BootCampHardDisk ||
					 pHardDisk->getEmulatedType() == PVE::RealHardDisk)
				strFullPath = QString( "%1/%2.hdd" ).
					arg( QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath() ).
					arg( pHardDisk->getUserFriendlyName().remove("/") );
			else
				strFullPath = ConvertToFullPath( pHardDisk->getUserFriendlyName() );
		}

		if (QFile::exists(strFullPath))//File exists so it's not necessary to create it
			continue;

		PRL_RESULT ret = PRL_ERR_SUCCESS;
		if ( pHardDisk->getEmulatedType() == PVE::HardDiskImage )
		{
			if ( (getRequestFlags() & PRNVM_ALLOW_TO_AUTO_DECREASE_HDD_SIZE) != 0
				&& ! doRegisterOnly() )
			{
				quint64 nAvailableSpace;
				quint64 nTotalSpace;

				CFileHelper::GetDiskAvailableSpace(
								QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath(),
								&nAvailableSpace, &nTotalSpace);
				if (nTotalSpace <= SIZE_64GB)
				{
					pHardDisk->setSize((quint32 )(SIZE_32GB / (1024*1024)));

					WRITE_TRACE(DBG_FATAL,
						"Create new VM: physical disk is 64 Gb or less - virtual disk size was set in 32 Gb !");
				}
			}

			QString hddXMLstr = ElementToString<CVmHardDisk*>(
				pHardDisk, XML_VM_CONFIG_EL_HARD_DISK );

			LOG_MESSAGE( DBG_DEBUG, "debug: hddXMLstr = \n%s", QSTR2UTF8(hddXMLstr) );

			SmartPtr<CDspClient> dspClient = getClient();
			SmartPtr<IOService::IOPackage> requestPackage = getRequestPackage();

			Task_CreateImage taskCreateImage(
				dspClient, requestPackage, m_pVmConfig, hddXMLstr, false, true);

			m_lpcCreateImageCurrentTask = &taskCreateImage;
			ret = taskCreateImage.run_body();
			m_mtxWaitCreateImage.lock();
			m_lpcCreateImageCurrentTask = NULL;
			m_mtxWaitCreateImage.unlock();


			if ( PRL_SUCCEEDED(ret) )
				m_lstCreatedDirs.append( strFullPath );
			else
			{
				getLastError()->setEventCode( ret );
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, strFullPath, EVT_PARAM_RETURN_PARAM_TOKEN) );
				for (int i = 0 ; i < taskCreateImage.getLastError()->m_lstEventParameters.size() ; i++)
				{
					// copy all event parameters
					getLastError()->addEventParameter(
						new CVmEventParameter(taskCreateImage.getLastError()->m_lstEventParameters[i])
						);
				}

				return ret;
			}
		}
		else
		{
			// It needs to have admin permissions to read physical disk information
			bool flgImpersonate = getClient()->getAuthHelper().RevertToSelf();
			PRL_ASSERT( flgImpersonate );
			if( !flgImpersonate )
				return PRL_ERR_REVERT_IMPERSONATE_FAILED;

			// NOTE : We should do BootCamp disk reconfig in any case, when creating/registering VM.
			// This will allow user to recreate VM and make forced BootCamp reconfiguration,
			// if he encountered some BootCamp reconfig issue.
			// (workaround solution for https://bugzilla.sw.ru/show_bug.cgi?id=464970)

			// ShadowVM and BootCamp functionality was removed as not used in PCS
			//bool bUseEfi = false;
			//ret = Task_EditVm::configureOnePhysicalDisk( pHardDisk, strFullPath, getClient(),
			//											 getLastError(), true, &bUseEfi );
			//if (bUseEfi)
			//	m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios()->setType( PBT_EFI );

			flgImpersonate =  getClient()->getAuthHelper().Impersonate();
			PRL_ASSERT( flgImpersonate );
			if( !flgImpersonate )
				return PRL_ERR_IMPERSONATE_FAILED;

			if ( PRL_SUCCEEDED(ret) )
			{
				m_lstCreatedDirs.append( strFullPath );

				if ( !doRegisterOnly() && pHardDisk->getEmulatedType() == PVE::BootCampHardDisk )
				{
					//Correspond with bug https://bugzilla.sw.ru/show_bug.cgi?id=124473 switching shared profiles off
					//for VMs with Bootcamp
					m_pVmConfig->getVmSettings()->getVmTools()->getVmSharedProfile()->setEnabled(false);
				}
			}
			else
			{
				getLastError()->setEventCode( ret );
				if (ret == PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS)
				{
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, m_pVmConfig->getVmIdentification()->getVmName(),
						EVT_PARAM_MESSAGE_PARAM_0));
				}
				else
				{
					getLastError()->addEventParameter(
						new CVmEventParameter( PVE::String, strFullPath, EVT_PARAM_RETURN_PARAM_TOKEN) );
				}
				return ret;
			}
		}
	}
	return ret;
}

PRL_RESULT Task_RegisterVm::createNVRAM()
{
	CVmStartupBios* b = m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios();

	if (!b->isEfiEnabled())
		return PRL_ERR_SUCCESS;

	if (b->getNVRAM().isEmpty())
		b->setNVRAM(ConvertToFullPath(PRL_VM_NVRAM_FILE_NAME));

	CDspTaskFailure f(*this);
	Edit::Vm::Create::Action<CVmStartupBios>(*b, *m_pVmConfig).execute(f);
	return getLastErrorCode();
}

/**
* Delete created config items to rollback a failed VM registration transaction
*/
void Task_RegisterVm::deleteCreatedConfigItems()
{
	for ( int i = 0; i < m_lstCreatedFiles.size(); i++ )
	{
		QString qsFile = m_lstCreatedFiles.at( i );
		if ( !CFileHelper::RemoveEntry(qsFile, &getClient()->getAuthHelper()) )
			WRITE_TRACE(DBG_FATAL,
			"Task_RegisterVm::deleteCreatedConfigItems() : Can not delete file %s.",
			QSTR2UTF8(qsFile) );
	}
	m_lstCreatedFiles.clear();

	for ( int i = 0; i < m_lstCreatedDirs.size(); i++ )
	{
		QString qsDir = m_lstCreatedDirs.at( i );
		if ( !CFileHelper::ClearAndDeleteDir( qsDir ) )
			WRITE_TRACE(DBG_FATAL,
			"Task_RegisterVm::deleteCreatedConfigItems() : Can not delete directory %s.",
			QSTR2UTF8(qsDir) );
	}
	m_lstCreatedDirs.clear();

	return;
}

/**
* Rollback the transaction if VM registration was failed
*/
void Task_RegisterVm::rollback()
{
	deleteCreatedConfigItems();

	if ( doRegisterOnly() && !m_strRenamedVmHome.isEmpty() )
	{
		if ( !CFileHelper::RenameEntry(m_strRenamedVmHome,
										m_strPathToVmDirToRegister,
										&getClient()->getAuthHelper()) )
			WRITE_TRACE(DBG_FATAL, "Task_RegisterVm::renameBundleItems() : Can not rename directory %s.",
				QSTR2UTF8(m_strRenamedVmHome) );
	}

	return;
}

void Task_RegisterVm::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pUserSession, p);
	QMutexLocker locker(&m_mtxWaitCreateImage);
	if (m_lpcCreateImageCurrentTask)
	{
		m_lpcCreateImageCurrentTask->cancelOperation(SmartPtr<CDspClient>(), SmartPtr<IOPackage>());
	}
}

void Task_RegisterVm::reassignTask( SmartPtr<CDspClient>& pNewClient, const SmartPtr<IOPackage>& pNewPackage )
{
	CDspTaskHelper::reassignTask(pNewClient,pNewPackage);
	// added
	QMutexLocker locker(&m_mtxWaitCreateImage);
	if (m_lpcCreateImageCurrentTask)
	{
		m_lpcCreateImageCurrentTask->reassignTask(pNewClient,pNewPackage);
	}
}

void Task_RegisterVm::validateNewConfiguration(  )
{
	// 1) validate only new configurations
	// 2) skip validating for registering VM
	// - to allow to user change setting after registering.
	if( doRegisterOnly() )
		return;


	CVmValidateConfig vc(getClient(), m_pVmConfig);
	CVmEvent evtResult;

	if (vc.HasCriticalErrors(evtResult))
	{
		WRITE_TRACE(DBG_FATAL, "Configuration validation failed" );
		LOG_MESSAGE( DBG_INFO, "CVmValidateConfig::HasCriticalErrors() return true\n");

		// send reply to user
		getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );

		getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
			evtResult.toString(),
			EVT_PARAM_COMPLEX_EVENT));
		throw PRL_ERR_INCONSISTENCY_VM_CONFIG;
	}
}

void Task_RegisterVm::patchNewConfiguration()
{
	{
		CDspBugPatcherLogic logic( *CDspService::instance()->getHostInfo()->data() );

		if( doRegisterOnly() )
			logic.patchOldConfig( m_dirUuid,
								  m_pVmConfig,
								  CDspBugPatcherLogic::pkRegisterVm );
		else
			logic.patchNewConfig( m_pVmConfig );

		// Remove bug patcher logic as useless in OpenDisp
		//CDspBugPatcherLogic::setVmBundleDefaultTag( QFileInfo(m_pVmInfo->vmXmlPath).absolutePath() );
	}

	if( doRegisterOnly() )
		return;

	// bug #269365

#ifdef _WIN_
	bool bAdmin = getClient()->getAuthHelper().isLocalAdministrator();
#endif

	foreach( CVmOpticalDisk* pCdRom , m_pVmConfig->getVmHardwareList()->m_lstOpticalDisks )
	{
#ifdef _LIN_
		CAuth::AccessMode nAccess = getClient()->getAuthHelper().CheckFile( pCdRom->getSystemName() );
#endif
		if ( pCdRom->getEmulatedType() == PVE::RealCdRom
			&& ! pCdRom->isRemote()
#ifdef _LIN_
			&& nAccess == ( CAuth::fileMayRead | CAuth::fileMayWrite )
#elif defined( _WIN_ )
			&& bAdmin
#endif
			)
		{
			WRITE_TRACE(DBG_FATAL, "Set enabled passthrough for '%s' CD-ROM", QSTR2UTF8(pCdRom->getSystemName()));
			pCdRom->setPassthrough( PVE::PassthroughEnabled );
		}
	}

	if (m_pVmConfig->getVmHardwareList()->getClock() != NULL &&
		m_pVmConfig->getVmHardwareList()->getClock()->getTimeShift() != 0)
		return;

	if (!IS_WINDOWS(m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion()))
		return;

	std::time_t current_time;
	std::time(&current_time);
	std::tm timeinfo;
	localtime_r(&current_time, &timeinfo);

	QScopedPointer<Clock> c(new Clock());
	c->setTimeShift(timeinfo.tm_gmtoff);
	m_pVmConfig->getVmHardwareList()->setClock(c.take());
}

QString Task_RegisterVm::regenerateVmUuid()
{
	QString vm_uuid = Uuid::createUuid().toString();

	WRITE_TRACE( DBG_FATAL, "Vm_uuid will be recreated. Old VmUuid= '%s', new VmUuid = %s"
		, QSTR2UTF8(m_pVmConfig->getVmIdentification()->getVmUuid())
		, QSTR2UTF8( vm_uuid )
		);

	m_pVmConfig->getVmIdentification()->setVmUuid( vm_uuid );

	if(m_pVmInfo)
		m_pVmInfo->vmUuid = vm_uuid;// update vm uuid to register

	return vm_uuid;
}

QString Task_RegisterVm::regenerateSrcVmUuid()
{
	QString vm_src_uuid = Uuid::createUuid().toString();

	WRITE_TRACE( DBG_FATAL, "Vm_src_uuid will be recreated. Old SourceVmUuid= '%s', new SourceVmUuid = %s"
		, QSTR2UTF8( m_pVmConfig->getVmIdentification()->getSourceVmUuid() )
		, QSTR2UTF8( vm_src_uuid )
		);

	m_pVmConfig->getVmIdentification()->setSourceVmUuid( vm_src_uuid );

	return vm_src_uuid;
}

PRL_RESULT Task_RegisterVm::tryToRestoreVmConfig( const QString& sPathToVmDir )
{
	WRITE_TRACE(DBG_FATAL, "Cannot open VM config and try to restore it!");
	// try to load VM configuration from backup file
	//////////////////////////////////////////////////////////////////////////
	QString vm_xml_path=QString("%1/%2")
		.arg( sPathToVmDir )
		.arg( VMDIR_DEFAULT_VM_CONFIG_FILE );

	if ( !CDspService::instance()->getVmConfigManager().canConfigRestore(vm_xml_path, getClient()) )
	{
		WRITE_TRACE(DBG_FATAL, "VM config '%s' cannot be restored from backup config!", QSTR2UTF8(vm_xml_path));
		return PRL_ERR_PARSE_VM_CONFIG;
	}

	QFile	file( vm_xml_path + VMDIR_DEFAULT_VM_BACKUP_SUFFIX );
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration( &file ));

	if ( PRL_FAILED( m_pVmConfig->m_uiRcInit ) )
		return PRL_ERR_PARSE_VM_CONFIG;

	PRL_RESULT nRes = CDspService::instance()->getVmConfigManager()
			.restoreConfig(vm_xml_path,
			getClient(),
			getClient()->getAuthHelper().getUserFullName());
	if ( PRL_FAILED(nRes) )
	{
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String,
			"Unknown",
			EVT_PARAM_MESSAGE_PARAM_0 ) );
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String,
			vm_xml_path,
			EVT_PARAM_MESSAGE_PARAM_1 ) );

		return PRL_ERR_VM_CONFIG_DOESNT_EXIST;
	}

	return PRL_ERR_SUCCESS;
}
