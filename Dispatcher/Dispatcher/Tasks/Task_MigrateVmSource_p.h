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
/// Copyright (c) 2010-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_MIGRATEVMSOURCE_P_H__
#define __TASK_MIGRATEVMSOURCE_P_H__

#include "CDspLibvirt.h"
#include "CDspInstrument.h"
#include <boost/function.hpp>
#include "Task_MigrateVmTunnel_p.h"
#include <prlcommon/HostUtils/PCSUtils.h>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/serialization/strong_typedef.hpp>
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
typedef Vm::Pump::Event<Virtuozzo::VmMigrateCheckPreconditionsReply> CheckReply;
typedef Vm::Pump::Event<Virtuozzo::VmMigrateReply> StartReply;
typedef Vm::Pump::Event<Virtuozzo::DispToDispResponseCmd> PeerFinished;

namespace Inspection
{
///////////////////////////////////////////////////////////////////////////////
// struct Report

struct Report
{
	void account(PRL_RESULT problem_);
	void account(PRL_RESULT problem_, const QString& text_);
	void account(PRL_RESULT problem_, const QString& text1_, const QString& text2_);
	void account(const QStringList& problems_)
	{
		m_result << problems_;
	}
	const QStringList& getResult() const
	{
		return m_result;
	}

private:
	QStringList m_result;
};

typedef QPair<Task_MigrateVmSource*, Report* > request_type;
typedef ::Instrument::Chain::Unit<request_type> base_type;

///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
struct Generic: base_type
{
	template<class U>
	explicit Generic(const U& redo_): base_type(boost::bind<result_type>(redo_, _1))
	{
	}

	result_type operator()(const request_type& request_)
	{
		if (T::upsets(request_.first))
			T::lament(request_.second);

		return base_type::operator()(request_);
	}
};

namespace Clone
{
///////////////////////////////////////////////////////////////////////////////
// struct Linked

struct Linked
{
	static bool upsets(request_type::first_type request_);
	static void lament(request_type::second_type report_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Online

struct Online
{
	static bool upsets(request_type::first_type request_);
	static void lament(request_type::second_type report_);
};

} // namespace Clone

///////////////////////////////////////////////////////////////////////////////
// struct ChangeSid

struct ChangeSid
{
	static bool upsets(request_type::first_type request_);
	static void lament(request_type::second_type report_);
};

namespace Hardware
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder
{
	explicit Builder(const CVmHardware& model_): m_model(&model_)
	{
	}

	void processCdrom();
	void processPci();
	void processMassStorage(const QString& name_);

	const QStringList& getResult() const
	{
		return m_report.getResult();
	}

private:
	Report m_report;
	const CVmHardware* m_model;
};

///////////////////////////////////////////////////////////////////////////////
// struct Chain

struct Chain: base_type
{
	result_type operator()(const request_type& request_);
};

} // namespace Hardware

namespace Reply
{
///////////////////////////////////////////////////////////////////////////////
// struct Efiw2k8

struct Efiw2k8: Instrument::Chain::Unit<request_type>
{
	explicit Efiw2k8(int version_, const redo_type& redo_):
		Instrument::Chain::Unit<request_type>(redo_), m_version(version_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	int m_version;
};

///////////////////////////////////////////////////////////////////////////////
// struct Interpreter

struct Interpreter
{
	explicit Interpreter(const QStringList& object_): m_object(object_)
	{
	}

