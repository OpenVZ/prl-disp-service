///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspBackupDevice.h
///
/// Custom storage support for HDD
///
/// @author ibazhitov
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDspBackupDevice_H_
#define __CDspBackupDevice_H_

#include <set>
#include <QString>
#include <QUrl>
#include <prlxmlmodel/VmConfig/CVmHardDisk.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include "CDspClient.h"
#include "CDspTaskHelper.h"
#include <prlcommon/Std/noncopyable.h>
#include "Tasks/Task_AttachVmBackup.h"
#include <boost/function.hpp>

namespace Backup
{
namespace Device
{
namespace Agent
{
///////////////////////////////////////////////////////////////////////////////
// struct Alone

struct Alone
{
	static PRL_RESULT run(CDspTaskHelper& task_);
	static SmartPtr<IOPackage> getRequest(int type_, const QString& body_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Nested

struct Nested
{
	explicit Nested(CDspTaskHelper& parent_): m_parent(&parent_)
	{
	}

	SmartPtr<IOPackage> getRequest(int type_, const QString& body_) const;
	PRL_RESULT run(CDspTaskHelper& task_) const
	{
		return m_parent->runExternalTask(&task_);
	}

private:
	CDspTaskHelper* m_parent;
};
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(CDspTaskHelper& parent_):
		m_parent(&parent_), m_user(parent_.getClient())
	{
	}
	Unit(const SmartPtr<CDspClient>& user_, CDspTaskHelper* parent_ = NULL):
		m_parent(parent_), m_user(user_)
	{
	}

	SmartPtr<CDspTaskHelper> getResult() const
	{
		return m_result;
	}
	template<class U>
	PRL_RESULT operator()(const U& event_)
	{
		m_result.reset();
		if (NULL == m_parent)
			return do_(Alone(), event_);
		else
			return do_(Nested(*m_parent), event_);
	}
private:
	template<class T, class U>
	PRL_RESULT do_(T policy_, const U& event_);

	CDspTaskHelper* m_parent;
	SmartPtr<CDspClient> m_user;
	SmartPtr<CDspTaskHelper> m_result;
};

} // namespace Agent

namespace Event
{
struct Visitor;
///////////////////////////////////////////////////////////////////////////////
// struct Base

struct Base
{
	Base(const QString& uuid_, const CVmHardDisk& model_);
	virtual ~Base();

	const QString& getVmUuid() const
	{
		return m_uuid;
	}
	const CVmHardDisk& getModel() const
	{
		return *m_model;
	}
	void setModel(const CVmHardDisk& model_)
	{
		m_model = &model_;
	}

	virtual PRL_RESULT accept(Visitor& visitor_) = 0;

private:
	QString m_uuid;
	const CVmHardDisk* m_model;
};

///////////////////////////////////////////////////////////////////////////////
// struct Setup

struct Setup: Base
{
	Setup(const QString& uuid_, const QString& home_, CVmHardDisk& model_):
		Base(uuid_, model_), m_home(home_), m_model(&model_)
	{
	}

	const QString& getVmHome() const
	{
		return m_home;
	}
	PRL_RESULT accept(Visitor& visitor_);
	PRL_RESULT setModel(Attach::Wrap::Hdd& src_);

private:
	QString m_home;
	CVmHardDisk* m_model;
};

///////////////////////////////////////////////////////////////////////////////
// struct Enable

struct Enable: Base
{
	Enable(const QString& uuid_, const CVmHardDisk& model_):
		Base(uuid_, model_)
	{
	}

