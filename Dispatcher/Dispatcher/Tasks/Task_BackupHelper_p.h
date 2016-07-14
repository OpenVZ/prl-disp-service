///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackupHelper_p.h
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

#ifndef __Task_BackupHelper_p_H_
#define __Task_BackupHelper_p_H_

#include <QObject>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "CDspService.h"
#include "Task_BackupHelper.h"
#include "CDspVmBackupInfrastructure_p.h"

namespace Backup
{
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
///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder : boost::static_visitor<QStringList>
{
	Builder(const Activity::Object::Model& activity_)
		: m_activity(activity_) {}

	QStringList operator()(Ct&) const;
	QStringList operator()(Vm&) const;

private:
	const Activity::Object::Model& m_activity;
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
	typedef boost::function1<Chain *, const QStringList& > builder_type;
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

	PRL_RESULT do_(object_type& variant_);

private:
	QStringList buildArgs(object_type& variant_);

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
} // namespace Backup

namespace
{
enum {QEMU_IMG_RUN_TIMEOUT = 60 * 60 * 1000};

const char QEMU_IMG[] = "/usr/bin/qemu-img";
}

#endif //__Task_BackupHelper_p_H_
