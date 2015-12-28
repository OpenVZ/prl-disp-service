///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspBugPatcherLogic.cpp
///
/// this object contains logic for patching variouse xml configs on dispatcher side
///
/// @author Artemr
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
///////////////////////////////////////////////////////////////////////////////

// #define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <QMutableListIterator>
#include <QListIterator>
#include <QSettings>
#include <QRegExp>

#include "CDspBugPatcherLogic.h"

#include "XmlModel/DispConfig/CDispWorkspacePreferences.h"
#include "XmlModel/VmDirectory/CVmDirectory.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/VmConfig/CVmSoundInputs.h"
#include "XmlModel/VmConfig/CVmSoundOutputs.h"
#include "XmlModel/ParallelsObjects/CVmProfileHelper.h"

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>

#include "Libraries/HostInfo/CHostInfo.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/CHardDiskHelper.h>

#include "Interfaces/ParallelsSdkPrivate.h"

#include "Tasks/Mixin_CreateHddSupport.h"
#include "Tasks/Task_ManagePrlNetService.h"

#include "CDspSync.h"
#include "CDspService.h"
#include "CDspCommon.h"
#include "CDspVmNetworkHelper.h"

#include "Build/Current.ver"

#define BUGFIXES_PARAMETERS_GROUP		"BUGFIXES_Parameters"

namespace{
	//  ============ NOT-VM patches ============
	static const char* bug424340_Restore_AllowUseNetworkShares = "bug424340_NeedRecoverReadOnlyValuesInDispConfig";
	static const char* PSBM_5755_enableUptimeFeature = "PSBM_5755_enableUptimeFeature";
	static const char* bug479190_switchOffVerboseLogging = "bug479190_switchOffVerboseLogging"; // INTERNAL
	static const char* jira_PWE_5629_unification_domain_part_of_username= "jira_PWE_5629_unification_domain_part_of_username";

	//  ============ VM patches ============
	static const char* bug7938_ShouldFixServerUuidInVmConfigs = "bug7938_ShouldFixServerUuidInVmConfigs";
	static const char* bug438530_MultiDisplay = "bug438530_ShouldFixMultyDisplayInVmConfigs";
	static const char* bug477998_app_version = "bug477998_UpdateAppVersion";
	static const char* PWE_3918_changeAutoStartLoadOption = "PWE_3918_changeAutoStartLoadOption";
	static const char* PSBM_13394_vmNetworkAdapterHostMac = "PSBM_13394_vmNetworkAdapterHostMac";
}

static const PatchKeysInitializer g_PathKeys;

PatchKeysInitializer::PatchKeysInitializer()
{
	lstVmPatchKeys << bug7938_ShouldFixServerUuidInVmConfigs
		<< bug438530_MultiDisplay
		<< bug477998_app_version
		<< PWE_3918_changeAutoStartLoadOption
		<< PSBM_13394_vmNetworkAdapterHostMac;

}

// constructor only for patchCommon()
CDspBugPatcherLogic::CDspBugPatcherLogic()
:	m_pHostInfo( new CHostHardwareInfo )
{

}
CDspBugPatcherLogic::CDspBugPatcherLogic(const CHostHardwareInfo& hostInfo)
:	m_pHostInfo( new CHostHardwareInfo(hostInfo.toString()) )
{
}

CDspBugPatcherLogic::~CDspBugPatcherLogic()
{
}

void CDspBugPatcherLogic::patchCommon(const QString& qsOldDispVersion, const QString& qsCurrentDispVersion )
{
	quint32 nRepatchMask = No_repatch;

	if (qsOldDispVersion != qsCurrentDispVersion)
		nRepatchMask |= BUILD_VERSION_WAS_CHANGED;

	if ( qsOldDispVersion.isEmpty() )
	{
		nRepatchMask |= PD_before_PD6;
		nRepatchMask |= PS_before_PSBM5;
		nRepatchMask |= PRODUCT_WAS_UPGRADED;
	}
	else if( qsOldDispVersion != qsCurrentDispVersion )
	{
		quint32 uiOldVer = qsOldDispVersion.section( ".", 0, 0 ).toUInt();
		quint32 uiCurrVer =  qsCurrentDispVersion.section( ".", 0, 0 ).toUInt();
		if( uiOldVer < uiCurrVer )
			nRepatchMask |= PRODUCT_WAS_UPGRADED;
		else if( uiOldVer > uiCurrVer )
			nRepatchMask |= PRODUCT_WAS_DOWNGRADED;
	}

	if( nRepatchMask & PRODUCT_WAS_UPGRADED )
		WRITE_TRACE( DBG_INFO, "Dispatcher was upgraded." );
	if( nRepatchMask & PRODUCT_WAS_DOWNGRADED )
		WRITE_TRACE( DBG_INFO, "Dispatcher was downgraded." );

	CDspBugPatcherLogic commonPatcher;
	commonPatcher.patchCommonConfigs( nRepatchMask );
}

