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
#include <boost/ref.hpp>
#include "Task_MigrateVm_p.h"
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <Libraries/VmFileList/CVmFileListCopy.h>
#include <prlcommon/HostUtils/PCSUtils.h>
#include "CDspLibvirt.h"

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

namespace Shadow
{
typedef Vm::Libvirt::Running runningState_type;

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void finished();
};

///////////////////////////////////////////////////////////////////////////////
// struct Table

template<class T>
struct Table
{
	typedef typename boost::mpl::push_back
		<
			typename boost::mpl::push_back
			<
				T,
				msmf::Row<runningState_type, Flop::Event, Flop::State>
			>::type,
			msmf::Row<runningState_type, boost::mpl::true_, Success>
		>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T, class U>
struct Frontend: Vm::Frontend<U>, Vm::Connector::Mixin<Shadow::Connector>
{
	template<typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_);

protected:
	void launch(T* task_);

private:
	QSharedPointer<T> m_task;
	QSharedPointer<QFutureWatcher<Flop::Event> > m_watcher;
};

} // namespace Shadow

namespace Content
{
typedef QList<QPair<QFileInfo, QString> > itemList_type;

///////////////////////////////////////////////////////////////////////////////
// struct Copier

struct Copier
{
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
// struct Task

struct Task
{
	Task(Task_MigrateVmSource& task_, const itemList_type& folders_,
		const itemList_type &files_);

	Flop::Event run();
	void cancel();

private:
	const itemList_type* m_files;
	const itemList_type* m_folders;
	QScopedPointer<Copier> m_copier;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flag

struct Flag
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Shadow::Frontend<Task, Frontend>
{
	typedef boost::mpl::vector1<Flag> flag_list;
	typedef Shadow::runningState_type initial_state;

	explicit Frontend(const boost::function0<Task* >& factory_): m_factory(factory_)
	{
	}
	Frontend()
	{
	}

	template<typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Frontend>::on_entry(event_, fsm_);
		launch(m_factory());
	}

	struct transition_table: Shadow::Table<boost::mpl::vector<> >::type
	{
	};

private:
	boost::function0<Task* > m_factory;
};

} // namespace Content

namespace Pump
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T, Parallels::IDispToDispCommands X>
struct Frontend: Vm::Frontend<Frontend<T, X> >
{
	typedef Vm::Frontend<Frontend<T, X> > def_type;
	typedef Vm::Pump::Push::Pump<T, X> pushState_type;
	typedef Vm::Pump::Pull::Pump<T, X> pullState_type;

	Frontend(): m_device()
	{
	}

	template<typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		def_type::on_exit(event_, fsm_);
		if (NULL != m_device)
			m_device->close();

		m_device = NULL;
	}

	using def_type::on_entry;

	template<typename FSM>
	void on_entry(const Vm::Pump::Launch_type& event_, FSM& fsm_)
	{
		def_type::on_entry(event_, fsm_);
		m_device = event_.second;
	}

	struct transition_table : boost::mpl::vector<
		msmf::Row<pushState_type, Flop::Event,      Flop::State>,
		msmf::Row<pushState_type, Vm::Pump::Quit<X>,Success>
	>
	{
	};

	typedef boost::mpl::vector<pushState_type, pullState_type> initial_state;

private:
	QIODevice* m_device;
};

} // namespace Pump

namespace Libvirt
{
namespace Trick
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	virtual ~Unit()
	{
	}

	static const char suffix[];
	static QString disguise(const QString& word_)
	{
		return word_ + "." + suffix;
	}

	virtual ::Libvirt::Result execute() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Decorator

struct Decorator: Unit
{
	Decorator(Unit* trick_)
	{
		m_next.reset(trick_);
	}

	::Libvirt::Result execute();

private:
	virtual ::Libvirt::Result do_() = 0;
	virtual void rollback() = 0;
	virtual void cleanup() = 0;

	QScopedPointer<Unit> m_next;
};

///////////////////////////////////////////////////////////////////////////////
// struct Migration

struct Migration: Unit
{
	explicit Migration(const boost::function0< ::Libvirt::Result>& work_): m_work(work_)
	{
	}

	::Libvirt::Result execute()
	{
		return m_work();
	}

private:
	boost::function0< ::Libvirt::Result> m_work;
};

///////////////////////////////////////////////////////////////////////////////
// struct File

struct File: Decorator
{
	File(const QString& path_, Unit* next_): Decorator(next_), m_path(path_)
	{
	}

