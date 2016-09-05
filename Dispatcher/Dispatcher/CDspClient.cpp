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

// #define FORCE_LOGGING_ON
// #define FORCE_LOGGING_LEVEL		DBG_DEBUG
#include <prlcommon/Logging/Logging.h>

#include "CDspClient.h"
#include "CDspService.h"
#include "CDspUserHelper.h"

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"

using namespace Parallels;

#define TIMEOUT_CLIENT_MSEC		100

/*****************************************************************************/

ClientQuestionHelper::ClientQuestionHelper(PRL_RESULT nQuestionId,
										   const QList<PRL_RESULT>& lstChoices,
										   const QList<CVmEventParameter* >& lstParams,
										   const SmartPtr<IOPackage> &pRequestPkg)
: m_nQuestionId(nQuestionId),
  m_lstChoices(lstChoices),
  m_lstParams(lstParams),
  m_pRequestPkg(pRequestPkg)
{
}

ClientQuestionHelper::~ClientQuestionHelper()
{
	while(!m_lstParams.isEmpty())
	{
		CVmEventParameter* pParam = m_lstParams.takeFirst();
		m_lstParams.removeAll(pParam);
		if (pParam)
			delete pParam;
	}
}

bool ClientQuestionHelper::needAddNewClients()
{
	return true;
}

bool ClientQuestionHelper::isClientSatisfied(const SmartPtr<CDspClient >& pClient)
{
	return pClient.isValid();
}

/*****************************************************************************/
///////////////////////////////////////////////////////////////////////////////
// TODO: need replace all dispatcher construction to next call
SmartPtr<CDspClient> CDspClient::makeServiceUser( const QString& sDirUuid )
{
	SmartPtr<CDspClient> pUser = SmartPtr<CDspClient>( new CDspClient(IOSender::Handle()) );
	pUser->getAuthHelper().AuthUserBySelfProcessOwner();
	pUser->setVmDirectoryUuid( sDirUuid );
	return pUser;
}



CDspClient::CDspClient ( const IOSender::Handle& h,
						 const QString& userName,
						 quint32 nFlags ) :
	m_clientHandle(h),
	m_authHelper(userName),
	m_creationTimeStamp( PrlGetTickCount64() ),
	m_state( PVE::UserDisconnected ),
	m_isHostNameCached( false ),
	m_clientType(CT_USUAL_CLIENT),
	m_bIsLocal( false ),
	m_bNonInteractive( false ),
	m_bConfirmationEnabled( CDspDispConfigGuard::confirmationModeEnabled() ),
	m_bAdminAuthWasPassed( false ),
	m_nFlags(nFlags),
	m_pSessionInfo(new SessionInfo)
{
}

CDspClient::CDspClient ( const IOSender::Handle& h ) :
	m_clientHandle(h),
	m_creationTimeStamp( PrlGetTickCount64() ),
	m_state( PVE::UserDisconnected ),
	m_isHostNameCached( false ),
	m_clientType(CT_USUAL_CLIENT),
	m_bIsLocal( false ),
	m_bNonInteractive( false ),
	m_bConfirmationEnabled( CDspDispConfigGuard::confirmationModeEnabled() ),
	m_bAdminAuthWasPassed( false ),
	m_nFlags(0),
	m_pSessionInfo(new SessionInfo)
{
}

CDspClient::~CDspClient( )
{
	CDspService::instance()->emitCleanupOnUserSessionDestroy( getSessionUuid() );
}

CAuthHelper& CDspClient::getAuthHelper ()
{
	return m_authHelper;
}

void CDspClient::setUserState ( PVE::DispUserState st )
{
	m_state = st;
}

PRL_UINT64 CDspClient::getSessionUptimeInSec() const
{
	PRL_UINT64 currTimeStamp = PrlGetTickCount64();
	return ( currTimeStamp - m_creationTimeStamp ) / PrlGetTicksPerSecond();
}

