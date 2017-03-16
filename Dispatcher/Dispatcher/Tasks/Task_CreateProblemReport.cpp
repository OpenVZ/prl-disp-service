///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateProblemReport.cpp
///
/// Dispatcher task for problem report collection
///
/// @author artemr@
///
/// Copyright (c) 2009-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#include <QAuthenticator>
#include <QMutexLocker>
#include "Task_CreateProblemReport.h"

#include "CDspClient.h"
#include "CDspProblemReportHelper.h"
#include "Stat/CDspStatCollectingThread.h"

#include "Libraries/ProblemReportUtils/CProblemReportPostWrap.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>


void Task_CreateProblemReport::onSendingCompleted(PRL_RESULT result, QStringList params)
{
    if (!PRL_FAILED(result) && params.size() > 1)
        m_reportId = params[1].replace('=', ' ');

    QThread::exit(result);
}

void Task_CreateProblemReport::onProxyAuthenticationRequired(
    const QNetworkProxy&, QAuthenticator* auth)
{
    // TODO XXX FIXME Should be implemented after #PM-7235
    // see also Task_AutoStatisticReport
    Q_UNUSED(auth);
/*
    QString user;
    QString pass;

    if (!m_proxyHelper.getNextCredentials(user, pass))
        return;

    auth->setUser(user);
    auth->setPassword(pass);
*/
}

void Task_CreateProblemReport::onStartReportSending()
{
    // Call report's sending asynchroneously
    m_pPRSender->start();
}


Task_CreateProblemReport::Task_CreateProblemReport(SmartPtr<CDspClient> &pUser,
                                                   const SmartPtr<IOPackage> &pRequestPkg) :
    CDspTaskHelper(pUser, pRequestPkg)
{
    // To process events after launching event loop we need to call moveToThread!
    // The reason is that the current context is the dispatcher thread but the
    // event loop will be launched within new thread (see run_body). The call below
    // moves event bypassing from the disp event loop to the local "this" event loop.
    moveToThread(this);

	m_pFakeSession = CDspClient::makeServiceUser();
    m_starter.reset(new QTimer());

    m_starter->moveToThread(this);

    bool bConnected = true;
    bConnected &= QObject::connect(
        m_starter.getImpl(), SIGNAL(timeout()), this, SLOT(onStartReportSending()));
    Q_ASSERT(bConnected);
}

Task_CreateProblemReport::~Task_CreateProblemReport()
{}

PRL_RESULT Task_CreateProblemReport::run_body()
{
	CDspStatCollectingThread::SubscribeToHostStatistics(m_pFakeSession);

    if (getRequestPackage()->header.type == PVE::DspCmdSendProblemReport)
        return processReportSending();

    CDspProblemReportHelper::getProblemReport(getClient(), getRequestPackage());
    return PRL_ERR_SUCCESS;
}

void Task_CreateProblemReport::finalizeTask()
{
	CDspStatCollectingThread::UnsubscribeFromHostStatistics(m_pFakeSession);
	// do nothing - all answers was posted from run_body or from CDspRequestToVmHandler
}

PRL_RESULT Task_CreateProblemReport::processReportSending()
{
    SmartPtr<CPackedProblemReport> pPR = CDspProblemReportHelper::getProblemReportObj(
        getClient(), getRequestPackage());
    {
        QMutexLocker lk(&m_mtx);
        PRL_ASSERT(!m_pPRSender);

        m_pPRSender.reset(new CProblemReportPostWrap);
    }

    m_pPRSender->setIgnoreCertifErrors(false);
    m_pPRSender->initProblemReport(pPR);

    bool bConnected = true;
    bConnected &= QObject::connect(m_pPRSender.getImpl(), SIGNAL(complete(PRL_RESULT, QStringList)),
                                  this, SLOT(onSendingCompleted(PRL_RESULT, QStringList)));
    Q_ASSERT(bConnected);
    bConnected &= QObject::connect(m_pPRSender.getImpl(),
            SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)), this,
            SLOT(onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
    Q_ASSERT(bConnected);

    // Setting up the first event when we get into event loop
    m_starter->setSingleShot(true);
    m_starter->start(0);

    // Here we are blocking until any of onXxx callbacks are called
    int retCode = QThread::exec();
    {
        QMutexLocker lk(&m_mtx);
        m_pPRSender.reset(0);
    }

	pPR->setCleanupTempDir(true);

    if (PRL_FAILED(retCode))
    {
        getClient()->sendSimpleResponse(getRequestPackage(), retCode);
        WRITE_TRACE(DBG_WARNING, "Failed to send problem report from dispatcher");
        setLastErrorCode(retCode);
        return retCode;
    }

    CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(
        getRequestPackage(), PRL_ERR_SUCCESS);
    CProtoCommandDspWsResponse *pResp =
            CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
    PRL_ASSERT(pResp);
    pResp->AddStandardParam(m_reportId);
    getClient()->sendResponse(pCmd, getRequestPackage());
    WRITE_TRACE(DBG_DEBUG, "Report was sent by server, id=%s", QSTR2UTF8(m_reportId));
    return retCode;
}

void Task_CreateProblemReport::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
    {
        QMutexLocker lk(&m_mtx);
        if (0 != m_pPRSender.getImpl())
            m_pPRSender->cancel();
    }

    CancelOperationSupport::cancelOperation(pUser, p);

    QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}
