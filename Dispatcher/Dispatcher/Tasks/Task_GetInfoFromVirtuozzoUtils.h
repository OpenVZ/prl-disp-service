////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Task_GetInfoFromVirtuozzoUtils.h
///
/// @brief
///	Definition of the class Task_GetInfoFromVirtuozzoUtils
///
/// @brief
///	This class implements Virtuozzo Utitlites functions Information requests
///
/// @author sergeyt
///	Artemr@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_GetInfoFromVirtuozzoUtils_H_
#define __Task_GetInfoFromVirtuozzoUtils_H_

#include "CDspTaskHelper.h"
class CVmHardDisk;
class Task_GetInfoFromVirtuozzoUtils : public  CDspTaskHelper
{
public:
	Task_GetInfoFromVirtuozzoUtils(SmartPtr<CDspClient>&,
				  const SmartPtr<IOPackage>&);

	virtual ~Task_GetInfoFromVirtuozzoUtils();
	static PRL_RESULT GetDiskImageInformation(/*in */const QString & strPathToFile,
			/* out */CVmHardDisk & cHardDisk, SmartPtr<CDspClient> pUserSession);

protected:
	virtual PRL_RESULT run_body() {return PRL_ERR_OPERATION_FAILED;}

private:
};



#endif //__Task_GetInfoFromVirtuozzoUtils_H_
