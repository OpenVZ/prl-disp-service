///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmBackupInfrastructure_p.h
///
/// Common backup helper class implementation
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

#ifndef __CDspVmBackupInfrastructure_p_H_
#define __CDspVmBackupInfrastructure_p_H_

#include <CDspTaskHelper.h>
#include <CDspVmDirHelper.h>
#include <boost/noncopyable.hpp>
#include <CDspDispConfigGuard.h>
#include <Libraries/Virtuozzo/CVzHelper.h>
//#include <Libraries/VirtualDisk/VirtualDisk.h>
#include <Libraries/PrlCommonUtils/CFileHelper.h>

class IDisk;
class CDspVzHelper;

namespace Backup
{
typedef SmartPtr<CDspClient> actor_type;
typedef SmartPtr<CVmConfiguration> config_type;

namespace Activity
{
namespace Object
{
typedef QPair<QFileInfo, QString> component_type;
typedef QList<component_type> componentList_type;

} // namespace Object
} // namespace Activity

namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct Component

struct Component
{
	Component(const CVmHardDisk& , const QString& , const QString& );

	const CVmHardDisk& getDevice() const
	{
		return *m_device;
	}
	quint64 getDeviceSizeInBytes() const
	{
		return m_device->getSizeInBytes() ?: (m_device->getSize() << 20);
	}
	const QString& getFolder() const
	{
		return m_folder;
	}
	const QString& getImage() const
	{
		return m_image;
	}
	QString getRestorePath() const;

private:
	QString m_folder;
	QString m_image;
	QString m_objectHome;
	const CVmHardDisk* m_device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Model

struct Model
{
	explicit Model(const config_type& config_);

	const config_type& getConfig() const
	{
		return m_config;
	}
	bool isBad() const;
	bool canFreeze() const;
	QList<CVmHardDisk* > getImages() const;
	Model clone(const QString& uuid_, const QString& name_) const;

private:
	config_type m_config;
};

} // namespace Object

namespace Product
{
typedef QPair<Object::Component, QFileInfo> component_type;
typedef QList<component_type> componentList_type;

///////////////////////////////////////////////////////////////////////////////
// struct Model

struct Model
{
	Model(const Object::Model& object_, const QString& home_):
		m_home(home_), m_object(object_), m_suffix(".tib")
	{
	}

	void setStore(const QString& value_)
	{
		m_store = QDir(value_);
	}
	const QDir& getStore() const
	{
		return m_store;
	}
	const Object::Model& getObject() const
	{
		return m_object;
	}
	void setSuffix(const QString& suffix_)
	{
		m_suffix = suffix_;
	}

	componentList_type getCtTibs() const;
	componentList_type getVmTibs() const;

private:
	QString getTibName(const QString& , const QStringList&) const;

	QDir m_store;
	QString m_home;
	Object::Model m_object;
	QString m_suffix;
};

} // namespace Product

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Model

struct Model
{
	const Product::componentList_type& getComponents() const
	{
		return m_components;
	}
	const QString& getUuid() const
	{
		return m_uuid;
	}

protected:
	void setUuid(const QString& value_)
	{
		m_uuid = value_;
	}
	void setComponents(const Product::componentList_type& value_)
	{
		m_components = value_;
	}

private:
	QString m_uuid;
	Product::componentList_type m_components;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mergable

struct Mergable: Model
{
	virtual ~Mergable()
	{
	}

