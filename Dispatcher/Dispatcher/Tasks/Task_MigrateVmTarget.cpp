///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmTarget.cpp
///
/// Target task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
/// Copyright (c) 2017-2020 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "CDspVm_p.h"
#include "CDspVmBrand.h"
#include "Interfaces/Debug.h"
#include "CDspTemplateStorage.h"
#include "CDspClientManager.h"
#include <prlcommon/HostUtils/PCSUtils.h>
#include <prlcommon/VirtualDisk/Qcow2Disk.h>
#include "Task_MigrateVmTarget.h"
#include "Task_RegisterVm.h"
#include "Task_CloneVm.h"
#include "Task_ChangeSID.h"
#include "CDspService.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"
#include "Task_ManagePrlNetService.h"
#include "Task_BackgroundJob.h"
#include "Libraries/PrlCommonUtils/CVmMigrateHelper.h"
#include "Task_MigrateVmTarget_p.h"
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/core/value.hpp>
#include <boost/phoenix/core/argument.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/phoenix/object/construct.hpp>
#include <boost/phoenix/bind/bind_function.hpp>
#include <boost/phoenix/bind/bind_function_object.hpp>

namespace Migrate
{
namespace Vm
{
namespace Pump
{
namespace Fragment
{
///////////////////////////////////////////////////////////////////////////////
// struct Format

Format::~Format()
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor<CtMigrateCmd>

const char* Flavor<CtMigrateCmd>::getData(const bin_type& bin_) const
{
	if (bin_.isValid() && bin_->header.buffersNumber > 1)
		return bin_->buffers[1].getImpl();

	return NULL;
}

qint64 Flavor<CtMigrateCmd>::getDataSize(const bin_type& bin_) const
{
	if (bin_.isValid() && bin_->header.buffersNumber > 1)
		return IODATAMEMBERCONST(bin_.getImpl())[1].bufferSize;

	return -1;
}

spice_type Flavor<CtMigrateCmd>::getSpice(const bin_type& bin_) const
{
	quint32 z = 0;
	SmartPtr<char> b;
	IOPackage::EncodingType t;
	if (!bin_.isValid() || !bin_->getBuffer(0, t, b, z))
		return boost::none;

	if (sizeof(short) != z)
		return boost::none;

	return QString().setNum(*(short* )b.getImpl());
}

bin_type Flavor<CtMigrateCmd>::
	assemble(const spice_type& spice_, const char* data_, qint64 size_) const
{
	bin_type output;
	if (!spice_)
		return output;

	bool k = false;
	short t = spice_->toShort(&k);
	if (!k) 
		return output;

	output = IOPackage::createInstance(CtMigrateCmd, 2);
	if (!output.isValid())
		return output;

	output->fillBuffer(0, IOPackage::RawEncoding, &t, sizeof(t));
	output->fillBuffer(1, IOPackage::RawEncoding, data_, size_);

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Packer

bin_type Packer::operator()()
{
	return getFormat().assemble(m_spice, NULL, 0);
}

bin_type Packer::operator()(const QByteArray& data_)
{
	if (data_.isEmpty())
		return bin_type();

	return getFormat().assemble(m_spice, data_.data(), data_.size());
}

bin_type Packer::operator()(QIODevice& source_)
{
	bin_type output;
	QByteArray b(source_.bytesAvailable(), 0);
	qint64 z = source_.read(b.data(), b.size());
	if (-1 == z)
	{
		WRITE_TRACE(DBG_FATAL, "read error: %s",
			qPrintable(source_.errorString()));
	}
	else
		output = (*this)(b);

	return output;
}

bin_type Packer::operator()(const QTcpSocket& source_)
{
	bin_type output;
	boost::optional<QString> b = QString::number(source_.peerPort());
	std::swap(m_spice, b);
	QVariant v = source_.property("channel");
	if (v.isValid())
	{
		quint32 p = v.toUInt();
		output = getFormat().assemble(m_spice, reinterpret_cast<char*>(&p), sizeof(p));
	}
	std::swap(m_spice, b);
	return output;
}

} // namespace Fragment

namespace Pull
{
///////////////////////////////////////////////////////////////////////////////
// struct WateringPot

qint64 WateringPot::getVolume() const
{
	qint64 output = m_format->getDataSize(m_load);
	if (-1 == output)
		output = 0;

	return output;
}

Prl::Expected<qint64, Flop::Event> WateringPot::operator()()
{
	if (!m_load.isValid())
		return Flop::Event(PRL_ERR_NO_DATA);

	const qint64 output = m_device->write(
			m_format->getData(m_load) + m_done,
			getLevel());

	if (output == -1)
	{
		WRITE_TRACE(DBG_FATAL, "write error: %s",
			qPrintable(m_device->errorString()));
		return Flop::Event(PRL_ERR_FAILURE);
	}
	m_done += output;
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Pouring

qint64 Pouring::getRemaining() const
{
	qint64 l = 0;
	foreach (const WateringPot& p, m_pots)
	{
		l += p.getLevel();
	}
	return l + m_portion;
}

Pouring::status_type Pouring::operator()()
{
	if (0 < m_portion)
		return status_type();

	while (!m_pots.isEmpty())
	{
		Prl::Expected<qint64, Flop::Event> x = m_pots.front()();
		if (x.isFailed())
			return x.error();

		m_portion += x.value();
		if (0 < m_pots.front().getLevel())
			break;

		m_pots.pop_front();
	}

	return status_type();
}

Pouring::status_type Pouring::account(qint64 value_)
{
	if (m_portion < value_)
		return Flop::Event(PRL_ERR_INVALID_ARG);

	m_portion -= value_;
	return status_type();
}

///////////////////////////////////////////////////////////////////////////////
// struct Queue

target_type Queue::dequeue()
{
	if (isEmpty())
		return state_type(Reading());
	if (isEof())
		return state_type(Success());

	QList<WateringPot> p;
	while (!isEmpty() && !isEof())
	{
		p << WateringPot(*m_format, Vm::Pump::Queue::dequeue(), *m_device);
	}
	return state_type(Pouring(p));
}

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Receipt

target_type Receipt::operator()
	(const boost::mpl::at_c<state_type::types, 0>::type& value_) const
{
	Q_UNUSED(value_);
	return m_queue->dequeue();
}

///////////////////////////////////////////////////////////////////////////////
// struct Accounting

target_type Accounting::operator()
	(boost::mpl::at_c<state_type::types, 3>::type value_) const
{
	Pouring x = value_();
	x.account(m_amount);
	if (0 < x.getRemaining())
		return state_type(x);

	return m_queue->dequeue();
}

///////////////////////////////////////////////////////////////////////////////
// struct Dispatch

target_type Dispatch::operator()
	(boost::mpl::at_c<state_type::types, 1>::type value_) const
{
	Prl::Expected<void, Flop::Event> x = value_();
	return x.isFailed() ? target_type(x.error()) :
		target_type(state_type(boost::phoenix::val(value_)));
}

target_type Dispatch::operator()
	(const boost::mpl::at_c<state_type::types, 2>::type& value_) const
{
	m_callback();
	return state_type(value_);
}

} // namespace Visitor
} // namespace Pull

namespace Push
{
///////////////////////////////////////////////////////////////////////////////
// struct Queue

Queue::Queue(const Fragment::Packer& packer_, IO& service_, QIODevice& device_):
	m_service(&service_), m_device(&device_), m_packer(packer_)
{
	m_collector.reserve(1<<22);
	setFormat(m_packer.getFormat());
}

Queue::enqueue_type Queue::enqueueEof()
{
	if (!m_collector.isEmpty())
	{
		enqueue_type x = enqueue();
		if (x.isFailed())
			return x;
	}
	return enqueue(m_packer());
}

Queue::enqueue_type Queue::enqueueData()
{
	Fragment::bin_type b = m_packer(*m_device);
	if (!b.isValid())
		return Flop::Event(PRL_ERR_FAILURE);

	const Fragment::Format& f = m_packer.getFormat();
	int a = m_collector.capacity() - m_collector.size();
	int d = f.getDataSize(b);
	if (Vm::Tunnel::libvirtChunk_type::s_command == b->header.type)
		WRITE_TRACE(DBG_FATAL, "Got a chunk from libvirt of size %d", d);

	int z = qMin(a, d);
	QBuffer u;
	u.setBuffer(&m_collector);
	if (!u.open(QIODevice::ReadWrite | QIODevice::Append))
	{
		WRITE_TRACE(DBG_FATAL, "cannot open QBuffer");
		return Flop::Event(PRL_ERR_UNEXPECTED);
	}
	u.write(f.getData(b), z);
	u.close();
	if (a != z)
		return enqueue_type();

	enqueue_type x = enqueue();
	if (x.isFailed())
		return x;
	
	d = d - z;
	if (0 == d)
		return enqueue_type();

	u.setBuffer(&m_collector);
	if (!u.open(QIODevice::ReadWrite | QIODevice::Append))
	{
		WRITE_TRACE(DBG_FATAL, "cannot open QBuffer");
		return Flop::Event(PRL_ERR_UNEXPECTED);
	}
	u.write(f.getData(b) + z, d);

	return enqueue_type();
}

target_type Queue::dequeue()
{
	if (!isEmpty())
	{
		IOSendJob::Handle j = m_service->sendPackage(head());
		if (!j.isValid())
			return Flop::Event(PRL_ERR_FAILURE);

		if (Vm::Tunnel::libvirtChunk_type::s_command == head()->header.type)
		{
			WRITE_TRACE(DBG_FATAL, "Write a libvirt chunk of size %lld",
				m_packer.getFormat().getDataSize(head()));
		}
		if (IOSendJob::SendQueueIsFull == m_service->getSendResult(j))
			return state_type(Sending());

		bool x = isEof();
		(void)Vm::Pump::Queue::dequeue();
		if (x)
			return state_type(Closing());

		return state_type(Sending());
	}
	if (m_collector.isEmpty())
		return state_type(Reading());

	enqueue_type x = enqueue();
	if (x.isFailed())
		return x.error();

	target_type output = dequeue();
	if (output.isSucceed() && !isEmpty())
	{
		// NB. the queue was empty, we cleared the collector
		// but the dequeue call didn't send anything thus
		// we remove the head item and revert m_collector
		// usage to the item size to retry on a next dequeue
		// call.
		m_collector.data_ptr()->size = m_packer.getFormat()
			.getDataSize(Vm::Pump::Queue::dequeue());
	}
	return output;
}

Queue::enqueue_type Queue::enqueue()
{
	enqueue_type output = enqueue(m_packer(m_collector));
	if (output.isSucceed())
		m_collector.data_ptr()->size = 0;
	
	return output;
}

Queue::enqueue_type Queue::enqueue(const_reference package_)
{
	if (package_.isValid())
	{
		Vm::Pump::Queue::enqueue(package_);
		return enqueue_type();
	}
	return Flop::Event(PRL_ERR_FAILURE);
}

namespace Visitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Eof

target_type Eof::operator()
	(const boost::mpl::at_c<state_type::types, 0>::type& value_) const
{
	Q_UNUSED(value_);
	Prl::Expected<void, Flop::Event> x = m_queue->enqueueEof();
	return x.isFailed() ? x.error() : m_queue->dequeue();
}

target_type Eof::operator()
	(const boost::mpl::at_c<state_type::types, 1>::type& value_) const
{
	Prl::Expected<void, Flop::Event> x = m_queue->enqueueEof();
	return x.isFailed() ? target_type(x.error()) : target_type(state_type(value_));
}

target_type Eof::operator()
	(const boost::mpl::at_c<state_type::types, 4>::type& value_) const
{
	Q_UNUSED(value_);
	Prl::Expected<void, Flop::Event> x = m_queue->enqueueData();
	if (x.isFailed())
		return x.error();

	return (*this)(boost::mpl::at_c<state_type::types, 1>::type());
}

///////////////////////////////////////////////////////////////////////////////
// struct Sent

target_type Sent::operator()
	(const boost::mpl::at_c<state_type::types, 1>::type& value_) const
{
	Q_UNUSED(value_);
	return m_queue->dequeue();
}

target_type Sent::operator()
	(const boost::mpl::at_c<state_type::types, 2>::type& value_) const
{
	if (m_callback.empty())
		return state_type(value_);

	m_callback();
	return state_type(Success());
}

target_type Sent::operator()
	(const boost::mpl::at_c<state_type::types, 4>::type& value_) const
{
	Q_UNUSED(value_);
	Prl::Expected<void, Flop::Event> x = m_queue->enqueueData();
	if (x.isFailed())
		return x.error();

	return (*this)(boost::mpl::at_c<state_type::types, 1>::type());
}

///////////////////////////////////////////////////////////////////////////////
// struct Ready

target_type Ready::operator()
	(const boost::mpl::at_c<state_type::types, 0>::type& value_) const
{
	Q_UNUSED(value_);
	Prl::Expected<void, Flop::Event> x = m_queue->enqueueData();
	return x.isFailed() ? x.error() : m_queue->dequeue();
}

target_type Ready::operator()
	(const boost::mpl::at_c<state_type::types, 1>::type& value_) const
{
	if (5 < m_queue->size())
		return target_type(state_type(Obstruction()));

	Prl::Expected<void, Flop::Event> x = m_queue->enqueueData();
	return x.isFailed() ? target_type(x.error()) : target_type(state_type(value_));
}

} // namespace Visitor
} // namespace Push
} // namespace Pump

namespace Target
{
namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Pstorage

const char Pstorage::suffix[] = "pstorage-target";

Pstorage::Pstorage(const QStringList& files_)
{
	foreach(const QString& checkFile, files_)
	{
		QFileInfo i(checkFile);
		if (i.exists())
			m_sharedDirs << i.dir().absolutePath();
	}
}

bool Pstorage::monkeyPatch(CVmConfiguration& config_)
{
	// patch enabled shared disks on pstorage
	foreach(CVmHardDisk* d, config_.getVmHardwareList()->m_lstHardDisks)
	{
		if (isNotMigratable(*d) && !patch(*d))
			return false;
	}
	return true;
}

bool Pstorage::patch(CVmHardDisk& disk_)
{
	QString path = disk_.getSystemName();
	QString patchedPath = disguise(path);
	if (!createBackedImage(disk_, patchedPath))
	{
		WRITE_TRACE(DBG_DEBUG, "unable to create backed image %s",
			qPrintable(patchedPath));
		return false;
	}

	disk_.setSystemName(patchedPath);
	disk_.setUserFriendlyName(patchedPath);

	WRITE_TRACE(DBG_DEBUG, "patched path %s", qPrintable(patchedPath));
	return true;
}

bool Pstorage::isNotMigratable(const CVmHardDisk& disk_) const
{
	// check if disk has a problem with shared migration because of Pstorage
	if (disk_.getConnected() == PVE::DeviceDisconnected
		|| disk_.getEnabled() == PVE::DeviceDisabled)
		return false;

	return pcs_fs(qPrintable(disk_.getSystemName())) &&
		m_sharedDirs.contains(QFileInfo(disk_.getSystemName()).dir().absolutePath());
}

bool Pstorage::createBackedImage(const CVmHardDisk& disk_, const QString& path_) const
{
	VirtualDisk::qcow2PolicyList_type p;
	p.push_back(VirtualDisk::Policy::Qcow2::base_type(disk_.getSystemName()));
	p.push_back(VirtualDisk::Policy::Qcow2::size_type(disk_.getSize() << 20));

	return PRL_SUCCEEDED(VirtualDisk::Qcow2::create(path_, p));
}

QList<CVmHardDisk> Pstorage::getDisks(const CVmConfiguration& config_) const
{
	QList<CVmHardDisk> disks;
	foreach(CVmHardDisk d, config_.getVmHardwareList()->m_lstHardDisks)
	{
		if (!isNotMigratable(d))
			continue;

		d.setSystemName(disguise(d.getSystemName()));
		disks << d;
	}
	return disks;
}

void Pstorage::cleanup(const CVmConfiguration& config_) const
{
	foreach(CVmHardDisk* d, config_.getVmHardwareList()->m_lstHardDisks)
	{
		if (isNotMigratable(*d))
			QFile::remove(disguise(d->getSystemName()));
	}
}

} // namespace Libvirt

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct IO

IO::IO(CDspDispConnection& io_): m_io(&io_)
{
	bool x;
	x = this->connect(
		m_io,
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(reactReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::QueuedConnection);
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
	x = this->connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(reactDisconnected(IOSender::Handle)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
	x = this->connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onAfterSend(IOServerInterface*, IOSender::Handle,
			    IOSendJob::Result, const SmartPtr<IOPackage>)),
		SLOT(reactSend(IOServerInterface*, IOSender::Handle,
			    IOSendJob::Result, const SmartPtr<IOPackage>)),
		Qt::QueuedConnection);
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
}

IO::~IO()
{
	m_io->disconnect(SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this, SLOT(reactReceived(IOSender::Handle, const SmartPtr<IOPackage>)));
	CDspService::instance()->getIOServer().disconnect(
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this, SLOT(reactDisconnected(IOSender::Handle)));
	CDspService::instance()->getIOServer().disconnect(
		SIGNAL(onAfterSend(IOServerInterface*, IOSender::Handle,
			    IOSendJob::Result, const SmartPtr<IOPackage>)), this,
		SLOT(reactSend(IOServerInterface*, IOSender::Handle,
			    IOSendJob::Result, const SmartPtr<IOPackage>))
		);
}

IOSendJob::Handle IO::sendPackage(const SmartPtr<IOPackage>& package_)
{
	return m_io->sendPackage(package_);
}

IOSendJob::Result IO::getSendResult(const IOSendJob::Handle& job_)
{
	return CDspService::instance()->getIOServer().getSendResult(job_);
}

void IO::reactReceived(IOSender::Handle handle_, const SmartPtr<IOPackage>& package_)
{
	if (handle_ == m_io->GetConnectionHandle())
		emit onReceived(package_);
}

void IO::reactDisconnected(IOSender::Handle handle_)
{
	if (handle_ == m_io->GetConnectionHandle())
		emit disconnected();
}

void IO::reactSend(IOServerInterface*, IOSender::Handle handle_,
	    IOSendJob::Result, const SmartPtr<IOPackage> package_)
{
	if (handle_ == m_io->GetConnectionHandle())
		emit onSent(package_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Socket<QTcpSocket>

bool Socket<QTcpSocket>::isConnected(const type& socket_)
{
	return socket_.state() == QAbstractSocket::ConnectedState;
}

void Socket<QTcpSocket>::disconnect(type& socket_)
{
	socket_.disconnectFromHost();
}

} // namespace Tunnel

#ifdef __USE_ISOCXX11
#else // __USE_ISOCXX11
namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

void Connector::reactState(unsigned, unsigned, QString vmUuid_, QString)
{
	WRITE_TRACE(DBG_DEBUG, "got change state event");
	if (m_uuid != vmUuid_)
		return;

	WRITE_TRACE(DBG_DEBUG, "handle change state event");
	handle(Tentative::Defined());
}

///////////////////////////////////////////////////////////////////////////////
// struct Tentative

template <typename Event, typename FSM>
void Tentative::on_entry(const Event&, FSM&)
{
	CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
	bool x;
	getConnector()->setUuid(m_uuid);
	x = getConnector()->connect(s.getPtr(),
		SIGNAL(signalVmStateChanged(unsigned, unsigned, QString, QString)),
		SLOT(reactState(unsigned, unsigned, QString, QString)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
}

template <typename Event, typename FSM>
void Tentative::on_exit(const Event&, FSM&)
{
	CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
	s->disconnect(SIGNAL(signalVmStateChanged(unsigned, unsigned, QString, QString)),
			getConnector(), SLOT(reactState(unsigned, unsigned, QString, QString)));
	getConnector()->setUuid(QString());
}

} // namespace Libvirt

namespace Content
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template <typename Event, typename FSM>
void Frontend::on_entry(const Event& event_, FSM& fsm_)
{
	vsd::Frontend<Frontend>::on_entry(event_, fsm_);
	std::pair<CVmFileListCopySender*, CVmFileListCopyTarget*> pair = m_task->createCopier();
	m_sender.reset(pair.first);
	m_copier.reset(pair.second);
}

template <typename Event, typename FSM>
void Frontend::on_exit(const Event& event_, FSM& fsm_)
{
	vsd::Frontend<Frontend>::on_exit(event_, fsm_);
	// order is significant
	m_copier.reset();
	m_sender.reset();
}

void Frontend::copy(const CopyCommand_type& event_)
{
	bool f = false;
	PRL_RESULT e = m_copier->handlePackage(event_.getPackage(), &f);
	if (!f)
		return;
	if (PRL_FAILED(e))
	{
		static_cast<boost::msm::back::state_machine<Frontend> &>(*this)
			.process_event(Flop::Event(e));
	}
	else
	{
		m_task->setDefaultVmPermissions();
		static_cast<boost::msm::back::state_machine<Frontend> &>(*this)
			.process_event(Good());
	}
}

} // namespace Content

namespace Tune
{

///////////////////////////////////////////////////////////////////////////////
// struct Perform

Perform::Perform(Task_MigrateVmTarget& task_, VIRTUAL_MACHINE_STATE state_)
{
	if (VMS_STOPPED != state_)
		return;

	m_work = QSharedPointer<work_type>(new work_type());
	m_editor = QSharedPointer<editor_type>(new editor_type
		(task_.getVmUuid(), task_.getClient(), *CDspService::instance()));

	bool (*u)(CVmConfiguration&) = &CCpuHelper::update;
	m_work->connect(boost::bind<void>(u, _1));
	if (PVMT_CLONE_MODE & task_.getRequestFlags() && !task_.isTemplate())
	{
		namespace bp = boost::phoenix;
		m_work->connect(bp::bind(&Task_CloneVm::ResetNetSettings,
			bp::construct<SmartPtr<CVmConfiguration> >
				(&bp::placeholders::arg1, SmartPtrPolicy::DoNotReleasePointee)));
	}
}

template <typename Event, typename FSM>
void Perform::on_entry(const Event& event_, FSM& fsm_)
{
	Trace<Perform>::on_entry(event_, fsm_);
	if (m_work.isNull() || m_work->empty())
		return;

	namespace bp = boost::phoenix;
	typedef editor_type::action_type::result_type result_type;

	CVmConfiguration x;
	boost::function<result_type (CVmConfiguration& )> a =
		(
			bp::bind(bp::ref(*m_work), bp::placeholders::arg1),
			bp::ref(x) = bp::placeholders::arg1,
			bp::construct<result_type>()
		);

	if (PRL_FAILED((*m_editor)(a)))
	{
		fsm_.process_event(Flop::Event(PRL_ERR_FAILURE));
		return;
	}

	::Libvirt::Instrument::Agent::Filter::List filter_list(::Libvirt::Kit.getLink());
	const QList<CVmGenericNetworkAdapter* >& adapters =
			x.getVmHardwareList()->m_lstNetworkAdapters;
	// define filters for non-template VMs on offline migration
	if (!x.getVmSettings()->getVmCommonOptions()->isTemplate())
	{
		if(filter_list.define(adapters).isFailed())
		{
			fsm_.process_event(Flop::Event(PRL_ERR_FAILURE));
			return;
		}
	}

	if(::Libvirt::Kit.vms().getGrub(x).spawnPersistent().isFailed())
		fsm_.process_event(Flop::Event(PRL_ERR_FAILURE));
}
} // namespace Tune

namespace Commit
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

void Connector::reactFinished()
{
	handle(Perform::Done());
}

///////////////////////////////////////////////////////////////////////////////
// struct Perform

template <typename Event, typename FSM>
void Perform::on_entry(const Event& event_, FSM& fsm_)
{
	Trace<Perform>::on_entry(event_, fsm_);
	if (NULL == m_helper)
	{
		fsm_.process_event(Done());
		return;
	}

	::Libvirt::Instrument::Agent::Vm::Block::Launcher s =
		::Libvirt::Kit.vms().at(m_config->getVmIdentification()->getVmUuid()).getVolume();

	m_receiver = QSharedPointer<receiver_type>(new receiver_type());
	if (getConnector()->connect(m_receiver.data(),
			SIGNAL(done()),
			SLOT(reactFinished())))
		m_merge = s.merge(m_helper->getDisks(*m_config), *m_receiver);
	else
	{
		WRITE_TRACE(DBG_FATAL, "can't connect commit future");
		m_receiver.clear();
	}
}

template <typename Event, typename FSM>
void Perform::on_exit(const Event& event_, FSM& fsm_)
{
	Trace<Perform>::on_exit(event_, fsm_);
	m_merge.stop();
	if (!m_receiver.isNull())
	{
		m_receiver->disconnect(SIGNAL(done()), getConnector(), SLOT(reactFinished()));
		m_receiver.clear();
		::Libvirt::Kit.vms().at(m_config->getVmIdentification()->getVmUuid())
			.getMaintenance().emitDefined();
	}
}

} // namespace Commit

namespace Start
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend::Action

void Frontend::Action::operator()(const msmf::none&, Frontend& fsm_,
		acceptingState_type&, Preparing& target_)
{
	target_.setVolume(fsm_.m_task->getImagesToCreate());
}

void Frontend::Action::operator()(const boost::mpl::true_&, Frontend& fsm_,
		Preparing&, Success&)
{
	PRL_RESULT e = fsm_.m_task->sendStartConfirmation();
	if (PRL_FAILED(e))
		fsm_.getConnector()->handle(Flop::Event(e));
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

void Frontend::create(const CVmHardDisk& event_)
{
	QDir d(QFileInfo(event_.getSystemName()).dir());

	if (!d.exists() && !CFileHelper::WriteDirectory(d.path(),
				&m_task->getClient()->getAuthHelper()))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory",
				qPrintable(d.path()));
		getConnector()->handle(Flop::Event(PRL_ERR_VM_MIGRATE_CANNOT_CREATE_DIRECTORY));
		return;
	}
	QStringList a;
	a << "create" << "-f" << "qcow2";
	a << "-o" << "cluster_size=1M,lazy_refcounts=on";
	a << event_.getSystemName();
	a << QString::number(event_.getSize()).append("M");

