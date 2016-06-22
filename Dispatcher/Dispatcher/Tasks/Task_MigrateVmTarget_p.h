///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmTarget_p.h
//
/// Private part of a target side of Vm migration.
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

#ifndef __TASK_MIGRATEVMTARGET_P_H__
#define __TASK_MIGRATEVMTARGET_P_H__

#include <QTcpSocket>
#include <QHostAddress>
#include <QLocalSocket>
#include "Task_MigrateVm_p.h"
#include <boost/msm/back/state_machine.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <Libraries/VmFileList/CVmFileListCopy.h>
#include "CDspLibvirt.h"

class Task_MigrateVmTarget;

namespace Migrate
{
namespace Vm
{
namespace Target
{
struct Frontend;
typedef boost::msm::back::state_machine<Frontend> Machine_type;
typedef Pump::Event<Parallels::VmMigrateStartCmd> StartCommand_type;
typedef Pump::Event<Parallels::FileCopyRangeStart> CopyCommand_type;

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Pstorage

struct Pstorage
{
	explicit Pstorage(const QStringList& files_);
	~Pstorage();

	bool monkeyPatch(CVmConfiguration& config_);
	QList<CVmHardDisk> getDisks(const CVmConfiguration& config_) const;
	void cleanup(const CVmConfiguration& config_) const;

private:
	static const char suffix[];
	static QString disguise(const QString& word_)
	{
		return word_ + "." + suffix;
	}

	bool isNotMigratable(const CVmHardDisk& disk_) const;
	bool patch(CVmHardDisk& disk_);
	bool createBackedImage(const CVmHardDisk& disk_, const QString& path_) const;

	QStringList m_sharedDirs;
	QList<CVmHardDisk*> m_patchedDisks;
	QStringList m_savedSystemNames;
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void setUuid(const QString& uuid_)
	{
		m_uuid = uuid_;
	}

	void reactState(unsigned, unsigned, QString, QString);

private:
	QString m_uuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Tentative

struct Tentative: Trace<Tentative>, Vm::Connector::Mixin<Connector>
{
	explicit Tentative(const QString& uuid_): m_uuid(uuid_)
	{
	}
	Tentative(): m_uuid()
	{
	}

	struct Defined
	{
	};

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&);
	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

private:
	QString m_uuid;
};

} // namespace Libvirt

namespace Content
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>
{
	typedef struct Copying: Trace<Copying>
	{
	} initial_state;

	struct Good
	{
	};

	explicit Frontend(Task_MigrateVmTarget& task_): m_task(&task_)
	{
	}
	Frontend(): m_task()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&);

	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

	void copy(const CopyCommand_type& event_);

	// there in no use in copier cancel operation, it would not anyway
	// break blocking file io thus we have no action on cancel
	struct transition_table : boost::mpl::vector<
		a_row<Copying,CopyCommand_type,	Copying,&Frontend::copy>,
		_row<Copying,	Flop::Event,	Flop::State>,
		_row<Copying,	Good,		Success>
	>
	{
	};

private:
	Task_MigrateVmTarget *m_task;
	SmartPtr<CVmFileListCopySender> m_sender;
	SmartPtr<CVmFileListCopyTarget> m_copier;
};

} // namespace Content

namespace Commit
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void reactFinished();
};

///////////////////////////////////////////////////////////////////////////////
// struct Perform

struct Perform: Trace<Perform>, Vm::Connector::Mixin<Connector>
{
	Perform(CVmConfiguration& config_, const QStringList& checkFiles_,
			VIRTUAL_MACHINE_STATE state_):
		m_config(&config_), m_check(checkFiles_), m_state(state_)
	{
	}
	Perform(): m_config(), m_check(), m_state()
	{
	}

	struct Done
	{
	};

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&);

	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&);

private:
	typedef Migrate::Vm::Target::Libvirt::Pstorage helper_type;
	typedef ::Libvirt::Instrument::Agent::Vm::Block::Future future_type;
	typedef ::Libvirt::Instrument::Agent::Vm::Snapshot::Merge merge_type;

	CVmConfiguration* m_config;
	QStringList m_check;
	QSharedPointer<future_type> m_future;
	QSharedPointer<merge_type> m_merge;
	VIRTUAL_MACHINE_STATE m_state;
};

} // namespace Commit

