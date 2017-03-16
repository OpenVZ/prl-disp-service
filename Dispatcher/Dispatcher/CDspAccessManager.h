/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef H__CDspAccessManager__H
#define H__CDspAccessManager__H

#include <prlxmlmodel/VmDirectory/CVmDirectories.h>
#include "CDspClient.h"

// ACCESS_MODE
typedef PRL_UINT32 PRL_SEC_AM;


class CVmConfiguration;

class CDspAccessManager
{
	friend class CDspVmDirHelper;
	friend class CDspVmSnapshotStoreHelper;
	friend class CDspVm;
public:
	class VmAccessRights
	{
		friend class CDspAccessManager;
	public:
		enum AccessRights
		{
			arCanNone = 0,
			arCanRead=1, arCanWrite=2, arCanExecute=4,
			arVmIsExists=8,  // vm is exists on physical drive.
		};
		bool canRead() const;
		bool canWrite() const;
		bool canExecute() const;

		bool isExists() const;
		PRL_SEC_AM getVmAccessRights() const;

		static PRL_SEC_AM makeModeNO_ACCESS();
		static PRL_SEC_AM makeModeR();
		static PRL_SEC_AM makeModeRW();
		static PRL_SEC_AM makeModeRX();
		static PRL_SEC_AM makeModeRWX();

		VmAccessRights( const VmAccessRights& );
	private:
		VmAccessRights( PRL_SEC_AM );
	private:
		PRL_SEC_AM m_mode;
	};

public:
	CDspAccessManager();
	~CDspAccessManager();


	//////////////////////////////////////////////////////////////////////////
	//
	//   permission operations
	//
	//////////////////////////////////////////////////////////////////////////
	/**
	* Method check access for user to command's VM
	* return PRL_ERR_SUCCESS when access granted
	* return PRL_ERR_ACCESS_TO_VM_DENIED when access is denied or VM is not found by specified uuid
	* return PRL_ERR_VM_CONFIG_DOESNT_EXIST no configuration file found in directory
	* return PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID you are trying to operate VM, registered on another server
	* return PRL_ERR_VM_CONFIG_INVALID_VM_UUID specified VM configuration differs from one on disk.
	* return PRL_ERR_PARSE_VM_CONFIG configuration file is broken.
	* @param pSession	- user session object
	* @param cmd		- IDispatcherCommands
	* @param vmUuid/vmDirItem		- Vm Uuid / Virtual Machine, presented by cached item
	* @param bSetNotValid			- check invalid state
	 * @param pErrorInfo - pointer to additional error info (necessary error message parameters such as VM name
	 *			and etc. will be stored here if operation'll fail)
	**/
	PRL_RESULT checkAccess( SmartPtr<CDspClient> pSession, PVE::IDispatcherCommands cmd
		, const QString& vmUuid, bool *bSetNotValid = NULL, CVmEvent *pErrorInfo = NULL );
	PRL_RESULT checkAccess( SmartPtr<CDspClient> pSession, PVE::IDispatcherCommands cmd
		, CVmDirectoryItem* pVmDirItem, bool *bSetNotValid = NULL, CVmEvent *pErrorInfo = NULL );

	// check access for non vm operations and for all vm confirmations
	PRL_RESULT checkAccess( SmartPtr<CDspClient> pSession, PVE::IDispatcherCommands cmd );

	/**
	* @brief return AccessRights for user to VM
	* @param pSession	- user session object
	* @param vmUuid		- Vm Uuid
	**/
	VmAccessRights getAccessRightsToVm( SmartPtr<CDspClient> pSession, const QString& vmUuid );
	VmAccessRights getAccessRightsToVm( SmartPtr<CDspClient> pSession, const CVmDirectoryItem* pVmDirItem );

	/**
	* @brief return list of allowed commands for user to VM
	* @param pSession	- user session object
	* @param vmUuid		- Vm Uuid
	**/
	QList< PRL_ALLOWED_VM_COMMAND > getAllowedVmCommands(
		SmartPtr<CDspClient> pSession,
		const CVmDirectoryItem* pVmDirItem  );

