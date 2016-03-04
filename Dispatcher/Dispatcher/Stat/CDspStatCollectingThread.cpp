///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatCollectingThread.cpp
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

#include "CDspStatCollectingThread.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmBinaryEventParameter.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include "CDspClient.h"
#include "CDspVm.h"
#include "CDspService.h"
#include "CDspCommon.h"
#include "CDspVmStateSender.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include <prlsdk/PrlPerfCounters.h>
#include <prlsdk/PrlIOStructs.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/Std/BitOps.h>

#include <QDateTime>
#include <QRegExp>
#include <numeric>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#ifdef _WIN_
	#include <process.h>
	#define getpid _getpid
#endif

#include "CDspVzHelper.h"

/**
 * Since statistics module consumes a lot of CPU as described in
 * bug #3702 http://bugzilla.parallels.com/show_bug.cgi?id=3702
 * Extending the collect delay to reduce CPU usage.
 */
#define STAT_COLLECTING_TIMEOUT 1000

using namespace Parallels;

namespace {

///////////////////////////////////////////////////////////////////////////////
// struct DAO

template <typename Entity>
struct DAO
{
	Entity get(const QString &uuid) const;
	void set(const QString &uuid, const Entity &e) const;

private:
	static QMutex s_mutex;
	static QMap<QString, Entity> s_map;
};

template <typename Entity>
QMutex DAO<Entity>::s_mutex;
template <typename Entity>
QMap<QString, Entity> DAO<Entity>::s_map;

template <typename Entity>
Entity DAO<Entity>::get(const QString &uuid) const
{
	QMutexLocker guard(&s_mutex);
	return s_map[uuid];
}

template <typename Entity>
void DAO<Entity>::set(const QString &uuid, const Entity &e) const
{
	QMutexLocker guard(&s_mutex);
	s_map[uuid] = e;
}

} // anonymous namespace

namespace Stat
{
namespace Collecting
{

namespace Ct {

static DAO<QList< ::Ct::Statistics::Filesystem> > s_dao;

///////////////////////////////////////////////////////////////////////////////
// struct Farmer

Farmer::Farmer(const CVmIdent& ident_):
	m_timer(startTimer(0)), m_period(STAT_COLLECTING_TIMEOUT), m_ident(ident_)
{
	qint64 p = CDspService::instance()->getDispConfigGuard()
		.getDispWorkSpacePrefs()->getVmGuestCollectPeriod() * 1000;
	m_period = qMax(p, qint64(m_period));
}

void Farmer::reset()
{
	if (m_watcher)
	{
		m_watcher->disconnect(this, SLOT(finish()));
		m_watcher->waitForFinished();
		m_watcher.reset();
	}
}

void Farmer::handle(unsigned state_, QString uuid_, QString dir_, bool flag_)
{
	Q_UNUSED(flag_);
	if (uuid_ != m_ident.first || m_ident.second != dir_)
		return;

	if (VMS_RUNNING != state_)
		reset();
	else if (m_watcher)
		return;

	if (0 != m_timer)
		killTimer(m_timer);

	m_timer = VMS_RUNNING == state_ ? startTimer(0) : 0;
}

void Farmer::collect()
{
	const QString& u = m_ident.first;
	QList< ::Ct::Statistics::Filesystem> f;
	CVzHelper::get_env_fstat(u, f);
	s_dao.set(u, f);
}

void Farmer::finish()
{
	reset();
}

void Farmer::timerEvent(QTimerEvent *event_)
{
	killTimer(event_->timerId());
	if (!m_watcher)
	{
		m_watcher.reset(new QFutureWatcher<void>);
		this->connect(m_watcher.data(), SIGNAL(finished()), SLOT(finish()));
		m_watcher->setFuture(QtConcurrent::run(this, &Farmer::collect));
	}
	m_timer = startTimer(m_period);
}

} // namespace Ct

namespace Vm {

///////////////////////////////////////////////////////////////////////////////
// struct Farmer

Farmer::Farmer(const CVmIdent& ident_):
	m_timer(startTimer(0)), m_period(STAT_COLLECTING_TIMEOUT), m_ident(ident_)
{
	SmartPtr<Parallels::CProtoCommand> c(new Parallels::CProtoBasicVmGuestCommand(
						PVE::DspCmdCtlVmCollectGuestUsage,
						m_ident.first, QString()));
	m_request = Parallels::DispatcherPackage::createInstance(
						c->GetCommandId(),
						c->GetCommand()->toString(),
						SmartPtr<IOPackage>());
	qint64 p = CDspService::instance()->getDispConfigGuard()
			.getDispWorkSpacePrefs()->getVmGuestCollectPeriod() * 1000;
	m_period = qMax(p, qint64(m_period));
}

void Farmer::reset()
{
	m_vm = IOSender::InvalidHandle;
	m_pending = IOSendJob::Handle();
	IOServerPool& p = CDspService::instance()->getIOServer();
	p.disconnect(this, SLOT(disconnect(IOSender::Handle)));
	p.disconnect(this, SLOT(finish(IOSender::Handle, IOSendJob::Handle,
					const SmartPtr<IOPackage>)));
}

void Farmer::finish(IOSender::Handle handle_, IOSendJob::Handle job_,
			const SmartPtr<IOPackage> package_)
{
	Q_UNUSED(handle_);
	if (job_ != m_pending)
		return;

	reset();
	CVmEvent v(UTF8_2QSTR(package_->buffers[0].getImpl()));
	if (PRL_SUCCEEDED(v.getEventCode()))
	{
		// say something.
	}
}

void Farmer::disconnect(IOSender::Handle handle_)
{
	if (IOSender::InvalidHandle == handle_ || handle_ != m_vm)
		return;

	reset();
	if (0 != m_timer)
	{
		killTimer(m_timer);
		m_timer = 0;
	}
}

void Farmer::handle(unsigned state_, QString uuid_, QString dir_, bool flag_)
{
	Q_UNUSED(flag_);
	if (uuid_ != m_ident.first || m_ident.second != dir_)
		return;

	if (VMS_RUNNING != state_)
		reset();
	else if (m_pending.isValid())
		return;

	if (0 != m_timer)
		killTimer(m_timer);

	m_timer = VMS_RUNNING == state_ ? startTimer(0) : 0;
}

void Farmer::timerEvent(QTimerEvent* event_)
{
	killTimer(event_->timerId());
	m_timer = 0;
	IOServerPool& p = CDspService::instance()->getIOServer();
	if (m_pending.isValid())
	{
		IOSendJob::Result r = p.getSendResult(m_pending);
		if (IOSendJob::SendPended != r && r != IOSendJob::Success)
			reset();
	}
	do
	{
		if (m_pending.isValid())
			break;

		PRL_VM_TOOLS_STATE s = CDspVm::getVmToolsState(m_ident);
		if (PTS_INSTALLED != s)
			break;
		SmartPtr<CDspVm> v = CDspVm::GetVmInstanceByUuid(m_ident);
		if (!v.isValid())
			return;
//		m_vm = v->getVmConnectionHandle();
		if (IOSender::InvalidHandle == m_vm)
			return;
		this->connect(&p,
			SIGNAL(onResponsePackageReceived(IOSender::Handle,
							IOSendJob::Handle,
							const SmartPtr<IOPackage>)),
			SLOT(finish(IOSender::Handle, IOSendJob::Handle,
				const SmartPtr<IOPackage>)));
		this->connect(&p,
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			SLOT(disconnect(IOSender::Handle)));

//		m_pending = v->sendPackageToVm(m_request);
//		if (!m_pending.isValid())
//			return reset();
	} while(false);
	m_timer = startTimer(m_period);
}

} // namespace Vm

///////////////////////////////////////////////////////////////////////////////
// struct Mapper

void Mapper::abort(const CVmIdent& ident_)
{
	QString k = QString(ident_.first).append(ident_.second);
	QObject* f = findChild<QObject* >(k);
	if (NULL == f)
		return;
	f->setParent(NULL);
	f->deleteLater();
	typedef CDspLockedPointer<CDspVmStateSender> sender_type;
	sender_type s = CDspService::instance()->getVmStateSender();
	if (s.isValid())
		s->disconnect(f, SLOT(handle(unsigned, QString, QString, bool)));
}

void Mapper::begin(const CVmIdent& ident_)
{
	QString k = QString(ident_.first).append(ident_.second);
	QObject* f = findChild<QObject* >(k);
	if (NULL != f)
		return;

	PRL_VM_TYPE t = PVT_VM;
	if (CDspService::instance()->getVmDirManager().getVmTypeByUuid(ident_.first, t))
		f = new Ct::Farmer(ident_);
	else
		f = new Vm::Farmer(ident_);
	f->setObjectName(k);
	f->setParent(this);
	typedef CDspLockedPointer<CDspVmStateSender> sender_type;
	sender_type s = CDspService::instance()->getVmStateSender();
	if (s.isValid())
	{
		f->connect(s.getPtr(),
			SIGNAL(signalSendVmStateChanged(unsigned, QString, QString, bool)),
			SLOT(handle(unsigned, QString, QString, bool)));
	}
}

} // namespace Collecting

typedef std::multimap<QPair<CVmIdent, QString>, SmartPtr<CDspClient> > perfList_type;
} // namespace Stat

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Indicator

struct Indicator
{
	typedef CDspStatCollectingThread::VmStatisticsSubscribersMap
		vmSequence_type;
	typedef Stat::perfList_type perfSequence_type;

	Indicator(const CVmIdent& ident_, SmartPtr<CDspClient> user_):
		m_ident(&ident_), m_user(user_)
	{
	}

	bool operator()(vmSequence_type::const_reference entry_) const
	{
		return entry_.first == *m_ident && entry_.second == m_user;
	}
	bool operator()(perfSequence_type::const_reference entry_) const
	{
		return entry_.first.first == *m_ident && entry_.second == m_user;
	}
private:
	const CVmIdent* m_ident;
	SmartPtr<CDspClient> m_user;
};

int getCounterValueCallback(counters_storage_t *, counter_t *counter, void *data)
{
	QPair<QString, quint64> *d = (QPair<QString, quint64>*)data;

	// skip prefix (@I or @A)
	const char *n = counter->name + sizeof(PERF_COUNT_TYPE_INC) - 1;
	if (d->first == n) {
		d->second = PERF_COUNT_ATOMIC_GET(counter);
		return ENUM_BREAK;
	}

	return ENUM_CONTINUE;
}

quint64 GetPerfCounter(const ProcPerfStoragesContainer &c, const QString &name)
{
	QPair<QString, quint64> d = qMakePair(name, 0ULL);
	c.enum_counters(getCounterValueCallback, (void*)&d);
	return d.second;
}

