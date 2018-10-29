///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackupTarget.cpp
///
/// Target task for Vm backup restoring
///
/// @author krasnov@
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

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <QProcess>
#include "CDspVmBrand.h"
#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include "prlcommon/Interfaces/ApiDevNums.h"
#include "Task_RestoreVmBackup_p.h"
#include "prlcommon/Logging/Logging.h"
#include "prlcommon/PrlUuid/Uuid.h"
#include "CDspClientManager.h"
#include "Task_RestoreVmBackup.h"
#include "Task_BackupHelper_p.h"
#include "Tasks/Task_RegisterVm.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "CDspVmNetworkHelper.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "Libraries/Virtuozzo/CVzPloop.h"
#include "Libraries/PrlNetworking/netconfig.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"
#include "Legacy/VmConverter.h"

namespace Restore
{
///////////////////////////////////////////////////////////////////////////////
// struct Move

Move::Move(const QString& from_, const QString& to_, CAuthHelper& auth_):
	m_from(from_), m_to(to_), m_trash(), m_toolkit(auth_)
{
	m_trash = &m_from;
	if (m_from.isEmpty())
		return;

	m_revert = QString(to_).append(".restore");
	m_toolkit.unlink(m_revert);
	m_toolkit.rename(to_, m_revert);
}

Move::~Move()
{
	if (NULL == m_trash)
		return;
	if (!done())
	{
		if (m_from.isEmpty())
			m_toolkit.unlink(m_to);
		else
		{
			m_toolkit.unlink(m_from);
			if (!m_revert.isEmpty())
				m_toolkit.rename(m_revert, m_to);
		}
	}
	else if (!commit())
		m_toolkit.unlink(*m_trash);
}

bool Move::do_()
{
	if (done())
		return true;

	if (m_from.isEmpty())
		m_trash = &m_to;
	else if (PRL_SUCCEEDED(m_toolkit.rename(m_from, m_to)))
		m_trash = &m_revert;
	else
		return false;

	return true;
}

bool Move::commit()
{
	if (!done())
		return false;

	if (!m_from.isEmpty())
	{
		if (m_revert.isEmpty() || PRL_FAILED(m_toolkit.unlink(m_revert)))
			return false;

		m_revert.clear();
	}
	m_trash = NULL;
	return true;
}

bool Move::revert()
{
	if (!done())
		return false;

	if (m_from.isEmpty())
	{
		if (NULL == m_trash || PRL_FAILED(m_toolkit.unlink(*m_trash)))
			return false;
	}
	else if (m_revert.isEmpty())
		return false;
	else if (PRL_SUCCEEDED(m_toolkit.unlink(m_to)) &&
		PRL_SUCCEEDED(m_toolkit.rename(m_revert, m_to)))
		m_revert.clear();
	else
		return false;

	m_trash = NULL;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct Assembly

Assembly::~Assembly()
{
	if (m_pending.isEmpty())
	{
		foreach (Move* m, m_ready)
		{
			m->commit();
			delete m;
		}
	}
	else
		revert();

	Toolkit k(*m_auth);
	foreach(const QString& x, m_trash)
	{
		k.unlink(x);
	}
}

PRL_RESULT Assembly::do_()
{
	while(!m_pending.isEmpty())
	{
		Move* m = m_pending.front();
		if (!m->do_())
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;

		m_pending.pop_front();
		m_ready << m;
	}
	return PRL_ERR_SUCCESS;
}

void Assembly::revert()
{
	foreach (Move* m, m_ready)
	{
		m->revert();
		delete m;
	}
	m_ready.clear();
	foreach (Move* m, m_pending)
	{
		delete m;
	}
	m_pending.clear();
}

void Assembly::addExternal(const QFileInfo& src_, const QString& dst_)
{
	Toolkit k(*m_auth);
	k.mkdir(QFileInfo(dst_).absolutePath());
	m_trash << src_.absolutePath();
	m_pending << new Move(src_.absoluteFilePath(), dst_, *m_auth);
}

void Assembly::addEssential(const QString& src_, const QString& dst_)
{
	QString x;
	if (QFileInfo(src_) == QFileInfo(dst_))
	{
		// NB. don't need a temporary folder to restore a new vm.
		// we use the vm private directly. don't have to delete
		// the target folder if the task finishes successfully.
		// indicate this with the empty string sentinel.
	}
	else
		x = src_;

	m_pending << new Move(x, dst_, *m_auth);
}


namespace AClient
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(Backup::AClient& impl_, const QString& uuid_, CVmConfiguration& vm_):
	m_name(vm_.getVmIdentification()->getVmName()), m_uuid(uuid_),
	m_impl(&impl_)
{
}

PRL_RESULT Unit::operator()(const QStringList& argv_, unsigned disk_) const
{
	return m_impl->startABackupClient(m_name, argv_, m_uuid, disk_);
}


PRL_RESULT Unit::operator()(const QStringList& argv_, SmartPtr<Chain> custom_) const
{
	return m_impl->startABackupClient(m_name, argv_, custom_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Api

Api::Api(quint32 no_, const QString& backupRoot_):
	m_no(no_), m_backupRoot(backupRoot_)
{
}

QStringList Api::restore(const QString& home_, quint32 veid_) const
{
	return restore(QFileInfo(QDir(home_), "root.efd")) << QString::number(veid_);
}

QStringList Api::restore(const QFileInfo& target_) const
{
	QFileInfo a(m_backupRoot, PRL_CT_BACKUP_TIB_FILE_NAME);
	return restore(a.absoluteFilePath(), target_);
}

QStringList Api::restore(const QString& archive_, const QFileInfo& target_) const
{
	return QStringList() << "restore_ct" << target_.absoluteFilePath()
			<< m_backupRoot.absolutePath() << archive_
			<< QString::number(m_no);
}

QStringList Api::query(const Backup::Product::component_type& archive_) const
{
	return QStringList() << "query_archive" << ""
			<< m_backupRoot.absolutePath()
			<< m_backupRoot.absoluteFilePath
				(QFileInfo(archive_.second.toLocalFile()).fileName())
			<< QString::number(m_no);
}

QStringList Api::restore(const Backup::Product::component_type& archive_,
		const QFileInfo& target_) const
{
	return restore(m_backupRoot.absoluteFilePath
		(QFileInfo(archive_.second.toLocalFile()).fileName()), target_);
}

} // namespace AClient

///////////////////////////////////////////////////////////////////////////////
// struct Toolkit

Toolkit::Toolkit(const SmartPtr<CDspClient>& user_): m_auth()
{
	if (user_.isValid())
		m_auth = &user_->getAuthHelper();
}

PRL_RESULT Toolkit::chown(const QString& path_) const
{
	if (CDspAccessManager::setOwner(path_, m_auth, true))
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Cannot set owner of %s", QSTR2UTF8(path_));
	return PRL_ERR_CANT_CHANGE_OWNER_OF_FILE;
}

PRL_RESULT Toolkit::mkdir(const QString& path_) const
{
	if (CFileHelper::WriteDirectory(path_, m_auth))
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Cannot create directory %s", QSTR2UTF8(path_));
	return PRL_ERR_MAKE_DIRECTORY;
}

PRL_RESULT Toolkit::unlink(const QFileInfo& path_) const
{
	QString p = path_.filePath();
	if (path_.isDir())
	{
		if (!CFileHelper::ClearAndDeleteDir(p))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot remove directory %s", QSTR2UTF8(p));
			return PRL_ERR_CANT_REMOVE_ENTRY;
		}
	}
	else if (path_.exists())
	{
		if (!CFileHelper::RemoveEntry(p, m_auth))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot remove a non-directory item %s", QSTR2UTF8(p));
			return PRL_ERR_CANT_REMOVE_ENTRY;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Toolkit::rename(const QString& from_, const QString& to_) const
{
#ifdef _WIN_
	if (CFileHelper::RenameEntry(from_, to_, m_auth))
#else
	if (0 == QProcess::execute("mv", QStringList() << from_ << to_))
#endif
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Cannot rename %s -> %s", QSTR2UTF8(from_), QSTR2UTF8(to_));
	return PRL_ERR_CANT_RENAME_ENTRY;
}

bool Toolkit::folderExists(const QString& path_) const
{
	return CFileHelper::DirectoryExists(path_, m_auth);
}

///////////////////////////////////////////////////////////////////////////////
// struct Program

Program::result_type Program::execute(const QStringList& argv_, CDspTaskHelper& task_)
{
	if (argv_.isEmpty())
		return PRL_ERR_INVALID_ARG;

	QProcess q;
	QString j = argv_.join(" ");
	Program p(q, j, task_);
	WRITE_TRACE(DBG_FATAL, "Run cmd: %s", qPrintable(j));
	return HostUtils::RunCmdLineUtilityEx(argv_, q, QUANTUM)(p).getResult();
}

void Program::crashed()
{
	ExecHandlerBase::crashed();
	m_result = PRL_ERR_FAILURE;
}

void Program::waitFailed()
{
	forever
	{
		if (m_task->operationIsCancelled())
		{
			m_process->kill();
			m_process->waitForFinished();
			m_result = m_task->getCancelResult();
			return;
		}
		if (m_process->waitForFinished(QUANTUM))
		{
			if (QProcess::CrashExit == m_process->exitStatus())
				crashed();
			else
				exitCode(m_process->exitCode());
			return;
		}
	}
}

void Program::exitCode(int value_)
{
	ExecHandlerBase::exitCode(value_);
	if (0 == value_)
		m_result = UTF8_2QSTR(m_process->readAllStandardOutput());
	else
		m_result = PRL_ERR_OPERATION_FAILED;
}

Program::Program(QProcess& process_, const QString& name_, CDspTaskHelper& task_):
	ExecHandlerBase(process_, name_), m_task(&task_), m_result(PRL_ERR_UNINITIALIZED)
{
}

///////////////////////////////////////////////////////////////////////////////
// struct Assistant

Assistant::Assistant(Task_RestoreVmBackupTarget& task_, const AClient::Unit& unit_):
	m_unit(unit_), m_task(&task_)
{
}

CVmEvent* Assistant::event() const
{
	return m_task->getLastError();
}

PRL_RESULT Assistant::make(const QString& path_, bool failIfExists_) const
{
	Toolkit k = getToolkit();
	if (k.folderExists(path_))
	{
		if (failIfExists_)
		{
			CVmEvent* v = event();
			v->setEventCode(PRL_ERR_BACKUP_RESTORE_DIRECTORY_ALREADY_EXIST);
			v->addEventParameter(new CVmEventParameter(PVE::String, path_, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "[%s] Directory \"%s\" already exist", __FUNCTION__, QSTR2UTF8(path_));
		}
		return PRL_ERR_BACKUP_RESTORE_DIRECTORY_ALREADY_EXIST;
	}
	else if (PRL_FAILED(k.mkdir(path_)))
	{
		CVmEvent* v = event();
		v->setEventCode(PRL_ERR_BACKUP_RESTORE_CANNOT_CREATE_DIRECTORY);
		v->addEventParameter(new CVmEventParameter(PVE::String, path_, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory", __FUNCTION__, QSTR2UTF8(path_));
		return PRL_ERR_BACKUP_RESTORE_CANNOT_CREATE_DIRECTORY;
	}
	return PRL_ERR_SUCCESS;
}

Toolkit Assistant::getToolkit() const
{
	return Toolkit(m_task->getClient());
}

Assembly* Assistant::getAssembly() const
{
	return new Assembly(m_task->getClient()->getAuthHelper());
}

PRL_RESULT Assistant::operator()(const QStringList& argv_, unsigned disk_) const
{
	return m_unit(argv_, disk_);
}

PRL_RESULT Assistant::operator()(const QStringList& argv_, SmartPtr<Chain> custom_) const
{
	return m_unit(argv_, custom_);
}

PRL_RESULT Assistant::operator()(const QString& image_, const QString& archive_,
			const QString& format_) const
{
	Prl::Expected<QString, PRL_RESULT> e = m_task->sendMountImageRequest(archive_);
	if (e.isFailed())
		return e.error();

	QString u = e.value();;
	::Backup::Tunnel::Source::Factory::result_type t =
		m_task->craftTunnel()(m_task->getFlags());
	
	if (t.isFailed())
		return t.error();
	else if (t.value().isNull())
		u = m_task->patch(u);
	else
	{
		Prl::Expected<QUrl, PRL_RESULT> x = t.value()->addStrand(u);
		if (x.isFailed())
			return x.error();

		u = x.value().toString();
	}
	QStringList cmdline = QStringList() << QEMU_IMG << "convert" << "-O" << format_
			<< "-S" << "64k" << "-t" << "none";
	if (format_ == "qcow2")
		cmdline << "-o" << "cluster_size=1M,lazy_refcounts=on"; 
	cmdline << u << image_;

	Program::result_type q = Program::execute(cmdline, *m_task);
	if (q.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot restore hdd %s", qPrintable(image_));
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}

	WRITE_TRACE(DBG_DEBUG, "qemu-img output:\n%s", qPrintable(q.value()));
	return PRL_ERR_SUCCESS;
}

namespace Query
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

Handler::Handler(SmartPtr<IOClient> io_, quint32 timeout_): m_size(0ULL), m_usage(0ULL)
{
	next(SmartPtr<Chain>(new Forward(io_, timeout_)));
}

PRL_RESULT Handler::do_(SmartPtr<IOPackage> request_, process_type& dst_)
{
	if (ABackupQueryArchiveReply != request_->header.type)
		return forward(request_, dst_);

	bool ok;
	QString b;
	if (0 < request_->header.buffersNumber)
		b = request_->buffers[0].getImpl();

	WRITE_TRACE(DBG_DEBUG, "QueryArchive: size=\"%s\"", QSTR2UTF8(b));
	m_size = b.toULongLong(&(ok = false));
	if (!ok)
		WRITE_TRACE(DBG_FATAL, "QueryArchive: invalid size format %s", QSTR2UTF8(b));

	if (1 < request_->header.buffersNumber)
		b = request_->buffers[1].getImpl();

	WRITE_TRACE(DBG_DEBUG, "QueryArchive: usage=\"%s\"", QSTR2UTF8(b));
	m_usage = b.toULongLong(&(ok = false));
	if (!ok)
		WRITE_TRACE(DBG_FATAL, "QueryArchive: invalid usage format %s", QSTR2UTF8(b));

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Work

Work::Work(const AClient::Api& api_, SmartPtr<IOClient> io_, quint32 timeout_):
	m_timeout(timeout_), m_api(api_), m_io(io_)
{
}

PRL_RESULT Work::operator()(const Backup::Product::component_type& archive_,
		const Assistant& assist_, quint64& dst_) const
{
	Handler* h = new Handler(m_io, m_timeout);
	SmartPtr<Chain> g(h);
	PRL_RESULT e = assist_(m_api.query(archive_), g);
	if (PRL_FAILED(e))
		return e;

	dst_ = h->size();
	return PRL_ERR_SUCCESS;
}

} // namespace Query

namespace Target
{
///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(quint32 no_, const QString& home_, const QString& backupRoot_, const Assistant& assist_):
	m_no(no_), m_home(home_), m_backupRoot(backupRoot_), m_assist(assist_)
{
}

Vm::~Vm()
{
	std::for_each(m_auto.begin(), m_auto.end(), &CFileHelper::ClearAndDeleteDir);
}

PRL_RESULT Vm::add(const ::Backup::Product::component_type& component_)
{
	const ::Backup::Object::Component& c = component_.first;
	if (0 < m_hddMap.count(c.getDevice().getIndex()))
		return PRL_ERR_SUCCESS;

	QString y = c.getRestorePath();
	QFileInfo f(y);
	PRL_RESULT r = make(f.absolutePath());
	if (PRL_FAILED(r))
		return r;
	Hdd d;
	d.tib = component_.second.toLocalFile();
	QString x = c.getImage();
	if (x != y)
		d.final = x;

	d.mountPoint = CFileHelper::GetMountPoint(f.absolutePath());
	d.sizeOnDisk = c.getDevice().getSizeOnDisk() << 20;
	d.intermediate = f;
	d.format = (c.getDevice().getDiskType() == PHD_EXPANDING_HARD_DISK) ? "qcow2" : "raw";
	m_hddMap.insert(std::make_pair(c.getDevice().getIndex(), d));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::createHome()
{
	return make(m_home);
}

PRL_RESULT Vm::make(const QString& path_)
{
	if (m_auto.contains(path_))
		return PRL_ERR_SUCCESS;

	bool x = m_home == path_;
	PRL_RESULT output = m_assist.make(path_, x);
	if (PRL_SUCCEEDED(output))
		m_auto.insert(path_);
	else if (!x && output == PRL_ERR_BACKUP_RESTORE_DIRECTORY_ALREADY_EXIST)
		output = PRL_ERR_SUCCESS;

	return output;
}

bool Vm::isNoSpace(noSpace_type& dst_) const
{
	typedef std::map<QString, noSpace_type> mountMap_type;
	mountMap_type m;
	hddMap_type::const_iterator i, e = m_hddMap.end();
	for (i = m_hddMap.begin(); i != e; ++i)
	{
		QString p = i->second.mountPoint;
		if (0 < m.count(p))
			continue;

		quint64 z = 0;
		PRL_RESULT y = CFileHelper::GetDiskAvailableSpace(p, &z);
		if (PRL_FAILED(y))
			continue;
		m[p] = noSpace_type(z, 0);
	}
	for (i = m_hddMap.begin(); i != e; ++i)
	{
		QString p = i->second.mountPoint;
		mountMap_type::iterator y = m.find(p);
		if (m.end() == y)
			continue;

		y->second.second += i->second.sizeOnDisk;
	}
	mountMap_type::const_iterator a, b = m.end();
	for (a = m.begin(); a != b; ++a)
	{
		if (a->second.second > a->second.first)
		{
			dst_ = a->second;
			return true;
		}
	}
	return false;
}

PRL_RESULT Vm::restoreA(const hddMap_type::const_iterator& hdd_) const
{
	QStringList z;
	z << "restore" << hdd_->second.intermediate.absoluteFilePath()
		<< m_backupRoot << hdd_->second.tib.absoluteFilePath()
		<< QString::number(m_no);
	return m_assist(z, hdd_->first);
}

PRL_RESULT Vm::restoreV(const hddMap_type::const_iterator& hdd_) const
{
	return m_assist(hdd_->second.intermediate.absoluteFilePath(),
		hdd_->second.tib.absoluteFilePath(), hdd_->second.format);
}

PRL_RESULT Vm::restore(quint32 version_) const
{
	QStringList z;
	hddMap_type::const_iterator p, e = m_hddMap.end();
	for (p = m_hddMap.begin(); p != e; ++p)
	{
		PRL_RESULT r = (BACKUP_PROTO_V4 > version_) ?
			restoreA(p) : restoreV(p);
		if (PRL_FAILED(r))
			return r;
	}
	return PRL_ERR_SUCCESS;
}

Restore::Assembly* Vm::assemble(const QString& dst_)
{
	if (PRL_FAILED(m_assist.getToolkit().chown(m_home)))
		return NULL;

	hddMap_type::const_iterator i, e = m_hddMap.end();
	std::auto_ptr<Restore::Assembly> output(m_assist.getAssembly());
	for (i = m_hddMap.begin(); i != e; ++i)
	{
		if (i->second.final.isEmpty())
			continue;

		const QFileInfo& x = i->second.intermediate;
		output->addExternal(x, i->second.final);
		m_auto.remove(x.absolutePath());
	}
	if (m_auto.contains(m_home))
	{
		output->addEssential(m_home, dst_);
		m_auto.remove(m_home);
	}
	return output.release();
}

///////////////////////////////////////////////////////////////////////////////
// struct Ct

template<class F>
Restore::Assembly* Ct::operator()(F flavor_, const QString& home_, quint32 version_)
{
	Restore::Assembly* output = NULL;
	if (PRL_FAILED(m_result = flavor_.restore(*m_assist, version_)))
	{
		WRITE_TRACE(DBG_FATAL, "send ABackupProxyCancelCmd command");
		send(ABackupProxyCancelCmd);
	}
	else if (PRL_SUCCEEDED(m_result = send(ABackupProxyFinishCmd)))
	{
		std::auto_ptr<Restore::Assembly> x(m_assist->getAssembly());
		if (PRL_SUCCEEDED(m_result = flavor_.assemble(home_, *x)))
			output = x.release();
	}
	return output;
}

namespace Ploop
{
///////////////////////////////////////////////////////////////////////////////
// struct Device

Device::Device(const QString& path_): m_path(path_)
{
}

SmartPtr<Device> Device::make(const QString& path_, quint64 sizeBytes_)
{
	if (PRL_FAILED(CVzOperationHelper().create_disk_image(path_, sizeBytes_)))
		return SmartPtr<Device>();
	return SmartPtr<Device>(new Device(path_));
}

PRL_RESULT Device::mount()
{
	return mount(QString());
}

PRL_RESULT Device::mount(const QString& mountPoint_)
{
	return CVzOperationHelper().mount_disk_image(m_path, mountPoint_, m_name);
}

PRL_RESULT Device::umount()
{
	if (m_name.isEmpty())
		return PRL_ERR_SUCCESS;

	if (CVzOperationHelper().umount_snapshot(m_name) != 0)
		return PRL_ERR_FAILURE;

	m_name.clear();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::setEncryption(const CVmHddEncryption *encryption_)
{
	if (!encryption_ || encryption_->getKeyId().isEmpty())
		return PRL_ERR_SUCCESS;

	PloopImage::Image d(m_path);
	return d.setEncryptionKeyid(encryption_->getKeyId());
}

///////////////////////////////////////////////////////////////////////////////
// struct Image

Image::Image(const Backup::Product::component_type& component_, const Query::Work& query_):
	m_final(component_.first.getImage()), m_intermediate(component_.first.getRestorePath()),
	m_query(query_), m_archive(component_)
{
}

Image::~Image()
{
	if (!m_auto.isEmpty())
		CFileHelper::ClearAndDeleteDir(m_auto);
}

void Image::join(Restore::Assembly& dst_)
{
	if (m_final != m_intermediate)
	{
		dst_.addExternal(QFileInfo(m_intermediate), m_final);
		m_auto.clear();
	}
}

PRL_RESULT Image::do_(const Assistant& assist_, quint32 version_)
{
	QString a = QFileInfo(m_intermediate).absolutePath();
	// NB. assume that m_sTargetPath is handled by somebody outside.
	if (m_final != m_intermediate)
	{
		PRL_RESULT e = assist_.make(a, false);
		if (PRL_SUCCEEDED(e))
			m_auto = a;
		else if (PRL_ERR_BACKUP_RESTORE_DIRECTORY_ALREADY_EXIST != e)
			return e;
	}

	PRL_RESULT output;
	quint64 s = 0;
	if (BACKUP_PROTO_V3 < version_)
		s = m_archive.first.getDeviceSizeInBytes();
	else
	{
		if (PRL_FAILED(output = m_query(m_archive, assist_, s)))
			return output;
	}

	SmartPtr<Device> device(Device::make(m_intermediate, s));
	if (!device.isValid())
		return PRL_ERR_VM_CREATE_HDD_IMG_INVALID_CREATE;

	if (PRL_FAILED(device->mount()))
		return PRL_ERR_DISK_MOUNT_FAILED;

	if (BACKUP_PROTO_V3 < version_) {
		// we specify 'raw' format, because target is a mounted ploop device
		output = assist_(device->getName(), m_archive.second.toLocalFile(), "raw");
	} else {
		output = assist_(m_query.getApi().restore(m_archive,
			QFileInfo(device->getName())), m_archive.first.getDevice().getIndex());
	}
	device->umount();
	if (PRL_SUCCEEDED(output))
		output = device->setEncryption(m_archive.first.getDevice().getEncryption());
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

Flavor::Flavor(const QString& home_, const Backup::Product::componentList_type& ve_,
	const Query::Work& query_): m_home(home_)
{
	foreach (Backup::Product::component_type a, ve_)
	{
		m_imageList << Image(a, query_);
	}
}

PRL_RESULT Flavor::assemble(const QString& home_, Restore::Assembly& dst_)
{
	dst_.addEssential(m_home, home_);
	QList<Image>::iterator p = m_imageList.begin(), e = m_imageList.end();
	for (;p != e; ++p)
	{
		p->join(dst_);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Flavor::restore(const Assistant& assist_, quint32 version_)
{
	if (0 != CVzOperationHelper().create_env_private(m_home))
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;

	QList<Image>::iterator p = m_imageList.begin(), e = m_imageList.end();
	for (;p != e; ++p)
	{
		PRL_RESULT e = p->do_(assist_, version_);
		if (PRL_FAILED(e))
			return e;
	}
	return PRL_ERR_SUCCESS;
}
} // namespace Ploop
} // namespace Target
} // namespace Restore

static void NotifyClientsWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sVmDirectoryUuid,
		const QString &sVmUuid,
		int nPercents)
{
	CVmEvent event(PET_DSP_EVT_RESTORE_PROGRESS_CHANGED, sVmUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);

	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirectoryUuid, sVmUuid);
}

/* send notification for initiator only if Vm does not exist (#431127) */
static void NotifyInitiatorWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sVmDirectoryUuid,
		const QString &sHandle,
		int nPercents)
{
	Q_UNUSED(sVmDirectoryUuid);
	CVmEvent event(PET_DSP_EVT_RESTORE_PROGRESS_CHANGED, sHandle, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);

	CDspService::instance()->getIOServer().sendPackage(sHandle, pPackage);
}

/*******************************************************************************

 Backup restore task for client

********************************************************************************/
Task_RestoreVmBackupTarget::Task_RestoreVmBackupTarget(
		Registry::Public& registry_,
		SmartPtr<CDspClient> &client,
		CProtoCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
:Task_BackupHelper(client, p),
m_registry(registry_),
m_bVmExist(false),
m_nCurrentVmUptime(0)
{
	CProtoRestoreVmBackupCommand *pCmd =
		CProtoSerializer::CastToProtoCommand<CProtoRestoreVmBackupCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sOriginVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_sServerHostname = pCmd->GetServerHostname();
	if (!pCmd->GetTargetVmHomePath().isEmpty())
		m_sTargetVmHomePath = QDir(pCmd->GetTargetVmHomePath()).absolutePath();
	m_sTargetVmName = pCmd->GetTargetVmName();
//	m_sTargetStorageId = pCmd->GetTargetStorageId();
	m_nServerPort = pCmd->GetServerPort();
	m_sServerSessionUuid = pCmd->GetServerSessionUuid();
	m_nFlags = pCmd->GetFlags();
	m_nOriginalSize = 0;
}

Task_RestoreVmBackupTarget::~Task_RestoreVmBackupTarget()
{
	// #439777 to protect call handler for destroying object
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_RestoreVmBackupTarget::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (m_sTargetStorageId.size()) {
		WRITE_TRACE(DBG_FATAL, "iSCSI storage support does not implemented");
		nRetCode = PRL_ERR_UNIMPLEMENTED;
	}

	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	if (PRL_FAILED(nRetCode = sendStartRequest()))
		goto exit;

	if (m_nFlags & PBT_RESTORE_TO_COPY)
	{
		if (m_sVmUuid == m_sOriginVmUuid || m_sVmUuid.isEmpty())
			m_sVmUuid = Uuid::createUuid().toString();
	}
	else if (m_sVmUuid.isEmpty())
		m_sVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();

	if (m_sVmUuid.isEmpty()) {
		nRetCode = CDspTaskFailure(*this)
			(PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND, m_sBackupUuid);
		WRITE_TRACE(DBG_FATAL, "Can't get Vm uuid for backup uuid %s", QSTR2UTF8(m_sBackupUuid));
		goto exit;
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (m_nInternalFlags & PVM_CT_BACKUP)
		nRetCode = restoreCt();
	else
		nRetCode = restoreVm();

	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* Finalize task */
void Task_RestoreVmBackupTarget::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
	SmartPtr<IOPackage> pPackage;

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
		/* send finish event to existing vm, https://jira.sw.ru/browse/PSBM-6694 */
		if ( m_bVmExist ) {
			CVmEvent event(PET_DSP_EVT_RESTORE_BACKUP_FINISHED, m_sVmUuid, PIE_DISPATCHER);
			pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
			CDspService::instance()->getClientManager().sendPackageToVmClients(
					pPackage, getClient()->getVmDirectoryUuid(), m_sVmUuid);
		} else {
			CVmEvent event(
				PET_DSP_EVT_RESTORE_BACKUP_FINISHED, m_sOriginVmUuid, PIE_DISPATCHER);
			pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
			CDspService::instance()->getIOServer().sendPackage(getClient()->getClientHandle(), pPackage);
		}

		CVmEvent event2(PET_DSP_EVT_VM_CONFIG_CHANGED, m_sVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event2, getRequestPackage());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage,
				getClient()->getVmDirectoryUuid(), m_sVmUuid);

		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		CProtoCommandDspWsResponse *pDspWsResponseCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		pDspWsResponseCmd->AddStandardParam(m_sVmUuid);
		if (m_nBackupNumber == PRL_BASE_BACKUP_NUMBER)
			pDspWsResponseCmd->AddStandardParam(m_sBackupUuid);
		else
			pDspWsResponseCmd->AddStandardParam(QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber));

		getClient()->sendResponse(pResponse, getRequestPackage());
	} else {
		if (m_sVzCacheDir.size())
			CFileHelper::ClearAndDeleteDir(m_sVzCacheDir);
		getClient()->sendResponseError(getLastError(), getRequestPackage());
	}
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	/* rewrote ServerUuid in Vm config to avoid Task_RegisterVm failure during restore on alien server (#435666) */
	QString sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()->
					getVmServerIdentification()->getServerUuid();
	m_pVmConfig->getVmIdentification()->setServerUuid(sServerUuid);

	/* after first request - for unspecified Vm uuid case */
	nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
		m_sVmUuid, getClient()->getVmDirectoryUuid(), PVE::DspCmdRestoreVmBackup, getClient(),
		this->getJobUuid());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "registerExclusiveVmOperation failer. Reason: %#x (%s)",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}

	m_bVmExist = false;
	/* flag PBT_RESTORE_TO_COPY - create new CT and ignore existing CT */
	if (!(m_nFlags & PBT_RESTORE_TO_COPY)) {
		// #443839 in brackets to prevent deadlock with CDspVm::GetVmInstanceByUuid() mutexes
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem
			= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getClient()->getVmDirectoryUuid(), m_sVmUuid);
		if ( pVmDirItem )
			m_bVmExist = true;
	}

	if ( m_bVmExist ) {
		nRetCode = restoreVmOverExisting();
	} else {
		nRetCode = restoreNewVm();
	}

	CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
		m_sVmUuid, getClient()->getVmDirectoryUuid(), PVE::DspCmdRestoreVmBackup, getClient());

	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreVmOverExisting()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	PRL_RESULT code;
	CDispToDispCommandPtr pCmd;
	SmartPtr<IOPackage> pPackage;
	QString sVmName;
	QString sPathToVmConfig;
	QString sTmpPath, sVzCacheTmpPath;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	/* name, bundle : use values of existing CT, ignore m_sTargetVmHomePath & m_sTargetVmName from command
	   and values from backuped Vm config */

	/* do not change bundle of existing Vm */
	if (m_sTargetVmHomePath.size()) {
		WRITE_TRACE(DBG_FATAL, "It is't possible to set target private for restore existing VM");
		return PRL_ERR_BACKUP_RESTORE_EXISTING_SET_PRIVATE;
	}

	{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem
			= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getClient()->getVmDirectoryUuid(), m_sVmUuid);
		if ( !pVmDirItem ) {
			WRITE_TRACE(DBG_FATAL, "[%s] CDspVmDirManager::getVmDirItemByUuid(%s, %s) error",
				__FUNCTION__, QSTR2UTF8(getClient()->getVmDirectoryUuid()), QSTR2UTF8(m_sVmUuid));
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		}
		sPathToVmConfig = pVmDirItem->getVmHome();
		m_sTargetVmHomePath = CFileHelper::GetFileRoot(pVmDirItem->getVmHome());
		sVmName = pVmDirItem->getVmName();
	}

	/* do not change name of existing Vm - but PMC always send name */
	if (m_sTargetVmName.size() && (m_sTargetVmName != sVmName)) {
		WRITE_TRACE(DBG_FATAL, "It is't possible to set name for restore existing VM");
		return PRL_ERR_BAD_PARAMETERS;
	}

	VIRTUAL_MACHINE_STATE state = CDspVm::getVmState(m_sVmUuid, getClient()->getVmDirectoryUuid());
	if ((state == VMS_RUNNING) || (state == VMS_PAUSED)) {
		nRetCode = CDspTaskFailure(*this)
			(PRL_ERR_BACKUP_RESTORE_VM_RUNNING, sVmName);
		WRITE_TRACE(DBG_FATAL, "[%s] VM %s already exists and is running or paused",
				__FUNCTION__, QSTR2UTF8(m_sVmUuid));
		return nRetCode;
	}

	/* AccessCheck */
	bool bSetNotValid = false;
	code = CDspService::instance()->getAccessManager()
			.checkAccess(getClient(), PVE::DspCmdRestoreVmBackup, m_sVmUuid, &bSetNotValid);
	if (PRL_FAILED(code) && code != PRL_ERR_VM_CONFIG_DOESNT_EXIST) {
		nRetCode = CDspTaskFailure(*this).setCode(PRL_ERR_BACKUP_ACCESS_TO_VM_DENIED)
			(sVmName.isEmpty() ? m_sVmUuid : sVmName, PRL_RESULT_TO_STRING(code));
		WRITE_TRACE(DBG_FATAL, "[%s] Access check failed for user {%s} when accessing VM {%s}. Reason: %#x (%s)",
			__FUNCTION__, QSTR2UTF8(getClient()->getClientHandle()),
			QSTR2UTF8(m_sVmUuid), code, PRL_RESULT_TO_STRING(code));
		return nRetCode;
	}

	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
	//Store current VM uptime
	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration());
	nRetCode = CDspService::instance()->getVmConfigManager().loadConfig(pVmConfig, sPathToVmConfig, getClient());
	if ( PRL_FAILED(nRetCode) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to load VM '%s' configuration from '%s' with error: %.8X '%s'",
			QSTR2UTF8(m_sVmUuid), QSTR2UTF8(sPathToVmConfig), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	}
	else
	{
		m_nCurrentVmUptime = pVmConfig->getVmIdentification()->getVmUptimeInSeconds();
		m_current_vm_uptime_start_date = pVmConfig->getVmIdentification()->getVmUptimeStartDateTime();
		WRITE_TRACE(DBG_FATAL, "VM '%s' uptime values before restore: %llu '%s'",
			QSTR2UTF8(m_sVmUuid), m_nCurrentVmUptime,
			QSTR2UTF8(m_current_vm_uptime_start_date.toString(XML_DATETIME_FORMAT)));
	}

	/* create temporary directory */
	m_sTargetPath = QString("%1.%2.restore").arg(m_sTargetVmHomePath).arg(Uuid::createUuid().toString());

	std::auto_ptr<Restore::Assembly> a;
	if (PRL_FAILED(nRetCode = restoreVmToTargetPath(a)))
		return nRetCode;
	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
	//Process VM uptime
	m_pVmConfig->getVmIdentification()->setVmUptimeInSeconds(m_nCurrentVmUptime);
	m_pVmConfig->getVmIdentification()->setVmUptimeStartDateTime(m_current_vm_uptime_start_date);
	// VM name should not change!
	// https://jira.sw.ru/browse/PSBM-20397
	m_pVmConfig->getVmIdentification()->setVmName(sVmName);

	// set current VM home path to the restored config
	m_pVmConfig->getVmIdentification()->setHomePath(sPathToVmConfig);

	// TODO Should we use CDspBugPatcherLogic here?
	// #PSBM-13394
	CDspVmNetworkHelper::updateHostMacAddresses(m_pVmConfig, NULL, HMU_NONE);
	// save config : Task_RegisterVm read config from file system
	if (PRL_FAILED(nRetCode = saveVmConfig()))
		return nRetCode;
	if (PRL_FAILED(nRetCode = a->do_()))
		return nRetCode;
	// Our config is incorrect after moving. Reload it.
	if (PRL_FAILED(nRetCode = CDspService::instance()->getVmConfigManager().loadConfig(
			m_pVmConfig, m_pVmConfig->getVmIdentification()->getHomePath(),
			getClient(), true, true)))
		return nRetCode;

	Libvirt::Instrument::Agent::Vm::Grub::result_type r =
		Libvirt::Kit.vms().getGrub(*m_pVmConfig).spawnPersistent();
	if (r.isFailed())
	{
		a->revert();
		return r.error().code();
	}
	if (m_converter.get() != NULL && PRL_FAILED(nRetCode = doV2V()))
		return nRetCode;
	{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem
			= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getClient()->getVmDirectoryUuid(), m_sVmUuid);
		if ( pVmDirItem )
			pVmDirItem->setTemplate( m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreNewVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sVmName;
	SmartPtr<CVmDirectory::TemporaryCatalogueItem> pVmInfo;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (m_sTargetVmName.size())
		sVmName = m_sTargetVmName;
	else
		sVmName = m_pVmConfig->getVmIdentification()->getVmName();

	if (m_nFlags & PBT_RESTORE_TO_COPY)
	{
		m_pVmConfig = ::Backup::Perspective(m_pVmConfig).clone(m_sVmUuid, sVmName);
		if (!m_pVmConfig.isValid())
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}
	/* ! only after sendStartRequest() - to get m_pVmConfig */
	if (m_sTargetVmHomePath.isEmpty())
	{
		m_sTargetVmHomePath = QFileInfo(
				QString("%1/%2")
				.arg(getClient()->getUserDefaultVmDirPath())
				.arg(Vm::Config::getVmHomeDirName(
					m_pVmConfig->getVmIdentification()->getVmUuid())))
				.absoluteFilePath();
	}
	m_sTargetPath = m_sTargetVmHomePath;

	/* lock register uuid, private & name */
	pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
						m_sVmUuid, m_sTargetPath, sVmName));
	if (PRL_FAILED(nRetCode = lockExclusiveVmParameters(pVmInfo)))
		return nRetCode;
	do
	{
		std::auto_ptr<Restore::Assembly> a;
		if (PRL_FAILED(nRetCode = restoreVmToTargetPath(a)))
			break;
		//Reset VM uptime values
		m_pVmConfig->getVmIdentification()->setVmUptimeInSeconds();
		m_pVmConfig->getVmIdentification()->setVmUptimeStartDateTime();
		// save config : Task_RegisterVm read config from file system
		if (PRL_FAILED(nRetCode = saveVmConfig()))
			break;
		if (PRL_FAILED(nRetCode = a->do_()))
			break;
		if (PRL_FAILED(nRetCode = registerVm()))
		{
			a->revert();
			break;
		}
		if (m_converter.get() != NULL && PRL_FAILED(nRetCode = doV2V())
			&& nRetCode != PRL_ERR_FIXME)
		{
			CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(pVmInfo.getImpl());
			unregisterVm();
			a->revert();
		}

		if (nRetCode == PRL_ERR_FIXME)
			nRetCode = PRL_ERR_TIMEOUT;
	} while(false);
	CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(pVmInfo.getImpl());
	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::getFiles(bool bVmExist_)
{
	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderClient(m_pIoClient));
	if (bVmExist_) {
		m_pVmCopyTarget = SmartPtr<CVmFileListCopyTarget>(
			new CVmFileListCopyTarget(
				m_pSender.getImpl(),
				m_sVmUuid,
				m_sTargetPath,
				getLastError(),
				m_nTimeout)
		);
		m_pVmCopyTarget->SetProgressNotifySender(NotifyClientsWithProgress);
	} else {
		m_pVmCopyTarget = SmartPtr<CVmFileListCopyTarget>(
			new CVmFileListCopyTarget(
				m_pSender.getImpl(),
				getClient()->getClientHandle(),
				m_sTargetPath,
				getLastError(),
				m_nTimeout)
		);
		m_pVmCopyTarget->SetProgressNotifySender(NotifyInitiatorWithProgress);
	}
	m_pVmCopyTarget->SetRequest(getRequestPackage());
	m_pVmCopyTarget->SetVmDirectoryUuid(getClient()->getVmDirectoryUuid());

