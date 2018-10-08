////////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVzHelper.h
///
/// @author igor
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
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVzHelper_H_
#define __CDspVzHelper_H_

#include "CDspClient.h"
#include "CDspBackupHelper.h"
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Std/SmartPtr.h>
#include "CDspTaskHelper.h"
#include <prlxmlmodel/HostHardwareInfo/CSystemStatistics.h>
#include "Dispatcher/Dispatcher/CDspDispConnection.h"
#include "Dispatcher/Dispatcher/Cache/Cache.h"

#ifdef _CT_
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "Libraries/Virtuozzo/CVzTemplateHelper.h"
#endif

class CDspService;
class CDspVNCStarter;

class CDspVzHelper
{
#ifdef _CT_
public:
	typedef SmartPtr<CDspVNCStarter> vncServer_type;

	CDspVzHelper(CDspService& service_,
		const Backup::Task::Launcher& backup_,
		::Vm::Directory::Ephemeral& ephemeral_);
	~CDspVzHelper();

public:
	PRL_RESULT insertVmDirectoryItem(SmartPtr<CVmConfiguration> &pConfig);
	PRL_RESULT fillVzDirectory(CVmDirectory *pDir);
	SmartPtr<CVmConfiguration> getCtConfig(
			SmartPtr<CDspClient> pUserSession,
			const QString &sUuid,
			const QString &sHome = QString(),
			bool bFull = false);
	SmartPtr<CVmConfiguration> getConfig(
			SmartPtr<CDspClient> pUserSession,
			const QString &sUuid,
			const QString &sHome,
			bool bFull = false);
	PRL_RESULT getCtConfigList(SmartPtr<CDspClient> pUserSession,
			quint32 nFlags,
			QList<SmartPtr<CVmConfiguration> > &lstConfig);
	PRL_RESULT check_env_state(PRL_UINT32 nCmd, const QString &sUuid, CVmEvent *errEvt);

	// Handle Container Command
	// return is commend were processed sign
	bool handlePackage(const IOSender::Handle& sender,
			SmartPtr<CDspClient> &pUserSession,
			const SmartPtr<IOPackage>& pkg);
	// Handle Dispatcher-Dispatcher command for Container & Template
	bool handleToDispatcherPackage(
			SmartPtr<CDspDispConnection> pDispConnection,
			const SmartPtr<IOPackage>& p);

	void initVzStateMonitor();

	CVzHelper &getVzlibHelper() { return m_VzlibHelper; }
	CVzTemplateHelper &getVzTemplateHelper() { return m_VzTemplateHelper; }

	// add item into container's vnc server hash table
	bool addCtVNCServer(const QString &uuid, vncServer_type vncServer_);
	// terminate vnc server for CT ctid and remove appropriaate item from table
	PRL_RESULT removeCtVNCServer(const QString &uuid, bool onCtStop);
	// terminate vnc servers for all containers and clear table
	void removeAllCtVNCServer();
	bool isCtVNCServerRunning(const QString &uuid);
	bool sendCtConfigByUuid(const IOSender::Handle& sender,
                SmartPtr<CDspClient> pUserSession,
                const SmartPtr<IOPackage>& pkg,
                QString vm_uuid );
#ifdef _LIN_
	void syncCtsUptime();
#endif
	class CConfigCache
	{
	public:
		CConfigCache();
		SmartPtr<CVmConfiguration> get_config(const QString& sPath,
				SmartPtr<CDspClient> pUserSession);
		void update(const QString& sPath, const SmartPtr<CVmConfiguration> &pConfig,
				SmartPtr<CDspClient> pUserSession);
		void remove(const QString& sHome);
	private:
		SmartPtr< CacheBase<CVmConfiguration> > m_configCache;
	};

	CConfigCache &getConfigCache()
	{
		return m_configCache;
	}

private:
	bool checkAccess(SmartPtr<CDspClient> &pUserSession);
	void sendCtInfo(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUserSession,
			const SmartPtr<IOPackage>& pkg );
	void sendCtConfigSample(const IOSender::Handle& sender,
                SmartPtr<CDspClient> pUserSession,
                const SmartPtr<IOPackage>& pkg );
	void sendCtConfig(const IOSender::Handle& sender,
                SmartPtr<CDspClient> pUserSession,
                const SmartPtr<IOPackage>& pkg );
	void beginEditConfig(const IOSender::Handle& sender,
                SmartPtr<CDspClient> pUserSession,
                const SmartPtr<IOPackage>& pkg );
	void editConfig(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUserSession,
			const SmartPtr<IOPackage>& pkg );
	QList<PRL_ALLOWED_VM_COMMAND> getAllowedCommands();
	PRL_RESULT fillCtInfo(SmartPtr<CDspClient> pUserSession,
			const SmartPtr<CVmConfiguration>& config,
			CVmEvent& outVmEvent);
	void resetCtUptime(const QString &vm_uuid, SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage> &p);
	void registerGuestSession(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage> &p);
	void guestRunProgram(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage> &p);
	void sendCtNetworkSettings(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUserSession,
			const SmartPtr<IOPackage>& pkg );

	/* Retrieves list of Container templates from the node */
	void sendCtTemplateList(SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage> &p);

	/* Removes Container template from the node */
	void removeCtTemplate(SmartPtr<CDspClient> pUser,
		const SmartPtr<IOPackage> &p);

	/* start source-side task for copy of CT template */
	void copyCtTemplate(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	void appendAdvancedParamsToCtConfig(
			SmartPtr<CDspClient> pUserSession,
			SmartPtr<CVmConfiguration> pOutConfig);
	void UpdateHardDiskInformation(SmartPtr<CVmConfiguration> &config);

private:
	CConfigCache m_configCache;
	CVzHelper m_VzlibHelper;
	CVzTemplateHelper m_VzTemplateHelper;

	// hash table for Container's VNC server: Key = CtID, value = vncServer_type
	QHash<QString, vncServer_type> m_tblCtVNCServer;
	// and mutex for this table
	QMutex m_tblCtVNCServerMtx;
	CDspService* m_service;
	Backup::Task::Launcher m_backup;
	::Vm::Directory::Ephemeral* m_ephemeral;
#endif
};

#endif // __CDspVzHelper_H_
