////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Task_DeleteVm.h
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

#ifndef __Task_DeleteVm_H_
#define __Task_DeleteVm_H_

#include "Dispatcher/Dispatcher/CDspTaskHelper.h"
//#include "CDspTaskHelper.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

typedef enum _PRL_VM_DELETE_FLAGS
{
	PVD_UNREGISTER_ONLY		= (1 << 1),
	PVD_SKIP_VM_OPERATION_LOCK	= (1 << 2),
	PVD_NOT_MODIFY_VM_CONFIG	= (1 << 3),
	PVD_SKIP_HA_CLUSTER		= (1 << 4),
} PRL_VM_DELETE_FLAGS;

class Task_DeleteVm: public CDspTaskHelper
{
	Q_OBJECT
public:
	Task_DeleteVm ( SmartPtr<CDspClient>&,
					const SmartPtr<IOPackage>&,
					const QString& vm_config,
					PRL_UINT32 flags,
					const QStringList & strFilesToDelete = QStringList());

	virtual QString getVmUuid();

	bool doUnregisterOnly();

protected:
	virtual PRL_RESULT prepareTask();
	virtual void				finalizeTask();


	virtual PRL_RESULT run_body();

private:
	void setTaskParameters(	const QString& vm_uuid );

	/**
	* check if user is authorized to access this VM
	*/
	PRL_RESULT checkUserAccess( CDspLockedPointer<CVmDirectoryItem> pDirectoryItem );

	/**
	* Notify all users that VM was removed
	*/
	void postVmDeletedEvent();

private:
	PRL_UINT32 m_flags;

	SmartPtr<CVmConfiguration> m_pVmConfig;
	bool m_flgVmWasDeletedFromSystemTables;

	bool m_flgExclusiveOperationWasRegistred;

	bool			m_flgLockRegistred;

	QScopedPointer<CVmDirectory::TemporaryCatalogueItem>  m_pVmInfo;

	// list to delete
	QStringList		m_strListToDelete;

	QString m_sVmHomePath;
	QString m_vmDirectoryUuid;
};



#endif //__Task_DeleteVm_H_
