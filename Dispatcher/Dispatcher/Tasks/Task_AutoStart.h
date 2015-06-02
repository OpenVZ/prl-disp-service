///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_AutoStart.h
///
/// Dispatcher task for
///
/// @author sergeyt
/// @owner sergeym@
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

#ifndef __Task_AutoStart_H_
#define __Task_AutoStart_H_

#include "CDspTaskHelper.h"


class Task_AutoStart : public  CDspTaskHelper
{
	Q_OBJECT

public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	*/
	Task_AutoStart(SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg);

	/**
	* Class destructor
	*/
	virtual ~Task_AutoStart() {}

protected:
	/**
	* Task body
	* @return result code of task completion
	*/
	virtual PRL_RESULT run_body();
private:
	PRL_RESULT runTask(SmartPtr<IOPackage> p);
	PRL_RESULT startVms();
	void startCts();
};

#endif //__Task_AutoStart_H_
