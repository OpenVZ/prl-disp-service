///////////////////////////////////////////////////////////////////////////////
///
/// @file MigrationHandler.cpp
///
/// Legacy migration app handler
///
/// Copyright (c) 2010-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#include <prlcommon/Logging/Logging.h>
#include "CDspService.h"
#include "MigrationHandler.h"
#include "CVcmmdInterface.h"
#include "VmConverter.h"
#include "CDspVmManager_p.h"
#include "Tasks/Task_EditVm.h"
#include "Tasks/Task_EditVm_p.h"

namespace Legacy
{
namespace Vm
{
namespace Migration
{
namespace Step
{
////////////////////////////////////////////////////////////////////////////////
// struct Start

result_type Start::execute()
{
	return Libvirt::Kit.vms().at(m_uuid).getState().start();
}

////////////////////////////////////////////////////////////////////////////////
// struct Start

result_type FirstStart::execute()
{
	WRITE_TRACE(DBG_INFO, "Start converted VM for the first time");
	if (m_vnc)
		m_vnc->close();
	result_type e = m_v2v.start();
	if (e.isSucceed())
		return m_next->execute();

	return e;
}

////////////////////////////////////////////////////////////////////////////////
// struct Convert

result_type Convert::execute()
{
	PRL_RESULT e;
	if (PRL_SUCCEEDED(e = m_v2v.do_()))
		return m_next->execute();

	if (CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()
		->getDebug()->isLegacyVmUpgrade())
		return error_type(PRL_ERR_FIXME, "Unable to convert the migrated VM. It was preserved for investigation");

	return error_type(e, "Failed to convert VM. For more details, see logs on the target node.");
}

////////////////////////////////////////////////////////////////////////////////
// struct Vcmmd

result_type Vcmmd::execute()
{
	::Vcmmd::Frontend< ::Vcmmd::Unregistered> v(m_uuid);
	const SmartPtr<CVmConfiguration> config(new CVmConfiguration(m_config));
	PRL_RESULT e = v(::Vcmmd::Unregistered(config));

	if (PRL_FAILED(e))
		return error_type(e, "Unable to register VM in VCMMD");

	result_type r = m_next->execute();
	if (r.isSucceed())
		v.commit();

	return r;
}

////////////////////////////////////////////////////////////////////////////////
// struct Registration

result_type Registration::execute()
{
	Libvirt::Result r(Command::Vm::Gear<Command::Tag::State
			  <Command::Vm::Registrator, Command::Vm::Fork::State::Strict<VMS_STOPPED> > >
			  ::run(*m_config));

	if (r.isFailed())
		return r;

	result_type e = m_next->execute();

	if (e.isFailed() && e.error().code() != PRL_ERR_FIXME)
		Libvirt::Kit.vms().at(m_uuid).getState().undefine();

	return e;
}

////////////////////////////////////////////////////////////////////////////////
// struct Nvram

result_type Nvram::execute()
{
	PRL_RESULT e = PRL_ERR_SUCCESS;
	CVmStartupBios* b = m_config->getVmSettings()->getVmStartupOptions()->getBios();
	QString n = b->getNVRAM();
	if (!n.isEmpty())
	{
		QFile::remove(n);
		e = Edit::Vm::Create::Action<CVmStartupBios>(*b, *m_config).execute();
	}

	if (PRL_SUCCEEDED(e))
		return m_next->execute();

	return error_type(e, "Unable to create NVRAM for VM");
}

} // namespace Step

////////////////////////////////////////////////////////////////////////////////
// struct Convoy

const char Convoy::migrationBinary[] = "/usr/bin/prl_legacy_migration_app";

bool Convoy::appoint(const SmartPtr<CVmConfiguration>& config_, const QSharedPointer<QTcpServer>& vnc_)
{
	if (!config_.isValid())
		return false;

	m_vnc = vnc_;
	m_uuid = config_->getVmIdentification()->getVmUuid();
	m_config = config_;
	start(migrationBinary);

	if (!waitForStarted())
	{
		WRITE_TRACE(DBG_FATAL, "Failed to start migration app");
		return false;
	}

	return true;
}

bool Convoy::deport()
{
	if (CDspService::instance()->getIOServer().detachClient
			(m_connection->GetConnectionHandle(), pid(), m_package))
		return true;

	m_connection->sendSimpleResponse(m_package, PRL_ERR_VM_MIGRATE_COULDNT_DETACH_TARGET_CONNECTION);
	return false;
}

void Convoy::release(const IOSender::Handle& handle_, const SmartPtr<IOPackage>& package_)
{
	QScopedPointer<Step::Unit> u;
	u.reset(new Step::Vcmmd(m_uuid, *m_config, new Step::Start(m_uuid)));
	// TODO: possibly we need to setup vcmmd before the first start too
	boost::optional<Legacy::Vm::V2V> v2v = Legacy::Vm::Converter().getV2V(*m_config);
	if (v2v)
		u.reset(new Step::Convert(*v2v, new Step::FirstStart(*v2v, m_vnc, u.take())));
	u.reset(new Step::Nvram(*m_config, new Step::Registration(m_uuid, *m_config, u.take())));

	SmartPtr<CDspClient> pUserSession;

	Step::result_type e = u->execute();
	if (e.isFailed())
	{
		CVmEvent v = e.error().convertToEvent();
		CDispToDispCommandPtr d = CDispToDispProtoSerializer::CreateDispToDispResponseCommand(package_, &v);

		CDspDispConnection(handle_, pUserSession).sendPackageResult(DispatcherPackage::createInstance(
			d->GetCommandId(), d->GetCommand()->toString(), package_));
		m_result = e.error().code();
	}
	else
		CDspDispConnection(handle_, pUserSession).sendPackageResult(IOPackage::duplicateInstance(package_));
}

void Convoy::handlePackage(IOSender::Handle handle_, const SmartPtr<IOPackage> package_)
{
	switch(package_->header.type)
	{
	case PVE::DspVmAuth:
		WRITE_TRACE(DBG_DEBUG, "Migration app is ready to start migration");
		if (!deport())
			CDspService::instance()->getIOServer().disconnectClient(handle_);
		break;
	case VmMigrateFinishCmd:
		WRITE_TRACE(DBG_DEBUG, "Migration app is ready to finish migration");
		release(handle_, package_);
		break;
	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// struct Handler

Handler::Handler(IOServerPool& server_, const QSharedPointer<Convoy>& convoy_)
	: m_server(&server_), m_convoy(convoy_), m_client()
{
	if (!QObject::connect(m_server,	SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(packageReceived(IOSender::Handle, const SmartPtr<IOPackage>)), Qt::QueuedConnection))
		WRITE_TRACE(DBG_DEBUG, "unable to connect package received");

	if (!QObject::connect(m_server,
		SIGNAL(onDetachClient(IOSender::Handle, const IOCommunication::DetachedClient)),
		SLOT(clientDetached(IOSender::Handle, const IOCommunication::DetachedClient)), Qt::QueuedConnection))
		WRITE_TRACE(DBG_DEBUG, "unable to connect detach client");
}

void Handler::packageReceived(IOSender::Handle handle_, const SmartPtr<IOPackage> package_)
{
	if (m_server->clientSenderType(handle_) != IOSender::VmConverter)
		return;

	if (m_client.isEmpty() && package_->header.type == PVE::DspVmAuth)
	{
		qint64 pid = QString(package_->buffers[0].getImpl()).toLongLong();
		if (m_convoy->pid() <= 0 || m_convoy->pid() != pid)
			return;

		m_client = handle_;
	}

	if (m_client != handle_)
		return;

	m_convoy->handlePackage(handle_, package_);
}

void Handler::clientDetached(IOSender::Handle handle_, const IOCommunication::DetachedClient client_)
{
	if (m_convoy->getConnection() != handle_)
		return;

	IOSendJob::Handle job =	CDspService::instance()->getIOServer().sendDetachedClient(m_client, client_);
	IOSendJob::Result res = CDspService::instance()->getIOServer().waitForSend(job);
	if (res != IOSendJob::Success)
		WRITE_TRACE(DBG_FATAL, "Failed to send detached client");

	res = CDspService::instance()->getIOServer().getSendResult(job);
	if (res != IOSendJob::Success)
		WRITE_TRACE(DBG_FATAL, "Failed to send detached client");
}

} // namespace Migration
} // namespace Vm
} // namespace Legacy
