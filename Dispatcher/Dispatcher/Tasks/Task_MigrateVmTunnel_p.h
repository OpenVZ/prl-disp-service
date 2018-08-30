///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmTunnel_p.h
//
/// The tunnel for a Vm migration.
/// UML state chart: https://src.openvz.org/projects/OVZ/repos/prl-disp-service
/// /Docs/Diagrams/Vm/Migration/diagram.vsd
///
/// @author nshirokovskiy@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_MIGRATEVMTUNNEL_P_H__
#define __TASK_MIGRATEVMTUNNEL_P_H__

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <boost/ref.hpp>
#include "Task_MigrateVm_p.h"
#include <boost/phoenix/core/reference.hpp>

namespace Migrate
{
namespace Vm
{
namespace Source
{
namespace Pump
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T, Parallels::IDispToDispCommands X>
struct Frontend: vsd::Frontend<Frontend<T, X> >
{
	typedef vsd::Frontend<Frontend<T, X> > def_type;
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
		m_device = event_.get<1>();
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

namespace Tunnel
{
namespace Qemu
{
///////////////////////////////////////////////////////////////////////////////
// struct Launch

template<Parallels::IDispToDispCommands X>
struct Launch
{
	typedef boost::tuples::element<0, Vm::Pump::Launch_type>::type service_type;

	Launch(service_type service_, QTcpSocket* socket_):
		m_service(service_), m_socket(socket_)
	{
	}

	Prl::Expected<Vm::Pump::Launch_type, Flop::Event> operator()() const;

private:
	service_type m_service;
	QTcpSocket* m_socket;
};

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T>
struct Traits: T
{
	typedef Launch<T::spawnEvent_type::s_command> spawnEvent_type;
	typedef boost::msm::back::state_machine
		<
			Pump::Frontend
			<
				typename T::machine_type,
				T::haulEvent_type::s_command
			>
		> pump_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

template<class T, class U = Traits<T> >
struct Hub: Vm::Tunnel::Hub::Unit<Hub<T, U>, U>
{
	typedef Vm::Tunnel::Hub::Unit<Hub, U> def_type;
	typedef typename U::spawnEvent_type spawnEvent_type;

	struct Action
	{
		template<class M>
		void operator()(spawnEvent_type const& event_, M& fsm_, Hub& state_, Hub&)
		{
			Prl::Expected<Vm::Pump::Launch_type, Flop::Event> x = event_();
			if (x.isFailed())
				return (void)fsm_.process_event(x.error());

			if (!state_.start(x.value(), x.value().get<2>()))
				return (void)fsm_.process_event(Flop::Event(PRL_ERR_INVALID_ARG));
		}
	};

	struct internal_transition_table: boost::mpl::push_front
		<
			typename def_type::internal_transition_table,
			msmf::Internal<spawnEvent_type, Action>
		>::type
	{
	};
};

} // namespace Qemu
} // namespace Tunnel
} // namespace Source

namespace Target
{
namespace Pump
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T, Parallels::IDispToDispCommands X>
struct Frontend: vsd::Frontend<Frontend<T, X> >
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

struct Connecting: vsd::Trace<Connecting>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Disconnecting

struct Disconnecting: vsd::Trace<Disconnecting>
{
};

namespace Connector
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic

template<class T, class M>
struct Basic: T, Vm::Connector::Base<M>
{
	void reactConnected()
	{
		boost::optional<QString> t;
		QIODevice* d = (QIODevice* )this->sender();
		if (!this->objectName().isEmpty())
		{
			t = this->objectName();
			d->setObjectName(t.get());
		}
		this->handle(Vm::Pump::Launch_type(this->getService(), d, t));
	}

