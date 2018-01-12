///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmConfigManager
///
/// this object wraps load/store to disk vm configuration files
/// it synchronize threads for simultaneous access
/// @author Artemr
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

// #define FORCE_LOGGING_ON

#include "CDspVmConfigManager.h"
#include <QReadLocker>
#include <QWriteLocker>
#include "CDspVmDirManager.h"
#include "CDspService.h"
#include "CDspVNCStarter_p.h"
#include "Tasks/Task_EditVm.h"
#include <prlxmlmodel/VmConfig/CVmHardDisk.h>
#include "EditHelpers/CMultiEditMergeVmConfig.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>
#include <prlxmlmodel/ParallelsObjects/CXmlModelHelper.h>

#include "Dispatcher/Dispatcher/Cache/CacheImpl.h"

namespace Vm
{
namespace Config
{
enum Defaults {DEFAULT_MEMGUARANTEE_PERCENTS = 40};

QString getVmHomeDirName(const QString& uuid_)
{
	return Uuid(uuid_).toStringWithoutBrackets();
}

///////////////////////////////////////////////////////////////////////////////
// struct MemGuarantee

quint64 MemGuarantee::operator()(quint64 ramsize_) const
{
	switch(m_type)
	{
	case PRL_MEMGUARANTEE_AUTO:
		// ceil(ramsize * xx%)
		return (ramsize_ * DEFAULT_MEMGUARANTEE_PERCENTS + 99) / 100;
	case PRL_MEMGUARANTEE_PERCENTS:
		// ceil(ramsize * xx%)
		return (ramsize_ * m_guarantee + 99) / 100;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// struct Nvram

void Nvram::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmSettings()->getVmStartupOptions()->getBios()->setNVRAM
		(new_.getVmSettings()->getVmStartupOptions()->getBios()->getNVRAM());
}

namespace RemoteDisplay
{
///////////////////////////////////////////////////////////////////////////////
// struct Pivot

void Pivot::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	CVmRemoteDisplay* x = Vnc::Traits::purify(&old_);
	if (NULL == x)
		return;

	CVmRemoteDisplay* y = Vnc::Traits::purify(&new_);
	if (NULL != y)
	{
		y->setHostName(x->getHostName());
		y->setEncrypted(x->isEncrypted());
		y->setPortNumber(x->getPortNumber());
		y->setWebSocketPortNumber(x->getWebSocketPortNumber());
	}
}

} // namespace RemoteDisplay

namespace Index
{
///////////////////////////////////////////////////////////////////////////////
// struct Match

template<>
bool Match<CVmHardDisk>::operator()(const CVmHardDisk* item_)
{
	return (item_->getSystemName() == m_needle->getSystemName() ||
		(!item_->getSerialNumber().isEmpty() && 
		(item_->getSerialNumber() == m_needle->getSerialNumber())));
}

template<>
bool Match<CVmOpticalDisk>::operator()(const CVmOpticalDisk* item_)
{
	return item_->getStackIndex() == m_needle->getStackIndex() &&
		item_->getInterfaceType() == m_needle->getInterfaceType();
}

template<>
bool Match<CVmFloppyDisk>::operator()(const CVmFloppyDisk* item_)
{
	return item_->getIndex() ==
		Parallels::fromBase26(m_needle->getTargetDeviceName().remove(0, 2));
}

template<>
bool Match<CVmGenericNetworkAdapter>::operator()(const CVmGenericNetworkAdapter* item_)
{
	return item_->getHostInterfaceName() == m_needle->getHostInterfaceName();
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

template <class T, PRL_DEVICE_TYPE D>
typename Device<T, D>::iterator_type
	Device<T, D>::findDevice(
		typename Device<T, D>::iterator_type begin_,
		typename Device<T, D>::iterator_type end_, const T* needle_)
{
	return std::find_if(begin_, end_, Match<T>(*needle_));
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

	if (!m_population.isEmpty() && m_population.first() == 0)
	{
		population_type::const_iterator ii =
			std::adjacent_find(m_population.constBegin(), m_population.constEnd(),
				boost::lambda::_1 + 1 < boost::lambda::_2);

		if (ii != m_population.constEnd())
			output = *ii + 1;
		else
			output = m_population.last() + 1;
	}

	m_population.push_back(output);
	qSort(m_population);
	return output;
}

} // namespace Index

namespace Patch
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

template<class T>
Builder<T>& Builder<T>::drawAlias()
{
	foreach(T *d, *m_target)
	{
		T* x = CXmlModelHelper::GetDeviceByIndex(m_source, d->getIndex());
		if (NULL != x)
			d->setAlias(x->getAlias());
	}
	return *this;
}

template<class T>
Builder<T>& Builder<T>::updateDisabled()
{
	// disabled devices are not present in the list after "Reverse"
	// transformation - copy them from the original list
	foreach(const T *d, m_source)
	{
		if (d->getEnabled() != PVE::DeviceDisabled)
			continue;

		if (NULL == CXmlModelHelper::GetDeviceByIndex(*m_target, d->getIndex()))
			*m_target << new T(*d);
	}
	return *this;
}

template<class T>
Builder<T>& Builder<T>::updateConnected()
{
	typedef typename list_type::const_iterator iterator_type;
	iterator_type b = m_source.begin(), e = m_source.end();
	foreach(T *d, *m_target)
	{
		if (PVE::DeviceDisabled == d->getEnabled())
			continue;

		iterator_type m = std::find_if(b, e, Config::Index::Match<T>(*d));
		d->setConnected(e == m || *m == NULL || !guessConnected(**m) ?
			PVE::DeviceDisconnected : PVE::DeviceConnected);
	}
	return *this;
}

template<class T>
Builder<T>& Builder<T>::updateDisconnected()
{
	foreach(T *d, *m_target)
	{
		T* o = CXmlModelHelper::GetDeviceByIndex(m_source, d->getIndex());
		if (NULL == o || o->getConnected() != PVE::DeviceDisconnected)
			continue;

		d->setSystemName(o->getSystemName());
		d->setUserFriendlyName(o->getUserFriendlyName());
		d->setConnected(PVE::DeviceDisconnected);
		d->setDescription(o->getDescription());
	}
	return *this;
}

template <class T>
bool Builder<T>::guessConnected(const T&)
{
	return true;
}

template <>
bool Builder<CVmFloppyDisk>::guessConnected(const CVmFloppyDisk& device_)
{
	return !device_.getSystemName().isEmpty();
}

template <>
bool Builder<CVmOpticalDisk>::guessConnected(const CVmOpticalDisk& device_)
{
	return !device_.getSystemName().isEmpty();
}

template <>
bool Builder<CVmGenericNetworkAdapter>::guessConnected(const CVmGenericNetworkAdapter& device_)
{
	return PVE::DeviceConnected == device_.getConnected();
}

///////////////////////////////////////////////////////////////////////////////
// struct Runtime

void Runtime::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	drawAliases(new_, old_);
	CVmHardware *n = new_.getVmHardwareList();
	CVmHardware *o = old_.getVmHardwareList();
	
