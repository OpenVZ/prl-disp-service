///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVm_p.h
///
/// Class that wrapping Vm possible actions (start, stop and etc.)
///
/// @author shrike
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

#ifndef __CDSPVM_P_H__
#define __CDSPVM_P_H__

#include <QThread>
#include <QString>
#include <QReadWriteLock>
#include <QProcess>
#include "CDspCommon.h"
#include "CDspClient.h"
#include "CDspService.h"
#include "CDspStarter.h"
#include "CDspVNCStarter.h"
#include <prlcommon/Std/SmartPtr.h>
#include "CDspVmDirManager.h"
#include <prlcommon/Std/noncopyable.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "CDspDispConnection.h"
#include "CDspVmSuspendHelper.h"
#include <prlxmlmodel/DispConfig/CDispCpuPreferences.h>

#include "Libraries/PerfCount/PerfLib/PerfCounter.h"

namespace DspVm
{
inline CDspService& ds()
{
	return *CDspService::instance();
}

inline CDspClientManager& cm()
{
	return ds().getClientManager();
}

inline IOServerPool& io()
{
	return ds().getIOServer();
}

inline CDspVmDirHelper& vdh()
{
	return ds().getVmDirHelper();
}

inline CDspVmDirManager& vdm()
{
	return ds().getVmDirManager();
}

///////////////////////////////////////////////////////////////////////////////
// struct RequestInfo

struct RequestInfo
{
	// Request package
	SmartPtr<IOPackage> pkgRequest;

	// Deferred response to send after CDspVm object will destroyed
	SmartPtr<IOPackage> pkgResponse;

	// Previous vm state to rollback, if operation was filed.
	VIRTUAL_MACHINE_STATE nPrevVmState;

	// 'true' if response should be sent in destructor
	bool isDeferred;

	// 'true' if action was initiated by dispatcher (not client or vm)
	bool isActionByDispatcher;

	RequestInfo( SmartPtr<IOPackage> pkgRequest, bool bActionByDispatcher );
};

///////////////////////////////////////////////////////////////////////////////
// struct Details

struct Details: noncopyable
{
	/** VM configuration UUID */
	/** Parent VM directory UUID */
	CVmIdent m_VmIdent ;

	/** */
	PVE::IDispatcherCommands m_nInitDispatcherCommand;

	/** VM Name on starting state */
	QString m_sVmName;

	/** VNC starter */
	CDspVNCStarter m_vncStarter;

	/** VM process identifier */
	Q_PID m_VmProcessId;

	bool m_bVmIsChild;

	/** VM state */
	VIRTUAL_MACHINE_STATE m_nVmState;
	VIRTUAL_MACHINE_STATE m_nPrevVmState;

	/** VM power state **/
	CDspVm::VmPowerState m_nVmPowerState;

	/** VM connection object handle */
	IOSender::Handle m_VmConnectionHandle;

	/** Storing pointer to start VM package that using in handshake procedure */
	SmartPtr<IOPackage> m_pStartVmPkg;

	/** Storing starting VM job result object for next using */
	IOSendJob::Handle m_hStartVmJob;

	/** Storing handle to start VM procedure initiator */
	SmartPtr<CDspClient>  m_pVmRunner;

	/** Class members access synchronization object */
	mutable QReadWriteLock m_rwLock;

	/** Question packet from VM */
	SmartPtr<IOPackage> m_pQuestionPacket;

	bool m_suspendByDispatcher;
	CDspVm::SuspendMode m_suspendMode;

	QMutex m_SnapshotRequestMutex;
	SmartPtr<IOPackage> m_pSnapshotRequest;
	VIRTUAL_MACHINE_STATE m_SnapshotVmState;
	SmartPtr<CDspClient> m_pSnapshotUser;
	QString m_sSnapshotTaskUuid;

	/** Dispatcher itself iniated action (not client or VM) */
	bool m_bFinishedByDispatcher;

	/**
	 * Storing pointer to migrate VM package that using in handshake procedure
	 * in case when VM process was created in service migrate VM mode
	 */
	SmartPtr<IOPackage> m_pMigrateVmPkg;

	/**
	 * Pointer to the VM migration connection channel object. Using for passing migration
	 * channel to VM process on target side
	 */
	SmartPtr<CDspDispConnection> m_pMigratingVmConnection;

	/* UUID of snapshot, created for migration */
	QString m_sSnapshotUuid;

	QMutex	m_mtxPerfStoragesContainer;
	ProcPerfStoragesContainer m_perfstorage_container;

	/** Undo disks mode */
	PRL_UNDO_DISKS_MODE	m_nUndoDisksMode;
	/** Safe mode */
	bool m_bSafeMode;
	/** Package for undo disks mode */
	SmartPtr<IOPackage> m_pUndoDisksPkg;
	/** User for undo disks mode */
	SmartPtr<CDspClient> m_pUndoDisksUser;
	/** Do not show question if VM was not started actually */
	bool m_bNoUndoDisksQuestion;

	typedef QString RequestPackageUuid;
	typedef QHash< RequestPackageUuid, RequestInfo > RequestResponseHash;
	RequestResponseHash m_hashRequestResponse;

	VIRTUAL_MACHINE_STATE m_stateBeforeHandshake;

	// Guest OS sessions list
	QMutex m_GuestSessionsMtx;
	QMap<IOSender::Handle, QSet<QString> > m_GuestSessions;

	//Storing handle on connection initializes request of VM operation
	IOSender::Handle m_hLockOwner;

	/** System ticks count on VM start */
	PRL_UINT64 m_nVmStartTicksCount;

	/** System ticks count on VM start */
	PRL_UINT64 m_nVmProcStartTicksCount;

