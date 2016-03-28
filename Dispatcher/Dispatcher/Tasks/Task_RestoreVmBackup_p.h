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
#include "Task_BackupHelper.h"
#include "prlcommon/Std/noncopyable.h"

class Task_RestoreVmBackupTarget;

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
	Unit(Backup::AClient& impl_, quint32 timeout_, const QString& uuid_, const QString& name_):
		m_name(name_), m_uuid(uuid_), m_timeout(timeout_), m_impl(&impl_)
	{
	}
	Unit(Backup::AClient& impl_, quint32 timeout_, const QString& uuid_, CVmConfiguration& vm_);

	PRL_RESULT operator()(Task_BackupHelper& task_, const QStringList& argv_,
				unsigned disk_) const;
	PRL_RESULT operator()(Task_BackupHelper& task_, const QStringList& argv_,
				SmartPtr<Chain> custom_) const;
private:
	QString m_name;
	QString m_uuid;
	quint32 m_timeout;
	Backup::AClient* m_impl;
};

///////////////////////////////////////////////////////////////////////////////
// struct Api

struct Api
{
	Api(quint32 no_, const QString& backupRoot_);

	QStringList query(const Backup::Archive& archive_) const;
	QStringList restore(const QString& home_) const
	{
		return restore(QFileInfo(home_));
	}
	QStringList restore(const QString& home_, quint32 veid_) const;
	QStringList restore(const Backup::Archive& archive_, const QFileInfo& target_) const;
private:
	QStringList restore(const QFileInfo& target_) const;
	QStringList restore(const QString& archive_, const QFileInfo& target_) const;