	Efiw2k8::result_type operator()(const request_type& request_) const;

private:
	const QStringList m_object;
};

} // namespace Reply
} // namespace Inspection

namespace Shadow
{
typedef Vm::Libvirt::Running runningState_type;

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void reactSuccess();
	void reactFailure(const Flop::Event& reason_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Beholder

template<class T>
struct Beholder: Slot
{
	~Beholder();

	void reactFinish();
	Flop::Event operator()(T* task_);

private:
	QSharedPointer<T> m_task;
	QSharedPointer<QFutureWatcher<Flop::Event> > m_watcher;
};

///////////////////////////////////////////////////////////////////////////////
// struct State

template<class T, class U>
struct State: vsd::Trace<T>, Vm::Connector::Mixin<Shadow::Connector>
{
	typedef Beholder<U> beholder_type;
	typedef QSharedPointer<beholder_type> beholderPointer_type;

	template<typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		vsd::Trace<T>::on_exit(event_, fsm_);
		setBeholder(beholderPointer_type());
	}

protected:
	void setBeholder(beholderPointer_type const& value_);

private:
	beholderPointer_type m_beholder;
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
// struct State

struct State: Shadow::State<State, Task>
{
	typedef boost::mpl::vector1<Flag> flag_list;

	explicit State(const boost::function0<Task* >& factory_): m_factory(factory_)
	{
	}
	State()
	{
	}

	template<typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_);

private:
	boost::function0<Task* > m_factory;
};

} // namespace Content

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Progress

struct Progress: QObject
{
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Agent agent_type;
	typedef boost::function1<void, int> reporter_type;
	
	Progress(const agent_type& agent_, const reporter_type& reporter_):
		m_last(~0), m_agent(agent_), m_reporter(reporter_)
	{
	}

	void report(quint16 value_);

protected:
	void timerEvent(QTimerEvent* event_);

private:
	quint16 m_last;
	agent_type m_agent;
	reporter_type m_reporter;
};

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
// struct Vcmmd

struct Vcmmd: Decorator
{
	Vcmmd(const QString& uuid_, Unit* next_): Decorator(next_), m_uuid(uuid_)
	{
	}

	::Libvirt::Result do_();
	void rollback();
	void cleanup();

private:
	QString m_uuid;
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
	typedef ::Libvirt::Instrument::Agent::Vm::Unit agent_type;

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

namespace Online
{
BOOST_STRONG_TYPEDEF(quint64, downtime_type)

///////////////////////////////////////////////////////////////////////////////
// struct Separatist

struct Separatist
{
	explicit Separatist(const CVmConfiguration& source_): m_source(&source_)
	{
	}

	QString getNVRAM() const;
	QList<CVmHardDisk*> getDisks() const;
	QList<CVmSerialPort*> getSerialPorts() const;

private:
	template<class T>
	static QList<T*> refine(const QList<T*>& mix_);

	const CVmConfiguration* m_source;
};

///////////////////////////////////////////////////////////////////////////////
// struct Component

struct Component: Unit
{
	typedef Carrier* bus_type;
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Online::Agent
		agent_type;
	typedef boost::function<agent_type::result_type()> work_type;

	Component(const work_type& work_, bus_type bus_): m_bus(bus_), m_work(work_)
	{
	}

	::Libvirt::Result execute();

private:
	bus_type m_bus;
	work_type m_work;
};

///////////////////////////////////////////////////////////////////////////////
// struct Plan

struct Plan: private Separatist
{
	typedef QList<CVmHardDisk*> diskList_type;

	Plan(const Separatist& separatist_, Task_MigrateVmSource& context_);

	using Separatist::getNVRAM;
	using Separatist::getSerialPorts;
	const diskList_type& getDisks() const
	{
		return m_diskList;
	}
	const diskList_type& getDisksToSnapshot() const
	{
		return m_diskToSnapshotList;
	}
	const Component::agent_type::flavor_type& getFlavor() const
	{
		return m_flavor;
	}

private:
	diskList_type m_diskList;
	diskList_type m_diskToSnapshotList;
	Component::agent_type::flavor_type m_flavor;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hatchery

struct Hatchery
{
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Agent agent_type;

	Hatchery(Task_MigrateVmSource& task_, Carrier::target_type bus_):
		m_bus(new Carrier(bus_, bus_)), m_task(&task_), m_ports(boost::none)
	{
	}

	Hatchery& setPorts(const QPair<quint16,quint16>& ports_)
	{
		m_ports = ports_;
		return *this;
	}

