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
///	Mixin_CreateVmSupport.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	sergeyt@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Mixin_CreateVmSupport_H_
#define __Mixin_CreateVmSupport_H_

//#include "Libraries/Std/SmartPtr.h"
//#include "SDK/Include/PrlErrors.h"
#include "CDspTaskHelper.h"

class Mixin_CreateVmSupport
{
public:

	/**
	* Set Owner and Default Permissions to VM files
	* @param pointer to the user session object performing operation
	* @param path to VM home directory
	* @param sign whether permissions setting for creating VM or registering
	* @return return code specifying whether operation was completed successfully or not
	*/
	PRL_RESULT setDefaultVmPermissions(
		SmartPtr<CDspClient> pUser,
		QString vmPathToConfig,
		bool bKeepOthersPermissions
	);
};


#endif //__Mixin_CreateVmSupport_H_
