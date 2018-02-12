////////////////////////////////////////////////////////////////////////////////(
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
/// @file
///  CDspVmDirHelper.cpp
///
/// @brief
///  Implementation of the class CDspVmDirHelper
///
/// @brief
///  This class implements VM Directory managing logic
///
/// @author sergeyt
///  SergeyM
///
/// @date
///  2006-04-04
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>

#include <QMutexLocker>
#include <QMultiHash>
#include <QProcess>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CDspVmDirHelper.h"

#include "CDspCommon.h"
#include "CDspVmManager.h"
#include "CDspService.h"
#include "CDspClient.h"
#include "CDspClientManager.h"
#include "CDspSync.h"
#include "CDspVm.h"
#include "CDspVmStateSender.h"
#include "CDspVNCStarter_p.h"
#include <prlxmlmodel/DispConfig/CDispUser.h>
#include <prlxmlmodel/DispConfig/CDispWorkspacePreferences.h>
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/HostHardwareInfo/CHwFileSystemInfo.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/VmDirectory/CVmDirectoryItem.h>
#include <prlxmlmodel/VmConfig/CVmHardware.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"
#include "CVmValidateConfig.h"
#include "CDspBugPatcherLogic.h"
#include "Stat/CDspStatStorage.h"
#include "EditHelpers/CMultiEditMergeVmConfig.h"
#include "CDspTaskHelper.h"
#include "Tasks/Task_RegisterVm.h"
#include "Tasks/Task_CloneVm.h"
#include "Tasks/Task_MoveVm.h"
#include "Tasks/Task_DeleteVm.h"
#include "Tasks/Task_CreateImage.h"
#include "Tasks/Task_SearchLostConfigs.h"
#include "Tasks/Task_GetInfoFromParallelsUtils.h"
#include "Tasks/Task_ManagePrlNetService.h"
#include "Tasks/Task_DiskImageResizer.h"
#include "Tasks/Task_MigrateVm.h"
#include "Tasks/Task_UpdateVm.h"
#include "Tasks/Task_ConvertDisks.h"
#include "Tasks/Task_EditVm.h"
#include "Tasks/Task_CopyImage.h"
#ifdef _LIN_
#include "Tasks/Task_MountVm.h"
#endif
#include "Tasks/Task_VzManager.h"

#include <prlxmlmodel/ProblemReport/CProblemReport.h>
#include <prlcommon/Messaging/CVmBinaryEventParameter.h>
#include <prlxmlmodel/GuestOsInformation/CVmGuestOsInformation.h>

#include <prlcommon/Logging/Logging.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include <prlcommon/PrlCommonUtilsBase/CSimpleFileHelper.h>

#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"
#include <Libraries/PrlNetworking/netconfig.h>

#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>

#include <prlcommon/HostUtils/HostUtils.h>

#include "CDspService.h"
#include "CDspVzHelper.h"

#include "Tasks/Task_BackgroundJob.h"
#include <QSet>

#ifdef _WIN_
	#include <process.h>
	#define getpid _getpid
#endif

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define PRL_UNKNOWN_NAME	"Unknown name"


#include <prlsdk/PrlEnums.h>

// Build/Current.ver to define EXTERNALLY_AVAILABLE_BUILD macros
#include "Build/Current.ver"

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Reference

struct Reference
{
	typedef PVE::IDispatcherCommands query_type;
	Reference();

	bool isClone(query_type query_) const
	{
		return m_clones.contains(query_);
	}
	bool isSnapshot(query_type query_) const
	{
		return m_snapshots.contains(query_);
	}
	bool hasBackupConflict(query_type query_) const
	{
		return m_backupConflicts.contains(query_);
	}
	bool hasSnapshotConflict(query_type query_) const
	{
		return m_snapshotConflicts.contains(query_);
	}
private:
	typedef QSet<query_type> set_type;

	set_type m_clones;
	set_type m_snapshots;
	set_type m_backupConflicts;
	set_type m_snapshotConflicts;
};

Reference::Reference()
{
	m_clones	<< PVE::DspCmdDirVmClone
			<< PVE::DspCmdDirVmMigrateClone;
	m_snapshots	<< PVE::DspCmdVmCreateSnapshot
			<< PVE::DspCmdVmDeleteSnapshot
			<< PVE::DspCmdVmSwitchToSnapshot
			<< PVE::DspCmdVmUpdateSnapshotData
			<< PVE::DspCmdCtlVmCommitDiskUnfinished;
	m_backupConflicts
			<< PVE::DspCmdCreateVmBackup
			<< PVE::DspCmdRestoreVmBackup
			<< PVE::DspCmdDirVmDelete
			<< PVE::DspCmdDirVmMigrate
			<< PVE::DspCmdDirVmMigrateClone
			<< PVE::DspCmdDirUnregVm
			<< PVE::DspCmdVmCompact
			<< PVE::DspCmdVmResizeDisk
			<< PVE::DspCmdCtlVmEditWithRename
			<< PVE::DspCmdVmConvertDisks
			<< PVE::DspCmdDirVmMove;
	m_snapshotConflicts
			<< PVE::DspCmdDirVmClone
			<< PVE::DspCmdDirVmCloneLinked
			<< PVE::DspCmdDirVmMigrate
			<< PVE::DspCmdDirVmMigrateClone
			<< PVE::DspCmdDirVmDelete
			<< PVE::DspCmdDirUnregVm
			<< PVE::DspCmdDirVmEditCommit
			<< PVE::DspCmdCtlVmEditWithRename
			<< PVE::DspCmdCtlVmEditWithHardwareChanged
			<< PVE::DspCmdCtlVmEditFirewall
			<< PVE::DspCmdCreateVmBackup
			<< PVE::DspCmdRestoreVmBackup
			<< PVE::DspCmdVmResizeDisk
			<< PVE::DspCmdVmCompact
			<< PVE::DspCmdVmConvertDisks
			<< PVE::DspCmdDirCopyImage
			<< PVE::DspCmdDirVmMove;
}

Q_GLOBAL_STATIC(Reference, getReference);
} // namespace

