///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmMigrateHelper.h
///
/// VM migration helper class implementation
///
/// @author sandro
/// @owner sergeym
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

#ifndef CDspVmMigrateHelper_H
#define CDspVmMigrateHelper_H

#include "CDspDispConnection.h"
#include "CDspRegistry.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include "CDspVm.h"

/**
 * VM migration helper class implementation
 */
class CDspVmMigrateHelper
{

public:
	CDspVmMigrateHelper(Registry::Public& registry_): m_registry(registry_)
	{
	}
	/**
	 * Checks preconditions before VM migrate action
	 * @param pointer to dispatcher connection object which requested check preconditions action
	 * @param pointer to VM migration check preconditions request package
	 */
	void checkPreconditions(SmartPtr<CDspDispConnection> pDispConnection, const SmartPtr<IOPackage> &p);

	/**
	 * Cancel Vm migration task
	 * @param smart pointer to user session
	 * @param pointer to cancel package
	 * @param pointer to Vm object
	 */
	void cancelMigration(
		const SmartPtr<CDspClient> &pSession,
		const SmartPtr<IOPackage> &p,
		const SmartPtr<CDspVm> &pVm);
	void cancelMigration(
		const SmartPtr<CDspClient> &pSession,
		const SmartPtr<IOPackage> &p,
		const QString& uuid_);

private:
	SmartPtr<CDspTaskHelper> findTask(QString sVmUuid);

	Registry::Public& m_registry;
};

#endif //CDspVmMigrateHelper_H