	void reactDisconnected()
	{
		QIODevice* d = (QIODevice* )this->sender();
		if (NULL == d)
			return;

		this->handle(boost::phoenix::val(d));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Tcp

template<class M>
struct Tcp: Basic<Tcp_, M>
{
	void reactError(QAbstractSocket::SocketError value_)
	{
		WRITE_TRACE(DBG_DEBUG, "tunnel's socket emited SocketError with code %d", value_);
		switch (value_)
		{
		case QAbstractSocket::RemoteHostClosedError:
			return;
		default:
			this->handle(Flop::Event(PRL_ERR_FILE_READ_ERROR));
		}
	}
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
	static QSharedPointer<type> craft(Connector::Tcp<M>& connector_)
	{
		bool x;
		QSharedPointer<type> output = QSharedPointer<type>
			(new QTcpSocket(), &QObject::deleteLater);
		x = connector_.connect(output.data(), SIGNAL(connected()),
			SLOT(reactConnected()), Qt::QueuedConnection);
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect socket connect");
		x = connector_.connect(output.data(), SIGNAL(disconnected()),
			SLOT(reactDisconnected()), Qt::QueuedConnection);
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect socket disconnect");
		x = connector_.connect(output.data(), SIGNAL(error(QAbstractSocket::SocketError)),
			SLOT(reactError(QAbstractSocket::SocketError)));
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect socket errors");

		return output;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Haul

template<class T, class U, Parallels::IDispToDispCommands X, class V>
struct Haul: vsd::Frontend<U>, Vm::Connector::Mixin<typename Socket<T>::function_type::template apply<V>::type>
{
	typedef vsd::Frontend<U> def_type;
	typedef Pump::Frontend<V, X> pump_type;
	typedef boost::msm::back::state_machine<pump_type> pumpState_type;
	typedef Vm::Tunnel::Prime initial_state;

	using def_type::on_entry;

	template <typename FSM>
	void on_entry(ioEvent_type const& event_, FSM& fsm_)
	{
		def_type::on_entry(event_, fsm_);
		this->getConnector()->setService(&event_());
		m_socket = Socket<T>::craft(*this->getConnector());
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		def_type::on_exit(event_, fsm_);
		disconnect_();
		m_socket.clear();
		this->getConnector()->setService(NULL);
	}

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

template<class T, class U>
struct Shortcut
{
	typedef Haul
		<
			QTcpSocket,
			T,
			U::haulEvent_type::s_command,
			typename U::machine_type
		> type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Channel

template<class T>
struct Channel: Shortcut<Channel<T>, T>::type
{
	typedef typename T::spawnEvent_type launch_type;
	typedef typename T::haulEvent_type chunk_type;
	typedef int activate_deferred_events;
	typedef typename Shortcut<Channel, T>::type def_type;

	void connect(const launch_type& event_)
	{
		quint32 z;
		SmartPtr<char> b;
		IOPackage::EncodingType t;
		event_.getPackage()->getBuffer(0, t, b, z);
		Vm::Pump::Fragment::spice_type s =
			Vm::Pump::Fragment::Flavor<launch_type::s_command>()
				.getSpice(event_.getPackage());
		if (s)
			this->getConnector()->setObjectName(s.get());

		this->getSocket()->connectToHost(QHostAddress::LocalHost, *(quint16*)b.getImpl());
	}

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

template<class T>
struct Traits: T
{
	typedef boost::msm::back::state_machine<Channel<T> > pump_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

template<class T>
struct Hub: Vm::Tunnel::Hub::Unit<Hub<T>, Traits<T> >
{
	typedef Vm::Tunnel::Hub::Unit<Hub, Traits<T> > def_type;
	typedef typename T::spawnEvent_type spawnEvent_type;

	Hub(): m_service()
	{
	}

	using def_type::on_entry;

	template <typename FSM>
	void on_entry(ioEvent_type const& event_, FSM& fsm_)
	{
		def_type::on_entry(event_, fsm_);
		m_service = &event_();
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		def_type::on_exit(event_, fsm_);
		m_service = NULL;
	}

	struct Action
	{
		typedef typename def_type::pump_type pump_type;

		template<class M>
		void operator()(spawnEvent_type const& event_, M& fsm_, Hub& state_, Hub&)
		{
			Vm::Pump::Fragment::spice_type s =
				Vm::Pump::Fragment::Flavor<spawnEvent_type::s_command>()
					.getSpice(event_.getPackage());
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
					msmf::Internal<spawnEvent_type, Action>
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
} // namespace Tunnel
} // namespace Target
} // namespace Vm
} // namespace Migrate

#endif // __TASK_MIGRATEVMTUNNEL_P_H__