namespace Task
{
namespace Vm
{
namespace Exclusive
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

boost::logic::tribool Unit::isReconciled(const Unit& party_) const
{
	PVE::IDispatcherCommands x = party_.getCommand();
	if (x == m_command)
	{
		switch (x)
		{
		case PVE::DspCmdDirVmClone:
		case PVE::DspCmdDirCopyImage:
		case PVE::DspCmdDirVmMigrateClone:
			return true;
		default:
			return false;
		}
	}
	switch (m_command)
	{
	case PVE::DspCmdDirVmEditCommit:
		switch (x)
		{
		case PVE::DspCmdVmStart:
		case PVE::DspCmdVmResume:
		case PVE::DspCmdVmStartEx:
			return true;
		default:
			break;
		}
		break;

	case PVE::DspCmdVmCompact:
		switch (x)
		{
		case PVE::DspCmdVmStartEx:
			return true;
		default:
			break;
		}
		break;

	case PVE::DspCmdCtlVmEditFirewall:
	case PVE::DspCmdCtlVmEditBootcampReconfigure:
	case PVE::DspCmdCtlVmEditWithHardwareChanged:
		switch (x)
		{
		case PVE::DspCmdVmStartEx:
			break;
		default:
			return true;
		}
		break;
	default:
		if (getReference()->isClone(m_command) && getReference()->isClone(x))
			return true;
	}
	return boost::logic::indeterminate;
}

///////////////////////////////////////////////////////////////////////////////
// struct Newcomer

boost::logic::tribool Newcomer::isReconciled(const Native& native_) const
{
	boost::logic::tribool b = Unit::isReconciled(native_);
	if (!boost::logic::indeterminate(b))
		return b;

	PVE::IDispatcherCommands x = native_.getCommand();
	switch (getCommand())
	{
	case PVE::DspCmdVmLock:
		switch (x)
		{
		case PVE::DspCmdVmStartEx:
			return true;
		default:
			return getSession() == native_.getSession();
		}

	case PVE::DspCmdDirVmEditCommit:
		switch (x)
		{
		case PVE::DspCmdVmCompact:
			return true;
		default:
			break;
		}
		break;

	case PVE::DspCmdCtlApplyVmConfig:
		switch (x)
		{
		case PVE::DspCmdVmLock:
		case PVE::DspCmdDirVmMove:
		case PVE::DspCmdDirVmMigrate:
		case PVE::DspCmdRestoreVmBackup:
			return false;
		default:
			return true;
		}

	case PVE::DspCmdCtlVmEditWithRename:
		switch (x)
		{
		case PVE::DspCmdDirVmEditCommit:
			return true;
		default:
			break;
		}
		break;

	case PVE::DspCmdDirVmMigrate:
	case PVE::DspCmdDirVmMigrateClone:
	case PVE::DspCmdVmCompact:
	case PVE::DspCmdCreateVmBackup:
	case PVE::DspCmdCtlUpdateShadowVm:
		switch (x)
		{
		case PVE::DspCmdVmStartEx:
			return true;
		default:
			break;
		}
		break;

	case PVE::DspCmdRestoreVmBackup:
		switch (x)
		{
		case PVE::DspCmdVmStart:
		case PVE::DspCmdVmResume:
		case PVE::DspCmdVmStartEx:
			return false;
		default:
			return !getReference()->hasBackupConflict(x);
		}

	case PVE::DspCmdVmCreateSnapshot:
	case PVE::DspCmdVmDeleteSnapshot:
	case PVE::DspCmdVmSwitchToSnapshot:
	case PVE::DspCmdCtlVmCommitDiskUnfinished:
		return !getReference()->hasSnapshotConflict(x);

	case PVE::DspCmdVmResizeDisk:
		switch (x)
		{
		case PVE::DspCmdDirVmEditCommit:
			if (getTaskId() == native_.getTaskId())
				return true;
		default:
			break;
		}
		break;

	case PVE::DspCmdVmStartEx:
		switch (x)
		{
		case PVE::DspCmdVmStart:
		case PVE::DspCmdVmResume:
		case PVE::DspCmdDirVmMigrate:
		case PVE::DspCmdDirVmMigrateClone:
		case PVE::DspCmdVmSwitchToSnapshot:
			return true;
		default:
			break;
		}
		break;

	default:
		break;
	}
	return boost::logic::indeterminate;
}

///////////////////////////////////////////////////////////////////////////////
// struct Native

bool Native::isReconciled(const Newcomer& newcomer_) const
{
	boost::logic::tribool b;
	if (!boost::logic::indeterminate(b = Unit::isReconciled(newcomer_)))
		return b;

	if (!boost::logic::indeterminate(b = newcomer_.isReconciled(*this)))
		return b;

	PVE::IDispatcherCommands x = newcomer_.getCommand();
	switch (getCommand())
	{
	case PVE::DspCmdVmLock:
		return getSession() == newcomer_.getSession();

	case PVE::DspCmdCreateVmBackup:
		switch (x)
		{
		case PVE::DspCmdVmCreateSnapshot:
			if (getTaskId() == newcomer_.getTaskId())
			{
				// Allow snapshot creation under CreateVmBackup task
				return true;
			}
		default:
			return !getReference()->hasBackupConflict(x);
		}
		break;

	case PVE::DspCmdRestoreVmBackup:
		switch (x)
		{
		case PVE::DspCmdVmStart:
		case PVE::DspCmdVmResume:
			return false;
		default:
			return !(getReference()->hasBackupConflict(x) ||
							getReference()->isSnapshot(x));
		}
		break;

	case PVE::DspCmdVmStartEx:
		switch (x)
		{
		case PVE::DspCmdVmResume:
			return true;
		default:
			break;
		}
		break;

	default:
		break;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// struct Event

Event::Event(QMutex& mutex_): m_mutex(&mutex_), m_condition(new std::pair<bool, QWaitCondition>())
{
}

void Event::set()
{
	m_condition->first = true;
	m_condition->second.wakeAll();
}

boost::logic::tribool Event::wait()
{
	if (!m_condition->second.wait(m_mutex, TIMEOUT))
		return false;
	if (!m_condition->first)
		return boost::logic::indeterminate;

	m_condition->first = false;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct Conflict

Conflict::Conflict(const Unit& running_, const Unit& pending_, const Event& resolved_):
	m_pending(pending_), m_running(running_), m_resolved(resolved_)
{
}

PRL_RESULT Conflict::getResult() const
{
	switch (m_running.getCommand())
	{
	case PVE::DspCmdDirVmClone:
	case PVE::DspCmdDirVmCloneLinked:
		return PRL_ERR_VM_LOCKED_FOR_CLONE;
	case PVE::DspCmdDirVmDelete:
		return PRL_ERR_VM_LOCKED_FOR_DELETE;
	case PVE::DspCmdDirUnregVm:
		return PRL_ERR_VM_LOCKED_FOR_UNREGISTER;
	case PVE::DspCmdDirVmEditCommit:
	case PVE::DspCmdCtlApplyVmConfig:
		return PRL_ERR_VM_LOCKED_FOR_EDIT_COMMIT;
	case PVE::DspCmdVmStart:
		return PRL_ERR_VM_LOCKED_FOR_EXECUTE;
	case PVE::DspCmdVmStartEx:
		return PRL_ERR_VM_LOCKED_FOR_EXECUTE_EX;
	case PVE::DspCmdDirVmMigrate:
		return PRL_ERR_VM_LOCKED_FOR_MIGRATE;
	case PVE::DspCmdDirVmMigrateClone:
		return PRL_ERR_VM_LOCKED_FOR_MIGRATE;
	case PVE::DspCmdCtlVmEditWithRename:
		return PRL_ERR_VM_LOCKED_FOR_EDIT_COMMIT_WITH_RENAME;
	case PVE::DspCmdVmUpdateSecurity:
		return PRL_ERR_VM_LOCKED_FOR_UPDATE_SECURITY;
	case PVE::DspCmdCtlVmEditWithHardwareChanged:
	case PVE::DspCmdCtlVmEditBootcampReconfigure:
		return PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES;
	case PVE::DspCmdVmCreateSnapshot:
		return PRL_ERR_VM_LOCKED_FOR_CREATE_SNAPSHOT;
	case PVE::DspCmdVmSwitchToSnapshot:
		return PRL_ERR_VM_LOCKED_FOR_SWITCH_TO_SNAPSHOT;
	case PVE::DspCmdVmDeleteSnapshot:
	case PVE::DspCmdCtlVmCommitDiskUnfinished:
		return PRL_ERR_VM_LOCKED_FOR_DELETE_TO_SNAPSHOT;
	case PVE::DspCmdCreateVmBackup:
		return PRL_ERR_VM_LOCKED_FOR_BACKUP;
	case PVE::DspCmdCtlCreateVmBackup:
		return PRL_ERR_VM_LOCKED_CTL_FOR_BACKUP;
	case PVE::DspCmdRestoreVmBackup:
		return PRL_ERR_VM_LOCKED_FOR_RESTORE_FROM_BACKUP;
	case PVE::DspCmdVmLock:
		return PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED;
	case PVE::DspCmdVmResizeDisk:
		return PRL_ERR_VM_LOCKED_FOR_DISK_RESIZE;
	case PVE::DspCmdVmCompact:
		return PRL_ERR_VM_LOCKED_FOR_DISK_COMPACT;
	case PVE::DspCmdVmConvertDisks:
		return PRL_ERR_VM_LOCKED_FOR_DISK_CONVERT;
	case PVE::DspCmdCtlVmEditFirewall:
		return PRL_ERR_VM_LOCKED_FOR_CHANGE_FIREWALL;
	case PVE::DspCmdDirCopyImage:
		return PRL_ERR_VM_LOCKED_FOR_COPY_IMAGE;
	case PVE::DspCmdDirVmMove:
		return PRL_ERR_VM_LOCKED_FOR_MOVE;
	default:
		PRL_ASSERT( PRL_ERR_VM_LOCKED_FOR_INTERNAL_REASON == 0 );
		return PRL_ERR_VM_LOCKED_FOR_INTERNAL_REASON;
	};
}

PRL_RESULT Conflict::operator()()
{
	if (!m_pending.getTaskId().isEmpty() &&
		m_pending.getTaskId() == m_running.getTaskId())
		return getResult();

	boost::logic::tribool w = m_resolved.wait();
	if (boost::logic::indeterminate(w))
		return PRL_ERR_OPERATION_PENDING;
	if (w)
		return PRL_ERR_SUCCESS;

	return getResult();
}

///////////////////////////////////////////////////////////////////////////////
// struct Gang

Conflict* Gang::join(const Unit& party_)
{
	Newcomer n(party_);
	foreach(const Native& x, m_store)
	{
		if (!x.isReconciled(n))
			return new Conflict(x, party_, m_resolver);
	}
	m_store.insert(party_.getCommand(), Native(party_));
	return NULL;
}

bool Gang::eraseFirst(PVE::IDispatcherCommands command_)
{
	store_type::iterator p = m_store.find(command_);
	if (m_store.end() == p)
		return false;

	m_store.erase(p);
	m_resolver.set();
	return true;
}

bool Gang::eraseFirst(PVE::IDispatcherCommands command_, const IOSender::Handle& session_)
{
	store_type::iterator p = m_store.find(command_), e;
	for (e = m_store.end(); p != e && p.key() == command_; ++p)
	{
		if (session_ == p.value().getSession())
		{
			m_store.erase(p);
			m_resolver.set();
			return true;
		}
	}
	return false;
}

const Unit* Gang::findFirst(PVE::IDispatcherCommands command_) const
{
	store_type::const_iterator p = m_store.find(command_);
	if (m_store.end() == p)
		return NULL;

	return &p.value();
}

} // namespace Exclusive
} // namespace Vm
} // namespace Task

using namespace Parallels;

CDspVmDirHelper::ExclusiveVmOperations::ExclusiveVmOperations()
:m_mutex( QMutex::NonRecursive )
{

}

PRL_RESULT	CDspVmDirHelper::ExclusiveVmOperations::registerOp(
	const QString& vmUuid,
	const QString& vmDirUuid,
	PVE::IDispatcherCommands cmd,
	const IOSender::Handle &hSession,
	const QString &sTaskId )
{
	LOG_MESSAGE( DBG_DEBUG, "REGISTER: cmd %s[%#x] for Vm%s by session '%s'",
		PVE::DispatcherCommandToString(cmd),
		cmd,
		QSTR2UTF8( vmUuid + vmDirUuid ),
		QSTR2UTF8( hSession )
		);

	switch ( cmd )
	{
	case PVE::DspCmdDirVmClone:
	case PVE::DspCmdDirVmCloneLinked:
	case PVE::DspCmdDirVmMigrate:
	case PVE::DspCmdDirVmMigrateClone:
	case PVE::DspCmdDirVmDelete:
	case PVE::DspCmdDirUnregVm:
	case PVE::DspCmdVmStart:
	case PVE::DspCmdVmStartEx:
	case PVE::DspCmdDirVmEditCommit:
	case PVE::DspCmdVmDropSuspendedState:
	case PVE::DspCmdCtlVmEditWithRename:
	case PVE::DspCmdVmUpdateSecurity:
	case PVE::DspCmdCtlVmEditWithHardwareChanged:
	case PVE::DspCmdCtlVmEditFirewall:
	case PVE::DspCmdCtlVmEditBootcampReconfigure:
	case PVE::DspCmdVmCreateSnapshot:
	case PVE::DspCmdVmSwitchToSnapshot:
	case PVE::DspCmdVmDeleteSnapshot:
	case PVE::DspCmdCreateVmBackup:
	case PVE::DspCmdRestoreVmBackup:
	case PVE::DspCmdVmResizeDisk:
	case PVE::DspCmdVmCompact:
	case PVE::DspCmdVmLock:
	case PVE::DspCmdVmUpdateSnapshotData:
	case PVE::DspCmdVmConvertDisks:
	case PVE::DspCmdVmMount:
	case PVE::DspCmdVmUmount:
	case PVE::DspCmdDirCopyImage:
	case PVE::DspCmdDirVmMove:
	case PVE::DspCmdCtlVmCommitDiskUnfinished:
	case PVE::DspCmdVmResume:
	case PVE::DspCmdCtlApplyVmConfig:
		break;
	default:
		WRITE_TRACE(DBG_FATAL, "internal error: unsupported incomming cmd %#x", cmd );
		PRL_ASSERT( "unsupported incomming cmd" == 0  );
		return PRL_ERR_INVALID_ARG;
	}//switch

	QString key = makeKey( vmUuid, vmDirUuid );
	QTime b = QTime::currentTime().addMSecs(2*Task::Vm::Exclusive::Event::TIMEOUT);
	forever
	{
		QMutexLocker lock(&m_mutex);
		if (!m_opHash.contains(key))
		{
			m_opHash.insert(key, Task::Vm::Exclusive::Gang(
						Task::Vm::Exclusive::Event(m_mutex)));
		}
		Task::Vm::Exclusive::Gang& g = m_opHash.find(key).value();
		QScopedPointer<Task::Vm::Exclusive::Conflict>
			x(g.join(Task::Vm::Exclusive::Unit(cmd, hSession, sTaskId)));

		if (x.isNull())
			return PRL_ERR_SUCCESS;

		if (b <= QTime::currentTime())
			return x->getResult();

		PRL_RESULT e = (*x)();
		if (PRL_ERR_OPERATION_PENDING == e)
			continue;
		if (PRL_FAILED(e))
			return e;
	}
}

PRL_RESULT	CDspVmDirHelper::ExclusiveVmOperations::unregisterOp(
	const QString& vmUuid,
	const QString& vmDirUuid,
	PVE::IDispatcherCommands cmd,
	const IOSender::Handle &hSession )
{
	LOG_MESSAGE( DBG_DEBUG, "UNREGISTER: cmd %s[%#x] for Vm%s by session '%s'",
		PVE::DispatcherCommandToString(cmd),
		cmd,
		QSTR2UTF8( vmUuid + vmDirUuid ),
		QSTR2UTF8( hSession )
		);

	QMutexLocker lock( &m_mutex );

	QString key = makeKey(vmUuid, vmDirUuid);
	QHash< QString, Task::Vm::Exclusive::Gang>::iterator
		it = m_opHash.find( key );

	if (m_opHash.end() == it)
	{
		WRITE_TRACE(DBG_FATAL, "internal error: unsimmetric "
			"unregisterOp() for vmid+dirId=%s", QSTR2UTF8( key ) );
	}
	else if (PVE::DspCmdVmLock == cmd)
	{
		if (!it.value().eraseFirst(PVE::DspCmdVmLock, hSession))
			return (PRL_ERR_NOT_LOCK_OWNER_SESSION_TRIES_TO_UNLOCK);

		goto ok;
	}
	else if (it.value().eraseFirst(cmd))
		goto ok;
	else
	{
		WRITE_TRACE(DBG_FATAL, " internal error: for key %s, not found command %s[%#x]",
			QSTR2UTF8( key )
			, PVE::DispatcherCommandToString( cmd )
			, cmd );
	}
	return PVE::DspCmdVmLock == cmd ? PRL_ERR_VM_IS_NOT_LOCKED : PRL_ERR_FAILURE;

ok:
	if (it.value().isEmpty())
		m_opHash.erase(it);

	return (PRL_ERR_SUCCESS);
}

PRL_RESULT	CDspVmDirHelper::ExclusiveVmOperations::replaceOp(
	const QString& vmUuid,
	const QString& vmDirUuid,
	PVE::IDispatcherCommands fromCmd,
	PVE::IDispatcherCommands toCmd,
	const IOSender::Handle &hSession)
{
	WRITE_TRACE( DBG_DEBUG, "REPLACE: cmd %s[%#x] on %s[%#x] for Vm%s by session '%s'",
		PVE::DispatcherCommandToString(fromCmd),
		fromCmd,
		PVE::DispatcherCommandToString(toCmd),
		toCmd,
		QSTR2UTF8( vmUuid + vmDirUuid ),
		QSTR2UTF8( hSession )
		);

	QMutexLocker lock( &m_mutex );

	QString key = makeKey( vmUuid, vmDirUuid );
	QHash<QString, Task::Vm::Exclusive::Gang>::iterator
		it = m_opHash.find( key );

	if ( m_opHash.end() == it )
	{
		WRITE_TRACE(DBG_FATAL, "internal error: key not found for replaceOp() for vmid+dirId=%s", QSTR2UTF8( key ) );
		return (PRL_ERR_FAILURE);
	}
	const Task::Vm::Exclusive::Unit* f = it.value().findFirst(fromCmd);
	if (NULL == f)
	{
		WRITE_TRACE(DBG_FATAL, "REPLACE: internal error: for key %s, not found command %s[%#x]",
			QSTR2UTF8( key )
			, PVE::DispatcherCommandToString( fromCmd )
			, fromCmd );
		return (PRL_ERR_FAILURE);
	}
	Task::Vm::Exclusive::Unit u(toCmd, hSession, f->getTaskId());
	it.value().eraseFirst(fromCmd);
	QScopedPointer<Task::Vm::Exclusive::Conflict> x(it.value().join(u));
	if (x.isNull())
		return PRL_ERR_SUCCESS;

	return x->getResult();
}

IOSender::Handle CDspVmDirHelper::ExclusiveVmOperations::getVmLockerHandle( const QString& vmUuid, const QString& vmDirUuid ) const
{
	QMutexLocker lock( &m_mutex );

	QString key = makeKey( vmUuid, vmDirUuid );
	QHash<QString, Task::Vm::Exclusive::Gang>::const_iterator it = m_opHash.find(key);

	if( m_opHash.end() != it )
	{
		const Task::Vm::Exclusive::Unit* u = it.value().findFirst(PVE::DspCmdVmLock);
		if (NULL != u)
			return u->getSession();
	}

	return (IOSender::Handle());
}

void CDspVmDirHelper::ExclusiveVmOperations::cleanupSessionVmLocks( const IOSender::Handle &hSession )
{
	QMutexLocker lock( &m_mutex );
	QHash<QString, Task::Vm::Exclusive::Gang>::iterator it = m_opHash.begin();
	while ( m_opHash.end() != it )
	{
		while(it.value().eraseFirst(PVE::DspCmdVmLock, hSession))
		{
		}
		if (it.value().isEmpty())
			it = m_opHash.erase(it);
		else
			++it;
	}
}

QString CDspVmDirHelper::ExclusiveVmOperations::makeKey( const QString& vmUuid, const QString& vmDirUuid ) const
{
	return vmUuid + vmDirUuid;
}

// constructor
CDspVmDirHelper::CDspVmDirHelper(Registry::Public& registry_, Vm::Directory::Ephemeral& ephemeral_):
	m_registry(registry_), m_ephemeral(&ephemeral_), m_vmMountRegistry(new CDspVmMountRegistry()),
	m_pVmConfigEdit(new CMultiEditMergeVmConfig())
{
}

/**
* @brief Gets VM list for specified user
* @param pUserSession
*/
QList<QString> CDspVmDirHelper::getVmList (
	SmartPtr<CDspClient>& pUserSession ) const
{
	QList<QString> vmList;

	//LOCK before use
	CDspLockedPointer<CVmDirectory>
		pUserDirectory = CDspService::instance()->getVmDirManager()
		.getVmDirectory( pUserSession->getVmDirectoryUuid() );

	if ( !pUserDirectory ) {
		return vmList;
	}

	QStringList lstVmConfigurations;
	for( int iItemIndex = 0; iItemIndex < pUserDirectory->m_lstVmDirectoryItems.size(); iItemIndex++ )
	{
		// get next VM from directory
		CVmDirectoryItem* pDirectoryItem = pUserDirectory->m_lstVmDirectoryItems.at( iItemIndex );

		// Check if user is authorized to access this VM
		if (!CFileHelper::FileCanRead(pDirectoryItem->getVmHome(), &pUserSession->getAuthHelper()))
			continue;

		vmList.append(pDirectoryItem->getVmUuid());
	}

	return vmList;
}

SmartPtr<CVmConfiguration> CDspVmDirHelper::CreateDefaultVmConfigByRcValid(
	SmartPtr<CDspClient> pUserSession, PRL_RESULT rc, const QString& vmUuid)
{
	return ::List::Directory::Item::Inaccessible::Default
		(*CDspService::instance(), pUserSession).craft(rc, vmUuid);
}

void CDspVmDirHelper::sendNotValidState(
										SmartPtr<CDspClient> pUserSession, PRL_RESULT rc, const QString& vmUuid, bool bSetNotValid)
{
	sendNotValidState(pUserSession->getVmDirectoryUuid(),
		rc,
		vmUuid,
		bSetNotValid);
}

/**
* Update vm state on all client.
*/
void CDspVmDirHelper::sendNotValidState(const QString& strVmDirUuid, PRL_RESULT rc
										, const QString& vmUuid, bool bSetNotValid)
{
	if( ! bSetNotValid )
		return;

	switch (rc)
	{
	case PRL_ERR_VM_CONFIG_DOESNT_EXIST:
	case PRL_ERR_PARSE_VM_CONFIG:
	case PRL_ERR_VM_CONFIG_INVALID_VM_UUID:
	case PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID:
		sendVmConfigChangedEvent( strVmDirUuid, vmUuid );
		return;

	default:
		return;
	}
}

SmartPtr<CVmConfiguration> CDspVmDirHelper::CreateVmConfigFromDirItem(
				const QString& sServerUuid, const CVmDirectoryItem* pDirItem)
{
	SmartPtr<CVmConfiguration> pConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	if (!pConfig)
		return pConfig;
	pConfig->getVmIdentification()->setVmUuid(pDirItem->getVmUuid());
	pConfig->getVmIdentification()->setVmName(pDirItem->getVmName());
	pConfig->getVmIdentification()->setServerUuid(sServerUuid);
	pConfig->getVmIdentification()->setHomePath(pDirItem->getVmHome());
	pConfig->getVmIdentification()->setCtId(pDirItem->getCtId());
	pConfig->setVmType(pDirItem->getVmType());
	/* FIXME wrong value is possible:
	 *   - if shared storage connected later than dispatcher started
	 *   - if get_vm_list request comes earlier than information in DirItem
	 *     updated (loaded from VmConfig)
	 */
	pConfig->getVmSettings()->getVmCommonOptions()->setTemplate(pDirItem->isTemplate());
	pConfig->setValidRc(PRL_ERR_SUCCESS);
	return pConfig;
}

CVmIdent CDspVmDirHelper::getVmIdentByVmUuid(const QString &vmUuid_, SmartPtr<CDspClient> userSession_)
{
	return MakeVmIdent(vmUuid_, CDspVmDirHelper::getVmDirUuidByVmUuid(vmUuid_, userSession_));
}

QString CDspVmDirHelper::getVmDirUuidByVmUuid(const QString &vmUuid_, SmartPtr<CDspClient> userSession_)
{
	Q_UNUSED(userSession_);
	Vm::Directory::Dao::Locked x;
	foreach (const CVmDirectory& d, x.getList())
	{
		foreach(CVmDirectoryItem* pDirItem, d.m_lstVmDirectoryItems)
		{
			if (pDirItem->getVmUuid() == vmUuid_)
				return d.getUuid();
		}
	}
	return QString();
}

/**
* @brief Sends VM list.
* @param sender
* @param pUserSession
* @param pRequestParams
* @return
*/
bool CDspVmDirHelper::sendVmList(const IOSender::Handle& sender,
								 SmartPtr<CDspClient> pUserSession,
								 const SmartPtr<IOPackage>& pkg )
{
	Q_UNUSED( sender );
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}
	CDspService* s = CDspService::instance();
	quint32 nFlags = cmd->GetCommandFlags();
	QString sServerUuid = s->getDispConfigGuard().getDispConfig()
				->getVmServerIdentification()->getServerUuid();

	::List::Directory::Factory::ephemeral_type e = m_ephemeral->snapshot();

	QStringList dirUuids;
	dirUuids.append(pUserSession->getVmDirectoryUuidList());
	dirUuids.append(e.toList());

	QStringList lstVmConfigurations;
	QScopedPointer< ::List::Directory::Chain> x
		(::List::Directory::Factory(*s, e)(pUserSession, nFlags));
	if (!x.isNull())
	{
		foreach (const QString& u, dirUuids)
		{
			CDspLockedPointer<CVmDirectory>
				d = s->getVmDirManager().getVmDirectory(u);
			if (!d.isValid())
			{
				WRITE_TRACE(DBG_FATAL, "No VM Directory assigned for "
					PRODUCT_NAME_SHORT " user with sessionId = [%s] ",
					qPrintable(pUserSession->getClientHandle()));

				pUserSession->sendSimpleResponse(pkg, PRL_ERR_VM_DIRECTORY_NOT_EXIST);
				return false;
			}
			CVmDirectory y(d.getPtr());
			d.unlock();
			x->handle(y);
		}
		lstVmConfigurations = x->getResult();
	}
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetParamsList( lstVmConfigurations );

	pUserSession->sendResponse( pCmd, pkg );

	return true;
}

/**
* @brief Sends VM configuration by name or UUID.
* @param sender
* @param pUserSession
* @param pRequestParams
* @return
*/
bool CDspVmDirHelper::findVm(const IOSender::Handle& sender,
								   SmartPtr<CDspClient> pUserSession,
								   const SmartPtr<IOPackage>& pkg )
{
	LOG_MESSAGE( DBG_FATAL,  "CDspVmDirHelper::findVm()" );

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}

	QString sSearchId = cmd->GetFirstStrParam();
	quint32 nFlags = cmd->GetCommandFlags();

	if ( !(nFlags & PGVC_SEARCH_BY_UUID || nFlags & PGVC_SEARCH_BY_NAME) )
		nFlags |= PGVC_SEARCH_BY_UUID;

