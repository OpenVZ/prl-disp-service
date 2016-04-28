///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackup.cpp
///
/// Source and target tasks for Vm backup restoring
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

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include "Task_RestoreVmBackup_p.h"
#include "prlcommon/Logging/Logging.h"
#include "prlcommon/PrlUuid/Uuid.h"
#include "Libraries/StatesStore/SavedStateTree.h"

#include "Task_RestoreVmBackup.h"
#include "Tasks/Task_RegisterVm.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "CDspVmNetworkHelper.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlcommon/HostUtils/HostUtils.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "Libraries/PrlNetworking/netconfig.h"
#include "CDspBackupDevice.h"
#ifdef _CT_
#include "vzctl/libvzctl.h"
#endif

namespace
{

enum {V2V_RUN_TIMEOUT = 60 * 60 * 1000};

const char VIRT_V2V[] = "/usr/bin/virt-v2v";

QString getHostOnlyBridge()
{
	Libvirt::Instrument::Agent::Network::Unit u;
	// Get Host-Only network.
	Libvirt::Result r = Libvirt::Kit.networks().find(
			PrlNet::GetDefaultVirtualNetworkID(0, false), &u);
	if (r.isFailed())
		return QString();

	CVirtualNetwork n;
	r = u.getConfig(n);
	if (r.isFailed())
		return QString();

	CParallelsAdapter* a;
	if (NULL == n.getHostOnlyNetwork() ||
		(a = n.getHostOnlyNetwork()->getParallelsAdapter()) == NULL)
		return QString();

	return a->getName();
}

} // namespace

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
	return m_impl->startABackupClient(m_name, argv_, QStringList(), m_uuid, disk_);
}


