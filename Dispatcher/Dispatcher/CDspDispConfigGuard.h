///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConfigGuard.h
///
/// Dispatcher configuration file wrapper. Responses with right config initialization
/// and lets to determine whether config initialization was done correctly
///
/// @author sandro@
/// @owner sergeym@
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

#ifndef CDspDispConfigGuard_H
#define CDspDispConfigGuard_H

#include <prlcommon/Std/SmartPtr.h>
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>

#include "CDspSync.h"
#include "EditHelpers/CMultiEditMergeDispConfig.h"

/**
 * Dispatcher configuration file wrapper
 *
 * @author sandro@
 */
class CDspDispConfigGuard
{
public:
	CDspDispConfigGuard ();
	~CDspDispConfigGuard ();

	/**
	* enable crash safe mech to save config on disk
	*/
	void enableCrashSafeMech();

	/**
	 * save config to disk
	 */
	PRL_RESULT  saveConfig( bool bNoSaveNetwork = true );
	PRL_RESULT  saveConfig( const QString& path, bool bNoSaveNetwork = true );

	/**
	 * Returns pointer to dispatcher configuration object
	 */
	CDspLockedPointer<CDispatcherConfig> getDispConfig();
	/**
	 * Returns pointer to dispatcher settings configuration part object
	 */
	CDspLockedPointer<CDispatcherSettings> getDispSettings();
	/**
	 * Returns pointer to dispatcher common preferences configuration part object
	 */
	CDspLockedPointer<CDispCommonPreferences> getDispCommonPrefs();
	/**
	 * Returns pointer to dispatcher workspace preferences configuration part object
	 */
	CDspLockedPointer<CDispWorkspacePreferences> getDispWorkSpacePrefs();
	/**
	 * Returns pointer to DispatcherToDispatcher connection preferences configuration part object
	 */
	CDspLockedPointer<CDispToDispPreferences> getDispToDispPrefs();
	/**
	* Returns pointer to dispatcher users preferences object
	*/
	CDspLockedPointer<CDispUsersPreferences> getDispUserPreferences();
	/**
	 * Returns pointer to the proxy preferences.
	 */
	CDspLockedPointer<CDispProxyPreferences> getDispProxyPreferences();

	/**
	* Returns pointer to dispatcher user preferences object
	*/
	CDspLockedPointer<CDispUser> getDispUserByUuid( const QString& userUuid );

	/**
	* Returns pointer to dispatcher user preferences object
	*/
	CDspLockedPointer<CDispUser> getDispUserByName( const QString& userName );

	/**
	* Returns dispatcher user preferences object string representation
	* @param user UUID
	*/
	QString getDispUserByUuidAsString( const QString& userUuid );

	void CleanupConfig();

	// static methods to access to constant values from dispatcher config
	static QString getServerUuid();
	static bool confirmationModeEnabled();
	static bool isConfigCacheEnabled();

	CMultiEditMergeDispConfig* getMultiEditDispConfig();

protected:
	friend class CDspService; // to call static methods to init constant value on dispatcher init.
	static void storeConstantValue_ServerUuid( const QString& val );
	static void storeConstantValue_ConfirmationModeEnabledByDefault( bool val );
	static void storeConstantValue_setConfigCacheEnabled( bool val );

private:
	QMutex m_dispMutex;
	/**
	 * Pointer to wrapping dispatcher configuration object
	 */
	SmartPtr<CDispatcherConfig> m_pDispatcherConfig;

	/**
	 * Multi edit dispatcher config object
	 */
	CMultiEditMergeDispConfig*	m_pMultiEditDispConfig;

};

#endif//CDspDispConfigGuard_H
