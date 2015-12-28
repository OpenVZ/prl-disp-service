////////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzStateMonitor.h
///
/// @author igor
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
////////////////////////////////////////////////////////////////////////////////
#ifndef __Task_VzStateMonitor_H_

#include "CDspClient.h"
#include <prlcommon/Std/SmartPtr.h>
#include "CDspTaskHelper.h"
#include "Libraries/Virtuozzo/CVzHelper.h"

class Task_VzStateMonitor : public CDspTaskHelper
{
public:
	Task_VzStateMonitor(const SmartPtr<CDspClient> &user, const SmartPtr<IOPackage> &p);
	virtual ~Task_VzStateMonitor();
	static void stateEventHandler(void *obj, const QString &uuid, int state);
	void processChangeCtState(QString uuid, VIRTUAL_MACHINE_STATE vm_state);
	void sendState(const QString &uuid, int state);
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);
private:
	void processRegisterEvt(const QString &uuid);
	void processUnregisterEvt(const QString &uuid);
	void processConfigChangedEvt(const QString &uuid);
	void processNetConfigChangedEvt(const QString &sUuid);

protected:
	PRL_RESULT startMonitor();
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
private:
	CVzStateMonitor m_monitor;
	QString m_sVzDirUuid;

};

#endif //__Task_VzStateMonitor_H_