PRL_RESULT Unit::operator()(const QStringList& argv_, SmartPtr<Chain> custom_) const
{
	return m_impl->startABackupClient(m_name, argv_, QStringList(), custom_);
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

QStringList Api::query(const Backup::Archive& archive_) const
{
	return QStringList() << "query_archive" << ""
			<< m_backupRoot.absolutePath()
			<< m_backupRoot.absoluteFilePath(archive_.getName())
			<< QString::number(m_no);
}

QStringList Api::restore(const Backup::Archive& archive_, const QFileInfo& target_) const
{
	return restore(m_backupRoot.absoluteFilePath(archive_.getName()), target_);
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

namespace Query
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

Handler::Handler(SmartPtr<IOClient> io_, quint32 timeout_): m_size(0ULL), m_usage(0ULL)
{
	next(SmartPtr<Chain>(new Forward(io_, timeout_)));
}

PRL_RESULT Handler::do_(SmartPtr<IOPackage> request_, BackupProcess& dst_)
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

PRL_RESULT Work::operator()(const Backup::Archive& archive_, const Assistant& assist_,
			quint64& dst_) const
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

PRL_RESULT Vm::add(const ::Backup::Archive& archive_)
{
	if (0 < m_hddMap.count(archive_.getDevice().getIndex()))
		return PRL_ERR_SUCCESS;

	QString y = archive_.getRestoreFolder();
	QFileInfo f(y);
	PRL_RESULT r = make(f.absolutePath());
	if (PRL_FAILED(r))
		return r;
	Hdd d;
	d.tib.setFile(archive_.getPath(m_backupRoot));
	QString x = archive_.getImageFolder();
	if (x != y)
		d.final = x;

	d.mountPoint = CFileHelper::GetMountPoint(f.absolutePath());
	d.sizeOnDisk = archive_.getDevice().getSizeOnDisk() << 20;
	d.intermediate = f;
	m_hddMap.insert(std::make_pair(archive_.getDevice().getIndex(), d));
	return PRL_ERR_SUCCESS;
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

PRL_RESULT Vm::restore() const
{
	QStringList z;
	hddMap_type::const_iterator p, e = m_hddMap.end();
	for (p = m_hddMap.begin(); p != e; ++p)
	{
		z << "restore" << p->second.intermediate.absoluteFilePath()
			<< m_backupRoot << p->second.tib.absoluteFilePath()
			<< QString::number(m_no);
		PRL_RESULT r = m_assist(z, p->first);
		if (PRL_FAILED(r))
			return r;
		z.clear();
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

#ifdef _CT_
///////////////////////////////////////////////////////////////////////////////
// struct Ct

template<class F>
Restore::Assembly* Ct::operator()(F flavor_, const QString& home_)
{
	Restore::Assembly* output = NULL;
	if (PRL_FAILED(m_result = flavor_.restore(*m_assist)))
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

///////////////////////////////////////////////////////////////////////////////
// struct Vzwin

Vzwin::Vzwin(const QString& home_, const AClient::Api& api_, quint32 veid_):
	m_home(home_), m_argv(api_.restore(home_, veid_))
{
}

PRL_RESULT Vzwin::assemble(const QString& home_, Restore::Assembly& dst_)
{
	// Restore private from zip archive, if exists
	QFileInfo z(QDir(m_home), PRL_CT_BACKUP_ZIP_FILE_NAME);
	if (z.exists())
	{
		QProcess proc;
		QStringList args;

		args += "-o";
		args += "-XX";
		args += z.canonicalFilePath();
		args += "*.*";
		args += "-d";
		args += m_home;

		proc.setWorkingDirectory(m_home);
#ifdef _WIN_
		proc.start(CVzHelper::getVzInstallDir() + "\\bin\\unzip.exe", args);
#endif // _WIN_
		if (!proc.waitForFinished(ZIP_WORK_TIMEOUT))
		{
			WRITE_TRACE(DBG_FATAL, "restore unzip utility is not responding. Terminate it now.");
			proc.terminate();
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		}
		else if (QProcess::CrashExit == proc.exitStatus())
		{
			WRITE_TRACE(DBG_FATAL, "restore unzip utility crashed");
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		}
		else if (proc.exitCode() != 0)
		{
			WRITE_TRACE(DBG_FATAL, "restore unzip utility failed code %u: %s",
				    proc.exitCode(),
				    proc.readAllStandardOutput().constData());
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		}
		QFile::remove(z.canonicalFilePath());
	}
	dst_.addEssential(m_home, home_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vzwin::restore(const Assistant& assist_)
{
	return assist_(m_argv, 0);
}

#ifdef _LIN_
namespace Ploop
{
///////////////////////////////////////////////////////////////////////////////
// struct Device

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

Device::Device(const QString& path_): m_path(path_)
{
}

} // namespace Ploop

namespace Vzfs
{
///////////////////////////////////////////////////////////////////////////////
// struct Reference

QString Reference::getConfig() const
{
	return getHome(VZ_CT_CONFIG_FILE);
}

QString Reference::getCacheMark() const
{
	return QString("cache.%1" VMDIR_DEFAULT_VM_BACKUP_SUFFIX).arg(m_backup);
}

QString Reference::getPrivateMark() const
{
	return QString("private.%1" VMDIR_DEFAULT_VM_BACKUP_SUFFIX).arg(m_backup);
}

QString Reference::getCache() const
{
	QString t, u = m_uuid;
	u.remove(QChar('{'));
	u.remove(QChar('}'));
	if (CDspService::instance()->getVzHelper()->getVzlibHelper().get_vz_config_param("TEMPLATE", t))
		t = "/vz/template";

	return QString("%1/vc/%2").arg(t).arg(u);
}

namespace Assembly
{
///////////////////////////////////////////////////////////////////////////////
// struct Work

PRL_RESULT Work::operator()(const QString& target_, const Assistant& assist_) const
{
	Toolkit k = assist_.getToolkit();
	k.unlink(m_file);
	// move content to a proper place.
	PRL_RESULT e = Folder(m_file.dir()).move(target_, k);
	if (PRL_FAILED(e))
		return e;

	k.unlink(m_root);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Cache

Cache::Cache(const Reference& reference_, Work* work_):
	m_final(reference_.getCache()), m_work(work_)
{
	// move vz cache content to proper place, but with .backup suffix
	m_intermediate = QString(m_final).append(VMDIR_DEFAULT_VM_BACKUP_SUFFIX);
}

void Cache::join(Restore::Assembly& dst_) const
{
	dst_.addEssential(m_intermediate, m_final);
}

PRL_RESULT Cache::do_(const Assistant& assist_)
{
	Toolkit k = assist_.getToolkit();
	/* 1. remove existing */
	k.unlink(m_intermediate);
	/* 2. create directory */
	if (PRL_FAILED(k.mkdir(m_intermediate)))
	{
		CVmEvent *pEvent = assist_.event();
		pEvent->setEventCode(PRL_ERR_BACKUP_RESTORE_CANNOT_CREATE_DIRECTORY);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, m_intermediate, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory", __FUNCTION__, QSTR2UTF8(m_intermediate));
		return PRL_ERR_BACKUP_RESTORE_CANNOT_CREATE_DIRECTORY;
	}
	/* 3. set perms 755 on target directory */
	if (!QFile(m_intermediate).setPermissions(
			QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
			QFile::ReadGroup | QFile::ExeGroup |
			QFile::ReadOther | QFile::ExeOther))
	{
		WRITE_TRACE(DBG_FATAL,
			"[%s] Cannot set permissions for directory \"%s\", will use default permissions",
			__FUNCTION__, QSTR2UTF8(m_intermediate));
	}
	return (*m_work)(m_intermediate, assist_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Folder definition

Work* Folder::find(const QString& name_) const
{
	QFileInfo f(m_q, name_);
	if (f.exists())
		return new Work(path(), name_, *this);

	foreach (QFileInfo e, m_q.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
	{
		Work* x = Folder(e).find(name_);
		if (NULL != x)
			return x;
	}
	return NULL;
}

PRL_RESULT Folder::move(const QString& target_, Toolkit kit_) const
{
	QDir d(target_);
	foreach (QString e, m_q.entryList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System))
	{
		/* move private content to proper place */
		QFileInfo s(m_q, e), t(d, e);
		PRL_RESULT c = kit_.rename(s, t);
		if (PRL_FAILED(c))
			return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(const Reference& reference_): m_private(reference_.getHome()),
	m_joinSrc(reference_.getHome())
{
	QString p = reference_.getPrivateMark();
	QFileInfo q(QDir(m_private), p);
	if (q.exists())
	{
		m_short = q.absoluteFilePath();
		return;
	}
	std::auto_ptr<Work> y;
	QString c = reference_.getCacheMark();
	foreach (QFileInfo e, q.dir().entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
	{
		Folder f(e);
		if (NULL == m_privateWork.get())
		{
			m_privateWork.reset(f.find(p));
			if (NULL != m_privateWork.get())
				continue;
		}
		if (NULL == y.get())
			y.reset(f.find(c));
	}
	if (NULL == m_privateWork.get())
	{
		WRITE_TRACE(DBG_FATAL, "Can't find the private (file %s) in archive",
			QSTR2UTF8(p));
	}
	else if (NULL != y.get())
		m_cache.reset(new Cache(reference_, y.release()));
}

PRL_RESULT Unit::do_(const Assistant& assist_)
{
	if (!m_short.isEmpty())
	{
		QFile::remove(m_short);
		return PRL_ERR_SUCCESS;
	}
	if (NULL == m_privateWork.get())
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;

	PRL_RESULT e = (*m_privateWork)(m_private, assist_);
	if (PRL_FAILED(e) || m_cache.get() == NULL)
		return e;

	return m_cache->do_(assist_);
}

PRL_RESULT Unit::join(const QString& home_, Restore::Assembly& dst_) const
{
	dst_.addEssential(m_joinSrc, home_);
	if (NULL != m_cache.get())
		m_cache->join(dst_);

	return PRL_ERR_SUCCESS;
}

} // namespace Assembly

namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Primary

Primary::Primary(const AClient::Api& api_, const Reference& reference_):
	m_api(&api_), m_reference(reference_)
{
}

PRL_RESULT Primary::restore(const Assistant& assist_)
{
	m_vzfs.reset();
	PRL_RESULT e = assist_(m_api->restore(m_reference.getHome()), 0);
	if (PRL_FAILED(e))
		return e;

	SmartPtr<Vzfs::Assembly::Unit> u(new Vzfs::Assembly::Unit(m_reference));
	e = u->do_(assist_);
	if (PRL_FAILED(e))
		return e;

	m_vzfs = u;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Primary::assemble()
{
	if (NULL == m_vzfs.getImpl())
		return PRL_ERR_UNINITIALIZED;

	/* TODO : restore of vzcache */
	/* repair of private area */
	PRL_RESULT e = CDspService::instance()->getVzHelper()->getVzTemplateHelper().
		repair_env_private(m_reference.getHome(), m_reference.getConfig());
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "CVzTemplateHelper::repair_env_private() failer. Reason: %#x (%s)",
			e, PRL_RESULT_TO_STRING(e));
		return e;
	}
	/* we backuped Vm with running quota, but restore with stopped.
	   To fix quota file (https://jira.sw.ru/browse/PSBM-9309) will use 'vzquota off' call */
	QProcess p;
	p.start(m_reference.getQuotaOff());
	p.waitForFinished(-1);

	return e;
}

///////////////////////////////////////////////////////////////////////////////
// struct Pure

Pure::Pure(const AClient::Api& api_, const Reference& reference_)
	: Primary(api_, reference_)
{
}

PRL_RESULT Pure::restore(const Assistant& assist_)
{
	return Primary::restore(assist_);
}

PRL_RESULT Pure::assemble(const QString& home_, Restore::Assembly& dst_)
{
	PRL_RESULT e = Primary::assemble();
	if (PRL_FAILED(e))
		return e;
	return getUnit().join(home_, dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Blended

Blended::Blended(const AClient::Api& api_, const Reference& reference_,
	const SmartPtr<CVmConfiguration>& config_)
	: Primary(api_, reference_)
	, m_tempPloopSize(0)
	, m_originalHome(getReference().getHome())
{
	/**
	* Calculate required size of temporary ploop image in bytes to hold restored
	* VZFS backup. Assume that required size depends on disk quotas of backuped
	* container, and since VZFS container always has single disk extract size
	* directly from this disk. Additionally increase size using some fixed
	* coefficient (1/32) and declare minimal size (1GB).
	*/

	Backup::Perspective::archiveList_type archives =
		Backup::Perspective(config_).getCtArchives(reference_.getHome());
	if (archives.size() != 1)
		return;

	// extract size from disk and convert it from megabytes to bytes
	m_tempPloopSize = archives.first().getDevice().getSize();
	m_tempPloopSize <<= 20;

	// increase size using fixed coefficient
	const double ADDITIONAL_SIZE_COEF = (1.0 / 32);
	m_tempPloopSize += m_tempPloopSize * ADDITIONAL_SIZE_COEF;

	// check minimal size condition
	const quint64 MIN_SIZE = (1 << 30);
	if (m_tempPloopSize < MIN_SIZE)
		m_tempPloopSize = MIN_SIZE;
}

Blended::~Blended()
{
	// unmount ploop
	if (m_device.isValid())
	{
		if (PRL_FAILED(m_device->umount()))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot unmount temporary ploop %s",
				QSTR2UTF8(m_device->getName()));
		}
	}

	// delete temporary ploop directory
	if (!m_tempPloopPath.isEmpty())
		CFileHelper::ClearAndDeleteDir(m_tempPloopPath);
}

PRL_RESULT Blended::restore(const Assistant& assist_)
{
	if (m_tempPloopSize == 0)
		return PRL_ERR_FAILURE;

	if (m_device.isValid())
		return PRL_ERR_FAILURE;

	// create temporary ploop directory
	QString tempPloopPrivate;
	QString tempPloopMountPoint;
	if (m_tempPloopPath.isEmpty())
	{
		m_tempPloopPath = QString("%1.ploop").arg(getReference().getHome());
		Toolkit toolkit = assist_.getToolkit();
		if (toolkit.folderExists(m_tempPloopPath))
		{
			m_tempPloopPath.clear();
			return PRL_ERR_FAILURE;
		}

		PRL_RESULT res = toolkit.mkdir(m_tempPloopPath);
		if (PRL_FAILED(res))
		{
			m_tempPloopPath.clear();
			return res;
		}

		// create subdirectories in temporary directory
		tempPloopPrivate = QString("%1/private").arg(m_tempPloopPath);
		tempPloopMountPoint = QString("%1/mount_point").arg(m_tempPloopPath);
		toolkit.mkdir(tempPloopPrivate);
		toolkit.mkdir(tempPloopMountPoint);
	}

	// create ploop image
	m_device = Ploop::Device::make(tempPloopPrivate, m_tempPloopSize);
	if (!m_device.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot create temporary ploop image at %s",
			QSTR2UTF8(tempPloopPrivate));
		return PRL_ERR_FAILURE;
	}

	// mount ploop image
	PRL_RESULT res = m_device->mount(tempPloopMountPoint);
	if (PRL_FAILED(res))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot mount temporary ploop image to %s",
			QSTR2UTF8(tempPloopMountPoint));
		m_device.reset();
		return res;
	}

	// restore VZFS backup to temporary ploop directory
	getReference().setHome(tempPloopMountPoint);
	return Primary::restore(assist_);
}

PRL_RESULT Blended::assemble(const QString& home_, Restore::Assembly& dst_)
{
	if (!m_device.isValid())
		return PRL_ERR_UNINITIALIZED;

	PRL_RESULT e = Primary::assemble();
	if (PRL_FAILED(e))
		return e;

	// Convert restored VZFS backup to ploop format disposed at specified path.
	e = CVzOperationHelper().convert2_env(getReference().getHome(),
		m_originalHome, VZCTL_LAYOUT_5);
	if (PRL_FAILED(e))
		return e;

	getUnit().setJoinSrc(m_originalHome);
	return getUnit().join(home_, dst_);
}

/**
 * Figure out is restoring VZFS backup must be converted to Ploop layout. VZFS
 * backup of existing CT must be restored as ploop-based CT under condition of
 * CT has ploop layout at the present moment. VZFS backup of deleted CT must be
 * restored as ploop-based CT under condition of destination path resides on
 * PStorage.
 */
bool Blended::needConvertToPloop(SmartPtr<CDspClient> client_,
	const QString& sUuid_, const QString& sTargetPath_)
{
	SmartPtr<CVmConfiguration> pCtConfig =
		CDspService::instance()->getVzHelper()->getCtConfig(client_, sUuid_);
	if (pCtConfig)
	{
		// restoring of existing CT
		return (pCtConfig->getCtSettings()->getLayout() > VZCTL_LAYOUT_4);
	}
	else
	{
		// restoring of deleted CT
		return (HostUtils::GetFSType(sTargetPath_) == PRL_FS_FUSE);
	}
}

} // namespace Flavor
} // namespace Vzfs

namespace Ploop
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

Image::Image(const Backup::Archive& archive_, const Query::Work& query_):
	m_final(archive_.getImageFolder()), m_intermediate(archive_.getRestoreFolder()),
	m_query(query_), m_archive(archive_)
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

PRL_RESULT Image::do_(const Assistant& assist_)
{
	PRL_RESULT output;
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
	quint64 s = 0;
	if (PRL_FAILED(output = m_query(m_archive, assist_, s)))
		return output;

	SmartPtr<Device> device(Device::make(m_intermediate, s));
	if (!device.isValid())
		return PRL_ERR_VM_CREATE_HDD_IMG_INVALID_CREATE;

	if (PRL_FAILED(device->mount()))
		return PRL_ERR_DISK_MOUNT_FAILED;

	output = assist_(m_query.getApi().restore(m_archive,
		QFileInfo(device->getName())), m_archive.getDevice().getIndex());
	device->umount();
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

Flavor::Flavor(const QString& home_, const Backup::Perspective::archiveList_type& ve_,
	const Query::Work& query_): m_home(home_)
{
	foreach (Backup::Archive a, ve_)
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

PRL_RESULT Flavor::restore(const Assistant& assist_)
{
	if (0 != CVzOperationHelper().create_env_private(m_home))
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;

	QList<Image>::iterator p = m_imageList.begin(), e = m_imageList.end();
	for (;p != e; ++p)
	{
		PRL_RESULT e = p->do_(assist_);
		if (PRL_FAILED(e))
			return e;
	}
	return PRL_ERR_SUCCESS;
}
} // namespace Ploop
#endif // _LIN_
#endif // _CT_
} // namespace Target

///////////////////////////////////////////////////////////////////////////////
// struct Converter

void Converter::convertHardware(SmartPtr<CVmConfiguration> &cfg) const
{
	CVmHardware *pVmHardware;
	if ((pVmHardware = cfg->getVmHardwareList()) == NULL) {
		WRITE_TRACE(DBG_FATAL, "[%s] Can not get Vm hardware list", __FUNCTION__);
		return;
	}

	// Convert Cdrom devices to IDE since SATA is unsupported.
	foreach(CVmOpticalDisk* pDevice, pVmHardware->m_lstOpticalDisks) {
		if (pDevice != NULL && pDevice->getEmulatedType() == PVE::CdRomImage)
			pDevice->setInterfaceType(PMS_IDE_DEVICE);
	}

	// Convert disks to virtio-scsi
	// There's no virtio-scsi drivers for win2003-, use virtio-block.
	foreach(CVmHardDisk *pDevice, pVmHardware->m_lstHardDisks) {
		if (NULL == pDevice || pDevice->getEmulatedType() != PVE::HardDiskImage)
			continue;
		bool noSCSI = cfg->getVmSettings()->getVmCommonOptions()->getOsType() ==
				PVS_GUEST_TYPE_WINDOWS &&
			IS_WIN_VER_BELOW(cfg->getVmSettings()->getVmCommonOptions()->getOsVersion(),
			                 PVS_GUEST_VER_WIN_VISTA);
		pDevice->setInterfaceType(noSCSI ? PMS_VIRTIO_BLOCK_DEVICE : PMS_SCSI_DEVICE);
	}

	// Convert network interfaces to virtio-net
	foreach(CVmGenericNetworkAdapter *pDevice, pVmHardware->m_lstNetworkAdapters) {
		if (pDevice != NULL)
			pDevice->setAdapterType(PNT_VIRTIO);
	}

	// Parallel ports are not supported anymore
	pVmHardware->m_lstParallelPortOlds.clear();
	pVmHardware->m_lstParallelPorts.clear();
}

PRL_RESULT Converter::convertVm(const QString &vmUuid) const
{
	// Get windows driver ISO.
	QString winDriver = ParallelsDirs::getToolsImage(PAM_SERVER, PVS_GUEST_VER_WIN_2K);
	if (!QFile(winDriver).exists())
	{
		WRITE_TRACE(DBG_WARNING, "Windows drivers image does not exist: %s\n"
								 "Restoring Windows will fail.",
								 qPrintable(winDriver));
	}
	::setenv("VIRTIO_WIN", qPrintable(winDriver), 1);
	WRITE_TRACE(DBG_DEBUG, "setenv: %s=%s", "VIRTIO_WIN", qPrintable(winDriver));
	// If default bridge is not 'virbr0', virt-v2v fails.
	// So we try to find actual bridge name.
	QString bridge = getHostOnlyBridge();
	if (!bridge.isEmpty())
	{
		::setenv("LIBGUESTFS_BACKEND_SETTINGS",
				 qPrintable(QString("network_bridge=%1").arg(bridge)), 1);
		WRITE_TRACE(DBG_DEBUG, "setenv: %s=%s",
					"LIBGUESTFS_BACKEND_SETTINGS",
					qPrintable(QString("network_bridge=%1").arg(bridge)));
	}

	QStringList cmdline = QStringList()
		<< VIRT_V2V
		<< "-i" << "libvirt"
		<< "--in-place" << ::Uuid(vmUuid).toStringWithoutBrackets();

	QProcess process;
	QString out;
	if (!HostUtils::RunCmdLineUtility(cmdline.join(" "), out, V2V_RUN_TIMEOUT, &process))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot convert VM to vz7 format: virt-v2v failed:\n%s",
					process.readAllStandardError().constData());
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}
	WRITE_TRACE(DBG_DEBUG, "virt-v2v output:\n%s", qPrintable(out));

	return PRL_ERR_SUCCESS;
}

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

 Backup restore task for server

********************************************************************************/
Task_RestoreVmBackupSource::Task_RestoreVmBackupSource(
		SmartPtr<CDspDispConnection> &pDispConnection,
		const CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_ioServer(CDspService::instance()->getIOServer()),
m_bBackupLocked(false)
{
	CVmBackupRestoreCommand *pCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupRestoreCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_nBackupNumber = PRL_BASE_BACKUP_NUMBER;
	m_nFlags = pCmd->GetFlags();
	m_nTotalSize = 0;
	m_hHandle = m_pDispConnection->GetConnectionHandle();
	m_nInternalFlags = 0;
	m_nRemoteVersion = pCmd->GetVersion();
	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_RestoreVmBackupSource::~Task_RestoreVmBackupSource()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle))
		);

	// #439777 to protect call handler for destroying object
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_RestoreVmBackupSource::prepareTask()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	VmItem cVmItem;

	if (m_sBackupId.isEmpty()) {
		if (m_sVmUuid.isEmpty()) {
			WRITE_TRACE(DBG_FATAL, "[%s] Vm uuid and backup id are empty", __FUNCTION__);
			nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
			goto exit;
		}
		/* restore from last incremental of last backup if backup id does not specified (#420596) */
		BackupItem* b = getLastBaseBackup(m_sVmUuid, &getClient()->getAuthHelper(), PRL_BACKUP_CHECK_MODE_READ);
		if (NULL == b) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sVmUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm %s", QSTR2UTF8(m_sVmUuid));
			goto exit;
		}
		m_sBackupUuid = b->getUuid();
		delete b;
		QList<unsigned> lstBackupNumber;
		getPartialBackupList(m_sVmUuid, m_sBackupUuid, lstBackupNumber);
		if (lstBackupNumber.size())
			/* it will restore from last partial backup */
			m_nBackupNumber = lstBackupNumber.last();
		else
			m_nBackupNumber = PRL_BASE_BACKUP_NUMBER;
	} else {
		if (PRL_FAILED(parseBackupId(m_sBackupId, m_sBackupUuid, m_nBackupNumber))) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Invalid backup id \"%s\"", QSTR2UTF8(m_sBackupId));
			goto exit;
		}
	}

	if (m_sVmUuid.isEmpty()) {
		if (PRL_FAILED(findVmUuidForBackupUuid(m_sBackupUuid, m_sVmUuid))) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Backup \"%s\" does not exist", QSTR2UTF8(m_sBackupUuid));
			goto exit;
		}
	}

	m_sBackupRootPath = QString("%1/%2/%3").arg(getBackupDirectory()).arg(m_sVmUuid).arg(m_sBackupUuid);
	if (m_nBackupNumber == PRL_BASE_BACKUP_NUMBER)
		m_sBackupPath = QString("%1/" PRL_BASE_BACKUP_DIRECTORY).arg(m_sBackupRootPath);
	else
		m_sBackupPath = QString("%1/%2").arg(m_sBackupRootPath).arg(m_nBackupNumber);

	/* to check access before */
	if (!CFileHelper::FileCanRead(m_sBackupRootPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "User %s have not permissions for restore backup %s",
			QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sBackupUuid));
		goto exit;
	}

	/* to get vmtype */
	if (PRL_FAILED(nRetCode = loadVmMetadata(m_sVmUuid, &cVmItem)))
		goto exit;
	m_sVmName = cVmItem.getName();
	if (cVmItem.getVmType() == PVBT_CT_VZFS)
		m_nInternalFlags |= PVM_CT_VZFS_BACKUP;
	else if (cVmItem.getVmType() == PVBT_CT_PLOOP)
		m_nInternalFlags |= PVM_CT_PLOOP_BACKUP;
	else if (cVmItem.getVmType() == PVBT_CT_VZWIN)
		m_nInternalFlags |= PVM_CT_VZWIN_BACKUP;

	/* to lock backup */
	if (PRL_FAILED(nRetCode = lockShared(m_sBackupUuid)))
		goto exit;
	m_bBackupLocked = true;