	getConnector()->launch("qemu-img", a);
}

} // namespace Start

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct Socket<QLocalSocket>

bool Socket<QLocalSocket>::isConnected(const type& socket_)
{
	return socket_.state() == QLocalSocket::ConnectedState;
}

void Socket<QLocalSocket>::disconnect(type& socket_)
{
	socket_.disconnectFromServer();
}

QSharedPointer<QLocalSocket> Socket<QLocalSocket>::craft(connector_type& connector_)
{
	bool x;
	QSharedPointer<type> output = QSharedPointer<type>
		(new QLocalSocket(), &QObject::deleteLater);
	x = connector_.connect(output.data(), SIGNAL(connected()),
		SLOT(reactConnected()), Qt::QueuedConnection);
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect local socket connect");
	x = connector_.connect(output.data(), SIGNAL(disconnected()),
		SLOT(reactDisconnected()), Qt::QueuedConnection);
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect local socket disconnect");

	return output;
}

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Channel

template <typename FSM>
void Channel::on_entry(ioEvent_type const& event_, FSM& fsm_)
{
	def_type::on_entry(event_, fsm_);
	getSocket()->connectToServer("/var/run/libvirt/libvirt-sock");
}

} // namespace Libvirt
} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector

void Connector::cancel()
{
	handle(Flop::Event(PRL_ERR_OPERATION_WAS_CANCELED));
}

