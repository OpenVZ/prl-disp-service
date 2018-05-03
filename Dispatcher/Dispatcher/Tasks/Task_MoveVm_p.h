///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MoveVm_p.h
///
/// Move Vm private stuff declarations
///
/// @author shrike
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

#ifndef __TASK_MOVEVM_P_H__
#define __TASK_MOVEVM_P_H__

#include <QList>
#include <QPair>
#include <QString>
#include <QFileInfo>
#include "CDspInstrument.h"
#include <prlsdk/PrlTypes.h>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include "CDspTemplateStorage.h"
#include <prlcommon/Std/SmartPtr.h>

class CDspService;
class Task_MoveVm;
class CDspTaskManager;
class CDspVmDirManager;
class CVmConfiguration;
class CDspHaClusterHelper;
class CDspVmConfigManager;

namespace Vm
{
namespace Directory
{
struct Ephemeral;

} // namespace Directory
} // namespace Vm

namespace Command
{
namespace Move
{
///////////////////////////////////////////////////////////////////////////////
// struct Attribute

struct Attribute
{
	explicit Attribute(CAuthHelper& auth_): m_auth(&auth_)
	{
	}

	void operator()(const QFileInfo& source_, const QFileInfo& target_);

private:
	CAuthHelper* m_auth;
};

///////////////////////////////////////////////////////////////////////////////
// struct Remove

struct Remove: Instrument::Command::Base
{
	explicit Remove(const QString& path_): m_path(path_)
	{
	}

	result_type operator()();

private:
	const QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
// struct Copy

struct Copy: Instrument::Command::Base
{
	explicit Copy(Task_MoveVm& task_): m_task(&task_)
	{
	}

	void setAttribute(const Attribute& value_)
	{
		m_attribute = value_;
	}
	result_type operator()(const QFileInfo& source_, const QFileInfo& target_);

private:
	PRL_RESULT process(const QFileInfo& source_, const QFileInfo& target_);

	Task_MoveVm* m_task;
	QList<QFileInfo> m_queue;
	boost::optional<Attribute> m_attribute;
};

///////////////////////////////////////////////////////////////////////////////
// struct Rename

struct Rename: Instrument::Command::Base
{
	Rename(const QString& source_, const QString& target_):
		m_source(source_), m_target(target_)
	{
	}

	result_type operator()();

private:
	const QString m_source;
	const QString m_target;
};

///////////////////////////////////////////////////////////////////////////////
// struct Directory

struct Directory: Instrument::Command::Base
{
	explicit Directory(const CVmIdent& ident_);

	result_type operator()(const QFileInfo& value_);

private:
	CDspVmDirManager& getManager() const;

	CDspService* m_service;
	const CVmIdent m_ident;
};

///////////////////////////////////////////////////////////////////////////////
// struct Config

struct Config: Instrument::Command::Base
{
	Config(const QString& directory_, const SmartPtr<CVmConfiguration>& config_);

	result_type operator()(const QFileInfo& value_);

private:
	CDspVmConfigManager& getManager() const;

	CDspService* m_service;
	const QString m_directory;
	const SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cluster

struct Cluster: Instrument::Command::Base
{
	explicit Cluster(const SmartPtr<CVmConfiguration>& config_);

	result_type operator()(const QString& source_, const QString& target_);

private:
	CDspHaClusterHelper& getHelper() const;

	CDspService* m_service;
	const SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Libvirt

struct Libvirt: Instrument::Command::Base
{
	explicit Libvirt(Task_MoveVm& task_): m_task(&task_)
	{
	}

	result_type operator()();

private:
	Task_MoveVm* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Storage

struct Storage: Instrument::Command::Base
{
	typedef Template::Storage::Dao::value_type catalog_type;

	Storage(Task_MoveVm& task_, const QFileInfo& source_, catalog_type* catalog_):
		m_task(&task_), m_source(source_), m_catalog(catalog_)
	{
	}

	PRL_RESULT query();
	PRL_RESULT sweep();
	PRL_RESULT import();

private:
	PRL_RESULT do_(const QFileInfo& target_);

