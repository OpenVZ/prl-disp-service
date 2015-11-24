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

#ifndef CDSP_VM_CONFIG_ACCESS_SYNCH_H
#define CDSP_VM_CONFIG_ACCESS_SYNCH_H

#include "XmlModel/VmConfig/CVmConfiguration.h"
#include <prlsdk/PrlErrors.h>
#include "Libraries/Std/SmartPtr.h"
#include "Dispatcher/Dispatcher/Cache/Cache.h"
#include <QReadWriteLock>
#include <QHash>
#include <QVector>
#include <QDateTime>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

class CDspClient;
class CVmConfiguration;
class CVmHardDisk;

namespace Vm
{
namespace Config
{
///////////////////////////////////////////////////////////////////////////////
// struct RemoteDisplay

struct RemoteDisplay
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

///////////////////////////////////////////////////////////////////////////////
// struct DeviceIndex

template <class T, PRL_DEVICE_TYPE D>
struct DeviceIndex
{
	DeviceIndex(const CVmConfiguration& config_)
	: m_disks(config_.getVmHardwareList()->m_lstHardDisks)
	{
		m_indexes.resize(m_disks.length());
		std::transform(m_disks.begin(), m_disks.end(), m_indexes.begin(),
			boost::bind(&T::getIndex, _1));
		qSort(m_indexes);
	}

	unsigned findIndex(const T* disk_);
	unsigned getAvaliableIndex()
	{
		unsigned index = 0;

		if (!m_indexes.isEmpty() || m_indexes.first() != 0)
		{
			QVector<unsigned>::const_iterator ii =
				std::adjacent_find(m_indexes.constBegin(), m_indexes.constEnd(),
					boost::lambda::_1 + 1 < boost::lambda::_2);

			if (ii != m_indexes.constEnd())
				index = *ii + 1;
			else
				index = m_indexes.last() + 1;
		}

		m_indexes.insert(index, index);
		return index;
	}

	static void do_(CVmConfiguration& new_, const CVmConfiguration& old_)
	{
		typedef QList<CVmStartupOptions::CVmBootDevice*> bootList_type;

		QList<T*>& newList = new_.getVmHardwareList()->m_lstHardDisks;
		bootList_type bootListCopy = 
			new_.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList;

		DeviceIndex<T, D> di(old_);
		foreach(T* h, newList)
		{
			unsigned index = di.findIndex(h);
			bootList_type::iterator b = std::find_if(bootListCopy.begin(), bootListCopy.end(),
				 boost::bind(&CVmStartupOptions::CVmBootDevice::getIndex, _1)
					== h->getIndex() &&
				 boost::bind(&CVmStartupOptions::CVmBootDevice::getType, _1)
					== D);

			if (b != bootListCopy.end())
			{
				(*b)->setIndex(index);
				bootListCopy.erase(b);
			}

			h->setIndex(index);
		}
		std::sort(newList.begin(), newList.end(), boost::bind(&T::getIndex, _1) <
				boost::bind(&T::getIndex, _2));
	};

private:
	QList<T*> m_disks;
	QVector<unsigned> m_indexes;
};

struct OsInfo
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct RuntimeOptions
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct GlobalNetwork
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct Cpu
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Reviser

template <class N, typename B>
struct Reviser
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_)
	{
		B::do_(old_, new_);
		N::do_(old_, new_);
	}
};

template<class N>
struct Reviser<N, void>
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_)
	{
		N::do_(old_, new_);
	}
};

typedef boost::mpl::vector<RemoteDisplay> revise_types;
typedef boost::mpl::vector<OsInfo, RuntimeOptions, GlobalNetwork,
		DeviceIndex<CVmHardDisk, PDE_HARD_DISK>, Cpu> untranslatable_types;

///////////////////////////////////////////////////////////////////////////////
// struct Repairer

template <class Types>
struct Repairer
: boost::mpl::fold<Types, void, Reviser<boost::mpl::_2, boost::mpl::_1> >
{
};

namespace Access
{
struct Work;
///////////////////////////////////////////////////////////////////////////////
// struct Base

struct Base
{
	Base();
	virtual ~Base();

