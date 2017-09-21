///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTemplateScanner.h
///
/// Declarations of templates scanners infrastructure
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

#ifndef __CDSPTEMPLATESCANNER_H__
#define __CDSPTEMPLATESCANNER_H__

#include <QObject>
#include "CDspTemplateFacade.h"
#include "CDspTemplateStorage.h"

namespace Template
{
namespace Scanner
{
typedef Storage::Dao dao_type;
typedef Storage::Entry::Unit entry_type;

///////////////////////////////////////////////////////////////////////////////
// struct Folder

struct Folder: QRunnable
{
	typedef QScopedPointer<const entry_type> entryPointer_type;

	Folder(const Facade::Folder& facade_, dao_type::value_type* catalog_):
		m_facade(facade_), m_catalog(catalog_)
	{
	}

	void run();
	static PRL_RESULT schedule(const Facade::Folder& facade_, dao_type dao_);

private:
	Facade::Folder m_facade;
	QScopedPointer<dao_type::value_type> m_catalog;
};

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host: QRunnable
{
	Host(Facade::Host& facade_, const dao_type& dao_):
		m_dao(dao_), m_facade(&facade_)
	{
	}

	void run();

private:
	Prl::Expected<QSet<QString>, PRL_RESULT> scan();

	dao_type m_dao;
	Facade::Host* m_facade;
};

///////////////////////////////////////////////////////////////////////////////
// struct Engine

struct Engine: QObject
{
	enum
	{
		PERIOD = 300000
	};

	Engine(::Vm::Directory::Ephemeral& catalog_, CDspService& service_);

	void start()
	{
		schedule(0);
	}

public slots:
	void stop();

protected slots:
	void execute();

private:
	Q_OBJECT

	void schedule(quint32 timeout_);

	CAuthHelper m_helper;
	Facade::Host m_facade;
};

} // namespace Scanner
} // namespace Template

#endif // __CDSPTEMPLATESCANNER_H__