	QString sVmUuid;
	PRL_VM_TYPE nVmType;
	{
		CDspVmDirManager &dirMgr = CDspService::instance()->getVmDirManager();
		/* Lock VM dir catalogue */
		CDspLockedPointer<CVmDirectories> pLockedDirs = dirMgr.getVmDirCatalogue();
		CVmDirectoryItem *pItem = NULL;
		foreach (const QString &vmDirUuid, pUserSession->getVmDirectoryUuidList() +
			m_ephemeral->snapshot().toList()) {
			if (nFlags & PGVC_SEARCH_BY_UUID)
				pItem = dirMgr.getVmDirItemByUuid(vmDirUuid, sSearchId).getPtr();
			if (pItem == NULL && nFlags & PGVC_SEARCH_BY_NAME)
				pItem = dirMgr.getVmDirItemByName(vmDirUuid, sSearchId).getPtr();
			if (pItem != NULL)
				break;
		}

		if( pItem == NULL ) {
			pUserSession->sendSimpleResponse(pkg, PRL_ERR_VM_UUID_NOT_FOUND);
			return false;
		}

		sVmUuid = pItem->getVmUuid();
		nVmType = pItem->getVmType();
	}

	if ( nVmType == PVT_VM )
	{
		return sendVmConfigByUuid(sender, pUserSession, pkg, sVmUuid);
	}
	else if ( nVmType == PVT_CT )
	{
		return CDspService::instance()->getVzHelper()->sendCtConfigByUuid(sender, pUserSession, pkg, sVmUuid);
	}
	else
	{
		PRL_ASSERT(0);
		pUserSession->sendSimpleResponse(pkg, PRL_ERR_VM_UUID_NOT_FOUND);
		return false;
	}
}



/**
* @brief Sends VM configuration.
* @param sender
* @param pUserSession
* @param pRequestParams
* @return
*/
bool CDspVmDirHelper::sendVmConfig(const IOSender::Handle& sender,
								   SmartPtr<CDspClient> pUserSession,
								   const SmartPtr<IOPackage>& pkg )
{
	LOG_MESSAGE( DBG_FATAL,  "CDspVmDirHelper::sendVmConfig()" );

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}

	return sendVmConfigByUuid(sender, pUserSession, pkg, cmd->GetVmUuid());
}

bool CDspVmDirHelper::sendVmConfigByUuid ( const IOSender::Handle& sender,
							SmartPtr<CDspClient> pUserSession,
							const SmartPtr<IOPackage>& pkg,
							QString vm_uuid )
{
	// AccessCheck
	SmartPtr<CVmConfiguration> pVmConfig(0);

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}

	quint32 nFlags = cmd->GetCommandFlags();

	CVmEvent evt;
	PRL_RESULT
		rc = CDspService::instance()->getAccessManager().checkAccess(
			pUserSession, PVE::DspCmdVmGetConfig, vm_uuid, NULL, &evt );
	if(PRL_SUCCEEDED(rc) )
	{
		PRL_RESULT error = PRL_ERR_SUCCESS;
		pVmConfig = getVmConfigByUuid(pUserSession, vm_uuid, error);
		if (!pVmConfig)
		{
			PRL_ASSERT( PRL_FAILED(error) );
			pUserSession->sendSimpleResponse(pkg, error);
			return false;
		}
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Access check failed for user {%s} when accessing VM {%s}. Reason: %#x (%s)"
			, __FUNCTION__
			, QSTR2UTF8(pUserSession->getClientHandle())
			, QSTR2UTF8(vm_uuid), rc, PRL_RESULT_TO_STRING(rc)
			);

		if (rc != PRL_ERR_ACCESS_TO_VM_DENIED)
		{
			pVmConfig = CreateDefaultVmConfigByRcValid(pUserSession, rc, vm_uuid);
		}

		if (!pVmConfig)
		{
			pUserSession->sendResponseError(evt, pkg);
			return false;
		}
	}

	try
	{
		PRL_ASSERT( pVmConfig );
		if( !pVmConfig )
			throw PRL_ERR_UNEXPECTED;

		bool bFillAutogenerated = nFlags & PGVC_FILL_AUTOGENERATED;
		fillOuterConfigParams( pUserSession, pVmConfig, bFillAutogenerated );

		////////////////////////////////////////////////////////////////////////
		// prepare response
		////////////////////////////////////////////////////////////////////////
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse
			*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
		pResponseCmd->SetVmConfig( pVmConfig->toString() );

		SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
		CDspService::instance()->getIOServer().sendPackage( sender, responsePkg );

	}
	catch (PRL_RESULT code)
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred while getting VM configuration with code [%#x] (%s)"
			, code, PRL_RESULT_TO_STRING( code ) );
		CDspService::instance()->sendSimpleResponseToClient( sender, pkg, code );

		return false;
	}

	return true;
}

void CDspVmDirHelper::fillVmState(
	SmartPtr<CDspClient> pUserSession
	, const QString& sVmUuid
	, CVmEvent& outVmEvent )
{
	CVmIdent ident(CDspVmDirHelper::getVmIdentByVmUuid(sVmUuid, pUserSession));

	// VMS_STOPPED is the only possible state for template
	VIRTUAL_MACHINE_STATE s = ident.second == CDspVmDirManager::getTemplatesDirectoryUuid() ?
		VMS_STOPPED : CDspVm::getVmState(ident.first, ident.second);

	outVmEvent.addEventParameter(
		new CVmEventParameter(PVE::Integer,
			QString("%1").arg(s),
			EVT_PARAM_VMINFO_VM_STATE)
		);
}

/**
* @brief fill VM info event.
* @param pUserSession
* @param pVmSecurity
* @param outVmEvent
* @return
*/
PRL_RESULT CDspVmDirHelper::fillVmInfo(
	SmartPtr<CDspClient> pUserSession
	, const QString& vm_uuid
	, CVmSecurity* pVmSecurity
	, CVmEvent& outVmEvent )
{
	PRL_ASSERT( pUserSession );

	outVmEvent.setDefaults();
	//#462822 to prevent merge conflicts ( after setDefaults they are uuids)
	outVmEvent.setEventIssuerId();
	outVmEvent.setInitRequestId();

	SmartPtr< CVmSecurity > pTmpVmSecurity;

	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	CVmIdent ident(CDspVmDirHelper::getVmIdentByVmUuid(vm_uuid, pUserSession));
	{
		// LOCK inside brackets
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem(CDspService::instance()->getVmDirManager().getVmDirItemByUuid(ident));

		if ( !pVmDirItem )
			return PRL_ERR_VM_UUID_NOT_FOUND;

		rc = CDspService::instance()->getAccessManager()
			.checkAccess(pUserSession, PVE::DspCmdGetVmInfo, pVmDirItem.getPtr(), &bSetNotValid);

		if( PRL_FAILED(rc) )
		switch(rc)
		{
			case PRL_ERR_VM_CONFIG_DOESNT_EXIST:
			case PRL_ERR_PARSE_VM_CONFIG:
			case PRL_ERR_VM_CONFIG_INVALID_VM_UUID:
			case PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID:
			case PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED:
				break;

			default:
				return rc;
		}

		if( !pVmSecurity )
		{
			pTmpVmSecurity = FillVmSecurity(pUserSession, pVmDirItem.getPtr());
			pVmSecurity = pTmpVmSecurity.getImpl();
		}
	}
	sendNotValidState(pUserSession, rc, vm_uuid, bSetNotValid);

	outVmEvent.addEventParameter(
		new CVmEventParameter( PVE::Integer
		, QString("%1").arg( PRL_FAILED( rc ) )
		, EVT_PARAM_VMINFO_VM_IS_INVALID )
		);

	outVmEvent.addEventParameter(
		new CVmEventParameter( PVE::Integer
		, QString("%1").arg( rc )
		, EVT_PARAM_VMINFO_VM_VALID_RC )
		);

	fillVmState(pUserSession, vm_uuid, outVmEvent);

	PRL_ASSERT( pVmSecurity );
	outVmEvent.addEventParameter(
		new CVmEventParameter( PVE::String, pVmSecurity->toString(), EVT_PARAM_VMINFO_VM_SECURITY )
		);

	PRL_RESULT err = PRL_ERR_FAILURE;
	SmartPtr<CVmConfiguration> c = getVmConfigByUuid(pUserSession, vm_uuid, err);
	VIRTUAL_MACHINE_STATE s(VMS_STOPPED);
	if (c && !c->getVmSettings()->getVmCommonOptions()->isTemplate())
		s = CDspVm::getVmState( vm_uuid, ident.second);
	bool bIsVncServerStarted = s == VMS_RUNNING
		&& PRL_SUCCEEDED(err)
		&& c->getVmSettings()->getVmRemoteDisplay()->getMode()
			!= PRD_DISABLED;

	outVmEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt
		, QString("%1").arg(bIsVncServerStarted)
		, EVT_PARAM_VMINFO_IS_VNC_SERVER_STARTED ));

	VIRTUAL_MACHINE_ADDITION_STATE addidionalState = VMAS_NOSTATE;
	if (c && !c->getVmSettings()->getVmCommonOptions()->isTemplate())
		addidionalState = CDspVm::getVmAdditionState(vm_uuid, ident.second);

	outVmEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt
		, QString("%1").arg(addidionalState)
		, EVT_PARAM_VMINFO_VM_ADDITION_STATE ));

	outVmEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt
		, QString("%1").arg(false)
		, EVT_PARAM_VMINFO_IS_VM_WAITING_FOR_ANSWER ));

	return PRL_ERR_SUCCESS;
}

/**
* @brief Sends VM info.
* @param sender
* @param pUserSession
* @param pRequestParams
* @return
*/
bool CDspVmDirHelper::sendVmInfo(const IOSender::Handle& sender,
								 SmartPtr<CDspClient> pUserSession,
								 const SmartPtr<IOPackage>& pkg )
{
	LOG_MESSAGE( DBG_INFO,  "CDspVmDirHelper::getVmInfo()" );

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}

	QString vm_uuid = cmd->GetVmUuid();

	CVmEvent retCont;
	PRL_RESULT rc = fillVmInfo( pUserSession, vm_uuid, 0, retCont );
	if( PRL_FAILED(rc) )
	{
		WRITE_TRACE(DBG_WARNING, "fillVmInfo failed: error #%x, %s", rc, PRL_RESULT_TO_STRING(rc) );
		pUserSession->sendSimpleResponse( pkg, rc );
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	// prepare response
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetVmEvent( retCont.toString() );

	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
	CDspService::instance()->getIOServer().sendPackage( sender, responsePkg );

	return true;
}


/**
* @brief Sends VM Tools info.
* @param sender
* @param pUserSession
* @param pRequestParams
* @return
*/
bool CDspVmDirHelper::sendVmToolsInfo(const IOSender::Handle& sender,
									  SmartPtr<CDspClient> pUserSession,
									  const SmartPtr<IOPackage>& pkg )
{
	LOG_MESSAGE( DBG_INFO,  "CDspVmDirHelper::getVmToolsInfo()" );

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}

	QString vm_uuid = cmd->GetVmUuid();

	// Check access right
	CVmEvent evt;
	PRL_RESULT
		err = CDspService::instance()->getAccessManager().checkAccess(
				pUserSession, PVE::DspCmdGetVmToolsInfo, vm_uuid, NULL, &evt);
	if ( PRL_FAILED(err) )
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Access check failed for user {%s} when accessing VM {%s}. Reason: %#x (%s)"
			, __FUNCTION__
			, QSTR2UTF8(pUserSession->getClientHandle())
			, QSTR2UTF8(vm_uuid), err, PRL_RESULT_TO_STRING(err)
			);
		pUserSession->sendResponseError(evt, pkg);
		return false;
	}

	// Get tools state
	QString toolsVersion;
	PRL_VM_TOOLS_STATE toolsState = m_registry.find(vm_uuid).getToolsState();
	SmartPtr<CVmConfiguration> pVmConfig =
		getVmConfigByUuid(pUserSession, vm_uuid, err);
	if (pVmConfig) {
		toolsVersion = pVmConfig->getVmSettings()->getVmTools()->getAgentVersion();
		// override tools state for templates from config
		if (!toolsVersion.isEmpty() && toolsState == PTS_NOT_INSTALLED)
			toolsState = PTS_POSSIBLY_INSTALLED;
	}

	////////////////////////////////////////////////////////////////////////
	// prepare response
	////////////////////////////////////////////////////////////////////////

	CVmEvent rev;
	rev.addEventParameter( new CVmEventParameter(PVE::Integer
				, QString::number(toolsState)
				, EVT_PARAM_VM_TOOLS_STATE ));
	rev.addEventParameter( new CVmEventParameter(PVE::String
				, toolsVersion
				, EVT_PARAM_VM_TOOLS_VERSION ));

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetVmEvent( rev.toString() );

	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
	CDspService::instance()->getIOServer().sendPackage( sender, responsePkg );

	return true;
}

/**
* @brief Create new VM and register in catalog.
* @param pRequestParams
* @return
*/
void CDspVmDirHelper::createNewVm(const IOSender::Handle& sender,
								  SmartPtr<CDspClient> pUserSession,
								  const SmartPtr<IOPackage>& pkg )
{

	Q_UNUSED( sender );
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	CProtoVmCreateCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCreateCommand>(cmd);
	QString vm_rootDir 	= pVmCmd->GetVmHomePath();
	QString vm_config 	= pVmCmd->GetVmConfig();
	bool bForceQuestionsSign = pVmCmd->GetForceQuestionsSign();

	// Prepare and start long running task helper
	CDspService::instance()->getTaskManager().schedule
		(new Task_RegisterVm(m_registry, pUserSession, pkg, vm_config,  vm_rootDir, bForceQuestionsSign));
}

namespace {
	/**
	* Searches for a VM configuration file at specified VM home dir
	* @param VM home dir path
	* @param path to the found VM configuration file
	*/
	QString FoundPvsFileAtVmDir(const QString &sVmDirPath)
	{
		QDir _vm_dir(sVmDirPath);
		QFileInfoList _dir_entries = _vm_dir.entryInfoList(QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Files);
		foreach(QFileInfo _entry, _dir_entries)
			if (_entry.suffix() == "pvs")
				return (_entry.absoluteFilePath());
		return ("");
	}

}

/**
* @brief Register existing Vm in catalog.
* @param pRequestParams
* @return
*/
void CDspVmDirHelper::registerVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg )
{

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	QString pathToVmDir = cmd->GetFirstStrParam();
	QString sCustomVmUuid = cmd->GetVmUuid();
	quint32 nFlags = cmd->GetCommandFlags();

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pUserSession->getAuthHelper() );

	// Check whether specified path is directory.
	// If it's not then convert it to directory path in view that absolute path to registering
	// VM configuration file was passed
	QFileInfo _fi(pathToVmDir);
	if (!_fi.isDir())
		pathToVmDir = _fi.absolutePath();

	if ( ! QFileInfo(pathToVmDir).exists() )
	{
		CVmEvent evt;
		evt.setEventCode( PRL_ERR_DIRECTORY_DOES_NOT_EXIST );
		evt.addEventParameter(new CVmEventParameter( PVE::String, pathToVmDir, EVT_PARAM_MESSAGE_PARAM_0 ));
		pUserSession->sendResponseError( evt, pkg );
		return;
	}

	//Check whether specified path writable for service (if it's not writable
	//then potentially it's readonly filesystem)
	// FIX IT - readonly filesystem param can get by statfs64!!!!
	QString sCheckFilePath = pathToVmDir + '/' + Uuid::createUuid().toString();
	QFile _check_file(sCheckFilePath);
	if (!_check_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_READONLY_FILESYSTEM );
		return;
	}
	_check_file.close();
	QFile::remove(sCheckFilePath);

	// Prepare and start long running task helper
	CDspService::instance()->getTaskManager().schedule
		(new Task_RegisterVm(m_registry, pUserSession, pkg, pathToVmDir, nFlags, sCustomVmUuid));
}

void CDspVmDirHelper::restoreVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}
	QString vmUuid = cmd->GetVmUuid();
	QString vmDirUuid = pUserSession->getVmDirectoryUuid();

	CDspLockedPointer<CVmDirectoryItem> pItem
		= CDspService::instance()->getVmDirManager()
			.getVmDirItemByUuid( vmDirUuid, vmUuid );
	if ( ! pItem.isValid() )
	{
		WRITE_TRACE(DBG_FATAL, "VM uuid = %s not found", QSTR2UTF8(vmUuid));
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_VM_UUID_NOT_FOUND );
		return;
	}

	CVmEvent evt;
	QString vm_xml_path = pItem->getVmHome();

// Check VM config

	PRL_RESULT nRes = CDspService::instance()->getAccessManager()
						.checkAccess( pUserSession, PVE::DspCmdDirRestoreVm, pItem.getPtr(), NULL, &evt );
	if (PRL_SUCCEEDED(nRes))
	{
		WRITE_TRACE(DBG_FATAL, "VM config '%s' already valid", QSTR2UTF8(vm_xml_path));
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_VM_CONFIG_IS_ALREADY_VALID );
		return;
	}
	else if (   nRes != PRL_ERR_VM_CONFIG_DOESNT_EXIST
			 && nRes != PRL_ERR_PARSE_VM_CONFIG)
	{
		WRITE_TRACE(DBG_FATAL, "Restore VM operation canceled due %.8X '%s' error code\
					was	received on check access rights",
					nRes, PRL_RESULT_TO_STRING(nRes));
		pUserSession->sendResponseError( evt, pkg );
		return;
	}