SmartPtr<IOPackage> create_binary_package(CVmEvent &event)
{
	QByteArray _byte_array ;
	QBuffer _buffer(&_byte_array) ;
	bool bRes = _buffer.open(QIODevice::ReadWrite) ;
	PRL_ASSERT(bRes) ;
	if (!bRes)
	{
		LOG_MESSAGE(DBG_FATAL, "Fatal error - couldn't to open binary data buffer for read/write") ;
		return SmartPtr<IOPackage>() ;
	}
	QDataStream _data_stream(&_buffer) ;
	_data_stream.setVersion(QDataStream::Qt_4_0) ;
	event.Serialize(_data_stream) ;
	_buffer.reset() ;
	return DispatcherPackage::createInstance(PVE::DspVmBinaryEvent, _data_stream, _byte_array.size()) ;
}

///////////////////////////////////////////////////////////////////////////////
// struct Usage

struct Usage
{
	Usage() : m_deltaTime(0), m_deltaValue(0)
	{
	}

	Usage(quint64 dTime, quint64 dValue) :
		m_deltaTime(dTime),
		m_deltaValue(dValue)
	{
	}

	quint32 getPercent() const
	{
		if (0 == m_deltaTime)
			return 0;

		quint32 x = qRound(100. * qreal(m_deltaValue) / qreal(m_deltaTime));
		return qMin(x, 100U);
	}
	quint64 getTimeDelta() const
	{
		return m_deltaTime;
	}
	quint64 getValueDelta() const
	{
		return m_deltaValue;
	}
private:
	quint64 m_deltaTime;
	quint64 m_deltaValue;
};

///////////////////////////////////////////////////////////////////////////////
// struct Meter

struct Meter
{
	void record(quint64 t, quint64 v);
	Usage report() const;

private:

	QPair<quint64, quint64> m_time;
	QPair<quint64, quint64> m_value;
};

void Meter::record(quint64 t, quint64 v)
{
	m_time = qMakePair(m_time.second, t);
	m_value = qMakePair(m_value.second, v > 0 ? v : m_value.second);
}

Usage Meter::report() const
{
	return Usage(m_time.second - m_time.first,
			m_value.second - m_value.first);
}

quint32 getHostCpus()
{
	static quint32 cpus = 0;
	if (0 < cpus)
		return cpus;
	CDspLockedPointer<CDspHostInfo> p = CDspService::instance()->getHostInfo();
	if (!p.isValid())
		return 0;
	CHostHardwareInfo* d = p->data();
	if (NULL == d)
		return 0;

	CHwCpu* c = d->getCpu();
	if (NULL == c)
		return 0;

	return cpus = c->getNumber();
}

