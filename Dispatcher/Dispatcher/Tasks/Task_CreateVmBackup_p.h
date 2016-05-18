///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackup_p.h
///
/// Source task for Vm backup creation
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CreateVmBackup_p_H_
#define __Task_CreateVmBackup_p_H_

#include <QObject>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "CDspService.h"
#include "Task_BackupHelper.h"
#include "CDspVmBackupInfrastructure_p.h"

namespace Work
{

///////////////////////////////////////////////////////////////////////////////
// struct Command

struct Command
{
	Command(Task_BackupHelper& task_, ::Backup::Product::Model& p_,
		const ::Backup::Activity::Object::Model& activity_)
		: m_context(&task_), m_product(p_), m_activity(activity_)
	{
	}

	const QFileInfo * findArchive(const ::Backup::Product::component_type& t_);

protected:
	QStringList buildArgs();

	Task_BackupHelper *m_context;
	::Backup::Product::Model& m_product;
	const ::Backup::Activity::Object::Model& m_activity;
};

///////////////////////////////////////////////////////////////////////////////
// struct ACommand

struct ACommand : Command
{
	typedef boost::function2<PRL_RESULT, const QStringList&, unsigned int> worker_type;

	ACommand(Task_BackupHelper& task_, ::Backup::Product::Model& p_,
		const ::Backup::Activity::Object::Model& activity_)
		: Command(task_, p_, activity_)
		, m_worker(boost::bind(&Task_BackupHelper::startABackupClient
			, m_context
			, m_product.getObject().getConfig()->getVmIdentification()->getVmName()
			, _1
			, m_product.getObject().getConfig()->getVmIdentification()->getVmUuid()
			, _2))

	{
	}

	PRL_RESULT do_();

private:
	QStringList buildArgs(const ::Backup::Product::component_type& t_, const QFileInfo* f_);

	worker_type m_worker;
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

///////////////////////////////////////////////////////////////////////////////
// struct VCommand

struct VCommand : Command
{
	typedef boost::function1<Chain *, const QStringList& > builder_type;
	typedef boost::function2<PRL_RESULT, const QStringList&, SmartPtr<Chain> > worker_type;
	typedef boost::variant<boost::blank, Frozen, Stopped> mode_type;

	VCommand(Task_BackupHelper& task_, ::Backup::Product::Model& p_,
		const ::Backup::Activity::Object::Model& activity_)
		: Command(task_, p_, activity_)
		, m_uuid(m_product.getObject().getConfig()->getVmIdentification()->getVmUuid())
		, m_builder(boost::bind(&Task_BackupHelper::prepareABackupChain
			, m_context, _1, m_uuid, 0))
		, m_worker(boost::bind(&Task_BackupHelper::startABackupClient
			, m_context
			, m_product.getObject().getConfig()->getVmIdentification()->getVmName()
			, _1
			, _2))

	{
	}

	PRL_RESULT do_();

private:
	QStringList buildArgs();
	Prl::Expected<mode_type, PRL_RESULT> getMode();

	QString m_uuid;
	builder_type m_builder;
	worker_type m_worker;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor : boost::static_visitor<PRL_RESULT>
{
	typedef boost::function1<PRL_RESULT, SmartPtr<Chain> > worker_type;

	Visitor(VCommand::worker_type worker_, SmartPtr<Chain> chain_,
		const QStringList& args_)
		: m_worker(boost::bind(worker_, args_, _1))
		, m_chain(chain_) {}

	PRL_RESULT operator()(const boost::blank&) const;
	PRL_RESULT operator()(Stopped& variant_) const;
	PRL_RESULT operator()(Frozen& variant_) const;

private:
	worker_type m_worker;
	SmartPtr<Chain> m_chain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Thaw

struct Thaw: QObject, Chain
{
	Thaw(const ::Backup::Snapshot::Vm::Object& object_) : m_object(object_) {}
	~Thaw();

	PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_);

public slots:
	void release();

private:
	Q_OBJECT
	void timerEvent(QTimerEvent *event);

	QMutex m_lock;
	boost::optional< ::Backup::Snapshot::Vm::Object> m_object;
};

} // namespace Work

#endif //__Task_CreateVmBackup_p_H_