void CDspBugPatcherLogic::collectCommonPatches( quint32 nRepatchMask )
{
	m_lstPatches.clear();
	// collect patches
	CDspLockedPointer<QSettings> pSettings = CDspService::instance()->getQSettings();
	pSettings->beginGroup(BUGFIXES_PARAMETERS_GROUP);

#define ADD_PATCH_TO_LIST( pathName ) \
	{	\
		if( pSettings->value(pathName).isNull() )	\
			m_lstPatches << pathName;	\
		pSettings->setValue(pathName, "true");	\
	}

	ADD_PATCH_TO_LIST(bug424340_Restore_AllowUseNetworkShares);
	ADD_PATCH_TO_LIST(jira_PWE_5629_unification_domain_part_of_username);

	if( (nRepatchMask & PS_before_PSBM5) && CDspService::isServerMode() )
		ADD_PATCH_TO_LIST(PSBM_5755_enableUptimeFeature);

	pSettings->endGroup();

	if( (nRepatchMask & PRODUCT_WAS_DOWNGRADED)
		|| (nRepatchMask & PRODUCT_WAS_UPGRADED) )
	{
		m_lstPatches << bug479190_switchOffVerboseLogging;
	}

	pSettings->sync();

#undef ADD_PATCH_TO_LIST

	// trace prepared patches
	if( m_lstPatches.size() )
	{
		QString sPatches;
		foreach( QString id, m_lstPatches )
			sPatches += QString(" %1, ").arg(id);
		WRITE_TRACE( DBG_FATAL, "Next patches will be applied: %s", QSTR2UTF8(sPatches) );
	}
}

void CDspBugPatcherLogic::patchCommonConfigs( quint32 nRepatchMask)
{
	bool bSaveDispCfg = false;

	collectCommonPatches(nRepatchMask);
	patchDefaultVmDirectoryPath();
	patchDispatcherXml(bSaveDispCfg);

	if(bSaveDispCfg)
		CDspService::instance()->getDispConfigGuard().saveConfig();
}

#define MAKE_VM_KEY(vmDirUuid, vmUuid)		(vmDirUuid + "-" + vmUuid)

QSet<PKeyId> CDspBugPatcherLogic::collectVmPatches(const QString& vmDirUuid, const QString& vmUuid)
{
	QSet<PKeyId> lstUnpatchedIds;

	CDspLockedPointer<QSettings> pSettings = CDspService::instance()->getQSettings();
	pSettings->beginGroup(BUGFIXES_PARAMETERS_GROUP);

	foreach(PKeyId prKeyId, g_PathKeys.getPatches() )
	{
		bool bHasOldPatch = ! pSettings->value(prKeyId).isNull();
		QString qsVmKey = MAKE_VM_KEY(vmDirUuid, vmUuid);

		pSettings->beginGroup(prKeyId);

		if (pSettings->value(qsVmKey).isNull())
		{
			if ( ! bHasOldPatch )
			{
				lstUnpatchedIds << prKeyId ;
				pSettings->setValue(qsVmKey, true);
			}
			else
				pSettings->setValue(qsVmKey, true);
		}

		pSettings->endGroup();
	}

	pSettings->endGroup();
	pSettings->sync();

	return lstUnpatchedIds;
}

void CDspBugPatcherLogic::cleanVmPatchMarks(const QString& vmDirUuid, const QString& vmUuid)
{
	CDspLockedPointer<QSettings> pSettings = CDspService::instance()->getQSettings();
	pSettings->beginGroup(BUGFIXES_PARAMETERS_GROUP);

	foreach(PKeyId prKeyId, g_PathKeys.getPatches())
	{
		pSettings->beginGroup(prKeyId);

		QString qsVmKey = MAKE_VM_KEY(vmDirUuid, vmUuid);
		pSettings->remove(qsVmKey);

		pSettings->endGroup();
	}

	pSettings->endGroup();
	pSettings->sync();
}

