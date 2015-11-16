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

#include <prlsdk/PrlErrors.h>
#include "Libraries/Std/SmartPtr.h"
#include "Dispatcher/Dispatcher/Cache/Cache.h"
#include <QReadWriteLock>
#include <QHash>
#include <QDateTime>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/fold.hpp>

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
typedef boost::mpl::vector<OsInfo, RuntimeOptions, GlobalNetwork> untranslatable_types;

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