void Connector::disconnected()
{
	handle(Flop::Event(PRL_ERR_FAILURE));
}

void Connector::react(const SmartPtr<IOPackage>& package_)
{
	if (IS_FILE_COPY_PACKAGE(package_->header.type))
		return handle(CopyCommand_type(package_));

	WRITE_TRACE(DBG_DEBUG, "react package %d.", package_->header.type);
	switch (package_->header.type)
	{
	case VmMigrateStartCmd:
		return handle(StartCommand_type(package_));
	case VmMigrateLibvirtTunnelChunk:
		return handle(Vm::Tunnel::libvirtChunk_type(package_));
	case VmMigrateConnectQemuStateCmd:
		return handle(Vm::Pump::Event<VmMigrateConnectQemuStateCmd>(package_));
	case VmMigrateQemuStateTunnelChunk:
		return handle(Vm::Pump::Event<VmMigrateQemuStateTunnelChunk>(package_));
	case VmMigrateConnectQemuDiskCmd:
		return handle(Vm::Pump::Event<VmMigrateConnectQemuDiskCmd>(package_));
	case VmMigrateQemuDiskTunnelChunk:
		return handle(Vm::Pump::Event<VmMigrateQemuDiskTunnelChunk>(package_));
	case VmMigrateFinishCmd:
		return handle(Vm::Pump::FinishCommand_type(package_));
	case VmMigrateCancelCmd:
		return cancel();
	default:
		WRITE_TRACE(DBG_FATAL, "Unexpected package type: %d.", package_->header.type);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Synch

void Synch::send(Tunnel::IO& io_, Connector& connector_) const
{
	SmartPtr<IOPackage> p = IOPackage::createInstance(Vm::Pump::FinishCommand_type::s_command, 1);
	if (!p.isValid())
		return connector_.handle(Flop::Event(PRL_ERR_FAILURE));

	if (!io_.sendPackage(p).isValid())
		return connector_.handle(Flop::Event(PRL_ERR_FAILURE));
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template <typename Event, typename FSM>
void Frontend::on_entry(const Event& event_, FSM& fsm_)
{
	bool x;
	vsd::Frontend<Frontend>::on_entry(event_, fsm_);
	x = getConnector()->connect(m_task, SIGNAL(cancel()), SLOT(cancel()),
		Qt::QueuedConnection);
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect cancle");
	x = getConnector()->connect(m_io,
			SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
			SLOT(react(const SmartPtr<IOPackage>&)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect react on received");
	x = getConnector()->connect(m_io, SIGNAL(disconnected()), SLOT(disconnected()),
		Qt::QueuedConnection);
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect disconnect");
}

template <typename Event, typename FSM>
void Frontend::on_exit(const Event& event_, FSM& fsm_)
{
	vsd::Frontend<Frontend>::on_exit(event_, fsm_);
	m_task->disconnect(SIGNAL(cancel()), getConnector(), SLOT(cancel()));
	m_io->disconnect(SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
			getConnector(), SLOT(react(const SmartPtr<IOPackage>&)));
	m_io->disconnect(SIGNAL(disconnected()), getConnector(), SLOT(disconnected()));
}

void Frontend::setResult(const Flop::Event& value_)
{
	if (value_.isFailed())
		boost::apply_visitor(Flop::Visitor(*m_task), value_.error());
}

bool Frontend::isTemplate(const msmf::none&)
{
	return m_task->isTemplate();
}

bool Frontend::isSwitched(const msmf::none&)
{
	return m_task->getRequestFlags() & PVMT_SWITCH_TEMPLATE;
}
#endif // __USE_ISOCXX11

} // namespace Target
} // namespace Vm
} // namespace Migrate

#define VM_MIGRATE_START_CMD_WAIT_TIMEOUT 600 * 1000

Task_MigrateVmTarget::Task_MigrateVmTarget(Registry::Public& registry_,
		const SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p)
:CDspTaskHelper(pDispConnection->getUserSession(), p),
Task_DispToDispConnHelper(getLastError()),
m_registry(registry_),
m_dispConnection(pDispConnection),
m_pCheckPackage(p),
m_cDstHostInfo(CDspService::instance()->getHostInfo()->data()),
m_nFlags(0),
m_nSteps(0),
m_nBundlePermissions(0),
m_nConfigPermissions(0)
{
	CVmMigrateCheckPreconditionsCommand * pCheckCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);

	m_hConnHandle = pDispConnection->GetConnectionHandle();

	m_sHaClusterId = pCheckCmd->GetStorageInfo();
	m_sSharedFileName = pCheckCmd->GetSharedFileName();
	m_nRequiresDiskSpace = pCheckCmd->GetRequiresDiskSpace();
	m_nMigrationFlags = pCheckCmd->GetMigrationFlags();
	m_nReservedFlags = pCheckCmd->GetReservedFlags();
	m_nVersion = pCheckCmd->GetVersion();
	m_sVmConfig = pCheckCmd->GetVmConfig();
	m_sVmName = pCheckCmd->GetTargetVmName();

	m_sSrcHostInfo = pCheckCmd->GetSourceHostHardwareInfo();
	if (m_nVersion >= MIGRATE_DISP_PROTO_V4)
		m_lstCheckFilesExt = pCheckCmd->GetSharedFileNamesExtra();
	if (m_nVersion >= MIGRATE_DISP_PROTO_V3)
		m_nPrevVmState = pCheckCmd->GetVmPrevState();
	if (pCheckCmd->GetTargetVmHomePath().isEmpty())
		m_sVmDirPath = m_dispConnection->getUserSession()->getUserDefaultVmDirPath();
	else
		m_sVmDirPath = pCheckCmd->GetTargetVmHomePath();
	/* initialize all vars from pCheckCmd - after exit from constructor package buffer will invalid */

	moveToThread(this);
}

Task_MigrateVmTarget::~Task_MigrateVmTarget()
{
}

/* process VmMigrateCheckPreconditionsCommand */
PRL_RESULT Task_MigrateVmTarget::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
	bool bSharedStorage = false;

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sVmDirPath;
	QString bundle;

	if (PVMT_CHANGE_SID & getRequestFlags() && !(PVMT_CLONE_MODE & getRequestFlags()))
	{
		WRITE_TRACE(DBG_FATAL, "Changing SID without clone is rejected!");
		nRetCode = PRL_ERR_INVALID_ARG;
		goto exit;
	}
	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - VM migrate rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration(m_sVmConfig));
	if (PRL_FAILED(m_pVmConfig->m_uiRcInit))
	{
		nRetCode = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Wrong VM condiguration was received: [%s]", QSTR2UTF8(m_sVmConfig));
		goto exit;
	}

	if (m_sVmName.isEmpty())
		m_sVmName = m_pVmConfig->getVmIdentification()->getVmName();

	m_pVmConfig->getVmIdentification()->setVmName(m_sVmName);

	/*  to get target VM home directory path */
	m_sTargetVmHomePath = QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath();
	bundle = QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).dir().dirName();

	m_sVmUuid = m_sOriginVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();
	m_sVmDirUuid = getClient()->getVmDirectoryUuid();
	if (!(m_nReservedFlags & PVM_DONT_COPY_VM) && (PVMT_CLONE_MODE & getRequestFlags()))
	{
		m_pVmConfig->getVmIdentification()->setVmUuid(m_sVmUuid = Uuid::createUuid().toString());
		m_pVmConfig->getVmIdentification()->setCtId(QString::number(Uuid::toVzid(m_sVmUuid)));
		// change bundle name for cloned VM
		bundle = Vm::Config::getVmHomeDirName(m_sVmUuid);
	}
	m_cSrcHostInfo.fromString(m_sSrcHostInfo);
	if (PRL_FAILED(m_cSrcHostInfo.m_uiRcInit))
	{
		nRetCode = PRL_ERR_PARSE_HOST_HW_INFO;
		WRITE_TRACE(DBG_FATAL, "Wrong source host hw info was received: [%s]", QSTR2UTF8(m_sSrcHostInfo));
		goto exit;
	}

	if ( !(PVMT_CLONE_MODE & getRequestFlags())
		&& m_sVmDirPath == m_dispConnection->getUserSession()->getUserDefaultVmDirPath()
		&& (bSharedStorage = (checkSharedStorage() == PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED)))
	{
		m_sVmConfigPath = m_pVmConfig->getVmIdentification()->getHomePath();
	}
	else
	{
		m_sTargetVmHomePath = QString("%1/%2").arg(m_sVmDirPath)
			.arg(bundle.isEmpty() ? Vm::Config::getVmHomeDirName(m_sVmUuid) : bundle);
		m_sTargetVmHomePath = QFileInfo(m_sTargetVmHomePath).absoluteFilePath();
		bSharedStorage = (checkSharedStorage() == PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED);
		m_sVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath);
		m_pVmConfig->getVmIdentification()->setHomePath(m_sVmConfigPath);
	}

	/* lock Vm exclusive parameters */
	m_pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
			m_sVmUuid, m_sVmConfigPath, m_sVmName));

	nRetCode = CDspService::instance()->getVmDirManager()
			.checkAndLockNotExistsExclusiveVmParameters(QStringList(), m_pVmInfo.getImpl());
	if (PRL_FAILED(nRetCode))
	{
		switch (nRetCode)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
			WRITE_TRACE(DBG_FATAL, "UUID '%s' already registered", QSTR2UTF8(m_pVmInfo->vmUuid));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
			WRITE_TRACE(DBG_FATAL, "name '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED:
			WRITE_TRACE(DBG_FATAL, "container '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				QSTR2UTF8(m_pVmInfo->vmUuid), QSTR2UTF8(m_pVmInfo->vmName), QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
		}
		goto exit;
	}
	m_nFlags |= MIGRATE_VM_EXCL_PARAMS_LOCKED;

	if (!(PVMT_SWITCH_TEMPLATE & getRequestFlags())) {
		/* skip checking for copy to template case (https://jira.sw.ru/browse/PSBM-9597) */
		checkTargetCpusNumber();
	}

	CVmMigrateHelper::buildNonSharedDisks(m_lstCheckFilesExt,
			m_pVmConfig->getVmHardwareList()->m_lstHardDisks,
			m_lstNonSharedDisks);

	if (!(m_nMigrationFlags & PVMT_DONT_CREATE_DISK))
	{
		// check that external disk paths are not exist
		foreach (const QString & disk, m_lstNonSharedDisks) {
			if (!QFileInfo(disk).exists())
				continue;
			WRITE_TRACE(DBG_FATAL,
				"External disk file '%s' already exists.", QSTR2UTF8(disk));
			nRetCode = CDspTaskFailure(*this)
					(PRL_ERR_VM_MIGRATE_EXT_DISK_DIR_ALREADY_EXISTS_ON_TARGET,
						disk);
			goto exit;
		}
	}

	{
		/*
		   Shared storage should be checked before target VM home existence
		   because latest will fail if storage is shared.
		   checkSharedStorage can return PRL_ERR_SUCCESS or PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED only.
		   In last case event with code PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED will added into m_lstCheckPrecondsErrors.
		   Source side will check m_lstCheckPrecondsErrors.
		*/
		if (!bSharedStorage)
		{
			/* it is not shared based Vm */
			if (!(m_nMigrationFlags & PVMT_IGNORE_EXISTING_BUNDLE) && QDir(m_sTargetVmHomePath).exists())
			{
				nRetCode = PRL_ERR_VM_MIGRATE_VM_HOME_ALREADY_EXISTS_ON_TARGET;
				WRITE_TRACE(DBG_FATAL,
					"The virtual machine home directory '%s' already exists.",
					QSTR2UTF8(m_sTargetVmHomePath));
				CVmEvent *pEvent = getLastError();
				pEvent->setEventCode(PRL_ERR_VM_MIGRATE_VM_HOME_ALREADY_EXISTS_ON_TARGET);
				pEvent->addEventParameter(new CVmEventParameter(PVE::String,
					m_sTargetVmHomePath,
					EVT_PARAM_MESSAGE_PARAM_0));
				goto exit;
			}
			checkRequiresDiskSpace();
		}
		else
			// add config to shared check list
			m_lstCheckFilesExt << m_sVmConfigPath;
	}

	m_pVmConfig->getVmHardwareList()->RevertDevicesPathToAbsolute(m_sTargetVmHomePath);
	if (m_pVmConfig->getVmSettings()->getVmStartupOptions() != NULL)
	{
		CVmStartupBios* b = m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios();
		if (b != NULL && !b->getNVRAM().isEmpty() && QDir::isRelativePath(b->getNVRAM()))
			b->setNVRAM(QDir(m_sTargetVmHomePath).absoluteFilePath(b->getNVRAM()));
	}

	m_pVmConfig->getVmIdentification()->setVmName(m_sVmName);

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}
	if (m_nPrevVmState == VMS_RUNNING || m_nPrevVmState == VMS_PAUSED)
	{
		if (!isTemplate())
			m_vcmmd.reset(new vcmmd_type(m_sVmUuid));
		if (m_nVersion < MIGRATE_DISP_PROTO_V9)
			m_pstorage.reset(new pstorage_type(m_lstCheckFilesExt));
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MigrateVmTarget::reactStart(const SmartPtr<IOPackage> &package)
{
	if (package->header.type != VmMigrateStartCmd)
	{
		WRITE_TRACE(DBG_FATAL, "unexpected package type: %d", package->header.type);
		return PRL_ERR_OPERATION_FAILED;
	}

	CDispToDispCommandPtr a = CDispToDispProtoSerializer::ParseCommand(package);
	CVmMigrateStartCommand *cmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(a);

	m_nMigrationFlags = cmd->GetMigrationFlags();
	m_nReservedFlags = cmd->GetReservedFlags();
	m_nBundlePermissions = cmd->GetBundlePermissions();
	m_nConfigPermissions = cmd->GetConfigPermissions();

	if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) ) {
		/* to create Vm bundle */
		if (!CFileHelper::WriteDirectory(m_sTargetVmHomePath, &getClient()->getAuthHelper())) {
			WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(m_sTargetVmHomePath));
			return CDspTaskFailure(*this)
				(PRL_ERR_VM_MIGRATE_CANNOT_CREATE_DIRECTORY,
					m_sTargetVmHomePath);
		}
		/* set original permissions to Vm bundle (https://jira.sw.ru/browse/PSBM-8269) */
		if (m_nBundlePermissions) {
			QFile vmBundle(m_sTargetVmHomePath);
			if (!vmBundle.setPermissions((QFile::Permissions)m_nBundlePermissions)) {
				WRITE_TRACE(DBG_FATAL,
					"[%s] Cannot set permissions for Vm bundle \"%s\", will use default",
					__FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			}
		}
		if (PVMT_CLONE_MODE & getRequestFlags())
		{
			PRL_RESULT e = ::Vm::Private::Brand(m_sTargetVmHomePath, getClient())
					.stamp();
			if (PRL_FAILED(e))
				return e;
		}
	}

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	m_nSteps |= MIGRATE_STARTED;

	if (!(m_nReservedFlags & PVM_DONT_COPY_VM))
	{
		if (PRL_FAILED(nRetCode = saveVmConfig()))
			return nRetCode;
	}
	/* will register resource on HA cluster just before
	   register CT on node : better to have a broken resource than
	   losing a valid */
	if (PRL_FAILED(nRetCode = registerHaClusterResource()))
		return nRetCode;

	if (PRL_FAILED(nRetCode = registerVmBeforeMigration()))
		return nRetCode;

	if (isTemplate())
		return PRL_ERR_SUCCESS;


	// define vzfilters for running VMs in case of running VM migration
	if (m_nPrevVmState == VMS_RUNNING)
	{
		const QList<CVmGenericNetworkAdapter* >& adapters =
					m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters;
		::Libvirt::Instrument::Agent::Filter::List filter_list(::Libvirt::Kit.getLink());
		::Libvirt::Result ret;
		if ((ret = filter_list.define(adapters)).isFailed())
			return ret.error().code();
	}

	WRITE_TRACE(DBG_DEBUG, "declare VM UUID:%s, Dir UUID:%s, Config:%s",
		qPrintable(m_sVmUuid), qPrintable(m_sVmDirUuid), qPrintable(m_sVmConfigPath));
	if (PRL_FAILED(nRetCode = m_registry.declare(CVmIdent(m_sVmUuid, m_sVmDirUuid), m_sVmConfigPath)))
		return nRetCode;

	return PRL_ERR_SUCCESS;
}

