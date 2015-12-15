///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConfigGuard.cpp
///
/// Dispatcher configuration file wrapper. Responses with right config initialization
/// and lets to determine whether config initialization was done correctly
///
/// @author sandro@
/// @owner sergeym@
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

#include "CDspDispConfigGuard.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"
#include "Libraries/PrlCommonUtilsBase/ParallelsDirs.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

CDspDispConfigGuard::CDspDispConfigGuard () :
	m_dispMutex( QMutex::Recursive ),
	m_pDispatcherConfig( new CDispatcherConfig ),
	m_pMultiEditDispConfig( new CMultiEditMergeDispConfig )
{
}

CDspDispConfigGuard::~CDspDispConfigGuard ()
{
	CleanupConfig();
	delete m_pMultiEditDispConfig;
}

void CDspDispConfigGuard::enableCrashSafeMech()
{
	if( ! getDispConfig()->enableCrashSafeSaving() )
		WRITE_TRACE(DBG_FATAL, "Unable to enableCrashSafeSaving mech for disp config" );
}


PRL_RESULT  CDspDispConfigGuard::saveConfig( bool bNoSaveNetwork )
{
	QString path = ParallelsDirs::getDispatcherConfigFilePath();
	return saveConfig( path, bNoSaveNetwork );
}

PRL_RESULT CDspDispConfigGuard::saveConfig( const QString& path, bool bNoSaveNetwork )
{
	CDspLockedPointer<CDispatcherConfig> pLockedDispConfig = getDispConfig();
	CDispNetworkPreferences*
		pNetwork = pLockedDispConfig->getDispatcherSettings()
						->getCommonPreferences()->getNetworkPreferences();
	CDispLockedOperationsList*
		pLockedOperationsList = pLockedDispConfig->getDispatcherSettings()->getCommonPreferences()
				->getLockedOperationsList();

	bool oldFlg = pNetwork->getNoSaveFlag();
	pNetwork->setNoSaveFlag( bNoSaveNetwork );

	// patch to not save operations with confirmation
	// #436109 ( we store this values in vmdirectory list to prevent deadlock in CDspAccessManager::checkAccess() )
	QList<PRL_ALLOWED_VM_COMMAND> confirmList = pLockedOperationsList->getLockedOperations();
	pLockedOperationsList->setLockedOperations( );

	CDispBackupSourcePreferences *pBackup = pLockedDispConfig->getDispatcherSettings()
						->getCommonPreferences()->getBackupSourcePreferences();
	pBackup->setSave(false);

	PRL_RESULT save_rc = pLockedDispConfig->saveToFile( path );

	pBackup->setSave(true);
	pNetwork->setNoSaveFlag( oldFlg );
	pLockedOperationsList->setLockedOperations( confirmList );

	if( PRL_FAILED(save_rc) )
	{
		WRITE_TRACE(DBG_FATAL, "Error %s on save dispatcher config. Reason: %ld: %s. path = '%s'"
			, PRL_RESULT_TO_STRING(save_rc)
			, Prl::GetLastError()
			, QSTR2UTF8( Prl::GetLastErrorAsString() )
			, QSTR2UTF8( path )
			);
		return PRL_ERR_DISP_CONFIG_WRITE_ERR;
	}

	return PRL_ERR_SUCCESS;
}


CDspLockedPointer<CDispatcherConfig> CDspDispConfigGuard::getDispConfig()
{
	return CDspLockedPointer<CDispatcherConfig>(&m_dispMutex, m_pDispatcherConfig.getImpl());
}

CDspLockedPointer<CDispatcherSettings> CDspDispConfigGuard::getDispSettings()
{
	return CDspLockedPointer<CDispatcherSettings>(&m_dispMutex, m_pDispatcherConfig->getDispatcherSettings());
}

CDspLockedPointer<CDispCommonPreferences> CDspDispConfigGuard::getDispCommonPrefs()
{
	return CDspLockedPointer<CDispCommonPreferences>(&m_dispMutex, m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences());
}

