///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmManager.cpp
///
/// Vm manager and handler, which is responsible for all Vms and
/// packages for these vms or packages from these vms.
///
/// @author romanp, sandro
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

#define FORCE_LOGGING_LEVEL DBG_INFO

#include "CDspVmManager.h"
#include "CDspService.h"
#include "CDspHandlerRegistrator.h"
#include "CDspClientManager.h"
#include "CDspRouter.h"
#include "CDspProblemReportHelper.h"
#include "CDspVmInfoDatabase.h"

#include "Libraries/PrlCommonUtils/CVmQuestionHelper.h"
#include "Libraries/NonQtUtils/CQuestionHelper.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"
#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include <Libraries/PowerWatcher/PowerWatcher.h>

#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/ProblemReport/CProblemReport.h"
#include "XmlModel/Messaging/CVmBinaryEventParameter.h"
#include "Libraries/HostInfo/CHostInfo.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "XmlModel/HostHardwareInfo/CHwGenericDevice.h"

#include "Tasks/Task_SwitchToSnapshot.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Tasks/Task_ChangeSID.h"

#ifdef _WIN_
	#include <process.h>
	#define getpid _getpid
#endif

#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

#include "Build/Current.ver" /* for SENTILLION_VTHERE_PLAYER */

template < class T > inline
	QString g_GetDeviceSystemName( CVmEventParameter *pDeviceConfigParam, bool &bConnected )
{
	T vmDevice;
	StringToElement<T*>(&vmDevice, pDeviceConfigParam->getData() );
	bConnected = ( PVE::DeviceConnected == vmDevice.getConnected() );
	return vmDevice.getSystemName();
}

static void SendEchoEventToVm( SmartPtr<CDspVm> &pVm, const CVmEvent& evt )
{
	SmartPtr<IOPackage> _p = DispatcherPackage::createInstance( PVE::DspVmSendEchoEvent, evt );
	pVm->sendPackageToVm(_p);
}

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Context

struct Context
{
	Context(const SmartPtr<CDspClient>& session_, const SmartPtr<IOPackage>& package_);

	const CVmIdent& getIdent() const
	{
		return m_ident;
	}
	const QString& getVmUuid() const
	{
		return m_ident.first;
	}
	const QString& getDirectoryUuid() const
	{
		return m_ident.second;
	}
	const SmartPtr<CDspClient>& getSession() const
	{
		return m_session;
	}
	const SmartPtr<IOPackage>& getPackage() const
	{
		return m_package;
	}
	const CProtoCommandPtr& getRequest() const
	{
		return m_request;
	}
	PVE::IDispatcherCommands getCommand() const
	{
		return m_request->GetCommandId();
	}
	void reply(int code_) const
	{
		m_session->sendSimpleResponse(m_package, code_);
	}
	void reply(const CVmEvent& error_) const
	{
		m_session->sendResponseError(&error_, m_package);
	}

private:
	SmartPtr<CDspClient> m_session;
	SmartPtr<IOPackage> m_package;
	CProtoCommandPtr m_request;
	CVmIdent m_ident;
};

Context::Context(const SmartPtr<CDspClient>& session_, const SmartPtr<IOPackage>& package_):
	m_session(session_), m_package(package_)
{
	m_request = CProtoSerializer::ParseCommand((PVE::IDispatcherCommands)m_package->header.type,
						UTF8_2QSTR(m_package->buffers[0].getImpl()));
	PRL_ASSERT(m_request.isValid());
	PRL_ASSERT(m_request->IsValid());
	m_ident = MakeVmIdent(m_request->GetVmUuid(), m_session->getVmDirectoryUuid());
}

namespace Tag
{
///////////////////////////////////////////////////////////////////////////////
// struct Simple

template<PVE::IDispatcherCommands X>
struct Simple
{
};

///////////////////////////////////////////////////////////////////////////////
// struct General

template<PVE::IDispatcherCommands X>
struct General
{
};

///////////////////////////////////////////////////////////////////////////////
// struct CreateDspVm

template<PVE::IDispatcherCommands X>
struct CreateDspVm: General<X>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct GuestSession

struct GuestSession
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Special

template<PVE::IDispatcherCommands X>
struct Special
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Libvirt

template<PVE::IDispatcherCommands X>
struct Libvirt
{
};

} // namespace Tag

namespace Details
{
///////////////////////////////////////////////////////////////////////////////
// struct Assistant

struct Assistant
{
	explicit Assistant(const Context& context_): m_context(&context_)
	{
	}

	bool canAccessVm() const;
	SmartPtr<CDspVm> createVm() const;
	void answerPendingQuestion(CDspVm& vm_) const;
	SmartPtr<CVmConfiguration> getConfig() const;
	static void sendDefaultAnswer(CDspVm& vm_, IOPackage& question_);

private:
	const Context* m_context;
};

SmartPtr<CVmConfiguration> Assistant::getConfig() const
{
	CVmEvent v;
	PRL_RESULT e = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> output = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(m_context->getSession(), m_context->getVmUuid(),
			e, &v);
	if (!output.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "handleFromDispatcherPackage: cannot get VM config: error %s !",
					PRL_RESULT_TO_STRING(e));
		m_context->reply(v);
	}

	return output;
}

SmartPtr<CDspVm> Assistant::createVm() const
{
	const QString& v = m_context->getVmUuid();
	const QString& d = m_context->getDirectoryUuid();
	// FIXME!!! It needs to check bNew value  (#123497)
	bool bNew = false;
	PRL_RESULT e = PRL_ERR_SUCCESS;
	SmartPtr<CDspVm> output = CDspVm::CreateInstance(v, d, e,
			bNew, m_context->getSession(), m_context->getCommand());
	if (!output.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Can't create instance of Vm '%s' "
			"which belongs to '%s' VM dir"
			"by error %#x", QSTR2UTF8(v), QSTR2UTF8(d), e);

		PRL_ASSERT(PRL_FAILED(e));
		m_context->reply(e);
	}
	return output;
}

void Assistant::answerPendingQuestion(CDspVm& vm_) const
{
	// bug #423415
	SmartPtr<IOPackage> q = vm_.getQuestionPacket();
	if (q.isValid() && m_context->getSession()->isNonInteractive())
	{
		WRITE_TRACE(DBG_FATAL, "VM uuid=%s has question and default answer will send to VM in order to unfreeze VM!",
					QSTR2UTF8(vm_.getVmUuid()));

		sendDefaultAnswer(vm_, *q);
	}
}

void Assistant::sendDefaultAnswer(CDspVm& vm_, IOService::IOPackage& question_)
{
	QString n = vm_.getVmName();
	QString u = vm_.getVmUuid();
	PRL_RESULT q = CVmEvent(UTF8_2QSTR(question_.buffers[0].getImpl())).getEventCode();
	WRITE_TRACE(DBG_FATAL, "Sending default answer on question %.8X '%s' to VM '%s' '%s'",\
				q, PRL_RESULT_TO_STRING(q), QSTR2UTF8(n), QSTR2UTF8(u));

	PRL_RESULT e = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> c = CDspService::instance()->getVmDirHelper().
					  getVmConfigByUuid(vm_.getVmIdent(), e);

	SmartPtr<IOPackage> p(&question_, SmartPtrPolicy::DoNotReleasePointee);
	SmartPtr<IOPackage> a = CVmQuestionHelper::getDefaultAnswerToVm(c.getImpl(), p);
	// Send answer to VM
	if (a.isValid())
	{
		vm_.sendPackageToVm(a);
		CVmEvent r( UTF8_2QSTR(a->buffers[0].getImpl()) );
		CVmEventParameter *p = r.getEventParameter( EVT_PARAM_MESSAGE_CHOICE_0 );
		e = NULL == p ? PRL_ERR_UNINITIALIZED : p->getParamValue().toInt();
		WRITE_TRACE(DBG_FATAL, "To VM '%s' '%s' was sent default answer %.8X '%s' on quesiton %.8X '%s'.",\
					QSTR2UTF8(n), QSTR2UTF8(u), e, PRL_RESULT_TO_STRING(e),
					q, PRL_RESULT_TO_STRING(q));
	}
	else
	{
		PRL_ASSERT(!!0);
		WRITE_TRACE(DBG_FATAL, "Can't send default answer on question %.8X '%s' to VM '%s' '%s'",\
					q, PRL_RESULT_TO_STRING(q), QSTR2UTF8(n), QSTR2UTF8(u));
	}
}

bool Assistant::canAccessVm() const
{
	const SmartPtr<CDspClient>& u = m_context->getSession();
	//At first collect necessary info about user session access to VM
	if (u->getAuthHelper().isLocalAdministrator())
		return true;

	CDspLockedPointer<CVmDirectoryItem> x = CDspService::instance()->getVmDirManager()
			.getVmDirItemByUuid(m_context->getIdent());
	PRL_ASSERT(x.getPtr());
	CDspAccessManager& m = CDspService::instance()->getAccessManager();
	if (m.isOwnerOfVm(u, x.getPtr()))
		return true;

	PRL_SEC_AM r = m.getAccessRightsToVm(u, x.getPtr()).getVmAccessRights();
	return	(r & CDspAccessManager::VmAccessRights::arCanRead) &&
		(r & CDspAccessManager::VmAccessRights::arCanWrite) &&
		(r & CDspAccessManager::VmAccessRights::arCanExecute);
}

///////////////////////////////////////////////////////////////////////////////
// struct General

template<class T>
struct General
{
	static void do_(const Context& context_, SmartPtr<CDspVm> vm_);
};

