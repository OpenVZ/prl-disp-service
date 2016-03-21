///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatCollectingThread.h
///
/// Collecting host system statistics thread implementation
///
/// @author sandro
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

#ifndef _DSP_STAT_COLLECTING_THREAD_H_
#define _DSP_STAT_COLLECTING_THREAD_H_

#include <QThread>
#include <QMutex>
#include <QSet>
#include <QMap>
#include "CDspStatCollector.h"
#include "CDspClient.h"
#include <prlcommon/Std/SmartPtr.h>
#include "CDspVmStateSender.h"
#include "CDspVm.h"

namespace Registry
{
struct Public;
} // namespace Registry

namespace Stat
{
struct Perf;
struct Storage;

namespace Collecting
{
namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// struct Farmer

class Farmer: public QObject
{
	Q_OBJECT
public:
	explicit Farmer(const CVmIdent& ident_);

public slots:
	void reset();
	void handle(unsigned state_, QString uuid_, QString dir_, bool flag_);

protected:
	void timerEvent(QTimerEvent* event_);

private:
	void collect();

	int m_timer;
	quint64 m_period;
	CVmIdent m_ident;
	QScopedPointer<QFutureWatcher<void> > m_watcher;
};

} //namespace Ct

///////////////////////////////////////////////////////////////////////////////
// struct Mapper

class Mapper: public QObject
{
	Q_OBJECT

	typedef CDspLockedPointer<CDspVmStateSender> sender_type;

public slots:
	void abort(CVmIdent ident_);
	void begin(CVmIdent ident_);
};

} // namespace Collecting
} // namespace Stat

/**
 * Thread that collecting host system resources usage statistics by corresponding interval
 */
class CDspStatCollectingThread : public QThread
{
	Q_OBJECT

public:
	typedef std::multimap<CVmIdent, SmartPtr<CDspClient> >
			VmStatisticsSubscribersMap;

	static void stop();
	static void start(Registry::Public& registry_);

public://Subscribers list maintaining calls

	/**
	 * Registries specified user at host statistics subscribers list
	 * @param pointer to the user session object
	 */
	static void SubscribeToHostStatistics(const SmartPtr<CDspClient> &pUser);
	/**
	 * Unregistries specified user from host statistics subscribers list
	 * @param pointer to the user session object
	 */
	static void UnsubscribeFromHostStatistics(const SmartPtr<CDspClient> &pUser);
	/**
	 * Registries specified user at host statistics subscribers list
	 * @param subscribing VM uuid
	 * @param pointer to the user session object
	 */
	static PRL_RESULT SubscribeToVmGuestStatistics(const QString &sVmUuid, const SmartPtr<CDspClient> &pUser);
	/**
	 * Unregistries specified user from host statistics subscribers list
	 * @param unsubscribing VM uuid
	 * @param pointer to the user session object
	 */
	static PRL_RESULT UnsubscribeFromVmGuestStatistics(const QString &sVmUuid, const SmartPtr<CDspClient> &pUser);

	static void ClientDisconnected(const SmartPtr<CDspClient> &pUser) ;

    static void ProcessPerfStatsCommand(const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage>& p) ;

	/**
	* Get host statistics
	* When statistic is not available register request in hash to send automatically
	* @param pointer to the user session object
	* @param pointer to request package object
	*/
	static void SendHostStatistics(SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage>& p);

	/**
	* Get VM statistics
	* When statistic is not available register request in hash to send automatically
	* @param unsubscribing VM uuid
	* @param pointer to the user session object
	* @param pointer to request package object
	*/
	static void SendVmGuestStatistics(const QString &sVmUuid
		, SmartPtr<CDspClient> &pUser
		, const SmartPtr<IOPackage>& p );

    /**
	 * Cleanups host statistics subscribers list
	 */
	static void CleanupSubscribersLists();