void CDspBugPatcherLogic::patchVmConfigs()
{
	QSet<QString> patchedConfigs;

	// Update information about used host macs (for all VMs)
	m_usedMacs.clear();
	CDspVmNetworkHelper::getUsedHostMacAddresses(m_usedMacs);

	Vm::Directory::Dao::Free d;
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		if (NULL == i.second)
			continue;

#ifdef _CT_
		if (i.second->getVmType() == PVT_CT)
			continue;
#endif

		// FIXME: Need check that item->getVmName() != "" ( PSfM 3 bug ) and fix it !!

		if( patchVmConfig(i.first, i.second) )
			patchedConfigs << i.second->getVmUuid();
	}

	// send config changed event when any user requests on startup config
	QList< SmartPtr<CDspClient> >
		sessions = CDspService::instance()->getClientManager().getSessionsListSnapshot().values();
	foreach( QString vmUuid, patchedConfigs )
	{
		CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, vmUuid, PIE_DISPATCHER );
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
		CDspService::instance()->getClientManager()
			.sendPackageToClientList( p, sessions);
	}
}

bool CDspBugPatcherLogic::patchVmConfig(const CVmIdent& vmId)
{
	SmartPtr<CVmDirectoryItem> pDirItem;
	{
		CDspLockedPointer<CVmDirectoryItem> pLockedItem = CDspService::instance()->getVmDirHelper()
			.getVmDirectoryItemByUuid(vmId.second, vmId.first);
		if( !pLockedItem )
			return false;
		pDirItem = SmartPtr<CVmDirectoryItem>( new CVmDirectoryItem( pLockedItem.getPtr() ) );
	}

	bool bRes = patchVmConfig(vmId.second, pDirItem.getImpl());
	if (!bRes)
		return false;

	return bRes;
}

bool CDspBugPatcherLogic::patchVmConfig(const QString& vmDirUuid,
										CVmDirectoryItem* item)
{
	PRL_ASSERT(item);

	SmartPtr<CDspClient> pFakeUserSession = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
	pFakeUserSession->getAuthHelper().AuthUserBySelfProcessOwner();

	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration());
	PRL_RESULT nRetCode = CDspService::instance()->getVmConfigManager().loadConfig(pVmConfig,
		item->getVmHome(), pFakeUserSession);
	//https://bugzilla.sw.ru/show_bug.cgi?id=481809

	if ( PRL_FAILED(nRetCode) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to load configuration for '%s' VM by '%s' VM home path with retcode: %.8x '%s'",
				QSTR2UTF8(item->getVmName()),
				QSTR2UTF8(item->getVmHome()),
				nRetCode,
				PRL_RESULT_TO_STRING(nRetCode));
		return false;
	}

	bool bSaveConfig = false;

	QSet<PKeyId> lstPatches = collectVmPatches(vmDirUuid, item->getVmUuid());

	if ( lstPatches.contains(bug7938_ShouldFixServerUuidInVmConfigs) )
	{
		pVmConfig->getVmIdentification()->setServerUuid( CDspDispConfigGuard::getServerUuid() );
		bSaveConfig |= true;
	}

	if ( lstPatches.contains(PSBM_13394_vmNetworkAdapterHostMac) )
	{
		patchVmHostMacs(pVmConfig, &m_usedMacs, HMU_NONE);
		bSaveConfig |= true;
	}

	bSaveConfig |= applyXmlPatches(vmDirUuid, pVmConfig, pkInitService, lstPatches);

	//////////////////////////////////////////////////////////
	// patchAppVersion has to be called the last !
	//////////////////////////////////////////////////////////
	if ( lstPatches.contains(bug477998_app_version) )
	{
		bSaveConfig |= true;
		patchAppVersion( pVmConfig );
	}

	if ( ! bSaveConfig )
		return false;

	CDspService::instance()->getVmConfigManager().saveConfig(pVmConfig,
		item->getVmHome(), pFakeUserSession);

	// FIXME TODO: Need print patch list here !

	WRITE_TRACE(DBG_FATAL, "[%s] several patches was applied to file: \"%s\""
		, __FUNCTION__
		, QSTR2UTF8(item->getVmHome())
		);
	return true;
}