namespace Conversion
{

///////////////////////////////////////////////////////////////////////////////
// struct Uint32

struct Uint32 {
	static CVmEventParameter *convert(quint32 v)
	{
		return new CVmEventParameter(PVE::UnsignedInt, QString::number(v));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Uint64

struct Uint64 {
	static CVmEventParameter *convert(quint64 v)
	{
		return new CVmEventParameter(PVE::UInt64, QString::number(v));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Network

struct Network {
	static CVmBinaryEventParameter *convert(const PRL_STAT_NET_TRAFFIC &stat)
	{
		CVmBinaryEventParameter *p = new CVmBinaryEventParameter();
		int sz = p->getBinaryDataStream()->writeRawData((const char*)&stat, sizeof(stat));
		PRL_ASSERT(sizeof(stat) == sz);
		return p;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct String

struct String {
	static CVmEventParameter *convert(const QString& s_)
	{
		return new CVmEventParameter(PVE::String, s_);
	}
};

} // namespace Conversion

bool isContainer(const QString &uuid)
{
	PRL_VM_TYPE t = PVT_VM;

	if (!CDspService::instance()->getVmDirManager().getVmTypeByUuid(uuid, t))
		return false;

	return t == PVT_CT;
}

///////////////////////////////////////////////////////////////////////////////
// struct SingleCounter

template <typename Flavor, typename Conversion>
struct SingleCounter {

	typedef typename Flavor::source_type source_type;
	typedef typename Flavor::value_type value_type;

	explicit SingleCounter(const source_type &source)
		: m_source(&source)
	{
	}

	const char* getName() const
	{
		return Flavor::getName();
	}

	CVmEventParameter *getParam() const
	{
		return Conversion::convert(getValue());
	}

	value_type getValue() const
	{
		return Flavor::extract(*m_source);
	}

private:

	const source_type * const m_source;
};

namespace Names {

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T>
struct Traits
{
	static QString getExternal(const T& t_)
	{
		return t_();
	}

	static QString getInternal(const T& t_)
	{
		return t_();
	}
};

namespace Filesystem {

///////////////////////////////////////////////////////////////////////////////
// struct Name

template <typename Leaf>
struct Name
{
	explicit Name(unsigned index_) : m_index(index_)
	{
	}

	QString operator()() const
	{
		return QString("fs%1.%2").arg(m_index).arg(Leaf::getName());
	}

private:
	unsigned m_index;
};

///////////////////////////////////////////////////////////////////////////////
// struct Total

struct Total
{
	static QString getName()
	{
		return "total";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Free

struct Free
{
	static QString getName()
	{
		return "free";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device
{
	static QString getName()
	{
		return "name";
	}
};

namespace Disk {

///////////////////////////////////////////////////////////////////////////////
// struct Name

struct Name
{
	Name(unsigned filesystem_, unsigned disk_)
		: m_name(filesystem_), m_index(disk_)
	{
	}

	QString operator()() const
	{
		return QString("%1.%2").arg(m_name()).arg(m_index);
	}

private:
	struct Tag
	{
		static QString getName()
		{
			return "disk";
		}
	};

	Filesystem::Name<Tag> m_name;
	unsigned m_index;
};

///////////////////////////////////////////////////////////////////////////////
// struct Count

struct Count
{
	static QString getName()
	{
		return "disk.count";
	}
};

} // namespace Disk

template<class T>
struct Traits
{
	static QString getExternal(const T& t_)
	{
		return t_().prepend("guest.");
	}

	static QString getInternal(const T& t_)
	{
		return t_();
	}
};

} // namespace Filesystem

template <class T>
struct Traits<Filesystem::Name<T> > : Filesystem::Traits<Filesystem::Name<T> >
{
};

template<>
struct Traits<Filesystem::Disk::Name> : Filesystem::Traits<Filesystem::Disk::Name>
{
};

} // namespace Names

} // end of namespace

QString multiCounterName(const char *prefix, quint32 i, const char *suffix)
{
	return QString("%1%2.%3").arg(prefix).arg(i).arg(suffix);
}

namespace Stat
{
///////////////////////////////////////////////////////////////////////////////
// struct Perf

struct Perf: private perfList_type
{
	using perfList_type::empty;

	bool add(const CVmIdent& vm_, const QString& filter_, const mapped_type& user_)
	{
		iterator p = insert(std::make_pair(qMakePair(vm_, filter_), user_));
		return end() != p;
	}
	void remove(const CVmIdent& vm_, const mapped_type& user_);
	bool has(const CVmIdent& vm_) const
	{
		if (!IsValidVmIdent(vm_))
			return true;

		const_iterator p = lower_bound(qMakePair(vm_, QString()));
		return end() != p && p->first.first == vm_;
	}
	QList<CVmIdent> select(const mapped_type& user_) const;
	template<class P>
	void report(P provider_);
};

void Perf::remove(const CVmIdent& vm_, const mapped_type& user_)
{
	iterator p = lower_bound(qMakePair(vm_, QString()));
	p = std::find_if(p, end(), Indicator(vm_, user_));
	if (end() != p)
		perfList_type::erase(p);
}

template<class P>
void Perf::report(P provider_)
{
	key_type k;
	SmartPtr<IOPackage> g;
	for (const_iterator p = begin(), e = end(); p != e; ++p)
	{
		if (p->first != k)
		{
			g.reset();
			k = p->first;
			SmartPtr<CVmEvent> event(provider_(k.first, k.second));
			if (event->m_lstEventParameters.isEmpty() || PRL_FAILED(event->getEventCode()))
			{
				// nothing to send
				continue;
			}
			g = create_binary_package(*event.getImpl()) ;
			if (!g.isValid())
			{
				LOG_MESSAGE(DBG_FATAL, "Failed to create an IOPackage");
				return;
			}
		}
		if (g.isValid())
			p->second->sendPackage(g);
	}
}

QList<CVmIdent> Perf::select(const mapped_type& user_) const
{
	QList<CVmIdent> output;
	for (const_iterator p = begin(), e = end(); p != e; ++p)
	{
		if (p->second == user_)
			output.append(p->first.first);
	}
	return output;
}

} // namespace Stat

namespace Vm
{
namespace
{

namespace Counter
{

///////////////////////////////////////////////////////////////////////////////
// struct VCpu

struct VCpu {

	explicit VCpu(const ProcPerfStoragesContainer &storage);

	quint64 getValue(quint32 index) const;
	void recordTime(Meter &m, quint64 v) const;
	void recordTsc(Meter &m, const CVmConfiguration *config) const;

private:
	const ProcPerfStoragesContainer *m_storage;
};

VCpu::VCpu(const ProcPerfStoragesContainer &storage)
	: m_storage(&storage)
{
}

quint64 VCpu::getValue(quint32 index) const
{
	return GetPerfCounter(*m_storage,
			multiCounterName("kernel.activity.vcpu", index, "tsc_guest"));
}

void VCpu::recordTime(Meter &m, quint64 v) const
{
	const quint64 t = (PrlGetTickCount64() * 1000) / PrlGetTicksPerSecond();
	m.record(t, v);
}

void VCpu::recordTsc(Meter &m, const CVmConfiguration *config) const
{
	quint64 v = 0;
	const quint32 k = getHostCpus();
	if (k > 0 && config != NULL) {
		quint32 n = config->getVmHardwareList()->getCpu()->getNumber();
		for (quint32 i = 0; i < n; ++i)
			v += getValue(i);
		v /= k;
	}

	m.record(HostUtils::GetTsc(), v);
}


///////////////////////////////////////////////////////////////////////////////
// struct VCpuTime

struct VCpuTime {

	VCpuTime(const VCpu &vcpu, quint32 index)
		: m_vcpu(&vcpu), m_index(index)
	{
	}

	QString getName() const
	{
		return multiCounterName("guest.vcpu", m_index, "time");
	}

	quint64 getValue() const
	{
		/* typedef unsigned int uint128_t __attribute__((mode(TI))); */

		// FIXME commented as getTscHz is not provided
		// convert from TSC to nanosec
		/* quint64 ns = ((uint128_t) m_vcpu->getValue(m_index)) * 1000000000ULL / CDspHostInfo::GetTscHz(); */
		return m_vcpu->getValue(m_index);
	}

	CVmEventParameter *getParam() const
	{
		return Conversion::Uint64::convert(getValue());
	}

private:
	const VCpu *m_vcpu;
	quint32 m_index;
};

namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct GuestUsage

struct GuestUsage {

	typedef const Meter source_type;
	typedef quint32 value_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_USAGE_PTRN;
	}

	static value_type extract(source_type &m)
	{
		return m.report().getPercent();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestTimeDelta

struct GuestTimeDelta {

	typedef const Meter source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_TIME_PTRN;
	}

	static value_type extract(source_type &m)
	{
		return m.report().getValueDelta();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct HostTimeDelta

struct HostTimeDelta {

	typedef const Meter source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_HOST_CPU_TIME_PTRN;
	}

	static value_type extract(source_type &m)
	{
		return m.report().getTimeDelta();
	}
};

} //namespace Flavor

typedef SingleCounter<Flavor::GuestUsage, Conversion::Uint32> GuestUsage;
typedef SingleCounter<Flavor::GuestTimeDelta, Conversion::Uint64> GuestTimeDelta;
typedef SingleCounter<Flavor::HostTimeDelta, Conversion::Uint64> HostTimeDelta;

namespace Memory
{
namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Used

struct Used {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_USAGE_PTRN;
	}

	static value_type extract(source_type &c)
	{
		// kb to bytes
		return GetPerfCounter(c, "mem.guest_used") << 10;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Cached

struct Cached {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_CACHED_PTRN;
	}

	static value_type extract(source_type &c)
	{
		// kb to bytes
		return GetPerfCounter(c, "mem.guest_cached") << 10;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Total

struct Total {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_TOTAL_PTRN;
	}

	static value_type extract(source_type &c)
	{
		// kb to bytes
		return GetPerfCounter(c, "mem.guest_total") << 10;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct BalloonActual

struct BalloonActual {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_BALLOON_ACTUAL;
	}

	static value_type extract(source_type &c)
	{
		// pages to bytes
		return GetPerfCounter(c, "kernel.ws.balloon_size") << 12;
	}
};
} // namespace Flavor

///////////////////////////////////////////////////////////////////////////////
// struct MemoryConversion

struct MemoryConversion {
	static CVmEventParameter *convert(quint64 v)
	{
		// bytes to Mb
		return Conversion::Uint32::convert(v >> 20);
	}
};

typedef SingleCounter<Flavor::Used, MemoryConversion> Used;
typedef SingleCounter<Flavor::Cached, MemoryConversion> Cached;
typedef SingleCounter<Flavor::Total, MemoryConversion> Total;
typedef SingleCounter<Flavor::BalloonActual, MemoryConversion> BalloonActual;

} // namespace Memory

namespace Flavor
{

///////////////////////////////////////////////////////////////////////////////
// struct SwapIn

struct SwapIn {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_SWAP_IN;
	}

	static value_type extract(source_type &c)
	{
		return GetPerfCounter(c, "mem.guest_swap_in");
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct SwapOut

struct SwapOut {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_SWAP_OUT;
	}

	static value_type extract(source_type &c)
	{
		return GetPerfCounter(c, "mem.guest_swap_out");
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct MinorFault

struct MinorFault {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_MINOR_FAULT;
	}

	static value_type extract(source_type &c)
	{
		return GetPerfCounter(c, "mem.guest_minor_fault");
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct MajorFault

struct MajorFault {

	typedef const ProcPerfStoragesContainer source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_MAJOR_FAULT;
	}

	static value_type extract(source_type &c)
	{
		return GetPerfCounter(c, "mem.guest_major_fault");
	}
};

} // namespace Flavor

typedef SingleCounter<Flavor::SwapIn, Conversion::Uint64> SwapIn;
typedef SingleCounter<Flavor::SwapOut, Conversion::Uint64> SwapOut;
typedef SingleCounter<Flavor::MinorFault, Conversion::Uint64> MinorFault;
typedef SingleCounter<Flavor::MajorFault, Conversion::Uint64> MajorFault;

///////////////////////////////////////////////////////////////////////////////
// struct Traits

template<class T>
struct Traits
{
	static QString getExternal(const T& t_)
	{
		return t_();
	}

	static QString getInternal(const T& t_)
	{
		return t_();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct VmCounter

template <typename Name>
struct VmCounter {

	VmCounter(const ProcPerfStoragesContainer &storage, const Name &name)
		: m_storage(&storage), m_name(name)
	{
	}

	QString getName() const
	{
		return Names::Traits<Name>::getExternal(m_name);
	}

	quint64 getValue() const
	{
		return GetPerfCounter(*m_storage, Traits<Name>::getInternal(m_name));
	}

	CVmEventParameter *getParam() const
	{
		return Conversion::Uint64::convert(getValue());
	}

private:

	const ProcPerfStoragesContainer *m_storage;
	const Name m_name;
};

template <typename Name>
VmCounter<Name> makeVmCounter(const ProcPerfStoragesContainer &storage, const Name &name)
{
	return VmCounter<Name>(storage, name);
}

namespace Hdd
{

///////////////////////////////////////////////////////////////////////////////
// struct Name

template <typename Leaf>
struct Name {

	Name(PRL_MASS_STORAGE_INTERFACE_TYPE type, quint32 index)
		: m_type(convert(type)), m_index(index)
	{
	}

	QString operator()() const;

private:
	static const char *convert(PRL_MASS_STORAGE_INTERFACE_TYPE t);

	const char * const m_type;
	const quint32 m_index;
};

template <typename Leaf>
QString Name<Leaf>::operator()() const
{
	return QString("devices.%1%2.%3").
		arg(m_type).
		arg(m_index).
		arg(Leaf::getName());
}

template <typename Leaf>
const char *Name<Leaf>::convert(PRL_MASS_STORAGE_INTERFACE_TYPE t)
{
	switch (t)
	{
	case PMS_IDE_DEVICE:
		return "ide";
	case PMS_SCSI_DEVICE:
		return "scsi";
	case PMS_SATA_DEVICE:
		return "sata";
	default:
		return "unknown";
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct ReadRequests

struct ReadRequests {

	static const char *getName()
	{
		return "read_requests";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct WriteRequests

struct WriteRequests {

	static const char* getName()
	{
		return "write_requests";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct ReadTotal

struct ReadTotal {

	static const char* getName()
	{
		return "read_total";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct WriteTotal

struct WriteTotal {

	static const char* getName()
	{
		return "write_total";
	}
};

} // namespace Hdd

namespace Network {

///////////////////////////////////////////////////////////////////////////////
// struct Name

template <typename Leaf>
struct Name {

	explicit Name(quint32 index)
		: m_index(index)
	{
	}

	QString operator()() const
	{
		return multiCounterName("net.nic", m_index, Leaf::getName());
	}

private:

	quint32 m_index;
};

///////////////////////////////////////////////////////////////////////////////
// struct PacketsIn

struct PacketsIn {

	static const char* getName()
	{
		return "pkts_in";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct PacketsOut

struct PacketsOut {

	static const char* getName()
	{
		return "pkts_out";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct BytesIn

struct BytesIn {

	static const char* getName()
	{
		return "bytes_in";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct BytesOut

struct BytesOut {

	static const char* getName()
	{
		return "bytes_out";
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct ClassfulOffline

struct ClassfulOffline {

	ClassfulOffline(const QString& uuid)
		: m_uuid(uuid)
	{
	}

	const char *getName() const
	{
		return PRL_NET_CLASSFUL_TRAFFIC_PTRN;
	}

	CVmEventParameter *getParam() const;

private:

	QString m_uuid;
};


CVmEventParameter *ClassfulOffline::getParam() const
{
	QScopedPointer<PRL_STAT_NET_TRAFFIC> s(CVzHelper::get_net_stat(m_uuid));
	return s.isNull() ? NULL : Conversion::Network::convert(*s);
}

///////////////////////////////////////////////////////////////////////////////
// struct ClassfulOnline

struct ClassfulOnline {

	ClassfulOnline(const QString &uuid, const ProcPerfStoragesContainer& storage,
			const QList<CVmGenericNetworkAdapter*>& nics)
		: m_uuid(uuid), m_storage(&storage), m_nics(&nics)
	{
	}

	const char *getName() const
	{
		return PRL_NET_CLASSFUL_TRAFFIC_PTRN;
	}

	CVmEventParameter *getParam() const;

private:

	const QString m_uuid;
	const ProcPerfStoragesContainer* m_storage;
	const QList<CVmGenericNetworkAdapter*>* m_nics;
};

CVmEventParameter *ClassfulOnline::getParam() const
{
	CVmEventParameter *p = ClassfulOffline(m_uuid).getParam();
	if (p != NULL)
		return p;

	PRL_STAT_NET_TRAFFIC stat;
	// Copy data to only 1 network class
	stat = PRL_STAT_NET_TRAFFIC();
	foreach (const CVmGenericNetworkAdapter* nic, *m_nics) {
		quint32 i = nic->getIndex();
		stat.incoming[1] += GetPerfCounter(*m_storage, Name<PacketsIn>(i)());
		stat.outgoing[1] += GetPerfCounter(*m_storage, Name<PacketsOut>(i)());
		stat.incoming_pkt[1] += GetPerfCounter(*m_storage, Name<BytesIn>(i)());
		stat.outgoing_pkt[1] += GetPerfCounter(*m_storage, Name<BytesOut>(i)());
	}

	return Conversion::Network::convert(stat);
}

}; // namespace Network

namespace Filesystem {

namespace nf = Names::Filesystem;

namespace Device {

///////////////////////////////////////////////////////////////////////////////
// struct Find

struct Find
{
	explicit Find(const QString& prefix_) : m_prefix(prefix_)
	{
	}

	int operator()(const counter_t& counter_);

	QString getResult() const
	{
		return m_result;
	}

private:
	QString m_prefix;
	QString m_result;
};

int Find::operator()(const counter_t& counter_)
{
	QString n = UTF8_2QSTR(counter_.name + sizeof(PERF_COUNT_TYPE_INC) - 1);
	if (n.startsWith(m_prefix) && PERF_COUNT_ATOMIC_GET(&counter_) != 0) {
		m_result = n.remove(m_prefix);
		return ENUM_BREAK;
	}
	return ENUM_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(const ProcPerfStoragesContainer& storage_, unsigned index_)
		: m_storage(storage_), m_name(index_)
	{
	}

	QString getName() const
	{
		return nf::Traits<nf::Name<nf::Device> >::getExternal(m_name);
	}

	CVmEventParameter *getParam() const
	{
		return Conversion::String::convert(getValue());
	}

	QString getValue() const;

private:
	static int find(counters_storage_t *, counter_t *counter_, void *context_)
	{
		return (*static_cast<Find*>(context_))(*counter_);
	}

	const ProcPerfStoragesContainer& m_storage;
	nf::Name<nf::Device> m_name;
};

QString Unit::getValue() const
{
	Find f(nf::Traits<nf::Name<nf::Device> >::getInternal(m_name) + ".");
	m_storage.enum_counters(find, &f);
	return f.getResult();
}

} // namespace Device

namespace Disk {

///////////////////////////////////////////////////////////////////////////////
// struct Index

struct Index
{
	Index(const ProcPerfStoragesContainer& storage_,
		const CVmConfiguration& config_,
		unsigned index_, unsigned diskIndex_)
	: m_storage(storage_), m_config(config_), m_name(index_, diskIndex_)
	{
	}

	QString getName() const
	{
		return nf::Traits<nf::Disk::Name>::getExternal(m_name);
	}

	CVmEventParameter *getParam() const
	{
		return Conversion::Uint64::convert(getValue());
	}

	quint64 getValue() const;

private:

	const ProcPerfStoragesContainer& m_storage;
	const CVmConfiguration& m_config;
	nf::Disk::Name m_name;
};

quint64 Index::getValue() const
{
	quint64 p = GetPerfCounter(m_storage, nf::Traits<nf::Disk::Name>::getInternal(m_name));
	PRL_MASS_STORAGE_INTERFACE_TYPE i = PMS_UNKNOWN_DEVICE;
	if (p < PIM_SCSI_MASK_OFFSET) {
		i = PMS_IDE_DEVICE;
		p /= PIM_IDE_MASK_OFFSET;
	} else if (p < PIM_SATA_MASK_OFFSET) {
		i = PMS_SCSI_DEVICE;
		p /= PIM_SCSI_MASK_OFFSET;
	} else {
		i = PMS_SATA_DEVICE;
		p /= PIM_SATA_MASK_OFFSET;
	}

	int x = BitFindLowestSet64(p);
	foreach(const CVmHardDisk& d, m_config.getVmHardwareList()->m_lstHardDisks) {
		if (d.getInterfaceType() == i && (int)d.getStackIndex() == x)
			return d.getIndex();
	}
	return 0;
}

} // namespace Disk
} // namespace Filesystem
} // namespace Counter
} // namespace
} // namespace Vm

#ifdef _CT_
namespace Ct
{
namespace
{
namespace Counter
{

struct Cpu {
	explicit Cpu(const Statistics::Cpu &cpu);
	void record(Meter &m) const;

private:
	const Statistics::Cpu *m_cpu;
};

Cpu::Cpu(const Statistics::Cpu &cpu)
	: m_cpu(&cpu)
{
}

void Cpu::record(Meter &m) const
{
	const quint32 n = getHostCpus();
	quint64 v = 0;
	if (n > 0)
		v = (m_cpu->user + m_cpu->system + m_cpu->nice) / n;
	m.record(m_cpu->uptime, v);
}

template <typename Value>
Value accumulateTraffic(Value *klass)
{
	return std::accumulate(klass, klass + PRL_TC_CLASS_MAX, Value(0));
}

quint32 getVcpuNum(const QString &uuid)
{
	SmartPtr<CVmConfiguration> c = CDspService::instance()->getVzHelper()->getCtConfig(
			CDspClient::makeServiceUser(), uuid);

	if (!c.isValid())
		return 0;

	quint32 v = c->getVmHardwareList()->getCpu()->getNumber();
	return v == PRL_CPU_UNLIMITED ? getHostCpus() : v;
}

namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct GuestUsage

struct GuestUsage {

	typedef const Meter source_type;
	typedef quint32 value_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_USAGE_PTRN;
	}

	static value_type extract(source_type &m)
	{
		return m.report().getPercent();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestTimeDelta

struct GuestTimeDelta {

	typedef const Meter source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_TIME_PTRN;
	}

	static value_type extract(source_type &m)
	{
		return m.report().getValueDelta();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct HostTimeDelta

struct HostTimeDelta {

	typedef const Meter source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_HOST_CPU_TIME_PTRN;
	}

	static value_type extract(source_type &m)
	{
		return m.report().getTimeDelta();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Uptime

struct Uptime {

	typedef const Ct::Statistics::Cpu source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "host.cpu.time.cumulative";
	}

	static value_type extract(source_type &c)
	{
		return c.uptime;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct SystemAverage

struct SystemAverage {

	typedef const Ct::Statistics::Cpu source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "guest.cpu.system_average";
	}

	static value_type extract(source_type &c)
	{
		return c.system / (getHostCpus() ?: 1);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct UserAverage

struct UserAverage {

	typedef const Ct::Statistics::Cpu source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "guest.cpu.user_average";
	}

	static value_type extract(source_type &c)
	{
		return c.user / (getHostCpus() ?: 1);
	}
};

} // namespace Flavor

typedef SingleCounter<Flavor::GuestUsage, Conversion::Uint32> GuestUsage;
typedef SingleCounter<Flavor::GuestTimeDelta, Conversion::Uint64> GuestTimeDelta;
typedef SingleCounter<Flavor::HostTimeDelta, Conversion::Uint64> HostTimeDelta;
typedef SingleCounter<Flavor::Uptime, Conversion::Uint64> Uptime;
typedef SingleCounter<Flavor::SystemAverage, Conversion::Uint64> SystemAverage;
typedef SingleCounter<Flavor::UserAverage, Conversion::Uint64> UserAverage;


namespace Disk
{
namespace Flavor
{

///////////////////////////////////////////////////////////////////////////////
// struct Read

struct Read {

	typedef const Ct::Statistics::Disk source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "devices.ide0.read_total";
	}

	static value_type extract(source_type &disk)
	{
		return disk.read;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Write

struct Write {

	typedef const Ct::Statistics::Disk source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "devices.ide0.write_total";
	}

	static value_type extract(source_type &disk)
	{
		return disk.write;
	}
};

} // namespace Flavor

typedef SingleCounter<Flavor::Read, Conversion::Uint64> Read;
typedef SingleCounter<Flavor::Write, Conversion::Uint64> Write;

} // namespace Disk

namespace Network
{
namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Classful

struct Classful {

	typedef const PRL_STAT_NET_TRAFFIC source_type;
	typedef PRL_STAT_NET_TRAFFIC value_type;

	static const char *getName()
	{
		return PRL_NET_CLASSFUL_TRAFFIC_PTRN;
	}

	static value_type extract(source_type &net)
	{
		return net;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct ReceivedSize

struct ReceivedSize {

	typedef const PRL_STAT_NET_TRAFFIC source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "net.nic0.bytes_in";
	}

	static value_type extract(source_type &net)
	{
		return accumulateTraffic(net.incoming);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct TransmittedSize

struct TransmittedSize {

	typedef const PRL_STAT_NET_TRAFFIC source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "net.nic0.bytes_out";
	}

	static value_type extract(source_type &net)
	{
		return accumulateTraffic(net.outgoing);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct ReceivedPackets

struct ReceivedPackets {

	typedef const PRL_STAT_NET_TRAFFIC source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "net.nic0.pkts_in";
	}

	static value_type extract(source_type &net)
	{
		return accumulateTraffic(net.incoming_pkt);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct TransmittedPackets

struct TransmittedPackets {

	typedef const PRL_STAT_NET_TRAFFIC source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "net.nic0.pkts_out";
	}

	static value_type extract(source_type &net)
	{
		return accumulateTraffic(net.outgoing_pkt);
	}
};

} // namespace Flavor

typedef SingleCounter<Flavor::Classful, Conversion::Network> Classful;
typedef SingleCounter<Flavor::ReceivedSize, Conversion::Uint64> ReceivedSize;
typedef SingleCounter<Flavor::TransmittedSize, Conversion::Uint64> TransmittedSize;
typedef SingleCounter<Flavor::ReceivedPackets, Conversion::Uint64> ReceivedPackets;
typedef SingleCounter<Flavor::TransmittedPackets, Conversion::Uint64> TransmittedPackets;

} // namespace Network

namespace Memory
{

namespace Flavour
{
///////////////////////////////////////////////////////////////////////////////
// struct Used

struct Used {

	typedef const Ct::Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_USAGE_PTRN;
	}

	static quint64 extract(source_type &m)
	{
		return m.total - m.free;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Cached

struct Cached {

	typedef const Ct::Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_CACHED_PTRN;
	}

	static quint64 extract(source_type &m)
	{
		return m.cached;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Total

struct Total {

	typedef const Ct::Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_TOTAL_PTRN;
	}

	static quint64 extract(source_type &m)
	{
		return m.total;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Free

struct Free {

	typedef const Ct::Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "guest.ram.free";
	}

	static quint64 extract(source_type &m)
	{
		return m.free;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Real

struct Real {

	typedef const Ct::Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return "guest.ram.real";
	}

	static quint64 extract(source_type &m)
	{
		return m.total - m.free - m.cached;
	}
};

} // namespace Flavour

struct MemoryConversion {
	static CVmEventParameter *convert(quint64 v)
	{
		// bytes to Mb
		return Conversion::Uint64::convert(v >> 20);
	}
};

typedef SingleCounter<Flavour::Free, MemoryConversion> Free;
typedef SingleCounter<Flavour::Total, MemoryConversion> Total;
typedef SingleCounter<Flavour::Cached, MemoryConversion> Cached;
typedef SingleCounter<Flavour::Real, MemoryConversion> Real;
typedef SingleCounter<Flavour::Used, MemoryConversion> Used;

} // namespace Memory


namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct SwapIn

struct SwapIn {

	typedef const Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_SWAP_IN;
	}

	static value_type extract(const Statistics::Memory &m)
	{
		return m.swap_in;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct SwapOut

struct SwapOut {

	typedef const Statistics::Memory source_type;
	typedef quint64 value_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_SWAP_OUT;
	}

	static value_type extract(const Statistics::Memory &m)
	{
		return m.swap_out;
	}
};

} // namespace Flavor

typedef SingleCounter<Flavor::SwapIn, Conversion::Uint64> SwapIn;
typedef SingleCounter<Flavor::SwapOut, Conversion::Uint64> SwapOut;

///////////////////////////////////////////////////////////////////////////////
// struct VCpuTime

struct VCpuTime {

	VCpuTime(const Statistics::Cpu &cpu, quint32 index, quint32 number)
		: m_cpu(&cpu), m_index(index), m_number(number)
	{
	}

	QString getName() const
	{
		return multiCounterName("guest.vcpu", m_index, "time");
	}

	quint64 getValue() const
	{
		// convert from microseconds to nanoseconds
		return (m_cpu->user + m_cpu->system + m_cpu->nice) * 1000 / m_number;
	}

	CVmEventParameter *getParam() const
	{
		return Conversion::Uint64::convert(getValue());
	}

private:
	const Statistics::Cpu *m_cpu;
	const quint32 m_index;
	const quint32 m_number;
};

namespace Filesystem {

namespace Disk {

///////////////////////////////////////////////////////////////////////////////
// struct Index

struct Index
{
	static QString getName()
	{
		return "disk.0";
	}
};

} // namespace Disk

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template <class Name>
struct Flavor
{
	typedef Statistics::Filesystem source_type;
	typedef Names::Filesystem::Name<Name> name_type;

	explicit Flavor(unsigned index_) : m_name(index_)
	{
	}

	const name_type& getName() const
	{
		return m_name;
	}

	static CVmEventParameter *getParam(const source_type& f_);

private:
	name_type m_name;
};

template<>
CVmEventParameter *Flavor<Names::Filesystem::Total>::getParam(const source_type& f_)
{
	return Conversion::Uint64::convert(f_.total);
}

template<>
CVmEventParameter *Flavor<Names::Filesystem::Free>::getParam(const source_type& f_)
{
	return Conversion::Uint64::convert(f_.free);
}

template<>
CVmEventParameter *Flavor<Disk::Index>::getParam(const source_type& f_)
{
	return Conversion::Uint64::convert(f_.index);
}

template<>
CVmEventParameter *Flavor<Names::Filesystem::Device>::getParam(const source_type& f_)
{
	return Conversion::String::convert(f_.device);
}

///////////////////////////////////////////////////////////////////////////////
// struct Counter

template <class Name>
struct Counter
{
	typedef typename Flavor<Name>::source_type source_type;
	typedef typename Flavor<Name>::name_type name_type;

	Counter(unsigned index_, const source_type& source_)
		: m_flavor(index_), m_source(&source_)
	{
	}

	QString getName() const
	{
		return Names::Filesystem::Traits<name_type>::getExternal(m_flavor.getName());
	}

	CVmEventParameter *getParam() const
	{
		return Flavor<Name>::getParam(*m_source);
	}

private:
	const Flavor<Name> m_flavor;
	const source_type * const m_source;
};

typedef Counter<Names::Filesystem::Total> Total;
typedef Counter<Names::Filesystem::Free> Free;
typedef Counter<Disk::Index> Index;
typedef Counter<Names::Filesystem::Device> Device;

} // namespace Filesystem

} // namespace Counter

} // namespace
} // namespace Ct

namespace
{

///////////////////////////////////////////////////////////////////////////////
// struct Collector

struct Collector {

	Collector(QString filter, CVmEvent &event);

	void collectCt(const QString &uuid,
			const Ct::Statistics::Aggregate &a);
	void collectVmOnline(CDspVm &vm, const CVmConfiguration &config);
	void collectVmOffline(const QString &uuid);

private:

	template <typename Counter>
	void collect(const Counter &c);

	QRegExp m_regexp;
	CVmEvent *m_event;
};

Collector::Collector(QString filter, CVmEvent &event) :
	m_event(&event)
{
	if (filter.isEmpty())
	{
		filter = ".*";
	}
	else
	{
		filter.replace('.', "\\.");
		filter.replace('*', ".*");
		filter.replace('#', "[0-9]+");
	}

	m_regexp = QRegExp(filter);
}

void Collector::collectCt(const QString &uuid,
		const Ct::Statistics::Aggregate &a)
{
	using namespace Ct::Counter;

	DAO<Meter> dao;
	Meter t = dao.get(uuid);
	Cpu(a.cpu).record(t);
	dao.set(uuid, t);

	collect(GuestUsage(t));
	collect(GuestTimeDelta(t));
	collect(HostTimeDelta(t));
	collect(Disk::Read(a.disk));
	collect(Disk::Write(a.disk));
	collect(Network::Classful(a.net));
	collect(Network::ReceivedSize(a.net));
	collect(Network::TransmittedSize(a.net));
	collect(Network::ReceivedPackets(a.net));
	collect(Network::TransmittedPackets(a.net));

	Ct::Statistics::Memory *m = a.memory.get();
	if (m != NULL) {
		collect(Memory::Used(*m));
		collect(Memory::Cached(*m));
		collect(Memory::Total(*m));
		collect(SwapIn(*m));
		collect(SwapOut(*m));
	}

	quint32 vcpunum = getVcpuNum(uuid);
	for (quint32 i = 0; i < vcpunum; ++i)
		collect(VCpuTime(a.cpu, i, vcpunum));

	const QList< ::Ct::Statistics::Filesystem>& f = Stat::Collecting::Ct::s_dao.get(uuid);
	for (int i = 0; i < f.size(); ++i)
	{
		const Ct::Statistics::Filesystem& fs = f.at(i);
		collect(Filesystem::Total(i, fs));
		collect(Filesystem::Free(i, fs));
		collect(Filesystem::Device(i, fs));
		collect(Filesystem::Index(i, fs));
	}
}

void Collector::collectVmOnline(CDspVm &vm, const CVmConfiguration &config)
{
	using namespace Vm::Counter;

	CVmIdent id = vm.getVmIdent();
	const QString &uuid = id.first;
	CDspLockedPointer<ProcPerfStoragesContainer> pct = vm.PerfStoragesContainer();
	const ProcPerfStoragesContainer &ct = *pct;

	typedef QPair<Meter, Meter> VmMeter;
	DAO<VmMeter> d;
	VmMeter m = d.get(uuid);
	Meter &t = m.first;
	VCpu(ct).recordTsc(t, &config);
	d.set(uuid, m);

	for (quint32 i = 0; i < config.getVmHardwareList()->getCpu()->getNumber(); ++i)
		collect(VCpuTime(VCpu(ct), i));

	collect(GuestUsage(t));
	collect(GuestTimeDelta(t));
	collect(HostTimeDelta(t));

	collect(Memory::Used(ct));
	collect(Memory::Cached(ct));
	collect(Memory::Total(ct));
	collect(Memory::BalloonActual(ct));
	collect(SwapIn(ct));
	collect(SwapOut(ct));
	collect(MinorFault(ct));
	collect(MajorFault(ct));

	foreach (const CVmHardDisk* d, config.getVmHardwareList()->m_lstHardDisks) {
		PRL_MASS_STORAGE_INTERFACE_TYPE t = d->getInterfaceType();
		quint32 i = d->getStackIndex();
		collect(makeVmCounter(ct, Hdd::Name<Hdd::ReadRequests>(t, i)));
		collect(makeVmCounter(ct, Hdd::Name<Hdd::WriteRequests>(t, i)));
		collect(makeVmCounter(ct, Hdd::Name<Hdd::ReadTotal>(t, i)));
		collect(makeVmCounter(ct, Hdd::Name<Hdd::WriteTotal>(t, i)));
	}

	const QList<CVmGenericNetworkAdapter*> &nics =
		config.getVmHardwareList()->m_lstNetworkAdapters;
	collect(Network::ClassfulOnline(uuid, ct, nics));
	foreach (const CVmGenericNetworkAdapter* nic, nics) {
		quint32 i = nic->getIndex();
		collect(makeVmCounter(ct, Network::Name<Network::PacketsIn>(i)));
		collect(makeVmCounter(ct, Network::Name<Network::PacketsOut>(i)));
		collect(makeVmCounter(ct, Network::Name<Network::BytesIn>(i)));
		collect(makeVmCounter(ct, Network::Name<Network::BytesOut>(i)));
	}

	namespace nf = Names::Filesystem;
	quint64 fsCount = GetPerfCounter(ct, "fs.count");
	for (unsigned i = 0; i < fsCount; ++i) {
		collect(makeVmCounter(ct, nf::Name<nf::Total>(i)));
		collect(makeVmCounter(ct, nf::Name<nf::Free>(i)));
		collect(Filesystem::Device::Unit(ct, i));
		quint64 diskCount = GetPerfCounter(ct,
			QSTR2UTF8(nf::Name<nf::Disk::Count>(i)()));
		for (unsigned j = 0; j < diskCount; ++j)
			collect(Filesystem::Disk::Index(ct, config, i, j));
	}
}

void Collector::collectVmOffline(const QString &uuid)
{
	using namespace Vm::Counter;
	collect(Network::ClassfulOffline(uuid));
}

template <typename Counter>
void Collector::collect(const Counter &c)
{
	if (!m_regexp.exactMatch(c.getName()))
		return;

	CVmEventParameter *p = c.getParam();
	if (p == NULL)
		return;

	p->setParamName(c.getName());
	m_event->addEventParameter(p);
}


bool GetPerformanceStatisticsCt(const CVmIdent &id, Collector &c)
{
	const QString &uuid = id.first;
	SmartPtr<Ct::Statistics::Aggregate> a(CVzHelper::get_env_stat(uuid));
	if (!a.isValid())
		return false;

	c.collectCt(uuid, *a);
	return true;
}

bool GetPerformanceStatisticsVm(const CVmIdent &id, Collector &c)
{
	SmartPtr<CDspVm> vm = CDspVm::GetVmInstanceByUuid(id);

	if (vm.isValid())
	{
		PRL_RESULT rc;
		SmartPtr<CVmConfiguration> config = vm->getVmConfig(SmartPtr<CDspClient>(), rc);
		if (!config.isValid())
			return false;

		c.collectVmOnline(*vm, *config);
	}
	else
	{
		if (CDspService::instance()->isServerModePSBM())
			return false;

		c.collectVmOffline(id.first);
	}

	return true;
}

} // namespace

#endif // _CT_

QSet<SmartPtr<CDspClient> > *CDspStatCollectingThread::g_pHostStatisticsSubscribers =
    new QSet<SmartPtr<CDspClient> >;
QMutex *CDspStatCollectingThread::g_pSubscribersMutex = new QMutex(QMutex::Recursive);
CDspStatCollectingThread::VmStatisticsSubscribersMap *CDspStatCollectingThread::g_pVmsGuestStatisticsSubscribers =
	new CDspStatCollectingThread::VmStatisticsSubscribersMap;

Stat::Perf *CDspStatCollectingThread::g_pPerfStatsSubscribers = new Stat::Perf();

CDspStatCollectingThread::CHostStatGettersList
	*CDspStatCollectingThread::g_pHostStatGetters = new CHostStatGettersList;

CDspStatCollectingThread::CVmStatGettersMap
	*CDspStatCollectingThread::g_pVmStatGetters = new CVmStatGettersMap;

quint64 CDspStatCollectingThread::g_uSuccessivelyCyclesCounter = 0;

QMutex *CDspStatCollectingThread::s_instanceMutex = new QMutex();
CDspStatCollectingThread* CDspStatCollectingThread::s_instance = NULL;

CDspStatCollectingThread::CDspStatCollectingThread()
:	m_bFinalizeWorkNow(false),
	m_pStatCollector(new CDspStatCollector),
	m_nDeltaMs(STAT_COLLECTING_TIMEOUT),
	m_timer()
{
}

bool CDspStatCollectingThread::ExistStatSubscribers()
{
	QMutexLocker _lock(g_pSubscribersMutex);

	return !g_pHostStatisticsSubscribers->isEmpty()
		|| !g_pVmsGuestStatisticsSubscribers->empty()
		|| !g_pHostStatGetters->isEmpty()
		|| !g_pVmStatGetters->isEmpty();
}

bool CDspStatCollectingThread::ExistPerfCountersSubscribers()
{
	QMutexLocker _lock(g_pSubscribersMutex);
	return !g_pPerfStatsSubscribers->empty();
}

void CDspStatCollectingThread::timerEvent(QTimerEvent* event_)
{
	COMMON_TRY
	{
		if (NULL != event_)
			killTimer(event_->timerId());
		//
		// Check current state (if should finalize, do have subscribers, ...)
		// - If m_bFinalizeWorkNow we will quit
		// - If we have NO subscribers we will sleep and wait until m_bFinalizeWorkNow or subscriber added
		//
		if (m_bFinalizeWorkNow)
		{
			exit(PRL_ERR_SUCCESS);
			return;
		}

		QMutexLocker _lock(g_pSubscribersMutex);
		bool bDoPerfStats = ExistPerfCountersSubscribers();
		bool bDoStats = ExistStatSubscribers();
		if (bDoStats)
			++g_uSuccessivelyCyclesCounter;
		else
		{
			g_uSuccessivelyCyclesCounter = 0;
			// If no subscribers - thread will sleep and wait
			if (!bDoPerfStats)
			{
				m_timer = 0;
				WRITE_TRACE(DBG_DEBUG, "No stats subscribers, will wait.");
				return;
			}
		}
		_lock.unlock();

		//
		// Collect and sent stats
		//
		if (bDoStats)
		{
			// Collect and send Host/Vm Stats
			ProcessCpuStat();
			ProcessRamStat();
			ProcessDisksStat();//Required for https://bugzilla.sw.ru/show_bug.cgi?id=422617
			//https://bugzilla.sw.ru/show_bug.cgi?id=430672
			if ( CDspService::instance()->isServerMode() )
			{
				ProcessSwapStat();
				ProcessUptimeStat();
				ProcessProcessesStat();

				QDateTime x = QDateTime::currentDateTime();
				m_nDeltaMs = m_last.daysTo(x) * 1000 * 60 * 60 * 24 +
						m_last.time().msecsTo(x.time());
				m_last = x;

				ProcessNetInfosStat();
				ProcessUsersSessionsStat();
			}

			NotifyHostStatisticsSubscribers();
			NotifyVmsStatisticsSubscribers();
		}

		if (bDoPerfStats)
		{
			// Collect and send Perf Stats
			_lock.relock();
			g_pPerfStatsSubscribers->report(&GetPerformanceStatistics);
			_lock.unlock();
		}

		m_timer = startTimer(STAT_COLLECTING_TIMEOUT);
		return;
	}
	COMMON_CATCH;
	exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

void CDspStatCollectingThread::run()
{
	Stat::Collecting::Mapper m;
	m.connect(this, SIGNAL(abort(const CVmIdent&)), SLOT(abort(const CVmIdent&)));
	m.connect(this, SIGNAL(begin(const CVmIdent&)), SLOT(begin(const CVmIdent&)));
	m_last = QDateTime::currentDateTime();
	{
		QMutexLocker g(g_pSubscribersMutex);
		if (0 == m_timer && (ExistPerfCountersSubscribers() || ExistStatSubscribers()))
			m_timer = startTimer(0);
	}
	exec();
	if (0 != m_timer)
	{
		killTimer(m_timer);
		m_timer = 0;
	}
}

void CDspStatCollectingThread::ProcessCpuStat()
{
	SmartPtr<CCpusStatInfo> pCpusStatInfo( new CCpusStatInfo );
	m_pStatCollector->GetCpusStatInfo(pCpusStatInfo);
	CDspStatisticsGuard::StoreCpusStatInfo(m_pCpusStatInfo, pCpusStatInfo);
	//Storing previous for further usage
	m_pCpusStatInfo = pCpusStatInfo;
}

void CDspStatCollectingThread::ProcessDisksStat()
{
	SmartPtr<CDisksStatInfo> pDisksStatInfo( new CDisksStatInfo );
	m_pStatCollector->GetDisksStatInfo(pDisksStatInfo);
	CDspStatisticsGuard::StoreDisksStatInfo(pDisksStatInfo);
}

void CDspStatCollectingThread::ProcessRamStat()
{
	SmartPtr<CRamStatInfo> pRamStatInfo( new CRamStatInfo );
	m_pStatCollector->GetRamStatInfo(pRamStatInfo);
	CDspStatisticsGuard::StoreRamStatInfo(pRamStatInfo);
}

void CDspStatCollectingThread::ProcessSwapStat()
{
	SmartPtr<CSwapStatInfo> pSwapStatInfo( new CSwapStatInfo );
	m_pStatCollector->GetSwapStatInfo(pSwapStatInfo);
	CDspStatisticsGuard::StoreSwapStatInfo(pSwapStatInfo);
}

void CDspStatCollectingThread::ProcessUptimeStat()
{
	SmartPtr<CUptimeStatInfo> pUptimeStatInfo( new CUptimeStatInfo );
	m_pStatCollector->GetUptimeStatInfo(pUptimeStatInfo);
	CDspStatisticsGuard::StoreUptimeStatInfo(pUptimeStatInfo);
}

void CDspStatCollectingThread::ProcessProcessesStat()
{
	SmartPtr<CProcsStatInfo> pProcsStatInfo( new CProcsStatInfo );
	m_pStatCollector->GetProcsStatInfo(pProcsStatInfo);
	CDspStatisticsGuard::StoreProcsStatInfo(pProcsStatInfo, m_nDeltaMs);
}

void CDspStatCollectingThread::ProcessNetInfosStat()
{
	SmartPtr<CIfacesStatInfo> pIfacesStatInfo( new CIfacesStatInfo );
	m_pStatCollector->GetIfacesStatInfo(pIfacesStatInfo);
	CDspStatisticsGuard::StoreIfacesStatInfo(pIfacesStatInfo);
}

void CDspStatCollectingThread::ProcessUsersSessionsStat()
{
	SmartPtr<CUsersStatInfo> pUsersStatInfo( new CUsersStatInfo );
	m_pStatCollector->GetUsersStatInfo(pUsersStatInfo);
	CDspStatisticsGuard::StoreUsersStatInfo(pUsersStatInfo);
}

void CDspStatCollectingThread::SubscribeToHostStatistics(const SmartPtr<CDspClient> &pUser)
{
	QMutexLocker _lock(g_pSubscribersMutex);
	g_pHostStatisticsSubscribers->insert(pUser);
	schedule();
}

void CDspStatCollectingThread::UnsubscribeFromHostStatistics(const SmartPtr<CDspClient> &pUser)
{
	QMutexLocker _lock(g_pSubscribersMutex);
	g_pHostStatisticsSubscribers->remove(pUser);
}

void CDspStatCollectingThread::CleanupSubscribersLists()
{
	QMutexLocker _lock(g_pSubscribersMutex);
	g_pHostStatisticsSubscribers->clear();
	g_pVmsGuestStatisticsSubscribers->clear();
}

//static
void CDspStatCollectingThread::NotifyHostStatisticsSubscribers()
{
	QMutexLocker _lock(g_pSubscribersMutex);

	if( ! IsAvalableStatisctic() )
		return;

	QString sHostStat;
	QMutableListIterator<StatGetter> it( *g_pHostStatGetters );
	while( it.hasNext() )
	{
		it.next();

		if( sHostStat.isEmpty() )
			sHostStat = GetHostStatistics();

		StatGetter& info = it.value();
		SendStatisticsResponse( info.pUser, info.pPkg, sHostStat );
		it.remove();
	}

	// subscribers
	if ( ! g_pHostStatisticsSubscribers->size() )
		return ;

	SmartPtr<CVmEvent> pStatisticsUpdatedEvent( new CVmEvent(PET_DSP_EVT_HOST_STATISTICS_UPDATED,
															 "", PIE_DISPATCHER) );

	CVmBinaryEventParameter *pBinaryEventParam = new CVmBinaryEventParameter(EVT_PARAM_STATISTICS);
	{
		QMutexLocker _lock(CDspStatisticsGuard::GetSynchroObject());
		CDspStatisticsGuard::GetHostStatistics().Serialize(*pBinaryEventParam->getBinaryDataStream().getImpl());
	}
	pStatisticsUpdatedEvent->addEventParameter(pBinaryEventParam);

	SmartPtr<IOPackage> p(create_binary_package(*pStatisticsUpdatedEvent.getImpl())) ;
    if (!p.isValid())
        return  ;
	foreach(SmartPtr<CDspClient> pUser, *g_pHostStatisticsSubscribers)
		pUser->sendPackage(p);
}

static PRL_RESULT checkAccessRight(const SmartPtr<CDspClient> &pUser,
                                   PVE::IDispatcherCommands dsp_cmd,
                                   const QString &sVmUuid, bool send_invalid_result = true)
{
    PRL_ASSERT(!sVmUuid.isEmpty()) ;

    PRL_RESULT rc = PRL_ERR_FAILURE;
    bool bSetNotValid = false;
    rc = CDspService::instance()->getAccessManager().checkAccess(pUser, dsp_cmd, sVmUuid, &bSetNotValid);
    if ( ! PRL_SUCCEEDED(rc) && send_invalid_result)
        CDspVmDirHelper::sendNotValidState(pUser, rc, sVmUuid, bSetNotValid) ;
    return rc;
}


PRL_RESULT CDspStatCollectingThread::SubscribeToVmGuestStatistics(const QString &sVmUuid,
                                                                  const SmartPtr<CDspClient> &pUser)
{
	PRL_VM_TYPE nType = PVT_VM;
	QString sDirUuid = pUser->getVmDirectoryUuid();
#ifdef _CT_
	bool bOk = CDspService::instance()->getVmDirManager().getVmTypeByUuid(sVmUuid, nType);
	if (bOk && nType == PVT_CT) {
		sDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();
	}
#endif

	if (nType == PVT_VM) {
		PRL_RESULT rc = checkAccessRight(pUser, PVE::DspCmdVmSubscribeToGuestStatistics, sVmUuid) ;
		if (!PRL_SUCCEEDED(rc))
			return rc;
	}

	CVmIdent x = qMakePair(sVmUuid, sDirUuid);
	QMutexLocker _lock(g_pSubscribersMutex);
	VmStatisticsSubscribersMap::iterator e = g_pVmsGuestStatisticsSubscribers->upper_bound(x);
	VmStatisticsSubscribersMap::iterator p = g_pVmsGuestStatisticsSubscribers->lower_bound(x);
	p = std::find_if(p, e, Indicator(x, pUser));
	if (e == p)
		g_pVmsGuestStatisticsSubscribers->insert(std::make_pair(x, pUser));
	if (1 == g_pVmsGuestStatisticsSubscribers->count(x))
	{
		schedule();
		QMutexLocker g(s_instanceMutex);
		if (NULL != s_instance)
			emit s_instance->begin(x);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspStatCollectingThread::UnsubscribeFromVmGuestStatistics(const QString &sVmUuid,
                                                                      const SmartPtr<CDspClient> &pUser)
{
	PRL_VM_TYPE nType = PVT_VM;
	QString sDirUuid = pUser->getVmDirectoryUuid();
#ifdef _CT_
	bool bOk = CDspService::instance()->getVmDirManager().getVmTypeByUuid(sVmUuid, nType);
	if (bOk && nType == PVT_CT) {
		sDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();
	}
#endif

	// AccessCheck
	if ( ! sVmUuid.isEmpty() && nType == PVT_VM)
	{
		PRL_RESULT rc = checkAccessRight(pUser, PVE::DspCmdVmUnsubscribeFromGuestStatistics, sVmUuid) ;
		if (!PRL_SUCCEEDED(rc) && rc!=PRL_ERR_VM_CONFIG_DOESNT_EXIST)
			return rc;
	}

	CVmIdent x(sVmUuid, sDirUuid);
	QMutexLocker _lock(g_pSubscribersMutex);
	if (sVmUuid.isEmpty())
	{
		//Unsubscribe user from all VMs
		VmStatisticsSubscribersMap::iterator p;
		p = g_pVmsGuestStatisticsSubscribers->begin();
		while (p != g_pVmsGuestStatisticsSubscribers->end())
		{
			if (p->second == pUser)
				g_pVmsGuestStatisticsSubscribers->erase(p++);
			else
				++p;
		}
	}
	else
		g_pVmsGuestStatisticsSubscribers->erase(x);

	if (0 == g_pVmsGuestStatisticsSubscribers->count(x))
	{
		//No more subscribers - cleanup VM record
		QMutexLocker g(s_instanceMutex);
		if (NULL != s_instance)
			emit s_instance->abort(x);
	}
	return PRL_ERR_SUCCESS;
}

void CDspStatCollectingThread::ClientDisconnected(const SmartPtr<CDspClient> &pUser)
{
	UnsubscribeFromHostStatistics(pUser);
	QString emptyStr;
	UnsubscribeFromVmGuestStatistics(emptyStr, pUser);
	QMutexLocker _lock(g_pSubscribersMutex);
	foreach (const CVmIdent& i, g_pPerfStatsSubscribers->select(pUser))
	{
		g_pPerfStatsSubscribers->remove(i, pUser);
		if (g_pPerfStatsSubscribers->has(i))
			continue;
		QMutexLocker g(s_instanceMutex);
		if (NULL != s_instance)
			emit s_instance->abort(i);
	}
}

void CDspStatCollectingThread::NotifyVmsStatisticsSubscribers()
{
	QMutexLocker _lock(g_pSubscribersMutex);

	if( ! IsAvalableStatisctic() )
		return;

	QMutableMapIterator< QPair<QString, QString>, QList<StatGetter> > it( *g_pVmStatGetters );
	while( it.hasNext() )
	{
		it.next();

		QString sVmStat;
		QList<StatGetter>& lst = it.value();
		for( int i=0; i< lst.size(); i ++ )
		{
			if( sVmStat.isEmpty() )
				sVmStat = GetVmGuestStatistics( it.key().first, it.key().second )->toString();
			SendStatisticsResponse( lst[i].pUser, lst[i].pPkg, sVmStat );
		}
		it.remove();
	}

	SmartPtr<IOPackage> g;
	typedef VmStatisticsSubscribersMap::const_iterator iterator_type;
	iterator_type e = g_pVmsGuestStatisticsSubscribers->end();
	iterator_type p = g_pVmsGuestStatisticsSubscribers->begin();
	for (VmStatisticsSubscribersMap::key_type k; p != e; ++p)
	{
		if (p->first != k)
		{
			k = p->first;
			CVmEvent x(PET_DSP_EVT_VM_STATISTICS_UPDATED, k.first, PIE_VIRTUAL_MACHINE);
			CVmBinaryEventParameter *b = new CVmBinaryEventParameter(EVT_PARAM_STATISTICS);
			GetVmGuestStatistics(k.first, k.second)->Serialize(*b->getBinaryDataStream().getImpl());
			x.addEventParameter(b);
			g = create_binary_package(x);
			if (!g.isValid())
				return;
		}
		p->second->sendPackage(g);
	}
}

//static
void CDspStatCollectingThread::ProcessPerfStatsCommand(const SmartPtr<CDspClient> &pUser,
                                                       const SmartPtr<IOPackage>& pkg)
{
	PRL_ASSERT(pkg->header.type == PVE::DspCmdPerfomanceStatistics) ;

	CProtoCommandPtr cmd_ptr = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd_ptr->IsValid() )
	{
		pUser->sendSimpleResponse(pkg, PRL_ERR_UNRECOGNIZED_REQUEST) ;
		return ;
	}
	CProtoPerfStatsCommand *cmd = CProtoSerializer::CastToProtoCommand<CProtoPerfStatsCommand>(cmd_ptr) ;

    PRL_RESULT result = PRL_ERR_FAILURE ;
    switch (cmd->GetAction())
    {
        case CProtoPerfStatsCommand::PSA_SUBSCRIBE:
            result = SubscribeToPerfStats(pUser, cmd->GetFilter(), cmd->GetVmUuid()) ;
            break ;
        case CProtoPerfStatsCommand::PSA_UNSUBSCRIBE:
            result = UnsubscribeFromPerfStats(pUser, cmd->GetVmUuid()) ;
            break ;

        case CProtoPerfStatsCommand::PSA_GET:
        {
            SendPerfStatsRequest(pUser, pkg, cmd->GetFilter(), cmd->GetVmUuid()) ;
            return  ;
        }

        default:
            LOG_MESSAGE(DBG_FATAL, "Unknown PerfStats Action: %d", cmd->GetAction()) ;
            PRL_ASSERT(!"Should not get here!") ;
    } ;

    pUser->sendSimpleResponse( pkg, result );
}

//static
void CDspStatCollectingThread::SendPerfStatsRequest(const SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage>& pkg,
                                                    const QString &sFilter, const QString &sVmUuid)
{
	CVmIdent vm_ident ;
	PRL_VM_TYPE nType = PVT_VM;

#ifdef _CT_
	bool bOk = CDspService::instance()->getVmDirManager().getVmTypeByUuid(sVmUuid, nType);
	if (bOk && nType == PVT_CT) {
		QString sDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();
		vm_ident = MakeVmIdent(sVmUuid, sDirUuid);
	}
#endif

	// if Vm perfomace statistics -> check for access rights
	if ( !sVmUuid.isEmpty() && nType == PVT_VM)
	{
		PRL_RESULT rc = checkAccessRight(pUser, PVE::DspCmdPerfomanceStatistics, sVmUuid) ;
		if (!PRL_SUCCEEDED(rc)) {
			pUser->sendSimpleResponse( pkg, rc );
			return ;
		}

		vm_ident = pUser->getVmIdent(sVmUuid) ;
	}
	SmartPtr<CVmEvent> pPerfCountersEvent(GetPerformanceStatistics(vm_ident, sFilter)) ;
	if (!pPerfCountersEvent.isValid() || PRL_FAILED(pPerfCountersEvent->getEventCode())) {
		PRL_RESULT nRetCode = pPerfCountersEvent.isValid() ? pPerfCountersEvent->getEventCode() : PRL_ERR_FAILURE;
		WRITE_TRACE(DBG_FATAL, "Performance counters extraction failed with code: %.8X '%s'", nRetCode, PRL_RESULT_TO_STRING( nRetCode ) );
		if (pPerfCountersEvent.isValid())
			pUser->sendResponseError( pPerfCountersEvent.getImpl(), pkg );
		else
			pUser->sendSimpleResponse( pkg, nRetCode );
		return;
	}

	CProtoCommandPtr pResponseCmd = CProtoSerializer::CreateDspWsResponseCommand(pkg, PRL_ERR_SUCCESS);
	QByteArray _byte_array;
	QBuffer _buffer(&_byte_array);
	bool bRes = _buffer.open(QIODevice::ReadWrite);
	PRL_ASSERT(bRes);
	if (!bRes)
	{
		WRITE_TRACE(DBG_FATAL, "Fatal error - couldn't to open binary data buffer for read/write");
		pUser->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}
	QDataStream _data_stream(&_buffer);
	_data_stream.setVersion(QDataStream::Qt_4_0);

	pResponseCmd->GetCommand()->Serialize(_data_stream);
	pPerfCountersEvent->Serialize(_data_stream);
	_buffer.reset();

	pUser->sendPackage( DispatcherPackage::createInstance(PVE::DspWsBinaryResponse, _data_stream, _byte_array.size(), pkg) );
}

//static
PRL_RESULT CDspStatCollectingThread::SubscribeToPerfStats(const SmartPtr<CDspClient> &pUser,
		const QString &sFilter, const QString &sVmUuid)
{
	CVmIdent vm_ident ;
	PRL_VM_TYPE nType = PVT_VM;

#ifdef _CT_
	bool bOk = CDspService::instance()->getVmDirManager().getVmTypeByUuid(sVmUuid, nType);
	if (bOk && nType == PVT_CT) {
		QString sDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();
		vm_ident = MakeVmIdent(sVmUuid, sDirUuid);
	}
#endif

	// if Vm perfomace statistics -> check for access rights
	if ( !sVmUuid.isEmpty() && nType == PVT_VM)
	{
		PRL_RESULT rc = checkAccessRight(pUser, PVE::DspCmdPerfomanceStatistics, sVmUuid) ;
		if (!PRL_SUCCEEDED(rc))
			return rc;

		vm_ident = pUser->getVmIdent(sVmUuid) ;
	}

	QMutexLocker _lock(g_pSubscribersMutex);
	g_pPerfStatsSubscribers->remove(vm_ident, pUser);
	bool x = g_pPerfStatsSubscribers->has(vm_ident);
	if (g_pPerfStatsSubscribers->add(vm_ident, sFilter, pUser))
	{
		schedule();
		QMutexLocker g(s_instanceMutex);
		if (NULL != s_instance && !x)
			emit s_instance->begin(vm_ident);
	}
	return PRL_ERR_SUCCESS ;
}

//static
PRL_RESULT CDspStatCollectingThread::UnsubscribeFromPerfStats(const SmartPtr<CDspClient> &pUser,
                                                              const QString &sVmUuid)
{
	CVmIdent vm_ident ;
	PRL_VM_TYPE nType = PVT_VM;
#ifdef _CT_
	bool bOk = CDspService::instance()->getVmDirManager().getVmTypeByUuid(sVmUuid, nType);
	if (bOk && nType == PVT_CT) {
		QString sDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();
		vm_ident = MakeVmIdent(sVmUuid, sDirUuid);
	}
#endif

	// if Vm perfomace statistics -> check for access rights
	if ( !sVmUuid.isEmpty() && nType == PVT_VM)
	{
		PRL_RESULT rc = checkAccessRight(pUser, PVE::DspCmdPerfomanceStatistics, sVmUuid) ;
		if (!PRL_SUCCEEDED(rc) && rc!=PRL_ERR_VM_CONFIG_DOESNT_EXIST)
			return rc;

		vm_ident = pUser->getVmIdent(sVmUuid) ;
	}
	QMutexLocker _lock(g_pSubscribersMutex);
	g_pPerfStatsSubscribers->remove(vm_ident, pUser);
	if (!g_pPerfStatsSubscribers->has(vm_ident))
	{
		QMutexLocker g(s_instanceMutex);
		if (NULL != s_instance)
			emit s_instance->abort(vm_ident);
	}
	return PRL_ERR_SUCCESS;
}

//static
SmartPtr<CVmEvent> CDspStatCollectingThread::GetPerformanceStatistics(const CVmIdent &id, const QString &filter)
{
	// empty id is a dispatcher perf stat request
	if (!IsValidVmIdent(id))
	{
		QString uuid = CDspService::instance()->getDispConfigGuard().
			getDispConfig()->getVmServerIdentification()->getServerUuid() ;
		SmartPtr<CVmEvent> e(new CVmEvent(PET_DSP_EVT_PERFSTATS, uuid, PIE_DISPATCHER));
		e->setEventCode(PRL_ERR_SUCCESS);
		return e;
	}

	SmartPtr<CVmEvent> e(new CVmEvent(PET_DSP_EVT_VM_PERFSTATS, id.first, PIE_VIRTUAL_MACHINE));
	Collector c(filter, *e);

	bool r;
	if (isContainer(id.first))
		r = GetPerformanceStatisticsCt(id, c);
	else
		r = GetPerformanceStatisticsVm(id, c);

	if (r)
	{
		e->setEventCode(PRL_ERR_SUCCESS);
	}
	else
	{
		e->setEventCode(PRL_ERR_DISP_VM_IS_NOT_STARTED);
		e->addEventParameter(new CVmEventParameter(
					PVE::String,
					CDspService::instance()->getVmDirManager().getVmNameByUuid(id),
					EVT_PARAM_MESSAGE_PARAM_0));
	}

	LOG_MESSAGE(DBG_DEBUG, "PerfStats: prm_count: %d", e->m_lstEventParameters.size());

	return e;
}

bool CDspStatCollectingThread::IsAvalableStatisctic()
{
	// true if two successively stat calculation;
	QMutexLocker _lock(g_pSubscribersMutex);
	return g_uSuccessivelyCyclesCounter > 0;
}

void CDspStatCollectingThread::SendHostStatistics(
	SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage>& p)
{
	if( IsAvalableStatisctic() )
	{
		// send response
		QString stat = GetHostStatistics();
		SendStatisticsResponse( pUser, p, stat );
	}
	else
	{
		// register
		QMutexLocker _lock(g_pSubscribersMutex) ;
		g_pHostStatGetters->append( StatGetter( pUser, p ) );
		schedule();
	}
}

void CDspStatCollectingThread::SendVmGuestStatistics(
	const QString &sVmUuid,
	SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage>& p )
{
	if( IsAvalableStatisctic() )
	{
		// send response
		QString stat = GetVmGuestStatistics( sVmUuid, pUser->getVmDirectoryUuid() )->toString();
		SendStatisticsResponse( pUser, p, stat );
	}
	else
	{
		// register
		QMutexLocker _lock(g_pSubscribersMutex) ;
		(*g_pVmStatGetters)[qMakePair(sVmUuid, pUser->getVmDirectoryUuid())].append( StatGetter( pUser, p ) );
		schedule();
	}
}

QString CDspStatCollectingThread::GetHostStatistics()
{
	QString sHostStatistics = CDspStatisticsGuard::GetStatisticsStringRepresentation();
	return sHostStatistics;
}

static bool GetTotalVmRamSizeFast(SmartPtr<CDspVm> &pVm, quint64 &totalRamSize)
{
	bool res = false;
#ifdef _LIN_
	if (!ParallelsDirs::isServerModePSBM())
		return res;
	// even if VM config caching is disabled we can still get total memory
	// size for VM without reading config
	char buf[256];
	FILE *fp;

	fp = fopen(QSTR2UTF8(QString("/proc/parallels/vm/%1/meminfo").arg(pVm->getVmUuid())), "r");
	if (!fp)
		return res;

	while (fgets(buf, sizeof(buf), fp) != NULL)
		if (sscanf(buf, "TotalRAM:%*[ ]%llu", &totalRamSize) == 1)
		{
			// ram size units here are Kb, make them Mb
			totalRamSize /= 1024;
			res = true;
			break;
		}

	fclose(fp);
#else
	Q_UNUSED(pVm);
	Q_UNUSED(totalRamSize);
#endif
	return res;
}

SmartPtr<CSystemStatistics> CDspStatCollectingThread::GetVmGuestStatistics(
	const QString &sVmUuid,
	const QString &sVmDirUuid)
{
#ifdef _CT_
	if (isContainer(sVmUuid))
	{
		using namespace Ct::Counter;
		SmartPtr< ::Ct::Statistics::Aggregate> a(CVzHelper::get_env_stat(sVmUuid));
		if (!a.isValid())
			return SmartPtr<CSystemStatistics>();
		DAO<Meter> dao;
		Meter t = dao.get(sVmUuid);
		Ct::Counter::Cpu(a->cpu).record(t);
		dao.set(sVmUuid, t);

		SmartPtr<CSystemStatistics> output(new CSystemStatistics());
		QScopedPointer<CNetIfaceStatistics> net(new CNetIfaceStatistics());
		QScopedPointer<CDiskStatistics> disk(new CDiskStatistics());

		QScopedPointer<CCpuStatistics> cpu(new CCpuStatistics());
		const Ct::Statistics::Cpu &c = a->cpu;
		cpu->setTotalTime(Uptime(c).getValue());
		cpu->setUserTime(UserAverage(c).getValue());
		cpu->setSystemTime(SystemAverage(c).getValue());
		cpu->setPercentsUsage(GuestUsage(t).getValue());
		output->m_lstCpusStatistics.append(cpu.take());
		output->getUptimeStatistics()->setOsUptime(Uptime(c).getValue());

		using ::Ct::Counter::accumulateTraffic;
		const PRL_STAT_NET_TRAFFIC& n = a->net;
		net->setInDataSize(Network::ReceivedSize(n).getValue());
		net->setOutDataSize(Network::TransmittedSize(n).getValue());
		net->setInPkgsCount(Network::ReceivedPackets(n).getValue());
		net->setOutPkgsCount(Network::TransmittedPackets(n).getValue());
		*output->getNetClassStatistics() = n;
		output->m_lstNetIfacesStatistics.append(net.take());

		CMemoryStatistics *memory = output->getMemoryStatistics();
		Ct::Statistics::Memory m;
		if (a->memory.isValid())
			m = *a->memory;
		memory->setTotalSize(Memory::Total(m).getValue());
		memory->setUsageSize(Memory::Used(m).getValue());
		memory->setFreeSize(Memory::Free(m).getValue());
		memory->setRealSize(Memory::Real(m).getValue());

		const Ct::Statistics::Disk &d = a->disk;
		disk->setReadBytesTotal(Disk::Read(d).getValue());
		disk->setWriteBytesTotal(Disk::Write(d).getValue());
		output->m_lstDisksStatistics.append(disk.take());

		return output;
	}
#endif

	SmartPtr<CProcsStatInfo> pProcsStatInfo( new CProcsStatInfo );

	SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( sVmUuid, sVmDirUuid );
	PRL_UINT64 nVmUptime = 0;
	quint64 nTotalVmRamSize = 0;
	quint64 usage = 0;
	quint32 tsc = 0, time = 0;
	if (pVm.isValid())
	{
		CDspLockedPointer<ProcPerfStoragesContainer> ct = pVm->PerfStoragesContainer();
		PRL_RESULT rc;
		SmartPtr<CVmConfiguration> pVmConfig = pVm->getVmConfig(SmartPtr<CDspClient>(), rc);

		if (CDspDispConfigGuard::isConfigCacheEnabled() || !GetTotalVmRamSizeFast(pVm, nTotalVmRamSize))
		{
			nTotalVmRamSize = pVmConfig.isValid() ?
				(pVmConfig->getVmHardwareList()->getMemory()->getRamSize() +
				 pVmConfig->getVmHardwareList()->getVideo()->getMemorySize()): 0;
		}
		nTotalVmRamSize *= (1024 * 1024);
		nVmUptime = pVm->getVmProcessUptimeInSecs();

		CDspStatCollector::GetProcsStatInfo(pProcsStatInfo, pVm->getVmProcessId());

		using namespace Vm::Counter;
		typedef QPair<Meter, Meter> VmMeter;
		DAO<VmMeter> d;
		VmMeter m = d.get(sVmUuid);
		VCpu(*ct).recordTsc(m.first, pVmConfig.get());
		VCpu(*ct).recordTime(m.second, !pProcsStatInfo->m_lstProcsStatInfo.isEmpty()?
				pProcsStatInfo->m_lstProcsStatInfo.front()->m_nTotalTime : 0);
		d.set(sVmUuid, m);
		tsc = m.first.report().getPercent();
		time = m.second.report().getPercent();
	}

	SmartPtr<CSystemStatistics> pVmStat( new CSystemStatistics );

	// Virtual cpu usage
	pVmStat->m_lstCpusStatistics.append(new CCpuStatistics);
	pVmStat->m_lstCpusStatistics.last()->setPercentsUsage(tsc);
	// Host cpu usage
	pVmStat->m_lstCpusStatistics.append(new CCpuStatistics);
	pVmStat->m_lstCpusStatistics.last()->setPercentsUsage(time);

	pVmStat->getMemoryStatistics()->setTotalSize(nTotalVmRamSize);
	pVmStat->getMemoryStatistics()->setUsageSize(usage);
	pVmStat->getMemoryStatistics()->setFreeSize(
		pVmStat->getMemoryStatistics()->getTotalSize() - pVmStat->getMemoryStatistics()->getUsageSize()
	);
	if ( ! pProcsStatInfo->m_lstProcsStatInfo.isEmpty() )
		pVmStat->getMemoryStatistics()->setRealSize(pProcsStatInfo->m_lstProcsStatInfo.front()->m_nRealMemUsage);

	pVmStat->getUptimeStatistics()->setOsUptime(nVmUptime);

	return pVmStat;
}

void CDspStatCollectingThread::SendStatisticsResponse(
	SmartPtr<CDspClient> &pUser,
	const SmartPtr<IOPackage>& p,
	const QString& sStatAsString
	)
{
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);
	CProtoCommandDspWsResponse
		*pDspWsResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	pDspWsResponseCmd->SetSystemStatistics(sStatAsString);
	pUser->sendResponse(pResponse, p);
}

void CDspStatCollectingThread::stop()
{
	QMutexLocker g(s_instanceMutex);
	CDspStatCollectingThread* x = s_instance;
	if (NULL == x)
		return;

	s_instance = NULL;
	g.unlock();
	x->m_bFinalizeWorkNow = true;
	QMetaObject::invokeMethod(x, "timerEvent", Q_ARG(QTimerEvent*, NULL));
	if (QThread::currentThread() == x->thread())
		x->deleteLater();
	else
	{
		x->wait();
		delete x;
	}
}

void CDspStatCollectingThread::start()
{
	QMutexLocker g(s_instanceMutex);
	if (NULL != s_instance)
		return;

	qRegisterMetaType<QTimerEvent*>("QTimerEvent*");
	s_instance = new CDspStatCollectingThread();
	s_instance->moveToThread(s_instance);
	s_instance->QThread::start(QThread::LowPriority);
}

void CDspStatCollectingThread::schedule()
{
	QMutexLocker g(s_instanceMutex);
	if (NULL == s_instance || s_instance->m_timer != 0)
		return;

	QMetaObject::invokeMethod(s_instance, "timerEvent", Q_ARG(QTimerEvent*, NULL));
}