	virtual PRL_RESULT merge() = 0;
	virtual PRL_RESULT destroy() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Shedable

struct Shedable: Mergable
{
	virtual PRL_RESULT dropState(const QDir& store_) = 0;
};

namespace Vm
{
struct Object;
struct Subject;

///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image
{
	PRL_RESULT open(const QString& path_);
	PRL_RESULT close();
	PRL_RESULT hasSnapshot(const QString& uuid_);
	PRL_RESULT getBackupSnapshotId(QString& dst_);
	PRL_RESULT dropState(const Uuid& uuid_, const QString& target_);

private:
	static void release(IDisk* value_);

	QString m_path;
	SmartPtr<IDisk> m_image;
};

} // namespace Vm

namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image
{
        PRL_RESULT operator()(const QString& snapshot_,
			const Product::component_type& tib_, const QDir& store_,
			QString& dst_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Mount

struct Mount
{
	Mount(const char* component_, const CVzOperationHelper& core_):
		m_component(component_), m_core(core_)
	{
	}

	PRL_RESULT operator()(const QString& snapshot_,
			const Product::component_type& tib_, const QDir& store_,
			QString& dst_);

private:
	const char* m_component;
	CVzOperationHelper m_core;
};

///////////////////////////////////////////////////////////////////////////////
// struct Complete

template<class T>
struct Complete
{
	typedef Product::componentList_type list_type;

	Complete(const QString& snapshot_, T primary_):
		m_snapshot(snapshot_), m_primary(primary_)
	{
	}

	PRL_RESULT operator()(const list_type& tibList_, const QDir& store_, list_type& dst_)
	{
		PRL_RESULT output = PRL_ERR_SUCCESS;
		foreach (const Product::component_type& a, tibList_)
		{
			QString x;
			output = m_primary(m_snapshot, a, store_, x);
			if (PRL_FAILED(output))
				break;

			dst_ << qMakePair(a.first, QFileInfo(x));
		}
		return output;
	}

private:
	QString m_snapshot;
	T m_primary;
};

} // namespace Export
} // namespace Snapshot

namespace Task
{
struct Comment;
///////////////////////////////////////////////////////////////////////////////
// struct Reporter

struct Reporter
{
	Reporter(CDspTaskHelper& task_, const QString& uuid_);

	void nameObject(const QString& value_)
	{
		m_object = value_.isEmpty() ? m_uuid : value_;
	}
	PRL_RESULT warn(PRL_RESULT code_);
	PRL_RESULT fail(const CVmEvent& event_)
	{
		return CDspTaskFailure(*m_task)(event_);
	}
	PRL_RESULT fail(PRL_RESULT code_, const QString& text_)
	{
		return CDspTaskFailure(*m_task)(code_, text_);
	}
	PRL_RESULT fail(PRL_RESULT code_, const Comment& comment_);

private:
	QString m_uuid;
	QString m_object;
	CDspTaskHelper* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Object

struct Object
{
	Object(const CVmIdent& ident_, const actor_type& actor_, CDspVmDirHelper& directory_):
		m_ident(ident_), m_actor(actor_), m_directory(&directory_)
	{
	}

	const CVmIdent& getIdent() const
	{
		return m_ident;
	}
	PRL_RESULT lock(const QString& task_);
	PRL_RESULT unlock();

protected:
	const actor_type& getActor() const
	{
		return m_actor;
	}
	CDspVmDirHelper& getDirectory() const
	{
		return *m_directory;
	}

private:
	CVmIdent m_ident;
	actor_type m_actor;
	CDspVmDirHelper* m_directory;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reference

struct Reference
{
	Reference()
	{
	}
	explicit Reference(const QString& backupUuid_): m_backupUuid(backupUuid_)
	{
	}

	const QString& getHome() const
	{
		return m_home;
	}
	const QString& getName() const
	{
		return m_name;
	}
	const QString& getStore() const
	{
		return m_store;
	}
	const QString& getBackupUuid() const
	{
		return m_backupUuid;
	}
	const config_type& getConfig() const
	{
		return m_config;
	}

protected:
	void update(const QString& , const QString& , const config_type& );

private:
	QString m_name;
	QString m_home;
	QString m_store;
	QString m_backupUuid;
	config_type m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Workbench

struct Workbench
{
	Workbench(CDspTaskHelper& dspTask_, Reporter& reporter_,
		CDspDispConfigGuard& dspConfig_);

	Reporter& getReporter() const
	{
		return *m_reporter;
	}
	CDspTaskHelper& getDspTask() const
	{
		return *m_dspTask;
	}
	PRL_RESULT openTmp(QString& dst_);

private:
	QString m_tmp;
	Reporter* m_reporter;
	CDspTaskHelper* m_dspTask;
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

template<class T>
struct Subject: Workbench
{
	Subject(const T& flavor_, const Workbench& workbench_):
		Workbench(workbench_), m_flavor(flavor_)
	{
	}

