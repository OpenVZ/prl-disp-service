///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_GetBackupTree.h
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

#ifndef __Task_GetBackupTree_H_
#define __Task_GetBackupTree_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "prlxmlmodel/BackupTree/CBackupDisks.h"

namespace Backup
{
namespace Tree
{
///////////////////////////////////////////////////////////////////////////////
// struct Branch

struct Branch
{
	Branch(const QString& uuid_, const Metadata::Sequence& metadata_):
		m_uuid(uuid_), m_metadata(metadata_)
	{
	}

	BackupItem* show() const;
	BackupItem* filterOne(const QString& sequence_, quint32 number_) const;
	BackupItem* filterChain(const QString& sequence_, quint32 number_) const;

private:
	BackupItem* filterList(quint32 head_, QList<quint32> filter_) const;

	const QString m_uuid;
	Metadata::Sequence m_metadata;
};

} // namespace Tree
} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// class Task_GetBackupTreeSource

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

///////////////////////////////////////////////////////////////////////////////
// class Task_GetBackupTreeTarget

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

	bool isConnected() const;
	bool filterSingleBackup() { return m_nFlags & PBT_BACKUP_ID; }
	bool filterBackupChain() { return m_nFlags & PBT_CHAIN; }
	bool backupFilterEnabled() { return filterSingleBackup() || filterBackupChain(); }

private:
	SmartPtr<CDspDispConnection> m_pDispConnection;
	// VM UUID or backup ID
	QString& m_sUuid;
};

#endif //__Task_GetBackupTree_H_
