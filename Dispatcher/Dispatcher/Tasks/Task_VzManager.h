///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzManager.h
///
/// @author igor
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

#ifndef __Task_VzManager_H__
#define __Task_VzManager_H__

#include "CDspService.h"
#include "CDspVzHelper.h"
#include "CDspTaskHelper.h"
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include "Libraries/Virtuozzo/CVzHelper.h"

using namespace Parallels;

class Task_VzManager : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_VzManager(const SmartPtr<CDspClient>& pClient,
			const SmartPtr<IOPackage>& p);

	Task_VzManager(const SmartPtr<CDspClient>& pClient,
			const SmartPtr<IOPackage>& p,
			const QString &sUuid, VIRTUAL_MACHINE_STATE nState);

private:
	CProtoCommandDspWsResponse *getResponseCmd();
	void sendEvent(PRL_EVENT_TYPE type, const QString &sUuid);
	PRL_RESULT insert_diritem(SmartPtr<CVmConfiguration> &pConfig);
	PRL_RESULT check_env_state(PRL_UINT32 nCmd, const QString &sUuid);
	PRL_RESULT create_env();
	PRL_RESULT setupFirewall(SmartPtr<CVmConfiguration> &pConfig);
	PRL_RESULT start_env();
	PRL_RESULT restart_env();
	PRL_RESULT stop_env();
	PRL_RESULT mount_env();
	PRL_RESULT umount_env();
	PRL_RESULT suspend_env();
	PRL_RESULT resume_env();
	PRL_RESULT delete_env();
	PRL_RESULT editConfig();
	PRL_RESULT register_env();
	PRL_RESULT unregister_env();
	PRL_RESULT set_env_userpasswd();
	PRL_RESULT auth_env_user();
	PRL_RESULT clone_env();
	PRL_RESULT create_env_disk();
	PRL_RESULT resize_env_disk();
	PRL_RESULT process_state();
	PRL_RESULT process_cmd();
	PRL_RESULT send_snapshot_tree();
	PRL_RESULT create_env_snapshot();
	PRL_RESULT delete_env_snapshot();
	PRL_RESULT switch_env_snapshot();
	PRL_RESULT start_vnc_server(QString sCtUuid, bool onCtStart);
	PRL_RESULT stop_vnc_server(QString sCtUuid, bool onCtStop);
	PRL_RESULT move_env();
	PRL_RESULT send_network_settings();
	PRL_RESULT send_problem_report();
	PRL_RESULT commit_encryption();

private:
	PRL_RESULT checkAndLockRegisterParameters(
			CVmDirectory::TemporaryCatalogueItem *pVmInfo);
	SmartPtr<CVzOperationHelper> get_op_helper() { return m_pVzOpHelper; }
	SmartPtr<CDspVzHelper> getVzHelper() { return CDspService::instance()->getVzHelper(); }
	PRL_RESULT changeVNCServerState(SmartPtr<CVmConfiguration> pOldConfig,
			SmartPtr<CVmConfiguration> pNewConfig, QString sUuid);
	void sendProgressEvt(PRL_EVENT_TYPE type, const QString &sUuid,
			const QString &sStage, int progress);
	static void sendEvt(void *obj, PRL_EVENT_TYPE type, const QString &sUuid,
			const QString &sStage, int data);
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);
	virtual QString getVmUuid() {return m_sVmUuid;}

private:
	QString m_sVzDirUuid;
	CProtoCommandPtr m_pResponseCmd;
	SmartPtr<CVzOperationHelper> m_pVzOpHelper;
	bool m_bProcessState;
	VIRTUAL_MACHINE_STATE m_nState;
	QString m_sVmUuid;
};

#endif	// __Task_VzManager_H__
