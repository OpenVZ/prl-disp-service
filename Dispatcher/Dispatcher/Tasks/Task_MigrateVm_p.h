///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVm_p.h
//
/// Common part for migration implementation
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

#ifndef __TASK_MIGRATEVM_P_H__
#define __TASK_MIGRATEVM_P_H__

#include <boost/mpl/pair.hpp>
#include <boost/mpl/copy_if.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/mpl/contains.hpp>

#include <CDspTaskHelper.h>
#include <CDspDispConnection.h>
#include <boost/mpl/has_xxx.hpp>
#include <CDspVmStateMachine_p.h>
#include <prlsdk/PrlErrorsValues.h>
#include "Task_MigrateVmQObject_p.h"
#include <prlcommon/Logging/Logging.h>
#include <boost/phoenix/core/value.hpp>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Interfaces/ParallelsDispToDispProto.h>

namespace Migrate
{
namespace Vm
{
namespace msmf = boost::msm::front;
namespace vsd = ::Vm::State::Details;

using IOService::IOPackage;
using IOService::IOSendJob;

BOOST_MPL_HAS_XXX_TRAIT_DEF(stt);

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

///////////////////////////////////////////////////////////////////////////////
// struct Success

struct Success: msmf::exit_pseudo_state<boost::msm::front::none>
{
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

template<class T>
struct Good
{
	typedef typename T::gootEvent_type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Initial

template<class T>
struct Initial
{
	typedef typename boost::mpl::transform
		<
			T,
			Self<boost::mpl::_1>
		>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Table

template<class F, class G, class S, class T>
struct Table
{
	typedef typename boost::mpl::transform
		<
			F,
			msmf::Row
			<
				Function::Flop<boost::mpl::_1>,
				Vm::Flop::Event,
				Vm::Flop::State
			>
		>::type flop_type;

	typedef typename boost::mpl::transform
		<
			G,
			msmf::Row
			<
				Function::Quit<boost::mpl::_1>,
				Function::Good<boost::mpl::_1>,
				T
			>
		>::type good_type;

	typedef typename boost::mpl::copy
		<
			flop_type,
			typename boost::mpl::back_inserter
			<
				typename boost::mpl::copy
				<
					good_type,
					boost::mpl::back_inserter<S>
				>::type
			>
		>::type type;
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
struct Frontend: vsd::Frontend<Frontend<T> >
{
	typedef typename Function::Initial<T>::type initial_state;

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		vsd::Frontend<Frontend<T> >::on_entry(event_, fsm_);
		m_count = 0;
	}

	struct Joined
	{
	};

	struct Good: vsd::Trace<Good>
	{
		template<typename Event, typename FSM>
		void on_entry(const Event& event_, FSM& fsm_)
		{
			vsd::Trace<Good>::on_entry(event_, fsm_);
			++fsm_.m_count;
			if (fsm_.m_count == boost::mpl::size<T>::value)
				fsm_.process_event(Joined());
		}
	};

	struct transition_table: Function::Table
		<
			T,
			T,
			boost::mpl::vector<msmf::Row<Good, Joined, Success> >,
			Good
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
		setConnector(new connector_type());
		m_connector->setTop(value_);
	}

protected:
	connector_type* getConnector() const
	{
		return m_connector.data();
	}

private:
	template<class U>
	typename boost::enable_if<boost::is_base_of<QObject, U> >
	::type setConnector(U* value_)
	{
		m_connector = QSharedPointer<connector_type>(value_,
			&connector_type::deleteLater);
	}

	template<class U>
	typename boost::disable_if<boost::is_base_of<QObject, U> >
	::type setConnector(U* value_)
	{
		m_connector = QSharedPointer<connector_type>(value_);
	}