	/**
	* Method return access rights to VM for owner and others users
	* @return PRL_ERR_SUCCESS when successfully
	* @param pVmDirItem		[in]- VmDirItem
	* @param ownerAccess	[out] access mask for owner
	* @param othersAccess	[out] access mask for others users
	* @param flgOthersAccessIsMixed [out] flag to show mixed others permissions
	* @param _current_user [in] reference to the current user operating with VM
	**/
	PRL_RESULT getFullAccessRightsToVm( const CVmDirectoryItem* pVmDirItem
		, PRL_SEC_AM& ownerAccess
		, PRL_SEC_AM& othersAccess
		, bool& flgOthersAccessIsMixed
		, CAuthHelper &_current_user );

	/**
	* Method set access rights to VM for owner and others users
	* @return PRL_ERR_SUCCESS when successfully
	* @param pSession		[in] user session object
							( set NULL to ignore isOwner check )
	* @param pVmDirItem		[in] VmDirItem
	* @param ownerAccess	[in] access mask for owner
	*						( set NULL to ignore mask )
	* @param othersAccess	[in] access mask for others users ( set NULL to ignore mask )
	*						( set NULL to ignore mask )
	**/
	PRL_RESULT setFullAccessRightsToVm( SmartPtr<CDspClient> pSession
		, const CVmDirectoryItem* pVmDirItem
		, const PRL_SEC_AM* ownerAccess
		, const PRL_SEC_AM* othersAccess);

	/**
	* Method SETS ACCESS RIGHTS to dir or file by path for owner and others users
	* according Vm rights
	* Also this method SETS OWNER according owner of config.pvs
	* @return PRL_ERR_SUCCESS when successfully
	* @param sPath			[in] path to file or dir
	* @param pSession		[in] user session object
	* @param pVmDirItem		[in] VmDirItem
	* @param bRecursive		[in] flag to define recursive or not
	**/
	PRL_RESULT
		setOwnerAndAccessRightsToPathAccordingVmRights(	const QString& sPath
		, SmartPtr<CDspClient> pSession
		, const CVmDirectoryItem* pVmDirItem
		, bool bRecursive =  true);

	static PRL_RESULT setOwnerByTemplate(	const QString& sPath
		, const QString& sTemplateVmConfigPath
		, CAuthHelper& authHelper
		, bool bRecursive =  true);

	/**
	* Method get owner of VM
	* @return owner name when successfully
	*		  or empty string if owner not defined( network resource / filesystem  not supported /.. )
	* @param pVmDirItem		[in]- VmDirItem
	**/
	QString getOwnerOfVm( const CVmDirectoryItem* pVmDirItem  );

	/**
	* Method set owner to files ( = CFileHelper::setOwner() + dispatcher scpecific )
	* @return PRL_ERR_SUCCESS when successfully
	**/
	static bool setOwner( const QString & strFileName, CAuthHelper* pAuthHelper, bool bRecursive);

	/**
	* Method get owner of VM and compare with users of pSession
	* @return true when is equal
	* @return false when isn't equal
	* @param pSession	- [in] user session object
	* @param pVmDirItem		[in]- VmDirItem
	**/
	bool isOwnerOfVm( SmartPtr<CDspClient> pSession, const CVmDirectoryItem* pVmDirItem  );

private:
	void	initAccessRights();

public:
	static PRL_SEC_AM ConvertCAuthPermToVmPerm( const CAuth::AccessMode& vmConfigMode
			, const CAuth::AccessMode& vmDirMode);
	static void ConvertVmPermToCAuthPerm( const PRL_SEC_AM& vmPerm, CAuth::AccessMode& vmConfigMode
			, CAuth::AccessMode& vmDirMode);

private:
	typedef QPair< PRL_SEC_AM, PRL_ALLOWED_VM_COMMAND > AccessRigthsPair;
	QHash<PVE::IDispatcherCommands, AccessRigthsPair >  m_accessRights;
};
#endif //H__CDspAccessManager__H
