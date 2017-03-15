///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackup_p.h
///
/// Private header of restore task
///
/// @author shrike@
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

#ifndef __Task_RestoreVmBackup_p_H_
#define __Task_RestoreVmBackup_p_H_
#include <memory>
#include <boost/function.hpp>
#include "CDspVmBackupInfrastructure_p.h"
#include "Task_BackupHelper.h"
#include <prlcommon/Std/noncopyable.h>
#include <prlcommon/HostUtils/HostUtils.h>

class Task_RestoreVmBackupTarget;
class Task_RestoreVmBackupSource;

namespace Restore
{
struct Assembly;
///////////////////////////////////////////////////////////////////////////////
// struct Toolkit

struct Toolkit
{
	explicit Toolkit(const SmartPtr<CDspClient>& user_);
	explicit Toolkit(CAuthHelper& auth_): m_auth(&auth_)
	{
	}

	bool folderExists(const QString& path_) const;
	PRL_RESULT chown(const QString& path_) const;
	PRL_RESULT mkdir(const QString& path_) const;
	PRL_RESULT unlink(const QFileInfo& path_) const;
	PRL_RESULT rename(const QString& from_, const QString& to_) const;
	PRL_RESULT rename(const QFileInfo& from_, const QFileInfo& to_) const
	{
		return this->rename(from_.absoluteFilePath(),
					to_.absoluteFilePath());
	}
private:
	CAuthHelper* m_auth;
};

///////////////////////////////////////////////////////////////////////////////
// struct Move

struct Move: noncopyable
{
	Move(const QString& from_, const QString& to_, CAuthHelper& auth_);
	~Move();

	bool commit();
	bool revert();
	bool do_();
private:
	bool done() const
	{
		return &m_from != m_trash;
	}

	QString m_from;
	QString m_to;
	QString m_revert;
	QString* m_trash;
	Toolkit m_toolkit;
};

namespace AClient
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(Backup::AClient& impl_, const QString& uuid_, const QString& name_):
		m_name(name_), m_uuid(uuid_), m_impl(&impl_)
	{
	}
	Unit(Backup::AClient& impl_, const QString& uuid_, CVmConfiguration& vm_);

	PRL_RESULT operator()(const QStringList& argv_, unsigned disk_) const;
	PRL_RESULT operator()(const QStringList& argv_, SmartPtr<Chain> custom_) const;
private:
	QString m_name;
	QString m_uuid;
	Backup::AClient* m_impl;
};

///////////////////////////////////////////////////////////////////////////////
// struct Api

struct Api
{
	Api(quint32 no_, const QString& backupRoot_);

	QStringList query(const Backup::Product::component_type& archive_) const;
	QStringList restore(const QString& home_) const
	{
		return restore(QFileInfo(home_));
	}
	QStringList restore(const QString& home_, quint32 veid_) const;
	QStringList restore(const Backup::Product::component_type& archive_, const QFileInfo& target_) const;
private:
	QStringList restore(const QFileInfo& target_) const;
	QStringList restore(const QString& archive_, const QFileInfo& target_) const;

	quint32 m_no;
	QDir m_backupRoot;
};

} // namespace AClient

///////////////////////////////////////////////////////////////////////////////
// struct Program

struct Program: ExecHandlerBase
{
	typedef Prl::Expected<QString, PRL_RESULT> result_type;
	enum
	{
		QUANTUM = 3000
	};

	static result_type execute(const QStringList& argv_, CDspTaskHelper& task_);

	void crashed();

	void waitFailed();

	void exitCode(int value_);

	const result_type& getResult() const
	{
		return m_result;
	}

private:
	Program(QProcess& process_, const QString& name_, CDspTaskHelper& task_);

	CDspTaskHelper* m_task;
	result_type m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Assistant

struct Assistant
{
	Assistant(Task_RestoreVmBackupTarget& task_, const AClient::Unit& unit_);

	CVmEvent* event() const;
	Toolkit getToolkit() const;
	Assembly* getAssembly() const;
	PRL_RESULT operator()(const QStringList& argv_, unsigned disk_) const;
	PRL_RESULT operator()(const QStringList& argv_, SmartPtr<Chain> custom_) const;
	PRL_RESULT operator()(const QString& image_, const QString& archive_,
			const QString& format_) const;
	PRL_RESULT make(const QString& path_, bool failIfExists_) const;
private:
	AClient::Unit m_unit;
	Task_RestoreVmBackupTarget* m_task;
};

namespace Query
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

struct Handler: Chain
{
	Handler(SmartPtr<IOClient> io_, quint32 timeout_);

	quint64 kbs() const
	{
		return m_size >> 10;
	}
	quint64 size() const
	{
		return m_size;
	}
	quint64 usage() const
	{
		return m_usage;
	}
	PRL_RESULT do_(SmartPtr<IOPackage> request_, process_type& dst_);
private:
	quint64 m_size;
	quint64 m_usage;
};

///////////////////////////////////////////////////////////////////////////////
// struct Work

struct Work
{
	Work(const AClient::Api& api_, SmartPtr<IOClient> io_, quint32 timeout_);

