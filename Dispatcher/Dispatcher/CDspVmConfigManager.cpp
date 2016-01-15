///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmConfigManager
///
/// this object wraps load/store to disk vm configuration files
/// it synchronize threads for simultaneous access
/// @author Artemr
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

// #define FORCE_LOGGING_ON

#include "CDspVmConfigManager.h"
#include <QReadLocker>
#include <QWriteLocker>
#include "CDspVmDirManager.h"
#include "CDspService.h"
#include <prlxmlmodel/VmConfig/CVmHardDisk.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>

#include "Dispatcher/Dispatcher/Cache/CacheImpl.h"


namespace Vm
{
namespace Config
{
///////////////////////////////////////////////////////////////////////////////
// struct RemoteDisplay

void RemoteDisplay::do_(CVmConfiguration& old_, const CVmConfiguration& new_) 
{
	old_.getVmSettings()->getVmRemoteDisplay()->setPortNumber
		(new_.getVmSettings()->getVmRemoteDisplay()->getPortNumber());
}

///////////////////////////////////////////////////////////////////////////////
// struct Nvram

void Nvram::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmSettings()->getVmStartupOptions()->getBios()->setNVRAM
		(new_.getVmSettings()->getVmStartupOptions()->getBios()->getNVRAM());
}

namespace Index
{

namespace Match
{

///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image
{
	explicit Image(const QString& image_) : m_image(image_)
	{
	}

	template<class T>
	bool operator()(const T* device_) const
	{
		 return device_->getSystemName() == m_image;
	}

private:
	QString m_image;
};

///////////////////////////////////////////////////////////////////////////////
// struct Index

struct Index
{
	explicit Index(QString target_)
	{
		// remove 'sd/fd' prefix
		m_index = Parallels::fromBase26(target_.remove(0, 2));
	}