	/* set FileCopy* packages handler.
	   Attn: disconnect() on success and on failure both */
	bool bConnected = QObject::connect(m_pIoClient.getImpl(),
		SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
		SLOT(handlePackage(const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	/* Target is ready - give source a kick to start file copy */
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(PRL_ERR_SUCCESS, m_pReply);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(DispToDispResponseCmd, pCmd, m_pReply);

	PRL_RESULT nRetCode = SendPkg(pPackage);
	if (PRL_SUCCEEDED(nRetCode))
		/* and start to process incoming FileCopy* packages */
		nRetCode = exec();
	QObject::disconnect(m_pIoClient.getImpl(),
		SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(const SmartPtr<IOPackage>)));

	if (PRL_FAILED(nRetCode))
		return nRetCode;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (m_nFlags & PBT_RESTORE_TO_COPY)
	{
		::Vm::Private::Brand b(m_sTargetPath, getClient());
		b.remove();
		nRetCode = b.stamp();
		
	}
	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreVmToTargetPath(std::auto_ptr<Restore::Assembly>& dst_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode;
	Legacy::Vm::result_type res;
	// Check resulting config before restoring disks.
	if (m_converter.get() != NULL &&
		(res = m_converter->convertHardware(m_pVmConfig)).isFailed())
		return CDspTaskFailure(*this)(*res.error());

	::Backup::Object::Model m(m_pVmConfig);
	if (m.isBad())
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	::Backup::Product::Model p(m, m_sTargetPath);
	p.setStore(m_sBackupRootPath);
	if (BACKUP_PROTO_V4 <= m_nRemoteVersion)
		p.setSuffix(::Backup::Suffix(m_nBackupNumber)());
	Restore::Target::Vm u(m_nBackupNumber, m_sTargetPath, m_sBackupRootPath,
			Restore::Assistant(*this,
				Restore::AClient::Unit(*this, m_sOriginVmUuid, *m_pVmConfig)));
	if (PRL_FAILED(nRetCode = u.createHome()))
		return nRetCode;
	foreach (const ::Backup::Product::component_type& d, p.getVmTibs())
	{
		if (PRL_FAILED(nRetCode = u.add(d)))
			return nRetCode;
	}
        /* now to set permissions from backup info (https://jira.sw.ru/browse/PSBM-1035) */
        if (m_nBundlePermissions)
	{
                QFile vmBundle(m_sTargetPath);
                if (!vmBundle.setPermissions((QFile::Permissions)m_nBundlePermissions))
		{
                        WRITE_TRACE(DBG_FATAL,
                                "[%s] Cannot set permissions for Vm bundle \"%s\", will use default permissions",
                                __FUNCTION__, QSTR2UTF8(m_sTargetPath));
                }
        }
	// NB. the protocol requires the getFiles goes first. otherwise
	// the source side hangs.
	if (PRL_FAILED(nRetCode = getFiles(m_bVmExist)))
		return nRetCode;

	Restore::Target::Vm::noSpace_type y;
	if (u.isNoSpace(y))
	{
		nRetCode = Task_BackupHelper::checkFreeDiskSpace(y.second, y.first, false);
		if (PRL_FAILED(nRetCode))
			return nRetCode;
	}
	// Stage TWO
	if (PRL_FAILED(nRetCode = u.restore(m_nRemoteVersion)))
		return nRetCode;
        /*
           Now target side wait new acronis proxy commands due to acronis have not call to close connection.
           To fix it will send command to close connection from here.
           Pay attention: on success and on failure both we will wait reply from target.
        */
	SmartPtr<IOPackage> pPackage = IOPackage::createInstance(ABackupProxyFinishCmd, 0);
	if (PRL_FAILED(nRetCode = SendPkg(pPackage)))
		return nRetCode;

	/* to check device images in new home directory (#460845) */
	if (PRL_FAILED(nRetCode = fixHardWareList()))
		return nRetCode;

	if (m_pVmConfig->getVmIdentification()->getHomePath().isEmpty())
	{
		m_pVmConfig->getVmIdentification()->setHomePath(
				QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath));
	}
	// Update cpu features on pcs6 restore
	if (m_converter.get() != NULL)
		CCpuHelper::update(*m_pVmConfig);

	dst_.reset(u.assemble(m_sTargetVmHomePath));
	return NULL == dst_.get() ? PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR : PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreCt()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sVzDirUuid;
	QString sDefaultCtFolder;
	SmartPtr<CVmConfiguration> pConfig;

/*
1. if PBT_RESTORE_TO_COPY - create new CT with new UUID, do not touch existing CT
2. if CT already exist - ignore m_sTargetVmHomePath & m_sTargetVmName,
   restore over existing CT with the same CTID, name and private
3. create new CT:
   - m_sTargetVmHomePath defined - use as private
   - m_sTargetVmHomePath not defined - private is default private for this node (VE_PRIVATE from /etc/vz/vz.conf)
   - m_sTargetVmName defined - if numeric, use as CTID (do not change private), if non-numeric - use as name
   - m_sTargetVmName not defined - use name from config
   - Conflicts:
        private - error
        name - error
	CTID - error if m_sTargetVmName is numeric, if m_sTargetVmName is non-numeric, will generate new CTID
*/

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	{
		CDspLockedPointer<CVmDirectory> pDir = CDspService::instance()->getVmDirManager().getVzDirectory();
		if (!pDir) {
			WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
			return PRL_ERR_VM_UUID_NOT_FOUND;
		}
		sVzDirUuid = pDir->getUuid();
		sDefaultCtFolder = CVzHelper::getVzPrivateDir();
	}

	nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
		m_sVmUuid, sVzDirUuid, PVE::DspCmdRestoreVmBackup, getClient(), this->getJobUuid());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "registerExclusiveVmOperation failed. Reason: %#x (%s)",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}

