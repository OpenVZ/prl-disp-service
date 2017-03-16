///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConnection.cpp
///
/// Dispatcher-dispatcher client class which is responsible for representation some another
/// dispatcher connection in the current dispatcher system
///
/// @author sandro
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

#include "CDspDispConnection.h"
#include "CDspService.h"

CDspDispConnection::CDspDispConnection ( const IOSender::Handle &h, const SmartPtr<CDspClient> &pUserSession )
: m_clientHandle(h), m_pUserSession(pUserSession)
{
}

PRL_RESULT CDspDispConnection::AuthorizeDispatcherSession(const QString &sUserSessionId)
{
	Q_UNUSED(sUserSessionId);
	return (PRL_ERR_UNIMPLEMENTED);
}

IOSendJob::Handle CDspDispConnection::sendPackage ( const SmartPtr<IOPackage> &p) const
{
	return (CDspService::instance()->getIOServer().sendPackage(m_clientHandle, p));
}

PRL_RESULT CDspDispConnection::sendPackageResult(const SmartPtr<IOPackage> &p) const
{
	IOSendJob::Handle job;

	job = CDspService::instance()->getIOServer().sendPackage(m_clientHandle, p);
	return job.isValid() ? PRL_ERR_SUCCESS : PRL_ERR_OPERATION_FAILED;
}

IOSendJob::Handle CDspDispConnection::sendSimpleResponse ( const SmartPtr<IOPackage> &pRequestPkg,
				PRL_RESULT nRetCode ) const
{
	return (CDspService::instance()->sendSimpleResponseToDispClient(m_clientHandle, pRequestPkg, nRetCode));
}

IOSendJob::Handle CDspDispConnection::sendResponse ( const CDispToDispCommandPtr &pResponse,
				const SmartPtr<IOPackage> &pRequestPkg) const
{
	return (CDspService::instance()->sendResponseToDispClient(m_clientHandle, pResponse, pRequestPkg));
}

IOSendJob::Handle CDspDispConnection::sendResponseError ( const CVmEvent &vm_event,
				const SmartPtr<IOPackage> &pRequestPkg) const
{
	CDispToDispCommandPtr pResponse =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			pRequestPkg, &vm_event);
	return sendResponse( pResponse, pRequestPkg );
}

IOSendJob::Handle CDspDispConnection::sendResponseError ( const CVmEvent *pVmEvent,
				const SmartPtr<IOPackage> &pRequestPkg) const
{
	CDispToDispCommandPtr pResponse =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			pRequestPkg, pVmEvent);
	return sendResponse( pResponse, pRequestPkg );
}

IOSender::Handle CDspDispConnection::GetConnectionHandle() const
{
	return (m_clientHandle);
}

SmartPtr<CDspClient> CDspDispConnection::getUserSession() const
{
	return (m_pUserSession);
}

void CDspDispConnection::handlePackage(const SmartPtr<IOPackage> p)
{
	emit onPackageReceived(m_clientHandle, p);
}