namespace Start
{
///////////////////////////////////////////////////////////////////////////////
// struct Preparing

struct Preparing: Trace<Preparing>
{
	typedef QList<CVmHardDisk> volume_type;

	void setVolume(const volume_type& value_)
	{
		m_volume = value_;
	}

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		Trace<Preparing>::on_entry(event_, fsm_);
		if (m_volume.isEmpty())
			fsm_.process_event(boost::mpl::true_());
		else
			fsm_.process_event(m_volume.takeFirst());
	}

private:
	volume_type m_volume;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Libvirt::Frontend_<Frontend, Machine_type>
{
	typedef Pipeline::State<Machine_type, StartCommand_type> acceptingState_type;
	typedef acceptingState_type initial_state;

	Frontend(Task_MigrateVmTarget& task_): m_task(&task_)
	{
	}
	Frontend()
	{
	}

	void create(const CVmHardDisk& event_);

	struct Action
	{
		void operator()(const msmf::none&, Frontend& fsm_,
				acceptingState_type&, Preparing& target_);
		void operator()(const boost::mpl::true_&, Frontend& fsm_,
				Preparing&, Success&);
	};

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,   Flop::Event,         Flop::State>,
		msmf::Row<initial_state,   initial_state::Good, Preparing,           Action>,
		a_row<Preparing,           CVmHardDisk,         Vm::Libvirt::Running,&Frontend::create>,
		msmf::Row<Preparing,       boost::mpl::true_,   Success,             Action>,
		_row<Preparing,            Flop::Event,         Flop::State>,
		_row<Vm::Libvirt::Running, boost::mpl::true_,   Preparing>,
		_row<Vm::Libvirt::Running, Flop::Event,         Flop::State>
	>
	{
	};

private:
	Task_MigrateVmTarget *m_task;
};

} // namespace Start

namespace Pump
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T, Parallels::IDispToDispCommands X>
struct Frontend: Vm::Frontend<Frontend<T, X> >
{
	typedef Vm::Pump::Push::Pump<T, X> pushState_type;
	typedef Vm::Pump::Pull::Pump<T, X> pullState_type;

	struct transition_table : boost::mpl::vector<
		msmf::Row<pullState_type, Flop::Event,      Flop::State>,
		msmf::Row<pullState_type, Vm::Pump::Quit<X>,Success>
	>
	{
	};

	typedef boost::mpl::vector<pushState_type, pullState_type> initial_state;
};

} // namespace Pump

namespace Tunnel
{
typedef boost::phoenix::expression::reference<IO>::type ioEvent_type;
typedef boost::phoenix::expression::value<QIODevice* >::type disconnected_type;

///////////////////////////////////////////////////////////////////////////////
// struct Connecting

struct Connecting: Trace<Connecting>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Disconnecting

struct Disconnecting: Trace<Disconnecting>
{
};

namespace Connector
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic

template<class T, class M>
struct Basic: T, Vm::Connector::Base<M>
{
	void reactConnected();

	void reactDisconnected();
};

///////////////////////////////////////////////////////////////////////////////
// struct Tcp

template<class M>
struct Tcp: Basic<Tcp_, M>
{
	void reactError(QAbstractSocket::SocketError);
};

} // namespace Connector

///////////////////////////////////////////////////////////////////////////////
// struct Socket

template<class T>
struct Socket;

///////////////////////////////////////////////////////////////////////////////
// struct Socket<QTcpSocket>

template<>
struct Socket<QTcpSocket>
{
	typedef QTcpSocket type;
	typedef boost::mpl::quote1<Connector::Tcp> function_type;

	static bool isConnected(const type& socket_);

	static void disconnect(type& socket_);