	Builder<CVmHardDisk>(o->m_lstHardDisks, n->m_lstHardDisks).updateConnected();
	Builder<CVmFloppyDisk>(o->m_lstFloppyDisks, n->m_lstFloppyDisks).updateConnected();
	Builder<CVmOpticalDisk>(o->m_lstOpticalDisks, n->m_lstOpticalDisks).updateConnected();
	Builder<CVmGenericNetworkAdapter>(o->m_lstNetworkAdapters, n->m_lstNetworkAdapters).updateConnected();
}

void Runtime::drawAliases(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	CVmHardware *n = new_.getVmHardwareList();
	CVmHardware *o = old_.getVmHardwareList();
	Builder<CVmHardDisk>(o->m_lstHardDisks, n->m_lstHardDisks).drawAlias();
	Builder<CVmSerialPort>(o->m_lstSerialPorts, n->m_lstSerialPorts).drawAlias();
	Builder<CVmOpticalDisk>(o->m_lstOpticalDisks, n->m_lstOpticalDisks).drawAlias();
}

///////////////////////////////////////////////////////////////////////////////
// struct Index

void Index::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	CVmStartupOptions* o = old_.getVmSettings()->getVmStartupOptions();
	CVmStartupOptions* n = new_.getVmSettings()->getVmStartupOptions();
	n->setAutoStart(o->getAutoStart());
	n->setAutoStartDelay(o->getAutoStartDelay());
	new_.getVmSettings()->getShutdown()->setAutoStop
		(old_.getVmSettings()->getShutdown()->getAutoStop());