	QSharedPointer<connector_type> m_connector;
};

template<class A0, class A1, class A2, class A3, class A4>
class Mixin<boost::msm::back::state_machine<A0, A1, A2, A3, A4> >
{
	typedef boost::msm::back::state_machine<A0, A1, A2, A3, A4>
		machine_type;
public:
	void initConnector(machine_type& value_)
	{
		m_machine = &value_;
	}

protected:
	machine_type& getMachine() const
	{
		return *m_machine;
	}

private:
	machine_type* m_machine;
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
struct State: vsd::Trace<State<T, U> >, Vm::Connector::Mixin<Connector<T> >
{
	typedef boost::function1<PRL_RESULT, const SmartPtr<IOPackage>& > callback_type;
	typedef vsd::Trace<State<T, U> > def_type;

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
typedef boost::tuple<IO*, QIODevice*, boost::optional<QString> > Launch_type;

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
	explicit Quit(const QString& ticket_): m_ticket(ticket_)
	{
	}

	const QString& operator()() const
	{
		return m_ticket;
	}

private:
	QString m_ticket;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reading

struct Reading
{
};

namespace Fragment
{
typedef SmartPtr<IOPackage> bin_type;
typedef boost::optional<QString> spice_type;

///////////////////////////////////////////////////////////////////////////////
// struct Format

struct Format
{
	virtual ~Format();

