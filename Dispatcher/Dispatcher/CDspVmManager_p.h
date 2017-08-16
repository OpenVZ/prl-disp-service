///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmManager_p.h
///
/// Public interfaces of the libvirt interaction.
///
/// @author shrike
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPVMMANAGER_P_H__
#define __CDSPVMMANAGER_P_H__

#include "CDspClient.h"
#include "CDspLibvirt.h"
#include "CDspRegistry.h"
#include "CDspTaskTrace.h"
#include <boost/mpl/copy.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/signals2/signal.hpp>
#include <prlcommon/Std/SmartPtr.h>
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>

class CDspVmDirHelper;

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Context

struct Context
{
	Context(const SmartPtr<CDspClient>& session_, const SmartPtr<IOPackage>& package_);

	const CVmIdent& getIdent() const
	{
		return m_ident;
	}
	const QString& getVmUuid() const
	{
		return m_ident.first;
	}
	const QString& getDirectoryUuid() const
	{
		return m_ident.second;
	}
	const SmartPtr<CDspClient>& getSession() const
	{
		return m_session;
	}
	const SmartPtr<IOPackage>& getPackage() const
	{
		return m_package;
	}
	const CProtoCommandPtr& getRequest() const
	{
		return m_request;
	}
	PVE::IDispatcherCommands getCommand() const
	{
		return m_request->GetCommandId();
	}
	void reportStart();
	void reply(int code_) const;
	void reply(const CVmEvent& error_) const;
	void reply(const Libvirt::Result& result_) const;

private:
	SmartPtr<CDspClient> m_session;
	SmartPtr<IOPackage> m_package;
	CProtoCommandPtr m_request;
	CVmIdent m_ident;
	Task::Trace m_trace;
};

namespace Tag
{
///////////////////////////////////////////////////////////////////////////////
// struct Simple

template<PVE::IDispatcherCommands X>
struct Simple
{
};

///////////////////////////////////////////////////////////////////////////////
// struct General

template<PVE::IDispatcherCommands X>
struct General
{
};

///////////////////////////////////////////////////////////////////////////////
// struct CreateDspVm

template<PVE::IDispatcherCommands X>
struct CreateDspVm: General<X>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestSession

struct GuestSession
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Special

template<PVE::IDispatcherCommands X>
struct Special
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Libvirt

template<PVE::IDispatcherCommands X>
struct Libvirt
{
};

///////////////////////////////////////////////////////////////////////////////
// struct ForK

template<class T>
struct Fork
{
};

///////////////////////////////////////////////////////////////////////////////
// struct State

template<class T, class U>
struct State
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Timeout

template<class T, class U>
struct Timeout
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Reply

template<class T>
struct Reply
{
};

///////////////////////////////////////////////////////////////////////////////
// struct IsAsync

template<class T>
struct IsAsync: boost::mpl::false_
{
};

template<class T, class U>
struct IsAsync<Tag::State<T, U> >: boost::mpl::true_
{
};

template<class T, class U>
struct IsAsync<Tag::Timeout<T, U> >: boost::mpl::true_
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Lock

template<PVE::IDispatcherCommands X, class T>
struct Lock
{
};

} // namespace Tag