template<>
void General<Tag::General<PVE::DspCmdVmInternal> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->InternalCmd(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmReset> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->reset(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmPause> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->pause(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmSuspend> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->suspend(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmDevConnect> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->connectDevice(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmDevDisconnect> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->disconnectDevice(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::Simple<PVE::DspCmdVmAnswer> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->sendAnswerToVm(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmInitiateDevStateNotifications> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->InitiateDevStateNotifications(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmInstallUtility> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->installUtility(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmInstallTools> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->installTools(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmUpdateToolsSection> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->updateToolsSection(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmRunCompressor> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->runCompressor(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmCancelCompressor> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->cancelCompressor(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::Simple<PVE::DspCmdVmStartVNCServer> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->startVNCServer(context_.getSession(), context_.getPackage(), false, true);
}

template<>
void General<Tag::Simple<PVE::DspCmdVmStopVNCServer> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->stopVNCServer(context_.getSession(), context_.getPackage(), false, true);
}

template<>
void General<Tag::General<PVE::DspCmdVmMigrateCancel> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	CDspService::instance()->getVmMigrateHelper()
		.cancelMigration(context_.getSession(), context_.getPackage(), vm_);
}

template<>
void General<Tag::General<PVE::DspCmdVmRestartGuest> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->restartGuest(context_.getSession(), context_.getPackage());
}

template<>
void General<Tag::General<PVE::DspCmdVmStop> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	CProtoVmCommandStop* x = CProtoSerializer::CastToProtoCommand
		<CProtoVmCommandStop>(context_.getRequest());
	vm_->stop(context_.getSession(), context_.getPackage(), x->GetStopMode());
}

template<>
void General<Tag::GuestSession >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	vm_->processGuestOsSessionCmd(context_.getSession(),
		context_.getRequest(), context_.getPackage());
}

template<PVE::IDispatcherCommands X>
struct General<Tag::CreateDspVm<X> >
{
	static bool do_(const Context& context_, SmartPtr<CDspVm> vm_);
};

template<>
bool General<Tag::CreateDspVm<PVE::DspCmdVmCreateSnapshot> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	return vm_->createSnapshot(context_.getSession(), context_.getPackage());
}

template<>
bool General<Tag::CreateDspVm<PVE::DspCmdVmSwitchToSnapshot> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	return vm_->switchToSnapshot(context_.getSession(), context_.getPackage());
}

template<>
bool General<Tag::CreateDspVm<PVE::DspCmdVmDeleteSnapshot> >::do_(const Context& context_, SmartPtr<CDspVm> vm_)
{
	return vm_->deleteSnapshot(context_.getSession(), context_.getPackage());
}

///////////////////////////////////////////////////////////////////////////////
// struct Proxy

template<class T>
struct Proxy
{
	static void do_(Context& context_, SmartPtr<CDspVm> vm_)
	{
		Assistant(context_).answerPendingQuestion(*vm_);
		General<T>::do_(context_, vm_);
	}
};

template<PVE::IDispatcherCommands X>
struct Proxy<Tag::Simple<X> >: General<Tag::Simple<X> >
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Execute

template<class T>
struct Execute: Proxy<T>
{
	using Proxy<T>::do_;

	static void do_(Context& context_, const CVmConfiguration& config_)
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to find running VM with uuid '%s' which belongs to '%s' VM dir",
				QSTR2UTF8(context_.getVmUuid()), QSTR2UTF8(context_.getDirectoryUuid()));

		CVmEvent evt;
		evt.setEventCode( PRL_ERR_DISP_VM_IS_NOT_STARTED );
		evt.addEventParameter( new CVmEventParameter (
			PVE::String,
			config_.getVmIdentification()->getVmName(),
			EVT_PARAM_MESSAGE_PARAM_0 ) );

		context_.reply(evt);
	}
};

template<PVE::IDispatcherCommands X>
struct Execute<Tag::CreateDspVm<X> >
{
	static bool do_(Context& context_, SmartPtr<CDspVm> vm_)
	{
		Assistant(context_).answerPendingQuestion(*vm_);
		return General<Tag::CreateDspVm<X> >::do_(context_, vm_);
	}
	static void do_(Context& context_, const CVmConfiguration& config_)
	{
		// VM process borning now - let instantiate it
		if (config_.getVmSettings()->getVmCommonOptions()->isTemplate())
		{
			WRITE_TRACE(DBG_FATAL, "Can't start the template Vm '%s' "
				"which belongs to '%s' VM dir"
				"by error %#x",
				QSTR2UTF8(context_.getVmUuid()),
				QSTR2UTF8(context_.getDirectoryUuid()),
				PRL_ERR_CANT_TO_START_VM_TEMPLATE);
			context_.reply(PRL_ERR_CANT_TO_START_VM_TEMPLATE);
			return;
		}
		SmartPtr<CDspVm> m = Assistant(context_).createVm();
		if (m.isValid() && do_(context_, m))
			CDspVm::UnregisterVmObject(m);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Body

template<class T>
struct Body
{
	static void run(Context& context_);
};

template<class T>
void Body<T>::run(Context& context_)
{
	SmartPtr<CDspVm> m = CDspVm::GetVmInstanceByUuid(context_.getIdent());
	if (m.isValid())
		return (void)Execute<T>::do_(context_, m);

	SmartPtr<CVmConfiguration> c = Details::Assistant(context_).getConfig();
	if (c.isValid())
		Execute<T>::do_(context_, *c);
}

template<>
void Body<Tag::Special<PVE::DspCmdVmChangeSid> >::run(Context& context_)
{
	SmartPtr<CVmConfiguration> c = Details::Assistant(context_).getConfig();
	if (!c.isValid())
		return;

	CDspService::instance()->getTaskManager().schedule(
		new Task_ChangeSID(context_.getSession(), context_.getPackage(), c, true));
}

template<>
void Body<Tag::Special<PVE::DspCmdVmResetUptime> >::run(Context& context_)
{
	if (!Details::Assistant(context_).canAccessVm())
		return context_.reply(PRL_ERR_ACCESS_DENIED);

	WRITE_TRACE(DBG_FATAL, "Resetting VM '%s' uptime", QSTR2UTF8(context_.getVmUuid()));
	//Reset uptime for VM process
	{
		SmartPtr<CDspVm> m = CDspVm::GetVmInstanceByUuid(context_.getIdent());
		if (m.isValid())
			m->resetUptime();
	}
	//Reset uptime at configuration
	// Save config
	CVmEvent _evt;
	_evt.addEventParameter(new CVmEventParameter(PVE::String, "0", EVT_PARAM_VM_UPTIME_DELTA));
	if (CDspService::instance()->getVmDirHelper()
		.atomicEditVmConfigByVm(context_.getIdent(), _evt, context_.getSession()))
		return context_.reply(PRL_ERR_SUCCESS);

	WRITE_TRACE(DBG_FATAL, "error on resetting uptime of VM '%s' at configuration", QSTR2UTF8(context_.getVmUuid()));
	context_.reply(PRL_ERR_FAILURE);
}

template<>
void Body<Tag::Special<PVE::DspCmdVmAuthWithGuestSecurityDb> >::run(Context& context_)
{
	CDspService::instance()->getTaskManager().schedule(
		new Task_AuthUserWithGuestSecurityDb(context_.getSession(),
			context_.getPackage(), context_.getRequest()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Body<Tag::Libvirt<X> >

template<PVE::IDispatcherCommands X>
struct Body<Tag::Libvirt<X> >: QRunnable
{
	void run();

	static void run(Context& context_)
	{
		QRunnable* q = new Body(context_);
		q->setAutoDelete(true);
		QThreadPool::globalInstance()->start(q);
	}

private:
	Body(const Context& context_): m_context(context_)
	{
	}

	Context m_context;
};

template<>
void Body<Tag::Libvirt<PVE::DspCmdVmStop> >::run()
{
#ifdef _LIBVIRT_
	m_context.reply(Libvirt::Kit.vms().at(m_context.getVmUuid()).stop());
#endif // _LIBVIRT_
}

template<>
void Body<Tag::Libvirt<PVE::DspCmdVmStart> >::run()
{
#ifdef _LIBVIRT_
	m_context.reply(Libvirt::Kit.vms().at(m_context.getVmUuid()).start());
#endif // _LIBVIRT_
}

} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// struct Dispatcher

struct Dispatcher
{
	Dispatcher();

	void do_(const SmartPtr<CDspClient>& session_, const SmartPtr<IOPackage>& package_) const;

private:
	typedef void ( *command_type)(Context& context_);
	typedef QHash<PVE::IDispatcherCommands, command_type> map_type;

	template<class T>
	static command_type map(T)
	{
		return &Details::Body<T>::run;
	}

	map_type m_map;
};

Dispatcher::Dispatcher()
{
	m_map[PVE::DspCmdVmChangeSid] = map(Tag::Special<PVE::DspCmdVmChangeSid>());
	m_map[PVE::DspCmdVmResetUptime] = map(Tag::Special<PVE::DspCmdVmResetUptime>());
	m_map[PVE::DspCmdVmAuthWithGuestSecurityDb] = map(Tag::Special<PVE::DspCmdVmAuthWithGuestSecurityDb>());
	m_map[PVE::DspCmdVmStart] = map(Tag::Libvirt<PVE::DspCmdVmStart>());
	m_map[PVE::DspCmdVmStartEx] = map(Tag::Libvirt<PVE::DspCmdVmStart>());
	m_map[PVE::DspCmdVmCreateSnapshot] = map(Tag::CreateDspVm<PVE::DspCmdVmCreateSnapshot>());
	m_map[PVE::DspCmdVmSwitchToSnapshot] = map(Tag::CreateDspVm<PVE::DspCmdVmSwitchToSnapshot>());
	m_map[PVE::DspCmdVmDeleteSnapshot] = map(Tag::CreateDspVm<PVE::DspCmdVmDeleteSnapshot>());
	m_map[PVE::DspCmdVmAnswer] = map(Tag::Simple<PVE::DspCmdVmAnswer>());
	m_map[PVE::DspCmdVmStartVNCServer] = map(Tag::Simple<PVE::DspCmdVmStartVNCServer>());
	m_map[PVE::DspCmdVmStopVNCServer] = map(Tag::Simple<PVE::DspCmdVmStopVNCServer>());
	m_map[PVE::DspCmdVmInternal] = map(Tag::General<PVE::DspCmdVmInternal>());
	m_map[PVE::DspCmdVmReset] = map(Tag::General<PVE::DspCmdVmReset>());
	m_map[PVE::DspCmdVmPause] = map(Tag::General<PVE::DspCmdVmPause>());
	m_map[PVE::DspCmdVmSuspend] = map(Tag::General<PVE::DspCmdVmSuspend>());
	m_map[PVE::DspCmdVmDevConnect] = map(Tag::General<PVE::DspCmdVmDevConnect>());
	m_map[PVE::DspCmdVmDevDisconnect] = map(Tag::General<PVE::DspCmdVmDevDisconnect>());
	m_map[PVE::DspCmdVmInitiateDevStateNotifications] = map(Tag::General<PVE::DspCmdVmInitiateDevStateNotifications>());
	m_map[PVE::DspCmdVmInstallUtility] = map(Tag::General<PVE::DspCmdVmInstallUtility>());
	m_map[PVE::DspCmdVmInstallTools] = map(Tag::General<PVE::DspCmdVmInstallTools>());
	m_map[PVE::DspCmdVmUpdateToolsSection] = map(Tag::General<PVE::DspCmdVmUpdateToolsSection>());
	m_map[PVE::DspCmdVmRunCompressor] = map(Tag::General<PVE::DspCmdVmRunCompressor>());
	m_map[PVE::DspCmdVmCancelCompressor] = map(Tag::General<PVE::DspCmdVmCancelCompressor>());
	m_map[PVE::DspCmdVmMigrateCancel] = map(Tag::General<PVE::DspCmdVmMigrateCancel>());
	m_map[PVE::DspCmdVmRestartGuest] = map(Tag::General<PVE::DspCmdVmRestartGuest>());
	m_map[PVE::DspCmdVmStop] = map(Tag::Libvirt<PVE::DspCmdVmStop>());
	m_map[PVE::DspCmdVmLoginInGuest] = map(Tag::GuestSession());
	m_map[PVE::DspCmdVmGuestLogout] = map(Tag::GuestSession());
	m_map[PVE::DspCmdVmGuestRunProgram] = map(Tag::GuestSession());
	m_map[PVE::DspCmdVmGuestGetNetworkSettings] = map(Tag::GuestSession());
	m_map[PVE::DspCmdVmGuestSetUserPasswd] = map(Tag::GuestSession());
	m_map[PVE::DspCmdVmGuestChangeSID] = map(Tag::GuestSession());
}

void Dispatcher::do_(const SmartPtr<CDspClient>& session_, const SmartPtr<IOPackage>& package_) const
{
	Context x(session_, package_);
	PVE::IDispatcherCommands y = x.getCommand();
	map_type::const_iterator p = m_map.find(y);
	if (m_map.end() != p)
		return p.value()(x);

	WRITE_TRACE(DBG_FATAL, "Couldn't to process package with type %d '%s'",
		y, PVE::DispatcherCommandToString(y));
	x.reply(PRL_ERR_UNRECOGNIZED_REQUEST);
}

Q_GLOBAL_STATIC(Dispatcher, getDispatcher);

} // namespace Command

void CDspVmManager::updateUsbDeviceState( const SmartPtr<CDspVm> &pVm, const CVmEvent &_evt )
{
	CVmEventParameter *pDeviceTypeParam = _evt.getEventParameter( EVT_PARAM_DEVICE_TYPE );

	if ( !pDeviceTypeParam ) {
		WRITE_TRACE(DBG_FATAL,
					"Can't get device type from vm's event! (%s)",
					QSTR2UTF8( _evt.toString() ) );
		return;
	}

	PRL_DEVICE_TYPE dev_type = (PRL_DEVICE_TYPE)pDeviceTypeParam->getParamValue().toUInt();
	CVmEventParameter *pDeviceConfigParam = _evt.getEventParameter( EVT_PARAM_VM_CONFIG_DEV_STATE );

	if ( !pDeviceConfigParam ) {
		WRITE_TRACE(DBG_FATAL,
					"Can't get device configuration from vm's event! (%s)",
					QSTR2UTF8( _evt.toString() ) );
		return;
	}

	QString	strDeviceSystemName;
	bool	bConnected;
	bool	bNeedSendHwNotify = false;

	switch( dev_type )
	{
		case PDE_USB_DEVICE:
			strDeviceSystemName = g_GetDeviceSystemName<CVmUsbDevice>( pDeviceConfigParam, bConnected );
			break;
		default:
			return;
	}

	// walk over USB devices list to search itseflf for PDE_USB_DEVICE and
	// parent devices for USB-to-COM, USB-HDD, and USB-CD/DVD
	CDspLockedPointer<CDspHostInfo> spHostInfo = CDspService::instance()->getHostInfo();
	QList<CHwUsbDevice*> &lstUsbDevices = spHostInfo->data()->m_lstUsbDevices;
	foreach( CHwUsbDevice* pUsbDevice, lstUsbDevices )
	{
		if ( dev_type == PDE_USB_DEVICE ) {
			if ( strDeviceSystemName.compare( pUsbDevice->getDeviceId(),
											  Qt::CaseInsensitive ) )
				continue;
		} else {
			if ( -1 == strDeviceSystemName.lastIndexOf( pUsbDevice->getDeviceName() ) )
				continue;
		}

		// USB device found, update state by event (connect/disconnect)
		QStringList lstVmUuids = pUsbDevice->getVmUuids();

		if ( bConnected )
		{
			// we already own that device, nothung to do
			// TODO: think how to avoid recursive events:
			//     PET_DSP_EVT_VM_DEV_STATE_CHANGED(connected) ->
			//       PET_DSP_EVT_HW_CONFIG_CHANGED(vmid, to-vm) ->
			//         PET_DSP_EVT_VM_DEV_STATE_CHANGED(connected)... WTF
			if ( lstVmUuids.contains( pVm->getVmUuid() ) ) {
				WRITE_TRACE(DBG_WARNING,
							"USB device \"%s\" aleady owned by this VM %s",
							QSTR2UTF8( pUsbDevice->getDeviceName() ),
							QSTR2UTF8( pVm->getVmUuid() ) );
				break;
			}

			// sanity checks: no other owners, but skip shared device (CCID)
			if ( !pUsbDevice->getDeviceId().startsWith(PRL_VIRTUAL_CCID_PATH0) ) {
				PRL_ASSERT( pUsbDevice->getDeviceState() == PGS_CONNECTED_TO_HOST );
				PRL_ASSERT( lstVmUuids.isEmpty() );
			}

			// take ownership
			lstVmUuids.append( pVm->getVmUuid() );
			pUsbDevice->setVmUuids( lstVmUuids );

			if ( dev_type != PDE_USB_DEVICE )
				pUsbDevice->setDeviceState( PGS_NON_CONTROLLED_USB );
			else if ( !strDeviceSystemName.section( '|', 4, 4 ).compare( "PW" ) )
				pUsbDevice->setDeviceState( PGS_CONNECTING_TO_VM );
			else
				pUsbDevice->setDeviceState( PGS_CONNECTED_TO_VM );
		} else {
			// check that we own that device, don't touch if it not us
			// TODO: is this check really needed ?
			//   This check intended to prevent possible recursive events as
			//   in "connect" branch above. But I don't see that condition
			//   during smoke tests.
			if ( !lstVmUuids.contains(pVm->getVmUuid()) ) {
				WRITE_TRACE(DBG_WARNING,
							"USB device \"%s\" not owned by this VM %s",
							QSTR2UTF8( pUsbDevice->getDeviceName() ),
							QSTR2UTF8( pVm->getVmUuid() ) );
				break;
			}

			// sanity check: device state
			PRL_ASSERT( pUsbDevice->getDeviceState() == PGS_CONNECTED_TO_VM
						|| pUsbDevice->getDeviceState() == PGS_CONNECTING_TO_VM
						|| pUsbDevice->getDeviceState() == PGS_NON_CONTROLLED_USB );

			// release ownership
			lstVmUuids.removeAll(pVm->getVmUuid());
			pUsbDevice->setVmUuids( lstVmUuids );

			if ( lstVmUuids.isEmpty() ) {
				// return device to host
				pUsbDevice->setDeviceState( PGS_CONNECTED_TO_HOST );
			} else {
				// sanity check: only CCID device can be shared between VMs now,
				//   others should have only one owner (released now)
				PRL_ASSERT( lstVmUuids.isEmpty()
							|| pUsbDevice->getDeviceId().startsWith(PRL_VIRTUAL_CCID_PATH0) );
			}
		}
		bNeedSendHwNotify = true;
		break;
	}
	spHostInfo.unlock();

	// Notify users that host hardware configuration was changed
	if ( bNeedSendHwNotify )
	{
		CVmEvent event( PET_DSP_EVT_HW_CONFIG_CHANGED, Uuid().toString(), PIE_DISPATCHER );
		SmartPtr<IOPackage> _p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
		sendPackageFromVmToPermittedUsers( pVm, _p );
	}
}



using namespace Parallels;

REGISTER_HANDLER( IOService::IOSender::Vm,
				  "VmHandler",
				  CDspVmManager);

/*****************************************************************************/

CDspVmManager::CDspVmManager ( IOSender::Type type, const char* name ) :
	CDspHandler(type, name)
{
}

CDspVmManager::~CDspVmManager ()
{
}

void CDspVmManager::init ()
{
}

void CDspVmManager::handleClientConnected ( const IOSender::Handle& h )
{
	WRITE_TRACE( DBG_WARNING, "%s", __FUNCTION__ );

	// Check Vm protocol version
	IOCommunication::ProtocolVersion ver;
	::memset(&ver, 0, sizeof(ver));
	if ( ! CDspService::instance()->getIOServer().clientProtocolVersion(h, ver) ||
		 IOService::IOProtocolVersion.majorNumber != ver.majorNumber ||
		 IOService::IOProtocolVersion.minorNumber != ver.minorNumber )
	{
		WRITE_TRACE( DBG_FATAL,
					 "Error: Dispatcher protocol version [%d.%d, %s] "
					 "differs from Vm protocol version [%d.%d, %s]. "
					 "Vm will be stopped.",
					 IOService::IOProtocolVersion.majorNumber,
					 IOService::IOProtocolVersion.minorNumber,
					 IOService::IOProtocolVersion.whatWeAre,
					 ver.majorNumber,
					 ver.minorNumber,
					 ver.whatWeAre  );
		CDspService::instance()->getIOServer().disconnectClient(h);
	}

	if (getAllRunningVms().size() == 0)
	{
		CPowerWatcher::setDarkWakeEnable(true);
		if (CPowerWatcher::isDarkWakeEnabled())
			WRITE_TRACE(DBG_DEBUG,"CDspVmManager::handleClientConnected: DarkWake is enabled");
		else
			WRITE_TRACE(DBG_DEBUG,"CDspVmManager::handleClientConnected: DarkWake is disabled");
	}
}

void CDspVmManager::handleClientDisconnected ( const IOSender::Handle& h )
{
	WRITE_TRACE( DBG_WARNING, "%s", __FUNCTION__ );

	QWriteLocker _lock(&m_rwLock);
	SmartPtr<CDspVm> pVm = m_vms.take(h);

	_lock.unlock();

	if (getAllRunningVms().size() == 0)
	{
		CPowerWatcher::setDarkWakeEnable(false);
		WRITE_TRACE(DBG_DEBUG, "CDspVmManager::handleClientDisconnected, DarkWake is disabled");
	}

	if ( ! pVm )//In common case we can do not have life object here
		return;

	WRITE_TRACE( DBG_WARNING, "%s: begin to processing", __FUNCTION__ );

	// Notify running snapshot tasks that connection with VM closed
	{
#ifdef _LIN_
		CVzHelper::release_cpu_mask(pVm->getVmUuid());
#endif

		SmartPtr<IOPackage> pkg;
		VIRTUAL_MACHINE_STATE vmState;
		SmartPtr<CDspClient> user;
		QString sTaskUuid;
		pVm->GetSnapshotRequestParams(pkg, vmState, user, sTaskUuid);

		bool bPoststopScriptExecuted = false;
		if (pkg.getImpl() && pkg->header.type == PVE::DspCmdVmSwitchToSnapshot)
		{
			SmartPtr< CDspTaskHelper > pTask = CDspService::instance()->getTaskManager()
				.findTaskByUuid( sTaskUuid );
			if (pTask)
			{
				Task_SwitchToSnapshot* pTaskSwitch = dynamic_cast<Task_SwitchToSnapshot*>(pTask.getImpl());

				bool bNeedUnregisterVmObject = false;
				bPoststopScriptExecuted = true;
				pTaskSwitch->handleClientDisconnected( bNeedUnregisterVmObject );
				if( !bNeedUnregisterVmObject )
					return;
			}
		}

		if (!bPoststopScriptExecuted)
		{
			// For shapshot action posstop script called in the Task_SwitchToSnapshot.
			// FIXME for UndoDisksRevertOrCommitTask logic posstop script called
			// before the Task_DeleteSnapshot task
			pVm->runActionScript(PVA_POSTSTOP, pVm);
		}
	}

	if (pVm->isUndoDisksMode() && pVm->getVmState() != VMS_SUSPENDING_SYNC)
	{
		// For now only on Mac OS we must send report if Vm suddenly
		// disconnects! On other platforms report will be send
		// after crash report monitor has detected Vm dump.
		// If crash dump has not been generated in 10 seconds,
		// we send dumpless report to the clients.
		tryToSendProblemReport(pVm);

		bool bRes = pVm->startUndoDisksRevertOrCommitTask();
		if (!bRes)
		{
			CDspVm::UnregisterVmObject(pVm);
		}
		return;
	}
	else
	{
		CDspVm::UnregisterVmObject(pVm);
	}
	if (pVm.isValid())
	{

		{	// Logic to send STOPPED event by old schema ( revision <194322 )
			// on new schema STOPPED event sent from destructor of CDspVm

			// THIS LOGIC ONLY TO PROVIDE COMPATIBILITY WITH OLD CLIENTS ONLY( based on revision < 194322  )
			// BUG #122476


			PRL_EVENT_TYPE eventType;

			VIRTUAL_MACHINE_STATE vmState;
			vmState = CDspVm::getVmState( pVm->getVmUuid(), pVm->getVmDirUuid() );

			eventType = vmState == VMS_SUSPENDED ? PET_DSP_EVT_VM_SUSPENDED :
				PET_DSP_EVT_VM_STOPPED;

			SmartPtr<CVmEvent> pVmStoppedEvent( new CVmEvent(eventType, pVm->getVmUuid(), PIE_VIRTUAL_MACHINE) );
			SmartPtr<IOPackage> pStop= DispatcherPackage::createInstance(PVE::DspVmEvent, pVmStoppedEvent->toString());

			// send stop event to clients!
			CDspService::instance()->getClientManager().
				sendPackageToVmClients( pStop, pVm->getVmDirUuid(), pVm->getVmUuid() );
		}

		// For now only on Mac OS we must send report if Vm suddenly
		// disconnects! On other platforms report will be send
		// after crash report monitor has detected Vm dump.
		// If crash dump has not been generated in 10 seconds,
		// we send dumpless report to the clients.
		tryToSendProblemReport(pVm);

		PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
		SmartPtr<CVmConfiguration> pVmConfig =
						CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
									pVm->getVmDirUuid(), pVm->getVmUuid(), nRetCode);
	}
}

// For now only on Mac OS we must send report if Vm suddenly
// disconnects! On other platforms report will be send
// after crash report monitor has detected Vm dump.
// If crash dump has not been generated in 10 seconds,
// we send dumpless report to the clients.
void CDspVmManager::tryToSendProblemReport( SmartPtr<CDspVm> pVm )
{

	const QString prl_vm_app( "prl_vm_app" );

	PRL_ASSERT(pVm.isValid());
	if (!pVm.isValid())
	{
		return;
	}

	if (   pVm->getVmState() == VMS_STOPPED
		|| pVm->getVmState() == VMS_STOPPING
		|| pVm->getVmState() == VMS_SUSPENDED
		|| pVm->getVmState() == VMS_SUSPENDING_SYNC)
	{
		return;
	}

	if( CDspService::instance()->isServerStopping() )
	{
		WRITE_TRACE( DBG_FATAL, "Skip problem report collecting because dispatcher is in STOPPING state now." );
		return;
	}

	// we have crash or kill vm process here
	SmartPtr<CDspClient> pUser = pVm->getVmRunner();
	QString vmUuid = pVm->getVmUuid();
	QString vmDirUuid = pVm->getVmDirUuid();

	CDspCrashReportMonitor::RepCache repCache;

	// Wait for 10 seconds for Vm crash dump.
	// If nothing was created, just send report from here.
	bool dumpCreated = CDspService::instance()->getCrashReportMonitor().
		waitForVmCrashDump( vmDirUuid, vmUuid, 10000 );
	if ( dumpCreated )
		// Great, dump was created, just return.
		return;

	CPackedProblemReport * pTmpReport = NULL;
	CPackedProblemReport::createInstance( CPackedProblemReport::DispSide, &pTmpReport );
	SmartPtr<CPackedProblemReport> pReport( pTmpReport );
	if ( !pReport )
	{
		WRITE_TRACE(DBG_FATAL, "cannot create report instance!");
		PRL_ASSERT(false);
		return;
	}

	// try get vm config
	PRL_RESULT res = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> pVmConfig =
		CDspService::instance()->getVmDirHelper().getVmConfigByUuid(vmDirUuid,
																	vmUuid,
																	res);
	if( pVmConfig )
		CDspProblemReportHelper::FormProblemReportDataForDisconnect( *pReport.getImpl(), pVmConfig );
	else
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get vm config for vm uuid %s, error %s"
			, QSTR2UTF8( pVm->getVmUuid() )
			, PRL_RESULT_TO_STRING( res ) );
	}

	// Fill problem report with report cache
	CDspProblemReportHelper::FillProblemReportData( *pReport.getImpl(), pUser, vmDirUuid, repCache );

	pReport->saveMainXml();

	QList< SmartPtr<CDspClient> > lstOldClients;
	QList< SmartPtr<CDspClient> > lstNewClients;

	CDspProblemReportHelper::getOldAndNewProblemReportClients( vmDirUuid, vmUuid, lstOldClients, lstNewClients );

	SmartPtr<IOPackage> p =
		CDspProblemReportHelper::createProblemReportEventPackage( *pReport.getImpl() );

	// Send to all clients except LIGHTWEIGHT
	foreach ( SmartPtr<CDspClient> client, lstNewClients )
	{
		if ( client->getFlags() & PCF_LIGHTWEIGHT_CLIENT )
			continue;
		client->sendPackage( p );
	}

	if ( ! lstOldClients.isEmpty() )
	{
		SmartPtr<CProblemReport> pOldReport = pReport->convertToProblemReportOldFormat();
		p = CDspProblemReportHelper::createProblemReportOldFormatEventPackage( *pOldReport.getImpl() );

		// Send to all clients except LIGHTWEIGHT
		foreach ( SmartPtr<CDspClient> client, lstOldClients )
		{
			if ( client->getFlags() & PCF_LIGHTWEIGHT_CLIENT )
				continue;
			client->sendPackage( p );
		}
	}
}

QString CDspVmManager::getVmIdByHandle(const IOSender::Handle& h) const
{
	QReadLocker l(&m_rwLock);
	if ( ! m_vms.contains(h) )
		return QString();
	return m_vms.value(h)->getVmUuid();
}

void CDspVmManager::handleToDispatcherPackage ( const IOSender::Handle& h,
												const SmartPtr<IOPackage> &p )
{
 	LOG_MESSAGE(DBG_DEBUG, "CDspVmManager::handleToDispatcherPackage() received package [%s]", p->buffers[0].getImpl());
	if (p->header.type == PVE::DspVmAuth
		|| p->header.type == PVE::DspVmRestoreState)
	{
		QReadLocker locker( &m_rwLock );
		bool bVmExists = m_vms.contains(h);
		locker.unlock();
		if (!bVmExists)
		{
			CVmEvent authPkg(UTF8_2QSTR(p->buffers[0].getImpl()));
			QString sVmUuid = authPkg.getEventIssuerId();
			CVmEventParameter *pVmDirUuidParam = authPkg.getEventParameter(EVT_PARAM_VM_DIR_UUID);
			if (!pVmDirUuidParam)
			{
				WRITE_TRACE(DBG_FATAL, "Wrong VM authorization command format [%s]", authPkg.toString().toUtf8().constData());
				CDspService::instance()->getIOServer().disconnectClient(h);
				return;
			}

			SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid(sVmUuid, pVmDirUuidParam->getParamValue());
			if (pVm.isValid())
			{
				if (p->header.type == PVE::DspVmAuth)
				{
					QWriteLocker _lock(&m_rwLock);
					m_vms[h] = pVm;
					_lock.unlock();
					pVm->handshakeWithVmProcess(h);
				}
				else
				{
					CVmEventParameter *pVmProductVerParam =
						authPkg.getEventParameter(EVT_PARAM_PRL_SERVER_INFO_PRODUCT_VERSION);
					PRL_ASSERT(pVmProductVerParam);
					if ( ! pVmProductVerParam )
					{
						WRITE_TRACE(DBG_FATAL, "Wrong VM state restoration command format [%s]",
							authPkg.toString().toUtf8().constData());
						CDspService::instance()->getIOServer().disconnectClient(h);
						return;
					}

					CVmEventParameter *pVmStateParam = authPkg.getEventParameter(EVT_PARAM_VMINFO_VM_STATE);
					PRL_ASSERT(pVmStateParam);
					if ( ! pVmStateParam )
					{
						WRITE_TRACE(DBG_FATAL, "Wrong VM state restoration command format [%s]",
							authPkg.toString().toUtf8().constData());
						CDspService::instance()->getIOServer().disconnectClient(h);
						return;
					}

					CVmEventParameter *pVmProcessIdParam = authPkg.getEventParameter(EVT_PARAM_VMINFO_VM_PROCESS_ID);
					PRL_ASSERT(pVmProcessIdParam);
					if ( ! pVmProcessIdParam )
					{
						WRITE_TRACE(DBG_FATAL, "Wrong VM state restoration command format [%s]",
							authPkg.toString().toUtf8().constData());
						CDspService::instance()->getIOServer().disconnectClient(h);
						return;
					}

					pVm->setRestoredVmProductVersion(pVmProductVerParam->getParamValue());
#ifndef _WIN_
					pVm->restoreVmProcess((VIRTUAL_MACHINE_STATE )pVmStateParam->getParamValue().toInt(),
											(Q_PID )pVmProcessIdParam->getParamValue().toULongLong());
#endif
					QWriteLocker _lock(&m_rwLock);
					m_vms[h] = pVm;
					_lock.unlock();

					pVm->handshakeWithVmProcess(h);
				}
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Non known VM with UUID '%s' tried to authorize on dispatcher", sVmUuid.toUtf8().constData());
				CDspService::instance()->getIOServer().disconnectClient(h);
			}
		}

#ifdef SENTILLION_VTHERE_PLAYER
		// Handle situation, when GUI has been disconnected
		// just before dispatcher receives Auth response from Vm.

		locker.relock();
		SmartPtr<CDspVm> pVm = m_vms[h];
		locker.unlock();

		SmartPtr<CDspClient> vmRunner = pVm->getVmRunner();

		CDspClientManager& cliManager = CDspService::instance()->getClientManager();
		if ( vmRunner && ! cliManager.getUserSession(vmRunner->getClientHandle()) )
			shutdownVmsByClient( vmRunner, false );
#endif

		return;
	}

	QReadLocker locker( &m_rwLock );
	bool bVmExists = m_vms.contains(h);

	// Check authorization
	if ( ! bVmExists ) {
		locker.unlock();

		// Non authorized
		WRITE_TRACE(DBG_FATAL, "Non authorized VM sender with handle '%s' tried to send dispatcher a package", h.toUtf8().constData());
		CDspService::instance()->getIOServer().disconnectClient(h);
		return;
	}

	SmartPtr<CDspVm> pVm = m_vms.value(h);
	locker.unlock();

	bool bNeedRoute = true;
	SmartPtr<CDspClient> r;
	switch (p->header.type)
	{
	case PVE::DspVmEventStartVNCServer:
	case PVE::DspVmEventStopVNCServer:
	case PVE::DspVmDevConnect:
	case PVE::DspVmDevDisconnect:
		r = pVm->getVmRunner();
		if (!r.isValid())
		{
			WRITE_TRACE(DBG_FATAL,"Couldn't process command %d '%s' "
				"because the CDspVm object is still incomplete"
				,p->header.type
				,PVE::DispatcherCommandToString(p->header.type));
			return;
		}
		break;
	default:
		pVm->changeVmState(p, bNeedRoute);
		if( !bNeedRoute )
			WRITE_TRACE(DBG_WARNING, "Note! Route for this package was disabled.");
	}
	switch (p->header.type)
	{
	case PVE::DspVmEventStartVNCServer:
		return pVm->startVNCServer(r, p, true, true);
	case PVE::DspVmEventStopVNCServer:
		return pVm->stopVNCServer(r, p, true, true);
	case PVE::DspVmDevConnect:
		return pVm->connectDevice(r, p);
	case PVE::DspVmDevDisconnect:
		return pVm->disconnectDevice(r, p);
	case PVE::DspWsResponse:
		handleWsResponse( pVm, p, bNeedRoute );
		break;
	case PVE::DspVmEvent:
		handleVmEvent( pVm, p, bNeedRoute );
		break;
	case PVE::DspVmBinaryEvent:
		handleVmBinaryEvent( pVm, p, bNeedRoute );
	default:
		break;
	}
	if(bNeedRoute && !CDspRouter::instance().routePackage(this, h, p))
	{
		WRITE_TRACE(DBG_FATAL,"Couldn't to route package with code %d '%s'"
			, p->header.type
			, PVE::DispatcherCommandToString(p->header.type));
	}
}

void CDspVmManager::handleFromDispatcherPackage (
    const SmartPtr<CDspHandler> &pHandler,
	const IOSender::Handle& h,
	const SmartPtr<IOPackage> &pPackage)
{
	const CDspClientManager *pClientManager = dynamic_cast<const CDspClientManager *>(pHandler.getImpl());
	if (!pClientManager)
	{
		WRITE_TRACE(DBG_FATAL, "Unknown packages handler with type '%s'"
			, (pHandler.getImpl() ? pHandler->handlerName().toUtf8().constData() : "unknown"));
		return;
	}

	SmartPtr<CDspClient> pUser = pClientManager->getUserSession(h);
	if (!pUser.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Unknown user session with handle '%s'", h.toUtf8().constData());
		return;
	}

	SmartPtr<IOPackage> p = pPackage;

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand((PVE::IDispatcherCommands)p->header.type, UTF8_2QSTR(p->buffers[0].getImpl()));
	if (!pCmd->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Wrong package received with type %d '%s'", p->header.type, PVE::DispatcherCommandToString(p->header.type));
		pUser->sendSimpleResponse( p, PRL_ERR_UNRECOGNIZED_REQUEST );
		return;
	}

	QString sVmUuid = pCmd->GetVmUuid();

	WRITE_TRACE(DBG_FATAL, "Processing command '%s' %d for vm_uuid='%s' ",
		PVE::DispatcherCommandToString(p->header.type),
		p->header.type,
		QSTR2UTF8( sVmUuid ) );

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
		pUser->sendSimpleResponse(p,PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
		return;
	}

	handleFromDispatcherPackageInternal( pUser, p );
}


void CDspVmManager::handleFromDispatcherPackageInternal (
		const SmartPtr<CDspClient> pUser,
		const SmartPtr<IOPackage> &pPackage)
{
	Command::getDispatcher()->do_(pUser, pPackage);
}

void CDspVmManager::handleFromDispatcherPackage (
    const SmartPtr<CDspHandler>&,
	const IOSender::Handle& hSender,
	const IOSender::Handle& hReceiver,
	const SmartPtr<IOPackage> &p )
{
	//TODO: CDspVmManager::handleFromDispatcherPackage
	WRITE_TRACE(DBG_FATAL, "Processing command '%s' %d",
				PVE::DispatcherCommandToString(p->header.type),
				p->header.type);

	QReadLocker locker( &m_rwLock );
	if ( ! m_vms.contains(hReceiver) )
		return;
	SmartPtr<CDspVm> pVm = m_vms.value(hReceiver);
	locker.unlock();

	if (!pVm || !p.isValid())
	{
		return;
	}

	if (p->header.type == PVE::DspCmdVmAnswer && pVm->getQuestionPacket().isValid())
	{
		SmartPtr<IOPackage> pQuestionPacket = pVm->getQuestionPacket();

		if (Uuid::toString(p->header.parentUuid) == Uuid::toString(pQuestionPacket->header.uuid))
		{
			// Prepare notification

			CVmEvent eventQuestion(UTF8_2QSTR(pQuestionPacket->buffers[0].getImpl()));
			CVmEvent eventAnswer(UTF8_2QSTR(p->buffers[0].getImpl()));
			CVmEventParameter *pParamAnswer = eventAnswer.getEventParameter( EVT_PARAM_MESSAGE_CHOICE_0 );
			if (pParamAnswer)
			{
				PRL_RESULT nAnswer = pParamAnswer->getParamValue().toInt();
				WRITE_TRACE(DBG_FATAL, "Sending answer %.8X '%s' on question %.8X '%s' to VM '%s' '%s' from user session '%s'",
							nAnswer, PRL_RESULT_TO_STRING(nAnswer),
							eventQuestion.getEventCode(), PRL_RESULT_TO_STRING(eventQuestion.getEventCode()),
							QSTR2UTF8(pVm->getVmName()), QSTR2UTF8(pVm->getVmUuid()),
							QSTR2UTF8(hSender));
			}
			else
				WRITE_TRACE(DBG_FATAL, "Answer package on question %.8X '%s' received from user session '%s' doesn't contain answer choice!!!",\
								eventQuestion.getEventCode(), PRL_RESULT_TO_STRING(eventQuestion.getEventCode()),\
								QSTR2UTF8(hSender));

			CVmEvent eventNotification;

			eventNotification.setEventType(PET_DSP_EVT_ANSWER_TO_VM_WAS_DONE);
			eventNotification.setEventCode(eventQuestion.getEventCode());
			eventNotification.setEventIssuerType(PIE_DISPATCHER);
			eventNotification.setEventIssuerId(eventQuestion.getEventIssuerId());	// VM uuid

			SmartPtr<IOService::IOPackage> pNotification
				= DispatcherPackage::createInstance(PVE::DspVmEvent, eventNotification);

			// Get clients with Read + Execute permissions

			QHash< IOSender::Handle, SmartPtr<CDspClient> > hashClients
				= CDspService::instance()->getClientManager()
					.getSessionListByVm(pVm->getVmDirUuid(), pVm->getVmUuid(),
					(CDspAccessManager::VmAccessRights::arCanRead | CDspAccessManager::VmAccessRights::arCanExecute));

			// Send notification to clients

			hashClients.remove(hSender);
			QList< SmartPtr<CDspClient> > lstClients = hashClients.values();
			CDspService::instance()->getClientManager().sendPackageToClientList(pNotification, lstClients);

			// Delete answered question
			pVm->setQuestionPacket(SmartPtr<IOPackage>());

			//Potentially here can incoming just answers on VMs questions. So just forward it to VM
			CDspService::instance()->getIOServer().sendPackage(hReceiver, p);
		}
	}
}

void CDspVmManager::handleClientStateChanged ( const IOSender::Handle&,
											   IOSender::State  )
{
}

void CDspVmManager::handleDetachClient (
    const IOSender::Handle&,
	const IOCommunication::DetachedClient& )
{
	// Never should be called for this handler
	PRL_ASSERT(0);
}

void CDspVmManager::handleVmEvent( SmartPtr<CDspVm> pVm,
					const SmartPtr<IOPackage>& pOrigPackage,
					bool & bNeedToRoute)
{
	SmartPtr<IOPackage> p( pOrigPackage );
	PRL_ASSERT( pVm );
	if( !pVm )
		return;

	CVmEvent _evt(UTF8_2QSTR(p->buffers[0].getImpl()));
	PRL_EVENT_TYPE
		evType = (PRL_EVENT_TYPE )_evt.getEventType();

	switch(evType)
	{

	case PET_DSP_EVT_VM_QUESTION:
		WRITE_TRACE(DBG_FATAL, "Received question %.8X '%s' from VM '%s' '%s'",\
						_evt.getEventCode(), PRL_RESULT_TO_STRING(_evt.getEventCode()),
						QSTR2UTF8(pVm->getVmName()), QSTR2UTF8(pVm->getVmUuid()));

		if (_evt.getRespRequired() == PVE::EventRespRequired)
		{
			QHash< IOSender::Handle, SmartPtr<CDspClient> > hashSessions =
				CDspService::instance()->getClientManager().getSessionListByVm(
							pVm->getVmDirUuid(),
							pVm->getVmUuid(),
							( CDspAccessManager::VmAccessRights::arCanRead |
							  CDspAccessManager::VmAccessRights::arCanExecute ));
			if (   bNeedToRoute
			  	&& !CQuestionHelper::isInternalQuestion( _evt.getEventCode() )
				&& !hashSessions.isEmpty()
				&& haveInteractiveSessions( hashSessions.values() )
				&& !haveNonInteractiveSessionsWithRequestToVm( pVm, hashSessions.values() )
				&& !defaultAnswersMechActivated( pVm ) )
			{
				WRITE_TRACE(DBG_FATAL, "Sending question to permitted clients sessions");
				pVm->setQuestionPacket(p);
				sendPackageFromVmToSessions( pVm, p, hashSessions.values(), true);
			}
			else
			{
				sendDefaultAnswerToVm(pVm, p);
			}
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Sent question to permitted clients sessions as not response required");
			if(bNeedToRoute)
				sendPackageFromVmToPermittedUsers( pVm, p );
		}

		break;


	case PET_DSP_EVT_VM_DEV_STATE_CHANGED:
		updateUsbDeviceState( pVm, _evt );
		if(bNeedToRoute)
			sendPackageFromVmToPermittedUsers( pVm, p );
		break;

	case PET_VM_DEFAULT_SOUND_RECONNECTED:
		SendEchoEventToVm( pVm, _evt );
		break;

	case PET_VM_EVT_VM_CONFIG_CHANGED:
		{
			// One of the configuration parameter is changed
			LOG_MESSAGE( DBG_DEBUG, "DBG: PET_VM_EVT_VM_CONFIG_CHANGED event catched.");

			bool ret = CDspService::instance()->getVmDirHelper()
				.atomicEditVmConfigByVm( pVm->getVmDirUuid(), pVm->getVmUuid(), _evt, pVm->getVmRunner() );

			if( ret )
			{
				// notify every client of configuration changes.
				CVmEvent event(PET_DSP_EVT_VM_CONFIG_CHANGED, pVm->getVmUuid(), PIE_DISPATCHER);

				SmartPtr<IOPackage> pkg = DispatcherPackage::createInstance(PVE::DspVmEvent, event );
				sendPackageFromVmToPermittedUsers( pVm, pkg );
			}
		}
		break;

	case PET_DSP_EVT_VM_PIS_NOTIFICATION_ASK_TO_INSTALL:
		WRITE_TRACE( DBG_INFO,
			"PET_DSP_EVT_VM_PIS_NOTIFICATION_ASK_TO_INSTALL is received from VM." );
		sendPackageFromVmToPermittedUsers( pVm, p );
		break;

	case PET_DSP_EVT_VM_MIGRATE_WAIT_REMOUNT:
	case PET_DSP_EVT_VM_MIGRATE_FINISHED_DISP:
	case PET_DSP_EVT_VM_MIGRATE_CANCELLED_DISP:
		CDspService::instance()->handleVmMigrateEvent( pVm, p );
		break;

	case PET_VM_EVT_TIS_BACKUP_WRITE:
		{
			QString sTisBackup;
			CVmEventParameter *evParam = _evt.getEventParameter( EVT_PARAM_WRITING_FILE_STRING );

			PRL_ASSERT(evParam);
			if (!evParam)
				break;

			sTisBackup = evParam->getParamValue();

			QString sVmHome = CDspService::instance()->getVmDirManager().getVmHomeByUuid( pVm->getVmIdent() );
			if( sVmHome.isEmpty() )
			{
				WRITE_TRACE(DBG_FATAL, "Unable to get VmHome for vm %s", QSTR2UTF8( pVm->getVmUuid() ) );
			}
			else
			{
				QString sVmHomeDir = CFileHelper::GetFileRoot( sVmHome );
				CDspVmInfoDatabase::writeTisBackup( CFileHelper::GetFileRoot( sVmHome ), sTisBackup );
			}
		}
		break;
	case PET_DSP_EVT_VM_CONFIG_APPLY_FINISHED:
		pVm->wakeupApplyConfigWaiters();
		break;

	case PET_DSP_EVT_VM_IO_CLIENT_STATISTICTS:
		{
			bNeedToRoute = false;
			break;
		}

	case PET_DSP_EVT_VM_SET_HOST_TIME:
		{
			CVmEventParameter *evParam1 = _evt.getEventParameter( EVT_PARAM_VM_SETTIME_SECONDS );
			CVmEventParameter *evParam2 = _evt.getEventParameter( EVT_PARAM_VM_SETTIME_TIMEZONE );

			int tzIndex;
			int timeSeconds;

			PRL_ASSERT( evParam1 );
			PRL_ASSERT( evParam2 );

			if ( !evParam1 || !evParam2 )
				break;

			bool bOk;
			timeSeconds = evParam1->getParamValue().toInt( &bOk );
			if ( !bOk )
			{
				WRITE_TRACE( DBG_WARNING, "failed to get time from host command" );
				break;
			}
			tzIndex = evParam2->getParamValue().toInt( &bOk );
			if ( !bOk )
			{
				WRITE_TRACE( DBG_WARNING, "failed to get time zone index from host command" );
				break;
			}
			changeHostTimeSettings( timeSeconds, tzIndex );

		}
		break;

	default:
		LOG_MESSAGE( DBG_DEBUG, "Event 0x%x received from VM", evType);
		if(bNeedToRoute)
			sendPackageFromVmToPermittedUsers( pVm, p );
	}

	bNeedToRoute = false;
}

void CDspVmManager::handleWsResponse( SmartPtr<CDspVm> pVm,
							  const SmartPtr<IOPackage>& p,
							  bool & bNeedToRoute)
{
	PRL_ASSERT( pVm );
	if( !pVm )
		return;

	SmartPtr<IOPackage> newPackage = p;

	if( bNeedToRoute && !CDspRouter::instance().routePackage( this, pVm->getVmConnectionHandle() , newPackage ))
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to route package with code %d '%s'"
			, newPackage->header.type
			, PVE::DispatcherCommandToString(newPackage->header.type)
			);
	}
	else if( bNeedToRoute )
		WRITE_TRACE(DBG_FATAL, "Response package was successfully routed to client" );

	bNeedToRoute = false;
}

QList< IOSendJob::Handle > CDspVmManager::sendPackageToAllVMs( const SmartPtr<IOPackage>& p )
{
	QReadLocker _lock(&m_rwLock);
	QList<IOSendJob::Handle> jobs;
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	for ( int i=0 ; i != lstVms.size(); i++)
	{
		IOSendJob::Handle job = lstVms[i]->sendPackageToVm(p);
		jobs.append( job );
	}

	return jobs;
}

bool CDspVmManager::hasAnyRunningVms() const
{
	QList<CVmIdent> lstIdents = CDspService::instance()->getVmDirManager().getAllVmIdList();

	foreach ( CVmIdent vmIdent, lstIdents )
	{
		VIRTUAL_MACHINE_STATE state = CDspVm::getVmState( vmIdent );
		switch( state )
		{
			case VMS_STOPPED:
			case VMS_SUSPENDED:
				 continue;
			default:
				 return true;
		}
	}

	return (false);
}

void CDspVmManager::shutdownVms ( bool useAutoStopOpt /* = true */ ) const
{
	QReadLocker locker( &m_rwLock );
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	locker.unlock();
	shutdownVms( lstVms, useAutoStopOpt );
}

void CDspVmManager::shutdownVmsByClient ( const SmartPtr<CDspClient>& pUser,
										  bool useAutoStopOpt ) const
{
	// Get vms of this user
	QList<SmartPtr<CDspVm> > lstVms = getVmsByClient( pUser );
	// Shutdown vms
	shutdownVms( lstVms, useAutoStopOpt );
}

/* Check is suspend to PRAM action have to be performed.
 * Suspend to PRAM could be initiated by vzreboot feature
 *    /sys/kernel/kexec_loaded = 1
 * or by request
 *    PSHF_SUSPEND_VM_TO_PRAM
 */
bool CDspVmManager::checkFastReboot(void) const
{
	bool bFastReboot = false;

#ifdef _LIN_
	if (!CDspService::isServerModePSBM())
		return false;

	if (CDspService::instance()->getDispConfigGuard()
			.getDispCommonPrefs()->getFastRebootPreferences()->isFastReboot() &&
			QFile::exists("/sys/kernel/pram"))
	{

		/* Fastboot enabled by PSHF_SUSPEND_VM_TO_PRAM request */
		bFastReboot = true;
	}
	else if (HostUtils::isFastRebootNodeAllowed())
	{
		/* Fastboot enabled by kexec */
		bFastReboot = true;
		//set flag FastReboot in dispatcher config
		CDspService::instance()->getDispConfigGuard()
			.getDispCommonPrefs()->getFastRebootPreferences()->setFastReboot(true);
		CDspService::instance()->getDispConfigGuard().saveConfig();
	}
#endif
	return bFastReboot;
}

void CDspVmManager::shutdownVms ( const QList< SmartPtr<CDspVm> >& lstVms,
								  bool useAutoStopOpt ) const
{
	WRITE_TRACE(DBG_FATAL, "Stop of VMs. Please wait...");

	bool bFastReboot = checkFastReboot();

	for(int i = 0; i < lstVms.size(); i++)
	{
		SmartPtr<CDspVm> pVm = lstVms[i];
		if (!pVm)
			continue;

		VIRTUAL_MACHINE_STATE state = pVm->getVmState();
		switch (state)
		{
			case VMS_STOPPED:
			case VMS_STOPPING:
			case VMS_SUSPENDED:
			case VMS_SUSPENDING:
			case VMS_SUSPENDING_SYNC:
				continue;
			case VMS_MIGRATING:
				{
					WRITE_TRACE(DBG_FATAL, "cleanup migrating VM: %s, %s"
						, QSTR2UTF8( pVm->getVmName() )
						, QSTR2UTF8( pVm->getVmUuid() )
						);

					SmartPtr<CVmEvent> event(new CVmEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED_DISP,
										pVm->getVmUuid(),
										PIE_VIRTUAL_MACHINE));
					const SmartPtr<IOPackage> p =
						DispatcherPackage::createInstance(PVE::DspVmEvent, event->toString());
					pVm->changeVmState(p);
					continue;
				}
			default:
				break;
		}

		PRL_RESULT res = PRL_ERR_SUCCESS;
		SmartPtr<CVmConfiguration> pVmConfig =
			CDspService::instance()->getVmDirHelper().getVmConfigByUuid (pVm->getVmDirUuid(),
				pVm->getVmUuid(),
				res);

		if( ! pVmConfig )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get vm config for vm uuid %s, error %s"
				, QSTR2UTF8( pVm->getVmUuid() )
				, PRL_RESULT_TO_STRING( res ) );
			continue;
		}

		PRL_VM_AUTOSTOP_OPTION type = pVmConfig->getVmSettings()->getShutdown()->getAutoStop();
		if ( bFastReboot )
		{
			type = PAO_VM_SUSPEND;
			pVm->storeFastRebootState(true);
			WRITE_TRACE(DBG_FATAL, "Set suspend to PRAM mode for VM: %s, %s"
				, QSTR2UTF8( pVm->getVmName() )
				, QSTR2UTF8( pVm->getVmUuid() ) );
		}
		else if ( CDspService::instance()->getDispConfigGuard()
				.getDispCommonPrefs()->getFastRebootPreferences()->isFastReboot() )
		{
			/* Dispatcher in shutdown state with PSHF_SUSPEND_VM_TO_PRAM flags,
			 * If PRAM is not supported by kernel use suspend to disk.
			 */
			type = PAO_VM_SUSPEND;
			/* Mark Vm to be resumed on Dispatcher start */
			pVm->storeFastRebootState(true);
			WRITE_TRACE(DBG_FATAL, "Set suspend to disk mode for VM: %s, %s"
				, QSTR2UTF8( pVm->getVmName() )
				, QSTR2UTF8( pVm->getVmUuid() ) );
		}
		else if ( ! useAutoStopOpt )
		{
			// Force stop
			type = PAO_VM_STOP;
		}

		SmartPtr<CDspClient> pUser = pVm->getVmRunner();
		if ( ! pUser.isValid() )
		{
			pUser = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
			pUser->getAuthHelper().AuthUserBySelfProcessOwner();
		}

		pUser->setVmDirectoryUuid(pVm->getVmDirUuid());

		switch(type)
		{
		case PAO_VM_SUSPEND:
		{
			WRITE_TRACE(DBG_FATAL, "suspend VM: %s, %s"
				, QSTR2UTF8( pVm->getVmName() )
				, QSTR2UTF8( pVm->getVmUuid())
				);

			SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdVmSuspend );
			CDspVm::SuspendMode nMode = CDspVm::SM_STOP_ON_FAILURE;

			if (bFastReboot)
			{
				QString path = pVm->prepareFastReboot(true);

				if (!path.isEmpty())
				{
					nMode = CDspVm::SM_SUSPEND_ON_FAILURE;
					CProtoCommandPtr pCmd =
						CProtoSerializer::CreateProtoBasicVmCommand(PVE::DspCmdVmSuspend,
						pVm->getVmUuid());
					pCmd->GetCommand()->addEventParameter(new CVmEventParameter(PVE::String
						, path
						, EVT_PARAM_VM_DUMP_META_FILE));

					p = DispatcherPackage::createInstance( PVE::DspCmdVmSuspend,  pCmd );
					WRITE_TRACE(DBG_FATAL, "Fast reboot: suspend VM for fast reboot: %s, %s to %s"
							, QSTR2UTF8( pVm->getVmName() )
							, QSTR2UTF8( pVm->getVmUuid())
							, QSTR2UTF8( path )
							);
				}
				else
					//error
					WRITE_TRACE(DBG_FATAL, "Failed to suspend VM for fast reboot: %s, %s"
							, QSTR2UTF8( pVm->getVmName() )
							, QSTR2UTF8( pVm->getVmUuid())
							);
			}
			pVm->suspend(pUser, p, true, nMode);
			break;
		}
		case PAO_VM_STOP:
		case PAO_VM_SHUTDOWN:
		{
			WRITE_TRACE(DBG_FATAL, "Try to stop Vm %s (%s) by %s command."
				, QSTR2UTF8( pVm->getVmName() )
				, QSTR2UTF8( pVm->getVmUuid() )
				, type == PAO_VM_STOP ? "STOP" : "ACPI-SHUTDOWN"
				);


			PRL_UINT32 nStopMode = (type == PAO_VM_SHUTDOWN) ? PSM_SHUTDOWN : PSM_KILL;

			CProtoCommandPtr pCmd =
				CProtoSerializer::CreateProtoVmCommandStop(pVm->getVmUuid(), nStopMode, PSF_FORCE);
			const SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
			pVm->stop(pUser, p, nStopMode, true);
			break;
		}
		default:
			WRITE_TRACE(DBG_FATAL, "Unknown stopping action %d !", type );
		}//switch
	}//for
}

