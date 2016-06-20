///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RemoveVmBackup.h
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

#ifndef __Task_RemoveVmBackup_H_
#define __Task_RemoveVmBackup_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"

namespace Backup
{
namespace Remove
{

typedef boost::variant<BackupItem, PartialBackupItem> item_type;

///////////////////////////////////////////////////////////////////////////////
// struct Item

struct Item
{
	Item() : m_number(0) {}

	Item(const QString& path_,  const QString& ve_, const QString& uuid_)
		: m_ve(ve_), m_uuid(uuid_), m_number(0)
	{
		m_dir = QFileInfo(QDir(path_), PRL_BASE_BACKUP_DIRECTORY);
	}

	void setNumber(quint32 number_)
	{
		m_number = number_;
		if (m_number >= PRL_PARTIAL_BACKUP_START_NUMBER)
			m_dir.setFile(m_dir.absoluteDir(), QString::number(number_));
	}
	quint32 getNumber() const
	{
		return m_number;
	}
	PRL_RESULT load(Task_BackupHelper *context_);
	const QStringList& getFiles() const
	{
		return m_files;
	}
	QString getDir() const
	{
		return m_dir.absoluteFilePath();
	}
	const item_type& getData() const
	{
		return m_data;
	}

private:
	QString m_ve;
	QString m_uuid;
	quint32 m_number;
	QStringList m_files;
	QFileInfo m_dir;
	item_type m_data;
};

typedef QSharedPointer<Item> element_type;
typedef QList<element_type> list_type;
typedef boost::function0<PRL_RESULT> action_type;

///////////////////////////////////////////////////////////////////////////////
// struct Meta

struct Meta : boost::static_visitor<PRL_RESULT>
{
	explicit Meta(const QString& path_)
	{
		m_file = QString("%1/" PRL_BACKUP_METADATA).arg(path_);
	}
	
	PRL_RESULT operator()(const PartialBackupItem& from_, const BackupItem& to_) const;
	PRL_RESULT operator()(const PartialBackupItem& from_, const PartialBackupItem& to_) const;

	template<class T, class Y>
	PRL_RESULT operator()(const T&, const Y&) const
	{
		return PRL_ERR_INVALID_ARG;
	}

private:
	QString m_file;
};

///////////////////////////////////////////////////////////////////////////////
// struct Remover

struct Remover
{
	explicit Remover(Task_BackupHelper& context_) : m_context(&context_) {}

	static PRL_RESULT rmdir(const QString& dir_, CDspTaskFailure fail_);
	static PRL_RESULT unlink(const QString& file_);
	static QList<action_type> unlinkItem(const Item& item_,
				const CDspTaskFailure& fail_);
	QList<action_type> operator()(const list_type& objects_, quint32 index_);

private:
	Task_BackupHelper *m_context;
};

///////////////////////////////////////////////////////////////////////////////
// struct Shifter

struct Shifter
{
	typedef boost::function1<PRL_RESULT, unsigned> updater_type;

	Shifter(Task_BackupHelper& context_, updater_type updater_)
		: m_context(&context_), m_updater(updater_) {}

	static PRL_RESULT move(const QString& from_, const QString& to_);
	static PRL_RESULT moveDir(const QString& from_, const QString& to_);
	QList<action_type> moveItem(const Item& from_, const Item& to_);
	static PRL_RESULT rebase(const QString& file_, const QString& base_, bool safe_);
	static QList<action_type> rebaseItem(const Item& item_, const Item& base_, bool safe_);
	QList<action_type> operator()(const list_type& objects_, quint32 index_);

private:
	Task_BackupHelper *m_context;
	updater_type m_updater;
};

} // namespace Remove
} // namespace Backup

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

public:
	Task_RemoveVmBackupTarget(
		SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_RemoveVmBackupTarget() {}

protected:
	Prl::Expected<QList<Backup::Remove::action_type>, PRL_RESULT> prepare();
	PRL_RESULT do_(const QList<Backup::Remove::action_type>& actions_);
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	/* to remove all backups for this Vm */
	Prl::Expected<QList<Backup::Remove::action_type>, PRL_RESULT> removeAllBackupsForVm();

private:
	QString m_sBackupId;
	QString m_sBackupUuid;
	unsigned m_nBackupNumber;
	SmartPtr<CDspDispConnection> m_pDispConnection;
	int m_nLocked;
	QStringList m_lstBackupUuid;
};

#endif //__Task_RemoveVmBackup_H_
