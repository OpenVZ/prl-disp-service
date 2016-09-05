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
///	CDspVmSnapshotInfrastructure.h
///
/// @brief
///	Declaration of utilities to call the snapshot management inside a vm.
///
/// @author shrike@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmSnapshotInfrastructure_H_
#define __CDspVmSnapshotInfrastructure_H_

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
//#include "Libraries/VirtualDisk/DiskStates.h"  // VirtualDisk commented out by request from CP team

class CDspVm;
class CDspTaskHelper;
namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct DiskState

struct DiskState
{
	DiskState();
	DiskState(const QString& disk_);
	bool isUnfinished() const { return m_unfinished; }
	QString disk() const { return m_disk; }
	QString snapshot() const  { return m_snapshot; }
// VirtualDisk commented out by request from CP team
//	PROCESS_TYPE operation() const { return m_operation; }

	bool isDeleteOp() const;
	QString toString() const;

private:
	bool m_unfinished;
	QString m_disk;
	QString m_snapshot;
// VirtualDisk commented out by request from CP team
//	PROCESS_TYPE m_operation;
};

QList<DiskState>
getUnfinishedOps( const SmartPtr<CVmConfiguration>& config_ );


namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Destroy

struct Destroy
{
	Destroy(const QString& vm_, const QString& uuid_, quint32 flags_, bool child_);

	void step(quint32 value_);
	void merge(bool value_);
	void steps(quint32 value_);
	void config(SmartPtr<CVmConfiguration> value_)
	{
		m_config = value_;
	}
	PRL_RESULT operator()(SmartPtr<IOPackage>& dst_) const;
	static PVE::IDispatcherCommands name()
	{
		return PVE::DspCmdVmDeleteSnapshot;
	}
private:
	Parallels::CProtoDeleteSnapshotCommand& cast();

	CProtoCommandPtr m_command;
	SmartPtr<CVmConfiguration> m_config;
};

} // namespace Command

namespace Answer
{
///////////////////////////////////////////////////////////////////////////////
// struct Miner

struct Miner
{
	explicit Miner(IOSendJob::Handle job_);
	explicit Miner(const IOSendJob::Response& response_);

	SmartPtr<IOPackage> operator()() const;
private:
	IOSendJob::Response m_response;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<PRL_RESULT Fail>
struct Unit
{
	static PRL_RESULT fail()
	{
		return Fail;
	}
	PRL_RESULT operator()(IOSendJob::Handle job_)
	{
		setResult(Miner(job_)());
		if (NULL == m_result.getImpl())
			return Fail;

		return PRL_ERR_SUCCESS;
	}

	Parallels::CProtoCommandDspWsResponse* command() const
	{
		if (NULL == m_result.getImpl())
			return NULL;
		return Parallels::CProtoSerializer::CastToProtoCommand
				<Parallels::CProtoCommandDspWsResponse>(m_command);
	}

	SmartPtr<IOPackage> package() const
	{
		return m_result;
	}

	PRL_RESULT result() const
	{
		if (NULL == m_result.getImpl())
			return Fail;

		CVmEvent v(UTF8_2QSTR(m_result->buffers[0].getImpl()));
		LOG_MESSAGE(DBG_DEBUG, "Received response = [%s]",QSTR2UTF8(v.toString()));
		return v.getEventCode();
	}

	void setResult(PRL_RESULT result_, const SmartPtr<IOPackage>& request_)
	{
		m_command	= CProtoSerializer::CreateDspWsResponseCommand(
			request_, result_);
		m_result =
			DispatcherPackage::createInstance( PVE::DspWsResponse, m_command, request_ );
	}
	void setResult(SmartPtr<IOPackage> value_)
	{
		m_result = value_;
		m_command = CProtoCommandPtr();

		if (NULL == value_.getImpl())
			return;
		m_command = Parallels::CProtoSerializer::ParseCommand(PVE::DspWsResponse,
					UTF8_2QSTR(value_->buffers[0].getImpl()));
	}
private:
	CProtoCommandPtr m_command;
	SmartPtr<IOPackage> m_result;
};
} // namespace Answer

namespace Revert
{
///////////////////////////////////////////////////////////////////////////////
// struct Note

struct Note
{
	Note(): m_isVmStopped(false), m_isUnexpected(false)
	{
	}

	bool isVmStopped() const
	{
		return m_isVmStopped;
	}
	void setVmStopped()
	{
		m_isVmStopped = true;
	}
	bool isUnexpected() const
	{
		return m_isUnexpected;
	}
	void setUnexpected()
	{
		m_isUnexpected = true;
	}
private:
	bool m_isVmStopped;
	bool m_isUnexpected;
};

///////////////////////////////////////////////////////////////////////////////
// struct Answer

struct Answer: Snapshot::Answer::Unit<PRL_ERR_VM_RESTORE_STATE_FAILED>
{
	explicit Answer(Note& note_): m_note(&note_)
	{
	}

	PRL_RESULT operator()(IOSendJob::Handle job_);
private:
	Note* m_note;
};

///////////////////////////////////////////////////////////////////////////////
// struct Command

struct Command
{
	Command(const QString& snapshot_, CDspTaskHelper& task_,
		SmartPtr<CVmConfiguration> config_, SmartPtr<CVmConfiguration> backup_):
		m_snapshot(snapshot_), m_task(&task_), m_config(config_),
		m_backup(backup_)
	{
	}

	SmartPtr<IOPackage> do_() const;
	static PVE::IDispatcherCommands name()
	{
		return PVE::DspCmdVmSwitchToSnapshot;
	}
private:
	void info(CVmEvent& dst_) const;

	QString m_snapshot;
	CDspTaskHelper* m_task;
	SmartPtr<CVmConfiguration> m_config;
	SmartPtr<CVmConfiguration> m_backup;
};

} // namespace Revert

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(const QString& vm_, CDspTaskHelper& task_, VIRTUAL_MACHINE_STATE state_);
	Unit(SmartPtr<CDspVm> vm_, CDspTaskHelper& task_, VIRTUAL_MACHINE_STATE state_):
		m_vm(vm_), m_task(&task_), m_state(state_)
	{
	}

	PRL_RESULT create();
	PRL_RESULT destroy(const Command::Destroy& command_);
	PRL_RESULT revert(Revert::Note& note_, const Revert::Command& command_);
private:
	// wait() returns PRL_ERR_SUCCESS only when valid answer was received from VM.
	// In the other cases it returns error.
	template<class T>
	PRL_RESULT wait(IOSendJob::Handle job_, T& answer_);

	SmartPtr<CDspVm> m_vm;
	CDspTaskHelper* m_task;
	VIRTUAL_MACHINE_STATE m_state;
};

} // namespace Snapshot

#endif // __CDspVmSnapshotInfrastructure_H_

