//////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVm.cpp
///
/// Source task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
/// Copyright (c) 2017-2023 Virtuozzo International GmbH, All rights reserved.
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

#include "CDspTaskTrace.h"
#include "CDspClientManager.h"
#include "CDspVmBrand.h"
#include "Task_MigrateVm.h"
#include "Task_CloneVm.h"
#include "Task_ChangeSID.h"
#include "CDspService.h"
#include <prlcommon/PrlCommonUtilsBase/CFileHelper.h>
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/PrlCommonUtils/CVmMigrateHelper.h"
#include "Task_MigrateVmSource_p.h"
#include <boost/functional/factory.hpp>

namespace Migrate
{
namespace Vm
{
namespace Source
{
namespace Inspection
{
///////////////////////////////////////////////////////////////////////////////
// struct Report

void Report::account(PRL_RESULT problem_)
{
	CVmEvent x;
	x.setEventCode(problem_);

	m_result << x.toString();
}

void Report::account(PRL_RESULT problem_, const QString& text_)
{
	CVmEvent x;
	x.setEventCode(problem_);
	x.addEventParameter(new CVmEventParameter(PVE::String,
			text_, EVT_PARAM_MESSAGE_PARAM_0));

	m_result << x.toString();
}

void Report::account(PRL_RESULT problem_, const QString& text1_, const QString& text2_)
{
	CVmEvent x;
	x.setEventCode(problem_);
	x.addEventParameter(new CVmEventParameter(PVE::String,
			text1_, EVT_PARAM_MESSAGE_PARAM_0));
	x.addEventParameter(new CVmEventParameter(PVE::String,
			text2_, EVT_PARAM_MESSAGE_PARAM_1));

	m_result << x.toString();
}

namespace Clone
{
///////////////////////////////////////////////////////////////////////////////
// struct Linked

bool Linked::upsets(request_type::first_type request_)
{
	const CVmConfiguration* x = request_->getVmConfig();
	return NULL != x && !x->getVmIdentification()->getLinkedVmUuid().isEmpty();
}

void Linked::lament(request_type::second_type report_)
{
	report_->account(PRL_ERR_UNIMPLEMENTED);
	WRITE_TRACE(DBG_FATAL, "Migration of linked clones is not supported");
}

///////////////////////////////////////////////////////////////////////////////
// struct Online

bool Online::upsets(request_type::first_type request_)
{
	const CVmConfiguration* x = request_->getVmConfig();
	return (PVMT_CLONE_MODE & request_->getRequestFlags() &&
		request_->getOldState() != VMS_STOPPED &&
		(x != NULL && !x->getVmSettings()->getVmCommonOptions()->isTemplate()));
}

void Online::lament(request_type::second_type report_)
{
	report_->account(PRL_ERR_VZ_OPERATION_FAILED,
		"Online migration in the clone mode is not supported. "
		"Stop the VM and try again.");
}

} // namespace Clone

///////////////////////////////////////////////////////////////////////////////
// struct ChangeSid

bool ChangeSid::upsets(request_type::first_type request_)
{
	const quint32 x = (PVMT_CLONE_MODE | PVMT_CHANGE_SID);
	if (x != (request_->getRequestFlags() & x))
		return false;

	return !Task_ChangeSID::canChangeSid(request_->getVmConfig());
}

void ChangeSid::lament(request_type::second_type report_)
{
	report_->account(PRL_ERR_CHANGESID_NOT_SUPPORTED);
}

namespace Hardware
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

void Builder::processCdrom()
{
	foreach (CVmOpticalDisk* d, m_model->m_lstOpticalDisks)
	{
		if (NULL == d)
			continue;
		if (!d->isRemote() || !(d->getEnabled() == PVE::DeviceEnabled &&
			d->getConnected() != PVE::DeviceDisconnected))
			continue;

		WRITE_TRACE(DBG_FATAL, "ERROR: Connected remote device");
		m_report.account(PRL_ERR_VM_MIGRATE_REMOTE_DEVICE_IS_ATTACHED,
			d->getSystemName());
	}
}

void Builder::processPci()
{
	QList<CVmDevice* > a;
	a += *((QList<CVmDevice*>* )&m_model->m_lstGenericPciDevices);
	a += *((QList<CVmDevice*>* )&m_model->m_lstNetworkAdapters);
	a += *((QList<CVmDevice*>* )&m_model->m_lstPciVideoAdapters);

	foreach (CVmDevice* d, a)
	{
		if (PVE::DeviceEnabled != d->getEnabled() || (
			d->getDeviceType() == PDE_GENERIC_NETWORK_ADAPTER &&
			d->getEmulatedType() != PDT_USE_DIRECT_ASSIGN))
			continue;

		WRITE_TRACE(DBG_DEBUG, "ERROR: Connected VTd device");
		m_report.account(PRL_ERR_VM_MIGRATE_REMOTE_DEVICE_IS_ATTACHED,
			d->getUserFriendlyName());
	}
}

void Builder::processMassStorage(const QString& name_)
{
	Q_UNUSED(name_);

	foreach(CVmHardDisk* d, m_model->m_lstHardDisks)
	{
		if (PDT_USE_REAL_DEVICE != (_PRL_VM_DEV_EMULATION_TYPE)d->getEmulatedType())
			continue;
		if (PVE::DeviceEnabled == d->getEnabled() && d->getConnected() == PVE::DeviceConnected)
		{
			WRITE_TRACE(DBG_FATAL, "Device %s has inappropriate emulated type %d",
				qPrintable(d->getUserFriendlyName()), d->getEmulatedType());
		}
	}
	foreach(CVmOpticalDisk* d, m_model->m_lstOpticalDisks)
	{
		if (PDT_USE_REAL_DEVICE != (_PRL_VM_DEV_EMULATION_TYPE)d->getEmulatedType())
			continue;
		if (PVE::DeviceEnabled == d->getEnabled() && d->getConnected() == PVE::DeviceConnected)
		{
			WRITE_TRACE(DBG_FATAL, "Device %s has inappropriate emulated type %d",
				qPrintable(d->getUserFriendlyName()), d->getEmulatedType());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Chain

Chain::result_type Chain::operator()(const request_type& request_)
{
	CVmHardware* h = NULL;
	const CVmConfiguration* x = request_.first->getVmConfig();
	if (NULL != x)
		h = x->getVmHardwareList();

	if (NULL == h)
	{
		WRITE_TRACE(DBG_FATAL, "Can not get the Vm hardware list");
		request_.second->account(PRL_ERR_OPERATION_FAILED);
	}
	else
	{
		Builder b(*h);
		b.processCdrom();
		b.processPci();
		b.processMassStorage(x->getVmIdentification()->getVmName());
		request_.second->account(b.getResult());
	}

	return base_type::operator()(request_);
}

} // namespace Hardware

namespace Reply
{
///////////////////////////////////////////////////////////////////////////////
// struct Efiw2k8

Efiw2k8::result_type Efiw2k8::operator()(const request_type& request_)
{
	const CVmSettings* x = NULL;
	if (NULL != request_.first->getVmConfig())
		x = request_.first->getVmConfig()->getVmSettings();
	if (MIGRATE_DISP_PROTO_V8 > m_version && x != NULL &&
		x->getVmStartupOptions()->getBios()->isEfiEnabled() &&
		x->getVmCommonOptions()->getOsVersion() <= PVS_GUEST_VER_WIN_2008)
	{
		request_.second->account(PRL_ERR_VMCONF_EFI_UNSUPPORTED_GUEST);
	}

	return Instrument::Chain::Unit<request_type>::operator()(request_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interpreter

Efiw2k8::result_type Interpreter::operator()(const request_type& request_) const
{
	QStringList x;
	foreach (const QString& i, m_object)
	{
		switch (CVmEvent(i).getEventCode())
		{
		case PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED:
			request_.first->setSharedStorage();
			break;
		case PRL_ERR_VM_MIGRATE_NON_COMPATIBLE_CPU_ON_TARGET:
			if (VMS_STOPPED == request_.first->getOldState() ||
				request_.first->getFlags() & PVM_DONT_RESUME_VM)
			{
				// error with CPUs incompatibility
				// so ignore it
				// If another error will be encountered in this loop, it will be added
				// to m_lstCheckPrecondsErrors and checked below...
				break;
			}
		default:
			x << i;
		}
	}
	request_.second->account(x);

	return PRL_ERR_SUCCESS;
}

} // namespace Reply
} // namespace Inspection

namespace Shadow
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

void Connector::reactSuccess()
{
	handle(boost::mpl::true_());
}

void Connector::reactFailure(const Flop::Event& reason_)
{
	handle(reason_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Beholder

template<class T>
Beholder<T>::~Beholder()
{
	if (m_task.isNull())
		return;

	m_watcher->disconnect(SIGNAL(finished()), this, SLOT(reactFinish()));
	m_task->cancel();
	m_watcher->waitForFinished();
}

template<class T>
Flop::Event Beholder<T>::operator()(T* task_)
{
	QSharedPointer<T> g(task_);
	if (!m_task.isNull())
	{
		WRITE_TRACE(DBG_FATAL, "task is not NULL");
		return Flop::Event(PRL_ERR_SERVICE_BUSY);
	}
	m_task = g;
	if (m_task.isNull())
	{
		WRITE_TRACE(DBG_FATAL, "task is NULL");
		return Flop::Event(PRL_ERR_INVALID_ARG);
	}
	m_watcher = QSharedPointer<QFutureWatcher<Flop::Event> >(new QFutureWatcher<Flop::Event>());
	bool x = this->connect(m_watcher.data(), SIGNAL(finished()), SLOT(reactFinish()));
	if (!x)
	{
		WRITE_TRACE(DBG_FATAL, "can't connect");
		return Flop::Event(PRL_ERR_FAILURE);
	}
	m_watcher->setFuture(QtConcurrent::run(boost::bind(&T::run, m_task.data())));
	return Flop::Event();
}

template<class T>
void Beholder<T>::reactFinish()
{
	m_task.clear();
	Flop::Event r = static_cast<QFutureWatcher<Flop::Event>* >(sender())->result();
	if (r.isFailed())
		emit failed(r);
	else
		emit completed();

}

///////////////////////////////////////////////////////////////////////////////
// struct State

template<class T, class U>
void State<T, U>::setBeholder(beholderPointer_type const& value_)
{
	if (!m_beholder.isNull())
	{
		m_beholder->disconnect(SIGNAL(completed()), m_beholder.data());
		m_beholder->disconnect(SIGNAL(failed(const Flop::Event&)), m_beholder.data());
	}
	m_beholder = value_;
	if (value_.isNull())
		return;

	this->getConnector()->connect(value_.data(), SIGNAL(completed()),
		SLOT(reactSuccess()), Qt::DirectConnection);
	this->getConnector()->connect(value_.data(), SIGNAL(failed(const Flop::Event&)),
		SLOT(reactFailure(Flop::Event)), Qt::DirectConnection);
}

} // namespace Shadow

namespace Content
{
///////////////////////////////////////////////////////////////////////////////
// struct Copier

Flop::Event Copier::execute(const itemList_type& folderList_, const itemList_type& fileList_)
{
	PRL_RESULT e = m_copier->Copy(folderList_, fileList_);
	if (PRL_SUCCEEDED(e))
	{
		if (PRL_SUCCEEDED(m_event->getEventCode()))
			return Flop::Event();
	}
	else
		m_event->setEventCode(e);

	return Flop::Event(m_event);
}

void Copier::cancel()
{
	m_copier->cancelOperation();
}

///////////////////////////////////////////////////////////////////////////////
// struct Task

Task::Task(Task_MigrateVmSource& task_, const itemList_type& folders_,
	const itemList_type &files_): m_files(&files_), m_folders(&folders_),
	m_copier(task_.createCopier())
{
}

Flop::Event Task::run()
{
	if (m_copier.isNull())
		return Flop::Event(PRL_ERR_UNINITIALIZED);

	return m_copier->execute(*m_folders, *m_files);
}
void Task::cancel()
{
	if (!m_copier.isNull())
		m_copier->cancel();
}

///////////////////////////////////////////////////////////////////////////////
// struct State

template<typename Event, typename FSM>
void State::on_entry(const Event& event_, FSM& fsm_)
{
	Shadow::State<State, Task>::on_entry(event_, fsm_);

	beholderPointer_type b = beholderPointer_type(new beholder_type());
	this->setBeholder(b);
	Flop::Event x = (*b)(m_factory());
	if (x.isFailed())
		this->getConnector()->handle(x);
}

} // namespace Content

namespace Libvirt
{
namespace Trick
{

///////////////////////////////////////////////////////////////////////////////
// struct Unit

const char Unit::suffix[] = "pstorage-source";

///////////////////////////////////////////////////////////////////////////////
// struct Decorator

::Libvirt::Result Decorator::execute()
{
	::Libvirt::Result r = do_();

	if (r.isFailed())
		return r;

	if (m_next.isNull() || (r = m_next->execute()).isSucceed())
		cleanup();
	else
		rollback();

	return r;
}

///////////////////////////////////////////////////////////////////////////////
// struct File

::Libvirt::Result File::do_()
{
	if (m_path.isEmpty())
		return ::Libvirt::Result();

	QString patchedFile = disguise(m_path);

	WRITE_TRACE(DBG_DEBUG, "hook file %s", qPrintable(m_path));
	if (QFileInfo(patchedFile).exists())
	{
		return Error::Simple(PRL_ERR_FAILURE, QString("unable to migrate %1: already exists").arg(m_path));
	}

	if (!QFile::rename(m_path, patchedFile))
	{
		WRITE_TRACE(DBG_DEBUG, "unable to rename %s", qPrintable(m_path));
		return Error::Simple(PRL_ERR_FAILURE, QString("unable to migrate %1: failed to rename").arg(m_path));
	}

	if (!QFile::copy(patchedFile, m_path))
	{
		WRITE_TRACE(DBG_DEBUG, "unable to copy %s", qPrintable(m_path));
		return Error::Simple(PRL_ERR_FAILURE, QString("unable to migrate %1: failed to copy").arg(m_path));
	}

	return ::Libvirt::Result();
}

void File::cleanup()
{
	if (m_path.isEmpty())
		return;

	QFile::remove(disguise(m_path));
}

void File::rollback()
{
	if (m_path.isEmpty())
		return;

	QFile::remove(m_path);
	QFile::rename(disguise(m_path), m_path);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disks

::Libvirt::Result Disks::do_()
{
	if (m_disks.isEmpty())
		return ::Libvirt::Result();

	WRITE_TRACE(DBG_DEBUG, "creating external snapshot");
	return m_agent.getSnapshot().createExternal(suffix, m_disks);
}

void Disks::cleanup()
{
	foreach (const CVmHardDisk* d, m_disks)
		QFile::remove(disguise(d->getSystemName()));
}

void Disks::rollback()
{
	QList<CVmHardDisk> disks;

	foreach (CVmHardDisk d, m_disks)
	{
		d.setSystemName(disguise(d.getSystemName()));
		disks << d;
	}

	WRITE_TRACE(DBG_DEBUG, "start merge and wait");
	::Libvirt::Instrument::Agent::Vm::Block::Completion r;
	::Libvirt::Instrument::Agent::Vm::Block::Activity a = m_agent.getVolume().merge(disks, r);
	r.wait();
	WRITE_TRACE(DBG_DEBUG, "finish merge");
	a.stop();
	WRITE_TRACE(DBG_DEBUG, "merge finished");
}

namespace Online
{
///////////////////////////////////////////////////////////////////////////////
// struct Separatist

QString Separatist::getNVRAM() const
{
	CVmSettings* a = m_source->getVmSettings();
	if (NULL == a)
		return QString();

	CVmStartupOptions* b = a->getVmStartupOptions();
	if (NULL == b)
		return QString();

	CVmStartupBios* c = b->getBios();
	if (NULL == c)
		return QString();

	QString r = c->getNVRAM();
	if (pcs_fs(qPrintable(r)))
		return r;

	return QString();
}

QList<CVmHardDisk*> Separatist::getDisks() const
{
	return refine(m_source->getVmHardwareList()->m_lstHardDisks);
}

QList<CVmSerialPort*> Separatist::getSerialPorts() const
{
	return refine(m_source->getVmHardwareList()->m_lstSerialPorts);
}
 
template<class T>
QList<T*> Separatist::refine(const QList<T*>& mix_)
{
	QList<T*> output;
	foreach (T* d, mix_)
	{
		if (d->getEnabled() == PVE::DeviceEnabled &&
			d->getConnected() == PVE::DeviceConnected &&
			pcs_fs(qPrintable(d->getSystemName())))
		{
			WRITE_TRACE(DBG_DEBUG, "%s is stored on vstorage", qPrintable(d->getSystemName()));
			output << d;
		}
		else
			WRITE_TRACE(DBG_DEBUG, "%s is stored NOT on vstorage", qPrintable(d->getSystemName()));
	}
	return output;
}
 
///////////////////////////////////////////////////////////////////////////////
// struct Component

::Libvirt::Result Component::execute()
{
	agent_type::result_type r = m_work();
	if (r.isFailed())
		return r.error();

	QMetaObject::invokeMethod(m_bus, "haul", Q_ARG(quint64, r.value()));
	return ::Libvirt::Result();
}
 
///////////////////////////////////////////////////////////////////////////////
// struct Plan

Plan::Plan(const Separatist& separatist_, Task_MigrateVmSource& context_):
	Separatist(separatist_)
{
	m_diskList = context_.getVmUnsharedDisks();
	diskList_type s = Separatist::getDisks();
	if (m_diskList.isEmpty())
		WRITE_TRACE(DBG_DEBUG, "there are no unshared disks to migrate");
	else
		m_flavor.setDeep();

	if (!s.isEmpty())
	{
		WRITE_TRACE(DBG_DEBUG, "there are disks on vstorage to migrate");
		m_diskSharedList = s.toSet().subtract(m_diskList.toSet()).toList();
		if (context_.getRemoteVersion() < MIGRATE_DISP_PROTO_V9)
		{
			WRITE_TRACE(DBG_DEBUG, "some shared disks will be migrated using snapshots");
			m_diskToSnapshotList = m_diskSharedList;
			m_diskList << m_diskSharedList;
			m_flavor.setShallow();
		}
		else if (m_diskList.isEmpty())
			m_flavor.setShared();
	}
	if (m_diskList.isEmpty() && m_diskSharedList.isEmpty())
		WRITE_TRACE(DBG_DEBUG, "there is no disk to migrate");
}

///////////////////////////////////////////////////////////////////////////////
// struct Hatchery

Unit* Hatchery::operator()(const agent_type& agent_, const CVmConfiguration& target_)
{
	const CVmConfiguration* config = m_task->getVmConfig();
	if (NULL == config)
		return NULL;

	Component::agent_type o(agent_);
	if (m_task->getFlags() & PVMT_UNCOMPRESSED)
		o.setUncompressed();

	if (m_ports)
		o.setQemuState(m_ports->first);

	Plan P(Separatist(*config), *m_task);
	m_ports ? o.setQemuDisk(P.getDisks(), m_ports->second) : o.setQemuDisk(P.getDisks());

	quint64 bw = m_task->getDegree();
	if (bw > 0)
		o.setBandwidth(bw);

	Unit* output = new Component(boost::bind(o, target_, P.getFlavor()), m_bus);
	if (!P.getDisksToSnapshot().isEmpty())
	{
		output = new Disks(P.getDisksToSnapshot(),
			::Libvirt::Kit.vms().at(m_task->getVmUuid()), output);
	}

	if (m_task->getRemoteVersion() < MIGRATE_DISP_PROTO_V9)
	{
		foreach(CVmSerialPort* p, P.getSerialPorts())
			output = new File(p->getSystemName(), output);
	}

	if (m_task->getRemoteVersion() < MIGRATE_DISP_PROTO_V10) {
		QString r = P.getNVRAM();
		if (!r.isEmpty())
			output = new File(r, output);
	}

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Carrier

void Carrier::haul(quint64 downtime_)
{
	m_target->handle(downtime_type(downtime_));
}

} // namespace Online

namespace Offline
{
///////////////////////////////////////////////////////////////////////////////
// struct Hatchery

Unit* Hatchery::operator()(const agent_type& agent_, const CVmConfiguration& target_) const
{
	WRITE_TRACE(DBG_DEBUG, "we use offline migration");

	return new Component(Component::agent_type(agent_), target_);
}

} // namespace Offline
} // namespace Trick

///////////////////////////////////////////////////////////////////////////////
// struct Task

Flop::Event Task::run()
{
	if (NULL == m_config)
		return Flop::Event(PRL_ERR_NO_DATA);

	QScopedPointer<Trick::Unit> work(m_factory(m_agent, *m_config));
	if (work.isNull())
	{
		WRITE_TRACE(DBG_DEBUG, "empty list of work for migration");
		return Flop::Event(PRL_ERR_FAILURE);
	}

	::Libvirt::Result r = work->execute();
	if (r.isSucceed())
		return Flop::Event();

	return Flop::Event(SmartPtr<CVmEvent>(new CVmEvent(r.error().convertToEvent())));
}

void Task::cancel()
{
	m_agent.cancel();
}

///////////////////////////////////////////////////////////////////////////////
// struct Progress

void Progress::report(quint16 value_)
{
	if (m_last != value_)
		m_reporter(m_last = value_);
}

void Progress::timerEvent(QTimerEvent* event_)
{
	if (NULL != event_)
		killTimer(event_->timerId());

	Prl::Expected<std::pair<quint64, quint64>, ::Error::Simple> p =
		m_agent.getProgress();
	if (p.isFailed())
		WRITE_TRACE(DBG_WARNING, "Can't read migration progress");
	else
	{
		quint16 x = 0;
		if (0 == p.value().first)
		{
			// not started
			x = 0;
		}
		else if (0 == p.value().second)
			x = 100;
		else
		{
			x = 100 - (100 * p.value().second + p.value().first - 1) /
				p.value().first;
			x = std::min<quint16>(99, x);
		}
		report(x);
		if (100 == x)
			return;
	}
	startTimer(100);
}

///////////////////////////////////////////////////////////////////////////////
// struct State

template<typename Event, typename FSM>
void State::on_exit(const Event& event_, FSM& fsm_)
{
	def_type::on_exit(event_, fsm_);
	m_beholder.clear();
	m_progress.clear();
}

template<typename FSM>
void State::on_exit(const boost::mpl::true_& event_, FSM& fsm_)
{
	def_type::on_exit(event_, fsm_);
	if (!m_progress.isNull())
	{
		m_progress->report(100);
		m_progress.clear();
	}
	m_beholder.clear();
}

Flop::Event State::start(serverList_type const& serverList_)
{
	if (m_beholder.isNull())
		return Flop::Event(PRL_ERR_UNINITIALIZED);
	if (3 > serverList_.size())
		return Flop::Event(PRL_ERR_INVALID_ARG);

	const CVmConfiguration* t = m_task->getTargetConfig();
	const QString u = QString("qemu+tcp://%1:%2/system")
				.arg(QHostAddress(QHostAddress::LocalHost).toString())
				.arg(serverList_.first()->serverPort());
	WRITE_TRACE(DBG_DEBUG, "Start migrate for uri: '%s'", QSTR2UTF8(u));
	Task::agent_type a = ::Libvirt::Kit.vms().at(m_task->getVmUuid()).getState().migrate(u);
	switch (m_task->getOldState())
	{
	case VMS_PAUSED:
	case VMS_RUNNING:
		break;
	default:
		return (*m_beholder)(new Task(a, boost::bind<Trick::Unit* >
			(Trick::Offline::Hatchery(), _1, _2),
 				m_task->getTargetConfig()));
	}
	m_progress = QSharedPointer<Progress>(new Progress(a, m_reporter),
			&QObject::deleteLater);
	m_progress->startTimer(0);
	m_progress->moveToThread(QCoreApplication::instance()->thread());

	Trick::Online::Hatchery f(*m_task, getConnector());
	if (0 == (m_task->getFlags() & PVMT_DIRECT_DATA_CONNECTION))
	{
		f.setPorts(qMakePair(serverList_.at(1)->serverPort(),
			serverList_.at(2)->serverPort()));
	}
	return (*m_beholder)(new Task(a, boost::bind<Trick::Unit* >(f, _1, _2), t));
}

} // namespace Libvirt

namespace Tunnel
{
///////////////////////////////////////////////////////////////////////////////
// struct Connector

void Connector::acceptLibvirt()
{
	QSharedPointer<QTcpSocket> s = accept_();
	if (!s.isNull())
	{
		((QTcpServer* )sender())->close();
		handle(Vm::Pump::Launch_type(getService(), s.data()));
	}
}

void Connector::acceptQemuDisk()
{
	handle(Qemu::Launch<Virtuozzo::VmMigrateConnectQemuDiskCmd>
		(getService(), accept_().data()));
}

void Connector::acceptQemuState()
{
	handle(Qemu::Launch<Virtuozzo::VmMigrateConnectQemuStateCmd>
		(getService(), accept_().data()));
}

QSharedPointer<QTcpSocket> Connector::accept_()
{
	QTcpServer* s = (QTcpServer* )sender();
	if (NULL == s)
		return QSharedPointer<QTcpSocket>();

	QSharedPointer<QTcpSocket> output(s->nextPendingConnection());
	output->setReadBufferSize(1 << 24);
	output->setProperty("channel", QVariant(uint(output->localPort())));
	handle(output);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct IO

IO::IO(IOClient& io_): m_io(&io_)
{
	bool x;
	x = this->connect(
		m_io, SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
		SLOT(reactReceived(const SmartPtr<IOPackage>)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
	x = this->connect(m_io,
		SIGNAL(onAfterSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage>)),
		SLOT(reactSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage>)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
	x = this->connect(m_io, SIGNAL(onStateChanged(IOSender::State)),
		SLOT(reactChange(IOSender::State)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
}

IO::~IO()
{
	m_io->disconnect(SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
		this, SLOT(reactReceived(const SmartPtr<IOPackage>)));
	m_io->disconnect(SIGNAL(onAfterSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage>)),
		this, SLOT(reactSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage>)));
	m_io->disconnect(SIGNAL(onStateChanged(IOSender::State)),
		this, SLOT(reactChange(IOSender::State)));
}

IOSendJob::Handle IO::sendPackage(const SmartPtr<IOPackage> &package)
{
	return m_io->sendPackage(package);
}

IOSendJob::Result IO::getSendResult(const IOSendJob::Handle& job_)
{
	return m_io->getSendResult(job_);
}

void IO::reactReceived(const SmartPtr<IOPackage>& package)
{
	emit onReceived(package);
}

void IO::reactSend(IOClientInterface*, IOSendJob::Result, const SmartPtr<IOPackage> package_)
{
	emit onSent(package_);
}

void IO::reactChange(IOSender::State value_)
{
	if (IOSender::Disconnected == value_)
		emit disconnected();
}

namespace Qemu
{
///////////////////////////////////////////////////////////////////////////////
// struct Launch

template<Virtuozzo::IDispToDispCommands X>
Prl::Expected<Vm::Pump::Launch_type, Flop::Event> Launch<X>::operator()() const
{
	using Vm::Pump::Fragment::Packer;
	Vm::Pump::Fragment::Flavor<X> f;
	SmartPtr<IOPackage> x = Packer(f)(*m_socket);
	if (!x.isValid())
		return Flop::Event(PRL_ERR_FAILURE);

	IOSendJob::Handle j = m_service->sendPackage(x);
	if (!j.isValid())
		return Flop::Event(PRL_ERR_FAILURE);

	return Vm::Pump::Launch_type(m_service, m_socket, f.getSpice(x));
}

} // namespace Qemu

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

void Frontend::accept(const client_type& connection_)
{
	m_clients << connection_;
}

bool Frontend::setup(const char* method_)
{
	bool x;
	listenerMap_type::mapped_type s(new QTcpServer());
	x = getConnector()->connect(s.data(), SIGNAL(newConnection()), method_);
	if (!x)
	{
		WRITE_TRACE(DBG_FATAL, "can't connect");
		return false;
	}
	x = s->listen(QHostAddress::LocalHost);
	if (!x)
	{
		WRITE_TRACE(DBG_FATAL, "can't listen");
		return false;
	}
	WRITE_TRACE(DBG_DEBUG, "Listen '%s' at localhost: %d", method_, s->serverPort());
	m_listenerMap[method_] = s;
	return true;
}

template <typename Event, typename FSM>
void Frontend::on_entry(const Event& event_, FSM& fsm_)
{
	vsd::Frontend<Frontend>::on_entry(event_, fsm_);
	getConnector()->setService(m_service);
	if (setup(SLOT(acceptLibvirt())) && setup(SLOT(acceptQemuState()))
		&& setup(SLOT(acceptQemuDisk())))
	{
		Libvirt::State::serverList_type listnerList;
		listnerList.push_back(m_listenerMap[SLOT(acceptLibvirt())]);
		listnerList.push_back(m_listenerMap[SLOT(acceptQemuState())]);
		listnerList.push_back(m_listenerMap[SLOT(acceptQemuDisk())]);
		return getConnector()->handle(listnerList);
	}

	fsm_.process_event(Flop::Event(PRL_ERR_FAILURE));
}

template <typename Event, typename FSM>
void Frontend::on_exit(const Event& event_, FSM& fsm_)
{
	vsd::Frontend<Frontend>::on_exit(event_, fsm_);
	foreach (const QString& alias_, m_listenerMap.keys())
	{
		closeListener(qPrintable(alias_));
	}
	foreach (const client_type& c, m_clients)
	{
		c->close();
	}
	getConnector()->setService(NULL);
}

void Frontend::closeListener(const char* alias_)
{
	QString k(alias_);
	if (!m_listenerMap.contains(k))
		return;

	listenerMap_type::mapped_type s = m_listenerMap.value(k);
	if (!s.isNull() && s->isListening())
	{
		s->disconnect(SIGNAL(newConnection()),
				getConnector());
		s->close();
	}
}

void Frontend::closeDiskListener(const qemuDisk_type::Down& event_)
{
	Q_UNUSED(event_);
	closeListener(SLOT(acceptQemuDisk()));
}

void Frontend::closeStateListener(const qemuState_type::Down& event_)
{
	Q_UNUSED(event_);
	closeListener(SLOT(acceptQemuState()));
}

} // namespace Tunnel

///////////////////////////////////////////////////////////////////////////////
// struct Connector

void Connector::cancel()
{
	handle(Flop::Event(PRL_ERR_OPERATION_WAS_CANCELED));
}

void Connector::react(const SmartPtr<IOPackage>& package_)
{
	// coping handle io by itself, so just ignore
	if (m_top->is_flag_active<Content::Flag>())
		return;

	switch (package_->header.type)
	{
	case DispToDispResponseCmd:
	{
		CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(package_);
		CDispToDispResponseCommand *cmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		if (PRL_FAILED(cmd->GetRetCode()))
			handle(Flop::Event(cmd->GetErrorInfo()));
		else
			handle(PeerFinished(SmartPtr<IOPackage>()));
		break;
	}
	case VmMigrateCheckPreconditionsReply:
		return handle(CheckReply(package_));
	case VmMigrateReply:
		return handle(StartReply(package_));
	case VmMigrateLibvirtTunnelChunk:
		return handle(Vm::Tunnel::libvirtChunk_type(package_));
	case VmMigrateQemuDiskTunnelChunk:
		return handle(Vm::Pump::Event<VmMigrateQemuDiskTunnelChunk>(package_));
	case VmMigrateQemuStateTunnelChunk:
		return handle(Vm::Pump::Event<VmMigrateQemuStateTunnelChunk>(package_));
	case VmMigrateFinishCmd:
		return handle(Vm::Pump::FinishCommand_type(package_));
	default:
		WRITE_TRACE(DBG_FATAL, "Unexpected package type: %d.", package_->header.type);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template <typename Event, typename FSM>
void Frontend::on_entry(const Event& event_, FSM& fsm_)
{
	bool x;
	vsd::Frontend<Frontend>::on_entry(event_, fsm_);
	x = getConnector()->connect(m_task, SIGNAL(cancel()), SLOT(cancel()));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
	x = getConnector()->connect(m_io,
			SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
			SLOT(react(const SmartPtr<IOPackage>&)));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
	x = getConnector()->connect(m_io, SIGNAL(disconnected()),
			SLOT(cancel()));
	if (!x)
		WRITE_TRACE(DBG_FATAL, "can't connect");
}

template <typename Event, typename FSM>
void Frontend::on_exit(const Event& event_, FSM& fsm_)
{
	vsd::Frontend<Frontend>::on_exit(event_, fsm_);
	m_task->disconnect(SIGNAL(cancel()), getConnector(), SLOT(cancel()));
	m_io->disconnect(SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
			getConnector(), SLOT(react(const SmartPtr<IOPackage>&)));
	m_io->disconnect(SIGNAL(disconnected()), getConnector(), SLOT(cancel()));
}

bool Frontend::isTemplate(const boost::mpl::true_&)
{
	return m_task->getTargetConfig()->getVmSettings()
		->getVmCommonOptions()->isTemplate()
		|| PVMT_SWITCH_TEMPLATE & m_task->getRequestFlags();
}

void Frontend::setResult(const peerQuitState_type::Good&)
{
	if (PVMT_CLONE_MODE & m_task->getRequestFlags())
		return;

	::Libvirt::Kit.vms().at(m_task->getVmUuid()).getState().kill();
}

void Frontend::setResult(const Flop::Event& value_)
{
	if (value_.isFailed())
		boost::apply_visitor(Flop::Visitor(*m_task), value_.error());
}

template<class T>
void Frontend::pokePeer(const T&)
{
	m_task->confirmFinish();
}

void Frontend::audit(const Libvirt::Trick::Online::downtime_type& downtime_)
{
	boost::property_tree::ptree p;
	p.put("target_address", m_task->getTargetAddress().toStdString());
	p.put("target_private", m_task->getTargetPrivate().toStdString());
	p.put("downtime", downtime_);
	p.put("clone", !!(m_task->getFlags() & PVMT_CLONE_MODE));
	p.put("remove_src", !!(m_task->getFlags() & PVMT_REMOVE_SOURCE_BUNDLE));
	p.put("change_sid", !!(m_task->getFlags() & PVMT_CHANGE_SID));
	p.put("compression", !(m_task->getFlags() & PVMT_UNCOMPRESSED));
	p.put("encryption", !(m_task->getFlags() & PVMT_DIRECT_DATA_CONNECTION));
	Task::Trace t(m_task->getRequestPackage());
	t.report(p);
}

} // namespace Source
} // namespace Vm
} // namespace Migrate

/*
end of shared code from Vm/CVmMigrateTask.cpp
*/

static void NotifyClientsWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sVmDirectoryUuid,
		const QString &sVmUuid,
		int nPercents)
{
	CVmEvent event(PET_DSP_EVT_VM_MIGRATE_PROGRESS_CHANGED, sVmUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirectoryUuid, sVmUuid);
}


Task_MigrateVmSource::Task_MigrateVmSource(
		Registry::Public& registry_,
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:CDspTaskHelper(client, p),
Task_DispToDispConnHelper(getLastError()),
m_nRemoteVersion(MIGRATE_DISP_PROTO_V1),
m_pVmConfig(new CVmConfiguration()),
m_bNewVmInstance(false),
m_nPrevVmState(VMS_UNKNOWN),
m_nSteps(0),
m_nRegisterCmd(PVE::DspIllegalCommand),
m_registry(registry_)
{
	CProtoVmMigrateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_sVmUuid = pCmd->GetVmUuid();
	m_sVmDirUuid = getClient()->getVmDirectoryUuid();
	m_sServerHostname = pCmd->GetTargetServerHostname();
	m_nServerPort = pCmd->GetTargetServerPort();
	if (m_nServerPort == 0)
		m_nServerPort = CDspService::getDefaultListenPort();
	m_sServerSessionUuid = pCmd->GetTargetServerSessionUuid();
	m_sTargetServerVmName = pCmd->GetTargetServerVmName();
	m_sTargetServerVmHomePath = pCmd->GetTargetServerVmHomePath();
	m_nMigrationFlags = pCmd->GetMigrationFlags();
	/* clear migration type bits */
	m_nMigrationFlags &= ~PVMT_HOT_MIGRATION & ~PVMT_WARM_MIGRATION & ~PVMT_COLD_MIGRATION;
	m_nReservedFlags = pCmd->GetReservedFlags();
	moveToThread(this);
}

Task_MigrateVmSource::~Task_MigrateVmSource()
{
}

PRL_RESULT Task_MigrateVmSource::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - VM migrate rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	if (CDspService::instance()->getShellServiceHelper().isLocalAddress(m_sServerHostname)) {
		WRITE_TRACE(DBG_FATAL,
			"Host %s is a local host, migration is impossible", QSTR2UTF8(m_sServerHostname));
		nRetCode = PRL_ERR_VM_MIGRATE_TO_THE_SAME_NODE;
		goto exit;
	}

	/* check access for migration (for PVE::DspCmdDirVmMigrate, as for more strict) */
	nRetCode = CDspService::instance()->getAccessManager().checkAccess(
			getClient(), PVE::DspCmdDirVmMigrate, m_sVmUuid, NULL, getLastError());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while CDspAccessManager::checkAccess() with code [%#x][%s]",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	{
		/* before CDspVm get instance, to select m_nRegisterCmd */
		/* LOCK inside brackets */
		CVmIdent ident(CDspVmDirHelper::getVmIdentByVmUuid(m_sVmUuid, getClient()));
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem(CDspService::instance()->getVmDirManager()
				.getVmDirItemByUuid(ident));
		if (!pVmDirItem) {
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			WRITE_TRACE(DBG_FATAL, "Couldn't to find Vm with UUID '%s'", QSTR2UTF8(m_sVmUuid));
			goto exit;
		}
		m_sVmDirUuid = ident.second;
		m_sVmName = pVmDirItem->getVmName();
		m_sVmConfigPath = pVmDirItem->getVmHome();
		m_sVmHomePath = CFileHelper::GetFileRoot(m_sVmConfigPath);

		/* will load config with relative path */
		nRetCode = CDspService::instance()->getVmConfigManager().loadConfig(
					m_pVmConfig, pVmDirItem.getPtr()->getVmHome(), getClient(), false, false);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "Vm %s config loading failed. Reason: %#x (%s)",
					QSTR2UTF8(m_sVmName), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
			goto exit;
		}
	}
	/*
	 * Lets allow more then one simultaneous tasks for the same VM template migration in clone mode,
	 * as PACI want (https://jira.sw.ru/browse/PSBM-13328)
	 * Will use other command than PVE::DspCmdDirVmMigrate for registerExclusiveVmOperation() in this case.
	 */
	if ((PVMT_CLONE_MODE & getRequestFlags()))
		m_nRegisterCmd = PVE::DspCmdDirVmMigrateClone;
	else
		m_nRegisterCmd = PVE::DspCmdDirVmMigrate;

	nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
		m_sVmUuid, m_sVmDirUuid, m_nRegisterCmd, getClient());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}
	m_nSteps |= MIGRATE_VM_EXCL_PARAMS_LOCKED;

	m_nPrevVmState = CDspVm::getVmState(m_sVmUuid, m_sVmDirUuid);
	// template already in migrating stage, but original template state is stopped
	if (VMS_MIGRATING == m_nPrevVmState)
		m_nPrevVmState = VMS_STOPPED;
	{
		namespace in = Migrate::Vm::Source::Inspection;
		in::Report x;
		in::Generic<in::Clone::Linked> y = in::Generic<in::Clone::Linked>
			(in::Generic<in::Clone::Online>	(in::Generic<in::ChangeSid>
				(in::Hardware::Chain())));
		y(qMakePair(this, &x));
		if (x.getResult().isEmpty())
		{
			m_cHostInfo.fromString(CDspService::instance()->getHostInfo()->data()->toString());
			nRetCode = Connect(
				m_sServerHostname, m_nServerPort, m_sServerSessionUuid, QString(), QString(),
				m_nMigrationFlags & PACF_MASK);
		}
		else
		{
			m_lstCheckPrecondsErrors = x.getResult();
			nRetCode = PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;
		}
	}
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MigrateVmSource::prepareStart()
{
	SmartPtr<CVmEvent> pEvent;
	SmartPtr<IOPackage> pPackage;

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	// after CheckVmMigrationPreconditions() only
	if ((m_nReservedFlags & PVM_DONT_COPY_VM) && (PVMT_CLONE_MODE & getRequestFlags())) {
		// it's valid for templates only
		if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate()) {
			WRITE_TRACE(DBG_FATAL, "Can't clone shared VM '%s'", QSTR2UTF8(m_sVmUuid));
			nRetCode = PRL_ERR_VM_MIGRATE_CANNOT_REMOTE_CLONE_SHARED_VM;
			goto exit;
		}
	}

	/* set migration mode */
	switch (m_nPrevVmState) {
	case VMS_RUNNING:
	case VMS_PAUSED:
		if (m_nRemoteVersion < MIGRATE_DISP_PROTO_V3) {
			WRITE_TRACE(DBG_FATAL, "[%s] Old Virtuozzo Server on target (protocol version %d)."
				" Warm migration is not supported due to suspend/resume incompatibility,",
				__FUNCTION__, m_nRemoteVersion);
			nRetCode = PRL_ERR_VM_MIGRATE_WARM_MODE_NOT_SUPPORTED;
			goto exit;
		}
		m_nMigrationFlags |= PVMT_HOT_MIGRATION;
		break;
	default:
		m_nMigrationFlags |= PVMT_COLD_MIGRATION;
	}

	pEvent = SmartPtr<CVmEvent>(new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sVmUuid, PIE_DISPATCHER));
	pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "true", EVT_PARAM_MIGRATE_IS_SOURCE));
	pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());
	/* change Vm state to VMS_MIGRATING */
	CDspService::instance()->getVmStateSender()->
		onVmStateChanged(m_nPrevVmState, VMS_MIGRATING, m_sVmUuid, m_sVmDirUuid, false);
	m_nSteps |= MIGRATE_VM_STATE_CHANGED;
	/* and notify clients about VM migration start event */
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sVmUuid);
	nRetCode = migrateStoppedVm();

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MigrateVmSource::releaseLocks()
{
	if (m_nSteps & MIGRATE_VM_EXCL_PARAMS_LOCKED) {
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
				m_sVmUuid, m_sVmDirUuid, m_nRegisterCmd, getClient());
		m_nSteps &= ~MIGRATE_VM_EXCL_PARAMS_LOCKED;
	}

