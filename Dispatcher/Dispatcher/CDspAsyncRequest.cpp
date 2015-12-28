///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspAsyncRequest.cpp
///
/// Base classes for handling requests to dispatcher.
///
/// @owner artemk@
/// @author dbrylev@
///
/// Copyright (c) 2013-2015 Parallels IP Holdings GmbH
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

#include "CDspAsyncRequest.h"
#include "CDspClient.h"

#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>

#include <QElapsedTimer>



//
// CDspAsyncRequest methods
//

CDspAsyncRequest::CDspAsyncRequest()
{
	bool bOk = connect(this, SIGNAL(sigInvokeRun()),
					   this, SLOT(invokeRun()),
					   Qt::QueuedConnection);
	PRL_ASSERT(bOk);
}

void CDspAsyncRequest::complete()
{
	deleteLater();
}

void CDspAsyncRequest::runAsync(CDspAsyncRequest * pRequest, QThread * pRunner)
{
	Q_ASSERT(pRequest);
	Q_ASSERT(pRunner);

	// Request should be bound to the runner thread,
	// to ensure it has event loop.
	if (pRequest->thread() != pRunner)
	{
		// request must be passed to runner thread from thread, where it has been created,
		// following scenario is NOT supported:
		//   thread 1: create request R.
		//	 thread 2: invoke runAsync with R.
		//   thread 1 != thread 2 != runner thread.
		PRL_ASSERT(pRequest->thread() == QThread::currentThread());
		pRequest->moveToThread(pRunner);
	}

	// Request should be performed in context of the runner thread.
	if (QThread::currentThread() != pRequest->thread())
	{
		// invoke from runner thread.
		emit pRequest->sigInvokeRun();
		return;
	}

	pRequest->invokeRun();
}

void CDspAsyncRequest::invokeRun()
{
	static const PRL_UINT64 kRunTimeout = 20000ULL; // 20 secs - sergeyt insists!


	// We are in the right thread to go.
	QElapsedTimer runTimer;
	runTimer.start();

	run();

	if (runTimer.hasExpired(kRunTimeout))
		WRITE_TRACE(DBG_FATAL, "Request took too much time in the runner thread: %llu",
					runTimer.elapsed());
}


//
// CDspClientRequest methods
//

CDspClientRequest::CDspClientRequest(SmartPtr<CDspClient> pSender,
									 SmartPtr<IOPackage> pMsg)
	: m_pSender(pSender)
	, m_pMsg(pMsg)
{}

CDspClientRequest::~CDspClientRequest()
{}

void CDspClientRequest::respondToSender(PRL_RESULT nResult)
{
	if (m_pSender && m_pMsg)
		m_pSender->sendSimpleResponse(m_pMsg, nResult);
}

void CDspClientRequest::completeClientRequest(PRL_RESULT nResult)
{
	respondToSender(nResult);
	CDspAsyncRequest::complete();
}
