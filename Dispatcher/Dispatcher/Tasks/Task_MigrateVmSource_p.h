///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmSource_p.h
//
/// Private part of a source side of Vm migration.
/// UML state chart: https://src.openvz.org/projects/OVZ/repos/prl-disp-service
/// /Docs/Diagrams/Vm/Migration/diagram.vsd
///
/// @author nshirokovskiy@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_MIGRATEVMSOURCE_P_H__
#define __TASK_MIGRATEVMSOURCE_P_H__

#include <QTcpServer>
#include <QTcpSocket>
#include "Task_MigrateVm_p.h"
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <Libraries/VmFileList/CVmFileListCopy.h>

class Task_MigrateVmSource;

namespace Migrate
{
namespace Vm
{
namespace Source
{
struct Frontend;
typedef boost::msm::back::state_machine<Frontend> Machine_type;
typedef Pump::Event<Parallels::VmMigrateCheckPreconditionsReply> CheckReply;
typedef Pump::Event<Parallels::VmMigrateReply> StartReply;
typedef Pump::Event<Parallels::DispToDispResponseCmd> PeerFinished;

namespace Content
{
///////////////////////////////////////////////////////////////////////////////
// struct Copier

struct Copier
{
	typedef QList<QPair<QFileInfo, QString> > itemList_type;
	typedef SmartPtr<CVmFileListCopySender> sender_type;
	typedef SmartPtr<CVmFileListCopySource> copier_type;

	Copier(const sender_type& sender_, const copier_type& copier_,
		const SmartPtr<CVmEvent>& event_): m_event(event_),
		m_sender(sender_), m_copier(copier_)
	{
	}

	void cancel();
	Flop::Event execute(const itemList_type& folderList_, const itemList_type& fileList_);

private:
	SmartPtr<CVmEvent> m_event;
	SmartPtr<CVmFileListCopySender> m_sender;
	SmartPtr<CVmFileListCopySource> m_copier;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flag

struct Flag
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject, Vm::Connector::Base<Machine_type>
{
public slots:
	void finished();

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef Copying initial_state;
	typedef boost::mpl::vector1<Flag> flag_list;

	struct Good
	{
	};

	Frontend(Task_MigrateVmSource& task_, const Copier::itemList_type& folderList_,
		const Copier::itemList_type& fileList_): m_task(&task_), m_fileList(&fileList_),
		m_folderList(&folderList_)
	{
	}
	Frontend()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&);

	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

	struct transition_table : boost::mpl::vector<
		_row<Copying,	Flop::Event,	Flop::State>,
		_row<Copying,	Good,		Success>
	>
	{
	};

private:
	Task_MigrateVmSource *m_task;
	QSharedPointer<Copier> m_copier;
	const Copier::itemList_type* m_fileList;
	const Copier::itemList_type* m_folderList;
	QSharedPointer<QFutureWatcher<Flop::Event> > m_watcher;
};

} // namespace Content

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Running

struct Running: Trace<Running>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject, Vm::Connector::Base<Machine_type>
{
public slots:
	void finished(int code, QProcess::ExitStatus status);

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef Running initial_state;

	struct Good
	{
	};

	explicit Frontend(const QString& vmname) : m_vmname(vmname)
	{
	}

	Frontend()
	{
	}

	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

	void start(const QSharedPointer<QTcpServer>& event_);

	struct transition_table : boost::mpl::vector<
		a_row<initial_state,	QSharedPointer<QTcpServer>,initial_state,&Frontend::start>,
		_row<initial_state,	Flop::Event,		Flop::State>,
		_row<initial_state,	Good,			Success>
	>
	{
	};

private:
	QString m_vmname;
	QSharedPointer<QProcess> m_process;
};

} // namespace Libvirt

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct Accepted

struct Accepted
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject, Vm::Connector::Base<Machine_type>
{
public slots:
	void newConnection();

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct IO

struct IO: Pump::IO
{
public:
	explicit IO(IOClient &io);
	~IO();

	IOSendJob::Handle sendPackage(const SmartPtr<IOPackage>& package_);

private slots:
	void reactReceived(const SmartPtr<IOPackage>& package);

	void reactSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage>);

private:
	Q_OBJECT

	IOClient *m_io;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef Pump::Frontend<Machine_type, boost::mpl::true_> pump_type;
	typedef boost::msm::back::state_machine<pump_type> Pumping;
	typedef struct Idle: Trace<Idle>
	{
	} initial_state; 

	explicit Frontend(IO& service_) : m_service(&service_)
	{
	}
	Frontend(): m_service()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&);

	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

