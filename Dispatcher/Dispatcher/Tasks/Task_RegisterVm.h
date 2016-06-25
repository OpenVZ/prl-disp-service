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
///	Task_RegisterVm.h
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

#ifndef __Task_RegisterVm_H_
#define __Task_RegisterVm_H_

#include "CDspTaskHelper.h"
#include "CDspRegistry.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Tasks/Mixin_CreateHddSupport.h"
#include "Tasks/Mixin_CreateVmSupport.h"

enum {
	 REG_SKIP_VM_PARAMS_LOCK	= 0x02,
};

class Task_RegisterVm
	: public Mixin_CreateHddSupport,
	  private Mixin_CreateVmSupport
{
	// friend callback for control hard disk creation progress
	friend bool HddCallbackToRegisterVmTask ( int iDone,
											  int iTotal,
											  void *pUserData);

	Q_OBJECT
public:

	// constructor to create VM
	Task_RegisterVm(Registry::Public&,
			SmartPtr<CDspClient>&,
			const SmartPtr<IOPackage>&,
			const  QString& vm_config,
			const  QString& vm_rootDir,
			int nFlags);

	// constructor to register VM
	Task_RegisterVm(Registry::Public&,
			SmartPtr<CDspClient>&,
			const SmartPtr<IOPackage>&,
			const QString& strPathToVmDirToRegister,
			int nFlags,
			const QString& strCustomVmUuid = QString(),
			const QString& strApplianceId = QString(),
			unsigned int nInternalParamsAsMask = 0);

	virtual ~Task_RegisterVm ();

	virtual QString getVmUuid();

	bool	doRegisterOnly();
	QString	getVmConfig();

	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);

	virtual void reassignTask( SmartPtr<CDspClient>& pNewClient,
								const SmartPtr<IOPackage>& pNewPackage );

	static QString getUniqueVmName( const QString& sVmNamePattern, const QString& vmDirUuid);

	PRL_VM_COLOR getUniqueVmColor( const QString& vmDirUuid );

protected:
	virtual PRL_RESULT		prepareTask();
	virtual void            finalizeTask();

	virtual PRL_RESULT run_body();

	void updateSharedWindowsApplications();

private:
	// to create VM
	void setTaskParameters(
		const  QString& vm_config,
		const  QString& vm_rootDir );

	//to register VM
	void setTaskParameters(
		const QString& strPathToVmDirToRegister );

	PRL_RESULT	createConfigItems( );
	PRL_RESULT	createFloppyDisks( );
	PRL_RESULT	createSerialPorts( );
	PRL_RESULT	createParallelPorts( );
	PRL_RESULT	createHardDisks( );
	PRL_RESULT	createNVRAM();

	void rollback();
	void deleteCreatedConfigItems();

	PRL_RESULT	saveVmConfig( );

	/**
	 * Helper method that converts specified image path to absolute path
	 */
	QString ConvertToFullPath(const QString &sImagePath);

	/**
	* Checks various params for correctness upon VM creation.
	* @param configuration to check against all registered VMs.
	* @note throw PRL_RESULT if errors
	*/
	void checkOperationPermission();

	/**
	* Checks if vm was registered early by another server.
	* set flag when server_uuid  was changed in configuration
	* @param [out] pointer to bool flag pbServerUuidWasChanged
	* @note throw PRL_RESULT if errors
	*/
	void checkVMOnOtherServerUuid( bool *pbServerUuidWasChanged /* out */ );

	/**
	* Checks if vm has dynamic MAC addreeses
	* @param [in] pointer to bool flag pbServerUuidWasChanged
	* @note throw PRL_RESULT if errors
	* @note should be called after checkVMOnOtherServerUuid()
	*/
	void checkDynamicMACAddress();

	/**
	* Checks where from registered vm
	* @param [in] pointer to bool flag pbServerUuidWasChanged
	* @note throw PRL_RESULT if errors
	* @note should be called before checkDynamicMACAddress() and after after checkVMOnOtherServerUuid()
	*/
	void checkWhereFromRegisteredVm( bool bServerUuidWasChanged );

	/**
	* Validate config for create VM.
	* @param configuration to check.
	* @note throw PRL_RESULT if errors
	*/
	void validateNewConfiguration();

	/**
	* Patch VM configuration
	* @param configuration to patch.
	*/
	void patchNewConfiguration();

	/**
	* Regenerate vm_uuid, store it in m_pVmConfig and return it.
	* @return new vm_uuid
	*/
	QString regenerateVmUuid();

	/**
	* Regenerate source_vm_uuid and return it.
	* @return new source_vm_uuid
	*/
	QString regenerateSrcVmUuid();

	// #115173
	bool isVMConfigurationHasDynamicMacAddress(SmartPtr<CVmConfiguration> config);

	void extractMACAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config, QSet<QString>& macs);

	// function generates a new MAC for all adapters in the VmConfiguratioin that aren't static
	// or for MAC that are present in the duplicates.
	void createMACAddress(SmartPtr<CVmConfiguration> config,
			QSet<QString> duplicates = QSet<QString>(), bool bCheckEmpty = false);

	// Check appliance registration
	bool isApplianceRegistered() const { return ! m_strApplianceId.isEmpty(); }

	PRL_RESULT tryToRestoreVmConfig( const QString& sPathToVmDir );

	void PatchNetworkAdapters();

private:
	QString m_vmRootDir; // using to CreateVm Only
	QString m_strPathToVmDirToRegister; // use to Register VM  Only

	CVmDirectory::TemporaryCatalogueItem*	m_pVmInfo;
	SmartPtr<CVmConfiguration>				m_pVmConfig;
	QString m_dirUuid;

	bool m_flgLockRegistred;
	bool m_flgRegisterOnly;

	QStringList m_lstCreatedDirs;
	QStringList m_lstCreatedFiles;

	QMutex		m_mtxWaitCreateImage;
	// list of internal task threads
	CDspTaskHelper*	m_lpcCreateImageCurrentTask;
	// flag to detect empty name from register operation
	bool	m_bVmRegisterNameWasEmpty;

	QString m_strRenamedVmHome; // use to rollback  Register VM  Only + bug 119951

	int m_nFlags;
	QString m_strApplianceId;
	unsigned int m_nInternalParamsAsMasks;
	QString m_sCustomVmUuid;

	bool m_bForceRegisterOnSharedStorage;
	Registry::Public& m_registry;
};

#endif //__Task_RegisterVm_H_
