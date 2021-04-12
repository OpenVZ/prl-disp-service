///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConnectionsManager.cpp
///
/// Manager of connections from another dispatchers to this one
///
/// @author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspDispConnectionsManager.h"
#include "CDspService.h"
#include "CDspClientManager.h"
#include "CDspRouter.h"
#include "CDspVm.h"
#include <boost/mpl/fold.hpp>
#include <boost/scope_exit.hpp>
#include <boost/range/join.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/range/irange.hpp>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/CRsaHelper.hpp>
#include <boost/range/algorithm/find.hpp>
#include "Tasks/Legacy/MigrateVmTarget.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

#include "CDspVzHelper.h"

using namespace Virtuozzo;

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Next

template<class T>
struct Next
{
	static void do_(const SmartPtr<IOPackage>& request_)
	{
		T::do_(request_);
	}
};

template<>
struct Next<void>
{
	static void do_(const SmartPtr<IOPackage>& request_)
	{
		WRITE_TRACE(DBG_FATAL, "Processing command %d", request_->header.type);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Step

template<class T, class U>
struct Step
{
	static void do_(const SmartPtr<IOPackage>& request_)
	{
		quint32 t = request_->header.type;
		if (T::contains(t))
			WRITE_TRACE(DBG_DEBUG, "Processing command %d", t);
		else
			Next<U>::do_(request_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Copy_

struct Copy_
{
	static bool contains(quint32 type_)
	{
		return IS_FILE_COPY_PACKAGE(type_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Backup_

struct Backup_
{
	static bool contains(quint32 type_)
	{
		return IS_ABACKUP_PROXY_PACKAGE(type_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Migrate_

struct Migrate_
{
	static bool contains(quint32 type_)
	{
		typedef boost::integer_range<unsigned> irange_type;
		boost::joined_range
		<
			const irange_type,
			const irange_type
		> x = boost::join(irange_type(CtMigrateCmd, CtMigrateCmd + 1),
			irange_type(VmMigrateTunnelRangeStart, VmMigrateTunnelRangeEnd + 1));

		return boost::end(x) != boost::find(x, type_);
	}
};

bool is_migrate_protocol_package(int header_type)
{
	return header_type > VmMigrateRangeStart && header_type < VmMigrateRangeEnd;
}

bool is_migrate_package(int header_type)
{
	return is_migrate_protocol_package(header_type) || IS_FILE_COPY_PACKAGE(header_type);
}

}; // namespace

/*****************************************************************************/

CDspDispConnectionsManager::CDspDispConnectionsManager
	(CDspService& service_, const Backup::Task::Launcher& backup_):
	CDspHandler(IOService::IOSender::Dispatcher, "DispToDispHandler"),
	m_service(&service_), m_backup(backup_)
{
}

CDspDispConnectionsManager::~CDspDispConnectionsManager ()
{
}

void CDspDispConnectionsManager::handleClientDisconnected ( const IOSender::Handle& h )
{
	DeleteDispConnection(h);
}

void CDspDispConnectionsManager::DeleteDispConnection( const IOSender::Handle& h )
{
	QWriteLocker locker( &m_rwLock );
	m_dispconns.take(h);
}

void CDspDispConnectionsManager::handleToDispatcherPackage ( const IOSender::Handle& h,
												const SmartPtr<IOPackage> &p )
{
 	LOG_MESSAGE(DBG_DEBUG, "%s received package [%s]", __FUNCTION__, p->buffers[0].getImpl());

	boost::mpl::fold
	<   
		boost::mpl::vector<Copy_, Backup_, Migrate_>,
		void,
		Step<boost::mpl::_2, boost::mpl::_1>
	>::type::do_(p);

	switch( p->header.type )
	{
		case DispToDispAuthorizeCmd:
		{
			if (CDispToDispProtoSerializer::ParseCommand(p)->GetCommandFlags() & PLLF_LOGIN_WITH_RSA_KEYS)
				processPubKeyAuthorizeCmd(h, p);
			else
				processAuthorizeCmd(h, p);
			return;
		}
	}


	// Package should be authorized!
	m_rwLock.lockForRead();
	bool cliExists = m_dispconns.contains(h) && !m_dispconns[h]->isAuthorizationInProgress();

	// Check authorization
	if ( ! cliExists ) {
		m_rwLock.unlock();

		// Send error
		m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_USER_OPERATION_NOT_AUTHORISED);
		return;
	}

	// #455781 under read access (QReadLocker) we should call only const methods
	// to prevent app crash after simultaneously call QT_CONT::detach_helper().
	SmartPtr<CDspDispConnection> pDispConnection = m_dispconns.value(h);

	m_rwLock.unlock();

#ifdef _CT_
	if (m_service->getVzHelper()->handleToDispatcherPackage(pDispConnection, p))
		return;
#endif

	switch (p->header.type)
	{
		case DispToDispLogoffCmd:
			ProcessDispConnectionLogoff(pDispConnection, p);
			break;
		case VmMigrateCheckPreconditionsCmd:
			m_service->getVmMigrateHelper().checkPreconditions(pDispConnection, p);
			break;
		case VmMigrateStartCmd:
			m_service->getVmMigrateHelper().startMigration(pDispConnection, p);
			break;
		case VmBackupCreateCmd:
		case VmBackupCreateLocalCmd:
			m_backup.startCreateVmBackupTargetTask(pDispConnection, p);
			break;
		case VmBackupRestoreCmd:
			m_backup.startRestoreVmBackupSourceTask(pDispConnection, p);
			break;
		case VmBackupGetTreeCmd:
			m_backup.startGetBackupTreeTargetTask(pDispConnection, p);
			break;
		case VmBackupRemoveCmd:
			m_backup.startRemoveVmBackupTargetTask(pDispConnection, p);
			break;
		default:
		{
			if (IS_FILE_COPY_PACKAGE(p->header.type) ||
				IS_ABACKUP_PROXY_PACKAGE(p->header.type) ||
				is_migrate_package(p->header.type) ||
				(p->header.type == VmBackupMountImage) ||
				(p->header.type == CtMigrateCmd))
			{
				/* retransmit package to task */
				pDispConnection->handlePackage(p);
				break;
			}
			m_service->sendSimpleResponseToDispClient( h, p, PRL_ERR_UNRECOGNIZED_REQUEST);
			break;
		}
	}
}

PRL_RESULT CDspDispConnectionsManager::preAuthChecks(
	const IOSender::Handle& h
)
{
	if (m_service->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
		return PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
	}

	m_rwLock.lockForRead();
	bool cliExists = m_dispconns.contains(h) && !m_dispconns[h]->isAuthorizationInProgress();
	m_rwLock.unlock();

	if (cliExists)
		return PRL_ERR_DISP2DISP_SESSION_ALREADY_AUTHORIZED;
	return PRL_ERR_SUCCESS;
}

void CDspDispConnectionsManager::processAuthorizeCmd(
	const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p
)
{
	BOOST_SCOPE_EXIT(&p)
	{
		IOPackage::PODData &d = IODATAMEMBER(p.getImpl())[0];
		bzero(p->buffers[0].getImpl(), d.bufferSize);
	}
	BOOST_SCOPE_EXIT_END;
	PRL_RESULT ret;
	if ((ret = preAuthChecks(h)) != PRL_ERR_SUCCESS)
	{
		m_service->sendSimpleResponseToDispClient(h, p, ret);
		return;
	}
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	CDispToDispAuthorizeCommand *pAuthorizeCommand =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(pCmd);

	if (!pAuthorizeCommand->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Wrong key authorization package was received: [%s]", \
            p->buffers[0].getImpl());
		m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}

	bool is_auth_in_progress;
	{
		QReadLocker locker(&m_rwLock);
		is_auth_in_progress = m_dispconns.contains(h) && m_dispconns[h]->isAuthorizationInProgress();
	}

	if (is_auth_in_progress)
	{
		m_service->sendSimpleResponseToDispClient(h, p, VerifyDispClientIdentity(h, pAuthorizeCommand));
		return;
	}

	Prl::Expected<SmartPtr<CDspDispConnection>, Error::Simple> pDispConnection;
	if (pAuthorizeCommand->NeedAuthBySessionUuid())
		pDispConnection = RestoreDispConnectionFromUserSession(h, pAuthorizeCommand);
	else
		pDispConnection = CreateDispSession(h, p);

	if (pDispConnection.isFailed())
	{
		m_service->sendSimpleResponseToDispClient(h, p, pDispConnection.error().code());
		return;
	}

	{
		QWriteLocker locker(&m_rwLock);
		/* We have to recheck connection because lock was dropped. */
		if (m_dispconns.contains(h))
		{
			m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_DISP2DISP_SESSION_ALREADY_AUTHORIZED);
			return;
		}
		m_dispconns[h] = pDispConnection.value();
	}
	m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_SUCCESS);
}

void CDspDispConnectionsManager::processPubKeyAuthorizeCmd(
	const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p
)
{
	BOOST_SCOPE_EXIT(&p)
	{
		IOPackage::PODData& d = IODATAMEMBER(p.getImpl())[0];
		bzero(p->buffers[0].getImpl(), d.bufferSize);
	}
	BOOST_SCOPE_EXIT_END;
	PRL_RESULT ret;
	if ((ret = preAuthChecks(h)) != PRL_ERR_SUCCESS)
	{
		m_service->sendSimpleResponseToDispClient(h, p, ret);
		return;
	}
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	CDispToDispAuthorizeCommand *pAuthorizeCommand =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(pCmd);

	if (!pAuthorizeCommand->IsValid())
	{
		WRITE_TRACE(DBG_FATAL, "Wrong public key authorization package was received: [%s]", \
            p->buffers[0].getImpl());
		m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}

	QString username = pAuthorizeCommand->GetUserName();
	CRsaHelper rsa(CAuthHelper(username).getHomePath());
	// Check public key is authorized
	QString public_key = pAuthorizeCommand->GetPassword();
	if (!rsa.isAuthorized(public_key))
	{
		m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_PUBLIC_KEY_NOT_AUTHORIZED);
		return;
	}

	{
		// Authorize connection
		auto pClient = SmartPtr<CDspClient>(new CDspClient(h, username, 0));
		bool res = m_service->getUserHelper().fillUserPreferences(h, p, pClient);
		if (!res) {
			// Error response is sent by UserHelper
			return;
		}
		m_rwLock.lockForWrite();
		/* We have to recheck connection because lock was dropped. */
		if (m_dispconns.contains(h))
		{
			m_rwLock.unlock();
			m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_DISP2DISP_SESSION_ALREADY_AUTHORIZED);
			return;
		}
		m_dispconns[h] = SmartPtr<CDspDispConnection>(new CDspDispConnection(h, pClient, /*m_bAuthorizationInProgress=*/true));
		m_rwLock.unlock();
	}

	// Encrypt session uuid
	auto enc_session_uuid = rsa.Encrypt(h, public_key);
	if (enc_session_uuid.isFailed())
	{
		m_service->sendSimpleResponseToDispClient(h, p, enc_session_uuid.error().code());
		return;
	}
	m_service->sendSimpleResponseToDispClient(h, p, PRL_ERR_SUCCESS, QStringList{enc_session_uuid.value()});
}

void CDspDispConnectionsManager::ProcessDispConnectionLogoff(
	const SmartPtr<CDspDispConnection> &pDispConnection,
	const SmartPtr<IOPackage>& p
)
{
	IOSendJob::Handle hJob = pDispConnection->sendSimpleResponse( p, PRL_ERR_SUCCESS );
	m_service->getIOServer().waitForSend(hJob);
	DeleteDispConnection(pDispConnection->GetConnectionHandle());
}

PRL_RESULT CDspDispConnectionsManager::VerifyDispClientIdentity(
	const IOSender::Handle& h,
	CDispToDispAuthorizeCommand *pAuthorizeCommand
)
{
	if (h != pAuthorizeCommand->GetUserSessionUuid())
		return PRL_ERR_AUTHENTICATION_FAILED;
	QWriteLocker locker(&m_rwLock);
	m_dispconns[h]->setAuthorizationInProgress(false);
	return PRL_ERR_SUCCESS;
}

Prl::Expected<SmartPtr<CDspDispConnection>, Error::Simple> CDspDispConnectionsManager::RestoreDispConnectionFromUserSession(
	const IOSender::Handle& h,
	CDispToDispAuthorizeCommand *pAuthorizeCommand
)
{
	auto pUserSession =
		m_service->getClientManager().getUserSession(pAuthorizeCommand->GetUserSessionUuid());
	if (!pUserSession.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher session wasn't authorized with '%s' session UUID", \
            qPrintable(pAuthorizeCommand->GetUserSessionUuid()));
		return Error::Simple(PRL_ERR_DISP2DISP_WRONG_USER_SESSION_UUID);
	}
	WRITE_TRACE(DBG_FATAL, "Dispatcher session was successfully authorized with '%s' session UUID",\
			qPrintable(pAuthorizeCommand->GetUserSessionUuid()));
	return SmartPtr<CDspDispConnection>(new CDspDispConnection(h, pUserSession));
}

Prl::Expected<SmartPtr<CDspDispConnection>, Error::Simple> CDspDispConnectionsManager::CreateDispSession(
	const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p
)
{
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	CDispToDispAuthorizeCommand *pAuthorizeCommand =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(pCmd);
	auto pUserSession = m_service->getUserHelper().processDispacherLogin(h, p);
	if (!pUserSession.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher session wasn't authorized for user '%s'", \
            qPrintable(pAuthorizeCommand->GetUserName()));
		return Error::Simple(PRL_ERR_DISP2DISP_WRONG_USER_SESSION_UUID);
	}
	WRITE_TRACE(DBG_FATAL, "Dispatcher session was successfully authorized for '%s' user",\
			qPrintable(pAuthorizeCommand->GetUserName()));
	return SmartPtr<CDspDispConnection>(new CDspDispConnection(h, pUserSession));
}