	/* flag PBT_RESTORE_TO_COPY - create new CT and ignore existing CT */
	if (!(m_nFlags & PBT_RESTORE_TO_COPY)) {
		/* does this Ct already exists? */
		pConfig = CVzHelper::get_env_config(m_sVmUuid);
	}
	if (pConfig) {
		m_bVmExist = true;
		nRetCode = restoreCtOverExisting(pConfig);
	} else {
		nRetCode = restoreNewCt(sDefaultCtFolder);
	}

	CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
		m_sVmUuid, sVzDirUuid, PVE::DspCmdRestoreVmBackup, getClient());

	sync();
	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreCtOverExisting(const SmartPtr<CVmConfiguration> &pConfig)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sCtName;

	/* ID, name, private : use values of existing CT, ignore m_sTargetVmHomePath & m_sTargetVmName from command
	   and values from backuped CT config */

	/* do not change bundle of existing Vm */
	if (m_sTargetVmHomePath.size()) {
		WRITE_TRACE(DBG_FATAL, "It is't possible to set target private for restore existing VM");
		return PRL_ERR_BACKUP_RESTORE_EXISTING_SET_PRIVATE;
	}

	QString ctId = pConfig->getVmIdentification()->getCtId();
	/* attn : this function restore real name or ct id */
	sCtName = pConfig->getVmIdentification()->getVmName();
	m_sTargetVmHomePath = pConfig->getVmIdentification()->getHomePath();
	/* check */
	if (ctId.isEmpty() || sCtName.isEmpty() || m_sTargetVmHomePath.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "[%s] ctId = %s, sCtName = '%s', m_sTargetVmHomePath = '%s'",
				__FUNCTION__, QSTR2UTF8(ctId), QSTR2UTF8(sCtName),
				QSTR2UTF8(m_sTargetVmHomePath));
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}

	/* do not change name of existing Vm - but PMC always send name */
	if (m_sTargetVmName.size() && (m_sTargetVmName != sCtName)) {
		WRITE_TRACE(DBG_FATAL, "It is't possible to set name for restore existing VM");
		return PRL_ERR_BAD_PARAMETERS;
	}

	CVzHelper& VzHelper = CDspService::instance()->getVzHelper()->getVzlibHelper();

	/* check existing Ct state */
	VIRTUAL_MACHINE_STATE state;
	nRetCode = VzHelper.get_env_status(m_sVmUuid, state);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "get_env_status() failer. Reason: %#x (%s)",
				nRetCode, PRL_RESULT_TO_STRING(nRetCode));

		return nRetCode;
	}

	if (state == VMS_RUNNING || state == VMS_MOUNTED) {
		nRetCode = CDspTaskFailure(*this)(PRL_ERR_BACKUP_RESTORE_VM_RUNNING, sCtName);
		WRITE_TRACE(DBG_FATAL, "[%s] VM %s already exists and is running or mounted",
				__FUNCTION__, QSTR2UTF8(m_sVmUuid));
		return nRetCode;
	}

	int lockfd = CVzHelper::lock_env(m_sVmUuid, "Restoring");
	if (lockfd < 0) {
		WRITE_TRACE(DBG_FATAL, "Can't lock Container");
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}

	/* will restore over existing home - to create temporary directory */
	m_sTargetPath = QString("%1.%2.restore").arg(m_sTargetVmHomePath).arg(Uuid::createUuid().toString());

	/* Store current VM uptime */
	m_nCurrentVmUptime = pConfig->getVmIdentification()->getVmUptimeInSeconds();
	m_current_vm_uptime_start_date = pConfig->getVmIdentification()->getVmUptimeStartDateTime();
	WRITE_TRACE(DBG_FATAL, "VM '%s' uptime values before restore: %llu '%s'",
			QSTR2UTF8(m_sVmUuid), m_nCurrentVmUptime,
			QSTR2UTF8(m_current_vm_uptime_start_date.toString(XML_DATETIME_FORMAT)));

	std::auto_ptr<Restore::Assembly> x;

	do {
		if (PRL_FAILED(nRetCode = restoreCtToTargetPath(sCtName, false, x)))
			break;
		/* replace original private */
		nRetCode = x->do_();

		/* and reregister to set ctId & m_sTargetVmHomePath in restored config */
		nRetCode = m_VzOpHelper.register_env(pConfig, PRCF_FORCE|PRVF_IGNORE_HA_CLUSTER);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "register_env() exited with error %#x, %s",
					nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
			break;
		}

		/* restore uptime */
		nRetCode = VzHelper.set_env_uptime(m_sVmUuid, m_nCurrentVmUptime,
				m_current_vm_uptime_start_date);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "Restore uptime failed with error %#x, %s",
					nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
			break;
		}
		/* invalidate config */
		CDspService::instance()->getVzHelper()->getConfigCache().
				remove(m_sTargetVmHomePath);
	} while(0);

	if (PRL_FAILED(nRetCode) && x.get()) {
		x->revert();
		CFileHelper::ClearAndDeleteDir(m_sTargetPath);
	}

	CVzHelper::unlock_env(m_sVmUuid, lockfd);

	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreNewCt(const QString &sDefaultCtFolder)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString ctId;
	QString sCtName;
	VIRTUAL_MACHINE_STATE nState;
	SmartPtr<CVmConfiguration> pConfig;
	bool registered = false;

	ctId = m_pVmConfig->getVmIdentification()->getCtId();
	if (ctId.isEmpty() || m_sVmUuid != m_sOriginVmUuid) {
		ctId = CVzHelper::build_ctid_from_uuid(m_sVmUuid);
	}

	if (m_sTargetVmName.isEmpty())
		sCtName = m_pVmConfig->getVmIdentification()->getVmName();
	else
		sCtName = m_sTargetVmName;

	if (m_sTargetVmHomePath.isEmpty())
		m_sTargetVmHomePath = QString("%1/%2").arg(sDefaultCtFolder).arg(ctId);
	/* will restore to target directory */
	m_sTargetPath = m_sTargetVmHomePath;

	if (m_nFlags & PBT_RESTORE_TO_COPY)
	{
		QString n = sCtName.isEmpty() ? ctId : sCtName;
		m_pVmConfig = ::Backup::Perspective(m_pVmConfig).clone(m_sVmUuid, n);
		if (!m_pVmConfig.isValid())
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		m_pVmConfig->getVmIdentification()->setCtId(ctId);
		m_pVmConfig->getVmIdentification()->setHomePath(m_sTargetPath);
	}

	/* check env id */
	nRetCode = CVzHelper::get_env_status_by_ctid(ctId, nState);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "CVzTemplateHelper::get_env_status() failed. Reason: %#x (%s)",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}
	if (nState != VMS_UNKNOWN) {
		/* failure */
		nRetCode = CDspTaskFailure(*this)(PRL_ERR_BACKUP_CT_ID_ALREADY_EXIST, ctId);
		WRITE_TRACE(DBG_FATAL, "Container with ID %s already exist",
				QSTR2UTF8(ctId));
		return nRetCode;
	}

	/* lock register uuid, private & name */
	SmartPtr<CVmDirectory::TemporaryCatalogueItem>pCtInfo =
		SmartPtr<CVmDirectory::TemporaryCatalogueItem>(
			new CVmDirectory::TemporaryCatalogueItem(
				m_sVmUuid, m_sTargetPath, sCtName));
	nRetCode = lockExclusiveVmParameters(pCtInfo);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	std::auto_ptr<Restore::Assembly> x;
	do {
		if (PRL_FAILED(nRetCode = restoreCtToTargetPath(sCtName, true, x)))
			break;

		if (PRL_FAILED(nRetCode = x->do_()))
			break;

		nRetCode = m_VzOpHelper.register_env(m_sTargetPath, ctId,
				m_pVmConfig->getVmIdentification()->getVmUuid(),
				sCtName, PRCF_FORCE, pConfig);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "register_env() exited with error %#x, %s",
					nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
			break;
		}
		registered = true;

		CVzHelper::update_ctid_map(m_sVmUuid, ctId);
		if (m_nFlags & PBT_RESTORE_TO_COPY) {
			nRetCode = m_VzOpHelper.apply_env_config(m_pVmConfig, pConfig, 0);
			if (PRL_FAILED(nRetCode)) {
				WRITE_TRACE(DBG_FATAL, "apply_env_config() exited with error %#x, %s",
						nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
				break;
			}
			pConfig = m_pVmConfig;
		}

		nRetCode = CDspService::instance()->getVzHelper()->insertVmDirectoryItem(pConfig);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "Can't insert vm to VmDirectory by error %#x, %s",
					nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
			break;
		}

		if (PRL_FAILED(CVzHelper::reset_env_uptime(m_sVmUuid))) {
			WRITE_TRACE(DBG_FATAL, "Reset uptime failed with error %#x, %s",
				nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
		}
	} while (0);

	if (PRL_FAILED(nRetCode)) {
		if (x.get())
			x->revert();
		if (registered)
			m_VzOpHelper.unregister_env(m_sVmUuid, PRCF_FORCE | PRCF_UNREG_PRESERVE);
	}

	CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(pCtInfo.getImpl());

	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreCtToTargetPath(
			const QString &sCtName,
			bool bIsRealMountPoint,
			std::auto_ptr<Restore::Assembly>& dst_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	Restore::Assistant a(*this, Restore::AClient::Unit(*this, m_sOriginVmUuid, sCtName));
	PRL_RESULT output = a.make(m_sTargetPath, true);
	if (PRL_FAILED(output))
		return output;

	Restore::AClient::Api api(m_nBackupNumber, m_sBackupRootPath);
	Restore::Target::Ct t(a, *this, &Task_RestoreVmBackupTarget::SendPkg);
	if (m_nInternalFlags & PVM_CT_PLOOP_BACKUP)
	{
		if (PRL_SUCCEEDED(output = getFiles(bIsRealMountPoint)))
		{
			Restore::Query::Work w(api, getIoClient(), m_nBackupTimeout);
			Backup::Product::Model p(Backup::Object::Model(m_pVmConfig), m_sTargetPath);
			p.setStore(m_sBackupRootPath);
			Backup::Product::componentList_type l = p.getCtTibs();
			if (BACKUP_PROTO_V4 <= m_nRemoteVersion) {
				p.setSuffix(::Backup::Suffix(m_nBackupNumber)());
				l = p.getVmTibs();
			}
			Restore::Target::Ploop::Flavor f(m_sTargetPath, l, w);
			dst_.reset(t(f, m_sTargetVmHomePath, m_nRemoteVersion));
		}
	}
	else if (m_nInternalFlags & PVM_CT_VZFS_BACKUP)
		return PRL_ERR_UNIMPLEMENTED;

	if (PRL_SUCCEEDED(output))
		output = t.getResult();
	if (PRL_FAILED(output))
		CFileHelper::ClearAndDeleteDir(m_sTargetPath);

	return output;
}