	PRL_RESULT getActivity(typename T::object_type ve_,
			QScopedPointer<typename T::activity_type>& dst_)
	{
		QString x = m_flavor.getReference().getStore();
		if (x.isEmpty())
			return PRL_ERR_UNINITIALIZED;

		CAuthHelper& a = getDspTask().getClient()->getAuthHelper();
		if (CFileHelper::DirectoryExists(x, &a))
		{
			WRITE_TRACE(DBG_FATAL, "Directory \"%s\" already exists", QSTR2UTF8(x));
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
		if (!CFileHelper::CreateDirectoryPath(x, &a))
		{
			WRITE_TRACE(DBG_FATAL, "Can't create directory \"%s\"", QSTR2UTF8(x));
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
		PRL_RESULT e;
		if (getDspTask().operationIsCancelled())
		{
			e = getDspTask().getCancelResult();
			goto error;
		}
		if (PRL_FAILED(e = ve_.lock(getDspTask().getJobUuid())))
			goto error;

		dst_.reset(m_flavor.craftActivity(ve_, *this));
		return PRL_ERR_SUCCESS;

	error:
		// NB. x is not empty here.
		CFileHelper::ClearAndDeleteDir(x);
		return e;
	}
	PRL_RESULT getSnapshot(typename T::object_type const& ve_,
			QScopedPointer<typename T::snapshot_type>& dst_)
	{
		PRL_RESULT e = m_flavor.update(ve_, *this);
		if (PRL_FAILED(e))
			return e;

		dst_.reset(m_flavor.craftSnapshot(ve_));
		return PRL_ERR_SUCCESS;
	}

private:
	T m_flavor;
};

namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// struct Reference

struct Reference: Task::Reference
{
	Reference(const QString& backupUuid_, const CVzOperationHelper& operations_):
		Task::Reference(backupUuid_), m_operations(operations_)
	{
	}

	const CVzOperationHelper& getOperations() const
	{
		return m_operations;
	}
	PRL_RESULT update(const config_type& config_);

private:
	CVzOperationHelper m_operations;
};

///////////////////////////////////////////////////////////////////////////////
// struct Object

struct Object: Task::Object
{
	Object(const Task::Object& core_, const SmartPtr<CDspVzHelper>& vz_):
		Task::Object(core_), m_vz(vz_)
	{
	}

	PRL_RESULT getConfig(config_type& dst_) const;

private:
	SmartPtr<CDspVzHelper> m_vz;
};

} // namespace Ct

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Object

struct Object: Task::Object
{
	Object(const CVmIdent& ident_, const actor_type& actor_, CDspVmDirHelper& directory_):
		Task::Object(ident_, actor_, directory_)
	{
	}