void Task_MigrateVmTarget::changeSID()
{
	// nobody requested SID change
	if (!(PVMT_CHANGE_SID & getRequestFlags()) ||
		!(PVMT_CLONE_MODE & getRequestFlags()))
		return;

	// VM is on shared storage
	if (m_nReservedFlags & PVM_DONT_COPY_VM)
		return;

	// SID is Windows feature
	if (m_pVmConfig->getVmSettings()->getVmCommonOptions()->getOsType() != PVS_GUEST_TYPE_WINDOWS)
		return;

	// no need to change SID for templates - it will be changed on
	// deployment from it
	if (isTemplate())
		return;

	CProtoCommandPtr pRequest = CProtoSerializer::CreateProtoBasicVmCommand(
				PVE::DspCmdVmChangeSid,
				m_pVmConfig->getVmIdentification()->getVmUuid(), 0);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmChangeSid, pRequest);
	CDspService::instance()->getTaskManager()
		.schedule(new Task_ChangeSID(getClient(), pPackage, m_pVmConfig)).wait();
}

void Task_MigrateVmTarget::finalizeTask()
{
	IOSendJob::Handle hJob;
	SmartPtr<IOPackage> pPackage;

	// delete temporary registration
	if (m_nFlags & MIGRATE_VM_EXCL_PARAMS_LOCKED)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		if (!m_vcmmd.isNull())
			m_vcmmd->commit();

		// leave previously running VM in suspended state
		if (m_nPrevVmState == VMS_RUNNING && (m_nMigrationFlags & PVM_DONT_RESUME_VM))
			m_nPrevVmState = VMS_SUSPENDED;

		if (m_nPrevVmState == VMS_RUNNING)
		{
			Task_ManagePrlNetService::addVmIPAddress(m_pVmConfig);
			Task_ManagePrlNetService::updateVmNetworking(m_pVmConfig, true);
			announceMacAddresses(m_pVmConfig);
		}

		if (isTemplate())
		{
			CDspService::instance()->getVmStateSender()->
				onVmStateChanged(VMS_MIGRATING, VMS_STOPPED, m_sVmUuid, m_sVmDirUuid, false);
		}

		PRL_EVENT_TYPE evtType;
		/* restore Vm previous state */
		switch (m_nPrevVmState)
		{
		case VMS_RUNNING:
			evtType = PET_DSP_EVT_VM_STARTED;
			break;
		case VMS_PAUSED:
			evtType = PET_DSP_EVT_VM_PAUSED;
			break;
		case VMS_SUSPENDED:
			evtType = PET_DSP_EVT_VM_SUSPENDED;
			break;
		case VMS_STOPPED:
		default:
			evtType = PET_DSP_EVT_VM_STOPPED;
			break;
		}

		CVmEvent cStateEvent(evtType, m_sVmUuid, PIE_DISPATCHER);
		SmartPtr<IOPackage> pUpdateVmStatePkg =
			DispatcherPackage::createInstance( PVE::DspVmEvent, cStateEvent.toString());
		CDspService::instance()->getClientManager().
			sendPackageToVmClients(pUpdateVmStatePkg, m_sVmDirUuid, m_sVmUuid);

		if (m_nPrevVmState == VMS_STOPPED || m_nPrevVmState == VMS_SUSPENDED)
			 changeSID();

		if (m_nVersion >= MIGRATE_DISP_PROTO_V3) {
			/* notify source task about target task finish,
			   will send reply to check precondition request. https://jira.sw.ru/browse/PSBM-9596 */
			m_dispConnection->sendSimpleResponse(m_pCheckPackage, PRL_ERR_SUCCESS);
		}
		/* migration initiator wait event from original Vm uuid */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_FINISHED, m_sOriginVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sOriginVmUuid);
	}
	else
	{
		if (m_pVmConfig.isValid() && !m_pstorage.isNull())
			m_pstorage->cleanup(*m_pVmConfig);
		/* It is not possible to get Vm client list after deleteVmDirectoryItem(), and
		   sendPackageToVmClients() does not work. Get list right now and will use
		   sendPackageToClientList() call (https://jira.sw.ru/browse/PSBM-9159) */
		QList< SmartPtr<CDspClient> > clientList =
			CDspService::instance()->getClientManager().
				getSessionListByVm(m_sVmDirUuid, m_sVmUuid).values();

		unregisterHaClusterResource();

		/* if migration was not started, do not cleanup anything -
		   we can remove 'already existed Vm' */
		if (m_nSteps & MIGRATE_STARTED)
		{
			if (!isTemplate()
				&& PRL_FAILED(m_registry.undeclare(m_sVmUuid)))
				WRITE_TRACE(DBG_FATAL, "Unable to undeclare VM after migration fail");

			if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
				CFileHelper::ClearAndDeleteDir(m_sTargetVmHomePath);
			foreach (const QString & disk, m_lstNonSharedDisks)
				CFileHelper::RemoveEntry(disk, &getClient()->getAuthHelper());

			// Unregister VM dir item
			CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(m_sVmDirUuid, m_sVmUuid);

			CVmEvent cDelEvent(PET_DSP_EVT_VM_DELETED, m_sVmUuid, PIE_DISPATCHER);
			SmartPtr<IOPackage> pDelPackage =
					DispatcherPackage::createInstance(PVE::DspVmEvent, cDelEvent.toString());
			CDspService::instance()->getClientManager().sendPackageToClientList(
					pDelPackage, clientList);

			if (!isTemplate())
			{
				// cleanup vz-filters
				const QList<CVmGenericNetworkAdapter* >& adapters =
						m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters; 
				::Libvirt::Instrument::Agent::Filter::List filter_list(::Libvirt::Kit.getLink());
				filter_list.undefine(adapters, true);
			}
		}
		/* migration initiator wait event from original Vm uuid */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED, m_sOriginVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToClientList(pPackage, clientList);

		hJob = m_dispConnection->sendResponseError(getLastError(), getRequestPackage());
		CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);
	}
}