PRL_RESULT Task_RestoreVmBackupTarget::lockExclusiveVmParameters(SmartPtr<CVmDirectory::TemporaryCatalogueItem> pInfo)
{
	CDspTaskFailure f(*this);
	/* lock register uuid, private & name */
	PRL_RESULT nRetCode = CDspService::instance()->getVmDirManager()
				.checkAndLockNotExistsExclusiveVmParameters(QStringList(), pInfo.getImpl());
	switch (nRetCode)
	{
	case PRL_ERR_SUCCESS:
		return PRL_ERR_SUCCESS;

	case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
		WRITE_TRACE(DBG_FATAL, "UUID '%s' already registered", QSTR2UTF8(pInfo->vmUuid));
		return f.setToken(pInfo->vmUuid)(nRetCode);

	case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
		WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(pInfo->vmXmlPath));
		return f.setCode(nRetCode)(pInfo->vmName, pInfo->vmXmlPath);

	case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
		WRITE_TRACE(DBG_FATAL, "name '%s' already registered", QSTR2UTF8(pInfo->vmName));
		return f(nRetCode, pInfo->vmName);

	case PRL_ERR_VM_ALREADY_REGISTERED:
		WRITE_TRACE(DBG_FATAL, "container '%s' already registered", QSTR2UTF8(pInfo->vmName));
		return f(nRetCode, pInfo->vmName);
		break;

	case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default
	default:
		WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
			QSTR2UTF8(pInfo->vmUuid), QSTR2UTF8(pInfo->vmName), QSTR2UTF8(pInfo->vmXmlPath));
		return f.setToken(pInfo->vmUuid).setToken(pInfo->vmXmlPath)
			.setToken(pInfo->vmName)(nRetCode);
	}
}