// Check VM backup config

	CVmDirectoryItem item;
	item.fromString(pItem->toString());
	item.setVmHome( vm_xml_path + VMDIR_DEFAULT_VM_BACKUP_SUFFIX );

	nRes = CDspService::instance()->getAccessManager()
			.checkAccess( pUserSession, PVE::DspCmdDirRestoreVm, &item, NULL, &evt );
	if (PRL_FAILED(nRes))
	{
		WRITE_TRACE(DBG_FATAL, "Restore VM operation canceled due %.8X '%s' error code\
					was	received on check access rights",
					nRes, PRL_RESULT_TO_STRING(nRes));
		sendVmConfigChangedEvent(vmDirUuid, vmUuid, pkg);
		pUserSession->sendResponseError( evt, pkg );
		return;
	}

	nRes = CDspService::instance()->getVmConfigManager()
				.restoreConfig(vm_xml_path, pUserSession, item.getRegisteredBy());
	if (PRL_FAILED(nRes))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot restore VM config '%s'!", QSTR2UTF8(vm_xml_path));
		sendVmConfigChangedEvent(vmDirUuid, vmUuid, pkg);
		pUserSession->sendSimpleResponse( pkg, nRes );
		return;
	}

	SmartPtr<CVmConfiguration> pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	nRes = CDspService::instance()->getVmConfigManager()
				.loadConfig(pVmConfig, vm_xml_path, pUserSession, true);
	PRL_ASSERT(PRL_SUCCEEDED(nRes));
	if (PRL_FAILED(nRes))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot load VM config '%s' after restoring!", QSTR2UTF8(vm_xml_path));
		pUserSession->sendSimpleResponse( pkg, nRes );
		return;
	}

	appendAdvancedParamsToVmConfig( pUserSession, pVmConfig, true );

// Send config changed events

	sendVmConfigChangedEvent(vmDirUuid, vmUuid, pkg);
	sendEventVmSecurityChanged(vmDirUuid, vmUuid, pkg);

// Send success response

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse*
		pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetVmConfig( pVmConfig->toString() );

	pUserSession->sendResponse( pCmd, pkg );
}

void CDspVmDirHelper::sendVmRemovedEvent(const CVmIdent& vmIdent, PRL_EVENT_TYPE type_,
		const SmartPtr<IOPackage> &pRequest)
{
	// Generate "VM Deleted" event
	CVmEvent event(type_, vmIdent.first, PIE_DISPATCHER);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, event, pRequest);

	//////////////////////////////////////////////////////////////////////////
	// Send to all vm dir clients because
	// 1) #120118 vm was removed manualy
	// 2) #265595 user has access denied after registration this vm by root.
	// 3) This code is very simpler than any other.
	QList< SmartPtr<CDspClient> > allVmDirClients = CDspService::instance()->getClientManager()
		.getSessionsListSnapshot(vmIdent.second).values();

	CDspService::instance()->getClientManager()
		.sendPackageToClientList(p, allVmDirClients);
}

void CDspVmDirHelper::sendVmConfigChangedEvent(const CVmIdent& vmIdent, const SmartPtr<IOPackage> &pRequest)
{
	CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, vmIdent.first, PIE_DISPATCHER );
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event, pRequest );
	CDspService::instance()->getClientManager()
		.sendPackageToVmClients( p, vmIdent.second, vmIdent.first );
	CDspLockedPointer<CDspVmStateSender> x(CDspService::instance()->getVmStateSender());
	if (x.isValid())
		x->onVmConfigChanged(vmIdent.second, vmIdent.first);
}

void CDspVmDirHelper::sendVmConfigChangedEvent(const QString& vmDirUuid, const QString& vmUuid, const SmartPtr<IOPackage> &pRequest)
{
	sendVmConfigChangedEvent(MakeVmIdent(vmUuid, vmDirUuid), pRequest);
}

//
// delete VM from disk
//
void CDspVmDirHelper::deleteVm(const IOSender::Handle& sender,
							   SmartPtr<CDspClient> pUserSession,
							   const SmartPtr<IOPackage>& pkg )
{

	Q_UNUSED( sender );
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong package in deleteVm()");
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}
	CProtoVmDeleteCommand * pDeleteVmCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeleteCommand>(cmd);
	QString vm_uuid = pDeleteVmCmd->GetVmUuid();

	// AccessCheck
	PRL_RESULT err = PRL_ERR_FAILURE;
	CVmEvent _error;
	bool bSetNotValid = false;
	SmartPtr<CVmConfiguration> pVmConfig;

	err = CDspService::instance()->getAccessManager().checkAccess(pUserSession, PVE::DspCmdDirVmDelete,
		vm_uuid, &bSetNotValid, &_error);
	if (PRL_FAILED(err))
	{
		sendNotValidState(pUserSession, err, vm_uuid, bSetNotValid);
		if (err != PRL_ERR_VM_CONFIG_DOESNT_EXIST)
		{
			pUserSession->sendResponseError(_error, pkg);
			WRITE_TRACE(DBG_FATAL, "Delete VM operation canceled due %.8X '%s' error code "
			"was received on check access rights", err, PRL_RESULT_TO_STRING(err));
			return;
		}
		pVmConfig = CreateDefaultVmConfigByRcValid(pUserSession, err, vm_uuid);
		WRITE_TRACE(DBG_WARNING, "Delete VM: Configuration file for VM '%s' not found. Unregistering the VM.", QSTR2UTF8(vm_uuid));
		CVmEvent event(PET_DSP_EVT_VM_MESSAGE, vm_uuid, PIE_DISPATCHER, PRL_WARN_DELETE_NO_CONFIG_UNREGISTER);
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, pkg);
		pUserSession->sendPackage(pPackage);
		unregOrDeleteVm(pUserSession, pkg, pVmConfig->toString(), PVD_UNREGISTER_ONLY);
		return;
	}

	QStringList strFilesToDelete = pDeleteVmCmd->GetVmDevicesList();

	pVmConfig = getVmConfigByUuid(pUserSession, vm_uuid, err, &_error);

	if (!pVmConfig )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to get configuration for '%s'", QSTR2UTF8(vm_uuid));
		PRL_ASSERT ( PRL_FAILED( err ) );
		pUserSession->sendResponseError( _error, pkg );
		return;
	}

	unregOrDeleteVm(pUserSession, pkg, pVmConfig->toString(), 0, strFilesToDelete);
}

//
// Unregister VM
//
void CDspVmDirHelper::unregVm(
							  const IOSender::Handle& sender,
							  SmartPtr<CDspClient> pUserSession,
							  const SmartPtr<IOPackage>& pkg )
{

	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	QString vm_uuid = cmd->GetVmUuid();


	SmartPtr<CVmConfiguration> pVmConfig;

	// AccessCheck
	PRL_RESULT err = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	err = CDspService::instance()->getAccessManager().checkAccess( pUserSession, PVE::DspCmdDirUnregVm
		, vm_uuid, &bSetNotValid );
	if ( ! PRL_SUCCEEDED(err) )
	{
		sendNotValidState(pUserSession, err, vm_uuid, bSetNotValid);
		bool flgDenyToUnregConfig = (
				PRL_ERR_VM_CONFIG_DOESNT_EXIST == err
				&& ! pUserSession->getAuthHelper().isLocalAdministrator()
				);

		pVmConfig =  CreateDefaultVmConfigByRcValid( pUserSession, err, vm_uuid );

		if ( !pVmConfig || flgDenyToUnregConfig )
		{
			pUserSession->sendSimpleResponse( pkg, PRL_ERR_CANT_REMOVE_INVALID_VM_AS_NON_ADMIN );
			return;
		}
	}
	else
		pVmConfig = getVmConfigByUuid ( pUserSession, vm_uuid, err );

	if ( ! pVmConfig )
	{
		PRL_ASSERT ( PRL_FAILED( err ) );
		CDspService::instance()->sendSimpleResponseToClient( sender, pkg, err );
		return;
	}

	unregOrDeleteVm( pUserSession, pkg, pVmConfig->toString(), PVD_UNREGISTER_ONLY );
}

bool CDspVmDirHelper::atomicEditVmConfigByVm(
	const QString &vmDirUuid,
	const QString& vmUuid,
	const CVmEvent& evtFromVm,
	SmartPtr<CDspClient> pUserSession
	)
{
	return Task_EditVm::atomicEditVmConfigByVm(vmDirUuid, vmUuid, evtFromVm, pUserSession);
}


void CDspVmDirHelper::beginEditVm(const IOSender::Handle& sender,
								  SmartPtr<CDspClient> pUserSession,
								  const SmartPtr<IOPackage>& pkg )
{
	Task_EditVm::beginEditVm(sender, pUserSession, pkg);
}

//
// Edit VM
//
void CDspVmDirHelper::editVm(const IOSender::Handle& ,
							 SmartPtr<CDspClient> pUserSession,
							 const SmartPtr<IOPackage>& pkg )
{
	CDspService::instance()->getTaskManager().schedule(new Task_EditVm( pUserSession, pkg ));
}

/**
* @brief Create new image file.
* @param pRequestParams
* @return
*/
void CDspVmDirHelper::createNewImage(const IOSender::Handle& sender,
									 SmartPtr<CDspClient> pUserSession,
									 const SmartPtr<IOPackage>& pkg )
{

	Q_UNUSED( sender );
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	// AccessCheck
	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	CVmEvent evt;
	rc = CDspService::instance()->getAccessManager().checkAccess( pUserSession, PVE::DspCmdDirCreateImage
		, cmd->GetVmUuid(), &bSetNotValid, &evt );
	if ( ! PRL_SUCCEEDED(rc) )
	{
		sendNotValidState(pUserSession, rc, cmd->GetVmUuid(), bSetNotValid);
		pUserSession->sendResponseError( evt, pkg );
		return;
	}

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> pVmConfig = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(pUserSession, cmd->GetVmUuid(), ret);
	if ( !pVmConfig )
	{
		PRL_ASSERT( PRL_FAILED(ret) );
		if( !PRL_FAILED( ret ) )
			ret = PRL_ERR_FAILURE;

		WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for UUID '%s'", QSTR2UTF8(cmd->GetVmUuid()));
		pUserSession->sendSimpleResponse( pkg, ret );
		return;
	}
	// append additional parameters from VM directory
	// (VM home, last change date, last modification date, ... )
	// it's necessary for correct relative paths of VM device processing
	appendAdvancedParamsToVmConfig( pUserSession, pVmConfig );

	CProtoCreateImageCommand
		*pCreateImageCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateImageCommand>(cmd);
	QString image_config = pCreateImageCmd->GetImageConfig();
	bool bRecreateIsAllowed = pCreateImageCmd->IsRecreateAllowed();
	bool bForceQuestionsSign = pCreateImageCmd->GetForceQuestionsSign();

	// Prepare and start long running task helper
	CDspService::instance()->getTaskManager()
		.schedule(new Task_CreateImage( pUserSession
			, pkg
			, pVmConfig
			, image_config
			, bRecreateIsAllowed
			, bForceQuestionsSign ));

}

void CDspVmDirHelper::copyImage(SmartPtr<CDspClient> pUserSession,
								const SmartPtr<IOPackage>& p)
{
	CDspService::instance()->getTaskManager().schedule(new Task_CopyImage(pUserSession, p));
}

//////////////////////////////////////////////////////////////////////////
// prepare parameters for vm clone
void CDspVmDirHelper::cloneVm(const IOSender::Handle& sender,
							  SmartPtr<CDspClient> pUserSession,
							  const SmartPtr<IOPackage>& pkg )
{

	Q_UNUSED( sender );
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	CProtoVmCloneCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCloneCommand>(cmd);

	QString strVmUuid = pVmCmd->GetVmUuid();

	// AccessCheck
	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	CVmEvent evt;
	rc = CDspService::instance()->getAccessManager()
		.checkAccess( pUserSession, PVE::DspCmdDirVmClone, strVmUuid, &bSetNotValid, &evt );
	if ( ! PRL_SUCCEEDED(rc) )
	{
		sendNotValidState(pUserSession, rc, strVmUuid, bSetNotValid);
		pUserSession->sendResponseError( evt, pkg );
		return;
	}

	QString strVmNewRootPath = pVmCmd->GetVmHomePath();
	QString strVmNewName = pVmCmd->GetVmName();
	QString sNewVmUuid = pVmCmd->GetNewVmUuid();
	unsigned int nFlags = pVmCmd->GetCommandFlags();

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration>
		pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(pUserSession, strVmUuid, ret);
	if( !pVmConfig )
	{
		PRL_ASSERT( PRL_FAILED(ret) );
		if( !PRL_FAILED( ret ) )
			ret = PRL_ERR_FAILURE;

		WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for UUID '%s'", QSTR2UTF8(cmd->GetVmUuid()));
		pUserSession->sendSimpleResponse( pkg, ret );
		return;
	}

	// Prepare and start long running task helper
	CDspService::instance()->getTaskManager().schedule
		(new Task_CloneVm(m_registry, pUserSession, pkg, pVmConfig, strVmNewName,
			sNewVmUuid, strVmNewRootPath, nFlags));
}//CDspVmDirHelper::cloneVm

void CDspVmDirHelper::migrateVm ( const IOSender::Handle&,
								 SmartPtr<CDspClient> pUserSession,
								 const SmartPtr<IOPackage> &pkg )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_UNRECOGNIZED_REQUEST );
		return;
	}

	CProtoVmMigrateCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	QString strVmUuid = pVmCmd->GetVmUuid();
	QString strVmDirUuid = pUserSession->getVmDirectoryUuid();

	// AccessCheck
	PRL_RESULT rc = PRL_ERR_FAILURE;
	bool bSetNotValid = false;
	CVmEvent evt;
	rc = CDspService::instance()->getAccessManager()
			.checkAccess( pUserSession, PVE::DspCmdDirVmMigrate, strVmUuid, &bSetNotValid, &evt );
	if ( PRL_FAILED(rc) )
	{
		sendNotValidState(pUserSession, rc, strVmUuid, bSetNotValid);
		pUserSession->sendResponseError( evt, pkg );
		return;
	}

	if (pVmCmd->GetCommandFlags() & PVM_UPDATE_MODE)
	{
		CDspService::instance()->getTaskManager()
			.schedule(new Task_UpdateVm(pUserSession, cmd, pkg));
	}
	else
	{
		CDspService::instance()->getTaskManager()
			.schedule(new Task_MigrateVmSource(m_registry,
						pUserSession, cmd, pkg));
	}
}

void CDspVmDirHelper::lockVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p )
{
	CVmEvent evtErr;
	try
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
		if ( ! cmd->IsValid() )
			throw PRL_ERR_UNRECOGNIZED_REQUEST;

		QString strVmUuid = cmd->GetVmUuid();

		// AccessCheck
		PRL_RESULT rc = PRL_ERR_FAILURE;
		bool bSetNotValid = false;
		rc = CDspService::instance()->getAccessManager()
			.checkAccess( pUserSession, PVE::DspCmdVmLock, strVmUuid, &bSetNotValid, &evtErr );
		if ( PRL_FAILED(rc) )
		{
			sendNotValidState(pUserSession, rc, strVmUuid, bSetNotValid);
			throw rc;
		}

		rc = lockVm( strVmUuid, pUserSession, p );
		throw rc;
	}
	catch (PRL_RESULT rc)
	{
		if ( PRL_FAILED(rc) )
			WRITE_TRACE(DBG_FATAL, "VM lock failed with code: %.8X, '%s'", rc, PRL_RESULT_TO_STRING(rc));
		evtErr.setEventCode(rc);
		pUserSession->sendResponseError( evtErr, p );
	}
}

PRL_RESULT CDspVmDirHelper::lockVm(
			const QString& vmUuid,
			const SmartPtr<CDspClient> &pUserSession,
			const SmartPtr<IOPackage> &p)
{
	PRL_RESULT rc;

	rc = registerExclusiveVmOperation(
				vmUuid, pUserSession->getVmDirectoryUuid(), PVE::DspCmdVmLock, pUserSession);
	if ( PRL_FAILED(rc) ) {
		WRITE_TRACE(DBG_FATAL, "VM lock failed with code: %.8X, '%s'", rc, PRL_RESULT_TO_STRING(rc));
	} else {
		WRITE_TRACE(DBG_INFO, "VM locked '%s'", QSTR2UTF8(vmUuid));
		/* send event to GUI (PMC) to change Vm state to Locked (and reread config) */
		CVmEvent event(PET_DSP_EVT_VM_CONFIG_CHANGED, vmUuid, PIE_DISPATCHER);
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);
		CDspService::instance()->getClientManager().sendPackageToVmClients(
						pPackage, pUserSession->getVmDirectoryUuid(), vmUuid);
	}
	return rc;
}