PRL_RESULT Task_MigrateVmTarget::sendStartConfirmation()
{
	QString p;
	if ((m_nPrevVmState == VMS_RUNNING) || (m_nPrevVmState == VMS_PAUSED))
	{}
	else
	{
		quint64 m = m_pVmConfig->getVmHardwareList()->getMemory()->getRamSize();
		quint64 v = m_pVmConfig->getVmHardwareList()->getVideo()->getMemorySize();
		p = VirtuozzoDirs::getVmMemoryFileLocation(
			m_sVmUuid, m_sTargetVmHomePath,
			m_pVmConfig->getVmSettings()->getVmCommonOptions()->getSwapDir(),
			CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->getSwapPathForVMOnNetworkShares(),
			false, (m << 20) + (v << 20));
	}
	CDispToDispCommandPtr a = CDispToDispProtoSerializer::CreateVmMigrateReply(p);;
	SmartPtr<IOPackage> b = DispatcherPackage::createInstance(
		a->GetCommandId(), a->GetCommand()->toString(), m_pStartPackage);

	return m_dispConnection->sendPackageResult(b);
}

bool Task_MigrateVmTarget::isSharedDisk(const QString& name) const
{
	return name.startsWith(m_sTargetVmHomePath + QDir::separator()) ? m_nReservedFlags & PVM_DONT_COPY_VM
		: !m_lstNonSharedDisks.contains(name);
}