	PRL_RESULT getConfig(config_type& dst_) const;
	PRL_RESULT getDirectoryItem(CVmDirectoryItem& dst_) const;
	void getSnapshot(QScopedPointer<Snapshot::Vm::Object>& dst_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reference

struct Reference: Task::Reference
{
	explicit Reference(Reporter& reporter_): m_reporter(&reporter_)
	{
	}
	Reference(const QString& backupUuid_, Reporter& reporter_):
		Task::Reference(backupUuid_), m_reporter(&reporter_)
	{
	}

	PRL_RESULT update(const CVmDirectoryItem& item_, const config_type& config_);

private:
	PRL_RESULT validate(const CVmConfiguration& tentative_);

	Reporter* m_reporter;
};

} // namespace Vm
} // namespace Task

namespace Escort
{
///////////////////////////////////////////////////////////////////////////////
// struct Model

struct Model
{
	const Activity::Object::componentList_type& getFiles() const
	{
		return m_files;
	}
	const Activity::Object::componentList_type& getFolders() const
	{
		return m_folders;
	}

protected:
	void setFiles(const Activity::Object::componentList_type& value_)
	{
		m_files = value_;
	}
	void setFolders(const Activity::Object::componentList_type& value_)
	{
		m_folders = value_;
	}

private:
	Activity::Object::componentList_type m_files;
	Activity::Object::componentList_type m_folders;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct: Model
{
	Ct(const config_type& config_, const CVzOperationHelper& vz_):
		m_config(config_), m_vz(vz_)
	{
	}

	void collect(const QDir& home_);
	PRL_RESULT extract(const QDir& store_);

private:
	config_type m_config;
	CVzOperationHelper m_vz;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: Model
{
	explicit Vm(const config_type& config_): m_config(config_)
	{
	}

	void collect(const QDir& home_);
	PRL_RESULT extract(const QDir& store_);

private:
	config_type m_config;
};

} // namespace Escort

namespace Activity
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder
{
	Builder(const CVmIdent& ident_, CDspTaskHelper& task_):
		m_ident(ident_), m_task(&task_), m_reporter(task_, ident_.first)
	{
	}

	const CVmIdent& getIdent() const
	{
		return m_ident;
	}
	CDspTaskHelper& getTask() const
	{
		return *m_task;
	}
	void setSubject(CDspDispConfigGuard& configGuard_)
	{
		m_workbench.reset(new Task::Workbench(*m_task, m_reporter, configGuard_));
	}

protected:
	Task::Reporter& getReporter()
	{
		return m_reporter;
	}
	Task::Workbench* getWorkbench() const
	{
		return m_workbench.data();
	}

private:
	CVmIdent m_ident;
	CDspTaskHelper* m_task;
	Task::Reporter m_reporter;
	QScopedPointer<Task::Workbench> m_workbench;
};

namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct Model

struct Model
{
	QString toString() const;
	const QString& getUuid() const
	{
		return m_uuid;
	}
	const Escort::Model& getEscort() const
	{
		return m_escort;
	}
	const Snapshot::Model& getSnapshot() const
	{
		return m_snapshot;
	}

protected:
	void setUuid(const QString& value_)
	{
		m_uuid = value_;
	}
	void setEscort(const Escort::Model& value_)
	{
		m_escort = value_;
	}
	void setSnapshot(const Snapshot::Model& value_)
	{
		m_snapshot = value_;
	}

private:
	QString m_uuid;
	Escort::Model m_escort;
	Snapshot::Model m_snapshot;
};

} // namespace Object

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<class T, class U>
struct Flavor: boost::noncopyable
{
	typedef U escort_type;
	typedef T snapshot_type;

	Flavor(const QString& home_, const QString& store_, const U& prototype_):
		m_home(home_), m_store(store_), m_prototype(prototype_)
	{
	}
	~Flavor()
	{
		if (!m_store.isEmpty())
			CFileHelper::ClearAndDeleteDir(m_store);
	}

	PRL_RESULT operator()(snapshot_type& value_) const
	{
		return value_.dropState(m_store);
	}
	PRL_RESULT operator()(QScopedPointer<escort_type>& dst_) const
	{
		QScopedPointer<escort_type> s(new escort_type(m_prototype));
		PRL_RESULT e = s->extract(m_store);
		if (PRL_FAILED(e))
			return e;

		s->collect(m_home);
		dst_.reset(s.take());
		return PRL_ERR_SUCCESS;
	}

private:
	QString m_home;
	QString m_store;
	U m_prototype;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T, class U>
struct Unit: Activity::Object::Model
{
	typedef T flavor_type;

	// NB. T must be an object type because we use m_flavor the
	// object way.
	Unit(T* flavor_, const actor_type& actor_, const U& object_):
		m_object(object_), m_actor(actor_), m_flavor(flavor_), m_final()
	{
	}
	~Unit()
	{
		if (!m_snapshot.isNull())
			(m_snapshot.data()->*m_final)();

		m_object.unlock();
	}

	const actor_type& getActor() const
	{
		return m_actor;
	}
	PRL_RESULT start(typename T::snapshot_type* snapshot_)
	{
                QScopedPointer<Snapshot::Mergable> x(snapshot_);
                if (x.isNull())
                        return PRL_ERR_INVALID_ARG;

		if (!m_snapshot.isNull())
		{
			x->destroy();
			return PRL_ERR_DOUBLE_INIT;
		}
		m_snapshot.reset(x.take());
		m_final = &Snapshot::Mergable::destroy;
		PRL_RESULT e = (*m_flavor)(*snapshot_);
		if (PRL_FAILED(e))
			return e;

		e = (*m_flavor)(m_escort);
		if (PRL_FAILED(e))
			return e;

		if (m_escort.isNull())
			return PRL_ERR_UNEXPECTED;

		m_final = &Snapshot::Mergable::merge;
		Activity::Object::Model::setEscort(*m_escort);
		Activity::Object::Model::setSnapshot(*m_snapshot);
		return PRL_ERR_SUCCESS;
	}
	void abort()
	{
		if (!m_snapshot.isNull())
			m_final = &Snapshot::Mergable::destroy;
	}
	using Activity::Object::Model::setUuid;

private:
	U m_object;
	actor_type m_actor;
	QScopedPointer<T> m_flavor;
	PRL_RESULT (Snapshot::Mergable::* m_final)();
	QScopedPointer<Snapshot::Mergable> m_snapshot;
	QScopedPointer<typename T::escort_type> m_escort;
};

namespace Vm
{
struct Unit;
} // namespace Vm
} // namespace Activity

namespace Snapshot
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Object

struct Object
{
	Object(const QString& , const actor_type& );