namespace Need
{
///////////////////////////////////////////////////////////////////////////////
// class Agent

class Agent
{
	typedef Libvirt::Instrument::Agent::Vm::Unit value_type;

public:
	static Libvirt::Result meetRequirements(const ::Command::Context& context_, Agent& dst_);

protected:
	value_type getAgent() const
	{
		return m_agent;
	}

private:
	value_type m_agent;
};

///////////////////////////////////////////////////////////////////////////////
// class Config

class Config
{
	typedef SmartPtr<CVmConfiguration> value_type;

public:
	static Libvirt::Result meetRequirements(const ::Command::Context& context_, Config& dst_);

protected:
	const value_type& getConfig() const
	{
		return m_config;
	}

private:
	value_type m_config;
};

///////////////////////////////////////////////////////////////////////////////
// class Context

class Context
{
	typedef Command::Context value_type;

public:
	static Libvirt::Result meetRequirements(const ::Command::Context& context_, Context& dst_);

protected:
	void sendEvent(PRL_EVENT_TYPE type_, PRL_EVENT_ISSUER_TYPE issuer_) const;
	void respond(const QString& parameter_, PRL_RESULT code_ = PRL_ERR_SUCCESS) const;
	const value_type& getContext() const
	{
		return m_context.get();
	}

private:
	boost::optional<value_type> m_context;
};

///////////////////////////////////////////////////////////////////////////////
// class Command

template<class T>
class Command
{
	typedef T* value_type;

public:
	void setCommand(value_type value_)
	{
		m_command = value_;
	}

protected:
	value_type getCommand() const
	{
		return m_command;
	}

private:
	value_type m_command;
};

} // namespace Need

///////////////////////////////////////////////////////////////////////////////
// struct Start