	template<class T>
	bool operator()(const T* device_) const
	{
		 return device_->getIndex() == m_index;
	}

private:
	uint m_index;
};

template<class T>
typename QList<T*>::iterator choose(typename QList<T*>::iterator begin_,
		typename QList<T*>::iterator end_, const T& device_)
{
	if (!device_.getSystemName().isEmpty())
		return std::find_if(begin_, end_, Image(device_.getSystemName()));
	// If image path is not set, then this means that the
	// device was originally disconnected.
	// Try matching by target device name, which we expect to
	// be in the form: <prefix><base26-encoded device index>.
	if (!device_.getTargetDeviceName().isEmpty())
		return std::find_if(begin_, end_, Index(device_.getTargetDeviceName()));
	return end_;
}

} // namespace Match

///////////////////////////////////////////////////////////////////////////////
// struct Device

template<>
QList<CVmHardDisk*>::iterator
Device<CVmHardDisk, PDE_HARD_DISK>::findDevice
	(QList<CVmHardDisk*>::iterator begin_, QList<CVmHardDisk*>::iterator end_,
	 	const CVmHardDisk* disk_)
{
	return std::find_if(begin_, end_, Match::Image(disk_->getSystemName()));
}

template<>
QList<CVmOpticalDisk*>::iterator
Device<CVmOpticalDisk, PDE_OPTICAL_DISK>::findDevice
	(QList<CVmOpticalDisk*>::iterator begin_, QList<CVmOpticalDisk*>::iterator end_,
	 	const CVmOpticalDisk* cdrom_)
{
	return Match::choose(begin_, end_, *cdrom_);
}

template<>
QList<CVmFloppyDisk*>::iterator
Device<CVmFloppyDisk, PDE_FLOPPY_DISK>::findDevice
	(QList<CVmFloppyDisk*>::iterator begin_, QList<CVmFloppyDisk*>::iterator end_,
		const CVmFloppyDisk* floppy_)
{
	return Match::choose(begin_, end_, *floppy_);
}

template<>
QList<CVmGenericNetworkAdapter*>::iterator
Device<CVmGenericNetworkAdapter, PDE_GENERIC_NETWORK_ADAPTER>::findDevice
	(QList<CVmGenericNetworkAdapter*>::iterator begin_, QList<CVmGenericNetworkAdapter*>::iterator end_,
		const CVmGenericNetworkAdapter* adapter_)
{
	return std::find_if(begin_, end_,
		 boost::bind(&CVmGenericNetworkAdapter::getHostInterfaceName, _1) ==
			adapter_->getHostInterfaceName());
}

///////////////////////////////////////////////////////////////////////////////
// struct Pool

Pool::Pool(const population_type& initial_): m_population(initial_)
{
	qSort(m_population);
}

quint32 Pool::getAvailable()
{
	quint32 output = 0;
	if (!m_population.isEmpty() && m_population.first() != 0)
	{
		population_type::const_iterator ii =
			std::adjacent_find(m_population.constBegin(), m_population.constEnd(),
				boost::lambda::_1 + 1 < boost::lambda::_2);

		if (ii != m_population.constEnd())
			output = *ii + 1;
		else
			output = m_population.last() + 1;
	}

	m_population.insert(output, output);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Patch

void Patch::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	QList<CVmStartupOptions::CVmBootDevice*> b = new_.getVmSettings()
		->getVmStartupOptions()->m_lstBootDeviceList;

	Device<CVmHardDisk, PDE_HARD_DISK>
		(old_.getVmHardwareList()->m_lstHardDisks, b)
			(new_.getVmHardwareList()->m_lstHardDisks);
	Device<CVmOpticalDisk, PDE_OPTICAL_DISK>
		(old_.getVmHardwareList()->m_lstOpticalDisks, b)
			(new_.getVmHardwareList()->m_lstOpticalDisks);
	Device<CVmFloppyDisk, PDE_FLOPPY_DISK>
		(old_.getVmHardwareList()->m_lstFloppyDisks, b)
			(new_.getVmHardwareList()->m_lstFloppyDisks);
	Device<CVmGenericNetworkAdapter, PDE_GENERIC_NETWORK_ADAPTER>
		(old_.getVmHardwareList()->m_lstNetworkAdapters, b)
			(new_.getVmHardwareList()->m_lstNetworkAdapters);
}

} // namespace Index

namespace State
{

template <class T>
void updateDisconnected(QList<T*>& new_, const QList<T*>& old_)
{
	foreach(T *d, new_)
	{
		typename QList<T*>::const_iterator i = std::find_if(old_.begin(), old_.end(),
			boost::bind(&T::getIndex, _1) == d->getIndex());
		if (i == old_.end())
			continue;
		if ((*i)->getConnected() == PVE::DeviceDisconnected)
		{
			d->setSystemName((*i)->getSystemName());
			d->setUserFriendlyName((*i)->getUserFriendlyName());
			d->setConnected(PVE::DeviceDisconnected);
		}
	}
}

template <class T>
void updateDisabled(QList<T*>& new_, const QList<T*>& old_)
{
	// disabled devices are not present in the list after "Reverse"
	// transformation - copy them from the original list
	foreach(const T *d, old_)
	{
		if (d->getEnabled() != PVE::DeviceDisabled)
			continue;
		if (std::find_if(new_.begin(), new_.end(),
			boost::bind(&T::getIndex, _1) == d->getIndex()) == new_.end())
		{
			new_ << new T(*d);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Patch

void Patch::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	CVmHardware *o = new_.getVmHardwareList();
	CVmHardware *n = old_.getVmHardwareList();

	// XXX: HDDs could not be disconnected
	updateDisabled(o->m_lstHardDisks, n->m_lstHardDisks);
	updateDisconnected(o->m_lstOpticalDisks, n->m_lstOpticalDisks);
	updateDisabled(o->m_lstOpticalDisks, n->m_lstOpticalDisks);
	updateDisconnected(o->m_lstFloppyDisks, n->m_lstFloppyDisks);
	updateDisabled(o->m_lstFloppyDisks, n->m_lstFloppyDisks);
}

} // namespace State

///////////////////////////////////////////////////////////////////////////////
// struct OsInfo

void OsInfo::do_(CVmConfiguration& old_, const CVmConfiguration& new_) 
{
	old_.getVmSettings()->getVmCommonOptions()->setOsType
		(new_.getVmSettings()->getVmCommonOptions()->getOsType());
	old_.getVmSettings()->getVmCommonOptions()->setOsVersion
		(new_.getVmSettings()->getVmCommonOptions()->getOsVersion());
}

///////////////////////////////////////////////////////////////////////////////
// struct RuntimeOptions

void RuntimeOptions::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmSettings()->setVmRuntimeOptions
		(new CVmRunTimeOptions(new_.getVmSettings()->getVmRuntimeOptions()));
}

///////////////////////////////////////////////////////////////////////////////
// struct GlobalNetwork

void GlobalNetwork::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmSettings()->setGlobalNetwork(
		(new CVmGlobalNetwork(new_.getVmSettings()->getGlobalNetwork())));
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

void Cpu::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmHardwareList()->setCpu
		(new CVmCpu(new_.getVmHardwareList()->getCpu()));
}

///////////////////////////////////////////////////////////////////////////////
// struct HardDisks

void HardDisks::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	QList<CVmHardDisk*>& l = new_.getVmHardwareList()->m_lstHardDisks;
	QList<CVmHardDisk*>& o = old_.getVmHardwareList()->m_lstHardDisks;