void CDspBugPatcherLogic::patchDefaultVmDirectoryPath()
{
	// https://bugzilla.sw.ru/show_bug.cgi?id=445937
	// [ [win7] 2x Parallels folders on Windows 7 with PD4 ]

	// TODO:
	// 1. replace default path for default vm directory
	// 2. fix default path in users vm directories if they have path which equals path to common vm directory

	QString sDefaultSharedVmCataloguePath =  ParallelsDirs::getCommonDefaultVmCatalogue();
	//////////////////////////////////////////////////////////////////////////
	// 1. set default path for default vm directory
	QString sDirUuid = CDspService::instance()->getDispConfigGuard()
		.getDispWorkSpacePrefs()->getDefaultVmDirectory();

	CDspLockedPointer<CVmDirectory> pVmDir = CDspService::instance()->getVmDirManager()
		.getVmDirectory( sDirUuid );
	PRL_ASSERT( pVmDir );
	if( sDefaultSharedVmCataloguePath == pVmDir->getDefaultVmFolder() )
		return;

	WRITE_TRACE( DBG_FATAL, "Patch path for common vm catalogue:\n"
		"'%s' ==> '%s'"
		, QSTR2UTF8( pVmDir->getDefaultVmFolder() )
		, QSTR2UTF8( sDefaultSharedVmCataloguePath )
		);
	QString sOldCommonPath = pVmDir->getDefaultVmFolder();
	pVmDir->setDefaultVmFolder( sDefaultSharedVmCataloguePath );

	// 2. fix default path in users vm directories
	//		if they have path which equals path to common vm directory
	Vm::Directory::Dao::Locked x;
	foreach (CVmDirectory& d, x.getList())
	{
		if( d.getUuid() == pVmDir->getUuid() )
			continue;

		// fix users vm directory when defaultVmFolder is path to common vm directory
		//	( it may be by #115878 when dispatcher can't to create users vm directory)
		if( d.getDefaultVmFolder() == sOldCommonPath )
		{
			WRITE_TRACE( DBG_FATAL, "Patch path for users vm catalogue (%s):\n"
				"'%s' ==> '%s'"
				, QSTR2UTF8( d.getUserFriendlyName() )
				, QSTR2UTF8( sOldCommonPath )
				, QSTR2UTF8( sDefaultSharedVmCataloguePath )
				);
			d.setDefaultVmFolder( sDefaultSharedVmCataloguePath );
		} //if
	}// foreach(CVmDirectory*
}

void CDspBugPatcherLogic::patchDispatcherXml(bool & bSaveDispCfg)
{
	if( m_lstPatches.contains( bug479190_switchOffVerboseLogging ) )
	{
		CDspLockedPointer<CDispCommonPreferences>
			pCommonPrefs = CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
		if( pCommonPrefs->getDebug()->isVerboseLogEnabled() )
		{
			WRITE_TRACE( DBG_WARNING, "NOTE: Verbose logging was disabled." );
			pCommonPrefs->getDebug()->setVerboseLogEnabled( false );
			bSaveDispCfg = true;
		}
	}

	if( m_lstPatches.contains( bug424340_Restore_AllowUseNetworkShares ) )
	{
		// #424340 Restore <AllowUseNetworkShares> parameter
		CDspLockedPointer<CDispWorkspacePreferences> pWorkspace =
			CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs();
		bool bAllowUseNetworkShares = true;
		if( bAllowUseNetworkShares!= pWorkspace->isAllowUseNetworkShares() )
		{
			WRITE_TRACE(DBG_FATAL, "Restore AllowUseNetworkShares parameter by default by #424340.");
			pWorkspace->setAllowUseNetworkShares( bAllowUseNetworkShares );
			bSaveDispCfg = true;
		}
	}

	if( m_lstPatches.contains( PSBM_5755_enableUptimeFeature ) )
	{
		PRL_ASSERT( CDspService::isServerMode() );

		CDspLockedPointer<CDispWorkspacePreferences> pWorkspace =
			CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs();

		uint nOldValue = pWorkspace->getVmUptimeSyncTimeoutInMinutes();
		if( nOldValue == 0)
		{
			pWorkspace->setVmUptimeSyncTimeoutInMinutes();
			bSaveDispCfg = true;

			WRITE_TRACE(DBG_FATAL, "Restore VmUptimeSyncTimeoutInMinutes parameter to default value %d(from %d)"
				, pWorkspace->getVmUptimeSyncTimeoutInMinutes(), nOldValue );
		}
	}

	if ( m_lstPatches.contains(jira_PWE_5629_unification_domain_part_of_username) )
	{
		patchDomainPartOfLocalUserName();
		bSaveDispCfg = true;
	}
}

//////////////////////////////////////////////////////////////////////////

class CXmlPatchBase
{
public:
	bool tryToPatch(const SmartPtr<CVmConfiguration>& pVmConfig,
					CDspBugPatcherLogic::PatcherKind patcher,
					const QSet<PKeyId>& lstPatches);
	void markAsPatched(const SmartPtr<CVmConfiguration>& pVmConfig);
	virtual ~CXmlPatchBase() {}

	virtual void setVmDirUuid(const QString& ) {}
	virtual void setHostInfo(const SmartPtr<CHostHardwareInfo>& ) {}

protected:
	CXmlPatchBase(const QString& path, const QString& stamp = "1")
		: m_qsPath(path), m_qsStamp(stamp) {}

