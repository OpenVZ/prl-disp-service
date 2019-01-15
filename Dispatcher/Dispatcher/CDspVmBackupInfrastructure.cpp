///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmBackupInfrastructure.cpp
///
/// Definition of utilities to create ve backups.
///
/// @author shrike@
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

#include "CDspVmBrand.h"
#if defined(_LIN_)
#include <CDspBackupDevice.h>
#endif
#if defined(_CT_)
#include <vzctl/libvzctl.h>
#endif
#include "CDspLibvirtExec.h"
#include "CDspVmManager_p.h"
#include <Tasks/Task_CloneVm.h>
#include <Tasks/Task_CloneVm_p.h>
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "CDspVmBackupInfrastructure.h"
#include "CDspVmSnapshotInfrastructure.h"
#include <prlxmlmodel/BackupActivity/BackupActivity.h>
#include <prlcommon/VirtualDisk/PloopDisk.h>

namespace Backup
{
namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Comment

struct Comment
{
	explicit Comment(const QString& text_): m_text(&text_)
	{
	}

	const QString& getText() const
	{
		return *m_text;
	}

private:
	const QString* m_text;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reporter

Reporter::Reporter(CDspTaskHelper& task_, const QString& uuid_):
        m_uuid(uuid_), m_object(uuid_), m_task(&task_)
{
}

PRL_RESULT Reporter::warn(PRL_RESULT code_)
{
	CVmEvent e(PET_DSP_EVT_VM_MESSAGE, m_uuid, PIE_DISPATCHER, code_);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance
			(PVE::DspVmEvent, e, m_task->getRequestPackage());
	m_task->getClient()->sendPackage(p);
	return code_;
}

PRL_RESULT Reporter::fail(PRL_RESULT code_, const Comment& comment_)
{
	return CDspTaskFailure(*m_task).setCode(code_)(m_object, comment_.getText());
}

///////////////////////////////////////////////////////////////////////////////
// struct Reference

void Reference::update(const QString& name_, const QString& home_, const config_type& config_)
{
	m_home = home_;
	m_name = name_;
	m_store = QFileInfo(QDir(m_home), Uuid::createUuid().toString()).absoluteFilePath();
	m_config = config_;
}

///////////////////////////////////////////////////////////////////////////////
// struct Workbench

Workbench::Workbench(CDspTaskHelper& dspTask_, Reporter& reporter_,
	CDspDispConfigGuard& dspConfig_):
	m_tmp(dspConfig_.getDispCommonPrefs()->getBackupSourcePreferences()->getTmpDir()),
	m_reporter(&reporter_), m_dspTask(&dspTask_)
{
	if (!m_tmp.isEmpty())
	{
		m_tmp = QFileInfo(m_tmp, QString("backup.")
				.append(Uuid::createUuid().toString()))
				.absoluteFilePath();
	}
}

PRL_RESULT Workbench::openTmp(QString& dst_)
{
	if (m_tmp.isEmpty())
		return PRL_ERR_SUCCESS;

	CAuthHelper& a = getDspTask().getClient()->getAuthHelper();
	if (CFileHelper::DirectoryExists(m_tmp, &a) || !CFileHelper::WriteDirectory(m_tmp, &a))
	{
		WRITE_TRACE(DBG_FATAL, "Can't create directory \"%s\"", QSTR2UTF8(m_tmp));
		return getReporter().fail(PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY, m_tmp);
	}
	dst_ = m_tmp;
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Object

PRL_RESULT Object::lock(const QString& task_)
{
	PRL_RESULT output = m_directory->registerExclusiveVmOperation
		(m_ident.first, m_ident.second, PVE::DspCmdCreateVmBackup, m_actor, task_);
	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "registerExclusiveVmOperation failed. Reason: %#x (%s)",
			output, PRL_RESULT_TO_STRING(output));
	}
	return output;
}

PRL_RESULT Object::unlock()
{
	return m_directory->unregisterExclusiveVmOperation
		(m_ident.first, m_ident.second, PVE::DspCmdCreateVmBackup, m_actor);
}

namespace Ct
{
///////////////////////////////////////////////////////////////////////////////
// struct Object

PRL_RESULT Object::getConfig(config_type& dst_) const
{
	dst_ = m_vz->getCtConfig(getActor(), getIdent().first, QString(), true);
	if (dst_.isValid())
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s",
		QSTR2UTF8(getIdent().first));
	return PRL_ERR_RETRIEVE_VM_CONFIG;
}

PRL_RESULT Object::lock(const QString& task_)
{
	PRL_RESULT ret = ::Backup::Task::Object::lock(task_);
	if (PRL_SUCCEEDED(ret))
	{
		if (CVzHelper::lock_env(getIdent().first, "Backing up") < 0)
		{
			WRITE_TRACE(DBG_FATAL, "Can't lock Container");
			::Backup::Task::Object::unlock();
			return PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED;
		}
	}
	return ret;
}

PRL_RESULT Object::unlock()
{
	CVzHelper::unlock_env(getIdent().first, -1);
	return ::Backup::Task::Object::unlock();
}

///////////////////////////////////////////////////////////////////////////////
// struct Reference

PRL_RESULT Reference::update(const config_type& config_)
{
	if (!config_.isValid())
		return PRL_ERR_INVALID_PARAM;

	config_type x(new CVmConfiguration(config_.getImpl()));
	CVmIdentification* y = x->getVmIdentification();
	if (NULL == y)
		return PRL_ERR_UNEXPECTED;

	Task::Reference::update(y->getVmName(), y->getHomePath(), x);
	return PRL_ERR_SUCCESS;

}

///////////////////////////////////////////////////////////////////////////////
// struct Sketch

Escort::Ct Sketch::craftEscort() const
{
	return Escort::Ct(m_reference->getConfig(), m_reference->getOperations());
}

PRL_RESULT Sketch::update(const object_type& ct_, const Workbench& workbench_)
{
	SmartPtr<CVmConfiguration> c;
	PRL_RESULT e = ct_.getConfig(c);
	if (PRL_FAILED(e))
		return e;

	if (workbench_.getDspTask().operationIsCancelled())
		return workbench_.getDspTask().getCancelResult();

	return m_reference->update(c);
}

Sketch::activity_type* Sketch::craftActivity(const object_type& ct_,
		const Workbench& workbench_) const
{
	typedef Activity::Flavor<Snapshot::Shedable, Escort::Ct>
		flavor_type;
	flavor_type* f = new flavor_type
		(m_reference->getHome(), m_reference->getStore(), craftEscort());
	activity_type* output = new activity_type(f, workbench_.getDspTask().getClient(), ct_);
	output->setUuid(m_reference->getBackupUuid());
	return output;
}

} // namespace Ct

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Object

void Object::getSnapshot(QScopedPointer<Snapshot::Vm::Object>& dst_) const
{
	dst_.reset(new Snapshot::Vm::Object(getIdent().first, getActor()));
}

PRL_RESULT Object::getConfig(config_type& dst_) const
{
	PRL_RESULT output = PRL_ERR_SUCCESS;
	config_type x = getDirectory().getVmConfigByUuid(getIdent(), output, true);
	if (!x.isValid() && PRL_SUCCEEDED(output))
		output = PRL_ERR_BACKUP_INTERNAL_ERROR;
	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "Vm %s config loading failed. Reason: %#x (%s)",
			QSTR2UTF8(getIdent().first), output, PRL_RESULT_TO_STRING(output));
	}
	else
		dst_ = config_type(new CVmConfiguration(x.getImpl()));

	return output;
}

PRL_RESULT Object::getDirectoryItem(CVmDirectoryItem& dst_) const
{
	CDspLockedPointer<CVmDirectoryItem> d =
			getDirectory().getVmDirectoryItemByUuid
				(getIdent().second, getIdent().first);
	if (!d.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to find Vm with UUID '%s'",
			QSTR2UTF8(getIdent().first));
		return PRL_ERR_VM_UUID_NOT_FOUND;
	}
	dst_ = *d.getPtr();
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Reference