	foreach(CVmHardDisk* h, l)
	{
		QList<CVmHardDisk*>::iterator it = std::find_if(o.begin(), o.end(),
			boost::bind(&CVmHardDisk::getIndex, _1)
				== h->getIndex());

		if (it == o.end())
			continue;

		h->setUuid((*it)->getUuid());
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct NetworkDevices

void NetworkDevices::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	QList<CVmGenericNetworkAdapter*>& l = new_.getVmHardwareList()->m_lstNetworkAdapters;
	QList<CVmGenericNetworkAdapter*>& o = old_.getVmHardwareList()->m_lstNetworkAdapters;

	foreach(CVmGenericNetworkAdapter* a, l)
	{
		QList<CVmGenericNetworkAdapter*>::iterator it = std::find_if(o.begin(), o.end(),
			 boost::bind(&CVmGenericNetworkAdapter::getIndex, _1)
			 	== a->getIndex());

		if (it == o.end())
			continue;

		a->setAutoApply((*it)->isAutoApply());
		a->setDefaultGateway((*it)->getDefaultGateway());
		a->setDefaultGatewayIPv6((*it)->getDefaultGatewayIPv6());
		a->setConfigureWithDhcp((*it)->isConfigureWithDhcp());
		a->setConfigureWithDhcpIPv6((*it)->isConfigureWithDhcpIPv6());
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct MemoryOptions

void MemoryOptions::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmHardwareList()->setMemory(
		(new CVmMemory(new_.getVmHardwareList()->getMemory())));
}

namespace Access
{
///////////////////////////////////////////////////////////////////////////////
// struct Work

struct Work
{
	Work(const QString& path_, const SmartPtr<CDspClient>& user_):
		m_path(path_), m_user(user_)
	{
	}

	const QString& getPath() const
	{
		return m_path;
	}
	CAuthHelper* getAuth() const
	{
		return m_user.isValid() ? &(m_user->getAuthHelper()) : NULL;
	}
	const SmartPtr<CVmConfiguration>& getConfig() const
	{
		return m_config;
	}
	Work& setConfig(const SmartPtr<CVmConfiguration>& config_)
	{
		m_config = config_;
		return *this;
	}
	bool setConfig(CacheBase<CVmConfiguration>& src_)
	{
		setConfig(src_.getFromCache(m_path, m_user));
		return m_config.isValid();
	}
	void save(CacheBase<CVmConfiguration>& dst_) const
	{
		dst_.updateCache(m_path, m_config, m_user);
	}
private:
	QString m_path;
	SmartPtr<CDspClient> m_user;
	SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Base

Base::Base(): m_cache(new Cache<CVmConfiguration>())
{
}

Base::~Base()
{
}

void Base::forget(const Work& unit_)
{
	Work u = unit_;
	u.setConfig(SmartPtr<CVmConfiguration>());
	u.save(getCache());
}

///////////////////////////////////////////////////////////////////////////////
// struct InMemory

PRL_RESULT InMemory::load(Work& dst_, bool )
{
	return dst_.setConfig(getCache()) ?
			PRL_ERR_SUCCESS : PRL_ERR_FILE_NOT_FOUND;
}
PRL_RESULT InMemory::save(const Work& unit_, bool , bool saveRelative_)
{
	SmartPtr<CVmConfiguration> x = unit_.getConfig();
	if (!x.isValid())
		return PRL_ERR_INVALID_ARG;
	if (saveRelative_)
	{
		x = SmartPtr<CVmConfiguration>(new CVmConfiguration(*x));
		x->setRelativePath();
		Work w = unit_;
		w.setConfig(x).save(getCache());
	}
	else
		unit_.save(getCache());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Backup

struct Backup
{
	explicit Backup(const Work& work_):
		m_work(&work_), m_path(getPath(work_))
	{
	}

	static QString getPath(const Work& work_)
	{
		return work_.getPath() + VMDIR_DEFAULT_VM_BACKUP_SUFFIX;
	}
	void operator()(bool replace_, bool saveRelative_) const;
	bool prepareTarget() const;
private:
	bool setPermissions() const;

	const Work* m_work;
	const QString m_path;
};

bool Backup::setPermissions() const
{
	PRL_ASSERT( QFile::exists(m_path) );
	if( !QFile::exists(m_path) )
		return false;

	if( !CFileHelper::isFsSupportPermsAndOwner(m_work->getPath()) )
		return true;

	CAuth::AccessMode origOwnerPerm = 0, origOthersPerm = 0;
	CAuth::AccessMode backupOwnerPerm = 0, backupOthersPerm = 0;
	bool bFake = false;

	CAuthHelper* a = m_work->getAuth();
	if (NULL == a)
		return false;

	PRL_RESULT errOrig = CFileHelper::GetSimplePermissionsToFile(
		m_work->getPath(), origOwnerPerm, origOthersPerm, bFake, *a);

	PRL_RESULT errBackup = CFileHelper::GetSimplePermissionsToFile(
		m_path, backupOwnerPerm, backupOthersPerm, bFake, *a);

	// DEVELOPERS NOTE:
	//              : DO NOT TRACE for FAILED operations to prevent log flooding
	if( PRL_SUCCEEDED( errOrig ) && PRL_SUCCEEDED( errBackup )
		&& ( origOwnerPerm != backupOwnerPerm || origOthersPerm != backupOthersPerm )
		)
	{
		PRL_RESULT res = CFileHelper::SetSimplePermissionsToFile(
			m_path, *a, &origOwnerPerm, &origOthersPerm, false);
		if( PRL_SUCCEEDED(res) )
			return true;

		WRITE_TRACE(DBG_FATAL, "Unable to set permission ( error =%s ) to backup file %s"
			, PRL_RESULT_TO_STRING( res )
			, QSTR2UTF8(m_path));
	}
	return false;
}

void Backup::operator()(bool replace_, bool saveRelative_) const
{
	PRL_RESULT e = m_work->getConfig()
			->saveToFile(m_path, replace_, saveRelative_);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot write VM config backup file '%s'! Error: 0x%x",
			QSTR2UTF8(m_path), e);
	}
}

bool Backup::prepareTarget() const
{
	if (!QFile::exists(m_path) && !CFileHelper::CreateBlankFile(m_path, m_work->getAuth()))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot prepare blank file for VM config backup file '%s'!",
			QSTR2UTF8(m_path));
		return false;
	}
	if (!setPermissions())
	{
		WRITE_TRACE(DBG_DEBUG, "Unable to update permissions of backup config for %s ",
			QSTR2UTF8(m_work->getPath()));
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct Restore

struct Restore
{
	explicit Restore(const Work& work_): m_work(work_)
	{
		m_work.setConfig(SmartPtr<CVmConfiguration>
			(new CVmConfiguration()));
	}

	PRL_RESULT prepareSource();
	PRL_RESULT prepareTarget();
	PRL_RESULT operator()(CacheBase<CVmConfiguration>& dst_);
private:
	bool setPermissions();

	Work m_work;
};

bool Restore::setPermissions()
{
	CAuthHelper* a = m_work.getAuth();
	QString b = Backup::getPath(m_work), c = m_work.getPath();
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate(a);

		PRL_ASSERT( QFile::exists(c) );
		if( !QFile::exists(c) )
			return false;

		PRL_ASSERT( QFile::exists(b) );
		if( !QFile::exists(b) )
			return false;

		if( !CFileHelper::isFsSupportPermsAndOwner(c) )
			return true;
	}
	if (NULL == a)
		return false;
	CAuth::AccessMode backupOwnerPerm = 0, backupOthersPerm = 0;
	bool bFake = false;

	PRL_RESULT errBackup = CFileHelper::GetSimplePermissionsToFile(
		b, backupOwnerPerm, backupOthersPerm, bFake,
		*a);

	if( PRL_FAILED( errBackup ) )
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get permissions  from backup config %s."
			" Try to set default permissions."
			, QSTR2UTF8(b) );

		backupOwnerPerm = CAuth::fileMayRead | CAuth::fileMayWrite | CAuth::fileMayExecute;
		backupOthersPerm = CAuth::fileMayRead ;
	}

