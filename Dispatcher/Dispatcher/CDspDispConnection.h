///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConnection.h
///
/// Dispatcher-dispatcher client class which is responsible for representation some another
/// dispatcher connection in the current dispatcher system
///
/// @author sandro
/// @owner sergeym
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

#ifndef CDspDispConnection_H
#define CDspDispConnection_H

#include "CDspClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

using namespace Parallels;

class CDspDispConnection : public QObject
{
	Q_OBJECT

public:
	/**
	 * Class constructor
	 * @param handle to dipatcher connection
	 * @param pointer to the user session that was used to dispatcher connection authorization
	 */
	CDspDispConnection ( const IOSender::Handle &h, const SmartPtr<CDspClient> &pUserSession );

	/**
	 * Authorizes dispatcher session by specified session id of already
	 * exists client-dispatcher connection
	 * @param exists session UUID
	 * @returns result of authorization
	 */
	PRL_RESULT AuthorizeDispatcherSession(const QString &sUserSessionId);

	/**
	 * Sends specified package to dispatcher's client
	 * @param pointer to the sending package
	 * @return handle to the sent job
	 */
	IOSendJob::Handle sendPackage ( const SmartPtr<IOPackage> &p) const;
	PRL_RESULT sendPackageResult(const SmartPtr<IOPackage> &p) const;

	/**
	 * Sends simple response (without additional parameters) to dispatcher client
	 * @param pointer to reqeust package object
	 * @param operation return code
	 * @return handle to sent job
	 */
	IOSendJob::Handle sendSimpleResponse ( const SmartPtr<IOPackage> &pRequestPkg, PRL_RESULT nRetCode ) const;

	/**
	 * Sends prepared response package to dispatcher client
	 * @param pointer to prepared response package object
	 * @param pointer to initial request package object
	 * @return handle to sent job
	 */
	IOSendJob::Handle sendResponse (
		const CDispToDispCommandPtr &pResponse,
		const SmartPtr<IOPackage> &pRequestPkg
	) const;

	/**
	 * Sends vm event as error response package to dispatcher client
	 * @param pointer to vm event
	 * @param pointer to initial request package object
	 * @return handle to sent job
	 */
	IOSendJob::Handle sendResponseError ( const CVmEvent&, const SmartPtr<IOPackage> &pRequestPkg) const;

	/**
	 * Same as above, but ptr event
	 */
	IOSendJob::Handle sendResponseError ( const CVmEvent*, const SmartPtr<IOPackage> &pRequestPkg) const;

	/**
	 * Returns dispatcher connection handle
	 */
	IOSender::Handle GetConnectionHandle() const;

	/**
	 * Returns pointer to user session object
	 */
	SmartPtr<CDspClient> getUserSession() const;

	/**
	 * To emit onPackageReceived signal
	 */
	void handlePackage(const SmartPtr<IOPackage>);
signals:
	/**
	 * Emits when server has been received a package
	 */
	void onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>);
private:
	/**
	 * Handle to dispatcher connection
	 */
	IOSender::Handle m_clientHandle;
	/**
	 * Pointer to corresponding user session which using to provide all operations
	 */
	SmartPtr<CDspClient> m_pUserSession;
};

#endif //CDspDispConnection_H