void CDspVmDirHelper::unlockVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p )
{
	CVmEvent evtErr;
	try
	{
		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
		if ( ! cmd->IsValid() )
			throw PRL_ERR_UNRECOGNIZED_REQUEST;

		QString strVmUuid = cmd->GetVmUuid();

		// AccessCheck
		PRL_RESULT rc = PRL_ERR_FAILURE;
		bool bSetNotValid = false;
		rc = CDspService::instance()->getAccessManager()
			.checkAccess( pUserSession, PVE::DspCmdVmUnlock, strVmUuid, &bSetNotValid, &evtErr );
		if ( PRL_FAILED(rc) )
		{
			sendNotValidState(pUserSession, rc, strVmUuid, bSetNotValid);
			throw rc;
		}

		rc = unlockVm( strVmUuid, pUserSession, p );
		throw rc;
	}
	catch (PRL_RESULT rc)
	{
		if ( PRL_FAILED(rc) )
			WRITE_TRACE(DBG_FATAL, "VM unlock failed with code: %.8X, '%s'", rc, PRL_RESULT_TO_STRING(rc));
		evtErr.setEventCode(rc);
		pUserSession->sendResponseError( evtErr, p );
	}
}

PRL_RESULT CDspVmDirHelper::unlockVm(
			const QString& vmUuid,
			const SmartPtr<CDspClient> &pUserSession,
			const SmartPtr<IOPackage> &p)
{
	PRL_RESULT rc;

	rc = unregisterExclusiveVmOperation(
			vmUuid, pUserSession->getVmDirectoryUuid(), PVE::DspCmdVmLock, pUserSession);
	if ( PRL_FAILED(rc) ) {
		WRITE_TRACE(DBG_FATAL, "VM lock failed with code: %.8X, '%s'", rc, PRL_RESULT_TO_STRING(rc));
	} else {
		WRITE_TRACE(DBG_INFO, "VM unlocked '%s'", QSTR2UTF8(vmUuid));
		/* send event to clients (PMC) to change Vm state from Locked (#455964) and reread config */
		CVmEvent event(PET_DSP_EVT_VM_CONFIG_CHANGED, vmUuid, PIE_DISPATCHER);
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);
		CDspService::instance()->getClientManager().sendPackageToVmClients(
						pPackage, pUserSession->getVmDirectoryUuid(), vmUuid);
	}
	return rc;
}

void CDspVmDirHelper::searchLostConfigs(
										SmartPtr<CDspClient> pUserSession,
										const SmartPtr<IOPackage>& pkg )
{
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(pkg);
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	CProtoStartSearchConfigCommand
		*pSearchConfigsCmd = CProtoSerializer::CastToProtoCommand<CProtoStartSearchConfigCommand>( cmd );

	CDspService::instance()->getTaskManager()
		.schedule(new Task_SearchLostConfigs( pUserSession, pkg, pSearchConfigsCmd->GetSearchDirs()));
}

// get problem report for stopped vm
bool CDspVmDirHelper::UpdateDeviceInfo (const IOSender::Handle& sender,
										SmartPtr<CDspClient> pUser,
										const SmartPtr<IOPackage>&p )
{
	///////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(p);
	if ( ! cmd->IsValid() )
	{
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return false;
	}

	QString strDev = cmd->GetFirstStrParam();

	CVmDevice cDevice;
	StringToElement<CVmDevice*>(&cDevice,strDev);
	switch(cDevice.getDeviceType())
	{
	case PDE_HARD_DISK:
		break;
	default:
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return false;
	}

	SmartPtr<CVmHardDisk> lpcHard( new CVmHardDisk );
	StringToElement<CVmHardDisk*>(lpcHard.getImpl(),strDev);
	QList<CVmHardDisk*> lstDisks;
	lstDisks << lpcHard.getImpl();

	PRL_RESULT res = PRL_ERR_SUCCESS;
	// updated hard disk image path must ends with .hdd
	if ( (lpcHard->getEmulatedType() == PVE::HardDiskImage) &&
		 !lpcHard->getSystemName().endsWith( ".hdd" ) &&
		 !lpcHard->getSystemName().endsWith( ".hdd/" ) &&
		 !lpcHard->getSystemName().endsWith( ".vmdk" ) )
	{
		lpcHard->setVersion( PVE::HardDiskInvalidVersion );
	}
	else
	{
		// Leave it for compatibility, anyway this API is deprecated.
		res = PRL_ERR_UNIMPLEMENTED;
	}

	// create response command
	if (PRL_FAILED(res))
	{
		switch(res)
		{
		case PRL_CHECKED_DISK_OLD_VERSION:
		case PRL_CHECKED_DISK_INVALID:
		case PRL_ERR_VM_GET_HDD_IMG_NOT_OPEN:
			break;
		default:
			WRITE_TRACE(DBG_FATAL, "Unknown error returns %#x %s for disk '%s'"
				, res
				, PRL_RESULT_TO_STRING( res )
				, QSTR2UTF8( lpcHard->getSystemName() ) );
			res = PRL_ERR_CANT_UPDATE_DEVICE_INFO;
		}
	}
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, res );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

	// create xml representation of device
	QString strhddXML = ElementToString<CVmHardDisk*>(
		lpcHard.getImpl(), XML_VM_CONFIG_EL_HARD_DISK );

	pResponseCmd->SetVmDevice( strhddXML );

	SmartPtr<IOPackage> responsePkg =
		DispatcherPackage::createInstance( PVE::DspWsResponse, pResponseCmd, p );

	CDspService::instance()->getIOServer().sendPackage( sender, responsePkg );

	return true;
}


void CDspVmDirHelper::updateVmSecurityInfo ( SmartPtr<CDspClient> pUserSession
											, const SmartPtr<IOPackage>&p )
{
	////////////////////////////////////////////////////////////////////////
	// retrieve user parameters from request data
	////////////////////////////////////////////////////////////////////////

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(p);
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// FIXME: Need add check with CDspVmDirManager::checkAccess()
	//////////////////////////////////////////////////////////////////////////

	CProtoVmUpdateSecurityCommand
		*pVmUpdateSecurityCmd = CProtoSerializer::CastToProtoCommand<CProtoVmUpdateSecurityCommand>( cmd );

	QString vm_uuid = cmd->GetVmUuid();
	QString sVmSecurity = pVmUpdateSecurityCmd->GetVmSecurity();

	bool flgExclusiveOperationWasRegistred = false;

	PRL_RESULT ret = PRL_ERR_FAILURE;
	SmartPtr< CVmSecurity>  pVmSecurity( new CVmSecurity );
	CVmEvent ws_error;
	ws_error.setEventType( PET_DSP_EVT_ERROR_MESSAGE );
	try
	{
		// #9860
		ret = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			vm_uuid
			, pUserSession->getVmDirectoryUuid()
			, cmd->GetCommandId()
			, pUserSession );
		if( PRL_FAILED( ret ) )
			throw ret;
		flgExclusiveOperationWasRegistred = true;

		if( int err = pVmSecurity->fromString(sVmSecurity) )
		{
			WRITE_TRACE(DBG_FATAL, "Invalid Data in error %#x, CVmSecurity = %s"
				, err
				, QSTR2UTF8( pVmSecurity->toString() ) );
			throw PRL_ERR_FAILURE;
		}

		CDspLockedPointer< CVmDirectoryItem >
			pVmDirItem =  CDspService::instance()->getVmDirManager()
			.getVmDirItemByUuid( pUserSession->getVmDirectoryUuid(), vm_uuid );
		if( !pVmDirItem )
			throw PRL_ERR_VM_UUID_NOT_FOUND;

		ret = updateVmSecurityInfo( pUserSession, pVmDirItem.getPtr(), pVmSecurity );
		if( PRL_FAILED( ret ) )
		{
			if ( ret == PRL_ERR_CANT_CHANGE_FILE_PERMISSIONS
				|| ret == PRL_ERR_ACCESS_DENIED_TO_CHANGE_PERMISSIONS
				|| ret == PRL_ERR_CANT_TO_CHANGE_PERMISSIONS_ON_REMOTE_LOCATION )
			{
				ws_error.addEventParameter(
					new CVmEventParameter( PVE::String, pVmDirItem->getVmName(),
					EVT_PARAM_MESSAGE_PARAM_0));
			}
			throw ret;
		}

		ret = PRL_ERR_SUCCESS;

		pVmSecurity = FillVmSecurity( pUserSession, pVmDirItem.getPtr() );
	}
	catch( PRL_RESULT err )
	{
		ret = err;

		WRITE_TRACE(DBG_FATAL, "%s was failed err=%#x (%s) : vm_uuid '%s'"
			, __FUNCTION__
			, err
			, PRL_RESULT_TO_STRING( err )
			, QSTR2UTF8( vm_uuid ) );
	}

	if( flgExclusiveOperationWasRegistred )
	{
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			vm_uuid
			, pUserSession->getVmDirectoryUuid()
			, cmd->GetCommandId()
			, pUserSession );
	}

	if (PRL_FAILED(ret))
	{
		ws_error.setEventCode( ret );

		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, ret );
		CProtoCommandDspWsResponse *pResponseCmd
			= CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
		pResponseCmd->SetError( ws_error.toString() );

		SmartPtr<IOPackage> responsePkg =
			DispatcherPackage::createInstance( PVE::DspWsResponse, pResponseCmd, p );
		pUserSession->sendPackage( responsePkg );
	}
	else
	{
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, ret );
		CProtoCommandDspWsResponse
			*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
		pResponseCmd->SetVmSecurity( pVmSecurity->toString() );

		SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, p );
		pUserSession->sendPackage( responsePkg );

		// Generate "VM security changed" event for all active sessions
		sendEventVmSecurityChanged( pUserSession->getVmDirectoryUuid(), vm_uuid, p );
	}
}

/**
* @brief Gets VM list
*/
QMultiHash<QString, SmartPtr<CVmConfiguration> >
CDspVmDirHelper::getAllVmList (const QString& vmDirUuid) const
{
	QMultiHash<QString, SmartPtr<CVmConfiguration> > vmHash;

	QString sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()
				->getVmServerIdentification()->getServerUuid();

	SmartPtr<CDspClient> pFakeUserSession = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
	pFakeUserSession->getAuthHelper().AuthUserBySelfProcessOwner();
	Vm::Directory::Dao::Locked x;
	foreach (const CVmDirectory& d, x.getList())
	{
		if (!vmDirUuid.isEmpty() && d.getUuid() != vmDirUuid)
		{
			continue;
		}

		if (d.getUuid() == CDspVmDirManager::getTemplatesDirectoryUuid())
			continue;

		QStringList lstVmConfigurations;
		foreach( CVmDirectoryItem* pDirectoryItem, d.m_lstVmDirectoryItems )
		{
#ifdef _CT_
			if (pDirectoryItem->getVmType() != PVT_VM)
				continue;
#endif
			PRL_RESULT err = PRL_ERR_SUCCESS;
			SmartPtr<CVmConfiguration> pVmConfig = getVmConfigForDirectoryItem( pDirectoryItem, err, pFakeUserSession );
			if( PRL_SUCCEEDED( err ) && pVmConfig )
			{
				if (sServerUuid != pVmConfig->getVmIdentification()->getServerUuid())
					pVmConfig->setValidRc(PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID);
				vmHash.insert( d.getUuid(), pVmConfig );
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Failed getAllVmList() for by error [%#x,%s] by VmId = %s ",
					err,
					PRL_RESULT_TO_STRING( err ),
					QSTR2UTF8( pDirectoryItem->getVmUuid() ) );
			}
		}
	}

	return vmHash;
}


/**
* @brief Initialize VM Directory.
* @return
*/
bool CDspVmDirHelper::createUsersVmDirectory( SmartPtr<CDspClient> pUserSession )
{
	PRL_ASSERT( pUserSession );
	if ( ! pUserSession )
		return false;

	QString strVmDirectoryUuid;
	QString strUserSettingsUuid;
	{
		// Very long method. It is good reason to unlock pointer in destructor.
		CDspLockedPointer<CDispUser> p_user = CDspService::instance()->getDispConfigGuard()
			.getDispUserByUuid( pUserSession->getUserSettingsUuid() );

		if ( !p_user )
		{
			WRITE_TRACE(DBG_FATAL, "%s: can't found user with uuid [%s]"
				, __FUNCTION__
				, QSTR2UTF8( pUserSession->getUserSettingsUuid() )
				);

			PRL_ASSERT( 0 );

			return false;
		}

		strVmDirectoryUuid = p_user->getUserWorkspace()->getVmDirectory();
		strUserSettingsUuid = p_user->getUserId();
	}

	if( ! CDspService::instance()->getVmDirManager().getVmDirectory( strVmDirectoryUuid ) )
	{
		WRITE_TRACE(DBG_FATAL, "Error: not exist vmDir in VmDirCatalogue for dir_uuid [%s]", QSTR2UTF8( strVmDirectoryUuid ) );
		return false;
	}

	QString strVmDirectoryName = getVmRootPathForUser( strVmDirectoryUuid, strUserSettingsUuid );

	////////////////////////////////////////////////////////////////////////
	// check if VM Directory catalog exists
	////////////////////////////////////////////////////////////////////////

	QDir vm_dir( strVmDirectoryName );
	if( !vm_dir.exists() )
	{
		// directory does not exist
		WRITE_TRACE(DBG_FATAL, "VM Directory %s does not exists." , QSTR2UTF8( vm_dir.path() ) );

		QString strVmHomeOfDirectory;
		{
			CDspLockedPointer<CVmDirectory>
				pVmDirectory = CDspService::instance()->getVmDirManager().getVmDirectory( strVmDirectoryUuid );

			PRL_ASSERT( pVmDirectory );
			strVmHomeOfDirectory = pVmDirectory->getDefaultVmFolder();
		}

		// #127473 to prevent create directory on external unmounted disk
		// #431558 compare paths by spec way to prevent errors with symlinks, unexisting files, ...
		if( !CFileHelper::IsPathsEqual( strVmHomeOfDirectory, strVmDirectoryName ) )
		{
			WRITE_TRACE(DBG_FATAL, "Ignore to create user defined vm directory %s",
				QSTR2UTF8( strVmDirectoryName )
				);
		}
	}

	return true;
}

/**
* @brief Get VM configuration by request.
* Here we retrieve VM configuration from VM Catalog as VM Directory item.
* @param pRequestParams
* @return
*/
SmartPtr<CVmConfiguration>
CDspVmDirHelper::getVmConfigByUuid(
								   SmartPtr<CDspClient> pUserSession,
								   const QString& vmUuid,
								   PRL_RESULT& error,
								   CVmEvent *pErrorInfo
								   )
{
	error = PRL_ERR_SUCCESS;
	////////////////////////////////////////////////////////////////////////////
	// get corresponding directory item
	////////////////////////////////////////////////////////////////////////////

	//LOCK before use
	CDspLockedPointer<CVmDirectoryItem> pVmDirItem(CDspService::instance()
			->getVmDirManager().getVmDirItemByUuid(CDspVmDirHelper::getVmIdentByVmUuid(vmUuid, pUserSession)));
	if (pVmDirItem)
		return getVmConfigForDirectoryItem(pUserSession, pVmDirItem.operator->(), error, pErrorInfo);

	error = PRL_ERR_VM_UUID_NOT_FOUND;
	return SmartPtr<CVmConfiguration>();
}

/**
* @brief Get VM configuration by VM and parent VM dir UUIDs.
* Here we retrieve VM configuration from VM Catalog as VM Directory item.
* @return
*/
SmartPtr<CVmConfiguration>
CDspVmDirHelper::getVmConfigByUuid (
									const QString &sVmDirUuid,
									const QString& vm_uuid,
									PRL_RESULT&	outError,
									bool bAbsolute,
									bool bLoadConfigDirectlyFromDisk)
{
	outError = PRL_ERR_SUCCESS;
	////////////////////////////////////////////////////////////////////////////
	// get corresponding directory item
	////////////////////////////////////////////////////////////////////////////

	//LOCK before use
	CDspLockedPointer<CVmDirectoryItem>
		pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid( sVmDirUuid, vm_uuid );

	if( !pVmDirItem )
	{
		outError = PRL_ERR_VM_UUID_NOT_FOUND;
		return SmartPtr<CVmConfiguration>();
	}

	SmartPtr<CVmConfiguration> pVmConfig = getVmConfigForDirectoryItem( pVmDirItem.operator->(), outError, bAbsolute, bLoadConfigDirectlyFromDisk);

	return pVmConfig;
}

