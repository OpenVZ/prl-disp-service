///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackupHelper.cpp
///
/// Common functions for backup tasks
///
/// @author krasnov@
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

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <QProcess>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Task_CloneVm_p.h"
#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"

#include "Task_BackupHelper.h"
#include "Task_BackupHelper_p.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "prlcommon/HostUtils/HostUtils.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "CDspVmStateSender.h"
#include "CDspVmManager_p.h"

#include "CDspVzHelper.h"
#include <sys/resource.h>
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "prlcommon/IOService/IOCommunication/Socket/Socket_p.h"
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <time.h>
#include <vzctl/libvzctl.h>


// milliseconds sleep for read from subprocess
#define SLEEP_INTERVAL	1000

namespace Backup
{
namespace Work
{
///////////////////////////////////////////////////////////////////////////////
// struct Ct

Ct::Ct(Task_BackupHelper& task_) : m_context(&task_)
{
}

QStringList Ct::buildArgs(const Product::component_type& t_, const QFileInfo* f_) const
{
	QStringList a;
	a << QString((m_context->getFlags() & PBT_INCREMENTAL) ? "append_ct" : "create_ct");

	// <private area> <backup path> <tib> <ct_id> <ve_root> <is_running>
	a << f_->absoluteFilePath()
		<< m_context->getProduct()->getStore().absolutePath()
		<< t_.second.absoluteFilePath()
		<< m_context->getProduct()->getObject().getConfig()->getVmIdentification()->getCtId()
		<< m_context->getProduct()->getObject().getConfig()->getCtSettings()->getMountPath()
		<< (m_context->isRunning() ? "1" : "0");

	a << "--last-tib" << QString::number(m_context->getBackupNumber());
	return a;
}

QStringList Ct::buildPushArgs(const Activity::Object::Model& activity_) const
{
	QStringList a;
	a << QString((m_context->getFlags() & PBT_INCREMENTAL) ? "append_ct" : "create_ct");

	foreach (const Product::component_type& t, m_context->getProduct()->getVmTibs())
	{
		const QFileInfo* f = Command::findArchive(t, activity_);
		QString u;
		foreach (const Activity::Object::component_type& c, m_context->getUrls())
		{
			if (t.second.absoluteFilePath() == c.first.absoluteFilePath())
			{
				u = c.second;
				break;
			}
		}
		a << "--image" << QString("ploop://%1::%2").arg(f->absoluteFilePath())
						.arg(u);
	}
	return a;
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(Task_BackupHelper& task_) : m_context(&task_)
{
}

QStringList Vm::buildArgs(const QString& snapshot_,
	const Product::component_type& t_, const QFileInfo* f_) const
{
	QStringList a;
	a << QString((m_context->getFlags() & PBT_INCREMENTAL) ? "append" : "create");

	a << t_.first.getFolder() << m_context->getProduct()->getStore().absolutePath()
		<< t_.second.absoluteFilePath() << snapshot_ << f_->absoluteFilePath();

	return a;
}

QStringList Vm::buildPushArgs() const
{
	QStringList a;
	a << QString((m_context->getFlags() & PBT_INCREMENTAL) ? "append" : "create");

	QString n = m_context->getProduct()->getObject()
			.getConfig()->getVmIdentification()->getVmName();
	a << "-n" << n;

	foreach (const Product::component_type& t, m_context->getProduct()->getVmTibs())
	{
		a << "--image" << QString("%1::%2").arg(t.first.getImage())
						.arg(t.second.absoluteFilePath());
	}
	return a;
}

///////////////////////////////////////////////////////////////////////////////
// struct Command

const QFileInfo * Command::findArchive(const Product::component_type& t_,
	const Activity::Object::Model& a_)
{
	foreach (const Product::component_type& c, a_.getSnapshot().getComponents())
	{
		if (t_.first.getFolder() == c.first.getFolder())
			return &c.second;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// struct Loader

Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> Loader::operator()(const Ct&) const
{
	PRL_RESULT e = PRL_ERR_SUCCESS;
	QString s = QString("%1/" VZ_CT_CONFIG_FILE).arg(m_path);
	SmartPtr<CVmConfiguration> p = CVzHelper::get_env_config_from_file(
				s, e, VZCTL_LAYOUT_5, true);
	if (!p)
		return e;
	return p;
}

Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> Loader::operator()(const Vm&) const
{
	QString s = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_path);
	SmartPtr<CVmConfiguration> p = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	PRL_RESULT code = CDspService::instance()->getVmConfigManager().loadConfig(
				p, s, m_client, false, true);
	if (PRL_FAILED(code))
		return code;
	return p;
}

namespace Acronis
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

QStringList Builder::operator()(Ct& variant_) const
{
	return variant_.buildArgs(m_component, m_file);
}

QStringList Builder::operator()(Vm& variant_) const
{
	return variant_.buildArgs(m_snapshot, m_component, m_file);
}

///////////////////////////////////////////////////////////////////////////////
// struct Archives

Product::componentList_type Archives::operator()(Ct&) const
{
	return m_product.getCtTibs();
}

Product::componentList_type Archives::operator()(Vm&) const
{
	return m_product.getVmTibs();
}

///////////////////////////////////////////////////////////////////////////////
// struct ACommand

QStringList ACommand::buildArgs(const ::Backup::Product::component_type& t_, const QFileInfo* f_,
		object_type& variant_)
{
	QStringList a(boost::apply_visitor(
			Builder(m_activity.getSnapshot().getUuid(), t_, f_), variant_));

	QString b = CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()
		->getBackupSourcePreferences()->getSandbox();
	a << "--sandbox" << b;

	if (m_context->getFlags() & PBT_UNCOMPRESSED)
		a << "--uncompressed";
	return a;
}

PRL_RESULT ACommand::do_(object_type& variant_)
{
	PRL_RESULT output = PRL_ERR_SUCCESS;
	foreach (const Product::component_type& t,
		boost::apply_visitor(Archives(*(m_context->getProduct().get())), variant_))
	{
		const QFileInfo* f = findArchive(t, m_activity);
		QStringList args = buildArgs(t, f, variant_);
		if (PRL_FAILED(output = m_worker(args, t.first.getDevice().getIndex())))
			break;
	}
	return output;
}

} // namespace Acronis

namespace Push
{
///////////////////////////////////////////////////////////////////////////////
// struct Mode

Prl::Expected<mode_type, PRL_RESULT> Mode::operator()(Ct&) const
{
	return mode_type(boost::blank());
}

Prl::Expected<mode_type, PRL_RESULT> Mode::operator()(Vm&) const
{
	Libvirt::Instrument::Agent::Vm::Unit u = Libvirt::Kit.vms().at(m_uuid);
	VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
	if (u.getState(s).isFailed())
		return PRL_ERR_VM_UUID_NOT_FOUND;

	if (s == VMS_STOPPED)
		return mode_type(Stopped(m_uuid));
	if (m_context->getProduct()->getObject().canFreeze())
		return mode_type(Frozen(m_context, m_uuid));

	return mode_type(boost::blank());
}

///////////////////////////////////////////////////////////////////////////////
// struct Builder

QStringList Builder::operator()(Ct& variant_) const
{
	return variant_.buildPushArgs(m_activity);
}

QStringList Builder::operator()(Vm& variant_) const
{
	return variant_.buildPushArgs();
}

///////////////////////////////////////////////////////////////////////////////
// struct VCommand

QStringList VCommand::buildArgs(object_type& variant_)
{
	QStringList a(boost::apply_visitor(Builder(m_activity), variant_));

	a << "-p" << m_context->getBackupUuid();
	if (m_context->getFlags() & PBT_UNCOMPRESSED)
		a << "--uncompressed";
	a << "--disp-mode";
	return a;
}

PRL_RESULT VCommand::do_(object_type& variant_)
{
	Prl::Expected<mode_type, PRL_RESULT> m =
		boost::apply_visitor(Mode(m_uuid, m_context), variant_);
	if (m.isFailed())
		return m.error();

	QStringList a(buildArgs(variant_));
	SmartPtr<Chain> p(m_builder(a));
	return boost::apply_visitor(Visitor(m_worker, p, a), m.value());
}

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

PRL_RESULT Visitor::operator()(const boost::blank&) const
{
	return m_worker(m_chain);
}

PRL_RESULT Visitor::operator()(Stopped& variant_) const
{
	return variant_.wrap(boost::bind(m_worker, m_chain));
}

PRL_RESULT Visitor::operator()(Frozen& variant_) const
{
	return m_worker(variant_.decorate(m_chain));
}

///////////////////////////////////////////////////////////////////////////////
// struct Frozen

SmartPtr<Chain> Frozen::decorate(SmartPtr<Chain> chain_)
{
	::Backup::Task::Reporter r(*m_context, m_uuid);
	::Backup::Task::Workbench w(*m_context, r, CDspService::instance()->getDispConfigGuard());
	if (PRL_FAILED(m_object.freeze(w)))
		return chain_;

	Thaw* t = new Thaw(m_object);
	t->moveToThread(QCoreApplication::instance()->thread());
	t->startTimer(20 * 1000);
	t->next(chain_);
	return SmartPtr<Chain>(t);
}

///////////////////////////////////////////////////////////////////////////////
// struct Thaw

Thaw::~Thaw()
{
	release();
}

void Thaw::release()
{
	QMutexLocker l(&m_lock);
	if (m_object) {
		m_object->thaw();
		m_object = boost::none;
	}
}

void Thaw::timerEvent(QTimerEvent *event_)
{
	killTimer(event_->timerId());
	release();
}

PRL_RESULT Thaw::do_(SmartPtr<IOPackage> request_, BackupProcess& dst_)
{
	release();
	return forward(request_, dst_);
}

typedef ::Command::Tag::State< ::Command::Vm::Frankenstein,
		::Command::Vm::Fork::State::Strict<VMS_PAUSED> > startPaused_type;

///////////////////////////////////////////////////////////////////////////////
// struct Stopped

template<class T>
PRL_RESULT Stopped::wrap(const T& worker_) const
{
	Libvirt::Result e = ::Command::Vm::Gear<startPaused_type>::run(m_uuid);
	if (e.isFailed()) {
		WRITE_TRACE(DBG_FATAL, "Failed to start a qemu process!");
		return e.error().code();
	}

	PRL_RESULT output = worker_();

	Libvirt::Instrument::Agent::Vm::Unit u = Libvirt::Kit.vms().at(m_uuid);
	VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
	u.getState(s);
	if (s == VMS_PAUSED) // check that vm is in an expected state
		u.kill();

	return output;
}

namespace Bitmap
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

QStringList Builder::operator()(Ct&) const
{
	QStringList a;
	a << "bitmaps_ct";
	foreach (const Product::component_type& c, m_components)
		a << "--image" << QString("ploop://%1").arg(c.second.absoluteFilePath());
	return a;
}

QStringList Builder::operator()(Vm&) const
{
	QStringList a;
	a << "bitmaps";
	a << "-n" << m_config->getVmIdentification()->getVmName();
	foreach (const Product::component_type& c, m_components)
		a << "--image" << c.first.getImage();
	return a;
}

///////////////////////////////////////////////////////////////////////////////
// struct Worker 

PRL_RESULT Worker::operator()(Stopped& variant_) const
{
	return variant_.wrap(m_worker);
}

template<class T>
PRL_RESULT Worker::operator()(const T&) const
{
	return m_worker();
}

///////////////////////////////////////////////////////////////////////////////
// struct Getter

PRL_RESULT Getter::run(const QStringList& args_, QString& output_)
{
	QProcess process;
	DefaultExecHandler h(process, args_.join(" "));
	WRITE_TRACE(DBG_INFO, "Run cmd: %s", QSTR2UTF8(args_.join(" ")));
	if (!HostUtils::RunCmdLineUtilityEx(args_, process, 60 * 1000, NULL)(h).isSuccess())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot retrieve bitmap information: %s", h.getStderr().constData());
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	output_ = process.readAllStandardOutput(); 
	return PRL_ERR_SUCCESS;
}

QStringList Getter::process(const QString& data_)
{
	typedef boost::property_tree::ptree tree_type;

	QStringList output;
	std::istringstream is(data_.toUtf8().data());
	tree_type r;
	try {
		boost::property_tree::json_parser::read_json(is, r);
	} catch (const boost::property_tree::json_parser::json_parser_error&) {
		WRITE_TRACE(DBG_FATAL, "Exception during parse of bitmap data");
		return output;
	}

	QMap<QString, int> m;
	BOOST_FOREACH(const tree_type::value_type& it, r) {
		if (it.first != "image")
			continue;
		boost::optional<const tree_type& > b = it.second.get_child_optional("bitmaps");
		if (!b)
			return output;
		BOOST_FOREACH(const tree_type::value_type& bit, (*b)) {
			boost::optional<std::string> s = bit.second.get_value_optional<std::string>();
			if (!s)
				continue;
			QString q((*s).c_str());
			m.insert(q, m.value(q, 0) + 1);
		}
	}
	qint32 count =  m_context->getProduct()->getVmTibs().size();
	foreach(QString u, m.keys()) {
		if (m.value(u) == count)
			output << u;
	}
	return output;
}

Prl::Expected<QStringList, PRL_RESULT> Getter::operator()(
		const Activity::Object::Model& activity_, object_type& variant_)
{
	QString u = m_config->getVmIdentification()->getVmUuid();
	Prl::Expected<mode_type, PRL_RESULT> m =
		boost::apply_visitor(Mode(u, m_context), variant_);
	if (m.isFailed())
		return m.error();

	QStringList a = boost::apply_visitor(Builder(m_config,
			activity_.getSnapshot().getComponents()), variant_);
	a.prepend(VZ_BACKUP_CLIENT);

	QString out;
	PRL_RESULT e = boost::apply_visitor(Worker(
			boost::bind(&run, a, boost::ref(out))), m.value());
	if (PRL_FAILED(e))
		return e;
	return process(out);
}

} // namespace Bitmap
} // namespace Push
} // namespace Work
} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// struct Chain

