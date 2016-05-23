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
#include "Task_CloneVm_p.h"
#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"

#include "Task_BackupHelper.h"
#include "Task_BackupHelper_p.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "CDspVmStateSender.h"
//#include "VI/Sources/BackupTool/ABackup/AcronisWrap/Interface/BackupErrors.h"

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
// struct State

VIRTUAL_MACHINE_STATE State::operator()(Ct&) const
{
	VIRTUAL_MACHINE_STATE s; 
	PRL_RESULT res = CDspService::instance()->getVzHelper()->
			getVzlibHelper().get_env_status(m_uuid, s);
	if (PRL_FAILED(res))
		return VMS_UNKNOWN;
	return s;
}

VIRTUAL_MACHINE_STATE State::operator()(Vm&) const
{
	Libvirt::Instrument::Agent::Vm::Unit u = Libvirt::Kit.vms().at(m_uuid);
	VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
	if (u.getState(s).isFailed())
		return VMS_UNKNOWN;
	return s;
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

	// current and previous PITs calculation
	unsigned i = m_context->getBackupNumber();
	QString u = m_context->getBackupUuid();
	if ((m_context->getFlags() & PBT_INCREMENTAL) && i) {
		a << "-p" << QString("%1.%2").arg(u).arg(i);
		QString s(u);
		if (i > PRL_PARTIAL_BACKUP_START_NUMBER)
			s += QString(".%1").arg(i - 1);
		a << "--last-pit" << s;
	} else
		a << "-p" << u;

	if (m_context->getFlags() & PBT_UNCOMPRESSED)
		a << "--uncompressed";
	a << "--disp-mode";
	return a;
}

Prl::Expected<VCommand::mode_type, PRL_RESULT> VCommand::getMode(object_type& variant_)
{
	VIRTUAL_MACHINE_STATE s = boost::apply_visitor(State(m_uuid), variant_);
	if (s == VMS_UNKNOWN)
		return PRL_ERR_VM_UUID_NOT_FOUND;

	if (s == VMS_STOPPED)
		return mode_type(Stopped(m_uuid));
	if (m_context->getProduct()->getObject().canFreeze())
		return mode_type(Frozen(m_context, m_uuid));

	return mode_type(boost::blank());
}

PRL_RESULT VCommand::do_(object_type& variant_)
{
	Prl::Expected<mode_type, PRL_RESULT> m = getMode(variant_);
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

///////////////////////////////////////////////////////////////////////////////
// struct Stopped

template<class T>
PRL_RESULT Stopped::wrap(const T& worker_) const
{
	Libvirt::Instrument::Agent::Vm::Unit u = Libvirt::Kit.vms().at(m_uuid);
	Libvirt::Result e = u.startPaused();
	if (e.isFailed())
		return e.error().code();

	PRL_RESULT output = worker_();

	u = Libvirt::Kit.vms().at(m_uuid); // refresh unit - there could be a reconnect meanwhile
	VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
	u.getState(s);
	if (s == VMS_PAUSED) // check that vm is in an expected state
		u.kill();

	return output;
}

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
	PRL_RESULT e = dst_.writeToABackupClient(
				(char *)&uReplySize, sizeof(uReplySize), m_timeout);
	if (PRL_FAILED(e))
		return e;

	return dst_.writeToABackupClient((char *)&nReply, sizeof(uReplySize), m_timeout);
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
	PRL_RESULT e = dst_.writeToABackupClient((char *)&uSize, sizeof(uSize), m_timeout);
	if (PRL_FAILED(e))
		return e;

	return dst_.writeToABackupClient(b.getImpl(), uSize, m_timeout);
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

	static QString getAcronisErrorString(int code_);
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
	PRL_RESULT e = m_process->readFromABackupClient((char *)&nSize, sizeof(nSize));
	if (PRL_FAILED(e))
		return SmartPtr<IOPackage>();
	// read data.
	if (cb_ < nSize)
	{
		WRITE_TRACE(DBG_FATAL, "Too small read buffer: %ld, requres: %ld", (long)cb_, (long)nSize);
		return SmartPtr<IOPackage>();
	}
	e = m_process->readFromABackupClient(buffer_.getImpl(), nSize);
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
		m_process->setInFdNonBlock();
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
		QString sErr = getAcronisErrorString(m_process->getExitCode()), sENum;
		if (m_argv.at(0) == "restore" || m_argv.at(0) == "restore_ct") {
			event_->setEventCode(e = PRL_ERR_BACKUP_RESTORE_CMD_FAILED);
			WRITE_TRACE(DBG_FATAL, "[%s] restore client failed for Vm \"%s\": retcode=%d, '%s'",
				__FUNCTION__, QSTR2UTF8(vm_), e, QSTR2UTF8(sErr));
		} else {
			event_->setEventCode(e = PRL_ERR_BACKUP_BACKUP_CMD_FAILED);
			WRITE_TRACE(DBG_FATAL, "[%s] backup client failed for Vm \"%s\": retcode=%d, '%s'",
				__FUNCTION__, QSTR2UTF8(vm_), e, QSTR2UTF8(sErr));
		}
		sENum.setNum(m_process->getExitCode());
		event_->addEventParameter(new CVmEventParameter(PVE::String, vm_, EVT_PARAM_MESSAGE_PARAM_0));
		event_->addEventParameter(new CVmEventParameter(PVE::Integer, sENum, EVT_PARAM_MESSAGE_PARAM_1));
		event_->addEventParameter(new CVmEventParameter(PVE::String, sErr, EVT_PARAM_MESSAGE_PARAM_2));
	}
	return cancelled_ ? PRL_ERR_OPERATION_WAS_CANCELED : e;
}