	virtual bool needApplyPatch(CDspBugPatcherLogic::PatcherKind ) const;
	virtual void patch( const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind patcher,
						const QSet<PKeyId>& lstPatches ) = 0;
	bool isFieldPatched(const SmartPtr<CVmConfiguration>& pVmConfig);
	void patchField(const SmartPtr<CVmConfiguration>& pVmConfig, QVariant val);
private:
	QString m_qsPath;
	QString m_qsStamp;
};

bool CXmlPatchBase::tryToPatch( const SmartPtr<CVmConfiguration>& pVmConfig,
								CDspBugPatcherLogic::PatcherKind patcher,
								const QSet<PKeyId>& lstPatches)
{
	if ( ! needApplyPatch(patcher) || isFieldPatched(pVmConfig))
		return false;

	patch(pVmConfig, patcher, lstPatches);
	markAsPatched(pVmConfig);
	return true;
}

bool CXmlPatchBase::needApplyPatch(CDspBugPatcherLogic::PatcherKind ) const
{
	return true;
}

bool CXmlPatchBase::isFieldPatched(const SmartPtr<CVmConfiguration>& pVmConfig)
{
	QVariant val = pVmConfig->getPropertyValue(m_qsPath + ".patch_stamp");
	PRL_ASSERT(val.isValid());
	return val.toString() == m_qsStamp;
}

void CXmlPatchBase::markAsPatched(const SmartPtr<CVmConfiguration>& pVmConfig)
{
	PRL_ASSERT(pVmConfig->setPropertyValue(m_qsPath + ".patch_stamp", m_qsStamp));
}

void CXmlPatchBase::patchField(const SmartPtr<CVmConfiguration>& pVmConfig, QVariant val)
{
	PRL_ASSERT(pVmConfig->setPropertyValue(m_qsPath, val));
}

bool CDspBugPatcherLogic::applyXmlPatches(const QString& vmDirUuid,
										  const SmartPtr<CVmConfiguration>& pVmConfig,
										  PatcherKind patcher,
										  const QSet<PKeyId>& lstPatches)
{
	bool bSaveConfig = false;

	foreach(CXmlPatchBase* pXmlPatch, getXmlPatches() )
	{
		pXmlPatch->setVmDirUuid(vmDirUuid);
		pXmlPatch->setHostInfo(m_pHostInfo);
		bSaveConfig |= pXmlPatch->tryToPatch(pVmConfig, patcher, lstPatches);
	}

	return bSaveConfig;
}

void CDspBugPatcherLogic::patchOldConfig(
	const QString& vmDirUuid, const SmartPtr<CVmConfiguration>& pVmConfig, CDspBugPatcherLogic::PatcherKind patcher)
{
	PRL_ASSERT(patcher == pkRegisterVm || patcher == pkSwitchToSnapshot);

	applyXmlPatches(vmDirUuid, pVmConfig, patcher);

	patchVmHostMacs(pVmConfig, NULL, HMU_NONE);
	patchAppVersion(pVmConfig);	// must be called the last
}

void CDspBugPatcherLogic::patchNewConfig(const SmartPtr<CVmConfiguration>& pVmConfig)
{
	// mark as 'patched' to prevent repatch on dispatcher startup
	foreach(CXmlPatchBase* pXmlPatch, getXmlPatches() )
	{
		pXmlPatch->markAsPatched(pVmConfig);
	}

	patchVmHostMacs(pVmConfig, NULL, HMU_CHECK_NONEMPTY );
	patchAppVersion(pVmConfig);	// must be called the last
}

void CDspBugPatcherLogic::patchDomainPartOfLocalUserName()
{
#ifndef _WIN_
	return;
#else // _WIN_

	CDspLockedPointer<CDispUsersPreferences> pUsers =
		CDspService::instance()->getDispConfigGuard().getDispUserPreferences();

	QHash< QString, CDispUser* > dispUsersHash;
	foreach( CDispUser* pUser, pUsers->m_lstDispUsers )
	{
		dispUsersHash[pUser->getUserName()] = pUser;
	}

	QString sComputerName;
	if( !CAuth::getComputerName(sComputerName) )
	{
		WRITE_TRACE( DBG_FATAL
			, "Unable to automatically patch local usernames entries because getComputerName() failed");
		return;
	}

	foreach( QString fullName, dispUsersHash.keys() )
	{
		CAuthHelper parser( fullName );
		if( 0 != parser.getUserDomain().compare( sComputerName, Qt::CaseInsensitive ) )
			continue;

		QString localNameInDefaultFormat = parser.getUserName() + "@.";
		if( !dispUsersHash.contains(localNameInDefaultFormat) )
		{
			WRITE_TRACE( DBG_FATAL, "Entry of user '%s' in dispatcher.xml was automatically renamed to '%s'."
				, QSTR2UTF8(fullName), QSTR2UTF8(localNameInDefaultFormat) );

			dispUsersHash.value(fullName)->setUserName(localNameInDefaultFormat);
		}
		else
		{
			WRITE_TRACE( DBG_FATAL, "Unable to automatically patch entry of user '%s' in dispatcher.xml"
				" because entry with name '%s' already exists."
				, QSTR2UTF8(fullName), QSTR2UTF8(localNameInDefaultFormat)
				);
		}
	}

#endif // _WIN_

}

