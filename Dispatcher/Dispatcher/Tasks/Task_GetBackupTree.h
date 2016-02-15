///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_GetBackupTree.h
///
/// Dispatcher source-side task for Vm backup creation
///
/// @author krasnov@
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

#ifndef __Task_GetBackupTree_H_
#define __Task_GetBackupTree_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "prlxmlmodel/BackupTree/CBackupDisks.h"

class Task_GetBackupTreeSource : public Task_BackupHelper
{
	Q_OBJECT

public:
	Task_GetBackupTreeSource(
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_GetBackupTreeSource() {}

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	QString m_sBackupTree;
	// VM UUID or backup ID
	QString& m_sUuid;
};

class Task_GetBackupTreeTarget : public Task_BackupHelper
{
	Q_OBJECT

public:
	Task_GetBackupTreeTarget(
		SmartPtr<CDspDispConnection> &,
		CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_GetBackupTreeTarget() {}

protected:
	virtual PRL_RESULT run_body();

private:
	void getBackupTree(QString &msg);

	template <class T>
	void addBackup(QList<T*>& list, T *backup);

	bool filterSingleBackup() { return m_nFlags & PBT_BACKUP_ID; }
	bool filterBackupChain() { return m_nFlags & PBT_CHAIN; }
	bool backupFilterEnabled() { return filterSingleBackup() || filterBackupChain(); }

	template <class T>
	PRL_RESULT addDisks(T& entry, const VmItem& vm, const QString& uuid, unsigned number);

private:
	SmartPtr<CDspDispConnection> m_pDispConnection;
	// VM UUID or backup ID
	QString& m_sUuid;
};

#endif //__Task_GetBackupTree_H_