QString Client::getAcronisErrorString(int code_)
{
	Q_UNUSED(code_);
	QString str;
/*
	switch (code_)
	{
	case BCKP_OK:
		str = "No errors";
		break;
	case BCKP_READ_ERROR:
		str = "Read error";
		break;
	case BCKP_WRITE_ERROR:
		str = "Write error";
		break;
	case BCKP_SEEK_ERROR:
		str = "Seek error";
		break;
	case BCKP_OUT_OF_MEMORY:
		str = "Out of memory";
		break;
	case BCKP_OPEN_ERROR:
		str = "Open error";
		break;
	case BCKP_REMOVE_ERROR:
		str = "Remove error";
		break;
	case BCKP_RENAME_ERROR:
		str = "Rename error";
		break;
	case BCKP_CREATE_ERROR:
		str = "Create error";
		break;
	case BCKP_NOT_READY:
		str = "Is not ready";
		break;
	case BCKP_CORRUPTED:
		str = "Corrupted";
		break;
	case BCKP_UNSUPPORTED:
		str = "Unsupported";
		break;
	case BCKP_CANCELED:
		str = "Operation canceled";
		break;
	case BCKP_DEVICE_BUSY:
		str = "Device is busy";
		break;
	case BCKP_NOT_FOUND:
		str = "Object not found";
		break;
	case BCKP_NOT_LAST_VOLUME:
		str = "It is not a last volume";
		break;
	case BCKP_ALREADY_EXIST:
		str = "Object already exist";
		break;
	case BCKP_ACCESS_DENIED:
		str = "Access denied";
		break;
	case BCKP_NETWORK_ERROR:
		str = "Network error";
		break;
	case BCKP_NETWORK_TIMEOUT:
		str = "Timeout exceeded";
		break;
	case BCKP_FORMAT_DISK:
		str = "Disk format error";
		break;
	case BCKP_RESTORE_ERROR:
		str = "Restore error";
		break;
	case BCKP_DECRYPT_ERROR:
		str = "Decrypt error";
		break;
	case BCKP_UMOUNT_ERROR:
		str = "Umount error";
		break;
	case BCKP_DISK_FULL:
		str = "No free space";
		break;
	case BCKP_EOF:
		str = "Unexpected eof";
		break;
	case BCKP_BACKUP_ERROR:
		str = "Backup error";
		break;
	case BCKP_READONLY:
		str = "Read-only disk";
		break;
	case BCKP_LOCK_ERROR:
		str = "No lock";
		break;
	case BCKP_NO_CONTEXT:
		str = "No context";
		break;
	case BCKP_NO_CUR_DISK:
		str = "No current disk";
		break;
	case BCKP_NO_BUILDER:
		str = "No backup image builder";
		break;
	case BCKP_MINMAX:
		str = "Invalid parameters for format/resize";
		break;
	case BCKP_PROPERTY:
		str = "Invalid property";
		break;
	case BCKP_NO_CUR_PART:
		str = "No current partition";
		break;
	case BCKP_RESIZE:
		str = "Format/Resize error (after COMMIT)";
		break;
	case BCKP_PARAM:
		str = "Invalid parameter";
		break;
	case BCKP_NO_DISK:
		str = "No disk";
		break;
	case BCKP_NO_ROOM:
		str = "No room in partition table";
		break;
	case BCKP_LABEL:
		str = "Impossible to set label";
		break;
	case BCKP_STACK:
		str = "Stack underflow";
		break;
	case BCKP_CHECK:
		str = "Check error (after CONTEXT/COPY)";
		break;
	case BCKP_INTERNAL:
		str = "Internal error";
		break;
	case BCKP_TYPE:
		str = "Invalid partition type";
		break;
	case BCKP_NOT_FILLED:
		str = "Context parameter is not filled yet";
		break;
	case BCKP_NO_CUR_COMP:
		str = "No current computer";
		break;
	case BCKP_CONSISTENCY:
		str = "Commit impossible because of inconsistency";
		break;
	case BCKP_USER_PROPERTY:
		str = "Error returned from setting user property";
		break;
	case BCKP_WIPE:
		str = "Wipe error";
		break;
	case BCKP_STRID_MORE:
		str = "Strid valid, but more specific";
		break;
	case BCKP_STRID_ROUGH:
		str = "Strid valid, but rough (unaccurate)";
		break;
	case BCKP_STRID_MORE_ROUGH:
		str = "Both previous together";
		break;
	case BCKP_NON_TMP_OP:
		str = "This op is not intended for tmp mode";
		break;
	case BCKP_SYSTEM_RESTORE:
		str = "System restore error";
		break;
	case BCKP_OP_UNKNOWN:
		str = "Split/Merge: Unknown error";
		break;
	case BCKP_OP_NO_MIN_SIZE:
		str = "Split/Merge: Has no minimal free space for operation";
		break;
	case BCKP_OP_LOW_SIZE:
		str = "Split/Merge: Has low free space for run operation";
		break;
	case BCKP_OP_ERR_SPARSE:
		str = "Split/Merge (not usable): Error, sparse not supported now";
		break;
	case BCKP_OP_ERR_SYM_LINK:
		str = "Split/Merge (not usable): Error, symbolic link not supported on this FS";
		break;
	case BCKP_OP_ERR_CRYPTED:
		str = "Split/Merge: Error, can't split if present encrypted files";
		break;
	case BCKP_OP_PREPARE_FAIL:
		str = "Split/Merge: Error, prepare operation fail";
		break;
	case BCKP_LDM_NOT_SUPPORTED:
		str = "ldm deploy. fdisk2 computer never started";
		break;
	case BCKP_LDM_ERROR:
		str = "ldm op error";
		break;
	default:
		str = QString("Undefined error (%1)").arg(code_);
		break;
	}
*/
	return str;
}

