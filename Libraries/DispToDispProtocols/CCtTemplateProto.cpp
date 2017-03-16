/////////////////////////////////////////////////////////////////////////////
///
///	@file CCtTemplateProto.cpp
///
///	Implementation of copy of CT template protocol commands serializer helpers.
///
///	@author krasnov
/// @owner sergeym@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////

#include "CCtTemplateProto.h"

namespace Parallels {

//**************************************************************************
CCtTemplateProto::CCtTemplateProto(Parallels::IDispToDispCommands nCmdIdentifier, quint32 nFlags)
:CDispToDispCommand(nCmdIdentifier, false, nFlags)
{
	SetUnsignedIntParamValue(COPY_CT_TMPL_PROTO_VERSION, EVT_PARAM_COPY_CT_TMPL_PROTO_VERSION);
}

bool CCtTemplateProto::IsValid()
{
	return (CheckWhetherParamPresents(EVT_PARAM_COPY_CT_TMPL_PROTO_VERSION, PVE::UnsignedInt) &&
		CheckWhetherParamPresents(EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt));
}

quint32 CCtTemplateProto::GetVersion()
{
	return (GetUnsignedIntParamValue(EVT_PARAM_COPY_CT_TMPL_PROTO_VERSION));
}

//*************************** Start copy command *************************
CCopyCtTemplateCommand::CCopyCtTemplateCommand(
	const QString &sTmplName,
	const QString &sOsTmplName,
	quint32 nFlags,
	quint32 nReservedFlags
)
:CCtTemplateProto(CopyCtTemplateCmd, nFlags)
{
	SetStringParamValue(sTmplName, EVT_PARAM_COPY_CT_TMPL_TMPL_NAME);
	SetStringParamValue(sOsTmplName, EVT_PARAM_COPY_CT_TMPL_OS_TMPL_NAME);
	SetUnsignedIntParamValue(nReservedFlags, EVT_PARAM_COPY_CT_TMPL_RESERVED_FLAGS);
}

bool CCopyCtTemplateCommand::IsValid()
{
	return (CCtTemplateProto::IsValid() &&
		CheckWhetherParamPresents(EVT_PARAM_COPY_CT_TMPL_TMPL_NAME, PVE::String) &&
		CheckWhetherParamPresents(EVT_PARAM_COPY_CT_TMPL_RESERVED_FLAGS, PVE::UnsignedInt));
}

QString CCopyCtTemplateCommand::GetTmplName()
{
	return GetStringParamValue(EVT_PARAM_COPY_CT_TMPL_TMPL_NAME);
}

QString CCopyCtTemplateCommand::GetOsTmplName()
{
	return GetStringParamValue(EVT_PARAM_COPY_CT_TMPL_OS_TMPL_NAME);
}

quint32 CCopyCtTemplateCommand::GetReservedFlags()
{
	return GetUnsignedIntParamValue(EVT_PARAM_COPY_CT_TMPL_RESERVED_FLAGS);
}

}//namespace Parallels
