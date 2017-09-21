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
// struct Import

struct Import: Instrument::Command::Base
{
	typedef Template::Storage::Dao::value_type catalog_type;

	Import(Task_MoveVm& task_, const QFileInfo& source_, catalog_type* catalog_):
		m_task(&task_), m_source(source_), m_catalog(catalog_)
	{
	}

	PRL_RESULT execute();
	PRL_RESULT rollback();

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
typedef Instrument::Chain::Unit<QPair<QFileInfo, QFileInfo> > base_type;

///////////////////////////////////////////////////////////////////////////////
// struct Copy

struct Copy: base_type
{
	Copy(Task_MoveVm& task_, bool attribute_):
		m_attribute(attribute_), m_task(&task_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	bool m_attribute;
	Task_MoveVm* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Rename

struct Rename: base_type
{
	Rename(Task_MoveVm& task_, const redo_type& redo_):
		base_type(redo_), m_task(&task_)
	{
	}

	result_type operator()(const request_type& request_);

private:
        Task_MoveVm* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Import

struct Import: base_type
{
	Import(Task_MoveVm& task_, Vm::Directory::Ephemeral& ephemeral_,
		const redo_type& redo_): base_type(redo_), m_task(&task_),
		m_ephemeral(&ephemeral_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	Task_MoveVm* m_task;
	Vm::Directory::Ephemeral* m_ephemeral;
};

} // namespace Move
} // namespace Chain

#endif // __TASK_MOVEVM_P_H__

