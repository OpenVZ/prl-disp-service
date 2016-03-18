///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmBackupInfrastructure.cpp
///
/// Definition of utilities to create ve backups.
///
/// @author shrike@
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

#if defined(_LIN_)
#include <CDspBackupDevice.h>
#endif
#include <Tasks/Task_CloneVm_p.h>
#include "CDspVmBackupInfrastructure.h"
#include "CDspVmSnapshotInfrastructure.h"
#include <prlxmlmodel/BackupActivity/BackupActivity.h>
//#include <Libraries/VirtualDisk/DiskStatesManager.h>
//#include <Libraries/VirtualDisk/PrlDiskDescriptor.h>

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
	dst_ = m_vz->getCtConfig(getActor(), getIdent().first);
	if (dst_.isValid())
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s",
		QSTR2UTF8(getIdent().first));
	return PRL_ERR_RETRIEVE_VM_CONFIG;
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

PRL_RESULT Object::getSnapshot(PVE::IDispatcherCommands command_, const QString& task_,
			QScopedPointer<Snapshot::Vm::Object>& dst_) const
{
	bool x = false;
	PRL_RESULT output = PRL_ERR_SUCCESS;
	SmartPtr<CDspVm> m = CDspVm::CreateInstance(
				getIdent().first, getIdent().second,
				output, x, getActor(), command_, task_);
	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while CDspVm::CreateInstance() with code [%#x][%s]",
			output, PRL_RESULT_TO_STRING(output));
	}
	else if (!m.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Unknown CDspVm::CreateInstance() error");
		output = PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	else
		dst_.reset(new Snapshot::Vm::Object(m, x, getActor()));

	return output;
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

///////////////////////////////////////////////////////////////////////////////
// struct Subject

PRL_RESULT Subject::update(const object_type& vm_, const Workbench& workbench_)
{
	CVmDirectoryItem d;
	PRL_RESULT e = vm_.getDirectoryItem(d);
	if (PRL_FAILED(e))
		return e;

	SmartPtr<CVmConfiguration> c;
	e = vm_.getConfig(c);
	if (PRL_FAILED(e))
		return e;

	workbench_.getReporter().nameObject(d.getVmName());
	if (workbench_.getDspTask().operationIsCancelled())
		return workbench_.getDspTask().getCancelResult();

	return m_reference->update(d, c);
}

Subject::activity_type* Subject::craftActivity(const object_type& vm_,
			const Workbench& workbench_) const
{
	return new activity_type(vm_, *m_reference, workbench_.getDspTask().getClient());
}

Subject::snapshot_type* Subject::craftSnapshot(const object_type& vm_) const
{
	return new snapshot_type(vm_, *m_reference);
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
	Activity::Vm::Builder b(ident_, task_);
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

PRL_RESULT Image::operator()(const QString& snapshot_, const Product::component_type& tib_,
		const QDir& store_, QString& dst_)
{
	PRL_RESULT e;
	Snapshot::Vm::Image x;
	QString f = tib_.first.getFolder();
	if (PRL_FAILED(e = x.open(f)))
		return e;

	QString t = store_.absoluteFilePath(tib_.second.completeBaseName());
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
	QString x = store_.absoluteFilePath(tib_.second.fileName()), d;
	if (!store_.exists(tib_.second.fileName()) && !store_.mkdir(tib_.second.fileName()))
	{
		WRITE_TRACE(DBG_FATAL, "Can't create directory \"%s\"", QSTR2UTF8(x));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
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
	if (m_image.isValid())
		return PRL_ERR_DOUBLE_INIT;

/*
	PRL_RESULT e;
	IDisk* d = IDisk::OpenDisk(path_, PRL_DISK_NO_ERROR_CHECKING | PRL_DISK_READ | PRL_DISK_FAKE_OPEN, &e);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "IDisk::OpenDisk(%s) error : %#x( '%s' )",
			QSTR2UTF8(path_), e, PRL_RESULT_TO_STRING(e));
		return e;
	}
	if (NULL == d)
	{
		WRITE_TRACE(DBG_FATAL, "IDisk::OpenDisk(%s) return invalid pointer",
			QSTR2UTF8(path_));
		return PRL_ERR_UNEXPECTED;
	}
	m_path = path_;
	m_image = SmartPtr<IDisk>(d, &release);
*/
	m_path = path_;
	m_image = SmartPtr<IDisk>(NULL, &release);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::close()
{
	if (!m_image.isValid())
		return PRL_ERR_UNINITIALIZED;

	m_path.clear();
	m_image.reset();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Image::hasSnapshot(const QString& uuid_)
{
	if (!m_image.isValid())
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
	if (!m_image.isValid())
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
	if (!m_image.isValid())
		return PRL_ERR_UNINITIALIZED;

	Q_UNUSED(uuid_);
	Q_UNUSED(target_);
	return PRL_ERR_SUCCESS;
/*
	CDSManager m;
	m.AddDisk(m_image.getImpl());
	PRL_RESULT output = m.CloneState(uuid_, QStringList() << target_, NULL, NULL);
	m.WaitForCompletion();
	m.Clear();
	return output;
*/
}

void Image::release(IDisk* value_)
{
	if (NULL != value_)
		Q_UNUSED(value_);
/*
		value_->Release();
*/
}

///////////////////////////////////////////////////////////////////////////////
// struct Object

enum
{
	FREEZE_TIMEOUT_SECONDS = 300
};

Object::Object(const SmartPtr<CDspVm>& vm_, bool unregister_, const actor_type& actor_):
	m_unregister(unregister_), m_vm(vm_), m_actor(actor_)
{
}

Object::~Object()
{
	if (m_unregister)
		CDspVm::UnregisterVmObject(m_vm);
}

PRL_RESULT Object::freeze(Task::Workbench& task_)
{
	VIRTUAL_MACHINE_STATE s = m_vm->getVmState();
	switch (s)
	{
	case VMS_PAUSED:
		WRITE_TRACE(DBG_FATAL,
			"Guest OS filesystem synchronization error: vm is paused");
		task_.getReporter().warn(PRL_WARN_BACKUP_GUEST_UNABLE_TO_SYNCHRONIZE);
		return PRL_WARN_BACKUP_GUEST_UNABLE_TO_SYNCHRONIZE;
	case VMS_SUSPENDED:
		task_.getReporter().warn(PRL_WARN_BACKUP_GUEST_UNABLE_TO_SYNCHRONIZE);
		WRITE_TRACE(DBG_FATAL,
			"Guest OS filesystem synchronization error: vm is suspended");

		return PRL_WARN_BACKUP_GUEST_UNABLE_TO_SYNCHRONIZE;
	case VMS_RUNNING:
		break;
	default:
		return PRL_ERR_UNIMPLEMENTED;
	}
	::Snapshot::Unit u(m_vm, task_.getDspTask(), s);
	Q_UNUSED(u);
	PRL_RESULT output = PRL_ERR_UNIMPLEMENTED;
/*
	PRL_RESULT output = u.pokeGuest(PVE::DspCmdVmGuestSuspendHardDisk, FREEZE_TIMEOUT_SECONDS);
*/
	if (PRL_FAILED(output) && output != PRL_ERR_OPERATION_WAS_CANCELED)
	{
		task_.getReporter().warn(PRL_WARN_BACKUP_GUEST_SYNCHRONIZATION_FAILED);
		WRITE_TRACE(DBG_FATAL, "Guest OS filesystem synchronization error %d", output);
	}
	return output;
}

PRL_RESULT Object::thaw(Task::Workbench& task_)
{
	::Snapshot::Unit u(m_vm, task_.getDspTask(), m_vm->getVmState());
	Q_UNUSED(u);
	return PRL_ERR_UNIMPLEMENTED;
/*
	return u.pokeGuest(PVE::DspCmdVmGuestResumeHardDisk, FREEZE_TIMEOUT_SECONDS);
*/
}

PRL_RESULT Object::begin(const QString& path_, const QString& map_, Task::Reporter& reporter_)
{
	Q_UNUSED(path_);
	Q_UNUSED(map_);
	Q_UNUSED(reporter_);
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
		if (PRL_ERR_OPERATION_WAS_CANCELED == object_.thaw(*m_task))
		{
			Rollback()(object_);
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

template<class T>
PRL_RESULT Subject::disband(T command_)
{
	if (m_tibList.isEmpty())
		return PRL_ERR_SUCCESS;

	if (getUuid().isEmpty())
		return PRL_ERR_UNINITIALIZED;

	QScopedPointer<Object> o;
	PRL_RESULT output = m_vm.getSnapshot(PVE::DspCmdVmDeleteSnapshot, QString(), o);
	if (PRL_FAILED(output))
		return output;

	if (PRL_SUCCEEDED(output = command_(*o)))
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
	return disband(Commit(getUuid()));
}

PRL_RESULT Subject::destroy()
{
	if (m_map.isEmpty())
		return merge();

	return disband(Rollback());
}

PRL_RESULT Subject::create(Task::Workbench& task_)
{
	if (m_tibList.isEmpty())
		return PRL_ERR_SUCCESS;

	if (!getUuid().isEmpty())
		return PRL_ERR_DOUBLE_INIT;

	QScopedPointer<Object> o;
	PRL_RESULT output = m_vm.getSnapshot(PVE::DspCmdVmCreateSnapshot,
				task_.getDspTask().getJobUuid(), o);
	if (PRL_FAILED(output))
		return output;

	output = task_.openTmp(m_tmp);
	if (PRL_FAILED(output))
		return output;

	Begin b(m_tmp, m_map, task_);
	if (m_product.getObject().canFreeze())
		output = b.doConsistent(*o);
	else
		output = b.doTrivial(*o);

	if (PRL_SUCCEEDED(output))
		attach();
	else if (!m_tmp.isEmpty())
	{
		CFileHelper::ClearAndDeleteDir(m_tmp);
		m_tmp.clear();
	}
	return output;
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
	if (getUuid().isEmpty() && !m_tibList.isEmpty())
		return PRL_ERR_UNINITIALIZED;

	Product::componentList_type c;
	PRL_RESULT output = Export::Complete<Export::Image>
				(getUuid(), Export::Image())
				(m_tibList, store_, c);
	setComponents(c);
	return output;
}

} // namespace Vm

namespace Ct
{
namespace Flavor
{
///////////////////////////////////////////////////////////////////////////////
// struct Sketch

const char Sketch::s_component[] = "prl_backup";

PRL_RESULT Sketch::open(const QString& uuid_, const QString& tmp_)
{
	if (!getUuid().isEmpty())
		return PRL_ERR_DOUBLE_INIT;

	QString c = QString(s_component).append(".").append(uuid_);
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

/*
	m_core.delete_tsnapshot(m_ct, getUuid(), flavor_);
*/
	Q_UNUSED(flavor_);
	m_core.delete_tsnapshot(m_ct, getUuid());
	setUuid(QString());
	setComponents(Product::componentList_type());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Mount

Mount::Mount(const QString& ct_, const CVzOperationHelper& core_):
	Pure<Export::Mount>(ct_, Export::Mount(s_component, core_), core_)
{
}

void Mount::clean()
{
	foreach(const Product::component_type& c, getComponents())
	{
		getCore().umount_snapshot(c.second.absoluteFilePath());
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
// struct Bitmap

Bitmap::Bitmap(const QString& ct_, const QString& uuid_, const CVzOperationHelper& core_):
	Pure<Export::Image>(ct_, Export::Image(), core_)
{
	setMap(uuid_);
}

PRL_RESULT Bitmap::begin(const QString& tmp_)
{
/*
	QString u = PrlDiskDescriptor().DescriptorBackupUID().toString();
*/
	QString u;
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
		f = QFileInfo(QDir(objectHome_), f.fileName());
	}
	if (f.isDir())
		m_folder = f.absoluteFilePath();
	else
		m_folder = f.absolutePath();
}

QString Component::getRestoreFolder() const
{
	if (!m_objectHome.isEmpty())
		return m_folder;

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
			osVer != PVS_GUEST_VER_WIN_2012)
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

QString Model::getTibName(const QString& prototype_, const QStringList& met_)
{
	QString output = QFileInfo(prototype_).fileName().append(".tib");
	while (met_.contains(output))
		output.prepend("_");

	return output;
}

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
	output << qMakePair(Object::Component(*g.front(), r, m_home), t);
	g.pop_front();
	QStringList w;
	foreach (CVmHardDisk* h, g)
	{
		w << output.last().second.fileName();
		// NB. igor@ said that the friendly name was always
		// an absolute path.
		// NB. he lied.
		n = h->getUserFriendlyName();
		t = QFileInfo(m_store, getTibName(n, w));
		output << qMakePair(Object::Component(*h, n, m_home), t);
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
		output << qMakePair(Object::Component(*h, n, m_home), t);
		w << output.last().second.fileName();
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
	setFiles(y);
	setFolders(z);
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
	setFiles(x);
	setFolders(y);
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
		x->setState(c.second.absoluteFilePath());
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
// class Unit

Unit::Unit(const Task::Vm::Object& vm_, const Task::Vm::Reference& task_,
		const actor_type& actor_):
	base_type(new Flavor<Snapshot::Vm::Subject, Escort::Vm>
			(task_.getHome(), task_.getStore(),
			Escort::Vm(task_.getConfig())),
			actor_, vm_),
	m_config(task_.getConfig())
{
	setUuid(task_.getBackupUuid());
}

PRL_RESULT Unit::start(Snapshot::Vm::Subject* snapshot_)
{
	if (m_config.isValid())
	{
		CDspVmDirHelper::UpdateHardDiskInformation(m_config);
		m_config->setRelativePath();
		m_config = config_type();
	}
	return base_type::start(snapshot_);
}

///////////////////////////////////////////////////////////////////////////////
// Builder

Builder::Builder(const CVmIdent& ident_, CDspTaskHelper& task_):
	Activity::Builder(ident_, task_), m_reference(getReporter())
{
}

void Builder::setObject(CDspVmDirHelper& dirHelper_)
{
	m_object.reset(new Task::Vm::Object
		(getIdent(), getTask().getClient(), dirHelper_));
}

void Builder::setBackupUuid(const QString& value_)
{
	m_reference = Task::Vm::Reference(value_, getReporter());
}

PRL_RESULT Builder::startActivity(Activity::Service& service_)
{
	Task::Workbench* w = getWorkbench();
	if (NULL == w || m_object.isNull())
		return PRL_ERR_UNINITIALIZED;

	return service_.start(*m_object, Task::Subject<Task::Vm::Subject>
		(Task::Vm::Subject(m_reference), *w));
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

PRL_RESULT Service::find(const CVmIdent& ident_, Activity::Object::Model& dst_) const
{
	Store::Extract e(dst_);
	QMutexLocker g(&m_mutex);
	return find(ident_, e);
}

PRL_RESULT Service::start(Task::Vm::Object vm_, Task::Subject<Task::Vm::Subject> task_)
{
	QScopedPointer<Snapshot::Vm::Subject> s;
	PRL_RESULT e = task_.getSnapshot(vm_, s);
	if (PRL_FAILED(e))
		return e;

	if (PRL_SUCCEEDED(s->attach()) && PRL_FAILED(e = s->destroy()))
		return e;

	QScopedPointer<Vm::Unit> a;
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
	m_vmActivities[vm_.getIdent()] = QSharedPointer<Vm::Unit>(a.take());
	return PRL_ERR_SUCCESS;

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
} // namespace Backup

