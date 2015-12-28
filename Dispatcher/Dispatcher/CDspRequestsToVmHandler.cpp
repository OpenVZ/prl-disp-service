///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRequestsToVmHandler
///
/// manager of posted requests to vm this object must be created in main application thread
///
/// @author Artemr
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

#include "CDspRequestsToVmHandler.h"

#include <QMutexLocker>
#include <QTimer>

#include "CDspService.h"
#include "CDspClient.h"
#include "CDspClientManager.h"
#include "CDspRouter.h"
#include "CDspProblemReportHelper.h"

#define TIME_OUT_INTERVAL		1000*60		// 1 min
#define EXT_TIME_OUT_INTERVAL	1000*60*3	// 3 min
#define PERF_COUNTERS_TIMEOUT	200		// msec
#define PERF_COUNTERS_TICKS		10		// 2 sec

#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PerfCount/PerfLib/PerfCountersOut.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"


CDspRequestsToVmHandler::CDspRequestsToVmHandler(void)
: m_HashSyncMutex(QMutex::Recursive)
{
	// connect signals to run operations in main dispatcher threads
	// it need to run and stop timers

	bool bConnected = connect(this,SIGNAL(toCompleteRequest( QString )),
		SLOT(onCompleteRequest( QString )) );
	PRL_ASSERT(bConnected);

	bConnected = connect(this,SIGNAL(startTimeout( QString , int )),
		SLOT(onStartTimeout(QString , int )));
	PRL_ASSERT(bConnected);

	Q_UNUSED(bConnected);
}

CDspRequestsToVmHandler::~CDspRequestsToVmHandler(void)
{
	// clear timers hash
	QHash< QTimer * , SmartPtr<IOPackage> /*Request*/ >::iterator  it = m_hashTimers.begin();
	while( it != m_hashTimers.end() )
	{
		QTimer * pTimer  = it.key();
		it = m_hashTimers.erase(it);
		// stop and delete timer
		pTimer->stop();
		delete pTimer;
		pTimer = NULL;
	}
	// clear Hash, SmartPtr objects deleted automatic
	m_hashRequests.clear();
}

/**
* @function addPostedRequest(SmartPtr<IOPackage> pRequest,
* SmartPtr<CDspClient> pClient);
*
* @brief adde request pointer to timout queque.
*
* @return none
*
* @author Artemr
*/
void CDspRequestsToVmHandler::addPostedRequest(SmartPtr<IOPackage> pRequest,
					  SmartPtr<CDspClient> pClient)
{
	QMutexLocker locker(&m_HashSyncMutex);
	switch(pRequest->header.type)
	{
		case PVE::DspCmdVmGetProblemReport:
		case PVE::DspCmdVmGetPackedProblemReport:
		{
			// add to hash request uuid and client uuid
			m_hashRequests.insert(pRequest,
										pClient->getClientHandle());

			// add to hash request and pref. counters output object
			SmartPtr<PerfCountersOut> pPerfCountersOut = SmartPtr<PerfCountersOut>(new PerfCountersOut());
			pPerfCountersOut->Fill(false);
			m_hashPerfCounters.insert(pRequest, pPerfCountersOut);

			// emit signal to main thread to start timer!

			int iTimeout = TIME_OUT_INTERVAL;

			CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pRequest );
			if ( ! CDspService::isServerModePSBM() && cmd->IsValid() )
			{
				CAuthHelperImpersonateWrapperPtr pImpersonate;
				if ( pClient.isValid() )
					pImpersonate = CAuthHelperImpersonateWrapper::create( &pClient->getAuthHelper() );

				CDspLockedPointer<CVmDirectoryItem>
					pVmDirItem = CDspService::instance()->getVmDirManager()
						.getVmDirItemByUuid( pClient->getVmDirectoryUuid(), cmd->GetVmUuid() );
				if (pVmDirItem && CFileHelper::isRemotePath(pVmDirItem->getVmHome()))
				{
					iTimeout = EXT_TIME_OUT_INTERVAL;
				}
			}

			emit startTimeout(Uuid::toString(pRequest->header.uuid), iTimeout);
			break;
		}
		default:
			break;
	}
}