PRL_RESULT Reference::validate(const CVmConfiguration& tentative_)
{
	if (!tentative_.getVmIdentification()->getLinkedVmUuid().isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Backing up of linked clones is not supported");
		return PRL_ERR_UNIMPLEMENTED;
	}
	CVmHardware *h = tentative_.getVmHardwareList();
	if (NULL == h)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot get VM hardware list");
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	bool w = false;
	foreach (CVmHardDisk *pHdd, h->m_lstHardDisks)
	{
		switch (pHdd->getEmulatedType())
		{
		case PVE::RealHardDisk:
			w = true;
			break;
		case PVE::BootCampHardDisk:
			WRITE_TRACE(DBG_FATAL, "Device %s has inappropriate emulated type %d",
				QSTR2UTF8(pHdd->getUserFriendlyName()), PVE::BootCampHardDisk);
			return m_reporter->fail(PRL_ERR_VM_BACKUP_INVALID_DISK_TYPE,
				Comment(pHdd->getSystemName()));
		default:
			continue;
		}
	}
	if (w)
		m_reporter->warn(PRL_WARN_BACKUP_NON_IMAGE_HDD);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reference::update(const CVmDirectoryItem& item_, const config_type& config_)
{
	if (!config_.isValid())
		return PRL_ERR_INVALID_PARAM;

	config_type x = config_type(new CVmConfiguration(config_.getImpl()));
#if defined(_LIN_)
	::Backup::Device::Dao(x).deleteAll();
#endif
	PRL_RESULT e = validate(*x);
	if (PRL_FAILED(e))
		return e;

	Task::Reference::update(item_.getVmName(), CFileHelper::GetFileRoot(item_.getVmHome()), x);
	return PRL_ERR_SUCCESS;
}

} // namespace Vm