void CDspBugPatcherLogic::patchVmHostMacs(const SmartPtr<CVmConfiguration>& pVmConfig,
										  QSet<QString>* pUsedMacs,
										  HostMacUpdateFlags flags)
{
	WRITE_TRACE(DBG_DEBUG, "Patch host mac adresses (flags:%d)", flags);
	CDspVmNetworkHelper::updateHostMacAddresses(pVmConfig, pUsedMacs, flags);
}

void CDspBugPatcherLogic::patchAppVersion(const SmartPtr<CVmConfiguration>& pVmConfig)
{
	pVmConfig->setAppVersion(CVmConfiguration::makeAppVersion());
}

class CPatchVmMultiDisplay : public CXmlPatchBase
{
public:
	CPatchVmMultiDisplay()
		: CXmlPatchBase("Settings.Tools.Coherence.MultiDisplay") {}

	virtual void patch( const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind patcher,
						const QSet<PKeyId>& lstPatches )
	{
		QString strCfgAppVersion = pVmConfig->getAppVersion();
		QString strCurrentAppVersion = pVmConfig->makeAppVersion();
		quint32 uiCfgVer = strCfgAppVersion.section( ".", 0, 0 ).toUInt();
		quint32 uiAppVer =  strCurrentAppVersion.section( ".", 0, 0 ).toUInt();

		bool needPatchVmMultiDisplay = ( uiCfgVer < uiAppVer && uiCfgVer < 5 );
		if (needPatchVmMultiDisplay
			&& (lstPatches.contains(bug438530_MultiDisplay) || patcher != CDspBugPatcherLogic::pkInitService)
			)
			patchField(pVmConfig, true);
	}

	bool needApplyPatch(CDspBugPatcherLogic::PatcherKind patcher) const
	{
		return patcher == CDspBugPatcherLogic::pkInitService
			|| patcher == CDspBugPatcherLogic::pkRegisterVm;
	}
};

class CPatchAutoStartOption : public CXmlPatchBase
{
	static QString getPatchVersion()
	{
		return "0";
	}

public:
	CPatchAutoStartOption()
		: CXmlPatchBase("Settings.Startup.AutoStart", getPatchVersion() ) {}

	virtual void patch(	const SmartPtr<CVmConfiguration>&,
						CDspBugPatcherLogic::PatcherKind,
						const QSet<PKeyId>&)
	{
	}

	bool needApplyPatch(CDspBugPatcherLogic::PatcherKind patcher) const
	{
		return patcher == CDspBugPatcherLogic::pkInitService
			|| patcher == CDspBugPatcherLogic::pkRegisterVm;
	}
};

class CPatchCopyPrintersToNewXmlTag : public CXmlPatchBase
{
public:
	CPatchCopyPrintersToNewXmlTag()
		: CXmlPatchBase("Settings.VirtualPrintersInfo.UseHostPrinters") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		bool bVirtualPrinters = false;
		do
		{
			foreach(CVmParallelPortOld* pPrinterOld, pVmConfig->getVmHardwareList()->m_lstParallelPortOlds)
			{
				CVmParallelPort* pPrinterNew = new CVmParallelPort;

				pPrinterOld->setExtRootTagName("Printer");
				pPrinterNew->fromString(pPrinterOld->toString());
				pPrinterNew->setPrinterInterfaceType(PRN_LPT_DEVICE);
				pPrinterOld->setExtRootTagName("");

				pVmConfig->getVmHardwareList()->m_lstParallelPorts += pPrinterNew;
			}
		}
		while(0);

		CVmVirtualPrintersInfo* pVPInfo = pVmConfig->getVmSettings()->getVirtualPrintersInfo();
		pVPInfo->setUseHostPrinters(bVirtualPrinters);
		pVPInfo->setSyncDefaultPrinter(bVirtualPrinters);
	}
};

class CPatchShowTaskBarEnabled : public CXmlPatchBase
{
public:
	CPatchShowTaskBarEnabled()
		: CXmlPatchBase("Settings.Tools.Coherence.ShowTaskBar") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, true);
	}
};

