///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackupHelper_p.h
///
/// Source task for Vm backup creation
///
/// Copyright (c) 2016-2017, Parallels International GmbH
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

#ifndef __Task_BackupHelper_p_H_
#define __Task_BackupHelper_p_H_

#include <QObject>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "CDspService.h"
#include "Task_BackupHelper.h"
#include "Task_MigrateVmTunnel_p.h"
#include "CDspVmBackupInfrastructure_p.h"

namespace Backup
{
namespace Tunnel
{
namespace Source
{
struct Frontend;
typedef boost::msm::back::state_machine<Frontend> backend_type;

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject, Migrate::Vm::Connector::Base<backend_type>
{
	Connector(): m_service()
	{
	}

	void setService(Migrate::Vm::Source::Tunnel::IO* value_)
	{
		m_service = value_;
	}

public slots:
	void reactReceive(const SmartPtr<IOPackage>& package_);

	void reactAccept();

	void reactDisconnect();

private:
	Q_OBJECT

	Migrate::Vm::Source::Tunnel::IO* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Terminal

struct Terminal: boost::msm::front::terminate_state<>
{
	Terminal(): m_loop()
	{
	}
	explicit Terminal(QEventLoop& loop_): m_loop(&loop_)
	{
	}

	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		m_loop->exit();
	}

private:
	QEventLoop* m_loop;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: ::Vm::State::Details::Frontend<Frontend>,
	Migrate::Vm::Connector::Mixin<Connector>
{
	typedef ::Vm::State::Details::Frontend<Frontend> def_type;
	typedef QSharedPointer<QTcpSocket> client_type;
	typedef QSharedPointer<QTcpServer> server_type;
	typedef Migrate::Vm::Source::Tunnel::Qemu::Hub
	<
		Migrate::Vm::Tunnel::Hub::Traits
		<
			backend_type,
			Parallels::VmMigrateConnectQemuDiskCmd,
			Parallels::VmMigrateQemuDiskTunnelChunk
		>
	> qemuDisk_type;
	typedef qemuDisk_type initial_state;

	explicit Frontend(Migrate::Vm::Source::Tunnel::IO& service_):
		m_service(&service_)
	{
	}
	Frontend(): m_service()
	{
	}

	using def_type::on_entry;

	template <typename Event, typename FSM>
	void on_entry(const Event& event_, FSM& fsm_);

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_);

	void accept(const client_type& client_);

	void listen(const server_type& server_);

	struct transition_table : boost::mpl::vector
	<
		a_irow
		<
			qemuDisk_type,
			client_type,
			&Frontend::accept
		>,
		a_irow
		<
			qemuDisk_type,
			server_type,
			&Frontend::listen
		>,
		boost::msm::front::Row
		<
			qemuDisk_type,
			Migrate::Vm::Flop::Event,
			Terminal
		>,
		boost::msm::front::Row
		<
			qemuDisk_type,
			qemuDisk_type::Good,
			Terminal
		>
	>
	{
	};

private:
	QList<server_type> m_servers;
	QList<client_type> m_clients;
	Migrate::Vm::Source::Tunnel::IO* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Agent

struct Agent: QObject
{
	explicit Agent(backend_type& backend_): m_backend(&backend_)
	{
	}

	Q_INVOKABLE void cancel();
	Q_INVOKABLE qint32 addStrand(quint16 spice_);

private:
	Q_OBJECT

	backend_type* m_backend;
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject: QRunnable
{
	typedef boost::promise<QWeakPointer<Agent> > promise_type;

	Subject(const SmartPtr<IOClient>& channel_, promise_type& promise_):
		m_channel(channel_)
	{
		m_promise.swap(promise_);
	}

	void run();

private:
	promise_type m_promise;
	SmartPtr<IOClient> m_channel;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	~Unit();

	PRL_RESULT operator()(const SmartPtr<IOClient>& channel_);

	Prl::Expected<QUrl, PRL_RESULT> addStrand(const QUrl& remote_);

private:
	QThreadPool m_pool;
	QWeakPointer<Agent> m_agent;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef QSharedPointer<Unit> value_type;
	typedef Prl::Expected<value_type, PRL_RESULT> result_type;

	Factory(const QString& target_, const SmartPtr<IOClient>& channel_):
		m_target(target_), m_channel(channel_)
	{
	}

	result_type operator()(quint32 flags_) const;

private:
	const QString m_target;
	SmartPtr<IOClient> m_channel;
};

} // namespace Source

namespace Target
{
struct Frontend;
typedef boost::msm::back::state_machine<Frontend> backend_type;

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector: QObject, Migrate::Vm::Connector::Base<backend_type>
{
public slots:
	void reactReceive(const SmartPtr<IOPackage>& package_);

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Terminal

struct Terminal: boost::msm::front::terminate_state<>
{
	Terminal(): m_task()
	{
	}
	explicit Terminal(CDspTaskHelper& task_): m_task(&task_)
	{
	}

	template <typename FSM>
	void on_entry(const Migrate::Vm::Flop::Event& event_, FSM&)
	{
		if (NULL == m_task)
			return;

		boost::apply_visitor(Migrate::Vm::Flop::Visitor(*m_task), event_.error());
		m_task->exit(m_task->getLastErrorCode());
	}
	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "leave state machine with an event %s",
			qPrintable(::Vm::State::Details::Trace<Event>::demangle()));
	}

private:
	CDspTaskHelper* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: ::Vm::State::Details::Frontend<Frontend>,
	Migrate::Vm::Connector::Mixin<Connector>
{
	typedef ::Vm::State::Details::Frontend<Frontend> def_type;
	typedef QSharedPointer<QTcpSocket> client_type;
	typedef SmartPtr<CDspDispConnection> entry_type;
	typedef Migrate::Vm::Target::Tunnel::ioEvent_type ioEvent_type;
	typedef Migrate::Vm::Target::Tunnel::Qemu::Hub
	<
		Migrate::Vm::Tunnel::Hub::Traits
		<
			backend_type,
			Parallels::VmMigrateConnectQemuDiskCmd,
			Parallels::VmMigrateQemuDiskTunnelChunk
		>
	> qemuDisk_type;
	typedef Migrate::Vm::Tunnel::Prime initial_state;

	Frontend(): m_service()
	{
	}

	using def_type::on_entry;

	template <typename FSM>
	void on_entry(const entry_type& event_, FSM& fsm_)
	{
		def_type::on_entry(event_, fsm_);
		if (event_.isValid())
		{
			m_service = QSharedPointer<io_type>(new io_type(*event_));
			getConnector()->connect(m_service.data(),
				SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
				SLOT(reactReceive(const SmartPtr<IOPackage>&)));
		}
		spin(qemuDisk_type::Good());
	}

	template <typename Event, typename FSM>
	void on_exit(const Event& event_, FSM& fsm_)
	{
		def_type::on_exit(event_, fsm_);
		if (!m_service.isNull())
		{
			m_service->disconnect(
				SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
				getConnector());
		}
		m_service.clear();
	}

	void spin(const qemuDisk_type::Good&);

	template<class T, class D>
	static PRL_RESULT decorate(const entry_type& event_, T& task_, D decorated_);

	struct transition_table : boost::mpl::vector
	<
		boost::msm::front::Row
		<
			initial_state,
			ioEvent_type,
			qemuDisk_type
		>,
		boost::msm::front::Row
		<
			initial_state,
			Migrate::Vm::Flop::Event,
			Terminal
		>,
		boost::msm::front::Row
		<
			qemuDisk_type,
			Migrate::Vm::Flop::Event,
			Terminal
		>,
		a_row
		<
			qemuDisk_type,
			qemuDisk_type::Good,
			initial_state,
			&Frontend::spin
		>
	>
	{
	};

private:
	typedef Migrate::Vm::Target::Tunnel::IO io_type;

	QSharedPointer<io_type> m_service;
};

template<class T, class D>
PRL_RESULT Frontend::decorate(const entry_type& event_, T& task_, D decorated_)
{
	Backup::Tunnel::Target::backend_type b(boost::msm::back::states_ << Terminal(task_));
	(Migrate::Vm::Walker<backend_type>(b))();
	b.start(event_);
	PRL_RESULT output = decorated_(&task_);
	b.stop();

	return output;
}

} // namespace Target
} // namespace Tunnel

namespace Work
{
///////////////////////////////////////////////////////////////////////////////
// struct Command

struct Command
{
	Command(Task_BackupHelper& task_, const Activity::Object::Model& activity_)
		: m_context(&task_), m_activity(activity_) {}

