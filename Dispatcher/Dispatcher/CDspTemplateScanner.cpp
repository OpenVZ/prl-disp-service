///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTemplateScanner.cpp
///
/// Definitions of templates scanners infrastructure
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

#include "CDspService.h"
#include "CDspTemplateScanner.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

namespace Template
{
namespace Scanner
{
///////////////////////////////////////////////////////////////////////////////
// struct Folder

void Folder::run()
{
	Prl::Expected<QStringList, PRL_RESULT> a = m_catalog->list();
	if (a.isFailed())
		return;

	QSet<QString> b, c;
	foreach (const QString& i, a.value())
	{
		b.insert(m_catalog->getRoot().absoluteFilePath(i));
	}
	foreach (Facade::File f, m_facade.getFiles())
	{
		Prl::Expected<QString, PRL_RESULT> r = f.getPath();
		if (r.isFailed())
			m_facade.remove(f.getId());
		else if (b.contains(r.value()))
			c.insert(r.value());
		else
			m_facade.remove(f.getId());
	}
	foreach (const QString& i, b)
	{
		entryPointer_type p;
		PRL_RESULT e = m_catalog->find(QFileInfo(i).fileName(), p);
		if (PRL_FAILED(e))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot find the entry %s: %s",
				qPrintable(i), PRL_RESULT_TO_STRING(e));
			continue;
		}
		SmartPtr<CVmConfiguration> x = p->getConfig();
		if (!x.isValid())
			continue;

		e = m_facade.settle(*x);
		if (PRL_FAILED(e))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot settle the entry %s: %s",
				qPrintable(i), PRL_RESULT_TO_STRING(e));
			continue;
		}
	}
}

PRL_RESULT Folder::schedule(const Facade::Folder& facade_, dao_type dao_)
{
	Prl::Expected<QString, PRL_RESULT> r = facade_.getPath();
	if (r.isFailed())
		return r.error();

	dao_type::pointer_type x;
	PRL_RESULT e = dao_.findByRoot(r.value(), x);
	if (PRL_FAILED(e))
		return e;

	Folder* f = new Folder(facade_, x.take());
	f->setAutoDelete(true);
	QThreadPool::globalInstance()->start(f);

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Host

void Host::run()
{
	QList<Facade::Folder> a = m_facade->getFolders();
	Prl::Expected<QSet<QString>, PRL_RESULT> b = scan();
	if (b.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot scan shared storages: %s",
			PRL_RESULT_TO_STRING(b.error()));
		return;
	}
	QSet<QString> c;
	foreach (Facade::Folder f, a)
	{
		Prl::Expected<QString, PRL_RESULT> r = f.getPath();
		if (r.isFailed())
			m_facade->remove(f.getId());
		else if (b.value().contains(r.value()))
			c.insert(r.value());
		else
			m_facade->remove(f.getId());
	}
	foreach (const QString& p, b.value().subtract(c))
	{
		PRL_RESULT e = m_facade->insert(p);
		if (PRL_FAILED(e))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot insert the shared storage %s: %s",
				qPrintable(p), PRL_RESULT_TO_STRING(e));
		}
	}
	foreach (const Facade::Folder& f, m_facade->getFolders())
	{
		Folder::schedule(f, m_dao);
	}
}

Prl::Expected<QSet<QString>, PRL_RESULT> Host::scan()
{
	Prl::Expected<QStringList, PRL_RESULT> b = m_dao.list();
	if (b.isFailed())
		return b.error();

	QSet<QString> output;
	foreach (const QString& p, b.value())
	{
		dao_type::pointer_type x;
		PRL_RESULT e = m_dao.findByMountPoint(p, x);
		if (PRL_SUCCEEDED(e))
			output << x->getRoot().absolutePath();
		else
		{
			WRITE_TRACE(DBG_FATAL, "Skip the mount point %s scan: %s",
				qPrintable(p), PRL_RESULT_TO_STRING(e));
		}
	}

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Engine

Engine::Engine(::Vm::Directory::Ephemeral& catalog_, CDspService& service_):
	QObject(&service_), m_facade(catalog_, Facade::Workbench(service_))
{
}

void Engine::stop()
{
}

void Engine::execute()
{
	Host* h = new Host(m_facade, dao_type(m_helper));
	h->setAutoDelete(true);
	QThreadPool::globalInstance()->start(h);
	schedule(PERIOD);
}

void Engine::schedule(quint32 timeout_)
{
	QTimer::singleShot(timeout_, this, SLOT(execute()));
}

} // namespace Scanner
} // namespace Template
