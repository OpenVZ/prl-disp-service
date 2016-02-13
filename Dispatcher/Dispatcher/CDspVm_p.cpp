//////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVm_p.cpp
///
/// Class which wraps a Vm object private stuff.
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

#include "CDspVm_p.h"
#include "CVmValidateConfig.h"
#include <prlcommon/PrlCommonUtilsBase/Common.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Tasks/Task_PrepareForHibernate.h"
#include <prlcommon/PrlCommonUtilsBase/OsInfo.h>
//#include <Libraries/VirtualDisk/VirtualDisk.h>  // VirtualDisk commented out by request from CP team
#include "Tasks/Task_CommitUnfinishedDiskOp.h"
#include <Libraries/PrlCommonUtils/CFirewallHelper.h>
#include <prlcommon/Interfaces/ApiDevNums.h>
#ifdef _LIN_
#include "Libraries/Virtuozzo/CCpuHelper.h"
#endif

#define DISK_TOOLS_VERSION_KEY        "GuestToolsVersion"
#define DISK_TOOLS_NO_TOOLS_VAL       "0"

namespace
{
PVE::VmBinaryMode getWinVmBinaryMode(PVE::VmBinaryMode vmBinaryMode, const CVmIdent& vmIdent )
{
#ifndef _WIN_
	Q_UNUSED(vmIdent);
#else
#ifdef _64BIT_
	return vmBinaryMode;
#endif
	PRL_ASSERT( IsValidVmIdent(vmIdent) );

	if (vmBinaryMode == PVE::VBMODE_32BIT)
		return vmBinaryMode;

	OSINFO_WIN_INFORMATION_V2 winInfo;
	osInfo_getWinInformation(&winInfo);
	// Check only XP and 2003 Windows
	if ( winInfo.verMajor != 5 || winInfo.verMinor < 2 )
		return vmBinaryMode;

	if (osInfo_getArchitecture() != OSINFO_ARCHITECTURE_64_BIT)
		return vmBinaryMode;

	HKEY hk;
	LONG rerr = RegOpenKeyExA(
					HKEY_LOCAL_MACHINE,
					"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\KB928646",
					0,
					KEY_QUERY_VALUE | KEY_WOW64_64KEY,
					&hk);
	if (rerr == ERROR_SUCCESS)
	{
		RegCloseKey(hk);
		return vmBinaryMode;
	}

	vmBinaryMode = PVE::VBMODE_32BIT;

	CVmEvent evt(PET_DSP_EVT_VM_MESSAGE,
				 vmIdent.first,
				 PIE_DISPATCHER,
				 PRL_ERR_MSG_START_32BIT_VM_ON_64BIT_HOST );
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, evt);

	CDspService::instance()->getClientManager()
		.sendPackageToVmClients(p, vmIdent);

#endif

	return vmBinaryMode;
}

} // namespace