	PRL_RESULT res = CFileHelper::SetSimplePermissionsToFile(
		c, *a, &backupOwnerPerm, &backupOthersPerm, false);

	if( PRL_SUCCEEDED( res ) )
		return true;

	WRITE_TRACE(DBG_FATAL, "Unable to set permissions  to restored config %s", QSTR2UTF8(c));
	return false;
}

PRL_RESULT Restore::prepareSource()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(m_work.getAuth());

	QFile file;
	file.setFileName(m_work.getPath());
	if(file.exists() && PRL_SUCCEEDED(m_work.getConfig()->loadFromFile(&file, false)))
	{
		WRITE_TRACE(DBG_FATAL, "VM config file '%s' is valid",
			QSTR2UTF8( file.fileName() ));
		return PRL_ERR_VM_CONFIG_IS_ALREADY_VALID;
	}

	file.setFileName(Backup::getPath(m_work));
	if(!file.exists() || PRL_FAILED(m_work.getConfig()->loadFromFile( &file, false )))
	{
		WRITE_TRACE(DBG_FATAL, "VM config backup file '%s' is broken or doesn't exists",
			QSTR2UTF8( file.fileName() ));
		return PRL_ERR_FILE_NOT_FOUND;
	}
	if (QFile::exists(m_work.getPath()))
		return PRL_ERR_SUCCESS;

	return PRL_ERR_NO_VM_DIR_CONFIG_FOUND;
}