namespace Create
{
///////////////////////////////////////////////////////////////////////////////
// struct Sketch

Sketch::Sketch(const SmartPtr<CDspClient>& actor_,
		const SmartPtr<IOPackage>& request_):
	CDspTaskHelper(actor_, request_)
{
	setDirectory(actor_->getVmDirectoryUuid());
}

PRL_RESULT Sketch::prepareTask()
{
	PRL_RESULT output = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	do
	{
		CProtoCommandPtr a = CProtoSerializer::ParseCommand(getRequestPackage());
		if (!(a.isValid() && a->IsValid()))
			break;

		CProtoCommandWithTwoStrParams* b = CProtoSerializer
			::CastToProtoCommand<CProtoCommandWithTwoStrParams>(a);
		if (NULL == b)
			break;
		if (0 != (b->GetCommandFlags() & PBMBF_CREATE_MAP))
			m_backupUuid = b->GetSecondStrParam();

		m_ident.first = b->GetFirstStrParam();
		output = PRL_ERR_SUCCESS;
	} while(false);

	setLastErrorCode(output);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct End

PRL_RESULT End::do_(Sketch& task_)
{
	if (0 == (task_.getRequestFlags() & PEMBF_FAILURE_CLEANUP))
		return m_service->finish(task_.getIdent(), task_.getClient());
	else
		return m_service->abort(task_.getIdent(), task_.getClient());
}

void End::reportDone(Sketch& task_)
{
	SmartPtr<IOPackage> q = task_.getRequestPackage();
	CProtoCommandPtr p = CProtoSerializer
		::CreateDspWsResponseCommand(q, PRL_ERR_SUCCESS);
	task_.getClient()->sendResponse(p, q);
}

void End::reportFailure(Sketch& task_)
{
	task_.getClient()->
		sendResponseError(task_.getLastError(),
			task_.getRequestPackage());
}

namespace Begin
{
///////////////////////////////////////////////////////////////////////////////
// struct Reporter

void Reporter::reportDone(Sketch& task_)
{
	Activity::Object::Model m;
	m_service->find(task_.getIdent(), m);
	SmartPtr<IOPackage> q = task_.getRequestPackage();
	CProtoCommandPtr p = CProtoSerializer
		::CreateDspWsResponseCommand(q, PRL_ERR_SUCCESS);
	CProtoCommandDspWsResponse *r =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(p);
	r->AddStandardParam(m.toString());
	task_.getClient()->sendResponse(p, q);
}

void Reporter::reportFailure(Sketch& task_)
{
	m_service->abort(task_.getIdent(), task_.getClient());
	task_.getClient()->
		sendResponseError(task_.getLastError(),
			task_.getRequestPackage());
}

///////////////////////////////////////////////////////////////////////////////
// struct Ct

Ct::Ct(CDspVmDirHelper& dirHelper_, CDspDispConfigGuard& configGuard_,
	const SmartPtr<CDspVzHelper>& vz_): m_dirHelper(&dirHelper_),
	m_configGuard(&configGuard_), m_vz(vz_)
{
}

PRL_RESULT Ct::operator()
	(const CVmIdent& ident_, Activity::Service& service_, Sketch& task_)
{
	typedef Snapshot::Ct::Flavor::Bitmap flavor_type;

	flavor_type f(ident_.first, task_.getBackupUuid(), CVzOperationHelper());
	Activity::Ct::Builder<flavor_type> b
		(Activity::Builder(ident_, task_), f, m_vz);
	return Director(*m_dirHelper, service_, *m_configGuard)(b);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(CDspVmDirHelper& dirHelper_, CDspDispConfigGuard& configGuard_):
	m_dirHelper(&dirHelper_), m_configGuard(&configGuard_)
{
}

PRL_RESULT Vm::operator()
	(const CVmIdent& ident_, Activity::Service& service_, Sketch& task_)
{
	Activity::Vm::Builder<Snapshot::Vm::Pull::Subject> b(ident_, task_);
	if (!task_.getBackupUuid().isEmpty())
		b.setBackupUuid(task_.getBackupUuid());

	return Director(*m_dirHelper, service_, *m_configGuard)(b);
}

} // namespace Begin
} // namespace Create
} // namespace Task

namespace Snapshot
{
namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

PRL_RESULT Image::operator()(const QString &, const Product::component_type& tib_,
		const QDir& store_, QString& dst_)
{
	if (!tib_.second.isLocalFile())
		return PRL_ERR_INVALID_ARG;

	QString t = store_.absoluteFilePath(QFileInfo(tib_.second.toLocalFile()).completeBaseName());
	CFileHelper::ClearAndDeleteDir(t);

	dst_ = t;
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Ploop

PRL_RESULT Ploop::operator()(const QString &snapshot_, const Product::component_type& tib_,
		const QDir& store_, QString& dst_)
{
	if (!tib_.second.isLocalFile())
		return PRL_ERR_INVALID_ARG;

	PRL_RESULT e;
	Snapshot::Vm::Image x;
	QString f = tib_.first.getFolder();
	if (PRL_FAILED(e = x.open(f)))
		return e;

	QString t = store_.absoluteFilePath(QFileInfo(tib_.second.toLocalFile()).completeBaseName());
	CFileHelper::ClearAndDeleteDir(t);
	if (PRL_FAILED(e = x.dropState(snapshot_, t)))
		return e;

	dst_ = t;
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Mount

PRL_RESULT Mount::operator()(const QString& snapshot_, const Product::component_type& tib_,
		const QDir& store_, QString& dst_)
{
	if (!tib_.second.isLocalFile())
		return PRL_ERR_INVALID_ARG;

	QFileInfo f(QFileInfo(tib_.second.toLocalFile()));
	QString x = store_.absoluteFilePath(f.fileName());
	if (!store_.exists(f.fileName()) && !store_.mkdir(f.fileName()))
	{
		WRITE_TRACE(DBG_FATAL, "Can't create directory \"%s\"", QSTR2UTF8(x));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	QString d;
	if (0 != m_core.mount_disk_snapshot(tib_.first.getFolder(), snapshot_, m_component, x, d))
		return PRL_ERR_DISK_MOUNT_FAILED;

	dst_ = d;
	return PRL_ERR_SUCCESS;
}

} // namespace Export

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

PRL_RESULT Image::open(const QString& path_)
{
	if (m_image)
		return PRL_ERR_DOUBLE_INIT;

	m_image = QSharedPointer<VirtualDisk::Format>(
			VirtualDisk::detectImageFormat(path_));
	if (!m_image)
		return PRL_ERR_INVALID_ARG;

	m_path = path_;
	return m_image->open(path_, PRL_DISK_READ | PRL_DISK_WRITE);
}

PRL_RESULT Image::close()
{
	if (!m_image)
		return PRL_ERR_UNINITIALIZED;

	m_path.clear();
	m_image->close();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::hasSnapshot(const QString& uuid_)
{
	if (!m_image)
		return PRL_ERR_UNINITIALIZED;

/*
	SNAPTREE_ELEMENT r;
	PRL_RESULT e = m_image->GetSnapshotsTree(&r);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "IDisk::GetSnapshotsTree(%s) error : %#x( '%s' )",
			QSTR2UTF8(m_path), e, PRL_RESULT_TO_STRING(e));
		return e;
	}
	QList<SNAPTREE_ELEMENT *> x;
	x << &r;
	typedef std::list<struct __SNAPTREE_ELEMENT>::iterator iterator_type;
	for (int i = 0; i < x.size(); ++i)
	{
		SNAPTREE_ELEMENT *n = x.at(i);
		if (uuid_ == n->m_Uid.toString())
		{
			WRITE_TRACE(DBG_INFO, "Disk %s has backup's snapshot %s",
				QSTR2UTF8(m_path), QSTR2UTF8(uuid_));
			return PRL_ERR_SUCCESS;
		}
		iterator_type p = n->m_Children.begin(), e = n->m_Children.end();
		for (; p != e; ++p)
		{
			x << &(*p);
		}
	}
*/
	Q_UNUSED(uuid_);
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT Image::getBackupSnapshotId(QString& dst_)
{
	if (!m_image)
		return PRL_ERR_UNINITIALIZED;

/*
	dst_ = m_image->GetBackupUID().toString();
	if (dst_.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Empty backup's snapshot ID for disk %s",
				QSTR2UTF8(m_path));
		return PRL_ERR_NO_DATA;
	}
*/
	Q_UNUSED(dst_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::dropState(const Uuid& uuid_, const QString& target_)
{
	if (!m_image)
		return PRL_ERR_UNINITIALIZED;

	QDir d;
	d.mkdir(target_);

	return m_image->cloneState(uuid_, target_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Object

enum
{
	FREEZE_TIMEOUT_SECONDS = 300
};

Object::Object(const QString& uuid_, const actor_type& actor_):
	m_uuid(uuid_), m_actor(actor_)
{
}

PRL_RESULT Object::freeze(Task::Workbench& task_)
{
	VIRTUAL_MACHINE_STATE s = CDspVm::getVmState(m_uuid, m_actor->getVmDirectoryUuid());
	if (s != VMS_RUNNING) {
		WRITE_TRACE(DBG_FATAL, "Guest OS filesystem synchronization error: "
			 "vm state is %s", PRL_VM_STATE_TO_STRING(s));
		task_.getReporter().warn(PRL_WARN_BACKUP_GUEST_UNABLE_TO_SYNCHRONIZE);
		return PRL_WARN_BACKUP_GUEST_UNABLE_TO_SYNCHRONIZE;
	}

	PRL_RESULT output = PRL_ERR_SUCCESS;
	Libvirt::Result r = Libvirt::Kit.vms().at(m_uuid).getGuest().freezeFs();
	if (r.isFailed())
		output = r.error().code();
	if (PRL_FAILED(output) && output != PRL_ERR_OPERATION_WAS_CANCELED)
	{
		task_.getReporter().warn(PRL_WARN_BACKUP_GUEST_SYNCHRONIZATION_FAILED);
		WRITE_TRACE(DBG_FATAL, "Guest OS filesystem synchronization error %d", output);
	}
	return output;
}

PRL_RESULT Object::thaw()
{
	Libvirt::Result r = Libvirt::Kit.vms().at(m_uuid).getGuest().thawFs();
	if (r.isFailed()) {
		WRITE_TRACE(DBG_FATAL, "Failed to thaw guest OS filesystem, error %d",
				r.error().code());
		return r.error().code();
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Object::begin(const QString& path_, const QString& map_, Task::Reporter& reporter_)
{
	Q_UNUSED(path_);
	Q_UNUSED(map_);
	Q_UNUSED(reporter_);
	/* snapshot is not used in new backup scheme */
	return PRL_ERR_SUCCESS;
/*	Create "backup" snapshot */
/*	CVmEvent v;
	QString u = m_vm->getVmUuid();
	CProtoCommandPtr r = CProtoSerializer::CreateCreateSnapshotProtoCommand(
				u, QString("Snapshot for backup"), QString(),
				map_, QString(), path_, PCSF_BACKUP);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdVmCreateSnapshot, r);
	bool x = m_vm->createSnapshot(m_actor, p, &v, true);
	PRL_RESULT output = v.getEventCode();
	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while snapshot creating with code [%#x][%s]",
			output, PRL_RESULT_TO_STRING(output));
	}
	if (!x)
		output = reporter_.fail(v);
	else if (PRL_FAILED(output))
	{
		output = reporter_.fail(PRL_ERR_BACKUP_CREATE_SNAPSHOT_FAILED,
				Task::Comment(PRL_RESULT_TO_STRING(output)));
	}
	return output; */
}

PRL_RESULT Object::disband(quint32 flags_)
{
	Q_UNUSED(flags_);
	return PRL_ERR_SUCCESS;

/*	Delete "backup" snapshot */
/*	CProtoCommandPtr r =
		CProtoSerializer::CreateDeleteSnapshotProtoCommand(m_vm->getVmUuid(),
			QString(), false, PDSF_BACKUP | flags_);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdVmDeleteSnapshot, r);
	CVmEvent v;
	bool x = m_vm->deleteSnapshot(m_actor, p, &v, true);
	PRL_RESULT output = v.getEventCode();
	if (!x)
	{
		WRITE_TRACE(DBG_FATAL, "Unknown error occurred while snapshot deleting");
		if (PRL_SUCCEEDED(output))
			output = PRL_ERR_VM_DELETE_STATE_FAILED;
	}
	else if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while snapshot deleting with code [%#x][%s]",
			output, PRL_RESULT_TO_STRING(output));
	}
	return output; */
}

PRL_RESULT Object::commit(const QString& uuid_)
{
	Q_UNUSED(uuid_);
	return disband(0);
}

PRL_RESULT Object::rollback()
{
	return disband(PDSF_BACKUP_MAP);
}

namespace Push
{
///////////////////////////////////////////////////////////////////////////////
// struct Begin

Begin::Begin(const QString& tmp_, const QString& map_, Task::Workbench& task_):
	m_tmp(tmp_), m_map(map_), m_task(&task_)
{
}

PRL_RESULT Begin::doTrivial(Object& object_)
{
	return object_.begin(m_tmp, m_map, m_task->getReporter());
}

PRL_RESULT Begin::doConsistent(Object& object_)
{
	PRL_RESULT e = object_.freeze(*m_task);
	switch (e)
	{
	case PRL_ERR_SUCCESS:
		e = doTrivial(object_);
		if (PRL_ERR_OPERATION_WAS_CANCELED == object_.thaw())
		{
			object_.rollback();
			e = PRL_ERR_OPERATION_WAS_CANCELED;
		}
	case PRL_ERR_OPERATION_WAS_CANCELED:
		return e;
	default:
		return doTrivial(object_);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(const Task::Vm::Object& vm_, const Task::Vm::Reference& reference_):
	m_map(reference_.getBackupUuid()), m_vm(vm_),
	m_product(Backup::Object::Model(reference_.getConfig()), reference_.getHome()),
	m_tibList(m_product.getVmTibs())
{
}

PRL_RESULT Subject::disband(const boost::function<PRL_RESULT (Object&)>& command_)
{
	if (m_tibList.isEmpty())
		return PRL_ERR_SUCCESS;

	if (getUuid().isEmpty())
		return PRL_ERR_UNINITIALIZED;

	QScopedPointer<Object> o;
	m_vm.getSnapshot(o);

	PRL_RESULT output = command_(*o);
	if (PRL_SUCCEEDED(output))
		setUuid(QString());

	if (!m_tmp.isEmpty())
	{
		if (getUuid().isEmpty())
			CFileHelper::ClearAndDeleteDir(m_tmp);

		m_tmp.clear();
	}
	return output;
}
PRL_RESULT Subject::merge()
{
	return disband(boost::bind(&Object::commit, _1, boost::cref(getUuid())));
}

PRL_RESULT Subject::destroy()
{
	if (m_map.isEmpty())
		return merge();

	return disband(boost::bind(&Object::rollback, _1));
}

PRL_RESULT Subject::create(Task::Workbench& task_)
{
	if (m_tibList.isEmpty())
		return PRL_ERR_SUCCESS;

	if (!getUuid().isEmpty())
		return PRL_ERR_DOUBLE_INIT;

	QScopedPointer<Object> o;
	m_vm.getSnapshot(o);

	if (!m_product.getObject().canFreeze())
		return PRL_ERR_SUCCESS;

	return o->freeze(task_);
}

PRL_RESULT Subject::attach()
{
	PRL_RESULT e;
	foreach (const Product::component_type& a, m_tibList)
	{
		Image x;
		if (PRL_FAILED(e = x.open(a.first.getFolder())))
			continue;

		QString u;
		if (PRL_FAILED(e = x.getBackupSnapshotId(u)))
			continue;

		if (PRL_SUCCEEDED(e = x.hasSnapshot(u)))
		{
			setUuid(u);
			return PRL_ERR_SUCCESS;
		}
	}
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT Subject::dropState(const QDir& store_)
{
	Product::componentList_type c;
	PRL_RESULT output = Export::Complete<Export::Image>
				(getUuid(), Export::Image())
				(m_tibList, store_, c);
	setComponents(c);
	return output;
}

} // namespace Push

namespace Pull
{
namespace Mode
{
///////////////////////////////////////////////////////////////////////////////
// struct Online

PRL_RESULT Online::stop()
{
	if (m_agent.isNull())
		return PRL_ERR_UNINITIALIZED;

	Libvirt::Result e = m_agent->undefine();
	m_agent.clear();
	if (e.isFailed())
		return e.error().code();

	return PRL_ERR_SUCCESS;
}

startResult_type Online::start(const QDir& tmp_)
{
	if (!m_agent.isNull())
		return ::Error::Simple(PRL_ERR_DOUBLE_INIT);

	QList<SnapshotComponent> q;
	foreach (const Product::component_type& a, m_tibList)
	{
		if (!a.second.isLocalFile())
			return ::Error::Simple(PRL_ERR_INVALID_HANDLE);

		QString s;
		if (tmp_.isRelative())
			s = a.first.getImage() + ".xblocksnapshot";
		else
		{
			s = tmp_.absoluteFilePath
				(QFileInfo(a.second.toLocalFile()).fileName());
		}
		SnapshotComponent c;
		c.setState(s);
		c.setDevice(new CVmHardDisk(a.first.getDevice()));
		c.getDevice()->setSystemName(a.first.getImage());

		q << c;
	}
	Prl::Expected<agent_type, ::Error::Simple> x =
		Libvirt::Kit.vms().at(m_vm).getSnapshot().defineBlock(m_map, q);
	if (x.isFailed())
		return x.error();

	return m_agent = agentPointer_type(new agent_type(x.value()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Stopped

startResult_type Stopped::start(const QDir& tmp_)
{
	Libvirt::Result e = ::Command::Vm::Gear
						<
							::Command::Tag::State
							<
								::Command::Vm::Frankenstein,
								::Command::Vm::Fork::State::Strict<VMS_PAUSED>
							>
						>::run(getVm());
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Failed to start a qemu process!");
		return e.error();
	}
	startResult_type output = Online::start(tmp_);
	if (output.isFailed())
		stop_();

	return output;
}

PRL_RESULT Stopped::stop()
{
	PRL_RESULT output = Online::stop();
	if (PRL_FAILED(output))
		stop_();
	else
	{
		PRL_RESULT e = stop_();
		if (PRL_FAILED(e) && e != PRL_ERR_WRONG_VM_STATE)
			output = e;
	}
	return output;
}

PRL_RESULT Stopped::stop_()
{
	VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
	Libvirt::Kit.vms().at(getVm()).getState().getValue(s);
	if (VMS_PAUSED != s)
	{
		// check that vm is in an expected state
		// somebody may change the state from outside
		return PRL_ERR_WRONG_VM_STATE;
	}
	Libvirt::Result e = ::Command::Vm::Gear
						<
							::Command::Tag::State
							<
								::Command::Vm::Shutdown::Killer,
								::Command::Vm::Fork::State::Plural
								<
									boost::mpl::vector_c<unsigned, VMS_STOPPED, VMS_SUSPENDED>
								>
							>
						>::run(getVm());
	if (e.isFailed())
		return e.error().code();

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Freezing

startResult_type Freezing::start(const QDir& tmp_)
{
	PRL_RESULT e = m_object.freeze(m_workbench);
	if (PRL_FAILED(e))
		return Libvirt::Failure(e);

	startResult_type output = Online::start(tmp_);
	m_object.thaw();

	return output;
}

} // namespace Mode

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(const Task::Vm::Object& vm_, const Task::Vm::Reference& reference_):
	m_map(reference_.getBackupUuid()), m_vm(vm_),
	m_product(Backup::Object::Model(reference_.getConfig()), reference_.getHome()),
	m_tibList(m_product.getVmTibs())
{
}

PRL_RESULT Subject::attach()
{
	if (m_snapshot.isNull())
		return PRL_ERR_UNINITIALIZED;

	QString u;
	Libvirt::Result e = m_snapshot->getUuid(u);
	if (e.isFailed())
		return e.error().code();

	setUuid(u);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Subject::destroy()
{
	if (m_snapshot.isNull())
		return PRL_ERR_UNINITIALIZED;

	QSharedPointer<agent::Snapshot::Block> x = m_snapshot;
	merge();
	Libvirt::Result e = x->deleteMap();
	if (e.isFailed())
		return e.error().code();

	return PRL_ERR_SUCCESS;
}

void Subject::setUuid(const QString& value_)
{
	if (value_.isEmpty())
	{
		m_mode = boost::blank();
		m_snapshot.clear();
		if (!m_tmp.isEmpty())
		{
			CFileHelper::ClearAndDeleteDir(m_tmp);
			m_tmp.clear();
		}
		setComponents(Product::componentList_type());
	}
	Shedable::setUuid(value_);
}

PRL_RESULT Subject::merge()
{
	if (!m_export.isNull())
	{
		grub_type g;
		foreach (const Product::component_type& t, getComponents())
		{
			g << grub_type::value_type(getUuid(), t.first.getImage(), t.second);
		}
		Libvirt::Result e = m_export->stop(g);
		m_export.clear();
		Q_UNUSED(e);
	}
	PRL_RESULT output = boost::apply_visitor(Visitor::Stop(), m_mode);
	setUuid(QString());

	return output;
}

PRL_RESULT Subject::create(Task::Workbench& task_)
{
	QString u = m_vm.getIdent().first;
	VIRTUAL_MACHINE_STATE s = VMS_UNKNOWN;
	agent::Unit a = Libvirt::Kit.vms().at(u);
	if (a.getState().getValue(s).isFailed())
		return PRL_ERR_VM_UUID_NOT_FOUND;

	// else means a diskless VM
	if (!m_tibList.isEmpty())
	{
		Mode::Online o(u, m_tibList, m_map);
		if (VMS_STOPPED == s)
			m_mode = Mode::Stopped(o);
		else if (m_product.getObject().canFreeze())
		{
			QScopedPointer<Snapshot::Vm::Object> m;
			m_vm.getSnapshot(m);
			m_mode = Mode::Freezing(o, task_, *m);
		}
		else
			m_mode = o;

		// NB. there is no need to create a folder without a single image
		PRL_RESULT e = task_.openTmp(m_tmp);
		if (PRL_FAILED(e))
		{
			setUuid(QString());
			return e;
		}
	}
	Visitor::Start::result_type x = boost::apply_visitor(Visitor::Start(m_tmp), m_mode);
	if (x.isSucceed())
	{
		m_snapshot = x.value();
		attach();
		return PRL_ERR_SUCCESS;
	}
	setUuid(QString());

	return x.error().code();
}

PRL_RESULT Subject::dropState(const QDir& store_)
{
	Q_UNUSED(store_);
	if (getUuid().isEmpty() && !m_tibList.isEmpty())
		return PRL_ERR_UNINITIALIZED;

	grub_type g;
	foreach (const Product::component_type& t, m_tibList)
	{
		QUrl u;
		u.setScheme(QLatin1String("nbd"));
		u.setHost(QHostAddress(QHostAddress::LocalHost).toString());
		u.setPort(0);
		g << grub_type::value_type(getUuid(), t.first.getImage(), u);
	}
	typedef agent::Block::Export agent_type;
	agent_type a = Libvirt::Kit.vms().at(m_vm.getIdent().first).getExport();
	Libvirt::Result e = a.start(g);
	if (e.isFailed())
		return e.error().code();

	m_export = QSharedPointer<agent::Block::Export>(new agent::Block::Export(a));
	Prl::Expected<grub_type, ::Error::Simple> x = m_export->list();
	if (x.isFailed())
		return x.error().code();

	// NB. what if the list doesn't contain some of requested entries?
	// should we fail or may we skip?
	Product::componentList_type c;
	foreach (const Product::component_type& t, m_tibList)
	{
		foreach (grub_type::const_reference y, x.value())
		{
			if (t.first.getImage() == y.get<1>() && y.get<0>() == getUuid())
			{
				c << qMakePair(t.first, y.get<2>());
				break;
			}
		}
	}
	setComponents(c);

	return PRL_ERR_SUCCESS;
}

} // namespace Pull
} // namespace Vm

namespace Ct
{
namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Sketch


PRL_RESULT Sketch::open(const QString& uuid_, const QString& tmp_)
{
	if (!getUuid().isEmpty())
		return PRL_ERR_DOUBLE_INIT;

	QString c = VirtualDisk::Ploop::getComponentName(uuid_);
	if (0 != m_core.create_tsnapshot(m_ct, uuid_, m_map, QSTR2UTF8(c),
		tmp_.isEmpty() ? NULL : QSTR2UTF8(tmp_)))
		return PRL_ERR_BACKUP_CREATE_SNAPSHOT_FAILED;

	setUuid(uuid_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Sketch::close(bool flavor_)
{
	if (getUuid().isEmpty())
		return PRL_ERR_UNINITIALIZED;

	m_core.delete_tsnapshot(m_ct, getUuid(), flavor_);
	setUuid(QString());
	setComponents(Product::componentList_type());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Mount

Mount::Mount(const QString& ct_, const CVzOperationHelper& core_):
	Pure<Export::Mount>(ct_, Export::Mount(VirtualDisk::Ploop::getComponentName(), core_), core_)
{
}

void Mount::clean()
{
	foreach(const Product::component_type& c, getComponents())
	{
		getCore().umount_snapshot(c.second.toLocalFile());
	}
}

PRL_RESULT Mount::commit()
{
	clean();
	return close(false);
}

PRL_RESULT Mount::export_(const Product::componentList_type& tibList_, const QDir& store_)
{
	PRL_RESULT output = Pure<Export::Mount>::export_(tibList_, store_);
	if (PRL_FAILED(output))
	{
		clean();
		setComponents(Product::componentList_type());
	}
	return output;
}

PRL_RESULT Mount::begin(const QString& tmp_)
{
	QString u = Uuid::createUuid().toString();
	return open(u, tmp_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mountv4

Mountv4::Mountv4(const QString& ct_, const CVzOperationHelper& core_):
	Sketch(ct_, core_)
{
}

void Mountv4::clean()
{
	foreach(const QString& i, m_mounts)
		PloopImage::Image(i).umount();
}

PRL_RESULT Mountv4::commit()
{
	clean();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Mountv4::export_(const Product::componentList_type& tibList_, const QDir&)
{
	PRL_RESULT output = PRL_ERR_SUCCESS;
	Product::componentList_type c;
	foreach (const Product::component_type& a, tibList_) {
		PloopImage::Image p(a.first.getFolder());
		QString d;
		if (PRL_FAILED(output = p.getMountedDevice(d)))
			break;

		if (d.isEmpty()) {
			// ploop is not mounted case
			if (PRL_FAILED(output = p.mount(d)))
				break;
			m_mounts << a.first.getFolder();
		}
		c << qMakePair(a.first, QUrl::fromLocalFile(d));
	}

	if (PRL_FAILED(output)) {
		clean();
		setComponents(Product::componentList_type());
	} else
		setComponents(c);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Bitmap

Bitmap::Bitmap(const QString& ct_, const QString& uuid_, const CVzOperationHelper& core_):
	Pure<Export::Ploop>(ct_, Export::Ploop(), core_)
{
	setMap(uuid_);
}

PRL_RESULT Bitmap::begin(const QString& tmp_)
{
	QString u("{704718e1-2314-44c8-9087-d78ed36b0f4e}");
	return open(u, tmp_);
}

} // namespace Flavor
} // namespace Ct
} // namespace Snapshot

namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct Component

Component::Component(const CVmHardDisk& device_, const QString& image_, const QString& objectHome_):
	m_device(&device_)
{
	QFileInfo f(QDir::cleanPath(image_));
	if (!f.isAbsolute())
	{
		m_objectHome = objectHome_;
		f = QFileInfo(QDir(objectHome_).absoluteFilePath(f.filePath()));
	}
	if (f.isDir())
		m_folder = f.absoluteFilePath();
	else
		m_folder = f.absolutePath();
	m_image = f.absoluteFilePath();
}

QString Component::getRestorePath() const
{
	if (!m_objectHome.isEmpty())
		return m_image;

	QString s = m_folder;
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

///////////////////////////////////////////////////////////////////////////////
// struct Model

Model::Model(const config_type& config_): m_config(config_)
{
	if (isBad())
 		WRITE_TRACE(DBG_FATAL, "Cannot get a VE hardware list");
}

bool Model::isBad() const
{
	return !m_config.isValid() || m_config->getVmHardwareList() == NULL;
}

bool Model::canFreeze() const
{
	if (isBad())
		return false;

	if (m_config->getVmType() == PVT_CT)
		return false;
	
	//skip unsupported OSes
	unsigned int osType = m_config->getVmSettings()->getVmCommonOptions()->getOsType();
	unsigned int osVer = m_config->getVmSettings()->getVmCommonOptions()->getOsVersion();

	WRITE_TRACE(DBG_DEBUG, "osType %d, osVer %d", osType, osVer);
	if (osType == PVS_GUEST_TYPE_WINDOWS)
	{
		if (osVer != PVS_GUEST_VER_WIN_2003 &&
			osVer != PVS_GUEST_VER_WIN_VISTA &&
			osVer != PVS_GUEST_VER_WIN_2008 &&
			osVer != PVS_GUEST_VER_WIN_WINDOWS7 &&
			osVer != PVS_GUEST_VER_WIN_WINDOWS8 &&
			osVer != PVS_GUEST_VER_WIN_WINDOWS8_1 &&
			osVer != PVS_GUEST_VER_WIN_WINDOWS10 &&
			osVer != PVS_GUEST_VER_WIN_2012 &&
			osVer != PVS_GUEST_VER_WIN_2016 &&
			osVer != PVS_GUEST_VER_WIN_2019)
		{
			WRITE_TRACE(DBG_FATAL, "Windows: Suspend of HDD is not supported");
			return false;
		}
	}
	else if (osType == PVS_GUEST_TYPE_LINUX)
	{
		if (osVer == PVS_GUEST_VER_LIN_RHLES3 ||
			osVer == PVS_GUEST_VER_LIN_KRNL_24)
		{
			WRITE_TRACE(DBG_FATAL, "Linux: Suspend of HDD is not supported");
			return false;
		}
	}
	else if (osType != PVS_GUEST_TYPE_MACOS)
	{
		WRITE_TRACE(DBG_FATAL, "Other: Suspend of HDD is not supported");
		return false;
	}
	return true;
}

QList<CVmHardDisk* > Model::getImages() const
{
	QList<CVmHardDisk* > output;
	if (!isBad())
	{
		foreach (CVmHardDisk* d, m_config->getVmHardwareList()->m_lstHardDisks)
		{
			if (d->getEnabled() && (
				d->getEmulatedType() == PVE::HardDiskImage ||
				d->getEmulatedType() == PVE::ContainerHardDisk)
				&& !::Backup::Device::Details::Finding(*d).isKindOf())
			{
				output.push_back(d);
			}
		}
	}
	return output;
}

Model Model::clone(const QString& uuid_, const QString& name_) const
{
	if (isBad())
		return Model(config_type());

	config_type x(new CVmConfiguration(m_config.getImpl()));
	if (PRL_FAILED(x->m_uiRcInit))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot copy a VE config");
		return Model(config_type());
	}
	x->getVmIdentification()->setVmUuid(uuid_);
	x->getVmIdentification()->setVmName(name_);
	Clone::Sink::Flavor<Clone::Sink::Vm::General>::prepare(x);
	Model output(x);
	foreach (CVmHardDisk* d, output.getImages())
	{
		using namespace Clone::HardDisk;
		QFileInfo n(Flavor::getLocation(*d));
		if (n.isAbsolute())
			Flavor::update(*d, Flavor::getExternal(*d, name_));
	}
	return output;
}

} // namespace Object

namespace Product
{
///////////////////////////////////////////////////////////////////////////////
// struct Model

QString Model::getTibName(const QString& prototype_, const QStringList& met_) const
{
	QString output = QFileInfo(prototype_).fileName().append(m_suffix);
	while (met_.contains(output))
		output.prepend("_");

	return output;
}

// legacy method, used for BACKUP_PROTO_V3 and below
componentList_type Model::getCtTibs() const
{
	componentList_type output;
	QList<CVmHardDisk* > g = m_object.getImages();
	if (g.isEmpty())
		return output;

	QString n = g.front()->getUserFriendlyName();
	QString r = n.isEmpty() ? "root.hdd" : n;
	// NB. always name the first tib the old way to preserve compatibility
	// for restore. now all the ploop-based ves archives are private.tib.
	QFileInfo t(m_store, "private.tib");
	output << qMakePair(Object::Component(*g.front(), r, m_home),
		QUrl::fromLocalFile(t.absoluteFilePath()));
	g.pop_front();
	QStringList w;
	foreach (CVmHardDisk* h, g)
	{
		w << t.fileName();
		// NB. igor@ said that the friendly name was always
		// an absolute path.
		// NB. he lied.
		n = h->getUserFriendlyName();
		t = QFileInfo(m_store, getTibName(n, w));
		output << qMakePair(Object::Component(*h, n, m_home),
			QUrl::fromLocalFile(t.absoluteFilePath()));
	}
	return output;
}

componentList_type Model::getVmTibs() const
{
	QStringList w;
	componentList_type output;
	foreach (CVmHardDisk* h, m_object.getImages())
	{
		QString n = h->getSystemName();
		QFileInfo t(m_store, getTibName(n, w));
		output << qMakePair(Object::Component(*h, n, m_home),
			QUrl::fromLocalFile(t.absoluteFilePath()));
		w << t.fileName();
	}
	return output;
}

} // namespace Product

namespace Escort
{
///////////////////////////////////////////////////////////////////////////////
// struct Ct

PRL_RESULT Ct::extract(const QDir& store_)
{
	CVmIdentification* y = m_config->getVmIdentification();
	if (NULL == y)
		return PRL_ERR_UNEXPECTED;

	QDir h(y->getHomePath());
	CVmConfiguration x = *m_config;
	config_type g(&x, SmartPtrPolicy::DoNotReleasePointee);
#if defined(_LIN_)
	// XXX: here we are creating a copy of VM-like config and removing attached
	// backups from this copy for the sole purpose of making a CT config, that
	// would not contain attached backups (remove_disks_from_env_config()).
	// We simply don't have any other ways for removing disks from a CT config.
	::Backup::Device::Dao(g).deleteAll();
#endif
	QFileInfo c(store_, VZ_CT_CONFIG_FILE);
	QString a = h.absoluteFilePath(c.fileName()), b = c.absoluteFilePath();
	if (!QFile::copy(a, b))
	{
		WRITE_TRACE(DBG_FATAL, "cannot copy %s to the temporary store",
			QSTR2UTF8(a));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	if (m_vz.remove_disks_from_env_config(g, m_config, b))
		return PRL_ERR_BACKUP_INTERNAL_ERROR;

	Activity::Object::componentList_type f = getFiles();
	f << qMakePair(c, c.fileName());
	if (h.exists(VZ_CT_XML_CONFIG_FILE))
	{
		// PSBM XML conf
		QFileInfo m(store_, VZ_CT_XML_CONFIG_FILE);
		if (PRL_FAILED(x.saveToFile(m.absoluteFilePath())))
			return PRL_ERR_BACKUP_INTERNAL_ERROR;

		f << qMakePair(m, m.fileName());
	}
	setFiles(f);
	return PRL_ERR_SUCCESS;
}

void Ct::collect(const QDir& home_)
{
	QList<QPair<QFileInfo, QString> > top;
	QFileInfoList infos = home_.entryInfoList(QStringList("scripts"), QDir::Dirs);
	if (!infos.isEmpty())
		top.append(qMakePair(infos.front(), infos.front().fileName()));

	infos = home_.entryInfoList(QStringList("templates"), QDir::Dirs | QDir::Files);
	if (!infos.isEmpty())
	{
		if (!infos.front().isSymLink())
			top.append(qMakePair(infos.front(), infos.front().fileName()));
		else
		{
			QFileInfo y(infos.front().symLinkTarget());
			if (y.exists())
				top.append(qMakePair(y, infos.front().fileName()));
		}
	}
	Activity::Object::componentList_type y = getFiles(), z;
	QDir::Filters f = QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
			QDir::Hidden | QDir::NoSymLinks;
	for (int i = 0; i < top.size(); ++i)
	{
		QList<QPair<QFileInfo, QString> > subtree;
		subtree << top.at(i);
		QDir top(subtree.front().first.absoluteFilePath());
		for (int j = 0; j < subtree.size(); ++j)
		{
			foreach (const QFileInfo& x, QDir(subtree.at(j).first.absoluteFilePath()).entryInfoList(f))
			{
				QString r = top.dirName().append('/')
						.append(top.relativeFilePath(x.absoluteFilePath()));
				if (x.isDir())
					subtree << qMakePair(x, r);
				else
					y << qMakePair(x, r);
			}
		}
		z << subtree;
	}
	::Vm::Private::Brand b(home_.absolutePath(), NULL);
	setFiles(y << b.getFiles());
	setFolders(z << b.getFolders());
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

PRL_RESULT Vm::extract(const QDir& store_)
{
	CVmConfiguration c = *m_config;
	QFileInfo d(store_, VMDIR_DEFAULT_VM_CONFIG_FILE);
	if (PRL_FAILED(c.saveToFile(d.absoluteFilePath())))
	{
		WRITE_TRACE(DBG_FATAL, "file save to %s failed", QSTR2UTF8(d.absoluteFilePath()));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	Activity::Object::componentList_type x = getFiles();
	setFiles(x << qMakePair(d, d.fileName()));
	return PRL_ERR_SUCCESS;
}

void Vm::collect(const QDir& home_)
{
	Activity::Object::componentList_type x = getFiles(), y;
	foreach (CVmHardDisk* d, m_config->getVmHardwareList()->m_lstHardDisks)
	{
		if (PVE::RealHardDisk != d->getEmulatedType())
			continue;

		QFileInfo r(home_, d->getSystemName());
		if (r.exists())
			y << qMakePair(r, r.fileName());

		QDir f(r.absoluteFilePath());
		foreach (const QFileInfo& i, f.entryInfoList(QDir::NoDotAndDotDot|QDir::Files|QDir::NoSymLinks))
		{
			if ("xml" != i.suffix())
				continue;
			QString n = QString("%1/%2").arg(r.fileName()).arg(i.fileName());
			x << qMakePair(i, n);
		}
	}

	QString nvram = m_config->getVmSettings()->getVmStartupOptions()->getBios()->getNVRAM();
	if (!nvram.isEmpty())
	{
		QFileInfo nvram_f(home_, nvram);
		x << qMakePair(nvram_f, nvram_f.fileName());
	}

	QDir h(home_);
	foreach(const QFileInfo& i, h.entryInfoList(QDir::NoDotAndDotDot|QDir::Files|QDir::NoSymLinks))
	{
		if (i.fileName().startsWith("parallels.log"))
			x << qMakePair(i, i.fileName());
	}

	QFileInfo s(home_, "scripts");
	if (s.exists())
	{
		QDir d(s.absoluteFilePath());
		y << qMakePair(s, s.fileName());
		foreach(const QFileInfo& t, d.entryInfoList(QDir::NoDotAndDotDot|QDir::Files|QDir::NoSymLinks))
		{
			x << qMakePair(t, QString("%1/%2").arg(s.fileName()).arg(t.fileName()));
		}
	}
	::Vm::Private::Brand b(home_.absolutePath(), NULL);
	setFiles(x << b.getFiles());
	setFolders(y << b.getFolders());
}

} // namespace Escort

namespace Activity
{
namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct Model

QString Model::toString() const
{
	BackupEscort *e = new BackupEscort();
	if (!getEscort().getFiles().isEmpty())
	{
		EscortFilesList* l = new EscortFilesList();
		foreach (const component_type& f, getEscort().getFiles())
		{
			EscortComponent* x = new EscortComponent();
			x->setSource(f.first.absoluteFilePath());
			x->setTarget(f.second);
			l->m_lstComponent << x;
		}
		e->setFiles(l);
	}
	if (!getEscort().getFolders().isEmpty())
	{
		EscortFoldersList* l = new EscortFoldersList();
		foreach (const component_type& d, getEscort().getFolders())
		{
			EscortComponent* x = new EscortComponent();
			x->setSource(d.first.absoluteFilePath());
			x->setTarget(d.second);
			l->m_lstComponent << x;
		}
		e->setFolders(l);
	}
	BackupSnapshot *s = new BackupSnapshot();
	s->setUuid(getSnapshot().getUuid());
	foreach (const Product::component_type& c, getSnapshot().getComponents())
	{
		SnapshotComponent* x = new SnapshotComponent();
		if (c.second.isLocalFile())
			x->setState(c.second.toLocalFile());
		else
			x->setState(c.second.toString());

		x->setDevice(new CVmHardDisk(c.first.getDevice()));
		s->m_lstSnapshotComponents << x;
	}
	BackupActivity a;
	a.setUuid(getUuid());
	a.setEscort(e);
	a.setSnapshot(s);
	return a.toString();
}

} // namespace Object

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Facade

Facade::~Facade()
{
}

} // namespace Vm

namespace Store
{
///////////////////////////////////////////////////////////////////////////////
// struct Fetch

bool Fetch::operator()(Ct::map_type::const_reference ct_)
{
	if (m_actor != ct_.second->getActor()->getClientHandle())
		return false;

	m_ct = ct_.second;
	return true;
}

bool Fetch::operator()(Vm::map_type::const_reference vm_)
{
	if (m_actor != vm_.second->getActor()->getClientHandle())
		return false;

	m_vm = vm_.second;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct Abort

void Abort::operator()() const
{
	if (!getVm().isNull())
		getVm()->abort();
	else if (!getCt().isNull())
		getCt()->abort();
}

} // namespace Store

///////////////////////////////////////////////////////////////////////////////
// struct Service

PRL_RESULT Service::start(Task::Vm::Object vm_,
	Traits::Vm<Snapshot::Vm::Pull::Subject>::task_type task_)
{
	QScopedPointer<Traits::Vm<Snapshot::Vm::Pull::Subject>::snapshot_type> s;
	PRL_RESULT e = task_.getSnapshot(vm_, s);
	if (PRL_FAILED(e))
		return e;

	if (PRL_SUCCEEDED(s->attach()) && PRL_FAILED(e = s->destroy()))
		return e;

	QScopedPointer<Traits::Vm<Snapshot::Vm::Pull::Subject>::activity_type> a;
	e = task_.getActivity(vm_, a);
	if (PRL_FAILED(e))
		return e;

	e = s->create(task_);
	if (PRL_FAILED(e))
			return e;

	if (task_.getDspTask().operationIsCancelled())
	{
		s->destroy();
		return PRL_ERR_OPERATION_WAS_CANCELED;
	}
	e = a->start(s.take());
	if (PRL_FAILED(e))
		return e;

	QMutexLocker g(&m_mutex);
	m_vmActivities[vm_.getIdent()] = Vm::map_type::mapped_type(a.take());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Service::start(Task::Vm::Object vm_,
	Traits::Vm<Snapshot::Vm::Push::Subject>::task_type task_)
{
	QScopedPointer<Traits::Vm<Snapshot::Vm::Push::Subject>::snapshot_type> s;
	PRL_RESULT e = task_.getSnapshot(vm_, s);
	if (PRL_FAILED(e))
		return e;

	QScopedPointer<Traits::Vm<Snapshot::Vm::Push::Subject>::activity_type> a;
	e = task_.getActivity(vm_, a);
	if (PRL_FAILED(e))
		return e;

	if (task_.getDspTask().operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	e = a->start(s.take());
	if (PRL_FAILED(e))
		return e;

	QMutexLocker g(&m_mutex);
	m_vmActivities[vm_.getIdent()] = Vm::map_type::mapped_type(a.take());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Service::find(const CVmIdent& ident_, Activity::Object::Model& dst_) const
{
	Store::Extract e(dst_);
	QMutexLocker g(&m_mutex);
	return find(ident_, e);
}

PRL_RESULT Service::finish(const CVmIdent& ident_, const actor_type& actor_)
{
	Store::Finish f(actor_, m_tracker);
	m_mutex.lock();
	PRL_RESULT output = erase(ident_, f);
	m_mutex.unlock();
	if (PRL_SUCCEEDED(output) && !f.getResult())
		output = PRL_ERR_ACCESS_DENIED;

	return output;
}

PRL_RESULT Service::track(const CVmIdent& ident_, const actor_type& actor_)
{
	Store::Track t(actor_, m_tracker);
	QMutexLocker g(&m_mutex);
	find(ident_, t);
	return t.getResult();
}

void Service::abort(IOSender::Handle actor_)
{
	m_mutex.lock();
	QList<Store::Abort> a;
	tracker_type::iterator p = m_tracker.find(actor_);
	while (m_tracker.end() != p && p.key() == actor_)
	{
		Store::Abort f(actor_);
		erase(p.value(), f);
		p = m_tracker.erase(p);
		a << f;
	}
	m_mutex.unlock();
	foreach (const Store::Abort& u, a)
	{
		u();
	}
}

PRL_RESULT Service::abort(const CVmIdent& ident_, const actor_type& actor_)
{
	if (actor_.isValid())
	{
		Store::Abort a(actor_->getClientHandle());
		m_mutex.lock();
		find(ident_, a);
		m_mutex.unlock();
		a();
	}
	return finish(ident_, actor_);
}

template<class T>
PRL_RESULT Service::find(const CVmIdent& ident_, T& t_) const
{
	ctMap_type::const_iterator t = m_ctActivities.find(ident_);
	vmMap_type::const_iterator m = m_vmActivities.find(ident_);
	if (m_ctActivities.end() != t)
		t_(*t);
	else if (m_vmActivities.end() != m)
		t_(*m);
	else
		return PRL_ERR_FILE_NOT_FOUND;

	return PRL_ERR_SUCCESS;
}

template<class T>
PRL_RESULT Service::erase(const CVmIdent& ident_, T& t_)
{
	ctMap_type::iterator t = m_ctActivities.find(ident_);
	vmMap_type::iterator m = m_vmActivities.find(ident_);
	if (m_ctActivities.end() != t)
	{
		if (t_(*t))
			m_ctActivities.erase(t);
	}
	else if (m_vmActivities.end() != m)
	{
		if (t_(*m))
			m_vmActivities.erase(m);
	}
	else
		return PRL_ERR_FILE_NOT_FOUND;

	return PRL_ERR_SUCCESS;
}

} // namespace Activity

namespace Metadata
{
///////////////////////////////////////////////////////////////////////////////
// struct Lock

PRL_RESULT Lock::grabShared(const QString& sequence_)
{
	PRL_ASSERT(!sequence_.isEmpty());
	QMutexLocker g(&m_mutex);

	if (m_exclusive.contains(sequence_))
	{
		WRITE_TRACE(DBG_FATAL, "Backup %s has already been locked for writing",
			QSTR2UTF8(sequence_));
		return PRL_ERR_BACKUP_LOCKED_FOR_WRITING;
	}
	m_shared.insert(sequence_, QThread::currentThread());
	return PRL_ERR_SUCCESS;
}

void Lock::releaseShared(const QString& sequence_)
{
	QMutexLocker g(&m_mutex);
	QMultiHash<QString, QThread* >::iterator p =
		m_shared.find(sequence_, QThread::currentThread());
	if (m_shared.end() == p)
	{
		WRITE_TRACE(DBG_FATAL, "Unlock for non-locked backup %s",
			QSTR2UTF8(sequence_));
	}
	else
		m_shared.erase(p);
}

PRL_RESULT Lock::grabExclusive(const QString& sequence_)
{
	PRL_ASSERT(!sequence_.isEmpty());
	QMutexLocker g(&m_mutex);

	if (m_exclusive.contains(sequence_))
	{
		WRITE_TRACE(DBG_FATAL, "Backup %s has already been locked for writing",
			QSTR2UTF8(sequence_));
		return PRL_ERR_BACKUP_LOCKED_FOR_WRITING;
	}
	if (m_shared.contains(sequence_))
	{
		WRITE_TRACE(DBG_FATAL, "Backup %s has already been locked for reading",
			QSTR2UTF8(sequence_));
		return PRL_ERR_BACKUP_LOCKED_FOR_READING;
	}
	m_exclusive.insert(sequence_);
	return PRL_ERR_SUCCESS;
}

void Lock::releaseExclusive(const QString& sequence_)
{
	QMutexLocker g(&m_mutex);
	if (0 == m_exclusive.remove(sequence_))
	{
		WRITE_TRACE(DBG_FATAL, "Unlock for non-locked backup %s",
			QSTR2UTF8(sequence_));
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Carcass

Carcass::Carcass(const QString& root_, const QString& ve_):
	m_root(QDir(root_).filePath(ve_))
{
}

QDir Carcass::getSequence(const QString& uuid_) const
{
	return m_root.absoluteFilePath(uuid_);
}

QDir Carcass::getItem(const QString& sequence_, quint32 number_) const
{
	if (Sequence::BASE == number_)
	{
		return QDir(getSequence(sequence_)
			.absoluteFilePath(PRL_BASE_BACKUP_DIRECTORY));
	}
	return QDir(getSequence(sequence_)
		.absoluteFilePath(QString::number(number_)));
}

///////////////////////////////////////////////////////////////////////////////
// struct Ve

QFileInfo Ve::showItem() const
{
	return m_fs.getCatalog().filePath(PRL_BACKUP_METADATA);
}

Prl::Expected<VmItem, PRL_RESULT> Ve::loadItem()
{
	if (m_cache)
		return m_cache.get();

	VmItem output;
	QString sPath = showItem().absoluteFilePath();
	PRL_RESULT e = output.loadFromFile(sPath);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL,
			"Error occurred while Vm metadata \"%s\" loading with code [%#x][%s]",
			QSTR2UTF8(sPath), e, PRL_RESULT_TO_STRING(e));
		return e;
	}
	m_cache = output;
	return output;
}

PRL_RESULT Ve::saveItem(const VmItem& value_)
{
	PRL_RESULT output = VmItem(value_).saveToFile(showItem().absoluteFilePath());
	if (PRL_SUCCEEDED(output))
		m_cache = value_;

	return output;
}

Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT>
	Ve::loadConfig(const QString& sequence_, quint32 number_)
{
	Prl::Expected<VmItem, PRL_RESULT> m = loadItem();
	if (m.isFailed())
		return m.error();

	Ct::Config::LoadOps l;

	l.setRelative();

	PRL_RESULT e = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> output;
	switch (m.value().getVmType())
	{
	case PVBT_VM:
	{
		QFile file(m_fs.getItem(sequence_, number_).filePath(VMDIR_DEFAULT_VM_CONFIG_FILE));
		output = SmartPtr<CVmConfiguration>(new(std::nothrow) CVmConfiguration);
		if (!output.isValid())
			return PRL_ERR_OUT_OF_MEMORY;
		if (PRL_FAILED(e = output->loadFromFile(&file, false)))
			WRITE_TRACE(DBG_FATAL, "Failed to load config file '%s'", QSTR2UTF8(file.fileName()));
		break;
	}
#ifdef _CT_
	case PVBT_CT_PLOOP:
		// NB. there is no break here intentionally.
		l.setLayout(VZCTL_LAYOUT_5);
	case PVBT_CT_VZFS:
	{
		int x = 0;
		QString file(m_fs.getItem(sequence_, number_).filePath(VZ_CT_CONFIG_FILE));
		output = CVzHelper::get_env_config_from_file(file, x, l);
		if (!output.isValid())
		{
			WRITE_TRACE(DBG_FATAL, "Failed to load config file '%s'", QSTR2UTF8(file));
			e = PRL_ERR_UNEXPECTED;
		}
		break;
	}
#endif // _CT_
	default:
		WRITE_TRACE(DBG_FATAL, "loading VE config for backup type %d is not implemented",
			m.value().getVmType());
		return PRL_ERR_UNEXPECTED;
	}
	if (PRL_FAILED(e))
		return e;

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Sequence

QList<quint32> Sequence::getIndex() const
{
	QList<quint32> output;
	QDir d(m_fs.getSequence(m_uuid));
	if (!d.exists())
		return output;

	foreach (const QFileInfo& e, d.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
	{
		bool ok;
		quint32 number = e.fileName().toUInt(&ok);
		if (!ok)
			continue;
		if (QFile::exists(showItem(number)))
			output << number;
	}
	std::sort(output.begin(), output.end());
	if (showItemLair(BASE).exists())
		output.prepend(BASE);

	return output;
}

Prl::Expected<BackupItem, PRL_RESULT> Sequence::getHeadItem(quint32 at_) const
{
	BackupItem output;
	output.setDateTime(QDateTime(QDate(1970, 1, 1)));
	QString sPath = showItem(at_);
	PRL_RESULT e = output.loadFromFile(sPath);
	if (PRL_FAILED(e))
	{
		if (QFile::exists(sPath))
		{
			WRITE_TRACE(DBG_FATAL,
				"Cannot load backup \"%s\" metadata with code [%#x][%s]",
				QSTR2UTF8(sPath), e, PRL_RESULT_TO_STRING(e));
		}
		else
		{
			WRITE_TRACE(DBG_FATAL,
				"backup metadata file %s does not exist", QSTR2UTF8(sPath));
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
		}
		// in any case set backup uuid
		output.setUuid(m_uuid);
		output.setId(m_uuid);
	}
	output.setType(PRL_BACKUP_FULL_TYPE);
	if (PRL_FAILED(e))
		return e;

	return output;
}

Prl::Expected<PartialBackupItem, PRL_RESULT> Sequence::getTailItem(quint32 at_) const
{
	PartialBackupItem output;
	output.setDateTime(QDateTime(QDate(1970, 1, 1)));
	QString sPath = showItem(at_);
	PRL_RESULT e = output.loadFromFile(sPath);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL,
			"Error occurred while backup metadata \"%s\" loading with code [%#x][%s]",
			QSTR2UTF8(sPath), e, PRL_RESULT_TO_STRING(e));
		// in any case set backup uuid
		output.setNumber(at_);
		output.setId(QString("%1.%2").arg(m_uuid).arg(at_));
	}
	output.setType(PRL_BACKUP_INCREMENTAL_TYPE);

	if (PRL_FAILED(e))
		return e;

	return output;
}

Prl::Expected<CBackupDisks, PRL_RESULT> Sequence::getDisks(quint32 at_)
{
	Ve u(m_fs);
	Prl::Expected<VmItem, PRL_RESULT> m = u.loadItem();
	if (m.isFailed())
		return m.error();

	Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> x =
		u.loadConfig(m_uuid, at_);

	if (x.isFailed())
		return x.error();

	x.value()->setRelativePath();
	// XXX: empty home doesn't work here, so generate some random cookie
	QString home("/" + Uuid::createUuid().toString());
	Backup::Product::Model p(Backup::Object::Model(x.value()), home);
	Backup::Product::componentList_type archives;
	if (BACKUP_PROTO_V4 <= m.value().getVersion())
	{
		p.setSuffix(::Backup::Suffix(at_)());
		archives = p.getVmTibs();
	}
	else if (m.value().getVmType() == PVBT_VM)
		archives = p.getVmTibs();
	else
		archives = p.getCtTibs();

	CBackupDisks output;
	foreach(const Backup::Product::component_type& a, archives)
	{
		if (!a.second.isLocalFile())
			return PRL_ERR_UNEXPECTED;

		CBackupDisk *b = new (std::nothrow) CBackupDisk;
		if (!b)
			return PRL_ERR_OUT_OF_MEMORY;
		b->setName(QFileInfo(a.second.toLocalFile()).fileName());
		QFileInfo fi(a.first.getImage());
		// use relative paths for disks that reside in the VM home directory
		b->setOriginalPath(fi.dir() == home ? fi.fileName() : fi.filePath());
		b->setSize(a.first.getDevice().getSize() << 20);
		CVmHddEncryption *e = a.first.getDevice().getEncryption();
		if (e)
			b->setEncryption(new CVmHddEncryption(e));
		output.m_lstBackupDisks << b;
	}
	return output;
}

PRL_RESULT Sequence::save(const BackupItem& value_, quint32 at_)
{
	return BackupItem(value_).saveToFile(showItem(at_));
}

PRL_RESULT Sequence::create(const PartialBackupItem& value_, quint32 at_)
{
	PRL_RESULT e = update(value_, at_);
	if (PRL_FAILED(e))
		return e;

	quint32 i = getIndex().first();
	Prl::Expected<BackupItem, PRL_RESULT> h = getHeadItem(i);
	if (h.isFailed())
		return h.error();

	h.value().setLastNumber(at_);
	return save(h.value(), i);
}

PRL_RESULT Sequence::update(const PartialBackupItem& value_, quint32 at_)
{
	return PartialBackupItem(value_).saveToFile(showItem(at_));
}

PRL_RESULT Sequence::remove(quint32 at_)
{
	QDir d = m_fs.getItem(m_uuid, at_);
	QString p = d.absolutePath();
	if (!d.cdUp())
		return PRL_ERR_FILE_NOT_FOUND;
	if (!CFileHelper::ClearAndDeleteDir(p))
		return PRL_ERR_FAILURE;
	if (getIndex().isEmpty() && !CFileHelper::ClearAndDeleteDir(d.absolutePath()))
		return PRL_ERR_FAILURE;

	return PRL_ERR_SUCCESS;
}

QString Sequence::showItem(quint32 at_) const
{
	return m_fs.getItem(m_uuid, at_).filePath(PRL_BACKUP_METADATA);
}

///////////////////////////////////////////////////////////////////////////////
// struct Catalog

Sequence Catalog::getSequence(const QString& at_) const
{
	return Sequence(at_, getFs());
}

QFileInfoList Catalog::getSequences() const
{
	QFileInfoList output;
	if (!showItem().exists())
		return output;

	foreach (const QFileInfo& e, getFs().getCatalog()
		.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
	{
		if (Uuid::isUuid(e.fileName()))
			output << e;
	}
	return output;
}

QStringList Catalog::getIndexForRead(CAuthHelper* auth_) const
{
	QStringList output;
	QFileInfoList x = getSequences();
	if (NULL == auth_)
	{
		foreach (const QFileInfo& e, x)
		{
			output << e.fileName();
		}
	}
	else
	{
		foreach (const QFileInfo& e, x)
		{
			if (CFileHelper::FileCanRead(e.absoluteFilePath(), auth_))
				output << e.fileName();
		}
	}
	return output;
}

QStringList Catalog::getIndexForWrite(CAuthHelper& auth_) const
{
	QStringList output;
	foreach (const QFileInfo& e, getSequences())
	{
		if (CFileHelper::FileCanWrite(e.absoluteFilePath(), &auth_))
			output << e.fileName();
	}
	return output;
}

} // namespace Metadata
} // namespace Backup