QList<CVmHardDisk> Task_MigrateVmTarget::getImagesToCreate()
{
	QList<CVmHardDisk> output;
	if (!(m_nMigrationFlags & PVMT_DONT_CREATE_DISK) &&
			((m_nPrevVmState == VMS_RUNNING) || (m_nPrevVmState == VMS_PAUSED)))
	{
		CVmHardware h(m_pVmConfig->getVmHardwareList());
		h.RevertDevicesPathToAbsolute(m_sTargetVmHomePath);
		foreach (const CVmHardDisk* d, h.m_lstHardDisks)
		{
			if (PVE::HardDiskImage != d->getEmulatedType() ||
				d->getConnected() != PVE::DeviceConnected)
				continue;

			QString name = d->getSystemName();
			if  (!isSharedDisk(name))
			{
				WRITE_TRACE(DBG_INFO, "non shared disk %s", qPrintable(name));
				output << *d;
			}
			else
				WRITE_TRACE(DBG_INFO, "shared disk %s", qPrintable((name)));
		}
	}
	return output;
}

// cancel command
void Task_MigrateVmTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	emit cancel();
}

void Task_MigrateVmTarget::checkTargetCpusNumber()
{
	if (m_pVmConfig->getVmHardwareList()->getCpu()->getNumber() > m_cDstHostInfo.getCpu()->getNumber())
	{
		//Fill additional error info
		CVmEvent cEvent;
		cEvent.setEventCode(PRL_ERR_VM_MIGRATE_NOT_ENOUGH_CPUS_ON_TARGET);
		//Add required by VM CPUs number
		cEvent.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
			QString("%1").arg(m_pVmConfig->getVmHardwareList()->getCpu()->getNumber()),
			EVT_PARAM_MESSAGE_PARAM_0));
		//Add available at host CPUs number
		cEvent.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
			QString("%1").arg(m_cDstHostInfo.getCpu()->getNumber()),
			EVT_PARAM_MESSAGE_PARAM_1));
		m_lstCheckPrecondsErrors.append(cEvent.toString());
		WRITE_TRACE(DBG_FATAL,
			"Not enough CPUs : VM requires %d CPUs, target server has %d only.",
			m_pVmConfig->getVmHardwareList()->getCpu()->getNumber(),
			m_cDstHostInfo.getCpu()->getNumber());
	}
}