	PRL_RESULT begin(const QString& path_, const QString& map_, Task::Reporter& reporter_);
	PRL_RESULT thaw();
	PRL_RESULT commit(const QString& uuid_);
	PRL_RESULT rollback();
	PRL_RESULT freeze(Task::Workbench& task_);

private:
	PRL_RESULT disband(quint32 flags_);

	QString m_uuid;
	actor_type m_actor;
};

///////////////////////////////////////////////////////////////////////////////
// struct Begin

struct Begin
{
	Begin(const QString& tmp_, const QString& map_, Task::Workbench& task_);

	PRL_RESULT doTrivial(Object& object_);
	PRL_RESULT doConsistent(Object& object_);

private:
	QString m_tmp;
	QString m_map;
	Task::Workbench* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Commit

struct Commit
{
	explicit Commit(const QString& uuid_): m_uuid(uuid_)
	{
	}

	PRL_RESULT operator()(Object& object_)
	{
		return object_.commit(m_uuid);
	}

private:
	QString m_uuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Rollback

struct Rollback
{
	PRL_RESULT operator()(Object& object_)
	{
		return object_.rollback();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject: Shedable
{
	Subject(const Task::Vm::Object& vm_, const Task::Vm::Reference& reference_);

	PRL_RESULT merge();
	PRL_RESULT attach();
	PRL_RESULT destroy();
	PRL_RESULT create(Task::Workbench& task_);
	PRL_RESULT dropState(const QDir& store_);

private:
	template<class T>
	PRL_RESULT disband(T command_);

	QString m_map;
	QString m_tmp;
	Task::Vm::Object m_vm;
	Product::Model m_product;
	Product::componentList_type m_tibList;
};

} // namespace Vm

namespace Ct
{
namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Sketch

struct Sketch: Model
{
	Sketch(const QString& ct_, const CVzOperationHelper& core_):
		m_ct(ct_), m_core(core_)
	{
	}

	const QString& getMap() const
	{
		return m_map;
	}

protected:
	static const char s_component[];

	PRL_RESULT open(const QString& uuid_, const QString& tmp_);
	PRL_RESULT close(bool flavor_);
	CVzOperationHelper& getCore()
	{
		return m_core;
	}
	void setMap(const QString& value_)
	{
		m_map = value_;
	}

private:
	QString m_ct;
	QString m_map;
	CVzOperationHelper m_core;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pure

template<class T>
struct Pure: Sketch
{
	Pure(const QString& ct_, T primary_, const CVzOperationHelper& core_):
		Sketch(ct_, core_), m_primary(primary_)
	{
	}

	PRL_RESULT export_(const Product::componentList_type& tibList_, const QDir& store_)
	{
		if (getUuid().isEmpty())
			return PRL_ERR_UNINITIALIZED;

		Product::componentList_type c;
		PRL_RESULT output = Export::Complete<T>(getUuid(), m_primary)
					(tibList_, store_, c);
		setComponents(c);
		return output;
	}

private:
	T m_primary;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mount

struct Mount: Pure<Export::Mount>
{
	Mount(const QString& ct_, const CVzOperationHelper& core_);

	PRL_RESULT commit();
	PRL_RESULT rollback()
	{
		return commit();
	}
	PRL_RESULT begin(const QString& tmp_);
	PRL_RESULT export_(const Product::componentList_type& tibList_, const QDir& store_);

private:
	void clean();
};

///////////////////////////////////////////////////////////////////////////////
// struct Mountv4

struct Mountv4: Pure<Export::Mount>
{
	Mountv4(const QString& ct_, const CVzOperationHelper& core_);

