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
#include <boost/msm/back/tools.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

#include <QObject>
#include <cxxabi.h>
#include <CDspTaskHelper.h>
#include <CDspDispConnection.h>
#include <prlsdk/PrlErrorsValues.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/Logging/Logging.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <Interfaces/ParallelsDispToDispProto.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/IOService/IOCommunication/IOSendJob.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>

namespace Migrate
{
namespace Vm
{
namespace msmf = boost::msm::front;

using IOService::IOPackage;
using IOService::IOSendJob;

///////////////////////////////////////////////////////////////////////////////
// struct Success

struct Success: msmf::exit_pseudo_state<boost::msm::front::none>
{
};

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

///////////////////////////////////////////////////////////////////////////////
// struct Join

template<class T>
struct Join: Vm::Frontend<Join<T> >
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
	template<class U, class V>
	struct Exit
	{
		typedef typename U::template exit_pt<V> type;
	};
	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Join>::on_entry(event_, fsm_);
		m_count = 0;
	}

	typedef typename boost::mpl::transform
		<
			T,
			boost::msm::back::state_machine<boost::mpl::_1>
		>::type initial_state;

	typedef boost::mpl::vector<msmf::Row<Good, Joined, Success> > seed_type;
	typedef typename boost::mpl::transform
		<
			typename boost::mpl::transform
			<
				initial_state,
				Exit<boost::mpl::_1, Flop::State>
			>::type,
			msmf::Row<boost::mpl::_1, Flop::Event, Flop::State>
		>::type flop_type;
	typedef typename boost::mpl::transform
		<
			typename boost::mpl::transform
			<
				initial_state,
				Exit<boost::mpl::_1, Success>
			>::type,
			msmf::Row<boost::mpl::_1, msmf::none, Good>
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
// struct Check

struct Check: Trace<Check>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Waiting

struct Waiting: Trace<Waiting>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
public slots:
	virtual void timeout() = 0;

private:
	Q_OBJECT
};

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
// struct Frontend

template<class T, class U>
struct Frontend: Vm::Frontend<Frontend<T, U> >, Vm::Connector::Mixin<Connector<T> >
{
	typedef boost::function1<PRL_RESULT, const SmartPtr<IOPackage>& > callback_type;
	typedef Waiting initial_state;
	typedef msmf::state_machine_def<Frontend> def_type;

	explicit Frontend(quint32 timeout_): m_timeout(timeout_)
	{
	}
	Frontend(const callback_type& callback_, quint32 timeout_):
		m_timeout(timeout_), m_callback(callback_)
	{
	}

	Frontend(): m_timeout()
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Frontend>::on_entry(event_, fsm_);
		m_timer = QSharedPointer<QTimer>(new QTimer());
		m_timer->setSingleShot(true);
		m_timer->start(m_timeout);
		this->getConnector()->connect(m_timer.data(), SIGNAL(timeout()), SLOT(timeout()));
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Frontend>::on_exit(event_, fsm_);
		m_timer->disconnect(SIGNAL(timeout()), this->getConnector());
		m_timer->stop();
		m_timer.clear();
	}

	void handle(const U& event_)
	{
		if (m_callback.empty())
			return;

		const PRL_RESULT e = m_callback(event_.getPackage());
		if (PRL_FAILED(e))
			this->getConnector()->handle(Flop::Event(e));
	}

	struct transition_table : boost::mpl::vector<
		typename def_type::template
		a_row<Waiting,		U,		Success,	&Frontend::handle>,
		msmf::Row<Waiting,	Flop::Event,	Flop::State>
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

typedef Event<Parallels::VmMigrateTunnelChunk> TunnelChunk_type;
// NB. target side needs a handshake on finish because it doesn't know if the
// overall process fails. thus it waits for a finish reply from the source to
// complete with ok or for cancel to clean up and fail.
typedef Event<Parallels::VmMigrateFinishCmd> FinishCommand_type;

///////////////////////////////////////////////////////////////////////////////
// struct Reading

struct Reading: Trace<Reading>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Queueing

