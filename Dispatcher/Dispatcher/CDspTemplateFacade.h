///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTemplateFacade.h
///
/// Declarations of facades for managing directory entries for templates
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

#ifndef __CDSPTEMPLATEFACADE_H__
#define __CDSPTEMPLATEFACADE_H__

#include "CDspInstrument.h"
#include "CDspVmDirManager.h"
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

class CDspClient;
class CDspService;
class CDspVmDirHelper;
class CDspVmDirManager;
class CDspVmConfigManager;

namespace Vm
{
namespace Directory
{
struct Ephemeral;

} // namespace Directory
} // namespace Vm

namespace Template
{
namespace Facade
{
///////////////////////////////////////////////////////////////////////////////
// struct Workbench

struct Workbench
{
	Workbench();
	explicit Workbench(CDspService& service_): m_service(&service_)
	{
	}

	CDspService& getService() const
	{
		return *m_service;
	}
	CDspVmDirHelper& getDirectoryHelper() const;
	CDspVmDirManager& getDirectoryManager() const;
	CDspVmConfigManager& getConfigManager() const;

private:
	CDspService* m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct File

struct File: private Workbench
{
	File(const QString& id_, const QString& folder_, const Workbench& workbench_):
		Workbench(workbench_), m_id(id_), m_folder(folder_)
	{
	}

	const QString& getId() const
	{
		return m_id;
	}
	Prl::Expected<QString, PRL_RESULT> getPath() const;
	Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> getConfig() const;

private:
	QString m_id;
	QString m_folder;
};

///////////////////////////////////////////////////////////////////////////////
// struct Instrument

struct Instrument: protected Workbench
{
	Instrument(const QString& folder_, const Workbench& workbench_):
		Workbench(workbench_), m_folder(folder_)
	{
	}
	
	PRL_RESULT persist(CVmConfiguration item_);
	void discard(const CVmConfiguration& item_);
	PRL_RESULT register_(const CVmConfiguration& item_);
	PRL_RESULT unregister(const CVmConfiguration& item_);

private:
	const QString m_folder;
};

///////////////////////////////////////////////////////////////////////////////
// struct Registrar

struct Registrar: private Workbench
{
	Registrar(const QString& folder_, const CVmConfiguration& candidate_,
		const Workbench& workbench_);

	PRL_RESULT begin();
	PRL_RESULT execute();
	PRL_RESULT commit()
	{
		return unlock();
	}
	PRL_RESULT rollback();
	
private:
	typedef CVmDirectory::TemporaryCatalogueItem lock_type;

	PRL_RESULT unlock();

	QString m_folder;
	const CVmConfiguration* m_candidate;
	boost::optional<lock_type> m_guard;
	::Instrument::Command::Batch m_instrument;
};

///////////////////////////////////////////////////////////////////////////////
// struct Settler

struct Settler: private Instrument
{
	Settler(const QString& folder_, const Workbench& workbench_):
		Instrument(folder_, workbench_), m_folder(folder_)
	{
	}

	PRL_RESULT operator()(const CVmConfiguration& entry_);

private:
	const QString m_folder;
};

///////////////////////////////////////////////////////////////////////////////
// struct Folder

struct Folder: private Workbench
{
	Folder(const QString& id_, const Workbench& workbench_);

	const QString& getId() const
	{
		return m_id;
	}
	Prl::Expected<QString, PRL_RESULT> getPath() const;
	QList<File> getFiles() const;
	PRL_RESULT remove(const QString& uid_);
	PRL_RESULT settle(const CVmConfiguration& entry_);

private:
	QString m_id;
	SmartPtr<CDspClient> m_user;
};

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host: private Workbench
{
	Host(::Vm::Directory::Ephemeral& catalog_, const Workbench& workbench_):
		Workbench(workbench_), m_catalog(&catalog_)
	{
	}

	QList<Folder> getFolders() const;
	PRL_RESULT remove(const QString& uid_);
	PRL_RESULT insert(const QString& path_);

private:
	::Vm::Directory::Ephemeral* m_catalog;
	mutable QMutex m_lock;
	QSet<QString> m_folderList;
};

} // namespace Facade
} // namespace Template

#endif // __CDSPTEMPLATEFACADE_H__