	static const QFileInfo * findArchive(const Product::component_type& t_,
		const Activity::Object::Model& a_);

protected:
	Task_BackupHelper *m_context;
	const Activity::Object::Model& m_activity;
};

///////////////////////////////////////////////////////////////////////////////
// struct Loader

struct Loader : boost::static_visitor<Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> >
{
	Loader(const QString& path_, SmartPtr<CDspClient> client_)
		: m_path(path_), m_client(client_) {}

	Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> operator()(const Ct&) const;
	Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> operator()(const Vm&) const;

private:
	QString m_path;
	SmartPtr<CDspClient> m_client;
};

namespace Acronis
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder : boost::static_visitor<QStringList>
{
	Builder(const QString& snapshot_, const Product::component_type& t_,
			const QFileInfo* f_)
		: m_snapshot(snapshot_), m_component(t_), m_file(f_) {}

	QStringList operator()(Ct&) const;
	QStringList operator()(Vm&) const;

private:
	const QString& m_snapshot;
	Product::component_type m_component;
	const QFileInfo *m_file;
};

///////////////////////////////////////////////////////////////////////////////
// struct Archives

struct Archives : boost::static_visitor<Product::componentList_type>
{
	Archives(Product::Model& p_) : m_product(p_) {}

	Product::componentList_type operator()(Ct&) const;
	Product::componentList_type operator()(Vm&) const;

private:
	Product::Model m_product;
};

///////////////////////////////////////////////////////////////////////////////
// struct ACommand

struct ACommand : Command, boost::static_visitor<PRL_RESULT>
{
	typedef boost::function2<PRL_RESULT, const QStringList&, unsigned int> worker_type;