namespace Backup
{
///////////////////////////////////////////////////////////////////////////////
// struct Archive
//

Archive::Archive(CVmHardDisk* device_, const QString& name_,
	const QString& image_, const QString& home_):
	m_device(device_), m_name(name_)
{
	QFileInfo f(image_);
	if (!f.isAbsolute())
	{
		m_home = home_;
		f = QFileInfo(QDir(home_), f.fileName());
	}
	m_image = f.absoluteFilePath();
}

QString Archive::getPath(const QString& prefix_) const
{
	return QString("%1/%2").arg(prefix_).arg(m_name);
}

QString Archive::getRestoreFolder() const
{
	if (!m_home.isEmpty())
		return m_image;

	QString s = m_image;
	QFileInfo f(s);
	do
	{
		QString p = f.absolutePath();
		f.setFile(p);
		s = CFileHelper::GetMountPoint(p);
	} while(!f.isRoot() && s.isEmpty());
	s = QString("%1/%2.restore").arg(s).arg(Uuid::createUuid().toString());
	return QFileInfo(QDir(s), f.fileName()).absoluteFilePath();
}

QString Archive::getSnapshotFolder(const QString& prefix_) const
{
	QString x = QFileInfo(m_name).completeBaseName();
	return QString("%1/%2").arg(prefix_).arg(x);
}

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

Perspective::archiveList_type Perspective::getCtArchives(const QString& home_) const
{
	archiveList_type output;
	if (bad())
		return output;

	imageList_type g = getImages();
	if (g.isEmpty())
		return output;

	QString n = g.front()->getUserFriendlyName();
	QString r = n.isEmpty() ? "root.hdd" : n;
	// NB. always name the first tib the old way to preserve compatibility
	// for restore. now all the ploop-based ves archives are private.tib.
	output << Archive(g.front(), PRL_CT_BACKUP_TIB_FILE_NAME, r, home_);
	g.pop_front();
	QStringList w;
	foreach (CVmHardDisk* h, g)
	{
		w << output.last().getName();
		// NB. igor@ said that the friendly name was always
		// an absolute path.
		// NB. he lied.
		n = h->getUserFriendlyName();
		output << Archive(h, getName(n, w), n, home_);
	}
	return output;
}

Perspective::archiveList_type Perspective::getVmArchives(const QString& home_) const
{
	archiveList_type output;
	if (bad())
		return output;

	imageList_type g = getImages();
	if (g.isEmpty())
		return output;

	QStringList w;
	foreach (CVmHardDisk* h, g)
	{
		QString n = h->getSystemName();
		output << Archive(h, getName(n, w), n, home_);
		w << output.last().getName();
	}
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
	output.append(".qcow2");
	if (!(m_flags & PBT_UNCOMPRESSED))
		output.append("c");
	return output;
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

PRL_RESULT Task_BackupHelper::checkFreeDiskSpace(const QString &sVmUuid,
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

		if ( !getForceQuestionsSign() )//If interactive mode then send question to user
		{
			QList<PRL_RESULT> lstChoices;
			lstChoices.append( PET_ANSWER_YES );
			lstChoices.append( PET_ANSWER_NO );

			QList<CVmEventParameter*> lstParams;
			lstParams.append(new CVmEventParameter(PVE::String,
						sVmUuid,
						EVT_PARAM_VM_UUID )
					);
			lstParams.append(new CVmEventParameter(PVE::String, strSize, EVT_PARAM_MESSAGE_PARAM_0));
			lstParams.append(new CVmEventParameter(PVE::String, strFree, EVT_PARAM_MESSAGE_PARAM_1));

			PRL_RESULT nAnswer = getClient()
				->sendQuestionToUser(bIsCreateOp ? PET_QUESTION_BACKUP_CREATE_NOT_ENOUGH_FREE_DISK_SPACE :
								PET_QUESTION_BACKUP_RESTORE_NOT_ENOUGH_FREE_DISK_SPACE
						, lstChoices, lstParams, getRequestPackage() );

			if( nAnswer != PET_ANSWER_YES )
				return PRL_ERR_OPERATION_WAS_CANCELED;
		}
		else
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
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	if (!Uuid::isUuid(sBackupUuid))
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
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

BackupProcess::BackupProcess()
{
	m_pid = -1;
	m_isKilled = false;
	m_in = -1;
	m_out = -1;
}

BackupProcess::~BackupProcess()
{
	if (m_in != -1)
		close(m_in);
	if (m_out != -1)
		close(m_out);

	m_pid = -1;
}

PRL_RESULT BackupProcess::start(const QStringList& arg_, int version_)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	int in[2], out[2];
	int status;
	pid_t pid;
	long flags;

	QStringList a(arg_);
	PRL_ASSERT(arg_.size());
	m_sCmd = arg_.first();
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, in) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() error : %s", strerror(errno));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() error : %s", strerror(errno));
		close(in[0]);
		close(in[1]);
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}

	if (BACKUP_PROTO_V4 > version_) {
		// old syntax, prepend arguments by fds
		a.insert(1, QString::number(in[0]));
		a.insert(2, QString::number(out[1]));
	} else {
		// new syntax, append arguments by named fd args
		a << "--fdin" << QString::number(in[0]);
		a << "--fdout" << QString::number(out[1]);
	}

	char **pArgv = new char *[a.size()+2];
	int i;
	for (i = 0; i < a.size(); ++i)
		pArgv[i] = strdup(QSTR2UTF8(a.at(i)));
	pArgv[i] = NULL;

	if ((flags = fcntl(in[0], F_GETFL)) != -1)
		fcntl(in[0], F_SETFL, flags | O_NONBLOCK);
	if ((flags = fcntl(in[1], F_GETFL)) != -1)
		fcntl(in[1], F_SETFL, flags | O_NONBLOCK);
	if ((flags = fcntl(out[0], F_GETFL)) != -1)
		fcntl(out[0], F_SETFL, flags | O_NONBLOCK);
	if ((flags = fcntl(out[1], F_GETFL)) != -1)
		fcntl(out[1], F_SETFL, flags | O_NONBLOCK);
	m_pid = fork();
	if (m_pid < 0) {
		WRITE_TRACE(DBG_FATAL, "fork() error : %s", strerror(errno));
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
		goto cleanup;
	} else if (m_pid == 0) {
		/* to close all unused descriptors */
		int fdnum;
		struct rlimit rlim;
		if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
			fdnum = (int)rlim.rlim_cur;
		else
			fdnum = 1024;
		for (i = 3; i < fdnum; ++i) {
			if (	(i == in[0]) ||
				(i == out[1]))
				continue;
			close(i);
		}

		close(in[1]); close(out[0]);
		fcntl(in[0], F_SETFD, ~FD_CLOEXEC);
		fcntl(out[1], F_SETFD, ~FD_CLOEXEC);
		execvp(QSTR2UTF8(m_sCmd), (char* const*)pArgv);
		WRITE_TRACE(DBG_FATAL, "Can't exec cmd '%s': %s",
					QSTR2UTF8(m_sCmd), strerror(errno));
		_exit(-1);
	}

	close(in[0]); close(out[1]);
	while ((pid = waitpid(m_pid, &status, WNOHANG)) == -1)
		if (errno != EINTR)
			break;

	if (pid < 0) {
		WRITE_TRACE(DBG_FATAL, "waitpid() error : %s", strerror(errno));
		close(in[1]); close(out[0]);
		if (errno == ECHILD)
			nRetCode = PRL_ERR_BACKUP_TOOL_CANNOT_START;
		else
			nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
		goto cleanup;
	}
	m_in = out[0];
	m_out = in[1];
