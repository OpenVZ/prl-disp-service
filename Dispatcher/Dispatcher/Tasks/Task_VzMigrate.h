///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzMigrate.h
///
/// Dispatcher target-side task for Virtuozzo container migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_MigrateCt_H_
#define __Task_MigrateCt_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "CDspDispConnection.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Task_DispToDispConnHelper.h"
#include "Libraries/Virtuozzo/CVzHelper.h"

#define PRL_CT_MIGRATE_CLIENT "/usr/sbin/vzmsrc"
#define PRL_CT_MIGRATE_SERVER "/usr/sbin/vzmdest"
#define PRL_COPT_CT_TEMPLATE_CLIENT "/usr/sbin/vzmtemplate"
#define PRL_COPT_CT_TEMPLATE_SERVER "/usr/sbin/vzmdestmpl"

#define PRL_CT_MIGRATE_OUT_FD		0
#define PRL_CT_MIGRATE_CMD_FD		1
#define PRL_CT_MIGRATE_DATA_FD		2
#define PRL_CT_MIGRATE_TMPLDATA_FD	3
#define PRL_CT_MIGRATE_SWAP_FD		4

struct Task_HandleDispPackage;
/*
  Separate thread for handle of incoming dispatcher-dispatcher packages:
  - wait packages as reply on m_hJob,
  - send buffer to target socket
*/
class Task_VzMigrate : public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

public:
	Task_VzMigrate(const SmartPtr<CDspClient> &client, const SmartPtr<IOPackage> &p);
	~Task_VzMigrate();

protected:
	PRL_RESULT startVzMigrate(const QString& cmd_, const QStringList& args_,
			const CProgressHepler::callback_type& reporter_ = CProgressHepler::callback_type());

	PRL_RESULT execVzMigrate(
			SmartPtr<IOPackage> pParentPackage,
			IOSendJobInterface *pSendJobInterface,
			IOSendJob::Handle &hJob);

	/* send package with migration data to target dispatcher */
	PRL_RESULT sendDispPackage(quint16 nType, quint32 nSize);
	virtual bool isCancelled() { return operationIsCancelled(); }
	virtual PRL_RESULT sendDispPackage(SmartPtr<IOPackage> &)
		{ return PRL_ERR_SUCCESS; }

	void terminateVzMigrate();
	void terminateHandleDispPackageTask();

private:
	PRL_RESULT readFromVzMigrate(quint16 nFdNum);
	void readOutFromVzMigrate();

private:
	QVector<int> m_nFd;

	quint32 m_nBufferSize;
	SmartPtr<char> m_pBuffer;
	SmartPtr<IOPackage> m_pPackage;

#ifdef _WIN_
	ULONG m_pid;
#else
	pid_t m_pid;
#endif
	QString m_sCmd;

	Task_HandleDispPackage *m_pHandleDispPackageTask;
	QMutex m_terminateVzMigrateMutex;
	QMutex m_terminateHandleDispPackageTaskMutex;

	QScopedPointer<CProgressHepler> m_progress;

	void (Task_VzMigrate::*m_pfnTermination)(pid_t);
private:
	void setPidPolicy(pid_t p);
	void terminatePidPolicy(pid_t p);

private slots:
	void handleDispPackageHandlerFailed(PRL_RESULT nRetCode, const QString &sErrInfo);
};

#endif //__Task_MigrateCt_H_
