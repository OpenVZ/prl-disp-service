///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVm_p.h
//
/// Common part for migration implementation
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

#ifndef __TASK_MIGRATEVM_P_H__
#define __TASK_MIGRATEVM_P_H__

#include <typeinfo>
#include <boost/mpl/copy_if.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/msm/back/tools.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

#include <cxxabi.h>
#include <CDspTaskHelper.h>
#include <CDspDispConnection.h>
#include <boost/mpl/has_xxx.hpp>
#include <prlsdk/PrlErrorsValues.h>
#include "Task_MigrateVmQObject_p.h"
#include <prlcommon/Logging/Logging.h>
#include <boost/phoenix/core/value.hpp>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Interfaces/ParallelsDispToDispProto.h>

namespace Migrate
{
namespace Vm
{
namespace msmf = boost::msm::front;

using IOService::IOPackage;
using IOService::IOSendJob;

///////////////////////////////////////////////////////////////////////////////
// struct Finished

struct Finished: msmf::terminate_state<>
{
	Finished()
	{
	}
	explicit Finished(CDspTaskHelper& task_): m_task(&task_)
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		m_task->quit();
	}

private:
	CDspTaskHelper* m_task;
};

QString demangle(const char* );

///////////////////////////////////////////////////////////////////////////////
// struct Trace

template<class T>
struct Trace: boost::msm::front::state<>
{
	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "enter %s state\nevent %s\n",
			qPrintable(demangle()), qPrintable(Trace<Event>::demangle()));
	}
	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "leave %s state\nevent %s\n",
			qPrintable(demangle()), qPrintable(Trace<Event>::demangle()));
	}
	static QString demangle()
	{
		return Vm::demangle(typeid(T).name());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Success

struct Success: msmf::exit_pseudo_state<boost::msm::front::none>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T>
struct Frontend: msmf::state_machine_def<T>
{
	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "enter %s state\nevent %s\n",
			qPrintable(Trace<T>::demangle()), qPrintable(Trace<Event>::demangle()));
	}
	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "leave %s state\nevent %s\n",
			qPrintable(Trace<T>::demangle()), qPrintable(Trace<Event>::demangle()));
	}
	template <class FSM, class Event>
	void no_transition(const Event& , FSM&, int state_)
	{
		typedef typename boost::msm::back::recursive_get_transition_table<FSM>::type recursive_stt;
		typedef typename boost::msm::back::generate_state_set<recursive_stt>::type all_states;
		std::string n;
		boost::mpl::for_each<all_states,boost::msm::wrap<boost::mpl::_1> >
			(boost::msm::back::get_state_name<recursive_stt>(n, state_));

		WRITE_TRACE(DBG_FATAL, "no transition from state %s on event %s\n",
			qPrintable(demangle(n.c_str())),
			qPrintable(Trace<Event>::demangle()));
	}
	template <class FSM,class Event>
	void exception_caught (Event const&,FSM&, std::exception& exception_)
	{
		WRITE_TRACE(DBG_FATAL, "exception %s, fsm %s, event %s\n", exception_.what(),
			qPrintable(Trace<FSM>::demangle()),
			qPrintable(Trace<Event>::demangle()));
	}
};

