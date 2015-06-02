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
///	CDspVmSnapshotInfrastructure.cpp
///
/// @brief
///	Definition of utilities to call the snapshot management inside a vm.
///
/// @author shrike@
///
////////////////////////////////////////////////////////////////////////////////

// #define FORCE_LOGGING_ON
// #define FORCE_LOGGING_LEVEL   DBG_DEBUG

#include <Libraries/Logging/Logging.h>

#include "CDspService.h"
#include "CDspVmInfoDatabase.h"
#include "Interfaces/ParallelsQt.h"
#include "XmlModel/VmInfo/CVmInfo.h"
#include "CDspVmSnapshotInfrastructure.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"

#include "Tasks/Task_CommitUnfinishedDiskOp.h"

namespace Snapshot
{

///////////////////////////////////////////////////////////////////////////////
// struct DiskState

DiskState::DiskState()
	: m_unfinished(false)
// VirtualDisk commented out by request from CP team
//	, m_operation(PROCESS_NONE)
{
}

DiskState::DiskState(const QString& disk_)
	: m_unfinished(false)
	, m_disk(disk_)
// VirtualDisk commented out by request from CP team
//	, m_operation(PROCESS_NONE)
{
// VirtualDisk commented out by request from CP team
//	// Disk instance
//	IDisk* pDisk = NULL;
//	// Return value
//	PRL_RESULT retVal = PRL_ERR_SUCCESS;
//	// Open disk
//	pDisk = IDisk::OpenDisk(m_disk
//		, PRL_DISK_NO_ERROR_CHECKING | PRL_DISK_READ | PRL_DISK_FAKE_OPEN
//		, &retVal);
//	if (pDisk)
//	{
//		m_unfinished = pDisk->IsUncommited();
//		Uuid snapshotUid = pDisk->GetUnfinishedOpUid();
//		if (!snapshotUid.isNull())
//		{
//			m_snapshot = snapshotUid.toString();
//			m_operation = pDisk->GetUnfinishedOpType();
//		}
//		pDisk->Release();
//	}
//	else
//	{
//		WRITE_TRACE(DBG_FATAL, "Can't open disk %s, %s",
//			QSTR2UTF8(m_disk), PRL_RESULT_TO_STRING(retVal));
//		m_disk.clear();
//	}

	LOG_MESSAGE(DBG_FATAL, "diskState: %s", QSTR2UTF8(toString()));
}

QString DiskState::toString() const
{
// VirtualDisk commented out by request from CP team
//	QString s = QString("unfinished=%1, snapId='%2', opType=%3, disk='%4'")
//		.arg(m_unfinished)
//		.arg(m_snapshot)
//		.arg(m_operation)
//		.arg(m_disk);
//	return s;
	return QString();
}

bool DiskState::isDeleteOp() const
{
// VirtualDisk commented out by request from CP team
//	return !m_snapshot.isEmpty()
//					&& (m_operation == PROCESS_DELETE || m_operation == PROCESS_DELETE_FILES);
	return false;
}

QList<DiskState> getUnfinishedOps(const SmartPtr<CVmConfiguration>& config_)
{
	QList<DiskState> lst;
	if (!config_)
		return lst;

	foreach (CVmHardDisk* hdd, config_->getVmHardwareList()->m_lstHardDisks)
	{
		if (!hdd->getEnabled())
			continue;
		if (hdd->getEmulatedType() != PVE::HardDiskImage)
			continue;
		DiskState st(hdd->getSystemName());
		if (!st.isUnfinished())
			continue;

		lst << st;
		WRITE_TRACE(DBG_DEBUG, "getUnfinishedOps(): %s", QSTR2UTF8(st.toString()));

	}
	WRITE_TRACE(DBG_DEBUG, "Found %d unfinished states", lst.size());
	return lst;
}

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Destroy

Destroy::Destroy(const QString& vm_, const QString& uuid_, quint32 flags_, bool child_)
{
	m_command = Parallels::CProtoSerializer::CreateDeleteSnapshotProtoCommand
			(vm_, uuid_, child_, flags_);
}

PRL_RESULT Destroy::operator()(SmartPtr<IOPackage>& dst_) const
{
	if (NULL == m_config.getImpl())
		return PRL_ERR_NO_DATA;

	CVmEvent v(m_command->GetCommand()->toString());
	v.addEventParameter(new CVmEventParameter(PVE::String,
		m_config->toString(), EVT_PARAM_VM_CONFIG));
	dst_ = Parallels::DispatcherPackage::createInstance(name(), v.toString());
	return PRL_ERR_SUCCESS;
}

void Destroy::step(quint32 value_)
{
	cast().SetDeletionStep(value_);
}

void Destroy::merge(bool value_)
{
	cast().SetMerge(value_);
}

void Destroy::steps(quint32 value_)
{
	cast().SetStepsCount(value_);
}

Parallels::CProtoDeleteSnapshotCommand& Destroy::cast()
{
	return *Parallels::CProtoSerializer::CastToProtoCommand
			<Parallels::CProtoDeleteSnapshotCommand>(m_command);
}

} // namespace Command