	PRL_RESULT commit();
	PRL_RESULT rollback()
	{
		return commit();
	}
	PRL_RESULT export_(const Product::componentList_type& tibList_, const QDir& store_);

private:
	void clean();

	QStringList m_mounts;
};

///////////////////////////////////////////////////////////////////////////////
// struct Bitmap

struct Bitmap: Pure<Export::Image>
{
	Bitmap(const QString& ct_, const QString& uuid_, const CVzOperationHelper& core_);

	PRL_RESULT commit()
	{
		return close(false);
	}
	PRL_RESULT rollback()
	{
		return close(!getMap().isEmpty());
	}
	PRL_RESULT begin(const QString& tmp_);
};

} // namespace Flavor

///////////////////////////////////////////////////////////////////////////////
// struct Subject

template<class T>
struct Subject: Shedable
{
	Subject(T flavor_, const Task::Ct::Reference& reference_):
		m_flavor(flavor_),
		m_product(Backup::Object::Model(reference_.getConfig()),
			reference_.getHome())
	{
	}

	PRL_RESULT merge()
	{
		PRL_RESULT output = m_flavor.commit();
		close();
		return output;
	}
	PRL_RESULT destroy()
	{
		PRL_RESULT output = m_flavor.rollback();
		close();
		return output;
	}
	PRL_RESULT create(Task::Workbench& task_)
	{
		if (!m_flavor.getUuid().isEmpty())
			return PRL_ERR_DOUBLE_INIT;

		PRL_RESULT e;
		if (PRL_FAILED(e = task_.openTmp(m_tmp)))
			return e;

		if (PRL_FAILED(e = m_flavor.begin(m_tmp)))
		{
			close();
			return e;
		}
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT dropState(const QDir& store_)
	{
		if (!getUuid().isEmpty())
			return PRL_ERR_DOUBLE_INIT;

		PRL_RESULT e = m_flavor.export_(m_product.getCtTibs(), store_);
		if (PRL_FAILED(e))
			return e;

		setUuid(m_flavor.getUuid());
		setComponents(m_flavor.getComponents());
		return PRL_ERR_SUCCESS;
	}

private:
	void close()
	{
		if (!m_tmp.isEmpty())
		{
			CFileHelper::ClearAndDeleteDir(m_tmp);
			m_tmp.clear();
		}
		setUuid(QString());
		setComponents(Product::componentList_type());
	}

	T m_flavor;
	QString m_tmp;
	Product::Model m_product;
};

} // namespace Ct
} // namespace Snapshot

namespace Activity
{
namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// Unit

typedef Activity::Unit<Activity::Flavor<Snapshot::Shedable, Escort::Ct>,
		Task::Ct::Object> Unit;
typedef std::map<CVmIdent, QSharedPointer<Unit> > map_type;

} // namespace Ct

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// class Unit

class Unit: private Activity::Unit<Activity::Flavor<Snapshot::Vm::Subject, Escort::Vm>,
			Task::Vm::Object>
{
	typedef Activity::Unit<Activity::Flavor<Snapshot::Vm::Subject, Escort::Vm>,
			Task::Vm::Object> base_type;

public:
	Unit(const Task::Vm::Object& , const Task::Vm::Reference& , const actor_type& );

	const Activity::Object::Model& getModel() const
	{
		return *this;
	}
	PRL_RESULT start(Snapshot::Vm::Subject* snapshot_);
	using base_type::abort;
	using base_type::getActor;

private:
	config_type m_config;
};

typedef std::map<CVmIdent, QSharedPointer<Vm::Unit> > map_type;

} // namespace Vm

namespace Store
{
typedef QMultiHash<IOSender::Handle, CVmIdent> tracker_type;

///////////////////////////////////////////////////////////////////////////////
// struct Extract

struct Extract
{
	explicit Extract(Object::Model& dst_): m_dst(&dst_)
	{
	}

