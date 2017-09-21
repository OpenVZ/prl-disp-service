///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTemplateFacade.cpp
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

#include "CDspService.h"
#include "CDspTemplateFacade.h"

namespace Template
{
namespace Facade
{
///////////////////////////////////////////////////////////////////////////////
// struct Workbench

Workbench::Workbench(): m_service(CDspService::instance())
{
}

CDspVmDirHelper& Workbench::getDirectoryHelper() const
{
	return m_service->getVmDirHelper();
}

CDspVmDirManager& Workbench::getDirectoryManager() const
{
	return m_service->getVmDirManager();
}

CDspVmConfigManager& Workbench::getConfigManager() const
{
	return m_service->getVmConfigManager();
}

///////////////////////////////////////////////////////////////////////////////
// struct File

Prl::Expected<QString, PRL_RESULT> File::getPath() const
{
	CDspLockedPointer<CVmDirectoryItem> p = getDirectoryManager()
		.getVmDirItemByUuid(m_folder, m_id);
	if (!p.isValid())
		return PRL_ERR_FILE_NOT_FOUND;

	return QFileInfo(p->getVmHome()).absolutePath();
}

Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> File::getConfig() const
{
	PRL_RESULT e;
	SmartPtr<CVmConfiguration> output = getDirectoryHelper()
		.getVmConfigByUuid(m_folder, m_id, e);
	if (PRL_FAILED(e))
		return e;
	if (!output.isValid())
		return PRL_ERR_FILE_NOT_FOUND;

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Instrument

PRL_RESULT Instrument::persist(CVmConfiguration item_)
{
	const CVmIdentification* i = item_.getVmIdentification();
	return getConfigManager().saveConfig(SmartPtr<CVmConfiguration>
		(&item_, SmartPtrPolicy::DoNotReleasePointee),
		i->getHomePath(), CDspClient::makeServiceUser(m_folder));
}

void Instrument::discard(const CVmConfiguration& item_)
{
	const CVmIdentification* i = item_.getVmIdentification();
	getConfigManager().removeFromCache(i->getHomePath());
}

PRL_RESULT Instrument::register_(const CVmConfiguration& item_)
{
	const CVmIdentification* i = item_.getVmIdentification();
	QScopedPointer<CVmDirectoryItem> x(new CVmDirectoryItem());
	x->setVmUuid(i->getVmUuid());
	x->setVmName(i->getVmName());
	x->setVmHome(i->getHomePath());
	x->setVmType(PVT_VM);
	x->setValid(PVE::VmValid);
	x->setRegistered(PVE::VmRegistered);
	x->setTemplate(true);
	PRL_RESULT output = getDirectoryHelper().insertVmDirectoryItem(m_folder, x.data());
	if (PRL_SUCCEEDED(output))
		x.take();

	return output;
}

PRL_RESULT Instrument::unregister(const CVmConfiguration& item_)
{
	const CVmIdentification* i = item_.getVmIdentification();
	return getDirectoryHelper().deleteVmDirectoryItem(m_folder, i->getVmUuid());
}

///////////////////////////////////////////////////////////////////////////////
// struct Registrar

Registrar::Registrar(const QString& folder_, const CVmConfiguration& candidate_,
	const Workbench& workbench_):
	Workbench(workbench_), m_folder(folder_), m_candidate(&candidate_)
{
	Facade::Instrument x(folder_, workbench_);
	m_instrument.addItem(boost::bind(&Facade::Instrument::persist, x, boost::cref(candidate_)),
			boost::bind(&Facade::Instrument::discard, x, boost::cref(candidate_)));
	m_instrument.addItem(boost::bind(&Facade::Instrument::register_, x, boost::cref(candidate_)),
			boost::bind(&Facade::Instrument::unregister, x, boost::cref(candidate_)));
}

PRL_RESULT Registrar::begin()
{
	if (m_guard)
		return PRL_ERR_DOUBLE_INIT;

	CVmIdentification i(m_candidate->getVmIdentification());
	m_guard = lock_type(i.getVmUuid(), i.getHomePath(), i.getVmName());
	PRL_RESULT output = getDirectoryManager()
		.checkAndLockNotExistsExclusiveVmParameters(m_folder,
			m_guard.get_ptr());
	if (PRL_FAILED(output))
		m_guard = boost::none;

	return output;
}

PRL_RESULT Registrar::execute()
{
	if (!m_guard)
		return PRL_ERR_UNINITIALIZED;

	return m_instrument.execute();
}

PRL_RESULT Registrar::rollback()
{
	if (!m_guard)
		return PRL_ERR_UNINITIALIZED;

	m_instrument.rollback();
	return unlock();
}

PRL_RESULT Registrar::unlock()
{
	if (!m_guard)
		return PRL_ERR_UNINITIALIZED;

	getDirectoryManager().unlockExclusiveVmParameters(m_guard.get_ptr());
	m_guard = boost::none;

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Settler

PRL_RESULT Settler::operator()(const CVmConfiguration& entry_)
{
	typedef CVmDirectory::TemporaryCatalogueItem lock_type;

	CVmIdentification i(entry_.getVmIdentification());
	lock_type x(i.getVmUuid(), i.getHomePath(), i.getVmName());
	PRL_RESULT e = getDirectoryManager()
		.lockExistingExclusiveVmParameters(m_folder, &x);
	if (PRL_FAILED(e))
		return e;

	PRL_RESULT output = persist(entry_);
	if (PRL_SUCCEEDED(output))
	{
		getDirectoryHelper().sendVmConfigChangedEvent
			(m_folder, entry_.getVmIdentification()->getVmUuid());
	}
	getDirectoryManager().unlockExclusiveVmParameters(&x);

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Folder

Folder::Folder(const QString& id_, const Workbench& workbench_):
	Workbench(workbench_), m_id(id_), m_user(CDspClient::makeServiceUser(m_id))
{
}

Prl::Expected<QString, PRL_RESULT> Folder::getPath() const
{
	CDspLockedPointer<CVmDirectory> p = getDirectoryManager().getVmDirectory(m_id);
	if (!p.isValid())
		return PRL_ERR_FILE_NOT_FOUND;

	return p->getDefaultVmFolder();
}

QList<File> Folder::getFiles() const
{
	QList<File> output;
	::Vm::Directory::Dao::Locked d(getDirectoryManager());
	foreach (const ::Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		if (m_id == i.first)
			output << File(i.second->getVmUuid(), i.first, *this);
	}

	return output;
}

PRL_RESULT Folder::remove(const QString& uid_)
{
	QList<File> a = getFiles();
	QList<File>::iterator e = a.end(), m = std::find_if
		(a.begin(), e, boost::bind(&File::getId, _1) == boost::cref(uid_));
	if (e == m)
		return PRL_ERR_FILE_NOT_FOUND;

	Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> c = m->getConfig();
	if (c.isFailed())
		return c.error();

	CProtoCommandPtr x = CProtoSerializer::CreateVmDeleteProtoCommand(uid_, QStringList());
	SmartPtr<IOPackage> y = DispatcherPackage::createInstance(PVE::DspCmdDirUnregVm, x);
	return getDirectoryHelper().unregOrDeleteVm(m_user, y, c.value()->toString(),
		PVD_UNREGISTER_ONLY | PVD_NOT_MODIFY_VM_CONFIG | PVD_SKIP_HA_CLUSTER)
		.wait().getTask()->getLastErrorCode();
}

PRL_RESULT Folder::settle(const CVmConfiguration& entry_)
{
	Registrar r(m_id, entry_, *this);
	PRL_RESULT e = r.begin();
	CVmIdentification i(entry_.getVmIdentification());
	switch (e)
	{
	case PRL_ERR_VM_ALREADY_REGISTERED:
	case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
	{
		Settler u(m_id, *this);
		return u(entry_);
	}
	case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
	{
		CVmConfiguration y(entry_);
		QString z(i.getVmName().append(".")
			.append(Uuid::createUuid().toString()));
		y.getVmIdentification()->setVmName(z);
		return settle(y);
	}
	default:
		if (PRL_FAILED(e))
			return e;
	}

	PRL_RESULT output = r.execute();
	if (PRL_SUCCEEDED(output))
	{
		r.commit();
		getService().getVmStateSender()
			->onVmRegistered(m_id, i.getVmUuid(), i.getVmName());
	}
	else
		r.rollback();

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Host

QList<Folder> Host::getFolders() const
{
	QMutexLocker g(&m_lock);
	QList<Folder> output;
	output.reserve(m_folderList.size());
	foreach (const QString& u, m_folderList)
	{
		output << Folder(u, *this);
	}
	return output;
}

PRL_RESULT Host::remove(const QString& uid_)
{
	{
		QMutexLocker g(&m_lock);
		if (!m_folderList.remove(uid_))
			return PRL_ERR_VM_UUID_NOT_FOUND;
	}
	Folder x(uid_, *this);
	Prl::Expected<QString, PRL_RESULT> p = x.getPath();
	if (p.isFailed())
		return p.error();

	foreach (File f, x.getFiles())
	{
		x.remove(f.getId());
	}
	m_catalog->remove(x.getId());
	getConfigManager().adopt(p.value(), NULL);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Host::insert(const QString& path_)
{
	Prl::Expected<QString, PRL_RESULT> u = m_catalog->insert(path_);
	if (u.isFailed())
		return u.error();

	getConfigManager().adopt(path_, new ::Vm::Config::Access::InMemory());
	QMutexLocker g(&m_lock);
	m_folderList.insert(u.value());

	return PRL_ERR_SUCCESS;
}

} // namespace Facade
} // namespace Template

