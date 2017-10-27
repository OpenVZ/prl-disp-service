///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RemoveVmBackup_p.h
///
/// Private part of the dispatcher task for a Vm backup removal at the target
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

#ifndef __TASK_REMOVEVMBACKUP_H__
#define __TASK_REMOVEVMBACKUP_H__

#include "CDspInstrument.h"
#include <boost/variant.hpp>
#include "CDspVmBackupInfrastructure.h"
#include <prlxmlmodel/BackupTree/VmItem.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

class Task_BackupHelper;

namespace Backup
{
typedef Instrument::Command::Batch batch_type;

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

///////////////////////////////////////////////////////////////////////////////
// struct Meta

struct Meta : boost::static_visitor<PRL_RESULT>
{
	Meta(const Item& item_, const Backup::Metadata::Sequence& sequence_):
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

	Item m_item;
	mutable Backup::Metadata::Sequence m_sequence;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

struct Flavor
{
	typedef QList<QSharedPointer<Item> > list_type;

	explicit Flavor(Task_BackupHelper& context_);

	batch_type operator()(const Item& item_);
	PRL_RESULT operator()(const QString& folder_);

	static PRL_RESULT unlink(const QString& file_);

private:
	CDspTaskFailure m_failure;
};

} // namespace Remove

namespace Coalesce
{
///////////////////////////////////////////////////////////////////////////////
// struct Setup

struct Setup
{
	typedef Remove::Item item_type;
	typedef Backup::Metadata::Sequence sequence_type;

	Setup(sequence_type& sequence_, quint32 point_):
		m_point(point_), m_index(sequence_.getIndex()), m_sequence(&sequence_)
	{
	}

	Prl::Expected<item_type, PRL_RESULT> getLeader();
	Prl::Expected<item_type, PRL_RESULT> getVictim();
	Prl::Expected<QList<item_type>, PRL_RESULT> getTail();

private:
	Prl::Expected<item_type, PRL_RESULT> find(quint32 point_);

	quint32 m_point;
	QList<quint32> m_index;
	sequence_type* m_sequence;
	QHash<quint32, item_type> m_cache;
};

///////////////////////////////////////////////////////////////////////////////
// struct Instrument

struct Instrument
{
	Instrument(const QString& home_, Setup& setup_);

	PRL_RESULT addImages();
	PRL_RESULT addBackingStores();
	const CVmConfiguration& getResult() const
	{
		return m_result;
	}

private:
	Setup* m_setup;
	CVmConfiguration m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	typedef ::Libvirt::Instrument::Agent::Vm::Unit agent_type;

	explicit Unit(const agent_type& agent_): m_agent(agent_)
	{
	}

	PRL_RESULT operator()();

private:
	agent_type m_agent;
};

///////////////////////////////////////////////////////////////////////////////
// struct Confectioner

struct Confectioner
{
	typedef Setup::sequence_type sequence_type;

	Confectioner(Task_BackupHelper& context_, const sequence_type& sequence_):
		m_sequence(sequence_), m_context(&context_)
	{
	}

	Prl::Expected<batch_type, PRL_RESULT> operator()(quint32 item_);

private:
	sequence_type m_sequence;
	Task_BackupHelper *m_context;
};

} // namespace Coalesce

namespace Remove
{
///////////////////////////////////////////////////////////////////////////////
// struct Confectioner

struct Confectioner
{
	typedef Prl::Expected<batch_type, PRL_RESULT> result_type;

	Confectioner(const QList<quint32>& index_, quint32 number_):
		m_number(number_), m_index(index_)
	{
	}

	result_type operator()
		(Coalesce::Confectioner producer_,
		 Prl::Expected<VmItem, PRL_RESULT> catalog_) const;
	result_type operator()
		(Flavor flavor_, const Metadata::Sequence& sequence_) const;

private:
	quint32 m_number;
	const QList<quint32> m_index;
};

} // namespace Remove
} // namespace Backup

#endif // __TASK_REMOVEVMBACKUP_H__

