////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CDspVmDirHelper_p.h
///
/// @brief
///	Exclusive tasks internals
///
/// @author shrike
///
/// @date
///	2016-06-24
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmDirHelper_p_H_
#define __CDspVmDirHelper_p_H_

#include <boost/logic/tribool.hpp>

namespace Task
{
namespace Vm
{
namespace Exclusive
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(PVE::IDispatcherCommands command_, const IOSender::Handle& session_,
		const QString &taskId_):
		m_taskId(taskId_), m_session(session_), m_command(command_)
	{
	}

	const QString& getTaskId() const
	{
		return m_taskId;
	}
	const IOSender::Handle& getSession() const
	{
		return m_session;
	}
	PVE::IDispatcherCommands getCommand() const
	{
		return m_command;
	}
	bool conflicts(const Unit& party_) const;

protected:
	boost::logic::tribool isReconciled(const Unit& party_) const;

private:
	QString m_taskId;
	IOSender::Handle m_session;
	PVE::IDispatcherCommands m_command;

};

struct Native;
///////////////////////////////////////////////////////////////////////////////
// struct Newcomer

struct Newcomer: Unit
{
	explicit Newcomer(const Unit& unit_): Unit(unit_)
	{
	}

	boost::logic::tribool isReconciled(const Native& native_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Native

struct Native: Unit
{
	explicit Native(const Unit& unit_): Unit(unit_)
	{
	}

	bool isReconciled(const Newcomer& newcomer_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Event

struct Event
{
	enum
	{
		TIMEOUT = 30000
	};

	explicit Event(QMutex& mutex_);

	void set();
	boost::logic::tribool wait();

private:
	QMutex* m_mutex;
	QSharedPointer<std::pair<bool, QWaitCondition> > m_condition;
};

///////////////////////////////////////////////////////////////////////////////
// struct Conflict

struct Conflict
{
	Conflict(const Unit& running_, const Unit& pending_, const Event& resolved_);

	PRL_RESULT getResult() const;
	PRL_RESULT operator()();

private:
	Unit m_pending;
	Unit m_running;
	Event m_resolved;
};

///////////////////////////////////////////////////////////////////////////////
// struct Gang

struct Gang
{
	explicit Gang(const Event& resolver_): m_resolver(resolver_)
	{
	}

	bool isEmpty() const
	{
		return m_store.isEmpty();
	}
	Conflict* join(const Unit& unit_);
	bool eraseFirst(PVE::IDispatcherCommands command_);
	bool eraseFirst(PVE::IDispatcherCommands command_, const IOSender::Handle& session_);
	const Unit* findFirst(PVE::IDispatcherCommands command_) const;

private:
	typedef QMultiHash<PVE::IDispatcherCommands, Native> store_type;

	Event m_resolver;
	store_type m_store;
};

} // namespace Exclusive
} // namespace Vm
} // namespace Task

namespace List
{
namespace Directory
{
namespace Item
{
///////////////////////////////////////////////////////////////////////////////
// struct Component

struct Component
{
	typedef SmartPtr<CDspClient> session_type;
	typedef SmartPtr<CVmConfiguration> value_type;
	typedef Prl::Expected<value_type, PRL_RESULT> result_type;

	virtual ~Component();

	virtual result_type handle(const CVmDirectoryItem& item_) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Chain

struct Chain: Component
{
	result_type handle(const CVmDirectoryItem& item_);
	void setNext(Component* value_)
	{
		m_next.reset(value_);
	}

private:
	QScopedPointer<Component> m_next;
};

///////////////////////////////////////////////////////////////////////////////
// struct Identity

struct Identity: Component
{
	explicit Identity(const QString& uuid_): m_uuid(uuid_)
	{
	}

	result_type handle(const CVmDirectoryItem& item_);

private:
	QString m_uuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Sterling

struct Sterling: Component
{
	Sterling(const session_type& session_, CDspService& service_):
		m_service(&service_), m_session(session_)
	{
	}

