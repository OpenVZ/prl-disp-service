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

#ifndef CDSP_VM_CONFIG_ACCESS_SYNCH_H
#define CDSP_VM_CONFIG_ACCESS_SYNCH_H

#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlsdk/PrlErrors.h>
#include <prlcommon/Std/SmartPtr.h>
#include "Dispatcher/Dispatcher/Cache/Cache.h"
#include <QReadWriteLock>
#include <QHash>
#include <QObject>
#include <QVector>
#include <QDateTime>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/utility/result_of.hpp>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

class CDspClient;
class CVmConfiguration;
class CVmHardDisk;
class CDspService;

namespace Vm
{
namespace Config
{

QString getVmHomeDirName(const QString& uuid_);

///////////////////////////////////////////////////////////////////////////////
// struct MemGuarantee

struct MemGuarantee
{
	explicit MemGuarantee(const CVmMemory& memory_)
	: m_type(memory_.getMemGuaranteeType()), m_guarantee(memory_.getMemGuarantee())
	{
	}
	MemGuarantee(quint32 guarantee_, PRL_MEMGUARANTEE_TYPE type_):
		m_type(type_), m_guarantee(guarantee_)
	{
	}

	quint64 operator()(quint64 ramsize_) const;
	PRL_MEMGUARANTEE_TYPE getType() const
	{
		return m_type;
	}

private:
	PRL_MEMGUARANTEE_TYPE m_type;
	quint32 m_guarantee;
};

///////////////////////////////////////////////////////////////////////////////
// struct Nvram

struct Nvram
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

namespace RemoteDisplay
{
///////////////////////////////////////////////////////////////////////////////
// struct Pivot

struct Pivot
{
	static void do_(CVmConfiguration& new_, const CVmConfiguration& old_);
};

} // namespace RemoteDisplay

namespace Index
{
///////////////////////////////////////////////////////////////////////////////
// struct Boot

template <PRL_DEVICE_TYPE D>
struct Boot
{
	typedef QList<CVmStartupOptions::CVmBootDevice*> bootList_type;

	explicit Boot(const bootList_type& bootList_): m_bootList(bootList_)
	{
	}

	void change(unsigned old_, unsigned new_)
	{
		bootList_type::iterator b = std::find_if(m_bootList.begin(), m_bootList.end(),
			boost::bind(&CVmStartupOptions::CVmBootDevice::getIndex, _1) == old_
			&& boost::bind(&CVmStartupOptions::CVmBootDevice::getType, _1) == D);

		if (b != m_bootList.end())
		{
			(*b)->setIndex(new_);
			m_bootList.erase(b);
		}
	}

private:
	bootList_type m_bootList;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pool

struct Pool
{
	typedef QVector<quint32> population_type;

	explicit Pool(const population_type& initial_);

	quint32 getAvailable();

private:
	population_type m_population;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

template <class T, PRL_DEVICE_TYPE D>
struct Device
{
	typedef QList<T*> list_type;
	typedef typename list_type::iterator iterator_type;
	typedef Boot<D> boot_type;
	typedef typename boot_type::bootList_type bootList_type;

	Device(const list_type& devices_, const bootList_type& bootList_)
	: m_devices(devices_), m_bootList(bootList_)
	{
	}

	void operator()(list_type& newDevices_)
	{
		QVector<quint32> a;
		std::transform(m_devices.begin(), m_devices.end(), std::back_inserter(a),
			boost::bind(&T::getIndex, _1));

		Pool p(a);
		boot_type boot(m_bootList);
		foreach(T* h, newDevices_)
		{
			quint32 o = h->getIndex();
			iterator_type it = findDevice(m_devices.begin(), m_devices.end(), h);
			if (m_devices.end() == it)
			{
				qint32 i = p.getAvailable();
				h->setIndex(i);
				h->setItemId(i);
			}
			else
			{
				h->setIndex((*it)->getIndex());
				h->setItemId((*it)->getItemId());
				// We shouldn't find the same device twice
				m_devices.erase(it);
			}

			boot.change(o, h->getIndex());
		}
		std::sort(newDevices_.begin(), newDevices_.end(), boost::bind(&T::getIndex, _1) <
				boost::bind(&T::getIndex, _2));
		m_devices = newDevices_;
	}

private:
	iterator_type findDevice(iterator_type begin_, iterator_type end_, const T* device_);

	list_type m_devices;
	bootList_type m_bootList;
};

} // namespace Index

namespace Patch
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

template<class T>
struct Builder
{
	typedef QList<T*> list_type;

	Builder(const list_type& source_, list_type& target_):
		m_source(source_), m_target(&target_)
	{
	}

	Builder& drawAlias();
	Builder& updateDisabled();
	Builder& updateConnected();
	Builder& updateDisconnected();

private:
	static bool guessConnected(const T& device_);