namespace Revert
{
///////////////////////////////////////////////////////////////////////////////
// struct Answer

PRL_RESULT Answer::operator()(IOSendJob::Handle job_)
{
	setResult(SmartPtr<IOPackage>());
	IOSendJob::Response r = CDspService::instance()->getIOServer()
							.takeResponse(job_);
	if (m_note->isVmStopped())
		return PRL_ERR_SUCCESS;

	if (IOSendJob::Success == r.responseResult)
	{
		SmartPtr<IOPackage> p = Snapshot::Answer::Miner(r)();
		if (NULL != p.getImpl())
		{
			setResult(p);
			return PRL_ERR_SUCCESS;
		}
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "Connection with VM unexpectedly closed.");
		m_note->setUnexpected();
	}
	return fail();
}

///////////////////////////////////////////////////////////////////////////////
// struct Command

void Command::info(CVmEvent& dst_) const
{
	QString h = m_backup->getVmIdentification()->getHomePath();
	QString p = QString("%1/%2/%3%4")
					.arg(CFileHelper::GetFileRoot(h))
					.arg(VM_GENERATED_WINDOWS_SNAPSHOTS_DIR)
					.arg(m_snapshot)
					.arg(VM_INFO_FILE_SUFFIX);

	QString i;
	SmartPtr<CVmInfo> x = CDspVmInfoDatabase::readVmInfo(p);
	if (NULL != x.getImpl())
		i = x->toString();
	dst_.addEventParameter(new CVmEventParameter(PVE::String, i, EVT_PARAM_VM_INFO));
}

SmartPtr<IOPackage> Command::do_() const
{
	CVmEvent v(UTF8_2QSTR(m_task->getRequestPackage()->buffers[0].getImpl()));
	v.addEventParameter(new CVmEventParameter(PVE::String,
		m_backup->toString(), EVT_PARAM_VM_CONFIG));
	v.addEventParameter(new CVmEventParameter(PVE::String,
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->toString(),
		EVT_PARAM_DISP_COMMON_PREFERENSES));
	v.addEventParameter(new CVmEventParameter(PVE::String,
		CDspService::instance()->getNetworkConfig()->toString(),
		EVT_PARAM_NETWORK_PREFERENSES));

	PVE::SnapshotedVmState s = CDspService::instance()->getVmSnapshotStoreHelper().
		getSnapshotedVmState(m_task->getClient(), m_task->getRequestPackage());

	v.addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(quint32(s)),
		EVT_PARAM_SWITCH_TO_SNAPSHOT_VM_STATE));

	v.addEventParameter(new CVmEventParameter(PVE::String, m_task->getJobUuid().toString(),
		EVT_PARAM_DISP_TASK_UUID));

	info(v);
	return DispatcherPackage::createInstance(name(), v.toString());
}

} // namespace Revert

