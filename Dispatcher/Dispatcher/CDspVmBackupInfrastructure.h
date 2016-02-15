///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmBackupInfrastructure.h
///
/// Common backup helper class implementation
///
/// @author shrike
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

#ifndef __CDspVmBackupInfrastructure_H_
#define __CDspVmBackupInfrastructure_H_

#include "CDspVmBackupInfrastructure_p.h"

namespace Backup
{
namespace Activity
{
///////////////////////////////////////////////////////////////////////////////
// class Service

class Service: public QObject
{
	Q_OBJECT

public:
	template<class T>
	PRL_RESULT start(Task::Ct::Object ct_, Task::Subject<Task::Ct::Subject<T> > task_)
	{
		QScopedPointer<Snapshot::Ct::Subject<T> > s;
		PRL_RESULT e = task_.getSnapshot(ct_, s);
		if (PRL_FAILED(e))
			return e;

		QScopedPointer<Ct::Unit> a;
		e = task_.getActivity(ct_, a);
		if (PRL_FAILED(e))
			return e;

		e = s->create(task_);
		if (PRL_FAILED(e))
			return e;

		if (task_.getDspTask().operationIsCancelled())
		{
			s->destroy();
			return PRL_ERR_OPERATION_WAS_CANCELED;
		}
		e = a->start(s.take());
		if (PRL_FAILED(e))
			return e;

		QMutexLocker g(&m_mutex);
		m_ctActivities[ct_.getIdent()] = QSharedPointer<Ct::Unit>(a.take());
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT start(Task::Vm::Object vm_, Task::Subject<Task::Vm::Subject> task_);
	PRL_RESULT find(const CVmIdent& ident_, Activity::Object::Model& dst_) const;
	PRL_RESULT track(const CVmIdent& ident_, const actor_type& actor_);
	PRL_RESULT finish(const CVmIdent& ident_, const actor_type& actor_);
	PRL_RESULT abort(const CVmIdent& ident_, const actor_type& actor_);

public slots:
	void abort(IOSender::Handle actor_);

private:
	typedef Store::tracker_type tracker_type;
	typedef Ct::map_type ctMap_type;
	typedef Vm::map_type vmMap_type;

	template<class T>
	PRL_RESULT find(const CVmIdent& ident_, T& t_) const;
	template<class T>
	PRL_RESULT erase(const CVmIdent& ident_, T& t_);

	mutable QMutex m_mutex;
	ctMap_type m_ctActivities;
	vmMap_type m_vmActivities;
	tracker_type m_tracker;
};

} // namespace Activity

namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Director

struct Director
{
	Director(CDspVmDirHelper& dirHelper_, Activity::Service& service_,
		CDspDispConfigGuard& configGuard_):
		m_dirHelper(&dirHelper_), m_service(&service_), m_configGuard(&configGuard_)
	{
	}

	template<class T>
	PRL_RESULT operator()(T& builder_)
	{
		builder_.setObject(*m_dirHelper);
		builder_.setSubject(*m_configGuard);
		return builder_.startActivity(*m_service);
	}

private:
	CDspVmDirHelper* m_dirHelper;
	Activity::Service* m_service;
	CDspDispConfigGuard* m_configGuard;
};

namespace Create
{
///////////////////////////////////////////////////////////////////////////////
// struct Sketch

struct Sketch: CDspTaskHelper
{
	Sketch(const SmartPtr<CDspClient>& actor_,
			const SmartPtr<IOPackage>& request_);

	const CVmIdent& getIdent() const
	{
		return m_ident;
	}
	const QString& getBackupUuid() const
	{
		return m_backupUuid;
	}
	void setDirectory(const QString& value_)
	{
		m_ident.second = value_;
	}

protected:
	PRL_RESULT prepareTask();

private:
	CVmIdent m_ident;
	QString m_backupUuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct End

struct End
{
	explicit End(Activity::Service& service_): m_service(&service_)
	{
	}

	PRL_RESULT do_(Sketch& task_);
	void reportDone(Sketch& task_);
	void reportFailure(Sketch& task_);

private:
	Activity::Service* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavored

template<class T, bool F>
struct Flavored: Sketch
{
	Flavored(const SmartPtr<CDspClient>& actor_,
			const SmartPtr<IOPackage>& request_,
			T flavor_):
		Sketch(actor_, request_), m_flavor(flavor_)
	{
	}