	virtual PRL_RESULT load(Work& , bool ) = 0;
	virtual PRL_RESULT save(const Work& , bool , bool ) = 0;
	virtual PRL_RESULT restore(const Work& , const QString& ) = 0;
	virtual bool canRestore(const Work& ) const = 0;
	void forget(const Work& unit_);
protected:
	CacheBase<CVmConfiguration>& getCache() const
	{
		return *m_cache;
	}
private:
	QScopedPointer<CacheBase<CVmConfiguration> > m_cache;
};

///////////////////////////////////////////////////////////////////////////////
// struct InMemory

struct InMemory: Base
{
	PRL_RESULT load(Work& dst_, bool );
	PRL_RESULT save(const Work& unit_, bool , bool saveRelative_);
	PRL_RESULT restore(const Work& , const QString& )
	{
		return PRL_ERR_UNIMPLEMENTED;
	}
	bool canRestore(const Work& ) const
	{
		return false;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Mixed

struct Mixed: Base
{
	PRL_RESULT load(Work& dst_, bool direct_);
	PRL_RESULT save(const Work& src_, bool replace_, bool saveRelative_);
	PRL_RESULT restore(const Work& unit_, const QString& owner_);
	bool canRestore(const Work& unit_) const;
};

} // namespace Access
} // namespace Config
} // namespace Vm

// NB. In computer science, a trie, also called digital tree and sometimes
// radix tree or prefix tree (as they can be searched by prefixes), is an
// ordered tree data structure that is used to store a dynamic set or
// associative array where the keys are usually strings.
namespace Trie
{
struct Root;
} // namespace Trie

class CDspVmConfigManager
{
public:
	CDspVmConfigManager(void);
	virtual ~CDspVmConfigManager(void);

	/**
	* @brief Load config from disk.
	**/
	PRL_RESULT loadConfig( SmartPtr<CVmConfiguration>& pConfig,
							const QString& strFileName,
							SmartPtr<CDspClient> pUserSession,
							bool BNeedLoadAbsolutePath = true, // FIXME: Need replace to QFlags
							bool bLoadDirectlyFromDisk = false );
	/**
	* @brief Save config from disk.
	*/
	PRL_RESULT saveConfig( SmartPtr<CVmConfiguration> pConfig,
							const QString& config_file,
							SmartPtr<CDspClient> pUserSession,
							bool do_replace = true,
							bool BNeedToSaveRelativePath = true);

	/**
	* @brief Restore corrupted config file from backup config file
	*/
	PRL_RESULT restoreConfig(const QString& config_file,
							 SmartPtr<CDspClient> pUserSession,
							 const QString& owner);

	/**
	* @brief Check restoring ability
	*/
	bool canConfigRestore( const QString& config_file, SmartPtr<CDspClient> pUserSession );

	void removeFromCache( const QString& path );

	/**
	 * Helper lock functions for specific logic of working with VM configs
	 */
	SmartPtr<QWriteLocker> lockOnWrite();
	SmartPtr<QReadLocker> lockOnRead();

	/* Vm DiskDescriptor.xml config cache */
	class CHardDiskConfigCache
	{
	public:
		CHardDiskConfigCache();
		SmartPtr<CVmHardDisk> getConfig(const QString& sFileName,
				SmartPtr<CDspClient> pUserSession);
		void update(const QString& path, const SmartPtr<CVmHardDisk>& pConfig,
				SmartPtr<CDspClient> pUserSession);
		void remove(SmartPtr<CVmConfiguration> &pConfig);
		void remove(const QString& sFileName);
	private:
		SmartPtr< CacheBase<CVmHardDisk> >	m_HardDiskCache;
	};

	CHardDiskConfigCache &getHardDiskConfigCache()
	{
		return m_HardDiskCache;
	}

	PRL_RESULT adopt(const QString& path_, Vm::Config::Access::Base* access_);
private:
	// FIXME: May be need implement lock for every config exclude common lock
	// mutex for lock access from other thread
	QReadWriteLock	m_mtxAccessLocker;

	CHardDiskConfigCache m_HardDiskCache;
	QScopedPointer<Trie::Root> m_trie;
};

#endif // CDSP_VM_CONFIG_ACCESS_SYNCH_H
