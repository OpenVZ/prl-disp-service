///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_DispToDispConnHelper.h
///
/// Common functions for backup tasks
///
/// @author krasnov@
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_DispToDispConnHelper_H_
#define __Task_DispToDispConnHelper_H_

#include <QString>

#include "CDspClient.h"
#include "Libraries/IOService/src/IOCommunication/IOClient.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"
#include "Dispatcher/Dispatcher/CDspDispConnection.h"

#define TARGET_DISPATCHER_TIMEOUT_COUNTS 60

class Task_DispToDispConnHelper
{
public:
	Task_DispToDispConnHelper(CVmEvent *pEvent);
	virtual ~Task_DispToDispConnHelper();

protected:
	bool isConnected();
	PRL_RESULT Connect(
			QString sServerHostname,
			quint32 nServerPort,
			QString sServerSessionUuid,
			QString sUser,
			QString sPassword,
			quint32 nFlags);
	void Disconnect();
	PRL_RESULT SendPkg(const SmartPtr<IOPackage> &package);
	PRL_RESULT SendReqAndWaitReply(const SmartPtr<IOPackage> &package);
	PRL_RESULT SendReqAndWaitReply(const SmartPtr<IOPackage> &package,
			SmartPtr<IOPackage> &pReply);
	PRL_RESULT SendReqAndWaitReply(const SmartPtr<IOPackage> &package,
			SmartPtr<IOPackage> &pReply, IOSendJob::Handle &hJob);

	virtual bool isCancelled() { return true; }
	SmartPtr<IOClient> getIoClient() { return m_pIoClient; }

protected:
	SmartPtr<IOClient> m_pIoClient;
	CVmEvent *m_pEvent;
	quint32 m_nTimeout;
};

#endif //__Task_DispToDispConnHelper_H_