cleanup:
	for (i = 0; pArgv[i]; ++i)
		free(pArgv[i]);
	delete []pArgv;
	return nRetCode;
}

PRL_RESULT BackupProcess::waitForFinished()
{
	pid_t pid;
	int status;

	while ((pid = waitpid(m_pid, &status, 0)) == -1)
		if (errno != EINTR)
			break;
	m_pid = -1;
	if (pid < 0) {
		WRITE_TRACE(DBG_FATAL, "waitpid() error : %s", strerror(errno));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}

	if (WIFEXITED(status)) {
		if ((m_nRetCode = WEXITSTATUS(status))) {
			WRITE_TRACE(DBG_FATAL, "%s exited with code %d", QSTR2UTF8(m_sCmd), m_nRetCode);
			if (m_nRetCode == 0xff)
				return PRL_ERR_BACKUP_TOOL_CANNOT_START;
			else
				return PRL_ERR_BACKUP_ACRONIS_ERR;
		}
	} else if (WIFSIGNALED(status)) {
		WRITE_TRACE(DBG_FATAL, "%s got signal %d", QSTR2UTF8(m_sCmd), WTERMSIG(status));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	} else {
		WRITE_TRACE(DBG_FATAL, "%s exited with status %d", QSTR2UTF8(m_sCmd), status);
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

void BackupProcess::kill()
{
	//#444144 copy pid to prevent race and call kill(-1, SIGKILL);
	pid_t pid = m_pid;

	if (pid != -1)
		::kill(pid, SIGTERM);
	m_isKilled = true;
}

PRL_RESULT BackupProcess::readFromABackupClient(char *data, qint32 size)
{
	int rc;
	size_t count;

	count = 0;
	/* read data */
	while (1) {
		errno = 0;
		rc = read(m_in, data + count, size - count);
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
		if (errno == EINTR) {
			continue;
		} else {
			WRITE_TRACE(DBG_FATAL, "[%s] read() error : %s", __FUNCTION__, strerror(errno));
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
	}

	/* but we never should be here */
	return PRL_ERR_BACKUP_INTERNAL_ERROR;
}

PRL_RESULT BackupProcess::readFromABackupServer(char *data, qint32 size, UINT32 tmo)
{
	int rc;
	size_t count;
	SmartPtr<fd_set> fds(IOService::allocFDSet(m_in + 1));
	struct timeval tv;
	unsigned nTimeout = tmo/TARGET_DISPATCHER_TIMEOUT_COUNTS; // in sec

	count = 0;
	while (1) {
		/* read data */
		while (1) {
			errno = 0;
			rc = read(m_in, data + count, size - count);
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

		/* wait next data in socket */
		for (int i = 1; i <= TARGET_DISPATCHER_TIMEOUT_COUNTS; i++) {
			if (m_isKilled)
				return PRL_ERR_OPERATION_WAS_CANCELED;
			do {
				::memset(fds.getImpl(), 0, FDNUM2SZ(m_in + 1));
				FD_SET(m_in, fds.getImpl());
				tv.tv_sec = nTimeout;
				tv.tv_usec = 0;
				rc = select(m_in + 1, fds.getImpl(), NULL, NULL, &tv);
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
			} while(!FD_ISSET(m_in, fds.getImpl()));
			if (rc > 0)
				break;
		}
	}

	/* but we never should be here */
	return PRL_ERR_BACKUP_INTERNAL_ERROR;
}

PRL_RESULT BackupProcess::writeToABackupClient(char *data, quint32 size, UINT32 tmo)
{
	int rc;
	size_t count;
	SmartPtr<fd_set> fds(IOService::allocFDSet(m_out + 1));
	struct timeval tv;
	unsigned nTimeout = tmo/TARGET_DISPATCHER_TIMEOUT_COUNTS; // in sec

	if (size == 0)
		return PRL_ERR_SUCCESS;
	count = 0;
	while (1) {
		while (1) {
			errno = 0;
			rc = write(m_out, data + count, (size_t)(size - count));
			if (rc > 0) {
				count += rc;
				if (count >= size)
					return PRL_ERR_SUCCESS;
				continue;
			}
			if ((errno == EAGAIN) || (errno == EINTR)) {
				break;
			} else {
				WRITE_TRACE(DBG_FATAL, "write() error : %s", strerror(errno));
				return PRL_ERR_BACKUP_INTERNAL_ERROR;
			}
		}

		/* wait until socket will ready */
		for (int i = 1; i <= TARGET_DISPATCHER_TIMEOUT_COUNTS; i++) {
			if (m_isKilled)
				return PRL_ERR_OPERATION_WAS_CANCELED;
			do {
				::memset( fds.getImpl(), 0, FDNUM2SZ(m_out + 1) );
				FD_SET(m_out, fds.getImpl());
				tv.tv_sec = nTimeout;
				tv.tv_usec = 0;
				rc = select(m_out + 1, NULL, fds.getImpl(), NULL, &tv);
				if (rc == 0 && i == TARGET_DISPATCHER_TIMEOUT_COUNTS) {
					WRITE_TRACE(DBG_FATAL, "[%s] full timeout expired (%d sec)",
						__FUNCTION__, tmo);
					return PRL_ERR_BACKUP_INTERNAL_ERROR;
				} else if (rc == 0) {
					WRITE_TRACE(DBG_DEBUG, "[%s] partial timeout expired (%d sec)",
						__FUNCTION__, nTimeout*i);
					break;
				} else if (rc <= 0) {
					WRITE_TRACE(DBG_FATAL, "select() error : %s", strerror(errno));
					return PRL_ERR_BACKUP_INTERNAL_ERROR;
				}
			} while (!FD_ISSET(m_out, fds.getImpl()));
			if (rc > 0)
				break;
		}
	}

	/* but we never should be here */
	return PRL_ERR_BACKUP_INTERNAL_ERROR;
}

PRL_RESULT BackupProcess::writeToABackupServer(char *data, quint32 size, UINT32 tmo)
{
	return writeToABackupClient(data, size, tmo);
}

void BackupProcess::setInFdNonBlock()
{
	long flags;

	if ((flags = fcntl(m_in, F_GETFL)) != -1)
		fcntl(m_in, F_SETFL, flags & ~O_NONBLOCK);
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
	if (PRL_FAILED(nRetCode = m_cABackupServer.writeToABackupServer((char *)&uSize, sizeof(uSize), tmo)))
		return nRetCode;
	if (PRL_FAILED(nRetCode = m_cABackupServer.writeToABackupServer(pBuffer.getImpl(), uSize, tmo)))
		return nRetCode;
	/* and read reply */
	if (PRL_FAILED(nRetCode = m_cABackupServer.readFromABackupServer((char *)&nSize, sizeof(nSize), tmo)))
		return nRetCode;
	if (m_nBufSize < nSize) {
		WRITE_TRACE(DBG_FATAL, "Too small read buffer: %ld, requires: %ld", (long)m_nBufSize, (long)nSize);
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	if (PRL_FAILED(nRetCode = m_cABackupServer.readFromABackupServer(m_pBuffer.getImpl(), nSize, tmo)))
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

QString Task_BackupHelper::getAcronisErrorString(int code_)
{
	return Client::getAcronisErrorString(code_);
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

PRL_RESULT Task_BackupHelper::copyEscort(const ::Backup::Escort::Model& escort_,
	const QString& directory_, const QString& source_)
{
	CVmFileListCopySenderClient s(m_pIoClient);
	CVmFileListCopySource c(&s, m_sVmUuid, source_, 0, getLastError(),
			m_nTimeout);

	c.SetRequest(getRequestPackage());
	c.SetVmDirectoryUuid(directory_);
	c.SetProgressNotifySender(Backup::NotifyClientsWithProgress);

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	return c.Copy(escort_.getFolders(), escort_.getFiles());
}

PRL_RESULT Task_BackupHelper::backupHardDiskDevices(const ::Backup::Activity::Object::Model& activity_,
		::Backup::Work::object_type& variant_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	m_product = SmartPtr< ::Backup::Product::Model>(
		new ::Backup::Product::Model(::Backup::Object::Model(m_pVmConfig), m_sVmHomePath));
	m_product->setStore(m_sBackupRootPath);
	if (BACKUP_PROTO_V4 <= m_nRemoteVersion) {
		m_product->setSuffix(::Backup::Suffix(getBackupNumber(), getFlags())());
		return ::Backup::Work::Push::VCommand(*this, activity_).do_(variant_);
	} else
		return ::Backup::Work::Acronis::ACommand(*this, activity_).do_(variant_);
}

/* send start request for remote dispatcher and wait reply from dispatcher */
PRL_RESULT Task_BackupHelper::sendStartRequest()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pBackupCmd;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<IOPackage> pReply;
	quint32 nFlags;
	QString sServerUuid;
	QFileInfo vmBundle(m_sVmHomePath);

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	{
		sServerUuid = CDspService::instance()->getDispConfigGuard().
			getDispConfig()->getVmServerIdentification()->getServerUuid();
	}

	pBackupCmd = CDispToDispProtoSerializer::CreateVmBackupCreateCommand(
			m_sVmUuid,
			m_sVmName,
			QHostInfo::localHostName(),
			sServerUuid,
			m_sDescription,
			m_pVmConfig->toString(),
			m_nOriginalSize,
			(quint32)vmBundle.permissions(),
			m_nFlags,
			getInternalFlags());

	pPackage = DispatcherPackage::createInstance(
			pBackupCmd->GetCommandId(),
			pBackupCmd->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply, m_hJob)))
		return nRetCode;

	if ((pReply->header.type != VmBackupCreateFirstReply) && (pReply->header.type != DispToDispResponseCmd)) {
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x or %x",
			pReply->header.type, DispToDispResponseCmd, VmBackupCreateFirstReply);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	CDispToDispCommandPtr pDspReply = CDispToDispProtoSerializer::ParseCommand(
		(Parallels::IDispToDispCommands)pReply->header.type, UTF8_2QSTR(pReply->buffers[0].getImpl()));

	if (pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pDspReply);

		nRetCode = pResponseCmd->GetRetCode();
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "sendStartRequest response failed: %s ",
				PRL_RESULT_TO_STRING(nRetCode));
			getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
			return nRetCode;
		}
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	CVmBackupCreateFirstReply *pCreateReply =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupCreateFirstReply>(pDspReply);

	m_nRemoteVersion = pCreateReply->GetVersion();
	m_sBackupUuid = pCreateReply->GetBackupUuid();
	m_nBackupNumber = pCreateReply->GetBackupNumber();
	m_sBackupRootPath = pCreateReply->GetBackupRootPath();
	nFlags = pCreateReply->GetFlags();
	quint64 nFreeDiskSpace;

	if (pCreateReply->GetFreeDiskSpace(nFreeDiskSpace))
	{
		nRetCode = checkFreeDiskSpace(m_sVmUuid, m_nOriginalSize, nFreeDiskSpace, true);
		if (PRL_FAILED(nRetCode))
			return nRetCode;
	}

	if ((m_nFlags & PBT_INCREMENTAL) && (nFlags & PBT_FULL)) {
		CVmEvent event(PET_DSP_EVT_VM_MESSAGE, m_sVmUuid, PIE_DISPATCHER, PRL_WARN_BACKUP_HAS_NOT_FULL_BACKUP);
		SmartPtr<IOPackage> pPackage =
			DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		getClient()->sendPackage(pPackage);
	}
	m_nFlags = nFlags;

	if (BACKUP_PROTO_V4 <= m_nRemoteVersion) {
		for (unsigned i = 1; i < pReply->header.buffersNumber; i += 2) {
			QFileInfo x(UTF8_2QSTR(pReply->buffers[i].getImpl()));
			QString u(UTF8_2QSTR(pReply->buffers[i+1].getImpl()));
			m_urls.push_back(::Backup::Activity::Object::component_type(x, u));
		}
	}

	return PRL_ERR_SUCCESS;
}

// cancel command
void Task_BackupHelper::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	killABackupClient();
	SmartPtr<IOClient> x = getIoClient();
	if (x.isValid())
	{
		x->urgentResponseWakeUp(m_hJob);
		x->disconnectClient();
	}
}

PRL_RESULT Task_BackupHelper::doBackup(const QString& source_, ::Backup::Work::object_type& variant_)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> p;

	::Backup::Activity::Object::Model a;
	nRetCode = m_service->find(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), a);
	if (PRL_FAILED(nRetCode))
		goto exit;

	if (PRL_FAILED(nRetCode = sendStartRequest()))
		goto exit;

	/* part one : plain copy of config files */
	nRetCode = copyEscort(a.getEscort(), m_sVmDirUuid, source_);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	nRetCode = backupHardDiskDevices(a, variant_);
	/*
	   Now target side wait new acronis proxy commands due to acronis have not call to close connection.
	   To fix it will send command to close connection from here.
	   Pay attention: on success and on failure both we will wait reply from target.
	   */
	if (PRL_FAILED(nRetCode)) {
		p = IOPackage::createInstance(ABackupProxyCancelCmd, 0);
		WRITE_TRACE(DBG_FATAL, "send ABackupProxyCancelCmd command");
		SendPkg(p);
	} else {
		p = IOPackage::createInstance(ABackupProxyFinishCmd, 0);
		nRetCode = SendPkg(p);
		// TODO:	nRetCode = SendReqAndWaitReply(p);
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_BackupHelper::finalizeTask()
{
	if (m_pVmConfig.isValid())
		m_service->finish(MakeVmIdent(m_sVmUuid, m_sVmDirUuid), getClient());

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		CVmEvent event(PET_DSP_EVT_CREATE_BACKUP_FINISHED, m_sVmUuid, PIE_DISPATCHER);
		event.addEventParameter(
			new CVmEventParameter( PVE::String,
					(m_nFlags&PBT_FULL) ?
					m_sBackupUuid :
					QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber),
					EVT_PARAM_BACKUP_CMD_BACKUP_UUID) );
		event.addEventParameter( new CVmEventParameter( PVE::String,
					m_sDescription,
					EVT_PARAM_BACKUP_CMD_DESCRIPTION ) );
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage,
				m_sVmDirUuid, m_sVmUuid);

		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		CProtoCommandDspWsResponse *pDspWsResponseCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		pDspWsResponseCmd->AddStandardParam(m_sVmUuid);
		pDspWsResponseCmd->AddStandardParam(
			(m_nFlags&PBT_FULL) ? m_sBackupUuid : QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber));
		getClient()->sendResponse(pResponse, getRequestPackage());
	} else {
		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
}
