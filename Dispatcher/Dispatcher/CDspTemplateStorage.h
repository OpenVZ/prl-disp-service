///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTemplateStorage.h
///
/// Declarations of templates storage infrastructure
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

#ifndef __CDSPTEMPLATESTORAGE_H__
#define __CDSPTEMPLATESTORAGE_H__

#include "CDspInstrument.h"
#include <boost/mpl/at.hpp>
#include <boost/function.hpp>
#include <prlcommon/Std/SmartPtr.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

namespace Template
{
namespace Storage
{
namespace Lock
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(const QFileInfo& path_, QFile::OpenMode mode_);

	PRL_RESULT enter();
private:
	QFile m_file;
	QFile::OpenMode m_mode;
};

///////////////////////////////////////////////////////////////////////////////
// struct Sentinel

struct Sentinel
{
	typedef QSharedPointer<const Unit> result_type;

	explicit Sentinel(const QString& path_): m_path(path_)
	{
	}

	result_type getForRead();
	result_type getForWrite();

private:
	const QString m_path;
	QWeakPointer<const Unit> m_read;
	QWeakPointer<const Unit> m_write;
};

///////////////////////////////////////////////////////////////////////////////
// struct Catalog

struct Catalog
{
	typedef Sentinel::result_type result_type;

	explicit Catalog(const QDir& root_);

	result_type getRFast()
	{
		return m_fast.getForRead();
	}
	result_type getWFast()
	{
		return m_fast.getForWrite();
	}
	result_type getRSlow()
	{
		return m_slow.getForRead();
	}
	result_type getWSlow()
	{
		return m_slow.getForWrite();
	}

private:
	Sentinel m_fast;
	Sentinel m_slow;
};

} // namespace Lock

struct Sandbox;
namespace Batch
{
namespace Mode
{
struct Both;
} // namespace Mode
} // namespace Batch

namespace Entry
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(const QFileInfo& root_, Lock::Catalog& locking_):
		m_root(root_), m_locking(&locking_)
	{
	}
	Unit(const QFileInfo& root_, QSharedPointer<const Lock::Unit> guard_):
		m_root(root_), m_locking(), m_guard(guard_)
	{
	}

	QString getName() const
	{
		return m_root.fileName();
	}
	const QFileInfo& getRoot() const
	{
		return m_root;
	}
	SmartPtr<CVmConfiguration> getConfig() const;
	QSharedPointer<const Lock::Unit> getLock();

private:
	QFileInfo m_root;
	Lock::Catalog* m_locking;
	QSharedPointer<const Lock::Unit> m_guard;
};

namespace Action
{
///////////////////////////////////////////////////////////////////////////////
// struct Abstract

struct Abstract
{
	virtual ~Abstract();

	virtual PRL_RESULT do_(Unit& object_) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Read

struct Read: Abstract
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Both

struct Both: Abstract
{
	virtual PRL_RESULT prepare(const Batch::Mode::Both& batch_) = 0;
	virtual PRL_RESULT undo(Unit& object_) = 0;
};

} // namespace Action

///////////////////////////////////////////////////////////////////////////////
// struct Import

struct Import: Action::Both
{
	typedef boost::function<PRL_RESULT (const QString&)> content_type;

	explicit Import(const content_type& content_): m_content(content_)
	{
	}

	PRL_RESULT prepare(const Batch::Mode::Both& batch_);
	PRL_RESULT do_(Unit& object_);
	PRL_RESULT undo(Unit& object_);

private:
	content_type m_content;
	std::auto_ptr<Sandbox> m_workspace;
};

///////////////////////////////////////////////////////////////////////////////
// struct Export

struct Export: Action::Read
{
	typedef boost::function<PRL_RESULT (const QString&)> content_type;

	explicit Export(const content_type& content_): m_content(content_)
	{
	}

	PRL_RESULT do_(Unit& object_);

private:
	content_type m_content;
};

} // namespace Entry

namespace Batch
{
namespace Mode
{
///////////////////////////////////////////////////////////////////////////////
// struct Abstract

struct Abstract
{
	typedef Lock::Catalog::result_type guard_type;
	typedef ::Instrument::Command::Batch log_type;
	typedef boost::shared_ptr<Entry::Action::Both> bothAction_type;
	typedef boost::shared_ptr<Entry::Action::Read> readAction_type;

	explicit Abstract(const guard_type& guard_): m_guard(guard_)
	{
	}

	PRL_RESULT operator()();
	PRL_RESULT add(const Entry::Unit& entry_, const bothAction_type& action_);
	PRL_RESULT add(const Entry::Unit& entry_, const readAction_type& action_);

private:
	log_type m_log;
	guard_type m_guard;
};

///////////////////////////////////////////////////////////////////////////////
// struct Read

struct Read: private Abstract
{
	explicit Read(const guard_type& guard_): Abstract(guard_)
	{
	}

