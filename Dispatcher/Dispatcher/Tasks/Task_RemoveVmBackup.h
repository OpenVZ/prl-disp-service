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
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
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
	Item() : m_number()
	{
	}
	explicit Item(quint32 number_): m_number(number_)
	{
	}

	quint32 getNumber() const
	{
		return m_number;
	}
	PRL_RESULT load(const Backup::Metadata::Sequence& sequence_);
	const QStringList& getFiles() const
	{
		return m_files;
	}
	const QString& getLair() const
	{
		return m_lair;
	}
	const item_type& getData() const
	{
		return m_data;
	}

private:
	quint32 m_number;
	QStringList m_files;
	QString m_lair;
	item_type m_data;
};

typedef QSharedPointer<Item> element_type;
typedef QList<element_type> list_type;
typedef boost::function<PRL_RESULT()> action_type;

///////////////////////////////////////////////////////////////////////////////
// struct Meta

struct Meta : boost::static_visitor<PRL_RESULT>
{
	Meta(element_type item_, const Backup::Metadata::Sequence& sequence_):
		m_item(item_), m_sequence(sequence_)
	{
	}
	
	PRL_RESULT operator()(const BackupItem& from_, const PartialBackupItem& to_) const;
	PRL_RESULT operator()(const PartialBackupItem& from_, const PartialBackupItem& to_) const;
	template<class T, class Y>
	PRL_RESULT operator()(const T&, const Y&) const
	{
		return PRL_ERR_INVALID_ARG;
	}

private:
	qulonglong getSize() const;

	element_type m_item;
	mutable Backup::Metadata::Sequence m_sequence;
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
	Shifter(Task_BackupHelper& context_, const Backup::Metadata::Sequence& sequence_):
		m_context(&context_), m_sequence(sequence_)
	{
	}

	static PRL_RESULT rebase(const QString& file_, const QString& base_);
	static QList<action_type> rebaseItem(const Item& item_, const Item& base_);
	QList<action_type> operator()(quint32 item_);

	void setNext(quint32 value_);
	void setPreceding(quint32 value_);

private:
	Task_BackupHelper *m_context;
	Backup::Metadata::Sequence m_sequence;
	element_type m_next, m_preceding;
};

///////////////////////////////////////////////////////////////////////////////
// struct Confectioner

struct Confectioner
{
	typedef Prl::Expected<QList<action_type>, PRL_RESULT> result_type;

	Confectioner(const QList<quint32>& index_, quint32 number_):
		m_number(number_), m_index(index_)
	{
	}

	result_type operator()
		(Shifter producer_,Prl::Expected<VmItem, PRL_RESULT> catalog_) const;
	result_type operator()
		(Remover producer_, const Metadata::Sequence& sequence_) const;

private:
	quint32 m_number;
	const QList<quint32> m_index;
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