	quint32 m_no;
	QDir m_backupRoot;
};

} // namespace AClient

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
	PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_);
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
	PRL_RESULT operator()(const Backup::Archive& archive_, const Assistant& assist_,
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
	PRL_RESULT restore() const;
	PRL_RESULT add(const ::Backup::Archive& archive_);
	Restore::Assembly* assemble(const QString& dst_);
private:
	struct Hdd
	{
		QString final;
		QString mountPoint;
		QFileInfo tib;
		QFileInfo intermediate;
		quint64 sizeOnDisk;
	};
	typedef std::map<int, Hdd> hddMap_type;

	PRL_RESULT make(const QString& path_);
	QStringList make(const char* command_, const Hdd& hdd_) const;

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
	Restore::Assembly* operator()(F flavor_, const QString& home_);
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

///////////////////////////////////////////////////////////////////////////////
// struct Vzwin

struct Vzwin
{
	Vzwin(const QString& home_, const AClient::Api& api_, quint32 veid_);

	PRL_RESULT restore(const Assistant& assist_);
	PRL_RESULT assemble(const QString& home_, Restore::Assembly& dst_);
private:
	QString m_home;
	QStringList m_argv;
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
} // namespace Ploop

namespace Vzfs
{
///////////////////////////////////////////////////////////////////////////////
// struct Reference

struct Reference
{
	Reference(quint32 veid_, const QString& uuid_, const QString& home_,
		const QString& backup_): m_home(home_), m_uuid(uuid_),
		m_veid(veid_), m_backup(backup_)
	{
	}

	QString getHome() const
	{
		return m_home.canonicalPath();
	}
	void setHome(const QString& homePath_)
	{
		m_home.setPath(homePath_);
	}
	QString getCache() const;
	QString getConfig() const;
	QString getQuotaOff() const
	{
		return QString("/usr/sbin/vzquota off %1 -R -p %2")
			.arg(m_veid).arg(getHome("fs"));
	}
	QString getCacheMark() const;
	QString getPrivateMark() const;
private:
	QString getHome(const char* name_) const
	{
		return QFileInfo(m_home, name_).canonicalFilePath();
	}

	QDir m_home;		// m_sTargetPath
	QString m_uuid;		// m_sVmUuid
	quint32 m_veid;
	QString m_backup;	// m_sBackupUuid
};

namespace Assembly
{
struct Work;
///////////////////////////////////////////////////////////////////////////////
// struct Folder

struct Folder
{
	explicit Folder(const QDir& q_): m_q(q_)
	{
	}
	explicit Folder(const QString& path_): m_q(path_)
	{
	}
	explicit Folder(const QFileInfo& info_): m_q(info_.absoluteFilePath())
	{
	}

	QString path() const
	{
		return m_q.absolutePath();
	}
	Work* find(const QString& name_) const;
	PRL_RESULT move(const QString& target_, ::Restore::Toolkit kit_) const;
private:
	QDir m_q;
};

///////////////////////////////////////////////////////////////////////////////
// struct Work

struct Work
{
	Work(const QString& folder_, const QString& name_, const Folder& root_):
		m_root(root_.path()), m_file(QDir(folder_), name_)
	{
	}

	PRL_RESULT operator()(const QString& target_, const Assistant& assist_) const;
private:
	QString m_root;
	QFileInfo m_file;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cache

struct Cache
{
	Cache(const Reference& reference_, Work* work_);

	void join(Restore::Assembly& dst_) const;
	PRL_RESULT do_(const Assistant& assist_);
private:
	QString m_final;
	QString m_intermediate;
	std::auto_ptr<Work> m_work;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(const Reference& reference_);

	PRL_RESULT do_(const Assistant& assist_);
	PRL_RESULT join(const QString& home_, Restore::Assembly& dst_) const;
	void setJoinSrc(const QString& src_)
	{
		m_joinSrc = src_;
	}
private:
	QString m_short;
	QString m_private;
	QString m_joinSrc;
	std::auto_ptr<Work> m_privateWork;
	std::auto_ptr<Cache> m_cache;
};
} // namespace Assembly

namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Primary

struct Primary
{
	Primary(const AClient::Api& api_, const Reference& reference_);

	PRL_RESULT restore(const Assistant& assist_);
	PRL_RESULT assemble();
protected:
	Reference& getReference()
	{
		return m_reference;
	}
	Vzfs::Assembly::Unit& getUnit()
	{
		return *m_vzfs;
	}
private:
	const AClient::Api* m_api;
	Reference m_reference;
	SmartPtr<Vzfs::Assembly::Unit> m_vzfs;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pure

/**
 * Usual flavor aimed to restore VZFS backup.
 */
struct Pure: private Primary
{
	Pure(const AClient::Api& api_, const Reference& reference_);

	PRL_RESULT restore(const Assistant& assist_);
	PRL_RESULT assemble(const QString& home_, Restore::Assembly& dst_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Blended

/**
 * Special flavor aimed to convert restoring VZFS backup to ploop format.
 */
struct Blended: private Primary
{
	Blended(const AClient::Api& api_, const Reference& reference_,
		const SmartPtr<CVmConfiguration>& config_);
	~Blended();

	PRL_RESULT restore(const Assistant& assist_);
	PRL_RESULT assemble(const QString& home_, Restore::Assembly& dst_);
	static bool needConvertToPloop(SmartPtr<CDspClient> client_,
		const QString& sUuid_, const QString& sTargetPath_);
private:
	SmartPtr<Ploop::Device> m_device;
	quint64 m_tempPloopSize;
	QString m_tempPloopPath;
	QString m_originalHome;
};
} // namespace Flavor
} // namespace Vzfs

namespace Ploop
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image
{
	Image(const Backup::Archive& archive_, const Query::Work& query_);
	~Image();

	void join(Restore::Assembly& dst_);
	PRL_RESULT do_(const Assistant& assist_);
private:
	QString m_auto;
	QString m_final;
	QString m_intermediate;
	Query::Work m_query;
	Backup::Archive m_archive;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

struct Flavor
{
	Flavor(const QString& home_, const Backup::Perspective::archiveList_type& ve_,
		const Query::Work& query_);

	PRL_RESULT restore(const Assistant& assist_);
	PRL_RESULT assemble(const QString& home_, Restore::Assembly& dst_);
private:
	QString m_home;
	QList<Image> m_imageList;
};

} // namespace Ploop
} // namespace Target

///////////////////////////////////////////////////////////////////////////////
// struct Converter

struct Converter
{
	void convertHardware(SmartPtr<CVmConfiguration> &cfg) const;
};

} // namespace Restore

#endif // __Task_RestoreVmBackup_p_H_