CDspLockedPointer< CDspVmSuspendHelper >
CDspVmManager::getSuspendHelper()
{
	return CDspLockedPointer< CDspVmSuspendHelper > (&m_suspendHelperMutex, &m_suspendHelper );
}

void CDspVmManager::shutdownVmsFinal() const
{
	if (!hasAnyRunningVms())
	{
		return;
	}

	WRITE_TRACE(DBG_FATAL, "All running VMs would be killed!");

	QReadLocker locker( &m_rwLock );
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	locker.unlock();

	for(int i = 0; i < lstVms.size(); i++)
	{
		SmartPtr<CDspVm> pVm = lstVms[i];

		QString qsVmName = "unknown VM name";
		QString qsVmUuid = pVm->getVmUuid();

		CDspLockedPointer<CVmDirectoryItem> pDirItem
			= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(pVm->getVmDirUuid(), qsVmUuid);
		if (pDirItem.isValid())
		{
			qsVmName = pDirItem->getVmName();
		}
		QString qsVmInfo = QString("[VM name: %1, VM uuid: %2]").arg(qsVmName, qsVmUuid);

#ifndef _WIN_
		if (CDspService::instance()->isServerMode())
		{
			WRITE_TRACE(DBG_FATAL, "Virtual machine %s will be force stopped!", QSTR2UTF8(qsVmInfo));

			CProtoCommandPtr pCmd =
				CProtoSerializer::CreateProtoVmCommandStop(pVm->getVmUuid(), PSM_KILL, PSF_FORCE);
			const SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );

			pVm->sendPackageToVm(p);
		}
		else
#endif
			WRITE_TRACE(DBG_FATAL, "Virtual machine %s would be killed!", QSTR2UTF8(qsVmInfo));
	}
}

