////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Task_CloneVm.cpp
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_CloneVm.h"
#include "Task_CloneVm_p.h"
#include "Task_CommonHeaders.h"

#include "CProtoSerializer.h"
#include "CDspClientManager.h"
#include "CFileHelperDepPart.h"
#include "Libraries/Std/PrlAssert.h"
#include "Tasks/Task_ChangeSID.h"
#include "Tasks/Task_BackgroundJob.h"
#include "CDspVm.h"
#include "CDspVmNetworkHelper.h"
#include "CDspLibvirt.h"
#include "Libraries/Std/noncopyable.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

namespace Clone
{
///////////////////////////////////////////////////////////////////////////////
// struct Facade

SmartPtr<CDspClient> Facade::getClient() const
{
	return m_task->getClient();
}

const QString& Facade::getNewVmName() const
{
	return m_task->getNewVmName();
}

const QString& Facade::getNewVmUuid() const
{
	return m_task->getNewVmUuid();
}

const SmartPtr<CVmConfiguration>& Facade::getConfig() const
{
	return m_task->getVmConfig();
}

SmartPtr<IOPackage> Facade::getRequest() const
{
	return m_task->getRequestPackage();
}

///////////////////////////////////////////////////////////////////////////////
// struct Failure

Failure& Failure::code(PRL_RESULT code_)
{
	m_code = code_;
	return *this;
}

Failure& Failure::token(const QString& token_)
{
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, token_, EVT_PARAM_RETURN_PARAM_TOKEN));
	return *this;
}

PRL_RESULT Failure::operator()()
{
	PRL_RESULT output = m_code;
	getError().setEventCode(output);
	m_code = PRL_ERR_SUCCESS;
	return output;
}

PRL_RESULT Failure::operator()(const QString& first_)
{
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, first_, EVT_PARAM_MESSAGE_PARAM_0));
	return operator()();
}

PRL_RESULT Failure::operator()(const QString& first_, const QString& second_)
{
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, first_, EVT_PARAM_MESSAGE_PARAM_0));
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, second_, EVT_PARAM_MESSAGE_PARAM_1));
	return operator()();
}

PRL_RESULT Failure::operator()(const Source::Total& source_)
{
	const QString& n = source_.getConfig().getName();
	return operator()(PRL_ERR_FREE_DISC_SPACE_FOR_CLONE, n);
}

PRL_RESULT Failure::operator()(const CVmEvent& src_)
{
	getError().fromString(src_.toString());
	return operator()(src_.getEventCode());
}

CVmEvent& Failure::getError() const
{
	return *(m_task->getLastError());
}

///////////////////////////////////////////////////////////////////////////////
// struct Toolkit

bool Toolkit::canRead(const QString& path_) const
{
	return CFileHelper::FileCanRead(path_, getAuth());
}

bool Toolkit::fileExists(const QString& path_) const
{
	return CFileHelper::FileExists(path_, getAuth());
}

bool Toolkit::folderExists(const QString& path_) const
{
	return CFileHelper::DirectoryExists(path_, getAuth());
}

PRL_RESULT Toolkit::addFile(const QString& path_) const
{
	if (CFileHelper::CreateBlankFile(path_, getAuth()))
		return PRL_ERR_SUCCESS;

	return PRL_ERR_OPERATION_FAILED;
}