PRL_RESULT Restore::operator()(CacheBase<CVmConfiguration>& dst_)
{
	CAuthHelperImpersonateWrapper _impersonate(m_work.getAuth());
	PRL_RESULT e = m_work.getConfig()->saveToFile(m_work.getPath(), false, false);
	if(PRL_FAILED(e))
		return e;

	m_work.save(dst_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Restore::prepareTarget()
{
	if (!CFileHelper::CreateBlankFile(m_work.getPath(), m_work.getAuth()))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot prepare blank file for VM config file '%s'!",
			QSTR2UTF8(m_work.getPath()));
		return PRL_ERR_FAILURE;
	}
	//#462940 setPermissions() should NOT be called under Impersonate
	if (!setPermissions())
	{
		WRITE_TRACE(DBG_FATAL, "Unable to set permissions to restored config");
		PRL_ASSERT(QFile::remove(m_work.getPath())) ;
		return PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS;
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Mixed

PRL_RESULT Mixed::load(Work& dst_, bool direct_)
{
	if(!direct_ && dst_.setConfig(getCache()))
		return PRL_ERR_SUCCESS;

	LOG_MESSAGE(DBG_FATAL, "xxx YYY: Config will be loaded from disk. path = %s",
			QSTR2UTF8(dst_.getPath()));

	QFile f(dst_.getPath());
	SmartPtr<CVmConfiguration> x(new CVmConfiguration());
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(dst_.getAuth());
	PRL_RESULT e = x->loadFromFile(&f, true);
	if (PRL_FAILED(e))
	{
		forget(dst_);
		return e;
	}
	dst_.setConfig(x).save(getCache());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Mixed::save(const Work& src_, bool replace_, bool saveRelative_)
{
	Backup u(src_);
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(src_.getAuth());
	if (u.prepareTarget())
		u(replace_, saveRelative_);

	PRL_RESULT output = src_.getConfig()->saveToFile
				(src_.getPath(), replace_, saveRelative_);
	if (PRL_SUCCEEDED(output))
		src_.save(getCache());

	return output;
}

PRL_RESULT Mixed::restore(const Work& unit_, const QString& owner_)
{
	Restore u(unit_);
	PRL_RESULT e = u.prepareSource();
	switch (e)
	{
	case PRL_ERR_NO_VM_DIR_CONFIG_FOUND:
		if (CDspService::instance()->isServerMode()
			&& unit_.getAuth()->getUserFullName() != owner_)
		{
			WRITE_TRACE(DBG_FATAL, "Restoring VM config when owner is not the same user is not implemented");
			return PRL_ERR_UNIMPLEMENTED;
		}
		e = u.prepareTarget();
		if (PRL_FAILED(e))
			return e;
	case PRL_ERR_SUCCESS:
		return u(getCache());
	default:
		return e;
	}
}

bool Mixed::canRestore(const Work& unit_) const
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(unit_.getAuth());

	QString b = Backup::getPath(unit_);
	if (!QFile::exists(b))
		return false;

	QFile file(b);
	CVmConfiguration x;
	return PRL_SUCCEEDED(x.loadFromFile(&file, false));
}

} // namespace Access
} // namespace Config
} // namespace Vm