///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(const QString& vm_, CDspTaskHelper& task_, VIRTUAL_MACHINE_STATE state_):
	m_task(&task_), m_state(state_)
{
	m_vm = CDspVm::GetVmInstanceByUuid(vm_, task_.getClient()->getVmDirectoryUuid());
}

template<class T>
PRL_RESULT Unit::wait(IOSendJob::Handle job_, T& answer_)
{
	IOSendJob::Result s = IOSendJob::Timeout;
	while(IOSendJob::Timeout == s)
	{
		if (m_task->operationIsCancelled())
			return PRL_ERR_OPERATION_WAS_CANCELED;

		s = CDspService::instance()->getIOServer()
			.waitForResponse(job_, 100);
	}
	WRITE_TRACE(DBG_FATAL, "The wait finished with 0x%x", s);
	if (IOSendJob::Success != s)
		return T::fail();

	return answer_(job_);
}

static inline SmartPtr<IOPackage> log(const SmartPtr<IOPackage>& request_)
{
	WRITE_TRACE(DBG_FATAL, "Send command %s to vm_app",
			PVE::DispatcherCommandToString(request_->header.type));
	return request_;
}

static inline PRL_RESULT log(PRL_RESULT result_)
{
	WRITE_TRACE(DBG_FATAL, "The wait result is 0x%x (%s)",
			result_, PRL_RESULT_TO_STRING(result_));
	return result_;
}