	void operator()(Ct::map_type::const_reference ct_)
	{
		*m_dst = *ct_.second;
	}
	void operator()(Vm::map_type::const_reference vm_)
	{
		*m_dst = vm_.second->getModel();
	}

private:
	Object::Model* m_dst;
};

///////////////////////////////////////////////////////////////////////////////
// struct Fetch

struct Fetch
{
	explicit Fetch(const IOSender::Handle& actor_): m_actor(actor_)
	{
	}

	const IOSender::Handle& getActor() const
	{
		return m_actor;
	}
	const Ct::map_type::mapped_type& getCt() const
	{
		return m_ct;
	}
	const Vm::map_type::mapped_type& getVm() const
	{
		return m_vm;
	}
	bool operator()(Ct::map_type::const_reference ct_);
	bool operator()(Vm::map_type::const_reference vm_);

private:
	IOSender::Handle m_actor;
	Ct::map_type::mapped_type m_ct;
	Vm::map_type::mapped_type m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Finish

struct Finish: private Fetch
{
	Finish(const actor_type& actor_, tracker_type& tracker_):
		Fetch(actor_->getClientHandle()), m_tracker(&tracker_)
	{
	}

	template<class T>
	bool operator()(T t_)
	{
		if (Fetch::operator()(t_))
		{
			m_tracker->remove(getActor(), t_.first);
			return true;
		}
		return false;
	}
	bool getResult() const
	{
		return !(getCt().isNull() && getVm().isNull());
	}

private:
	tracker_type* m_tracker;
};

///////////////////////////////////////////////////////////////////////////////
// struct Track

struct Track
{
	Track(const actor_type& actor_, tracker_type& tracker_):
		m_actor(actor_), m_result(PRL_ERR_FILE_NOT_FOUND),
		m_tracker(&tracker_)
	{
	}

	PRL_RESULT getResult() const
	{
		return m_result;
	}
	template<class T>
	void operator()(T t_)
	{
		if (m_actor != t_.second->getActor())
		{
			m_result = PRL_ERR_ACCESS_DENIED;
			return;
		}
		IOSender::Handle h = m_actor->getClientHandle();
		if (m_tracker->contains(h, t_.first))
			m_result = PRL_ERR_DOUBLE_INIT;
		else
		{
			m_tracker->insert(h, t_.first);
			m_result = PRL_ERR_SUCCESS;
		}
	}

private:
	actor_type m_actor;
	PRL_RESULT m_result;
	tracker_type* m_tracker;
};

///////////////////////////////////////////////////////////////////////////////
// struct Abort

struct Abort: private Fetch
{
	explicit Abort(const IOSender::Handle& actor_): Fetch(actor_)
	{
	}

	void operator()() const;
	using Fetch::operator();
};

} // namespace Store
} // namespace Activity

namespace Task
{
namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// struct Sketch

struct Sketch
{
	typedef Object object_type;
	typedef Activity::Ct::Unit activity_type;

	explicit Sketch(Reference& reference_): m_reference(&reference_)
	{
	}

	const Reference& getReference() const
	{
		return *m_reference;
	}
	Escort::Ct craftEscort() const;
	PRL_RESULT update(const object_type& , const Workbench& );
	activity_type* craftActivity(const object_type& ct_,
			const Workbench& workbench_) const;

private:
	Reference* m_reference;
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

template<class T>
struct Subject: Sketch
{
	typedef Snapshot::Ct::Subject<T> snapshot_type;

	Subject(T flavor_, Reference& reference_):
		Sketch(reference_), m_flavor(flavor_)
	{
	}

	snapshot_type* craftSnapshot(const object_type& ) const
	{
		return new snapshot_type(m_flavor, getReference());
	}

private:
	T m_flavor;
};

} // namespace Ct

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject
{
	typedef Object object_type;
	typedef Activity::Vm::Unit activity_type;
	typedef Snapshot::Vm::Subject snapshot_type;

	explicit Subject(Reference& reference_): m_reference(&reference_)
	{
	}

	const Reference& getReference() const
	{
		return *m_reference;
	}
	PRL_RESULT update(const object_type& , const Workbench& );
	activity_type* craftActivity(const object_type& , const Workbench& ) const;
	snapshot_type* craftSnapshot(const object_type& ) const;

private:
	Reference* m_reference;
};

} // namespace Vm
} // namespace Task
} // namespace Backup

#endif // __CDspVmBackupInfrastructure_p_H_

