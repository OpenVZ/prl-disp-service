///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspAsyncRequest.h
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

#pragma once

#include <QObject>
#include <prlsdk/PrlTypes.h>
#include <Libraries/Std/SmartPtr.h>

class CDspClient;
namespace IOService { class IOPackage; }



/**
 * Base request interface.
 */
class CDspAsyncRequest : public QObject
{
	Q_OBJECT
public:
	virtual ~CDspAsyncRequest() {}
	/**
	 * Execute request in the specific thread.
	 */
	static void runAsync(CDspAsyncRequest * pRequest, QThread * pRunner);
protected slots:
	void invokeRun();
signals:
	void sigInvokeRun();
protected:
	CDspAsyncRequest();
	/**
	 * Performs the request. This method is starting point for request processing.
	 */
	virtual void run() = 0;

	/**
	 * Complete the request. This method ends async request execution and
	 * shedules request for deletion. So, request must not do any work after
	 * this method call.
	 */
	void complete();
};



/**
 * Request from dispatcher client.
 */
class CDspClientRequest : public CDspAsyncRequest
{
	Q_OBJECT
public:
	CDspClientRequest(SmartPtr<CDspClient> pSender,
					  SmartPtr<IOService::IOPackage> pMsg);
	virtual ~CDspClientRequest();
protected:
	/**
	 * Send response to client and complete the request.
	 */
	void completeClientRequest(PRL_RESULT nResult);
	/**
	 * Send simple response with error code only to the request sender.
	 */
	void respondToSender(PRL_RESULT nResult);
protected:
	SmartPtr<CDspClient>			m_pSender;
	SmartPtr<IOService::IOPackage>	m_pMsg;
};
