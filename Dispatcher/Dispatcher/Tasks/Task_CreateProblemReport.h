///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateProblemReport.h
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

#ifndef __Task_CreateProblemReport_H_
#define __Task_CreateProblemReport_H_

#include <QObject>
#include <QTimer>
#include <QNetworkProxy>
#include "CDspTaskHelper.h"

class CProblemReportPostWrap;
class CPackedProblemReport;
class QAuthenticator;


class Task_CreateProblemReport : public  CDspTaskHelper
{
    Q_OBJECT
public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	* @param
	*/
	Task_CreateProblemReport( SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &pRequestPkg );

	/**
    * Class destructor
	*/
    virtual ~Task_CreateProblemReport();

    /**
    * Cancels running operation
    * @param client requested the task
    * @param incoming package
    */
    virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage>&);

private slots:
    /**
     * Processing incoming requests from the ProblemReport post wrap
     */
    void onSendingCompleted(PRL_RESULT, QStringList);
    void onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);
    void onStartReportSending();

protected:
	/**
	* Task body
	* @return result code of task completion
	*/
	virtual PRL_RESULT run_body();

	/**
	* Makes all necessary actions (sending answer and etc.) on task finish
	*/
	virtual void finalizeTask();

private:
    PRL_RESULT processReportSending();

private:
	SmartPtr<CDspClient> m_pFakeSession;
    SmartPtr<CProblemReportPostWrap> m_pPRSender;
    SmartPtr<QTimer> m_starter;

    QString m_reportId;
    QMutex m_mtx; // mutex for m_pPRSender
};

#endif //__Task_CreateProblemReport_H_
