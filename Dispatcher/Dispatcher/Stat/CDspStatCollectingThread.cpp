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
#include "Libraries/Logging/Logging.h"
#include "Libraries/Std/PrlAssert.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmBinaryEventParameter.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "CDspClient.h"
#include "CDspVm.h"
#include "CDspService.h"
#include "CDspCommon.h"
#include "CDspVmStateSender.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include <prlsdk/PrlPerfCounters.h>
#include <prlsdk/PrlIOStructs.h>
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/Std/PrlTime.h"

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

namespace Stat
{
namespace Collecting
{
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
		m_vm = v->getVmConnectionHandle();
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

		m_pending = v->sendPackageToVm(m_request);
		if (!m_pending.isValid())
			return reset();
	} while(false);
	m_timer = startTimer(m_period);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mapper

void Mapper::abort(const CVmIdent& ident_)
{
	QString k = QString(ident_.first).append(ident_.second);
	Farmer* f = findChild<Farmer* >(k);
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
	Farmer* f = findChild<Farmer* >(k);
	if (NULL != f)
		return;

	PRL_VM_TYPE t = PVT_VM;
	if (CDspService::instance()->getVmDirManager().getVmTypeByUuid(ident_.first, t))
		return;

	f = new Farmer(ident_);
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

CVmEventParameter *eventParamUint32(quint32 v)
{
	return new CVmEventParameter(PVE::UnsignedInt, QString::number(v));
}

CVmEventParameter *eventParamUint64(quint64 v)
{
	return new CVmEventParameter(PVE::UInt64, QString::number(v));
}

CVmBinaryEventParameter *eventParam(const PRL_STAT_NET_TRAFFIC &stat)
{
	CVmBinaryEventParameter *p = new CVmBinaryEventParameter();
	int sz = p->getBinaryDataStream()->writeRawData((const char*)&stat, sizeof(stat));
	PRL_ASSERT(sizeof(stat) == sz);
	return p;
}

bool isContainer(const QString &uuid)
{
	PRL_VM_TYPE t = PVT_VM;

	if (!CDspService::instance()->getVmDirManager().getVmTypeByUuid(uuid, t))
		return false;

	return t == PVT_CT;
}

///////////////////////////////////////////////////////////////////////////////
// struct SingleCounter

template <typename Flavor>
struct SingleCounter {

	typedef typename Flavor::source_type source_type;

	explicit SingleCounter(const source_type &source)
		: m_source(&source)
	{
	}

	const char* getName() const
	{
		return Flavor::getName();
	}

	CVmEventParameter *getValue() const
	{
		return Flavor::extract(*m_source);
	}

private:

	const source_type * const m_source;
};

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

	CVmEventParameter *getValue() const
	{
		/* typedef unsigned int uint128_t __attribute__((mode(TI))); */

		// FIXME commented as getTscHz is not provided
		// convert from TSC to nanosec
		/* quint64 ns = ((uint128_t) m_vcpu->getValue(m_index)) * 1000000000ULL / CDspHostInfo::GetTscHz(); */
		quint64 ns = m_vcpu->getValue(m_index);
		return eventParamUint64(ns);
	}

private:
	const VCpu *m_vcpu;
	quint32 m_index;
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestUsage

struct GuestUsage {

	typedef const Meter source_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_USAGE_PTRN;
	}

	static CVmEventParameter *extract(const Meter &m)
	{
		return eventParamUint32(m.report().getPercent());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestTimeDelta

struct GuestTimeDelta {

	typedef const Meter source_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_TIME_PTRN;
	}

	static CVmEventParameter *extract(const Meter &m)
	{
		return eventParamUint64(m.report().getValueDelta());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct HostTimeDelta

struct HostTimeDelta {

	typedef const Meter source_type;

	static const char *getName()
	{
		return PRL_HOST_CPU_TIME_PTRN;
	}

	static CVmEventParameter *extract(const Meter &m)
	{
		return eventParamUint64(m.report().getTimeDelta());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Reclaimable

struct Reclaimable {

	typedef const ProcPerfStoragesContainer source_type;

	static const char *getName()
	{
		return PRL_WS_RECLAIMABLE_PTRN;
	}

	static CVmEventParameter *extract(const ProcPerfStoragesContainer &c)
	{
		return eventParamUint32(extractUint32(c));
	}

	static quint32 extractUint32(const ProcPerfStoragesContainer &c)
	{
		return GetPerfCounter(c, "kernel.ws.reclaimable") / 256;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct MemoryUsed

struct MemoryUsed {

	typedef const ProcPerfStoragesContainer source_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_USAGE_PTRN;
	}

	static CVmEventParameter *extract(const ProcPerfStoragesContainer &c)
	{
		return eventParamUint32(GetPerfCounter(c, "mem.guest_used") / 1024);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct MemoryCached

struct MemoryCached {

	typedef const ProcPerfStoragesContainer source_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_CACHED_PTRN;
	}

	static CVmEventParameter *extract(const ProcPerfStoragesContainer &c)
	{
		return eventParamUint32(GetPerfCounter(c, "mem.guest_cached") / 1024);
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
		return m_name();
	}

	CVmEventParameter *getValue() const
	{
		return eventParamUint64(GetPerfCounter(*m_storage, m_name()));
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

	CVmEventParameter *getValue() const;

private:

	QString m_uuid;
};


CVmEventParameter *ClassfulOffline::getValue() const
{
	PRL_STAT_NET_TRAFFIC stat = PRL_STAT_NET_TRAFFIC();
#ifdef _CT_
	int r = CVzHelper::get_net_stat(m_uuid, &stat);
#else
	int r = -1;
#endif

	return r ? NULL : eventParam(stat);
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

	CVmEventParameter *getValue() const;

private:

	const QString m_uuid;
	const ProcPerfStoragesContainer* m_storage;
	const QList<CVmGenericNetworkAdapter*>* m_nics;
};

CVmEventParameter *ClassfulOnline::getValue() const
{
	CVmEventParameter *p = ClassfulOffline(m_uuid).getValue();
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
		stat.outgoing_pkt[1] += GetPerfCounter(*m_storage, Name<BytesOut>(i)());;
	}

	return eventParam(stat);
}

}; // namespace Network


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
	SmartPtr<CVmConfiguration> c = CVzHelper::get_env_config(uuid);
	if (!c.isValid())
		return 0;

	quint32 v = c->getVmHardwareList()->getCpu()->getNumber();
	return v == PRL_CPU_UNLIMITED ? getHostCpus() : v;
}

///////////////////////////////////////////////////////////////////////////////
// struct GuestUsage

struct GuestUsage {

	typedef const Meter source_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_USAGE_PTRN;
	}

	static CVmEventParameter *extract(const Meter &m)
	{
		return eventParamUint32(m.report().getPercent());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestTimeDelta

struct GuestTimeDelta {

	typedef const Meter source_type;

	static const char *getName()
	{
		return PRL_GUEST_CPU_TIME_PTRN;
	}

	static CVmEventParameter *extract(const Meter &m)
	{
		return eventParamUint64(m.report().getValueDelta());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct HostTimeDelta

struct HostTimeDelta {

	typedef const Meter source_type;

	static const char *getName()
	{
		return PRL_HOST_CPU_TIME_PTRN;
	}

	static CVmEventParameter *extract(const Meter &m)
	{
		return eventParamUint64(m.report().getTimeDelta());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct DiskRead

struct DiskRead {

	typedef const CDiskStatistics source_type;

	static const char *getName()
	{
		return "devices.ide0.read_total";
	}

	static CVmEventParameter *extract(const CDiskStatistics &disk)
	{
		return eventParamUint64(disk.getReadBytesTotal());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct DiskWrite

struct DiskWrite {

	typedef const CDiskStatistics source_type;

	static const char *getName()
	{
		return "devices.ide0.write_total";
	}

	static CVmEventParameter *extract(const CDiskStatistics &disk)
	{
		return eventParamUint64(disk.getWriteBytesTotal());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Classful

struct Classful {

	typedef const PRL_STAT_NET_TRAFFIC source_type;

	static const char *getName()
	{
		return PRL_NET_CLASSFUL_TRAFFIC_PTRN;
	}

	static CVmEventParameter *extract(const PRL_STAT_NET_TRAFFIC &net)
	{
		return eventParam(net);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct NetBytesIn

struct NetBytesIn {

	typedef const PRL_STAT_NET_TRAFFIC source_type;

	static const char *getName()
	{
		return "net.nic0.bytes_in";
	}

	static CVmEventParameter *extract(const PRL_STAT_NET_TRAFFIC &net)
	{
		return eventParamUint64(accumulateTraffic(net.incoming));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct NetBytesOut

struct NetBytesOut {

	typedef const PRL_STAT_NET_TRAFFIC source_type;

	static const char *getName()
	{
		return "net.nic0.bytes_out";
	}

	static CVmEventParameter *extract(const PRL_STAT_NET_TRAFFIC &net)
	{
		return eventParamUint64(accumulateTraffic(net.outgoing));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct NetPacketsIn

struct NetPacketsIn {

	typedef const PRL_STAT_NET_TRAFFIC source_type;

	static const char *getName()
	{
		return "net.nic0.pkts_in";
	}

	static CVmEventParameter *extract(const PRL_STAT_NET_TRAFFIC &net)
	{
		return eventParamUint64(accumulateTraffic(net.incoming_pkt));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct NetPacketsOut

struct NetPacketsOut {

	typedef const PRL_STAT_NET_TRAFFIC source_type;

	static const char *getName()
	{
		return "net.nic0.pkts_out";
	}

	static CVmEventParameter *extract(const PRL_STAT_NET_TRAFFIC &net)
	{
		return eventParamUint64(accumulateTraffic(net.outgoing_pkt));
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct MemoryUsed

struct MemoryUsed {

	typedef const CMemoryStatistics source_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_USAGE_PTRN;
	}

	static CVmEventParameter *extract(const CMemoryStatistics &memory)
	{
		if (memory.getTotalSize() == 0)
			return NULL;
		return eventParamUint32(memory.getUsageSize() >> 20);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct MemoryCached

struct MemoryCached {

	typedef const CMemoryStatistics source_type;

	static const char *getName()
	{
		return PRL_GUEST_RAM_CACHED_PTRN;
	}

	static CVmEventParameter *extract(const CMemoryStatistics &memory)
	{
		if (memory.getTotalSize() == 0)
			return NULL;
		return eventParamUint32((memory.getUsageSize() - memory.getRealSize()) >> 20);
	}
};

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

	CVmEventParameter *getValue() const
	{
		return eventParamUint64((m_cpu->user + m_cpu->system + m_cpu->nice) * 1000 / m_number);
	}

private:
	const Statistics::Cpu *m_cpu;
	const quint32 m_index;
	const quint32 m_number;
};

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

	QString fixFilter(QString filter);

	template <typename Counter>
	void collect(const Counter &c);

	const QRegExp m_regexp;
	CVmEvent *m_event;
};

Collector::Collector(QString filter, CVmEvent &event) :
	m_regexp(fixFilter(filter)),
	m_event(&event)
{
}

QString Collector::fixFilter(QString filter)
{
	if (filter.isEmpty())
		return ".*";

	filter.replace('.', "\\.");
	filter.replace('*', ".*");
	filter.replace('#', "[0-9]+");

	return filter;
}

void Collector::collectCt(const QString &uuid,
		const Ct::Statistics::Aggregate &a)
{
	using namespace Ct::Counter;

	DAO<Meter> d;
	Meter m = d.get(uuid);
	Cpu(a.getCpu()).record(m);
	d.set(uuid, m);

	collect(SingleCounter<GuestUsage>(m));
	collect(SingleCounter<GuestTimeDelta>(m));
	collect(SingleCounter<HostTimeDelta>(m));
	collect(SingleCounter<DiskRead>(a.getDisk()));
	collect(SingleCounter<DiskWrite>(a.getDisk()));
	collect(SingleCounter<Classful>(a.getNetworkClassful()));
	collect(SingleCounter<NetBytesIn>(a.getNetworkClassful()));
	collect(SingleCounter<NetBytesOut>(a.getNetworkClassful()));
	collect(SingleCounter<NetPacketsIn>(a.getNetworkClassful()));
	collect(SingleCounter<NetPacketsOut>(a.getNetworkClassful()));
	collect(SingleCounter<MemoryUsed>(a.getMemory()));
	collect(SingleCounter<MemoryCached>(a.getMemory()));

	quint32 vcpunum = getVcpuNum(uuid);
	for (quint32 i = 0; i < vcpunum; ++i)
		collect(VCpuTime(a.getCpu(), i, vcpunum));
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

	collect(SingleCounter<GuestUsage>(t));
	collect(SingleCounter<GuestTimeDelta>(t));
	collect(SingleCounter<HostTimeDelta>(t));

	collect(SingleCounter<Reclaimable>(ct));
	collect(SingleCounter<MemoryUsed>(ct));
	collect(SingleCounter<MemoryCached>(ct));

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

	CVmEventParameter *p = c.getValue();
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
		SmartPtr< ::Ct::Statistics::Aggregate> a(CVzHelper::get_env_stat(sVmUuid));
		if (!a.isValid())
			return SmartPtr<CSystemStatistics>();
		DAO<Meter> d;
		Meter m = d.get(sVmUuid);
		Ct::Counter::Cpu(a->getCpu()).record(m);
		d.set(sVmUuid, m);

		SmartPtr<CSystemStatistics> output(new CSystemStatistics(a->getSystem()));
		CCpuStatistics *cpu = new CCpuStatistics();
		CNetIfaceStatistics *net = new CNetIfaceStatistics();

		Ct::Statistics::Cpu c = a->getCpu();
		cpu->setTotalTime(c.uptime);
		cpu->setUserTime(c.user / (getHostCpus() ?: 1));
		cpu->setSystemTime(c.system / (getHostCpus() ?: 1));
		cpu->setPercentsUsage(m.report().getPercent());


		using ::Ct::Counter::accumulateTraffic;
		const PRL_STAT_NET_TRAFFIC& r = a->getNetworkClassful();
		net->setInDataSize(accumulateTraffic(r.incoming));
		net->setOutDataSize(accumulateTraffic(r.outgoing));
		net->setInPkgsCount(accumulateTraffic(r.incoming_pkt));
		net->setOutPkgsCount(accumulateTraffic(r.outgoing_pkt));

		output->m_lstCpusStatistics.append(cpu);
		output->m_lstNetIfacesStatistics.append(net);
		output->m_lstDisksStatistics.append(new CDiskStatistics(a->getDisk()));
		output->getUptimeStatistics()->setOsUptime(a->getCpu().uptime);

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
		usage = Vm::Counter::Reclaimable::extractUint32(*ct.getPtr()) * 1024 * 1024;

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