struct Start: Need::Agent, Need::Config
{
protected:
	template<class T>
	Libvirt::Result do_(T policy_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Essence

template<PVE::IDispatcherCommands X>
struct Essence;

namespace Vm
{
namespace Fork
{
struct Reactor;

namespace State
{
struct Detector;
} // namespace State

namespace Timeout
{
struct Handler;
} // namespace Timeout;
} // namespace Fork

namespace Prepare
{
///////////////////////////////////////////////////////////////////////////////
// struct Step

template<class U, class V>
struct Step
{
	template<class W>
	static Libvirt::Result do_(const Context& context_, W& launcher_)
	{
		Libvirt::Result e = U::meetRequirements(context_, launcher_);
		if (e.isFailed())
			return e;

		return V::do_(context_, launcher_);
	}
};
template<class U>
struct Step<U, void>
{
	template<class V>
	static Libvirt::Result do_(const Context& context_, V& launcher_)
	{
		return U::meetRequirements(context_, launcher_);
	};
};

///////////////////////////////////////////////////////////////////////////////
// struct Command

template<class T>
class Command
{
	typedef char arrayOfOne_type[1];
	typedef char arrayOfTwo_type[2];

	template<typename U>
	static arrayOfOne_type& func(Need::Command<U>* );

	static arrayOfTwo_type& func(...);

public:
	typedef Command type;
	enum
	{
		value = sizeof(func((T* )0)) == sizeof(arrayOfOne_type)
	};

	template<class U>
	static Libvirt::Result meetRequirements(const Context& context_, Need::Command<U>& dst_)
	{
		U* x = Parallels::CProtoSerializer::CastToProtoCommand<U>
			(context_.getRequest());
		if (NULL == x)
			return Error::Simple(PRL_ERR_UNRECOGNIZED_REQUEST);

		dst_.setCommand(x);
		return Libvirt::Result();
	};
};

///////////////////////////////////////////////////////////////////////////////
// struct Mixture

template<class T>
struct Mixture
{
	typedef typename boost::mpl::vector
		<
			boost::mpl::pair<boost::is_base_of<Need::Agent, T>, Need::Agent>,
			boost::mpl::pair<boost::is_base_of<Need::Config, T>, Need::Config>,
			boost::mpl::pair<boost::is_base_of<Need::Context, T>, Need::Context>,
			boost::mpl::pair<Command<T>, Command<T> >
		>::type ore_type;

	typedef typename boost::mpl::copy
		<
			ore_type,
			boost::mpl::inserter
			<
				boost::mpl::vector<>,
				boost::mpl::if_<boost::mpl::first<boost::mpl::_2>,
					boost::mpl::push_back<boost::mpl::_1, boost::mpl::second<boost::mpl::_2> >,
					boost::mpl::_1>
			>
		>::type type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Policy

template<class T, class U = void>
struct Policy
{
	static Libvirt::Result do_(T launcher_, const Context& context_)
	{
		Libvirt::Result e = boost::mpl::fold
					<
						typename Mixture<T>::type,
						void,
						Step<boost::mpl::_2, boost::mpl::_1>
					>::type::do_(context_, launcher_);
		if (e.isFailed())
			return e;

		return launcher_();
	};
};

template<class T>
struct Policy<T, typename boost::enable_if<boost::mpl::empty<typename Mixture<T>::type> >::type>
{
	static Libvirt::Result do_(T launcher_, const Context& context_)
	{
		Q_UNUSED(context_);
		return launcher_();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Lock

struct Lock: QObject
{
	Lock(const CVmIdent& ident_, const IOSender::Handle& session_, CDspVmDirHelper& service_);
	~Lock();

	PRL_RESULT operator()(PVE::IDispatcherCommands command_);

private:
	CVmIdent m_ident;
	IOSender::Handle m_session;
	CDspVmDirHelper* m_service;
	boost::optional<PVE::IDispatcherCommands> m_command;
};

///////////////////////////////////////////////////////////////////////////////
// struct Extra

struct Extra
{
	Extra(QEventLoop& loop_, QObjectCleanupHandler& tracker_):
		m_loop(&loop_), m_tracker(&tracker_)
	{
	}

	Libvirt::Result operator()(PVE::IDispatcherCommands name_, const CVmIdent& ident_,
		const SmartPtr<CDspClient>& session_);
	Libvirt::Result operator()(quint32 timeout_, Fork::Timeout::Handler* reactor_);

	Libvirt::Result operator()
		(Fork::State::Detector* detector_, Fork::Reactor* reactor_);

private:
	QEventLoop* m_loop;
	QObjectCleanupHandler* m_tracker;
};

} // namespace Prepare

namespace Fork
{
///////////////////////////////////////////////////////////////////////////////
// struct Reactor

struct Reactor: QObject
{
public slots:
	virtual void react() = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Gear

struct Gear
{
	explicit Gear(QEventLoop& loop_): m_loop(&loop_)
	{
	}

	Libvirt::Result operator()(Reactor& reactor_);

private:
	QEventLoop* m_loop;
};

namespace State
{
///////////////////////////////////////////////////////////////////////////////
// struct Check

template <typename V>
struct Check: std::unary_function<unsigned, bool>
{
	typedef QSet<unsigned> storage_t;

	Check()
	{
		boost::mpl::for_each<V>(boost::bind(&storage_t::insert, &m_storage, _1));
	}

	bool operator()(unsigned state_) const
	{
		return m_storage.contains(state_);
	}

private:
	storage_t m_storage;
};

///////////////////////////////////////////////////////////////////////////////
// struct Detector

struct Detector: QObject
{
	Detector(const QString& uuid_, const boost::function1<bool, unsigned>& predicate_):
		m_uuid(uuid_), m_predicate(predicate_)
	{
	}

public slots:
	void react(unsigned oldState_, unsigned newState_, QString vmUuid_, QString dirUuid_);

signals:
	void detected();

private:
	Q_OBJECT

	const QString m_uuid;
	boost::function1<bool, unsigned> m_predicate;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reactor

struct Reactor: Fork::Reactor
{
	explicit Reactor(const ::Command::Context& context_): m_context(context_)
	{
	}

	void react()
	{
		m_context.reply(PRL_ERR_SUCCESS);
	}

private:
	::Command::Context m_context;
};

///////////////////////////////////////////////////////////////////////////////
// struct Plural

template<typename L>
struct Plural
{
	static Detector* craftDetector(const Context& context_)
	{
		return craftDetector(context_.getVmUuid());
	}
	static Detector* craftDetector(const CVmConfiguration& config_)
	{
		return craftDetector(config_.getVmIdentification()->getVmUuid());
	}
	static Detector* craftDetector(const QString& uuid_)
	{
		return new Detector(uuid_, Check<L>());
	}
	static Fork::Reactor* craftReactor(const Context& context_)
	{
		return new Reactor(context_);
	}

};

///////////////////////////////////////////////////////////////////////////////
// struct Strict

template<VIRTUAL_MACHINE_STATE S>
struct Strict: Plural<boost::mpl::vector_c<unsigned, S> >
{
};

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Adapter

struct Adapter: Need::Context
{
	Libvirt::Result operator()()
	{
		getContext().reply(PRL_ERR_SUCCESS);
		sendEvent(PET_DSP_EVT_VM_MEMORY_SWAPPING_FINISHED, PIE_VIRTUAL_MACHINE);
		return Libvirt::Result();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Reactor

struct Reactor: Fork::Reactor
{
	explicit Reactor(const ::Command::Context& context_): m_context(context_)
	{
	}

	void react()
	{
		Prepare::Policy<Adapter>::do_(Adapter(), m_context);
	}

private:
	::Command::Context m_context;
};

///////////////////////////////////////////////////////////////////////////////
// struct Switch

struct Switch: Need::Agent, Need::Config, Need::Context, Need::Command<Parallels::CProtoSwitchToSnapshotCommand>
{
	Switch(VIRTUAL_MACHINE_STATE& state_): m_state(&state_)
	{
	}

	static Detector* craftDetector(const ::Command::Context& context_);
	static Fork::Reactor* craftReactor(const ::Command::Context& context_)
	{
		return new Reactor(context_);
	}
	Libvirt::Result operator()();

private:
	VIRTUAL_MACHINE_STATE* m_state;
};

///////////////////////////////////////////////////////////////////////////////
// struct Response

struct Response: Need::Context, Need::Command<Parallels::CProtoCreateSnapshotCommand>
{
	Libvirt::Result operator()()
	{
		// tree changed
		sendEvent(PET_DSP_EVT_VM_SNAPSHOTS_TREE_CHANGED, PIE_DISPATCHER);
		// snapshooted
		sendEvent(PET_DSP_EVT_VM_SNAPSHOTED, PIE_DISPATCHER);
		// reply
		respond(getCommand()->GetSnapshotUuid());
		// swapping finished
		sendEvent(PET_DSP_EVT_VM_MEMORY_SWAPPING_FINISHED, PIE_VIRTUAL_MACHINE);
		return Libvirt::Result();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Sender

struct Sender: Fork::Reactor
{
	explicit Sender(const ::Command::Context& context_): m_context(context_)
	{
	}

	void react()
	{
		Prepare::Policy<Response>::do_(Response(), m_context);
	}

private:
	::Command::Context m_context;
};

///////////////////////////////////////////////////////////////////////////////
// struct Create

struct Create: Need::Agent
{
	explicit Create(VIRTUAL_MACHINE_STATE& state_): m_state(&state_)
	{
	}

	static Detector* craftDetector(const ::Command::Context& context_);
	static Fork::Reactor* craftReactor(const ::Command::Context& context_)
	{
		return new Sender(context_);
	}
	Libvirt::Result operator()();

private:
	VIRTUAL_MACHINE_STATE* m_state;
};

} // namespace Snapshot
} // namespace State

namespace Timeout
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

struct Handler: Fork::Reactor
{
signals:
	void finish();

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Reactor

template<PVE::IDispatcherCommands X>
struct Reactor: Handler
{
	explicit Reactor(const Context& context_): m_context(context_)
	{
	}

	void react();
	quint32 getInterval() const;

private:
	Context m_context;
};

} // namespace Timeout

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor
{
	Visitor(QEventLoop& loop_, const Context& context_, QObjectCleanupHandler& tracker_):
		m_context(context_), m_loop(&loop_), m_setup(loop_, tracker_)
	{
	}

	template<class T>
	Libvirt::Result operator()
		(T launcher_, boost::mpl::true_ = boost::mpl::true_());

	template<class T>
	Libvirt::Result operator()(T launcher_, boost::mpl::false_);

	template<class T>
	Libvirt::Result operator()(Tag::Reply<T> launcher_);

	template<class T, class U>
	Libvirt::Result operator()
		(Tag::State<T, U>, boost::mpl::true_ = boost::mpl::true_());

	template<class T, PVE::IDispatcherCommands X>
	Libvirt::Result operator()
		(Tag::Timeout<T, Tag::Libvirt<X> >, boost::mpl::true_ = boost::mpl::true_());

	template<class T, PVE::IDispatcherCommands X>
	Libvirt::Result operator()(Tag::Lock<X, T>);

private:
	Context m_context;
	QEventLoop* m_loop;
	Prepare::Extra m_setup;
};

///////////////////////////////////////////////////////////////////////////////
// struct Slip

template<class T>
struct Slip: Reactor
{
	Slip(QEventLoop& loop_, const Context& context_):
		m_context(context_), m_loop(&loop_)
	{
	}

	void react();

private:
	Context m_context;
	QEventLoop* m_loop;
	QObjectCleanupHandler m_tracker;
};

} // namespace Fork

namespace Async
{
///////////////////////////////////////////////////////////////////////////////
// struct Launcher

template<class T>
struct Launcher
{
	Launcher(const Prepare::Extra& setup_, const T& load_, Libvirt::Result& sink_):
		m_load(load_), m_setup(setup_), m_sink(&sink_)
	{
	}

	template<class U>
	void operator()(U launcher_, boost::mpl::false_)
	{
		*m_sink = launcher_(m_load);
	}

	template<class U, class V>
	void operator()(Tag::State<U, V>, boost::mpl::true_ = boost::mpl::true_())
	{
		*m_sink = m_setup(V::craftDetector(m_load), NULL);
		if (m_sink->isSucceed())
			this->operator()(U(), Tag::IsAsync<U>());
	}

	template<class U, class V>
	void operator()(Tag::Timeout<U, V>, boost::mpl::true_ = boost::mpl::true_())
	{
		*m_sink = m_setup(V::getTimeout(), V::craft(m_load, *m_sink));
		if (m_sink->isSucceed())
			this->operator()(U(), Tag::IsAsync<U>());
	}

private:
	T m_load;
	Prepare::Extra m_setup;
	Libvirt::Result* m_sink;
};

///////////////////////////////////////////////////////////////////////////////
// struct Gear

template<class T, class U>
struct Gear: Fork::Reactor
{
	Gear(const U& load_, QEventLoop& loop_):
		m_load(load_), m_loop(&loop_)
	{
	}

	void react()
	{
		Prepare::Extra x(*m_loop, m_tracker);
		Launcher<U>(x, m_load, m_result)(T());
		if (m_result.isFailed())
			m_loop->quit();
	}
	const Libvirt::Result& getResult() const
	{
		return m_result;
	}

private:
	U m_load;
	QEventLoop* m_loop;
	Libvirt::Result m_result;
	QObjectCleanupHandler m_tracker;
};

} // namespace Async

///////////////////////////////////////////////////////////////////////////////
// struct Gear

template<class T, class Enabled = void>
struct Gear
{
	template<class U>
	static Libvirt::Result run(const U& load_)
	{
		return T()(load_);
	}
};

template<class T>
struct Gear<T, typename boost::enable_if<Tag::IsAsync<T> >::type>
{
	template<class U>
	static Libvirt::Result run(const U& load_)
	{
		QEventLoop x;
		Async::Gear<T, U> g(load_, x);
		Libvirt::Result e = Fork::Gear(x)(g);
		if (e.isFailed())
			return e;

		return g.getResult();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Registrator

struct Registrator
{
	Libvirt::Result operator()(const CVmConfiguration& uuid_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Starter

struct Starter
{
	Libvirt::Result operator()(const CVmConfiguration& config_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frankenstein

struct Frankenstein
{
	Libvirt::Result operator()(const QString& uuid_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Resurrect

struct Resurrect
{
	Libvirt::Result operator()(const QString& uuid_);
};

namespace Shutdown
{
///////////////////////////////////////////////////////////////////////////////
// struct Launcher

struct Launcher
{
	Libvirt::Result operator()(const QString& uuid_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Killer

struct Killer
{
	Libvirt::Result operator()(const QString& uuid_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Fallback

struct Fallback: Fork::Timeout::Handler
{
	typedef boost::optional<VIRTUAL_MACHINE_STATE> exception_type;

	Fallback(const QString& uuid_, Libvirt::Result& sink_):
		m_uuid(uuid_), m_sink(&sink_)
	{
		setException(VMS_STOPPED);
	}

	void react();
	void setException(const exception_type& value_)
	{
		m_exception = value_;
	}
	static quint32 getTimeout();
	static Fallback* craft(const QString& uuid_, Libvirt::Result& sink_)
	{
		return new Fallback(uuid_, sink_);
	}

private:
	QString m_uuid;
	exception_type m_exception;
	Libvirt::Result* m_sink;
};

typedef Tag::Timeout<Tag::State<Launcher, Fork::State::Strict<VMS_STOPPED> >, Fallback>
	schema_type;

} // namespace Shutdown
} // namespace Vm

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

struct Hotplug
{
	Hotplug(Libvirt::Instrument::Agent::Vm::Editor runtime_,
		const CVmIdentification& ident_, const SmartPtr<CDspClient>& session_)
		: m_runtime(runtime_), m_ident(ident_), m_session(session_)
	{
	}

	Libvirt::Result plug(const CVmHardDisk& disk_);
	Libvirt::Result unplug(const CVmHardDisk& disk_);

private:
	Libvirt::Instrument::Agent::Vm::Editor m_runtime;
	const CVmIdentification& m_ident;
	SmartPtr<CDspClient> m_session;
};

} // namespace Command

namespace Vm
{
namespace Config
{
namespace Edit
{
///////////////////////////////////////////////////////////////////////////////
// struct Gear

struct Gear
{
	typedef Libvirt::Instrument::Agent::Vm::Unit unit_type;
	typedef boost::signals2::signal<void (CVmConfiguration& )> signal_type;
	typedef boost::function2<Libvirt::Result, unit_type, const CVmConfiguration& >
		configure_type;

	Gear(const QSharedPointer<signal_type>& signal_, const configure_type& configure_)
		: m_signal(signal_), m_configure(configure_)
	{
	}

	void operator()(Registry::Access access_, Libvirt::Instrument::Agent::Vm::Unit unit_);

private:
	QSharedPointer<signal_type> m_signal;
	configure_type m_configure;
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector
{
	Connector(): m_signal(new Gear::signal_type), m_configure()
	{
	}

	void setLimitType(quint32 type_);
	void setCpuFeatures(const CDispCpuPreferences& cpu_);
	boost::optional<Gear> getResult();

private:
	QSharedPointer<Gear::signal_type> m_signal;
	Libvirt::Result (Gear::unit_type::* m_configure)(const CVmConfiguration& );
};

namespace Cpu
{
///////////////////////////////////////////////////////////////////////////////
// struct LimitType

struct LimitType: std::unary_function<PRL_RESULT, CVmConfiguration&>
{
	explicit LimitType(quint32 value_): m_value(value_)
	{
	}

	PRL_RESULT operator()(CVmConfiguration& config_) const;

private:
	quint32 m_value;
};

///////////////////////////////////////////////////////////////////////////////
// struct Features

struct Features: std::unary_function<PRL_RESULT, CVmConfiguration&>
{
	explicit Features(const CDispCpuPreferences& value_): m_value(value_)
	{
	}

	PRL_RESULT operator()(CVmConfiguration& config_) const;

private:
	CDispCpuPreferences m_value;
};

} // namespace Cpu
} // namespace Edit
} // namespace Config
} // namespace Vm

#endif // __CDSPVMMANAGER_P_H__