#ifdef _LIN_
	/* check that the backup is not attached to some VM - otherwise we'll get
	 * acronis 'Access Denied' error */
	if (Backup::Device::Service::isAttached(m_sBackupUuid)) {
		WRITE_TRACE(DBG_FATAL, "backup '%s' is attached to VM, restore is not possible",
			QSTR2UTF8(m_sBackupUuid));
		nRetCode = PRL_ERR_BACKUP_RESTORE_PROHIBIT_WHEN_ATTACHED;
		getLastError()->setEventCode(nRetCode);
		getLastError()->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
	}
#endif // _LIN_
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

static bool excludeFunc(const QString &sRelPath)
{
	if (sRelPath == PRL_BACKUP_METADATA)
		return true; /* skip metadata */
	return false;
}

PRL_RESULT Task_RestoreVmBackupSource::run_body()
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

void Task_RestoreVmBackupSource::finalizeTask()
{
	m_cABackupServer.kill();

	if (m_bBackupLocked)
		unlockShared(m_sBackupUuid);
	m_bBackupLocked = false;

	if (PRL_FAILED(getLastErrorCode()))
		m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
}

PRL_RESULT Task_RestoreVmBackupSource::sendFiles(IOSendJob::Handle& job_)
{
	/* wait client's reply for syncronization */
	if (m_ioServer.waitForResponse(job_, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "[%s] Package reading failure", __FUNCTION__);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
/* TODO : process response retcode */
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode = getEntryLists(m_sBackupPath, excludeFunc);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(m_ioServer, m_hHandle));
	m_pVmCopySource = SmartPtr<CVmFileListCopySource>(
			new CVmFileListCopySource(
				m_pSender.getImpl(),
				m_sVmUuid,
				m_sBackupPath,
				m_nTotalSize,
				getLastError(),
				m_nTimeout));

	nRetCode = m_pVmCopySource->Copy(m_DirList, m_FileList);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupSource::sendStartReply(const SmartPtr<CVmConfiguration>& ve_, IOSendJob::Handle& job_)
{
	qulonglong nOriginalSize = 0;
	quint32 nBundlePermissions = 0;
	PRL_RESULT code = Task_BackupHelper::getBackupParams(
		m_sVmUuid, m_sBackupUuid, m_nBackupNumber, nOriginalSize, nBundlePermissions);
	if (PRL_FAILED(code))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get backup OriginalSize %x", code);
		nOriginalSize = 0;
	}

	bool ok;
	// e.g. 6.10.24168.1187340 or 7.0.200
	int major = ve_->getAppVersion().split(".")[0].toInt(&ok);
	if (!ok)
	{
		WRITE_TRACE(DBG_FATAL, "Invalid AppVersion: %s",
					qPrintable(ve_->getAppVersion()));
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}

	quint32 protoVersion = major < 7 ? BACKUP_PROTO_V3 : BACKUP_PROTO_VERSION;
	CDispToDispCommandPtr pReply = CDispToDispProtoSerializer::CreateVmBackupRestoreFirstReply(
			m_sVmUuid,
			m_sVmName,
			ve_->toString(),
			m_sBackupUuid,
			m_nBackupNumber,
			m_sBackupRootPath,
			nOriginalSize,
			nBundlePermissions,
			m_nInternalFlags,
			protoVersion);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(),
			pReply->GetCommand()->toString(),
			getRequestPackage());

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	// XXX: this job is required because the target generates the ready
	// to copy files packet using the start reply package as a source.
	// that new packet is to sync transfering sides.
	job_ = m_ioServer.sendPackage(m_hHandle, pPackage);
	if (m_ioServer.waitForSend(job_, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "[%s] Package sending failure", __FUNCTION__);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupSource::restoreVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	PRL_RESULT code;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle job;
	QString sVmConfigPath;
	SmartPtr<CVmConfiguration> pVmConfig;
	QStringList args;
	bool bConnected;

	if (!CFileHelper::DirectoryExists(m_sBackupPath, &m_pDispConnection->getUserSession()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Backup directory \"%s\" does not exist", QSTR2UTF8(m_sBackupPath));
		goto exit;
	}
	/* load Vm config and send to client (as mininal, for getVmName()) */
	sVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sBackupPath);
	pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	code = CDspService::instance()->getVmConfigManager().loadConfig(
				pVmConfig, sVmConfigPath, getClient(), false, true);
	if (PRL_FAILED(code)) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while Vm config \"%s\" loading with code [%#x][%s]",
			__FUNCTION__, QSTR2UTF8(sVmConfigPath), code, PRL_RESULT_TO_STRING(code));
		goto exit;
	}
	bConnected = QObject::connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	nRetCode = sendStartReply(pVmConfig, job);
	if (PRL_FAILED(nRetCode))
		goto exit;

	args << QString(PRL_ABACKUP_SERVER);
	if (PRL_FAILED(nRetCode = m_cABackupServer.start(args, QStringList(), m_nRemoteVersion)))
		goto exit;

	if (PRL_FAILED(nRetCode = sendFiles(job)))
	{
		m_cABackupServer.kill();
		m_cABackupServer.waitForFinished();
	}
	else
	{
		// part two : backup of hdd's via acronis library.
		nRetCode = m_cABackupServer.waitForFinished();
	}