	/* Do not touch Vm config */
	if (m_pVm)
		m_pVm->disableStoreRunningState(true);

	// call destructor to unregister exclusive operaion (https://jira.sw.ru/browse/PSBM-13445)
	m_pVm = SmartPtr<CDspVm>(0);
}

void Task_MigrateVmSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
	SmartPtr<IOPackage> pPackage;
	CProtoCommandPtr pResponse =
		CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), getLastErrorCode());

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		QList< SmartPtr< CDspClient > > lstClient = CDspService::instance()->getClientManager().
				getSessionListByVm(m_sVmDirUuid, m_sVmUuid).values();

		if (CFileHelper::isSharedFS(m_sVmHomePath) && !(m_nReservedFlags & PVM_DONT_COPY_VM))
		{
			// handle only VM on shared FS - nfs, gfs, gfs2, pcs
			// for shared migration case target task already moved resource
			CDspService::instance()->getHaClusterHelper()->removeClusterResource(m_sVmName);
		}

		if (m_cSavFileCopy.exists())
			QFile::remove(m_cSavFileCopy.absoluteFilePath());
		if ((m_nSteps & MIGRATE_CONFIGSAV_BACKUPED) && m_sConfigSavBackup.size())
			QFile::remove(m_sConfigSavBackup);
		if (m_cLocalMemFile.exists()) {
			/* to remove origin mem file on success */
			QDir cMemFileDir = m_cLocalMemFile.absoluteDir();
			if (m_sVmUuid == cMemFileDir.dirName())
				CFileHelper::ClearAndDeleteDir(cMemFileDir.absolutePath());
			else
				QFile::remove(m_cLocalMemFile.absoluteFilePath());
		}

