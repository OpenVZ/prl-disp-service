///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspClient
///
/// Client class, which is responsible for representation some user in
/// the disaptcher system
///
/// @author romanp
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

#ifndef CDSPCLIENT_H
#define CDSPCLIENT_H

#include <QString>
#include <QHash>

#include "CVmIdent.h"
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include <prlxmlmodel/DispConfig/CDispUser.h>
#include <prlxmlmodel/UserInformation/SessionInfo.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include <prlcommon/IOService/IOCommunication/IOSendJob.h>
#include "Libraries/ProtoSerializer/CProtoSerializer.h"

using namespace IOService;
using Parallels::CProtoCommandPtr;

class CDspClient;

class ClientQuestionHelper
{
public:

	ClientQuestionHelper(PRL_RESULT nQuestionId,
						 const QList<PRL_RESULT>& lstChoices,
						 const QList<CVmEventParameter* >& lstParams = QList<CVmEventParameter* >(),
						 const SmartPtr<IOPackage> &pRequestPkg=SmartPtr<IOPackage>( 0 ) );
	virtual ~ClientQuestionHelper();

	virtual SmartPtr<IOPackage > getQuestionDonePackage() = 0;

	virtual bool needAddNewClients();
	virtual bool isClientSatisfied(const SmartPtr<CDspClient >& pClient);

	// Input
	PRL_RESULT					m_nQuestionId;
	QList<PRL_RESULT>			m_lstChoices;
	QList<CVmEventParameter* >	m_lstParams;
	SmartPtr<IOPackage>			m_pRequestPkg;

	// Output
	CVmEvent					m_responseEvent;
	SmartPtr<CDspClient >		m_pAnswerClient;
};


class CDspClient
{
public:
	static SmartPtr<CDspClient> makeServiceUser( const QString& sDirUuid = "" );

public:
	CDspClient ( const IOSender::Handle&,
				 const QString& user,
				 quint32 nFlags = 0 );

	CDspClient ( const IOSender::Handle& );
	~CDspClient ();

	/** Returns auth helper instance */
	CAuthHelper& getAuthHelper ();

	/** Sets user state */
	void setUserState ( PVE::DispUserState );

	/** Returns user session uptime */
	PRL_UINT64 getSessionUptimeInSec () const;

	/** Returns uuid on user settings */
	const QString getUserSettingsUuid () const;

	/** Sets user settings */
	void setUserSettings ( const QString& uuid, const QString& userName );

	/** Returns uuid of vm directory  */
	const QString getVmDirectoryUuid() const;
	const QStringList getVmDirectoryUuidList () const;

	/** Sets vm directory uuid */
	void setVmDirectoryUuid( const QString& id);

    inline CVmIdent getVmIdent(const QString &sVmUuid) const
    {
        return MakeVmIdent(sVmUuid, getVmDirectoryUuid()) ;
    }

	/** Returns user session remote host name */
	QString getUserHostAddress() const;

	/** Returns client handle */
	const IOSender::Handle getClientHandle () const;

	/** Returns client handle */
	const IOSender::Handle getSessionUuid () const { return getClientHandle(); }

	/** Returns uuid of previous session */
	const QString getPrevSessionUuid( ) const;

	/** Store uuid of previous session */
	void setPrevSessionUuid( const QString& prevSessionUuid );

	/** Store info of client process **/
	void storeClientProcessInfo( Q_PID clientPid );


	// returns false when variable "envVarName" not found in envinronment
	// returns true and "value" when variable was found.
	bool getClientEnvinromentVariable(  const QString& envVarName, QString& value ) const;

	/** Returns client flags */
	quint32 getFlags () const { return m_nFlags; }


	/**
	 * Sends specified package to client
	 * @param pointer to the sending package
	 * @return handle to the sent job
	 */
	IOSendJob::Handle sendPackage ( const SmartPtr<IOPackage> &p) const;
	/**
	 * Sends simple response (without additional parameters) to client
	 * @param pointer to reqeust package object
	 * @param operation return code
	 * @return handle to sent job
	 */
	IOSendJob::Handle sendSimpleResponse ( const SmartPtr<IOPackage> &pRequestPkg, PRL_RESULT nRetCode ) const;
	/**
	 * Sends prepared response package to client
	 * @param pointer to prepared response package object
	 * @param pointer to initial request package object
	 * @return handle to sent job
	 */
	IOSendJob::Handle sendResponse ( const CProtoCommandPtr &pResponse, const SmartPtr<IOPackage> &pRequestPkg) const;
	/**
	 * Sends vm event as error response package to client
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
	 * Returns path where user's VMs storing by default
	 */
	QString getUserDefaultVmDirPath() const;

	/**
	 * Returns user name of session owner
	 */
	QString getUserName() const;


	enum CLIENT_TYPE
	{
		CT_USUAL_CLIENT		= (1<<0),
		CT_ALL_CLIENTS		= (CT_USUAL_CLIENT),
	};

	void setClientType(CDspClient::CLIENT_TYPE ct);

	CDspClient::CLIENT_TYPE getClientType() const;

	// is client from local host
	bool	isLocal();
	// set client local host property
	void	setLocal( bool bLocal );

	// Sets noninteractive session mode
	void setNonInteractive( bool bNonInteractive );
	// Gets noninteractive session mode
	bool isNonInteractive() const;

	// #436109 ConfirmationMode operation for user session to enable confirm for some operation
	void setConfirmationMode( bool bConfirmationEnabled );
	bool isConfirmationEnabled() const;

	// Set admin authentication was passed
	void setAdminAuthWasPassed(bool bAuth);
	bool isAdminAuthWasPassed() const;

	const SmartPtr<SessionInfo > getSessionInfo() const { return m_pSessionInfo; }

private:

	static SmartPtr<IOPackage> MakeQuestionPackage( PRL_RESULT nQuestionId,
													const QList<PRL_RESULT> &lstChoices,
													const QList<CVmEventParameter*>& lstParams,
													const SmartPtr<IOPackage> &pRequestPkg );

	mutable QMutex m_mutex;
	IOSender::Handle m_clientHandle;
	CAuthHelper m_authHelper;
	PRL_UINT64 m_creationTimeStamp;
	QString	m_settingsUuid;
	QString	m_userName;
	QString	m_vmDirUuid;
	PVE::DispUserState m_state;
	mutable bool m_isHostNameCached;
	mutable QString m_hostName;

	CLIENT_TYPE	m_clientType;

	// used in connection restoring mech [ look task #6009 for more info ]
	QString	m_prevSessionUuid;
	bool	m_bIsLocal;

	bool	m_bNonInteractive;
	bool	m_bConfirmationEnabled; // #436109
	bool	m_bAdminAuthWasPassed;

	quint32 m_nFlags;

	/** client process envinroment on login local moment**/
	QHash<QString, QString> m_clientEnviron;

	// Contains different information about session
	SmartPtr<SessionInfo >	m_pSessionInfo;
};

#endif //CDSPCLIENT_H