PRL_RESULT Unit::commitUnfinished(SmartPtr<CVmConfiguration>& config_, const DiskState& st_)
{
	SmartPtr<IOPackage> y = CommitUnfinished::Command::request(
			m_vm->getVmUuid(), st_, m_task->getRequestPackage());

	WRITE_TRACE(DBG_FATAL, "Send command %s to vm_app, diskState: %s"
		, PVE::DispatcherCommandToString(y->header.type)
		, QSTR2UTF8(st_.toString())
	);

	Answer::Unit< PRL_ERR_VM_DELETE_STATE_FAILED > a;
	PRL_RESULT e = log(wait(m_vm->sendPackageToVm(y) , a));
	if (PRL_FAILED(e))
		return e;

	e = a.result();
	if (PRL_FAILED(e))
		return e;

	if (st_.isDeleteOp())
	{
		Task_CommitUnfinishedDiskOp::removeHalfDeletedSnapshots(config_
			, QSet<QString>() << st_.snapshot());
		CDspVmSnapshotStoreHelper::notifyVmClientsTreeChanged(
			m_task->getRequestPackage(), m_vm->getVmIdent());
	}
	WRITE_TRACE(DBG_FATAL, "Snapshot state was successfully processed.");
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::commitUnfinished()
{
	if (NULL == m_vm.getImpl())
		return PRL_ERR_FAILURE;

	PRL_RESULT e;
	SmartPtr<CVmConfiguration> pVmConfig = m_vm->getVmConfig(m_task->getClient(), e);
	if (!pVmConfig)
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get vm configuration");
		return PRL_ERR_FAILURE;
	}

	// get list of uuids of broken disks states
	QList<DiskState>
		lst = Snapshot::getUnfinishedOps(pVmConfig);

	foreach (const  DiskState&  st, lst)
	{
		PRL_RESULT e = commitUnfinished(pVmConfig, st);
		if (PRL_SUCCEEDED(e))
			continue;

		PRL_RESULT ret = PRL_ERR_DELETE_UNFINISHED_STATE_FAILED;
		WRITE_TRACE(DBG_FATAL, "Commit unfinished state for snapId %s failed with error %s. It is converted to %s."
			, QSTR2UTF8(st.snapshot())
			, PRL_RESULT_TO_STRING(e)
			, PRL_RESULT_TO_STRING(ret)
		);
		return ret;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::create()
{
	PRL_RESULT e = commitUnfinished();
	if (PRL_FAILED(e))
		return e;

	Answer::Unit<PRL_ERR_VM_CREATE_SNAPSHOT_FAILED> a;
	SmartPtr<IOPackage> x = Parallels::DispatcherPackage::createInstance(
				PVE::DspCmdVmCreateSnapshot,
				UTF8_2QSTR(m_task->getRequestPackage()->buffers[0].getImpl()));
	e = log(wait(m_vm->sendPackageToVmEx(log(x), m_state), a));
	if (PRL_FAILED(e))
		return e;
	if (NULL == a.command())
		return a.fail();

	return a.command()->GetRetCode();
}

PRL_RESULT Unit::destroy(const Command::Destroy& command_)
{
	PRL_RESULT e = commitUnfinished();
	if (PRL_FAILED(e))
		return e;

	SmartPtr<IOPackage> y;
	e = command_(y);
	if (PRL_FAILED(e))
		return e;

	Answer::Unit<PRL_ERR_VM_DELETE_STATE_FAILED> a;
	e = log(wait(m_vm->sendPackageToVmEx(log(y), m_state), a));
	if (PRL_FAILED(e))
		return e;
	if (NULL == a.command())
		return a.fail();

	return a.command()->GetRetCode();
}

PRL_RESULT Unit::revert(Revert::Note& note_, const Revert::Command& command_)
{
	PRL_RESULT e = commitUnfinished();
	if (PRL_FAILED(e))
		return e;

	SmartPtr<IOPackage> x = command_.do_();
	if (NULL == x.getImpl())
		return PRL_ERR_FAILURE;

	Revert::Answer a(note_);
	e = log(wait(m_vm->sendPackageToVmEx(log(x), m_state), a));
	if (PRL_FAILED(e) ||  a.command() == NULL)
		return e;

	return a.command()->GetRetCode();
}

namespace Answer
{
///////////////////////////////////////////////////////////////////////////////
// struct Miner

Miner::Miner(IOSendJob::Handle job_):
	m_response(CDspService::instance()->getIOServer().takeResponse(job_))
{
}

Miner::Miner(const IOSendJob::Response& response_): m_response(response_)
{
}

SmartPtr<IOPackage> Miner::operator()() const
{
	if (IOSendJob::Success != m_response.responseResult)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to get response");
		return SmartPtr<IOPackage>();
	}
	SmartPtr<IOPackage> output = m_response.responsePackages[0];
	if (!output.isValid() || output->header.type != PVE::DspWsResponse)
	{
		WRITE_TRACE(DBG_FATAL, "The response is invalid");
		return SmartPtr<IOPackage>();
	}
	return output;
}

} // namespace Answer

namespace CommitUnfinished
{
///////////////////////////////////////////////////////////////////////////////
// struct Command
SmartPtr<IOPackage> Command::request(const QString& vm_,
				const DiskState& st_,
				const SmartPtr<IOPackage>& parent_)
{
	CProtoCommandPtr c = CProtoSerializer::CreateVmCommitDiskUnfinished
					(vm_, st_.disk(), st_.snapshot());
	return Parallels::DispatcherPackage::createInstance(c->GetCommandId(),
					c->GetCommand()->toString(), parent_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Diagnostic

void Diagnostic::inspect(CVmHardDisk& device_)
{
	if (PVE::HardDiskImage != device_.getEmulatedType() || !device_.getEnabled())
		return;

	DiskState st(device_.getSystemName());
	if (st.isUnfinished())
		m_devices.push_back(&device_);
	if (st.isDeleteOp())
			m_snapshots.insert(st.snapshot());
}

Diagnostic::Diagnostic(SmartPtr<CVmConfiguration> config_): m_config(config_)
{
	if (!m_config.isValid())
		return;

	CVmHardware* h = m_config->getVmHardwareList();
	if (NULL == h)
		return;
	foreach (CVmHardDisk* d, h->m_lstHardDisks)
		inspect(*d);
}

} // namespace CommitUnfinished
} // namespace Snapshot