namespace Flop
{
typedef boost::variant<PRL_RESULT, SmartPtr<CVmEvent> > unexpected_type;

///////////////////////////////////////////////////////////////////////////////
// struct Event

struct Event: Prl::Expected<void, unexpected_type>
{
	Event()
	{
	}
	explicit Event(boost::mpl::at_c<unexpected_type::types, 0>::type value_):
		Prl::Expected<void, unexpected_type>(unexpected_type(value_))
	{
	}
	explicit Event(const boost::mpl::at_c<unexpected_type::types, 1>::type& value_):
		Prl::Expected<void, unexpected_type>(unexpected_type(value_))
	{
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct State

struct State: boost::msm::front::exit_pseudo_state<Event>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action
{
	template<class T, class U, class V>
	void operator()(const T& , U& fsm_, V& , V&)
	{
		fsm_.process_event(Event());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor: boost::static_visitor<void>
{
	explicit Visitor(CDspTaskHelper& task_): m_task(&task_)
	{
	}

	void operator()(boost::mpl::at_c<unexpected_type::types, 0>::type value_) const
	{
		(CDspTaskFailure(*m_task))(value_);
	}
	void operator()(const boost::mpl::at_c<unexpected_type::types, 1>::type& value_) const
	{
		(CDspTaskFailure(*m_task))(*value_);
	}

private:
	CDspTaskHelper* m_task;
};

} // namespace Flop 

namespace Join
{
namespace Function
{
///////////////////////////////////////////////////////////////////////////////
// struct Self

template<class T>
struct Self
{
	typedef typename T::self_type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flop

template<class T>
struct Flop
{
	typedef typename T::flopExit_type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Quit

template<class T>
struct Quit
{
	typedef typename T::goodExit_type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Good
//
template<class T>
struct Good
{
	typedef typename T::gootEvent_type type;
};

} // namespace Function

///////////////////////////////////////////////////////////////////////////////
// struct State

template<class T, class U>
struct State
{
	typedef T self_type;
	typedef T flopExit_type;
	typedef T goodExit_type;
	typedef U gootEvent_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Machine

template<class T>
struct Machine
{
	typedef msmf::none gootEvent_type;
	typedef boost::msm::back::state_machine<T> backend_type;
	typedef backend_type self_type;
	typedef typename backend_type::template exit_pt<Flop::State> flopExit_type;
	typedef typename backend_type::template exit_pt<Success> goodExit_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T>
struct Frontend: Vm::Frontend<Frontend<T> >
{
	struct Joined
	{
	};

	struct Good: Trace<Good>
	{
		template<typename Event, typename FSM>
		void on_entry(const Event& event_, FSM& fsm_)
		{
			Trace<Good>::on_entry(event_, fsm_);
			++fsm_.m_count;
			if (fsm_.m_count == boost::mpl::size<T>::value)
				fsm_.process_event(Joined());
		}
	};

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Frontend<T> >::on_entry(event_, fsm_);
		m_count = 0;
	}

	typedef typename boost::mpl::transform
		<
			T,
			Function::Self<boost::mpl::_1>
		>::type initial_state;

	typedef boost::mpl::vector<msmf::Row<Good, Joined, Success> > seed_type;
	typedef typename boost::mpl::transform
		<
			T,
			msmf::Row
			<
				Function::Flop<boost::mpl::_1>,
				Flop::Event,
				Flop::State
			>
		>::type flop_type;

	typedef typename boost::mpl::transform
		<
			T,
			msmf::Row
			<
				Function::Quit<boost::mpl::_1>,
				Function::Good<boost::mpl::_1>,
				Good
			>
		>::type good_type;

	struct transition_table: boost::mpl::copy
		<
			flop_type,
			typename boost::mpl::back_inserter
			<
				typename boost::mpl::copy
				<
					good_type,
					boost::mpl::back_inserter<seed_type>
				>::type
			>
		>::type
	{
	};

private:
	int m_count;
};

} // namespace Join

namespace Connector
{
///////////////////////////////////////////////////////////////////////////////
// struct Base

template<class T>
struct Base
{
	typedef T top_type;

	Base(): m_top()
	{
	}

	template <typename Event>
	void handle(const Event& event_)
	{
		m_top->process_event(event_);
	}
	void setTop(top_type& value_)
	{
		m_top = &value_;
	}

protected:
	top_type* m_top;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mixin

template<class T>
class Mixin
{
	typedef T connector_type;
	typedef typename connector_type::top_type top_type;

public:
	void initConnector(top_type& value_)
	{
		m_connector = QSharedPointer<connector_type>(new connector_type());
		m_connector->setTop(value_);
	}

protected:
	connector_type* getConnector() const
	{
		return m_connector.data();
	}

private:
	QSharedPointer<connector_type> m_connector;
};

///////////////////////////////////////////////////////////////////////////////
// struct Initializer

template <typename T>
struct Initializer
{
	explicit Initializer(T& top_): m_top(&top_)
	{
	}

	template <class U>
	void operator()(Mixin<U>* mixin_)
	{
		mixin_->initConnector(*m_top);
	}

	void operator()(...)
	{
	}

private:
	T* m_top;
};

} // namespace Connector

namespace Pipeline
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

template<class T>
struct Connector: Vm::Connector::Base<T>, Slot
{
	void timeout()
	{
		this->handle(Flop::Event(PRL_ERR_TIMEOUT));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct State

template<class T, class U>
struct State: Trace<State<T, U> >, Vm::Connector::Mixin<Connector<T> >
{
	typedef boost::function1<PRL_RESULT, const SmartPtr<IOPackage>& > callback_type;
	typedef Trace<State<T, U> > def_type;

	explicit State(quint32 timeout_): m_timeout(timeout_)
	{
	}
	State(const callback_type& callback_, quint32 timeout_):
		m_timeout(timeout_), m_callback(callback_)
	{
	}

	State(): m_timeout()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		def_type::on_entry(event_, fsm_);
		m_timer = QSharedPointer<QTimer>(new QTimer());
		m_timer->setSingleShot(true);
		m_timer->start(m_timeout);
		this->getConnector()->connect(m_timer.data(), SIGNAL(timeout()), SLOT(timeout()));
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		def_type::on_exit(event_, fsm_);
		m_timer->disconnect(SIGNAL(timeout()), this->getConnector());
		m_timer->stop();
		m_timer.clear();
	}

	struct Good
	{
	};
	struct Action
	{
		template <typename FSM>
		void operator()(const U& event_, FSM& fsm_, State& state_, State&)
		{
			PRL_RESULT e;
			if (state_.m_callback.empty() ||
				PRL_SUCCEEDED(e = state_.m_callback(event_.getPackage())))
			{
				fsm_.process_event(Good());
				return;
			}
			state_.getConnector()->handle(Flop::Event(e));
		}
	};

	struct internal_transition_table: boost::mpl::vector<
		msmf::Internal<U, Action>
	>
	{
	};

private:
	quint32 m_timeout;
	callback_type m_callback;
	QSharedPointer<QTimer> m_timer;
};

} // namespace Pipeline

namespace Pump
{
struct IO;
typedef QPair<IO*, QIODevice* > Launch_type;

///////////////////////////////////////////////////////////////////////////////
// struct Event

template<Parallels::IDispToDispCommands X>
struct Event
{
	explicit Event(const SmartPtr<IOPackage>& package_):
		m_package(package_)
	{
	}

	const SmartPtr<IOPackage>& getPackage() const
	{
		return m_package;
	}

	static const Parallels::IDispToDispCommands s_command;

private:
	SmartPtr<IOPackage> m_package;
};

template<Parallels::IDispToDispCommands X>
const Parallels::IDispToDispCommands Event<X>::s_command = X;

// NB. target side needs a handshake on finish because it doesn't know if the
// overall process fails. thus it waits for a finish reply from the source to
// complete with ok or for cancel to clean up and fail.
typedef Event<Parallels::VmMigrateFinishCmd> FinishCommand_type;

///////////////////////////////////////////////////////////////////////////////
// struct Quit

template<Parallels::IDispToDispCommands X>
struct Quit
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Reading

struct Reading
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Queue

struct Queue: QQueue<SmartPtr<IOPackage> >
{
	bool isEof() const
	{
		return !isEmpty() && head()->buffersSize() == 0;
	}
};

namespace Push
{
///////////////////////////////////////////////////////////////////////////////
// struct Closing

struct Closing
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Sending

struct Sending
{
};

typedef boost::variant<Reading, Sending, Closing, Success> state_type;
typedef Prl::Expected<state_type, Flop::Event> target_type;

///////////////////////////////////////////////////////////////////////////////
// struct Packer

struct Packer
{
	explicit Packer(Parallels::IDispToDispCommands name_): m_name(name_)
	{
	}

	SmartPtr<IOPackage> operator()();
	SmartPtr<IOPackage> operator()(QIODevice& source_);

private:
	Parallels::IDispToDispCommands m_name;
};

///////////////////////////////////////////////////////////////////////////////
// struct Queue

struct Queue: private Vm::Pump::Queue
{
	Queue(Parallels::IDispToDispCommands name_, IO& service_, QIODevice& device_):
		m_service(&service_), m_packer(name_), m_device(&device_)
	{
	}

	target_type dequeue();
	Prl::Expected<void, Flop::Event> enqueueEof();
	Prl::Expected<void, Flop::Event> enqueueData();

private:
	Prl::Expected<void, Flop::Event> enqueue(const SmartPtr<IOPackage>& package_);

	IO* m_service;
	Packer m_packer;
	QIODevice* m_device;
};

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Eof

struct Eof: boost::static_visitor<target_type>
{
	explicit Eof(Queue& queue_): m_queue(&queue_)
	{
	}

	template<class T>
	target_type operator()(const T& value_) const
	{
		return state_type(value_);
	}
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 0>::type& value_) const;
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 1>::type& value_) const;

private:
	Queue* m_queue;
};

///////////////////////////////////////////////////////////////////////////////
// struct Sent

struct Sent: boost::static_visitor<target_type>
{
	typedef boost::function0<void> callback_type;

	Sent(Queue& queue_, const callback_type& callback_):
		m_queue(&queue_), m_callback(callback_)
	{
	}

	template<class T>
	target_type operator()(const T& value_) const
	{
		return state_type(value_);
	}
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 1>::type& value_) const;
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 2>::type& value_) const;

private:
	Queue* m_queue;
	callback_type m_callback;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ready

struct Ready: boost::static_visitor<target_type>
{
	explicit Ready(Queue& queue_): m_queue(&queue_)
	{
	}

	template<class T>
	target_type operator()(const T& value_) const
	{
		return state_type(value_);
	}
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 0>::type& value_) const;
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 1>::type& value_) const;

private:
	Queue* m_queue;
};

} // namespace Visitor

///////////////////////////////////////////////////////////////////////////////
// struct Connector

template<class T, Parallels::IDispToDispCommands X>
struct Connector: Vm::Connector::Base<T>, Slot
{
	void setQueue(Queue* value_)
	{
		m_queue = QSharedPointer<Queue>(value_);
	}
	void onSent(const SmartPtr<IOPackage>& package_)
	{
		if (!(package_.isValid() && package_->header.type == X))
			return;

		if (m_queue.isNull())
			return this->handle(Flop::Event(PRL_ERR_UNINITIALIZED));

		this->setState(boost::apply_visitor(Visitor::Sent(*m_queue,
			boost::bind(&Connector::template handle<Quit<X> >, this, Quit<X>())),
				m_state));
	}

	void readyRead()
	{
		if (m_queue.isNull())
			return this->handle(Flop::Event(PRL_ERR_UNINITIALIZED));

		this->setState(boost::apply_visitor(Visitor::Ready(*m_queue), m_state));
	}

	void readChannelFinished()
	{
		if (m_queue.isNull())
			return this->handle(Flop::Event(PRL_ERR_UNINITIALIZED));

		this->setState(boost::apply_visitor(Visitor::Eof(*m_queue), m_state));
	}

private:
	void setState(const target_type& value_)
	{
		if (value_.isFailed())
			this->handle(value_.error());
		else
			m_state = value_.value();

	}

	state_type m_state;
	QSharedPointer<Queue> m_queue;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pump

template<class T, Parallels::IDispToDispCommands X>
struct Pump: Trace<Pump<T, X> >, Vm::Connector::Mixin<Connector<T, X> >
{
	Pump(): m_ioservice(), m_iodevice()
	{
	}

	using Trace<Pump>::on_entry;

	template <typename FSM>
	void on_entry(const Launch_type& event_, FSM& fsm_)
	{
		bool x;
		Trace<Pump>::on_entry(event_, fsm_);
		m_ioservice = event_.first;
		m_iodevice = event_.second;
		this->getConnector()->setQueue(new Queue(X, *m_ioservice, *m_iodevice));
		x = this->getConnector()->connect(m_iodevice, SIGNAL(readyRead()),
			SLOT(readyRead()), Qt::QueuedConnection);
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect");
		x = this->getConnector()->connect(m_iodevice,
			SIGNAL(readChannelFinished()), SLOT(readChannelFinished()),
			Qt::QueuedConnection);
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect");
		x = this->getConnector()->connect(m_ioservice,
			SIGNAL(onSent(const SmartPtr<IOPackage>&)),
			SLOT(onSent(const SmartPtr<IOPackage>&)),
			Qt::QueuedConnection);
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect");
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		Trace<Pump>::on_exit(event_, fsm_);
		m_iodevice->disconnect(SIGNAL(readyRead()), this->getConnector(),
			SLOT(readyRead()));
		m_iodevice->disconnect(SIGNAL(readChannelFinished()),
			this->getConnector(), SLOT(readChannelFinished()));
		m_ioservice->disconnect(SIGNAL(onSent(const SmartPtr<IOPackage>&)),
			this->getConnector(),
			SLOT(onSent(const SmartPtr<IOPackage>&)));

		this->getConnector()->setQueue(NULL);
		m_iodevice = NULL;
		m_ioservice = NULL;
	}

private:
	IO* m_ioservice;
	QIODevice *m_iodevice;
};

} // namespace Push

namespace Pull
{
///////////////////////////////////////////////////////////////////////////////
// struct WateringPot

struct WateringPot
{
	WateringPot(const SmartPtr<IOPackage>& load_, QIODevice& device_):
		m_done(), m_device(&device_), m_load(load_)
	{
	}

	qint64 getLevel() const
	{
		return getVolume() - m_done;
	}
	Prl::Expected<qint64, Flop::Event> operator()();

private:
	qint64 getVolume() const;

	qint64 m_done;
	QIODevice* m_device;
	SmartPtr<IOPackage> m_load;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pouring

struct Pouring
{
	typedef Prl::Expected<void, Flop::Event> status_type;

	Pouring(const SmartPtr<IOPackage>& load_, QIODevice& device_):
		m_portion(), m_pot(load_, device_)
	{
	}

	qint64 getRemaining() const
	{
		return m_pot.getLevel() + m_portion;
	}
	status_type operator()();
	status_type account(qint64 value_);

private:
	qint64 m_portion;
	WateringPot m_pot;
};

typedef boost::phoenix::expression::value<Pouring>::type writing_type;
typedef boost::variant<Reading, Pouring, Success, writing_type> state_type;
typedef Prl::Expected<state_type, Flop::Event> target_type;

///////////////////////////////////////////////////////////////////////////////
// struct Queue

struct Queue: private Vm::Pump::Queue
{
	explicit Queue(QIODevice& device_): m_device(&device_)
	{
	}

	target_type dequeue();
	using Vm::Pump::Queue::enqueue;

private:
	QIODevice* m_device;
};

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Receipt

struct Receipt: boost::static_visitor<target_type>
{
	explicit Receipt(Queue& queue_): m_queue(&queue_)
	{
	}

	template<class T>
	target_type operator()(const T& value_) const
	{
		return state_type(value_);
	}
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 0>::type& value_) const;

private:
	Queue* m_queue;
};

///////////////////////////////////////////////////////////////////////////////
// struct Accounting

struct Accounting: boost::static_visitor<target_type>
{
	Accounting(qint64 amount_, Queue& queue_): m_amount(amount_), m_queue(&queue_)
	{
	}

	template<class T>
	target_type operator()(const T& value_) const
	{
		return state_type(value_);
	}
	target_type operator()
		(boost::mpl::at_c<state_type::types, 3>::type value_) const;

private:
	qint64 m_amount;
	Queue* m_queue;
};

///////////////////////////////////////////////////////////////////////////////
// struct Dispatch

struct Dispatch: boost::static_visitor<target_type>
{
	typedef boost::function0<void> callback_type;

	explicit Dispatch(const callback_type& callback_): m_callback(callback_)
	{
	}

	template<class T>
	target_type operator()(const T& value_) const
	{
		return state_type(value_);
	}
	target_type operator()
		(boost::mpl::at_c<state_type::types, 1>::type value_) const;
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 2>::type& value_) const;

private:
	callback_type m_callback;
};

} // namespace Visitor

///////////////////////////////////////////////////////////////////////////////
// struct Connector

template<class T, Parallels::IDispToDispCommands X>
struct Connector: Vm::Connector::Base<T>, Slot
{
	void setQueue(Queue* value_)
	{
		m_queue = QSharedPointer<Queue>(value_);
	}
	void reactReceipt(const SmartPtr<IOPackage>& package_)
	{
		if (m_queue.isNull())
			return this->handle(Flop::Event(PRL_ERR_UNINITIALIZED));

		m_queue->enqueue(package_);
		setState(boost::apply_visitor(Visitor::Receipt(*m_queue), m_state));
	}
	void reactBytesWritten(qint64 value_)
	{
		if (m_queue.isNull())
			return this->handle(Flop::Event(PRL_ERR_UNINITIALIZED));

		setState(boost::apply_visitor(Visitor::Accounting(value_, *m_queue), m_state));
	}

private:
	void setState(const target_type& value_)
	{
		if (value_.isFailed())
			return this->handle(value_.error());

		target_type x = boost::apply_visitor(Visitor::Dispatch
			(boost::bind
				(&Connector::template handle<Quit<X> >, this, Quit<X>())),
					m_state = value_.value());
		if (x.isFailed())
			return this->handle(x.error());

		m_state = x.value();
	}

	state_type m_state;
	QSharedPointer<Queue> m_queue;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pump

template<class T, Parallels::IDispToDispCommands X>
struct Pump: Trace<Pump<T, X> >, Vm::Connector::Mixin<Connector<T, X> >
{
	Pump(): m_iodevice()
	{
	}

	using Trace<Pump>::on_entry;

	template <typename FSM>
	void on_entry(const Launch_type& event_, FSM& fsm_)
	{
		Trace<Pump>::on_entry(event_, fsm_);
		m_iodevice = event_.second;
		this->getConnector()->setQueue(new Queue(*m_iodevice));
		this->getConnector()->connect(m_iodevice, SIGNAL(bytesWritten(qint64)),
			SLOT(reactBytesWritten(qint64)), Qt::QueuedConnection);
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		Trace<Pump>::on_exit(event_, fsm_);
		this->getConnector()->setQueue(NULL);
		m_iodevice->disconnect(SIGNAL(bytesWritten(qint64)),
			this->getConnector(), SLOT(reactBytesWritten(qint64)));
		m_iodevice = NULL;
	}

	struct Action
	{
		template<class M, class S, class D>
		void operator()(const Event<X>& event_, M&, S& state_, D& )
		{
			state_.getConnector()->reactReceipt(event_.getPackage());
		}
	};

	struct internal_transition_table: boost::mpl::vector<
		msmf::Internal<Event<X>, Action>
	>
	{
	};

private:
	QIODevice *m_iodevice;
};

} // namespace Pull
} // namespace Pump

namespace Tunnel
{
typedef Pump::Event<Parallels::VmMigrateLibvirtTunnelChunk> libvirtChunk_type;

///////////////////////////////////////////////////////////////////////////////
// struct Prime

struct Prime: Trace<Prime>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Ready

struct Ready: Trace<Ready>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Essence

template<class L, class Q1, class Q2, class E>
struct Essence: Vm::Frontend<Essence<L, Q1, Q2, E> >
{
	struct Entry: msmf::entry_pseudo_state<0>
	{
	};
	typedef boost::msm::back::state_machine<Q1> qemu1State_type;
	typedef boost::msm::back::state_machine<Q2> qemu2State_type;
	typedef boost::msm::back::state_machine<L> libvirtState_type;
	typedef boost::mpl::vector<libvirtState_type, qemu1State_type, qemu2State_type> initial_state;

	struct transition_table : boost::mpl::vector<
		msmf::Row<Entry,                         E,          libvirtState_type>,
		msmf::Row<typename libvirtState_type
			::template exit_pt<Success>,     msmf::none, Success>,
		msmf::Row<typename qemu1State_type
			::template exit_pt<Flop::State>, Flop::Event,Flop::State>,
		msmf::Row<typename qemu2State_type
			::template exit_pt<Flop::State>, Flop::Event,Flop::State>,
		msmf::Row<typename libvirtState_type
			::template exit_pt<Flop::State>, Flop::Event,Flop::State>
	>
	{
	};
};

} // namespace Tunnel

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Running

struct Running: Trace<Running>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver

template<class T>
struct Driver: Slot, Vm::Connector::Base<T>
{
	void cancel();
	void launch(const QString& program_, const QStringList& arguments_);

protected:
	void reactFinished(int code_, QProcess::ExitStatus status_);

private:
	QSharedPointer<QProcess> m_process;
};

template<class T>
void Driver<T>::cancel()
{
	if (!m_process.isNull())
	{
		m_process->disconnect(SIGNAL(finished(int, QProcess::ExitStatus)),
				this, SLOT(reactFinished(int, QProcess::ExitStatus)));
		m_process->terminate();
		// probably should not block on success migration
		// as the process is finished
		m_process->waitForFinished();
		m_process.clear();
	}
}

template<class T>
void Driver<T>::launch(const QString& program_, const QStringList& arguments_)
{
	m_process = QSharedPointer<QProcess>(new QProcess());
	bool x = this->connect(m_process.data(),
			SIGNAL(finished(int, QProcess::ExitStatus)),
			SLOT(reactFinished(int, QProcess::ExitStatus)));
	if (x)
	{
		m_process->start(program_, arguments_);
		if (!m_process->waitForStarted())
		{
			WRITE_TRACE(DBG_FATAL, "failed to start programm '%s'", qPrintable(program_));
			m_process.clear();
			this->handle(Flop::Event(PRL_ERR_FAILURE));
		}
	}
	else
	{
		m_process.clear();
		WRITE_TRACE(DBG_FATAL, "can't connect");
		this->handle(Flop::Event(PRL_ERR_FAILURE));
	}
}

template<class T>
void Driver<T>::reactFinished(int code_, QProcess::ExitStatus status_)
{
	WRITE_TRACE(DBG_FATAL, "process stderr\n%s", m_process->readAllStandardError().data());
	if (status_ == QProcess::NormalExit && code_ == 0)
		this->handle(boost::mpl::true_());
	else
		this->handle(Flop::Event(PRL_ERR_FAILURE));
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend_

template<class T, class U>
struct Frontend_: Vm::Frontend<T>, Vm::Connector::Mixin<Driver<U> >
{
	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<T>::on_exit(event_, fsm_);
		this->getConnector()->cancel();
	}
};

} // namespace Libvirt

BOOST_MPL_HAS_XXX_TRAIT_DEF(stt);

///////////////////////////////////////////////////////////////////////////////
// struct Walker

template<class T, class U = T>
struct Walker
{
	explicit Walker(T& fsm_): m_fsm(&fsm_), m_chain(fsm_)
	{
	}
	Walker(T& fsm_, const Connector::Initializer<U>& chain_):
		m_fsm(&fsm_), m_chain(chain_)
	{
	}

	template<class V>
	typename boost::disable_if<has_stt<V> >::type
		operator()(boost::msm::wrap<V> const&)
	{
		m_chain(m_fsm->template get_state<V*>());
	}
	template<class V>
	typename boost::enable_if<has_stt<V> >::type
		operator()(boost::msm::wrap<V> const&)
	{
		do_(m_fsm->template get_state<V&>());
	}
	void operator()()
	{
		do_(*m_fsm);
	}

private:
	template<class V>
	void do_(V& fsm_)
	{
		typedef typename V::stt stt_type;
		typedef typename boost::msm::back::generate_state_set<stt_type>::type
			set_type;

		m_chain(&fsm_);
		boost::mpl::for_each<set_type, boost::msm::wrap<boost::mpl::_1> >
			(Walker<V, U>(fsm_, m_chain));
	}

	T* m_fsm;
	Connector::Initializer<U> m_chain;
};

} // namespace Vm
} // namespace Migrate

#endif // __TASK_MIGRATEVM_P_H__