const QString CDspClient::getUserSettingsUuid () const
{
	return m_settingsUuid;
}

void CDspClient::setUserSettings ( const QString& uuid, const QString& userName )
{
	m_settingsUuid = uuid;
	m_userName = userName;
}

const QString CDspClient::getVmDirectoryUuid () const
{
	return m_vmDirUuid;
}

const QStringList CDspClient::getVmDirectoryUuidList () const
{
	QStringList lst;
	lst += m_vmDirUuid;
	lst += CDspVmDirManager::getVzDirectoryUuid();
	lst += CDspVmDirManager::getTemplatesDirectoryUuid();
	return lst;
}

void CDspClient::setVmDirectoryUuid ( const QString& uuid )
{
	m_vmDirUuid = uuid;
}

QString CDspClient::getUserHostAddress () const
{
	// Lock
	QMutexLocker locker( &m_mutex );
	if ( ! m_isHostNameCached ) {
		m_isHostNameCached = true;
		m_hostName = (CDspService::instance()->getIOServer().clientHostName(m_clientHandle));
	}
	return m_hostName;
}

const IOSender::Handle CDspClient::getClientHandle () const
{
	return m_clientHandle;
}

const QString CDspClient::getPrevSessionUuid( ) const
{
	return m_prevSessionUuid;
}

void CDspClient::setPrevSessionUuid( const QString& prevSessionUuid )
{
	m_prevSessionUuid = prevSessionUuid;
}

void CDspClient::storeClientProcessInfo( Q_PID clientPid )
{
#ifndef _LIN_
	Q_UNUSED(clientPid);
	return;
#else
	if( !clientPid )
		return;
	// #PWE-5753 - Store only environment info on linux
	QString envPath = QString("/proc/%1/environ").arg(clientPid);
	QFile f( envPath );
	if( !f.open(QIODevice::ReadOnly) )
	{
		WRITE_TRACE( DBG_DEBUG, "Can't open %lld env", (qint64)clientPid );
		return;
	}
	QList<QByteArray> lstPairs= f.readAll().split(0);
	foreach(QByteArray pair, lstPairs)
	{
		QString str = UTF8_2QSTR( pair.constData() );
		// split by first '='
		QString key = str.section('=', 0,0);
		QString val = str.section('=', 1,-1);
		if( key.isEmpty() )
			continue;
		m_clientEnviron[key] = val;
		LOG_MESSAGE( DBG_DEBUG, "env: '%s' ; key='%s', val='%s'"
			, QSTR2UTF8(str), QSTR2UTF8(key), QSTR2UTF8(val) );
	}
#endif
}

bool CDspClient::getClientEnvinromentVariable( const QString& envVarName, QString& value ) const
{
	if( !m_clientEnviron.contains(envVarName) )
		return false;
	value = m_clientEnviron.value(envVarName);
	return true;
}


IOSendJob::Handle CDspClient::sendPackage(const SmartPtr<IOPackage> &p) const
{
	return (CDspService::instance()->getIOServer().sendPackage(m_clientHandle, p));
}

IOSendJob::Handle CDspClient::sendSimpleResponse( const SmartPtr<IOPackage> &pRequestPkg, PRL_RESULT nRetCode ) const
{
	return (CDspService::instance()->sendSimpleResponseToClient(m_clientHandle, pRequestPkg, nRetCode));
}

IOSendJob::Handle CDspClient::sendResponse(	const CProtoCommandPtr &pResponse, const SmartPtr<IOPackage> &pRequestPkg) const
{
	return (CDspService::instance()->sendResponse(m_clientHandle, pResponse, pRequestPkg));
}

IOSendJob::Handle CDspClient::sendResponseError (
    const CVmEvent& vmEvent,
	const SmartPtr<IOPackage>& p ) const
{
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, vmEvent.getEventCode());
	CProtoCommandDspWsResponse* wsResponse = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	wsResponse->SetError( vmEvent.toString() );
	return sendResponse( pResponse, p );
}