	Task_MoveVm* m_task;
	const QFileInfo m_source;
	QSharedPointer<catalog_type> m_catalog;
};

} // namespace Move
} // namespace Command

namespace Chain
{
namespace Move
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request: private QPair<QFileInfo, QFileInfo>
{
	Request(Task_MoveVm& context_, const QString& target_);

	CVmIdent getObject() const;
	QString getSourcePrivate() const
	{
		return getSourceConfig().absolutePath();
	}
	const QFileInfo& getSourceConfig() const
	{
		return first;
	}       
	QString getTargetPrivate() const
	{
		return getTargetConfig().absolutePath();
	}       
	const QFileInfo& getTargetConfig() const
	{
		return second;
	}       
	Task_MoveVm& getContext() const
	{
		return *m_context;
	}

private:
	typedef QPair<QFileInfo, QFileInfo> base_type;

	Task_MoveVm* m_context;
};

typedef Instrument::Chain::Unit<Request> base_type;

namespace Copy
{
///////////////////////////////////////////////////////////////////////////////
// struct Offline

struct Offline: base_type
{
	explicit Offline(bool attribute_): m_attribute(attribute_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	bool m_attribute;
};

namespace Online
{
namespace Special
{
///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk
{
	explicit Disk(Task_MoveVm& thread_): m_thread(&thread_)
	{
	}

	void add(const CVmHardDisk& value_)
	{
		m_source << value_;
	}
	PRL_RESULT execute(const QDir& target_);
	PRL_RESULT prepare(const QDir& target_, CDspTaskManager& dispatcher_);

private:
	Task_MoveVm* m_thread;
	QList<CVmHardDisk> m_source;
};

///////////////////////////////////////////////////////////////////////////////
// struct Facade

struct Facade
{
	explicit Facade(Task_MoveVm& thread_): m_disk(thread_)
	{
	}

	void account(const CVmHardDisk& value_)
	{
		m_disk.add(value_);
	}
	PRL_RESULT prepare(const QDir& target_);
	PRL_RESULT execute(const QDir& target_)
	{
		return m_disk.execute(target_);
	}

private:
	Disk m_disk;
};

} // namespace Special

///////////////////////////////////////////////////////////////////////////////
// struct Regular

struct Regular: Instrument::Command::Base
{
	typedef QList<QPair<QFileInfo, QString> > itemList_type;

	Regular(const itemList_type& folders_, const itemList_type& files_,
		Task_MoveVm& thread_):
		m_thread(&thread_), m_files(files_), m_folders(folders_)
	{
	}

	result_type operator()(const QDir& target_);

private:
	CAuthHelper* getAuth() const;

	Task_MoveVm* m_thread;
	itemList_type m_files;
	itemList_type m_folders;
};

///////////////////////////////////////////////////////////////////////////////
// struct Estimate

struct Estimate
{
	typedef Prl::Expected<Regular, PRL_RESULT> regular_type;
	typedef Prl::Expected<Special::Facade, PRL_RESULT> special_type;

	Estimate();

	PRL_RESULT operator()(const Request& request_);
	const regular_type& getRegular() const
	{
		return m_regular;
	}
	const special_type& getSpecial() const
	{
		return m_special;
	}

private:
	regular_type m_regular;
	special_type m_special;
};

///////////////////////////////////////////////////////////////////////////////
// struct Nexus

struct Nexus: base_type
{
	explicit Nexus(const redo_type& redo_): base_type(redo_)
	{
	}

	result_type operator()(const request_type& request_);
};

} // namespace Online
} // namespace Copy

///////////////////////////////////////////////////////////////////////////////
// struct Rename

struct Rename: base_type
{
	explicit Rename(const redo_type& redo_): base_type(redo_)
	{
	}

	result_type operator()(const request_type& request_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Import

struct Import: base_type
{
	Import(Vm::Directory::Ephemeral& ephemeral_,
		const redo_type& redo_): base_type(redo_),
		m_ephemeral(&ephemeral_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	Vm::Directory::Ephemeral* m_ephemeral;
};

} // namespace Move
} // namespace Chain

#endif // __TASK_MOVEVM_P_H__