	const AClient::Api& getApi() const
	{
		return m_api;
	}
	PRL_RESULT operator()(const Backup::Product::component_type& archive_, const Assistant& assist_,
				quint64& dst_) const;
private:
	quint32 m_timeout;
	AClient::Api m_api;
	SmartPtr<IOClient> m_io;
};

} // namespace Query

namespace Target
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	typedef QPair<quint64, quint64> noSpace_type;

	Vm(quint32 no_, const QString& home_, const QString& backupRoot_,
		const Assistant& assist_);
	~Vm();

	bool isNoSpace(noSpace_type& dst_) const;
	PRL_RESULT restore(quint32 version_) const;
	PRL_RESULT createHome();
	PRL_RESULT add(const ::Backup::Product::component_type& component_);
	Restore::Assembly* assemble(const QString& dst_);
private:
	struct Hdd
	{
		QString final;
		QString mountPoint;
		QFileInfo tib;
		QFileInfo intermediate;
		quint64 sizeOnDisk;
		QString format;
	};
	typedef std::map<int, Hdd> hddMap_type;

	PRL_RESULT make(const QString& path_);
	QStringList make(const char* command_, const Hdd& hdd_) const;
	PRL_RESULT restoreA(const hddMap_type::const_iterator& hdd_) const;
	PRL_RESULT restoreV(const hddMap_type::const_iterator& hdd_) const;

	quint32 m_no;
	QString m_home;
	QString m_backupRoot;
	Assistant m_assist;
	hddMap_type m_hddMap;
	QSet<QString> m_auto;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct
{
	typedef PRL_RESULT (Task_DispToDispConnHelper:: *sendCallback_type)
			(const SmartPtr<IOPackage>& );

	Ct(const Assistant& assist_, Task_DispToDispConnHelper& task_,
		sendCallback_type sendCallback_):
		m_result(PRL_ERR_SUCCESS), m_assist(&assist_), m_task(&task_),
		m_sendCallback(sendCallback_)
	{
	}

	template<class F>
	Restore::Assembly* operator()(F flavor_, const QString& home_, quint32 version_);
	PRL_RESULT getResult() const
	{
		return m_result;
	}
private:
	PRL_RESULT send(IDispToDispCommands command_) const
	{
		SmartPtr<IOPackage> x = IOPackage::createInstance(command_, 0);
		return (m_task->*m_sendCallback)(x);
	}

	PRL_RESULT m_result;
	const Assistant* m_assist;
	Task_DispToDispConnHelper* m_task;
	sendCallback_type m_sendCallback;
};

namespace Ploop
{
///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device
{
	static SmartPtr<Device> make(const QString& path_, quint64 sizeBytes_);
	PRL_RESULT mount();
	PRL_RESULT mount(const QString& mountPoint_);
	PRL_RESULT umount();
	PRL_RESULT setEncryption(const CVmHddEncryption *encryption_);
	const QString& getName() const
	{
		return m_name;
	}
private:
	explicit Device(const QString& path_);
private:
	QString m_path;
	QString m_name;
};

///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image
{
	Image(const Backup::Product::component_type& archive_, const Query::Work& query_);
	~Image();

	void join(Restore::Assembly& dst_);
	PRL_RESULT do_(const Assistant& assist_, quint32 version_);

private:
	QString m_auto;
	QString m_final;
	QString m_intermediate;
	Query::Work m_query;
	Backup::Product::component_type m_archive;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

struct Flavor
{
	Flavor(const QString& home_, const Backup::Product::componentList_type& ve_,
		const Query::Work& query_);

	PRL_RESULT restore(const Assistant& assist_, quint32 version_);
	PRL_RESULT assemble(const QString& home_, Restore::Assembly& dst_);
private:
	QString m_home;
	QList<Image> m_imageList;
};

} // namespace Ploop
} // namespace Target

namespace Source
{

typedef boost::function0<PRL_RESULT> sendFiles_type;

///////////////////////////////////////////////////////////////////////////////
// struct Workerv4

struct Workerv4
{
	typedef boost::function1<int, Task_RestoreVmBackupSource* > exec_type;

	Workerv4(sendFiles_type send_, exec_type exec_) : m_escort(send_), m_exec(exec_) {}

	PRL_RESULT operator()(Task_RestoreVmBackupSource* task_);

private:
	sendFiles_type m_escort;
	exec_type m_exec;
};

///////////////////////////////////////////////////////////////////////////////
// struct Workerv3

struct Workerv3
{
	Workerv3(sendFiles_type send_, Backup::Process::Unit *process_)
		: m_escort(send_), m_process(process_) {}
	~Workerv3();

	PRL_RESULT operator()();

private:
	sendFiles_type m_escort;
	Backup::Process::Unit* m_process;
};

} // namespace Source
} // namespace Restore

#endif // __Task_RestoreVmBackup_p_H_

