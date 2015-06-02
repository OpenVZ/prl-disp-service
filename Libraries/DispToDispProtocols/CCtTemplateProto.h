/////////////////////////////////////////////////////////////////////////////
///
///	@file CCtTemplateProto.h
///
///	Implementation of copy of CT template protocol commands serializer helpers.
///
///	@author krasnov
/// @owner sergeym@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////

#ifndef CCtTemplateProto_H
#define CCtTemplateProto_H

#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

namespace Parallels
{

class CCtTemplateProto : public CDispToDispCommand
{
public:
	CCtTemplateProto(Parallels::IDispToDispCommands nCmdIdentifier, quint32 nFlags = 0);
	bool IsValid();
	quint32 GetVersion();
};

/**
 * Serializer helper class that let to generate and process CopyCtTemplateCmd command
 */
class CCopyCtTemplateCommand : public CCtTemplateProto
{
public:
	/**
	 * Class default constructor.
	 */
	CCopyCtTemplateCommand()
	: CCtTemplateProto(CopyCtTemplateCmd)
	{}
	/**
	 * Class constructor.
	 * @param template name
	 * @param os template name
	 * @param command flags
	 * @param reserved command flags
	 */
	CCopyCtTemplateCommand(
		const QString &sTmplName,
		const QString &sOsTmplName,
		quint32 nFlags,
		quint32 nReservedFlags
	);
	/** Overridden method that let to determine whether protocol command valid */
	bool IsValid();
	QString GetTmplName();
	QString GetOsTmplName();
	quint32 GetReservedFlags();
};

/**
 * Serializer helper class that let to generate and process CopyCtTemplateReply
 */
class CCopyCtTemplateReply : public CCtTemplateProto
{
public:
	/**
	 * Class default constructor.
	 */
	CCopyCtTemplateReply()
	:CCtTemplateProto(CopyCtTemplateReply)
	{}
};

}//namespace Parallels

#endif
