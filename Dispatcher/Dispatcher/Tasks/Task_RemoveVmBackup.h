///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RemoveVmBackup.h
///
/// Dispatcher source-side task for Vm backup creation
///
/// @author krasnov@
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __Task_RemoveVmBackup_H_
#define __Task_RemoveVmBackup_H_

#include <QString>

#include "CDspInstrument.h"
#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"

class Task_RemoveVmBackupSource : public Task_BackupHelper
{
	Q_OBJECT

public:
	Task_RemoveVmBackupSource(
		SmartPtr<CDspClient> &,
		CProtoCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_RemoveVmBackupSource() {}

protected:
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	QString m_sBackupId;
};

class Task_RemoveVmBackupTarget : public Task_BackupHelper
{
	Q_OBJECT

	typedef Instrument::Command::Batch batch_type;
public:
	Task_RemoveVmBackupTarget(
		SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_RemoveVmBackupTarget() {}

protected:
	Prl::Expected<batch_type, PRL_RESULT> prepare();
	PRL_RESULT do_(batch_type batch_);
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	/* to remove all backups for this Vm */
	Prl::Expected<batch_type, PRL_RESULT> removeAllBackupsForVm();

private:
	QString m_sBackupId;
	QString m_sBackupUuid;
	unsigned m_nBackupNumber;
	SmartPtr<CDspDispConnection> m_pDispConnection;
	int m_nLocked;
	QStringList m_lstBackupUuid;
};

#endif //__Task_RemoveVmBackup_H_
