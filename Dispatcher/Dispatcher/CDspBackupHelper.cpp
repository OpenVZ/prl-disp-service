///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspBackupHelper.cpp
///
/// Common backup helper class implementation
///
/// @author krasnov
/// @owner sergeym
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

#include "CDspBackupHelper.h"
#include "CDspService.h"
#include "CDspClientManager.h"

#include "prlcommon/Std/PrlAssert.h"
#include "prlcommon/PrlCommonUtilsBase/SysError.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "Tasks/Task_CreateVmBackup.h"
#include "Tasks/Task_RestoreVmBackup.h"
#include "Tasks/Task_RemoveVmBackup.h"
#include "Tasks/Task_GetBackupTree.h"
#include <boost/functional/factory.hpp>

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Parser

template<class T>
struct Parser;

template<>
struct Parser<CDspClient>: std::unary_function<SmartPtr<IOPackage>, CProtoCommandPtr>
{
	static result_type do_(const argument_type& package_)
	{
		return CProtoSerializer::ParseCommand(package_);
	}
};

template<>
struct Parser<CDspDispConnection>:
	std::unary_function<SmartPtr<IOPackage>, CDispToDispCommandPtr>
{
	static result_type do_(const argument_type& package_)
	{
		return CDispToDispProtoSerializer::ParseCommand(package_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Launch

struct Launch
{
	explicit Launch(Backup::Activity::Service& service_): m_service(&service_)
	{
	}

	CDspTaskHelper* operator()(SmartPtr<CDspClient> actor_, CProtoCommandPtr command_,
		const SmartPtr<IOPackage>& package_);

private:
	Backup::Activity::Service* m_service;
};

CDspTaskHelper* Launch::operator()(SmartPtr<CDspClient> actor_, CProtoCommandPtr command_,
	const SmartPtr<IOPackage>& package_)
{
	bool v = false;
	CProtoCreateVmBackupCommand* b = CProtoSerializer::
		CastToProtoCommand<CProtoCreateVmBackupCommand>(command_);
	QString u = b->GetVmUuid();
	PRL_RESULT e = CDspService::instance()->getAccessManager().checkAccess(
			actor_, PVE::DspCmdCreateVmBackup, u, &v);
	if (PRL_SUCCEEDED(e))
	{
		return new Task_CreateVmBackupSource(actor_, command_,
			package_, *m_service);
	}
	WRITE_TRACE(DBG_FATAL,
		"Access check failed for user {%s} when accessing VM {%s}. Reason: %#x (%s)",
		QSTR2UTF8(actor_->getClientHandle()), QSTR2UTF8(u),
		e, PRL_RESULT_TO_STRING(e));

	CVmEvent event;
	event.setEventCode(PRL_ERR_BACKUP_ACCESS_TO_VM_DENIED);
	event.addEventParameter(new CVmEventParameter(PVE::String, u,
				EVT_PARAM_MESSAGE_PARAM_0));
	event.addEventParameter(new CVmEventParameter(PVE::String,
				PRL_RESULT_TO_STRING(e), EVT_PARAM_MESSAGE_PARAM_1));
	actor_->sendResponseError(event, package_);
	return NULL;
}

} // namespace

namespace Backup
{
namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Launcher

void Launcher::startCreateCtBackupSourceTask(SmartPtr<CDspClient> actor_,
		const SmartPtr<IOPackage>& package_) const
{
	return launch(actor_, boost::bind(boost::factory<Task_CreateCtBackupSource* >(),
		_1, _2, _3, boost::ref(*m_service)), package_);
}

void Launcher::startCreateVmBackupSourceTask(SmartPtr<CDspClient> actor_,
		const SmartPtr<IOPackage>& package_) const
{
	return launch(actor_, Launch(*m_service), package_);
}

void Launcher::startRestoreVmBackupTargetTask(
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg) const
{
	launch(pUserSession, boost::bind(boost::factory<Task_RestoreVmBackupTarget* >(),
		boost::ref(m_registry), _1, _2, _3), pkg);
}

void Launcher::startCreateVmBackupTargetTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage>& p) const
{
	launch(pDispConnection, boost::bind(boost::factory<Task_CreateVmBackupTarget* >(),
		_1, _2, _3, boost::ref(*m_service)), p);
}

void Launcher::startRestoreVmBackupSourceTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage>& p) const
{
	launch(pDispConnection, boost::bind(boost::factory<Task_RestoreVmBackupSource* >(), _1, _2, _3), p);
}

void Launcher::startGetBackupTreeSourceTask
	(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg) const
{
	launch(pUserSession, boost::bind(boost::factory<Task_GetBackupTreeSource* >(), _1, _2, _3), pkg);
}

void Launcher::startGetBackupTreeTargetTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage> &p) const
{
	launch(pDispConnection, boost::bind(boost::factory<Task_GetBackupTreeTarget* >(), _1, _2, _3), p);
}

void Launcher::startRemoveVmBackupSourceTask
	(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg) const
{
	launch(pUserSession, boost::bind(boost::factory<Task_RemoveVmBackupSource* >(), _1, _2, _3), pkg);
}

void Launcher::startRemoveVmBackupTargetTask(
		SmartPtr<CDspDispConnection> pDispConnection,
		const SmartPtr<IOPackage> &p) const
{
	launch(pDispConnection, boost::bind(boost::factory<Task_RemoveVmBackupTarget* >(), _1, _2, _3), p);
}

void Launcher::launchEndVeBackup(const SmartPtr<CDspClient>& actor_,
	const SmartPtr<IOPackage>& request_) const
{
	m_taskManager->schedule(new Create::Flavored<Create::End, false>
		(actor_, request_, Create::End(*m_service)));
}

void Launcher::launchBeginCtBackup(const SmartPtr<CDspClient>& actor_,
	const SmartPtr<IOPackage>& request_) const
{
	typedef Create::Begin::Flavored<Create::Begin::Ct> flavor_type;
	CDspService* s = CDspService::instance();
	Create::Begin::Ct f(s->getVmDirHelper(), s->getDispConfigGuard(), s->getVzHelper());
	m_taskManager->schedule(new Create::Flavored<flavor_type, true>(actor_,
		request_, flavor_type(f, *m_service)));
}

void Launcher::launchBeginVmBackup(const SmartPtr<CDspClient>& actor_,
	const SmartPtr<IOPackage>& request_) const
{
	typedef Create::Begin::Flavored<Create::Begin::Vm> flavor_type;
	CDspService* s = CDspService::instance();
	Create::Begin::Vm f(s->getVmDirHelper(), s->getDispConfigGuard());
	m_taskManager->schedule(new Create::Flavored<flavor_type, true>(actor_,
		request_, flavor_type(f, *m_service)));
}

template<class T, class U>
void Launcher::launch(SmartPtr<T>& actor_, U factory_, const SmartPtr<IOPackage>& package_) const
{
	typename Parser<T>::result_type x = Parser<T>::do_(package_);
	if (x->IsValid())
	{
		CDspTaskHelper* t = factory_(actor_, x, package_);
		if (NULL != t)
			m_taskManager->schedule(t);
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "Invalid package : [%s]", package_->buffers[0].getImpl());
		actor_->sendSimpleResponse(package_, PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR);
	}
}
} // namespace Task

void NotifyClientsWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sVmDirectoryUuid,
		const QString &sVmUuid,
		int nPercents)
{
	CVmEvent event(PET_DSP_EVT_BACKUP_PROGRESS_CHANGED, sVmUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);

	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirectoryUuid, sVmUuid);
}

} // namespace Backup