	::Libvirt::Result do_();
	void rollback();
	void cleanup();

private:
	QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disks

struct Disks: Decorator
{
	typedef ::Libvirt::Instrument::Agent::Vm::Snapshot::List agent_type;

	Disks(const QList<CVmHardDisk*>& disks_, const agent_type& agent_, Unit* next_):
		Decorator(next_), m_disks(disks_), m_agent(agent_)
	{
	}

	::Libvirt::Result do_();
	void rollback();
	void cleanup();

private:
	QList<CVmHardDisk*> m_disks;
	agent_type m_agent;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Task agent_type;

	explicit Factory(Task_MigrateVmSource& task_):
		m_task(&task_), m_ports(boost::none)
	{
	}

	Factory& setPorts(const QPair<quint16,quint16>& ports_)
	{
		m_ports = ports_;
		return *this;
	}

	Unit* operator()(const agent_type& agent, const CVmConfiguration& target_);

private:
	Task_MigrateVmSource* m_task;
	boost::optional<QPair<quint16,quint16> > m_ports;
};

} // namespace Trick

///////////////////////////////////////////////////////////////////////////////
// struct Task

struct Task
{
	typedef Progress::agent_type agent_type;
	typedef Trick::Factory factory_type;

	Task(const agent_type& agent_, const factory_type& factory_, const CVmConfiguration* config_):
		m_factory(factory_), m_agent(agent_), m_config(config_)
	{
	}

	Flop::Event run();
	void cancel();

private:
	factory_type m_factory;
	agent_type m_agent;
	const CVmConfiguration* m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Shadow::Frontend<Task, Frontend>
{
	typedef Vm::Tunnel::Ready initial_state;
	typedef Progress::reporter_type reporter_type;
	typedef QList<QSharedPointer<QTcpServer> > serverList_type;

	Frontend(Task_MigrateVmSource& task_, const reporter_type& reporter_):
		m_reporter(reporter_), m_task(&task_)
	{
	}
	Frontend(): m_task()
	{
	}

	template<typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_);
	template<typename FSM>
	void on_exit(const boost::mpl::true_& event_, FSM& fsm_);
	void start(const serverList_type& event_);

	typedef boost::mpl::vector
		<
			a_row
			<
				initial_state,
				serverList_type,
				Shadow::runningState_type,
				&Frontend::start
			>
		> grub_type;

	struct transition_table: Shadow::Table<grub_type>::type
	{
	};

private:
	reporter_type m_reporter;
	Task_MigrateVmSource* m_task;
	QSharedPointer<Progress> m_progress;
};

} // namespace Libvirt

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void acceptLibvirt();

	void acceptQemuDisk();

	void acceptQemuState();

private:
	QSharedPointer<QTcpSocket> accept_();
};

namespace Qemu
{
///////////////////////////////////////////////////////////////////////////////
// struct Launch

template<Parallels::IDispToDispCommands X>
struct Launch
{
	Launch(Vm::Pump::Launch_type::first_type service_, QTcpSocket* socket_):
		m_service(service_), m_socket(socket_)
	{
	}

	Prl::Expected<Vm::Pump::Launch_type, Flop::Event> operator()() const;

private:
	Vm::Pump::Launch_type::first_type m_service;
	QTcpSocket* m_socket;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<Parallels::IDispToDispCommands X, Parallels::IDispToDispCommands Y>
struct Frontend: Vm::Frontend<Frontend<X, Y> >
{
	typedef Launch<X> launch_type;
	typedef Pump::Frontend<Machine_type, Y> pump_type;
	typedef boost::msm::back::state_machine<pump_type> pumpState_type;
	typedef Vm::Tunnel::Prime initial_state;

