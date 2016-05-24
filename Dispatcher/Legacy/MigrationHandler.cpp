///////////////////////////////////////////////////////////////////////////////
///
/// @file MigrationHandler.cpp
///
/// Legacy migration app handler
///
/// Copyright (c) 2010-2016 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#include <prlcommon/Logging/Logging.h>
#include "CDspService.h"
#include "MigrationHandler.h"
#include "CVcmmdInterface.h"
#include "VmConverter.h"
#include "CDspVmManager_p.h"

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

PRL_RESULT Start::execute()
{
	return Libvirt::Kit.vms().at(m_uuid).start().isFailed() ?
		PRL_ERR_VM_START_FAILED : PRL_ERR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// struct Convert

PRL_RESULT Convert::execute()
{
	PRL_RESULT e;
	if (PRL_SUCCEEDED(e = Legacy::Vm::Converter().convertVm(m_uuid)))
		return m_next->execute();

	return e;
}

////////////////////////////////////////////////////////////////////////////////
// struct Vcmmd

PRL_RESULT Vcmmd::execute()
{
	CVmMemory* m = m_config->getVmHardwareList()->getMemory();
	quint64 z = m->getRamSize();
	quint64 g = ::Vm::Config::MemGuarantee(*m)(z);
	quint64 w  = m_config->
		getVmHardwareList()->getVideo()->getMemorySize();
	::Vcmmd::Frontend< ::Vcmmd::Unregistered> v(m_uuid);
	PRL_RESULT e = v(::Vcmmd::Unregistered(z << 20, g << 20, w << 20));

	if (PRL_FAILED(e))
		return e;

	if (PRL_SUCCEEDED(e = m_next->execute()))
		v.commit();

	return e;
}

////////////////////////////////////////////////////////////////////////////////
// struct Registration

PRL_RESULT Registration::execute()
{
	Libvirt::Result r(Command::Vm::Gear<Command::Tag::State
			  <Command::Vm::Registrator, Command::Vm::Fork::State::Strict<VMS_STOPPED> > >
			  ::run(*m_config));

	if (r.isFailed())
		return r.error().code();

	PRL_RESULT e = m_next->execute();

	if (PRL_FAILED(e))
		Libvirt::Kit.vms().at(m_uuid).undefine();

	return e;
}

} // namespace Step

REGISTER_HANDLER(IOSender::VmConverter, "VmConverter", Handler);

////////////////////////////////////////////////////////////////////////////////
// struct Convoy

bool Convoy::appoint(const SmartPtr<CVmConfiguration>& config_)
{
	QMutexLocker l(&m_mutex);

	if (!config_.isValid())
		return false;

	m_uuid = config_->getVmIdentification()->getVmUuid();
	m_config = config_;
	start("prl_legacy_migration_app");

	if (!waitForStarted())
	{
		WRITE_TRACE(DBG_FATAL, "Failed to start migration app");
		return false;
	}

	return true;
}

bool Convoy::deport()
{
	QMutexLocker l(&m_mutex);
	if (CDspService::instance()->getIOServer().detachClient
			(m_connection->GetConnectionHandle(), pid(), m_package))
		return true;

	m_connection->sendSimpleResponse(m_package, PRL_ERR_VM_MIGRATE_COULDNT_DETACH_TARGET_CONNECTION);
	return false;
}

void Convoy::release(const IOSender::Handle& handle_, const SmartPtr<IOPackage>& package_)
{
	QMutexLocker l(&m_mutex);

	QScopedPointer<Step::Unit> u;
	u.reset(new Step::Vcmmd(m_uuid, *m_config, new Step::Convert(m_uuid, new Step::Start(m_uuid))));
	u.reset(new Step::Registration(m_uuid, *m_config, u.take()));

	SmartPtr<IOPackage> p = IOPackage::duplicateInstance(package_);

	if (PRL_FAILED(u->execute()))
	{
		WRITE_TRACE(DBG_FATAL, "Asked migration app to cancel migration because unable to start Vm");
		CDispToDispCommandPtr c =
			CDispToDispProtoSerializer::CreateDispToDispCommandWithoutParams(VmMigrateCancelCmd);
		p = DispatcherPackage::createInstance(c);
	}

	SmartPtr<CDspClient> pUserSession;
	CDspDispConnection(handle_, pUserSession).sendPackageResult(p);
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

void Handler::handleClientConnected(const IOSender::Handle& handle_)
{
	QMutexLocker l(&m_mutex);

	if (m_convoy.isNull())
	{
		WRITE_TRACE(DBG_FATAL, "Unexpected migration app connection");
		CDspService::instance()->getIOServer().disconnectClient(handle_);
	}
	else
		m_client = handle_;
}

void Handler::handleToDispatcherPackage(const IOSender::Handle& handle_, const SmartPtr<IOPackage>& package_)
{
	QMutexLocker l(&m_mutex);

	QSharedPointer<Convoy> convoy = m_convoy.toStrongRef();

	if (convoy.isNull() ||  m_client != handle_)
	{
		CDspService::instance()->getIOServer().disconnectClient(handle_);
		return;
	}

	if (!QMetaObject::invokeMethod(convoy.data(), "handlePackage", Qt::QueuedConnection,
			Q_ARG(IOSender::Handle, handle_), Q_ARG(const SmartPtr<IOPackage>, package_)))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to invoke method");
		CDspService::instance()->getIOServer().disconnectClient(handle_);
	}
}

void Handler::handleDetachClient(const IOSender::Handle&, const IOCommunication::DetachedClient& client_)
{
	IOSendJob::Handle job =	CDspService::instance()->getIOServer().sendDetachedClient(m_client, client_);
	IOSendJob::Result res = CDspService::instance()->getIOServer().waitForSend(job);
	if (res != IOSendJob::Success)
		WRITE_TRACE(DBG_FATAL, "Failed to send detached client");

	res = CDspService::instance()->getIOServer().getSendResult(job);
	if (res != IOSendJob::Success)
		WRITE_TRACE(DBG_FATAL, "Failed to send detached client");
}

bool Handler::request(const QSharedPointer<Convoy>& convoy_)
{
	QMutexLocker l(&m_mutex);

	if (!m_convoy.isNull())
		return false;

	m_convoy = convoy_;
	return true;
}

} // namespace Migration
} // namespace Vm
} // namespace Legacy
