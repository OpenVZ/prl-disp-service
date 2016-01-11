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
///	DspLogicGuard.h
///
/// @brief
///	Definition of the class DspLogicGuard
///
/// @brief
///	This class implements checking for user commands before theirs executing
///
/// @author sergeyt
///	SergeyT
///
/// @date
///	2006-12-20
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////
#ifndef HH_DspLogicGuard_HH
#define HH_DspLogicGuard_HH

#include <QList>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include "Guards/SentryBase.h"


///////////////////////////////////////////////////////////////////////////////
///
/// @class DspLogicGuard
///
/// @brief Common interface to checking
///
/// @author sergeyt@
///
///////////////////////////////////////////////////////////////////////////////
class DspLogicGuard
{
public:
   DspLogicGuard();

   PRL_RESULT isCommandAllowed( SmartPtr<CDspClient>& pUser
	   , const SmartPtr<IOPackage>& p
	   , CVmEvent&  outErrParams );

private:
	QList< SmartPtr<SentryBase> > m_SentryList;
};


#endif //HH_DspLogicGuard_HH