	PRL_RESULT accept(Visitor& visitor_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Disconnect

template<bool F>
struct Disconnect: Base
{
	Disconnect(const QString& uuid_, const QString& home_, const CVmHardDisk& model_);

	const QString& getVmHome() const
	{
		return m_home;
	}

	PRL_RESULT accept(Visitor& visitor_);

private:
	QString m_home;
};

typedef Disconnect<false> Disable;
typedef Disconnect<true> Teardown;

///////////////////////////////////////////////////////////////////////////////
// struct Cleanup

struct Cleanup: Base
{
	Cleanup(const QString& uuid_, const CVmHardDisk& model_):
		Base(uuid_, model_)
	{
	}

	PRL_RESULT accept(Visitor& visitor_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T>
struct Traits;

template<>
struct Traits<Setup>
{
	static int getRequestType()
	{
		return VmBackupAttachCmd;
	}
	static QString getRequestBody(const Setup& event_);
	static CDspTaskHelper* getTask(SmartPtr<CDspClient> user_,
					const SmartPtr<IOPackage>& package_);
	static PRL_RESULT handle(const SmartPtr<CDspTaskHelper>& src_, Setup& dst_);
};

template<>
struct Traits<Enable>
{
	static int getRequestType()
	{
		return VmBackupConnectSourceCmd;
	}
	static QString getRequestBody(const Enable& event_);
	static CDspTaskHelper* getTask(SmartPtr<CDspClient> user_,
					const SmartPtr<IOPackage>& package_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Disconnector

struct Disconnector
{
	explicit Disconnector(CVmEvent* topic_ = NULL): m_topic(topic_)
	{
	}

	Attach::Wrap::Hdd* getHdd() const
	{
		return m_hdd.data();
	}
	template<bool F>
	PRL_RESULT operator()(const Disconnect<F>& event_);

private:
	CVmEvent* m_topic;
	QScopedPointer<Attach::Wrap::Hdd> m_hdd;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor
{
	Visitor(): m_topic()
	{
	}

	virtual ~Visitor()
	{
	}

	virtual PRL_RESULT visit(Setup& event_) = 0;
	virtual PRL_RESULT visit(const Enable& event_) = 0;
	virtual PRL_RESULT visit(const Disable& event_) = 0;
	virtual PRL_RESULT visit(const Teardown& event_) = 0;
	virtual PRL_RESULT visit(const Cleanup& event_) = 0;

	virtual void setTopic(CVmEvent* value_)
	{
		m_topic = value_;
	}

protected:
	CVmEvent* m_topic;
};

///////////////////////////////////////////////////////////////////////////////
// struct Handler

struct Handler : Visitor
{
	PRL_RESULT visit(Setup& event_);
	PRL_RESULT visit(const Enable& event_);
	PRL_RESULT visit(const Disable& event_);
	PRL_RESULT visit(const Teardown& event_);
	PRL_RESULT visit(const Cleanup& event_);

	void setAgent(Agent::Unit* value_)
	{
		m_agent.reset(value_);
	}

private:
	template<class T>
	PRL_RESULT delegate(const T& event_);

	QScopedPointer<Agent::Unit> m_agent;
};

///////////////////////////////////////////////////////////////////////////////
// struct EditVm

struct EditVm : Visitor
{
	EditVm(Agent::Unit* agent_, const QString& uuid_)
		: m_uuid(uuid_)
	{
		m_handler.setAgent(agent_);
	}

	void setTopic(CVmEvent* value_)
	{
		m_handler.setTopic(value_);
	}

	PRL_RESULT visit(Setup& event_)
	{
		return m_handler.visit(event_);
	}

	PRL_RESULT visit(const Cleanup& event_)
	{
		return m_handler.visit(event_);
	}

	PRL_RESULT visit(const Enable& event_);
	PRL_RESULT visit(const Disable& event_);
	PRL_RESULT visit(const Teardown& event_);

private:
	bool isRunning(const QString& uuid_) const;

	Handler m_handler;
	QString m_uuid;
};

namespace Deferred
{

///////////////////////////////////////////////////////////////////////////////
// struct Base

// XXX: QObject-based class could not be a template class at the same time.
//      To workaround this obstacle, we'll inherit our template class from
//      a QObject-based class with pure virtual functions.
struct Base : QObject
{
	virtual ~Base()
	{
	}

public slots:
	virtual void execute(QString uuid_, QString deviceAlias_) = 0;

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Action

template<class T>
struct Action : Base
{
	Action(const CVmHardDisk& model_, const T& event_);
	void execute(QString uuid_, QString deviceAlias_);
	static PRL_RESULT start(const T& event_);

private:
	void do_();

	CVmHardDisk m_model;
	T m_event;
};

} // namespace Deferred

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef QList<Base* > result_type;
	typedef QPair<Enable*, Disable* > updated_type;

	Factory(const QString& uuid_, const QString& home_): m_uuid(uuid_),
		m_home(home_)
	{
	}

	result_type novel(CVmHardDisk* disk_) const;
	result_type removed(CVmHardDisk* disk_) const;
	updated_type updated(CVmHardDisk* from_, CVmHardDisk* to_) const;
	result_type disabled(CVmHardDisk* disk_) const;
private:

	QString m_uuid;
	QString m_home;
};

} // namespace Event

///////////////////////////////////////////////////////////////////////////////
// struct Dao

struct Dao
{
	typedef SmartPtr<CVmConfiguration> dataSource_type;

	explicit Dao(const dataSource_type& dataSource_) : m_dataSource(dataSource_)
	{
	}

	bool deleteAll();
	QList<CVmHardDisk* > getAll() const;
	QList<CVmHardDisk* > getEnabled() const;
	QList<CVmHardDisk* > getDisabled() const;
private:
	QList<CVmHardDisk* >& getStore() const
	{
		return m_dataSource->getVmHardwareList()->m_lstHardDisks;
	}

	SmartPtr<CVmConfiguration> m_dataSource;
};

namespace Details
{
struct Transition;
} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// struct Framework

struct Framework
{
	Framework() : m_topic()
	{
	}

	Framework(const QString& home_, const QString& uuid_);

	virtual ~Framework()
	{
	}

	void setVmHome(const QString& value_)
	{
		m_home = value_;
	}

	void setContext(const Agent::Unit& agent_, CVmEvent* topic_ = NULL)
	{
		Event::Handler *h = new Event::Handler();
		h->setAgent(new Agent::Unit(agent_));
		m_visitor.reset(h);
		setTopic(topic_);
	}

	void setContext(CDspTaskHelper& context_)
	{
		Event::Handler *h = new Event::Handler();
		h->setAgent(new Agent::Unit(context_));
		m_visitor.reset(h);
		setTopic(context_.getLastError());
	}

	void setVisitor(Event::Visitor *visitor_)
	{
		m_visitor.reset(visitor_);
	}

	void setTopic(CVmEvent* topic_)
	{
		m_topic = topic_;
	}

protected:

	QString m_home;
	QString m_uuid;
	CVmEvent* m_topic;
	QScopedPointer<Event::Visitor> m_visitor;
};

///////////////////////////////////////////////////////////////////////////////
// struct Service

struct Service : Framework
{
	explicit Service(const Dao::dataSource_type& dataSource_);

	void disable();
	void teardown();
	void enable();
	Details::Transition getTransition(const Dao::dataSource_type& new_);
	PRL_RESULT setDifference(const Dao::dataSource_type& new_);

	Service& setVmHome(const QString& value_)
	{
		Framework::setVmHome(value_);
		return *this;
	}

	Service& setContext(const Agent::Unit& agent_, CVmEvent* topic_ = NULL)
	{
		Framework::setContext(agent_, topic_);
		return *this;
	}

	Service& setContext(CDspTaskHelper& context_)
	{
		Framework::setContext(context_);
		return *this;
	}

	Service& setVisitor(Event::Visitor *visitor_)
	{
		Framework::setVisitor(visitor_);
		return *this;
	}

	Service& setTopic(CVmEvent* topic_)
	{
		Framework::setTopic(topic_);
		return *this;
	}

private:
	Dao m_dao;
};

///////////////////////////////////////////////////////////////////////////////
// struct Oneshot

struct Oneshot : Framework
{
	Oneshot(const QString& home_, const QString& uuid_)
		: Framework(home_, uuid_)
	{
	}

	PRL_RESULT disable(const CVmHardDisk& disk_);
	PRL_RESULT enable(const CVmHardDisk& disk_);

	Oneshot& setVisitor(Event::Visitor *visitor_)
	{
		Framework::setVisitor(visitor_);
		return *this;
	}
};

bool isAttached(const QString& backupId);

namespace Details
{
///////////////////////////////////////////////////////////////////////////////
// struct Finding

struct Finding
{
	explicit Finding(const CVmHardDisk& object_): m_object(&object_)
	{
	}

	QString getUrlPath() const
	{
		QUrl u;
		return getStorageUrl(u) ? u.path() : QString();
	}
	bool isKindOf() const;
	bool isEnabled() const;
	const CVmHardDisk& getObject() const
	{
		return *m_object;
	}
private:
	bool getStorageUrl(QUrl& dst_) const;

	const CVmHardDisk* m_object;
};

///////////////////////////////////////////////////////////////////////////////
// struct IndexInRange

struct IndexInRange: std::unary_function<CVmHardDisk* , bool>
{
	explicit IndexInRange(CVmHardDisk* one_)
	{
		m_good.insert(one_->getIndex());
	}
	explicit IndexInRange(const QList<CVmHardDisk* >& range_);

	result_type operator()(argument_type candidate_) const
	{
		return 0 < m_good.count(candidate_->getIndex());
	}
private:
	std::set<int> m_good;
};

namespace Url
{

///////////////////////////////////////////////////////////////////////////////
// struct Compare

template <class T>
struct Compare: std::unary_function<CVmHardDisk*, bool>
{
	Compare(const Finding& finding_, T policy_)
		: m_finding(finding_), m_policy(policy_)
	{
	}

	result_type operator()(argument_type candidate_) const
	{
		return m_policy(Finding(*candidate_), m_finding);
	}

private:
	const Finding& m_finding;
	T& m_policy;
};

///////////////////////////////////////////////////////////////////////////////
// struct Same - matches two different disks, that have the same non-empty URL

struct Same
{
	bool operator()(const Finding& new_, const Finding& old_) const
	{
		if (new_.getObject().getIndex() == old_.getObject().getIndex())
			return false;
		QString path(new_.getUrlPath());
		return !path.isEmpty() && path == old_.getUrlPath();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Changed - matches two instances of one disk with different URLs

struct Changed
{
	bool operator()(const Finding& new_, const Finding& old_) const
	{
		if (new_.getObject().getIndex() != old_.getObject().getIndex())
			return false;
		return new_.getUrlPath() != old_.getUrlPath();
	}
};

} // namespace Url

///////////////////////////////////////////////////////////////////////////////
// struct Batch

struct Batch : noncopyable
{
	typedef QList<Event::Base* > store_type;

	~Batch()
	{
		sweep();
	}
	void addEvent(const store_type& event_)
	{
		m_store << event_;
	}

	PRL_RESULT operator()(Event::Visitor& visitor_);

private:
	void sweep();

	store_type m_store;
};

///////////////////////////////////////////////////////////////////////////////
// struct Difference

struct Difference
{
	typedef QList<CVmHardDisk* > operand_type;

	explicit Difference(const operand_type& from_, const operand_type& to_,
		const Event::Factory& factory_);

	PRL_RESULT getNovel(Batch& batch_);
	PRL_RESULT getRemoved(Batch& batch_);
	PRL_RESULT getUpdated(Batch *enabled_, Batch *disabled_);

private:
	PRL_RESULT validate();

	operand_type m_from;
	operand_type m_to;
	Event::Factory m_factory;
	PRL_RESULT m_result;
	bool m_validated;
};

///////////////////////////////////////////////////////////////////////////////
// struct Transition

struct Transition
{
	Transition(const Difference& diff_, Event::Visitor &visitor_, CVmEvent *topic_);

	PRL_RESULT replace();
	PRL_RESULT remove();
	PRL_RESULT plant();

private:
	PRL_RESULT do_(Batch& batch_);

	Difference m_diff;
	Event::Visitor& m_visitor;
	CVmEvent *m_topic;
};

///////////////////////////////////////////////////////////////////////////////
// struct Validator

struct Validator {
	typedef QList<CVmHardDisk*> operand_type;

	Validator(const operand_type& old_, const operand_type& new_)
		: m_old(old_), m_new(new_)
	{
	}

	bool checkModified();
	bool checkDuplicated();
	PRL_RESULT operator()();

private:
	template<class T>
	bool check(const Finding& finding_, T checker_)
	{
		operand_type::const_iterator e = m_new.end();
		return (std::find_if(m_new.constBegin(), e,
			Url::Compare<T>(finding_, checker_)) != e);
	}

	const operand_type& m_old;
	const operand_type& m_new;
};

} // namespace Details

} // namespace Device
} // namespace Backup
#endif // __CDspBackupDevice_H_