	struct Action
	{
		template<class M, class S, class T>
		void operator()(launch_type const& event_, M& fsm_, S&, T&)
		{
			Prl::Expected<Vm::Pump::Launch_type, Flop::Event> x = event_();
			if (x.isFailed())
				fsm_.process_event(x.error());
			else
				fsm_.process_event(x.value());
		}
	};
	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,     launch_type,          Vm::Tunnel::Ready, Action>,
		msmf::Row<initial_state,     Flop::Event,          Flop::State>,
		msmf::Row<Vm::Tunnel::Ready, Vm::Pump::Launch_type,pumpState_type>,
		msmf::Row<typename pumpState_type::template
			exit_pt<Success>,    msmf::none,           Success>,
		msmf::Row<typename pumpState_type::template
			exit_pt<Flop::State>,Flop::Event,          Flop::State>
	>
	{
	};
};

} // namespace Qemu

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef QSharedPointer<QTcpSocket> client_type;
	typedef Libvirt::Frontend::serverList_type serverList_type;
	typedef Pump::Frontend<Machine_type, Vm::Tunnel::libvirtChunk_type::s_command>
		libvirt_type;
	typedef Qemu::Frontend<Parallels::VmMigrateConnectQemuStateCmd, Parallels::VmMigrateQemuStateTunnelChunk>
		qemuState_type;
	typedef Qemu::Frontend<Parallels::VmMigrateConnectQemuDiskCmd, Parallels::VmMigrateQemuDiskTunnelChunk>
		qemuDisk_type;
	typedef Vm::Tunnel::Essence<libvirt_type, qemuState_type, qemuDisk_type, Vm::Pump::Launch_type>
		essence_type;
	typedef boost::msm::back::state_machine<essence_type> pumpingState_type;
	typedef Vm::Tunnel::Prime initial_state;

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

	void accept(const client_type& connection_);

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,       Flop::Event,      Flop::State>,
		a_irow<initial_state,          client_type,      &Frontend::accept>,
		msmf::Row<initial_state,       Vm::Pump::Launch_type,
			pumpingState_type::entry_pt<essence_type::Entry> >,
		a_irow<pumpingState_type,      client_type,      &Frontend::accept>,
		msmf::Row<pumpingState_type
			::exit_pt<Success>,    msmf::none,       Success>,
		msmf::Row<pumpingState_type
			::exit_pt<Flop::State>,Flop::Event,      Flop::State>
	>
	{
	};

private:
	bool setup(const char* method_);

	IO* m_service;
	serverList_type m_servers;
	QList<QSharedPointer<QTcpSocket> > m_clients;
};

} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void cancel();

	void react(const SmartPtr<IOPackage>& package_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef Pipeline::Frontend<Machine_type, CheckReply> checking_type;
	typedef Pipeline::Frontend<Machine_type, StartReply> starting_type;
	typedef Pipeline::Frontend<Machine_type, Vm::Pump::FinishCommand_type> peerWait_type;
	typedef Pipeline::Frontend<Machine_type, PeerFinished> peerQuit_type;
	typedef Join::Frontend
		<
			boost::mpl::vector
			<
				Join::Machine<Tunnel::Frontend>,
				Join::Machine<Libvirt::Frontend>,
				Join::Machine<peerWait_type>
			>
		> moving_type;

	typedef boost::msm::back::state_machine<checking_type> checkStep_type;
	typedef boost::msm::back::state_machine<starting_type> startStep_type;
	typedef boost::msm::back::state_machine<Content::Frontend> copyStep_type;
	typedef boost::msm::back::state_machine<moving_type> moveStep_type;
	typedef boost::msm::back::state_machine<peerQuit_type> peerQuitState_type;

	typedef checkStep_type initial_state;

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

	void pokePeer(const msmf::none&);
	void setResult(const msmf::none&);
	void setResult(const Flop::Event& value_);

	struct transition_table : boost::mpl::vector<
		// wire error exits to FINISHED immediately
		a_row<checkStep_type::exit_pt<Flop::State>,	Flop::Event, Finished, &Frontend::setResult>,
		a_row<startStep_type::exit_pt<Flop::State>,	Flop::Event, Finished, &Frontend::setResult>,
		a_row<copyStep_type::exit_pt<Flop::State>,	Flop::Event, Finished, &Frontend::setResult>,
		a_row<moveStep_type::exit_pt<Flop::State>,	Flop::Event, Finished, &Frontend::setResult>,
		a_row<peerQuitState_type::exit_pt<Flop::State>, Flop::Event, Finished, &Frontend::setResult>,

		// wire success exits sequentially up to FINISHED
		_row<checkStep_type::exit_pt<Success>,	    msmf::none, startStep_type>,
		_row<startStep_type::exit_pt<Success>,	    msmf::none, copyStep_type>,
		_row<copyStep_type::exit_pt<Success>,	    msmf::none, moveStep_type>,
		a_row<moveStep_type::exit_pt<Success>,      msmf::none, peerQuitState_type,&Frontend::pokePeer>,
		a_row<peerQuitState_type::exit_pt<Success>, msmf::none, Finished,          &Frontend::setResult>
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