/* send start request for remote dispatcher and wait reply from dispatcher */
PRL_RESULT Task_RestoreVmBackupTarget::sendStartRequest()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pStartCmd;
	SmartPtr<IOPackage> pPackage;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	QString u = (m_nFlags & PBT_RESTORE_TO_COPY) && !m_sBackupId.isEmpty() ? QString() : m_sVmUuid;
	pStartCmd = CDispToDispProtoSerializer::CreateVmBackupRestoreCommand(u, m_sBackupId, m_nFlags);

	pPackage = DispatcherPackage::createInstance(
			pStartCmd->GetCommandId(),
			pStartCmd->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, m_pReply)))
		return nRetCode;

	if (	(m_pReply->header.type != VmBackupRestoreFirstReply) &&
		(m_pReply->header.type != DispToDispResponseCmd))
	{
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x or %x",
			m_pReply->header.type, DispToDispResponseCmd, VmBackupRestoreFirstReply);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(
		(Parallels::IDispToDispCommands)m_pReply->header.type, UTF8_2QSTR(m_pReply->buffers[0].getImpl()));

	if (m_pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);

		if (PRL_FAILED(nRetCode = pResponseCmd->GetRetCode())) {
			getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
			return nRetCode;
		}
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
	CVmBackupRestoreFirstReply *pFirstReply =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupRestoreFirstReply>(pCmd);

	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	if (PRL_FAILED(nRetCode = m_pVmConfig->fromString(pFirstReply->GetVmConfig())))
		return nRetCode;
	m_nRemoteVersion = pFirstReply->GetVersion();
	if (m_nRemoteVersion <= BACKUP_PROTO_V3)
		m_converter.reset(new Legacy::Vm::Converter());
	m_sOriginVmUuid = pFirstReply->GetVmUuid();
	m_sBackupUuid = pFirstReply->GetBackupUuid();
	m_nBackupNumber = pFirstReply->GetBackupNumber();
	m_sBackupRootPath = pFirstReply->GetBackupRootPath();
	m_nOriginalSize = pFirstReply->GetOriginalSize();
	m_nBundlePermissions = (m_nRemoteVersion >= BACKUP_PROTO_V2) ? pFirstReply->GetBundlePermissions() : 0;
	m_nInternalFlags = pFirstReply->GetInternalFlags();

	return nRetCode;
}