	bool shouldBeCanceled_OnClientDisconnect()
	{
		return F;
	}

protected:
	PRL_RESULT run_body()
	{
		PRL_RESULT output = getLastErrorCode();
		if (PRL_FAILED(output))
			return output;

		setLastErrorCode(output = m_flavor.do_(*this));
		return output;
	}
	void finalizeTask()
	{
		if (PRL_SUCCEEDED(getLastErrorCode()))
			m_flavor.reportDone(*this);
		else
			m_flavor.reportFailure(*this);
	}

private:
	T m_flavor;
};

namespace Begin
{
///////////////////////////////////////////////////////////////////////////////
// struct Reporter

struct Reporter
{
	explicit Reporter(Activity::Service& service_): m_service(&service_)
	{
	}

	void reportDone(Sketch& task_);
	void reportFailure(Sketch& task_);

protected:
	Activity::Service& getService() const
	{
		return *m_service;
	}

private:
	Activity::Service* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavored

template<class T>
struct Flavored: Reporter
{
	Flavored(T flavor_, Activity::Service& service_):
		Reporter(service_), m_flavor(flavor_)
	{
	}

	PRL_RESULT do_(Sketch& task_)
	{
		PRL_RESULT output = m_flavor(task_.getIdent(), getService(), task_);
		if (PRL_FAILED(output))
			return output;

		output = getService().track(task_.getIdent(), task_.getClient());
		if (PRL_SUCCEEDED(output) && task_.operationIsCancelled())
			output = task_.getCancelResult();

		return output;
	}

private:
	T m_flavor;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct
{
	Ct(CDspVmDirHelper& dirHelper_, CDspDispConfigGuard& configGuard_,
		const SmartPtr<CDspVzHelper>& vz_);

	PRL_RESULT operator()
		(const CVmIdent& ident_, Activity::Service& service_, Sketch& task_);

private:
	CDspVmDirHelper* m_dirHelper;
	CDspDispConfigGuard* m_configGuard;
	SmartPtr<CDspVzHelper> m_vz;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	Vm(CDspVmDirHelper& dirHelper_, CDspDispConfigGuard& configGuard_);

	PRL_RESULT operator()
		(const CVmIdent& ident_, Activity::Service& service_, Sketch& task_);

private:
	CDspVmDirHelper* m_dirHelper;
	CDspDispConfigGuard* m_configGuard;
};

} // namespace Begin
} // namespace Create
} // namespace Task

namespace Activity
{
namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// Builder

template<class T>
struct Builder: Activity::Builder
{
	Builder(const Activity::Builder& z_, T flavor_, const SmartPtr<CDspVzHelper>& vz_):
		Activity::Builder(z_.getIdent(), z_.getTask()),
		m_flavor(flavor_), m_vz(vz_)
	{
	}

	void setObject(CDspVmDirHelper& dirHelper_)
	{
		m_object.reset(new Task::Ct::Object(Task::Object
			(getIdent(), getTask().getClient(), dirHelper_), m_vz));
	}
	PRL_RESULT startActivity(Activity::Service& service_)
	{
		Task::Workbench* w = getWorkbench();
		if (NULL == w || m_object.isNull())
			return PRL_ERR_UNINITIALIZED;

		Task::Ct::Reference r = Task::Ct::Reference
			(m_flavor.getMap(), CVzOperationHelper());
		return service_.start(*m_object, Task::Subject<Task::Ct::Subject<T> >
			(Task::Ct::Subject<T>(m_flavor, r), *w));
	}

private:
	T m_flavor;
	SmartPtr<CDspVzHelper> m_vz;
	QScopedPointer<Task::Ct::Object> m_object;
};

} // namespace Ct

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// Builder

struct Builder: Activity::Builder
{
	Builder(const CVmIdent& ident_, CDspTaskHelper& task_);

	void setObject(CDspVmDirHelper& dirHelper_);
	void setBackupUuid(const QString& value_);
	PRL_RESULT startActivity(Activity::Service& service_);
	const Task::Reference& getReference() const
	{
		return m_reference;
	}

private:
	Task::Vm::Reference m_reference;
	QScopedPointer<Task::Vm::Object> m_object;
};

} // namespace Vm
} // namespace Activity
} // namespace Backup

#endif // __CDspVmBackupInfrastructure_H_