PRL_RESULT Toolkit::addFolder(const QString& path_) const
{
	// Check directory
	if (!folderExists(path_))
	{
		if (!CFileHelper::WriteDirectory(path_, getAuth()))
			return Failure(getTask())(PRL_ERR_MAKE_DIRECTORY, path_);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Toolkit::getSpaceAvailable(QString path_, quint64& dst_) const
{
	dst_ = 0;
	if (!QFile::exists(path_))
		path_ = CFileHelper::GetFileRoot(path_);

	return CFileHelper::GetDiskAvailableSpace(path_, &dst_);
}

bool Toolkit::equal(const QString& one_, const QString& another_)
{
	return	QDir::convertSeparators(QDir::cleanPath(one_)) ==
		QDir::convertSeparators(QDir::cleanPath(another_));
}

///////////////////////////////////////////////////////////////////////////////
// struct Snapshot

Snapshot::Snapshot(Task_CloneVm& task_): Facade(task_),
	m_failure(task_)/*, m_manager(new CDSManager())*/
{
}

bool Snapshot::skip(const Source::Total& source_) const
{
	foreach(CVmHardDisk* pHdd, source_.getConfig().getHardware().m_lstHardDisks)
	{
		//Skip real HDDs - we can't to create snapshots for them anyway
		if ( pHdd->getEmulatedType() != PVE::HardDiskImage )
			continue;

		QString strFullPath = pHdd->getUserFriendlyName();

		//Skip external HDDs
		if (!source_.getPrivate().isInside(strFullPath))
			continue;
/*
		// Return value
		PRL_RESULT RetVal = PRL_ERR_SUCCESS;

		// open disk
		IDisk* pDisk = IDisk::OpenDisk(strFullPath, PRL_DISK_DEFAULT_FLAG, &RetVal);

		// Add pointer here
		if (!pDisk)
		{
			WRITE_TRACE(DBG_FATAL, "Failed to open HDD '%s' with error code: %.8X '%s'. Skipped.",
				QSTR2UTF8(strFullPath), RetVal, PRL_RESULT_TO_STRING(RetVal));
			continue;
		}

		//By design GetSnapshotsCount() == 1 means 0 - no snapshots. Contact with antonz@ for details
		bool bRes = (pDisk->GetSnapshotsCount() == 1) || pDisk->IsDirty();
		pDisk->Release();
		if (bRes)
			return false;
*/
	}
	return true;
}

PRL_RESULT Snapshot::take(const Source::Total& source_)
{
	m_uuid.clear();
	if (skip(source_))
		return PRL_ERR_SUCCESS;

	if (getTask().operationIsCancelled())
		return m_failure(PRL_ERR_OPERATION_WAS_CANCELED);

	bool bNewVmInstance = false;
	SmartPtr<CDspVm> pVm;
	PRL_RESULT output = source_.getVm(pVm, bNewVmInstance);
	if (PRL_FAILED(output))
		return m_failure(output);

	QString u = Uuid::createUuid().toString();
	CProtoCommandPtr pRequest = CProtoSerializer::CreateCreateSnapshotProtoCommand(
			source_.getConfig().getUuid(), QString("Snapshot for a linked clone"),
			QString(), u, QString(), QString(), PCSF_DISK_ONLY);
	CVmEvent evt;
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspCmdVmCreateSnapshot, pRequest);
	if (!pVm->createSnapshot(getClient(), pPackage, &evt, true) || PRL_FAILED(evt.getEventCode()))
	{
		output = m_failure(evt);
		WRITE_TRACE(DBG_FATAL, "Error occurred while snapshot creating with code [%#x][%s]",
			output, PRL_RESULT_TO_STRING(output));
	}
	else
		m_uuid = u;

	if (bNewVmInstance)
		CDspVm::UnregisterVmObject(pVm);

	return output;
}

PRL_RESULT Snapshot::link(const QString& source_, const QString& target_)
{
/*
	m_manager->AddDisk(source_);
	PRL_RESULT output = m_manager->CloneState(Uuid(m_uuid),
						QStringList() << target_,
						&Task_CloneVm::CloneStateCallback,
						&getTask());
	if (PRL_SUCCEEDED(output))
		m_manager->WaitForCompletion();

	m_manager->Clear();
*/
	PRL_RESULT output = PRL_ERR_SUCCESS;
	if (m_failure.getCode() != output)
		m_failure(output);

	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to create clone state for uuid '%s' "
			"original hdd '%s' target hdd '%s' with retcode: %.8X '%s'",
					QSTR2UTF8(m_uuid), QSTR2UTF8(source_),
					QSTR2UTF8(target_), output,
					PRL_RESULT_TO_STRING(output));
		m_failure(PRL_ERR_COULDNT_TO_CREATE_HDD_LINKED_CLONE, source_);
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Content

Content::Content(const Source::Total& source_, const Sink::Private& sink_,
	const ::Clone::Toolkit& toolkit_): Copy::Batch(toolkit_, source_),
	m_failure(toolkit_.getTask()), m_sink(sink_), m_source(&source_)
{
	setJournal(&m_journal);
}

PRL_RESULT Content::copyFile(const char* name_)
{
	return addFile(name_, m_sink.getCopyPath(name_))
		.commit(PDE_GENERIC_DEVICE, m_failure);
}

template<class U>
PRL_RESULT Content::copyHardDisks(CVmHardware& hardware_, U utility_)
{
	hardware_.ClearList(hardware_.m_lstHardDisks);
	hardware_.m_lstHardDisks = m_source->copyHardDisks();
	Copy::Reporter r(m_failure.code(PRL_ERR_HDD_IMAGE_COPY), *m_source);
	foreach (CVmHardDisk* pHdd, hardware_.m_lstHardDisks)
	{
		Copy::Device<CVmHardDisk> d(*pHdd, *m_source, m_sink);
		PRL_RESULT e = d.isDisabled() ? utility_(*pHdd, r.reset()):
			d(utility_, r.setCustom(PRL_ERR_HDD_IMAGE_CLONE_TO_SELF));
		if (PRL_FAILED(e))
			return e;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Content::copyHardDisks(CVmHardware& hardware_)
{
	PRL_RESULT e = copyHardDisks(hardware_, HardDisk::Copy(*this));
	if (PRL_FAILED(e))
		return e;

	return commit(PDE_HARD_DISK, m_failure.code(PRL_ERR_HDD_IMAGE_COPY));
}

PRL_RESULT Content::linkHardDisks(Snapshot& snapshot_, CVmHardware& hardware_)
{
	return copyHardDisks(hardware_,
			HardDisk::Link(getToolkit(), snapshot_, m_journal));
}

PRL_RESULT Content::copyFloppyDisks(CVmHardware& hardware_)
{
	Copy::Reporter r(m_failure.code(PRL_ERR_FDD_IMAGE_COPY), *m_source);
	foreach (CVmFloppyDisk *pFdd, hardware_.m_lstFloppyDisks)
	{
		// for real not copy present
		if (pFdd->getEmulatedType() != PVE::FloppyDiskImage)
			continue;

		Copy::Device<CVmFloppyDisk> d(*pFdd, *m_source, m_sink);
		PRL_RESULT e = d(Copy::Floppy(*this), r);
		if (PRL_FAILED(e))
			return e;
	}
	return commit(PDE_FLOPPY_DISK, r);
}

PRL_RESULT Content::copyNvram(CVmStartupBios& bios_)
{
	QString o = bios_.getNVRAM();
	if (o.isEmpty())
		return PRL_ERR_SUCCESS;

	QString n = m_sink.getPath(PRL_VM_NVRAM_FILE_NAME);
	bios_.setNVRAM(n);

	PRL_RESULT e = addExternalFile(o, n);
	if (PRL_FAILED(e))
		return e;

	m_failure.code(PRL_ERR_NVRAM_FILE_COPY);
	return commit(PDE_HARD_DISK, m_failure);
}

PRL_RESULT Content::copyInternals()
{
	m_failure.code(PRL_ERR_COPY_VM_INFO_FILE);
	return copyFile(VM_INFO_FILE_NAME);
}

PRL_RESULT Content::copySnapshots()
{
	// clone snapshot.xml file - this file always in vm dir
	m_failure.code(PRL_ERR_SNAPSHOTS_COPY);
	PRL_RESULT e = copyFile(VM_GENERATED_SNAPSHOTS_CONFIG_FILE);
	if (PRL_FAILED(e))
		return e;

	return addPrivateFolder(VM_GENERATED_WINDOWS_SNAPSHOTS_DIR,
			m_sink.getCopyPath(VM_GENERATED_WINDOWS_SNAPSHOTS_DIR))
		.commit(PDE_VIRTUAL_SNAPSHOT_DEVICE,
			m_failure.code(PRL_ERR_SNAPSHOTS_COPY));
}

PRL_RESULT Content::copySerialPorts(CVmHardware& hardware_)
{
	foreach (CVmSerialPort* pSerialPort, hardware_.m_lstSerialPorts)
	{
		if (PDT_USE_OUTPUT_FILE != (PRL_VM_DEV_EMULATION_TYPE)pSerialPort->getEmulatedType())
		{
			// Use real device or PIPE
			continue;
		}
		// Validate serial ports output file path
		QString strFullPath = pSerialPort->getUserFriendlyName();
		QString strFileName = CFileHelper::GetFileName(strFullPath);
		///////////////////////////////////////
		// Copying serial port output file
		///////////////////////////////////////
		QString p = m_sink.getPath(strFileName);
		// set new file path
		pSerialPort->setUserFriendlyName(p);
		pSerialPort->setSystemName(p);
		// we only create empty image if it need
		PRL_RESULT e = m_sink.addFile(strFileName);
		if (PRL_FAILED(e))
		{
			m_failure.token(strFullPath).token(p)(PRL_ERR_SERIAL_IMG_COPY);
			return e;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Content::copyParallelPorts(CVmHardware& hardware_)
{
	foreach (CVmParallelPort* pParallelPort, hardware_.m_lstParallelPorts)
	{
		if (PDT_USE_OUTPUT_FILE != (PRL_VM_DEV_EMULATION_TYPE)pParallelPort->getEmulatedType())
		{
			// Use real device or PIPE
			continue;
		}
		// Validate parallel ports output file path
		QString strFullPath = pParallelPort->getUserFriendlyName();
		QString strFileName = CFileHelper::GetFileName(strFullPath);

		///////////////////////////////////////
		// Copying parallel port output file
		///////////////////////////////////////
		QString p = m_sink.getPath(strFileName);
		// set output file name
		pParallelPort->setUserFriendlyName(p);
		pParallelPort->setSystemName(p);
		PRL_RESULT e = m_sink.addFile(strFileName);
		if (PRL_FAILED(e))
		{
			m_failure.token(strFullPath).token(p)(PRL_ERR_PARALLEL_PORT_IMG_COPY);
			return e;
		}
	}
	return PRL_ERR_SUCCESS;
}

namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Config

bool Config::isLinked() const
{
	return !(isTemplate() ||
		m_image->getVmIdentification()->getLinkedVmUuid().isEmpty());
}

bool Config::hasBootcampDevice() const
{
	foreach (CVmHardDisk* d, getHardware().m_lstHardDisks)
	{
		if (PVE::BootCampHardDisk == d->getEmulatedType())
			return true;
	}
	return false;
}

bool Config::canChangeSid() const
{
	CVmCommonOptions* o = m_image->getVmSettings()->getVmCommonOptions();
	return PVS_GUEST_TYPE_WINDOWS == o->getOsType() &&
			o->getOsVersion() >= PVS_GUEST_VER_WIN_XP;
}

///////////////////////////////////////////////////////////////////////////////
// struct Exclusive

Exclusive::Exclusive(Task_CloneVm& task_, const QString& uuid_):
		Facade(task_), m_uuid(uuid_),
		m_helper(&(CDspService::instance()->getVmDirHelper())),
		m_command(PVE::DspIllegalCommand)
{
}

PRL_RESULT Exclusive::lock(PVE::IDispatcherCommands command_)
{
	if (PVE::DspIllegalCommand != m_command)
	{
		WRITE_TRACE(DBG_FATAL, "The lock is busy");
		return PRL_ERR_OPERATION_PENDING;
	}
	PRL_RESULT e = m_helper->registerExclusiveVmOperation(
						m_uuid, getDirectory(),
						command_, getClient(),
						getTask().getJobUuid());
	if (PRL_FAILED(e))
		return e;

	m_command = command_;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Exclusive::unlock()
{
	if (PVE::DspIllegalCommand == m_command)
	{
		WRITE_TRACE(DBG_FATAL, "There is no lock");
		return PRL_ERR_UNEXPECTED;
	}
	PRL_RESULT output = m_helper->unregisterExclusiveVmOperation(m_uuid,
					getDirectory(),	m_command, getClient());
	m_command = PVE::DspIllegalCommand;
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Private

PRL_RESULT Private::setRoot(const QString& uuid_)
{
	m_root.clear();
	CVmIdent i = getClient()->getVmIdent(uuid_);
	m_config = CDspVmDirManager::getVmHomeByUuid(i);
	if (m_config.isEmpty())
		return PRL_ERR_INVALID_ARG;

	m_root = CFileHelper::GetFileRoot(m_config);
	if (!m_root.isEmpty())
		return PRL_ERR_SUCCESS;

	m_config.clear();
	return PRL_ERR_UNEXPECTED;
}

QString Private::getPath(const QString& path_) const
{
	QString p;
	return doGetPath(path_, p) ? p : QString();
}

bool Private::isInside(const QString& path_) const
{
	return !m_config.isEmpty() &&
		getTask().isFileInsideVmHome(path_, m_config);
}

PRL_RESULT Private::getSpaceUsed(quint32 mode_, quint64& dst_) const
{
	dst_ = 0;
	return Task_CalcVmSize::getVmSize(getConfig(), getClient(), dst_, mode_);
}

bool Private::checkAccess(const CVmDevice& device_) const
{
	QString p = device_.getSystemName();
	if (canRead(p))
		return true;
	CAuthHelper a;
	return !CFileHelper::FileExists(p, &a);
}

bool Private::checkAccess(const CVmHardDisk& device_) const
{
	QString p = device_.getSystemName();
	if (canRead(p))
		return true;
	CAuthHelper a;
	return !CFileHelper::DirectoryExists(p, &a);
}

bool Private::doGetPath(const QString& path_, QString& dst_) const
{
	if (m_root.isEmpty())
		return false;
	else if (!QFileInfo(path_).isAbsolute())
		dst_ = QFileInfo(QDir(m_root), path_).filePath();
	else if (!isInside(path_))
		return false;
	else
		dst_ = path_;

	return !dst_.isEmpty();
}

///////////////////////////////////////////////////////////////////////////////
// struct Total

QList<CVmHardDisk* > Total::copyHardDisks() const
{
	QList<CVmHardDisk* > output;
	HardDisk::Filter f(getTask());
	foreach(CVmHardDisk* d, m_config.getHardware().m_lstHardDisks)
	{
		QString p = HardDisk::Flavor::getLocation(*d);
		if (m_private->isInside(p) || f(*d, m_config.getName()))
			output.push_back(new CVmHardDisk(d));
	}
	return output;
}

PRL_RESULT Total::getVm(SmartPtr<CDspVm>& dst_, bool& unregister_) const
{
	CDspAccessManager::VmAccessRights p = CDspService::instance()
			->getAccessManager().getAccessRightsToVm(getClient(),
				m_config.getUuid());
	if (!(p.canRead() && p.canWrite() && p.canExecute()))
	{
		WRITE_TRACE(DBG_FATAL, "Client hasn't enough rights for linked clone");
		return PRL_ERR_NOT_ENOUGH_RIGHTS_FOR_LINKED_CLONE;
	}
	unregister_ = false;
	PRL_RESULT e = PRL_ERR_SUCCESS;
	dst_ = CDspVm::CreateInstance(m_config.getUuid(), m_private->getDirectory(),
			e, unregister_, getClient(), PVE::DspCmdVmCreateSnapshot);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while CDspVm::CreateInstance() with code [%#x][%s]",
			e, PRL_RESULT_TO_STRING(e));
		return e;
	}
	if (!dst_.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Unknown CDspVm::CreateInstance() error");
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Total::checkHardwareAccess(Failure& failure_) const
{
	foreach (CVmFloppyDisk* d, m_config.getHardware().m_lstFloppyDisks)
	{
		if (PVE::FloppyDiskImage == d->getEmulatedType() && !m_private->checkAccess(*d))
			return failure_(m_config.getName(), d->getSystemName());
	}
	foreach (CVmHardDisk* d, m_config.getHardware().m_lstHardDisks)
	{
		if (PVE::HardDiskImage == d->getEmulatedType() && !m_private->checkAccess(*d))
			return failure_(m_config.getName(), d->getSystemName());
	}
	foreach (CVmOpticalDisk* d, m_config.getHardware().m_lstOpticalDisks)
	{
		if (PVE::CdRomImage == d->getEmulatedType() && !m_private->checkAccess(*d))
			return failure_(m_config.getName(), d->getSystemName());
	}
	foreach (CVmSerialPort* d, m_config.getHardware().m_lstSerialPorts)
	{
		if (PVE::SerialOutputFile == d->getEmulatedType() && !m_private->checkAccess(*d))
			return failure_(m_config.getName(), d->getSystemName());
	}
	foreach (CVmParallelPort* d, m_config.getHardware().m_lstParallelPorts)
	{
		if (PVE::ParallelOutputFile == d->getEmulatedType() && !m_private->checkAccess(*d))
			return failure_(m_config.getName(), d->getSystemName());
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Total::checkAccess() const
{
	Failure f(getTask());
	PRL_RESULT e = CDspService::instance()->getAccessManager().checkAccess(
				getClient(), PVE::DspCmdDirVmClone,
				m_config.getUuid(), 0, getTask().getLastError());
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Client hasn't rights to read vm to clone. "
			"err: %#x, %s", e, PRL_RESULT_TO_STRING(e));
		return f(e);
	}
	f.code(PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED);
	switch (CDspVm::getVmState(m_config.getUuid(), m_private->getDirectory()))
	{
	default:
		return checkHardwareAccess(f.code(PRL_ERR_ACCESS_TO_CLONE_VM_DEVICE_DENIED));
	case VMS_SUSPENDED:
		WRITE_TRACE(DBG_FATAL, "Error: can't execute %s command, vm state is forbidden! (state = %#x, '%s')",
			PVE::DispatcherCommandToString(getRequest()->header.type),
			VMS_SUSPENDED, PRL_VM_STATE_TO_STRING(VMS_SUSPENDED));
		return f(m_config.getName(), PRL_VM_STATE_TO_STRING(VMS_SUSPENDED));
	case VMS_SUSPENDING_SYNC:
		WRITE_TRACE(DBG_FATAL, "Error: can't execute %s command, vm state is forbidden! (state = %#x, '%s')",
			PVE::DispatcherCommandToString( getRequest()->header.type),
			VMS_SUSPENDING_SYNC, PRL_VM_STATE_TO_STRING(VMS_SUSPENDING_SYNC));
		return f(m_config.getName(), PRL_VM_STATE_TO_STRING(VMS_SUSPENDING_SYNC));
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Space

PRL_RESULT Space::operator()(const Total& source_, quint64& dst_) const
{
	return source_.getPrivate().getSpaceUsed(m_mode, dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Work

void Work::setSpace(Space* space_)
{
	m_space.reset(space_);
}

void Work::setSnapshot(Snapshot* snapshot_)
{
	m_lock = NULL == snapshot_ ? PVE::DspCmdDirVmClone : PVE::DspCmdDirVmCloneLinked;
	m_snapshot.reset(snapshot_);
}

PRL_RESULT Work::operator()(Sink::Builder& sink_)
{
	PRL_RESULT output = m_source->lock(m_lock);
	if (PRL_FAILED(output))
		return output;
	else
	{
		CAuthHelperImpersonateWrapper g(getAuth());
		if (PRL_FAILED(output = m_source->checkAccess()))
			goto unlock;
		if (!m_snapshot.isNull())
		{
			m_source->unlock();
			if (PRL_FAILED(output = m_snapshot->take(*m_source)))
				return output;
			if (PRL_FAILED(output = m_source->lock(m_lock)))
				return output;
		}
		if (PRL_FAILED(output = sink_.addRoot()))
			goto unlock;
		if (!m_space.isNull())
		{
			output = sink_.checkSpace(*m_source, *m_space);
			if (PRL_FAILED(output))
				goto unlock;
		}
		if (PRL_FAILED(output = sink_.copyContent(*m_source, m_snapshot.data())))
			goto unlock;
	}
	output = sink_.saveConfig(getNewVmName(), getNewVmUuid());
unlock:
	m_source->unlock();
	return output;
}

} // namespace Source

namespace Sink
{
///////////////////////////////////////////////////////////////////////////////
// struct Private

Private::Private(Task_CloneVm& task_): Toolkit(task_), m_folder(task_.getNewVmHome())
{
}

QString Private::getPath(const QString& relative_) const
{
	return QFileInfo(QDir(m_folder), relative_).filePath();
}

QString Private::getCopyPath(const QString& full_) const
{
	QString p = full_;
	return getPath(CFileHelper::GetFileName(p));
}

PRL_RESULT Private::addFile(SmartPtr<CVmConfiguration> config_)
{
	QString p = getPath(VMDIR_DEFAULT_VM_CONFIG_FILE);
	PRL_RESULT e = CDspService::instance()->getVmConfigManager().saveConfig(
				config_, p, getClient(), true, true);
	if (PRL_FAILED(e))
		return e;

	config_->getVmIdentification()->setHomePath(p);
	////////////////////////////////////////////////////////////////////////////
	// Set default permissions to vm files
	// NOTE: It need as SYSTEM user ( after revertToSelf )
	////////////////////////////////////////////////////////////////////////////
	return setDefaultVmPermissions(getClient(), p, false);
}

PRL_RESULT Private::addFile(const QString& relative_)
{
	return Toolkit::addFile(getPath(relative_));
}

PRL_RESULT Private::getSpaceAvailable(quint64& dst_) const
{
	return Toolkit::getSpaceAvailable(m_folder, dst_);
}

PRL_RESULT Private::addRoot()
{
	PRL_RESULT e = addFolder(m_folder);
	if (PRL_FAILED(e))
		return e;

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Dress

Dress::Dress(Task_CloneVm& task_, Private& private_,
	SmartPtr<CVmConfiguration> config_):
	Facade(task_), m_path(private_.getPath(VMDIR_DEFAULT_VM_CONFIG_FILE)),
	m_config(config_)
{
}

CDspService& Dress::s()
{
	return *CDspService::instance();
}

PRL_RESULT Dress::setDirectoryItem()
{
	if (!m_config.isValid())
		return PRL_ERR_UNEXPECTED;

	QString u;
	CVmDirectoryItem* x = new CVmDirectoryItem();
	x->setVmUuid(m_config->getVmIdentification()->getVmUuid());
	x->setRegistered(PVE::VmRegistered);
	x->setValid(PVE::VmValid);
	x->setVmHome(m_path);
	x->setVmName(m_config->getVmIdentification()->getVmName());
	x->setTemplate(m_config->getVmSettings()->getVmCommonOptions()->isTemplate());
	{
		CDspLockedPointer< CDispUser > pLockedDispUser = s().getDispConfigGuard()
			.getDispUserByUuid(getClient()->getUserSettingsUuid());
		if( pLockedDispUser )
			u = pLockedDispUser->getUserName();
	}
	// FIXME set private according to actual flag value within VM config
	x->setIsPrivate(PVE::VmPublic);
	x->setRegisteredBy(u);
	x->setRegDateTime(QDateTime::currentDateTime());
	x->setChangedBy(u);
	x->setChangeDateTime(QDateTime::currentDateTime());
	PRL_RESULT output = s().getVmDirHelper().insertVmDirectoryItem(getDirectory(), x);
	if(PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, ">>> Can't insert vm into the VmDirectory with error %#x, %s",
			output, PRL_RESULT_TO_STRING(output));
	}
	return output;
}

void Dress::undoDirectoryItem()
{
	PRL_RESULT e = s().getVmDirHelper().deleteVmDirectoryItem(
				getDirectory(), getNewVmUuid());
	if (PRL_FAILED(e))
		WRITE_TRACE(DBG_FATAL, "Can not deleted VmDirectoryItem on clonning VM failure");
}

PRL_RESULT Dress::addClusterResource()
{
	if (!m_config.isValid())
		return PRL_ERR_UNEXPECTED;

	return s().getHaClusterHelper()->addClusterResource(
			getNewVmName(),
			m_config->getVmSettings()->getHighAvailability(),
			getTask().getNewVmHome());
}

void Dress::undoClusterResource()
{
	s().getHaClusterHelper()->removeClusterResource(getNewVmName());
}

PRL_RESULT Dress::changeSid()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper g(getAuth());
	return getTask().track(new Task_ChangeSID(getClient(), getRequest(),
					m_config));
}

PRL_RESULT Dress::importBootcamps()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper g(getAuth());
	return PRL_ERR_SUCCESS;
/*
	return getTask().track(new Task_ImportBootCampVm(getClient(), getRequest(),
					m_config, getConfig()));
*/
}

void Dress::undoLibvirtDomain()
{
#ifdef _LIBVIRT_
	Libvirt::Kit.vms().at(getNewVmUuid()).undefine();
#endif // _LIBVIRT_
}

PRL_RESULT Dress::addLibvirtDomain()
{
#ifdef _LIBVIRT_
	Libvirt::Result r(Libvirt::Kit.vms().define(*m_config));
	if (r.isSucceed())
		return PRL_ERR_SUCCESS;

	return Failure(getTask())(r.error().convertToEvent());

#else // _LIBVIRT_
	return PRL_ERR_SUCCESS;
#endif // _LIBVIRT_
}

///////////////////////////////////////////////////////////////////////////////
// struct Builder

PRL_RESULT Builder::addRoot()
{
	if (m_config.isValid())
		return PRL_ERR_DOUBLE_INIT;

	PRL_RESULT e = m_private->addRoot();
	if (PRL_FAILED(e))
		return e;

	m_trash = SmartPtr<QStringList>(new QStringList(m_private->getRoot()), &cleanse);
	m_config = SmartPtr<CVmConfiguration>(new CVmConfiguration(m_grub.get()));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Builder::checkSpace(const Source::Total& source_, const Source::Space& query_)
{
	quint64 a, u;
	PRL_RESULT e;
	Failure f(getTask());
	if (PRL_FAILED(e = query_(source_, u)))
		return f.token(source_.getPrivate().getRoot())(e);

	switch(e = m_private->getSpaceAvailable(a))
	{
	case PRL_ERR_SUCCESS:
		if (u < a)
			return PRL_ERR_SUCCESS;
		WRITE_TRACE(DBG_FATAL, "Task_Clone : There is not enough disk free space for operation!" );
		return f(PRL_ERR_FREE_DISC_SPACE_FOR_CLONE,
				source_.getConfig().getName());
	case PRL_ERR_GET_DISK_FREE_SPACE_FAILED:
		if (getTask().getForceQuestionsSign())
			return PRL_ERR_SUCCESS;
		break;
	case PRL_ERR_INCORRECT_PATH:
		return f(e, m_private->getRoot());
	default:
		return f.token(source_.getPrivate().getRoot())(e);
	}
	// If interactive mode then send question to user
	QList<PRL_RESULT> lstChoices;
	lstChoices.append( PET_ANSWER_YES );
	lstChoices.append( PET_ANSWER_NO );

	QList<CVmEventParameter*> lstParams;
	lstParams.append(new CVmEventParameter(PVE::String, source_.getConfig().getUuid(),
					EVT_PARAM_VM_UUID));
	e = getClient()->sendQuestionToUser(PRL_QUESTION_CAN_NOT_GET_DISK_FREE_SPACE,
			lstChoices, lstParams, getRequest());
	if (PET_ANSWER_YES == e)
		return PRL_ERR_SUCCESS;

	return f(PRL_ERR_OPERATION_WAS_CANCELED);
}

PRL_RESULT Builder::saveConfig(const QString& name_, const QString& uuid_)
{
	Failure f(getTask());
	if (!m_config.isValid())
		return f(PRL_ERR_UNINITIALIZED);

	//////////////////////////////////////////////////////////////////////////
	LOG_MESSAGE( DBG_FATAL,"##########  Configuration Cloned. ##########");
	////////////////////////////////////////////////////////////////////////////
	// save VM configuration
	// NOTE: It need as SYSTEM user ( after revertToSelf ) because saveVmConfig() sets permissions.
	// set new Vm name to config
	m_config->getVmIdentification()->setVmName(name_);
	// set uuid for new vm
	m_config->getVmIdentification()->setVmUuid(uuid_);

	PRL_RESULT e = m_private->addFile(m_config);
	if (PRL_SUCCEEDED(e))
		return PRL_ERR_SUCCESS;

	QString p = m_private->getPath(VMDIR_DEFAULT_VM_CONFIG_FILE);
	WRITE_TRACE(DBG_FATAL, "Parallels Dispatcher unable to save configuration of the VM %s to file %s. Reason: %ld: %s",
		QSTR2UTF8(uuid_), QSTR2UTF8(p), Prl::GetLastError(), QSTR2UTF8( Prl::GetLastErrorAsString() ));
	// check error code - it may be not free space for save config
	switch (e)
	{
	case PRL_ERR_NOT_ENOUGH_DISK_SPACE_TO_XML_SAVE:
		return e;
	case PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS:
		return f(e, name_);
	default:
		// send error to user: can't save VM config to file
		return f.code(PRL_ERR_SAVE_VM_CONFIG)(name_, p);
	}
}

PRL_RESULT Builder::copyContent(const Source::Total& source_, Snapshot* snapshot_)
{
	Failure f(getTask());
	if (!m_config.isValid())
		return f(PRL_ERR_UNINITIALIZED);

	if (!IS_OPERATION_SUCCEEDED(m_config->m_uiRcInit))
		return f(PRL_ERR_PARSE_VM_CONFIG);

	PRL_RESULT output;
	Content x(source_, *m_private, Toolkit(getTask()));
	CVmHardware& h = *(m_config->getVmHardwareList());
	if (PRL_FAILED(output = x.copySerialPorts(h)))
		goto quit;
	if (PRL_FAILED(output = x.copyParallelPorts(h)))
		goto quit;
	if (PRL_FAILED(output = x.copyFloppyDisks(h)))
		goto quit;
	if (PRL_FAILED(output = x.copyInternals()))
		goto quit;
	if (PRL_FAILED(output = x.copyNvram
			(*(m_config->getVmSettings()->getVmStartupOptions()->getBios()))))
		goto quit;

	if (NULL == snapshot_)
	{
		if (PRL_FAILED(output = x.copyHardDisks(h)))
			goto quit;
		output = x.copySnapshots();
	}
	else
	{
		if (PRL_FAILED(output = x.linkHardDisks(*snapshot_, h)))
			goto quit;
		//https://bugzilla.sw.ru/show_bug.cgi?id=468401
		m_config->getVmIdentification()->setLinkedVmUuid(source_.getConfig().getUuid());
	}
quit:
	m_trash->append(x.getJournal());
	return output;
}

Result Builder::getResult()
{
	Result output(m_config, m_trash);
	m_trash.reset();
	m_config.reset();
	return output;
}

void Builder::cleanse(QStringList* trash_)
{
	QScopedPointer<QStringList> g(trash_);
	if (g.isNull())
		return;
	foreach (const QString& p, *g)
	{
		CFileHelper::ClearAndDeleteDir(p);
	}
}

} // namespace Sink

namespace HardDisk
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

QString Flavor::getLocation(const CVmHardDisk& device_)
{
	switch (device_.getEmulatedType())
	{
	case PVE::HardDiskImage:
	case PVE::ContainerHardDisk:
		return device_.getUserFriendlyName();
	default:
		return device_.getSystemName();
	}
}

void Flavor::update(CVmHardDisk& device_, const QString& name_)
{
	// set new hdd file name to config
	switch (device_.getEmulatedType())
	{
	case PVE::HardDiskImage:
	case PVE::ContainerHardDisk:
		device_.setUserFriendlyName(name_);
	default:
		device_.setSystemName(name_);
		break;
	}
}

QString Flavor::getExternal(const CVmHardDisk& device_, const QString& bundle_)
{
	QFileInfo x(getLocation(device_));
	QString b = bundle_;
	if (PVE::HardDiskImage == device_.getEmulatedType() &&
		device_.getInterfaceType() != PMS_UNKNOWN_DEVICE)
		b.append(VMDIR_DEFAULT_BUNDLE_SUFFIX);

	QFileInfo y(x.absoluteDir(), b);
	QFileInfo z(QDir(y.absoluteFilePath()), x.fileName());
	return z.absoluteFilePath();
}

///////////////////////////////////////////////////////////////////////////////
// struct Filter

Filter::Filter(Task_CloneVm& task_): Facade(task_)
{
	if (0 == (getTask().getRequestFlags() & PCVF_DETACH_EXTERNAL_VIRTUAL_HDD))
		m_warning = PRL_WARN_OUTSIDED_HDD_WILL_BE_ONLY_LINKED_AFTER_CLONE;
	else
		m_warning = PRL_WARN_LINK_TO_OUTSIDED_HDD_WILL_BE_DROPPED_AFTER_CLONE;
}

bool Filter::operator()(const CVmHardDisk& device_, const QString& name_) const
{
	int x = device_.getIndex();
	QString p = Flavor::getLocation(device_);
	QString sDetails = QString("Full Path to Hard Disk %1: %2").arg(x).arg(p);

	CVmEvent event(PET_DSP_EVT_VM_MESSAGE, name_, PIE_DISPATCHER, m_warning);
	event.addEventParameter(
		new CVmEventParameter(PVE::String, QString::number(x),
			EVT_PARAM_MESSAGE_PARAM_0));
	event.addEventParameter(
		new CVmEventParameter(PVE::String, name_, EVT_PARAM_MESSAGE_PARAM_1));
	event.addEventParameter(
		new CVmEventParameter(PVE::String, sDetails, EVT_PARAM_DETAIL_DESCRIPTION));

	SmartPtr<IOPackage> w =
		DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequest());

	getClient()->sendPackage(w);
	WRITE_TRACE( DBG_WARNING, "send warn %s about external hdd %d %s"
		, PRL_RESULT_TO_STRING(m_warning), x, QSTR2UTF8(p));

	return PRL_WARN_OUTSIDED_HDD_WILL_BE_ONLY_LINKED_AFTER_CLONE == m_warning;
}

///////////////////////////////////////////////////////////////////////////////
// struct Copy

PRL_RESULT Copy::operator()(CVmHardDisk& device_, const QString& target_)
{
	m_batch->addFile(Flavor::getLocation(device_), target_);
	Flavor::update(device_, target_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Copy::operator()(CVmHardDisk& device_, const Clone::Copy::Reporter& reporter_)
{
	if (PVE::HardDiskImage != device_.getEmulatedType())
		return PRL_ERR_SUCCESS;

	QString s = Flavor::getLocation(device_);
	QString t = Flavor::getExternal(device_, m_batch->getToolkit());
	PRL_RESULT e = m_batch->addExternalFile(s, t);
	if (PRL_SUCCEEDED(e) || e == PRL_ERR_ENTRY_DOES_NOT_EXIST)
	{
		Flavor::update(device_, t);
		return PRL_ERR_SUCCESS;
	}
	return reporter_(s, t);
}

///////////////////////////////////////////////////////////////////////////////
// struct Link

Link::Link(const Toolkit& toolkit_, Snapshot& snapshot_, QStringList& journal_):
	m_toolkit(toolkit_), m_snapshot(&snapshot_), m_journal(&journal_)
{
}

PRL_RESULT Link::operator()(CVmHardDisk& device_, const Clone::Copy::Reporter& reporter_)
{
	if (PVE::HardDiskImage != device_.getEmulatedType())
		return PRL_ERR_SUCCESS;

	QString t = Flavor::getExternal(device_, m_toolkit);
	PRL_RESULT e = m_toolkit.addFolder(t);
	if (PRL_FAILED(e))
		return reporter_(Flavor::getLocation(device_), t);

	m_journal->push_back(t);
	return (*this)(device_, t);
}

PRL_RESULT Link::operator()(CVmHardDisk& device_, const QString& target_)
{
	QString s = Flavor::getLocation(device_);
	Flavor::update(device_, target_);
	if (!m_toolkit.folderExists(s))
		return PRL_ERR_SUCCESS;

	return m_snapshot->link(s, target_);
}

} // namespace HardDisk

namespace Copy
{
///////////////////////////////////////////////////////////////////////////////
// struct Reporter

PRL_RESULT Reporter::operator()(PRL_RESULT error_, const QString& source_,
				const QString& target_) const
{
	switch (error_)
	{
	case PRL_ERR_CLONE_OPERATION_CANCELED:
		return (*m_failure)(PRL_ERR_CLONE_OPERATION_CANCELED);
	case PRL_ERR_FILE_DISK_SPACE_ERROR:
		return (*m_failure)(*m_source);
	default:
		WRITE_TRACE(DBG_FATAL, "copy with notifications failed %s: %s",
			QSTR2UTF8(source_), PRL_RESULT_TO_STRING(error_));
		return (*this)(source_, target_);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

template<>
QString Device<CVmHardDisk>::getSource(const CVmHardDisk& config_)
{
	return HardDisk::Flavor::getLocation(config_);
}

template<>
QString Device<CVmFloppyDisk>::getSource(const CVmFloppyDisk& config_)
{
	return Floppy::getLocation(config_);
}

template<>
QString Device<CVmStartupBios>::getSource(const CVmStartupBios& config_)
{
	return config_.getNVRAM();
}

///////////////////////////////////////////////////////////////////////////////
// struct Batch

Batch& Batch::addFile(const QString& source_, const QString& target_)
{
	QString s = m_source->getPrivate().getPath(source_);
	if (!s.isEmpty() && fileExists(s))
		m_files << qMakePair(s, target_);

	return *this;
}

Batch& Batch::addPrivateFolder(const QString& source_, const QString& target_)
{
	QString s = m_source->getPrivate().getPath(source_);
	if (!s.isEmpty() && folderExists(s))
		m_folders << qMakePair(s, target_);

	return *this;
}

PRL_RESULT Batch::addExternalFolder(const QString& source_, const QString& target_)
{
	QString d = QFileInfo(target_).absolutePath();
	if (!folderExists(d))
	{
		PRL_RESULT e = addFolder(d);
		if (PRL_FAILED(e))
			return e;
		if (NULL != m_journal)
			m_journal->push_back(d);
	}
	else if (folderExists(target_))
		return PRL_ERR_FILE_OR_DIR_ALREADY_EXISTS;

	if (!folderExists(source_))
		return PRL_ERR_ENTRY_DOES_NOT_EXIST;

	if (NULL != m_journal)
		m_journal->push_back(target_);

	m_folders << qMakePair(source_, target_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Batch::addExternalFile(const QString& source_, const QString& target_)
{
	if (!fileExists(source_))
		return PRL_ERR_ENTRY_DOES_NOT_EXIST;

	if (fileExists(target_))
		return PRL_ERR_FILE_OR_DIR_ALREADY_EXISTS;

	QString b(QFileInfo(target_).absoluteDir().absolutePath());
	if (!folderExists(b))
	{
		PRL_RESULT e = addFolder(b);
		if (PRL_FAILED(e))
			return e;
		if (NULL != m_journal)
			m_journal->push_back(b);
	}

	if (NULL != m_journal)
		m_journal->push_back(target_);

	m_files << qMakePair(source_, target_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Batch::commit(PRL_DEVICE_TYPE kind_, const Reporter& reporter_)
{
	int i = 0;
	QList<item_type>::iterator p = m_folders.begin();
	for (; p != m_folders.end(); ++i)
	{
		PRL_RESULT e = CFileHelperDepPart::CopyDirectoryWithNotifications(
				p->first, p->second, getAuth(), &getTask(),
				kind_, i, true);
		if (PRL_FAILED(e))
			return reporter_(e, p->first, p->second);

		p = m_folders.erase(p);
	}
	p = m_files.begin();
	for (; p != m_files.end(); ++i)
	{
		PRL_RESULT e = CFileHelperDepPart::CopyFileWithNotifications(
				p->first, p->second, getAuth(), &getTask(),
				kind_, i);
		if (PRL_FAILED(e))
			return reporter_(e, p->first, p->second);

		p = m_files.erase(p);
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

PRL_RESULT Floppy::operator()(CVmFloppyDisk& device_, const QString& target_)
{
	// set new fdd file name to config
	QString s = getLocation(device_);
	device_.setSystemName(target_);
	device_.setUserFriendlyName(target_);
	m_batch->addFile(s, target_);
	return PRL_ERR_SUCCESS;
}

} // namespace Copy
} // namespace Clone

Task_CloneVm::Task_CloneVm ( SmartPtr<CDspClient>& user,
                             const SmartPtr<IOPackage>& p,
                             SmartPtr<CVmConfiguration> pVmConfig,
                             const QString& strVmNewName,
                             const QString& strVmNewUuid,
                             const QString & strNewVmRootDir,
			     unsigned int nFlags) :

	CDspTaskHelper( user, p ),
	m_pOldVmConfig( pVmConfig ),
	m_newVmUuid( Uuid::createUuid() ),
	m_newVmName(CFileHelper::ReplaceNonValidPathSymbols(strVmNewName)),
	m_mtxWaitExternalTask(QMutex::Recursive),
	m_externalTask(NULL),
	m_flgLockRegistred(false),
	m_pVmInfo(0),
	m_bCreateTemplate( nFlags & PCVF_CLONE_TO_TEMPLATE ),
	m_bChangeSID( nFlags & PCVF_CHANGE_SID ),
	m_bLinkedClone( nFlags & PCVF_LINKED_CLONE ),
	m_bImportBootCamp( nFlags & PCVF_IMPORT_BOOT_CAMP )
{
	setLastErrorCode(PRL_ERR_SUCCESS);

	QString newVmRootDir;
	do
	{
		PRL_ASSERT(m_pOldVmConfig);
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		if( ! strNewVmRootDir.isEmpty() && ! QDir::isAbsolutePath( strNewVmRootDir ) )
		{
			setLastErrorCode ( PRL_ERR_VMDIR_PATH_IS_NOT_ABSOLUTE );
			break;
		}

		if( !strNewVmRootDir.isEmpty() && !QDir( strNewVmRootDir ).exists() )
		{
			getLastError()->addEventParameter(
					new CVmEventParameter(PVE::String,
					strNewVmRootDir,
					EVT_PARAM_MESSAGE_PARAM_0));
			setLastErrorCode ( PRL_ERR_DIRECTORY_DOES_NOT_EXIST );
			break;
		}

		newVmRootDir=strNewVmRootDir;

		if ( newVmRootDir.isEmpty() )
		{
			newVmRootDir = CDspVmDirHelper::getVmRootPathForUser( getClient() );

			// #127473 to prevent create directory on external unmounted disk
			if( !QDir(newVmRootDir).exists() )
			{
				WRITE_TRACE(DBG_FATAL, "Parallels VM Directory %s does not exists." , QSTR2UTF8( newVmRootDir ) );
				setLastErrorCode (PRL_ERR_VM_DIRECTORY_FOLDER_DOESNT_EXIST);
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, newVmRootDir,
					EVT_PARAM_MESSAGE_PARAM_0 ) );

				break;
			}
		}

		if (!strVmNewUuid.isEmpty())
		{
			WRITE_TRACE(DBG_WARNING, "Clone Vm custom uuid %s" ,
					QSTR2UTF8(strVmNewUuid));
			m_newVmUuid = strVmNewUuid;
		}

		if (newVmRootDir.endsWith('/') || newVmRootDir.endsWith('\\'))
			newVmRootDir.chop(1);

		m_newVmXmlPath=QString("%1/%2%3/%4")
			.arg(newVmRootDir)
			.arg(m_newVmName)
			.arg( m_newVmName.endsWith(VMDIR_DEFAULT_BUNDLE_SUFFIX) ? "" : VMDIR_DEFAULT_BUNDLE_SUFFIX )
			.arg(VMDIR_DEFAULT_VM_CONFIG_FILE);

	} while(0);

	//////////////////////////////////////////////////////////////////////////
	setTaskParameters(
		m_pOldVmConfig->getVmIdentification()->getVmUuid(),
		m_newVmName,
		newVmRootDir,
		m_bCreateTemplate
		);
}

Task_CloneVm::~Task_CloneVm()
{
	if (m_pVmInfo)
		delete m_pVmInfo;

}

void Task_CloneVm::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pUserSession, p);
	QMutexLocker locker(&m_mtxWaitExternalTask);
	if (m_externalTask != NULL)
	{
		m_externalTask->cancelOperation(pUserSession, p);
	}
}

void Task_CloneVm::setTaskParameters( const QString& strVmUuid,
		 const QString& strVmNewName,
		 const QString & strNewVmRootDir,
		 bool bCreateTemplate )
{
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, strVmUuid, EVT_PARAM_DISP_TASK_CLONE_VM_UUID ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, strVmNewName, EVT_PARAM_DISP_TASK_CLONE_NEW_VM_NAME ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, strNewVmRootDir, EVT_PARAM_DISP_TASK_CLONE_NEW_VM_ROOT_DIR ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::Boolean, QString("%1").arg( bCreateTemplate ),
			EVT_PARAM_DISP_TASK_CLONE_AS_TEMPLATE ) );
}

QString Task_CloneVm::getVmUuid()
{
	return m_pOldVmConfig ? m_pOldVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_CloneVm::getCancelResult()
{
	return PRL_ERR_CLONE_OPERATION_CANCELED;
}

PRL_RESULT Task_CloneVm::prepareTask()
{
	// check params in existing VM
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

		if( !IS_OPERATION_SUCCEEDED( m_pOldVmConfig->m_uiRcInit ) )
			throw PRL_ERR_PARSE_VM_CONFIG;

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		if( CFileHelper::isRemotePath(m_newVmXmlPath) )
		{
			if( ! CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->isAllowUseNetworkShares() )
			{
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String,
					CFileHelper::GetFileRoot(getNewVmHome()),
					EVT_PARAM_MESSAGE_PARAM_0));
				throw PRL_ERR_UNSUPPORTED_NETWORK_FILE_SYSTEM;
			}
		}
		/// Check strVmNewPathName (path is valid and accessible, no file by name exists)
		if (CFileHelper::FileExists(m_newVmXmlPath, &getClient()->getAuthHelper()))
		{
			getLastError()->setEventCode(PRL_ERR_VM_CONFIG_ALREADY_EXISTS);
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_newVmXmlPath,
				EVT_PARAM_RETURN_PARAM_TOKEN));
			throw PRL_ERR_VM_CONFIG_ALREADY_EXISTS;
		}

		if( m_newVmName.isEmpty())
			throw PRL_ERR_VM_NAME_IS_EMPTY;

		m_pVmInfo =  new CVmDirectory::TemporaryCatalogueItem(m_newVmUuid, m_newVmXmlPath, m_newVmName);
		PRL_RESULT lockResult = CDspService::instance()->getVmDirManager()
			.checkAndLockNotExistsExclusiveVmParameters(
				getClient()->getVmDirectoryUuidList(),
				m_pVmInfo );

		if( ! PRL_SUCCEEDED( lockResult) )
		{

			switch ( lockResult )
			{
			case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
				break;

			case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

			default:
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
				getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
			}//switch

			throw lockResult;
		}
		m_flgLockRegistred=true;

		ret = PRL_ERR_SUCCESS;
	}
	catch(PRL_RESULT code)
	{
		ret=code;
		getLastError()->setEventCode( code );
		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

void Task_CloneVm::finalizeTask()
{
	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		/**
		* Notify all user: vm directory changed
		*/
		CVmEvent event( PET_DSP_EVT_VM_ADDED,
			m_pVmInfo->vmUuid,
			PIE_DISPATCHER );

		SmartPtr<IOPackage> p =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

		CDspService::instance()->getClientManager().sendPackageToVmClients( p,
			getClient()->getVmDirectoryUuid(), m_pVmInfo->vmUuid );
	}
	// delete temporary registration
	if (m_pVmInfo && m_flgLockRegistred)
	{
		CDspService::instance()->getVmDirManager()
			.unlockExclusiveVmParameters( m_pVmInfo );
	}

	// send response
	SendCloneResponse();
}

template<class T>
PRL_RESULT Task_CloneVm::do_(T , Clone::Source::Total& source_)
{
	using namespace Clone::Sink;
	Clone::Source::Work w(*this, source_);
	if (m_bCreateTemplate)
	{
		Flavor<Template::Local> f;
		return Clone::Work::Great<T, Template::Local>::do_(*this, w, f);
	}
	bool s = m_bChangeSID;
	if (s)
	{
		PRL_RESULT e = CheckWhetherChangeSidOpPossible(getVmConfig(),
					getClient()->getVmDirectoryUuid());
		if (PRL_FAILED(e))
			return e;
	}
	Clone::Source::Config x(*this);
	if (m_bLinkedClone || x.isLinked())
	{
		Flavor<Clone::Sink::Vm::Linked> f(s);
		return Clone::Work::Great<T, Clone::Sink::Vm::Linked>::do_(*this, w, f);
	}
	else if (!m_bImportBootCamp && x.hasBootcampDevice())
	{
		Flavor<Clone::Sink::Vm::Bootcamp> f(s);
		return Clone::Work::Great<T, Clone::Sink::Vm::Bootcamp>::do_(*this, w, f);
	}
	Flavor<Clone::Sink::Vm::General> f(s);
	f.setBootcamps(m_bImportBootCamp);
	return Clone::Work::Great<T, Clone::Sink::Vm::General>::do_(*this, w, f);
}

PRL_RESULT Task_CloneVm::run_body()
{
	PRL_RESULT output;
	do
	{
		using namespace Clone::Source;
		Private p(*this);
		QString u = getVmUuid();
		if (PRL_FAILED(output = p.setRoot(u)))
		{
			WRITE_TRACE(DBG_FATAL, "Unexpected error: couldn't to get VM '%s' home dir",
				QSTR2UTF8(u));
			break;
		}
		Total t(*this, p);
		if (t.getConfig().isLinked())
		{
			output = do_(Clone::Source::Vm::Linked(), t);
			break;
		}
		if (t.getConfig().isTemplate())
		{
			output = do_(Template::Local(), t);
			break;
		}
		if (t.getConfig().hasBootcampDevice())
		{
			output = do_(Clone::Source::Vm::Bootcamp(), t);
			break;
		}
		output = do_(Clone::Source::Vm::General(), t);
	} while(false);
	setLastErrorCode(output);
	return output;
}

void Task_CloneVm::SendCloneResponse()
{
	// send response
	if ( ! PRL_SUCCEEDED( getLastErrorCode() ) )
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	else
	{
		CProtoCommandPtr pCmd =
			CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
		CProtoCommandDspWsResponse*
			pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

		PRL_RESULT outError = PRL_ERR_UNINITIALIZED;
		SmartPtr<CVmConfiguration>
			pVmConfig = CDspService::instance()->getVmDirHelper()
			.getVmConfigByUuid( getClient(), m_newVmUuid, outError );
		if( !pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED(outError) );
			if( !PRL_FAILED(outError) )
				outError = PRL_ERR_FAILURE;

			setLastErrorCode(outError);
			getClient()->sendResponseError( getLastError(), getRequestPackage() );
			return;
		}

		CDspService::instance()->getVmDirHelper().appendAdvancedParamsToVmConfig(getClient(), pVmConfig);

		pResponseCmd->SetVmConfig( pVmConfig->toString() );

		getClient()->sendResponse( pCmd, getRequestPackage() );
	}
}

PRL_BOOL Task_CloneVm::CloneStateCallback(PRL_STATES_CALLBACK_TYPE iCallbackType,
				PRL_INT32 iProgress, PRL_VOID_PTR pParameter)
{
	Q_UNUSED(iCallbackType);
	WRITE_TRACE(DBG_DEBUG, "iCallbackType [%d] iProgress [%d]", iCallbackType, iProgress);

	Task_CloneVm *pTask = reinterpret_cast<Task_CloneVm *>(pParameter);
	Clone::Failure f(*pTask);
	if( pTask->operationIsCancelled() )
	{
		WRITE_TRACE(DBG_FATAL, "Operation was canceled!" );
		f(PRL_ERR_OPERATION_WAS_CANCELED);
		return PRL_FALSE;
	}

	// Check progress
	if (iProgress < 0)
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while taking snapshot with code [%#x][%s]"
			, iProgress, PRL_RESULT_TO_STRING( iProgress ) );

		f(iProgress);
		return PRL_FALSE;
	}

	if (iProgress == PRL_STATE_PROGRESS_COMPLETED)
		f(PRL_ERR_SUCCESS);

	return PRL_TRUE;
}

QString Task_CloneVm::getNewVmHome() const
{
	return CFileHelper::GetFileRoot(m_newVmXmlPath);
}

PRL_RESULT Task_CloneVm::track(CDspTaskHelper* task_)
{
	SmartPtr<CDspTaskHelper> p = CDspService::instance()->getTaskManager()
					.registerTask(task_);
	if (!p.isValid())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	m_mtxWaitExternalTask.lock();
	m_externalTask = p.getImpl();
	m_mtxWaitExternalTask.unlock();

	p->start();
	p->wait();
	PRL_RESULT output = p->getLastError()->getEventCode();
	if (PRL_ERR_OPERATION_WAS_CANCELED != output)
		CancelOperationSupport::undoCancelled();

	m_mtxWaitExternalTask.lock();
	m_externalTask = NULL;
	m_mtxWaitExternalTask.unlock();

	return output;
}

void Task_CloneVm::ResetNetSettings( SmartPtr<CVmConfiguration> pVmConfig )
{
	bool bCreateTemplate = pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate();

	HostUtils::MacPrefixType prefix = HostUtils::MAC_PREFIX_VM;
	switch (pVmConfig->getVmType()) {
	case PVT_CT:
		prefix = HostUtils::MAC_PREFIX_CT;
		break;
	case PVT_VM:
		prefix = HostUtils::MAC_PREFIX_VM;
		break;
	}

	foreach(CVmGenericNetworkAdapter *pNetAdapter, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		// regenerate mac address for cloned VM or CT
		pNetAdapter->setMacAddress(HostUtils::generateMacAddress(prefix));
		pNetAdapter->setHostInterfaceName
			(HostUtils::generateHostInterfaceName(pNetAdapter->getMacAddress()));

		// reset IP addresses for templates
		if ( bCreateTemplate )
			pNetAdapter->setNetAddresses();
	}

	CDspVmNetworkHelper::updateHostMacAddresses(pVmConfig, NULL, HMU_CHECK_NONEMPTY);
}

PRL_RESULT Task_CloneVm::CheckWhetherChangeSidOpPossible
	( const SmartPtr<CVmConfiguration> &pVmConfig, const QString &sVmDirUuid )
{
	Clone::Source::Config u(pVmConfig);
	if (!u.canChangeSid())
		return PRL_ERR_CHANGESID_NOT_SUPPORTED;

	if (CDspVm::getVmToolsState(u.getUuid(), sVmDirUuid) == PTS_NOT_INSTALLED)
		return PRL_ERR_CHANGESID_GUEST_TOOLS_NOT_AVAILABLE;
	return PRL_ERR_SUCCESS;
}