/**
* @function markRequestComplete(SmartPtr<IOPackage> pRequest)
*
* @brief mark request complete after it recieved from vm.
* @param SmartPtr<IOPackage> pResponse - responce package from vm
*
* @return none
*
* @author Artemr
*/
void CDspRequestsToVmHandler::markRequestComplete(SmartPtr<IOPackage> pResponse)
{
	// compare parent and current request uuids for PVE::DspCmdVmGetProblemReport
	emit toCompleteRequest(Uuid::toString(pResponse->header.parentUuid));
}

QString getAbsolutePerfCounters( SmartPtr<PerfCountersOut> pPerfCountersOut )
{
	if( !pPerfCountersOut )
		return QString();

	pPerfCountersOut->PrintCustomTextInOutput(
		"\n\n########### Absolute performance counters values ############\n\n" );
	pPerfCountersOut->Dump( "", "", false, false, false );


	return QString::fromStdString( pPerfCountersOut->GetOutputString() );

}

QString CDspRequestsToVmHandler::getPerfCountersInfo(QString qsRequestUuid)
{
	QMutexLocker locker(&m_HashSyncMutex);

	if (m_hashPerfCountersInfo.contains(qsRequestUuid))
		return m_hashPerfCountersInfo.value(qsRequestUuid);

	SmartPtr<PerfCountersOut> pPerfCountersOut = SmartPtr<PerfCountersOut>(new PerfCountersOut());
	pPerfCountersOut->Fill( false );
	return getAbsolutePerfCounters( pPerfCountersOut );
}

/**
* @function onRequestComplete( const QString & strParentRequestUuid )
*
* @brief Routine to clean hashes end stop timers.
*
* @return none
*
* @author Artemr
*/
void CDspRequestsToVmHandler::onCompleteRequest( QString strParentRequestUuid )
{
	QMutexLocker locker(&m_HashSyncMutex);

	// find timer , stop and delete it!
	SmartPtr<IOPackage> pHashRequest;
	QHash< QTimer * , SmartPtr<IOPackage> /*Request*/ >::iterator  it = m_hashTimers.begin();
	while( it != m_hashTimers.end() )
	{
		pHashRequest = it.value();
		// compare parent and current request uuids for PVE::DspCmdVmGetProblemReport
		if (Uuid::toString(pHashRequest->header.uuid) == strParentRequestUuid )
		{
			QTimer * pTimer = it.key();
			it = m_hashTimers.erase(it);
			// stop and delete timer
			pTimer->stop();
			delete pTimer;
			pTimer = NULL;
			break;
		}
		else
			++it;
	}
	// remove request from hash
	m_hashRequests.remove(pHashRequest);
	// remove perf. counters info
	m_hashPerfCountersInfo.remove(strParentRequestUuid);
}

/**
* @function onStartTimeout(const QString& strRequestUuid)
*
* @brief Routine to fill hashes end start timer.
*
* @return none
*
* @author Artemr
*/
void CDspRequestsToVmHandler::onStartTimeout( QString strRequestUuid, int iTimeout)
{
	// create timer
	QTimer * pTimer = new QTimer;
	pTimer->setSingleShot(true);

	QMutexLocker locker(&m_HashSyncMutex);
	SmartPtr<IOPackage> pRequest;
	bool bFind = false;
	QHashIterator< SmartPtr<IOPackage> /*Request*/,QString/*client session uuid*/ >  it(m_hashRequests);
	// find request and create item to timers hash
	while( it.hasNext() )
	{
		pRequest = it.next().key();
		if (Uuid::toString(pRequest->header.uuid) == strRequestUuid)
		{
			m_hashTimers.insert(pTimer,pRequest);
			bFind = true;
			// start perf. counters timer
			SmartPtr<PerfCountersOut> pPerfCountersOut = m_hashPerfCounters.value(pRequest);
			if (pPerfCountersOut.isValid())
			{
				int nTimerId = startTimer(PERF_COUNTERS_TIMEOUT);
				pPerfCountersOut->SetPrivateData(nTimerId);
			}
			break;
		}

	}

	if (!bFind)
		return;

	// connect timout
	bool bConnected = connect( pTimer, SIGNAL(timeout()),
		this, SLOT( onTimeoutReached() ) );
	PRL_ASSERT(bConnected);
	Q_UNUSED(bConnected);

	pTimer->start(iTimeout);

}