void CDspVmManager::sendDefaultAnswerToVm(SmartPtr<CDspVm>& pVm, const SmartPtr<IOPackage>& pQuestionPacket) const
{
	if (pVm.isValid() && pQuestionPacket.isValid())
		Command::Details::Assistant::sendDefaultAnswer(*pVm, *pQuestionPacket);
}

QList< SmartPtr<IOPackage> > CDspVmManager::getVmQuestions(const SmartPtr<CDspClient>& /*pClient*/) const
{
	QReadLocker locker( &m_rwLock );
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	locker.unlock();

	QList< SmartPtr<IOPackage> > lstQuestions;

	for(int i = 0; i < lstVms.size(); i++)
	{
		SmartPtr<CDspVm> pVm = lstVms[i];
		if (!pVm)
		{
			continue;
		}

		if (pVm->getQuestionPacket().isValid())
		{
			lstQuestions += pVm->getQuestionPacket();
		}
	}

	return lstQuestions;
}

void CDspVmManager::checkToSendDefaultAnswer() const
{
	QReadLocker locker( &m_rwLock );
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	locker.unlock();

	for(int i = 0; i < lstVms.size(); i++)
	{
		SmartPtr<CDspVm> pVm = lstVms[i];
		if (!pVm)
		{
			continue;
		}

		if ( CDspService::instance()->getClientManager()
			 .getSessionListByVm(pVm->getVmDirUuid(), pVm->getVmUuid(),
				(CDspAccessManager::VmAccessRights::arCanRead | CDspAccessManager::VmAccessRights::arCanExecute))
			 .isEmpty()
			&& pVm->getQuestionPacket().isValid()
			)
		{
			sendDefaultAnswerToVm(pVm, pVm->getQuestionPacket());

			// Delete answered question
			pVm->setQuestionPacket(SmartPtr<IOPackage>());
		}
	}
}