/**
* @brief Get VM configuration for specific Directory item.
* @param pUserSession
*/
SmartPtr<CVmConfiguration>
CDspVmDirHelper::getVmConfigForDirectoryItem(SmartPtr<CDspClient> pUserSession,
											 CVmDirectoryItem* pDirectoryItem ,
											 PRL_RESULT& error,
											 CVmEvent *pErrorInfo )
{
	error = PRL_ERR_SUCCESS;

	PRL_ASSERT( pDirectoryItem );
	CDspAccessManager::VmAccessRights
		permissionToVm = CDspService::instance()->getAccessManager()
		.getAccessRightsToVm( pUserSession, pDirectoryItem );

	if( ! permissionToVm.canRead() )
	{
		error = permissionToVm.isExists()
			? PRL_ERR_ACCESS_TO_VM_DENIED
			: PRL_ERR_VM_CONFIG_DOESNT_EXIST;
		if (pErrorInfo)
		{
			pErrorInfo->setEventCode(error);
			pErrorInfo->addEventParameter(
				new CVmEventParameter( PVE::String, pDirectoryItem->getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
			pErrorInfo->addEventParameter(
				new CVmEventParameter( PVE::String, pDirectoryItem->getVmHome(), EVT_PARAM_MESSAGE_PARAM_1));
		}
		return SmartPtr<CVmConfiguration>();
	}
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	PRL_ASSERT(pUserSession);
	CAuthHelperImpersonateWrapper _impersonate( &pUserSession->getAuthHelper() );

	return getVmConfigForDirectoryItem ( pDirectoryItem, error );
}

/**
* @brief Get VM configuration for specific Directory item.
* @param pUserSession
*/
SmartPtr<CVmConfiguration>
CDspVmDirHelper::getVmConfigForDirectoryItem(CVmDirectoryItem* pDirectoryItem, PRL_RESULT& error, bool bAbsolute, bool bLoadConfigDirectlyFromDisk )
{
	error = PRL_ERR_SUCCESS;

	if( !pDirectoryItem )
	{
		error = PRL_ERR_INVALID_ARG;
		return SmartPtr<CVmConfiguration>();
	}

	// VM config XML exists - try to read and interpret
	SmartPtr<CVmConfiguration> pVmConfig( new CVmConfiguration() );

	// get config file
	PRL_RESULT rc = PRL_ERR_SUCCESS;

	if (pDirectoryItem->getVmType() == PVT_CT)
	{ 
		pVmConfig = CDspService::instance()->getVzHelper()->getCtConfig(
				SmartPtr<CDspClient>(0),
				pDirectoryItem->getVmUuid(),
				pDirectoryItem->getVmHome());
		if (!pVmConfig)
			rc = PRL_ERR_VM_GET_CONFIG_FAILED;
	}
	else
	{
		rc = CDspService::instance()->getVmConfigManager().loadConfig(pVmConfig,
				pDirectoryItem->getVmHome(),
				SmartPtr<CDspClient>(0),
				bAbsolute,
				bLoadConfigDirectlyFromDisk
				);
	}

	if( !IS_OPERATION_SUCCEEDED(rc) )
	{
		error = PRL_ERR_PARSE_VM_CONFIG;
		pVmConfig = SmartPtr<CVmConfiguration>();
	}

	return pVmConfig;
}

// get VM directory item by uuid
QPair<const QString, CDspLockedPointer<CVmDirectoryItem> >
	CDspVmDirHelper::getVmDirectoryItemByUuid
		(SmartPtr<CDspClient> pUserSession, const QString& vmUuid)
{
	typedef QPair<const QString, CDspLockedPointer<CVmDirectoryItem> > result_type;

	if (pUserSession.isValid())
	{
		foreach (const QString& d, pUserSession->getVmDirectoryUuidList())
		{
			result_type::second_type x = getVmDirectoryItemByUuid(d, vmUuid);
			if (x.isValid())
				return result_type(d, x);
		}
	}

	return result_type(QString(), getVmDirectoryItemByUuid(QString(), vmUuid));
}


// get VM directory item by uuid
CDspLockedPointer<CVmDirectoryItem> CDspVmDirHelper::getVmDirectoryItemByUuid (
	const QString &sVmDirUuid,
	const QString& vmUuid )
{
	return CDspService::instance()->getVmDirManager().getVmDirItemByUuid( sVmDirUuid, vmUuid );
}


/**
* @brief Insert VM directory item.
* @param sDirUuid
* @param vm_item
*/
PRL_RESULT CDspVmDirHelper::insertVmDirectoryItem(const QString &sDirUuid, CVmDirectoryItem* vm_item)
{
	PRL_RESULT res = CDspService::instance()
		->getVmDirManager().addVmDirItem( sDirUuid, vm_item );

	if ( ! PRL_SUCCEEDED( res ) )
		WRITE_TRACE(DBG_FATAL, "addVmDirItem() failed by error %x (%s)", res, PRL_RESULT_TO_STRING( res ) );

	return res;
}

/**
* @brief delete VM directory item.
* @param sDirUuid
* @param vm_uuid
*/
PRL_RESULT CDspVmDirHelper::deleteVmDirectoryItem(const QString &sDirUuid, const QString& vm_uuid)
{
	PRL_RESULT res = CDspService::instance()
		->getVmDirManager().deleteVmDirItem( sDirUuid, vm_uuid );

	if ( ! PRL_SUCCEEDED( res ) )
		WRITE_TRACE(DBG_FATAL, "deleteVmDirItem() failed by error %x (%s)", res, PRL_RESULT_TO_STRING( res ) );

	return res;
}

//
// Unregister and delete VM from disk
//
CDspTaskFuture<Task_DeleteVm> CDspVmDirHelper::unregOrDeleteVm( SmartPtr<CDspClient> pUserSession,
									  const SmartPtr<IOPackage>& pkg,
									  const QString& vm_config ,
									  PRL_UINT32 flags,
									  const QStringList & strFilesToDelete)
{
	return CDspService::instance()->getTaskManager()
		.schedule(new Task_DeleteVm(pUserSession, pkg, vm_config, flags, strFilesToDelete));
}


QString CDspVmDirHelper::CreateVmDirCatalogueEntry( const QString& vmDirPath, const QString& vmDirName)
{
	Uuid	vmdirUuid = Uuid::createUuid();
	CVmDirectory* pVmDir = new CVmDirectory( vmdirUuid, vmDirPath, vmDirName );

	PRL_RESULT res = CDspService::instance()->getVmDirManager().addNewVmDirectory( pVmDir );

	if ( ! PRL_SUCCEEDED( res ) )
	{
		WRITE_TRACE(DBG_FATAL, "addNewVmDirectory() failed by error %x (%s)", res, PRL_RESULT_TO_STRING( res ) );
		if( pVmDir )
			delete pVmDir;
		return "";
	}

	return vmdirUuid.toString();
}

//===========================================
QString CDspVmDirHelper::getVmRootPathForUser ( SmartPtr<CDspClient> pUserSession )
{
	return getVmRootPathForUser( pUserSession->getVmDirectoryUuid(), pUserSession->getUserSettingsUuid() );
}

QString CDspVmDirHelper::getVmRootPathForUser ( const QString& dirUuid, const QString& dispUserUuid )
{
	QString strVmHomeOfDirectory;
	QString strVmHomeOfUser;

	{
		CDspLockedPointer<CVmDirectory>
			pVmDirectory = CDspService::instance()->getVmDirManager().getVmDirectory( dirUuid );

		PRL_ASSERT( pVmDirectory );
		if( pVmDirectory )
			strVmHomeOfDirectory = pVmDirectory->getDefaultVmFolder();
	}

	{
		CDspLockedPointer<CDispUser> p_user = CDspService::instance()->getDispConfigGuard()
			.getDispUserByUuid( dispUserUuid );

		PRL_ASSERT(p_user);
		if( p_user )
			strVmHomeOfUser = p_user->getUserWorkspace()->getDefaultVmFolder();
	}

	return  strVmHomeOfUser.isEmpty()
		? strVmHomeOfDirectory
		: strVmHomeOfUser;
}

/**
* append additional parameters from VM directory
* (VM home, last change date, last modification date, ....)
*/
void CDspVmDirHelper::appendAdvancedParamsToVmConfig(
	SmartPtr<CDspClient> pUserSession,
	SmartPtr<CVmConfiguration> pOutVmConfig
	//https://bugzilla.sw.ru/show_bug.cgi?id=439944
	, bool bSynchronizeFileColor,
	bool bFillAutogenerated
	)
{
	if ( ! pOutVmConfig )
		return;

	CVmIdent ident(CDspVmDirHelper::getVmIdentByVmUuid(pOutVmConfig->getVmIdentification()->getVmUuid(), pUserSession));
	// LOCK inside brackets
	{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem(CDspService::instance()->getVmDirManager()
				.getVmDirItemByUuid(ident));

		if (!pVmDirItem)
			return;

		pOutVmConfig->getVmIdentification()->setHomePath(pVmDirItem->getVmHome());
		pOutVmConfig->getVmIdentification()->setModifierName(pVmDirItem->getChangedBy());
		pOutVmConfig->getVmIdentification()->setLastModifDate(pVmDirItem->getChangeDateTime());
		pOutVmConfig->getVmIdentification()->setCtId(QString::number(Uuid::toVzid(ident.first)));

		Q_UNUSED(bSynchronizeFileColor);

		{
			//https://bugzilla.sw.ru/show_bug.cgi?id=267152
			CAuthHelperImpersonateWrapper _impersonate(&pUserSession->getAuthHelper());
			pOutVmConfig->getVmIdentification()->setVmFilesLocation(
				CFileHelper::GetVmFilesLocationType(pVmDirItem->getVmHome()));
		}

		SmartPtr< CVmSecurity> pVmSecurity = FillVmSecurity(pUserSession, pVmDirItem.getPtr());

		bool bParentalControlEnabled = pOutVmConfig->getVmSecurity()->isParentalControlEnabled();
		QString qsStamp = pOutVmConfig->getVmSecurity()->getFieldPatchedValue(PARENTAL_CONTROL_ENABLED_STR);
		pOutVmConfig->setVmSecurity(new CVmSecurity(pVmSecurity.getImpl()));
		pOutVmConfig->getVmSecurity()->setParentalControlEnabled(bParentalControlEnabled);
		pOutVmConfig->getVmSecurity()->markPatchedField(PARENTAL_CONTROL_ENABLED_STR, qsStamp);

		pOutVmConfig->getVmSettings()->getLockDown()->setHash(pVmDirItem->getLockDown()->getEditingPasswordHash());
	}

	// #463792 add vmInfo
	CVmEvent vmInfo;
	PRL_RESULT rcVmInfo
		= fillVmInfo(pUserSession, ident.first , pOutVmConfig->getVmSecurity(), vmInfo);
	if( PRL_FAILED(rcVmInfo) )
		WRITE_TRACE(DBG_FATAL, "fillVmInfo failed: error #%x, %s", rcVmInfo, PRL_RESULT_TO_STRING(rcVmInfo) );
	else
	{
		pOutVmConfig->getVmSettings()->getVmRuntimeOptions()
			->getInternalVmInfo()->setParallelsEvent( new CVmEvent(&vmInfo) );
	}

	// We've started to store default host interface names in config
	// and this argument become unused.
	Q_UNUSED(bFillAutogenerated);

	LOG_MESSAGE( DBG_DEBUG, "After adding advanced parameters config: \n%s"
		, QSTR2UTF8( pOutVmConfig->toString() )
		);


	//Update VM uptime
	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
    SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid(ident);
	if ( pVm )
		pOutVmConfig->getVmIdentification()->setVmUptimeInSeconds(
			pOutVmConfig->getVmIdentification()->getVmUptimeInSeconds() +
			pVm->getVmUptimeInSecs()
		);

	if (pOutVmConfig->getVmSettings()->getVmRemoteDisplay()->getMode() == PRD_DISABLED)
		pOutVmConfig->getVmSettings()->getVmRemoteDisplay()->setPortNumber(0);

	LOG_MESSAGE( DBG_DEBUG, "After adding advanced parameters config: \n%s"
		, QSTR2UTF8( pOutVmConfig->toString() )
		);
}

/**
* reset additional parameters in VM configuration
* (last change date, last modification date - never store in VM configuration itself!)
*/
void CDspVmDirHelper::resetAdvancedParamsFromVmConfig( SmartPtr<CVmConfiguration> pOutVmConfig )
{
	if ( ! pOutVmConfig )
		return;

	pOutVmConfig->getVmIdentification()->setModifierName( "" );
	pOutVmConfig->getVmIdentification()->setLastModifDate(
		QDateTime::fromString( XML_DEFAULT_DATE_TIME, XML_DATETIME_FORMAT ) );

	bool bParentalControlEnabled = pOutVmConfig->getVmSecurity()->isParentalControlEnabled();
	QString qsStamp = pOutVmConfig->getVmSecurity()->getFieldPatchedValue(PARENTAL_CONTROL_ENABLED_STR);
	pOutVmConfig->setVmSecurity( new CVmSecurity( ) );
	pOutVmConfig->getVmSecurity()->setParentalControlEnabled(bParentalControlEnabled);
	pOutVmConfig->getVmSecurity()->markPatchedField(PARENTAL_CONTROL_ENABLED_STR, qsStamp);

	// #PDFM-40676
	pOutVmConfig->getVmSettings()->getLockDown()->setHash( );

	CVmEvent* pEvt = new CVmEvent;
	pEvt->setDefaults();
	pEvt->setEventIssuerId();
	pEvt->setInitRequestId();
	pOutVmConfig->getVmSettings()->getVmRuntimeOptions()->getInternalVmInfo()
		->setParallelsEvent( pEvt );

	CVmRemoteDisplay *remDisplay = Vnc::Traits::purify(pOutVmConfig.getImpl());
	switch (remDisplay->getMode())
	{
	case PRD_DISABLED:
		remDisplay->setPortNumber(0);
		remDisplay->setWebSocketPortNumber(0);
		break;
	case PRD_AUTO:
		remDisplay->setHostName(QHostAddress(QHostAddress::Any).toString());
		remDisplay->setPortNumber(0);
		remDisplay->setWebSocketPortNumber(0);
	case PRD_MANUAL:
		remDisplay->setEncrypted(Vnc::Encryption
			(*CDspService::instance()->getQSettings().getPtr())
				.enabled());
	}
	CCpuHelper::update(*pOutVmConfig);
}

void CDspVmDirHelper::fillOuterConfigParams(
	SmartPtr<CDspClient> pUserSession, SmartPtr<CVmConfiguration> pOutVmConfig,
	bool bFillAutogenerated)
{
	PRL_ASSERT(pUserSession);
	PRL_ASSERT(pOutVmConfig);

	if( !pOutVmConfig || !pUserSession )
		return;

	// append additional parameters from VM directory
	// (VM home, last change date, last modification date, ... )
	appendAdvancedParamsToVmConfig( pUserSession, pOutVmConfig, true, bFillAutogenerated );

	UpdateHardDiskInformation(pOutVmConfig);
}

PRL_RESULT CDspVmDirHelper::UpdateHardDiskInformation(SmartPtr<CVmConfiguration> &pConfig)
{
	if (pConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_SUCCESS;

	QSharedPointer< ::Stat::Storage> s = m_registry.find(pConfig->getVmIdentification()->getVmUuid())
		.getStorage();
	if (s.isNull())
		return PRL_ERR_INVALID_HANDLE;

	foreach(CVmHardDisk *d, pConfig->getVmHardwareList()->m_lstHardDisks)
	{
		::Stat::timedValue_type v;
		v = s->read(::Stat::Name::Hdd::getAllocation(*d));
		if (0 != v.second)
			d->setSizeOnDisk(v.first >> 20);
/*
		v = s->read(::Stat::Name::Hdd::getCapacity(*d));
		if (0 != v.second)
			d->setSize(v.first >> 20);
*/
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVmDirHelper::registerExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const SmartPtr<CDspClient> &pUserSession,
		const QString &sTaskId )
{
	IOSender::Handle hSession = ( pUserSession.getImpl() ? pUserSession->getClientHandle() : IOSender::Handle() );
	return registerExclusiveVmOperation(vmUuid, vmDirUuid, cmd, hSession, sTaskId);
}

PRL_RESULT  CDspVmDirHelper::unregisterExclusiveVmOperation(
	const QString& vmUuid,
	const QString& vmDirUuid,
	PVE::IDispatcherCommands cmd,
	const SmartPtr<CDspClient> &pUserSession )
{
	IOSender::Handle hSession = ( pUserSession.getImpl() ? pUserSession->getClientHandle() : IOSender::Handle() );
	return unregisterExclusiveVmOperation(vmUuid, vmDirUuid, cmd, hSession);
}

PRL_RESULT  CDspVmDirHelper::replaceExclusiveVmOperation(
	const QString& vmUuid,
	const QString& vmDirUuid,
	PVE::IDispatcherCommands fromCmd,
	PVE::IDispatcherCommands toCmd,
	const SmartPtr<CDspClient> &pUserSession )
{
	IOSender::Handle hSession = ( pUserSession.getImpl() ? pUserSession->getClientHandle() : IOSender::Handle() );
	return replaceExclusiveVmOperation(vmUuid, vmDirUuid, fromCmd, toCmd, hSession);
}

PRL_RESULT	CDspVmDirHelper::registerExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const IOSender::Handle &hUserSession,
		const QString &sTaskId )
{
	QString u = sTaskId;
	do
	{
		if (!sTaskId.isEmpty())
		       break;

		QThread* x = QThread::currentThread();
		if (NULL == x)
		       break;

		if (!x->inherits("CDspTaskHelper"))
		       break;

		u = x->objectName();
	} while(false);
	return m_exclusiveVmOperations.registerOp(vmUuid, vmDirUuid, cmd, hUserSession, u);
}

PRL_RESULT  CDspVmDirHelper::unregisterExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const IOSender::Handle &hUserSession )
{
	return m_exclusiveVmOperations.unregisterOp( vmUuid, vmDirUuid, cmd, hUserSession );
}

PRL_RESULT  CDspVmDirHelper::replaceExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands fromCmd,
		PVE::IDispatcherCommands toCmd,
		const IOSender::Handle &hUserSession )
{
	return m_exclusiveVmOperations.replaceOp( vmUuid, vmDirUuid, fromCmd, toCmd, hUserSession );
}

IOSender::Handle CDspVmDirHelper::getVmLockerHandle( const QString& vmUuid, const QString& vmDirUuid ) const
{
	return m_exclusiveVmOperations.getVmLockerHandle( vmUuid, vmDirUuid );
}

void CDspVmDirHelper::cleanupSessionVmLocks( const IOSender::Handle &hSession )
{
	m_exclusiveVmOperations.cleanupSessionVmLocks( hSession );
}

QStringList CDspVmDirHelper::getListOfLastCrashedLogs(
	const QStringList& listOfCrashDirs )
{
	QStringList lststrFiles;

	foreach( const QString & strDirToSearch, listOfCrashDirs )
	{
		QDir	cDir(strDirToSearch);
		cDir.setNameFilters( CProblemReportUtils::GetCrashDumpsTemplates() );
		cDir.setFilter(QDir::Files| QDir::NoDotAndDotDot | QDir::Hidden);
		cDir.setSorting(QDir::Time);

		QFileInfoList cFileList = cDir.entryInfoList();

		QDateTime currTime = QDateTime::currentDateTime();
		for(int i = 0; i < cFileList.size();i++ )
		{
			if( cFileList[i].fileName().endsWith( ".desc" ) )
				continue;

			if(cFileList[i].created().daysTo( currTime ) < 3 )
				lststrFiles << cFileList[i].filePath();
			// No file count limit: on Leopard MacOS X every crash writes to new file
		}//for

	}//foreach

	return lststrFiles;
}

void CDspVmDirHelper::getSuspendedVmScreen(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( !cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}

	QString vm_uuid = cmd->GetVmUuid();
	QString vm_dir_uuid = pUserSession->getVmDirectoryUuid();
	QString qsVmPath;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &pUserSession->getAuthHelper() );

	PRL_RESULT rc = PRL_ERR_FAILURE;
	CVmEvent evt;
	rc = CDspService::instance()->getAccessManager().
		checkAccess(pUserSession, PVE::DspCmdVmGetSuspendedScreen, vm_uuid, NULL, &evt);
	if (PRL_FAILED(rc))
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Access check failed for user {%s} when accessing VM {%s}. Reason: %#x (%s)",
			__FUNCTION__, QSTR2UTF8(pUserSession->getClientHandle()),
			QSTR2UTF8(vm_uuid), rc, PRL_RESULT_TO_STRING(rc)
			);
		pUserSession->sendResponseError(evt, p);
		return;
	}

	{
		CDspLockedPointer<CVmDirectoryItem> pDirItem = getVmDirectoryItemByUuid(vm_dir_uuid, vm_uuid);
		if (!pDirItem.isValid())
		{
			WRITE_TRACE(DBG_FATAL, "[%s] Directory for VM with uuid = %s doesn't exist!",
				__FUNCTION__, QSTR2UTF8(vm_uuid));
			pUserSession->sendSimpleResponse(p, PRL_ERR_VM_DIRECTORY_NOT_EXIST);
			return;
		}
		qsVmPath = QFileInfo(pDirItem->getVmHome()).absolutePath();
	}

	VIRTUAL_MACHINE_STATE state = CDspVm::getVmState(vm_uuid, vm_dir_uuid);
	if ( state != VMS_SUSPENDED
		&& state != VMS_SUSPENDING
		&& state != VMS_RESUMING
		&& state != VMS_SUSPENDING_SYNC
	)
	{
		WRITE_TRACE(DBG_FATAL, "[%s] VM with uuid = %s is not in suspended state ( state = %s (%d) )!"
			, __FUNCTION__
			, QSTR2UTF8(vm_uuid)
			, PRL_VM_STATE_TO_STRING( state )
			, state
			);
		pUserSession->sendSimpleResponse(p, PRL_ERR_VM_IS_NOT_SUSPENDED);
		return;
	}

	QString qsScreenFile = qsVmPath + "/" + PRL_VM_SUSPENDED_SCREEN_FILE_NAME;

	// #125852 - search suspended screenshot in old format
	if( ! QFileInfo( qsScreenFile ).exists() )
		qsScreenFile = qsVmPath + "/" + vm_uuid + ".png";


	QFile file(qsScreenFile);
	if (!file.open(QIODevice::ReadOnly))
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Suspended VM screen file with uuid = %s doesn't exist!",
			__FUNCTION__, QSTR2UTF8(vm_uuid));
		pUserSession->sendSimpleResponse(p, PRL_ERR_FILE_NOT_EXIST);
		return;
	}

	SmartPtr<CVmConfiguration> pVmConfig = getVmConfigByUuid( pUserSession, vm_uuid, rc);
	if( !pVmConfig )
	{
		WRITE_TRACE(DBG_FATAL, "Can't load vm config for vmuuid = %s", QSTR2UTF8(vm_uuid));
		pUserSession->sendSimpleResponse(p, rc);
		return;
	}

	QByteArray baBuffer = file.readAll();

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->AddStandardParam( QString(baBuffer.toBase64()) );

	pUserSession->sendResponse( pCmd, p );
}