IOSendJob::Handle CDspClient::sendResponseError (
    const CVmEvent* vmEvent,
	const SmartPtr<IOPackage>& p ) const
{
	CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand( p, vmEvent->getEventCode() );
	CProtoCommandDspWsResponse* wsResponse = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
	wsResponse->SetError( vmEvent->toString() );
	return sendResponse( pResponse, p );
}

SmartPtr<IOPackage> CDspClient::MakeQuestionPackage(PRL_RESULT nQuestionId,
													const QList<PRL_RESULT> &lstChoices,
													const QList<CVmEventParameter*>& lstParams,
													const SmartPtr<IOPackage> &pRequestPkg )
{
	QString qsServerUuid = CDspService::instance()->getDispConfigGuard()
								.getDispConfig()->getVmServerIdentification()->getServerUuid();

	CVmEvent cVmQuestionEvent(PET_DSP_EVT_VM_QUESTION,
							  qsServerUuid,
							  PIE_DISPATCHER,
							  nQuestionId,
							  PVE::EventRespRequired);
	// Appending all choices to the event
	for ( int i = 0; i < lstChoices.size(); ++i)
	{
		cVmQuestionEvent.addEventParameter(
			new CVmEventParameter(PVE::UnsignedInt,
								  QString("%1").arg(lstChoices[i]),
								  QString(EVT_PARAM_MESSAGE_CHOICE_n).arg(i)) );
	}
	// Appending all parameters to the event
	for ( int i = 0; i < lstParams.size(); ++i)
		cVmQuestionEvent.addEventParameter( new CVmEventParameter(lstParams[i]) );

	return DispatcherPackage::createInstance( PVE::DspVmEvent, cVmQuestionEvent.toString(), pRequestPkg );
}

QString CDspClient::getUserDefaultVmDirPath() const
{
	QString sUserDefaultVmDirPath;
	{
		CDspLockedPointer<CDispUser> pDispUser =
			CDspService::instance()->getDispConfigGuard().getDispUserByUuid(m_settingsUuid);
		if (pDispUser.getPtr())
			sUserDefaultVmDirPath = pDispUser->getUserWorkspace()->getDefaultVmFolder();
	}

	if (!sUserDefaultVmDirPath.isEmpty())
		return (sUserDefaultVmDirPath);

	{
		CDspLockedPointer<CVmDirectory> pVmDirectory =
			CDspService::instance()->getVmDirManager().getVmDirectory(m_vmDirUuid);
		if (pVmDirectory.getPtr())
			sUserDefaultVmDirPath = pVmDirectory->getDefaultVmFolder();
	}
	return (sUserDefaultVmDirPath);
}

QString CDspClient::getUserName() const
{
	return m_userName;
}

void CDspClient::setClientType(CDspClient::CLIENT_TYPE ct)
{
	m_clientType = ct;
}

CDspClient::CLIENT_TYPE CDspClient::getClientType() const
{
	return m_clientType;
}

bool CDspClient::isLocal()
{
	return m_bIsLocal;
}

void CDspClient::setLocal( bool bLocal )
{
	m_bIsLocal = bLocal;
}

void CDspClient::setNonInteractive( bool bNonInteractive )
{
	m_bNonInteractive = bNonInteractive;
}

bool CDspClient::isNonInteractive() const
{
	return m_bNonInteractive;
}

void CDspClient::setConfirmationMode( bool bConfirmationEnabled )
{
	m_bConfirmationEnabled = bConfirmationEnabled;
}

bool CDspClient::isConfirmationEnabled() const
{
	return m_bConfirmationEnabled;
}

void CDspClient::setAdminAuthWasPassed(bool bAuth)
{
	m_bAdminAuthWasPassed = bAuth;
}

bool CDspClient::isAdminAuthWasPassed() const
{
	return m_bAdminAuthWasPassed;
}

/*****************************************************************************/