namespace Trie
{
///////////////////////////////////////////////////////////////////////////////
// struct Node

struct Node
{
	Vm::Config::Access::Base* find(QStringList& path_) const;
	void set(QStringList& path_, Vm::Config::Access::Base* data_)
	{
		if (path_.isEmpty())
		{
			m_data = QSharedPointer<Vm::Config::Access::Base>(data_);
			return;
		}
		QString x = path_.front();
		path_.pop_front();
		m_children[x].set(path_, data_);
	}
private:
	QHash<QString, Node> m_children;
	QSharedPointer<Vm::Config::Access::Base> m_data;
};

Vm::Config::Access::Base* Node::find(QStringList& path_) const
{
	do
	{
		if (path_.isEmpty())
			break;
		QHash<QString, Node>::const_iterator p =
			m_children.find(path_.front());
		if (m_children.end() == p)
			break;
		path_.pop_front();
		Vm::Config::Access::Base* x = p.value().find(path_);
		if (NULL != x)
			return x;
	} while(false);

	return m_data.data();
}

///////////////////////////////////////////////////////////////////////////////
// struct Root

struct Root: private Node
{
	Root(): m_default(new Vm::Config::Access::Mixed())
	{
	}

	Vm::Config::Access::Base& get(const QString& path_) const
	{
		QStringList x = getElements(path_);
		return *(find(x) ? : m_default.data());
	}
	bool set(const QString& path_, Vm::Config::Access::Base* data_)
	{
		QStringList x = getElements(path_);
		if (x.isEmpty())
		{
			delete data_;
			return false;
		}
		Node::set(x, data_);
		return true;
	}
private:
	static QStringList getElements(const QString& path_)
	{
		QFileInfo x(path_);
		if (!x.isAbsolute())
			return QStringList();

		QStringList output;
		while(!x.isRoot())
		{
			output.push_front(x.fileName());
			x = QFileInfo(x.absoluteDir(), QString());
		}
#ifdef _WIN_
		output.push_front(x.absoluteFilePath().toLower());
#endif // _WIN_
		return output;
	}