void CDspVmDirHelper::startConvertDisks( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p )
{
	// Retrieve user parameters from request data

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
	CDspService::instance()->getTaskManager()
		.schedule(new Task_ConvertDisks(
				pUserSession,
				p,
				cmd->GetVmUuid(),
				cmd->GetFirstStrParam().toUInt(),
				cmd->GetCommandFlags()));
}

void CDspVmDirHelper::mountVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p )
{
	// Retrieve user parameters from request data

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}
#ifdef _LIN_
	CDspService::instance()->getTaskManager()
		.schedule(new Task_MountVm(pUserSession, p, PVE::DspCmdVmMount,
								   m_vmMountRegistry));
#else
	pUserSession->sendSimpleResponse( p, PRL_ERR_UNIMPLEMENTED);
#endif
}

void CDspVmDirHelper::umountVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p )
{
	// Retrieve user parameters from request data

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( p, PRL_ERR_FAILURE );
		return;
	}

#ifdef _LIN_
	CDspService::instance()->getTaskManager()
		.schedule(new Task_MountVm(pUserSession, p, PVE::DspCmdVmUmount,
								   m_vmMountRegistry));
#else
	pUserSession->sendSimpleResponse( p, PRL_ERR_UNIMPLEMENTED);
#endif
}

SmartPtr<CVmSecurity> CDspVmDirHelper::FillVmSecurity(
	SmartPtr<CDspClient> pUserSession,
	CVmDirectoryItem* pVmDirItem )
{
	SmartPtr<CVmSecurity> pVmSecurity( new CVmSecurity() );

	PRL_ASSERT( pUserSession );
	PRL_ASSERT( pVmDirItem );
	if( !pUserSession || !pVmDirItem )
		return pVmSecurity;

	bool flgVmHasNotOwner = false;

	//////////////////////////////////////////////////////////////////////////
	// pVmSecurity->getAccessControlList()->setAccessControl()
	//////////////////////////////////////////////////////////////////////////
	QList<PRL_ALLOWED_VM_COMMAND>
		sdk_access_list = CDspService::instance()->getAccessManager().getAllowedVmCommands( pUserSession, pVmDirItem );
	pVmSecurity->getAccessControlList()->setAccessControl( sdk_access_list );

	//////////////////////////////////////////////////////////////////////////
	// pVmSecurity->getLockedOperationsList()->setLockedOperations()
	//////////////////////////////////////////////////////////////////////////
	QList<PRL_ALLOWED_VM_COMMAND>
		list_commands_with_confirmation = pVmDirItem->getLockedOperationsList()->getLockedOperations();
	pVmSecurity->getLockedOperationsList()->setLockedOperations( list_commands_with_confirmation );

	pVmSecurity->setLockedSign( pVmDirItem->isLockedSign() );
	//////////////////////////////////////////////////////////////////////////
	// pVmSecurity->setOwner( ... )
	//////////////////////////////////////////////////////////////////////////
	QString ownerName = CDspService::instance()->getAccessManager().getOwnerOfVm( pVmDirItem );
	if ( ownerName.isEmpty() )
	{
		ownerName = pUserSession->getAuthHelper().getUserName();
		flgVmHasNotOwner = true;
	}
	pVmSecurity->setOwner(  ownerName  );

	//////////////////////////////////////////////////////////////////////////
	// pVmSecurity->setOwnerPresent( ... )
	//////////////////////////////////////////////////////////////////////////
	if( flgVmHasNotOwner )
		pVmSecurity->setOwnerPresent( false );
	else
	{
		pVmSecurity->setOwnerPresent(
			CDspService::instance()->getAccessManager().isOwnerOfVm( pUserSession, pVmDirItem )
			);
	}

	//////////////////////////////////////////////////////////////////////////
	// pVmSecurity->setAccessForOthers( ... )
	//////////////////////////////////////////////////////////////////////////
	PRL_VM_ACCESS_FOR_OTHERS sdk_accessForOthers = PAO_VM_NOT_SHARED;

	if( flgVmHasNotOwner )
		sdk_accessForOthers = PAO_VM_SHARED_ON_FULL_ACCESS;
	else
	{
		PRL_SEC_AM ownerAccess = 0;
		PRL_SEC_AM othersAccess = 0;
		bool flgOthersAccessIsMixed = false;

		CDspService::instance()->getAccessManager()
			.getFullAccessRightsToVm( pVmDirItem, ownerAccess, othersAccess, flgOthersAccessIsMixed, pUserSession->getAuthHelper() );

		PRL_SEC_AM modeR = CDspAccessManager::VmAccessRights::makeModeR();
		//PRL_SEC_AM modeRW = CDspAccessManager::VmAccessRights::makeModeRW();
		//PRL_SEC_AM modeNOACCESS = CDspAccessManager::VmAccessRights::makeModeNO_ACCESS();
		PRL_SEC_AM modeRX = CDspAccessManager::VmAccessRights::makeModeRX();
		PRL_SEC_AM modeRWX = CDspAccessManager::VmAccessRights::makeModeRWX();

		if( ( othersAccess & modeRWX ) == modeRWX )
			sdk_accessForOthers = PAO_VM_SHARED_ON_FULL_ACCESS;
		else if( ( othersAccess & modeRX ) == modeRX )
			sdk_accessForOthers = PAO_VM_SHARED_ON_VIEW_AND_RUN;
		else if( ( othersAccess & modeR ) == modeR )
			sdk_accessForOthers = PAO_VM_SHARED_ON_VIEW;
		else
			sdk_accessForOthers = PAO_VM_NOT_SHARED;
	}
	pVmSecurity->setAccessForOthers( sdk_accessForOthers ); //FIXME:  need change test default values !

	//////////////////////////////////////////////////////////////////////////
	// pVmSecurity->setParentalControlEnabled( ... )
	//////////////////////////////////////////////////////////////////////////
	PRL_RESULT rc = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigForDirectoryItem( pUserSession, pVmDirItem, rc );
	if ( pVmConfig.isValid() && PRL_SUCCEEDED(rc) )
	{
		pVmSecurity->setParentalControlEnabled( pVmConfig->getVmSecurity()->isParentalControlEnabled() );
		pVmSecurity->markPatchedField(PARENTAL_CONTROL_ENABLED_STR,
			pVmConfig->getVmSecurity()->getFieldPatchedValue(PARENTAL_CONTROL_ENABLED_STR));
	}

	LOG_MESSAGE( DBG_DEBUG, "access rights to vm( vm_uuid='%s' ) for user (user id = %s) is \n%s"
		, QSTR2UTF8( pVmDirItem->getVmUuid() )
		, QSTR2UTF8( pUserSession->getUserSettingsUuid() )
		, QSTR2UTF8( pVmSecurity->toString() )
		);
	return pVmSecurity;
}

PRL_RESULT CDspVmDirHelper::updateVmSecurityInfo ( SmartPtr<CDspClient> pUserSession
												  , const CVmDirectoryItem* pVmDirItem
												  , SmartPtr<CVmSecurity> pVmSecurity )
{
	PRL_RESULT ret = PRL_ERR_FAILURE;

	PRL_ASSERT( pUserSession );
	PRL_ASSERT( pVmDirItem );
	PRL_ASSERT( pVmSecurity );
	if( !pUserSession || !pVmDirItem || ! pVmSecurity )
		return ret;

	try
	{
		typedef CDspAccessManager::VmAccessRights PERM_T;
		PRL_SEC_AM permOthers = 0;

		PRL_VM_ACCESS_FOR_OTHERS prl_access_mode = pVmSecurity->getAccessForOthers();
		switch( prl_access_mode )
		{
		case PAO_VM_NOT_SHARED:
			permOthers = PERM_T::arCanNone;
			break;
		case PAO_VM_SHARED_ON_VIEW:
			permOthers = PERM_T::arCanRead;
			break;
		case PAO_VM_SHARED_ON_VIEW_AND_RUN:
			permOthers = PERM_T::arCanRead | PERM_T::arCanExecute ;
			break;
		case PAO_VM_SHARED_ON_FULL_ACCESS:
			permOthers = PERM_T::arCanRead | PERM_T::arCanWrite | PERM_T::arCanExecute ;
			break;
		default:
			PRL_ASSERT( "INTERNAL ERROR: UNSUPPORTED VALUE OF PRL_VM_ACCESS_FOR_OTHERS enum" == NULL );
			throw PRL_ERR_UNIMPLEMENTED;
		}

		ret = CDspService::instance()->getAccessManager()
			.setFullAccessRightsToVm( pUserSession, pVmDirItem, NULL, &permOthers );
		if ( PRL_FAILED( ret ) )
			throw ret;

		LOG_MESSAGE( DBG_INFO, "vm permission successfully changed to "
			"mode=%#o, for vm: vm_uuid = %s, vm_config_path='%s'"
			, permOthers
			, QSTR2UTF8( pVmDirItem->getVmUuid() )
			, QSTR2UTF8( pVmDirItem->getVmHome() ) );

		ret = PRL_ERR_SUCCESS;
	}
	catch( PRL_RESULT& err )
	{
		ret = err;
		WRITE_TRACE(DBG_FATAL, "%s was failed: vm_uuid '%s' vmSecurity = %s"
			, __FUNCTION__
			, QSTR2UTF8( pVmDirItem->getVmUuid() )
			, QSTR2UTF8( pVmSecurity->toString() ) );
	}

	return ret;
}

namespace
{
	void SafeDeleteKeyInQSetting( CDspLockedPointer<QSettings>& pSettings, const QString& key )
	{
		if( pSettings->contains( key ) )
			pSettings->remove( key );
	}
}

void CDspVmDirHelper::sendEventVmSecurityChangedToUser( SmartPtr<CDspClient> pUserSession,
										const QString& vmUuid,
										const SmartPtr<IOPackage> &pRequest )
{
	if( !pUserSession )
		return;

	CDspLockedPointer< CVmDirectoryItem >
		pVmDirItem =  CDspService::instance()->getVmDirManager()
		.getVmDirItemByUuid( pUserSession->getVmDirectoryUuid(), vmUuid );

	if (!pVmDirItem)
		return;

	SmartPtr<CVmSecurity> pVmSecurity = FillVmSecurity(pUserSession, pVmDirItem.getPtr());

	CVmEvent event( PET_DSP_EVT_VM_SECURITY_CHANGED, vmUuid, PIE_DISPATCHER );
	event.addEventParameter(new CVmEventParameter(PVE::String, pVmSecurity->toString(),
		EVT_PARAM_VM_SECURITY_INFO));

	SmartPtr<IOPackage> pEvtPkg = DispatcherPackage::createInstance( PVE::DspVmEvent, event, pRequest );

	pUserSession->sendPackage(pEvtPkg);
}

void CDspVmDirHelper::sendEventVmSecurityChanged(
	const QString& vmDirUuid,
	const QString& vmUuid,
	const SmartPtr<IOPackage> &pRequest )
{
	// Generate "VM security changed" event for all active sessions
	QList< SmartPtr<CDspClient> >
		clientList = CDspService::instance()
		->getClientManager().getSessionsListSnapshot( vmDirUuid ).values();

	foreach(SmartPtr<CDspClient> pClient, clientList)
		sendEventVmSecurityChangedToUser( pClient, vmUuid, pRequest );
}

PRL_RESULT CDspVmDirHelper::saveFastRebootData( const QString &/*vmUuid*/, const QString &/*user*/ )
{
	return PRL_ERR_UNIMPLEMENTED;
}

