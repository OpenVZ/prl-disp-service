///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTemplateStorage.cpp
///
/// Definitions of templates storage infrastructure
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

#include <mntent.h>
#include <sys/file.h>
#include "CDspService.h"
#include "CDspVzHelper.h"
#include "CDspTemplateStorage.h"
#include <prlcommon/HostUtils/PCSUtils.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Facade

struct Facade
{
	explicit Facade(CAuthHelper& auth_): m_auth(&auth_)
	{
	}

	PRL_RESULT mkdir(const QDir& dir_) const;
	PRL_RESULT unlink(const QFileInfo& path_) const;
	PRL_RESULT unlink(const QDir& dir_) const
	{
		return unlink(QFileInfo(dir_, QString()));
	}
	PRL_RESULT rename(const QFileInfo& from_, const QFileInfo& to_) const;
private:
	CAuthHelper* m_auth;
};

PRL_RESULT Facade::mkdir(const QDir& dir_) const
{
	QString p = dir_.absolutePath();
	if (CFileHelper::WriteDirectory(p, m_auth))
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Cannot create directory %s", QSTR2UTF8(p));
	return PRL_ERR_MAKE_DIRECTORY;
}

PRL_RESULT Facade::unlink(const QFileInfo& path_) const
{
	QString p = path_.filePath();
	if (path_.isDir())
	{
		if (!CFileHelper::ClearAndDeleteDir(p))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot remove directory %s", QSTR2UTF8(p));
			return PRL_ERR_CANT_REMOVE_ENTRY;
		}
	}
	else if (path_.exists())
	{
		if (!CFileHelper::RemoveEntry(p, m_auth))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot remove a non-directory item %s", QSTR2UTF8(p));
			return PRL_ERR_CANT_REMOVE_ENTRY;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Facade::rename(const QFileInfo& from_, const QFileInfo& to_) const
{
	QString s = from_.filePath(), t = to_.filePath();
	if (CFileHelper::RenameEntry(s, t, m_auth))
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Cannot rename %s -> %s", QSTR2UTF8(s), QSTR2UTF8(t));
	return PRL_ERR_CANT_RENAME_ENTRY;
}

} // namespace