	QScopedPointer<Vm::Config::Access::Base> m_default;
};

} // namespace Trie

///////////////////////////////////////////////////////////////////////////////
// class CDspVmConfigManager

CDspVmConfigManager::CDspVmConfigManager(void): m_trie(new Trie::Root())
{
}

CDspVmConfigManager::~CDspVmConfigManager(void)
{
	m_trie.reset();
}

void CDspVmConfigManager::removeFromCache( const QString& path)
{
	QWriteLocker locker(&m_mtxAccessLocker);
	m_trie->get(path).forget(Vm::Config::Access::Work(path, SmartPtr<CDspClient>()));
}

/**
* @brief Load config from disk.
* @param SmartPtr<CVmConfiguration> pConfig - config pointer
* @param const QString & strFileName - filename to load
* @param bool BNeedLoadAbsolutePath - is need load from config to pConfig object
* absolutes pathes for device images
* @return result of operation
*/
PRL_RESULT CDspVmConfigManager::loadConfig( SmartPtr<CVmConfiguration>& pConfig,
											const QString& strFileName,
											SmartPtr<CDspClient> pUserSession,
											bool BNeedLoadAbsolutePath,
											bool bLoadDirectlyFromDisk )
{
	QReadLocker locker(&m_mtxAccessLocker);
	Vm::Config::Access::Work w(strFileName, pUserSession);
	PRL_RESULT e = m_trie->get(strFileName).load(w, bLoadDirectlyFromDisk);
	if (PRL_FAILED(e))
		return e;

	pConfig = w.getConfig();
	if (!BNeedLoadAbsolutePath)
		pConfig->setRelativePath();

	return PRL_ERR_SUCCESS;
}

/**
* @brief Save config from disk.
* @param SmartPtr<CVmConfiguration> pConfig - config pointer
* @param QString config_file - path to save configuration
* @param pUserSession - client session to set access rights for backup config file
* @param do_replace - reserved
* @param bool BNeedLoadAbsolutePath - is need save on disk config relative pathes
* @return result of operation
*/
PRL_RESULT CDspVmConfigManager::saveConfig( SmartPtr<CVmConfiguration> pConfig,
											const QString& config_file,
											SmartPtr<CDspClient> pUserSession,
											bool do_replace,
											bool BNeedToSaveRelativePath )
{
	PRL_ASSERT(pUserSession.isValid());
	QList<CVmIdent> lstConfigIdents = CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(config_file);

	Vm::Config::Access::Work w(config_file, pUserSession);
	w.setConfig(pConfig);
	QWriteLocker locker(&m_mtxAccessLocker);
	PRL_RESULT output = m_trie->get(config_file).save(w, do_replace, BNeedToSaveRelativePath);
	locker.unlock();

	foreach( CVmIdent id, lstConfigIdents )
		CDspService::instance()->getVmConfigWatcher().registerVmToWatch(config_file, id );

	return output;
}