void CDspVmManager::handleVmProblemReportEvent( SmartPtr<CDspVm> pVm,
										const SmartPtr<IOPackage>& p,
										bool & bNeedToRoute,
										const SmartPtr<CVmEvent> &pEvent)
{
	PRL_ASSERT( pVm );
	if( !pVm )
		return;


	CVmEventParameter * lpcParam = pEvent->getEventParameter(EVT_PARAM_VM_PROBLEM_REPORT );

	if (lpcParam)
	{
		QString qsPerfCountersInfo = CDspService::instance()
			->getRequestToVmHandler().getPerfCountersInfo(Uuid::toString(p->header.parentUuid));

		// mark request completed
		CDspService::instance()->getRequestToVmHandler().markRequestComplete( p );

		CPackedProblemReport * pTmpReport = NULL;
		CPackedProblemReport::createInstance( CPackedProblemReport::DispSide,
												&pTmpReport );
		SmartPtr<CPackedProblemReport> pReport( pTmpReport );
		if ( !pReport )
		{
			WRITE_TRACE(DBG_FATAL, "cannot create report instance!");
			PRL_ASSERT(false);
			return;
		}

		if( PRL_FAILED( pReport->fromBaseReport( lpcParam->getParamValue() ) ) )
		{
			WRITE_TRACE(DBG_FATAL, "cannot parse from base report instance!");
			PRL_ASSERT(false);
			return;
		}
		// get screenshot path from vm
		QString strPath;
		if( !pReport->getUserDefinedData()->getScreenShots()->m_lstScreenShot.isEmpty() )
		{
			strPath = pReport->getUserDefinedData()->getScreenShots()->m_lstScreenShot[0]->getName();
			pReport->getUserDefinedData()->getScreenShots()->ClearLists();

			CAuthHelperImpersonateWrapperPtr pImpersonate;
			if( pVm->getVmRunner() )
				pImpersonate = CAuthHelperImpersonateWrapper::create( &pVm->getVmRunner()->getAuthHelper() );

			PRL_RESULT res;
			SmartPtr<CVmConfiguration>
				pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid( pVm->getVmIdent(), res );

			if( QFile::exists( strPath )
				&& CDspService::instance()->getDispConfigGuard()
					.getDispCommonPrefs()->getProblemReportPreferences()->isAllowAttachScreenshots() )
			{
				QFile::remove( strPath );
			}
		}
		if ( PRL_FAILED( pReport->m_uiRcInit ) )
		{
			WRITE_TRACE(DBG_FATAL, "cannot parse report instance!");
			PRL_ASSERT(false);
			return;
		}

		QString sVmHomeDir;
		{
			CDspLockedPointer< CVmDirectoryItem >
				pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				pVm->getVmDirUuid(), pVm->getVmUuid() );

			if( !pVmDirItem )
			{
				WRITE_TRACE(DBG_FATAL, "Unable to get dirItem for vm %s", QSTR2UTF8( pVm->getVmUuid() ) );
				return;
			}

			sVmHomeDir = CFileHelper::GetFileRoot( pVmDirItem->getVmHome() );
		}

		SmartPtr<CDspClient> pUser = pVm->getVmRunner();
		QString strVmDirUuid;
		if ( pUser.isValid() )
			strVmDirUuid = pUser->getVmDirectoryUuid();

		pReport->setPerformanceCounters( qsPerfCountersInfo );

		CDspProblemReportHelper::FillProblemReportData( *pReport.getImpl(), pUser, strVmDirUuid );
		//Add latest VM memory dump to problem report
		//https://jira.sw.ru:9443/browse/PDFM-841
		CDspProblemReportHelper::AddVmMemoryDump( *pReport.getImpl(), pUser, sVmHomeDir );

		pReport->saveMainXml();
		CVmEventParameter * lpcParam = pEvent->getEventParameter( EVT_PARAM_VM_PROBLEM_REPORT_VERSION );
		CProblemReportUtils::ReportVersion version = CProblemReportUtils::xmlBaseVersion;

		if ( lpcParam )
			version = (CProblemReportUtils::ReportVersion)lpcParam->getParamValue().toInt();

		SmartPtr<IOPackage> newPackage;
		if( PRT_AUTOMATIC_VM_GENERATED_REPORT == pReport->getReportType() ||
		    PRT_AUTOMATIC_GUEST_GENERATED_REPORT == pReport->getReportType() )
			newPackage =
				CDspProblemReportHelper::createProblemReportEventPackage( *pReport.getImpl(), version );
		else
			newPackage =
				CDspProblemReportHelper::createProblemReportEventPackage( *pReport.getImpl(), version, p , true );

		if(!bNeedToRoute)
			WRITE_TRACE(DBG_WARNING, " problem report event package won't send to client");
		else
		{
			WRITE_TRACE(DBG_FATAL, " problem report event package was redirected by dispatcher to clients" );

			if (pVm)
			{
				QList< SmartPtr<CDspClient> > lstOldClients;
				QList< SmartPtr<CDspClient> > lstNewClients;

				CDspProblemReportHelper::getOldAndNewProblemReportClients(pVm->getVmDirUuid(),
																		  pVm->getVmUuid(),
																		  lstOldClients,
																		  lstNewClients,
																		  p);

				// Send to all clients except LIGHTWEIGHT
				foreach ( SmartPtr<CDspClient> client, lstNewClients )
				{
					if ( client->getFlags() & PCF_LIGHTWEIGHT_CLIENT )
						continue;
					client->sendPackage( newPackage );
				}

				if ( ! lstOldClients.isEmpty() )
				{
					SmartPtr<CProblemReport> pOldReport = pReport->convertToProblemReportOldFormat();
					if( PRT_AUTOMATIC_VM_GENERATED_REPORT == pReport->getReportType() ||
						PRT_AUTOMATIC_GUEST_GENERATED_REPORT == pReport->getReportType() )
						newPackage =
							CDspProblemReportHelper::createProblemReportOldFormatEventPackage(
																			*pOldReport.getImpl() );
					else
						newPackage =
							CDspProblemReportHelper::createProblemReportOldFormatEventPackage(
																			*pOldReport.getImpl(), p, true );

					// Send to all clients except LIGHTWEIGHT
					foreach ( SmartPtr<CDspClient> client, lstOldClients )
					{
						if ( client->getFlags() & PCF_LIGHTWEIGHT_CLIENT )
							continue;
						client->sendPackage( newPackage );
					}
				}
			}
		}

		bNeedToRoute = false;
	} //if (lpcParam) //EVT_PARAM_VM_PROBLEM_REPORT

}