	// Flag is set when number of virtual machines was changed by this object
	bool	m_bVmsQty;

	QString	m_strVmHome;

	bool m_bStoreRunningStateDisabled;

	// Restored VM product version
	QString m_qsRestoredVmProductVersion;

	QMutex m_applyConfigMutex;
	QWaitCondition m_applyConfigCondition;

	bool m_bVmCmdWasExclusiveRegistered;

	SmartPtr<CDspVmSuspendMounter> m_pSuspendMounter;

	Details(const CVmIdent& id_, const SmartPtr<CDspClient>& client_,
		PVE::IDispatcherCommands command_, VIRTUAL_MACHINE_STATE state_);

	CVmEvent authorize(SmartPtr<CDspClient> client_, PVE::IDispatcherCommands request_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Storage

struct Storage
{
	typedef SmartPtr<CDspVm> value_type;
	typedef QList<value_type> snapshot_type;

	template<typename T>
	value_type finr(const T& by_) const
	{
		value_type x = finw(by_);
		if (x.isValid())
			return x;

		return find(m_pending, by_);
	}
	template<typename T>
	value_type finw(const T& by_) const
	{
		return find(m_map, by_);
	}
	value_type enroll(SmartPtr<Details> data_);
	bool wipePending(const CVmIdent& key_)
	{
		return 0 != m_pending.erase(key_);
	}
	bool unregister(const CVmIdent& key_);
	snapshot_type unregisterAll();
	SmartPtr<Details> wipeShared(SmartPtr<Details>& data_);
	snapshot_type snapshot() const;
private:
	typedef std::pair<SmartPtr<Details>, bool> state_type;
	typedef std::map<CVmIdent, state_type> map_type;

	static value_type make(const state_type& state_);
	static value_type find(const map_type& map_, const CVmIdent& key_);
	static value_type find(const map_type& map_, const IOSender::Handle& connection_);

	map_type m_map;
	map_type m_pending;
};

namespace Start
{
///////////////////////////////////////////////////////////////////////////////
// struct Demand

struct Demand
{
	Demand(const SmartPtr<CDspClient>& user_,
		const SmartPtr<IOPackage>& request_):
		m_user(user_), m_package(request_)
	{
	}

	const SmartPtr<CDspClient>& getActor() const
	{
		return m_user;
	}
	const SmartPtr<IOPackage>& getRequest() const
	{
		return m_package;
	}
	void reject(const CVmEvent& error_);
	void reject(PRL_RESULT error_)
	{
		m_user->sendSimpleResponse(m_package, error_);
	}
	void rejectBadConfig(const CVmEvent& error_);
private:
	SmartPtr<CDspClient> m_user;
	SmartPtr<IOPackage> m_package;
};

///////////////////////////////////////////////////////////////////////////////
// struct TestSuite

struct TestSuite: noncopyable
{
	TestSuite(Demand& demand_, SmartPtr<CVmConfiguration> config_);

	bool checkConfig();
	bool checkHardware();
	bool checkFirewall();
private:
	Demand* m_demand;
	SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Setup

struct Setup
{
	Setup(Demand& demand_, Details& vm_);

	bool check();
	bool prepare();
	SmartPtr<CDspClient> getUser() const;
	SmartPtr<CVmConfiguration> getConfig() const;
private:
	Details* m_vm;
	Demand* m_demand;
	SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Process

struct Process
{
	PVE::VmBinaryMode getMode() const
	{
		return m_mode;
	}
	IOCommunication::SocketHandle getSocket() const
	{
		return m_socket;
	}
	QString getBinary() const;
	QString getWorkingDir() const;
	QStringList getArguments() const;
	QStringList getEnvironment(CDspClient& user_) const;
	void setMode(const CVmConfiguration& config_);

	static Process* yield(const CVmIdent& ident_);
private:
	Process(const CVmIdent& ident_, IOCommunication::SocketHandle socket_);

	CVmIdent m_ident;
	PVE::VmBinaryMode m_mode;
	IOCommunication::SocketHandle m_socket;
};

namespace Monitor
{
///////////////////////////////////////////////////////////////////////////////
// struct Routine

struct Routine
{
	explicit Routine(const CVmIdent& ident_);

	bool isPending();
	void fail();
	bool wait();
	SmartPtr<CDspVm> celebrate();
private:
	enum
	{
		MONITORING_TIMEOUT = 100,
		HANDSHAKE_TIMEOUT = 60000
	};
	void reply();

	int m_left;
	CVmIdent m_ident;
	SmartPtr<CDspVm> m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Standard

struct Standard
{
	explicit Standard(const CVmIdent& ident_): m_ident(ident_)
	{
	}

	SmartPtr<CDspVm> operator()() const;
private:
	CVmIdent m_ident;
};

///////////////////////////////////////////////////////////////////////////////
// struct Extended

struct Extended
{
	explicit Extended(const Standard& chain_): m_chain(chain_)
	{
	}

	void operator()() const;
private:
	Standard m_chain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Task

template<class T>
struct Task: QRunnable
{
	void run()
	{
		LOG_MESSAGE(DBG_DEBUG, "Monitor::Task::run() this=%p", this);
		COMMON_TRY
		{
			m_routine();
		}
		COMMON_CATCH;
	}
	static void schedule(T routine_)
	{
		Task* q = new Task(routine_);
		q->setAutoDelete(true);
		QThreadPool::globalInstance()->start(q);
	}
private:
	explicit Task(T routine_): m_routine(routine_)
	{
	}

	T m_routine;
};

} // namespace Monitor
} // namespace Start
} // namespace DspVm

#endif // __CDSPVM_P_H__