	void accept(const Accepted&);

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,			Flop::Event,		Flop::State>,
		msmf::Row<initial_state,			msmf::none,		Vm::Tunnel::Prime>,
		a_row<Vm::Tunnel::Prime,			Accepted,		Vm::Tunnel::Ready,	&Frontend::accept>,
		msmf::Row<Vm::Tunnel::Ready,			Pump::Launch_type,	Pumping>,
		msmf::Row<Pumping::exit_pt<Success>,		msmf::none,		Success>,
		msmf::Row<Pumping::exit_pt<Flop::State>,	Flop::Event,		Flop::State>
	>
	{
	};

private:
	IO* m_service;
	QSharedPointer<QTcpServer> m_server;
	QSharedPointer<QTcpSocket> m_client;
};

} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject, Vm::Connector::Base<Machine_type>
{
public slots:
	void cancel();

	void react(const SmartPtr<IOPackage>& package_);

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Workflow

struct Workflow: Vm::Frontend<Workflow>
{
	typedef Pipeline::Frontend<Machine_type, CheckReply> checking_type;
	typedef Pipeline::Frontend<Machine_type, StartReply> starting_type;
	typedef Join<Tunnel::Frontend, Libvirt::Frontend> moving_type;

	typedef boost::msm::back::state_machine<checking_type> checkStep_type;
	typedef boost::msm::back::state_machine<starting_type> startStep_type;
	typedef boost::msm::back::state_machine<Content::Frontend> copyStep_type;
	typedef boost::msm::back::state_machine<moving_type> moveStep_type;

	typedef checkStep_type initial_state;

	struct transition_table : boost::mpl::vector<
		// wire error exits to FINISHED immediately
		_row<checkStep_type::exit_pt<Flop::State>,	Flop::Event, Flop::State>,
		_row<startStep_type::exit_pt<Flop::State>,	Flop::Event, Flop::State>,
		_row<copyStep_type::exit_pt<Flop::State>,	Flop::Event, Flop::State>,
		_row<moveStep_type::exit_pt<Flop::State>,	Flop::Event, Flop::State>,

		// wire success exits sequentially up to FINISHED
		_row<checkStep_type::exit_pt<Success>,	msmf::none,	startStep_type>,
		_row<startStep_type::exit_pt<Success>,	msmf::none,	copyStep_type>,
		_row<copyStep_type::exit_pt<Success>,	msmf::none,	moveStep_type>,
		_row<moveStep_type::exit_pt<Success>,	msmf::none,	Success>
	>
	{
	};
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef Join<Workflow, Pipeline::Frontend<Machine_type, PeerFinished> >
		join_type;
	typedef boost::msm::back::state_machine<Workflow> workflow_type;
	typedef boost::msm::back::state_machine<Pipeline::Frontend
		<Machine_type, PeerFinished> > finishing_type;
	typedef boost::msm::back::state_machine<join_type> initial_state;

	Frontend(Task_MigrateVmSource &task, Tunnel::IO& io_): m_io(&io_), m_task(&task)
	{
	}
	Frontend(): m_io(), m_task()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&);

	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

	void setResult(const Flop::Event& value_);

	struct transition_table : boost::mpl::vector<
		// wire error exits to FINISHED immediately
		a_row<initial_state::exit_pt<Flop::State>, Flop::Event, Finished, &Frontend::setResult>,
		// wire success exits sequentially up to FINISHED
		_row<initial_state::exit_pt<Success>,	msmf::none,	Finished>
	>
	{
	};

private:
	Tunnel::IO* m_io;
	Task_MigrateVmSource *m_task;
};

} // namespace Source
} // namespace Vm
} // namespace Migrate

#endif // __TASK_MIGRATEVMSOURCE_P_H__

