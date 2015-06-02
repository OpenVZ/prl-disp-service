////////////////////////////////////////////////////////////////////////////////
///
/// @file
///	Server_Sentry.cpp
///
/// @brief
///	Implementation of the class Server_Sentry
///
/// @author myakhin
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

#include "Server_Sentry.h"
#include "CDspService.h"


PRL_RESULT Server_Sentry::isCommandAllowed( SmartPtr<CDspClient>& ,
											const SmartPtr<IOPackage>& p,
											CVmEvent& outErrParams )
{
	outErrParams.setEventCode(PRL_ERR_SUCCESS);

	if ( CDspService::isServerMode() )
		return outErrParams.getEventCode();

	switch(p->header.type)
	{
	case PVE::DspCmdDirVmMigrate:

	case PVE::DspCmdGetBackupTree:
	case PVE::DspCmdCreateVmBackup:
	case PVE::DspCmdRestoreVmBackup:
	case PVE::DspCmdRemoveVmBackup:

	case PVE::DspCmdStartClusterService:
	case PVE::DspCmdStopClusterService:

	case PVE::DspCmdUpdateNetworkClassesConfig:
	case PVE::DspCmdGetNetworkClassesConfig:

	case PVE::DspCmdUpdateNetworkShapingConfig:
	case PVE::DspCmdGetNetworkShapingConfig:
	case PVE::DspCmdRestartNetworkShaping:

	case PVE::DspCmdGetCtTemplateList:
	case PVE::DspCmdRemoveCtTemplate:
	case PVE::DspCmdCopyCtTemplate:

	case PVE::DspCmdAddIPPrivateNetwork:
	case PVE::DspCmdRemoveIPPrivateNetwork:
	case PVE::DspCmdUpdateIPPrivateNetwork:
	case PVE::DspCmdGetIPPrivateNetworksList:

		outErrParams.setEventCode(PRL_ERR_UNIMPLEMENTED);
		break;

	default:
		;
	}

	return outErrParams.getEventCode();
}
