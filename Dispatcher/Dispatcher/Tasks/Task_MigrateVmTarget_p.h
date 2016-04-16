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
#include <boost/msm/front/state_machine_def.hpp>
#include <Libraries/VmFileList/CVmFileListCopy.h>

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
	typedef Pipeline::Frontend<Machine_type, StartCommand_type> pipeline_type;
	typedef boost::msm::back::state_machine<pipeline_type> Accepting;
	typedef Accepting initial_state;

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
				Accepting&, Preparing& target_);
		void operator()(const boost::mpl::true_&, Frontend& fsm_,
				Preparing&, Success&);
	};

	struct transition_table : boost::mpl::vector<
		msmf::Row<Accepting
			::exit_pt<Flop::State>,Flop::Event,      Flop::State>,
		msmf::Row<Accepting
			::exit_pt<Success>,    msmf::none,       Preparing,           Action>,
		a_row<Preparing,               CVmHardDisk,      Vm::Libvirt::Running,&Frontend::create>,
		msmf::Row<Preparing,           boost::mpl::true_,Success,             Action>,
		_row<Preparing,                Flop::Event,      Flop::State>,
		_row<Vm::Libvirt::Running,     boost::mpl::true_,Preparing>,
		_row<Vm::Libvirt::Running,     Flop::Event,	 Flop::State>
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

///////////////////////////////////////////////////////////////////////////////
// struct IO

struct IO: Vm::Pump::IO
{
	explicit IO(CDspDispConnection& io_);
	~IO();

	IOSendJob::Handle sendPackage(const SmartPtr<IOPackage>& package_);

signals:
	void disconnected();

private slots:
	void reactReceived(IOSender::Handle handle_, const SmartPtr<IOPackage>& package_);

	void reactDisconnected(IOSender::Handle handle);

	void reactSend(IOServerInterface*, IOSender::Handle handle_,
		    IOSendJob::Result, const SmartPtr<IOPackage>);

private:
	Q_OBJECT

	CDspDispConnection *m_io;
};
typedef boost::phoenix::expression::reference<IO>::type ioEvent_type;

namespace Connector
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic

struct Basic: QObject, Vm::Connector::Base<Machine_type>
{
	Basic(): m_service()
	{
	}

	void setService(IO* value_)
	{
		m_service = value_;
	}

public slots:
	void reactConnected();

	void reactDisconnected();

private:
	Q_OBJECT

	IO* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Tcp

struct Tcp: Basic
{
public slots:
	void reactError(QAbstractSocket::SocketError);

private:
	Q_OBJECT
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
	typedef Connector::Tcp connector_type;

	static bool isConnected(const type& socket_);

	static void disconnect(type& socket_);

	static QSharedPointer<type> craft(connector_type& connector_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Socket<QLocalSocket>

template<>
struct Socket<QLocalSocket>
{
	typedef QLocalSocket type;
	typedef Connector::Basic connector_type;

	static bool isConnected(const type& socket_);

	static void disconnect(type& socket_);

	static QSharedPointer<type> craft(connector_type& connector_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Haul

template<class T, class U, Parallels::IDispToDispCommands X>
struct Haul: Vm::Frontend<U>, Vm::Connector::Mixin<typename Socket<T>::connector_type>
{
	typedef Vm::Frontend<U> def_type;
	typedef Pump::Frontend<Machine_type, X> pump_type;
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

///////////////////////////////////////////////////////////////////////////////
// struct Qemu

template<Parallels::IDispToDispCommands X, Parallels::IDispToDispCommands Y>
struct Qemu: Haul<QTcpSocket, Qemu<X, Y>, Y>
{
	typedef Vm::Pump::Event<X> launch_type;
	typedef Vm::Pump::Event<Y> chunk_type;
	typedef int activate_deferred_events;
	typedef Haul<QTcpSocket, Qemu<X, Y>, Y> def_type;

	void connect(const launch_type& event_);

	struct transition_table : boost::mpl::vector<
		typename def_type::template
		a_row<typename def_type
			::initial_state,     launch_type,          Connecting,   &Qemu::connect>,
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
// struct Libvirt

struct Libvirt: Haul<QLocalSocket, Libvirt, Vm::Tunnel::libvirtChunk_type::s_command>
{
	typedef Haul<QLocalSocket, Libvirt, Vm::Tunnel::libvirtChunk_type::s_command> def_type;

	template <typename FSM>
	void on_entry(ioEvent_type const& event_, FSM& fsm_);

	using def_type::on_entry;

	struct transition_table : boost::mpl::vector<
		msmf::Row<initial_state,       Vm::Pump::Launch_type,pumpState_type>,
		msmf::Row<pumpState_type,      disconnected_type,    msmf::none,   Action, Guard>,
		msmf::Row<pumpState_type
			::exit_pt<Flop::State>,Flop::Event,          Flop::State>,
		msmf::Row<pumpState_type
			::exit_pt<Success>,    msmf::none,           Success>,
		msmf::Row<pumpState_type
			::exit_pt<Success>,    msmf::none,           Disconnecting, Action, Guard>,
		msmf::Row<Disconnecting,       disconnected_type,    Success>

	>
	{
	};
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>
{
	typedef Vm::Tunnel::Essence
		<
			Libvirt,
			Qemu<Parallels::VmMigrateConnectQemuStateCmd, Parallels::VmMigrateQemuStateTunnelChunk>,
			Qemu<Parallels::VmMigrateConnectQemuDiskCmd, Parallels::VmMigrateQemuDiskTunnelChunk>,
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

struct Connector: QObject, Vm::Connector::Base<Machine_type>
{
public slots:
	void cancel();

	void disconnected();

	void react(const SmartPtr<IOPackage>& package_);

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Vm::Frontend<Frontend>, Vm::Connector::Mixin<Connector>
{
        typedef Pipeline::Frontend<Machine_type, Vm::Pump::FinishCommand_type> synch_type;
	typedef boost::msm::back::state_machine<Start::Frontend> Starting;
	typedef boost::msm::back::state_machine<Content::Frontend> Copying;
	typedef boost::msm::back::state_machine<Tunnel::Frontend> Tunneling;
        typedef boost::msm::back::state_machine<synch_type> Synching;
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

	void synch(const msmf::none&);
	void setResult(const Flop::Event& value_);

	struct transition_table : boost::mpl::vector<
		// wire error exits to FINISHED immediately
		a_row<Starting::exit_pt<Flop::State>,  Flop::Event,Finished, &Frontend::setResult>,
		a_row<Copying::exit_pt<Flop::State>,   Flop::Event,Finished, &Frontend::setResult>,
		a_row<Tunneling::exit_pt<Flop::State>, Flop::Event,Finished, &Frontend::setResult>,
		a_row<Synching::exit_pt<Flop::State>,  Flop::Event,Finished, &Frontend::setResult>,
		// wire success exits sequentially up to FINISHED
		msmf::Row<Starting::exit_pt<Success>,  msmf::none,Copying>,
		msmf::Row<Copying::exit_pt<Success>,   msmf::none,Tunneling>,
		a_row<Tunneling::exit_pt<Success>,     msmf::none,Synching,  &Frontend::synch>,
		msmf::Row<Synching::exit_pt<Success>,  msmf::none,Finished>,
		// handle asyncronous termination
		a_row<Starting,                        Flop::Event, Finished, &Frontend::setResult>,
		a_row<Copying,                         Flop::Event, Finished, &Frontend::setResult>,
		a_row<Synching,                        Flop::Event, Finished, &Frontend::setResult>,
		a_row<Tunneling,                       Flop::Event, Finished, &Frontend::setResult>
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