void Task_MigrateVmTarget::checkRequiresDiskSpace()
{
	PRL_RESULT nRetCode;
	PRL_UINT64 nFreeSpace = 0;

	if (m_nRequiresDiskSpace == 0)
		return;

	nRetCode = CFileHelper::GetDiskAvailableSpace( m_sVmDirPath, &nFreeSpace );
	if ( PRL_FAILED(nRetCode) ) {
		WRITE_TRACE(DBG_FATAL,
			"CFileHelper::GetDiskAvailableSpace(%s) return %#x[%s], disk space check will skipped",
			QSTR2UTF8(m_sTargetVmHomePath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return;
	}

	if (nFreeSpace < m_nRequiresDiskSpace)
	{
		WRITE_TRACE(DBG_FATAL,
			"Not enough disk space on the destination server: requires %llu MB, available %llu MB",
			m_nRequiresDiskSpace/1024/1024, nFreeSpace/1024/1024);
		//Fill additional error info
		CVmEvent cEvent;
		cEvent.setEventCode(PRL_ERR_VM_MIGRATE_NOT_ENOUGH_DISK_SPACE_ON_TARGET);
		//Add required by VM disk space in megabytes
		cEvent.addEventParameter(new CVmEventParameter(PVE::UInt64,
			QString("%1").arg(m_nRequiresDiskSpace/1024/1024),
			EVT_PARAM_MESSAGE_PARAM_0));
		// Add available disk space in megabytes
		cEvent.addEventParameter(new CVmEventParameter(PVE::UInt64,
			QString("%1").arg(nFreeSpace/1024/1024),
			EVT_PARAM_MESSAGE_PARAM_1));
		m_lstCheckPrecondsErrors.append(cEvent.toString());
	}
}

PRL_RESULT Task_MigrateVmTarget::checkSharedStorage()
{
	PRL_RESULT output = PRL_ERR_SUCCESS;
	do
	{
		::Template::Storage::Dao::pointer_type c;
		::Template::Storage::Dao d(getClient()->getAuthHelper());
		if (PRL_SUCCEEDED(d.findForEntry(m_sTargetVmHomePath, c)))
		{
			output = PRL_ERR_VM_MIGRATE_CANNOT_REMOTE_CLONE_SHARED_VM;
			break;
		}
		if (m_sSharedFileName.isEmpty())
		{
			WRITE_TRACE(DBG_FATAL,"Missed filename for shared storage check!");
			break;
		}

		QFileInfo sharedFileInfo(QDir(m_sTargetVmHomePath), m_sSharedFileName);
		if (!sharedFileInfo.exists())
		{
			if (CVmMigrateHelper::isInsideSharedVmPrivate(m_sTargetVmHomePath))
			{
				output = PRL_ERR_VM_MIGRATE_TARGET_INSIDE_SHARED_VM_PRIVATE;
				break;
			}
			WRITE_TRACE(DBG_FATAL,
				    "Found no %s file on target server. Storage is NOT shared.",
				    QSTR2UTF8(sharedFileInfo.absoluteFilePath()));
			break;
		}
		output = PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED;
		WRITE_TRACE(DBG_FATAL,
			"Found %s file on target server. Storage IS shared.", QSTR2UTF8(m_sSharedFileName));
	}
	while(false);


	if (PRL_ERR_SUCCESS != output)
	{
		//Fill additional error info
		CVmEvent e;
		e.setEventCode(output);
		m_lstCheckPrecondsErrors.append(e.toString());
	}
	return output;
}

PRL_RESULT Task_MigrateVmTarget::registerVmBeforeMigration()
{
	CVmDirectoryItem *pVmDirItem = new CVmDirectoryItem;

	pVmDirItem->setVmUuid(m_sVmUuid);
	pVmDirItem->setRegistered(PVE::VmRegistered);
	pVmDirItem->setValid(PVE::VmValid);
	pVmDirItem->setVmHome(m_sVmConfigPath);
	pVmDirItem->setVmName(m_sVmName);
	// FIXME set private according to actual flag value within VM config
	pVmDirItem->setIsPrivate(PVE::VmPublic);
	pVmDirItem->setRegisteredBy(getClient()->getUserName());
	pVmDirItem->setRegDateTime(QDateTime::currentDateTime());
	pVmDirItem->setChangedBy(getClient()->getUserName());
	pVmDirItem->setChangeDateTime(QDateTime::currentDateTime());
	if (isTemplate())
	{
		pVmDirItem->setTemplate(true);
		m_sVmDirUuid = CDspVmDirManager::getTemplatesDirectoryUuid();
	}
	PRL_RESULT e = PRL_ERR_SUCCESS;
	do
	{
		if (PRL_FAILED(e = DspVm::vdh().insertVmDirectoryItem(m_sVmDirUuid, pVmDirItem)))
		{
			WRITE_TRACE(DBG_FATAL, "Can't insert vm %s into VmDirectory by error %#x, %s",
				QSTR2UTF8(m_sVmUuid), e, PRL_RESULT_TO_STRING(e));
			break;
		}
		if (!m_vcmmd.isNull() && PRL_FAILED(e = (*m_vcmmd)(::Vcmmd::Unregistered(m_pVmConfig))))
		{
			WRITE_TRACE(DBG_FATAL, "Can't register vm %s in vcmmd error %#x, %s",
				QSTR2UTF8(m_sVmUuid), e, PRL_RESULT_TO_STRING(e));
			break;
		}
		/* Notify clients that new VM appeared */
		CVmEvent event(PET_DSP_EVT_VM_ADDED, m_sVmUuid, PIE_DISPATCHER );
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event);
		DspVm::cm().sendPackageToVmClients(p, m_sVmDirUuid, m_sVmUuid);

		/* Notify clients that VM migration started - and clients wait message with original Vm uuid */
		SmartPtr<CVmEvent> pEvent = SmartPtr<CVmEvent>(
			new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sOriginVmUuid, PIE_DISPATCHER));
		pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "false", EVT_PARAM_MIGRATE_IS_SOURCE));
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());
		/* and notify clients about VM migration start event */
		CDspService::instance()->getVmStateSender()->
			onVmStateChanged(VMS_STOPPED, VMS_MIGRATING, m_sVmUuid, m_sVmDirUuid, false);
		DspVm::cm().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sOriginVmUuid);

		return PRL_ERR_SUCCESS;
	} while(false);

	WRITE_TRACE(DBG_FATAL,
		"Error occurred while register Vm %s with code [%#x][%s]",
		QSTR2UTF8(m_sVmUuid), e, PRL_RESULT_TO_STRING(e));

	return CDspTaskFailure(*this).setCode(PRL_ERR_VM_MIGRATE_REGISTER_VM_FAILED)
			(m_sVmName, PRL_RESULT_TO_STRING(e));
}