	QList<CVmStartupOptions::CVmBootDevice*> b = n->m_lstBootDeviceList;
	Config::Index::Device<CVmHardDisk, PDE_HARD_DISK>
		(old_.getVmHardwareList()->m_lstHardDisks, b)
			(new_.getVmHardwareList()->m_lstHardDisks);
	Config::Index::Device<CVmOpticalDisk, PDE_OPTICAL_DISK>
		(old_.getVmHardwareList()->m_lstOpticalDisks, b)
			(new_.getVmHardwareList()->m_lstOpticalDisks);
	Config::Index::Device<CVmFloppyDisk, PDE_FLOPPY_DISK>
		(old_.getVmHardwareList()->m_lstFloppyDisks, b)
			(new_.getVmHardwareList()->m_lstFloppyDisks);
	Config::Index::Device<CVmGenericNetworkAdapter, PDE_GENERIC_NETWORK_ADAPTER>
		(old_.getVmHardwareList()->m_lstNetworkAdapters, b)
			(new_.getVmHardwareList()->m_lstNetworkAdapters);
}

///////////////////////////////////////////////////////////////////////////////
// struct State

void State::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	CVmHardware *o = new_.getVmHardwareList();
	CVmHardware *n = old_.getVmHardwareList();

	// XXX: HDDs could not be disconnected
	Builder<CVmHardDisk>(n->m_lstHardDisks, o->m_lstHardDisks).updateDisabled();
	Builder<CVmFloppyDisk>(n->m_lstFloppyDisks, o->m_lstFloppyDisks)
		.updateDisabled().updateDisconnected();
	Builder<CVmOpticalDisk>(n->m_lstOpticalDisks, o->m_lstOpticalDisks)
		.updateDisabled().updateDisconnected();
}

} // namespace Patch

///////////////////////////////////////////////////////////////////////////////
// struct OsInfo

void OsInfo::do_(CVmConfiguration& old_, const CVmConfiguration& new_) 
{
	// DEBUG LOGS FOR #PSBM-44712
	if (new_.getVmSettings() != NULL && new_.getVmSettings()->getVmCommonOptions() != NULL)
	{
		WRITE_TRACE(DBG_DEBUG, "#PSBM-44712 OsType: %d, OsVersion: %d",
			new_.getVmSettings()->getVmCommonOptions()->getOsType(),
			new_.getVmSettings()->getVmCommonOptions()->getOsVersion());
	}
	else
		WRITE_TRACE(DBG_DEBUG, "#PSBM-44712 NO OS INFO");

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

namespace Cpu
{

///////////////////////////////////////////////////////////////////////////////
// struct Copy

void Copy::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmHardwareList()->setCpu
		(new CVmCpu(new_.getVmHardwareList()->getCpu()));
}

} // namespace Cpu

///////////////////////////////////////////////////////////////////////////////
// struct Identification

void Identification::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	new_.getVmIdentification()->setHomePath
		(old_.getVmIdentification()->getHomePath());
	new_.getVmIdentification()->setCreationDate
		(old_.getVmIdentification()->getCreationDate());
	new_.getVmIdentification()->setSourceVmUuid
		(old_.getVmIdentification()->getSourceVmUuid());
	new_.getVmIdentification()->setVmUptimeStartDateTime
		(old_.getVmIdentification()->getVmUptimeStartDateTime());
}

///////////////////////////////////////////////////////////////////////////////
// struct OpticalDisks