/* send request to restore image for remote dispatcher and wait reply from dispatcher */
Prl::Expected<QString, PRL_RESULT> Task_RestoreVmBackupTarget::sendMountImageRequest(
		const QString& archive_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	SmartPtr<IOPackage> r, p = IOPackage::createInstance(VmBackupMountImage, 1);
	QByteArray d = archive_.toUtf8();
	p->fillBuffer(0, IOPackage::RawEncoding, d.constData(), d.size()+1);
	PRL_RESULT output;
	if (PRL_FAILED(output = SendReqAndWaitReply(p, r)))
		return output;

	if (r->header.type == VmBackupRestoreImage)
		return UTF8_2QSTR(r->buffers[0].getImpl());
		
	if (r->header.type != DispToDispResponseCmd)
	{
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x",
			r->header.type, DispToDispResponseCmd);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}

	CDispToDispResponseCommand *rc =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(
			CDispToDispProtoSerializer::ParseCommand((Parallels::IDispToDispCommands)r->header.type,
			UTF8_2QSTR(r->buffers[0].getImpl())));

	if (PRL_FAILED(output = rc->GetRetCode()))
		setLastErrorCode(output);
	return output;
}

void Task_RestoreVmBackupTarget::handlePackage(const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (!m_pVmCopyTarget) {
		WRITE_TRACE(DBG_FATAL,
			"handler of FileCopy package (type=%d) is NULL", p->header.type);
		exit(PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR);
	}

	nRetCode = m_pVmCopyTarget->handlePackage(p, &bExit);
	if (bExit)
		exit(nRetCode);
}