	virtual const char* getData(const bin_type& bin_) const = 0;
	virtual qint64 getDataSize(const bin_type& bin_) const = 0;
	virtual spice_type getSpice(const bin_type& bin_) const = 0;
	virtual bin_type assemble(const spice_type& spice_, const char* data_, qint64 size_) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor<X>

template<Parallels::IDispToDispCommands X>
struct Flavor: Format
{
	const char* getData(const bin_type& bin_) const
	{
		if (bin_.isValid() && bin_->header.buffersNumber > 0)
			return bin_->buffers[0].getImpl();

		return NULL;
	}
	qint64 getDataSize(const bin_type& bin_) const
	{
		if (bin_.isValid() && bin_->header.buffersNumber > 0)
			return IODATAMEMBERCONST(bin_.getImpl())[0].bufferSize;

		return -1;
	}
	spice_type getSpice(const bin_type& bin_) const
	{
		quint32 z = 0;
		SmartPtr<char> b;
		IOPackage::EncodingType t;
		if (!bin_.isValid() || !bin_->getBuffer(1, t, b, z))
			return boost::none;

		return QString::fromAscii(b.getImpl(), z);
	}
	bin_type assemble(const spice_type& spice_, const char* data_, qint64 size_) const
	{
		bin_type output = IOPackage::createInstance(X, 1 + !!spice_);
		if (!output.isValid())
			return output;

		output->fillBuffer(0, IOPackage::RawEncoding, data_, size_);
		if (spice_)
		{
			QByteArray b = spice_.get().toUtf8();
			output->fillBuffer(1, IOPackage::RawEncoding, b.data(), b.size());
		}
		return output;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor<CtMigrateCmd>

template<>
struct Flavor<CtMigrateCmd>: Format
{
	const char* getData(const bin_type& bin_) const;
	qint64 getDataSize(const bin_type& bin_) const;
	spice_type getSpice(const bin_type& bin_) const;
	bin_type assemble(const spice_type& spice_, const char* data_, qint64 size_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Packer

struct Packer
{
	template<Parallels::IDispToDispCommands X>
	explicit Packer(const Flavor<X>& format_): m_format(new Flavor<X>(format_))
	{
	}

	void setSpice(const QString& value_)
	{
		m_spice = value_;
	}
	const Format& getFormat() const
	{
		return *m_format;
	}
	bin_type operator()();
	bin_type operator()(QIODevice& source_);
	bin_type operator()(const QByteArray& data_);
	bin_type operator()(const QTcpSocket& source_);

private:
	spice_type m_spice;
	QSharedPointer<Format> m_format;
};

} // namespace Fragment

///////////////////////////////////////////////////////////////////////////////
// struct Queue

struct Queue: QQueue<Fragment::bin_type>
{
	bool isEof() const
	{
		return !isEmpty() && m_format != NULL &&
			m_format->getDataSize(head()) == 0;
	}
	void setFormat(const Fragment::Format& value_)
	{
		m_format = &value_;
	}

private:
	const Fragment::Format* m_format;
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

///////////////////////////////////////////////////////////////////////////////
// struct Obstruction

struct Obstruction
{
};

typedef boost::variant<Reading, Sending, Closing, Success, Obstruction> state_type;
typedef Prl::Expected<state_type, Flop::Event> target_type;

///////////////////////////////////////////////////////////////////////////////
// struct Queue

struct Queue: private Vm::Pump::Queue
{
	typedef Prl::Expected<void, Flop::Event> enqueue_type;

	Queue(const Fragment::Packer& packer_, IO& service_, QIODevice& device_);

	target_type dequeue();
	enqueue_type enqueueEof();
	enqueue_type enqueueData();
	using Vm::Pump::Queue::size;

private:
	enqueue_type enqueue();
	enqueue_type enqueue(const_reference package_);

	IO* m_service;
	QIODevice* m_device;
	QByteArray m_collector;
	Fragment::Packer m_packer;
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
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 4>::type& value_) const;

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
	target_type operator()
		(const boost::mpl::at_c<state_type::types, 4>::type& value_) const;

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
		if (m_queue.isNull())
			return;

		if (!(package_.isValid() && package_->header.type == X))
			return;

		Fragment::Flavor<X> f;
		if (!this->objectName().isEmpty())
		{
			Fragment::spice_type s = f.getSpice(package_);
			if (!(s && this->objectName() == s.get()))
				return;
		}
		Visitor::Sent::callback_type b;
		if (0 == f.getDataSize(package_))
		{
			b = boost::bind(&Connector::template handle<Quit<X> >,
				this, Quit<X>(this->objectName()));
		}
		this->setState(boost::apply_visitor(Visitor::Sent(*m_queue, b),
				m_state));
	}

	void readyRead()
	{
		if (m_queue.isNull())
			return;

		QObject* s = this->sender();
		if (NULL == s)
			return;

		if (!s->inherits("QIODevice"))
			return;

		if (0 == ((QIODevice*)s)->bytesAvailable())
			return;

		this->setState(boost::apply_visitor(Visitor::Ready(*m_queue), m_state));
	}

	void readChannelFinished()
	{
		if (m_queue.isNull())
			return;

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
struct Pump: vsd::Trace<Pump<T, X> >, Vm::Connector::Mixin<Connector<T, X> >
{
	Pump(): m_ioservice(), m_iodevice()
	{
	}

	using vsd::Trace<Pump>::on_entry;

	template <typename FSM>
	void on_entry(const Launch_type& event_, FSM& fsm_)
	{
		bool x;
		vsd::Trace<Pump>::on_entry(event_, fsm_);
		m_ioservice = event_.get<0>();
		m_iodevice = event_.get<1>();
		Fragment::Packer p((Fragment::Flavor<X>()));
		if (event_.get<2>())
		{
			p.setSpice(event_.get<2>().get());
			this->getConnector()->setObjectName(event_.get<2>().get());
		}
		this->getConnector()->setQueue(new Queue(p, *m_ioservice, *m_iodevice));
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
		vsd::Trace<Pump>::on_exit(event_, fsm_);
		m_iodevice->disconnect(SIGNAL(readyRead()), this->getConnector(),
			SLOT(readyRead()));
		m_iodevice->disconnect(SIGNAL(readChannelFinished()),
			this->getConnector(), SLOT(readChannelFinished()));
		m_ioservice->disconnect(SIGNAL(onSent(const SmartPtr<IOPackage>&)),
			this->getConnector(),
			SLOT(onSent(const SmartPtr<IOPackage>&)));

		this->getConnector()->setObjectName(QString());
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
	WateringPot(const Fragment::Format& format_, const Fragment::bin_type& load_,
		QIODevice& device_): m_done(), m_device(&device_), m_load(load_),
		m_format(&format_)
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
	Fragment::bin_type m_load;
	const Fragment::Format* m_format;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pouring

struct Pouring
{
	typedef Prl::Expected<void, Flop::Event> status_type;

	explicit Pouring(const QList<WateringPot>& pots_):
		m_portion(), m_pots(pots_)
	{
	}

	qint64 getRemaining() const;
	status_type operator()();
	status_type account(qint64 value_);

private:
	qint64 m_portion;
	QList<WateringPot> m_pots;
};

typedef boost::phoenix::expression::value<Pouring>::type writing_type;
typedef boost::variant<Reading, Pouring, Success, writing_type> state_type;
typedef Prl::Expected<state_type, Flop::Event> target_type;

///////////////////////////////////////////////////////////////////////////////
// struct Queue

struct Queue: private Vm::Pump::Queue
{
	template<Parallels::IDispToDispCommands X>
	Queue(const Fragment::Flavor<X>& format_, QIODevice& device_):
		m_device(&device_), m_format(new Fragment::Flavor<X>(format_))
	{
		setFormat(*m_format);
	}

	target_type dequeue();
	using Vm::Pump::Queue::enqueue;

private:
	QIODevice* m_device;
	QSharedPointer<Fragment::Format> m_format;
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
			return;

		m_queue->enqueue(package_);
		setState(boost::apply_visitor(Visitor::Receipt(*m_queue), m_state));
	}
	void reactBytesWritten(qint64 value_)
	{
		if (m_queue.isNull())
			return;

		setState(boost::apply_visitor(Visitor::Accounting(value_, *m_queue), m_state));
	}

private:
	void setState(const target_type& value_)
	{
		if (value_.isFailed())
			return this->handle(value_.error());

		target_type x = boost::apply_visitor(Visitor::Dispatch
			(boost::bind
				(&Connector::template handle<Quit<X> >, this,
					Quit<X>(this->objectName()))),
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
struct Pump: vsd::Trace<Pump<T, X> >, Vm::Connector::Mixin<Connector<T, X> >
{
	Pump(): m_iodevice()
	{
	}

	using vsd::Trace<Pump>::on_entry;

	template <typename FSM>
	void on_entry(const Launch_type& event_, FSM& fsm_)
	{
		vsd::Trace<Pump>::on_entry(event_, fsm_);
		m_iodevice = event_.get<1>();
		this->getConnector()->setQueue(new Queue(Fragment::Flavor<X>(), *m_iodevice));
		this->getConnector()->connect(m_iodevice, SIGNAL(bytesWritten(qint64)),
			SLOT(reactBytesWritten(qint64)), Qt::QueuedConnection);
		if (event_.get<2>())
			this->getConnector()->setObjectName(event_.get<2>().get());
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		vsd::Trace<Pump>::on_exit(event_, fsm_);
		this->getConnector()->setObjectName(QString());
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
// struct Dummy

struct Dummy: vsd::Trace<Dummy>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Prime

struct Prime: vsd::Trace<Prime>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Ready

struct Ready: vsd::Trace<Ready>
{
};

namespace Hub
{
///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T, Parallels::IDispToDispCommands X, Parallels::IDispToDispCommands Y>
struct Traits
{
	typedef T machine_type;
	typedef Vm::Pump::Event<X> spawnEvent_type;
	typedef Vm::Pump::Event<Y> haulEvent_type;
	typedef Vm::Pump::Quit<Y> quitEvent_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T,class U>
struct Unit: vsd::Trace<T>, Vm::Connector::Mixin<typename U::machine_type>
{
	typedef vsd::Trace<T> def_type;
	typedef typename U::pump_type pump_type;
	typedef typename U::haulEvent_type haulEvent_type;
	typedef typename U::quitEvent_type quitEvent_type;
	typedef QHash<QString, pump_type> pumpMap_type;
	typedef Vm::Tunnel::Ready initial_state;

	using def_type::on_exit;

	template <typename FSM>
	void on_exit(const Flop::Event& event_, FSM& fsm_)
	{
		def_type::on_exit(event_, fsm_);
		{
			typedef typename pumpMap_type::iterator iterator_type;
			iterator_type e = m_pumpMap.end();
			iterator_type p = m_pumpMap.begin();
			for (; p != e; ++p)
			{
				p.value().process_event(event_);
				p.value().stop();
			}
		}
		m_pumpMap.clear();
	}

	struct Good
	{
	};
	struct Down
	{
	};
	struct Action
	{
		template<class M>
		void operator()(haulEvent_type const& event_, M& fsm_, Unit& state_, Unit&)
		{
			Vm::Pump::Fragment::spice_type s =
				Vm::Pump::Fragment::Flavor<haulEvent_type::s_command>()
					.getSpice(event_.getPackage());
			if (!s)
				return (void)fsm_.process_event(Flop::Event(PRL_ERR_INVALID_ARG));

			pump_type* p = state_.match(s);
			if (NULL != p)
				p->process_event(event_);
		}
		template<class M>
		void operator()(quitEvent_type const& event_, M& fsm_, Unit& state_, Unit&)
		{
			QString k = event_();
			pump_type* p = state_.match(k);
			if (NULL == p)
				return (void)fsm_.process_event(Flop::Event(PRL_ERR_INVALID_ARG));

			p->process_event(event_);
			p->stop();
			state_.m_pumpMap.remove(k);
			if (state_.m_pumpMap.isEmpty())
			{
				fsm_.process_event(Good());
				state_.getMachine().process_event(Down());
			}
		}
	};

	struct internal_transition_table: boost::mpl::vector<
		msmf::Internal<quitEvent_type, Action>,
		msmf::Internal<haulEvent_type, Action>
	>
	{
	};

protected:
	template<class E>
	bool start(E const& event_, const boost::optional<QString>& spice_)
	{
		if (!spice_)
			return false;

		pump_type& p = m_pumpMap[spice_.get()];
		typedef typename U::machine_type machine_type;
		(Vm::Walker<pump_type, machine_type>(p,
			Vm::Connector::Initializer<machine_type>
				(this->getMachine())))();
		p.start(event_);

		return true;
	}
	pump_type* match(const boost::optional<QString>& spice_)
	{
		if (!spice_)
			return NULL;
		if (!m_pumpMap.contains(spice_.get()))
			return NULL;

		return &m_pumpMap[spice_.get()];
	}

private:
	pumpMap_type m_pumpMap;
};

} // namespace Hub

///////////////////////////////////////////////////////////////////////////////
// struct Essence

template<class L, class Q1, class Q2, class E>
struct Essence: vsd::Frontend<Essence<L, Q1, Q2, E> >
{
	struct Entry: msmf::entry_pseudo_state<0>
	{
	};
	typedef int no_message_queue;
	typedef boost::mpl::vector<L, Q1, Q2> raw_type;
	typedef typename Join::Function::Initial<raw_type>::type initial_state;

	struct transition_table: Join::Function::Table
		<
			raw_type,
			typename boost::mpl::pop_front<raw_type>::type,
			boost::mpl::vector
			<
				msmf::Row<Entry, E, typename boost::mpl::front<initial_state>::type>,
				msmf::Row
				<
					typename Join::Function::Quit<L>::type,
					typename Join::Function::Good<L>::type,
					Success
				>
			>,
			Dummy
		>::type
	{
	};
};

} // namespace Tunnel

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Running

struct Running: vsd::Trace<Running>
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
struct Frontend_: vsd::Frontend<T>, Vm::Connector::Mixin<Driver<U> >
{
	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		vsd::Frontend<T>::on_exit(event_, fsm_);
		this->getConnector()->cancel();
	}
};

} // namespace Libvirt

} // namespace Vm
} // namespace Migrate

#endif // __TASK_MIGRATEVM_P_H__

