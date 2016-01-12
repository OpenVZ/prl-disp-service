////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Task_DropSuspendedState.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	ilya@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_DropSuspendedState_H_
#define __Task_DropSuspendedState_H_

#include "Dispatcher/Dispatcher/CDspTaskHelper.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

class Task_DropSuspendedState
	: public  CDspTaskHelper
{
	Q_OBJECT

public:

	Task_DropSuspendedState ( SmartPtr<CDspClient>&,
					const SmartPtr<IOPackage>&,
					const QString& vm_config);
	~Task_DropSuspendedState();

	virtual QString getVmUuid();

protected:

	virtual PRL_RESULT			prepareTask();

	virtual void				finalizeTask();

	virtual PRL_RESULT run_body();

private:

	void setTaskParameters(	const QString& vm_uuid );

	// this function correctly removed files from list and fill list that can't remove
	bool RemoveStateFiles(const QStringList & strImagesList,QStringList & lstNotRemoved);

	// send 'stopped' event
	void sendEventStopped();

private:

	SmartPtr<CVmConfiguration> m_pVmConfig;

	bool m_flgExclusiveOperationWasRegistred;
};



#endif //__Task_DropSuspendedState_H_