		/* prepare event for client before Vm removing */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_FINISHED, m_sVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sVmUuid);

		/* Lock VM to prevent race after delete CDspVm and before remove VM. */
		if ( !(PVMT_CLONE_MODE & getRequestFlags()) )
			CDspService::instance()->getVmDirHelper().
						lockVm(m_sVmUuid, getClient(), getRequestPackage());

		/* cleanup CDspVm object before remove VM from disk #PSBM-9753
		 * The CDspVm::cleanupObject use Vm config that will be removed.
		 */
		releaseLocks();

		if (PVMT_CLONE_MODE & getRequestFlags())
		{
			CDspService::instance()->getVmStateSender()->
				onVmStateChanged(VMS_MIGRATING, m_nPrevVmState, m_sVmUuid, m_sVmDirUuid, false);
		}
		else
		{
			CProtoCommandPtr pRequest = CProtoSerializer::CreateVmDeleteProtoCommand(m_sVmUuid, QStringList());
			SmartPtr<IOPackage> pDelPackage
				= DispatcherPackage::createInstance(PVE::DspCmdDirVmDelete, pRequest);

			CDspTaskFuture<Task_DeleteVm> pTask;
			SmartPtr<CDspClient> fakeClient = CDspClient::makeServiceUser( m_sVmDirUuid );
			if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
			{
				WRITE_TRACE(DBG_INFO, "Delete VM '%s'", QSTR2UTF8(m_sVmName));
				pDelPackage->header.type = PVE::DspCmdDirVmDelete;

				pTask = CDspService::instance()->getVmDirHelper().
					unregOrDeleteVm( fakeClient, pDelPackage, m_pVmConfig->toString(),
					PVD_SKIP_VM_OPERATION_LOCK | PVD_SKIP_HA_CLUSTER);
			}
			else
			{
				WRITE_TRACE(DBG_INFO, "Unreg VM '%s'", QSTR2UTF8(m_sVmName));
				pDelPackage->header.type = PVE::DspCmdDirUnregVm;

				pTask = CDspService::instance()->getVmDirHelper().
					unregOrDeleteVm( fakeClient, pDelPackage, m_pVmConfig->toString(),
					PVD_UNREGISTER_ONLY | PVD_SKIP_VM_OPERATION_LOCK |
					PVD_NOT_MODIFY_VM_CONFIG | PVD_SKIP_HA_CLUSTER);
			}
			CVmEvent evt;
			pTask.wait().getResult(&evt);
			if (PRL_FAILED(evt.getEventCode()))
			{
				PRL_RESULT wcode = PRL_ERR_VM_MIGRATE_ERROR_UNREGISTER_VM;
				if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
					wcode = PRL_ERR_VM_MIGRATE_ERROR_DELETE_VM;

				CVmEvent evt( PET_DSP_EVT_VM_MESSAGE, m_sVmName,
								PIE_DISPATCHER, wcode );

				SmartPtr<IOPackage> pWarn = DispatcherPackage::createInstance(
						PVE::DspVmEvent, evt );

				getClient()->sendPackage( pWarn );
			}

			// delete non-shared external disks
			PRL_RESULT wcode = PRL_ERR_SUCCESS;
			foreach (const QString &disk, m_lstNonSharedDisks) {
				WRITE_TRACE(DBG_INFO, "Deleting external disk '%s'", QSTR2UTF8(disk));
				if (!CFileHelper::RemoveEntry(disk, &getClient()->getAuthHelper()))
					wcode = PRL_ERR_NOT_ALL_FILES_WAS_DELETED;
			}
			if (wcode != PRL_ERR_SUCCESS) {
				CVmEvent evt( PET_DSP_EVT_VM_MESSAGE, m_sVmName,
								PIE_DISPATCHER, wcode );

				SmartPtr<IOPackage> pWarn = DispatcherPackage::createInstance(
						PVE::DspVmEvent, evt );

				getClient()->sendPackage( pWarn );
			}

			CDspService::instance()->getVmDirHelper().
						unlockVm(m_sVmUuid, getClient(), getRequestPackage());
		}
	}
	else
	{
		PRL_EVENT_TYPE evtType;
		if (m_cSavFileCopy.exists())
			QFile::remove(m_cSavFileCopy.absoluteFilePath());
		if (m_nSteps & MIGRATE_CONFIGSAV_BACKUPED) {
			/* restore backuped config.sav on failure */
			if (m_cSavFile.exists())
				QFile::remove(m_cSavFile.absoluteFilePath());
			if (m_sConfigSavBackup.size())
				QFile::rename(m_sConfigSavBackup, m_cSavFile.absoluteFilePath());
		}
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED, m_sVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sVmUuid);
		if (m_nSteps & MIGRATE_VM_STATE_CHANGED)
		{
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
			CDspService::instance()->getVmStateSender()->onVmStateChanged
				(VMS_MIGRATING, m_nPrevVmState, m_sVmUuid, m_sVmDirUuid, false);
			CDspService::instance()->getClientManager().
				sendPackageToVmClients(pUpdateVmStatePkg, m_sVmDirUuid, m_sVmUuid);
		}

		releaseLocks();

		// fill response
		CProtoCommandDspWsResponse *wsResponse =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );
		if (getLastErrorCode() == PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED)
			wsResponse->SetParamsList( m_lstCheckPrecondsErrors );
		getLastError()->setEventCode(getLastErrorCode());
		wsResponse->SetError(getLastError()->toString());
	}

	getClient()->sendResponse(pResponse, getRequestPackage());
}