PRL_RESULT CDspVmDirHelper::loadFastRebootData( const QString &/*vmUuid*/, QString &/*user*/ )
{
	return PRL_ERR_UNIMPLEMENTED;
}

CMultiEditMergeVmConfig* CDspVmDirHelper::getMultiEditDispatcher()
{
	return m_pVmConfigEdit.data();
}

void CDspVmDirHelper::recoverMixedVmPermission( SmartPtr<CDspClient> pOwnerSession )
{
	// 1. get Vm List
	// 2. get Vm with isOwner
	// 3. check mixed permission
	// 4. fix mixed

	QList<QString> uuidList = getVmList( pOwnerSession );


	foreach( QString vmUuid, uuidList )
	{
		CDspVmDirManager& vmDirManager = CDspService::instance()->getVmDirManager();
		CDspAccessManager& accessManager = CDspService::instance()->getAccessManager();

		CDspLockedPointer< CVmDirectoryItem >
			pVmDirItem = vmDirManager.getVmDirItemByUuid( pOwnerSession->getVmDirectoryUuid(), vmUuid );

		if( !accessManager.isOwnerOfVm( pOwnerSession, pVmDirItem.getPtr() ) )
			continue;

		PRL_RESULT res = PRL_ERR_FIXME;
		PRL_SEC_AM ownerAccess = 0;
		PRL_SEC_AM othersAccess = 0;
		bool flgOthersAccessIsMixed = false;

		res = accessManager.getFullAccessRightsToVm(
			pVmDirItem.getPtr(),
			ownerAccess,
			othersAccess,
			flgOthersAccessIsMixed,
			pOwnerSession->getAuthHelper() );

		if( PRL_FAILED(res) )
		{
			WRITE_TRACE(DBG_FATAL, "getFullAccessRightsToVm() failed by error %#x(%s). vmHome = '%s'"
				, res
				, PRL_RESULT_TO_STRING( res)
				, QSTR2UTF8( pVmDirItem->getVmHome() ) );
			continue;
		}

		if( !flgOthersAccessIsMixed )
			continue;


		// bug #9058: reset mixed 'others' rights to vm
		// reset mixed 'others' rights to vm to READ-ONLY for others.
		PRL_SEC_AM newOthersAccess = CDspAccessManager::VmAccessRights::makeModeR();

		res = accessManager.setFullAccessRightsToVm( pOwnerSession, pVmDirItem.getPtr(), NULL, &newOthersAccess);

		if( PRL_SUCCEEDED(res) )
		{
			WRITE_TRACE(DBG_FATAL, "%s, NOTE: VM permission for 'others' was changed to %#o. vm_uuid=%s."
				, __FUNCTION__
				, newOthersAccess
				, QSTR2UTF8( pVmDirItem->getVmUuid() )
				);
			sendEventVmSecurityChanged( pOwnerSession->getVmDirectoryUuid(), pVmDirItem->getVmUuid() );
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "%s, NOTE: Can't change VM permissions. vm_uuid=%s. error = %#x( '%s' ) "
				, __FUNCTION__
				, QSTR2UTF8( pVmDirItem->getVmUuid() )
				, res
				, PRL_RESULT_TO_STRING( res )
				);
		}
	}//foreach
}

void CDspVmDirHelper::resizeDiskImage(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CDspService::instance()->getTaskManager()
		.schedule(new Task_DiskImageResizer(pUser, p));
}

PRL_VM_COLOR CDspVmDirHelper::getUniqueVmColor( const QString& vmDirUuid )
{
	// Cache all Vm colors
	QHash<PRL_VM_COLOR, int> usedColors;
	QMultiHash< QString, SmartPtr< CVmConfiguration > > vms = getAllVmList( vmDirUuid );
	foreach ( SmartPtr< CVmConfiguration > pVmConfig, vms.values() )
	{
		if ( ! pVmConfig.isValid() )
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM configuration" );
			continue;
		}

		CVmCommonOptions* commonOpts = pVmConfig->getVmSettings()->getVmCommonOptions();
		PRL_VM_COLOR vmColor = (PRL_VM_COLOR)commonOpts->getVmColor();

		if ( ! usedColors.contains(vmColor) )
			usedColors[vmColor] = 1;
		else
			usedColors[vmColor] += 1;
	}

	// Find rare or unique color
	PRL_VM_COLOR colors[] = { PVC_COLOR_RED, PVC_COLOR_ORANGE, PVC_COLOR_YELLOW,
		PVC_COLOR_GREEN, PVC_COLOR_BLUE, PVC_COLOR_PURPLE,
		PVC_COLOR_GREY };
	PRL_VM_COLOR rareColor = colors[0];
	int rareColorNum = INT_MAX;

	for ( unsigned int i = 0; i < sizeof(colors)/sizeof(colors[0]); ++i ) {
		if ( ! usedColors.contains(colors[i]) )
			return colors[i];
		if ( rareColorNum > usedColors[colors[i]] ) {
			rareColorNum = usedColors[colors[i]];
			rareColor = colors[i];
		}
	}

	return rareColor;
}

//////////////////////////////////////////////////////////////////////////
// prepare parameters for vm move
void CDspVmDirHelper::moveVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}
	CDspService::instance()->getTaskManager()
		.schedule(new Task_MoveVm(pUserSession , pkg, *m_ephemeral));
}

namespace List
{
namespace Directory
{
namespace Item
{
///////////////////////////////////////////////////////////////////////////////
// struct Component

Component::~Component()
{
}

///////////////////////////////////////////////////////////////////////////////
// struct Chain

Chain::result_type Chain::handle(const CVmDirectoryItem& item_)
{
	if (m_next.isNull())
		return PRL_ERR_UNIMPLEMENTED;

	return m_next->handle(item_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Identity

Identity::result_type Identity::handle(const CVmDirectoryItem& item_)
{
	value_type output = CDspVmDirHelper::CreateVmConfigFromDirItem(m_uuid, &item_);
	if (!output.isValid())
	{
		WRITE_TRACE(DBG_FATAL, ">>> Cannot create a Vm config for the file %s",
			qPrintable(item_.getVmHome()));
		return PRL_ERR_FAILURE;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Sterling

Sterling::result_type Sterling::handle(const CVmDirectoryItem& item_)
{
	SmartPtr<CVmConfiguration> output;
	PRL_RESULT e = PRL_ERR_SUCCESS;

	if (PVT_CT == item_.getVmType())
	{
		output = m_service->getVzHelper()->getCtConfig(m_session,
				item_.getVmUuid(), item_.getVmHome(), true);
	}
	else
	{
		e = m_service->getVmConfigManager().loadConfig(output,
				item_.getVmHome(), m_session, true, false);
	}

	if (PRL_FAILED(e) || !output.isValid())
	{
		WRITE_TRACE(DBG_FATAL, ">>> Error [%#x / %s ] occurred while trying to load VM config from file [%s]",
			e, PRL_RESULT_TO_STRING(e), qPrintable(item_.getVmHome()));
		return PRL_ERR_PARSE_VM_CONFIG;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::result_type Vm::handle(const CVmDirectoryItem& item_)
{
	if (PVT_CT == item_.getVmType())
		return PRL_ERR_INVALID_PARAM;

	return Chain::handle(item_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Template

Template::result_type Template::handle(const CVmDirectoryItem& item_)
{
	result_type output = Chain::handle(item_);
	if (output.isSucceed() && output.value().isValid())
	{
		output.value()->getVmSettings()
			->getVmCommonOptions()->setTemplate(true);
	}

	return output;
}

namespace Inaccessible
{
///////////////////////////////////////////////////////////////////////////////
// struct Chain

PRL_RESULT Chain::determine(const CVmDirectoryItem& item_)
{
	CVmDirectoryItem i(&item_);
	return m_service->getAccessManager()
		.checkAccess(m_session, PVE::DspCmdDirGetVmList, &i);
}

///////////////////////////////////////////////////////////////////////////////
// struct Identity

Identity::result_type Identity::handle(const CVmDirectoryItem& item_)
{
	PRL_RESULT e = determine(item_);
	if (PRL_SUCCEEDED(e))
		return Chain::handle(item_);

	return m_chain.handle(item_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Default

QString Default::craftName(const CVmDirectoryItem* item_, const QString& uid_) const
{
	QString d = getSession()->getVmDirectoryUuid();
	if (NULL == item_)
	{
		WRITE_TRACE(DBG_FATAL, "Can't found d by dirUuid=%s, vmUuid = %s",
			qPrintable(d), qPrintable(uid_));

		return PRL_UNKNOWN_NAME;
	}
	QString output = item_->getVmName();
	if (output.isEmpty())
	{
		// Make VM name unique
		output = Task_RegisterVm::getUniqueVmName
			(QFileInfo(item_->getVmHome()).dir().dirName(), d);
	}

	return output;
}

Default::result_type Default::handle(const CVmDirectoryItem& item_)
{
	PRL_RESULT e = determine(item_);
	if (PRL_SUCCEEDED(e))
		return Chain::handle(item_);

	value_type output = craft(e, item_.getVmUuid());
	if (!output.isValid())
	{
		WRITE_TRACE(DBG_FATAL, ">>> Error [%#x / %s ] occurred while "
			"trying to load VM config from file [%s]",
			e, PRL_RESULT_TO_STRING(e), qPrintable(item_.getVmHome()));
		return PRL_ERR_FAILURE;
	}

	return output;
}

Default::value_type Default::craft(PRL_RESULT code_, const QString& uid_)
{
	value_type output;
	CVmIdent o(getService().getVmDirHelper().getVmIdentByVmUuid(uid_, getSession()));
	CDspLockedPointer<CVmDirectoryItem> d(getService().getVmDirManager().getVmDirItemByUuid(o));
	switch (code_)
	{
	case PRL_ERR_VM_CONFIG_INVALID_VM_UUID:
	case PRL_ERR_VM_CONFIG_INVALID_SERVER_UUID:
	case PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED:
	{
		PRL_RESULT e;
		output = getService().getVmDirHelper().getVmConfigByUuid(getSession(), uid_, e);
		if (!output.isValid())
		{
			WRITE_TRACE(DBG_FATAL, "getVmConfigByUuid( %s ) return NULL. error %#x(%s)",
				qPrintable(uid_), e, PRL_RESULT_TO_STRING(e));
		}
		break;
	}
	case PRL_ERR_VM_CONFIG_DOESNT_EXIST:
	case PRL_ERR_PARSE_VM_CONFIG:
		if (d.isValid() && getService().getVmConfigManager().canConfigRestore(d->getVmHome(), getSession()))
			code_ = PRL_ERR_VM_CONFIG_CAN_BE_RESTORED;

	case PRL_ERR_ACCESS_TO_VM_DENIED:
		output = value_type(new CVmConfiguration());
	default:
		break;
	}
	if (output.isValid())
	{
		output->getVmIdentification()->setVmName(craftName(d.getPtr(), uid_));
		output->getVmIdentification()->setVmUuid(uid_);
		output->setValidRc(code_);
	}

	return output;
}

} // namespace Inaccessible

///////////////////////////////////////////////////////////////////////////////
// struct State

State::result_type State::handle(const CVmDirectoryItem& item_)
{
	result_type output = Chain::handle(item_);
	if (output.isFailed())
		return output;

	const value_type& r = output.value();
	QString u = item_.getVmUuid();
	QString d = CDspVmDirHelper::getVmDirUuidByVmUuid(u, m_session);
	VIRTUAL_MACHINE_STATE s = item_.isTemplate() ? VMS_STOPPED :
		CDspVm::getVmState(u, d);
	CVmEvent* e = new CVmEvent();
	e->addEventParameter(new CVmEventParameter(PVE::Integer, QString::number(s),
		EVT_PARAM_VMINFO_VM_STATE));
	r->getVmSettings()->getVmRuntimeOptions()->getInternalVmInfo()->setParallelsEvent(e);

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Extra

Extra::result_type Extra::handle(const CVmDirectoryItem& item_)
{
	result_type output = Chain::handle(item_);
	if (output.isFailed())
		return output;

	m_service->getVmDirHelper()
		.fillOuterConfigParams(m_session, output.value(), m_autogenerated);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Factory

Chain* Factory::operator()(const session_type& session_, quint32 flags_) const
{
	Chain* output = NULL;
	if (flags_ & PGVLF_GET_STATE_INFO)
		output = new State(session_);
	else if (0 == (flags_ & PGVLF_GET_ONLY_IDENTITY_INFO))
		output = new Extra(session_, flags_ & PGVLF_FILL_AUTOGENERATED, *m_service);

	Chain* y;
	Item::Identity* i = NULL;
	if (flags_ & PGVLF_GET_ONLY_IDENTITY_INFO)
	{
		i = new Item::Identity(m_service->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid());
		y = new Inaccessible::Identity(*m_service, session_, *i);
	}
	else
		y = new Inaccessible::Default(*m_service, session_);

	if (NULL == output)
		output = y;
	else
		output->setNext(y);

	if (flags_ & PGVLF_GET_ONLY_IDENTITY_INFO)
		y->setNext(i);
	else
		y->setNext(new Sterling(session_, *m_service));

	return output;
}

} // namespace Item

///////////////////////////////////////////////////////////////////////////////
// struct Chain

Chain::~Chain()
{
}

QStringList Chain::getResult() const
{
	if (m_next.isNull())
		return m_result;

	return QStringList() << m_result << m_next->getResult();
}

PRL_RESULT Chain::handle(const CVmDirectory& directory_)
{
	if (m_next.isNull())
		return PRL_ERR_SUCCESS;

	return m_next->handle(directory_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Loop

PRL_RESULT Loop::handle(const CVmDirectory& directory_)
{
	if (m_predicate.empty())
		return PRL_ERR_UNINITIALIZED;

	if (!m_predicate(directory_))
		return Chain::handle(directory_);

	if (m_loader.isNull())
		return PRL_ERR_UNINITIALIZED;

	foreach (CVmDirectoryItem* i, directory_.m_lstVmDirectoryItems)
	{
		Item::Component::result_type x = m_loader->handle(*i);
		if (x.isSucceed())
			deposit(*x.value());
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Ephemeral

Loop::predicate_type Ephemeral::craftContains(const list_type& list_)
{
	bool (list_type::* x)(const QString& ) const = &list_type::contains;
	return boost::bind(x, list_, boost::bind(&CVmDirectory::getUuid, _1));
}

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Generic

void Generic::setLoader(Item::Component* value_)
{
	Item::Vm* v = new Item::Vm();
	v->setNext(value_);
	Loop::setLoader(v);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

Ordinary::Ordinary(const Ephemeral::list_type& ephemeral_, Item::Component* loader_)
{
	setLoader(loader_);
	setPredicate(!boost::bind(Ephemeral::craftContains(ephemeral_), _1));
}

} // namespace Vm

namespace Template
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Generic

void Generic::setLoader(Item::Component* value_)
{
	Item::Template* t = new Item::Template();
	t->setNext(value_);
	Loop::setLoader(t);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

Ordinary::Ordinary(Item::Component* loader_)
{
	setLoader(loader_);
	setPredicate(boost::bind(&CVmDirectory::getUuid, _1) ==
		CDspVmDirManager::getTemplatesDirectoryUuid());
}

///////////////////////////////////////////////////////////////////////////////
// struct Ephemeral

Ephemeral::Ephemeral(const Directory::Ephemeral::list_type& ephemeral_, Item::Component* loader_)
{
	setLoader(loader_);
	setPredicate(Directory::Ephemeral::craftContains(ephemeral_));
}

} // namespace Vm
} // namespace Template

///////////////////////////////////////////////////////////////////////////////
// struct Ct

Ct::Ct(const session_type& session_, quint32 flags_, CDspService& service_):
	m_uuid(CDspVmDirManager::getVzDirectoryUuid()), m_flags(flags_),
	m_service(&service_), m_session(session_)
{
}

PRL_RESULT Ct::handle(const CVmDirectory& directory_)
{
	if (directory_.getUuid() != m_uuid)
		return Chain::handle(directory_);

	QList<value_type> a;
	m_service->getVzHelper()->getCtConfigList(m_session, m_flags, a);
	foreach(const value_type& i, a)
	{
		deposit(*i);
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Factory

Chain* Factory::operator()(const session_type& session_, quint32 flags_) const
{
	Chain *x = NULL, *output = NULL;
	quint32 m = flags_ & (PVTF_VM | PVTF_CT);
	if (m & PVTF_CT)
		output = x = new Ct(session_, flags_, *m_service);

	Item::Factory f(*m_service);
	if (0 == m || (m & PVTF_VM))
	{
		Chain* y = new Template::Vm::Ordinary(f(session_, flags_));
		if (NULL == x)
			output = y;
		else
			x->setNext(y);

		x = y;
		y = new Vm::Ordinary(m_ephemeral, f(session_, flags_));
		x->setNext(y);
		x = y;
	}

	if (x)
		x->setNext(new Template::Vm::Ephemeral(m_ephemeral, f(session_, flags_)));

	return output;
}

} // namespace Directory
} // namespace List