PRL_RESULT Task_MigrateVmTarget::saveVmConfig()
{
	// Try to create empty configuration file
	if (!CFileHelper::CreateBlankFile(m_sVmConfigPath, &getClient()->getAuthHelper()))
	{
		WRITE_TRACE(DBG_FATAL,
			"Couldn't to create blank VM config by path '%s'", QSTR2UTF8(m_sVmConfigPath));
		return PRL_ERR_SAVE_VM_CONFIG;
	}

	QString sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid();
	QString sLastServerUuid = m_pVmConfig->getVmIdentification()->getServerUuid();
	m_pVmConfig->getVmIdentification()->setVmUuid( m_sVmUuid );
	m_pVmConfig->getVmIdentification()->setServerUuid(sServerUuid);
	m_pVmConfig->getVmIdentification()->setLastServerUuid(sLastServerUuid);

	if (PVMT_SWITCH_TEMPLATE & getRequestFlags())
		m_pVmConfig->getVmSettings()->getVmCommonOptions()->setTemplate(!isTemplate());

	if (PVMT_CLONE_MODE & getRequestFlags())
	{
		m_pVmConfig->getVmIdentification()->setSourceVmUuid( m_sOriginVmUuid );
		if (isTemplate())
			Task_CloneVm::ResetNetSettings(m_pVmConfig);
	}
	/**
	* reset additional parameters in VM configuration
	* (VM home, last change date, last modification date - never store in VM configuration itself!)
	*/
	CDspService::instance()->getVmDirHelper().resetAdvancedParamsFromVmConfig(m_pVmConfig);

	PRL_RESULT nRetCode = CDspService::instance()->getVmConfigManager().saveConfig(
				m_pVmConfig, m_sVmConfigPath, getClient(), true, true);
	if (PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL, "Can't save VM config by error %#x, %s",
			    nRetCode, PRL_RESULT_TO_STRING(nRetCode) );

		return CDspTaskFailure(*this).setCode(PRL_ERR_SAVE_VM_CONFIG)(m_sVmUuid, m_sTargetVmHomePath);
	}
	/* set original permissions of Vm config (https://jira.sw.ru/browse/PSBM-8333) */
	if (m_nConfigPermissions) {
		QFile vmConfig(m_sVmConfigPath);
		if (!vmConfig.setPermissions((QFile::Permissions)m_nConfigPermissions)) {
			WRITE_TRACE(DBG_FATAL,
				"Cannot set permissions for Vm config \"%s\", will use default",
				QSTR2UTF8(m_sVmConfigPath));
		}
	}
	return PRL_ERR_SUCCESS;
}

// add/move resource on HA cluster
PRL_RESULT Task_MigrateVmTarget::registerHaClusterResource()
{
	PRL_RESULT nRetCode;

	if (isTemplate())
		return PRL_ERR_SUCCESS;

	CVmHighAvailability* pHighAvailability = m_pVmConfig->getVmSettings()->getHighAvailability();

	if (!pHighAvailability->isEnabled())
		return PRL_ERR_SUCCESS;

	if (m_nReservedFlags & PVM_HA_MOVE_VM) {
		// move resource from source node
		nRetCode = CDspService::instance()->getHaClusterHelper()->moveFromClusterResource(
					m_sVmName, m_sHaClusterId);
	} else {
		// add resource
		nRetCode = CDspService::instance()->getHaClusterHelper()->addClusterResource(
				m_sVmName, pHighAvailability, m_sTargetVmHomePath);
	}
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	m_nSteps |= MIGRATE_HA_RESOURCE_REGISTERED;
	return PRL_ERR_SUCCESS;
}

// rollback resource on HA cluster
void Task_MigrateVmTarget::unregisterHaClusterResource()
{
	if (!(m_nSteps & MIGRATE_HA_RESOURCE_REGISTERED))
		return;

	if (!m_pVmConfig->getVmSettings()->getHighAvailability()->isEnabled())
		return;

	if (m_nReservedFlags & PVM_HA_MOVE_VM) {
		// resource was moved - return to source node
		CDspService::instance()->getHaClusterHelper()->moveToClusterResource(m_sVmName, m_sHaClusterId);
	} else {
		// resource was added
		CDspService::instance()->getHaClusterHelper()->removeClusterResource(m_sVmName, false);
	}
}

PRL_RESULT Task_MigrateVmTarget::preconditionsReply()
{
	SmartPtr<CVmConfiguration> C(m_pVmConfig);
	CVmMigrateCheckPreconditionsReply c(m_lstCheckPrecondsErrors, m_lstNonSharedDisks, m_nFlags);
	if (!m_pstorage.isNull())
	{
		C = SmartPtr<CVmConfiguration>(new CVmConfiguration(m_pVmConfig.getImpl()));
		if (!m_pstorage->monkeyPatch(*C))
		{
			setLastErrorCode(PRL_ERR_OPERATION_FAILED);
			return PRL_ERR_OPERATION_FAILED;
		}
	}
	// use compatibility mode in response
	C->setExtRootTagName(C->getLegacyProductTag(C->getTagName()));
	c.SetConfig(C->toString());

	WRITE_TRACE(DBG_DEBUG, "hardware config that was recieved from the target:\n %s",
		qPrintable(C->getVmHardwareList()->toString()));

	SmartPtr<IOPackage> package = DispatcherPackage::createInstance(
			c.GetCommandId(), c.GetCommand()->toString(), m_pCheckPackage);
	setLastErrorCode(m_dispConnection->sendPackageResult(package));
	return getLastErrorCode();
}

PRL_RESULT Task_MigrateVmTarget::run_body()
{
#ifdef __USE_ISOCXX11
#else // __USE_ISOCXX11
	namespace mvt = Migrate::Vm::Target;
	typedef boost::msm::back::state_machine<mvt::Frontend> backend_type;

	mvt::Tunnel::IO io(*m_dispConnection);

	backend_type::Syncing syncingStep(boost::msm::back::states_
		<< boost::mpl::at_c<backend_type::syncing_type::initial_state, 0>::type(boost::cref(m_sVmUuid))
		<< boost::mpl::at_c<backend_type::syncing_type::initial_state, 1>::type(~0));

	backend_type::Moving moveStep(boost::msm::back::states_
		<< syncingStep
		<< boost::mpl::at_c<backend_type::moving_type::initial_state, 1>::type(boost::ref(io)));

	backend_type machine(boost::msm::back::states_
		<< Migrate::Vm::Finished(*this)
		<< backend_type::Starting(boost::msm::back::states_
			<< backend_type::Starting::acceptingState_type(
				boost::bind(&Task_MigrateVmTarget::reactStart, this, _1),
	                        VM_MIGRATE_START_CMD_WAIT_TIMEOUT),
			boost::ref(*this))
		<< backend_type::Copying(boost::ref(*this))
		<< moveStep
		<< syncingStep
		<< backend_type::Tuning(boost::ref(*this), m_nPrevVmState)
		<< backend_type::Commiting(boost::ref(*m_pVmConfig), m_pstorage.data())
		<< backend_type::Synch::State(~0),
		boost::ref(*this), boost::ref(io), boost::ref(*m_pVmConfig)
		);
	(Migrate::Vm::Walker<backend_type>(machine))();

	machine.start();
	if (PRL_SUCCEEDED(preconditionsReply()))
		exec();

	machine.stop();
#endif // __USE_ISOCXX11

	return getLastErrorCode();
}

std::pair<CVmFileListCopySender*, CVmFileListCopyTarget*> Task_MigrateVmTarget::createCopier()
{
	CVmFileListCopySender* sender = new CVmFileListCopySenderServer(
			CDspService::instance()->getIOServer(),
			m_dispConnection->GetConnectionHandle());

	/* CVmFileListCopyTarget will use Vm uuid in progress messages for clients, so will use original Vm uuid */
	CVmFileListCopyTarget* copier = new CVmFileListCopyTarget(
			sender,
			m_sOriginVmUuid,
			m_sTargetVmHomePath,
			NULL, m_nTimeout);

	return std::make_pair(sender, copier);
}

void Task_MigrateVmTarget::setDefaultVmPermissions()
{
	Mixin_CreateVmSupport().setDefaultVmPermissions(getClient(), m_sVmConfigPath, true);
}

