///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_PrepareForHibernate.h
///
/// Dispatcher task for configuration generic PCI devices.
///
/// @author myakhin@
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

#ifndef __Task_PrepareForHibernate_H_
#define __Task_PrepareForHibernate_H_


#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"


class Task_PrepareForHibernate : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_PrepareForHibernate(SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &pRequestPkg);

	static bool isTaskRunning();

	PRL_RESULT prepareForHostSuspend( bool bAllowCancelHostSuspend );

protected:

	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	QList<CVmIdent> getListVmWithVtd();
	PRL_RESULT prepareHypervisor();
	void sendWarningBeforeVmStop( const CVmIdent& vmIdent );

private:
	struct JobInfo
	{
		IOSendJob::Handle hJob;
		PVE::IDispatcherCommands cmd;
		bool	bSuspendTimeoutElapsed;

		JobInfo( IOSendJob::Handle hJob, PVE::IDispatcherCommands cmd )
			:hJob(hJob), cmd(cmd), bSuspendTimeoutElapsed( false )
		{}
	};


private:
	static bool m_bIsRunning;
};


#endif	// __Task_PrepareForHibernate_H_