/**
* @brief Restore corrupted config file from backup config file.
* @param QString config_file - path to restore configuration
* @param pUserSession - client session to set access rights for config file
* @return result of operation
*/
PRL_RESULT CDspVmConfigManager::restoreConfig(const QString& config_file,
											  SmartPtr<CDspClient> pUserSession,
											  const QString& owner)
{
	CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(config_file);

	PRL_RESULT output = PRL_ERR_SUCCESS;
	{
		Vm::Config::Access::Work w(config_file, pUserSession);
		QWriteLocker locker(&m_mtxAccessLocker);
		output = m_trie->get(config_file).restore(w, owner);
	}
	if (PRL_FAILED(output))
	{
		WRITE_TRACE( DBG_FATAL, "Unable to restore config by error %s (%#x) for vm with config %s"
			, PRL_RESULT_TO_STRING(output)
			, output
			, QSTR2UTF8(config_file) );
	}
	CDspService::instance()->getVmConfigWatcher().update();
	return output;
}

/**
* @brief Check restoring ability.
* @param QString config_file - path to restore configuration
* @return true - can restore
*/
bool CDspVmConfigManager::canConfigRestore( const QString& config_file, SmartPtr<CDspClient> pUserSession )
{
	QWriteLocker locker(&m_mtxAccessLocker);
	Vm::Config::Access::Work w(config_file, pUserSession);
	return m_trie->get(config_file).canRestore(w);
}

SmartPtr<QWriteLocker> CDspVmConfigManager::lockOnWrite()
{
	return SmartPtr<QWriteLocker>( new QWriteLocker(&m_mtxAccessLocker) );
}

SmartPtr<QReadLocker> CDspVmConfigManager::lockOnRead()
{
	return SmartPtr<QReadLocker>( new QReadLocker(&m_mtxAccessLocker) );
}

PRL_RESULT CDspVmConfigManager::adopt
	(const QString& path_, Vm::Config::Access::Base* access_)
{
	QWriteLocker locker(&m_mtxAccessLocker);
	if (m_trie->set(path_, access_))
		return PRL_ERR_SUCCESS;

	return PRL_ERR_INVALID_ARG;
}

/*********************CHardDiskConfigCache****************************/

static QString getVmHardDiskConfigName(const QString& sFileName)
{
	return QString(sFileName + "/" DISK_DESCRIPTOR_XML);
}

CDspVmConfigManager::CHardDiskConfigCache::CHardDiskConfigCache()
:m_HardDiskCache( new Cache<CVmHardDisk>(/* 2 days ttl */ 2*24*60*60) )
{
}

SmartPtr<CVmHardDisk> CDspVmConfigManager::CHardDiskConfigCache::getConfig(const QString& sFileName,
		SmartPtr<CDspClient> pUserSession)
{
	return m_HardDiskCache->getFromCache( getVmHardDiskConfigName(sFileName), pUserSession );
}

void CDspVmConfigManager::CHardDiskConfigCache::update(const QString& sFileName,
		const SmartPtr<CVmHardDisk>& pConfig, SmartPtr<CDspClient> pUserSession)
{

	m_HardDiskCache->updateCache( getVmHardDiskConfigName(sFileName), pConfig, pUserSession );
}

void CDspVmConfigManager::CHardDiskConfigCache::remove(SmartPtr<CVmConfiguration> &pConfig)
{
	Q_ASSERT(pConfig);

	foreach(CVmHardDisk *pHdd, pConfig->getVmHardwareList()->m_lstHardDisks)
		if (pHdd->getEmulatedType() == PVE::HardDiskImage)
			remove(pHdd->getSystemName());
}

void CDspVmConfigManager::CHardDiskConfigCache::remove(const QString& sFileName)
{
	update(sFileName, SmartPtr<CVmHardDisk>(0), SmartPtr<CDspClient>(0));
}
