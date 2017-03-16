////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// @file
///	DspMonitor.h
///
/// @brief
///	Implementation of the class CDspMonitor
///
/// @brief
///	This class implements request processing from Server Management Console
///
/// @author sergeyt
///	SergeyT
///
/// @date
///	2006-12-01
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef H_CDspMonitor_HHH
#define H_CDspMonitor_HHH

#include <QString>
#include <QPair>
#include <QList>

#include "CDspClient.h"

class CDspMonitor
{
public:
   CDspMonitor ();
   ~CDspMonitor ();

public:

      //note: probably incorrect show VM (show only active)
   void GetDispRTInfo ( SmartPtr<CDspClient>&, const SmartPtr<IOPackage>& );

   void ShutdownDispatcher ( SmartPtr<CDspClient>&, const SmartPtr<IOPackage>& );

   // FIXME TODO: Need add mech to list tasks by users
   void ForceCancelCommandOfUser ( SmartPtr<CDspClient>&,
								   const SmartPtr<IOPackage>&,
								   const QString& taskUuid);

      // note: used functinality CDspUserHelper::processUserLogoff()
      // client must be subscribed to notifications to get them.
	void ForceDisconnectUser ( SmartPtr<CDspClient>&,
							   const SmartPtr<IOPackage>& );
	void ForceDisconnectAllUsers ( SmartPtr<CDspClient>&,
								   const SmartPtr<IOPackage>& );

	void SendShutdownCompleteResponses(PRL_UINT32 nSendTimeoutInMSecs);
private:
	bool CheckUserPermission ( SmartPtr<CDspClient>& pUser);
	void StoreShutdownRequest( SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p );

private:
	QMutex m_listMutex;
	typedef QPair<IOSender::Handle, SmartPtr<IOPackage> > ShutdownRequestInfo;
	QList< ShutdownRequestInfo > m_shutdownRequests;

};

#endif //H_CDspMonitor_HHH