void CDspVmManager::handleVmBinaryEvent( SmartPtr<CDspVm> pVm,
								 const SmartPtr<IOPackage>& p,
								 bool & bNeedToRoute)
{
	PRL_ASSERT( pVm );
	if( !pVm )
		return;

	PRL_ASSERT(p->header.buffersNumber == 1);
	QBuffer _buffer;
	_buffer.setData(p->buffers[0].getImpl(), p->data[0].bufferSize);
	bool bRes = _buffer.open(QIODevice::ReadOnly);
	PRL_ASSERT(bRes);
	if (!bRes)
	{
		WRITE_TRACE(DBG_FATAL, "Fatal error - couldn't to open binary event buffer for read");
		return;
	}
	QDataStream _data_stream(&_buffer);
	_data_stream.setVersion(QDataStream::Qt_4_0);
	SmartPtr<CVmEvent> pEvent(new CVmEvent);
	pEvent->Deserialize(_data_stream);

	if((PRL_EVENT_TYPE)pEvent->getEventType() == PET_DSP_EVT_VM_PROBLEM_REPORT_CREATED)
	{
		handleVmProblemReportEvent( pVm, p, bNeedToRoute, pEvent );
	}
	else if( bNeedToRoute )
	{
		sendPackageFromVmToPermittedUsers( pVm, p );
		bNeedToRoute = false;
	}
}