	Unit* operator()(const agent_type& agent, const CVmConfiguration& target_);

private:
	Component::bus_type m_bus;
	Task_MigrateVmSource* m_task;
	boost::optional<QPair<quint16,quint16> > m_ports;
};

} // namespace Online

namespace Offline
{
///////////////////////////////////////////////////////////////////////////////
// struct Component

struct Component: Unit
{
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Offline agent_type;

	Component(const agent_type& agent_, const CVmConfiguration& target_):
		m_agent(agent_), m_target(target_)
	{
	}

	::Libvirt::Result execute()
	{
		return m_agent(m_target);
	}

private:
	agent_type m_agent;
	CVmConfiguration m_target;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hatchery

struct Hatchery
{
	typedef ::Libvirt::Instrument::Agent::Vm::Migration::Agent agent_type;

	Unit* operator()(const agent_type& agent_, const CVmConfiguration& target_) const;
};

} // namespace Offline
} // namespace Trick

///////////////////////////////////////////////////////////////////////////////
// struct Task

struct Task
{
	typedef Progress::agent_type agent_type;
	typedef boost::function2<Trick::Unit*, agent_type, const CVmConfiguration&>
		factory_type;

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
// struct State

struct State: Shadow::State<State, Task>
{
	typedef Shadow::State<State, Task> def_type;
	typedef Progress::reporter_type reporter_type;
	typedef QList<QSharedPointer<QTcpServer> > serverList_type;

	State(Task_MigrateVmSource& task_, const reporter_type& reporter_):
		m_reporter(reporter_), m_task(&task_)
	{
	}
	State(): m_task()
	{
	}

	template<typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_);

	template<typename FSM>
	void on_exit(const boost::mpl::true_& event_, FSM& fsm_);

	struct Action
	{
		template<class M>
		void operator()(serverList_type const& event_, M& fsm_, State& state_, State&)
		{
			Flop::Event x = state_.start(event_);
			if (x.isFailed())
				fsm_.process_event(x);
		}
		template<class M>
		void operator()(beholderPointer_type const& event_, M&, State& state_, State&)
		{
			state_.m_beholder = event_;
			state_.setBeholder(event_);
		}
	};

	struct internal_transition_table: boost::mpl::vector
		<
			msmf::Internal<serverList_type, Action>,
			msmf::Internal<beholderPointer_type, Action>
		>
	{
	};

private:
	Flop::Event start(serverList_type const& serverList_);

	reporter_type m_reporter;
	Task_MigrateVmSource* m_task;
	beholderPointer_type m_beholder;
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

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: vsd::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef QSharedPointer<QTcpSocket> client_type;
	typedef QHash<QString, Libvirt::State::serverList_type::value_type> listenerMap_type;
	typedef Pump::Frontend<Machine_type, Vm::Tunnel::libvirtChunk_type::s_command>
		libvirt_type;
	typedef Qemu::Hub
		<
			Vm::Tunnel::Hub::Traits
			<
				Machine_type,
				Virtuozzo::VmMigrateConnectQemuStateCmd,
				Virtuozzo::VmMigrateQemuStateTunnelChunk
			>
		> qemuState_type;
	typedef Qemu::Hub
		<
			Vm::Tunnel::Hub::Traits
			<
				Machine_type,
				Virtuozzo::VmMigrateConnectQemuDiskCmd,
				Virtuozzo::VmMigrateQemuDiskTunnelChunk
			>
		> qemuDisk_type;
	typedef Vm::Tunnel::Essence
		<
			Join::Machine<libvirt_type>,
			Join::State<qemuState_type, qemuState_type::Good>,
			Join::State<qemuDisk_type, qemuDisk_type::Good>,
			Vm::Pump::Launch_type
		> essence_type;
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

	void closeDiskListener(const qemuDisk_type::Down& event_);