// cancel command
void Task_MigrateVmSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& pkg)
{
	SmartPtr<IOPackage> p;
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);

	if (!pkg.isValid()) {
		// CDspTaskManager destructor call cancelOperation() for task with dummy package
		CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoBasicVmCommand(
					PVE::DspCmdVmMigrateCancel, m_sVmUuid);
		p = DispatcherPackage::createInstance(PVE::DspCmdVmMigrateCancel, pCmd);
	} else {
		p = pkg;
	}
	emit cancel();
	CancelOperationSupport::cancelOperation(pUser, p);
}

PRL_RESULT Task_MigrateVmSource::migrateStoppedVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) ) {
		/* get full directories and files lists for migration */
		if (PRL_FAILED(nRetCode = CVmMigrateHelper::GetEntryListsVmHome(m_sVmHomePath, m_dList, m_fList)))
			return nRetCode;

		if (PVMT_CLONE_MODE & getRequestFlags())
		{
			Vm::Private::Brand b(m_sVmHomePath, getClient());
			foreach (Vm::Private::Brand::entryList_type::const_reference e, b.getFiles())
				m_fList.removeOne(e);
		}
	}

	if ((m_nPrevVmState != VMS_RUNNING) && (m_nPrevVmState != VMS_PAUSED))
	{
		// first find set of parent dirs of non shared disks
		QSet<QString> dirs;
		foreach (const QString disk, m_lstNonSharedDisks)
			dirs.insert(QFileInfo(disk).dir().absolutePath());

		foreach (const QString &dir, dirs)
			m_dList << qMakePair(QFileInfo(dir), dir);

		foreach (const QString &disk, m_lstNonSharedDisks)
			m_fList << qMakePair(QFileInfo(disk), disk);
	}
	else
	{
		// For online migration we don't need to copy hard disk images
		foreach (const CVmHardDisk* d, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
		{
			m_fList.removeOne(qMakePair(QFileInfo(m_sVmHomePath, d->getSystemName()), d->getSystemName()));
		}
	}

	CDispToDispCommandPtr a = CDispToDispProtoSerializer::CreateVmMigrateStartCommand(
				m_pVmConfig->toString(),
				QString(), // we can't get runtime config here - do it on VM level
				m_sTargetServerVmHomePath,
				m_sSnapshotUuid,
				QFileInfo(m_sVmHomePath).permissions(),
				QFileInfo(m_sVmConfigPath).permissions(),
				m_nMigrationFlags,
				m_nReservedFlags,
				m_nPrevVmState);

	SmartPtr<IOPackage> b = DispatcherPackage::createInstance
			(a->GetCommandId(), a->GetCommand()->toString());

	m_pVmConfig->setAbsolutePath();
	return SendPkg(b);
}