struct Queueing: Trace<Queueing>, QQueue<SmartPtr<IOPackage> >
{
	bool isEof() const
	{
		return !isEmpty() && head()->buffersSize() == 0;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct IO

struct IO: QObject
{
	virtual IOSendJob::Handle sendPackage(const SmartPtr<IOPackage>&) = 0;

signals:
	void onReceived(const SmartPtr<IOPackage>& package_);
	void onSent();

private:
	Q_OBJECT
};

namespace Push
{
///////////////////////////////////////////////////////////////////////////////
// struct Ready

struct Ready
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Eof

struct Eof
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Sent

struct Sent
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Closing

struct Closing: Trace<Closing>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Sending

struct Sending: Trace<Sending>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
public slots:
	virtual void onSent() = 0;
	virtual void readyRead() = 0;
	virtual void readChannelFinished() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

template<class T>
struct Connector: Vm::Connector::Base<T>, Slot
{
	void onSent()
	{
		this->handle(Sent());
	}

	void readyRead()
	{
		this->handle(Ready());
	}

	void readChannelFinished()
	{
		this->handle(Eof());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Packer

struct Packer
{
	SmartPtr<IOPackage> operator()();
	SmartPtr<IOPackage> operator()(QIODevice& source_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Pump

template<class T>
struct Pump: Vm::Frontend<Pump<T> >, Vm::Connector::Mixin<Connector<T> >
{
	typedef Reading initial_state;
	struct Branch
	{
		bool operator()(const msmf::none&, Pump&,
			Queueing& queue_, Reading&)
		{
			return queue_.isEmpty();
		}
		bool operator()(const msmf::none&, Pump&,
			Queueing& queue_, Closing&)
		{
			return queue_.isEof();
		}
		bool operator()(const msmf::none&, Pump&,
			Queueing& queue_, Sending&)
		{
			return !queue_.isEof();
		}
	};
	struct Action
	{
		void operator()(const Ready&, Pump& pump_,
				Reading&, Queueing& queue_)
		{
			enqueue(pump_, Packer()(*pump_.m_iodevice), queue_);
		}
		void operator()(const Ready&, Pump& pump_,
				Sending&, Queueing& queue_)
		{
			if (enqueue(pump_, Packer()(*pump_.m_iodevice), queue_))
			{
				pump_.getConnector()->handle
					(TunnelChunk_type(queue_.back()));
			}
		}
		void operator()(const Eof&, Pump& pump_,
				Reading&, Queueing& queue_)
		{
			enqueue(pump_, Packer()(), queue_);
		}
		void operator()(const Eof&, Pump& pump_,
				Sending&, Queueing& queue_)
		{
			if (enqueue(pump_, Packer()(), queue_))
			{
				pump_.getConnector()->handle
					(TunnelChunk_type(queue_.back()));
			}
		}
		template<class U>
		void operator()(const msmf::none&, Pump& pump_,
				Queueing& queue_, U&)
		{
			IOSendJob::Handle j = pump_.m_ioservice->sendPackage(queue_.dequeue());
			if (j.isValid())
				return;

			pump_.getConnector()->handle(Flop::Event(PRL_ERR_FAILURE));
		}

	private:
		static bool enqueue(const Pump& pump_, const SmartPtr<IOPackage>& package_,
				Queueing& queue_)
		{
			if (package_.isValid())
				queue_.enqueue(package_);
			else
				pump_.getConnector()->handle(Flop::Event(PRL_ERR_FAILURE));

			return package_.isValid();
		}
	};

	Pump(): m_ioservice(), m_iodevice()
	{
	}

	using Vm::Frontend<Pump>::on_entry;

	template <typename FSM>
	void on_entry(const Launch_type& event_, FSM& fsm_)
	{
		bool x;
		Vm::Frontend<Pump>::on_entry(event_, fsm_);
		m_ioservice = event_.first;
		m_iodevice = event_.second;
		x = this->getConnector()->connect(m_iodevice, SIGNAL(readyRead()),
			SLOT(readyRead()));
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect");
		x = this->getConnector()->connect(m_iodevice,
			SIGNAL(readChannelFinished()), SLOT(readChannelFinished()));
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect");
		x = this->getConnector()->connect(m_ioservice, SIGNAL(onSent()),
			SLOT(onSent()));
		if (!x)
			WRITE_TRACE(DBG_FATAL, "can't connect");
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Pump>::on_exit(event_, fsm_);
		m_iodevice->disconnect(SIGNAL(readyRead()), this->getConnector(),
			SLOT(readyRead()));
		m_iodevice->disconnect(SIGNAL(readChannelFinished()),
			this->getConnector(), SLOT(readChannelFinished()));
		m_ioservice->disconnect(SIGNAL(onSent()), this->getConnector(),
			SLOT(onSent()));
		m_iodevice = NULL;
		m_ioservice = NULL;
	}

	struct transition_table : boost::mpl::vector<
		msmf::Row<Reading,	Ready,		Queueing,	Action>,
		msmf::Row<Reading,	Eof,		Queueing,	Action>,
		msmf::Row<Reading,	Flop::Event,	Flop::State>,
		msmf::Row<Queueing,	msmf::none,	Sending,	Action,		Branch>,
		msmf::Row<Queueing,	msmf::none,	Closing,	Action,		Branch>,
		msmf::Row<Queueing,	msmf::none,	Reading,	msmf::none,	Branch>,
		msmf::Row<Queueing,	TunnelChunk_type,Sending>,
		msmf::Row<Queueing,	Flop::Event,	Flop::State>,
		msmf::Row<Closing,	Flop::Event,	Flop::State>,
		msmf::Row<Closing,	Sent,		Success>,
		msmf::Row<Sending,	Sent,		Queueing>,
		msmf::Row<Sending,	Flop::Event,	Flop::State>,
		msmf::Row<Sending,	Ready,		Queueing,	Action>,
		msmf::Row<Sending,	Eof,		Queueing,	Action>
	>
	{
	};

private:
	IO* m_ioservice;
	QIODevice *m_iodevice;
};

} // namespace Push

namespace Pull
{
///////////////////////////////////////////////////////////////////////////////
// struct BytesWritten

struct BytesWritten
{
	explicit BytesWritten(qint64 value_): m_value(value_)
	{
	}

	qint64 getValue() const
	{
		return m_value;
	}

private:
	qint64 m_value;
};

///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
public slots:
	virtual void reactBytesWritten(qint64 value_) = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

template<class T>
struct Connector: Vm::Connector::Base<T>, Slot
{
	void reactBytesWritten(qint64 value_)
	{
		this->handle(BytesWritten(value_));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Accounting

struct Accounting: Trace<Accounting>
{
	Accounting(): m_value()
	{
	}

	void setValue(qint64 value_)
	{
		m_value = value_;
	}
	qint64 getValue() const
	{
		return m_value;
	}

private:
	qint64 m_value;
};

///////////////////////////////////////////////////////////////////////////////
// struct Writing

struct Writing: Trace<Writing>
{
	Writing(): m_sent()
	{
	}

	PRL_RESULT operator()(QIODevice& sink_);
	qint64 getRemaining() const
	{
		return getVolume() - m_sent;
	}
	void account(qint64 value_)
	{
		m_sent += value_;
	}
	void setup(const SmartPtr<IOPackage>& value_);

private:
	qint64 getVolume() const;

	qint64 m_sent;
	SmartPtr<IOPackage> m_package;
};

///////////////////////////////////////////////////////////////////////////////
// Pump

template<class T>
struct Pump: Vm::Frontend<Pump<T> >, Vm::Connector::Mixin<Connector<T> >
{
	typedef Reading initial_state;

	Pump(): m_iodevice()
	{
	}

	using Vm::Frontend<Pump>::on_entry;

	template <typename FSM>
	void on_entry(const Launch_type& event_, FSM& fsm_)
	{
		Vm::Frontend<Pump>::on_exit(event_, fsm_);
		m_iodevice = event_.second;
		this->getConnector()->connect(m_iodevice, SIGNAL(bytesWritten(qint64)),
			SLOT(reactBytesWritten(qint64)));
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		Vm::Frontend<Pump>::on_exit(event_, fsm_);
		m_iodevice->disconnect(SIGNAL(bytesWritten(qint64)),
			this->getConnector(), SLOT(reactBytesWritten(qint64)));
		m_iodevice = NULL;
	}

	struct Completion
	{
		bool operator()(const msmf::none&, Pump&, Queueing& queue_, Success&)
		{
			return queue_.isEof();
		}
		bool operator()(const msmf::none&, Pump&,
			Accounting& record_, Writing&)
		{
			return 0 < record_.getValue();
		}
	};

	struct Branch
	{
		bool operator()(const msmf::none&, Pump&,
			Queueing& queue_, Reading&)
		{
			return queue_.isEmpty();
		}
		bool operator()(const msmf::none&, Pump&,
			Queueing& queue_, Writing&)
		{
			return !queue_.isEof();
		}
	};

	struct Action
	{
		void operator()(const BytesWritten& event_, Pump&,
				Writing& job_, Accounting& record_)
		{
			job_.account(event_.getValue());
			record_.setValue(job_.getRemaining());
		}
		void operator()(const msmf::none&, Pump& pump_,
				Queueing& queue_, Writing& job_)
		{
			job_.setup(queue_.dequeue());
			write(pump_, job_);
		}
		void operator()(const msmf::none&, Pump& pump_,
				Accounting& , Writing& job_)
		{
			write(pump_, job_);
		}
		void operator()(const TunnelChunk_type& event_, Pump&,
				Reading&, Queueing& queue_)
		{
			queue_.enqueue(event_.getPackage());
		}
		void operator()(const TunnelChunk_type& event_, Pump&,
				Queueing& queue_, Writing&)
		{
			queue_.enqueue(event_.getPackage());
		}
		template<class U>
		void operator()(const TunnelChunk_type& event_, U& fsm_,
				Writing&, Queueing&)
		{
			fsm_.process_event(event_);
		}
	
	private:
		void write(Pump& pump_, Writing& job_)
		{
			PRL_RESULT e = job_(*pump_.m_iodevice);
			if (PRL_FAILED(e))
				pump_.getConnector()->handle(Flop::Event(e));
		}
	};

	struct transition_table : boost::mpl::vector<
		msmf::Row<Reading,	TunnelChunk_type,Queueing,	Action>,
		msmf::Row<Reading,	Flop::Event,	Flop::State>,
		msmf::Row<Queueing,	msmf::none,	Writing,	Action,		Branch>,
		msmf::Row<Queueing,	msmf::none,	Reading,	msmf::none,	Branch>,
		msmf::Row<Queueing,	msmf::none,	Success,	msmf::none,	Completion>,
		msmf::Row<Queueing,	TunnelChunk_type,Writing,	Action>,
		msmf::Row<Writing,	BytesWritten,	Accounting,	Action>,
		msmf::Row<Writing,	Flop::Event,	Flop::State>,
		msmf::Row<Writing,	TunnelChunk_type,Queueing,	Action>,
		msmf::Row<Accounting,	msmf::none,	Queueing>,
		msmf::Row<Accounting,	msmf::none,	Writing,	Action,		Completion>
	>
	{
	};

private:
	QIODevice *m_iodevice;
};

} // namespace Pull

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T, class U>
struct Frontend: Vm::Frontend<Frontend<T, U> >
{
	typedef Push::Pump<T> toFrontend_type;
	typedef Pull::Pump<T> fromFrontend_type;
	typedef boost::msm::back::state_machine<toFrontend_type> toMachine_type;
	typedef boost::msm::back::state_machine<fromFrontend_type> fromMachine_type;

	struct transition_table : boost::mpl::vector<
		msmf::Row<typename fromMachine_type::template exit_pt<Flop::State>,
			Flop::Event, Flop::State>,
		msmf::Row<typename toMachine_type::template exit_pt<Flop::State>,
			Flop::Event, Flop::State>,
		msmf::Row<typename boost::mpl::if_<U, toMachine_type, fromMachine_type>::type::
			template exit_pt<Success>, msmf::none,	Success>
	>
	{
	};

	typedef boost::mpl::vector<fromMachine_type, toMachine_type> initial_state;
};

} // namespace Pump

namespace Tunnel
{
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

} // namespace Tunnel

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Running

struct Running: Trace<Running>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot: QObject
{
protected slots:
	virtual void reactFinished(int, QProcess::ExitStatus) = 0;

private:
	Q_OBJECT
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
		m_process->start(program_, arguments_);
	else
	{
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

	template <class StateType>
	void operator()(boost::msm::wrap<StateType> const&)
	{
		m_chain(m_fsm->template get_state<StateType*>());
	}

	template <class A0, class A1, class A2, class A3, class A4>
	void operator()(boost::msm::wrap<boost::msm::back::state_machine<A0, A1, A2, A3, A4> > const&)
	{
		typedef boost::msm::back::state_machine<A0, A1, A2, A3, A4> fsm_type;
		do_(m_fsm->template get_state<fsm_type&>());
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