void CDspVmManager::sendPackageFromVmToSessions( SmartPtr<CDspVm>  /*pVm*/, const SmartPtr<IOPackage> &p,
	QList< SmartPtr<CDspClient> > lstClients, bool bSkipNonInteractive)
{
	if (bSkipNonInteractive)
	{
		QList< SmartPtr<CDspClient> >::iterator it = lstClients.begin();
		while(it != lstClients.end())
		{
			if ((*it)->isNonInteractive())
				it = lstClients.erase(it);
			else
				it++;
		}
	}

	// Send notification to clients
	CDspService::instance()->getClientManager().sendPackageToClientList(p, lstClients);
}

void CDspVmManager::sendPackageFromVmToPermittedUsers( SmartPtr<CDspVm>  pVm,
													 const SmartPtr<IOPackage> &p,
													 int accessRights)
{
	PRL_ASSERT( pVm );
	if( !pVm )
		return;

	if ((accessRights & CDspAccessManager::VmAccessRights::arCanRead) == 0)
		return;

	// Get permitted VM clients

	QList< SmartPtr<CDspClient> >
		lstClients = CDspService::instance()->getClientManager()
			.getSessionListByVm( pVm->getVmDirUuid(), pVm->getVmUuid(), accessRights
						).values();

	sendPackageFromVmToSessions(pVm, p, lstClients, false);
}