Chain::~Chain()
{
}

PRL_RESULT Chain::forward(SmartPtr<IOPackage> request_, BackupProcess& dst_)
{
	if (NULL == m_next.getImpl())
		return PRL_ERR_SUCCESS;

	return m_next->do_(request_, dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct GoodBye

struct GoodBye: Chain
{
	GoodBye(): m_yes(false)
	{
	}

	bool no() const
	{
		return !m_yes;
	}
	PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_);
private:
        bool m_yes;
};

PRL_RESULT GoodBye::do_(SmartPtr<IOPackage> request_, BackupProcess& dst_)
{
	if (ABackupProxyGoodBye != request_->header.type)
		return forward(request_, dst_);

	m_yes = true;
	WRITE_TRACE(DBG_DEBUG, "Backup client exited");
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Close

struct Close: Chain
{
	explicit Close(quint32 timeout_): m_timeout(timeout_)
	{
	}

	PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_);
private:
	quint32 m_timeout;
};

PRL_RESULT Close::do_(SmartPtr<IOPackage> request_, BackupProcess& dst_)
{
	if (ABackupProxyCloseRequest != request_->header.type)
		return forward(request_, dst_);

	// ABackupProxyCloseRequest is not implemented prior
	// to V3 but so backup_client wait reply from dst, will send valid reply
	// from local dispatcher
	qint32 nReply = ABackupProxyResponse;
	quint32 uReplySize = sizeof(nReply);
	PRL_RESULT e = dst_.write((char *)&uReplySize, sizeof(uReplySize));
	if (PRL_FAILED(e))
		return e;

	return dst_.write((char *)&nReply, sizeof(uReplySize));
}

///////////////////////////////////////////////////////////////////////////////
// struct Forward

PRL_RESULT Forward::do_(SmartPtr<IOPackage> request_, BackupProcess& dst_)
{
	IOSendJob::Handle hJob = m_client->sendPackage(request_);
	IOSendJob::Result res = m_client->waitForSend(hJob, m_timeout*1000);
	if (res != IOSendJob::Success)
	{
		WRITE_TRACE(DBG_FATAL, "Package sending failure, retcode %d", res);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	res = m_client->waitForResponse(hJob, m_timeout*1000);
	if (res != IOSendJob::Success)
	{
		WRITE_TRACE(DBG_FATAL, "Responce receiving failure, retcode %d", res);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	IOSendJob::Response resp = m_client->takeResponse(hJob);
	if (resp.responseResult != IOSendJob::Success)
	{
		WRITE_TRACE(DBG_FATAL, "Job failure: responseResult:%x", resp.responseResult);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	SmartPtr<IOPackage> pResponse = resp.responsePackages[0];
	if (!pResponse.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Invalid reply");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	quint32 uSize = 0;
	SmartPtr<char> b = pResponse->toBuffer(uSize);
	if (!b.getImpl())
	{
		WRITE_TRACE(DBG_FATAL, "IOPackage::toBuffer() error");
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	// send reply to proc
	PRL_RESULT e = dst_.write((char *)&uSize, sizeof(uSize));
	if (PRL_FAILED(e))
		return e;

	return dst_.write(b.getImpl(), uSize);
}

///////////////////////////////////////////////////////////////////////////////
// struct Progress

struct Progress: Chain
{
	Progress(CVmEvent& stub_, quint32 disk_, SmartPtr<IOPackage> src_);

	PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_);
private:
	quint32 m_disk;
	CVmEvent m_stub;
	SmartPtr<IOPackage> m_src;
};

Progress::Progress(CVmEvent& stub_, quint32 disk_, SmartPtr<IOPackage> src_):
	m_disk(disk_), m_stub(&stub_), m_src(src_)
{
}

PRL_RESULT Progress::do_(SmartPtr<IOPackage> request_, BackupProcess& dst_)
{
	if (ABackupProxyProgress != request_->header.type)
		return forward(request_, dst_);

	QString buffer = request_->buffers[0].getImpl();
	WRITE_TRACE(DBG_DEBUG, "handleProgress: progress=%s nDiskIdx=%d",
				QSTR2UTF8(buffer), m_disk);
	CVmEvent event(&m_stub);
	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
				buffer,
				EVT_PARAM_PROGRESS_CHANGED));
	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
				QString::number(m_disk),
				EVT_PARAM_DEVICE_INDEX));

	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event, m_src);
	// Use sendPackageToAllClients there to send event for use case:
	// restore non exists VM
	CDspService::instance()->getClientManager().sendPackageToAllClients(p);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Client

struct Client
{
	Client(BackupProcess* process_, const QStringList& args_);

	PRL_RESULT result(bool cancelled_, const QString& vm_, CVmEvent* event_);
	SmartPtr<IOPackage> pull(quint32 version_, SmartPtr<char> buffer_, qint64 cb_);
private:
	SmartPtr<IOPackage> doPull(SmartPtr<char> buffer_, qint64 cb_);

	PRL_RESULT m_start;
	QStringList m_argv;
	std::auto_ptr<BackupProcess> m_process;
};

Client::Client(BackupProcess* process_, const QStringList& argv_):
	m_start(PRL_ERR_UNINITIALIZED), m_argv(argv_), m_process(process_)
{
}

SmartPtr<IOPackage> Client::doPull(SmartPtr<char> buffer_, qint64 cb_)
{
	// read block size.
	qint32 nSize = 0;
	PRL_RESULT e = m_process->read((char *)&nSize, sizeof(nSize));
	if (PRL_FAILED(e))
		return SmartPtr<IOPackage>();
	// read data.
	if (cb_ < nSize)
	{
		WRITE_TRACE(DBG_FATAL, "Too small read buffer: %ld, requres: %ld", (long)cb_, (long)nSize);
		return SmartPtr<IOPackage>();
	}
	e = m_process->read(buffer_.getImpl(), nSize);
	if (PRL_FAILED(e))
		return SmartPtr<IOPackage>();

	SmartPtr<IOPackage> output;
	try
	{
		// handle incoming request, response will place into data
		output = IOPackage::createInstance(buffer_, nSize);
	}
	catch ( IOPackage::MalformedPackageException& )
	{
		WRITE_TRACE(DBG_FATAL, "MalformedPackageException in IOPackage::createInstance()");
	}
	if (NULL == output.getImpl())
		WRITE_TRACE(DBG_FATAL, "IOPackage::createInstance() error");

	return output;
}

SmartPtr<IOPackage> Client::pull(quint32 version_, SmartPtr<char> buffer_, qint64 cb_)
{
	PRL_RESULT e = m_start;
	if (PRL_ERR_UNINITIALIZED == e)
		m_start = m_process->start(m_argv, version_);

	if (PRL_FAILED(m_start))
		return SmartPtr<IOPackage>();
	if (PRL_ERR_UNINITIALIZED == e)
	{
		m_process->setReadFdBlock();
		WRITE_TRACE(DBG_INFO, "BACKUP_PROTO client version %u", version_);
	}
	SmartPtr<IOPackage> output = doPull(buffer_, cb_);
	if (NULL == output.getImpl())
		m_process->kill();

	return output;
}

PRL_RESULT Client::result(bool cancelled_, const QString& vm_, CVmEvent* event_)
{
	if (PRL_FAILED(m_start))
		return m_start;

	m_start = PRL_ERR_UNINITIALIZED;
	PRL_RESULT e = m_process->waitForFinished();
	if (PRL_ERR_BACKUP_ACRONIS_ERR == e)
	{
		e = (m_argv.at(0) == "restore" || m_argv.at(0) == "restore_ct") ?
			PRL_ERR_BACKUP_RESTORE_CMD_FAILED : PRL_ERR_BACKUP_BACKUP_CMD_FAILED;
		event_->setEventCode(e);
		event_->addEventParameter(new CVmEventParameter(PVE::String, vm_, EVT_PARAM_MESSAGE_PARAM_0));
		event_->addEventParameter(new CVmEventParameter(PVE::Integer,
				QString::number(m_process->exitCode()), EVT_PARAM_MESSAGE_PARAM_1));
		event_->addEventParameter(new CVmEventParameter(PVE::String,
				m_process->getError(), EVT_PARAM_DETAIL_DESCRIPTION));
	}
	return cancelled_ ? PRL_ERR_OPERATION_WAS_CANCELED : e;
}

namespace Backup
{
///////////////////////////////////////////////////////////////////////////////
// struct Perspective
//

Perspective::Perspective(const config_type& config_):
	m_config(config_)
{
	if (bad())
	{
 		WRITE_TRACE(DBG_FATAL, "[%s] Cannot get a VE hardware list",
					__FUNCTION__);
	}
}

Perspective::imageList_type Perspective::getImages() const
{
	imageList_type output;
	if (!bad())
	{
		foreach (CVmHardDisk* d, m_config->getVmHardwareList()->m_lstHardDisks)
		{
			if (d->getEnabled() && (
				d->getEmulatedType() == PVE::HardDiskImage ||
				d->getEmulatedType() == PVE::ContainerHardDisk))
				output.push_back(d);
		}
	}
	return output;
}

QString Perspective::getName(const QString& name_, const QStringList& met_)
{
	QString output = QFileInfo(name_).fileName().append(".tib");
	while (met_.contains(output))
		output.prepend("_");

	return output;
}

Perspective::config_type Perspective::clone(const QString& uuid_, const QString& name_) const
{
	if (bad())
		return config_type();

	config_type output(new CVmConfiguration(m_config.getImpl()));
	if (PRL_FAILED(output->m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot copy a VE config");
		return config_type();
	}
	output->getVmIdentification()->setVmUuid(uuid_);
	output->getVmIdentification()->setVmName(name_);
	Clone::Sink::Flavor<Clone::Sink::Vm::General>::prepare(output);
	foreach (CVmHardDisk* d, Perspective(output).getImages())
	{
		using namespace Clone::HardDisk;
		QFileInfo n(Flavor::getLocation(*d));
		if (n.isAbsolute())
			Flavor::update(*d, Flavor::getExternal(*d, name_));
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Suffix
//

QString Suffix::operator()() const
{
	QString output;
	if (m_index >= PRL_PARTIAL_BACKUP_START_NUMBER)
		output.append(QString(".%1").arg(m_index));
	return output.append(".qcow2");
}

} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// class Task_BackupHelper

Task_BackupHelper::Task_BackupHelper(const SmartPtr<CDspClient> &client, const SmartPtr<IOPackage> &p)
:CDspTaskHelper(client, p),
Task_DispToDispConnHelper(getLastError()),
m_pVmConfig(new CVmConfiguration()),
m_nInternalFlags(0),
m_nSteps(0),
m_product(NULL),
m_service(NULL)
{
	/* block size + our header size */
	m_nBufSize = IOPACKAGESIZE(1) + PRL_DISP_IO_BUFFER_SIZE;
	m_pBuffer = SmartPtr<char>(new char[m_nBufSize], SmartPtrPolicy::ArrayStorage);
	m_cABackupClient = NULL;
	m_bKillCalled = false;
	// will assume first backup proto version on dst side by default
	m_nRemoteVersion = BACKUP_PROTO_V1;
	// set backup client/server interface timeout (https://jira.sw.ru/browse/PSBM-10020)
	m_nBackupTimeout = CDspService::instance()->getDispConfigGuard().
			getDispCommonPrefs()->getBackupSourcePreferences()->getTimeout(); // in secs
	m_nBackupNumber = 0;
}

Task_BackupHelper::~Task_BackupHelper()
{
}

PRL_RESULT Task_BackupHelper::getEntryLists(const QString &sStartPath, bool (*excludeFunc)(const QString &))
{
	QFileInfo dirInfo;
	QFileInfoList entryList;
	QDir startDir, dir;
	int i, j;
	QString relativePath;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	dirInfo.setFile(sStartPath);
	if (!dirInfo.exists()) {
		WRITE_TRACE(DBG_FATAL, "Directory %s does not exist", QSTR2UTF8(sStartPath));
		return (PRL_ERR_VMDIR_INVALID_PATH);
	}
	startDir.setPath(sStartPath);
	m_DirList.append(qMakePair(dirInfo, QString(".")));
	for (i = 0; i < m_DirList.size(); ++i) {
		/* CDir::absoluteDir() is equal CDir::dir() : return parent directory */
		dir.setPath(m_DirList.at(i).first.absoluteFilePath());

		entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::Hidden);

		LOG_MESSAGE(DBG_DEBUG, "Directory %s", QSTR2UTF8(m_DirList.at(i).first.absoluteFilePath()));

		for (j = 0; j < entryList.size(); ++j) {
			const QFileInfo& fileInfo = entryList.at(j);

			// #431558 compare paths by spec way to prevent errors with symlinks, unexisting files, ...
			if (CFileHelper::IsPathsEqual(dirInfo.absoluteFilePath(), fileInfo.absoluteFilePath())) {
				WRITE_TRACE(DBG_FATAL, "Infinite recursion in : %s", QSTR2UTF8(dirInfo.absoluteFilePath()));
				return (PRL_ERR_FAILURE);
			}
			QFileInfo startInfo(sStartPath);
			if (!fileInfo.absoluteFilePath().startsWith(startInfo.absoluteFilePath())) {
				WRITE_TRACE(DBG_FATAL, "Path %s does not starts from VM home dir (%s)",
					QSTR2UTF8(fileInfo.absoluteFilePath()),
					QSTR2UTF8(startInfo.absoluteFilePath()));
				return PRL_ERR_FAILURE;
			}
			relativePath = startDir.relativeFilePath(fileInfo.absoluteFilePath());
			if (excludeFunc) {
				if (excludeFunc(relativePath)) {
					LOG_MESSAGE(DBG_DEBUG, "%s skipped due to excludes",
						QSTR2UTF8(fileInfo.absoluteFilePath()));
					continue;
				}
			}
			if (fileInfo.isDir())
				m_DirList.append(qMakePair(fileInfo, relativePath));
			else
				m_FileList.append(qMakePair(fileInfo, relativePath));
			LOG_MESSAGE(DBG_DEBUG, "%x\t%s.%s\t%s",
				int(fileInfo.permissions()),
				QSTR2UTF8(fileInfo.owner()),
				QSTR2UTF8(fileInfo.group()),
				QSTR2UTF8(fileInfo.absoluteFilePath()));
		}
		entryList.clear();
	}
	/* remove start directory */
	m_DirList.removeFirst();

	return (PRL_ERR_SUCCESS);
}

PRL_RESULT Task_BackupHelper::connect()
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	QString sLogin;
	QString sPw;

	{
		CDspLockedPointer<CDispCommonPreferences> dispPref = CDspService::instance()->
				getDispConfigGuard().getDispCommonPrefs();

		CDispBackupSourcePreferences *pBackupSource = dispPref->getBackupSourcePreferences();

		if (m_sServerSessionUuid.isEmpty())
		{
			if (pBackupSource->getDefaultBackupServer().isEmpty())
			{
				m_sServerHostname = "127.0.0.1";
				m_nServerPort = CDspService::getDefaultListenPort();
				//local backup. use current session
				SmartPtr<CDspClient> client = getActualClient();
				m_sServerSessionUuid = client->getClientHandle();
			}
			else
			{
				if (pBackupSource->getLogin().isEmpty() || !pBackupSource->isUsePassword())
				{
					return PRL_ERR_BACKUP_REQUIRE_LOGIN_PASSWORD;
				}
				m_sServerHostname = pBackupSource->getDefaultBackupServer();
				m_nServerPort = CDspService::getDefaultListenPort();
			}
		}

		sLogin = pBackupSource->getLogin();
		sPw = pBackupSource->getPassword();
	}

	return Task_DispToDispConnHelper::Connect(
			m_sServerHostname,
			m_nServerPort,
			m_sServerSessionUuid,
			sLogin,
			sPw,
			m_nFlags);
}

/* load data from .metadata file for Vm */
PRL_RESULT Task_BackupHelper::loadVmMetadata(const QString &sVmUuid, VmItem *pVmItem)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sPath = QString("%1/%2/" PRL_BACKUP_METADATA).arg(getBackupDirectory()).arg(sVmUuid);

	if (PRL_FAILED(nRetCode = pVmItem->loadFromFile(sPath))) {
		WRITE_TRACE(DBG_FATAL,
			"[%s] Error occurred while Vm metadata \"%s\" loading with code [%#x][%s]",
			__FUNCTION__, QSTR2UTF8(sPath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	}
	/* set vm uuid as directory name */
	pVmItem->setUuid(sVmUuid);
	return nRetCode;
}

PRL_RESULT Task_BackupHelper::loadVeConfig(const QString &backupUuid,
	const QString &path, PRL_VM_BACKUP_TYPE type, SmartPtr<CVmConfiguration>& conf)
{
	PRL_RESULT res = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> c;
	if (type == PVBT_VM) {
		QFile file(QDir(path).filePath(VMDIR_DEFAULT_VM_CONFIG_FILE));
		c = SmartPtr<CVmConfiguration>(new(std::nothrow) CVmConfiguration);
		if (!c)
			return PRL_ERR_OUT_OF_MEMORY;
		if (PRL_FAILED(res = lockShared(backupUuid)))
			return res;
		if (PRL_FAILED(res = c->loadFromFile(&file, false))) {
			WRITE_TRACE(DBG_FATAL, "Failed to load config file '%s'", QSTR2UTF8(file.fileName()));
			unlockShared(backupUuid);
			return res;
		}
	}
#ifdef _CT_
	else if (type == PVBT_CT_PLOOP || type == PVBT_CT_VZFS) {
		QString file(QDir(path).filePath(VZ_CT_CONFIG_FILE));
		if (PRL_FAILED(res = lockShared(backupUuid)))
			return res;
/*
		c = CVzHelper::get_env_config_from_file(file, res, 0,
			(type == PVBT_CT_PLOOP) * VZCTL_LAYOUT_5, true);
*/
		int x = 0;
		c = CVzHelper::get_env_config_from_file(file, x,
			(type == PVBT_CT_PLOOP) * VZCTL_LAYOUT_5, true);
		if (!c) {
			WRITE_TRACE(DBG_FATAL, "Failed to load config file '%s'", QSTR2UTF8(file));
			unlockShared(backupUuid);
			return PRL_FAILED(res) ? res : PRL_ERR_UNEXPECTED;
		}
	}
#endif // _CT_
	else {
		WRITE_TRACE(DBG_FATAL, "loading VE config for backup type %d is not implemented", type);
		res = PRL_ERR_UNEXPECTED;
	}

	if (PRL_SUCCEEDED(res))
		conf = c;
	unlockShared(backupUuid);
	return res;
}

/* load data from .metadata file for base backup */
PRL_RESULT Task_BackupHelper::loadBaseBackupMetadata(
		const QString &sVmUuid,
		const QString &sBackupUuid,
		BackupItem *pBackupItem)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QDateTime nullDateTime(QDate(1970, 1, 1));
	QString sPath = QString("%1/%2/%3/" PRL_BASE_BACKUP_DIRECTORY "/" PRL_BACKUP_METADATA)
		.arg(getBackupDirectory()).arg(sVmUuid).arg(sBackupUuid);

	if (PRL_FAILED(nRetCode = lockShared(sBackupUuid)))
		return nRetCode;

	pBackupItem->setDateTime(nullDateTime);
	if (PRL_FAILED(nRetCode = pBackupItem->loadFromFile(sPath))) {
		if (QFile::exists(sPath)) {
			WRITE_TRACE(DBG_FATAL,
				"[%s] Error occurred while backup metadata \"%s\" loading with code [%#x][%s]",
				__FUNCTION__, QSTR2UTF8(sPath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		} else {
			WRITE_TRACE(DBG_FATAL,
				"[%s] backup metadata file %s does not exist", __FUNCTION__, QSTR2UTF8(sPath));
			unlockShared(sBackupUuid);
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
	}
	/* in any case set Vm uuid */
	pBackupItem->setUuid(sBackupUuid);
	pBackupItem->setId(sBackupUuid);
	pBackupItem->setType(PRL_BACKUP_FULL_TYPE);

	unlockShared(sBackupUuid);
	return nRetCode;
}

/* load data from .metadata file for partial backup */
PRL_RESULT Task_BackupHelper::loadPartialBackupMetadata(
		const QString &sVmUuid,
		const QString &sBackupUuid,
		unsigned nBackupNumber,
		PartialBackupItem *pPartialBackupItem)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QDateTime nullDateTime(QDate(1970, 1, 1));
	QString sPath = QString("%1/%2/%3/%4/" PRL_BACKUP_METADATA)
		.arg(getBackupDirectory()).arg(sVmUuid).arg(sBackupUuid).arg(nBackupNumber);

	if (PRL_FAILED(nRetCode = lockShared(sBackupUuid)))
		return nRetCode;

	pPartialBackupItem->setDateTime(nullDateTime);
	if (PRL_FAILED(nRetCode = pPartialBackupItem->loadFromFile(sPath))) {
		WRITE_TRACE(DBG_FATAL,
			"[%s] Error occurred while backup metadata \"%s\" loading with code [%#x][%s]",
			__FUNCTION__, QSTR2UTF8(sPath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	}
	/* in any case set Vm uuid */
	pPartialBackupItem->setNumber(nBackupNumber);
	pPartialBackupItem->setId(QString("%1.%2").arg(sBackupUuid).arg(nBackupNumber));
	pPartialBackupItem->setType(PRL_BACKUP_INCREMENTAL_TYPE);

	unlockShared(sBackupUuid);
	return nRetCode;
}

/* get full backups uuid list for vm uuid */
void Task_BackupHelper::getBaseBackupList(
				const QString &sVmUuid,
				QStringList &lstBackupUuid,
				CAuthHelper *pAuthHelper,
				BackupCheckMode mode)
{
	QDir dir;
	QFileInfoList entryList;
	int i;

	lstBackupUuid.clear();
	dir.setPath(QString("%1/%2").arg(getBackupDirectory()).arg(sVmUuid));
	if (!dir.exists())
		return;
	if (!QFile::exists(QString("%1/" PRL_BACKUP_METADATA).arg(dir.absolutePath())))
		return;

	entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
	for (i = 0; i < entryList.size(); ++i) {
		if (pAuthHelper) {
			/* check client's permissions */
			if (mode == PRL_BACKUP_CHECK_MODE_WRITE) {
				if (!CFileHelper::FileCanWrite(entryList.at(i).absoluteFilePath(), pAuthHelper))
					continue;
			} else {
				if (!CFileHelper::FileCanRead(entryList.at(i).absoluteFilePath(), pAuthHelper))
					continue;
			}
		}
		if (!Uuid::isUuid(entryList.at(i).fileName()))
			continue;
		lstBackupUuid.append(entryList.at(i).fileName());
	}
}

BackupItem* Task_BackupHelper::getLastBaseBackup(const QString &sVmUuid,
			CAuthHelper *pAuthHelper, BackupCheckMode mode)
{
	int i;
	QStringList lstBackupUuid;
	std::auto_ptr<BackupItem> a, b;
	QDateTime lastDateTime(QDate(1970, 1, 1));

	getBaseBackupList(sVmUuid, lstBackupUuid, pAuthHelper, mode);
	for (i = 0; i < lstBackupUuid.size(); ++i) {
		b.reset(new BackupItem);
		loadBaseBackupMetadata(sVmUuid, lstBackupUuid.at(i), b.get());
		if (lastDateTime <= b->getDateTime()) {
			lastDateTime = b->getDateTime();
			a = b;
		}
	}
	return a.release();
}

PRL_RESULT Task_BackupHelper::updateLastPartialNumber(const QString &ve_,
		const QString &uuid_, unsigned number_)
{
	BackupItem b;
	QString path = QString("%1/%2/%3/" PRL_BASE_BACKUP_DIRECTORY "/" PRL_BACKUP_METADATA)
					.arg(getBackupDirectory()).arg(ve_).arg(uuid_); 
	PRL_RESULT e = b.loadFromFile(path);
	if (PRL_FAILED(e))
		return e;
	b.setLastNumber(number_);
	return b.saveToFile(path);
}

/* get partial backups list for vm uuid and full backup uuid */
void Task_BackupHelper::getPartialBackupList(
				const QString &sVmUuid,
				const QString &sBackupUuid,
				QList<unsigned> &lstBackupNumber)
{
	QDir dir;
	QFileInfoList entryList;
	int i, j;
	bool ok;
	unsigned number;

	lstBackupNumber.clear();
	dir.setPath(QString("%1/%2/%3").arg(getBackupDirectory()).arg(sVmUuid).arg(sBackupUuid));
	if (!dir.exists())
		return;

	entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
	for (i = 0; i < entryList.size(); ++i) {
		number = entryList.at(i).fileName().toUInt(&ok);
		if (!ok)
			continue;
		if (!QFile::exists(QString("%1/" PRL_BACKUP_METADATA).arg(entryList.at(i).absoluteFilePath())))
			continue;
		/* ordered list */
		for (j = 0; j < lstBackupNumber.size(); ++j)
			if (lstBackupNumber.at(j) > number)
				break;
		lstBackupNumber.insert(j, number);
	}
}

unsigned Task_BackupHelper::getNextPartialBackup(const QString &sVmUuid, const QString &sBackupUuid)
{
	QList<unsigned> lstBackupNumber;
	getPartialBackupList(sVmUuid, sBackupUuid, lstBackupNumber);
	if (lstBackupNumber.size())
		return lstBackupNumber.last() + 1;
	else
		return PRL_PARTIAL_BACKUP_START_NUMBER;
}

PRL_RESULT Task_BackupHelper::getBackupParams(const QString &sVmUuid, const QString &sBackupUuid,
		unsigned nBackupNumber, quint64 &nSize, quint32 &nBundlePermissions)
{
	PRL_RESULT nRetCode;

	if (nBackupNumber == PRL_BASE_BACKUP_NUMBER)
	{
		BackupItem bItem;
		nRetCode = loadBaseBackupMetadata(sVmUuid, sBackupUuid, &bItem);
		if (PRL_FAILED(nRetCode))
			return nRetCode;
		nSize = bItem.getOriginalSize();
		nBundlePermissions = bItem.getBundlePermissions();
	}
	else
	{
		PartialBackupItem bItem;
		nRetCode = loadPartialBackupMetadata(sVmUuid, sBackupUuid, nBackupNumber, &bItem);
		if (PRL_FAILED(nRetCode))
			return nRetCode;
		nSize = bItem.getOriginalSize();
		nBundlePermissions = bItem.getBundlePermissions();
	}
	return nRetCode;
}

PRL_RESULT Task_BackupHelper::checkFreeDiskSpace(
	quint64 nRequiredSize, quint64 nAvailableSize, bool bIsCreateOp)
{
	if (nRequiredSize == 0)
	{
		WRITE_TRACE(DBG_FATAL, "Warning: check available disk space is skipped");
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT nRetCode = 0;
	if (nRequiredSize > nAvailableSize)
	{

		QString strFree;
		strFree.sprintf("%.1f", (float)nAvailableSize / 1024.0 / 1024.0 / 1024.0);
		QString strSize;
		strSize.sprintf("%.1f", (float)nRequiredSize / 1024.0 / 1024.0 / 1024.0);

		{
			nRetCode = bIsCreateOp ? PRL_ERR_BACKUP_CREATE_NOT_ENOUGH_FREE_DISK_SPACE :
					PRL_ERR_BACKUP_RESTORE_NOT_ENOUGH_FREE_DISK_SPACE;

			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(PVE::String, strSize, EVT_PARAM_MESSAGE_PARAM_0));
			pEvent->addEventParameter(new CVmEventParameter(PVE::String, strFree, EVT_PARAM_MESSAGE_PARAM_1));

		}
		WRITE_TRACE(DBG_FATAL, "Not enought free disk space avail=%llu required=%llu",
				nAvailableSize, nRequiredSize);

	}
	return nRetCode;
}

PRL_RESULT Task_BackupHelper::parseBackupId(const QString &sBackupId, QString &sBackupUuid, unsigned &nBackupNumber)
{
	int nIndex;

	if ((nIndex = sBackupId.indexOf(".")) == -1) {
		sBackupUuid = sBackupId;
		nBackupNumber = PRL_BASE_BACKUP_NUMBER;
	} else {
		QString sBackupDir;
		bool ok;
		/* parse {BackupUuid}.BackupNumber pair */
		sBackupUuid = sBackupId.left(nIndex);
		sBackupDir = sBackupId.right(sBackupId.size() - nIndex - 1);
		nBackupNumber = sBackupDir.toUInt(&ok);
		if (!ok)
			return PRL_ERR_INVALID_ARG;
	}
	if (!Uuid::isUuid(sBackupUuid))
		return PRL_ERR_INVALID_ARG;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_BackupHelper::findVmUuidForBackupUuid(const QString &sBackupUuid, QString &sVmUuid)
{
	QDir dir, dirVm;
	QFileInfoList listVm, listBackup;
	int i, j;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	dir.setPath(getBackupDirectory());
	if (!dir.exists())
		return PRL_ERR_FAILURE;

	listVm = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
	for (i = 0; i < listVm.size(); ++i) {
		if (!Uuid::isUuid(listVm.at(i).fileName()))
			continue;
		dirVm.setPath(listVm.at(i).absoluteFilePath());
		if (!dirVm.exists())
			continue;

		listBackup = dirVm.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
		for (j = 0; j < listBackup.size(); ++j) {
			if (sBackupUuid == listBackup.at(j).fileName()) {
				sVmUuid = listVm.at(i).fileName();
				return PRL_ERR_SUCCESS;
			}
		}
	}
	return PRL_ERR_FAILURE;
}

QString Task_BackupHelper::getBackupDirectory()
{
	CDspLockedPointer<CDispatcherConfig> pLockedDispConfig =
		CDspService::instance()->getDispConfigGuard().getDispConfig();
	QString sBackupDirectory =
		pLockedDispConfig->getDispatcherSettings()->getCommonPreferences()
			->getBackupTargetPreferences()->getDefaultBackupDirectory();

	if (sBackupDirectory.isEmpty())
		return ParallelsDirs::getDefaultBackupDir();
	else
		return sBackupDirectory;
}

bool Task_BackupHelper::isBackupDirEmpty(const QString &sVmUuid, const QString &sBackupUuid)
{
	QString sBackupDir = QString("%1/%2/%3").arg(getBackupDirectory()).arg(sVmUuid).arg(sBackupUuid);
	QFileInfo dir;
	QList<unsigned> lstBackupNumber;

	dir.setFile(QString("%1/" PRL_BASE_BACKUP_DIRECTORY).arg(sBackupDir));
	if (dir.exists())
		return false;

	getPartialBackupList(sVmUuid, sBackupUuid, lstBackupNumber);
	return lstBackupNumber.isEmpty();
}

static QMutex g_mutex;
static QHash <QString, Task_BackupHelper *> g_exclusiveLocks;
static QMultiHash <QString, Task_BackupHelper *> g_sharedLocks;

PRL_RESULT Task_BackupHelper::lockShared(const QString &sBackupUuid)
{
	PRL_ASSERT(!sBackupUuid.isEmpty());
	QMutexLocker locker(&g_mutex);

	if (g_exclusiveLocks.find(sBackupUuid) != g_exclusiveLocks.end()) {
		WRITE_TRACE(DBG_FATAL, "[%s] Backup %s is already locked for writing",
			__FUNCTION__, QSTR2UTF8(sBackupUuid));
		return PRL_ERR_BACKUP_LOCKED_FOR_WRITING;
	}
	g_sharedLocks.insert(sBackupUuid, this);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_BackupHelper::lockExclusive(const QString &sBackupUuid)
{
	PRL_ASSERT(!sBackupUuid.isEmpty());
	QMutexLocker locker(&g_mutex);

	if (g_sharedLocks.find(sBackupUuid) != g_sharedLocks.end()) {
		WRITE_TRACE(DBG_FATAL, "[%s] Backup %s is already locked for reading",
			__FUNCTION__, QSTR2UTF8(sBackupUuid));
		return PRL_ERR_BACKUP_LOCKED_FOR_READING;
	}

	if (g_exclusiveLocks.find(sBackupUuid) != g_exclusiveLocks.end()) {
		WRITE_TRACE(DBG_FATAL, "[%s] Backup %s is already locked for writing",
			__FUNCTION__, QSTR2UTF8(sBackupUuid));
		return PRL_ERR_BACKUP_LOCKED_FOR_WRITING;
	}

	g_exclusiveLocks.insert(sBackupUuid, this);
	return PRL_ERR_SUCCESS;
}

void Task_BackupHelper::unlockShared(const QString &sBackupUuid)
{
	PRL_ASSERT(!sBackupUuid.isEmpty());
	QMutexLocker locker(&g_mutex);
	if (g_sharedLocks.remove(sBackupUuid, this) == 0) {
		WRITE_TRACE(DBG_FATAL, "[%s] Unlock for non-locked backup %s",
			__FUNCTION__, QSTR2UTF8(sBackupUuid));
	}
}

void Task_BackupHelper::unlockExclusive(const QString &sBackupUuid)
{
	PRL_ASSERT(!sBackupUuid.isEmpty());
	QMutexLocker locker(&g_mutex);
	QHash <QString, Task_BackupHelper *>::iterator it;
	it = g_exclusiveLocks.find(sBackupUuid);
	if (it == g_exclusiveLocks.end()) {
		WRITE_TRACE(DBG_FATAL, "[%s] Unlock for non-locked backup %s",
			__FUNCTION__, QSTR2UTF8(sBackupUuid));
		return;
	}
	while (it != g_exclusiveLocks.end() && it.key() == sBackupUuid) {
		PRL_ASSERT(it.value() == this);
		it = g_exclusiveLocks.erase(it);
	}
}

BackupProcess::BackupProcess() : m_isKilled(false)
{
	for (int j = 0; j < 2 ; j++)
		m_out[j] = -1;
}

BackupProcess::~BackupProcess()
{
	for (int j = 0; j < 2 ; j++) {
		if (m_out[j] != -1)
			::close(m_out[j]);
	}
}

void BackupProcess::setupChildProcess()
{
	QProcess::setupChildProcess(); // requires vz-built qt version
	fcntl(m_out[1], F_SETFD, ~FD_CLOEXEC);
}

PRL_RESULT BackupProcess::start(const QStringList& arg_, int version_)
{
	PRL_ASSERT(arg_.size());
	QStringList a(arg_);
	m_sCmd = a.takeFirst();
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_out) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() error : %s", strerror(errno));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}

	if (BACKUP_PROTO_V4 > version_) {
		// old syntax, prepend arguments by fds
		a.insert(0, QString::number(STDIN_FILENO));
		a.insert(1, QString::number(m_out[1]));
	} else {
		// new syntax, append arguments by named fd args
		a << "--fdin" << QString::number(STDIN_FILENO);
		a << "--fdout" << QString::number(m_out[1]);
	}
	WRITE_TRACE(DBG_WARNING, "Run cmd: %s", QSTR2UTF8(a.join(" ")));

	for (int j = 0; j < 2 ; j++) {
		long flags;
		if ((flags = fcntl(m_out[j], F_GETFL)) != -1)
			fcntl(m_out[j], F_SETFL, flags | O_NONBLOCK);
	}
	QProcess::start(m_sCmd, a, QIODevice::Unbuffered | QIODevice::ReadWrite);
	if (!waitForStarted())
		return PRL_ERR_BACKUP_TOOL_CANNOT_START;
	::close(m_out[1]);
	m_out[1] = -1;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT BackupProcess::waitForFinished()
{
	QProcess::waitForFinished(-1);
	if (exitStatus() == QProcess::CrashExit) {
		WRITE_TRACE(DBG_FATAL, "%s have crashed", QSTR2UTF8(m_sCmd));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}

	if ((exitCode())) {
		m_error = readAllStandardError();
		WRITE_TRACE(DBG_FATAL, "%s exited with code %d, error: %s", QSTR2UTF8(m_sCmd),
			exitCode(), QSTR2UTF8(m_error));
		return PRL_ERR_BACKUP_ACRONIS_ERR;
	}
	return PRL_ERR_SUCCESS;
}

QString BackupProcess::getError()
{
	return m_error;
}

void BackupProcess::kill()
{
	QProcess::terminate();
	m_isKilled = true;
}

PRL_RESULT BackupProcess::read(char *data, qint32 size)
{
	return read(data, size, 0);
}

PRL_RESULT BackupProcess::read(char *data, qint32 size, UINT32 tmo)
{
	int rc;
	size_t count;
	SmartPtr<fd_set> fds(IOService::allocFDSet(m_out[0] + 1));
	struct timeval tv;
	unsigned nTimeout = tmo/TARGET_DISPATCHER_TIMEOUT_COUNTS; // in sec

	count = 0;
	while (1) {
		/* read data */
		while (1) {
			errno = 0;
			rc = ::read(m_out[0], data + count, size - count);
			if (rc > 0) {
				count += rc;
				if (count >= (size_t)size) {
					return PRL_ERR_SUCCESS;
				}
				continue;
			} else if (rc == 0) {
				/* end of file - pipe was close, will check client exit code */
				WRITE_TRACE(DBG_DEBUG, "[%s] EOF", __FUNCTION__);
				return PRL_ERR_BACKUP_INTERNAL_ERROR;
			}
			if ((errno == EAGAIN) || (errno == EINTR)) {
				break;
			} else {
				WRITE_TRACE(DBG_FATAL, "[%s] read() error : %s", __FUNCTION__, strerror(errno));
				return PRL_ERR_BACKUP_INTERNAL_ERROR;
			}
		}

		if (!tmo)
			continue;

		/* wait next data in socket */
		for (int i = 1; i <= TARGET_DISPATCHER_TIMEOUT_COUNTS; i++) {
			if (m_isKilled)
				return PRL_ERR_OPERATION_WAS_CANCELED;
			do {
				::memset(fds.getImpl(), 0, FDNUM2SZ(m_out[0] + 1));
				FD_SET(m_out[0], fds.getImpl());
				tv.tv_sec = nTimeout;
				tv.tv_usec = 0;
				rc = select(m_out[0] + 1, fds.getImpl(), NULL, NULL, &tv);
				if (rc == 0 && i == TARGET_DISPATCHER_TIMEOUT_COUNTS) {
					WRITE_TRACE(DBG_FATAL, "[%s] full timeout expired (%d sec)",
						__FUNCTION__, tmo);
					return PRL_ERR_BACKUP_INTERNAL_ERROR;
				} else if (rc == 0) {
					WRITE_TRACE(DBG_DEBUG, "[%s] partial timeout expired (%d sec)",
						__FUNCTION__, nTimeout*i);
					break;
				} else if (rc < 0) {
					WRITE_TRACE(DBG_FATAL, "[%s] select() error : %s", __FUNCTION__, strerror(errno));
					return PRL_ERR_BACKUP_INTERNAL_ERROR;
				}
			} while(!FD_ISSET(m_out[0], fds.getImpl()));
			if (rc > 0)
				break;
		}
	}

	/* but we never should be here */
	return PRL_ERR_BACKUP_INTERNAL_ERROR;
}

PRL_RESULT BackupProcess::write(char *data, quint32 size)
{
	if (size == 0)
		return PRL_ERR_SUCCESS;

	quint32 count = 0;
	while (count < size) {
		int rc = QIODevice::write(data + count, size - count);
		if (rc < 0) {
			WRITE_TRACE(DBG_FATAL, "write() error : %s", QSTR2UTF8(errorString()));
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
		do {
			if (!waitForBytesWritten(-1)) {
				WRITE_TRACE(DBG_FATAL, "Failed to write to backup process: %s",
						QSTR2UTF8(QIODevice::errorString())); 
				return PRL_ERR_BACKUP_INTERNAL_ERROR;
			}
		} while (bytesToWrite());
		count += rc;
	}
	return PRL_ERR_SUCCESS;
}

void BackupProcess::setReadFdBlock()
{
	long flags;
	if ((flags = fcntl(m_out[0], F_GETFL)) != -1)
		fcntl(m_out[0], F_SETFL, flags & ~O_NONBLOCK);
}

PRL_RESULT Task_BackupHelper::handleABackupPackage(
				const SmartPtr<CDspDispConnection> &pDispConnection,
				const SmartPtr<IOPackage> &pRequest,
				UINT32 tmo)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> pResponse;
	SmartPtr<char> pBuffer;
	qint32 nSize;
	quint32 uSize;
	IOSendJob::Handle hJob;

	if (!IS_ABACKUP_PROXY_PACKAGE(pRequest->header.type))
		return PRL_ERR_SUCCESS;

	pBuffer = pRequest->toBuffer(uSize);
	if (!pBuffer.getImpl()) {
		WRITE_TRACE(DBG_FATAL, "IOPackage::toBuffer() error");
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	if (pRequest->header.type == ABackupProxyProgress)
		return PRL_ERR_SUCCESS;
	else if (pRequest->header.type == ABackupProxyCancelCmd)
		WRITE_TRACE(DBG_FATAL, "receive ABackupProxyCancelCmd command");
	/* write request to process */
	if (PRL_FAILED(nRetCode = m_cABackupServer.write((char *)&uSize, sizeof(uSize))))
		return nRetCode;
	if (PRL_FAILED(nRetCode = m_cABackupServer.write(pBuffer.getImpl(), uSize)))
		return nRetCode;
	/* and read reply */
	if (PRL_FAILED(nRetCode = m_cABackupServer.read((char *)&nSize, sizeof(nSize), tmo)))
		return nRetCode;
	if (m_nBufSize < nSize) {
		WRITE_TRACE(DBG_FATAL, "Too small read buffer: %ld, requires: %ld", (long)m_nBufSize, (long)nSize);
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	if (PRL_FAILED(nRetCode = m_cABackupServer.read(m_pBuffer.getImpl(), nSize, tmo)))
		return nRetCode;

	// send reply to client
	pResponse = IOPackage::createInstance(m_pBuffer, nSize, pRequest);
	if (!pResponse.getImpl()) {
		WRITE_TRACE(DBG_FATAL, "IOPackage::createInstance() error");
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	hJob = pDispConnection->sendPackage(pResponse);
	if (CDspService::instance()->getIOServer().waitForSend(hJob, tmo*1000) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

void Task_BackupHelper::killABackupClient()
{
	if (m_cABackupClient) {
		m_bKillCalled = true;
		m_cABackupClient->kill();
	}
}

PRL_RESULT Task_BackupHelper::startABackupClient(const QString& sVmName_, const QStringList& args_,
		SmartPtr<Chain> custom_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	SmartPtr<Chain> y = custom_;
	// NB. should not allow to override bye and close logic.
	// put them in the very beginning of the chain. the rest
	// is optional.
	if (BACKUP_PROTO_V3 > m_nRemoteVersion)
	{
		Chain* z = new Close(m_nBackupTimeout);
		z->next(y);
		y = SmartPtr<Chain>(z);
	}
	GoodBye* b = new GoodBye;
	b->next(y);
	y = SmartPtr<Chain>(b);

	QStringList a(args_);
	a.prepend((BACKUP_PROTO_V4 > m_nRemoteVersion) ?
                        QString(PRL_ABACKUP_CLIENT) : QString(VZ_BACKUP_CLIENT));
	a << "--timeout" << QString::number(m_nBackupTimeout);
	Client x(m_cABackupClient = new BackupProcess, a);
	while (b->no())
	{
		SmartPtr<IOPackage> q = x.pull(m_nRemoteVersion, m_pBuffer, m_nBufSize);
		if (NULL == q.getImpl())
			break;
        	if (PRL_FAILED(y->do_(q, *m_cABackupClient)))
		{
			m_cABackupClient->kill();
                	break;
		}
	}
	PRL_RESULT output = x.result(m_bKillCalled, sVmName_, getLastError());
	m_bKillCalled = false;
	m_cABackupClient = NULL;
	return output;
}

Chain * Task_BackupHelper::prepareABackupChain(const QStringList& args_,
		const QString &sNotificationVmUuid, unsigned int nDiskIdx)
{
	bool bRestore = (args_.at(0) == "restore" || args_.at(0) == "restore_ct");
	PRL_EVENT_TYPE event_type = bRestore ? PET_DSP_EVT_RESTORE_PROGRESS_CHANGED : PET_DSP_EVT_BACKUP_PROGRESS_CHANGED;
	CVmEvent e(event_type, sNotificationVmUuid, PIE_DISPATCHER);
	Chain *p = new Progress(e, nDiskIdx, getRequestPackage());
	if (args_.indexOf("--local") == -1)
		p->next(SmartPtr<Chain>(new Forward(m_pIoClient, m_nBackupTimeout)));
	return p;
}

PRL_RESULT Task_BackupHelper::startABackupClient(const QString& sVmName_, const QStringList& args_,
		const QString &sNotificationVmUuid, unsigned int nDiskIdx)
{
	Chain *p = prepareABackupChain(args_, sNotificationVmUuid, nDiskIdx);
	return startABackupClient(sVmName_, args_, SmartPtr<Chain>(p));
}

PRL_RESULT Task_BackupHelper::GetBackupTreeRequest(const QString &sVmUuid, QString &sBackupTree)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pCmd;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<IOPackage> pReply;

	pCmd = CDispToDispProtoSerializer::CreateGetBackupTreeCommand(sVmUuid, m_nFlags);

	pPackage = DispatcherPackage::createInstance(
			pCmd->GetCommandId(),
			pCmd->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply)))
		return nRetCode;

	if (	(pReply->header.type != VmBackupGetTreeReply) &&
		(pReply->header.type != DispToDispResponseCmd))
	{
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x or %x",
			pReply->header.type, DispToDispResponseCmd, VmBackupGetTreeReply);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	CDispToDispCommandPtr pDspReply = CDispToDispProtoSerializer::ParseCommand(
		DispToDispResponseCmd, UTF8_2QSTR(pReply->buffers[0].getImpl()));

	if (pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pDspReply);

		if (PRL_FAILED(nRetCode = pResponseCmd->GetRetCode())) {
			getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
			return nRetCode;
		}
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	CGetBackupTreeReply *pTreeReply =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CGetBackupTreeReply>(pDspReply);

	sBackupTree = pTreeReply->GetBackupTree();

	return nRetCode;
}

PRL_RESULT Task_BackupHelper::CloneHardDiskState(const QString &sDiskImage,
	const QString &sSnapshotUuid, const QString &sDstDirectory)
{
/*
	CDSManager DSManager;
	PRL_RESULT RetVal;

	IDisk *pDisk = IDisk::OpenDisk(sDiskImage, PRL_DISK_READ |
			PRL_DISK_NO_ERROR_CHECKING | PRL_DISK_FAKE_OPEN, &RetVal);
	if (!pDisk) {
		WRITE_TRACE(DBG_FATAL, "Can't open disk for clone, ret = %#x (%s)",
				RetVal, PRL_RESULT_TO_STRING(RetVal));
		return RetVal;
	}
	DSManager.AddDisk( pDisk );

	PRL_RESULT nRetCode = DSManager.CloneState( Uuid( sSnapshotUuid ),
			QStringList() << sDstDirectory, NULL, NULL );

	DSManager.WaitForCompletion();
	DSManager.Clear();
	pDisk->Release();

	return nRetCode;
*/
	Q_UNUSED(sDiskImage);
	Q_UNUSED(sSnapshotUuid);
	Q_UNUSED(sDstDirectory);
	return PRL_ERR_UNIMPLEMENTED;
}
