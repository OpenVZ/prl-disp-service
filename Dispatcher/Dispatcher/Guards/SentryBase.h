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
///	SentryBase.h
///
/// @brief
///	Definition of the class SentryBase
///
/// @brief
///	This class defined interface to Sentry
///
/// @author
///	sergeyt@
///
////////////////////////////////////////////////////////////////////////////////
#ifndef HH_SentryBase_HH
#define HH_SentryBase_HH


#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>

class CDspClient;
using namespace IOService;

class SentryBase
{
public:
	virtual ~SentryBase() {}
	virtual PRL_RESULT isCommandAllowed( SmartPtr<CDspClient>& pUser
		, const SmartPtr<IOPackage>& p
		, CVmEvent&  outErrParams ) = 0;
};


#endif // HH_SentryBase_HH