CDspLockedPointer<CDispWorkspacePreferences> CDspDispConfigGuard::getDispWorkSpacePrefs()
{
	return CDspLockedPointer<CDispWorkspacePreferences>(&m_dispMutex, m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()->getWorkspacePreferences());
}

CDspLockedPointer<CDispToDispPreferences> CDspDispConfigGuard::getDispToDispPrefs()
{
	return CDspLockedPointer<CDispToDispPreferences>(&m_dispMutex,
		m_pDispatcherConfig->getDispatcherSettings()->getCommonPreferences()->getDispToDispPreferences());
}

CDspLockedPointer<CDispUsersPreferences> CDspDispConfigGuard::getDispUserPreferences()
{
	return CDspLockedPointer<CDispUsersPreferences>( &m_dispMutex, m_pDispatcherConfig->getDispatcherSettings()->getUsersPreferences() );
}

CDspLockedPointer<CDispUser> CDspDispConfigGuard::getDispUserByUuid( const QString& userUuid )
{
	CDspLockedPointer<CDispUsersPreferences> pUsersPrefs = getDispUserPreferences();
	QListIterator< CDispUser* > it ( pUsersPrefs->m_lstDispUsers );
	while( it.hasNext() )
	{
		CDispUser* pUser = it.next();
		if( pUser->getUserId() == userUuid )
			return CDspLockedPointer<CDispUser>( &m_dispMutex, pUser );
	}

	return CDspLockedPointer<CDispUser>( &m_dispMutex, 0 );
}

CDspLockedPointer<CDispUser> CDspDispConfigGuard::getDispUserByName( const QString& userName )
{
	CDspLockedPointer<CDispUsersPreferences> pUsersPrefs = getDispUserPreferences();
	QListIterator< CDispUser* > it ( pUsersPrefs->m_lstDispUsers );
	while( it.hasNext() )
	{
		CDispUser* pUser = it.next();
		if( pUser->getUserName() == userName )
			return CDspLockedPointer<CDispUser>( &m_dispMutex, pUser );
	}

	return CDspLockedPointer<CDispUser>( &m_dispMutex, 0 );
}

QString CDspDispConfigGuard::getDispUserByUuidAsString( const QString& userUuid )
{
	CDspLockedPointer<CDispUser> pUser = getDispUserByUuid( userUuid );
	QString sUserProfile;
	if (pUser.getPtr())
		sUserProfile = pUser->toString();
	return sUserProfile;
}

void CDspDispConfigGuard::CleanupConfig()
{
	QMutexLocker locker( &m_dispMutex );
	m_pDispatcherConfig = SmartPtr<CDispatcherConfig>();
}

//////////////////////////////////////////////////////////////////////////
// storage of constants
//////////////////////////////////////////////////////////////////////////
namespace
{
	QString g_sServerUuid;
	bool bConfirmationModeEnabledByDefault = false;
	bool g_bConfigCacheEnabled = false;
}

QString CDspDispConfigGuard::getServerUuid()
{
	return g_sServerUuid;
}
void CDspDispConfigGuard::storeConstantValue_ServerUuid( const QString& val )
{
	g_sServerUuid = val;
}

bool CDspDispConfigGuard::confirmationModeEnabled()
{
	return bConfirmationModeEnabledByDefault;
}
void CDspDispConfigGuard::storeConstantValue_ConfirmationModeEnabledByDefault( bool val )
{
	bConfirmationModeEnabledByDefault = val;
}

bool CDspDispConfigGuard::isConfigCacheEnabled()
{
	return g_bConfigCacheEnabled;
}
void CDspDispConfigGuard::storeConstantValue_setConfigCacheEnabled( bool val )
{
	g_bConfigCacheEnabled = val;
}

CMultiEditMergeDispConfig* CDspDispConfigGuard::getMultiEditDispConfig()
{
	return m_pMultiEditDispConfig;
}