exit:
	QObject::disconnect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)));
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_RestoreVmBackupSource::restoreCt()
{
#ifdef _CT_
	int y = 0;
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle job;
	QStringList args;
	QString sVmConfigPath;
	SmartPtr<CVmConfiguration> pConfig;
	bool bConnected;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit_0;
	}
	if (m_nInternalFlags & PVM_CT_PLOOP_BACKUP && m_nRemoteVersion < BACKUP_PROTO_V3)
	{
		WRITE_TRACE(DBG_FATAL, "Unsupported protocol version %d. >= %d is expected",
			m_nRemoteVersion, BACKUP_PROTO_V3);
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
		goto exit_0;
	}
	if (!CFileHelper::DirectoryExists(m_sBackupPath, &m_pDispConnection->getUserSession()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Backup directory \"%s\" does not exist", QSTR2UTF8(m_sBackupPath));
		goto exit_0;
	}
	sVmConfigPath = QString("%1/" VZ_CT_CONFIG_FILE).arg(m_sBackupPath);
	pConfig = CVzHelper::get_env_config_from_file(sVmConfigPath, y, 
			(0 != (m_nInternalFlags & PVM_CT_PLOOP_BACKUP)) * VZCTL_LAYOUT_5,
			true);
	if (!pConfig) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while Ct config \"%s\" loading",
			__FUNCTION__, QSTR2UTF8(sVmConfigPath));
		goto exit_0;
	}
	bConnected = QObject::connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	nRetCode = sendStartReply(pConfig, job);
	if (PRL_FAILED(nRetCode))
		goto exit_1;

	args << QString(PRL_ABACKUP_SERVER);
	if (PRL_FAILED(nRetCode = m_cABackupServer.start(args, QStringList(), m_nRemoteVersion)))
		goto exit_1;

	if (m_nInternalFlags & (PVM_CT_PLOOP_BACKUP|PVM_CT_VZWIN_BACKUP))
	{
		if (PRL_FAILED(nRetCode = sendFiles(job)))
		{
			m_cABackupServer.kill();
			m_cABackupServer.waitForFinished();
			goto exit_1;
		}
	}
	nRetCode = m_cABackupServer.waitForFinished();