	static QWeakPointer<Stat::Storage> getStorage(const QString& uuid_);

protected:
	/** Overridden method of thread working body */
	void run();
	Q_INVOKABLE void timerEvent(QTimerEvent* event_);

private:
	/** Sign whether thread need to finalize it's work */
	bool m_bFinalizeWorkNow;
	/** Pointer to object of previous CPUs statistics result */
	SmartPtr<CCpusStatInfo> m_pCpusStatInfo;
	/** Pointer to platform dependent statistics collector object */
	SmartPtr<CDspStatCollector> m_pStatCollector;
	/** Latency of stat collecting loop iteration **/
	quint32 m_nDeltaMs;

private://Statistics collecting helpers set
	/** Processes CPUs statistics */
	void ProcessCpuStat();
	/** Processes disks statistics */
	void ProcessDisksStat();
	/** Processes RAM statistics */
	void ProcessRamStat();
	/** Processes swap statistics */
	void ProcessSwapStat();
	/** Processes uptime statistics */
	void ProcessUptimeStat();
	/** Processes processes statistics */
	void ProcessProcessesStat();
	/** Processes network interfaces statistics */
	void ProcessNetInfosStat();
	/** Processes users sessions statistics */
	void ProcessUsersSessionsStat();

private://Static data members
	/** stat collecting loop successively iteration counter**/
	static quint64 g_uSuccessivelyCyclesCounter;

	/** Subscribers list access synchronization object */
	static QMutex *g_pSubscribersMutex;
    typedef QSet<SmartPtr<CDspClient> > CDspClientPtrSet ;
	/** Host statistics receiving subscribers list */
	static CDspClientPtrSet *g_pHostStatisticsSubscribers;

	/** VMs guest OSes statistics receiving subscribers list */
	static VmStatisticsSubscribersMap *g_pVmsGuestStatisticsSubscribers;
	static Stat::Perf *g_pPerfStatsSubscribers ;

	struct StatGetter
	{
		SmartPtr<CDspClient> pUser;
		SmartPtr<IOPackage>  pPkg;

		StatGetter( SmartPtr<CDspClient> u, SmartPtr<IOPackage> p )
			:pUser( u ),  pPkg( p ) {}
	};
	typedef QList< StatGetter > CHostStatGettersList;
	static CHostStatGettersList *g_pHostStatGetters;

	typedef QMap<QPair<QString, QString>, QList<StatGetter> >  CVmStatGettersMap;
	static CVmStatGettersMap *g_pVmStatGetters;

	/** Internal object data access synchronization object */
	static QMutex* s_instanceMutex;
	static CDspStatCollectingThread* s_instance;

public://Custom counters callbacks sets

signals:
	void abort(CVmIdent ident_);
	void begin(CVmIdent ident_);

private:

	bool ExistStatSubscribers();
	bool ExistPerfCountersSubscribers();
	static bool IsAvalableStatisctic();

	/**
	 * Notifies host statistics subscribers with specified updated host statistics event
	 * @param pointer to notification event object
	 */
	void NotifyHostStatisticsSubscribers();
	/**
	 * Notifies VMs statistics subscribers with updated VMs statistics
	 */
	void NotifyVmsStatisticsSubscribers();

    static PRL_RESULT SubscribeToPerfStats(const SmartPtr<CDspClient> &pUser,
                                           const QString &Filter,
                                           const QString &sVmUuid) ;
    static PRL_RESULT UnsubscribeFromPerfStats(const SmartPtr<CDspClient> &pUser, const QString &sVmUuid) ;
    static void SendPerfStatsRequest(const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage>& pkg,
                                     const QString &sFilter, const QString &sVmUuid) ;
    static SmartPtr<CVmEvent> GetPerformanceStatistics(const CVmIdent &vm_ident, const QString &sFilter) ;

	static QString GetHostStatistics();
	static SmartPtr<CSystemStatistics> GetVmGuestStatistics( const QString &sVmUuid, const QString &sVmDirUuid );
	static void SendStatisticsResponse(SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage>& p,
		const QString& statAsString );
	static void schedule();

	explicit CDspStatCollectingThread(Registry::Public& registry_);

	int m_timer;
	QDateTime m_last;

	Registry::Public& m_registry;
};

#endif//_DSP_STAT_COLLECTING_THREAD_H_