bool CDspVmManager::haveInteractiveSessions(const QList< SmartPtr<CDspClient> >& lstSessions) const
{
	foreach( SmartPtr<CDspClient> pSession, lstSessions )
	{
		if ( ! pSession->isNonInteractive() )
			return true;
	}
	return false;
}

bool CDspVmManager::haveNonInteractiveSessionsWithRequestToVm(const SmartPtr<CDspVm>& pVm,
															  const QList< SmartPtr<CDspClient> >& lstSessions) const
{
	foreach( SmartPtr<CDspClient> pSession, lstSessions )
	{
		if (   pSession->isNonInteractive()
			&& pVm->hasUnansweredRequestForSession(pSession->getClientHandle()))
			return true;
	}
	return false;
}

bool CDspVmManager::defaultAnswersMechActivated(const SmartPtr<CDspVm>& pVm) const
{
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	SmartPtr<CVmConfiguration> pVmConfig =
					CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
								pVm->getVmDirUuid(), pVm->getVmUuid(), nRetCode);
	if ( pVmConfig && PRL_SUCCEEDED(nRetCode) )
		return ( pVmConfig->getVmSettings()->getVmRuntimeOptions()->isUseDefaultAnswers() );

	return ( false );
}

void CDspVmManager::changeLogLevelForActiveVMs( SmartPtr<CDspClient> pUser, bool bVerboseLogEnabled ) const
{
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateProtoCommandWithOneStrParam( PVE::DspCmdVmChangeLogLevel, QString( "%1" ).arg( bVerboseLogEnabled ) );
	SmartPtr<IOService::IOPackage> pkg =
		DispatcherPackage::createInstance(PVE::DspCmdVmChangeLogLevel, pCmd );

	QReadLocker locker( &m_rwLock );
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	locker.unlock();

	foreach( SmartPtr<CDspVm> pVm, lstVms )
		pVm->changeLogLevel( pUser, pkg );
}

QList< SmartPtr<CDspVm> > CDspVmManager::getVmsByClient (
    const SmartPtr<CDspClient>& client ) const
{
	QReadLocker _lock(&m_rwLock);
	QList<SmartPtr<CDspVm> > vms;
	QHash< IOSender::Handle, SmartPtr<CDspVm> >::ConstIterator it = m_vms.begin();
	for (; it != m_vms.end(); ++it)
	{
		if ( it.value()->getVmRunner() == client )
			vms.append( it.value() );
	}

	return vms;
}

QList< SmartPtr<CDspVm> > CDspVmManager::getAllRunningVms() const
{
	QReadLocker _lock(&m_rwLock);
	return m_vms.values();
}

void CDspVmManager::syncVMsUptime()
{
	WRITE_TRACE(DBG_FATAL, "Synchronizing VMs uptime values");

	QReadLocker locker( &m_rwLock );
	QList< SmartPtr<CDspVm> > lstVms = m_vms.values();
	locker.unlock();

	foreach(SmartPtr<CDspVm> pVm, lstVms)
	{
		if (!pVm)
			continue;

		VIRTUAL_MACHINE_STATE state = pVm->getVmState();
		switch (state)
		{
			case VMS_STOPPED:
			case VMS_STOPPING:
			case VMS_SUSPENDED:
			case VMS_SUSPENDING:
				continue;
			default:
				break;
		}
		pVm->updateVmUptime();
	}

	WRITE_TRACE(DBG_FATAL, "Synchronization of VMs uptime was completed");
}


void CDspVmManager::changeHostTimeSettings( int timeSeconds, int tzIndex )
{
	Q_UNUSED( timeSeconds );
	Q_UNUSED( tzIndex );
}

void CDspVmManager::changeTimezone( int tzIndex )
{
	Q_UNUSED( tzIndex );
}

/*****************************************************************************/
