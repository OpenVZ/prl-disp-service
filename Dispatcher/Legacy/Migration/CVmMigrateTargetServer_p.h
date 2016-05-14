///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmMigrateTargetServer_p.h
///
/// Copyright (c) 2016 Parallels IP Holdings GmbH
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

#ifndef CVmMigrateTargetServer_p_H
#define CVmMigrateTargetServer_p_H

#include <prlcommon/Logging/Logging.h>

namespace Migrate
{
namespace Vm
{
namespace Target
{
typedef SmartPtr<IOPackage> package_type;
typedef SmartPtr<CVmConfiguration> config_type;
typedef SmartPtr<CVmFileListCopyTarget> copier_type;

///////////////////////////////////////////////////////////////////////////////
// struct Command

struct Command
{
	virtual ~Command()
	{
	}
	virtual PRL_RESULT execute(const package_type& package_) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cancel

struct Cancel: Command
{
	explicit Cancel(const copier_type& copier_): m_copier(copier_)
	{
	}

	PRL_RESULT execute(const package_type& package_)
	{
		m_copier->RecvCancelRequest(package_);
		return PRL_ERR_OPERATION_WAS_CANCELED;
	}

private:
	copier_type m_copier;
};

///////////////////////////////////////////////////////////////////////////////
// struct Connection

struct Connection
{
	explicit Connection(IOServer& server_): m_server(&server_)
	{
	}

	bool send(const package_type& package_);
	package_type exchange(const package_type& package_);
	void disconnect();
	bool setCopier(const QString& uuid_, const QString& home_, quint32 timeout_);
	const copier_type& getCopier() const
	{
		return m_copier;
	}
	bool setHandle(const IOSender::Handle& value_);
	const IOSender::Handle& getHandle() const
	{
		return m_handle;
	}

private:
	typedef QSharedPointer<CVmFileListCopySender> sender_type;

	IOServer* m_server;
	copier_type m_copier;
	sender_type m_sender;
	IOSender::Handle m_handle;
};

///////////////////////////////////////////////////////////////////////////////
// struct Delegate

template<class T>
struct Delegate: Command
{
	Delegate(Connection& connection_, const T& chain_):
		m_chain(chain_), m_connection(&connection_)
	{
	}

	PRL_RESULT execute(const package_type& package_)
	{
		PRL_RESULT output = m_chain(package_);
		if (PRL_FAILED(output))
			m_connection->disconnect();

		return output;
	}

private:
	T m_chain;
	Connection* m_connection;
};

///////////////////////////////////////////////////////////////////////////////
// struct Return

template<class T>
struct Return: Command
{
	Return(Connection& connection_, const T& chain_):
		m_chain(chain_), m_connection(&connection_)
	{
	}

	PRL_RESULT execute(const package_type& package_)
	{
		PRL_RESULT output = m_chain.execute(package_);
		CDispToDispCommandPtr c =
			CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
				output, package_);
		package_type p = DispatcherPackage::createInstance(c->GetCommandId(),
				 c->GetCommand()->toString(), package_);
		if (!m_connection->send(p))
			WRITE_TRACE(DBG_FATAL, "Reply package sending failure");