namespace DspVm
{
///////////////////////////////////////////////////////////////////////////////
// struct RequestInfo

RequestInfo::RequestInfo( SmartPtr<IOPackage> pkgRequest, bool bActionByDispatcher ):
	pkgRequest(pkgRequest),
	pkgResponse(0),
	nPrevVmState(VMS_UNKNOWN),
	isDeferred(false),
	isActionByDispatcher(bActionByDispatcher)
{
	WRITE_TRACE( DBG_DEBUG, "create RequestInfo: requestId=%s"
		, (!pkgRequest) ? "": QSTR2UTF8(Uuid::toString(pkgRequest->header.uuid)) );
}

///////////////////////////////////////////////////////////////////////////////
// struct Details

Details::Details(const CVmIdent& id_, const SmartPtr<CDspClient>& client_,
	PVE::IDispatcherCommands command_, VIRTUAL_MACHINE_STATE state_):
	m_VmIdent(id_),
	m_nInitDispatcherCommand(command_),
	m_sVmName("unknown_vm_name"),
	m_VmProcessId(0),
	m_bVmIsChild(true),
	m_nVmState(state_),
	m_nPrevVmState( VMS_UNKNOWN ),
	m_nVmPowerState(CDspVm::vpsNormal),
#if QT_VERSION >= 0x040400
	m_rwLock(QReadWriteLock::Recursive),
#endif
	m_suspendByDispatcher(false),
	m_suspendMode(CDspVm::SM_STOP_ON_FAILURE),
	m_SnapshotVmState(VMS_STOPPED),
	m_sSnapshotTaskUuid(Uuid::createUuid().toString()),
	m_bFinishedByDispatcher(false),
	m_mtxPerfStoragesContainer(QMutex::Recursive),
	m_nUndoDisksMode(PUD_DISABLE_UNDO_DISKS),
	m_bSafeMode(false),
	m_bNoUndoDisksQuestion(false),
	m_stateBeforeHandshake( VMS_UNKNOWN ),
	m_hLockOwner(client_ ? client_->getClientHandle() : IOSender::Handle()),
	m_nVmStartTicksCount(0),
	m_nVmProcStartTicksCount(0),
	m_bVmsQty(false),
	m_bStoreRunningStateDisabled(false),
	m_bVmCmdWasExclusiveRegistered(false)
{
}

CVmEvent Details::authorize(SmartPtr<CDspClient> client_, PVE::IDispatcherCommands request_) const
{
	bool f = false;
	CVmEvent output;
	PRL_RESULT e = ds().getAccessManager().checkAccess(client_, request_,
				m_VmIdent.first, &f, &output);
	if (PRL_FAILED(e))
	{
		CDspVmDirHelper::sendNotValidState(client_, e,
			m_VmIdent.first, f);
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Storage

SmartPtr<Details> Storage::wipeShared(SmartPtr<Details>& data_)
{
	if (!data_.isValid())
		return data_;

	map_type::iterator p = m_map.find(data_->m_VmIdent);
	if (m_map.end() == p)
	{
		if (0 == m_pending.count(data_->m_VmIdent))
		{
			// NB. CDspVm object may be created but not registered
			// in the map. this is the case. we should destroy
			// it properly this way.
			return data_;
		}
		// NB. someone creates CDspVm on a pending details object.
		// we should destroy it twice.
		return SmartPtr<Details>();
	}
	data_ = SmartPtr<Details>();
	if (!p->second.second || p->second.first.countRefs() > 1)
		return SmartPtr<Details>();

	if (!m_pending.insert(*p).second)
		return SmartPtr<Details>();

	SmartPtr<Details> output = p->second.first;
	m_map.erase(p);
	return output;
}

bool Storage::unregister(const CVmIdent& key_)
{
        map_type::iterator p = m_map.find(key_);
        if (m_map.end() == p)
                return false;

        p->second.second = true;
        return true;
}

Storage::snapshot_type Storage::unregisterAll()
{
	snapshot_type output;
	for (map_type::iterator p = m_map.begin(); p != m_map.end();)
	{
		state_type& r = p->second;
		// Trace to understand case when dispatcher lost VM
		WRITE_TRACE( DBG_FATAL , "UnregisterAllVmObjects: uuid = %s, name = '%s'"
			, QSTR2UTF8(r.first->m_VmIdent.first)
			, QSTR2UTF8(r.first->m_sVmName)
		);
		if (1 < r.first.countRefs())
		{
			r.second = true;
			++p;
		}
		else
		{
			output.push_back(make(r));
			m_map.erase(p++);
		}
	}
	return output;
}

Storage::value_type Storage::make(const state_type& state_)
{
        return value_type(new CDspVm(state_.first));
}

Storage::value_type Storage::find(const map_type& map_, const CVmIdent& key_)
{
	map_type::const_iterator p = map_.find(key_);
	if (map_.end() == p)
		return value_type();

	return make(p->second);
}

Storage::value_type Storage::find(const map_type& map_, const IOSender::Handle& connection_)
{
	map_type::const_iterator p = map_.begin(), e = map_.end();
	for (; p != e; ++p)
	{
		CDspDispConnection* x = p->second.first->m_pMigratingVmConnection.getImpl();
		if (NULL != x && x->GetConnectionHandle() == connection_)
			return make(p->second);
	}
	return value_type();
}

Storage::value_type Storage::enroll(SmartPtr<Details> data_)
{
	if (!data_.isValid())
		return value_type();

	const CVmIdent& k = data_->m_VmIdent;
	map_type::const_iterator p = m_map.find(k);
	if (m_map.end() != p)
		return make(p->second);

	return make(m_map.insert(std::make_pair(k, std::make_pair(data_, false))).first->second);
}

Storage::snapshot_type Storage::snapshot() const
{
	snapshot_type output;
	map_type::const_iterator p = m_map.begin(), e = m_map.end();
	for (; p != e; ++p)
		output.push_back(make(p->second));

	return output;
}

namespace Start
{
///////////////////////////////////////////////////////////////////////////////
// struct Demand

void Demand::reject(const CVmEvent& error_)
{
	WRITE_TRACE(DBG_FATAL, "Error: 0x%x, (%s)!",
		error_.getEventCode(),
		PRL_RESULT_TO_STRING(error_.getEventCode()));
	m_user->sendResponseError(error_, m_package);
}

void Demand::rejectBadConfig(const CVmEvent& error_)
{
	WRITE_TRACE(DBG_FATAL, "Configuration validation failed" );
	// send reply to user
	CProtoCommandPtr c = CProtoSerializer::CreateDspWsResponseCommand(m_package, PRL_ERR_INCONSISTENCY_VM_CONFIG);
	CProtoCommandDspWsResponse* p = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(c);
	p->SetParamsList(QStringList(error_.toString()));
	m_user->sendResponse(c, m_package);
}

///////////////////////////////////////////////////////////////////////////////
// struct TestSuite

TestSuite::TestSuite(Demand& demand_, SmartPtr<CVmConfiguration> config_):
	m_demand(&demand_), m_config(config_)
{
}

bool TestSuite::checkConfig()
{
	if (!m_config.isValid())
		return false;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(&m_demand->getActor()->getAuthHelper());

	CVmEvent a;
	CVmValidateConfig vc(m_config);
	if (vc.HasCriticalErrors(a))
	{
		LOG_MESSAGE( DBG_INFO, "CVmValidateConfig::validate() return true\n");
		m_demand->rejectBadConfig(a);
		return false;
	}
	CVmEvent b;
	if (vc.HasCriticalErrorsForStartVm(b))
	{
		m_demand->reject(b);
		return false;
	}
	return true;
}

bool TestSuite::checkHardware()
{
	if (!m_config.isValid())
		return false;

#if defined(EXTERNALLY_AVAILABLE_BUILD)
	if (IS_MACOS(m_config->getVmSettings()->getVmCommonOptions()->getOsVersion()))
	{
#if defined(PSBM_MAC) or !defined(_LIN_)
		// MacOS Guest Support EULA Enforcement
		// MacOS guest can be running only on MacOS Server host
		if (!HostUtils::CheckWhetherAppleHardware())
#endif
		{
			WRITE_TRACE(DBG_FATAL, "MacOS can be started on MacOS Server only");
			m_demand->reject(PRL_ERR_GUEST_MAC_NOT_MACSERVER_HOST);
			return false;
		}
	}
#endif
	return true;
}

bool TestSuite::checkFirewall()
{
	if (!CDspService::isServerModePSBM())
		return true;
	if (!m_config.isValid())
		return false;

	// Setup firewall
	CFirewallHelper fw(m_config);
	PRL_RESULT e = fw.Execute();
	if (PRL_SUCCEEDED(e))
		return true;

	CVmEvent v;
	v.setEventCode(e);
	if (PRL_ERR_FIREWALL_TOOL_EXECUTED_WITH_ERROR == e)
	{
		v.addEventParameter(new CVmEventParameter(
			PVE::String, fw.GetErrorMessage(),
			EVT_PARAM_DETAIL_DESCRIPTION));
	}
	m_demand->reject(v);
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// struct Setup

Setup::Setup(Demand& demand_, Details& vm_): m_vm(&vm_), m_demand(&demand_)
{
	PRL_RESULT e = PRL_ERR_UNINITIALIZED;
	if (vm_.m_pMigrateVmPkg.isValid())
	{
		// VM process was started at VM migration service mode
		m_config = vdh().getVmConfigByUuid(vm_.m_VmIdent.second,
			vm_.m_VmIdent.first, e);
	}
	else
	{
		m_config = vdh().getVmConfigByUuid(m_demand->getActor(),
			vm_.m_VmIdent.first, e);
	}
	if(!m_config.isValid())
	{
		PRL_ASSERT(PRL_FAILED(e));
		m_demand->reject(e);
	}
	else if (!vdh().loadSecureData(m_config))
	{
		// #9246
		WRITE_TRACE(DBG_FATAL, "Invalid secure data - cannot start VM !" );
		m_config.reset();
	}
}

bool Setup::prepare()
{
#ifdef _LIN_
	if (CDspService::isServerModePSBM())
	{
		if (!CCpuHelper::sync())
		{
			m_demand->reject(PRL_ERR_CLUSTER_RESOURCE_ERROR);
			return false;
		}
		return true;
	}
#endif
	return true;
}

bool Setup::check()
{
	if (Task_PrepareForHibernate::isTaskRunning())
	{
		m_demand->reject(PRL_ERR_PREPARE_FOR_HIBERNATE_TASK_ALREADY_RUN);
		return false;
	}
	TestSuite x(*m_demand, m_config);
	if (!x.checkConfig() || !x.checkHardware())
		return false;
	if (!x.checkFirewall())
	{
		WRITE_TRACE( DBG_FATAL, "Start VM: %s, uuid=%s cannot set up firewall !",
			QSTR2UTF8(m_vm->m_sVmName), QSTR2UTF8(m_vm->m_VmIdent.first) );
		return false;
	}
	return true;
}

SmartPtr<CDspClient> Setup::getUser() const
{
	if (!m_config.isValid())
		return SmartPtr<CDspClient>();

	SmartPtr<CDspClient> u = m_demand->getActor();
	PRL_RESULT e = vdh().getVmStartUser(m_config, u);
	if (PRL_FAILED(e))
	{
		m_demand->reject(e);
		return SmartPtr<CDspClient>();
	}
	if (m_demand->getActor() == u)
		return u;

	u->setVmDirectoryUuid(m_vm->m_VmIdent.second);
	CVmEvent v = m_vm->authorize(u,
			m_vm->m_pMigrateVmPkg.isValid() ?
			PVE::DspCmdCtlStartMigratedVm : PVE::DspCmdVmStart);
	if (PRL_SUCCEEDED(v.getEventCode()))
		return u;

	m_demand->reject(v);
	return SmartPtr<CDspClient>();
}

SmartPtr<CVmConfiguration> Setup::getConfig() const
{
	if (!m_config.isValid())
		return SmartPtr<CVmConfiguration>();

	SmartPtr<CVmConfiguration> output(new CVmConfiguration(*m_config));
	vdh().resetSecureParamsFromVmConfig(output);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Process

Process::Process(const CVmIdent& ident_, IOCommunication::SocketHandle socket_):
	m_ident(ident_), m_mode(PVE::VBMODE_DEFAULT), m_socket(socket_)
{
}

void Process::setMode(const CVmConfiguration& config_)
{
	// Get VM Controller full path
	PVE::VmBinaryMode m =  PVE::VBMODE_DEFAULT;
	// Check if 64-bit executable exists and OS supports 64-bit apps
	bool bX64 = true;
	if (QFile::exists(ParallelsDirs::getVmAppPath(bX64)) &&
	    osInfo_getArchitecture() == OSINFO_ARCHITECTURE_64_BIT)
		m = PVE::VBMODE_64BIT;
	// Force using of 32-bit or 64-bit VM binary
	QString systemFlags = config_.getVmSettings()->getVmRuntimeOptions()->getSystemFlags();
	if (systemFlags.contains("vm.app_mode=32"))
		m = PVE::VBMODE_32BIT;
	else if (systemFlags.contains("vm.app_mode=64"))
		m = PVE::VBMODE_64BIT;

	m_mode = getWinVmBinaryMode(m, m_ident);
}

QStringList Process::getEnvironment(CDspClient& user_) const
{
	// Get custom environment for Vm process
	QStringList output = Prl::getenvU(PVS_VM_ENVIRONMENT_ENV)
					 .split(PVS_VM_ENVIRONMENT_ENV_SEPARATOR,
							QString::SkipEmptyParts);
	for (int i = 0; output.size() > i; ++i)
	{
		QString &str = output[i];
		str = str.trimmed();
		WRITE_TRACE(DBG_FATAL, "Custom VM envar for vmUuid=\"%s\": \"%s\"",
					QSTR2UTF8(m_ident.first), QSTR2UTF8(str));
	}

	// Appending user's environment to the vmEnvironment list
	output += getUsersEnvironment(user_.getAuthHelper());

	bool bAddXServerEnv = false;
#ifdef _LIN_
	bAddXServerEnv = !CDspService::isServerMode(); // to prevent vm_app crash in PSBM where XSession was closed
#endif
	// #PWE-5753 to increase VGA performance on Linux
	if( bAddXServerEnv )
	{
		QString val;
		QStringList env = QStringList() << "DISPLAY" << "XAUTHORITY";
		foreach( QString key, env )
			if(user_.getClientEnvinromentVariable(key, val) )
				output += QString("%1=%2").arg(key).arg(val);
	}
	return output;
}

QStringList Process::getArguments() const
{
	QString strExecuteMode;
	switch( ParallelsDirs::getAppExecuteMode() )
	{
	case PAM_SERVER:
		strExecuteMode = CommandLine::g_strCommonValue_ModeName_PS;
		break;
	default:
		WRITE_TRACE(DBG_FATAL, "Unsupported app executed mode %d",
			ParallelsDirs::getAppExecuteMode());
		throw PRL_ERR_UNIMPLEMENTED;
	}//switch

	QStringList output;
	// Setup VM process arguments list
	output.append( CommandLine::g_strCommonKeyName_Uuid );
	output.append( m_ident.first );
	output.append( CommandLine::g_strCommonKeyName_DirUuid );
	output.append( m_ident.second );
	output.append( CommandLine::g_strCommonKeyName_ModeName);
	output.append( strExecuteMode );

	return output;
}

QString Process::getBinary() const
{
	return ParallelsDirs::getVmAppPath(m_mode == PVE::VBMODE_64BIT);
}

QString Process::getWorkingDir() const
{
	QString x = QFileInfo(getBinary()).absolutePath();
	return QDir::fromNativeSeparators(QDir::cleanPath(x));
}

Process* Process::yield(const CVmIdent& ident_)
{
	IOCommunication::SocketHandle h;
#ifdef _WIN_
	h = ds().getListeningIOServer().createDetachedClientSocket();
	if (!h.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Error: can't detach client socket for Vm. "
					"Will not start process!");
		return NULL;
	}
#endif
	return new Process(ident_, h);
}

} // namespace Start
} // namespace DspVm

