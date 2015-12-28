///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRequestsToVmHandler
///
/// manager of posted requests to vm
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

#ifndef CDSP_REQUESTS_TO_VM_MANAGER_H
#define CDSP_REQUESTS_TO_VM_MANAGER_H

#include <QObject>
#include <QHash>
#include <QMutex>

class QTimer;
class CDspClient;
class PerfCountersOut;


#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include "ParallelsNamespace.h"

using namespace IOService;

class CDspRequestsToVmHandler :
	public QObject
{
	Q_OBJECT

public:
	CDspRequestsToVmHandler(void);
	virtual ~CDspRequestsToVmHandler(void);

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
	void addPostedRequest(SmartPtr<IOPackage> pRequest,
			SmartPtr<CDspClient> pClient);

	void markRequestComplete(SmartPtr<IOPackage> pResponse);

	QString getPerfCountersInfo(QString qsRequestUuid);

signals:
	void toCompleteRequest( QString );
	void startTimeout( QString , int );

public slots:
	void onStartTimeout( QString , int );
	void onTimeoutReached();
	void onCompleteRequest( QString/*strParentRequestUuid*/);

private:
	void postTimeoutRequest(SmartPtr<IOPackage> pRequest);
	virtual void timerEvent(QTimerEvent* te);

private:
	QHash<SmartPtr<IOPackage> /*Request*/,QString/*client session uuid*/> m_hashRequests;
	QHash<QTimer* /*timer*/,SmartPtr<IOPackage>/*request */> m_hashTimers;
	QHash<SmartPtr<IOPackage> /*Request*/, SmartPtr<PerfCountersOut> > m_hashPerfCounters;
	QHash<QString /*parent request uuid*/, QString /*perf. counters info*/ > m_hashPerfCountersInfo;
	QMutex m_HashSyncMutex;
};

#endif //CDSP_REQUESTS_TO_VM_MANAGER_H
