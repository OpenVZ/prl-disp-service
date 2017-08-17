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
// struct Catalog

struct Catalog
{
	explicit Catalog(const QDir& root_);

	QSharedPointer<const Unit> getRFast();
	QSharedPointer<const Unit> getWFast();
	QSharedPointer<const Unit> getWSlow();

private:
	const QString m_fast;
	const QString m_slow;
	QWeakPointer<const Unit> m_rFast;
	QWeakPointer<const Unit> m_wFast;
	QWeakPointer<const Unit> m_wSlow;
};

} // namespace Lock

struct Batch;
struct Sandbox;
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

///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action
{
	virtual ~Action();

	virtual PRL_RESULT prepare(const Batch& batch_) = 0;
	virtual PRL_RESULT do_(Unit& object_) = 0;
	virtual PRL_RESULT undo(Unit& object_) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Import

struct Import: Action
{
	typedef boost::function<PRL_RESULT (const QString&)> content_type;

	explicit Import(const content_type& content_): m_content(content_)
	{
	}

	PRL_RESULT prepare(const Batch& batch_);
	PRL_RESULT do_(Unit& object_);
	PRL_RESULT undo(Unit& object_);

private:
	content_type m_content;
	std::auto_ptr<Sandbox> m_workspace;
};

///////////////////////////////////////////////////////////////////////////////
// struct Export

struct Export: Action
{
	typedef boost::function<PRL_RESULT (const QString&)> content_type;

	explicit Export(const content_type& content_): m_content(content_)
	{
	}

	PRL_RESULT prepare(const Batch& batch_)
	{
		Q_UNUSED(batch_);
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT do_(Unit& object_);
	PRL_RESULT undo(Unit& object_)
	{
		Q_UNUSED(object_);
		return PRL_ERR_SUCCESS;
	}

private:
	content_type m_content;
};

} // namespace Entry

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

struct Catalog
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
	PRL_RESULT log(const QString& name_, Entry::Action* action_);

	QDir m_root;
	CAuthHelper* m_auth;
	std::auto_ptr<Batch> m_batch;
	mutable Lock::Catalog m_locking;
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