	result_type handle(const CVmDirectoryItem& item_);

private:
	CDspService* m_service;
	session_type m_session;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: Chain
{
	result_type handle(const CVmDirectoryItem& item_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Template

struct Template: Chain
{
	result_type handle(const CVmDirectoryItem& item_);
};

namespace Inaccessible
{
///////////////////////////////////////////////////////////////////////////////
// struct Chain

struct Chain: Item::Chain
{
	Chain(CDspService& service_, const session_type& session_):
		m_service(&service_), m_session(session_)
	{
	}

protected:
	PRL_RESULT determine(const CVmDirectoryItem& item_);
	CDspService& getService() const
	{
		return *m_service;
	}
	const session_type& getSession() const
	{
		return m_session;
	}

private:
	CDspService* m_service;
	session_type m_session;
};

///////////////////////////////////////////////////////////////////////////////
// struct Identity

struct Identity: Chain
{
	Identity(CDspService& service_, const session_type& session_, const Item::Identity& chain_):
		Chain(service_, session_), m_chain(chain_)
	{
	}

	result_type handle(const CVmDirectoryItem& item_);

private:
	Item::Identity m_chain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Default

struct Default: Chain
{
	Default(CDspService& service_, const session_type& session_):
		Chain(service_, session_)
	{
	}

	result_type handle(const CVmDirectoryItem& item_);
	value_type craft(PRL_RESULT code_, const QString& uid_);
	value_type craft(PRL_RESULT code_, const CVmDirectoryItem* item_);

private:
	QString craftName(const CVmDirectoryItem* item_, const QString& uid_) const;
};

} // namespace Inaccessible

///////////////////////////////////////////////////////////////////////////////
// struct State

struct State: Chain
{
	explicit State(const session_type& session_): m_session(session_)
	{
	}

	result_type handle(const CVmDirectoryItem& item_);

private:
	session_type m_session;
};

///////////////////////////////////////////////////////////////////////////////
// struct Extra

struct Extra: Chain
{
	Extra(const session_type& session_, bool autogenerated_, CDspService& service_):
		m_autogenerated(autogenerated_), m_service(&service_), m_session(session_)
	{
	}

	result_type handle(const CVmDirectoryItem& item_);

private:
	bool m_autogenerated;
	CDspService* m_service;
	session_type m_session;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef Chain::session_type session_type;

	explicit Factory(CDspService& service_): m_service(&service_)
	{
	}

	Chain* operator()(const session_type& session_, quint32 flags_) const;

private:
	CDspService* m_service;
};

} // namespace Item

///////////////////////////////////////////////////////////////////////////////
// struct Chain

struct Chain
{
	typedef Item::Component::value_type value_type;
	typedef Item::Component::session_type session_type;

	virtual ~Chain();

	QStringList getResult() const;
	void setNext(Chain* value_)
	{
		m_next.reset(value_);
	}
	virtual PRL_RESULT handle(const CVmDirectory& directory_) = 0;

protected:
	typedef boost::remove_pointer<value_type::StoredType>::type
		item_type;

	void deposit(const item_type& item_)
	{
		m_result << item_.toString();
	}

private:
	QStringList m_result;
	QScopedPointer<Chain> m_next;
};

///////////////////////////////////////////////////////////////////////////////
// struct Loop

struct Loop: Chain
{
	typedef boost::function<bool (const CVmDirectory&)> predicate_type;

	PRL_RESULT handle(const CVmDirectory& directory_);

protected:
	void setLoader(Item::Component* value_)
	{
		m_loader.reset(value_);
	}
	void setPredicate(const predicate_type& predicate_)
	{
		m_predicate = predicate_;
	}

private:
	predicate_type m_predicate;
	QScopedPointer<Item::Component> m_loader;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ephemeral

struct Ephemeral
{
	typedef ::Vm::Directory::Ephemeral::directoryList_type list_type;

	static Loop::predicate_type craftContains(const list_type& list_);
};

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Generic

struct Generic: Loop
{
protected:
	void setLoader(Item::Component* value_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

struct Ordinary: Generic
{
	Ordinary(const Ephemeral::list_type& ephemeral_, Item::Component* loader_);
};

} // namespace Vm

namespace Template
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Generic

struct Generic: Directory::Vm::Generic
{
protected:
	void setLoader(Item::Component* value_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

struct Ordinary: Generic
{
	explicit Ordinary(Item::Component* loader_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Ephemeral

struct Ephemeral: Generic
{
	Ephemeral(const Directory::Ephemeral::list_type& ephemeral_, Item::Component* loader_);
};

} // namespace Vm
} // namespace Template

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct: Chain
{
	Ct(const session_type& session_, quint32 flags_, CDspService& service_);

	PRL_RESULT handle(const CVmDirectory& directory_);

private:
	QString m_uuid;
	quint32 m_flags;
	CDspService* m_service;
	session_type m_session;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef Chain::session_type session_type;
	typedef ::Vm::Directory::Ephemeral::directoryList_type ephemeral_type;

	Factory(CDspService& service_, const ephemeral_type& ephemeral_):
		m_service(&service_), m_ephemeral(ephemeral_)
	{
	}

	Chain* operator()(const session_type& session_, quint32 flags_) const;

private:
	CDspService* m_service;
	ephemeral_type m_ephemeral;
};

} // namespace Directory
} // namespace List

#endif // __CDspVmDirHelper_p_H_

