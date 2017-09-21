///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_DeleteVm_p.h
///
/// Delete Vm private stuff declarations
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

#ifndef __TASK_DELETEVM_P_H__
#define __TASK_DELETEVM_P_H__

#include "CDspInstrument.h"
#include <prlcommon/Std/SmartPtr.h>

class Task_DeleteVm;
class CVmConfiguration;

namespace Instrument
{
namespace Command
{
namespace Delete
{
///////////////////////////////////////////////////////////////////////////////
// struct Libvirt

struct Libvirt
{
	explicit Libvirt(const QString& uid_): m_uid(uid_)
	{
	}

	PRL_RESULT operator()();

private:
	const QString m_uid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Guise

struct Guise
{
	explicit Guise(CAuthHelper& auth_): m_auth(&auth_)
	{
	}

	PRL_RESULT enable();
	PRL_RESULT disable();

private:
	CAuthHelper* m_auth;
};

///////////////////////////////////////////////////////////////////////////////
// struct Backup

struct Backup
{
	Backup(const QString& home_, const SmartPtr<CVmConfiguration>& vm_):
		m_home(home_), m_vm(vm_)
	{
	}

	PRL_RESULT disable();
	PRL_RESULT teardown();

private:
	const QString m_home;
	SmartPtr<CVmConfiguration> m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Content

struct Content
{
	typedef PRL_RESULT result_type;

	Content(const QString& home_, Task_DeleteVm& task_):
		m_home(home_), m_task(&task_)
	{
	}

	PRL_RESULT operator()();
	PRL_RESULT operator()(const QStringList& list_);

private:
	const QString m_home;
	Task_DeleteVm* m_task;
};

} // namespace Delete
} // namespace Command

namespace Chain
{
namespace Delete
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request
{
	typedef SmartPtr<CVmConfiguration> vm_type;

	Request(const vm_type& vm_, const QStringList& itemList_);

	const vm_type& getVm() const
	{
		return m_vm;
	}
	const QString& getHome() const
	{
		return m_home;
	}
	bool isTemplate() const;
	const QStringList& getItems() const
	{
		return m_itemList;
	}

private:
	QString m_home;
	const vm_type m_vm;
	const QStringList m_itemList;
};
typedef Unit<Request> base_type;

namespace Template
{
///////////////////////////////////////////////////////////////////////////////
// struct Shared

struct Shared: base_type
{
	Shared(CAuthHelper& auth_, const redo_type& redo_):
		base_type(redo_), m_auth(&auth_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	CAuthHelper* m_auth;
};

///////////////////////////////////////////////////////////////////////////////
// struct Regular

struct Regular: base_type
{
	Regular(Task_DeleteVm& task_, const redo_type& redo_):
		base_type(redo_), m_task(&task_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	Task_DeleteVm* m_task;
};

} // namespace Template

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Home

struct Home: base_type
{
	explicit Home(Task_DeleteVm& task_): m_task(&task_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	Task_DeleteVm* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List: base_type
{
	List(Task_DeleteVm& task_, const redo_type& redo_):
		base_type(redo_), m_task(&task_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	Task_DeleteVm* m_task;
};

} // namespace Vm
} // namespace Delete

namespace Unregister
{
///////////////////////////////////////////////////////////////////////////////
// struct Template

struct Template: Delete::base_type
{
	explicit Template(const redo_type& redo_): Delete::base_type(redo_)
	{
	}

	result_type operator()(const request_type& request_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: Delete::base_type
{
	explicit Vm(Task_DeleteVm& task_): m_task(&task_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	Task_DeleteVm* m_task;
};

} // namespace Unregister
} // namespace Chain
} // namespace Instrument

#endif // __TASK_DELETEVM_P_H__