	const list_type m_source;
	list_type* m_target;
};

///////////////////////////////////////////////////////////////////////////////
// struct Runtime

struct Runtime
{
	static void do_(CVmConfiguration& new_, const CVmConfiguration& old_);
	static void drawAliases(CVmConfiguration& new_, const CVmConfiguration& old_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Index

struct Index
{
	static void do_(CVmConfiguration& new_, const CVmConfiguration& old_);
};

///////////////////////////////////////////////////////////////////////////////
// struct State

struct State
{
	static void do_(CVmConfiguration& new_, const CVmConfiguration& old_);
};

} // namespace Patch

///////////////////////////////////////////////////////////////////////////////
// struct OsInfo

struct OsInfo
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

///////////////////////////////////////////////////////////////////////////////
// struct RuntimeOptions

struct RuntimeOptions
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

///////////////////////////////////////////////////////////////////////////////
// struct GlobalNetwork

struct GlobalNetwork
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

namespace Cpu
{

///////////////////////////////////////////////////////////////////////////////
// struct Copy

struct Copy
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

} // namespace Cpu

struct Identification
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct HardDisks
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct OpticalDisks
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct NetworkDevices
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct MemoryOptions
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct HighAvailability
{
	static void do_(CVmConfiguration& old_, const CVmConfiguration& new_);
};

struct Tools
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

typedef boost::mpl::vector<Nvram, Patch::Runtime> revise_types;
typedef boost::mpl::vector<Identification, OsInfo, RuntimeOptions, GlobalNetwork,
		Patch::Index, Cpu::Copy, NetworkDevices, HardDisks, OpticalDisks, Patch::State,
		MemoryOptions, HighAvailability, Tools, RemoteDisplay::Pivot>
		untranslatable_types;

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

namespace Edit
{
///////////////////////////////////////////////////////////////////////////////
// struct Unlocked

struct Unlocked
{
	typedef boost::function1<Prl::Expected<void, PRL_RESULT>, CVmConfiguration& >
		action_type;

	Unlocked(const QString& uuid_, const SmartPtr<CDspClient>& actor_,
		CDspService& service_):
		m_uuid(uuid_), m_service(&service_), m_actor(actor_)
	{
	}

	PRL_RESULT operator()(const action_type& action_);

	const QString& getUuid() const
	{
		return m_uuid;
	}
	const SmartPtr<CDspClient> getActor() const
	{
		return m_actor;
	}

private:
	QString m_uuid;
	CDspService* m_service;
	SmartPtr<CDspClient> m_actor;
};

///////////////////////////////////////////////////////////////////////////////
// class Wrapper

template<class T, class Enabled = void>
struct Wrapper
{
	Unlocked::action_type::result_type
		operator()(const T& action_, Unlocked::action_type::argument_type dst_) const
	{
		(void)action_(dst_);
		return Unlocked::action_type::result_type();
	}
};

template<class T>
struct Wrapper<T, typename boost::enable_if<boost::is_same<PRL_RESULT,
	typename boost::result_of<T(Unlocked::action_type::argument_type )>::type> >::type>
{
	Unlocked::action_type::result_type operator()
		(const T& action_, Unlocked::action_type::argument_type dst_) const
	{
		PRL_RESULT e = action_(dst_);
		if (PRL_FAILED(e))
			return e;

		return Unlocked::action_type::result_type();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Atomic

struct Atomic
{
	typedef Unlocked::action_type action_type;

	Atomic(const QString& uuid_, const SmartPtr<CDspClient>& actor_,
		CDspService& service_):
		m_decorated(uuid_, actor_, service_), m_service(&service_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& action_)
	{
		action_type a = boost::bind<action_type::result_type>
			(Wrapper<T>(), boost::cref(action_), _1);
		return (*this)(a);
	}
	PRL_RESULT operator()(const action_type& action_);

private:
	Unlocked m_decorated;
	CDspService* m_service;
};

} // namespace Edit
} // namespace Config

namespace Registration
{
namespace Unattended
{
///////////////////////////////////////////////////////////////////////////////
// struct Survey

struct Survey
{
	explicit Survey(const CVmConfiguration& model_): m_model(&model_)
	{
	}

	quint32 getGuestType() const;
	const CVmFloppyDisk* find(const QString& image_) const;
	quint32 getNextItemId() const;
	quint32 getNextIndex() const;
	quint32 getNextStackIndex() const;

private:
	const QList<CVmFloppyDisk* >& getList() const;

	const CVmConfiguration* m_model;
};

} // namespace Unattended

///////////////////////////////////////////////////////////////////////////////
// struct Reconfiguration

struct Reconfiguration: QObject
{
	Q_OBJECT

public:
	explicit Reconfiguration(CDspService& service_): m_service(&service_)
	{
	}

public slots:
	void react(QString directory_, QString uuid_);

private:
	CDspService* m_service;
};

} // namespace Registration
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

	template <typename T>
	PRL_RESULT modifyConfig(const QString& path,
				SmartPtr<CDspClient> user,
				T modifier,
				bool loadAbsolutePath,
				bool loadDirectlyFromDisk,
				bool saveRelativePath,
				bool replace)
	{
		SmartPtr<CVmConfiguration> conf(new CVmConfiguration());
		PRL_RESULT res = loadConfig(conf, path, user, loadAbsolutePath, loadDirectlyFromDisk);
		if (PRL_FAILED(res))
		{
			WRITE_TRACE(DBG_FATAL, "Can't parse VM config file '%s': %s",
				QSTR2UTF8(path), PRL_RESULT_TO_STRING(res));
			return res;
		}
		if (!modifier(conf))
			return PRL_ERR_SUCCESS;
		res = saveConfig(conf, path, user, replace, saveRelativePath);
		if (PRL_FAILED(res))
		{
			WRITE_TRACE(DBG_FATAL, "Can't save VM config to '%s': %s",
				QSTR2UTF8(path), PRL_RESULT_TO_STRING(res));
		}
		return res;
	}

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
