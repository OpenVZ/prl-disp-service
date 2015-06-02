///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspBugPatcherLogic.h
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

#ifndef _C_DSP_BUGS_PATCHER_LOGIC_H_
#define _C_DSP_BUGS_PATCHER_LOGIC_H_

#include <QList>
#include <QMap>
#include "Libraries/Std/SmartPtr.h"
#include "Libraries/HostInfo/CHostInfo.h"
#include "CDspSync.h"
#include "CVmIdent.h"
#include "CDspVmNetworkHelper.h"

class CVmConfiguration;
class CVmDirectoryItem;
class CDispProxyPreferences;

typedef QString PKeyId;

// Strings for using in others places
#define PARENTAL_CONTROL_ENABLED_STR	"ParentalControlEnabled"

class PatchKeysInitializer
{
	QSet<PKeyId> lstVmPatchKeys;
public:
	const QSet<PKeyId>& getPatches() const { return  lstVmPatchKeys; }

	PatchKeysInitializer();

};

class CXmlPatchBase;

class CDspBugPatcherLogic
{
public:
	CDspBugPatcherLogic( const CHostHardwareInfo& hostInfo);
	~CDspBugPatcherLogic();
	static void patchCommon(const QString& qsOldDispVersion, const QString& qsCurrentDispVersion);
	void patchVmConfigs();

	static void cleanVmPatchMarks(const QString& vmDirUuid, const QString& vmUuid);

	enum PatcherKind
	{
		pkInitService,
		pkRegisterVm,
		pkSwitchToSnapshot,
	};

	void patchOldConfig(const QString& vmDirUuid, const SmartPtr<CVmConfiguration>& pVmConfig, PatcherKind patcher);
	void patchNewConfig(const SmartPtr<CVmConfiguration>& pVmConfig);
	bool patchVmConfig(const CVmIdent& vmId);

private:
	enum RepatchMode
	{
		// NOTE: Values can be used as bit mask
		No_repatch = 0x0,
		PRODUCT_WAS_DOWNGRADED	= 0x1, // major version decreased
		PRODUCT_WAS_UPGRADED		= 0x2, // major version increased
		PD_before_PD6		= 0x4,
		PS_before_PSBM5		= PD_before_PD6,
		BUILD_VERSION_WAS_CHANGED	= 0x8,
	};

	typedef QPair<QString, QString> VmId; // <VmHome, VmUuid>

private:
	// constructor only for patchCommon()
	CDspBugPatcherLogic();

	void collectCommonPatches( quint32 nRepatchMask );
	void patchCommonConfigs( quint32 nRepatchMask );
	QSet<QString> collectVmPatches(const QString& vmDirUuid, const QString& vmUuid);
	bool patchVmConfig(const QString& vmDirUuid, CVmDirectoryItem* item);
	void patchDefaultVmDirectoryPath();
	void patchDispatcherXml(bool & bSaveDispCfg);
	void patchDomainPartOfLocalUserName();
	void migrateOldHostId(bool & bSaveDispCfg);
	/**
	 * Patch speciefic proxy prefs field if dispatcher version was changed.
	 * Stores last patched version in patch_stamp field.
	 *
	 * @param fieldName field to patch
	 * @param value patched value for field.
	 * @param pProxyPrefs proxy preferences for patching.
	 * @return true if field was patched, false otherwise.
	 */
	bool patchFieldIfVerChanged(const QString & fieldName, const QString & value,
								CDispProxyPreferences * pProxyPrefs);
private:

	// This method applies patches stored in config and defined in XML schema of document under tag "patch:"
	// returns true when any patch was applied
	bool applyXmlPatches(const QString& vmDirUuid,
						 const SmartPtr<CVmConfiguration>& pVmConfig,
						 PatcherKind patcher,
						 const QSet<PKeyId>& lstPatches = QSet<PKeyId>());

	// patched from QSettings patch system
	void patchVmHostMacs(const SmartPtr<CVmConfiguration>& pVmConfig,
						 QSet<QString>* pUsedMacs,
						 HostMacUpdateFlags flags);
	void patchAppVersion(const SmartPtr<CVmConfiguration>& pVmConfig);

	static QList<CXmlPatchBase* > getXmlPatches();

private:

	QSet<QString>	m_lstPatches;

	QSet<QString> m_usedMacs;

	SmartPtr<CHostHardwareInfo> m_pHostInfo;
};


#endif //_C_DSP_BUGS_PATCHER_LOGIC_H_