PRL_RESULT Task_RestoreVmBackupTarget::fixHardWareList()
{
	QFileInfo fileInfo;
	QDir dir(m_sTargetVmHomePath);
	CVmHardware *pVmHardware;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if ((pVmHardware = m_pVmConfig->getVmHardwareList()) == NULL) {
		WRITE_TRACE(DBG_FATAL, "[%s] Can not get Vm hardware list", __FUNCTION__);
		return PRL_ERR_SUCCESS;
	}

	/* process floppy disk images */
	foreach(CVmFloppyDisk *pDevice, pVmHardware->m_lstFloppyDisks) {
		if (NULL == pDevice)
			continue;
		if ((pDevice->getEnabled() == PVE::DeviceDisabled) ||
				(PVE::DeviceDisconnected == pDevice->getConnected()))
			continue;
		if (pDevice->getEmulatedType() != PVE::FloppyDiskImage)
			continue;
		fileInfo.setFile(dir, pDevice->getSystemName());
		if (fileInfo.exists())
			continue;
		pDevice->setConnected(PVE::DeviceDisconnected);
	}

	foreach(CVmOpticalDisk* pDevice, pVmHardware->m_lstOpticalDisks) {
		if (NULL == pDevice)
			continue;
		if (pDevice->getEmulatedType() != PVE::CdRomImage)
			continue;
		if ((pDevice->getEnabled() == PVE::DeviceDisabled) ||
				(PVE::DeviceDisconnected == pDevice->getConnected()))
			continue;
		fileInfo.setFile(dir, pDevice->getSystemName());
		if (fileInfo.exists())
			continue;
		pDevice->setConnected(PVE::DeviceDisconnected);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupTarget::saveVmConfig()
{
	// we force restoring to a stopped vm. set the cluster options
	// accordingly.
	m_pVmConfig->getVmSettings()->getClusterOptions()->setRunning(false);
	QString vmConfigPath = QString("%1/%2").arg(m_sTargetPath).arg(VMDIR_DEFAULT_VM_CONFIG_FILE);
	PRL_RESULT nRetCode = CDspService::instance()->getVmConfigManager().saveConfig(
			m_pVmConfig, vmConfigPath, getClient(), true);
	if (PRL_FAILED(nRetCode))
	{
		WRITE_TRACE(DBG_FATAL, "save Vm config %s failed: %s",
				QSTR2UTF8(vmConfigPath),
				PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupTarget::unregisterVm()
{
	SmartPtr<CDspClient> client = getClient();
	CVmEvent evt;

	CProtoCommandPtr pRequest =
		CProtoSerializer::CreateProtoBasicVmCommand(
			PVE::DspCmdDirUnregVm, m_sVmUuid);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdDirUnregVm, pRequest);

	/* do not call checkAndLockNotExistsExclusiveVmParameters */
	CDspService::instance()->getTaskManager().schedule(new Task_DeleteVm(
		client, pPackage, m_pVmConfig->toString(),
		PVD_UNREGISTER_ONLY | PVD_SKIP_VM_OPERATION_LOCK)).wait().getResult(&evt);

	// wait finishing thread and return task result
	if (PRL_FAILED(evt.getEventCode())) {
		WRITE_TRACE(DBG_FATAL,
			"Error occurred while unregister Vm %s with code [%#x][%s]",
			qPrintable(m_sVmUuid),
			evt.getEventCode(),
			PRL_RESULT_TO_STRING(evt.getEventCode()));
	}

	return evt.getEventCode();
}

PRL_RESULT Task_RestoreVmBackupTarget::registerVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<CDspClient> client = getClient();
	CVmEvent evt;

	CProtoCommandPtr pRequest =
		CProtoSerializer::CreateProtoCommandWithOneStrParam(
			PVE::DspCmdDirRegVm, m_sTargetVmHomePath, false, PRVF_KEEP_OTHERS_PERMISSIONS);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdDirRegVm, pRequest);

	/* do not call checkAndLockNotExistsExclusiveVmParameters */
	CDspService::instance()->getTaskManager().schedule(new Task_RegisterVm(m_registry,
		client, pPackage, m_sTargetVmHomePath, PACF_NON_INTERACTIVE_MODE,
			QString(), QString(), REG_SKIP_VM_PARAMS_LOCK)).wait().getResult(&evt);

	// wait finishing thread and return task result
	if (PRL_FAILED(evt.getEventCode())) {
		nRetCode = CDspTaskFailure(*this).setCode(PRL_ERR_BACKUP_REGISTER_VM_FAILED)
			(m_pVmConfig->getVmIdentification()->getVmName(), PRL_RESULT_TO_STRING(evt.getEventCode()));
		WRITE_TRACE(DBG_FATAL,
			"[%s] Error occurred while register Vm %s from backup %s with code [%#x][%s]",
			__FUNCTION__,
			QSTR2UTF8(m_sVmUuid),
			QSTR2UTF8(m_sBackupUuid),
			evt.getEventCode(),
			PRL_RESULT_TO_STRING(evt.getEventCode()));
	}

	return nRetCode;
}

void Task_RestoreVmBackupTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);
	checkVmAdditionState(true);

	if (m_pVmCopyTarget.isValid())
		m_pVmCopyTarget->cancelOperation();

	killABackupClient();

	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

PRL_RESULT Task_RestoreVmBackupTarget::doV2V()
{
	QTimer t;
	if (!QObject::connect(&t, SIGNAL(timeout()), SLOT(runV2V()), Qt::DirectConnection))
		return PRL_ERR_FAILURE;

	t.setInterval(0);
	t.setSingleShot(true);
	t.start();
	return exec();
}

void Task_RestoreVmBackupTarget::runV2V()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	boost::optional<Legacy::Vm::V2V> v2v = m_converter->getV2V(*m_pVmConfig);

	do {
		if (!v2v)
			break;
		//Substitute NVRAM
		Legacy::Vm::result_type res = m_converter->convertBios(m_pVmConfig);
		if (res.isFailed())
		{
			nRetCode = res.error()->getEventCode();
			break;
		}
		if (PRL_FAILED(nRetCode = v2v->do_()))
			break;
		Prl::Expected<void, Error::Simple> r = v2v->start();
		if (r.isFailed())
			nRetCode = r.error().code();
	} while (0);

	exit(nRetCode);
}

::Backup::Tunnel::Source::Factory Task_RestoreVmBackupTarget::craftTunnel()
{
	return ::Backup::Tunnel::Source::Factory(m_sServerHostname, getIoClient());
}