	template<class M>
	static QSharedPointer<type> craft(Connector::Tcp<M>& connector_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Socket<QLocalSocket>

template<>
struct Socket<QLocalSocket>
{
	typedef QLocalSocket type;
	typedef Connector::Basic<Connector::Basic_, Machine_type> connector_type;
	typedef boost::mpl::always<connector_type> function_type;

	static bool isConnected(const type& socket_);

	static void disconnect(type& socket_);

	static QSharedPointer<type> craft(connector_type& connector_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Haul

template<class T, class U, Parallels::IDispToDispCommands X, class V>
struct Haul: Vm::Frontend<U>, Vm::Connector::Mixin<typename Socket<T>::function_type::template apply<V>::type>
{
	typedef Vm::Frontend<U> def_type;
	typedef Pump::Frontend<V, X> pump_type;
	typedef boost::msm::back::state_machine<pump_type> pumpState_type;
	typedef Vm::Tunnel::Prime initial_state;

	using def_type::on_entry;

	template <typename FSM>
	void on_entry(ioEvent_type const& event_, FSM& fsm_);

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_);

	void disconnect()
	{
		if (!m_socket.isNull())
			Socket<T>::disconnect(*m_socket);
	}

	bool isConnected() const
	{
		return !m_socket.isNull() && Socket<T>::isConnected(*m_socket);
	}

	struct Action
	{
		template<class M, class S, class D>
		void operator()(const disconnected_type&, M& fsm_, S&, D&)
		{
			fsm_.disconnect_();
		}
		template<class M, class S, class D>
		void operator()(const msmf::none&, M& fsm_, S&, D&)
		{
			fsm_.disconnect();
		}
	};

	struct Guard
	{
		template<class M, class S>
		bool operator()(const msmf::none&, M& fsm_, S&, Disconnecting&)
		{
			return fsm_.isConnected();
		}
		template<class M, class S, class D>
		bool operator()(const disconnected_type& event_, M& fsm_, S&, D&)
		{
			return event_() == fsm_.getSocket();
		}
	};

protected:
	T* getSocket() const
	{
		return m_socket.data();
	}

private:
	void disconnect_()
	{
		if (!m_socket.isNull())
			m_socket->disconnect();

		disconnect();
	}

	QSharedPointer<T> m_socket;
};

namespace Qemu
{
///////////////////////////////////////////////////////////////////////////////
// struct Shortcut

template<class T, class U, Parallels::IDispToDispCommands X>
struct Shortcut
{
	typedef boost::msm::back::state_machine<U> top_type;
	typedef Haul
		<
			QTcpSocket,
			T,
			X,
			Machine_type
//			top_type
		> type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Channel

template<class T, Parallels::IDispToDispCommands X, Parallels::IDispToDispCommands Y>
struct Channel: Shortcut<Channel<T, X, Y>, T, Y>::type
{
	typedef Vm::Pump::Event<X> launch_type;
	typedef Vm::Pump::Event<Y> chunk_type;
	typedef int activate_deferred_events;
	typedef typename Shortcut<Channel<T, X, Y>, T, Y>::type def_type;

	void connect(const launch_type& event_);

	struct transition_table : boost::mpl::vector<
		typename def_type::template
		a_row<typename def_type
			::initial_state,     launch_type,          Connecting,   &Channel::connect>,
		msmf::Row<Connecting,        chunk_type,           msmf::none,   msmf::Defer>,
		msmf::Row<Connecting,        Flop::Event,          Flop::State>,
		msmf::Row<Connecting,        Vm::Pump::Launch_type,typename def_type::pumpState_type>,
		msmf::Row<typename def_type
			::pumpState_type,    disconnected_type,    msmf::none,   typename def_type::Action,
			typename def_type::Guard>,
		msmf::Row<typename def_type::pumpState_type::template
			exit_pt<Flop::State>,Flop::Event,          Flop::State>,
		msmf::Row<typename def_type::pumpState_type::template
			exit_pt<Success>,    msmf::none,           Success>,
		msmf::Row<typename def_type::pumpState_type::template
			exit_pt<Success>,    msmf::none,           Disconnecting,
			typename def_type::Action, typename def_type::Guard>,
		msmf::Row<Disconnecting,     disconnected_type,    Success>
	>
	{
	};
};

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T, Parallels::IDispToDispCommands X, Parallels::IDispToDispCommands Y>
struct Traits
{
	typedef Machine_type machine_type;
	typedef boost::msm::back::state_machine<Channel<T, X, Y> > pump_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

template<Parallels::IDispToDispCommands X, Parallels::IDispToDispCommands Y>
struct Hub: Vm::Tunnel::Hub<Hub<X, Y>, Traits<Hub<X, Y>, X, Y>, Y>
{
	typedef Vm::Tunnel::Hub<Hub, Traits<Hub, X, Y>, Y> def_type;

	Hub(): m_service()
	{
	}

	using def_type::on_entry;

	template <typename FSM>
	void on_entry(ioEvent_type const& event_, FSM& fsm_);

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_);

	struct Action
	{
		typedef typename def_type::pump_type pump_type;

		template<class M>
		void operator()(Vm::Pump::Event<X> const& event_, M& fsm_, Hub& state_, Hub&)
		{
			boost::optional<QString> s =
				Vm::Pump::Push::Packer::getSpice(*event_.getPackage());
			if (!state_.start(boost::phoenix::ref(*state_.m_service), s))
				return (void)fsm_.process_event(Flop::Event(PRL_ERR_INVALID_ARG));

			state_.match(s)->process_event(event_);
		}
		template<class M>
		void operator()(Vm::Pump::Launch_type const& event_, M&, Hub& state_, Hub&)
		{
			pump_type* p = state_.match(event_.get<2>());
			if (NULL != p)
				p->process_event(event_);
		}
		template<class M>
		void operator()(disconnected_type const& event_, M&, Hub& state_, Hub&)
		{
			pump_type* p = state_.match(event_()->objectName());
			if (NULL != p)
				p->process_event(event_);
		}
	};

	struct internal_transition_table: boost::mpl::push_front
		<
			typename boost::mpl::push_front
			<
				typename boost::mpl::push_front
				<
					typename def_type::internal_transition_table,
					msmf::Internal<Vm::Pump::Event<X>, Action>
				>::type,
				msmf::Internal<Vm::Pump::Launch_type, Action>
			>::type,
			msmf::Internal<disconnected_type, Action>
		>::type
	{
	};

private:
	IO* m_service;
};

} // namespace Qemu

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Shortcut

template<class T>
struct Shortcut
{
	typedef Haul
		<
			QLocalSocket,
			T,
			Vm::Tunnel::libvirtChunk_type::s_command,
			Machine_type
		> type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Channel

struct Channel: Shortcut<Channel>::type
{
	typedef Shortcut<Channel>::type def_type;
	typedef int activate_deferred_events;

	template <typename FSM>
	void on_entry(ioEvent_type const& event_, FSM& fsm_);

	using def_type::on_entry;

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,       Vm::Pump::Launch_type,	pumpState_type>,
		msmf::Row<initial_state,       Vm::Tunnel::libvirtChunk_type,	msmf::none,	msmf::Defer>,
		msmf::Row<pumpState_type,      disconnected_type,    msmf::none,   Action, Guard>,
		msmf::Row<pumpState_type
			::exit_pt<Flop::State>,Flop::Event,          Flop::State>,
		msmf::Row<pumpState_type
			::exit_pt<Success>,    msmf::none,           Success>,
		msmf::Row<pumpState_type
			::exit_pt<Success>,    msmf::none,           Disconnecting, Action,Guard>,
		msmf::Row<Disconnecting,       disconnected_type,    Success>

	>
	{
	};
};

} // namespace Libvirt

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>
{
	typedef Qemu::Hub<Parallels::VmMigrateConnectQemuStateCmd, Parallels::VmMigrateQemuStateTunnelChunk>
		qemuState_type;
	typedef Qemu::Hub<Parallels::VmMigrateConnectQemuDiskCmd, Parallels::VmMigrateQemuDiskTunnelChunk>
		qemuDisk_type;
	typedef Vm::Tunnel::Essence
		<
			Join::Machine<Libvirt::Channel>,
			Join::State<qemuState_type, qemuState_type::Good>,
			Join::State<qemuDisk_type, qemuDisk_type::Good>,
			ioEvent_type
		> essence_type;
	typedef boost::msm::back::state_machine<essence_type> essenceState_type;
	typedef Vm::Tunnel::Prime initial_state;

	explicit Frontend(IO& service_) : m_service(&service_)
	{
	}
	Frontend(): m_service()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Frontend>::on_entry(event_, fsm_);
		fsm_.process_event(boost::phoenix::ref(*m_service));
	}

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,       ioEvent_type,
			essenceState_type::entry_pt<essence_type::Entry> >,
		msmf::Row<essenceState_type
			::exit_pt<Success>,    msmf::none,  Success>,
		msmf::Row<essenceState_type
			::exit_pt<Flop::State>,Flop::Event, Flop::State>
	>
	{
	};

private:
        IO* m_service;
};

} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: Connector_, Vm::Connector::Base<Machine_type>
{
	void cancel();

	void disconnected();

	void react(const SmartPtr<IOPackage>& package_);
};

namespace Move
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef boost::msm::back::state_machine<Tunnel::Frontend> Tunneling;
	typedef Pipeline::State<Machine_type, Vm::Pump::FinishCommand_type> synchState_type;
	typedef Tunneling initial_state;

	explicit Frontend(Tunnel::IO& io_): m_io(&io_)
	{
	}
	Frontend(): m_io()
	{
	}

	void synch(const msmf::none&);

	struct transition_table : boost::mpl::vector<
		_row<Tunneling::exit_pt<Flop::State>, Flop::Event,           Flop::State>,
		_row<synchState_type,                 Flop::Event,           Flop::State>,
		a_row<Tunneling::exit_pt<Success>,    msmf::none,            synchState_type, &Frontend::synch>,
		msmf::Row<synchState_type,            synchState_type::Good, Success>
	>
	{
	};

private:
	Tunnel::IO* m_io;
};

} // namespace Move

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
	typedef boost::msm::back::state_machine<Start::Frontend> Starting;
	typedef boost::msm::back::state_machine<Content::Frontend> Copying;
	typedef Join::Frontend
		<
			boost::mpl::vector
			<
				Join::State<Libvirt::Tentative, Libvirt::Tentative::Defined>,
				Join::Machine<Move::Frontend>
			>
		> moving_type;
	typedef boost::msm::back::state_machine<moving_type> Moving;
	typedef Commit::Perform Commiting;
	typedef Starting initial_state;

	Frontend(Task_MigrateVmTarget& task_, Tunnel::IO& io_):
		m_io(&io_), m_task(&task_)
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
		a_row<Starting::exit_pt<Flop::State>,  Flop::Event,Finished, &Frontend::setResult>,
		a_row<Copying::exit_pt<Flop::State>,   Flop::Event,Finished, &Frontend::setResult>,
		a_row<Moving::exit_pt<Flop::State>,    Flop::Event,Finished, &Frontend::setResult>,
		// wire success exits sequentially up to FINISHED
		msmf::Row<Starting::exit_pt<Success>,  msmf::none,Copying>,
		msmf::Row<Copying::exit_pt<Success>,   msmf::none,Moving>,
		msmf::Row<Moving::exit_pt<Success>,    msmf::none,Commiting>,
		msmf::Row<Commiting,                   Commiting::Done,Finished>,
		// handle asyncronous termination
		a_row<Starting,                        Flop::Event, Finished, &Frontend::setResult>,
		a_row<Copying,                         Flop::Event, Finished, &Frontend::setResult>,
		a_row<Commiting,                       Flop::Event, Finished, &Frontend::setResult>,
		a_row<Moving,                          Flop::Event, Finished, &Frontend::setResult>
	>
	{
	};

private:
	Tunnel::IO* m_io;
	Task_MigrateVmTarget *m_task;
};

} // namespace Target
} // namespace Vm
} // namespace Migrate

#endif // __TASK_MIGRATEVMTARGET_P_H__