exit_1:
	QObject::disconnect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)));
exit_0:
	setLastErrorCode(nRetCode);
	return nRetCode;
#else
	WRITE_TRACE(DBG_FATAL, "Linux containers does not implemented");
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

void Task_RestoreVmBackupSource::handleABackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (PRL_FAILED(Task_BackupHelper::handleABackupPackage(m_pDispConnection, p, m_nBackupTimeout)))
		m_cABackupServer.kill();
}

void Task_RestoreVmBackupSource::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_hHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	CancelOperationSupport::cancelOperation(nullClient, getRequestPackage());
	if (m_pVmCopySource.getImpl())
		m_pVmCopySource->cancelOperation();
	m_cABackupServer.kill();
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
		m_sVmUuid = Uuid::createUuid().toString();
	else if (m_sVmUuid.isEmpty())
		m_sVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();

	if (m_sVmUuid.isEmpty()) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
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

	// update Vm config watcher list
	CDspService::instance()->getVmConfigWatcher().update();

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
		return PRL_ERR_BAD_PARAMETERS;
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
	if ((state == VMS_RUNNING) || (state == VMS_PAUSED) || (state == VMS_SUSPENDED)) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_VM_RUNNING;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, sVmName, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "[%s] VM %s already exists and is running, suspended or paused",
				__FUNCTION__, QSTR2UTF8(m_sVmUuid));
		return nRetCode;
	}

	/* AccessCheck */
	bool bSetNotValid = false;
	code = CDspService::instance()->getAccessManager()
			.checkAccess(getClient(), PVE::DspCmdRestoreVmBackup, m_sVmUuid, &bSetNotValid);
	if (PRL_FAILED(code) && code != PRL_ERR_VM_CONFIG_DOESNT_EXIST) {
		nRetCode = PRL_ERR_BACKUP_ACCESS_TO_VM_DENIED;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, sVmName.isEmpty() ? m_sVmUuid : sVmName , EVT_PARAM_MESSAGE_PARAM_0));
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, PRL_RESULT_TO_STRING(code), EVT_PARAM_MESSAGE_PARAM_1));
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

	/* remove target Vm config from watcher (#448235) */
	CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(
			QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath));

	std::auto_ptr<Restore::Assembly> a;
	if (PRL_FAILED(nRetCode = restoreVmToTargetPath(a)))
		goto cleanup_0;
	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
	//Process VM uptime
	m_pVmConfig->getVmIdentification()->setVmUptimeInSeconds(m_nCurrentVmUptime);
	m_pVmConfig->getVmIdentification()->setVmUptimeStartDateTime(m_current_vm_uptime_start_date);
	// VM name should not change!
	// https://jira.sw.ru/browse/PSBM-20397
	m_pVmConfig->getVmIdentification()->setVmName(sVmName);

	// TODO Should we use CDspBugPatcherLogic here?
	// #PSBM-13394
	CDspVmNetworkHelper::updateHostMacAddresses(m_pVmConfig, NULL, HMU_NONE);
	// save config : Task_RegisterVm read config from file system
	if (PRL_FAILED(nRetCode = saveVmConfig()))
		goto cleanup_0;
	if (PRL_FAILED(nRetCode = a->do_()))
		goto cleanup_0;
	if (m_converter.get() != NULL &&
		PRL_FAILED(nRetCode = m_converter->convertVm(m_sVmUuid)))
		goto cleanup_0;
	{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem
			= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getClient()->getVmDirectoryUuid(), m_sVmUuid);
		if ( pVmDirItem )
			pVmDirItem->setTemplate( m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );
	}