namespace Template
{
namespace Storage
{
namespace Lock
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(const QFileInfo& path_, QFile::OpenMode mode_):
	m_file(path_.filePath()), m_mode(mode_)
{
}

PRL_RESULT Unit::enter()
{
	if (m_file.isOpen())
		return PRL_ERR_DOUBLE_INIT;
	if (m_file.open(m_mode))
	{
		int m = m_mode & QIODevice::WriteOnly ? LOCK_EX : LOCK_SH;
		if (0 == TEMP_FAILURE_RETRY(::flock(m_file.handle(), m | LOCK_NB)))
			return PRL_ERR_SUCCESS;

		m_file.close();
	}

	return PRL_ERR_OPEN_FAILED;
}

///////////////////////////////////////////////////////////////////////////////
// struct Sentinel

Sentinel::result_type Sentinel::getForRead()
{
	result_type output = m_read;
	if (!output.isNull())
		return output;

	QScopedPointer<Unit> x(new Unit(m_path, QFile::ReadOnly));
	PRL_RESULT e = x->enter();
	if (PRL_SUCCEEDED(e))
	{
		output = result_type(x.take());
		m_read = output.toWeakRef();
	}
	return output;
}

Sentinel::result_type Sentinel::getForWrite()
{
	if (!m_read.isNull())
		return result_type();

	result_type output = m_write;
	if (!output.isNull())
		return output;

	QScopedPointer<Unit> x(new Unit(m_path, QFile::WriteOnly));
	PRL_RESULT e = x->enter();
	if (PRL_SUCCEEDED(e))
	{
		output = result_type(x.take());
		m_write = output.toWeakRef();
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Catalog

Catalog::Catalog(const QDir& root_):
	m_fast(root_.filePath(".fast")),
	m_slow(root_.filePath(".slow"))
{
}

} // namespace Lock

///////////////////////////////////////////////////////////////////////////////
// struct Sandbox

struct Sandbox: private Facade
{
	Sandbox(const QFileInfo& path_, CAuthHelper& auth_):
		Facade(auth_), m_path(path_)
	{
	}
	~Sandbox()
	{
		clear();
	}

	bool isClean()
	{
		return !m_path.exists();
	}
	QString getPath() const
	{
		return m_path.filePath();
	}
	PRL_RESULT clear()
	{
		return this->unlink(m_path);
	}
	PRL_RESULT draw(const QFileInfo& place_)
	{
		return this->rename(place_, m_path);
	}
	PRL_RESULT push(const QFileInfo& place_)
	{
		return this->rename(m_path, place_);
	}
private:
	QFileInfo m_path;
};

namespace Entry
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

QSharedPointer<const Lock::Unit> Unit::getLock()
{
	if (NULL == m_locking)
		return QSharedPointer<const Lock::Unit>();

	return m_locking->getWFast();
}

SmartPtr<CVmConfiguration> Unit::getConfig() const
{
	QSharedPointer<const Lock::Unit> g = m_guard;
	if (g.isNull() && m_locking != NULL)
	{
		g = m_locking->getRFast();
		if (g.isNull())
			return SmartPtr<CVmConfiguration>();
	}

	QFileInfo p(QDir(m_root.absoluteFilePath()), VMDIR_DEFAULT_VM_CONFIG_FILE);
	if (p.exists())
	{
		QScopedPointer<CVmConfiguration> x(new CVmConfiguration);
		QFile f(p.filePath());
		PRL_RESULT e = x->loadFromFile(&f, false);
		if (PRL_SUCCEEDED(e))
			return SmartPtr<CVmConfiguration>(x.take());

		return SmartPtr<CVmConfiguration>();
	}

	p = QFileInfo(QDir(m_root.absoluteFilePath()), "ve.conf");
	if (p.exists())
	{
		int e;

		return CVzHelper::get_env_config_from_file(
				p.absoluteFilePath(), e,
				Ct::Config::LoadOps().setUnregistered());
	}

	return SmartPtr<CVmConfiguration>();
}

namespace Action
{
///////////////////////////////////////////////////////////////////////////////
// struct Abstract

Abstract::~Abstract()
{
}

} // namespace Action

///////////////////////////////////////////////////////////////////////////////
// struct Unlink

struct Unlink: Action::Both
{
	PRL_RESULT prepare(const Batch::Mode::Both& batch_);
	PRL_RESULT do_(Unit& object_);
	PRL_RESULT undo(Unit& object_);
private:
	std::auto_ptr<Sandbox> m_backup;
};

PRL_RESULT Unlink::prepare(const Batch::Mode::Both& batch_)
{
	if (NULL != m_backup.get())
		return PRL_ERR_DOUBLE_INIT;

	return batch_.getSandbox("b", m_backup);
}

PRL_RESULT Unlink::do_(Unit& object_)
{
	if (NULL == m_backup.get())
		return PRL_ERR_UNINITIALIZED;

	QSharedPointer<const Lock::Unit> w(object_.getLock());
	if (w.isNull())
		return PRL_ERR_FAILURE;

	if (!object_.getRoot().exists())
		return PRL_ERR_SUCCESS;

	return m_backup->draw(object_.getRoot());
}

PRL_RESULT Unlink::undo(Unit& object_)
{
	if (NULL == m_backup.get())
		return PRL_ERR_UNINITIALIZED;

	if (m_backup->isClean())
		return PRL_ERR_SUCCESS;

	QSharedPointer<const Lock::Unit> w(object_.getLock());
	if (w.isNull())
		return PRL_ERR_FAILURE;

	return m_backup->push(object_.getRoot());
}

///////////////////////////////////////////////////////////////////////////////
// struct Import

PRL_RESULT Import::prepare(const Batch::Mode::Both& batch_)
{
	if (NULL != m_workspace.get())
		return PRL_ERR_DOUBLE_INIT;

	return batch_.getSandbox("w", m_workspace);
}

PRL_RESULT Import::do_(Unit& object_)
{
	if (m_content.empty())
		return PRL_ERR_UNINITIALIZED;

	if (NULL == m_workspace.get())
		return PRL_ERR_UNINITIALIZED;

	PRL_RESULT e = m_content(m_workspace->getPath());
	if (PRL_FAILED(e))
		return e;

	QSharedPointer<const Lock::Unit> w(object_.getLock());
	if (w.isNull())
		return PRL_ERR_FAILURE;

	return m_workspace->push(object_.getRoot());
}

PRL_RESULT Import::undo(Unit& object_)
{
	Q_UNUSED(object_);
	if (NULL == m_workspace.get())
		return PRL_ERR_UNINITIALIZED;

	return m_workspace->clear();
}

///////////////////////////////////////////////////////////////////////////////
// struct Export

PRL_RESULT Export::do_(Unit& object_)
{
	if (m_content.empty())
		return PRL_ERR_UNINITIALIZED;

	return m_content(object_.getRoot().absoluteFilePath());
}

} // namespace Entry

namespace Batch
{
namespace Mode
{
///////////////////////////////////////////////////////////////////////////////
// struct Abstract

PRL_RESULT Abstract::operator()()
{
	PRL_RESULT output = m_log.execute();
	m_log = ::Instrument::Command::Batch();

	return output;
}

PRL_RESULT Abstract::add(const Entry::Unit& entry_, const readAction_type& action_)
{
	if (NULL == action_.get())
		return PRL_ERR_INVALID_ARG;

	m_log.addItem(boost::bind(&Entry::Action::Read::do_, action_, entry_));

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Abstract::add(const Entry::Unit& entry_, const bothAction_type& action_)
{
	if (NULL == action_.get())
		return PRL_ERR_INVALID_ARG;

	m_log.addItem(boost::bind(&Entry::Action::Both::do_, action_, entry_),
		boost::bind(&Entry::Action::Both::undo, action_, entry_));

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Both

Both::Both(const QDir& root_, const guard_type& guard_):
	Abstract(guard_), m_root(root_.filePath("sandbox")), m_stage(), m_auth()
{
}

PRL_RESULT Both::operator()()
{
	PRL_RESULT output = Abstract::operator()();
	m_stage = 0;

	return output;
}

PRL_RESULT Both::add(const Entry::Unit& entry_, const bothAction_type& action_)
{
	if (NULL == action_.get())
		return PRL_ERR_INVALID_ARG;

	if (NULL == m_auth)
		return PRL_ERR_UNINITIALIZED;

	PRL_RESULT e = action_->prepare(*this);
	if (PRL_FAILED(e))
		return e;

	e = Abstract::add(entry_, action_);
	if (PRL_FAILED(e))
		return e;

	++m_stage;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Both::add(const Entry::Unit& entry_, const readAction_type& action_)
{
	PRL_RESULT e = Abstract::add(entry_, action_);
	if (PRL_FAILED(e))
		return e;

	++m_stage;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Both::prepare(CAuthHelper& auth_)
{
	if (NULL != m_auth)
		return PRL_ERR_DOUBLE_INIT;

	PRL_RESULT e;
	Facade f(auth_);
	if (PRL_RESULT(e = f.unlink(m_root)))
		return e;
	if (PRL_RESULT(e = f.mkdir(m_root)))
		return e;

	m_auth = &auth_;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Both::getSandbox(const char* prefix_, std::auto_ptr<Sandbox>& dst_) const
{
	if (NULL == m_auth)
		return PRL_ERR_UNINITIALIZED;

	if (NULL == prefix_ || prefix_[0] == 0)
		return PRL_ERR_INVALID_ARG;

	QFileInfo p(m_root, QString("%1%2").arg(prefix_).arg(m_stage));
	dst_.reset(new Sandbox(p, *m_auth));
	PRL_RESULT output = dst_->clear();
	if (PRL_SUCCEEDED(output))
		return PRL_ERR_SUCCESS;

	dst_.reset();
	return output;
}

} // namespace Mode

///////////////////////////////////////////////////////////////////////////////
// struct Factory

Entry::Unit Factory::craftObject(const QString& name_) const
{
	return Entry::Unit(QFileInfo(m_root, name_), *m_locking);
}

Prl::Expected<Mode::Read, PRL_RESULT> Factory::craftForRead() const
{
	Lock::Catalog::result_type r(m_locking->getRSlow());
	if (r.isNull())
		return PRL_ERR_FAILURE;

	return Mode::Read(r);
}

Prl::Expected<Mode::Both, PRL_RESULT> Factory::craftForBoth(CAuthHelper& auth_) const
{
	Lock::Catalog::result_type w(m_locking->getWSlow());
	if (w.isNull())
		return PRL_ERR_FAILURE;

	Mode::Both output(m_root, w);
	PRL_RESULT e = output.prepare(auth_);
	if (PRL_FAILED(e))
		return e;

	return output;
}

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Log

Log::result_type Log::operator()
	(boost::mpl::at_c<stateList_type, 0>::type& , const boost::mpl::at_c<workList_type, 0>::type& work_) const
{
	Prl::Expected<Mode::Both, PRL_RESULT> x = m_factory.craftForBoth(*m_auth);
	if (x.isFailed())
		return x.error();

	return this->operator()(x.value(), work_);
}

Log::result_type Log::operator()
	(boost::mpl::at_c<stateList_type, 0>::type& , const boost::mpl::at_c<workList_type, 1>::type& work_) const
{
	Prl::Expected<Mode::Read, PRL_RESULT> x = m_factory.craftForRead();
	if (x.isFailed())
		return x.error();

	return this->operator()(x.value(), work_);
}

Log::result_type Log::operator()
	(boost::mpl::at_c<stateList_type, 1>::type& , const boost::mpl::at_c<workList_type, 0>::type& ) const
{
	return PRL_ERR_INVALID_HANDLE;
}

template<class T, class U>
Log::result_type Log::operator()(T& batch_, const U& work_) const
{
	PRL_RESULT e = batch_.add(m_factory.craftObject(m_name), work_);
	if (PRL_FAILED(e))
		return e;

	return Mode::state_type(batch_);
}

} // namespace Visitor
} // namespace Batch

///////////////////////////////////////////////////////////////////////////////
// struct Layout

QDir Layout::getCatalogRoot() const
{
	return m_mountPoint.filePath("vmtemplates");
}

///////////////////////////////////////////////////////////////////////////////
// struct Catalog

Catalog::Catalog(const Layout& layout_, CAuthHelper& auth_):
	m_root(layout_.getCatalogRoot()), m_auth(&auth_), m_locking(layout_.getLockRoot())
{
}

PRL_RESULT Catalog::find(const QString& name_, QScopedPointer<const Entry::Unit>& dst_) const
{
	Lock::Catalog::result_type r(m_locking.getRFast());
	if (r.isNull())
		return PRL_ERR_FAILURE;

	if (!Uuid::isUuid(name_))
		return PRL_ERR_INVALID_ARG;

	QFileInfo p(m_root, name_);
	if (!p.isDir())
		return PRL_ERR_FILE_NOT_FOUND;

	dst_.reset(new Entry::Unit(p, r));
	return PRL_ERR_SUCCESS;
}

Prl::Expected<QStringList, PRL_RESULT> Catalog::list() const
{
	Lock::Catalog::result_type g(m_locking.getRFast());
	if (g.isNull())
		return PRL_ERR_FAILURE;

	QStringList output;
	QDir::Filters f = QDir::NoDotAndDotDot | QDir::AllDirs | QDir::NoSymLinks | QDir::Executable;
	foreach (const QString& x, m_root.entryList(QStringList("*"), f))
	{
		if (!Uuid::isUuid(x))
			continue;

		QDir y(m_root.filePath(x));
		if (y.exists(VMDIR_DEFAULT_VM_CONFIG_FILE) || y.exists("ve.conf"))
			output << x;
	}

	return output;
}

PRL_RESULT Catalog::commit()
{
	return boost::apply_visitor(Batch::Visitor::Commit(), m_batch);
}

PRL_RESULT Catalog::log(const QString& name_, const Batch::Mode::work_type& work_)
{
	Batch::Visitor::Log::result_type x = boost::apply_visitor
		(Batch::Visitor::Log(name_, Batch::Factory(m_root, m_locking), *m_auth),
			m_batch, work_);
	if (x.isFailed())
		return x.error();

	m_batch = x.value();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Catalog::import(const QString& name_, const import_type& content_)
{
	return log(name_, Batch::Mode::Abstract::bothAction_type(new Entry::Import(content_)));
}

PRL_RESULT Catalog::export_(const QString& name_, const export_type& content_)
{
	return log(name_, Batch::Mode::Abstract::readAction_type(new Entry::Export(content_)));
}

PRL_RESULT Catalog::unlink(const QString& name_)
{
	return log(name_, Batch::Mode::Abstract::bothAction_type(new Entry::Unlink()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Dao

Prl::Expected<QStringList, PRL_RESULT> Dao::list()
{
	m_dirList = boost::none;
	PRL_RESULT e = pull();
	if (PRL_FAILED(e))
		return e;

	return m_dirList.get();
}

Dao::value_type* Dao::yield(const QString& mountPoint_) const
{
	pointer_type u(new value_type(Layout(mountPoint_), *m_auth));
	if (!u->getRoot().exists())
	{
		PRL_RESULT e = Facade(*m_auth).mkdir(u->getRoot());
		if (PRL_FAILED(e))
			return NULL;
	}
	return u.take();
}

PRL_RESULT Dao::findByRoot(const QDir& root_, pointer_type& dst_)
{
	PRL_RESULT e;
	if (!m_dirList && PRL_FAILED(e = this->pull()))
		return e;

	foreach (const QString& d, m_dirList.get())
	{
		pointer_type u(yield(d));
		if (u.isNull() || root_ != u->getRoot())
			continue;

		dst_.swap(u);
		return PRL_ERR_SUCCESS;
	}
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT Dao::findForEntry(const QString& path_, pointer_type& dst_)
{
	return findByRoot(QFileInfo(path_).dir(), dst_);
}

PRL_RESULT Dao::findByMountPoint(const QString& path_, pointer_type& dst_)
{
	PRL_RESULT e;
	if (!m_dirList && PRL_FAILED(e = this->pull()))
		return e;

	dst_.reset(yield(path_));
	return dst_.isNull() ? PRL_ERR_FILE_NOT_FOUND : PRL_ERR_SUCCESS;
}

PRL_RESULT Dao::pull()
{
	if (m_dirList)
		return PRL_ERR_SUCCESS;

	FILE* h = setmntent("/proc/mounts", "r");
	if (NULL == h)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot read the /proc/mounts: %d", errno);
		return PRL_ERR_OPEN_FAILED;
	}
	QStringList x;
	struct mntent e;
	char b[BUFSIZ];
	while (NULL != getmntent_r(h, &e, b, sizeof(b)))
	{
		if (!pcs_fs(e.mnt_dir))
			continue;
		x.push_back(e.mnt_dir);
	}
	endmntent(h);
	m_dirList = x;

	return PRL_ERR_SUCCESS;
}

} // namespace Storage
} // namespace Template