	ACommand(Task_BackupHelper& task_, const Activity::Object::Model& activity_)
		: Command(task_, activity_)
		, m_worker(boost::bind(&Task_BackupHelper::startABackupClient
			, m_context
			, m_context->getProduct()->getObject()
					.getConfig()->getVmIdentification()->getVmName()
			, _1
			, m_context->getProduct()->getObject()
					.getConfig()->getVmIdentification()->getVmUuid()
			, _2))
	{
	}

	PRL_RESULT do_(object_type& mode_);

private:
	QStringList buildArgs(const Product::component_type& t_, const QFileInfo* f_, object_type& o_);

	worker_type m_worker;
};

} // namespace Acronis

namespace Push
{
namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Basic

struct Basic
{
	typedef Product::component_type component_type;

	static QStringList craftEpilog(Task_BackupHelper& context_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct: Basic
{
	explicit Ct(const Activity::Object::Model& activity_): m_activity(&activity_)
	{
	}

	static QString craftProlog(Task_BackupHelper& context_);

	QString craftImage(const component_type& component_, const QString& url_) const;

private:
	const Activity::Object::Model* m_activity;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: Basic
{
	static QStringList craftProlog(Task_BackupHelper& context_);

	static QString craftImage(const component_type& component_, const QString& url_);
};

} // namespace Flavor

///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder : boost::static_visitor<QStringList>
{
	Builder(Task_BackupHelper& context_, const Activity::Object::Model& activity_):
		m_context(&context_), m_activity(&activity_)
	{
	}

	QStringList operator()(Ct&) const;
	QStringList operator()(Vm&) const;
	void addMap(const Activity::Object::component_type& component_, const QUrl& url_);

private:
	template<class T>
	QStringList craft(T subject_) const;

	Task_BackupHelper* m_context;
	QHash<QString, QUrl> m_map;
	const Activity::Object::Model* m_activity;
};

///////////////////////////////////////////////////////////////////////////////
// struct Stopped

struct Stopped
{
	explicit Stopped(const QString& uuid_) : m_uuid(uuid_) {}

	template<class T>
	PRL_RESULT wrap(const T& worker_) const;

private:
	QString m_uuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Frozen

struct Frozen
{
	Frozen(Task_BackupHelper *context_, const QString& uuid_)
		: m_context(context_), m_uuid(uuid_)
		, m_object(uuid_, context_->getClient()) {}

	SmartPtr<Chain> decorate(SmartPtr<Chain> chain_);

private:
	Task_BackupHelper *m_context;
	QString m_uuid;
	::Backup::Snapshot::Vm::Object m_object;
};

typedef boost::variant<boost::blank, Frozen, Stopped> mode_type;

///////////////////////////////////////////////////////////////////////////////
// struct Mode

struct Mode : boost::static_visitor<Prl::Expected<mode_type, PRL_RESULT> >
{
	Mode(const QString& uuid_, Task_BackupHelper *context_)
		: m_uuid(uuid_), m_context(context_) {}

	Prl::Expected<mode_type, PRL_RESULT> operator()(Ct& variant_) const;
	Prl::Expected<mode_type, PRL_RESULT> operator()(Vm& variant_) const;

private:
	QString m_uuid;
	Task_BackupHelper *m_context;
};

///////////////////////////////////////////////////////////////////////////////
// struct VCommand

struct VCommand : Command
{
	typedef Tunnel::Source::Factory::value_type tunnel_type;
	typedef boost::function1<Chain *, const QStringList& > builder_type;
	typedef ::Backup::Activity::Object::componentList_type componentList_type;
	typedef boost::function2<PRL_RESULT, const QStringList&, SmartPtr<Chain> > worker_type;

	VCommand(Task_BackupHelper& task_, const Activity::Object::Model& activity_)
		: Command(task_, activity_)
		, m_uuid(m_context->getProduct()->getObject()
			.getConfig()->getVmIdentification()->getVmUuid())
		, m_builder(boost::bind(&Task_BackupHelper::prepareABackupChain
			, m_context, _1, m_uuid, 0))
		, m_worker(boost::bind(&Task_BackupHelper::startABackupClient
			, m_context
			, m_context->getProduct()->getObject()
				.getConfig()->getVmIdentification()->getVmName()
			, _1
			, _2))

	{
	}

	PRL_RESULT operator()(const Activity::Object::componentList_type& components_,
		object_type& variant_);
	void setTunnel(const tunnel_type& value_)
	{
		m_tunnel = value_;
	}

private:
	QString m_uuid;
	builder_type m_builder;
	tunnel_type m_tunnel;
	worker_type m_worker;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor : boost::static_visitor<PRL_RESULT>
{
	Visitor(VCommand::worker_type worker_, SmartPtr<Chain> chain_,
		const QStringList& args_)
		: m_worker(worker_)
		, m_chain(chain_)
		, m_args(args_) {}

	PRL_RESULT operator()(const boost::blank&) const;
	PRL_RESULT operator()(Stopped& variant_) const;
	PRL_RESULT operator()(Frozen& variant_) const;

private:
	VCommand::worker_type m_worker;
	SmartPtr<Chain> m_chain;
	QStringList m_args;
};

///////////////////////////////////////////////////////////////////////////////
// struct Thaw

struct Thaw: QObject, Chain
{
	Thaw(const ::Backup::Snapshot::Vm::Object& object_) : m_object(object_) {}
	~Thaw();

	PRL_RESULT do_(SmartPtr<IOPackage> request_, process_type& dst_);

public slots:
	void release();

private:
	Q_OBJECT
	void timerEvent(QTimerEvent *event);

	QMutex m_lock;
	boost::optional< ::Backup::Snapshot::Vm::Object> m_object;
};

namespace Bitmap
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder : boost::static_visitor<QStringList>
{
	Builder(SmartPtr<CVmConfiguration> ve_, Product::componentList_type components_)
		: m_config(ve_), m_components(components_) {}

	QStringList operator()(Ct&) const;
	QStringList operator()(Vm&) const;

private:
	SmartPtr<CVmConfiguration> m_config;
	Product::componentList_type m_components;
};

///////////////////////////////////////////////////////////////////////////////
// struct Worker

struct Worker : boost::static_visitor<PRL_RESULT>
{
	typedef boost::function0<PRL_RESULT> worker_type;

	explicit Worker(worker_type worker_) : m_worker(worker_) {}

	PRL_RESULT operator()(Stopped& variant_) const;
	template<class T>
	PRL_RESULT operator()(const T&) const;

private:
	worker_type m_worker;
};

///////////////////////////////////////////////////////////////////////////////
// struct Getter

struct Getter
{
	Getter(Task_BackupHelper &task_, SmartPtr<CVmConfiguration> ve_)
		: m_context(&task_), m_config(ve_) {}

	static PRL_RESULT run(const QStringList& args_, QString& output_);

	QStringList process(const QString& data_);
	Prl::Expected<QStringList, PRL_RESULT> operator()(
		const Activity::Object::Model& activity_, object_type& variant_);

private:
	Task_BackupHelper *m_context;
	SmartPtr<CVmConfiguration> m_config;
};

} // namespace Bitmap
} // namespace Push
} // namespace Work

namespace Process
{
///////////////////////////////////////////////////////////////////////////////
// struct Flop

struct Flop
{
	Flop(const QString& name_, CDspTaskHelper& task_):
		m_name(name_), m_task(&task_)
	{
	}

	void operator()(const QString& program_, int code_, const QString& stderr_);

private:
	QString m_name;
	CDspTaskHelper* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver

struct Driver: QProcess
{
	explicit Driver(int channel_);

	Q_INVOKABLE void write_(SmartPtr<char> data_, quint32 size_);

protected:
	void setupChildProcess();

private:
	Q_OBJECT

	int m_channel;
};

} // namespace Process

namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct State

struct State
{
	explicit State(const SmartPtr<CVmConfiguration>& model_);

	bool equals(const State& other_) const;

private:
	std::map<QString, quint64> m_list;
};

} // namespace Object
} // namespace Backup

namespace
{
enum {QEMU_IMG_RUN_TIMEOUT = 60 * 60 * 1000};

const char QEMU_IMG[] = "/usr/bin/qemu-img";
}

#endif //__Task_BackupHelper_p_H_