cleanup_0:
	return nRetCode;
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
		if (m_converter.get() != NULL &&
			PRL_FAILED(nRetCode = m_converter->convertVm(m_sVmUuid)))
		{
			unregisterVm();
			a->revert();
		}
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

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreVmToTargetPath(std::auto_ptr<Restore::Assembly>& dst_)
{
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	::Backup::Perspective z(m_pVmConfig);
	if (z.bad()) {
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}
	PRL_RESULT nRetCode;
	Restore::Target::Vm u(m_nBackupNumber, m_sTargetPath, m_sBackupRootPath,
			Restore::Assistant(*this,
				Restore::AClient::Unit(*this, m_sOriginVmUuid, *m_pVmConfig)));
	foreach (::Backup::Archive d, z.getVmArchives(m_sTargetPath))
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
		nRetCode = Task_BackupHelper::checkFreeDiskSpace(m_sVmUuid, y.second, y.first, false);
		if (PRL_FAILED(nRetCode))
			return nRetCode;
	}
	// Stage TWO
	if (PRL_FAILED(nRetCode = u.restore()))
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
	if (m_converter.get() != NULL)
		m_converter->convertHardware(m_pVmConfig);

	if (m_pVmConfig->getVmIdentification()->getHomePath().isEmpty())
	{
		m_pVmConfig->getVmIdentification()->setHomePath(
				QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath));
	}

	dst_.reset(u.assemble(m_sTargetVmHomePath));
	return NULL == dst_.get() ? PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR : PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupTarget::restoreCt()
{
#ifdef _CT_
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

#ifdef _LIN_
	/* vzwin and ploop allow restore w/o os template */
	/* check OS template */
	if (!(m_nInternalFlags & PVM_CT_PLOOP_BACKUP)) {
		QString sOsTemplate = m_pVmConfig->getCtSettings()->getOsTemplate();
		nRetCode = CDspService::instance()->getVzHelper()->getVzTemplateHelper().is_ostemplate_exists(sOsTemplate);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "[%s] OS template %s does not found", __FUNCTION__, QSTR2UTF8(sOsTemplate));
			return nRetCode;
		}
	}