class CPatchSharedCameraEnabled : public CXmlPatchBase
{
public:
	CPatchSharedCameraEnabled()
		: CXmlPatchBase("Settings.SharedCamera.Enabled") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		unsigned int nOsVersion
			= pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion();
		bool bEnabled = ! ( IS_WINDOWS(nOsVersion) && nOsVersion <= PVS_GUEST_VER_WIN_2K );

		patchField(pVmConfig, bEnabled);
	}
};

class CPatchCornerActionsReset : public CXmlPatchBase
{
public:
	CPatchCornerActionsReset()
		: CXmlPatchBase("Settings.Runtime.FullScreen.CornerAction", "2") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		CVmFullScreen* pObj = pVmConfig->getVmSettings()->getVmRuntimeOptions()->getVmFullScreen();
		pObj->setCornerActions(); // cleanup all actions
		pObj->setCornerAction(PWC_TOP_LEFT_CORNER, PCA_WINDOWED);
	}
};

class CPatchPatchingOptimizePowerCMReset : public CXmlPatchBase
{
public:
	CPatchPatchingOptimizePowerCMReset()
		: CXmlPatchBase("Settings.Runtime.OptimizePowerConsumptionMode") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, PVE::OptimizePerformance);
	}
};

class CPatchGroupAllWindows : public CXmlPatchBase
{
public:
	CPatchGroupAllWindows()
		: CXmlPatchBase("Settings.Tools.Coherence.GroupAllWindows") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, false);
	}
};

class CPatchRelocateTaskBar : public CXmlPatchBase
{
public:
	CPatchRelocateTaskBar()
		: CXmlPatchBase("Settings.Tools.Coherence.RelocateTaskBar") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, false);
	}
};

class CPatchExcludeDock : public CXmlPatchBase
{
public:
	CPatchExcludeDock()
		: CXmlPatchBase("Settings.Tools.Coherence.ExcludeDock") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, true);
	}
};

class CPatchDoNotMinimizeToDock : public CXmlPatchBase
{
public:
	CPatchDoNotMinimizeToDock()
		: CXmlPatchBase("Settings.Tools.Coherence.DoNotMinimizeToDock") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, false);
	}
};

class CPatchBringToFront : public CXmlPatchBase
{
public:
	CPatchBringToFront()
		: CXmlPatchBase("Settings.Tools.Coherence.BringToFront") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, false);
	}
};

class CPatchAlwaysOnTop : public CXmlPatchBase
{
public:
	CPatchAlwaysOnTop()
		: CXmlPatchBase("Settings.Tools.Coherence.AlwaysOnTop") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, false);
	}
};

class CPatchClipboardSyncEnabled : public CXmlPatchBase
{
public:
	CPatchClipboardSyncEnabled()
		: CXmlPatchBase("Settings.Tools.ClipboardSync.Enabled") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, true);
	}
};

class CPatchDragAndDropEnabled : public CXmlPatchBase
{
public:
	CPatchDragAndDropEnabled()
		: CXmlPatchBase("Settings.Tools.DragAndDrop.Enabled") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, true);
	}
};

class CPatchGesturesEnabled : public CXmlPatchBase
{
public:
	CPatchGesturesEnabled()
		: CXmlPatchBase("Settings.Tools.Gestures.Enabled") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, true);
	}
};

class CPatchEnableVTxSupport : public CXmlPatchBase
{
public:
	CPatchEnableVTxSupport()
		: CXmlPatchBase("Hardware.Cpu.EnableVTxSupport") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, PVE::VTxSupportEnabled);
	}
};

class CPatchTimeSyncInterval : public CXmlPatchBase
{
public:
	CPatchTimeSyncInterval()
		: CXmlPatchBase("Settings.Tools.TimeSync.SyncInterval") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, 60);
	}
};

class CPatchMapSharedFoldersOnLetters : public CXmlPatchBase
{
public:
	CPatchMapSharedFoldersOnLetters()
		: CXmlPatchBase("Settings.Tools.SharedFolders.HostSharing.MapSharedFoldersOnLetters") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, true);
	}
};

class CPatchModalityOpacity : public CXmlPatchBase
{
public:
	CPatchModalityOpacity()
		: CXmlPatchBase("Settings.Tools.Modality.Opacity") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>&,
		CDspBugPatcherLogic::PatcherKind ,
		const QSet<PKeyId>& )
	{
	}
};

class CPatchModalityStayOnTop : public CXmlPatchBase
{
public:
	CPatchModalityStayOnTop()
		: CXmlPatchBase("Settings.Tools.Modality.StayOnTop") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& ,
		CDspBugPatcherLogic::PatcherKind ,
		const QSet<PKeyId>& )
	{
	}
};

