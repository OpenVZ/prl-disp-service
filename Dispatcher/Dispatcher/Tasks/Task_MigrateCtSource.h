///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCtSource.h
///
/// Dispatcher source-side task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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

#ifndef __Task_MigrateCtSource_H_
#define __Task_MigrateCtSource_H_

#ifdef _WIN_
# error	Task_MigrateCt_win.h should be used instead
#endif

#include <QString>

#include "CDspClient.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "Task_VzMigrate.h"

class Task_MigrateCtSource : public Task_VzMigrate
{
	Q_OBJECT

public:
	Task_MigrateCtSource(
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &);
	~Task_MigrateCtSource();

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);
	CVmConfiguration* prepareTargetVmConfig() const;
	virtual QString getVmUuid() {return m_sCtUuid;}

	/* send package with migration data to target dispatcher */
	virtual PRL_RESULT sendDispPackage(SmartPtr<IOPackage> &pPackage);

private:
	PRL_RESULT CheckVmMigrationPreconditions();
	PRL_RESULT SendStartRequest();
	void terminate();

private slots:
	void onCheckResponse(IOSendJob::Handle, const SmartPtr<IOPackage>);

private:
	QString m_sCtUuid;
	QString m_sVzDirUuid;
	VIRTUAL_MACHINE_STATE m_nPrevVmState;
	quint32 m_nMigrationFlags;
	quint32 m_nReservedFlags;
	QString m_sServerHostname;
	quint32 m_nServerPort;
	QString m_sServerSessionUuid;
	QString m_sTargetServerCtName;
	QString m_sTargetServerCtHomePath;

	SmartPtr<CVmConfiguration> m_pVmConfig;

	/* preconditions errors set */
	QStringList m_lstCheckPrecondsErrors;
	CHostHardwareInfo m_cHostInfo;

	SmartPtr<IOPackage> m_pReply;

	IOSendJob::Handle m_hCheckReqJob;
	IOSendJob::Handle m_hStartReqJob;

	bool m_bExVmOperationRegistered;
};

#endif //__Task_MigrateCtSource_H_