PRL_RESULT Task_MigrateVmSource::reactStartReply(const SmartPtr<IOPackage>& package)
{
	if (package->header.type != VmMigrateReply)
	{
		WRITE_TRACE(DBG_FATAL, "Unexpected package type: %d.", package->header.type);
		return PRL_ERR_OPERATION_FAILED;
	}

	CDispToDispCommandPtr a = CDispToDispProtoSerializer::ParseCommand(package);
	CVmMigrateReply *b =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateReply>(a);

	m_sTargetMemFilePath = b->GetMemFilePath();

	if (m_nPrevVmState == VMS_SUSPENDED) {
		QString sMemFileName, sMemFilePath;
		QDir dir(m_sVmHomePath);
		QFileInfo cConfig(dir, VMDIR_DEFAULT_VM_CONFIG_FILE);
		CStatesHelper cStatesHelper(cConfig.absoluteFilePath());
		m_cSavFile.setFile(dir, cStatesHelper.getSavFileName());
	}

	return PRL_ERR_SUCCESS;
}

Migrate::Vm::Source::Content::Copier* Task_MigrateVmSource::createCopier()
{
	/* calculate total transaction size */
	quint64 totalSize = std::accumulate(m_fList.begin(), m_fList.end(), quint64(),
				boost::bind(std::plus<quint64>(), _1,
					boost::bind(&QFileInfo::size,
						boost::bind(&QPair<QFileInfo, QString>::first, _2))));

	// don't share non threadsave data with this objects as
	// they will be used in a different thread
	SmartPtr<CVmEvent> error(new CVmEvent());
	SmartPtr<CVmFileListCopySender> sender(new CVmFileListCopySenderClient(m_pIoClient, getDegree()));
	SmartPtr<CVmFileListCopySource> copier(new CVmFileListCopySource(
				sender.getImpl(),
				QString(m_pVmConfig->getVmIdentification()->getVmUuid()),
				QString(m_sVmHomePath),
				totalSize,
				error.getImpl(),
				m_nTimeout));
	copier->SetRequest(getRequestPackage());
	copier->SetVmDirectoryUuid(m_sVmDirUuid);
	copier->SetProgressNotifySender(NotifyClientsWithProgress);

	return new Migrate::Vm::Source::Content::Copier(sender, copier, error);
}