		return output;
	}

private:
	T m_chain;
	Connection* m_connection;
};

namespace Disk
{
typedef QHash<QString, CVmMigrateTargetDisk* > taskList_type;

///////////////////////////////////////////////////////////////////////////////
// struct Igniter

struct Igniter
{
	bool operator()(taskList_type& taskList_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Saviour

struct Saviour
{
	virtual ~Saviour();

	void setSuccessor(Saviour* value_)
	{
		m_successor.reset(value_);
	}
	virtual PRL_RESULT handle(const taskList_type& request_) = 0;

private:
	QScopedPointer<Saviour> m_successor;
};

///////////////////////////////////////////////////////////////////////////////
// struct Wait

struct Wait: Saviour
{
	PRL_RESULT handle(const taskList_type& request_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Result

struct Result: Saviour
{
	PRL_RESULT handle(const taskList_type& request_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Tracker

struct Tracker: Saviour
{
	PRL_RESULT handle(const taskList_type& request_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject
{
	Subject(Igniter* igniter_, Saviour* saviour_);
	~Subject();

	PRL_RESULT push(const QString& uuid_, const CVmMigrateDiskBlock_t& header_,
			const SmartPtr<char>& data_);
	PRL_RESULT wait();

private:
	taskList_type m_taskList;
	QScopedPointer<Igniter> m_igniter;
	QScopedPointer<Saviour> m_saviour;
};

///////////////////////////////////////////////////////////////////////////////
// struct Block

struct Block
{
	explicit Block(Subject& subject_): m_subject(&subject_)
	{
	}

	PRL_RESULT operator()(const package_type& package_);

private:
	Subject* m_subject;
};

namespace Map
{
///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject
{
	explicit Subject(const config_type& config_): m_config(config_)
	{
	}

private:
	config_type m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Fill

struct Fill
{
	explicit Fill(Subject& subject_): m_subject(&subject_)
	{
	}

	PRL_RESULT operator()(const package_type& package_);

private:
	Subject* m_subject;
};

///////////////////////////////////////////////////////////////////////////////
// struct Make

struct Make: Command
{
	explicit Make(Subject& subject_): m_subject(&subject_)
	{
	}

	PRL_RESULT execute(const package_type& package_);

private:
	Subject* m_subject;
};

} // namespace Map
} // namespace Disk

namespace Saviour
{
///////////////////////////////////////////////////////////////////////////////
// struct Component

struct Component
{
	virtual ~Component()
	{
	}
	virtual PRL_RESULT execute() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Decorator

struct Decorator: Component
{
	explicit Decorator(Component* decorated_): m_decorated(decorated_)
	{
	}

	virtual PRL_RESULT execute() = 0;

private:
	QScopedPointer<Component> m_decorated;
};

///////////////////////////////////////////////////////////////////////////////
// struct Kill

struct Kill: Component
{
	PRL_RESULT execute();
};

///////////////////////////////////////////////////////////////////////////////
// struct Start

struct Start: Decorator
{
	explicit Start(Component* decorated_): Decorator(decorated_)
	{
	}

	PRL_RESULT execute();
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory
// NB. this one cannot be a component because we need to construct the
// internal chain anyway. that is what the factory is required for.

struct Factory
{
	Factory(bool start_, quint32 flags_, const QString& home_):
		m_start(start_), m_flags(flags_), m_home(home_)
	{
	}

	Component* operator()() const;

private:
	bool m_start;
	quint32 m_flags;
	const QString m_home;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk: Decorator
{
	Disk(Target::Disk::Subject& subject_, Component* decorated_):
		Decorator(decorated_), m_subject(&subject_)
	{
	}

	PRL_RESULT execute();

private:
	Target::Disk::Subject* m_subject;
};

///////////////////////////////////////////////////////////////////////////////
// struct Config

struct Config: Decorator
{
	Config(const QString& /*path_*/, const config_type& config_,
	       Component* decorated_):
		Decorator(decorated_),/* m_assist(path_),*/ m_config(config_)
	{
	}

	PRL_RESULT execute();

private:
	//CStatesHelper m_assist;
	config_type m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Touch

struct Touch: Decorator
{
	Touch(const QString& path_, Component* decorated_):
		Decorator(decorated_), m_path(path_) //CStatesHelper(path_).getSavFileName())
	{
	}

	PRL_RESULT execute()
	{
		// fix for https://jira.sw.ru/browse/PSBM-6460 : clear
		// negative cache of nfs for config.sav
		// int fd = open(QSTR2UTF8(m_path), O_CREAT|O_EXCL, 0600);
		// if (fd >= 0)
		// 	close(fd);

		PRL_RESULT e = Decorator::execute();
		return e;
	}

private:
	QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
// struct Log

struct Log: Decorator
{
	Log(const QString& home_, const QString& name_, Component* decorated_);

	PRL_RESULT execute();

private:
	QString m_home;
	QString m_name;
};

///////////////////////////////////////////////////////////////////////////////
// struct The

struct The
{
	The(const copier_type& copier_, Component* saviour_);

	PRL_RESULT getResult() const
	{
		return m_result;
	}
	void operator()();

private:
	PRL_RESULT m_result;
	copier_type m_copier;
	QScopedPointer<Component> m_saviour;
};

///////////////////////////////////////////////////////////////////////////////
// struct Command

struct Command: Target::Command
{
	explicit Command(const QSharedPointer<Saviour::The>& the_): m_the(the_)
	{
	}

	PRL_RESULT execute(const package_type&)
	{
		(*m_the)();
		return m_the->getResult();
	}

private:
	QSharedPointer<Saviour::The> m_the;
};

} // namespace Saviour

namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(CVmMigrateStartCommand& request_, const CDispCommonPreferences& dispatcher_);

	const QString& getLog() const
	{
		return m_log;
	}
	const QString& getUuid() const
	{
		return m_uuid;
	}
	const QString& getHome() const
	{
		return m_home;
	}
	const config_type& getConfig() const
	{
		return m_config;
	}
	PRL_RESULT operator()();

private:
	QString m_log;
	QString m_home;
	QString m_uuid;
	CParallelsNetworkConfig m_network;
	config_type m_config;
	const CDispCommonPreferences* m_dispatcher;
};

///////////////////////////////////////////////////////////////////////////////
// struct Saviour

struct Saviour
{
	Saviour(quint32 flags_, VIRTUAL_MACHINE_STATE return_);

	Target::Disk::Saviour* saveDisk() const;
	Target::Saviour::Component* saveObject(const Unit& reference_) const;

private:
	bool m_copy;
	quint32 m_flags;
	VIRTUAL_MACHINE_STATE m_return;
};

} // namespace Object

namespace Memory
{
///////////////////////////////////////////////////////////////////////////////
// struct Main

struct Main
{
	PRL_RESULT operator()(const package_type& package_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Video

struct Video
{
	PRL_RESULT operator()(const package_type& package_);
};

} // namespace Memory

namespace Finish
{
///////////////////////////////////////////////////////////////////////////////
// struct Local

struct Local: Command
{
	Local(const QString& uuid_, const QSharedPointer<Saviour::The>& saviour_):
		m_uuid(uuid_), m_saviour(saviour_)
	{
	}

	PRL_RESULT execute(const package_type& package_);

protected:
	Saviour::The& getSaviour() const
	{
		return *m_saviour;
	}

private:
	const QString m_uuid;
	QSharedPointer<Saviour::The> m_saviour;
};

///////////////////////////////////////////////////////////////////////////////
// struct Global

struct Global: Local
{
	explicit Global(const Local& local_): Local(local_)
	{
	}

	PRL_RESULT execute(const package_type& package_)
	{
		(void)Local::execute(package_);
		return getSaviour().getResult();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Return

template<class T>
struct Return: Command
{
	explicit Return(const T& chain_): m_chain(chain_)
	{
	}

	PRL_RESULT execute(const package_type& package_)
	{
		(void)m_chain.execute(package_);
		return PRL_ERR_IO_STOPPED;
	}

private:
	T m_chain;
};

} // namespace Finish

namespace File
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

struct Handler: Command
{
	explicit Handler(const copier_type& chain_): m_chain(chain_)
	{
	}

	PRL_RESULT execute(const package_type& package_);

private:
	copier_type m_chain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cancel

struct Cancel: Handler
{
	explicit Cancel(const SmartPtr<CVmFileListCopyTarget>& chain_):
		Handler(chain_)
	{
	}

	PRL_RESULT execute(const package_type& package_)
	{
		Handler::execute(package_);
		return PRL_ERR_OPERATION_WAS_CANCELED;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Finish

struct Finish: Target::Finish::Local
{
	Finish(const Finish::Local& local_, const Handler& handler_):
		Target::Finish::Local(local_), m_handler(handler_)
	{
	}

	PRL_RESULT execute(const package_type& package_);

private:
	Handler m_handler;
};

} // namespace File

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject
{
	Subject(const Object::Unit& object_, const Object::Saviour& saviour_,
		Connection& connection_);
	~Subject();

	PRL_RESULT operator()(const package_type& package_);

private:
	typedef QSharedPointer<Command> command_type;
	typedef QHash<quint32, command_type> map_type;

	map_type m_map;
	Disk::Subject m_disk;
	File::Handler m_default;
	Disk::Map::Subject m_tracking;
};

} // namespace Target
} // namespace Vm
} // namespace Migrate

#endif // CVmMigrateTargetServer_p_H