#endif

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
		pConfig = CDspService::instance()->getVzHelper()->getVzlibHelper().get_env_config(m_sVmUuid);
	}
	if (pConfig) {
		m_bVmExist = true;
		nRetCode = restoreCtOverExisting(pConfig);
	} else {
		nRetCode = restoreNewCt(sDefaultCtFolder);
	}

	CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
		m_sVmUuid, sVzDirUuid, PVE::DspCmdRestoreVmBackup, getClient());

#ifdef _LIN_
	sync();
#endif
	return nRetCode;
#else
	WRITE_TRACE(DBG_FATAL, "Containers does not implemented");
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

#ifdef _CT_
PRL_RESULT Task_RestoreVmBackupTarget::restoreCtOverExisting(const SmartPtr<CVmConfiguration> &pConfig)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sCtName;
	SmartPtr<CVmConfiguration> pNewConfig;

	/* ID, name, private : use values of existing CT, ignore m_sTargetVmHomePath & m_sTargetVmName from command
	   and values from backuped CT config */

	/* do not change bundle of existing Vm */
	if (m_sTargetVmHomePath.size()) {
		WRITE_TRACE(DBG_FATAL, "It is't possible to set target private for restore existing VM");
		return PRL_ERR_BAD_PARAMETERS;
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

	if (state == VMS_RUNNING || state == VMS_SUSPENDED || state == VMS_MOUNTED) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_VM_RUNNING;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, sCtName, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "[%s] VM %s already exists and is running, suspended or mounted",
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
		nRetCode = m_VzOpHelper.register_env(m_sTargetVmHomePath, ctId,
				pConfig->getVmIdentification()->getVmUuid(),
				PRCF_FORCE | PRVF_IGNORE_HA_CLUSTER, pNewConfig);
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
	if (ctId.isEmpty()) {
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
		nRetCode = PRL_ERR_BACKUP_CT_ID_ALREADY_EXIST;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, ctId, EVT_PARAM_MESSAGE_PARAM_0));
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
				PRCF_FORCE, pConfig);
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
		if (!sCtName.isEmpty()) {
			nRetCode = m_VzOpHelper.set_env_name(m_sVmUuid, sCtName);
			if (PRL_FAILED(nRetCode)) {
				WRITE_TRACE(DBG_FATAL, "set_env_name() exited with error %#x, %s",
						nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
				break;
			}
			pConfig->getVmIdentification()->setVmName(sCtName);
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

	Restore::AClient::Api p(m_nBackupNumber, m_sBackupRootPath);
	Restore::Target::Ct t(a, *this, &Task_RestoreVmBackupTarget::SendPkg);
	if (m_nInternalFlags & PVM_CT_PLOOP_BACKUP)
	{
		if (PRL_SUCCEEDED(output = getFiles(bIsRealMountPoint)))
		{
			Restore::Query::Work w(p, getIoClient(), m_nBackupTimeout);
			Restore::Target::Ploop::Flavor f(m_sTargetPath,
					Backup::Perspective(m_pVmConfig)
						.getCtArchives(m_sTargetPath),
					w);
			dst_.reset(t(f, m_sTargetVmHomePath));
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
#endif // _CT_

PRL_RESULT Task_RestoreVmBackupTarget::lockExclusiveVmParameters(SmartPtr<CVmDirectory::TemporaryCatalogueItem> pInfo)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	/* lock register uuid, private & name */
	nRetCode = CDspService::instance()->getVmDirManager()
				.checkAndLockNotExistsExclusiveVmParameters(QStringList(), pInfo.getImpl());
	switch (nRetCode)
	{
	case PRL_ERR_SUCCESS:
		break;
	case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
		WRITE_TRACE(DBG_FATAL, "UUID '%s' already registered", QSTR2UTF8(pInfo->vmUuid));
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, pInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
		break;

	case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
		WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(pInfo->vmXmlPath));
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, pInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, pInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
		break;

	case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
		WRITE_TRACE(DBG_FATAL, "name '%s' already registered", QSTR2UTF8(pInfo->vmName));
		getLastError()->addEventParameter(
			new CVmEventParameter( PVE::String, pInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
		break;

	case PRL_ERR_VM_ALREADY_REGISTERED:
		WRITE_TRACE(DBG_FATAL, "container '%s' already registered", QSTR2UTF8(pInfo->vmName));
		getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, pInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
		break;

	case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default
	default:
		WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
			QSTR2UTF8(pInfo->vmUuid), QSTR2UTF8(pInfo->vmName), QSTR2UTF8(pInfo->vmXmlPath));
		getLastError()->addEventParameter(
			 new CVmEventParameter( PVE::String, pInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
		getLastError()->addEventParameter(
			 new CVmEventParameter( PVE::String, pInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
		getLastError()->addEventParameter(
			 new CVmEventParameter( PVE::String, pInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
		break;
	}
	return nRetCode;
}

/* send start request for remote dispatcher and wait reply from dispatcher */
PRL_RESULT Task_RestoreVmBackupTarget::sendStartRequest()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pStartCmd;
	SmartPtr<IOPackage> pPackage;

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	pStartCmd = CDispToDispProtoSerializer::CreateVmBackupRestoreCommand(m_sVmUuid, m_sBackupId, m_nFlags);

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
		m_converter.reset(new Restore::Converter());
	m_sVmUuid = pFirstReply->GetVmUuid();
	m_sBackupUuid = pFirstReply->GetBackupUuid();
	m_nBackupNumber = pFirstReply->GetBackupNumber();
	m_sBackupRootPath = pFirstReply->GetBackupRootPath();
	m_nOriginalSize = pFirstReply->GetOriginalSize();
	m_nBundlePermissions = (m_nRemoteVersion >= BACKUP_PROTO_V2) ? pFirstReply->GetBundlePermissions() : 0;
	m_nInternalFlags = pFirstReply->GetInternalFlags();

	return nRetCode;
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
		PVD_UNREGISTER_ONLY)).wait().getResult(&evt);

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
		nRetCode = PRL_ERR_BACKUP_REGISTER_VM_FAILED;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String,
				m_pVmConfig->getVmIdentification()->getVmName(),
				EVT_PARAM_MESSAGE_PARAM_0));
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, PRL_RESULT_TO_STRING(evt.getEventCode()), EVT_PARAM_MESSAGE_PARAM_1));
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
	checkVmAdditionState( this );

	if (m_pVmCopyTarget.isValid())
		m_pVmCopyTarget->cancelOperation();

	killABackupClient();

	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