/* check that image file of devive is place into Vm bundle */
void Task_MigrateVmSource::DisconnectExternalImageDevice(CVmDevice* pDevice)
{
	QFileInfo fInfo(pDevice->getSystemName());

	/* skip disabled and disconnected devices */
	if (PVE::DeviceDisabled == pDevice->getEnabled())
		return;
	if (PVE::DeviceDisconnected == pDevice->getConnected())
		return;

	if (fInfo.isRelative())
		return;

	if (fInfo.absoluteFilePath().startsWith(m_sVmHomePath))
		return;

	WRITE_TRACE(DBG_FATAL, "[%s] Image file %s of device %s is place out of Vm bundle %s",
			__FUNCTION__,
			QSTR2UTF8(pDevice->getUserFriendlyName()),
			QSTR2UTF8(pDevice->getSystemName()),
			QSTR2UTF8(m_sVmHomePath));
	pDevice->setConnected(PVE::DeviceDisconnected);
}

/* return PRL_ERR_SUCCESS in case of success checking, result no matter
*	  error - if can't check
*/
PRL_RESULT Task_MigrateVmSource::CheckVmDevices()
{
	CVmHardware* pVmHardware = m_pVmConfig->getVmHardwareList();

	if (pVmHardware == NULL) {
		WRITE_TRACE(DBG_FATAL, "[%s] Can not get Vm hardware list", __FUNCTION__);
		return PRL_ERR_OPERATION_FAILED;
	}

	foreach (CVmOpticalDisk* dvd, pVmHardware->m_lstOpticalDisks)
	{
		if ((_PRL_VM_DEV_EMULATION_TYPE)dvd->getEmulatedType() != PDT_USE_REAL_DEVICE)
			DisconnectExternalImageDevice(dvd);
	}

	/* process floppy disk images */
	foreach (CVmFloppyDisk* pDevice, pVmHardware->m_lstFloppyDisks) {
		if (pDevice->getEmulatedType() != PVE::FloppyDiskImage)
			continue;
		DisconnectExternalImageDevice(pDevice);
	}

	/* process serial ports output files */
	foreach (CVmSerialPort* pDevice, pVmHardware->m_lstSerialPorts) {
		if (pDevice->getEmulatedType() != PVE::SerialOutputFile)
			continue;
		DisconnectExternalImageDevice(pDevice);
	}

	/* process parallel ports output files */
	foreach (CVmParallelPort* pDevice, pVmHardware->m_lstParallelPorts) {
		if (pDevice->getEmulatedType() != PVE::ParallelOutputFile)
			continue;
		DisconnectExternalImageDevice(pDevice);
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_MigrateVmSource::run_body()
{
	namespace mvs = Migrate::Vm::Source;
	typedef mvs::Machine_type backend_type;

	const quint32 timeout = (quint32)
		CDspService::instance()->getDispConfigGuard().getDispToDispPrefs()->getSendReceiveTimeout() * 1000;
	quint32 finishing_timeout = timeout;
	if (PVMT_CHANGE_SID & getRequestFlags())
		finishing_timeout = CHANGESID_TIMEOUT;

	CAuthHelperImpersonateWrapper _impersonate(&getClient()->getAuthHelper());

	mvs::Tunnel::IO io(*m_pIoClient);
	backend_type::moveState_type m(boost::msm::back::states_
		<< boost::mpl::at_c<backend_type::moveState_type::initial_state, 0>::type
			(boost::ref(io))
		<< boost::mpl::at_c<backend_type::moveState_type::initial_state, 1>::type
			(boost::ref(*this), boost::bind(&NotifyClientsWithProgress,
				getRequestPackage(), boost::cref(m_sVmDirUuid),
				boost::cref(m_sVmUuid), _1))
		<< boost::mpl::at_c<backend_type::moveState_type::initial_state, 2>::type(~0));
	backend_type::copyState_type c(boost::bind(boost::factory<mvs::Content::Task* >(),
		boost::ref(*this), boost::cref(m_dList), boost::cref(m_fList)));

	backend_type machine(boost::msm::back::states_
		<< backend_type::checkState_type(boost::bind
			(&Task_MigrateVmSource::reactCheckReply, this, _1), timeout)
		<< backend_type::startState_type(boost::bind
			(&Task_MigrateVmSource::reactStartReply, this, _1), timeout)
		<< backend_type::peerQuitState_type(finishing_timeout)
		<< c << m << Migrate::Vm::Finished(*this),
		boost::ref(*this), boost::ref(io));
	(Migrate::Vm::Walker<backend_type>(machine))();

	machine.start();
	PRL_RESULT r = CheckVmMigrationPreconditions();
	if (PRL_SUCCEEDED(r))
		exec();
	else
		CDspTaskFailure(*this)(r);

	machine.stop();
	return getLastErrorCode();
}

/*
* Check preconditions
* return PRL_ERR_SUCCESS - no errors
* 	 PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED - precondition error
*	 other - internal error
*/
PRL_RESULT Task_MigrateVmSource::CheckVmMigrationPreconditions()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (PRL_FAILED(nRetCode = CheckVmDevices()))
		return nRetCode;

	if (m_lstCheckPrecondsErrors.size())
		return PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;


	if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
	{
		int snapNum = 0;

		if (PRL_FAILED(nRetCode = CDspVmSnapshotStoreHelper().countSnapshotsForVM(m_sVmUuid, snapNum)))
			return nRetCode;

		if (snapNum > 0)
		{
			WRITE_TRACE(DBG_FATAL, "Unable to migrate the virtual environment, because it has snapshots.");
			return PRL_ERR_VM_MIGRATE_SNAPSHOT_DETECTED;
		}
	}

	// Get size of VM directory
	PRL_UINT64 nRequiresDiskSpace;
	nRetCode = CSimpleFileHelper::GetDirSize(m_sVmHomePath, &nRequiresDiskSpace);
	if ( PRL_FAILED(nRetCode) ) {
		nRequiresDiskSpace = 0ULL;
		WRITE_TRACE(DBG_FATAL, "CSimpleFileHelper::GetDirSize(%s) return %#x[%s], disk space check will skipped",
				QSTR2UTF8(m_sVmHomePath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	}


	// Create temporary file in home to check if its shared between source
	// and destination
	QStringList extCheckFiles;
	QString tmpFile, homeCheckFile;
	if (PRL_FAILED(nRetCode = CVmMigrateHelper::createSharedFile(m_sVmHomePath, tmpFile)))
		return nRetCode;
	// relative path for home checkfile
	homeCheckFile = QFileInfo(tmpFile).fileName();
	m_lstAllCheckFiles.append(tmpFile);

	if (PRL_FAILED(nRetCode = CVmMigrateHelper::buildExternalTmpFiles(
					m_pVmConfig->getVmHardwareList()->m_lstHardDisks,
					m_sVmHomePath,
					extCheckFiles)))
		return nRetCode;
	m_lstAllCheckFiles << extCheckFiles;

	QString sHaClusterId;
	// try to get HA cluster ID for shared migration
	nRetCode = CDspService::instance()->getHaClusterHelper()->getHaClusterID(sHaClusterId);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	//2. Send check preconditions request to the target dispatcher
	CDispToDispCommandPtr pRequest =
		CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsCommand(
			m_pVmConfig->toString(),
			m_cHostInfo.toString(),
			m_sTargetServerVmName,
			m_sTargetServerVmHomePath,
			homeCheckFile,
			extCheckFiles,
			sHaClusterId,
			nRequiresDiskSpace,
			m_nMigrationFlags,
			m_nReservedFlags,
			m_nPrevVmState
		);

	SmartPtr<IOPackage> pPackage =
		DispatcherPackage::createInstance(pRequest->GetCommandId(), pRequest->GetCommand()->toString());
	return SendPkg(pPackage);
}

PRL_RESULT Task_MigrateVmSource::reactCheckReply(const SmartPtr<IOPackage>& package)
{
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(
			DispToDispResponseCmd,
			UTF8_2QSTR(package->buffers[0].getImpl())
		);

	CVmMigrateCheckPreconditionsReply *pResponseCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsReply>(pCmd);
	QStringList lstErrors = pResponseCmd->GetCheckPreconditionsResult();
	m_nRemoteVersion = pResponseCmd->GetVersion();
	m_nReservedFlags = pResponseCmd->GetCommandFlags();
	m_targetConfig.reset(new CVmConfiguration(pResponseCmd->GetConfig()));
	if (PRL_FAILED(m_targetConfig->m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Error on parsing updated config:\n%s",
				QSTR2UTF8(pResponseCmd->GetConfig()));
		return PRL_ERR_PARSE_VM_CONFIG;
	}
	if (m_nRemoteVersion >= MIGRATE_DISP_PROTO_V4)
		m_lstNonSharedDisks = pResponseCmd->GetNonSharedDisks();

	namespace in = Migrate::Vm::Source::Inspection;
	in::Report x;
	in::Reply::Efiw2k8(m_nRemoteVersion,
		in::Reply::Interpreter(lstErrors))(qMakePair(this, &x));

	// remove temporary file before migration
	foreach (const QString &file, m_lstAllCheckFiles)
		QFile::remove(file);

	if (!x.getResult().isEmpty())
	{
		m_lstCheckPrecondsErrors = x.getResult();
		return PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;
	}

	//prohibit migration of VM with enabled EFI to old dispatcher version
	//because it does not have proper OVMF 4M file
	if (m_nRemoteVersion < MIGRATE_DISP_PROTO_V11 &&
			m_pVmConfig->getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled())
	{
		WRITE_TRACE(DBG_FATAL, "Old Virtuozzo Server on target (protocol version %d)."
			" Migration is not supported with enabled booting with EFI firmware",
			m_nRemoteVersion);
		return PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;
	}

	return prepareStart();
}

PRL_RESULT Task_MigrateVmSource::confirmFinish()
{
	SmartPtr<IOPackage> p = IOPackage::createInstance
		(Migrate::Vm::Pump::FinishCommand_type::s_command, 1);
	if (!p.isValid())
		return PRL_ERR_FAILURE;

	return SendPkg(p);
}

QList<CVmHardDisk* > Task_MigrateVmSource::getVmUnsharedDisks() const
{
	QList<CVmHardDisk* > output;
	foreach (CVmHardDisk* d, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
	{
		if (m_lstNonSharedDisks.contains(d->getSystemName()))
			output << d;
	}
	if (0 == (m_nReservedFlags & PVM_DONT_COPY_VM))
	{
		foreach (CVmHardDisk* d, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
		{
			if (!d->getEnabled() ||
				d->getEmulatedType() != PVE::HardDiskImage)
				continue;

			if (!QFileInfo(d->getSystemName()).isAbsolute() ||
					d->getSystemName().startsWith(m_sVmHomePath))
				output << d;
		}
	}
	return output;
}

void Task_MigrateVmSource::setSharedStorage()
{
	m_nReservedFlags |= PVM_DONT_COPY_VM;
	if (m_pVmConfig.isValid() &&
		m_pVmConfig->getVmSettings()->getHighAvailability()->isEnabled())
	{
		QString ha;
		CDspService::instance()->getHaClusterHelper()->getHaClusterID(ha);
		m_nReservedFlags |= ((!ha.isEmpty()) * PVM_HA_MOVE_VM);
	}
}

