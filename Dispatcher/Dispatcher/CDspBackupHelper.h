///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspBackupHelper.h
///
/// Common backup helper class implementation
///
/// @author krasnov
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

#ifndef CDspBackupHelper_H
#define CDspBackupHelper_H

#include "CDspRegistry.h"
#include "CDspDispConnection.h"
#include "CDspVmBackupInfrastructure.h"

class CDspTaskManager;

namespace Backup
{
namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Launcher

struct Launcher
{
	Launcher(Registry::Public& registry_,
		const SmartPtr<CDspTaskManager>& taskManager_, Activity::Service& service_):
		m_registry(registry_), m_service(&service_), m_taskManager(taskManager_)
	{
	}

	/**
	 * Initiates VM backup create source action on client side
	 * @param incoming connection handler
	 * @param connection client
	 * @param pointer to request package from client
	 * @param internal flags (Vm (= 0) or Ct (=PVM_CT_BACKUP))
	 */
	void startCreateCtBackupSourceTask(SmartPtr<CDspClient> actor_,
		const SmartPtr<IOPackage>& package_) const;
	void startCreateVmBackupSourceTask(SmartPtr<CDspClient> actor_,
		const SmartPtr<IOPackage>& package_) const;

	/**
	 * Initiates VM backup restore target action on client side
	 * @param incoming connection handler
	 * @param connection client
	 * @param pointer to request package from client
	 */
	void startRestoreVmBackupTargetTask(
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg) const;

	/**
	 * Initiates VM backup create target action on server side
	 * @param pointer to dispatcher connection object which requested backup create action
	 * @param pointer to request package from remote dispatcher
	 */
	void startCreateVmBackupTargetTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage> &p) const;

	/**
	 * Initiates VM backup restore source action on server side
	 * @param pointer to dispatcher connection object which requested backup restore action
	 * @param pointer to request package from remote dispatcher
	 */
	void startRestoreVmBackupSourceTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage> &p) const;

	/**
	 * Retranslate 'get backup tree' request into server and wait answer
	 * @param connection client
	 * @param pointer to request package from client
	 */
	void startGetBackupTreeSourceTask(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg) const;

	/**
	 * Process 'get backup tree' request from remote dispatcher
	 * @param pointer to dispatcher connection object which requested backup restore action
	 * @param pointer to request package from remote dispatcher
	 */
	void startGetBackupTreeTargetTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage> &p) const;

	/**
	 * Initiates VM backup removing action on client side
	 * @param connection client
	 * @param pointer to request package from client
	 */
	void startRemoveVmBackupSourceTask(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg) const;

	/**
	 * Process 'remove backup' request from remote dispatcher
	 * @param pointer to dispatcher connection object which requested backup restore action
	 * @param pointer to request package from remote dispatcher
	 */
	void startRemoveVmBackupTargetTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage> &p) const;

	void launchEndVeBackup(const SmartPtr<CDspClient>& actor_,
		const SmartPtr<IOPackage>& request_) const;
	void launchBeginCtBackup(const SmartPtr<CDspClient>& actor_,
		const SmartPtr<IOPackage>& request_) const;
	void launchBeginVmBackup(const SmartPtr<CDspClient>& actor_,
		const SmartPtr<IOPackage>& request_) const;

private:
	Registry::Public& m_registry;
	template<class T, class U>
	void launch(SmartPtr<T>& actor_, U factory_,
		const SmartPtr<IOPackage>& package_) const;

	Activity::Service* m_service;
	SmartPtr<CDspTaskManager> m_taskManager;
};

} // namespace Task
} // namespace Backup

#endif //CDspBackupHelper_H