	void closeStateListener(const qemuState_type::Down& event_);

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,       Flop::Event,      Flop::State>,
		a_irow<initial_state,          client_type,      &Frontend::accept>,
		msmf::Row<initial_state,       Vm::Pump::Launch_type,
			pumpingState_type::entry_pt<essence_type::Entry> >,
		a_irow<pumpingState_type,      client_type,      &Frontend::accept>,
		a_irow<pumpingState_type,      qemuDisk_type::Down, &Frontend::closeDiskListener>,
		a_irow<pumpingState_type,      qemuState_type::Down, &Frontend::closeStateListener>,
		msmf::Row<pumpingState_type
			::exit_pt<Success>,    msmf::none,       Success>,
		msmf::Row<pumpingState_type
			::exit_pt<Flop::State>,Flop::Event,      Flop::State>
	>
	{
	};

private:
	bool setup(const char* method_);

	void closeListener(const char* alias_);

	IO* m_service;
	listenerMap_type m_listenerMap;
	QList<QSharedPointer<QTcpSocket> > m_clients;
};

} // namespace Tunnel

namespace Move
{
typedef Pipeline::State<Machine_type, Vm::Pump::FinishCommand_type> peerWaitState_type;
typedef Join::Frontend
	<
		boost::mpl::vector
		<
			Join::Machine<Tunnel::Frontend>,
			Join::State<Libvirt::State, boost::mpl::true_>,
			Join::State<peerWaitState_type, peerWaitState_type::Good>
		>
	> join_type;

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: join_type
{
	typedef boost::mpl::at_c<initial_state, 1>::type libvirtState_type;

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		join_type::on_exit(event_, fsm_);
		m_beholder.clear();
	}

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		join_type::on_entry(event_, fsm_);
		m_beholder = libvirtState_type::beholderPointer_type
			(new libvirtState_type::beholder_type());
		fsm_.process_event(m_beholder);
/*
		libvirtState_type& x = static_cast<boost::msm::back::state_machine<Frontend> &>(*this)
			.template get_state<libvirtState_type& >();
		x.setBeholder(m_beholder);
*/
	}

private:
	libvirtState_type::beholderPointer_type m_beholder;
};

} // namespace Move

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void cancel();

	void react(const SmartPtr<IOPackage>& package_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: vsd::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef Pipeline::State<Machine_type, CheckReply> checkState_type;
	typedef Pipeline::State<Machine_type, StartReply> startState_type;
	typedef Pipeline::State<Machine_type, PeerFinished> peerQuitState_type;
	typedef Content::State copyState_type;
	typedef boost::msm::back::state_machine<Move::Frontend> moveState_type;

	typedef checkState_type initial_state;

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

	bool isTemplate(const boost::mpl::true_&);

	template<class T>
	void pokePeer(const T&);

	void setResult(const peerQuitState_type::Good&);
	void setResult(const Flop::Event& value_);

	void audit(const Libvirt::Trick::Online::downtime_type& downtime_);

	struct transition_table : boost::mpl::vector<
		// wire error exits to FINISHED immediately
		a_row<checkState_type,                     Flop::Event, Finished, &Frontend::setResult>,
		a_row<startState_type,                     Flop::Event, Finished, &Frontend::setResult>,
		a_row<copyState_type,                      Flop::Event, Finished, &Frontend::setResult>,
		a_row<moveState_type::exit_pt<Flop::State>,Flop::Event, Finished, &Frontend::setResult>,
		a_row<peerQuitState_type,                  Flop::Event, Finished, &Frontend::setResult>,

		// wire success exits sequentially up to FINISHED
		_row<checkState_type,                  checkState_type::Good,    startState_type>,
		_row<startState_type,                  startState_type::Good,    copyState_type>,
		_row<copyState_type,                   boost::mpl::true_,        moveState_type>,
		row<copyState_type,                    boost::mpl::true_,        peerQuitState_type,
			&Frontend::pokePeer, &Frontend::isTemplate>,
		a_irow<
			moveState_type,
			Libvirt::Trick::Online::downtime_type,
			&Frontend::audit
		>,
		a_row<moveState_type::exit_pt<Success>,msmf::none,               peerQuitState_type,&Frontend::pokePeer>,
		a_row<peerQuitState_type,              peerQuitState_type::Good, Finished,          &Frontend::setResult>
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