class CPatchStickyMouse : public CXmlPatchBase
{
public:
	CPatchStickyMouse()
		: CXmlPatchBase("Settings.Runtime.StickyMouse") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
		CDspBugPatcherLogic::PatcherKind ,
		const QSet<PKeyId>& )
	{
		if ( pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion() == PVS_GUEST_VER_WIN_WINDOWS8_1)
			patchField(pVmConfig, true);
	}
};

class CPatchUsbController : public CXmlPatchBase
{
public:
	CPatchUsbController()
		: CXmlPatchBase("Settings.UsbController.UhcEnabled") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
		CDspBugPatcherLogic::PatcherKind ,
		const QSet<PKeyId>& )
	{
		if ( ! pVmConfig->getVmHardwareList()->m_lstUsbDevices.isEmpty() )
			return;

		pVmConfig->setPropertyValue("Settings.UsbController.UhcEnabled", false);
		pVmConfig->setPropertyValue("Settings.UsbController.EhcEnabled", false);
		pVmConfig->setPropertyValue("Settings.UsbController.XhcEnabled", false);
	}
};

class CPatchChipsetVersion : public CXmlPatchBase
{
public:
	CPatchChipsetVersion()
		: CXmlPatchBase("Hardware.Chipset.Version") {}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig, 0);
	}
};

class CPatchVideoMemory : public CXmlPatchBase
{
public:
	CPatchVideoMemory()
		: CXmlPatchBase("Hardware.Video.VideoMemorySize") {}

	virtual void setVmDirUuid(const QString& vmDirUuid) { m_vmDirUuid = vmDirUuid; }

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind pk,
						const QSet<PKeyId>& )
	{
		if ( ! IS_MACOS(pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion()) )
			return;

		if ( pk != CDspBugPatcherLogic::pkInitService && pk != CDspBugPatcherLogic::pkRegisterVm )
			return;

		if ( CDspVm::getVmState(pVmConfig->getVmIdentification()->getVmUuid(), m_vmDirUuid)
				!= VMS_STOPPED )
			return;

		patchField(pVmConfig, 32);
	}
private:
	QString m_vmDirUuid;
};

class CPatchCustomProfile : public CXmlPatchBase
{
public:
	CPatchCustomProfile()
		: CXmlPatchBase("Settings.General.Profile.Custom") {}

	virtual void setHostInfo(const SmartPtr<CHostHardwareInfo>& pHostInfo)
	{
		m_pHostInfo = pHostInfo;
	}

	virtual void patch(	const SmartPtr<CVmConfiguration>& pVmConfig,
						CDspBugPatcherLogic::PatcherKind ,
						const QSet<PKeyId>& )
	{
		patchField(pVmConfig,
			CVmProfileHelper::check_vm_custom_profile(*m_pHostInfo.getImpl(),
													  *pVmConfig.getImpl(),
													  CVmConfiguration(),
													  false) > 0);
	}
private:
	SmartPtr<CHostHardwareInfo> m_pHostInfo;
};

static QMutex* g_mtxXmlPatches = new QMutex;

QList<CXmlPatchBase* > CDspBugPatcherLogic::getXmlPatches()
{
	static QList<CXmlPatchBase* > *pLst = 0;

	if( pLst )
		return *pLst;

	QMutexLocker lock(g_mtxXmlPatches);
	if( pLst )
		return *pLst;

	QList<CXmlPatchBase* > *pTmp = new QList<CXmlPatchBase* >();
	*pTmp << new CPatchVmMultiDisplay
	<< new CPatchAutoStartOption
	<< new CPatchCopyPrintersToNewXmlTag
	<< new CPatchShowTaskBarEnabled
	<< new CPatchSharedCameraEnabled
	<< new CPatchCornerActionsReset
	<< new CPatchPatchingOptimizePowerCMReset
	<< new CPatchGroupAllWindows
	<< new CPatchRelocateTaskBar
	<< new CPatchExcludeDock
	<< new CPatchDoNotMinimizeToDock
	<< new CPatchBringToFront
	<< new CPatchAlwaysOnTop
	<< new CPatchClipboardSyncEnabled
	<< new CPatchDragAndDropEnabled
	<< new CPatchGesturesEnabled
	<< new CPatchEnableVTxSupport
	<< new CPatchTimeSyncInterval
	<< new CPatchMapSharedFoldersOnLetters
	<< new CPatchModalityOpacity
	<< new CPatchModalityStayOnTop
	<< new CPatchStickyMouse
	<< new CPatchUsbController
	<< new CPatchChipsetVersion
	<< new CPatchVideoMemory
	<< new CPatchCustomProfile
	;

	pLst = pTmp;
	return *pLst;
}