	using Abstract::operator();
	PRL_RESULT add(const Entry::Unit& entry_, const readAction_type& action_)
	{
		return Abstract::add(entry_, action_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Both

struct Both: private Abstract
{
	Both(const QDir& root_, const guard_type& guard_);

	PRL_RESULT operator()();
	PRL_RESULT add(const Entry::Unit& entry_, const bothAction_type& action_);
	PRL_RESULT add(const Entry::Unit& entry_, const readAction_type& action_);
	PRL_RESULT prepare(CAuthHelper& auth_);
	PRL_RESULT getSandbox(const char* prefix_, std::auto_ptr<Sandbox>& dst_) const;

private:
	QDir m_root;
	quint32 m_stage;
	CAuthHelper* m_auth;
};

typedef boost::variant<boost::blank, Read, Both> state_type;
typedef boost::variant<Abstract::bothAction_type, Abstract::readAction_type> work_type;

} // namespace Mode

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Factory(const QDir& root_, Lock::Catalog& locking_):
		m_root(root_), m_locking(&locking_)
	{
	}

	Entry::Unit craftObject(const QString& name_) const;
	Prl::Expected<Mode::Read, PRL_RESULT> craftForRead() const;
	Prl::Expected<Mode::Both, PRL_RESULT> craftForBoth(CAuthHelper& auth_) const;

private:
    QDir m_root;
    Lock::Catalog* m_locking;
};

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Log

struct Log: boost::static_visitor<Prl::Expected<Mode::state_type, PRL_RESULT> >
{
	typedef Mode::work_type::types workList_type;
	typedef Mode::state_type::types stateList_type;

	Log(const QString& name_, const Factory& factory_, CAuthHelper& auth_):
		m_name(name_), m_factory(factory_), m_auth(&auth_)
	{
	}

	result_type operator()(boost::mpl::at_c<stateList_type, 0>::type& ,
		const boost::mpl::at_c<workList_type, 0>::type& work_) const;
	result_type operator()(boost::mpl::at_c<stateList_type, 0>::type& ,
		const boost::mpl::at_c<workList_type, 1>::type& work_) const;
	result_type operator()(boost::mpl::at_c<stateList_type, 1>::type& ,
		const boost::mpl::at_c<workList_type, 0>::type& ) const;
	template<class T, class U>
	result_type operator()(T& batch_, const U& work_) const;

private:
	QString m_name;
	Factory m_factory;
	CAuthHelper* m_auth;
};
///////////////////////////////////////////////////////////////////////////////
// struct Commit

struct Commit: boost::static_visitor<PRL_RESULT>
{
	result_type operator()(boost::blank&) const
	{
		return PRL_ERR_UNINITIALIZED;
	}
	template<class T>
	result_type operator()(T& mode_) const
	{
		return mode_();
	}
};

} // namespace Visitor
} // namespace Batch

///////////////////////////////////////////////////////////////////////////////
// struct Layout

struct Layout
{
	explicit Layout(const QDir& mountPoint_): m_mountPoint(mountPoint_)
	{
	}

	const QDir& getMountPoint() const
	{
		return m_mountPoint;
	}

	QDir getLockRoot() const
	{
		return m_mountPoint;
	}

	QDir getCatalogRoot() const;

private:
	QDir m_mountPoint;
};

///////////////////////////////////////////////////////////////////////////////
// struct Catalog

struct Catalog: boost::noncopyable
{
	typedef Entry::Import::content_type import_type;
	typedef Entry::Export::content_type export_type;

	Catalog(const Layout& layout_, CAuthHelper& auth_);

	const QDir& getRoot() const
	{
		return m_root;
	}
	Prl::Expected<QStringList, PRL_RESULT> list() const;
	PRL_RESULT find(const QString& name_, QScopedPointer<const Entry::Unit>& dst_) const;
	PRL_RESULT commit();
	PRL_RESULT unlink(const QString& name_);
	PRL_RESULT import(const QString& name_, const import_type& content_);
	PRL_RESULT export_(const QString& name_, const export_type& content_);

private:
	PRL_RESULT log(const QString& name_, const Batch::Mode::work_type& work_);

	QDir m_root;
	CAuthHelper* m_auth;
	mutable Lock::Catalog m_locking;
	Batch::Mode::state_type m_batch;
};

///////////////////////////////////////////////////////////////////////////////
// struct Dao

struct Dao
{
	typedef Catalog value_type;
	typedef QScopedPointer<value_type> pointer_type;

	explicit Dao(CAuthHelper& auth_): m_auth(&auth_)
	{
	}

	Prl::Expected<QStringList, PRL_RESULT> list();
	PRL_RESULT findByRoot(const QDir& root_, pointer_type& dst_);
	PRL_RESULT findForEntry(const QString& path_, pointer_type& dst_);
	PRL_RESULT findByMountPoint(const QString& path_, pointer_type& dst_);

private:
	value_type* yield(const QString& mountPoint_) const;
	PRL_RESULT pull();

	CAuthHelper* m_auth;
	boost::optional<QStringList> m_dirList;
};

} // namespace Storage
} // namespace Template

#endif // __CDSPTEMPLATESTORAGE_H__