void OpticalDisks::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	QList<CVmOpticalDisk*>& l = new_.getVmHardwareList()->m_lstOpticalDisks;
	QList<CVmOpticalDisk*>& o = old_.getVmHardwareList()->m_lstOpticalDisks;

	foreach(CVmOpticalDisk* h, l)
	{
		CVmOpticalDisk* x = CXmlModelHelper::GetDeviceByIndex(o, h->getIndex());
		if (NULL == x)
			continue;

		h->setSystemName(x->getSystemName());
		h->setUserFriendlyName(x->getUserFriendlyName());
		h->setDescription(x->getDescription());
		h->setLabel(x->getLabel());
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct HardDisks

void HardDisks::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	QList<CVmHardDisk*>& l = new_.getVmHardwareList()->m_lstHardDisks;
	QList<CVmHardDisk*>& o = old_.getVmHardwareList()->m_lstHardDisks;

	foreach(CVmHardDisk* h, l)
	{
		CVmHardDisk* x = CXmlModelHelper::GetDeviceByIndex(o, h->getIndex());
		if (NULL == x)
			continue;

		h->setUuid(x->getUuid());
		h->setStorageURL(x->getStorageURL());
		if (0 == h->getSize())
			h->setSize(x->getSize());
	}
	foreach(CVmHardDisk* h, o)
	{
		if (h->getConnected() == PVE::DeviceConnected)
			continue;

		if (NULL == CXmlModelHelper::GetDeviceByIndex(l, h->getIndex()))
			new_.getVmHardwareList()->addHardDisk(new CVmHardDisk(*h));
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
		CVmGenericNetworkAdapter* x = CXmlModelHelper::GetDeviceByIndex(o, a->getIndex());
		if (NULL == x)
			continue;

		a->setAutoApply(x->isAutoApply());
		a->setDefaultGateway(x->getDefaultGateway());
		a->setDefaultGatewayIPv6(x->getDefaultGatewayIPv6());
		a->setConfigureWithDhcp(x->isConfigureWithDhcp());
		a->setConfigureWithDhcpIPv6(x->isConfigureWithDhcpIPv6());
		a->setDnsIPAddresses(x->getDnsIPAddresses());
		a->setSearchDomains(x->getSearchDomains());
		a->setHostMacAddress(x->getHostMacAddress());
		// If spoofing protection is disabled, check our config.
		if (!a->getPktFilter()->isPreventIpSpoof())
		{
			a->getPktFilter()->setPreventIpSpoof(
					x->getPktFilter()->isPreventIpSpoof());
		}
		a->setFirewall(new CVmNetFirewall(x->getFirewall()));
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct MemoryOptions

void MemoryOptions::do_(CVmConfiguration& old_, const CVmConfiguration& new_)
{
	old_.getVmHardwareList()->setMemory(
		(new CVmMemory(new_.getVmHardwareList()->getMemory())));
}

///////////////////////////////////////////////////////////////////////////////
// struct HighAvailability

void HighAvailability::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	new_.getVmSettings()->setHighAvailability(
		new CVmHighAvailability(old_.getVmSettings()->getHighAvailability()));
	new_.getVmSettings()->setClusterOptions(
		new ClusterOptions(old_.getVmSettings()->getClusterOptions()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Tools

void Tools::do_(CVmConfiguration& new_, const CVmConfiguration& old_)
{
	new_.getVmSettings()->setVmTools(
		new CVmTools(old_.getVmSettings()->getVmTools()));
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
		if (unit_.getAuth()->getUserFullName() != owner_)
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

namespace Edit
{
///////////////////////////////////////////////////////////////////////////////
// struct Unlocked

PRL_RESULT Unlocked::operator()(const action_type& action_)
{
	if (!m_actor.isValid())
		return PRL_ERR_INVALID_ARG;

	PRL_RESULT e;
	QString d = m_actor->getVmDirectoryUuid();
	CDspVmDirHelper& h = m_service->getVmDirHelper();
	SmartPtr<CVmConfiguration> x = h.getVmConfigByUuid(d, m_uuid, e);
	if (!x.isValid())
		return PRL_FAILED(e) ? e : PRL_ERR_FILE_NOT_FOUND;

	const IOSender::Handle f = QString("%1-%2").arg(d).arg(m_uuid);

	CMultiEditMergeVmConfig* m = h.getMultiEditDispatcher();
	m->registerBeginEdit(m_uuid, f, x);
	action_type::result_type r = action_(*x);
	if (r.isFailed())
	{
		m->cleanupBeginEditMark(d, f);
		return r.error();
	}
	e = m_service->getVmConfigManager()
		.saveConfig(x, x->getVmIdentification()->getHomePath(), m_actor, true, true);
	if (PRL_FAILED(e))
		return e;

	m->registerCommit(m_uuid, f);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// class Atomic

PRL_RESULT Atomic::operator()(const action_type& action_)
{
	const SmartPtr<CDspClient> a = m_decorated.getActor();
	if (!a.isValid())
		return PRL_ERR_INVALID_ARG;

	QString d = a->getVmDirectoryUuid();
	CDspVmDirHelper& h = m_service->getVmDirHelper();
	PRL_RESULT output = h.registerExclusiveVmOperation
		(m_decorated.getUuid(), d, PVE::DspCmdCtlApplyVmConfig, a);
	if (PRL_SUCCEEDED(output))
	{
		QMutexLocker g(h.getMultiEditDispatcher());
		output = m_decorated(action_);
		g.unlock();
		h.unregisterExclusiveVmOperation
			(m_decorated.getUuid(), d, PVE::DspCmdCtlApplyVmConfig, a);
	}
	return output;
}

CVmIdent Atomic::getObject() const
{
	QString d;
	const SmartPtr<CDspClient> a = m_decorated.getActor();
	if (a.isValid())
		d = a->getVmDirectoryUuid();

	return MakeVmIdent(m_decorated.getUuid(), d);
}

} // namespace Edit
} // namespace Config

namespace Registration
{
namespace Unattended
{
///////////////////////////////////////////////////////////////////////////////
// struct Survey

quint32 Survey::getGuestType() const
{
	const CVmSettings* s = m_model->getVmSettings();
	if (NULL == s)
		return 0;

	const CVmCommonOptions* o = s->getVmCommonOptions();
	if (NULL == o)
		return 0;

	return o->getOsVersion();
}

const CVmFloppyDisk* Survey::find(const QString& image_) const
{
	foreach(const CVmFloppyDisk* f, getList())
	{
		if (PVE::FloppyDiskImage != f->getEmulatedType())
			continue;

		if (image_ == f->getSystemName() && f->getUserFriendlyName() == image_)
			return f;
	}       
	return NULL;
}

quint32 Survey::getNextItemId() const
{
	return getList().size();
}

quint32 Survey::getNextIndex() const
{
	QList<CVmFloppyDisk* > x = getList();
	return CXmlModelHelper::GetUnusedDeviceIndex<CVmFloppyDisk>(x);
}

quint32 Survey::getNextStackIndex() const
{
	Vm::Config::Index::Pool::population_type p;
	std::transform(getList().begin(), getList().end(),
		std::back_inserter(p), boost::bind(&CVmFloppyDisk::getStackIndex, _1));
	return Vm::Config::Index::Pool(p).getAvailable();
}

const QList<CVmFloppyDisk* >& Survey::getList() const
{
	static const QList<CVmFloppyDisk* > s_empty;
	CVmHardware* h = m_model->getVmHardwareList();
	if (NULL == h)
		return s_empty;

	return h->m_lstFloppyDisks;
}

} // namespace Unattended

///////////////////////////////////////////////////////////////////////////////
// struct Reconfiguration

void Reconfiguration::react(QString directory_, QString uuid_)
{
	PRL_RESULT x;
	SmartPtr<CVmConfiguration> y = m_service->getVmDirHelper()
		.getVmConfigByUuid(directory_, uuid_, x);
	if (!y.isValid())
		return;

	Unattended::Survey s(*y);
	quint32 t = s.getGuestType();
	if (!IS_WINDOWS(t))
		return;

	QString i = ParallelsDirs::getWindowsUnattendedFloppy(t);
	if (i.isEmpty())
		return;

	if (!QFileInfo(i).exists())
		return;

	if (NULL != s.find(i))
		return;

	CVmFloppyDisk d;
	d.setEnabled(PVE::DeviceEnabled);
	d.setConnected(PVE::DeviceConnected);
	d.setEmulatedType(PVE::FloppyDiskImage);
	d.setSystemName(i);
	d.setUserFriendlyName(i);
	d.setItemId(s.getNextItemId());
	d.setIndex(s.getNextIndex());
	d.setStackIndex(s.getNextStackIndex());

	CVmEvent e;
	e.addEventParameter(new CVmEventParameter(PVE::String,
		d.toString(), EVT_PARAM_VMCFG_NEW_DEVICE_CONFIG));
	Task_EditVm::atomicEditVmConfigByVm(directory_, uuid_,
		e, CDspClient::makeServiceUser(directory_));
}

} // namespace Registration
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

	Vm::Config::Access::Work w(config_file, pUserSession);
	w.setConfig(pConfig);
	QWriteLocker locker(&m_mtxAccessLocker);
	return m_trie->get(config_file).save(w, do_replace, BNeedToSaveRelativePath);
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