void CDspRequestsToVmHandler::timerEvent(QTimerEvent* te)
{
	QMutexLocker locker(&m_HashSyncMutex);

	QHashIterator< SmartPtr<IOPackage> /*Request*/, SmartPtr<PerfCountersOut> > it(m_hashPerfCounters);
	while( it.hasNext() )
	{
		SmartPtr<IOPackage> pRequest = it.next().key();
		SmartPtr<PerfCountersOut> pPerfCountersOut = m_hashPerfCounters.value(pRequest);
		if (pPerfCountersOut.isValid() && pPerfCountersOut->GetPrivateData() == te->timerId())
		{
			if (pPerfCountersOut->GetCallDumpCount() == PERF_COUNTERS_TICKS)
			{
				if (m_hashRequests.contains(pRequest))
				{
					SmartPtr<CDspClient> pClient =
						CDspService::instance()->getClientManager().getUserSession(m_hashRequests[pRequest]);
					if (pClient.isValid())
					{
						m_hashPerfCountersInfo.insert( Uuid::toString( pRequest->header.uuid ),
													  getAbsolutePerfCounters( pPerfCountersOut ) );

						CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pRequest );
						if ( !cmd->IsValid() ) {
							// Send error
							pClient->sendSimpleResponse( pRequest, PRL_ERR_FAILURE );
						}
						else
						{
							SmartPtr<CDspVm> pVm =
								CDspVm::GetVmInstanceByUuid(cmd->GetVmUuid(), pClient->getVmDirectoryUuid());
							if (pVm.isValid())
							{
								pVm->sendProblemReport(pClient, pRequest);
							}
							else
							{
								onTimeoutReached();
							}
						}
					}
				}

				m_hashPerfCounters.remove(pRequest);
				killTimer(te->timerId());
			}
			else
			{
				pPerfCountersOut->Dump("", "", true, true, false);
			}

			return;
		}
	}
}

/**
* @function onStartTimeout(const QString& strRequestUuid)
*
* @brief Routine when timeout reached - it posted to client timeout package.
*
* @return none
*
* @author Artemr
*/
void CDspRequestsToVmHandler::onTimeoutReached()
{
	QTimer * pTimer = dynamic_cast<QTimer*>(sender());
	if( !pTimer )
		return;
	// get request uuid
	QMutexLocker locker(&m_HashSyncMutex);
	if( m_hashTimers.contains(pTimer) )
	{
		// clear hash
		SmartPtr<IOPackage> pRequest = m_hashTimers[pTimer];
		m_hashTimers.remove(pTimer);
		// stop and delete timer
		pTimer->stop();
		delete pTimer;
		pTimer = NULL;
		// post reply to client with timeout
		postTimeoutRequest(pRequest);
		// remove perf. counters info
		m_hashPerfCountersInfo.remove(Uuid::toString(pRequest->header.parentUuid));
	}
}

/**
* @function onStartTimeout(const QString& strRequestUuid)
*
* @brief Routine to post to client timeout package and clean request hashes.
*
* @return none
*
* @author Artemr
*/
void CDspRequestsToVmHandler::postTimeoutRequest(SmartPtr<IOPackage> pRequest)
{
	switch(pRequest->header.type)
	{
	case PVE::DspCmdVmGetProblemReport:
	case PVE::DspCmdVmGetPackedProblemReport:
		{
			if (!m_hashRequests.contains(pRequest))
				return;

			SmartPtr<CDspClient> pClient =
				CDspService::instance()->getClientManager().getUserSession(m_hashRequests[pRequest]);

			if (pClient.isValid())
			{
				CDspProblemReportHelper::getProblemReport( pClient, pRequest,true );
			}
			break;
		}
	default:
		break;
	}
	m_hashRequests.remove(pRequest);
}
