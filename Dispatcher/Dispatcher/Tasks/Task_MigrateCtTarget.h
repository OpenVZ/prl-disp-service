///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCtTarget.h
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

#ifndef __Task_MigrateCtTarget_H_
#define __Task_MigrateCtTarget_H_

#ifdef _WIN_
# error	Task_MigrateCt_win.h should be used instead
#endif

#include <QString>

#include "Task_DispToDispConnHelper.h"
#include "CDspClient.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "CDspDispConnection.h"
#include "CDspVm.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "CDspTaskHelper.h"

#ifndef _LIN_
class Task_MigrateCtTarget : public CDspTaskHelper, public Toll::Choke
{
	Q_OBJECT

public:
	Task_MigrateCtTarget(
		const QObject *,
		const SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateCtTarget();

protected:
	virtual PRL_RESULT run_body();

private:
	SmartPtr<CDspDispConnection> m_pDispConnection;
};

#else

#include "Task_VzMigrate.h"
#include "Libraries/Virtuozzo/CVzHelper.h"

class Task_MigrateCtTarget : public Task_VzMigrate, public Toll::Choke
{
	Q_OBJECT

public:
	Task_MigrateCtTarget(
		const SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateCtTarget();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);
	virtual QString getVmUuid() {return m_sCtOrigUuid;}

	/* send package with migration data to target dispatcher */
	virtual PRL_RESULT sendDispPackage(SmartPtr<IOPackage> &pPackage);
	CVzOperationHelper &get_op_helper() { return m_VzOpHelper; }

private:
	QString m_sCtOrigUuid;
	QString m_sCtUuid;
	QString m_sSrcCtUuid;
	QString m_sVzDirUuid;
	QString m_sCtOrigId;
	QString m_sCtNewId;
	QString m_sCtNewName;
	VIRTUAL_MACHINE_STATE m_nPrevVmState;
	quint32 m_nMigrationFlags;
	quint32 m_nReservedFlags;
	QString m_sCtNewPrivate;

	WaiterTillHandlerUsingObject m_waiter;

	SmartPtr<CDspDispConnection> m_pDispConnection;
	SmartPtr<IOPackage> m_pStartPackage;
	CVmConfiguration m_cVmConfig;
	QStringList m_lstCheckPrecondsErrors;
	QString m_sServerUuid;
	IOSender::Handle m_hConnHandle;

	QString m_sSharedFileName;
	quint32 m_nVersion;
	QString m_sVmConfig;
	QString m_sSrcHostInfo;
	quint32 m_nFlags;

	bool m_bExclusiveVmParametersLocked;
	SmartPtr<CVmDirectory::TemporaryCatalogueItem> m_pVmInfo;
	CVzOperationHelper m_VzOpHelper;

private slots:
	void clientDisconnected(IOSender::Handle h);
	void handlePackage(IOSender::Handle handle_,
			const SmartPtr<IOPackage>& package_);
	void handleStartCommandTimeout();

};
#endif // _LIN_

#endif //__Task_MigrateCtTarget_H_
